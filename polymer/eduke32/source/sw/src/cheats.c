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
#include "build.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "mytypes.h"
#include "text.h"

#include "control.h"
#include "function.h"
//#include "inv.h"


SWBOOL CheatInputMode = FALSE;
char CheatInputString[256];
SWBOOL EveryCheat = FALSE;
SWBOOL ResCheat = FALSE;

void ResCheatOn(PLAYERp UNUSED(pp), char *UNUSED(cheat_string))
{
    ResCheat = TRUE;
}

void VoxCheat(PLAYERp UNUSED(pp), char *UNUSED(cheat_string))
{
    //gs.Voxel ^= 1;
}

void RestartCheat(PLAYERp UNUSED(pp), char *UNUSED(cheat_string))
{
    ExitLevel = TRUE;
}

void RoomCheat(PLAYERp UNUSED(pp), char *UNUSED(cheat_string))
{
    extern SWBOOL FAF_DebugView;
    FAF_DebugView ^= 1;
}

void SecretCheat(PLAYERp UNUSED(pp), char *UNUSED(cheat_string))
{
    gs.Stats = !gs.Stats;
}

void NextCheat(PLAYERp UNUSED(pp), char *UNUSED(cheat_string))
{
    Level++;
    ExitLevel = TRUE;
}

void PrevCheat(PLAYERp UNUSED(pp), char *UNUSED(cheat_string))
{
    Level--;
    ExitLevel = TRUE;
}

static int32_t showallmap;

void MapCheat(PLAYERp pp, char *UNUSED(cheat_string))
{
    showallmap ^= 1;

    if (showallmap)
        MapSetAll2D(0);
    else
        MapSetAll2D(0xFF);

    sprintf(ds, "SHOWALLMAP %s", showallmap ? "ON" : "OFF");
    PutStringInfo(pp, ds);
}


void LocCheat(PLAYERp UNUSED(pp), char *UNUSED(cheat_string))
{
    extern SWBOOL LocationInfo;
    LocationInfo++;
    if (LocationInfo > 2)
        LocationInfo = 0;
}


void WeaponCheat(PLAYERp UNUSED(pp), char *UNUSED(cheat_string))
{
    PLAYERp p;
    short pnum;
    unsigned int i;
    USERp u;

    TRAVERSE_CONNECT(pnum)
    {
        p = &Player[pnum];
        u = User[p->PlayerSprite];

        // ALL WEAPONS
        if (!SW_SHAREWARE)
            p->WpnFlags = 0xFFFFFFFF;
        else
            p->WpnFlags = 0x0000207F;  // Disallows high weapon cheat in shareware

        for (i = 0; i < SIZ(p->WpnAmmo); i++)
        {
            p->WpnAmmo[i] = DamageData[i].max_ammo;
        }

        PlayerUpdateWeapon(p, u->WeaponNum);
    }
}


void GodCheat(PLAYERp pp, char *UNUSED(cheat_string))
{
    //
    // GOD mode
    //
    GodMode ^= 1;

    sprintf(ds, "GOD MODE %s", GodMode ? "ON" : "OFF");
    PutStringInfo(pp, ds);
}

void ClipCheat(PLAYERp pp, char *UNUSED(cheat_string))
{
    FLIP(pp->Flags, PF_CLIP_CHEAT);

    sprintf(ds, "NO CLIP MODE %s", TEST(pp->Flags, PF_CLIP_CHEAT) ? "ON" : "OFF");
    PutStringInfo(pp, ds);
}

void WarpCheat(PLAYERp pp, char *cheat_string)
{
    char *cp = cheat_string;
    int episode_num;
    int level_num;

    cp += sizeof("swtrek")-1;
    level_num = atol(cp);

    //DSPRINTF(ds,"ep %d, lev %d",episode_num, level_num);
    //MONO_PRINT(ds);

    if (!SW_SHAREWARE)
    {
        if (level_num > 28 || level_num < 1)
            return;
    }
    else
    {
        if (level_num > 4 || level_num < 1)
            return;
    }

    Level = level_num;
    ExitLevel = TRUE;

    sprintf(ds, "ENTERING %1d", Level);
    PutStringInfo(pp, ds);
}

void ItemCheat(PLAYERp pp, char *cheat_string)
{
    //
    // Get all ITEMS
    //
    PLAYERp p;
    short pnum;
    short inv;
    int i;

    PutStringInfo(pp, "ITEMS");

    TRAVERSE_CONNECT(pnum)
    {
        p = &Player[pnum];
        memset(p->HasKey, TRUE, sizeof(p->HasKey));

        if (p->Wpn[WPN_UZI] && p->CurWpn == p->Wpn[WPN_UZI])
        {
            SET(p->Flags, PF_TWO_UZI);
            SET(p->Flags, PF_PICKED_UP_AN_UZI);
            InitWeaponUzi(p);
        }

        p->WpnShotgunAuto = 50;
        p->WpnRocketHeat = 5;
        p->WpnRocketNuke = 1;
        p->Armor = 100;

        for (inv = 0; inv < MAX_INVENTORY; inv++)
        {
            p->InventoryPercent[inv] = 100;
            //p->InventoryAmount[inv] = 1;
            p->InventoryAmount[inv] = InventoryData[inv].MaxInv;
            //PlayerUpdateInventory(p, inv);
        }

        PlayerUpdateInventory(p, p->InventoryNum);
        //p->InventoryNum = 0;
    }

    for (i=0; i<numsectors; i++)
    {
        if (SectUser[i] && SectUser[i]->stag == SECT_LOCK_DOOR)
            SectUser[i]->number = 0;  // unlock all doors of this type
    }

    WeaponCheat(pp, cheat_string);
    PlayerUpdateKeys(pp);
}

void EveryCheatToggle(PLAYERp pp, char *cheat_string)
{
    EveryCheat ^= 1;

    WeaponCheat(pp, cheat_string);
    GodCheat(pp, cheat_string);
    ItemCheat(pp, cheat_string);

    sprintf(ds, "EVERY CHEAT %s", EveryCheat ? "ON" : "OFF");
    PutStringInfo(pp, ds);
}

void SaveCheat(PLAYERp pp, char *UNUSED(cheat_string))
{
    saveboard("swsave.map", (vec3_t *)pp,
              pp->pang, pp->cursectnum);
}

void GeorgeFunc(PLAYERp pp, char *UNUSED(cheat_string))
{
    PlayerSound(DIGI_TAUNTAI9,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_doppler|v3df_follow,pp);
}

void BlackburnFunc(PLAYERp pp, char *UNUSED(cheat_string))
{
    PlayerSound(DIGI_TAUNTAI3,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_doppler|v3df_follow,pp);
}

int cheatcmp(char *str1, char *str2, int len)
{
    char *cp1 = str1;
    char *cp2 = str2;

    do
    {
        if (*cp1 != *cp2)
        {
            if (!((*cp1 == '#' && isdigit(*cp2)) || (*cp2 == '#' && isdigit(*cp1))))
                return -1;
        }

        cp1++;
        cp2++;
    }
    while (--len);

    return 0;
}


#define CF_ALL    BIT(0)
#define CF_NOTSW  BIT(1)

typedef struct
{
    char *CheatInputCode;
    void (*CheatInputFunc)(PLAYERp, char *);
    char flags;
} CHEAT_INFO, *CHEAT_INFOp;


CHEAT_INFO ci[] =
{
    {"swchan",      GodCheat, 0},
    {"swgimme",     ItemCheat, 0},
    {"swtrek##",    WarpCheat, 0},
    {"swgreed",     EveryCheatToggle, 0},
    {"swghost",      ClipCheat, 0},

    {"swstart",     RestartCheat, 0},

    {"swres",       ResCheatOn, 0},
    {"swloc",       LocCheat, 0},
    {"swmap",       MapCheat, 0},
    {"swsave",      SaveCheat, CF_ALL},
    {"swroom",      RoomCheat, CF_NOTSW}, // Room above room dbug
#if DEBUG
    {"swsecret",    SecretCheat, CF_ALL},
#endif
};


// !JIM! My simplified version of CheatInput which simply processes MessageInputString
void CheatInput(void)
{
    static SWBOOL cur_show;
    int ret;
    SWBOOL match = FALSE;
    unsigned int i;

    //if (CommEnabled)
    //    return;

    strcpy(CheatInputString,MessageInputString);

    // make sure string is lower cased
    Bstrlwr(CheatInputString);

    // check for at least one single match
    for (i = 0; i < SIZ(ci); i++)
    {
        // compare without the NULL
        if (cheatcmp(CheatInputString, ci[i].CheatInputCode, strlen(CheatInputString)) == 0)
        {

            // if they are equal in length then its a complet match
            if (strlen(CheatInputString) == strlen(ci[i].CheatInputCode))
            {
                match = TRUE;
                CheatInputMode = FALSE;

                if (TEST(ci[i].flags, CF_NOTSW) && SW_SHAREWARE)
                    return;

                if (!TEST(ci[i].flags, CF_ALL))
                {
                    if (CommEnabled)
                        return;

                    if (Skill >= 3)
                    {
                        PutStringInfo(Player, "You're too skillful to cheat\n");
                        return;
                    }
                }

                if (ci[i].CheatInputFunc)
                    (*ci[i].CheatInputFunc)(Player, CheatInputString);

                return;
            }
            else
            {
                match = TRUE;
                break;
            }
        }
    }

    if (!match)
    {
        ////DSPRINTF(ds,"Lost A Match %s", CheatInputString);
        //MONO_PRINT(ds);

        CheatInputMode = FALSE;
    }
}

/*    OLD CODE
void CheatInput(void)
    {
    static SWBOOL cur_show;
    signed char MNU_InputString(char *, short);
    int ret;
    SWBOOL match = FALSE;
    short i;

    // don't use InputMode here - its set for CheatInputMode
    if (MessageInputMode || MenuInputMode)
        return;

    if (!CheatInputMode)
        {
        if (KEY_PRESSED(KEYSC_S))
            {
            //KEY_PRESSED(KEYSC_S) = FALSE;
            CheatInputMode = TRUE;
            strcpy(CheatInputString,"s");
            }
        }

    if (CheatInputMode)
        {
        // get new chars
        ret = MNU_InputString(CheatInputString, 320-20);

        // quick check input
        switch (ret)
            {
            case FALSE: // Input finished (RETURN)
            case -1: // Cancel Input (pressed ESC) or Err
                CheatInputMode = FALSE;
                KB_FlushKeyboardQueue();
                return;

            case TRUE: // Got input
                break;
            }

        // make sure string is lower cased
        strlwr(CheatInputString);

        // check for at least one single match
        for (i = 0; i < SIZ(ci); i++)
            {
            // compare without the NULL
            if (cheatcmp(CheatInputString, ci[i].CheatInputCode, strlen(CheatInputString)) == 0)
                {
                ////DSPRINTF(ds,"%s",CheatInputString);
                //MONO_PRINT(ds);

                // if they are equal in length then its a complet match
                if (strlen(CheatInputString) == strlen(ci[i].CheatInputCode))
                    {
                    ////DSPRINTF(ds,"Found A Match %s", CheatInputString);
                    //MONO_PRINT(ds);

                    match = TRUE;

                    CheatInputMode = FALSE;
                    KB_FlushKeyboardQueue();

                    if (ci[i].CheatInputFunc)
                        (*ci[i].CheatInputFunc)(Player, CheatInputString);

                    return;
                    }
                else
                    {
                    match = TRUE;
                    break;
                    }
                }
            }

        if (!match)
            {
            ////DSPRINTF(ds,"Lost A Match %s", CheatInputString);
            //MONO_PRINT(ds);

            CheatInputMode = FALSE;
            KB_FlushKeyboardQueue();
            }
        }
    }

*/
