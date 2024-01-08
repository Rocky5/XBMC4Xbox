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

#ifdef _XBOX
#pragma comment(linker, "/merge:PY_TEXT=PYTHON")
#pragma comment(linker, "/merge:PY_DATA=PY_RW")
#pragma comment(linker, "/merge:PY_BSS=PY_RW")
#pragma comment(linker, "/merge:PY_RDATA=PYTHON")
#endif

// python.h should always be included first before any other includes
#include "system.h"
#include "python/Include/Python.h"
#include "cores/DllLoader/DllLoaderContainer.h"
#include "GUIPassword.h"

#include "XBPython.h"
#include "XBPythonDll.h"
#include "Application.h"
#include "settings/Settings.h"
#include "settings/Profile.h"
#include "FileSystem/File.h"
#include "FileSystem/SpecialProtocol.h"
#include "xbox/network.h"
#include "settings/Settings.h"
#include "utils/log.h"

XBPython g_pythonParser;

#define PYTHON_DLL "Q:\\system\\python\\python27.dll"
#define PYTHON_LIBDIR "Q:\\system\\python\\lib\\"
#define PYTHON_EXT "Q:\\system\\python\\lib\\*.pyd"

extern "C" HMODULE __stdcall dllLoadLibraryA(LPCSTR file);
extern "C" BOOL __stdcall dllFreeLibrary(HINSTANCE hLibModule);


XBPython::XBPython()
{
  m_bInitialized = false;
  bThreadInitialize = false;
  bStartup = false;
  bLogin = false;
  nextid = 0;
  mainThreadState = NULL;
  m_hEvent = CreateEvent(NULL, false, false, (char*)"pythonEvent");
  m_globalEvent = CreateEvent(NULL, false, false, (char*)"pythonGlobalEvent");
  dThreadId = CThread::GetCurrentThreadId();
  m_iDllScriptCounter = 0;
  vecPlayerCallbackList.clear();
}

XBPython::~XBPython()
{
  CloseHandle(m_globalEvent);
}

// message all registered callbacks that xbmc stopped playing
void XBPython::OnPlayBackEnded()
{
  CSingleLock lock (m_critSection);
  if (m_bInitialized)
  {
    PlayerCallbackList::iterator it = vecPlayerCallbackList.begin();
    while (it != vecPlayerCallbackList.end())
    {
      ((IPlayerCallback*)(*it))->OnPlayBackEnded();
      it++;
    }
  }
}

// message all registered callbacks that we started playing
void XBPython::OnPlayBackStarted()
{
  CSingleLock lock (m_critSection);
  if (m_bInitialized)
  {
    PlayerCallbackList::iterator it = vecPlayerCallbackList.begin();
    while (it != vecPlayerCallbackList.end())
    {
      ((IPlayerCallback*)(*it))->OnPlayBackStarted();
      it++;
    }
  }
}

// message all registered callbacks that we paused playing
void XBPython::OnPlayBackPaused()
{
  if (m_bInitialized)
  {
    PlayerCallbackList::iterator it = vecPlayerCallbackList.begin();
    while (it != vecPlayerCallbackList.end())
    {
      ((IPlayerCallback*)(*it))->OnPlayBackPaused();
      it++;
    }
  }
}

// message all registered callbacks that we resumed playing
void XBPython::OnPlayBackResumed()
{
  if (m_bInitialized)
  {
    PlayerCallbackList::iterator it = vecPlayerCallbackList.begin();
    while (it != vecPlayerCallbackList.end())
    {
      ((IPlayerCallback*)(*it))->OnPlayBackResumed();
      it++;
    }
  }
}

// message all registered callbacks that user stopped playing
void XBPython::OnPlayBackStopped()
{
  CSingleLock lock (m_critSection);
  if (m_bInitialized)
  {
    PlayerCallbackList::iterator it = vecPlayerCallbackList.begin();
    while (it != vecPlayerCallbackList.end())
    {
      ((IPlayerCallback*)(*it))->OnPlayBackStopped();
      it++;
    }
  }
}

void XBPython::RegisterPythonPlayerCallBack(IPlayerCallback* pCallback)
{
  CSingleLock lock (m_critSection);
  vecPlayerCallbackList.push_back(pCallback);
}

void XBPython::UnregisterPythonPlayerCallBack(IPlayerCallback* pCallback)
{
  CSingleLock lock (m_critSection);
  PlayerCallbackList::iterator it = vecPlayerCallbackList.begin();
  while (it != vecPlayerCallbackList.end())
  {
    if (*it == pCallback)
      it = vecPlayerCallbackList.erase(it);
    else
      it++;
  }
}

/**
* Check for file and print an error if needed
*/
bool XBPython::FileExist(const char* strFile)
{
  if (!strFile)
    return false;

  if (!XFILE::CFile::Exists(strFile))
  {
    CLog::Log(LOGERROR, "Python: Cannot find '%s'", strFile);
    return false;
  }
  return true;
}

/**
* Should be called before executing a script
*/
void XBPython::Initialize()
{
  CLog::Log(LOGINFO, "initializing python engine. ");
  CSingleLock lock(m_critSection);
  m_iDllScriptCounter++;
  if (!m_bInitialized)
  {
    if (dThreadId == GetCurrentThreadId())
    {
      m_hModule = dllLoadLibraryA(PYTHON_DLL);
      LibraryLoader* pDll = DllLoaderContainer::GetModule(m_hModule);
      if (!pDll || !python_load_dll(*pDll))
      {
        CLog::Log(LOGFATAL, "Python: error loading python27.dll");
        Finalize();
        return;
      }

      // first we check if all necessary files are installed
      if (!FileExist("Q:\\system\\python\\python27.zlib") ||
        !FileExist("Q:\\system\\python\\DLLs\\_socket.pyd") ||
        !FileExist("Q:\\system\\python\\DLLs\\_sqlite3.pyd") ||
        !FileExist("Q:\\system\\python\\DLLs\\_ssl.pyd") ||
        !FileExist("Q:\\system\\python\\DLLs\\bz2.pyd") ||
        !FileExist("Q:\\system\\python\\DLLs\\pyexpat.pyd") ||
        !FileExist("Q:\\system\\python\\DLLs\\select.pyd") ||
        !FileExist("Q:\\system\\python\\DLLs\\unicodedata.pyd"))
      {
        CLog::Log(LOGERROR, "Python: Missing files, unable to execute script");
        Finalize();
        return;
      }

      Py_Initialize();
      PyEval_InitThreads();

      if (g_advancedSettings.m_bPythonVerbose)
      {
        CLog::Log(LOGDEBUG, "%s - Running Python in verbose(--verbose) mode", __FUNCTION__);
        char* python_argv[1] = { (char*)"--verbose" };
        PySys_SetArgv(1, python_argv);
      }
      else
      {
        char* python_argv[1] = { (char*)"" };
        PySys_SetArgv(1, python_argv);
      }

      initxbmc(); // init xbmc modules
      initxbmcplugin(); // init plugin modules
      initxbmcgui(); // init xbmcgui modules
      // redirecting default output to debug console
      if (PyRun_SimpleString(""
        "import xbmc\n"
        "class xbmcout:\n"
        "\tdef write(self, data):\n"
        "\t\txbmc.log(data)\n"
        "\tdef close(self):\n"
        "\t\txbmc.log('.')\n"
        "\tdef flush(self):\n"
        "\t\txbmc.log('.')\n"
        "\n"
        "import sys\n"
        "sys.stdout = xbmcout()\n"
        "sys.stderr = xbmcout()\n"
        "print '-->Python Initialized<--'\n"
        "") == -1)
      {
        CLog::Log(LOGFATAL, "Python Initialize Error");
      }

      if (!(mainThreadState = PyThreadState_Get()))
        CLog::Log(LOGERROR, "Python threadstate is NULL.");
      PyEval_ReleaseLock();

      m_bInitialized = true;
      bThreadInitialize = false;
      PulseEvent(m_hEvent);
    }
    else
    {
      // only the main thread should initialize python.
      m_iDllScriptCounter--;
      bThreadInitialize = true;

      lock.Leave();
      WaitForSingleObject(m_hEvent, INFINITE);
      lock.Enter();
    }
  }
}

/**
* Should be called when a script is finished
*/
void XBPython::Finalize()
{
  CSingleLock lock(m_critSection);
  m_iDllScriptCounter--;
  if (m_iDllScriptCounter == 0 && m_bInitialized)
  {
    m_bInitialized = false;
    CLog::Log(LOGINFO, "Python, unloading python27.dll because no scripts are running anymore");
    PyEval_AcquireLock();
    PyThreadState_Swap(mainThreadState);
    Py_Finalize();
    PyEval_ReleaseLock();
    // first free all dlls loaded by python, after that python27.dll (this is done by UnloadPythonDlls
    DllLoaderContainer::UnloadPythonDlls();
    m_hModule = NULL;
    mainThreadState = NULL;
  }
}

void XBPython::FreeResources()
{
  CSingleLock lock(m_critSection);
  if (m_bInitialized)
  {
    // cleanup threads that are still running
    PyList::iterator it = vecPyList.begin();
    while (it != vecPyList.end())
    {
      lock.Leave(); //unlock here because the python thread might lock when it exits
      delete it->pyThread;
      lock.Enter();
      it = vecPyList.erase(it);
      Finalize();
    }
  }

  if (m_hEvent)
    CloseHandle(m_hEvent);
}

void XBPython::Process()
{
  CStdString strAutoExecPy;

  // initialize if init was called from another thread
  if (bThreadInitialize) Initialize();

  if (bStartup)
  {
    bStartup = false;

    // autoexec.py - system
    strAutoExecPy = "special://xbmc/scripts/autoexec.py";

    if (XFILE::CFile::Exists(strAutoExecPy))
    {
      // We need to make sure the network is up in case the start scripts require network
      g_application.getNetwork().WaitForSetup(5000);

      evalFile(strAutoExecPy);
    }
    else
      CLog::Log(LOGDEBUG, "%s - no system autoexec.py (%s) found, skipping", __FUNCTION__, CSpecialProtocol::TranslatePath(strAutoExecPy).c_str());
  }

  if (bLogin)
  {
    bLogin = false;

    // autoexec.py - profile
    strAutoExecPy = "special://profile/scripts/autoexec.py";

    if (XFILE::CFile::Exists(strAutoExecPy))
      evalFile(strAutoExecPy);
    else
      CLog::Log(LOGDEBUG, "%s - no profile autoexec.py (%s) found, skipping", __FUNCTION__, CSpecialProtocol::TranslatePath(strAutoExecPy).c_str());
  }

  CSingleLock lock(m_critSection);

  if (m_bInitialized)
  {
    PyList::iterator it = vecPyList.begin();
    while (it != vecPyList.end())
    {
      //delete scripts which are done
      if (it->bDone)
      {
        delete it->pyThread;
        it = vecPyList.erase(it);
        Finalize();
      }
      else ++it;
    }
  }
}

int XBPython::evalFile(const char *src) { return evalFile(src, 0, NULL); }
// execute script, returns -1 if script doesn't exist
int XBPython::evalFile(const char *src, const unsigned int argc, const char ** argv)
{
  CSingleExit ex(g_graphicsContext);
  CSingleLock lock(m_critSection);
  // return if file doesn't exist
  if (!XFILE::CFile::Exists(src))
  {
    CLog::Log(LOGERROR, "Python script \"%s\" does not exist", CSpecialProtocol::TranslatePath(src).c_str());
    return -1;
  }

  // check if locked
  if (g_settings.GetCurrentProfile().programsLocked() && !g_passwordManager.IsMasterLockUnlocked(true))
    return -1;

  Initialize();

  if (!m_bInitialized) return -1;

  nextid++;
  XBPyThread *pyThread = new XBPyThread(this, mainThreadState, nextid);
  if (argv != NULL)
    pyThread->setArgv(argc, argv);
  pyThread->evalFile(src);
  PyElem inf;
  inf.id = nextid;
  inf.bDone = false;
  inf.strFile = src;
  inf.pyThread = pyThread;

  vecPyList.push_back(inf);

  return nextid;
}

void XBPython::setDone(int id)
{
  CSingleLock lock(m_critSection);
  PyList::iterator it = vecPyList.begin();
  while (it != vecPyList.end())
  {
    if (it->id == id)
    {
      if (it->pyThread->isStopping())
        CLog::Log(LOGINFO, "Python script interrupted by user");
      else
        CLog::Log(LOGINFO, "Python script stopped");
      it->bDone = true;
    }
    ++it;
  }
}

void XBPython::stopScript(int id)
{
  CSingleExit ex(g_graphicsContext);
  CSingleLock lock(m_critSection);
  PyList::iterator it = vecPyList.begin();
  while (it != vecPyList.end())
  {
    if (it->id == id) {
      CLog::Log(LOGINFO, "Stopping script with id: %i", id);
      it->pyThread->stop();
      return;
    }
    ++it;
  }
}

PyThreadState *XBPython::getMainThreadState()
{
  CSingleLock lock(m_critSection);
  return mainThreadState;
}

int XBPython::ScriptsSize()
{
  CSingleLock lock(m_critSection);
  return vecPyList.size();
}

const char* XBPython::getFileName(int scriptId)
{
  const char* cFileName = NULL;

  CSingleLock lock(m_critSection);
  PyList::iterator it = vecPyList.begin();
  while (it != vecPyList.end())
  {
    if (it->id == scriptId)
      cFileName = it->strFile.c_str();
    ++it;
  }

  return cFileName;
}

int XBPython::getScriptId(const char* strFile)
{
  int iId = -1;

  CSingleLock lock(m_critSection);

  PyList::iterator it = vecPyList.begin();
  while (it != vecPyList.end())
  {
    if (!stricmp(it->strFile.c_str(), strFile))
      iId = it->id;
    ++it;
  }

  return iId;
}

bool XBPython::isRunning(int scriptId)
{
  CSingleLock lock(m_critSection);

  for(PyList::iterator it = vecPyList.begin(); it != vecPyList.end(); it++)
  {
    if (it->id == scriptId)
    {
      if(it->bDone)
        return false;
      else
        return true;
    }
  }
  return false;
}

bool XBPython::isStopping(int scriptId)
{
  bool bStopping = false;

  CSingleLock lock(m_critSection);
  PyList::iterator it = vecPyList.begin();
  while (it != vecPyList.end())
  {
    if (it->id == scriptId)
      bStopping = it->pyThread->isStopping();
    ++it;
  }

  return bStopping;
}

int XBPython::GetPythonScriptId(int scriptPosition)
{
  CSingleLock lock(m_critSection);
  return (int)vecPyList[scriptPosition].id;
}

void XBPython::PulseGlobalEvent()
{
  SetEvent(m_globalEvent);
}

void XBPython::WaitForEvent(HANDLE hEvent, DWORD timeout)
{
  // wait for either this event our our global event
  HANDLE handles[2] = { hEvent, m_globalEvent };
  WaitForMultipleObjects(2, handles, FALSE, timeout);
  ResetEvent(m_globalEvent);
}

// execute script, returns -1 if script doesn't exist
int XBPython::evalString(const char *src, const unsigned int argc, const char ** argv)
{
  CLog::Log(LOGDEBUG, "XBPython::evalString (python)");
  CSingleLock lock(m_critSection);

  Initialize();

  if (!m_bInitialized)
  {
    CLog::Log(LOGERROR, "XBPython::evalString, python not initialized (python)");
    return -1;
  }

  // Previous implementation would create a new thread for every script
  nextid++;
  XBPyThread *pyThread = new XBPyThread(this, mainThreadState, nextid);
  if (argv != NULL)
    pyThread->setArgv(argc, argv);
  pyThread->evalString(src);

  PyElem inf;
  inf.id = nextid;
  inf.bDone = false;
  inf.strFile = "<string>";
  inf.pyThread = pyThread;

  vecPyList.push_back(inf);

  return nextid;
}