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

#include "GUIWindow.h"
#include "lib/libPython/python/Include/Python.h"

class PyXBMCAction
{
public:
  int param;
  PyObject* pCallbackWindow;
  PyObject* pObject;
  int controlId; // for XML window
  PyXBMCAction() { }
  //virtual ~PyXBMCAction();
};

int Py_XBMC_Event_OnAction(void* arg);
int Py_XBMC_Event_OnControl(void* arg);

class CGUIPythonWindow : public CGUIWindow
{
public:
  CGUIPythonWindow(int id);
  virtual ~CGUIPythonWindow(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual bool    OnAction(const CAction &action);
  virtual bool    OnBack(int actionID);
  void             SetCallbackWindow(PyObject *object);
  void             WaitForActionEvent(unsigned int timeout);
  void             PulseActionEvent();
protected:
  PyObject*        pCallbackWindow;
  HANDLE           m_actionEvent;
};
