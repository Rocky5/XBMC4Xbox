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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "DynamicDll.h"

extern "C" {
#ifndef HAVE_MMX
#define HAVE_MMX
#endif
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __GNUC__
#pragma warning(disable:4244)
#endif

#include "libswresample/swresample.h"

}

class DllSwResampleInterface
{
public:
  virtual ~DllSwResampleInterface() {}
  virtual struct SwrContext *swr_alloc_set_opts(struct SwrContext *s, int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate, int64_t in_ch_layout, enum AVSampleFormat in_sample_fmt, int in_sample_rate, int log_offset, void *log_ctx)=0;
  virtual int swr_init(struct SwrContext *s)=0;
  virtual void swr_free(struct SwrContext **s)=0;
  virtual int swr_convert(struct SwrContext *s, uint8_t **out, int out_count, const uint8_t **in , int in_count)=0;
};

#if (defined USE_EXTERNAL_FFMPEG)

// Use direct mapping
class DllSwResample : public DllDynamic, DllSwResampleInterface
{
public:
  virtual ~DllSwResample() {}

  // DLL faking.
  virtual bool ResolveExports() { return true; }
  virtual bool Load() {
    CLog::Log(LOGDEBUG, "DllAvFormat: Using libswresample system library");
    return true;
  }
  virtual void Unload() {}
  virtual struct SwrContext *swr_alloc_set_opts(struct SwrContext *s, int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate, int64_t in_ch_layout, enum AVSampleFormat in_sample_fmt, int in_sample_rate, int log_offset, void *log_ctx) { return ::swr_alloc_set_opts(s, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout, in_sample_fmt, in_sample_rate, log_offset, log_ctx); }
  virtual int swr_init(struct SwrContext *s) { return ::swr_init(s); }
  virtual void swr_free(struct SwrContext **s){ return ::swr_free(s); }
  virtual int swr_convert(struct SwrContext *s, uint8_t **out, int out_count, const uint8_t **in , int in_count){ return ::swr_convert(s, out, out_count, in, in_count); }
};

#else

class DllSwResample : public DllDynamic, DllSwResampleInterface
{
public:
  DllSwResample() : DllDynamic( g_settings.GetFFmpegDllFolder() + "swresample-0.dll") {}

  LOAD_SYMBOLS()

  DEFINE_METHOD9(SwrContext*, swr_alloc_set_opts, (struct SwrContext *p1, int64_t p2, enum AVSampleFormat p3, int p4, int64_t p5, enum AVSampleFormat p6, int p7, int p8, void * p9));
  DEFINE_METHOD1(int, swr_init, (struct SwrContext *p1))
  DEFINE_METHOD1(void, swr_free, (struct SwrContext **p1))
  DEFINE_METHOD5(int, swr_convert, (struct SwrContext *p1, uint8_t **p2, int p3, const uint8_t **p4, int p5))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(swr_alloc_set_opts)
    RESOLVE_METHOD(swr_init)
    RESOLVE_METHOD(swr_free)
    RESOLVE_METHOD(swr_convert)
  END_METHOD_RESOLVE()

  /* dependencies of libavformat */
  DllAvUtil m_dllAvUtil;

public:

  virtual bool Load()
  {
    if (!m_dllAvUtil.Load())
      return false;
    return DllDynamic::Load();
  }
};

#endif
