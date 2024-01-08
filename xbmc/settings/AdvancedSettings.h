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

#include <vector>

#include "tinyXML/tinyxml.h"
#include "utils/StdString.h"
#include "utils/StringUtils.h"

struct TVShowRegexp
{
  bool byDate;
  CStdString regexp;
  TVShowRegexp(bool d, const CStdString& r)
  {
    byDate = d;
    regexp = r;
  }
};

typedef std::vector<TVShowRegexp> SETTINGS_TVSHOWLIST;

class CAdvancedSettings
{
  public:
    CAdvancedSettings();

    bool Load();
    void Clear();

    static void GetCustomTVRegexps(TiXmlElement *pRootElement, SETTINGS_TVSHOWLIST& settings);
    static void GetCustomRegexps(TiXmlElement *pRootElement, CStdStringArray& settings);
    static void GetCustomRegexpReplacers(TiXmlElement *pRootElement, CStdStringArray& settings);
    static void GetCustomExtensions(TiXmlElement *pRootElement, CStdString& extensions);

    bool m_DisableModChipDetection;
    bool m_bPowerSave;

    int m_audioHeadRoom;
    float m_karaokeSyncDelay;
    float m_ac3Gain;
    float m_audioPlayCountMinimumPercent;
    bool m_dvdplayerIgnoreDTSinWAV;

    float m_videoSubsDelayRange;
    float m_videoAudioDelayRange;
    int m_videoSmallStepBackSeconds;
    int m_videoSmallStepBackTries;
    int m_videoSmallStepBackDelay;
    bool m_videoUseTimeSeeking;
    int m_videoTimeSeekForward;
    int m_videoTimeSeekBackward;
    int m_videoTimeSeekForwardBig;
    int m_videoTimeSeekBackwardBig;
    int m_videoPercentSeekForward;
    int m_videoPercentSeekBackward;
    int m_videoPercentSeekForwardBig;
    int m_videoPercentSeekBackwardBig;
    CStdString m_videoPPFFmpegDeint;
    CStdString m_videoPPFFmpegPostProc;

    bool m_musicUseTimeSeeking;
    int m_musicTimeSeekForward;
    int m_musicTimeSeekBackward;
    int m_musicTimeSeekForwardBig;
    int m_musicTimeSeekBackwardBig;
    int m_musicPercentSeekForward;
    int m_musicPercentSeekBackward;
    int m_musicPercentSeekForwardBig;
    int m_musicPercentSeekBackwardBig;
    int m_musicResample;
    int m_videoBlackBarColour;
    int m_videoIgnoreSecondsAtStart;
    float m_videoIgnorePercentAtEnd;
    bool m_audioApplyDrc;

    float m_videoPlayCountMinimumPercent;

    unsigned int m_cacheMemBufferSize;

    float m_slideshowBlackBarCompensation;
    float m_slideshowZoomAmount;
    float m_slideshowPanAmount;

    int m_lcdRows;
    int m_lcdColumns;
    int m_lcdAddress1;
    int m_lcdAddress2;
    int m_lcdAddress3;
    int m_lcdAddress4;

    int m_autoDetectPingTime;

    int m_songInfoDuration;
    int m_busyDialogDelay;
    int m_logLevel;
    int m_logLevelHint;
    CStdString m_cddbAddress;
    bool m_usePCDVDROM;
    bool m_fullScreenOnMovieStart;
    bool m_noDVDROM;
    CStdString m_cachePath;
    bool m_displayRemoteCodes;
    CStdString m_videoCleanDateTimeRegExp;
    CStdStringArray m_videoCleanStringRegExps;
    CStdStringArray m_videoExcludeFromListingRegExps;
    CStdStringArray m_moviesExcludeFromScanRegExps;
    CStdStringArray m_tvshowExcludeFromScanRegExps;
    CStdStringArray m_audioExcludeFromListingRegExps;
    CStdStringArray m_audioExcludeFromScanRegExps;
    CStdStringArray m_pictureExcludeFromListingRegExps;
    CStdStringArray m_videoStackRegExps;
    CStdStringArray m_trailerMatchRegExps;
    SETTINGS_TVSHOWLIST m_tvshowStackRegExps;
    CStdString m_tvshowMultiPartStackRegExp;
    CStdStringArray m_pathSubstitutions;
    int m_remoteRepeat;
    float m_controllerDeadzone;
    bool m_FTPShowCache;

    bool m_playlistAsFolders;
    bool m_detectAsUdf;

    int m_thumbSize;
    int m_fanartHeight;
    //dds DXT1 support
    bool m_useddsfanart;

    int m_sambaclienttimeout;
    CStdString m_sambadoscodepage;
    bool m_sambastatfiles;
   
    bool m_bHTTPDirectoryStatFilesize;

    bool m_bFTPThumbs;

    CStdString m_musicThumbs;
    CStdString m_dvdThumbs;
    CStdString m_fanartImages;

    bool m_bMusicLibraryHideAllItems;
    int m_iMusicLibraryRecentlyAddedItems;
    bool m_bMusicLibraryAllItemsOnBottom;
    bool m_bMusicLibraryAlbumsSortByArtistThenYear;
    CStdString m_strMusicLibraryAlbumFormat;
    CStdString m_strMusicLibraryAlbumFormatRight;
    bool m_prioritiseAPEv2tags;
    CStdString m_musicItemSeparator;
    CStdString m_videoItemSeparator;
    std::vector<CStdString> m_musicTagsFromFileFilters;

    bool m_bVideoLibraryHideAllItems;
    bool m_bVideoLibraryAllItemsOnBottom;
    int m_iVideoLibraryRecentlyAddedItems;
    bool m_bVideoLibraryHideRecentlyAddedItems;
    bool m_bVideoLibraryHideEmptySeries;
    bool m_bVideoLibraryCleanOnUpdate;
    bool m_bVideoLibraryExportAutoThumbs;
    bool m_bVideoLibraryMyMoviesCategoriesToGenres;
    bool m_bVideoLibraryImportWatchedState;

    bool m_bVideoScannerIgnoreErrors;

    std::vector<CStdString> m_vecTokens; // cleaning strings tied to language
    //TuxBox
    int m_iTuxBoxStreamtsPort;
    bool m_bTuxBoxSubMenuSelection;
    int m_iTuxBoxDefaultSubMenu;
    int m_iTuxBoxDefaultRootMenu;
    bool m_bTuxBoxAudioChannelSelection;
    bool m_bTuxBoxPictureIcon;
    int m_iTuxBoxEpgRequestTime;
    int m_iTuxBoxZapWaitTime;
    bool m_bTuxBoxSendAllAPids;
    bool m_bTuxBoxZapstream;
    int m_iTuxBoxZapstreamPort;

    int m_iMythMovieLength;         // minutes

    // EDL Commercial Break
    bool m_bEdlMergeShortCommBreaks;
    int m_iEdlMaxCommBreakLength;   // seconds
    int m_iEdlMinCommBreakLength;   // seconds
    int m_iEdlMaxCommBreakGap;      // seconds
    int m_iEdlMaxStartGap;          // seconds
    int m_iEdlCommBreakAutowait;    // seconds
    int m_iEdlCommBreakAutowind;    // seconds

    bool m_bFirstLoop;
    int m_curlconnecttimeout;
    int m_curllowspeedtime;
    int m_curlretries;

    int m_playlistRetries;
    int m_playlistTimeout;
    bool m_bVirtualShares; 
    bool m_bNavVKeyboard; // if true we navigate the virtual keyboard using cursor keys
    
    bool m_bPythonVerbose;
    int m_bgInfoLoaderMaxThreads;
    
    void SetDebugMode(bool debug);
};

extern CAdvancedSettings g_advancedSettings;

