#ifndef SCRAPER_PARSER_H
#define SCRAPER_PARSER_H

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

#include <vector>
#include "tinyXML/tinyxml.h"
#include "utils/StdString.h"
#include "XBDateTime.h"

#define MAX_SCRAPER_BUFFERS 20

class CScraperSettings;

class CScraperParser
{
public:
  CScraperParser();
  CScraperParser(const CScraperParser& parser);
  ~CScraperParser();
  CScraperParser& operator= (const CScraperParser& parser);

  void Clear();
  bool Load(const CStdString& strXMLFile);
  const CStdString GetName() { return m_name; }
  const CStdString GetThumb() { return m_thumb; }
  const CStdString GetContent() { return m_content; }
  const CStdString GetLanguage() { return m_language; }
  const CStdString GetFramework() { return m_framework; }
  const CStdString GetDate() { return m_date; }
  const CStdString GetFilename() { return m_strFile; }
  const CStdString GetSearchStringEncoding() { return m_SearchStringEncoding; }
  const CStdString Parse(const CStdString& strTag, const CScraperSettings* pSettings=NULL);
  bool HasFunction(const CStdString& strTag);
  bool RequiresSettings() { return m_requiressettings; }

  CStdString m_param[MAX_SCRAPER_BUFFERS];
  void ClearCache();

private:
  bool LoadFromXML();
  void ReplaceBuffers(CStdString& strDest);
  void ParseExpression(const CStdString& input, CStdString& dest, TiXmlElement* element, bool bAppend);
  void ParseNext(TiXmlElement* element);
  void Clean(CStdString& strDirty);
  /*! \brief Remove spaces, tabs, and newlines from a string
   \param string the string in question, which will be modified.
   */
  void RemoveWhiteSpace(CStdString &string);
  void ConvertJSON(CStdString &string);
  void ClearBuffers();
  void GetBufferParams(bool* result, const char* attribute, bool defvalue);
  void InsertToken(CStdString& strOutput, int buf, const char* token);

  TiXmlDocument* m_document;
  TiXmlElement* m_pRootElement;

  const char* m_name;
  const char* m_thumb;
  const char* m_content;
  const char* m_language;
  const char* m_framework;
  const char* m_date;
  const char* m_SearchStringEncoding;
  CDateTimeSpan m_persistence;
  bool m_requiressettings;

  CStdString m_strFile;

  const CScraperSettings* m_settings;
};

#endif


