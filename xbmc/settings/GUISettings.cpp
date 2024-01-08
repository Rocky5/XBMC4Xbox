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
#include "utils/log.h"
#include "settings/GUISettings.h"
#include "dialogs/GUIDialogFileBrowser.h"
#ifdef HAS_XBOX_HARDWARE
#include "utils/FanController.h"
#endif
#include "XBAudioConfig.h"
#include "XBVideoConfig.h"
#include "XBTimeZone.h"
#ifdef HAS_XFONT
#include <xfont.h>
#endif
#include "storage/MediaManager.h"
#include "FileSystem/SpecialProtocol.h"
#include "settings/AdvancedSettings.h"
#include "LocalizeStrings.h"
#include "GUIFont.h" // for FONT_STYLE_* definitions

using namespace std;

// String id's of the masks
#define MASK_MINS   14044
#define MASK_SECS   14045
#define MASK_MS    14046
#define MASK_PERCENT 14047
#define MASK_KBPS   14048
#define MASK_KB    14049
#define MASK_DB    14050

#define TEXT_OFF 351

class CGUISettings g_guiSettings;

#define DEFAULT_VISUALISATION "milkdrop.vis"
struct sortsettings
{
  bool operator()(const CSetting* pSetting1, const CSetting* pSetting2)
  {
    return pSetting1->GetOrder() < pSetting2->GetOrder();
  }
};

void CSettingBool::FromString(const CStdString &strValue)
{
  m_bData = (strValue == "true");
}

CStdString CSettingBool::ToString()
{
  return m_bData ? "true" : "false";
}

CSettingSeparator::CSettingSeparator(int iOrder, const char *strSetting)
    : CSetting(iOrder, strSetting, 0, SEPARATOR_CONTROL)
{
}

CSettingFloat::CSettingFloat(int iOrder, const char *strSetting, int iLabel, float fData, float fMin, float fStep, float fMax, int iControlType)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_fData = fData;
  m_fMin = fMin;
  m_fStep = fStep;
  m_fMax = fMax;
}

void CSettingFloat::FromString(const CStdString &strValue)
{
  SetData((float)atof(strValue.c_str()));
}

CStdString CSettingFloat::ToString()
{
  CStdString strValue;
  strValue.Format("%f", m_fData);
  return strValue;
}

CSettingInt::CSettingInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_iData = iData;
  m_iMin = iMin;
  m_iMax = iMax;
  m_iStep = iStep;
  m_iFormat = -1;
  m_iLabelMin = -1;
  if (strFormat)
    m_strFormat = strFormat;
  else
    m_strFormat = "%i";
}

CSettingInt::CSettingInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, int iFormat, int iLabelMin)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_iData = iData;
  m_iMin = iMin;
  m_iMax = iMax;
  m_iStep = iStep;
  m_iLabelMin = iLabelMin;
  m_iFormat = iFormat;
  if (m_iFormat < 0)
    m_strFormat = "%i";
}

void CSettingInt::FromString(const CStdString &strValue)
{
  SetData(atoi(strValue.c_str()));
}

CStdString CSettingInt::ToString()
{
  CStdString strValue;
  strValue.Format("%i", m_iData);
  return strValue;
}

void CSettingHex::FromString(const CStdString &strValue)
{
  int iHexValue;
  if (sscanf(strValue, "%x", (unsigned int *)&iHexValue))
    SetData(iHexValue);
}

CStdString CSettingHex::ToString()
{
  CStdString strValue;
  strValue.Format("%x", m_iData);
  return strValue;
}

CSettingString::CSettingString(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_strData = strData;
  m_bAllowEmpty = bAllowEmpty;
  m_iHeadingString = iHeadingString;
}

void CSettingString::FromString(const CStdString &strValue)
{
  m_strData = strValue;
}

CStdString CSettingString::ToString()
{
  return m_strData;
}

CSettingPath::CSettingPath(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
    : CSettingString(iOrder, strSetting, iLabel, strData, iControlType, bAllowEmpty, iHeadingString)
{
}

void CSettingsGroup::GetCategories(vecSettingsCategory &vecCategories)
{
  vecCategories.clear();
  for (unsigned int i = 0; i < m_vecCategories.size(); i++)
  {
    vecSettings settings;
    // check whether we actually have these settings available.
    g_guiSettings.GetSettingsGroup(m_vecCategories[i]->m_strCategory, settings);
    if (settings.size())
      vecCategories.push_back(m_vecCategories[i]);
  }
}

// Settings are case sensitive
CGUISettings::CGUISettings(void)
{
}

void CGUISettings::Initialize()
{
  ZeroMemory(&m_replayGain, sizeof(ReplayGainSettings));

  // Pictures settings
  AddGroup(0, 1);
  AddCategory(0, "pictures", 14081);
  AddBool(1, "pictures.usetags", 14082, true);
  AddBool(2,"pictures.generatethumbs",13360,true);
  AddBool(3, "pictures.useexifrotation", 20184, true);
  AddBool(4, "pictures.showvideos", 22022, false);
  AddInt(8, "pictures.displayresolution", 169, (int)AUTORES, (int)HDTV_1080i, 1, (int)AUTORES, SPIN_CONTROL_TEXT);

  AddCategory(0, "slideshow", 108);
  AddInt(1, "slideshow.staytime", 12378, 5, 1, 1, 100, SPIN_CONTROL_INT_PLUS, MASK_SECS);
  AddBool(2, "slideshow.displayeffects", 12379, true);
  AddBool(0, "slideshow.shuffle", 13319, false);

  // Programs settings
  AddGroup(1, 0);
  AddCategory(1, "myprograms", 16000);
  AddBool(1, "myprograms.autoffpatch", 29999, false);
  AddSeparator(2,"myprograms.sep1");
  AddBool(3, "myprograms.gameautoregion",511,false);
  AddInt(4, "myprograms.ntscmode", 16110, 0, 0, 1, 3, SPIN_CONTROL_TEXT);
  AddSeparator(5,"myprograms.sep2");
  AddString(6, "myprograms.trainerpath", 20003, "select folder", BUTTON_CONTROL_PATH_INPUT, false, 657);
  AddSeparator(7,"myprograms.sep3");
  AddBool(8, "myprograms.usedashpath", 13007, true);
  AddString(9, "myprograms.dashboard", 13006, "C:\\xboxdash.xbe", BUTTON_CONTROL_PATH_INPUT, false, 655);

  // My Weather settings
  AddGroup(2, 8);
  AddCategory(2, "weather", 16000);
  AddString(1, "weather.areacode1", 14019, "USNY0996 - New York, NY", BUTTON_CONTROL_STANDARD);
  AddString(2, "weather.areacode2", 14020, "UKXX0085 - London, United Kingdom", BUTTON_CONTROL_STANDARD);
  AddString(3, "weather.areacode3", 14021, "JAXX0085 - Tokyo, Japan", BUTTON_CONTROL_STANDARD);
  AddSeparator(4, "weather.sep1");
  AddString(5, "weather.plugin", 23000, "", SPIN_CONTROL_TEXT, true);
  AddString(6, "weather.pluginsettings", 23001, "", BUTTON_CONTROL_STANDARD, true);

  // My Music Settings
  AddGroup(3, 2);
  AddCategory(3,"musiclibrary",14022);
  AddBool(1, "musiclibrary.enabled", 421, true);
  AddBool(2, "musiclibrary.showcompilationartists", 13414, true);
  AddSeparator(3,"musiclibrary.sep1");
  AddBool(4,"musiclibrary.downloadinfo", 20192, false);
  AddString(6, "musiclibrary.scraper", 20194, "tadb.xml", SPIN_CONTROL_TEXT);
  AddString(7, "musiclibrary.scrapersettings", 21417, "", BUTTON_CONTROL_STANDARD);
  AddBool(8, "musiclibrary.updateonstartup", 22000, false);
  AddBool(0, "musiclibrary.backgroundupdate", 22001, false);
  AddSeparator(9,"musiclibrary.sep2");
  AddString(10, "musiclibrary.cleanup", 334, "", BUTTON_CONTROL_STANDARD);
  AddString(11, "musiclibrary.export", 20196, "", BUTTON_CONTROL_STANDARD);
  AddString(12, "musiclibrary.import", 20197, "", BUTTON_CONTROL_STANDARD);

  AddCategory(3, "musicplayer", 14086);
  AddBool(1, "musicplayer.autoplaynextitem", 489, true);
  AddBool(2, "musicplayer.queuebydefault", 14084, false);
  AddSeparator(3, "musicplayer.sep1");
  AddInt(4, "musicplayer.replaygaintype", 638, REPLAY_GAIN_ALBUM, REPLAY_GAIN_NONE, 1, REPLAY_GAIN_TRACK, SPIN_CONTROL_TEXT);
  AddInt(0, "musicplayer.replaygainpreamp", 641, 89, 77, 1, 101, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddInt(0, "musicplayer.replaygainnogainpreamp", 642, 89, 77, 1, 101, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddBool(0, "musicplayer.replaygainavoidclipping", 643, false);
  AddInt(5, "musicplayer.crossfade", 13314, 0, 0, 1, 15, SPIN_CONTROL_INT_PLUS, MASK_SECS, TEXT_OFF);
  AddBool(6, "musicplayer.crossfadealbumtracks", 13400, true);
  AddSeparator(7, "musicplayer.sep2");
  AddString(8, "musicplayer.visualisation", 250, DEFAULT_VISUALISATION, SPIN_CONTROL_TEXT);
  AddSeparator(9, "musicplayer.sep3");
  AddInt(10, "musicplayer.defaultplayer", 22003, PLAYER_PAPLAYER, PLAYER_MPLAYER, 1, PLAYER_PAPLAYER, SPIN_CONTROL_TEXT);
#ifdef _XBOX
  AddBool(11, "musicplayer.outputtoallspeakers", 252, false);
#endif

  AddCategory(3, "musicfiles", 14081);
  AddBool(1, "musicfiles.usetags", 258, false);
  AddString(2, "musicfiles.trackformat", 13307, "[%N. ]%A - %T", EDIT_CONTROL_INPUT, false, 16016);
  AddString(3, "musicfiles.trackformatright", 13387, "%D", EDIT_CONTROL_INPUT, false, 16016);
  // advanced per-view trackformats.
  AddString(0, "musicfiles.nowplayingtrackformat", 13307, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(0, "musicfiles.nowplayingtrackformatright", 13387, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(0, "musicfiles.librarytrackformat", 13307, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(0, "musicfiles.librarytrackformatright", 13387, "", EDIT_CONTROL_INPUT, false, 16016);
  AddBool(4, "musicfiles.findremotethumbs", 14059, true);

  AddCategory(3, "scrobbler", 15221);
  AddBool(1, "scrobbler.lastfmsubmit", 15201, false);
  AddBool(2, "scrobbler.lastfmsubmitradio", 15250, false);
  AddString(3, "scrobbler.lastfmusername", 15202, "", EDIT_CONTROL_INPUT, false, 15202);
  AddString(4, "scrobbler.lastfmpass", 15203, "", EDIT_CONTROL_MD5_INPUT, false, 15203);
  AddSeparator(5, "scrobbler.sep1");
  AddBool(6, "scrobbler.librefmsubmit", 15217, false);
  AddString(7, "scrobbler.librefmusername", 15218, "", EDIT_CONTROL_INPUT, false, 15218);
  AddString(8, "scrobbler.librefmpass", 15219, "", EDIT_CONTROL_MD5_INPUT, false, 15219);

  AddCategory(3, "audiocds", 620);
  AddBool(2, "audiocds.usecddb", 227, true);
  AddSeparator(3, "audiocds.sep1");
  AddPath(4,"audiocds.recordingpath",20000,"select writable folder",BUTTON_CONTROL_PATH_INPUT,false,657);
  AddString(5, "audiocds.trackpathformat", 13307, "%A - %B/[%N. ][%A - ]%T", EDIT_CONTROL_INPUT, false, 16016);
  AddInt(6, "audiocds.encoder", 621, CDDARIP_ENCODER_LAME, CDDARIP_ENCODER_LAME, 1, CDDARIP_ENCODER_FLAC, SPIN_CONTROL_TEXT);
  AddInt(7, "audiocds.quality", 622, CDDARIP_QUALITY_CBR, CDDARIP_QUALITY_CBR, 1, CDDARIP_QUALITY_EXTREME, SPIN_CONTROL_TEXT);
  AddInt(8, "audiocds.bitrate", 623, 192, 128, 32, 320, SPIN_CONTROL_INT_PLUS, MASK_KBPS);
  AddInt(9, "audiocds.compressionlevel", 665, 5, 0, 1, 8, SPIN_CONTROL_INT_PLUS);

  AddCategory(3, "karaoke", 13327);
  AddBool(1, "karaoke.enabled", 13323, false);
  AddBool(2, "karaoke.voiceenabled", 13361, false);
  AddInt(3, "karaoke.volume", 13376, 100, 0, 1, 100, SPIN_CONTROL_INT, MASK_PERCENT);
  AddString(4, "karaoke.port0voicemask", 13382, "None", SPIN_CONTROL_TEXT);
  AddString(5, "karaoke.port1voicemask", 13383, "None", SPIN_CONTROL_TEXT);
  AddString(6, "karaoke.port2voicemask", 13384, "None", SPIN_CONTROL_TEXT);
  AddString(7, "karaoke.port3voicemask", 13385, "None", SPIN_CONTROL_TEXT);

  // System settings
  AddGroup(4, 13000);
#ifdef HAS_XBOX_HARDWARE
  AddCategory(4, "system", 128);
  AddBool(3, "system.mceremote", 13601, false);
  AddInt(4, "system.shutdowntime", 357, 0, 0, 5, 120, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF);
  AddInt(5, "system.ledcolour", 13339, LED_COLOUR_NO_CHANGE, LED_COLOUR_NO_CHANGE, 1, LED_COLOUR_OFF, SPIN_CONTROL_TEXT);
  AddInt(6, "system.leddisableonplayback", 13345, LED_PLAYBACK_OFF, LED_PLAYBACK_OFF, 1, LED_PLAYBACK_VIDEO_MUSIC, SPIN_CONTROL_TEXT);
  AddBool(7, "system.ledenableonpaused", 20313, true);
  AddSeparator(8, "system.sep1");
  AddBool(9, "system.fanspeedcontrol", 13302, false);
  AddInt(10, "system.fanspeed", 13300, CFanController::Instance()->GetFanSpeed(), 5, 5, 50, SPIN_CONTROL_TEXT);
  AddSeparator(11, "system.sep2");
  AddBool(12, "system.autotemperature", 13301, false);
  AddInt(13, "system.targettemperature", 13299, 55, 40, 1, 68, SPIN_CONTROL_TEXT);
  AddInt(14, "system.minfanspeed", 13411, 1, 1, 1, 50, SPIN_CONTROL_TEXT);
#endif
  
  AddCategory(4, "videooutput", 21373);
  AddInt(1, "videooutput.aspect", 21374, VIDEO_NORMAL, VIDEO_NORMAL, 1, VIDEO_WIDESCREEN, SPIN_CONTROL_TEXT);
  AddBool(2,  "videooutput.hd480p", 21378, true);
  AddBool(3,  "videooutput.hd720p", 21379, true);
  AddBool(4,  "videooutput.hd1080i", 21380, false);

  AddCategory(4, "audiooutput", 772);
  AddInt(3, "audiooutput.mode", 337, AUDIO_ANALOG, AUDIO_ANALOG, 1, AUDIO_DIGITAL, SPIN_CONTROL_TEXT);
  AddBool(4, "audiooutput.ac3passthrough", 364, true);
  AddBool(5, "audiooutput.dtspassthrough", 254, true);
  AddBool(6, "audiooutput.aacpassthrough", 299, false);
#ifdef _XBOX
  AddBool(10, "audiooutput.downmixmultichannel", 548, true);
#endif

  AddCategory(4, "lcd", 448);
  AddInt(2, "lcd.type", 4501, LCD_TYPE_NONE, LCD_TYPE_NONE, 1, LCD_TYPE_VFD, SPIN_CONTROL_TEXT);
  AddInt(3, "lcd.modchip", 471, MODCHIP_SMARTXX, MODCHIP_SMARTXX, 1, MODCHIP_XECUTER3, SPIN_CONTROL_TEXT);
  AddInt(4, "lcd.backlight", 463, 80, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddInt(5, "lcd.contrast", 465, 100, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddSeparator(6, "lcd.sep1");
  AddInt(7, "lcd.disableonplayback", 20310, LED_PLAYBACK_OFF, LED_PLAYBACK_OFF, 1, LED_PLAYBACK_VIDEO_MUSIC, SPIN_CONTROL_TEXT);
  AddBool(8, "lcd.enableonpaused", 20312, true);

  AddCategory(4, "debug", 14092);
  AddBool(1, "debug.showloginfo", 20191, false);
  AddPath(2, "debug.screenshotpath",20004,"select writable folder",BUTTON_CONTROL_PATH_INPUT,false,657);

  AddCategory(4, "autorun", 447);
  AddBool(1, "autorun.dvd", 240, true);
  AddBool(2, "autorun.vcd", 241, true);
  AddBool(3, "autorun.cdda", 242, true);
  AddBool(4, "autorun.xbox", 243, true);
  AddBool(5, "autorun.video", 244, true);
  AddBool(6, "autorun.music", 245, true);
  AddBool(7, "autorun.pictures", 246, true);

  AddCategory(4, "harddisk", 440);
  AddInt(1, "harddisk.spindowntime", 229, 0, 0, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF); // Minutes
  AddInt(2, "harddisk.remoteplayspindown", 13001, 0, 0, 1, 3, SPIN_CONTROL_TEXT); // off, music, video, both
  AddInt(0, "harddisk.remoteplayspindownminduration", 13004, 20, 0, 1, 20, SPIN_CONTROL_INT_PLUS, MASK_MINS); // Minutes
  AddInt(0, "harddisk.remoteplayspindowndelay", 13003, 20, 5, 5, 300, SPIN_CONTROL_INT_PLUS, MASK_SECS); // seconds
  AddInt(3, "harddisk.aamlevel", 21386, AAM_FAST, AAM_FAST, 1, AAM_QUIET, SPIN_CONTROL_TEXT);
  AddInt(4, "harddisk.apmlevel", 21390, APM_HIPOWER, APM_HIPOWER, 1, APM_LOPOWER_STANDBY, SPIN_CONTROL_TEXT);

  AddCategory(4, "dvdplayercache", 483);
  AddInt(1, "dvdplayercache.video", 14096, 1024, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(2, "dvdplayercache.videotime", 14097, 8, 0, 1, 30, SPIN_CONTROL_INT_PLUS, MASK_SECS);
  AddSeparator(3, "dvdplayercache.sep1");
  AddInt(4, "dvdplayercache.audio", 14098, 384, 0, 128, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(5, "dvdplayercache.audiotime", 14099, 8, 0, 1, 30, SPIN_CONTROL_INT_PLUS, MASK_SECS);

  AddCategory(4, "cache", 439);
  AddInt(1, "cache.harddisk", 14025, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(2, "cache.sep1");
  AddInt(3, "cachevideo.dvdrom", 14026, 1024, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(4, "cachevideo.lan", 14027, 1024, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(5, "cachevideo.internet", 14028, 1024, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(6, "cache.sep2");
  AddInt(7, "cacheaudio.dvdrom", 14030, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(8, "cacheaudio.lan", 14031, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(9, "cacheaudio.internet", 14032, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(10, "cache.sep3");
  AddInt(11, "cachedvd.dvdrom", 14034, 512, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(12, "cachedvd.lan", 14035, 512, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(13, "cache.sep4");
  AddInt(14, "cacheunknown.internet", 14060, 1024, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);

  // !! Should be the last category, else disabling it will cause problems!
  AddCategory(4, "masterlock", 12360);
  AddString(1, "masterlock.lockcode"       , 20100, "-", BUTTON_CONTROL_STANDARD);
  AddBool(4, "masterlock.startuplock"      , 20076,false);
  AddBool(5, "masterlock.enableshutdown"   , 12362,false);  
  // hidden masterlock settings
  AddInt(0,"masterlock.maxretries", 12364, 3, 3, 1, 100, SPIN_CONTROL_TEXT);

  // video settings
  AddGroup(5, 3);
  AddCategory(5, "videolibrary", 14022);
  AddBool(2, "videolibrary.enabled", 421, true);
  AddBool(3, "videolibrary.showunwatchedplots", 20369, true);
  AddBool(4, "videolibrary.seasonthumbs", 20382, true);
  AddBool(5, "videolibrary.actorthumbs", 20402, false);
  AddInt(0, "videolibrary.flattentvshows", 20412, 1, 0, 1, 2, SPIN_CONTROL_TEXT);
  AddBool(7, "videolibrary.groupmoviesets", 20458, false);
  AddBool(8, "videolibrary.updateonstartup", 22000, false);
  AddBool(0, "videolibrary.backgroundupdate", 22001, false);
  AddSeparator(10, "videolibrary.sep3");
  AddString(11, "videolibrary.cleanup", 334, "", BUTTON_CONTROL_STANDARD);
  AddString(12, "videolibrary.export", 647, "", BUTTON_CONTROL_STANDARD);
  AddString(13, "videolibrary.import", 648, "", BUTTON_CONTROL_STANDARD);

  AddCategory(5, "videoplayer", 14086);
  AddInt(1, "videoplayer.resumeautomatically", 12017, RESUME_ASK, RESUME_NO, 1, RESUME_ASK, SPIN_CONTROL_TEXT);
  AddString(2, "videoplayer.calibrate", 214, "", BUTTON_CONTROL_STANDARD);
  AddSeparator(3, "videoplayer.sep1");
  AddInt(4, "videoplayer.rendermethod", 13354, RENDER_HQ_RGB_SHADER, RENDER_LQ_RGB_SHADER, 1, RENDER_HQ_RGB_SHADERV2, SPIN_CONTROL_TEXT);
  AddInt(5, "videoplayer.displayresolution", 169, (int)AUTORES, (int)HDTV_1080i, 1, (int)AUTORES, SPIN_CONTROL_TEXT);
  AddInt(6, "videoplayer.framerateconversions", 336, FRAME_RATE_LEAVE_AS_IS, FRAME_RATE_LEAVE_AS_IS, 1, FRAME_RATE_USE_PAL60, SPIN_CONTROL_TEXT);
  AddInt(7, "videoplayer.flicker", 13100, 1, 0, 1, 5, SPIN_CONTROL_INT_PLUS, -1, TEXT_OFF);
  AddBool(8, "videoplayer.soften", 215, false);
  AddFloat(9, "videoplayer.errorinaspect", 22021, 3.0f, 0.0f, 1.0f, 20.0f);
  AddSeparator(11, "videoplayer.sep2");
  AddInt(12, "videoplayer.defaultplayer", 22003, PLAYER_DVDPLAYER, PLAYER_MPLAYER, 1, PLAYER_DVDPLAYER, SPIN_CONTROL_TEXT);
  AddBool(13, "videoplayer.allcodecs", 22025, false);
  AddBool(14, "videoplayer.fast", 22026, false);
  AddInt(15, "videoplayer.skiploopfilter", 14100, VS_SKIPLOOP_NONREF, VS_SKIPLOOP_DEFAULT, 1, VS_SKIPLOOP_ALL, SPIN_CONTROL_TEXT);

  AddCategory(5, "myvideos", 14081);
  AddBool(0, "myvideos.treatstackasfile", 20051, true);
  AddBool(0, "myvideos.extractflags",20433, false);
  AddBool(3, "myvideos.cleanstrings", 20418, false);
  AddBool(0, "myvideos.extractthumb",20433, false);

  AddCategory(5, "subtitles", 287);
  AddString(1, "subtitles.font", 288, "Arial.ttf", SPIN_CONTROL_TEXT);
  AddInt(2, "subtitles.height", 289, 28, 16, 2, 74, SPIN_CONTROL_TEXT); // use text as there is a disk based lookup needed
  AddInt(3, "subtitles.style", 736, FONT_STYLE_BOLD, FONT_STYLE_NORMAL, 1, FONT_STYLE_BOLD | FONT_STYLE_ITALICS, SPIN_CONTROL_TEXT);
  AddInt(4, "subtitles.color", 737, SUBTITLE_COLOR_START + 1, SUBTITLE_COLOR_START, 1, SUBTITLE_COLOR_END, SPIN_CONTROL_TEXT);
  AddString(5, "subtitles.charset", 735, "DEFAULT", SPIN_CONTROL_TEXT);
  AddSeparator(7, "subtitles.sep1");
  AddPath(11, "subtitles.custompath", 21366, "", BUTTON_CONTROL_PATH_INPUT, false, 657);
  AddSeparator(12,"subtitles.sep2");
  AddBool(13, "subtitles.searchrars", 13249, false);

  AddCategory(5, "dvds", 14087);
  AddInt(2, "dvds.playerregion", 21372, 0, 0, 1, 8, SPIN_CONTROL_INT_PLUS, -1, TEXT_OFF);
  AddBool(3, "dvds.automenu", 21882, false);
  AddBool(4, "dvds.useexternaldvdplayer", 20001, false);
  AddString(5, "dvds.externaldvdplayer", 20002, "",  BUTTON_CONTROL_PATH_INPUT, true, 655);

  // Don't add the category - makes them hidden in the GUI
  //AddCategory(5, "postprocessing", 14041);
  AddBool(2, "postprocessing.enable", 286, false);
  AddBool(3, "postprocessing.auto", 307, true); // only has effect if PostProcessing.Enable is on.
  AddBool(4, "postprocessing.verticaldeblocking", 308, false);
  AddInt(5, "postprocessing.verticaldeblocklevel", 308, 0, 0, 1, 100, SPIN_CONTROL_INT);
  AddBool(6, "postprocessing.horizontaldeblocking", 309, false);
  AddInt(7, "postprocessing.horizontaldeblocklevel", 309, 0, 0, 1, 100, SPIN_CONTROL_INT);
  AddBool(8, "postprocessing.autobrightnesscontrastlevels", 310, false);
  AddBool(9, "postprocessing.dering", 311, false);

  AddCategory(5, "scrapers", 21412);
  AddString(1, "scrapers.moviedefault", 21413, "tmdb.xml", SPIN_CONTROL_TEXT);
  AddString(2, "scrapers.tvshowdefault", 21414, "tvdb.xml", SPIN_CONTROL_TEXT);
  AddString(3, "scrapers.musicvideodefault", 21415, "mtv.xml", SPIN_CONTROL_TEXT);
  AddSeparator(4,"scrapers.sep2");
  AddBool(5, "scrapers.langfallback", 21416, false);

  // network settings
  AddGroup(6, 705);

  AddCategory(6, "services", 14036);
  AddBool(1, "services.upnpserver", 21360, false);
  AddBool(2, "services.upnprenderer", 21881, false);
  AddSeparator(3,"services.sep3");

  AddBool(4,  "services.webserver",        263, false);
  AddString(5,"services.webserverport",    730, "80", EDIT_CONTROL_NUMBER_INPUT, false, 730);
  AddString(6,"services.webserverusername",1048, "xbmc", EDIT_CONTROL_INPUT);
  AddString(7,"services.webserverpassword",733, "", EDIT_CONTROL_HIDDEN_INPUT, true, 733);

#ifdef HAS_EVENT_SERVER
  AddSeparator(8,"services.sep1");
  AddBool(9,  "services.esenabled",         794, true);
  AddString(0,"services.esport",            792, "9777", EDIT_CONTROL_NUMBER_INPUT, false, 792);
  AddInt(0,   "services.esportrange",       793, 10, 1, 1, 100, SPIN_CONTROL_INT);
  AddInt(0,   "services.esmaxclients",      797, 20, 1, 1, 100, SPIN_CONTROL_INT);
  AddInt(0,   "services.esinitialdelay",    795, 750, 5, 5, 10000, SPIN_CONTROL_INT);
  AddInt(0,   "services.escontinuousdelay", 796, 25, 5, 5, 10000, SPIN_CONTROL_INT);
#endif

  AddSeparator(11, "services.sep2");
  AddBool(12,  "services.ftpserver",        167, true);
  AddString(13,"services.ftpserveruser",    1245, "xbox", SPIN_CONTROL_TEXT);
  AddString(14,"services.ftpserverpassword",1246, "xbox", EDIT_CONTROL_HIDDEN_INPUT, true, 1246);
  AddBool(15,  "services.ftpautofatx",      771, true);

  AddCategory(6,"autodetect",           1250  );
  AddBool(1,    "autodetect.onoff",     1251, false);
  AddBool(2,    "autodetect.popupinfo", 1254, true);
  AddString(3,  "autodetect.nickname",  1252, "XBMC-NickName",EDIT_CONTROL_INPUT, false, 1252);
  AddSeparator(4, "autodetect.sep1");
  AddBool(5,    "autodetect.senduserpw",1255, true); // can be in advanced.xml! default:true

  AddCategory(6, "smb", 1200);
  AddString(3, "smb.winsserver",  1207,   "",  EDIT_CONTROL_IP_INPUT);
  AddString(4, "smb.workgroup",   1202,   "WORKGROUP", EDIT_CONTROL_INPUT, false, 1202);

  AddCategory(6, "network", 705);
  AddInt(1, "network.assignment", 715, NETWORK_DHCP, NETWORK_DASH, 1, NETWORK_STATIC, SPIN_CONTROL_TEXT);
  AddString(2, "network.ipaddress", 719, "0.0.0.0", EDIT_CONTROL_IP_INPUT);
  AddString(3, "network.subnet", 720, "255.255.255.0", EDIT_CONTROL_IP_INPUT);
  AddString(4, "network.gateway", 721, "0.0.0.0", EDIT_CONTROL_IP_INPUT);
  AddString(5, "network.dns", 722, "0.0.0.0", EDIT_CONTROL_IP_INPUT);
  AddString(6, "network.dns2", 722, "0.0.0.0", EDIT_CONTROL_IP_INPUT);
  AddString(7, "network.dnssuffix", 22002, "", EDIT_CONTROL_INPUT, true);
  AddInt(8, "network.bandwidth", 14042, 0, 0, 512, 100*1024, SPIN_CONTROL_INT_PLUS, MASK_KBPS, TEXT_OFF);

  AddSeparator(12, "network.sep1");
  AddBool(13, "network.usehttpproxy", 708, false);
  AddString(14, "network.httpproxyserver", 706, "", EDIT_CONTROL_INPUT);
  AddString(15, "network.httpproxyport", 730, "8080", EDIT_CONTROL_NUMBER_INPUT, false, 707);
  AddString(16, "network.httpproxyusername", 1048, "", EDIT_CONTROL_INPUT);
  AddString(17, "network.httpproxypassword", 733, "", EDIT_CONTROL_HIDDEN_INPUT,true,733);

  // appearance settings
  AddGroup(7, 480);
  AddCategory(7,"lookandfeel", 166);
  AddString(1, "lookandfeel.skin",166,DEFAULT_SKIN, SPIN_CONTROL_TEXT);
  AddString(2, "lookandfeel.skintheme",15111,"SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddString(3, "lookandfeel.skincolors",14078, "SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddString(4, "lookandfeel.font",13303,"Default", SPIN_CONTROL_TEXT);
  AddInt(5, "lookandfeel.skinzoom",20109, 0, -20, 2, 20, SPIN_CONTROL_INT, MASK_PERCENT);
  AddInt(6, "lookandfeel.startupwindow",512,1, WINDOW_HOME, 1, WINDOW_PYTHON_END, SPIN_CONTROL_TEXT);
  AddString(7, "lookandfeel.soundskin",15108,"SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddSeparator(8, "lookandfeel.sep2");
  AddBool(9, "lookandfeel.enablerssfeeds",13305,  true);
  AddString(10, "lookandfeel.rssedit", 21450, "", BUTTON_CONTROL_STANDARD);
  AddSeparator(14, "lookandfeel.sep3");
  AddBool(15, "lookandfeel.enablemouse", 21369, false);

  AddCategory(7, "locale", 14090);
  AddString(1, "locale.language",248,"english", SPIN_CONTROL_TEXT);
  AddString(2, "locale.country", 20026, "UK (24h)", SPIN_CONTROL_TEXT);
  AddString(3, "locale.charset",14091,"DEFAULT", SPIN_CONTROL_TEXT); // charset is set by the language file
  AddSeparator(4, "locale.sep1");
  AddString(5, "locale.time", 14065, "", BUTTON_CONTROL_MISC_INPUT);
  AddString(6, "locale.date", 14064, "", BUTTON_CONTROL_MISC_INPUT);
  AddInt(7, "locale.timezone", 14074, 0, 0, 1, g_timezone.GetNumberOfTimeZones(), SPIN_CONTROL_TEXT);
  AddBool(8, "locale.usedst", 14075, false);
  AddSeparator(9, "locale.sep2");
  AddBool(10, "locale.timeserver", 168, false);
  AddString(11, "locale.timeserveraddress", 731, "pool.ntp.org", EDIT_CONTROL_INPUT);

  AddCategory(7, "videoscreen", 131);
  AddInt(1, "videoscreen.resolution",169,(int)AUTORES, (int)HDTV_1080i, 1, (int)AUTORES, SPIN_CONTROL_TEXT);
  AddString(2, "videoscreen.guicalibration",214,"", BUTTON_CONTROL_STANDARD);
  AddInt(3, "videoscreen.flickerfilter", 13100, 5, 0, 1, 5, SPIN_CONTROL_INT_PLUS, -1, TEXT_OFF);
  AddBool(4, "videoscreen.soften", 215, false);

  AddCategory(7, "filelists", 14081);
  AddBool(1, "filelists.showparentdiritems", 13306, true);
  AddBool(2, "filelists.showextensions", 497, true);
  AddBool(3, "filelists.ignorethewhensorting", 13399, true);
  AddBool(4, "filelists.allowfiledeletion", 14071, false);
  AddBool(5, "filelists.showaddsourcebuttons", 21382,  true);
  AddBool(6, "filelists.showhidden", 21330, false);
  AddSeparator(7, "filelists.sep1");
  AddBool(8, "filelists.unrollarchives",516, false);

  AddCategory(7, "screensaver", 360);
  AddInt(1, "screensaver.time", 355, 3, 1, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS);
  AddString(2, "screensaver.mode", 356, "Dim", SPIN_CONTROL_TEXT);
  AddBool(3, "screensaver.usemusicvisinstead", 13392, true);
  AddBool(4, "screensaver.usedimonpause", 22014, true);
  AddSeparator(5, "screensaver.sep1");
  AddInt(6, "screensaver.dimlevel", 362, 20, 0, 10, 80, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddPath(7, "screensaver.slideshowpath", 774, "F:\\Pictures\\", BUTTON_CONTROL_PATH_INPUT, false, 657);
  AddSeparator(8, "screensaver.sep2");
  AddString(9, "screensaver.preview", 1000, "", BUTTON_CONTROL_STANDARD);

  AddPath(0,"system.playlistspath",20006,"set default",BUTTON_CONTROL_PATH_INPUT,false);
}

CGUISettings::~CGUISettings(void)
{
  Clear();
}

void CGUISettings::AddGroup(int groupID, int labelID)
{
  CSettingsGroup *pGroup = new CSettingsGroup(groupID, labelID);
  if (pGroup)
    settingsGroups.push_back(pGroup);
}

void CGUISettings::AddCategory(int groupID, const char *strSetting, int labelID)
{
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
  {
    if (settingsGroups[i]->GetGroupID() == groupID)
      settingsGroups[i]->AddCategory(CStdString(strSetting).ToLower(), labelID);
  }
}

CSettingsGroup *CGUISettings::GetGroup(int groupID)
{
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
  {
    if (settingsGroups[i]->GetGroupID() == groupID)
      return settingsGroups[i];
  }
  CLog::Log(LOGDEBUG, "Error: Requested setting group (%i) was not found.  "
                      "It must be case-sensitive",
            groupID);
  return NULL;
}

void CGUISettings::AddSeparator(int iOrder, const char *strSetting)
{
  CSettingSeparator *pSetting = new CSettingSeparator(iOrder, CStdString(strSetting).ToLower());
  if (!pSetting) return;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddBool(int iOrder, const char *strSetting, int iLabel, bool bData, int iControlType)
{
  CSettingBool* pSetting = new CSettingBool(iOrder, CStdString(strSetting).ToLower(), iLabel, bData, iControlType);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}
bool CGUISettings::GetBool(const char *strSetting) const
{
  ASSERT(settingsMap.size());
  CStdString lower(strSetting);
  lower.ToLower();
  constMapIter it = settingsMap.find(lower);
  if (it != settingsMap.end())
  { // old category
    return ((CSettingBool*)(*it).second)->GetData();
  }
  // Forward compatibility for new skins (skins use this setting)
  if (lower == "input.enablemouse")
    return GetBool("lookandfeel.enablemouse");
  // Assert here and write debug output
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  return false;
}

void CGUISettings::SetBool(const char *strSetting, bool bSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  { // old category
    ((CSettingBool*)(*it).second)->SetData(bSetting);
    return ;
  }
  // Assert here and write debug output
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::ToggleBool(const char *strSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  { // old category
    ((CSettingBool*)(*it).second)->SetData(!((CSettingBool *)(*it).second)->GetData());
    return ;
  }
  // Assert here and write debug output
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::AddFloat(int iOrder, const char *strSetting, int iLabel, float fData, float fMin, float fStep, float fMax, int iControlType)
{
  CSettingFloat* pSetting = new CSettingFloat(iOrder, CStdString(strSetting).ToLower(), iLabel, fData, fMin, fStep, fMax, iControlType);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

float CGUISettings::GetFloat(const char *strSetting) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    return ((CSettingFloat *)(*it).second)->GetData();
  }
  // Assert here and write debug output
  //ASSERT(false);
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  return 0.0f;
}

void CGUISettings::SetFloat(const char *strSetting, float fSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    ((CSettingFloat *)(*it).second)->SetData(fSetting);
    return ;
  }
  // Assert here and write debug output
  ASSERT(false);
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::LoadMasterLock(TiXmlElement *pRootElement)
{
  std::map<CStdString,CSetting*>::iterator it = settingsMap.find("masterlock.enableshutdown");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
  it = settingsMap.find("masterlock.maxretries");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
  it = settingsMap.find("masterlock.startuplock");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
    it = settingsMap.find("autodetect.nickname");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
}


void CGUISettings::AddInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
{
  CSettingInt* pSetting = new CSettingInt(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, strFormat);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, int iFormat, int iLabelMin/*=-1*/)
{
  CSettingInt* pSetting = new CSettingInt(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, iFormat, iLabelMin);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddHex(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
{
  CSettingHex* pSetting = new CSettingHex(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, strFormat);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

int CGUISettings::GetInt(const char *strSetting) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    return ((CSettingInt *)(*it).second)->GetData();
  }
  // Assert here and write debug output
  CLog::Log(LOGERROR,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  //ASSERT(false);
  return 0;
}

void CGUISettings::SetInt(const char *strSetting, int iSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    ((CSettingInt *)(*it).second)->SetData(iSetting);
    if (stricmp(strSetting, "videoscreen.resolution") == 0)
      g_guiSettings.m_LookAndFeelResolution = (RESOLUTION)iSetting;
    return ;
  }
  // Assert here and write debug output
  ASSERT(false);
}

void CGUISettings::AddString(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
{
  CSettingString* pSetting = new CSettingString(iOrder, CStdString(strSetting).ToLower(), iLabel, strData, iControlType, bAllowEmpty, iHeadingString);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddPath(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
{
  CSettingPath* pSetting = new CSettingPath(iOrder, CStdString(strSetting).ToLower(), iLabel, strData, iControlType, bAllowEmpty, iHeadingString);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

const CStdString &CGUISettings::GetString(const char *strSetting, bool bPrompt) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    CSettingString* result = ((CSettingString *)(*it).second);
    if (result->GetData() == "select folder" || result->GetData() == "select writable folder")
    {
      CStdString strData = "";
      if (bPrompt)
      {
        VECSOURCES shares;
        g_mediaManager.GetLocalDrives(shares);
        if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares,g_localizeStrings.Get(result->GetLabel()),strData,result->GetData() == "select writable folder"))
        {
          result->SetData(strData);
          g_settings.Save();
        }
        else
          return StringUtils::EmptyString;
      }
      else
        return StringUtils::EmptyString;
    }
    return result->GetData();
  }
  // Assert here and write debug output
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  //ASSERT(false);
  // hardcoded return value so that compiler is happy
  return StringUtils::EmptyString;
}

void CGUISettings::SetString(const char *strSetting, const char *strData)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    ((CSettingString *)(*it).second)->SetData(strData);
    return ;
  }
  // Assert here and write debug output
  ASSERT(false);
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

CSetting *CGUISettings::GetSetting(const char *strSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
    return (*it).second;
  else
    return NULL;
}

// get all the settings beginning with the term "strGroup"
void CGUISettings::GetSettingsGroup(const char *strGroup, vecSettings &settings)
{
  vecSettings unorderedSettings;
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    if ((*it).first.Left(strlen(strGroup)).Equals(strGroup) && (*it).second->GetOrder() > 0 && !(*it).second->IsAdvanced())
      unorderedSettings.push_back((*it).second);
  }
  // now order them...
  sort(unorderedSettings.begin(), unorderedSettings.end(), sortsettings());

  // remove any instances of 2 separators in a row
  bool lastWasSeparator(false);
  for (vecSettings::iterator it = unorderedSettings.begin(); it != unorderedSettings.end(); it++)
  {
    CSetting *setting = *it;
    // only add separators if we don't have 2 in a row
    if (setting->GetType() == SETTINGS_TYPE_SEPARATOR)
    {
      if (!lastWasSeparator)
        settings.push_back(setting);
      lastWasSeparator = true;
    }
    else
    {
      lastWasSeparator = false;
      settings.push_back(setting);
    }
  }
}

void CGUISettings::LoadXML(TiXmlElement *pRootElement, bool hideSettings /* = false */)
{ // load our stuff...
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    LoadFromXML(pRootElement, it, hideSettings);
  }
  // Get hardware based stuff...
  CLog::Log(LOGNOTICE, "Getting hardware information now...");
  if (GetInt("audiooutput.mode") == AUDIO_DIGITAL && !g_audioConfig.HasDigitalOutput())
    SetInt("audiooutput.mode", AUDIO_ANALOG);
  SetBool("audiooutput.ac3passthrough", g_audioConfig.GetAC3Enabled());
  SetBool("audiooutput.dtspassthrough", g_audioConfig.GetDTSEnabled());
  CLog::Log(LOGINFO, "Using %s output", GetInt("audiooutput.mode") == AUDIO_ANALOG ? "analog" : "digital");
  CLog::Log(LOGINFO, "AC3 pass through is %s", GetBool("audiooutput.ac3passthrough") ? "enabled" : "disabled");
  CLog::Log(LOGINFO, "DTS pass through is %s", GetBool("audiooutput.dtspassthrough") ? "enabled" : "disabled");
  CLog::Log(LOGINFO, "AAC pass through is %s", GetBool("audiooutput.aacpassthrough") ? "enabled" : "disabled");

  if (g_videoConfig.HasLetterbox())
    SetInt("videooutput.aspect", VIDEO_LETTERBOX);
  else if (g_videoConfig.HasWidescreen())
    SetInt("videooutput.aspect", VIDEO_WIDESCREEN);
  else
    SetInt("videooutput.aspect", VIDEO_NORMAL);
  SetBool("videooutput.hd480p", g_videoConfig.Has480p());
  SetBool("videooutput.hd720p", g_videoConfig.Has720p());

  SetInt("locale.timezone", g_timezone.GetTimeZoneIndex());
  SetBool("locale.usedst", g_timezone.GetDST());

  g_guiSettings.m_LookAndFeelResolution = (RESOLUTION)GetInt("videoscreen.resolution");
  CLog::Log(LOGNOTICE, "Checking resolution %i", g_guiSettings.m_LookAndFeelResolution);
  g_videoConfig.PrintInfo();
  if (
    (g_guiSettings.m_LookAndFeelResolution == AUTORES) ||
    (!g_graphicsContext.IsValidResolution(g_guiSettings.m_LookAndFeelResolution))
  )
  {
#ifdef _XBOX
    RESOLUTION newRes = g_videoConfig.GetBestMode();
#else
    RESOLUTION newRes = g_videoConfig.GetSafeMode();
#endif
    if (g_guiSettings.m_LookAndFeelResolution == AUTORES)
    {
      //"videoscreen.resolution" will stay at AUTORES, m_LookAndFeelResolution will be the real mode
      CLog::Log(LOGNOTICE, "Setting autoresolution mode %i", newRes);
      g_guiSettings.m_LookAndFeelResolution = newRes;
    }
    else
    {
      CLog::Log(LOGNOTICE, "Setting safe mode %i", newRes);
      SetInt("videoscreen.resolution", newRes);
    }
  }

  // Move replaygain settings into our struct
  m_replayGain.iPreAmp = GetInt("musicplayer.replaygainpreamp");
  m_replayGain.iNoGainPreAmp = GetInt("musicplayer.replaygainnogainpreamp");
  m_replayGain.iType = GetInt("musicplayer.replaygaintype");
  m_replayGain.bAvoidClipping = GetBool("musicplayer.replaygainavoidclipping");
}

void CGUISettings::LoadFromXML(TiXmlElement *pRootElement, mapIter &it, bool advanced /* = false */)
{
  CStdStringArray strSplit;
  StringUtils::SplitString((*it).first, ".", strSplit);
  if (strSplit.size() > 1)
  {
    const TiXmlNode *pChild = pRootElement->FirstChild(strSplit[0].c_str());
    if (pChild)
    {
      const TiXmlElement *pGrandChild = pChild->FirstChildElement(strSplit[1].c_str());
      if (pGrandChild && pGrandChild->FirstChild())
      {
        CStdString strValue = pGrandChild->FirstChild()->Value();
        if (strValue.size() )
        {
          if (strValue != "-")
          { // update our item
            if ((*it).second->GetType() == SETTINGS_TYPE_PATH)
            { // check our path
              int pathVersion = 0;
              pGrandChild->Attribute("pathversion", &pathVersion);
              strValue = CSpecialProtocol::ReplaceOldPath(strValue, pathVersion);
            }
            (*it).second->FromString(strValue);
            if (advanced)
              (*it).second->SetAdvanced();
          }
        }
      }
    }
  }
}

void CGUISettings::SaveXML(TiXmlNode *pRootNode)
{
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    // don't save advanced settings
    CStdString first = (*it).first;
    if ((*it).second->IsAdvanced())
      continue;

    CStdStringArray strSplit;
    StringUtils::SplitString((*it).first, ".", strSplit);
    if (strSplit.size() > 1)
    {
      TiXmlNode *pChild = pRootNode->FirstChild(strSplit[0].c_str());
      if (!pChild)
      { // add our group tag
        TiXmlElement newElement(strSplit[0].c_str());
        pChild = pRootNode->InsertEndChild(newElement);
      }

      if (pChild)
      { // successfully added (or found) our group
        TiXmlElement newElement(strSplit[1]);
        if ((*it).second->GetType() == SETTINGS_TYPE_PATH)
          newElement.SetAttribute("pathversion", CSpecialProtocol::path_version);
        TiXmlNode *pNewNode = pChild->InsertEndChild(newElement);
        if (pNewNode)
        {
          TiXmlText value((*it).second->ToString());
          pNewNode->InsertEndChild(value);
        }
      }
    }
  }
}

void CGUISettings::Clear()
{
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
    delete (*it).second;
  settingsMap.clear();
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
    delete settingsGroups[i];
  settingsGroups.clear();
}
