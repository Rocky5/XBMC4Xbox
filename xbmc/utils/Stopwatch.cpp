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

#include "Stopwatch.h"
#if defined(_LINUX) && !defined(__APPLE__)
#include <sys/sysinfo.h>
#endif

CStopWatch::CStopWatch()
{
  m_timerPeriod      = 0.0f;
  m_startTick        = 0;
  m_isRunning        = false;

  // Get the timer frequency (ticks per second)
#ifndef _LINUX
  LARGE_INTEGER timerFreq;
  QueryPerformanceFrequency( &timerFreq );
  m_timerPeriod = 1.0f / (float)timerFreq.QuadPart;
#else
  m_timerPeriod = 1.0f / 1000.0f; // we want seconds
#endif

}

CStopWatch::~CStopWatch()
{
}

bool CStopWatch::IsRunning() const
{
  return m_isRunning;
}

void CStopWatch::StartZero()
{
  m_startTick = GetTicks();
  m_isRunning = true;
}

void CStopWatch::Start()
{
  if (!m_isRunning)
    m_startTick = GetTicks();
  m_isRunning = true;
}

void CStopWatch::Stop()
{
  if( m_isRunning )
  {
    m_startTick = 0;
    m_isRunning = false;
  }
}

void CStopWatch::Reset()
{
  if (m_isRunning)
    m_startTick = GetTicks();
}

float CStopWatch::GetElapsedSeconds() const
{
  LONGLONG totalTicks = m_isRunning ? (GetTicks() - m_startTick) : 0;
  return (FLOAT)totalTicks * m_timerPeriod;
}

float CStopWatch::GetElapsedMilliseconds() const
{
  return GetElapsedSeconds() * 1000.0f;
}

LONGLONG CStopWatch::GetTicks() const
{
#ifndef _LINUX
  LARGE_INTEGER currTicks;
  QueryPerformanceCounter( &currTicks );
  return currTicks.QuadPart;
#else
  return timeGetTime();
#endif
}
