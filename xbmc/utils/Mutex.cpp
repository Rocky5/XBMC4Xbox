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

#include "Mutex.h"


CMutex::CMutex()
{
  m_hMutex = CreateMutex( NULL, FALSE, NULL);
}

CMutex::CMutex( char* pName )
{
  m_hMutex = CreateMutex( NULL, FALSE, pName);
}

CMutex::~CMutex()
{
  CloseHandle(m_hMutex);
  m_hMutex = NULL;
}

bool CMutex::Wait()
{
  if (m_hMutex)
  {
    if (WAIT_OBJECT_0 == WaitForSingleObject(m_hMutex, INFINITE)) return true;
  }
  return false;
}

void CMutex::Release()
{
  if (m_hMutex)
  {
    ReleaseMutex(m_hMutex);
  }
}

CMutexWait::CMutexWait(CMutex& mutex)
    : m_mutex(mutex)
{
  m_bLocked = m_mutex.Wait();
}

CMutexWait::~CMutexWait()
{
  if (m_bLocked)
  {
    m_mutex.Release();
  }
}


HANDLE CMutex::GetHandle()
{
  return m_hMutex;
}

void CMutex::WaitMSec(DWORD dwMillSeconds)
{
  if (m_hMutex) WaitForSingleObject(m_hMutex, dwMillSeconds);
}
