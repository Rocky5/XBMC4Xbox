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

#include "GUIWindowVideoFiles.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "pictures/Picture.h"
#include "utils/IMDB.h"
#include "GUIInfoManager.h"
#include "playlists/PlayListFactory.h"
#include "Application.h"
#include "NfoFile.h"
#include "PlayListPlayer.h"
#include "GUIPassword.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "settings/GUIDialogContentSettings.h"
#include "video/dialogs/GUIDialogVideoScan.h"
#include "FileSystem/MultiPathDirectory.h"
#include "utils/RegExp.h"
#include "GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "FileSystem/File.h"
#include "playlists/PlayList.h"
#include "utils/log.h"

using namespace std;
using namespace MEDIA_DETECT;
using namespace XFILE;
using namespace PLAYLIST;
using namespace VIDEO;

#define CONTROL_LIST              50
#define CONTROL_THUMBS            51

#define CONTROL_PLAY_DVD           6
#define CONTROL_STACK              7
#define CONTROL_BTNSCAN            8
#define CONTROL_IMDB               9
#define CONTROL_BTNPLAYLISTS      13

CGUIWindowVideoFiles::CGUIWindowVideoFiles()
: CGUIWindowVideoBase(WINDOW_VIDEO_FILES, "MyVideo.xml")
{
  m_stackingAvailable = true;
  m_cleaningAvailable = true;
}

CGUIWindowVideoFiles::~CGUIWindowVideoFiles()
{
}

bool CGUIWindowVideoFiles::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      // is this the first time accessing this window?
      if (m_vecItems->GetPath() == "?" && message.GetStringParam().IsEmpty())
        message.SetStringParam(g_settings.m_defaultVideoSource);

      return CGUIWindowVideoBase::OnMessage(message);
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      //if (iControl == CONTROL_BTNSCAN)
      //{
      //  OnScan();
     // }
      /*else*/ if (iControl == CONTROL_STACK)
      {
        // toggle between the following states:
        //   0 : no stacking
        //   1 : stacking
        g_settings.m_iMyVideoStack++;

        if (g_settings.m_iMyVideoStack > STACK_SIMPLE)
          g_settings.m_iMyVideoStack = STACK_NONE;

        g_settings.Save();
        UpdateButtons();
        Update( m_vecItems->GetPath() );
      }
      else if (iControl == CONTROL_BTNPLAYLISTS)
      {
        if (!m_vecItems->GetPath().Equals("special://videoplaylists/"))
        {
          CStdString strParent = m_vecItems->GetPath();
          UpdateButtons();
          Update("special://videoplaylists/");
        }
      }
      // list/thumb panel
      else if (m_viewControl.HasControl(iControl))
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        const CFileItemPtr pItem = m_vecItems->Get(iItem);

        // use play button to add folders of items to temp playlist
        if (iAction == ACTION_PLAYER_PLAY && pItem->m_bIsFolder && !pItem->IsParentFolder())
        {
          if (pItem->IsDVD())
            return CAutorun::PlayDisc();

          if (pItem->m_bIsShareOrDrive)
            return false;
          // if playback is paused or playback speed != 1, return
          if (g_application.IsPlayingVideo())
          {
            if (g_application.m_pPlayer->IsPaused()) return false;
            if (g_application.GetPlaySpeed() != 1) return false;
          }
          // not playing, or playback speed == 1
          // queue folder or playlist into temp playlist and play
          if ((pItem->m_bIsFolder && !pItem->IsParentFolder()) || pItem->IsPlayList())
            PlayItem(iItem);
          return true;
        }
      }
    }
  }
  return CGUIWindowVideoBase::OnMessage(message);
}

bool CGUIWindowVideoFiles::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_TOGGLE_WATCHED)
  {
    CFileItemPtr pItem = m_vecItems->Get(m_viewControl.GetSelectedItem());
    if (pItem->IsParentFolder())
      return false;

    if (pItem && pItem->GetOverlayImage().Equals("OverlayWatched.png"))
      return OnContextButton(m_viewControl.GetSelectedItem(),CONTEXT_BUTTON_MARK_UNWATCHED);
    else
      return OnContextButton(m_viewControl.GetSelectedItem(),CONTEXT_BUTTON_MARK_WATCHED);
  }
  return CGUIWindowVideoBase::OnAction(action);
}

void CGUIWindowVideoFiles::UpdateButtons()
{
  CGUIWindowVideoBase::UpdateButtons();
  const CGUIControl *stack = GetControl(CONTROL_STACK);
  if (stack)
  {
    if (m_stackingAvailable)
    {
      CONTROL_ENABLE(CONTROL_STACK);
      if (stack->GetControlType() == CGUIControl::GUICONTROL_RADIO)
      {
        SET_CONTROL_SELECTED(GetID(), CONTROL_STACK, g_settings.m_iMyVideoStack == STACK_SIMPLE);
        SET_CONTROL_LABEL(CONTROL_STACK, 14000);  // Stack
      }
      else
      {
        SET_CONTROL_LABEL(CONTROL_STACK, g_settings.m_iMyVideoStack + 14000);
      }
    }
    else
    {
      if (stack->GetControlType() == CGUIControl::GUICONTROL_RADIO)
      {
        SET_CONTROL_LABEL(CONTROL_STACK, 14000);  // Stack
      }

      CONTROL_DISABLE(CONTROL_STACK);
    }
  }
}

bool CGUIWindowVideoFiles::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (!CGUIWindowVideoBase::GetDirectory(strDirectory, items))
    return false;

  SScraperInfo info2;

  m_stackingAvailable = true;
  m_cleaningAvailable = true;

  if ((m_database.GetScraperForPath(strDirectory,info2) && info2.strContent.Equals("tvshows")) || items.IsTuxBox() || items.IsPlugin() || items.IsRSS() || items.IsInternetStream())
  { // dont stack or clean strings in tv dirs
    m_stackingAvailable = false;
    m_cleaningAvailable = false;
  }
  else if (!items.IsStack() && g_settings.m_iMyVideoStack != STACK_NONE)
    items.Stack();

  if ((!info2.strContent.IsEmpty() && !info2.strContent.Equals("None")) && items.GetContent().IsEmpty())
    items.SetContent(info2.strContent.c_str());
  else
    items.SetContent("files");

  items.SetThumbnailImage("");
  items.SetVideoThumb();

  return true;
}

void CGUIWindowVideoFiles::OnPrepareFileItems(CFileItemList &items)
{
  CGUIWindowVideoBase::OnPrepareFileItems(items);
  if (g_guiSettings.GetBool("myvideos.cleanstrings") && !items.IsVirtualDirectoryRoot())
  {
    for (int i = 0; i < (int)items.Size(); ++i)
    {
      CFileItemPtr item = items[i];
      if ((item->m_bIsFolder && !URIUtils::IsInArchive(item->GetPath())) || m_cleaningAvailable)
        item->CleanString();
    }
  }
}

bool CGUIWindowVideoFiles::OnClick(int iItem)
{
  return CGUIWindowVideoBase::OnClick(iItem);
}

bool CGUIWindowVideoFiles::OnPlayMedia(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() ) return false;
  CFileItemPtr pItem = m_vecItems->Get(iItem);

  if (pItem->IsDVD())
    return CAutorun::PlayDisc();

  if (pItem->m_bIsShareOrDrive)
  	return false;

  AddFileToDatabase(pItem.get());

  return CGUIWindowVideoBase::OnPlayMedia(iItem);
}

void CGUIWindowVideoFiles::AddFileToDatabase(const CFileItem* pItem)
{
  if (!pItem->IsVideo()) return ;
  if ( pItem->IsNFO()) return ;
  if ( pItem->IsPlayList()) return ;

  /* subtitles are assumed not to exist here, if we need it  */
  /* player should update this when it figures out if it has */
  m_database.AddFile(pItem->GetPath());

  if (pItem->IsStack())
  { // get stacked items
    // TODO: This should be removed as soon as we no longer need the individual
    // files for saving settings etc.
    vector<CStdString> movies;
    GetStackedFiles(pItem->GetPath(), movies);
    for (unsigned int i = 0; i < movies.size(); i++)
    {
      CFileItem item(movies[i], false);
      m_database.AddFile(item.GetPath());
    }
  }
}

bool CGUIWindowVideoFiles::OnUnAssignContent(int iItem, int label1, int label2, int label3)
{
  bool bCanceled;
  if (CGUIDialogYesNo::ShowAndGetInput(label1,label2,label3,20022,bCanceled))
  {
    m_database.RemoveContentForPath(m_vecItems->Get(iItem)->GetPath(),m_dlgProgress);
    CUtil::DeleteVideoDatabaseDirectoryCache();
    return true;
  }
  else
  {
    if (!bCanceled)
    {
      SScraperInfo info;
      SScanSettings settings;
      m_database.SetScraperForPath(m_vecItems->Get(iItem)->GetPath(),info,settings);
    }
  }

  return false;
}

void CGUIWindowVideoFiles::OnAssignContent(int iItem, int iFound, SScraperInfo& info, SScanSettings& settings)
{
  if (!g_guiSettings.GetBool("videolibrary.enabled")) 
    return;
 
  CFileItemPtr item = m_vecItems->Get(iItem);
  bool bScan=false;
  if (iFound == 0)
  {
    m_database.GetScraperForPath(item->GetPath(),info,settings,iFound);
  }
  SScraperInfo info2 = info;
  SScanSettings settings2 = settings;
  
  if (CGUIDialogContentSettings::Show(info2, settings2, bScan))
  {
    if((info2.strContent.IsEmpty() || info2.strContent.Equals("None")) && 
      (!info.strContent.IsEmpty() && !info.strContent.Equals("None")))
    {
      OnUnAssignContent(iItem,20375,20340,20341);
    }
    if (!info.strContent.IsEmpty()      && 
        !info2.strContent.IsEmpty()     &&
        !info.strContent.Equals("None") && 
       (info2.strContent != info.strContent ||
        !info.strPath.Equals(info2.strPath)))
    {
      if (OnUnAssignContent(iItem,20442,20443,20444))
        bScan = true;
    }

    m_database.Open();
    m_database.SetScraperForPath(item->GetPath(),info2,settings2);
    m_database.Close();

    if (bScan)
    {
      GetScraperForItem(item.get(),info2,settings2);
      OnScan(item->GetPath(),info2,settings2);
    }
  }
}

void CGUIWindowVideoFiles::GetStackedDirectory(const CStdString &strPath, CFileItemList &items)
{
  items.Clear();
  m_rootDir.GetDirectory(strPath, items);
  items.Stack();
}

void CGUIWindowVideoFiles::LoadPlayList(const CStdString& strPlayList)
{
  // load a playlist like .m3u, .pls
  // first get correct factory to load playlist
  auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(strPlayList));
  if ( NULL != pPlayList.get())
  {
    // load it
    if (!pPlayList->Load(strPlayList))
    {
      CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
      return ; //hmmm unable to load playlist?
    }
  }

  int iSize = pPlayList->size();
  if (g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, PLAYLIST_VIDEO))
  {
    if (m_guiState.get())
      m_guiState->SetPlaylistDirectory("playlistvideo://");
    // activate the playlist window if its not activated yet
    if (GetID() == g_windowManager.GetActiveWindow() && iSize > 1)
    {
      g_windowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    }
  }
}

void CGUIWindowVideoFiles::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  CGUIDialogVideoScan *pScanDlg = (CGUIDialogVideoScan *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
  if (item)
  {
    // are we in the playlists location?
    if (m_vecItems->IsVirtualDirectoryRoot())
    {
      // get the usual shares, and anything for all media windows
      CGUIDialogContextMenu::GetContextButtons("video", item, buttons);
      CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
      // add scan button somewhere here
      if (pScanDlg && pScanDlg->IsScanning())
        buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353);  // Stop Scanning
      if (g_guiSettings.GetBool("videolibrary.enabled") && !item->IsDVD() && item->GetPath() != "add" &&
         (g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser))
      {
        CGUIDialogVideoScan *pScanDlg = (CGUIDialogVideoScan *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
        if (!pScanDlg || (pScanDlg && !pScanDlg->IsScanning()))
          buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20333);
        CVideoDatabase database;
        database.Open();
        SScraperInfo info;

        if (item && database.GetScraperForPath(item->GetPath(),info))
        {
          if (!info.strPath.IsEmpty() && !info.strContent.IsEmpty())
            if (!pScanDlg || (pScanDlg && !pScanDlg->IsScanning()))
              buttons.Add(CONTEXT_BUTTON_SCAN, 13349);
        }
      }
    }
    else
    {
      CGUIWindowVideoBase::GetContextButtons(itemNumber, buttons);
      if (!item->GetPropertyBOOL("pluginreplacecontextitems"))
      {
        // Movie Info button
        if (pScanDlg && pScanDlg->IsScanning())
          buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353);
        if (g_guiSettings.GetBool("videolibrary.enabled") && 
          (g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser))
        {
          SScraperInfo info;
          VIDEO::SScanSettings settings;
          int iFound = GetScraperForItem(item.get(), info, settings);

          int infoString = 13346;
          if (info.strContent.Equals("tvshows"))
            infoString = item->m_bIsFolder ? 20351 : 20352;
          if (info.strContent.Equals("musicvideos"))
            infoString = 20393;

          if (item->m_bIsFolder && !item->IsParentFolder())
          {
            if (!pScanDlg || (pScanDlg && !pScanDlg->IsScanning()))
              if (!item->IsPlayList() && !item->IsLiveTV())
                buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20333);
            if (iFound==0)
            { // scraper not set - allow movie information or set content
              CStdString strPath(item->GetPath());
              URIUtils::AddSlashAtEnd(strPath);
              if ((info.strContent.Equals("movies") && m_database.HasMovieInfo(strPath)) ||
                  (info.strContent.Equals("tvshows") && m_database.HasTvShowInfo(strPath)))
                buttons.Add(CONTEXT_BUTTON_INFO, infoString);
            }
            else
            { // scraper found - allow movie information, scan for new content, or set different type of content
              if (!info.strContent.Equals("musicvideos"))
                buttons.Add(CONTEXT_BUTTON_INFO, infoString);
              if (!info.strPath.IsEmpty() && !info.strContent.IsEmpty())
                if (!pScanDlg || (pScanDlg && !pScanDlg->IsScanning()))
                  buttons.Add(CONTEXT_BUTTON_SCAN, 13349);
            }
          }
          else
          {
            // single file
            buttons.Add(CONTEXT_BUTTON_INFO, infoString);

            m_database.Open();
            if (!item->IsLiveTV())
            {
              if (!m_database.HasMovieInfo(item->GetPath()) 
              &&  !m_database.HasEpisodeInfo(item->GetPath()))
                buttons.Add(CONTEXT_BUTTON_ADD_TO_LIBRARY, 527); // Add to Database
              else  
                buttons.Add(CONTEXT_BUTTON_DELETE, 646); // Remove from Database
            }
            m_database.Close();
          }
        }
      }
      if (!item->IsParentFolder())
      {
        if ((m_vecItems->GetPath().Equals("special://videoplaylists/")) || 
             g_guiSettings.GetBool("filelists.allowfiledeletion"))
        { // video playlists or file operations are allowed
          if (!item->IsReadOnly())
          {
            buttons.Add(CONTEXT_BUTTON_DELETE, 117);
            buttons.Add(CONTEXT_BUTTON_RENAME, 118);
          }
        }
      }
      if (m_vecItems->IsPlugin() && item->HasVideoInfoTag() && !item->GetPropertyBOOL("pluginreplacecontextitems"))
        buttons.Add(CONTEXT_BUTTON_INFO,13346); // only movie information for now

      if (item->m_bIsFolder)
      {
        // Have both options for folders since we don't know whether all childs are watched/unwatched
        buttons.Add(CONTEXT_BUTTON_MARK_UNWATCHED, 16104); //Mark as UnWatched
        buttons.Add(CONTEXT_BUTTON_MARK_WATCHED, 16103);   //Mark as Watched
      }
      else
      if (item->GetOverlayImage().Equals("OverlayWatched.png"))
        buttons.Add(CONTEXT_BUTTON_MARK_UNWATCHED, 16104); //Mark as UnWatched
      else
        buttons.Add(CONTEXT_BUTTON_MARK_WATCHED, 16103);   //Mark as Watched
    }
  }
  else
  {
    if (pScanDlg && pScanDlg->IsScanning())
      buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353);	// Stop Scanning
  }
  if(!(item && item->GetPropertyBOOL("pluginreplacecontextitems")))
  {
    if (!m_vecItems->IsVirtualDirectoryRoot())
      buttons.Add(CONTEXT_BUTTON_SWITCH_MEDIA, 523);

    CGUIWindowVideoBase::GetNonContextButtons(itemNumber, buttons);
  }
}

bool CGUIWindowVideoFiles::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  if ( m_vecItems->IsVirtualDirectoryRoot() && item)
  {
    if (CGUIDialogContextMenu::OnContextButton("video", item, button))
    {
      //TODO should we search DB for entries from plugins?
      if (button == CONTEXT_BUTTON_REMOVE_SOURCE && !item->IsPlugin()
          && !item->IsLiveTV() &&!item->IsRSS())
      {
        OnUnAssignContent(itemNumber,20375,20340,20341);
      }
      Update("");
      return true;
    }
  }

  switch (button)
  {
  case CONTEXT_BUTTON_SWITCH_MEDIA:
    CGUIDialogContextMenu::SwitchMedia("video", m_vecItems->GetPath());
    return true;

  case CONTEXT_BUTTON_SET_CONTENT:
    {
      SScraperInfo info;
      SScanSettings settings;
      if (item->HasVideoInfoTag())  // files view shouldn't need this check I think?
        m_database.GetScraperForPath(item->GetVideoInfoTag()->m_strPath, info, settings);
      else
        m_database.GetScraperForPath(item->GetPath(), info, settings);
      OnAssignContent(itemNumber,0, info, settings);
      return true;
    }

  case CONTEXT_BUTTON_ADD_TO_LIBRARY:
    AddToDatabase(itemNumber);
    return true;

  default:
    break;
  }
  return CGUIWindowVideoBase::OnContextButton(itemNumber, button);
}

void CGUIWindowVideoFiles::OnQueueItem(int iItem)
{
  CGUIWindowVideoBase::OnQueueItem(iItem);
}

CStdString CGUIWindowVideoFiles::GetStartFolder(const CStdString &dir)
{
  SetupShares();
  VECSOURCES shares;
  m_rootDir.GetSources(shares);
  bool bIsSourceName = false;
  int iIndex = CUtil::GetMatchingSource(dir, shares, bIsSourceName);
  if (iIndex > -1)
  {
    if (iIndex < (int)shares.size() && shares[iIndex].m_iHasLock == 2)
    {
      CFileItem item(shares[iIndex]);
      if (!g_passwordManager.IsItemUnlocked(&item,"video"))
        return "";
    }
    // set current directory to matching share
    if (bIsSourceName)
      return shares[iIndex].strPath;
    return dir;
  }
  return CGUIWindowVideoBase::GetStartFolder(dir);
}
