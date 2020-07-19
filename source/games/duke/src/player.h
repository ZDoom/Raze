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
#include "tarray.h"
#include "constants.h"
#include "types.h"

BEGIN_DUKE_NS

extern int32_t playerswhenstarted;

extern int32_t PHEIGHT;


enum gamemode_t {
    MODE_MENU                   = 0x00000001,
    MODE_DEMO                   = 0x00000002,
    MODE_GAME                   = 0x00000004,
    MODE_EOL                    = 0x00000008,
    MODE_TYPE                   = 0x00000010,
    MODE_RESTART                = 0x00000020,
};


typedef struct
{
    float   horizAngleAdjust;
    fix16_t horizSkew;
    bool    lookLeft;
    bool    lookRight;

} playerdata_t;

extern uint16_t frags[MAXPLAYERS][MAXPLAYERS];
extern input_t sync[MAXPLAYERS];

# define PWEAPON(Player, Weapon, Wmember) (aplWeapon ## Wmember [Weapon][Player])

extern playerdata_t g_player[MAXPLAYERS];

void    P_GetInput(int playerNum);
void    P_GetInputMotorcycle(int playerNum);
void    P_GetInputBoat(int playerNum);

extern int16_t max_ammo_amount[MAX_WEAPONS];


extern int lastvisinc;

END_DUKE_NS

#endif
