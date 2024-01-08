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

// FileShoutcast.h: interface for the CShoutcastFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILESHOUTCAST_H__6B6082E6_547E_44C4_8801_9890781659C0__INCLUDED_)
#define AFX_FILESHOUTCAST_H__6B6082E6_547E_44C4_8801_9890781659C0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"
#include "utils/StdString.h"

namespace MUSIC_INFO
{
  class CMusicInfoTag;
}

namespace XFILE
{
typedef struct FileStateSt
{
  bool bBuffering;
  bool bRipDone;
  bool bRipStarted;
  bool bRipError;
}
FileState;

class CShoutcastFile : public IFile
{
public:
  CShoutcastFile();
  virtual ~CShoutcastFile();
  virtual int64_t GetPosition();
  virtual int64_t GetLength();
  virtual bool Open(const CURL& url);
  virtual bool Exists(const CURL& url) { return true;};
  virtual int Stat(const CURL& url, struct __stat64* buffer) { errno = ENOENT; return -1; };
  virtual unsigned int Read(void* lpBuf, int64_t uiBufSize);
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
  virtual bool CanRecord();
  virtual bool Record();
  virtual void StopRecording();
  virtual bool IsRecording();
  virtual bool GetMusicInfoTag(MUSIC_INFO::CMusicInfoTag& tag);
  virtual CStdString GetContent();
protected:
  void outputTimeoutMessage(const char* message);
  DWORD m_dwLastTime;
  CStdString m_mimetype;
};
}
#endif // !defined(AFX_FILESHOUTCAST_H__6B6082E6_547E_44C4_8801_9890781659C0__INCLUDED_)
