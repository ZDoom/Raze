//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

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
#include "ns.h"	// Must come before everything else!

#include "duke3d.h"
#include "savegame.h"
#include "statistics.h"
#include "menu/menu.h"
#include "mapinfo.h"
#include "cmdlib.h"
#include "v_2ddrawer.h"
#include "secrets.h"
#include "sbar.h"
#include "glbackend/glbackend.h"

BEGIN_DUKE_NS

void G_InitRRRASkies(void)
{
    if (!isRRRA())
        return;
    
    for (int i = 0; i < MAXSECTORS; i++)
    {
        if (sector[i].ceilingpicnum != TILE_LA && sector[i].ceilingpicnum != TILE_MOONSKY1 && sector[i].ceilingpicnum != TILE_BIGORBIT1)
        {
            int const picnum = sector[i].ceilingpicnum;
            if (tileWidth(picnum) == 512)
            {
                psky_t *sky = tileSetupSky(picnum);
                sky->horizfrac = 32768;
                sky->lognumtiles = 1;
                sky->tileofs[0] = 0;
                sky->tileofs[1] = 0;
            }
            else if (tileWidth(picnum) == 1024)
            {
                psky_t *sky = tileSetupSky(picnum);
                sky->horizfrac = 32768;
                sky->lognumtiles = 0;
                sky->tileofs[0] = 0;
            }
        }
    }
}

void G_NewGame(MapRecord *map, int skillNum)
{
    struct player_struct *const pPlayer = g_player[0].ps;

    handleevents();

    ready2send = 0;

#if 0
    if (m_recstat != 2 && ud.last_level >= 0 &&
        (g_netServer || ud.multimode > 1) && (ud.coop&GAMETYPE_SCORESHEET))
    {
        dobonus(1);
    }

    if (isRR() && !isRRRA() && map->levelNumber == levelnum(0, 6))
        dobonus(0);
#endif

    show_shareware = REALGAMETICSPERSEC*30;

    ud.nextLevel = map;
    ud.player_skill = skillNum;
    ud.secretlevel = 0;
    ud.from_bonus = 0;

    ud.last_level = -1;
    
    int const UserMap = false;// Menu_HaveUserMap();

    // we don't want the intro to play after the multiplayer setup screen.
    if (!isRR() && (!g_netServer && ud.multimode < 2) && UserMap == 0 && map->levelNumber == levelnum(3, 0))
    {
        e4intro([](bool) {});
    }

    pPlayer->zoom = 768;
    pPlayer->gm = 0;
	M_ClearMenus();

    ResetGameVars();

    //AddLog("Newgame");

    if (m_coop != 1)
    {
        for (int weaponNum = 0; weaponNum < 12/*MAX_WEAPONS*/; weaponNum++)
        {
            auto const worksLike = isWW2GI() ? PWEAPON(0, weaponNum, WorksLike) : weaponNum;
            if (worksLike == PISTOL_WEAPON)
            {
                pPlayer->curr_weapon = weaponNum;
                pPlayer->gotweapon.Set(weaponNum);
                pPlayer->ammo_amount[weaponNum] = min<int16_t>(max_ammo_amount[weaponNum], 48);
            }
            else if (worksLike == KNEE_WEAPON || (!isRR() && worksLike == HANDREMOTE_WEAPON) || (isRRRA() && worksLike == SLINGBLADE_WEAPON))
            {
                pPlayer->gotweapon.Set(weaponNum);
                if (isRRRA())
                    pPlayer->ammo_amount[KNEE_WEAPON] = 1;
            }
        }
        pPlayer->last_weapon = -1;
    }

    display_mirror = 0;
}

void resetpspritevars(int gameMode);

void clearfrags(void)
{
    for (int i = 0; i < ud.multimode; i++)
    {
        playerdata_t *const pPlayerData = &g_player[i];
        pPlayerData->ps->frag = pPlayerData->ps->fraggedself = 0;
        memset(pPlayerData->frags, 0, sizeof(pPlayerData->frags));
    }
}


END_DUKE_NS
