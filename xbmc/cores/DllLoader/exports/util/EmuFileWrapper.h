#pragma once

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

#include <stdio.h>

#include "system.h"

#define MAX_EMULATED_FILES    50
#define FILE_WRAPPER_OFFSET   0x00000100

namespace XFILE
{
  class CFile;
}

typedef struct stEmuFileObject
{
  bool    used;
  FILE    file_emu;
  XFILE::CFile*  file_xbmc;
} EmuFileObject;
  
class CEmuFileWrapper
{
public:
  CEmuFileWrapper();
  ~CEmuFileWrapper();
  
  /**
   * Only to be called when shutting down xbmc
   */
  void CleanUp();
  
  EmuFileObject* RegisterFileObject(XFILE::CFile* pFile);
  void UnRegisterFileObjectByDescriptor(int fd);
  void UnRegisterFileObjectByStream(FILE* stream);
  EmuFileObject* GetFileObjectByDescriptor(int fd);  
  EmuFileObject* GetFileObjectByStream(FILE* stream);  
  XFILE::CFile* GetFileXbmcByDescriptor(int fd);
  XFILE::CFile* GetFileXbmcByStream(FILE* stream);
  int GetDescriptorByStream(FILE* stream);
  FILE* GetStreamByDescriptor(int fd);
  bool DescriptorIsEmulatedFile(int fd);
  bool StreamIsEmulatedFile(FILE* stream);
private:
  EmuFileObject m_files[MAX_EMULATED_FILES];
  CRITICAL_SECTION m_criticalSection;
  bool m_initialized;
};

extern CEmuFileWrapper g_emuFileWrapper;

