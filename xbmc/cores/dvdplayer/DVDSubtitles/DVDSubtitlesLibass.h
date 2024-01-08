#pragma once

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

#include "DllLibass.h"

extern "C"{
  #include "../../../lib/libass/ass.h"
}

/** Wrapper for Libass **/

class CDVDSubtitlesLibass
{
public:
  CDVDSubtitlesLibass();
  ~CDVDSubtitlesLibass();

  ass_image_t* RenderImage(int imageWidth, int imageHeight, double pts);
  ass_event_t* GetEvents();

  int GetNrOfEvents();

  bool DecodeHeader(char* data, int size);
  bool DecodeDemuxPkt(char* data, int size, double start, double duration);
  bool ReadFile(const std::string& strFile);

  long GetNrOfReferences();
  long Acquire();
  long Release();

private:
  DllLibass m_dll;
  long m_references;
  ass_library_t* m_library;
  ass_track_t* m_track;
  ass_renderer_t* m_renderer;
};

