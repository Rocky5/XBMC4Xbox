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

#include "music/tags/ImusicInfoTagLoader.h"
#include "FileSystem/File.h"

namespace MUSIC_INFO
{

class CMusicInfoTagLoaderMP4: public IMusicInfoTagLoader
{
public:
  CMusicInfoTagLoaderMP4(void);
  virtual ~CMusicInfoTagLoaderMP4();

  virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);

private:
  unsigned int ReadUnsignedInt( const char* pData );
  void ParseTag( unsigned int metaKey, const char* pMetaData, int metaSize, CMusicInfoTag& tag);
  int GetILSTOffset( const char* pBuffer, int bufferSize );
  int ParseAtom( __int64 startOffset, __int64 stopOffset, CMusicInfoTag& tag );

  unsigned int m_thumbSize;
  BYTE *m_thumbData;
  bool m_isCompilation;

  XFILE::CFile m_file;
};
}
