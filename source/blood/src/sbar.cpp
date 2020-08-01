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

#include <stdlib.h>
#include <string.h>

#include "compat.h"
#include "build.h"
#include "pragmas.h"
#include "mmulti.h"
#include "v_font.h"

#include "endgame.h"
#include "aistate.h"
#include "map2d.h"
#include "loadsave.h"
#include "screen.h"
#include "sectorfx.h"
#include "choke.h"
#include "view.h"
#include "nnexts.h"
#include "zstring.h"
#include "menu.h"
#include "gstrings.h"
#include "v_2ddrawer.h"
#include "v_video.h"
#include "v_font.h"
#include "glbackend/glbackend.h"

CVARD(Bool, hud_powerupduration, true, CVAR_ARCHIVE/*|CVAR_FRONTEND_BLOOD*/, "enable/disable displaying the remaining seconds for power-ups")

BEGIN_BLD_NS


void viewDrawStats(PLAYER* pPlayer, int x, int y)
{
    const int nFont = 3;
    char buffer[128];
    if (!hud_stats)
        return;

    int nHeight;
    viewGetFontInfo(nFont, NULL, NULL, &nHeight);
    sprintf(buffer, "T:%d:%02d.%02d",
        (gLevelTime / (kTicsPerSec * 60)),
        (gLevelTime / kTicsPerSec) % 60,
        ((gLevelTime % kTicsPerSec) * 33) / 10
    );
    viewDrawText(3, buffer, x, y, 20, 0, 0, true, 256);
    y += nHeight + 1;
    if (gGameOptions.nGameType != 3)
        sprintf(buffer, "K:%d/%d", gKillMgr.at4, gKillMgr.at0);
    else
        sprintf(buffer, "F:%d", pPlayer->fragCount);
    viewDrawText(3, buffer, x, y, 20, 0, 0, true, 256);
    y += nHeight + 1;
    sprintf(buffer, "S:%d/%d", gSecretMgr.at4 + gSecretMgr.at8, gSecretMgr.at0);
    viewDrawText(3, buffer, x, y, 20, 0, 0, true, 256);
}






int gPackIcons[5] = {
    2569, 2564, 2566, 2568, 2560
};

struct PACKICON2 {
    short nTile;
    int nScale;
    int nYOffs;
};

PACKICON2 gPackIcons2[] = {
    { 519, (int)(65536 * 0.5), 0 },
    { 830, (int)(65536 * 0.3), 0 },
    { 760, (int)(65536 * 0.6), 0 },
    { 839, (int)(65536 * 0.5), -4 },
    { 827, (int)(65536 * 0.4), 0 },
};

struct AMMOICON {
    short nTile;
    int nScale;
    int nYOffs;
};

AMMOICON gAmmoIcons[] = {
    { -1, 0, 0 },
    { 816, (int)(65536 * 0.5), 0 },
    { 619, (int)(65536 * 0.8), 0 },
    { 817, (int)(65536 * 0.7), 3 },
    { 801, (int)(65536 * 0.5), -6 },
    { 589, (int)(65536 * 0.7), 2 },
    { 618, (int)(65536 * 0.5), 4 },
    { 548, (int)(65536 * 0.3), -6 },
    { 820, (int)(65536 * 0.3), -6 },
    { 525, (int)(65536 * 0.6), -6 },
    { 811, (int)(65536 * 0.5), 2 },
    { 810, (int)(65536 * 0.45), 2 },
};

struct POWERUPDISPLAY
{
    int nTile;
    float nScaleRatio;
    int yOffset;
    int remainingDuration;
};

void sortPowerUps(POWERUPDISPLAY* powerups) {
    for (int i = 1; i < 5; i++)
    {
        for (int j = 0; j < 5 - i; j++)
        {
            if (powerups[j].remainingDuration > powerups[j + 1].remainingDuration)
            {
                POWERUPDISPLAY temp = powerups[j];
                powerups[j] = powerups[j + 1];
                powerups[j + 1] = temp;
            }
        }
    }
}

void viewDrawPowerUps(PLAYER* pPlayer)
{
    if (!hud_powerupduration)
        return;

    // NoOne to author: the following powerups can be safely added in this list:
    // kPwUpFeatherFall -   (used in some user addons, makes player immune to fall damage)
    // kPwUpGasMask -       (used in some user addons, makes player immune to choke damage)
    // kPwUpDoppleganger -  (works in multiplayer, it swaps player's team colors, so enemy team player thinks it's a team mate)
    // kPwUpAsbestArmor -   (used in some user addons, makes player immune to fire damage and draws hud)
    // kPwUpGrowShroom -    (grows player size, works only if gModernMap == true)
    // kPwUpShrinkShroom -  (shrinks player size, works only if gModernMap == true)

    POWERUPDISPLAY powerups[5];
    powerups[0] = { gPowerUpInfo[kPwUpShadowCloak].picnum,  0.4f, 0, pPlayer->pwUpTime[kPwUpShadowCloak] }; // invisibility
    powerups[1] = { gPowerUpInfo[kPwUpReflectShots].picnum, 0.4f, 5, pPlayer->pwUpTime[kPwUpReflectShots] };
    powerups[2] = { gPowerUpInfo[kPwUpDeathMask].picnum, 0.3f, 9, pPlayer->pwUpTime[kPwUpDeathMask] }; // invulnerability
    powerups[3] = { gPowerUpInfo[kPwUpTwoGuns].picnum, 0.3f, 5, pPlayer->pwUpTime[kPwUpTwoGuns] };
    // does nothing, only appears at near the end of Cryptic Passage's Lost Monastery (CP04)
    powerups[4] = { gPowerUpInfo[kPwUpShadowCloakUseless].picnum, 0.4f, 9, pPlayer->pwUpTime[kPwUpShadowCloakUseless] };

    sortPowerUps(powerups);

    const int warningTime = 5;
    const int x = 15;
    int y = 50;
    for (int i = 0; i < 5; i++)
    {
        if (powerups[i].remainingDuration)
        {
            int remainingSeconds = powerups[i].remainingDuration / 100;
            if (remainingSeconds > warningTime || ((int)totalclock & 32))
            {
                DrawStatMaskedSprite(powerups[i].nTile, x, y + powerups[i].yOffset, 0, 0, 256, (int)(65536 * powerups[i].nScaleRatio));
            }

            DrawStatNumber("%d", remainingSeconds, kSBarNumberInv, x + 15, y, 0, remainingSeconds > warningTime ? 0 : 2, 256, 65536 * 0.5);
            y += 20;
        }
    }
}

void viewDrawPack(PLAYER* pPlayer, int x, int y)
{
    static int dword_14C508;
    int packs[5];
    if (pPlayer->packItemTime)
    {
        int nPacks = 0;
        int width = 0;
        for (int i = 0; i < 5; i++)
        {
            if (pPlayer->packSlots[i].curAmount)
            {
                packs[nPacks++] = i;
                width += tilesiz[gPackIcons[i]].x + 1;
            }
        }
        width /= 2;
        x -= width;
        for (int i = 0; i < nPacks; i++)
        {
            int nPack = packs[i];
            DrawStatSprite(2568, x + 1, y - 8);
            DrawStatSprite(2568, x + 1, y - 6);
            DrawStatSprite(gPackIcons[nPack], x + 1, y + 1);
            if (nPack == pPlayer->packItemId)
                DrawStatMaskedSprite(2559, x + 1, y + 1);
            int nShade;
            if (pPlayer->packSlots[nPack].isActive)
                nShade = 4;
            else
                nShade = 24;
            DrawStatNumber("%3d", pPlayer->packSlots[nPack].curAmount, 2250, x - 4, y - 13, nShade, 0);
            x += tilesiz[gPackIcons[nPack]].x + 1;
        }
    }
    if (pPlayer->packItemTime != dword_14C508)
    {
        viewUpdatePages();
    }
    dword_14C508 = pPlayer->packItemTime;
}

void DrawPackItemInStatusBar(PLAYER* pPlayer, int x, int y, int x2, int y2, int nStat)
{
    if (pPlayer->packItemId < 0) return;

    DrawStatSprite(gPackIcons[pPlayer->packItemId], x, y, 0, 0, nStat);
    DrawStatNumber("%3d", pPlayer->packSlots[pPlayer->packItemId].curAmount, 2250, x2, y2, 0, 0, nStat);
}

void DrawPackItemInStatusBar2(PLAYER* pPlayer, int x, int y, int x2, int y2, int nStat, int nScale)
{
    if (pPlayer->packItemId < 0) return;

    DrawStatMaskedSprite(gPackIcons2[pPlayer->packItemId].nTile, x, y + gPackIcons2[pPlayer->packItemId].nYOffs, 0, 0, nStat, gPackIcons2[pPlayer->packItemId].nScale);
    DrawStatNumber("%3d", pPlayer->packSlots[pPlayer->packItemId].curAmount, kSBarNumberInv, x2, y2, 0, 0, nStat, nScale);
}

void viewDrawPlayerSlots(void)
{
    for (int nRows = (gNetPlayers - 1) / 4; nRows >= 0; nRows--)
    {
        for (int nCol = 0; nCol < 4; nCol++)
        {
            DrawStatSprite(2229, 40 + nCol * 80, 4 + nRows * 9, 16);
        }
    }
}

void viewDrawPlayerFrags(void)
{
    FString gTempStr;
    viewDrawPlayerSlots();
    for (int i = 0, p = connecthead; p >= 0; i++, p = connectpoint2[p])
    {
        int x = 80 * (i & 3);
        int y = 9 * (i / 4);
        int col = gPlayer[p].teamId & 3;
        char* name = gProfile[p].name;
        if (gProfile[p].skill == 2)
            gTempStr.Format("%s", name);
        else
            gTempStr.Format("%s [%d]", name, gProfile[p].skill);
        gTempStr.ToUpper();
        viewDrawText(4, gTempStr, x + 4, y + 1, -128, 11 + col, 0, 0);
        gTempStr.Format("%2d", gPlayer[p].fragCount);
        viewDrawText(4, gTempStr, x + 76, y + 1, -128, 11 + col, 2, 0);
    }
}

void viewDrawPlayerFlags(void)
{
    FString gTempStr;
    viewDrawPlayerSlots();
    for (int i = 0, p = connecthead; p >= 0; i++, p = connectpoint2[p])
    {
        int x = 80 * (i & 3);
        int y = 9 * (i / 4);
        int col = gPlayer[p].teamId & 3;
        char* name = gProfile[p].name;
        if (gProfile[p].skill == 2)
            gTempStr.Format("%s", name);
        else
            gTempStr.Format("%s [%d]", name, gProfile[p].skill);
        gTempStr.ToUpper();
        viewDrawText(4, gTempStr, x + 4, y + 1, -128, 11 + col, 0, 0);

        gTempStr.Format("F");
        x += 76;
        if (gPlayer[p].hasFlag & 2)
        {
            viewDrawText(4, gTempStr, x, y + 1, -128, 12, 2, 0);
            x -= 6;
        }

        if (gPlayer[p].hasFlag & 1)
            viewDrawText(4, gTempStr, x, y + 1, -128, 11, 2, 0);
    }
}


void viewDrawCtfHudVanilla(ClockTicks arg)
{
    FString gTempStr;
    int x = 1, y = 1;
    if (dword_21EFD0[0] == 0 || ((int)totalclock & 8))
    {
        viewDrawText(0, GStrings("TXT_COLOR_BLUE"), x, y, -128, 10, 0, 0, 256);
        dword_21EFD0[0] = dword_21EFD0[0] - arg;
        if (dword_21EFD0[0] < 0)
            dword_21EFD0[0] = 0;
        gTempStr.Format("%-3d", dword_21EFB0[0]);
        viewDrawText(0, gTempStr, x, y + 10, -128, 10, 0, 0, 256);
    }
    x = 319;
    if (dword_21EFD0[1] == 0 || ((int)totalclock & 8))
    {
        viewDrawText(0, GStrings("TXT_COLOR_RED"), x, y, -128, 7, 2, 0, 512);
        dword_21EFD0[1] = dword_21EFD0[1] - arg;
        if (dword_21EFD0[1] < 0)
            dword_21EFD0[1] = 0;
        gTempStr.Format("%3d", dword_21EFB0[1]);
        viewDrawText(0, gTempStr, x, y + 10, -128, 7, 2, 0, 512);
    }
}

void flashTeamScore(ClockTicks arg, int team, bool show)
{
    dassert(0 == team || 1 == team); // 0: blue, 1: red

    if (dword_21EFD0[team] == 0 || ((int)totalclock & 8))
    {
        dword_21EFD0[team] = dword_21EFD0[team] - arg;
        if (dword_21EFD0[team] < 0)
            dword_21EFD0[team] = 0;

        if (show)
            DrawStatNumber("%d", dword_21EFB0[team], kSBarNumberInv, 290, team ? 125 : 90, 0, team ? 2 : 10, 512, 65536 * 0.75);
    }
}

static void viewDrawCtfHud(ClockTicks arg)
{
    if (0 == gViewSize)
    {
        flashTeamScore(arg, 0, false);
        flashTeamScore(arg, 1, false);
        return;
    }

    bool blueFlagTaken = false;
    bool redFlagTaken = false;
    int blueFlagCarrierColor = 0;
    int redFlagCarrierColor = 0;
    for (int i = 0, p = connecthead; p >= 0; i++, p = connectpoint2[p])
    {
        if ((gPlayer[p].hasFlag & 1) != 0)
        {
            blueFlagTaken = true;
            blueFlagCarrierColor = gPlayer[p].teamId & 3;
        }
        if ((gPlayer[p].hasFlag & 2) != 0)
        {
            redFlagTaken = true;
            redFlagCarrierColor = gPlayer[p].teamId & 3;
        }
    }

    bool meHaveBlueFlag = gMe->hasFlag & 1;
    DrawStatMaskedSprite(meHaveBlueFlag ? 3558 : 3559, 320, 75, 0, 10, 512, 65536 * 0.35);
    if (gBlueFlagDropped)
        DrawStatMaskedSprite(2332, 305, 83, 0, 10, 512, 65536);
    else if (blueFlagTaken)
        DrawStatMaskedSprite(4097, 307, 77, 0, blueFlagCarrierColor ? 2 : 10, 512, 65536);
    flashTeamScore(arg, 0, true);

    bool meHaveRedFlag = gMe->hasFlag & 2;
    DrawStatMaskedSprite(meHaveRedFlag ? 3558 : 3559, 320, 110, 0, 2, 512, 65536 * 0.35);
    if (gRedFlagDropped)
        DrawStatMaskedSprite(2332, 305, 117, 0, 2, 512, 65536);
    else if (redFlagTaken)
        DrawStatMaskedSprite(4097, 307, 111, 0, redFlagCarrierColor ? 2 : 10, 512, 65536);
    flashTeamScore(arg, 1, true);
}



void UpdateStatusBar(ClockTicks arg)
{
    PLAYER* pPlayer = gView;
    XSPRITE* pXSprite = pPlayer->pXSprite;

    int nPalette = 0;

    if (gGameOptions.nGameType == 3)
    {
        if (pPlayer->teamId & 1)
            nPalette = 7;
        else
            nPalette = 10;
    }

    if (gViewSize < 0) return;

    if (gViewSize == 1)
    {
        DrawStatMaskedSprite(2169, 12, 195, 0, 0, 256, (int)(65536 * 0.56));
        DrawStatNumber("%d", pXSprite->health >> 4, kSBarNumberHealth, 28, 187, 0, 0, 256);
        if (pPlayer->armor[1])
        {
            DrawStatMaskedSprite(2578, 70, 186, 0, 0, 256, (int)(65536 * 0.5));
            DrawStatNumber("%3d", pPlayer->armor[1] >> 4, kSBarNumberArmor2, 83, 187, 0, 0, 256, (int)(65536 * 0.65));
        }
        if (pPlayer->armor[0])
        {
            DrawStatMaskedSprite(2586, 112, 195, 0, 0, 256, (int)(65536 * 0.5));
            DrawStatNumber("%3d", pPlayer->armor[0] >> 4, kSBarNumberArmor1, 125, 187, 0, 0, 256, (int)(65536 * 0.65));
        }
        if (pPlayer->armor[2])
        {
            DrawStatMaskedSprite(2602, 155, 196, 0, 0, 256, (int)(65536 * 0.5));
            DrawStatNumber("%3d", pPlayer->armor[2] >> 4, kSBarNumberArmor3, 170, 187, 0, 0, 256, (int)(65536 * 0.65));
        }

        DrawPackItemInStatusBar2(pPlayer, 225, 194, 240, 187, 512, (int)(65536 * 0.7));

        if (pPlayer->curWeapon && pPlayer->weaponAmmo != -1)
        {
            int num = pPlayer->ammoCount[pPlayer->weaponAmmo];
            if (pPlayer->weaponAmmo == 6)
                num /= 10;
            if ((unsigned int)gAmmoIcons[pPlayer->weaponAmmo].nTile < kMaxTiles)
                DrawStatMaskedSprite(gAmmoIcons[pPlayer->weaponAmmo].nTile, 304, 192 + gAmmoIcons[pPlayer->weaponAmmo].nYOffs,
                    0, 0, 512, gAmmoIcons[pPlayer->weaponAmmo].nScale);
            DrawStatNumber("%3d", num, kSBarNumberAmmo, 267, 187, 0, 0, 512);
        }

        for (int i = 0; i < 6; i++)
        {
            if (pPlayer->hasKey[i + 1])
                DrawStatMaskedSprite(2552 + i, 260 + 10 * i, 170, 0, 0, 512, (int)(65536 * 0.25));
        }

        if (pPlayer->throwPower)
            TileHGauge(2260, 124, 175 - 10, pPlayer->throwPower, 65536);
        else
            viewDrawPack(pPlayer, 166, 200 - tilesiz[2201].y / 2 - 30);
        viewDrawStats(pPlayer, 2, 140);
        viewDrawPowerUps(pPlayer);
    }
    else if (gViewSize <= 2)
    {
        if (pPlayer->throwPower)
            TileHGauge(2260, 124, 175, pPlayer->throwPower, 65536);
        else
            viewDrawPack(pPlayer, 166, 200 - tilesiz[2201].y / 2);
    }
    if (gViewSize == 2)
    {
        DrawStatSprite(2201, 34, 187, 16, nPalette, 256);
        if (pXSprite->health >= 16 || ((int)totalclock & 16) || pXSprite->health == 0)
        {
            DrawStatNumber("%3d", pXSprite->health >> 4, 2190, 8, 183, 0, 0, 256);
        }
        if (pPlayer->curWeapon && pPlayer->weaponAmmo != -1)
        {
            int num = pPlayer->ammoCount[pPlayer->weaponAmmo];
            if (pPlayer->weaponAmmo == 6)
                num /= 10;
            DrawStatNumber("%3d", num, 2240, 42, 183, 0, 0, 256);
        }
        DrawStatSprite(2173, 284, 187, 16, nPalette, 512);
        if (pPlayer->armor[1])
        {
            TileHGauge(2207, 250, 175, pPlayer->armor[1], 3200, 512);
            DrawStatNumber("%3d", pPlayer->armor[1] >> 4, 2230, 255, 178, 0, 0, 512);
        }
        if (pPlayer->armor[0])
        {
            TileHGauge(2209, 250, 183, pPlayer->armor[0], 3200, 512);
            DrawStatNumber("%3d", pPlayer->armor[0] >> 4, 2230, 255, 186, 0, 0, 512);
        }
        if (pPlayer->armor[2])
        {
            TileHGauge(2208, 250, 191, pPlayer->armor[2], 3200, 512);
            DrawStatNumber("%3d", pPlayer->armor[2] >> 4, 2230, 255, 194, 0, 0, 512);
        }
        DrawPackItemInStatusBar(pPlayer, 286, 186, 302, 183, 512);

        for (int i = 0; i < 6; i++)
        {
            int nTile = 2220 + i;
            int x, nStat = 0;
            int y = 200 - 6;
            if (i & 1)
            {
                x = 320 - (78 + (i >> 1) * 10);
                nStat |= 512;
            }
            else
            {
                x = 73 + (i >> 1) * 10;
                nStat |= 256;
            }
            if (pPlayer->hasKey[i + 1])
                DrawStatSprite(nTile, x, y, 0, 0, nStat);
#if 0
            else
                DrawStatSprite(nTile, x, y, 40, 5, nStat);
#endif
        }
        viewDrawStats(pPlayer, 2, 140);
        viewDrawPowerUps(pPlayer);
    }
    else if (gViewSize > 2)
    {
        viewDrawPack(pPlayer, 160, 200 - tilesiz[2200].y);
        DrawStatMaskedSprite(2200, 160, 172, 16, nPalette);
        DrawPackItemInStatusBar(pPlayer, 265, 186, 260, 172);
        if (pXSprite->health >= 16 || ((int)totalclock & 16) || pXSprite->health == 0)
        {
            DrawStatNumber("%3d", pXSprite->health >> 4, 2190, 86, 183, 0, 0);
        }
        if (pPlayer->curWeapon && pPlayer->weaponAmmo != -1)
        {
            int num = pPlayer->ammoCount[pPlayer->weaponAmmo];
            if (pPlayer->weaponAmmo == 6)
                num /= 10;
            DrawStatNumber("%3d", num, 2240, 216, 183, 0, 0);
        }
        for (int i = 9; i >= 1; i--)
        {
            int x = 135 + ((i - 1) / 3) * 23;
            int y = 182 + ((i - 1) % 3) * 6;
            int num = pPlayer->ammoCount[i];
            if (i == 6)
                num /= 10;
            if (i == pPlayer->weaponAmmo)
            {
                DrawStatNumber("%3d", num, 2230, x, y, -128, 10);
            }
            else
            {
                DrawStatNumber("%3d", num, 2230, x, y, 32, 10);
            }
        }

        if (pPlayer->weaponAmmo == 10)
        {
            DrawStatNumber("%2d", pPlayer->ammoCount[10], 2230, 291, 194, -128, 10);
        }
        else
        {
            DrawStatNumber("%2d", pPlayer->ammoCount[10], 2230, 291, 194, 32, 10);
        }

        if (pPlayer->weaponAmmo == 11)
        {
            DrawStatNumber("%2d", pPlayer->ammoCount[11], 2230, 309, 194, -128, 10);
        }
        else
        {
            DrawStatNumber("%2d", pPlayer->ammoCount[11], 2230, 309, 194, 32, 10);
        }

        if (pPlayer->armor[1])
        {
            TileHGauge(2207, 44, 174, pPlayer->armor[1], 3200);
            DrawStatNumber("%3d", pPlayer->armor[1] >> 4, 2230, 50, 177, 0, 0);
        }
        if (pPlayer->armor[0])
        {
            TileHGauge(2209, 44, 182, pPlayer->armor[0], 3200);
            DrawStatNumber("%3d", pPlayer->armor[0] >> 4, 2230, 50, 185, 0, 0);
        }
        if (pPlayer->armor[2])
        {
            TileHGauge(2208, 44, 190, pPlayer->armor[2], 3200);
            DrawStatNumber("%3d", pPlayer->armor[2] >> 4, 2230, 50, 193, 0, 0);
        }
        //FString gTempStr;
        //gTempStr.Format("v%s", GetVersionString());
        //viewDrawText(3, gTempStr, 20, 191, 32, 0, 1, 0);

        for (int i = 0; i < 6; i++)
        {
            int nTile = 2220 + i;
            int x = 73 + (i & 1) * 173;
            int y = 171 + (i >> 1) * 11;
            if (pPlayer->hasKey[i + 1])
                DrawStatSprite(nTile, x, y);
            else
                DrawStatSprite(nTile, x, y, 40, 5);
        }
        DrawStatMaskedSprite(2202, 118, 185, pPlayer->isRunning ? 16 : 40);
        DrawStatMaskedSprite(2202, 201, 185, pPlayer->isRunning ? 16 : 40);
        if (pPlayer->throwPower)
        {
            TileHGauge(2260, 124, 175, pPlayer->throwPower, 65536);
        }
        viewDrawStats(pPlayer, 2, 140);
        viewDrawPowerUps(pPlayer);
    }

    if (gGameOptions.nGameType < 1) return;

    if (gGameOptions.nGameType == 3)
    {
        if (VanillaMode())
        {
            viewDrawCtfHudVanilla(arg);
        }
        else
        {
            viewDrawCtfHud(arg);
            viewDrawPlayerFlags();
        }
    }
    else
    {
        viewDrawPlayerFrags();
    }
}


END_BLD_NS
