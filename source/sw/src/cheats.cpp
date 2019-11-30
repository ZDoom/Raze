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
// From SWP:
// Added SWKEYS
// Added SWGUN# and SWGOD
// Added SWMEDIC    (25%)
// Added Full name key cheats - swbluecard - swgoldkey
// Added swquit
// Added 2 uzi's for swgimme
//

#include "ns.h"

#include "build.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "mytypes.h"
#include "text.h"

#include "control.h"
#include "gamecontrol.h"
//#include "inv.h"

BEGIN_SW_NS

SWBOOL CheatInputMode = FALSE;
char CheatInputString[256];
SWBOOL EveryCheat = FALSE;
SWBOOL ResCheat = FALSE;

const char *CheatKeyType;
void KeysCheat(PLAYERp pp, const char *cheat_string);

void ResCheatOn(PLAYERp, const char *)
{
    ResCheat = TRUE;
}

void VoxCheat(PLAYERp, const char *)
{
    //gs.Voxel ^= 1;
}

void RestartCheat(PLAYERp, const char *)
{
    ExitLevel = TRUE;
}

void RoomCheat(PLAYERp, const char *)
{
    extern SWBOOL FAF_DebugView;
    FAF_DebugView ^= 1;
}

void SecretCheat(PLAYERp pp, const char *)
{
    hud_stats = !hud_stats;
}

void NextCheat(PLAYERp pp, const char *)
{
    Level++;
    ExitLevel = TRUE;
}

void PrevCheat(PLAYERp pp, const char *)
{
    Level--;
    ExitLevel = TRUE;
}

void MapCheat(PLAYERp pp, const char *)
{
    automapping ^= 1;

    if (automapping)
        MapSetAll2D(0);
    else
        MapSetAll2D(0xFF);

    sprintf(ds, "AUTOMAPPING %s", automapping ? "ON" : "OFF" );
    PutStringInfo(pp, ds);
}

void LocCheat(PLAYERp pp, const char *)
{
    extern SWBOOL LocationInfo;
    LocationInfo++;
    if (LocationInfo > 2)
        LocationInfo = 0;
}

void GunsCheat(PLAYERp pp, const char *cheat_string)
{
    PLAYERp p;
    short pnum;
    unsigned int i;
    short gAmmo[10] = {0,9,12,20,3,6,5,5,10,1};
    const char *cp = cheat_string;
	const char *str = "GIVEN WEAPON %1d";
    int gunnum, x;
    USERp u;

    cp += sizeof("swgun")-1;
    gunnum = atol(cp);
    if (gunnum == 0)
        gunnum = 10;
    if (gunnum < 2 || gunnum > 10)
        return;

    TRAVERSE_CONNECT(pnum)
    {
        p = &Player[pnum];
        u = User[p->PlayerSprite];
        x = gAmmo[gunnum-1];
        if (TEST(p->WpnFlags, BIT(gunnum-1)) == 0)
            p->WpnFlags += BIT(gunnum-2) << 1;
        else
            str = "ADD AMMO TO WEAPON %1d";
        p->WpnAmmo[gunnum-1] += x;
        if (p->WpnAmmo[gunnum-1] > DamageData[gunnum-1].max_ammo)
        {
           p->WpnAmmo[gunnum-1] = DamageData[gunnum-1].max_ammo;
           str = "";
        }
        PlayerUpdateWeapon(p, u->WeaponNum);
    }
    sprintf(ds, str, gunnum);
    PutStringInfo(pp, ds);
}

void WeaponCheat(PLAYERp pp, const char *)
{
    PLAYERp p;
    short pnum;
    unsigned int i;
    USERp u;

    TRAVERSE_CONNECT(pnum)
    {
        p = &Player[pnum];
        u = User[p->PlayerSprite];

        if (!TEST(p->Flags, PF_TWO_UZI))
        {
            SET(p->Flags, PF_TWO_UZI);
            SET(p->Flags, PF_PICKED_UP_AN_UZI);
        }

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

void GodCheat(PLAYERp pp, const char *)
{
    //
    // GOD mode
    //
    GodMode ^= 1;

    sprintf(ds, "GOD MODE %s", GodMode ? "ON" : "OFF");
    PutStringInfo(pp, ds);
}

void ClipCheat(PLAYERp pp, const char *)
{
    FLIP(pp->Flags, PF_CLIP_CHEAT);

    sprintf(ds, "NO CLIP MODE %s", TEST(pp->Flags, PF_CLIP_CHEAT) ? "ON" : "OFF");
    PutStringInfo(pp, ds);
}

void WarpCheat(PLAYERp pp, const char *cheat_string)
{
    const char *cp = cheat_string;
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

void ItemCheat(PLAYERp pp, const char *cheat_string)
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

VOID HealCheat(PLAYERp pp, const char *cheat_string)
{
    short pnum;
    const char *str = "";

    TRAVERSE_CONNECT(pnum)
    {
        if (User[Player[pnum].PlayerSprite]->Health < pp->MaxHealth)
            str = "ADDED HEALTH";
        User[Player[pnum].PlayerSprite]->Health += 25;
    }
    PutStringInfo(pp, str);
}

VOID SortKeyCheat(PLAYERp pp, const char *sKey)
{
    const char *sTemp = "";

    CheatKeyType = "";

    if (Bstrncasecmp(sKey, "swredcard",9) == 0)
    {
       sTemp = "swkey1";
       CheatKeyType = "Red Cardkey";
    }
    else
    if (Bstrncasecmp(sKey, "swbluecard",10) == 0)
    {
       sTemp = "swkey2";
       CheatKeyType = "Blue Cardkey";
    }
    else
    if (Bstrncasecmp(sKey, "swgreencard",11) == 0)
    {
       sTemp = "swkey3";
       CheatKeyType = "Green Cardkey";
    }
    else
    if (Bstrncasecmp(sKey, "swyellowcard",12) == 0)
    {
       sTemp = "swkey4";
       CheatKeyType = "Yellow Cardkey";
    }
    else
    if (Bstrncasecmp(sKey, "swgoldkey",9) == 0)
    {
       sTemp = "swkey5";
       CheatKeyType = "Gold Key";
    }
    else
    if (Bstrncasecmp(sKey, "swsilverkey",11) == 0)
    {
       sTemp = "swkey6";
       CheatKeyType = "Silver Key";
    }
    else
    if (Bstrncasecmp(sKey, "swbronzekey",11) == 0)
    {
       sTemp = "swkey7";
       CheatKeyType = "Bronze Key";
    }
    else
    if (Bstrncasecmp(sKey, "swredkey",8) == 0)
    {
       sTemp = "swkey8";
       CheatKeyType = "Red Key";
    }

    if (Bstrncmp(sTemp, "", 1) != 0)
        KeysCheat(pp, sTemp);
}

VOID KeysCheat(PLAYERp pp, const char *cheat_string)
{
    // Get KEYS
    PLAYERp p;
    short pnum;
    const char *cp = cheat_string;
	const char *str = "Given all keys";
    int keynum = 0;

    cp += sizeof("swkey")-1;
    keynum = atol(cp);

    TRAVERSE_CONNECT(pnum)
    {
        p = &Player[pnum];
        if (keynum < 1 || keynum > 8)
        {
           memset(p->HasKey, TRUE, sizeof(p->HasKey));
           keynum = 0;
        }
        else
        {
           if (p->HasKey[keynum-1] == FALSE)
           {
              p->HasKey[keynum-1] = TRUE; // cards: 0=red 1=blue 2=green 3=yellow | keys: 4=gold 5=silver 6=bronze 7=red
              str = "Given %s";
           }
           else
           {
              p->HasKey[keynum-1] = FALSE;
              str = "Removed %s";
           }
        }
    }
    PlayerUpdateKeys(pp);
    if (keynum == 0)
        sprintf(ds, str);
    else
        sprintf(ds, str, CheatKeyType);
    PutStringInfo(pp, ds);
}

void EveryCheatToggle(PLAYERp pp, const char *cheat_string)
{
    EveryCheat ^= 1;

    WeaponCheat(pp, cheat_string);
    GodCheat(pp, cheat_string);
    ItemCheat(pp, cheat_string);

    sprintf(ds, "EVERY CHEAT %s", EveryCheat ? "ON" : "OFF");
    PutStringInfo(pp, ds);
}

void GeorgeFunc(PLAYERp pp, char *)
{
    PlayerSound(DIGI_TAUNTAI9,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_doppler|v3df_follow,pp);
}

void BlackburnFunc(PLAYERp pp, char *)
{
    PlayerSound(DIGI_TAUNTAI3,&pp->posx,&pp->posy,&pp->posz,v3df_dontpan|v3df_doppler|v3df_follow,pp);
}

int cheatcmp(const char *str1, const char *str2, int len)
{
    const char *cp1 = str1;
    const char *cp2 = str2;

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
    const char *CheatInputCode;
    void (*CheatInputFunc)(PLAYERp, const char *);
    char flags;
}CHEAT_INFO, *CHEAT_INFOp;


CHEAT_INFO ci[] =
    {
    {"swgod",        GodCheat, 0},
    {"swchan",       GodCheat, 0},
    {"swgimme",      ItemCheat, 0},
    {"swmedic",      HealCheat, 0},
    {"swkeys",       KeysCheat, 0},
    {"swredcard",    SortKeyCheat, 0},
    {"swbluecard",   SortKeyCheat, 0},
    {"swgreencard",  SortKeyCheat, 0},
    {"swyellowcard", SortKeyCheat, 0},
    {"swgoldkey",    SortKeyCheat, 0},
    {"swsilverkey",  SortKeyCheat, 0},
    {"swbronzekey",  SortKeyCheat, 0},
    {"swredkey",     SortKeyCheat, 0},
    {"swgun#",       GunsCheat, 0},
    {"swtrek##",    WarpCheat, 0},
    {"swgreed",     EveryCheatToggle, 0},
    {"swghost",      ClipCheat, 0},

    {"swstart",     RestartCheat, 0},

    {"swres",       ResCheatOn, 0},
    {"swloc",       LocCheat, 0},
    {"swmap",       MapCheat, 0},
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
        CheatInputMode = FALSE;
    }
}
END_SW_NS
