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
#include "build.h"
#include "mmulti.h"
#include "compat.h"
#include "keyboard.h"
#include "control.h"
#include "function.h"
#include "common_game.h"
#include "blood.h"
#include "config.h"
#include "demo.h"
#include "eventq.h"
#include "globals.h"
#include "levels.h"
#include "loadsave.h"
#include "menu.h"
#include "messages.h"
#include "network.h"
#include "player.h"
#include "view.h"

CPlayerMsg gPlayerMsg;
CCheatMgr gCheatMgr;

void sub_5A928(void)
{
    for (int i = 0; i < NUMGAMEFUNCTIONS-1; i++)
        CONTROL_ClearButton(i);
}

extern uint8_t KeyboardKeys[NUMGAMEFUNCTIONS][2];
void sub_5A944(char key)
{
    for (int i = 0; i < NUMGAMEFUNCTIONS-1; i++)
    {
        char key1, key2;
        key1 = KeyboardKeys[i][0];
        key2 = KeyboardKeys[i][1];
        if (key1 == key || key2 == key)
            CONTROL_ClearButton(i);
    }
}

void SetGodMode(bool god)
{
    playerSetGodMode(gMe, god);
    if (gMe->at31a)
        viewSetMessage("You are immortal.");
    else
        viewSetMessage("You are mortal.");
}

void SetClipMode(bool noclip)
{
    gNoClip = noclip;
    if (gNoClip)
        viewSetMessage("Unclipped movement.");
    else
        viewSetMessage("Normal movement.");
}

void packStuff(PLAYER *pPlayer)
{
    for (int i = 0; i < 5; i++)
        packAddItem(pPlayer, i);
}

void packClear(PLAYER *pPlayer)
{
    pPlayer->at321 = 0;
    for (int i = 0; i < 5; i++)
    {
        pPlayer->packInfo[i].at0 = 0;
        pPlayer->packInfo[i].at1 = 0;
    }
}

void SetAmmo(bool stat)
{
    if (stat)
    {
        for (int i = 0; i < 12; i++)
            gMe->at181[i] = gAmmoInfo[i].at0;
        viewSetMessage("You have full ammo.");
    }
    else
    {
        for (int i = 0; i < 12; i++)
            gMe->at181[i] = 0;
        viewSetMessage("You have no ammo.");
    }
}

void SetWeapons(bool stat)
{
    for (int i = 0; i < 14; i++)
    {
        gMe->atcb[i] = stat;
    }
    SetAmmo(stat);
    if (stat)
        viewSetMessage("You have all weapons.");
    else
    {
        if (!VanillaMode())
        {
            // Keep the pitchfork to avoid freeze
            gMe->atcb[1] = 1;
            gMe->atbd = 0;
            gMe->atbe = 1;
        }
        viewSetMessage("You have no weapons.");
    }
}

void SetToys(bool stat)
{
    if (stat)
    {
        packStuff(gMe);
        viewSetMessage("Your inventory is full.");
    }
    else
    {
        packClear(gMe);
        viewSetMessage("Your inventory is empty.");
    }
}

void SetArmor(bool stat)
{
    int nAmount;
    if (stat)
    {
        viewSetMessage("You have full armor.");
        nAmount = 3200;
    }
    else
    {
        viewSetMessage("You have no armor.");
        nAmount = 0;
    }
    for (int i = 0; i < 3; i++)
        gMe->at33e[i] = nAmount;
}

void SetKeys(bool stat)
{
    for (int i = 1; i <= 6; i++)
        gMe->at88[i] = stat;
    if (stat)
        viewSetMessage("You have all keys.");
    else
        viewSetMessage("You have no keys.");
}

void SetInfiniteAmmo(bool stat)
{
    gInfiniteAmmo = stat;
    if (gInfiniteAmmo)
        viewSetMessage("You have infinite ammo.");
    else
        viewSetMessage("You have limited ammo.");
}

void SetMap(bool stat)
{
    gFullMap = stat;
    if (gFullMap)
        viewSetMessage("You have the map.");
    else
        viewSetMessage("You have no map.");
}

void SetWooMode(bool stat)
{
    if (stat)
    {
        if (!powerupCheck(gMe, 17))
            powerupActivate(gMe, 17);
    }
    else
    {
        if (powerupCheck(gMe, 17))
        {
            if (!VanillaMode())
                gMe->at202[17] = 0;
            powerupDeactivate(gMe, 17);
        }
    }
}

void ToggleWooMode(void)
{
    SetWooMode(!(powerupCheck(gMe, 17) != 0));
}

void ToggleBoots(void)
{
    if (powerupCheck(gMe, 15))
    {
        viewSetMessage("You have no Jumping Boots.");
        if (!VanillaMode())
        {
            gMe->at202[15] = 0;
            gMe->packInfo[4].at1 = 0;
        }
        powerupDeactivate(gMe, 15);
    }
    else
    {
        viewSetMessage("You have the Jumping Boots.");
        if (!VanillaMode())
            gMe->at202[15] = gPowerUpInfo[15].at3;
        powerupActivate(gMe, 15);
    }
}

void ToggleInvisibility(void)
{
    if (powerupCheck(gMe, 13))
    {
        viewSetMessage("You are visible.");
        if (!VanillaMode())
            gMe->at202[13] = 0;
        powerupDeactivate(gMe, 13);
    }
    else
    {
        viewSetMessage("You are invisible.");
        powerupActivate(gMe, 13);
    }
}

void ToggleInvulnerability(void)
{
    if (powerupCheck(gMe, 14))
    {
        viewSetMessage("You are vulnerable.");
        if (!VanillaMode())
            gMe->at202[14] = 0;
        powerupDeactivate(gMe, 14);
    }
    else
    {
        viewSetMessage("You are invulnerable.");
        powerupActivate(gMe, 14);
    }
}

void ToggleDelirium(void)
{
    if (powerupCheck(gMe, 28))
    {
        viewSetMessage("You are not delirious.");
        if (!VanillaMode())
            gMe->at202[28] = 0;
        powerupDeactivate(gMe, 28);
    }
    else
    {
        viewSetMessage("You are delirious.");
        powerupActivate(gMe, 28);
    }
}

void LevelWarp(int nEpisode, int nLevel)
{
    levelSetupOptions(nEpisode, nLevel);
    StartLevel(&gGameOptions);
    viewResizeView(gViewSize);
}

void LevelWarpAndRecord(int nEpisode, int nLevel)
{
    char buffer[BMAX_PATH];
    levelSetupOptions(nEpisode, nLevel);
    gGameStarted = false;
    strcpy(buffer, levelGetFilename(nEpisode, nLevel));
    ChangeExtension(buffer, ".DEM");
    gDemo.Create(buffer);
    StartLevel(&gGameOptions);
    viewResizeView(gViewSize);
}

CGameMessageMgr::CGameMessageMgr()
{
    if (!VanillaMode())
        Clear();
    x = 1;
    y = 0;
    at9 = 0;
    atd = 0;
    nFont = 0;
    fontHeight = 8;
    maxNumberOfMessagesToDisplay = 4;
    visibilityDurationInSecs = 5;
    messageFlags = 15;
    numberOfDisplayedMessages = 0;
    nextMessagesIndex = messagesIndex = 0;
}

void CGameMessageMgr::SetState(char state)
{
    if (this->state && !state)
    {
        this->state = 0;
        Clear();
    }
    else if (!this->state && state)
        this->state = 1;
}

void CGameMessageMgr::Add(const char *pText, char a2, const int pal, const MESSAGE_PRIORITY priority)
{
    if (a2 && messageFlags)
    {
        messageStruct *pMessage = &messages[nextMessagesIndex];
        strncpy(pMessage->text, pText, kMaxMessageTextLength-1);
        pMessage->text[kMaxMessageTextLength-1] = 0;
        pMessage->lastTickWhenVisible = gFrameClock + visibilityDurationInSecs*kTicRate;
        pMessage->pal = pal;
        pMessage->priority = priority;
        pMessage->deleted = false;
        nextMessagesIndex = (nextMessagesIndex+1)%kMessageLogSize;
        if (VanillaMode())
        {
            numberOfDisplayedMessages++;
            if (numberOfDisplayedMessages > maxNumberOfMessagesToDisplay)
            {
                messagesIndex = (messagesIndex+1)%kMessageLogSize;
                atd = 0;
                numberOfDisplayedMessages = maxNumberOfMessagesToDisplay;
                at9 = fontHeight;
            }
        }
    }
}

void CGameMessageMgr::Display(void)
{
    if (VanillaMode())
    {
        if (numberOfDisplayedMessages && this->state && gInputMode != INPUT_MODE_2)
        {
            int initialNrOfDisplayedMsgs = numberOfDisplayedMessages;
            int initialMessagesIndex = messagesIndex;
            int shade = ClipHigh(initialNrOfDisplayedMsgs*8, 48);
            int x = gViewMode == 3 ? gViewX0S : 0;
            int y = (gViewMode == 3 ? this->y : 0) + (int)at9;
            for (int i = 0; i < initialNrOfDisplayedMsgs; i++)
            {
                messageStruct* pMessage = &messages[(initialMessagesIndex+i)%kMessageLogSize];
                if (pMessage->lastTickWhenVisible < gFrameClock)
                {
                    messagesIndex = (messagesIndex+1)%kMessageLogSize;
                    numberOfDisplayedMessages--;
                    continue;
                }
                viewDrawText(nFont, pMessage->text, x, y, shade, pMessage->pal, 0, false, 256);
                if (gViewMode == 3)
                {
                    int height;
                    gMenuTextMgr.GetFontInfo(nFont, pMessage->text, &height, NULL);
                    if (x+height > gViewX1S)
                        viewUpdatePages();
                }
                y += fontHeight;
                shade = ClipLow(shade-64/initialNrOfDisplayedMsgs, -128);
            }
        }
    }
    else
    {
        if (this->state && gInputMode != INPUT_MODE_2)
        {
            messageStruct* currentMessages[kMessageLogSize];
            int currentMessagesCount = 0;
            for (int i = 0; i < kMessageLogSize; i++)
            {
                messageStruct* pMessage = &messages[i];
                if (gFrameClock < pMessage->lastTickWhenVisible && !pMessage->deleted)
                {
                    currentMessages[currentMessagesCount++] = pMessage;
                }
            }

            SortMessagesByPriority(currentMessages, currentMessagesCount);

            messageStruct* messagesToDisplay[kMessageLogSize];
            int messagesToDisplayCount = 0;
            for (int i = 0; i < currentMessagesCount && messagesToDisplayCount < maxNumberOfMessagesToDisplay; i++)
            {
                messagesToDisplay[messagesToDisplayCount++] = currentMessages[i];
            }

            SortMessagesByTime(messagesToDisplay, messagesToDisplayCount);

            int shade = ClipHigh(messagesToDisplayCount*8, 48);
            int x = gViewMode == 3 ? gViewX0S : 0;
            int y = (gViewMode == 3 ? this->y : 0) + (int)at9;
            for (int i = 0; i < messagesToDisplayCount; i++)
            {
                messageStruct* pMessage = messagesToDisplay[i];
                viewDrawText(nFont, pMessage->text, x, y, shade, pMessage->pal, 0, false, 256);
                if (gViewMode == 3)
                {
                    int height;
                    gMenuTextMgr.GetFontInfo(nFont, pMessage->text, &height, NULL);
                    if (x+height > gViewX1S)
                        viewUpdatePages();
                }
                y += fontHeight;
                shade = ClipLow(shade-64/messagesToDisplayCount, -128);
            }
        }
    }
    if (at9 != 0)
    {
        at9 = fontHeight*at9/kTicRate;
        atd += gFrameTicks;
    }
}

void CGameMessageMgr::Clear(void)
{
    if (VanillaMode())
    {
        messagesIndex = nextMessagesIndex = numberOfDisplayedMessages = 0;
    }
    else
    {
        for (int i = 0; i < kMessageLogSize; i++)
        {
            messageStruct* pMessage = &messages[i];
            pMessage->deleted = true;
        }
    }
}

void CGameMessageMgr::SetMaxMessages(int nMessages)
{
    maxNumberOfMessagesToDisplay = ClipRange(nMessages, 1, 16);
}

void CGameMessageMgr::SetFont(int nFont)
{
    this->nFont = nFont;
    fontHeight = gFont[nFont].ySize;
}

void CGameMessageMgr::SetCoordinates(int x, int y)
{
    this->x = ClipRange(x, 0, gViewX1S);
    this->y = ClipRange(y, 0, gViewY1S);
}

void CGameMessageMgr::SetMessageTime(int nTime)
{
    visibilityDurationInSecs = ClipRange(nTime, 1, 8);
}

void CGameMessageMgr::SetMessageFlags(unsigned int nFlags)
{
    messageFlags = nFlags&0xf;
}

void CGameMessageMgr::SortMessagesByPriority(messageStruct** messages, int count) {
    for (int i = 1; i < count; i++)
    {
        for (int j = 0; j < count - i; j++)
        {
            if (messages[j]->priority != messages[j + 1]->priority ? messages[j]->priority < messages[j + 1]->priority : messages[j]->lastTickWhenVisible < messages[j + 1]->lastTickWhenVisible)
            {
                messageStruct* temp = messages[j];
                messages[j] = messages[j + 1];
                messages[j + 1] = temp;
            }
        }
    }
}

void CGameMessageMgr::SortMessagesByTime(messageStruct** messages, int count) {
    for (int i = 1; i < count; i++)
    {
        for (int j = 0; j < count - i; j++)
        {
            if (messages[j]->lastTickWhenVisible > messages[j + 1]->lastTickWhenVisible)
            {
                messageStruct* temp = messages[j];
                messages[j] = messages[j + 1];
                messages[j + 1] = temp;
            }
        }
    }
}

void CPlayerMsg::Clear(void)
{
    text[0] = 0;
    at0 = 0;
}

void CPlayerMsg::Term(void)
{
    Clear();
    gInputMode = INPUT_MODE_0;
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
    viewUpdatePages();
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
    keyFlushScans();
}

void CPlayerMsg::ProcessKeys(void)
{
    int key = keyGetScan();
    char ch;
    if (key != 0)
    {
        bool UNUSED(alt) = keystatus[sc_LeftAlt] || keystatus[sc_RightAlt];
        bool ctrl = keystatus[sc_LeftControl] || keystatus[sc_RightControl];
        bool shift = keystatus[sc_LeftShift] || keystatus[sc_RightShift];
        switch (key)
        {
        case sc_Escape:
            Term();
            break;
        case sc_F1:
        case sc_F2:
        case sc_F3:
        case sc_F4:
        case sc_F5:
        case sc_F6:
        case sc_F7:
        case sc_F8:
        case sc_F9:
        case sc_F10:
            CONTROL_ClearButton(gamefunc_See_Chase_View);
            Set(CommbatMacro[key-sc_F1]);
            Send();
            keystatus[key] = 0;
            break;
        case sc_BackSpace:
            if (ctrl)
                Clear();
            else
                DelChar();
            break;
        case sc_Enter:
        case sc_kpad_Enter:
            if (gCheatMgr.Check(text))
                Term();
            else
                Send();
            break;
        default:
            if (key < 128)
            {
                ch =  shift ? g_keyAsciiTableShift[key] : g_keyAsciiTable[key];
                if (ch)
                    AddChar(ch);
            }
            break;
        }
        sub_5A944(key);
    }
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
    Bstrupr(buffer);
    for (size_t i = 0; i < strlen(pzString); i++)
        buffer[i]++;
    for (int i = 0; i < 36; i++)
    {
        int nCheatLen = strlen(s_CheatInfo[i].pzString);
        if (s_CheatInfo[i].flags & 1)
        {
            if (!strncmp(buffer, s_CheatInfo[i].pzString, nCheatLen))
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

    if (gDemo.at0) return;
    if (nCheatCode == kCheatRate)
    {
        gShowFps = !gShowFps;
        return;
    }
    if (gGameOptions.nGameType != 0)
        return;
    int nEpisode, nLevel;
    switch (nCheatCode)
    {
    case kCheatSpielberg:
        if (parseArgs(pzArgs, &nEpisode, &nLevel) == 2)
            LevelWarpAndRecord(nEpisode, nLevel);
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
        SetGodMode(!gMe->at31a);
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
        actDamageSprite(gMe->at5b, gMe->pSprite, DAMAGE_TYPE_2, 8000);
        viewSetMessage("Kevorkian approves.");
        break;
    case kCheatMcGee:
    {
        if (!gMe->pXSprite->burnTime)
            evPost(gMe->at5b, 3, 0, CALLBACK_ID_0);
        actBurnSprite(actSpriteIdToOwnerId(gMe->at5b), gMe->pXSprite, 2400);
        viewSetMessage("You're fired!");
        break;
    }
    case kCheatEdmark:
        actDamageSprite(gMe->at5b, gMe->pSprite, DAMAGE_TYPE_3, 8000);
        viewSetMessage("Ahhh...those were the days.");
        break;
    case kCheatKrueger:
    {
        actHealDude(gMe->pXSprite, 200, 200);
        gMe->at33e[1] = VanillaMode() ? 200 : 3200;
        if (!gMe->pXSprite->burnTime)
            evPost(gMe->at5b, 3, 0, CALLBACK_ID_0);
        actBurnSprite(actSpriteIdToOwnerId(gMe->at5b), gMe->pXSprite, 2400);
        viewSetMessage("Flame retardant!");
        break;
    }
    case kCheatSterno:
        gMe->at36a = 250;
        break;
    case kCheat14: // quake (causing a little flicker), not used by any cheat code (dead code)
        gMe->at35a = 360;
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
            viewSetMessage("You have half armor.");
            for (int i = 0; i < 3; i++)
                gMe->at33e[i] = 1600;
        }
        break;
    case kCheatFrankenstein:
        gMe->packInfo[0].at1 = 100;
        break;
    case kCheatCheeseHead:
        gMe->packInfo[1].at1 = 100;
        if (!VanillaMode())
            gMe->at202[18] = gPowerUpInfo[18].at3;
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
        gMe->packInfo[1].at1 = 100;
        if (!VanillaMode())
            gMe->at202[18] = gPowerUpInfo[18].at3;
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
        powerupActivate(gMe, 28);
        gMe->pXSprite->health = 16;
        gMe->atcb[1] = 1;
        gMe->atbd = 0;
        gMe->atbe = 1;
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
