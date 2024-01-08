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

#include "dialogs/GUIDialogBusy.h"
#include "ApplicationRenderer.h"

CGUIDialogBusy::CGUIDialogBusy(void)
: CGUIDialog(WINDOW_DIALOG_BUSY, "DialogBusy.xml")
{
  m_loadOnDemand = true;
}

CGUIDialogBusy::~CGUIDialogBusy(void)
{
}

bool CGUIDialogBusy::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBusy::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
}

void CGUIDialogBusy::Render()
{
  //only render if system is busy
  if (g_ApplicationRenderer.IsBusy() || IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
  {
    CGUIDialog::Render();
  }
}
