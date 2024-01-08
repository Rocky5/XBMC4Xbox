// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

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

#define DEBUG_MOUSE
#define DEBUG_KEYBOARD

#include "system.h"
#include <vector>
#include <list>
#include <map>
#include <set>
#include <memory>
#include <queue>
#include <stdio.h>
#include "utils/StdString.h"
#ifdef _XBOX
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <FStream>
#include <stdlib.h>
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
#include "utils/log.h"

// guilib internal
#include "gui3d.h"
#include "tinyXML/tinyxml.h"


#ifdef _XBOX
 #ifdef QueryPerformanceFrequency
  #undef QueryPerformanceFrequency
 #endif
 WINBASEAPI BOOL WINAPI QueryPerformanceFrequencyXbox(LARGE_INTEGER *lpFrequency);
 #define QueryPerformanceFrequency(a) QueryPerformanceFrequencyXbox(a)
#endif

#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

// Useful pixel colour manipulation macros
#define GET_A(color)            ((color & AMASK) >> PIXEL_ASHIFT)
#define GET_R(color)            ((color & RMASK) >> PIXEL_RSHIFT)
#define GET_G(color)            ((color & GMASK) >> PIXEL_GSHIFT)
#define GET_B(color)            ((color & BMASK) >> PIXEL_BSHIFT)

// TODO: reference additional headers your program requires here
