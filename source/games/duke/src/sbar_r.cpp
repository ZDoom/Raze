//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2020 - Christoph Oelckers 

This file is part of Enhanced Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms

*/
//------------------------------------------------------------------------- 
#include "ns.h"	// Must come before everything else!

#include "v_font.h"
#include "duke3d.h"
#include "compat.h"
#include "sbar.h"
#include "v_draw.h"
#include "names_r.h"
#include "texturemanager.h"

BEGIN_DUKE_NS

//==========================================================================
//
// very much a dummy just to access the methods.
// The goal is to export this to a script.
//
//==========================================================================

class DRedneckStatusBar : public DDukeCommonStatusBar
{

public:
    DRedneckStatusBar()
    {
        // optionally draw at the top of the screen.
        SetSize(tilesiz[BOTTOMSTATUSBAR].y);
        scale = 0.5;
        ammo_sprites = { -1, AMMO, SHOTGUNAMMO, BATTERYAMMO, HBOMBAMMO, HBOMBAMMO, RRTILE43, DEVISTATORAMMO, TRIPBOMBSPRITE, GROWSPRITEICON, HBOMBAMMO, -1, BOWLINGBALLSPRITE, MOTOAMMO, BOATAMMO, -1, RPG2SPRITE };
		item_icons = { 0, FIRSTAID_ICON, STEROIDS_ICON, HOLODUKE_ICON, JETPACK_ICON, HEAT_ICON, AIRTANK_ICON, BOOT_ICON };
    }


	int getinvamount(const DukePlayer_t* p)
	{
		switch (p->inven_icon)
		{
		case ICON_FIRSTAID:
			return p->inv_amount[GET_FIRSTAID];
		case ICON_STEROIDS:
			return (p->inv_amount[GET_STEROIDS] + 3) >> 2;
		case ICON_HOLODUKE:
			return p->inv_amount[GET_HOLODUKE] / 400;
		case ICON_JETPACK:
			return p->inv_amount[GET_JETPACK] / 100;
		case ICON_HEATS:
			return p->inv_amount[GET_HEATS] / 12;
		case ICON_SCUBA:
			return (p->inv_amount[GET_SCUBA] + 63) >> 6;
		case ICON_BOOTS:
			return (p->inv_amount[GET_BOOTS] / 10) >> 1;
		}

		return -1;
	}


    //==========================================================================
    //
    // Fullscreen HUD variant #1 for RR
    //
    //==========================================================================

    void FullscreenHUD1(DukePlayer_t* p, int snum)
    {
        //
        // Health
        //

        DrawGraphic(tileGetTexture(SPINNINGNUKEICON+1), 2, -2, DI_ITEM_LEFT_BOTTOM, 1, 0, 0, 10000. / 65536., 10000. / 65536.);

        FString format;
        if (!althud_flashing || p->last_extra > (max_player_health >> 2) || ((int)totalclock & 32) || (sprite[p->i].pal == 1 && p->last_extra < 2))
        {
            int s = -8;
            if (althud_flashing && p->last_extra > max_player_health)
                s += (sintable[((int)totalclock << 5) & 2047] >> 10);
            int intens = clamp(255 - 4 * s, 0, 255);
            auto pe = PalEntry(255, intens, intens, intens);
            format.Format("%d", p->last_extra);
            SBar_DrawString(this, &numberFont, format, 44, -BigFont->GetHeight() * scale - 0.5, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);
        }

        //
        // drink
        //
        DrawGraphic(tileGetTexture(COLA), 70, -2, DI_ITEM_LEFT_BOTTOM, 1, 0, 0, 10000. / 65536., 10000. / 65536.);
        format.Format("%d", p->drink_amt);
        SBar_DrawString(this, &numberFont, format, 98, -BigFont->GetHeight() * scale - 0.5, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

        //
        // eat
        //
        DrawGraphic(tileGetTexture(JETPACK), 122, -2, DI_ITEM_LEFT_BOTTOM, 1, 0, 0, 20000. / 65536., 20000. / 65536.);
        format.Format("%d", p->eat);
        SBar_DrawString(this, &numberFont, format, 175, -BigFont->GetHeight() * scale - 0.5, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

        //
        // selected weapon
        //
        int wicon = ammo_sprites[p->curr_weapon];
        if (wicon > 0)
        {
            auto img = tileGetTexture(wicon);
            auto myscale = img && img->GetDisplayHeight() >= 50 ? 0.25 : 0.5;
            DrawGraphic(img, -50, -2, DI_ITEM_RIGHT_BOTTOM, 1, -1, -1, myscale, myscale);
        }

        int weapon = p->curr_weapon;
        if (weapon == HANDREMOTE_WEAPON) weapon = DYNAMITE_WEAPON;

        if (p->curr_weapon != KNEE_WEAPON && p->curr_weapon != SLINGBLADE_WEAPON && (!althud_flashing || (int)totalclock & 32 || p->ammo_amount[weapon] > (max_ammo_amount[weapon] / 10)))
        {
            format.Format("%d", p->ammo_amount[weapon]);
            SBar_DrawString(this, &numberFont, format, -20, -numberFont.mFont->GetHeight() * scale - 0.5, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);
        }

        //
        // Selected inventory item
        //

        unsigned icon = p->inven_icon;
        if (icon > 0)
        {
            int x = -122;

            if (icon < ICON_MAX)
                DrawGraphic(tileGetTexture(item_icons[icon]), x, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);

            int percentv = getinvamount(p);
            if (icon <= 2) format.Format("%3d%%", percentv);
            else format.Format("%3d ", percentv);
            SBar_DrawString(this, &miniFont, format, x + 35, -miniFont.mFont->GetHeight() * scale - 0.5, DI_TEXT_ALIGN_RIGHT, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

            auto text = ontext(p);
            if (text.first) SBar_DrawString(this, &miniFont, text.first, x + 35, -miniFont.mFont->GetHeight() * scale - 9.5, DI_TEXT_ALIGN_RIGHT, CR_UNTRANSLATED, 1, 0, 0, scale, scale);
        }
        if (p->keys[1]) DrawGraphic(tileGetTexture(ACCESSCARD), -29, -32, DI_ITEM_BOTTOM, 1, -1, -1, 0.5, 0.5, 0xffffffff, TRANSLATION(Translation_Remap, 0));
        if (p->keys[3]) DrawGraphic(tileGetTexture(ACCESSCARD), -24, -30, DI_ITEM_BOTTOM, 1, -1, -1, 0.5, 0.5, 0xffffffff, TRANSLATION(Translation_Remap, 23));
        if (p->keys[2]) DrawGraphic(tileGetTexture(ACCESSCARD), -19, -28, DI_ITEM_BOTTOM, 1, -1, -1, 0.5, 0.5, 0xffffffff, TRANSLATION(Translation_Remap, 21));
    }


    //==========================================================================
    //
    // Fullscreen HUD variant #2 for RR
    //
    //==========================================================================

    void FullscreenHUD2(DukePlayer_t* p)
    {
        //
        // health
        //
        DrawGraphic(tileGetTexture(HEALTHBOX), 2, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);
        int health = (sprite[p->i].pal == 1 && p->last_extra < 2) ? 1 : p->last_extra;
        FStringf format("%d", health);
        SBar_DrawString(this, &digiFont, format, 17, -digiFont.mFont->GetHeight() * scale - 7, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

        //
        // ammo
        //
        DrawGraphic(tileGetTexture(AMMOBOX), 41, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);
        int wp = (p->curr_weapon == HANDREMOTE_WEAPON) ? DYNAMITE_WEAPON : p->curr_weapon;
        format.Format("%d", p->ammo_amount[wp]);
        SBar_DrawString(this, &digiFont, format, 57, -digiFont.mFont->GetHeight() * scale - 7, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

        //
        // inventory
        //
        unsigned icon = p->inven_icon;
        if (icon > 0)
        {
            int x = 81;
            DrawGraphic(tileGetTexture(INVENTORYBOX), 77, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);
            if (icon < ICON_MAX)
                DrawGraphic(tileGetTexture(item_icons[icon]), x, -14, DI_ITEM_LEFT | DI_ITEM_VCENTER, 1, -1, -1, scale, scale);

            int percentv = getinvamount(p);
            if (icon <= 2) format.Format("%3d%%", percentv);
            else format.Format("%3d ", percentv);
            SBar_DrawString(this, &miniFont, format, x + 34, -miniFont.mFont->GetHeight() * scale - 5.5, DI_TEXT_ALIGN_RIGHT, CR_UNTRANSLATED, 1, 0, 0, scale, scale);
        }
    }

    //==========================================================================
    //
    // Fullscreen HUD drawer
    //
    //==========================================================================

    void DrawHud(int snum, int style)
    {
        auto p = g_player[snum].ps;
        BeginHUD(320, 200, 1.f, false);
        if (style == 1)
        {
			double y = -40;
			if (ud.multimode > 1)
				y -= 4;
			if (ud.multimode > 4)
				y -= 4;
			DrawInventory(p, 0, y, DI_SCREEN_CENTER_BOTTOM);
			FullscreenHUD1(p, snum);
            PrintLevelStats(scale * tilesiz[BIGALPHANUM].y + 10);
        }
        else if (style == 2)
        {
            DrawInventory(p, 56, -20, DI_SCREEN_CENTER_BOTTOM);
            FullscreenHUD2(p);
            PrintLevelStats(scale * tilesiz[HEALTHBOX].y + 4);
        }
        else
        {
            DrawInventory(p, 0, -20, DI_SCREEN_CENTER_BOTTOM);
            PrintLevelStats(2);
        }
    }

    //==========================================================================
    //
    // Status bar drawer (RR)
    //
    //==========================================================================

    void DrawWeaponBar(const DukePlayer_t* p, int top)
    {
        double sbscale = 32800. / 65536.;

        DrawGraphic(tileGetTexture(WEAPONBAR), 0, 158, DI_ITEM_OFFSETS, 1, 0, 0, sbscale, sbscale);

        FString format;
        for (int i = 0; i < 9; i++) 
        {
            if ((g_gameType & GAMEFLAG_RRRA) && i == 4 && p->curr_weapon == CHICKEN_WEAPON)
            {
                DrawGraphic(tileGetTexture(AMMO_ICON + 10), 18 + i * 32, top - 6, DI_ITEM_OFFSETS, 1, 0, 0, sbscale, sbscale);
                format.Format("%d", p->ammo_amount[CHICKEN_WEAPON]);
            }
            else
            {
                if (p->gotweapon[i+1]) {
                    DrawGraphic(tileGetTexture(AMMO_ICON + i), 18 + i * 32, top - 6, DI_ITEM_OFFSETS, 1, 0, 0, sbscale, sbscale);
                }
                format.Format("%d", p->ammo_amount[i+1]);
            }
            SBar_DrawString(this, &miniFont, format, 38 + i * 32, 162 - miniFont.mFont->GetHeight() * scale * 0.5, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);
        }
    }


    //==========================================================================
    //
    // Status bar drawer (RR)
    //
    //==========================================================================

    void Statusbar(int snum)
    {
        auto p = g_player[snum].ps;
        double h = tilesiz[BOTTOMSTATUSBAR].y * scale;
        double top = 200 - h;
        BeginStatusBar(320, 200, h, true);
        DrawInventory(p, 160, 154, 0);

        if (ud.screen_size > 8)
            DrawWeaponBar(p, top);

        DrawGraphic(tileGetTexture(BOTTOMSTATUSBAR), 0, top, DI_ITEM_LEFT_TOP, 1, -1, -1, scale, scale);

        FString format;

        if (ud.multimode > 1 && (g_gametypeFlags[ud.coop] & GAMETYPE_FRAGBAR))
        {
            DrawGraphic(tileGetTexture(KILLSICON), 228, top + 8, DI_ITEM_OFFSETS, 1, 0, 0, 1, 1);
            format.Format("%d", max(p->frag - p->fraggedself, 0));
            SBar_DrawString(this, &digiFont, format, 287, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);
        }
        else
        {
            auto key = tileGetTexture(ACCESS_ICON);
            if (p->keys[3]) DrawGraphic(key, 140, top + 16, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale, 0xffffffff, TRANSLATION(Translation_Remap, 23));
            if (p->keys[2]) DrawGraphic(key, 153, top + 16, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale, 0xffffffff, TRANSLATION(Translation_Remap, 21));
            if (p->keys[1]) DrawGraphic(key, 146, top + 23, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale, 0xffffffff, TRANSLATION(Translation_Remap, 0));
        }

        int num = (sprite[p->i].pal == 1 && p->last_extra < 2) ? 1 : p->last_extra;
        format.Format("%d", num);
        SBar_DrawString(this, &digiFont, format, 64, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

        if (p->curr_weapon != KNEE_WEAPON)
        {
            int wep = (p->curr_weapon == HANDREMOTE_WEAPON) ? DYNAMITE_WEAPON : p->curr_weapon;
            format.Format("%d", p->ammo_amount[wep]);
            SBar_DrawString(this, &digiFont, format, 107, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);
        }

        int icon = p->inven_icon;
        if (icon)
        {
            int x = 183;
            if (icon < ICON_MAX)
                DrawGraphic(tileGetTexture(item_icons[icon]), x, top + 20, DI_ITEM_LEFT | DI_ITEM_VCENTER, 1, -1, -1, scale, scale);

            int percentv = getinvamount(p);
            if (icon <= 2) format.Format("%3d%%", percentv);
            else format.Format("%3d ", percentv);
            SBar_DrawString(this, &miniFont, format, x + 34, top + 24, DI_TEXT_ALIGN_RIGHT, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

            if (p->inven_icon == ICON_SCUBA || p->inven_icon == ICON_BOOTS) 
                SBar_DrawString(this, &miniFont, "AUTO", x + 34, top + 14, DI_TEXT_ALIGN_RIGHT, CR_UNTRANSLATED, 1, 0, 0, scale, scale);
        }

        p->drunkang = ((p->drink_amt * 8) + 1647) & 2047;
        if (p->drink_amt >= 100)
        {
            p->drink_amt = 100;
            p->drunkang = 400;
        }
		
		// Todo: These need rotation support which currently does not exist.
        DrawGraphic(tileGetTexture(GUTMETER), 257, top + 24, DI_ITEM_BOTTOM, 1, -1, -1, scale, scale, 0xffffffff, 0 /*, p->drunkang * 360. / 2048 */ );
        DrawGraphic(tileGetTexture(GUTMETER), 293, top + 24, DI_ITEM_BOTTOM, 1, -1, -1, scale, scale, 0xffffffff, 0 /*, p->eatang * 360. / 2048 */);

        if (p->drink_amt >= 0 && p->drink_amt <= 30)
        {
            DrawGraphic(tileGetTexture(GUTMETER_LIGHT1), 239, top + 24, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale);
        }
        else if (p->drink_amt >= 31 && p->drink_amt <= 65)
        {
            DrawGraphic(tileGetTexture(GUTMETER_LIGHT2), 248, top + 24, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale);
        }
        else if (p->drink_amt >= 66 && p->drink_amt <= 87)
        {
            DrawGraphic(tileGetTexture(GUTMETER_LIGHT3), 256, top + 24, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale);
        }
        else
        {
            DrawGraphic(tileGetTexture(GUTMETER_LIGHT4), 265, top + 24, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale);
        }

        if (p->eat >= 0 && p->eat <= 30)
        {
            DrawGraphic(tileGetTexture(GUTMETER_LIGHT1), 276, top + 24, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale);
        }
        else if (p->eat >= 31 && p->eat <= 65)
        {
            DrawGraphic(tileGetTexture(GUTMETER_LIGHT2), 285, top + 24, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale);
        }
        else if (p->eat >= 66 && p->eat <= 87)
        {
            DrawGraphic(tileGetTexture(GUTMETER_LIGHT3), 294, top + 24, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale);
        }
        else
        {
            DrawGraphic(tileGetTexture(GUTMETER_LIGHT4), 302, top + 24, DI_ITEM_OFFSETS, 1, -1, -1, scale, scale);
        }
        PrintLevelStats(-1);
    }
};

void PrintLevelName_r(double alpha);

void drawstatusbar_r(int snum)
{
	DRedneckStatusBar dsb;
	if (ud.screen_size <= 4)
	{
		dsb.DrawHud(snum, ud.screen_size < 4 ? 0 : ud.althud ? 1 : 2);
	}
	else
	{
		dsb.Statusbar(snum);
	}

    if (ud.show_level_text && hud_showmapname && g_levelTextTime > 1 && !M_Active())
    {
        double alpha;
        if (g_levelTextTime > 16) alpha = 1.;
        else alpha = (g_levelTextTime) / 16.;
        PrintLevelName_r(alpha);
    }

}

END_DUKE_NS
