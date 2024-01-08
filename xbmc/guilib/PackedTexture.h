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

#ifdef HAS_XBOX_D3D
HRESULT LoadPackedTexture(LPDIRECT3DDEVICE8 pDevice, const char* szFilename, D3DXIMAGE_INFO* pInfo, LPDIRECT3DTEXTURE8* ppTexture,
                          LPDIRECT3DPALETTE8* ppPalette);

int LoadPackedAnim(LPDIRECT3DDEVICE8 pDevice, const char* szFilename, D3DXIMAGE_INFO* pInfo, LPDIRECT3DTEXTURE8** ppTextures,
                   LPDIRECT3DPALETTE8* ppPalette, int& nLoops, int** ppDelays);
#endif
