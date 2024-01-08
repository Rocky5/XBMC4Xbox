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

#include "pictures/PictureThumbLoader.h"
#include "pictures/Picture.h"
#include "URL.h"
#include "FileSystem/File.h"
#include "FileItem.h"
#include "video/VideoInfoTag.h"
#include "TextureManager.h"

using namespace XFILE;

CPictureThumbLoader::CPictureThumbLoader()
{
  m_regenerateThumbs = false;  
}

CPictureThumbLoader::~CPictureThumbLoader()
{
  StopThread();
}

bool CPictureThumbLoader::LoadItem(CFileItem* pItem)
{
  pItem->SetCachedPictureThumb();
  if (pItem->m_bIsShareOrDrive) return true;
  
  if(pItem->HasThumbnail())
  {
    CStdString thumb(pItem->GetThumbnailImage());

    // look for remote thumbs    
    if (!g_TextureManager.CanLoad(thumb))
    {
      CStdString cachedThumb(pItem->GetCachedPictureThumb());
      if(CFile::Exists(cachedThumb))
        pItem->SetThumbnailImage(cachedThumb);
      else
      {
        // see if we have additional info to download this thumb with
        if (pItem->HasVideoInfoTag())
          return DownloadVideoThumb(pItem, cachedThumb);
        else
        {
          CPicture pic;
          if(pic.CreateThumbnail(thumb, cachedThumb))
            pItem->SetThumbnailImage(cachedThumb);
          else
            pItem->SetThumbnailImage("");
        }
      }
    }
    else if (m_regenerateThumbs)
    {
      CFile::Delete(thumb);
      pItem->SetThumbnailImage("");
    }
  }

  if ((pItem->IsPicture() && !pItem->IsZIP() && !pItem->IsRAR() && !pItem->IsCBZ() && !pItem->IsCBR() && !pItem->IsPlayList()) && !pItem->HasThumbnail())
  { // load the thumb from the image file
    CPicture pic;
    pic.CreateThumbnail(pItem->GetPath(), pItem->GetCachedPictureThumb());
  }
  // refill in the thumb to get it to update
  pItem->SetCachedPictureThumb();
  pItem->FillInDefaultIcon();
  return true;
}

void CPictureThumbLoader::OnLoaderFinish()
{
  m_regenerateThumbs = false;
}

bool CPictureThumbLoader::DownloadVideoThumb(CFileItem *item, const CStdString &cachedThumb)
{
  if (item->GetVideoInfoTag()->m_strPictureURL.m_url.size())
  { // yep - download using this thumb
    if (CScraperUrl::DownloadThumbnail(cachedThumb, item->GetVideoInfoTag()->m_strPictureURL.m_url[0]))
      item->SetThumbnailImage(cachedThumb);
    else
      item->SetThumbnailImage("");
  }
  else if (item->GetVideoInfoTag()->m_fanart.GetNumFanarts() > 0 && item->HasProperty("fanart_number"))
  { // yep - download our fanart preview
    if (item->GetVideoInfoTag()->m_fanart.DownloadThumb(item->GetPropertyInt("fanart_number"), cachedThumb))
      item->SetThumbnailImage(cachedThumb);
    else
      item->SetThumbnailImage("");
  }
  if (item->HasProperty("labelonthumbload"))
    item->SetLabel(item->GetProperty("labelonthumbload"));
  return true; // we don't need to do anything else here
}
