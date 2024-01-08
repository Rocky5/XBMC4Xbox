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

#include "GUIDialogVisualisationSettings.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "settings/GUIWindowSettingsCategory.h"
#include "GUIControlGroupList.h"
#include "GUIInfoManager.h"
#include "LocalizeStrings.h"

#define CONTROL_SETTINGS_LABEL      2
#define CONTROL_NONE_AVAILABLE      3
#define CONTROL_GROUP_LIST          5
#define CONTROL_DEFAULT_BUTTON      7
#define CONTROL_DEFAULT_RADIOBUTTON 8
#define CONTROL_DEFAULT_SPIN        9
#define CONTROL_START              50
#define CONTROL_PAGE               60

CGUIDialogVisualisationSettings::CGUIDialogVisualisationSettings(void)
    : CGUIDialog(WINDOW_DIALOG_VIS_SETTINGS, "MusicOSDVisSettings.xml")
{
  m_pOriginalSpin = NULL;
  m_pOriginalRadioButton = NULL;
  m_pOriginalSettingsButton = NULL;
  m_pVisualisation = NULL;
  m_pSettings = NULL;
}

CGUIDialogVisualisationSettings::~CGUIDialogVisualisationSettings(void)
{
}

bool CGUIDialogVisualisationSettings::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      unsigned int iControl = message.GetSenderId();
      if (iControl >= CONTROL_START && iControl < CONTROL_PAGE)
        OnClick(iControl);
      return true;
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
  case GUI_MSG_VISUALISATION_UNLOADING:
    {
      FreeControls();
      m_pVisualisation = NULL;
      m_pSettings = NULL;
    }
    break;
  case GUI_MSG_VISUALISATION_LOADED:
    {
      SetVisualisation((CVisualisation *)message.GetPointer());
      SetupPage();
      SET_CONTROL_FOCUS(CONTROL_START, 0);
    }
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogVisualisationSettings::SetupPage()
{
  // cleanup first, if necessary
  FreeControls();
  m_pOriginalSpin = (CGUISpinControlEx*)GetControl(CONTROL_DEFAULT_SPIN);
  m_pOriginalRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_DEFAULT_RADIOBUTTON);
  m_pOriginalSettingsButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_BUTTON);
  if (!m_pOriginalSpin || !m_pOriginalRadioButton || !m_pOriginalSettingsButton)
    return;
  m_pOriginalSpin->SetVisible(false);
  m_pOriginalRadioButton->SetVisible(false);
  m_pOriginalSettingsButton->SetVisible(false);

  // update our settings label
  CStdString strSettings;
  strSettings.Format("%s %s", g_infoManager.GetLabel(402).c_str(), g_localizeStrings.Get(5));
  SET_CONTROL_LABEL(CONTROL_SETTINGS_LABEL, strSettings);

  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CONTROL_GROUP_LIST);
  if (!group)
    return;

  if (!m_pSettings || !m_pSettings->size())
  { // no settings available
    SET_CONTROL_VISIBLE(CONTROL_NONE_AVAILABLE);
    return;
  }
  else
  {
    SET_CONTROL_HIDDEN(CONTROL_NONE_AVAILABLE);
  }


  // run through and create our controls
  for (unsigned int i = 0; i < m_pSettings->size(); i++)
  {
    VisSetting &setting = m_pSettings->at(i);
    AddSetting(setting, group->GetWidth(), CONTROL_START + i);
  }
  UpdateSettings();
}


void CGUIDialogVisualisationSettings::UpdateSettings()
{
}

void CGUIDialogVisualisationSettings::OnClick(int iID)
{
  if (!m_pSettings || !m_pVisualisation) return;
  unsigned int settingNum = iID - CONTROL_START;
  if (settingNum >= m_pSettings->size()) return;
  VisSetting &setting = m_pSettings->at(settingNum);
  if (setting.type == VisSetting::SPIN)
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(iID);
    setting.current = pControl->GetValue();
  }
  else if (setting.type == VisSetting::CHECK)
  {
    CGUIRadioButtonControl *pControl = (CGUIRadioButtonControl *)GetControl(iID);
    setting.current = pControl->IsSelected() ? 1 : 0;
  }
  m_pVisualisation->UpdateSetting(settingNum);
  UpdateSettings();
}

void CGUIDialogVisualisationSettings::FreeControls()
{
  // just clear our group list
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CONTROL_GROUP_LIST);
  if (group)
  {
    group->FreeResources();
    group->ClearAll();
  }
}

void CGUIDialogVisualisationSettings::AddSetting(VisSetting &setting, float width, int iControlID)
{
  CGUIControl *pControl = NULL;
  if (setting.type == VisSetting::CHECK)
  {
    pControl = new CGUIRadioButtonControl(*m_pOriginalRadioButton);
    if (!pControl) return ;
    ((CGUIRadioButtonControl *)pControl)->SetLabel(setting.name);
    pControl->SetWidth(width);
    ((CGUIRadioButtonControl *)pControl)->SetSelected(setting.current == 1);
  }
  else if (setting.type == VisSetting::SPIN && setting.entry.size() > 0)
  {
    pControl = new CGUISpinControlEx(*m_pOriginalSpin);
    if (!pControl) return ;
    pControl->SetWidth(width);
    ((CGUISpinControlEx *)pControl)->SetText(setting.name);
    pControl->SetWidth(width);
    for (unsigned int i = 0; i < setting.entry.size(); i++)
      ((CGUISpinControlEx *)pControl)->AddLabel(setting.entry[i], i);
    ((CGUISpinControlEx *)pControl)->SetValue(setting.current);
  }
  if (!pControl) return;
  pControl->SetID(iControlID);
  pControl->SetVisible(true);
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CONTROL_GROUP_LIST);
  if (group)
  {
    pControl->AllocResources();
    group->AddControl(pControl);
  }
  else
    delete pControl;
}

void CGUIDialogVisualisationSettings::Render()
{
  CGUIDialog::Render();
}

void CGUIDialogVisualisationSettings::SetVisualisation(CVisualisation *pVisualisation)
{
  m_pVisualisation = pVisualisation;
  if (m_pVisualisation)
  {
    m_pVisualisation->GetSettings(&m_pSettings);
  }
}

void CGUIDialogVisualisationSettings::OnInitWindow()
{
  // set our visualisation
  CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
  g_windowManager.SendMessage(msg);
  SetVisualisation((CVisualisation *)msg.GetPointer());

  SetupPage();
  // reset the default control
  m_lastControlID = CONTROL_START;
  CGUIDialog::OnInitWindow();
}

