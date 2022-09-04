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

#undef MAIN
#include "build.h"

#include "names2.h"
#include "panel.h"
#include "game.h"
#include "pal.h"
#include "misc.h"
#include "player.h"

BEGIN_SW_NS

//#define SAVE_EXTERN
//#include "_save.h"
//#undef SAVE_EXTERN

extern short NormalVisibility;

void PlayerUpdateInventory(PLAYER* pp, short InventoryNum);
void InventoryUse(PLAYER* pp);
void InventoryStop(PLAYER* pp, short InventoryNum);



void UseInventoryRepairKit(PLAYER* pp);
void UseInventoryMedkit(PLAYER* pp);
void UseInventoryRepairKit(PLAYER* pp);
void UseInventoryCloak(PLAYER* pp);
void UseInventoryEnvironSuit(PLAYER* pp);
void UseInventoryNightVision(PLAYER* pp);
void UseInventoryChemBomb(PLAYER* pp);
void UseInventoryFlashBomb(PLAYER* pp);
void UseInventoryCaltrops(PLAYER* pp);

void StopInventoryRepairKit(PLAYER* pp, short);
void StopInventoryMedkit(PLAYER* pp, short);
void StopInventoryRepairKit(PLAYER* pp, short);
void StopInventoryCloak(PLAYER* pp, short);
void StopInventoryEnvironSuit(PLAYER* pp, short);
void StopInventoryNightVision(PLAYER* pp, short);

extern PANEL_STATE ps_PanelEnvironSuit[];


INVENTORY_DATA InventoryData[MAX_INVENTORY+1] =
{
    {"PORTABLE MEDKIT",  UseInventoryMedkit,      nullptr,                      0,   1, (FRACUNIT),     0},
    {"REPAIR KIT",       nullptr,                    nullptr,                      100, 1, (FRACUNIT),     INVF_AUTO_USE},
    {"SMOKE BOMB",       UseInventoryCloak,       StopInventoryCloak,        4,   1, (FRACUNIT),     INVF_TIMED},
    {"NIGHT VISION",     UseInventoryNightVision, StopInventoryNightVision, 3,  1, (FRACUNIT),     INVF_TIMED},
    {"GAS BOMB",         UseInventoryChemBomb,    nullptr,                      0,   1, (FRACUNIT),     INVF_COUNT},
    {"FLASH BOMB",       UseInventoryFlashBomb,   nullptr,                      0,   2, (FRACUNIT),     INVF_COUNT},
    {"CALTROPS",         UseInventoryCaltrops,    nullptr,                      0,   3, (FRACUNIT),     INVF_COUNT},
    {nullptr, nullptr, nullptr, 0, 0, 0, 0}
};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void PanelInvTestSuicide(PANEL_SPRITE* psp)
{
    if (psp->flags & (PANF_SUICIDE))
    {
        pKillSprite(psp);
    }
}

void KillPanelInv(PLAYER* pp, short InventoryNum)
{
    ASSERT(InventoryNum < MAX_INVENTORY);

    pp->InventoryTics[InventoryNum] = 0;
}

void KillAllPanelInv(PLAYER* pp)
{
    for (int i = 0; i < MAX_INVENTORY; i++)
    {
        pp->InventoryTics[i] = 0;
    }
}

//////////////////////////////////////////////////////////////////////
//
// MEDKIT
//
//////////////////////////////////////////////////////////////////////

void AutoPickInventory(PLAYER* pp)
{
    int i;

    // auto pick only if run out of currently selected one

    if (pp->InventoryAmount[pp->InventoryNum] <= 0)
    {
        for (i = 0; i < MAX_INVENTORY; i++)
        {
            if (i == INVENTORY_REPAIR_KIT)
                continue;

            if (pp->InventoryAmount[i])
            {
                pp->InventoryNum = i;
                return;
            }
        }

        // only take this if there is nothing else
        if (pp->InventoryAmount[INVENTORY_REPAIR_KIT])
            pp->InventoryNum = INVENTORY_REPAIR_KIT;
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void UseInventoryMedkit(PLAYER* pp)
{
    short diff;
    short inv = INVENTORY_MEDKIT;
    short amt;


    if (!pp->InventoryAmount[inv])
        return;

    diff = 100 - pp->actor->user.Health;
    if (diff <= 0)
        return;

    if (diff > pp->InventoryPercent[inv]) // If not enough to get to 100, use what's left
        amt = pp->InventoryPercent[inv];
    else
        amt = diff;

    PlayerUpdateHealth(pp, amt);

    pp->InventoryPercent[inv] -= diff;
    if (pp->InventoryPercent[inv] < 0)
    {
        pp->InventoryPercent[inv] = 0;
        pp->InventoryAmount[inv]--;
    }

    AutoPickInventory(pp);

    //percent
    PlayerUpdateInventory(pp, pp->InventoryNum);

    if (pp == Player+myconnectindex)
    {
        if (amt >= 30)
            PlayerSound(DIGI_GETMEDKIT, v3df_follow|v3df_dontpan,pp);
        else
            PlayerSound(DIGI_AHH, v3df_follow|v3df_dontpan,pp);
    }
}

//////////////////////////////////////////////////////////////////////
//
// CHEMICAL WARFARE CANISTERS
//
//////////////////////////////////////////////////////////////////////
void UseInventoryChemBomb(PLAYER* pp)
{
    short inv = INVENTORY_CHEMBOMB;

    if (!pp->InventoryAmount[inv])
        return;

    PlayerInitChemBomb(pp); // Throw a chemical bomb out there

    pp->InventoryPercent[inv] = 0;
    if (--pp->InventoryAmount[inv] < 0)
        pp->InventoryAmount[inv] = 0;

    AutoPickInventory(pp);

    PlayerUpdateInventory(pp, pp->InventoryNum);
}

//////////////////////////////////////////////////////////////////////
//
// FLASH BOMBS
//
//////////////////////////////////////////////////////////////////////
void UseInventoryFlashBomb(PLAYER* pp)
{
    short inv = INVENTORY_FLASHBOMB;

    if (!pp->InventoryAmount[inv])
        return;

    PlayerInitFlashBomb(pp);

    pp->InventoryPercent[inv] = 0;
    if (--pp->InventoryAmount[inv] < 0)
        pp->InventoryAmount[inv] = 0;

    AutoPickInventory(pp);

    PlayerUpdateInventory(pp, pp->InventoryNum);
}

//////////////////////////////////////////////////////////////////////
//
// CALTROPS
//
//////////////////////////////////////////////////////////////////////
void UseInventoryCaltrops(PLAYER* pp)
{
    short inv = INVENTORY_CALTROPS;

    if (!pp->InventoryAmount[inv])
        return;

    PlayerInitCaltrops(pp);

    pp->InventoryPercent[inv] = 0;
    if (--pp->InventoryAmount[inv] < 0)
        pp->InventoryAmount[inv] = 0;

    AutoPickInventory(pp);

    PlayerUpdateInventory(pp, pp->InventoryNum);
}

//////////////////////////////////////////////////////////////////////
//
// REPAIR KIT
//
//////////////////////////////////////////////////////////////////////

void UseInventoryRepairKit(PLAYER* pp)
{
    short inv = INVENTORY_REPAIR_KIT;

    //PlaySound(DIGI_TOOLBOX, pp, v3df_none);
    if (pp == Player + myconnectindex)
    {
        if (StdRandomRange(1000) > 500)
            PlayerSound(DIGI_NOREPAIRMAN, v3df_follow|v3df_dontpan,pp);
        else
            PlayerSound(DIGI_NOREPAIRMAN2, v3df_follow|v3df_dontpan,pp);
    }

    pp->InventoryPercent[inv] = 0;
    pp->InventoryAmount[inv] = 0;

    AutoPickInventory(pp);

    //percent
    PlayerUpdateInventory(pp, pp->InventoryNum);
}

//////////////////////////////////////////////////////////////////////
//
// CLOAK
//
//////////////////////////////////////////////////////////////////////

void UseInventoryCloak(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;

    if (pp->InventoryActive[pp->InventoryNum])
    {
//        StopInventoryCloak(pp, pp->InventoryNum);
        return;
    }

    pp->InventoryActive[pp->InventoryNum] = true;

    AutoPickInventory(pp);

    // on/off
    PlayerUpdateInventory(pp, pp->InventoryNum);

    plActor->spr.cstat |= (CSTAT_SPRITE_TRANSLUCENT);
    plActor->spr.shade = 100;

    PlaySound(DIGI_GASPOP, pp, v3df_none);
    if (pp == Player+myconnectindex)
        PlayerSound(DIGI_IAMSHADOW, v3df_follow|v3df_dontpan,pp);
}

void StopInventoryCloak(PLAYER* pp, short InventoryNum)
{
    DSWActor* plActor = pp->actor;

    pp->InventoryActive[InventoryNum] = false;

    if (pp->InventoryPercent[InventoryNum] <= 0)
    {
        pp->InventoryPercent[InventoryNum] = 0;
        if (--pp->InventoryAmount[InventoryNum] < 0)
            pp->InventoryAmount[InventoryNum] = 0;
    }

    // on/off
    PlayerUpdateInventory(pp, InventoryNum);

    plActor->spr.cstat &= ~(CSTAT_SPRITE_TRANSLUCENT);
    plActor->spr.shade = 0;

    PlaySound(DIGI_GASPOP, pp, v3df_none);
}

//////////////////////////////////////////////////////////////////////
//
// NIGHT VISION
//
//////////////////////////////////////////////////////////////////////

void DoPlayerNightVisionPalette(PLAYER* pp)
{
    if (pp != Player + screenpeek) return;

    if (pp->InventoryActive[INVENTORY_NIGHT_VISION])
    {
//        if (pp->NightVision && pp->StartColor == 148)
//            return;
        SetFadeAmt(pp,-1005,148); // Night vision green tint
        pp->NightVision = true;
    }
    else
    {
        // Put it all back to normal
        if (pp->StartColor == 148)
        {
            pp->FadeAmt = 0;
            videoFadePalette(0,0,0,0);
        }
        pp->NightVision = false;
    }
}

void UseInventoryNightVision(PLAYER* pp)
{
    if (pp->InventoryActive[pp->InventoryNum])
    {
        StopInventoryNightVision(pp, pp->InventoryNum);
        return;
    }

    pp->InventoryActive[pp->InventoryNum] = true;

    // on/off
    PlayerUpdateInventory(pp, pp->InventoryNum);

    DoPlayerNightVisionPalette(pp);
    PlaySound(DIGI_NIGHTON, pp, v3df_dontpan|v3df_follow);
}

void StopInventoryNightVision(PLAYER* pp, short InventoryNum)
{
    pp->InventoryActive[InventoryNum] = false;

    if (pp->InventoryPercent[InventoryNum] <= 0)
    {
        pp->InventoryPercent[InventoryNum] = 0;
        if (--pp->InventoryAmount[InventoryNum] < 0)
            pp->InventoryAmount[InventoryNum] = 0;
    }

    AutoPickInventory(pp);

    // on/off
    PlayerUpdateInventory(pp, pp->InventoryNum);

    DoPlayerNightVisionPalette(pp);
    DoPlayerDivePalette(pp);
    PlaySound(DIGI_NIGHTOFF, pp, v3df_dontpan|v3df_follow);
}

//////////////////////////////////////////////////////////////////////
//
// INVENTORY KEYS
//
//////////////////////////////////////////////////////////////////////

void InventoryKeys(PLAYER* pp)
{
    // scroll SPELLs left
    if (pp->input.actions & SB_INVPREV)
    {
        if (pp->KeyPressBits & SB_INVPREV)
        {
            pp->KeyPressBits &= ~SB_INVPREV;
            pp->InventoryBarTics = SEC(2);
            PlayerUpdateInventory(pp, pp->InventoryNum - 1);
            PutStringInfo(pp, InventoryData[pp->InventoryNum].Name);
        }
    }
    else
    {
        pp->KeyPressBits |= SB_INVPREV;
    }

    // scroll SPELLs right
    if (pp->input.actions & SB_INVNEXT)
    {
        if (pp->KeyPressBits & SB_INVNEXT)
        {
            pp->KeyPressBits &= ~SB_INVNEXT;
            pp->InventoryBarTics = SEC(2);
            PlayerUpdateInventory(pp, pp->InventoryNum + 1);
            PutStringInfo(pp, InventoryData[pp->InventoryNum].Name);
        }
    }
    else
    {
        pp->KeyPressBits |= SB_INVNEXT;
    }

    if (pp->input.actions & SB_INVUSE)
    {
        if (pp->KeyPressBits & SB_INVUSE)
        {
            pp->KeyPressBits &= ~SB_INVUSE;
            if (InventoryData[pp->InventoryNum].Init)
            {
                if (pp->InventoryAmount[pp->InventoryNum])
                {
                    InventoryUse(pp);
                }
                else
                {
                    sprintf(ds,"No %s",InventoryData[pp->InventoryNum].Name);
                    PutStringInfo(pp,ds); // DONT have message
                }
            }
        }
    }
    else
    {
        pp->KeyPressBits |= SB_INVUSE;
    }

    // test all 7 items
    for (int i = 0; i <= 7; i++)
    {
        ESyncBits bit = ESyncBits::FromInt(SB_ITEM_BIT_1 << i);
        if (pp->input.isItemUsed(i))
        {
            if (pp->KeyPressBits & bit)
            {
                pp->KeyPressBits &= ~bit;

	            // switches you to this inventory item
	            pp->InventoryNum = i;

	            if (InventoryData[pp->InventoryNum].Init && !(pp->Flags & PF_CLIMBING))
	            {
	                if (pp->InventoryAmount[pp->InventoryNum])
	                {
	                    InventoryUse(pp);
	                }
	            }
	        }
	    }
	    else
	    {
             pp->KeyPressBits |= bit;
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void InventoryTimer(PLAYER* pp)
{
    // called every time through loop
    short inv = 0;
    INVENTORY_DATA* id;

    // if bar is up
    if (pp->InventoryBarTics)
    {
        pp->InventoryBarTics -= synctics;
        // if bar time has elapsed
        if (pp->InventoryBarTics <= 0)
        {
            // don't update bar anymore
            pp->InventoryBarTics = 0;
        }
    }

    for (id = InventoryData; id->Name; id++, inv++)
    {
        // if timed and active
        if ((id->Flags & INVF_TIMED) && pp->InventoryActive[inv])
        {
            // dec tics
            pp->InventoryTics[inv] -= synctics;
            if (pp->InventoryTics[inv] <= 0)
            {
                // take off a percentage
                pp->InventoryPercent[inv] -= id->DecPerSec;
                if (pp->InventoryPercent[inv] <= 0)
                {
                    // ALL USED UP
                    pp->InventoryPercent[inv] = 0;
                    InventoryStop(pp, inv);
                    pp->InventoryActive[inv] = false;
                }
                else
                {
                    // reset 1 sec tic clock
                    pp->InventoryTics[inv] = SEC(1);
                }

                PlayerUpdateInventory(pp, pp->InventoryNum);
            }
        }
        else
        // the idea behind this is that the USE function will get called
        // every time the player is in an AUTO_USE situation.
        // This code will decrement the timer and set the Item to InActive
        // EVERY SINGLE TIME.  Relies on the USE function getting called!
        if ((id->Flags & INVF_AUTO_USE) && pp->InventoryActive[inv])
        {
            pp->InventoryTics[inv] -= synctics;
            if (pp->InventoryTics[inv] <= 0)
            {
                // take off a percentage
                pp->InventoryPercent[inv] -= id->DecPerSec;
                if (pp->InventoryPercent[inv] <= 0)
                {
                    // ALL USED UP
                    pp->InventoryPercent[inv] = 0;
                    // should get rid if Amount - stop it for good
                    InventoryStop(pp, inv);
                }
                else
                {
                    // reset 1 sec tic clock
                    pp->InventoryTics[inv] = SEC(1);
                    // set to InActive EVERY TIME THROUGH THE LOOP!
                    pp->InventoryActive[inv] = false;
                }

                PlayerUpdateInventory(pp, pp->InventoryNum);
            }
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void InventoryUse(PLAYER* pp)
{
    INVENTORY_DATA* id = &InventoryData[pp->InventoryNum];

    if (id->Init)
        (*id->Init)(pp);
}

void InventoryStop(PLAYER* pp, short InventoryNum)
{
    INVENTORY_DATA* id = &InventoryData[InventoryNum];

    if (id->Stop)
        (*id->Stop)(pp, InventoryNum);
}

/////////////////////////////////////////////////////////////////
//
// Inventory Console Area
//
/////////////////////////////////////////////////////////////////

void PlayerUpdateInventory(PLAYER* pp, short InventoryNum)
{
    pp->InventoryNum = InventoryNum;

    if (pp->InventoryNum < 0)
        pp->InventoryNum = MAX_INVENTORY-1;

    if (pp->InventoryNum >= MAX_INVENTORY)
        pp->InventoryNum = 0;

}

END_SW_NS
