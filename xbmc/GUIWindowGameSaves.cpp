/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "utils/log.h"
#include "GUIWindowGameSaves.h"
#include "Util.h"
#include "FileSystem/ZipManager.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "windows/GUIWindowFileManager.h"
#include "GUIPassword.h"
#include <fstream>
//#include "Utils/HTTP.h"  // For Download Function
#include "storage/MediaManager.h"
#include "utils/LabelFormatter.h"
#include "music/tags/MusicInfoTag.h"// todo - program tags
#include "GUIWindowManager.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogYesNo.h"
#include "FileSystem/Directory.h"
#include "FileItem.h"
#include "utils/URIUtils.h"
#include "LocalizeStrings.h"
#include "utils/CharsetConverter.h"

using namespace XFILE;
using namespace XFILE;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_LABELFILES        12

CGUIWindowGameSaves::CGUIWindowGameSaves()
: CGUIMediaWindow(WINDOW_GAMESAVES, "MyGameSaves.xml")
{

}

CGUIWindowGameSaves::~CGUIWindowGameSaves()
{
}

void CGUIWindowGameSaves::GoParentFolder()
{
  if (m_history.GetParentPath() == m_vecItems->GetPath())
    m_history.RemoveParentPath();
  CStdString strParent=m_history.RemoveParentPath();
  CStdString strOldPath(m_vecItems->GetPath());
  CStdString strPath(m_strParentPath);
  VECSOURCES shares;
  bool bIsSourceName = false;

  SetupShares();
  m_rootDir.GetSources(shares);

  int iIndex = CUtil::GetMatchingSource(strPath, shares, bIsSourceName);

  if (iIndex > -1)
  {
    if (strPath.size() == 2)
      if (strPath[1] == ':')
        URIUtils::AddSlashAtEnd(strPath);
    Update(strPath);
  }
  else
  {
    Update(strParent);
  }
  if (!g_guiSettings.GetBool("filelists.fulldirectoryhistory"))
      m_history.RemoveSelectedItem(strOldPath); //Delete current path
}

bool CGUIWindowGameSaves::OnClick(int iItem)
{
  if (!m_vecItems->Get(iItem)->m_bIsFolder)
  {
    OnPopupMenu(iItem);
    return true;
  }

  return CGUIMediaWindow::OnClick(iItem);
}

bool CGUIWindowGameSaves::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == CONTROL_BTNSORTBY)
      {
        if (CGUIMediaWindow::OnMessage(message))
        {
          LABEL_MASKS labelMasks;
          m_guiState->GetSortMethodLabelMasks(labelMasks);
          CLabelFormatter formatter("", labelMasks.m_strLabel2File);
          for (int i=0;i<m_vecItems->Size();++i)
          {
            CFileItemPtr item = m_vecItems->Get(i);
            formatter.FormatLabel2(item.get());
          }
          return true;
        }
        else
          return false;
      }
    }
  case GUI_MSG_WINDOW_INIT:
    {
      CStdString strDestination = message.GetStringParam();
      if (m_vecItems->GetPath() == "?")
        m_vecItems->SetPath("E:\\udata");

      m_rootDir.SetMask("/");
      VECSOURCES shares;
      bool bIsSourceName = false;
      SetupShares();
      m_rootDir.GetSources(shares);
      int iIndex = CUtil::GetMatchingSource(strDestination, shares, bIsSourceName);
      // if bIsSourceName == True,   chances are its "Local GameSaves" or something else :D)
      if (iIndex > -1)
      {
        bool bDoStuff = true;
        if (shares[iIndex].m_iHasLock == 2)
        {
          CFileItem item(shares[iIndex]);
          if (!g_passwordManager.IsItemUnlocked(&item,"programs"))
          {
            m_vecItems->SetPath(""); // no u don't
            bDoStuff = false;
            CLog::Log(LOGINFO, "  Failure! Failed to unlock destination path: %s", strDestination.c_str());
          }
        }
        if (bIsSourceName)
        {
          m_vecItems->SetPath(shares[iIndex].strPath);
        }
        else if (CDirectory::Exists(strDestination))
        {
          m_vecItems->SetPath(strDestination);
        }
      }
      return CGUIMediaWindow::OnMessage(message);
    }
    break;
  }
  return CGUIMediaWindow::OnMessage(message);
}


bool CGUIWindowGameSaves::OnPlayMedia(int iItem)
{
  CFileItemPtr pItem=m_vecItems->Get(iItem);
  CStdString strPath = pItem->GetPath();
  return true;
}

bool CGUIWindowGameSaves::GetDirectory(const CStdString& strDirectory, CFileItemList& items)
{
  if (!m_rootDir.GetDirectory(strDirectory,items,false))
    return false;

  CLog::Log(LOGDEBUG,"CGUIWindowGameSaves::GetDirectory (%s)", strDirectory.c_str());

  CStdString strParentPath;
  bool bParentExists = URIUtils::GetParentPath(strDirectory, strParentPath);
  if (bParentExists)
    m_strParentPath = strParentPath;
  else
    m_strParentPath = "";
  //FILE *newfile;
  CFile newfile;
  // flatten any folders with 1 save
  DWORD dwTick=timeGetTime();
  bool bProgressVisible = false;
  CGUIDialogProgress* m_dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  LABEL_MASKS labelMasks;
  m_guiState->GetSortMethodLabelMasks(labelMasks);
  CLabelFormatter formatter("", labelMasks.m_strLabel2File);
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items[i];

    if (!bProgressVisible && timeGetTime()-dwTick>1500 && m_dlgProgress)
    { // tag loading takes more then 1.5 secs, show a progress dialog
      m_dlgProgress->SetHeading(189);
      m_dlgProgress->SetLine(0, 20120);
      m_dlgProgress->SetLine(1,"");
      m_dlgProgress->SetLine(2, "");
      m_dlgProgress->StartModal();
      bProgressVisible = true;
    }
    if (!item->m_bIsFolder)
    {
      items.Remove(i);
      i--;
      continue;
    }

    if (item->m_bIsFolder && !item->IsParentFolder() && !item->m_bIsShareOrDrive)
    {
      if (bProgressVisible)
      {
        m_dlgProgress->SetLine(2,item->GetLabel());
        m_dlgProgress->Progress();
      }

      CStdString titlemetaXBX;
      CStdString savemetaXBX;
      URIUtils::AddFileToFolder(item->GetPath(), "titlemeta.xbx", titlemetaXBX);
      URIUtils::AddFileToFolder(item->GetPath(), "savemeta.xbx", savemetaXBX);
      int domode = 0;
      if (CFile::Exists(titlemetaXBX))
      {
        domode = 1;
        newfile.Open(titlemetaXBX);
      }
      else if (CFile::Exists(savemetaXBX))
      {
        domode = 2;
        newfile.Open(savemetaXBX);
      }
      if (domode != 0)
      {
        WCHAR *yo = new WCHAR[(int)newfile.GetLength()+1];
        newfile.Read(yo,newfile.GetLength());
        yo[newfile.GetLength()] = L'\0';
        CStdString strDescription;
        g_charsetConverter.wToUTF8(yo, strDescription);
        int poss = strDescription.find("Name=");
        if (poss == -1)
        {
          char *chrtxt = new char[(int)newfile.GetLength()+2];
          newfile.Seek(0);
          newfile.Read(chrtxt,newfile.GetLength());
          chrtxt[(int)newfile.GetLength()+1] = '\n';
          g_charsetConverter.wToUTF8(chrtxt, strDescription);
          poss = strDescription.find("Name=");
        }
        newfile.Close();
        int pose = strDescription.find("\n",poss+1);
        strDescription = strDescription.Mid(poss+5, pose - poss-6);
        strDescription = CUtil::MakeLegalFileName(strDescription, LEGAL_NONE);

        if (domode == 1)
        {
          CLog::Log(LOGDEBUG, "Loading GameSave info from %s,  data is %s, located at %i to %i",  titlemetaXBX.c_str(),strDescription.c_str(),poss,pose);
          // check for subfolders with savemeta.xbx
          CFileItemList items2;
          m_rootDir.GetDirectory(item->GetPath(),items2,false);
          int j;
          for (j=0;j<items2.Size();++j)
          {
            if (items2[j]->m_bIsFolder)
            {
              CStdString strPath;
              URIUtils::AddFileToFolder(items2[j]->GetPath(),"savemeta.xbx",strPath);
              if (CFile::Exists(strPath))
                break;
            }
          }
          if (j == items2.Size())
          {
            item->m_bIsFolder = false;
            item->SetPath(titlemetaXBX);
          }
        }
        else
        {
          CLog::Log(LOGDEBUG, "Loading GameSave info from %s,  data is %s, located at %i to %i", savemetaXBX.c_str(),strDescription.c_str(),poss,pose);
          item->m_bIsFolder = false;
          item->SetPath(savemetaXBX);
        }
        item->GetMusicInfoTag()->SetTitle(item->GetLabel());  // Note we set ID as the TITLE to save code makign a SORT ID and a ID varible to the FileItem
        item->SetLabel(strDescription);
        item->SetIconImage("defaultProgram.png");
        formatter.FormatLabel2(item.get());
        item->SetLabelPreformated(true);
      }
    }
  }
  items.SetGameSavesThumbs();
  if (bProgressVisible)
    m_dlgProgress->Close();

  return true;
}

/*
bool CGUIWindowGameSaves::DownloadSaves(CFileItem item)
{
  CStdString strURL;
  string theHtml;
  CHTTP http;

  strURL.Format("http://www.xboxmediacenter.com/xbmc.php?gameid=%s",item.m_musicInfoTag.GetTitle()); // donnos little fix the unleashx.php is broken (content lenght is greater then lenght sent)
  if (http.Get(strURL, theHtml))
  {
    TiXmlDocument gsXml;
    gsXml.Parse(theHtml.c_str(), 0);
    TiXmlElement *pRootElement = gsXml.RootElement();
    if (pRootElement)
    {
      if (pRootElement->Value())
      {
        if (strcmpi(pRootElement->Value(),"xboxsaves") == 0)
        {
          TiXmlElement* pGame = pRootElement->FirstChildElement("game");
          if (pGame)
          {
            TiXmlElement* pSave = pGame->FirstChildElement("save");
            if (pSave->Value())
            {
              CGUIDialogSelect *pDlg = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
              if (pDlg)
              {
                pDlg->SetHeading(20320);
                pDlg->Reset();
                pDlg->EnableButton(false);
                std::vector<CStdString> vecSaveUrl;
                vecSaveUrl.clear();
                while (pSave)
                {
                  if (pSave->Value())
                  {
                      pDlg->Add(pSave->Attribute("desc"));
                      vecSaveUrl.push_back(pSave->Attribute("url"));
                  }
                  pSave = pSave->NextSiblingElement("save");
                }
                //pDlg->Sort();
                pDlg->DoModal();
                int iSelectedSave = pDlg->GetSelectedLabel();
                CLog::Log(LOGINFO,"GSM: %i",  iSelectedSave);
                if (iSelectedSave > -1)
                {
                  CLog::Log(LOGINFO,"GSM vecSaveUrl[i] : %s",  vecSaveUrl[iSelectedSave].c_str());
                  if (http.Download(vecSaveUrl[iSelectedSave], "Z:\\gamesave.zip"))
                  {
                    if (g_ZipManager.ExtractArchive("Z:\\gamesave.zip","E:\\"))
                    {
                      ::DeleteFile("E:\\gameid.ini");   // delete file E:\\gameid.ini artifcat continatin info about the save we got
                      CGUIDialogOK::ShowAndGetInput(20317, 0, 20318, 0);
                      return true;
                    }
                  }
                  CGUIDialogOK::ShowAndGetInput(20317, 0, 20319, 0);  // Download Failed
                  CLog::Log(LOGINFO,"GSM: Failed to download: %s",  vecSaveUrl[iSelectedSave].c_str());
                  return true;
                }
                CLog::Log(LOGINFO,"GSM: Action Aborted: %s",  vecSaveUrl[iSelectedSave].c_str());
                return true;
              }
            }
          }
        }
      }
    }
  }
  return false;
}*/

void CGUIWindowGameSaves::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size() || m_vecItems->Get(itemNumber)->m_bIsShareOrDrive)
    return;

  buttons.Add(CONTEXT_BUTTON_COPY, 115);
  buttons.Add(CONTEXT_BUTTON_MOVE, 116);
  buttons.Add(CONTEXT_BUTTON_DELETE, 117);
  // Only add if we are on E:/udata/
  // CStdString strFileName = URIUtils::GetFileName(m_vecItems->Get(iItem)->GetPath());
  // if (!strFileName.Equals("savemeta.xbx"))
  //   buttons.Add(CONTEXT_BUTTON_DOWNLOAD, 20317);
}

bool CGUIWindowGameSaves::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return false;

  VECSOURCES localMemShares;
  CVirtualDirectory dir;

  CMediaSource share;
  share.strName = "Local GameSaves";
  share.strPath = "E:\\udata";
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  localMemShares.push_back(share);

  g_mediaManager.GetLocalDrives(localMemShares);
  dir.SetSources(localMemShares);
  dir.GetSources(localMemShares);

  CFileItem item(*m_vecItems->Get(itemNumber));
  CStdString strFileName = URIUtils::GetFileName(item.GetPath());
  switch (button)
  {
  case CONTEXT_BUTTON_COPY:
    {
      CStdString value;
      if (!CGUIDialogFileBrowser::ShowAndGetDirectory(localMemShares, g_localizeStrings.Get(20328), value, true))
        return true;
      if (!CGUIDialogYesNo::ShowAndGetInput(120,123,20022,20022)) // enable me for confirmation
        return true;
      CStdString path;
      if (strFileName.Equals("savemeta.xbx") || strFileName.Equals("titlemeta.xbx") )
      {
        CStdString itemPath;
        URIUtils::GetDirectory(m_vecItems->Get(itemNumber)->GetPath(),itemPath);
        item.SetPath(itemPath);
        item.m_bIsFolder = true;
        // first copy the titlemeta dir
        CFileItemList items2;
        CStdString strParent;
        URIUtils::GetParentPath(item.GetPath(),strParent);
        CDirectory::GetDirectory(strParent,items2);
        URIUtils::AddFileToFolder(value,URIUtils::GetFileName(strParent),path);
        for (int j=0;j<items2.Size();++j)
        {
          if (!items2[j]->m_bIsFolder)
          {
            CStdString strDest;
            URIUtils::AddFileToFolder(path,URIUtils::GetFileName(items2[j]->GetPath()),strDest);
            CFile::Cache(items2[j]->GetPath(),strDest);
          }
        }
        URIUtils::AddFileToFolder(path,URIUtils::GetFileName(item.GetPath()),path);
      }
      else
      {
        URIUtils::AddFileToFolder(value,URIUtils::GetFileName(item.GetPath()),path);
      }

      item.Select(true);
      CLog::Log(LOGDEBUG,"GSM: Copy of folder confirmed for folder %s",  item.GetPath().c_str());
      CGUIWindowFileManager::CopyItem(&item,path,true);
      return true;
    }
  case CONTEXT_BUTTON_DELETE:
    {
      CLog::Log(LOGDEBUG,"GSM: Deletion of folder confirmed for folder %s", item.GetPath().c_str());
      if (strFileName.Equals("savemeta.xbx") || strFileName.Equals("titlemeta.xbx"))
      {
        CStdString itemPath;
        URIUtils::GetDirectory(m_vecItems->Get(itemNumber)->GetPath(),itemPath);
        item.SetPath(itemPath);
        item.m_bIsFolder = true;
      }

      item.Select(true);
      if (CGUIWindowFileManager::DeleteItem(&item))
      {
        CFile::Delete(item.GetThumbnailImage());
        Update(m_vecItems->GetPath());
      }
      return true;
    }
  case CONTEXT_BUTTON_MOVE:
    {
      CStdString value;
      if (!CGUIDialogFileBrowser::ShowAndGetDirectory(localMemShares, g_localizeStrings.Get(20328) , value,true))
        return true;
      if (!CGUIDialogYesNo::ShowAndGetInput(121,124,20022,20022)) // enable me for confirmation
        return true;
      CStdString path;
      if (strFileName.Equals("savemeta.xbx") || strFileName.Equals("titlemeta.xbx"))
      {
        CStdString itemPath;
        URIUtils::GetDirectory(m_vecItems->Get(itemNumber)->GetPath(),itemPath);
        item.SetPath(itemPath);
        item.m_bIsFolder = true;
        // first copy the titlemeta dir
        CFileItemList items2;
        CStdString strParent;
        URIUtils::GetParentPath(item.GetPath(),strParent);
        CDirectory::GetDirectory(strParent,items2);

        URIUtils::AddFileToFolder(value,URIUtils::GetFileName(strParent),path);
        for (int j=0;j<items2.Size();++j) // only copy main title stuff
        {
          if (!items2[j]->m_bIsFolder)
          {
            CStdString strDest;
            URIUtils::AddFileToFolder(path,URIUtils::GetFileName(items2[j]->GetPath()),strDest);
            CFile::Cache(items2[j]->GetPath(),strDest);
          }
        }

        URIUtils::AddFileToFolder(path,URIUtils::GetFileName(item.GetPath()),path);
      }
      else
      {
        URIUtils::AddFileToFolder(value,URIUtils::GetFileName(item.GetPath()),path);
      }

      item.Select(true);
      CLog::Log(LOGDEBUG,"GSM: Copy of folder confirmed for folder %s",  item.GetPath().c_str());
      CGUIWindowFileManager::MoveItem(&item,path,true);
      CDirectory::Remove(item.GetPath());
      Update(m_vecItems->GetPath());
      return true;
    }
  /*
  case CONTEXT_BUTTON_DOWNLOAD:
    {
      CHTTP http;
      CStdString strURL;
      if (item.m_musicInfoTag.GetTitle() != "")
      {
        if (!CGUIWindowGameSaves::DownloadSaves(item))
        {
          CGUIDialogOK::ShowAndGetInput(20317, 0, 20321, 0);  // No Saves found
          CLog::Log(LOGINFO,"GSM: No saves available for game on internet: %s",  item.GetLabel().c_str());
        }
        else
        {
          Update(m_vecItems->GetPath());
        }
      }
      return true;
    }*/
   default:
     break;
  }
  return false;
}
