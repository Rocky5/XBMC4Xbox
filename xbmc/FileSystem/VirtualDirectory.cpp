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
#include "VirtualDirectory.h"
#include "DirectoryFactory.h"
#include "Util.h"
#include "settings/Profile.h"
#include "Directory.h"
#include "DirectoryCache.h"
#ifdef HAS_XBOX_HARDWARE
#include "utils/MemoryUnitManager.h"
#endif
#include "storage/DetectDVDType.h"
#include "FileSystem/File.h"
#include "FileItem.h"
#include "utils/URIUtils.h"

using namespace XFILE;

namespace XFILE
{

CVirtualDirectory::CVirtualDirectory(void)
{
  m_allowPrompting = true;  // by default, prompting is allowed.
  m_cacheDirectory = DIR_CACHE_ONCE;  // by default, caching is done.
  m_allowNonLocalSources = true;
}

CVirtualDirectory::~CVirtualDirectory(void)
{}

/*!
 \brief Add shares to the virtual directory
 \param VECSOURCES Shares to add
 \sa CMediaSource, VECSOURCES
 */
void CVirtualDirectory::SetSources(const VECSOURCES& vecSources)
{
  m_vecSources = vecSources;
}

/*!
 \brief Retrieve the shares or the content of a directory.
 \param strPath Specifies the path of the directory to retrieve or pass an empty string to get the shares.
 \param items Content of the directory.
 \return Returns \e true, if directory access is successfull.
 \note If \e strPath is an empty string, the share \e items have thumbnails and icons set, else the thumbnails
    and icons have to be set manually.
 */

bool CVirtualDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  return GetDirectory(strPath,items,true);
}
bool CVirtualDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items, bool bUseFileDirectories)
{
  VECSOURCES shares;
  GetSources(shares);
  if (!strPath.IsEmpty() && strPath != "files://")
    return CDirectory::GetDirectory(strPath, items, m_strFileMask, bUseFileDirectories, m_allowPrompting, m_cacheDirectory, m_extFileInfo);

  // if strPath is blank, clear the list (to avoid parent items showing up)
  if (strPath.IsEmpty())
    items.Clear();

  // return the root listing
  items.SetPath(strPath);

  // grab our shares
  for (unsigned int i = 0; i < shares.size(); ++i)
  {
    CMediaSource& share = shares[i];
    CFileItemPtr pItem(new CFileItem(share));
    if (pItem->IsLastFM() || pItem->IsShoutCast() || (pItem->GetPath().Left(14).Equals("musicsearch://")))
      pItem->SetCanQueue(false);
    CStdString strPathUpper = pItem->GetPath();
    strPathUpper.ToUpper();

    CStdString strIcon;
    // We have the real DVD-ROM, set icon on disktype
    if (share.m_iDriveType == CMediaSource::SOURCE_TYPE_DVD && share.m_strThumbnailImage.IsEmpty())
    {
      CUtil::GetDVDDriveIcon( pItem->GetPath(), strIcon );
      // CDetectDVDMedia::SetNewDVDShareUrl() caches disc thumb as special://temp/dvdicon.tbn
      CStdString strThumb = "special://temp/dvdicon.tbn";
      if (XFILE::CFile::Exists(strThumb))
        pItem->SetThumbnailImage(strThumb);
    }
    else if (strPathUpper.Left(11) == "SOUNDTRACK:")
      strIcon = "DefaultHardDisk.png";
    else if (pItem->IsLastFM()
          || pItem->IsShoutCast()
          || pItem->IsVideoDb()
          || pItem->IsMusicDb()
          || pItem->IsPlugin()
          || pItem->IsPluginRoot()
          || pItem->GetPath() == "special://musicplaylists/"
          || pItem->GetPath() == "special://videoplaylists/"
          || pItem->GetPath() == "musicsearch://")
      strIcon = "DefaultFolder.png";
    else if (pItem->IsRemote())
      strIcon = "DefaultNetwork.png";
    else if (pItem->IsISO9660())
      strIcon = "DefaultDVDRom.png";
    else if (pItem->IsDVD())
      strIcon = "DefaultDVDRom.png";
    else if (pItem->IsCDDA())
      strIcon = "DefaultCDDA.png";
    else
      strIcon = "DefaultHardDisk.png";

    pItem->SetIconImage(strIcon);
    if (share.m_iHasLock == 2 && g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
      pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_LOCKED);
    else
      pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_NONE);

    items.Add(pItem);
  }

  return true;
}

/*!
 \brief Is the share \e strPath in the virtual directory.
 \param strPath Share to test
 \return Returns \e true, if share is in the virtual directory.
 \note The parameter \e strPath can not be a share with directory. Eg. "iso9660://dir" will return \e false.
    It must be "iso9660://".
 */
bool CVirtualDirectory::IsSource(const CStdString& strPath, VECSOURCES *sources, CStdString *name) const
{
  CStdString strPathCpy = strPath;
  strPathCpy.TrimRight("/");
  strPathCpy.TrimRight("\\");

  // just to make sure there's no mixed slashing in share/default defines
  // ie. f:/video and f:\video was not be recognised as the same directory,
  // resulting in navigation to a lower directory then the share.
  if(URIUtils::IsDOSPath(strPathCpy))
    strPathCpy.Replace("/", "\\");

  VECSOURCES shares;
  if (sources)
    shares = *sources;
  else
    GetSources(shares);
  for (int i = 0; i < (int)shares.size(); ++i)
  {
    const CMediaSource& share = shares.at(i);
    CStdString strShare = share.strPath;
    strShare.TrimRight("/");
    strShare.TrimRight("\\");
    if(URIUtils::IsDOSPath(strShare))
      strShare.Replace("/", "\\");
    if (strShare == strPathCpy)
    {
      if (name)
        *name = share.strName;
      return true;
    }
  }
  return false;
}

/*!
 \brief Is the share \e path in the virtual directory.
 \param path Share to test
 \return Returns \e true, if share is in the virtual directory.
 \note The parameter \e path CAN be a share with directory. Eg. "iso9660://dir" will
       return the same as "iso9660://".
 */
bool CVirtualDirectory::IsInSource(const CStdString &path) const
{
  bool isSourceName;
  VECSOURCES shares;
  GetSources(shares);
  int iShare = CUtil::GetMatchingSource(path, shares, isSourceName);
  if (URIUtils::IsOnDVD(path))
  { // check to see if our share path is still available and of the same type, as it changes during autodetect
    // and GetMatchingSource() is too naive at it's matching
    for (unsigned int i = 0; i < shares.size(); i++)
    {
      CMediaSource &share = shares[i];
      if (URIUtils::IsOnDVD(share.strPath) && share.strPath.Equals(path.Left(share.strPath.GetLength())))
        return true;
    }
    return false;
  }
  // TODO: May need to handle other special cases that GetMatchingSource() fails on
  return (iShare > -1);
}

void CVirtualDirectory::GetSources(VECSOURCES &shares) const
{
  shares = m_vecSources;
  // add our plug n play shares

  if (m_allowNonLocalSources)
  {
#ifdef HAS_XBOX_HARDWARE
    g_memoryUnitManager.GetMemoryUnitSources(shares);
#endif
    CUtil::AutoDetectionGetSource(shares);
  }

  // and update our dvd share
  for (unsigned int i = 0; i < shares.size(); ++i)
  {
    CMediaSource& share = shares[i];
    if (share.m_iDriveType == CMediaSource::SOURCE_TYPE_DVD)
    {
      share.strStatus = MEDIA_DETECT::CDetectDVDMedia::GetDVDLabel();
      share.strPath = MEDIA_DETECT::CDetectDVDMedia::GetDVDPath();
    }
  }
}
}

