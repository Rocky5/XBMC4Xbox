/*
* XBMC Media Center
* Copyright (c) 2002 d7o3g4q and RUNTiME
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

// AsyncAudioRenderer.h: interface for the CResampleDirectSound class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IDirectSoundRenderer.h"
#include "IAudioCallback.h"
#include "cores/ssrc.h"

class CResampleDirectSound : public IDirectSoundRenderer
{
public:
  virtual void UnRegisterAudioCallback();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual DWORD GetChunkLen();
  virtual FLOAT GetDelay();
  virtual FLOAT GetCacheTime();
  virtual FLOAT GetCacheTotal();
  CResampleDirectSound(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, char* strAudioCodec = "", bool bIsMusic = false);
  virtual ~CResampleDirectSound();

  virtual DWORD AddPackets(unsigned char* data, DWORD len);
  virtual DWORD GetSpace();
  virtual HRESULT Deinitialize();
  virtual HRESULT Pause();
  virtual HRESULT Stop();
  virtual HRESULT Resume();
  virtual LONG GetMinimumVolume() const;
  virtual LONG GetMaximumVolume() const;
  virtual LONG GetCurrentVolume() const;
  virtual void Mute(bool bMute);
  virtual HRESULT SetCurrentVolume(LONG nVolume);
  virtual int SetPlaySpeed(int iSpeed);
  virtual void WaitCompletion();
  virtual void DoWork();
  virtual void SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers);
  virtual void SetDynamicRangeCompression(long drc);

private:

  DWORD m_dwOutputSize;
  DWORD m_dwInputSize;
  unsigned int m_uiChannels;

  unsigned char* m_pSampleData;
  Cssrc m_Resampler;
  IDirectSoundRenderer *m_pRenderer;
};
