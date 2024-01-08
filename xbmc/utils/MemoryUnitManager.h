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

#include <vector>
#include "settings/Settings.h"  // for VECSOURCES

class IDevice;
class IFileSystem;

class CMemoryUnitManager
{
public:
  CMemoryUnitManager();

  // update the memory units (plug, unplug)
  bool Update();

  bool IsDriveValid(char Drive);    // for backward compatibility
                                    // with fatx drives in filezilla

  IDevice *GetDevice(unsigned char unit) const;
  IFileSystem *GetFileSystem(unsigned char unit);

  bool IsDriveWriteable(const CStdString &path) const;

  void GetMemoryUnitSources(VECSOURCES &shares);


private:
  void Notify(unsigned long port, unsigned long slot, bool success);

  bool HasDevice(unsigned long port, unsigned long slot);
  bool MountDevice(unsigned long port, unsigned long slot);
  bool UnMountDevice(unsigned long port, unsigned long slot);
  
  void MountUnits(unsigned long device, bool notify);
  void UnMountUnits(unsigned long device);

  char DriveLetterFromPort(unsigned long port, unsigned long slot);

  void DumpImage(const CStdString &path, unsigned char unit, unsigned long sectors);

  std::vector<IDevice *> m_memUnits;

  bool m_initialized;
};

extern CMemoryUnitManager g_memoryUnitManager;
