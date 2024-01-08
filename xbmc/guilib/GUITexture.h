/*!
\file GUITexture.h
\brief
*/

#ifndef GUILIB_GUITEXTURE_H
#define GUILIB_GUITEXTURE_H

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

#include "TextureManager.h"
#include "Geometry.h"

typedef uint32_t color_t;

struct FRECT
{
  float left;
  float top;
  float right;
  float bottom;
};

// image alignment for <aspect>keep</aspect>, <aspect>scale</aspect> or <aspect>center</aspect>
#define ASPECT_ALIGN_CENTER  0
#define ASPECT_ALIGN_LEFT    1
#define ASPECT_ALIGN_RIGHT   2
#define ASPECT_ALIGNY_CENTER 0
#define ASPECT_ALIGNY_TOP    4
#define ASPECT_ALIGNY_BOTTOM 8
#define ASPECT_ALIGN_MASK    3
#define ASPECT_ALIGNY_MASK  ~3

class CAspectRatio
{
public:
  enum ASPECT_RATIO { AR_STRETCH = 0, AR_SCALE, AR_KEEP, AR_CENTER };
  CAspectRatio(ASPECT_RATIO aspect = AR_STRETCH)
  {
    ratio = aspect;
    align = ASPECT_ALIGN_CENTER | ASPECT_ALIGNY_CENTER;
    scaleDiffuse = true;
  };
  bool operator!=(const CAspectRatio &right) const
  {
    if (ratio != right.ratio) return true;
    if (align != right.align) return true;
    if (scaleDiffuse != right.scaleDiffuse) return true;
    return false;
  };

  ASPECT_RATIO ratio;
  uint32_t     align;
  bool         scaleDiffuse;
};

class CTextureInfo
{
public:
  CTextureInfo()
  {
    memset(&border, 0, sizeof(FRECT));
    orientation = 0;
    useLarge = false;
  };

  CTextureInfo(const CStdString &file)
  {
    memset(&border, 0, sizeof(FRECT));
    orientation = 0;
    useLarge = false;
    filename = file;
  }

  void operator=(const CTextureInfo &right)
  {
    memcpy(&border, &right.border, sizeof(FRECT));
    orientation = right.orientation;
    diffuse = right.diffuse;
    filename = right.filename;
    useLarge = right.useLarge;
  };
  bool       useLarge;
  FRECT      border;      // scaled  - unneeded if we get rid of scale on load
  int        orientation; // orientation of the texture (0 - 7 == EXIForientation - 1)
  CStdString diffuse;     // diffuse overlay texture
  CStdString filename;    // main texture file
};

class CGUITextureBase
{
public:
  CGUITextureBase(float posX, float posY, float width, float height, const CTextureInfo& texture);
  CGUITextureBase(const CGUITextureBase &left);
  virtual ~CGUITextureBase(void);

  void Render();

  void DynamicResourceAlloc(bool bOnOff);
  void PreAllocResources();
  void AllocResources();
  void FreeResources(bool immediately = false);

  void SetVisible(bool visible);
  void SetAlpha(unsigned char alpha);
  void SetDiffuseColor(color_t color);
  void SetPosition(float x, float y);
  void SetWidth(float width);
  void SetHeight(float height);
  void SetFileName(const CStdString &filename);
  void SetAspectRatio(const CAspectRatio &aspect);

  const CStdString& GetFileName() const { return m_info.filename; };
  float GetTextureWidth() const { return m_frameWidth; };
  float GetTextureHeight() const { return m_frameHeight; };
  float GetWidth() const { return m_width; };
  float GetHeight() const { return m_height; };
  float GetXPosition() const { return m_posX; };
  float GetYPosition() const { return m_posY; };
  int GetOrientation() const;
  const CRect &GetRenderRect() const { return m_vertex; };
  bool IsLazyLoaded() const { return m_info.useLarge; };

  bool HitTest(const CPoint &point) const { return CRect(m_posX, m_posY, m_posX + m_width, m_posY + m_height).PtInRect(point); };
  bool IsAllocated() const { return m_isAllocated != NO; };
  bool FailedToAlloc() const { return m_isAllocated == NORMAL_FAILED || m_isAllocated == LARGE_FAILED; };
  bool ReadyToRender() const;
protected:
  void CalculateSize();
  void LoadDiffuseImage();
  void AllocateOnDemand();
  void UpdateAnimFrame();
  void Render(float left, float top, float bottom, float right, float u1, float v1, float u2, float v2, float u3, float v3);
  void OrientateTexture(CRect &rect, float width, float height, int orientation);

  // functions that our implementation classes handle
  virtual void Allocate() {}; ///< called after our textures have been allocated
  virtual void Free() {};     ///< called after our textures have been freed
  virtual void Begin() {};
  virtual void Draw(float *x, float *y, float *z, const CRect &texture, const CRect &diffuse, color_t color, int orientation)=0;
  virtual void End() {};

  bool m_visible;
  color_t m_diffuseColor;

  float m_posX;         // size of the frame
  float m_posY;
  float m_width;
  float m_height;

  CRect m_vertex;       // vertex coords to render
  bool m_invalid;       // if true, we need to recalculate

  unsigned char m_alpha;

  float m_frameWidth, m_frameHeight;          // size in pixels of the actual frame within the texture
  float m_texCoordsScaleU, m_texCoordsScaleV; // scale factor for pixel->texture coordinates

  // animations
  int m_currentLoop;
  unsigned int m_currentFrame;
  DWORD m_frameCounter;

  float m_diffuseU, m_diffuseV;           // size of the diffuse frame (in tex coords)
  float m_diffuseScaleU, m_diffuseScaleV; // scale factor of the diffuse frame (from texture coords to diffuse tex coords)
  CPoint m_diffuseOffset;                 // offset into the diffuse frame (it's not always the origin)

  bool m_allocateDynamically;
  enum ALLOCATE_TYPE { NO = 0, NORMAL, LARGE, NORMAL_FAILED, LARGE_FAILED };
  ALLOCATE_TYPE m_isAllocated;

  CTextureInfo m_info;
  CAspectRatio m_aspect;

  int m_largeOrientation;   // orientation for large textures

  CTexture m_diffuse;
  CTexture m_texture;
};

#ifndef HAS_SDL
#include "GUITextureD3D.h"
#elif defined(HAS_SDL_2D)
#include "GUITextureSDL.h"
#elif defined(HAS_SDL_OPENGL)
#include "GUITextureGL.h"
#endif

#endif
