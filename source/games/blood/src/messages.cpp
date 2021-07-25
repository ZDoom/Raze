//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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

#include "build.h"
#include "compat.h"
#include "gamecontrol.h"

#include "blood.h"
#include "gstrings.h"
#include "cheathandler.h"
#include "d_protocol.h"
#include "gamestate.h"
#include "automap.h"

BEGIN_BLD_NS

void sub_5A928(void)
{
    for (int i = 0; i < buttonMap.NumButtons(); i++)
        buttonMap.ClearButton(i);
}

const char *SetGodMode(bool god)
{
    playerSetGodMode(gMe, god);
    bPlayerCheated = true;
    return gMe->godMode? GStrings("TXTB_GODMODE") : GStrings("TXTB_NOTGODMODE");
}

const char *SetClipMode(bool noclip)
{
    gNoClip = noclip;
    bPlayerCheated = true;
    return gNoClip? GStrings("TXTB_NOCLIP") : GStrings("TXTB_NOCLIPOFF");
}

void packStuff(PLAYER *pPlayer)
{
    for (int i = 0; i < 5; i++)
        packAddItem(pPlayer, i);
}

void packClear(PLAYER *pPlayer)
{
    pPlayer->packItemId = 0;
    for (int i = 0; i < 5; i++)
    {
        pPlayer->packSlots[i].isActive = 0;
        pPlayer->packSlots[i].curAmount = 0;
    }
}

void SetAmmo(bool stat)
{
    if (stat)
    {
        for (int i = 0; i < 12; i++)
            gMe->ammoCount[i] = gAmmoInfo[i].max;
        viewSetMessage(GStrings("TXTB_FULLAMMO"));
    }
    else
    {
        for (int i = 0; i < 12; i++)
            gMe->ammoCount[i] = 0;
        viewSetMessage(GStrings("TXTB_NOAMMO"));
    }
}

void SetWeapons(bool stat)
{
    for (int i = 0; i < 14; i++)
    {
        gMe->hasWeapon[i] = stat;
    }
    SetAmmo(stat);
    if (stat)
        viewSetMessage(GStrings("TXTB_ALLWEAP"));
    else
    {
        if (!VanillaMode())
        {
            // Keep the pitchfork to avoid freeze
            gMe->hasWeapon[1] = 1;
            gMe->curWeapon = 0;
            gMe->nextWeapon = 1;
        }
        viewSetMessage(GStrings("TXTB_NOWEAP"));
    }
}

void SetToys(bool stat)
{
    if (stat)
    {
        packStuff(gMe);
        viewSetMessage(GStrings("TXTB_FULLINV"));
    }
    else
    {
        packClear(gMe);
        viewSetMessage(GStrings("TXTB_NOINV"));
    }
}

void SetArmor(bool stat)
{
    int nAmount;
    if (stat)
    {
        viewSetMessage(GStrings("TXTB_FULLARM"));
        nAmount = 3200;
    }
    else
    {
        viewSetMessage(GStrings("TXTB_NOARM"));
        nAmount = 0;
    }
    for (int i = 0; i < 3; i++)
        gMe->armor[i] = nAmount;
}

void SetKeys(bool stat)
{
    for (int i = 1; i <= 6; i++)
        gMe->hasKey[i] = stat;
    if (stat)
        viewSetMessage(GStrings("TXTB_ALLKEYS"));
    else
        viewSetMessage(GStrings("TXTB_NOKEYS"));
}

void SetInfiniteAmmo(bool stat)
{
    gInfiniteAmmo = stat;
    if (gInfiniteAmmo)
        viewSetMessage(GStrings("TXTB_INFAMMO"));
    else
        viewSetMessage(GStrings("TXTB_LIMAMMO"));
}

void SetMap(bool stat)
{
    gFullMap = stat;
    if (gFullMap)
        viewSetMessage(GStrings("TXTB_ALLMAP"));
    else
        viewSetMessage(GStrings("TXTB_NOALLMAP"));
}

void SetWooMode(bool stat)
{
    if (stat)
    {
        if (!powerupCheck(gMe, kPwUpTwoGuns))
            powerupActivate(gMe, kPwUpTwoGuns);
    }
    else
    {
        if (powerupCheck(gMe, kPwUpTwoGuns))
        {
            if (!VanillaMode())
                gMe->pwUpTime[kPwUpTwoGuns] = 0;
            powerupDeactivate(gMe, kPwUpTwoGuns);
        }
    }
}

void ToggleWooMode(void)
{
    SetWooMode(!(powerupCheck(gMe, kPwUpTwoGuns) != 0));
}

void ToggleBoots(void)
{
    if (powerupCheck(gMe, kPwUpJumpBoots))
    {
        viewSetMessage(GStrings("TXTB_NOJBOOTS"));
        if (!VanillaMode())
        {
            gMe->pwUpTime[kPwUpJumpBoots] = 0;
            gMe->packSlots[4].curAmount = 0;
        }
        powerupDeactivate(gMe, kPwUpJumpBoots);
    }
    else
    {
        viewSetMessage(GStrings("TXTB_JBOOTS"));
        if (!VanillaMode())
            gMe->pwUpTime[kPwUpJumpBoots] = gPowerUpInfo[kPwUpJumpBoots].bonusTime;
        powerupActivate(gMe, kPwUpJumpBoots);
    }
}

void ToggleInvisibility(void)
{
    if (powerupCheck(gMe, kPwUpShadowCloak))
    {
        viewSetMessage(GStrings("TXTB_VISIBLE"));
        if (!VanillaMode())
            gMe->pwUpTime[kPwUpShadowCloak] = 0;
        powerupDeactivate(gMe, kPwUpShadowCloak);
    }
    else
    {
        viewSetMessage(GStrings("TXTB_INVISIBLE"));
        powerupActivate(gMe, kPwUpShadowCloak);
    }
}

void ToggleInvulnerability(void)
{
    if (powerupCheck(gMe, kPwUpDeathMask))
    {
        viewSetMessage(GStrings("TXTB_VULN"));
        if (!VanillaMode())
            gMe->pwUpTime[kPwUpDeathMask] = 0;
        powerupDeactivate(gMe, kPwUpDeathMask);
    }
    else
    {
        viewSetMessage(GStrings("TXTB_INVULN"));
        powerupActivate(gMe, kPwUpDeathMask);
    }
}

void ToggleDelirium(void)
{
    if (powerupCheck(gMe, kPwUpDeliriumShroom))
    {
        viewSetMessage(GStrings("TXTB_NODELIR"));
        if (!VanillaMode())
            gMe->pwUpTime[kPwUpDeliriumShroom] = 0;
        powerupDeactivate(gMe, kPwUpDeliriumShroom);
    }
    else
    {
        viewSetMessage(GStrings("TXTB_DELIR"));
        powerupActivate(gMe, kPwUpDeliriumShroom);
    }
}

bool bPlayerCheated = false;

static int parseArgs(char *pzArgs, int *nArg1, int *nArg2)
{
    if (!nArg1 || !nArg2 || strlen(pzArgs) < 3)
        return -1;
	*nArg1 = pzArgs[0] - '0';
    int a1 = pzArgs[1] == ' ' ? 0 : pzArgs[1] - '0';
    *nArg2 = a1 * 10 + (pzArgs[2] - '0');
    return 2;
}

const char* GameInterface::GenericCheat(int player, int cheat)
{
    // message processing is not perfect because many cheats output multiple messages.

    if (gGameOptions.nGameType != 0 || numplayers > 1) // sp only for now.
        return nullptr;

    if (gamestate != GS_LEVEL || gMe->pXSprite->health == 0) // must be alive and in a level to cheat.
        return nullptr;

    bPlayerCheated = true;
    switch (cheat)
    {
    case CHT_GOD:
        return SetGodMode(!gMe->godMode);

    case CHT_GODOFF:
        return SetGodMode(false);

    case CHT_GODON:
        return SetGodMode(true);

    case CHT_NOCLIP:
        return SetClipMode(!gNoClip);

    case kCheatSpielberg:
        // demo record
        break;
    case kCheatSatchel:
        SetToys(true);
        break;
    case kCheatKevorkian:
        actDamageSprite(gMe->nSprite, gMe->pSprite, kDamageBullet, 8000);
        return GStrings("TXTB_KEVORKIAN");

    case kCheatMcGee:
    {
        if (!gMe->pXSprite->burnTime)
            evPost(gMe->nSprite, 3, 0, kCallbackFXFlameLick);
        actBurnSprite(gMe->pSprite->index, gMe->pXSprite, 2400);
        return GStrings("TXTB_FIRED");
    }
    case kCheatEdmark:
        actDamageSprite(gMe->nSprite, gMe->pSprite, kDamageExplode, 8000);
        return GStrings("TXTB_THEDAYS");

    case kCheatKrueger:
    {
        actHealDude(gMe->pXSprite, 200, 200);
        gMe->armor[1] = VanillaMode() ? 200 : 3200;
        if (!gMe->pXSprite->burnTime)
            evPost(gMe->nSprite, 3, 0, kCallbackFXFlameLick);
        actBurnSprite(gMe->pSprite->index, gMe->pXSprite, 2400);
        return GStrings("TXTB_RETARD");
    }
    case kCheatSterno:
        gMe->blindEffect = 250;
        break;
    case kCheat14: // quakeEffect (causing a little flickerEffect), not used by any cheat code (dead code)
        gMe->flickerEffect = 360;
        break;
    case kCheatSpork:
        actHealDude(gMe->pXSprite, 200, 200);
        break;
    case kCheatClarice:
        for (int i = 0; i < 3; i++)
            gMe->armor[i] = 1600;
        return GStrings("TXTB_HALFARMOR");
    case kCheatFrankenstein:
        gMe->packSlots[0].curAmount = 100;
        break;
    case kCheatCheeseHead:
        gMe->packSlots[1].curAmount = 100;
        if (!VanillaMode())
            gMe->pwUpTime[kPwUpDivingSuit] = gPowerUpInfo[kPwUpDivingSuit].bonusTime;
        break;
    case kCheatTequila:
        ToggleWooMode();
        break;
    case kCheatFunkyShoes:
        ToggleBoots();
        break;
    case kCheatKeyMaster:
        SetKeys(true);
        break;
    case kCheatOneRing:
        ToggleInvisibility();
        break;
    case kCheatVoorhees:
        ToggleInvulnerability();
        break;
    case kCheatJoJo:
        ToggleDelirium();
        break;
    case kCheatLaraCroft:
        SetInfiniteAmmo(!gInfiniteAmmo);
        SetWeapons(gInfiniteAmmo);
        break;
    case kCheatHongKong:
        SetWeapons(true);
        SetInfiniteAmmo(true);
        break;
    case kCheatMontana:
        SetWeapons(true);
        SetToys(true);
        break;
    case kCheatBunz:
        SetWeapons(true);
        SetWooMode(true);
        break;
    case kCheatCousteau:
        actHealDude(gMe->pXSprite, 200, 200);
        gMe->packSlots[1].curAmount = 100;
        if (!VanillaMode())
            gMe->pwUpTime[kPwUpDivingSuit] = gPowerUpInfo[kPwUpDivingSuit].bonusTime;
        break;
    case kCheatForkYou:
        SetInfiniteAmmo(false);
        SetMap(false);
        SetWeapons(false);
        SetAmmo(false);
        SetArmor(false);
        SetToys(false);
        SetKeys(false);
        SetWooMode(true);
        powerupActivate(gMe, kPwUpDeliriumShroom);
        gMe->pXSprite->health = 16;
        gMe->hasWeapon[1] = 1;
        gMe->curWeapon = 0;
        gMe->nextWeapon = 1;
        break;

    default:
        return nullptr;
    }
    return nullptr;
}

static bool cheatGoonies(cheatseq_t*)
{
    SetMap(!gFullMap);
    return true;
}

static bool cheatMario(cheatseq_t* c)
{
    int nEpisode, nLevel;
    if (parseArgs((char*)c->Args, &nEpisode, &nLevel) == 2)
	{
		auto map = FindMapByIndex(nEpisode, nLevel);
		if (map) DeferredStartGame(map, g_nextskill);
	}
    return true;
}

static bool cheatCalgon(cheatseq_t*)
{
    levelEndLevel(0);
    return true;
}


static cheatseq_t s_CheatInfo[] = {
    {"MPKFA",                 nullptr,			SendGenericCheat, 0, CHT_GOD },
    {"CAPINMYASS",            nullptr,			SendGenericCheat, 0, CHT_GODOFF },
    {"NOCAPINMYASS",          nullptr,			SendGenericCheat, 0, CHT_GODON },
    {"I WANNA BE LIKE KEVIN", nullptr,			SendGenericCheat, 0, CHT_GODON },
    {"IDAHO",                 "give weapons" },
    {"GRISWOLD",              "give armor" },
    {"MONTANA",               nullptr,          SendGenericCheat, 0, kCheatMontana }, // MONTANA (All weapons, full ammo and all items)
    {"EDMARK",                nullptr,          SendGenericCheat, 0, kCheatEdmark }, // EDMARK (Does a lot of fire damage to you (if you have 200HP and 200 fire armor then you can survive). Displays the message "THOSE WERE THE DAYS".)
    {"TEQUILA",               nullptr,          SendGenericCheat, 0, kCheatTequila }, // TEQUILA (Guns akimbo power-up)
    {"BUNZ",                  nullptr,          SendGenericCheat, 0, kCheatBunz }, // BUNZ (All weapons, full ammo, and guns akimbo power-up)
    {"FUNKY SHOES",           nullptr,          SendGenericCheat, 0, kCheatFunkyShoes }, // FUNKY SHOES (Gives jump boots item and activates it)
    {"GATEKEEPER",            nullptr,          SendGenericCheat, 0, kCheatGateKeeper }, // GATEKEEPER (Sets the you cheated flag to true, at the end of the level you will see that you have cheated)
    {"KEYMASTER",             nullptr,          SendGenericCheat, 0, kCheatKeyMaster }, // KEYMASTER (All keys)
    {"JOJO",                  nullptr,          SendGenericCheat, 0, kCheatJoJo }, // JOJO (Drunk mode (same effect as getting bitten by red spider))
    {"SATCHEL",               nullptr,          SendGenericCheat, 0, kCheatSatchel }, // SATCHEL (Full inventory)
    {"SPORK",                 nullptr,          SendGenericCheat, 0, kCheatSpork }, // SPORK (200% health (same effect as getting life seed))
    {"ONERING",               nullptr,          SendGenericCheat, 0, kCheatOneRing }, // ONERING (Cloak of invisibility power-up)
    {"MARIO###",              nullptr,          cheatMario }, // MARIO (Warp to level E M, e.g.: MARIO 1 3 will take you to Phantom Express)
    {"CALGON",                nullptr,          cheatCalgon }, // CALGON (Jumps to next level)
    {"KEVORKIAN",             nullptr,          SendGenericCheat, 0, kCheatKevorkian }, // KEVORKIAN (Does a lot of physical damage to you (if you have 200HP and 200 fire armor then you can survive). Displays the message "KEVORKIAN APPROVES".)
    {"MCGEE",                 nullptr,          SendGenericCheat, 0, kCheatMcGee }, // MCGEE (Sets you on fire. Displays the message "YOU'RE FIRED".)
    {"KRUEGER",               nullptr,          SendGenericCheat, 0, kCheatKrueger }, // KRUEGER (200% health, but sets you on fire. Displays the message "FLAME RETARDANT".)
    {"CHEESEHEAD",            nullptr,          SendGenericCheat, 0, kCheatCheeseHead }, // CHEESEHEAD (100% diving suit)
    {"COUSTEAU",              nullptr,          SendGenericCheat, 0, kCheatCousteau }, // COUSTEAU (200% health and diving suit)
    {"VOORHEES",              nullptr,          SendGenericCheat, 0, kCheatVoorhees }, // VOORHEES (Death mask power-up)
    {"LARA CROFT",            nullptr,          SendGenericCheat, 0, kCheatLaraCroft }, // LARA CROFT (All weapons and infinite ammo. Displays the message "LARA RULES". Typing it the second time will lose all weapons and ammo.)
    {"HONGKONG",              nullptr,          SendGenericCheat, 0, kCheatHongKong }, // HONGKONG (All weapons and infinite ammo)
    {"FRANKENSTEIN",          nullptr,          SendGenericCheat, 0, kCheatFrankenstein }, // FRANKENSTEIN (100% med-kit)
    {"STERNO",                nullptr,          SendGenericCheat, 0, kCheatSterno }, // STERNO (Temporary blindness (same effect as getting bitten by green spider))
    {"CLARICE",               nullptr,          SendGenericCheat, 0, kCheatClarice }, // CLARICE (Gives 100% body armor, 100% fire armor, 100% spirit armor)
    {"FORK YOU",              nullptr,          SendGenericCheat, 0, kCheatForkYou }, // FORK YOU (Drunk mode, 1HP, no armor, no weapons, no ammo, no items, no keys, no map, guns akimbo power-up)
    {"LIEBERMAN",             nullptr,          SendGenericCheat, 0, kCheatLieberMan }, // LIEBERMAN (Sets the you cheated flag to true, at the end of the level you will see that you have cheated)
    {"EVA GALLI",             nullptr,			SendGenericCheat, 0, CHT_NOCLIP },
    {"RATE",                  "toggle r_showfps", nullptr, 1 }, // RATE (Display frame rate (doesn't count as a cheat))
    {"GOONIES",               nullptr,           cheatGoonies, 0 }, // GOONIES (Enable full map. Displays the message "YOU HAVE THE MAP".)
    //{"SPIELBERG",           nullptr,           doCheat<kCheatSpielberg, 1 }, // SPIELBERG (Disables all cheats. If number values corresponding to a level and episode number are entered after the cheat word (i.e. "spielberg 1 3" for Phantom Express), you will be spawned to said level and the game will begin recording a demo from your actions.)
}; 

void cheatReset(void)
{
    bPlayerCheated = 0;
    playerSetGodMode(gMe, 0);
    gNoClip = 0;
    packClear(gMe);
    gInfiniteAmmo = 0;
    gFullMap = 0;
}


static void cmd_Give(int player, uint8_t **stream, bool skip)
{
    int type = ReadByte(stream);
    if (skip) return;

    if (numplayers != 1 || gamestate != GS_LEVEL || gMe->pXSprite->health == 0)
    {
        Printf("give: Cannot give while dead or not in a single-player game.\n");
        return;
    }

    switch (type)
    {
    case GIVE_ALL:
        SetWeapons(true);
        SetAmmo(true);
        SetToys(true);
        SetArmor(true);
        SetKeys(true);
        bPlayerCheated = true;
        break;

    case GIVE_HEALTH:
        actHealDude(gMe->pXSprite, 200, 200);
        bPlayerCheated = true;
        break;

    case GIVE_WEAPONS:
        SetWeapons(true);
        bPlayerCheated = true;
        break;

    case GIVE_AMMO:
        SetAmmo(true);
        bPlayerCheated = true;
        break;

    case GIVE_ARMOR:
        SetArmor(true);
        bPlayerCheated = true;
        break;

    case GIVE_KEYS:
        SetKeys(true);
        bPlayerCheated = true;
        break;

    case GIVE_INVENTORY:
        SetToys(true);
        bPlayerCheated = true;
        break;

    default:
        break;
    }
}


void InitCheats()
{
    SetCheats(s_CheatInfo, countof(s_CheatInfo));
    Net_SetCommandHandler(DEM_GIVE, cmd_Give);
}

END_BLD_NS
