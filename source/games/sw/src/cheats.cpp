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
#include "misc.h"

#include "gamecontrol.h"
#include "gstrings.h"
#include "cheathandler.h"
#include "d_protocol.h"
#include "i_protocol.h"
#include "cheats.h"
#include "gamestate.h"
#include "automap.h"
//#include "inv.h"

BEGIN_SW_NS

bool CheatInputMode = false;
bool EveryCheat = false;
bool mapcheat = false;
extern bool FAF_DebugView;
extern bool ToggleFlyMode;

const char *CheatKeyType;
void KeysCheat(DSWPlayer* pp, const char *cheat_string);

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static DSWPlayer* checkCheat(cheatseq_t* c)
{
	if (::CheckCheatmode(true, true)) return nullptr;
    return getPlayer(screenpeek);
}

const char *GameInterface::CheckCheatMode() 
{
     if (Skill >= 3 && !sv_cheats)
     {
         return GStrings.GetString("TXTS_TOOSKILLFUL");
     }
     return nullptr;
 }

const char *GameInterface::GenericCheat(int player, int cheat)
{
    switch (cheat)
    {
    case CHT_GOD:
        GodMode ^= 1;   // fixme: Make god mode a player property.
        return GStrings.GetString(GodMode ? "GOD MODE: ON" : "GOD MODE: OFF");

    case CHT_GODOFF:
        GodMode = 0;   // fixme: Make god mode a player property.
        return GStrings.GetString("GOD MODE: OFF");

    case CHT_GODON:
        GodMode = 1;   // fixme: Make god mode a player property.
        return GStrings.GetString("GOD MODE: ON");

    case CHT_NOCLIP:
        getPlayer(player)->Flags ^= PF_CLIP_CHEAT;
        return GStrings.GetString(getPlayer(player)->Flags & PF_CLIP_CHEAT ? "CLIPPING: OFF" : "CLIPPING: ON");

    case CHT_FLY:
        ToggleFlyMode = true;
        return nullptr;

    default:
        return nullptr;
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool RestartCheat(cheatseq_t* c)
{
    if (!checkCheat(c)) return false;
	DeferredStartGame(currentLevel, g_nextskill);
    return true;
}

bool RoomCheat(cheatseq_t* c)
{
    FAF_DebugView = !FAF_DebugView;
    return true;
}

bool NextCheat(cheatseq_t* c)
{
    if (!checkCheat(c)) return false;
    if (!currentLevel) return true;
    auto map = FindNextMap(currentLevel);
	if (map) DeferredStartGame(map, g_nextskill);
    return true;
}

bool PrevCheat(cheatseq_t* c)
{
    if (!checkCheat(c)) return false;
    if (!currentLevel) return true;
    auto map = FindMapByLevelNum(currentLevel->levelNumber - 1);
	if (map) DeferredStartGame(map, g_nextskill);
    return true;
}

bool MapCheat(cheatseq_t* c)
{
    DSWPlayer* pp;
    if (!(pp=checkCheat(c))) return false;
    gFullMap = !gFullMap;
    // Need to do this differently. The code here was completely broken.
    PutStringInfo(pp, GStrings.GetString(gFullMap ? "TXTS_AMON" : "TXTS_AMOFF"));
    return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool WarpCheat(cheatseq_t* c)
{
    DSWPlayer* pp;
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
    if (pp->Flags & (PF_DEAD))
        return true;

    DeferredStartGame(maprec, g_nextskill);
    return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

EXTERN_CVAR(Bool, Pachinko_Win_Cheat)

bool WinPachinkoCheat(cheatseq_t* c)
{
    if (CommEnabled)
        return false;

    Pachinko_Win_Cheat = !Pachinko_Win_Cheat;
    PutStringInfo(getPlayer(myconnectindex), GStrings.GetString(Pachinko_Win_Cheat ? "TXTS_WINPACHINKOEN" : "TXTS_WINPACHINKODIS"));
    return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool BunnyCheat(cheatseq_t* c)
{
    if (CommEnabled)
        return false;

    sw_bunnyrockets = !sw_bunnyrockets;
    PutStringInfo(getPlayer(myconnectindex), GStrings.GetString(sw_bunnyrockets ? "TXTS_BUNNYENABLED" : "TXTS_BUNNYDISABLED"));
    return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool EveryCheatToggle(cheatseq_t* c)
{
    EveryCheat = !EveryCheat;
    C_DoCommand("god");
    C_DoCommand("give weapons");
    C_DoCommand("give items");
    return true;
}

// The prefix was changed from 'sw' to 'lw' so that it doesn't contain two keys of the WASD control scheme, which interferes with input control.
static cheatseq_t swcheats[] = {
    {"lwgod",      "god" },
    {"lwchan",     "god" },
    {"lwgimme",    "give all" },
    {"lwmedic",    "give health" },
    {"lwkeys",     "give keys" },
    {"lwammo",     "give ammo" },
    {"lwarmor",    "give armor" },
    {"lwitems",    "give items" },
    {"lwguns",     "give weapons" },
    {"lwtrek##",   nullptr,     WarpCheat, 0},
    {"lwgreed",    nullptr,     EveryCheatToggle, 0},
    {"lwghost",    "noclip" },
    {"lwstart",    nullptr,     RestartCheat, 0},
    {"lwloc",      "stat coord", nullptr, true},
    {"lwmap",      nullptr,     MapCheat, 0},
    {"lwroom",     nullptr,     RoomCheat, true}, // Room above room debug
    {"lwtrix",     nullptr,     BunnyCheat, true}, // Bunny rockets cheat
    {"winpachinko",nullptr,     WinPachinkoCheat, true},
};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void WeaponCheat(int player)
{
    auto p = getPlayer(player);

    if (!(p->Flags & PF_TWO_UZI))
    {
        p->Flags |= PF_TWO_UZI | PF_PICKED_UP_AN_UZI;
    }

    // ALL WEAPONS
    if (!SW_SHAREWARE) p->WpnFlags = 0xFFFFFFFF;
    else p->WpnFlags = 0x0000207F;  // Disallows high weapon cheat in shareware

    for (size_t i = 0; i < SIZ(p->WpnAmmo); i++)
    {
        p->WpnAmmo[i] = DamageData[i].max_ammo;
    }

    p->WpnShotgunAuto = 50;
    p->WpnRocketHeat = 5;
    p->WpnRocketNuke = 1;

    PlayerUpdateWeapon(p, p->GetActor()->user.WeaponNum);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void ItemCheat(int player)
{
    auto p = getPlayer(player);
    PutStringInfo(p, GStrings.GetString("GIVING EVERYTHING!"));
    memset(p->HasKey, true, sizeof(p->HasKey));

    p->WpnShotgunAuto = 50;
    p->WpnRocketHeat = 5;
    p->WpnRocketNuke = 1;
    p->Armor = 100;

    for (int inv = 0; inv < MAX_INVENTORY; inv++)
    {
        p->InventoryPercent[inv] = 100;
        p->InventoryAmount[inv] = (uint8_t)InventoryData[inv].MaxInv;
    }

    PlayerUpdateInventory(p, p->InventoryNum);

    for (auto& sect: sector)
    {
        if (sect.hasU() && sect.stag == SECT_LOCK_DOOR)
            sect.number = 0;  // unlock all doors of this type
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void cmd_Give(int player, TArrayView<uint8_t>& stream, bool skip)
{
    int type = ReadInt8(stream);
    if (skip) return;

    if (numplayers != 1 || gamestate != GS_LEVEL || (getPlayer(player)->Flags & PF_DEAD))
    {
        Printf("give: Cannot give while dead or not in a single-player game.\n");
        return;
    }

    switch (type)
    {
    case GIVE_ALL:
        ItemCheat(player);
        WeaponCheat(player);
        break;

    case GIVE_HEALTH:
        if (getPlayer(player)->GetActor()->user.Health < getPlayer(player)->MaxHealth)
        {
            getPlayer(player)->GetActor()->user.Health += 25;
            PutStringInfo(getPlayer(player), GStrings.GetString("TXTS_ADDEDHEALTH"));
        }
        break;

    case GIVE_WEAPONS:
        WeaponCheat(player);
        break;

    case GIVE_AMMO:
    {
        auto p = getPlayer(player);

        p->WpnShotgunAuto = 50;
        p->WpnRocketHeat = 5;
        p->WpnRocketNuke = 1;

        for (size_t i = 0; i < SIZ(p->WpnAmmo); i++)
        {
            p->WpnAmmo[i] = DamageData[i].max_ammo;
        }

        PlayerUpdateWeapon(p, p->GetActor()->user.WeaponNum);
        break;
    }

    case GIVE_ARMOR:
        if (getPlayer(player)->GetActor()->user.Health < getPlayer(player)->MaxHealth)
        {
            getPlayer(player)->Armor = 100;
            PutStringInfo(getPlayer(player), GStrings.GetString("TXTB_FULLARM"));
        }
        break;

    case GIVE_KEYS:
        memset(getPlayer(player)->HasKey, true, sizeof(getPlayer(player)->HasKey));
        PutStringInfo(getPlayer(player), GStrings.GetString("TXTS_GIVEKEY"));
        break;

    case GIVE_INVENTORY:
    {
        auto p = getPlayer(player);
        PutStringInfo(p, GStrings.GetString("GOT ALL INVENTORY"));

        p->WpnShotgunAuto = 50;
        p->WpnRocketHeat = 5;
        p->WpnRocketNuke = 1;
        p->Armor = 100;

        for (int inv = 0; inv < MAX_INVENTORY; inv++)
        {
            p->InventoryPercent[inv] = 100;
            p->InventoryAmount[inv] = (uint8_t)InventoryData[inv].MaxInv;
        }

        PlayerUpdateInventory(p, p->InventoryNum);
    }
    break;

    case GIVE_ITEMS:
        ItemCheat(player);
        break;
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void InitCheats()
{
    SetCheats(swcheats, countof(swcheats));
    Net_SetCommandHandler(DEM_GIVE, cmd_Give);
}

END_SW_NS
