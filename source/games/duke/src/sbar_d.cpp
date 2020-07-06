//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
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

EDuke enhancements integrated: 04/13/2003 - Matt Saettler

Note: EDuke source was in transition.  Changes are in-progress in the
source as it is released.

*/
//------------------------------------------------------------------------- 
#include "ns.h"	// Must come before everything else!

#include <array>
#include "v_font.h"
#include "duke3d.h"
#include "compat.h"
#include "sbar.h"
#include "statusbar.h"
#include "v_draw.h"
#include "names_d.h"
#include "texturemanager.h"

BEGIN_DUKE_NS

//==========================================================================
//
// very much a dummy to access the methods.
// The goal is to export this to a script.
//
//==========================================================================

class DDukeStatusBar : public DDukeCommonStatusBar
{
public:
    DDukeStatusBar()
    {
        // optionally draw at the top of the screen.
        SetSize(tilesiz[TILE_BOTTOMSTATUSBAR].y);
        scale = 1;

        ammo_sprites = { -1, AMMO, SHOTGUNAMMO, BATTERYAMMO, RPGAMMO, HBOMBAMMO, CRYSTALAMMO, DEVISTATORAMMO, TRIPBOMBSPRITE, FREEZEAMMO + 1, HBOMBAMMO, GROWAMMO/*, FLAMETHROWERAMMO + 1*/ };
		item_icons = { 0, FIRSTAID_ICON, STEROIDS_ICON, HOLODUKE_ICON, JETPACK_ICON, HEAT_ICON, AIRTANK_ICON, BOOT_ICON };
    }

	//==========================================================================
	//
	// Helpers
	//
	//==========================================================================

	int getinvamount(const DukePlayer_t* p)
	{
		switch (p->inven_icon)
		{
		case ICON_FIRSTAID:
			return p->firstaid_amount;
		case ICON_STEROIDS:
			return (p->steroids_amount + 3) >> 2;
		case ICON_HOLODUKE:
			return (p->holoduke_amount + 15) / 24;
		case ICON_JETPACK:
			return (p->jetpack_amount + 15) >> 4;
		case ICON_HEATS:
			return p->heat_amount / 12;
		case ICON_SCUBA:
			return (p->scuba_amount + 63) >> 6;
		case ICON_BOOTS:
			return p->boot_amount >> 1;
		}

		return -1;
	}

	int GetMoraleOrShield(DukePlayer_t *p, int snum)
	{
		// special handling for WW2GI
		int lAmount = GetGameVar("PLR_MORALE", -1, p->i, snum);
		if (lAmount == -1) lAmount = p->shield_amount;
		return lAmount;
	}


    //==========================================================================
    //
    // Fullscreen HUD variant #1
    //
    //==========================================================================

    void FullscreenHUD1(DukePlayer_t* p, int snum)
    {
        //
        // Health
        //
        DrawGraphic(tileGetTexture(COLA), 2, -2, DI_ITEM_LEFT_BOTTOM, 1., -1, -1, 0.75, 0.75);

        FString format;
        if (!althud_flashing || p->last_extra > (max_player_health >> 2) || ((int)totalclock & 32) || (sprite[p->i].pal == 1 && p->last_extra < 2))
        {
            int s = -8;
            if (althud_flashing && p->last_extra > max_player_health)
                s += (sintable[((int)totalclock << 5) & 2047] >> 10);
            int intens = clamp(255 - 4 * s, 0, 255);
            auto pe = PalEntry(255, intens, intens, intens);
            format.Format("%d", p->last_extra);
            SBar_DrawString(this, &numberFont, format, 40, -BigFont->GetHeight() - 0.5, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
        }

        //
        // Armor
        //
        DrawGraphic(tileGetTexture(SHIELD), 62, -2, DI_ITEM_LEFT_BOTTOM, 1., -1, -1, 0.75, 0.75);

        format.Format("%d", GetMoraleOrShield(p, snum));
        SBar_DrawString(this, &numberFont, format, 105, -numberFont.mFont->GetHeight() - 0.5, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);

        //
        // Weapon
        //

        int wicon = ammo_sprites[p->curr_weapon];
        if (wicon > 0)
        {
            auto img = tileGetTexture(wicon);
            auto scale = img && img->GetDisplayHeight() >= 50 ? 0.25 : 0.5;
            DrawGraphic(img, -57, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);
        }

        int weapon = p->curr_weapon;
        if (weapon == HANDREMOTE_WEAPON) weapon = HANDBOMB_WEAPON;

        if (p->curr_weapon != KNEE_WEAPON && (!althud_flashing || (int)totalclock & 32 || p->ammo_amount[weapon] > (max_ammo_amount[weapon] / 10)))
        {
            format.Format("%d", p->ammo_amount[weapon]);
            SBar_DrawString(this, &numberFont, format, -22, -numberFont.mFont->GetHeight() - 0.5, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
        }

        //
        // Selected inventory item
        //

        unsigned icon = p->inven_icon;
        if (icon > 0)
        {
            int x = 131;

            if (icon < ICON_MAX)
                DrawGraphic(tileGetTexture(item_icons[icon]), x, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, 1, 1);

            int percentv = getinvamount(p);
            format.Format("%3d%%", percentv);
            EColorRange color = percentv > 50 ? CR_GREEN : percentv > 25 ? CR_GOLD : CR_RED;
            SBar_DrawString(this, &indexFont, format, x + 35, -indexFont.mFont->GetHeight() - 0.5, DI_TEXT_ALIGN_RIGHT, color, 1, 0, 0, 1, 1);

            auto text = ontext(p);
            if (text.first) SBar_DrawString(this, &miniFont, text.first, x + 35, -miniFont.mFont->GetHeight() - 9.5, DI_TEXT_ALIGN_RIGHT, text.second, 1, 0, 0, 1, 1);
        }

        //
        // keys
        //
        if (p->got_access & 1) DrawGraphic(tileGetTexture(ACCESSCARD), -29, -30, DI_ITEM_CENTER, 1, -1, -1, 0.5, 0.5, 0xffffffff, TRANSLATION(Translation_Remap, 0));
        if (p->got_access & 4) DrawGraphic(tileGetTexture(ACCESSCARD), -24, -28, DI_ITEM_CENTER, 1, -1, -1, 0.5, 0.5, 0xffffffff, TRANSLATION(Translation_Remap, 23));
        if (p->got_access & 2) DrawGraphic(tileGetTexture(ACCESSCARD), -19, -26, DI_ITEM_CENTER, 1, -1, -1, 0.5, 0.5, 0xffffffff, TRANSLATION(Translation_Remap, 21));
    }


    //==========================================================================
    //
    // Fullscreen HUD variant #2
    //
    //==========================================================================

    void FullscreenHUD2(DukePlayer_t *p)
    {
        //
        // health
        //
        DrawGraphic(tileGetTexture(HEALTHBOX), 5, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);
        int health = (sprite[p->i].pal == 1 && p->last_extra < 2) ? 1 : p->last_extra;
        FStringf format("%d", health);
        SBar_DrawString(this, &digiFont, format, 19, -digiFont.mFont->GetHeight() * scale - 7, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

        //
        // ammo
        //
        DrawGraphic(tileGetTexture(AMMOBOX), 37, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);
        int wp = (p->curr_weapon == HANDREMOTE_WEAPON) ? HANDBOMB_WEAPON : p->curr_weapon;
        format.Format("%d", p->ammo_amount[wp]);
        SBar_DrawString(this, &digiFont, format, 53, -digiFont.mFont->GetHeight() * scale - 7, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

        //
        // inventory
        //
        unsigned icon = p->inven_icon;
        if (icon > 0)
        {
            int x = 73;
            DrawGraphic(tileGetTexture(INVENTORYBOX), 69, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);
            if (icon < ICON_MAX)
                DrawGraphic(tileGetTexture(item_icons[icon]), x, -14, DI_ITEM_LEFT|DI_ITEM_VCENTER, 1, -1, -1, scale, scale);

            int percentv = getinvamount(p);
            format.Format("%3d%%", percentv);
            EColorRange color = percentv > 50 ? CR_GREEN : percentv > 25 ? CR_GOLD : CR_RED;
            SBar_DrawString(this, &indexFont, format, x + 34, -indexFont.mFont->GetHeight() - 5.5, DI_TEXT_ALIGN_RIGHT, color, 1, 0, 0, 1, 1);

            auto text = ontext(p);
            if (text.first) SBar_DrawString(this, &miniFont, text.first, x + 34, -miniFont.mFont->GetHeight() - 14.5, DI_TEXT_ALIGN_RIGHT, text.second, 1, 0, 0, 1, 1);

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
            DrawInventory(p, 0, -46, DI_SCREEN_CENTER_BOTTOM);
            FullscreenHUD1(p, snum);
            PrintLevelStats(tilesiz[BIGALPHANUM].y +10);
        }
        else if (style == 2)
        {
			DrawInventory(p, (ud.multimode > 1) ? 56 : 65, -28, DI_SCREEN_CENTER_BOTTOM);
			FullscreenHUD2(p);
            PrintLevelStats(tilesiz[HEALTHBOX].y + 4);
        }
        else
        {
            DrawInventory(p, 0, -28, DI_SCREEN_CENTER_BOTTOM);
            PrintLevelStats(2);
        }
    }


    //==========================================================================
    //
    // Helper for weapon display
    //
    //==========================================================================

    void DrawWeaponNum(int index, int x, int y, int num1, int num2, int shade, int numdigits)
    {
        /*
        if (VOLUMEONE && (ind > HANDBOMB_WEAPON || ind < 0))
        {
            minitextshade(x + 1, y - 4, "ORDER", 20, 11, 2 + 8 + 16 + ROTATESPRITE_MAX);
            return;
        }
        */
        FString format;

        if (numdigits == 2)
        {
            if (num1 > 99) num1 = 99;
            if (num2 > 99) num2 = 99;
            format.Format("%2d/%d", num1, num2);
        }
        else
        {
            if (num1 > 999) num1 = 999;
            if (num2 > 999) num2 = 999;
            format.Format("%3d/%d", num1, num2);
        }
        y--;
        DrawGraphic(tileGetTexture(THREEBYFIVE + index), x - 7, y, DI_ITEM_LEFT|DI_ITEM_VCENTER, 1, 0, 0, 1, 1, LightForShade(shade - 10), TRANSLATION(Translation_Remap, 7));
        auto pe = LightForShade(shade);
        DrawGraphic(tileGetTexture(THREEBYFIVE + 10), x - 3, y, DI_ITEM_LEFT | DI_ITEM_VCENTER, 1, 0, 0, 1, 1, pe);
        for (size_t i = 0; i < format.Len(); i++) 
        {
            if (format[i] != ' ')
            {
                char c = format[i] == '/' ? 11 : format[i] - '0';
                DrawGraphic(tileGetTexture(THREEBYFIVE + c), x + 4 * i, y, DI_ITEM_LEFT | DI_ITEM_VCENTER, 1, 0, 0, 1, 1, pe);
            }
        }
    }

    //==========================================================================
    //
    // Weapon display (Duke only)
    //
    //==========================================================================

    void DrawWeaponAmounts(const DukePlayer_t* p, int x, int y)
    {
        int cw = p->curr_weapon;

        auto ShadeForWeapon = [=](int weapon, int optweapon = -1)
        {
            // Headache-inducing math at play here.
            return (((!p->ammo_amount[weapon]) | (!p->gotweapon[weapon])) * 9) + 12 - 18 * ((cw == weapon) || (optweapon != -1 && cw == optweapon));
        };

        DrawWeaponNum(2, x, y, p->ammo_amount[PISTOL_WEAPON], max_ammo_amount[PISTOL_WEAPON], 12 - 20 * (cw == PISTOL_WEAPON), 3);
        DrawWeaponNum(3, x, y + 6, p->ammo_amount[SHOTGUN_WEAPON], max_ammo_amount[SHOTGUN_WEAPON], ShadeForWeapon(SHOTGUN_WEAPON), 3);
        DrawWeaponNum(4, x, y + 12, p->ammo_amount[CHAINGUN_WEAPON], max_ammo_amount[CHAINGUN_WEAPON], ShadeForWeapon(CHAINGUN_WEAPON), 3);
        DrawWeaponNum(5, x + 39, y, p->ammo_amount[RPG_WEAPON], max_ammo_amount[RPG_WEAPON], ShadeForWeapon(RPG_WEAPON), 2);
        DrawWeaponNum(6, x + 39, y + 6, p->ammo_amount[HANDBOMB_WEAPON], max_ammo_amount[HANDBOMB_WEAPON], ShadeForWeapon(HANDBOMB_WEAPON, HANDREMOTE_WEAPON), 2);
        if (p->subweapon & (1 << GROW_WEAPON)) // original code says: if(!p->ammo_amount[SHRINKER_WEAPON] || cw == GROW_WEAPON)
            DrawWeaponNum(7, x + 39, y + 12, p->ammo_amount[GROW_WEAPON], max_ammo_amount[GROW_WEAPON], ShadeForWeapon(GROW_WEAPON), 2);
        else
            DrawWeaponNum(7, x + 39, y + 12, p->ammo_amount[SHRINKER_WEAPON], max_ammo_amount[SHRINKER_WEAPON], ShadeForWeapon(SHRINKER_WEAPON), 2);
        DrawWeaponNum(8, x + 70, y, p->ammo_amount[DEVISTATOR_WEAPON], max_ammo_amount[DEVISTATOR_WEAPON], ShadeForWeapon(DEVISTATOR_WEAPON), 2);
        DrawWeaponNum(9, x + 70, y + 6, p->ammo_amount[TRIPBOMB_WEAPON], max_ammo_amount[TRIPBOMB_WEAPON], ShadeForWeapon(TRIPBOMB_WEAPON), 2);
        DrawWeaponNum(0, x + 70, y + 12, p->ammo_amount[FREEZE_WEAPON], max_ammo_amount[FREEZE_WEAPON], ShadeForWeapon(FREEZE_WEAPON), 2);
    }

    //==========================================================================
    //
    // Status bar drawer
    //
    //==========================================================================

    void Statusbar(int snum)
    {
        auto p = g_player[snum].ps;
        int h = tilesiz[TILE_BOTTOMSTATUSBAR].y;
        int top = 200 - h;
        BeginStatusBar(320, 200, h, true);
        DrawInventory(p, 160, 154, 0);
        DrawGraphic(tileGetTexture(TILE_BOTTOMSTATUSBAR), 0, top, DI_ITEM_LEFT_TOP, 1, -1, -1, 1, 1);

        FString format;

        if (ud.multimode > 1 && !ud.coop)
        {
            DrawGraphic(tileGetTexture(KILLSICON), 228, top + 8, DI_ITEM_OFFSETS, 1, 0, 0, 1, 1);
            format.Format("%d", max(p->frag - p->fraggedself, 0));
            SBar_DrawString(this, &digiFont, format, 287, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
        }
        else
        {
            auto key = tileGetTexture(ACCESS_ICON);
            if (p->got_access & 4) DrawGraphic(key, 275, top + 16, DI_ITEM_OFFSETS, 1, -1, -1, 1, 1, 0xffffffff, TRANSLATION(Translation_Remap, 23));
            if (p->got_access & 2) DrawGraphic(key, 288, top + 16, DI_ITEM_OFFSETS, 1, -1, -1, 1, 1, 0xffffffff, TRANSLATION(Translation_Remap, 21));
            if (p->got_access & 1) DrawGraphic(key, 281, top + 23, DI_ITEM_OFFSETS, 1, -1, -1, 1, 1, 0xffffffff, TRANSLATION(Translation_Remap, 0));
        }
        DrawWeaponAmounts(p, 96, top + 16);

        int num = (sprite[p->i].pal == 1 && p->last_extra < 2) ? 1 : p->last_extra;
        format.Format("%d", num);
        SBar_DrawString(this, &digiFont, format, 32, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
        format.Format("%d", GetMoraleOrShield(p, snum));
        SBar_DrawString(this, &digiFont, format, 64, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);

        if (p->curr_weapon != KNEE_WEAPON)
        {
            int wep = (p->curr_weapon == HANDREMOTE_WEAPON)? HANDBOMB_WEAPON : p->curr_weapon;
            format.Format("%d", p->ammo_amount[wep]);
            SBar_DrawString(this, &digiFont, format, 208, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
        }

        int icon = p->inven_icon;
        if (icon)
        {
            int x = 231;
            if (icon < ICON_MAX)
                DrawGraphic(tileGetTexture(item_icons[icon]), x, top + 20, DI_ITEM_LEFT | DI_ITEM_VCENTER, 1, -1, -1, 1, 1);

            int percentv = getinvamount(p);
            format.Format("%3d%%", percentv);
            EColorRange color = percentv > 50 ? CR_GREEN : percentv > 25 ? CR_GOLD : CR_RED;
            SBar_DrawString(this, &indexFont, format, x + 34, top + 24, DI_TEXT_ALIGN_RIGHT, color, 1, 0, 0, 1, 1);

            auto text = ontext(p);
            if (text.first) SBar_DrawString(this, &miniFont, text.first, x + 34, top + 14, DI_TEXT_ALIGN_RIGHT, text.second, 1, 0, 0, 1, 1);
        }
        PrintLevelStats(-1);
    }


};

void PrintLevelName_d(double alpha);

void drawstatusbar_d(int snum)
{
	DDukeStatusBar dsb;
	if (ud.screen_size <= 4)
	{
		dsb.DrawHud(snum, ud.screen_size < 4 ? 0 : ud.althud ? 1 : 2);
	}
	else
	{
		dsb.Statusbar(snum);
    }

    if (ud.show_level_text && hud_showmapname && levelTextTime > 1 && !M_Active())
    {
        double alpha;
        if (levelTextTime > 16) alpha = 1.;
        else alpha = (levelTextTime) / 16.;
        PrintLevelName_d(alpha);
    }


}

END_DUKE_NS
