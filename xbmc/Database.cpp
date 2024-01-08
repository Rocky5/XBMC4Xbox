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
#include "AutoPtrHandle.h"
#include "Database.h"
#include "utils/URIUtils.h"
#include "settings/Settings.h"
#include "utils/Crc32.h"
#include "FileSystem/SpecialProtocol.h"
#include "AutoPtrHandle.h"

using namespace AUTOPTR;
using namespace dbiplus;

#define MAX_COMPRESS_COUNT 20

CDatabase::CDatabase(void)
{
  m_bOpen = false;
  m_iRefCount = 0;
  m_bMultiWrite = false;
}

CDatabase::~CDatabase(void)
{
  Close();
}

void CDatabase::Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
{
  strFileName = "";
  strPath = "";
  int i = strFileNameAndPath.size() - 1;
  while (i > 0)
  {
    char ch = strFileNameAndPath[i];
    if (ch == ':' || ch == '/' || ch == '\\') break;
    else i--;
  }
  strPath = strFileNameAndPath.Left(i);
  strFileName = strFileNameAndPath.Right(strFileNameAndPath.size() - i);
}

DWORD CDatabase::ComputeCRC(const CStdString &text)
{
  Crc32 crc;
  crc.ComputeFromLowerCase(text);
  return (DWORD)crc;
}

CStdString CDatabase::FormatSQL(CStdString strStmt, ...)
{
  //  %q is the sqlite format string for %s.
  //  Any bad character, like "'", will be replaced with a proper one
  strStmt.Replace("%s", "%q");
  //  the %I64 enhancement is not supported by sqlite3_vmprintf
  //  must be %ll instead
  strStmt.Replace("%I64", "%ll");

  va_list args;
  va_start(args, strStmt);
  auto_aptr<char> szStmt(sqlite3_vmprintf(strStmt.c_str(), args));
  va_end(args);

  return szStmt.get();
}

CStdString CDatabase::PrepareSQL(CStdString strStmt, ...) const
{
  CStdString strResult = "";

  if (NULL != m_pDB.get())
  {
    va_list args;
    va_start(args, strStmt);
    strResult = m_pDB->vprepare(strStmt.c_str(), args);
    va_end(args);
  }

  return strResult;
}

std::string CDatabase::GetSingleValue(const std::string &query, std::auto_ptr<Dataset> &ds)
{
  std::string ret;
  try
  {
    if (!m_pDB.get() || !ds.get())
      return ret;

    if (ds->query(query.c_str()) && ds->num_rows() > 0)
      ret = ds->fv(0).get_asString();

    ds->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - failed on query '%s'", __FUNCTION__, query.c_str());
  }
  return ret;
}

CStdString CDatabase::GetSingleValue(const CStdString &strTable, const CStdString &strColumn, const CStdString &strWhereClause /* = CStdString() */, const CStdString &strOrderBy /* = CStdString() */)
{
  CStdString query = PrepareSQL("SELECT %s FROM %s", strColumn.c_str(), strTable.c_str());
  if (!strWhereClause.empty())
    query += " WHERE " + strWhereClause;
  if (!strOrderBy.empty())
    query += " ORDER BY " + strOrderBy;
  query += " LIMIT 1";
  return GetSingleValue(query, m_pDS);
}

bool CDatabase::DeleteValues(const CStdString &strTable, const CStdString &strWhereClause /* = CStdString() */)
{
  bool bReturn = true;

  CStdString strQueryBase = "DELETE FROM %s";
  if (!strWhereClause.IsEmpty())
    strQueryBase.AppendFormat(" WHERE %s", strWhereClause.c_str());

  CStdString strQuery = PrepareSQL(strQueryBase, strTable.c_str());

  bReturn = ExecuteQuery(strQuery);

  return bReturn;
}

bool CDatabase::ExecuteQuery(const CStdString &strQuery)
{
  bool bReturn = false;

  try
  {
    if (NULL == m_pDB.get()) return bReturn;
    if (NULL == m_pDS.get()) return bReturn;
    m_pDS->exec(strQuery.c_str());
    bReturn = true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - failed to execute query '%s'",
        __FUNCTION__, strQuery.c_str());
  }

  return bReturn;
}

bool CDatabase::ResultQuery(const CStdString &strQuery)
{
  bool bReturn = false;

  try
  {
    if (NULL == m_pDB.get()) return bReturn;
    if (NULL == m_pDS.get()) return bReturn;

    CStdString strPreparedQuery = PrepareSQL(strQuery.c_str());

    bReturn = m_pDS->query(strPreparedQuery.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - failed to execute query '%s'",
        __FUNCTION__, strQuery.c_str());
  }

  return bReturn;
}

bool CDatabase::QueueInsertQuery(const CStdString &strQuery)
{
  if (strQuery.IsEmpty())
    return false;

  if (!m_bMultiWrite)
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    m_bMultiWrite = true;
    m_pDS2->insert();
  }

  m_pDS2->add_insert_sql(strQuery);

  return true;
}

bool CDatabase::CommitInsertQueries()
{
  bool bReturn = false;

  if (m_bMultiWrite)
  {
    try
    {
      m_bMultiWrite = false;
      m_pDS2->post();
      m_pDS2->clear_insert_sql();
      bReturn = true;
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "%s - failed to execute queries",
          __FUNCTION__);
    }
  }

  return bReturn;
}

bool CDatabase::Open()
{
  if (IsOpen())
  {
    m_iRefCount++;
    return true;
  }

  CStdString strDatabase;
  URIUtils::AddFileToFolder(g_settings.GetDatabaseFolder(), m_strDatabaseFile, strDatabase);

  m_pDB.reset(new SqliteDatabase() ) ;
  m_pDB->setDatabase(_P(strDatabase).c_str());

  m_pDS.reset(m_pDB->CreateDataset());
  m_pDS2.reset(m_pDB->CreateDataset());

  if (m_pDB->connect() != DB_CONNECTION_OK)
  {
    CLog::Log(LOGERROR, "Unable to open %s (old version?)", strDatabase.c_str());
    return false;
  }
  
  // test if db already exists, if not we need to create the tables
  if (!m_pDB->exists())
  {
    CreateTables();
  }

  // Mark our db as open here to make our destructor to properly close the file handle
  m_bOpen = true;

  // Database exists, check the version number
  int version = 0;
  m_pDS->query("SELECT idVersion FROM version\n");
  if (m_pDS->num_rows() > 0)
    version = m_pDS->fv("idVersion").get_asInt();

  if (version < GetMinVersion())
  {
    CLog::Log(LOGNOTICE, "Attempting to update the database %s from version %i to %i", m_strDatabaseFile.c_str(), version, GetMinVersion());
    if (UpdateOldVersion(version) && UpdateVersionNumber())
      CLog::Log(LOGINFO, "Update to version %i successfull", GetMinVersion());
    else
    {
      CLog::Log(LOGERROR, "Can't update the database %s from version %i to %i", m_strDatabaseFile.c_str(), version, GetMinVersion());
      Close();
      return false;
    }
  }
  else if (version > GetMinVersion())
  {
    CLog::Log(LOGERROR, "Can't open the database %s as it is a NEWER version than what we were expecting!", m_strDatabaseFile.c_str());
    Close();
    return false;
  }

  m_pDS->exec("PRAGMA cache_size=16384\n");
  m_pDS->exec("PRAGMA synchronous='NORMAL'\n");
  m_pDS->exec("PRAGMA journal_mode='TRUNCATE'\n");
//  m_pDS->exec("PRAGMA journal_mode='OFF'\n");
  m_pDS->exec("PRAGMA count_changes='OFF'\n");

  m_iRefCount++;
  return true;
}

bool CDatabase::IsOpen()
{
  return m_bOpen;
}

void CDatabase::Close()
{
  if (!m_bOpen)
    return ;

  if (m_iRefCount > 1)
  {
    m_iRefCount--;
    return ;
  }

  m_iRefCount--;
  m_bOpen = false;

  if (NULL == m_pDB.get() ) return ;
  if (NULL != m_pDS.get()) m_pDS->close();
  m_pDB->disconnect();
  m_pDB.reset();
  m_pDS.reset();
  m_pDS2.reset();
}

bool CDatabase::Compress(bool bForce /* =true */)
{
  // compress database
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    if (!bForce)
    {
      m_pDS->query("select iCompressCount from version");
      if (!m_pDS->eof())
      {
        int iCount = m_pDS->fv(0).get_asInt();
        if (iCount > MAX_COMPRESS_COUNT)
          iCount = -1;
        m_pDS->close();
        CStdString strSQL=PrepareSQL("update version set iCompressCount=%i\n",++iCount);
        m_pDS->exec(strSQL.c_str());
        if (iCount != 0)
          return true;
      }
    }

    if (!m_pDS->exec("vacuum\n"))
      return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Compressing the database %s failed", m_strDatabaseFile.c_str());
    return false;
  }
  return true;
}

void CDatabase::Interupt()
{
  m_pDS->interrupt();
}

void CDatabase::BeginTransaction()
{
  try
  {
    if (NULL != m_pDB.get())
      m_pDB->start_transaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "database:begintransaction failed");
  }
}

bool CDatabase::CommitTransaction()
{
  try
  {
    if (NULL != m_pDB.get())
      m_pDB->commit_transaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "database:committransaction failed");
    return false;
  }
  return true;
}

void CDatabase::RollbackTransaction()
{
  try
  {
    if (NULL != m_pDB.get())
      m_pDB->rollback_transaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "database:rollbacktransaction failed");
  }
}

bool CDatabase::InTransaction()
{
  if (NULL != m_pDB.get()) return false;
  return m_pDB->in_transaction();
}

bool CDatabase::CreateTables()
{
    //  all fatx formatted partitions, except the utility drive,
    //  have a cluster size of 16k. To gain better performance
    //  when performing write operations to the database, set
    //  the page size of the database file to 16k.
    //  This needs to be done before any table is created.
    CLog::Log(LOGINFO, "Set page size");
    m_pDS->exec("PRAGMA page_size=16384\n");
    //  Also set the memory cache size to 16k
    CLog::Log(LOGINFO, "Set default cache size");
    m_pDS->exec("PRAGMA default_cache_size=16384\n");

    CLog::Log(LOGINFO, "creating version table");
    m_pDS->exec("CREATE TABLE version (idVersion integer, iCompressCount integer)\n");
    CStdString strSQL=PrepareSQL("INSERT INTO version (idVersion,iCompressCount) values(%i,0)\n", GetMinVersion());
    m_pDS->exec(strSQL.c_str());

    return true;
}

bool CDatabase::UpdateVersionNumber()
{
  try
  {
    CStdString strSQL=PrepareSQL("UPDATE version SET idVersion=%i\n", GetMinVersion());
    m_pDS->exec(strSQL.c_str());
  }
  catch(...)
  {
    return false;
  }

  return true;
}

