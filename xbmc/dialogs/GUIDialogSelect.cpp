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

#include "dialogs/GUIDialogSelect.h"
#include "GUIWindowManager.h"
#include "FileItem.h"
#include "LocalizeStrings.h"

#define CONTROL_HEADING       1
#define CONTROL_LIST          3
#define CONTROL_NUMBEROFFILES 2
#define CONTROL_BUTTON        5

CGUIDialogSelect::CGUIDialogSelect(void)
    : CGUIDialogBoxBase(WINDOW_DIALOG_SELECT, "DialogSelect.xml")
{
  m_bButtonEnabled = false;
  m_vecListInternal = new CFileItemList;
  m_selectedItem = new CFileItem;
  m_vecList = m_vecListInternal;
}

CGUIDialogSelect::~CGUIDialogSelect(void)
{
  delete m_vecListInternal;
  delete m_selectedItem;
}

bool CGUIDialogSelect::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIDialog::OnMessage(message);
      Reset();
      return true;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_bButtonPressed = false;
      CGUIDialog::OnMessage(message);
      m_iSelected = -1;
      CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_vecList);
      g_windowManager.SendMessage(msg);

      CStdString items;
      items.Format("%i %s", m_vecList->Size(), g_localizeStrings.Get(127).c_str());
      SET_CONTROL_LABEL(CONTROL_NUMBEROFFILES, items);

      if (m_bButtonEnabled)
      {
        CGUIMessage msg2(GUI_MSG_VISIBLE, GetID(), CONTROL_BUTTON);
        g_windowManager.SendMessage(msg2);
      }
      else
      {
        CGUIMessage msg2(GUI_MSG_HIDDEN, GetID(), CONTROL_BUTTON);
        g_windowManager.SendMessage(msg2);
      }
      return true;
    }
    break;


  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (CONTROL_LIST == iControl)
      {
        int iAction = message.GetParam1();
        if (ACTION_SELECT_ITEM == iAction || ACTION_MOUSE_LEFT_CLICK == iAction)
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
          g_windowManager.SendMessage(msg);
          m_iSelected = msg.GetParam1();
          if(m_iSelected >= 0 && m_iSelected < (int)m_vecList->Size())
          {
            *m_selectedItem = *m_vecList->Get(m_iSelected);
            Close();
          }
          else
            m_iSelected = -1;
        }
      }
      if (CONTROL_BUTTON == iControl)
      {
        m_iSelected = -1;
        m_bButtonPressed = true;
        Close();
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogSelect::Reset()
{
  m_bButtonEnabled = false;
  m_vecListInternal->Clear();
  m_vecList = m_vecListInternal;
}

void CGUIDialogSelect::Add(const CStdString& strLabel)
{
  CFileItemPtr pItem(new CFileItem(strLabel));
  m_vecListInternal->Add(pItem);
}

void CGUIDialogSelect::Add(const CFileItemList& items)
{
  for (int i=0;i<items.Size();++i)
  {
    CFileItemPtr item = items[i];
    Add(item.get());
  }
}

void CGUIDialogSelect::Add(const CFileItem* pItem)
{
  CFileItemPtr item(new CFileItem(*pItem));
  m_vecListInternal->Add(item);
}

void CGUIDialogSelect::SetItems(CFileItemList* pList)
{
  m_vecList = pList;
}

int CGUIDialogSelect::GetSelectedLabel() const
{
  return m_iSelected;
}

const CFileItem& CGUIDialogSelect::GetSelectedItem()
{
  return *m_selectedItem;
}

const CStdString& CGUIDialogSelect::GetSelectedLabelText()
{
  return m_selectedItem->GetLabel();
}

void CGUIDialogSelect::EnableButton(bool bOnOff)
{
  m_bButtonEnabled = bOnOff;
}

void CGUIDialogSelect::SetButtonLabel(int iString)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_BUTTON);
  msg.SetLabel(iString);
  OnMessage(msg);
}

bool CGUIDialogSelect::IsButtonPressed()
{
  return m_bButtonPressed;
}

void CGUIDialogSelect::Sort(bool bSortOrder /*=true*/)
{
  m_vecList->Sort(SORT_METHOD_LABEL,bSortOrder?SORT_ORDER_ASC:SORT_ORDER_DESC);
}

void CGUIDialogSelect::SetSelected(int iSelected)
{
  if (iSelected < 0 || iSelected >= (int)m_vecList->Size()) return;
  m_iSelected = iSelected;
}

