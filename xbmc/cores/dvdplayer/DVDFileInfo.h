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
#pragma once

#include "utils/StdString.h"

class CFileItem;
class CDVDDemux;
class CStreamDetails;

class CDVDFileInfo
{
public:
  // Extract a thumbnail immage from the media at strPath an image file in strTarget, optionally populating a streamdetails class with the data
  static bool ExtractThumb(const CStdString &strPath, const CStdString &strTarget, CStreamDetails *pStreamDetails);
  
  // GetFileMetaData will fill pItem's properties according to what can be extracted from the file.
  static void GetFileMetaData(const CStdString &strPath, CFileItem *pItem); 
  
  // Probe the files streams and store the info in the VideoInfoTag
  static bool GetFileStreamDetails(CFileItem *pItem);
  static bool DemuxerToStreamDetails(CDVDDemux *pDemux, CStreamDetails &details, const CStdString &path = "");

  static bool GetFileDuration(const CStdString &path, int &duration);
};
