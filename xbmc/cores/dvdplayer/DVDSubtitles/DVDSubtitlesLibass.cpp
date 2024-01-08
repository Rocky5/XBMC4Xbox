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
#include "utils/log.h"
#include "DVDSubtitlesLibass.h"
#include "DVDClock.h"
#include "FileSystem/SpecialProtocol.h"

using namespace std;

CDVDSubtitlesLibass::CDVDSubtitlesLibass()
{

  m_track = NULL;
  m_library = NULL;
  m_renderer = NULL;
  m_references = 1;

  if(!m_dll.Load())
  {
    CLog::Log(LOGERROR, "CDVDSubtitlesLibass: Failed to load libass library");
    return;
  }

#ifdef _XBOX
  CStdString strPath = "special://xbmc/media/Fonts/";
#else
  //Setting the font directory to the temp dir(where mkv fonts are extracted to)
  CStdString strPath = "special://temp/fonts/";
#endif
  CLog::Log(LOGINFO, "CDVDSubtitlesLibass: Creating ASS library structure");
  m_library  = m_dll.ass_library_init();
  if(!m_library)
    return;

  CLog::Log(LOGINFO, "CDVDSubtitlesLibass: Initializing ASS library font settings");
  // libass uses fontconfig (system lib) which is not wrapped 
  //  so translate the path before calling into libass
  m_dll.ass_set_fonts_dir(m_library,  _P(strPath).c_str());
  m_dll.ass_set_extract_fonts(m_library, 0);
  m_dll.ass_set_style_overrides(m_library, NULL);

  CLog::Log(LOGINFO, "CDVDSubtitlesLibass: Initializing ASS Renderer");

  m_renderer = m_dll.ass_renderer_init(m_library);

  if(!m_renderer)
    return;

  //Setting default font to the Arial in \media\fonts (used if FontConfig fails)
  strPath = "special://xbmc/media/Fonts/arial.ttf";

  m_dll.ass_set_margins(m_renderer, 0, 0, 0, 0);
  m_dll.ass_set_use_margins(m_renderer, 0);
  m_dll.ass_set_font_scale(m_renderer, 1);
  // libass uses fontconfig (system lib) which is not wrapped 
  //  so translate the path before calling into libass
  m_dll.ass_set_fonts(m_renderer, _P(strPath).c_str(), "");
}


CDVDSubtitlesLibass::~CDVDSubtitlesLibass()
{
  if(m_dll.IsLoaded())
  {
    m_dll.ass_renderer_done(m_renderer);
    m_dll.ass_library_done(m_library);
    m_dll.Unload();
  }
}

/*Decode Header of SSA, needed to properly decode demux packets*/
bool CDVDSubtitlesLibass::DecodeHeader(char* data, int size)
{

  if(!m_library || !data)
    return false;

  if(!m_track)
  {
    CLog::Log(LOGINFO, "CDVDSubtitlesLibass: Creating new ASS track");
    m_track = m_dll.ass_new_track(m_library) ;
  }

  m_dll.ass_process_codec_private(m_track, data, size);
  return true;
}

bool CDVDSubtitlesLibass::DecodeDemuxPkt(char* data, int size, double start, double duration)
{
  if(!m_track)
  {
    CLog::Log(LOGERROR, "CDVDSubtitlesLibass: No SSA header found.");
    return false;
  }

  m_dll.ass_process_chunk(m_track, data, size, DVD_TIME_TO_MSEC(start), DVD_TIME_TO_MSEC(duration));
  return true;
}

bool CDVDSubtitlesLibass::ReadFile(const string& strFile)
{
  if(!m_library)
  {
    CLog::Log(LOGERROR, "CDVDSubtitlesLibass: %s - No ASS library struct", __FUNCTION__);
    return false;
  }

  string fileName =  strFile;
#ifdef _LINUX
  fileName = PTH_IC(fileName);
#endif

  CLog::Log(LOGINFO, "SSA Parser: Creating m_track from SSA file:  %s", fileName.c_str());

  m_track = m_dll.ass_read_file(m_library, (char* )fileName.c_str(), 0);
  if(m_track == NULL)
    return false;

  return true;
}


long CDVDSubtitlesLibass::Acquire()
{
  long count = InterlockedIncrement(&m_references);
  return count;
}

long CDVDSubtitlesLibass::Release()
{
  long count = InterlockedDecrement(&m_references);
  if (count == 0)
    delete this;

  return count;
}

long CDVDSubtitlesLibass::GetNrOfReferences()
{
  return m_references;
}

ass_image_t* CDVDSubtitlesLibass::RenderImage(int imageWidth, int imageHeight, double pts)
{
  if(!m_renderer || !m_track)
  {
    CLog::Log(LOGERROR, "CDVDSubtitlesLibass: %s - Missing ASS structs(m_track or m_renderer)", __FUNCTION__);
    return NULL;
  }

  m_dll.ass_set_frame_size(m_renderer, imageWidth, imageHeight);
  return m_dll.ass_render_frame(m_renderer, m_track, DVD_TIME_TO_MSEC(pts), NULL);
}

ass_event_t* CDVDSubtitlesLibass::GetEvents()
{
  if(!m_track)
  {
    CLog::Log(LOGERROR, "CDVDSubtitlesLibass: %s -  Missing ASS structs(m_track)", __FUNCTION__);
    return NULL;
  }
  return m_track->events;
}

int CDVDSubtitlesLibass::GetNrOfEvents()
{
  if(!m_track)
    return 0;
  return m_track->n_events;
}

