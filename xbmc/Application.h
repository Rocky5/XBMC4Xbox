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

#include "system.h"
#include "XBApplicationEx.h"

#include "IMsgTargetCallback.h"
#include "Key.h"

class CFileItem;
class CFileItemList;

#include "dialogs/GUIDialogSeekBar.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogVolumeBar.h"
#include "dialogs/GUIDialogMuteBug.h"
#include "windows/GUIWindowPointer.h"   // Mouse pointer

#include "xbox/Network.h"
#include "utils/Idle.h"
#include "utils/DelayController.h"
#include "cores/IPlayer.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "PlayListPlayer.h"
#include "storage/DetectDVDType.h"
#include "Autorun.h"
#include "video/Bookmark.h"
#include "utils/Stopwatch.h"
#include "ApplicationMessenger.h"


class CWebServer;
class CXBFileZilla;
class CSNTPClient;
class CCdgParser;
class CApplicationMessenger;
class CProfile;
class CSplash;

class CApplication : public CXBApplicationEx, public IPlayerCallback, public IMsgTargetCallback
{
public:
  CApplication(void);
  virtual ~CApplication(void);
  virtual HRESULT Initialize();
  virtual void FrameMove();
  virtual void Render();
  virtual void DoRender();
#ifndef HAS_XBOX_D3D
  virtual void RenderNoPresent();
#endif
  virtual HRESULT Create(HWND hWnd);
  virtual HRESULT Cleanup();
  void StartServices();
  void StopServices();
  void StartIdleThread();
  void StopIdleThread();
  void StartWebServer();
  void StopWebServer();
  void StartFtpServer();
  void StopFtpServer();
  void StartTimeServer();
  void StopTimeServer();
  void StartUPnP();
  void StopUPnP(bool bWait = false);
  void StartUPnPRenderer();
  void StopUPnPRenderer();
  void StartUPnPServer();
  void StopUPnPServer();
  void StartEventServer();
  bool StopEventServer(bool promptuser=false);
  void RefreshEventServer();
  void StartLEDControl(bool switchoff = false);
  void DimLCDOnPlayback(bool dim);
  bool IsCurrentThread() const;
  void PrintXBEToLCD(const char* xbePath);
  void CheckDate();
  DWORD GetThreadId() const { return m_threadID; };
  void Stop(bool bLCDStop = true);
  void RestartApp();
  void LoadSkin(const CStdString& strSkin);
  void UnloadSkin();
  bool LoadUserWindows();
  void DelayLoadSkin();
  void CancelDelayLoadSkin();
  void ReloadSkin();
  const CStdString& CurrentFile();
  CFileItem& CurrentFileItem();
  virtual bool OnMessage(CGUIMessage& message);
  PLAYERCOREID GetCurrentPlayer();
  virtual void OnPlayBackEnded();
  virtual void OnPlayBackStarted();
  virtual void OnPlayBackPaused();
  virtual void OnPlayBackResumed();
  virtual void OnPlayBackStopped();
  virtual void OnQueueNextItem();
  virtual void OnPlayBackSeek(int iTime, int seekOffset);
  virtual void OnPlayBackSeekChapter(int iChapter);
  virtual void OnPlayBackSpeedChanged(int iSpeed);
  bool PlayMedia(const CFileItem& item, int iPlaylist = PLAYLIST_MUSIC);
  bool ProcessAndStartPlaylist(const CStdString& strPlayList, PLAYLIST::CPlayList& playlist, int iPlaylist, int track=0);
  bool PlayFile(const CFileItem& item, bool bRestart = false);
  void SaveFileState();
  void UpdateFileState();
  void StopPlaying();
  void Restart(bool bSamePosition = true);
  void DelayedPlayerRestart();
  void CheckDelayedPlayerRestart();
  void RenderFullScreen();
  void DoRenderFullScreen();
  bool NeedRenderFullScreen();
  bool IsPlaying() const;
  bool IsPaused() const;
  bool IsPlayingAudio() const;
  bool IsPlayingVideo() const;
  bool IsPlayingFullScreenVideo() const;
  bool IsStartingPlayback() const { return m_bPlaybackStarting; }
  bool OnKey(CKey& key);
  bool OnAction(CAction &action);
  void RenderMemoryStatus();
  bool MustBlockHDSpinDown(bool bCheckThisForNormalSpinDown = true);
  void CheckNetworkHDSpinDown(bool playbackStarted = false);
  void CheckHDSpindown();
  void CheckShutdown();
  void CheckScreenSaver();   // CB: SCREENSAVER PATCH
  void CheckPlayingProgress();
  void CheckAudioScrobblerStatus();
  void ActivateScreenSaver(bool forceType = false);

  virtual void Process();
  void ProcessSlow();
  void ResetScreenSaver();
  int GetVolume() const;
  void SetVolume(int iPercent);
  void Mute(void);
  int GetPlaySpeed() const;
  int GetSubtitleDelay() const;
  int GetAudioDelay() const;
  void SetPlaySpeed(int iSpeed);
  bool IsButtonDown(DWORD code);
  bool AnyButtonDown();
  bool ResetScreenSaverWindow();
  double GetTotalTime() const;
  double GetTime() const;
  float GetPercentage() const;

  // Get the percentage of data currently cached/buffered (aq/vq + FileCache) from the input stream if applicable.
  float GetCachePercentage() const;

  void SeekPercentage(float percent);
  void SeekTime( double dTime = 0.0 );
  void ResetPlayTime();

  void SaveMusicScanSettings();
  void RestoreMusicScanSettings();
  void UpdateLibraries();
  void CheckMusicPlaylist();

  CApplicationMessenger& getApplicationMessenger();
  CNetwork& getNetwork();

  bool ExecuteXBMCAction(std::string action);

  CGUIDialogVolumeBar m_guiDialogVolumeBar;
  CGUIDialogSeekBar m_guiDialogSeekBar;
  CGUIDialogKaiToast m_guiDialogKaiToast;
  CGUIDialogMuteBug m_guiDialogMuteBug;
  CGUIWindowPointer m_guiPointer;

  CIdleThread m_idleThread;
  MEDIA_DETECT::CAutorun m_Autorun;
  MEDIA_DETECT::CDetectDVDMedia m_DetectDVDType;
  CDelayController m_ctrDpad;
  CSNTPClient *m_psntpClient;
  CWebServer* m_pWebServer;
  CXBFileZilla* m_pFileZilla;
  IPlayer* m_pPlayer;

  bool m_bSpinDown;
  bool m_bNetworkSpinDown;
  DWORD m_dwSpinDownTime;

  inline bool IsInScreenSaver() { return m_bScreenSave; };
  int m_iScreenSaveLock; // spiff: are we checking for a lock? if so, ignore the screensaver state, if -1 we have failed to input locks

  unsigned int m_skinReloadTime;
  bool m_bIsPaused;
  bool m_bPlaybackStarting;
  bool m_128MBHack;

  CCdgParser* m_pCdgParser;

  PLAYERCOREID m_eForcedNextPlayer;
  CStdString m_strPlayListFile;

  int GlobalIdleTime();

protected:
  friend class CApplicationMessenger;
  // screensaver
  bool m_bScreenSave;
  CStdString m_screenSaverMode;

  D3DGAMMARAMP m_OldRamp;

  // timer information
  CStopWatch m_idleTimer;
  CStopWatch m_restartPlayerTimer;
  CStopWatch m_frameTime;
  CStopWatch m_navigationTimer;
  CStopWatch m_slowTimer;
  CStopWatch m_screenSaverTimer;
  CStopWatch m_shutdownTimer;

  CFileItemPtr m_itemCurrentFile;
  CFileItemList* m_currentStack;
  CStdString m_prevMedia;
  CSplash* m_splash;
  DWORD m_threadID;       // application thread ID.  Used in applicationMessanger to know where we are firing a thread with delay from.
  PLAYERCOREID m_eCurrentPlayer;
  bool m_bXboxMediacenterLoaded;
  bool m_bSettingsLoaded;
  bool m_bAllSettingsLoaded;
  bool m_bInitializing;

  CBookmark m_progressTrackingVideoResumeBookmark;
  CFileItemPtr m_progressTrackingItem;
  bool m_progressTrackingPlayCountUpdate;

  int m_iPlaySpeed;
  int m_currentStackPosition;
  int m_nextPlaylistItem;

  static LONG WINAPI UnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo);

  void SetHardwareVolume(long hardwareVolume);
  void UpdateLCD();
  void FatalErrorHandler(bool InitD3D, bool MapDrives, bool InitNetwork);
  void InitBasicD3D();

  bool PlayStack(const CFileItem& item, bool bRestart);
  bool SwitchToFullScreen();
  bool ProcessMouse();
  bool ProcessHTTPApiButtons();
  bool ProcessKeyboard();
  bool ProcessRemote(float frameTime);
  bool ProcessGamepad(float frameTime);
  bool ProcessEventServer(float frameTime);

  bool ProcessJoystickEvent(const std::string& joystickName, int button, bool isAxis, float fAmount, unsigned int holdTime = 0);

  void CheckForDebugButtonCombo();
  void StartFtpEmergencyRecoveryMode();
  float NavigationIdleTime();
  void CheckForTitleChange();
  static bool AlwaysProcess(const CAction& action);

  void SaveCurrentFileSettings();

  void InitDirectoriesXbox();

  CApplicationMessenger m_applicationMessenger;
  CNetwork m_network;
  
#ifdef HAS_EVENT_SERVER
  std::map<std::string, std::map<int, float> > m_lastAxisMap;
#endif
};

extern CApplication g_application;
extern CStdString g_LoadErrorStr;
