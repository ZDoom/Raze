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

#include "build.h"

#include "names2.h"
#include "panel.h"
#include "game.h"
#include "mytypes.h"
#include "misc.h"

#include "gamecontrol.h"
#include "gstrings.h"
#include "cheathandler.h"
#include "d_protocol.h"
#include "cheats.h"
//#include "inv.h"

BEGIN_SW_NS

SWBOOL CheatInputMode = FALSE;
SWBOOL EveryCheat = FALSE;
SWBOOL mapcheat = false;
extern SWBOOL FAF_DebugView;

const char *CheatKeyType;
void KeysCheat(PLAYERp pp, const char *cheat_string);

static PLAYERp checkCheat(cheatseq_t* c)
{
    if (CommEnabled)
        return nullptr;

    if (Skill >= 3 && !c->DontCheck && !sv_cheats)
    {
        PutStringInfo(&Player[screenpeek], GStrings("TXTS_TOOSKILLFUL"));
        return nullptr;
    }

    return &Player[screenpeek];
}

const char *GameInterface::CheckCheatMode() 
{
     if (Skill >= 3 && !sv_cheats)
     {
         return GStrings("TXTS_TOOSKILLFUL");
     }
     return nullptr;
 }

const char *GameInterface::GenericCheat(int player, int cheat)
{
    switch (cheat)
    {
    case CHT_GOD:
        GodMode ^= 1;   // fixme: Make god mode a player property.
        return GStrings(GodMode ? "GOD MODE: ON" : "GOD MODE: OFF");

    case CHT_GODOFF:
        GodMode = 0;   // fixme: Make god mode a player property.
        return GStrings("GOD MODE: OFF");

    case CHT_GODON:
        GodMode = 1;   // fixme: Make god mode a player property.
        return GStrings("GOD MODE: ON");

    case CHT_NOCLIP:
        Player[player].Flags ^= PF_CLIP_CHEAT;
        return GStrings(Player[player].Flags & PF_CLIP_CHEAT ? "CLIPPING: OFF" : "CLIPPING: ON");

    default:
        return nullptr;
    }
}


bool RestartCheat(cheatseq_t* c)
{
    if (!checkCheat(c)) return false;
    ExitLevel = TRUE;
    return true;
}

bool RoomCheat(cheatseq_t* c)
{
    FAF_DebugView ^= 1;
    return true;
}

bool NextCheat(cheatseq_t* c)
{
    if (!checkCheat(c)) return false;
    if (!currentLevel) return true;
    NextLevel = FindMapByLevelNum(currentLevel->levelNumber + 1);
    if (NextLevel) ExitLevel = TRUE;
    return true;
}

bool PrevCheat(cheatseq_t* c)
{
    if (!checkCheat(c)) return false;
    if (!currentLevel) return true;
    NextLevel = FindMapByLevelNum(currentLevel->levelNumber - 1);
    if (NextLevel) ExitLevel = TRUE;
    return true;
}

bool MapCheat(cheatseq_t* c)
{
    PLAYERp pp;
    if (!(pp=checkCheat(c))) return false;
    mapcheat = !mapcheat;
    // Need to do this differently. The code here was completely broken.
    PutStringInfo(pp, GStrings(mapcheat ? "TXTS_AMON" : "TXTS_AMOFF"));
    return true;
}

bool WeaponCheat(cheatseq_t* c)
{
    if (!checkCheat(c)) return false;
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

        p->WpnShotgunAuto = 50;
        p->WpnRocketHeat = 5;
        p->WpnRocketNuke = 1;

        PlayerUpdateWeapon(p, u->WeaponNum);
    }
    return true;
}

bool AmmoCheat(cheatseq_t* c)
{
    if (!checkCheat(c)) return false;
    PLAYERp p;
    short pnum;
    unsigned int i;
    USERp u;

    TRAVERSE_CONNECT(pnum)
    {
        p = &Player[pnum];
        u = User[p->PlayerSprite];

        p->WpnShotgunAuto = 50;
        p->WpnRocketHeat = 5;
        p->WpnRocketNuke = 1;

        for (i = 0; i < SIZ(p->WpnAmmo); i++)
        {
            p->WpnAmmo[i] = DamageData[i].max_ammo;
        }

        PlayerUpdateWeapon(p, u->WeaponNum);
    }
    return true;
}

bool WarpCheat(cheatseq_t* c)
{
    PLAYERp pp;
    if (!(pp = checkCheat(c))) return false;
    int level_num;

    level_num = atol((char*)c->Args);
    auto maprec = FindMapByLevelNum(level_num);
    if (!maprec) return false;

	if (!pp) return true;
    if (SW_SHAREWARE)
    {
        if (level_num > 4 || level_num < 1)
            return false;
    }
    if (TEST(pp->Flags, PF_DEAD))
        return true;


    NextLevel = maprec;
    ExitLevel = TRUE;

    sprintf(ds, "%s %s", GStrings("TXT_ENTERING"), maprec->DisplayName());
    PutStringInfo(pp, ds);
    return true;
}

bool ItemCheat(cheatseq_t* c)
{
    PLAYERp pp;
    if (!(pp = checkCheat(c))) return false;
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

    WeaponCheat(c);
    return true;
}

bool InventoryCheat(cheatseq_t* c)
{
    PLAYERp pp;
    if (!(pp = checkCheat(c))) return false;
    //
    // Get all ITEMS
    //
    PLAYERp p;
    short pnum;
    short inv;
    int i;

    PutStringInfo(pp, "INVENTORY");

    TRAVERSE_CONNECT(pnum)
    {
        p = &Player[pnum];

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
    return true;
}

bool ArmorCheat(cheatseq_t* c)
{
    PLAYERp pp;
    if (!(pp = checkCheat(c))) return false;
    short pnum;
    const char *str = nullptr;

    TRAVERSE_CONNECT(pnum)
    {
        if (User[Player[pnum].PlayerSprite]->Health < pp->MaxHealth)
            str = "ARMOR";
        Player[pnum].Armor = 100;
    }
    if (str) PutStringInfo(pp, GStrings(str));
    return true;
}

bool HealCheat(cheatseq_t* c)
{
    PLAYERp pp;
    if (!(pp = checkCheat(c))) return false;
    short pnum;
    const char *str = nullptr;

    TRAVERSE_CONNECT(pnum)
    {
        if (User[Player[pnum].PlayerSprite]->Health < pp->MaxHealth)
            str = "TXTS_ADDEDHEALTH";
        User[Player[pnum].PlayerSprite]->Health += 25;
    }
    if (str) PutStringInfo(pp, GStrings(str));
    return true;
}

bool KeyCheat(cheatseq_t* c)
{
    PLAYERp pp;
    if (!(pp = checkCheat(c))) return false;
    // Get KEYS
    PLAYERp p;
    short pnum;
    const char *cp = (char*)c->Args;
	const char *str = "TXTS_GIVEKEY";
    int keynum = 0;

    keynum = atol(cp);

    TRAVERSE_CONNECT(pnum)
    {
        p = &Player[pnum];
        if (keynum >= 1 && keynum <= 8)
        {
           if (p->HasKey[keynum-1] == FALSE)
           {
              p->HasKey[keynum-1] = TRUE; // cards: 0=red 1=blue 2=green 3=yellow | keys: 4=gold 5=silver 6=bronze 7=red
              str = "TXTS_KEYGIVEN";
           }
           else
           {
              p->HasKey[keynum-1] = FALSE;
              str = "TXTS_KEYREMOVED";
           }
        }
    }
    PutStringInfo(pp, GStrings(str));
    return true;
}

bool KeysCheat(cheatseq_t* c)
{
    PLAYERp pp;
    if (!(pp = checkCheat(c))) return false;
    // Get KEYS
    PLAYERp p;
    short pnum;
    const char* str = "TXTS_GIVEKEY";
    int keynum = 0;

    TRAVERSE_CONNECT(pnum)
    {
        p = &Player[pnum];
        memset(p->HasKey, TRUE, sizeof(p->HasKey));
    }
    PutStringInfo(pp, GStrings(str));
    return true;
}

bool EveryCheatToggle(cheatseq_t* c)
{
    EveryCheat ^= 1;
    C_DoCommand("god");
    return WeaponCheat(c) && ItemCheat(c);
}

// The prefix was changed from 'sw' to 'lw' so that it doesn't contain two keys of the WASD control scheme, which interferes with input control.
static cheatseq_t swcheats[] = {
    {"lwgod",      "god" },
    {"lwchan",     "god" },
    {"lwgimme",    nullptr,     ItemCheat, 0},
    {"lwmedic",    nullptr,     HealCheat, 0},
    {"lwkey#",     nullptr,     KeyCheat, 0},
    {"lwkeys",     nullptr,     KeysCheat, 0},
    {"lwammo",     nullptr,     AmmoCheat, 0},
    {"lwarmor",    nullptr,     ArmorCheat, 0},
    {"lwitems",    nullptr,     ItemCheat, 0},
    {"lwguns",     nullptr,     WeaponCheat, 0},
    {"lwtrek##",   nullptr,     WarpCheat, 0},
    {"lwgreed",    nullptr,     EveryCheatToggle, 0},
    {"lwghost",    "noclip" },
    {"lwstart",    nullptr,     RestartCheat, 0},
    {"lwloc",      "stat coord", nullptr, true},
    {"lwmap",      nullptr,     MapCheat, 0},
    {"lwroom",     nullptr,     RoomCheat, true}, // Room above room dbug
};


void InitCheats()
{
    SetCheats(swcheats, countof(swcheats));
}

END_SW_NS
