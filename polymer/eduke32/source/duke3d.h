//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#ifndef __duke3d_h__
#define __duke3d_h__

#ifdef __cplusplus
extern "C" {
#endif

// JBF
#include "compat.h"
#include "a.h"
#include "build.h"

#ifdef POLYMER
    #include "polymer.h"
#else
#ifdef POLYMOST
    #include "polymost.h"
#endif
#endif

#include "cache1d.h"
#include "pragmas.h"
#include "baselayer.h"
#include "file_lib.h"
#include "keyboard.h"
#include "mathutil.h"
#include "fx_man.h"

#define APPNAME             "EDuke32"
#define VERSION             "2.0.0devel"
#define HEAD2               APPNAME" "VERSION

#define GAMEDUKE            0
#define GAMENAM             1
#define GAMEWW2             3

#define VOLUMEALL           (g_Shareware == 0)
#define PLUTOPAK            (g_scriptVersion == 14)
#define VOLUMEONE           (g_Shareware == 1)

#define NAM                 (g_gameType & 1)
#define WW2GI               (g_gameType & 2)

// increase by 3, because atomic GRP adds 1, and Shareware adds 2
#define BYTEVERSION_JF      195
#define BYTEVERSION_13      27
#define BYTEVERSION_14      116
#define BYTEVERSION_15      117
#define BYTEVERSION         (BYTEVERSION_JF+(PLUTOPAK?1:(VOLUMEONE<<1)))

#define NUMPAGES            1

#define RECSYNCBUFSIZ       2520   //2520 is the (LCM of 1-8)*3
#define MOVEFIFOSIZ         2

#define MAXVOLUMES          7
#define MAXLEVELS           32
#define MAXGAMETYPES        16

// used as a constant to satisfy all of the calculations written with ticrate = 26 in mind
#define GAMETICSPERSEC      26
#define REALGAMETICSPERSEC  30
// this used to be TICRATE/GAMETICSPERSEC, which was 120/26 = 4.615~ truncated to 4 by integer division
#define TICSPERFRAME        4
#define TICRATE             120

#define MAXQUOTES           16384
#define MAXQUOTELEN         128
#define OBITQUOTEINDEX      MAXQUOTES-128
#define SUICIDEQUOTEINDEX   MAXQUOTES-32

#define NO                  0
#define YES                 1

#define PACKBUF_SIZE        65535

#define TILE_SAVESHOT       (MAXTILES-1)
#define TILE_LOADSHOT       (MAXTILES-3)
#define TILE_TILT           (MAXTILES-2)
#define TILE_ANIM           (MAXTILES-4)
#define TILE_VIEWSCR        (MAXTILES-5)

// JBF 20040604: sync is a function on some platforms
#define sync                dsync

#include "namesdyn.h"
#include "function.h"
#include "macros.h"
#include "gamedefs.h"
#include "config.h"
#include "sounds.h"
#include "control.h"
#include "_rts.h"
#include "rts.h"
#include "soundefs.h"
#include "music.h"
#include "player.h"
#include "actors.h"
#include "global.h"
#include "sector.h"
#include "net.h"
#include "game.h"
#include "gamedef.h"
#include "gameexec.h"
#include "gamevars.h"

#ifdef __cplusplus
}
#endif
#endif
