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

#include "Thread.h"
#include <process.h>
#include "win32exception.h"
#include "log.h"
#include "Util.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifndef _MT
#pragma message( "Please compile using multithreaded run-time libraries" )
#endif
typedef unsigned (WINAPI *PBEGINTHREADEX_THREADFUNC)(LPVOID lpThreadParameter);

#define MS_VC_EXCEPTION 0x406d1388
typedef struct tagTHREADNAME_INFO
{
  DWORD dwType; // must be 0x1000
  LPCSTR szName; // pointer to name (in same addr space)
  DWORD dwThreadID; // thread ID (-1 caller thread)
  DWORD dwFlags; // reserved for future use, most be zero
} THREADNAME_INFO;

CThread::CThread()
{
  m_bStop = false;

  m_bAutoDelete = false;
  m_ThreadHandle = NULL;
  m_ThreadId = 0;
  m_iLastTime = 0;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;
  m_StopEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

  m_pRunnable=NULL;
}

CThread::CThread(IRunnable* pRunnable)
{
  m_bStop = false;

  m_bAutoDelete = false;
  m_ThreadHandle = NULL;
  m_ThreadId = 0;
  m_iLastTime = 0;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;
  m_StopEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

  m_pRunnable=pRunnable;
}

CThread::~CThread()
{
  if (m_ThreadHandle != NULL)
  {
    CloseHandle(m_ThreadHandle);
  }
  m_ThreadHandle = NULL;

  if (m_StopEvent)
    CloseHandle(m_StopEvent);
}


DWORD WINAPI CThread::staticThread(LPVOID* data)
{
  CThread* pThread = (CThread*)(data);
  if (!pThread) {
    CLog::Log(LOGERROR,"%s, sanity failed. thread is NULL.",__FUNCTION__);
    return 1;
  }
  
#ifdef _XBOX
  CUtil::InitRandomSeed();
#endif

//  CLog::Log(LOGDEBUG,"thread start, auto delete: %d",pThread->IsAutoDelete());

  /* install win32 exception translator */
  win32_exception::install_handler();

  try 
  {
    pThread->OnStartup();
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
    if( pThread->IsAutoDelete() )
    {
      delete pThread;
      _endthreadex(123);
      return 0;
    }
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception caught in thread startup, aborting. auto delete: %d", __FUNCTION__, pThread->IsAutoDelete());
    if( pThread->IsAutoDelete() )
    {
      delete pThread;
#ifndef _LINUX
      _endthreadex(123);
#endif
      return 0;
    }
  }

  try
  {
    pThread->Process();
  }
#ifndef _LINUX
  catch (const access_violation &e)
  {
    e.writelog(__FUNCTION__);
  }
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception caught in thread process, attemping cleanup in OnExit", __FUNCTION__);
  }

  try
  {
    pThread->OnExit();
  }
#ifndef _LINUX
  catch (const access_violation &e)
  {
    e.writelog(__FUNCTION__);
  }
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception caught in thread exit", __FUNCTION__);
  }

  if ( pThread->IsAutoDelete() )
  {
//    CLog::Log(LOGDEBUG,"Thread %"PRIu64" terminating (autodelete)", (uint64_t)CThread::GetCurrentThreadId());
    delete pThread;
    pThread = NULL;
  }
//  else
//    CLog::Log(LOGDEBUG,"Thread %"PRIu64" terminating", (uint64_t)CThread::GetCurrentThreadId());  
#ifndef _LINUX
  _endthreadex(123);
#endif
  return 0;
}

void CThread::Create(bool bAutoDelete, unsigned stacksize)
{
  if (m_ThreadHandle != NULL)
  {
    throw 1; //ERROR should not b possible!!!
  }
  m_iLastTime = GetTickCount() * 10000;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;
  m_bAutoDelete = bAutoDelete;
  m_bStop = false;
  ::ResetEvent(m_StopEvent);

  m_ThreadHandle = (HANDLE)_beginthreadex(NULL, stacksize, (PBEGINTHREADEX_THREADFUNC)staticThread, (void*)this, 0, &m_ThreadId);

}

bool CThread::IsAutoDelete() const
{
  return m_bAutoDelete;
}

void CThread::StopThread(bool bWait /*= true*/)
{
  m_bStop = true;
  SetEvent(m_StopEvent);
  if (m_ThreadHandle && bWait)
  {
    WaitForThreadExit(INFINITE);
    CloseHandle(m_ThreadHandle);
    m_ThreadHandle = NULL;
  }
}

DWORD CThread::ThreadId() const
{
  return (DWORD)m_ThreadId;
}


CThread::operator HANDLE()
{
  return m_ThreadHandle;
}

CThread::operator HANDLE() const
{
  return m_ThreadHandle;
}

bool CThread::SetPriority(const int iPriority)
// Set thread priority
// Return true for success
{
  if (m_ThreadHandle)
  {
    return ( SetThreadPriority( m_ThreadHandle, iPriority ) == TRUE );
  }
  else
  {
    return false;
  }
}

void CThread::SetName( LPCTSTR szThreadName )
{
  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = szThreadName;
  info.dwThreadID = m_ThreadId;
  info.dwFlags = 0;
#ifndef _LINUX
  try
  {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), (DWORD *)&info);
  }
  catch(...)
  {
  }
#endif
}

bool CThread::WaitForThreadExit(DWORD dwMilliseconds)
// Waits for thread to exit, timeout in given number of msec.
// Returns true when thread ended
{
  if (!m_ThreadHandle) return true;

  // boost priority of thread we are waiting on to same as caller
  int callee = GetThreadPriority(m_ThreadHandle);
  int caller = GetThreadPriority(GetCurrentThread());
  if(caller > callee)
    SetThreadPriority(m_ThreadHandle, caller);

  if (::WaitForSingleObject(m_ThreadHandle, dwMilliseconds) != WAIT_TIMEOUT)
    return true;

  // restore thread priority if thread hasn't exited
  if(caller > callee)
    SetThreadPriority(m_ThreadHandle, callee);

  return false;
}

HANDLE CThread::ThreadHandle()
{
  return m_ThreadHandle;
}

void CThread::Process()
{
  if(m_pRunnable)
    m_pRunnable->Run();
}

float CThread::GetRelativeUsage()
{
  unsigned __int64 iTime = GetTickCount();
  iTime *= 10000; // convert into 100ns tics

  // only update every 1 second
  if( iTime < m_iLastTime + 1000*10000 ) return m_fLastUsage;

  FILETIME CreationTime, ExitTime, UserTime, KernelTime;
  if( GetThreadTimes( m_ThreadHandle, &CreationTime, &ExitTime, &KernelTime, &UserTime ) )
  {
    unsigned __int64 iUsage = 0;
    iUsage += (((unsigned __int64)UserTime.dwHighDateTime) << 32) + ((unsigned __int64)UserTime.dwLowDateTime);
    iUsage += (((unsigned __int64)KernelTime.dwHighDateTime) << 32) + ((unsigned __int64)KernelTime.dwLowDateTime);

    if(m_iLastUsage > 0 && m_iLastTime > 0)
      m_fLastUsage = (float)( iUsage - m_iLastUsage ) / (float)( iTime - m_iLastTime );
      
    m_iLastUsage = iUsage;
    m_iLastTime = iTime;

    return m_fLastUsage;
  }
  return 0.0f;
}

bool CThread::IsCurrentThread() const
{
  return IsCurrentThread(ThreadId());
}


ThreadIdentifier CThread::GetCurrentThreadId()
{
  return ::GetCurrentThreadId();
}

bool CThread::IsCurrentThread(const ThreadIdentifier tid)
{
  return (::GetCurrentThreadId() == tid);
}

int CThread::GetMinPriority(void)
{
  return(THREAD_PRIORITY_IDLE);
}

int CThread::GetMaxPriority(void)
{
  return(THREAD_PRIORITY_HIGHEST);
}

int CThread::GetNormalPriority(void)
{
  return(THREAD_PRIORITY_NORMAL);
}

DWORD CThread::WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
  if(dwMilliseconds > 10 && IsCurrentThread())
  {
    HANDLE handles[2] = {hHandle, m_StopEvent};
    DWORD result = ::WaitForMultipleObjects(2, handles, false, dwMilliseconds);

    if(result == WAIT_TIMEOUT || result == WAIT_OBJECT_0)
      return result;

    if( dwMilliseconds == INFINITE )
      return WAIT_ABANDONED;
    else
      return WAIT_TIMEOUT;
  }
  else
    return ::WaitForSingleObject(hHandle, dwMilliseconds);
}

DWORD CThread::WaitForMultipleObjects(DWORD nCount, HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds)
{
  // for now not implemented
  return ::WaitForMultipleObjects(nCount, lpHandles, bWaitAll, dwMilliseconds);
}

void CThread::Sleep(DWORD dwMilliseconds)
{
  if(dwMilliseconds > 10 && IsCurrentThread())
    ::WaitForSingleObject(m_StopEvent, dwMilliseconds);
  else
    ::Sleep(dwMilliseconds);
}
