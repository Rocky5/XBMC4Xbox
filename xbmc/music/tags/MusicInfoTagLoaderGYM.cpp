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

#include "MusicInfoTagLoaderGYM.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/log.h"

#include <fstream>

using namespace MUSIC_INFO;

CMusicInfoTagLoaderGYM::CMusicInfoTagLoaderGYM(void)
{
  m_gym = 0;
}

CMusicInfoTagLoaderGYM::~CMusicInfoTagLoaderGYM()
{
}

bool CMusicInfoTagLoaderGYM::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  tag.SetLoaded(false);

  if (!m_dll.Load())
    return false;

  m_dll.Init();

  m_gym = m_dll.LoadGYM(strFileName.c_str());
  if (!m_gym)
  {
    CLog::Log(LOGERROR,"MusicInfoTagLoaderGYM: failed to open GYM %s",strFileName.c_str());
    return false;
  }

  tag.SetURL(strFileName);

  tag.SetLoaded(false);
  char* szTitle = (char*)m_dll.GetTitle(m_gym); // no alloc
  if (szTitle)
    if( strcmp(szTitle,"") )
    {
      tag.SetTitle(szTitle);
      tag.SetLoaded(true);
    }

  char* szArtist = (char*)m_dll.GetArtist(m_gym); // no alloc
  if (szArtist)
    if( strcmp(szArtist,"") && tag.Loaded() )
      tag.SetArtist(szArtist);

  m_dll.FreeGYM(m_gym);
  m_gym = 0;

  return tag.Loaded();
}
