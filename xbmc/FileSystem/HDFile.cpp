
#include "system.h"
#include "utils/log.h"
/*
 * XBMC Media Center
 * Copyright (c) 2002 Frodo
 * Portions Copyright (c) by the authors of ffmpeg and xvid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "HDFile.h"
#include "Util.h"
#include "URL.h"
#include "settings/GUISettings.h"
#include "utils/CharsetConverter.h"
#include "utils/CriticalSection.h"

#include <sys/stat.h>

using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//*********************************************************************************************
CFileHD::CFileHD()
    : m_hFile(INVALID_HANDLE_VALUE)
{}

//*********************************************************************************************
CFileHD::~CFileHD()
{
  if (m_hFile != INVALID_HANDLE_VALUE) Close();
}
//*********************************************************************************************
CStdString CFileHD::GetLocal(const CURL &url)
{
  CStdString path( url.GetFileName() );

  if( url.GetProtocol().Equals("file", false) )
  {
    // file://drive[:]/path
    // file:///drive:/path
    CStdString host( url.GetHostName() );

    if(host.size() > 0)
    {
      if(host.Right(1) == ":")
        path = host + "/" + path;
      else
        path = host + ":/" + path;
    }
  }

  path.Replace('/', '\\');
  g_charsetConverter.utf8ToStringCharset(path);
  return path;
}

//*********************************************************************************************
bool CFileHD::Open(const CURL& url)
{
  CStdString strFile = GetLocal(url);

  m_hFile.attach(CreateFile(strFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL));
  if (!m_hFile.isValid()) return false;

  m_i64FilePos = 0;
  m_i64FileLen = 0;

  LARGE_INTEGER i64Size;
  GetFileSizeEx((HANDLE)m_hFile, &i64Size);
  m_i64FileLength = i64Size.QuadPart;

  return true;
}

bool CFileHD::Exists(const CURL& url)
{
  CStdString strFile = GetLocal(url);

  struct __stat64 buffer;
  return (_stat64(strFile.c_str(), &buffer)==0);
}


int CFileHD::Stat(const CURL& url, struct __stat64* buffer)
{
  CStdString strFile = GetLocal(url);

  return _stat64(strFile.c_str(), buffer);
}


//*********************************************************************************************
bool CFileHD::OpenForWrite(const CURL& url, bool bOverWrite)
{
  // make sure it's a legal FATX filename (we are writing to the harddisk)
  CStdString strPath = GetLocal(url);

  if (g_guiSettings.GetBool("services.ftpautofatx")) // allow overriding
  {
    CStdString strPathOriginal = strPath;
    CUtil::GetFatXQualifiedPath(strPath);
    if (strPathOriginal != strPath)
      CLog::Log(LOGINFO,"CFileHD::OpenForWrite: WARNING: Truncated filename %s %s", strPathOriginal.c_str(), strPath.c_str());
  }
  
  m_hFile.attach(CreateFile(strPath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, bOverWrite ? CREATE_ALWAYS : OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
  if (!m_hFile.isValid()) 
    return false;

  m_i64FilePos = 0;
  LARGE_INTEGER i64Size;
  GetFileSizeEx((HANDLE)m_hFile, &i64Size);
  m_i64FileLength = i64Size.QuadPart;
  Seek(0, SEEK_SET);

  return true;
}

//*********************************************************************************************
unsigned int CFileHD::Read(void *lpBuf, int64_t uiBufSize)
{
  if (!m_hFile.isValid()) return 0;
  DWORD nBytesRead;
  if ( ReadFile((HANDLE)m_hFile, lpBuf, (DWORD)uiBufSize, &nBytesRead, NULL) )
  {
    m_i64FilePos += nBytesRead;
    return nBytesRead;
  }
  return 0;
}

//*********************************************************************************************
int CFileHD::Write(const void *lpBuf, int64_t uiBufSize)
{
  if (!m_hFile.isValid())
    return 0;

  DWORD nBytesWriten;
  if ( WriteFile((HANDLE)m_hFile, (void*) lpBuf, (DWORD)uiBufSize, &nBytesWriten, NULL) )
    return nBytesWriten;

  return 0;
}

//*********************************************************************************************
void CFileHD::Close()
{
  m_hFile.reset();
}

//*********************************************************************************************
int64_t CFileHD::Seek(int64_t iFilePosition, int iWhence)
{
  LARGE_INTEGER lPos, lNewPos;
  lPos.QuadPart = iFilePosition;
  int bSuccess;

  switch (iWhence)
  {
  case SEEK_SET:
    bSuccess = SetFilePointerEx((HANDLE)m_hFile, lPos, &lNewPos, FILE_BEGIN);
    break;

  case SEEK_CUR:
    bSuccess = SetFilePointerEx((HANDLE)m_hFile, lPos, &lNewPos, FILE_CURRENT);
    break;

  case SEEK_END:
    bSuccess = SetFilePointerEx((HANDLE)m_hFile, lPos, &lNewPos, FILE_END);
    break;

  default:
    return -1;
  }
  if (bSuccess)
  {
    m_i64FilePos = lNewPos.QuadPart;
    return m_i64FilePos;
  }
  else
    return -1;
}

//*********************************************************************************************
int64_t CFileHD::GetLength()
{
  if(m_i64FileLen <= m_i64FilePos || m_i64FileLen == 0)
  {
    LARGE_INTEGER i64Size;
    if(GetFileSizeEx((HANDLE)m_hFile, &i64Size))
      m_i64FileLen = i64Size.QuadPart;
    else
      CLog::Log(LOGERROR, "CFileHD::GetLength - GetFileSizeEx failed with error %d", GetLastError());
  }
  return m_i64FileLen;
}

//*********************************************************************************************
int64_t CFileHD::GetPosition()
{
  return m_i64FilePos;
}

bool CFileHD::Delete(const CURL& url)
{
  CStdString strFile=GetLocal(url);

  return ::DeleteFile(strFile.c_str()) ? true : false;
}

bool CFileHD::Rename(const CURL& url, const CURL& urlnew)
{
  CStdString strFile=GetLocal(url);
  CStdString strNewFile=GetLocal(urlnew);

  return ::MoveFile(strFile.c_str(), strNewFile.c_str()) ? true : false;
}

void CFileHD::Flush()
{
  ::FlushFileBuffers(m_hFile);
}

int CFileHD::IoControl(EIoControl request, void* param)
{
#ifdef _LINUX
  if(request == IOCTRL_NATIVE && param)
  {
    SNativeIoControl* s = (SNativeIoControl*)param;
    return ioctl((*m_hFile).fd, s->request, s->param);
  }
#endif
  return -1;
}
