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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#ifndef actors_h_
#define actors_h_

#include "player.h"
# include "names.h"
#include "stats.h"

extern glcycle_t drawtime, actortime, thinktime, gameupdatetime;

BEGIN_DUKE_NS

#define MAXSLEEPDIST        16384
#define SLEEPTIME           1536
#define ZOFFSET             (1<<8)
#define ZOFFSET2            (16<<8)
#define ZOFFSET3            (8<<8)
#define ZOFFSET4            (12<<8)
#define ZOFFSET5            (32<<8)
#define ZOFFSET6            (4<<8)
#define FOURSLEIGHT ZOFFSET

#define ACTOR_MAXFALLINGZVEL 6144
#define ACTOR_ONWATER_ADDZ (24<<8)



// Defines for 'useractor' keyword
enum uactortypes_t
{
    notenemy,
    enemy,
    enemystayput
};

enum actionparams
{
    ACTION_STARTFRAME = 0,
    ACTION_NUMFRAMES,
    ACTION_VIEWTYPE,
    ACTION_INCVAL,
    ACTION_DELAY,
    ACTION_FLAGS,
    ACTION_PARAM_COUNT,
};

// (+ 40 16 16 4 8 6 8 6 4 20)
typedef struct
{
    int32_t temp_data[10];  // 40b sometimes used to hold offsets to con code

    int32_t aflags;                             // 4b
    union
    {
        vec3_t  bpos;                              // 12b
        struct { int bposx, bposy, bposz; };
    };
    int32_t floorz, ceilingz;                  // 8b
    union
    {
        vec2_t lastv;                              // 8b
        struct { int lastvx, lastvy; };
    };
    int16_t picnum, ang, extra, owner;         // 8b
    int16_t movflag, tempang, timetosleep;     // 6b
    int16_t actorstayput;                      // 2b

    uint8_t cgg, lasttransport;                // 2b
    // NOTE: 'dispicnum' is updated every frame, not in sync with game tics!
    int16_t dispicnum;                         // 2b

} actor_t;


// Todo - put more state in here
struct ActorInfo
{
    uint32_t scriptaddress;
    uint32_t flags;
    int aimoffset;
};


// KEEPINSYNC lunatic/con_lang.lua


extern ActorInfo   actorinfo[MAXTILES];
extern actor_t      actor[MAXSPRITES];
extern actor_t* hittype;
extern int32_t      g_noEnemies;
#define actor_tog g_noEnemies
extern int32_t      otherp;
extern int g_canSeePlayer;



END_DUKE_NS
#include "funct.h"

#endif
