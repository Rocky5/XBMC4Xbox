/*!
\file GraphicContext.h
\brief
*/

#ifndef GUILIB_GRAPHICCONTEXT_H
#define GUILIB_GRAPHICCONTEXT_H

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

#include <vector>
#include <stack>
#include "utils/CriticalSection.h"  // base class
#include "TransformMatrix.h"        // for the members m_guiTransform etc.
#include "Geometry.h"               // for CRect/CPoint
#include "gui3d.h"
#include "utils/StdString.h"

#include "common/Mouse.h"

/*!
 \ingroup graphics
 \brief
 */
enum RESOLUTION {
  INVALID = -1,
  HDTV_1080i = 0,
  HDTV_720p = 1,
  HDTV_480p_4x3 = 2,
  HDTV_480p_16x9 = 3,
  NTSC_4x3 = 4,
  NTSC_16x9 = 5,
  PAL_4x3 = 6,
  PAL_16x9 = 7,
  PAL60_4x3 = 8,
  PAL60_16x9 = 9,
  AUTORES = 10
};

enum VIEW_TYPE { VIEW_TYPE_NONE = 0,
                 VIEW_TYPE_LIST,
                 VIEW_TYPE_ICON,
                 VIEW_TYPE_BIG_LIST,
                 VIEW_TYPE_BIG_ICON,
                 VIEW_TYPE_WIDE,
                 VIEW_TYPE_BIG_WIDE,
                 VIEW_TYPE_WRAP,
                 VIEW_TYPE_BIG_WRAP,
                 VIEW_TYPE_AUTO,
                 VIEW_TYPE_MAX };

/*!
 \ingroup graphics
 \brief
 */
struct OVERSCAN
{
  int left;
  int top;
  int right;
  int bottom;
};

/*!
 \ingroup graphics
 \brief
 */
struct RESOLUTION_INFO
{
  OVERSCAN Overscan;
  int iWidth;
  int iHeight;
  int iSubtitles;
  DWORD dwFlags;
  float fPixelRatio;
  char strMode[11];
};

/*!
 \ingroup graphics
 \brief
 */
class CGraphicContext : public CCriticalSection
{
public:
  CGraphicContext(void);
  virtual ~CGraphicContext(void);
  LPDIRECT3DDEVICE8 Get3DDevice() { return m_pd3dDevice; }
  void SetD3DDevice(LPDIRECT3DDEVICE8 p3dDevice);
  //  void         GetD3DParameters(D3DPRESENT_PARAMETERS &params);
  void SetD3DParameters(D3DPRESENT_PARAMETERS *p3dParams);
  int GetBackbufferCount() const { return (m_pd3dParams)?m_pd3dParams->BackBufferCount:0; }
  int GetWidth() const { return m_iScreenWidth; }
  int GetHeight() const { return m_iScreenHeight; }
  int GetFPS() const;
  DWORD GetNewID();
  const CStdString& GetMediaDir() const { return m_strMediaDir; }
  void SetMediaDir(const CStdString& strMediaDir);
  bool IsWidescreen() const { return m_bWidescreen; }
  bool SetViewPort(float fx, float fy , float fwidth, float fheight, bool intersectPrevious = false);
  void RestoreViewPort();
  const RECT& GetViewWindow() const;
  void SetViewWindow(float left, float top, float right, float bottom);
  void SetFullScreenViewWindow(RESOLUTION &res);
  void ClipToViewWindow();
  void SetFullScreenVideo(bool bOnOff);
  bool IsFullScreenVideo() const;
  bool IsCalibrating() const;
  void SetCalibrating(bool bOnOff);
  void GetAllowedResolutions(std::vector<RESOLUTION> &res, bool bAllowPAL60 = false);
  bool IsValidResolution(RESOLUTION res);
  void SetVideoResolution(RESOLUTION &res, BOOL NeedZ = FALSE, bool forceClear = false);
  RESOLUTION GetVideoResolution() const;
  void SetScreenFilters(bool useFullScreenFilters);
  void ResetOverscan(RESOLUTION res, OVERSCAN &overscan);
  void ResetScreenParameters(RESOLUTION res);
  void Lock() { EnterCriticalSection(*this); }
  void Unlock() { LeaveCriticalSection(*this); }
  float GetPixelRatio(RESOLUTION iRes) const;
  void CaptureStateBlock();
  void ApplyStateBlock();
  void Clear(color_t color = 0);

  // output scaling
  void SetRenderingResolution(RESOLUTION res, bool needsScaling);  ///< Sets scaling up for rendering
  void SetScalingResolution(RESOLUTION res, bool needsScaling);    ///< Sets scaling up for skin loading etc.
  float GetScalingPixelRatio() const;

  void InvertFinalCoords(float &x, float &y) const;
  inline float ScaleFinalXCoord(float x, float y) const { return m_finalTransform.TransformXCoord(x, y, 0); }
  inline float ScaleFinalYCoord(float x, float y) const { return m_finalTransform.TransformYCoord(x, y, 0); }
  inline float ScaleFinalZCoord(float x, float y) const { return m_finalTransform.TransformZCoord(x, y, 0); }
  inline void ScaleFinalCoords(float &x, float &y, float &z) const { m_finalTransform.TransformPosition(x, y, z); }
  bool RectIsAngled(float x1, float y1, float x2, float y2) const;

  inline float GetGUIScaleX() const { return m_guiScaleX; };
  inline float GetGUIScaleY() const { return m_guiScaleY; };
  inline DWORD MergeAlpha(color_t color) const
  {
    color_t alpha = m_finalTransform.TransformAlpha((color >> 24) & 0xff);
    if (alpha > 255) alpha = 255;
    return ((alpha << 24) & 0xff000000) | (color & 0xffffff);
  }

  void SetOrigin(float x, float y);
  void RestoreOrigin();
  void SetCameraPosition(const CPoint &camera);
  void RestoreCameraPosition();
  /*! \brief Set a region in which to clip all rendering
   Anything that is rendered after setting a clip region will be clipped so that no part renders
   outside of the clip region.  Successive calls to SetClipRegion intersect the clip region, which
   means the clip region may eventually become an empty set.  In this case SetClipRegion returns false
   to indicate that no rendering need be performed.

   This call must be matched with a RestoreClipRegion call unless SetClipRegion returns false.

   Usage should be of the form:

     if (SetClipRegion(x, y, w, h))
     {
       ...
       perform rendering
       ...
       RestoreClipRegion();
     }

   \param x the left-most coordinate of the clip region
   \param y the top-most coordinate of the clip region
   \param w the width of the clip region
   \param h the height of the clip region
   \returns true if the region is set and the result is non-empty. Returns false if the resulting region is empty.
   \sa RestoreClipRegion
   */
  bool SetClipRegion(float x, float y, float w, float h);

   /*! \brief Restore a clip region to the previous clip region (if any) prior to the last SetClipRegion call
    This function should be within an if (SetClipRegion(x,y,w,h)) block.
    \sa SetClipRegion
    */
  void RestoreClipRegion();
  void ClipRect(CRect &vertex, CRect &texture, CRect *diffuse = NULL);
  inline void ResetWindowTransform()
  {
    while (m_groupTransform.size())
      m_groupTransform.pop();
    m_groupTransform.push(m_guiTransform);
  }
  inline void AddTransform(const TransformMatrix &matrix)
  {
    ASSERT(m_groupTransform.size());
    if (m_groupTransform.size())
      m_groupTransform.push(m_groupTransform.top() * matrix);
    else
      m_groupTransform.push(matrix);
    UpdateFinalTransform(m_groupTransform.top());
  }
  inline void RemoveTransform()
  {
    ASSERT(m_groupTransform.size() > 1);
    if (m_groupTransform.size())
      m_groupTransform.pop();
    if (m_groupTransform.size())
      UpdateFinalTransform(m_groupTransform.top());
    else
      UpdateFinalTransform(TransformMatrix());
  }

  int GetMaxTextureSize() const { return m_maxTextureSize; };
protected:
  LPDIRECT3DDEVICE8 m_pd3dDevice;
  D3DPRESENT_PARAMETERS* m_pd3dParams;
  std::stack<D3DVIEWPORT8*> m_viewStack;
  DWORD m_stateBlock;
  int m_iScreenHeight;
  int m_iScreenWidth;
  int m_iBackBufferCount;
  bool m_bWidescreen;
  CStdString m_strMediaDir;
  RECT m_videoRect;
  bool m_bFullScreenVideo;
  bool m_bCalibrating;
  RESOLUTION m_Resolution;

private:
  void UpdateCameraPosition(const CPoint &camera);
  void UpdateFinalTransform(const TransformMatrix &matrix);
  RESOLUTION m_windowResolution;
  float m_guiScaleX;
  float m_guiScaleY;
  std::stack<CPoint> m_cameras;
  std::stack<CPoint> m_origins;
  std::stack<CRect>  m_clipRegions;

  TransformMatrix m_guiTransform;
  TransformMatrix m_finalTransform;
  std::stack<TransformMatrix> m_groupTransform;

  int m_maxTextureSize;
};

/*!
 \ingroup graphics
 \brief
 */
extern CGraphicContext g_graphicsContext;
#endif
