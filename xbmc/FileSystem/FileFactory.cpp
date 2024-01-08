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
#include "FileFactory.h"
#include "HDFile.h"
#include "CurlFile.h"
#include "HTTPFile.h"
#include "DAVFile.h"
#include "ShoutcastFile.h"
#include "LastFMFile.h"
#include "FileReaderFile.h"
#ifdef HAS_FILESYSTEM
#include "ISOFile.h"
#include "SMBFile.h"
#include "XBMSPFile.h"
#include "RTVFile.h"
#include "SndtrkFile.h"
#include "CDDAFile.h"
#include "MemUnitFile.h"
#include "DAAPFile.h"
#endif
#include "ZipFile.h"
#include "RarFile.h"
#include "UPnPFile.h"
#include "MusicDatabaseFile.h"
#include "SpecialProtocolFile.h"
#include "MultiPathFile.h"
#include "Application.h"
#include "TuxBoxFile.h"
#include "HDHomeRunFile.h"
#include "SlingboxFile.h"
#include "MythFile.h"
#include "URL.h"
#include "utils/log.h"

using namespace XFILE;

CFileFactory::CFileFactory()
{
}

CFileFactory::~CFileFactory()
{
}

IFile* CFileFactory::CreateLoader(const CStdString& strFileName)
{
  CURL url(strFileName);
  return CreateLoader(url);
}

IFile* CFileFactory::CreateLoader(const CURL& url)
{
  CStdString strProtocol = url.GetProtocol();
  strProtocol.MakeLower();

  if (strProtocol == "zip") return new CZipFile();
  else if (strProtocol == "rar") return new CRarFile();
  else if (strProtocol == "musicdb") return new CMusicDatabaseFile();
  else if (strProtocol == "videodb") return NULL;
  else if (strProtocol == "special") return new CSpecialProtocolFile();
  else if (strProtocol == "multipath") return new CMultiPathFile();
  else if (strProtocol == "file" || strProtocol.IsEmpty()) return new CFileHD();
  else if (strProtocol == "filereader") return new CFileFileReader();
#ifdef HAS_FILESYSTEM
  else if (strProtocol == "iso9660") return new CISOFile();
  else if (strProtocol == "soundtrack") return new CSndtrkFile();
  else if (strProtocol == "cdda") return new CCDDAFile();
  else if (strProtocol.Left(3) == "mem") return new CMemUnitFile();
#endif
  if( g_application.getNetwork().IsAvailable() )
  {
    if (strProtocol == "ftp"
    ||  strProtocol == "ftpx"
    ||  strProtocol == "ftps"
    ||  strProtocol == "rss") return new CCurlFile();
    else if (strProtocol == "http" ||  strProtocol == "https") return new CHTTPFile();
    else if (strProtocol == "dav" || strProtocol == "davs") return new CDAVFile();
    else if (strProtocol == "shout") return new CShoutcastFile();
    else if (strProtocol == "lastfm") return new CLastFMFile();
    else if (strProtocol == "tuxbox") return new CTuxBoxFile();
    else if (strProtocol == "hdhomerun") return new CHomeRunFile();
    else if (strProtocol == "sling") return new CSlingboxFile();
    else if (strProtocol == "myth") return new CMythFile();
    else if (strProtocol == "cmyth") return new CMythFile();
#ifdef HAS_FILESYSTEM
    else if (strProtocol == "smb") return new CSmbFile();
    else if (strProtocol == "xbms") return new CXBMSPFile();
    else if (strProtocol == "rtv") return new CRTVFile();
    else if (strProtocol == "daap") return new CDAAPFile();
    else if (strProtocol == "upnp") return new CUPnPFile();
#endif
  }

  CLog::Log(LOGWARNING, "%s - Unsupported protocol(%s) in %s", __FUNCTION__, strProtocol.c_str(), url.Get().c_str() );
  return NULL;
}
