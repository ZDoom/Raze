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

#undef MAIN
#include "build.h"

#include "game.h"
#include "names2.h"
#include "panel.h"
#include "pal.h"
#include "misc.h"
#include "player.h"
#include "v_2ddrawer.h"
#include "statusbar.h"
#include "network.h"
#include "v_draw.h"
#include "menus.h"
#include "automap.h"


BEGIN_SW_NS

enum
{
    ID_PanelMedkit = 2396,
    ID_PanelRepairKit = 2399,
    ID_PanelCloak = 2397, //2400
    ID_PanelNightVision = 2398,
    ID_PanelChemBomb = 2407,
    ID_PanelFlashBomb = 2408,
    ID_PanelCaltrops = 2409,
};

static const short icons[] = {
    ID_PanelMedkit,
    ID_PanelRepairKit,
    ID_PanelCloak,
    ID_PanelNightVision,
    ID_PanelChemBomb,
    ID_PanelFlashBomb,
    ID_PanelCaltrops,
};

class DSWStatusBar : public DBaseStatusBar
{
    DHUDFont miniFont, numberFont;

    enum
    {
        PANEL_HEALTH_BOX_X = 20,
        PANEL_BOX_Y = (174 - 6),
        PANEL_HEALTH_XOFF = 2,
        PANEL_HEALTH_YOFF = 4,

        PANEL_AMMO_BOX_X = 197,
        PANEL_AMMO_XOFF = 1,
        PANEL_AMMO_YOFF = 4,

        WSUM_X = 93,
        WSUM_Y = PANEL_BOX_Y+1,
        WSUM_XOFF = 25,
        WSUM_YOFF = 6,

        PANEL_KEYS_BOX_X = 276,
        PANEL_KEYS_XOFF = 0,
        PANEL_KEYS_YOFF = 2,

        PANEL_ARMOR_BOX_X = 56,
        PANEL_ARMOR_XOFF = 2,
        PANEL_ARMOR_YOFF = 4,

        FRAG_YOFF = 2,

        INVENTORY_BOX_X = 231,
        INVENTORY_BOX_Y = (176-8),
        
        INVENTORY_PIC_XOFF = 1,
        INVENTORY_PIC_YOFF = 1,
        
        INVENTORY_PERCENT_XOFF = 19,
        INVENTORY_PERCENT_YOFF = 13,
        
        INVENTORY_STATE_XOFF = 19,
        INVENTORY_STATE_YOFF = 1,

        MINI_BAR_Y = 174  ,
        MINI_BAR_HEALTH_BOX_X = 4,
        MINI_BAR_AMMO_BOX_X = 32,
        MINI_BAR_INVENTORY_BOX_X = 64,
        MINI_BAR_INVENTORY_BOX_Y = MINI_BAR_Y,

    };

    enum
    {
        PANEL_FONT_G = 3636,
        PANEL_FONT_Y = 3646,
        PANEL_FONT_R = 3656,

        PANEL_SM_FONT_G = 3601,
        PANEL_SM_FONT_Y = 3613,
        PANEL_SM_FONT_R = 3625,

        PANEL_KEY_RED       = 2392,
        PANEL_KEY_GREEN     = 2393,
        PANEL_KEY_BLUE      = 2394,
        PANEL_KEY_YELLOW    = 2395,
        PANEL_SKELKEY_GOLD  = 2448,
        PANEL_SKELKEY_SILVER=  2449,
        PANEL_SKELKEY_BRONZE=  2458,
        PANEL_SKELKEY_RED   = 2459,

        MINI_BAR_HEALTH_BOX_PIC = 2437,
        MINI_BAR_AMMO_BOX_PIC = 2437,
        MINI_BAR_INVENTORY_BOX_PIC = 2438,

        ID_SelectionBox = 2435,
    };


public:
    DSWStatusBar()
    {
        numberFont = { BigFont, 0, Off, 1, 1 };
        miniFont = { SmallFont2, 0, Off, 1, 1 };
    }

private:
    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DisplayPanelNumber(double xs, double ys, int number)
    {
        char buffer[32];
        char* ptr;
        double x;

        mysnprintf(buffer, 32, "%03d", number);

        for (ptr = buffer, x = xs; *ptr; ptr++)
        {
            if (!isdigit(*ptr))
            {
                continue;
            }
            int tex = PANEL_FONT_G + (*ptr - '0');
            DrawGraphic(tileGetTexture(tex), x, ys, DI_ITEM_LEFT_TOP, 1, -1, -1, 1, 1);
            x += tileWidth(tex) + 1;
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DisplaySummaryString(double xs, double ys, int color, int shade, const char* buffer)
    {
        double x;
        const char* ptr;
        char ch;
        int font_pic;
        static const short font_base[] = { PANEL_SM_FONT_G, PANEL_SM_FONT_Y, PANEL_SM_FONT_R };

        assert(color < 3);
        for (ptr = buffer, x = xs; *ptr; ptr++)
        {
            ch = *ptr;
            if (ch == ' ')
            {
                x += 4;
                continue;
            }

            switch (ch)
            {
            case '\\':
                ch = '0' - 1; // one pic before 0
                break;
            case ':':
                ch = '9' + 1; // one pic after nine
                break;
            }

            font_pic = font_base[color] + (ch - '0');
            DrawGraphic(tileGetTexture(font_pic), x, ys, DI_ITEM_LEFT_TOP, 1, -1, -1, 1, 1, shadeToLight(shade));
            x += tilesiz[font_pic].x + 1;
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DisplayTimeLimit(PLAYERp pp)
    {
        int seconds = gNet.TimeLimitClock / 120;
        sprintf(ds, "%03d:%02d", seconds / 60, seconds % 60);
        DisplaySummaryString(PANEL_KEYS_BOX_X + 1, PANEL_BOX_Y + 6, 0, 0, ds);
    }

    //---------------------------------------------------------------------------
    //
    // todo: migrate to FFont to support localization
    //
    //---------------------------------------------------------------------------

    void DisplayTinyString(double xs, double ys, const char* buffer, int pal)
    {
        SBar_DrawString(this, &miniFont, buffer, xs, ys, DI_ITEM_LEFT_TOP, TRANSLATION(Translation_Remap, pal), 1, -1, -1, 1, 1);
    }

    void DisplayFragString(PLAYERp pp, double xs, double ys, const char* buffer)
    {
        DisplayTinyString(xs, ys, buffer, User[pp->SpriteP - sprite]->spal);
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DisplayFragNumbers()
    {
        for (int pnum = 0; pnum < 4; pnum++)
        {
            char buffer[32];
            short xs, ys;
            short frag_bar;

            static int xoffs[] =
            {
                69, 147, 225, 303
            };

            ys = FRAG_YOFF;

            // frag bar 0 or 1
            frag_bar = ((pnum) / 4);
            // move y down according to frag bar number
            ys = ys + (tilesiz[FRAG_BAR].y - 2) * frag_bar;

            // move x over according to the number of players
            xs = xoffs[MOD4(pnum)];

            mysnprintf(buffer, 32, "%03d", Player[pnum].Kills);

            DisplayFragString(&Player[pnum], xs, ys, buffer);
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DisplayFragNames()
    {
        for (int pnum = 0; pnum < 4; pnum++)
        {
            short xs, ys;
            short frag_bar;

            static int xoffs[] =
            {
                7, 85, 163, 241
            };

            ys = FRAG_YOFF;

            // frag bar 0 or 1
            frag_bar = ((pnum) / 4);
            // move y down according to frag bar number
            ys = ys + (tilesiz[FRAG_BAR].y - 2) * frag_bar;

            // move x over according to the number of players
            xs = xoffs[MOD4(pnum)];

            DisplayFragString(&Player[pnum], xs, ys, Player[pnum].PlayerName);
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DisplayFragBar(PLAYERp pp)
    {
        // must draw this in HUD mode and align to the top center
        short i, num_frag_bars;
        int y;

        if (numplayers <= 1)
            return;

        if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
            return;

        // if player sprite has not been initialized we have no business
        // sticking a frag bar up.  Prevents processing from MenuLevel etc.
        if (!pp->SpriteP)
            return;

        //num_frag_bars = ((numplayers-1)/4)+1;
        num_frag_bars = ((OrigCommPlayers - 1) / 4) + 1;

        for (i = windowxy1.x; i <= windowxy2.x; i++)
        {
            y = (tilesiz[FRAG_BAR].y * num_frag_bars) - (2 * (num_frag_bars - 1));
            y = y * (ydim / 200.0);
        }

        for (i = 0, y = 0; i < num_frag_bars; i++)
        {
            DrawGraphic(tileGetTexture(FRAG_BAR), 0, y, DI_ITEM_LEFT_TOP, 1, -1, -1, 1, 1);
            y += tilesiz[FRAG_BAR].y - 2;
        }
        DisplayFragNames();
        DisplayFragNumbers();
    }


    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void PlayerUpdateWeaponSummary(PLAYERp pp, int UpdateWeaponNum)
    {
        USERp u = User[pp->PlayerSprite];
        int x, y;
        int pos;
        int column;
        int WeaponNum, wpntmp;
        int color, shade;
        char ds[32];

        WeaponNum = UpdateWeaponNum;

        if (DamageData[WeaponNum].with_weapon != -1)
        {
            WeaponNum = DamageData[WeaponNum].with_weapon;
        }

        static short wsum_xoff[3] = { 0,36,66 };
        static const char* wsum_fmt2[3] = { "%3d/%-3d", "%2d/%-2d", "%2d/%-2d" };

        pos = WeaponNum - 1;
        column = pos / 3;
        if (column > 2) column = 2;
        x = WSUM_X + wsum_xoff[column];
        y = WSUM_Y + (WSUM_YOFF * (pos % 3));

        if (UpdateWeaponNum == u->WeaponNum)
        {
            shade = 0;
            color = 0;
        }
        else
        {
            shade = 11;
            color = 0;
        }

        wpntmp = WeaponNum + 1;
        if (wpntmp > 9)
            wpntmp = 0;
        mysnprintf(ds, 32, "%d:", wpntmp);

        if (TEST(pp->WpnFlags, BIT(WeaponNum)))
            DisplaySummaryString(x, y, 1, shade, ds);
        else
            DisplaySummaryString(x, y, 2, shade + 6, ds);

        mysnprintf(ds, 32, wsum_fmt2[column], pp->WpnAmmo[WeaponNum], DamageData[WeaponNum].max_ammo);
        DisplaySummaryString(x + 6, y, color, shade, ds);
    }

    void PlayerUpdateWeaponSummaryAll(PLAYERp pp)
    {
        for (int i = WPN_STAR; i <= WPN_HEART; i++)
        {
            PlayerUpdateWeaponSummary(pp, i);
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DisplayKeys(PLAYERp pp, double xs, double ys, double scalex = 1, double scaley = 1)
    {
        double x, y;
        int row, col;
        int i, xsize, ysize;

        static short StatusKeyPics[] =
        {
            PANEL_KEY_RED,
            PANEL_KEY_BLUE,
            PANEL_KEY_GREEN,
            PANEL_KEY_YELLOW,
            PANEL_SKELKEY_GOLD,
            PANEL_SKELKEY_SILVER,
            PANEL_SKELKEY_BRONZE,
            PANEL_SKELKEY_RED
        };


        xsize = tilesiz[PANEL_KEY_RED].x + 1;
        ysize = tilesiz[PANEL_KEY_RED].y + 2;

        i = 0;
        for (row = 0; row < 2; row++)
        {
            for (col = 0; col < 2; col++)
            {
                if (pp->HasKey[i])
                {
                    x = xs + PANEL_KEYS_XOFF + (row * xsize);
                    y = ys + PANEL_KEYS_YOFF + (col * ysize);
                    DrawGraphic(tileGetTexture(StatusKeyPics[i]), x, y, DI_ITEM_LEFT_TOP, 1, -1, -1, scalex, scaley);
                }
                i++;
            }
        }

        // Check for skeleton keys
        i = 0;
        for (row = 0; row < 2; row++)
        {
            for (col = 0; col < 2; col++)
            {
                if (pp->HasKey[i + 4])
                {
                    x = xs + PANEL_KEYS_XOFF + (row * xsize);
                    y = ys + PANEL_KEYS_YOFF + (col * ysize);
                    DrawGraphic(tileGetTexture(StatusKeyPics[i + 4]), x, y, DI_ITEM_LEFT_TOP, 1, -1, -1, scalex, scaley);
                }
                i++;
            }
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void PlayerUpdateInventoryPercent(PLAYERp pp, int InventoryBoxX, int InventoryBoxY, int InventoryXoff, int InventoryYoff)
    {
        char ds[32];
        INVENTORY_DATAp id = &InventoryData[pp->InventoryNum];

        int x = InventoryBoxX + INVENTORY_PERCENT_XOFF + InventoryXoff;
        int y = InventoryBoxY + INVENTORY_PERCENT_YOFF + InventoryYoff;

        if (TEST(id->Flags, INVF_COUNT))
        {
            mysnprintf(ds, 32, "%d", pp->InventoryAmount[pp->InventoryNum]);
        }
        else
        {
            mysnprintf(ds, 32, "%d%c", pp->InventoryPercent[pp->InventoryNum], '%');
        }
        DisplayTinyString(x, y, ds, 0);
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void PlayerUpdateInventoryPic(PLAYERp pp, int InventoryBoxX, int InventoryBoxY, int InventoryXoff, int InventoryYoff)
    {
        INVENTORY_DATAp id = &InventoryData[pp->InventoryNum];
        int x = InventoryBoxX + INVENTORY_PIC_XOFF + InventoryXoff;
        int y = InventoryBoxY + INVENTORY_PIC_YOFF + InventoryYoff;
        DrawGraphic(tileGetTexture(icons[pp->InventoryNum]), x, y, DI_ITEM_LEFT_TOP, 1, -1, -1, id->Scale/65536., id->Scale / 65536.);
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void PlayerUpdateInventoryState(PLAYERp pp, double InventoryBoxX, double InventoryBoxY, int InventoryXoff, int InventoryYoff)
    {
        char ds[32];
        INVENTORY_DATAp id = &InventoryData[pp->InventoryNum];

        double x = InventoryBoxX + INVENTORY_STATE_XOFF + InventoryXoff;
        double y = InventoryBoxY + INVENTORY_STATE_YOFF + InventoryYoff;

        if (TEST(id->Flags, INVF_AUTO_USE))
        {
            DisplayTinyString(x, y, "AUTO", 0);
        }
        else if (TEST(id->Flags, INVF_TIMED))
        {
            DisplayTinyString(x, y, pp->InventoryActive[pp->InventoryNum] ? "ON" : "OFF", 0);
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DisplayBarInventory(PLAYERp pp)
    {
        int InventoryBoxX = INVENTORY_BOX_X;
        int InventoryBoxY = INVENTORY_BOX_Y;

        int InventoryXoff = -1;
        int InventoryYoff = 0;

        // put pic
        if (pp->InventoryAmount[pp->InventoryNum])
        {
            PlayerUpdateInventoryPic(pp, InventoryBoxX, InventoryBoxY, 0, InventoryYoff);
            // Auto/On/Off
            PlayerUpdateInventoryState(pp, InventoryBoxX, InventoryBoxY, InventoryXoff, InventoryYoff);
            // Percent count/Item count
            PlayerUpdateInventoryPercent(pp, InventoryBoxX, InventoryBoxY, InventoryXoff, InventoryYoff);
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DrawCompass(PLAYERp pp)
    {
        enum
        {
            COMPASS_TIC    = 2380,
            COMPASS_TIC2   = 2381,

            COMPASS_NORTH  = 2382,
            COMPASS_NORTH2 = 2383,

            COMPASS_SOUTH  = 2384,
            COMPASS_SOUTH2 = 2385,

            COMPASS_EAST   = 2386,
            COMPASS_EAST2  = 2387,

            COMPASS_WEST   = 2388,
            COMPASS_WEST2  = 2389,

            COMPASS_MID_TIC   = 2390,
            COMPASS_MID_TIC2  = 2391,

            COMPASS_X  = 140,
            COMPASS_Y  = (162-5),
        };

        auto NORM_CANG = [](int ang) { return (((ang)+32) & 31); };

        int start_ang, ang;
        int x_size = tilesiz[COMPASS_NORTH].x;
        int x;
        int i;

        static const short CompassPic[32] =
        {
            COMPASS_EAST, COMPASS_EAST2,
            COMPASS_TIC, COMPASS_TIC2,
            COMPASS_MID_TIC, COMPASS_MID_TIC2,
            COMPASS_TIC, COMPASS_TIC2,

            COMPASS_SOUTH, COMPASS_SOUTH2,
            COMPASS_TIC, COMPASS_TIC2,
            COMPASS_MID_TIC, COMPASS_MID_TIC2,
            COMPASS_TIC, COMPASS_TIC2,

            COMPASS_WEST, COMPASS_WEST2,
            COMPASS_TIC, COMPASS_TIC2,
            COMPASS_MID_TIC, COMPASS_MID_TIC2,
            COMPASS_TIC, COMPASS_TIC2,

            COMPASS_NORTH, COMPASS_NORTH2,
            COMPASS_TIC, COMPASS_TIC2,
            COMPASS_MID_TIC, COMPASS_MID_TIC2,
            COMPASS_TIC, COMPASS_TIC2,
        };

        static const short CompassShade[10] =
        {
            //20, 16, 11, 6, 1, 1, 6, 11, 16, 20
            25, 19, 15, 9, 1, 1, 9, 15, 19, 25
        };

        ang = pp->angle.ang.asbuild();

        if (pp->sop_remote)
            ang = 0;

        start_ang = (ang + 32) >> 6;

        start_ang = NORM_CANG(start_ang - 4);

        for (i = 0, x = COMPASS_X; i < 10; i++)
        {
            DrawGraphic(tileGetTexture(CompassPic[NORM_CANG(start_ang + i)]), x, COMPASS_Y, DI_ITEM_LEFT_TOP, 1, -1, -1, 1, 1, shadeToLight(CompassShade[i]));
            x += x_size;
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DrawStatusBar()
    {
        auto pp = Player + screenpeek;
        USERp u = User[pp->PlayerSprite];
        BeginStatusBar(320, 200, tileHeight(STATUS_BAR));

        if (hud_size == Hud_StbarOverlay) Set43ClipRect();
        int left = (320 - tilesiz[STATUS_BAR].x) / 2;
        DrawGraphic(tileGetTexture(STATUS_BAR), left, 200, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, 1, 1);
        twod->ClearClipRect();
        DisplayPanelNumber(PANEL_HEALTH_BOX_X + PANEL_HEALTH_XOFF, PANEL_BOX_Y + PANEL_HEALTH_YOFF, u->Health);
        DisplayPanelNumber(PANEL_ARMOR_BOX_X + PANEL_ARMOR_XOFF, PANEL_BOX_Y + PANEL_ARMOR_YOFF, pp->Armor);
        if (u->WeaponNum != WPN_FIST && u->WeaponNum != WPN_SWORD)
            DisplayPanelNumber(PANEL_AMMO_BOX_X + PANEL_AMMO_XOFF, PANEL_BOX_Y + PANEL_AMMO_YOFF, pp->WpnAmmo[u->WeaponNum]);
        PlayerUpdateWeaponSummaryAll(pp);
        if (gNet.MultiGameType != MULTI_GAME_COMMBAT)
            DisplayKeys(pp, PANEL_KEYS_BOX_X, PANEL_BOX_Y);
        else if (gNet.TimeLimit)
            DisplayTimeLimit(pp);
        DisplayBarInventory(pp);
        DrawCompass(pp);
        PrintLevelStats(-1);
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DisplayMinibarInventory(PLAYERp pp)
    {
        int InventoryBoxX = MINI_BAR_INVENTORY_BOX_X;
        int InventoryBoxY = MINI_BAR_INVENTORY_BOX_Y;

        int InventoryXoff = 0;
        int InventoryYoff = 1;

        if (pp->InventoryAmount[pp->InventoryNum])
        {
            PlayerUpdateInventoryPic(pp, InventoryBoxX, InventoryBoxY, InventoryXoff+1, InventoryYoff);
            // Auto/On/Off
            PlayerUpdateInventoryState(pp, InventoryBoxX, InventoryBoxY, InventoryXoff, InventoryYoff);
            // Percent count/Item count
            PlayerUpdateInventoryPercent(pp, InventoryBoxX, InventoryBoxY, InventoryXoff, InventoryYoff);
        }
    }

    //---------------------------------------------------------------------------
    //
    // Used in DrawHUD2() for determining whether a reloadable weapon is reloading.
    //
    //---------------------------------------------------------------------------

    bool DoReloadStatus(char *reloadstate, int ammo)
    {
        bool reloading = ammo == 0 && *reloadstate != 2;

        if (ammo == 0 && *reloadstate == 0)
        {
            *reloadstate = 1;
        }
        else if (ammo)
        {
            *reloadstate = 0;
        }

        return reloading;
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DrawHUD1()
    {
        BeginHUD(320, 200, 1);
        auto pp = Player + screenpeek;
        USERp u = User[pp->PlayerSprite];
        int x, y;
        INVENTORY_DATAp id;

        x = MINI_BAR_HEALTH_BOX_X;
        y = -26;

        DrawGraphic(tileGetTexture(MINI_BAR_HEALTH_BOX_PIC), x, y, DI_ITEM_LEFT_TOP, 1, -1, -1, 1, 1);

        x = MINI_BAR_HEALTH_BOX_X + 3;
        DisplayPanelNumber(x, y + 5, u->Health);

        if (u->WeaponNum != WPN_SWORD && u->WeaponNum != WPN_FIST)
        {
            x = MINI_BAR_AMMO_BOX_X;
            DrawGraphic(tileGetTexture(MINI_BAR_AMMO_BOX_PIC), x, y, DI_ITEM_LEFT_TOP, 1, -1, -1, 1, 1);

            x = MINI_BAR_AMMO_BOX_X + 3;
            DisplayPanelNumber(x, y + 5, pp->WpnAmmo[u->WeaponNum]);
        }
        PrintLevelStats(30);

        if (!pp->InventoryAmount[pp->InventoryNum])
            return;

        // Inventory Box
        x = MINI_BAR_INVENTORY_BOX_X;

        DrawGraphic(tileGetTexture(MINI_BAR_INVENTORY_BOX_PIC), x, y, DI_ITEM_LEFT_TOP, 1, -1, -1, 1, 1);
        DisplayMinibarInventory(pp);
    }

    //==========================================================================
    //
    // Fullscreen HUD variant #1
    //
    //==========================================================================

    void DrawHUD2()
    {
        BeginHUD(320, 200, 1);

        auto pp = Player + screenpeek;
        USERp u = User[pp->PlayerSprite];

        FString format;
        FGameTexture* img;
        double imgScale;
        double baseScale = numberFont.mFont->GetHeight() * 0.9375;

        //
        // Health
        //
        img = tileGetTexture(ICON_SM_MEDKIT);
        imgScale = baseScale / img->GetDisplayHeight();
        DrawGraphic(img, 1.5, -1, DI_ITEM_LEFT_BOTTOM, 1., -1, -1, imgScale, imgScale);

        if (!althud_flashing || u->Health > (u->MaxHealth >> 2) || (PlayClock & 32))
        {
            int s = -8;
            if (althud_flashing && u->Health > u->MaxHealth)
                s += (sintable[(PlayClock << 5) & 2047] >> 10);
            int intens = clamp(255 - 4 * s, 0, 255);
            auto pe = PalEntry(255, intens, intens, intens);
            format.Format("%d", u->Health);
            SBar_DrawString(this, &numberFont, format, 24.25, -numberFont.mFont->GetHeight(), DI_TEXT_ALIGN_LEFT, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
        }

        //
        // Armor
        //
        img = tileGetTexture(ICON_ARMOR);
        imgScale = baseScale / img->GetDisplayHeight();
        DrawGraphic(img, 80.75, -1, DI_ITEM_LEFT_BOTTOM, 1., -1, -1, imgScale, imgScale);

        format.Format("%d", pp->Armor);
        SBar_DrawString(this, &numberFont, format, 108.5, -numberFont.mFont->GetHeight(), DI_TEXT_ALIGN_LEFT, CR_UNTRANSLATED, 1, 0, 0, 1, 1);

        //
        // Weapon
        //
        const short ammo_sprites[] = { -1, ICON_STAR, ICON_LG_SHOTSHELL, ICON_LG_UZI_AMMO, ICON_MICRO_BATTERY, ICON_LG_GRENADE, ICON_LG_MINE, ICON_RAIL_AMMO,
            ICON_FIREBALL_LG_AMMO, ICON_HEART_LG_AMMO, ICON_FIREBALL_LG_AMMO, ICON_FIREBALL_LG_AMMO,ICON_MICRO_BATTERY, -1 };

        int weapon = u->WeaponNum;
        int wicon = ammo_sprites[weapon];
        if (wicon > 0)
        {
            int ammo = pp->WpnAmmo[weapon];
            bool reloadableWeapon = weapon == WPN_SHOTGUN || weapon == WPN_UZI;
            if (!reloadableWeapon || (reloadableWeapon && !cl_showmagamt))
            {
                format.Format("%d", ammo);
            }
            else
            {
                short capacity;
                switch (weapon)
                {
                    case WPN_SHOTGUN:
                        capacity = 4;
                        break;
                    case WPN_UZI:
                        capacity = pp->WpnUziType ? 50 : 100;
                        break;
                }
                short clip = CalcMagazineAmount(ammo, capacity, DoReloadStatus(&pp->WpnReloadState, ammo % capacity));
                format.Format("%d/%d", clip, ammo - clip);
            }
            img = tileGetTexture(wicon);
            imgScale = baseScale / img->GetDisplayHeight();
            auto imgX = 21.125;
            auto strlen = format.Len();

            if (strlen > 1)
            {
                imgX += (imgX * 0.855) * (strlen - 1);
            }

            if ((!althud_flashing || PlayClock & 32 || ammo > (DamageData[weapon].max_ammo / 10)))
            {
                SBar_DrawString(this, &numberFont, format, -1.5, -numberFont.mFont->GetHeight(), DI_TEXT_ALIGN_RIGHT, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
            }

            DrawGraphic(img, -imgX, -1, DI_ITEM_RIGHT_BOTTOM, 1, -1, -1, imgScale, imgScale);
        }

        //
        // Selected inventory item
        //
        img = tileGetTexture(icons[pp->InventoryNum]);
        imgScale = baseScale / img->GetDisplayHeight();
        int x = 165;
        DrawGraphic(img, x, -1, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, imgScale, imgScale);

        PlayerUpdateInventoryState(pp, x + 3.0, -18.0, 1, 1);
        PlayerUpdateInventoryPercent(pp, x + 3.5, -20.5, 1, 1);

        //
        // keys
        //
        DisplayKeys(pp, -25, -38, 0.8625, 0.8625);
        PrintLevelStats(int(baseScale + 4));
    }


    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DrawInventoryIcon(double xs, double ys, int align, int InventoryNum, int amount, bool selected)
    {
        double x, y;
        const int INVENTORY_ICON_WIDTH = 28;

        // check invalid value
        assert(InventoryNum < MAX_INVENTORY);

        x =xs + (InventoryNum * INVENTORY_ICON_WIDTH);
        y = ys;
        auto tex = icons[InventoryNum];
        auto scale = InventoryData[InventoryNum].Scale / 65536.;
        DrawGraphic(tileGetTexture(tex), x, y, align | DI_ITEM_LEFT_TOP, amount? 1. : 0.333, -1, -1, scale, scale);
        if (selected)
        {
            DrawGraphic(tileGetTexture(ID_SelectionBox), x-5, y-5, align | DI_ITEM_LEFT_TOP, 1, -1, -1, 1, 1);
        }
    }

    //////////////////////////////////////////////////////////////////////
    //
    // INVENTORY BAR
    //
    //////////////////////////////////////////////////////////////////////

    void DrawInventory(double xs, double ys, int align)
    {
        auto pp = Player + screenpeek;
        short inv = 0;
        INVENTORY_DATAp id;

        if (!pp->InventoryBarTics)
        {
            return;
        }

        for (id = InventoryData; id->Name; id++, inv++)
        {
            DrawInventoryIcon(xs, ys, align, inv, pp->InventoryAmount[inv], inv == pp->InventoryNum);
        }
    }

    //==========================================================================
    //
    // Statistics output
    //
    //==========================================================================

    void PrintLevelStats(int bottomy)
    {
        FLevelStats stats{};
        stats.fontscale = 1;
        stats.spacing = 7;
        stats.screenbottomspace = bottomy;
        stats.font = SmallFont;
        stats.time = Scale(PlayClock, 1000, 120);

		if (automapMode == am_full)
		{
            stats.letterColor = CR_SAPPHIRE;
            stats.standardColor = CR_UNTRANSLATED;

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
            else stats.spacing = SmallFont->GetHeight() + 1;
            DBaseStatusBar::PrintAutomapInfo(stats, textfont);
		}
        // JBF 20040124: display level stats in screen corner
        else if (hud_stats && !(CommEnabled || numplayers > 1))
        {
            auto pp = Player + screenpeek;

            stats.kills = Player->Kills;
            stats.maxkills = TotalKillable;
            stats.frags = -1;
            stats.secrets = Player->SecretsFound;
            stats.maxsecrets = LevelSecrets;

            stats.letterColor = CR_RED;
            stats.standardColor = CR_TAN;
            stats.completeColor = CR_FIRE;

            DBaseStatusBar::PrintLevelStats(stats);
        }
    }


    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------
public:
    void UpdateStatusBar()
    {
        int nPalette = 0;
        double inv_x, inv_y;
        int align;

        if (hud_size == Hud_Nothing)
        {
            align = DI_SCREEN_RIGHT_BOTTOM;
            inv_x = -210 * hud_scale;
            inv_y = -28 * hud_scale;
            PrintLevelStats(2);
        }
        else if (hud_size == Hud_full)
        {
            align = DI_SCREEN_CENTER_BOTTOM;
            inv_x = -80 * hud_scale;
            inv_y = -40 * hud_scale;
            DrawHUD2();
        }
        else if (hud_size == Hud_Mini)
        {
            align = DI_SCREEN_RIGHT_BOTTOM;
            inv_x = -210 * hud_scale;
            inv_y = -28 * hud_scale;
            DrawHUD1();
        }
        else
        {
            align = 0;
            inv_x = 80 * hud_scale;
            inv_y = 130 * hud_scale;
            DrawStatusBar();
        }
        DrawInventory(inv_x, inv_y, align);
    }

};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void UpdateFrame(void)
{
	static const int kBackTile = 51;
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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DrawConString(int x, int y, const char* string, double alpha)
{
    x = x * 2 - SmallFont->StringWidth(string) / 2;
    y *= 2;
    DrawText(twod, SmallFont, CR_TAN, x, y, string, DTA_FullscreenScale, FSMode_Fit640x400, DTA_Alpha, alpha, TAG_DONE);

}


void UpdateStatusBar()
{
    DSWStatusBar sbar;

    if (hud_size <= Hud_Stbar)
    {
        UpdateFrame();
    }

    sbar.UpdateStatusBar();
    PLAYERp pp = &Player[screenpeek];
    if (pp->cookieTime > 0)
    {
        const int MESSAGE_LINE = 142;    // Used to be 164
        
        if (!SmallFont2->CanPrint(pp->cookieQuote))
            DrawConString(160, MESSAGE_LINE, pp->cookieQuote, clamp(pp->cookieTime / 60., 0., 1.));
        else
            MNU_DrawSmallString(160, MESSAGE_LINE, pp->cookieQuote, 0, 0, 0, clamp(pp->cookieTime / 60., 0., 1.));
    }
}



END_SW_NS
