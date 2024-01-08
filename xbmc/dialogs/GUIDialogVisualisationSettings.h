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

#include "GUIDialog.h"
#include "visualizations/Visualisation.h"
#include "GUISpinControlEx.h"
#include "GUIRadioButtonControl.h"
#include "GUIButtonControl.h"

class CGUIDialogVisualisationSettings :
      public CGUIDialog
{
public:
  CGUIDialogVisualisationSettings(void);
  virtual ~CGUIDialogVisualisationSettings(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Render();
protected:
  virtual void OnInitWindow();
  void SetVisualisation(CVisualisation *pVisualisation);
  virtual void SetupPage();
  void UpdateSettings();
  virtual void FreeControls();
  void OnClick(int iControl);
  void AddSetting(VisSetting &setting, float width, int iControlID);

  CGUISpinControlEx *m_pOriginalSpin;
  CGUIRadioButtonControl *m_pOriginalRadioButton;
  CGUIButtonControl *m_pOriginalSettingsButton;

  CVisualisation *m_pVisualisation;
  std::vector<VisSetting> *m_pSettings;
};
