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

#include "pictures/GUIViewStatePictures.h"
#include "GUIBaseContainer.h"
#include "FileItem.h"
#include "ViewState.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "FileSystem/Directory.h"
#include "FileSystem/PluginDirectory.h"
#include "Util.h"
#include "LocalizeStrings.h"

using namespace XFILE;

CGUIViewStateWindowPictures::CGUIViewStateWindowPictures(const CFileItemList& items) : CGUIViewState(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS());
    AddSortMethod(SORT_METHOD_DRIVE_TYPE, 564, LABEL_MASKS());
    SetSortMethod(SORT_METHOD_LABEL);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SORT_ORDER_ASC);
  }
  else
  {
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | Foldername, empty
    AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // Filename, Size | Foldername, Size
    AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Filename, Date | Foldername, Date
    AddSortMethod(SORT_METHOD_FILE, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | FolderName, empty

    SetSortMethod(g_settings.m_viewStatePictures.m_sortMethod);
    SetViewAsControl(g_settings.m_viewStatePictures.m_viewMode);
    SetSortOrder(g_settings.m_viewStatePictures.m_sortOrder);
  }
  LoadViewState(items.GetPath(), WINDOW_PICTURES);
}

void CGUIViewStateWindowPictures::SaveViewState()
{
    SaveViewToDb(m_items.GetPath(), WINDOW_PICTURES, &g_settings.m_viewStatePictures);
}

CStdString CGUIViewStateWindowPictures::GetLockType()
{
  return "pictures";
}

bool CGUIViewStateWindowPictures::UnrollArchives()
{
  return g_guiSettings.GetBool("filelists.unrollarchives");
}

CStdString CGUIViewStateWindowPictures::GetExtensions()
{
  if (g_guiSettings.GetBool("pictures.showvideos"))
    return g_settings.m_pictureExtensions+"|"+g_settings.m_videoExtensions;

  return g_settings.m_pictureExtensions;
}

VECSOURCES& CGUIViewStateWindowPictures::GetSources()
{
  // plugins share
  if (CPluginDirectory::HasPlugins("pictures") && g_advancedSettings.m_bVirtualShares)
  {
    CMediaSource share;
    share.strName = g_localizeStrings.Get(1039); // Picture Plugins
    share.strPath = "plugin://pictures/";
    share.m_strThumbnailImage = CUtil::GetDefaultFolderThumb("DefaultPicturePlugins.png");
    share.m_ignore = true;
    AddOrReplace(g_settings.m_pictureSources,share);
  }
  return g_settings.m_pictureSources; 
}

