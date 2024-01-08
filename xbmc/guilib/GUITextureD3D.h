/*!
\file GUITextureD3D.h
\brief
*/

#ifndef GUILIB_GUITEXTURED3D_H
#define GUILIB_GUITEXTURED3D_H

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

#include "GUITexture.h"

#ifndef HAS_SDL

#define CGUITexture CGUITextureD3D

class CGUITextureD3D : public CGUITextureBase
{
public:
  CGUITextureD3D(float posX, float posY, float width, float height, const CTextureInfo& texture);
protected:
  void Begin();
  void Draw(float *x, float *y, float *z, const CRect &texture, const CRect &diffuse, color_t color, int orientation);
  void End();
};

#endif

#endif