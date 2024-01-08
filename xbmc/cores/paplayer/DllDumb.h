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

#include "DynamicDll.h"

#ifndef _XBOX
#ifdef FreeModule
#undef FreeModule
#endif
#endif

class DllDumbInterface
{
public:
    int LoadModule(const char* szFileName);
    void FreeModule(int);
    int GetModuleLength(int duh);
    int GetModulePosition(int sic);
    int StartPlayback(int duh, long pos);
    void StopPlayback(int sic);
    long FillBuffer(int, int sig, char* buffer, int size, float volume);
};

class DllDumb : public DllDynamic, DllDumbInterface
{
  DECLARE_DLL_WRAPPER(DllDumb, Q:\\system\\players\\PAPlayer\\dumb.dll)
  DEFINE_METHOD1(int, LoadModule, (const char* p1))
  DEFINE_METHOD1(void, FreeModule, (int p1))
  DEFINE_METHOD1(int, GetModuleLength, (int p1))
  DEFINE_METHOD1(int, GetModulePosition, (int p1))
  DEFINE_METHOD2(int, StartPlayback, (int p1, long p2))
  DEFINE_METHOD1(void, StopPlayback, (int p1))
  DEFINE_METHOD5(long, FillBuffer, (int p1, int p2, char* p3, int p4, float p5))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(DLL_LoadModule, LoadModule)
    RESOLVE_METHOD_RENAME(DLL_FreeModule, FreeModule)
    RESOLVE_METHOD_RENAME(DLL_GetModuleLength, GetModuleLength)
    RESOLVE_METHOD_RENAME(DLL_GetModulePosition, GetModulePosition)
    RESOLVE_METHOD_RENAME(DLL_StartPlayback, StartPlayback)
    RESOLVE_METHOD_RENAME(DLL_StopPlayback, StopPlayback)
    RESOLVE_METHOD_RENAME(DLL_FillBuffer, FillBuffer)
  END_METHOD_RESOLVE()
};
