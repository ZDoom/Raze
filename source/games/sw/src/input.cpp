//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "game.h"
#include "network.h"
#include "gamecontrol.h"
#include "player.h"
#include "razemenu.h"


BEGIN_SW_NS

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void InitTimingVars(void)
{
    PlayClock = 0;
    randomseed = 17L;
    MoveSkip8 = 2;
    MoveSkip2 = 0;
    MoveSkip4 = 1;                      // start slightly offset so these
}

#if 0
enum
{
    TURBOTURNTIME = (120 / 8),
    NORMALTURN = (12 + 6),
    RUNTURN = (28),
    PREAMBLETURN = 3,
    NORMALKEYMOVE = 35,
    MAXFVEL = ((NORMALKEYMOVE * 2) + 10),
    MAXSVEL = ((NORMALKEYMOVE * 2) + 10),
    MAXANGVEL = 100,
    MAXHORIZVEL = 128
};
#endif

//---------------------------------------------------------------------------
//
// handles movement
//
//---------------------------------------------------------------------------

void processWeapon(PLAYER* const pp)
{
    DSWActor* plActor = pp->actor;
    if (plActor == nullptr) return;
    int i;

    if (pp->input.getNewWeapon() == WeaponSel_Next)
    {
        int next_weapon = plActor->user.WeaponNum + 1;
        int start_weapon;

        start_weapon = plActor->user.WeaponNum + 1;

        if (plActor->user.WeaponNum == WPN_SWORD)
            start_weapon = WPN_STAR;

        if (plActor->user.WeaponNum == WPN_FIST)
        {
            next_weapon = 14;
        }
        else
        {
            next_weapon = -1;
            for (i = start_weapon; true; i++)
            {
                if (i >= MAX_WEAPONS_KEYS)
                {
                    next_weapon = 13;
                    break;
                }

                if (pp->WpnFlags & (BIT(i)) && pp->WpnAmmo[i])
                {
                    next_weapon = i;
                    break;
                }
            }
        }

        pp->input.setNewWeapon(next_weapon + 1);
    }
    else if (pp->input.getNewWeapon() == WeaponSel_Prev)
    {
        int prev_weapon = plActor->user.WeaponNum - 1;
        int start_weapon;

        start_weapon = plActor->user.WeaponNum - 1;

        if (plActor->user.WeaponNum == WPN_SWORD)
        {
            prev_weapon = 13;
        }
        else if (plActor->user.WeaponNum == WPN_STAR)
        {
            prev_weapon = 14;
        }
        else
        {
            prev_weapon = -1;
            for (i = start_weapon; true; i--)
            {
                if (i <= -1)
                    i = WPN_HEART;

                if (pp->WpnFlags & (BIT(i)) && pp->WpnAmmo[i])
                {
                    prev_weapon = i;
                    break;
                }
            }
        }
        pp->input.setNewWeapon(prev_weapon + 1);
    }
    else if (pp->input.getNewWeapon() == WeaponSel_Alt)
    {
        int which_weapon = plActor->user.WeaponNum + 1;
        pp->input.setNewWeapon(which_weapon);
    }
}

END_SW_NS
