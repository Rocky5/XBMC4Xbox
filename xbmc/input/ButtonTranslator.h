#ifndef BUTTON_TRANSLATOR_H
#define BUTTON_TRANSLATOR_H

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

#include <map>
#ifdef HAS_EVENT_SERVER
#include "utils/EventClient.h"
#endif

class CKey;
class CAction;
class TiXmlNode;
class TiXmlElement;

struct CButtonAction
{
  int id;
  CStdString strID; // needed for "XBMC.ActivateWindow()" type actions
};
///
/// singleton class to map from buttons to actions
/// Warning: _not_ threadsafe!
class CButtonTranslator
{
#ifdef HAS_EVENT_SERVER
  friend class EVENTCLIENT::CEventButtonState;
#endif
private:
  //private construction, and no assignements; use the provided singleton methods
  CButtonTranslator();
  CButtonTranslator(const CButtonTranslator&);
  CButtonTranslator const& operator=(CButtonTranslator const&);
  virtual ~CButtonTranslator();

public:
  ///access to singleton
  static CButtonTranslator& GetInstance();

  /// loads Lircmap.xml/IRSSmap.xml (if enabled) and Keymap.xml
  bool Load();
  /// clears the maps
  void Clear();

  CAction GetAction(int window, const CKey &key);

  /*! \brief Translate between a window name and it's id
   \param window name of the window
   \return id of the window, or WINDOW_INVALID if not found
   */
  static int TranslateWindow(const CStdString &window);

  /*! \brief Translate between a window id and it's name
   \param window id of the window
   \return name of the window, or an empty string if not found
   */
  static CStdString TranslateWindow(int window);

  static bool TranslateActionString(const char *szAction, int &action);

#if defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
  bool TranslateJoystickString(int window, const char* szDevice, int id,
                               bool axis, int& action, CStdString& strAction,
                               bool &fullrange);
#endif

private:
  typedef std::multimap<int, CButtonAction> buttonMap; // our button map to fill in
  std::map<int, buttonMap> translatorMap;       // mapping of windows to button maps
  int GetActionCode(int window, const CKey &key, CStdString &strAction);

  static int TranslateGamepadString(const char *szButton);
  static int TranslateRemoteString(const char *szButton);
  static int TranslateUniversalRemoteString(const char *szButton);
  static int TranslateKeyboardString(const char *szButton);
  static int TranslateKeyboardButton(TiXmlElement *pButton);

  void MapWindowActions(TiXmlNode *pWindow, int wWindowID);
  void MapAction(int buttonCode, const char *szAction, buttonMap &map);

  bool LoadKeymap(const CStdString &keymapPath);
#if defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
  void MapJoystickActions(int windowID, TiXmlNode *pJoystick);

  typedef std::map<int, std::map<int, std::string> > JoystickMap; // <window, <button/axis, action> >
  std::map<std::string, JoystickMap> m_joystickButtonMap;      // <joy name, button map>
  std::map<std::string, JoystickMap> m_joystickAxisMap;        // <joy name, axis map>
#endif
};

#endif

