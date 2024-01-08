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

#include "utils/Thread.h"
#include "utils/CriticalSection.h"
#include "TextureManager.h"

class CGUILargeTextureManager : public CThread
{
public:
  CGUILargeTextureManager();
  virtual ~CGUILargeTextureManager();

  virtual void Process();

  bool GetImage(const CStdString &path, CTexture &texture, int &orientation, bool firstRequest);
  void ReleaseImage(const CStdString &path, bool immediately = false);

  void CleanupUnusedImages();

protected:
  class CLargeTexture
  {
  public:

    CLargeTexture(const CStdString &path);
    virtual ~CLargeTexture();

    void AddRef();
    bool DecrRef(bool deleteImmediately);
    bool DeleteIfRequired();
    void SetTexture(LPDIRECT3DTEXTURE8 texture, int width, int height, int orientation);

    const CStdString &GetPath() const { return m_path; };
    const CTexture &GetTexture() const { return m_texture; };
    int GetOrientation() const { return m_orientation; };

  private:
    static const unsigned int TIME_TO_DELETE = 2000;

    unsigned int m_refCount;
    CStdString m_path;
    CTexture m_texture;
    int m_orientation;
    unsigned int m_timeToDelete;
  };

  void QueueImage(const CStdString &path);

private:
  std::vector<CLargeTexture *> m_queued;
  std::vector<CLargeTexture *> m_allocated;
  typedef std::vector<CLargeTexture *>::iterator listIterator;

  CCriticalSection m_listSection;
  CEvent m_listEvent;
  bool m_running;
};

extern CGUILargeTextureManager g_largeTextureManager;

