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
#include "dec_if.h"

class DllMadCodecInterface
{
public:
  virtual ~DllMadCodecInterface() {}
  virtual IAudioDecoder* CreateAudioDecoder (unsigned int type, IAudioOutput **output)=0;
};

class DllMadCodec : public DllDynamic, DllMadCodecInterface
{
  DECLARE_DLL_WRAPPER(DllMadCodec, Q:\\system\\players\\PAPlayer\\MADCodec.dll)
  DEFINE_METHOD2(IAudioDecoder*, CreateAudioDecoder, (unsigned int p1, IAudioOutput **p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(CreateAudioDecoder)
  END_METHOD_RESOLVE()
};
