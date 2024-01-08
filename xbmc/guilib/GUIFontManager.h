/*!
\file GUIFontManager.h
\brief
*/

#ifndef GUILIB_FONTMANAGER_H
#define GUILIB_FONTMANAGER_H

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

#include "GraphicContext.h"

// Forward
class CGUIFont;
class CGUIFontTTF;
class TiXmlDocument;
class TiXmlNode;

struct OrigFontInfo
{
   int size;
   float aspect;
   CStdString fontFilePath;
   CStdString fileName;
   RESOLUTION sourceRes;
   bool preserveAspect;
   bool border;
};

/*!
 \ingroup textures
 \brief
 */
class GUIFontManager
{
public:
  GUIFontManager(void);
  virtual ~GUIFontManager(void);
  void Unload(const CStdString& strFontName);
  void LoadFonts(const CStdString& strFontSet);
  CGUIFont* LoadTTF(const CStdString& strFontName, const CStdString& strFilename, color_t textColor, color_t shadowColor, const int iSize, const int iStyle, bool border = false, float lineSpacing = 1.0f, float aspect = 1.0f, RESOLUTION res = INVALID, bool preserveAspect = false);
  CGUIFont* GetFont(const CStdString& strFontName, bool fallback = true);

  /*! \brief return a default font
   \param border whether the font should be a font with an outline
   \return the font.  NULL if no default font can be found.
   */
  CGUIFont* GetDefaultFont(bool border = false);

  void Clear();
  void FreeFontFile(CGUIFontTTF *pFont);

  bool IsFontSetUnicode() { return m_fontsetUnicode; }
  bool IsFontSetUnicode(const CStdString& strFontSet);
  bool GetFirstFontSetUnicode(CStdString& strFontSet);

  void ReloadTTFFonts(void);

protected:
  void RescaleFontSizeAndAspect(float *size, float *aspect, RESOLUTION sourceRes, bool preserveAspect) const;
  void LoadFonts(const TiXmlNode* fontNode);
  CGUIFontTTF* GetFontFile(const CStdString& strFontFile);
  bool OpenFontFile(TiXmlDocument& xmlDoc);

  std::vector<CGUIFont*> m_vecFonts;
  std::vector<CGUIFontTTF*> m_vecFontFiles;
  std::vector<OrigFontInfo> m_vecFontInfo;
  bool m_fontsetUnicode;
  RESOLUTION m_skinResolution;
};

/*!
 \ingroup textures
 \brief
 */
extern GUIFontManager g_fontManager;
#endif
