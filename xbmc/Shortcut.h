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
// Shortcut.h: interface for the CShortcut class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SHORTCUT_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
#define AFX_SHORTCUT_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "utils/StdString.h"

class CShortcut
{
public:
  CShortcut();
  virtual ~CShortcut();

  bool Create(const CStdString& szPath);
  bool Save(const CStdString& strFileName);

  CStdString m_strPath;
  CStdString m_strVideo;
  CStdString m_strParameters;
  CStdString m_strCustomGame;
  CStdString m_strThumb;
  CStdString m_strLabel;
};

#endif // !defined(AFX_SHORTCUT_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
