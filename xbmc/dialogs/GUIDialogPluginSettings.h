#ifndef GUIDIALOG_PLUGIN_SETTINGS_
#define GUIDIALOG_PLUGIN_SETTINGS_

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
#include "PluginSettings.h"

struct SScraperInfo;

class CGUIDialogPluginSettings : public CGUIDialogBoxBase
{
public:
  CGUIDialogPluginSettings(void);
  virtual ~CGUIDialogPluginSettings(void);
  virtual bool OnMessage(CGUIMessage& message);
  /*! \brief Show the addon settings dialog, allowing the user to configure an addon
   \param addon the addon to configure
   \param saveToDisk whether the changes should be saved to disk or just made local to the addon.  Defaults to true
   \return true if settings were changed and the dialog confirmed, false otherwise.
   */
  static bool ShowAndGetInput(CURL& url, bool saveToDisk = true);
  static bool ShowAndGetInput(SScraperInfo& info, bool saveToDisk = true);
  static bool ShowAndGetInput(CStdString& path, bool saveToDisk = true);
  virtual void Render();

protected:
  virtual void OnInitWindow();

private:
  /*! \brief return a (localized) addon string.
   \param value either a character string (which is used directly) or a number to lookup in the addons strings.xml
   \param subsetting whether the character string should be prefixed by "- ", defaults to false
   \return the localized addon string
   */
  CStdString GetString(const char *value, bool subSetting = false) const;

  /*! \brief return a the values for a fileenum setting
   \param path the path to use for files
   \param mask the mask to use
   \param options any options, such as "hideext" to hide extensions
   \return the filenames in the path that match the mask
   */
  std::vector<CStdString> GetFileEnumValues(const CStdString &path, const CStdString &mask, const CStdString &options) const;

  void CreateSections();
  void FreeSections();
  void CreateControls();
  void FreeControls();
  void UpdateFromControls();
  void EnableControls();
  void SetDefaults();
  bool GetCondition(const CStdString &condition, const int controlId);

  void SaveSettings(void);
  bool ShowVirtualKeyboard(int iControl);
  bool TranslateSingleString(const CStdString &strCondition, std::vector<CStdString> &enableVec);

  const TiXmlElement *GetFirstSetting();

  CBasicSettings m_addon;
  CStdString m_strHeading;
  std::map<CStdString,CStdString> m_buttonValues;
  CStdString m_closeAction;
  CStdString m_profile;
  bool m_changed;
  bool m_saveToDisk; // whether the addon settings should be saved to disk or just stored locally in the addon

  unsigned int m_currentSection;
  unsigned int m_totalSections;

  CStdString NormalizePath(const char *value) const;

  std::map<CStdString,CStdString> m_settings; // local storage of values
  static CURL m_url;
};

#endif
