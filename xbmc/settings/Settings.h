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

#define PRE_SKIN_VERSION_9_10_COMPATIBILITY 1

#define DEFAULT_SKIN "Confluence Lite"
#define DEFAULT_WEATHER_PLUGIN "Weather.com (standard)"

#include "settings/VideoSettings.h"
#include "settings/GUISettings.h"
#include "settings/Profile.h"
#include "MediaSource.h"
#include "ViewState.h"
#include "settings/AdvancedSettings.h"

#include <vector>
#include <map>

#define CACHE_AUDIO 0
#define CACHE_VIDEO 1
#define CACHE_VOB   2

#define VOLUME_MINIMUM -6000  // -60dB
#define VOLUME_MAXIMUM 0      // 0dB
#define VOLUME_DRC_MINIMUM 0    // 0dB
#define VOLUME_DRC_MAXIMUM 3000 // 30dB

#define VIEW_MODE_NORMAL        0
#define VIEW_MODE_ZOOM          1
#define VIEW_MODE_STRETCH_4x3   2
#define VIEW_MODE_STRETCH_14x9  3
#define VIEW_MODE_STRETCH_16x9  4
#define VIEW_MODE_ORIGINAL      5
#define VIEW_MODE_CUSTOM        6

#define STACK_NONE          0
#define STACK_SIMPLE        1

#define VIDEO_SHOW_ALL 0
#define VIDEO_SHOW_UNWATCHED 1
#define VIDEO_SHOW_WATCHED 2

/* FIXME: eventually the profile should dictate where special://masterprofile/ is but for now it
   makes sense to leave all the profile settings in a user writeable location
   like special://masterprofile/ */
#define PROFILES_FILE "special://masterprofile/profiles.xml"

class CSkinString
{
public:
  CStdString name;
  CStdString value;
};

class CSkinBool
{
public:
  CStdString name;
  bool value;
};

struct VOICE_MASK {
  float energy;
  float pitch;
  float robotic;
  float whisper;
};

class CGUISettings;
class TiXmlElement;
class TiXmlNode;

class CSettings
{
public:
  CSettings(void);
  virtual ~CSettings(void);

  void Initialize();

  bool Load(bool& bXboxMediacenter, bool& bSettings);
  void Save() const;
  bool Reset();

  void Clear();

  bool LoadProfile(unsigned int index);
  bool DeleteProfile(unsigned int index);
  void DeleteAllProfiles();
  void CreateProfileFolders();

  VECSOURCES *GetSourcesFromType(const CStdString &type);
  CStdString GetDefaultSourceFromType(const CStdString &type);

  bool UpdateSource(const CStdString &strType, const CStdString strOldName, const CStdString &strUpdateChild, const CStdString &strUpdateValue);
  bool DeleteSource(const CStdString &strType, const CStdString strName, const CStdString strPath, bool virtualSource = false);
  bool UpdateShare(const CStdString &type, const CStdString oldName, const CMediaSource &share);
  bool AddShare(const CStdString &type, const CMediaSource &share);

  int TranslateSkinString(const CStdString &setting);
  const CStdString &GetSkinString(int setting) const;
  void SetSkinString(int setting, const CStdString &label);

  int TranslateSkinBool(const CStdString &setting);
  bool GetSkinBool(int setting) const;
  void SetSkinBool(int setting, bool set);

  void ResetSkinSetting(const CStdString &setting);
  void ResetSkinSettings();

  struct AdvancedSettings
  {
public:
    // multipath testing
    bool m_useMultipaths;
    bool m_DisableModChipDetection;

    int m_audioHeadRoom;
    float m_ac3Gain;
    CStdString m_audioDefaultPlayer;
    float m_audioPlayCountMinimumPercent;

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
    CStdString m_videoPPFFmpegType;
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
    int m_videoIgnoreAtStart;
    int m_videoIgnoreAtEnd;
    CStdString m_audioHost;
    bool m_audioApplyDrc;

    CStdString m_videoDefaultPlayer;
    CStdString m_videoDefaultDVDPlayer;
    float m_videoPlayCountMinimumPercent;

    float m_slideshowBlackBarCompensation;
    float m_slideshowZoomAmount;
    float m_slideshowPanAmount;

    int m_lcdRows;
    int m_lcdColumns;
    int m_lcdAddress1;
    int m_lcdAddress2;
    int m_lcdAddress3;
    int m_lcdAddress4;
    bool m_lcdHeartbeat;
    int m_lcdScrolldelay;

    int m_autoDetectPingTime;

    int m_songInfoDuration;
    int m_busyDialogDelay;
    int m_logLevel;
    int m_logLevelHint;
    CStdString m_cddbAddress;
#ifdef HAS_HAL
    bool m_useHalMount;
#endif
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
    SETTINGS_TVSHOWLIST m_tvshowStackRegExps;
    CStdString m_tvshowMultiPartStackRegExp;
    CStdStringArray m_pathSubstitutions;
    int m_remoteRepeat;
    float m_controllerDeadzone;
    bool m_FTPShowCache;

    bool m_playlistAsFolders;
    bool m_detectAsUdf;

    int m_thumbSize;

    int m_sambaclienttimeout;
    CStdString m_sambadoscodepage;
    bool m_sambastatfiles;

    bool m_bHTTPDirectoryStatFilesize;

    CStdString m_musicThumbs;
    CStdString m_dvdThumbs;

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

    bool m_bUseEvilB;
    std::vector<CStdString> m_vecTokens; // cleaning strings tied to language
    //TuxBox
    bool m_bTuxBoxSubMenuSelection;
    int m_iTuxBoxDefaultSubMenu;
    int m_iTuxBoxDefaultRootMenu;
    bool m_bTuxBoxAudioChannelSelection;
    bool m_bTuxBoxPictureIcon;
    int m_iTuxBoxEpgRequestTime;
    int m_iTuxBoxZapWaitTime;
    bool m_bTuxBoxSendAllAPids;

    int m_iMythMovieLength;         // minutes

    // EDL Commercial Break
    bool m_bEdlMergeShortCommBreaks;
    int m_iEdlMaxCommBreakLength;   // seconds
    int m_iEdlMinCommBreakLength;   // seconds
    int m_iEdlMaxCommBreakGap;      // seconds

    bool m_bFirstLoop;
    int m_curlconnecttimeout;
    int m_curllowspeedtime;
    int m_curlretries;

    bool m_fullScreen;
    bool m_startFullScreen;
    bool m_alwaysOnTop;  /* makes xbmc to run always on top .. osx/win32 only .. */
    int m_playlistRetries;
    int m_playlistTimeout;
    bool m_GLRectangleHack;
    int m_iSkipLoopFilter;
    float m_ForcedSwapTime; /* if nonzero, set's the explicit time in ms to allocate for buffer swap */

    bool m_osx_GLFullScreen;
    bool m_bVirtualShares;
    bool m_bNavVKeyboard; // if true we navigate the virtual keyboard using cursor keys

    float m_karaokeSyncDelayCDG; // seems like different delay is needed for CDG and MP3s
    float m_karaokeSyncDelayLRC;
    bool m_karaokeChangeGenreForKaraokeSongs;
    bool m_karaokeKeepDelay; // store user-changed song delay in the database
    int m_karaokeStartIndex; // auto-assign numbering start from this value
    bool m_karaokeAlwaysEmptyOnCdgs; // always have empty background on CDG files
    bool m_karaokeUseSongSpecificBackground; // use song-specific video or image if available instead of default
    CStdString m_karaokeDefaultBackgroundType; // empty string or "vis", "image" or "video"
    CStdString m_karaokeDefaultBackgroundFilePath; // only for "image" or "video" types above

    CStdString m_cpuTempCmd;
    CStdString m_gpuTempCmd;
    int m_bgInfoLoaderMaxThreads;
  };

  CStdString m_pictureExtensions;
  CStdString m_musicExtensions;
  CStdString m_videoExtensions;

  CStdString m_logFolder;

  bool m_bMyMusicSongInfoInVis;
  bool m_bMyMusicSongThumbInVis;

  CViewState m_viewStateMusicNavArtists;
  CViewState m_viewStateMusicNavAlbums;
  CViewState m_viewStateMusicNavSongs;
  CViewState m_viewStateMusicShoutcast;
  CViewState m_viewStateMusicLastFM;
  CViewState m_viewStateVideoNavActors;
  CViewState m_viewStateVideoNavYears;
  CViewState m_viewStateVideoNavGenres;
  CViewState m_viewStateVideoNavTitles;
  CViewState m_viewStateVideoNavEpisodes;
  CViewState m_viewStateVideoNavSeasons;
  CViewState m_viewStateVideoNavTvShows;
  CViewState m_viewStateVideoNavMusicVideos;

  CViewState m_viewStatePrograms;
  CViewState m_viewStatePictures;
  CViewState m_viewStateMusicFiles;
  CViewState m_viewStateVideoFiles;

  bool m_bMyMusicPlaylistRepeat;
  bool m_bMyMusicPlaylistShuffle;
  int m_iMyMusicStartWindow;

  // for scanning
  bool m_bMyMusicIsScanning;

  CVideoSettings m_defaultVideoSettings;
  CVideoSettings m_currentVideoSettings;

  float m_fZoomAmount;      // current zoom amount
  float m_fPixelRatio;      // current pixel ratio

  int m_iMyVideoWatchMode;

  bool m_bMyVideoPlaylistRepeat;
  bool m_bMyVideoPlaylistShuffle;
  bool m_bMyVideoNavFlatten;
  bool m_bStartVideoWindowed;

  int m_iVideoStartWindow;

  int m_iMyVideoStack;

  int iAdditionalSubtitleDirectoryChecked;

  char szOnlineArenaPassword[32]; // private arena password
  char szOnlineArenaDescription[64]; // private arena description

  int m_HttpApiBroadcastPort;
  int m_HttpApiBroadcastLevel;
  int m_nVolumeLevel;                     // measured in milliBels -60dB -> 0dB range.
  int m_dynamicRangeCompressionLevel;     // measured in milliBels  0dB -> 30dB range.
  int m_iPreMuteVolumeLevel;    // save the m_nVolumeLevel for proper restore
  bool m_bMute;
  int m_iSystemTimeTotalUp;    // Uptime in minutes!

  VOICE_MASK m_karaokeVoiceMask[4];

  struct RssSet
  {
    bool rtl;
    std::vector<int> interval;
    std::vector<std::string> url;
  };

  std::map<int,RssSet> m_mapRssUrls;
  std::map<int, CSkinString> m_skinStrings;
  std::map<int, CSkinBool> m_skinBools;

  VECSOURCES m_programSources;
  VECSOURCES m_pictureSources;
  VECSOURCES m_fileSources;
  VECSOURCES m_musicSources;
  VECSOURCES m_videoSources;

  CStdString m_defaultProgramSource;
  CStdString m_defaultMusicSource;
  CStdString m_defaultPictureSource;
  CStdString m_defaultFileSource;
  CStdString m_defaultVideoSource;
  CStdString m_defaultMusicLibSource;
  CStdString m_defaultVideoLibSource;

  CStdString m_UPnPUUIDServer;
  int        m_UPnPPortServer;
  int        m_UPnPMaxReturnedItems;
  CStdString m_UPnPUUIDRenderer;
  int        m_UPnPPortRenderer;

  /*! \brief Retrieve the master profile
   \return const reference to the master profile
   */
  const CProfile &GetMasterProfile() const;

  /*! \brief Retreive the current profile
   \return const reference to the current profile
   */
  const CProfile &GetCurrentProfile() const;

  /*! \brief Retreive the profile from an index
   \param unsigned index of the profile to retrieve
   \return const pointer to the profile, NULL if the index is invalid
   */
  const CProfile *GetProfile(unsigned int index) const;

  /*! \brief Retreive the profile from an index
   \param unsigned index of the profile to retrieve
   \return pointer to the profile, NULL if the index is invalid
   */
  CProfile *GetProfile(unsigned int index);

  /*! \brief Retreive index of a particular profile by name
   \param name name of the profile index to retrieve
   \return index of this profile, -1 if invalid.
   */
  int GetProfileIndex(const CStdString &name) const;

  /*! \brief Retrieve the number of profiles
   \return number of profiles
   */
  unsigned int GetNumProfiles() const;

  /*! \brief Add a new profile
   \param profile CProfile to add
   */
  void AddProfile(const CProfile &profile);

  /*! \brief Are we using the login screen?
   \return true if we're using the login screen, false otherwise
   */
  bool UsingLoginScreen() const { return m_usingLoginScreen; };

  /*! \brief Toggle login screen use on and off
   Toggles the login screen state
   */
  void ToggleLoginScreen() { m_usingLoginScreen = !m_usingLoginScreen; };

  /*! \brief Are we the master user?
   \return true if the current profile is the master user, false otherwise
   */
  bool IsMasterUser() const { return 0 == m_currentProfile; };

  /*! \brief Update the date of the current profile
   */
  void UpdateCurrentProfileDate();

  /*! \brief Load the master user for the purposes of logging in
   Loads the master user.  Identical to LoadProfile(0) but doesn't update the last logged in details
   */
  void LoadMasterForLogin();

  /*! \brief Retreive the last used profile index
   \return the last used profile that logged in.  Does not count the master user during login.
   */
  unsigned int GetLastUsedProfileIndex() const { return m_lastUsedProfile; };

  /*! \brief Retrieve the current profile index
   \return the index of the currently logged in profile.
   */
  unsigned int GetCurrentProfileIndex() const { return m_currentProfile; };

  RESOLUTION_INFO m_ResInfo[10];

  // utility functions for user data folders

  //uses HasSlashAtEnd to determine if a directory or file was meant
  CStdString GetUserDataItem(const CStdString& strFile) const;
  CStdString GetProfileUserDataFolder() const;
  CStdString GetUserDataFolder() const;
  CStdString GetDatabaseFolder() const;
  CStdString GetCDDBFolder() const;
  CStdString GetThumbnailsFolder() const;
  CStdString GetMusicThumbFolder() const;
  CStdString GetLastFMThumbFolder() const;
  CStdString GetMusicArtistThumbFolder() const;
  CStdString GetVideoThumbFolder() const;
  CStdString GetBookmarksThumbFolder() const;
  CStdString GetPicturesThumbFolder() const;
  CStdString GetProgramsThumbFolder() const;
  CStdString GetGameSaveThumbFolder() const;
  CStdString GetProfilesThumbFolder() const;
  CStdString GetSourcesFile() const;
  CStdString GetSkinFolder() const;
  CStdString GetSkinFolder(const CStdString& skinName) const;
  CStdString GetScriptsFolder() const;
  CStdString GetVideoFanartFolder() const;
  CStdString GetMusicFanartFolder() const;
  CStdString GetFFmpegDllFolder() const;
  CStdString GetPlayerName(const int& player) const;
  CStdString GetDefaultVideoPlayerName() const;
  CStdString GetDefaultAudioPlayerName() const;

  CStdString GetSettingsFile() const;
  CStdString GetAvpackSettingsFile() const;

  bool LoadUPnPXml(const CStdString& strSettingsFile);
  bool SaveUPnPXml(const CStdString& strSettingsFile) const;

  /*! \brief Load the user profile information from disk
   Loads the profiles.xml file and creates the list of profiles. If no profiles
   exist, a master user is created.  Should be called after special://masterprofile/
   has been defined.
   \param profilesFile XML file to load.
   */
  void LoadProfiles(const CStdString& profilesFile);

  /*! \brief Save the user profile information to disk
   Saves the list of profiles to the profiles.xml file.
   \param profilesFile XML file to save.
   \return true on success, false on failure to save
   */
  bool SaveProfiles(const CStdString& profilesFile) const;

  bool SaveSettings(const CStdString& strSettingsFile, CGUISettings *localSettings = NULL) const;

  bool SaveSources();

  void LoadRSSFeeds();
  bool GetInteger(const TiXmlElement* pRootElement, const char *strTagName, int& iValue, const int iDefault, const int iMin, const int iMax);
  bool GetFloat(const TiXmlElement* pRootElement, const char *strTagName, float& fValue, const float fDefault, const float fMin, const float fMax);
  static bool GetPath(const TiXmlElement* pRootElement, const char *tagName, CStdString &strValue);
  static bool GetString(const TiXmlElement* pRootElement, const char *strTagName, CStdString& strValue, const CStdString& strDefaultValue);
  bool GetString(const TiXmlElement* pRootElement, const char *strTagName, char *szValue, const CStdString& strDefaultValue);
  bool GetSource(const CStdString &category, const TiXmlNode *source, CMediaSource &share);
protected:
  void GetSources(const TiXmlElement* pRootElement, const CStdString& strTagName, VECSOURCES& items, CStdString& strDefault);
  bool SetSources(TiXmlNode *root, const char *section, const VECSOURCES &shares, const char *defaultPath);
  void GetViewState(const TiXmlElement* pRootElement, const CStdString& strTagName, CViewState &viewState, SORT_METHOD defaultSort = SORT_METHOD_LABEL, int defaultView = DEFAULT_VIEW_LIST);

  void ConvertHomeVar(CStdString& strText);
  // functions for writing xml files
  void SetViewState(TiXmlNode* pRootNode, const CStdString& strTagName, const CViewState &viewState) const;

  bool LoadCalibration(const TiXmlElement* pElement, const CStdString& strSettingsFile);
  bool SaveCalibration(TiXmlNode* pRootNode) const;

  bool LoadSettings(const CStdString& strSettingsFile);
//  bool SaveSettings(const CStdString& strSettingsFile) const;

  bool LoadPlayerCoreFactorySettings(const CStdString& fileStr, bool clear);

  // skin activated settings
  void LoadSkinSettings(const TiXmlElement* pElement);
  void SaveSkinSettings(TiXmlNode *pElement) const;

  void LoadUserFolderLayout();

  bool SaveAvpackXML() const;
  bool SaveNewAvpackXML() const;
  bool SaveAvpackSettings(TiXmlNode *io_pRoot) const;
  bool LoadAvpackXML();

private:
  std::vector<CProfile> m_vecProfiles;
  bool m_usingLoginScreen;
  unsigned int m_lastUsedProfile;
  unsigned int m_currentProfile;
};

extern class CSettings g_settings;