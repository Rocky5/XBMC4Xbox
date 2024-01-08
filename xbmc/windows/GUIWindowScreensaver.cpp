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
#include "windows/GUIWindowScreensaver.h"
#ifdef HAS_SCREENSAVER
#include "screensavers/ScreenSaverFactory.h"
#endif
#include "Application.h"
#include "GUIPassword.h"
#include "settings/GUISettings.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "utils/SingleLock.h"

CGUIWindowScreensaver::CGUIWindowScreensaver(void)
    : CGUIWindow(WINDOW_SCREENSAVER, "")
{
#ifdef HAS_SCREENSAVER
  m_pScreenSaver = NULL;
#endif
}

CGUIWindowScreensaver::~CGUIWindowScreensaver(void)
{
}

void CGUIWindowScreensaver::Render()
{
  CSingleLock lock (m_critSection);

#ifdef HAS_SCREENSAVER
  if (m_pScreenSaver)
  {
    if (m_bInitialized)
    {
      try
      {
        //some screensavers seem to be depending on xbmc clearing the screen
        //       g_graphicsContext.Get3DDevice()->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x00010001, 1.0f, 0L );
        g_graphicsContext.ApplyStateBlock();
        m_pScreenSaver->Render();
        g_graphicsContext.ApplyStateBlock();
      }
      catch (...)
      {
        OutputDebugString("Screensaver Render - ohoh!\n");
      }
      return ;
    }
    else
    {
      try
      {
        m_pScreenSaver->Start();
        m_bInitialized = true;
      }
      catch (...)
      {
        OutputDebugString("Screensaver Start - ohoh!\n");
      }
      return ;
    }
  }
#endif
  CGUIWindow::Render();
}

bool CGUIWindowScreensaver::OnAction(const CAction &action)
{
  // We're just a screen saver, nothing to do here
  return false;
}

// called when the mouse is moved/clicked etc. etc.
bool CGUIWindowScreensaver::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  g_windowManager.PreviousWindow();
  return true;
}

bool CGUIWindowScreensaver::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CSingleLock lock (m_critSection);
#ifdef HAS_SCREENSAVER
      if (m_pScreenSaver)
      {
        OutputDebugString("ScreenSaver::Stop()\n");
        m_pScreenSaver->Stop();

        OutputDebugString("delete ScreenSaver()\n");
        delete m_pScreenSaver;

        g_graphicsContext.ApplyStateBlock();
      }
      m_pScreenSaver = NULL;
#endif
      m_bInitialized = false;

      // remove z-buffer
//      RESOLUTION res = g_graphicsContext.GetVideoResolution();
 //     g_graphicsContext.SetVideoResolution(res, FALSE);

      // enable the overlay
      g_windowManager.ShowOverlay(OVERLAY_STATE_SHOWN);
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      CSingleLock lock (m_critSection);

#ifdef HAS_SCREENSAVER
      if (m_pScreenSaver)
      {
        m_pScreenSaver->Stop();
        delete m_pScreenSaver;
        g_graphicsContext.ApplyStateBlock();
      }
      m_pScreenSaver = NULL;
      m_bInitialized = false;

      // Setup new screensaver instance
      CScreenSaverFactory factory;
      CStdString strScr;
      OutputDebugString("Load Screensaver\n");
      strScr.Format("special://xbmc/screensavers/%s", g_guiSettings.GetString("screensaver.mode").c_str());
      m_pScreenSaver = factory.LoadScreenSaver(strScr.c_str());
      if (m_pScreenSaver)
      {
        OutputDebugString("ScreenSaver::Create()\n");
		g_graphicsContext.ApplyStateBlock();
        m_pScreenSaver->Create();
      }
#endif
      // setup a z-buffer
//      RESOLUTION res = g_graphicsContext.GetVideoResolution();
//      g_graphicsContext.SetVideoResolution(res, TRUE);

      // disable the overlay
      g_windowManager.ShowOverlay(OVERLAY_STATE_HIDDEN);
      return true;
    }
  case GUI_MSG_CHECK_LOCK:
    if (!g_passwordManager.IsProfileLockUnlocked())
    {
      g_application.m_iScreenSaveLock = -1;
      return false;
    }
    g_application.m_iScreenSaveLock = 1;
    return true;
  }
  return CGUIWindow::OnMessage(message);
}

