/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "XMLUtils.h"
#include "ScraperSettings.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "FileSystem/CurlFile.h"
#include "utils/ScraperParser.h"
#include "utils/ScraperUrl.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"

#include <sstream>

using namespace std;

CScraperSettings::CScraperSettings()
{
}

CScraperSettings::~CScraperSettings()
{
}

bool CScraperSettings::Load(const CStdString& strSettings, const CStdString& strSaved)
{
  if (!LoadSettingsXML(strSettings))
    return false;
  if (!LoadUserXML(strSaved))
    return false;

  return true;
}

bool CScraperSettings::LoadUserXML(const CStdString& strSaved)
{
  m_userXmlDoc.Clear();
  m_userXmlDoc.Parse(strSaved.c_str());

  return m_userXmlDoc.RootElement()?true:false;
}

bool CScraperSettings::LoadSettingsXML(const CStdString& strScraper, const CStdString& strFunction, const CScraperUrl* url)
{
  CScraperParser parser;
  // load our scraper xml
  if (!parser.Load(strScraper))
    return false;

  if (!parser.HasFunction(strFunction))
    return false;

  if (!url && strFunction.Equals("GetSettings")) // entry point
    m_pluginXmlDoc.Clear();
    
  vector<CStdString> strHTML;
  if (url)
  {
    XFILE::CCurlFile http;
    for (unsigned int i=0;i<url->m_url.size();++i)
    {
      CStdString strCurrHTML;
      if (!CScraperUrl::Get(url->m_url[i],strCurrHTML,http,parser.GetFilename()) || strCurrHTML.size() == 0)
        return false;
      strHTML.push_back(strCurrHTML);
    }
  }

  // now grab our details using the scraper
  for (unsigned int i=0;i<strHTML.size();++i)
    parser.m_param[i] = strHTML[i];

  CStdString strXML = parser.Parse(strFunction);
  if (strXML.IsEmpty())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse web site",__FUNCTION__);
    return false;
  }
  // abit ugly, but should work. would have been better if parser
  // set the charset of the xml, and we made use of that
  if (!XMLUtils::HasUTF8Declaration(strXML))
    g_charsetConverter.unknownToUTF8(strXML);
  
  // ok, now parse the xml file
  TiXmlDocument doc;
  doc.Parse(strXML.c_str(),0,TIXML_ENCODING_UTF8);
  if (!doc.RootElement())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse xml",__FUNCTION__);
    return false;
  }

  // check our document
  if (!m_pluginXmlDoc.RootElement())
  {
    TiXmlElement xmlRootElement("settings");
    m_pluginXmlDoc.InsertEndChild(xmlRootElement);
  }

  // loop over all tags and append any setting tags
  TiXmlElement* pElement = doc.RootElement()->FirstChildElement("setting");
  while (pElement)
  {
    m_pluginXmlDoc.RootElement()->InsertEndChild(*pElement);
    pElement = pElement->NextSiblingElement("setting");
  }

  // and call any chains
  TiXmlElement* pRoot = doc.RootElement();
  TiXmlElement* xurl = pRoot->FirstChildElement("url");
  while (xurl && xurl->FirstChild())
  {
    const char* szFunction = xurl->Attribute("function");
    if (szFunction)
    {      
      CScraperUrl scrURL(xurl);
      if (!LoadSettingsXML(strScraper,szFunction,&scrURL))
        return false;
    }
    xurl = xurl->NextSiblingElement("url");
  }

  return m_pluginXmlDoc.RootElement()?true:false;
}

CStdString CScraperSettings::GetSettings() const
{
  stringstream stream;
  if (m_userXmlDoc.RootElement())
    stream << *m_userXmlDoc.RootElement();

  return stream.str();
}

