#ifndef XWINAPI_H__
#define XWINAPI_H__

#include "PlatformDefs.h"

#ifdef __APPLE__
static void getline(char **a, size_t* l, FILE* f) {}
#endif

LPTSTR GetCommandLine();
DWORD GetCurrentDirectory(DWORD nBufferLength, LPTSTR lpBuffer);
BOOL SetCurrentDirectory(LPCTSTR lpPathName);
DWORD GetLastError();
#endif // XWINAPI_H__

