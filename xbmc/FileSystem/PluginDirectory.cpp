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
#include "PluginDirectory.h"
#include "utils/URIUtils.h"
#ifdef HAS_PYTHON
#include "lib/libPython/XBPython.h"
#endif
#include "../utils/SingleLock.h"
#include "PluginSettings.h"
#include "GUIWindowManager.h"
#include "dialogs/GUIDialogProgress.h"
#include "FileSystem/File.h"
#include "settings/GUISettings.h"
#include "FileItem.h"
#include "LocalizeStrings.h"

using namespace XFILE;
using namespace std;

vector<CPluginDirectory *> CPluginDirectory::globalHandles;
CCriticalSection CPluginDirectory::m_handleLock;

CPluginDirectory::CPluginDirectory(void)
{
  m_fetchComplete = CreateEvent(NULL, false, false, NULL);
  m_listItems = new CFileItemList;
  m_fileResult = new CFileItem;
}

CPluginDirectory::~CPluginDirectory(void)
{
  CloseHandle(m_fetchComplete);
  delete m_listItems;
  delete m_fileResult;
}

int CPluginDirectory::getNewHandle(CPluginDirectory *cp)
{
  CSingleLock lock(m_handleLock);
  int handle = (int)globalHandles.size();
  globalHandles.push_back(cp);
  return handle;
}

void CPluginDirectory::removeHandle(int handle)
{
  CSingleLock lock(m_handleLock);
  if (handle >= 0 && handle < (int)globalHandles.size())
    globalHandles.erase(globalHandles.begin() + handle);
}

bool CPluginDirectory::StartScript(const CStdString& strPath)
{
  CURL url(strPath);

  CStdString fileName;
  
  // path is special://home/plugins/<path from here>
  CStdString pathToScript = "special://home/plugins/";
  URIUtils::AddFileToFolder(pathToScript, url.GetHostName(), pathToScript);
  URIUtils::AddFileToFolder(pathToScript, url.GetFileName(), pathToScript);
  URIUtils::AddFileToFolder(pathToScript, "default.py", pathToScript);

  // base path
  CStdString basePath = "plugin://";
  URIUtils::AddFileToFolder(basePath, url.GetHostName(), basePath);
  URIUtils::AddFileToFolder(basePath, url.GetFileName(), basePath);

  // options
  CStdString options = url.GetOptions();
  URIUtils::RemoveSlashAtEnd(options); // This MAY kill some scripts (eg though with a URL ending with a slash), but
                                    // is needed for all others, as XBMC adds slashes to "folders"

  // Load the plugin settings
  CLog::Log(LOGDEBUG, "%s - URL for plugin settings: %s", __FUNCTION__, url.GetFileName().c_str() );
  g_currentPluginSettings.Load(url);

  // Load language strings
  LoadPluginStrings(url);

  // reset our wait event, and grab a new handle
  ResetEvent(m_fetchComplete);
  int handle = getNewHandle(this);

  // clear out our status variables
  m_fileResult->Reset();
  m_listItems->Clear();
  m_listItems->SetPath(strPath);
  m_cancelled = false;
  m_success = false;
  m_totalItems = 0;

  // setup our parameters to send the script
  CStdString strHandle;
  strHandle.Format("%i", handle);
  const char *plugin_argv[] = {basePath.c_str(), strHandle.c_str(), options.c_str(), NULL };

  // run the script
  CLog::Log(LOGDEBUG, "%s - calling plugin %s('%s','%s','%s')", __FUNCTION__, pathToScript.c_str(), plugin_argv[0], plugin_argv[1], plugin_argv[2]);
  bool success = false;
#ifdef HAS_PYTHON
  if (g_pythonParser.evalFile(pathToScript.c_str(), 3, (const char**)plugin_argv) >= 0)
  { // wait for our script to finish
    CStdString scriptName = url.GetFileName();
    URIUtils::RemoveSlashAtEnd(scriptName);
    success = WaitOnScriptResult(pathToScript, scriptName);
  }
  else
#endif
    CLog::Log(LOGERROR, "Unable to run plugin %s", pathToScript.c_str());

  // free our handle
  removeHandle(handle);

  return success;
}

bool CPluginDirectory::GetPluginResult(const CStdString& strPath, CFileItem &resultItem)
{
  CPluginDirectory* newDir = new CPluginDirectory();

  bool success = newDir->StartScript(strPath);

  if (success)
  { // update the play path, saving the old one as needed
    if (!resultItem.HasProperty("original_listitem_url"))
      resultItem.SetProperty("original_listitem_url", resultItem.GetPath());
    resultItem.SetPath(newDir->m_fileResult->GetPath());
    resultItem.SetMimeType(newDir->m_fileResult->GetMimeType(false));
  }
  delete newDir;

  return success;
}

bool CPluginDirectory::AddItem(int handle, const CFileItem *item, int totalItems)
{
  CSingleLock lock(m_handleLock);
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, " %s - called with an invalid handle.", __FUNCTION__);
    return false;
  }
  
  CPluginDirectory *dir = globalHandles[handle];
  CFileItemPtr pItem(new CFileItem(*item));
  dir->m_listItems->Add(pItem);
  dir->m_totalItems = totalItems;

  return !dir->m_cancelled;
}

bool CPluginDirectory::AddItems(int handle, const CFileItemList *items, int totalItems)
{
  CSingleLock lock(m_handleLock);
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, " %s - called with an invalid handle.", __FUNCTION__);
    return false;
  }
  
  CPluginDirectory *dir = globalHandles[handle];
  CFileItemList pItemList = *items;
  dir->m_listItems->Append(pItemList);
  dir->m_totalItems = totalItems;

  return !dir->m_cancelled;
}

void CPluginDirectory::EndOfDirectory(int handle, bool success, bool replaceListing, bool cacheToDisc)
{
  CSingleLock lock(m_handleLock);
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, " %s - called with an invalid handle.", __FUNCTION__);
    return;
  }
  CPluginDirectory *dir = globalHandles[handle];

  // set cache to disc
  dir->m_listItems->SetCacheToDisc(cacheToDisc ? CFileItemList::CACHE_IF_SLOW : CFileItemList::CACHE_NEVER);

  dir->m_success = success;
  dir->m_listItems->SetReplaceListing(replaceListing);

  if (!dir->m_listItems->HasSortDetails())
    dir->m_listItems->AddSortMethod(SORT_METHOD_NONE, 552, LABEL_MASKS("%L", "%D"));

  // Unload temporary language strings
  ClearPluginStrings();

  // set the event to mark that we're done
  SetEvent(dir->m_fetchComplete);
}

void CPluginDirectory::AddSortMethod(int handle, SORT_METHOD sortMethod, const CStdString &label2Mask)
{
  CSingleLock lock(m_handleLock);
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, "%s - called with an invalid handle.", __FUNCTION__);
    return;
  }

  CPluginDirectory *dir = globalHandles[handle];

  // TODO: Add all sort methods and fix which labels go on the right or left
  switch(sortMethod)
  {
    case SORT_METHOD_LABEL:
    case SORT_METHOD_LABEL_IGNORE_THE:
      {
        if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
          dir->m_listItems->AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%T", label2Mask));
        else
          dir->m_listItems->AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_TITLE:
    case SORT_METHOD_TITLE_IGNORE_THE:
      {
        if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
          dir->m_listItems->AddSortMethod(SORT_METHOD_TITLE_IGNORE_THE, 556, LABEL_MASKS("%T", label2Mask));
        else
          dir->m_listItems->AddSortMethod(SORT_METHOD_TITLE, 556, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_ARTIST:
    case SORT_METHOD_ARTIST_IGNORE_THE:
      {
        if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
          dir->m_listItems->AddSortMethod(SORT_METHOD_ARTIST_IGNORE_THE, 557, LABEL_MASKS("%T", "%A"));
        else
          dir->m_listItems->AddSortMethod(SORT_METHOD_ARTIST, 557, LABEL_MASKS("%T", "%A"));
        break;
      }
    case SORT_METHOD_ALBUM:
    case SORT_METHOD_ALBUM_IGNORE_THE:
      {
        if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
          dir->m_listItems->AddSortMethod(SORT_METHOD_ALBUM_IGNORE_THE, 558, LABEL_MASKS("%T", "%B"));
        else
          dir->m_listItems->AddSortMethod(SORT_METHOD_ALBUM, 558, LABEL_MASKS("%T", "%B"));
        break;
      }
    case SORT_METHOD_DATE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%T", "%J"));
        break;
      }
    case SORT_METHOD_BITRATE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_BITRATE, 623, LABEL_MASKS("%T", "%X"));
        break;
      }             
    case SORT_METHOD_SIZE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS("%T", "%I"));
        break;
      }
    case SORT_METHOD_FILE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_FILE, 561, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_TRACKNUM:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_TRACKNUM, 554, LABEL_MASKS("[%N. ]%T", label2Mask));
        break;
      }
    case SORT_METHOD_DURATION:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_DURATION, 555, LABEL_MASKS("%T", "%D"));
        break;
      }
    case SORT_METHOD_VIDEO_RATING:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_VIDEO_RATING, 563, LABEL_MASKS("%T", "%R"));
        break;
      }
    case SORT_METHOD_YEAR:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_YEAR, 562, LABEL_MASKS("%T", "%Y"));
        break;
      }
    case SORT_METHOD_SONG_RATING:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_SONG_RATING, 563, LABEL_MASKS("%T", "%R"));
        break;
      }
    case SORT_METHOD_GENRE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_GENRE, 515, LABEL_MASKS("%T", "%G"));
        break;
      }
    case SORT_METHOD_COUNTRY:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_COUNTRY, 574, LABEL_MASKS("%T", "%G"));
        break;
      }
    case SORT_METHOD_VIDEO_TITLE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_VIDEO_TITLE, 369, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_MPAA_RATING:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_MPAA_RATING, 563, LABEL_MASKS("%T", "%O"));
        break;
      }
    case SORT_METHOD_VIDEO_RUNTIME:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_VIDEO_RUNTIME, 555, LABEL_MASKS("%T", "%D"));
        break;
      }
    case SORT_METHOD_STUDIO:
    case SORT_METHOD_STUDIO_IGNORE_THE:
      {
        if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
          dir->m_listItems->AddSortMethod(SORT_METHOD_STUDIO_IGNORE_THE, 572, LABEL_MASKS("%T", "%U"));
        else
          dir->m_listItems->AddSortMethod(SORT_METHOD_STUDIO, 572, LABEL_MASKS("%T", "%U"));
        break;
      }
    case SORT_METHOD_PROGRAM_COUNT:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_PROGRAM_COUNT, 567, LABEL_MASKS("%T", "%C"));
        break;
      }
    case SORT_METHOD_UNSORTED:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_UNSORTED, 571, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_NONE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_NONE, 552, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_DRIVE_TYPE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_DRIVE_TYPE, 564, LABEL_MASKS()); // Preformatted
        break;
      }
    case SORT_METHOD_PLAYLIST_ORDER:
      {
        CStdString strTrackLeft=g_guiSettings.GetString("musicfiles.trackformat");
        CStdString strTrackRight=g_guiSettings.GetString("musicfiles.trackformatright");

        dir->m_listItems->AddSortMethod(SORT_METHOD_PLAYLIST_ORDER, 559, LABEL_MASKS(strTrackLeft, strTrackRight));
        break;
      }
    case SORT_METHOD_EPISODE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_EPISODE,20359,LABEL_MASKS("%E. %T","%R"));
        break;
      }
    case SORT_METHOD_PRODUCTIONCODE:
      {
        //dir->m_listItems.AddSortMethod(SORT_METHOD_PRODUCTIONCODE,20368,LABEL_MASKS("%E. %T","%P", "%E. %T","%P"));
        dir->m_listItems->AddSortMethod(SORT_METHOD_PRODUCTIONCODE,20368,LABEL_MASKS("%H. %T","%P", "%H. %T","%P"));
        break;
      }
    case SORT_METHOD_LISTENERS:
      {
       dir->m_listItems->AddSortMethod(SORT_METHOD_LISTENERS,20455,LABEL_MASKS("%T","%W"));
       break;
      }
   
    default:  
      break;
  }
}

bool CPluginDirectory::GetDirectory(const CStdString& strPath, CFileItemList& items)
{
  CURL url(strPath);
  if (url.GetFileName().IsEmpty())
  { // called with no script - should never happen
    return GetPluginsDirectory(url.GetHostName(), items);
  }

  bool success = this->StartScript(strPath);

  // append the items to the list
  items.Assign(*m_listItems, true); // true to keep the current items
  m_listItems->Clear();
  return success;
}

bool CPluginDirectory::RunScriptWithParams(const CStdString& strPath)
{
  CURL url(strPath);
  if (url.GetFileName().IsEmpty()) // called with no script - should never happen
    return false;

  // Load the settings incase they changed while in the plugins directory
  g_currentPluginSettings.Load(url);

  // Load language strings
  LoadPluginStrings(url);

  // path is special://home/plugins/<path from here>
  CStdString pathToScript = "special://home/plugins/";
  URIUtils::AddFileToFolder(pathToScript, url.GetHostName(), pathToScript);
  URIUtils::AddFileToFolder(pathToScript, url.GetFileName(), pathToScript);
  URIUtils::AddFileToFolder(pathToScript, "default.py", pathToScript);

  // options
  CStdString options = url.GetOptions();
  URIUtils::RemoveSlashAtEnd(options); // This MAY kill some scripts (eg though with a URL ending with a slash), but
                                    // is needed for all others, as XBMC adds slashes to "folders"
  // base path
  CStdString basePath = "plugin://";
  URIUtils::AddFileToFolder(basePath, url.GetHostName(), basePath);
  URIUtils::AddFileToFolder(basePath, url.GetFileName(), basePath);

  // setup our parameters to send the script
  CStdString strHandle;
  strHandle.Format("%i", -1);
  const char *argv[3];
  argv[0] = basePath.c_str();
  argv[1] = strHandle.c_str();
  argv[2] = options.c_str();

  // run the script
#ifdef HAS_PYTHON
  CLog::Log(LOGDEBUG, "%s - calling plugin %s('%s','%s','%s')", __FUNCTION__, pathToScript.c_str(), argv[0], argv[1], argv[2]);
  if (g_pythonParser.evalFile(pathToScript.c_str(), 3, (const char**)argv) >= 0)
    return true;
  else
#endif
    CLog::Log(LOGERROR, "Unable to run plugin %s", pathToScript.c_str());

  return false;
}

bool CPluginDirectory::HasPlugins(const CStdString &type)
{
  CStdString path = "special://home/plugins/";
  URIUtils::AddFileToFolder(path, type, path);
  CFileItemList items;
  if (CDirectory::GetDirectory(path, items, "/", false))
  {
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      if (item->m_bIsFolder && !item->IsParentFolder() && !item->m_bIsShareOrDrive)
      {
        CStdString defaultPY;
        URIUtils::AddFileToFolder(item->GetPath(), "default.py", defaultPY);
        if (XFILE::CFile::Exists(defaultPY))
          return true;
      }
    }
  }
  return false;
}

bool CPluginDirectory::GetPluginsDirectory(const CStdString &type, CFileItemList &items)
{
  // retrieve our folder
  CStdString pluginsFolder = "special://home/plugins";
  URIUtils::AddFileToFolder(pluginsFolder, type, pluginsFolder);
  URIUtils::AddSlashAtEnd(pluginsFolder);

  if (!CDirectory::GetDirectory(pluginsFolder, items, "*.py", false))
    return false;

  CStdString path = items.GetPath();
  path.Replace("special://home/plugins/", "plugin://");
  items.SetPath(path);

  // flatten any folders - TODO: Assigning of thumbs
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items[i];
    item->SetThumbnailImage("");
    item->SetCachedProgramThumb();
    if (!item->HasThumbnail())
      item->SetUserProgramThumb();
    if (!item->HasThumbnail())
    {
      CFileItem item2(item->GetPath());
      item2.SetPath(URIUtils::AddFileToFolder(item->GetPath(),"default.py"));
      item2.m_bIsFolder = false;
      item2.SetCachedProgramThumb();
      if (!item2.HasThumbnail())
        item2.SetUserProgramThumb();
      if (item2.HasThumbnail())
      {
        XFILE::CFile::Cache(item2.GetThumbnailImage(),item->GetCachedProgramThumb());
        item->SetThumbnailImage(item->GetCachedProgramThumb());
      }
    }
    path = item->GetPath();
    path.Replace("special://home/plugins/", "plugin://");
    path.Replace("\\", "/");
    item->SetPath(path);
  }
  return true;
}

bool CPluginDirectory::WaitOnScriptResult(const CStdString &scriptPath, const CStdString &scriptName)
{
  const unsigned int timeBeforeProgressBar = 1500;
  const unsigned int timeToKillScript = 1000;

  DWORD startTime = timeGetTime();
  CGUIDialogProgress *progressBar = NULL;
  
  CLog::Log(LOGDEBUG, "%s - waiting on the %s plugin...", __FUNCTION__, scriptName.c_str());
  while (true)
  {
    CSingleExit ex(g_graphicsContext);
    // check if the python script is finished
    if (WaitForSingleObject(m_fetchComplete, 20) == WAIT_OBJECT_0)
    { // python has returned
      CLog::Log(LOGDEBUG, "%s- plugin returned %s", __FUNCTION__, m_success ? "successfully" : "failure");
      break;
    }
    ex.Restore();

    // check our script is still running
#ifdef HAS_PYTHON
    int id = g_pythonParser.getScriptId(scriptPath.c_str());
    if (id == -1)
#endif
    { // check whether we exited normally
      if (WaitForSingleObject(m_fetchComplete, 0) == WAIT_TIMEOUT)
      { // python didn't return correctly
        CLog::Log(LOGDEBUG, " %s - plugin exited prematurely - terminating", __FUNCTION__);
        m_success = false;
      }
      break;
    }

    // check whether we should pop up the progress dialog
    if (!progressBar && timeGetTime() - startTime > timeBeforeProgressBar)
    { // loading takes more then 1.5 secs, show a progress dialog
      progressBar = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      // if script has shown progressbar don't override it
      if (progressBar && progressBar->IsActive())
      {
        startTime = timeGetTime();
        progressBar = NULL;
      }

      if (progressBar)
      {
        progressBar->SetHeading(scriptName);
        progressBar->SetLine(0, 1040);
        progressBar->SetLine(1, "");
        progressBar->SetLine(2, "");
        progressBar->StartModal();
      }
    }

    if (progressBar)
    { // update the progress bar and check for user cancel
      CStdString label;
      if (m_totalItems > 0)
      {
        label.Format(g_localizeStrings.Get(1042).c_str(), m_listItems->Size(), m_totalItems);
        progressBar->SetPercentage((int)((m_listItems->Size() * 100 ) / m_totalItems));
        progressBar->ShowProgressBar(true);
      }
      else
        label.Format(g_localizeStrings.Get(1041).c_str(), m_listItems->Size());
      progressBar->SetLine(2, label);
      progressBar->Progress();
      if (progressBar->IsCanceled())
      { // user has cancelled our process - cancel our process
        if (!m_cancelled)
        {
          m_cancelled = true;
          startTime = timeGetTime();
        }
        if (m_cancelled && timeGetTime() - startTime > timeToKillScript)
        { // cancel our script
#ifdef HAS_PYTHON
          int id = g_pythonParser.getScriptId(scriptPath.c_str());
          if (id != -1 && g_pythonParser.isRunning(id))
          {
            CLog::Log(LOGDEBUG, "%s- cancelling plugin %s", __FUNCTION__, scriptName.c_str());
            g_pythonParser.stopScript(id);
            break;
          }
#endif
        }
      }
    }
  }
  if (progressBar)
    progressBar->Close();

  return !m_cancelled && m_success;
}

void CPluginDirectory::SetResolvedUrl(int handle, bool success, const CFileItem *resultItem)
{
  CSingleLock lock(m_handleLock);
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, " %s - called with an invalid handle.", __FUNCTION__);
    return;
  }
  CPluginDirectory* dir  = globalHandles[handle];

  dir->m_success = success;
  *dir->m_fileResult = *resultItem;
  
  // set the event to mark that we're done
  SetEvent(dir->m_fetchComplete);
}

void CPluginDirectory::SetContent(int handle, const CStdString &strContent)
{
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, "%s called with an invalid handle.", __FUNCTION__);
    return;
  }

  CPluginDirectory *dir = globalHandles[handle];
  dir->m_listItems->SetContent(strContent);
}

void CPluginDirectory::SetProperty(int handle, const CStdString &strProperty, const CStdString &strValue)
{
  CSingleLock lock(m_handleLock);
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, "%s called with an invalid handle.", __FUNCTION__);
    return;
  }

  CPluginDirectory *dir = globalHandles[handle];
  dir->m_listItems->SetProperty(strProperty, strValue);
}

void CPluginDirectory::LoadPluginStrings(const CURL &url)
{
  // Path where the plugin resides
  CStdString pathToPlugin = "special://home/plugins/";
  URIUtils::AddFileToFolder(pathToPlugin, url.GetHostName(), pathToPlugin);
  URIUtils::AddFileToFolder(pathToPlugin, url.GetFileName(), pathToPlugin);

  // Path where the language strings reside
  CStdString pathToLanguageFile = pathToPlugin;
  URIUtils::AddFileToFolder(pathToLanguageFile, "resources", pathToLanguageFile);
  URIUtils::AddFileToFolder(pathToLanguageFile, "language", pathToLanguageFile);
  URIUtils::AddSlashAtEnd(pathToLanguageFile);

  // Load language strings temporarily
  g_localizeStringsTemp.Load(pathToLanguageFile, g_guiSettings.GetString("locale.language"));
}

void CPluginDirectory::ClearPluginStrings()
{
  // Unload temporary language strings
  g_localizeStringsTemp.Clear();
}
