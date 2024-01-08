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
#ifdef HAS_SCREENSAVER
#include "screensavers/ScreenSaver.h"
#endif

#include "utils/CriticalSection.h"

#define SCREENSAVER_FADE   1
#define SCREENSAVER_BLACK  2
#define SCREENSAVER_XBS    3

class CGUIWindowScreensaver : public CGUIWindow
{
public:
  CGUIWindowScreensaver(void);
  virtual ~CGUIWindowScreensaver(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();

protected:
  virtual bool OnMouseEvent(const CPoint &point, const CMouseEvent &event);

private:
#ifdef HAS_SCREENSAVER
  CScreenSaver* m_pScreenSaver;
#endif
  bool m_bInitialized;
  CCriticalSection m_critSection;
};
