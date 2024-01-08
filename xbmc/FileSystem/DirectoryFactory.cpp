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

#include "system.h"
#include "DirectoryFactory.h"
#include "HDDirectory.h"
#include "SpecialProtocolDirectory.h"
#include "MultiPathDirectory.h"
#include "StackDirectory.h"
#include "FileDirectoryFactory.h"
#include "PlaylistDirectory.h"
#include "MusicDatabaseDirectory.h"
#include "MusicSearchDirectory.h"
#include "VideoDatabaseDirectory.h"
#include "LastFMDirectory.h"
#include "FTPDirectory.h"
#include "HTTPDirectory.h"
#include "DAVDirectory.h"
#include "Application.h"
#include "utils/log.h"

#ifdef HAS_FILESYSTEM_SMB
#ifdef _WIN32PC
#include "WINSMBDirectory.h"
#else
#include "SMBDirectory.h"
#endif
#endif
#ifdef HAS_CCXSTREAM
#include "XBMSDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_CDDA
#include "CDDADirectory.h"
#endif
#include "PluginDirectory.h"
#ifdef HAS_FILESYSTEM
#include "ISO9660Directory.h"
#include "SMBDirectory.h"
#include "XBMSDirectory.h"
#include "CDDADirectory.h"
#include "RTVDirectory.h"
#include "SndtrkDirectory.h"
#include "DAAPDirectory.h"
#include "MemUnitDirectory.h"
#include "HTSPDirectory.h"
#endif
#ifdef HAS_UPNP
#include "UPnPDirectory.h"
#endif
#include "xbox/Network.h"
#include "ZipDirectory.h"
#include "RarDirectory.h"
#include "TuxBoxDirectory.h"
#include "HDHomeRunDirectory.h"
#include "SlingboxDirectory.h"
#include "MythDirectory.h"
#include "FileItem.h"
#include "URL.h"
#include "RSSDirectory.h"

using namespace XFILE;

/*!
 \brief Create a IDirectory object of the share type specified in \e strPath .
 \param strPath Specifies the share type to access, can be a share or share with path.
 \return IDirectory object to access the directories on the share.
 \sa IDirectory
 */
IDirectory* CFactoryDirectory::Create(const CStdString& strPath)
{
  CURL url(strPath);

  CFileItem item;
  IFileDirectory* pDir=CFactoryFileDirectory::Create(strPath, &item);
  if (pDir)
    return pDir;

  CStdString strProtocol = url.GetProtocol();

  if (strProtocol.size() == 0 || strProtocol == "file") return new CHDDirectory();
  if (strProtocol == "special") return new CSpecialProtocolDirectory();
#ifdef HAS_FILESYSTEM_CDDA
  if (strProtocol == "cdda") return new CCDDADirectory();
#endif
#ifdef HAS_FILESYSTEM
  if (strProtocol == "iso9660") return new CISO9660Directory();
  if (strProtocol == "cdda") return new CCDDADirectory();
  if (strProtocol == "soundtrack") return new CSndtrkDirectory();
#endif
  if (strProtocol == "plugin") return new CPluginDirectory();
  if (strProtocol == "zip") return new CZipDirectory();
  if (strProtocol == "rar") return new CRarDirectory();
  if (strProtocol == "multipath") return new CMultiPathDirectory();
  if (strProtocol == "stack") return new CStackDirectory();
  if (strProtocol == "playlistmusic") return new CPlaylistDirectory();
  if (strProtocol == "playlistvideo") return new CPlaylistDirectory();
  if (strProtocol == "musicdb") return new CMusicDatabaseDirectory();
  if (strProtocol == "musicsearch") return new CMusicSearchDirectory();
  if (strProtocol == "videodb") return new CVideoDatabaseDirectory();
  if (strProtocol == "filereader") 
    return CFactoryDirectory::Create(url.GetFileName());
#ifdef HAS_XBOX_HARDWARE
  if (strProtocol.Left(3) == "mem") return new CMemUnitDirectory();
#endif

  if( g_application.getNetwork().IsAvailable(true) )
  {
    if (strProtocol == "lastfm") return new CLastFMDirectory();
    if (strProtocol == "tuxbox") return new CDirectoryTuxBox();
    if (strProtocol == "ftp" ||  strProtocol == "ftpx" ||  strProtocol == "ftps") return new CFTPDirectory();
    if (strProtocol == "http" || strProtocol == "https") return new CHTTPDirectory();
    if (strProtocol == "dav" || strProtocol == "davs") return new CDAVDirectory();
#ifdef HAS_FILESYSTEM
    if (strProtocol == "smb") return new CSMBDirectory();
    if (strProtocol == "daap") return new CDAAPDirectory();
    if (strProtocol == "xbms") return new CXBMSDirectory();
    if (strProtocol == "rtv") return new CRTVDirectory();
    if (strProtocol == "htsp") return new CHTSPDirectory();
#endif
#ifdef HAS_UPNP
    if (strProtocol == "upnp") return new CUPnPDirectory();
#endif
    if (strProtocol == "hdhomerun") return new CHomeRunDirectory();
    if (strProtocol == "sling") return new CSlingboxDirectory();
    if (strProtocol == "myth") return new CMythDirectory();
    if (strProtocol == "cmyth") return new CMythDirectory();
    if (strProtocol == "rss") return new CRSSDirectory();
  }

  CLog::Log(LOGWARNING, "%s - Unsupported protocol(%s) in %s", __FUNCTION__, strProtocol.c_str(), url.Get().c_str() );
  return NULL;
}

