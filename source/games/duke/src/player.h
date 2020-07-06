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

#ifndef player_h_
#define player_h_

#include "names.h"
#include "fix16.h"
#include "net.h"
#include "tarray.h"
#include "constants.h"

BEGIN_DUKE_NS

extern int32_t playerswhenstarted;

#define MOVEFIFOSIZ                 256

#define HORIZ_MIN                   -99
#define HORIZ_MAX                   299
#define AUTO_AIM_ANGLE              48
#define PHEIGHT_DUKE                (38<<8)
#define PHEIGHT_RR                  (40<<8);
extern int32_t PHEIGHT;

#define WEAPON_POS_LOWER            -9
#define WEAPON_POS_RAISE            10
#define WEAPON_POS_START             6

#define MAX_WEAPON_RECS             256

enum gamemode_t {
    MODE_MENU                   = 0x00000001,
    MODE_DEMO                   = 0x00000002,
    MODE_GAME                   = 0x00000004,
    MODE_EOL                    = 0x00000008,
    MODE_TYPE                   = 0x00000010,
    MODE_RESTART                = 0x00000020,
    MODE_SENDTOWHOM             = 0x00000040,
};

typedef struct {
    union
    {
        vec3_t pos;
        struct { int ox, oy, oz; };
    };
    union
    {
        int16_t oa;
        int16_t ang;
    };
    union
    {
        int16_t sect;
        int16_t os;
    };
} playerspawn_t;

typedef struct {
    ESyncBits bits;
    int16_t fvel, svel;
    fix16_t q16avel, q16horz;
} input_t;

#pragma pack(push,1)


// KEEPINSYNC lunatic/_defs_game.lua
typedef struct
{
    struct player_struct *ps;
    input_t *input;

    bool    horizRecenter;
    float   horizAngleAdjust;
    fix16_t horizSkew;

    int32_t pcolor, pteam;
    // NOTE: wchoice[HANDREMOTE_WEAPON .. MAX_WEAPONS-1] unused
    uint8_t frags[MAXPLAYERS];

    char playerreadyflag, playerquitflag, connected;
    char user_name[32];
    char syncval[SYNCFIFOSIZ][MAXSYNCBYTES];
    double  lastInputTicks;

} playerdata_t;
#pragma pack(pop)


# define PWEAPON(Player, Weapon, Wmember) (aplWeapon ## Wmember [Weapon][Player])

extern playerspawn_t    g_playerSpawnPoints[MAXPLAYERS];
extern playerdata_t     *const g_player;
extern int32_t          mouseyaxismode;


void    P_GetInput(int playerNum);
void    P_GetInputMotorcycle(int playerNum);
void    P_GetInputBoat(int playerNum);

extern int16_t max_ammo_amount[MAX_WEAPONS];


extern int lastvisinc;

END_DUKE_NS

#endif
