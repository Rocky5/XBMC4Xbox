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
// NfoFile.h: interface for the CNfoFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NfoFile_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
#define AFX_NfoFile_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "tinyXML/tinyxml.h"
#include "ScraperSettings.h"
#include "utils/CharsetConverter.h"

class CVideoInfoTag;
class CScraperParser;
class CScraperUrl;

class CNfoFile
{
public:
  CNfoFile();
  virtual ~CNfoFile();
  enum NFOResult
  {
    NO_NFO       = 0,
    FULL_NFO     = 1,
    URL_NFO      = 2,
    COMBINED_NFO = 3,
    ERROR_NFO    = 4
  };

  NFOResult Create(const CStdString&,const CStdString&, int episode=-1);
  NFOResult Create(const CStdString&,SScraperInfo&, int episode=-1);
  template<class T>
    bool GetDetails(T& details,const char* document=NULL)
  {
    TiXmlDocument doc;
    CStdString strDoc;
    if (document)
      strDoc = document;
    else
      strDoc = m_headofdoc;
    // try to load using string charset
    if (strDoc.Find("encoding=") == -1)
      g_charsetConverter.unknownToUTF8(strDoc);

    doc.Parse(strDoc.c_str());
    return details.Load(doc.RootElement(),true);
  }

  CStdString m_strScraper;
  CStdString m_strImDbUrl;
  CStdString m_strImDbNr;
  void Close();
  void SetScraperInfo(const SScraperInfo& info) { m_info.Reset(); m_info = info; }
  const SScraperInfo& GetScraperInfo() const { return m_info; }
private:
  int Load(const CStdString&);
  int Scrape(const CStdString&, const CStdString& strURL="");
private:
  char* m_doc;
  char* m_headofdoc;
  int m_size;
  SScraperInfo m_info;
  CStdString m_strContent;
  bool DoScrape(CScraperParser& parser, const CScraperUrl* pURL=NULL, const CStdString& strFunction="NfoUrl");
};

#endif // !defined(AFX_NfoFile_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
