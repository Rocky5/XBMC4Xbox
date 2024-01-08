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
#include "dll_tracker_memory.h"
#include "dll_tracker.h"
#include "DllLoader.h"
#include "utils/SingleLock.h"

struct MSizeCount
{
  size_t size;
  unsigned count;
};

typedef std::map<uintptr_t, MSizeCount> CallerMap;
typedef std::map<uintptr_t, MSizeCount>::iterator CallerMapIter;

extern "C" inline void tracker_memory_track(uintptr_t caller, void* data_addr, size_t size)
{
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(caller);
  if (pInfo)
  {
    CSingleLock lock(g_trackerLock);
    AllocLenCaller temp;
    temp.size = size;
    temp.calleraddr = caller;
    pInfo->dataList[(uintptr_t)data_addr] = temp;
  }
}

extern "C" inline bool tracker_memory_free(DllTrackInfo* pInfo, void* data_addr)
{
  if(data_addr == NULL)
    return true;

  bool bFreeFailed = !pInfo;
  if (!bFreeFailed)
  {
    CSingleLock lock(g_trackerLock);
    bFreeFailed = pInfo->dataList.erase((uintptr_t)data_addr) < 1;
  }
  if (bFreeFailed)
  {
    // unable to delete the pointer from one of the trackers, but track_free is called!!
    // This will happen when memory is freed by another dll then the one which allocated the memory.
    // We, have to search every map for this memory pointer.
    // Yes, it's slow todo, but if we are freeing already freed pointers when unloading a dll
    // xbmc will crash.
    CSingleLock locktd(g_trackerLock);
    for (TrackedDllsIter it = g_trackedDlls.begin(); it != g_trackedDlls.end(); ++it)
    {
      // try to free the pointer from this list, and break if success
      if ((*it)->dataList.erase((uintptr_t)data_addr) > 0) return true;
    }
    return false;
  }
  return true;
}

extern "C" void tracker_memory_free_all(DllTrackInfo* pInfo)
{
  CSingleLock lock(g_trackerLock);
  if (!pInfo->dataList.empty() || !pInfo->virtualList.empty())
  {
    CLog::Log(LOGDEBUG,"%s (base %p): Detected memory leaks: %d leaks", pInfo->pDll->GetFileName(), pInfo->pDll->hModule, (int)(pInfo->dataList.size() + pInfo->virtualList.size()));
    size_t total = 0;
    CallerMap tempMap;
    CallerMapIter itt;
    for (DataListIter p = pInfo->dataList.begin(); p != pInfo->dataList.end(); ++p)
    {
      try
      {
        total += (p->second).size;
        free((void*)p->first);
        if ( ( itt = tempMap.find((p->second).calleraddr) ) != tempMap.end() )
        {
          (itt->second).size += (p->second).size;
          (itt->second).count++;
        }
        else
        {
          MSizeCount temp;
          temp.size = (p->second).size;
          temp.count = 1;
          tempMap.insert(std::make_pair( (p->second).calleraddr, temp ) );
        }
      }
      catch(...)
      {
        CLog::Log(LOGERROR, "failed to free memory at address %p. buffer overrun is likely cause", (void*)p->first);
      }

    }
    for (VAllocListIter p = pInfo->virtualList.begin(); p != pInfo->virtualList.end(); ++p)
    {
      total += (p->second).size;
      VirtualFree((void*)p->first, 0, MEM_RELEASE);
      if ( ( itt = tempMap.find((p->second).calleraddr) ) != tempMap.end() )
      {
        (itt->second).size += (p->second).size;
        (itt->second).count++;
      }
      else
      {
        MSizeCount temp;
        temp.size = (p->second).size;
        temp.count = 1;
        tempMap.insert(std::make_pair( (p->second).calleraddr, temp ) );
      }
    }

    for ( itt = tempMap.begin(); itt != tempMap.end();++itt )
    {
      CLog::Log(LOGDEBUG,"leak caller address %p, size %8i, counter %"PRIdS"", (void*)itt->first, (int)(itt->second).size, (itt->second).count);
    }
    CLog::Log(LOGDEBUG,"%s: Total bytes leaked: %"PRIdS"", pInfo->pDll->GetName(), total);
    tempMap.erase(tempMap.begin(), tempMap.end());
  }
  pInfo->dataList.erase(pInfo->dataList.begin(), pInfo->dataList.end());
  pInfo->virtualList.erase(pInfo->virtualList.begin(), pInfo->virtualList.end());
}

extern "C" void* __cdecl track_malloc(size_t s)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  void* p = malloc(s);
  if (!p) 
  {    
    CLog::Log(LOGSEVERE, "DLL: %s : malloc failed, crash imminent (Out of memory requesting %"PRIdS" bytes)", tracker_getdllname(loc), s);
    return NULL;
  }

  tracker_memory_track(loc, p, s);

  return p;
}

extern "C" void* __cdecl track_calloc(size_t n, size_t s)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  void* p = calloc(n, s);
  if (!p) 
  {    
    CLog::Log(LOGSEVERE, "DLL: %s : calloc failed, crash imminent (Out of memory)", tracker_getdllname(loc));
    return NULL;
  }

  tracker_memory_track(loc, p, s);
  
  return p;
}

extern "C" void* __cdecl track_realloc(void* p, size_t s)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  void* q = realloc(p, s);
  if (!q) 
  {
    //  a dll may realloc with a size of 0, so NULL is the correct return value is this case
    if (s > 0) CLog::Log(LOGSEVERE, "DLL: %s : realloc failed, crash imminent (Out of memory)", tracker_getdllname(loc));
    return NULL;
  }

  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
  if (pInfo)
  {
    CSingleLock lock(g_trackerLock);
    if (p != q) tracker_memory_free(pInfo, p);
    AllocLenCaller temp;
    temp.size = s;
    temp.calleraddr = loc;
    pInfo->dataList[(uintptr_t)q] = temp;
  }

  return q;
}

extern "C" void __cdecl track_free(void* p)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  //Only call free if this is actually something that has been allocated
  if( tracker_memory_free(tracker_get_dlltrackinfo(loc), p) )
  {
    free(p);
  }
  else
  { 
    //Only do this on memmory that wasn't found in track list.
    //this should only be an exception and using exception handling
    //here in the normal case seems overkill
    try
    {
      //free(p);
    }
    catch(...)
    {
      CLog::Log(LOGDEBUG,"DLL tried to free memory not allocated by any DLL, skipping");
    }
  }
}

extern "C" char* __cdecl track_strdup(const char* str)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  char* pdup = strdup(str);

  tracker_memory_track(loc, pdup, 0);

  return pdup;
}

extern "C" void tracker_heapobjects_free_all(DllTrackInfo* pInfo)
{
  if (!pInfo->heapobjectList.empty())
  {
    CLog::Log(LOGDEBUG,"%s: Detected heapobject leaks: %d leaks", pInfo->pDll->GetFileName(), pInfo->heapobjectList.size());

    CSingleLock lock(g_trackerLock);
    for (HeapObjectListIter it = pInfo->heapobjectList.begin(); it != pInfo->heapobjectList.end(); ++it)
    {
      try
      {
        HeapDestroy(*it);
      }
      catch(...)
      {
        CLog::Log(LOGERROR, "failed to free heapobject. might have been freed from somewhere else");
      }
    }
    pInfo->heapobjectList.erase(pInfo->heapobjectList.begin(), pInfo->heapobjectList.end());
  }
}

HANDLE
WINAPI
track_HeapCreate(
    IN DWORD flOptions,
    IN SIZE_T dwInitialSize,
    IN SIZE_T dwMaximumSize
    )
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
  HANDLE hHeap = HeapCreate(flOptions, dwInitialSize, dwMaximumSize);

  if( pInfo && hHeap )
  {
    CSingleLock lock(g_trackerLock);
    pInfo->heapobjectList.push_back(hHeap);    
  }
  
  return hHeap;
}

BOOL
WINAPI
track_HeapDestroy(
    IN OUT HANDLE hHeap
    )
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();


  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
  if (pInfo && hHeap)
  {
    for (HeapObjectListIter it = pInfo->heapobjectList.begin(); it != pInfo->heapobjectList.end(); ++it)
    {
      if (*it == hHeap)
      {
        CSingleLock lock(g_trackerLock);
        pInfo->heapobjectList.erase(it);
        break;
      }
    }
  }

  return HeapDestroy(hHeap);
}


BOOL WINAPI track_VirtualFreeEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  if(dwFreeType == MEM_RELEASE)
  {    
    DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
    if (pInfo)
    {
      CSingleLock lock(g_trackerLock);
      pInfo->virtualList.erase((uintptr_t)lpAddress);
    }
  }
  return VirtualFreeEx(hProcess, lpAddress, dwSize, dwFreeType);
}


LPVOID WINAPI track_VirtualAllocEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  LPVOID address = VirtualAllocEx(hProcess, lpAddress, dwSize, flAllocationType, flProtect);
  
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
  if(pInfo)
  {
    CSingleLock lock(g_trackerLock);
    // make sure we get the base address for this allocation
    MEMORY_BASIC_INFORMATION info;
    if(VirtualQueryEx(hProcess, address, &info, sizeof(info)))
    {
      AllocLenCaller temp;
      temp.size = (uintptr_t)info.AllocationBase;
      temp.calleraddr = loc;    
      pInfo->virtualList[(uintptr_t)address] = temp;
    }
  }
  return address;
}

LPVOID WINAPI track_VirtualAlloc( LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  LPVOID address = VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
  
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
  if(pInfo)
  {
    CSingleLock lock(g_trackerLock);
    // make sure we get the base address for this allocation
    MEMORY_BASIC_INFORMATION info;
    if(VirtualQuery(address, &info, sizeof(info)))
    {
      AllocLenCaller temp;
      temp.size = (size_t)dwSize;
      temp.calleraddr = loc;    
      pInfo->virtualList[(uintptr_t)info.AllocationBase] = temp;
    }
  }
  return address;
}

BOOL WINAPI track_VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  if(dwFreeType == MEM_RELEASE)
  {    
    DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
    if (pInfo)
    {
      CSingleLock lock(g_trackerLock);
      pInfo->virtualList.erase((uintptr_t)lpAddress);
    }
  }
  return VirtualFree(lpAddress, dwSize, dwFreeType);
}

