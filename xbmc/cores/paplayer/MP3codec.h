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

#include "CachingCodec.h"
#include "music/tags/MusicInfoTagLoaderMP3.h"
#include "DllMadCodec.h"

class MP3Codec : public CachingCodec
{
public:
  MP3Codec();
  virtual ~MP3Codec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual bool CanSeek();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual int ReadSamples(float *pBuffer, int numsamples, int *actualsamples);
  virtual bool CanInit();
  virtual bool SkipNext();
  virtual bool HasFloatData() const { return m_BitsPerSampleInternal == 32; };
private:
  void OnFileReaderClearEvent();
  void FlushDecoder();
  int Read(int size, bool init = false);

  // Decoding variables
  __int64 m_lastByteOffset;
  bool    m_eof;
  IAudioDecoder* m_pDecoder;    // handle to the codec.
  bool    m_Decoding;
  bool    m_CallAgainWithSameBuffer;
  int     m_readRetries;

  // Input buffer to read our mp3 data into
  BYTE*         m_InputBuffer;
  unsigned int  m_InputBufferSize;
  unsigned int  m_InputBufferPos;

  // Output buffer.  We require this, as mp3 decoding means keeping at least 2 frames (1152 * 2 samples)
  // of data in order to remove that data at the end as it may be surplus to requirements.
  BYTE*         m_OutputBuffer;
  unsigned int  m_OutputBufferSize;
  unsigned int  m_OutputBufferPos;    // position in our buffer

  unsigned int m_Formatdata[8];

  // Seeking helpers
  MUSIC_INFO::CVBRMP3SeekHelper m_seekInfo;

  // Gapless playback
  bool m_IgnoreFirst;     // Ignore first samples if this is true (for gapless playback)
  bool m_IgnoreLast;      // Ignore first samples if this is true (for gapless playback)
  int m_IgnoredBytes;     // amount of samples ignored thus far

  int m_BitsPerSampleInternal;

  DllMadCodec m_dll;
};

