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
#include "utils/log.h"
#include "interfaces/Builtins.h"
#include "settings/GUIWindowSettingsCategory.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "input/KeyboardLayoutConfiguration.h"
#include "FileSystem/Directory.h"
#include "Util.h"
#include "GUILabelControl.h"
#include "GUICheckMarkControl.h"
#include "GUIImage.h"
#include "utils/Weather.h"
#include "music/MusicDatabase.h"
#include "ProgramDatabase.h"
#include "ViewDatabase.h"
#include "XBAudioConfig.h"
#include "XBVideoConfig.h"
#ifdef HAS_XBOX_HARDWARE
#include "utils/LED.h"
#include "utils/FanController.h"
#include "xbox/XKHDD.h"
#endif
#ifdef HAS_LCD
#include "utils/LCDFactory.h"
#endif
#include "PlayListPlayer.h"
#include "SkinInfo.h"
#include "GUIAudioManager.h"
#include "AudioContext.h"
#include "lib/libscrobbler/lastfmscrobbler.h"
#include "lib/libscrobbler/librefmscrobbler.h"
#include "GUIPassword.h"
#include "GUIInfoManager.h"
#include "dialogs/GUIDialogGamepad.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "GUIFontManager.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "programs/GUIWindowPrograms.h"
#include "storage/MediaManager.h"
#include "xbox/network.h"
#include "lib/libGoAhead/WebServer.h"
#include "GUIControlGroupList.h"
#include "XBTimeZone.h"
#include "video/VideoDatabase.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogKeyboard.h"
#include "FileSystem/Directory.h"
#include "utils/ScraperParser.h"
#include "FileItem.h"
#include "GUIToggleButtonControl.h"
#include "FileSystem/SpecialProtocol.h"
#include "FileSystem/File.h"
#include "LocalizeStrings.h"
#include "LangInfo.h"

#include "ScraperSettings.h"
#include "ScriptSettings.h"
#include "dialogs/GUIDialogPluginSettings.h"
#include "settings/AdvancedSettings.h"
#include "utils/URIUtils.h"
#include "utils/CharsetConverter.h"

using namespace std;
using namespace XFILE;

#define CONTROL_GROUP_BUTTONS           0
#define CONTROL_GROUP_SETTINGS          1
#define CONTROL_SETTINGS_LABEL          2
#define CATEGORY_GROUP_ID               3
#define SETTINGS_GROUP_ID               5
#define CONTROL_DEFAULT_BUTTON          7
#define CONTROL_DEFAULT_RADIOBUTTON     8
#define CONTROL_DEFAULT_SPIN            9
#define CONTROL_DEFAULT_CATEGORY_BUTTON 10
#define CONTROL_DEFAULT_SEPARATOR       11
#define CONTROL_DEFAULT_EDIT            12
#define CONTROL_START_BUTTONS           -40
#define CONTROL_START_CONTROL           -20

#define PREDEFINED_SCREENSAVERS          5

#define RSSEDITOR_PATH "special://home/scripts/RSS Editor/default.py"

CGUIWindowSettingsCategory::CGUIWindowSettingsCategory(void)
    : CGUIWindow(WINDOW_SETTINGS_MYPICTURES, "SettingsCategory.xml")
{
  m_pOriginalSpin = NULL;
  m_pOriginalRadioButton = NULL;
  m_pOriginalButton = NULL;
  m_pOriginalCategoryButton = NULL;
  m_pOriginalImage = NULL;
  m_pOriginalEdit = NULL;
  // set the correct ID range...
  m_idRange = 8;
  m_iScreen = 0;
  // set the network settings so that we don't reset them unnecessarily
  m_iNetworkAssignment = -1;
  m_strErrorMessage = "";
  m_strOldTrackFormat = "";
  m_strOldTrackFormatRight = "";
  m_returningFromSkinLoad = false;
  m_delayedSetting = NULL;
}

CGUIWindowSettingsCategory::~CGUIWindowSettingsCategory(void)
{
  FreeControls();
  delete m_pOriginalEdit;
}

bool CGUIWindowSettingsCategory::OnBack(int actionID)
{
  g_settings.Save();
  m_lastControlID = 0; // don't save the control as we go to a different window each time
  return CGUIWindow::OnBack(actionID);
}

bool CGUIWindowSettingsCategory::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      unsigned int iControl = message.GetSenderId();
      /*   if (iControl >= CONTROL_START_BUTTONS && iControl < CONTROL_START_BUTTONS + m_vecSections.size())
         {
          // change the setting...
          m_iSection = iControl-CONTROL_START_BUTTONS;
          CheckNetworkSettings();
          CreateSettings();
          return true;
         }*/
      for (unsigned int i = 0; i < m_vecSettings.size(); i++)
      {
        if (m_vecSettings[i]->GetID() == iControl)
          OnClick(m_vecSettings[i]);
      }
    }
    break;
  case GUI_MSG_FOCUSED:
    {
      CGUIWindow::OnMessage(message);
      int focusedControl = GetFocusedControlID();
      if (focusedControl >= CONTROL_START_BUTTONS && focusedControl < (int)(CONTROL_START_BUTTONS + m_vecSections.size()) &&
          focusedControl - CONTROL_START_BUTTONS != m_iSection)
      {
        // changing section, check for updates and cancel any delayed changes
        m_delayedSetting = NULL;
        CheckForUpdates();

        if (m_vecSections[focusedControl-CONTROL_START_BUTTONS]->m_strCategory == "masterlock")
        {
          if (!g_passwordManager.IsMasterLockUnlocked(true))
          { // unable to go to this category - focus the previous one
            SET_CONTROL_FOCUS(CONTROL_START_BUTTONS + m_iSection, 0);
            return false;
          }
        }
        m_iSection = focusedControl - CONTROL_START_BUTTONS;
        CheckNetworkSettings();

        CreateSettings();
      }
      return true;
    }
  case GUI_MSG_LOAD_SKIN:
    {
      // Do we need to reload the language file
      if (!m_strNewLanguage.IsEmpty())
      {
        g_guiSettings.SetString("locale.language", m_strNewLanguage);
        g_settings.Save();

        CStdString strLangInfoPath;
        strLangInfoPath.Format("special://xbmc/language/%s/langinfo.xml", m_strNewLanguage.c_str());
        g_langInfo.Load(strLangInfoPath);

        if (g_langInfo.ForceUnicodeFont() && !g_fontManager.IsFontSetUnicode())
        {
          CLog::Log(LOGINFO, "Language needs a ttf font, loading first ttf font available");
          CStdString strFontSet;
          if (g_fontManager.GetFirstFontSetUnicode(strFontSet))
          {
            m_strNewSkinFontSet=strFontSet;
          }
          else
            CLog::Log(LOGERROR, "No ttf font found but needed: %s", strFontSet.c_str());
        }

        g_charsetConverter.reset();

        CStdString strKeyboardLayoutConfigurationPath;
        strKeyboardLayoutConfigurationPath.Format("special://xbmc/language/%s/keyboardmap.xml", m_strNewLanguage.c_str());
        CLog::Log(LOGINFO, "load keyboard layout configuration info file: %s", strKeyboardLayoutConfigurationPath.c_str());
        g_keyboardLayoutConfiguration.Load(strKeyboardLayoutConfigurationPath);

        g_localizeStrings.Load("special://xbmc/language/", m_strNewLanguage);

        // also tell our weather to reload, as this must be localized
        g_weatherManager.Refresh();
      }

      // Do we need to reload the skin font set
      if (!m_strNewSkinFontSet.IsEmpty())
      {
        g_guiSettings.SetString("lookandfeel.font", m_strNewSkinFontSet);
        g_settings.Save();
      }

      // Reload another skin
      if (!m_strNewSkin.IsEmpty())
      {
        g_guiSettings.SetString("lookandfeel.skin", m_strNewSkin);
        g_settings.Save();
      }

      // Reload a skin theme
      if (!m_strNewSkinTheme.IsEmpty())
      {
        g_guiSettings.SetString("lookandfeel.skintheme", m_strNewSkinTheme);
        // also set the default color theme
        CStdString colorTheme(URIUtils::ReplaceExtension(m_strNewSkinTheme, ".xml"));
        if (colorTheme.Equals("Textures.xml"))
          colorTheme = "defaults.xml";
        g_guiSettings.SetString("lookandfeel.skincolors", colorTheme);
        g_settings.Save();
      }

      // Reload a skin color
      if (!m_strNewSkinColors.IsEmpty())
      {
        g_guiSettings.SetString("lookandfeel.skincolors", m_strNewSkinColors);
        g_settings.Save();
      }

      // Reload a resolution
      if (m_NewResolution != INVALID)
      {
        g_guiSettings.SetInt("videoscreen.resolution", m_NewResolution);
        //set the gui resolution, if newRes is AUTORES newRes will be set to the highest available resolution
        g_graphicsContext.SetVideoResolution(m_NewResolution, TRUE);
        //set our lookandfeelres to the resolution set in graphiccontext
        g_guiSettings.m_LookAndFeelResolution = m_NewResolution;
      }

      if (IsActive())
        m_returningFromSkinLoad = true;
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      m_delayedSetting = NULL;
      if (message.GetParam1() != WINDOW_INVALID && !m_returningFromSkinLoad)
      { // coming to this window first time (ie not returning back from some other window)
        // so we reset our section and control states
        m_iSection = 0;
        ResetControlStates();
      }
      m_returningFromSkinLoad = false;
      m_iScreen = (int)message.GetParam2() - (int)CGUIWindow::GetID();
      return CGUIWindow::OnMessage(message);
    }
    break;
  case GUI_MSG_UPDATE_ITEM:
    if (m_delayedSetting)
    {
      OnSettingChanged(m_delayedSetting);
      m_delayedSetting = NULL;
      return true;
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_delayedSetting = NULL;
      // Hardware based stuff
      // TODO: This should be done in a completely separate screen
      // to give warning to the user that it writes to the EEPROM.
      if ((g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL))
      {
        g_audioConfig.SetAC3Enabled(g_guiSettings.GetBool("audiooutput.ac3passthrough"));
        g_audioConfig.SetDTSEnabled(g_guiSettings.GetBool("audiooutput.dtspassthrough"));
        g_audioConfig.SetAACEnabled(g_guiSettings.GetBool("audiooutput.aacpassthrough"));
        g_audioConfig.SetMP1Enabled(g_guiSettings.GetBool("audiooutput.mp1passthrough"));
        g_audioConfig.SetMP2Enabled(g_guiSettings.GetBool("audiooutput.mp2passthrough"));
        g_audioConfig.SetMP3Enabled(g_guiSettings.GetBool("audiooutput.mp3passthrough"));
        if (g_audioConfig.NeedsSave())
        { // should we perhaps show a dialog here?
          g_audioConfig.Save();
        }
      }
      switch(g_guiSettings.GetInt("videooutput.aspect"))
      {
      case VIDEO_NORMAL:
        g_videoConfig.SetNormal();
        break;
      case VIDEO_LETTERBOX:
        g_videoConfig.SetLetterbox(true);
        break;
      case VIDEO_WIDESCREEN:
        g_videoConfig.SetWidescreen(true);
        break;
      }
      g_videoConfig.Set480p(g_guiSettings.GetBool("videooutput.hd480p"));
      g_videoConfig.Set720p(g_guiSettings.GetBool("videooutput.hd720p"));
      g_videoConfig.Set1080i(g_guiSettings.GetBool("videooutput.hd1080i"));

      if (g_videoConfig.NeedsSave())
        g_videoConfig.Save();

      if (g_timezone.GetTimeZoneIndex() != g_guiSettings.GetInt("locale.timezone"))
        g_timezone.SetTimeZoneIndex(g_guiSettings.GetInt("locale.timezone"));

      if (g_timezone.GetDST() != g_guiSettings.GetBool("locale.usedst"))
        g_timezone.SetDST(g_guiSettings.GetBool("locale.usedst"));

      CheckForUpdates();
      CheckNetworkSettings();
      CGUIWindow::OnMessage(message);
      FreeControls();
      return true;
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSettingsCategory::SetupControls()
{
  // cleanup first, if necessary
  FreeControls();
  m_pOriginalSpin = (CGUISpinControlEx*)GetControl(CONTROL_DEFAULT_SPIN);
  m_pOriginalRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_DEFAULT_RADIOBUTTON);
  m_pOriginalCategoryButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_CATEGORY_BUTTON);
  m_pOriginalButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_BUTTON);
  m_pOriginalImage = (CGUIImage *)GetControl(CONTROL_DEFAULT_SEPARATOR);
  if (!m_pOriginalCategoryButton || !m_pOriginalSpin || !m_pOriginalRadioButton || !m_pOriginalButton)
    return ;
  m_pOriginalEdit = (CGUIEditControl *)GetControl(CONTROL_DEFAULT_EDIT);
  if (!m_pOriginalEdit || m_pOriginalEdit->GetControlType() != CGUIControl::GUICONTROL_EDIT)
  {
    delete m_pOriginalEdit;
    m_pOriginalEdit = new CGUIEditControl(*m_pOriginalButton);
  }
  m_pOriginalSpin->SetVisible(false);
  m_pOriginalRadioButton->SetVisible(false);
  m_pOriginalButton->SetVisible(false);
  m_pOriginalCategoryButton->SetVisible(false);
  m_pOriginalEdit->SetVisible(false);
  if (m_pOriginalImage) m_pOriginalImage->SetVisible(false);
  // setup our control groups...
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CATEGORY_GROUP_ID);
  if (!group)
    return;
  // get a list of different sections
  CSettingsGroup *pSettingsGroup = g_guiSettings.GetGroup(m_iScreen);
  if (!pSettingsGroup) return ;
  // update the screen string
  SET_CONTROL_LABEL(CONTROL_SETTINGS_LABEL, pSettingsGroup->GetLabelID());
  // get the categories we need
  pSettingsGroup->GetCategories(m_vecSections);
  // run through and create our buttons...
  int j=0;
  for (unsigned int i = 0; i < m_vecSections.size(); i++)
  {
    if (m_vecSections[i]->m_labelID == 12360 && !g_settings.IsMasterUser())
      continue;
    CGUIButtonControl *pButton = NULL;
    if (m_pOriginalCategoryButton->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON)
      pButton = new CGUIToggleButtonControl(*(CGUIToggleButtonControl *)m_pOriginalCategoryButton);
    else
      pButton = new CGUIButtonControl(*m_pOriginalCategoryButton);
    pButton->SetLabel(g_localizeStrings.Get(m_vecSections[i]->m_labelID));
    pButton->SetID(CONTROL_START_BUTTONS + j);
    pButton->SetVisible(true);
    pButton->AllocResources();
    group->AddControl(pButton);
    j++;
  }
  if (m_iSection < 0 || m_iSection >= (int)m_vecSections.size())
    m_iSection = 0;
  CreateSettings();
  // set focus correctly
  m_defaultControl = CONTROL_START_BUTTONS;
}

void CGUIWindowSettingsCategory::CreateSettings()
{
  FreeSettingsControls();

  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
  if (!group)
    return;
  vecSettings settings;
  g_guiSettings.GetSettingsGroup(m_vecSections[m_iSection]->m_strCategory, settings);
  int iControlID = CONTROL_START_CONTROL;
  for (unsigned int i = 0; i < settings.size(); i++)
  {
    CSetting *pSetting = settings[i];
    AddSetting(pSetting, group->GetWidth(), iControlID);
    CStdString strSetting = pSetting->GetSetting();
    if (strSetting.Equals("myprograms.ntscmode"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i++)
      {
        pControl->AddLabel(g_localizeStrings.Get(16106 + i), i);
      }
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("musicplayer.visualisation"))
    {
      FillInVisualisations(pSetting, GetSetting(pSetting->GetSetting())->GetID());
    }
    else if (strSetting.Equals("musiclibrary.scraper"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      FillInScrapers(pControl, g_guiSettings.GetString("musiclibrary.scraper"), "music");
    }
    else if (strSetting.Equals("scrapers.moviedefault"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      FillInScrapers(pControl, g_guiSettings.GetString("scrapers.moviedefault"), "movies");
    }
    else if (strSetting.Equals("scrapers.tvshowdefault"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      FillInScrapers(pControl, g_guiSettings.GetString("scrapers.tvshowdefault"), "tvshows");
    }
    else if (strSetting.Equals("scrapers.musicvideodefault"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      FillInScrapers(pControl, g_guiSettings.GetString("scrapers.musicvideodefault"), "musicvideos");
    }
    else if (strSetting.Equals("karaoke.port0voicemask"))
    {
      FillInVoiceMasks(0, pSetting);
    }
    else if (strSetting.Equals("karaoke.port1voicemask"))
    {
      FillInVoiceMasks(1, pSetting);
    }
    else if (strSetting.Equals("karaoke.port2voicemask"))
    {
      FillInVoiceMasks(2, pSetting);
    }
    else if (strSetting.Equals("karaoke.port3voicemask"))
    {
      FillInVoiceMasks(3, pSetting);
    }
    else if (strSetting.Equals("audiooutput.mode"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(338), AUDIO_ANALOG);
      if (g_audioConfig.HasDigitalOutput())
        pControl->AddLabel(g_localizeStrings.Get(339), AUDIO_DIGITAL);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videooutput.aspect"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(21375), VIDEO_NORMAL);
      pControl->AddLabel(g_localizeStrings.Get(21376), VIDEO_LETTERBOX);
      pControl->AddLabel(g_localizeStrings.Get(21377), VIDEO_WIDESCREEN);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("audiocds.encoder"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel("Lame", CDDARIP_ENCODER_LAME);
      pControl->AddLabel("Vorbis", CDDARIP_ENCODER_VORBIS);
      pControl->AddLabel("Wav", CDDARIP_ENCODER_WAV);
      pControl->AddLabel("Flac", CDDARIP_ENCODER_FLAC);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("audiocds.quality"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(604), CDDARIP_QUALITY_CBR);
      pControl->AddLabel(g_localizeStrings.Get(601), CDDARIP_QUALITY_MEDIUM);
      pControl->AddLabel(g_localizeStrings.Get(602), CDDARIP_QUALITY_STANDARD);
      pControl->AddLabel(g_localizeStrings.Get(603), CDDARIP_QUALITY_EXTREME);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("lcd.type"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(351), LCD_TYPE_NONE);
      pControl->AddLabel("LCD - HD44780", LCD_TYPE_LCD_HD44780);
      pControl->AddLabel("LCD - KS0073", LCD_TYPE_LCD_KS0073);
      pControl->AddLabel("VFD", LCD_TYPE_VFD);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("lcd.modchip"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel("SmartXX", MODCHIP_SMARTXX);
      pControl->AddLabel("Xenium", MODCHIP_XENIUM);
      pControl->AddLabel("Xecuter3", MODCHIP_XECUTER3);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("harddisk.aamlevel"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(21388), AAM_QUIET);
      pControl->AddLabel(g_localizeStrings.Get(21387), AAM_FAST);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("harddisk.apmlevel"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(21391), APM_HIPOWER);
      pControl->AddLabel(g_localizeStrings.Get(21392), APM_LOPOWER);
      pControl->AddLabel(g_localizeStrings.Get(21393), APM_HIPOWER_STANDBY);
      pControl->AddLabel(g_localizeStrings.Get(21394), APM_LOPOWER_STANDBY);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("system.targettemperature"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i++)
      {
        CTemperature temp=CTemperature::CreateFromCelsius(i);
        pControl->AddLabel(temp.ToString(), i);
      }
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("system.fanspeed") || strSetting.Equals("system.minfanspeed"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      CStdString strPercentMask = g_localizeStrings.Get(14047);
      for (int i=pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i += pSettingInt->m_iStep)
      {
        CStdString strLabel;
        strLabel.Format(strPercentMask.c_str(), i*2);
        pControl->AddLabel(strLabel, i);
      }
      pControl->SetValue(int(pSettingInt->GetData()));
    }
    else if (strSetting.Equals("harddisk.remoteplayspindown"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(474), SPIN_DOWN_NONE);
      pControl->AddLabel(g_localizeStrings.Get(475), SPIN_DOWN_MUSIC);
      pControl->AddLabel(g_localizeStrings.Get(13002), SPIN_DOWN_VIDEO);
      pControl->AddLabel(g_localizeStrings.Get(476), SPIN_DOWN_BOTH);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("services.webserverusername"))
    {
      // get password from the webserver if it's running (and update our settings)
      if (g_application.m_pWebServer)
      {
        ((CSettingString *)GetSetting(strSetting)->GetSetting())->SetData(g_application.m_pWebServer->GetUserName());
        g_settings.Save();
      }
    }
    else if (strSetting.Equals("services.webserverpassword"))
    {
      // get password from the webserver if it's running (and update our settings)
      if (g_application.m_pWebServer)
      {
        ((CSettingString *)GetSetting(strSetting)->GetSetting())->SetData(g_application.m_pWebServer->GetPassword());
        g_settings.Save();
      }
    }
    else if (strSetting.Equals("services.webserverport"))
    {
      CBaseSettingControl *control = GetSetting(pSetting->GetSetting());
      control->SetDelayed();
    }
    else if (strSetting.Equals("services.esport"))
    {
      CBaseSettingControl *control = GetSetting(pSetting->GetSetting());
      control->SetDelayed();
    }
    else if (strSetting.Equals("network.assignment"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(718), NETWORK_DASH);
      pControl->AddLabel(g_localizeStrings.Get(716), NETWORK_DHCP);
      pControl->AddLabel(g_localizeStrings.Get(717), NETWORK_STATIC);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("network.httpproxyport"))
    {
      CBaseSettingControl *control = GetSetting(pSetting->GetSetting());
      control->SetDelayed();
    }
    else if (strSetting.Equals("subtitles.style"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(738), FONT_STYLE_NORMAL);
      pControl->AddLabel(g_localizeStrings.Get(739), FONT_STYLE_BOLD);
      pControl->AddLabel(g_localizeStrings.Get(740), FONT_STYLE_ITALICS);
      pControl->AddLabel(g_localizeStrings.Get(741), FONT_STYLE_BOLD_ITALICS);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("subtitles.color"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = SUBTITLE_COLOR_START; i <= SUBTITLE_COLOR_END; i++)
        pControl->AddLabel(g_localizeStrings.Get(760 + i), i);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("subtitles.height"))
    {
      FillInSubtitleHeights(pSetting);
    }
    else if (strSetting.Equals("subtitles.font"))
    {
      FillInSubtitleFonts(pSetting);
    }
    else if (strSetting.Equals("subtitles.charset") || strSetting.Equals("locale.charset"))
    {
      FillInCharSets(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.font"))
    {
      FillInSkinFonts(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.skin"))
    {
      FillInSkins(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.soundskin"))
    {
      FillInSoundSkins(pSetting);
    }
    else if (strSetting.Equals("locale.language"))
    {
      FillInLanguages(pSetting);
    }
    else if (strSetting.Equals("locale.timezone"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i=0; i < g_timezone.GetNumberOfTimeZones(); i++)
        pControl->AddLabel(g_timezone.GetTimeZoneString(i), i);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videoscreen.resolution"))
    {
      FillInResolutions(pSetting, false);
    }
    else if (strSetting.Equals("lookandfeel.skintheme"))
    {
      FillInSkinThemes(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.skincolors"))
    {
      FillInSkinColors(pSetting);
    }
    else if (strSetting.Equals("screensaver.mode"))
    {
      FillInScreenSavers(pSetting);
    }
    else if (strSetting.Equals("videoplayer.displayresolution") || strSetting.Equals("pictures.displayresolution"))
    {
      FillInResolutions(pSetting, true);
    }
    else if (strSetting.Equals("videoplayer.framerateconversions"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(231), FRAME_RATE_LEAVE_AS_IS); // "None"
      pControl->AddLabel(g_videoConfig.HasPAL() ? g_localizeStrings.Get(12380) : g_localizeStrings.Get(12381), FRAME_RATE_CONVERT); // "Play PAL videos at NTSC rates" or "Play NTSC videos at PAL rates"
      if (g_videoConfig.HasPAL() && g_videoConfig.HasPAL60())
        pControl->AddLabel(g_localizeStrings.Get(12382), FRAME_RATE_USE_PAL60); // "Play NTSC videos in PAL60"
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videoplayer.skiploopfilter"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(14101), VS_SKIPLOOP_DEFAULT);
      pControl->AddLabel(g_localizeStrings.Get(14102), VS_SKIPLOOP_NONREF);
      pControl->AddLabel(g_localizeStrings.Get(14103), VS_SKIPLOOP_BIDIR);
      pControl->AddLabel(g_localizeStrings.Get(14104), VS_SKIPLOOP_NONKEY);
      pControl->AddLabel(g_localizeStrings.Get(14105), VS_SKIPLOOP_ALL);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videolibrary.flattentvshows"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(20420), 0); // Never
      pControl->AddLabel(g_localizeStrings.Get(20421), 1); // One Season
      pControl->AddLabel(g_localizeStrings.Get(20422), 2); // Always
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("system.ledcolour"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13340), LED_COLOUR_NO_CHANGE);
      pControl->AddLabel(g_localizeStrings.Get(13341), LED_COLOUR_GREEN);
      pControl->AddLabel(g_localizeStrings.Get(13342), LED_COLOUR_ORANGE);
      pControl->AddLabel(g_localizeStrings.Get(13343), LED_COLOUR_RED);
      pControl->AddLabel(g_localizeStrings.Get(13344), LED_COLOUR_CYCLE);
      pControl->AddLabel(g_localizeStrings.Get(351), LED_COLOUR_OFF);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("system.leddisableonplayback") || strSetting.Equals("lcd.disableonplayback"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(106), LED_PLAYBACK_OFF);     // No
      pControl->AddLabel(g_localizeStrings.Get(13002), LED_PLAYBACK_VIDEO);   // Video Only
      pControl->AddLabel(g_localizeStrings.Get(475), LED_PLAYBACK_MUSIC);    // Music Only
      pControl->AddLabel(g_localizeStrings.Get(476), LED_PLAYBACK_VIDEO_MUSIC); // Video & Music
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videoplayer.rendermethod"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13355), RENDER_LQ_RGB_SHADER);
      pControl->AddLabel(g_localizeStrings.Get(13356), RENDER_OVERLAYS);
      pControl->AddLabel(g_localizeStrings.Get(13357), RENDER_HQ_RGB_SHADER);
      pControl->AddLabel(g_localizeStrings.Get(21397), RENDER_HQ_RGB_SHADERV2);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("musicplayer.replaygaintype"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(351), REPLAY_GAIN_NONE);
      pControl->AddLabel(g_localizeStrings.Get(639), REPLAY_GAIN_TRACK);
      pControl->AddLabel(g_localizeStrings.Get(640), REPLAY_GAIN_ALBUM);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("musicplayer.defaultplayer"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(22027), PLAYER_PAPLAYER);
      pControl->AddLabel(g_localizeStrings.Get(22029), PLAYER_MPLAYER);
      pControl->AddLabel(g_localizeStrings.Get(22028), PLAYER_DVDPLAYER);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("lookandfeel.startupwindow"))
    {
      FillInStartupWindow(pSetting);
    }
    else if (strSetting.Equals("services.ftpserveruser"))
    {
      FillInFTPServerUser(pSetting);
    }
    else if (strSetting.Equals("autodetect.nickname"))
    {
#ifdef HAS_XBOX_HARDWARE
      CStdString strXboxNickNameOut;
      if (CUtil::GetXBOXNickName(strXboxNickNameOut))
        g_guiSettings.SetString("autodetect.nickname", strXboxNickNameOut.c_str());
#endif
    }
    else if (strSetting.Equals("dvds.externaldvdplayer"))
    {
      CSettingString *pSettingString = (CSettingString *)pSetting;
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      if (pSettingString->GetData().IsEmpty())
        pControl->SetLabel2(g_localizeStrings.Get(20009));
    }
    else if (strSetting.Equals("locale.country"))
    {
      FillInRegions(pSetting);
    }
    else if (strSetting.Equals("videoplayer.resumeautomatically"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(106), RESUME_NO);
      pControl->AddLabel(g_localizeStrings.Get(107), RESUME_YES);
      pControl->AddLabel(g_localizeStrings.Get(12020), RESUME_ASK);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videoplayer.defaultplayer"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(22029), PLAYER_MPLAYER);
      pControl->AddLabel(g_localizeStrings.Get(22028), PLAYER_DVDPLAYER);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("weather.plugin"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
      FillInWeatherPlugins(pControl, g_guiSettings.GetString("weather.plugin"));
    }
  }
  // update our settings (turns controls on/off as appropriate)
  UpdateSettings();
}

void CGUIWindowSettingsCategory::UpdateSettings()
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    CBaseSettingControl *pSettingControl = m_vecSettings[i];
    pSettingControl->Update();
    CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
    if (strSetting.Equals("filelists.allowfiledeletion"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(!g_settings.GetCurrentProfile().filesLocked() || g_passwordManager.bMasterUser);
    }
    else if (strSetting.Equals("filelists.showaddsourcebuttons"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_settings.GetCurrentProfile().canWriteSources() || g_passwordManager.bMasterUser);
    }
    else if (strSetting.Equals("myprograms.ntscmode"))
    { // set visibility based on our other setting...
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("myprograms.gameautoregion"));
    }
    else if (strSetting.Equals("masterlock.startuplock") || strSetting.Equals("masterlock.enableshutdown"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE);
    }
    else if (!strSetting.Equals("services.esenabled")
             && strSetting.Left(11).Equals("services.es"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("services.esenabled"));
    }
    else if (strSetting.Equals("audiocds.quality"))
    { // only visible if we are not ripping as WAV or FLAC
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("audiocds.encoder") != CDDARIP_ENCODER_WAV &&
                                         g_guiSettings.GetInt("audiocds.encoder") != CDDARIP_ENCODER_FLAC);
    }
    else if (strSetting.Equals("audiocds.bitrate"))
    { // only visible if we are ripping to CBR
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("audiocds.encoder") != CDDARIP_ENCODER_WAV && 
                                         g_guiSettings.GetInt("audiocds.encoder") != CDDARIP_ENCODER_FLAC &&
                                         g_guiSettings.GetInt("audiocds.quality") == CDDARIP_QUALITY_CBR);
    }
    else if (strSetting.Equals("audiocds.compressionlevel"))
    { // only visible if we are doing FLAC ripping
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("audiocds.encoder") == CDDARIP_ENCODER_FLAC);
    }
    else if (
             strSetting.Equals("musicplayer.outputtoallspeakers") ||
             strSetting.Equals("audiooutput.ac3passthrough") ||
             strSetting.Equals("audiooutput.dtspassthrough") ||
             strSetting.Equals("audiooutput.aacpassthrough") ||
             strSetting.Equals("audiooutput.mp1passthrough") ||
             strSetting.Equals("audiooutput.mp2passthrough") ||
             strSetting.Equals("audiooutput.mp3passthrough"))
    { // only visible if we are in digital mode
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL);
    }
    else if (strSetting.Equals("videooutput.hd480p") || strSetting.Equals("videooutput.hd720p") || strSetting.Equals("videooutput.hd1080i"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      // disable if we do not have the HDTV pack and are not NTSC
#ifdef HAS_XBOX_HARDWARE
      if (pControl) pControl->SetEnabled(g_videoConfig.HasNTSC() && g_videoConfig.HasHDPack());
#endif
    }
    else if (strSetting.Equals("musicplayer.crossfadealbumtracks"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("musicplayer.crossfade") > 0);
    }
    else if (strSetting.Left(12).Equals("karaoke.port") || strSetting.Equals("karaoke.volume"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("karaoke.voiceenabled"));
    }
    else if (strSetting.Equals("system.fanspeed"))
    { // only visible if we have fancontrolspeed enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("system.fanspeedcontrol"));
    }
    else if (strSetting.Equals("system.targettemperature") || strSetting.Equals("system.minfanspeed"))
    { // only visible if we have autotemperature enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("system.autotemperature"));
    }
    else if (strSetting.Equals("harddisk.remoteplayspindowndelay"))
    { // only visible if we have spin down enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("harddisk.remoteplayspindown") != SPIN_DOWN_NONE);
    }
    else if (strSetting.Equals("harddisk.remoteplayspindownminduration"))
    { // only visible if we have spin down enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("harddisk.remoteplayspindown") != SPIN_DOWN_NONE);
    }
    else if (strSetting.Equals("services.ftpserveruser") || strSetting.Equals("services.ftpserverpassword") || strSetting.Equals("services.ftpautofatx"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("services.ftpserver"));
    }
    else if (strSetting.Equals("services.webserverusername"))
    {
      CGUIEditControl *pControl = (CGUIEditControl *)GetControl(pSettingControl->GetID());
      if (pControl)
        pControl->SetEnabled(g_guiSettings.GetBool("services.webserver"));
    }
    else if (strSetting.Equals("services.webserverpassword"))
    { // Fill in a blank pass if we don't have it
      CGUIEditControl *pControl = (CGUIEditControl *)GetControl(pSettingControl->GetID());
      if (((CSettingString *)pSettingControl->GetSetting())->GetData().size() == 0 && pControl)
      {
        pControl->SetLabel2(g_localizeStrings.Get(734));
        pControl->SetEnabled(g_guiSettings.GetBool("services.webserver"));
      }
    }
    else if (strSetting.Equals("network.ipaddress") || strSetting.Equals("network.subnet") || strSetting.Equals("network.gateway") || strSetting.Equals("network.dns") || strSetting.Equals("network.dns2"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        if (g_guiSettings.GetInt("network.assignment") != NETWORK_STATIC)
        {
          //We are in non Static Mode! Setting the Received IP Information
          if(strSetting.Equals("network.ipaddress"))
            pControl->SetLabel2(g_application.getNetwork().m_networkinfo.ip);
          else if(strSetting.Equals("network.subnet"))
            pControl->SetLabel2(g_application.getNetwork().m_networkinfo.subnet);
          else if(strSetting.Equals("network.gateway"))
            pControl->SetLabel2(g_application.getNetwork().m_networkinfo.gateway);
          else if(strSetting.Equals("network.dns"))
            pControl->SetLabel2(g_application.getNetwork().m_networkinfo.DNS1);
          else if(strSetting.Equals("network.dns2"))
            pControl->SetLabel2(g_application.getNetwork().m_networkinfo.DNS2);
        }
        pControl->SetEnabled(g_guiSettings.GetInt("network.assignment") == NETWORK_STATIC);
      }
    }
    else if (strSetting.Equals("network.httpproxyserver")   || strSetting.Equals("network.httpproxyport") ||
             strSetting.Equals("network.httpproxyusername") || strSetting.Equals("network.httpproxypassword"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("network.usehttpproxy"));
    }
    else if (strSetting.Equals("scrobbler.lastfmusername") || strSetting.Equals("scrobbler.lastfmpass"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl)
        pControl->SetEnabled(g_guiSettings.GetBool("scrobbler.lastfmsubmit") | g_guiSettings.GetBool("scrobbler.lastfmsubmitradio"));
    }
    else if (strSetting.Equals("scrobbler.librefmusername") || strSetting.Equals("scrobbler.librefmpass"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("scrobbler.librefmsubmit"));
    }
    else if (strSetting.Equals("postprocessing.verticaldeblocklevel"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("postprocessing.verticaldeblocking") &&
                           g_guiSettings.GetBool("postprocessing.enable") &&
                           !g_guiSettings.GetBool("postprocessing.auto"));
    }
    else if (strSetting.Equals("postprocessing.horizontaldeblocklevel"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("postprocessing.horizontaldeblocking") &&
                           g_guiSettings.GetBool("postprocessing.enable") &&
                           !g_guiSettings.GetBool("postprocessing.auto"));
    }
    else if (strSetting.Equals("postprocessing.verticaldeblocking") || strSetting.Equals("postprocessing.horizontaldeblocking") || strSetting.Equals("postprocessing.autobrightnesscontrastlevels") || strSetting.Equals("postprocessing.dering"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("postprocessing.enable") &&
                           !g_guiSettings.GetBool("postprocessing.auto"));
    }
    else if (strSetting.Equals("postprocessing.auto"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("postprocessing.enable"));
    }
    else if (strSetting.Equals("VideoPlayer.InvertFieldSync"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("VideoPlayer.FieldSync"));
    }
    else if (strSetting.Equals("subtitles.color") || strSetting.Equals("subtitles.style") || strSetting.Equals("subtitles.charset"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(CUtil::IsUsingTTFSubtitles());
    }
    else if (strSetting.Equals("locale.charset"))
    { // TODO: Determine whether we are using a TTF font or not.
      //   CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      //   if (pControl) pControl->SetEnabled(g_guiSettings.GetString("lookandfeel.font").Right(4) == ".ttf");
    }
    else if (strSetting.Equals("screensaver.dimlevel"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("screensaver.mode") == "Dim");
    }
    else if (strSetting.Equals("screensaver.slideshowpath"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("screensaver.mode") == "SlideShow");
    }
    else if (strSetting.Equals("screensaver.slideshowshuffle"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("screensaver.mode") == "SlideShow" ||
                           g_guiSettings.GetString("screensaver.mode") == "Fanart Slideshow");
    }
    else if (strSetting.Equals("screensaver.preview")           ||
             strSetting.Equals("screensaver.usedimonpause")     ||
             strSetting.Equals("screensaver.usemusicvisinstead"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("screensaver.mode") != "None");
      if (strSetting.Equals("screensaver.usedimonpause") && g_guiSettings.GetString("screensaver.mode").Equals("Dim"))
        pControl->SetEnabled(false);
    }
    else if (strSetting.Left(16).Equals("weather.areacode"))
    {
      CSettingString *pSetting = (CSettingString *)GetSetting(strSetting)->GetSetting();
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetLabel2(g_weatherManager.GetAreaCity(pSetting->GetData()));
    }
    else if (strSetting.Equals("weather.plugin"))
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
      if (pControl->GetCurrentLabel().Equals(g_localizeStrings.Get(13611)))
        g_guiSettings.SetString("weather.plugin", "");
      else
        g_guiSettings.SetString("weather.plugin", pControl->GetCurrentLabel());
    }
    else if (strSetting.Equals("system.leddisableonplayback"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      // LED_COLOUR_NO_CHANGE: we can't disable the LED on playback, 
      //                       we have no previos reference LED COLOUR, to set the LED colour back
      pControl->SetEnabled(g_guiSettings.GetInt("system.ledcolour") != LED_COLOUR_NO_CHANGE && g_guiSettings.GetInt("system.ledcolour") != LED_COLOUR_OFF);
    }
    else if (strSetting.Equals("musicfiles.trackformat"))
    {
      if (m_strOldTrackFormat != g_guiSettings.GetString("musicfiles.trackformat"))
      {
        CUtil::DeleteMusicDatabaseDirectoryCache();
        m_strOldTrackFormat = g_guiSettings.GetString("musicfiles.trackformat");
      }
    }
    else if (strSetting.Equals("musicfiles.trackformatright"))
    {
      if (m_strOldTrackFormatRight != g_guiSettings.GetString("musicfiles.trackformatright"))
      {
        CUtil::DeleteMusicDatabaseDirectoryCache();
        m_strOldTrackFormatRight = g_guiSettings.GetString("musicfiles.trackformatright");
      }
    }
#ifdef HAS_TIME_SERVER
    else if (strSetting.Equals("locale.timeserveraddress"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("locale.timeserver"));
    }
    else if (strSetting.Equals("locale.time") || strSetting.Equals("locale.date"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(!g_guiSettings.GetBool("locale.timeserver"));
      SYSTEMTIME curTime;
      GetLocalTime(&curTime);
      CStdString time;
      if (strSetting.Equals("locale.time"))
        time = g_infoManager.GetTime();
      else
        time = g_infoManager.GetDate();
      CSettingString *pSettingString = (CSettingString*)pSettingControl->GetSetting();
      pSettingString->SetData(time);
      pSettingControl->Update();
    }
#endif
    else if (strSetting.Equals("autodetect.nickname") || strSetting.Equals("autodetect.senduserpw"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("autodetect.onoff") && (g_settings.GetCurrentProfileIndex() == 0));
    }
    else if ( strSetting.Equals("autodetect.popupinfo"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("autodetect.onoff"));
    }
    else if (strSetting.Equals("dvds.externaldvdplayer"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("dvds.useexternaldvdplayer"));
    }
    else if (strSetting.Equals("myprograms.trainerpath") || strSetting.Equals("audiocds.recordingpath") || strSetting.Equals("debug.screenshotpath"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl && g_guiSettings.GetString(strSetting, false).IsEmpty())
        pControl->SetLabel2("");
    }
    else if (strSetting.Equals("lookandfeel.rssedit"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(XFILE::CFile::Exists(RSSEDITOR_PATH) && g_guiSettings.GetBool("lookandfeel.enablerssfeeds"));
    }
    else if (strSetting.Equals("myprograms.dashboard"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("myprograms.usedashpath"));
    }
    else if (strSetting.Equals("lcd.enableonpaused"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("lcd.disableonplayback") != LED_PLAYBACK_OFF && g_guiSettings.GetInt("lcd.type") != LCD_TYPE_NONE);
    }
    else if (strSetting.Equals("musiclibrary.scrapersettings"))
    {
      CScraperParser parser;
      bool enabled=false;
      if (parser.Load("special://xbmc/system/scrapers/music/"+g_guiSettings.GetString("musiclibrary.scraper")))
        enabled = parser.HasFunction("GetSettings");

      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(enabled);
    }
    else if (strSetting.Equals("system.ledenableonpaused"))
    {
      // LED_COLOUR_NO_CHANGE: we can't enable LED on paused, 
      //                       we have no previos reference LED COLOUR, to set the LED colour back
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("system.leddisableonplayback") != LED_PLAYBACK_OFF && g_guiSettings.GetInt("system.ledcolour") != LED_COLOUR_OFF && g_guiSettings.GetInt("system.ledcolour") != LED_COLOUR_NO_CHANGE);
    }
    else if (strSetting.Equals("lcd.modchip") || strSetting.Equals("lcd.backlight") || strSetting.Equals("lcd.disableonplayback"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("lcd.type") != LCD_TYPE_NONE);
    }
    else if (strSetting.Equals("lcd.contrast"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      // X3 can't control the contrast via software graying out!
      if(g_guiSettings.GetInt("lcd.type") != LCD_TYPE_NONE)
      {
        if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("lcd.modchip") != MODCHIP_XECUTER3);
      }
      else 
      { 
        if (pControl) pControl->SetEnabled(false); 
      }
    }
    else if (strSetting.Equals("weather.pluginsettings"))
    {
      // Create our base path
      CStdString basepath = "special://home/plugins/weather/" + g_guiSettings.GetString("weather.plugin");
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(!g_guiSettings.GetString("weather.plugin").IsEmpty() && CScriptSettings::SettingsExist(basepath));
    }
    else if (!strSetting.Equals("musiclibrary.enabled")
      && strSetting.Left(13).Equals("musiclibrary."))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("musiclibrary.enabled"));
    }
    else if (!strSetting.Equals("videolibrary.enabled")
      && strSetting.Left(13).Equals("videolibrary."))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("videolibrary.enabled"));
    }
  }
}

void CGUIWindowSettingsCategory::UpdateRealTimeSettings()
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    CBaseSettingControl *pSettingControl = m_vecSettings[i];
    CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
    if (strSetting.Equals("locale.time") || strSetting.Equals("locale.date"))
    {
#ifdef HAS_TIME_SERVER
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(!g_guiSettings.GetBool("locale.timeserver"));
#endif
      SYSTEMTIME curTime;
      GetLocalTime(&curTime);
      CStdString time;
      if (strSetting.Equals("locale.time"))
        time = g_infoManager.GetTime();
      else
        time = g_infoManager.GetDate();
      CSettingString *pSettingString = (CSettingString*)pSettingControl->GetSetting();
      pSettingString->SetData(time);
      pSettingControl->Update();
    }
  }
}

void CGUIWindowSettingsCategory::OnClick(CBaseSettingControl *pSettingControl)
{
  CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
  if (strSetting.Left(16).Equals("weather.areacode"))
  {
    CStdString strSearch;
    if (CGUIDialogKeyboard::ShowAndGetInput(strSearch, g_localizeStrings.Get(14024), false))
    {
      strSearch.Replace(" ", "+");
      CStdString strResult = ((CSettingString *)pSettingControl->GetSetting())->GetData();
      if (g_weatherManager.GetSearchResults(strSearch, strResult))
        ((CSettingString *)pSettingControl->GetSetting())->SetData(strResult);
      g_weatherManager.Refresh();
    }
  }
  else if (strSetting.Equals("weather.plugin"))
  {
    g_weatherManager.Refresh();
  }
  else if (strSetting.Equals("weather.pluginsettings"))
  {
    // Create our base path
    CURL url("plugin://weather/" + g_guiSettings.GetString("weather.plugin"));
    if (CGUIDialogPluginSettings::ShowAndGetInput(url))
    {
      // TODO: maybe have ShowAndGetInput return a bool if settings changed, then only reset weather if true.
      g_weatherManager.Refresh();
    }
  }
  else if (strSetting.Equals("lookandfeel.rssedit"))
    CBuiltins::Execute("RunScript("RSSEDITOR_PATH")");
  else if (strSetting.Equals("musiclibrary.scrapersettings"))
  {
    CMusicDatabase database;
    database.Open();
    SScraperInfo info;
    database.GetScraperForPath("musicdb://",info);

    if (info.settings.LoadSettingsXML("special://xbmc/system/scrapers/music/" + info.strPath))
        CGUIDialogPluginSettings::ShowAndGetInput(info);

    database.SetScraperForPath("musicdb://",info);
    database.Close();
  }

  // if OnClick() returns false, the setting hasn't changed or doesn't
  // require immediate update
  if (!pSettingControl->OnClick())
  {
    UpdateSettings();
    if (!pSettingControl->IsDelayed())
      return;
  }

  if (pSettingControl->IsDelayed())
  { // delayed setting
    m_delayedSetting = pSettingControl;
    m_delayedTimer.StartZero();
  }
  else
    OnSettingChanged(pSettingControl);
}

void CGUIWindowSettingsCategory::CheckForUpdates()
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    CBaseSettingControl *pSettingControl = m_vecSettings[i];
    if (pSettingControl->NeedsUpdate())
    {
      OnSettingChanged(pSettingControl);
      pSettingControl->Reset();
    }
  }
}

void CGUIWindowSettingsCategory::OnSettingChanged(CBaseSettingControl *pSettingControl)
{
  CStdString strSetting = pSettingControl->GetSetting()->GetSetting();

  // ok, now check the various special things we need to do
  if (strSetting.Equals("musicplayer.visualisation"))
  { // new visualisation choosen...
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    if (pControl->GetValue() == 0)
      pSettingString->SetData("None");
    else
      pSettingString->SetData(pControl->GetCurrentLabel() + ".vis");
  }
  else if (strSetting.Equals("debug.showloginfo"))
  {
    g_advancedSettings.SetDebugMode(g_guiSettings.GetBool("debug.showloginfo"));
  }
  /*else if (strSetting.Equals("musicfiles.repeat"))
  {
    g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC_TEMP, g_guiSettings.GetBool("musicfiles.repeat") ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  }*/
  else if (strSetting.Equals("karaoke.port0voicemask"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("karaoke.port0voicemask", pControl->GetCurrentLabel());
    FillInVoiceMaskValues(0, g_guiSettings.GetSetting("karaoke.port0voicemask"));
  }
  else if (strSetting.Equals("karaoke.port1voicemask"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("karaoke.port1voicemask", pControl->GetCurrentLabel());
    FillInVoiceMaskValues(1, g_guiSettings.GetSetting("karaoke.port1voicemask"));
  }
  else if (strSetting.Equals("karaoke.port2voicemask"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("karaoke.port2voicemask", pControl->GetCurrentLabel());
    FillInVoiceMaskValues(2, g_guiSettings.GetSetting("karaoke.port2voicemask"));
  }
  else if (strSetting.Equals("karaoke.port2voicemask"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("karaoke.port3voicemask", pControl->GetCurrentLabel());
    FillInVoiceMaskValues(3, g_guiSettings.GetSetting("karaoke.port3voicemask"));
  }
  else if (strSetting.Equals("musiclibrary.cleanup"))
  {
    CMusicDatabase musicdatabase;
    musicdatabase.Clean();
    CUtil::DeleteMusicDatabaseDirectoryCache();
  }
  else if (strSetting.Equals("musiclibrary.scraper"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    FillInScrapers(pControl, pControl->GetCurrentLabel(), "music");
  }
  else if (strSetting.Equals("scrapers.moviedefault"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    FillInScrapers(pControl, pControl->GetCurrentLabel(), "movies");
  }
  else if (strSetting.Equals("scrapers.tvshowdefault"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    FillInScrapers(pControl, pControl->GetCurrentLabel(), "tvshows");
  }
  else if (strSetting.Equals("scrapers.musicvideodefault"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    FillInScrapers(pControl, pControl->GetCurrentLabel(), "musicvideos");
  }
  else if (strSetting.Equals("videolibrary.cleanup"))
  {
    if (CGUIDialogYesNo::ShowAndGetInput(313, 333, 0, 0))
    {
      CVideoDatabase videodatabase;
      videodatabase.Open();
      videodatabase.CleanDatabase();
      videodatabase.Close();
    }
  }
  else if (strSetting.Equals("videolibrary.export"))
    CBuiltins::Execute("exportlibrary(video)");
  else if (strSetting.Equals("musiclibrary.export"))
    CBuiltins::Execute("exportlibrary(music)");

  else if (strSetting.Equals("videolibrary.import"))
  {
    CStdString path;
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(651) , path))
    {
      CVideoDatabase videodatabase;
      videodatabase.Open();
      videodatabase.ImportFromXML(path);
      videodatabase.Close();
    }
  }
  else if (strSetting.Equals("musiclibrary.import"))
  {
    CStdString path;
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, "musicdb.xml", g_localizeStrings.Get(651) , path))
    {
      CMusicDatabase musicdatabase;
      musicdatabase.Open();
      musicdatabase.ImportFromXML(path);
      musicdatabase.Close();
    }
  }
  else if (strSetting.Equals("scrobbler.lastfmsubmit") || strSetting.Equals("scrobbler.lastfmsubmitradio") || strSetting.Equals("scrobbler.lastfmusername") || strSetting.Equals("scrobbler.lastfmpass"))
  {
    CStdString strPassword=g_guiSettings.GetString("scrobbler.lastfmpass");
    CStdString strUserName=g_guiSettings.GetString("scrobbler.lastfmusername");
    if ((g_guiSettings.GetBool("scrobbler.lastfmsubmit") || 
         g_guiSettings.GetBool("scrobbler.lastfmsubmitradio")) &&
         !strUserName.IsEmpty() && !strPassword.IsEmpty())
    {
      CLastfmScrobbler::GetInstance()->Init();
    }
    else
    {
      CLastfmScrobbler::GetInstance()->Term();
    }
  }
  else if (strSetting.Equals("scrobbler.librefmsubmit") || strSetting.Equals("scrobbler.librefmsubmitradio") || strSetting.Equals("scrobbler.librefmusername") || strSetting.Equals("scrobbler.librefmpass"))
  {
    CStdString strPassword=g_guiSettings.GetString("scrobbler.librefmpass");
    CStdString strUserName=g_guiSettings.GetString("scrobbler.librefmusername");
    if ((g_guiSettings.GetBool("scrobbler.librefmsubmit") || 
         g_guiSettings.GetBool("scrobbler.librefmsubmitradio")) &&
         !strUserName.IsEmpty() && !strPassword.IsEmpty())
    {
      CLibrefmScrobbler::GetInstance()->Init();
    }
    else
    {
      CLibrefmScrobbler::GetInstance()->Term();
    }
  }
  else if (strSetting.Equals("musicplayer.outputtoallspeakers"))
  {
    if (!g_application.IsPlaying())
    {
      g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
    }
  }
  else if (strSetting.Left(22).Equals("MusicPlayer.ReplayGain"))
  { // Update our replaygain settings
    g_guiSettings.m_replayGain.iType = g_guiSettings.GetInt("musicplayer.replaygaintype");
    g_guiSettings.m_replayGain.iPreAmp = g_guiSettings.GetInt("musicplayer.replaygainpreamp");
    g_guiSettings.m_replayGain.iNoGainPreAmp = g_guiSettings.GetInt("musicplayer.replaygainnogainpreamp");
    g_guiSettings.m_replayGain.bAvoidClipping = g_guiSettings.GetBool("musicplayer.replaygainavoidclipping");
  }
#ifdef HAS_LCD
  else if (strSetting.Equals("lcd.type"))
  {
    g_lcd->Initialize();
  }
  else if (strSetting.Equals("lcd.backlight"))
  {
    g_lcd->SetBackLight(((CSettingInt *)pSettingControl->GetSetting())->GetData());
  }
  else if (strSetting.Equals("lcd.modchip"))
  {
    g_lcd->Stop();
    CLCDFactory factory;
    delete g_lcd;
    g_lcd = factory.Create();
    g_lcd->Initialize();
  }
  else if (strSetting.Equals("lcd.contrast"))
  {
    g_lcd->SetContrast(((CSettingInt *)pSettingControl->GetSetting())->GetData());
  }
#endif
#ifdef HAS_XBOX_HARDWARE
  else if (strSetting.Equals("system.targettemperature"))
  {
    CSettingInt *pSetting = (CSettingInt*)pSettingControl->GetSetting();
    CFanController::Instance()->SetTargetTemperature(pSetting->GetData());
  }
  else if (strSetting.Equals("system.fanspeed"))
  {
    CSettingInt *pSetting = (CSettingInt*)pSettingControl->GetSetting();
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID);
    g_windowManager.SendMessage(msg);
    int iSpeed = (RESOLUTION)msg.GetParam1();
    g_guiSettings.SetInt("system.fanspeed", iSpeed);
    CFanController::Instance()->SetFanSpeed(iSpeed);
  }
  else if (strSetting.Equals("system.autotemperature"))
  {
    CSettingBool *pSetting = (CSettingBool*)pSettingControl->GetSetting();
    if (pSetting->GetData())
    {
      g_guiSettings.SetBool("system.fanspeedcontrol", false);
      CFanController::Instance()->Start(g_guiSettings.GetInt("system.targettemperature"), g_guiSettings.GetInt("system.minfanspeed") );
    }
    else
      CFanController::Instance()->Stop();
  }
  else if (strSetting.Equals("system.minfanspeed"))
  {
    CSettingInt *pSetting = (CSettingInt*)pSettingControl->GetSetting();
    CFanController::Instance()->SetMinFanSpeed(pSetting->GetData());
  }
  else if (strSetting.Equals("system.fanspeedcontrol"))
  {
    CSettingBool *pSetting = (CSettingBool*)pSettingControl->GetSetting();
    if (pSetting->GetData())
    {
      g_guiSettings.SetBool("system.autotemperature", false);
      CFanController::Instance()->Stop();
      CFanController::Instance()->SetFanSpeed(g_guiSettings.GetInt("system.fanspeed"));
    }
    else
      CFanController::Instance()->RestoreStartupSpeed();
  }
  else if (strSetting.Equals("harddisk.aamlevel"))
  {
    CSettingInt * pSetting = (CSettingInt*)pSettingControl->GetSetting();
    int setting_level = pSetting->GetData();

    if (setting_level == AAM_QUIET)
      XKHDD::SetAAMLevel(0x80);
    else if (setting_level == AAM_FAST)
      XKHDD::SetAAMLevel(0xFE);
  }
  else if (strSetting.Equals("harddisk.apmlevel"))
  {
    CSettingInt * pSetting = (CSettingInt*)pSettingControl->GetSetting();
    int setting_level = pSetting->GetData();

    switch(setting_level)
    {
    case APM_LOPOWER:
      XKHDD::SetAPMLevel(0x80);
      break;
    case APM_HIPOWER:
      XKHDD::SetAPMLevel(0xFE);
      break;
    case APM_LOPOWER_STANDBY:
      XKHDD::SetAPMLevel(0x01);
      break;
    case APM_HIPOWER_STANDBY:
      XKHDD::SetAPMLevel(0x7F);
      break;
    }
  }
  else if (strSetting.Equals("autodetect.nickname") )
  {
    CStdString strXboxNickNameIn = g_guiSettings.GetString("autodetect.nickname");
    CUtil::SetXBOXNickName(strXboxNickNameIn, strXboxNickNameIn);
  }
#endif
  else if (strSetting.Equals("services.ftpserver"))
  {
    g_application.StopFtpServer();
    if (g_guiSettings.GetBool("services.ftpserver"))
      g_application.StartFtpServer();
  }
  else if (strSetting.Equals("services.ftpserverpassword"))
  {
   SetFTPServerUserPass();
  }
  else if (strSetting.Equals("services.ftpserveruser"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("services.ftpserveruser", pControl->GetCurrentLabel());
  }

  else if ( strSetting.Equals("services.webserver") || strSetting.Equals("services.webserverport") || 
            strSetting.Equals("services.webserverusername") || strSetting.Equals("services.webserverpassword"))
  {
    if (strSetting.Equals("services.webserverport"))
    {
      CSettingString *pSetting = (CSettingString *)pSettingControl->GetSetting();
      // check that it's a valid port
      int port = atoi(pSetting->GetData().c_str());
      if (port <= 0 || port > 65535)
        pSetting->SetData("80");
    }
    g_application.StopWebServer();
    if (g_guiSettings.GetBool("services.webserver"))
    {
      g_application.StartWebServer();
      if (g_application.m_pWebServer) {
        if (strSetting.Equals("services.webserverusername"))
          g_application.m_pWebServer->SetUserName(g_guiSettings.GetString("services.webserverusername").c_str());
        else
          g_application.m_pWebServer->SetPassword(g_guiSettings.GetString("services.webserverpassword").c_str());
      }
    }
  }
  
  else if (strSetting.Equals("network.ipaddress"))
  {
    if (g_guiSettings.GetInt("network.assignment") == NETWORK_STATIC)
    {
      CStdString strDefault = g_guiSettings.GetString("network.ipaddress").Left(g_guiSettings.GetString("network.ipaddress").ReverseFind('.'))+".1";
      if (g_guiSettings.GetString("network.gateway").Equals("0.0.0.0"))
        g_guiSettings.SetString("network.gateway",strDefault);
      if (g_guiSettings.GetString("network.dns").Equals("0.0.0.0"))
        g_guiSettings.SetString("network.dns",strDefault);
      if (g_guiSettings.GetString("network.dns2").Equals("0.0.0.0"))
        g_guiSettings.SetString("network.dns2",strDefault);
    }
  }
    
  else if (strSetting.Equals("network.httpproxyport"))
  {
    CSettingString *pSetting = (CSettingString *)pSettingControl->GetSetting();
    // check that it's a valid port
    int port = atoi(pSetting->GetData().c_str());
    if (port <= 0 || port > 65535)
      pSetting->SetData("8080");
  }
  else if (strSetting.Equals("videoplayer.calibrate") || strSetting.Equals("videoscreen.guicalibration"))
  { // activate the video calibration screen
    g_windowManager.ActivateWindow(WINDOW_SCREEN_CALIBRATION);
  }
  else if (strSetting.Equals("dvds.externaldvdplayer"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = pSettingString->GetData();
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    // TODO 2.0: Localize this
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, ".xbe", g_localizeStrings.Get(655), path))
      pSettingString->SetData(path);
  }
  else if (strSetting.Equals("subtitles.height"))
  {
    if (!CUtil::IsUsingTTFSubtitles())
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
      ((CSettingInt *)pSettingControl->GetSetting())->FromString(pControl->GetCurrentLabel());
    }
  }
  else if (strSetting.Equals("subtitles.font"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    pSettingString->SetData(pControl->GetCurrentLabel());
    FillInSubtitleHeights(g_guiSettings.GetSetting("subtitles.height"));
  }
  else if (strSetting.Equals("subtitles.charset"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString newCharset="DEFAULT";
    if (pControl->GetValue()!=0)
     newCharset = g_charsetConverter.getCharsetNameByLabel(pControl->GetCurrentLabel());
    if (newCharset != "" && (newCharset != pSettingString->GetData() || newCharset=="DEFAULT"))
    {
      pSettingString->SetData(newCharset);
      g_charsetConverter.reset();
    }
  }
  else if (strSetting.Equals("locale.charset"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString newCharset="DEFAULT";
    if (pControl->GetValue()!=0)
     newCharset = g_charsetConverter.getCharsetNameByLabel(pControl->GetCurrentLabel());
    if (newCharset != "" && (newCharset != pSettingString->GetData() || newCharset=="DEFAULT"))
    {
      pSettingString->SetData(newCharset);
      g_charsetConverter.reset();
    }
  }
  else if (strSetting.Equals("lookandfeel.font"))
  { // new font choosen...
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString strSkinFontSet = pControl->GetCurrentLabel();
    if (strSkinFontSet != ".svn" && strSkinFontSet != g_guiSettings.GetString("lookandfeel.font"))
    {
      m_strNewSkinFontSet = strSkinFontSet;
      g_application.DelayLoadSkin();
    }
    else
    { // Do not reload the language we are already using
      m_strNewSkinFontSet.Empty();
      g_application.CancelDelayLoadSkin();
    }
  }
  else if (strSetting.Equals("lookandfeel.skin"))
  { // new skin choosen...
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString strSkin = pControl->GetCurrentLabel();
    CStdString strSkinPath = "Q:\\skin\\" + strSkin;
    if (g_SkinInfo.Check(strSkinPath))
    {
      m_strErrorMessage.Empty();
      pControl->SettingsCategorySetSpinTextColor(pControl->GetButtonLabelInfo().textColor);
      if (strSkin != ".svn" && strSkin != g_guiSettings.GetString("lookandfeel.skin"))
      {
        m_strNewSkin = strSkin;
        g_application.DelayLoadSkin();
      }
      else
      { // Do not reload the skin we are already using
        m_strNewSkin.Empty();
        g_application.CancelDelayLoadSkin();
      }
    }
    else
    {
      m_strErrorMessage.Format("Incompatible skin. We require skins of version %0.2f or higher", g_SkinInfo.GetMinVersion());
      m_strNewSkin.Empty();
      g_application.CancelDelayLoadSkin();
      pControl->SettingsCategorySetSpinTextColor(pControl->GetButtonLabelInfo().disabledColor);
    }
  }
  else if (strSetting.Equals("lookandfeel.soundskin"))
  { // new sound skin choosen...
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    if (pControl->GetValue()==0)
      g_guiSettings.SetString("lookandfeel.soundskin", "OFF");
    else if (pControl->GetValue()==1)
      g_guiSettings.SetString("lookandfeel.soundskin", "SKINDEFAULT");
    else
      g_guiSettings.SetString("lookandfeel.soundskin", pControl->GetCurrentLabel());

    g_audioManager.Enable(true);
    g_audioManager.Load();
  }
  else if (strSetting.Equals("lookandfeel.enablemouse"))
  {
    g_Mouse.SetEnabled(g_guiSettings.GetBool("lookandfeel.enablemouse"));
  }
  else if (strSetting.Equals("videoscreen.resolution"))
  { // new resolution choosen... - update if necessary
    CSettingInt *pSettingInt = (CSettingInt *)pSettingControl->GetSetting();
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID);
    g_windowManager.SendMessage(msg);
    m_NewResolution = (RESOLUTION)msg.GetParam1();
    // reset our skin if necessary
    // delay change of resolution
    if (m_NewResolution != g_guiSettings.m_LookAndFeelResolution)
    {
      g_application.DelayLoadSkin();
    }
    else
    { // Do not reload the resolution we are using
      m_NewResolution = INVALID;
      g_application.CancelDelayLoadSkin();
    }
  }
  else if (strSetting.Equals("system.ledcolour"))
  { 
#ifdef HAS_XBOX_HARDWARE
    // Alter LED Colour immediately
    int iData =  ((CSettingInt *)pSettingControl->GetSetting())->GetData();
    if (iData == LED_COLOUR_NO_CHANGE)
      // LED_COLOUR_NO_CHANGE: to prevent "led off" on colour immediately change, set to default green! 
      //                       (we have no previos reference LED COLOUR, to set the LED colour back)
      //                       on next boot the colour will not changed and the default BIOS led colour will used
      ILED::CLEDControl(LED_COLOUR_GREEN); 
    else
      ILED::CLEDControl(iData);
#endif
  }
  else if (strSetting.Equals("locale.language"))
  { // new language chosen...
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString strLanguage = pControl->GetCurrentLabel();
    if (strLanguage != ".svn" && strLanguage != pSettingString->GetData())
    {
      m_strNewLanguage = strLanguage;
      g_application.DelayLoadSkin();
    }
    else
    { // Do not reload the language we are already using
      m_strNewLanguage.Empty();
      g_application.CancelDelayLoadSkin();
    }
  }
  else if (strSetting.Equals("lookandfeel.skintheme"))
  { //a new Theme was chosen
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());

    CStdString strSkinTheme;

    if (pControl->GetValue() == 0) // Use default theme
      strSkinTheme = "SKINDEFAULT";
    else
      strSkinTheme = pControl->GetCurrentLabel() + ".xpr";

    if (strSkinTheme != pSettingString->GetData())
    {
      m_strNewSkinTheme = strSkinTheme;
      g_application.DelayLoadSkin();
    }
    else
    { // Do not reload the skin theme we are using
      m_strNewSkinTheme.Empty();
      g_application.CancelDelayLoadSkin();
    }
  }
  else if (strSetting.Equals("lookandfeel.skincolors"))
  { //a new color was chosen
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());

    CStdString strSkinColor;

    if (pControl->GetValue() == 0) // Use default colors
      strSkinColor = "SKINDEFAULT";
    else
      strSkinColor = pControl->GetCurrentLabel() + ".xml";

    if (strSkinColor != pSettingString->GetData())
    {
      m_strNewSkinColors = strSkinColor;
      g_application.DelayLoadSkin();
    }
    else
    { // Do not reload the skin colors we are using
      m_strNewSkinColors.Empty();
      g_application.CancelDelayLoadSkin();
    }
  }
  else if (strSetting.Equals("videoplayer.displayresolution"))
  {
    CSettingInt *pSettingInt = (CSettingInt *)pSettingControl->GetSetting();
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID);
    g_windowManager.SendMessage(msg);
    pSettingInt->SetData(msg.GetParam1());
  }
  else if (strSetting.Equals("videoscreen.flickerfilter") || strSetting.Equals("videoscreen.soften"))
  { // reset display
    g_graphicsContext.SetVideoResolution(g_guiSettings.m_LookAndFeelResolution, TRUE);
  }
  else if (strSetting.Equals("screensaver.mode"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    int iValue = pControl->GetValue();
    CStdString strScreenSaver;
    if (iValue == 0)
      strScreenSaver = "None";
    else if (iValue == 1)
      strScreenSaver = "Dim";
    else if (iValue == 2)
      strScreenSaver = "Black";
    else if (iValue == 3)
      strScreenSaver = "SlideShow"; // PictureSlideShow
    else if (iValue == 4)
      strScreenSaver = "Fanart Slideshow"; //Fanart Slideshow
    else
      strScreenSaver = pControl->GetCurrentLabel() + ".xbs";
    pSettingString->SetData(strScreenSaver);
  }
  else if (strSetting.Equals("screensaver.preview"))
  {
    g_application.ActivateScreenSaver(true);
  }
  else if (strSetting.Equals("screensaver.slideshowpath"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = pSettingString->GetData();
    VECSOURCES shares = g_settings.m_pictureSources;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(pSettingString->m_iHeadingString), path))
      pSettingString->SetData(path);
  }
  else if (strSetting.Equals("myprograms.dashboard"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = pSettingString->GetData();
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, ".xbe", g_localizeStrings.Get(pSettingString->m_iHeadingString), path))
      pSettingString->SetData(path);
  }
  else if (strSetting.Equals("myprograms.trainerpath") || strSetting.Equals("debug.screenshotpath") || strSetting.Equals("audiocds.recordingpath") || strSetting.Equals("cddaripper.path") || strSetting.Equals("subtitles.custompath"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = g_guiSettings.GetString(strSetting,false);
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    UpdateSettings();
    bool bWriteOnly = true;
    if (strSetting.Equals("myprograms.trainerpath"))
      bWriteOnly = false;

    if (strSetting.Equals("subtitles.custompath"))
    {
      bWriteOnly = false;
      shares = g_settings.m_videoSources;
      g_mediaManager.GetLocalDrives(shares);
    }
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(pSettingString->m_iHeadingString), path, bWriteOnly))
    {
      pSettingString->SetData(path);

      if (strSetting.Equals("myprograms.trainerpath"))
      {
        if (CGUIDialogYesNo::ShowAndGetInput(12012,20135,20022,20022))
        {
          CGUIWindowPrograms* pWindow = (CGUIWindowPrograms*)g_windowManager.GetWindow(WINDOW_PROGRAMS);
          if (pWindow)
            pWindow->PopulateTrainersList();
        }
      }
    }
  }
  else if (strSetting.Left(22).Equals("MusicPlayer.ReplayGain"))
  { // Update our replaygain settings
    g_guiSettings.m_replayGain.iType = g_guiSettings.GetInt("musicplayer.replaygaintype");
    g_guiSettings.m_replayGain.iPreAmp = g_guiSettings.GetInt("musicplayer.replaygainpreamp");
    g_guiSettings.m_replayGain.iNoGainPreAmp = g_guiSettings.GetInt("musicplayer.replaygainnogainpreamp");
    g_guiSettings.m_replayGain.bAvoidClipping = g_guiSettings.GetBool("musicplayer.replaygainavoidclipping");
  }
  else if (strSetting.Equals("locale.country"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());

    const CStdString& strRegion=pControl->GetCurrentLabel();
    g_langInfo.SetCurrentRegion(strRegion);
    g_guiSettings.SetString("locale.country", strRegion);
    g_weatherManager.Refresh(); // need to reset our weather, as temperatures need re-translating.
  }
#ifdef HAS_TIME_SERVER
  else if (strSetting.Equals("locale.timeserver") || strSetting.Equals("locale.timeserveraddress"))
  {
    g_application.StopTimeServer();
    if (g_guiSettings.GetBool("locale.timeserver"))
      g_application.StartTimeServer();
  }
#endif
  else if (strSetting.Equals("locale.time"))
  {
    SYSTEMTIME curTime;
    GetLocalTime(&curTime);
    if (CGUIDialogNumeric::ShowAndGetTime(curTime, g_localizeStrings.Get(14066)))
    { // yay!
      SYSTEMTIME curDate;
      GetLocalTime(&curDate);
      CUtil::SetSysDateTimeYear(curDate.wYear, curDate.wMonth, curDate.wDay, curTime.wHour, curTime.wMinute);
    }
  }
  else if (strSetting.Equals("locale.date"))
  {
    SYSTEMTIME curDate;
    GetLocalTime(&curDate);
    if (CGUIDialogNumeric::ShowAndGetDate(curDate, g_localizeStrings.Get(14067)))
    { // yay!
      SYSTEMTIME curTime;
      GetLocalTime(&curTime);
      CUtil::SetSysDateTimeYear(curDate.wYear, curDate.wMonth, curDate.wDay, curTime.wHour, curTime.wMinute);
    }
  }
  else if (strSetting.Equals("smb.winsserver") || strSetting.Equals("smb.workgroup") )
  {
    if (g_guiSettings.GetString("smb.winsserver") == "0.0.0.0")
      g_guiSettings.SetString("smb.winsserver", "");

    /* okey we really don't need to restarat, only deinit samba, but that could be damn hard if something is playing*/
    //TODO - General way of handling setting changes that require restart

    CGUIDialogOK *dlg = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!dlg) return ;
    dlg->SetHeading( g_localizeStrings.Get(14038) );
    dlg->SetLine( 0, g_localizeStrings.Get(14039) );
    dlg->SetLine( 1, g_localizeStrings.Get(14040));
    dlg->SetLine( 2, "");
    dlg->DoModal();

    if (dlg->IsConfirmed())
    {
      g_settings.Save();
      g_application.getApplicationMessenger().RestartApp();
    }
  }
  else if (strSetting.Equals("services.upnpserver"))
  {
#ifdef HAS_UPNP
    if (g_guiSettings.GetBool("services.upnpserver"))
      g_application.StartUPnPServer();
    else
      g_application.StopUPnPServer();
#endif
  }
  else if (strSetting.Equals("services.upnprenderer"))
  {
#ifdef HAS_UPNP
    if (g_guiSettings.GetBool("services.upnprenderer"))
      g_application.StartUPnPRenderer();
    else
      g_application.StopUPnPRenderer();
#endif
  }
  else if (strSetting.Equals("services.esenabled"))
  {
#ifdef HAS_EVENT_SERVER
    if (g_guiSettings.GetBool("services.esenabled"))
      g_application.StartEventServer();
    else
    {
      if (!g_application.StopEventServer(true))
      {
        g_guiSettings.SetBool("services.esenabled", true);
        CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
        if (pControl) pControl->SetEnabled(true);
      }
    }
#endif
  }
  else if (strSetting.Equals("services.esallinterfaces"))
  {
#ifdef HAS_EVENT_SERVER
    if (g_guiSettings.GetBool("services.esenabled"))
    {
      if (g_application.StopEventServer(true))
        g_application.StartEventServer();
      else
      {
        g_guiSettings.SetBool("services.esenabled", true);
        CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
        if (pControl) pControl->SetEnabled(true);
      }
    }
#endif
  }
  else if (strSetting.Equals("services.esinitialdelay") || 
           strSetting.Equals("services.escontinuousdelay"))    
  {
#ifdef HAS_EVENT_SERVER
    if (g_guiSettings.GetBool("services.esenabled"))
    {
      g_application.RefreshEventServer();
    }
#endif      
  }
  else if (strSetting.Equals("masterlock.lockcode"))
  {
    // Now Prompt User to enter the old and then the new MasterCode!
    if(g_passwordManager.SetMasterLockMode())
    {
      // We asked for the master password and saved the new one!
      // Nothing todo here
    }
  }

  else if (strSetting.Equals("lookandfeel.skinzoom"))
  {
    g_fontManager.ReloadTTFFonts();
  }
  else if (strSetting.Equals("videolibrary.flattentvshows") ||
           strSetting.Equals("videolibrary.removeduplicates"))
  {
    CUtil::DeleteVideoDatabaseDirectoryCache();
  }
  else if (strSetting.Equals("system.mceremote"))
  {
    CBuiltins::Execute("Action(reloadkeymaps)");
  }
  UpdateSettings();
}

void CGUIWindowSettingsCategory::FreeControls()
{
  // clear the category group
  CGUIControlGroupList *control = (CGUIControlGroupList *)GetControl(CATEGORY_GROUP_ID);
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }
  m_vecSections.clear();
  FreeSettingsControls();
}

void CGUIWindowSettingsCategory::FreeSettingsControls()
{
  // clear the settings group
  CGUIControlGroupList *control = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }

  for(int i = 0; (size_t)i < m_vecSettings.size(); i++)
  {
    delete m_vecSettings[i];
  }
  m_vecSettings.clear();
}

void CGUIWindowSettingsCategory::AddSetting(CSetting *pSetting, float width, int &iControlID)
{
  CBaseSettingControl *pSettingControl = NULL;
  CGUIControl *pControl = NULL;
  if (pSetting->GetControlType() == CHECKMARK_CONTROL)
  {
    pControl = new CGUIRadioButtonControl(*m_pOriginalRadioButton);
    if (!pControl) return ;
    ((CGUIRadioButtonControl *)pControl)->SetLabel(g_localizeStrings.Get(pSetting->GetLabel()));
    pControl->SetWidth(width);
    pSettingControl = new CRadioButtonSettingControl((CGUIRadioButtonControl *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() == SPIN_CONTROL_FLOAT || pSetting->GetControlType() == SPIN_CONTROL_INT_PLUS || pSetting->GetControlType() == SPIN_CONTROL_TEXT || pSetting->GetControlType() == SPIN_CONTROL_INT)
  {
    pControl = new CGUISpinControlEx(*m_pOriginalSpin);
    if (!pControl) return ;
    pControl->SetWidth(width);
    ((CGUISpinControlEx *)pControl)->SetText(g_localizeStrings.Get(pSetting->GetLabel()));
    pSettingControl = new CSpinExSettingControl((CGUISpinControlEx *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() == SEPARATOR_CONTROL && m_pOriginalImage)
  {
    pControl = new CGUIImage(*m_pOriginalImage);
    if (!pControl) return;
    pControl->SetWidth(width);
    pSettingControl = new CSeparatorSettingControl((CGUIImage *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() == EDIT_CONTROL_INPUT ||
           pSetting->GetControlType() == EDIT_CONTROL_HIDDEN_INPUT ||
           pSetting->GetControlType() == EDIT_CONTROL_MD5_INPUT ||
           pSetting->GetControlType() == EDIT_CONTROL_NUMBER_INPUT ||
           pSetting->GetControlType() == EDIT_CONTROL_IP_INPUT)
  {
    pControl = new CGUIEditControl(*m_pOriginalEdit);
    if (!pControl) return ;
    ((CGUIEditControl *)pControl)->SettingsCategorySetTextAlign(XBFONT_CENTER_Y);
    ((CGUIEditControl *)pControl)->SetLabel(g_localizeStrings.Get(pSetting->GetLabel()));
    pControl->SetWidth(width);
    pSettingControl = new CEditSettingControl((CGUIEditControl *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() != SEPARATOR_CONTROL) // button control
  {
    pControl = new CGUIButtonControl(*m_pOriginalButton);
    if (!pControl) return ;
    ((CGUIButtonControl *)pControl)->SettingsCategorySetTextAlign(XBFONT_CENTER_Y);
    ((CGUIButtonControl *)pControl)->SetLabel(g_localizeStrings.Get(pSetting->GetLabel()));
    pControl->SetWidth(width);
    pSettingControl = new CButtonSettingControl((CGUIButtonControl *)pControl, iControlID, pSetting);
  }
  if (!pControl) return;
  pControl->SetID(iControlID++);
  pControl->SetVisible(true);
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
  if (group)
  {
    pControl->AllocResources();
    group->AddControl(pControl);
    m_vecSettings.push_back(pSettingControl);
  }
}

void CGUIWindowSettingsCategory::FrameMove()
{
  // update realtime changeable stuff
  UpdateRealTimeSettings();

  if (m_delayedSetting && m_delayedTimer.GetElapsedMilliseconds() > 3000)
  { // we send a thread message so that it's processed the following frame (some settings won't
    // like being changed during Render())
    CGUIMessage message(GUI_MSG_UPDATE_ITEM, GetID(), GetID());
    g_windowManager.SendThreadMessage(message, GetID());
    m_delayedTimer.Stop();
  }
  CGUIWindow::FrameMove();
}

void CGUIWindowSettingsCategory::Render()
{
  // update alpha status of current button
  bool bAlphaFaded = false;
  CGUIControl *control = GetFirstFocusableControl(CONTROL_START_BUTTONS + m_iSection);
  if (control && !control->HasFocus())
  {
    if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
    {
      control->SetFocus(true);
      ((CGUIButtonControl *)control)->SetAlpha(0x80);
      bAlphaFaded = true;
    }
    else if (control->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON)
    {
      control->SetFocus(true);
      ((CGUIButtonControl *)control)->SetSelected(true);
      bAlphaFaded = true;
    }
  }
  CGUIWindow::Render();
  if (bAlphaFaded)
  {
    control->SetFocus(false);
    if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
      ((CGUIButtonControl *)control)->SetAlpha(0xFF);
    else
      ((CGUIButtonControl *)control)->SetSelected(false);
  }
  // render the error message if necessary
  if (m_strErrorMessage.size())
  {
    CGUIFont *pFont = g_fontManager.GetFont("font13");
    float fPosY = g_graphicsContext.GetHeight() * 0.8f;
    float fPosX = g_graphicsContext.GetWidth() * 0.5f;
    CGUITextLayout::DrawText(pFont, fPosX, fPosY, 0xffffffff, 0, m_strErrorMessage, XBFONT_CENTER_X);
  }
}

void CGUIWindowSettingsCategory::CheckNetworkSettings()
{
  // check if our network needs restarting (requires a reset, so check well!)
  if (m_iNetworkAssignment == -1)
  {
    // nothing to do here, folks - move along.
    return ;
  }
  // we need a reset if:
  // 1.  The Network Assignment has changed OR
  // 2.  The Network Assignment is STATIC and one of the network fields have changed
  if (m_iNetworkAssignment != g_guiSettings.GetInt("network.assignment") ||
      (m_iNetworkAssignment == NETWORK_STATIC && (
         m_strNetworkIPAddress != g_guiSettings.GetString("network.ipaddress") ||
         m_strNetworkSubnet != g_guiSettings.GetString("network.subnet") ||
         m_strNetworkGateway != g_guiSettings.GetString("network.gateway") ||
         m_strNetworkDNS != g_guiSettings.GetString("network.dns") ||
         m_strNetworkDNS2 != g_guiSettings.GetString("network.dns2"))))
  {
/*    // our network settings have changed - we should prompt the user to reset XBMC
    if (CGUIDialogYesNo::ShowAndGetInput(14038, 14039, 14040, 0))
    {
      // reset settings
      g_application.getApplicationMessenger().RestartApp();
      // Todo: aquire new network settings without restart app!
    }
    else*/
    {
      g_application.getNetwork().NetworkMessage(CNetwork::SERVICES_DOWN,1);
      g_application.getNetwork().SetupNetwork();
    }

    // update our settings variables    
    m_iNetworkAssignment = g_guiSettings.GetInt("network.assignment");
    m_strNetworkIPAddress = g_guiSettings.GetString("network.ipaddress");
    m_strNetworkSubnet = g_guiSettings.GetString("network.subnet");
    m_strNetworkGateway = g_guiSettings.GetString("network.gateway");
    m_strNetworkDNS = g_guiSettings.GetString("network.dns");
    m_strNetworkDNS2 = g_guiSettings.GetString("network.dns2");

    // replace settings
    /*   g_guiSettings.SetInt("network.assignment", m_iNetworkAssignment);
       g_guiSettings.SetString("network.ipaddress", m_strNetworkIPAddress);
       g_guiSettings.SetString("network.subnet", m_strNetworkSubnet);
       g_guiSettings.SetString("network.gateway", m_strNetworkGateway);
       g_guiSettings.SetString("network.dns", m_strNetworkDNS);*/
  }
}

void CGUIWindowSettingsCategory::FillInSubtitleHeights(CSetting *pSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  if (CUtil::IsUsingTTFSubtitles())
  { // easy - just fill as per usual
    CStdString strLabel;
    for (int i = pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i += pSettingInt->m_iStep)
    {
      if (pSettingInt->m_iFormat > -1)
      {
        CStdString strFormat = g_localizeStrings.Get(pSettingInt->m_iFormat);
        strLabel.Format(strFormat, i);
      }
      else
        strLabel.Format(pSettingInt->m_strFormat, i);
      pControl->AddLabel(strLabel, i);
    }
    pControl->SetValue(pSettingInt->GetData());
  }
#ifdef _XBOX
  else
  {
    if (g_guiSettings.GetString("subtitles.font").size())
    {
      //find font sizes...
      CFileItemList items;
      CStdString strPath = "special://xbmc/system/players/mplayer/font/";
      strPath += g_guiSettings.GetString("subtitles.font");
      strPath += "/";
      CDirectory::GetDirectory(strPath, items);
      int iCurrentSize = 0;
      int iSize = 0;
      for (int i = 0; i < items.Size(); ++i)
      {
        CFileItemPtr pItem = items[i];
        if (pItem->m_bIsFolder)
        {
          if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
          int iSizeTmp = atoi(pItem->GetLabel().c_str());
          if (iSizeTmp == pSettingInt->GetData())
            iCurrentSize = iSize;
          pControl->AddLabel(pItem->GetLabel(), iSize++);
        }
      }
      pControl->SetValue(iCurrentSize);
    }
  }
#endif
}

void CGUIWindowSettingsCategory::FillInSubtitleFonts(CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  int iCurrentFont = 0;
  int iFont = 0;

#ifdef _XBOX
  // Find mplayer fonts...
  {
    CFileItemList items;
    CDirectory::GetDirectory("special://xbmc/system/players/mplayer/font/", items);
    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr pItem = items[i];
      if (pItem->m_bIsFolder)
      {
        if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
        if (strcmpi(pItem->GetLabel().c_str(), pSettingString->GetData().c_str()) == 0)
          iCurrentFont = iFont;
        pControl->AddLabel(pItem->GetLabel(), iFont++);
      }
    }
  }
#endif

  // find TTF fonts
  {
    CFileItemList items;
    if (CDirectory::GetDirectory("special://xbmc/media/Fonts/", items))
    {
      for (int i = 0; i < items.Size(); ++i)
      {
        CFileItemPtr pItem = items[i];

        if (!pItem->m_bIsFolder)
        {

          if ( !URIUtils::GetExtension(pItem->GetLabel()).Equals(".ttf") ) continue;
          if (pItem->GetLabel().Equals(pSettingString->GetData(), false))
            iCurrentFont = iFont;

          pControl->AddLabel(pItem->GetLabel(), iFont++);
        }

      }
    }
  }
  pControl->SetValue(iCurrentFont);
}

void CGUIWindowSettingsCategory::FillInSkinFonts(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();

  int iSkinFontSet = 0;

  m_strNewSkinFontSet.Empty();

  CStdString strPath = g_SkinInfo.GetSkinPath("Font.xml");

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strPath))
  {
    CLog::Log(LOGERROR, "Couldn't load %s", strPath.c_str());
    return ;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();

  CStdString strValue = pRootElement->Value();
  if (strValue != CStdString("fonts"))
  {
    CLog::Log(LOGERROR, "file %s doesnt start with <fonts>", strPath.c_str());
    return ;
  }

  const TiXmlNode *pChild = pRootElement->FirstChild();
  strValue = pChild->Value();
  if (strValue == "fontset")
  {
    while (pChild)
    {
      strValue = pChild->Value();
      if (strValue == "fontset")
      {
        const char* idAttr = ((TiXmlElement*) pChild)->Attribute("id");
        const char* unicodeAttr = ((TiXmlElement*) pChild)->Attribute("unicode");

        bool isUnicode=(unicodeAttr && stricmp(unicodeAttr, "true") == 0);

        bool isAllowed=true;
        if (g_langInfo.ForceUnicodeFont() && !isUnicode)
          isAllowed=false;

        if (idAttr != NULL && isAllowed)
        {
          pControl->AddLabel(idAttr, iSkinFontSet);
          if (strcmpi(idAttr, g_guiSettings.GetString("lookandfeel.font").c_str()) == 0)
            pControl->SetValue(iSkinFontSet);
          iSkinFontSet++;
        }
      }
      pChild = pChild->NextSibling();
    }

  }
  else
  {
    // Since no fontset is defined, there is no selection of a fontset, so disable the component
    pControl->AddLabel(g_localizeStrings.Get(13278), 1);
    pControl->SetValue(1);
    pControl->SetEnabled(false);
  }
}

void CGUIWindowSettingsCategory::FillInSkins(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  pControl->SetShowRange(true);

  m_strNewSkin.Empty();

  //find skins...
  CFileItemList items;
  CDirectory::GetDirectory("special://xbmc/skin/", items);
  if (!CSpecialProtocol::XBMCIsHome())
    CDirectory::GetDirectory("special://home/skin/", items);

  int iCurrentSkin = 0;
  int iSkin = 0;
  vector<CStdString> vecSkins;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "fonts") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "media") == 0) continue;
      //   if (g_SkinInfo.Check(pItem->GetPath()))
      //   {
      vecSkins.push_back(pItem->GetLabel());
      //   }
    }
  }

  sort(vecSkins.begin(), vecSkins.end(), sortstringbyname());
  for (unsigned int i = 0; i < vecSkins.size(); ++i)
  {
    CStdString strSkin = vecSkins[i];
    if (strcmpi(strSkin.c_str(), g_guiSettings.GetString("lookandfeel.skin").c_str()) == 0)
    {
      iCurrentSkin = iSkin;
    }
    pControl->AddLabel(strSkin, iSkin++);
  }
  pControl->SetValue(iCurrentSkin);
  return ;
}

void CGUIWindowSettingsCategory::FillInSoundSkins(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  pControl->SetShowRange(true);

  m_strNewSkin.Empty();

  //find skins...
  CFileItemList items;
  CDirectory::GetDirectory("special://xbmc/sounds/", items);
  if (!CSpecialProtocol::XBMCIsHome())
    CDirectory::GetDirectory("special://home/sounds/", items);

  int iCurrentSoundSkin = 0;
  int iSoundSkin = 0;
  vector<CStdString> vecSoundSkins;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "fonts") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "media") == 0) continue;
      vecSoundSkins.push_back(pItem->GetLabel());
    }
  }

  pControl->AddLabel(g_localizeStrings.Get(474), iSoundSkin++); // Off
  pControl->AddLabel(g_localizeStrings.Get(15109), iSoundSkin++); // Skin Default

  if (g_guiSettings.GetString("lookandfeel.soundskin")=="SKINDEFAULT")
    iCurrentSoundSkin=1;

  sort(vecSoundSkins.begin(), vecSoundSkins.end(), sortstringbyname());
  for (int i = 0; i < (int) vecSoundSkins.size(); ++i)
  {
    CStdString strSkin = vecSoundSkins[i];
    if (strcmpi(strSkin.c_str(), g_guiSettings.GetString("lookandfeel.soundskin").c_str()) == 0)
    {
      iCurrentSoundSkin = iSoundSkin;
    }
    pControl->AddLabel(strSkin, iSoundSkin++);
  }
  pControl->SetValue(iCurrentSoundSkin);
  return ;
}

void CGUIWindowSettingsCategory::FillInCharSets(CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  int iCurrentCharset = 0;
  vector<CStdString> vecCharsets = g_charsetConverter.getCharsetLabels();

  CStdString strCurrentCharsetLabel="DEFAULT";
  if (pSettingString->GetData()!="DEFAULT")
    strCurrentCharsetLabel = g_charsetConverter.getCharsetLabelByName(pSettingString->GetData());

  sort(vecCharsets.begin(), vecCharsets.end(), sortstringbyname());

  vecCharsets.insert(vecCharsets.begin(), g_localizeStrings.Get(13278)); // "Default"

  bool bIsAuto=(pSettingString->GetData()=="DEFAULT");

  for (int i = 0; i < (int) vecCharsets.size(); ++i)
  {
    CStdString strCharsetLabel = vecCharsets[i];

    if (!bIsAuto && strCharsetLabel == strCurrentCharsetLabel)
      iCurrentCharset = i;

    pControl->AddLabel(strCharsetLabel, i);
  }

  pControl->SetValue(iCurrentCharset);
}

void CGUIWindowSettingsCategory::FillInVisualisations(CSetting *pSetting, int iControlID)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  if (!pSetting) return;
  int iWinID = g_windowManager.GetActiveWindow();
  {
    CGUIMessage msg(GUI_MSG_LABEL_RESET, iWinID, iControlID);
    g_windowManager.SendMessage(msg);
  }
  vector<CStdString> vecVis;
  //find visz....
  CFileItemList items;
  CDirectory::GetDirectory("special://xbmc/visualisations/", items);
  if (!CSpecialProtocol::XBMCIsHome())
    CDirectory::GetDirectory("special://home/visualisations/", items);

  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      CStdString strExtension;
      URIUtils::GetExtension(pItem->GetPath(), strExtension);
      if (strExtension == ".vis")
      {
        CStdString strLabel = pItem->GetLabel();
        vecVis.push_back(strLabel.Mid(0, strLabel.size() - 4));
      }
    }
  }

  CStdString strDefaultVis = pSettingString->GetData();
  if (!strDefaultVis.Equals("None"))
    strDefaultVis.Delete(strDefaultVis.size() - 4, 4);

  sort(vecVis.begin(), vecVis.end(), sortstringbyname());

  // add the "disabled" setting first
  int iVis = 0;
  int iCurrentVis = 0;
  {
    CGUIMessage msg(GUI_MSG_LABEL_ADD, iWinID, iControlID, iVis++);
    msg.SetLabel(231);
    g_windowManager.SendMessage(msg);
  }
  for (int i = 0; i < (int) vecVis.size(); ++i)
  {
    CStdString strVis = vecVis[i];

    if (strcmpi(strVis.c_str(), strDefaultVis.c_str()) == 0)
      iCurrentVis = iVis;

    {
      CGUIMessage msg(GUI_MSG_LABEL_ADD, iWinID, iControlID, iVis++);
      msg.SetLabel(strVis);
      g_windowManager.SendMessage(msg);
    }
  }
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, iWinID, iControlID, iCurrentVis);
    g_windowManager.SendMessage(msg);
  }
}

void CGUIWindowSettingsCategory::FillInVoiceMasks(DWORD dwPort, CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetShowRange(true); // show the range
  int iCurrentMask = 0;
  int iMask = 0;
  vector<CStdString> vecMask;

  //find masks in xml...
  TiXmlDocument xmlDoc;
  CStdString fileName = "special://xbmc/system/voicemasks.xml";
  if ( !xmlDoc.LoadFile(fileName) ) return ;
  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue = pRootElement->Value();
  if ( strValue != "VoiceMasks") return ;
  if (pRootElement)
  {
    const TiXmlNode *pChild = pRootElement->FirstChild("Name");
    while (pChild)
    {
      if (pChild->FirstChild())
      {
        CStdString strName = pChild->FirstChild()->Value();
        vecMask.push_back(strName);
      }
      pChild = pChild->NextSibling("Name");
    }
  }
  xmlDoc.Clear();


  CStdString strDefaultMask = pSettingString->GetData();

  sort(vecMask.begin(), vecMask.end(), sortstringbyname());
//  CStdString strCustom = "Custom";
  CStdString strNone = "None";
//  vecMask.insert(vecMask.begin(), strCustom);
  vecMask.insert(vecMask.begin(), strNone);
  for (int i = 0; i < (int) vecMask.size(); ++i)
  {
    CStdString strMask = vecMask[i];

    if (strcmpi(strMask.c_str(), strDefaultMask.c_str()) == 0)
      iCurrentMask = iMask;

    pControl->AddLabel(strMask, iMask++);
  }

  pControl->SetValue(iCurrentMask);
}

void CGUIWindowSettingsCategory::FillInVoiceMaskValues(DWORD dwPort, CSetting *pSetting)
{
  CStdString strCurMask = g_guiSettings.GetString(pSetting->GetSetting());
  if (strCurMask.CompareNoCase("None") == 0 || strCurMask.CompareNoCase("Custom") == 0 )
  {
#ifndef HAS_XBOX_AUDIO
#define XVOICE_MASK_PARAM_DISABLED (-1.0f)
#endif
    g_settings.m_karaokeVoiceMask[dwPort].energy = XVOICE_MASK_PARAM_DISABLED;
    g_settings.m_karaokeVoiceMask[dwPort].pitch = XVOICE_MASK_PARAM_DISABLED;
    g_settings.m_karaokeVoiceMask[dwPort].whisper = XVOICE_MASK_PARAM_DISABLED;
    g_settings.m_karaokeVoiceMask[dwPort].robotic = XVOICE_MASK_PARAM_DISABLED;
    return;
  }

  //find mask values in xml...
  TiXmlDocument xmlDoc;
  CStdString fileName = "special://xbmc/system/voicemasks.xml";
  if ( !xmlDoc.LoadFile( fileName ) ) return ;
  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue = pRootElement->Value();
  if ( strValue != "VoiceMasks") return ;
  if (pRootElement)
  {
    const TiXmlNode *pChild = pRootElement->FirstChild("Name");
    while (pChild)
    {
      CStdString strMask = pChild->FirstChild()->Value();
      if (strMask.CompareNoCase(strCurMask) == 0)
      {
        for (int i = 0; i < 4;i++)
        {
          pChild = pChild->NextSibling();
          if (pChild)
          {
            CStdString strValue = pChild->Value();
            if (strValue.CompareNoCase("fSpecEnergyWeight") == 0)
            {
              if (pChild->FirstChild())
              {
                CStdString strName = pChild->FirstChild()->Value();
                g_settings.m_karaokeVoiceMask[dwPort].energy = (float) atof(strName.c_str());
              }
            }
            else if (strValue.CompareNoCase("fPitchScale") == 0)
            {
              if (pChild->FirstChild())
              {
                CStdString strName = pChild->FirstChild()->Value();
                g_settings.m_karaokeVoiceMask[dwPort].pitch = (float) atof(strName.c_str());
              }
            }
            else if (strValue.CompareNoCase("fWhisperValue") == 0)
            {
              if (pChild->FirstChild())
              {
                CStdString strName = pChild->FirstChild()->Value();
                g_settings.m_karaokeVoiceMask[dwPort].whisper = (float) atof(strName.c_str());
              }
            }
            else if (strValue.CompareNoCase("fRoboticValue") == 0)
            {
              if (pChild->FirstChild())
              {
                CStdString strName = pChild->FirstChild()->Value();
                g_settings.m_karaokeVoiceMask[dwPort].robotic = (float) atof(strName.c_str());
              }
            }
          }
        }
        break;
      }
      pChild = pChild->NextSibling("Name");
    }
  }
  xmlDoc.Clear();
}

void CGUIWindowSettingsCategory::FillInResolutions(CSetting *pSetting, bool playbackSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();
  // Find the valid resolutions and add them as necessary
  vector<RESOLUTION> res;
  g_graphicsContext.GetAllowedResolutions(res, false);

  /* add the virtual resolutions */
  res.push_back(AUTORES);

  for (vector<RESOLUTION>::iterator it = res.begin(); it != res.end();it++)
  {
    RESOLUTION res = *it;
    if (res == AUTORES)
    {
      if (playbackSetting)
      {
        //  TODO: localize 2.0
        if (g_videoConfig.Has1080i() || g_videoConfig.Has720p())
          pControl->AddLabel(g_localizeStrings.Get(20049) , res); // Best Available
        else if (g_videoConfig.HasWidescreen())
          pControl->AddLabel(g_localizeStrings.Get(20050) , res); // Autoswitch between 16x9 and 4x3
        else
          continue;   // don't have a choice of resolution (other than 480p vs NTSC, which isn't a choice)
      }
      else  // "Auto"
        pControl->AddLabel(g_localizeStrings.Get(14061), res);
    }
    else
    {
      pControl->AddLabel(g_settings.m_ResInfo[res].strMode, res);
    }
  }
  pControl->SetValue(pSettingInt->GetData());
}

void CGUIWindowSettingsCategory::FillInLanguages(CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();
  m_strNewLanguage.Empty();
  //find languages...
  CFileItemList items;
  CDirectory::GetDirectory("special://xbmc/language/", items);

  int iCurrentLang = 0;
  int iLanguage = 0;
  vector<CStdString> vecLanguage;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "fonts") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "media") == 0) continue;
      vecLanguage.push_back(pItem->GetLabel());
    }
  }

  sort(vecLanguage.begin(), vecLanguage.end(), sortstringbyname());
  for (unsigned int i = 0; i < vecLanguage.size(); ++i)
  {
    CStdString strLanguage = vecLanguage[i];
    if (strcmpi(strLanguage.c_str(), pSettingString->GetData().c_str()) == 0)
      iCurrentLang = iLanguage;
    pControl->AddLabel(strLanguage, iLanguage++);
  }

  pControl->SetValue(iCurrentLang);
}

void CGUIWindowSettingsCategory::FillInScreenSavers(CSetting *pSetting)
{ // Screensaver mode
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  pControl->AddLabel(g_localizeStrings.Get(351), 0); // Off
  pControl->AddLabel(g_localizeStrings.Get(352), 1); // Dim
  pControl->AddLabel(g_localizeStrings.Get(353), 2); // Black
  pControl->AddLabel(g_localizeStrings.Get(108), 3); // PictureSlideShow
  pControl->AddLabel(g_localizeStrings.Get(20425), 4); // Fanart Slideshow

  //find screensavers ....
  CFileItemList items;
  CDirectory::GetDirectory( "special://xbmc/screensavers/", items);
  if (!CSpecialProtocol::XBMCIsHome())
    CDirectory::GetDirectory("special://home/screensavers/", items);

  int iCurrentScr = -1;
  vector<CStdString> vecScr;
  int i;
  for (i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      CStdString strExtension;
      URIUtils::GetExtension(pItem->GetPath(), strExtension);
      if (strExtension == ".xbs")
      {
        CStdString strLabel = pItem->GetLabel();
        vecScr.push_back(strLabel.Mid(0, strLabel.size() - 4));
      }
    }
  }

  CStdString strDefaultScr = pSettingString->GetData();
  CStdString strExtension;
  URIUtils::GetExtension(strDefaultScr, strExtension);
  if (strExtension == ".xbs")
    strDefaultScr.Delete(strDefaultScr.size() - 4, 4);

  sort(vecScr.begin(), vecScr.end(), sortstringbyname());
  for (i = 0; i < (int) vecScr.size(); ++i)
  {
    CStdString strScr = vecScr[i];

    if (strcmpi(strScr.c_str(), strDefaultScr.c_str()) == 0)
      iCurrentScr = i + PREDEFINED_SCREENSAVERS;

    pControl->AddLabel(strScr, i + PREDEFINED_SCREENSAVERS);
  }

  // if we can't find the screensaver previously configured
  // then fallback to turning the screensaver off.
  if (iCurrentScr < 0)
  {
    if (strDefaultScr == "Dim")
      iCurrentScr = 1;
    else if (strDefaultScr == "Black")
      iCurrentScr = 2;
    else if (strDefaultScr == "SlideShow") // PictureSlideShow
      iCurrentScr = 3;
    else if (strDefaultScr == "Fanart Slideshow") // Fanart slideshow
      iCurrentScr = 4;
    else
    {
      iCurrentScr = 0;
      pSettingString->SetData("None");
    }
  }
  pControl->SetValue(iCurrentScr);
}

void CGUIWindowSettingsCategory::FillInFTPServerUser(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  pControl->SetShowRange(true);
  int iDefaultFtpUser = 0;

#ifdef HAS_FTP_SERVER
  CStdString strFtpUser1; int iUserMax;
  // Get FTP XBOX Users and list them !
  if (CUtil::GetFTPServerUserName(0, strFtpUser1, iUserMax))
  {
    for (int i = 0; i < iUserMax; i++)
    {
      if (CUtil::GetFTPServerUserName(i, strFtpUser1, iUserMax))
        pControl->AddLabel(strFtpUser1.c_str(), i);
      if (strFtpUser1.ToLower() == "xbox") iDefaultFtpUser = i;
    }
    pControl->SetValue(iDefaultFtpUser);
    CUtil::GetFTPServerUserName(iDefaultFtpUser, strFtpUser1, iUserMax);
    g_guiSettings.SetString("services.ftpserveruser", strFtpUser1.c_str());
    pControl->SetInvalid();
  }
  else { //Set "None" if there is no FTP User found!
    pControl->AddLabel(g_localizeStrings.Get(231).c_str(), 0);
    pControl->SetValue(0);
    pControl->SetInvalid();
  }
#endif
}
bool CGUIWindowSettingsCategory::SetFTPServerUserPass()
{
#ifdef HAS_FTP_SERVER
  // TODO: Read the FileZilla Server XML and Set it here!
  // Get GUI USER and pass and set pass to FTP Server
  CStdString strFtpUserName, strFtpUserPassword;
  strFtpUserName      = g_guiSettings.GetString("services.ftpserveruser");
  strFtpUserPassword  = g_guiSettings.GetString("services.ftpserverpassword");
  if(strFtpUserPassword.size()!=0)
  {
    if (CUtil::SetFTPServerUserPassword(strFtpUserName, strFtpUserPassword))
    {
      // todo! ERROR check! if something goes wrong on SetPW!
      // PopUp OK and Display: FTP Server Password was set succesfull!
      CGUIDialogOK::ShowAndGetInput(728, 0, 1247, 0);
    }
    return true;
  }
  else
  {
    // PopUp OK and Display: FTP Server Password is empty! Try Again!
    CGUIDialogOK::ShowAndGetInput(728, 0, 12358, 0);
  }
#endif
  return true;
}

void CGUIWindowSettingsCategory::FillInRegions(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();

  int iCurrentRegion=0;
  CStdStringArray regions;
  g_langInfo.GetRegionNames(regions);

  CStdString strCurrentRegion=g_langInfo.GetCurrentRegion();

  sort(regions.begin(), regions.end(), sortstringbyname());

  for (int i = 0; i < (int) regions.size(); ++i)
  {
    const CStdString& strRegion = regions[i];

    if (strRegion == strCurrentRegion)
      iCurrentRegion = i;

    pControl->AddLabel(strRegion, i);
  }

  pControl->SetValue(iCurrentRegion);
}

CBaseSettingControl *CGUIWindowSettingsCategory::GetSetting(const CStdString &strSetting)
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    if (m_vecSettings[i]->GetSetting()->GetSetting() == strSetting)
      return m_vecSettings[i];
  }
  return NULL;
}

void CGUIWindowSettingsCategory::FillInSkinThemes(CSetting *pSetting)
{
  // There is a default theme (just Textures.xpr)
  // any other *.xpr files are additional themes on top of this one.
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  CStdString strSettingString = g_guiSettings.GetString("lookandfeel.skintheme");

  m_strNewSkinTheme.Empty();

  // Clear and add. the Default Label
  pControl->Clear();
  pControl->SetShowRange(true);
  pControl->AddLabel(g_localizeStrings.Get(15109), 0); // "SKINDEFAULT"! The standart Textures.xpr will be used!

  // find all *.xpr in this path
  CStdString strDefaultTheme = pSettingString->GetData();

  // Search for Themes in the Current skin!
  vector<CStdString> vecTheme;
  CUtil::GetSkinThemes(vecTheme);

  // Remove the .xpr extension from the Themes
  CStdString strExtension;
  URIUtils::GetExtension(strSettingString, strExtension);
  if (strExtension == ".xpr") strSettingString.Delete(strSettingString.size() - 4, 4);
  // Sort the Themes for GUI and list them
  int iCurrentTheme = 0;
  for (int i = 0; i < (int) vecTheme.size(); ++i)
  {
    CStdString strTheme = vecTheme[i];
    // Is the Current Theme our Used Theme! If yes set the ID!
    if (strTheme.CompareNoCase(strSettingString) == 0 )
      iCurrentTheme = i + 1; // 1: #of Predefined Theme [Label]
    pControl->AddLabel(strTheme, i + 1);
  }
  // Set the Choosen Theme
  pControl->SetValue(iCurrentTheme);
}

void CGUIWindowSettingsCategory::FillInSkinColors(CSetting *pSetting)
{
  // There is a default theme (just defaults.xml)
  // any other *.xml files are additional color themes on top of this one.
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  CStdString strSettingString = g_guiSettings.GetString("lookandfeel.skincolors");

  m_strNewSkinColors.Empty();

  // Clear and add. the Default Label
  pControl->Clear();
  pControl->SetShowRange(true);
  pControl->AddLabel(g_localizeStrings.Get(15109), 0); // "SKINDEFAULT"! The standard defaults.xml will be used!

  // Search for colors in the Current skin!
  vector<CStdString> vecColors;

  CStdString strPath;
  URIUtils::AddFileToFolder(g_SkinInfo.GetBaseDir(),"colors",strPath);

  CFileItemList items;
  CDirectory::GetDirectory(strPath, items, ".xml");
  // Search for Themes in the Current skin!
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder && pItem->GetLabel().CompareNoCase("defaults.xml") != 0)
    { // not the default one
      CStdString strLabel = pItem->GetLabel();
      vecColors.push_back(strLabel.Mid(0, strLabel.size() - 4));
    }
  }
  sort(vecColors.begin(), vecColors.end(), sortstringbyname());

  // Remove the .xml extension from the Themes
  if (URIUtils::GetExtension(strSettingString) == ".xml")
    URIUtils::RemoveExtension(strSettingString);

  int iCurrentColor = 0;
  for (int i = 0; i < (int) vecColors.size(); ++i)
  {
    CStdString strColor = vecColors[i];
    // Is the Current Theme our Used Theme! If yes set the ID!
    if (strColor.CompareNoCase(strSettingString) == 0 )
      iCurrentColor = i + 1; // 1: #of Predefined Theme [Label]
    pControl->AddLabel(strColor, i + 1);
  }
  // Set the Choosen Theme
  pControl->SetValue(iCurrentColor);
}

void CGUIWindowSettingsCategory::FillInStartupWindow(CSetting *pSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  const vector<CSkinInfo::CStartupWindow> &startupWindows = g_SkinInfo.GetStartupWindows();

  // TODO: How should we localize this?
  // In the long run there is no way to do it really without the skin having some
  // translation information built in to it, which isn't really feasible.

  // Alternatively we could lookup the strings in the english strings file to get
  // their id and then get the string from that

  // easier would be to have the skinner use the "name" as the label number.

  // eg <window id="0">513</window>

  bool currentSettingFound(false);
  for (vector<CSkinInfo::CStartupWindow>::const_iterator it = startupWindows.begin(); it != startupWindows.end(); it++)
  {
    CStdString windowName((*it).m_name);
    if (StringUtils::IsNaturalNumber(windowName))
      windowName = g_localizeStrings.Get(atoi(windowName.c_str()));
    int windowID((*it).m_id);
    pControl->AddLabel(windowName, windowID);
    if (pSettingInt->GetData() == windowID)
      currentSettingFound = true;
  }

  // ok, now check whether our current option is one of these
  // and set it's value
  if (!currentSettingFound)
  { // nope - set it to the "default" option - the first one
    pSettingInt->SetData(startupWindows[0].m_id);
  }
  pControl->SetValue(pSettingInt->GetData());
}

void CGUIWindowSettingsCategory::OnInitWindow()
{
  m_iNetworkAssignment = g_guiSettings.GetInt("network.assignment");
  m_strNetworkIPAddress = g_guiSettings.GetString("network.ipaddress");
  m_strNetworkSubnet = g_guiSettings.GetString("network.subnet");
  m_strNetworkGateway = g_guiSettings.GetString("network.gateway");
  m_strNetworkDNS = g_guiSettings.GetString("network.dns");
  m_strNetworkDNS2 = g_guiSettings.GetString("network.dns2");
  m_strOldTrackFormat = g_guiSettings.GetString("musicfiles.trackformat");
  m_strOldTrackFormatRight = g_guiSettings.GetString("musicfiles.trackformatright");
  m_NewResolution = INVALID;
  SetupControls();
  CGUIWindow::OnInitWindow();
}

void CGUIWindowSettingsCategory::FillInViewModes(CSetting *pSetting, int windowID)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->AddLabel("Auto", DEFAULT_VIEW_AUTO);
  bool found(false);
  int foundType = 0;
  CGUIWindow *window = g_windowManager.GetWindow(windowID);
  if (window)
  {
    window->Initialize();
    for (int i = 50; i < 60; i++)
    {
      CGUIBaseContainer *control = (CGUIBaseContainer *)window->GetControl(i);
      if (control)
      {
        int type = (control->GetType() << 16) | i;
        pControl->AddLabel(control->GetLabel(), type);
        if (type == pSettingInt->GetData())
          found = true;
        else if ((type >> 16) == (pSettingInt->GetData() >> 16))
          foundType = type;
      }
    }
    window->ClearAll();
  }
  if (!found)
    pSettingInt->SetData(foundType ? foundType : (DEFAULT_VIEW_AUTO));
  pControl->SetValue(pSettingInt->GetData());
}

void CGUIWindowSettingsCategory::FillInSortMethods(CSetting *pSetting, int windowID)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  CFileItemList items("C:");
  CGUIViewState *state = CGUIViewState::GetViewState(windowID, items);
  if (state)
  {
    bool found(false);
    vector< pair<int,int> > sortMethods;
    state->GetSortMethods(sortMethods);
    for (unsigned int i = 0; i < sortMethods.size(); i++)
    {
      pControl->AddLabel(g_localizeStrings.Get(sortMethods[i].second), sortMethods[i].first);
      if (sortMethods[i].first == pSettingInt->GetData())
        found = true;
    }
    if (!found && sortMethods.size())
      pSettingInt->SetData(sortMethods[0].first);
  }
  pControl->SetValue(pSettingInt->GetData());
  delete state;
}

void CGUIWindowSettingsCategory::FillInScrapers(CGUISpinControlEx *pControl, const CStdString& strSelected, const CStdString& strContent)
{
  CFileItemList items;
  if (strContent.Equals("music"))
    CDirectory::GetDirectory("special://xbmc/system/scrapers/music",items,".xml",false);
  else
    CDirectory::GetDirectory("special://xbmc/system/scrapers/video",items,".xml",false);
  int j=0;
  int k=0;
  pControl->Clear();
  for ( int i=0;i<items.Size();++i)
  {
    if (items[i]->m_bIsFolder)
      continue;

    CScraperParser parser;
    if (parser.Load(items[i]->GetPath()))
    {
      if (parser.GetContent() != strContent && !strContent.Equals("music"))
        continue;

      if (parser.GetName().Equals(strSelected) || URIUtils::GetFileName(items[i]->GetPath()).Equals(strSelected))
      {
        if (strContent.Equals("music")) // native strContent would be albums or artists but we're using the same scraper for both
        {
          if (g_guiSettings.GetString("musiclibrary.scraper") != strSelected)
          {
            g_guiSettings.SetString("musiclibrary.scraper", URIUtils::GetFileName(items[i]->GetPath()));

            SScraperInfo info;
            CMusicDatabase database;

            info.strPath = g_guiSettings.GetString("musiclibrary.scraper");
            info.strContent = "albums";
            info.strTitle = parser.GetName();

            database.Open();
            database.SetScraperForPath("musicdb://",info);
            database.Close();
          }
        }
        else if (strContent.Equals("movies"))
          g_guiSettings.SetString("scrapers.moviedefault", URIUtils::GetFileName(items[i]->GetPath()));
        else if (strContent.Equals("tvshows"))
          g_guiSettings.SetString("scrapers.tvshowdefault", URIUtils::GetFileName(items[i]->GetPath()));
        else if (strContent.Equals("musicvideos"))
          g_guiSettings.SetString("scrapers.musicvideodefault", URIUtils::GetFileName(items[i]->GetPath()));
        k = j;
      }
      pControl->AddLabel(parser.GetName(),j++);
    }
  }
  pControl->SetValue(k);
}

void CGUIWindowSettingsCategory::FillInWeatherPlugins(CGUISpinControlEx *pControl, const CStdString& strSelected)
{
  int j=0;
  int k=0;
  pControl->Clear();
  // add our disable option
  pControl->AddLabel(g_localizeStrings.Get(13611), j++);

  CFileItemList items;
  if (CDirectory::GetDirectory("special://home/plugins/weather/", items, "/", false))
  {
    for (int i=0; i<items.Size(); ++i)
    {    
      // create the full path to the plugin
      CStdString plugin;
      CStdString pluginPath = items[i]->GetPath();
      // remove slash at end so we can use the plugins folder as plugin name
      URIUtils::RemoveSlashAtEnd(pluginPath);
      // add default.py to our plugin path to create the full path
      URIUtils::AddFileToFolder(pluginPath, "default.py", plugin);
      if (XFILE::CFile::Exists(plugin))
      {
        // is this the users choice
        if (URIUtils::GetFileName(pluginPath).Equals(strSelected))
          k = j;
        // we want to use the plugins folder as name
        pControl->AddLabel(URIUtils::GetFileName(pluginPath), j++);
      }
    }
  }
  pControl->SetValue(k);
}
