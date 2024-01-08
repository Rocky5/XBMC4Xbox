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
#include "windows/GUIWindowWeather.h"
#include "GUIImage.h"
#include "utils/Weather.h"
#include "settings/GUISettings.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "lib/libPython/XBPython.h"
#include "LangInfo.h"
#include "utils/log.h"

#define CONTROL_BTNREFRESH             2
#define CONTROL_SELECTLOCATION         3
#define CONTROL_LABELUPDATED          11
#define CONTROL_IMAGELOGO            101

#define CONTROL_STATICTEMP           223
#define CONTROL_STATICFEEL           224
#define CONTROL_STATICUVID           225
#define CONTROL_STATICWIND           226
#define CONTROL_STATICDEWP           227
#define CONTROL_STATICHUMI           228

#define CONTROL_LABELD0DAY            31
#define CONTROL_LABELD0HI             32
#define CONTROL_LABELD0LOW            33
#define CONTROL_LABELD0GEN            34
#define CONTROL_IMAGED0IMG            35

#define MAX_LOCATION                   3
#define LOCALIZED_TOKEN_FIRSTID      370
#define LOCALIZED_TOKEN_LASTID       395

DWORD timeToCallPlugin = 1000;
/*
FIXME'S
>strings are not centered
*/

CGUIWindowWeather::CGUIWindowWeather(void)
    : CGUIWindow(WINDOW_WEATHER, "MyWeather.xml")
{
  m_iCurWeather = 0;
#ifdef _USE_ZIP_


#endif
}

CGUIWindowWeather::~CGUIWindowWeather(void)
{}

bool CGUIWindowWeather::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNREFRESH)
      {
        Refresh(); // Refresh clicked so do a complete update
      }
      else if (iControl == CONTROL_SELECTLOCATION)
      {
        // stop the plugin timer here, so the user has a full second
        if (m_pluginTimer.IsRunning())
          m_pluginTimer.Stop();

        CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_SELECTLOCATION);
        g_windowManager.SendMessage(msg);
        m_iCurWeather = msg.GetParam1();

        CStdString strLabel=g_weatherManager.GetLocation(m_iCurWeather);
        int iPos = strLabel.ReverseFind(", ");
        if (iPos)
        {
          CStdString strLabel2(strLabel);
          strLabel = strLabel2.substr(0,iPos);
        }

        SET_CONTROL_LABEL(CONTROL_SELECTLOCATION,strLabel);
        Refresh();
      }
    }
    break;
  case GUI_MSG_NOTIFY_ALL:
    if (message.GetParam1() == GUI_MSG_WINDOW_RESET)
    {
      g_weatherManager.Reset();
      return true;
    }
    else if (message.GetParam1() == GUI_MSG_WEATHER_FETCHED)
    {
      UpdateLocations();
      SetProperties();
      if (g_windowManager.GetActiveWindow() == WINDOW_WEATHER)
      {
        if (!g_guiSettings.GetString("weather.plugin").IsEmpty())
          m_pluginTimer.StartZero();
      }
      else
        CallPlugin();
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowWeather::OnInitWindow()
{
  // call UpdateButtons() so that we start with our initial stuff already present
  UpdateButtons();
  UpdateLocations();
  CGUIWindow::OnInitWindow();
}

void CGUIWindowWeather::UpdateLocations()
{
  if (!IsActive()) return;

  CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_SELECTLOCATION);
  g_windowManager.SendMessage(msg);
  CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_SELECTLOCATION);

  for (unsigned int i = 0; i < MAX_LOCATION; i++)
  {
    CStdString strLabel = g_weatherManager.GetLocation(i);
    if (strLabel.size() > 1) //got the location string yet?
    {
      int iPos = strLabel.ReverseFind(", ");
      if (iPos)
      {
        CStdString strLabel2(strLabel);
        strLabel = strLabel2.substr(0,iPos);
      }
      msg2.SetParam1(i);
      msg2.SetLabel(strLabel);
      g_windowManager.SendMessage(msg2);
    }
    else
    {
      strLabel.Format("AreaCode %i", i + 1);

      msg2.SetLabel(strLabel);
      msg2.SetParam1(i);
      g_windowManager.SendMessage(msg2);
    }
    if (i==m_iCurWeather)
      SET_CONTROL_LABEL(CONTROL_SELECTLOCATION,strLabel);
  }

  CONTROL_SELECT_ITEM(CONTROL_SELECTLOCATION, m_iCurWeather);
}

void CGUIWindowWeather::UpdateButtons()
{
    CONTROL_ENABLE(CONTROL_BTNREFRESH);

  SET_CONTROL_LABEL(CONTROL_BTNREFRESH, 184);   //Refresh

  SET_CONTROL_LABEL(WEATHER_LABEL_LOCATION, g_weatherManager.GetLocation(m_iCurWeather));
  SET_CONTROL_LABEL(CONTROL_LABELUPDATED, g_weatherManager.GetLastUpdateTime());

  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_COND, g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_COND));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_TEMP, g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_TEMP) + g_langInfo.GetTempUnitString());
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_FEEL, g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_FEEL) + g_langInfo.GetTempUnitString());
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_UVID, g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_UVID));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_WIND, g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_WIND));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_DEWP, g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_DEWP) + g_langInfo.GetTempUnitString());
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_HUMI, g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_HUMI));

  CGUIImage *pImage = (CGUIImage *)GetControl(WEATHER_IMAGE_CURRENT_ICON);
  if (pImage) pImage->SetFileName(g_weatherManager.GetInfo(WEATHER_IMAGE_CURRENT_ICON));

  //static labels
  SET_CONTROL_LABEL(CONTROL_STATICTEMP, 401);  //Temperature
  SET_CONTROL_LABEL(CONTROL_STATICFEEL, 402);  //Feels Like
  SET_CONTROL_LABEL(CONTROL_STATICUVID, 403);  //UV Index
  SET_CONTROL_LABEL(CONTROL_STATICWIND, 404);  //Wind
  SET_CONTROL_LABEL(CONTROL_STATICDEWP, 405);  //Dew Point
  SET_CONTROL_LABEL(CONTROL_STATICHUMI, 406);  //Humidity

  for (int i = 0; i < NUM_DAYS; i++)
  {
    SET_CONTROL_LABEL(CONTROL_LABELD0DAY + (i*10), g_weatherManager.m_dfForcast[i].m_day);
    SET_CONTROL_LABEL(CONTROL_LABELD0HI + (i*10), g_weatherManager.m_dfForcast[i].m_high + g_langInfo.GetTempUnitString());
    SET_CONTROL_LABEL(CONTROL_LABELD0LOW + (i*10), g_weatherManager.m_dfForcast[i].m_low + g_langInfo.GetTempUnitString());
    SET_CONTROL_LABEL(CONTROL_LABELD0GEN + (i*10), g_weatherManager.m_dfForcast[i].m_overview);
    pImage = (CGUIImage *)GetControl(CONTROL_IMAGED0IMG + (i * 10));
    if (pImage) pImage->SetFileName(g_weatherManager.m_dfForcast[i].m_icon);
  }
}

void CGUIWindowWeather::FrameMove()
{
  // update our controls
  UpdateButtons();

  // call weather plugin
  if (m_pluginTimer.IsRunning() && m_pluginTimer.GetElapsedMilliseconds() > timeToCallPlugin)
  {
    m_pluginTimer.Stop();
    CallPlugin();
  }

  CGUIWindow::FrameMove();
}

//Do a complete download, parse and update
void CGUIWindowWeather::Refresh()
{
  g_weatherManager.SetArea(m_iCurWeather);
  g_weatherManager.Refresh();
}

void CGUIWindowWeather::SetProperties()
{
  // Current weather
  SetProperty("Location", g_weatherManager.GetLocation(m_iCurWeather));
  SetProperty("LocationIndex", int(m_iCurWeather + 1));
  CStdString strSetting;
  strSetting.Format("weather.areacode%i", m_iCurWeather + 1);
  SetProperty("AreaCode", g_weatherManager.GetAreaCode(g_guiSettings.GetString(strSetting)));
  SetProperty("Updated", g_weatherManager.GetLastUpdateTime());
  SetProperty("Current.ConditionIcon", g_weatherManager.GetInfo(WEATHER_IMAGE_CURRENT_ICON));
  SetProperty("Current.Condition", g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_COND));
  SetProperty("Current.Temperature", g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_TEMP));
  SetProperty("Current.FeelsLike", g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_FEEL));
  SetProperty("Current.UVIndex", g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_UVID));
  SetProperty("Current.Wind", g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_WIND));
  SetProperty("Current.DewPoint", g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_DEWP));
  SetProperty("Current.Humidity", g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_HUMI));
  // we use the icons code number for fanart as it's the safest way
  CStdString fanartcode = URIUtils::GetFileName(g_weatherManager.GetInfo(WEATHER_IMAGE_CURRENT_ICON));
  URIUtils::RemoveExtension(fanartcode);
  SetProperty("Current.FanartCode", fanartcode);

  // Future weather
  CStdString day;
  for (int i = 0; i < NUM_DAYS; i++)
  {
    day.Format("Day%i.", i);
    SetProperty(day + "Title", g_weatherManager.m_dfForcast[i].m_day);
    SetProperty(day + "HighTemp", g_weatherManager.m_dfForcast[i].m_high);
    SetProperty(day + "LowTemp", g_weatherManager.m_dfForcast[i].m_low);
    SetProperty(day + "Outlook", g_weatherManager.m_dfForcast[i].m_overview);
    SetProperty(day + "OutlookIcon", g_weatherManager.m_dfForcast[i].m_icon);
    fanartcode = URIUtils::GetFileName(g_weatherManager.m_dfForcast[i].m_icon);
    URIUtils::RemoveExtension(fanartcode);
    SetProperty(day + "FanartCode", fanartcode);
  }
}

void CGUIWindowWeather::CallPlugin()
{
  if (!g_guiSettings.GetString("weather.plugin").IsEmpty())
  {
    // create the full path to the plugin
    CStdString plugin = "special://home/plugins/weather/" + g_guiSettings.GetString("weather.plugin") + "/default.py";

    // initialize our sys.argv variables
    unsigned int argc = 2;
    char ** argv = new char*[argc];
    argv[0] = (char*)plugin.c_str();

    // if plugin is running we wait for another timeout only when in weather window
    if (g_windowManager.GetActiveWindow() == WINDOW_WEATHER)
    {
      int id = g_pythonParser.getScriptId(argv[0]);
      if (id != -1 && g_pythonParser.isRunning(id))
      {
        m_pluginTimer.StartZero();
        return;
      }
    }

    // get the current locations area code
    CStdString strSetting;
    strSetting.Format("weather.areacode%i", m_iCurWeather + 1);
    const CStdString &areacode = g_weatherManager.GetAreaCode(g_guiSettings.GetString(strSetting));
    argv[1] = (char*)areacode.c_str();

    // call our plugin, passing the areacode
    g_pythonParser.evalFile(argv[0], argc, (const char**)argv);

    CLog::Log(LOGDEBUG, "%s - Weather plugin called: %s (%s)", __FUNCTION__, argv[0], argv[1]);
  }
}
