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

#include "SharedSection.h"

#define MAXSHAREDACCESSES 5000

CSharedSection::CSharedSection()
{
  m_sharedLock = 0;
  m_exclusive = false;
  InitializeCriticalSection(&m_critSection);
  m_eventFree = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CSharedSection::~CSharedSection()
{
  DeleteCriticalSection(&m_critSection);
  CloseHandle(m_eventFree);
}

void CSharedSection::EnterShared()
{
  EnterCriticalSection(&m_critSection);
  if( !m_exclusive )
  { //exclusve will be set if this thread already owns this object
    ResetEvent(m_eventFree);
    InterlockedIncrement(&m_sharedLock);
  }
  LeaveCriticalSection(&m_critSection);
}

void CSharedSection::LeaveShared()
{
  if( !m_exclusive )
  {
    if( InterlockedDecrement(&m_sharedLock) == 0 ) 
      SetEvent(m_eventFree);
  }
}

void CSharedSection::EnterExclusive()
{
  EnterCriticalSection(&m_critSection);
  if( InterlockedCompareExchange(&m_sharedLock, 0, 0) != 0 )
    WaitForSingleObject(m_eventFree, INFINITE);
  m_exclusive = true;
}

void CSharedSection::LeaveExclusive()
{
  m_exclusive = false;
  LeaveCriticalSection(&m_critSection);
}

//////////////////////////////////////////////////////////////////////
// SharedLock
//////////////////////////////////////////////////////////////////////

CSharedLock::CSharedLock(CSharedSection& cs)
    : m_cs( cs )
    , m_bIsOwner( false )
{
  Enter();
}

CSharedLock::CSharedLock(const CSharedSection& cs)
    : m_cs( const_cast<CSharedSection&>(cs) )
    , m_bIsOwner( false )
{
  Enter();
}

CSharedLock::~CSharedLock()
{
  Leave();
}

bool CSharedLock::IsOwner() const
{
  return m_bIsOwner;
}

bool CSharedLock::Enter()
{
  // Test if we already own the critical section
  if ( true == m_bIsOwner )
  {
    return true;
  }

  // Blocking call
  m_cs.EnterShared();
  m_bIsOwner = true;

  return m_bIsOwner;
}

void CSharedLock::Leave()
{
  if ( false == m_bIsOwner )
  {
    return ;
  }

  m_cs.LeaveShared();
  m_bIsOwner = false;
}

//////////////////////////////////////////////////////////////////////
// ExclusiveLock
//////////////////////////////////////////////////////////////////////

CExclusiveLock::CExclusiveLock(CSharedSection& cs)
    : m_cs( cs )
    , m_bIsOwner( false )
{
  Enter();
}

CExclusiveLock::CExclusiveLock(const CSharedSection& cs)
    : m_cs( const_cast<CSharedSection&>(cs) )
    , m_bIsOwner( false )
{
  Enter();
}

CExclusiveLock::~CExclusiveLock()
{
  Leave();
}

bool CExclusiveLock::IsOwner() const
{
  return m_bIsOwner;
}

bool CExclusiveLock::Enter()
{
  // Test if we already own the critical section
  if ( true == m_bIsOwner )
  {
    return true;
  }

  // Blocking call
  m_cs.EnterExclusive();
  m_bIsOwner = true;

  return m_bIsOwner;
}

void CExclusiveLock::Leave()
{
  if ( false == m_bIsOwner )
  {
    return ;
  }

  m_cs.LeaveExclusive();
  m_bIsOwner = false;
}

