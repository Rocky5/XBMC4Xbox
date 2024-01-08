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

#include "utils/log.h"
#include "Directory.h"
#include "DirectoryFactory.h"
#include "FileDirectoryFactory.h"
#ifndef _LINUX
#include "utils/Win32Exception.h"
#endif
#include "FileItem.h"
#include "DirectoryCache.h"
#include "settings/Settings.h"

using namespace std;
using namespace XFILE;

CDirectory::CDirectory()
{}

CDirectory::~CDirectory()
{}

bool CDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items, CStdString strMask /*=""*/, bool bUseFileDirectories /* = true */, bool allowPrompting /* = false */, DIR_CACHE_TYPE cacheDirectory /* = DIR_CACHE_ONCE */, bool extFileInfo /* = true */)
{
  try
  {
    auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(strPath));
    if (!pDirectory.get())
      return false;

    // check our cache for this path
    if (g_directoryCache.GetDirectory(strPath, items, cacheDirectory == DIR_CACHE_ALWAYS))
      items.SetPath(strPath);
    else
    {
      // need to clear the cache (in case the directory fetch fails)
      // and (re)fetch the folder
      if (cacheDirectory != DIR_CACHE_NEVER)
        g_directoryCache.ClearDirectory(strPath);

      pDirectory->SetAllowPrompting(allowPrompting);
      pDirectory->SetCacheDirectory(cacheDirectory);
      pDirectory->SetUseFileDirectories(bUseFileDirectories);
      pDirectory->SetExtFileInfo(extFileInfo);

      items.SetPath(strPath);

      if (!pDirectory->GetDirectory(strPath, items))
      {
        CLog::Log(LOGERROR, "%s - Error getting %s", __FUNCTION__, strPath.c_str());
        return false;
      }

      // cache the directory, if necessary
      if (cacheDirectory != DIR_CACHE_NEVER)
        g_directoryCache.SetDirectory(strPath, items, pDirectory->GetCacheType(strPath));
    }

    // now filter for allowed files
    pDirectory->SetMask(strMask);
    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr item = items[i];
      if ((!item->m_bIsFolder && !pDirectory->IsAllowed(item->GetPath())) ||
          (item->GetPropertyBOOL("file:hidden") && !g_guiSettings.GetBool("filelists.showhidden")))
      {
        items.Remove(i);
        i--; // don't confuse loop
      }
    }

    //  Should any of the files we read be treated as a directory?
    //  Disable for database folders, as they already contain the extracted items
    if (bUseFileDirectories && !items.IsMusicDb() && !items.IsVideoDb() && !items.IsSmartPlayList())
      FilterFileDirectories(items, strMask);

    return true;
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error getting %s", __FUNCTION__, strPath.c_str());
  return false;
}

bool CDirectory::Create(const CStdString& strPath)
{
  try
  {
    auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(strPath));
    if (pDirectory.get())
      if(pDirectory->Create(strPath.c_str()))
        return true;
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error creating %s", __FUNCTION__, strPath.c_str());
  return false;
}

bool CDirectory::Exists(const CStdString& strPath)
{
  try
  {
    auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(strPath));
    if (pDirectory.get())
      return pDirectory->Exists(strPath.c_str());
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error checking for %s", __FUNCTION__, strPath.c_str());
  return false;
}

bool CDirectory::Remove(const CStdString& strPath)
{
  try
  {
    auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(strPath));
    if (pDirectory.get())
      if(pDirectory->Remove(strPath.c_str()))
        return true;
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error removing %s", __FUNCTION__, strPath.c_str());
  return false;
}

void CDirectory::FilterFileDirectories(CFileItemList &items, const CStdString &mask)
{
  for (int i=0; i< items.Size(); ++i)
  {
    CFileItemPtr pItem=items[i];
    if ((!pItem->m_bIsFolder) && (!pItem->IsInternetStream()))
    {
      auto_ptr<IFileDirectory> pDirectory(CFactoryFileDirectory::Create(pItem->GetPath(),pItem.get(),mask));
      if (pDirectory.get())
        pItem->m_bIsFolder = true;
      else
        if (pItem->m_bIsFolder)
        {
          items.Remove(i);
          i--; // don't confuse loop
        }
    }
  }
}
