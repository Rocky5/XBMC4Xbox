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


// FileShoutcast.cpp: implementation of the CShoutcastFile class.
//
//////////////////////////////////////////////////////////////////////

#include "settings/AdvancedSettings.h"
#include "ShoutcastFile.h"
#include "settings/GUISettings.h"
#include "dialogs/GUIDialogProgress.h"
#include "GUIWindowManager.h"
#include "URL.h"
#include "LocalizeStrings.h"
#include "utils/log.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/* needed for the shoutcast headers */
#if defined(_XBOX) && !defined(WIN32)
#define WIN32 1
#endif

#include "lib/libshout/types.h"
#include "lib/libshout/rip_manager.h"
#include "lib/libshout/util.h"
#include "lib/libshout/filelib.h"
#include "RingBuffer.h"
#include "ShoutcastRipFile.h"
#include "GUIInfoManager.h"

using namespace MUSIC_INFO;
using namespace XFILE;

const int SHOUTCASTTIMEOUT = 60;
static CRingBuffer m_ringbuf;

static FileState m_fileState;
static CShoutcastRipFile m_ripFile;


static RIP_MANAGER_INFO m_ripInfo;
static ERROR_INFO m_errorInfo;



void rip_callback(int message, void *data)
{
  switch (message)
  {
    RIP_MANAGER_INFO *info;
  case RM_UPDATE:
    info = (RIP_MANAGER_INFO*)data;
    memcpy(&m_ripInfo, info, sizeof(m_ripInfo));
    if (info->status == RM_STATUS_BUFFERING)
    {
      m_fileState.bBuffering = true;
    }
    else if ( info->status == RM_STATUS_RIPPING)
    {
      m_ripFile.SetRipManagerInfo( &m_ripInfo );
      m_fileState.bBuffering = false;
    }
    else if (info->status == RM_STATUS_RECONNECTING)
    {}
    break;
  case RM_ERROR:
    ERROR_INFO *errInfo;
    errInfo = (ERROR_INFO*)data;
    memcpy(&m_errorInfo, errInfo, sizeof(m_errorInfo));
    m_fileState.bRipError = true;
    OutputDebugString("error\n");
    break;
  case RM_DONE:
    OutputDebugString("done\n");
    m_fileState.bRipDone = true;
    break;
  case RM_NEW_TRACK:
    char *trackName;
    trackName = (char*) data;
    m_ripFile.SetTrackname( trackName );
    break;
  case RM_STARTED:
    m_fileState.bRipStarted = true;
    OutputDebugString("Started\n");
    break;
  }

}

error_code filelib_write(char *buf, u_long size)
{
  if ((int)size > m_ringbuf.getSize())
  {
    CLog::Log(LOGERROR, "Shoutcast chunk too big: %lu", size);
    return SR_ERROR_BUFFER_FULL;
  }
  while (m_ringbuf.getMaxWriteSize() < (int)size) Sleep(10);
  m_ringbuf.WriteData(buf, size);
  m_ripFile.Write( buf, size ); //will only write, if it has to
  return SR_SUCCESS;
}
CShoutcastFile* m_pShoutCastRipper = NULL;

CShoutcastFile::CShoutcastFile()
{
  // FIXME: without this check
  // the playback stops when CFile::Stat()
  // or CFile::Exists() is called

  // Do we already have another file
  // using the ripper?
  if (!m_pShoutCastRipper)
  {
    m_fileState.bBuffering = true;
    m_fileState.bRipDone = false;
    m_fileState.bRipStarted = false;
    m_fileState.bRipError = false;
    m_ringbuf.Create(1024*256);
    m_pShoutCastRipper = this;
  }
}

CShoutcastFile::~CShoutcastFile()
{
  // FIXME: without this check
  // the playback stops when CFile::Stat()
  // or CFile::Exists() is called

  // Has this object initialized the ripper?
  if (m_pShoutCastRipper==this)
  {
    rip_manager_stop();
    m_pShoutCastRipper = NULL;
    m_ripFile.Reset();
    m_ringbuf.Destroy();
  }
}

bool CShoutcastFile::CanRecord()
{
  if ( !m_fileState.bRipStarted )
    return false;
  return m_ripFile.CanRecord();
}

bool CShoutcastFile::Record()
{
  return m_ripFile.Record();
}

void CShoutcastFile::StopRecording()
{
  m_ripFile.StopRecording();
}


int64_t CShoutcastFile::GetPosition()
{
  return 0;
}

int64_t CShoutcastFile::GetLength()
{
  return 0;
}


bool CShoutcastFile::Open(const CURL& url)
{
  m_dwLastTime = timeGetTime();
  int ret;
  RIP_MANAGER_OPTIONS m_opt;
  m_opt.relay_port = 8000;
  m_opt.max_port = 18000;
  m_opt.flags = OPT_AUTO_RECONNECT |
                OPT_SEPERATE_DIRS |
                OPT_SEARCH_PORTS |
                OPT_ADD_ID3;

  // No progress dialog for now as it may deadlock when DVDplayer is used
  CGUIDialogProgress* dlgProgress = NULL;
  //CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

  strcpy(m_opt.output_directory, "./");
  m_opt.proxyurl[0] = '\0';

  // Use a proxy, if the GUI was configured as such
  bool bProxyEnabled = g_guiSettings.GetBool("network.usehttpproxy");
  if (bProxyEnabled)
  {
    const CStdString &strProxyServer = g_guiSettings.GetString("network.httpproxyserver");
    const CStdString &strProxyPort = g_guiSettings.GetString("network.httpproxyport");
	  // Should we check for valid strings here
	  _snprintf( m_opt.proxyurl, MAX_URL_LEN, "http://%s:%s", strProxyServer.c_str(), strProxyPort.c_str() );
  }

  CStdString strUrl = url.Get();
  strUrl.Replace("shout://", "http://");
  strncpy(m_opt.url, strUrl.c_str(), MAX_URL_LEN);
  sprintf(m_opt.useragent, "x%s", url.GetFileName().c_str());
  if (dlgProgress)
  {
    dlgProgress->SetHeading(260);
    dlgProgress->SetLine(0, 259);
    dlgProgress->SetLine(1, strUrl);
    dlgProgress->SetLine(2, "");
    if (!dlgProgress->IsDialogRunning())
      dlgProgress->StartModal();
    dlgProgress->Progress();
  }

  if ((ret = rip_manager_start(rip_callback, &m_opt)) != SR_SUCCESS)
  {
    if (dlgProgress) dlgProgress->Close();
    return false;
  }
  int iShoutcastTimeout = 10 * SHOUTCASTTIMEOUT; //i.e: 10 * 10 = 100 * 100ms = 10s
  int iCount = 0;
  while (!m_fileState.bRipDone && !m_fileState.bRipStarted && !m_fileState.bRipError)
  {
    if (iCount <= iShoutcastTimeout) //Normally, this isn't the problem,
      //because if RIP_MANAGER fails, this would be here
      //with m_fileState.bRipError
    {
      Sleep(100);
    }
    else
    {
      if (dlgProgress)
      {
        dlgProgress->SetLine(1, 257);
        dlgProgress->SetLine(2, "Connection timed out...");
        Sleep(1500);
        dlgProgress->Close();
      }
      return false;
    }
    iCount++;
  }

  /* store content type of stream */
  m_mimetype = m_ripInfo.contenttype;

  //CHANGED CODE: Don't reset timer anymore.

  while (!m_fileState.bRipDone && !m_fileState.bRipError && m_fileState.bBuffering)
  {
    if (iCount <= iShoutcastTimeout) //Here is the real problem: Sometimes the buffer fills just to
      //slowly, thus the quality of the stream will be bad, and should be
      //aborted...
    {
      Sleep(100);
      char szTmp[1024];
      //g_dialog.SetCaption(0, "Shoutcast" );
      sprintf(szTmp, g_localizeStrings.Get(23052).c_str(), m_ringbuf.getMaxReadSize());
      if (dlgProgress)
      {
        dlgProgress->SetLine(2, szTmp );
        dlgProgress->Progress();
      }

      sprintf(szTmp, "%s", m_ripInfo.filename);
      for (int i = 0; i < (int)strlen(szTmp); i++)
        szTmp[i] = tolower((unsigned char)szTmp[i]);
      szTmp[50] = 0;
      if (dlgProgress)
      {
        dlgProgress->SetLine(1, szTmp );
        dlgProgress->Progress();
      }
    }
    else //it's not really a connection timeout, but it's here,
      //where things get boring, if connection is slow.
      //trust me, i did a lot of testing... Doesn't happen often,
      //but if it does it sucks to wait here forever.
      //CHANGED: Other message here
    {
      if (dlgProgress)
      {
        dlgProgress->SetLine(1, 257);
        dlgProgress->SetLine(2, "Connection to server too slow...");
        dlgProgress->Close();
      }
      return false;
    }
    iCount++;
  }
  if ( m_fileState.bRipError )
  {
    if (dlgProgress)
    {
      dlgProgress->SetLine(1, 257);
      dlgProgress->SetLine(2, 16029);
      CLog::Log(LOGERROR, "%s: error - %s", __FUNCTION__, m_errorInfo.error_str);
      dlgProgress->Progress();

      Sleep(1500);
      dlgProgress->Close();
    }
    return false;
  }
  if (dlgProgress)
  {
    dlgProgress->SetLine(2, 261);
    dlgProgress->Progress();
    dlgProgress->Close();
  }
  return true;
}

unsigned int CShoutcastFile::Read(void* lpBuf, int64_t uiBufSize)
{
  if (m_fileState.bRipDone)
  {
    OutputDebugString("Read done\n");
    return 0;
  }

  int slept=0;
  while (m_ringbuf.getMaxReadSize() <= 0)
  {
    Sleep(10);
    if (slept += 10 > SHOUTCASTTIMEOUT*1000)
      return -1;
  }

  int iRead = m_ringbuf.getMaxReadSize();
  if (iRead > uiBufSize) iRead = (int)uiBufSize;
  m_ringbuf.ReadData((char*)lpBuf, iRead);

  if (timeGetTime() - m_dwLastTime > 500)
  {
    m_dwLastTime = timeGetTime();
    CMusicInfoTag tag;
    GetMusicInfoTag(tag);
    g_infoManager.SetCurrentSongTag(tag);
  }
  return iRead;
}

void CShoutcastFile::outputTimeoutMessage(const char* message)
{
  //g_dialog.SetCaption(0, "Shoutcast"  );
  //g_dialog.SetMessage(0,  message );
  //g_dialog.Render();
  Sleep(1500);
}

int64_t CShoutcastFile::Seek(int64_t iFilePosition, int iWhence)
{
  return -1;
}

void CShoutcastFile::Close()
{
  OutputDebugString("Shoutcast Stopping\n");
  if ( m_ripFile.IsRecording() )
    m_ripFile.StopRecording();
  m_ringbuf.Clear();
  rip_manager_stop();
  m_ripFile.Reset();
  OutputDebugString("Shoutcast Stopped\n");
}

bool CShoutcastFile::IsRecording()
{
  return m_ripFile.IsRecording();
}



bool CShoutcastFile::GetMusicInfoTag(CMusicInfoTag& tag)
{
  m_ripFile.GetMusicInfoTag(tag);
  return true;
}

CStdString CShoutcastFile::GetContent()
{
  return m_mimetype;
}
