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

#include "DVDVideoCodec.h"
#include "Codecs/DllAvCodec.h"
#include "Codecs/DllAvFormat.h"
#include "Codecs/DllAvUtil.h"
#include "Codecs/DllSwScale.h"

class CDVDVideoCodecFFmpeg : public CDVDVideoCodec
{
public:
  CDVDVideoCodecFFmpeg();
  virtual ~CDVDVideoCodecFFmpeg();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);  
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize, double dts, double pts);
  virtual void Reset();
  bool GetPictureCommon(DVDVideoPicture* pDvdVideoPicture);
  virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual const char* GetName() { return m_name.c_str(); }; // m_name is never changed after open

protected:
  friend int my_get_buffer(struct AVCodecContext *, AVFrame *);
  friend void my_release_buffer(struct AVCodecContext *, AVFrame *);

  void GetVideoAspect(AVCodecContext* CodecContext, unsigned int& iWidth, unsigned int& iHeight);

  AVFrame* m_pFrame;
  AVCodecContext* m_pCodecContext;

  AVPicture* m_pConvertFrame;

  int m_iPictureWidth;
  int m_iPictureHeight;

  int m_iScreenWidth;
  int m_iScreenHeight;

  DllAvCodec m_dllAvCodec;
  DllAvUtil  m_dllAvUtil;
  DllSwScale m_dllSwScale;
  std::string m_name;
  double m_dts;
};

