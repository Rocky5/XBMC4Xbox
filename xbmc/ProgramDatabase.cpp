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

#include "ProgramDatabase.h"
#include "Util.h"
#include "xbox/xbeheader.h"
#include "windows/GUIWindowFileManager.h"
#include "FileItem.h"
#include "utils/Crc32.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace XFILE;

//********************************************************************************************************************************
CProgramDatabase::CProgramDatabase(void)
{
  m_strDatabaseFile=PROGRAM_DATABASE_NAME;
}

//********************************************************************************************************************************
CProgramDatabase::~CProgramDatabase(void)
{

}

//********************************************************************************************************************************
bool CProgramDatabase::CreateTables()
{

  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "create files table");
    m_pDS->exec("CREATE TABLE files ( idFile integer primary key, strFilename text, titleId integer, xbedescription text, iTimesPlayed integer, lastAccessed integer, iRegion integer, iSize integer)\n");
    CLog::Log(LOGINFO, "create trainers table");
    m_pDS->exec("CREATE TABLE trainers (idKey integer auto_increment primary key, idCRC integer, idTitle integer, strTrainerPath text, strSettings text, Active integer)\n");
    CLog::Log(LOGINFO, "create files index");
    m_pDS->exec("CREATE INDEX idxFiles ON files(strFilename)");
    CLog::Log(LOGINFO, "create files - titleid index");
    m_pDS->exec("CREATE INDEX idxTitleIdFiles ON files(titleId)");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "programdatabase::unable to create tables:%lu", GetLastError());
    return false;
  }

  return true;
}

bool CProgramDatabase::UpdateOldVersion(int version)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  if (NULL == m_pDS2.get()) return false;

  try
  {
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Error attempting to update the database version!");
    return false;
  }
  return true;
}

int CProgramDatabase::GetRegion(const CStdString& strFilenameAndPath)
{
  if (NULL == m_pDB.get()) return 0;
  if (NULL == m_pDS.get()) return 0;

  try
  {
    CStdString strSQL = PrepareSQL("select * from files where files.strFileName like '%s'", strFilenameAndPath.c_str());
    if (!m_pDS->query(strSQL.c_str()))
      return 0;

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return 0;
    }
    int iRegion = m_pDS->fv("files.iRegion").get_asInt();
    m_pDS->close();

    return iRegion;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:GetRegion(%s) failed", strFilenameAndPath.c_str());
  }
  return 0;
}

int CProgramDatabase::GetTitleId(const CStdString& strFilenameAndPath)
{
  if (NULL == m_pDB.get()) return 0;
  if (NULL == m_pDS.get()) return 0;

  try
  {
    CStdString strSQL = PrepareSQL("select * from files where files.strFileName like '%s'", strFilenameAndPath.c_str());
    if (!m_pDS->query(strSQL.c_str()))
      return 0;

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return 0;
    }
    int idTitle = m_pDS->fv("files.TitleId").get_asInt();
    m_pDS->close();
    return idTitle;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:GetTitleId(%s) failed", strFilenameAndPath.c_str());
  }
  return 0;
}

bool CProgramDatabase::SetRegion(const CStdString& strFileName, int iRegion)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = PrepareSQL("select * from files where files.strFileName like '%s'", strFileName.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    int idFile = m_pDS->fv("files.idFile").get_asInt();
    m_pDS->close();

    CLog::Log(LOGDEBUG, "CProgramDatabase::SetRegion(%s), idFile=%i, region=%i",
              strFileName.c_str(), idFile,iRegion);

    strSQL=PrepareSQL("update files set iRegion=%i where idFile=%i",
                  iRegion, idFile);
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:SetDescription(%s) failed", strFileName.c_str());
  }

  return false;
}

bool CProgramDatabase::SetTitleId(const CStdString& strFileName, int idTitle)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = PrepareSQL("select * from files where files.strFileName like '%s'", strFileName.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    int idFile = m_pDS->fv("files.idFile").get_asInt();
    m_pDS->close();

    CLog::Log(LOGDEBUG, "CProgramDatabase::SetTitle(%s), idFile=%i, region=%i",
              strFileName.c_str(), idFile, idTitle);

    strSQL=PrepareSQL("update files set titleId=%i where idFile=%i",
                  idTitle, idFile);
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:SetDescription(%s) failed", strFileName.c_str());
  }

  return false;
}

bool CProgramDatabase::GetXBEPathByTitleId(const int idTitle, CStdString& strPathAndFilename)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=PrepareSQL("select files.strFilename from files where files.titleId=%i", idTitle);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      strPathAndFilename = m_pDS->fv("files.strFilename").get_asString();
      strPathAndFilename.Replace('/', '\\');
      m_pDS->close();
      return true;
    }
    m_pDS->close();
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetXBEPathByTitleId(%i) failed", idTitle);
  }
  return false;
}

bool CProgramDatabase::ItemHasTrainer(unsigned int iTitleId)
{
  CStdString strSQL;
  try
  {
    strSQL = PrepareSQL("select * from trainers where idTitle=%u", iTitleId);
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    if (m_pDS->num_rows())
      return true;

    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"error checking for title's trainers (%s)",strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::HasTrainer(const CStdString& strTrainerPath)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    strSQL = PrepareSQL("select * from trainers where idCRC=%u", (unsigned __int32) crc);
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    if (m_pDS->num_rows())
      return true;

    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"error checking for trainer existance (%s)",strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::AddTrainer(int iTitleId, const CStdString& strTrainerPath)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    char temp[101];
    for( int i=0;i<100;++i)
      temp[i] = '0';
    temp[100] = '\0';
    strSQL=PrepareSQL("insert into trainers (idKey,idCRC,idTitle,strTrainerPath,strSettings,Active) values(NULL,%u,%u,'%s','%s',%i)",(unsigned __int32)crc,iTitleId,strTrainerPath.c_str(),temp,0);
    if (!m_pDS->exec(strSQL.c_str()))
      return false;

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"programdatabase: unable to add trainer (%s)",strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::RemoveTrainer(const CStdString& strTrainerPath)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    strSQL=PrepareSQL("delete from trainers where idCRC=%u", (unsigned __int32)crc);
    if (!m_pDS->exec(strSQL.c_str()))
      return false;

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"programdatabase: unable to remove trainer (%s)",strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::GetTrainers(unsigned int iTitleId, std::vector<CStdString>& vecTrainers)
{
  vecTrainers.clear();
  CStdString strSQL;
  try
  {
    strSQL = PrepareSQL("select * from trainers where idTitle=%u", iTitleId);
    if (!m_pDS->query(strSQL.c_str()))
      return false;

    while (!m_pDS->eof())
    {
      vecTrainers.push_back(m_pDS->fv("strTrainerPath").get_asString());
      m_pDS->next();
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"programdatabase: error reading trainers for %i (%s)",iTitleId,strSQL.c_str());
  }
  return false;

}

bool CProgramDatabase::GetAllTrainers(std::vector<CStdString>& vecTrainers)
{
  vecTrainers.clear();
  CStdString strSQL;
  try
  {
    strSQL = PrepareSQL("select distinct strTrainerPath from trainers");//PrepareSQL("select * from trainers");
    if (!m_pDS->query(strSQL.c_str()))
      return false;

    while (!m_pDS->eof())
    {
      vecTrainers.push_back(m_pDS->fv("strTrainerPath").get_asString());
      m_pDS->next();
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"programdatabase: error reading trainers (%s)",strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::SetTrainerOptions(const CStdString& strTrainerPath, unsigned int iTitleId, unsigned char* data, int numOptions)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    char temp[101];
    int i;
    for (i=0;i<numOptions && i<100;++i)
    {
      if (data[i] == 1)
        temp[i] = '1';
      else
        temp[i] = '0';
    }
    temp[i] = '\0';

    strSQL = PrepareSQL("update trainers set strSettings='%s' where idCRC=%u and idTitle=%u", temp, (unsigned __int32)crc,iTitleId);
    if (m_pDS->exec(strSQL.c_str()))
      return true;

    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"CProgramDatabase::SetTrainerOptions failed (%s)",strSQL.c_str());
  }

  return false;
}

void CProgramDatabase::SetTrainerActive(const CStdString& strTrainerPath, unsigned int iTitleId, bool bActive)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    strSQL = PrepareSQL("update trainers set Active=%u where idCRC=%u and idTitle=%u", bActive?1:0, (unsigned __int32)crc, iTitleId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"CProgramDatabase::SetTrainerOptions failed (%s)",strSQL.c_str());
  }
}

CStdString CProgramDatabase::GetActiveTrainer(unsigned int iTitleId)
{
  CStdString strSQL;
  try
  {
    strSQL = PrepareSQL("select * from trainers where idTitle=%u and Active=1", iTitleId);
    if (!m_pDS->query(strSQL.c_str()))
      return "";

    if (!m_pDS->eof())
      return m_pDS->fv("strTrainerPath").get_asString();
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"programdatabase: error finding active trainer for %i (%s)",iTitleId,strSQL.c_str());
  }

  return "";
}

bool CProgramDatabase::GetTrainerOptions(const CStdString& strTrainerPath, unsigned int iTitleId, unsigned char* data, int numOptions)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    strSQL = PrepareSQL("select * from trainers where idCRC=%u and idTitle=%u", (unsigned __int32)crc, iTitleId);
    if (m_pDS->query(strSQL.c_str()))
    {
      CStdString strSettings = m_pDS->fv("strSettings").get_asString();
      for (int i=0;i<numOptions && i < 100;++i)
        data[i] = strSettings[i]=='1'?1:0;

      return true;
    }

    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"CProgramDatabase::GetTrainerOptions failed (%s)",strSQL.c_str());
  }

  return false;
}

int CProgramDatabase::GetProgramInfo(CFileItem *item)
{
  int idTitle = 0;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = PrepareSQL("select xbedescription,iTimesPlayed,lastAccessed,titleId,iSize from files where strFileName like '%s'", item->GetPath().c_str());
    m_pDS->query(strSQL.c_str());
    if (!m_pDS->eof())
    { // get info - only set the label if not preformatted
      if (!item->IsLabelPreformated())
        item->SetLabel(m_pDS->fv("xbedescription").get_asString());
      item->m_iprogramCount = m_pDS->fv("iTimesPlayed").get_asInt();
      item->m_strTitle = item->GetLabel();  // is this needed?
      item->m_dateTime = TimeStampToLocalTime(_atoi64(m_pDS->fv("lastAccessed").get_asString().c_str()));
      item->m_dwSize = _atoi64(m_pDS->fv("iSize").get_asString().c_str());
      idTitle = m_pDS->fv("titleId").get_asInt();
      if (item->m_dwSize == -1)
      {
        CStdString strPath;
        URIUtils::GetDirectory(item->GetPath(),strPath);
        __int64 iSize = CGUIWindowFileManager::CalculateFolderSize(strPath);
        CStdString strSQL=PrepareSQL("update files set iSize=%I64u where strFileName like '%s'",iSize,item->GetPath().c_str());
        m_pDS->exec(strSQL.c_str());
      }
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetProgramInfo(%s) failed", item->GetPath().c_str());
  }
  return idTitle;
}

bool CProgramDatabase::AddProgramInfo(CFileItem *item, unsigned int titleID)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int iRegion = -1;
    if (g_guiSettings.GetBool("myprograms.gameautoregion"))
    {
      CXBE xbe;
      iRegion = xbe.ExtractGameRegion(item->GetPath());
      if (iRegion < 1 || iRegion > 7)
        iRegion = 0;
    }
    FILETIME time;
    item->m_dateTime=CDateTime::GetCurrentDateTime();
    item->m_dateTime.GetAsTimeStamp(time);

    ULARGE_INTEGER lastAccessed;
    lastAccessed.u.LowPart = time.dwLowDateTime; 
    lastAccessed.u.HighPart = time.dwHighDateTime;

    CStdString strPath, strParent;
    URIUtils::GetDirectory(item->GetPath(),strPath);
    // special case - programs in root of sources
    bool bIsShare=false;
    CUtil::GetMatchingSource(strPath,g_settings.m_programSources,bIsShare);
    __int64 iSize=0;
    if (bIsShare || !item->IsDefaultXBE())
    {
      __stat64 stat;
      if (CFile::Stat(item->GetPath(),&stat) == 0)
        iSize = stat.st_size;
    }
    else
      iSize = CGUIWindowFileManager::CalculateFolderSize(strPath);
    if (titleID == 0)
      titleID = (unsigned int) -1;
    CStdString strSQL=PrepareSQL("insert into files (idFile, strFileName, titleId, xbedescription, iTimesPlayed, lastAccessed, iRegion, iSize) values(NULL, '%s', %u, '%s', %i, %I64u, %i, %I64u)", item->GetPath().c_str(), titleID, item->GetLabel().c_str(), 0, lastAccessed.QuadPart, iRegion, iSize);
    m_pDS->exec(strSQL.c_str());
    item->m_dwSize = iSize;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::AddProgramInfo(%s) failed", item->GetPath().c_str());
  }
  return true;
}

FILETIME CProgramDatabase::TimeStampToLocalTime( unsigned __int64 timeStamp )
{
  FILETIME fileTime;
  ::FileTimeToLocalFileTime( (const FILETIME *)&timeStamp, &fileTime);
  return fileTime;
}

bool CProgramDatabase::IncTimesPlayed(const CStdString& strFileName)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = PrepareSQL("select * from files where files.strFileName like '%s'", strFileName.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    int idFile = m_pDS->fv("files.idFile").get_asInt();
    int iTimesPlayed = m_pDS->fv("files.iTimesPlayed").get_asInt();
    m_pDS->close();

    CLog::Log(LOGDEBUG, "CProgramDatabase::IncTimesPlayed(%s), idFile=%i, iTimesPlayed=%i",
              strFileName.c_str(), idFile, iTimesPlayed);

    strSQL=PrepareSQL("update files set iTimesPlayed=%i where idFile=%i",
                  ++iTimesPlayed, idFile);
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:IncTimesPlayed(%s) failed", strFileName.c_str());
  }

  return false;
}

bool CProgramDatabase::SetDescription(const CStdString& strFileName, const CStdString& strDescription)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = PrepareSQL("select * from files where files.strFileName like '%s'", strFileName.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    int idFile = m_pDS->fv("files.idFile").get_asInt();
    m_pDS->close();

    CLog::Log(LOGDEBUG, "CProgramDatabase::SetDescription(%s), idFile=%i, description=%s",
              strFileName.c_str(), idFile,strDescription.c_str());

    strSQL=PrepareSQL("update files set xbedescription='%s' where idFile=%i",
                  strDescription.c_str(), idFile);
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:SetDescription(%s) failed", strFileName.c_str());
  }

  return false;
}

bool CProgramDatabase::GetArbitraryQuery(const CStdString& strQuery,      const CStdString& strOpenRecordSet, const CStdString& strCloseRecordSet,
                                         const CStdString& strOpenRecord, const CStdString& strCloseRecord,   const CStdString& strOpenField, 
										 const CStdString& strCloseField,       CStdString& strResult)
{
  try
  {
    strResult = "";
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    CStdString strSQL=strQuery;
    if (!m_pDS->query(strSQL.c_str()))
    {
      strResult = m_pDB->getErrorMsg();
      return false;
    }
    strResult=strOpenRecordSet;
    while (!m_pDS->eof())
    {
      strResult += strOpenRecord;
      for (int i=0; i<m_pDS->fieldCount(); i++)
      {
        strResult += strOpenField + CStdString(m_pDS->fv(i).get_asString()) + strCloseField;
      }
      strResult += strCloseRecord;
      m_pDS->next();
    }
    strResult += strCloseRecordSet;
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  try
  {
    if (NULL == m_pDB.get()) return false;
    strResult = m_pDB->getErrorMsg();
  }
  catch (...)
  {

  }

  return false;
}
