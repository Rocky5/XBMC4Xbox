#ifndef _ENCODERWAV_H
#define _ENCODERWAV_H

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

#include "Encoder.h"
#include <stdio.h>

typedef struct
{
  BYTE riff[4];         /* must be "RIFF"    */
  DWORD len;             /* #bytes + 44 - 8   */
  BYTE cWavFmt[8];      /* must be "WAVEfmt " */
  DWORD dwHdrLen;
  WORD wFormat;
  WORD wNumChannels;
  DWORD dwSampleRate;
  DWORD dwBytesPerSec;
  WORD wBlockAlign;
  WORD wBitsPerSample;
  BYTE cData[4];        /* must be "data"   */
  DWORD dwDataLen;       /* #bytes           */
}
WAVHDR, *PWAVHDR, *LPWAVHDR;

class CEncoderWav : public CEncoder
{
public:
  CEncoderWav();
  virtual ~CEncoderWav() {}
  bool Init(const char* strFile, int iInChannels, int iInRate, int iInBits);
  int Encode(int nNumBytesRead, BYTE* pbtStream);
  bool Close();
  void AddTag(int key, const char* value);

private:
  bool WriteWavHeader();

  int m_iBytesWritten;
};

#endif // _ENCODERWAV_H
