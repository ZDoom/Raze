/*
** version.h
**
**---------------------------------------------------------------------------
** Copyright 1998-2007 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#ifndef __VERSION_H__
#define __VERSION_H__

const char *GetGitDescription();
const char *GetGitHash();
const char *GetGitTime();
const char *GetVersionString();

/** Lots of different version numbers **/

#define VERSIONSTR "1.5pre"

// The version as seen in the Windows resource
#define RC_FILEVERSION 1,5,9999,0
#define RC_PRODUCTVERSION 1,5,9999,0
#define RC_PRODUCTVERSION2 VERSIONSTR
// These are for content versioning.
#define VER_MAJOR 4
#define VER_MINOR 7
#define VER_REVISION 0

#define ENG_MAJOR 1
#define ENG_MINOR 5
#define ENG_REVISION 0

// More stuff that needs to be different for derivatives.
#define GAMENAME "Raze"
#define WGAMENAME L"Raze"
#define GAMENAMELOWERCASE "raze"
#define FORUM_URL "http://forum.zdoom.org/"
#define BUGS_FORUM_URL	"http://forum.zdoom.org/viewforum.php?f=340"
#define ENGINERES_FILE GAMENAMELOWERCASE ".pk3"

#define SAVESIG_DN3D GAMENAME ".Duke"
#define SAVESIG_BLD GAMENAME ".Blood"
#define SAVESIG_SW GAMENAME ".ShadowWarrior"
#define SAVESIG_PS GAMENAME ".Exhumed"

#define MINSAVEVER_DN3D 16
#define MINSAVEVER_BLD 16
#define MINSAVEVER_SW 17
#define MINSAVEVER_PS 16

#define SAVEVER_DN3D 16
#define SAVEVER_BLD 16
#define SAVEVER_SW 17
#define SAVEVER_PS 16

#define NETGAMEVERSION 1

#if defined(__APPLE__) || defined(_WIN32)
#define GAME_DIR GAMENAME
#else
#define GAME_DIR ".config/" GAMENAMELOWERCASE
#endif

#define DEFAULT_DISCORD_APP_ID "954282576464449556"

const int SAVEPICWIDTH = 240;
const int SAVEPICHEIGHT = 180;
const int VID_MIN_WIDTH = 640;
const int VID_MIN_HEIGHT = 400;
#define NPOT_EMULATION

#endif //__VERSION_H__
