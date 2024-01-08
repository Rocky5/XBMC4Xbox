#ifndef XBPYTHREAD_H_
#define XBPYTHREAD_H_

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

#include "python/Include/Python.h"
#include "utils/Thread.h"

class XBPyThread : public CThread
{
public:
  XBPyThread(LPVOID pExecuter, PyThreadState* mainThreadState, int id);
  virtual ~XBPyThread();
  int evalFile(const char*);
  int evalString(const char*);
  int setArgv(const unsigned int, const char **);
  bool isDone();
  bool isStopping();
  void stop();

protected:
  PyThreadState*	threadState;
  LPVOID					pExecuter;

  char type;
  char *source;
  char **argv;
  unsigned int  argc;
  bool done;
  bool stopping;
  int id;

  virtual void OnStartup();
  virtual void Process();
  virtual void OnExit();
};

#endif // XBPYTHREAD_H_
