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
#include "utils/SystemInfo.h"
#include "XBVideoConfig.h"
#include "AlarmClock.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "Autorun.h"
#include "Builtins.h"
#include "input/ButtonTranslator.h"
#include "FileItem.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogKeyboard.h"
#include "music/dialogs/GUIDialogMusicScan.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogYesNo.h"
#include "video/dialogs/GUIDialogVideoScan.h"
#include "GUIUserMessages.h"
#include "windows/GUIWindowLoginScreen.h"
#include "video/windows/GUIWindowVideoBase.h"
#include "music/LastFmManager.h"
#include "LCD.h"
#include "log.h"
#include "storage/MediaManager.h"
#include "RssReader.h"
#include "PartyModeManager.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "Util.h"
#include "video/VideoDatabase.h"

#include "FileSystem/PluginDirectory.h"
#include "FileSystem/RarManager.h"
#include "FileSystem/VideoDatabaseDirectory.h"
#include "FileSystem/ZipManager.h"

#include "utils/URIUtils.h"
#include "xbox/xbeheader.h"
#include "xbox/network.h"
#include "lib/libGoAhead/XBMChttp.h"
#ifdef HAS_XBOX_HARDWARE
#include "utils/FanController.h"
#endif

#include "GUIWindowManager.h"
#include "LocalizeStrings.h"
#include "system.h"

#ifdef HAS_PYTHON
#include "lib/libPython/XBPython.h"
#endif

#ifdef HAS_WEB_SERVER
#include "lib/libGoAhead/XBMChttp.h"
#include "lib/libGoAhead/WebServer.h"
#endif

#include <vector>
#include "settings/AdvancedSettings.h"

using namespace std;
using namespace XFILE;
using namespace MEDIA_DETECT;

typedef struct
{
  const char *command;
  bool needsParameters;
  const char *description;
} BUILT_IN;

const BUILT_IN commands[] = {
  { "Help",                       false,  "This help message" },
  { "Reboot",                     false,  "Reboot the xbox (power cycle)" },
  { "Restart",                    false,  "Restart the xbox (power cycle)" },
  { "ShutDown",                   false,  "Shutdown the xbox" },
  { "Dashboard",                  false,  "Run your dashboard" },
  { "Powerdown",                  false,  "Powerdown system" },
  { "Quit",                       false,  "Quit XBMC" },
  { "Hibernate",                  false,  "Hibernates the system" },
  { "Suspend",                    false,  "Suspends the system" },
  { "RestartApp",                 false,  "Restart XBMC" },
  { "Credits",                    false,  "Run XBMCs Credits" },
  { "Reset",                      false,  "Reset the xbox (warm reboot)" },
  { "Mastermode",                 false,  "Control master mode" },
  { "ActivateWindow",             true,   "Activate the specified window" },
  { "ReplaceWindow",              true,   "Replaces the current window with the new one" },
  { "TakeScreenshot",             false,  "Takes a Screenshot" },
  { "RunScript",                  true,   "Run the specified script" },
  { "RunXBE",                     true,   "Run the specified executeable" },
  { "RunPlugin",                  true,   "Run the specified plugin" },
  { "Extract",                    true,   "Extracts the specified archive" },
  { "PlayMedia",                  true,   "Play the specified media file (or playlist)" },
  { "SlideShow",                  true,   "Run a slideshow from the specified directory" },
  { "RecursiveSlideShow",         true,   "Run a slideshow from the specified directory, including all subdirs" },
  { "ReloadSkin",                 false,  "Reload XBMC's skin" },
  { "RefreshRSS",                 false,  "Reload RSS feeds from RSSFeeds.xml"},
  { "PlayerControl",              true,   "Control the music or video player" },
  { "Playlist.PlayOffset",        true,   "Start playing from a particular offset in the playlist" },
  { "Playlist.Clear",             false,  "Clear the current playlist" },
  { "EjectTray",                  false,  "Close or open the DVD tray" },
  { "AlarmClock",                 true,   "Prompt for a length of time and start an alarm clock" },
  { "CancelAlarm",                true,   "Cancels an alarm" },
  { "Action",                     true,   "Executes an action for the active window (same as in keymap)" },
  { "Notification",               true,   "Shows a notification on screen, specify header, then message, and optionally time in milliseconds and a icon." },
  { "PlayDVD",                    false,  "Plays the inserted CD or DVD media from the DVD-ROM Drive!" },
  { "Skin.ToggleSetting",         true,   "Toggles a skin setting on or off" },
  { "Skin.SetString",             true,   "Prompts and sets skin string" },
  { "Skin.SetNumeric",            true,   "Prompts and sets numeric input" },
  { "Skin.SetPath",               true,   "Prompts and sets a skin path" },
  { "Skin.Theme",                 true,   "Control skin theme" },
  { "Skin.SetImage",              true,   "Prompts and sets a skin image" },
  { "Skin.SetLargeImage",         true,   "Prompts and sets a large skin images" },
  { "Skin.SetFile",               true,   "Prompts and sets a file" },
  { "Skin.SetBool",               true,   "Sets a skin setting on" },
  { "Skin.Reset",                 true,   "Resets a skin setting to default" },
  { "Skin.ResetSettings",         false,  "Resets all skin settings" },
  { "Mute",                       false,  "Mute the player" },
  { "SetVolume",                  true,   "Set the current volume" },
  { "Dialog.Close",               true,   "Close a dialog" },
  { "System.LogOff",              false,  "Log off current user" },
  { "System.PWMControl",          true,   "Control PWM RGB LEDs" },
  { "Resolution",                 true,   "Change XBMC's Resolution" },
  { "SetFocus",                   true,   "Change current focus to a different control id" }, 
  { "BackupSystemInfo",           false,  "Backup System Informations to local hdd" },
  { "UpdateLibrary",              true,   "Update the selected library (music or video)" },
  { "CleanLibrary",               true,   "Clean the video library" },
  { "ExportLibrary",              true,   "Export the video/music library" },
  { "PageDown",                   true,   "Send a page down event to the pagecontrol with given id" },
  { "PageUp",                     true,   "Send a page up event to the pagecontrol with given id" },
  { "LastFM.Love",                false,  "Add the current playing last.fm radio track to the last.fm loved tracks" },
  { "LastFM.Ban",                 false,  "Ban the current playing last.fm radio track" },
  { "Container.Refresh",          false,  "Refresh current listing" },
  { "Container.Update",           false,  "Update current listing. Send Container.Update(path,replace) to reset the path history" },
  { "Container.NextViewMode",     false,  "Move to the next view type (and refresh the listing)" },
  { "Container.PreviousViewMode", false,  "Move to the previous view type (and refresh the listing)" },
  { "Container.SetViewMode",      true,   "Move to the view with the given id" },
  { "Container.NextSortMethod",   false,  "Change to the next sort method" },
  { "Container.PreviousSortMethod",false, "Change to the previous sort method" },
  { "Container.SetSortMethod",    true,   "Change to the specified sort method" },
  { "Container.SortDirection",    false,  "Toggle the sort direction" },
  { "Control.Move",               true,   "Tells the specified control to 'move' to another entry specified by offset" },
  { "Control.SetFocus",           true,   "Change current focus to a different control id" },
  { "Control.Message",            true,   "Send a given message to a control within a given window" },
  { "SendClick",                  true,   "Send a click message from the given control to the given window" },
  { "LoadProfile",                true,   "Load the specified profile (note; if locks are active it won't work)" },
  { "SetProperty",                true,   "Sets a window property for the current focused window/dialog (key,value)" },
  { "ClearProperty",              true,   "Clears a window property for the current focused window/dialog (key,value)" },
  { "PlayWith",                   true,   "Play the selected item with the specified core" },
  { "WakeOnLan",                  true,   "Sends the wake-up packet to the broadcast address for the specified MAC address" },
  { "toggledebug",                false,  "Enables/disables debug mode" },
};

bool CBuiltins::HasCommand(const CStdString& execString)
{
  CStdString function;
  vector<CStdString> parameters;
  CUtil::SplitExecFunction(execString, function, parameters);
  for (unsigned int i = 0; i < sizeof(commands)/sizeof(BUILT_IN); i++)
  {
    if (function.CompareNoCase(commands[i].command) == 0 && (!commands[i].needsParameters || parameters.size()))
      return true;
  }
  return false;
}

void CBuiltins::GetHelp(CStdString &help)
{
  help.Empty();
  for (unsigned int i = 0; i < sizeof(commands)/sizeof(BUILT_IN); i++)
  {
    help += commands[i].command;
    help += "\t";
    help += commands[i].description;
    help += "\n";
  }
}

int CBuiltins::Execute(const CStdString& execString)
{
  // Get the text after the "XBMC."
  CStdString execute;
  vector<CStdString> params;
  CUtil::SplitExecFunction(execString, execute, params);
  execute.ToLower();
  CStdString parameter = params.size() ? params[0] : "";
  CStdString strParameterCaseIntact = parameter;
  
  if (execute.Equals("reboot") || execute.Equals("restart"))  //Will reboot the xbox, aka cold reboot
  {
    g_application.getApplicationMessenger().Restart();
  }
  else if (execute.Equals("shutdown"))
  {
    g_application.getApplicationMessenger().Shutdown();
  }
  else if (execute.Equals("dashboard"))
  {
    if (g_guiSettings.GetBool("myprograms.usedashpath"))
      CUtil::RunXBE(g_guiSettings.GetString("myprograms.dashboard").c_str());
    else
      CUtil::BootToDash();
  }
  else if (execute.Equals("powerdown"))
  {
    g_application.getApplicationMessenger().Powerdown();
  }
  else if (execute.Equals("restartapp"))
  {
    g_application.getApplicationMessenger().RestartApp();
  }
  else if (execute.Equals("hibernate"))
  {
    g_application.getApplicationMessenger().Hibernate();
  }
  else if (execute.Equals("suspend"))
  {
    g_application.getApplicationMessenger().Suspend();
  }
  else if (execute.Equals("quit"))
  {
    g_application.getApplicationMessenger().Quit();
  }
  else if (execute.Equals("loadprofile") && g_settings.GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE)
  {
    int index = g_settings.GetProfileIndex(parameter);
    if (index >= 0)
      CGUIWindowLoginScreen::LoadProfile(index);
  }
  else if (execute.Equals("mastermode"))
  {
    if (g_passwordManager.bMasterUser)
    {
      g_passwordManager.bMasterUser = false;
      g_passwordManager.LockSources(true);
      g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(20052),g_localizeStrings.Get(20053));
    }
    else if (g_passwordManager.IsMasterLockUnlocked(true))
    {
      g_passwordManager.LockSources(false);
      g_passwordManager.bMasterUser = true;
      g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(20052),g_localizeStrings.Get(20054));
    }

    CUtil::DeleteVideoDatabaseDirectoryCache();
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
    g_windowManager.SendMessage(msg);
  }
  else if (execute.Equals("takescreenshot"))
  {
    CUtil::TakeScreenshot();
  }
  else if (execute.Equals("credits"))
  {
#ifdef HAS_CREDITS
    RunCredits();
#endif
  }
  else if (execute.Equals("reset")) //Will reset the xbox, aka soft reset
  {
    g_application.getApplicationMessenger().Reset();
  }
  else if (execute.Equals("activatewindow") || execute.Equals("replacewindow"))
  {
    // get the parameters
    CStdString strWindow;
    CStdString strPath;
    if (params.size())
    {
      strWindow = params[0];
      params.erase(params.begin());
    }

    // confirm the window destination is valid prior to switching
    int iWindow = CButtonTranslator::TranslateWindow(strWindow);
    if (iWindow != WINDOW_INVALID)
    {
      // disable the screensaver
      g_application.ResetScreenSaverWindow();
      g_windowManager.ActivateWindow(iWindow, params, !execute.Equals("activatewindow"));
    }
    else
    {
      CLog::Log(LOGERROR, "Activate/ReplaceWindow called with invalid destination window: %s", strWindow.c_str());
      return false;
    }
  }
  else if (execute.Equals("setfocus") || execute.Equals("control.setfocus") && params.size())
  {
    int controlID = atol(params[0].c_str());
    int subItem = (params.size() > 1) ? atol(params[1].c_str())+1 : 0;
    CGUIMessage msg(GUI_MSG_SETFOCUS, g_windowManager.GetActiveWindow(), controlID, subItem);
    g_windowManager.SendMessage(msg);
  }
  else if (execute.Equals("runscript") && params.size())
  {
    unsigned int argc = params.size();
    char ** argv = new char*[argc];

    argv[0] = (char*)params[0].c_str();

    for(unsigned int i = 1; i < argc; i++)
      argv[i] = (char*)params[i].c_str();

    g_pythonParser.evalFile(params[0].c_str(), argc, (const char**)argv);
    delete [] argv;
  }
  else if (execute.Equals("resolution"))
  {
    RESOLUTION res = PAL_4x3;
    if (parameter.Equals("pal")) res = PAL_4x3;
    else if (parameter.Equals("pal16x9")) res = PAL_16x9;
    else if (parameter.Equals("ntsc")) res = NTSC_4x3;
    else if (parameter.Equals("ntsc16x9")) res = NTSC_16x9;
    else if (parameter.Equals("720p")) res = HDTV_720p;
    else if (parameter.Equals("1080i")) res = HDTV_1080i;
    if (g_videoConfig.IsValidResolution(res))
    {
      g_guiSettings.SetInt("videoscreen.resolution", res);
      //set the gui resolution, if newRes is AUTORES newRes will be set to the highest available resolution
      g_graphicsContext.SetVideoResolution(res, TRUE);
      //set our lookandfeelres to the resolution set in graphiccontext
      g_guiSettings.m_LookAndFeelResolution = res;
      g_application.ReloadSkin();
    }
  }
  else if (execute.Equals("extract") && params.size())
  {
    // Detects if file is zip or zip then extracts
    CStdString strDestDirect = "";
    if (params.size() < 2)
      URIUtils::GetDirectory(params[0],strDestDirect);
    else
      strDestDirect = params[1];

    URIUtils::AddSlashAtEnd(strDestDirect);

    if (URIUtils::IsZIP(params[0]))
      g_ZipManager.ExtractArchive(params[0],strDestDirect);
    else if (URIUtils::IsRAR(params[0]))
      g_RarManager.ExtractArchive(params[0],strDestDirect);
    else
      CLog::Log(LOGERROR, "CUtil::ExecuteBuiltin: No archive given");
  }
  else if (execute.Equals("runxbe"))
  {
    // only useful if there is actually an XBE to execute
    if (params.size())
    {
      CFileItem item(params[0]);
      item.SetPath(params[0]);
      if (item.IsShortCut())
        CUtil::RunShortcut(params[0]);
      else if (item.IsXBE())
      {
        int iRegion;
        if (g_guiSettings.GetBool("myprograms.gameautoregion"))
        {
          CXBE xbe;
          iRegion = xbe.ExtractGameRegion(params[0]);
          if (iRegion < 1 || iRegion > 7)
            iRegion = 0;
          iRegion = xbe.FilterRegion(iRegion);
        }
        else
          iRegion = 0;

        CUtil::RunXBE(params[0].c_str(),NULL,F_VIDEO(iRegion));
      }
    }
    else
    {
      CLog::Log(LOGERROR, "CBuiltins::Execute, runxbe called with no arguments.");
    }
  }
  else if (execute.Equals("runplugin"))
  {
    if (params.size())
    {
      CFileItem item(params[0]);
      if (!item.m_bIsFolder)
      {
        item.SetPath(params[0]);
        CPluginDirectory::RunScriptWithParams(item.GetPath());
      }
    }
    else
    {
      CLog::Log(LOGERROR, "CBuiltins::Execute, runplugin called with no arguments.");
    }
  }
  else if (execute.Equals("playmedia"))
  {
    if (!params.size())
    {
      CLog::Log(LOGERROR, "XBMC.PlayMedia called with empty parameter");
      return -3;
    }

    CFileItem item(params[0], false);
    if (URIUtils::HasSlashAtEnd(params[0]))
      item.m_bIsFolder = true;

    // restore to previous window if needed
    if( g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW ||
        g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
        g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION )
        g_windowManager.PreviousWindow();

    // reset screensaver
    g_application.ResetScreenSaver();
    g_application.ResetScreenSaverWindow();

    // ask if we need to check guisettings to resume
    bool askToResume = true;
    for (unsigned int i = 1 ; i < params.size() ; i++)
    {
      if (params[i].Equals("isdir"))
        item.m_bIsFolder = true;
      else if (params[i].Equals("1")) // set fullscreen or windowed
        g_settings.m_bStartVideoWindowed = true;
      else if (params[i].Equals("resume"))
      {
        // force the item to resume (if applicable) (see CApplication::PlayMedia)
        item.m_lStartOffset = STARTOFFSET_RESUME;
        askToResume = false;
      }
      else if (params[i].Equals("noresume"))
      {
        // force the item to start at the beginning (m_lStartOffset is initialized to 0)
        askToResume = false;
      }
      else if (params[i].Left(11).Equals("playoffset="))
        item.SetProperty("playlist_starting_track", atoi(params[i].Mid(11)) - 1);
    }

    if (!item.m_bIsFolder && item.IsPlugin())
      item.SetProperty("IsPlayable", true);

    if ( askToResume == true )
    {
      if ( CGUIWindowVideoBase::OnResumeShowMenu(item) == false )
        return false;
    }
    if (item.m_bIsFolder)
    {
      CFileItemList items;
      CDirectory::GetDirectory(item.GetPath(),items,g_settings.m_videoExtensions);
      g_playlistPlayer.Add(PLAYLIST_VIDEO,items);
      g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
      g_playlistPlayer.Play();
    }
    else
    {
      // play media
      if (!g_application.PlayMedia(item, item.IsAudio() ? PLAYLIST_MUSIC : PLAYLIST_VIDEO))
      {
        CLog::Log(LOGERROR, "XBMC.PlayMedia could not play media: %s", params[0].c_str());
        return false;
      }
    }
  }
  else if (execute.Equals("slideShow") || execute.Equals("recursiveslideShow"))
  {
    if (!params.size())
    {
      CLog::Log(LOGERROR, "XBMC.SlideShow called with empty parameter");
      return -2;
    }
    // leave RecursiveSlideShow command as-is
    unsigned int flags = 0;
    if (execute.Equals("RecursiveSlideShow"))
      flags |= 1;

    // SlideShow(dir[,recursive][,[not]random])
    else
    {
      if ((params.size() > 1 && params[1] == "recursive") || (params.size() > 2 && params[2] == "recursive"))
        flags |= 1;
      if ((params.size() > 1 && params[1] == "random") || (params.size() > 2 && params[2] == "random"))
        flags |= 2;
      if ((params.size() > 1 && params[1] == "notrandom") || (params.size() > 2 && params[2] == "notrandom"))
        flags |= 4;
    }

    CGUIMessage msg(GUI_MSG_START_SLIDESHOW, 0, 0, flags);
    msg.SetStringParam(params[0]);
    CGUIWindow *pWindow = g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    if (pWindow) pWindow->OnMessage(msg);
  }
  else if (execute.Equals("reloadskin"))
  {
    //  Reload the skin
    g_application.ReloadSkin();
  }
  else if (execute.Equals("refreshrss"))
  {
    g_rssManager.Stop();
    g_settings.LoadRSSFeeds();
    g_rssManager.Start();
  }
  else if (execute.Equals("playercontrol"))
  {
    g_application.ResetScreenSaver();
    g_application.ResetScreenSaverWindow();
    if (!params.size())
    {
      CLog::Log(LOGERROR, "XBMC.PlayerControl called with empty parameter");
      return -3;
    }
    if (parameter.Equals("play"))
    { // play/pause
      // either resume playing, or pause
      if (g_application.IsPlaying())
      {
        if (g_application.GetPlaySpeed() != 1)
          g_application.SetPlaySpeed(1);
        else
          g_application.m_pPlayer->Pause();
      }
    }
    else if (parameter.Equals("stop"))
    {
      g_application.StopPlaying();
    }
    else if (parameter.Equals("rewind") || parameter.Equals("forward"))
    {
      if (g_application.IsPlaying() && !g_application.m_pPlayer->IsPaused())
      {
        int iPlaySpeed = g_application.GetPlaySpeed();
        if (parameter.Equals("rewind") && iPlaySpeed == 1) // Enables Rewinding
          iPlaySpeed *= -2;
        else if (parameter.Equals("rewind") && iPlaySpeed > 1) //goes down a notch if you're FFing
          iPlaySpeed /= 2;
        else if (parameter.Equals("forward") && iPlaySpeed < 1) //goes up a notch if you're RWing
        {
          iPlaySpeed /= 2;
          if (iPlaySpeed == -1) iPlaySpeed = 1;
        }
        else
          iPlaySpeed *= 2;

        if (iPlaySpeed > 32 || iPlaySpeed < -32)
          iPlaySpeed = 1;

        g_application.SetPlaySpeed(iPlaySpeed);
      }
    }
    else if (parameter.Equals("next"))
    {
      g_application.OnAction(CAction(ACTION_NEXT_ITEM));
    }
    else if (parameter.Equals("previous"))
    {
      g_application.OnAction(CAction(ACTION_PREV_ITEM));
    }
    else if (parameter.Equals("bigskipbackward"))
    {
      if (g_application.IsPlaying())
        g_application.m_pPlayer->Seek(false, true);
    }
    else if (parameter.Equals("bigskipforward"))
    {
      if (g_application.IsPlaying())
        g_application.m_pPlayer->Seek(true, true);
    }
    else if (parameter.Equals("smallskipbackward"))
    {
      if (g_application.IsPlaying())
        g_application.m_pPlayer->Seek(false, false);
    }
    else if (parameter.Equals("smallskipforward"))
    {
      if (g_application.IsPlaying())
        g_application.m_pPlayer->Seek(true, false);
    }
    else if( parameter.Equals("showvideomenu") )
    {
      if( g_application.IsPlaying() && g_application.m_pPlayer )
       g_application.m_pPlayer->OnAction(CAction(ACTION_SHOW_VIDEOMENU));
    }
    else if( parameter.Equals("record") )
    {
      if( g_application.IsPlaying() && g_application.m_pPlayer && g_application.m_pPlayer->CanRecord())
      {
        if (m_pXbmcHttp && g_settings.m_HttpApiBroadcastLevel>=1)
          g_application.getApplicationMessenger().HttpApi(g_application.m_pPlayer->IsRecording()?"broadcastlevel; RecordStopping;1":"broadcastlevel; RecordStarting;1");
        g_application.m_pPlayer->Record(!g_application.m_pPlayer->IsRecording());
      }
    }
    else if (parameter.Left(9).Equals("partymode"))
    {
      CStdString strXspPath = "";
      //empty param=music, "music"=music, "video"=video, else xsp path
      PartyModeContext context = PARTYMODECONTEXT_MUSIC;
      if (parameter.size() > 9)
      {
        if (parameter.Mid(10).Equals("video)"))
          context = PARTYMODECONTEXT_VIDEO;
        else if (!parameter.Mid(10).Equals("music)"))
        {
          strXspPath = parameter.Mid(10).TrimRight(")");
          context = PARTYMODECONTEXT_UNKNOWN;
        }
      }
      if (g_partyModeManager.IsEnabled())
        g_partyModeManager.Disable();
      else
        g_partyModeManager.Enable(context, strXspPath);
    }
    else if (parameter.Equals("random")    ||
             parameter.Equals("randomoff") ||
             parameter.Equals("randomon"))
    {
      // get current playlist
      int iPlaylist = g_playlistPlayer.GetCurrentPlaylist();

      // reverse the current setting
      bool shuffled = g_playlistPlayer.IsShuffled(iPlaylist);
      if ((shuffled && parameter.Equals("randomon")) || (!shuffled && parameter.Equals("randomoff")))
        return 0;
      g_playlistPlayer.SetShuffle(iPlaylist, !shuffled);

      // save settings for now playing windows
      switch (iPlaylist)
      {
      case PLAYLIST_MUSIC:
        g_settings.m_bMyMusicPlaylistShuffle = g_playlistPlayer.IsShuffled(iPlaylist);
        g_settings.Save();
        break;
      case PLAYLIST_VIDEO:
        g_settings.m_bMyVideoPlaylistShuffle = g_playlistPlayer.IsShuffled(iPlaylist);
        g_settings.Save();
      }

      // send message
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_RANDOM, 0, 0, iPlaylist, g_playlistPlayer.IsShuffled(iPlaylist));
      g_windowManager.SendThreadMessage(msg);

    }
    else if (parameter.Left(6).Equals("repeat"))
    {
      // get current playlist
      int iPlaylist = g_playlistPlayer.GetCurrentPlaylist();
      PLAYLIST::REPEAT_STATE state = g_playlistPlayer.GetRepeat(iPlaylist);

      if (parameter.Equals("repeatall"))
        state = PLAYLIST::REPEAT_ALL;
      else if (parameter.Equals("repeatone"))
        state = PLAYLIST::REPEAT_ONE;
      else if (parameter.Equals("repeatoff"))
        state = PLAYLIST::REPEAT_NONE;
      else if (state == PLAYLIST::REPEAT_NONE)
        state = PLAYLIST::REPEAT_ALL;
      else if (state == PLAYLIST::REPEAT_ALL)
        state = PLAYLIST::REPEAT_ONE;
      else
        state = PLAYLIST::REPEAT_NONE;

      g_playlistPlayer.SetRepeat(iPlaylist, state);

      // save settings for now playing windows
      switch (iPlaylist)
      {
      case PLAYLIST_MUSIC:
        g_settings.m_bMyMusicPlaylistRepeat = (state == PLAYLIST::REPEAT_ALL);
        g_settings.Save();
        break;
      case PLAYLIST_VIDEO:
        g_settings.m_bMyVideoPlaylistRepeat = (state == PLAYLIST::REPEAT_ALL);
        g_settings.Save();
      }

      // send messages so now playing window can get updated
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_REPEAT, 0, 0, iPlaylist, (int)state);
      g_windowManager.SendThreadMessage(msg);
    }
  }
  else if (execute.Equals("playwith"))
  {
    g_application.m_eForcedNextPlayer = CPlayerCoreFactory::GetPlayerCore(parameter);
    g_application.OnAction(CAction(ACTION_PLAYER_PLAY));
  }
  else if (execute.Equals("mute"))
  {
    g_application.Mute();
  }
  else if (execute.Equals("setvolume"))
  {
    g_application.SetVolume(atoi(parameter.c_str()));
  }
  else if (execute.Equals("playlist.playoffset"))
  {
    // playlist.playoffset(offset) 
    // playlist.playoffset(music|video,offset) 
    CStdString strPos = parameter; 
    CStdString strPlaylist; 
    if (params.size() > 1) 
    { 
      // ignore any other parameters if present 
      strPlaylist = params[0]; 
      strPos = params[1]; 
 
      int iPlaylist = PLAYLIST_NONE; 
      if (strPlaylist.Equals("music")) 
        iPlaylist = PLAYLIST_MUSIC; 
      else if (strPlaylist.Equals("video")) 
        iPlaylist = PLAYLIST_VIDEO; 
 
      // unknown playlist 
      if (iPlaylist == PLAYLIST_NONE) 
      { 
        CLog::Log(LOGERROR,"Playlist.PlayOffset called with unknown playlist: %s", strPlaylist.c_str()); 
        return false; 
      } 
 
      // user wants to play the 'other' playlist 
      if (iPlaylist != g_playlistPlayer.GetCurrentPlaylist()) 
      { 
        g_application.StopPlaying(); 
        g_playlistPlayer.Reset(); 
        g_playlistPlayer.SetCurrentPlaylist(iPlaylist); 
      } 
    } 
    // play the desired offset 
    int pos = atol(strPos.c_str()); 
    // playlist is already playing 
    if (g_application.IsPlaying()) 
      g_playlistPlayer.PlayNext(pos); 
    // we start playing the 'other' playlist so we need to use play to initialize the player state 
    else 
      g_playlistPlayer.Play(pos);   
  }
  else if (execute.Equals("playlist.clear"))
  {
    g_playlistPlayer.Clear();
  }
  else if (execute.Equals("ejecttray"))
  {
    CIoSupport::ToggleTray();
  }
  else if( execute.Equals("alarmclock") && params.size() > 1 )
  {
    // format is alarmclock(name,command[,seconds,true]);
    float seconds = 0;
    if (params.size() > 2)
    {
      if (params[2].Find(':') == -1)
        seconds = static_cast<float>(atoi(params[2].c_str())*60);
      else
        seconds = (float)StringUtils::TimeStringToSeconds(params[2]);
    }
    else
    { // check if shutdown is specified in particular, and get the time for it
      CStdString strHeading;
      CStdString command;
      vector<CStdString> commandParams;
      CUtil::SplitExecFunction(params[1], command, commandParams);
      if (command.CompareNoCase("shutdown") == 0)
        strHeading = g_localizeStrings.Get(20145);
      else
        strHeading = g_localizeStrings.Get(13209);
      CStdString strTime;
      if( CGUIDialogNumeric::ShowAndGetNumber(strTime, strHeading) )
        seconds = static_cast<float>(atoi(strTime.c_str())*60);
      else
        return false;
    }
    bool silent = false;
    bool loop = false;
    for (unsigned int i = 3; i < params.size() ; i++)
    {
      // check "true" for backward comp
      if (params[i].CompareNoCase("true") == 0 || params[i].CompareNoCase("silent") == 0)
        silent = true;
      else if (params[i].CompareNoCase("loop") == 0)
        loop = true;
    }

    if( g_alarmClock.isRunning() )
      g_alarmClock.stop(params[0]);

    g_alarmClock.Start(params[0], seconds, params[1], silent, loop);
  }
  else if (execute.Equals("notification"))
  {
    if (params.size() < 2)
      return -1;
    if (params.size() == 4)
      g_application.m_guiDialogKaiToast.QueueNotification(params[3],params[0],params[1],atoi(params[2].c_str()));
    else if (params.size() == 3)
      g_application.m_guiDialogKaiToast.QueueNotification("",params[0],params[1],atoi(params[2].c_str()));
    else
      g_application.m_guiDialogKaiToast.QueueNotification(params[0],params[1]);
  }
  else if (execute.Equals("cancelalarm"))
  {
    g_alarmClock.stop(parameter);
  }
  else if (execute.Equals("playdvd"))
  {
    CAutorun::PlayDisc();
  }
  else if (execute.Equals("skin.togglesetting"))
  {
    int setting = g_settings.TranslateSkinBool(parameter);
    g_settings.SetSkinBool(setting, !g_settings.GetSkinBool(setting));
    g_settings.Save();
  }
  else if (execute.Equals("skin.setbool") && params.size())
  {
    if (params.size() > 1)
    {
      int string = g_settings.TranslateSkinBool(params[0]);
      g_settings.SetSkinBool(string, params[1].CompareNoCase("true") == 0);
      g_settings.Save();
      return 0;
    }
    // default is to set it to true
    int setting = g_settings.TranslateSkinBool(params[0]);
    g_settings.SetSkinBool(setting, true);
    g_settings.Save();
  }
  else if (execute.Equals("skin.reset"))
  {
    g_settings.ResetSkinSetting(parameter);
    g_settings.Save();
  }
  else if (execute.Equals("skin.resetsettings"))
  {
    g_settings.ResetSkinSettings();
    g_settings.Save();
  }
  else if (execute.Equals("skin.theme"))
  {
    // enumerate themes
    vector<CStdString> vecTheme;
    CUtil::GetSkinThemes(vecTheme);

    int iTheme = -1;

    CStdString strTmpTheme;
    // find current theme
    if (!g_guiSettings.GetString("lookandfeel.skintheme").Equals("skindefault"))
      for (unsigned int i=0;i<vecTheme.size();++i)
      {
        strTmpTheme = g_guiSettings.GetString("lookandfeel.skintheme");
        URIUtils::RemoveExtension(strTmpTheme);
        if (vecTheme[i].Equals(strTmpTheme))
        {
          iTheme=i;
          break;
        }
      }

    int iParam = atoi(parameter.c_str());
    if (iParam == 0 || iParam == 1)
      iTheme++;
    else if (iParam == -1)
      iTheme--;
    if (iTheme > (int)vecTheme.size()-1)
      iTheme = -1;
    if (iTheme < -1)
      iTheme = vecTheme.size()-1;

    CStdString strSkinTheme;
    if (iTheme==-1)
      g_guiSettings.SetString("lookandfeel.skintheme","skindefault");
    else
    {
      strSkinTheme.Format("%s.xpr",vecTheme[iTheme]);
      g_guiSettings.SetString("lookandfeel.skintheme",strSkinTheme);
    }
    // also set the default color theme
    CStdString colorTheme(URIUtils::ReplaceExtension(strSkinTheme, ".xml"));

    g_guiSettings.SetString("lookandfeel.skincolors", colorTheme);

    g_application.DelayLoadSkin();
  }
  else if (execute.Equals("skin.setstring") || execute.Equals("skin.setimage") || execute.Equals("skin.setfile") ||
           execute.Equals("skin.setpath") || execute.Equals("skin.setnumeric") || execute.Equals("skin.setlargeimage"))
  {
    // break the parameter up if necessary
    int string = 0;
    if (params.size() > 1)
    {
      string = g_settings.TranslateSkinString(params[0]);
      if (execute.Equals("skin.setstring"))
      {
        g_settings.SetSkinString(string, params[1]);
        g_settings.Save();
        return 0;
      }
    }
    else
      string = g_settings.TranslateSkinString(params[0]);
    CStdString value = g_settings.GetSkinString(string);
    VECSOURCES localShares;
    g_mediaManager.GetLocalDrives(localShares);
    if (execute.Equals("skin.setstring"))
    {
      if (CGUIDialogKeyboard::ShowAndGetInput(value, g_localizeStrings.Get(1029), true))
        g_settings.SetSkinString(string, value);
    }
    else if (execute.Equals("skin.setnumeric"))
    {
      if (CGUIDialogNumeric::ShowAndGetNumber(value, g_localizeStrings.Get(611)))
        g_settings.SetSkinString(string, value);
    }
    else if (execute.Equals("skin.setimage"))
    {
      if (CGUIDialogFileBrowser::ShowAndGetImage(localShares, g_localizeStrings.Get(1030), value))
        g_settings.SetSkinString(string, value);
    }
    else if (execute.Equals("skin.setlargeimage"))
    {
      VECSOURCES *shares = g_settings.GetSourcesFromType("pictures");
      if (!shares) shares = &localShares;
      if (CGUIDialogFileBrowser::ShowAndGetImage(*shares, g_localizeStrings.Get(1030), value))
        g_settings.SetSkinString(string, value);
    }
    else if (execute.Equals("skin.setfile"))
    {
      CStdString strMask = (params.size() > 1) ? params[1] : "";
    
      if (params.size() > 2)
      {
        value = params[2];
        URIUtils::AddSlashAtEnd(value);
        bool bIsSource;
        if (CUtil::GetMatchingSource(value,localShares,bIsSource) < 0) // path is outside shares - add it as a separate one
        {
          CMediaSource share;
          share.strName = g_localizeStrings.Get(13278);
          share.strPath = value;
          localShares.push_back(share);
        }
      }
      if (CGUIDialogFileBrowser::ShowAndGetFile(localShares, strMask, g_localizeStrings.Get(1033), value))
        g_settings.SetSkinString(string, value);
    }
    else // execute.Equals("skin.setpath"))
    {
      if (CGUIDialogFileBrowser::ShowAndGetDirectory(localShares, g_localizeStrings.Get(1031), value))
        g_settings.SetSkinString(string, value);
    }
    g_settings.Save();
  }
  else if (execute.Equals("dialog.close") && params.size())
  {
    bool bForce = false;
    if (params.size() > 1 && params[1].CompareNoCase("true") == 0)
      bForce = true;
    if (params[0].CompareNoCase("all") == 0)
    {
      g_windowManager.CloseDialogs(bForce);
    }
    else
    {
      DWORD id = CButtonTranslator::TranslateWindow(params[0]);
      CGUIWindow *window = (CGUIWindow *)g_windowManager.GetWindow(id);
      if (window && window->IsDialog())
        ((CGUIDialog *)window)->Close(bForce);
    }
  }
  else if (execute.Equals("system.logoff"))
  {
    if (g_windowManager.GetActiveWindow() == WINDOW_LOGIN_SCREEN || !g_settings.UsingLoginScreen())
      return -1;

    g_application.StopPlaying();
    CGUIDialogMusicScan *musicScan = (CGUIDialogMusicScan *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
    if (musicScan && musicScan->IsScanning())
      musicScan->StopScanning();

    g_application.getNetwork().NetworkMessage(CNetwork::SERVICES_DOWN,1);
    g_application.getNetwork().Deinitialize();
#ifdef HAS_XBOX_HARDWARE
    CLog::Log(LOGNOTICE, "stop fancontroller");
    CFanController::Instance()->Stop();
#endif
    g_settings.LoadMasterForLogin();
    g_passwordManager.bMasterUser = false;
    g_windowManager.ActivateWindow(WINDOW_LOGIN_SCREEN);
    g_application.getNetwork().SetupNetwork();
  }
  else if (execute.Left(18).Equals("system.pwmcontrol"))
  {
    CStdString strTemp ,strRgbA, strRgbB, strWhiteA, strWhiteB, strTran; 
    CStdStringArray arSplit; 
    int iTrTime = 0;
    StringUtils::SplitString(parameter,",", arSplit);

    if ((int)arSplit.size() >= 6)
    {
      strRgbA  = arSplit[0].c_str();
      strRgbB  = arSplit[1].c_str();
      strWhiteA= arSplit[2].c_str();
      strWhiteB= arSplit[3].c_str();
      strTran  = arSplit[4].c_str();
      iTrTime  = atoi(arSplit[5].c_str());
    }
    else if(parameter.size() > 6)
    {
      strRgbA = strRgbB = parameter;
      strWhiteA = strWhiteB = "#000000";
      strTran = "none";
    }
    CUtil::PWMControl(strRgbA,strRgbB,strWhiteA,strWhiteB,strTran, iTrTime);
  }
  else if (execute.Equals("backupsysteminfo"))
  {
#ifdef HAS_XBOX_HARDWARE
    g_sysinfo.WriteTXTInfoFile();
    g_sysinfo.CreateBiosBackup();
    g_sysinfo.CreateEEPROMBackup();
#endif
  }
  else if (execute.Equals("pagedown"))
  {
    int id = atoi(parameter.c_str());
    CGUIMessage message(GUI_MSG_PAGE_DOWN, g_windowManager.GetFocusedWindow(), id);
    g_windowManager.SendMessage(message);
  }
  else if (execute.Equals("pageup"))
  {
    int id = atoi(parameter.c_str());
    CGUIMessage message(GUI_MSG_PAGE_UP, g_windowManager.GetFocusedWindow(), id);
    g_windowManager.SendMessage(message);
  }
  else if (execute.Equals("updatelibrary") && params.size())
  {
    if (params[0].Equals("music"))
    {
      CGUIDialogMusicScan *scanner = (CGUIDialogMusicScan *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
      if (scanner)
      {
        if (scanner->IsScanning())
          scanner->StopScanning();
        else
          scanner->StartScanning("");
      }
    }
    if (params[0].Equals("video"))
    {
      CGUIDialogVideoScan *scanner = (CGUIDialogVideoScan *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
      SScraperInfo info;
      VIDEO::SScanSettings settings;
      if (scanner)
      {
        if (scanner->IsScanning())
          scanner->StopScanning();
        else
          CGUIWindowVideoBase::OnScan(params.size() > 1 ? params[1] : "",info,settings);
      }
    }
  }
  else if (execute.Equals("cleanlibrary"))
  {
    CGUIDialogVideoScan *scanner = (CGUIDialogVideoScan *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
    if (scanner)
    {
      if (!scanner->IsScanning())
      {
         CVideoDatabase videodatabase;
         videodatabase.Open();
         videodatabase.CleanDatabase();
         videodatabase.Close();
      }
      else
        CLog::Log(LOGERROR, "XBMC.CleanLibrary is not possible while scanning for media info");
    }
  }
  else if (execute.Equals("exportlibrary"))
  {
    int iHeading = 647;
    if (params[0].Equals("music"))
      iHeading = 20196;
    CStdString path;
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    bool singleFile;
    bool thumbs=false;
    bool actorThumbs=false;
    bool overwrite=false;
    bool cancelled=false;

    if (params.size() > 1)
      singleFile = params[1].Equals("true");
    else
      singleFile = CGUIDialogYesNo::ShowAndGetInput(iHeading,20426,20427,-1,20428,20429,cancelled);

    if (cancelled)
        return -1;

    if (singleFile)
    {
      if (params.size() > 2)
        thumbs = params[2].Equals("true");
      else
        thumbs = CGUIDialogYesNo::ShowAndGetInput(iHeading,20430,-1,-1,cancelled);
    }

    if (cancelled)
      return -1;

    if (thumbs && params[0].Equals("video"))
    {
      if (params.size() > 4)
        actorThumbs = params[4].Equals("true");
      else
        actorThumbs = CGUIDialogYesNo::ShowAndGetInput(iHeading,20436,-1,-1,cancelled);
    }

    if (cancelled)
      return -1;

    if (singleFile)
    {
      if (params.size() > 3)
        overwrite = params[3].Equals("true");
      else
        overwrite = CGUIDialogYesNo::ShowAndGetInput(iHeading,20431,-1,-1,cancelled);
    }

    if (cancelled)
      return -1;

    if (params.size() > 2)
      path=params[2];
    if (singleFile || !path.IsEmpty() ||
        CGUIDialogFileBrowser::ShowAndGetDirectory(shares,
          g_localizeStrings.Get(661), path, true))
    {
      if (params[0].Equals("video"))
      {
        CVideoDatabase videodatabase;
        videodatabase.Open();
        videodatabase.ExportToXML(path, singleFile, thumbs, actorThumbs, overwrite);
        videodatabase.Close();
      }
      else
      {
        if (URIUtils::HasSlashAtEnd(path))
          URIUtils::AddFileToFolder(path, "musicdb.xml", path);
        CMusicDatabase musicdatabase;
        musicdatabase.Open();
        musicdatabase.ExportToXML(path, singleFile, thumbs, overwrite);
        musicdatabase.Close();
      }
    }
  }
  else if (execute.Equals("lastfm.love"))
  {
    CLastFmManager::GetInstance()->Love(parameter.Equals("false") ? false : true);
  }
  else if (execute.Equals("lastfm.ban"))
  {
    CLastFmManager::GetInstance()->Ban(parameter.Equals("false") ? false : true);
  }
  else if (execute.Equals("control.move") && params.size() > 1)
  {
    CGUIMessage message(GUI_MSG_MOVE_OFFSET, g_windowManager.GetFocusedWindow(), atoi(params[0].c_str()), atoi(params[1].c_str()));
    g_windowManager.SendMessage(message);
  }
  else if (execute.Equals("container.refresh"))
  { // NOTE: These messages require a media window, thus they're sent to the current activewindow.
    //       This shouldn't stop a dialog intercepting it though.
    CGUIMessage message(GUI_MSG_NOTIFY_ALL, g_windowManager.GetActiveWindow(), 0, GUI_MSG_UPDATE, 1); // 1 to reset the history
    message.SetStringParam(parameter);
    g_windowManager.SendMessage(message);
  }
  else if (execute.Equals("container.update") && params.size())
  {
    CGUIMessage message(GUI_MSG_NOTIFY_ALL, g_windowManager.GetActiveWindow(), 0, GUI_MSG_UPDATE, 0);
    message.SetStringParam(params[0]);
    if (params.size() > 1 && params[1].CompareNoCase("replace") == 0)
      message.SetParam2(1); // reset the history
    g_windowManager.SendMessage(message);
  }
  else if (execute.Equals("container.nextviewmode"))
  {
    CGUIMessage message(GUI_MSG_CHANGE_VIEW_MODE, g_windowManager.GetActiveWindow(), 0, 0, 1);
    g_windowManager.SendMessage(message);
  }
  else if (execute.Equals("container.previousviewmode"))
  {
    CGUIMessage message(GUI_MSG_CHANGE_VIEW_MODE, g_windowManager.GetActiveWindow(), 0, 0, (DWORD)-1);
    g_windowManager.SendMessage(message);
  }
  else if (execute.Equals("container.setviewmode"))
  {
    CGUIMessage message(GUI_MSG_CHANGE_VIEW_MODE, g_windowManager.GetActiveWindow(), 0, atoi(parameter.c_str()));
    g_windowManager.SendMessage(message);
  }
  else if (execute.Equals("container.nextsortmethod"))
  {
    CGUIMessage message(GUI_MSG_CHANGE_SORT_METHOD, g_windowManager.GetActiveWindow(), 0, 0, 1);
    g_windowManager.SendMessage(message);
  }
  else if (execute.Equals("container.previoussortmethod"))
  {
    CGUIMessage message(GUI_MSG_CHANGE_SORT_METHOD, g_windowManager.GetActiveWindow(), 0, 0, (DWORD)-1);
    g_windowManager.SendMessage(message);
  }
  else if (execute.Equals("container.setsortmethod"))
  {
    CGUIMessage message(GUI_MSG_CHANGE_SORT_METHOD, g_windowManager.GetActiveWindow(), 0, atoi(parameter.c_str()));
    g_windowManager.SendMessage(message);
  }
  else if (execute.Equals("container.sortdirection"))
  {
    CGUIMessage message(GUI_MSG_CHANGE_SORT_DIRECTION, g_windowManager.GetActiveWindow(), 0, 0);
    g_windowManager.SendMessage(message);
  }
  else if (execute.Equals("control.message") && params.size() >= 2)
  {
    int controlID = atoi(params[0].c_str());
    int windowID = (params.size() == 3) ? CButtonTranslator::TranslateWindow(params[2]) : g_windowManager.GetActiveWindow();
    if (params[1] == "moveup")
      g_windowManager.SendMessage(GUI_MSG_MOVE_OFFSET, windowID, controlID, 1);
    else if (params[1] == "movedown")
      g_windowManager.SendMessage(GUI_MSG_MOVE_OFFSET, windowID, controlID, (DWORD)-1);
    else if (params[1] == "pageup")
      g_windowManager.SendMessage(GUI_MSG_PAGE_UP, windowID, controlID);
    else if (params[1] == "pagedown")
      g_windowManager.SendMessage(GUI_MSG_PAGE_DOWN, windowID, controlID);
    else if (params[1] == "click")
      g_windowManager.SendMessage(GUI_MSG_CLICKED, controlID, windowID);
  }
  else if (execute.Equals("sendclick") && params.size())
  {
    if (params.size() == 2)
    {
      // have a window - convert it
      int windowID = CButtonTranslator::TranslateWindow(params[0]);
      CGUIMessage message(GUI_MSG_CLICKED, atoi(params[1].c_str()), windowID);
      g_windowManager.SendMessage(message);
    }
    else
    { // single param - assume you meant the active window
      CGUIMessage message(GUI_MSG_CLICKED, atoi(params[0].c_str()), g_windowManager.GetActiveWindow());
      g_windowManager.SendMessage(message);
    }
  }
  else if (execute.Equals("action") && params.size())
  {
    // try translating the action from our ButtonTranslator
    int actionID;
    if (CButtonTranslator::TranslateActionString(params[0].c_str(), actionID))
    {
      if (params.size() == 2)
      { // have a window - convert it and send to it.
        int windowID = CButtonTranslator::TranslateWindow(params[1].c_str());
        CGUIWindow *window = g_windowManager.GetWindow(windowID);
        if (window)
          window->OnAction(CAction(actionID));
      }
      else // send to our app
        g_application.OnAction(CAction(actionID));
    }
  }
  else if (execute.Equals("setproperty") && params.size() == 2)
  {
    CGUIWindow *window = g_windowManager.GetWindow(g_windowManager.GetFocusedWindow());
    if (window)
      window->SetProperty(params[0],params[1]);
  }
  else if (execute.Equals("clearproperty") && params.size() == 2)
  {
    CGUIWindow *window = g_windowManager.GetWindow(g_windowManager.GetFocusedWindow());
    if (window)
      window->SetProperty(params[0],"");
  }
  else if (execute.Equals("wakeonlan"))
  {
    g_application.getNetwork().WakeOnLan((char*)params[0].c_str());
  }
  else if (execute.Equals("toggledebug"))
  {
    bool debug = g_guiSettings.GetBool("debug.showloginfo");
    g_guiSettings.SetBool("debug.showloginfo", !debug);
    g_advancedSettings.SetDebugMode(!debug);
  }
  else
    return -1;
  return 0;
}