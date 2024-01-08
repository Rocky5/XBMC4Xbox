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

#include "utils/StdString.h"

#include "lib/libPython/python/Include/Python.h"
#include "../XBPythonDll.h"

#ifdef __cplusplus
extern "C" {
#endif

// credits and version information
#define PY_XBMC_AUTHOR    "J. Mulder <darkie@xbmc.org>"
#define PY_XBMC_CREDITS   "XBMC TEAM."
#define PY_XBMC_PLATFORM  "XBOX"

namespace PYXBMC
{
  int   PyXBMCGetUnicodeString(std::string& buf, PyObject* pObject, int pos = -1);
  void  PyXBMCGUILock();
  void  PyXBMCGUIUnlock();
  const char* PyXBMCGetDefaultImage(char* controlType, char* textureType, char* cDefault);
  bool  PyXBMCWindowIsNull(void* pWindow);

  void  PyXBMCInitializeTypeObject(PyTypeObject* type_object);
  long PyXBMCLongAsStringOrLong(PyObject *value);
  void  PyXBMCWaitForThreadMessage(int message, int param1, int param2);
}

#ifdef __cplusplus
}
#endif
