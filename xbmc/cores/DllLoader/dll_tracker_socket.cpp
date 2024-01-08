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
 
#include "utils/log.h"
#include "dll_tracker_socket.h"
#include "DllLoader.h"
#include "dll_tracker.h"
#include "utils/SingleLock.h"
#include "exports/emu_socket.h"

extern "C" void tracker_socket_track(uintptr_t caller, SOCKET socket)
{
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(caller);
  if (pInfo)
  {
    CSingleLock lock(g_trackerLock);
    pInfo->socketList.push_back(socket);
  }
}

extern "C" void tracker_socket_free(uintptr_t caller, SOCKET socket)
{
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(caller);
  if (pInfo)
  {
    CSingleLock lock(g_trackerLock);
    pInfo->socketList.remove(socket);
  }
}

extern "C" void tracker_socket_free_all(DllTrackInfo* pInfo)
{
  if (!pInfo->fileList.empty())
  {
    CSingleLock lock(g_trackerLock);
    SOCKET socket;
    CLog::Log(LOGDEBUG,"%s: Detected open sockets: %d", pInfo->pDll->GetFileName(), pInfo->socketList.size());
    for (SocketListIter it = pInfo->socketList.begin(); it != pInfo->socketList.end(); ++it)
    {
      socket = *it;
      CLog::Log(LOGDEBUG,"socket des. : %x", socket);
      dllclosesocket(socket);
    }
  }
  pInfo->socketList.erase(pInfo->socketList.begin(), pInfo->socketList.end());
}

extern "C"
{
  SOCKET __stdcall track_socket(int af, int type, int protocol)
  {
    uintptr_t loc = (uintptr_t)_ReturnAddress();
    
    SOCKET socket = dllsocket(af, type, protocol);
    if(socket>=0)
      tracker_socket_track(loc, socket);
    return socket;
  }

  int __stdcall track_closesocket(SOCKET socket)
  {
    uintptr_t loc = (uintptr_t)_ReturnAddress();
    
    tracker_socket_free(loc, socket);
    return dllclosesocket(socket);
  }
  
  SOCKET __stdcall track_accept(SOCKET s, struct sockaddr FAR * addr, OUT int FAR * addrlen)
  {
    uintptr_t loc = (uintptr_t)_ReturnAddress();
    
    SOCKET socket = dllaccept(s, addr, addrlen);
    if (socket>=0) 
      tracker_socket_track(loc, socket);
    return socket;
  }
}
