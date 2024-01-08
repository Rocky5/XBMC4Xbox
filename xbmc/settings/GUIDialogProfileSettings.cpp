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

#include "settings/GUIDialogProfileSettings.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogKeyboard.h"
#include "dialogs/GUIDialogGamepad.h"
#include "settings/GUIDialogLockSettings.h"
#include "GUIImage.h"
#include "GUIWindowManager.h"
#include "storage/MediaManager.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "GUIPassword.h"
#include "pictures/Picture.h"
#include "dialogs/GUIDialogYesNo.h"
#include "FileSystem/Directory.h"
#include "FileSystem/File.h"
#include "FileItem.h"
#include "LocalizeStrings.h"

using namespace XFILE;

#define CONTROL_PROFILE_IMAGE       2
#define CONTROL_START              30

CGUIDialogProfileSettings::CGUIDialogProfileSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_PROFILE_SETTINGS, "ProfileSettings.xml")
{
  m_bNeedSave = false;
}

CGUIDialogProfileSettings::~CGUIDialogProfileSettings(void)
{
}

bool CGUIDialogProfileSettings::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIDialogSettings::OnMessage(message);
    }
    break;

  case GUI_MSG_CLICKED:
    int iControl = message.GetSenderId();
    if (iControl == 500)
      Close();
    if (iControl == 501)
    {
      m_bNeedSave = false;
      Close();
    }
    break;
  }
  return CGUIDialogSettings::OnMessage(message);
}

void CGUIDialogProfileSettings::OnWindowLoaded()
{
  CGUIDialogSettings::OnWindowLoaded();
  CGUIImage *pImage = (CGUIImage*)GetControl(2);
  m_strDefaultImage = pImage ? pImage->GetFileName() : "";
}

void CGUIDialogProfileSettings::SetupPage()
{
  CGUIDialogSettings::SetupPage();
  SET_CONTROL_LABEL(1000,m_strName);
  SET_CONTROL_LABEL(1001,m_strDirectory);
  CGUIImage *pImage = (CGUIImage*)GetControl(2);
  if (pImage)
    pImage->SetFileName(!m_strThumb.IsEmpty() ? m_strThumb : m_strDefaultImage);
}

void CGUIDialogProfileSettings::CreateSettings()
{
  // clear out any old settings
  m_settings.clear();

  AddButton(1,20093);
  AddButton(2,20065);
  if (!m_bIsDefault && m_bShowDetails)
    AddButton(3,20070);

  if (m_bShowDetails)
    AddButton(4,20066);
  if (!m_bShowDetails && m_locks.mode == LOCK_MODE_EVERYONE && g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
    AddButton(4,20066);

  if (!m_bIsDefault && m_bShowDetails)
  {
    SettingInfo setting;
    setting.id = 5;
    setting.name = g_localizeStrings.Get(20060);
    setting.data = &m_iDbMode;
    setting.type = SettingInfo::SPIN;
    setting.entry.push_back(make_pair(setting.entry.size(), g_localizeStrings.Get(20062)));
    setting.entry.push_back(make_pair(setting.entry.size(), g_localizeStrings.Get(20063)));
    setting.entry.push_back(make_pair(setting.entry.size(), g_localizeStrings.Get(20061)));
    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
      setting.entry.push_back(make_pair(setting.entry.size(), g_localizeStrings.Get(20107)));

    m_settings.push_back(setting);

    SettingInfo setting2;
    setting2.id = 6;
    setting2.name = g_localizeStrings.Get(20094);
    setting2.data = &m_iSourcesMode;
    setting2.type = SettingInfo::SPIN;
    setting2.entry.push_back(make_pair(setting2.entry.size(), g_localizeStrings.Get(20062)));
    setting2.entry.push_back(make_pair(setting2.entry.size(), g_localizeStrings.Get(20063)));
    setting2.entry.push_back(make_pair(setting2.entry.size(), g_localizeStrings.Get(20061)));
    if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
      setting2.entry.push_back(make_pair(setting2.entry.size(), g_localizeStrings.Get(20107)));

    m_settings.push_back(setting2);
  }
  if (m_bIsNewUser)
  {
    SetupPage();
    OnSettingChanged(0); // id=1
    if (!m_bNeedSave)
    {
      OnCancel();
      Close();
      return;
    }
    if (!m_strName.IsEmpty())
    {
      m_strDirectory = URIUtils::AddFileToFolder("profiles", m_strName);
      CUtil::GetFatXQualifiedPath(m_strDirectory);
      CStdString strPath;
      URIUtils::AddFileToFolder("special://masterprofile/",m_strDirectory,strPath);
      CDirectory::Create(strPath);
    }
    CStdString strPath = m_strDirectory;
    OnSettingChanged(2); // id=3
    if (strPath != m_strDirectory)
    {
      CStdString strPath2;
      URIUtils::AddFileToFolder("special://masterprofile/",strPath,strPath2);
      CDirectory::Remove(strPath2);
    }
  }
}

void CGUIDialogProfileSettings::OnSettingChanged(unsigned int num)
{
  // setting has changed - update anything that needs it
  if (num >= m_settings.size()) return;
  OnSettingChanged(m_settings.at(num));
}

void CGUIDialogProfileSettings::OnSettingChanged(SettingInfo &setting)
{
  // check and update anything that needs it
  if (setting.id == 1)
  {
    if (CGUIDialogKeyboard::ShowAndGetInput(m_strName,g_localizeStrings.Get(20093),false))
    {
      m_bNeedSave = true;
      SET_CONTROL_LABEL(1000,m_strName);
    }
  }
  if (setting.id == 2)
  {
    CStdString strThumb;
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    CFileItemList items;
    if (!m_strThumb.IsEmpty())
    {
      CFileItemPtr item(new CFileItem("thumb://Current", false));
      item->SetThumbnailImage(m_strThumb);
      item->SetLabel(g_localizeStrings.Get(20016));
      items.Add(item);
    } 
    CFileItemPtr item(new CFileItem("thumb://None", false));
    item->SetThumbnailImage(m_strDefaultImage);
    item->SetLabel(g_localizeStrings.Get(20018));
    items.Add(item);
    if (CGUIDialogFileBrowser::ShowAndGetImage(items,shares,g_localizeStrings.Get(1030),strThumb) && 
        !strThumb.Equals("thumb://Current"))
    {
      m_bNeedSave = true;
      CFileItem item(strThumb);
      item.SetPath(strThumb);
      m_strThumb = item.GetCachedProfileThumb();
      if (CFile::Exists(m_strThumb))
        CFile::Delete(m_strThumb);

      if (!strThumb.Equals("thumb://None"))
      {
        CPicture pic;
        pic.CreateThumbnail(strThumb, m_strThumb);
      }
      else
        m_strThumb.clear();

      CGUIImage *pImage = (CGUIImage*)GetControl(2);
      if (pImage)
      {
        pImage->SetFileName("");
        pImage->SetInvalid();
        pImage->SetFileName(!m_strThumb.IsEmpty() ? m_strThumb : m_strDefaultImage);
      }
    }
  }
  if (setting.id == 3)
  {
    VECSOURCES shares;
    CMediaSource share;
    share.strName = "Profiles";
    share.strPath = URIUtils::AddFileToFolder("special://masterprofile/","profiles");
    shares.push_back(share);
    CStdString strDirectory;
    if (m_strDirectory == "")
      strDirectory = share.strPath;
    else
      strDirectory = URIUtils::AddFileToFolder("special://masterprofile/",m_strDirectory);
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares,g_localizeStrings.Get(657),strDirectory,true))
    {
      m_strDirectory = strDirectory;
      if (!m_bIsDefault)
        m_strDirectory.erase(0,24);
      m_bNeedSave = true;
      SET_CONTROL_LABEL(1001,m_strDirectory);
    }
  }

  if (setting.id == 4)
  {
    if (m_bShowDetails)
    {
      if (g_settings.GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE && !m_bIsDefault)
      {
        if (CGUIDialogYesNo::ShowAndGetInput(20066,20118,20119,20022))
          g_passwordManager.SetMasterLockMode(false);
        if (g_settings.GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE)
          return;
      }
      if (CGUIDialogLockSettings::ShowAndGetLock(m_locks, m_bIsDefault ? 12360 : 20068, g_settings.GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE || m_bIsDefault))
        m_bNeedSave = true;
    }
    else
    {
      if (CGUIDialogLockSettings::ShowAndGetLock(m_locks, m_bIsDefault ? 12360 : 20068, false, false))
        m_bNeedSave = true;
    }
  }
  if (setting.id > 4)
    m_bNeedSave = true;
}

void CGUIDialogProfileSettings::OnCancel()
{
  m_bNeedSave = false;
}

bool CGUIDialogProfileSettings::ShowForProfile(unsigned int iProfile, bool bDetails)
{
  CGUIDialogProfileSettings *dialog = (CGUIDialogProfileSettings *)g_windowManager.GetWindow(WINDOW_DIALOG_PROFILE_SETTINGS);
  if (!dialog) return false;
  if (iProfile == 0)
    dialog->m_bIsDefault = true;
  else
    dialog->m_bIsDefault = false;
  if (!bDetails && iProfile > g_settings.GetNumProfiles())
    return false;

  dialog->m_bShowDetails = bDetails;

  const CProfile *profile = g_settings.GetProfile(iProfile);

  if (!profile)
  { // defaults
    dialog->m_strName.Empty();
    dialog->m_iDbMode = 2;
    dialog->m_iSourcesMode = 2;

    dialog->m_locks = CProfile::CLock();
    dialog->m_locks.settings = true;
    dialog->m_locks.files = true;

    dialog->m_strDirectory.Empty();
    dialog->m_strThumb.Empty();
    dialog->m_strName = "";
    dialog->m_bIsNewUser = true;
  }
  else
  {
    dialog->m_strName = profile->getName();
    dialog->m_strThumb = profile->getThumb();
    dialog->m_strDirectory = profile->getDirectory();
    dialog->m_iDbMode = profile->canWriteDatabases()?0:1;
    dialog->m_iSourcesMode = profile->canWriteSources()?0:1;
    if (profile->hasDatabases())
      dialog->m_iDbMode += 2;
    if (profile->hasSources())
      dialog->m_iSourcesMode += 2;

    dialog->m_locks = profile->GetLocks();

    dialog->m_bIsNewUser = false;
  }
  dialog->DoModal();
  if (dialog->m_bNeedSave)
  {
    if (iProfile >= g_settings.GetNumProfiles())
    {
      if (dialog->m_strName.IsEmpty() || dialog->m_strDirectory.IsEmpty())
        return false;
      /*CStdString strLabel;
      strLabel.Format(g_localizeStrings.Get(20047),dialog->m_strName);
      if (!CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(20058),strLabel,dialog->m_strDirectory,""))
      {
        CDirectory::Remove(URIUtils::AddFileToFolder(g_settings.GetUserDataFolder(), dialog->m_strDirectory));
        return false;
      }*/

      // check for old profile settings
      CProfile profile;
      g_settings.AddProfile(profile);
      bool bExists = CFile::Exists(URIUtils::AddFileToFolder("special://masterprofile/",
                                                          dialog->m_strDirectory+"/guisettings.xml"));

      if (bExists)
        if (!CGUIDialogYesNo::ShowAndGetInput(20058,20104,20105,20022))
          bExists = false;

      if (!bExists)
      {
        // save new profile guisettings
        if (CGUIDialogYesNo::ShowAndGetInput(20058,20048,20102,20022,20044,20064))
        {
          CFile::Cache(URIUtils::AddFileToFolder("special://masterprofile/","guisettings.xml"),
                       URIUtils::AddFileToFolder("special://masterprofile/",
                                              dialog->m_strDirectory+"/guisettings.xml"));
        }
        else
        {
          // create some new settings
          CGUISettings localSettings;
          CStdString path = URIUtils::AddFileToFolder("special://masterprofile/", dialog->m_strDirectory);
          path = URIUtils::AddFileToFolder(path, "guisettings.xml");
          g_settings.SaveSettings(path, &localSettings);
        }
      }

      bExists = CFile::Exists(URIUtils::AddFileToFolder("special://masterprofile/",
                                                     dialog->m_strDirectory+"/sources.xml"));
      if (bExists)
        if (!CGUIDialogYesNo::ShowAndGetInput(20058,20106,20105,20022))
          bExists = false;

      if (!bExists)
      {
        if ((dialog->m_iSourcesMode & 2) == 2)
          if (CGUIDialogYesNo::ShowAndGetInput(20058,20071,20102,20022,20044,20064))
          {
            CFile::Cache(URIUtils::AddFileToFolder("special://masterprofile/","sources.xml"),
                         URIUtils::AddFileToFolder("special://masterprofile/",
                         dialog->m_strDirectory+"/sources.xml"));
          }
      }
    }

    /*if (!dialog->m_bIsNewUser)
      if (!CGUIDialogYesNo::ShowAndGetInput(20067,20103,20022,20022))
        return false;*/

    CProfile *profile = g_settings.GetProfile(iProfile);
    assert(profile);
    profile->setName(dialog->m_strName);
    profile->setDirectory(dialog->m_strDirectory);
    profile->setThumb(dialog->m_strThumb);
    profile->setWriteDatabases(!((dialog->m_iDbMode & 1) == 1));
    profile->setWriteSources(!((dialog->m_iSourcesMode & 1) == 1));
    profile->setDatabases((dialog->m_iDbMode & 2) == 2);
    profile->setSources((dialog->m_iSourcesMode & 2) == 2);

    profile->SetLocks(dialog->m_locks);

    g_settings.SaveProfiles(PROFILES_FILE);
    return true;
  }

  return !dialog->m_bNeedSave;
}

void CGUIDialogProfileSettings::OnInitWindow()
{
  m_bNeedSave = false;

  CGUIDialogSettings::OnInitWindow();
}

