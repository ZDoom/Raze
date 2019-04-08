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

#undef MAIN
#include "build.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "pal.h"
#include "text.h"
#include "colormap.h"
#include "player.h"

//#define SAVE_EXTERN
//#include "_save.h"
//#undef SAVE_EXTERN

extern short NormalVisibility;

// indexed by gs.BorderNum   130,172
short InventoryBarXpos[] = {110, 110, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80};
short InventoryBarYpos[] = {172, 172, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130};

void PlayerUpdateInventory(PLAYERp pp, short InventoryNum);
void SpawnInventoryBar(PLAYERp pp);
void InventoryBarUpdatePosition(PLAYERp pp);
void InvBorderRefresh(PLAYERp pp);
void InventoryUse(PLAYERp pp);
void InventoryStop(PLAYERp pp, short InventoryNum);
void KillInventoryBar(PLAYERp pp);
void PlayerUpdateInventoryPercent(PLAYERp pp);
void PlayerUpdateInventoryPic(PLAYERp pp);
void PlayerUpdateInventoryState(PLAYERp pp);


//#define INVENTORY_ICON_WIDTH  32
#define INVENTORY_ICON_WIDTH  28

void UseInventoryRepairKit(PLAYERp pp);
void UseInventoryMedkit(PLAYERp pp);
void UseInventoryRepairKit(PLAYERp pp);
void UseInventoryCloak(PLAYERp pp);
void UseInventoryEnvironSuit(PLAYERp pp);
void UseInventoryNightVision(PLAYERp pp);
void UseInventoryChemBomb(PLAYERp pp);
void UseInventoryFlashBomb(PLAYERp pp);
void UseInventoryCaltrops(PLAYERp pp);

void StopInventoryRepairKit(PLAYERp pp, short);
void StopInventoryMedkit(PLAYERp pp, short);
void StopInventoryRepairKit(PLAYERp pp, short);
void StopInventoryCloak(PLAYERp pp, short);
void StopInventoryEnvironSuit(PLAYERp pp, short);
void StopInventoryNightVision(PLAYERp pp, short);

extern PANEL_STATE ps_PanelEnvironSuit[];
extern PANEL_STATE ps_PanelCloak[];
extern PANEL_STATE ps_PanelMedkit[];
extern PANEL_STATE ps_PanelRepairKit[];
extern PANEL_STATE ps_PanelSelectionBox[];
extern PANEL_STATE ps_PanelNightVision[];
extern PANEL_STATE ps_PanelChemBomb[];
extern PANEL_STATE ps_PanelFlashBomb[];
extern PANEL_STATE ps_PanelCaltrops[];


INVENTORY_DATA InventoryData[MAX_INVENTORY+1] =
{
    {"PORTABLE MEDKIT",  UseInventoryMedkit,      NULL,                 ps_PanelMedkit,        0,   1, (1<<16),     0},
    {"REPAIR KIT",       NULL,                    NULL,                 ps_PanelRepairKit,     100, 1, (1<<16),     INVF_AUTO_USE},
    {"SMOKE BOMB",       UseInventoryCloak,       StopInventoryCloak,   ps_PanelCloak,         4,   1, (1<<16),     INVF_TIMED},
    {"NIGHT VISION",     UseInventoryNightVision, StopInventoryNightVision, ps_PanelNightVision,3,  1, (1<<16),     INVF_TIMED},
    {"GAS BOMB",         UseInventoryChemBomb,    NULL,                 ps_PanelChemBomb,      0,   1, (1<<16),     INVF_COUNT},
    {"FLASH BOMB",       UseInventoryFlashBomb,   NULL,                 ps_PanelFlashBomb,     0,   2, (1<<16),     INVF_COUNT},
    {"CALTROPS",         UseInventoryCaltrops,    NULL,                 ps_PanelCaltrops,      0,   3, (1<<16),     INVF_COUNT},
    {NULL, NULL, NULL, NULL, 0, 0, 0, 0}
};

void PanelInvTestSuicide(PANEL_SPRITEp psp);

void UpdateMiniBar(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    short i;
    int x,y;
    SWBOOL found;
    INVENTORY_DATAp id;
    extern SWBOOL PanelUpdateMode;

#define MINI_BAR_Y 174

#define MINI_BAR_HEALTH_BOX_PIC 2437
#define MINI_BAR_AMMO_BOX_PIC 2437
#define MINI_BAR_INVENTORY_BOX_PIC 2438

#define MINI_BAR_HEALTH_BOX_X 4
#define MINI_BAR_AMMO_BOX_X 32
#define MINI_BAR_INVENTORY_BOX_X 64
#define MINI_BAR_INVENTORY_BOX_Y MINI_BAR_Y

    if (!PanelUpdateMode)
        return;

    if (gs.BorderNum != BORDER_MINI_BAR)
        return;

    x = MINI_BAR_HEALTH_BOX_X;
    y = 200 - 26;

    rotatesprite(x << 16, y << 16, (1 << 16), 0,
                 MINI_BAR_HEALTH_BOX_PIC, 0, 0,
                 ROTATE_SPRITE_SCREEN_CLIP | ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);

    x = MINI_BAR_HEALTH_BOX_X+3;
    DisplayMiniBarNumber(pp, x, y+5, u->Health);

    if (u->WeaponNum != WPN_SWORD && u->WeaponNum != WPN_FIST)
    {
        x = MINI_BAR_AMMO_BOX_X;

        rotatesprite(x << 16, y << 16, (1 << 16), 0,
                     MINI_BAR_AMMO_BOX_PIC, 0, 0,
                     ROTATE_SPRITE_SCREEN_CLIP | ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);

        x = MINI_BAR_AMMO_BOX_X+3;
        DisplayMiniBarNumber(pp, x, y+5, pp->WpnAmmo[u->WeaponNum]);
    }

    if (!pp->InventoryAmount[pp->InventoryNum])
        return;

    // Inventory Box
    x = MINI_BAR_INVENTORY_BOX_X;

    rotatesprite(x << 16, y << 16, (1 << 16), 0,
                 MINI_BAR_INVENTORY_BOX_PIC, 0, 0,
                 ROTATE_SPRITE_SCREEN_CLIP | ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);

    id = &InventoryData[pp->InventoryNum];

    // Inventory pic
    x = MINI_BAR_INVENTORY_BOX_X + 2;
    y += 2;

    rotatesprite(x << 16, y << 16, (1 << 16), 0,
                 id->State->picndx, 0, 0,
                 ROTATE_SPRITE_SCREEN_CLIP | ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);

    // will update the AUTO and % inventory values
    PlayerUpdateInventory(pp, pp->InventoryNum);
}


void
PanelInvTestSuicide(PANEL_SPRITEp psp)
{
    if (TEST(psp->flags, PANF_SUICIDE))
    {
        pKillSprite(psp);
    }
}

PANEL_SPRITEp
SpawnInventoryIcon(PLAYERp pp, short InventoryNum)
{
    PANEL_SPRITEp psp;
    short x,y;

    // check invalid value
    ASSERT(InventoryNum < MAX_INVENTORY);

    // check to see if its already spawned
    if (pp->InventorySprite[InventoryNum])
        return NULL;

    // check for Icon panel state
    if (!InventoryData[InventoryNum].State)
        return NULL;

    x = InventoryBarXpos[gs.BorderNum] + (InventoryNum*INVENTORY_ICON_WIDTH);
    y = InventoryBarYpos[gs.BorderNum];
    psp = pSpawnSprite(pp, InventoryData[InventoryNum].State, PRI_FRONT, x, y);
    pp->InventorySprite[InventoryNum] = psp;

    psp->x1 = 0;
    psp->y1 = 0;
    psp->x2 = xdim - 1;
    psp->y2 = ydim - 1;
    psp->scale = InventoryData[InventoryNum].Scale;
    SET(psp->flags, PANF_STATUS_AREA | PANF_SCREEN_CLIP);

    return psp;
}

void
KillPanelInv(PLAYERp pp, short InventoryNum)
{
    ASSERT(pp->InventorySprite[InventoryNum]);
    ASSERT(InventoryNum < MAX_INVENTORY);

    pp->InventoryTics[InventoryNum] = 0;
    SET(pp->InventorySprite[InventoryNum]->flags, PANF_SUICIDE);
    pp->InventorySprite[InventoryNum] = NULL;
}

void
KillPlayerIcon(PLAYERp pp, PANEL_SPRITEp *pspp)
{
    SET((*pspp)->flags, PANF_SUICIDE);
    (*pspp) = NULL;
}

void
KillAllPanelInv(PLAYERp pp)
{
    short i;

    for (i = 0; i < MAX_INVENTORY; i++)
    {
        if (!pp->InventorySprite[i])
            continue;

        pp->InventoryTics[i] = 0;
        SET(pp->InventorySprite[i]->flags, PANF_SUICIDE);
        pp->InventorySprite[i]->numpages = 0;
        pp->InventorySprite[i] = NULL;
    }
}

PANEL_SPRITEp
SpawnIcon(PLAYERp pp, PANEL_STATEp state)
{
    PANEL_SPRITEp psp;
    short i;

    psp = pSpawnSprite(pp, state, PRI_FRONT, 0, 0);

    psp->x1 = 0;
    psp->y1 = 0;
    psp->x2 = xdim - 1;
    psp->y2 = ydim - 1;
    SET(psp->flags, PANF_STATUS_AREA | PANF_SCREEN_CLIP);
    return psp;
}

//////////////////////////////////////////////////////////////////////
//
// MEDKIT
//
//////////////////////////////////////////////////////////////////////

void AutoPickInventory(PLAYERp pp)
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

void
UseInventoryMedkit(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    short diff;
    short inv = INVENTORY_MEDKIT;
    short amt;


    if (!pp->InventoryAmount[inv])
        return;

    diff = 100 - u->Health;
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
            PlayerSound(DIGI_GETMEDKIT,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
        else
            PlayerSound(DIGI_AHH,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
    }
}

//////////////////////////////////////////////////////////////////////
//
// CHEMICAL WARFARE CANISTERS
//
//////////////////////////////////////////////////////////////////////
void
UseInventoryChemBomb(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    short diff;
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
void
UseInventoryFlashBomb(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    short diff;
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
void
UseInventoryCaltrops(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    short diff;
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

void
UseInventoryRepairKit(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP;
    short inv = INVENTORY_REPAIR_KIT;

    //PlaySound(DIGI_TOOLBOX, &pp->posx, &pp->posy, &pp->posz, v3df_none);
    if (pp == Player + myconnectindex)
    {
        if (STD_RANDOM_RANGE(1000) > 500)
            PlayerSound(DIGI_NOREPAIRMAN,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
        else
            PlayerSound(DIGI_NOREPAIRMAN2,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
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

void
UseInventoryCloak(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP;

    if (pp->InventoryActive[pp->InventoryNum])
    {
//        StopInventoryCloak(pp, pp->InventoryNum);
        return;
    }

    pp->InventoryActive[pp->InventoryNum] = TRUE;

    AutoPickInventory(pp);

    // on/off
    PlayerUpdateInventory(pp, pp->InventoryNum);

    SET(sp->cstat, CSTAT_SPRITE_TRANSLUCENT);
    sp->shade = 100;

    PlaySound(DIGI_GASPOP, &pp->posx, &pp->posy, &pp->posz, v3df_none);
    //if(RANDOM_RANGE(1000) > 950)
    if (pp == Player+myconnectindex)
        PlayerSound(DIGI_IAMSHADOW,&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
}

void
StopInventoryCloak(PLAYERp pp, short InventoryNum)
{
    SPRITEp sp = pp->SpriteP;

    pp->InventoryActive[InventoryNum] = FALSE;

    if (pp->InventoryPercent[InventoryNum] <= 0)
    {
        pp->InventoryPercent[InventoryNum] = 0;
        if (--pp->InventoryAmount[InventoryNum] < 0)
            pp->InventoryAmount[InventoryNum] = 0;
    }

    // on/off
    PlayerUpdateInventory(pp, InventoryNum);

    RESET(sp->cstat, CSTAT_SPRITE_TRANSLUCENT);
    sp->shade = 0;

    PlaySound(DIGI_GASPOP, &pp->posx, &pp->posy, &pp->posz, v3df_none);
}

//////////////////////////////////////////////////////////////////////
//
// ENVIRONMENT SUIT
//
//////////////////////////////////////////////////////////////////////
#if 0
void
UseInventoryEnvironSuit(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP;
    short inv = INVENTORY_ENVIRON_SUIT;

    // only activate if you have one
    if (pp->InventoryAmount[inv])
    {
        pp->InventoryActive[inv] = TRUE;
        // on/off
        PlayerUpdateInventory(pp, inv);
    }
}

void
StopInventoryEnvironSuit(PLAYERp pp, short InventoryNum)
{
    SPRITEp sp = pp->SpriteP;
    short inv = INVENTORY_ENVIRON_SUIT;

    pp->InventoryActive[InventoryNum] = FALSE;

    if (pp->InventoryPercent[InventoryNum] <= 0)
    {
        pp->InventoryPercent[InventoryNum] = 0;
        if (--pp->InventoryAmount[InventoryNum] < 0)
            pp->InventoryAmount[InventoryNum] = 0;
    }

    // on/off
    PlayerUpdateInventory(pp, InventoryNum);

    PlaySound(DIGI_SWCLOAKUNCLOAK, &pp->posx, &pp->posy, &pp->posz, v3df_none);
}
#endif

//////////////////////////////////////////////////////////////////////
//
// NIGHT VISION
//
//////////////////////////////////////////////////////////////////////

static char sectorfloorpals[MAXSECTORS], sectorceilingpals[MAXSECTORS], wallpals[MAXWALLS];

#if 0
void
DoPlayerNightVisionPalette(PLAYERp pp)
{
    short i;

    if (pp->InventoryActive[INVENTORY_NIGHT_VISION] && (pp - Player == screenpeek))
    {
        if (NightVision)
            return;                     // Already using night vision, don't
        // bother.
        g_visibility = 0;
        for (i = 0; i < numsectors; i++)
        {
            sectorfloorpals[i] = sector[i].floorpal;
            sectorceilingpals[i] = sector[i].ceilingpal;
            sector[i].floorpal = PALETTE_GREEN_LIGHTING;
            sector[i].ceilingpal = PALETTE_GREEN_LIGHTING;
        }
        for (i = 0; i < numwalls; i++)
        {
            wallpals[i] = wall[i].pal;
            wall[i].pal = PALETTE_GREEN_LIGHTING;
        }


        NightVision = TRUE;
        COVERsetbrightness(4, (char *) &palette_data[0][0]);
    }
    else
    {
        g_visibility = NormalVisibility;
        for (i = 0; i < numsectors; i++)
        {
            sector[i].floorpal = sectorfloorpals[i];
            sector[i].ceilingpal = sectorceilingpals[i];
        }
        for (i = 0; i < numwalls; i++)
        {
            wall[i].pal = wallpals[i];
        }
        DoPlayerDivePalette(pp);  // Check again to see if its a water palette
        NightVision = FALSE;
        COVERsetbrightness(gs.Brightness, (char *) &palette_data[0][0]);
    }
}
#endif

void
DoPlayerNightVisionPalette(PLAYERp pp)
{
    if (pp != Player + screenpeek) return;

    if (pp->InventoryActive[INVENTORY_NIGHT_VISION])
    {
//        if (pp->NightVision && pp->StartColor == 148)
//            return;
        SetFadeAmt(pp,-1005,148); // Night vision green tint
        pp->NightVision = TRUE;
    }
    else
    {
        // Put it all back to normal
        if (pp->StartColor == 148)
        {
            memcpy(pp->temp_pal, palette_data, sizeof(palette_data));
            memcpy(palookup[PALETTE_DEFAULT], DefaultPalette, 256 * 32);
            pp->FadeAmt = 0;
            if (videoGetRenderMode() < REND_POLYMOST)
                COVERsetbrightness(gs.Brightness, &palette_data[0][0]);
            else
                videoFadePalette(0,0,0,0);
        }
        pp->NightVision = FALSE;
    }
}

void
UseInventoryNightVision(PLAYERp pp)
{
    SPRITEp sp = pp->SpriteP;
#define NIGHT_INVENTORY_TIME 30

    if (pp->InventoryActive[pp->InventoryNum])
    {
        StopInventoryNightVision(pp, pp->InventoryNum);
        return;
    }

    pp->InventoryActive[pp->InventoryNum] = TRUE;

    // on/off
    PlayerUpdateInventory(pp, pp->InventoryNum);

    DoPlayerNightVisionPalette(pp);
    PlaySound(DIGI_NIGHTON, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_follow);
}

void
StopInventoryNightVision(PLAYERp pp, short InventoryNum)
{
    SPRITEp sp = pp->SpriteP;

    pp->InventoryActive[InventoryNum] = FALSE;

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
    PlaySound(DIGI_NIGHTOFF, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_follow);
}

//////////////////////////////////////////////////////////////////////
//
// INVENTORY KEYS
//
//////////////////////////////////////////////////////////////////////

void InventoryKeys(PLAYERp pp)
{
    short inv_hotkey;

    // scroll SPELLs left
    if (TEST_SYNC_KEY(pp, SK_INV_LEFT))
    {
        if (FLAG_KEY_PRESSED(pp, SK_INV_LEFT))
        {
            FLAG_KEY_RELEASE(pp, SK_INV_LEFT);
            SpawnInventoryBar(pp);
            PlayerUpdateInventory(pp, pp->InventoryNum - 1);
            PutStringInfo(pp, InventoryData[pp->InventoryNum].Name);
            InventoryBarUpdatePosition(pp);
            InvBorderRefresh(pp);
        }
    }
    else
    {
        FLAG_KEY_RESET(pp, SK_INV_LEFT);
    }

    // scroll SPELLs right
    if (TEST_SYNC_KEY(pp, SK_INV_RIGHT))
    {
        if (FLAG_KEY_PRESSED(pp, SK_INV_RIGHT))
        {
            FLAG_KEY_RELEASE(pp, SK_INV_RIGHT);
            SpawnInventoryBar(pp);
            PlayerUpdateInventory(pp, pp->InventoryNum + 1);
            PutStringInfo(pp, InventoryData[pp->InventoryNum].Name);
            InventoryBarUpdatePosition(pp);
            InvBorderRefresh(pp);
        }
    }
    else
    {
        FLAG_KEY_RESET(pp, SK_INV_RIGHT);
    }

    if (TEST_SYNC_KEY(pp, SK_INV_USE))
    {
        if (FLAG_KEY_PRESSED(pp, SK_INV_USE))
        {
            FLAG_KEY_RELEASE(pp, SK_INV_USE);

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
        FLAG_KEY_RESET(pp, SK_INV_USE);
    }

    // get hotkey number out of input bits
    inv_hotkey = TEST(pp->input.bits, SK_INV_HOTKEY_MASK) >> SK_INV_HOTKEY_BIT0;

    if (inv_hotkey)
    {
        if (FLAG_KEY_PRESSED(pp, SK_INV_HOTKEY_BIT0))
        {
            FLAG_KEY_RELEASE(pp, SK_INV_HOTKEY_BIT0);

            inv_hotkey -= 1;

            ////DSPRINTF(ds,"inv_hotkey %d",inv_hotkey);
            //MONO_PRINT(ds);

            // switches you to this inventory item
            //PlayerUpdateInventory(pp, inv_hotkey);
            pp->InventoryNum = inv_hotkey;

            if (InventoryData[pp->InventoryNum].Init && !TEST(pp->Flags, PF_CLIMBING))
            {
                if (pp->InventoryAmount[pp->InventoryNum])
                {
                    InventoryUse(pp);
                }
#if 0
                else
                {
                    sprintf(ds,"No %s",InventoryData[pp->InventoryNum].Name);
                    PutStringInfo(pp,ds); // DONT have message
                }
#endif
            }

            //PlayerUpdateInventory(pp, pp->InventoryNum);
        }
    }
    else
    {
        FLAG_KEY_RESET(pp, SK_INV_HOTKEY_BIT0);
    }
}

void InvBorderRefresh(PLAYERp pp)
{
    int x,y;

    if (pp != Player + myconnectindex)
        return;

    x = InventoryBarXpos[gs.BorderNum];
    y = InventoryBarYpos[gs.BorderNum];

    SetRedrawScreen(pp);
    //BorderRefreshClip(pp, x-5, y-5, x + (MAX_INVENTORY * INVENTORY_ICON_WIDTH), y + 24);
}

void InventoryTimer(PLAYERp pp)
{
    // called every time through loop
    short inv = 0;
    INVENTORY_DATAp id;

    // if bar is up
    if (pp->InventoryBarTics)
    {
        InventoryBarUpdatePosition(pp);

        pp->InventoryBarTics -= synctics;
        // if bar time has elapsed
        if (pp->InventoryBarTics <= 0)
        {
            // get rid of the bar
            KillInventoryBar(pp);
            // don't update bar anymore
            pp->InventoryBarTics = 0;

            InvBorderRefresh(pp);
            //BorderRefresh(pp);
        }
    }

    for (id = InventoryData; id->Name; id++, inv++)
    {
        // if timed and active
        if (TEST(id->Flags, INVF_TIMED) && pp->InventoryActive[inv])
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
                    pp->InventoryActive[inv] = FALSE;
                }
                else
                {
                    // reset 1 sec tic clock
                    pp->InventoryTics[inv] = SEC(1);
                }

                //PlayerUpdateInventoryPercent(pp);
                PlayerUpdateInventory(pp, pp->InventoryNum);
            }
        }
        else
        // the idea behind this is that the USE function will get called
        // every time the player is in an AUTO_USE situation.
        // This code will decrement the timer and set the Item to InActive
        // EVERY SINGLE TIME.  Relies on the USE function getting called!
        if (TEST(id->Flags, INVF_AUTO_USE) && pp->InventoryActive[inv])
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
                    pp->InventoryActive[inv] = FALSE;
                }

                //PlayerUpdateInventoryPercent(pp);
                PlayerUpdateInventory(pp, pp->InventoryNum);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////
//
// INVENTORY BAR
//
//////////////////////////////////////////////////////////////////////

void SpawnInventoryBar(PLAYERp pp)
{
    short inv = 0;
    INVENTORY_DATAp id;
    PANEL_SPRITEp psp;


    // its already up
    if (pp->InventoryBarTics)
    {
        pp->InventoryBarTics = SEC(2);
        return;
    }

    pp->InventorySelectionBox = SpawnIcon(pp, ps_PanelSelectionBox);

    for (id = InventoryData; id->Name; id++, inv++)
    {
        psp = SpawnInventoryIcon(pp, inv);

        if (!pp->InventoryAmount[inv])
        {
            //SET(psp->flags, PANF_TRANSLUCENT);
            //SET(psp->flags, PANF_TRANS_FLIP);
            psp->shade = 100; //Darken it
        }
    }

    pp->InventoryBarTics = SEC(2);
}

void KillInventoryBar(PLAYERp pp)
{
    KillAllPanelInv(pp);
    KillPlayerIcon(pp, &pp->InventorySelectionBox);
}

// In case the BorderNum changes - move the postions
void InventoryBarUpdatePosition(PLAYERp pp)
{
    short inv = 0;
    short x,y;
    INVENTORY_DATAp id;

    x = InventoryBarXpos[gs.BorderNum] + (pp->InventoryNum * INVENTORY_ICON_WIDTH);
    y = InventoryBarYpos[gs.BorderNum];

    pp->InventorySelectionBox->x = x - 5;
    pp->InventorySelectionBox->y = y - 5;

    for (id = InventoryData; id->Name; id++, inv++)
    {
        x = InventoryBarXpos[gs.BorderNum] + (inv * INVENTORY_ICON_WIDTH);
        y = InventoryBarYpos[gs.BorderNum];

        pp->InventorySprite[inv]->x = x;
        pp->InventorySprite[inv]->y = y;
    }

}

void InventoryUse(PLAYERp pp)
{
    INVENTORY_DATAp id = &InventoryData[pp->InventoryNum];

    if (id->Init)
        (*id->Init)(pp);
}

void InventoryStop(PLAYERp pp, short InventoryNum)
{
    INVENTORY_DATAp id = &InventoryData[InventoryNum];

    if (id->Stop)
        (*id->Stop)(pp, InventoryNum);
}

/////////////////////////////////////////////////////////////////
//
// Inventory Console Area
//
/////////////////////////////////////////////////////////////////

#define INVENTORY_BOX_X 231
#define INVENTORY_BOX_Y (176-8)
#define INVENTORY_BOX_ERASE 2403

short InventoryBoxX;
short InventoryBoxY;
short InventoryXoff;
short InventoryYoff;
void (*InventoryDisplayString)(PLAYERp, short, short, short, const char *);

#define INVENTORY_PIC_XOFF 1
#define INVENTORY_PIC_YOFF 1

#define INVENTORY_PERCENT_XOFF 19
#define INVENTORY_PERCENT_YOFF 13

#define INVENTORY_STATE_XOFF 19
#define INVENTORY_STATE_YOFF 1

void
PlayerUpdateInventory(PLAYERp pp, short InventoryNum)
{
    USERp u = User[pp->PlayerSprite];
    short w,h;

    // Check for items that need to go translucent from use
    if (pp->InventoryBarTics)
    {
        short inv = 0;
        INVENTORY_DATAp id;
        PANEL_SPRITEp psp;

        // Go translucent if used
        for (id = InventoryData; id->Name; id++, inv++)
        {
            psp = pp->InventorySprite[inv];
            if (!pp->InventoryAmount[inv])
            {
                //SET(psp->flags, PANF_TRANSLUCENT);
                //SET(psp->flags, PANF_TRANS_FLIP);
                psp->shade = 100; // Darken it
            }
            else
            {
                //RESET(psp->flags, PANF_TRANSLUCENT);
                //RESET(psp->flags, PANF_TRANS_FLIP);
                psp->shade = 0;
            }
        }
    }

    pp->InventoryNum = InventoryNum;

    if (pp->InventoryNum < 0)
        pp->InventoryNum = MAX_INVENTORY-1;

    if (pp->InventoryNum >= MAX_INVENTORY)
        pp->InventoryNum = 0;

    if (pp - Player != screenpeek)
        return;

    if (gs.BorderNum == BORDER_MINI_BAR)
    {
        InventoryBoxX = MINI_BAR_INVENTORY_BOX_X;
        InventoryBoxY = MINI_BAR_INVENTORY_BOX_Y;

        InventoryXoff = 1;
        InventoryYoff = 1;

        InventoryDisplayString = DisplayMiniBarSmString;
    }
    else
    {
        if (gs.BorderNum < BORDER_BAR)
            return;

        InventoryBoxX = INVENTORY_BOX_X;
        InventoryBoxY = INVENTORY_BOX_Y;

        InventoryXoff = 0;
        InventoryYoff = 0;

        InventoryDisplayString = DisplaySmString;

        // erase old info
        pSpawnFullScreenSprite(pp, INVENTORY_BOX_ERASE, PRI_MID, INVENTORY_BOX_X, INVENTORY_BOX_Y);

        // put pic
        if (pp->InventoryAmount[pp->InventoryNum])
            PlayerUpdateInventoryPic(pp);
    }

    if (pp->InventoryAmount[pp->InventoryNum])
    {
        // Auto/On/Off
        PlayerUpdateInventoryState(pp);
        // Percent count/Item count
        PlayerUpdateInventoryPercent(pp);
    }
}

void
PlayerUpdateInventoryPercent(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    short x,y;
    INVENTORY_DATAp id = &InventoryData[pp->InventoryNum];

    x = InventoryBoxX + INVENTORY_PERCENT_XOFF + InventoryXoff;
    y = InventoryBoxY + INVENTORY_PERCENT_YOFF + InventoryYoff;

    if (TEST(id->Flags, INVF_COUNT))
    {
        sprintf(ds,"%d", pp->InventoryAmount[pp->InventoryNum]);
        InventoryDisplayString(pp, x, y, 0, ds);
    }
    else
    {
        sprintf(ds,"%d%c", pp->InventoryPercent[pp->InventoryNum],'%');
        InventoryDisplayString(pp, x, y, 0, ds);
    }
}

void
PlayerUpdateInventoryPic(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    PANEL_SPRITEp psp;
    short pic;
    short x,y;
    INVENTORY_DATAp id = &InventoryData[pp->InventoryNum];

    x = InventoryBoxX + INVENTORY_PIC_XOFF + InventoryXoff;
    y = InventoryBoxY + INVENTORY_PIC_YOFF + InventoryYoff;

    pic = id->State->picndx;

    psp = pSpawnFullScreenSprite(pp, pic, PRI_FRONT_MAX, x, y);

    psp->scale = id->Scale;
}

void
PlayerUpdateInventoryState(PLAYERp pp)
{
    USERp u = User[pp->PlayerSprite];
    short x,y;
    INVENTORY_DATAp id = &InventoryData[pp->InventoryNum];

    x = InventoryBoxX + INVENTORY_STATE_XOFF + InventoryXoff;
    y = InventoryBoxY + INVENTORY_STATE_YOFF + InventoryYoff;

    if (TEST(id->Flags, INVF_AUTO_USE))
    {
        sprintf(ds,"%s", "AUTO");
        InventoryDisplayString(pp, x, y, 0, ds);
    }
    else if (TEST(id->Flags, INVF_TIMED))
    {
        sprintf(ds,"%s", pp->InventoryActive[pp->InventoryNum] ? "ON" : "OFF");
        InventoryDisplayString(pp, x, y, 0, ds);
    }
}
