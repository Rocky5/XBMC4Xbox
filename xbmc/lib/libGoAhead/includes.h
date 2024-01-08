#pragma once

#ifndef WEBS
#define WEBS
#endif
#ifndef UEMF
#define UEMF
#endif
#ifndef DIGEST_ACCESS_SUPPORT
#define DIGEST_ACCESS_SUPPORT
#endif
#ifndef DEV
#define DEV
#endif
#ifndef WIN
#define WIN
#endif
#ifndef USER_MANAGEMENT_SUPPORT
#define USER_MANAGEMENT_SUPPORT
#endif
#ifndef __NO_CGI_BIN
#define __NO_CGI_BIN
#endif

#define SOCK_DFT_SVC_TIME	1000

#ifdef __cplusplus
extern "C" {
#endif

#include	"wsIntrn.h"

#ifdef WEBS_SSL_SUPPORT
#include	"websSSL.h"
#endif

#ifdef USER_MANAGEMENT_SUPPORT
#include	"um.h"
	void		formDefineUserMgmt(void);
#endif

#if defined(__cplusplus)
}
#endif 