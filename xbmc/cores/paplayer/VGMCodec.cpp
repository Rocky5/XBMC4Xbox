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
#include "VGMCodec.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "FileSystem/File.h"


VGMCodec::VGMCodec()
{
  m_CodecName = "VGM";
  m_vgm = 0;
  m_iDataPos = -1; 
}

VGMCodec::~VGMCodec()
{
  DeInit();
}

bool VGMCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_dll.Load())
    return false; // error logged previously
  
  m_dll.Init();

  //CStdString strFileToLoad = "filereader://"+strFile;
  XFILE::CFile::Cache(strFile,"special://temp/"+URIUtils::GetFileName(strFile));

  //m_vgm = m_dll.LoadVGM(strFileToLoad.c_str(),&m_SampleRate,&m_BitsPerSample,&m_Channels);
  m_vgm = m_dll.LoadVGM("special://temp/"+URIUtils::GetFileName(strFile),&m_SampleRate,&m_BitsPerSample,&m_Channels);
  if (!m_vgm)
  {
    CLog::Log(LOGERROR,"%s: error opening file %s!",__FUNCTION__,strFile.c_str());
    return false;
  }
  
  m_TotalTime = (__int64)m_dll.GetLength(m_vgm);

  return true;
}

void VGMCodec::DeInit()
{
  if (m_vgm)
    m_dll.FreeVGM(m_vgm);
  m_vgm = 0;
}

__int64 VGMCodec::Seek(__int64 iSeekTime)
{
  __int64 result = (__int64)m_dll.Seek(m_vgm,(unsigned long)iSeekTime);
  m_iDataPos = result/1000*m_SampleRate*m_BitsPerSample*m_Channels/8;
  
  return result;
}

int VGMCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_iDataPos == -1)
  {
    m_iDataPos = 0;
  }

  if (m_iDataPos >= m_TotalTime/1000*m_SampleRate*m_BitsPerSample*m_Channels/8)
  {
    return READ_EOF;
  }
  
  if ((*actualsize=m_dll.FillBuffer(m_vgm,(char*)pBuffer,size))> 0)
  {
    m_iDataPos += *actualsize;
    return READ_SUCCESS;
  }

  return READ_ERROR;
}

bool VGMCodec::CanInit()
{
  return m_dll.CanLoad();
}

bool VGMCodec::IsSupportedFormat(const CStdString& strExt)
{
  if (strExt == "acm" ||
      strExt == "adp" ||
      strExt == "ads" ||
      strExt == "adx" ||
      strExt == "afc" ||
      strExt == "agsc" ||
      strExt == "ahx" ||
      strExt == "aifc" ||
      strExt == "aix" ||
      strExt == "amts" ||
      strExt == "as4" ||
      strExt == "asf" ||
      strExt == "ast" ||
      strExt == "aud" ||
      strExt == "aus" ||
      strExt == "bg00" ||
      strExt == "biodsp" ||
      strExt == "bmdx" ||
      strExt == "brstm" ||
      strExt == "cfn" ||
      strExt == "cnk" ||
      strExt == "dsp" ||
      strExt == "dvi" ||
      strExt == "dxh" ||
      strExt == "eam" ||
      strExt == "enth" ||
      strExt == "filp" ||
      strExt == "fsb" ||
      strExt == "gcm" ||
      strExt == "gcw" ||
      strExt == "genh" ||
      strExt == "gms" ||
      strExt == "hgc1" ||
      strExt == "hps" ||
      strExt == "ikm" ||
      strExt == "ild" ||
      strExt == "int" ||
      strExt == "ivb" ||
      strExt == "kces" ||
      strExt == "kcey" ||
      strExt == "leg" ||
      strExt == "logg" ||
      strExt == "lwav" ||
	  strExt == "matx" ||
      strExt == "mi4" ||
      strExt == "mib" ||
      strExt == "mic" ||
      strExt == "mpdsp" ||
      strExt == "mss" ||
      strExt == "mus" ||
      strExt == "musc" ||
      strExt == "musx" ||
      strExt == "npsf" ||
      strExt == "nwa" ||
      strExt == "pcm" ||
      strExt == "pnb" ||
      strExt == "pos" ||
      strExt == "psh" ||
      strExt == "psw" ||
      strExt == "raw" ||
      strExt == "rkv" ||
      strExt == "rsd" ||
      strExt == "rsf" ||
      strExt == "rstm" ||
      strExt == "rws" ||
      strExt == "rwsd" ||
      strExt == "rwx" ||
      strExt == "rxw" ||
      strExt == "sad" ||
      strExt == "sdt" ||
      strExt == "sfl" ||
      strExt == "sfs" ||
      strExt == "sl3" ||
      strExt == "sli" ||
      strExt == "sng" ||
      strExt == "ss2" ||
	  strExt == "stma" ||
      strExt == "str" ||
      strExt == "strm" ||
      strExt == "sts" ||
      strExt == "svag" ||
      strExt == "svs" ||
      strExt == "swd" ||
      strExt == "tec" ||
      strExt == "tydsp" ||
      strExt == "um3" ||
      strExt == "vag" ||
      strExt == "vas" ||
      strExt == "vig" ||
      strExt == "vjdsp" ||
      strExt == "vpk" ||
      strExt == "wavm" ||
      strExt == "wp2" ||
      strExt == "wsi" ||
      strExt == "wvs" ||
      strExt == "xa" ||
      strExt == "xa30" ||
      strExt == "xss" ||
      strExt == "xwav" ||
      strExt == "xwb")
    return true;
  
  return false;
}
