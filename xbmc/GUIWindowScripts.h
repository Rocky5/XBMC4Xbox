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

#include "windows/GUIMediaWindow.h"

class CGUIWindowScripts : public CGUIMediaWindow
{
public:
  CGUIWindowScripts(void);
  virtual ~CGUIWindowScripts(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void FrameMove();

protected:
  virtual bool Update(const CStdString &strDirectory);
  virtual bool OnPlayMedia(int iItem);
  virtual bool GetDirectory(const CStdString& strDirectory, CFileItemList& items);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  void OnInfo();

  bool m_bViewOutput;
  int m_scriptSize;
  VECSOURCES m_shares;
};
