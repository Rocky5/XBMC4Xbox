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

#include "DDSImage.h"
#include "XBTF.h"
#include "Util.h"
#include <string.h>
#include "FileSystem/File.h"
#include "utils/log.h"

using namespace XFILE;
using namespace std;

CDDSImage::CDDSImage()
{
  m_data = NULL;
  memset(&m_desc, 0, sizeof(m_desc));
}

CDDSImage::CDDSImage(unsigned int width, unsigned int height, unsigned int format)
{
  m_data = NULL;
  Allocate(width, height, format);
}

CDDSImage::~CDDSImage()
{
  delete[] m_data;
}

/*
  If dds file is marked with our magic mark, read orgWidth and orgHeight, otherwise we read
  normal width and height (file is a standard dds file and does not contain our modification.
  See DDSImage.h for details.
*/

unsigned int CDDSImage::GetOrgWidth() const
{
  return (m_desc.xbmcMagic == DD_XBMC_MAGIC) ? m_desc.orgWidth : m_desc.width;
}

unsigned int CDDSImage::GetOrgHeight() const
{
  return (m_desc.xbmcMagic == DD_XBMC_MAGIC) ? m_desc.orgHeight : m_desc.height;
}

unsigned int CDDSImage::GetWidth() const
{
  return m_desc.width;
}

unsigned int CDDSImage::GetHeight() const
{
  return m_desc.height;
}

unsigned int CDDSImage::GetFormat() const
{
  if (m_desc.pixelFormat.flags & DDPF_RGB)
    return 0; // Not supported
  if (m_desc.pixelFormat.flags & DDPF_FOURCC)
  {
    if (strncmp((const char *)&m_desc.pixelFormat.fourcc, "DXT1", 4) == 0)
      return XB_FMT_DXT1;
    /*
    //We are only supporting DXT1 at this time.

    if (strncmp((const char *)&m_desc.pixelFormat.fourcc, "DXT3", 4) == 0)
      return XB_FMT_DXT3;
    if (strncmp((const char *)&m_desc.pixelFormat.fourcc, "DXT5", 4) == 0)
      return XB_FMT_DXT5;
    if (strncmp((const char *)&m_desc.pixelFormat.fourcc, "ARGB", 4) == 0)
      return XB_FMT_A8R8G8B8;
    
    */
  }
  return 0;
}

unsigned int CDDSImage::GetSize() const
{
  return m_desc.linearSize;
}

unsigned char *CDDSImage::GetData() const
{
  return m_data;
}

bool CDDSImage::ReadFile(const std::string &inputFile)
{
  // open the file
  CFile file;
  if (!file.Open(inputFile))
  {
	  CLog::Log(LOGERROR, "%s - CFile.Open failed %s", __FUNCTION__, inputFile.c_str());   
	  return false;
  }

  // read the header
  uint32_t magic;
  if (file.Read(&magic, 4) != 4)
  {
    CLog::Log(LOGERROR, "%s - Magic Header not found %s", __FUNCTION__, inputFile.c_str());   
    return false;
  }
  if (file.Read(&m_desc, sizeof(m_desc)) != sizeof(m_desc))
  {
    CLog::Log(LOGERROR, "%s - Description Invalid %s", __FUNCTION__, inputFile.c_str());   
    return false;
  }
  if (!GetFormat())
  {
    CLog::Log(LOGERROR, "%s - GetFormat returned false %s", __FUNCTION__, inputFile.c_str());   
    return false;  // not supported
  }

  //This is temporary to make sure a "generic" .dds file is not read in at this point
  //This can get removed once a proper routine is written to dynamically pad to POT (if deemed
  //that such support is needed)
  if (m_desc.xbmcMagic != DD_XBMC_MAGIC)
  {
    CLog::Log(LOGERROR, "%s - DDS file was not marked for xbmc use %s", __FUNCTION__, inputFile.c_str());   
    return false;  // not supported
  }

  // allocate our data
  m_data = new unsigned char[m_desc.linearSize];
  if (!m_data)
  {
    CLog::Log(LOGERROR, "%s - No Data %s", __FUNCTION__, inputFile.c_str());   
    return false;
  }

  // and read it in
  if (file.Read(m_data, m_desc.linearSize) != m_desc.linearSize)
  {
    CLog::Log(LOGERROR, "%s - Data doesn't match header size %s", __FUNCTION__, inputFile.c_str());   
    return false;
  }

  file.Close();
  return true;
}

bool CDDSImage::WriteFile(const std::string &outputFile) const
{
  // open the file
  CFile file;
  if (!file.OpenForWrite(outputFile, true))
    return false;

  // write the header
  file.Write("DDS ", 4);
  file.Write(&m_desc, sizeof(m_desc));
  // now the data
  file.Write(m_data, m_desc.linearSize);
  file.Close();
  return true;
}

unsigned int CDDSImage::GetStorageRequirements(unsigned int width, unsigned int height, unsigned int format) const
{
  switch (format)
  {
  case XB_FMT_DXT1:
    return ((width + 3) / 4) * ((height + 3) / 4) * 8;
  case XB_FMT_DXT3:
  case XB_FMT_DXT5:
    return ((width + 3) / 4) * ((height + 3) / 4) * 16;
  case XB_FMT_A8R8G8B8:
  default:
    return width * height * 4;
  }
}

void CDDSImage::Allocate(unsigned int width, unsigned int height, unsigned int format)
{
  memset(&m_desc, 0, sizeof(m_desc));
  m_desc.size = sizeof(m_desc);
  m_desc.flags = ddsd_caps | ddsd_pixelformat | ddsd_width | ddsd_height | ddsd_linearsize;
  m_desc.height = height;
  m_desc.width = width;
  m_desc.linearSize = GetStorageRequirements(width, height, format);
  m_desc.pixelFormat.size = sizeof(m_desc.pixelFormat);
  m_desc.pixelFormat.flags = ddpf_fourcc;
  memcpy(&m_desc.pixelFormat.fourcc, GetFourCC(format), 4);
  m_desc.caps.flags1 = ddscaps_texture;
  delete[] m_data;
  m_data = new unsigned char[m_desc.linearSize];
}

const char *CDDSImage::GetFourCC(unsigned int format) const
{
  switch (format)
  {
  case XB_FMT_DXT1:
    return "DXT1";
  case XB_FMT_DXT3:
    return "DXT3";
  case XB_FMT_DXT5:
    return "DXT5";
  case XB_FMT_A8R8G8B8:
  default:
    return "ARGB";
  }
}
