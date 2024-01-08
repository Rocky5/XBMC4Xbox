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
#include "utils/log.h"
#include "utils/SingleLock.h"
#include "DVDAudio.h"
#ifdef _XBOX
#include "cores/mplayer/ASyncDirectSound.h"
#include "cores/mplayer/ac97directsound.h"
#else
#include "cores/mplayer/Win32DirectSound.h"
#endif
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDPlayerAudio.h"

using namespace std;


CPTSOutputQueue::CPTSOutputQueue()
{
  Flush();
}

void CPTSOutputQueue::Add(double pts, double delay, double duration, double timestamp)
{
  CSingleLock lock(m_sync);

  // don't accept a re-add, since that would cause time moving back
  double last = m_queue.empty() ? m_current.pts : m_queue.back().pts;
  if(last == pts)
    return;

  TPTSItem item;
  item.pts = pts;
  item.timestamp = timestamp + delay;
  item.duration = duration;

  // first one is applied directly
  if(m_queue.empty() && m_current.pts == DVD_NOPTS_VALUE)
    m_current = item;
  else
    m_queue.push(item);

  // call function to make sure the queue
  // doesn't grow should nobody call it
  Current(timestamp);
}
void CPTSOutputQueue::Flush()
{
  CSingleLock lock(m_sync);

  while( !m_queue.empty() ) m_queue.pop();
  m_current.pts = DVD_NOPTS_VALUE;
  m_current.timestamp = 0.0;
  m_current.duration = 0.0;
}

double CPTSOutputQueue::Current(double timestamp)
{
  CSingleLock lock(m_sync);

  if(!m_queue.empty() && m_current.pts == DVD_NOPTS_VALUE)
  {
    m_current = m_queue.front();
    m_queue.pop();
  }

  while( !m_queue.empty() && timestamp >= m_queue.front().timestamp )
  {
    m_current = m_queue.front();
    m_queue.pop();
  }

  if( m_current.timestamp == 0 ) return m_current.pts;

  return m_current.pts + min(m_current.duration, (timestamp - m_current.timestamp));
}


CDVDAudio::CDVDAudio(volatile bool &bStop)
  : m_bStop(bStop)
{
  m_pAudioDecoder = NULL;
  m_pCallback = NULL;
  m_iBufferSize = 0;
  m_dwPacketSize = 0;
  m_pBuffer = NULL;
  m_bPassthrough = false;
  m_iBitsPerSample = 0;
  m_iBitrate = 0;
  m_iChannels = 0;
  m_SecondsPerByte = 0.0;
  m_bPaused = true;
}

CDVDAudio::~CDVDAudio()
{
  CSingleLock lock (m_critSection);
  if (m_pAudioDecoder)
  {
    m_pAudioDecoder->Deinitialize();
    delete m_pAudioDecoder;
  }
  free(m_pBuffer);
}

void CDVDAudio::RegisterAudioCallback(IAudioCallback* pCallback)
{
  CSingleLock lock (m_critSection);
  m_pCallback = pCallback;
  if (m_pCallback && m_pAudioDecoder) m_pAudioDecoder->RegisterAudioCallback(pCallback);
}

void CDVDAudio::UnRegisterAudioCallback()
{
  CSingleLock lock (m_critSection);
  m_pCallback = NULL;
  if (m_pAudioDecoder) m_pAudioDecoder->UnRegisterAudioCallback();
}

bool CDVDAudio::Create(const DVDAudioFrame &audioframe, AVCodecID codec)
{
  CLog::Log(LOGNOTICE, "Creating audio device with codec id: %i, channels: %i, sample rate: %i, %s", codec, audioframe.channels, audioframe.sample_rate, audioframe.passthrough ? "pass-through" : "no pass-through");

  // if passthrough isset do something else
  CSingleLock lock (m_critSection);

  const char* codecstring="";

  if(codec == AV_CODEC_ID_AAC)
#ifdef USE_LIBFAAD
    codecstring = "AAC";
#else
    codecstring = "FF-AAC"; // ffmpeg decodes aac with a different channel order so we need to differentiate
#endif
  else if (codec == AV_CODEC_ID_VORBIS)
    codecstring = "Vorbis";
  else if(codec == AV_CODEC_ID_AC3 || codec == AV_CODEC_ID_DTS)
    codecstring = ""; // TODO, fix ac3 and dts decoder to output standard windows mapping
  else
    codecstring = "PCM";

#ifdef _XBOX
  // we don't allow resampling now, there is a bug in sscc that causes it to return the wrong chunklen.
  if( audioframe.passthrough )
    m_pAudioDecoder = new CAc97DirectSound(m_pCallback, audioframe.channels, audioframe.sample_rate, audioframe.bits_per_sample, true); // true = resample, 128 buffers
  else
    m_pAudioDecoder = new CASyncDirectSound(m_pCallback, audioframe.channels, audioframe.sample_rate, audioframe.bits_per_sample, codecstring);
#else

  if( audioframe.passthrough )
    return false;

  m_pAudioDecoder = new CWin32DirectSound(m_pCallback, audioframe.channels, audioframe.sample_rate, audioframe.bits_per_sample, false, codecstring);
#endif

  if (!m_pAudioDecoder) return false;

  m_iChannels = audioframe.channels;
  m_iBitrate = audioframe.sample_rate;
  m_iBitsPerSample = audioframe.bits_per_sample;
  m_bPassthrough = audioframe.passthrough;
  if(m_iChannels && m_iBitrate && m_iBitsPerSample)
    m_SecondsPerByte = 1.0 / (m_iChannels * m_iBitrate * (m_iBitsPerSample>>3));
  else
    m_SecondsPerByte = 0.0;

  m_dwPacketSize = m_pAudioDecoder->GetChunkLen();
  if(m_bPaused)
    m_pAudioDecoder->Pause();

  m_iBufferSize = 0;

  return true;
}

void CDVDAudio::Destroy()
{
  CSingleLock lock (m_critSection);

  if (m_pAudioDecoder)
  {
    m_pAudioDecoder->Stop();
    m_pAudioDecoder->Deinitialize();
    delete m_pAudioDecoder;
  }
  free(m_pBuffer);
  m_pBuffer = NULL;
  m_dwPacketSize = 0;
  m_pAudioDecoder = NULL;
  m_iBufferSize = 0;
  m_iChannels = 0;
  m_iBitrate = 0;
  m_iBitsPerSample = 0;
  m_bPassthrough = false;
  m_bPaused = true;
  m_time.Flush();
}

DWORD CDVDAudio::AddPacketsRenderer(unsigned char* data, DWORD len, CSingleLock &lock)
{
  if(!m_pAudioDecoder)
    return 0;

  DWORD bps = m_iChannels * m_iBitrate * (m_iBitsPerSample>>3);
  if(!bps)
    return 0;

  //Calculate a timeout when this definitely should be done
  double timeout;
  timeout  = DVD_SEC_TO_TIME(m_pAudioDecoder->GetDelay() + len * m_SecondsPerByte);
  timeout += DVD_SEC_TO_TIME(1.0);
  timeout += CDVDClock::GetAbsoluteClock();

  DWORD  total = len;
  DWORD  copied;
  do
  {
    copied = m_pAudioDecoder->AddPackets(data, len);
    data += copied;
    len -= copied;
    if (len < m_dwPacketSize)
      break;

    if (copied == 0 && timeout < CDVDClock::GetAbsoluteClock())
    {
      CLog::Log(LOGERROR, "CDVDAudio::AddPacketsRenderer - timeout adding data to renderer");
      break;
    }

    lock.Leave();
    Sleep(1);
    lock.Enter();
  } while (!m_bStop);

  return total - len;
}

DWORD CDVDAudio::AddPackets(const DVDAudioFrame &audioframe)
{
  CSingleLock lock (m_critSection);

  unsigned char* data = audioframe.data;
  DWORD len = audioframe.size;

  DWORD total = len;
  DWORD copied;

  // When paused, we need to buffer all data as renderers don't need to accept it
  if (m_bPaused)
  {
    m_pBuffer = (BYTE*)realloc(m_pBuffer, m_iBufferSize + len);
    memcpy(m_pBuffer+m_iBufferSize, data, len);
    m_iBufferSize += len;
    return len;
  }

  if (m_iBufferSize > 0) // See if there are carryover bytes from the last call. need to add them 1st.
  {
    copied = std::min(m_dwPacketSize - m_iBufferSize % m_dwPacketSize, len); // Smaller of either the data provided or the leftover data
    if(copied)
    {
      m_pBuffer = (BYTE*)realloc(m_pBuffer, m_iBufferSize + copied);
      memcpy(m_pBuffer + m_iBufferSize, data, copied); // Tack the caller's data onto the end of the buffer
      data += copied; // Move forward in caller's data
      len -= copied; // Decrease amount of data available from caller
      m_iBufferSize += copied; // Increase amount of data available in buffer
    }

    if(m_iBufferSize < m_dwPacketSize) // If we don't have enough data to give to the renderer, wait until next time
      return copied;

    if(AddPacketsRenderer(m_pBuffer, m_iBufferSize, lock) != m_iBufferSize)
    {
      m_iBufferSize = 0;
      CLog::Log(LOGERROR, "%s - failed to add leftover bytes to render", __FUNCTION__);
      return copied;
    }

    m_iBufferSize = 0;
    if (!len)
      return copied; // We used up all the caller's data
  }

  copied = AddPacketsRenderer(data, len, lock);
  data += copied;
  len -= copied;

  // if we have more data left, save it for the next call to this funtion
  if (len > 0 && !m_bStop)
  {
    m_pBuffer     = (BYTE*)realloc(m_pBuffer, len);
    m_iBufferSize = len;
    memcpy(m_pBuffer, data, len);
  }

  double time_added = DVD_SEC_TO_TIME(m_SecondsPerByte * (data - audioframe.data));
  double delay      = GetDelay();
  double timestamp  = CDVDClock::GetAbsoluteClock();
  m_time.Add(audioframe.pts, delay - time_added, audioframe.duration, timestamp);

  return total;
}

void CDVDAudio::Finish()
{
  CSingleLock lock (m_critSection);
  if (!m_pAudioDecoder)
    return;

  DWORD silence = m_dwPacketSize - m_iBufferSize % m_dwPacketSize;

  if(silence > 0 && m_iBufferSize > 0)
  {
    CLog::Log(LOGDEBUG, "CDVDAudio::Drain - adding %d bytes of silence, buffer size: %d, chunk size: %d", silence, m_iBufferSize, m_dwPacketSize);
    m_pBuffer = (BYTE*)realloc(m_pBuffer, m_iBufferSize + silence);
    memset(m_pBuffer+m_iBufferSize, 0, silence);
    m_iBufferSize += silence;
  }

  if(AddPacketsRenderer(m_pBuffer, m_iBufferSize, lock) != m_iBufferSize)
    CLog::Log(LOGERROR, "CDVDAudio::Drain - failed to play the final %d bytes", m_iBufferSize);

  m_iBufferSize = 0;
}

void CDVDAudio::Drain()
{
  Finish();
  CSingleLock lock (m_critSection);
  if (m_pAudioDecoder)
    m_pAudioDecoder->WaitCompletion();
}

void CDVDAudio::SetVolume(int iVolume)
{
  CSingleLock lock (m_critSection);
  if (m_pAudioDecoder) m_pAudioDecoder->SetCurrentVolume(iVolume);
}

void CDVDAudio::SetDynamicRangeCompression(long drc)
{
  CSingleLock lock (m_critSection);
  if (m_pAudioDecoder) m_pAudioDecoder->SetDynamicRangeCompression(drc);
}

void CDVDAudio::Pause()
{
  CSingleLock lock (m_critSection);
  m_bPaused = true;
  if (m_pAudioDecoder) m_pAudioDecoder->Pause();
  m_time.Flush();
}

void CDVDAudio::Resume()
{
  CSingleLock lock (m_critSection);
  m_bPaused = false;
  if (m_pAudioDecoder) m_pAudioDecoder->Resume();
}

double CDVDAudio::GetDelay()
{
  CSingleLock lock (m_critSection);

  double delay = 0.0;
  if(m_pAudioDecoder)
    delay = m_pAudioDecoder->GetDelay();

  delay += m_SecondsPerByte * m_iBufferSize;

  return delay * DVD_TIME_BASE;
}

void CDVDAudio::Flush()
{
  CSingleLock lock (m_critSection);

  if (m_pAudioDecoder)
  {
    m_pAudioDecoder->Stop();
    m_pAudioDecoder->Resume();
  }
  m_iBufferSize = 0;
  m_time.Flush();
}

bool CDVDAudio::IsValidFormat(const DVDAudioFrame &audioframe)
{
  if(!m_pAudioDecoder)
    return false;

  if(audioframe.passthrough != m_bPassthrough)
    return false;

  if(audioframe.channels != m_iChannels
  || audioframe.sample_rate != m_iBitrate
  || audioframe.bits_per_sample != m_iBitsPerSample)
    return false;

  return true;
}

double CDVDAudio::GetCacheTime()
{
  CSingleLock lock (m_critSection);
  if(!m_pAudioDecoder)
    return 0.0;

  double delay = 0.0;
  if(m_pAudioDecoder)
    delay = m_pAudioDecoder->GetCacheTime();

  delay += m_SecondsPerByte * m_iBufferSize;

  return delay;
}

double CDVDAudio::GetCacheTotal()
{
  CSingleLock lock (m_critSection);
  if(!m_pAudioDecoder)
    return 0.0;
  return m_pAudioDecoder->GetCacheTotal();
}

void CDVDAudio::SetPlayingPts(double pts)
{
  CSingleLock lock (m_critSection);
  m_time.Flush();
  double delay     = GetDelay();
  double timestamp = CDVDClock::GetAbsoluteClock();
  m_time.Add(pts, delay, 0, timestamp);
}

double CDVDAudio::GetPlayingPts()
{
  return m_time.Current(CDVDClock::GetAbsoluteClock());
}
