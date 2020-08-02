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
#include "mmulti.h"
#include "compat.h"
#include "gamecontrol.h"
#include "common_game.h"
#include "blood.h"
#include "eventq.h"
#include "globals.h"
#include "levels.h"
#include "loadsave.h"
#include "messages.h"
#include "network.h"
#include "player.h"
#include "view.h"
#include "gstrings.h"

BEGIN_BLD_NS

CPlayerMsg gPlayerMsg;
CCheatMgr gCheatMgr;

void sub_5A928(void)
{
    for (int i = 0; i < buttonMap.NumButtons(); i++)
        buttonMap.ClearButton(i);
}

void SetGodMode(bool god)
{
    playerSetGodMode(gMe, god);
    if (gMe->godMode)
		viewSetMessage(GStrings("TXTB_GODMODE"));
    else
        viewSetMessage(GStrings("TXTB_NOTGODMODE"));
}

void SetClipMode(bool noclip)
{
    gNoClip = noclip;
    if (gNoClip)
		viewSetMessage(GStrings("TXTB_NOCLIP"));
    else
        viewSetMessage(GStrings("TXTB_NOCLIPOFF"));
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

void LevelWarp(int nEpisode, int nLevel)
{
    levelSetupOptions(nEpisode, nLevel);
    StartLevel(&gGameOptions);
    viewResizeView(gViewSize);
}

void CPlayerMsg::Clear(void)
{
    text[0] = 0;
    at0 = 0;
}

void CPlayerMsg::Term(void)
{
    Clear();
}

void CPlayerMsg::Draw(void)
{
    char buffer[44];
    strcpy(buffer, text);
    if ((int)totalclock & 16)
        strcat(buffer, "_");
    int x = gViewMode == 3 ? gViewX0S : 0;
    int y = gViewMode == 3 ? gViewY0S : 0;
    if (gViewSize >= 1)
        y += tilesiz[2229].y*((gNetPlayers+3)/4);
    viewDrawText(0, buffer, x+1,y+1, -128, 0, 0, false, 256);
}

bool CPlayerMsg::AddChar(char ch)
{
    if (at0 < 40)
    {
        text[at0++] = ch;
        text[at0] = 0;
        return true;
    }
    return false;
}

void CPlayerMsg::DelChar(void)
{
    if (at0 > 0)
        text[--at0] = 0;
}

void CPlayerMsg::Set(const char * pzString)
{
    strncpy(text, pzString, 40);
    at0 = ClipHigh(strlen(pzString), 40);
    text[at0] = 0;
}

void CPlayerMsg::Send(void)
{
    if (VanillaMode() || !IsWhitespaceOnly(text))
    {
        netBroadcastMessage(myconnectindex, text);
        if (!VanillaMode())
        {
            char *myName = gProfile[myconnectindex].name;
            char szTemp[128];
            sprintf(szTemp, "%s: %s", myName, text);
            viewSetMessage(szTemp, 10); // 10: dark blue
        }
        else
            viewSetMessage(text);
    }

    Term();
    inputState.keyFlushScans();
}

void CPlayerMsg::ProcessKeys(void)
{
	if (inputState.GetKeyStatus(sc_Escape)) Term();
}

bool CPlayerMsg::IsWhitespaceOnly(const char * const pzString)
{
    const char *p = pzString;
    while (*p != 0)
        if (*p++ > 32)
            return false;
    return true;
}

CCheatMgr::CHEATINFO CCheatMgr::s_CheatInfo[] = {
    {"NQLGB", kCheatMpkfa, 0 }, // MPKFA (Invincibility)
    {"DBQJONZBTT", kCheatCapInMyAss, 0 }, // CAPINMYASS (Disable invincibility )
    {"OPDBQJONZBTT", kCheatNoCapInMyAss, 0 }, // NOCAPINMYASS (Invincibility)
    {"J!XBOOB!CF!MJLF!LFWJO", kCheatNoCapInMyAss, 0 }, // I WANNA BE LIKE KEVIN (Invincibility)
    {"JEBIP", kCheatIdaho, 0 }, // IDAHO (All weapons and full ammo)
    {"NPOUBOB", kCheatMontana, 0 }, // MONTANA (All weapons, full ammo and all items)
    {"HSJTXPME", kCheatGriswold, 0 }, // GRISWOLD (Full armor (same effect as getting super armor))
    {"FENBSL", kCheatEdmark, 0 }, // EDMARK (Does a lot of fire damage to you (if you have 200HP and 200 fire armor then you can survive). Displays the message "THOSE WERE THE DAYS".)
    {"UFRVJMB", kCheatTequila, 0 }, // TEQUILA (Guns akimbo power-up)
    {"CVO[", kCheatBunz, 0 }, // BUNZ (All weapons, full ammo, and guns akimbo power-up)
    {"GVOLZ!TIPFT", kCheatFunkyShoes, 0 }, // FUNKY SHOES (Gives jump boots item and activates it)
    {"HBUFLFFQFS", kCheatGateKeeper, 0 }, // GATEKEEPER (Sets the you cheated flag to true, at the end of the level you will see that you have cheated)
    {"LFZNBTUFS", kCheatKeyMaster, 0 }, // KEYMASTER (All keys)
    {"KPKP", kCheatJoJo, 0 }, // JOJO (Drunk mode (same effect as getting bitten by red spider))
    {"TBUDIFM", kCheatSatchel, 0 }, // SATCHEL (Full inventory)
    {"TQPSL", kCheatSpork, 0 }, // SPORK (200% health (same effect as getting life seed))
    {"POFSJOH", kCheatOneRing, 0 }, // ONERING (Cloak of invisibility power-up)
    {"NBSJP", kCheatMario, 1 }, // MARIO (Warp to level E M, e.g.: MARIO 1 3 will take you to Phantom Express)
    {"DBMHPO", kCheatCalgon, 1 }, // CALGON (Jumps to next level or can be used like MARIO with parameters)
    {"LFWPSLJBO", kCheatKevorkian, 0 }, // KEVORKIAN (Does a lot of physical damage to you (if you have 200HP and 200 fire armor then you can survive). Displays the message "KEVORKIAN APPROVES".)
    {"NDHFF", kCheatMcGee, 0 }, // MCGEE (Sets you on fire. Displays the message "YOU'RE FIRED".)
    {"LSVFHFS", kCheatKrueger, 0 }, // KRUEGER (200% health, but sets you on fire. Displays the message "FLAME RETARDANT".)
    {"DIFFTFIFBE", kCheatCheeseHead, 0 }, // CHEESEHEAD (100% diving suit)
    {"DPVTUFBV", kCheatCousteau, 0 }, // COUSTEAU (200% health and diving suit)
    {"WPPSIFFT", kCheatVoorhees, 0 }, // VOORHEES (Death mask power-up)
    {"MBSB!DSPGU", kCheatLaraCroft, 0 }, // LARA CROFT (All weapons and infinite ammo. Displays the message "LARA RULES". Typing it the second time will lose all weapons and ammo.)
    {"IPOHLPOH", kCheatHongKong, 0 }, // HONGKONG (All weapons and infinite ammo)
    {"GSBOLFOTUFJO", kCheatFrankenstein, 0 }, // FRANKENSTEIN (100% med-kit)
    {"TUFSOP", kCheatSterno, 0 }, // STERNO (Temporary blindness (same effect as getting bitten by green spider))
    {"DMBSJDF", kCheatClarice, 0 }, // CLARICE (Gives 100% body armor, 100% fire armor, 100% spirit armor)
    {"GPSL!ZPV", kCheatForkYou, 0 }, // FORK YOU (Drunk mode, 1HP, no armor, no weapons, no ammo, no items, no keys, no map, guns akimbo power-up)
    {"MJFCFSNBO", kCheatLieberMan, 0 }, // LIEBERMAN (Sets the you cheated flag to true, at the end of the level you will see that you have cheated)
    {"FWB!HBMMJ", kCheatEvaGalli, 0 }, // EVA GALLI (Disable/enable clipping (grant the ability to walk through walls))
    {"SBUF", kCheatRate, 0 }, // RATE (Display frame rate (doesn't count as a cheat))
    {"HPPOJFT", kCheatGoonies, 0 }, // GOONIES (Enable full map. Displays the message "YOU HAVE THE MAP".)
    {"TQJFMCFSH", kCheatSpielberg, 1 }, // SPIELBERG (Disables all cheats. If number values corresponding to a level and episode number are entered after the cheat word (i.e. "spielberg 1 3" for Phantom Express), you will be spawned to said level and the game will begin recording a demo from your actions.)
};

bool CCheatMgr::m_bPlayerCheated = false;

bool CCheatMgr::Check(char *pzString)
{
    char buffer[80];
    strcpy(buffer, pzString);
    for (size_t i = 0; i < strlen(pzString); i++)
        buffer[i]++;
    for (int i = 0; i < 36; i++)
    {
        int nCheatLen = strlen(s_CheatInfo[i].pzString);
        if (s_CheatInfo[i].flags & 1)
        {
            if (!strnicmp(buffer, s_CheatInfo[i].pzString, nCheatLen))
            {
                Process(s_CheatInfo[i].id, buffer+nCheatLen);
                return true;
            }
        }
        if (!strcmp(buffer, s_CheatInfo[i].pzString))
        {
            Process(s_CheatInfo[i].id, NULL);
            return true;
        }
    }
    return false;
}

int parseArgs(char *pzArgs, int *nArg1, int *nArg2)
{
    if (!nArg1 || !nArg2)
        return -1;
    int nLength = strlen(pzArgs);
    for (int i = 0; i < nLength; i++)
        pzArgs[i]--;
    int stat = sscanf(pzArgs, " %d %d", nArg1, nArg2);
    if (stat == 2 && (*nArg1 == 0 || *nArg2 == 0))
        return -1;
    *nArg1 = ClipRange(*nArg1-1, 0, gEpisodeCount-1);
    *nArg2 = ClipRange(*nArg2-1, 0, gEpisodeInfo[*nArg1].nLevels-1);
    return stat;
}

void CCheatMgr::Process(CCheatMgr::CHEATCODE nCheatCode, char* pzArgs)
{
    dassert(nCheatCode > kCheatNone && nCheatCode < kCheatMax);

    if (nCheatCode == kCheatRate)
    {
        r_showfps = !r_showfps;
        return;
    }
    if (gGameOptions.nGameType != 0)
        return;
    int nEpisode, nLevel;
    switch (nCheatCode)
    {
    case kCheatSpielberg:
		// demo record
        break;
    case kCheat1:
        SetAmmo(true);
        break;
    case kCheatGriswold:
        SetArmor(true);
        break;
    case kCheatSatchel:
        SetToys(true);
        break;
    case kCheatEvaGalli:
        SetClipMode(!gNoClip);
        break;
    case kCheatMpkfa:
        SetGodMode(!gMe->godMode);
        break;
    case kCheatCapInMyAss:
        SetGodMode(false);
        break;
    case kCheatNoCapInMyAss:
        SetGodMode(true);
        break;
    case kCheatIdaho:
        SetWeapons(true);
        break;
    case kCheatKevorkian:
        actDamageSprite(gMe->nSprite, gMe->pSprite, DAMAGE_TYPE_2, 8000);
        viewSetMessage(GStrings("TXTB_KEVORKIAN"));
        break;
    case kCheatMcGee:
    {
        if (!gMe->pXSprite->burnTime)
            evPost(gMe->nSprite, 3, 0, kCallbackFXFlameLick);
        actBurnSprite(actSpriteIdToOwnerId(gMe->nSprite), gMe->pXSprite, 2400);
        viewSetMessage(GStrings("TXTB_FIRED"));
        break;
    }
    case kCheatEdmark:
        actDamageSprite(gMe->nSprite, gMe->pSprite, DAMAGE_TYPE_3, 8000);
        viewSetMessage(GStrings("TXTB_THEDAYS"));
        break;
    case kCheatKrueger:
    {
        actHealDude(gMe->pXSprite, 200, 200);
        gMe->armor[1] = VanillaMode() ? 200 : 3200;
        if (!gMe->pXSprite->burnTime)
            evPost(gMe->nSprite, 3, 0, kCallbackFXFlameLick);
        actBurnSprite(actSpriteIdToOwnerId(gMe->nSprite), gMe->pXSprite, 2400);
        viewSetMessage(GStrings("TXTB_RETARD"));
        break;
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
    case kCheatGoonies:
        SetMap(!gFullMap);
        break;
    case kCheatClarice:
        if (!VanillaMode())
        {
            viewSetMessage(GStrings("TXTB_HALFARMOR"));
            for (int i = 0; i < 3; i++)
                gMe->armor[i] = 1600;
        }
        break;
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
    case kCheatRate: // show FPS, handled before (dead code), leave here for safety
        return;
    case kCheatMario:
        if (parseArgs(pzArgs, &nEpisode, &nLevel) == 2)
            LevelWarp(nEpisode, nLevel);
        break;
    case kCheatCalgon:
        if (parseArgs(pzArgs, &nEpisode, &nLevel) == 2)
            LevelWarp(nEpisode, nLevel);
        else
            if (!VanillaMode())
                levelEndLevel(0);
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
        actHealDude(gMe->pXSprite,200,200);
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
        break;
    }
    m_bPlayerCheated = true;
}

void CCheatMgr::sub_5BCF4(void)
{
    m_bPlayerCheated = 0;
    playerSetGodMode(gMe, 0);
    gNoClip = 0;
    packClear(gMe);
    gInfiniteAmmo = 0;
    gFullMap = 0;
}

class MessagesLoadSave : public LoadSave
{
public:
    virtual void Load();
    virtual void Save();
};

void MessagesLoadSave::Load()
{
    Read(&CCheatMgr::m_bPlayerCheated, sizeof(CCheatMgr::m_bPlayerCheated));
}

void MessagesLoadSave::Save()
{
    Write(&CCheatMgr::m_bPlayerCheated, sizeof(CCheatMgr::m_bPlayerCheated));
}

static MessagesLoadSave *myLoadSave;

void MessagesLoadSaveConstruct(void)
{
    myLoadSave = new MessagesLoadSave();
}

END_BLD_NS
