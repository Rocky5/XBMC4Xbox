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
#include "IOSDOption.h"
#include "IExecutor.h"
#include "GUISliderControl.h"
namespace OSD
{
class COSDOptionFloatRange :
      public IOSDOption
{
public:
  COSDOptionFloatRange(int iAction, int iHeading);
  COSDOptionFloatRange(int iAction, int iHeading, float fStart, float fEnd, float fInterval, float fValue);
  COSDOptionFloatRange(const COSDOptionFloatRange& option);
  const OSD::COSDOptionFloatRange& operator = (const COSDOptionFloatRange& option);


  virtual ~COSDOptionFloatRange(void);
  virtual IOSDOption* Clone() const;
  virtual void Draw(int x, int y, bool bFocus = false, bool bSelected = false);
  virtual bool OnAction(IExecutor& executor, const CAction& action);
  float GetValue() const;

  virtual int GetMessage() const { return m_iAction;};
  virtual void SetValue(int iValue){};
  virtual void SetLabel(const CStdString& strLabel){};
private:
  CGUISliderControl m_slider;
  int m_iAction;
  float m_fMin;
  float m_fMax;
  float m_fInterval;
  float m_fValue;
};
};
