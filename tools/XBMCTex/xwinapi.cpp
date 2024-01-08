
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "xwinapi.h"

// I hope this doesn't need to handle unicode...
LPTSTR GetCommandLine() {
  pid_t pid = 0;
  char  procFile[32],
        *cmdline = NULL;
  FILE  *fp = NULL;
  size_t cmdlinelen = 0;
  int i;

  pid = getpid();
  sprintf(procFile, "/proc/%u/cmdline", pid);
  if((fp = fopen(procFile, "r")) == NULL) return NULL;
  // getline() allocates memory so be sure to free it
  // after calling GetCommandLine()
  getline(&cmdline, &cmdlinelen, fp);
  fclose(fp);
  fp = NULL;

  for (i = 0; i < (int)cmdlinelen; i++) {
    if (cmdline[i] == 0x00) {
      if (cmdline[i + 1] == 0x00)
        break;
      cmdline[i] = ' ';
    }
  }
  
  cmdline = (char *)realloc(cmdline, strlen(cmdline) + 1);
  return cmdline;
}

DWORD GetCurrentDirectory(DWORD nBufferLength, LPTSTR lpBuffer) {
  bool bSizeTest = (nBufferLength == 0 && lpBuffer == NULL);
  if (getcwd(lpBuffer, nBufferLength) == NULL) {
    if (errno == ERANGE) {
      LPTSTR tmp = NULL;
      getcwd(tmp, 0);
      nBufferLength = strlen(tmp) + 1;
      free(tmp);
      return nBufferLength;
    }
    return 0;
  }
  if (bSizeTest) {
    nBufferLength = strlen(lpBuffer) + 1;
    free(lpBuffer);
    lpBuffer = NULL;
    return nBufferLength;
  }
  return strlen(lpBuffer);
}

BOOL SetCurrentDirectory(LPCTSTR lpPathName) {
  return (chdir(lpPathName) == 0);
}

DWORD GetLastError( ) {
  return errno;
}

