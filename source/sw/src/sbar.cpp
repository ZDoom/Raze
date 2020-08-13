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

#include "names2.h"
#include "panel.h"
#include "game.h"
#include "pal.h"
#include "misc.h"
#include "player.h"
#include "v_2ddrawer.h"
#include "statusbar.h"
#include "network.h"

BEGIN_SW_NS


class DSWStatusBar : public DBaseStatusBar
{

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

    };

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
    // 
    //
    //---------------------------------------------------------------------------

    void DisplayFragString(PLAYERp pp, double xs, double ys, const char* buffer)
    {
        double x;
        const char* ptr;

        const int FRAG_FIRST_ASCII = ('!');
        const int FRAG_FIRST_TILE = 2930;

        for (ptr = buffer, x = xs; *ptr; ptr++)
        {
            if (*ptr == ' ')
                continue;

            assert(*ptr >= '!' && *ptr <= '}');

            auto tex = tileGetTexture(FRAG_FIRST_TILE + (*ptr - FRAG_FIRST_ASCII));
            DrawGraphic(tex, x, ys, DI_ITEM_LEFT_TOP, 1, -1, -1, 1, 1, 0xffffffff, TRANSLATION(Translation_Remap, User[pp->SpriteP - sprite]->spal));
            x += 4;
        }
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void DisplayFragNumbers()
    {
        // must draw this in HUD mode!
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
        // must draw this in HUD mode!
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

        if (gs.BorderNum < BORDER_BAR || pp - Player != screenpeek)
            return;

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

    void DisplayKeys(PLAYERp pp)
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
                    x = PANEL_KEYS_BOX_X + PANEL_KEYS_XOFF + (row * xsize);
                    y = PANEL_BOX_Y + PANEL_KEYS_YOFF + (col * ysize);
                    DrawGraphic(tileGetTexture(StatusKeyPics[i]), x, y, DI_ITEM_LEFT_TOP, 1, -1, -1, 1, 1);
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
                    x = PANEL_KEYS_BOX_X + PANEL_KEYS_XOFF + (row * xsize);
                    y = PANEL_BOX_Y + PANEL_KEYS_YOFF + (col * ysize);
                    DrawGraphic(tileGetTexture(StatusKeyPics[i + 4]), x, y, DI_ITEM_LEFT_TOP, 1, -1, -1, 1, 1);
                }
                i++;
            }
        }
    }



    void DrawStatusBar()
    {
        auto pp = Player + screenpeek;
        USERp u = User[pp->PlayerSprite];
        BeginStatusBar(320, 200, tileHeight(STATUS_BAR));

        DrawGraphic(tileGetTexture(STATUS_BAR), 0, 200, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, 1, 1);
        DisplayPanelNumber(PANEL_HEALTH_BOX_X + PANEL_HEALTH_XOFF, PANEL_BOX_Y + PANEL_HEALTH_YOFF, u->Health);
        DisplayPanelNumber(PANEL_ARMOR_BOX_X + PANEL_ARMOR_XOFF, PANEL_BOX_Y + PANEL_ARMOR_YOFF, pp->Armor);
        DisplayPanelNumber(PANEL_AMMO_BOX_X + PANEL_AMMO_XOFF, PANEL_BOX_Y + PANEL_AMMO_YOFF, pp->WpnAmmo[u->WeaponNum]);
        PlayerUpdateWeaponSummaryAll(pp);
        if (gNet.MultiGameType != MULTI_GAME_COMMBAT)
            DisplayKeys(pp);
        else if (gNet.TimeLimit)
            DisplayTimeLimit(pp);

    }


    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------
public:
    void UpdateStatusBar(ClockTicks arg)
    {
        int nPalette = 0;

        if (gs.BorderNum <= BORDER_NONE) return;

        /*if (gs.BorderNum == BORDER_HUD)
        {
            DrawHUD2();
        }
        else*/ if (gs.BorderNum == BORDER_MINI_BAR)
        {
            //DrawHUD1(nPalette);
        }
        else
        {
            DrawStatusBar();
        }
    }

};


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

void UpdateStatusBar(ClockTicks arg)
{
    DSWStatusBar sbar;

    if (gs.BorderNum >= BORDER_BAR)
    {
        UpdateFrame();
    }

    sbar.UpdateStatusBar(arg);
}



END_SW_NS
