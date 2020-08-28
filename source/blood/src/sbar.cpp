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
#include "statusbar.h"

CVARD(Bool, hud_powerupduration, true, CVAR_ARCHIVE/*|CVAR_FRONTEND_BLOOD*/, "enable/disable displaying the remaining seconds for power-ups")

BEGIN_BLD_NS

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static int gPackIcons[5] = {
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

static AMMOICON gAmmoIcons[] = {
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



class DBloodStatusBar : public DBaseStatusBar
{
    enum NewRSFlags
    {
        RS_CENTERBOTTOM = 16384,
    };

    DHUDFont smallf, tinyf;

public:
    DBloodStatusBar()
    {
        smallf = { SmallFont, 0, Off, 0, 0 };
        tinyf = { gFont[4], 4, CellRight, 0, 0 };
    }

private:
    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DrawStatSprite(int nTile, double x, double y, int nShade = 0, int nPalette = 0, unsigned int nStat = 0, int nScale = 65536, ERenderStyle style = STYLE_Normal, int align = DI_SCREEN_AUTO)
    {
        int flags = align | ((nStat & RS_CENTERBOTTOM)? DI_ITEM_CENTER_BOTTOM : (nStat & RS_TOPLEFT)? DI_ITEM_LEFT_TOP : DI_ITEM_RELCENTER);
        double alpha = 1.;
        double scale = nScale / 65536.;
        DrawGraphic(tileGetTexture(nTile, true), x, y, flags, alpha, -1, -1, scale, scale, shadeToLight(nShade), TRANSLATION(Translation_Remap, nPalette), 0, style);
    }
    void DrawStatMaskedSprite(int nTile, double x, double y, int nShade = 0, int nPalette = 0, unsigned int nStat = 0, int nScale = 65536, int align = DI_SCREEN_AUTO)
    {
        DrawStatSprite(nTile, x, y, nShade, nPalette, nStat, nScale, STYLE_Translucent, align);
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DrawStatNumber(const char* pFormat, int nNumber, int nTile, double x, double y, int nShade, int nPalette, unsigned int nStat = 0, int nScale = 65536, int align = 0)
    {
        double width = (tileWidth(nTile) + 1) * (nScale / 65536.);

        char tempbuf[80];
        mysnprintf(tempbuf, 80, pFormat, nNumber);
        x += 0.5;
        y += 0.5;   // This is needed because due to using floating point math, this code rounds slightly differently which for the numbers can be a problem.
        for (unsigned int i = 0; tempbuf[i]; i++, x += width)
        {
            if (tempbuf[i] == ' ') continue;
            DrawStatSprite(nTile + tempbuf[i] - '0', x, y, nShade, nPalette, nStat, nScale, STYLE_Translucent, align);
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DrawCharArray(const char* string, int nTile, double x, double y, int nShade, int nPalette, unsigned int nStat = 0, int nScale = 65536, int align = 0)
    {
        double width = (tileWidth(nTile) + 1) * (nScale / 65536.);

        x += 0.5;
        y += 0.5;   // This is needed because due to using floating point math, this code rounds slightly differently which for the numbers can be a problem.

        for (unsigned int i = 0; string[i]; i++, x += width)
        {
            // Hackasaurus rex to give me a slash when drawing the weapon count of a reloadable gun.
            if (string[i] == 47 && nTile == kSBarNumberAmmo)
            {
                DrawStatSprite(4207, x, y, nShade, nPalette, nStat, nScale, STYLE_Translucent, align);
            }
            else
            {
                DrawStatSprite(nTile + string[i] - '0', x, y, nShade, nPalette, nStat, nScale, STYLE_Translucent, align);
            }
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void TileHGauge(int nTile, double x, double y, int nMult, int nDiv, int nStat = 0, int nScale = 65536)
    {
        int bx = scale(mulscale16(tilesiz[nTile].x, nScale), nMult, nDiv) + x;
        double scale = double(bx - x) / tileWidth(nTile);
        double sc = nScale / 65536.;
        DrawGraphic(tileGetTexture(nTile, true), x, y, DI_ITEM_LEFT_TOP, 1., -1, -1, scale*sc, sc, 0xffffffff, 0, 0);
    }



    //---------------------------------------------------------------------------
    //
    //
    //
    //---------------------------------------------------------------------------

    void PrintLevelStats(PLAYER* pPlayer, int bottomy)
    {
        FLevelStats stats{};

        stats.fontscale = 1.;
        stats.spacing = SmallFont->GetHeight() + 1;
        stats.screenbottomspace = bottomy;
        stats.font = SmallFont;
        stats.letterColor = CR_DARKRED;
        stats.standardColor = CR_DARKGRAY;
        stats.time = Scale(gLevelTime, 1000, kTicsPerSec);

		if (automapMode == am_full)
		{
            if (!am_textfont)
            {
                stats.font = SmallFont2;
                stats.spacing = 6;
            }
            if (hud_size <= Hud_StbarOverlay) stats.screenbottomspace = 56;
            DBaseStatusBar::PrintAutomapInfo(stats);
		}
        if (automapMode == am_off && hud_stats)
        {
            stats.completeColor = CR_DARKGREEN;

            stats.kills = gKillMgr.at4;
            stats.maxkills = gKillMgr.at0;
            stats.frags = gGameOptions.nGameType == 3? pPlayer->fragCount : -1;
            stats.secrets = gSecretMgr.at4 + gSecretMgr.at8;
            stats.maxsecrets = gSecretMgr.at0;

            DBaseStatusBar::PrintLevelStats(stats);
        }
    }


    //---------------------------------------------------------------------------
    //
    // ok
    //
    //---------------------------------------------------------------------------

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

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

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
        int y = -50;
        for (int i = 0; i < 5; i++)
        {
            if (powerups[i].remainingDuration)
            {
                int remainingSeconds = powerups[i].remainingDuration / 100;
                if (remainingSeconds > warningTime || (gameclock & 32))
                {
                    DrawStatMaskedSprite(powerups[i].nTile, x, y + powerups[i].yOffset, 0, 0, 256, (int)(65536 * powerups[i].nScaleRatio), DI_SCREEN_LEFT_CENTER);
                }

                DrawStatNumber("%d", remainingSeconds, kSBarNumberInv, x + 15, y, 0, remainingSeconds > warningTime ? 0 : 2, 256, 65536 * 0.5, DI_SCREEN_LEFT_CENTER);
                y += 20;
            }
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void drawInventory(PLAYER* pPlayer, int x, int y)
    {
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
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DrawPackItemInStatusBar(PLAYER* pPlayer, int x, int y, int x2, int y2)
    {
        auto id = pPlayer->packItemId;
        //id = 0;
        if (id < 0) return;

        DrawStatSprite(gPackIcons[id], x, y, 0, 0);
        DrawStatNumber("%3d", pPlayer->packSlots[id].curAmount, 2250, x2, y2, 0, 0);
    }

    void DrawPackItemInStatusBar2(PLAYER* pPlayer, int x, int y, int x2, int y2, int nStat, int nScale)
    {
        if (pPlayer->packItemId < 0) return;

        DrawStatMaskedSprite(gPackIcons2[pPlayer->packItemId].nTile, x, y + gPackIcons2[pPlayer->packItemId].nYOffs, 0, 0, nStat, gPackIcons2[pPlayer->packItemId].nScale);
        DrawStatNumber("%3d", pPlayer->packSlots[pPlayer->packItemId].curAmount, kSBarNumberInv, x2, y2, 0, 0, nStat, nScale);
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void viewDrawPlayerSlots(void)
    {
        for (int nRows = (gNetPlayers - 1) / 4; nRows >= 0; nRows--)
        {
            for (int nCol = 0; nCol < 4; nCol++)
            {
                DrawStatSprite(2229, -120 + nCol * 80, 4 + nRows * 9, 16, 0, 0, 65536, STYLE_Normal, DI_SCREEN_CENTER_TOP);
            }
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void viewDrawPlayerFrags(void)
    {
        FString gTempStr;
        viewDrawPlayerSlots();
        for (int i = 0, p = connecthead; p >= 0; i++, p = connectpoint2[p])
        {
            int x = -160 + 80 * (i & 3);
            int y = 9 * (i / 4);
            int col = gPlayer[p].teamId & 3;
            char* name = gProfile[p].name;
            if (gProfile[p].skill == 2)
                gTempStr.Format("%s", name);
            else
                gTempStr.Format("%s [%d]", name, gProfile[p].skill);
            
            int color = CR_UNDEFINED;// todo: remap the colors. (11+col)
            SBar_DrawString(this, &tinyf, gTempStr, x + 4, y, DI_SCREEN_CENTER_TOP, color, 1., -1, -1, 1, 1);
            gTempStr.Format("%2d", gPlayer[p].fragCount);
            SBar_DrawString(this, &tinyf, gTempStr, x + 76, y, DI_SCREEN_CENTER_TOP, color, 1., -1, -1, 1, 1);
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void viewDrawPlayerFlags(void)
    {
        FString gTempStr;
        viewDrawPlayerSlots();
        for (int i = 0, p = connecthead; p >= 0; i++, p = connectpoint2[p])
        {
            int x = -160 + 80 * (i & 3);
            int y = 9 * (i / 4);
            int col = gPlayer[p].teamId & 3;
            char* name = gProfile[p].name;
            if (gProfile[p].skill == 2)
                gTempStr.Format("%s", name);
            else
                gTempStr.Format("%s [%d]", name, gProfile[p].skill);
            gTempStr.ToUpper();
            int color = CR_UNDEFINED;// todo: remap the colors.
            SBar_DrawString(this, &tinyf, gTempStr, x + 4, y, DI_SCREEN_CENTER_TOP, color, 1., -1, -1, 1, 1);

            gTempStr = "F";
            x += 76;
            if (gPlayer[p].hasFlag & 2)
            {
                SBar_DrawString(this, &tinyf, gTempStr, x, y, DI_SCREEN_CENTER_TOP, CR_GREEN/*12*/, 1., -1, -1, 1, 1);
                x -= 6;
            }

            if (gPlayer[p].hasFlag & 1)
                SBar_DrawString(this, &tinyf, gTempStr, x, y, DI_SCREEN_CENTER_TOP, CR_RED/*11*/, 1., -1, -1, 1, 1);
        }
    }


    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void viewDrawCtfHudVanilla(int arg)
    {
        FString gTempStr;
        int x = 1, y = 1;
        if (dword_21EFD0[0] == 0 || (gameclock & 8))
        {
            SBar_DrawString(this, &smallf, GStrings("TXT_COLOR_BLUE"), x, y, 0, CR_LIGHTBLUE, 1., -1, -1, 1, 1);
            dword_21EFD0[0] = dword_21EFD0[0] - arg;
            if (dword_21EFD0[0] < 0)
                dword_21EFD0[0] = 0;
            gTempStr.Format("%-3d", dword_21EFB0[0]);
            SBar_DrawString(this, &smallf, gTempStr, x, y + 10, 0, CR_LIGHTBLUE, 1., -1, -1, 1, 1);
        }
        x = -2;
        if (dword_21EFD0[1] == 0 || (gameclock & 8))
        {
            SBar_DrawString(this, &smallf, GStrings("TXT_COLOR_RED"), x, y, DI_TEXT_ALIGN_RIGHT, CR_BRICK, 1., -1, -1, 1, 1);
            dword_21EFD0[1] = dword_21EFD0[1] - arg;
            if (dword_21EFD0[1] < 0)
                dword_21EFD0[1] = 0;
            gTempStr.Format("%3d", dword_21EFB0[1]);
            SBar_DrawString(this, &smallf, gTempStr, x, y + 10, DI_TEXT_ALIGN_RIGHT, CR_BRICK, 1., -1, -1, 1, 1);
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void flashTeamScore(int arg, int team, bool show)
    {
        dassert(0 == team || 1 == team); // 0: blue, 1: red

        if (dword_21EFD0[team] == 0 || (gameclock & 8))
        {
            dword_21EFD0[team] = dword_21EFD0[team] - arg;
            if (dword_21EFD0[team] < 0)
                dword_21EFD0[team] = 0;

            if (show)
                DrawStatNumber("%d", dword_21EFB0[team], kSBarNumberInv, -30, team ? 25 : -10, 0, team ? 2 : 10, 512, 65536 * 0.75, DI_SCREEN_RIGHT_CENTER);
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void viewDrawCtfHud(int arg)
    {
        if (hud_size == Hud_Nothing)
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
        DrawStatMaskedSprite(meHaveBlueFlag ? 3558 : 3559, 0, 75-100, 0, 10, 512, 65536 * 0.35, DI_SCREEN_RIGHT_CENTER);
        if (gBlueFlagDropped)
            DrawStatMaskedSprite(2332, 305-320, 83 - 100, 0, 10, 512, 65536, DI_SCREEN_RIGHT_CENTER);
        else if (blueFlagTaken)
            DrawStatMaskedSprite(4097, 307-320, 77 - 100, 0, blueFlagCarrierColor ? 2 : 10, 512, 65536, DI_SCREEN_RIGHT_CENTER);
        flashTeamScore(arg, 0, true);

        bool meHaveRedFlag = gMe->hasFlag & 2;
        DrawStatMaskedSprite(meHaveRedFlag ? 3558 : 3559, 0, 10, 0, 2, 512, 65536 * 0.35, DI_SCREEN_RIGHT_CENTER);
        if (gRedFlagDropped)
            DrawStatMaskedSprite(2332, 305-320, 17, 0, 2, 512, 65536, DI_SCREEN_RIGHT_CENTER);
        else if (redFlagTaken)
            DrawStatMaskedSprite(4097, 307-320, 11, 0, redFlagCarrierColor ? 2 : 10, 512, 65536, DI_SCREEN_RIGHT_CENTER);
        flashTeamScore(arg, 1, true);
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DrawStatusBar(int nPalette)
    {
        BeginStatusBar(320, 200, tilesiz[2200].y);

        PLAYER* pPlayer = gView;
        XSPRITE* pXSprite = pPlayer->pXSprite;

        DrawStatMaskedSprite(2200, 160, 200, 0, nPalette, RS_CENTERBOTTOM);
        DrawPackItemInStatusBar(pPlayer, 265, 186, 260, 172);

        if (pXSprite->health >= 16 || (gameclock & 16) || pXSprite->health == 0)
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
            DrawStatNumber("%3d", num, 2230, x, y, i == pPlayer->weaponAmmo? -128 : 32, 10);
        }
        DrawStatNumber("%2d", pPlayer->ammoCount[10], 2230, 291, 194, pPlayer->weaponAmmo == 10? -128 : 32, 10);
        DrawStatNumber("%2d", pPlayer->ammoCount[11], 2230, 309, 194, pPlayer->weaponAmmo == 11? -128 : 32, 10);

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

        for (int i = 0; i < 6; i++)
        {
            int nTile = 2220 + i;
            double x = 73.5 + (i & 1) * 173;
            double y = 171.5 + (i >> 1) * 11;
            if (pPlayer->hasKey[i + 1])
                DrawStatSprite(nTile, x, y);
            else
                DrawStatSprite(nTile, x, y, 40, 5);
        }
        DrawStatMaskedSprite(2202, 118.5, 185.5, pPlayer->isRunning ? 16 : 40);
        DrawStatMaskedSprite(2202, 201.5, 185.5, pPlayer->isRunning ? 16 : 40);
        if (pPlayer->throwPower)
        {
            TileHGauge(2260, 124, 175.5, pPlayer->throwPower, 65536);
        }
        drawInventory(pPlayer, 166, 200 - tilesiz[2200].y);
        // Depending on the scale we can lower the stats display. This needs some tweaking but this catches the important default case already.
        PrintLevelStats(pPlayer, (hud_statscale <= 0.501f || hud_scale < 0.7) && double(xdim)/ydim > 1.6? 28 : 56);

    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DrawHUD1(int nPalette)
    {
        PLAYER* pPlayer = gView;
        XSPRITE* pXSprite = pPlayer->pXSprite;

        BeginHUD(320, 200, 1);
        DrawStatSprite(2201, 34, 187 - 200, 16, nPalette);
        if (pXSprite->health >= 16 || (gameclock & 16) || pXSprite->health == 0)
        {
            DrawStatNumber("%3d", pXSprite->health >> 4, 2190, 8, 183 - 200, 0, 0);
        }
        if (pPlayer->curWeapon && pPlayer->weaponAmmo != -1)
        {
            int num = pPlayer->ammoCount[pPlayer->weaponAmmo];
            if (pPlayer->weaponAmmo == 6)
                num /= 10;
            DrawStatNumber("%3d", num, 2240, 42, 183 - 200, 0, 0);
        }
        DrawStatSprite(2173, 284-320, 187 - 200, 16, nPalette);
        if (pPlayer->armor[1])
        {
            TileHGauge(2207, 250-320, 175 - 200, pPlayer->armor[1], 3200);
            DrawStatNumber("%3d", pPlayer->armor[1] >> 4, 2230, 255-320, 178 - 200, 0, 0);
        }
        if (pPlayer->armor[0])
        {
            TileHGauge(2209, 250-320, 183 - 200, pPlayer->armor[0], 3200);
            DrawStatNumber("%3d", pPlayer->armor[0] >> 4, 2230, 255-320, 186 - 200, 0, 0);
        }
        if (pPlayer->armor[2])
        {
            TileHGauge(2208, 250-320, 191 - 200, pPlayer->armor[2], 3200);
            DrawStatNumber("%3d", pPlayer->armor[2] >> 4, 2230, 255-320, 194 - 200, 0, 0);
        }

        DrawPackItemInStatusBar(pPlayer, 286-320, 186 - 200, 302-320, 183 - 200);

        for (int i = 0; i < 6; i++)
        {
            int nTile = 2220 + i;
            int x;
            int y = - 6;
            if (i & 1)
            {
                x = - (78 + (i >> 1) * 10);
            }
            else
            {
                x = 73 + (i >> 1) * 10;
            }
            if (pPlayer->hasKey[i + 1])
                DrawStatSprite(nTile, x, y, 0, 0);
        }
        PrintLevelStats(pPlayer, 28);
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DrawHUD2()
    {
        PLAYER* pPlayer = gView;
        XSPRITE* pXSprite = pPlayer->pXSprite;

        BeginHUD(320, 200, 1);
        DrawStatMaskedSprite(2169, 12, 195 - 200, 0, 0, 256, (int)(65536 * 0.56));
        DrawStatNumber("%d", pXSprite->health >> 4, kSBarNumberHealth, 28, 187 - 200, 0, 0, 256);
        if (pPlayer->armor[1])
        {
            DrawStatMaskedSprite(2578, 70, 186 - 200, 0, 0, 256, (int)(65536 * 0.5));
            DrawStatNumber("%3d", pPlayer->armor[1] >> 4, kSBarNumberArmor2, 83, 187 - 200, 0, 0, 256, (int)(65536 * 0.65));
        }
        if (pPlayer->armor[0])
        {
            DrawStatMaskedSprite(2586, 112, 195 - 200, 0, 0, 256, (int)(65536 * 0.5));
            DrawStatNumber("%3d", pPlayer->armor[0] >> 4, kSBarNumberArmor1, 125, 187 - 200, 0, 0, 256, (int)(65536 * 0.65));
        }
        if (pPlayer->armor[2])
        {
            DrawStatMaskedSprite(2602, 155, 196 - 200, 0, 0, 256, (int)(65536 * 0.5));
            DrawStatNumber("%3d", pPlayer->armor[2] >> 4, kSBarNumberArmor3, 170, 187 - 200, 0, 0, 256, (int)(65536 * 0.65));
        }

        DrawPackItemInStatusBar2(pPlayer, 225 - 320, 194 - 200, 240 - 320, 187 - 200, 512, (int)(65536 * 0.7));

        if (pPlayer->curWeapon && pPlayer->weaponAmmo != -1)
        {
            int num = pPlayer->ammoCount[pPlayer->weaponAmmo];
            if (pPlayer->weaponAmmo == 6)
                num /= 10;
            if ((unsigned int)gAmmoIcons[pPlayer->weaponAmmo].nTile < kMaxTiles)
                DrawStatMaskedSprite(gAmmoIcons[pPlayer->weaponAmmo].nTile, 304-320, -8 + gAmmoIcons[pPlayer->weaponAmmo].nYOffs,
                    0, 0, 512, gAmmoIcons[pPlayer->weaponAmmo].nScale);

            if (pPlayer->curWeapon != 3 || (pPlayer->curWeapon == 3 && !cl_showmagamt))
            {
                DrawStatNumber("%3d", num, kSBarNumberAmmo, 267-320, 187 - 200, 0, 0, 512);
            }
            else
            {
                FString format;
                bool twoGuns = powerupCheck(pPlayer, kPwUpTwoGuns);
                short reload = !twoGuns ? 1 : 6;
                short capacity = !twoGuns ? 2 : 4;
                short clip = CalcMagazineAmount(num, capacity, pPlayer->weaponState == reload);
                format.Format("%d/%d", clip, num - clip);

                DrawCharArray(format.GetChars(), kSBarNumberAmmo, 267-320, 187 - 200, 0, 0, 512);
            }
        }

        for (int i = 0; i < 6; i++)
        {
            if (pPlayer->hasKey[i + 1])
                DrawStatMaskedSprite(2552 + i, -60 + 10 * i, 170 - 200, 0, 0, 0, (int)(65536 * 0.25));
        }

        BeginStatusBar(320, 200, 28);
        if (pPlayer->throwPower)
            TileHGauge(2260, 124, 175, pPlayer->throwPower, 65536);
        else
            drawInventory(pPlayer, 166, 200-tilesiz[2201].y / 2 - 30);
        PrintLevelStats(pPlayer, 28);
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------
    public:
    void UpdateStatusBar(int arg)
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

        if (hud_size == Hud_full)
        {
            DrawHUD2();
        }
        else if (hud_size > Hud_Stbar)
        {
            BeginStatusBar(320, 200, 28);
            if (pPlayer->throwPower)
                TileHGauge(2260, 124, 175, pPlayer->throwPower, 65536);
            else
                drawInventory(pPlayer, 166, 200 - tilesiz[2201].y / 2);
        }
        if (hud_size == Hud_Mini)
        {
            DrawHUD1(nPalette);
        }
        else if (hud_size <= Hud_StbarOverlay)
        {
            DrawStatusBar(nPalette);
        }

        // All remaining parts must be done with HUD alignment rules, even when showing a status bar.
        BeginHUD(320, 200, 1);
        viewDrawPowerUps(pPlayer);

        if (gGameOptions.nGameType >= 1)
        {
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
    }
};


static void UpdateFrame(void)
{
    auto tex = tileGetTexture(kBackTile);

    twod->AddFlatFill(0, 0, xdim, windowxy1.y - 3, tex);
    twod->AddFlatFill(0, windowxy2.y + 4, xdim, ydim, tex);
    twod->AddFlatFill(0, windowxy1.y - 3, windowxy1.x - 3, windowxy2.y + 4, tex);
    twod->AddFlatFill(windowxy2.x + 4, windowxy1.y - 3, xdim, windowxy2.y + 4, tex);

    twod->AddFlatFill(windowxy1.x - 3, windowxy1.y - 3, windowxy1.x, windowxy2.y + 1, tex, 0, 1, 0xff545454);
    twod->AddFlatFill(windowxy1.x, windowxy1.y - 3, windowxy2.x + 4, windowxy1.y, tex, 0, 1, 0xff545454);
    twod->AddFlatFill(windowxy2.x + 1, windowxy1.y, windowxy2.x + 4, windowxy2.y + 4, tex, 0, 1, 0xff2a2a2a);
    twod->AddFlatFill(windowxy1.x - 3, windowxy2.y + 1, windowxy2.x + 1, windowxy2.y + 4, tex, 0, 1, 0xff2a2a2a);
}

void UpdateStatusBar(int arg)
{
    DBloodStatusBar sbar;

    if (automapMode == am_off && hud_size <= Hud_Stbar)
    {
        UpdateFrame();
    }

    sbar.UpdateStatusBar(arg);
}


END_BLD_NS
