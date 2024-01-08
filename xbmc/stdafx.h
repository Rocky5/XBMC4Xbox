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

#define XBMC_MAX_PATH 1024 // normal max path is 260, but smb shares and the like can be longer

#define DEBUG_MOUSE
#define DEBUG_KEYBOARD
#include "system.h"

#ifdef _XBOX
#if defined(_DEBUG) && defined(_MEMTRACKING)
#define _CRTDBG_MAP_ALLOC
#include <FStream>
#include <stdlib.h>
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif

#ifdef _XBOX
#ifdef QueryPerformanceFrequency
#undef QueryPerformanceFrequency
#endif
WINBASEAPI BOOL WINAPI QueryPerformanceFrequencyXbox(LARGE_INTEGER *lpFrequency);
#define QueryPerformanceFrequency(a) QueryPerformanceFrequencyXbox(a)
#else
#undef GetFreeSpace
#endif

#define SAFE_DELETE(p)       { delete (p);     (p)=NULL; }
#define SAFE_DELETE_ARRAY(p) { delete[] (p);   (p)=NULL; }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
