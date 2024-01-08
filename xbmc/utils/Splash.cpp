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

#include "system.h"
#include "Splash.h"
#include "guiImage.h"
#include "FileSystem/File.h"
#include "log.h"

using namespace XFILE;

CSplash::CSplash(const CStdString& imageName)
{
  m_ImageName = imageName;
}


CSplash::~CSplash()
{
  Stop();
}

void CSplash::OnStartup()
{}

void CSplash::OnExit()
{}

void CSplash::Process()
{
  D3DGAMMARAMP newRamp;
  D3DGAMMARAMP oldRamp;

  g_graphicsContext.Lock();
  g_graphicsContext.Get3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
  
  g_graphicsContext.SetCameraPosition(CPoint(0, 0));
  float w = g_graphicsContext.GetWidth() * 0.5f;
  float h = g_graphicsContext.GetHeight() * 0.5f;
  CGUIImage* image = new CGUIImage(0, 0, w*0.5f, h*0.5f, w, h, m_ImageName);
  image->SetAspectRatio(CAspectRatio::AR_KEEP);
  image->AllocResources();

  // Store the old gamma ramp
  g_graphicsContext.Get3DDevice()->GetGammaRamp(&oldRamp);
  float fade = 0.5f;
  for (int i = 0; i < 256; i++)
  {
    newRamp.red[i] = (int)((float)oldRamp.red[i] * fade);
    newRamp.green[i] = (int)((float)oldRamp.red[i] * fade);
    newRamp.blue[i] = (int)((float)oldRamp.red[i] * fade);
  }
  g_graphicsContext.Get3DDevice()->SetGammaRamp(GAMMA_RAMP_FLAG, &newRamp);
  //render splash image
#ifndef HAS_XBOX_D3D
  g_graphicsContext.Get3DDevice()->BeginScene();
#endif
  image->Render();
  image->FreeResources();
  delete image;
  //show it on screen
#ifdef HAS_XBOX_D3D
  g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();
#else
  g_graphicsContext.Get3DDevice()->EndScene();
#endif
  g_graphicsContext.Get3DDevice()->Present( NULL, NULL, NULL, NULL );
  g_graphicsContext.Unlock();

  //fade in and wait untill the thread is stopped
  while (!m_bStop)
  {
    if (fade <= 1.f)
    {
      Sleep(1);
      for (int i = 0; i < 256; i++)
      {
        newRamp.red[i] = (int)((float)oldRamp.red[i] * fade);
        newRamp.green[i] = (int)((float)oldRamp.green[i] * fade);
        newRamp.blue[i] = (int)((float)oldRamp.blue[i] * fade);
      }
      g_graphicsContext.Lock();
      g_graphicsContext.Get3DDevice()->SetGammaRamp(GAMMA_RAMP_FLAG, &newRamp);
      g_graphicsContext.Unlock();
      fade += 0.01f;
    }
    else
    {
      Sleep(10);
    }
  }

  g_graphicsContext.Lock();
  // fade out
  for (float fadeout = fade - 0.01f; fadeout >= 0.f; fadeout -= 0.01f)
  {
    for (int i = 0; i < 256; i++)
    {
      newRamp.red[i] = (int)((float)oldRamp.red[i] * fadeout);
      newRamp.green[i] = (int)((float)oldRamp.green[i] * fadeout);
      newRamp.blue[i] = (int)((float)oldRamp.blue[i] * fadeout);
    }
    Sleep(1);
    g_graphicsContext.Get3DDevice()->SetGammaRamp(GAMMA_RAMP_FLAG, &newRamp);
  }
  //restore original gamma ramp
  g_graphicsContext.Get3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
  g_graphicsContext.Get3DDevice()->SetGammaRamp(0, &oldRamp);
  g_graphicsContext.Get3DDevice()->Present( NULL, NULL, NULL, NULL );
  g_graphicsContext.Unlock();
}

bool CSplash::Start()
{
  if (m_ImageName.IsEmpty() || !CFile::Exists(m_ImageName))
  {
    CLog::Log(LOGDEBUG, "Splash image %s not found", m_ImageName.c_str());
    return false;
  }
  Create();
  return true;
}

void CSplash::Stop()
{
  StopThread();
}

bool CSplash::IsRunning()
{
  return (m_ThreadHandle != NULL);
}
