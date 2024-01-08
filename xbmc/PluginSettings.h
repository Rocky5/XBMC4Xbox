#ifndef PLUGINSETTINGS_H_
#define PLUGINSETTINGS_H_
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

#include "tinyXML/tinyxml.h"
#include "Url.h"

class CBasicSettings
{
public:
  CBasicSettings();
  virtual ~CBasicSettings();

  bool SaveFromDefault(void);
  virtual bool Load(const CURL& url)  { return false; }
  virtual bool Save(void) { return false; }
  void Clear();

  void Set(const CStdString& key, const CStdString& value);
  CStdString Get(const CStdString& key) const;

  TiXmlElement* GetPluginRoot();
protected: 
  TiXmlDocument   m_userXmlDoc;
  TiXmlDocument   m_pluginXmlDoc;
};

class CPluginSettings : public CBasicSettings
{
public:
  CPluginSettings();
  virtual ~CPluginSettings();
  bool Load(const CURL& url);
  bool Save(void);
  static bool SettingsExist(const CStdString &strPath);
  
  CPluginSettings& operator =(const CBasicSettings&);
private: 
  CStdString      m_id;
  CURL            m_url;
  CStdString      m_userFileName;  
};

extern CPluginSettings g_currentPluginSettings;

#endif
