/*
* XBMC Media Center
* Copyright (c) 2002 Frodo
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

#include "system.h"
#include "CriticalSection.h"
#ifdef _XBOX
#include "xbox/Undocumented.h"
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCriticalSection::CCriticalSection()
{
  InitializeCriticalSection(&m_critSection);
}

CCriticalSection::~CCriticalSection()
{
  DeleteCriticalSection(&m_critSection);
}

CCriticalSection::operator LPCRITICAL_SECTION()
{
  return &m_critSection;
}

//////////////////////////////////////////////////////////////////////
CCriticalSection::CCriticalSection(const CCriticalSection& section)
{
  *this = section;
}
 
CCriticalSection& CCriticalSection::operator=(const CCriticalSection& section)
{
  if (this == &section) return * this;
 
  InitializeCriticalSection(&m_critSection);
  return *this;
}


BOOL NTAPI OwningCriticalSection(LPCRITICAL_SECTION section)
{
#ifdef _XBOX
  return (PKTHREAD)section->OwningThread == GetCurrentKPCR()->PrcbData.CurrentThread;
#else
  return section->OwningThread == (HANDLE)GetCurrentThreadId();
#endif
}

DWORD NTAPI ExitCriticalSection(LPCRITICAL_SECTION section)
{
  if(!OwningCriticalSection(section))
    return 0;

  DWORD count = section->RecursionCount;
  
  for(DWORD i=0;i<count;i++)
    LeaveCriticalSection(section);

  return count;
}

VOID NTAPI RestoreCriticalSection(LPCRITICAL_SECTION section, DWORD count)
{
  for(DWORD i=0;i<count;i++)
    EnterCriticalSection(section);  
}
