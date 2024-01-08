#pragma once

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

#include <map>
#include <vector>

#include "utils/ScraperUrl.h"
#include "utils/Fanart.h"

class CArtist
{
public:
  long idArtist;
  bool operator<(const CArtist& a) const
  {
    return strArtist < a.strArtist;
  }

  void Reset()
  {
    strArtist.Empty();
    strGenre.Empty();
    strBiography.Empty();
    strStyles.Empty();
    strMoods.Empty();
    strInstruments.Empty();
    strBorn.Empty();
    strFormed.Empty();
    strDied.Empty();
    strDisbanded.Empty();
    strYearsActive.Empty();
    thumbURL.Clear();
    discography.clear();
    idArtist = -1;
  }

  bool Load(const TiXmlElement *movie, bool chained=false);
  bool Save(TiXmlNode *node, const CStdString &tag, const CStdString& strPath);

  CStdString strArtist;
  CStdString strGenre;
  CStdString strBiography;
  CStdString strStyles;
  CStdString strMoods;
  CStdString strInstruments;
  CStdString strBorn;
  CStdString strFormed;
  CStdString strDied;
  CStdString strDisbanded;
  CStdString strYearsActive;
  CScraperUrl thumbURL;
  CFanart fanart;
  std::vector<std::pair<CStdString,CStdString> > discography;
};

typedef std::vector<CArtist> VECARTISTS;
