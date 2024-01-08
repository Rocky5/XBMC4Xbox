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


#include "OGGFileDirectory.h"
#include "music/tags/OggTag.h"

using namespace MUSIC_INFO;
using namespace XFILE;

COGGFileDirectory::COGGFileDirectory(void)
{
  m_strExt = "oggstream";
}

COGGFileDirectory::~COGGFileDirectory(void)
{
}

int COGGFileDirectory::GetTrackCount(const CStdString& strPath)
{
  COggTag tag;
  return tag.GetStreamCount(strPath);
}
