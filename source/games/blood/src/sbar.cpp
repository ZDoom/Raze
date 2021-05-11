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
#include "v_font.h"

#include "blood.h"
#include "zstring.h"
#include "razemenu.h"
#include "gstrings.h"
#include "v_2ddrawer.h"
#include "v_video.h"
#include "v_font.h"
#include "statusbar.h"
#include "automap.h"
#include "v_draw.h"
#include "gamecvars.h"

CVARD(Bool, hud_powerupduration, true, CVAR_ARCHIVE/*|CVAR_FRONTEND_BLOOD*/, "enable/disable displaying the remaining seconds for power-ups")

BEGIN_BLD_NS

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static const char* gPackIcons[5] = {
    "PackIcon1", "PackIcon2", "PackIcon3", "PackIcon4", "PackIcon5" 
};

class DBloodStatusBar : public DBaseStatusBar
{
    DECLARE_CLASS(DBloodStatusBar, DBaseStatusBar)
    enum NewRSFlags
    {
        RS_CENTERBOTTOM = 16384,
    };

    TObjPtr<DHUDFont*> smallf, tinyf;

public:
    DBloodStatusBar()
    {
        smallf = Create<DHUDFont>(SmallFont, 0, Off, 0, 0 );
        tinyf = Create<DHUDFont>(gFont[4], 4, CellRight, 0, 0 );
    }

private:
    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DrawStatSprite(const char* nTile, double x, double y, int nShade = 0, int nPalette = 0, unsigned int nStat = 0, int nScale = 65536, ERenderStyle style = STYLE_Normal, int align = DI_SCREEN_AUTO)
    {
        int flags = align | ((nStat & RS_CENTERBOTTOM)? DI_ITEM_CENTER_BOTTOM : (nStat & RS_TOPLEFT)? DI_ITEM_LEFT_TOP : DI_ITEM_RELCENTER);
        double alpha = 1.;
        double scale = nScale / 65536.;
        DrawGraphic(TexMan.CheckForTexture(nTile, ETextureType::Any), x, y, flags, alpha, -1, -1, scale, scale, STYLE_Translucent, shadeToLight(nShade), TRANSLATION(Translation_Remap, nPalette), style);
    }
    void DrawStatMaskedSprite(const char* nTile, double x, double y, int nShade = 0, int nPalette = 0, unsigned int nStat = 0, int nScale = 65536, int align = DI_SCREEN_AUTO)
    {
        DrawStatSprite(nTile, x, y, nShade, nPalette, nStat, nScale, STYLE_Translucent, align);
    }

    void DrawStatSprite(int nTile, double x, double y, int nShade = 0, int nPalette = 0, unsigned int nStat = 0, int nScale = 65536, ERenderStyle style = STYLE_Normal, int align = DI_SCREEN_AUTO)
    {
        int flags = align | ((nStat & RS_CENTERBOTTOM) ? DI_ITEM_CENTER_BOTTOM : (nStat & RS_TOPLEFT) ? DI_ITEM_LEFT_TOP : DI_ITEM_RELCENTER);
        double alpha = 1.;
        double scale = nScale / 65536.;
        DrawGraphic(tileGetTexture(nTile, true), x, y, flags, alpha, -1, -1, scale, scale, STYLE_Translucent, shadeToLight(nShade), TRANSLATION(Translation_Remap, nPalette), style);
    }

    void DrawStatMaskedSprite(int nTile, double x, double y, int nShade = 0, int nPalette = 0, unsigned int nStat = 0, int nScale = 65536, int align = DI_SCREEN_AUTO)
    {
        DrawStatSprite(nTile, x, y, nShade, nPalette, nStat, nScale, STYLE_Translucent, align);
    }

    int texWidth(const char* name)
    {
        auto tex = TexMan.FindGameTexture(name, ETextureType::Any);
        return tex ? int(tex->GetDisplayWidth()) : 0;
    }

    int texHeight(const char* name)
    {
        auto tex = TexMan.FindGameTexture(name, ETextureType::Any);
        return tex ? int(tex->GetDisplayHeight()) : 0;
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DrawStatNumber(const char* pFormat, int nNumber, const char* nametemplate, double x, double y, int nShade, int nPalette, unsigned int nStat = 0, int nScale = 65536, int align = 0)
    {
        FStringf name("%s%d", nametemplate, 1);
        double width = (texWidth(name) + 1) * (nScale / 65536.);

        char tempbuf[80];
        mysnprintf(tempbuf, 80, pFormat, nNumber);
        x += 0.5;
        y += 0.5;   // This is needed because due to using floating point math, this code rounds slightly differently which for the numbers can be a problem.
        for (unsigned int i = 0; tempbuf[i]; i++, x += width)
        {
            if (tempbuf[i] == ' ') continue;
            name.Format("%s%d", nametemplate, tempbuf[i] - '0');
            DrawStatSprite(name, x, y, nShade, nPalette, nStat, nScale, STYLE_Translucent, align);
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DrawCharArray(const char* text, const char *nametemplate, double x, double y, int nShade, int nPalette, unsigned int nStat = 0, int nScale = 65536, int align = 0)
    {
        FStringf name("%s%d", nametemplate, 1);
        double width = (texWidth(name) + 1) * (nScale / 65536.);

        x += 0.5;
        y += 0.5;   // This is needed because due to using floating point math, this code rounds slightly differently which for the numbers can be a problem.

        for (unsigned int i = 0; text[i]; i++, x += width)
        {
            // Hackasaurus rex to give me a slash when drawing the weapon count of a reloadable gun.
            if (text[i] == 47)
            {
                DrawStatSprite("SBarSlash", x, y, nShade, nPalette, nStat, nScale, STYLE_Translucent, align);
            }
            else
            {
                name.Format("%s%d", nametemplate, text[i] - '0');
                DrawStatSprite(name, x, y, nShade, nPalette, nStat, nScale, STYLE_Translucent, align);
            }
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void TileHGauge(const char* nTile, double x, double y, int nMult, int nDiv, int nStat = 0, int nScale = 65536)
    {
        auto tex = TexMan.CheckForTexture(nTile, ETextureType::Any);
        if (!tex.isValid()) return;
        int w = TexMan.GetGameTexture(tex)->GetDisplayWidth();
        int bx = scale(MulScale(w, nScale, 16), nMult, nDiv) + x;
        double scale = double(bx - x) / w;
        double sc = nScale / 65536.;
        DrawGraphic(tex, x, y, DI_ITEM_LEFT_TOP, 1., -1, -1, sc, sc, STYLE_Translucent, 0xffffffff, 0, scale);
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
        stats.time = gFrameCount / GameTicRate;

		if (automapMode == am_full)
		{
            bool textfont = am_textfont;
            if (!am_textfont)
            {
                // For non-English languages force use of the text font. The tiny one is simply too small to ever add localized characters to it.
                auto p = GStrings["REQUIRED_CHARACTERS"];
                if (p && *p) textfont = true;
            }

            if (!textfont)
            {
                stats.font = SmallFont2;
                stats.spacing = 6;
            }
            if (hud_size <= Hud_StbarOverlay) stats.screenbottomspace = 56;
            DBaseStatusBar::PrintAutomapInfo(stats, textfont);
		}
        if (automapMode == am_off && hud_stats)
        {
            stats.completeColor = CR_DARKGREEN;

            stats.kills = gKillMgr.Kills;
            stats.maxkills = gKillMgr.TotalKills;
            stats.frags = gGameOptions.nGameType == 3? pPlayer->fragCount : -1;
            stats.secrets = gSecretMgr.Founds;
            stats.supersecrets = gSecretMgr.Super;
            stats.maxsecrets = max(gSecretMgr.Founds, gSecretMgr.Total); // If we found more than there are, increase the total. Some levels have a bugged counter.
            stats.time = Scale(PlayClock, 1000, 120);

            DBaseStatusBar::PrintLevelStats(stats);
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void viewDrawPowerUps(PLAYER* pPlayer)
    {
        enum { nPowerUps = 11 };

        static const float powerScale[] = { 0.4f, 0.4f, 0.3f, 0.3f, 0.4f, 0.3f, 0.4f, 0.5f, 0.3f, 0.4f, 0.4f };
        static const int powerYoffs[] = { 0, 5, 9, 5, 9, 7, 4, 5, 9, 4, 4 };

        static const int powerOrder[] = { kPwUpShadowCloak, kPwUpReflectShots, kPwUpDeathMask, kPwUpTwoGuns, kPwUpShadowCloakUseless, kPwUpFeatherFall,
                                        kPwUpGasMask, kPwUpDoppleganger, kPwUpAsbestArmor, kPwUpGrowShroom, kPwUpShrinkShroom };

        if (!hud_powerupduration)
            return;

        int powersort[nPowerUps];

        for (int i = 0; i < nPowerUps; i++) powersort[i] = i;

        for (int i = 0; i < nPowerUps; i++)
        {
            int power1 = powersort[i];
            for (int j = i + 1; j < nPowerUps; j++)
            {
                int power2 = powersort[j];
                if (pPlayer->pwUpTime[powerOrder[power1]] > pPlayer->pwUpTime[powerOrder[power2]])
                {
                    powersort[i] = power2;
                    powersort[j] = power1;
                }
            }
        }

        const int warningTime = 5;
        const int x = 15;
        int y = -50;
        for (int i = 0; i < nPowerUps; i++)
        {
            int order = powersort[i];
            int power = powerOrder[order];
            int time = pPlayer->pwUpTime[power];
            if (time > 0)
            {
                int remainingSeconds = time / 100;
                if (remainingSeconds > warningTime || (PlayClock & 32))
                {
                    DrawStatMaskedSprite(gPowerUpInfo[power].picnum, x, y + powerYoffs[order], 0, 0, 256, (int)(65536 * powerScale[order]), DI_SCREEN_LEFT_CENTER);
                }

                DrawStatNumber("%d", remainingSeconds, "SBarNumberInv", x + 15, y, 0, remainingSeconds > warningTime ? 0 : 2, 256, 65536 * 0.5, DI_SCREEN_LEFT_CENTER);
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
                    width += texWidth(gPackIcons[i]) + 1;
                }
            }
            width /= 2;
            x -= width;
            for (int i = 0; i < nPacks; i++)
            {
                int nPack = packs[i];
                DrawStatSprite("PackBG", x + 1, y - 8);
                DrawStatSprite("PackBG", x + 1, y - 6);
                DrawStatSprite(gPackIcons[nPack], x + 1, y + 1);
                if (nPack == pPlayer->packItemId)
                    DrawStatMaskedSprite("PackSelect", x + 1, y + 1);
                int nShade;
                if (pPlayer->packSlots[nPack].isActive)
                    nShade = 4;
                else
                    nShade = 24;
                DrawStatNumber("%3d", pPlayer->packSlots[nPack].curAmount, "SBarPackAmount", x - 4, y - 13, nShade, 0);
                x += texWidth(gPackIcons[nPack]) + 1;
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
        DrawStatNumber("%3d", pPlayer->packSlots[id].curAmount, "SBarPackAmount", x2, y2, 0, 0);
    }

    void DrawPackItemInStatusBar2(PLAYER* pPlayer, int x, int y, int x2, int y2, int nStat, int nScale)
    {
        static const char* packIcons2[] = { "Pack2Icon1", "Pack2Icon2", "Pack2Icon3", "Pack2Icon4", "Pack2Icon5" };
        static const float packScale[] = { 0.5f, 0.3f, 0.6f, 0.5f, 0.4f };
        static const int packYoffs[] = { 0, 0, 0, -4, 0 };

        if (pPlayer->packItemId < 0) return;

        DrawStatMaskedSprite(packIcons2[pPlayer->packItemId], x, y + packYoffs[pPlayer->packItemId], 0, 0, nStat, packScale[pPlayer->packItemId] * 65536);
        DrawStatNumber("%3d", pPlayer->packSlots[pPlayer->packItemId].curAmount, "SBarNumberInv", x2, y2, 0, 0, nStat, nScale);
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
                DrawStatSprite("SBPlayerSlot", -120 + nCol * 80, 4 + nRows * 9, 16, 0, 0, 65536, STYLE_Normal, DI_SCREEN_CENTER_TOP);
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
            const char* name = PlayerName(p);
            gTempStr.Format("%s", name);
            int color = CR_UNDEFINED;// todo: remap the colors. (11+col)
            SBar_DrawString(this, tinyf, gTempStr, x + 4, y, DI_SCREEN_CENTER_TOP, color, 1., -1, -1, 1, 1);
            gTempStr.Format("%2d", gPlayer[p].fragCount);
            SBar_DrawString(this, tinyf, gTempStr, x + 76, y, DI_SCREEN_CENTER_TOP, color, 1., -1, -1, 1, 1);
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
            const char* name = PlayerName(p);
            gTempStr.Format("%s", name);
            gTempStr.ToUpper();
            int color = CR_UNDEFINED;// todo: remap the colors.
            SBar_DrawString(this, tinyf, gTempStr, x + 4, y, DI_SCREEN_CENTER_TOP, color, 1., -1, -1, 1, 1);

            gTempStr = "F";
            x += 76;
            if (gPlayer[p].hasFlag & 2)
            {
                SBar_DrawString(this, tinyf, gTempStr, x, y, DI_SCREEN_CENTER_TOP, CR_GREEN/*12*/, 1., -1, -1, 1, 1);
                x -= 6;
            }

            if (gPlayer[p].hasFlag & 1)
                SBar_DrawString(this, tinyf, gTempStr, x, y, DI_SCREEN_CENTER_TOP, CR_RED/*11*/, 1., -1, -1, 1, 1);
        }
    }


    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void viewDrawCtfHudVanilla()
    {
        FString gTempStr;
        int x = 1, y = 1;
        if (team_ticker[0] == 0 || (PlayClock & 8))
        {
            SBar_DrawString(this, smallf, GStrings("TXT_COLOR_BLUE"), x, y, 0, CR_LIGHTBLUE, 1., -1, -1, 1, 1);
            gTempStr.Format("%-3d", team_score[0]);
            SBar_DrawString(this, smallf, gTempStr, x, y + 10, 0, CR_LIGHTBLUE, 1., -1, -1, 1, 1);
        }
        x = -2;
        if (team_ticker[1] == 0 || (PlayClock & 8))
        {
            SBar_DrawString(this, smallf, GStrings("TXT_COLOR_RED"), x, y, DI_TEXT_ALIGN_RIGHT, CR_BRICK, 1., -1, -1, 1, 1);
            gTempStr.Format("%3d", team_score[1]);
            SBar_DrawString(this, smallf, gTempStr, x, y + 10, DI_TEXT_ALIGN_RIGHT, CR_BRICK, 1., -1, -1, 1, 1);
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void flashTeamScore(int team, bool show)
    {
        assert(0 == team || 1 == team); // 0: blue, 1: red

        if (team_ticker[team] == 0 || (PlayClock & 8))
        {
             if (show)
                DrawStatNumber("%d", team_score[team], "SBarNumberInv", -30, team ? 25 : -10, 0, team ? 2 : 10, 512, 65536 * 0.75, DI_SCREEN_RIGHT_CENTER);
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void viewDrawCtfHud()
    {
        if (hud_size == Hud_Nothing)
        {
            flashTeamScore(0, false);
            flashTeamScore(1, false);
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
        DrawStatMaskedSprite(meHaveBlueFlag ? "FlagHave" : "FlagHaveNot", 0, 75-100, 0, 10, 512, 65536 * 0.35, DI_SCREEN_RIGHT_CENTER);
        if (gBlueFlagDropped)
            DrawStatMaskedSprite("FlagDropped", 305-320, 83 - 100, 0, 10, 512, 65536, DI_SCREEN_RIGHT_CENTER);
        else if (blueFlagTaken)
            DrawStatMaskedSprite("FlagTaken", 307-320, 77 - 100, 0, blueFlagCarrierColor ? 2 : 10, 512, 65536, DI_SCREEN_RIGHT_CENTER);
        flashTeamScore(0, true);

        bool meHaveRedFlag = gMe->hasFlag & 2;
        DrawStatMaskedSprite(meHaveRedFlag ? "FlagHave" : "FlagHaveNot", 0, 10, 0, 2, 512, 65536 * 0.35, DI_SCREEN_RIGHT_CENTER);
        if (gRedFlagDropped)
            DrawStatMaskedSprite("FlagDropped", 305-320, 17, 0, 2, 512, 65536, DI_SCREEN_RIGHT_CENTER);
        else if (redFlagTaken)
            DrawStatMaskedSprite("FlagTaken", 307-320, 11, 0, redFlagCarrierColor ? 2 : 10, 512, 65536, DI_SCREEN_RIGHT_CENTER);
        flashTeamScore(1, true);
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DrawStatusBar(int nPalette)
    {
        int th = texHeight("Statusbar");
        BeginStatusBar(320, 200, th);

        PLAYER* pPlayer = gView;
        XSPRITE* pXSprite = pPlayer->pXSprite;

        DrawStatMaskedSprite("Statusbar", 160, 200, 0, nPalette, RS_CENTERBOTTOM);
        DrawPackItemInStatusBar(pPlayer, 265, 186, 260, 172);

        if (pXSprite->health >= 16 || (PlayClock & 16) || pXSprite->health == 0)
        {
            DrawStatNumber("%3d", pXSprite->health >> 4, "SBarHealthAmount", 86, 183, 0, 0);
        }
        if (pPlayer->curWeapon && pPlayer->weaponAmmo != -1)
        {
            int num = pPlayer->ammoCount[pPlayer->weaponAmmo];
            if (pPlayer->weaponAmmo == 6)
                num /= 10;
            DrawStatNumber("%3d", num, "SBarWeaponNum", 216, 183, 0, 0);
        }
        for (int i = 9; i >= 1; i--)
        {
            int x = 135 + ((i - 1) / 3) * 23;
            int y = 182 + ((i - 1) % 3) * 6;
            int num = pPlayer->ammoCount[i];
            if (i == 6)
                num /= 10;
            DrawStatNumber("%3d", num, "SBarAmmoAmount", x, y, i == pPlayer->weaponAmmo? -128 : 32, 10);
        }
        DrawStatNumber("%2d", pPlayer->ammoCount[10], "SBarAmmoAmount", 291, 194, pPlayer->weaponAmmo == 10? -128 : 32, 10);
        DrawStatNumber("%2d", pPlayer->ammoCount[11], "SBarAmmoAmount", 309, 194, pPlayer->weaponAmmo == 11? -128 : 32, 10);

        if (pPlayer->armor[1])
        {
            TileHGauge("Armor1Gauge", 44, 174, pPlayer->armor[1], 3200);
            DrawStatNumber("%3d", pPlayer->armor[1] >> 4, "SBarAmmoAmount", 50, 177, 0, 0);
        }
        if (pPlayer->armor[0])
        {
            TileHGauge("Armor3Gauge", 44, 182, pPlayer->armor[0], 3200);
            DrawStatNumber("%3d", pPlayer->armor[0] >> 4, "SBarAmmoAmount", 50, 185, 0, 0);
        }
        if (pPlayer->armor[2])
        {
            TileHGauge("Armor2Gauge", 44, 190, pPlayer->armor[2], 3200);
            DrawStatNumber("%3d", pPlayer->armor[2] >> 4, "SBarAmmoAmount", 50, 193, 0, 0);
        }

        for (int i = 0; i < 6; i++)
        {
            FStringf nTile("KEYICON%d", i+1);
            double x = 73.5 + (i & 1) * 173;
            double y = 171.5 + (i >> 1) * 11;
            if (pPlayer->hasKey[i + 1])
                DrawStatSprite(nTile, x, y);
            else
                DrawStatSprite(nTile, x, y, 40, 5);
        }
        DrawStatMaskedSprite("BlinkIcon", 118.5, 185.5, /*pPlayer->isRunning ? 16 :*/ 40);
        DrawStatMaskedSprite("BlinkIcon", 201.5, 185.5, /*pPlayer->isRunning ? 16 :*/ 40);
        if (pPlayer->throwPower)
        {
            TileHGauge("ThrowGauge", 124, 175.5, pPlayer->throwPower, 65536);
        }
        drawInventory(pPlayer, 166, 200 - th);
        // Depending on the scale we can lower the stats display. This needs some tweaking but this catches the important default case already.
        PrintLevelStats(pPlayer, (hud_statscale <= 0.501f || hud_scalefactor < 0.7) && double(twod->GetWidth())/twod->GetHeight() > 1.6? 28 : 56);

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
        DrawStatSprite("FullHUD", 34, 187 - 200, 16, nPalette);
        if (pXSprite->health >= 16 || (PlayClock & 16) || pXSprite->health == 0)
        {
            DrawStatNumber("%3d", pXSprite->health >> 4, "SBarHealthAmount", 8, 183 - 200, 0, 0);
        }
        if (pPlayer->curWeapon && pPlayer->weaponAmmo != -1)
        {
            int num = pPlayer->ammoCount[pPlayer->weaponAmmo];
            if (pPlayer->weaponAmmo == 6)
                num /= 10;
            DrawStatNumber("%3d", num, "SBarWeaponNum", 42, 183 - 200, 0, 0);
        }
        DrawStatSprite("ArmorBox", 284-320, 187 - 200, 16, nPalette);
        if (pPlayer->armor[1])
        {
            TileHGauge("Armor1Gauge", 250-320, 175 - 200, pPlayer->armor[1], 3200);
            DrawStatNumber("%3d", pPlayer->armor[1] >> 4, "SBarAmmoAmount", 255-320, 178 - 200, 0, 0);
        }
        if (pPlayer->armor[0])
        {
            TileHGauge("Armor3Gauge", 250-320, 183 - 200, pPlayer->armor[0], 3200);
            DrawStatNumber("%3d", pPlayer->armor[0] >> 4, "SBarAmmoAmount", 255-320, 186 - 200, 0, 0);
        }
        if (pPlayer->armor[2])
        {
            TileHGauge("Armor2Gauge", 250-320, 191 - 200, pPlayer->armor[2], 3200);
            DrawStatNumber("%3d", pPlayer->armor[2] >> 4, "SBarAmmoAmount", 255-320, 194 - 200, 0, 0);
        }

        DrawPackItemInStatusBar(pPlayer, 286-320, 186 - 200, 302-320, 183 - 200);

        for (int i = 0; i < 6; i++)
        {
            FStringf nTile("KEYICON%d", i+1);
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
        static const char* ammoIcons[] = { nullptr, "AmmoIcon1", "AmmoIcon2", "AmmoIcon3", "AmmoIcon4", "AmmoIcon5", "AmmoIcon6",
                        "AmmoIcon7", "AmmoIcon8", "AmmoIcon9", "AmmoIcon10", "AmmoIcon11" };

        static const float ammoScale[] = { 0, 0.5f, 0.8f, 0.7f, 0.5f, 0.7f, 0.5f, 0.3f, 0.3f, 0.6f, 0.5f, 0.45f };
        static const int ammoYoffs[] = { 0, 0, 0, 3, -6, 2, 4, -6, -6, -6, 2, 2 };

        PLAYER* pPlayer = gView;
        XSPRITE* pXSprite = pPlayer->pXSprite;

        BeginHUD(320, 200, 1);
        DrawStatMaskedSprite("HealthIcon", 12, 195 - 200, 0, 0, 256, (int)(65536 * 0.56));
        DrawStatNumber("%d", pXSprite->health >> 4, "SBarNumberHealth", 28, 187 - 200, 0, 0, 256);
        if (pPlayer->armor[1])
        {
            DrawStatMaskedSprite("Armor1Icon", 70, 186 - 200, 0, 0, 256, (int)(65536 * 0.5));
            DrawStatNumber("%3d", pPlayer->armor[1] >> 4, "SBarNumberArmor2_", 83, 187 - 200, 0, 0, 256, (int)(65536 * 0.65));
        }
        if (pPlayer->armor[0])
        {
            DrawStatMaskedSprite("Armor3Icon", 112, 195 - 200, 0, 0, 256, (int)(65536 * 0.5));
            DrawStatNumber("%3d", pPlayer->armor[0] >> 4, "SBarNumberArmor1_", 125, 187 - 200, 0, 0, 256, (int)(65536 * 0.65));
        }
        if (pPlayer->armor[2])
        {
            DrawStatMaskedSprite("Armor2Icon", 155, 196 - 200, 0, 0, 256, (int)(65536 * 0.5));
            DrawStatNumber("%3d", pPlayer->armor[2] >> 4, "SBarNumberArmor3_", 170, 187 - 200, 0, 0, 256, (int)(65536 * 0.65));
        }

        DrawPackItemInStatusBar2(pPlayer, 216 - 320, 194 - 200, 231 - 320, 187 - 200, 512, (int)(65536 * 0.7));

        if (pPlayer->curWeapon && pPlayer->weaponAmmo != -1)
        {
            int num = pPlayer->ammoCount[pPlayer->weaponAmmo];
            if (pPlayer->weaponAmmo == 6)
                num /= 10;
            if (ammoIcons[pPlayer->weaponAmmo])
                DrawStatMaskedSprite(ammoIcons[pPlayer->weaponAmmo], 304-320, -8 + ammoYoffs[pPlayer->weaponAmmo],
                    0, 0, 512, ammoScale[pPlayer->weaponAmmo] * 65536);

            bool reloadableWeapon = pPlayer->curWeapon == 3 && !powerupCheck(pPlayer, kPwUpTwoGuns);
            if (!reloadableWeapon || (reloadableWeapon && !cl_showmagamt))
            {
                DrawStatNumber("%3d", num, "SBarNumberAmmo", 267-320, 187 - 200, 0, 0, 512);
            }
            else
            {
                FString format;
                short clip = CalcMagazineAmount(num, 2, pPlayer->weaponState == 1);
                short total = num - clip;
                format.Format("%d/%d", clip, num - clip);

                DrawCharArray(format.GetChars(), "SBarNumberAmmo", (total < 10 ? 267 : 258) - 320, 187 - 200, 0, 0, 512);
            }
        }

        for (int i = 0; i < 6; i++)
        {
            if (pPlayer->hasKey[i + 1])
            {
                FStringf tile("HUDKEYICON%d", i + 1);
                DrawStatMaskedSprite(tile, -60 + 10 * i, 170 - 200, 0, 0, 0, (int)(65536 * 0.25));
            }
        }

        BeginStatusBar(320, 200, 28);
        if (pPlayer->throwPower)
            TileHGauge("ThrowGauge", 124, 175, pPlayer->throwPower, 65536);
        else
            drawInventory(pPlayer, 166, 200-texHeight("FULLHUD") / 2 - 30);
        PrintLevelStats(pPlayer, 28);
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------
    public:
    void UpdateStatusBar()
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
                TileHGauge("ThrowGauge", 124, 175, pPlayer->throwPower, 65536);
            else if (hud_size > Hud_StbarOverlay)
                drawInventory(pPlayer, 166, 200 - tileHeight(2201) / 2);
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
                    viewDrawCtfHudVanilla();
                }
                else
                {
                    viewDrawCtfHud();
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

IMPLEMENT_CLASS(DBloodStatusBar, false, false)


static void UpdateFrame(void)
{
    auto tex = tileGetTexture(kBackTile);
    int width = twod->GetWidth();
    int height = twod->GetHeight();

    twod->AddFlatFill(0, 0, width, windowxy1.y - 3, tex);
    twod->AddFlatFill(0, windowxy2.y + 4, width, height, tex);
    twod->AddFlatFill(0, windowxy1.y - 3, windowxy1.x - 3, windowxy2.y + 4, tex);
    twod->AddFlatFill(windowxy2.x + 4, windowxy1.y - 3, width, windowxy2.y + 4, tex);

    twod->AddFlatFill(windowxy1.x - 3, windowxy1.y - 3, windowxy1.x, windowxy2.y + 1, tex, 0, 1, 0xff545454);
    twod->AddFlatFill(windowxy1.x, windowxy1.y - 3, windowxy2.x + 4, windowxy1.y, tex, 0, 1, 0xff545454);
    twod->AddFlatFill(windowxy2.x + 1, windowxy1.y, windowxy2.x + 4, windowxy2.y + 4, tex, 0, 1, 0xff2a2a2a);
    twod->AddFlatFill(windowxy1.x - 3, windowxy2.y + 1, windowxy2.x + 1, windowxy2.y + 4, tex, 0, 1, 0xff2a2a2a);
}

void UpdateStatusBar()
{
    if (automapMode == am_off && hud_size <= Hud_Stbar)
    {
        UpdateFrame();
    }

    StatusBar->UpdateStatusBar();
}


END_BLD_NS
