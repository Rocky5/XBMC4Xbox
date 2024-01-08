#pragma once
#include "DynamicDll.h"
#include "DllAvUtil.h"

extern "C" {
#ifndef HAVE_MMX
#define HAVE_MMX
#endif

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#ifndef __GNUC__
#pragma warning(disable:4244)
#endif

#include "Codecs/ffmpeg/libswscale/swscale.h"
#include "Codecs/ffmpeg/libswscale/rgb2rgb.h"
}

#include "settings/Settings.h"

class DllSwScaleInterface
{
public:
   virtual ~DllSwScaleInterface() {}
   virtual struct SwsContext *sws_getContext(int srcW, int srcH, int srcFormat, int dstW, int dstH, int dstFormat, int flags,
                                  SwsFilter *srcFilter, SwsFilter *dstFilter, double *param)=0;

   virtual int sws_scale(struct SwsContext *context, uint8_t* src[], int srcStride[], int srcSliceY,
                         int srcSliceH, uint8_t* dst[], int dstStride[])=0;
    #if (! defined USE_EXTERNAL_FFMPEG)
      virtual void sws_rgb2rgb_init(int flags)=0;
    #elif (defined HAVE_LIBSWSCALE_RGB2RGB_H) || (defined HAVE_FFMPEG_RGB2RGB_H)
   virtual void sws_rgb2rgb_init(int flags)=0;
    #endif

   virtual void sws_freeContext(struct SwsContext *context)=0;
};

#if (defined USE_EXTERNAL_FFMPEG)

// We call into this library directly.
class DllSwScale : public DllDynamic, public DllSwScaleInterface
{
public:
  virtual ~DllSwScale() {}
  virtual struct SwsContext *sws_getContext(int srcW, int srcH, int srcFormat, int dstW, int dstH, int dstFormat, int flags,
                               SwsFilter *srcFilter, SwsFilter *dstFilter, double *param) 
    { return ::sws_getContext(srcW, srcH, (enum PixelFormat)srcFormat, dstW, dstH, (enum PixelFormat)dstFormat, flags, srcFilter, dstFilter, param); }

  virtual int sws_scale(struct SwsContext *context, uint8_t* src[], int srcStride[], int srcSliceY,
                int srcSliceH, uint8_t* dst[], int dstStride[])  
    { return ::sws_scale(context, src, srcStride, srcSliceY, srcSliceH, dst, dstStride); }
  #if (! defined USE_EXTERNAL_FFMPEG)
    virtual void sws_rgb2rgb_init(int flags) { ::sws_rgb2rgb_init(flags); }
  #elif (defined HAVE_LIBSWSCALE_RGB2RGB_H) || (defined HAVE_FFMPEG_RGB2RGB_H)
  virtual void sws_rgb2rgb_init(int flags) { ::sws_rgb2rgb_init(flags); }
  #endif
  virtual void sws_freeContext(struct SwsContext *context) { ::sws_freeContext(context); }
  
  // DLL faking.
  virtual bool ResolveExports() { return true; }
  virtual bool Load() {
    CLog::Log(LOGDEBUG, "DllSwScale: Using libswscale system library");
    return true;
  }
  virtual void Unload() {}
};

#else

class DllSwScale : public DllDynamic, public DllSwScaleInterface
{
public:
  DllSwScale() : DllDynamic( g_settings.GetFFmpegDllFolder() + "swscale-2.dll") {}
  DEFINE_METHOD10(struct SwsContext *, sws_getContext, ( int p1, int p2, int p3, int p4, int p5, int p6, int p7, 
							 SwsFilter * p8, SwsFilter * p9, double * p10))
  DEFINE_METHOD7(int, sws_scale, (struct SwsContext *p1, uint8_t** p2, int *p3, int p4, int p5, uint8_t **p6, int *p7))
  DEFINE_METHOD1(void, sws_rgb2rgb_init, (int p1))
  DEFINE_METHOD1(void, sws_freeContext, (struct SwsContext *p1))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(sws_getContext)
    RESOLVE_METHOD(sws_scale)
    RESOLVE_METHOD(sws_rgb2rgb_init)
    RESOLVE_METHOD(sws_freeContext)
  END_METHOD_RESOLVE()

  /* dependency of libswscale */
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
