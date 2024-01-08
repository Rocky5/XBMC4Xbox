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
#include "VisualisationFactory.h"
#include "Util.h"
#include "utils/URIUtils.h"


CVisualisationFactory::CVisualisationFactory()
{

}

CVisualisationFactory::~CVisualisationFactory()
{

}

CVisualisation* CVisualisationFactory::LoadVisualisation(const CStdString& strVisz) const
{
  // strip of the path & extension to get the name of the visualisation
  // like goom or spectrum
  CStdString strName = URIUtils::GetFileName(strVisz);
  strName = strName.Left(strName.size() - 4);

#ifdef HAS_VISUALISATION
  // load visualisation
  DllVisualisation* pDll = new DllVisualisation;
  pDll->SetFile(strVisz);
  //  FIXME: Some Visualisations do not work 
  //  when their dll is not unloaded immediatly
  pDll->EnableDelayedUnload(false);
  if (!pDll->Load())
  {
    delete pDll;
    return NULL;
  }

  struct Visualisation* pVisz = (struct Visualisation*)malloc(sizeof(struct Visualisation));
  ZeroMemory(pVisz, sizeof(struct Visualisation));
  pDll->GetModule(pVisz);

  // and pass it to a new instance of CVisualisation() which will hanle the visualisation
  return new CVisualisation(pVisz, pDll, strName);
#else
  return NULL;
#endif
}
