#ifndef DINPUT_KEYBOARD_H
#define DINPUT_KEYBOARD_H

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

class CLowLevelKeyboard
{
public:
  CLowLevelKeyboard();
  ~CLowLevelKeyboard();

  void Initialize(HWND hWnd);
  void Acquire();
  void Update();
  bool GetShift() { return m_bShift;};
  bool GetCtrl() { return m_bCtrl;};
  bool GetAlt() { return m_bAlt;};
  bool GetRAlt() { return m_bRAlt;};
  char GetAscii() { return m_cAscii;}; // FIXME should be replaced completly by GetUnicode() 
  WCHAR GetUnicode() { return GetAscii();}; // FIXME HELPME is there any unicode feature available?
  BYTE GetKey() { return m_VKey;};
  bool KeyHeld() const { return false; };

private:
  inline bool KeyDown(unsigned char key) const { return (m_keystate[key] & 0x80) ? true : false; };
  LPDIRECTINPUTDEVICE m_keyboard;
  unsigned char m_keystate[256];
  unsigned int m_keyDownLastFrame;

  bool m_bShift;
  bool m_bCtrl;
  bool m_bAlt;
  bool m_bRAlt;
  char m_cAscii;
  BYTE m_VKey;

  bool m_bInitialized;
  bool m_bKeyDown;
};

#endif

