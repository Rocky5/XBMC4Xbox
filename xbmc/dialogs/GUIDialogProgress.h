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

#include "dialogs/GUIDialogBoxBase.h"
#include "IProgressCallback.h"

class CGUIDialogProgress :
      public CGUIDialogBoxBase, public IProgressCallback
{
public:
  CGUIDialogProgress(void);
  virtual ~CGUIDialogProgress(void);

  virtual void DoModal(int iWindowID = WINDOW_INVALID, const CStdString &param = "");  ///< Override that just starts the window up.  Needs Close() to close it
  void StartModal(bool threadSafe = false);  ///< Thread safe routine wrapping DoModal() if set to true
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnBack(int actionID);
  virtual void OnWindowLoaded();
  void Progress();
  void ProgressKeys();
  bool IsCanceled() const { return m_bCanceled; }
  void SetPercentage(int iPercentage);
  int GetPercentage() const { return m_percentage; };
  void ShowProgressBar(bool bOnOff);
  void SetHeading(const std::string& strLine);
  void SetHeading(int iString);             // for convenience to lookup in strings.xml

  // Implements IProgressCallback
  virtual void SetProgressMax(int iMax);
  virtual void SetProgressAdvance(int nSteps=1);
  virtual bool Abort();

  void SetCanCancel(bool bCanCancel);

protected:
  bool m_bCanCancel;
  bool m_bCanceled;
  std::string m_strHeading;

  int  m_iCurrent;
  int  m_iMax;
  int m_percentage;
};
