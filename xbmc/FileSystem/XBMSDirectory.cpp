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


#include "XBMSDirectory.h"
#include "SectionLoader.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "FileItem.h"
#include "utils/CharsetConverter.h"

using namespace XFILE;

extern "C"
{
#include "lib/libXBMS/ccincludes.h"
 #include "lib/libXBMS/ccbuffer.h"
 #include "lib/libXBMS/ccxclient.h"
 #include "lib/libXBMS/ccxmltrans.h"
}


struct DiscoveryCallbackContext
{
  CFileItemList *items;
  const char *username;
  const char *password;
};

static void DiscoveryCallback(const char *addr, const char *port, const char *version,
                              const char *comment, void *context);

CXBMSDirectory::CXBMSDirectory(void)
{
  CSectionLoader::Load("LIBXBMS");
}

CXBMSDirectory::~CXBMSDirectory(void)
{
  CSectionLoader::Unload("LIBXBMS");
}

bool CXBMSDirectory::GetDirectory(const CStdString& strPathUtf8, CFileItemList &items)
{
  unsigned long handle;
  char *filename, *fileinfo;
  bool rv = false;

  CStdString strPath=strPathUtf8;
  g_charsetConverter.utf8ToStringCharset(strPath);

  CURL url(strPath);

  CStdString strRoot = strPath;
  URIUtils::AddSlashAtEnd(strPath);

  CcXstreamServerConnection conn = 0;

  if (strPath == "xbms://")
  {
    if (url.GetFileName() == "")
    {
      int iOldSize=items.Size();
      // Let's do the automatic discovery.
      struct DiscoveryCallbackContext dc_context;
      CStdString strPassword = url.GetPassWord();
      CStdString strUserName = url.GetUserName();

      dc_context.items = &items;
      dc_context.username = ((strUserName.c_str() != NULL) && (strlen(strUserName.c_str()) > 0)) ? strUserName.c_str() : NULL;
      dc_context.password = ((strPassword.c_str() != NULL) && (strlen(strPassword.c_str()) > 0)) ? strPassword.c_str() : NULL;
      ccx_client_discover_servers(DiscoveryCallback, (void *)(&dc_context));
      rv = S_OK;

      return (items.Size()>iOldSize);
    }
  }

  if (cc_xstream_client_connect(url.GetHostName().c_str(),
                                (url.HasPort()) ? url.GetPort() : 1400, &conn) != CC_XSTREAM_CLIENT_OK)
  {
    if (conn != 0) cc_xstream_client_disconnect(conn);

    return false;
  }

  if (cc_xstream_client_version_handshake(conn) != CC_XSTREAM_CLIENT_OK)
  {
    if (conn != 0) cc_xstream_client_disconnect(conn);

    return false;
  }

  // Authenticate here!
  CStdString strPassword = url.GetPassWord();
  CStdString strUserName = url.GetUserName();
  if (strPassword.size() && strUserName.size())
  {
    // We don't check the return value here.  If authentication
    // step fails, let's try if server lets us log in without
    // authentication.
    cc_xstream_client_password_authenticate(conn,
                                            strUserName.c_str(),
                                            strPassword.c_str() );
  }
  CStdString strFileName = url.GetFileName();
  CStdString strDir, strFile;
  strDir = "";
  if (cc_xstream_client_setcwd(conn, "/") == CC_XSTREAM_CLIENT_OK)
  {
    strFile = url.GetFileName();
    for (int i = 0; i < (int)strFile.size(); ++i)
    {
      if (strFile[i] == '/' || strFile[i] == '\\')
      {
        if (strDir != "")
        {
          if (cc_xstream_client_setcwd(conn, strDir.c_str()) != CC_XSTREAM_CLIENT_OK)
          {
            if (conn != 0) cc_xstream_client_disconnect(conn);
            return false;
          }
        }
        strDir = "";
      }
      else
      {
        strDir += strFile[i];
      }
    }
  }
  else
  {
    if (conn != 0) cc_xstream_client_disconnect(conn);
    return false;
  }
  if (strDir.size() > 0)
  {
    if (cc_xstream_client_setcwd(conn, strDir.c_str()) != CC_XSTREAM_CLIENT_OK)
    {
      if (conn != 0) cc_xstream_client_disconnect(conn);

      return false;
    }
  }

  if (cc_xstream_client_dir_open(conn, &handle) != CC_XSTREAM_CLIENT_OK)
  {
    if (conn != 0) cc_xstream_client_disconnect(conn);

    return false;
  }

  while (cc_xstream_client_dir_read(conn, handle, &filename, &fileinfo) == CC_XSTREAM_CLIENT_OK)
  {
    if (*filename == '\0')
    {
      free(filename);
      free(fileinfo);
      break;
    }
    bool bIsDirectory = false;

    if (strstr(fileinfo, "<ATTRIB>directory</ATTRIB>"))
      bIsDirectory = true;

    CStdString strLabel=filename;
    g_charsetConverter.unknownToUTF8(strLabel);
    CFileItemPtr pItem(new CFileItem(strLabel));

    char* pstrSizeStart = strstr(fileinfo, "<SIZE>");
    char* pstrSizeEnd = strstr(fileinfo, "</SIZE>");
    if (pstrSizeStart && pstrSizeEnd)
    {
      char szSize[128];
      pstrSizeStart += strlen("<SIZE>");
      strncpy(szSize, pstrSizeStart, pstrSizeEnd - pstrSizeStart);
      szSize[pstrSizeEnd - pstrSizeStart] = 0;
      pItem->m_dwSize = _atoi64(szSize);
    }

    char* pstrModificationStart = strstr(fileinfo, "<MODIFICATION>");
    char* pstrModificationEnd = strstr(fileinfo, "</MODIFICATION>");
    if (pstrModificationStart && pstrModificationEnd)
    {
      char szModification[128];
      pstrModificationStart += strlen("<MODIFICATION>");
      strncpy(szModification, pstrModificationStart, pstrModificationEnd - pstrModificationStart);
      szModification[pstrModificationEnd - pstrModificationStart] = 0;
      int64_t lTimeDate = _atoi64(szModification);

      FILETIME fileTime, localTime;
      LONGLONG ll = Int32x32To64(lTimeDate, 10000000) + 116444736000000000LL;
      fileTime.dwLowDateTime = (DWORD) (ll & 0xFFFFFFFF);
      fileTime.dwHighDateTime = (DWORD)(ll >> 32);

      FileTimeToLocalFileTime(&fileTime, &localTime);
      pItem->m_dateTime=localTime;

    }

    CStdString path;
    path = strRoot + filename;
    g_charsetConverter.unknownToUTF8(path);
    pItem->m_bIsFolder = bIsDirectory;
    if (pItem->m_bIsFolder)
      URIUtils::AddSlashAtEnd(path);
    pItem->SetPath(path);

    items.Add(pItem);

    free(filename);
    free(fileinfo);
  }
  cc_xstream_client_close_all(conn);
  rv = true;

  if (conn != 0)
    cc_xstream_client_disconnect(conn);

  return true;
}

static void DiscoveryCallback(const char *addr, const char *port, const char *version,
                              const char *comment, void *context)
{
  struct DiscoveryCallbackContext *c = (struct DiscoveryCallbackContext *)context;

  //Construct name
  CStdString itemName = "Server: ";
  CStdString strPath = "xbms://";
  itemName += addr;

  if (strcmp(port, "1400") != 0)
  {
    itemName += " Port: ";
    itemName += port;
  }
  if (strlen(comment) > 0)
  {
    itemName += " (";
    itemName += comment;
    itemName += ")";
  }

  // Construct URL
  if (c->username != NULL)
  {
    strPath += c->username;
    if (c->password != NULL)
    {
      strPath += ":";
      strPath += c->password;
    }
    strPath += "@";
  }
  strPath += addr;
  strPath += ":";
  strPath += port;
  strPath += "/";

  // Add to items
  g_charsetConverter.unknownToUTF8(itemName);
  CFileItemPtr pItem(new CFileItem(itemName));
  CStdString path(strPath);
  g_charsetConverter.unknownToUTF8(path);
  pItem->SetPath(path);
  pItem->m_bIsFolder = true;
  pItem->m_bIsShareOrDrive = true;
  pItem->SetIconImage("DefaultNetwork.png");
  c->items->Add(pItem);
}
bool CXBMSDirectory::Exists(const char* strPath)
{
  CFileItemList items;
  CStdString strPath2(strPath);
  if (GetDirectory(strPath2,items))
    return true;
  
  return false;
}

