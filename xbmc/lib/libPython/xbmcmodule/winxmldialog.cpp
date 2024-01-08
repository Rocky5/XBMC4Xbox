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
#include "winxml.h"
#include "lib/libPython/python/Include/Python.h"
#include "../XBPythonDll.h"
#include "pyutil.h"
#include "GUIPythonWindowXMLDialog.h"
#include "SkinInfo.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "FileSystem/File.h"

#define ACTIVE_WINDOW  g_windowManager.GetActiveWindow()

#ifndef __GNUC__
#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")
#endif 

#ifdef __cplusplus
extern "C" {
#endif

using namespace std;

namespace PYXBMC
{
  PyObject* WindowXMLDialog_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    WindowXMLDialog *self;

    self = (WindowXMLDialog*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    self->iWindowId = -1;
    PyObject* pyOXMLname = NULL;
    PyObject* pyOname = NULL;
    PyObject* pyDName = NULL;
    PyObject* pyRes = NULL;

    string strXMLname, strFallbackPath;
    string strDefault = "Default";
    string resolution = "720p";

    if (!PyArg_ParseTuple(args, (char*)"OO|OO", &pyOXMLname, &pyOname, &pyDName, &pyRes)) return NULL;

    PyXBMCGetUnicodeString(strXMLname, pyOXMLname);
    PyXBMCGetUnicodeString(strFallbackPath, pyOname);
    if (pyDName) PyXBMCGetUnicodeString(strDefault, pyDName);
    if (pyRes) PyXBMCGetUnicodeString(resolution, pyRes);

    RESOLUTION res = INVALID;
    CStdString strSkinPath = g_SkinInfo.GetSkinPath(strXMLname, &res);

    // Check to see if the XML file exists in current skin. If not use fallback path to find a skin for the script
    if (!XFILE::CFile::Exists(strSkinPath))
    {
      // Check for the matching folder for the skin in the fallback skins folder
      CStdString fallbackPath = URIUtils::AddFileToFolder(strFallbackPath, "resources");
      fallbackPath = URIUtils::AddFileToFolder(fallbackPath, "skins");
      CStdString basePath = URIUtils::AddFileToFolder(fallbackPath, URIUtils::GetFileName(g_SkinInfo.GetBaseDir()));
      strSkinPath = g_SkinInfo.GetSkinPath(strXMLname, &res, basePath);
      if (!XFILE::CFile::Exists(strSkinPath))
      {
        // Finally fallback to the DefaultSkin as it didn't exist in either the XBMC Skin folder or the fallback skin folder
        CStdString basePath = URIUtils::AddFileToFolder(fallbackPath, strDefault);
        res = CSkinInfo::TranslateResolution(resolution, HDTV_720p);
        CSkinInfo skinInfo;
        strSkinPath = skinInfo.GetSkinPath(strXMLname, &res, basePath);
        if (!XFILE::CFile::Exists(strSkinPath))
        {
          skinInfo.Load(basePath);
          strSkinPath = skinInfo.GetSkinPath(strXMLname, &res, basePath);

          if (!XFILE::CFile::Exists(strSkinPath))
          {
            CStdString error;
            error.Format("XML file(%s) for window is missing", strSkinPath);
            PyErr_SetString(PyExc_TypeError, error.c_str());
            return NULL;
          }
        }
      }
    }
    self->sFallBackPath = strFallbackPath;
    self->sXMLFileName = strSkinPath;
    self->bUsingXML = true;

    // create new GUIWindow
    if (!Window_CreateNewWindow((Window*)self, true))
    {
      // error is already set by Window_CreateNewWindow, just release the memory
      self->ob_type->tp_free((PyObject*)self);
      return NULL;
    }
    ((CGUIWindow*)(self->pWindow))->SetCoordsRes(res);
    return (PyObject*)self;
  }

  PyDoc_STRVAR(windowXMLDialog__doc__,
    "WindowXMLDialog class.\n"
    "\n"
    "WindowXMLDialog(self, xmlFilename, scriptPath[, defaultSkin, defaultRes]) -- Create a new WindowXMLDialog script.\n"
    "\n"
    "xmlFilename     : string - the name of the xml file to look for.\n"
    "scriptPath      : string - path to script. used to fallback to if the xml doesn't exist in the current skin. (eg os.getcwd())\n"
    "defaultSkin     : [opt] string - name of the folder in the skins path to look in for the xml. (default='Default')\n"
    "defaultRes      : [opt] string - default skins resolution. (default='720p')\n"
    "\n"
    "*Note, skin folder structure is eg(resources/skins/Default/720p)\n"
    "\n"
    "example:\n"
    " - ui = GUI('script-Lyrics-main.xml', os.getcwd(), 'LCARS', 'PAL')\n"
    "   ui.doModal()\n"
    "   del ui\n");

  PyMethodDef windowXMLDialog_methods[] = {
    {NULL, NULL, 0, NULL}
  };
// Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject WindowXMLDialog_Type;

  void initWindowXMLDialog_Type()
  {
    PyXBMCInitializeTypeObject(&WindowXMLDialog_Type);

    WindowXMLDialog_Type.tp_name = (char*)"xbmcgui.WindowXMLDialog";
    WindowXMLDialog_Type.tp_basicsize = sizeof(WindowXMLDialog);
    WindowXMLDialog_Type.tp_dealloc = (destructor)Window_Dealloc;
    WindowXMLDialog_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    WindowXMLDialog_Type.tp_doc = windowXMLDialog__doc__;
    WindowXMLDialog_Type.tp_methods = windowXMLDialog_methods;
    WindowXMLDialog_Type.tp_base = &WindowXML_Type;
    WindowXMLDialog_Type.tp_new = WindowXMLDialog_New;
  }
}

#ifdef __cplusplus
}
#endif


