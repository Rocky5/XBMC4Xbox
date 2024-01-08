#ifndef Timidity_CODEC_H_
#define Timidity_CODEC_H_

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

#include "ICodec.h"
#include "DllTimidity.h"

class TimidityCodec : public ICodec
{
public:
  TimidityCodec();
  virtual ~TimidityCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();
  static bool IsSupportedFormat(const CStdString& strExt);

private:
  LibraryLoader* m_loader;
  typedef void  (__cdecl *InitMethod) (void);
  typedef int (__cdecl *LoadMethod) ( const char* p1);
  typedef int (__cdecl *FillMethod) ( int p1, char* p2, int p3);
  typedef void  (__cdecl *FreeMethod) ( int p1);
  typedef unsigned long (__cdecl *LengthMethod) ( int p1);
  typedef unsigned long (__cdecl *SeekMethod) ( int p1, unsigned long p2);
  struct   
  {
    InitMethod Init;
    LoadMethod LoadMID;
    FillMethod FillBuffer;
    FreeMethod FreeMID;
    LengthMethod GetLength;
    SeekMethod Seek;
  } m_dll;

  int m_mid;
  int m_iTrack;
  __int64 m_iDataPos;
};

#endif

