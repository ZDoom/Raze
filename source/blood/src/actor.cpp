//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
//#include <iostream>

#include "ns.h"	// Must come before everything else!


#include "build.h"
#include "pragmas.h"
#include "mmulti.h"
#include "common.h"
#include "common_game.h"

#include "actor.h"
#include "ai.h"
#include "aibat.h"
#include "aibeast.h"
#include "aiboneel.h"
#include "aiburn.h"
#include "aicaleb.h"
#include "aicerber.h"
#include "aicult.h"
#include "aigarg.h"
#include "aighost.h"
#include "aigilbst.h"
#include "aihand.h"
#include "aihound.h"
#include "aiinnoc.h"
#include "aipod.h"
#include "airat.h"
#include "aispid.h"
#include "aitchern.h"
#include "aizomba.h"
#include "aizombf.h"
#include "aiunicult.h"
#include "blood.h"
#include "callback.h"
#include "config.h"
#include "db.h"
#include "endgame.h"
#include "eventq.h"
#include "fx.h"
#include "gameutil.h"
#include "gib.h"
#include "globals.h"
#include "levels.h"
#include "loadsave.h"
#include "player.h"
#include "seq.h"
#include "sfx.h"
#include "sound.h"
#include "tile.h"
#include "trig.h"
#include "triggers.h"
#include "view.h"
#include "warp.h"
#include "weapon.h"

BEGIN_BLD_NS

VECTORDATA gVectorData[] = {
    // Tine
    {
        DAMAGE_TYPE_2,
        17,
        174762,
        1152,
        10240,
        0,
        1,
        20480,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_43, FX_5, FX_NONE, 500,
        FX_NONE, FX_5, FX_NONE, 501,
        FX_43, FX_6, FX_NONE, 502,
        FX_43, FX_0, FX_NONE, 503,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_7, 502,
        FX_43, FX_6, FX_7, 502,
        FX_NONE, FX_NONE, FX_NONE, 503,
        FX_43, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 503,
        FX_NONE, FX_6, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, -1,
        1207, 1207,
    },
    // Shell
    {
        DAMAGE_TYPE_2,
        4,
        65536,
        0,
        8192,
        0,
        1,
        12288,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_43, FX_5, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, 501,
        FX_43, FX_6, FX_NONE, -1,
        FX_43, FX_0, FX_NONE, -1,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, -1,
        FX_43, FX_6, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_43, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, -1,
        1001, 1001,
    },
    // Bullet
    {
        DAMAGE_TYPE_2,
        7,
        21845,
        0,
        32768,
        0,
        1,
        12288,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_43, FX_5, FX_7, 510,
        FX_NONE, FX_5, FX_7, 511,
        FX_43, FX_6, FX_NONE, 512,
        FX_43, FX_0, FX_NONE, 513,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_7, 512,
        FX_43, FX_6, FX_7, 512,
        FX_NONE, FX_NONE, FX_NONE, 513,
        FX_43, FX_NONE, FX_NONE, 513,
        FX_NONE, FX_6, FX_NONE, 513,
        FX_NONE, FX_6, FX_NONE, 513,
        FX_NONE, FX_6, FX_NONE, 513,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, -1,
        4001, 4002,
    },
    // Tommy AP
    {
        DAMAGE_TYPE_2,
        20,
        65536,
        0,
        16384,
        0,
        1,
        20480,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_43, FX_5, FX_7, 510,
        FX_NONE, FX_5, FX_7, 511,
        FX_43, FX_6, FX_NONE, 512,
        FX_43, FX_0, FX_NONE, 513,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_7, 512,
        FX_43, FX_6, FX_7, 512,
        FX_NONE, FX_NONE, FX_NONE, 513,
        FX_43, FX_NONE, FX_NONE, 513,
        FX_NONE, FX_6, FX_NONE, 513,
        FX_NONE, FX_6, FX_NONE, 513,
        FX_NONE, FX_6, FX_NONE, 513,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, -1,
        431, 431
    },
    // Shell AP
    {
        DAMAGE_TYPE_2,
        6,
        87381,
        0,
        12288,
        0,
        1,
        6144,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_43, FX_5, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, 501,
        FX_43, FX_6, FX_NONE, -1,
        FX_43, FX_0, FX_NONE, -1,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, -1,
        FX_43, FX_6, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_43, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, -1,
        1002, 1002,
    },
    // Tommy regular
    {
        DAMAGE_TYPE_2,
        12,
        65536,
        0,
        16384,
        0,
        1,
        12288,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_43, FX_5, FX_7, 510,
        FX_NONE, FX_5, FX_7, 511,
        FX_43, FX_6, FX_NONE, 512,
        FX_43, FX_0, FX_NONE, 513,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_7, 512,
        FX_43, FX_6, FX_7, 512,
        FX_NONE, FX_NONE, FX_NONE, 513,
        FX_43, FX_NONE, FX_NONE, 513,
        FX_NONE, FX_6, FX_NONE, 513,
        FX_NONE, FX_6, FX_NONE, 513,
        FX_NONE, FX_6, FX_NONE, 513,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, -1,
        359, 359,
    },
    // Bat bite
    {
        DAMAGE_TYPE_2,
        4,
        0,
        921,
        0,
        0,
        1,
        4096,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_0, FX_NONE, 503,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        521, 521,
    },
    // Eel bite
    {
        DAMAGE_TYPE_2,
        12,
        0,
        1177,
        0,
        0,
        0,
        0,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, 500,
        FX_NONE, FX_5, FX_NONE, 501,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_0, FX_NONE, 503,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        513, 513
    },
    // Gill bite
    {
        DAMAGE_TYPE_2,
        9,
        0,
        1177,
        0,
        0,
        0,
        0,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, 500,
        FX_NONE, FX_5, FX_NONE, 501,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_0, FX_NONE, 503,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        499, 499
    },
            // Beast slash
    {
        DAMAGE_TYPE_3,
        50,
        43690,
        1024,
        8192,
        0,
        4,
        32768,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, 500,
        FX_NONE, FX_5, FX_NONE, 501,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_0, FX_NONE, 503,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        9012, 9014
    },
            // Axe
    {
        DAMAGE_TYPE_2,
        18,
        436906,
        1024,
        16384,
        0,
        2,
        20480,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, 500,
        FX_NONE, FX_5, FX_NONE, 501,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_0, FX_NONE, 503,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, -1,
        1101, 1101
    },
            // Cleaver
    {
        DAMAGE_TYPE_2,
        9,
        218453,
        1024,
        0,
        0,
        1,
        24576,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, 500,
        FX_NONE, FX_5, FX_NONE, 501,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_0, FX_NONE, 503,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, -1,
        1207, 1207
    },
            // Phantasm slash
    {
        DAMAGE_TYPE_2,
        20,
        436906,
        1024,
        16384,
        0,
        3,
        24576,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, 500,
        FX_NONE, FX_5, FX_NONE, 501,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_0, FX_NONE, 503,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, -1,
        499, 495
    },
            // Gargoyle Slash
    {
        DAMAGE_TYPE_2,
        16,
        218453,
        1024,
        8192,
        0,
        4,
        20480,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, 500,
        FX_NONE, FX_5, FX_NONE, 501,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_0, FX_NONE, 503,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, -1,
        495, 496
    },
            // Cerberus bite
    {
        DAMAGE_TYPE_2,
        19,
        218453,
        614,
        8192,
        0,
        2,
        24576,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, 500,
        FX_NONE, FX_5, FX_NONE, 501,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_0, FX_NONE, 503,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        9013, 499
    },
            // Hound bite
    {
        DAMAGE_TYPE_2,
        10,
        218453,
        614,
        8192,
        0,
        2,
        24576,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, 500,
        FX_NONE, FX_5, FX_NONE, 501,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_0, FX_NONE, 503,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        1307, 1308
    },
            // Rat bite
    {
        DAMAGE_TYPE_2,
        4,
        0,
        921,
        0,
        0,
        1,
        24576,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_0, FX_NONE, 503,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        499, 499
    },
            // Spider bite
    {
        DAMAGE_TYPE_2,
        8,
        0,
        614,
        0,
        0,
        1,
        24576,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, 500,
        FX_NONE, FX_5, FX_NONE, 501,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_0, FX_NONE, 503,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        499, 499
    },
            // Unk
    {
        DAMAGE_TYPE_2,
        9,
        0,
        512,
        0,
        0,
        0,
        0,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_5, FX_NONE, 500,
        FX_NONE, FX_5, FX_NONE, 501,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_0, FX_NONE, 503,
        FX_NONE, FX_4, FX_NONE, -1,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_6, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, 502,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        499, 499
    },
    
    {
        (DAMAGE_TYPE)-1,
        0,
        0,
        2560,
        0,
        0,
        0,
        0,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_34, FX_35, -1,
        FX_NONE, FX_34, FX_35, -1,
        FX_NONE, FX_34, FX_35, -1,
        FX_NONE, FX_34, FX_35, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_34, FX_35, -1,
        FX_NONE, FX_34, FX_35, -1,
        FX_NONE, FX_34, FX_35, -1,
        FX_NONE, FX_34, FX_35, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        499, 499
    },
    // Tchernobog burn vector
    {
        DAMAGE_TYPE_1,
        2,
        0,
        0,
        0,
        15,
        0,
        0,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        351, 351
    },
    // Vodoo 1.0 vector
    {
        DAMAGE_TYPE_5,
        25,
        0,
        0,
        0,
        0,
        0,
        0,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, -1,
        0,0
    },
    // 22 kVectorGenDudePunch
    {
    DAMAGE_TYPE_0, 
        37, 
        874762, 
        620, 
        0, 
        0, 
        0, 
        0,
        FX_NONE, FX_NONE, FX_NONE, -1,
        FX_NONE, FX_NONE, FX_NONE, 357,
        FX_NONE, FX_NONE, FX_NONE, 357,
        FX_NONE, FX_NONE, FX_NONE, 357,
        FX_NONE, FX_NONE, FX_NONE, 357,
        FX_NONE, FX_NONE, FX_NONE, 357,
        FX_NONE, FX_NONE, FX_NONE, 357,
        FX_NONE, FX_NONE, FX_NONE, 357,
        FX_NONE, FX_NONE, FX_NONE, 357,
        FX_NONE, FX_NONE, FX_NONE, 357,
        FX_NONE, FX_NONE, FX_NONE, 357,
        FX_NONE, FX_NONE, FX_NONE, 357,
        FX_NONE, FX_NONE, FX_NONE, 357,
        FX_NONE, FX_NONE, FX_NONE, 357,
        FX_NONE, FX_NONE, FX_NONE, 357,
        357, 499
    },
};

ITEMDATA gItemData[] = {
    {
        0,
        2552,
        (char)-8,
        0,
        32,
        32,
        -1,
    },
    {
        0,
        2553,
        (char)-8,
        0,
        32,
        32,
        -1,
    },
    {
        0,
        2554,
        (char)-8,
        0,
        32,
        32,
        -1,
    },
    {
        0,
        2555,
        (char)-8,
        0,
        32,
        32,
        -1,
    },
    {
        0,
        2556,
        (char)-8,
        0,
        32,
        32,
        -1,
    },
    {
        0,
        2557,
        (char)-8,
        0,
        32,
        32,
        -1,
    },
    {
        0,
        -1,
        (char)-8,
        0,
        255,
        255,
        -1,
    },
    {
        0,
        519,
        (char)-8,
        0,
        48,
        48,
        0,
    },
    {
        0,
        822,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        2169,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        2433,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        517,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        783,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        896,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        825,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        827,
        (char)-8,
        0,
        40,
        40,
        4,
    },
    {
        0,
        828,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        829,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        830,
        (char)-8,
        0,
        80,
        64,
        1,
    },
    {
        0,
        831,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        863,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        760,
        (char)-8,
        0,
        40,
        40,
        2,
    },
    {
        0,
        836,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        851,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        2428,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        839,
        (char)-8,
        0,
        40,
        40,
        3,
    },
    {
        0,
        768,
        (char)-8,
        0,
        64,
        64,
        -1,
    },
    {
        0,
        840,
        (char)-8,
        0,
        48,
        48,
        -1,
    },
    {
        0,
        841,
        (char)-8,
        0,
        48,
        48,
        -1,
    },
    {
        0,
        842,
        (char)-8,
        0,
        48,
        48,
        -1,
    },
    {
        0,
        843,
        (char)-8,
        0,
        48,
        48,
        -1,
    },
    {
        0,
        683,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        521,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        604,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        520,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        803,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        518,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        522,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        523,
        (char)-8,
        0,
        40,
        40,
        -1,
    },
    {
        0,
        837,
        (char)-8,
        0,
        80,
        64,
        -1,
    },
    {
        0,
        2628,
        (char)-8,
        0,
        64,
        64,
        -1,
    },
    {
        0,
        2586,
        (char)-8,
        0,
        64,
        64,
        -1,
    },
    {
        0,
        2578,
        (char)-8,
        0,
        64,
        64,
        -1,
    },
    {
        0,
        2602,
        (char)-8,
        0,
        64,
        64,
        -1,
    },
    {
        0,
        2594,
        (char)-8,
        0,
        64,
        64,
        -1,
    },
    {
        0,
        753,
        (char)-8,
        0,
        64,
        64,
        -1,
    },
    {
        0,
        753,
        (char)-8,
        7,
        64,
        64,
        -1,
    },
    {
        0,
        3558,
        (char)-128,
        0,
        64,
        64,
        -1,
    },
    {
        0,
        3558,
        (char)-128,
        7,
        64,
        64,
        -1,
    }
};

AMMOITEMDATA gAmmoItemData[] = {
    {
        0,
        618,
        (char)-8,
        0,
        40,
        40,
        480,
        6,
        7
    },
    {
        0,
        589,
        (char)-8,
        0,
        48,
        48,
        1,
        5,
        6
    },
    {
        0,
        589,
        (char)-8,
        0,
        48,
        48,
        1,
        5,
        6
    },
    {
        0,
        809,
        (char)-8,
        0,
        48,
        48,
        5,
        5,
        6
    },
    {
        0,
        811,
        (char)-8,
        0,
        48,
        48,
        1,
        10,
        11
    },
    {
        0,
        810,
        (char)-8,
        0,
        48,
        48,
        1,
        11,
        12
    },
    {
        0,
        820,
        (char)-8,
        0,
        24,
        24,
        10,
        8,
        0
    },
    {
        0,
        619,
        (char)-8,
        0,
        48,
        48,
        4,
        2,
        0
    },
    {
        0,
        812,
        (char)-8,
        0,
        48,
        48,
        15,
        2,
        0
    },
    {
        0,
        813,
        (char)-8,
        0,
        48,
        48,
        15,
        3,
        0
    },
    {
        0,
        525,
        (char)-8,
        0,
        48,
        48,
        100,
        9,
        10
    },
    {
        0,
        814,
        (char)-8,
        0,
        48,
        48,
        15,
        255,
        0
    },
    {
        0,
        817,
        (char)-8,
        0,
        48,
        48,
        100,
        3,
        0
    },
    {
        0,
        548,
        (char)-8,
        0,
        24,
        24,
        32,
        7,
        0
    },
    {
        0,
        0,
        (char)-8,
        0,
        48,
        48,
        6,
        255,
        0
    },
    {
        0,
        0,
        (char)-8,
        0,
        48,
        48,
        6,
        255,
        0
    },
    {
        0,
        816,
        (char)-8,
        0,
        48,
        48,
        8,
        1,
        0
    },
    {
        0,
        818,
        (char)-8,
        0,
        48,
        48,
        8,
        255,
        0
    },
    {
        0,
        819,
        (char)-8,
        0,
        48,
        48,
        8,
        255,
        0
    },
    {
        0,
        801,
        (char)-8,
        0,
        48,
        48,
        6,
        4,
        0
    },
    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    },
};

WEAPONITEMDATA gWeaponItemData[] = {
    {
        0,
        -1,
        (char)0,
        0,
        0,
        0,
        0,
        -1,
        0
    },
    {
        0,
        559,
        (char)-8,
        0,
        48,
        48,
        3,
        2,
        8
    },
    {
        0,
        558,
        (char)-8,
        0,
        48,
        48,
        4,
        3,
        50
    },
    {
        0,
        524,
        (char)-8,
        0,
        48,
        48,
        2,
        1,
        9
    },
    {
        0,
        525,
        (char)-8,
        0,
        48,
        48,
        10,
        9,
        100
    },
    {
        0,
        539,
        (char)-8,
        0,
        48,
        48,
        8,
        7,
        64
    },
    {
        0,
        526,
        (char)-8,
        0,
        48,
        48,
        5,
        4,
        6
    },
    {
        0,
        -1,
        (char)0,
        0,
        0,
        0,
        1,
        -1,
        0
    },
    {
        0,
        618,
        (char)-8,
        0,
        48,
        48,
        7,
        6,
        480
    },
    {
        0,
        589,
        (char)-8,
        0,
        48,
        48,
        6,
        5,
        1
    },
    {
        0,
        800,
        (char)-8,
        0,
        48,
        48,
        9,
        8,
        35
    }
};

MissileType missileInfo[] = {
    // Cleaver
    {
        2138,
        978670,
        512,
        40,
        40,
        (char)-16,
        16,
        1207, 1207,
        false, false, false, false, false, true, false

    },
    // Regular flare
    {
        2424,
        3145728,
        0,
        32,
        32,
        (char)-128,
        32,
        420, 420,
        false, true, true, false, false, false, false
    },
    // Tesla alt
    {
        3056,
        2796202,
        0,
        32,
        32,
        (char)-128,
        32,
        471, 471,
        false, false, false, false, false, false, true
    },
    // Flare alt
    {
        2424,
        2446677,
        0,
        32,
        32,
        (char)-128,
        4,
        421, 421,
        false, true, false, true, false, false, false
    },
    // Spray flame
    {
        0,
        1118481,
        0,
        24,
        24,
        (char)-128,
        16,
        1309, 351,
        false, true, false, false, false, false, false
    },
    // Fireball
    {
        0,
        1118481,
        0,
        32,
        32,
        (char)-128,
        32,
        480, 480,
        false, true, false, true, false, false, false
    },
    // Tesla regular
    {
        2130,
        2796202,
        0,
        32,
        32,
        (char)-128,
        16,
        470, 470,
        false, false, false, false, false, false, true
    },
    // EctoSkull
    {
        870,
        699050,
        0,
        32,
        32,
        (char)-24,
        32,
        489, 490,
        false, false, false, false, false, true, false
    },
    // Hellhound flame
    {
        0,
        1118481,
        0,
        24,
        24,
        (char)-128,
        16,
        462, 351,
        false, true, false, false, false, false, false
    },
    // Puke
    {
        0,
        838860,
        0,
        16,
        16,
        (char)-16,
        16,
        1203, 172,
        false, false, true, false, false, false, false
    },
    // Reserved
    {
        0,
        838860,
        0,
        8,
        8,
        (char)0,
        16,
        0,0,
        false, false, true, false, false, false, false
    },
    // Stone gargoyle projectile
    {
        3056,
        2097152,
        0,
        32,
        32,
        (char)-128,
        16,
        1457, 249,
        false, false, false, false, false, true, false
    },
    // Napalm launcher
    {
        0,
        2446677,
        0,
        30,
        30,
        (char)-128,
        24,
        480, 489,
        false, true, false, true, false, false, false
    },
    // Cerberus fireball
    {
        0,
        2446677,
        0,
        30,
        30,
        (char)-128,
        24,
        480, 489,
        false, false, false, true, false, false, false
    },
    // Tchernobog fireball
    {
        0,
        1398101,
        0,
        24,
        24,
        (char)-128,
        16,
        480, 489,
        false, false, false, true, false, false, false
    },
    // Regular life leech
    {
        2446,
        2796202,
        0,
        32,
        32,
        (char)-128,
        16,
        491, 491,
        true, true, true, true, true, true, true
    },
    // Dropped life leech (enough ammo)
    {
        3056,
        2446677,
        0,
        16,
        16,
        (char)-128,
        16,
        520, 520,
        false, false, false, false, false, true, false
    },
    // Dropped life leech (no ammo)
    {
        3056,
        1747626,
        0,
        32,
        32,
        (char)-128,
        16,
        520, 520,
        false, false, false, false, false, true, false
    }
};

const THINGINFO thingInfo[] = {
    //TNT Barrel
    {
        25,
        250,
        32,
        11,
        4096,
        80,
        384,
        907,
        (char)0,
        0,
        0,
        0,
        256, 256, 128, 64, 0, 0, 128,
        1
    },
    // Armed Proxy Dynamite
    {
        5,
        5,
        16,
        3,
        24576,
        1600,
        256,
        3444,
        (char)-16,
        0,
        32,
        32,
        256, 256, 256, 64, 0, 0, 512,
        1
    },
    // Armed Remote Dynamite
    {
        5,
        5,
        16,
        3,
        24576,
        1600,
        256,
        3457,
        (char)-16,
        0,
        32,
        32,
        256, 256, 256, 64, 0, 0, 512,
        1
    },
    // Vase1
    {
        1,
        20,
        32,
        3,
        32768,
        80,
        0,
        739,
        (char)0,
        0,
        0,
        0,
        256, 0, 256, 128, 0, 0, 0,
        0
    },
    // Vase2
    {
        1,
        150,
        32,
        3,
        32768,
        80,
        0,
        642,
        (char)0,
        0,
        0,
        0,
        256, 256, 256, 128, 0, 0, 0,
        0
    },
    // Crate face
    {
        10,
        0,
        0,
        0,
        0,
        0,
        0,
        462,
        (char)0,
        0,
        0,
        0,
        0, 0, 0, 256, 0, 0, 0,
        0
    },
    // Glass window
    {
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        266,
        (char)0,
        0,
        0,
        0,
        256, 0, 256, 256, 0, 0, 0,
        0,
    },
    // Flourescent Light
    {
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        796,
        (char)0,
        0,
        0,
        0,
        256, 0, 256, 256, 0, 0, 512,
        0,
    },
    // Wall Crack
    {
        50,
        0,
        0,
        0,
        0,
        0,
        0,
        1127,
        (char)0,
        0,
        0,
        0,
        0, 0, 0, 256, 0, 0, 0,
        0,
    },
    // Wood Beam
    {
        8,
        0,
        0,
        0,
        0,
        0,
        0,
        1142,
        (char)0,
        0,
        0,
        0,
        256, 0, 256, 128, 0, 0, 0,
        0,
    },
    // Spider's Web
    {
        4,
        0,
        0,
        0,
        0,
        0,
        0,
        1069,
        (char)0,
        0,
        0,
        0,
        256, 256, 64, 256, 0, 0, 128,
        0,
    },
    // Metal Grate
    {
        40,
        0,
        0,
        0,
        0,
        0,
        0,
        483,
        (char)0,
        0,
        0,
        0,
        64, 0, 128, 256, 0, 0, 0,
        0,
    },
    // Flammable Tree
    {
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        -1,
        (char)0,
        0,
        0,
        0,
        0, 256, 0, 256, 0, 0, 128,
        0,
    },
    // MachineGun Trap
    {
        1000,
        0,
        0,
        8,
        0,
        0,
        0,
        -1,
        (char)0,
        0,
        0,
        0,
        0, 0, 128, 256, 0, 0, 512,
        0,
    },
    // Falling Rock
    {
        0,
        15,
        8,
        3,
        32768,
        0,
        0,
        -1,
        (char)0,
        0,
        0,
        0,
        0, 0, 0, 0, 0, 0, 0,
        0,
    },
    // Kickable Pail
    {
        0,
        8,
        48,
        3,
        49152,
        0,
        0,
        -1,
        (char)0,
        0,
        0,
        0,
        0, 0, 0, 0, 0, 0, 0,
        1,
    },
    // Gib Object
    {
        10,
        2,
        0,
        0,
        32768,
        0,
        0,
        -1,
        (char)0,
        0,
        0,
        0,
        256, 0, 256, 256, 0, 0, 128,
        0,
    },
    // Explode Object
    {
        20,
        2,
        0,
        0,
        32768,
        0,
        0,
        -1,
        (char)0,
        0,
        0,
        0,
        0, 0, 0, 256, 0, 0, 128,
        0,
    },
    // Armed stick Of TNT
    {
        5,
        14,
        16,
        3,
        24576,
        1600,
        256,
        3422,
        (char)-32,
        0,
        32,
        32,
        64, 256, 128, 64, 0, 0, 256,
        1
    },
    // Armed bundle Of TNT
    {
        5,
        14,
        16,
        3,
        24576,
        1600,
        256,
        3433,
        (char)-32,
        0,
        32,
        32,
        64, 256, 128, 64, 0, 0, 256,
        1
    },
    // Armed aerosol
    {
        5,
        14,
        16,
        3,
        32768,
        1600,
        256,
        3467,
        (char)-128,
        0,
        32,
        32,
        64, 256, 128, 64, 0, 0, 256,
        1
    },
    // Bone (Flesh Garg.)
    {
        5,
        6,
        16,
        3,
        32768,
        1600,
        256,
        1462,
        (char)0,
        0,
        32,
        32,
        0, 0, 0, 0, 0, 0, 0,
        1
    },
    // Some alpha stuff
    {
        8,
        3,
        16,
        11,
        32768,
        1600,
        256,
        -1,
        (char)0,
        0,
        0,
        0,
        256, 0, 256, 256, 0, 0, 0,
        0,
    },
    // WaterDrip 
    {
        0,
        1,
        1,
        2,
        0,
        0,
        0,
        1147,
        (char)0,
        10,
        0,
        0,
        0, 0, 0, 0, 0, 0, 0,
        0
    },
    // BloodDrip 
    {
        0,
        1,
        1,
        2,
        0,
        0,
        0,
        1160,
        (char)0,
        2,
        0,
        0,
        0, 0, 0, 0, 0, 0, 0,
        0,
    },
    // Blood chucks1 
    {
        15,
        4,
        4,
        3,
        24576,
        0,
        257,
        -1,
        (char)0,
        0,
        0,
        0,
        128, 64, 256, 256, 0, 0, 256,
        0,
    },
    // Blood chucks2
    {
        30,
        30,
        8,
        3,
        8192,
        0,
        257,
        -1,
        (char)0,
        0,
        0,
        0,
        128, 64, 256, 256, 0, 0, 64,
        0,
    },
    // Axe Zombie Head 
    {
        60,
        5,
        32,
        3,
        40960,
        1280,
        257,
        3405,
        (char)0,
        0,
        40,
        40,
        128, 64, 256, 256, 0, 0, 64,
        1,
    },
    // Napalm's Alt Fire explosion
    {
        80,
        30,
        32,
        3,
        57344,
        1600,
        256,
        3281,
        (char)-128,
        0,
        32,
        32,
        0, 0, 0, 0, 0, 0, 0,
        1,
    },
    // Fire Pod Explosion
    {
        80,
        30,
        32,
        3,
        57344,
        1600,
        256,
        2020,
        (char)-128,
        0,
        32,
        32,
        256, 0, 256, 256, 0, 0, 0,
        1,
    },
    // Green Pod Explosion
    {
        80,
        30,
        32,
        3,
        57344,
        1600,
        256,
        1860,
        (char)-128,
        0,
        32,
        32,
        256, 0, 256, 256, 0, 0, 0,
        1,
    },
    // Life Leech
    {
        150,
        30,
        48,
        3,
        32768,
        1600,
        257,
        800,
        (char)-128,
        0,
        48,
        48,
        64, 64, 112, 64, 0, 96, 96,
        1,
    },
    // Voodoo Head
    {
        1,
        30,
        48,
        3,
        32768,
        1600,
        0,
        2443,
        (char)-128,
        0,
        16,
        16,
        0, 0, 0, 0, 0, 0, 0,
        1,
    },
    // 433 - kModernThingTNTProx
    {
        5,
        5,
        16,
        3,
        24576,
        1600,
        256,
        3444,
        (char)-16,
        7,
        32,
        32,
        256, 256, 256, 64, 0, 0, 512,
        1
    },
    // 434 - kModernThingThrowableRock
    {
        5,
        6,
        16,
        3,
        32768,
        1600,
        256,
        1462,
        (char)0,
        0,
        32,
        32,
        0, 0, 0, 0, 0, 0, 0,
        1
    },
    // 435 - kModernThingEnemyLifeLeech
    {
        150,
        30,
        48,
        3,
        32768,
        1600,
        257,
        800,
        (char)-128,
        0,
        48,
        48,
        64, 64, 112, 64, 0, 96, 96,
        1,
    },
};

EXPLOSION explodeInfo[] = {
    {
        40,
        10,
        10,
        75,
        450,
        0,
        60,
        80,
        40
    },
    {
        80,
        20,
        10,
        150,
        900,
        0,
        60,
        160,
        60
    },
    {
        120,
        40,
        15,
        225,
        1350,
        0,
        60,
        240,
        80
    },
    {
        80,
        5,
        10,
        120,
        20,
        10,
        60,
        0,
        40
    },
    {
        120,
        10,
        10,
        180,
        40,
        10,
        60,
        0,
        80
    },
    {
        160,
        15,
        10,
        240,
        60,
        10,
        60,
        0,
        120
    },
    {
        40,
        20,
        10,
        120,
        0,
        10,
        30,
        60,
        40
    },
    {
        80,
        20,
        10,
        150,
        800,
        5,
        60,
        160,
        60
    },
};

int gDudeDrag = 0x2a00;

short gAffectedSectors[kMaxSectors];
short gAffectedXWalls[kMaxXWalls];
short gPlayerGibThingComments[] = {
    734, 735, 736, 737, 738, 739, 740, 741, 3038, 3049
};

void FireballSeqCallback(int, int);
void sub_38938(int, int);
void NapalmSeqCallback(int, int);
void sub_3888C(int, int);
void TreeToGibCallback(int, int);
void DudeToGibCallback1(int, int);
void DudeToGibCallback2(int, int);

int nFireballClient = seqRegisterClient(FireballSeqCallback);
int dword_2192D8 = seqRegisterClient(sub_38938); // fireball smoke
int nNapalmClient = seqRegisterClient(NapalmSeqCallback);
int dword_2192E0 = seqRegisterClient(sub_3888C); // flame lick
int nTreeToGibClient = seqRegisterClient(TreeToGibCallback);
int nDudeToGibClient1 = seqRegisterClient(DudeToGibCallback1);
int nDudeToGibClient2 = seqRegisterClient(DudeToGibCallback2);

int gPostCount = 0;

struct POSTPONE {
    short at0;
    short at2;
};

POSTPONE gPost[kMaxSprites];

//static char buffer[120];

bool IsItemSprite(spritetype *pSprite)
{
    return pSprite->type >= kItemBase && pSprite->type < kItemMax;
}

bool IsWeaponSprite(spritetype *pSprite)
{
    return pSprite->type >= kItemWeaponBase && pSprite->type < kItemWeaponMax;
}

bool IsAmmoSprite(spritetype *pSprite)
{
    return pSprite->type >= kItemAmmoBase && pSprite->type < kItemAmmoMax;
}

bool IsUnderwaterSector(int nSector)
{
    int nXSector = sector[nSector].extra;
    if (nXSector > 0 && xsector[nXSector].Underwater)
        return 1;
    return 0;
}

int actSpriteOwnerToSpriteId(spritetype *pSprite)
{
    dassert(pSprite != NULL);
    if (pSprite->owner == -1)
        return -1;
    int nSprite = pSprite->owner & (kMaxSprites-1);
    if (pSprite->owner & kMaxSprites)
        nSprite = gPlayer[nSprite].pSprite->index;
    return nSprite;
}

void actPropagateSpriteOwner(spritetype *pTarget, spritetype *pSource)
{
    dassert(pTarget != NULL && pSource != NULL);
    if (IsPlayerSprite(pSource))
        pTarget->owner = (pSource->type - kDudePlayer1) | kMaxSprites;
    else
        pTarget->owner = pSource->index;
}

int actSpriteIdToOwnerId(int nSprite)
{
    if (nSprite == -1)
        return -1;
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    spritetype *pSprite = &sprite[nSprite];
    if (IsPlayerSprite(pSprite))
        nSprite = (pSprite->type - kDudePlayer1) | kMaxSprites;
    return nSprite;
}

int actOwnerIdToSpriteId(int nSprite)
{
    if (nSprite == -1)
        return -1;
    if (nSprite & kMaxSprites)
        nSprite = gPlayer[nSprite&(kMaxSprites-1)].pSprite->index;
    return nSprite;
}

bool actTypeInSector(int nSector, int nType)
{
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        if (sprite[nSprite].index == nType)
            return 1;
    }
    return 0;
}

void actAllocateSpares(void)
{
}

int DudeDifficulty[5] = {
    512, 384, 256, 208, 160
};

SPRITEMASS gSpriteMass[]; // by NoOne: cache for getSpriteMassBySize();

short gProxySpritesList[]; // by NoOne: list of additional sprites which can be triggered by Proximity
short gProxySpritesCount; // current count

short gSightSpritesList[]; // by NoOne: list of additional sprites which can be triggered by Sight
short gSightSpritesCount; // current count

short gPhysSpritesList[];  // by NoOne: list of additional sprites which can be affected by physics
short gPhysSpritesCount; // current count

void actInit(bool bSaveLoad) {
    
    // by NoOne: init code for all my stuff
    if (gModernMap) {

        // reset counters
        gProxySpritesCount = gSightSpritesCount = gPhysSpritesCount = 0;

        // fill arrays with negative values to avoid xvel 0 situation
        memset(gSightSpritesList, -1, sizeof(gSightSpritesList));
        memset(gProxySpritesList, -1, sizeof(gProxySpritesList));
        memset(gPhysSpritesList, -1, sizeof(gPhysSpritesList));

        for (int i = 0; i < kMaxXSprites; i++) {

            if (xsprite[i].reference < 0) continue;
            XSPRITE* pXSprite = &xsprite[i];  spritetype* pSprite = &sprite[pXSprite->reference];

            switch (pSprite->type) {
                case kDudeModernCustom:
                case kDudeModernCustomBurning:
                    getSpriteMassBySize(pSprite); // create mass cache
                    break;
            }

            // init after loading save file
            if (bSaveLoad) {
                
                // add in list of physics affected sprites
                if (pXSprite->physAttr != 0) {
                    //xvel[pSprite->index] = yvel[pSprite->index] = zvel[pSprite->index] = 0;
                    
                    gPhysSpritesList[gPhysSpritesCount++] = pSprite->index; // add sprite index
                    getSpriteMassBySize(pSprite); // create mass cache
                }
                
                if (pXSprite->data3 != pXSprite->sysData1) {
                    switch (pSprite->statnum) {
                        case kStatDude:
                            switch (pSprite->type) {
                                case kDudeModernCustom:
                                case kDudeModernCustomBurning:
                                    pXSprite->data3 = pXSprite->sysData1; // move sndStartId back from sysData1 to data3 
                                    break;
                            }
                            break;
                    }
                }
            }

            // make Proximity flag work not just for dudes and things...
            if (pXSprite->Proximity && gProxySpritesCount < kMaxSuperXSprites) {
                switch (pSprite->statnum) {
                    // exceptions
                    case kStatThing: // things already treated in their functions
                    case kStatDude: // enemies already treated in their functions
                        // senseless to have sight and proximity together
                        if (pXSprite->Sight && pXSprite->DudeLockout) pXSprite->Proximity = false;
                        break;
                    case kStatFX: // effects
                    case kStatExplosion: // explosions
                    case kStatItem: // items
                    case kStatPurge: // purgeable sprites
                    case kStatSpares: // ???
                    case kStatFlare: // burning flares stuck
                    case kStatInactive: // inactive enemies
                    case kStatFree: // removed sprites
                    case kStatMarker: // markers
                    case kStatPathMarker: // path markers
                        break;
                    default:
                        // senseless to have sight and proximity together
                        if (pXSprite->Sight && pXSprite->DudeLockout) pXSprite->Proximity = false;
                        else {
                            gProxySpritesList[gProxySpritesCount++] = pSprite->xvel;
                            if (gProxySpritesCount == kMaxSuperXSprites)
                                ThrowError("Max (%d) *additional* Proximity sprites reached!", kMaxSuperXSprites);
                        }
                        break;
                }
            }

            // make Sight flag work not just for dudes and things...
            if (pXSprite->Sight && gSightSpritesCount < kMaxSuperXSprites) {
                switch (pSprite->statnum) {
                    // exceptions
                    case kStatFX: // effects
                    case kStatExplosion: // explosions
                    case kStatItem: // items
                    case kStatPurge: // purgeable sprites
                    case kStatSpares: // ???
                    case kStatFlare: // burning flares stuck
                    case kStatInactive: // inactive enemies
                    case kStatFree: // removed sprites
                    case kStatMarker: // markers
                    case kStatPathMarker: // path markers
                        break;
                    default:
                        gSightSpritesList[gSightSpritesCount++] = pSprite->xvel;
                        if (gSightSpritesCount == kMaxSuperXSprites)
                            ThrowError("Max (%d) Sight sprites reached!", kMaxSuperXSprites);
                        break;
                }
            }
        }
    }
    
    for (int nSprite = headspritestat[kStatItem]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        switch (sprite[nSprite].type) {
            case kItemWeaponVoodooDoll:
                sprite[nSprite].type = kAmmoItemVoodooDoll;
                break;
        }
    }

    for (int nSprite = headspritestat[kStatTraps]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        spritetype *pSprite = &sprite[nSprite];
        switch (pSprite->type) {
            case kTrapExploder:
                pSprite->cstat &= ~1; pSprite->cstat |= CSTAT_SPRITE_INVISIBLE;
                if (pSprite->extra <= 0 || pSprite->extra >= kMaxXSprites) continue;
                xsprite[pSprite->extra].waitTime = ClipLow(xsprite[pSprite->extra].waitTime, 1);
                xsprite[pSprite->extra].state = 0;
                break;
        }
    }

    for (int nSprite = headspritestat[kStatThing]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        if (sprite[nSprite].extra <= 0 || sprite[nSprite].extra >= kMaxXSprites) continue;
        spritetype* pSprite = &sprite[nSprite]; XSPRITE *pXSprite = &xsprite[pSprite->extra];
        
        int nType = pSprite->type - kThingBase;
        pXSprite->health = thingInfo[nType].startHealth << 4;
        // by NoOne: allow level designer to set custom clipdist.
        // this is especially useful for various Gib and Explode objects which have clipdist 1 for some reason predefined,
        // but what if it have voxel model...?
        if (!gModernMap) pSprite->clipdist = thingInfo[nType].clipdist;
        
        pSprite->flags = thingInfo[nType].flags;
        if (pSprite->flags & kPhysGravity) pSprite->flags |= kPhysFalling;
        xvel[nSprite] = yvel[nSprite] = zvel[nSprite] = 0;
        
        switch (pSprite->type) {
            case kThingArmedProxBomb:
            case kTrapMachinegun:
            case kModernThingTNTProx:
                pXSprite->state = 0;
                break;
            case kThingBloodChunks: {
                SEQINST *pInst = GetInstance(3, pSprite->extra);
                if (pInst && pInst->at13) {
                    DICTNODE *hSeq = gSysRes.Lookup(pInst->at8, "SEQ");
                    if (!hSeq) break;
                    seqSpawn(pInst->at8, 3, pSprite->extra);
                }
                break;
            }
            default:
                pXSprite->state = 1;
                break;
        }
    }
    
    if (gGameOptions.nMonsterSettings == 0) {
        gKillMgr.SetCount(0);
        while (headspritestat[kStatDude] >= 0) {
            spritetype *pSprite = &sprite[headspritestat[kStatDude]];
            if (pSprite->extra > 0 && pSprite->extra < kMaxXSprites && xsprite[pSprite->extra].key > 0) // Drop Key
                actDropObject(pSprite, kItemKeyBase + (xsprite[pSprite->extra].key - 1));
            DeleteSprite(headspritestat[kStatDude]);
        }
    } else {
        // by NoOne: WTF is this?
        ///////////////
        char unk[kDudeMax-kDudeBase];
        memset(unk, 0, sizeof(unk));
        for (int nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
            spritetype *pSprite = &sprite[nSprite];
            if (pSprite->type < kDudeBase || pSprite->type >= kDudeMax)
                ThrowError("Non-enemy sprite (%d) in the enemy sprite list.\n", nSprite);
            unk[pSprite->type - kDudeBase] = 1;
        }
        
        gKillMgr.sub_2641C();
        ///////////////

        for (int i = 0; i < kDudeMax - kDudeBase; i++)
            for (int j = 0; j < 7; j++)
                dudeInfo[i].at70[j] = mulscale8(DudeDifficulty[gGameOptions.nDifficulty], dudeInfo[i].startDamage[j]);

        for (int nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
            if (sprite[nSprite].extra <= 0 || sprite[nSprite].extra >= kMaxXSprites) continue;
            spritetype *pSprite = &sprite[nSprite]; XSPRITE *pXSprite = &xsprite[pSprite->extra];
            
            int nType = pSprite->type - kDudeBase; int seqStartId = dudeInfo[nType].seqStartID;
            if (!IsPlayerSprite(pSprite)) {
                switch (pSprite->type) {
                    case kDudeModernCustom:
                    case kDudeModernCustomBurning:
                        pSprite->cstat |= 4096 + CSTAT_SPRITE_BLOCK_HITSCAN + CSTAT_SPRITE_BLOCK;
                        seqStartId = genDudeSeqStartId(pXSprite); // by NoOne: Custom Dude stores it's SEQ in data2
                        pXSprite->sysData1 = pXSprite->data3; // by NoOne move sndStartId to sysData1, because data3 used by the game;
                        pXSprite->data3 = 0;
                        break;
                    case kDudePodMother:  // by NoOne: FakeDude type (no seq, custom flags, clipdist and cstat)
                        if (gModernMap) break;
                        fallthrough__;
                    default:
                        pSprite->clipdist = dudeInfo[nType].clipdist;
                        pSprite->cstat |= 4096 + CSTAT_SPRITE_BLOCK_HITSCAN + CSTAT_SPRITE_BLOCK;
                        break;
                }

                xvel[nSprite] = yvel[nSprite] = zvel[nSprite] = 0;
                
                // By NoOne: add a way to set custom hp for every enemy - should work only if map just started and not loaded.
                if (!gModernMap || pXSprite->data4 <= 0) pXSprite->health = dudeInfo[nType].startHealth << 4;
                else pXSprite->health = ClipRange(pXSprite->data4 << 4, 1, 65535);
            }

            if (gSysRes.Lookup(seqStartId, "SEQ")) seqSpawn(seqStartId, 3, pSprite->extra);
        }
        aiInit();
    }
}

void ConcussSprite(int a1, spritetype *pSprite, int x, int y, int z, int a6)
{
    dassert(pSprite != NULL);
    int dx = pSprite->x-x;
    int dy = pSprite->y-y;
    int dz = (pSprite->z-z)>>4;
    int dist2 = 0x40000+dx*dx+dy*dy+dz*dz;
    dassert(dist2 > 0);
    a6 = scale(0x40000, a6, dist2);

    if (pSprite->flags & kPhysMove) {
        int mass = 0;
        if (IsDudeSprite(pSprite)) {

            mass = dudeInfo[pSprite->type - kDudeBase].mass;
            switch (pSprite->type) {
            case kDudeModernCustom:
            case kDudeModernCustomBurning:
                mass = getSpriteMassBySize(pSprite);
                break;
            }

        } else if (pSprite->type >= kThingBase && pSprite->type < kThingMax) {
            mass = thingInfo[pSprite->type - kThingBase].mass;
        } else {
            consoleSysMsg("Unexpected type in ConcussSprite(): Sprite: %d  Type: %d  Stat: %d", (int)pSprite->index, (int)pSprite->type, (int)pSprite->statnum);
            return;
        }

        int size = (tilesiz[pSprite->picnum].x*pSprite->xrepeat*tilesiz[pSprite->picnum].y*pSprite->yrepeat)>>1;
        dassert(mass > 0);

        int t = scale(a6, size, mass);
        dx = mulscale16(t, dx);
        dy = mulscale16(t, dy);
        dz = mulscale16(t, dz);
        int nSprite = pSprite->index;
        dassert(nSprite >= 0 && nSprite < kMaxSprites);
        xvel[nSprite] += dx;
        yvel[nSprite] += dy;
        zvel[nSprite] += dz;
    }

    actDamageSprite(a1, pSprite, DAMAGE_TYPE_3, a6);
}

int actWallBounceVector(int *x, int *y, int nWall, int a4)
{
    int wx, wy;
    GetWallNormal(nWall, &wx, &wy);
    int t = dmulscale16(*x, wx, *y, wy);
    int t2 = mulscale16r(t, a4+0x10000);
    *x -= mulscale16(wx, t2);
    *y -= mulscale16(wy, t2);
    return mulscale16r(t, 0x10000-a4);
}

int actFloorBounceVector(int *x, int *y, int *z, int nSector, int a5)
{
    int t = 0x10000-a5;
    if (sector[nSector].floorheinum == 0)
    {
        int t2 = mulscale16(*z, t);
        *z = -(*z-t2);
        return t2;
    }
    walltype *pWall = &wall[sector[nSector].wallptr];
    walltype *pWall2 = &wall[pWall->point2];
    int angle = getangle(pWall2->x-pWall->x, pWall2->y-pWall->y)+512;
    int t2 = sector[nSector].floorheinum<<4;
    int t3 = approxDist(-0x10000, t2);
    int t4 = divscale16(-0x10000, t3);
    int t5 = divscale16(t2, t3);
    int t6 = mulscale30(t5, Cos(angle));
    int t7 = mulscale30(t5, Sin(angle));
    int t8 = tmulscale16(*x, t6, *y, t7, *z, t4);
    int t9 = mulscale16(t8, 0x10000+a5);
    *x -= mulscale16(t6, t9);
    *y -= mulscale16(t7, t9);
    *z -= mulscale16(t4, t9);
    return mulscale16r(t8, t);
}

void sub_2A620(int nSprite, int x, int y, int z, int nSector, int nDist, int a7, int a8, DAMAGE_TYPE a9, int a10, int a11, int a12, int a13)
{
    UNREFERENCED_PARAMETER(a12);
    UNREFERENCED_PARAMETER(a13);
    char va0[(kMaxSectors+7)>>3];
    int nOwner = actSpriteIdToOwnerId(nSprite);
    gAffectedSectors[0] = 0;
    gAffectedXWalls[0] = 0;
    GetClosestSpriteSectors(nSector, x, y, nDist, gAffectedSectors, va0, gAffectedXWalls);
    nDist <<= 4;
    if (a10 & 2)
    {
        for (int i = headspritestat[kStatDude]; i >= 0; i = nextspritestat[i])
        {
            if (i != nSprite || (a10 & 1))
            {
                spritetype *pSprite2 = &sprite[i];
                if (pSprite2->extra > 0 && pSprite2->extra < kMaxXSprites)
                {

                    if (pSprite2->flags & 0x20)
                        continue;
                    if (!TestBitString(va0, pSprite2->sectnum))
                        continue;
                    if (!CheckProximity(pSprite2, x, y, z, nSector, nDist))
                        continue;
                    int dx = klabs(x-pSprite2->x);
                    int dy = klabs(y-pSprite2->y);
                    int dz = klabs(z-pSprite2->z)>>4;
                    int dist = ksqrt(dx*dx+dy*dy+dz*dz);
                    if (dist > nDist)
                        continue;
                    int vcx;
                    if (dist != 0)
                        vcx = a7+((nDist-dist)*a8)/nDist;
                    else
                        vcx = a7+a8;
                    actDamageSprite(nSprite, pSprite2, a9, vcx<<4);
                    if (a11)
                        actBurnSprite(nOwner, &xsprite[pSprite2->extra], a11);
                }
            }
        }
    }
    if (a10 & 4)
    {
        for (int i = headspritestat[kStatThing]; i >= 0; i = nextspritestat[i])
        {
            spritetype *pSprite2 = &sprite[i];

            if (pSprite2->flags&0x20)
                continue;
            if (!TestBitString(va0, pSprite2->sectnum))
                continue;
            if (!CheckProximity(pSprite2, x, y, z, nSector, nDist))
                continue;
            XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
            if (pXSprite2->locked)
                continue;
            int dx = klabs(x-pSprite2->x);
            int dy = klabs(y-pSprite2->y);
            int dist = ksqrt(dx*dx+dy*dy);
            if (dist > nDist)
                continue;
            int vcx;
            if (dist != 0)
                vcx = a7+((nDist-dist)*a8)/nDist;
            else
                vcx = a7+a8;
            actDamageSprite(nSprite, pSprite2, a9, vcx<<4);
            if (a11)
                actBurnSprite(nOwner, pXSprite2, a11);
        }
    }
}

void sub_2AA94(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = actOwnerIdToSpriteId(pSprite->owner);
    actPostSprite(pSprite->index, kStatDecoration);
    seqSpawn(9, 3, pSprite->extra);
    if (Chance(0x8000))
        pSprite->cstat |= 4;

    sfxPlay3DSound(pSprite, 303, 24+(pSprite->flags&3), 1);
    sub_2A620(nSprite, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, 128, 0, 60, DAMAGE_TYPE_3, 15, 120, 0, 0);
    if (pXSprite->data4 > 1)
    {
        GibSprite(pSprite, GIBTYPE_5, NULL, NULL);
        int v14[2];
        v14[0] = pXSprite->data4>>1;
        v14[1] = pXSprite->data4-v14[0];
        int v4 = pSprite->ang;
        xvel[pSprite->index] = 0;
        yvel[pSprite->index] = 0;
        zvel[pSprite->index] = 0;
        for (int i = 0; i < 2; i++)
        {
            int t1 = Random(0x33333)+0x33333;
            int t2 = Random2(0x71);
            pSprite->ang = (t2+v4+2048)&2047;
            spritetype *pSprite2 = actFireThing(pSprite, 0, 0, -0x93d0, kThingNapalmBall, t1);
            XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
            pSprite2->owner = pSprite->owner;
            seqSpawn(61, 3, pSprite2->extra, nNapalmClient);
            pXSprite2->data4 = v14[i];
        }
    }
}

spritetype *actSpawnFloor(spritetype *pSprite)
{
    short nSector = pSprite->sectnum;
    int x = pSprite->x;
    int y = pSprite->y;
    updatesector(x, y, &nSector);
    int zFloor = getflorzofslope(nSector, x, y);
    spritetype *pSprite2 = actSpawnSprite(nSector, x, y, zFloor, 3, 0);
    if (pSprite2)
        pSprite2->cstat &= ~257;
    return pSprite2;
}

spritetype *actDropAmmo(spritetype *pSprite, int nType)
{
    spritetype *pSprite2 = NULL;
    if (pSprite && pSprite->statnum < kMaxStatus && nType >= kItemAmmoBase && nType < kItemAmmoMax)
    {
        pSprite2 = actSpawnFloor(pSprite);
        AMMOITEMDATA *pAmmo = &gAmmoItemData[nType - kItemAmmoBase];
        pSprite2->type = nType;
        pSprite2->picnum = pAmmo->picnum;
        pSprite2->shade = pAmmo->shade;
        pSprite2->xrepeat = pAmmo->xrepeat;
        pSprite2->yrepeat = pAmmo->yrepeat;
    }
    return pSprite2;
}

spritetype *actDropWeapon(spritetype *pSprite, int nType)
{
    spritetype *pSprite2 = NULL;
    if (pSprite && pSprite->statnum < kMaxStatus && nType >= kItemWeaponBase && nType < kItemWeaponMax)
    {
        pSprite2 = actSpawnFloor(pSprite);
        WEAPONITEMDATA *pWeapon = &gWeaponItemData[nType - kItemWeaponBase];
        pSprite2->type = nType;
        pSprite2->picnum = pWeapon->picnum;
        pSprite2->shade = pWeapon->shade;
        pSprite2->xrepeat = pWeapon->xrepeat;
        pSprite2->yrepeat = pWeapon->yrepeat;
    }
    return pSprite2;
}

spritetype *actDropItem(spritetype *pSprite, int nType)
{
    spritetype *pSprite2 = NULL;
    if (pSprite && pSprite->statnum < kMaxStatus && nType >= kItemBase && nType < kItemMax)
    {
        pSprite2 = actSpawnFloor(pSprite);
        ITEMDATA *pItem = &gItemData[nType - kItemBase];
        pSprite2->type = nType;
        pSprite2->picnum = pItem->picnum;
        pSprite2->shade = pItem->shade;
        pSprite2->xrepeat = pItem->xrepeat;
        pSprite2->yrepeat = pItem->yrepeat;
    }
    return pSprite2;
}

spritetype *actDropKey(spritetype *pSprite, int nType)
{
    spritetype *pSprite2 = NULL;
    if (pSprite && pSprite->statnum < kMaxStatus && nType >= kItemKeyBase && nType < kItemKeyMax)
    {
        pSprite2 = actDropItem(pSprite, nType);
        if (pSprite2 && gGameOptions.nGameType == 1)
        {
            if (pSprite2->extra == -1)
                dbInsertXSprite(pSprite2->index);
            xsprite[pSprite2->extra].respawn = 3;
            gSpriteHit[pSprite2->extra].florhit = 0;
            gSpriteHit[pSprite2->extra].ceilhit = 0;
        }
    }
    return pSprite2;
}

spritetype *actDropFlag(spritetype *pSprite, int nType)
{
    spritetype *pSprite2 = NULL;
    if (pSprite && pSprite->statnum < kMaxStatus && (nType == 147 || nType == 148))
    {
        pSprite2 = actDropItem(pSprite, nType);
        if (pSprite2 && gGameOptions.nGameType == 3)
        {
            evPost(pSprite2->index, 3, 1800, kCallbackReturnFlag);
        }
    }
    return pSprite2;
}

spritetype *actDropObject(spritetype *pSprite, int nType) {
    spritetype *pSprite2 = NULL;
    
    if (nType >= kItemKeyBase && nType < kItemKeyMax) pSprite2 = actDropKey(pSprite, nType);
    else if (nType == kItemFlagA || nType == kItemFlagB) pSprite2 = actDropFlag(pSprite, nType);
    else if (nType >= kItemBase && nType < kItemMax) pSprite2 = actDropItem(pSprite, nType);
    else if (nType >= kItemAmmoBase && nType < kItemAmmoMax) pSprite2 = actDropAmmo(pSprite, nType);
    else if (nType >= kItemWeaponBase && nType < kItemWeaponMax) pSprite2 = actDropWeapon(pSprite, nType);
    
    if (pSprite2) {
        int top, bottom;
        GetSpriteExtents(pSprite2, &top, &bottom);
        if (bottom >= pSprite2->z)
            pSprite2->z -= bottom - pSprite2->z;
    }

    return pSprite2;
}

bool actHealDude(XSPRITE *pXDude, int a2, int a3)
{
    dassert(pXDude != NULL);
    a2 <<= 4;
    a3 <<= 4;
    if (pXDude->health < a3)
    {
        spritetype *pSprite = &sprite[pXDude->reference];
        if (IsPlayerSprite(pSprite))
            sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, 780, pSprite->sectnum);
        pXDude->health = ClipHigh(pXDude->health+a2, a3);
        return 1;
    }
    return 0;
}

void actKillDude(int nKillerSprite, spritetype *pSprite, DAMAGE_TYPE damageType, int damage)
{
    spritetype *pKillerSprite = &sprite[nKillerSprite];
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    int nType = pSprite->type-kDudeBase;
    int nXSprite = pSprite->extra;
    dassert(nXSprite > 0);
    XSPRITE *pXSprite = &xsprite[pSprite->extra];
    
    switch (pSprite->type) {
    case kDudeModernCustom: {
        
        GENDUDEEXTRA* pExtra = genDudeExtra(pSprite);
        removeDudeStuff(pSprite);
        if (pXSprite->txID <= 0 || getNextIncarnation(pXSprite) == NULL) {
            
            if (pExtra->weaponType == kGenDudeWeaponKamikaze && Chance(0x4000) && damageType != 5 && damageType != 4) {
                doExplosion(pSprite, pXSprite->data1 - kTrapExploder);
                if (Chance(0x9000)) damageType = (DAMAGE_TYPE) 3;
            }

            if (damageType == DAMAGE_TYPE_1) {
                if (pExtra->availDeaths[DAMAGE_TYPE_1] && !spriteIsUnderwater(pSprite)) {
                    if (pExtra->canBurn) {
                        pSprite->type = kDudeModernCustomBurning;
                        if (pXSprite->data2 == kGenDudeDefaultSeq) // don't inherit palette for burning if using default animation
                            pSprite->pal = 0;

                        aiGenDudeNewState(pSprite, &genDudeBurnGoto);
                        actHealDude(pXSprite, dudeInfo[55].startHealth, dudeInfo[55].startHealth);
                        if (pXSprite->burnTime <= 0) pXSprite->burnTime = 1200;
                        gDudeExtra[pSprite->extra].at0 = (int)gFrameClock + 360;
                        return;
                    }

                } else {
                    pXSprite->burnTime = 0;
                    pXSprite->burnSource = -1;
                    damageType = DAMAGE_TYPE_0;
                }
            }
            
        } else {
            
            pXSprite->locked = 1; // lock while transforming

            aiSetGenIdleState(pSprite, pXSprite); // set idle state
            
            if (pXSprite->key > 0) // drop keys
                actDropObject(pSprite, kItemKeyBase + pXSprite->key - 1);
            
            if (pXSprite->dropMsg > 0) // drop items
                actDropObject(pSprite, pXSprite->dropMsg);
            
           
            pSprite->flags &= ~kPhysMove; xvel[pSprite->index] = yvel[pSprite->index] = 0;
            
            playGenDudeSound(pSprite, kGenDudeSndTransforming);
            int seqId = pXSprite->data2 + kGenDudeSeqTransform;
            if (gSysRes.Lookup(seqId, "SEQ")) seqSpawn(seqId, 3, nXSprite, -1);
            else {
                seqKill(3, nXSprite);
                spritetype* pEffect = gFX.fxSpawn((FX_ID)52, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, pSprite->ang);
                if (pEffect != NULL) {
                    pEffect->cstat = CSTAT_SPRITE_ALIGNMENT_FACING;
                    pEffect->pal = 6;
                    pEffect->xrepeat = pSprite->xrepeat;
                    pEffect->yrepeat = pSprite->yrepeat;
                }

                GIBTYPE nGibType;
                for (int i = 0; i < 3; i++) {
                    if (Chance(0x3000)) nGibType = GIBTYPE_6;
                    else if (Chance(0x2000)) nGibType = GIBTYPE_5;
                    else nGibType = GIBTYPE_17;

                    int top, bottom;
                    GetSpriteExtents(pSprite, &top, &bottom);
                    CGibPosition gibPos(pSprite->x, pSprite->y, top);
                    CGibVelocity gibVel(xvel[pSprite->index] >> 1, yvel[pSprite->index] >> 1, -0xccccc);
                    GibSprite(pSprite, nGibType, &gibPos, &gibVel);
                }
            }

            pXSprite->sysData1 = kGenDudeTransformStatus; // in transform
            return;
        }
        break;
    }
    case kDudeCerberusTwoHead: // Cerberus
        seqSpawn(dudeInfo[nType].seqStartID+1, 3, nXSprite, -1);
        return;
    case kDudeCultistTommy:
    case kDudeCultistShotgun:
    case kDudeCultistTesla:
    case kDudeCultistTNT:
        if (damageType == DAMAGE_TYPE_1 && pXSprite->medium == kMediumNormal)
        {
            pSprite->type = kDudeBurningCultist;
            aiNewState(pSprite, pXSprite, &cultistBurnGoto);
            actHealDude(pXSprite, dudeInfo[40].startHealth, dudeInfo[40].startHealth);
            return;
        }
        // no break
        fallthrough__;
    case kDudeBeast:
        if (damageType == DAMAGE_TYPE_1 && pXSprite->medium == kMediumNormal)
        {
            pSprite->type = kDudeBurningBeast;
            aiNewState(pSprite, pXSprite, &beastBurnGoto);
            actHealDude(pXSprite, dudeInfo[53].startHealth, dudeInfo[53].startHealth);
            return;
        }
        // no break
        fallthrough__;
    case kDudeInnocent:
        if (damageType == DAMAGE_TYPE_1 && pXSprite->medium == kMediumNormal)
        {
            pSprite->type = kDudeBurningInnocent;
            aiNewState(pSprite, pXSprite, &innocentBurnGoto);
            actHealDude(pXSprite, dudeInfo[39].startHealth, dudeInfo[39].startHealth);
            return;
        }
        break;
    }
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
    {
        if (gPlayer[p].fraggerId == pSprite->index && gPlayer[p].deathTime > 0)
            gPlayer[p].fraggerId = -1;
    }
    if (pSprite->type != kDudeCultistBeast)
        trTriggerSprite(pSprite->index, pXSprite, kCmdOff, nKillerSprite);

    pSprite->flags |= 7;
    if (VanillaMode()) {
        if (IsPlayerSprite(pKillerSprite)) {
            PLAYER *pPlayer = &gPlayer[pKillerSprite->type - kDudePlayer1];
            if (gGameOptions.nGameType == 1)
                pPlayer->fragCount++;
        }
    } else if (gGameOptions.nGameType == 1 && IsPlayerSprite(pKillerSprite) && pSprite->statnum == kStatDude) {
            switch (pSprite->type) {
                case kDudeBat:
                case kDudeRat:
                case kDudeInnocent:
                case kDudeBurningInnocent:
                    break;
                default:
                    PLAYER* pKillerPlayer = &gPlayer[pKillerSprite->type - kDudePlayer1];
                    pKillerPlayer->fragCount++;
                    break;
            }

     }

    if (pXSprite->key > 0)
        actDropObject(pSprite, kItemKeyBase + pXSprite->key - 1);
    
    if (pXSprite->dropMsg > 0)
        actDropObject(pSprite, pXSprite->dropMsg);
    
    switch (pSprite->type) {
        case kDudeCultistTommy: {
            int nRand = Random(100);
            if (nRand < 10) actDropObject(pSprite, kItemWeaponTommygun);
            else if (nRand < 50) actDropObject(pSprite, kItemAmmoTommygunFew);
        }
        break;
        case kDudeCultistShotgun: {
            int nRand = Random(100);
            if (nRand <= 10) actDropObject(pSprite, kItemWeaponSawedoff);
            else if (nRand <= 50) actDropObject(pSprite, kItemAmmoSawedoffFew);
        }
        break;
    }

    int nSeq;
    switch (damageType)
    {
    case DAMAGE_TYPE_3:
        nSeq = 2;
        switch (pSprite->type) {
            case kDudeModernCustom:
            case kDudeModernCustomBurning: {
                playGenDudeSound(pSprite, kGenDudeSndDeathExplode);
                GENDUDEEXTRA* pExtra = genDudeExtra(pSprite);
                if (!pExtra->availDeaths[damageType]) {
                    nSeq = 1; damageType = DAMAGE_TYPE_0;
                }
                break;
            }
            case kDudeCultistTommy:
            case kDudeCultistShotgun:
            case kDudeCultistTommyProne:
            case kDudeBurningInnocent:
            case kDudeBurningCultist:
            case kDudeInnocent:
            case kDudeCultistShotgunProne:
            case kDudeCultistTesla:
            case kDudeCultistTNT:
            case kDudeCultistBeast:
            case kDudeTinyCaleb:
            case kDudeBurningTinyCaleb:
                sfxPlay3DSound(pSprite, 717,-1,0);
                break;
        }
        break;
    case DAMAGE_TYPE_1:
        nSeq = 3;
        sfxPlay3DSound(pSprite, 351, -1, 0);
        break;
    case DAMAGE_TYPE_5:
        switch (pSprite->type) {
            case kDudeZombieAxeNormal:
            case kDudeZombieAxeBuried:
                nSeq = 14;
                break;
            case kDudeZombieButcher:
                nSeq = 11;
                break;
            default:
                nSeq = 1;
                break;
        }
        break;
    case DAMAGE_TYPE_0:
        switch (pSprite->type)
        {
        case kDudeCultistTommy:
        case kDudeCultistShotgun:
            nSeq = 1;
            break;
        default:
            nSeq = 1;
            break;
        }
        break;
    default:
        nSeq = 1;
        break;
    }

    if (!gSysRes.Lookup(dudeInfo[nType].seqStartID + nSeq, "SEQ"))
    {
        seqKill(3, nXSprite);
        gKillMgr.AddKill(pSprite);
        actPostSprite(pSprite->index, kStatFree);
        return;
    }

    switch (pSprite->type) {
    case kDudeZombieAxeNormal:
        sfxPlay3DSound(pSprite, 1107+Random(2), -1, 0);
        if (nSeq == 2) {
            
            seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, nDudeToGibClient1);
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            CGibPosition gibPos(pSprite->x, pSprite->y, top);
            CGibVelocity gibVel(xvel[pSprite->index]>>1, yvel[pSprite->index]>>1, -0xccccc);
            GibSprite(pSprite, GIBTYPE_27, &gibPos, &gibVel);
        
        } else if (nSeq == 1 && Chance(0x4000)) {
            
            seqSpawn(dudeInfo[nType].seqStartID+7, 3, nXSprite, nDudeToGibClient1);
            evPost(pSprite->index, 3, 0, kCallbackFXZombieSpurt);
            sfxPlay3DSound(pSprite, 362, -1, 0);
            pXSprite->data1 = 35;
            pXSprite->data2 = 5;
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            CGibPosition gibPos(pSprite->x, pSprite->y, top);
            CGibVelocity gibVel(xvel[pSprite->index] >> 1, yvel[pSprite->index] >> 1, -0x111111);
            GibSprite(pSprite, GIBTYPE_27, &gibPos, &gibVel);

        } else if (nSeq == 14)
            seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        else if (nSeq == 3)
            seqSpawn(dudeInfo[nType].seqStartID+13, 3, nXSprite, nDudeToGibClient2);
        else
            seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, nDudeToGibClient1);
        break;
    case kDudeCultistTommy:
    case kDudeCultistShotgun:
    case kDudeCultistTesla:
    case kDudeCultistTNT:
        sfxPlay3DSound(pSprite, 1018+Random(2), -1, 0);
        if (nSeq == 3)
            seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, nDudeToGibClient2);
        else
            seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, nDudeToGibClient1);
        break;
    case kDudeBurningCultist:
        if (Chance(0x4000) && nSeq == 3)
            sfxPlay3DSound(pSprite, 718, -1, 0);
        else
            sfxPlay3DSound(pSprite, 1018+Random(2), -1, 0);
        damageType = DAMAGE_TYPE_3;
        if (Chance(0x8000))
        {
            for (int i = 0; i < 3; i++)
                GibSprite(pSprite, GIBTYPE_7, NULL, NULL);
            seqSpawn(dudeInfo[nType].seqStartID+16-Random(1), 3, nXSprite, nDudeToGibClient1);
        }
        else
            seqSpawn(dudeInfo[nType].seqStartID+15, 3, nXSprite, nDudeToGibClient2);
        break;
    case kDudeModernCustom:
        playGenDudeSound(pSprite, kGenDudeSndDeathNormal);
        if (nSeq == 3) {
            
            GENDUDEEXTRA* pExtra = genDudeExtra(pSprite);
            if (pExtra->availDeaths[kDmgBurn] == 3) seqSpawn((15 + Random(2)) + pXSprite->data2, 3, nXSprite, nDudeToGibClient2);
            else if (pExtra->availDeaths[kDmgBurn] == 2) seqSpawn(16 + pXSprite->data2, 3, nXSprite, nDudeToGibClient2);
            else if (pExtra->availDeaths[kDmgBurn] == 1) seqSpawn(15 + pXSprite->data2, 3, nXSprite, nDudeToGibClient2);
            else if (gSysRes.Lookup(pXSprite->data2 + nSeq, "SEQ"))seqSpawn(nSeq + pXSprite->data2, 3, nXSprite, nDudeToGibClient2);
            else seqSpawn(1 + pXSprite->data2, 3, nXSprite, nDudeToGibClient2);

         } else {
            seqSpawn(nSeq + pXSprite->data2, 3, nXSprite, nDudeToGibClient1);
         }

        pXSprite->txID = 0; // to avoid second trigger.
        break;

    case kDudeModernCustomBurning: {
        playGenDudeSound(pSprite, kGenDudeSndDeathExplode);
        damageType = DAMAGE_TYPE_3;

        if (Chance(0x4000)) {
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            CGibPosition gibPos(pSprite->x, pSprite->y, top);
            CGibVelocity gibVel(xvel[pSprite->index] >> 1, yvel[pSprite->index] >> 1, -0xccccc);
            GibSprite(pSprite, GIBTYPE_7, &gibPos, &gibVel);
        }

        GENDUDEEXTRA* pExtra = genDudeExtra(pSprite);
        if (pExtra->availDeaths[kDmgBurn] == 3) seqSpawn((15 + Random(2)) + pXSprite->data2, 3, nXSprite, nDudeToGibClient1);
        else if (pExtra->availDeaths[kDmgBurn] == 2) seqSpawn(16 + pXSprite->data2, 3, nXSprite, nDudeToGibClient1);
        else if (pExtra->availDeaths[kDmgBurn] == 1) seqSpawn(15 + pXSprite->data2, 3, nXSprite, nDudeToGibClient1);
        else seqSpawn(1 + pXSprite->data2, 3, nXSprite, nDudeToGibClient1);
        break;
    }
    case kDudeBurningZombieAxe:
        if (Chance(0x8000) && nSeq == 3)
            sfxPlay3DSound(pSprite, 1109, -1, 0);
        else
            sfxPlay3DSound(pSprite, 1107+Random(2), -1, 0);
        damageType = DAMAGE_TYPE_3;
        if (Chance(0x8000))
        {
            seqSpawn(dudeInfo[nType].seqStartID+13, 3, nXSprite, nDudeToGibClient1);
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            CGibPosition gibPos(pSprite->x, pSprite->y, top);
            CGibVelocity gibVel(xvel[pSprite->index]>>1, yvel[pSprite->index]>>1, -0xccccc);
            GibSprite(pSprite, GIBTYPE_27, &gibPos, &gibVel);
        }
        else
            seqSpawn(dudeInfo[nType].seqStartID+13, 3, nXSprite, nDudeToGibClient2);
        break;
    case kDudeBurningZombieButcher:
        if (Chance(0x4000) && nSeq == 3)
            sfxPlay3DSound(pSprite, 1206, -1, 0);
        else
            sfxPlay3DSound(pSprite, 1204+Random(2), -1, 0);
        seqSpawn(dudeInfo[4].seqStartID+10, 3, nXSprite, -1);
        break;
    case kDudeBurningInnocent:
        damageType = DAMAGE_TYPE_3;
        seqSpawn(dudeInfo[nType].seqStartID+7, 3, nXSprite, nDudeToGibClient1);
        break;
    case kDudeZombieButcher:
        if (nSeq == 14) {
            sfxPlay3DSound(pSprite, 1206, -1, 0);
            seqSpawn(dudeInfo[nType].seqStartID+11, 3, nXSprite, -1);
            break;
        }
        sfxPlay3DSound(pSprite, 1204+Random(2), -1, 0);
        if (nSeq == 3)
            seqSpawn(dudeInfo[nType].seqStartID+10, 3, nXSprite, -1);
        else
            seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudeGargoyleFlesh:
        if (Chance(0x4000) && nSeq == 3) sfxPlay3DSound(pSprite, 1405, -1, 0);
        else sfxPlay3DSound(pSprite, 1403+Random(2), -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudeGargoyleStone:
        if (Chance(0x4000) && nSeq == 3) sfxPlay3DSound(pSprite, 1455, -1, 0);
        else sfxPlay3DSound(pSprite, 1453+Random(2), -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudePhantasm:
        if (Chance(0x4000) && nSeq == 3) sfxPlay3DSound(pSprite, 1605, -1, 0);
        else sfxPlay3DSound(pSprite, 1603+Random(2), -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudeHellHound:
        if (Chance(0x4000) && nSeq == 3) sfxPlay3DSound(pSprite, 1305, -1, 0);
        else sfxPlay3DSound(pSprite, 1303+Random(2), -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudeHand:
        if (Chance(0x4000) && nSeq == 3) sfxPlay3DSound(pSprite, 1905, -1, 0);
        else sfxPlay3DSound(pSprite, 1903+Random(2), -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudeSpiderBrown:
        if (pSprite->owner != -1) {
            spritetype *pOwner = &sprite[actSpriteOwnerToSpriteId(pSprite)];
            gDudeExtra[pOwner->extra].at6.u1.at4--;
        }
        
        if (Chance(0x4000) && nSeq == 3) sfxPlay3DSound(pSprite, 1805, -1, 0);
        else sfxPlay3DSound(pSprite, 1803+Random(2), -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudeSpiderRed:
        if (pSprite->owner != -1) {
            spritetype *pOwner = &sprite[actSpriteOwnerToSpriteId(pSprite)];
            gDudeExtra[pOwner->extra].at6.u1.at4--;
        }
        
        if (Chance(0x4000) && nSeq == 3) sfxPlay3DSound(pSprite, 1805, -1, 0);
        else sfxPlay3DSound(pSprite, 1803+Random(2), -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudeSpiderBlack:
        if (pSprite->owner != -1) {
            spritetype *pOwner = &sprite[actSpriteOwnerToSpriteId(pSprite)];
            gDudeExtra[pOwner->extra].at6.u1.at4--;
        }
        
        if (Chance(0x4000) && nSeq == 3) sfxPlay3DSound(pSprite, 1805, -1, 0);
        else sfxPlay3DSound(pSprite, 1803+Random(2), -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudeSpiderMother:
        sfxPlay3DSound(pSprite, 1850, -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudeGillBeast:
        if (Chance(0x4000) && nSeq == 3) sfxPlay3DSound(pSprite, 1705, -1, 0);
        else sfxPlay3DSound(pSprite, 1703+Random(2), -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudeBoneEel:
        if (Chance(0x4000) && nSeq == 3) sfxPlay3DSound(pSprite, 1505, -1, 0);
        else sfxPlay3DSound(pSprite, 1503+Random(2), -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudeBat:
        if (Chance(0x4000) && nSeq == 3)
            sfxPlay3DSound(pSprite, 2005, -1, 0);
        else
            sfxPlay3DSound(pSprite, 2003+Random(2), -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudeRat:
        if (Chance(0x4000) && nSeq == 3)
            sfxPlay3DSound(pSprite, 2105, -1, 0);
        else
            sfxPlay3DSound(pSprite, 2103+Random(2), -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudePodGreen:
    case kDudeTentacleGreen:
    case kDudePodFire:
    case kDudeTentacleFire:
        if ((pSprite->cstat & CSTAT_SPRITE_YFLIP)) pSprite->cstat &= ~CSTAT_SPRITE_YFLIP;
        switch (pSprite->type) {
            case kDudePodGreen:
                if (Chance(0x4000) && nSeq == 3)
                    sfxPlay3DSound(pSprite, 2205, -1, 0);
                else
                    sfxPlay3DSound(pSprite, 2203 + Random(2), -1, 0);
                seqSpawn(dudeInfo[nType].seqStartID + nSeq, 3, nXSprite, -1);
                break;
            case kDudeTentacleGreen:
                if (damage == 5)
                    sfxPlay3DSound(pSprite, 2471, -1, 0);
                else
                    sfxPlay3DSound(pSprite, 2472, -1, 0);
                seqSpawn(dudeInfo[nType].seqStartID + nSeq, 3, nXSprite, -1);
                break;
            case kDudePodFire:
                if (damage == 5)
                    sfxPlay3DSound(pSprite, 2451, -1, 0);
                else
                    sfxPlay3DSound(pSprite, 2452, -1, 0);
                seqSpawn(dudeInfo[nType].seqStartID + nSeq, 3, nXSprite, -1);
                break;
            case kDudeTentacleFire:
                sfxPlay3DSound(pSprite, 2501, -1, 0);
                seqSpawn(dudeInfo[nType].seqStartID + nSeq, 3, nXSprite, -1);
                break;
        }
        break;
    case kDudePodMother:
        if (Chance(0x4000) && nSeq == 3)
            sfxPlay3DSound(pSprite, 2205, -1, 0);
        else
            sfxPlay3DSound(pSprite, 2203+Random(2), -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudeTentacleMother:
        if (Chance(0x4000) && nSeq == 3)
            sfxPlay3DSound(pSprite, 2205, -1, 0);
        else
            sfxPlay3DSound(pSprite, 2203+Random(2), -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudeCerberusTwoHead:
        if (Chance(0x4000) && nSeq == 3)
            sfxPlay3DSound(pSprite, 2305, -1, 0);
        else
            sfxPlay3DSound(pSprite, 2305+Random(2), -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudeCerberusOneHead:
        if (Chance(0x4000) && nSeq == 3)
            sfxPlay3DSound(pSprite, 2305, -1, 0);
        else
            sfxPlay3DSound(pSprite, 2305+Random(2), -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudeTchernobog:
        sfxPlay3DSound(pSprite, 2380, -1, 0);
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    case kDudeBurningTinyCaleb:
        damageType = DAMAGE_TYPE_3;
        seqSpawn(dudeInfo[nType].seqStartID+11, 3, nXSprite, nDudeToGibClient1);
        break;
    case kDudeBeast:
        sfxPlay3DSound(pSprite, 9000+Random(2), -1, 0);
        if (nSeq == 3)
            seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, nDudeToGibClient2);
        else
            seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, nDudeToGibClient1);
        break;
    case kDudeBurningBeast:
        damageType = DAMAGE_TYPE_3;
        seqSpawn(dudeInfo[nType].seqStartID+12, 3, nXSprite, nDudeToGibClient1);
        break;
    default:
        seqSpawn(dudeInfo[nType].seqStartID+nSeq, 3, nXSprite, -1);
        break;
    }
    
    if (damageType == DAMAGE_TYPE_3)
    {
        DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
        for (int i = 0; i < 3; i++)
            if (pDudeInfo->nGibType[i] > -1)
                GibSprite(pSprite, (GIBTYPE)pDudeInfo->nGibType[i], NULL, NULL);
        for (int i = 0; i < 4; i++)
            fxSpawnBlood(pSprite, damage);
    }
    gKillMgr.AddKill(pSprite);
    actCheckRespawn(pSprite);
    pSprite->type = kThingBloodChunks;
    actPostSprite(pSprite->index, kStatThing);
}

int actDamageSprite(int nSource, spritetype *pSprite, DAMAGE_TYPE damageType, int damage) {
    dassert(nSource < kMaxSprites);

    if (pSprite->flags&32 || pSprite->extra <= 0 || pSprite->extra >= kMaxXSprites || xsprite[pSprite->extra].reference != pSprite->index) 
        return 0;
    
    XSPRITE *pXSprite = &xsprite[pSprite->extra];
    if ((pXSprite->health == 0 && pSprite->statnum != kStatDude) || pXSprite->locked)
        return 0;

    if (nSource == -1) 
        nSource = pSprite->index;
    
    PLAYER *pSourcePlayer = NULL;
    if (IsPlayerSprite(&sprite[nSource])) pSourcePlayer = &gPlayer[sprite[nSource].type - kDudePlayer1];
    if (!gGameOptions.bFriendlyFire && IsTargetTeammate(pSourcePlayer, pSprite)) return 0;
    
    switch (pSprite->statnum) {
        case kStatDude: {
            if (!IsDudeSprite(pSprite)) {
                consoleSysMsg("Bad Dude Failed: initial=%d type=%d %s\n", (int)pSprite->inittype, (int)pSprite->type, (int)(pSprite->flags & kHitagRespawn) ? "RESPAWN" : "NORMAL");
                return damage >> 4;
                //ThrowError("Bad Dude Failed: initial=%d type=%d %s\n", (int)pSprite->inittype, (int)pSprite->type, (int)(pSprite->flags & 16) ? "RESPAWN" : "NORMAL");
            }

            int nType = pSprite->type - kDudeBase; int nDamageFactor = dudeInfo[nType].at70[damageType];
            if (pSprite->type == kDudeModernCustom)
                nDamageFactor = gGenDudeExtra[pSprite->index].dmgControl[damageType];

            if (!nDamageFactor) return 0;
            else if (nDamageFactor != 256)
                damage = mulscale8(damage, nDamageFactor);

            if (!IsPlayerSprite(pSprite)) {
            
                if (pXSprite->health <= 0) return 0;
                damage = aiDamageSprite(pSprite, pXSprite, nSource, damageType, damage);
                if (pXSprite->health <= 0)
                    actKillDude(nSource, pSprite, ((damageType == DAMAGE_TYPE_3 && damage < 160) ? DAMAGE_TYPE_0 : damageType), damage);
        
            } else {
            
                PLAYER *pPlayer = &gPlayer[pSprite->type - kDudePlayer1];
                if (pXSprite->health > 0 || playerSeqPlaying(pPlayer, 16))
                    damage = playerDamageSprite(nSource, pPlayer, damageType, damage);

            }
        }
        break;
    case kStatThing:
        dassert(pSprite->type >= kThingBase && pSprite->type < kThingMax);
        int nType = pSprite->type - kThingBase; int nDamageFactor = thingInfo[nType].dmgControl[damageType];
        
        if (!nDamageFactor) return 0;
        else if (nDamageFactor != 256)
            damage = mulscale8(damage, nDamageFactor);

        pXSprite->health = ClipLow(pXSprite->health - damage, 0);
        if (pXSprite->health <= 0) {
            switch (pSprite->type) {
                case kThingDroppedLifeLeech:
                case kModernThingEnemyLifeLeech:
                    GibSprite(pSprite, GIBTYPE_14, NULL, NULL);
                    pXSprite->data1 = pXSprite->data2 =  pXSprite->data3 = pXSprite->DudeLockout = 0;
                    pXSprite->stateTimer =  pXSprite->data4 = pXSprite->isTriggered = 0;

                    if (pSprite->owner >= 0 && sprite[pSprite->owner].type == kDudeModernCustom)
                        sprite[pSprite->owner].owner = kMaxSprites - 1; // By NoOne: indicates if custom dude had life leech.
                    break;
                default:
                    if (!(pSprite->flags & kHitagRespawn))
                        actPropagateSpriteOwner(pSprite, &sprite[nSource]);
                    break;
            }

            trTriggerSprite(pSprite->index, pXSprite, kCmdOff, nSource);
            
            switch (pSprite->type) {
                case kThingObjectGib:
                case kThingObjectExplode:
                case kThingBloodBits:
                case kThingBloodChunks:
                case kThingZombieHead:
                    if (damageType == 3 && pSourcePlayer && gFrameClock > pSourcePlayer->laughCount && Chance(0x4000)) {
                        sfxPlay3DSound(pSourcePlayer->pSprite, gPlayerGibThingComments[Random(10)], 0, 2);
                        pSourcePlayer->laughCount = (int)gFrameClock+3600;
                    }
                    break;
                case kTrapMachinegun:
                    seqSpawn(28, 3, pSprite->extra, -1);
                    break;
                case kThingFluorescent:
                    seqSpawn(12, 3, pSprite->extra, -1);
                    GibSprite(pSprite, GIBTYPE_6, NULL, NULL);
                    break;
                case kThingSpiderWeb:
                    seqSpawn(15, 3, pSprite->extra, -1);
                    break;
                case kThingMetalGrate:
                    seqSpawn(21, 3, pSprite->extra, -1);
                    GibSprite(pSprite, GIBTYPE_4, NULL, NULL);
                    break;
                case kThingFlammableTree:
                    switch (pXSprite->data1) {
                        case -1:
                            GibSprite(pSprite, GIBTYPE_14, NULL, NULL);
                            sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, 312, pSprite->sectnum);
                            actPostSprite(pSprite->index, kStatFree);
                            break;
                        case 0:
                            seqSpawn(25, 3, pSprite->extra, nTreeToGibClient);
                            sfxPlay3DSound(pSprite, 351, -1, 0);
                            break;
                        case 1:
                            seqSpawn(26, 3, pSprite->extra, nTreeToGibClient);
                            sfxPlay3DSound(pSprite, 351, -1, 0);
                            break;
                    }
                    break;
            }
        }
        break;
    }
    
    return damage >> 4;
}

void actHitcodeToData(int a1, HITINFO *pHitInfo, int *a3, spritetype **a4, XSPRITE **a5, int *a6, walltype **a7, XWALL **a8, int *a9, sectortype **a10, XSECTOR **a11)
{
    dassert(pHitInfo != NULL);
    int nSprite = -1;
    spritetype *pSprite = NULL;
    XSPRITE *pXSprite = NULL;
    int nWall = -1;
    walltype *pWall = NULL;
    XWALL *pXWall = NULL;
    int nSector = -1;
    sectortype *pSector = NULL;
    XSECTOR *pXSector = NULL;
    switch (a1)
    {
    case 3:
    case 5:
        nSprite = pHitInfo->hitsprite;
        dassert(nSprite >= 0 && nSprite < kMaxSprites);
        pSprite = &sprite[nSprite];
        if (pSprite->extra > 0)
            pXSprite = &xsprite[pSprite->extra];
        break;
    case 0:
    case 4:
        nWall = pHitInfo->hitwall;
        dassert(nWall >= 0 && nWall < kMaxWalls);
        pWall = &wall[nWall];
        if (pWall->extra > 0)
            pXWall = &xwall[pWall->extra];
        break;
    case 1:
    case 2:
    case 6:
        nSector = pHitInfo->hitsect;
        dassert(nSector >= 0 && nSector < kMaxSectors);
        pSector = &sector[nSector];
        if (pSector->extra > 0)
            pXSector = &xsector[pSector->extra];
        break;
    }
    if (a3)
        *a3 = nSprite;
    if (a4)
        *a4 = pSprite;
    if (a5)
        *a5 = pXSprite;
    if (a6)
        *a6 = nWall;
    if (a7)
        *a7 = pWall;
    if (a8)
        *a8 = pXWall;
    if (a9)
        *a9 = nSector;
    if (a10)
        *a10 = pSector;
    if (a11)
        *a11 = pXSector;
}

void actImpactMissile(spritetype *pMissile, int hitCode)
{
    int nXMissile = pMissile->extra;
    dassert(nXMissile > 0 && nXMissile < kMaxXSprites);
    XSPRITE *pXMissile = &xsprite[pMissile->extra];
    
    int nSpriteHit = -1; int nWallHit = -1; int nSectorHit = -1;
    spritetype *pSpriteHit = NULL; XSPRITE *pXSpriteHit = NULL;
    walltype *pWallHit = NULL; XWALL *pXWallHit = NULL;
    sectortype *pSectorHit = NULL; XSECTOR *pXSectorHit = NULL;

    actHitcodeToData(hitCode, &gHitInfo, &nSpriteHit, &pSpriteHit, &pXSpriteHit, &nWallHit, &pWallHit, &pXWallHit, &nSectorHit, &pSectorHit, &pXSectorHit);
    const THINGINFO *pThingInfo = NULL; DUDEINFO *pDudeInfo = NULL;

    if (hitCode == 3 && pSpriteHit) {
        switch (pSpriteHit->statnum) {
            case kStatThing:
                pThingInfo = &thingInfo[pSpriteHit->type - kThingBase];
                break;
            case kStatDude:
                pDudeInfo = &dudeInfo[pSpriteHit->type - kDudeBase];
                break;
        }
    }
    switch (pMissile->type) {
        case kMissileLifeLeechRegular:
            if (hitCode == 3 && pSpriteHit && (pThingInfo || pDudeInfo)) {
                int nOwner = actSpriteOwnerToSpriteId(pMissile);
                DAMAGE_TYPE rand1 = (DAMAGE_TYPE)Random(7);
                int rand2 = (7 + Random(7)) << 4;
                int nDamage = actDamageSprite(nOwner, pSpriteHit, rand1, rand2);
                if ((pThingInfo && pThingInfo->dmgControl[DAMAGE_TYPE_1] != 0) || (pDudeInfo && pDudeInfo->at70[DAMAGE_TYPE_1] != 0))
                    actBurnSprite(pMissile->owner, pXSpriteHit, 360);

                // by NoOne: make Life Leech heal user, just like it was in 1.0x versions
                if (gGameOptions.weaponsV10x && !VanillaMode() && !DemoRecordStatus() && pDudeInfo != NULL) {
                    spritetype* pSource = &sprite[nOwner];
                    XSPRITE* pXSource = (pSource->extra >= 0) ? &xsprite[pSource->extra] : NULL;

                    if (IsDudeSprite(pSource) && pXSource != NULL && pXSource->health != 0)

                        actHealDude(pXSource, nDamage >> 2, dudeInfo[pSource->type - kDudeBase].startHealth);
                }
            }
        
            if (pMissile->extra > 0) {
                actPostSprite(pMissile->index, kStatDecoration);
                if (pMissile->ang == 1024) sfxPlay3DSound(pMissile, 307, -1, 0);
                pMissile->type = kSpriteDecoration;
                seqSpawn(9, 3, pMissile->extra, -1);
            } else {
                actPostSprite(pMissile->index, kStatFree);
            }

            break;
        case kMissileTeslaAlt:
            sub_51340(pMissile, hitCode);
            switch (hitCode) {
                case 0:
                case 4:
                    if (pWallHit) {
                        spritetype* pFX = gFX.fxSpawn(FX_52, pMissile->sectnum, pMissile->x, pMissile->y, pMissile->z, 0);
                        if (pFX) pFX->ang = (GetWallAngle(nWallHit) + 512) & 2047;
                    }
                    break;
            }
            GibSprite(pMissile, GIBTYPE_24, NULL, NULL);
            actPostSprite(pMissile->index, kStatFree);
            break;
        case kMissilePukeGreen:
            seqKill(3, nXMissile);
            if (hitCode == 3 && pSpriteHit && (pThingInfo || pDudeInfo))
            {
                int nOwner = actSpriteOwnerToSpriteId(pMissile);
                int nDamage = (15+Random(7))<<4;
                actDamageSprite(nOwner, pSpriteHit, DAMAGE_TYPE_2, nDamage);
            }
            actPostSprite(pMissile->index, kStatFree);
            break;
        case kMissileArcGargoyle:
            sfxKill3DSound(pMissile, -1, -1);
            sfxPlay3DSound(pMissile->x, pMissile->y, pMissile->z, 306, pMissile->sectnum);
            GibSprite(pMissile, GIBTYPE_6, NULL, NULL);
            if (hitCode == 3 && pSpriteHit && (pThingInfo || pDudeInfo))
            {
                int nOwner = actSpriteOwnerToSpriteId(pMissile);
                int nDamage = (25+Random(20))<<4;
                actDamageSprite(nOwner, pSpriteHit, DAMAGE_TYPE_5, nDamage);
            }
            actPostSprite(pMissile->index, kStatFree);
            break;
        case kMissileLifeLeechAltNormal:
        case kMissileLifeLeechAltSmall:
            sfxKill3DSound(pMissile, -1, -1);
            sfxPlay3DSound(pMissile->x, pMissile->y, pMissile->z, 306, pMissile->sectnum);
            if (hitCode == 3 && pSpriteHit && (pThingInfo || pDudeInfo)) {
                int nOwner = actSpriteOwnerToSpriteId(pMissile);
                int nDmgMul = (pMissile->type == kMissileLifeLeechAltSmall) ? 6 : 3;
                int nDamage = (nDmgMul+Random(nDmgMul))<<4;
                actDamageSprite(nOwner, pSpriteHit, DAMAGE_TYPE_5, nDamage);
            }
            actPostSprite(pMissile->index, kStatFree);
            break;
        case kMissileFireball:
        case kMissileFireballNapam:
            if (hitCode == 3 && pSpriteHit && (pThingInfo || pDudeInfo))
            {
                if (pThingInfo && pSpriteHit->type == kThingTNTBarrel && pXSpriteHit->burnTime == 0)
                    evPost(nSpriteHit, 3, 0, kCallbackFXFlameLick);
                int nOwner = actSpriteOwnerToSpriteId(pMissile);
                int nDamage = (50+Random(50))<<4;
                actDamageSprite(nOwner, pSpriteHit, DAMAGE_TYPE_2, nDamage);
            }
            actExplodeSprite(pMissile);
            break;
        case kMissileFlareAlt:
            sfxKill3DSound(pMissile, -1, -1);
            actExplodeSprite(pMissile);
            break;
        case kMissileFlareRegular:
            sfxKill3DSound(pMissile, -1, -1);
            if ((hitCode == 3 && pSpriteHit) && (pThingInfo || pDudeInfo)) {
                int nOwner = actSpriteOwnerToSpriteId(pMissile);
                if ((pThingInfo && pThingInfo->dmgControl[DAMAGE_TYPE_1] != 0) || (pDudeInfo && pDudeInfo->at70[DAMAGE_TYPE_1] != 0)) {
                    if (pThingInfo && pSpriteHit->type == kThingTNTBarrel && pXSpriteHit->burnTime == 0)
                        evPost(nSpriteHit, 3, 0, kCallbackFXFlameLick);
                
                    actBurnSprite(pMissile->owner, pXSpriteHit, 480);
                    sub_2A620(nOwner, pMissile->x, pMissile->y, pMissile->z, pMissile->sectnum, 16, 20, 10, DAMAGE_TYPE_2, 6, 480, 0, 0);

                    // by NoOne: allow additional bullet damage for Flare Gun
                    if (gGameOptions.weaponsV10x && !VanillaMode() && !DemoRecordStatus()) {
                        int nDamage = (20 + Random(10)) << 4;
                        actDamageSprite(nOwner, pSpriteHit, DAMAGE_TYPE_2, nDamage);
                    }
                } else  {
                    int nDamage = (20+Random(10))<<4;
                    actDamageSprite(nOwner, pSpriteHit, DAMAGE_TYPE_2, nDamage);
                }
            
                if (surfType[pSpriteHit->picnum] == kSurfFlesh) {
                    pMissile->picnum = 2123;
                    pXMissile->target = nSpriteHit;
                    pXMissile->targetZ = pMissile->z-pSpriteHit->z;
                    pXMissile->goalAng = getangle(pMissile->x-pSpriteHit->x, pMissile->y-pSpriteHit->y)-pSpriteHit->ang;
                    pXMissile->state = 1;
                    actPostSprite(pMissile->index, kStatFlare);
                    pMissile->cstat &= ~257;
                    break;
                }
            }
            GibSprite(pMissile, GIBTYPE_17, NULL, NULL);
            actPostSprite(pMissile->index, kStatFree);
            break;
        case kMissileFlameSpray:
        case kMissileFlameHound:
            if (hitCode == 3)
            {
                int nObject = gHitInfo.hitsprite;
                dassert(nObject >= 0 && nObject < kMaxSprites);
                spritetype *pObject = &sprite[nObject];
                if (pObject->extra > 0)
                {
                    XSPRITE *pXObject = &xsprite[pObject->extra];
                    if ((pObject->statnum == kStatThing || pObject->statnum == kStatDude) && pXObject->burnTime == 0)
                        evPost(nObject, 3, 0, kCallbackFXFlameLick);
                    int nOwner = actSpriteOwnerToSpriteId(pMissile);
                    actBurnSprite(pMissile->owner, pXObject, (4+gGameOptions.nDifficulty)<<2);
                    actDamageSprite(nOwner, pObject, DAMAGE_TYPE_1, 8);
                }
            }
            break;
        case kMissileFireballCerberus:
            actExplodeSprite(pMissile);
            if (hitCode == 3)
            {
                int nObject = gHitInfo.hitsprite;
                dassert(nObject >= 0 && nObject < kMaxSprites);
                spritetype *pObject = &sprite[nObject];
                if (pObject->extra > 0)
                {
                    XSPRITE *pXObject = &xsprite[pObject->extra];
                    if ((pObject->statnum == kStatThing || pObject->statnum == kStatDude) && pXObject->burnTime == 0)
                        evPost(nObject, 3, 0, kCallbackFXFlameLick);
                    int nOwner = actSpriteOwnerToSpriteId(pMissile);
                    actBurnSprite(pMissile->owner, pXObject, (4+gGameOptions.nDifficulty)<<2);
                    actDamageSprite(nOwner, pObject, DAMAGE_TYPE_1, 8);
                    int nDamage = (25+Random(10))<<4;
                    actDamageSprite(nOwner, pObject, DAMAGE_TYPE_2, nDamage);
                }
            }
            actExplodeSprite(pMissile);
            break;
        case kMissileFireballTchernobog:
            actExplodeSprite(pMissile);
            if (hitCode == 3)
            {
                int nObject = gHitInfo.hitsprite;
                dassert(nObject >= 0 && nObject < kMaxSprites);
                spritetype *pObject = &sprite[nObject];
                if (pObject->extra > 0)
                {
                    XSPRITE *pXObject = &xsprite[pObject->extra];
                    if ((pObject->statnum == kStatThing || pObject->statnum == kStatDude) && pXObject->burnTime == 0)
                        evPost(nObject, 3, 0, kCallbackFXFlameLick);
                    int nOwner = actSpriteOwnerToSpriteId(pMissile);
                    actBurnSprite(pMissile->owner, pXObject, 32);
                    actDamageSprite(nOwner, pObject, DAMAGE_TYPE_5, 12);
                    int nDamage = (25+Random(10))<<4;
                    actDamageSprite(nOwner, pObject, DAMAGE_TYPE_2, nDamage);
                }
            }
            actExplodeSprite(pMissile);
            break;
        case kMissileEctoSkull:
            sfxKill3DSound(pMissile, -1, -1);
            sfxPlay3DSound(pMissile->x, pMissile->y, pMissile->z, 522, pMissile->sectnum);
            actPostSprite(pMissile->index, kStatDebris);
            seqSpawn(20, 3, pMissile->extra, -1);
            if (hitCode == 3)
            {
                int nObject = gHitInfo.hitsprite;
                dassert(nObject >= 0 && nObject < kMaxSprites);
                spritetype *pObject = &sprite[nObject];
                if (pObject->statnum == kStatDude)
                {
                    int nOwner = actSpriteOwnerToSpriteId(pMissile);
                    int nDamage = (25+Random(10))<<4;
                    actDamageSprite(nOwner, pObject, DAMAGE_TYPE_5, nDamage);
                }
            }
            break;
        case kMissileButcherKnife:
            actPostSprite(pMissile->index, kStatDebris);
            pMissile->cstat &= ~16;
            pMissile->type = kSpriteDecoration;
            seqSpawn(20, 3, pMissile->extra, -1);
            if (hitCode == 3)
            {
                int nObject = gHitInfo.hitsprite;
                dassert(nObject >= 0 && nObject < kMaxSprites);
                spritetype *pObject = &sprite[nObject];
                if (pObject->statnum == kStatDude)
                {
                    int nOwner = actSpriteOwnerToSpriteId(pMissile);
                    int nDamage = (10+Random(10))<<4;
                    actDamageSprite(nOwner, pObject, DAMAGE_TYPE_5, nDamage);
                    spritetype *pOwner = &sprite[nOwner];
                    XSPRITE *pXOwner = &xsprite[pOwner->extra];
                    int nType = pOwner->type-kDudeBase;
                    if (pXOwner->health > 0)
                        actHealDude(pXOwner, 10, dudeInfo[nType].startHealth);
                }
            }
            break;
        case kMissileTeslaRegular:
            sfxKill3DSound(pMissile, -1, -1);
            sfxPlay3DSound(pMissile->x, pMissile->y, pMissile->z, 518, pMissile->sectnum);
            GibSprite(pMissile, (hitCode == 2) ? GIBTYPE_23 : GIBTYPE_22, NULL, NULL);
            evKill(pMissile->index, 3);
            seqKill(3, nXMissile);
            actPostSprite(pMissile->index, kStatFree);
            if (hitCode == 3)
            {
                int nObject = gHitInfo.hitsprite;
                dassert(nObject >= 0 && nObject < kMaxSprites);
                spritetype *pObject = &sprite[nObject];
                int nOwner = actSpriteOwnerToSpriteId(pMissile);
                int nDamage = (15+Random(10))<<4;
                actDamageSprite(nOwner, pObject, DAMAGE_TYPE_6, nDamage);
            }
            break;
        default:
            seqKill(3, nXMissile);
            actPostSprite(pMissile->index, kStatFree);
            if (hitCode == 3)
            {
                int nObject = gHitInfo.hitsprite;
                dassert(nObject >= 0 && nObject < kMaxSprites);
                spritetype *pObject = &sprite[nObject];
                int nOwner = actSpriteOwnerToSpriteId(pMissile);
                int nDamage = (10+Random(10))<<4;
                actDamageSprite(nOwner, pObject, DAMAGE_TYPE_0, nDamage);
            }
            break;
    }
    pMissile->cstat &= ~257;
}

void actKickObject(spritetype *pSprite1, spritetype *pSprite2)
{
    int nSprite1 = pSprite1->index;
    int nSprite2 = pSprite2->index;
    int nSpeed = ClipLow(approxDist(xvel[nSprite1], yvel[nSprite1])*2, 0xaaaaa);
    xvel[nSprite2] = mulscale30(nSpeed, Cos(pSprite1->ang+Random2(85)));
    yvel[nSprite2] = mulscale30(nSpeed, Sin(pSprite1->ang+Random2(85)));
    zvel[nSprite2] = mulscale(nSpeed, -0x2000, 14);
    pSprite2->flags = 7;
}

void actTouchFloor(spritetype *pSprite, int nSector)
{
    dassert(pSprite != NULL);
    dassert(nSector >= 0 && nSector < kMaxSectors);
    sectortype * pSector = &sector[nSector];
    XSECTOR * pXSector = NULL;
    if (pSector->extra > 0)
        pXSector = &xsector[pSector->extra];


    if (pXSector && (pSector->type == kSectorDamage || pXSector->damageType > 0))
    {
        DAMAGE_TYPE nDamageType;

        if (pSector->type == kSectorDamage)
            nDamageType = (DAMAGE_TYPE)ClipRange(pXSector->damageType, DAMAGE_TYPE_0, DAMAGE_TYPE_6);
        else
            nDamageType = (DAMAGE_TYPE)ClipRange(pXSector->damageType - 1, DAMAGE_TYPE_0, DAMAGE_TYPE_6);
        int nDamage;
        if (pXSector->data)
            nDamage = ClipRange(pXSector->data, 0, 1000);
        else
            nDamage = 1000;
        actDamageSprite(pSprite->index, pSprite, nDamageType, scale(4, nDamage, 120) << 4);
    }
    if (tileGetSurfType(nSector + 0x4000) == kSurfLava)
    {
        actDamageSprite(pSprite->index, pSprite, DAMAGE_TYPE_1, 16);
        sfxPlay3DSound(pSprite, 352, 5, 2);
    }
}

void ProcessTouchObjects(spritetype *pSprite, int nXSprite)
{
    int nSprite = pSprite->index;
    XSPRITE *pXSprite = &xsprite[nXSprite];
    SPRITEHIT *pSpriteHit = &gSpriteHit[nXSprite];
    PLAYER *pPlayer = NULL;
    if (IsPlayerSprite(pSprite))
        pPlayer = &gPlayer[pSprite->type-kDudePlayer1];
    int nHitSprite = pSpriteHit->ceilhit & 0x3fff;
    switch (pSpriteHit->ceilhit&0xc000)
    {
    case 0x8000:
        break;
    case 0xc000:
        if (sprite[nHitSprite].extra > 0)
        {
            spritetype *pSprite2 = &sprite[nHitSprite];
            XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
            if ((pSprite2->statnum == kStatThing || pSprite2->statnum == kStatDude) && (xvel[nSprite] != 0 || yvel[nSprite] != 0 || zvel[nSprite] != 0))
            {
                if (pSprite2->statnum == kStatThing)
                {
                    int nType = pSprite2->type-kThingBase;
                    const THINGINFO *pThingInfo = &thingInfo[nType];
                    if (pThingInfo->flags&1)

                        pSprite2->flags |= 1;
                    if (pThingInfo->flags&2)

                        pSprite2->flags |= 4;
                    // Inlined ?
                    xvel[pSprite2->index] += mulscale(4, pSprite2->x-sprite[nSprite].x, 2);
                    yvel[pSprite2->index] += mulscale(4, pSprite2->y-sprite[nSprite].y, 2);
                }
                else
                {

                    pSprite2->flags |= 5;
                    xvel[pSprite2->index] += mulscale(4, pSprite2->x-sprite[nSprite].x, 2);
                    yvel[pSprite2->index] += mulscale(4, pSprite2->y-sprite[nSprite].y, 2);
                    
                    // by NoOne: add size shroom abilities
                    if ((IsPlayerSprite(pSprite) && isShrinked(pSprite)) || (IsPlayerSprite(pSprite2) && isGrown(pSprite2))) {

                        int mass1 = dudeInfo[pSprite2->type - kDudeBase].mass;
                        int mass2 = dudeInfo[pSprite->type - kDudeBase].mass;
                        switch (pSprite->type) {
                            case kDudeModernCustom:
                            case kDudeModernCustomBurning:
                                mass2 = getSpriteMassBySize(pSprite);
                                break;
                        }
                        if (mass1 > mass2) {
                            int dmg = abs((mass1 - mass2) * (pSprite2->clipdist - pSprite->clipdist));
                            if (IsDudeSprite(pSprite2)) {
                                if (dmg > 0)
                                    actDamageSprite(pSprite2->xvel, pSprite, (Chance(0x2000)) ? DAMAGE_TYPE_0 : (Chance(0x4000)) ? DAMAGE_TYPE_3 : DAMAGE_TYPE_2, dmg);

                                if (Chance(0x0200))
                                    actKickObject(pSprite2, pSprite);
                            }
                        }
                    }
                    
                    if (!IsPlayerSprite(pSprite) || gPlayer[pSprite->type - kDudePlayer1].godMode == 0) {
                        switch (pSprite2->type) {
                            case kDudeTchernobog:
                                actDamageSprite(pSprite2->index, pSprite, DAMAGE_TYPE_3, pXSprite->health << 2);
                                break;
                            case kDudeModernCustom:
                            case kDudeModernCustomBurning:
                                int dmg = 0;
                                if (!IsDudeSprite(pSprite) || (dmg = ClipLow((getSpriteMassBySize(pSprite2) - getSpriteMassBySize(pSprite)) >> 1, 0)) == 0)
                                    break;

                                if (!IsPlayerSprite(pSprite)) {
                                    actDamageSprite(pSprite2->index, pSprite, DAMAGE_TYPE_0, dmg);
                                    if (xspriRangeIsFine(pSprite->extra) && !isActive(pSprite->index))
                                        aiActivateDude(pSprite, &xsprite[pSprite->extra]);
                                }
                                else if (powerupCheck(&gPlayer[pSprite->type - kDudePlayer1], kPwUpJumpBoots) > 0) actDamageSprite(pSprite2->index, pSprite, DAMAGE_TYPE_3, dmg);
                                else actDamageSprite(pSprite2->index, pSprite, DAMAGE_TYPE_0, dmg);
                                break;

                        }
                            
                    }
                }
            }
            
            if (pSprite2->type == kTrapSawCircular) {
                if (!pXSprite2->state) actDamageSprite(nSprite, pSprite, DAMAGE_TYPE_2, 1);
                else {
                    pXSprite2->data1 = 1;
                    pXSprite2->data2 = ClipHigh(pXSprite2->data2+8, 600);
                    actDamageSprite(nSprite, pSprite, DAMAGE_TYPE_2, 16);
                }
            }

        }
        break;
    }
    nHitSprite = pSpriteHit->hit & 0x3fff;
    switch (pSpriteHit->hit&0xc000)
    {
    case 0x8000:
        break;
    case 0xc000:
        if (sprite[nHitSprite].extra > 0)
        {
            spritetype *pSprite2 = &sprite[nHitSprite];
            //XSPRITE *pXSprite2 = &Xsprite[pSprite2->extra];
            
            // by NoOne: add size shroom abilities
            if ((IsPlayerSprite(pSprite2) && isShrinked(pSprite2)) || (IsPlayerSprite(pSprite) && isGrown(pSprite))) {
                if (xvel[pSprite->xvel] != 0 && IsDudeSprite(pSprite2)) {
                    int mass1 = dudeInfo[pSprite->type - kDudeBase].mass;
                    int mass2 = dudeInfo[pSprite2->type - kDudeBase].mass;
                    switch (pSprite2->type) {
                        case kDudeModernCustom:
                        case kDudeModernCustomBurning:
                            mass2 = getSpriteMassBySize(pSprite2);
                            break;
                    }
                    if (mass1 > mass2) {
                        actKickObject(pSprite, pSprite2);
                        sfxPlay3DSound(pSprite, 357, -1, 1);
                        int dmg = (mass1 - mass2) + abs(xvel[pSprite->xvel] >> 16);
                        if (dmg > 0)
                            actDamageSprite(nSprite, pSprite2, (Chance(0x2000)) ? DAMAGE_TYPE_0 : DAMAGE_TYPE_2, dmg);
                    }
                }
            }

            /*switch (pSprite->type) {
                case kDudeModernCustom:
                case kDudeModernCustomBurning:
                {
                    if (IsDudeSprite(pSprite2) && !IsPlayerSprite(pSprite2)) {
                        int mass1 = getSpriteMassBySize(pSprite);
                        int mass2 = getSpriteMassBySize(pSprite2);

                        if ((mass1 - mass2) >= mass2) {
                            if ((pXSprite->target == pSprite2->xvel && !dudeIsMelee(pXSprite) && Chance(0x0500)) || pXSprite->target != pSprite2->xvel) 
                                actKickObject(pSprite, pSprite2);
                            if (pSprite2->extra >= 0 && !isActive(pSprite2->xvel))
                                aiActivateDude(pSprite2, &xsprite[pSprite2->extra]);
                        }
                    }
                    break;
                }
            }*/
            
            switch (pSprite2->type) {
                case kThingKickablePail:
                    actKickObject(pSprite, pSprite2);
                    break;
                case kThingZombieHead:
                    sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, 357, pSprite->sectnum);
                    actKickObject(pSprite, pSprite2);
                    actDamageSprite(-1, pSprite2, DAMAGE_TYPE_0, 80);
                    break;
                case kDudeBurningInnocent:
                case kDudeBurningCultist:
                case kDudeBurningZombieAxe:
                case kDudeBurningZombieButcher:
                    // This does not make sense
                    pXSprite->burnTime = ClipLow(pXSprite->burnTime-4, 0);
                    actDamageSprite(actOwnerIdToSpriteId(pXSprite->burnSource), pSprite, DAMAGE_TYPE_1, 8);
                    break;
            }
        }
        break;
    }
    nHitSprite = pSpriteHit->florhit & 0x3fff;
    switch (pSpriteHit->florhit & 0xc000) {
    case 0x8000:
        break;
    case 0x4000:
        actTouchFloor(pSprite, nHitSprite);
        break;
    case 0xc000:
        if (sprite[nHitSprite].extra > 0)
        {
            spritetype *pSprite2 = &sprite[nHitSprite];
            XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
            
            // by NoOne: add size shroom abilities
            if ((IsPlayerSprite(pSprite2) && isShrinked(pSprite2)) || (IsPlayerSprite(pSprite) && isGrown(pSprite))) {
                
                int mass1 = dudeInfo[pSprite->type - kDudeBase].mass;
                int mass2 = dudeInfo[pSprite2->type - kDudeBase].mass;
                switch (pSprite2->type) {
                    case kDudeModernCustom:
                    case kDudeModernCustomBurning:
                        mass2 = getSpriteMassBySize(pSprite2);
                        break;
                }
                if (mass1 > mass2 && IsDudeSprite(pSprite2)) {
                    if ((IsPlayerSprite(pSprite2) && Chance(0x500)) || !IsPlayerSprite(pSprite2))
                        actKickObject(pSprite, pSprite2);

                    int dmg = (mass1 - mass2) + pSprite->clipdist;
                    if (dmg > 0)
                        actDamageSprite(nSprite, pSprite2, (Chance(0x2000)) ? DAMAGE_TYPE_0 : DAMAGE_TYPE_2, dmg);
                }
            }

            /*switch (pSprite->type) {
                case kDudeModernCustom:
                case kDudeModernCustomBurning:
                {
                    if (IsDudeSprite(pSprite2) && !IsPlayerSprite(pSprite2)) {
                        int mass1 = getSpriteMassBySize(pSprite);
                        int mass2 = getSpriteMassBySize(pSprite2);

                        if ((mass1 - mass2) >= mass2) {
                            if (Chance((pXSprite->target == pSprite2->xvel) ? 0x0500 : 0x1000)) actKickObject(pSprite, pSprite2);
                            if (pSprite2->extra >= 0 && !isActive(pSprite2->xvel))
                                aiActivateDude(pSprite2, &xsprite[pSprite2->extra]);
                        }
                    }
                    break;
                }
            }*/

            
            switch (pSprite2->type) {
            case kThingKickablePail:
                if (pPlayer) {
                    if (pPlayer->kickPower > gFrameClock) return;
                    pPlayer->kickPower = (int)gFrameClock+60;
                }
                actKickObject(pSprite, pSprite2);
                sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, 357, pSprite->sectnum);
                sfxPlay3DSound(pSprite, 374, 0, 0);
                break;
            case kThingZombieHead:
                if (pPlayer) {
                    if (pPlayer->kickPower > gFrameClock) return;
                    pPlayer->kickPower = (int)gFrameClock+60;
                }
                actKickObject(pSprite, pSprite2);
                sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, 357, pSprite->sectnum);
                actDamageSprite(-1, pSprite2, DAMAGE_TYPE_0, 80);
                break;
            case kTrapSawCircular:
                if (!pXSprite2->state) actDamageSprite(nSprite, pSprite, DAMAGE_TYPE_2, 1);
                else {
                    pXSprite2->data1 = 1;
                    pXSprite2->data2 = ClipHigh(pXSprite2->data2+8, 600);
                    actDamageSprite(nSprite, pSprite, DAMAGE_TYPE_2, 16);
                }
                break;
            case kDudeCultistTommy:
            case kDudeCultistShotgun:
            case kDudeZombieAxeNormal:
            case kDudeZombieButcher:
            case kDudeZombieAxeBuried:
            case kDudeGargoyleFlesh:
            case kDudeGargoyleStone:
            case kDudePhantasm:
            case kDudeHellHound:
            case kDudeHand:
            case kDudeSpiderBrown:
            case kDudeSpiderRed:
            case kDudeSpiderBlack:
            case kDudeGillBeast:
            case kDudeBat:
            case kDudeRat:
            case kDudePodGreen:
            case kDudeTentacleGreen:
            case kDudePodFire:
            case kDudeTentacleFire:
            case kDudePodMother:
            case kDudeTentacleMother:
            case kDudeCerberusTwoHead:
            case kDudeCerberusOneHead:
            case kDudeTchernobog:
            case kDudePlayer1:
            case kDudePlayer2:
            case kDudePlayer3:
            case kDudePlayer4:
            case kDudePlayer5:
            case kDudePlayer6:
            case kDudePlayer7:
            case kDudePlayer8:
                if (pPlayer && !isShrinked(pSprite))
                    actDamageSprite(nSprite, pSprite2,DAMAGE_TYPE_2, 8);
                break;
            }
        }
        break;
    }

    // by NoOne: add more trigger statements for Touch flag
    if (gModernMap && IsDudeSprite(pSprite)) {
        
        // Touch sprites
        int nHSprite = -1;
        if ((gSpriteHit[nXSprite].hit & 0xc000) == 0xc000)
            nHSprite = gSpriteHit[nXSprite].hit & 0x3fff;
        else if ((gSpriteHit[nXSprite].florhit & 0xc000) == 0xc000)
            nHSprite = gSpriteHit[nXSprite].florhit & 0x3fff;
        else if ((gSpriteHit[nXSprite].ceilhit & 0xc000) == 0xc000)
            nHSprite = gSpriteHit[nXSprite].ceilhit & 0x3fff;

        if (spriRangeIsFine(nHSprite) && xspriRangeIsFine(sprite[nHSprite].extra)) {
            XSPRITE* pXHSprite = &xsprite[sprite[nHSprite].extra];
            if (pXHSprite->Touch && !pXHSprite->isTriggered && (!pXHSprite->DudeLockout || IsPlayerSprite(pSprite)))
                trTriggerSprite(nHSprite, pXHSprite, kCmdSpriteTouch, nSprite);
        }

        // Touch walls
        int nHWall = -1;
        if ((gSpriteHit[nXSprite].hit & 0xc000) == 0x8000) {
            nHWall = gSpriteHit[nXSprite].hit & 0x3fff;
            if (wallRangeIsFine(nHWall) && xwallRangeIsFine(wall[nHWall].extra)) {
                XWALL* pXHWall = &xwall[wall[nHWall].extra];
                if (pXHWall->triggerTouch && !pXHWall->isTriggered && (!pXHWall->dudeLockout || IsPlayerSprite(pSprite)))
                    trTriggerWall(nHWall, pXHWall, kCmdWallTouch, nSprite);
            }
        }

        // enough to reset gSpriteHit values
        if (nHWall != -1 || nHSprite != -1) xvel[nSprite] += 5;

    }
}

void actAirDrag(spritetype *pSprite, int a2)
{
    int vbp = 0;
    int v4 = 0;
    int nSector = pSprite->sectnum;
    dassert(nSector >= 0 && nSector < kMaxSectors);
    sectortype *pSector = &sector[nSector];
    int nXSector = pSector->extra;
    if (nXSector > 0)
    {
        dassert(nXSector < kMaxXSectors);
        XSECTOR *pXSector = &xsector[nXSector];
        if (pXSector->windVel && (pXSector->windAlways || pXSector->busy))
        {
            int vcx = pXSector->windVel<<12;
            if (!pXSector->windAlways && pXSector->busy)
                vcx = mulscale16(vcx, pXSector->busy);
            vbp = mulscale30(vcx, Cos(pXSector->windAng));
            v4 = mulscale30(vcx, Sin(pXSector->windAng));
        }
    }
    xvel[pSprite->index] += mulscale16(vbp-xvel[pSprite->index], a2);
    yvel[pSprite->index] += mulscale16(v4-yvel[pSprite->index], a2);
    zvel[pSprite->index] -= mulscale16(zvel[pSprite->index], a2);
}

int MoveThing(spritetype *pSprite)
{
    int nXSprite = pSprite->extra;
    dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pSprite->index;
    int v8 = 0;
    dassert(pSprite->type >= kThingBase && pSprite->type < kThingMax);
    const THINGINFO *pThingInfo = &thingInfo[pSprite->type-kThingBase];
    int nSector = pSprite->sectnum;
    dassert(nSector >= 0 && nSector < kMaxSectors);
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    if (xvel[nSprite] || yvel[nSprite])
    {
        short bakCstat = pSprite->cstat;
        pSprite->cstat &= ~257;
        v8 = gSpriteHit[nXSprite].hit = ClipMove((int*)&pSprite->x, (int*)&pSprite->y, (int*)&pSprite->z, &nSector, xvel[nSprite]>>12, yvel[nSprite]>>12, pSprite->clipdist<<2, (pSprite->z-top)/4, (bottom-pSprite->z)/4, CLIPMASK0);
        pSprite->cstat = bakCstat;
        dassert(nSector >= 0);
        if (pSprite->sectnum != nSector)
        {
            dassert(nSector >= 0 && nSector < kMaxSectors);
            ChangeSpriteSect(nSprite, nSector);
        }
        if ((gSpriteHit[nXSprite].hit&0xc000) == 0x8000) {
            int nHitWall = gSpriteHit[nXSprite].hit&0x3fff;
            actWallBounceVector((int*)&xvel[nSprite], (int*)&yvel[nSprite], nHitWall, pThingInfo->elastic);
            switch (pSprite->type) {
                case kThingZombieHead:
                    sfxPlay3DSound(pSprite, 607, 0, 0);
                    actDamageSprite(-1, pSprite, DAMAGE_TYPE_0, 80);
                    break;
                case kThingKickablePail:
                    sfxPlay3DSound(pSprite, 374, 0, 0);
                    break;
            }
        }
    }
    else
    {
        dassert(nSector >= 0 && nSector < kMaxSectors);
        FindSector(pSprite->x, pSprite->y, pSprite->z, &nSector);
    }
    if (zvel[nSprite])
        pSprite->z += zvel[nSprite]>>8;
    int ceilZ, ceilHit, floorZ, floorHit;
    GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist<<2, CLIPMASK0);
    GetSpriteExtents(pSprite, &top, &bottom);
    if ((pSprite->flags & 2) && bottom < floorZ)
    {
        pSprite->z += 455;
        zvel[nSprite] += 58254;
        if (pSprite->type == kThingZombieHead)
        {
            spritetype *pFX = gFX.fxSpawn(FX_27, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
            if (pFX)
            {
                int v34 = ((int)gFrameClock*3)&2047;
                int v30 = ((int)gFrameClock*5)&2047;
                int vbx = ((int)gFrameClock*11)&2047;
                int v2c = 0x44444;
                int v28 = 0;
                int v24 = 0;
                RotateVector(&v2c,&v28,vbx);
                RotateVector(&v2c,&v24,v30);
                RotateVector(&v28,&v24,v34);
                xvel[pFX->index] = xvel[pSprite->index]+v2c;
                yvel[pFX->index] = yvel[pSprite->index]+v28;
                zvel[pFX->index] = zvel[pSprite->index]+v24;
            }
        }
    }
    if (CheckLink(pSprite))
        GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist<<2, CLIPMASK0);
    GetSpriteExtents(pSprite, &top, &bottom);
    if (bottom >= floorZ)
    {
        actTouchFloor(pSprite, pSprite->sectnum);
        gSpriteHit[nXSprite].florhit = floorHit;
        pSprite->z += floorZ-bottom;
        int v20 = zvel[nSprite]-velFloor[pSprite->sectnum];
        if (v20 > 0)
        {

            pSprite->flags |= 4;
            int vax = actFloorBounceVector((int*)&xvel[nSprite], (int*)&yvel[nSprite], (int*)&v20, pSprite->sectnum, pThingInfo->elastic);
            int nDamage = mulscale(vax, vax, 30)-pThingInfo->dmgResist;
            if (nDamage > 0)
                actDamageSprite(nSprite, pSprite, DAMAGE_TYPE_0, nDamage);
            zvel[nSprite] = v20;
            if (velFloor[pSprite->sectnum] == 0 && klabs(zvel[nSprite]) < 0x10000)
            {
                zvel[nSprite] = 0;

                pSprite->flags &= ~4;
            }
            
            switch (pSprite->type) {
                case kThingNapalmBall:
                    if (zvel[nSprite] == 0 || Chance(0xA000)) sub_2AA94(pSprite, pXSprite);
                    break;
                case kThingZombieHead:
                    if (klabs(zvel[nSprite]) > 0x80000) {
                        sfxPlay3DSound(pSprite, 607, 0, 0);
                        actDamageSprite(-1, pSprite, DAMAGE_TYPE_0, 80);
                    }
                    break;
                case kThingKickablePail:
                    if (klabs(zvel[nSprite]) > 0x80000)
                        sfxPlay3DSound(pSprite, 374, 0, 0);
                    break;
            }

            v8 = 0x4000|nSector;
        }
        else if (zvel[nSprite] == 0)

            pSprite->flags &= ~4;
    }
    else
    {
        gSpriteHit[nXSprite].florhit = 0;

        if (pSprite->flags&2)
            pSprite->flags |= 4;
    }
    if (top <= ceilZ)
    {
        gSpriteHit[nXSprite].ceilhit = ceilHit;
        pSprite->z += ClipLow(ceilZ-top, 0);
        if (zvel[nSprite] < 0)
        {
            xvel[nSprite] = mulscale16(xvel[nSprite], 0xc000);
            yvel[nSprite] = mulscale16(yvel[nSprite], 0xc000);
            zvel[nSprite] = mulscale16(-zvel[nSprite], 0x4000);
            switch (pSprite->type) {
                case kThingZombieHead:
                    if (klabs(zvel[nSprite]) > 0x80000) {
                        sfxPlay3DSound(pSprite, 607, 0, 0);
                        actDamageSprite(-1, pSprite, DAMAGE_TYPE_0, 80);
                    }
                    break;
                case kThingKickablePail:
                    if (klabs(zvel[nSprite]) > 0x80000)
                        sfxPlay3DSound(pSprite, 374, 0, 0);
                    break;
            }
        }
    }
    else
        gSpriteHit[nXSprite].ceilhit = 0;
    if (bottom >= floorZ)
    {
        int nVel = approxDist(xvel[nSprite], yvel[nSprite]);
        int nVelClipped = ClipHigh(nVel, 0x11111);
        if ((floorHit & 0xc000) == 0xc000)
        {
            int nHitSprite = floorHit & 0x3fff;
            if ((sprite[nHitSprite].cstat & 0x30) == 0)
            {
                xvel[nSprite] += mulscale(4, pSprite->x - sprite[nHitSprite].x, 2);
                yvel[nSprite] += mulscale(4, pSprite->y - sprite[nHitSprite].y, 2);
                v8 = gSpriteHit[nXSprite].hit;
            }
        }
        if (nVel > 0)
        {
            int t = divscale16(nVelClipped, nVel);
            xvel[nSprite] -= mulscale16(t, xvel[nSprite]);
            yvel[nSprite] -= mulscale16(t, yvel[nSprite]);
        }
    }
    if (xvel[nSprite] || yvel[nSprite])
        pSprite->ang = getangle(xvel[nSprite], yvel[nSprite]);
    return v8;
}

void MoveDude(spritetype *pSprite)
{
    int nXSprite = pSprite->extra;
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pSprite->index;
    PLAYER *pPlayer = NULL;
    if (IsPlayerSprite(pSprite))
        pPlayer = &gPlayer[pSprite->type-kDudePlayer1];
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        consoleSysMsg("pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    int bz = (bottom-pSprite->z)/4;
    int tz = (pSprite->z-top)/4;
    int wd = pSprite->clipdist<<2;
    int nSector = pSprite->sectnum;
    dassert(nSector >= 0 && nSector < kMaxSectors);
    if (xvel[nSprite] || yvel[nSprite])
    {
        if (pPlayer && gNoClip)
        {
            pSprite->x += xvel[nSprite]>>12;
            pSprite->y += yvel[nSprite]>>12;
            if (!FindSector(pSprite->x, pSprite->y, &nSector))
                nSector = pSprite->sectnum;
        }
        else
        {
            short bakCstat = pSprite->cstat;
            pSprite->cstat &= ~257;
            gSpriteHit[nXSprite].hit = ClipMove((int*)&pSprite->x, (int*)&pSprite->y, (int*)&pSprite->z, &nSector, xvel[nSprite]>>12, yvel[nSprite]>>12, wd, tz, bz, CLIPMASK0);
            if (nSector == -1)
            {
                nSector = pSprite->sectnum;
                if (pSprite->statnum == kStatDude || pSprite->statnum == kStatThing)
                    actDamageSprite(pSprite->index, pSprite, DAMAGE_TYPE_0, 1000<<4);
            }

            if (sector[nSector].type >= kSectorPath && sector[nSector].type <= kSectorRotate)
            {
                short nSector2 = nSector;
                if (pushmove_old(&pSprite->x, &pSprite->y, &pSprite->z, &nSector2, wd, tz, bz, CLIPMASK0) == -1)
                    actDamageSprite(nSprite, pSprite, DAMAGE_TYPE_0, 1000 << 4);
                if (nSector2 != -1)
                    nSector = nSector2;
            }
            dassert(nSector >= 0);
            pSprite->cstat = bakCstat;
        }
        switch (gSpriteHit[nXSprite].hit&0xc000)
        {
        case 0xc000:
        {
            int nHitSprite = gSpriteHit[nXSprite].hit&0x3fff;
            spritetype *pHitSprite = &sprite[nHitSprite];
            XSPRITE *pHitXSprite = NULL;
            // Should be pHitSprite here
            if (pSprite->extra > 0)
                pHitXSprite = &xsprite[pHitSprite->extra];
            int nOwner = actSpriteOwnerToSpriteId(pHitSprite);

            if (pHitSprite->statnum == kStatProjectile && !(pHitSprite->flags&32) && pSprite->index != nOwner)
            {
                HITINFO hitInfo = gHitInfo;
                gHitInfo.hitsprite = nSprite;
                actImpactMissile(pHitSprite, 3);
                gHitInfo = hitInfo;
            }

            if (!gModernMap && pHitXSprite && pHitXSprite->Touch && !pHitXSprite->state && !pHitXSprite->isTriggered)
                trTriggerSprite(nHitSprite, pHitXSprite, kCmdSpriteTouch, nSprite);

            if (pDudeInfo->lockOut && pHitXSprite && pHitXSprite->Push && !pHitXSprite->key && !pHitXSprite->DudeLockout && !pHitXSprite->state && !pHitXSprite->busy && !pPlayer)
                trTriggerSprite(nHitSprite, pHitXSprite, kCmdSpritePush, nSprite);

            break;
        }
        case 0x8000:
        {
            int nHitWall = gSpriteHit[nXSprite].hit&0x3fff;
            walltype *pHitWall = &wall[nHitWall];
            XWALL *pHitXWall = NULL;
            if (pHitWall->extra > 0)
                pHitXWall = &xwall[pHitWall->extra];
            if (pDudeInfo->lockOut && pHitXWall && pHitXWall->triggerPush && !pHitXWall->key && !pHitXWall->dudeLockout && !pHitXWall->state && !pHitXWall->busy && !pPlayer)
                trTriggerWall(nHitWall, pHitXWall, kCmdWallPush, nSprite);
            if (pHitWall->nextsector != -1)
            {
                sectortype *pHitSector = &sector[pHitWall->nextsector];
                XSECTOR *pHitXSector = NULL;
                if (pHitSector->extra > 0)
                    pHitXSector = &xsector[pHitSector->extra];
                if (pDudeInfo->lockOut && pHitXSector && pHitXSector->Wallpush && !pHitXSector->Key && !pHitXSector->dudeLockout && !pHitXSector->state && !pHitXSector->busy && !pPlayer)
                    trTriggerSector(pHitWall->nextsector, pHitXSector, kCmdSectorPush, nSprite);
                if (top < pHitSector->ceilingz || bottom > pHitSector->floorz)
                {
                    // ???
                }
            }
            actWallBounceVector((int*)&xvel[nSprite], (int*)&yvel[nSprite], nHitWall, 0);
            break;
        }
        }
    }
    else
    {
        dassert(nSector >= 0 && nSector < kMaxSectors);
        FindSector(pSprite->x, pSprite->y, pSprite->z, &nSector);
    }
    if (pSprite->sectnum != nSector)
    {
        dassert(nSector >= 0 && nSector < kMaxSectors);
        XSECTOR *pXSector;
        int nXSector = sector[pSprite->sectnum].extra;
        if (nXSector > 0)
            pXSector = &xsector[nXSector];
        else
            pXSector = NULL;
        if (pXSector && pXSector->Exit && (pPlayer || !pXSector->dudeLockout))
            trTriggerSector(pSprite->sectnum, pXSector, kCmdSectorExit, nSprite);
        ChangeSpriteSect(nSprite, nSector);
        
        nXSector = sector[nSector].extra;
        pXSector = (nXSector > 0) ? &xsector[nXSector] : NULL;
        if (pXSector && pXSector->Enter && (pPlayer || !pXSector->dudeLockout)) {

            if (sector[nSector].type == kSectorTeleport)
                pXSector->data = pPlayer ? nSprite : -1;
            trTriggerSector(nSector, pXSector, kCmdSectorEnter, nSprite);
        }

        nSector = pSprite->sectnum;
    }
    char bUnderwater = 0;
    char bDepth = 0;
    if (sector[nSector].extra > 0)
    {
        XSECTOR *pXSector = &xsector[sector[nSector].extra];
        if (pXSector->Underwater)
            bUnderwater = 1;
        if (pXSector->Depth)
            bDepth = 1;
    }
    int nUpperLink = gUpperLink[nSector];
    int nLowerLink = gLowerLink[nSector];
    if (nUpperLink >= 0 && (sprite[nUpperLink].type == kMarkerUpWater || sprite[nUpperLink].type == kMarkerUpGoo))
        bDepth = 1;
    if (nLowerLink >= 0 && (sprite[nLowerLink].type == kMarkerLowWater || sprite[nLowerLink].type == kMarkerLowGoo))
        bDepth = 1;
    if (pPlayer)
        wd += 16;
    if (zvel[nSprite])
        pSprite->z += zvel[nSprite]>>8;
    int ceilZ, ceilHit, floorZ, floorHit;
    GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, wd, CLIPMASK0, PARALLAXCLIP_CEILING|PARALLAXCLIP_FLOOR);
    GetSpriteExtents(pSprite, &top, &bottom);

    if (pSprite->flags & 2)
    {
        int vc = 58254;
        if (bDepth)
        {
            if (bUnderwater)
            {
                int cz = getceilzofslope(nSector, pSprite->x, pSprite->y);
                if (cz > top)
                    vc += ((bottom-cz)*-80099) / (bottom-top);
                else
                    vc = 0;
            }
            else
            {
                int fz = getflorzofslope(nSector, pSprite->x, pSprite->y);
                if (fz < bottom)
                    vc += ((bottom-fz)*-80099) / (bottom-top);
            }
        }
        else
        {
            if (bUnderwater)
                vc = 0;
            else if (bottom >= floorZ)
                vc =  0;
        }
        if (vc)
        {
            pSprite->z += ((vc*4)/2)>>8;
            zvel[nSprite] += vc;
        }
    }
    if (pPlayer && zvel[nSprite] > 0x155555 && !pPlayer->fallScream && pXSprite->height > 0)
    {
        pPlayer->fallScream = 1;
        sfxPlay3DSound(pSprite, 719, 0, 0);
    }
    vec3_t const oldpos = pSprite->pos;
    int nLink = CheckLink(pSprite);
    if (nLink)
    {
        GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, wd, CLIPMASK0, PARALLAXCLIP_CEILING|PARALLAXCLIP_FLOOR);
        if (pPlayer)
        {
            if (bVanilla)
                playerResetInertia(pPlayer);
            else
                playerCorrectInertia(pPlayer, &oldpos);
        }
        switch (nLink) {
        case kMarkerLowStack:
            if (pPlayer == gView)
                SetBitString(gotpic, sector[pSprite->sectnum].floorpicnum);
            break;
        case kMarkerUpStack:
            if (pPlayer == gView)
                SetBitString(gotpic, sector[pSprite->sectnum].ceilingpicnum);
            break;
        case kMarkerLowWater:
        case kMarkerLowGoo:
            pXSprite->medium = kMediumNormal;
            if (pPlayer) {
                pPlayer->posture = 0;
                pPlayer->bubbleTime = 0;
                if (!pPlayer->cantJump && pPlayer->input.buttonFlags.jump) {
                    zvel[nSprite] = -0x6aaaa;
                    pPlayer->cantJump = 1;
                }
                sfxPlay3DSound(pSprite, 721, -1, 0);
            } else {
                switch (pSprite->type) {
                    case kDudeCultistTommy:
                    case kDudeCultistShotgun:
                        aiNewState(pSprite, pXSprite, &cultistGoto);
                        break;
                    case kDudeGillBeast:
                        aiNewState(pSprite, pXSprite, &gillBeastGoto);
                        pSprite->flags |= 6;
                        break;
                    case kDudeBoneEel:
                        actKillDude(pSprite->index, pSprite, DAMAGE_TYPE_0, 1000<<4);
                        break;
                }
            }
            break;
        // By NoOne: part of "change of global view palette for stacks" feature
        case kMarkerUpWater:
        case kMarkerUpGoo:
        {
            pXSprite->medium = (nLink == kMarkerUpGoo ? kMediumGoo : kMediumWater);

            if (pPlayer)
            {
                // look for palette in data2 of marker. If value <= 0, use default ones.
                pPlayer->nWaterPal = 0;
                int nXUpper = sprite[gUpperLink[nSector]].extra;
                if (nXUpper >= 0)
                    pPlayer->nWaterPal = xsprite[nXUpper].data2;

                pPlayer->posture = 1;
                pXSprite->burnTime = 0;
                pPlayer->bubbleTime = klabs(zvel[nSprite]) >> 12;
                evPost(nSprite, 3, 0, kCallbackPlayerBubble);
                sfxPlay3DSound(pSprite, 720, -1, 0);
            }
            else
            {
                switch (pSprite->type) {
                case kDudeCultistTommy:
                case kDudeCultistShotgun:
                    pXSprite->burnTime = 0;
                    evPost(nSprite, 3, 0, kCallbackEnemeyBubble);
                    sfxPlay3DSound(pSprite, 720, -1, 0);
                    aiNewState(pSprite, pXSprite, &cultistSwimGoto);
                    break;
                case kDudeBurningCultist:
                {
                    // There is no difference between water and goo except following chance:
                    if (Chance(nLink == kMarkerUpGoo ? 0x400 : 0xa00))
                    {
                        pSprite->type = kDudeCultistTommy;
                        pXSprite->burnTime = 0;
                        evPost(nSprite, 3, 0, kCallbackEnemeyBubble);
                        sfxPlay3DSound(pSprite, 720, -1, 0);
                        aiNewState(pSprite, pXSprite, &cultistSwimGoto);
                    }
                    else
                    {
                        pSprite->type = kDudeCultistShotgun;
                        pXSprite->burnTime = 0;
                        evPost(nSprite, 3, 0, kCallbackEnemeyBubble);
                        sfxPlay3DSound(pSprite, 720, -1, 0);
                        aiNewState(pSprite, pXSprite, &cultistSwimGoto);
                    }
                    break;
                }
                case kDudeZombieAxeNormal:
                    pXSprite->burnTime = 0;
                    evPost(nSprite, 3, 0, kCallbackEnemeyBubble);
                    sfxPlay3DSound(pSprite, 720, -1, 0);
                    aiNewState(pSprite, pXSprite, &zombieAGoto);
                    break;
                case kDudeZombieButcher:
                    pXSprite->burnTime = 0;
                    evPost(nSprite, 3, 0, kCallbackEnemeyBubble);
                    sfxPlay3DSound(pSprite, 720, -1, 0);
                    aiNewState(pSprite, pXSprite, &zombieFGoto);
                    break;
                case kDudeGillBeast:
                    pXSprite->burnTime = 0;
                    evPost(nSprite, 3, 0, kCallbackEnemeyBubble);
                    sfxPlay3DSound(pSprite, 720, -1, 0);
                    aiNewState(pSprite, pXSprite, &gillBeastSwimGoto);

                    pSprite->flags &= ~6;
                    break;
                case kDudeGargoyleFlesh:
                case kDudeHellHound:
                case kDudeSpiderBrown:
                case kDudeSpiderRed:
                case kDudeSpiderBlack:
                case kDudeBat:
                case kDudeRat:
                case kDudeBurningInnocent:
                    actKillDude(pSprite->index, pSprite, DAMAGE_TYPE_0, 1000 << 4);
                    break;
                case kDudeModernCustom:
                    evPost(nSprite, 3, 0, kCallbackEnemeyBubble);
                    if (!canSwim(pSprite)) actKillDude(pSprite->index, pSprite, DAMAGE_TYPE_0, 1000 << 4);
                    break;
                }
            }
            break;
        }
        /*case 13:
            pXSprite->medium = kMediumGoo;
            if (pPlayer)
            {
                pPlayer->changeTargetKin = 1;
                pXSprite->burnTime = 0;
                pPlayer->bubbleTime = klabs(zvel[nSprite])>>12;
                evPost(nSprite, 3, 0, kCallbackPlayerBubble);
                sfxPlay3DSound(pSprite, 720, -1, 0);
            }
            else
            {
                switch (pSprite->type)
                {
                case kDudeCultistTommy:
                case kDudeCultistShotgun:
                    pXSprite->burnTime = 0;
                    evPost(nSprite, 3, 0, kCallbackEnemeyBubble);
                    sfxPlay3DSound(pSprite, 720, -1, 0);
                    aiNewState(pSprite, pXSprite, &cultistSwimGoto);
                    break;
                case kDudeBurningCultist:
                    if (Chance(0x400))
                    {
                        pSprite->type = kDudeCultistTommy;
                        pXSprite->burnTime = 0;
                        evPost(nSprite, 3, 0, kCallbackEnemeyBubble);
                        sfxPlay3DSound(pSprite, 720, -1, 0);
                        aiNewState(pSprite, pXSprite, &cultistSwimGoto);
                    }
                    else
                    {
                        pSprite->type = kDudeCultistShotgun;
                        pXSprite->burnTime = 0;
                        evPost(nSprite, 3, 0, kCallbackEnemeyBubble);
                        sfxPlay3DSound(pSprite, 720, -1, 0);
                        aiNewState(pSprite, pXSprite, &cultistSwimGoto);
                    }
                    break;
                case kDudeZombieAxeNormal:
                    pXSprite->burnTime = 0;
                    evPost(nSprite, 3, 0, kCallbackEnemeyBubble);
                    sfxPlay3DSound(pSprite, 720, -1, 0);
                    aiNewState(pSprite, pXSprite, &zombieAGoto);
                    break;
                case kDudeZombieButcher:
                    pXSprite->burnTime = 0;
                    evPost(nSprite, 3, 0, kCallbackEnemeyBubble);
                    sfxPlay3DSound(pSprite, 720, -1, 0);
                    aiNewState(pSprite, pXSprite, &zombieFGoto);
                    break;
                case kDudeGillBeast:
                    pXSprite->burnTime = 0;
                    evPost(nSprite, 3, 0, kCallbackEnemeyBubble);
                    sfxPlay3DSound(pSprite, 720, -1, 0);
                    aiNewState(pSprite, pXSprite, &gillBeastSwimGoto);
                    pSprite->flags &= ~6;
                    break;
                case kDudeGargoyleFlesh:
                case kDudeHellHound:
                case kDudeSpiderBrown:
                case kDudeSpiderRed:
                case kDudeSpiderBlack:
                case kDudeBat:
                case kDudeRat:
                case kDudeBurningInnocent:
                    actKillDude(pSprite->index, pSprite, DAMAGE_TYPE_0, 1000<<4);
                    break;
                }
            }
            break;*/
        }
    }
    GetSpriteExtents(pSprite, &top, &bottom);
    if (pPlayer && bottom >= floorZ)
    {
        int floorZ2 = floorZ;
        int floorHit2 = floorHit;
        GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist<<2, CLIPMASK0, PARALLAXCLIP_CEILING|PARALLAXCLIP_FLOOR);
        if (bottom <= floorZ && pSprite->z - floorZ2 < bz)
        {
            floorZ = floorZ2;
            floorHit = floorHit2;
        }
    }
    if (floorZ <= bottom)
    {
        gSpriteHit[nXSprite].florhit = floorHit;
        pSprite->z += floorZ-bottom;
        int v30 = zvel[nSprite]-velFloor[pSprite->sectnum];
        if (v30 > 0)
        {
            int vax = actFloorBounceVector((int*)&xvel[nSprite], (int*)&yvel[nSprite], (int*)&v30, pSprite->sectnum, 0);
            int nDamage = mulscale(vax, vax, 30);
            if (pPlayer)
            {
                pPlayer->fallScream = 0;

                if (nDamage > (15<<4) && (pSprite->flags&4))
                    playerLandingSound(pPlayer);
                if (nDamage > (30<<4))
                    sfxPlay3DSound(pSprite, 701, 0, 0);
            }
            nDamage -= 100<<4;
            if (nDamage > 0)
                actDamageSprite(nSprite, pSprite, DAMAGE_TYPE_0, nDamage);
            zvel[nSprite] = v30;
            if (klabs(zvel[nSprite]) < 0x10000)
            {
                zvel[nSprite] = velFloor[pSprite->sectnum];

                pSprite->flags &= ~4;
            }
            else

                pSprite->flags |= 4;
            switch (tileGetSurfType(floorHit))
            {
            case kSurfWater:
                gFX.fxSpawn(FX_9, pSprite->sectnum, pSprite->x, pSprite->y, floorZ, 0);
                break;
            case kSurfLava:
            {
                spritetype *pFX = gFX.fxSpawn(FX_10, pSprite->sectnum, pSprite->x, pSprite->y, floorZ, 0);
                if (pFX)
                {
                    for (int i = 0; i < 7; i++)
                    {
                        spritetype *pFX2 = gFX.fxSpawn(FX_14, pFX->sectnum, pFX->x, pFX->y, pFX->z, 0);
                        if (pFX2)
                        {
                            xvel[pFX2->index] = Random2(0x6aaaa);
                            yvel[pFX2->index] = Random2(0x6aaaa);
                            zvel[pFX2->index] = -Random(0xd5555);
                        }
                    }
                }
                break;
            }
            }
        }
        else if (zvel[nSprite] == 0)

            pSprite->flags &= ~4;
    }
    else
    {
        gSpriteHit[nXSprite].florhit = 0;

        if (pSprite->flags&2)
            pSprite->flags |= 4;
    }
    if (top <= ceilZ)
    {
        gSpriteHit[nXSprite].ceilhit = ceilHit;
        pSprite->z += ClipLow(ceilZ-top, 0);

        if (zvel[nSprite] <= 0 && (pSprite->flags&4))
            zvel[nSprite] = mulscale16(-zvel[nSprite], 0x2000);
    }
    else
        gSpriteHit[nXSprite].ceilhit = 0;
    GetSpriteExtents(pSprite,&top,&bottom);

    pXSprite->height = ClipLow(floorZ-bottom, 0)>>8;
    if (xvel[nSprite] || yvel[nSprite])
    {
        if ((floorHit & 0xc000) == 0xc000)
        {
            int nHitSprite = floorHit & 0x3fff;
            if ((sprite[nHitSprite].cstat & 0x30) == 0)
            {
                xvel[nSprite] += mulscale(4, pSprite->x - sprite[nHitSprite].x, 2);
                yvel[nSprite] += mulscale(4, pSprite->y - sprite[nHitSprite].y, 2);
                return;
            }
        }
        int nXSector = sector[pSprite->sectnum].extra;
        if (nXSector > 0 && xsector[nXSector].Underwater)
            return;
        if (pXSprite->height >= 0x100)
            return;
        int nDrag = gDudeDrag;
        if (pXSprite->height > 0)
            nDrag -= scale(gDudeDrag, pXSprite->height, 0x100);
        xvel[nSprite] -= mulscale16r(xvel[nSprite], nDrag);
        yvel[nSprite] -= mulscale16r(yvel[nSprite], nDrag);

        if (approxDist(xvel[nSprite], yvel[nSprite]) < 0x1000)
            xvel[nSprite] = yvel[nSprite] = 0;
    }
}

int MoveMissile(spritetype *pSprite)
{
    int nXSprite = pSprite->extra;
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int vdi = -1;
    spritetype *pOwner = NULL;
    int bakCstat = 0;
    if (pSprite->owner >= 0)
    {
        int nOwner = actSpriteOwnerToSpriteId(pSprite);
        pOwner = &sprite[nOwner];
        if (IsDudeSprite(pOwner))
        {
            bakCstat = pOwner->cstat;
            pOwner->cstat &= ~257;
        }
        else
            pOwner = NULL;
    }
    gHitInfo.hitsect = -1;
    gHitInfo.hitwall = -1;
    gHitInfo.hitsprite = -1;
    if (pSprite->type == kMissileFlameSpray)
        actAirDrag(pSprite, 0x1000);
    int nSprite = pSprite->index;
    if (pXSprite->target != -1 && (xvel[nSprite] || yvel[nSprite] || zvel[nSprite]))
    {
        spritetype *pTarget = &sprite[pXSprite->target];
        XSPRITE *pXTarget;
        if (pTarget->extra > 0)
            pXTarget = &xsprite[pTarget->extra];
        else
            pXTarget = NULL;
        if (pTarget->statnum == kStatDude && pXTarget && pXTarget->health > 0)
        {
            int nTargetAngle = getangle(-(pTarget->y-pSprite->y), pTarget->x-pSprite->x);
            int UNUSED(nAngle) = getangle(xvel[nSprite]>>12,yvel[nSprite]>>12);
            int vx = missileInfo[pSprite->type - kMissileBase].velocity;
            int vy = 0;
            RotatePoint(&vx, &vy, (nTargetAngle+1536)&2047, 0, 0);
            xvel[nSprite] = vx;
            yvel[nSprite] = vy;
            int dx = pTarget->x-pSprite->x;
            int dy = pTarget->y-pSprite->y;
            int dz = pTarget->z-pSprite->z;
            // Inlined
            int vax = dz/10;
            if (pTarget->z < pSprite->z)
                vax = -vax;
            zvel[nSprite] += vax;
            ksqrt(dx*dx+dy*dy+dz*dz);
        }
    }
    int vx = xvel[nSprite]>>12;
    int vy = yvel[nSprite]>>12;
    int vz = zvel[nSprite]>>8;
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    int i = 1;
    while (1)
    {
        int x = pSprite->x;
        int y = pSprite->y;
        int z = pSprite->z;
        int nSector2 = pSprite->sectnum;
        clipmoveboxtracenum = 1;
        int vdx = ClipMove(&x, &y, &z, &nSector2, vx, vy, pSprite->clipdist<<2, (z-top)/4, (bottom-z)/4, CLIPMASK0);
        clipmoveboxtracenum = 3;
        short nSector = nSector2;
        if (nSector2 < 0)
        {
            vdi = -1;
            break;
        }
        if (vdx)
        {
            int nHitSprite = vdx & 0x3fff;
            if ((vdx&0xc000) == 0xc000)
            {
                gHitInfo.hitsprite = nHitSprite;
                vdi = 3;
            }
            else if ((vdx & 0xc000) == 0x8000)
            {
                gHitInfo.hitwall = nHitSprite;
                if (wall[nHitSprite].nextsector == -1)
                    vdi = 0;
                else
                {
                    int32_t fz, cz;
                    getzsofslope(wall[nHitSprite].nextsector, x, y, &cz, &fz);
                    if (z <= cz || z >= fz)
                        vdi = 0;
                    else
                        vdi = 4;
                }
            }
        }
        if (vdi == 4)
        {
            walltype *pWall = &wall[gHitInfo.hitwall];
            if (pWall->extra > 0)
            {
                XWALL *pXWall = &xwall[pWall->extra];
                if (pXWall->triggerVector)
                {
                    trTriggerWall(gHitInfo.hitwall, pXWall, kCmdWallImpact, nSprite);
                    if (!(pWall->cstat&64))
                    {
                        vdi = -1;
                        if (i-- > 0)
                            continue;
                        vdi = 0;
                        break;
                    }
                }
            }
        }
        if (vdi >= 0 && vdi != 3)
        {
            int nAngle = getangle(xvel[nSprite], yvel[nSprite]);
            x -= mulscale30(Cos(nAngle), 16);
            y -= mulscale30(Sin(nAngle), 16);
            int nVel = approxDist(xvel[nSprite], yvel[nSprite]);
            vz -= scale(0x100, zvel[nSprite], nVel);
            updatesector(x, y, &nSector);
            nSector2 = nSector;
        }
        int ceilZ, ceilHit, floorZ, floorHit;
        GetZRangeAtXYZ(x, y, z, nSector2, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist<<2, CLIPMASK0);
        GetSpriteExtents(pSprite, &top, &bottom);
        top += vz;
        bottom += vz;
        if (bottom >= floorZ)
        {
            gSpriteHit[nXSprite].florhit = floorHit;
            vz += floorZ-bottom;
            vdi = 2;
        }
        if (top <= ceilZ)
        {
            gSpriteHit[nXSprite].ceilhit = ceilHit;
            vz += ClipLow(ceilZ-top, 0);
            vdi = 1;
        }
        pSprite->x = x;
        pSprite->y = y;
        pSprite->z = z+vz;
        updatesector(x, y, &nSector);
        if (nSector >= 0 && nSector != pSprite->sectnum)
        {
            dassert(nSector >= 0 && nSector < kMaxSectors);
            ChangeSpriteSect(nSprite, nSector);
        }
        CheckLink(pSprite);
        gHitInfo.hitsect = pSprite->sectnum;
        gHitInfo.hitx = pSprite->x;
        gHitInfo.hity = pSprite->y;
        gHitInfo.hitz = pSprite->z;
        break;
    }
    if (pOwner)
        pOwner->cstat = bakCstat;
    return vdi;
}

void actExplodeSprite(spritetype *pSprite)
{
    int nXSprite = pSprite->extra;
    if (nXSprite <= 0 || nXSprite >= kMaxXSprites)
        return;
    if (pSprite->statnum == kStatExplosion)
        return;
    sfxKill3DSound(pSprite, -1, -1);
    evKill(pSprite->index, 3);
    int nType = kExplosionStandard;
    switch (pSprite->type)
    {
    case kMissileFireballNapam:
        nType = kExplosionNapalm;
        seqSpawn(4, 3, nXSprite, -1);
        if (Chance(0x8000))
            pSprite->cstat |= 4;
        sfxPlay3DSound(pSprite, 303, -1, 0);
        GibSprite(pSprite, GIBTYPE_5, NULL, NULL);
        break;
    case kMissileFlareAlt:
        nType = kExplosionFireball;
        seqSpawn(9, 3, nXSprite, -1);
        if (Chance(0x8000))
            pSprite->cstat |= 4;
        sfxPlay3DSound(pSprite, 306, 24+(pSprite->index&3), 1);
        GibSprite(pSprite, GIBTYPE_5, NULL, NULL);
        break;
    case kMissileFireballCerberus:
    case kMissileFireballTchernobog:
        nType = kExplosionFireball;
        seqSpawn(5, 3, nXSprite, -1);
        sfxPlay3DSound(pSprite, 304, -1, 0);
        GibSprite(pSprite, GIBTYPE_5, NULL, NULL);
        break;
    case kThingArmedTNTStick:
        nType = kExplosionSmall;
        if (gSpriteHit[nXSprite].florhit == 0) seqSpawn(4,3,nXSprite,-1);
        else seqSpawn(3,3,nXSprite,-1);
        sfxPlay3DSound(pSprite, 303, -1, 0);
        GibSprite(pSprite, GIBTYPE_5, NULL, NULL);
        break;
    case kThingArmedProxBomb:
    case kThingArmedRemoteBomb:
    case kThingArmedTNTBundle:
    case kModernThingTNTProx:
        nType = kExplosionStandard;
        if (gSpriteHit[nXSprite].florhit == 0)
            seqSpawn(4,3,nXSprite,-1);
        else
            seqSpawn(3,3,nXSprite,-1);
        sfxPlay3DSound(pSprite, 304, -1, 0);
        GibSprite(pSprite, GIBTYPE_5, NULL, NULL);
        break;
    case kThingArmedSpray:
        nType = kExplosionSpray;
        seqSpawn(5, 3, nXSprite, -1);
        sfxPlay3DSound(pSprite, 307, -1, 0);
        GibSprite(pSprite, GIBTYPE_5, NULL, NULL);
        break;
    case kThingTNTBarrel:
    {
        spritetype *pSprite2 = actSpawnSprite(pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0, 1);
        pSprite2->owner = pSprite->owner;
        if (actCheckRespawn(pSprite))
        {
            XSPRITE *pXSprite = &xsprite[nXSprite];
            pXSprite->state = 1;
            pXSprite->health = thingInfo[0].startHealth<<4;
        }
        else
            actPostSprite(pSprite->index, kStatFree);
        nType = kExplosionLarge;
        nXSprite = pSprite2->extra;
        seqSpawn(4, 3, nXSprite, -1);
        sfxPlay3DSound(pSprite2, 305, -1, 0);
        GibSprite(pSprite2, GIBTYPE_14, NULL, NULL);
        pSprite = pSprite2;
        break;
    }
    case kTrapExploder:	// By NoOne: allow to customize hidden exploder trap
	{
		// Defaults for exploder
		nType = 1; int nSnd = 304; int nSeq = 4;

        if (gModernMap) {
            // Temp variables for override via data fields
            int tSnd = 0; int tSeq = 0;


            XSPRITE* pXSPrite = &xsprite[nXSprite];
            nType = pXSPrite->data1;  // Explosion type
            tSeq = pXSPrite->data2; // SEQ id
            tSnd = pXSPrite->data3; // Sound Id

            if (nType <= 1 || nType > kExplodeMax) { nType = 1; nSeq = 4; nSnd = 304; }
            else if (nType == 2) { nSeq = 4; nSnd = 305; }
            else if (nType == 3) { nSeq = 9; nSnd = 307; }
            else if (nType == 4) { nSeq = 5; nSnd = 307; }
            else if (nType <= 6) { nSeq = 4; nSnd = 303; }
            else if (nType == 7) { nSeq = 4; nSnd = 303; }
            else if (nType == 8) { nType = 0; nSeq = 3; nSnd = 303; }

            // Override previous sound and seq assigns
            if (tSeq > 0) nSeq = tSeq;
            if (tSnd > 0) nSnd = tSnd;
        }
		
        if (gSysRes.Lookup(nSeq, "SEQ"))
		    seqSpawn(nSeq, 3, nXSprite, -1);

		sfxPlay3DSound(pSprite, nSnd, -1, 0);
	}
        break;
    case kThingPodFireBall:
        nType = kExplosionFireball;
        seqSpawn(9, 3, nXSprite, -1);
        sfxPlay3DSound(pSprite, 307, -1, 0);
        GibSprite(pSprite, GIBTYPE_5, NULL, NULL);
        sub_746D4(pSprite, 240);
        break;
    default:
        nType = kExplosionStandard;
        seqSpawn(4, 3, nXSprite, -1);
        if (Chance(0x8000))
            pSprite->cstat |= 4;
        sfxPlay3DSound(pSprite, 303, -1, 0);
        GibSprite(pSprite, GIBTYPE_5, NULL, NULL);
        break;
    }
    int nSprite = pSprite->index;
    xvel[nSprite] = yvel[nSprite] = zvel[nSprite] = 0;
    actPostSprite(nSprite, kStatExplosion);
    pSprite->xrepeat = pSprite->yrepeat = explodeInfo[nType].repeat;

    pSprite->flags &= ~3;
    pSprite->type = nType;
    EXPLOSION *pExplodeInfo = &explodeInfo[nType];
    xsprite[nXSprite].target = 0;
    xsprite[nXSprite].data1 = pExplodeInfo->ticks;
    xsprite[nXSprite].data2 = pExplodeInfo->quakeEffect;
    xsprite[nXSprite].data3 = pExplodeInfo->flashEffect;
}

void actActivateGibObject(spritetype *pSprite, XSPRITE *pXSprite)
{
    int vdx = ClipRange(pXSprite->data1, 0, 31);
    int vc = ClipRange(pXSprite->data2, 0, 31);
    int v4 = ClipRange(pXSprite->data3, 0, 31);
    int vbp = pXSprite->data4;
    int v8 = pXSprite->dropMsg;
    if (vdx > 0)
        GibSprite(pSprite, (GIBTYPE)(vdx-1), NULL, NULL);
    if (vc > 0)
        GibSprite(pSprite, (GIBTYPE)(vc-1), NULL, NULL);
    if (v4 > 0 && pXSprite->burnTime > 0)
        GibSprite(pSprite, (GIBTYPE)(v4-1), NULL, NULL);
    if (vbp > 0)
        sfxPlay3DSound(pSprite->x, pSprite->y, pSprite->z, vbp, pSprite->sectnum);
    if (v8 > 0)
        actDropObject(pSprite, v8);

    if (!(pSprite->cstat&32768) && !(pSprite->flags&kHitagRespawn))
        actPostSprite(pSprite->index, kStatFree);
}

bool IsUnderWater(spritetype *pSprite)
{
    int nSector = pSprite->sectnum;
    int nXSector = sector[nSector].extra;
    if (nXSector > 0 && nXSector < kMaxXSectors)
        if (xsector[nXSector].Underwater)
            return 1;
    return 0;
}

void MakeSplash(spritetype *pSprite, XSPRITE *pXSprite);

void actProcessSprites(void)
{
    int nSprite;
    int nNextSprite;
    
    if (gModernMap) {

        // by NoOne: process additional proximity sprites
        if (gProxySpritesCount > 0) {
            for (int i = 0; i < gProxySpritesCount; i++) {
                if (sprite[gProxySpritesList[i]].extra < 0) continue;

                XSPRITE * pXProxSpr = &xsprite[sprite[gProxySpritesList[i]].extra];
                if (!pXProxSpr->Proximity || (!pXProxSpr->Interrutable && pXProxSpr->state != pXProxSpr->restState)  || pXProxSpr->locked == 1 
                    || pXProxSpr->isTriggered) continue;  // don't process locked or triggered sprites

                int x = sprite[gProxySpritesList[i]].x;	int y = sprite[gProxySpritesList[i]].y;
                int z = sprite[gProxySpritesList[i]].z;	int index = sprite[gProxySpritesList[i]].xvel;
                int sectnum = sprite[gProxySpritesList[i]].sectnum;

                if (!pXProxSpr->DudeLockout) {

                    for (int nAffected = headspritestat[kStatDude]; nAffected >= 0; nAffected = nextspritestat[nAffected]) {

                        if ((sprite[nAffected].flags & 32) || xsprite[sprite[nAffected].extra].health <= 0) continue;
                        else if (CheckProximity(&sprite[nAffected], x, y, z, sectnum, 96)) {
                            trTriggerSprite(index, pXProxSpr, kCmdSpriteProximity, nAffected);
                            break;
                        }
                    }

                } else {

                    for (int a = connecthead; a >= 0; a = connectpoint2[a]) {
                        if (gPlayer[a].pXSprite->health > 0 && CheckProximity(gPlayer[a].pSprite, x, y, z, sectnum, 96)) {
                            trTriggerSprite(index, pXProxSpr, kCmdSpriteProximity, gPlayer[a].pSprite->index);
                            break;
                        }
                    }

                }
            }
        }

        // by NoOne: process sight sprites (for players only)
        if (gSightSpritesCount > 0) {
            for (int i = 0; i < gSightSpritesCount; i++) {
                if (sprite[gSightSpritesList[i]].extra < 0) continue;

                XSPRITE * pXSightSpr = &xsprite[sprite[gSightSpritesList[i]].extra];
                if (!pXSightSpr->Sight || (!pXSightSpr->Interrutable && pXSightSpr->state != pXSightSpr->restState) || pXSightSpr->locked == 1 || 
                    pXSightSpr->isTriggered) continue; // don't process locked or triggered sprites

                int x = sprite[gSightSpritesList[i]].x;	int y = sprite[gSightSpritesList[i]].y;
                int z = sprite[gSightSpritesList[i]].z;	int index = sprite[gSightSpritesList[i]].xvel;
                int sectnum = sprite[gSightSpritesList[i]].sectnum;

                for (int a = connecthead; a >= 0; a = connectpoint2[a]) {
                    spritetype* pPlaySprite = gPlayer[a].pSprite;
                    if (gPlayer[a].pXSprite->health > 0 && cansee(x, y, z, sectnum, pPlaySprite->x, pPlaySprite->y, pPlaySprite->z, pPlaySprite->sectnum)) {
                        trTriggerSprite(index, pXSightSpr, kCmdSpriteSight, pPlaySprite->index);
                        break;
                    }
                }
            }
        }

        // by NoOne: process Debris sprites for movement
        if (gPhysSpritesCount > 0) {
            //viewSetSystemMessage("PHYS COUNT: %d", gPhysSpritesCount);
            for (int i = 0; i < gPhysSpritesCount; i++) {
                if (gPhysSpritesList[i] == -1) continue;
                else if (sprite[gPhysSpritesList[i]].statnum == kStatFree || (sprite[gPhysSpritesList[i]].flags & kHitagFree) != 0) {
                    gPhysSpritesList[i] = -1;
                    continue;
                }
                
                XSPRITE* pXDebris = &xsprite[sprite[gPhysSpritesList[i]].extra];
                if (!(pXDebris->physAttr & kPhysMove) && !(pXDebris->physAttr & kPhysGravity)) {
                    gPhysSpritesList[i] = -1;
                    continue;
                }

                spritetype* pDebris = &sprite[gPhysSpritesList[i]];
                XSECTOR* pXSector = (sector[pDebris->sectnum].extra >= 0) ? &xsector[sector[pDebris->sectnum].extra] : NULL;
                viewBackupSpriteLoc(pDebris->xvel, pDebris);
                int airVel = gSpriteMass[pDebris->extra].airVel;
                if (pXSector != NULL) {
                    if (pXSector->Underwater) airVel <<= 6;
                    if (pXSector->panVel != 0) {
                        int top, bottom;
                        GetSpriteExtents(pDebris,&top,&bottom);

                        if (getflorzofslope(pDebris->sectnum, pDebris->x, pDebris->y) <= bottom)
                        {
                            int angle = pXSector->panAngle;
                            int speed = 0;
                            if (pXSector->panAlways || pXSector->state || pXSector->busy)
                            {
                                speed = pXSector->panVel << 9;
                                if (!pXSector->panAlways && pXSector->busy)
                                    speed = mulscale16(speed, pXSector->busy);
                            }
                            if (sector[pDebris->sectnum].floorstat & 64)
                                angle = (angle + GetWallAngle(sector[pDebris->sectnum].wallptr) + 512) & 2047;
                            int dx = mulscale30(speed, Cos(angle));
                            int dy = mulscale30(speed, Sin(angle));
                            xvel[pDebris->xvel] += dx;
                            yvel[pDebris->xvel] += dy;
                        }
                    }
                }

                actAirDrag(pDebris, airVel);

                if (((pDebris->index >> 8) & 15) == (gFrame & 15) && (pXDebris->physAttr & kPhysGravity))
                    pXDebris->physAttr |= kPhysFalling;

                if ((pXDebris->physAttr & 4) == 0 && xvel[pDebris->xvel] == 0 && yvel[pDebris->xvel] == 0 &&
                    zvel[pDebris->xvel] == 0 && velFloor[pDebris->sectnum] == 0 && velCeil[pDebris->sectnum] == 0)
                    continue;

                debrisMove(i);

            }
        }
    }

    for (nSprite = headspritestat[kStatThing]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];

        if (pSprite->flags&32)
            continue;
        int nXSprite = pSprite->extra;
        if (nXSprite > 0) {
            XSPRITE *pXSprite = &xsprite[nXSprite];
            switch (pSprite->type) {
                case kThingBloodBits:
                case kThingBloodChunks:
                case kThingZombieHead:
                    if (pXSprite->locked && gFrameClock >= pXSprite->targetX) pXSprite->locked = 0;
                    break;
            }

            if (pXSprite->burnTime > 0)
            {
                pXSprite->burnTime = ClipLow(pXSprite->burnTime-4,0);
                actDamageSprite(actOwnerIdToSpriteId(pXSprite->burnSource), pSprite, DAMAGE_TYPE_1, 8);
            }
                                       
            if (pXSprite->Proximity) {
                // by NoOne: don't process locked or 1-shot things for proximity
                if (gModernMap && (pXSprite->locked || pXSprite->isTriggered)) 
                    continue;
                
                if (pSprite->type == kThingDroppedLifeLeech) pXSprite->target = -1;
                for (int nSprite2 = headspritestat[kStatDude]; nSprite2 >= 0; nSprite2 = nNextSprite)
                {
                    
                    nNextSprite = nextspritestat[nSprite2];
                    spritetype *pSprite2 = &sprite[nSprite2];

                    if (pSprite2->flags&32) continue;
                    XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
                    if ((unsigned int)pXSprite2->health > 0) {
                   
                        // by NoOne: allow dudeLockout for proximity flag
                        if (gModernMap && pSprite->type != kThingDroppedLifeLeech && pXSprite->DudeLockout && !IsPlayerSprite(pSprite2))
                            continue;

                        int proxyDist = 96;
                        if (pSprite->type == kModernThingEnemyLifeLeech) proxyDist = 512;
                        else if (pSprite->type == kThingDroppedLifeLeech && pXSprite->target == -1)  {
                            int nOwner = actOwnerIdToSpriteId(pSprite->owner);
                            spritetype *pOwner = &sprite[nOwner];
                            PLAYER *pPlayer = &gPlayer[pOwner->type - kDudePlayer1];
                            PLAYER *pPlayer2 = NULL;
                            if (IsPlayerSprite(pSprite2))
                                pPlayer2 = &gPlayer[pSprite2->type - kDudePlayer1];
                            if (nSprite2 == nOwner || pSprite2->type == kDudeZombieAxeBuried || pSprite2->type == kDudeRat || pSprite2->type == kDudeBat)
                                continue;
                            if (gGameOptions.nGameType == 3 && pPlayer2 && pPlayer->teamId == pPlayer2->teamId)
                                continue;
                            if (gGameOptions.nGameType == 1 && pPlayer2)
                                continue;
                            proxyDist = 512;
                        }
                        
                        if (CheckProximity(pSprite2, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, proxyDist)) {

                            switch (pSprite->type) {
                                case kModernThingTNTProx:
                                    if (!IsPlayerSprite(pSprite2)) continue;
                                    pSprite->pal = 0;
                                    break;
                                case kThingDroppedLifeLeech:
                                    if (!Chance(0x4000) && nNextSprite >= 0) continue;
                                    if (pSprite2->cstat & CLIPMASK0) pXSprite->target = pSprite2->index;
                                    else continue;
                                    break;
                                case kModernThingEnemyLifeLeech:
                                    if (pXSprite->target != pSprite2->xvel) continue;
                                    break;
                            }
                            if (pSprite->owner == -1) actPropagateSpriteOwner(pSprite, pSprite2);
                            trTriggerSprite(nSprite, pXSprite, kCmdSpriteProximity, pSprite2->index);
                        }
                    }
                }
            }
        }
    }
    for (nSprite = headspritestat[kStatThing]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];

        if (pSprite->flags & 32)
            continue;
        int nSector = pSprite->sectnum;
        int nXSprite = pSprite->extra;
        dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
        int nXSector = sector[nSector].extra;
        XSECTOR *pXSector = NULL;
        if (nXSector > 0)
        {
            dassert(nXSector > 0 && nXSector < kMaxXSectors);
            dassert(xsector[nXSector].reference == nSector);
            pXSector = &xsector[nXSector];
        }
        if (pXSector && pXSector->panVel && (pXSector->panAlways || pXSector->state || pXSector->busy))
        {
            int nType = pSprite->type - kThingBase;
            const THINGINFO *pThingInfo = &thingInfo[nType];
            if (pThingInfo->flags & 1)

                pSprite->flags |= 1;
            if (pThingInfo->flags & 2)

                pSprite->flags |= 4;
        }

        if (pSprite->flags&3)
        {
            viewBackupSpriteLoc(nSprite, pSprite);
            if (pXSector && pXSector->panVel)
            {
                int top, bottom;
                GetSpriteExtents(pSprite, &top, &bottom);
                if (getflorzofslope(nSector, pSprite->x, pSprite->y) <= bottom)
                {
                    int angle = pXSector->panAngle;
                    int speed = 0;
                    if (pXSector->panAlways || pXSector->state || pXSector->busy)
                    {
                        speed = pXSector->panVel << 9;
                        if (!pXSector->panAlways && pXSector->busy)
                            speed = mulscale16(speed, pXSector->busy);
                    }
                    if (sector[nSector].floorstat&64)
                        angle = (angle+GetWallAngle(sector[nSector].wallptr)+512)&2047;
                    int dx = mulscale30(speed, Cos(angle));
                    int dy = mulscale30(speed, Sin(angle));
                    xvel[nSprite] += dx;
                    yvel[nSprite] += dy;
                }
            }
            actAirDrag(pSprite, 128);

            if (((pSprite->index>>8)&15) == (gFrame&15) && (pSprite->flags&2))
                pSprite->flags |= 4;
            if ((pSprite->flags&4) || xvel[nSprite] || yvel[nSprite] || zvel[nSprite] ||
                velFloor[pSprite->sectnum] || velCeil[pSprite->sectnum])
            {
                int hit = MoveThing(pSprite);
                if (hit)
                {
                    int nXSprite = pSprite->extra;
                    if (nXSprite)
                    {
                        XSPRITE *pXSprite = &xsprite[nXSprite];
                        if (pXSprite->Impact)
                            trTriggerSprite(nSprite, pXSprite, kCmdOff, -1);
                        switch (pSprite->type) {
                        case kThingDripWater:
                        case kThingDripBlood:
                            MakeSplash(pSprite, pXSprite);
                            break;
                        case kModernThingThrowableRock:
                            seqSpawn(24, 3, nXSprite, -1);
                            if ((hit & 0xc000) == 0xc000)
                            {
                                pSprite->xrepeat = 32;
                                pSprite->yrepeat = 32;
                                int nObject = hit & 0x3fff;
                                dassert(nObject >= 0 && nObject < kMaxSprites);
                                spritetype * pObject = &sprite[nObject];
                                actDamageSprite(actSpriteOwnerToSpriteId(pSprite), pObject, DAMAGE_TYPE_0, pXSprite->data1);
                            }
                            break;
                        case kThingBone:
                            seqSpawn(24, 3, nXSprite, -1);
                            if ((hit&0xc000) == 0xc000)
                            {
                                int nObject = hit & 0x3fff;
                                dassert(nObject >= 0 && nObject < kMaxSprites);
                                spritetype *pObject = &sprite[nObject];
                                actDamageSprite(actSpriteOwnerToSpriteId(pSprite), pObject, DAMAGE_TYPE_0, 12);
                            }
                            break;
                        case kThingPodGreenBall:
                            if ((hit&0xc000) == 0x4000)
                            {
                                sub_2A620(actSpriteOwnerToSpriteId(pSprite), pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, 200, 1, 20, DAMAGE_TYPE_3, 6, 0, 0, 0);
                                evPost(pSprite->index, 3, 0, kCallbackFXPodBloodSplat);
                            }
                            else
                            {
                                int nObject = hit & 0x3fff;
                                if ((hit&0xc000) != 0xc000 && (nObject < 0 || nObject >= 4096))
                                    break;
                                dassert(nObject >= 0 && nObject < kMaxSprites);
                                spritetype *pObject = &sprite[nObject];
                                actDamageSprite(actSpriteOwnerToSpriteId(pSprite), pObject, DAMAGE_TYPE_0, 12);
                                evPost(pSprite->index, 3, 0, kCallbackFXPodBloodSplat);
                            }
                            break;
                        case kThingPodFireBall:
                        {
                            int nObject = hit & 0x3fff;
                            if ((hit&0xc000) != 0xc000 && (nObject < 0 || nObject >= 4096))
                                break;
                            dassert(nObject >= 0 && nObject < kMaxSprites);
                            int UNUSED(nOwner) = actSpriteOwnerToSpriteId(pSprite);
                            actExplodeSprite(pSprite);
                            break;
                        }
                        }
                    }
                }
            }
        }
    }
    for (nSprite = headspritestat[kStatProjectile]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];

        if (pSprite->flags & 32)
            continue;
        viewBackupSpriteLoc(nSprite, pSprite);
        int hit = MoveMissile(pSprite);
        if (hit >= 0)
            actImpactMissile(pSprite, hit);
    }
    for (nSprite = headspritestat[kStatExplosion]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        char v24c[(kMaxSectors+7)>>3];
        spritetype *pSprite = &sprite[nSprite];

        if (pSprite->flags & 32)
            continue;
        int nOwner = actSpriteOwnerToSpriteId(pSprite);
        int nType = pSprite->type;
        dassert(nType >= 0 && nType < kExplodeMax);
        EXPLOSION *pExplodeInfo = &explodeInfo[nType];
        int nXSprite = pSprite->extra;
        dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
        XSPRITE *pXSprite = &xsprite[nXSprite];
        int x = pSprite->x;
        int y = pSprite->y;
        int z = pSprite->z;
        int nSector = pSprite->sectnum;
        gAffectedSectors[0] = -1;
        gAffectedXWalls[0] = -1;
        int radius = pExplodeInfo->radius;

        // By NoOne: Allow to override explosion radius by data4 field of any sprite which have statnum 2 set in editor
        // or of Hidden Exploder.
        if (gModernMap && pXSprite->data4 > 0)
            radius = pXSprite->data4;
        
        GetClosestSpriteSectors(nSector, x, y, radius, gAffectedSectors, v24c, gAffectedXWalls);
        
        for (int i = 0; i < kMaxXWalls; i++)
        {
            int nWall = gAffectedXWalls[i];
            if (nWall == -1)
                break;
            XWALL *pXWall = &xwall[wall[nWall].extra];
            trTriggerWall(nWall, pXWall, kCmdWallImpact, nSprite);
        }
        
        for (int nSprite2 = headspritestat[kStatDude]; nSprite2 >= 0; nSprite2 = nextspritestat[nSprite2])
        {
            spritetype *pDude = &sprite[nSprite2];

            if (pDude->flags & 32)
                continue;
            if (TestBitString(v24c, pDude->sectnum))
            {
                if (pXSprite->data1 && CheckProximity(pDude, x, y, z, nSector, radius))
                {
                    if (pExplodeInfo->dmg && pXSprite->target == 0)
                    {
                        pXSprite->target = 1;
                        actDamageSprite(nOwner, pDude, DAMAGE_TYPE_0, (pExplodeInfo->dmg+Random(pExplodeInfo->dmgRng))<<4);
                    }
                    if (pExplodeInfo->dmgType)
                        ConcussSprite(nOwner, pDude, x, y, z, pExplodeInfo->dmgType);
                    if (pExplodeInfo->burnTime)
                    {
                        dassert(pDude->extra > 0 && pDude->extra < kMaxXSprites);
                        XSPRITE *pXDude = &xsprite[pDude->extra];
                        if (!pXDude->burnTime)
                            evPost(nSprite2, 3, 0, kCallbackFXFlameLick);
                        actBurnSprite(pSprite->owner, pXDude, pExplodeInfo->burnTime<<2);
                    }
                }
            }
        }
        
        for (int nSprite2 = headspritestat[kStatThing]; nSprite2 >= 0; nSprite2 = nextspritestat[nSprite2])
        {
            spritetype *pThing = &sprite[nSprite2];

            if (pThing->flags & 32)
                continue;
            if (TestBitString(v24c, pThing->sectnum))
            {
                if (pXSprite->data1 && CheckProximity(pThing, x, y, z, nSector, radius))
                {
                    XSPRITE *pXSprite2 = &xsprite[pThing->extra];
                    if (!pXSprite2->locked)
                    {
                        if (pExplodeInfo->dmgType)
                            ConcussSprite(nOwner, pThing, x, y, z, pExplodeInfo->dmgType);
                        if (pExplodeInfo->burnTime)
                        {
                            dassert(pThing->extra > 0 && pThing->extra < kMaxXSprites);
                            XSPRITE *pXThing = &xsprite[pThing->extra];
                            if (pThing->type == kThingTNTBarrel && !pXThing->burnTime)
                                evPost(nSprite2, 3, 0, kCallbackFXFlameLick);
                            actBurnSprite(pSprite->owner, pXThing, pExplodeInfo->burnTime<<2);
                        }
                    }
                }
            }
        }
        
        // by NoOne: add impulse for sprites from physics list
        if (gPhysSpritesCount > 0 && pExplodeInfo->dmgType != 0 && pXSprite->data1 != 0) {
            for (int i = 0; i < gPhysSpritesCount; i++) {
                if (gPhysSpritesList[i] == -1) continue;

                else if (sprite[gPhysSpritesList[i]].sectnum < 0 || (sprite[gPhysSpritesList[i]].flags & kHitagFree) != 0)
                    continue;

                spritetype* pDebris = &sprite[gPhysSpritesList[i]];
                if (!TestBitString(v24c, pDebris->sectnum) || !CheckProximity(pDebris, x, y, z, nSector, radius)) continue;
                else debrisConcuss(nOwner, i, x, y, z, pExplodeInfo->dmgType);
            }
        }

        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            spritetype *pSprite2 = gPlayer[p].pSprite;
            int dx = (x - pSprite2->x)>>4;
            int dy = (y - pSprite2->y)>>4;
            int dz = (z - pSprite2->z)>>8;
            int nDist = dx*dx+dy*dy+dz*dz+0x40000;
            int t = divscale16(pXSprite->data2, nDist);
            gPlayer[p].flickerEffect += t;
        }
        
        // By NoOne: if data4 > 0, do not remove explosion. This can be useful when designer wants put explosion generator in map manually
	    // via sprite statnum 2.

        if (!gModernMap || !(pSprite->flags & kModernTypeFlag1)) {
            pXSprite->data1 = ClipLow(pXSprite->data1 - 4, 0);
            pXSprite->data2 = ClipLow(pXSprite->data2 - 4, 0);
            pXSprite->data3 = ClipLow(pXSprite->data3 - 4, 0);
        }

        if (pXSprite->data1 == 0 && pXSprite->data2 == 0 && pXSprite->data3 == 0 && seqGetStatus(3, nXSprite) < 0)
            actPostSprite(nSprite, kStatFree);
    }
   
    for (nSprite = headspritestat[kStatTraps]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        spritetype *pSprite = &sprite[nSprite];

        if (pSprite->flags & 32)
            continue;
        int nXSprite = pSprite->extra;
        dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
        XSPRITE *pXSprite = &xsprite[nXSprite];
        switch (pSprite->type) {
        case kTrapSawCircular:
            pXSprite->data2 = ClipLow(pXSprite->data2-4, 0);
            break;
        case kTrapFlame:
            if (pXSprite->state && seqGetStatus(3, nXSprite) < 0) {
                int x = pSprite->x;
                int y = pSprite->y;
                int z = pSprite->z;
                int t = (pXSprite->data1<<23)/120;
                int dx = mulscale30(t, Cos(pSprite->ang));
                int dy = mulscale30(t, Sin(pSprite->ang));
                for (int i = 0; i < 2; i++)
                {
                    spritetype *pFX = gFX.fxSpawn(FX_32, pSprite->sectnum, x, y, z, 0);
                    if (pFX)
                    {
                        xvel[pFX->index] = dx + Random2(0x8888);
                        yvel[pFX->index] = dy + Random2(0x8888);
                        zvel[pFX->index] = Random2(0x8888);
                    }
                    x += (dx/2)>>12;
                    y += (dy/2)>>12;
                }
                dy = Sin(pSprite->ang)>>16;
                dx = Cos(pSprite->ang)>>16;
                gVectorData[VECTOR_TYPE_20].maxDist = pXSprite->data1<<9;
                actFireVector(pSprite, 0, 0, dx, dy, Random2(0x8888), VECTOR_TYPE_20);
            }
            break;
        }
    }
    for (nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];

        if (pSprite->flags & 32)
            continue;
        int nXSprite = pSprite->extra;
        if (nXSprite > 0)
        {
            XSPRITE *pXSprite = &xsprite[nXSprite];
            if (pXSprite->burnTime > 0)
            {
                switch (pSprite->type)
                {
                case kDudeBurningInnocent:
                case kDudeBurningCultist:
                case kDudeBurningZombieAxe:
                case kDudeBurningZombieButcher:
                    actDamageSprite(actOwnerIdToSpriteId(pXSprite->burnSource), pSprite, DAMAGE_TYPE_1, 8);
                    break;
                default:
                    pXSprite->burnTime = ClipLow(pXSprite->burnTime-4, 0);
                    actDamageSprite(actOwnerIdToSpriteId(pXSprite->burnSource), pSprite, DAMAGE_TYPE_1, 8);
                    break;
                }
            }

            // By NoOne: handle incarnations of custom dude
            if (pSprite->type == kDudeModernCustom && pXSprite->txID > 0 && pXSprite->sysData1 == kGenDudeTransformStatus) {
                xvel[pSprite->index] = yvel[pSprite->index] =  0;
                if (seqGetStatus(3, nXSprite) < 0) {
                XSPRITE* pXIncarnation = getNextIncarnation(pXSprite);
                    if (pXIncarnation != NULL) {
                    spritetype* pIncarnation = &sprite[pXIncarnation->reference];
                    pXSprite->key = pXSprite->dropMsg = pXSprite->locked = 0;

                    // save incarnation's going on and off options
                    bool triggerOn = pXIncarnation->triggerOn;
                    bool triggerOff = pXIncarnation->triggerOff;

                    // then remove it from incarnation so it will not send the commands
                    pXIncarnation->triggerOn = false;
                    pXIncarnation->triggerOff = false;

                    // trigger dude death before transform
                    trTriggerSprite(nSprite, pXSprite, kCmdOff, pSprite->owner);

                    pSprite->type = pIncarnation->type;
                    pSprite->flags = pIncarnation->flags;
                    pSprite->pal = pIncarnation->pal;
                    pSprite->shade = pIncarnation->shade;
                    pSprite->clipdist = pIncarnation->clipdist;
                    pSprite->xrepeat = pIncarnation->xrepeat;
                    pSprite->yrepeat = pIncarnation->yrepeat;

                    pXSprite->txID = pXIncarnation->txID;
                    pXSprite->command = pXIncarnation->command;
                    pXSprite->triggerOn = triggerOn;
                    pXSprite->triggerOff = triggerOff;
                    pXSprite->busyTime = pXIncarnation->busyTime;
                    pXSprite->waitTime = pXIncarnation->waitTime;

                    pXSprite->burnTime = 0;
                    pXSprite->burnSource = -1;

                    pXSprite->data1 = pXIncarnation->data1;
                    pXSprite->data2 = pXIncarnation->data2;

                    // if incarnation is active dude, it's sndStartId will be stored in sysData1, otherwise it will be data3
                    if (pIncarnation->statnum == kStatDude && pIncarnation->type == kDudeModernCustom) pXSprite->sysData1 = pXIncarnation->sysData1;
                        else pXSprite->sysData1 = pXIncarnation->data3;

                    pXSprite->data4 = pXIncarnation->data4;

                    pXSprite->dudeGuard = pXIncarnation->dudeGuard;
                    pXSprite->dudeDeaf = pXIncarnation->dudeDeaf;
                    pXSprite->dudeAmbush = pXIncarnation->dudeAmbush;
                    pXSprite->dudeFlag4 = pXIncarnation->dudeFlag4;

                    pXSprite->dropMsg = pXIncarnation->dropMsg;
                    pXSprite->key = pXIncarnation->key;

                    pXSprite->locked = pXIncarnation->locked;
                    pXSprite->Decoupled = pXIncarnation->Decoupled;

                    // clear drop items of the incarnation
                    pXIncarnation->key = pXIncarnation->dropMsg = 0;

                    // set hp
                    if (pXSprite->data4 <= 0) pXSprite->health = dudeInfo[pSprite->type - kDudeBase].startHealth << 4;
                        else pXSprite->health = ClipRange(pXSprite->data4 << 4, 1, 65535);

                    int seqId = dudeInfo[pSprite->type - kDudeBase].seqStartID;
                    switch (pSprite->type) {
                        case kDudePodMother: // fake dude
                        case kDudeTentacleMother: // fake dude
                            break;
                        case kDudeModernCustom:
                        case kDudeModernCustomBurning:
                            seqId = genDudeSeqStartId(pXSprite);
                            genDudePrepare(pSprite, kGenDudePropertyMass);
                            fallthrough__; // go below
                        default:
                            seqSpawn(seqId, 3, nXSprite, -1);

                            // save target
                            int target = pXSprite->target;

                            // re-init sprite
                            aiInitSprite(pSprite);

                            // try to restore target
                            if (target == -1) aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);
                            else aiSetTarget(pXSprite, target);

                            // finally activate it
                            aiActivateDude(pSprite, pXSprite);

                            break;
                    }

                    // remove the incarnation in case if non-locked
                    if (pXIncarnation->locked == 0) {
                        pXIncarnation->txID = pIncarnation->type = 0;
                        actPostSprite(pIncarnation->xvel, kStatFree);
                    // or restore triggerOn and off options
                    } else {
                        pXIncarnation->triggerOn = triggerOn;
                        pXIncarnation->triggerOff = triggerOff;
                    }
                } else {
                        if (pXSprite->sysData1 == kGenDudeTransformStatus) pXSprite->sysData1 = 0;
                    // just trigger dude death
                    trTriggerSprite(nSprite, pXSprite, kCmdOff, pSprite->owner);
                }
            }
            }

            if (pSprite->type == kDudeCerberusTwoHead)
            {
                if (pXSprite->health <= 0 && seqGetStatus(3, nXSprite) < 0)
                {
                    pXSprite->health = dudeInfo[28].startHealth<<4;
                    pSprite->type = kDudeCerberusOneHead;
                    if (pXSprite->target != -1)
                        aiSetTarget(pXSprite, pXSprite->target);
                    aiActivateDude(pSprite, pXSprite);
                }
            }
            if (pXSprite->Proximity && !pXSprite->isTriggered)
            {
                for (int nSprite2 = headspritestat[kStatDude]; nSprite2 >= 0; nSprite2 = nNextSprite)
                {
                    nNextSprite = nextspritestat[nSprite2];
                    spritetype *pSprite2 = &sprite[nSprite2];

                    if (pSprite2->flags&32)
                        continue;
                    XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
                    if ((unsigned int)pXSprite2->health > 0 && IsPlayerSprite(pSprite2)) {
                        if (CheckProximity(pSprite2, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, 128))
                            trTriggerSprite(nSprite, pXSprite, kCmdSpriteProximity, pSprite2->index);
                    }
                }
            }
            if (IsPlayerSprite(pSprite))
            {
                PLAYER *pPlayer = &gPlayer[pSprite->type-kDudePlayer1];
                if (pPlayer->voodooTargets)
                    sub_41250(pPlayer);
                if (pPlayer->hand && Chance(0x8000))
                    actDamageSprite(nSprite, pSprite, DAMAGE_TYPE_4, 12);
                if (pPlayer->isUnderwater)
                {
                    char bActive = packItemActive(pPlayer, 1);
                    if (bActive || pPlayer->godMode)
                        pPlayer->underwaterTime = 1200;
                    else
                        pPlayer->underwaterTime = ClipLow(pPlayer->underwaterTime-4, 0);
                    if (pPlayer->underwaterTime < 1080 && packCheckItem(pPlayer, 1) && !bActive)
                        packUseItem(pPlayer, 1);
                    if (!pPlayer->underwaterTime)
                    {
                        pPlayer->chokeEffect += 4;
                        if (Chance(pPlayer->chokeEffect))
                            actDamageSprite(nSprite, pSprite, DAMAGE_TYPE_4, 3<<4);
                    }
                    else
                        pPlayer->chokeEffect = 0;
                    if (xvel[nSprite] || yvel[nSprite])
                        sfxPlay3DSound(pSprite, 709, 100, 2);
                    pPlayer->bubbleTime = ClipLow(pPlayer->bubbleTime-4, 0);
                }
                else if (gGameOptions.nGameType == 0)
                {
                    if (pPlayer->pXSprite->health > 0 && pPlayer->restTime >= 1200 && Chance(0x200))
                    {
                        pPlayer->restTime = -1;
                        sfxPlay3DSound(pSprite, 3100+Random(11), 0, 2);
                    }
                }
            }
            ProcessTouchObjects(pSprite, nXSprite);
        }
    }
    for (nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];

        if (pSprite->flags & 32)
            continue;
        int nXSprite = pSprite->extra;
        dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
        int nSector = pSprite->sectnum;
        viewBackupSpriteLoc(nSprite, pSprite);
        int nXSector = sector[nSector].extra;
        XSECTOR *pXSector = NULL;
        if (nXSector > 0)
        {
            dassert(nXSector > 0 && nXSector < kMaxXSectors);
            dassert(xsector[nXSector].reference == nSector);
            pXSector = &xsector[nXSector];
        }
        if (pXSector)
        {
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            if (getflorzofslope(nSector, pSprite->x, pSprite->y) <= bottom)
            {
                int angle = pXSector->panAngle;
                int speed = 0;
                if (pXSector->panAlways || pXSector->state || pXSector->busy)
                {
                    speed = pXSector->panVel << 9;
                    if (!pXSector->panAlways && pXSector->busy)
                        speed = mulscale16(speed, pXSector->busy);
                }
                if (sector[nSector].floorstat&64)
                    angle = (angle+GetWallAngle(sector[nSector].wallptr)+512)&2047;
                int dx = mulscale30(speed, Cos(angle));
                int dy = mulscale30(speed, Sin(angle));
                xvel[nSprite] += dx;
                yvel[nSprite] += dy;
            }
        }
        if (pXSector && pXSector->Underwater)
            actAirDrag(pSprite, 5376);
        else
            actAirDrag(pSprite, 128);

        if ((pSprite->flags&4) || xvel[nSprite] || yvel[nSprite] || zvel[nSprite] ||
            velFloor[pSprite->sectnum] || velCeil[pSprite->sectnum])
            MoveDude(pSprite);
    }
    for (nSprite = headspritestat[kStatFlare]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];

        if (pSprite->flags & 32)
            continue;
        int nXSprite = pSprite->extra;
        dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
        XSPRITE *pXSprite = &xsprite[nXSprite];
        int nTarget = pXSprite->target;
        dassert(nTarget >= 0);
        viewBackupSpriteLoc(nSprite, pSprite);
        dassert(nTarget < kMaxSprites);
        spritetype *pTarget = &sprite[nTarget];
        if (pTarget->statnum == kMaxStatus)
        {
            GibSprite(pSprite, GIBTYPE_17, NULL, NULL);
            actPostSprite(pSprite->index, kStatFree);
        }
        if (pTarget->extra > 0 && xsprite[pTarget->extra].health > 0)
        {
            int x = pTarget->x+mulscale30r(Cos(pXSprite->goalAng+pTarget->ang), pTarget->clipdist*2);
            int y = pTarget->y+mulscale30r(Sin(pXSprite->goalAng+pTarget->ang), pTarget->clipdist*2);
            int z = pTarget->z+pXSprite->targetZ;
            vec3_t pos = { x, y, z };
            setsprite(nSprite,&pos);
            xvel[nSprite] = xvel[nTarget];
            yvel[nSprite] = yvel[nTarget];
            zvel[nSprite] = zvel[nTarget];
        }
        else
        {
            GibSprite(pSprite, GIBTYPE_17, NULL, NULL);
            actPostSprite(pSprite->index, kStatFree);
        }
    }
    aiProcessDudes();
    gFX.fxProcess();
}

spritetype * actSpawnSprite(int nSector, int x, int y, int z, int nStat, char a6)
{
    int nSprite = InsertSprite(nSector, nStat);
    if (nSprite >= 0)
        sprite[nSprite].extra = -1;
    else
    {
        nSprite = headspritestat[kStatPurge];
        dassert(nSprite >= 0);
        dassert(nSector >= 0 && nSector < kMaxSectors);
        ChangeSpriteSect(nSprite, nSector);
        actPostSprite(nSprite, nStat);
    }
    vec3_t pos = { x, y, z };
    setsprite(nSprite, &pos);
    spritetype *pSprite = &sprite[nSprite];
    pSprite->type = kSpriteDecoration;
    if (a6 && pSprite->extra == -1)
    {
        int nXSprite = dbInsertXSprite(nSprite);
        gSpriteHit[nXSprite].florhit = 0;
        gSpriteHit[nXSprite].ceilhit = 0;
        if (!VanillaMode())
            xsprite[nXSprite].target = -1;
    }
    return pSprite;
}

spritetype * actSpawnSprite(spritetype *pSource, int nStat);

spritetype *actSpawnDude(spritetype *pSource, short nType, int a3, int a4)
{
    XSPRITE* pXSource = &xsprite[pSource->extra];
    spritetype *pSprite2 = actSpawnSprite(pSource, kStatDude);
    if (!pSprite2) return NULL;
    XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
    int angle = pSource->ang;
    int nDude = nType-kDudeBase;
    int x, y, z;
    z = a4 + pSource->z;
    if (a3 < 0)
    {
        x = pSource->x;
        y = pSource->y;
    }
    else
    {
        x = pSource->x+mulscale30r(Cos(angle), a3);
        y = pSource->y+mulscale30r(Sin(angle), a3);
    }
    pSprite2->type = nType;
    pSprite2->ang = angle;
    vec3_t pos = { x, y, z };
    setsprite(pSprite2->index, &pos);
    pSprite2->cstat |= 0x1101;
    pSprite2->clipdist = dudeInfo[nDude].clipdist;
    pXSprite2->health = dudeInfo[nDude].startHealth<<4;
    pXSprite2->respawn = 1;
    if (gSysRes.Lookup(dudeInfo[nDude].seqStartID, "SEQ"))
        seqSpawn(dudeInfo[nDude].seqStartID, 3, pSprite2->extra, -1);
    
    // By NoOne: add a way to inherit some values of spawner type 18 by dude.
    // This way designer can count enemies via switches and do many other interesting things.
    if (gModernMap && pSource->flags & kModernTypeFlag1) {
        switch (pSource->type) { // allow inheriting only for selected source types
            case kMarkerDudeSpawn:
                //inherit pal?
                if (pSprite2->pal <= 0) pSprite2->pal = pSource->pal;

                // inherit spawn sprite trigger settings, so designer can count monsters.
                pXSprite2->txID = pXSource->txID;
                pXSprite2->command = pXSource->command;
                pXSprite2->triggerOn = pXSource->triggerOn;
                pXSprite2->triggerOff = pXSource->triggerOff;

                // inherit drop items
                pXSprite2->dropMsg = pXSource->dropMsg;

                // inherit dude flags
                pXSprite2->dudeDeaf = pXSource->dudeDeaf;
                pXSprite2->dudeGuard = pXSource->dudeGuard;
                pXSprite2->dudeAmbush = pXSource->dudeAmbush;
                pXSprite2->dudeFlag4 = pXSource->dudeFlag4;
                break;
        }
    }

    aiInitSprite(pSprite2);
    return pSprite2;
}

spritetype * actSpawnSprite(spritetype *pSource, int nStat)
{
    int nSprite = InsertSprite(pSource->sectnum, nStat);
    if (nSprite < 0)
    {
        nSprite = headspritestat[kStatPurge];
        dassert(nSprite >= 0);
        dassert(pSource->sectnum >= 0 && pSource->sectnum < kMaxSectors);
        ChangeSpriteSect(nSprite, pSource->sectnum);
        actPostSprite(nSprite, nStat);
    }
    spritetype *pSprite = &sprite[nSprite];
    pSprite->x = pSource->x;
    pSprite->y = pSource->y;
    pSprite->z = pSource->z;
    xvel[nSprite] = xvel[pSource->index];
    yvel[nSprite] = yvel[pSource->index];
    zvel[nSprite] = zvel[pSource->index];
    pSprite->flags = 0;
    int nXSprite = dbInsertXSprite(nSprite);
    gSpriteHit[nXSprite].florhit = 0;
    gSpriteHit[nXSprite].ceilhit = 0;
    if (!VanillaMode())
        xsprite[nXSprite].target = -1;
    return pSprite;
}

spritetype * actSpawnThing(int nSector, int x, int y, int z, int nThingType)
{
    dassert(nThingType >= kThingBase && nThingType < kThingMax);
    spritetype *pSprite = actSpawnSprite(nSector, x, y, z, 4, 1);
    int nType = nThingType-kThingBase;
    int nThing = pSprite->index;
    int nXThing = pSprite->extra;
    pSprite->type = nThingType;
    dassert(nXThing > 0 && nXThing < kMaxXSprites);
    XSPRITE *pXThing = &xsprite[nXThing];
    const THINGINFO *pThingInfo = &thingInfo[nType];
    pXThing->health = pThingInfo->startHealth<<4;
    pSprite->clipdist = pThingInfo->clipdist;
    pSprite->flags = pThingInfo->flags;
    if (pSprite->flags & 2)
        pSprite->flags |= 4;
    pSprite->cstat |= pThingInfo->cstat;
    pSprite->picnum = pThingInfo->picnum;
    pSprite->shade = pThingInfo->shade;
    pSprite->pal = pThingInfo->pal;
    if (pThingInfo->xrepeat)
        pSprite->xrepeat = pThingInfo->xrepeat;
    if (pThingInfo->yrepeat)
        pSprite->yrepeat = pThingInfo->yrepeat;
    SetBitString(show2dsprite, pSprite->index);
    switch (nThingType) {
    case kThingVoodooHead:
        pXThing->data1 = 0;
        pXThing->data2 = 0;
        pXThing->data3 = 0;
        pXThing->data4 = 0;
        pXThing->state = 1;
        pXThing->triggerOnce = 1;
        pXThing->isTriggered = 0;
        break;
    case kThingDroppedLifeLeech:
    case kModernThingEnemyLifeLeech:
        pXThing->data1 = 0;
        pXThing->data2 = 0;
        pXThing->data3 = 0;
        pXThing->data4 = 0;
        pXThing->state = 1;
        pXThing->triggerOnce = 0;
        pXThing->isTriggered = 0;
        break;
    case kThingZombieHead:
        pXThing->data1 = 8;
        pXThing->data2 = 0;
        pXThing->data3 = 0;
        pXThing->data4 = 318;
        pXThing->targetX = (int)gFrameClock+180.0;
        pXThing->locked = 1;
        pXThing->state = 1;
        pXThing->triggerOnce = 0;
        pXThing->isTriggered = 0;
        break;
    case kThingBloodBits:
    case kThingBloodChunks:
        pXThing->data1 = (nThingType == kThingBloodBits) ? 19 : 8;
        pXThing->data2 = 0;
        pXThing->data3 = 0;
        pXThing->data4 = 318;
        pXThing->targetX = (int)gFrameClock+180.0;
        pXThing->locked = 1;
        pXThing->state = 1;
        pXThing->triggerOnce = 0;
        pXThing->isTriggered = 0;
        break;
    case kThingArmedTNTStick:
        evPost(nThing, 3, 0, kCallbackFXDynPuff);
        sfxPlay3DSound(pSprite, 450, 0, 0);
        break;
    case kThingArmedTNTBundle:
        sfxPlay3DSound(pSprite, 450, 0, 0);
        evPost(nThing, 3, 0, kCallbackFXDynPuff);
        break;
    case kThingArmedSpray:
        evPost(nThing, 3, 0, kCallbackFXDynPuff);
        break;
    }
    return pSprite;
}

spritetype * actFireThing(spritetype *pSprite, int a2, int a3, int a4, int thingType, int a6)
{
    dassert(thingType >= kThingBase && thingType < kThingMax);
    int x = pSprite->x+mulscale30(a2, Cos(pSprite->ang+512));
    int y = pSprite->y+mulscale30(a2, Sin(pSprite->ang+512));
    int z = pSprite->z+a3;
    x += mulscale28(pSprite->clipdist, Cos(pSprite->ang));
    y += mulscale28(pSprite->clipdist, Sin(pSprite->ang));
    if (HitScan(pSprite, z, x-pSprite->x, y-pSprite->y, 0, CLIPMASK0, pSprite->clipdist) != -1)
    {
        x = gHitInfo.hitx-mulscale28(pSprite->clipdist<<1, Cos(pSprite->ang));
        y = gHitInfo.hity-mulscale28(pSprite->clipdist<<1, Sin(pSprite->ang));
    }
    spritetype *pThing = actSpawnThing(pSprite->sectnum, x, y, z, thingType);
    actPropagateSpriteOwner(pThing, pSprite);
    pThing->ang = pSprite->ang;
    xvel[pThing->index] = mulscale30(a6, Cos(pThing->ang));
    yvel[pThing->index] = mulscale30(a6, Sin(pThing->ang));
    zvel[pThing->index] = mulscale(a6, a4, 14);
    xvel[pThing->index] += xvel[pSprite->index]/2;
    yvel[pThing->index] += yvel[pSprite->index]/2;
    zvel[pThing->index] += zvel[pSprite->index]/2;
    return pThing;
}

spritetype* actFireMissile(spritetype *pSprite, int a2, int a3, int a4, int a5, int a6, int nType)
{
    
    dassert(nType >= kMissileBase && nType < kMissileMax);
    char v4 = 0;
    int nSprite = pSprite->index;
    MissileType *pMissileInfo = &missileInfo[nType-kMissileBase];
    int x = pSprite->x+mulscale30(a2, Cos(pSprite->ang+512));
    int y = pSprite->y+mulscale30(a2, Sin(pSprite->ang+512));
    int z = pSprite->z+a3;
    int clipdist = pMissileInfo->clipDist+pSprite->clipdist;
    x += mulscale28(clipdist, Cos(pSprite->ang));
    y += mulscale28(clipdist, Sin(pSprite->ang));
    int hit = HitScan(pSprite, z, x-pSprite->x, y-pSprite->y, 0, CLIPMASK0, clipdist);
    if (hit != -1)
    {
        if (hit == 3 || hit == 0)
        {
            v4 = 1;
            x = gHitInfo.hitx-mulscale30(Cos(pSprite->ang), 16);
            y = gHitInfo.hity-mulscale30(Sin(pSprite->ang), 16);
        }
        else
        {
            x = gHitInfo.hitx-mulscale28(pMissileInfo->clipDist<<1, Cos(pSprite->ang));
            y = gHitInfo.hity-mulscale28(pMissileInfo->clipDist<<1, Sin(pSprite->ang));
        }
    }
    spritetype *pMissile = actSpawnSprite(pSprite->sectnum, x, y, z, 5, 1);
    int nMissile = pMissile->index;
    SetBitString(show2dsprite, nMissile);
    pMissile->type = nType;
    pMissile->shade = pMissileInfo->shade;
    pMissile->pal = 0;
    pMissile->clipdist = pMissileInfo->clipDist;
    pMissile->flags = 1;
    pMissile->xrepeat = pMissileInfo->xrepeat;
    pMissile->yrepeat = pMissileInfo->yrepeat;
    pMissile->picnum = pMissileInfo->picnum;
    pMissile->ang = (pSprite->ang+pMissileInfo->angleOfs)&2047;
    xvel[nMissile] = mulscale(pMissileInfo->velocity, a4, 14);
    yvel[nMissile] = mulscale(pMissileInfo->velocity, a5, 14);
    zvel[nMissile] = mulscale(pMissileInfo->velocity, a6, 14);
    actPropagateSpriteOwner(pMissile, pSprite);
    pMissile->cstat |= 1;
    int nXSprite = pMissile->extra;
    dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
    xsprite[nXSprite].target = -1;
    evPost(nMissile, 3, 600, kCallbackRemove);
   
    actBuildMissile(pMissile, nXSprite, nSprite);
    
    if (v4)
    {
        actImpactMissile(pMissile, hit);
        pMissile = NULL;
    }
    return pMissile;
}

void actBuildMissile(spritetype* pMissile, int nXSprite, int nSprite) {
    int nMissile = pMissile->index;
    switch (pMissile->type) {
        case kMissileLifeLeechRegular:
            evPost(nMissile, 3, 0, kCallbackFXFlameLick);
            break;
        case kMissileTeslaAlt:
            evPost(nMissile, 3, 0, kCallbackFXTeslaAlt);
            break;
        case kMissilePukeGreen:
            seqSpawn(29, 3, nXSprite, -1);
            break;
        case kMissileButcherKnife:
            pMissile->cstat |= 16;
            break;
        case kMissileTeslaRegular:
            sfxPlay3DSound(pMissile, 251, 0, 0);
            break;
        case kMissileEctoSkull:
            seqSpawn(2, 3, nXSprite, -1);
            sfxPlay3DSound(pMissile, 493, 0, 0);
            break;
        case kMissileFireballNapam:
            seqSpawn(61, 3, nXSprite, nNapalmClient);
            sfxPlay3DSound(pMissile, 441, 0, 0);
            break;
        case kMissileFireball:
            seqSpawn(22, 3, nXSprite, nFireballClient);
            sfxPlay3DSound(pMissile, 441, 0, 0);
            break;
        case kMissileFlameHound:
            seqSpawn(27, 3, nXSprite, -1);
            xvel[nMissile] += xvel[nSprite] / 2 + Random2(0x11111);
            yvel[nMissile] += yvel[nSprite] / 2 + Random2(0x11111);
            zvel[nMissile] += zvel[nSprite] / 2 + Random2(0x11111);
            break;
        case kMissileFireballCerberus:
            seqSpawn(61, 3, nXSprite, dword_2192E0);
            sfxPlay3DSound(pMissile, 441, 0, 0);
            break;
        case kMissileFireballTchernobog:
            seqSpawn(23, 3, nXSprite, dword_2192D8);
            xvel[nMissile] += xvel[nSprite] / 2 + Random2(0x11111);
            yvel[nMissile] += yvel[nSprite] / 2 + Random2(0x11111);
            zvel[nMissile] += zvel[nSprite] / 2 + Random2(0x11111);
            break;
        case kMissileFlameSpray:
            if (Chance(0x8000))
                seqSpawn(0, 3, nXSprite, -1);
            else
                seqSpawn(1, 3, nXSprite, -1);
            xvel[nMissile] += xvel[nSprite] + Random2(0x11111);
            yvel[nMissile] += yvel[nSprite] + Random2(0x11111);
            zvel[nMissile] += zvel[nSprite] + Random2(0x11111);
            break;
        case kMissileFlareAlt:
            evPost(nMissile, 3, 30, kCallbackFXFlareBurst);
            evPost(nMissile, 3, 0, kCallbackFXFlareSpark);
            sfxPlay3DSound(pMissile, 422, 0, 0);
            break;
        case kMissileFlareRegular:
            evPost(nMissile, 3, 0, kCallbackFXFlareSpark);
            sfxPlay3DSound(pMissile, 422, 0, 0);
            break;
        case kMissileLifeLeechAltSmall:
            evPost(nMissile, 3, 0, kCallbackFXArcSpark);
            break;
        case kMissileArcGargoyle:
            sfxPlay3DSound(pMissile, 252, 0, 0);
            break;
    }
}

int actGetRespawnTime(spritetype *pSprite) {
    if (pSprite->extra <= 0) return -1; 
    XSPRITE *pXSprite = &xsprite[pSprite->extra];
    if (IsDudeSprite(pSprite) && !IsPlayerSprite(pSprite)) {
        if (pXSprite->respawn == 2 || (pXSprite->respawn != 1 && gGameOptions.nMonsterSettings == 2)) 
            return gGameOptions.nMonsterRespawnTime;
        return -1;
    }

    if (IsWeaponSprite(pSprite)) {
        if (pXSprite->respawn == 3 || gGameOptions.nWeaponSettings == 1) return 0;
        else if (pXSprite->respawn != 1 && gGameOptions.nWeaponSettings != 0) 
            return gGameOptions.nWeaponRespawnTime;
        return -1;
    }

    if (IsAmmoSprite(pSprite)) {
        if (pXSprite->respawn == 2 || (pXSprite->respawn != 1 && gGameOptions.nWeaponSettings != 0)) 
            return gGameOptions.nWeaponRespawnTime;
        return -1;
    }

    if (IsItemSprite(pSprite)) {
        if (pXSprite->respawn == 3 && gGameOptions.nGameType == 1) return 0;
        else if (pXSprite->respawn == 2 || (pXSprite->respawn != 1 && gGameOptions.nItemSettings != 0)) {
            switch (pSprite->type) {
                case kItemShadowCloak:
                case kItemTwoGuns:
                case kItemReflectShots:
                    return gGameOptions.nSpecialRespawnTime;
                case kItemDeathMask:
                    return gGameOptions.nSpecialRespawnTime<<1;
                default:
                    return gGameOptions.nItemRespawnTime;
            }
        }
        return -1;
    }
    return -1;
}

bool actCheckRespawn(spritetype *pSprite)
{
    int nSprite = pSprite->index;
    int nXSprite = pSprite->extra;
    if (nXSprite > 0)
    {
        XSPRITE *pXSprite = &xsprite[nXSprite];
        int nRespawnTime = actGetRespawnTime(pSprite);
        if (nRespawnTime < 0)
            return 0;
        pXSprite->respawnPending = 1;
        if (pSprite->type >= kThingBase && pSprite->type < kThingMax)
        {
            pXSprite->respawnPending = 3;
            if (pSprite->type == kThingTNTBarrel)
                pSprite->cstat |= 32768;
        }
        if (nRespawnTime > 0)
        {
            if (pXSprite->respawnPending == 1)
                nRespawnTime = mulscale16(nRespawnTime, 0xa000);
            pSprite->owner = pSprite->statnum;
            actPostSprite(pSprite->index, kStatRespawn);
            pSprite->flags |= kHitagRespawn;
            if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax))
            {
                pSprite->cstat &= ~257;
                pSprite->x = baseSprite[nSprite].x;
                pSprite->y = baseSprite[nSprite].y;
                pSprite->z = baseSprite[nSprite].z;
            }
            evPost(nSprite, 3, nRespawnTime, kCallbackRespawn);
        }
        return 1;
    }
    return  0;
}

bool actCanSplatWall(int nWall)
{
    dassert(nWall >= 0 && nWall < kMaxWalls);
    walltype *pWall = &wall[nWall];
    if (pWall->cstat & 16384)
        return 0;
    if (pWall->cstat & 32768)
        return 0;
    int nType = GetWallType(nWall);
    if (nType >= kWallBase && nType < kWallMax)
        return 0;
    if (pWall->nextsector != -1)
    {
        sectortype *pSector = &sector[pWall->nextsector];
        if (pSector->type >= kSectorBase && pSector->type < kSectorMax)
            return 0;
    }
    return 1;
}

void actFireVector(spritetype *pShooter, int a2, int a3, int a4, int a5, int a6, VECTOR_TYPE vectorType)
{
    int nShooter = pShooter->index;
    dassert(vectorType >= 0 && vectorType < kVectorMax);
    VECTORDATA *pVectorData = &gVectorData[vectorType];
    int nRange = pVectorData->maxDist;
    int hit = VectorScan(pShooter, a2, a3, a4, a5, a6, nRange, 1);
    if (hit == 3)
    {
        int nSprite = gHitInfo.hitsprite;
        dassert(nSprite >= 0 && nSprite < kMaxSprites);
        spritetype *pSprite = &sprite[nSprite];
        if (!gGameOptions.bFriendlyFire && IsTargetTeammate(pShooter, pSprite)) return;
        if (IsPlayerSprite(pSprite)) {
            PLAYER *pPlayer = &gPlayer[pSprite->type-kDudePlayer1];
            if (powerupCheck(pPlayer, kPwUpReflectShots))
            {
                gHitInfo.hitsprite = nShooter;
                gHitInfo.hitx = pShooter->x;
                gHitInfo.hity = pShooter->y;
                gHitInfo.hitz = pShooter->z;
            }
        }
    }
    int x = gHitInfo.hitx-mulscale(a4, 16, 14);
    int y = gHitInfo.hity-mulscale(a5, 16, 14);
    int z = gHitInfo.hitz-mulscale(a6, 256, 14);
    short nSector = gHitInfo.hitsect;
    char nSurf = kSurfNone;
    if (nRange == 0 || approxDist(gHitInfo.hitx-pShooter->x, gHitInfo.hity-pShooter->y) < nRange)
    {
        switch (hit)
        {
        case 1:
        {
            int nSector = gHitInfo.hitsect;
            if (sector[nSector].ceilingstat&1)
                nSurf = kSurfNone;
            else
                nSurf = surfType[sector[nSector].ceilingpicnum];
            break;
        }
        case 2:
        {
            int nSector = gHitInfo.hitsect;
            if (sector[nSector].floorstat&1)
                nSurf = kSurfNone;
            else
                nSurf = surfType[sector[nSector].floorpicnum];
            break;
        }
        case 0:
        {
            int nWall = gHitInfo.hitwall;
            dassert(nWall >= 0 && nWall < kMaxWalls);
            nSurf = surfType[wall[nWall].picnum];
            if (actCanSplatWall(nWall))
            {
                int x = gHitInfo.hitx-mulscale(a4, 16, 14);
                int y = gHitInfo.hity-mulscale(a5, 16, 14);
                int z = gHitInfo.hitz-mulscale(a6, 256, 14);
                int nSurf = surfType[wall[nWall].picnum];
                dassert(nSurf < kSurfMax);
                if (pVectorData->surfHit[nSurf].fx1 >= 0)
                {
                    spritetype *pFX = gFX.fxSpawn(pVectorData->surfHit[nSurf].fx1, nSector, x, y, z, 0);
                    if (pFX)
                    {
                        pFX->ang = (GetWallAngle(nWall)+512)&2047;
                        pFX->cstat |= 16;
                    }
                }
            }
            break;
        }
        case 4:
        {
            int nWall = gHitInfo.hitwall;
            dassert(nWall >= 0 && nWall < kMaxWalls);
            nSurf = surfType[wall[nWall].overpicnum];
            int nXWall = wall[nWall].extra;
            if (nXWall > 0)
            {
                XWALL *pXWall = &xwall[nXWall];
                if (pXWall->triggerVector)
                    trTriggerWall(nWall, pXWall, kCmdWallImpact, nShooter);
            }
            break;
        }
        case 3:
        {
            int nSprite = gHitInfo.hitsprite;
            nSurf = surfType[sprite[nSprite].picnum];
            dassert(nSprite >= 0 && nSprite < kMaxSprites);
            spritetype *pSprite = &sprite[nSprite];
            x -= mulscale(a4, 112, 14);
            y -= mulscale(a5, 112, 14);
            z -= mulscale(a6, 112<<4, 14);
            int shift = 4;
            if (vectorType == VECTOR_TYPE_0 && !IsPlayerSprite(pSprite))
                shift = 3;
            actDamageSprite(nShooter, pSprite, pVectorData->dmgType, pVectorData->dmg<<shift);
            int nXSprite = pSprite->extra;
            if (nXSprite > 0)
            {
                XSPRITE *pXSprite = &xsprite[nXSprite];
                if (pXSprite->Vector)
                    trTriggerSprite(nSprite, pXSprite, kCmdSpriteImpact, nShooter);
            }
            if (pSprite->statnum == kStatThing)
            {
                int t = thingInfo[pSprite->type-kThingBase].mass;
                if (t > 0 && pVectorData->impulse)
                {
                    int t2 = divscale(pVectorData->impulse, t, 8);
                    xvel[nSprite] += mulscale16(a4, t2);
                    yvel[nSprite] += mulscale16(a5, t2);
                    zvel[nSprite] += mulscale16(a6, t2);
                }
                if (pVectorData->burnTime)
                {
                    XSPRITE *pXSprite = &xsprite[nXSprite];
                    if (!pXSprite->burnTime)
                        evPost(nSprite, 3, 0, kCallbackFXFlameLick);
                    actBurnSprite(actSpriteIdToOwnerId(nShooter), pXSprite, pVectorData->burnTime);
                }
            }
            if (pSprite->statnum == kStatDude)
            {
                int t = pSprite->type == kThingBloodChunks ? 0 : dudeInfo[pSprite->type-kDudeBase].mass;
                
                if (IsDudeSprite(pSprite)) {
                    switch (pSprite->type) {
                        case kDudeModernCustom:
                        case kDudeModernCustomBurning:
                            t = getSpriteMassBySize(pSprite);
                            break;
                    }
                }

                if (t > 0 && pVectorData->impulse)
                {
                    int t2 = divscale(pVectorData->impulse, t, 8);
                    xvel[nSprite] += mulscale16(a4, t2);
                    yvel[nSprite] += mulscale16(a5, t2);
                    zvel[nSprite] += mulscale16(a6, t2);
                }
                if (pVectorData->burnTime)
                {
                    XSPRITE *pXSprite = &xsprite[nXSprite];
                    if (!pXSprite->burnTime)
                        evPost(nSprite, 3, 0, kCallbackFXFlameLick);
                    actBurnSprite(actSpriteIdToOwnerId(nShooter), pXSprite, pVectorData->burnTime);
                }
                if (Chance(pVectorData->fxChance))
                {
                    int t = gVectorData[19].maxDist;
                    a4 += Random3(4000);
                    a5 += Random3(4000);
                    a6 += Random3(4000);
                    if (HitScan(pSprite, gHitInfo.hitz, a4, a5, a6, CLIPMASK1, t) == 0)
                    {
                        if (approxDist(gHitInfo.hitx-pSprite->x, gHitInfo.hity-pSprite->y) <= t)
                        {
                            int nWall = gHitInfo.hitwall;
                            int nSector = gHitInfo.hitsect;
                            if (actCanSplatWall(nWall))
                            {
                                int x = gHitInfo.hitx - mulscale(a4, 16, 14);
                                int y = gHitInfo.hity - mulscale(a5, 16, 14);
                                int z = gHitInfo.hitz - mulscale(a6, 16<<4, 14);
                                int nSurf = surfType[wall[nWall].picnum];
                                VECTORDATA *pVectorData = &gVectorData[19];
                                FX_ID t2 = pVectorData->surfHit[nSurf].fx2;
                                FX_ID t3 = pVectorData->surfHit[nSurf].fx3;
                                spritetype *pFX = NULL;
                                if (t2 > FX_NONE && (t3 == FX_NONE || Chance(0x4000)))
                                    pFX = gFX.fxSpawn(t2, nSector, x, y, z, 0);
                                else if(t3 > FX_NONE)
                                    pFX = gFX.fxSpawn(t3, nSector, x, y, z, 0);
                                if (pFX)
                                {
                                    zvel[pFX->index] = 0x2222;
                                    pFX->ang = (GetWallAngle(nWall)+512)&2047;
                                    pFX->cstat |= 16;
                                }
                            }
                        }
                    }
                }
                for (int i = 0; i < pVectorData->bloodSplats; i++)
                    if (Chance(pVectorData->splatChance))
                        fxSpawnBlood(pSprite, pVectorData->dmg<<4);
            }

            // by NoOne: add impulse for sprites from physics list
            if (gPhysSpritesCount > 0 && pVectorData->impulse) {
                int nIndex = isDebris(pSprite->index);
                if (nIndex != -1 && (xsprite[pSprite->extra].physAttr & kPhysDebrisVector)) {
                    int impulse = divscale(pVectorData->impulse, ClipLow(gSpriteMass[pSprite->extra].mass, 10), 6);
                    xvel[nSprite] += mulscale16(a4, impulse);
                    yvel[nSprite] += mulscale16(a5, impulse);
                    zvel[nSprite] += mulscale16(a6, impulse);

                    if (pVectorData->burnTime != 0) {
                        if (!xsprite[nXSprite].burnTime) evPost(nSprite, 3, 0, kCallbackFXFlameLick);
                        actBurnSprite(actSpriteIdToOwnerId(nShooter), &xsprite[nXSprite], pVectorData->burnTime);
                    }

                    if (pSprite->type >= kThingBase && pSprite->type < kThingMax)
                        changespritestat(pSprite->index, kStatThing);
                        //actPostSprite(pSprite->index, kStatThing); // if it was a thing, return it's statnum back
                }
            }

            break;
        }
        }
    }
    dassert(nSurf < kSurfMax);
    if (pVectorData->surfHit[nSurf].fx2 >= 0)
        gFX.fxSpawn(pVectorData->surfHit[nSurf].fx2, nSector, x, y, z, 0);
    if (pVectorData->surfHit[nSurf].fx3 >= 0)
        gFX.fxSpawn(pVectorData->surfHit[nSurf].fx3, nSector, x, y, z, 0);
    if (pVectorData->surfHit[nSurf].fxSnd >= 0)
        sfxPlay3DSound(x, y, z, pVectorData->surfHit[nSurf].fxSnd, nSector);
}

void FireballSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    spritetype *pFX = gFX.fxSpawn(FX_11, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        int nFX = pFX->index;
        xvel[nFX] = xvel[nSprite];
        yvel[nFX] = yvel[nSprite];
        zvel[nFX] = zvel[nSprite];
    }
}

void NapalmSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    spritetype *pFX = gFX.fxSpawn(FX_12, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        int nFX = pFX->index;
        xvel[nFX] = xvel[nSprite];
        yvel[nFX] = yvel[nSprite];
        zvel[nFX] = zvel[nSprite];
    }
}

void sub_3888C(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    spritetype *pFX = gFX.fxSpawn(FX_32, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        int nFX = pFX->index;
        xvel[nFX] = xvel[nSprite];
        yvel[nFX] = yvel[nSprite];
        zvel[nFX] = zvel[nSprite];
    }
}

void sub_38938(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    spritetype *pFX = gFX.fxSpawn(FX_33, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pFX)
    {
        int nFX = pFX->index;
        xvel[nFX] = xvel[nSprite];
        yvel[nFX] = yvel[nSprite];
        zvel[nFX] = zvel[nSprite];
    }
}

void TreeToGibCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    pSprite->type = kThingObjectExplode;
    pXSprite->state = 1;
    pXSprite->data1 = 15;
    pXSprite->data2 = 0;
    pXSprite->data3 = 0;
    pXSprite->health = thingInfo[17].startHealth;
    pXSprite->data4 = 312;
    pSprite->cstat |= 257;
}

void DudeToGibCallback1(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    pSprite->type = kThingBloodChunks;
    pXSprite->data1 = 8;
    pXSprite->data2 = 0;
    pXSprite->data3 = 0;
    pXSprite->health = thingInfo[26].startHealth;
    pXSprite->data4 = 319;
    pXSprite->triggerOnce = 0;
    pXSprite->isTriggered = 0;
    pXSprite->locked = 0;
    pXSprite->targetX = (int)gFrameClock;
    pXSprite->state = 1;
}

void DudeToGibCallback2(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    pSprite->type = kThingBloodChunks;
    pXSprite->data1 = 3;
    pXSprite->data2 = 0;
    pXSprite->data3 = 0;
    pXSprite->health = thingInfo[26].startHealth;
    pXSprite->data4 = 319;
    pXSprite->triggerOnce = 0;
    pXSprite->isTriggered = 0;
    pXSprite->locked = 0;
    pXSprite->targetX = (int)gFrameClock;
    pXSprite->state = 1;
}

void actPostSprite(int nSprite, int nStatus)
{
    int n;
    dassert(gPostCount < kMaxSprites);
    dassert(nSprite < kMaxSprites && sprite[nSprite].statnum < kMaxStatus);
    dassert(nStatus >= 0 && nStatus <= kStatFree);
    if (sprite[nSprite].flags&32)
    {
        for (n = 0; n < gPostCount; n++)
            if (gPost[n].at0 == nSprite)
                break;
        dassert(n < gPostCount);
    }
    else
    {
        n = gPostCount;
        sprite[nSprite].flags |= 32;
        gPostCount++;
    }
    gPost[n].at0 = nSprite;
    gPost[n].at2 = nStatus;
}

void actPostProcess(void)
{
    for (int i = 0; i < gPostCount; i++)
    {
        POSTPONE *pPost = &gPost[i];
        int nSprite = pPost->at0;
        spritetype *pSprite = &sprite[nSprite];
        pSprite->flags &= ~32;
        int nStatus = pPost->at2;
        if (nStatus == kStatFree)
        {
            evKill(nSprite, 3);
            if (sprite[nSprite].extra > 0)
                seqKill(3, sprite[nSprite].extra);
            DeleteSprite(nSprite);
        }
        else
            ChangeSpriteStat(nSprite, nStatus);
    }
    gPostCount = 0;
}

void MakeSplash(spritetype *pSprite, XSPRITE *pXSprite)
{
    UNREFERENCED_PARAMETER(pXSprite);
    pSprite->flags &= ~2;
    int nXSprite = pSprite->extra;
    pSprite->z -= 4 << 8;
    int nSurface = tileGetSurfType(gSpriteHit[nXSprite].florhit);
    switch (pSprite->type) {
        case kThingDripWater:
            switch (nSurface) {
                case kSurfWater:
                    seqSpawn(6, 3, nXSprite, -1);
                    sfxPlay3DSound(pSprite, 356, -1, 0);
                    break;
                default:
                    seqSpawn(7, 3, nXSprite, -1);
                    sfxPlay3DSound(pSprite, 354, -1, 0);
                    break;
            }
            break;
        case kThingDripBlood:
            seqSpawn(8, 3, nXSprite, -1);
            sfxPlay3DSound(pSprite, 354, -1, 0);
            break;
    }
}

class ActorLoadSave : public LoadSave
{
    virtual void Load(void);
    virtual void Save(void);
};

void ActorLoadSave::Load(void)
{
    Read(gSpriteHit, sizeof(gSpriteHit));
    Read(gAffectedSectors, sizeof(gAffectedSectors));
    Read(gAffectedXWalls, sizeof(gAffectedXWalls));
    Read(&gPostCount, sizeof(gPostCount));
    Read(gPost, sizeof(gPost));
    actInit(true);
}

void ActorLoadSave::Save(void)
{
    Write(gSpriteHit, sizeof(gSpriteHit));
    Write(gAffectedSectors, sizeof(gAffectedSectors));
    Write(gAffectedXWalls, sizeof(gAffectedXWalls));
    Write(&gPostCount, sizeof(gPostCount));
    Write(gPost, sizeof(gPost));
}

static ActorLoadSave *myLoadSave;

void ActorLoadSaveConstruct(void)
{
    myLoadSave = new ActorLoadSave();
}


// By NoOne: The following functions required for random event features
//-------------------------
int GetDataVal(spritetype* pSprite, int data) {
    if (pSprite->extra >= 0) {
        switch (data) {
        case 0:
            return xsprite[pSprite->extra].data1;
        case 1:
            return xsprite[pSprite->extra].data2;
        case 2:
            return xsprite[pSprite->extra].data3;
        case 3:
            return xsprite[pSprite->extra].data4;
        }
    }
    return -1;
}

// tries to get random data field of sprite
int GetRandDataVal(XSPRITE* pXSprite, int randType) {
    if (pXSprite == NULL) return -1;
    int random = 0; int bad = 0; int maxRetries = 10;

    int rData[4];
    rData[0] = pXSprite->data1; rData[2] = pXSprite->data3;
    rData[1] = pXSprite->data2; rData[3] = pXSprite->data4;
    // randomize only in case if at least 2 data fields fits.
    for (int i = 0; i < 4; i++) {
        switch (randType) {
            case kRandomizeItem:
                if (rData[i] >= kItemWeaponBase && rData[i] < kItemMax) break;
                else bad++;
                break;
            case kRandomizeDude:
                if (rData[i] >= kDudeBase && rData[i] < kDudeMax) break;
                else bad++;
                break;
            case kRandomizeTX:
                if (rData[i] > kChannelZero && rData[i] < kChannelUserMax) break;
                else bad++;
                break;
            default:
                bad++;
                break;
        }
    }
    
    if (bad < 3) {
    // try randomize few times
    while (maxRetries > 0) {
        // use true random only for single player mode, otherwise use Blood's default one.
        random = (gGameOptions.nGameType == 0 && !VanillaMode() && !DemoRecordStatus()) ? STD_Random(0, 3) : Random(3);
            if (rData[random] > 0) return rData[random];
            maxRetries--;
    }
    }

    return -1;
}

// this function drops random item using random pickup generator(s)
spritetype* DropRandomPickupObject(spritetype* pSource, short prevItem) {
    spritetype* pSprite2 = NULL; int selected = -1; int maxRetries = 9;
    if (xspriRangeIsFine(pSource->extra)) {
        XSPRITE* pXSource = &xsprite[pSource->extra];
        while ((selected = GetRandDataVal(pXSource, kRandomizeItem)) == prevItem) if (maxRetries-- <= 0) break;
    if (selected > 0) {
            pSprite2 = actDropObject(pSource, selected);
        if (pSprite2 != NULL) {

            pXSource->dropMsg = pSprite2->type; // store dropped item type in dropMsg
            pSprite2->x = pSource->x;
            pSprite2->y = pSource->y;
            pSprite2->z = pSource->z;

            if ((pSource->flags & kModernTypeFlag1) && (pXSource->txID > 0 || (pXSource->txID != 3 && pXSource->lockMsg > 0)) &&
                dbInsertXSprite(pSprite2->xvel) > 0) {

                    XSPRITE* pXSprite2 = &xsprite[pSprite2->extra];

                // inherit spawn sprite trigger settings, so designer can send command when item picked up.
                pXSprite2->txID = pXSource->txID;
                pXSprite2->command = pXSource->command;
                pXSprite2->triggerOn = pXSource->triggerOn;
                pXSprite2->triggerOff = pXSource->triggerOff;

                pXSprite2->Pickup = true;

            }
        }
    }
    }
    return pSprite2;
}

// this function spawns random dude using dudeSpawn
spritetype* spawnRandomDude(spritetype* pSource) {
    spritetype* pSprite2 = NULL; int selected = -1;
    if (xspriRangeIsFine(pSource->extra)) {
        XSPRITE* pXSource = &xsprite[pSource->extra];
        if ((selected = GetRandDataVal(pXSource, kRandomizeDude)) > 0)
            pSprite2 = actSpawnDude(pSource, selected, -1, 0);
    }
    return pSprite2;
}
//-------------------------

// By NoOne: this function plays sound predefined in missile info
bool sfxPlayMissileSound(spritetype* pSprite, int missileId) {
    MissileType* pMissType = &missileInfo[missileId - kMissileBase];
    if (Chance(0x4000))
        sfxPlay3DSound(pSprite, pMissType->fireSound[0], -1, 0);
    else
        sfxPlay3DSound(pSprite, pMissType->fireSound[1], -1, 0);

    return true;
}

// By NoOne: this function plays sound predefined in vector info
bool sfxPlayVectorSound(spritetype* pSprite, int vectorId) {
    VECTORDATA* pVectorData = &gVectorData[vectorId];
    if (Chance(0x4000))
        sfxPlay3DSound(pSprite, pVectorData->fireSound[0], -1, 0);
    else
        sfxPlay3DSound(pSprite, pVectorData->fireSound[1], -1, 0);

    return true;
}

// By NoOne: this function allows to spawn new custom dude and inherit spawner settings,
// so custom dude can have different weapons, hp and so on...
spritetype* actSpawnCustomDude(spritetype* pSprite, int nDist) {

    spritetype* pSource = pSprite; XSPRITE* pXSource = &xsprite[pSource->extra];
    spritetype* pDude = actSpawnSprite(pSprite,6); XSPRITE* pXDude = &xsprite[pDude->extra];

    int x, y, z = pSprite->z, nAngle = pSprite->ang, nType = kDudeModernCustom;

    if (nDist > 0) {
        x = pSprite->x + mulscale30r(Cos(nAngle), nDist);
        y = pSprite->y + mulscale30r(Sin(nAngle), nDist);
    }
    else {
        x = pSprite->x;
        y = pSprite->y;
    }

    pDude->type = nType; pDude->ang = nAngle;
    vec3_t pos = { x, y, z }; setsprite(pDude->index, &pos); 
    pDude->cstat |= 0x1101; pDude->clipdist = dudeInfo[nType - kDudeBase].clipdist;

    // inherit weapon, seq and sound settings.
    pXDude->data1 = pXSource->data1;
    pXDude->data2 = pXSource->data2;
    pXDude->sysData1 = pXSource->data3; // move sndStartId from data3 to sysData1
    pXDude->data3 = 0;

    // spawn seq
    seqSpawn(genDudeSeqStartId(pXDude), 3, pDude->extra, -1);

    // inherit movement speed.
    pXDude->busyTime = pXSource->busyTime;

    // inherit clipdist?
    if (pSource->clipdist > 0) pDude->clipdist = pSource->clipdist;

    // inherit custom hp settings
    if (pXSource->data4 <= 0) pXDude->health = dudeInfo[nType - kDudeBase].startHealth << 4;
    else pXDude->health = ClipRange(pXSource->data4 << 4, 1, 65535);


    if (pSource->flags & kModernTypeFlag1) {
        switch (pSource->type) {
            case kModernCustomDudeSpawn:
            //inherit pal?
            if (pDude->pal <= 0) pDude->pal = pSource->pal;

            // inherit spawn sprite trigger settings, so designer can count monsters.
            pXDude->txID = pXSource->txID;
            pXDude->command = pXSource->command;
            pXDude->triggerOn = pXSource->triggerOn;
            pXDude->triggerOff = pXSource->triggerOff;

            // inherit drop items
            pXDude->dropMsg = pXSource->dropMsg;

            // inherit required key so it can be dropped
            pXDude->key = pXSource->key;

            // inherit dude flags
            pXDude->dudeDeaf = pXSource->dudeDeaf;
            pXDude->dudeGuard = pXSource->dudeGuard;
            pXDude->dudeAmbush = pXSource->dudeAmbush;
            pXDude->dudeFlag4 = pXSource->dudeFlag4;
            break;
        }
    }

    // inherit sprite size (useful for seqs with zero repeats)
    if (pSource->flags & kModernTypeFlag2) {
        pDude->xrepeat = pSource->xrepeat;
        pDude->yrepeat = pSource->yrepeat;
    }

    aiInitSprite(pDude);
    return pDude;
}

int getSpriteMassBySize(spritetype* pSprite) {
    int mass = 0; int seqId = -1; int clipDist = pSprite->clipdist; Seq* pSeq = NULL;
    if (IsDudeSprite(pSprite)) {

        switch (pSprite->type) {
            case kDudePodMother: // fake dude, no seq
                break;
            case kDudeModernCustom:
            case kDudeModernCustomBurning:
                seqId = xsprite[pSprite->extra].data2;
                clipDist = gGenDudeExtra[pSprite->index].initVals[2];
                break;
            default:
                seqId = dudeInfo[pSprite->type - kDudeBase].seqStartID;
                break;
        }

    } else if (pSprite->extra >= 0) {
        
        seqId = seqGetID(3, pSprite->extra);

    }
    
    SPRITEMASS* cached = &gSpriteMass[pSprite->extra];
    if (((seqId >= 0 && seqId == cached->seqId) || pSprite->picnum == cached->picnum) && pSprite->xrepeat == cached->xrepeat &&
        pSprite->yrepeat == cached->yrepeat && clipDist == cached->clipdist) {
        return cached->mass;
    }

    short picnum = pSprite->picnum;
    short massDiv = 30;  short addMul = 2; short subMul = 2;

    if (seqId >= 0) {
        DICTNODE* hSeq = gSysRes.Lookup(seqId, "SEQ");
        if (hSeq)
        {
            pSeq = (Seq*)gSysRes.Load(hSeq);
            picnum = seqGetTile(&pSeq->frames[0]);
        }
        else
            picnum = pSprite->picnum;
    }

    clipDist = ClipLow(pSprite->clipdist, 1);
    short x = tilesiz[picnum].x;        short y = tilesiz[picnum].y;
    short xrepeat = pSprite->xrepeat; 	short yrepeat = pSprite->yrepeat;

    // take surface type into account
    switch (tileGetSurfType(pSprite->xvel + 0xc000)) {
        case 1:  massDiv = 16; break; // stone
        case 2:  massDiv = 18; break; // metal
        case 3:  massDiv = 21; break; // wood
        case 4:  massDiv = 25; break; // flesh
        case 5:  massDiv = 28; break; // water
        case 6:  massDiv = 26; break; // dirt
        case 7:  massDiv = 27; break; // clay
        case 8:  massDiv = 35; break; // snow
        case 9:  massDiv = 22; break; // ice
        case 10: massDiv = 37; break; // leaves
        case 11: massDiv = 33; break; // cloth
        case 12: massDiv = 36; break; // plant
        case 13: massDiv = 24; break; // goo
        case 14: massDiv = 23; break; // lava
    }

    mass = ((x + y) * (clipDist / 2)) / massDiv;

    if (xrepeat > 64) mass += ((xrepeat - 64) * addMul);
    else if (xrepeat < 64 && mass > 0) {
        for (int i = 64 - xrepeat; i > 0; i--) {
            if ((mass -= subMul) <= 100 && subMul-- <= 1) {
                mass -= i;
                break;
            }
        }
    }

    if (yrepeat > 64) mass += ((yrepeat - 64) * addMul);
    else if (yrepeat < 64 && mass > 0) {
        for (int i = 64 - yrepeat; i > 0; i--) {
            if ((mass -= subMul) <= 100 && subMul-- <= 1) {
                mass -= i;
                break;
            }
        }
    }

    if (mass <= 0) cached->mass = 1 + Random(10);
    else cached->mass = ClipRange(mass, 1, 65535);

    cached->airVel = ClipRange(400 - cached->mass, 32, 400);
    cached->fraction = ClipRange(60000 - (cached->mass << 7), 8192, 60000);

    cached->xrepeat = pSprite->xrepeat;             cached->yrepeat = pSprite->yrepeat;
    cached->picnum = pSprite->picnum;               cached->seqId = seqId;
    cached->clipdist = pSprite->clipdist;

    return cached->mass;
}

int isDebris(int nSprite) {
    if (sprite[nSprite].extra < 0 || xsprite[sprite[nSprite].extra].physAttr == 0) 
        return -1;

    for (int i = 0; i < gPhysSpritesCount; i++) {
        if (gPhysSpritesList[i] != nSprite) continue;
        return i;
    }

    return -1;
}

int debrisGetFreeIndex(void) {
    for (int i = 0; i < kMaxSuperXSprites; i++) {
        if (gPhysSpritesList[i] == -1 || sprite[gPhysSpritesList[i]].statnum == kStatFree) return i;

        else if ((sprite[gPhysSpritesList[i]].flags & kHitagFree) || sprite[gPhysSpritesList[i]].extra < 0) return i;
        else if (xsprite[sprite[gPhysSpritesList[i]].extra].physAttr == 0) return i;
    }

    return -1;
}

void debrisConcuss(int nOwner, int listIndex, int x, int y, int z, int dmg) {
    spritetype* pSprite = (gPhysSpritesList[listIndex] >= 0) ? &sprite[gPhysSpritesList[listIndex]] : NULL;
    if (pSprite != NULL && pSprite->extra >= 0 && pSprite->extra < kMaxXSprites) {
        int dx = pSprite->x - x; int dy = pSprite->y - y; int dz = (pSprite->z - z) >> 4;
        dmg = scale(0x40000, dmg, 0x40000 + dx * dx + dy * dy + dz * dz);

        int size = (tilesiz[pSprite->picnum].x * pSprite->xrepeat * tilesiz[pSprite->picnum].y * pSprite->yrepeat) >> 1;
        if (xsprite[pSprite->extra].physAttr & kPhysDebrisExplode) {
            if (gSpriteMass[pSprite->extra].mass > 0) {
                int t = scale(dmg, size, gSpriteMass[pSprite->extra].mass);

                xvel[pSprite->xvel] += mulscale16(t, dx);
                yvel[pSprite->xvel] += mulscale16(t, dy);
                zvel[pSprite->xvel] += mulscale16(t, dz);
            }


            if (pSprite->type >= kThingBase && pSprite->type < kThingMax)
                //actPostSprite(pSprite->index, kStatThing); // !!! (does not working here) if it was a thing, return it's statnum back
                changespritestat(pSprite->index, kStatThing);
        }


        actDamageSprite(nOwner, pSprite, DAMAGE_TYPE_3, dmg);
        return;
    }
}

void debrisMove(int listIndex) {
    if (!(sprite[gPhysSpritesList[listIndex]].extra > 0 && sprite[gPhysSpritesList[listIndex]].extra < kMaxXSprites)) {
        gPhysSpritesList[listIndex] = -1;
        return;
    }
    else if (!(sprite[gPhysSpritesList[listIndex]].sectnum >= 0 && sprite[gPhysSpritesList[listIndex]].sectnum < kMaxSectors)) {
        gPhysSpritesList[listIndex] = -1;
        return;
    }

    int nSprite = gPhysSpritesList[listIndex];
    int nXSprite = sprite[nSprite].extra;       XSPRITE* pXDebris = &xsprite[nXSprite];
    spritetype* pSprite = &sprite[nSprite];     int nSector = pSprite->sectnum;

    int top, bottom;	GetSpriteExtents(pSprite, &top, &bottom);

    int moveHit = 0;
    //int floorDist = (bottom - pSprite->z) / 4;
    //int ceilDist = (pSprite->z - top) / 4;
    //int clipDist = pSprite->clipdist << 2;

    int tmpFraction = gSpriteMass[pSprite->extra].fraction;
    if (sector[nSector].extra >= 0 && xsector[sector[nSector].extra].Underwater)
        tmpFraction >>= 1;

    if (xvel[pSprite->xvel] != 0 || yvel[pSprite->xvel] != 0) {

        short oldcstat = pSprite->cstat;
        pSprite->cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

        moveHit = gSpriteHit[nXSprite].hit = ClipMove((int*)& pSprite->x, (int*)& pSprite->y, (int*)& pSprite->z, &nSector, xvel[nSprite] >> 12,
            yvel[nSprite] >> 12, pSprite->clipdist << 2, (pSprite->z - top) / 4, (bottom - pSprite->z) / 4, CLIPMASK0);

        pSprite->cstat = oldcstat;

        dassert(nSector >= 0);

        if (pSprite->sectnum != nSector) {
            dassert(nSector >= 0 && nSector < kMaxSectors);
            ChangeSpriteSect(nSprite, nSector);
        }

        if ((gSpriteHit[nXSprite].hit & 0xc000) == 0x8000) {
            int nHitWall = gSpriteHit[nXSprite].hit & 0x3fff;
            actWallBounceVector((int*)& xvel[nSprite], (int*)& yvel[nSprite], nHitWall, tmpFraction);
        }

    }
    else {
        dassert(nSector >= 0 && nSector < kMaxSectors);
        FindSector(pSprite->x, pSprite->y, pSprite->z, &nSector);
    }

    if (zvel[nSprite])
        pSprite->z += zvel[nSprite] >> 8;

    int ceilZ, ceilHit, floorZ, floorHit;
    GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist << 2, CLIPMASK0);
    GetSpriteExtents(pSprite, &top, &bottom);

    if ((pXDebris->physAttr & kPhysGravity) && bottom < floorZ) {
        pSprite->z += 455;
        zvel[nSprite] += 58254;
    }
    int warp = CheckLink(pSprite);
    if (warp != 0) {
        GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist << 2, CLIPMASK0);
        if (!(pSprite->cstat & CSTAT_SPRITE_INVISIBLE)) {
            switch (warp) {
            case kMarkerUpWater:
            case kMarkerUpGoo:
                int pitch = (150000 - (gSpriteMass[pSprite->extra].mass << 9)) + Random3(8192);
                sfxPlay3DSoundCP(pSprite, 720, -1, 0, pitch, 75 - Random(40));
                
                if (sector[pSprite->sectnum].extra < 0 || !xsector[sector[pSprite->sectnum].extra].Underwater)
                    evKill(pSprite->xvel, 3, kCallbackEnemeyBubble);
                else {
                    if (Chance(0x8000))
                        evPost(pSprite->xvel, 3, 0, kCallbackEnemeyBubble);

                    for (int i = 2; i <= 5; i++) {
                        if (Chance(0x3000 * i))
                            evPost(pSprite->xvel, 3, 0, kCallbackEnemeyBubble);
                    }
                }
                break;
            }
        }
    }

    GetSpriteExtents(pSprite, &top, &bottom);

    if ((floorHit & 0xe000) == 0xc000) {
        if ((sprite[floorHit & 0x1fff].cstat & 0x30) == 0x20)
            if (klabs(bottom - floorZ) < 1024) floorZ -= 1024;
    }

    if (bottom >= floorZ) {

        gSpriteHit[nXSprite].florhit = floorHit;
        pSprite->z += floorZ - bottom;
        int v20 = zvel[nSprite] - velFloor[pSprite->sectnum];
        if (v20 > 0) {

            pXDebris->physAttr |= kPhysFalling;
            actFloorBounceVector((int*)& xvel[nSprite], (int*)& yvel[nSprite], (int*)& v20, pSprite->sectnum, tmpFraction);
            zvel[nSprite] = v20;

            if (velFloor[pSprite->sectnum] == 0 && klabs(zvel[nSprite]) < 0x10000) {
                zvel[nSprite] = 0;
                pXDebris->physAttr &= ~kPhysFalling;
            }

            moveHit = 0x4000 | nSector;

        }
        else if (zvel[nSprite] == 0)
            pXDebris->physAttr &= ~kPhysFalling;

    }
    else {

        gSpriteHit[nXSprite].florhit = 0;
        if (pXDebris->physAttr & kPhysGravity)
            pXDebris->physAttr |= kPhysFalling;
    }

    if (top <= ceilZ) {

        gSpriteHit[nXSprite].ceilhit = ceilHit;
        pSprite->z += ClipLow(ceilZ - top, 0);
        if (zvel[nSprite] < 0)
        {
            xvel[nSprite] = mulscale16(xvel[nSprite], 0xc000);
            yvel[nSprite] = mulscale16(yvel[nSprite], 0xc000);
            zvel[nSprite] = mulscale16(-zvel[nSprite], 0x4000);
        }

    }
    else {

        gSpriteHit[nXSprite].ceilhit = 0;

    }

    if (bottom >= floorZ) {
        int nVel = approxDist(xvel[nSprite], yvel[nSprite]);
        int nVelClipped = ClipHigh(nVel, 0x11111);

        if ((floorHit & 0xc000) == 0xc000) {
            int nHitSprite = floorHit & 0x3fff;
            if ((sprite[nHitSprite].cstat & 0x30) == 0)
            {
                xvel[nSprite] += mulscale(4, pSprite->x - sprite[nHitSprite].x, 2);
                yvel[nSprite] += mulscale(4, pSprite->y - sprite[nHitSprite].y, 2);
                moveHit = gSpriteHit[nXSprite].hit;
            }
        }
        if (nVel > 0)
        {
            int t = divscale16(nVelClipped, nVel);
            xvel[nSprite] -= mulscale16(t, xvel[nSprite]);
            yvel[nSprite] -= mulscale16(t, yvel[nSprite]);
        }
    }

    if (xvel[nSprite] || yvel[nSprite])
        pSprite->ang = getangle(xvel[nSprite], yvel[nSprite]);

    if (moveHit != 0 && pXDebris->Impact && pXDebris->locked != 1 && !pXDebris->isTriggered) {
        if (!pXDebris->Interrutable && pXDebris->state != pXDebris->restState) return;

        if (pSprite->type >= kThingBase && pSprite->type < kThingMax)
            // if thing was turned in debris, change it's stat back so it will do on impact what it supposed to do...
            //actPostSprite(nSprite, kStatThing); // !!!! not working here for some reason
            changespritestat(nSprite, kStatThing);

        
        if (pXDebris->state == 1) trTriggerSprite(pSprite->xvel, pXDebris, kCmdOff, -1);
        else trTriggerSprite(pSprite->xvel, pXDebris, kCmdOn, -1);
    }
}

bool ceilIsTooLow(spritetype* pSprite) {
    if (pSprite != NULL) {

        sectortype* pSector = &sector[pSprite->sectnum];
        int a = pSector->ceilingz - pSector->floorz;
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        int b = top - bottom;
        if (a > b) return true;
    }

    return false;
}

bool isImmune(spritetype* pSprite, int dmgType, int minScale) {

    if (dmgType >= kDmgFall && dmgType < kDmgMax && pSprite->extra >= 0 && xsprite[pSprite->extra].locked != 1) {
        if (pSprite->type >= kThingBase && pSprite->type < kThingMax)
            return (thingInfo[pSprite->type - kThingBase].dmgControl[dmgType] <= minScale);
        else if (IsDudeSprite(pSprite)) {
            if (IsPlayerSprite(pSprite)) return (gPlayer[pSprite->type - kDudePlayer1].damageControl[dmgType] <= minScale);
            else if (pSprite->type == kDudeModernCustom) return (gGenDudeExtra[pSprite->index].dmgControl[dmgType] <= minScale);
            else return (dudeInfo[pSprite->type - kDudeBase].at70[dmgType] <= minScale);
        }
    }

    return true;
}

#ifdef POLYMER

// this is the same crap as in game.c's tspr manipulation.  puke.
// XXX: may access tilesizy out-of-bounds by bad user code.
#define LIGHTRAD(spriteNum, s) (s->yrepeat * tilesiz[s->picnum].y)
#define LIGHTRAD2(spriteNum, s) ((s->yrepeat + ((rand() % s->yrepeat)>>2)) * tilesiz[s->picnum].y)

void G_AddGameLight(int lightRadius, int spriteNum, int zOffset, int lightRange, int lightColor, int lightPrio)
{
    auto const s = &sprite[spriteNum];

    if (videoGetRenderMode() != REND_POLYMER || pr_lighting != 1)
        return;

    if (gPolymerLight[spriteNum].lightptr == NULL)
    {
#pragma pack(push, 1)
        _prlight mylight;
#pragma pack(pop)
        Bmemset(&mylight, 0, sizeof(mylight));

        mylight.sector = s->sectnum;
        mylight.x = s->x;
        mylight.y = s->y;
        mylight.z = s->z - zOffset;
        mylight.color[0] = lightColor & 255;
        mylight.color[1] = (lightColor >> 8) & 255;
        mylight.color[2] = (lightColor >> 16) & 255;
        mylight.radius = lightRadius;
        gPolymerLight[spriteNum].lightmaxrange = mylight.range = lightRange;

        mylight.priority = lightPrio;
        mylight.tilenum = 0;

        mylight.publicflags.emitshadow = 1;
        mylight.publicflags.negative = 0;

        gPolymerLight[spriteNum].lightId = polymer_addlight(&mylight);
        if (gPolymerLight[spriteNum].lightId >= 0)
            gPolymerLight[spriteNum].lightptr = &prlights[gPolymerLight[spriteNum].lightId];
        return;
    }

    s->z -= zOffset;

    if (lightRange<gPolymerLight[spriteNum].lightmaxrange>> 1)
        gPolymerLight[spriteNum].lightmaxrange = 0;

    if (lightRange > gPolymerLight[spriteNum].lightmaxrange || lightPrio != gPolymerLight[spriteNum].lightptr->priority ||
        Bmemcmp(&sprite[spriteNum], gPolymerLight[spriteNum].lightptr, sizeof(int32_t) * 3))
    {
        if (lightRange > gPolymerLight[spriteNum].lightmaxrange)
            gPolymerLight[spriteNum].lightmaxrange = lightRange;

        Bmemcpy(gPolymerLight[spriteNum].lightptr, &sprite[spriteNum], sizeof(int32_t) * 3);
        gPolymerLight[spriteNum].lightptr->sector = s->sectnum;
        gPolymerLight[spriteNum].lightptr->flags.invalidate = 1;
    }

    gPolymerLight[spriteNum].lightptr->priority = lightPrio;
    gPolymerLight[spriteNum].lightptr->range = lightRange;
    gPolymerLight[spriteNum].lightptr->color[0] = lightColor & 255;
    gPolymerLight[spriteNum].lightptr->color[1] = (lightColor >> 8) & 255;
    gPolymerLight[spriteNum].lightptr->color[2] = (lightColor >> 16) & 255;

    s->z += zOffset;
}

void actDoLight(int nSprite)
{
    auto const pSprite = &sprite[nSprite];
    int savedFires = 0;
    if (((sector[pSprite->sectnum].floorz - sector[pSprite->sectnum].ceilingz) < 16) || pSprite->z > sector[pSprite->sectnum].floorz)
    {
        if (gPolymerLight[nSprite].lightptr != NULL)
            DeleteLight(nSprite);
    }
    else
    {
        if (gPolymerLight[nSprite].lightptr != NULL && gPolymerLight[nSprite].lightcount)
        {
            if (!(--gPolymerLight[nSprite].lightcount))
                DeleteLight(nSprite);
        }

        if (pr_lighting != 1)
            return;

        switch (pSprite->statnum)
        {
        case kStatProjectile:
            switch (pSprite->type)
            {
            case kMissileTeslaRegular:
                {
                    int32_t x = ((sintable[(pSprite->ang+512)&2047])>>6);
                    int32_t y = ((sintable[(pSprite->ang)&2047])>>6);

                    pSprite->x -= x;
                    pSprite->y -= y;

                    G_AddGameLight(0, nSprite, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), 2048, 80+(252<<8)+(120<<16),PR_LIGHT_PRIO_HIGH_GAME);

                    pSprite->x += x;
                    pSprite->y += y;
                }
                break;
            }
            break;
        case kStatExplosion:
            switch (pSprite->type)
            {
            default:
                if (!gPolymerLight[nSprite].lightcount)
                {
                    // XXX: This block gets CODEDUP'd too much.
                    int32_t x = ((sintable[(pSprite->ang+512)&2047])>>6);
                    int32_t y = ((sintable[(pSprite->ang)&2047])>>6);

                    pSprite->x -= x;
                    pSprite->y -= y;

                    G_AddGameLight(0, nSprite, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), LIGHTRAD(spriteNum, pSprite), 240+(160<<8)+(80<<16),
                        pSprite->yrepeat > 32 ? PR_LIGHT_PRIO_HIGH_GAME : PR_LIGHT_PRIO_LOW_GAME);

                    pSprite->x += x;
                    pSprite->y += y;
                }
                break;
            }
            break;
        }
    }
#if 0
    if (((sector[pSprite->sectnum].floorz - sector[pSprite->sectnum].ceilingz) < 16) || pSprite->z > sector[pSprite->sectnum].floorz || pSprite->z > actor[spriteNum].floorz ||
        (pSprite->picnum != SECTOREFFECTOR && ((pSprite->cstat & 32768) || pSprite->yrepeat < 4)) ||
        A_CheckSpriteFlags(spriteNum, SFLAG_NOLIGHT) || (A_CheckSpriteFlags(spriteNum, SFLAG_USEACTIVATOR) && sector[pSprite->sectnum].lotag & 16384))
    {
        if (actor[spriteNum].lightptr != NULL)
            A_DeleteLight(spriteNum);
    }
    else
    {
        if (actor[spriteNum].lightptr != NULL && actor[spriteNum].lightcount)
        {
            if (!(--actor[spriteNum].lightcount))
                A_DeleteLight(spriteNum);
        }

        if (pr_lighting != 1)
            return;

#ifndef EDUKE32_STANDALONE
        for (int ii=0; ii<2; ii++)
        {
            if (pSprite->picnum <= 0)  // oob safety
                break;

            switch (DYNAMICTILEMAP(pSprite->picnum-1+ii))
            {
            case DIPSWITCH__STATIC:
            case DIPSWITCH2__STATIC:
            case DIPSWITCH3__STATIC:
            case PULLSWITCH__STATIC:
            case SLOTDOOR__STATIC:
            case LIGHTSWITCH__STATIC:
            case SPACELIGHTSWITCH__STATIC:
            case SPACEDOORSWITCH__STATIC:
            case FRANKENSTINESWITCH__STATIC:
            case POWERSWITCH1__STATIC:
            case LOCKSWITCH1__STATIC:
            case POWERSWITCH2__STATIC:
            case TECHSWITCH__STATIC:
            case ACCESSSWITCH__STATIC:
            case ACCESSSWITCH2__STATIC:
                {
                    if ((pSprite->cstat & 32768) || A_CheckSpriteFlags(spriteNum, SFLAG_NOLIGHT))
                    {
                        if (actor[spriteNum].lightptr != NULL)
                            A_DeleteLight(spriteNum);
                        break;
                    }

                    vec2_t const d = { sintable[(pSprite->ang+512)&2047]>>7, sintable[(pSprite->ang)&2047]>>7 };

                    pSprite->x += d.x;
                    pSprite->y += d.y;

                    int16_t sectnum = pSprite->sectnum;
                    updatesector(pSprite->x, pSprite->y, &sectnum);

                    if ((unsigned) sectnum >= MAXSECTORS || pSprite->z > sector[sectnum].floorz || pSprite->z < sector[sectnum].ceilingz)
                        goto POOP;

                    G_AddGameLight(0, spriteNum, (pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1, 512-ii*128,
                        ii==0 ? (172+(200<<8)+(104<<16)) : 216+(52<<8)+(20<<16), PR_LIGHT_PRIO_LOW);

                POOP:
                    pSprite->x -= d.x;
                    pSprite->y -= d.y;
                }
                break;
            }
        }

        switch (DYNAMICTILEMAP(pSprite->picnum))
        {
        case ATOMICHEALTH__STATIC:
            G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), LIGHTRAD2(spriteNum, pSprite), 128+(128<<8)+(255<<16),PR_LIGHT_PRIO_HIGH_GAME);
            break;

        case FIRE__STATIC:
        case FIRE2__STATIC:
        case BURNING__STATIC:
        case BURNING2__STATIC:
            {
                uint32_t color;
                int32_t jj;

                static int32_t savedfires[32][4];  // sectnum x y z

                /*
                if (Actor[i].floorz - Actor[i].ceilingz < 128) break;
                if (s->z > Actor[i].floorz+2048) break;
                */

                switch (pSprite->pal)
                {
                case 1: color = 128+(128<<8)+(255<<16); break;
                case 2: color = 255+(48<<8)+(48<<16); break;
                case 8: color = 48+(255<<8)+(48<<16); break;
                default: color = 240+(160<<8)+(80<<16); break;
                }

                for (jj=savedFires-1; jj>=0; jj--)
                    if (savedfires[jj][0]==pSprite->sectnum && savedfires[jj][1]==(pSprite->x>>3) &&
                        savedfires[jj][2]==(pSprite->y>>3) && savedfires[jj][3]==(pSprite->z>>7))
                        break;

                if (jj==-1 && savedFires<32)
                {
                    jj = savedFires;
                    G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), LIGHTRAD2(spriteNum, pSprite), color, PR_LIGHT_PRIO_HIGH_GAME);
                    savedfires[jj][0] = pSprite->sectnum;
                    savedfires[jj][1] = pSprite->x>>3;
                    savedfires[jj][2] = pSprite->y>>3;
                    savedfires[jj][3] = pSprite->z>>7;
                    savedFires++;
                }
            }
            break;

        case OOZFILTER__STATIC:
            if (pSprite->xrepeat > 4)
                G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), 4096, 176+(252<<8)+(120<<16),PR_LIGHT_PRIO_HIGH_GAME);
            break;
        case FLOORFLAME__STATIC:
        case FIREBARREL__STATIC:
        case FIREVASE__STATIC:
            G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<2), LIGHTRAD2(spriteNum, pSprite)>>1, 255+(95<<8),PR_LIGHT_PRIO_HIGH_GAME);
            break;

        case EXPLOSION2__STATIC:
            if (!actor[spriteNum].lightcount)
            {
                // XXX: This block gets CODEDUP'd too much.
                int32_t x = ((sintable[(pSprite->ang+512)&2047])>>6);
                int32_t y = ((sintable[(pSprite->ang)&2047])>>6);

                pSprite->x -= x;
                pSprite->y -= y;

                G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), LIGHTRAD(spriteNum, pSprite), 240+(160<<8)+(80<<16),
                    pSprite->yrepeat > 32 ? PR_LIGHT_PRIO_HIGH_GAME : PR_LIGHT_PRIO_LOW_GAME);

                pSprite->x += x;
                pSprite->y += y;
            }
            break;
        case FORCERIPPLE__STATIC:
        case TRANSPORTERBEAM__STATIC:
            G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), LIGHTRAD(spriteNum, pSprite), 80+(80<<8)+(255<<16),PR_LIGHT_PRIO_LOW_GAME);
            break;
        case GROWSPARK__STATIC:
            {
                int32_t x = ((sintable[(pSprite->ang+512)&2047])>>6);
                int32_t y = ((sintable[(pSprite->ang)&2047])>>6);

                pSprite->x -= x;
                pSprite->y -= y;

                G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), 1024, 216+(52<<8)+(20<<16),PR_LIGHT_PRIO_HIGH_GAME);

                pSprite->x += x;
                pSprite->y += y;
            }
            break;
        case SHRINKEREXPLOSION__STATIC:
            {
                int32_t x = ((sintable[(pSprite->ang+512)&2047])>>6);
                int32_t y = ((sintable[(pSprite->ang)&2047])>>6);

                pSprite->x -= x;
                pSprite->y -= y;

                G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), 2048, 176+(252<<8)+(120<<16),PR_LIGHT_PRIO_HIGH_GAME);

                pSprite->x += x;
                pSprite->y += y;
            }
            break;
        case FREEZEBLAST__STATIC:
            G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), LIGHTRAD(spriteNum, pSprite)<<2, 72+(88<<8)+(140<<16),PR_LIGHT_PRIO_HIGH_GAME);
            break;
        case COOLEXPLOSION1__STATIC:
            G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), LIGHTRAD(spriteNum, pSprite)<<2, 128+(0<<8)+(255<<16),PR_LIGHT_PRIO_HIGH_GAME);
            break;
        case SHRINKSPARK__STATIC:
            G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), LIGHTRAD(spriteNum, pSprite), 176+(252<<8)+(120<<16),PR_LIGHT_PRIO_HIGH_GAME);
            break;
        case FIRELASER__STATIC:
            if (pSprite->statnum == STAT_PROJECTILE)
                G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), 64 * pSprite->yrepeat, 255+(95<<8),PR_LIGHT_PRIO_LOW_GAME);
            break;
        case RPG__STATIC:
            G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), 128 * pSprite->yrepeat, 255+(95<<8),PR_LIGHT_PRIO_LOW_GAME);
            break;
        case SHOTSPARK1__STATIC:
            if (actor[spriteNum].t_data[2] == 0) // check for first frame of action
            {
                int32_t x = ((sintable[(pSprite->ang+512)&2047])>>7);
                int32_t y = ((sintable[(pSprite->ang)&2047])>>7);

                pSprite->x -= x;
                pSprite->y -= y;

                G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), 8 * pSprite->yrepeat, 240+(160<<8)+(80<<16),PR_LIGHT_PRIO_LOW_GAME);
                actor[spriteNum].lightcount = 1;

                pSprite->x += x;
                pSprite->y += y;
            }
            break;
        }
#endif
    }
#endif
}
#endif // POLYMER
END_BLD_NS
