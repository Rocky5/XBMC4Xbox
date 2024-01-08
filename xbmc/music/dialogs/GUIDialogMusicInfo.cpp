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

#include "music/dialogs/GUIDialogMusicInfo.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "GUIImage.h"
#include "pictures/Picture.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "GUIPassword.h"
#include "music/MusicDatabase.h"
#include "music/LastFmManager.h"
#include "music/tags/MusicInfoTag.h"
#include "URL.h"
#include "FileSystem/File.h"
#include "FileItem.h"
#include "video/VideoInfoTag.h"
#include "storage/MediaManager.h"
#include "FileSystem/Directory.h"
#include "utils/AsyncFileCopy.h"
#include "settings/AdvancedSettings.h"
#include "LocalizeStrings.h"
#include "utils/log.h"

using namespace XFILE;

#define CONTROL_ALBUM           20
#define CONTROL_ARTIST          21
#define CONTROL_DATE            22
#define CONTROL_RATING          23
#define CONTROL_GENRE           24
#define CONTROL_MOODS           25
#define CONTROL_STYLES          26

#define CONTROL_IMAGE            3
#define CONTROL_TEXTAREA         4

#define CONTROL_BTN_TRACKS       5
#define CONTROL_BTN_REFRESH      6
#define CONTROL_BTN_GET_THUMB   10
#define CONTROL_BTN_LASTFM      11
#define  CONTROL_BTN_GET_FANART 12

#define CONTROL_LIST            50

CGUIWindowMusicInfo::CGUIWindowMusicInfo(void)
    : CGUIDialog(WINDOW_MUSIC_INFO, "DialogAlbumInfo.xml")
    , m_albumItem(new CFileItem)
{
  m_bRefresh = false;
  m_albumSongs = new CFileItemList;
}

CGUIWindowMusicInfo::~CGUIWindowMusicInfo(void)
{
  delete m_albumSongs;
}

bool CGUIWindowMusicInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIMessage message(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
      OnMessage(message);
      m_albumSongs->Clear();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      m_bViewReview = true;
      m_bRefresh = false;
        RefreshThumb();
      Update();
      return true;
    }
    break;


  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTN_REFRESH)
      {
        m_bRefresh = true;
        Close();
        return true;
      }
      else if (iControl == CONTROL_BTN_GET_THUMB)
      {
        OnGetThumb();
      }
      else if (iControl == CONTROL_BTN_TRACKS)
      {
        m_bViewReview = !m_bViewReview;
        Update();
      }
      else if (iControl == CONTROL_LIST)
      {
        int iAction = message.GetParam1();
        if (m_bArtistInfo && (ACTION_SELECT_ITEM == iAction || ACTION_MOUSE_LEFT_CLICK == iAction))
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
          g_windowManager.SendMessage(msg);
          int iItem = msg.GetParam1();
          if (iItem < 0 || iItem >= (int)m_albumSongs->Size())
            break;
          CFileItemPtr item = m_albumSongs->Get(iItem);
          OnSearch(item.get());
          return true;
        }
      }
      else if (iControl == CONTROL_BTN_LASTFM)
      {
        CStdString strArtist = m_album.strArtist;
        CURL::Encode(strArtist);
        CStdString strLink;
        strLink.Format("lastfm://artist/%s/similarartists", strArtist.c_str());
        CURL url(strLink);
        CLastFmManager::GetInstance()->ChangeStation(url);
      }
      else if (iControl == CONTROL_BTN_GET_FANART)
      {
        OnGetFanart();
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIWindowMusicInfo::SetAlbum(const CAlbum& album, const CStdString &path)
{
  m_album = album;
  SetSongs(m_album.songs);
  *m_albumItem = CFileItem(path, true);
  m_albumItem->GetMusicInfoTag()->SetAlbum(m_album.strAlbum);
  m_albumItem->GetMusicInfoTag()->SetAlbumArtist(m_album.strArtist);
  m_albumItem->GetMusicInfoTag()->SetArtist(m_album.strArtist);
  m_albumItem->GetMusicInfoTag()->SetYear(m_album.iYear);
  m_albumItem->GetMusicInfoTag()->SetLoaded(true);
  m_albumItem->GetMusicInfoTag()->SetRating('0' + (m_album.iRating + 1) / 2);
  m_albumItem->GetMusicInfoTag()->SetGenre(m_album.strGenre);
  CMusicDatabase::SetPropertiesFromAlbum(*m_albumItem,m_album);
  m_albumItem->SetMusicThumb();
  // set the artist thumb
  CFileItem artist(m_album.strArtist);
  artist.SetCachedArtistThumb();
  if (CFile::Exists(artist.GetThumbnailImage()))
    m_albumItem->SetProperty("artistthumb", artist.GetThumbnailImage());
  m_hasUpdatedThumb = false;
  m_bArtistInfo = false;
  m_albumSongs->SetContent("albums");
}

void CGUIWindowMusicInfo::SetArtist(const CArtist& artist, const CStdString &path)
{
  m_artist = artist;
  SetDiscography();
  *m_albumItem = CFileItem(path, true);
  m_albumItem->SetLabel(artist.strArtist);
  m_albumItem->GetMusicInfoTag()->SetAlbumArtist(m_artist.strArtist);
  m_albumItem->GetMusicInfoTag()->SetArtist(m_artist.strArtist);
  m_albumItem->GetMusicInfoTag()->SetLoaded(true);
  m_albumItem->GetMusicInfoTag()->SetGenre(m_artist.strGenre);
  CMusicDatabase::SetPropertiesFromArtist(*m_albumItem,m_artist);
  CStdString strFanart = m_albumItem->GetCachedFanart();
  if (CFile::Exists(strFanart))
    m_albumItem->SetProperty("fanart_image",strFanart);
  m_albumItem->SetCachedArtistThumb();
  m_hasUpdatedThumb = false;
  m_bArtistInfo = true;
  m_albumSongs->SetContent("artists");
}

void CGUIWindowMusicInfo::SetSongs(const VECSONGS &songs)
{
  m_albumSongs->Clear();
  for (unsigned int i = 0; i < songs.size(); i++)
  {
    const CSong& song = songs[i];
    CFileItemPtr item(new CFileItem(song));
    m_albumSongs->Add(item);
  }
}

void CGUIWindowMusicInfo::SetDiscography()
{
  m_albumSongs->Clear();
  CMusicDatabase database;
  database.Open();

  for (unsigned int i=0;i<m_artist.discography.size();++i)
  {
    CFileItemPtr item(new CFileItem(m_artist.discography[i].first));
    item->SetLabel2(m_artist.discography[i].second);
    long idAlbum = database.GetAlbumByName(item->GetLabel(),m_artist.strArtist);
    CStdString strThumb;
    if (idAlbum != -1) // we need this slight stupidity to get correct case for the album name
      database.GetAlbumThumb(idAlbum,strThumb);

    if (!strThumb.IsEmpty() && CFile::Exists(strThumb))
      item->SetThumbnailImage(strThumb);
    else
      item->SetThumbnailImage("DefaultAlbumCover.png");

    m_albumSongs->Add(item);
  }
}

void CGUIWindowMusicInfo::Update()
{
  if (m_bArtistInfo)
  {
    CONTROL_ENABLE(CONTROL_BTN_GET_FANART);
    SetLabel(CONTROL_ARTIST, m_artist.strArtist );
    SetLabel(CONTROL_GENRE, m_artist.strGenre);
    SetLabel(CONTROL_MOODS, m_artist.strMoods);
    SetLabel(CONTROL_STYLES, m_artist.strStyles );
    if (m_bViewReview)
    {
      SET_CONTROL_VISIBLE(CONTROL_TEXTAREA);
      SET_CONTROL_HIDDEN(CONTROL_LIST);
      SetLabel(CONTROL_TEXTAREA, m_artist.strBiography);
      SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 21888);
    }
    else
    {
      SET_CONTROL_VISIBLE(CONTROL_LIST);
      if (GetControl(CONTROL_LIST))
      {
        SET_CONTROL_HIDDEN(CONTROL_TEXTAREA);
        CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_albumSongs);
        OnMessage(message);
      }
      else
        CLog::Log(LOGERROR, "Out of date skin - needs list with id %i", CONTROL_LIST);
      SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 21887);
    }
  }
  else
  {
    CONTROL_DISABLE(CONTROL_BTN_GET_FANART);
    SetLabel(CONTROL_ALBUM, m_album.strAlbum );
    SetLabel(CONTROL_ARTIST, m_album.strArtist );
    CStdString date; date.Format("%d", m_album.iYear);
    SetLabel(CONTROL_DATE, date );

    CStdString strRating;
    if (m_album.iRating > 0)
      strRating.Format("%i/9", m_album.iRating);
    SetLabel(CONTROL_RATING, strRating );

    SetLabel(CONTROL_GENRE, m_album.strGenre);
    SetLabel(CONTROL_MOODS, m_album.strMoods);
    SetLabel(CONTROL_STYLES, m_album.strStyles );

    if (m_bViewReview)
    {
      SET_CONTROL_VISIBLE(CONTROL_TEXTAREA);
      SET_CONTROL_HIDDEN(CONTROL_LIST);
      SetLabel(CONTROL_TEXTAREA, m_album.strReview);
      SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 182);
    }
    else
    {
      SET_CONTROL_VISIBLE(CONTROL_LIST);
      if (GetControl(CONTROL_LIST))
      {
        SET_CONTROL_HIDDEN(CONTROL_TEXTAREA);
        CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_albumSongs);
        OnMessage(message);
      }
      else
        CLog::Log(LOGERROR, "Out of date skin - needs list with id %i", CONTROL_LIST);
      SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 183);
    }
  }
  // update the thumbnail
  const CGUIControl* pControl = GetControl(CONTROL_IMAGE);
  if (pControl)
  {
    CGUIImage* pImageControl = (CGUIImage*)pControl;
    pImageControl->FreeResources();
    pImageControl->SetFileName(m_albumItem->GetThumbnailImage());
  }

  // disable the GetThumb button if the user isn't allowed it
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_GET_THUMB, g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser);

  if (!m_album.strArtist.IsEmpty() && CLastFmManager::GetInstance()->IsLastFmEnabled())
  {
    SET_CONTROL_VISIBLE(CONTROL_BTN_LASTFM);
  }
  else
  {
    SET_CONTROL_HIDDEN(CONTROL_BTN_LASTFM);
  }

}

void CGUIWindowMusicInfo::SetLabel(int iControl, const CStdString& strLabel)
{
  if (strLabel.IsEmpty())
  {
    SET_CONTROL_LABEL(iControl, (iControl == CONTROL_TEXTAREA) ? (m_bArtistInfo?547:414) : 416);
  }
  else
  {
    SET_CONTROL_LABEL(iControl, strLabel);
  }
}

void CGUIWindowMusicInfo::Render()
{
  CGUIDialog::Render();
}


void CGUIWindowMusicInfo::RefreshThumb()
{
  CStdString thumbImage = m_albumItem->GetThumbnailImage();
  if (!m_albumItem->HasThumbnail())
  {
    if (m_bArtistInfo)
      thumbImage = m_albumItem->GetCachedArtistThumb();
    else
      thumbImage = CUtil::GetCachedAlbumThumb(m_album.strAlbum, m_album.strArtist);

    if (!CFile::Exists(thumbImage))
    {
      DownloadThumbnail(thumbImage);
      m_hasUpdatedThumb = true;
    }
  }
  if (!CFile::Exists(thumbImage) )
    thumbImage.Empty();

  m_albumItem->SetThumbnailImage(thumbImage);
}

bool CGUIWindowMusicInfo::NeedRefresh() const
{
  return m_bRefresh;
}

int CGUIWindowMusicInfo::DownloadThumbnail(const CStdString &thumbFile, bool bMultiple)
{
  // Download image and save as thumbFile
  if (m_bArtistInfo)
  {
    if (m_artist.thumbURL.m_url.size() == 0)
      return 0;

    int iResult=0;
    int iMax = 1;
    if (bMultiple)
      iMax = INT_MAX;
    for (unsigned int i=0;i<m_artist.thumbURL.m_url.size()&&iResult<iMax;++i)
    {
      CStdString strThumb;

      if (bMultiple)
        strThumb.Format("%s%i.tbn",thumbFile.c_str(),i);
      else
        strThumb = thumbFile;
      if (CScraperUrl::DownloadThumbnail(strThumb,m_artist.thumbURL.m_url[i]))
        iResult++;
    }
    return iResult;
  }
  else
  {
    if (m_album.thumbURL.m_url.size() == 0)
      return 0;

    int iResult=0;
    int iMax = 1;
    if (bMultiple)
      iMax = INT_MAX;
    for (unsigned int i=0;i<m_album.thumbURL.m_url.size() && iResult<iMax;++i)
    {
      CStdString strThumb;
      if (bMultiple)
        strThumb.Format("%s%i.tbn",thumbFile.c_str(),i);
      else
        strThumb = thumbFile;
      if (CScraperUrl::DownloadThumbnail(strThumb,m_album.thumbURL.m_url[i]))
        iResult++;
    }
    return iResult;
  }
  return 0;
}

void CGUIWindowMusicInfo::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
}

// Get Thumb from user choice.
// Options are:
// 1.  Current thumb
// 2.  AllMusic.com thumb
// 3.  Local thumb
// 4.  No thumb (if no Local thumb is available)

// TODO: Currently no support for "embedded thumb" as there is no easy way to grab it
//       without sending a file that has this as it's album to this class
void CGUIWindowMusicInfo::OnGetThumb()
{
  CFileItemList items;

  // Current thumb
  if (CFile::Exists(m_albumItem->GetThumbnailImage()))
  {
    CFileItemPtr item(new CFileItem("thumb://Current", false));
    item->SetThumbnailImage(m_albumItem->GetThumbnailImage());
    item->SetLabel(g_localizeStrings.Get(20016));
    items.Add(item);
  }

  // Grab the thumbnail(s) from the web
  CScraperUrl url;
  if (m_bArtistInfo)
    url = m_artist.thumbURL;
  else
    url = m_album.thumbURL;

  for (unsigned int i = 0; i < url.m_url.size(); i++)
  {
    CStdString strThumb;
    strThumb.Format("thumb://Remote%i",i);
    CFileItemPtr item(new CFileItem(strThumb, false));
    item->SetThumbnailImage("http://this.is/a/thumb/from/the/web");
    item->SetIconImage("DefaultPicture.png");
    item->GetVideoInfoTag()->m_strPictureURL.m_url.push_back(url.m_url[i]);
    item->SetLabel(g_localizeStrings.Get(415));
    item->SetProperty("labelonthumbload", g_localizeStrings.Get(20015));
    // make sure any previously cached thumb is removed
    if (CFile::Exists(item->GetCachedPictureThumb()))
      CFile::Delete(item->GetCachedPictureThumb());
    items.Add(item);
  }

  // local thumb
  CStdString cachedLocalThumb;
  CStdString localThumb;
  if (m_bArtistInfo)
  {
    CMusicDatabase database;
    database.Open();
    CStdString strArtistPath;
    if (database.GetArtistPath(m_artist.idArtist,strArtistPath))
      URIUtils::AddFileToFolder(strArtistPath,"folder.jpg",localThumb);
  }
  else
    localThumb = m_albumItem->GetUserMusicThumb();
  if (CFile::Exists(localThumb))
  {
    URIUtils::AddFileToFolder(g_advancedSettings.m_cachePath, "localthumb.jpg", cachedLocalThumb);
    CPicture pic;
    if (pic.CreateThumbnail(localThumb, cachedLocalThumb))
    {
      CFileItemPtr item(new CFileItem("thumb://Local", false));
      item->SetThumbnailImage(cachedLocalThumb);
      item->SetLabel(g_localizeStrings.Get(20017));
      items.Add(item);
    }
  }
  else
  {
    CFileItemPtr item(new CFileItem("thumb://None", false));
    if (m_bArtistInfo)
      item->SetIconImage("DefaultArtist.png");
    else
      item->SetIconImage("DefaultAlbumCover.png");
    item->SetLabel(g_localizeStrings.Get(20018));
    items.Add(item);
  }
  
  CStdString result;
  bool flip=false;
  VECSOURCES sources(g_settings.m_musicSources);
  g_mediaManager.GetLocalDrives(sources);
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(1030), result, &flip))
    return;   // user cancelled

  if (result == "thumb://Current")
    return;   // user chose the one they have

  // delete the thumbnail if that's what the user wants, else overwrite with the
  // new thumbnail
  CStdString cachedThumb;
  if (m_bArtistInfo)
    cachedThumb = m_albumItem->GetCachedArtistThumb();
  else
    cachedThumb = CUtil::GetCachedAlbumThumb(m_album.strAlbum, m_album.strArtist);

  if (result.Left(14).Equals("thumb://Remote"))
  {
    CStdString strFile;
    CFileItem chosen(result, false);
    CStdString thumb = chosen.GetCachedPictureThumb();
    if (CFile::Exists(thumb))
    {
      // NOTE: This could fail if the thumbloader was too slow and the user too impatient
      CFile::Cache(thumb, cachedThumb);
    }
    else
      result = "thumb://None";
  }
  if (result == "thumb://None")
  { // cache the default thumb
    CPicture pic;
    pic.CacheSkinImage("DefaultAlbumCover.png", cachedThumb);
  }
  else if (result == "thumb://Local")
    CFile::Cache(cachedLocalThumb, cachedThumb);
  else if (CFile::Exists(result))
  {
    CPicture pic;
    pic.CreateThumbnail(result, cachedThumb);
  }

  m_albumItem->SetThumbnailImage(cachedThumb);
  m_hasUpdatedThumb = true;

  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  g_windowManager.SendMessage(msg);
  // Update our screen
  Update();
}


// Allow user to select a Fanart
void CGUIWindowMusicInfo::OnGetFanart()
{
  CFileItemList items;

  CStdString cachedThumb(CFileItem::GetCachedThumb(m_artist.strArtist,g_settings.GetMusicFanartFolder()));
  if (CFile::Exists(cachedThumb))
  {
    CFileItemPtr itemCurrent(new CFileItem("fanart://Current",false));
    itemCurrent->SetThumbnailImage(cachedThumb);
    itemCurrent->SetLabel(g_localizeStrings.Get(20440));
    items.Add(itemCurrent);
  }

  // Grab the thumbnails from the web
  CStdString strPath;
  URIUtils::AddFileToFolder(g_advancedSettings.m_cachePath,"fanartthumbs",strPath);
  CUtil::WipeDir(strPath);
  XFILE::CDirectory::Create(strPath);
  for (unsigned int i = 0; i < m_artist.fanart.GetNumFanarts(); i++)
  {
    CStdString strItemPath;
    strItemPath.Format("fanart://Remote%i",i);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    item->SetThumbnailImage("http://this.is/a/thumb/from/the/web");
    item->SetIconImage("DefaultPicture.png");
    item->GetVideoInfoTag()->m_fanart = m_artist.fanart;
    item->SetProperty("fanart_number", (int)i);
    item->SetLabel(g_localizeStrings.Get(415));
    item->SetProperty("labelonthumbload", g_localizeStrings.Get(20441));

    // make sure any previously cached thumb is removed
    if (CFile::Exists(item->GetCachedPictureThumb()))
      CFile::Delete(item->GetCachedPictureThumb());
    items.Add(item);
  }
  
  // Grab a local thumb
  CMusicDatabase database;
  database.Open();
  CStdString strArtistPath;
  database.GetArtistPath(m_artist.idArtist,strArtistPath);
  CFileItem item(strArtistPath,true);
  CStdString strLocal = item.GetLocalFanart();
  if (!strLocal.IsEmpty())
  {
    CFileItemPtr itemLocal(new CFileItem("fanart://Local",false));
    itemLocal->SetThumbnailImage(strLocal);
    itemLocal->SetLabel(g_localizeStrings.Get(20438));
    // make sure any previously cached thumb is removed
    if (CFile::Exists(itemLocal->GetCachedPictureThumb()))
      CFile::Delete(itemLocal->GetCachedPictureThumb());
    items.Add(itemLocal);
  }
  else
  {
    CFileItemPtr itemNone(new CFileItem("fanart://None", false));
    itemNone->SetIconImage("DefaultArtist.png");
    itemNone->SetLabel(g_localizeStrings.Get(20439));
    items.Add(itemNone);
  }
  
  CStdString result;
  VECSOURCES sources(g_settings.m_musicSources);
  g_mediaManager.GetLocalDrives(sources);
  bool flip=false;
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(20437), result, &flip, 20445))
    return;   // user cancelled

  // delete the thumbnail if that's what the user wants, else overwrite with the
  // new thumbnail
  if (result.Equals("fanart://Current"))
   return; 

  if (result.Equals("fanart://Local"))
    result = strLocal;
 
  if (CFile::Exists(cachedThumb))
    CFile::Delete(cachedThumb);

  if (!result.Equals("fanart://None"))
  { // local file
    if (result.Left(15)  == "fanart://Remote")
    {
      int iFanart = atoi(result.Mid(15).c_str());
      m_artist.fanart.SetPrimaryFanart(iFanart);
      // download the fullres fanart image
      CStdString tempFile = "special://temp/fanart_download.jpg";
      CAsyncFileCopy downloader;
      if (!downloader.Copy(m_artist.fanart.GetImageURL(), tempFile, g_localizeStrings.Get(13413)))
        return;
      result = tempFile;
    }

    CPicture pic;
    if (flip)
      pic.ConvertFile(result, cachedThumb,0,1920,-1,100,true);
    else
      pic.CacheFanart(result, cachedThumb);

    m_albumItem->SetProperty("fanart_image",cachedThumb);
    m_hasUpdatedThumb = true;
  }

  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  g_windowManager.SendMessage(msg);
  // Update our screen
  Update();
}

void CGUIWindowMusicInfo::OnSearch(const CFileItem* pItem)
{
  CMusicDatabase database;
  database.Open();
  long idAlbum = database.GetAlbumByName(pItem->GetLabel(),m_artist.strArtist);
  if (idAlbum != -1)
  {
    CAlbum album;
    CStdString strPath;

    if (database.GetAlbumInfo(idAlbum,album,&album.songs))
    {
      database.GetAlbumPath(idAlbum,strPath);
      SetAlbum(album,strPath);
      Update();
    }
  }
}

CFileItemPtr CGUIWindowMusicInfo::GetCurrentListItem(int offset)
{ 
  return m_albumItem; 
}

