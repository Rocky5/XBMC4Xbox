// class CSurface
#include "SurfaceSDL.h"

#ifdef USE_SDL
#include <stdio.h>
#include "xbox.h"

CGraphicsDevice g_device;

CSurface::CSurface()
{
  m_surface = NULL;
  m_width = 0;
  m_height = 0;
  m_bpp = 0;
}

CSurface::~CSurface()
{
  if (m_surface)
    SDL_FreeSurface(m_surface);
}

bool CSurface::Create(unsigned int width, unsigned int height, CSurface::FORMAT format)
{
  if (m_surface)
  {
    SDL_FreeSurface(m_surface);
    m_surface = NULL;
  }

  m_info.width = width;
  m_info.height = height;
  m_info.format = format;

  if (format == FMT_LIN_ARGB)
  { // round width to multiple of 64 pixels
    m_width = (width + 63) & ~63;
    m_height = height;
  }
  else
  { // round up to nearest power of 2
    m_width = PadPow2(width);
    m_height = PadPow2(height);
  }
  m_bpp = (format == FMT_PALETTED) ? 1 : 4;

  m_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, m_width, m_height, m_bpp * 8, 
                                  0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);

  if (0 == m_surface)
    return false;

  Clear();
  return true;
}

void CSurface::Clear()
{
  CSurfaceRect rect;
  if (Lock(&rect))
  {
    BYTE *pixels = rect.pBits;
    for (unsigned int i = 0; i < m_height; i++)
    {
      memset(pixels, 0, rect.Pitch);
      pixels += rect.Pitch;
    }
    Unlock();
  }
}

bool CSurface::CreateFromFile(const char *Filename, FORMAT format)
{
  SDL_Surface *original = IMG_Load(Filename);
  if (!original)
    return false;

  if (!Create(original->w, original->h, format))
  {
    SDL_FreeSurface(original);
    return false;
  }

  // copy into our surface
  SDL_SetAlpha(original, 0, 255);
  int ret = SDL_BlitSurface(original, NULL, m_surface, NULL);
  SDL_FreeSurface(original);

  ClampToEdge();

  return (0 == ret);
}

void CSurface::ClampToEdge()
{
  // fix up the last row and column to simulate clamp_to_edge
  if (!m_info.width || !m_info.height == 0)
    return; // invalid texture
  CSurfaceRect rect;
  if (Lock(&rect))
  {
    for (unsigned int y = 0; y < m_info.height; y++)
    {
      BYTE *src = rect.pBits + y * rect.Pitch;
      for (unsigned int x = m_info.width; x < m_width; x++)
        memcpy(src + x*m_bpp, src + (m_info.width - 1)*m_bpp, m_bpp);
    }
    BYTE *src = rect.pBits + (m_info.height - 1) * rect.Pitch;
    for (unsigned int y = m_info.height; y < m_height; y++)
    {
      BYTE *dest = rect.pBits + y * rect.Pitch;
      memcpy(dest, src, rect.Pitch);
    }
    Unlock();
  }
}

bool CSurface::Lock(CSurfaceRect *rect)
{
  if (m_surface && rect)
  {
    SDL_LockSurface(m_surface);
    rect->pBits = (BYTE *)m_surface->pixels;
    rect->Pitch = m_surface->pitch;
    return true;
  }
  return false;
}

bool CSurface::Unlock()
{
  if (m_surface)
  {
    SDL_UnlockSurface(m_surface);
    return true;
  }
  return false;
}

CGraphicsDevice::CGraphicsDevice()
{
}

CGraphicsDevice::~CGraphicsDevice()
{
}

bool CGraphicsDevice::Create()
{
  return true;
}

bool CGraphicsDevice::CreateSurface(unsigned int width, unsigned int height, enum CSurface::FORMAT format, CSurface *surface)
{
  return false;
}
#endif

