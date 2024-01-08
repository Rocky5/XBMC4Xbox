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

#include "video/VideoInfoScanner.h"
#include "FileSystem/DirectoryCache.h"
#include "Util.h"
#include "NfoFile.h"
#include "utils/RegExp.h"
#include "utils/md5.h"
#include "pictures/Picture.h"
#include "FileSystem/StackDirectory.h"
#include "xbox/XKGeneral.h"
#include "utils/IMDB.h"
#include "GUIInfoManager.h"
#include "FileSystem/File.h"
#include "dialogs/GUIDialogProgress.h"
#include "settings/Settings.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "settings/AdvancedSettings.h"
#include "FileItem.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"
#include "LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "utils/log.h"

using namespace std;
using namespace XFILE;

namespace VIDEO
{

  CVideoInfoScanner::CVideoInfoScanner()
  {
    m_bRunning = false;
    m_pObserver = NULL;
    m_bCanInterrupt = false;
    m_currentItem = 0;
    m_itemCount = 0;
    m_bClean = false;
  }

  CVideoInfoScanner::~CVideoInfoScanner()
  {
  }

  void CVideoInfoScanner::Process()
  {
    try
    {
      unsigned int tick = CTimeUtils::GetTimeMS();

      m_database.Open();

      if (m_pObserver)
        m_pObserver->OnStateChanged(PREPARING);

      m_bCanInterrupt = true;

      CLog::Log(LOGNOTICE, "VideoInfoScanner: Starting scan ..");

      // Reset progress vars
      m_currentItem = 0;
      m_itemCount = -1;

      SetPriority(THREAD_PRIORITY_IDLE);

      // Database operations should not be canceled
      // using Interupt() while scanning as it could
      // result in unexpected behaviour.
      m_bCanInterrupt = false;

      bool bCancelled = false;
      for(std::map<CStdString,VIDEO::SScanSettings>::iterator it = m_pathsToScan.begin(); it != m_pathsToScan.end(); it++)
      {
        if(!DoScan(it->first, it->second))
        {
          bCancelled = true;
          break;
        }
      }

      if (!bCancelled)
      {
        if (m_bClean)
          m_database.CleanDatabase(m_pObserver,&m_pathsToClean);
        else
        {
          if (m_pObserver)
            m_pObserver->OnStateChanged(COMPRESSING_DATABASE);
          m_database.Compress(false);
        }
      }

      m_database.Close();

      tick = CTimeUtils::GetTimeMS() - tick;
      CLog::Log(LOGNOTICE, "VideoInfoScanner: Finished scan. Scanning for video info took %s", StringUtils::SecondsToTimeString(tick / 1000).c_str());
      ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnScanFinished");
      
      m_bRunning = false;
      if (m_pObserver)
        m_pObserver->OnFinished();
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "VideoInfoScanner: Exception while scanning.");
    }
  }

  void CVideoInfoScanner::Start(const CStdString& strDirectory, const SScraperInfo& info, const SScanSettings& settings, bool bUpdateAll)
  {
    m_strStartDir = strDirectory;
    m_bUpdateAll = bUpdateAll;
    m_info = info;
    m_pathsToScan.clear();
    m_pathsToClean.clear();

    if (strDirectory.IsEmpty())
    { // scan all paths in the database.  We do this by scanning all paths in the db, and crossing them off the list as
      // we go.
      m_database.Open();
      m_database.GetPaths(m_pathsToScan);
      m_database.Close();
    }
    else
    {
      m_pathsToScan.insert(pair<CStdString,SScanSettings>(strDirectory,settings));
    }
    m_bClean = g_advancedSettings.m_bVideoLibraryCleanOnUpdate;

    StopThread();
    Create();
    m_bRunning = true;
  }

  bool CVideoInfoScanner::IsScanning()
  {
    return m_bRunning;
  }

  void CVideoInfoScanner::Stop()
  {
    if (m_bCanInterrupt)
      m_database.Interupt();

    StopThread();
  }

  void CVideoInfoScanner::SetObserver(IVideoInfoScannerObserver* pObserver)
  {
    m_pObserver = pObserver;
  }

  bool CVideoInfoScanner::DoScan(const CStdString& strDirectory, SScanSettings settings)
  {
    if (m_bUpdateAll)
    {
      if (m_pObserver)
        m_pObserver->OnStateChanged(REMOVING_OLD);

      m_database.RemoveContentForPath(strDirectory);
    }

    if (m_pObserver)
    {
      m_pObserver->OnDirectoryChanged(strDirectory);
      m_pObserver->OnSetTitle(g_localizeStrings.Get(20415));
    }

    // load subfolder
    CFileItemList items;
    int iFound;
    bool bSkip=false;
    m_database.GetScraperForPath(strDirectory,m_info,settings, iFound);
    if (m_info.strContent.IsEmpty() || m_info.strContent.Equals("None"))
      bSkip = true;

    CStdString hash, dbHash;
    if ((m_info.strContent.Equals("movies") || m_info.strContent.Equals("musicvideos")) && !settings.noupdate)
    {
      if (m_pObserver)
        m_pObserver->OnStateChanged(m_info.strContent.Equals("movies") ? FETCHING_MOVIE_INFO : FETCHING_MUSICVIDEO_INFO);

      CStdString fastHash = GetFastHash(strDirectory);
      if (m_database.GetPathHash(strDirectory, dbHash) && !fastHash.IsEmpty() && fastHash == dbHash)
      { // fast hashes match - no need to process anything
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Skipping dir '%s' due to no change (fasthash)", strDirectory.c_str());
        hash = fastHash;
        bSkip = true;
      }
      if (!bSkip)
      { // need to fetch the folder
        CDirectory::GetDirectory(strDirectory, items, g_settings.m_videoExtensions);
        if (m_info.strContent.Equals("movies"))
          items.Stack();
        // compute hash
        GetPathHash(items, hash);
        if (hash != dbHash && !hash.IsEmpty())
        {
          if (dbHash.IsEmpty())
            CLog::Log(LOGDEBUG, "VideoInfoScanner: Scanning dir '%s' as not in the database", strDirectory.c_str());
          else
            CLog::Log(LOGDEBUG, "VideoInfoScanner: Rescanning dir '%s' due to change (%s != %s)", strDirectory.c_str(), dbHash.c_str(), hash.c_str());
        }
        else
        { // they're the same or the hash is empty (dir empty/dir not retrievable)
          if (hash.IsEmpty() && !dbHash.IsEmpty())
          {
            CLog::Log(LOGDEBUG, "VideoInfoScanner: Skipping dir '%s' as it's empty or doesn't exist - adding to clean list", strDirectory.c_str());
            m_pathsToClean.push_back(m_database.GetPathId(strDirectory));
          }
          else
            CLog::Log(LOGDEBUG, "VideoInfoScanner: Skipping dir '%s' due to no change", strDirectory.c_str());
          bSkip = true;
          if (m_pObserver)
            m_pObserver->OnDirectoryScanned(strDirectory);
        }
        // update the hash to a fast hash if needed
        if (CanFastHash(items) && !fastHash.IsEmpty())
          hash = fastHash;
      }
    }
    else if (m_info.strContent.Equals("tvshows") && !settings.noupdate)
    {
      if (m_pObserver)
        m_pObserver->OnStateChanged(FETCHING_TVSHOW_INFO);

      if (iFound == 1 && !settings.parent_name_root)
      {
        CDirectory::GetDirectory(strDirectory,items,g_settings.m_videoExtensions);
        items.SetPath(strDirectory);
        GetPathHash(items, hash);
        bSkip = true;
        if (!m_database.GetPathHash(strDirectory, dbHash) || dbHash != hash)
        {
          m_database.SetPathHash(strDirectory, hash);
          bSkip = false;
        }
        else
          items.Clear();
      }
      else
      {
        CFileItemPtr item(new CFileItem(URIUtils::GetFileName(strDirectory)));
        item->SetPath(strDirectory);
        item->m_bIsFolder = true;
        items.Add(item);
        items.SetPath(URIUtils::GetParentPath(item->GetPath()));
      }
    }

    if (!m_info.settings.GetPluginRoot() && m_info.settings.GetSettings().IsEmpty()) // check for settings, if they are around load defaults - to workaround the nastyness
    {
      CScraperParser parser;
      CStdString strPath;
      if (!m_info.strContent.IsEmpty())
        strPath = "special://xbmc/system/scrapers/video/" + m_info.strPath;
      if (!strPath.IsEmpty() && parser.Load(strPath) && parser.HasFunction("GetSettings"))
      {
        if (m_info.settings.LoadSettingsXML("special://xbmc/system/scrapers/video/" + m_info.strPath))
          m_info.settings.SaveFromDefault();
      }
    }

    if (!bSkip)
    {
      if (RetrieveVideoInfo(items, settings.parent_name_root, m_info))
      {
        if (!m_bStop && (m_info.strContent.Equals("movies") || m_info.strContent.Equals("musicvideos")))
        {
          m_database.SetPathHash(strDirectory, hash);
          m_pathsToClean.push_back(m_database.GetPathId(strDirectory));
          CLog::Log(LOGDEBUG, "VideoInfoScanner: Finished adding information from dir %s", strDirectory.c_str());
        }
      } 
      else
      {
        m_pathsToClean.push_back(m_database.GetPathId(strDirectory));
        CLog::Log(LOGDEBUG, "VideoInfoScanner: No (new) information was found in dir %s", strDirectory.c_str());
      }
    }
    else if (hash != dbHash && (m_info.strContent.Equals("movies") || m_info.strContent.Equals("musicvideos")))
    { // update the hash either way - we may have changed the hash to a fast version
      m_database.SetPathHash(strDirectory, hash);
    }

    if (m_pObserver)
      m_pObserver->OnDirectoryScanned(strDirectory);

    // exclude folders that match our exclude regexps
    CStdStringArray regexps = m_info.strContent.Equals("tvshows") ? g_advancedSettings.m_tvshowExcludeFromScanRegExps
                                                                  : g_advancedSettings.m_moviesExcludeFromScanRegExps;

    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr pItem = items[i];

      if (m_bStop)
        break;

      // if we have a directory item (non-playlist) we then recurse into that folder
      if (pItem->m_bIsFolder && !pItem->GetLabel().Equals("sample") && !pItem->GetLabel().Equals("subs") && !pItem->IsParentFolder() && !pItem->IsPlayList() && settings.recurse > 0 && !m_info.strContent.Equals("tvshows")) // do not recurse for tv shows - we have already looked recursively for episodes
      {
        CStdString strPath=pItem->GetPath();

        // do not process items which will be scanned by main loop
        std::map<CStdString,VIDEO::SScanSettings>::iterator it = m_pathsToScan.find(strPath);
        if (it != m_pathsToScan.end())
          continue;

        if (CUtil::ExcludeFileOrFolder(strPath, regexps))
          continue;

        SScanSettings settings2;
        settings2.recurse = settings.recurse-1;
        settings2.parent_name_root = settings.parent_name;
        settings2.parent_name = settings.parent_name;
        if (!DoScan(strPath,settings2))
        {
          m_bStop = true;
        }
      }
    }
    return !m_bStop;
  }

  bool CVideoInfoScanner::RetrieveVideoInfo(CFileItemList& items, bool bDirNames, const SScraperInfo& info, bool bRefresh, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress, bool ignoreNfo)
  {
    m_IMDB.SetScraperInfo(info);

    if (pDlgProgress)
    {
      if (items.Size() > 1 || (items[0]->m_bIsFolder && !bRefresh))
      {
        pDlgProgress->ShowProgressBar(true);
        pDlgProgress->SetPercentage(0);
      }
      else
        pDlgProgress->ShowProgressBar(false);

      pDlgProgress->Progress();
    }

    // for every file found
    CVideoInfoTag showDetails;
    int idTvShow = -1;
    m_database.Open();
    // needed to ensure the movie count etc is cached
    for (int i=LIBRARY_HAS_VIDEO;i<LIBRARY_HAS_MUSICVIDEOS+1;++i)
      g_infoManager.GetBool(i);
    //m_database.BeginTransaction();

    bool Return(false);
    for (int i = 0; i < (int)items.Size(); ++i)
    {
      m_nfoReader.Close();
      IMDB_EPISODELIST episodes;
      EPISODES files;
      CFileItemPtr pItem = items[i];

      // we do this since we may have a override per dir
      SScraperInfo info2;
      if (pItem->m_bIsFolder)
        m_database.GetScraperForPath(pItem->GetPath(),info2);
      else
        m_database.GetScraperForPath(items.GetPath(),info2);

      if (info2.strContent.Equals("None")) // skip
        continue;

      if (!info2.settings.GetPluginRoot() && info2.settings.GetSettings().IsEmpty()) // check for settings, if they are around load defaults - to workaround the nastyness
      {
        CScraperParser parser;
        if (parser.Load("special://xbmc/system/scrapers/video/"+info2.strPath) && parser.HasFunction("GetSettings"))
        {
          if (info2.settings.LoadSettingsXML("special://xbmc/system/scrapers/video/" + info2.strPath))
            info2.settings.SaveFromDefault();
          else if (!DownloadFailed(pDlgProgress))
            return false;
          else
            continue;
        }
      }

      // we might override scraper
      if (info2.strContent == info.strContent)
        info2.strPath = info.strPath;

      m_IMDB.SetScraperInfo(info2);

      // Discard all exclude files defined by regExExclude
      if (CUtil::ExcludeFileOrFolder(pItem->GetPath(), info.strContent.Equals("tvshows") ? g_advancedSettings.m_tvshowExcludeFromScanRegExps
                                                                                         : g_advancedSettings.m_moviesExcludeFromScanRegExps))
        continue;

      if (info2.strContent.Equals("movies") || info2.strContent.Equals("musicvideos"))
      {
        if (m_pObserver)
        {
          m_pObserver->OnSetCurrentProgress(i,items.Size());
          if (!pItem->m_bIsFolder && m_itemCount)
            m_pObserver->OnSetProgress(m_currentItem++,m_itemCount);
        }

      }
      if (info2.strContent.Equals("tvshows"))
      {
        if (pItem->m_bIsFolder)
          idTvShow = m_database.GetTvShowId(pItem->GetPath());
        else
        {
          CStdString strPath;
          URIUtils::GetDirectory(pItem->GetPath(),strPath);
          idTvShow = m_database.GetTvShowId(strPath);
        }
        if (idTvShow > -1 && (!bRefresh || !pItem->m_bIsFolder))
        {
          // fetch episode guide
          m_database.GetTvShowInfo(pItem->GetPath(),showDetails,idTvShow);
          files.clear();
          EnumerateSeriesFolder(pItem.get(), files);
          if (files.size() == 0) // no update or no files
            continue;

          CScraperUrl url;
          //convert m_strEpisodeGuide in url.m_scrURL
          if (!showDetails.m_strEpisodeGuide.IsEmpty()) // assume local-only series if no episode guide url
          {
            url.ParseEpisodeGuide(showDetails.m_strEpisodeGuide);
            if (pDlgProgress)
            {
              if (pItem->m_bIsFolder)
                pDlgProgress->SetHeading(20353);
              else
                pDlgProgress->SetHeading(20361);
              pDlgProgress->SetLine(0, pItem->GetLabel());
              pDlgProgress->SetLine(1,showDetails.m_strTitle);
              pDlgProgress->SetLine(2,20354);
              pDlgProgress->Progress();
            }
            if (!m_IMDB.GetEpisodeList(url,episodes))
            {
              if (pDlgProgress)
                pDlgProgress->Close();
              //m_database.RollbackTransaction();
              m_database.Close();
              return false;
            }
          }
          if (m_bStop || (pDlgProgress && pDlgProgress->IsCanceled()))
          {
            if (pDlgProgress)
              pDlgProgress->Close();
            //m_database.RollbackTransaction();
            m_database.Close();
            return false;
          }
          if (m_pObserver)
            m_pObserver->OnDirectoryChanged(pItem->GetPath());

          if (OnProcessSeriesFolder(episodes, files, ignoreNfo, idTvShow, showDetails.m_strTitle, pDlgProgress))
          {
            Return = true;
            m_database.SetPathHash(pItem->GetPath(),pItem->GetProperty("hash"));
          }
          continue;
        }
      }

      if (!pItem->m_bIsFolder || info2.strContent.Equals("tvshows"))
      {
        if ((pItem->IsVideo() && !pItem->IsNFO() && (!pItem->IsPlayList() || URIUtils::GetExtension(pItem->GetPath()).Equals(".strm")) ) || info2.strContent.Equals("tvshows") )
        {
          if (pDlgProgress)
          {
            int iString=198;
            if (info2.strContent.Equals("tvshows"))
            {
              if (pItem->m_bIsFolder)
                iString = 20353;
              else
                iString = 20361;
            }
            if (info2.strContent.Equals("musicvideos"))
              iString = 20394;
            pDlgProgress->SetHeading(iString);
            pDlgProgress->SetLine(0, pItem->GetLabel());
            pDlgProgress->SetLine(2,"");
            pDlgProgress->Progress();
            if (pDlgProgress->IsCanceled())
            {
              pDlgProgress->Close();
              //m_database.RollbackTransaction();
              m_database.Close();
              return false;
            }
          }
          if (m_bStop)
          {
            //m_database.RollbackTransaction();
            m_database.Close();
            return false;
          }
          if ((info2.strContent.Equals("movies") && m_database.HasMovieInfo(pItem->GetPath())) ||
              (info2.strContent.Equals("musicvideos") && m_database.HasMusicVideoInfo(pItem->GetPath())))
             continue;

          CNfoFile::NFOResult result=CNfoFile::NO_NFO;
          CScraperUrl scrUrl;
          // handle .nfo files
          if (!ignoreNfo)
            result = CheckForNFOFile(pItem.get(),bDirNames,info2,scrUrl);
          if (result == CNfoFile::ERROR_NFO)
            continue;
          
          if (info2.strContent.Equals("tvshows") && result != CNfoFile::NO_NFO)
          {
            SScraperInfo info3(info2);
            SScanSettings settings;
            m_database.GetScraperForPath(pItem->GetPath(),info3,settings);
            info3.strPath = info2.strPath;
            m_database.SetScraperForPath(pItem->GetPath(),info3,settings);
          }
          if (result == CNfoFile::FULL_NFO)
          {
            pItem->GetVideoInfoTag()->Reset();
            m_nfoReader.GetDetails(*pItem->GetVideoInfoTag());
            if (m_pObserver)
              m_pObserver->OnSetTitle(pItem->GetVideoInfoTag()->m_strTitle);

            long lResult = AddMovieAndGetThumb(pItem.get(), info2.strContent, *pItem->GetVideoInfoTag(), -1, bDirNames, bRefresh, pDlgProgress);
            if (bRefresh && info.strContent.Equals("tvshows") && g_guiSettings.GetBool("videolibrary.seasonthumbs"))
              FetchSeasonThumbs(lResult);
            if (!bRefresh && info2.strContent.Equals("tvshows"))
              i--;
            Return = true;
            continue;
          }
          if (result == CNfoFile::URL_NFO || result == CNfoFile::COMBINED_NFO)
            pURL = &scrUrl;

          // Get the correct movie title 
          CStdString strMovieName = pItem->GetMovieName(bDirNames);

          IMDB_MOVIELIST movielist;
          int returncode=0;
          if (pURL || (returncode=m_IMDB.FindMovie(strMovieName, movielist, pDlgProgress)) > 0)
          {
            CScraperUrl url;
            int iMoviesFound=1;
            if (!pURL)
            {
              iMoviesFound = movielist.size();
              if (iMoviesFound)
                url = movielist[0];
            }
            else
            {
              url = *pURL;
            }
            if (iMoviesFound > 0)
            {
              if (m_pObserver && !url.strTitle.IsEmpty())
                m_pObserver->OnSetTitle(url.strTitle);
              long lResult=1;
              // force thumb and fanart

              if (pDlgProgress)
                lResult=GetIMDBDetails(pItem.get(), url, info2, bDirNames && info2.strContent.Equals("movies"), pDlgProgress, result == CNfoFile::COMBINED_NFO, ignoreNfo);
              else
                lResult=GetIMDBDetails(pItem.get(), url, info2, bDirNames && info2.strContent.Equals("movies"), NULL, result == CNfoFile::COMBINED_NFO, ignoreNfo);

              if (info2.strContent.Equals("tvshows"))
              {
                if (!bRefresh)
                {
                  // fetch episode guide
                  CVideoInfoTag details;
                  m_database.GetTvShowInfo(pItem->GetPath(),details,lResult);
                  if (!details.m_strEpisodeGuide.IsEmpty()) // assume local-only series if no episode guide url
                  {
                    CScraperUrl url;
                    url.ParseEpisodeGuide(details.m_strEpisodeGuide);
                    EnumerateSeriesFolder(pItem.get(),files);
                    if (!m_IMDB.GetEpisodeList(url,episodes))
                      continue;
                  }
                  if (OnProcessSeriesFolder(episodes, files, ignoreNfo, lResult, details.m_strTitle, pDlgProgress))
                    m_database.SetPathHash(pItem->GetPath(),pItem->GetProperty("hash"));
                }
                else
                  if (g_guiSettings.GetBool("videolibrary.seasonthumbs"))
                    FetchSeasonThumbs(lResult);
              }
              Return = true;
            }
          }
          else if (returncode == -1 || !DownloadFailed(pDlgProgress))
          {
            m_bStop = true;
            return false;
          }
          else
            continue;
        }
      }
      pURL = NULL;
    }
    if(pDlgProgress)
      pDlgProgress->ShowProgressBar(false);

    //m_database.CommitTransaction();
    g_infoManager.ResetPersistentCache();
    m_database.Close();
    return Return;
  }

  void CVideoInfoScanner::EnumerateSeriesFolder(CFileItem* item, EPISODES& episodeList)
  {
    CFileItemList items;

    if (item->m_bIsFolder)
    {
      CUtil::GetRecursiveListing(item->GetPath(),items,g_settings.m_videoExtensions,true);
      CStdString hash, dbHash;
      int numFilesInFolder = GetPathHash(items, hash);

      if (m_database.GetPathHash(item->GetPath(), dbHash) && dbHash == hash)
      {
        m_currentItem += numFilesInFolder;

        // notify our observer of our progress
        if (m_pObserver)
        {
          if (m_itemCount>0)
          {
            m_pObserver->OnSetProgress(m_currentItem, m_itemCount);
            m_pObserver->OnSetCurrentProgress(numFilesInFolder,numFilesInFolder);
          }
          m_pObserver->OnDirectoryScanned(item->GetPath());
        }
        return;
      }
      m_pathsToClean.push_back(m_database.GetPathId(item->GetPath()));
      m_database.GetPathsForTvShow(m_database.GetTvShowId(item->GetPath()),m_pathsToClean);
      item->SetProperty("hash",hash);
    }
    else
    {
      CFileItemPtr newItem(new CFileItem(*item));
      items.Add(newItem);
    }

    /*
    stack down any dvd folders
    need to sort using the full path since this is a collapsed recursive listing of all subdirs
    video_ts.ifo files should sort at the top of a dvd folder in ascending order

    /foo/bar/video_ts.ifo
    /foo/bar/vts_x_y.ifo
    /foo/bar/vts_x_y.vob
    */

    // since we're doing this now anyway, should other items be stacked?
    items.Sort(SORT_METHOD_FULLPATH, SORT_ORDER_ASC);
    int x = 0;
    while (x < items.Size())
    {
      if (items[x]->m_bIsFolder)
        continue;


      CStdString strPathX, strFileX;
      URIUtils::Split(items[x]->GetPath(), strPathX, strFileX);
      //CLog::Log(LOGDEBUG,"%i:%s:%s", x, strPathX.c_str(), strFileX.c_str());

      int y = x + 1;
      if (strFileX.Equals("VIDEO_TS.IFO"))
      {
        while (y < items.Size())
        {
          CStdString strPathY, strFileY;
          URIUtils::Split(items[y]->GetPath(), strPathY, strFileY);
          //CLog::Log(LOGDEBUG," %i:%s:%s", y, strPathY.c_str(), strFileY.c_str());

          if (strPathY.Equals(strPathX))
            /*
            remove everything sorted below the video_ts.ifo file in the same path.
            understandbly this wont stack correctly if there are other files in the the dvd folder.
            this should be unlikely and thus is being ignored for now but we can monitor the
            where the path changes and potentially remove the items above the video_ts.ifo file.
            */
            items.Remove(y);
          else
            break;
        }
      }
      x = y;
    }

    // enumerate
    SETTINGS_TVSHOWLIST expression = g_advancedSettings.m_tvshowStackRegExps;
    CStdStringArray regexps = g_advancedSettings.m_tvshowExcludeFromScanRegExps;

    for (int i=0;i<items.Size();++i)
    {
      if (items[i]->m_bIsFolder)
        continue;
      CStdString strPath;
      URIUtils::GetDirectory(items[i]->GetPath(), strPath);
      URIUtils::RemoveSlashAtEnd(strPath); // want no slash for the test that follows

      if (URIUtils::GetFileName(strPath).Equals("sample"))
        continue;

      // Discard all exclude files defined by regExExcludes
      if (CUtil::ExcludeFileOrFolder(items[i]->GetPath(), regexps))
        continue;

      bool bMatched=false;
      for (unsigned int j=0;j<expression.size();++j)
      {
        if (expression[j].byDate)
        {
          bMatched = ProcessItemByDate(items[i], episodeList, expression[j].regexp);
        }
        else
        {
          bMatched = ProcessItemNormal(items[i], episodeList, expression[j].regexp);
        }
        if (bMatched)
          break;
      }

      if (!bMatched)
      {
        CStdString decode(items[i]->GetPath());
        CURL::Decode(decode);
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Could not enumerate file %s", decode.c_str());
      }
    }
  }

  bool CVideoInfoScanner::ProcessItemNormal(CFileItemPtr item, EPISODES &episodeList, CStdString regexp)
  {
    CRegExp reg;
    if (!reg.RegComp(regexp))
      return false;

    CStdString strLabel=item->GetPath();
    // URLDecode in case an episode is on a http/https/dav/davs:// source and URL-encoded like foo%201x01%20bar.avi
    CURL::Decode(strLabel);
    strLabel.MakeLower();
//    CLog::Log(LOGDEBUG,"running expression %s on label %s",regexp.c_str(),strLabel.c_str());
    int regexppos, regexp2pos;

    if ((regexppos = reg.RegFind(strLabel.c_str())) < 0)
      return false;


    SEpisode episode;
    episode.strPath = item->GetPath();
    episode.cDate.SetValid(false);
    if (!GetEpisodeAndSeasonFromRegExp(reg, episode))
      return false;
    
    CLog::Log(LOGDEBUG, "VideoInfoScanner: Found episode match %s (s%ie%i) [%s]", strLabel.c_str(), episode.iSeason, episode.iEpisode, regexp.c_str());
    episodeList.push_back(episode);

    // check the remainder of the string for any further episodes.
    CRegExp reg2;
    if (!reg2.RegComp(g_advancedSettings.m_tvshowMultiPartStackRegExp))
      return true;

    char *remainder = reg.GetReplaceString("\\3");
    int offset = 0;

    // we want "long circuit" OR below so that both offsets are evaluated
    while (((regexp2pos = reg2.RegFind(remainder + offset)) > -1) | ((regexppos = reg.RegFind(remainder + offset)) > -1))
    {
      if (((regexppos <= regexp2pos) && regexppos != -1) ||
         (regexppos >= 0 && regexp2pos == -1))
      {
        GetEpisodeAndSeasonFromRegExp(reg, episode);
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Adding new season %u, multipart episode %u", episode.iSeason, episode.iEpisode);
        episodeList.push_back(episode);
        free(remainder);
        remainder = reg.GetReplaceString("\\3");
        offset = 0;
      }
      else if (((regexp2pos < regexppos) && regexp2pos != -1) ||
               (regexp2pos >= 0 && regexppos == -1))
      {
        char *ep = reg2.GetReplaceString("\\1");
        episode.iEpisode = atoi(ep);
        free(ep);
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Adding multipart episode %u", episode.iEpisode);
        episodeList.push_back(episode);
        offset += regexp2pos + reg2.GetFindLen();
      }
    }
    free(remainder);
    return true;
  }

  bool CVideoInfoScanner::GetEpisodeAndSeasonFromRegExp(CRegExp &reg, SEpisode &episodeInfo)
  {
    char* season = reg.GetReplaceString("\\1");
    char* episode = reg.GetReplaceString("\\2");

    if (season && episode)
    {
      if (strlen(season) == 0 && strlen(episode) > 0)
      { // no season specified -> assume season 1
        episodeInfo.iSeason = 1;
        if ((episodeInfo.iEpisode = CUtil::TranslateRomanNumeral(episode)) == -1)
          episodeInfo.iEpisode = atoi(episode);
      } 
      else if (strlen(season) > 0 && strlen(episode) == 0)
      { // no episode specification -> assume season 1
        episodeInfo.iSeason = 1;
        if ((episodeInfo.iEpisode = CUtil::TranslateRomanNumeral(season)) == -1)
          episodeInfo.iEpisode = atoi(season);
      }
      else
      { // season and episode specified
        episodeInfo.iSeason = atoi(season);
        episodeInfo.iEpisode = atoi(episode);
      }
    }
    free(season);
    free(episode);
    return (season && episode);
  }

  bool CVideoInfoScanner::ProcessItemByDate(CFileItemPtr item, EPISODES &episodeList, CStdString regexp)
  {
    CRegExp reg;
    if (!reg.RegComp(regexp))
      return false;

    CStdString strLabel=item->GetPath();
    // URLDecode in case an episode is on a http/https/dav/davs:// source and URL-encoded like foo%201x01%20bar.avi
    CURL::Decode(strLabel);
    strLabel.MakeLower();
//    CLog::Log(LOGDEBUG,"running expression %s on label %s",regexp.c_str(),strLabel.c_str());
    int regexppos;

    if ((regexppos = reg.RegFind(strLabel.c_str())) < 0)
      return false;

    bool bMatched = false;

    char* param1 = reg.GetReplaceString("\\1");
    char* param2 = reg.GetReplaceString("\\2");
    char* param3 = reg.GetReplaceString("\\3");
    if (param1 && param2 && param3)
    {
      // regular expression by date
      int len1 = strlen( param1 );
      int len2 = strlen( param2 );
      int len3 = strlen( param3 );
      char* day;
      char* month;
      char* year;
      if (len1==4 && len2==2 && len3==2)
      {
        // yyyy mm dd format
        bMatched = true;
        year = param1;
        month = param2;
        day = param3;
      }
      else if (len1==2 && len2==2 && len3==4)
      {
        // mm dd yyyy format
        bMatched = true;
        year = param3;
        month = param1;
        day = param2;
      }
      if (bMatched)
      {
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Found date based match %s (Y%sm=%sd=%s) [%s]", strLabel.c_str(), year, month, day, regexp.c_str());
        SEpisode myEpisode;
        myEpisode.strPath = item->GetPath();
        myEpisode.iSeason = -1;
        myEpisode.iEpisode = -1;
        myEpisode.cDate.SetDate(atoi(year),atoi(month),atoi(day));
        episodeList.push_back(myEpisode);
      }
    }
    free(param1);
    free(param2);
    free(param3);
    return bMatched;
  }

  long CVideoInfoScanner::AddMovie(CFileItem *pItem, const CStdString &content, CVideoInfoTag &movieDetails, int idShow)
  {
    // ensure our database is open (this can get called via other classes)
    if (!m_database.Open())
    {
      CLog::Log(LOGERROR, "%s - failed to open database", __FUNCTION__);
      return -1;
    }
    
    CLog::Log(LOGDEBUG, "VideoInfoScanner: Adding new item to %s:%s", content.c_str(), pItem->GetPath().c_str());
    long lResult = -1;

    // add to all movies in the stacked set
    if (content.Equals("movies"))
    {
      // find local trailer first
      CStdString strTrailer = pItem->FindTrailer();
      if (!strTrailer.IsEmpty())
        movieDetails.m_strTrailer = strTrailer;

      int idMovie=m_database.SetDetailsForMovie(pItem->GetPath(), movieDetails);

      // setup links to shows if the linked shows are in the db
      if (!movieDetails.m_strShowLink.IsEmpty())
      {
        CStdStringArray list;
        StringUtils::SplitString(movieDetails.m_strShowLink,
                                 g_advancedSettings.m_videoItemSeparator,list);
        for (unsigned int i=0;i<list.size();++i)
        {
          CFileItemList items;
          m_database.GetTvShowsByName(list[i],items);
          if (items.Size())
            m_database.LinkMovieToTvshow(idMovie,items[0]->GetVideoInfoTag()->m_iDbId,false);
          else
            CLog::Log(LOGDEBUG, "VideoInfoScanner: Failed to link movie %s to show %s", movieDetails.m_strTitle.c_str(), list[i].c_str());
        }
      }
    }
    else if (content.Equals("tvshows"))
    {
      if (pItem->m_bIsFolder)
      {
        lResult=m_database.SetDetailsForTvShow(pItem->GetPath(), movieDetails);
      }
      else
      {
        // we add episode then set details, as otherwise set details will delete the
        // episode then add, which breaks multi-episode files.
        int idEpisode = m_database.AddEpisode(idShow, pItem->GetPath());
        lResult = m_database.SetDetailsForEpisode(pItem->GetPath(), movieDetails, idShow, idEpisode);
        if (movieDetails.m_fEpBookmark > 0)
        {
          movieDetails.m_strFileNameAndPath = pItem->GetPath();
          CBookmark bookmark;
          bookmark.timeInSeconds = movieDetails.m_fEpBookmark;
          bookmark.seasonNumber = movieDetails.m_iSeason;
          bookmark.episodeNumber = movieDetails.m_iEpisode;
          m_database.AddBookMarkForEpisode(movieDetails,bookmark);
        }
      }
    }
    else if (content.Equals("musicvideos"))
    {
      m_database.SetDetailsForMusicVideo(pItem->GetPath(), movieDetails);
    }

    if (g_advancedSettings.m_bVideoLibraryImportWatchedState)
      m_database.SetPlayCount(*pItem, movieDetails.m_playCount, movieDetails.m_lastPlayed);

    return lResult;
  }

  long CVideoInfoScanner::AddMovieAndGetThumb(CFileItem *pItem, const CStdString &content, CVideoInfoTag &movieDetails, int idShow, bool bApplyToDir, bool bRefresh, CGUIDialogProgress* pDialog /* == NULL */)
  {
    long lResult = AddMovie(pItem, content, movieDetails, idShow);
    // get & save fanart image
    if (!pItem->CacheLocalFanart() || bRefresh)
    {
      if (!movieDetails.m_fanart.m_xml.IsEmpty() && !movieDetails.m_fanart.GetImageURL().IsEmpty() && !movieDetails.m_fanart.DownloadImage(pItem->GetCachedFanart()))
        CLog::Log(LOGWARNING, "VideoInfoScanner: Failed to download fanart %s to %s", movieDetails.m_fanart.GetImageURL().c_str(), pItem->GetCachedFanart().c_str());
    }

    CStdString strUserThumb = pItem->GetUserVideoThumb();
    if (bApplyToDir && (strUserThumb.IsEmpty() || bRefresh))
    {
      CStdString strParent;
      URIUtils::GetParentPath(pItem->GetPath(),strParent);
      CFileItem item(*pItem);
      item.SetPath(strParent);
      item.m_bIsFolder = true;
      strUserThumb = item.GetUserVideoThumb();
    }

    CStdString strThumb = pItem->GetCachedVideoThumb();
    if (content.Equals("tvshows") && !pItem->m_bIsFolder && CFile::Exists(strThumb))
    {
      movieDetails.m_strFileNameAndPath = pItem->GetPath();
      CFileItem item(movieDetails);
      strThumb = item.GetCachedEpisodeThumb();
    }

    CStdString strImage = movieDetails.m_strPictureURL.GetFirstThumb().m_url;
    if (strImage.size() > 0 && (strUserThumb.IsEmpty() || bRefresh))
    {
      if (pDialog)
      {
        pDialog->SetLine(2, 415);
        pDialog->Progress();
      }

      CPicture picture;
      try
      {
        if (strImage.Find("http://") < 0 &&
            strImage.Find("/") < 0 &&
            strImage.Find("\\") < 0)
        {
          CStdString strPath;
          URIUtils::GetDirectory(pItem->GetPath(), strPath);
          strImage = URIUtils::AddFileToFolder(strPath,strImage);
        }
        picture.CreateThumbnail(strImage,strThumb);
      }
      catch (...)
      {
        CLog::Log(LOGERROR, "VideoInfoScanner: Could not make imdb thumb from %s", strImage.c_str());
        CFile::Delete(strThumb);
      }
    }
    else
    if (!strUserThumb.IsEmpty())
    {
      CPicture picture;
      picture.CacheThumb(strUserThumb, strThumb);
    }

    CStdString strCheck=pItem->GetPath();
    CStdString strDirectory;
    if (pItem->IsStack())
      strCheck = CStackDirectory::GetFirstStackedFile(pItem->GetPath());

    URIUtils::GetDirectory(strCheck,strDirectory);
    if (URIUtils::IsInRAR(strCheck))
    {
      CStdString strPath=strDirectory;
      URIUtils::GetParentPath(strPath,strDirectory);
    }
    if (pItem->IsStack())
    {
      strCheck = strDirectory;
      URIUtils::RemoveSlashAtEnd(strCheck);
      if (URIUtils::GetFileName(strCheck).size() == 3 && URIUtils::GetFileName(strCheck).Left(2).Equals("cd"))
        URIUtils::GetDirectory(strCheck,strDirectory);
    }
    if (bApplyToDir && !strThumb.IsEmpty())
      ApplyIMDBThumbToFolder(strDirectory,strThumb);

    if (g_guiSettings.GetBool("videolibrary.actorthumbs"))
      FetchActorThumbs(movieDetails.m_cast,strDirectory);
    m_database.Close();
    return lResult;
  }

  bool CVideoInfoScanner::OnProcessSeriesFolder(IMDB_EPISODELIST& episodes, EPISODES& files, bool ignoreNfo, int idShow, const CStdString& strShowTitle, CGUIDialogProgress* pDlgProgress /* = NULL */)
  {
    if (pDlgProgress)
    {
      pDlgProgress->SetLine(2, 20361);
      pDlgProgress->SetPercentage(0);
      pDlgProgress->ShowProgressBar(true);
      pDlgProgress->Progress();
    }

    int iMax = files.size();
    int iCurr = 1;
    m_database.Open();
    for (EPISODES::iterator file = files.begin(); file != files.end(); ++file)
    {
      m_nfoReader.Close();
      if (pDlgProgress)
      {
        pDlgProgress->SetLine(2, 20361);
        pDlgProgress->SetPercentage((int)((float)(iCurr++)/iMax*100));
        pDlgProgress->Progress();
      }
      if (m_pObserver)
      {
        if (m_itemCount > 0)
          m_pObserver->OnSetProgress(m_currentItem++,m_itemCount);
        m_pObserver->OnSetCurrentProgress(iCurr++,iMax);
      }
      if ((pDlgProgress && pDlgProgress->IsCanceled()) || m_bStop)
      {
        if (pDlgProgress)
          pDlgProgress->Close();
        //m_database.RollbackTransaction();
        m_database.Close();
        return false;
      }

      CVideoInfoTag episodeDetails;
      if (m_database.GetEpisodeId(file->strPath,file->iEpisode,file->iSeason) > -1)
      {
        if (m_pObserver)
          m_pObserver->OnSetTitle(g_localizeStrings.Get(20415));
        continue;
      }

      CFileItem item;
      item.SetPath(file->strPath);

      // handle .nfo files
      CNfoFile::NFOResult result=CNfoFile::NO_NFO; 
      CScraperUrl scrUrl;
      SScraperInfo info(m_IMDB.GetScraperInfo());
      item.GetVideoInfoTag()->m_iEpisode = file->iEpisode;
      if (!ignoreNfo)
        result = CheckForNFOFile(&item,false,info,scrUrl);
      if (result == CNfoFile::FULL_NFO)
      {
        m_nfoReader.GetDetails(episodeDetails);
        if (m_pObserver)
        {
          CStdString strTitle;
          strTitle.Format("%s - %ix%i - %s",strShowTitle.c_str(),episodeDetails.m_iSeason,episodeDetails.m_iEpisode,episodeDetails.m_strTitle.c_str());
          m_pObserver->OnSetTitle(strTitle);
        }
        AddMovieAndGetThumb(&item,"tvshows",episodeDetails,idShow);
        continue;
      }

      if (episodes.empty())
      {
        CLog::Log(LOGERROR, "VideoInfoScanner: Asked to lookup episode %s"
                            " online, but we have no episode guide. Check your tvshow.nfo and make"
                            " sure the <episodeguide> tag is in place.", file->strPath.c_str());
        continue;
      }

      std::pair<int,int> key;
      key.first = file->iSeason;
      key.second = file->iEpisode;
      bool bFound = false;
      IMDB_EPISODELIST::iterator guide = episodes.begin();;

      for (; guide != episodes.end(); ++guide )
      {
        if (file->cDate.IsValid() && guide->cDate.IsValid() && file->cDate==guide->cDate)
        {
          bFound = true;
          break;
        }
        if ((file->iEpisode!=-1) && (file->iSeason!=-1) && (key==guide->key))
        {
          bFound = true;
          break;
        }
      }

      if (bFound)
      {
        if (!m_IMDB.GetEpisodeDetails(guide->cScraperUrl,episodeDetails,pDlgProgress))
        {
          m_database.Close();
          return false;
        }
        episodeDetails.m_iSeason = guide->key.first;
        episodeDetails.m_iEpisode = guide->key.second;
        if (m_pObserver)
        {
          CStdString strTitle;
          strTitle.Format("%s - %ix%i - %s",strShowTitle.c_str(),episodeDetails.m_iSeason,episodeDetails.m_iEpisode,episodeDetails.m_strTitle.c_str());
          m_pObserver->OnSetTitle(strTitle);
        }
        CFileItem item;
        item.SetPath(file->strPath);
        AddMovieAndGetThumb(&item,"tvshows",episodeDetails,idShow);
      }
    }
    if (g_guiSettings.GetBool("videolibrary.seasonthumbs"))
      FetchSeasonThumbs(idShow);
    m_database.Close();
    return true;
  }

  CStdString CVideoInfoScanner::GetnfoFile(CFileItem *item, bool bGrabAny)
  {
    CStdString nfoFile;
    // Find a matching .nfo file
    if (!item->m_bIsFolder)
    {
      // file
      CStdString strExtension;
      URIUtils::GetExtension(item->GetPath(), strExtension);

      if (URIUtils::IsInRAR(item->GetPath())) // we have a rarred item - we want to check outside the rars
      {
        CFileItem item2(*item);
        CURL url(item->GetPath());
        CStdString strPath;
        URIUtils::GetDirectory(url.GetHostName(), strPath);
        item2.SetPath(URIUtils::AddFileToFolder(strPath, URIUtils::GetFileName(item->GetPath())));
        return GetnfoFile(&item2, bGrabAny);
      }

      // grab the folder path
      CStdString strPath;
      URIUtils::GetDirectory(item->GetPath(), strPath);

      if (bGrabAny)
      { // looking up by folder name - movie.nfo and mymovies.xml take priority
        nfoFile = URIUtils::AddFileToFolder(strPath, "movie.nfo");
        if (CFile::Exists(nfoFile))
          return nfoFile;
        nfoFile = URIUtils::AddFileToFolder(strPath, "mymovies.xml");
        if (CFile::Exists(nfoFile))
          return nfoFile;
      }

      // try looking for .nfo file for a stacked item
      if (item->IsStack())
      {
        // first try .nfo file matching first file in stack
        CStackDirectory dir;
        CStdString firstFile = dir.GetFirstStackedFile(item->GetPath());
        CFileItem item2;
        item2.SetPath(firstFile);
        nfoFile = GetnfoFile(&item2, bGrabAny);
        // else try .nfo file matching stacked title
        if (nfoFile.IsEmpty())
        {
          CStdString stackedTitlePath = dir.GetStackedTitlePath(item->GetPath());
          item2.SetPath(stackedTitlePath);
          nfoFile = GetnfoFile(&item2, bGrabAny);
        }
      }
      else
      {
         // already an .nfo file?
        if ( strcmpi(strExtension.c_str(), ".nfo") == 0 )
          nfoFile = item->GetPath();
        // no, create .nfo file
        else
          nfoFile = URIUtils::ReplaceExtension(item->GetPath(), ".nfo");
      }

      // test file existence
      if (!nfoFile.IsEmpty() && !CFile::Exists(nfoFile))
        nfoFile.Empty();

      if (nfoFile.IsEmpty()) // final attempt - strip off any cd1 folders
      {
        URIUtils::RemoveSlashAtEnd(strPath); // need no slash for the check that follows
        CFileItem item2;
        if (strPath.Mid(strPath.size()-3).Equals("cd1"))
        {
          strPath = strPath.Mid(0,strPath.size()-3);
          item2.SetPath(URIUtils::AddFileToFolder(strPath, URIUtils::GetFileName(item->GetPath())));
          return GetnfoFile(&item2, bGrabAny);
        }
      }
    }
    // folders (or stacked dvds) can take any nfo file if there's a unique one
    if (item->m_bIsFolder || item->IsDVDFile(false, true) || (bGrabAny && nfoFile.IsEmpty()))
    {
      // see if there is a unique nfo file in this folder, and if so, use that
      CFileItemList items;
      CDirectory dir;
      CStdString strPath = item->GetPath();
      if (!item->m_bIsFolder)
        URIUtils::GetDirectory(item->GetPath(), strPath);
      if (dir.GetDirectory(strPath, items, ".nfo") && items.Size())
      {
        int numNFO = -1;
        for (int i = 0; i < items.Size(); i++)
        {
          if (items[i]->IsNFO())
          {
            if (numNFO == -1)
              numNFO = i;
            else
            {
              numNFO = -1;
              break;
            }
          }
        }
        if (numNFO > -1)
          return items[numNFO]->GetPath();
      }
    }

    return nfoFile;
  }

  long CVideoInfoScanner::GetIMDBDetails(CFileItem *pItem, CScraperUrl &url, const SScraperInfo& info, bool bUseDirNames, CGUIDialogProgress* pDialog /* = NULL */, bool bCombined, bool bRefresh)
  {
    CVideoInfoTag movieDetails;
    m_IMDB.SetScraperInfo(info);
    movieDetails.m_strFileNameAndPath = pItem->GetPath();

    if ( m_IMDB.GetDetails(url, movieDetails, pDialog) )
    {
      if (bCombined)
        m_nfoReader.GetDetails(movieDetails);

      if (m_pObserver && url.strTitle.IsEmpty())
        m_pObserver->OnSetTitle(movieDetails.m_strTitle);

      if (pDialog)
      {
        pDialog->SetLine(1, movieDetails.m_strTitle);
        pDialog->Progress();
      }

      return AddMovieAndGetThumb(pItem, info.strContent, movieDetails, -1, bUseDirNames, bRefresh);
    }
    return -1;
  }

  void CVideoInfoScanner::ApplyIMDBThumbToFolder(const CStdString &folder, const CStdString &imdbThumb)
  {
    // copy icon to folder also;
    if (CFile::Exists(imdbThumb))
    {
      CFileItem folderItem(folder, true);
      CStdString strThumb(folderItem.GetCachedVideoThumb());
      CFile::Cache(imdbThumb.c_str(), strThumb.c_str(), NULL, NULL);
    }
  }

  int CVideoInfoScanner::GetPathHash(const CFileItemList &items, CStdString &hash)
  {
    // Create a hash based on the filenames, filesize and filedate.  Also count the number of files
    if (0 == items.Size()) return 0;
    XBMC::XBMC_MD5 md5state;
    int count = 0;
    for (int i = 0; i < items.Size(); ++i)
    {
      const CFileItemPtr pItem = items[i];
      md5state.append(pItem->GetPath());
      md5state.append((unsigned char *)&pItem->m_dwSize, sizeof(pItem->m_dwSize));
      FILETIME time = pItem->m_dateTime;
      md5state.append((unsigned char *)&time, sizeof(FILETIME));
      if (pItem->IsVideo() && !pItem->IsPlayList() && !pItem->IsNFO())
        count++;
    }
    md5state.getDigest(hash);
    return count;
  }

  bool CVideoInfoScanner::CanFastHash(const CFileItemList &items) const
  {
    // TODO: Probably should account for excluded folders here (eg samples), though that then
    //       introduces possible problems if the user then changes the exclude regexps and
    //       expects excluded folders that are inside a fast-hashed folder to then be picked
    //       up. The chances that the user has a folder which contains only excluded folders
    //       where some of those folders should be scanned recursively is pretty small.
    return items.GetFolderCount() == 0;
  }

  CStdString CVideoInfoScanner::GetFastHash(const CStdString &directory) const
  {
    struct __stat64 buffer;
    if (XFILE::CFile::Stat(directory, &buffer) == 0)
    {
      int64_t time = buffer.st_mtime;
      if (!time)
        time = buffer.st_ctime;
      if (time)
      {
        CStdString hash;
        hash.Format("fast%"PRId64, time);
        return hash;
      }
    }
    return "";
  }

  void CVideoInfoScanner::FetchSeasonThumbs(int idTvShow, const CStdString &folderToCheck, bool download, bool overwrite)
  {
    CVideoInfoTag movie;
    m_database.GetTvShowInfo("",movie,idTvShow);
    CStdString showDir(folderToCheck.IsEmpty() ? movie.m_strPath : folderToCheck);
    CFileItemList items;
    CStdString strPath;
    strPath.Format("videodb://2/2/%i/",idTvShow);
    m_database.GetSeasonsNav(strPath,items,-1,-1,-1,-1,idTvShow);
    CFileItemPtr pItem;
    pItem.reset(new CFileItem(g_localizeStrings.Get(20366)));  // "All Seasons"
    CStdString path;
    path.Format("%s/-1/",strPath.c_str());
    pItem->SetPath(path);
    pItem->GetVideoInfoTag()->m_iSeason = -1;
    pItem->GetVideoInfoTag()->m_strPath = movie.m_strPath;
    if (overwrite || !XFILE::CFile::Exists(pItem->GetCachedSeasonThumb()))
      items.Add(pItem);

    // used for checking for a season[ ._-](number).tbn
    CFileItemList tbnItems;
    CDirectory::GetDirectory(showDir,tbnItems,".tbn");
    for (int i=0;i<items.Size();++i)
    {
      if (overwrite || !items[i]->HasThumbnail())
      {
        CStdString strExpression;
        int iSeason = items[i]->GetVideoInfoTag()->m_iSeason;
        if (iSeason == -1)
          strExpression = "season-all.tbn";
        else if (iSeason == 0)
          strExpression = "season-specials.tbn";
        else
          strExpression.Format("season[ ._-]?(0?%i)\\.tbn",items[i]->GetVideoInfoTag()->m_iSeason);
        bool bDownload = download;
        CRegExp reg;
        if (reg.RegComp(strExpression.c_str()))
        {
          for (int j=0;j<tbnItems.Size();++j)
          {
            CStdString strCheck = URIUtils::GetFileName(tbnItems[j]->GetPath());
            strCheck.ToLower();
            if (reg.RegFind(strCheck.c_str()) > -1)
            {
              CPicture picture;
              picture.CreateThumbnail(tbnItems[j]->GetPath(),items[i]->GetCachedSeasonThumb());
              bDownload=false;
              break;
            }
          }
        }
        if (bDownload)
          CScraperUrl::DownloadThumbnail(items[i]->GetCachedSeasonThumb(),movie.m_strPictureURL.GetSeasonThumb(items[i]->GetVideoInfoTag()->m_iSeason));
      }
    }
  }

  void CVideoInfoScanner::FetchActorThumbs(const vector<SActorInfo>& actors, const CStdString& strPath)
  {
    for (unsigned int i=0;i<actors.size();++i)
    {
      CFileItem item;
      item.SetLabel(actors[i].strName);
      CStdString strThumb = item.GetCachedActorThumb();
      if (!CFile::Exists(strThumb))
      {
        CStdString thumbFile = actors[i].strName;
        thumbFile.Replace(" ","_");
        thumbFile += ".tbn";
        CStdString strLocal = URIUtils::AddFileToFolder(URIUtils::AddFileToFolder(strPath,".actors"),thumbFile);
        if (CFile::Exists(strLocal))
        {
          CPicture pic;
          pic.CreateThumbnail(strLocal,strThumb);
        }
        else if (!actors[i].thumbUrl.GetFirstThumb().m_url.IsEmpty())
          CScraperUrl::DownloadThumbnail(strThumb,actors[i].thumbUrl.GetFirstThumb());
      }
    }
  }

  CNfoFile::NFOResult CVideoInfoScanner::CheckForNFOFile(CFileItem* pItem, bool bGrabAny, SScraperInfo& info, CScraperUrl& scrUrl)
  {
    CStdString strNfoFile;
    if (info.strContent.Equals("movies") || info.strContent.Equals("musicvideos") || (info.strContent.Equals("tvshows") && !pItem->m_bIsFolder))
      strNfoFile = GetnfoFile(pItem,bGrabAny);
    if (info.strContent.Equals("tvshows") && pItem->m_bIsFolder)
      URIUtils::AddFileToFolder(pItem->GetPath(),"tvshow.nfo",strNfoFile);

    CNfoFile::NFOResult result=CNfoFile::NO_NFO;
    if (!strNfoFile.IsEmpty() && CFile::Exists(strNfoFile))
    {
      result = m_nfoReader.Create(strNfoFile,info,pItem->GetVideoInfoTag()->m_iEpisode);

      CStdString type;
      switch(result)
      {
        case CNfoFile::COMBINED_NFO:
          type = "Mixed";
          break;
        case CNfoFile::FULL_NFO:
          type = "Full";
          break;
        case CNfoFile::URL_NFO:
          type = "URL";
          break;
        case CNfoFile::NO_NFO:
          type = "";
          break;
        default:
          type = "malformed";
      }
      if (result != CNfoFile::NO_NFO)
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Found matching %s NFO file: %s", type.c_str(), strNfoFile.c_str());
      if (result == CNfoFile::FULL_NFO)
      {
        if (info.strContent.Equals("tvshows"))
          info.strPath = m_nfoReader.m_strScraper;
      }
      else if (result != CNfoFile::NO_NFO && result != CNfoFile::ERROR_NFO)
      {
        CScraperUrl url(m_nfoReader.m_strImDbUrl);
        scrUrl = url;

        scrUrl.strId  = m_nfoReader.m_strImDbNr;
        info.strPath = m_nfoReader.m_strScraper;

        CLog::Log(LOGDEBUG, "VideoInfoScanner: Fetching url '%s' using %s scraper (file: '%s', content: '%s', language: '%s', date: '%s', framework: '%s')",
          scrUrl.m_url[0].m_url.c_str(), info.strTitle.c_str(), info.strPath.c_str(), info.strContent.c_str(), info.strLanguage.c_str(), info.strDate.c_str(), info.strFramework.c_str());
        if (result == CNfoFile::COMBINED_NFO)
          m_nfoReader.GetDetails(*pItem->GetVideoInfoTag());
      }
    }
    else
      CLog::Log(LOGDEBUG, "VideoInfoScanner: No NFO file found. Using title search for '%s'", pItem->GetPath().c_str());

    return result;
  }

  bool CVideoInfoScanner::DownloadFailed(CGUIDialogProgress* pDialog)
  {
    if (g_advancedSettings.m_bVideoScannerIgnoreErrors)
      return true;

    if (pDialog)
    {
      CGUIDialogOK::ShowAndGetInput(20448,20449,20022,20022);
      return false;
    }
    return CGUIDialogYesNo::ShowAndGetInput(20448,20449,20450,20022);
  }
}
