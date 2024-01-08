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
#include "utils/log.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <io.h>
#include <direct.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include "Util.h"
#include "FileSystem/IDirectory.h"
#include "FileSystem/DirectoryFactory.h"
#include "FileSystem/SpecialProtocol.h"
#include "URL.h"
#include "FileSystem/File.h"
#include "settings/GUISettings.h"
#include "FileItem.h"
#include "utils/URIUtils.h"

#include "emu_msvcrt.h"
#include "emu_dummy.h"
#include "emu_kernel32.h"
#include "util/EmuFileWrapper.h"

using namespace std;
using namespace XFILE;

#if defined(_MSC_VER) && _MSC_VER < 1500
extern "C" {
  __int64 __cdecl _ftelli64(FILE *);
  int __cdecl _fseeki64(FILE *, __int64, int);
}
#endif

struct SDirData
{
  XFILE::IDirectory* Directory;
  CFileItemList items;
  SDirData()
  {
    Directory = NULL;
  }
};

#define MAX_OPEN_DIRS 10
static SDirData vecDirsOpen[MAX_OPEN_DIRS];
bool bVecDirsInited = false;
#ifdef HAS_VIDEO_PLAYBACK
extern void update_cache_dialog(const char* tmp);
#endif

struct _env
{
  const char* name;
  char* value;
};

#define EMU_MAX_ENVIRONMENT_ITEMS 50
char *dll__environ[EMU_MAX_ENVIRONMENT_ITEMS + 1]; 
CRITICAL_SECTION dll_cs_environ;

#define dll_environ    (*dll___p__environ())   /* pointer to environment table */

extern "C" void __stdcall init_emu_environ()
{
  InitializeCriticalSection(&dll_cs_environ);
  memset(dll__environ, 0, EMU_MAX_ENVIRONMENT_ITEMS + 1);
  
  // libdvdnav
  dll_putenv("DVDREAD_NOKEYS=1");
  //dll_putenv("DVDREAD_VERBOSE=1");
  //dll_putenv("DVDREAD_USE_DIRECT=1");
  
  // libdvdcss
  dll_putenv("DVDCSS_METHOD=key");
  dll_putenv("DVDCSS_VERBOSE=3");
  dll_putenv("DVDCSS_CACHE=T:\\cache");
  
  // python
#ifdef _XBOX
  dll_putenv("OS=xbox");
#elif defined(_WIN32)
  dll_putenv("OS=win32");
#elif defined(_LINUX)
  dll_putenv("OS=linux");
#elif defined(_APPLE)
  dll_putenv("OS=osx");
#else
  dll_putenv("OS=unknown");
#endif
  dll_putenv("PYTHONPATH=Q:\\system\\python\\python27.zlib;Q:\\system\\python\\DLLs;Q:\\system\\python\\Lib;Q:\\system\\python\\spyce");
  dll_putenv("PYTHONHOME=Q:\\system\\python");
  dll_putenv("PATH=.;Q:\\;Q:\\system\\python");
  //dll_putenv("PYTHONCASEOK=1");
  //dll_putenv("PYTHONDEBUG=1");
  //dll_putenv("PYTHONVERBOSE=2"); // "1" for normal verbose, "2" for more verbose ?
  dll_putenv("PYTHONOPTIMIZE=1");
  //dll_putenv("PYTHONDUMPREFS=1");
  //dll_putenv("THREADDEBUG=1");
  //dll_putenv("PYTHONMALLOCSTATS=1");
  //dll_putenv("PYTHONY2K=1");
  dll_putenv("TEMP=Z:\\temp"); // for python tempdir
}

extern "C" void __stdcall update_emu_environ()
{
  // Use a proxy, if the GUI was configured as such
  if (g_guiSettings.GetBool("network.usehttpproxy")
      && !g_guiSettings.GetString("network.httpproxyserver").empty()
      && !g_guiSettings.GetString("network.httpproxyport").empty())
  {
    const CStdString &strProxyServer = g_guiSettings.GetString("network.httpproxyserver");
    const CStdString &strProxyPort = g_guiSettings.GetString("network.httpproxyport");
    // Should we check for valid strings here? should HTTPS_PROXY use https://?
    dll_putenv( "HTTP_PROXY=http://" + strProxyServer + ":" + strProxyPort );
    dll_putenv( "HTTPS_PROXY=http://" + strProxyServer + ":" + strProxyPort );
  }
  else
  {
    // is there a better way to delete an environment variable?
    // this works but leaves the variable
    dll_putenv( "HTTP_PROXY=" );
    dll_putenv( "HTTPS_PROXY=" );
  }
}

bool emu_is_hd(const char* path)
{
  if (path[0] != 0 && path[1] == ':')
  {
    if (path[0] == 'C' ||
        path[0] == 'E' ||
        path[0] == 'F' ||
        path[0] == 'Q' ||
        path[0] == 'S' ||
        path[0] == 'T' ||
        path[0] == 'U' ||
        path[0] == 'V' ||
        path[0] == 'Y' ||
        path[0] == 'Z')
    {
      return true;
    }
  }
  return false;
}

bool emu_is_root_drive(const char* path)
{
  int pathlen = strlen(path);
  if (pathlen == 2 || pathlen == 3)
  {
    return emu_is_hd(path);
  }
  return false;
}

extern "C"
{
  void dll_sleep(unsigned long imSec)
  {
    Sleep(imSec);
  }

  // FIXME, XXX, !!!!!!
  void dllReleaseAll( )
  {
    // close all open dirs...
    if (bVecDirsInited)
    {
      for (int i=0;i < MAX_OPEN_DIRS; ++i)
      {
        if (vecDirsOpen[i].Directory)
        {
          delete vecDirsOpen[i].Directory;
          vecDirsOpen[i].items.Clear();
          vecDirsOpen[i].Directory = NULL;
        }
      }
      bVecDirsInited = false;
    }
  }

  void* dllmalloc(size_t size)
  {
    void* pBlock = malloc(size);
    if (!pBlock)
    {
      CLog::Log(LOGSEVERE, "malloc %"PRIdS" bytes failed, crash imminent", size);
    }
    return pBlock;
  }

  void dllfree( void* pPtr )
  {
    free(pPtr);
  }

  void* dllcalloc(size_t num, size_t size)
  {
    void* pBlock = calloc(num, size);
    if (!pBlock)
    {
      CLog::Log(LOGSEVERE, "calloc %"PRIdS" bytes failed, crash imminent", size);
    }
    return pBlock;
  }

  void* dllrealloc( void *memblock, size_t size )
  {
    void* pBlock =  realloc(memblock, size);
    if (!pBlock)
    {
      CLog::Log(LOGSEVERE, "realloc %"PRIdS" bytes failed, crash imminent", size);
    }
    return pBlock;
  }

  void dllexit(int iCode)
  {
    not_implement("msvcrt.dll fake function exit() called\n");      //warning
  }

  void dllabort()
  {
    not_implement("msvcrt.dll fake function abort() called\n");     //warning
  }

  void* dll__dllonexit(PFV input, PFV ** start, PFV ** end)
  {
    //ported from WINE code
    PFV *tmp;
    int len;

    if (!start || !*start || !end || !*end)
    {
      //FIXME("bad table\n");
      return NULL;
    }

    len = (*end - *start);

    if (++len <= 0)
      return NULL;

    tmp = (PFV*) realloc (*start, len * sizeof(tmp) );
    if (!tmp)
      return NULL;
    *start = tmp;
    *end = tmp + len;
    tmp[len - 1] = input;
    return (void *)input;

    //wrong handling, this function is used for register functions
    //that called before exit use _initterm functions.

    //dllReleaseAll( );
    //return TRUE;
  }

  _onexit_t dll_onexit(_onexit_t func)
  {
    not_implement("msvcrt.dll fake function dll_onexit() called\n");

    // register to dll unload list
    // return func if succsesfully added to the dll unload list
    return NULL;
  }

  int dllputs(const char* szLine)
  {
    if (!szLine[0]) return EOF;
    if (szLine[strlen(szLine) - 1] != '\n')
      CLog::Log(LOGDEBUG,"  msg: %s", szLine);
    else
      CLog::Log(LOGDEBUG,"  msg: %s\n", szLine);
    
    // return a non negative value
    return 0;
  }

  int dllprintf(const char *format, ...)
  {
    va_list va;
    static char tmp[2048];
    va_start(va, format);
    _vsnprintf(tmp, 2048, format, va);
    va_end(va);
    tmp[2048 - 1] = 0;
#if defined(HAS_VIDEO_PLAYBACK) && defined(HAS_XBOX_HARDWARE)
    update_cache_dialog(tmp);
#endif
    CLog::Log(LOGDEBUG, "  msg: %s", tmp);
    
    return strlen(tmp);
  }

  char *_fullpath(char *absPath, const char *relPath, size_t maxLength)
  {
    unsigned int len = strlen(relPath);
    if (len > maxLength && absPath != NULL) return NULL;

    // dll has to make sure it uses the correct path for now
    if (len > 1 && relPath[1] == ':')
    {
      if (absPath == NULL) absPath = dll_strdup(relPath);
      else
      {
        strncpy(absPath, relPath, maxLength);
        if (maxLength != 0)
          absPath[maxLength-1] = '\0';
      }
      return absPath;
    }
    if (!strncmp(relPath, "\\Device\\Cdrom0", 14))
    {
      // needed?
      if (absPath == NULL) absPath = strdup(relPath);
      else
      {
        strncpy(absPath, relPath, maxLength);
        if (maxLength != 0)
          absPath[maxLength-1] = '\0';
      }
      return absPath;
    }

    not_implement("msvcrt.dll incomplete function _fullpath(...) called\n");      //warning
    return NULL;
  }

  FILE* dll_popen(const char *command, const char *mode)
  {
    not_implement("msvcrt.dll fake function _popen(...) called\n"); //warning
    return NULL;
  }

  int dll_pclose(FILE *stream)
  {
    not_implement("msvcrt.dll fake function _pclose(...) called\n");        //warning
    return 0;
  }

  FILE* dll_fdopen(int fd, const char* mode)
  {
    if (g_emuFileWrapper.DescriptorIsEmulatedFile(fd))
    {
      not_implement("msvcrt.dll incomplete function _fdopen(...) called\n");
      // file is probably already open here ???
      // the only correct thing todo is to close and reopn the file ???
      // for now, just return its stream
      FILE* stream = g_emuFileWrapper.GetStreamByDescriptor(fd);
      return stream;
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return _fdopen(fd, mode);
    }
    
    not_implement("msvcrt.dll incomplete function _fdopen(...) called\n");
    return NULL;
  }

  int dll_open(const char* szFileName, int iMode)
  {
    char str[XBMC_MAX_PATH];
    int size = sizeof(str);
    // move to CFile classes
    if (strncmp(szFileName, "\\Device\\Cdrom0", 14) == 0)
    {
      // replace "\\Device\\Cdrom0" with "D:"
      strncpy(str, "D:", size);
      if (size)
      {
        str[size-1] = '\0';
        strncat(str, szFileName + 14, size - strlen(str));
      }
    }
    else
    {
      strncpy(str, szFileName, size);
      if (size)
        str[size-1] = '\0';
    }

    CFile* pFile = new CFile();
    bool bWrite = false;
    if ((iMode & O_RDWR) || (iMode & O_WRONLY))
      bWrite = true;
    bool bOverwrite=false;
    if ((iMode & _O_TRUNC) || (iMode & O_CREAT))
      bOverwrite = true;
    // currently always overwrites
    bool bResult;

    // We need to validate the path here as some calls from ie. libdvdnav
    // or the python DLLs have malformed slashes on Win32 & Xbox
    // (-> E:\test\VIDEO_TS/VIDEO_TS.BUP))
    if (bWrite)
      bResult = pFile->OpenForWrite(CUtil::ValidatePath(str, true), bOverwrite);
    else
      bResult = pFile->Open(CUtil::ValidatePath(str, true));
    
    if (bResult)
    {
      EmuFileObject* object = g_emuFileWrapper.RegisterFileObject(pFile);
      if (object == NULL)
      {
        VERIFY(0);
        pFile->Close();
        delete pFile;
        return -1;
      }
      return g_emuFileWrapper.GetDescriptorByStream(&object->file_emu);
    }
    delete pFile;
    return -1;
  }

  FILE* dll_freopen(const char *path, const char *mode, FILE *stream)
  {
    if (g_emuFileWrapper.StreamIsEmulatedFile(stream))
    {
      dll_fclose(stream);
      return dll_fopen(path, mode);
    }
    else if (!IS_STD_STREAM(stream))
    {
      // Translate the path
      return freopen(_P(path).c_str(), mode, stream);
    }
    
    // error
    // close stream and return NULL
    dll_fclose(stream);
    return NULL;
  }


  int dll_read(int fd, void* buffer, unsigned int uiSize)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
       return pFile->Read(buffer, uiSize);
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return read(fd, buffer, uiSize);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  int dll_write(int fd, const void* buffer, unsigned int uiSize)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
       return pFile->Write(buffer, uiSize);
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return write(fd, buffer, uiSize);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  int dll_close(int fd)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
      g_emuFileWrapper.UnRegisterFileObjectByDescriptor(fd);
      
      pFile->Close();
      delete pFile;
      return 0;
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return close(fd);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  __int64 dll_lseeki64(int fd, __int64 lPos, int iWhence)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
      lPos = pFile->Seek(lPos, iWhence);
      return lPos;
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      // not supported: return lseeki64(fd, lPos, iWhence);
      CLog::Log(LOGWARNING, "msvcrt.dll: dll_lseeki64 called, TODO: add 'int64 -> long' type checking");      //warning
      return (__int64)lseek(fd, (long)lPos, iWhence);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return (__int64)-1;
  }

  long dll_lseek(int fd, long lPos, int iWhence)
  {
    if (g_emuFileWrapper.DescriptorIsEmulatedFile(fd))
    {
      return (long)dll_lseeki64(fd, (__int64)lPos, iWhence);
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return lseek(fd, lPos, iWhence);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  void dll_rewind(FILE* stream)
  {
    int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
    if (fd >= 0)
    {
      dll_lseeki64(fd, (__int64)0, SEEK_SET);
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, let the operating system handle it
      rewind(stream);
    }
    else
    {
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    }
  }

  //---------------------------------------------------------------------------------------------------------
  int dll_fclose(FILE * stream)
  {
    int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
    if (fd >= 0)
    {
      return dll_close(fd);
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, let the operating system handle it
      return fclose(stream);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return EOF;
  }

  // should be moved to CFile classes
  intptr_t dll_findfirst(const char *file, struct _finddata_t *data)
  {
    char str[XBMC_MAX_PATH];
    int size = sizeof(str);
    CURL url(_P(file));
    if (url.IsLocal())
    {
      // move to CFile classes
      if (strncmp(file, "\\Device\\Cdrom0", 14) == 0)
      {
        // replace "\\Device\\Cdrom0" with "D:"
        strncpy(str, "D:", size);
        if (size)
        {
          str[size - 1] = '\0';
          strncat(str, file + 14, size - strlen(str));
        }
      }
      else
      {
        strncpy(str, file, size);
        if (size)
          str[size - 1] = '\0';
      }

      // Make sure the slashes are correct & translate the path
      return _findfirst(CUtil::ValidatePath(_P(str), true), data);
    }
    // non-local files. handle through IDirectory-class - only supports '*.bah' or '*.*'
    CStdString strURL(file);
    CStdString strMask;
    if (url.GetFileName().Find("*.*") != string::npos)
    {
      CStdString strReplaced = url.GetFileName();
      strReplaced.Replace("*.*","");
      url.SetFileName(strReplaced);
    }
    else if (url.GetFileName().Find("*.") != string::npos)
    {
      URIUtils::GetExtension(url.GetFileName(),strMask);
      url.SetFileName(url.GetFileName().Left(url.GetFileName().Find("*.")));
    }
    int iDirSlot=0; // locate next free directory
    while ((vecDirsOpen[iDirSlot].Directory) && (iDirSlot<MAX_OPEN_DIRS)) iDirSlot++;
    if (iDirSlot > MAX_OPEN_DIRS)
      return 0xFFFF; // no free slots
    if (url.GetProtocol().Equals("filereader"))
    {
      CURL url2(url.GetFileName());
      url = url2;
    }
    CStdString fName = url.GetFileName();
    url.SetFileName("");
    strURL = url.Get();
    bVecDirsInited = true;
    vecDirsOpen[iDirSlot].items.Clear();
    vecDirsOpen[iDirSlot].Directory = CFactoryDirectory::Create(strURL);
    vecDirsOpen[iDirSlot].Directory->SetMask(strMask);
    vecDirsOpen[iDirSlot].Directory->GetDirectory(strURL+fName,vecDirsOpen[iDirSlot].items);
    if (vecDirsOpen[iDirSlot].items.Size())
    {
      int size = sizeof(data->name);
      strncpy(data->name,vecDirsOpen[iDirSlot].items[0]->GetLabel().c_str(), size);
      if (size)
        data->name[size - 1] = '\0';
      data->size = static_cast<_fsize_t>(vecDirsOpen[iDirSlot].items[0]->m_dwSize);
      data->time_write = iDirSlot; // save for later lookups
      data->time_access = 0;
      delete vecDirsOpen[iDirSlot].Directory;
      vecDirsOpen[iDirSlot].Directory = NULL;
      return NULL;
    }
    delete vecDirsOpen[iDirSlot].Directory;
    vecDirsOpen[iDirSlot].Directory = NULL;
    return 0xFFFF; // whatever != NULL
  }

  // should be moved to CFile classes
  int dll_findnext(intptr_t f, _finddata_t* data)
  {
    if ((data->time_write < 0) || (data->time_write > MAX_OPEN_DIRS)) // assume not one of our's
      return _findnext(f, data); // local dir

    // we have a valid data struture. get next item!
    int iItem=data->time_access;
    if (iItem+1 < vecDirsOpen[data->time_write].items.Size()) // we have a winner!
    {
      int size = sizeof(data->name);
      strncpy(data->name,vecDirsOpen[data->time_write].items[iItem+1]->GetLabel().c_str(), size);
      if (size)
        data->name[size - 1] = '\0';
      data->size = static_cast<_fsize_t>(vecDirsOpen[data->time_write].items[iItem+1]->m_dwSize);
      data->time_access++;
      return 0;
    }

    vecDirsOpen[data->time_write].items.Clear();
    return -1;
  }

  int dll_findclose(intptr_t handle)
  {
    not_implement("msvcrt.dll fake function dll_findclose() called\n");
    return 0;
  }

  char* dll_fgets(char* pszString, int num ,FILE * stream)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(stream);
    if (pFile != NULL)
    {
      if (pFile->GetPosition() < pFile->GetLength())
      {
        bool bRead = pFile->ReadString(pszString, num);
        if (bRead)
        {
          return pszString;
        }
      }
      else return NULL; //eof
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return fgets(pszString, num, stream);
    } 
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return NULL;
  }

  int dll_feof(FILE * stream)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(stream);
    if (pFile != NULL)
    {
      if (pFile->GetPosition() < pFile->GetLength()) return 0;
      else return 1;
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return feof(stream);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return 1; // eof by default
  }

  int dll_fread(void * buffer, size_t size, size_t count, FILE * stream)
  {
    int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
    if (fd >= 0)
    {
      int iItemsRead = dll_read(fd, buffer, count * size);
      if (iItemsRead >= 0)
      {
        if (size)
          iItemsRead /= size;
        return iItemsRead;
      }
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, let the operating system handle it
      return fread(buffer, size, count, stream);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  int dll_fgetc(FILE* stream)
  {
    if (g_emuFileWrapper.StreamIsEmulatedFile(stream))
    {
      // it is a emulated file
      unsigned char buf;
      
      if (dll_fread(&buf, 1, 1, stream) <= 0)
        return EOF;

      return (int)buf;
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return getc(stream);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return EOF;
  }

  int dll_getc(FILE* stream)
  {
    if (g_emuFileWrapper.StreamIsEmulatedFile(stream))
    {
      // This routine is normally implemented as a macro with the same result as fgetc().
      return dll_fgetc(stream);
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return getc(stream);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return EOF;
  }
  
  FILE* dll_fopen(const char* filename, const char* mode)
  {
    FILE* file = NULL;
    
    int iMode = O_BINARY;
    if (strstr(mode, "r+"))
      iMode |= O_RDWR;
    else if (strchr(mode, 'r'))
      iMode |= _O_RDONLY;
    if (strstr(mode, "w+"))
      iMode |= O_RDWR | _O_TRUNC;
    else if (strchr(mode, 'w'))
      iMode |= _O_WRONLY  | O_CREAT;
      
    int fd = dll_open(filename, iMode);
    if (fd >= 0)
    {
      file = g_emuFileWrapper.GetStreamByDescriptor(fd);;
    }
    
    return file;
  }

  int dll_putc(int c, FILE *stream)
  {
    if (g_emuFileWrapper.StreamIsEmulatedFile(stream) || IS_STD_STREAM(stream))
    {
      return dll_fputc(c, stream);
    }
    else
    {
      return putc(c, stream);
    }
    return EOF;
  }

  int dll_putchar(int c)
  {
    return dll_putc(c, stdout);
  }
  
  int dll_fputc(int character, FILE* stream)
  {
    if (IS_STDOUT_STREAM(stream) || IS_STDERR_STREAM(stream))
    {
      char tmp[2] = { (char)character, 0 };
      dllputs(tmp);
      return character;
    }
    else
    {
      if (g_emuFileWrapper.StreamIsEmulatedFile(stream))
      {
        int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
        if (fd >= 0)
        {
          int iItemsWritten = dll_write(fd, (char* )&character, 1);
          if (iItemsWritten == 1)
            return character;
        }
      }
      else if (!IS_STD_STREAM(stream))
      {
        // it might be something else than a file, or the file is not emulated
        // let the operating system handle it
        return fputc(character, stream);
      }
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return EOF;
  }

  int dll_fputs(const char * szLine, FILE* stream)
  {
    if (IS_STDOUT_STREAM(stream) || IS_STDERR_STREAM(stream))
    {
      dllputs(szLine);
      return 0;
    }
    else
    {
      if (g_emuFileWrapper.StreamIsEmulatedFile(stream))
      {
        int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
        if (fd >= 0)
        {
          int iItemsWritten = dll_write(fd, &szLine, strlen(szLine));
          if (iItemsWritten != -1 )
            return 1;
        }
      }
      else if (!IS_STD_STREAM(stream))
      {
        // it might be something else than a file, or the file is not emulated
        // let the operating system handle it
        return fputs(szLine, stream);
      }
    }
    
    OutputDebugString(szLine);
    OutputDebugString("\n");
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return EOF;
  }

  int dll_fseek(FILE* stream, long offset, int origin)
  {
    int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
    if (fd >= 0)
    {
      if (dll_lseek(fd, offset, origin) != -1)
      {
        return 0;
      }
      else return -1;
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return fseeko64(stream, offset, origin);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  int dll_ungetc(int c, FILE* stream)
  {
    if (g_emuFileWrapper.StreamIsEmulatedFile(stream))
    {
      // it is a emulated file
      char szString[10];
      if (dll_fseek(stream, -1, SEEK_CUR)!=0)
      {
        return -1;
      }
      if (dll_fread(&szString[0], 1, 1, stream) <= 0)
      {
        return -1;
      }
      if (dll_feof(stream))
      {
        return -1;
      }

      byte byKar = (byte)szString[0];
      int iKar = byKar;
      return iKar;
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return ungetc(c, stream);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return EOF;
  }

  long dll_ftell(FILE *stream)
  {
    return (long)dll_ftell64(stream);
  }

  off64_t dll_ftell64(FILE *stream)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(stream);
    if (pFile != NULL)
    {
       return (off64_t)pFile->GetPosition();
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return ftello64(stream);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  long dll_tell(int fd)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
       return (long)pFile->GetPosition();
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return tell(fd);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  __int64 dll_telli64(int fd)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
       return (__int64)pFile->GetPosition();
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      // not supported return telli64(fd);
      CLog::Log(LOGWARNING, "msvcrt.dll: dll_telli64 called, TODO: add 'int64 -> long' type checking");      //warning
      return (__int64)tell(fd);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  size_t dll_fwrite(const void * buffer, size_t size, size_t count, FILE* stream)
  {
    if (IS_STDOUT_STREAM(stream) || IS_STDERR_STREAM(stream))
    {
      char* buf = (char*)malloc(size * count + 1);
      if (buf)
      {
        memcpy(buf, buffer, size * count);
        buf[size * count] = 0; // string termination
        
        CLog::Log(LOGDEBUG, "%s", buf);
        
        free(buf);
        return count;
      }
    }
    else
    {
      int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
      if (fd >= 0)
      {
        int iItemsWritten = dll_write(fd, buffer, count * size);
        if (iItemsWritten >= 0)
        {
          iItemsWritten /= size;
          return iItemsWritten;
        }
      }
      else if (!IS_STD_STREAM(stream))
      {
        // it might be something else than a file, or the file is not emulated
        // let the operating system handle it
        return fwrite(buffer, size, count, stream);
      }
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  int dll_fflush(FILE* stream)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(stream);
    if (pFile != NULL)
    {
      pFile->Flush();
      return 0;
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return fflush(stream);
    }
    
    // std stream, no need to flush
    return 0;
  }

  int dll_ferror(FILE* stream)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(stream);
    if (pFile != NULL)
    {
      // unimplemented
      return 0;
    }
    else if (IS_STD_STREAM(stream))
      return 0;
    else
      return ferror(stream);

    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return -1;
  }

  int dllvprintf(const char *format, va_list va)
  {
    CStdString buffer;
    buffer.FormatV(format, va);
    CLog::Log(LOGDEBUG, "  msg: %s", buffer.c_str());
    return buffer.length();
  }

  int dll_vfprintf(FILE *stream, const char *format, va_list va)
  {
    static char tmp[2048];

    if (_vsnprintf(tmp, 2048, format, va) == -1)
    {
      CLog::Log(LOGWARNING, "dll_vfprintf: Data lost due to undersized buffer");
    }
    tmp[2048 - 1] = 0;
    
    if (IS_STDOUT_STREAM(stream) || IS_STDERR_STREAM(stream))
    {
      CLog::Log(LOGINFO, "  msg: %s", tmp);
      return strlen(tmp);
    }
    else
    {
      CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(stream);
      if (pFile != NULL)
      {
        int len = strlen(tmp);
        // replace all '\n' occurences with '\r\n'...
        char tmp2[2048];
        int j = 0;
        for (int i = 0; i < len; i++)
        {
          if (j == 2047)
          { // out of space
            if (i != len-1)
              CLog::Log(LOGWARNING, "dll_fprintf: Data lost due to undersized buffer");
            break;
          }
          if (tmp[i] == '\n' && ((i > 0 && tmp[i-1] != '\r') || i == 0) && j < 2047 - 2)
          { // need to add a \r
            tmp2[j++] = '\r';
            tmp2[j++] = '\n';
          }
          else
          { // just add the character as-is
            tmp2[j++] = tmp[i];
          }
        }
        // terminate string
        tmp2[j] = 0;
        len = strlen(tmp2);
        pFile->Write(tmp2, len);
        return len;
      }
      else if (!IS_STD_STREAM(stream))
      {
        // it might be something else than a file, or the file is not emulated
        // let the operating system handle it
        return vfprintf(stream, format, va);
      }
    }
    
    OutputDebugString(tmp);
    OutputDebugString("\n");
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return strlen(tmp);
  }

  int dll_fprintf(FILE* stream, const char* format, ...)
  {
    int res;
    va_list va;
    va_start(va, format);
    res = dll_vfprintf(stream, format, va);
    va_end(va);
    return res;
  }
  
  int dll_fgetpos(FILE* stream, fpos_t* pos)
  {
    fpos64_t tmpPos;
    int ret;

    ret = dll_fgetpos64(stream, &tmpPos);
#if !defined(_LINUX) || defined(__APPLE__)
    *pos = (fpos_t)tmpPos;
#else
    pos->__pos = (off_t)tmpPos.__pos;
#endif
    return ret;
  }

  int dll_fgetpos64(FILE *stream, fpos64_t *pos)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByStream(stream);
    if (pFile != NULL)
    {
      *pos = pFile->GetPosition();
      return 0;
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return fgetpos(stream, pos);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return EINVAL;
  }

  int dll_fsetpos(FILE* stream, const fpos_t* pos)
  {
    int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
    if (fd >= 0)
    {
      if (dll_lseeki64(fd, *pos, SEEK_SET) >= 0)
      {
        return 0;
      }
      else
      {
        return EINVAL;
      }
    }
    else if (!IS_STD_STREAM(stream))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return fsetpos(stream, (fpos_t*)pos);
    }
    CLog::Log(LOGERROR, "%s emulated function failed",  __FUNCTION__);
    return EINVAL;
  }

  int dll_fileno(FILE* stream)
  {
    int fd = g_emuFileWrapper.GetDescriptorByStream(stream);
    if (fd >= 0)
    {
      return fd;
    }
    else if (IS_STDIN_STREAM(stream))
    {
      return 0;
    }
    else if (IS_STDOUT_STREAM(stream))
    {
      return 1;
    }
    else if (IS_STDERR_STREAM(stream))
    {
      return 2;
    }
    else
    {
      return fileno(stream);
    }

    return -1;
  }

  void dll_clearerr(FILE* stream)
  {
    if (g_emuFileWrapper.StreamIsEmulatedFile(stream))
    {
      // not implemented
    }
    else if (!IS_STD_STREAM(stream))
    {
      return clearerr(stream);
    }
  }

  char* dll_strdup( const char* str)
  {
    char* pdup;
    pdup = strdup(str);
    return pdup;
  }


  //Critical Section has been fixed in EMUkernel32.cpp

  int dll_initterm(PFV * start, PFV * end)        //pncrt.dll
  {
    PFV * temp;
    for (temp = start; temp < end; temp ++)
      if (*temp)
        (*temp)(); //call initial function table.
    return 0;
  }

  HANDLE dll_beginthreadex(LPSECURITY_ATTRIBUTES lpThreadAttributes, DWORD dwStackSize,
                           LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags,
                           LPDWORD lpThreadId)
  {
    return dllCreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
  }

  //SLOW CODE SHOULD BE REVISED
  int dll_stat(const char *path, struct _stat *buffer)
  {
    //stating a root, for example C:\\, failes on the xbox
    if (emu_is_root_drive(path))
    {
        buffer->st_dev = 4294967280;
        buffer->st_ino = 0;
        buffer->st_mode = 16895;
        buffer->st_nlink = 1;
        buffer->st_uid = 0;
        buffer->st_gid = 0;
        buffer->st_rdev = 4294967280;
        buffer->st_size = 0;
        buffer->st_atime = 1000000000;
        buffer->st_mtime = 1000000000;
        buffer->st_ctime = 1000000000;
        return 0;
    }
  
    if (!strnicmp(path, "shout://", 8)) // don't stat shoutcast
      return -1;
    if (!strnicmp(path, "http://", 7)
    ||  !strnicmp(path, "https://", 8)) // don't stat http
      return -1;
    if (!strnicmp(path, "mms://", 6)) // don't stat mms
      return -1;
      
#ifndef _LINUX
    // check for remaining letter drives
    if (path && isalpha(path[0]) && path[1] == ':' && ( strlen(path) == 2 || strlen(path) == 3 ) )
#else
    if (!_stricmp(path, "D:") || !_stricmp(path, "D:\\"))
#endif
    {
      buffer->st_mode = S_IFDIR;
      return 0;
    }
    if (!stricmp(path, "\\Device\\Cdrom0") || !stricmp(path, "\\Device\\Cdrom0\\"))
    {
      buffer->st_mode = _S_IFDIR;
      return 0;
    }

    struct __stat64 tStat;
    if (CFile::Stat(path, &tStat) == 0)
    {
      CUtil::Stat64ToStat(buffer, &tStat);
      return 0;
    }
    // errno is set by file.Stat(...)
    return -1;
  }

  int dll_stati64(const char *path, struct _stati64 *buffer)
  {
    struct __stat64 a;
    if(dll_stat64(path, &a) == 0)
    {
      CUtil::Stat64ToStatI64(buffer, &a);
      return 0;
    }
    return -1;
  }

  int dll_stat64(const char *path, struct __stat64 *buffer)
  {
    //stating a root, for example C:\\, failes on the xbox
    if (emu_is_root_drive(path))
    {
        buffer->st_dev = 4294967280;
        buffer->st_ino = 0;
        buffer->st_mode = 16895;
        buffer->st_nlink = 1;
        buffer->st_uid = 0;
        buffer->st_gid = 0;
        buffer->st_rdev = 4294967280;
        buffer->st_size = 0;
        buffer->st_atime = 1000000000;
        buffer->st_mtime = 1000000000;
        buffer->st_ctime = 1000000000;
        return 0;
    }
  
    if (!strnicmp(path, "shout://", 8)) // don't stat shoutcast
      return -1;
    if (!strnicmp(path, "http://", 7)
    ||  !strnicmp(path, "https://", 8)) // don't stat http
      return -1;
    if (!strnicmp(path, "mms://", 6)) // don't stat mms
      return -1;

#ifndef _LINUX
    // check for remaining letter drives
    if (path && isalpha(path[0]) && path[1] == ':' && ( strlen(path) == 2 || strlen(path) == 3 ) )
#else
    if (!_stricmp(path, "D:") || !_stricmp(path, "D:\\"))
#endif
    {
      buffer->st_mode = _S_IFDIR;
      return 0;
    }
    if (!stricmp(path, "\\Device\\Cdrom0") || !stricmp(path, "\\Device\\Cdrom0\\"))
    {
      buffer->st_mode = _S_IFDIR;
      return 0;
    }

    return CFile::Stat(path, buffer);
  }


  int dll_fstat(int fd, struct stat* buffer)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
      CLog::Log(LOGINFO, "Stating open file");
    
      __int64 size = pFile->GetLength();
      if (size <= LONG_MAX)
        buffer->st_size = (_off_t)size;
      else
      {
        buffer->st_size = 0;
        CLog::Log(LOGWARNING, "WARNING: File is larger than 32bit stat can handle, file size will be reported as 0 bytes");
      }
      buffer->st_mode = _S_IFREG;

      return 0;
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      return fstat(fd, buffer);
    }
    
    // fstat on stdin, stdout or stderr should fail
    // this is what python expects
    return -1;
  }

  int dll_fstati64(int fd, struct _stati64 *buffer)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
      CLog::Log(LOGINFO, "Stating open file");
      
      buffer->st_size = pFile->GetLength();
      buffer->st_mode = _S_IFREG;
      return 0;
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      CLog::Log(LOGWARNING, "msvcrt.dll: dll_fstati64 called, TODO: add 'int64 <-> long' type checking");      //warning
      // need to use fstat and convert everything
      struct stat temp;
      int res = fstat(fd, &temp);
      if (res == 0)
      {
        CUtil::StatToStatI64(buffer, &temp);
      }
      return res;
    }
    
    // fstat on stdin, stdout or stderr should fail
    // this is what python expects
    return -1;
  }

  int dll_setmode ( int handle, int mode )
  {
    not_implement("msvcrt.dll fake function dll_setmode() called\n");
    return -1;
  }

  void dllperror(const char* s)
  {
    if (s)
    {
      CLog::Log(LOGERROR, "perror: %s", s);
    }
  }

  char* dllstrerror(int iErr)
  {
    static char szError[32];
    sprintf(szError, "err:%i", iErr);
    return (char*)szError;
  }

  int dll_mkdir(const char* dir)
  {
    if (!dir) return -1;
    // Make sure the slashes are correct & translate the path
    return mkdir( CUtil::ValidatePath(_P(dir), true).c_str() );
  }

  char* dll_getcwd(char *buffer, int maxlen)
  {
    not_implement("msvcrt.dll fake function dll_getcwd() called\n");
    return (char*)"Q:";
  }

  int dll_putenv(const char* envstring)
  {
    bool added = false;
    
    if (envstring != NULL)
    {
      const char *value_start = strchr(envstring, '=');
      
      if (value_start != NULL)
      {
        char var[64];
        int size = strlen(envstring) + 1;
        char *value = (char*)malloc(size);
        
        if (!value)
          return -1;
        value[0] = 0;
        
        memcpy(var, envstring, value_start - envstring);
        var[value_start - envstring] = 0;
        strupr(var);
        
        strncpy(value, value_start + 1, size);
        if (size)
          value[size - 1] = '\0';

        EnterCriticalSection(&dll_cs_environ);
        
        char** free_position = NULL;
        for (int i = 0; i < EMU_MAX_ENVIRONMENT_ITEMS && free_position == NULL; i++)
        {
          if (dll__environ[i] != NULL)
          {
            // we only support overwriting the old values
            if (strnicmp(dll__environ[i], var, strlen(var)) == 0)
            {
              // free it first
              free(dll__environ[i]);
              dll__environ[i] = NULL;
              free_position = &dll__environ[i];
            }
          }
          else
          {
            free_position = &dll__environ[i];
          }
        }
        
        if (free_position != NULL)
        {
          // free position, copy value
          size = strlen(var) + strlen(value) + 2;
          *free_position = (char*)malloc(size); // for '=' and 0 termination
          if ((*free_position))
          {
            strncpy(*free_position, var, size);
            (*free_position)[size - 1] = '\0';
            strncat(*free_position, "=", size - strlen(*free_position));
            strncat(*free_position, value, size - strlen(*free_position));
            added = true;
          }
        }
        
        LeaveCriticalSection(&dll_cs_environ);

        free(value);
      }
    }
    
    return added ? 0 : -1;
  }


#ifdef _XBOX
  char *getenv(const char *s)
  {
    // some libs in the solution linked to getenv which was exported in python.lib
    // now python is in a dll this needs the be fixed, or not 
    CLog::Log(LOGWARNING, "old getenv from python.lib called, library check needed");
    return NULL;
  }
#endif

  char* dll_getenv(const char* szKey)
  {
    char* value = NULL;

    EnterCriticalSection(&dll_cs_environ);

    update_emu_environ();//apply any changes

    for (int i = 0; i < EMU_MAX_ENVIRONMENT_ITEMS && value == NULL; i++)
    {
      if (dll__environ[i])
      {
        if (strnicmp(dll__environ[i], szKey, strlen(szKey)) == 0)
        {
          // found it
          value = dll__environ[i] + strlen(szKey) + 1;
        }
      }
    }
    
    LeaveCriticalSection(&dll_cs_environ);
    
    if (value != NULL)
    {
      return value;
    }
    
    return NULL;
  }
  
  int dll_ctype(int i)
  {
    not_implement("msvcrt.dll fake function dll_ctype() called\n");
    return 0;
  }

  int dll_system(const char *command)
  {
    not_implement("msvcrt.dll fake function dll_system() called\n");
    return 0; //system(command);
  }

  void (__cdecl * dll_signal(int sig, void (__cdecl *func)(int)))(int)
  {
    // the xbox has a NSIG of 23 (+1), problem is when calling signal with 
    // one of the signals below the xbox wil crash. Just return SIG_ERR
    if (sig == SIGILL || sig == SIGFPE || sig == SIGSEGV) return SIG_ERR;
    
    return signal(sig, func);
  }

  int dll_getpid()
  {
    return 1;
  }
  
  int dll__commit(int fd)
  {
    CFile* pFile = g_emuFileWrapper.GetFileXbmcByDescriptor(fd);
    if (pFile != NULL)
    {
      pFile->Flush();
      return 0;
    }
    else if (!IS_STD_DESCRIPTOR(fd))
    {
      // it might be something else than a file, or the file is not emulated
      // let the operating system handle it
      return _commit(fd);
    }
    
    // std stream, no need to flush
    return 0;
  }
  
  char*** dll___p__environ()
  {
    static char*** t = (char***)&dll__environ;
    return (char***)&t;
  }

  int dll_setvbuf(FILE *stream, char *buf, int type, size_t size)
  {
    CLog::Log(LOGWARNING, "%s - May not be implemented correctly",
              __FUNCTION__);
    return 0;
  }
}


