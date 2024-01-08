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
#include "Application.h"
#include "AutoPtrHandle.h"
#include "video/windows/GUIWindowVideoBase.h"
#include "Util.h"
#include "xbox/IoSupport.h"
#include "xbox/xbeheader.h"
#ifdef HAS_XBOX_HARDWARE
#include "xbox/Undocumented.h"
#include "xbresource.h"
#endif
#include "storage/DetectDVDType.h"
#include "Autorun.h"
#include "FileSystem/HDDirectory.h"
#include "FileSystem/StackDirectory.h"
#include "FileSystem/MultiPathDirectory.h"
#include "FileSystem/DirectoryCache.h"
#include "FileSystem/SpecialProtocol.h"
#include "ThumbnailCache.h"
#include "FileSystem/ZipManager.h"
#include "FileSystem/RarManager.h"
#include "FileSystem/MythDirectory.h"
#include "FileSystem/VideoDatabaseDirectory.h"
#ifdef HAS_UPNP
#include "FileSystem/UPnPDirectory.h"
#endif
#ifdef HAS_CREDITS
#include "Credits.h"
#endif
#include "Shortcut.h"
#include "PlayListPlayer.h"
#include "PartyModeManager.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#include "lib/libPython/XBPython.h"
#include "utils/RegExp.h"
#include "utils/AlarmClock.h"
#include "utils/RssFeed.h"
#include "input/ButtonTranslator.h"
#include "pictures/Picture.h"
#include "dialogs/GUIDialogNumeric.h"
#include "music/dialogs/GUIDialogMusicScan.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "video/dialogs/GUIDialogVideoScan.h"
#include "utils/fstrcmp.h"
#include "utils/Trainer.h"
#ifdef HAS_XBOX_HARDWARE
#include "utils/MemoryUnitManager.h"
#include "utils/FilterFlickerPatch.h"
#include "utils/LED.h"
#include "utils/FanController.h"
#include "utils/SystemInfo.h"
#endif
#include "storage/MediaManager.h"
#ifdef _XBOX
#include <xbdm.h>
#endif
#include "xbox/network.h"
#include "GUIPassword.h"
#ifdef HAS_FTP_SERVER
#include "lib/libfilezilla/xbfilezilla.h"
#endif
#include "lib/libscrobbler/scrobbler.h"
#include "music/LastFmManager.h"
#include "music/MusicInfoLoader.h"
#include "XBVideoConfig.h"
#ifndef HAS_XBOX_D3D
#include "DirectXGraphics.h"
#endif
#include "music/tags/MusicInfoTag.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogKeyboard.h"
#include "FileSystem/File.h"
#include "playlists/PlayList.h"
#include "utils/Crc32.h"
#include "utils/RssReader.h"
#include "settings/AdvancedSettings.h"
#include "utils/URIUtils.h"
#include "cores/dvdplayer/DVDSubtitles/DVDSubtitleTagSami.h"
#include "cores/dvdplayer/DVDSubtitles/DVDSubtitleStream.h"
#include "LocalizeStrings.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"

using namespace std;

#define clamp(x) (x) > 255.f ? 255 : ((x) < 0 ? 0 : (BYTE)(x+0.5f)) // Valid ranges: brightness[-1 -> 1 (0 is default)] contrast[0 -> 2 (1 is default)]  gamma[0.5 -> 3.5 (1 is default)] default[ramp is linear]
static const __int64 SECS_BETWEEN_EPOCHS = 11644473600LL;
static const __int64 SECS_TO_100NS = 10000000;

using namespace AUTOPTR;
using namespace MEDIA_DETECT;
using namespace XFILE;
using namespace PLAYLIST;
static D3DGAMMARAMP oldramp, flashramp;

XBOXDETECTION v_xboxclients;

#ifdef HAS_XBOX_HARDWARE
// This are 70 Original Data Bytes because we have to restore 70 patched Bytes, not just 57
static BYTE rawData[70] =
{
    0x55, 0x8B, 0xEC, 0x81, 0xEC, 0x04, 0x01, 0x00, 0x00, 0x8B, 0x45, 0x08, 0x3D, 0x04, 0x01, 0x00, 
    0x00, 0x53, 0x75, 0x32, 0x8B, 0x4D, 0x18, 0x85, 0xC9, 0x6A, 0x04, 0x58, 0x74, 0x02, 0x89, 0x01, 
    0x39, 0x45, 0x14, 0x73, 0x0A, 0xB8, 0x23, 0x00, 0x00, 0xC0, 0xE9, 0x59, 0x01, 0x00, 0x00, 0x8B, 
    0x4D, 0x0C, 0x89, 0x01, 0x8B, 0x45, 0x10, 0x8B, 0x0D, 0x9C, 0xFB, 0x04, 0x80, 0x89, 0x08, 0x33, 
    0xC0, 0xE9, 0x42, 0x01, 0x00, 0x00, 
};
static BYTE OriginalData[57]=
{
  0x55,0x8B,0xEC,0x81,0xEC,0x04,0x01,0x00,0x00,0x8B,0x45,0x08,0x3D,0x04,0x01,0x00,
  0x00,0x53,0x75,0x32,0x8B,0x4D,0x18,0x85,0xC9,0x6A,0x04,0x58,0x74,0x02,0x89,0x01,
  0x39,0x45,0x14,0x73,0x0A,0xB8,0x23,0x00,0x00,0xC0,0xE9,0x59,0x01,0x00,0x00,0x8B,
  0x4D,0x0C,0x89,0x01,0x8B,0x45,0x10,0x8B,0x0D
};

static BYTE PatchData[70]=
{
  0x55,0x8B,0xEC,0xB9,0x04,0x01,0x00,0x00,0x2B,0xE1,0x8B,0x45,0x08,0x53,0x3B,0xC1,
  0x74,0x0C,0x49,0x3B,0xC1,0x75,0x2F,0xB8,0x00,0x03,0x80,0x00,0xEB,0x05,0xB8,0x04,
  0x00,0x00,0x00,0x50,0x8B,0x4D,0x18,0x6A,0x04,0x58,0x85,0xC9,0x74,0x02,0x89,0x01,
  0x8B,0x4D,0x0C,0x89,0x01,0x59,0x8B,0x45,0x10,0x89,0x08,0x33,0xC0,0x5B,0xC9,0xC2,
  0x14,0x00,0x00,0x00,0x00,0x00
};


// for trainers
#define KERNEL_STORE_ADDRESS 0x8000000C // this is address in kernel we store the address of our allocated memory block
#define KERNEL_START_ADDRESS 0x80010000 // base addy of kernel
#define KERNEL_ALLOCATE_ADDRESS 0x7FFD2200 // where we want to put our allocated memory block (under kernel so it works retail)
#define KERNEL_SEARCH_RANGE 0x02AF90 // used for loop control base + search range to look xbe entry point bytes

#define XBTF_HEAP_SIZE 15360 // plenty of room for trainer + xbtf support functions
#define ETM_HEAP_SIZE 2400  // just enough room to match evox's etm spec limit (no need to give them more room then evox does)
// magic kernel patch (asm included w/ source)
static unsigned char trainerloaderdata[167] =
{
       0x60, 0xBA, 0x34, 0x12, 0x00, 0x00, 0x60, 0x6A, 0x01, 0x6A, 0x07, 0xE8, 0x67, 0x00, 0x00, 0x00,
       0x6A, 0x0C, 0x6A, 0x08, 0xE8, 0x5E, 0x00, 0x00, 0x00, 0x61, 0x8B, 0x35, 0x18, 0x01, 0x01, 0x00,
       0x83, 0xC6, 0x08, 0x8B, 0x06, 0x8B, 0x72, 0x12, 0x03, 0xF2, 0xB9, 0x03, 0x00, 0x00, 0x00, 0x3B,
       0x06, 0x74, 0x0C, 0x83, 0xC6, 0x04, 0xE2, 0xF7, 0x68, 0xF0, 0x00, 0x00, 0x00, 0xEB, 0x29, 0x8B,
       0xEA, 0x83, 0x7A, 0x1A, 0x00, 0x74, 0x05, 0x8B, 0x4A, 0x1A, 0xEB, 0x03, 0x8B, 0x4A, 0x16, 0x03,
       0xCA, 0x0F, 0x20, 0xC0, 0x50, 0x25, 0xFF, 0xFF, 0xFE, 0xFF, 0x0F, 0x22, 0xC0, 0xFF, 0xD1, 0x58,
       0x0F, 0x22, 0xC0, 0x68, 0xFF, 0x00, 0x00, 0x00, 0x6A, 0x08, 0xE8, 0x08, 0x00, 0x00, 0x00, 0x61,
       0xFF, 0x15, 0x28, 0x01, 0x01, 0x00, 0xC3, 0x55, 0x8B, 0xEC, 0x66, 0xBA, 0x04, 0xC0, 0xB0, 0x20,
       0xEE, 0x66, 0xBA, 0x08, 0xC0, 0x8A, 0x45, 0x08, 0xEE, 0x66, 0xBA, 0x06, 0xC0, 0x8A, 0x45, 0x0C,
       0xEE, 0xEE, 0x66, 0xBA, 0x02, 0xC0, 0xB0, 0x1A, 0xEE, 0x50, 0xB8, 0x40, 0x42, 0x0F, 0x00, 0x48,
       0x75, 0xFD, 0x58, 0xC9, 0xC2, 0x08, 0x00,
};

#define SIZEOFLOADERDATA 167// loaderdata is our kernel hack to handle if trainer (com file) is executed for title about to run

static unsigned char trainerdata[XBTF_HEAP_SIZE] = { NULL }; // buffer to hold trainer in mem - needs to be global?
// SM_Bus function for xbtf trainers
static unsigned char sm_bus[48] =
{
	0x55, 0x8B, 0xEC, 0x66, 0xBA, 0x04, 0xC0, 0xB0, 0x20, 0xEE, 0x66, 0xBA, 0x08, 0xC0, 0x8A, 0x45, 
	0x08, 0xEE, 0x66, 0xBA, 0x06, 0xC0, 0x8A, 0x45, 0x0C, 0xEE, 0xEE, 0x66, 0xBA, 0x02, 0xC0, 0xB0, 
	0x1A, 0xEE, 0x50, 0xB8, 0x40, 0x42, 0x0F, 0x00, 0x48, 0x75, 0xFD, 0x58, 0xC9, 0xC2, 0x08, 0x00, 
};
 
// PatchIt dynamic patching
static unsigned char patch_it_toy[33] =
{
	0x55, 0x8B, 0xEC, 0x60, 0x0F, 0x20, 0xC0, 0x50, 0x25, 0xFF, 0xFF, 0xFE, 0xFF, 0x0F, 0x22, 0xC0, 
	0x8B, 0x4D, 0x0C, 0x8B, 0x55, 0x08, 0x89, 0x0A, 0x58, 0x0F, 0x22, 0xC0, 0x61, 0xC9, 0xC2, 0x08, 
	0x00, 
};

// HookIt
static unsigned char hookit_toy[43] =
{
	0x55, 0x8B, 0xEC, 0x60, 0x8B, 0x55, 0x08, 0x8B, 0x45, 0x0C, 0xBD, 0x0C, 0x00, 0x00, 0x80, 0x8B, 
	0x6D, 0x00, 0x83, 0xC5, 0x02, 0x8B, 0x6D, 0x00, 0x8D, 0x44, 0x05, 0x00, 0xC6, 0x02, 0x68, 0x89, 
	0x42, 0x01, 0xC6, 0x42, 0x05, 0xC3, 0x61, 0xC9, 0xC2, 0x08, 0x00, 
};

// in game keys (main)
static unsigned char igk_main_toy[403] =
{
	0x81, 0x3D, 0x4C, 0x01, 0x01, 0x00, 0xA4, 0x01, 0x00, 0x00, 0x74, 0x30, 0xC7, 0x05, 0x4C, 0x01, 
	0x01, 0x00, 0xA4, 0x01, 0x00, 0x00, 0xC7, 0x05, 0x14, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 
	0x60, 0xBA, 0x50, 0x01, 0x01, 0x00, 0xB9, 0x05, 0x00, 0x00, 0x00, 0xC7, 0x02, 0x00, 0x00, 0x00, 
	0x00, 0x83, 0xC2, 0x04, 0xE2, 0xF5, 0x61, 0xE9, 0x4B, 0x01, 0x00, 0x00, 0x51, 0x8D, 0x4A, 0x08, 
	0x60, 0x33, 0xC0, 0x8A, 0x41, 0x0C, 0x50, 0x8D, 0x15, 0x14, 0x00, 0x00, 0x80, 0x8B, 0x0A, 0x89, 
	0x42, 0x04, 0x3B, 0xC1, 0x0F, 0x84, 0x1F, 0x01, 0x00, 0x00, 0x66, 0x25, 0x81, 0x00, 0x66, 0x3D, 
	0x81, 0x00, 0x75, 0x0C, 0x80, 0x35, 0x51, 0x01, 0x01, 0x00, 0x01, 0xE9, 0x09, 0x01, 0x00, 0x00, 
	0x58, 0x50, 0x66, 0x25, 0x82, 0x00, 0x66, 0x3D, 0x82, 0x00, 0x75, 0x0C, 0x80, 0x35, 0x52, 0x01, 
	0x01, 0x00, 0x01, 0xE9, 0xF1, 0x00, 0x00, 0x00, 0x58, 0x50, 0x66, 0x25, 0x84, 0x00, 0x66, 0x3D, 
	0x84, 0x00, 0x75, 0x0C, 0x80, 0x35, 0x53, 0x01, 0x01, 0x00, 0x01, 0xE9, 0xD9, 0x00, 0x00, 0x00, 
	0x58, 0x50, 0x66, 0x25, 0x88, 0x00, 0x66, 0x3D, 0x88, 0x00, 0x75, 0x0C, 0x80, 0x35, 0x54, 0x01, 
	0x01, 0x00, 0x01, 0xE9, 0xC1, 0x00, 0x00, 0x00, 0x58, 0x50, 0x66, 0x83, 0xE0, 0x41, 0x66, 0x83, 
	0xF8, 0x41, 0x75, 0x0C, 0x80, 0x35, 0x55, 0x01, 0x01, 0x00, 0x01, 0xE9, 0xA9, 0x00, 0x00, 0x00, 
	0x58, 0x50, 0x66, 0x83, 0xE0, 0x42, 0x66, 0x83, 0xF8, 0x42, 0x75, 0x0C, 0x80, 0x35, 0x56, 0x01, 
	0x01, 0x00, 0x01, 0xE9, 0x91, 0x00, 0x00, 0x00, 0x58, 0x50, 0x66, 0x83, 0xE0, 0x44, 0x66, 0x83, 
	0xF8, 0x44, 0x75, 0x09, 0x80, 0x35, 0x57, 0x01, 0x01, 0x00, 0x01, 0xEB, 0x7C, 0x58, 0x50, 0x66, 
	0x83, 0xE0, 0x48, 0x66, 0x83, 0xF8, 0x48, 0x75, 0x09, 0x80, 0x35, 0x58, 0x01, 0x01, 0x00, 0x01, 
	0xEB, 0x67, 0x58, 0x50, 0x66, 0x25, 0xC0, 0x00, 0x66, 0x3D, 0xC0, 0x00, 0x75, 0x09, 0x80, 0x35, 
	0x59, 0x01, 0x01, 0x00, 0x01, 0xEB, 0x52, 0x58, 0x50, 0x66, 0x83, 0xE0, 0x60, 0x66, 0x83, 0xF8, 
	0x60, 0x75, 0x09, 0x80, 0x35, 0x5A, 0x01, 0x01, 0x00, 0x01, 0xEB, 0x3D, 0x58, 0x50, 0x66, 0x83, 
	0xE0, 0x50, 0x66, 0x83, 0xF8, 0x50, 0x75, 0x09, 0x80, 0x35, 0x5B, 0x01, 0x01, 0x00, 0x01, 0xEB, 
	0x28, 0x58, 0x50, 0x66, 0x25, 0xA0, 0x00, 0x66, 0x3D, 0xA0, 0x00, 0x75, 0x09, 0x80, 0x35, 0x5C, 
	0x01, 0x01, 0x00, 0x01, 0xEB, 0x13, 0x58, 0x50, 0x66, 0x25, 0x90, 0x00, 0x66, 0x3D, 0x90, 0x00, 
	0x75, 0x07, 0x80, 0x35, 0x5D, 0x01, 0x01, 0x00, 0x01, 0x58, 0x8D, 0x15, 0x14, 0x00, 0x00, 0x80, 
	0x8B, 0x42, 0x04, 0x89, 0x02, 0x61, 0x59, 0x5B, 0xB9, 0x10, 0x00, 0x00, 0x80, 0xFF, 0x11, 0x53, 
	0x33, 0xDB, 0xC3, 
};

// HOOKIGK (moves stock pad call and patches game igk_main to use correct register)
static unsigned char hook_igk_toy[76] =
{
	0x55, 0x8B, 0xEC, 0x60, 0xBA, 0x34, 0x12, 0x00, 0x00, 0x8B, 0x45, 0x08, 0xB9, 0x20, 0x00, 0x00, 
	0x00, 0x8A, 0x18, 0x80, 0xFB, 0x50, 0x7C, 0x07, 0x80, 0xFB, 0x53, 0x7F, 0x02, 0xEB, 0x05, 0x48, 
	0xE2, 0xEF, 0xEB, 0x23, 0x80, 0xEB, 0x08, 0x88, 0x5A, 0x3E, 0x83, 0x45, 0x08, 0x01, 0x8B, 0x45, 
	0x08, 0x50, 0x03, 0x00, 0x83, 0xC0, 0x04, 0x2B, 0x55, 0x08, 0x83, 0xEA, 0x04, 0xBB, 0x10, 0x00, 
	0x00, 0x80, 0x89, 0x03, 0x58, 0x89, 0x10, 0x61, 0xC9, 0xC2, 0x04, 0x00, 
};

// SmartXX / Aladin4 functions
static unsigned char lcd_toy_xx[378] =
{
	0x55, 0x8B, 0xEC, 0x90, 0x66, 0x8B, 0x55, 0x08, 0x90, 0x8A, 0x45, 0x0C, 0x90, 0xEE, 0x90, 0x90, 
	0xC9, 0xC2, 0x08, 0x00, 0x55, 0x8B, 0xEC, 0x60, 0x33, 0xC0, 0x33, 0xDB, 0x0F, 0xB6, 0x45, 0x08, 
	0xC1, 0xF8, 0x02, 0x83, 0xE0, 0x28, 0xA2, 0x40, 0x01, 0x01, 0x00, 0x0F, 0xB6, 0x45, 0x08, 0x83, 
	0xE0, 0x50, 0x0F, 0xB6, 0x0D, 0x40, 0x01, 0x01, 0x00, 0x0B, 0xC8, 0x88, 0x0D, 0x40, 0x01, 0x01, 
	0x00, 0x0F, 0xB6, 0x45, 0x08, 0xC1, 0xE0, 0x02, 0x83, 0xE0, 0x28, 0xA2, 0x41, 0x01, 0x01, 0x00, 
	0x0F, 0xB6, 0x45, 0x08, 0xC1, 0xE0, 0x04, 0x83, 0xE0, 0x50, 0x0F, 0xB6, 0x0D, 0x41, 0x01, 0x01, 
	0x00, 0x0B, 0xC8, 0x88, 0x0D, 0x41, 0x01, 0x01, 0x00, 0x0F, 0xB6, 0x45, 0x0C, 0x83, 0xE0, 0x02, 
	0x0F, 0xB6, 0x0D, 0x40, 0x01, 0x01, 0x00, 0x0B, 0xC1, 0x50, 0x68, 0x00, 0xF7, 0x00, 0x00, 0xE8, 
	0x7C, 0xFF, 0xFF, 0xFF, 0x0F, 0xB6, 0x45, 0x0C, 0x83, 0xE0, 0x02, 0x83, 0xC8, 0x04, 0x0F, 0xB6, 
	0x0D, 0x40, 0x01, 0x01, 0x00, 0x0B, 0xC1, 0x50, 0x68, 0x00, 0xF7, 0x00, 0x00, 0xE8, 0x5E, 0xFF, 
	0xFF, 0xFF, 0x0F, 0xB6, 0x45, 0x0C, 0x83, 0xE0, 0x02, 0x0F, 0xB6, 0x0D, 0x40, 0x01, 0x01, 0x00, 
	0x0B, 0xC1, 0x50, 0x68, 0x00, 0xF7, 0x00, 0x00, 0xE8, 0x43, 0xFF, 0xFF, 0xFF, 0x0F, 0xB6, 0x45, 
	0x0C, 0x83, 0xE0, 0x01, 0x75, 0x6A, 0x0F, 0xB6, 0x45, 0x0C, 0x83, 0xE0, 0x02, 0x0F, 0xB6, 0x0D, 
	0x41, 0x01, 0x01, 0x00, 0x0B, 0xC1, 0x50, 0x68, 0x00, 0xF7, 0x00, 0x00, 0xE8, 0x1F, 0xFF, 0xFF, 
	0xFF, 0x0F, 0xB6, 0x45, 0x0C, 0x83, 0xE0, 0x02, 0x83, 0xC8, 0x04, 0x0F, 0xB6, 0x0D, 0x41, 0x01, 
	0x01, 0x00, 0x0B, 0xC1, 0x50, 0x68, 0x00, 0xF7, 0x00, 0x00, 0xE8, 0x01, 0xFF, 0xFF, 0xFF, 0x0F, 
	0xB6, 0x45, 0x0C, 0x83, 0xE0, 0x02, 0x0F, 0xB6, 0x0D, 0x41, 0x01, 0x01, 0x00, 0x0B, 0xC1, 0x50, 
	0x68, 0x00, 0xF7, 0x00, 0x00, 0xE8, 0xE6, 0xFE, 0xFF, 0xFF, 0x0F, 0xB6, 0x45, 0x08, 0x25, 0xFC, 
	0x00, 0x00, 0x00, 0x75, 0x0B, 0xF4, 0x90, 0x90, 0x90, 0x90, 0x90, 0xF4, 0x90, 0x90, 0x90, 0x90, 
	0xF4, 0x90, 0x90, 0x90, 0x90, 0x90, 0xF4, 0x90, 0x90, 0x90, 0x90, 0x90, 0x61, 0xC9, 0xC2, 0x08, 
	0x00, 0x6A, 0x00, 0x6A, 0x01, 0xE8, 0xCA, 0xFE, 0xFF, 0xFF, 0xC3, 0x55, 0x8B, 0xEC, 0x60, 0xB1, 
	0x80, 0x0A, 0x4D, 0x0C, 0x6A, 0x00, 0x8A, 0xC1, 0x50, 0xE8, 0xB6, 0xFE, 0xFF, 0xFF, 0x33, 0xC9, 
	0x8B, 0x55, 0x08, 0x8A, 0x04, 0x11, 0x3C, 0x00, 0x74, 0x0B, 0x6A, 0x02, 0x50, 0xE8, 0xA2, 0xFE, 
	0xFF, 0xFF, 0x41, 0xEB, 0xEE, 0x61, 0xC9, 0xC2, 0x08, 0x00, 
};

// X3 LCD functions
static unsigned char lcd_toy_x3[246] =
{
	0x55, 0x8B, 0xEC, 0x90, 0x66, 0x8B, 0x55, 0x08, 0x90, 0x8A, 0x45, 0x0C, 0x90, 0xEE, 0x90, 0x90, 
	0xC9, 0xC2, 0x08, 0x00, 0x55, 0x8B, 0xEC, 0x60, 0x50, 0x33, 0xC0, 0x8A, 0x45, 0x0C, 0x24, 0x02, 
	0x3C, 0x00, 0x74, 0x05, 0x32, 0xE4, 0x80, 0xCC, 0x01, 0x8A, 0x45, 0x08, 0x24, 0xF0, 0x50, 0x68, 
	0x04, 0xF5, 0x00, 0x00, 0xE8, 0xC7, 0xFF, 0xFF, 0xFF, 0x8A, 0xC4, 0x50, 0x68, 0x05, 0xF5, 0x00, 
	0x00, 0xE8, 0xBA, 0xFF, 0xFF, 0xFF, 0x50, 0x80, 0xCC, 0x04, 0x8A, 0xC4, 0x50, 0x68, 0x05, 0xF5, 
	0x00, 0x00, 0xE8, 0xA9, 0xFF, 0xFF, 0xFF, 0x58, 0x8A, 0xC4, 0x50, 0x68, 0x05, 0xF5, 0x00, 0x00, 
	0xE8, 0x9B, 0xFF, 0xFF, 0xFF, 0x8A, 0x45, 0x0C, 0x24, 0x01, 0x3C, 0x00, 0x75, 0x3D, 0x8A, 0x45, 
	0x08, 0xC0, 0xE0, 0x04, 0x50, 0x68, 0x04, 0xF5, 0x00, 0x00, 0xE8, 0x81, 0xFF, 0xFF, 0xFF, 0x8A, 
	0xC4, 0x50, 0x68, 0x05, 0xF5, 0x00, 0x00, 0xE8, 0x74, 0xFF, 0xFF, 0xFF, 0x50, 0x80, 0xCC, 0x04, 
	0x8A, 0xC4, 0x50, 0x68, 0x05, 0xF5, 0x00, 0x00, 0xE8, 0x63, 0xFF, 0xFF, 0xFF, 0x58, 0x8A, 0xC4, 
	0x50, 0x68, 0x05, 0xF5, 0x00, 0x00, 0xE8, 0x55, 0xFF, 0xFF, 0xFF, 0x58, 0xF4, 0x90, 0x90, 0x90, 
	0x90, 0x90, 0xF4, 0x90, 0x90, 0x90, 0x90, 0x90, 0x61, 0xC9, 0xC2, 0x08, 0x00, 0x6A, 0x00, 0x6A, 
	0x01, 0xE8, 0x4E, 0xFF, 0xFF, 0xFF, 0xC3, 0x55, 0x8B, 0xEC, 0x60, 0xB1, 0x80, 0x0A, 0x4D, 0x0C, 
	0x6A, 0x00, 0x8A, 0xC1, 0x50, 0xE8, 0x3A, 0xFF, 0xFF, 0xFF, 0x33, 0xC9, 0x8B, 0x55, 0x08, 0x8A, 
	0x04, 0x11, 0x3C, 0x00, 0x74, 0x0B, 0x6A, 0x02, 0x50, 0xE8, 0x26, 0xFF, 0xFF, 0xFF, 0x41, 0xEB, 
	0xEE, 0x61, 0xC9, 0xC2, 0x08, 0x00, 
};
#endif


CUtil::CUtil(void)
{
}

CUtil::~CUtil(void)
{}

CStdString CUtil::GetTitleFromPath(const CStdString& strFileNameAndPath, bool bIsFolder /* = false */)
{
  // use above to get the filename
  CStdString path(strFileNameAndPath);
  URIUtils::RemoveSlashAtEnd(path);
  CStdString strFilename = URIUtils::GetFileName(path);

  CURL url(strFileNameAndPath);
  CStdString strHostname = url.GetHostName();

#ifdef HAS_UPNP
  // UPNP
  if (url.GetProtocol() == "upnp")
    strFilename = CUPnPDirectory::GetFriendlyName(strFileNameAndPath.c_str());
#endif

  if (url.GetProtocol() == "rss")
  {
    url.SetProtocol("http");
    path = url.Get();
    CRssFeed feed;
    feed.Init(path);
    feed.ReadFeed();
    strFilename = feed.GetFeedTitle();
  }

  // LastFM
  if (url.GetProtocol() == "lastfm")
  {
    if (strFilename.IsEmpty())
      strFilename = g_localizeStrings.Get(15200);
    else
      strFilename = g_localizeStrings.Get(15200) + " - " + strFilename;
  }

  // Shoutcast
  else if (url.GetProtocol() == "shout")
  {
    const int genre = strFileNameAndPath.find_first_of('=');
    if(genre <0)
      strFilename = g_localizeStrings.Get(260);
    else
      strFilename = g_localizeStrings.Get(260) + " - " + strFileNameAndPath.substr(genre+1).c_str();
  }

  // Windows SMB Network (SMB)
  else if (url.GetProtocol() == "smb" && strFilename.IsEmpty())
  {
    if (url.GetHostName().IsEmpty())
    {
      strFilename = g_localizeStrings.Get(20171);
    }
    else
    {
      strFilename = url.GetHostName();
    }
  }
  // XBMSP Network
  else if (url.GetProtocol() == "xbms" && strFilename.IsEmpty())
    strFilename = "XBMSP Network";

  // iTunes music share (DAAP)
  else if (url.GetProtocol() == "daap" && strFilename.IsEmpty())
    strFilename = g_localizeStrings.Get(20174);

  // HDHomerun Devices
  else if (url.GetProtocol() == "hdhomerun" && strFilename.IsEmpty())
    strFilename = "HDHomerun Devices";

  // Slingbox Devices
  else if (url.GetProtocol() == "sling")
    strFilename = "Slingbox";

  // ReplayTV Devices
  else if (url.GetProtocol() == "rtv")
    strFilename = "ReplayTV Devices";

  // HTS Tvheadend client
  else if (url.GetProtocol() == "htsp")
    strFilename = g_localizeStrings.Get(20256);

  // VDR Streamdev client
  else if (url.GetProtocol() == "vtp")
    strFilename = g_localizeStrings.Get(20257);
  
  // MythTV client
  else if (url.GetProtocol() == "myth")
    strFilename = g_localizeStrings.Get(20258);

  // SAP Streams
  else if (url.GetProtocol() == "sap" && strFilename.IsEmpty())
    strFilename = "SAP Streams";

  // Root file views
  else if (url.GetProtocol() == "sources")
    strFilename = g_localizeStrings.Get(744);

  // Music Playlists
  else if (path.Left(24).Equals("special://musicplaylists"))
    strFilename = g_localizeStrings.Get(136);

  // Video Playlists
  else if (path.Left(24).Equals("special://videoplaylists"))
    strFilename = g_localizeStrings.Get(136);

  else if ((url.GetProtocol() == "rar" || url.GetProtocol() == "zip") && strFilename.IsEmpty())
    strFilename = URIUtils::GetFileName(url.GetHostName());

  // now remove the extension if needed
  if (!g_guiSettings.GetBool("filelists.showextensions") && !bIsFolder)
  {
    URIUtils::RemoveExtension(strFilename);
    return strFilename;
  }
  
  // URLDecode since the original path may be an URL
  CURL::Decode(strFilename);
  return strFilename;
}

bool CUtil::GetVolumeFromFileName(const CStdString& strFileName, CStdString& strFileTitle, CStdString& strVolumeNumber)
{
  const CStdStringArray &regexps = g_advancedSettings.m_videoStackRegExps;

  CStdString strFileNameTemp = strFileName;
  CStdString strFileNameLower = strFileName;
  strFileNameLower.MakeLower();

  CStdString strVolume;
  CStdString strTestString;
  CRegExp reg;

//  CLog::Log(LOGDEBUG, "GetVolumeFromFileName:[%s]", strFileNameLower.c_str());
  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    CStdString strRegExp = regexps[i];
    if (!reg.RegComp(strRegExp.c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "Invalid RegExp:[%s]", regexps[i].c_str());
      continue;
    }
//    CLog::Log(LOGDEBUG, "Regexp:[%s]", regexps[i].c_str());

    int iFoundToken = reg.RegFind(strFileNameLower.c_str());
    if (iFoundToken >= 0)
    {
      int iRegLength = reg.GetFindLen();
      int iCount = reg.GetSubCount();

      /*
      reg.DumpOvector(LOGDEBUG);
      CLog::Log(LOGDEBUG, "Subcount=%i", iCount);
      for (int j = 0; j <= iCount; j++)
      {
        CStdString str = reg.GetMatch(j);
        CLog::Log(LOGDEBUG, "Sub(%i):[%s]", j, str.c_str());
      }
      */

      // simple regexp, only the volume is captured
      if (iCount == 1)
      {
        strVolumeNumber = reg.GetMatch(1);
        if (strVolumeNumber.IsEmpty()) return false;

        // Remove the extension (if any).  We do this on the base filename, as the regexp
        // match may include some of the extension (eg the "." in particular).
        // The extension will then be added back on at the end - there is no reason
        // to clean it off here. It will be cleaned off during the display routine, if
        // the settings to hide extensions are turned on.
        CStdString strFileNoExt = strFileNameTemp;
        URIUtils::RemoveExtension(strFileNoExt);
        CStdString strFileExt = strFileNameTemp.Right(strFileNameTemp.length() - strFileNoExt.length());
        CStdString strFileRight = strFileNoExt.Mid(iFoundToken + iRegLength);
        strFileTitle = strFileName.Left(iFoundToken) + strFileRight + strFileExt;

        return true;
      }

      // advanced regexp with prefix (1), volume (2), and suffix (3)
      else if (iCount == 3)
      {
        // second subpatten contains the stacking volume
        strVolumeNumber = reg.GetMatch(2);
        if (strVolumeNumber.IsEmpty()) return false;

        // everything before the regexp match
        strFileTitle = strFileName.Left(iFoundToken);

        // first subpattern contains prefix
        strFileTitle += reg.GetMatch(1);

        // third subpattern contains suffix
        strFileTitle += reg.GetMatch(3);

        // everything after the regexp match
        strFileTitle += strFileNameTemp.Mid(iFoundToken + iRegLength);

        return true;
      }

      // unknown regexp format
      else
      {
        CLog::Log(LOGERROR, "Incorrect movie stacking regexp format:[%s]", regexps[i].c_str());
      }
    }
  }
  return false;
}

void CUtil::CleanString(const CStdString& strFileName, CStdString& strTitle, CStdString& strTitleAndYear, CStdString& strYear, bool bRemoveExtension /* = false */, bool bCleanChars /* = true */)
{
  strTitleAndYear = strFileName;

  if (strFileName.Equals(".."))
   return;

  const CStdStringArray &regexps = g_advancedSettings.m_videoCleanStringRegExps;

  CRegExp reTags(true);
  CRegExp reYear;
  CStdString strExtension;
  URIUtils::GetExtension(strFileName, strExtension);

  if (!reYear.RegComp(g_advancedSettings.m_videoCleanDateTimeRegExp))
  {
    CLog::Log(LOGERROR, "%s: Invalid datetime clean RegExp:'%s'", __FUNCTION__, g_advancedSettings.m_videoCleanDateTimeRegExp.c_str());
  }
  else
  {
    if (reYear.RegFind(strTitleAndYear.c_str()) >= 0)
    {
      strTitleAndYear = reYear.GetReplaceString("\\1");
      strYear = reYear.GetReplaceString("\\2");
    }
  }

  URIUtils::RemoveExtension(strTitleAndYear);

  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    if (!reTags.RegComp(regexps[i].c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "%s: Invalid string clean RegExp:'%s'", __FUNCTION__, regexps[i].c_str());
      continue;
    }
    int j=0;
    if ((j=reTags.RegFind(strFileName.c_str())) > 0)
      strTitleAndYear = strTitleAndYear.Mid(0, j);
  }

  // final cleanup - special characters used instead of spaces:
  // all '_' tokens should be replaced by spaces
  // if the file contains no spaces, all '.' tokens should be replaced by
  // spaces - one possibility of a mistake here could be something like:
  // "Dr..StrangeLove" - hopefully no one would have anything like this.
  if (bCleanChars)
  {
    bool initialDots = true;
    bool alreadyContainsSpace = (strTitleAndYear.Find(' ') >= 0);

    for (int i = 0; i < (int)strTitleAndYear.size(); i++)
    {
      char c = strTitleAndYear.GetAt(i);

      if (c != '.')
        initialDots = false;

      if ((c == '_') || ((!alreadyContainsSpace) && !initialDots && (c == '.')))
      {
        strTitleAndYear.SetAt(i, ' ');
      }
    }
  }

  strTitle = strTitleAndYear.Trim();

  // append year
  if (!strYear.IsEmpty())
    strTitleAndYear = strTitle + " (" + strYear + ")";

  // restore extension if needed
  if (!bRemoveExtension)
    strTitleAndYear += strExtension;
}

void CUtil::GetQualifiedFilename(const CStdString &strBasePath, CStdString &strFilename)
{
  //Make sure you have a full path in the filename, otherwise adds the base path before.
  CURL plItemUrl(strFilename);
  CURL plBaseUrl(strBasePath);
  int iDotDotLoc, iBeginCut, iEndCut;

  if (plBaseUrl.IsLocal()) //Base in local directory
  {
    if (plItemUrl.IsLocal() ) //Filename is local or not qualified
    {
#ifdef _LINUX
      if (!( (strFilename.c_str()[1] == ':') || (strFilename.c_str()[0] == '/') ) ) //Filename not fully qualified
#else
      if (!( strFilename.c_str()[1] == ':')) //Filename not fully qualified
#endif
      {
        if (strFilename.c_str()[0] == '/' || strFilename.c_str()[0] == '\\' || URIUtils::HasSlashAtEnd(strBasePath))
        {
          strFilename = strBasePath + strFilename;
          strFilename.Replace('/', '\\');
        }
        else
        {
          strFilename = strBasePath + '\\' + strFilename;
          strFilename.Replace('/', '\\');
        }
      }
    }
    strFilename.Replace("\\.\\", "\\");
    while ((iDotDotLoc = strFilename.Find("\\..\\")) > 0)
    {
      iEndCut = iDotDotLoc + 4;
      iBeginCut = strFilename.Left(iDotDotLoc).ReverseFind('\\') + 1;
      strFilename.Delete(iBeginCut, iEndCut - iBeginCut);
    }

    // This routine is only called from the playlist loaders,
    // where the filepath is in UTF-8 anyway, so we don't need
    // to do checking for FatX characters.
    //if (g_guiSettings.GetBool("services.ftpautofatx") && (URIUtils::IsHD(strFilename)))
    //  CUtil::GetFatXQualifiedPath(strFilename);
  }
  else //Base is remote
  {
    if (plItemUrl.IsLocal()) //Filename is local
    {
#ifdef _LINUX
      if ( (strFilename.c_str()[1] == ':') || (strFilename.c_str()[0] == '/') )  //Filename not fully qualified
#else
      if (strFilename[1] == ':') // already fully qualified
#endif
        return;
      if (strFilename.c_str()[0] == '/' || strFilename.c_str()[0] == '\\' || URIUtils::HasSlashAtEnd(strBasePath)) //Begins with a slash.. not good.. but we try to make the best of it..

      {
        strFilename = strBasePath + strFilename;
        strFilename.Replace('\\', '/');
      }
      else
      {
        strFilename = strBasePath + '/' + strFilename;
        strFilename.Replace('\\', '/');
      }
    }
    strFilename.Replace("/./", "/");
    while ((iDotDotLoc = strFilename.Find("/../")) > 0)
    {
      iEndCut = iDotDotLoc + 4;
      iBeginCut = strFilename.Left(iDotDotLoc).ReverseFind('/') + 1;
      strFilename.Delete(iBeginCut, iEndCut - iBeginCut);
    }
  }
}

bool CUtil::PatchCountryVideo(F_COUNTRY Country, F_VIDEO Video)
{
#ifdef HAS_XBOX_HARDWARE
  BYTE  *Kernel=(BYTE *)0x80010000;
  DWORD i, j = 0, k;
  DWORD *CountryPtr;
  BYTE  CountryValues[4]={0, 1, 2, 4};
  BYTE  VideoTyValues[5]={0, 1, 2, 3, 3};
  BYTE  VideoFrValues[5]={0x00, 0x40, 0x40, 0x80, 0x40};

  // Skip if no change is necessary...
  // That is to avoid a situation in which our Patch *and* the EvoX patch are installed
  // Otherwise the Infinite-Reboot-Patch does not work anymore!
  if(Video == XGetVideoStandard())
    return true;

  switch (Country)
  {
    case COUNTRY_EUR:
      if (!Video)
          Video = VIDEO_PAL50;
        break;
      case COUNTRY_USA:
        Video = VIDEO_NTSCM;
      Country = COUNTRY_USA;
        break;
      case COUNTRY_JAP:
        Video = VIDEO_NTSCJ;
      Country = COUNTRY_JAP;
        break;
      default:
      Country = COUNTRY_EUR;
        Video = VIDEO_PAL50;
  };

  // Search for the original code in the Kernel.
  // Searching from 0x80011000 to 0x80024000 in order that this will work on as many Kernels
  // as possible.

  for(i=0x1000; i<0x14000; i++)
  {
    if(Kernel[i]!=OriginalData[0])
	    continue;

    for(j=0; j<57; j++)
    {
	    if(Kernel[i+j]!=OriginalData[j])
		    break;
    }
    if(j==57)
	    break;
  }

  if(j==57)
  {
    // Ok, found the code to patch. Get pointer to original Country setting.
    // This may not be strictly neccessary, but lets do it anyway for completeness.

    j=(Kernel[i+57])+(Kernel[i+58]<<8)+(Kernel[i+59]<<16)+(Kernel[i+60]<<24);
    CountryPtr=(DWORD *)j;
  }
  else
  {
    // Did not find code in the Kernel. Check if my patch is already there.

    for(i=0x1000; i<0x14000; i++)
    {
      if(Kernel[i]!=PatchData[0])
        continue;

      for(j=0; j<25; j++)
      {
        if(Kernel[i+j]!=PatchData[j])
          break;
      }
      if(j==25)
        break;
    }

    if(j==25)
    {
      // Ok, found my patch. Get pointer to original Country setting.
      // This may not be strictly neccessary, but lets do it anyway for completeness.

      j=(Kernel[i+66])+(Kernel[i+67]<<8)+(Kernel[i+68]<<16)+(Kernel[i+69]<<24);
      CountryPtr=(DWORD *)j;
    }
    else
    {
      // Did not find my patch - so I can't work with this BIOS. Exit.
      return( false );
    }
  }

  // Patch in new code.

  j=MmQueryAddressProtect(&Kernel[i]);
  MmSetAddressProtect(&Kernel[i], 70, PAGE_READWRITE);

  memcpy(&Kernel[i], &PatchData[0], 70);

  // Patch Success. Fix up values.

  *CountryPtr=(DWORD)CountryValues[Country];
  Kernel[i+0x1f]=CountryValues[Country];
  Kernel[i+0x19]=VideoTyValues[Video];
  Kernel[i+0x1a]=VideoFrValues[Video];

  k=(DWORD)CountryPtr;
  Kernel[i+66]=(BYTE)(k&0xff);
  Kernel[i+67]=(BYTE)((k>>8)&0xff);
  Kernel[i+68]=(BYTE)((k>>16)&0xff);
  Kernel[i+69]=(BYTE)((k>>24)&0xff);

  MmSetAddressProtect(&Kernel[i], 70, j);

#endif
  // All Done!
  return( true );
}

bool CUtil::InstallTrainer(CTrainer& trainer)
{
  bool Found = false;
#ifdef HAS_XBOX_HARDWARE
  unsigned char *xboxkrnl = (unsigned char *)KERNEL_START_ADDRESS;
  unsigned char *hackptr = (unsigned char *)KERNEL_STORE_ADDRESS;
  void *ourmemaddr = NULL; // pointer used to allocated trainer mem
  unsigned int i = 0;
  DWORD memsize;

  CLog::Log(LOGDEBUG,"installing trainer %s",trainer.GetPath().c_str());

  if (trainer.IsXBTF()) // size of our allocation buffer for trainer
    memsize = XBTF_HEAP_SIZE;
  else
    memsize = ETM_HEAP_SIZE;

  unsigned char xbe_entry_point[] = {0xff,0x15,0x28,0x01,0x01,0x00}; // xbe entry point bytes in kernel
  unsigned char evox_tsr_hook[] = {0xff,0x15,0x10,0x00,0x00,0x80}; // check for evox's evil tsr hook

  for(i = 0; i < KERNEL_SEARCH_RANGE; i++)
  {
    if (memcmp(&xboxkrnl[i], xbe_entry_point, sizeof(xbe_entry_point)) == 0 ||
      memcmp(&xboxkrnl[i], evox_tsr_hook, sizeof(evox_tsr_hook)) == 0)
    {
      Found = true;
      break;
    }
  }

  if(Found)
  {
    unsigned char *patchlocation = xboxkrnl;

    patchlocation += i + 2; // adjust to xbe entry point bytes in kernel (skipping actual call opcodes)
    _asm
    {
      pushad

      mov eax, cr0
      push eax
      and eax, 0FFFEFFFFh
      mov cr0, eax // disable memory write prot

      mov	edi, patchlocation // address of call to xbe entry point in kernel
      mov	dword ptr [edi], KERNEL_STORE_ADDRESS // patch with address of where we store loaderdata+trainer buffer address

      pop eax
      mov cr0, eax // restore memory write prot

      popad
    }
  }
  else
  {
    __asm // recycle check
    {
      pushad

      mov edx, KERNEL_STORE_ADDRESS
      mov ecx, DWORD ptr [edx]
      cmp ecx, 0 // just in case :)
      jz cleanup

      cmp word ptr [ecx], 0BA60h // address point to valid loaderdata?
      jnz cleanup

      mov Found, 1 // yes! flag it found

      push ecx
      call MmFreeContiguousMemory // release old memory
cleanup:
      popad
    }
  }

  // allocate our memory space BELOW the kernel (so we can access buffer from game's scope)
  // if you allocate above kernel our buffer is out of scope and only debug bio will allow
  // game to access it
  ourmemaddr = MmAllocateContiguousMemoryEx(memsize, 0, -1, KERNEL_ALLOCATE_ADDRESS, PAGE_NOCACHE | PAGE_READWRITE);
  if ((DWORD)ourmemaddr > 0)
  {
    MmPersistContiguousMemory(ourmemaddr, memsize, true); // so we survive soft boots
    memcpy(hackptr, &ourmemaddr, 4); // store location of ourmemaddr in kernel

    memset(ourmemaddr, 0xFF, memsize); // init trainer buffer
    memcpy(ourmemaddr, trainerloaderdata, sizeof(trainerloaderdata)); // copy loader data (actual kernel hack)

    // patch loaderdata with trainer base address
    _asm
    {
      pushad

      mov eax, ourmemaddr
      mov ebx, eax
      add ebx, SIZEOFLOADERDATA
      mov dword ptr [eax+2], ebx

      popad
    }

		// adjust ourmemaddr pointer past loaderdata
		ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(trainerloaderdata));

		// copy our trainer data into allocated mem
		memcpy(ourmemaddr, trainer.data(), trainer.Size());

    if (trainer.IsXBTF())
    {
      DWORD dwSection = 0;

      // get address of XBTF_Section
      _asm
      {
        pushad

        mov eax, ourmemaddr

        cmp dword ptr [eax+0x1A], 0 // real xbtf or just a converted etm? - XBTF_ENTRYPOINT
        je converted_etm

        push eax
        mov ebx, 0x16
        add eax, ebx
        mov ecx, DWORD PTR [eax]
        pop	eax
        add eax, ecx
        mov dwSection, eax // get address of xbtf_section

      converted_etm:
        popad
      }

      if (dwSection == 0)
        return Found; // its a converted etm so we do not have toys section :)

      // adjust past trainer
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + trainer.Size());

      // inject SMBus code
      memcpy(ourmemaddr, sm_bus, sizeof(sm_bus));
      _asm
      {
        pushad

        mov eax, dwSection
        mov ebx, ourmemaddr
        cmp dword ptr [eax], 0
        jne nosmbus
        mov DWORD PTR [eax], ebx
      nosmbus:
        popad
      }
      // adjust past SMBus
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(sm_bus));

      // PatchIt
      memcpy(ourmemaddr, patch_it_toy, sizeof(patch_it_toy));
      _asm
      {
        pushad

        mov eax, dwSection
        add eax, 4 // 2nd dword in XBTF_Section
        mov ebx, ourmemaddr
        cmp dword PTR [eax], 0
        jne nopatchit
        mov DWORD PTR [eax], ebx
      nopatchit:
        popad
      }

      // adjust past PatchIt
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(patch_it_toy));

      // HookIt
      memcpy(ourmemaddr, hookit_toy, sizeof(hookit_toy));
      _asm
      {
        pushad

        mov eax, dwSection
        add eax, 8 // 3rd dword in XBTF_Section
        mov ebx, ourmemaddr
        cmp dword PTR [eax], 0
        jne nohookit
        mov DWORD PTR [eax], ebx
      nohookit:
        popad
      }

      // adjust past HookIt
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(hookit_toy));

      // igk_main_toy
      memcpy(ourmemaddr, igk_main_toy, sizeof(igk_main_toy));
      _asm
      {
        // patch hook_igk_toy w/ address
        pushad

        mov edx, offset hook_igk_toy
        add edx, 5
        mov ecx, ourmemaddr
        mov dword PTR [edx], ecx

        popad
      }

      // adjust past igk_main_toy
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(igk_main_toy));

      // hook_igk_toy
      memcpy(ourmemaddr, hook_igk_toy, sizeof(hook_igk_toy));
      _asm
      {
        pushad

        mov eax, dwSection
        add eax, 0ch // 4th dword in XBTF_Section
        mov ebx, ourmemaddr
        cmp dword PTR [eax], 0
        jne nohookigk
        mov DWORD PTR [eax], ebx
      nohookigk:
        popad
      }
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(igk_main_toy));

      if (g_guiSettings.GetInt("lcd.mode") > 0 && g_guiSettings.GetInt("lcd.type") == MODCHIP_SMARTXX)
      {
        memcpy(ourmemaddr, lcd_toy_xx, sizeof(lcd_toy_xx));
        _asm
        {
          pushad

          mov ecx, ourmemaddr
          add ecx, 0141h // lcd clear

          mov eax, dwSection
          add eax, 010h // 5th dword

          cmp dword PTR [eax], 0
          jne nolcdxx

          mov dword PTR [eax], ecx
          add ecx, 0ah // lcd writestring
          add eax, 4 // 6th dword

          cmp dword ptr [eax], 0
          jne nolcd

          mov dword ptr [eax], ecx
        nolcdxx:
          popad
        }
        ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(lcd_toy_xx));
      }
      else
      {
        // lcd toy
        memcpy(ourmemaddr, lcd_toy_x3, sizeof(lcd_toy_x3));
        _asm
        {
          pushad

          mov ecx, ourmemaddr
          add ecx, 0bdh // lcd clear

          mov eax, dwSection
          add eax, 010h // 5th dword

          cmp dword PTR [eax], 0
          jne nolcd

          mov dword PTR [eax], ecx
          add ecx, 0ah // lcd writestring
          add eax, 4 // 6th dword

          cmp dword ptr [eax], 0
          jne nolcd

          mov dword ptr [eax], ecx
        nolcd:
          popad
        }
        ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(lcd_toy_x3));
      }
    }
  }
#endif

	return Found;
}

bool CUtil::RemoveTrainer()
{
	bool Found = false;
#ifdef HAS_XBOX_HARDWARE
  unsigned char *xboxkrnl = (unsigned char *)KERNEL_START_ADDRESS;
	unsigned int i = 0;

  unsigned char xbe_entry_point[] = {0xff,0x15,0x80,0x00,0x00,0x0c}; // xbe entry point bytes in kernel
  *((DWORD*)(xbe_entry_point+2)) = KERNEL_STORE_ADDRESS;

	for(i = 0; i < KERNEL_SEARCH_RANGE; i++)
	{
		if (memcmp(&xboxkrnl[i], xbe_entry_point, 6) == 0)
		{
			Found = true;
			break;
		}
	}

	if(Found)
  {
    unsigned char *patchlocation = xboxkrnl;
    patchlocation += i + 2; // adjust to xbe entry point bytes in kernel (skipping actual call opcodes)
    __asm // recycle check
    {
        pushad

        mov eax, cr0
        push eax
        and eax, 0FFFEFFFFh
        mov cr0, eax // disable memory write prot

        mov	edi, patchlocation // address of call to xbe entry point in kernel
        mov	dword ptr [edi], 0x00010128 // patch with address of where we store loaderdata+trainer buffer address

        pop eax
        mov cr0, eax // restore memory write prot

        popad
    }
  }
#endif
  return Found;
}

bool CUtil::IsWritable(const CStdString& strFile)
{
#ifdef HAS_XBOX_HARDWARE
 if (strFile.substr(0,4) == "mem:")
 {
   return g_memoryUnitManager.IsDriveWriteable(strFile);
 }
#endif
  return ( URIUtils::IsHD(strFile) || URIUtils::IsSmb(strFile) ) && !URIUtils::IsDVD(strFile);
}

bool CUtil::IsPicture(const CStdString& strFile)
{
  CStdString extension = URIUtils::GetExtension(strFile);

  if (extension.IsEmpty())
    return false;

  extension.ToLower();
  if (g_settings.m_pictureExtensions.Find(extension) != -1)
    return true;

  if (extension == ".tbn" || extension == ".dds")
    return true;

  return false;
}

bool CUtil::ExcludeFileOrFolder(const CStdString& strFileOrFolder, const CStdStringArray& regexps)
{
  if (strFileOrFolder.IsEmpty())
    return false;

  CStdString strExclude = strFileOrFolder;
  strExclude.MakeLower();

  CRegExp regExExcludes;

  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    if (!regExExcludes.RegComp(regexps[i].c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "%s: Invalid exclude RegExp:'%s'", __FUNCTION__, regexps[i].c_str());
      continue;
    }
    if (regExExcludes.RegFind(strExclude) > -1)
    {
      CLog::Log(LOGDEBUG, "%s: File '%s' excluded. (Matches exclude rule RegExp:'%s')", __FUNCTION__, strExclude.c_str(), regexps[i].c_str());
      return true;
    }
  }
  return false;
}

void CUtil::GetFileAndProtocol(const CStdString& strURL, CStdString& strDir)
{
  strDir = strURL;
  if (!URIUtils::IsRemote(strURL)) return ;
  if (URIUtils::IsDVD(strURL)) return ;

  CURL url(strURL);
  strDir.Format("%s://%s", url.GetProtocol().c_str(), url.GetFileName().c_str());
}

int CUtil::GetDVDIfoTitle(const CStdString& strFile)
{
  CStdString strFilename = URIUtils::GetFileName(strFile);
  if (strFilename.Equals("video_ts.ifo")) return 0;
  //VTS_[TITLE]_0.IFO
  return atoi(strFilename.Mid(4, 2).c_str());
}

bool CUtil::CacheXBEIcon(const CStdString& strFilePath, const CStdString& strIcon)
{
  bool success(false);
  // extract icon from .xbe
  CStdString localFile;
  g_charsetConverter.utf8ToStringCharset(strFilePath, localFile);
  CXBE xbeReader;
  CStdString strTempFile;
  CStdString strExtension;

  URIUtils::AddFileToFolder(g_advancedSettings.m_cachePath,"1.xpr",strTempFile);
  URIUtils::GetExtension(strFilePath,strExtension);
  if (strExtension.Equals(".xbx"))
  {
  ::CopyFile(strFilePath.c_str(), strTempFile.c_str(),FALSE);
  }
  else
  {
  if (!xbeReader.ExtractIcon(localFile, strTempFile.c_str()))
    return false;
  }

  CXBPackedResource* pPackedResource = new CXBPackedResource();
  if ( SUCCEEDED( pPackedResource->Create( strTempFile.c_str(), 1, NULL ) ) )
  {
    LPDIRECT3DTEXTURE8 pTexture = pPackedResource->GetTexture((DWORD)0);
    if ( pTexture )
    {
      D3DSURFACE_DESC descSurface;
      if ( SUCCEEDED( pTexture->GetLevelDesc( 0, &descSurface ) ) )
      {
        int iHeight = descSurface.Height;
        int iWidth = descSurface.Width;
        DWORD dwFormat = descSurface.Format;
        CPicture pic;
        success = pic.CreateThumbnailFromSwizzledTexture(pTexture, iHeight, iWidth, strIcon.c_str());
      }
      pTexture->Release();
    }
  }
  delete pPackedResource;
  CFile::Delete(strTempFile);
  return success;
}

bool CUtil::GetDirectoryName(const CStdString& strFileName, CStdString& strDescription)
{
  CStdString strFName = URIUtils::GetFileName(strFileName);
  strDescription = strFileName.Left(strFileName.size() - strFName.size());
  URIUtils::RemoveSlashAtEnd(strDescription);

  int iPos = strDescription.ReverseFind("\\");
  if (iPos < 0)
    iPos = strDescription.ReverseFind("/");
  if (iPos >= 0)
  {
    CStdString strTmp = strDescription.Right(strDescription.size()-iPos-1);
    strDescription = strTmp;//strDescription.Right(strDescription.size() - iPos - 1);
  }
  else if (strDescription.size() <= 0)
    strDescription = strFName;
  return true;
}

bool CUtil::GetXBEDescription(const CStdString& strFileName, CStdString& strDescription)
{
  _XBE_CERTIFICATE HC;
  _XBE_HEADER HS;

  FILE* hFile = fopen(strFileName.c_str(), "rb");
  if (!hFile)
  {
    strDescription = URIUtils::GetFileName(strFileName);
    return false;
  }
  fread(&HS, 1, sizeof(HS), hFile);
  fseek(hFile, HS.XbeHeaderSize, SEEK_SET);
  fread(&HC, 1, sizeof(HC), hFile);
  fclose(hFile);

  // The XBE title is stored in WCHAR (UTF16) format

  // XBE titles can in fact use all 40 characters available to them,
  // and thus are not necessarily NULL terminated
  WCHAR TitleName[41];
  wcsncpy(TitleName, HC.TitleName, 40);
  TitleName[40] = 0;
  if (wcslen(TitleName) > 0)
  {
    g_charsetConverter.wToUTF8(TitleName, strDescription);
    return true;
  }
  strDescription = URIUtils::GetFileName(strFileName);
  return false;
}

bool CUtil::SetXBEDescription(const CStdString& strFileName, const CStdString& strDescription)
{
  _XBE_CERTIFICATE HC;
  _XBE_HEADER HS;

  FILE* hFile = fopen(strFileName.c_str(), "r+b");
  fread(&HS, 1, sizeof(HS), hFile);
  fseek(hFile, HS.XbeHeaderSize, SEEK_SET);
  fread(&HC, 1, sizeof(HC), hFile);
  fseek(hFile,HS.XbeHeaderSize, SEEK_SET);

  // The XBE title is stored in WCHAR (UTF16)

  CStdStringW shortDescription;
  g_charsetConverter.utf8ToW(strDescription, shortDescription);
  if (shortDescription.size() > 40)
    shortDescription = shortDescription.Left(40);
  wcsncpy(HC.TitleName, shortDescription.c_str(), 40);  // only allow 40 chars*/
  fwrite(&HC,1,sizeof(HC),hFile);
  fclose(hFile);
  return true;
}

DWORD CUtil::GetXbeID( const CStdString& strFilePath)
{
  DWORD dwReturn = 0;

  DWORD dwCertificateLocation;
  DWORD dwLoadAddress;
  DWORD dwRead;
  //  WCHAR wcTitle[41];

  CAutoPtrHandle hFile( CreateFile( strFilePath.c_str(),
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL ));
  if ( hFile.isValid() )
  {
    if ( SetFilePointer( (HANDLE)hFile, 0x104, NULL, FILE_BEGIN ) == 0x104 )
    {
      if ( ReadFile( (HANDLE)hFile, &dwLoadAddress, 4, &dwRead, NULL ) )
      {
        if ( SetFilePointer( (HANDLE)hFile, 0x118, NULL, FILE_BEGIN ) == 0x118 )
        {
          if ( ReadFile( (HANDLE)hFile, &dwCertificateLocation, 4, &dwRead, NULL ) )
          {
            dwCertificateLocation -= dwLoadAddress;
            // Add offset into file
            dwCertificateLocation += 8;
            if ( SetFilePointer( (HANDLE)hFile, dwCertificateLocation, NULL, FILE_BEGIN ) == dwCertificateLocation )
            {
              dwReturn = 0;
              ReadFile( (HANDLE)hFile, &dwReturn, sizeof(DWORD), &dwRead, NULL );
              if ( dwRead != sizeof(DWORD) )
              {
                dwReturn = 0;
              }
            }

          }
        }
      }
    }
  }

  return dwReturn;
}

void CUtil::CreateShortcut(CFileItem* pItem)
{
  if ( pItem->IsXBE() )
  {
    // xbe
    pItem->SetIconImage("defaultProgram.png");
    if ( !pItem->IsOnDVD() )
    {
      CStdString strDescription;
      if (! CUtil::GetXBEDescription(pItem->GetPath(), strDescription))
      {
        CUtil::GetDirectoryName(pItem->GetPath(), strDescription);
      }
      if (strDescription.size())
      {
        CStdString strFname;
        strFname = URIUtils::GetFileName(pItem->GetPath());
        strFname.ToLower();
        if (strFname != "dashupdate.xbe" && strFname != "downloader.xbe" && strFname != "update.xbe")
        {
          CShortcut cut;
          cut.m_strPath = pItem->GetPath();
          cut.Save(strDescription);
        }
      }
    }
  }
}

void CUtil::GetFatXQualifiedPath(CStdString& strFileNameAndPath)
{
  // This routine gets rid of any "\\"'s at the start of the path.
  // Should this be the case?
  vector<CStdString> tokens;
  CStdString strBasePath, strFileName;

  // We need to check whether we must use forward (ie. special://)
  // or backslashes (ie. Q:\)
  CStdString sep;
  if (strFileNameAndPath.c_str()[1] == ':' || strFileNameAndPath.Find('\\')>=0)
  {
    strFileNameAndPath.Replace('/', '\\');
    sep="\\";
  }
  else
  {
//    strFileNameAndPath.Replace('\\', '/');
    sep="/";
  }
  
  if(strFileNameAndPath.Right(1) == sep)
  {
    strBasePath = strFileNameAndPath;
    strFileName = "";
  }
  else
  {
    URIUtils::GetDirectory(strFileNameAndPath,strBasePath);
    // TODO: GETDIR - is this required?  What happens to the tokenize below otherwise?
    strFileName = URIUtils::GetFileName(strFileNameAndPath);
  }
  
  StringUtils::SplitString(strBasePath,sep,tokens);
  if (tokens.empty())
    return; // nothing to do here (invalid path)
  
  strFileNameAndPath = tokens.front();
  for (vector<CStdString>::iterator token=tokens.begin()+1;token != tokens.end();++token)
  {
    CStdString strToken = token->Left(42);
    if (token->size() > 42)
    {
      // remove any spaces as a result of truncation (only):
      while (strToken[strToken.size()-1] == ' ')
        strToken.erase(strToken.size()-1);
    }
    CUtil::RemoveIllegalChars(strToken);
    strFileNameAndPath += sep+strToken;
  }
  
  if (!strFileName.IsEmpty())
  {
    CUtil::RemoveIllegalChars(strFileName);
    
    if (strFileName.Left(1) == sep)
      strFileName.erase(0,1);

    if (CUtil::ShortenFileName(strFileName))
    {
      CStdString strExtension;
      URIUtils::GetExtension(strFileName, strExtension);
      CStdString strNoExt(URIUtils::ReplaceExtension(strFileName, ""));
      // remove any spaces as a result of truncation (only):
      while (strNoExt[strNoExt.size()-1] == ' ')
        strNoExt.erase(strNoExt.size()-1);
      
      strFileNameAndPath += strNoExt+strExtension;
    }
    else
      strFileNameAndPath += strFileName;
  }
}

bool CUtil::ShortenFileName(CStdString& strFileNameAndPath)
{
  CStdString strFile = URIUtils::GetFileName(strFileNameAndPath);
  if (strFile.size() > 42)
  {
    CStdString strExtension;
    URIUtils::GetExtension(strFileNameAndPath, strExtension);
    CStdString strPath = strFileNameAndPath.Left( strFileNameAndPath.size() - strFile.size() );

    CRegExp reg;
    CStdString strSearch=strFile; strSearch.ToLower();
    reg.RegComp("([_\\-\\. ](cd|part)[0-9]*)[_\\-\\. ]");          // this is to ensure that cd1, cd2 or partXXX. do not
    int matchPos = reg.RegFind(strSearch.c_str());                 // get cut from filenames when they are shortened.

    CStdString strPartNumber;
    char* szPart = reg.GetReplaceString("\\1");
    strPartNumber = szPart;
    free(szPart);

    int partPos = 42 - strPartNumber.size() - strExtension.size();

    if (matchPos > partPos )
    {
       strFile = strFile.Left(partPos);
       strFile += strPartNumber;
    } 
    else
    {
       strFile = strFile.Left(42 - strExtension.size());
    }
    strFile += strExtension;

    CStdString strNewFile = strPath;
    if (!URIUtils::HasSlashAtEnd(strNewFile) && !strNewFile.IsEmpty())
      strNewFile += "\\";

    strNewFile += strFile;
    strFileNameAndPath = strNewFile;
    
    // We shortened the file:
    return true;
  }
  
  return false;
}


void CUtil::GetDVDDriveIcon( const CStdString& strPath, CStdString& strIcon )
{
  if ( !CDetectDVDMedia::IsDiscInDrive() )
  {
    strIcon = "DefaultDVDEmpty.png";
    return ;
  }

  if ( URIUtils::IsDVD(strPath) )
  {
    CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
    //  xbox DVD
    if ( pInfo != NULL && pInfo->IsUDFX( 1 ) )
    {
      strIcon = "DefaultXboxDVD.png";
      return ;
    }
    strIcon = "DefaultDVDRom.png";
    return ;
  }

  if ( URIUtils::IsISO9660(strPath) )
  {
    CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
    if ( pInfo != NULL && pInfo->IsVideoCd( 1 ) )
    {
      strIcon = "DefaultVCD.png";
      return ;
    }
    strIcon = "DefaultDVDRom.png";
    return ;
  }

  if ( URIUtils::IsCDDA(strPath) )
  {
    strIcon = "DefaultCDDA.png";
    return ;
  }
}

void CUtil::RemoveTempFiles()
{
  CStdString searchPath = g_settings.GetDatabaseFolder();
  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(searchPath, items, ".tmp", false))
    return;
  for (int i = 0; i < items.Size(); ++i)
  {
    if (items[i]->m_bIsFolder)
      continue;
    XFILE::CFile::Delete(items[i]->GetPath());
  }
}

void CUtil::DeleteGUISettings()
{
  // Load in master code first to ensure it's setting isn't reset
  TiXmlDocument doc;
  if (doc.LoadFile(g_settings.GetSettingsFile()))
  {
    g_guiSettings.LoadMasterLock(doc.RootElement());
  }
  // delete the settings file only
  CLog::Log(LOGINFO, "  DeleteFile(%s)", g_settings.GetSettingsFile().c_str());
  CFile::Delete(g_settings.GetSettingsFile());
}

void CUtil::RemoveIllegalChars( CStdString& strText)
{
  char szRemoveIllegal [1024];
  strcpy(szRemoveIllegal , strText.c_str());
  static char legalChars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!#$%&'()-@[]^_`{}~.��������������������������������";
  
  char *cursor;
  for (cursor = szRemoveIllegal; *(cursor += strspn(cursor, legalChars)); /**/ )
  {
    // Convert FatX illegal characters, if possible, to the closest "looking" character:
    if (strchr("������", (int) *cursor)) *cursor = 'A';
    else
    if (strchr("������", (int) *cursor)) *cursor = 'a';
    else
    if (strchr("�����", (int) *cursor)) *cursor = 'O';
    else
    if (strchr("�����", (int) *cursor)) *cursor = 'o';
    else
    if (strchr("����", (int) *cursor)) *cursor = 'U';
    else
    if (strchr("�����", (int) *cursor)) *cursor = 'u';
    else
    if (strchr("����", (int) *cursor)) *cursor = 'E';
    else
    if (strchr("����", (int) *cursor)) *cursor = 'e';
    else
    if (strchr("����", (int) *cursor)) *cursor = 'I';
    else
    if (strchr("����", (int) *cursor)) *cursor = 'i';
    else
    *cursor = '_';
  }
  
  strText = szRemoveIllegal;
}

void CUtil::ClearSubtitles()
{
  //delete cached subs
  CFileItemList items;
  CDirectory::GetDirectory("special://temp/",items);
  for( int i=0;i<items.Size();++i)
  {
    if (!items[i]->m_bIsFolder)
    {
      if ( items[i]->GetPath().Find("subtitle") >= 0 || items[i]->GetPath().Find("vobsub_queue") >= 0 )
      {
        CLog::Log(LOGDEBUG, "%s - Deleting temporary subtitle %s", __FUNCTION__, items[i]->GetPath().c_str());
        CFile::Delete(items[i]->GetPath());
      }
    }
  }
}

static const char * sub_exts[] = { ".utf", ".utf8", ".utf-8", ".sub", ".srt", ".smi", ".rt", ".txt", ".ssa", ".aqt", ".jss", ".ass", ".idx", NULL};

void CUtil::CacheSubtitles(const CStdString& strMovie, CStdString& strExtensionCached, XFILE::IFileCallback *pCallback )
{
  DWORD startTimer = timeGetTime();
  CLog::Log(LOGDEBUG,"%s: START", __FUNCTION__);

  // new array for commons sub dirs
  const char * common_sub_dirs[] = {"subs",
                              "Subs",
                              "subtitles",
                              "Subtitles",
                              "vobsubs",
                              "Vobsubs",
                              "sub",
                              "Sub",
                              "vobsub",
                              "Vobsub",
                              "subtitle",
                              "Subtitle",
                              NULL};

  vector<CStdString> vecExtensionsCached;
  strExtensionCached = "";

  CFileItem item(strMovie, false);
  if (item.IsInternetStream()) return ;
  if (item.IsHDHomeRun()) return ;
  if (item.IsSlingbox()) return ;
  if (item.IsPlayList()) return ;
  if (!item.IsVideo()) return ;

  vector<CStdString> strLookInPaths;

  CStdString strFileName;
  CStdString strPath;

  URIUtils::Split(strMovie, strPath, strFileName);
  CStdString strFileNameNoExt(URIUtils::ReplaceExtension(strFileName, ""));
  strLookInPaths.push_back(strPath);

  if (!g_settings.iAdditionalSubtitleDirectoryChecked && !g_guiSettings.GetString("subtitles.custompath").IsEmpty()) // to avoid checking non-existent directories (network) every time..
  {
    if (!g_application.getNetwork().IsAvailable() && !URIUtils::IsHD(g_guiSettings.GetString("subtitles.custompath")))
    {
      CLog::Log(LOGINFO,"CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's nonaccessible");
      g_settings.iAdditionalSubtitleDirectoryChecked = -1; // disabled
    }
    else if (!CDirectory::Exists(g_guiSettings.GetString("subtitles.custompath")))
    {
      CLog::Log(LOGINFO,"CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's nonexistant");
      g_settings.iAdditionalSubtitleDirectoryChecked = -1; // disabled
    }

    g_settings.iAdditionalSubtitleDirectoryChecked = 1;
  }

  if (strMovie.substr(0,6) == "rar://") // <--- if this is found in main path then ignore it!
  {
    CURL url(strMovie);
    CStdString strArchive = url.GetHostName();
    URIUtils::Split(strArchive, strPath, strFileName);
    strLookInPaths.push_back(strPath);
  }

  // checking if any of the common subdirs exist ..
  CLog::Log(LOGDEBUG,"%s: Checking for common subdirs...", __FUNCTION__);

  vector<CStdString> token;
  Tokenize(strPath,token,"/\\");
  if (token[token.size()-1].size() == 3 && token[token.size()-1].Mid(0,2).Equals("cd"))
  {
    CStdString strPath2;
    URIUtils::GetParentPath(strPath,strPath2);
    strLookInPaths.push_back(strPath2);
  }
  int iSize = strLookInPaths.size();
  for (int i=0;i<iSize;++i)
  {
    for (int j=0; common_sub_dirs[j]; j++)
    {
      CStdString strPath2;
      URIUtils::AddFileToFolder(strLookInPaths[i],common_sub_dirs[j],strPath2);
      if (CDirectory::Exists(strPath2))
        strLookInPaths.push_back(strPath2);
    }
  }
  // .. done checking for common subdirs

  // check if there any cd-directories in the paths we have added so far
  char temp[6];
  iSize = strLookInPaths.size();
  for (int i=0;i<9;++i) // 9 cd's
  {
    sprintf(temp,"cd%i",i+1);
    for (int i=0;i<iSize;++i)
    {
      CStdString strPath2;
      URIUtils::AddFileToFolder(strLookInPaths[i],temp,strPath2);
      if (CDirectory::Exists(strPath2))
        strLookInPaths.push_back(strPath2);
    }
  }
  // .. done checking for cd-dirs

  // this is last because we dont want to check any common subdirs or cd-dirs in the alternate <subtitles> dir.
  if (g_settings.iAdditionalSubtitleDirectoryChecked == 1)
  {
    strPath = g_guiSettings.GetString("subtitles.custompath");
    if (!URIUtils::HasSlashAtEnd(strPath))
      strPath += "/"; //Should work for both remote and local files
    strLookInPaths.push_back(strPath);
  }

  DWORD nextTimer = timeGetTime();
  CLog::Log(LOGDEBUG,"%s: Done (time: %i ms)", __FUNCTION__, (int)(nextTimer - startTimer));

  CStdString strLExt;
  CStdString strDest;
  CStdString strItem;

  // 2 steps for movie directory and alternate subtitles directory
  CLog::Log(LOGDEBUG,"%s: Searching for subtitles...", __FUNCTION__);
  for (unsigned int step = 0; step < strLookInPaths.size(); step++)
  {
    if (strLookInPaths[step].length() != 0)
    {
      CFileItemList items;

      CDirectory::GetDirectory(strLookInPaths[step], items, ".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.text|.ssa|.aqt|.jss|.ass|.idx|.ifo|.rar|.zip", false, false, DIR_CACHE_NEVER, true);
      int fnl = strFileNameNoExt.size();

      CStdString strFileNameNoExtNoCase(strFileNameNoExt);
      strFileNameNoExtNoCase.MakeLower();
      for (int j = 0; j < (int)items.Size(); j++)
      {
        URIUtils::Split(items[j]->GetPath(), strPath, strItem);

        // is this a rar-file ..
        if ((URIUtils::IsRAR(strItem) || URIUtils::IsZIP(strItem)) && g_guiSettings.GetBool("subtitles.searchrars"))
        {
          CStdString strRar, strItemWithPath;
          URIUtils::AddFileToFolder(strLookInPaths[step],strFileNameNoExt+URIUtils::GetExtension(strItem),strRar);
          URIUtils::AddFileToFolder(strLookInPaths[step],strItem,strItemWithPath);

          unsigned int iPos = strMovie.substr(0,6)=="rar://"?1:0;
          iPos = strMovie.substr(0,6)=="zip://"?1:0;
          if ((step != iPos) || (strFileNameNoExtNoCase+".rar").Equals(strItem) || (strFileNameNoExtNoCase+".zip").Equals(strItem))
            CacheRarSubtitles(items[j]->GetPath(), strFileNameNoExtNoCase);
        }
        else
        {
          for (int i = 0; sub_exts[i]; i++)
          {
            int l = strlen(sub_exts[i]);

            //Cache any alternate subtitles.
            if (strItem.Left(9).ToLower() == "subtitle." && strItem.Right(l).ToLower() == sub_exts[i])
            {
              strLExt = strItem.Right(strItem.GetLength() - 9);
              strDest.Format("special://temp/subtitle.alt-%s", strLExt);
              if (CFile::Cache(items[j]->GetPath(), strDest, pCallback, NULL))
              {
                CLog::Log(LOGINFO, " cached subtitle %s->%s\n", strItem.c_str(), strDest.c_str());
                strExtensionCached = strLExt;
              }
            }

            //Cache subtitle with same name as movie
            if (strItem.Right(l).ToLower() == sub_exts[i] && strItem.Left(fnl).ToLower() == strFileNameNoExt.ToLower())
            {
              strLExt = strItem.Right(strItem.size() - fnl);
              strDest.Format("special://temp/subtitle%s", strLExt);
              if (CFile::Cache(items[j]->GetPath(), strDest, pCallback, NULL))
                CLog::Log(LOGINFO, " cached subtitle %s->%s\n", strItem.c_str(), strDest.c_str());
            }
          }
        }
      }
    }
  }
  CLog::Log(LOGDEBUG,"%s: Done (time: %i ms)", __FUNCTION__, (int)(timeGetTime() - nextTimer));

  // build the vector with extensions
  CFileItemList items;
  CDirectory::GetDirectory("special://temp/", items,".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.text|.ssa|.aqt|.jss|.ass|.idx|.ifo|.rar|.zip",false);
  for (int i=0;i<items.Size();++i)
  {
    if (items[i]->m_bIsFolder)
      continue;

    CStdString filename = URIUtils::GetFileName(items[i]->GetPath());
    strLExt = filename.Right(filename.size()-8);
    vecExtensionsCached.push_back(strLExt);
    if (URIUtils::GetExtension(filename).Equals(".smi"))
    {
      //Cache multi-language sami subtitle
      CDVDSubtitleStream* pStream = new CDVDSubtitleStream();
      if(pStream->Open(items[i]->GetPath()))
      {
        CDVDSubtitleTagSami TagConv;
        TagConv.LoadHead(pStream);
        if (TagConv.m_Langclass.size() >= 2)
        {
          for (unsigned int k = 0; k < TagConv.m_Langclass.size(); k++)
          {
            strDest.Format("special://temp/subtitle.%s%s", TagConv.m_Langclass[k].Name, strLExt);
            if (CFile::Cache(items[i]->GetPath(), strDest, pCallback, NULL))
              CLog::Log(LOGINFO, " cached subtitle %s->%s\n", filename.c_str(), strDest.c_str());
            CStdString strTemp;
            strTemp.Format(".%s%s", TagConv.m_Langclass[k].Name, strLExt);
            vecExtensionsCached.push_back(strTemp);
          }
        }
      }
      delete pStream;
    }
  }

  // construct string of added exts
  for (vector<CStdString>::iterator it=vecExtensionsCached.begin(); it != vecExtensionsCached.end(); ++it)
    strExtensionCached += *it+"|";

  CLog::Log(LOGDEBUG,"%s: END (total time: %i ms)", __FUNCTION__, (int)(timeGetTime() - startTimer));
}

bool CUtil::CacheRarSubtitles(const CStdString& strRarPath, 
                              const CStdString& strCompare)
{
  bool bFoundSubs = false;
  CFileItemList ItemList;

  // zip only gets the root dir
  if (URIUtils::GetExtension(strRarPath).Equals(".zip"))
  {
    CStdString strZipPath;
    URIUtils::CreateArchivePath(strZipPath,"zip",strRarPath,"");
    if (!CDirectory::GetDirectory(strZipPath,ItemList,"",false))
      return false;
  }
  else
  {
    // get _ALL_files in the rar, even those located in subdirectories because we set the bMask to false.
    // so now we dont have to find any subdirs anymore, all files in the rar is checked.
    if( !g_RarManager.GetFilesInRar(ItemList, strRarPath, false, "") )
      return false;
  }
  for (int it= 0 ; it <ItemList.Size();++it)
  {
    CStdString strPathInRar = ItemList[it]->GetPath();
    CStdString strExt = URIUtils::GetExtension(strPathInRar);

    CLog::Log(LOGDEBUG, "CacheRarSubs:: Found file %s", strPathInRar.c_str());
    // always check any embedded rar archives
    // checking for embedded rars, I moved this outside the sub_ext[] loop. We only need to check this once for each file.
    if (URIUtils::IsRAR(strPathInRar) || URIUtils::IsZIP(strPathInRar))
    {
      CStdString strRarInRar;
      if (URIUtils::GetExtension(strPathInRar).Equals(".rar"))
        URIUtils::CreateArchivePath(strRarInRar, "rar", strRarPath, strPathInRar);
      else
        URIUtils::CreateArchivePath(strRarInRar, "zip", strRarPath, strPathInRar);
      CacheRarSubtitles(strRarInRar,strCompare);
    }
    // done checking if this is a rar-in-rar

    int iPos=0;
    CStdString strFileName = URIUtils::GetFileName(strPathInRar);
    CStdString strFileNameNoCase(strFileName);
    strFileNameNoCase.MakeLower();
    if (strFileNameNoCase.Find(strCompare) >= 0)
      while (sub_exts[iPos])
      {
        if (strExt.CompareNoCase(sub_exts[iPos]) == 0)
        {
          CStdString strSourceUrl;
          if (URIUtils::GetExtension(strRarPath).Equals(".rar"))
            URIUtils::CreateArchivePath(strSourceUrl, "rar", strRarPath, strPathInRar);
          else
            strSourceUrl = strPathInRar;

          CStdString strDestFile;
          strDestFile.Format("special://temp/subtitle%s", sub_exts[iPos]);

          if (CFile::Cache(strSourceUrl,strDestFile))
          {
            CLog::Log(LOGINFO, " cached subtitle %s->%s", strPathInRar.c_str(), strDestFile.c_str());
            bFoundSubs = true;
            break;
          }
        }

        iPos++;
      }
  }
  return bFoundSubs;
}

void CUtil::PrepareSubtitleFonts()
{
  CStdString strFontPath = "special://xbmc/system/players/mplayer/font";

  if( IsUsingTTFSubtitles()
    || g_guiSettings.GetInt("subtitles.height") == 0
    || g_guiSettings.GetString("subtitles.font").size() == 0)
  {
    /* delete all files in the font dir, so mplayer doesn't try to load them */

    CStdString strSearchMask = strFontPath + "\\*.*";
    WIN32_FIND_DATA wfd;
    CAutoPtrFind hFind ( FindFirstFile(_P(strSearchMask).c_str(), &wfd));
    if (hFind.isValid())
    {
      do
      {
        if(wfd.cFileName[0] == 0) continue;
        if( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
          CFile::Delete(URIUtils::AddFileToFolder(strFontPath, wfd.cFileName));
      }
      while (FindNextFile((HANDLE)hFind, &wfd));
    }
  }
  else
  {
    CStdString strPath;
    strPath.Format("%s\\%s\\%i",
                  strFontPath.c_str(),
                  g_guiSettings.GetString("Subtitles.Font").c_str(),
                  g_guiSettings.GetInt("Subtitles.Height"));

    CStdString strSearchMask = strPath + "\\*.*";
    WIN32_FIND_DATA wfd;
    CAutoPtrFind hFind ( FindFirstFile(_P(strSearchMask).c_str(), &wfd));
    if (hFind.isValid())
    {
      do
      {
        if (wfd.cFileName[0] == 0) continue;
        if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
        {
          CStdString strSource = URIUtils::AddFileToFolder(strPath, wfd.cFileName);
          CStdString strDest = URIUtils::AddFileToFolder(strFontPath, wfd.cFileName);
          CFile::Cache(strSource, strDest);
        }
      }
      while (FindNextFile((HANDLE)hFind, &wfd));
    }
  }
}

__int64 CUtil::ToInt64(DWORD dwHigh, DWORD dwLow)
{
  __int64 n;
  n = dwHigh;
  n <<= 32;
  n += dwLow;
  return n;
}

bool CUtil::ThumbExists(const CStdString& strFileName, bool bAddCache)
{
  return CThumbnailCache::GetThumbnailCache()->ThumbExists(strFileName, bAddCache);
}

void CUtil::ThumbCacheAdd(const CStdString& strFileName, bool bFileExists)
{
  CThumbnailCache::GetThumbnailCache()->Add(strFileName, bFileExists);
}

void CUtil::ThumbCacheClear()
{
  CThumbnailCache::GetThumbnailCache()->Clear();
}

bool CUtil::ThumbCached(const CStdString& strFileName)
{
  return CThumbnailCache::GetThumbnailCache()->IsCached(strFileName);
}

void CUtil::PlayDVD()
{
  if (g_guiSettings.GetBool("dvds.useexternaldvdplayer") && !g_guiSettings.GetString("dvds.externaldvdplayer").IsEmpty())
  {
    RunXBE(g_guiSettings.GetString("dvds.externaldvdplayer").c_str());
  }
  else
  {
    CIoSupport::Dismount("Cdrom0");
    CIoSupport::RemapDriveLetter('D', "Cdrom0");
    CFileItem item("dvd://1", false);
    item.SetLabel(CDetectDVDMedia::GetDVDLabel());
    g_application.PlayFile(item);
  }
}

CStdString CUtil::GetNextFilename(const CStdString &fn_template, int max)
{
  if (!fn_template.Find("%03d"))
    return "";

  CStdString searchPath;
  URIUtils::GetDirectory(fn_template, searchPath);
  CStdString mask = URIUtils::GetExtension(fn_template);

  CStdString name;
  name.Format(fn_template.c_str(), 0);

  CFileItemList items;
  if (!CDirectory::GetDirectory(searchPath, items, mask, false))
    return name;

  items.SetFastLookup(true);
  for (int i = 0; i <= max; i++)
  {
    CStdString name;
    name.Format(fn_template.c_str(), i);
    if (!items.Get(name))
      return name;
  }
  return "";
}

void CUtil::InitGamma()
{
  g_graphicsContext.Get3DDevice()->GetGammaRamp(&oldramp);
}
void CUtil::RestoreBrightnessContrastGamma()
{
  g_graphicsContext.Lock();
  g_graphicsContext.Get3DDevice()->SetGammaRamp(GAMMA_RAMP_FLAG, &oldramp);
  g_graphicsContext.Unlock();
}

void CUtil::SetBrightnessContrastGammaPercent(float brightness, float contrast, float gamma, bool immediate)
{
  if (brightness < 0) brightness = 0;
  if (brightness > 100) brightness = 100;
  if (contrast < 0) contrast = 0;
  if (contrast > 100) contrast = 100;
  if (gamma < 0) gamma = 0;
  if (gamma > 100) gamma = 100;

  float fBrightNess = brightness / 50.0f - 1.0f; // -1..1    Default: 0
  float fContrast = contrast / 50.0f;            // 0..2     Default: 1
  float fGamma = gamma / 40.0f + 0.5f;           // 0.5..3.0 Default: 1
  CUtil::SetBrightnessContrastGamma(fBrightNess, fContrast, fGamma, immediate);
}

void CUtil::SetBrightnessContrastGamma(float Brightness, float Contrast, float Gamma, bool bImmediate)
{
  // calculate ramp
  D3DGAMMARAMP ramp;

  Gamma = 1.0f / Gamma;
  for (int i = 0; i < 256; ++i)
  {
    float f = (powf((float)i / 255.f, Gamma) * Contrast + Brightness) * 255.f;
    ramp.blue[i] = ramp.green[i] = ramp.red[i] = clamp(f);
  }

  // set ramp next v sync
  g_graphicsContext.Lock();
  g_graphicsContext.Get3DDevice()->SetGammaRamp(bImmediate ? GAMMA_RAMP_FLAG : 0, &ramp);
  g_graphicsContext.Unlock();
}


void CUtil::Tokenize(const CStdString& path, vector<CStdString>& tokens, const string& delimiters)
{
  // Tokenize ripped from http://www.linuxselfhelp.com/HOWTO/C++Programming-HOWTO-7.html
  // Skip delimiters at beginning.
  string::size_type lastPos = path.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  string::size_type pos = path.find_first_of(delimiters, lastPos);

  while (string::npos != pos || string::npos != lastPos)
  {
    // Found a token, add it to the vector.
    tokens.push_back(path.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = path.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = path.find_first_of(delimiters, lastPos);
  }
}


void CUtil::FlashScreen(bool bImmediate, bool bOn)
{
  static bool bInFlash = false;

  if (bInFlash == bOn)
    return ;
  bInFlash = bOn;
  g_graphicsContext.Lock();
  if (bOn)
  {
    g_graphicsContext.Get3DDevice()->GetGammaRamp(&flashramp);
    SetBrightnessContrastGamma(0.5f, 1.2f, 2.0f, bImmediate);
  }
  else
    g_graphicsContext.Get3DDevice()->SetGammaRamp(bImmediate ? GAMMA_RAMP_FLAG : 0, &flashramp);
  g_graphicsContext.Unlock();
}

void CUtil::TakeScreenshot(const CStdString& strFileName, bool flashScreen)
{
    LPDIRECT3DSURFACE8 lpSurface = NULL;
    g_graphicsContext.Lock();
    CStdString strFileNameTranslated = CSpecialProtocol::TranslatePath(strFileName);
    if (g_application.IsPlayingVideo())
    {
#ifdef HAS_VIDEO_PLAYBACK
      g_renderManager.SetupScreenshot();
#endif
    }
    if (0)
    { // reset calibration to defaults
      OVERSCAN oscan;
      memcpy(&oscan, &g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan, sizeof(OVERSCAN));
      g_graphicsContext.ResetOverscan(g_graphicsContext.GetVideoResolution(), g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan);
      g_application.Render();
      memcpy(&g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan, &oscan, sizeof(OVERSCAN));
    }
    // now take screenshot
#ifdef HAS_XBOX_D3D
    g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();
#endif
#ifdef HAS_XBOX_D3D
    if (SUCCEEDED(g_graphicsContext.Get3DDevice()->GetBackBuffer( -1, D3DBACKBUFFER_TYPE_MONO, &lpSurface)))
#else
    g_application.RenderNoPresent();
    if (SUCCEEDED(g_graphicsContext.Get3DDevice()->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &lpSurface)))
#endif
    {
      if (FAILED(XGWriteSurfaceToFile(lpSurface, strFileNameTranslated.c_str())))
      {
        CLog::Log(LOGERROR, "Failed to Generate Screenshot");
      }
      else
      {
        // hack - need to add it manually to the directory cache for both the special:// and the mapped
        // folder due to CFile::Open on a special:// path being checked against both. Ideally we would use
        // the XBMC Fileystem for writing the file. TODO.
        g_directoryCache.AddFile(strFileName);
        g_directoryCache.AddFile(strFileNameTranslated);
        CLog::Log(LOGINFO, "Screen shot saved as %s", strFileNameTranslated.c_str());
      }
      lpSurface->Release();
    }
    g_graphicsContext.Unlock();
    if (flashScreen)
    {
#ifdef HAS_XBOX_D3D
      g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();
#endif
      FlashScreen(true, true);
      Sleep(10);
#ifdef HAS_XBOX_D3D
      g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();
#endif
      FlashScreen(true, false);
    }
}

void CUtil::TakeScreenshot()
{
  static bool savingScreenshots = false;
  static vector<CStdString> screenShots;

  bool promptUser = false;
  // check to see if we have a screenshot folder yet
  CStdString strDir = g_guiSettings.GetString("debug.screenshotpath", false);
  if (strDir.IsEmpty())
  {
    strDir = "special://temp/";
    if (!savingScreenshots)
    {
      promptUser = true;
      savingScreenshots = true;
      screenShots.clear();
    }
  }
  URIUtils::RemoveSlashAtEnd(strDir);

  if (!strDir.IsEmpty())
  {
    CStdString file = CUtil::GetNextFilename(URIUtils::AddFileToFolder(strDir, "screenshot%03d.bmp"), 999);

    if (!file.IsEmpty())
    {
      TakeScreenshot(file.c_str(), true);
      if (savingScreenshots)
        screenShots.push_back(file);
      if (promptUser)
      { // grab the real directory
        CStdString newDir = g_guiSettings.GetString("debug.screenshotpath");
        if (!newDir.IsEmpty())
        {
          for (unsigned int i = 0; i < screenShots.size(); i++)
          {
            CStdString file = CUtil::GetNextFilename(URIUtils::AddFileToFolder(newDir, "screenshot%03d.bmp"), 999);
            CFile::Cache(screenShots[i], file);
          }
          screenShots.clear();
        }
        savingScreenshots = false;
      }
    }
    else
    {
      CLog::Log(LOGWARNING, "Too many screen shots or invalid folder");
    }
  }
}

void CUtil::StatToStatI64(struct _stati64 *result, struct stat *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = (__int64)stat->st_size;

#ifndef _LINUX
  result->st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#else
  result->_st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->_st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->_st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#endif
}

void CUtil::Stat64ToStatI64(struct _stati64 *result, struct __stat64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = stat->st_size;
#ifndef _LINUX
  result->st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#else
  result->_st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->_st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->_st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#endif
}

void CUtil::StatI64ToStat64(struct __stat64 *result, struct _stati64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = stat->st_size;
#ifndef _LINUX
  result->st_atime = stat->st_atime;
  result->st_mtime = stat->st_mtime;
  result->st_ctime = stat->st_ctime;
#else
  result->st_atime = stat->_st_atime;
  result->st_mtime = stat->_st_mtime;
  result->st_ctime = stat->_st_ctime;
#endif
}

void CUtil::Stat64ToStat(struct _stat *result, struct __stat64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
#ifndef _LINUX
  if (stat->st_size <= LONG_MAX)
    result->st_size = (_off_t)stat->st_size;
#else
  if (sizeof(stat->st_size) <= sizeof(result->st_size) )
    result->st_size = (off_t)stat->st_size;
#endif
  else
  {
    result->st_size = 0;
    CLog::Log(LOGWARNING, "WARNING: File is larger than 32bit stat can handle, file size will be reported as 0 bytes");
  }
  result->st_atime = (time_t)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (time_t)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (time_t)(stat->st_ctime & 0xFFFFFFFF);
}

bool CUtil::CreateDirectoryEx(const CStdString& strPath)
{
  // Function to create all directories at once instead
  // of calling CreateDirectory for every subdir.
  // Creates the directory and subdirectories if needed.

  // return true if directory already exist
  if (CDirectory::Exists(strPath)) return true;
  
  // we currently only allow HD and smb paths
  if (!URIUtils::IsHD(strPath) && !URIUtils::IsSmb(strPath))
  {
    CLog::Log(LOGERROR,"%s called with an unsupported path: %s", __FUNCTION__, strPath.c_str());
    return false;
  }
  
  CStdStringArray dirs = URIUtils::SplitPath(strPath);
  CStdString dir(dirs.front());
  URIUtils::AddSlashAtEnd(dir);
  for (CStdStringArray::iterator it = dirs.begin() + 1; it != dirs.end(); it ++)
  {
    dir = URIUtils::AddFileToFolder(dir, *it);
    CDirectory::Create(dir);
  }
  
  // was the final destination directory successfully created ?
  if (!CDirectory::Exists(strPath)) return false;
  return true;
}

CStdString CUtil::MakeLegalFileName(const CStdString &strFile, int LegalType)
{
  CStdString result = strFile;

  result.Replace('/', '_');
  result.Replace('\\', '_');
  result.Replace('?', '_');

  if (LegalType == LEGAL_WIN32_COMPAT)
  {
    // just filter out some illegal characters on windows
    result.Replace(':', '_');
    result.Replace('*', '_');
    result.Replace('?', '_');
    result.Replace('\"', '_');
    result.Replace('<', '_');
    result.Replace('>', '_');
    result.Replace('|', '_');
    result.TrimRight(".");
    result.TrimRight(" ");
  }

  // check if the filename is a legal FATX one.
  if (LegalType == LEGAL_FATX) 
  {
    result.Replace(':', '_');
    result.Replace('*', '_');
    result.Replace('?', '_');
    result.Replace('\"', '_');
    result.Replace('<', '_');
    result.Replace('>', '_');
    result.Replace('|', '_');
    result.Replace(',', '_');
    result.Replace('=', '_');
    result.Replace('+', '_');
    result.Replace(';', '_');
    result.Replace('"', '_');
    result.Replace('\'', '_');
    result.TrimRight(".");
    result.TrimRight(" ");

    GetFatXQualifiedPath(result);
  }

  return result;
}

// legalize entire path
CStdString CUtil::MakeLegalPath(const CStdString &strPathAndFile, int LegalType)
{
  if (URIUtils::IsStack(strPathAndFile))
    return MakeLegalPath(CStackDirectory::GetFirstStackedFile(strPathAndFile));
  if (URIUtils::IsMultiPath(strPathAndFile))
    return MakeLegalPath(CMultiPathDirectory::GetFirstPath(strPathAndFile));
  if (!URIUtils::IsHD(strPathAndFile) && !URIUtils::IsSmb(strPathAndFile))
    return strPathAndFile; // we don't support writing anywhere except HD, SMB and NFS - no need to legalize path

  bool trailingSlash = URIUtils::HasSlashAtEnd(strPathAndFile);
  CStdStringArray dirs = URIUtils::SplitPath(strPathAndFile);
  // we just add first token to path and don't legalize it - possible values: 
  // "X:" (local win32), "" (local unix - empty string before '/') or
  // "protocol://domain"
  CStdString dir(dirs.front());
  URIUtils::AddSlashAtEnd(dir);
  for (CStdStringArray::iterator it = dirs.begin() + 1; it != dirs.end(); it ++)
    dir = URIUtils::AddFileToFolder(dir, MakeLegalFileName(*it, LegalType));
  if (trailingSlash) URIUtils::AddSlashAtEnd(dir);
  return dir;
}

CStdString CUtil::ValidatePath(const CStdString &path, bool bFixDoubleSlashes /* = false */)
{
  CStdString result = path;

  // Don't do any stuff on URLs containing %-characters or protocols that embed
  // filenames. NOTE: Don't use IsInZip or IsInRar here since it will infinitely
  // recurse and crash XBMC
  if (URIUtils::IsURL(path) && 
     (path.Find('%') >= 0 ||
      path.Left(4).Equals("zip:") ||
      path.Left(4).Equals("rar:") ||
      path.Left(6).Equals("stack:") ||
      path.Left(10).Equals("multipath:") ))
    return result;

  // check the path for incorrect slashes
  if (URIUtils::IsDOSPath(path))
  {
    result.Replace('/', '\\');
    /* The double slash correction should only be used when *absolutely*
       necessary! This applies to certain DLLs or use from Python DLLs/scripts
       that incorrectly generate double (back) slashes.
    */
    if (bFixDoubleSlashes)
    {
      // Fixup for double back slashes (but ignore the \\ of unc-paths)
      for (int x = 1; x < result.GetLength() - 1; x++)
      {
        if (result[x] == '\\' && result[x+1] == '\\')
          result.Delete(x);
      }
    }
  }
  else if (path.Find("://") >= 0 || path.Find(":\\\\") >= 0)
  {
    result.Replace('\\', '/');
    /* The double slash correction should only be used when *absolutely*
       necessary! This applies to certain DLLs or use from Python DLLs/scripts
       that incorrectly generate double (back) slashes.
    */
    if (bFixDoubleSlashes)
    {
      // Fixup for double forward slashes(/) but don't touch the :// of URLs
      for (int x = 2; x < result.GetLength() - 1; x++)
      {
        if ( result[x] == '/' && result[x + 1] == '/' && !(result[x - 1] == ':' || (result[x - 1] == '/' && result[x - 2] == ':')) )
          result.Delete(x);
      }
    }
  }
  return result;
}

bool CUtil::IsUsingTTFSubtitles()
{
  return URIUtils::GetExtension(g_guiSettings.GetString("subtitles.font")).Equals(".ttf");
}

void CUtil::SplitExecFunction(const CStdString &execString, CStdString &function, vector<CStdString> &parameters)
{
  CStdString paramString;
 
  int iPos = execString.Find("(");
  int iPos2 = execString.ReverseFind(")");
  if (iPos > 0 && iPos2 > 0)
  {
    paramString = execString.Mid(iPos + 1, iPos2 - iPos - 1);
    function = execString.Left(iPos);
  }
  else
    function = execString;

  // remove any whitespace, and the standard prefix (if it exists)
  function.Trim();
  if( function.Left(5).Equals("xbmc.", false) )
    function.Delete(0, 5);

  SplitParams(paramString, parameters);
}

void CUtil::SplitParams(const CStdString &paramString, std::vector<CStdString> &parameters)
{
  bool inQuotes = false;
  bool lastEscaped = false; // only every second character can be escaped
  int inFunction = 0;
  size_t whiteSpacePos = 0;
  CStdString parameter;
  parameters.clear();
  for (size_t pos = 0; pos < paramString.size(); pos++)
  {
    char ch = paramString[pos];
    bool escaped = (pos > 0 && paramString[pos - 1] == '\\' && !lastEscaped);
    lastEscaped = escaped;
    if (inQuotes)
    { // if we're in a quote, we accept everything until the closing quote
      if (ch == '\"' && !escaped)
      { // finished a quote - no need to add the end quote to our string
        inQuotes = false;
      }
    }
    else
    { // not in a quote, so check if we should be starting one
      if (ch == '\"' && !escaped)
      { // start of quote - no need to add the quote to our string
        inQuotes = true;
      }
      if (inFunction && ch == ')')
      { // end of a function
        inFunction--;
      }
      if (ch == '(')
      { // start of function
        inFunction++;
      }
      if (!inFunction && ch == ',')
      { // not in a function, so a comma signfies the end of this parameter
        if (whiteSpacePos)
          parameter = parameter.Left(whiteSpacePos);
        // trim off start and end quotes
        if (parameter.GetLength() > 1 && parameter[0] == '\"' && parameter[parameter.GetLength() - 1] == '\"')
          parameter = parameter.Mid(1,parameter.GetLength() - 2);
        parameters.push_back(parameter);
        parameter.Empty();
        whiteSpacePos = 0;
        continue;
      }
    }
    if ((ch == '\"' || ch == '\\') && escaped)
    { // escaped quote or backslash
      parameter[parameter.size()-1] = ch;
      continue;
    }
    // whitespace handling - we skip any whitespace at the left or right of an unquoted parameter
    if (ch == ' ' && !inQuotes)
    {
      if (parameter.IsEmpty()) // skip whitespace on left
        continue;
      if (!whiteSpacePos) // make a note of where whitespace starts on the right
        whiteSpacePos = parameter.size();
    }
    else
      whiteSpacePos = 0;
    parameter += ch;
  }
  if (inFunction || inQuotes)
    CLog::Log(LOGWARNING, "%s(%s) - end of string while searching for ) or \"", __FUNCTION__, paramString.c_str());
  if (whiteSpacePos)
    parameter = parameter.Left(whiteSpacePos);
  // trim off start and end quotes
  if (parameter.GetLength() > 1 && parameter[0] == '\"' && parameter[parameter.GetLength() - 1] == '\"')
    parameter = parameter.Mid(1,parameter.GetLength() - 2);
  if (!parameter.IsEmpty() || parameters.size())
    parameters.push_back(parameter);
}

int CUtil::GetMatchingSource(const CStdString& strPath1, VECSOURCES& VECSOURCES, bool& bIsSourceName)
{
  if (strPath1.IsEmpty())
    return -1;

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, testing original path/name [%s]", strPath1.c_str());

  // copy as we may change strPath
  CStdString strPath = strPath1;

  // Check for special protocols
  CURL checkURL(strPath);

  // stack://
  if (checkURL.GetProtocol() == "stack")
    strPath.Delete(0, 8); // remove the stack protocol

  if (checkURL.GetProtocol() == "shout")
    strPath = checkURL.GetHostName();
  if (checkURL.GetProtocol() == "lastfm")
    return 1;
  if (checkURL.GetProtocol() == "tuxbox")
    return 1;
  if (checkURL.GetProtocol() == "plugin")
    return 1;
  if (checkURL.GetProtocol() == "multipath")
    strPath = CMultiPathDirectory::GetFirstPath(strPath);

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, testing for matching name [%s]", strPath.c_str());
  bIsSourceName = false;
  int iIndex = -1;
  int iLength = -1;
  // we first test the NAME of a source
  for (int i = 0; i < (int)VECSOURCES.size(); ++i)
  {
    CMediaSource share = VECSOURCES.at(i);
    CStdString strName = share.strName;

    // special cases for dvds
    if (URIUtils::IsOnDVD(share.strPath))
    {
      if (URIUtils::IsOnDVD(strPath))
        return i;

      // not a path, so we need to modify the source name
      // since we add the drive status and disc name to the source
      // "Name (Drive Status/Disc Name)"
      int iPos = strName.ReverseFind('(');
      if (iPos > 1)
        strName = strName.Mid(0, iPos - 1);
    }
    //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, comparing name [%s]", strName.c_str());
    if (strPath.Equals(strName))
    {
      bIsSourceName = true;
      return i;
    }
  }

  // now test the paths

  // remove user details, and ensure path only uses forward slashes
  // and ends with a trailing slash so as not to match a substring
  CURL urlDest(strPath);
  urlDest.SetOptions("");
  CStdString strDest = urlDest.GetWithoutUserDetails();
  ForceForwardSlashes(strDest);
  if (!URIUtils::HasSlashAtEnd(strDest))
    strDest += "/";
  int iLenPath = strDest.size();

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, testing url [%s]", strDest.c_str());

  for (int i = 0; i < (int)VECSOURCES.size(); ++i)
  {
    CMediaSource share = VECSOURCES.at(i);

    // does it match a source name?
    if (share.strPath.substr(0,8) == "shout://")
    {
      CURL url(share.strPath);
      if (strPath.Equals(url.GetHostName()))
        return i;
    }

    // doesnt match a name, so try the source path
    vector<CStdString> vecPaths;

    // add any concatenated paths if they exist
    if (share.vecPaths.size() > 0)
      vecPaths = share.vecPaths;

    // add the actual share path at the front of the vector
    vecPaths.insert(vecPaths.begin(), share.strPath);

    // test each path
    for (int j = 0; j < (int)vecPaths.size(); ++j)
    {
      // remove user details, and ensure path only uses forward slashes
      // and ends with a trailing slash so as not to match a substring
      CURL urlShare(vecPaths[j]);
      urlShare.SetOptions("");
      CStdString strShare = urlShare.GetWithoutUserDetails();
      ForceForwardSlashes(strShare);
      if (!URIUtils::HasSlashAtEnd(strShare))
        strShare += "/";
      int iLenShare = strShare.size();
      //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, comparing url [%s]", strShare.c_str());

      if ((iLenPath >= iLenShare) && (strDest.Left(iLenShare).Equals(strShare)) && (iLenShare > iLength))
      {
        //CLog::Log(LOGDEBUG,"Found matching source at index %i: [%s], Len = [%i]", i, strShare.c_str(), iLenShare);

        // if exact match, return it immediately
        if (iLenPath == iLenShare)
        {
          // if the path EXACTLY matches an item in a concatentated path
          // set source name to true to load the full virtualpath
          bIsSourceName = false;
          if (vecPaths.size() > 1)
            bIsSourceName = true;
          return i;
        }
        iIndex = i;
        iLength = iLenShare;
      }
    }
  }

  // return the index of the share with the longest match
  if (iIndex == -1)
  {

    // rar:// and zip://
    // if archive wasn't mounted, look for a matching share for the archive instead
    if( strPath.Left(6).Equals("rar://") || strPath.Left(6).Equals("zip://") )
    {
      // get the hostname portion of the url since it contains the archive file
      strPath = checkURL.GetHostName();

      bIsSourceName = false;
      bool bDummy;
      return GetMatchingSource(strPath, VECSOURCES, bDummy);
    }

    CLog::Log(LOGWARNING,"CUtil::GetMatchingSource... no matching source found for [%s]", strPath1.c_str());
  }
  return iIndex;
}

CStdString CUtil::TranslateSpecialSource(const CStdString &strSpecial)
{
  CStdString strReturn=strSpecial;
  if (!strSpecial.IsEmpty() && strSpecial[0] == '$')
  {
    if (strSpecial.Left(5).Equals("$HOME"))
      URIUtils::AddFileToFolder("special://home/", strSpecial.Mid(5), strReturn);
    else if (strSpecial.Left(10).Equals("$SUBTITLES"))
      URIUtils::AddFileToFolder("special://subtitles/", strSpecial.Mid(10), strReturn);
    else if (strSpecial.Left(9).Equals("$USERDATA"))
      URIUtils::AddFileToFolder("special://userdata/", strSpecial.Mid(9), strReturn);
    else if (strSpecial.Left(9).Equals("$DATABASE"))
      URIUtils::AddFileToFolder("special://database/", strSpecial.Mid(9), strReturn);
    else if (strSpecial.Left(11).Equals("$THUMBNAILS"))
      URIUtils::AddFileToFolder("special://thumbnails/", strSpecial.Mid(11), strReturn);
    else if (strSpecial.Left(11).Equals("$RECORDINGS"))
      URIUtils::AddFileToFolder("special://recordings/", strSpecial.Mid(11), strReturn);
    else if (strSpecial.Left(12).Equals("$SCREENSHOTS"))
      URIUtils::AddFileToFolder("special://screenshots/", strSpecial.Mid(12), strReturn);
    else if (strSpecial.Left(15).Equals("$MUSICPLAYLISTS"))
      URIUtils::AddFileToFolder("special://musicplaylists/", strSpecial.Mid(15), strReturn);
    else if (strSpecial.Left(15).Equals("$VIDEOPLAYLISTS"))
      URIUtils::AddFileToFolder("special://videoplaylists/", strSpecial.Mid(15), strReturn);
    else if (strSpecial.Left(7).Equals("$CDRIPS"))
      URIUtils::AddFileToFolder("special://cdrips/", strSpecial.Mid(7), strReturn);
    // this one will be removed post 2.0
    else if (strSpecial.Left(10).Equals("$PLAYLISTS"))
      URIUtils::AddFileToFolder(g_guiSettings.GetString("system.playlistspath",false), strSpecial.Mid(10), strReturn);
  }
  return strReturn;
}

CStdString CUtil::MusicPlaylistsLocation()
{
  vector<CStdString> vec;
  CStdString strReturn;
  URIUtils::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "music", strReturn);
  vec.push_back(strReturn);
  URIUtils::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "mixed", strReturn);
  vec.push_back(strReturn);
  return XFILE::CMultiPathDirectory::ConstructMultiPath(vec);;
}

CStdString CUtil::VideoPlaylistsLocation()
{
  vector<CStdString> vec;
  CStdString strReturn;
  URIUtils::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "video", strReturn);
  vec.push_back(strReturn);
  URIUtils::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "mixed", strReturn);
  vec.push_back(strReturn);
  return XFILE::CMultiPathDirectory::ConstructMultiPath(vec);;
}

void CUtil::DeleteMusicDatabaseDirectoryCache()
{
  CUtil::DeleteDirectoryCache("mdb-");
}

void CUtil::DeleteVideoDatabaseDirectoryCache()
{
  CUtil::DeleteDirectoryCache("vdb-");
}

void CUtil::DeleteDirectoryCache(const CStdString &prefix)
{
  CStdString searchPath = "special://temp/";
  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(searchPath, items, ".fi", false))
    return;

  for (int i = 0; i < items.Size(); ++i)
  {
    if (items[i]->m_bIsFolder)
      continue;
    CStdString fileName = URIUtils::GetFileName(items[i]->GetPath());
    if (fileName.Left(prefix.GetLength()) == prefix)
      XFILE::CFile::Delete(items[i]->GetPath());
  }

}

bool CUtil::SetSysDateTimeYear(int iYear, int iMonth, int iDay, int iHour, int iMinute)
{
  TIME_ZONE_INFORMATION tziNew;
  SYSTEMTIME CurTime;
  SYSTEMTIME NewTime;
  GetLocalTime(&CurTime);
  GetLocalTime(&NewTime);
  int iRescBiases, iHourUTC;
  int iMinuteNew;

  DWORD dwRet = GetTimeZoneInformation(&tziNew);  // Get TimeZone Informations
  float iGMTZone = (float(tziNew.Bias)/(60));     // Calc's the GMT Time

  CLog::Log(LOGDEBUG, "------------ TimeZone -------------");
  CLog::Log(LOGDEBUG, "-      GMT Zone: GMT %.1f",iGMTZone);
  CLog::Log(LOGDEBUG, "-          Bias: %lu minutes",tziNew.Bias);
  CLog::Log(LOGDEBUG, "-  DaylightBias: %lu",tziNew.DaylightBias);
  CLog::Log(LOGDEBUG, "-  StandardBias: %lu",tziNew.StandardBias);

  switch (dwRet)
  {
    case TIME_ZONE_ID_STANDARD:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 1, Standart");
      }
      break;
    case TIME_ZONE_ID_DAYLIGHT:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias + tziNew.DaylightBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 2, Daylight");
      }
      break;
    case TIME_ZONE_ID_UNKNOWN:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 0, Unknown");
      }
      break;
    case TIME_ZONE_ID_INVALID:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: Invalid");
      }
      break;
    default:
      iRescBiases   = tziNew.Bias + tziNew.StandardBias;
  }
    CLog::Log(LOGDEBUG, "--------------- END ---------------");

  // Calculation
  iHourUTC = GMTZoneCalc(iRescBiases, iHour, iMinute, iMinuteNew);
  iMinute = iMinuteNew;
  if(iHourUTC <0)
  {
    iDay = iDay - 1;
    iHourUTC =iHourUTC + 24;
  }
  if(iHourUTC >23)
  {
    iDay = iDay + 1;
    iHourUTC =iHourUTC - 24;
  }

  // Set the New-,Detected Time Values to System Time!
  NewTime.wYear     = (WORD)iYear;
  NewTime.wMonth    = (WORD)iMonth;
  NewTime.wDay      = (WORD)iDay;
  NewTime.wHour     = (WORD)iHourUTC;
  NewTime.wMinute   = (WORD)iMinute;

  FILETIME stNewTime, stCurTime;
  SystemTimeToFileTime(&NewTime, &stNewTime);
  SystemTimeToFileTime(&CurTime, &stCurTime);
#ifdef HAS_XBOX_HARDWARE
  bool bReturn=NT_SUCCESS(NtSetSystemTime(&stNewTime, &stCurTime));
#else
  bool bReturn(false);
#endif
  return bReturn;
}
int CUtil::GMTZoneCalc(int iRescBiases, int iHour, int iMinute, int &iMinuteNew)
{
  int iHourUTC, iTemp;
  iMinuteNew = iMinute;
  iTemp = iRescBiases/60;

  if (iRescBiases == 0 )return iHour;   // GMT Zone 0, no need calculate
  if (iRescBiases > 0)
    iHourUTC = iHour + abs(iTemp);
  else
    iHourUTC = iHour - abs(iTemp);

  if ((iTemp*60) != iRescBiases)
  {
    if (iRescBiases > 0)
      iMinuteNew = iMinute + abs(iTemp*60 - iRescBiases);
    else
      iMinuteNew = iMinute - abs(iTemp*60 - iRescBiases);

    if (iMinuteNew >= 60)
    {
      iMinuteNew = iMinuteNew -60;
      iHourUTC = iHourUTC + 1;
    }
    else if (iMinuteNew < 0)
    {
      iMinuteNew = iMinuteNew +60;
      iHourUTC = iHourUTC - 1;
    }
  }
  return iHourUTC;
}

bool CUtil::AutoDetection()
{
  bool bReturn=false;
  if (g_guiSettings.GetBool("autodetect.onoff"))
  {
    static DWORD pingTimer = 0;
    if( timeGetTime() - pingTimer < (DWORD)g_advancedSettings.m_autoDetectPingTime * 1000)
      return false;
    pingTimer = timeGetTime();

  // send ping and request new client info
  if ( CUtil::AutoDetectionPing(
    g_guiSettings.GetBool("Autodetect.senduserpw") ? g_guiSettings.GetString("services.ftpserveruser"):"anonymous",
    g_guiSettings.GetBool("Autodetect.senduserpw") ? g_guiSettings.GetString("services.ftpserverpassword"):"anonymous",
    g_guiSettings.GetString("autodetect.nickname"),21 /*Our FTP Port! TODO: Extract FTP from FTP Server settings!*/) )
  {
    CStdString strFTPPath, strNickName, strFtpUserName, strFtpPassword, strFtpPort, strBoosMode;
    CStdStringArray arSplit;
    // do we have clients in our list ?
    for(unsigned int i=0; i < v_xboxclients.client_ip.size(); i++)
    {
      // extract client informations
      StringUtils::SplitString(v_xboxclients.client_info[i],";", arSplit);
      if ((int)arSplit.size() > 1 && !v_xboxclients.client_informed[i])
      {
        //extract client info and build the ftp link!
        strNickName     = arSplit[0].c_str();
        strFtpUserName  = arSplit[1].c_str();
        strFtpPassword  = arSplit[2].c_str();
        strFtpPort      = arSplit[3].c_str();
        strBoosMode     = arSplit[4].c_str();
        strFTPPath.Format("ftp://%s:%s@%s:%s/",strFtpUserName.c_str(),strFtpPassword.c_str(),v_xboxclients.client_ip[i],strFtpPort.c_str());

        //Do Notification for this Client
        CStdString strtemplbl;
        strtemplbl.Format("%s %s",strNickName, v_xboxclients.client_ip[i]);
        g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(1251), strtemplbl);

        //Debug Log
        CLog::Log(LOGDEBUG,"%s: %s FTP-Link: %s", g_localizeStrings.Get(1251).c_str(), strNickName.c_str(), strFTPPath.c_str());

        //set the client_informed to TRUE, to prevent loop Notification
        v_xboxclients.client_informed[i]=true;

        //YES NO PopUP: ask for connecting to the detected client via Filemanger!
        if (g_guiSettings.GetBool("autodetect.popupinfo") && CGUIDialogYesNo::ShowAndGetInput(1251, 0, 1257, 0))
        {
          g_windowManager.ActivateWindow(WINDOW_FILES, strFTPPath); //Open in MyFiles
        }
        bReturn = true;
      }
    }
  }
  }
  return bReturn;
}

bool CUtil::AutoDetectionPing(CStdString strFTPUserName, CStdString strFTPPass, CStdString strNickName, int iFTPPort)
{
  bool bFoundNewClient= false;
  CStdString strLocalIP;
  CStdString strSendMessage = "ping\0";
  CStdString strReceiveMessage = "ping";
  int iUDPPort = 4905;
  char sztmp[512];

  static int udp_server_socket, inited=0;
	int cliLen, t1,t2,t3,t4, init_counter=0, life=0;

  struct sockaddr_in	server;
  struct sockaddr_in	cliAddr;
  struct timeval timeout={0,500};
  fd_set readfds;
#ifdef HAS_XBOX_HARDWARE
    XNADDR xna;
    DWORD dwState = XNetGetTitleXnAddr(&xna);
    XNetInAddrToString(xna.ina,(char *)strLocalIP.c_str(),64);
#else
    char hostname[255];
    WORD wVer;
    WSADATA wData;
    PHOSTENT hostinfo;
    wVer = MAKEWORD( 2, 0 );
    if (WSAStartup(wVer,&wData) == 0)
    {
      if(gethostname(hostname,sizeof(hostname)) == 0)
      {
        if((hostinfo = gethostbyname(hostname)) != NULL)
        {
          strLocalIP = inet_ntoa (*(struct in_addr *)*hostinfo->h_addr_list);
          strNickName.Format("%s",hostname);
        }
      }
      WSACleanup();
    }
#endif
  // get IP address
  sscanf( (char *)strLocalIP.c_str(), "%d.%d.%d.%d", &t1, &t2, &t3, &t4 );
  if( !t1 ) return false;
  cliLen = sizeof( cliAddr);
  // setup UDP socket
  if( !inited )
  {
    int tUDPsocket  = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	  char value      = 1;
	  setsockopt( tUDPsocket, SOL_SOCKET, SO_BROADCAST, &value, value );
	  struct sockaddr_in addr;
	  memset(&(addr),0,sizeof(addr));
	  addr.sin_family       = AF_INET;
	  addr.sin_addr.s_addr  = INADDR_ANY;
	  addr.sin_port         = htons(iUDPPort);
	  bind(tUDPsocket,(struct sockaddr *)(&addr),sizeof(addr));
    udp_server_socket = tUDPsocket;
    inited = 1;
  }
  FD_ZERO(&readfds);
  FD_SET(udp_server_socket, &readfds);
  life = select( 0,&readfds, NULL, NULL, &timeout );
  if (life == SOCKET_ERROR )
    return false;
  memset(&(server),0,sizeof(server));
  server.sin_family = AF_INET;
#ifndef _LINUX
  server.sin_addr.S_un.S_addr = INADDR_BROADCAST;
#else
  server.sin_addr.s_addr = INADDR_BROADCAST;
#endif
  server.sin_port = htons(iUDPPort);
  sendto(udp_server_socket,(char *)strSendMessage.c_str(),5,0,(struct sockaddr *)(&server),sizeof(server));
	FD_ZERO(&readfds);
	FD_SET(udp_server_socket, &readfds);
	life = select( 0,&readfds, NULL, NULL, &timeout );
  
  unsigned int iLookUpCountMax = 2;
  unsigned int i=0;
  bool bUpdateShares=false;

  // Ping able clients? 0:false
  if (life == 0 )
  {
    if(v_xboxclients.client_ip.size() > 0)
    {
      // clients in list without life signal!
      // calculate iLookUpCountMax value counter dependence on clients size!
      if(v_xboxclients.client_ip.size() > iLookUpCountMax)
        iLookUpCountMax += (v_xboxclients.client_ip.size()-iLookUpCountMax);

      for (i=0; i<v_xboxclients.client_ip.size(); i++)
      {
        bUpdateShares=false;
        //only 1 client, clear our list
        if(v_xboxclients.client_lookup_count[i] >= iLookUpCountMax && v_xboxclients.client_ip.size() == 1 )
        {
          v_xboxclients.client_ip.clear();
          v_xboxclients.client_info.clear();
          v_xboxclients.client_lookup_count.clear();
          v_xboxclients.client_informed.clear();
          
          // debug log, clients removed from our list 
          CLog::Log(LOGDEBUG,"Autodetection: all Clients Removed! (mode LIFE 0)");
          bUpdateShares = true;
        }
        else 
        {
          // check client lookup counter! Not reached the CountMax, Add +1!
          if(v_xboxclients.client_lookup_count[i] < iLookUpCountMax ) 
            v_xboxclients.client_lookup_count[i] = v_xboxclients.client_lookup_count[i]+1;
          else
          {
            // client lookup counter REACHED CountMax, remove this client
            v_xboxclients.client_ip.erase(v_xboxclients.client_ip.begin()+i);
            v_xboxclients.client_info.erase(v_xboxclients.client_info.begin()+i);
            v_xboxclients.client_lookup_count.erase(v_xboxclients.client_lookup_count.begin()+i);
            v_xboxclients.client_informed.erase(v_xboxclients.client_informed.begin()+i);
            
            // debug log, clients removed from our list 
            CLog::Log(LOGDEBUG,"Autodetection: Client ID:[%i] Removed! (mode LIFE 0)",i );
            bUpdateShares = true;
          }
        }
        if(bUpdateShares)
        {
          // a client is removed from our list, update our shares
          CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
          g_windowManager.SendThreadMessage(msg);
        }
      }
    }
  }
  // life !=0 we are online and ready to receive and send
  while( life )
    {
    bFoundNewClient = false;
    bUpdateShares = false;
    // Receive ping request or Info
    int iSockRet = recvfrom(udp_server_socket, sztmp, 512, 0,(struct sockaddr *) &cliAddr, &cliLen); 
    if (iSockRet != SOCKET_ERROR)
    {
      CStdString strTmp;
      // do we received a new Client info or just a "ping" request
      if(strReceiveMessage.Equals(sztmp))
      {
        // we received a "ping" request, sending our informations
        strTmp.Format("%s;%s;%s;%d;%d\r\n\0",
          strNickName.c_str(),  // Our Nick-, Device Name!
          strFTPUserName.c_str(), // User Name for our FTP Server
          strFTPPass.c_str(), // Password for our FTP Server
          iFTPPort, // FTP PORT Adress for our FTP Server
          0 ); // BOOSMODE, for our FTP Server!
        sendto(udp_server_socket,(char *)strTmp.c_str(),strlen((char *)strTmp.c_str())+1,0,(struct sockaddr *)(&cliAddr),sizeof(cliAddr));
      }
      else
      {
        //We received new client information, extracting information
        CStdString strInfo, strIP;
        strInfo.Format("%s",sztmp); //this is the client info
        strIP.Format("%d.%d.%d.%d", 
#ifndef _LINUX
          cliAddr.sin_addr.S_un.S_un_b.s_b1,
          cliAddr.sin_addr.S_un.S_un_b.s_b2,
          cliAddr.sin_addr.S_un.S_un_b.s_b3,
          cliAddr.sin_addr.S_un.S_un_b.s_b4
#else
          (int)((char *)(cliAddr.sin_addr.s_addr))[0],
          (int)((char *)(cliAddr.sin_addr.s_addr))[1],
          (int)((char *)(cliAddr.sin_addr.s_addr))[2],
          (int)((char *)(cliAddr.sin_addr.s_addr))[3]
#endif
        ); //this is the client IP
        
        //Is this our Local IP ?
        if ( !strIP.Equals(strLocalIP) )
        {
          //is our list empty?
          if(v_xboxclients.client_ip.size() <= 0 )
          {
            // the list is empty, add. this client to the list!
            v_xboxclients.client_ip.push_back(strIP);
            v_xboxclients.client_info.push_back(strInfo);
            v_xboxclients.client_lookup_count.push_back(0);
            v_xboxclients.client_informed.push_back(false);
            bFoundNewClient = true;
            bUpdateShares = true;
          }
          // our list is not empty, check if we allready have this client in our list!
          else
          {
            // this should be a new client or?
            // check list
            bFoundNewClient = true;
            for (i=0; i<v_xboxclients.client_ip.size(); i++)
            {
              if(strIP.Equals(v_xboxclients.client_ip[i].c_str()))
                bFoundNewClient=false;
            }
            if(bFoundNewClient)
            {
              // bFoundNewClient is still true, the client is not in our list!
              // add. this client to our list!
              v_xboxclients.client_ip.push_back(strIP);
              v_xboxclients.client_info.push_back(strInfo);
              v_xboxclients.client_lookup_count.push_back(0);
              v_xboxclients.client_informed.push_back(false);
              bUpdateShares = true;
            }
            else // this is a existing client! check for LIFE & lookup counter
            {
              // calculate iLookUpCountMax value counter dependence on clients size!
              if(v_xboxclients.client_ip.size() > iLookUpCountMax)
                iLookUpCountMax += (v_xboxclients.client_ip.size()-iLookUpCountMax);

              for (i=0; i<v_xboxclients.client_ip.size(); i++)
              {
                if(strIP.Equals(v_xboxclients.client_ip[i].c_str()))
                {
                  // found client in list, reset looup_Count and the client_info
                  v_xboxclients.client_info[i]=strInfo;
                  v_xboxclients.client_lookup_count[i] = 0;
                }
                else
                {
                  // check client lookup counter! Not reached the CountMax, Add +1!
                  if(v_xboxclients.client_lookup_count[i] < iLookUpCountMax )
                    v_xboxclients.client_lookup_count[i] = v_xboxclients.client_lookup_count[i]+1;
                  else
                  {
                    // client lookup counter REACHED CountMax, remove this client
                    v_xboxclients.client_ip.erase(v_xboxclients.client_ip.begin()+i);
                    v_xboxclients.client_info.erase(v_xboxclients.client_info.begin()+i);
                    v_xboxclients.client_lookup_count.erase(v_xboxclients.client_lookup_count.begin()+i);
                    v_xboxclients.client_informed.erase(v_xboxclients.client_informed.begin()+i);

                    // debug log, clients removed from our list
                    CLog::Log(LOGDEBUG,"Autodetection: Client ID:[%i] Removed! (mode LIFE 1)",i );

                    // client is removed from our list, update our shares
                    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
                    g_windowManager.SendThreadMessage(msg);
                  }
                }
              }
              // here comes our list for debug log
              for (i=0; i<v_xboxclients.client_ip.size(); i++)
              {
                CLog::Log(LOGDEBUG,"Autodetection: Client ID:[%i] (mode LIFE=1)",i );
                CLog::Log(LOGDEBUG,"----------------------------------------------------------------" );
                CLog::Log(LOGDEBUG,"IP:%s Info:%s LookUpCount:%i Informed:%s",
                  v_xboxclients.client_ip[i].c_str(),
                  v_xboxclients.client_info[i].c_str(),
                  v_xboxclients.client_lookup_count[i],
                  v_xboxclients.client_informed[i] ? "true":"false");
                CLog::Log(LOGDEBUG,"----------------------------------------------------------------" );
              }
            }
          }
          if(bUpdateShares)
          {
            // a client is add or removed from our list, update our shares
            CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
            g_windowManager.SendThreadMessage(msg);
          }
        }
      }
    }
    else
    {
       CLog::Log(LOGDEBUG, "Autodetection: Socket error %u", WSAGetLastError());
    }
    timeout.tv_sec=0;
    timeout.tv_usec = 5000;
    FD_ZERO(&readfds);
    FD_SET(udp_server_socket, &readfds);
    life = select( 0,&readfds, NULL, NULL, &timeout );
  }
  return bFoundNewClient;
}

void CUtil::AutoDetectionGetSource(VECSOURCES &shares)
{
  if(v_xboxclients.client_ip.size() > 0)
  {
    // client list is not empty, add to shares
    CMediaSource share;
    for (unsigned int i=0; i< v_xboxclients.client_ip.size(); i++)
    {
      //extract client info string: NickName;FTP_USER;FTP_Password;FTP_PORT;BOOST_MODE
      CStdString strFTPPath, strNickName, strFtpUserName, strFtpPassword, strFtpPort, strBoosMode;
      CStdStringArray arSplit;
      StringUtils::SplitString(v_xboxclients.client_info[i],";", arSplit);
      if ((int)arSplit.size() > 1)
      {
        strNickName     = arSplit[0].c_str();
        strFtpUserName  = arSplit[1].c_str();
        strFtpPassword  = arSplit[2].c_str();
        strFtpPort      = arSplit[3].c_str();
        strBoosMode     = arSplit[4].c_str();
        strFTPPath.Format("ftp://%s:%s@%s:%s/",strFtpUserName.c_str(),strFtpPassword.c_str(),v_xboxclients.client_ip[i].c_str(),strFtpPort.c_str());

        strNickName.TrimRight(' ');
#ifdef HAS_XBOX_HARDWARE
        share.strName.Format("FTP XBMC (%s)", strNickName.c_str());
#else
        share.strName.Format("FTP XBMC_PC (%s)", strNickName.c_str());
#endif
        share.strPath.Format("%s",strFTPPath.c_str());
        shares.push_back(share);
      }
    }
  }
}

bool CUtil::GetFTPServerUserName(int iFTPUserID, CStdString &strFtpUser1, int &iUserMax )
{
#ifdef HAS_FTP_SERVER
  if( !g_application.m_pFileZilla )
    return false;

  class CXFUser* m_pUser;
  vector<CXFUser*> users;
  g_application.m_pFileZilla->GetAllUsers(users);
  iUserMax = users.size();
  if (iUserMax > 0)
  {
    //for (int i = 1 ; i < iUserSize; i++){ delete users[i]; }
    m_pUser = users[iFTPUserID];
    strFtpUser1 = m_pUser->GetName();
    if (strFtpUser1.size() != 0) return true;
    else return false;
  }
  else
#endif
    return false;
}
bool CUtil::SetFTPServerUserPassword(CStdString strFtpUserName, CStdString strFtpUserPassword)
{
#ifdef HAS_FTP_SERVER
  if( !g_application.m_pFileZilla )
    return false;

  CStdString strTempUserName;
  class CXFUser* p_ftpUser;
  vector<CXFUser*> v_ftpusers;
  bool bFoundUser = false;
  g_application.m_pFileZilla->GetAllUsers(v_ftpusers);
  int iUserSize = v_ftpusers.size();
  if (iUserSize > 0)
  {
    int i = 1 ;
    while( i <= iUserSize)
    {
      p_ftpUser = v_ftpusers[i-1];
      strTempUserName = p_ftpUser->GetName();
      if (strTempUserName.Equals(strFtpUserName.c_str()) )
      {
        if (p_ftpUser->SetPassword(strFtpUserPassword.c_str()) != XFS_INVALID_PARAMETERS)
        {
          p_ftpUser->CommitChanges();
          g_guiSettings.SetString("services.ftpserverpassword",strFtpUserPassword.c_str());
          return true;
        }
        break;
      }
      i++;
    }
  }
#endif
  return false;
}
//strXboxNickNameIn: New NickName to write
//strXboxNickNameOut: Same if it is in NICKNAME Cache
bool CUtil::SetXBOXNickName(CStdString strXboxNickNameIn, CStdString &strXboxNickNameOut)
{
#ifdef HAS_XBOX_HARDWARE
  WCHAR pszNickName[MAX_NICKNAME];
  unsigned int uiSize = MAX_NICKNAME;
  bool bfound= false;
  HANDLE hNickName = XFindFirstNickname(false,pszNickName,MAX_NICKNAME);
  if (hNickName != INVALID_HANDLE_VALUE)
  { do
      {
        strXboxNickNameOut.Format("%ls",pszNickName );
        if (strXboxNickNameIn.Equals(strXboxNickNameOut))
        {
          bfound = true;
          break;
        }
        else if (strXboxNickNameIn.IsEmpty()) strXboxNickNameOut.Format("XbMediaCenter");
      }while(XFindNextNickname(hNickName,pszNickName,uiSize) != false);
    XFindClose(hNickName);
  }
  if(!bfound)
  {
    CStdStringW wstrName = strXboxNickNameIn.c_str();
    XSetNickname(wstrName.c_str(), false);
  }
#endif
  return true;
}
//strXboxNickNameOut: Will fast receive the last XBOX NICKNAME from Cache
bool CUtil::GetXBOXNickName(CStdString &strXboxNickNameOut)
{
#ifdef HAS_XBOX_HARDWARE
  WCHAR wszXboxNickname[MAX_NICKNAME];
  HANDLE hNickName = XFindFirstNickname( FALSE, wszXboxNickname, MAX_NICKNAME );
	if ( hNickName != INVALID_HANDLE_VALUE )
	{
    strXboxNickNameOut.Format("%ls",wszXboxNickname);
		XFindClose( hNickName );
    return true;
	}
  else
#endif
  {
    // it seems to be empty? should we create one? or the user
    strXboxNickNameOut.Format("");
    return false;
  }
}

void CUtil::GetRecursiveListing(const CStdString& strPath, CFileItemList& items, const CStdString& strMask, bool bUseFileDirectories)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,strMask,bUseFileDirectories);
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder)
      CUtil::GetRecursiveListing(myItems[i]->GetPath(),items,strMask,bUseFileDirectories);
    else 
//    if (!myItems[i]->IsRAR() && !myItems[i]->IsZIP())
      items.Add(myItems[i]);
  }
}

void CUtil::GetRecursiveDirsListing(const CStdString& strPath, CFileItemList& item)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,"",false);
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder && !myItems[i]->GetPath().Equals(".."))
    {
      item.Add(myItems[i]);
      CUtil::GetRecursiveDirsListing(myItems[i]->GetPath(),item);
    }
  }
}

void CUtil::ForceForwardSlashes(CStdString& strPath)
{
  int iPos = strPath.ReverseFind('\\');
  while (iPos > 0)
  {
    strPath.at(iPos) = '/';
    iPos = strPath.ReverseFind('\\');
  }
}

double CUtil::AlbumRelevance(const CStdString& strAlbumTemp1, const CStdString& strAlbum1, const CStdString& strArtistTemp1, const CStdString& strArtist1)
{
  // case-insensitive fuzzy string comparison on the album and artist for relevance
  // weighting is identical, both album and artist are 50% of the total relevance
  // a missing artist means the maximum relevance can only be 0.50
  CStdString strAlbumTemp = strAlbumTemp1;
  strAlbumTemp.MakeLower();
  CStdString strAlbum = strAlbum1;
  strAlbum.MakeLower();
  double fAlbumPercentage = fstrcmp(strAlbumTemp, strAlbum, 0.0f);
  double fArtistPercentage = 0.0f;
  if (!strArtist1.IsEmpty())
  {
    CStdString strArtistTemp = strArtistTemp1;
    strArtistTemp.MakeLower();
    CStdString strArtist = strArtist1;
    strArtist.MakeLower();
    fArtistPercentage = fstrcmp(strArtistTemp, strArtist, 0.0f);
  }
  double fRelevance = fAlbumPercentage * 0.5f + fArtistPercentage * 0.5f;
  return fRelevance;
}

bool CUtil::MakeShortenPath(CStdString StrInput, CStdString& StrOutput, int iTextMaxLength)
{
  int iStrInputSize = StrInput.size();
  if((iStrInputSize <= 0) || (iTextMaxLength >= iStrInputSize))
    return false;

  char cDelim = '\0';
  size_t nGreaterDelim, nPos;

  nPos = StrInput.find_last_of( '\\' );
  if ( nPos != CStdString::npos )
    cDelim = '\\';
  else
  {
    nPos = StrInput.find_last_of( '/' );
    if ( nPos != CStdString::npos )
      cDelim = '/';
  }
  if ( cDelim == '\0' )
    return false;

  if (nPos == StrInput.size() - 1)
  {
    StrInput.erase(StrInput.size() - 1);
    nPos = StrInput.find_last_of( cDelim );
  }
  while( iTextMaxLength < iStrInputSize )
  {
    nPos = StrInput.find_last_of( cDelim, nPos );
    nGreaterDelim = nPos;
    if ( nPos != CStdString::npos )
      nPos = StrInput.find_last_of( cDelim, nPos - 1 );
    if ( nPos == CStdString::npos ) break;
    if ( nGreaterDelim > nPos ) StrInput.replace( nPos + 1, nGreaterDelim - nPos - 1, ".." );
    iStrInputSize = StrInput.size();
  }
  // replace any additional /../../ with just /../ if necessary
  CStdString replaceDots;
  replaceDots.Format("..%c..", cDelim);
  while (StrInput.size() > (unsigned int)iTextMaxLength)
    if (!StrInput.Replace(replaceDots, ".."))
      break;
  // finally, truncate our string to force inside our max text length,
  // replacing the last 2 characters with ".."

  // eg end up with:
  // "smb://../Playboy Swimsuit Cal.."
  if (iTextMaxLength > 2 && StrInput.size() > (unsigned int)iTextMaxLength)
  {
    StrInput = StrInput.Left(iTextMaxLength - 2);
    StrInput += "..";
  }
  StrOutput = StrInput;
  return true;
}

float CUtil::CurrentCpuUsage()
{
  return (1.0f - g_application.m_idleThread.GetRelativeUsage())*100;
}

bool CUtil::SupportsWriteFileOperations(const CStdString& strPath)
{
  // currently only hd,smb and dav support delete and rename
  if (URIUtils::IsHD(strPath))
    return true;
  if (URIUtils::IsSmb(strPath))
    return true;
  if (URIUtils::IsDAV(strPath))
    return true;
  if (URIUtils::IsMythTV(strPath))
  {
    /*
     * Can't use CFile::Exists() to check whether the myth:// path supports file operations because
     * it hits the directory cache on the way through, which has the Live Channels and Guide
     * items cached.
     */
    return CMythDirectory::SupportsWriteFileOperations(strPath);
  }
  if (URIUtils::IsStack(strPath))
    return SupportsWriteFileOperations(CStackDirectory::GetFirstStackedFile(strPath));
  if (URIUtils::IsMultiPath(strPath))
    return CMultiPathDirectory::SupportsWriteFileOperations(strPath);
#ifdef HAS_XBOX_HARDWARE
  if (URIUtils::IsMemCard(strPath) && g_memoryUnitManager.IsDriveWriteable(strPath))
    return true;
#endif
  return false;
}

bool CUtil::SupportsReadFileOperations(const CStdString& strPath)
{
  if (URIUtils::IsVideoDb(strPath))
    return false;

  return true;
}

CStdString CUtil::GetCachedAlbumThumb(const CStdString& album, const CStdString& artist)
{
  if (album.IsEmpty())
    return GetCachedMusicThumb("unknown"+artist);
  if (artist.IsEmpty())
    return GetCachedMusicThumb(album+"unknown");
  return GetCachedMusicThumb(album+artist);
}

CStdString CUtil::GetCachedMusicThumb(const CStdString& path)
{
  Crc32 crc;
  CStdString noSlashPath(path);
  URIUtils::RemoveSlashAtEnd(noSlashPath);
  crc.ComputeFromLowerCase(noSlashPath);
  CStdString hex;
  hex.Format("%08x", (unsigned __int32) crc);
  CStdString thumb;
  thumb.Format("%c/%s.tbn", hex[0], hex.c_str());
  return URIUtils::AddFileToFolder(g_settings.GetMusicThumbFolder(), thumb);
}

CStdString CUtil::GetDefaultFolderThumb(const CStdString &folderThumb)
{
  if (g_TextureManager.HasTexture(folderThumb))
    return folderThumb;
  return "";
}

void CUtil::GetSkinThemes(vector<CStdString>& vecTheme)
{
  CStdString strPath;
  URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(),"media",strPath);
  CFileItemList items;
  CDirectory::GetDirectory(strPath, items);
  // Search for Themes in the Current skin!
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      CStdString strExtension;
      URIUtils::GetExtension(pItem->GetPath(), strExtension);
      if (strExtension == ".xpr" && pItem->GetLabel().CompareNoCase("Textures.xpr"))
      {
        CStdString strLabel = pItem->GetLabel();
        vecTheme.push_back(strLabel.Mid(0, strLabel.size() - 4));
      }
    }
  }
  sort(vecTheme.begin(), vecTheme.end(), sortstringbyname());
}

void CUtil::WipeDir(const CStdString& strPath) // DANGEROUS!!!!
{
  if (!CDirectory::Exists(strPath)) return;

  CFileItemList items;
  GetRecursiveListing(strPath,items,"");
  for (int i=0;i<items.Size();++i)
  {
    if (!items[i]->m_bIsFolder)
      CFile::Delete(items[i]->GetPath());
  }
  items.Clear();
  GetRecursiveDirsListing(strPath,items);
  for (int i=items.Size()-1;i>-1;--i) // need to wipe them backwards
  {
    CStdString strDir = items[i]->GetPath();
    URIUtils::AddSlashAtEnd(strDir);
    CDirectory::Remove(strDir);
  }

  if (!URIUtils::HasSlashAtEnd(strPath))
  {
    CStdString tmpPath = strPath;
    URIUtils::AddSlashAtEnd(tmpPath);
    CDirectory::Remove(tmpPath);
  }
}

bool CUtil::PWMControl(const CStdString &strRGBa, const CStdString &strRGBb, const CStdString &strWhiteA, const CStdString &strWhiteB, const CStdString &strTransition, int iTrTime)
{
#ifdef HAS_XBOX_HARDWARE
    if (strRGBa.IsEmpty() && strRGBb.IsEmpty() && strWhiteA.IsEmpty() && strWhiteB.IsEmpty()) // no color, return false!
      return false;
  if(g_iledSmartxxrgb.IsRunning())
  {
    return g_iledSmartxxrgb.SetRGBState(strRGBa,strRGBb, strWhiteA, strWhiteB, strTransition, iTrTime);
  }
  g_iledSmartxxrgb.Start();
  return g_iledSmartxxrgb.SetRGBState(strRGBa,strRGBb, strWhiteA, strWhiteB, strTransition, iTrTime);
#else
  return false;
#endif
}

// We check if the MediaCenter-Video-patch is already installed.
// To do this we search for the original code in the Kernel.
// This is done by searching from 0x80011000 to 0x80024000.
bool CUtil::LookForKernelPatch()
{
#ifdef HAS_XBOX_HARDWARE
  BYTE	*Kernel=(BYTE *)0x80010000;
  DWORD	i, j = 0;

  for(i=0x1000; i<0x14000; i++)
  {
    if(Kernel[i]!=PatchData[0])
      continue;
    for(j=0; j<25; j++)
    {
      if(Kernel[i+j]!=PatchData[j])
        break;
    }
    if(j==25)
      return true;
  }
#endif
  return false;
}
// This routine removes our patch if it is not used.
// This is to ensure proper testing whether we are responsible
// for a mismatch of eeprom setting and current resolution.
void CUtil::RemoveKernelPatch()
{
#ifdef HAS_XBOX_HARDWARE
  BYTE  *Kernel=(BYTE *)0x80010000;
  DWORD i, j = 0;

  for(i=0x1000; i<0x14000; i++)
  {
    if(Kernel[i]!=PatchData[0])
      continue;

    for(j=0; j<25; j++)
    {
      if(Kernel[i+j]!=PatchData[j])
        break;
    }
    if(j==25)
    {
      j=MmQueryAddressProtect(&Kernel[i]);
      MmSetAddressProtect(&Kernel[i], 70, PAGE_READWRITE);
      memcpy(&Kernel[i], &rawData[0], 70); // Reset Kernel
      MmSetAddressProtect(&Kernel[i], 70, j);
    }
  }
#endif
}

void CUtil::BootToDash()
{
#ifdef HAS_XBOX_HARDWARE
  LD_LAUNCH_DASHBOARD ld;

  ZeroMemory(&ld, sizeof(LD_LAUNCH_DASHBOARD));

  ld.dwReason = XLD_LAUNCH_DASHBOARD_MAIN_MENU;
  XLaunchNewImage(0, (PLAUNCH_DATA)&ld);
#endif
}

void CUtil::InitRandomSeed()
{
  // Init random seed 
  LARGE_INTEGER now; 
  QueryPerformanceCounter(&now); 
  unsigned int seed = (now.LowPart);
//  CLog::Log(LOGDEBUG, "%s - Initializing random seed with %u", __FUNCTION__, seed);
  srand(seed);
}

void CUtil::RunShortcut(const char* szShortcutPath)
{
  CShortcut shortcut;
  char szPath[1024];
  char szParameters[1024];
  if ( shortcut.Create(szShortcutPath))
  {
    CFileItem item(shortcut.m_strPath, false);
    // if another shortcut is specified, load this up and use it
    if (item.IsShortCut())
    {
      CHAR szNewPath[1024];
      strcpy(szNewPath, szShortcutPath);
      CHAR* szFile = strrchr(szNewPath, '\\');
      strcpy(&szFile[1], shortcut.m_strPath.c_str());

      CShortcut targetShortcut;
      if (FAILED(targetShortcut.Create(szNewPath)))
        return;

      shortcut.m_strPath = targetShortcut.m_strPath;
    }

    strcpy( szPath, shortcut.m_strPath.c_str() );

    CHAR szMode[16];
    strcpy( szMode, shortcut.m_strVideo.c_str() );
    strlwr( szMode );

    strcpy(szParameters, shortcut.m_strParameters.c_str());

    BOOL bRow = strstr(szMode, "pal") != NULL;
    BOOL bJap = strstr(szMode, "ntsc-j") != NULL;
    BOOL bUsa = strstr(szMode, "ntsc") != NULL;

    F_VIDEO video = VIDEO_NULL;
    if (bRow)
      video = VIDEO_PAL50;
    if (bJap)
      video = (F_VIDEO)CXBE::FilterRegion(VIDEO_NTSCJ);
    if (bUsa)
      video = (F_VIDEO)CXBE::FilterRegion(VIDEO_NTSCM);

#ifdef HAS_XBOX_HARDWARE
    CUSTOM_LAUNCH_DATA data;
    if (!shortcut.m_strCustomGame.IsEmpty())
    {
      char remap_path[MAX_PATH] = "";
      char remap_xbe[MAX_PATH] = "";

      memset(&data,0,sizeof(CUSTOM_LAUNCH_DATA));

      strcpy(data.szFilename,shortcut.m_strCustomGame.c_str());

      strncpy(remap_path, XeImageFileName->Buffer, XeImageFileName->Length);
      for (int i = strlen(remap_path) - 1; i >=0; i--)
        if (remap_path[i] == '\\' || remap_path[i] == '/')
        {
          break;
        }
      strcpy(remap_xbe, &remap_path[i+1]);
      remap_path[i+1] = 0;

      strcpy(data.szRemap_D_As, remap_path);
      strcpy(data.szLaunchXBEOnExit, remap_xbe);

      data.executionType = 0;

      // not the actual "magic" value - used to pass XbeId for some reason?
      data.magic = GetXbeID(szPath);
    }

    CUtil::RunXBE(szPath,strcmp(szParameters,"")?szParameters:NULL,video,COUNTRY_NULL,shortcut.m_strCustomGame.IsEmpty()?NULL:&data);
#endif
  }
}

void CUtil::GetHomePath(CStdString& strPath)
{
  char szXBEFileName[1024];
  CIoSupport::GetXbePath(szXBEFileName);
  char *szFileName = strrchr(szXBEFileName, '\\');
  *szFileName = 0;
  strPath = szXBEFileName;
}

bool CUtil::RunFFPatchedXBE(CStdString szPath1, CStdString& szNewPath)
{
  if (!g_guiSettings.GetBool("myprograms.autoffpatch"))
  {
    CLog::Log(LOGDEBUG, "%s - Auto Filter Flicker is off. Skipping Filter Flicker Patching.", __FUNCTION__);
    return false;
  }
  CStdString strIsPMode = g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].strMode;
  if ( strIsPMode.Equals("480p 16:9") || strIsPMode.Equals("480p 4:3") || strIsPMode.Equals("720p 16:9"))
  {
    CLog::Log(LOGDEBUG, "%s - Progressive Mode detected: Skipping Auto Filter Flicker Patching!", __FUNCTION__);
    return false;
  }
  if (strncmp(szPath1, "D:", 2) == 0)
  {
    CLog::Log(LOGDEBUG, "%s - Source is DVD-ROM! Skipping Filter Flicker Patching.", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGDEBUG, "%s - Auto Filter Flicker is ON. Starting Filter Flicker Patching.", __FUNCTION__);

  // Test if we already have a patched _ffp XBE
  // Since the FF can be changed in XBMC, we will not check for a pre patched _ffp xbe!
  /* // May we can add. a changed FF detection.. then we can actived this!
  CFile	xbe;
	if (xbe.Exists(szPath1))
  {
    char szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szFname[_MAX_FNAME], szExt[_MAX_EXT];
		_splitpath(szPath1, szDrive, szDir, szFname, szExt);
		strncat(szFname, "_ffp", 4);
		_makepath(szNewPath.GetBuffer(MAX_PATH), szDrive, szDir, szFname, szExt);
		szNewPath.ReleaseBuffer();
		if (xbe.Exists(szNewPath))
			return true;
	} */


  CXBE m_xbe;
  if((int)m_xbe.ExtractGameRegion(szPath1.c_str()) <= 0) // Reading the GameRegion is enought to detect a Patchable xbe!
  {
    CLog::Log(LOGDEBUG, "%s - %s",szPath1.c_str(), __FUNCTION__);
    CLog::Log(LOGDEBUG, "%s - Not Patchable xbe detected (Homebrew?)! Skipping Filter Flicker Patching.", __FUNCTION__);
    return false;
  }
#ifdef HAS_XBOX_HARDWARE
  CGFFPatch m_ffp;
  if (!m_ffp.FFPatch(szPath1, szNewPath))
  {
    CLog::Log(LOGDEBUG, "%s - ERROR during Filter Flicker Patching. Falling back to the original source.", __FUNCTION__);
    return false;
  }
#endif
  if(szNewPath.IsEmpty())
  {
    CLog::Log(LOGDEBUG, "%s - ERROR NO Patchfile Path is empty! Falling back to the original source.", __FUNCTION__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s - Filter Flicker Patching done. Starting %s.", __FUNCTION__, szNewPath.c_str());
  return true;
}

void CUtil::RunXBE(const char* szPath1, char* szParameters, F_VIDEO ForceVideo, F_COUNTRY ForceCountry, CUSTOM_LAUNCH_DATA* pData)
{
  // check if locked
  if (g_settings.GetCurrentProfile().programsLocked() && 
      g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return;

  /// \brief Runs an executable file
  /// \param szPath1 Path of executeable to run
  /// \param szParameters Any parameters to pass to the executeable being run
  g_application.PrintXBEToLCD(szPath1); //write to LCD
  Sleep(600);        //and wait a little bit to execute

  char szPath[1024];
  strcpy(szPath, _P(szPath1).c_str());

  CStdString szNewPath;
  if (RunFFPatchedXBE(szPath, szNewPath))
  {
    strcpy(szPath, szNewPath.c_str());
  }
  
  if (strncmp(szPath, "Q:", 2) == 0)
  { // may aswell support the virtual drive as well...
    CStdString strPath;
    // home dir is xbe dir
    GetHomePath(strPath);
    if (!URIUtils::HasSlashAtEnd(strPath))
      strPath += "\\";
    if (szPath[2] == '\\')
      strPath += szPath + 3;
    else
      strPath += szPath + 2;
    strcpy(szPath, strPath.c_str());
  }
  
  char* szBackslash = strrchr(szPath, '\\');
  if (szBackslash)
  {
    *szBackslash = 0x00;
    char* szXbe = &szBackslash[1];

    char* szColon = strrchr(szPath, ':');
    if (szColon)
    {
      *szColon = 0x00;
      char* szDrive = szPath;
      char* szDirectory = &szColon[1];

      char szDevicePath[1024];
      char szXbePath[1024];

      CIoSupport::GetPartition(szDrive[0], szDevicePath);

      strcat(szDevicePath, szDirectory);
      wsprintf(szXbePath, "d:\\%s", szXbe);

#ifdef HAS_XBOX_HARDWARE
      g_application.Stop(false);

      CUtil::LaunchXbe(szDevicePath, szXbePath, szParameters, ForceVideo, ForceCountry, pData);
#endif
    }
  }

  CLog::Log(LOGERROR, "Unable to run xbe : %s", szPath);
}

void CUtil::LaunchXbe(const char* szPath, const char* szXbe, const char* szParameters, F_VIDEO ForceVideo, F_COUNTRY ForceCountry, CUSTOM_LAUNCH_DATA* pData)
{
  CStdString strPath(_P(szPath));
  CLog::Log(LOGINFO, "launch xbe:%s %s", strPath.c_str(), szXbe);
  CLog::Log(LOGINFO, " mount %s as D:", strPath.c_str());

#ifdef HAS_XBOX_HARDWARE
  CIoSupport::RemapDriveLetter('D', const_cast<char*>(strPath.c_str()));

  CLog::Log(LOGINFO, "launch xbe:%s", szXbe);

  if (ForceVideo != VIDEO_NULL)
  {
    if (!ForceCountry)
    {
      if (ForceVideo == VIDEO_NTSCM)
        ForceCountry = COUNTRY_USA;
      if (ForceVideo == VIDEO_NTSCJ)
        ForceCountry = COUNTRY_JAP;
      if (ForceVideo == VIDEO_PAL50)
        ForceCountry = COUNTRY_EUR;
    }
    CLog::Log(LOGDEBUG,"forcing video mode: %i",ForceVideo);
    bool bSuccessful = PatchCountryVideo(ForceCountry, ForceVideo);
    if( !bSuccessful )
      CLog::Log(LOGINFO,"AutoSwitch: Failed to set mode");
  }
  if (pData)
  {
    DWORD dwTitleID = pData->magic;
    pData->magic = CUSTOM_LAUNCH_MAGIC;
    const char* xbe = szXbe+3;
    CLog::Log(LOGINFO, "launching game %s from path %s", pData->szFilename, strPath.c_str());
    CIoSupport::UnmapDriveLetter('D');
    XWriteTitleInfoAndRebootA( (char*)xbe, (char*)(CStdString("\\Device\\")+strPath).c_str(), LDT_TITLE, dwTitleID, pData);
  }
  else
  {
    if (szParameters == NULL)
    {
      DWORD error = XLaunchNewImage(szXbe, NULL );
      CLog::Log(LOGERROR, "%s - XLaunchNewImage returned with error code %d", __FUNCTION__, error);
    }
    else
    {
      LAUNCH_DATA LaunchData;
      strcpy((char*)LaunchData.Data, szParameters);

      DWORD error = XLaunchNewImage(szXbe, &LaunchData );
      CLog::Log(LOGERROR, "%s - XLaunchNewImage returned with error code %d", __FUNCTION__, error);
    }
  }
#endif
}

int CUtil::LookupRomanDigit(char roman_digit)
{
  switch (roman_digit)
  {
    case 'i':
    case 'I':
      return 1;
    case 'v':
    case 'V':
      return 5;
    case 'x':
    case 'X':
      return 10;
    case 'l':
    case 'L':
      return 50;
    case 'c':
    case 'C':
      return 100;
    case 'd':
    case 'D':
      return 500;
    case 'm':
    case 'M':
      return 1000;
    default:
      return 0;
  }
}

int CUtil::TranslateRomanNumeral(const char* roman_numeral)
{
  
  int decimal = -1;

  if (roman_numeral && roman_numeral[0])
  {
    int temp_sum  = 0,
        last      = 0,
        repeat    = 0,
        trend     = 1;
    decimal = 0;
    while (*roman_numeral)
    {
      int digit = CUtil::LookupRomanDigit(*roman_numeral);
      int test  = last;
      
      // General sanity checks

      // numeral not in LUT
      if (!digit)
        return -1;
      
      while (test > 5)
        test /= 10;
      
      // N = 10^n may not precede (N+1) > 10^(N+1)
      if (test == 1 && digit > last * 10)
        return -1;
      
      // N = 5*10^n may not precede (N+1) >= N
      if (test == 5 && digit >= last) 
        return -1;

      // End general sanity checks

      if (last < digit)
      {
        // smaller numerals may not repeat before a larger one
        if (repeat) 
          return -1;

        temp_sum += digit;
        
        repeat  = 0;
        trend   = 0;
      }
      else if (last == digit)
      {
        temp_sum += digit;
        repeat++;
        trend = 1;
      }
      else
      {
        if (!repeat)
          decimal += 2 * last - temp_sum;
        else
          decimal += temp_sum;
        
        temp_sum = digit;

        trend   = 1;
        repeat  = 0;
      }
      // Post general sanity checks

      // numerals may not repeat more than thrice
      if (repeat == 3)
        return -1;

      last = digit;
      roman_numeral++;
    }

    if (trend)
      decimal += temp_sum;
    else
      decimal += 2 * last - temp_sum;
  }
  return decimal;
}
