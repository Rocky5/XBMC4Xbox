#ifndef SPC_CODEC_H_
#define SPC_CODEC_H_

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
#include "spc/Types.h"
#include "cores/DllLoader/DllLoader.h"

class SPCCodec : public ICodec
{
public:
  SPCCodec();
  virtual ~SPCCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();
private:
  typedef void  (__stdcall* LoadMethod) ( const void* p1);
  typedef void* (__stdcall * EmuMethod) ( void *p1, u32 p2, u32 p3);
  typedef void  (__stdcall * SeekMethod) ( u32 p1, b8 p2 );
  struct   
  {
    LoadMethod LoadSPCFile;
    EmuMethod EmuAPU;
    SeekMethod SeekAPU;
  } m_dll;

  DllLoader* m_loader;
  CStdString m_loader_name;

  char* m_szBuffer;
  u8* m_pApuRAM;
  __int64 m_iDataPos;
};

#endif
