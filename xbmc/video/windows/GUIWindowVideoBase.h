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

#include "windows/GUIMediaWindow.h"
#include "video/VideoDatabase.h"
#include "PlayListPlayer.h"
#include "ThumbLoader.h"

class CGUIWindowVideoBase : public CGUIMediaWindow, public IBackgroundLoaderObserver, public IStreamDetailsObserver
{
public:
  CGUIWindowVideoBase(int id, const CStdString &xmlFile);
  virtual ~CGUIWindowVideoBase(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

  void PlayMovie(const CFileItem *item);
  int  GetResumeItemOffset(const CFileItem *item);

  void AddToDatabase(int iItem);
  static void OnScan(const CStdString& strPath, const SScraperInfo& info, const VIDEO::SScanSettings& settings);
  virtual void OnInfo(CFileItem* pItem, const SScraperInfo& info);
  virtual void OnStreamDetails(const CStreamDetails &details, const CStdString &strFileName, long lFileId);
  static void MarkWatched(const CFileItemPtr &pItem, bool bMark);
  static void UpdateVideoTitle(const CFileItem* pItem);

  //! Shows the resume menu following the 'resumeautomatically' guisettig (also checks if there is a bookmark on the file)
  //! It changes item.m_lStartOffset if resuming otherwise does nothing to item
  //! If the user cancels the operation on the menu "false" is returned
  static bool OnResumeShowMenu(CFileItem &item);

  /*! \brief Append a set of search items to a results list using a specific prepend label
   Sorts the search items first, then appends with the given prependLabel to the results list.
   Then empty the search item list so it can be refilled.
   \param searchItems The search items to append.
   \param prependLabel the label that should be prepended to all search results.
   \param results the fileitemlist to append the search results to.
   \sa DoSearch
   */
  static void AppendAndClearSearchItems(CFileItemList &searchItems, const CStdString &prependLabel, CFileItemList &results);

private:
  bool IsCorrectDiskInDrive(const CStdString& strFileName, const CStdString& strDVDLabel);
protected:
  virtual void UpdateButtons();
  virtual bool Update(const CStdString &strDirectory);
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void OnItemLoaded(CFileItem* pItem) {};
  virtual void OnPrepareFileItems(CFileItemList &items);

  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  void GetNonContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual void OnAssignContent(int iItem, int iFound, SScraperInfo& info, VIDEO::SScanSettings& settings) {};
  virtual void OnUnAssignContent(int iItem) {};
  virtual void OnQueueItem(int iItem);
  virtual void OnDeleteItem(CFileItemPtr pItem);
  virtual void OnDeleteItem(int iItem);
  virtual void DoSearch(const CStdString& strSearch, CFileItemList& items) {};
  virtual CStdString GetStartFolder(const CStdString &dir);

  virtual CStdString GetQuickpathName(const CStdString& strPath) const {return strPath;};

  bool OnClick(int iItem);
  void OnRestartItem(int iItem);
  void OnResumeItem(int iItem);
  void PlayItem(int iItem);
  virtual bool OnPlayMedia(int iItem);
  void LoadPlayList(const CStdString& strPlayList, int iPlayList = PLAYLIST_VIDEO);

  bool ShowIMDB(CFileItem *item, const SScraperInfo& info);

  void OnManualIMDB();
  bool CheckMovie(const CStdString& strFileName);

  void AddItemToPlayList(const CFileItemPtr &pItem, CFileItemList &queuedItems);
  void GetStackedFiles(const CStdString &strFileName, std::vector<CStdString> &movies);

  void OnSearch();
  void OnSearchItemFound(const CFileItem* pSelItem);
  int GetScraperForItem(CFileItem *item, SScraperInfo &info, VIDEO::SScanSettings& settings);

  CGUIDialogProgress* m_dlgProgress;
  CVideoDatabase m_database;

  CVideoThumbLoader m_thumbLoader;
};
