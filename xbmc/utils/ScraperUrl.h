#ifndef SCRAPER_URL_H
#define SCRAPER_URL_H

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

#include "tinyXML/tinyxml.h"
#include "utils/StdString.h"


#include <vector>

namespace XFILE { class CCurlFile; }

class CScraperUrl
{
public:
  CScraperUrl(const CStdString&);
  CScraperUrl(const TiXmlElement*);
  CScraperUrl();
  ~CScraperUrl();

  enum URLTYPES
  {
    URL_TYPE_GENERAL = 1,
    URL_TYPE_SEASON = 2
  };

  struct SUrlEntry
  {
    CStdString m_spoof;
    CStdString m_url;
    CStdString m_cache;
    URLTYPES m_type;
    bool m_post;
    bool m_isgz;
    int m_season;
  };

  bool Parse();
  bool ParseString(CStdString); // copies by intention
  bool ParseElement(const TiXmlElement*);
  bool ParseEpisodeGuide(CStdString strUrls); // copies by intention

  const SUrlEntry GetFirstThumb() const;
  const SUrlEntry GetSeasonThumb(int) const;
  void Clear();
  static bool Get(const SUrlEntry&, std::string&, XFILE::CCurlFile& http,
                 const CStdString& cacheContext);
  static bool DownloadThumbnail(const CStdString &thumb, const SUrlEntry& entry);
  static void ClearCache();

  CStdString m_xml;
  CStdString m_spoof; // for backwards compatibility only!
  CStdString strTitle;
  CStdString strId;
  double relevance;
  std::vector<SUrlEntry> m_url;
};

#endif


