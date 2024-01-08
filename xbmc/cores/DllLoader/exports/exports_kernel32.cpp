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
 
#include "system.h"
#include "../DllLoader.h"

#include "emu_kernel32.h"
#include "../dll_tracker_library.h"
#include "../dll_tracker_memory.h"
#include "../dll_tracker_critical_section.h"

Export export_kernel32[] =
{
  { "AddAtomA",                                     -1, dllAddAtomA,                                  NULL },
  { "FindAtomA",                                    -1, dllFindAtomA,                                 NULL },
  { "GetAtomNameA",                                 -1, dllGetAtomNameA,                              NULL },
  { "CreateThread",                                 -1, dllCreateThread,                              NULL },
  { "FindClose",                                    -1, dllFindClose,                                 NULL },
  { "FindFirstFileA",                               -1, FindFirstFileA,                               NULL },
  { "FindNextFileA",                                -1, FindNextFileA,                                NULL },
  { "GetFileAttributesA",                           -1, dllGetFileAttributesA,                        NULL },
  { "GetLastError",                                 -1, GetLastError,                                 NULL },
  { "SetUnhandledExceptionFilter",                  -1, dllSetUnhandledExceptionFilter,               NULL },
  { "Sleep",                                        -1, dllSleep,                                     NULL },
  { "SleepEx",                                      -1, SleepEx,                                      NULL },
  { "TerminateThread",                              -1, dllTerminateThread,                           NULL },
  { "GetCurrentThread",                             -1, dllGetCurrentThread,                          NULL },
  { "QueryPerformanceCounter",                      -1, QueryPerformanceCounter,                      NULL },
#ifdef _XBOX
  { "QueryPerformanceFrequency",                    -1, QueryPerformanceFrequencyXbox,                NULL },
#else
  { "QueryPerformanceFrequency",                    -1, QueryPerformanceFrequency,                    NULL },
#endif
  { "SetThreadPriority",                            -1, SetThreadPriority,                            NULL },
  { "GetTickCount",                                 -1, GetTickCount,                                 NULL },
  { "GetCurrentThreadId",                           -1, GetCurrentThreadId,                           NULL },
  { "GetCurrentProcessId",                          -1, dllGetCurrentProcessId,                       NULL },
  { "GetSystemTimeAsFileTime",                      -1, GetSystemTimeAsFileTime,                      NULL },
  { "OutputDebugStringA",                           -1, OutputDebugString,                            NULL },
  { "DisableThreadLibraryCalls",                    -1, dllDisableThreadLibraryCalls,                 NULL },
  { "GlobalMemoryStatus",                           -1, GlobalMemoryStatus,                           NULL },
  { "CreateEventA",                                 -1, CreateEventA,                                 NULL },
  { "ResetEvent",                                   -1, ResetEvent,                                   NULL },
  { "WaitForSingleObject",                          -1, dllWaitForSingleObject,                       NULL },
  { "LoadLibraryA",                                 -1, dllLoadLibraryA,                              track_LoadLibraryA },
  { "FreeLibrary",                                  -1, dllFreeLibrary,                               track_FreeLibrary },
  { "GetProcAddress",                               -1, dllGetProcAddress,                            NULL },
  { "LeaveCriticalSection",                         -1, dllLeaveCriticalSection,                      NULL },
  { "EnterCriticalSection",                         -1, dllEnterCriticalSection,                      NULL },
  { "DeleteCriticalSection",                        -1, dllDeleteCriticalSection,                     track_DeleteCriticalSection },
  { "InitializeCriticalSection",                    -1, dllInitializeCriticalSection,                 track_InitializeCriticalSection },
  { "GetSystemInfo",                                -1, dllGetSystemInfo,                             NULL },
  { "CloseHandle",                                  -1, CloseHandle,                                  NULL },
  { "GetPrivateProfileIntA",                        -1, dllGetPrivateProfileIntA,                     NULL },
  { "WaitForMultipleObjects",                       -1, dllWaitForMultipleObjects,                    NULL },
  { "SetEvent",                                     -1, SetEvent,                                     NULL },
  { "TlsAlloc",                                     -1, dllTlsAlloc,                                  NULL },
  { "TlsFree",                                      -1, dllTlsFree,                                   NULL },
  { "TlsGetValue",                                  -1, dllTlsGetValue,                               NULL },
  { "TlsSetValue",                                  -1, dllTlsSetValue,                               NULL },
  { "HeapFree",                                     -1, HeapFree,                                     NULL },
  { "HeapAlloc",                                    -1, HeapAlloc,                                    NULL },
  { "LocalFree",                                    -1, LocalFree,                                    NULL },
  { "LocalAlloc",                                   -1, LocalAlloc,                                   NULL },
  { "LocalReAlloc",                                 -1, LocalReAlloc,                                 NULL },
  { "LocalLock",                                    -1, LocalLock,                                    NULL },
  { "LocalUnlock",                                  -1, LocalUnlock,                                  NULL },
  { "LocalHandle",                                  -1, LocalHandle,                                  NULL },
  { "InterlockedIncrement",                         -1, InterlockedIncrement,                         NULL },
  { "InterlockedDecrement",                         -1, InterlockedDecrement,                         NULL },
  { "InterlockedExchange",                          -1, InterlockedExchange,                          NULL },
  { "GetProcessHeap",                               -1, GetProcessHeap,                               NULL },
  { "GetModuleHandleA",                             -1, dllGetModuleHandleA,                          NULL },
  { "InterlockedCompareExchange",                   -1, InterlockedCompareExchange,                   NULL },
  { "GetVersionExA",                                -1, dllGetVersionExA,                             NULL },
  { "GetVersionExW",                                -1, dllGetVersionExW,                             NULL },
  { "GetProfileIntA",                               -1, dllGetProfileIntA,                            NULL },
  { "CreateFileA",                                  -1, dllCreateFileA,                               NULL },
  { "DeviceIoControl",                              -1, DeviceIoControl,                              NULL },
  { "ReadFile",                                     -1, ReadFile,                                     NULL },
  { "dllDVDReadFile",                               -1, dllDVDReadFileLayerChangeHack,                NULL },
  { "SetFilePointer",                               -1, SetFilePointer,                               NULL },
#ifdef _XBOX
  { "xboxopendvdrom",                               -1, xboxopendvdrom,                               NULL },
#endif
  { "GetVersion",                                   -1, dllGetVersion,                                NULL },
  { "MulDiv",                                       -1, MulDiv,                                       NULL },
  { "lstrlenA",                                     -1, lstrlenA,                                     NULL },
  { "lstrlenW",                                     -1, lstrlenW,                                     NULL },
  { "LoadLibraryExA",                               -1, dllLoadLibraryExA,                            track_LoadLibraryExA },

  { "DeleteFileA",                                  -1, DeleteFileA,                                  NULL },
  { "GetModuleFileNameA",                           -1, dllGetModuleFileNameA,                        NULL },
  { "GlobalAlloc",                                  -1, GlobalAlloc,                                  NULL },
  { "GlobalLock",                                   -1, GlobalLock,                                   NULL },
  { "GlobalUnlock",                                 -1, GlobalUnlock,                                 NULL },
  { "GlobalHandle",                                 -1, GlobalHandle,                                 NULL },
  { "GlobalFree",                                   -1, GlobalFree,                                   NULL },
  { "FreeEnvironmentStringsW",                      -1, dllFreeEnvironmentStringsW,                   NULL },
  { "SetLastError",                                 -1, SetLastError,                                 NULL },
  { "RestoreLastError",                             -1, SetLastError,                                 NULL },
  { "GetOEMCP",                                     -1, dllGetOEMCP,                                  NULL },
  { "SetEndOfFile",                                 -1, SetEndOfFile,                                 NULL },
  { "RtlUnwind",                                    -1, dllRtlUnwind,                                 NULL },
  { "GetCommandLineA",                              -1, dllGetCommandLineA,                           NULL },
  { "HeapReAlloc",                                  -1, HeapReAlloc,                                  NULL },
  { "ExitProcess",                                  -1, dllExitProcess,                               NULL },
  { "TerminateProcess",                             -1, dllTerminateProcess,                          NULL },
  { "GetCurrentProcess",                            -1, dllGetCurrentProcess,                         NULL },
  { "HeapSize",                                     -1, HeapSize,                                     NULL },
  { "WriteFile",                                    -1, WriteFile,                                    NULL },
  { "GetACP",                                       -1, dllGetACP,                                    NULL },
  { "SetHandleCount",                               -1, dllSetHandleCount,                            NULL },
  { "GetStdHandle",                                 -1, dllGetStdHandle,                              NULL },
  { "GetFileType",                                  -1, dllGetFileType,                               NULL },
  { "GetStartupInfoA",                              -1, dllGetStartupInfoA,                           NULL },
  { "FreeEnvironmentStringsA",                      -1, dllFreeEnvironmentStringsA,                   NULL },
  { "WideCharToMultiByte",                          -1, dllWideCharToMultiByte,                       NULL },
  { "GetEnvironmentStrings",                        -1, dllGetEnvironmentStrings,                     NULL },
  { "GetEnvironmentStringsW",                       -1, dllGetEnvironmentStringsW,                    NULL },
  { "GetEnvironmentVariableA",                      -1, dllGetEnvironmentVariableA,                   NULL },
  { "HeapDestroy",                                  -1, HeapDestroy,                                  track_HeapDestroy },
  { "HeapCreate",                                   -1, HeapCreate,                                   track_HeapCreate },
  { "VirtualFree",                                  -1, VirtualFree,                                  track_VirtualFree },
  { "VirtualFreeEx",                                -1, VirtualFreeEx,                                track_VirtualFreeEx },
  { "VirtualAlloc",                                 -1, VirtualAlloc,                                 track_VirtualAlloc },
  { "VirtualAllocEx",                               -1, VirtualAllocEx,                               track_VirtualAllocEx },
  { "MultiByteToWideChar",                          -1, dllMultiByteToWideChar,                       NULL },
  { "LCMapStringA",                                 -1, dllLCMapStringA,                              NULL },
  { "LCMapStringW",                                 -1, dllLCMapStringW,                              NULL },
  { "IsBadWritePtr",                                -1, IsBadWritePtr,                                NULL },
  { "SetStdHandle",                                 -1, dllSetStdHandle,                              NULL },
  { "FlushFileBuffers",                             -1, FlushFileBuffers,                             NULL },
  { "GetStringTypeA",                               -1, dllGetStringTypeA,                            NULL },
  { "GetStringTypeW",                               -1, dllGetStringTypeW,                            NULL },
  { "IsBadReadPtr",                                 -1, IsBadReadPtr,                                 NULL },
  { "IsBadCodePtr",                                 -1, IsBadCodePtr,                                 NULL },
  { "GetCPInfo",                                    -1, dllGetCPInfo,                                 NULL },

  { "CreateMutexA",                                 -1, CreateMutexA,                                 NULL },
  { "CreateSemaphoreA",                             -1, CreateSemaphoreA,                             NULL },
  { "PulseEvent",                                   -1, PulseEvent,                                   NULL },
  { "ReleaseMutex",                                 -1, ReleaseMutex,                                 NULL },
  { "ReleaseSemaphore",                             -1, ReleaseSemaphore,                             NULL },

  { "GetThreadLocale",                              -1, dllGetThreadLocale,                           NULL },
  { "SetPriorityClass",                             -1, dllSetPriorityClass,                          NULL },
  { "FormatMessageA",                               -1, dllFormatMessageA,                            NULL },
  { "GetFullPathNameA",                             -1, dllGetFullPathNameA,                          NULL },
#ifdef _XBOX
  { "SignalObjectAndWait",                          -1, SignalObjectAndWait,                          NULL },
#endif
  { "ExpandEnvironmentStringsA",                    -1, dllExpandEnvironmentStringsA,                 NULL },
  { "GetVolumeInformationA",                        -1, GetVolumeInformationA,                        NULL },
  { "GetWindowsDirectoryA",                         -1, dllGetWindowsDirectoryA,                      NULL },
  { "GetSystemDirectoryA",                          -1, dllGetSystemDirectoryA,                       NULL },
  { "DuplicateHandle",                              -1, dllDuplicateHandle,                           NULL },
  { "GetShortPathNameA",                            -1, dllGetShortPathName,                          NULL },
  { "GetTempPathA",                                 -1, dllGetTempPathA,                              NULL },
  { "SetErrorMode",                                 -1, dllSetErrorMode,                              NULL },
  { "IsProcessorFeaturePresent",                    -1, dllIsProcessorFeaturePresent,                 NULL },
  { "FileTimeToLocalFileTime",                      -1, FileTimeToLocalFileTime,                      NULL },
  { "FileTimeToSystemTime",                         -1, FileTimeToSystemTime,                         NULL },
  { "GetTimeZoneInformation",                       -1, GetTimeZoneInformation,                       NULL },

  { "GetCurrentDirectoryA",                         -1, dllGetCurrentDirectoryA,                      NULL },
  { "SetCurrentDirectoryA",                         -1, dllSetCurrentDirectoryA,                      NULL },

  { "SetEnvironmentVariableA",                      -1, dllSetEnvironmentVariableA,                   NULL },
  { "CreateDirectoryA",                             -1, dllCreateDirectoryA,                          NULL },

  { "GetProcessAffinityMask",                       -1, dllGetProcessAffinityMask,                    NULL },

  { "lstrcpyA",                                     -1, lstrcpyA,                                     NULL },
  { "GetProcessTimes",                              -1, dllGetProcessTimes,                           NULL },

  { "GetLocaleInfoA",                               -1, dllGetLocaleInfoA,                            NULL },
  { "GetConsoleCP",                                 -1, dllGetConsoleCP,                              NULL },
  { "GetConsoleOutputCP",                           -1, dllGetConsoleOutputCP,                        NULL },
  { "SetConsoleCtrlHandler",                        -1, dllSetConsoleCtrlHandler,                     NULL },
  { "GetExitCodeThread",                            -1, GetExitCodeThread,                            NULL },
  { "ResumeThread",                                 -1, ResumeThread,                                 NULL },
  { "ExitThread",                                   -1, ExitThread,                                   NULL },
  { "VirtualQuery",                                 -1, VirtualQuery,                                 NULL },
  { "VirtualQueryEx",                               -1, VirtualQueryEx,                               NULL },
  { "VirtualProtect",                               -1, VirtualProtect,                               NULL },
  { "VirtualProtectEx",                             -1, VirtualProtectEx,                             NULL },
  { "UnhandledExceptionFilter",                     -1, UnhandledExceptionFilter,                     NULL },
  { "RaiseException",                               -1, RaiseException,                               NULL },
  { "FlsAlloc",                                     -1, dllFlsAlloc,                                  NULL },
  { "FlsGetValue",                                  -1, dllFlsGetValue,                               NULL },
  { "FlsSetValue",                                  -1, dllFlsSetValue,                               NULL },
  { "FlsFree",                                      -1, dllFlsFree,                                   NULL },
  { "DebugBreak",                                   -1, DebugBreak,                                   NULL },
  { "GetThreadTimes",                               -1, GetThreadTimes,                               NULL },
  { "EncodePointer",                                -1, dllEncodePointer,                             NULL },
  { "DecodePointer",                                -1, dllDecodePointer,                             NULL },

  { "LockFile",                                     -1, dllLockFile,                                  NULL },
  { "LockFileEx",                                   -1, dllLockFileEx,                                NULL },
  { "UnlockFile",                                   -1, dllUnlockFile,                                NULL },

  { "GetSystemTime",                                -1, GetSystemTime,                                NULL },
  { "GetFileSize",                                  -1, GetFileSize,                                  NULL },
  { "FindResourceA",                                -1, dllFindResourceA,                             NULL },
  { "LoadResource",                                 -1, dllLoadResource,                              NULL },
  { NULL,                                         NULL, NULL,                                         NULL }
};
