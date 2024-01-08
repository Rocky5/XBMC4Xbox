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

#include "settings/GUIDialogContentSettings.h"
#include "dialogs/GUIDialogPluginSettings.h"
#include "Util.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoScanner.h"
#include "FileSystem/Directory.h"
#include "FileSystem/MultiPathDirectory.h"
#include "GUIWindowManager.h"
#include "utils/ScraperParser.h"
#include "FileItem.h"
#include "settings/Settings.h"
#include "utils/LangCodeExpander.h"
#include "utils/URIUtils.h"
#include "LocalizeStrings.h"

#define CONTROL_CONTENT_TYPE        3
#define CONTROL_SCRAPER_LIST        4
#define CONTROL_SCRAPER_SETTINGS    6
#define CONTROL_START              30

using namespace XFILE;
using namespace std;

CGUIDialogContentSettings::CGUIDialogContentSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_CONTENT_SETTINGS, "DialogContentSettings.xml")
{
  m_bNeedSave = false;
  m_vecItems = new CFileItemList;
}

CGUIDialogContentSettings::~CGUIDialogContentSettings(void)
{
  delete m_vecItems;
}

bool CGUIDialogContentSettings::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_scrapers.clear();
      m_vecItems->Clear();
      CGUIDialogSettings::OnMessage(message);
    }
    break;

  case GUI_MSG_ITEM_SELECT:
    {
      if (message.GetControlId() == CONTROL_SCRAPER_LIST)
      {
        if (!m_info.strContent.IsEmpty())
          m_info = m_scrapers[m_info.strContent][message.GetParam1()];
      }
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
    
    if (iControl == CONTROL_CONTENT_TYPE)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_CONTENT_TYPE);
      g_windowManager.SendMessage(msg);
      int iSelected = msg.GetParam1();

      m_bNeedSave = true;
      CStdString strLabel;
      switch (iSelected)
      {
      case 0: m_info.strContent.Empty();
              m_info.strPath.Empty();
              m_info.strThumb.Empty();
              m_info.strTitle.Empty();
              OnSettingChanged(0);
              SetupPage();
              break;
      case 1: strLabel = g_localizeStrings.Get(20342);
              m_info = FindDefault("movies", g_guiSettings.GetString("scrapers.moviedefault"));
              CreateSettings();
              SetupPage();
              break;
      case 2: strLabel = g_localizeStrings.Get(20343);
              m_info = FindDefault("tvshows", g_guiSettings.GetString("scrapers.tvshowdefault"));
              CreateSettings();
              SetupPage();
              break;
      case 3: strLabel = g_localizeStrings.Get(20389);
              m_info = FindDefault("musicvideos", g_guiSettings.GetString("scrapers.musicvideodefault"));
              CreateSettings();
              SetupPage();
              break;
      case 4: strLabel = g_localizeStrings.Get(132);
              m_info = FindDefault("albums", g_guiSettings.GetString("musiclibrary.scraper"));
              CreateSettings();
              SetupPage();
              break;
      }  
    }
    if (iControl == CONTROL_SCRAPER_LIST)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_SCRAPER_LIST);
      g_windowManager.SendMessage(msg);
      int iSelected = msg.GetParam1();
     
      m_info = m_scrapers[m_info.strContent][iSelected];
      FillListControl();
      SET_CONTROL_FOCUS(30,0);
    }
    CStdString scraperPath = GetScraperDirectory(m_info);
    if (iControl == CONTROL_SCRAPER_SETTINGS)
    {
      if (!scraperPath.IsEmpty() && m_info.settings.LoadSettingsXML(scraperPath + m_info.strPath))
      {
        CGUIDialogPluginSettings::ShowAndGetInput(m_info);
        m_bNeedSave = true;
        return true;
      }
      return false;
    }
    CScraperParser parser;
    CStdString strPath;
    if (!m_info.strContent.IsEmpty())
      strPath=scraperPath;
    if (!strPath.IsEmpty() && parser.Load(strPath + m_info.strPath)
        && parser.HasFunction("GetSettings"))
      CONTROL_ENABLE(CONTROL_SCRAPER_SETTINGS);
    else
      CONTROL_DISABLE(CONTROL_SCRAPER_SETTINGS);
    break;
  }
  return CGUIDialogSettings::OnMessage(message);
}

void CGUIDialogContentSettings::OnWindowLoaded()
{
  CGUIDialogSettings::OnWindowLoaded();
  
  CFileItemList items;
  CStdString baseDir = GetScraperDirectory(m_info);
  if (!baseDir.IsEmpty())
    CDirectory::GetDirectory(baseDir, items, ".xml", false);

  for (int i=0;i<items.Size();++i)
  {
    CScraperParser parser;
    if (parser.Load(items[i]->GetPath()))
    {
      bool IsDefaultScraper = false;

      SScraperInfo info;
      info.strTitle = parser.GetName();
      info.strPath = URIUtils::GetFileName(items[i]->GetPath());
      info.strThumb = parser.GetThumb();
      info.strContent = parser.GetContent();
      info.strLanguage = parser.GetLanguage();
      info.settings = m_scraperSettings;

      if ( info.strPath == g_guiSettings.GetString("musiclibrary.scraper")
        || info.strPath == g_guiSettings.GetString("scrapers.moviedefault")
        || info.strPath == g_guiSettings.GetString("scrapers.tvshowdefault")
        || info.strPath == g_guiSettings.GetString("scrapers.musicvideodefault"))
      {
         IsDefaultScraper = true;
      }

      map<CStdString,vector<SScraperInfo> >::iterator iter=m_scrapers.find(info.strContent);
      if (iter != m_scrapers.end())
      {
        if (IsDefaultScraper)
          iter->second.insert(iter->second.begin(),info);
        else
          iter->second.push_back(info);
      }

      vector<SScraperInfo> vec;
      vec.push_back(info);
      m_scrapers.insert(make_pair(info.strContent,vec));
    }
  }

  // now select the correct scraper
  if (!m_info.strContent.IsEmpty())
  {
    map<CStdString,vector<SScraperInfo> >::iterator iter = m_scrapers.find(m_info.strContent);
    if (iter != m_scrapers.end())
    {
      for (vector<SScraperInfo>::iterator iter2 = iter->second.begin();iter2 != iter->second.end();++iter2)
      {
        if (iter2->strPath == m_info.strPath)
        {
          m_info = *iter2;
          break;
        }
      }
    }
  }
}

void CGUIDialogContentSettings::SetupPage()
{
  CGUIDialogSettings::SetupPage();

  CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_CONTENT_TYPE);
  g_windowManager.SendMessage(msg);
  CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_CONTENT_TYPE);

  if (!m_info.strContent.Equals("albums")) // none does not apply to music
  {
    msg2.SetLabel("<"+g_localizeStrings.Get(231)+">");
    msg2.SetParam1(0);
    g_windowManager.SendMessage(msg2);
  }

  if (m_scrapers.find("movies") != m_scrapers.end())
  {
    msg2.SetLabel(g_localizeStrings.Get(20342));
    msg2.SetParam1(1);
    g_windowManager.SendMessage(msg2);
    if (m_info.strContent.Equals("movies"))
    {
      SET_CONTROL_LABEL(CONTROL_CONTENT_TYPE,g_localizeStrings.Get(20342));
      CONTROL_SELECT_ITEM(CONTROL_CONTENT_TYPE, 1);
    }
  }

  if (m_scrapers.find("tvshows") != m_scrapers.end())
  {
    msg2.SetLabel(g_localizeStrings.Get(20343));
    msg2.SetParam1(2);
    g_windowManager.SendMessage(msg2);
    if (m_info.strContent.Equals("tvshows"))
    {
      CONTROL_SELECT_ITEM(CONTROL_CONTENT_TYPE, 2);
    }
  }
  if (m_scrapers.find("musicvideos") != m_scrapers.end())
  {
    msg2.SetLabel(g_localizeStrings.Get(20389));
    msg2.SetParam1(3);
    g_windowManager.SendMessage(msg2);
    if (m_info.strContent.Equals("musicvideos"))
    {
      SET_CONTROL_LABEL(CONTROL_CONTENT_TYPE,g_localizeStrings.Get(20389));
      CONTROL_SELECT_ITEM(CONTROL_CONTENT_TYPE, 3);
    }
  }
  if (m_scrapers.find("albums") != m_scrapers.end())
  {
    msg2.SetLabel(m_strContentType);
    msg2.SetParam1(4);
    g_windowManager.SendMessage(msg2);
    if (m_info.strContent.Equals("albums"))
    {
      SET_CONTROL_LABEL(CONTROL_CONTENT_TYPE,m_strContentType);
      CONTROL_SELECT_ITEM(CONTROL_CONTENT_TYPE, 3);
    }
  }
  SET_CONTROL_VISIBLE(CONTROL_CONTENT_TYPE);
  // now add them scrapers to the list control
  if (m_info.strContent.IsEmpty() || m_info.strContent.Equals("None"))
  {
    CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SCRAPER_LIST);
    OnMessage(msgReset); 
    CONTROL_DISABLE(CONTROL_SCRAPER_LIST);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_SCRAPER_LIST);
    FillListControl();
  }

  OnSettingChanged(0);
}

void CGUIDialogContentSettings::CreateSettings()
{
  // clear out any old settings
  m_settings.clear();

  if (m_info.strContent.IsEmpty() || m_info.strContent.Equals("None"))
  {
    AddBool(1,20380,&m_bExclude);
  }
  if (m_info.strContent.Equals("movies"))
  {
    AddBool(1,20345,&m_bRunScan);
    AddBool(2,20330,&m_bUseDirNames);
    AddBool(3,20346,&m_bScanRecursive);    
    AddBool(4,20383,&m_bSingleItem, m_bUseDirNames);
    AddBool(5,20432,&m_bUpdate);
  }
  if (m_info.strContent.Equals("tvshows"))
  {
    AddBool(1,20345,&m_bRunScan);
    AddBool(2,20379,&m_bSingleItem);
    AddBool(3,20432,&m_bUpdate);
  }
  if (m_info.strContent.Equals("musicvideos"))
  {
    AddBool(1,20345,&m_bRunScan);
    AddBool(2,20346,&m_bScanRecursive);    
    AddBool(3,20432,&m_bUpdate);
  }
  if (m_info.strContent.Equals("albums"))
  {
    AddBool(1,20345,&m_bRunScan);
  }
}

void CGUIDialogContentSettings::OnSettingChanged(unsigned int num)
{
  // setting has changed - update anything that needs it
  if (num >= m_settings.size()) return;
  SettingInfo &setting = m_settings.at(num);
  OnSettingChanged(setting);
}

void CGUIDialogContentSettings::OnSettingChanged(SettingInfo &setting)
{
  // check and update anything that needs it
  if (setting.id == 1 || setting.id == 2)
  {
    CreateSettings();
    UpdateSetting(1);
    UpdateSetting(2);
    UpdateSetting(3);
    UpdateSetting(4);
    UpdateSetting(5);
  }

  m_bNeedSave = true;
}

void CGUIDialogContentSettings::OnCancel()
{
  m_bNeedSave = false;
}

void CGUIDialogContentSettings::OnInitWindow()
{
  m_bNeedSave = false;

  CGUIDialogSettings::OnInitWindow();
  SET_CONTROL_FOCUS(CONTROL_CONTENT_TYPE,0);
}

void CGUIDialogContentSettings::FillListControl()
{
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SCRAPER_LIST);
  OnMessage(msgReset); 
  int iIndex=0;
  int selectedIndex = 0;
  m_vecItems->Clear();
  for (vector<SScraperInfo>::iterator iter=m_scrapers.find(m_info.strContent)->second.begin();iter!=m_scrapers.find(m_info.strContent)->second.end();++iter)
  {
    CFileItemPtr item(new CFileItem(iter->strTitle));
    item->SetPath(iter->strPath);

    CStdString strLanguage;

    if (iter->strLanguage.Equals("multi"))
      strLanguage = g_localizeStrings.Get(21418);
    else
      g_LangCodeExpander.Lookup(strLanguage, iter->strLanguage); 

    item->SetLabel(iter->strTitle + " (" + strLanguage + ")");

     CStdString baseDir = GetScraperDirectory(*iter);
     if (!baseDir.IsEmpty())
       item->SetThumbnailImage(baseDir + iter->strThumb);

    if (iter->strPath.Equals(m_info.strPath))
    {
      item->Select(true);
      selectedIndex = iIndex;
    }
    m_vecItems->Add(item);
    iIndex++;
  }
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_SCRAPER_LIST, 0, 0, m_vecItems);
  OnMessage(msg);
  CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_SCRAPER_LIST, selectedIndex);
  OnMessage(msg2);
}

CFileItemPtr CGUIDialogContentSettings::GetCurrentListItem(int offset)
{
  int currentItem = -1;
  if( m_info.strContent.IsEmpty())
    return CFileItemPtr();
  for (int i=0;i<m_vecItems->Size();++i )
  {
    if (m_vecItems->Get(i)->IsSelected())
    {
      currentItem = i;
      break;
    }
  }
  if (currentItem == -1) return CFileItemPtr();
  int item = (currentItem + offset) % m_vecItems->Size();
  if (item < 0) item += m_vecItems->Size();
  return m_vecItems->Get(item);
}

bool CGUIDialogContentSettings::ShowForDirectory(const CStdString& strDirectory, SScraperInfo& scraper, VIDEO::SScanSettings& settings, bool& bRunScan)
{
  CVideoDatabase database;
  database.Open();
  database.GetScraperForPath(strDirectory,scraper,settings);
  bool bResult = Show(scraper,settings,bRunScan);
  if (bResult)
    database.SetScraperForPath(strDirectory,scraper,settings);

  return bResult;
}

bool CGUIDialogContentSettings::Show(SScraperInfo& scraper, bool& bRunScan, int iLabel)
{
  VIDEO::SScanSettings dummy;
  dummy.recurse = -1;
  dummy.parent_name = false;
  dummy.parent_name_root = false;
  dummy.noupdate = false;
  return Show(scraper,dummy,bRunScan,iLabel);
}

bool CGUIDialogContentSettings::Show(SScraperInfo& scraper, VIDEO::SScanSettings& settings, bool& bRunScan, int iLabel)
{
  CGUIDialogContentSettings *dialog = (CGUIDialogContentSettings *)g_windowManager.GetWindow(WINDOW_DIALOG_CONTENT_SETTINGS);
  if (!dialog) return false;

  dialog->m_info = scraper;
  if (iLabel != -1) // to switch between albums and artists
    dialog->m_strContentType = g_localizeStrings.Get(iLabel);

  dialog->m_scraperSettings = scraper.settings;
  dialog->m_bRunScan = bRunScan;
  dialog->m_bScanRecursive = (settings.recurse > 0 && !settings.parent_name) || (settings.recurse > 1 && settings.parent_name);
  dialog->m_bUseDirNames   = settings.parent_name;
  dialog->m_bExclude       = scraper.strContent.Equals("None");
  dialog->m_bSingleItem    = settings.parent_name_root;
  dialog->m_bNeedSave = false;
  dialog->m_bUpdate = settings.noupdate;
  dialog->DoModal();
  if (dialog->m_bNeedSave)
  {
    scraper = dialog->m_info;
    settings.noupdate = dialog->m_bUpdate;

    if (scraper.strContent.Equals("tvshows"))
    {
      settings.parent_name = dialog->m_bSingleItem;
      settings.parent_name_root = dialog->m_bSingleItem;
      settings.recurse = 0;
      
      bRunScan = dialog->m_bRunScan;
    }
    else if (scraper.strContent.Equals("movies"))
    {            
      if (dialog->m_bUseDirNames)
      {
        settings.parent_name = true;
        settings.parent_name_root = false;
        settings.recurse = dialog->m_bScanRecursive ? INT_MAX : 1;

        if (dialog->m_bSingleItem)
        {
          settings.parent_name_root = true;
          settings.recurse = 0;
        }
      }
      else
      {
        settings.parent_name = false;
        settings.parent_name_root = false;
        settings.recurse = dialog->m_bScanRecursive ? INT_MAX : 0;
      }

      bRunScan = dialog->m_bRunScan;
    }
    else if (scraper.strContent.Equals("musicvideos"))
    {            
      settings.parent_name = false;
      settings.parent_name_root = false;
      settings.recurse = dialog->m_bScanRecursive ? INT_MAX : 0;

      bRunScan = dialog->m_bRunScan;
    }
    else if (scraper.strContent.Equals("albums"))
    {
      bRunScan = dialog->m_bRunScan;
    }
    else if (scraper.strContent.IsEmpty() || scraper.strContent.Equals("None") )
    {
      if (dialog->m_bExclude)
        scraper.strContent = "None";
      else
        scraper.strContent = "";

      bRunScan = false;
    }

    CStdString baseDir = GetScraperDirectory(scraper);
    if (!baseDir.IsEmpty() && (!scraper.settings.GetPluginRoot() || scraper.settings.GetSettings().IsEmpty()))
    { // load default scraper settings
      if (scraper.settings.LoadSettingsXML(baseDir + scraper.strPath))
        scraper.settings.SaveFromDefault();
    }
    
    return true;
  }
  return false;
}

SScraperInfo CGUIDialogContentSettings::FindDefault(const CStdString& strType, const CStdString& strDefault)
{
  SScraperInfo result = m_scrapers[strType][0]; // default to first
  for (unsigned int i=0;i<m_scrapers[strType].size();++i)
  {
    if (m_scrapers[strType][i].strPath.Equals(strDefault))
    {
      result = m_scrapers[strType][i];
      break;
    }
  }
  
  return result;
}

CStdString CGUIDialogContentSettings::GetScraperDirectory(const SScraperInfo &scraper)
{
  CStdString dir = "video/";

  if (scraper.strContent.Equals("albums"))
    dir = "music/";
  else if (scraper.strContent.Equals("None"))
    return "";
  return "special://xbmc/system/scrapers/" + dir;
}

