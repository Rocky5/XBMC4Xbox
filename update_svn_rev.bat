@echo off
REM subwcrev is included in the tortoise svn client: http://tortoisesvn.net/downloads

REM Current directory including drive
SET CWD=%~dp0

SET REV_FILE="%CWD%xbmc\xbox\svn_rev.h"
SET SVN_TEMPLATE="%CWD%xbmc\xbox\svn_rev.tmpl"

IF EXIST %REV_FILE% del %REV_FILE%

SET SUBWCREV=""

REM IF EXIST "%ProgramFiles(x86)%\TortoiseSVN\bin\subwcrev.exe" SET SUBWCREV="%ProgramFiles(x86)%\TortoiseSVN\bin\subwcrev.exe"
REM IF EXIST "%ProgramFiles%\TortoiseSVN\bin\subwcrev.exe"      SET SUBWCREV="%ProgramFiles%\TortoiseSVN\bin\subwcrev.exe"
REM IF EXIST "%ProgramW6432%\TortoiseSVN\bin\subwcrev.exe" SET SUBWCREV="%ProgramW6432%\TortoiseSVN\bin\subwcrev.exe"

REM IF NOT EXIST %SUBWCREV% (
   REM ECHO subwcrev.exe not found in expected locations, skipping generation
   REM GOTO SKIPSUBWCREV
REM )

REM %SUBWCREV% %SVN_TEMPLATE% %REV_FILE% -f

REM Generate the SVN revision header if it does not exist
IF NOT EXIST %REV_FILE% (
   ECHO Generating SVN revision header from SVN repo
   (
   Echo #ifndef __SVN_REV__H__
   Echo #define __SVN_REV__H__
   Echo /*
   Echo *      Copyright ^(C^) 2005-2013 Team XBMC
   Echo *      http://xbmc.org
   Echo *
   Echo *  This Program is free software; you can redistribute it and/or modify
   Echo *  it under the terms of the GNU General Public License as published by
   Echo *  the Free Software Foundation; either version 2, or ^(at your option^)
   Echo *  any later version.
   Echo *
   Echo *  This Program is distributed in the hope that it will be useful,
   Echo *  but WITHOUT ANY WARRANTY; without even the implied warranty of
   Echo *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   Echo *  GNU General Public License for more details.
   Echo *
   Echo *  You should have received a copy of the GNU General Public License
   Echo *  along with XBMC; see the file COPYING.  If not, see
   Echo *  ^<http://www.gnu.org/licenses/^>.
   Echo *
   Echo */
   Echo #define SVN_REV   "33046"
   Echo #define SVN_DATE  "2023/01/14 18:16:15"
   Echo #endif
   )>%REV_FILE%
   REM %SUBWCREV% "%CWD%." %SVN_TEMPLATE% %REV_FILE% -f
)

REM :SKIPSUBWCREV

REM Copy the default unknown revision header if the generation did not occur
REM IF NOT EXIST %REV_FILE% (
   REM ECHO using default svn revision unknown header
   REM copy %CWD%xbmc\xbox\svn_rev.unknown %REV_FILE%
REM )

REM SET REV_FILE=
REM SET SUBWCREV=