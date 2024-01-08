/*	MikMod sound library
	(c) 1998-2001 Miodrag Vallat and others - see file AUTHORS for
	complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.
 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.
 
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id: drv_ds.cpp 31144 2012-07-15 15:38:56Z buzz $

  Driver for output on win32 platforms using DirectSound

==============================================================================*/

/*

	Written by Brian McKinney <Brian.McKinney@colorado.edu>
	Xbox'd by Pete Knepley
*/

/*
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
*/

#include "mikmod_internals.h"

#ifdef DRV_DS

#include <memory.h>
#include <string.h>

#define INITGUID
#include <xtl.h>
#include <stdio.h>

/* DSBCAPS_CTRLALL is not defined anymore with DirectX 7. Of course DirectSound
   is a coherent, backwards compatible API... */
#ifndef DSBCAPS_CTRLALL
//CTRLPAN doesn't exist on xbox 
#define DSBCAPS_CTRLALL ( DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_CTRLVOLUME | \
				          /*DSBCAPS_CTRLPAN |*/ DSBCAPS_CTRLFREQUENCY /*| \ DSBCAPS_CTRL3D */ )

#endif

/* size of each buffer */
#define FRAGSIZE 16
/* buffer count */
#define UPDATES  2

/* sleep time (in milliseconds) */
#define	SLEEPINTERVAL	50		/* 20Hz */

static LPDIRECTSOUND8 pSoundCard = NULL;
static LPDIRECTSOUNDBUFFER8 pPrimarySoundBuffer = NULL; 
static LPDIRECTSOUNDBUFFER8 pSoundBuffer = NULL;
static LPDIRECTSOUNDNOTIFY pSoundBufferNotify = NULL;

static HANDLE notifyUpdateHandle = 0, updateBufferHandle = 0;
static DWORD updateBufferThreadID = 0, soundBufferCurrentPosition = 0;
static DWORD blockBytes1 = 0,blockBytes2 = 0;
static LPVOID pBlock1 = NULL, pBlock2 = NULL;
static BOOL threadInUse = FALSE;
static int fragsize=1<<FRAGSIZE;

//global focus doesn't exist on xbox
static DWORD controlflags = DSBCAPS_CTRLALL;// & ~DSBCAPS_GLOBALFOCUS;

static BOOL threadRunning = FALSE;

static DWORD WINAPI updateBufferProc(LPVOID lpParameter)
{
	threadRunning = TRUE;

	while(threadInUse) {
		if(WaitForSingleObject(notifyUpdateHandle,SLEEPINTERVAL)==WAIT_OBJECT_0) {
			
			pSoundBuffer->GetCurrentPosition(&soundBufferCurrentPosition,NULL);
		
			if(soundBufferCurrentPosition<fragsize) {
					pSoundBuffer->Lock
				          (fragsize,fragsize,&pBlock1,&blockBytes1,
				           &pBlock2,&blockBytes2,0);
				//Xbox can never lose the sound buffer
				/*
				if(pSoundBuffer->Lock
				          (fragsize,fragsize,&pBlock1,&blockBytes1,
				           &pBlock2,&blockBytes2,0)==DSERR_BUFFERLOST) {	
					pSoundBuffer->lpVtbl->Restore(pSoundBuffer);
					pSoundBuffer->lpVtbl->Lock
					      (pSoundBuffer,fragsize,fragsize,&pBlock1,&blockBytes1,
					       &pBlock2,&blockBytes2,0);
				}
				*/
				MUTEX_LOCK(vars);
					VC_WriteBytes((SBYTE*)pBlock1,(ULONG)blockBytes1);
					if(pBlock2)
						VC_WriteBytes((SBYTE*)pBlock2,(ULONG)blockBytes2);
				MUTEX_UNLOCK(vars);

				pSoundBuffer->Unlock
				         (pBlock1,blockBytes1,pBlock2,blockBytes2);
			} else {
				pSoundBuffer->Lock
				                 (0,fragsize,&pBlock1,&blockBytes1,
				                  &pBlock2,&blockBytes2,0);
				//xbox never loses the buffer
				/*
				if(pSoundBuffer->Lock
				                 (0,fragsize,&pBlock1,&blockBytes1,
				                  &pBlock2,&blockBytes2,0)==DSERR_BUFFERLOST) {	
					pSoundBuffer->Restore();
					pSoundBuffer->Lock
					             (0,fragsize,&pBlock1,&blockBytes1,
					              &pBlock2,&blockBytes2,0);
				}
				*/
				if(Player_Paused()) {
					MUTEX_LOCK(vars);
						VC_SilenceBytes((SBYTE*)pBlock1,(ULONG)blockBytes1);
						if(pBlock2)
							VC_SilenceBytes((SBYTE*)pBlock2,(ULONG)blockBytes2);
					MUTEX_UNLOCK(vars);
				} else {
					MUTEX_LOCK(vars);
						VC_WriteBytes((SBYTE*)pBlock1,(ULONG)blockBytes1);
						if(pBlock2)
							VC_WriteBytes((SBYTE*)pBlock2,(ULONG)blockBytes2);
					MUTEX_UNLOCK(vars);
				}
			
				pSoundBuffer->Unlock
				         (pBlock1,blockBytes1,pBlock2,blockBytes2);
			}
		}
	}
	threadRunning = FALSE;
	return 0;
}

static void DS_WaitForThreadEnd(void)
{
	threadInUse = 0;
	if (updateBufferHandle && threadRunning)
			while (threadRunning == TRUE)
					Sleep(0);
	if (pSoundBuffer)
		pSoundBuffer->Stop();
}

static void DS_CommandLine(CHAR *cmdline)
{
	CHAR *ptr=MD_GetAtom("buffer",cmdline,0);

	if(ptr) {
		int buf=atoi(ptr);

		if((buf<12)||(buf>19)) buf=FRAGSIZE;
		fragsize=1<<buf;

		free(ptr);
	}
	
	//global focus doesn't exist in xbox
	/*
	if ((ptr=MD_GetAtom("globalfocus",cmdline,1))) {
		controlflags |= DSBCAPS_GLOBALFOCUS;
		free(ptr);
	} else
		controlflags &= ~DSBCAPS_GLOBALFOCUS;
		*/
}

static BOOL DS_IsPresent(void)
{
	if(DirectSoundCreate(NULL,&pSoundCard,NULL)!=DS_OK)
		return 0;
	pSoundCard->Release();
	return 1;
}

static BOOL DS_Init(void)
{
	DSBUFFERDESC soundBufferFormat;
	WAVEFORMATEX pcmwf;
	DSBPOSITIONNOTIFY positionNotifications[2];

	if(DirectSoundCreate(NULL,&pSoundCard,NULL)!=DS_OK) {
		_mm_errno=MMERR_OPENING_AUDIO;
		return 1;
	}

	//XBOX OWNS THE SOUND CARD MOTHERFUCKER!
	/*
	if(pSoundCard->SetCooperativeLevel
	                 (GetForegroundWindow(),DSSCL_PRIORITY)!=DS_OK) {
		_mm_errno=MMERR_DS_PRIORITY;
		return 1;
	}
	*/

	memset(&pcmwf,0,sizeof(WAVEFORMATEX));
    pcmwf.wFormatTag     =WAVE_FORMAT_PCM;
    pcmwf.nChannels      =(md_mode&DMODE_STEREO)?2:1;
    pcmwf.nSamplesPerSec =md_mixfreq;
    pcmwf.wBitsPerSample =(md_mode&DMODE_16BITS)?16:8;
    pcmwf.nBlockAlign    =(pcmwf.wBitsPerSample * pcmwf.nChannels) / 8;
    pcmwf.nAvgBytesPerSec=pcmwf.nSamplesPerSec*pcmwf.nBlockAlign;
   
	memset(&soundBufferFormat,0,sizeof(DSBUFFERDESC));
    soundBufferFormat.dwSize       =sizeof(DSBUFFERDESC);
	soundBufferFormat.dwFlags = 0;
	//xbox don't need a fucking primary buffer
//    soundBufferFormat.dwFlags      =DSBCAPS_PRIMARYBUFFER;
    soundBufferFormat.dwFlags      =controlflags;
   // soundBufferFormat.dwBufferBytes=0;                                
    soundBufferFormat.dwBufferBytes=fragsize*UPDATES;
    soundBufferFormat.lpwfxFormat  =&pcmwf;
	//xbox no like NULL format
	//soundBufferFormat.lpwfxFormat  =NULL; 
	
	if(pSoundCard->CreateSoundBuffer
	         (&soundBufferFormat,&pPrimarySoundBuffer,NULL)!=DS_OK) {
		_mm_errno=MMERR_DS_BUFFER;
		return 1;
	}
	//no need for this primary buffer lame shit
/*
    if(pPrimarySoundBuffer->SetFormat(&pcmwf)!=DS_OK) {
		_mm_errno=MMERR_DS_FORMAT;
		return 1;
	}
	*/
    pPrimarySoundBuffer->Play(0,0,DSBPLAY_LOOPING);
   	
	//keep the same format
	/*
	memset(&soundBufferFormat,0,sizeof(DSBUFFERDESC));
    soundBufferFormat.dwSize       =sizeof(DSBUFFERDESC);
	//taken out for xbox
	soundBufferFormat.dwFlags = 0;
//    soundBufferFormat.dwFlags      =controlflags|DSBCAPS_GETCURRENTPOSITION2 ;
    soundBufferFormat.dwBufferBytes=fragsize*UPDATES;
    soundBufferFormat.lpwfxFormat  =&pcmwf;
	*/

	if(pSoundCard->CreateSoundBuffer(&soundBufferFormat,&pSoundBuffer,NULL)!=DS_OK) {
		_mm_errno=MMERR_DS_BUFFER;
		return 1;
	}

	
	

	notifyUpdateHandle=CreateEvent
	     (NULL,FALSE,FALSE,"libmikmod DirectSound Driver positionNotify Event");
	if(!notifyUpdateHandle) {
		_mm_errno=MMERR_DS_EVENT;
		return 1;
	}

	updateBufferHandle=CreateThread
	      (NULL,0,updateBufferProc,NULL,CREATE_SUSPENDED,&updateBufferThreadID);
	if(!updateBufferHandle) {
		_mm_errno=MMERR_DS_THREAD;
		return 1;
	}

	memset(positionNotifications,0,2*sizeof(DSBPOSITIONNOTIFY));
	positionNotifications[0].dwOffset    =0;
	positionNotifications[0].hEventNotify=notifyUpdateHandle;
	positionNotifications[1].dwOffset    =fragsize;
	positionNotifications[1].hEventNotify=notifyUpdateHandle;
	//on xbox the buffer can just be notified
	if(pSoundBuffer->SetNotificationPositions
	                     (2,positionNotifications)!=DS_OK) {
		_mm_errno=MMERR_DS_UPDATE;
		return 1;
	}
	
	if(VC_Init())
		return 1;

	return 0;
}

static void DS_Exit(void)
{
	DWORD statusInfo;

	DS_WaitForThreadEnd();

	if(updateBufferHandle) {
		CloseHandle(updateBufferHandle),
		updateBufferHandle = 0;
	}
	if (notifyUpdateHandle) {
		CloseHandle(notifyUpdateHandle),
		notifyUpdateHandle = 0;
	}
	
	VC_Exit();

	pSoundBufferNotify->Release();
	if(pSoundBuffer) {
		if(pSoundBuffer->GetStatus(&statusInfo)==DS_OK)
			if(statusInfo&DSBSTATUS_PLAYING)
				pSoundBuffer->Stop();
		pSoundBuffer->Release();
	}

	if(pPrimarySoundBuffer) {
		if(pPrimarySoundBuffer->GetStatus
		                               (&statusInfo)==DS_OK)
			if(statusInfo&DSBSTATUS_PLAYING)
				pPrimarySoundBuffer->Stop();
		pPrimarySoundBuffer->Release();
	}

	pSoundCard->Release();
}

static void DS_Update(void)
{
	return;
}

static void DS_PlayStop(void)
{
	DS_WaitForThreadEnd();

	VC_PlayStop();
}

static BOOL DS_PlayStart(void)
{
	pSoundBuffer->Play(0,0,DSBPLAY_LOOPING);
	threadInUse=1;
	threadRunning = TRUE;
	ResumeThread(updateBufferHandle);
	VC_PlayStart();

	return 0;
}

MIKMODAPI MDRIVER drv_ds=
{
	NULL,
	"DirectSound",
	"DirectSound Driver (DX6+) v0.3",
	0,255,
	"ds",

	DS_CommandLine,
	DS_IsPresent,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	DS_Init,
	DS_Exit,
	NULL,
	VC_SetNumVoices,
	DS_PlayStart,
	DS_PlayStop,
	DS_Update,
	NULL,
	VC_VoiceSetVolume,
	VC_VoiceGetVolume,
	VC_VoiceSetFrequency,
	VC_VoiceGetFrequency,
	VC_VoiceSetPanning,
	VC_VoiceGetPanning,
	VC_VoicePlay,
	VC_VoiceStop,
	VC_VoiceStopped,
	VC_VoiceGetPosition,
	VC_VoiceRealVolume
};

#else

MISSING(drv_ds);

#endif

/* ex:set ts=4: */
