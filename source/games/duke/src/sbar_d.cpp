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
#include "dukeactor.h"

bool PickTexture(FGameTexture* tex, int paletteid, TexturePick& pick);

BEGIN_DUKE_NS

//==========================================================================
//
// very much a dummy to access the methods.
// The goal is to export this to a script.
//
//==========================================================================

class DDukeStatusBar : public DDukeCommonStatusBar
{
	DECLARE_CLASS(DDukeStatusBar, DDukeCommonStatusBar)

	int fontheight[2];
public:

	DDukeStatusBar()
	{
		numberFont = Create<DHUDFont>( BigFont, 0, Off, 1, 1 );
		indexFont = Create<DHUDFont>(IndexFont, 4, CellRight, 1, 1 );
		miniFont = Create<DHUDFont>(SmallFont2, 0, Off, 1, 1 );
		digiFont = Create<DHUDFont>(DigiFont, 1, Off, 1, 1 );

		// optionally draw at the top of the screen.
		SetSize(tileHeight(TILE_BOTTOMSTATUSBAR));
		scale = 1;

		ammo_sprites = { -1, AMMO, SHOTGUNAMMO, BATTERYAMMO, RPGAMMO, HBOMBAMMO, CRYSTALAMMO, DEVISTATORAMMO, TRIPBOMBSPRITE, FREEZEAMMO + 1, HBOMBAMMO, GROWAMMO, FLAMETHROWERAMMO + 1 };
		item_icons = { 0, FIRSTAID_ICON, STEROIDS_ICON, HOLODUKE_ICON, JETPACK_ICON, HEAT_ICON, AIRTANK_ICON, BOOT_ICON };

		fontheight[1] = fontheight[0] = 0;
		for (int i = 0; i < 9; i++)
		{
			auto zerotex = tileGetTexture(BIGALPHANUM - 10 + i);
			if (zerotex)
			{
				int fh0 = zerotex->GetTexture()->CheckRealHeight();
				int fh1 = fh0;
				TexturePick pick;
				if (PickTexture(zerotex, TRANSLATION(Translation_Remap, 0), pick))
				{
					int oheight = zerotex->GetTexelHeight();
					int dheight = pick.texture->GetTexelHeight();
					int dReal = pick.texture->CheckRealHeight();
					fh1 = Scale(dReal, oheight, dheight);
				}
				if (fh0 > fontheight[0]) fontheight[0] = fh0;
				if (fh1 > fontheight[1]) fontheight[1] = fh1;
			}
		}
	}

	//==========================================================================
	//
	// Helpers
	//
	//==========================================================================

	int getinvamount(const struct player_struct* p)
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

	int GetMoraleOrShield(struct player_struct *p, int snum)
	{
		// special handling for WW2GI
		int lAmount = GetGameVar("PLR_MORALE", -1, p->GetActor(), snum);
		if (lAmount == -1) lAmount = p->shield_amount;
		return lAmount;
	}


	//==========================================================================
	//
	// Fullscreen HUD variant #1
	//
	//==========================================================================

	void FullscreenHUD1(struct player_struct* p, int snum)
	{
		int fh = fontheight[hw_hightile ? 1 : 0];
		FString format;
		FGameTexture* img;
		double imgScale;
		double baseScale = (scale * (fh+1));
		double texty = -fh - 2.5;

		//
		// Health
		//
		img = tileGetTexture(isNamWW2GI()? FIRSTAID_ICON : COLA);
		imgScale = baseScale / img->GetDisplayHeight();
		DrawGraphic(img, 2, -1.5, DI_ITEM_LEFT_BOTTOM, 1., -1, -1, imgScale, imgScale);

		if (!althud_flashing || p->last_extra > (gs.max_player_health >> 2) || (PlayClock & 32) || (p->GetActor()->s.pal == 1 && p->last_extra < 2))
		{
			int s = -8;
			if (althud_flashing && p->last_extra > gs.max_player_health)
				s += bsin(I_GetBuildTime() << 5) / 768;
			int intens = clamp(255 - 6 * s, 0, 255);
			format.Format("%d", p->last_extra);
			SBar_DrawString(this, numberFont, format, 25, texty, DI_TEXT_ALIGN_LEFT, CR_UNTRANSLATED, intens / 255., 0, 0, 1, 1);
		}

		//
		// Armor
		//
		img = tileGetTexture(SHIELD);
		imgScale = baseScale / img->GetDisplayHeight();
		DrawGraphic(img, 67.375, -1.5, DI_ITEM_LEFT_BOTTOM, 1., -1, -1, imgScale, imgScale);

		format.Format("%d", GetMoraleOrShield(p, snum));
		SBar_DrawString(this, numberFont, format, 85, texty, DI_TEXT_ALIGN_LEFT, CR_UNTRANSLATED, 1, 0, 0, 1, 1);

		//
		// Weapon
		//
		int weapon = p->curr_weapon;
		if (weapon == HANDREMOTE_WEAPON) weapon = HANDBOMB_WEAPON;

		int wicon = ammo_sprites[weapon];
		if (wicon > 0)
		{
			int ammo = p->ammo_amount[weapon];
			if (weapon != PISTOL_WEAPON || (weapon == PISTOL_WEAPON && !cl_showmagamt))
			{
				format.Format("%d", ammo);
			}
			else
			{
				short clip = CalcMagazineAmount(ammo, isNam() ? 20 : 12, p->kickback_pic >= 1);
				format.Format("%d/%d", clip, ammo - clip);
			}
			img = tileGetTexture(wicon);
			imgScale = baseScale / img->GetDisplayHeight();
			auto imgX = 20.;
			auto strlen = format.Len();

			if (strlen > 1)
			{
				imgX += (imgX * 0.6) * (strlen - 1);
			}

			if (weapon != KNEE_WEAPON && (!althud_flashing || PlayClock & 32 || ammo > (gs.max_ammo_amount[weapon] / 10)))
			{
				SBar_DrawString(this, numberFont, format, -3, texty, DI_TEXT_ALIGN_RIGHT, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
			}

			DrawGraphic(img, -imgX, -1.5, DI_ITEM_RIGHT_BOTTOM, 1, -1, -1, imgScale, imgScale);
		}

		//
		// Selected inventory item
		//
		unsigned icon = p->inven_icon;
		if (icon > 0)
		{
			int x = 128;

			if (icon < ICON_MAX)
			{
				img = tileGetTexture(item_icons[icon]);
				imgScale = baseScale / img->GetDisplayHeight();
				DrawGraphic(img, x, -1.5, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, imgScale, imgScale);
			}

			int percentv = getinvamount(p);
			format.Format("%3d%%", percentv);
			int color = percentv > 50 ? 11 : percentv > 25 ? 23 : 2;
			SBar_DrawString(this, miniFont, format, x + 36.5, -indexFont->mFont->GetHeight() + 0.5, DI_TEXT_ALIGN_RIGHT, color, 1, 0, 0, 1, 1, TRANSLATION(Translation_Remap, color));

			auto text = ontext(p);
			if (text.first) SBar_DrawString(this, miniFont, text.first, x + 36.5, -miniFont->mFont->GetHeight() - 9.5, DI_TEXT_ALIGN_RIGHT, 
				CR_UNDEFINED, 1, 0, 0, 1, 1, TRANSLATION(Translation_Remap, text.second));
		}

		//
		// keys
		//
		if (p->got_access & 1) DrawGraphic(tileGetTexture(ACCESSCARD), -12, -23.5, DI_ITEM_RIGHT, 1, -1, -1, 0.5, 0.5, 0xffffffff, TRANSLATION(Translation_Remap, 0));
		if (p->got_access & 4) DrawGraphic(tileGetTexture(ACCESSCARD), -7 , -21.5, DI_ITEM_RIGHT, 1, -1, -1, 0.5, 0.5, 0xffffffff, TRANSLATION(Translation_Remap, 23));
		if (p->got_access & 2) DrawGraphic(tileGetTexture(ACCESSCARD), -2 , -19.5, DI_ITEM_RIGHT, 1, -1, -1, 0.5, 0.5, 0xffffffff, TRANSLATION(Translation_Remap, 21));
	}


	//==========================================================================
	//
	// Fullscreen HUD variant #2
	//
	//==========================================================================

	void FullscreenHUD2(struct player_struct *p)
	{
		//
		// health
		//
		DrawGraphic(tileGetTexture(HEALTHBOX), 5, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);
		int health = (p->GetActor()->s.pal == 1 && p->last_extra < 2) ? 1 : p->last_extra;
		FStringf format("%d", health);
		SBar_DrawString(this, digiFont, format, 20, -digiFont->mFont->GetHeight() * scale - 3, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

		//
		// ammo
		//
		DrawGraphic(tileGetTexture(AMMOBOX), 37, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);
		int wp = (p->curr_weapon == HANDREMOTE_WEAPON) ? HANDBOMB_WEAPON : p->curr_weapon;
		format.Format("%d", p->ammo_amount[wp]);
		SBar_DrawString(this, digiFont, format, 52, -digiFont->mFont->GetHeight() * scale - 3, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, scale, scale);

		//
		// inventory
		//
		unsigned icon = p->inven_icon;
		if (icon > 0)
		{
			int x = 73;
			DrawGraphic(tileGetTexture(INVENTORYBOX), 69, -2, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, scale, scale);
			if (icon < ICON_MAX)
				DrawGraphic(tileGetTexture(item_icons[icon]), x, -13.5, DI_ITEM_LEFT|DI_ITEM_VCENTER, 1, -1, -1, scale, scale);

			int percentv = getinvamount(p);
			format.Format("%3d%%", percentv);
			int color = percentv > 50 ? 11 : percentv > 25 ? 23 : 2;
			SBar_DrawString(this, miniFont, format, x + 34, -indexFont->mFont->GetHeight() - 3, DI_TEXT_ALIGN_RIGHT, color, 1, 0, 0, 1, 1, TRANSLATION(Translation_Remap, color));

			auto text = ontext(p);
			if (text.first) SBar_DrawString(this, miniFont, text.first, x + 34, -miniFont->mFont->GetHeight() - 14, DI_TEXT_ALIGN_RIGHT, CR_UNDEFINED, 1, 0, 0, 1, 1, TRANSLATION(Translation_Remap, text.second));
		}
	}

	//==========================================================================
	//
	// Fullscreen HUD drawer
	//
	//==========================================================================

	void DrawHud(int snum, int style)
	{
		auto p = &ps[snum];
		BeginHUD(320, 200, 1.f);
		if (style == 1)
		{
			DrawInventory(p, 0, -46, DI_SCREEN_CENTER_BOTTOM);
			FullscreenHUD1(p, snum);
			PrintLevelStats(tileHeight(BIGALPHANUM) +10);
		}
		else if (style == 2)
		{
			DrawInventory(p, (ud.multimode > 1) ? 56 : 65, -28, DI_SCREEN_CENTER_BOTTOM);
			FullscreenHUD2(p);
			PrintLevelStats(tileHeight(HEALTHBOX) + 4);
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

	void DrawWeaponNum(int index, double x, double y, int num1, int num2, int shade, int numdigits)
	{
		/*
		if (isShareware() && (ind > HANDBOMB_WEAPON || ind < 0))
		{
			minitextshade(x + 1, y - 4, "ORDER", 20, 11, 2 + 8 + 16 + ROTATESPRITE_MAX);
			return;
		}
		*/
		FString format;
		bool parsedDivisor = false;

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
				DrawGraphic(tileGetTexture(THREEBYFIVE + c), x + 4 * i + (parsedDivisor ? 1 : 0), y, DI_ITEM_LEFT | DI_ITEM_VCENTER, 1, 0, 0, 1, 1, pe);
			}
			if (format[i] == '/')
			{
				parsedDivisor = true;
			}
		}
	}

	//==========================================================================
	//
	// Weapon display (Duke only)
	//
	//==========================================================================

	void DrawWeaponAmounts(const struct player_struct* p, double x, double y)
	{
		int cw = p->curr_weapon;

		auto ShadeForWeapon = [=](int weapon, int optweapon = -1)
		{
			// Headache-inducing math at play here.
			return (((!p->ammo_amount[weapon]) | (!p->gotweapon[weapon])) * 9) + 12 - 18 * ((cw == weapon) || (optweapon != -1 && cw == optweapon));
		};

		DrawWeaponNum(2, x, y, p->ammo_amount[PISTOL_WEAPON], gs.max_ammo_amount[PISTOL_WEAPON], 12 - 20 * (cw == PISTOL_WEAPON), 3);
		DrawWeaponNum(3, x, y + 6, p->ammo_amount[SHOTGUN_WEAPON], gs.max_ammo_amount[SHOTGUN_WEAPON], ShadeForWeapon(SHOTGUN_WEAPON), 3);
		DrawWeaponNum(4, x, y + 12, p->ammo_amount[CHAINGUN_WEAPON], gs.max_ammo_amount[CHAINGUN_WEAPON], ShadeForWeapon(CHAINGUN_WEAPON), 3);
		DrawWeaponNum(5, x + 39, y, p->ammo_amount[RPG_WEAPON], gs.max_ammo_amount[RPG_WEAPON], ShadeForWeapon(RPG_WEAPON), 2);
		DrawWeaponNum(6, x + 39, y + 6, p->ammo_amount[HANDBOMB_WEAPON], gs.max_ammo_amount[HANDBOMB_WEAPON], ShadeForWeapon(HANDBOMB_WEAPON, HANDREMOTE_WEAPON), 2);
		if (p->subweapon & (1 << GROW_WEAPON)) // original code says: if(!p->ammo_amount[SHRINKER_WEAPON] || cw == GROW_WEAPON)
			DrawWeaponNum(7, x + 39, y + 12, p->ammo_amount[GROW_WEAPON], gs.max_ammo_amount[GROW_WEAPON], ShadeForWeapon(GROW_WEAPON), 2);
		else
			DrawWeaponNum(7, x + 39, y + 12, p->ammo_amount[SHRINKER_WEAPON], gs.max_ammo_amount[SHRINKER_WEAPON], ShadeForWeapon(SHRINKER_WEAPON), 2);
		DrawWeaponNum(8, x + 70, y, p->ammo_amount[DEVISTATOR_WEAPON], gs.max_ammo_amount[DEVISTATOR_WEAPON], ShadeForWeapon(DEVISTATOR_WEAPON), 2);
		DrawWeaponNum(9, x + 70, y + 6, p->ammo_amount[TRIPBOMB_WEAPON], gs.max_ammo_amount[TRIPBOMB_WEAPON], ShadeForWeapon(TRIPBOMB_WEAPON), 2);
		DrawWeaponNum(0, x + 70, y + 12, p->ammo_amount[FREEZE_WEAPON], gs.max_ammo_amount[FREEZE_WEAPON], ShadeForWeapon(FREEZE_WEAPON), 2);
	}

	//==========================================================================
	//
	// Status bar drawer
	//
	//==========================================================================

	void Statusbar(int snum)
	{
		auto p = &ps[snum];
		int h = tileHeight(BOTTOMSTATUSBAR);
		int top = 200 - h;
		int left = (320 - tileWidth(BOTTOMSTATUSBAR)) / 2;
		BeginStatusBar(320, 200, h);
		DrawInventory(p, 160, 154, 0);
		if (hud_size == Hud_StbarOverlay) Set43ClipRect();
		DrawGraphic(tileGetTexture(BOTTOMSTATUSBAR), left, top, DI_ITEM_LEFT_TOP, 1, -1, -1, 1, 1);
		twod->ClearClipRect();

		FString format;

		if (ud.multimode > 1 && !ud.coop)
		{
			DrawGraphic(tileGetTexture(KILLSICON), 228, top + 8, DI_ITEM_OFFSETS, 1, 0, 0, 1, 1);
			format.Format("%d", max(p->frag - p->fraggedself, 0));
			SBar_DrawString(this, digiFont, format, 287, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
		}
		else
		{
			auto key = tileGetTexture(ACCESS_ICON);
			if (p->got_access & 4) DrawGraphic(key, 275.5, top + 16, DI_ITEM_OFFSETS, 1, -1, -1, 1, 1, 0xffffffff, TRANSLATION(Translation_Remap, 23));
			if (p->got_access & 2) DrawGraphic(key, 288.5, top + 16, DI_ITEM_OFFSETS, 1, -1, -1, 1, 1, 0xffffffff, TRANSLATION(Translation_Remap, 21));
			if (p->got_access & 1) DrawGraphic(key, 282, top + 23, DI_ITEM_OFFSETS, 1, -1, -1, 1, 1, 0xffffffff, TRANSLATION(Translation_Remap, 0));
		}
		DrawWeaponAmounts(p, 96, top + 15.5);

		int num = (p->GetActor()->s.pal == 1 && p->last_extra < 2) ? 1 : p->last_extra;
		format.Format("%d", num);
		SBar_DrawString(this, digiFont, format, 31, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
		format.Format("%d", GetMoraleOrShield(p, snum));
		SBar_DrawString(this, digiFont, format, 63, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);

		if (p->curr_weapon != KNEE_WEAPON)
		{
			int wep = (p->curr_weapon == HANDREMOTE_WEAPON)? HANDBOMB_WEAPON : p->curr_weapon;
			format.Format("%d", p->ammo_amount[wep]);
			SBar_DrawString(this, digiFont, format, 207, top + 17, DI_TEXT_ALIGN_CENTER, CR_UNTRANSLATED, 1, 0, 0, 1, 1);
		}

		int icon = p->inven_icon;
		if (icon)
		{
			int x = 232;
			if (icon < ICON_MAX)
				DrawGraphic(tileGetTexture(item_icons[icon]), x, top + 20.5, DI_ITEM_LEFT | DI_ITEM_VCENTER, 1, -1, -1, 1, 1);

			int percentv = getinvamount(p);
			format.Format("%3d%%", percentv);
			int color = percentv > 50 ? 11 : percentv > 25 ? 23 : 2;
			SBar_DrawString(this, miniFont, format, x + 34, top + 24, DI_TEXT_ALIGN_RIGHT, CR_UNDEFINED, 1, 0, 0, 1, 1, TRANSLATION(Translation_Remap, color));

			auto text = ontext(p);
			if (text.first) SBar_DrawString(this, miniFont, text.first, x + 34, top + 14, DI_TEXT_ALIGN_RIGHT, CR_UNDEFINED, 1, 0, 0, 1, 1, TRANSLATION(Translation_Remap, text.second));
		}
		PrintLevelStats(-1);
	}

	void UpdateStatusBar()
	{
		if (hud_size >= Hud_Mini)
		{
			DrawHud(screenpeek, hud_size == Hud_Nothing ? 0 : hud_size == Hud_full ? 1 : 2);
		}
		else
		{
			Statusbar(screenpeek);
		}
	}
};

IMPLEMENT_CLASS(DDukeStatusBar, false, false)

DBaseStatusBar* CreateDukeStatusBar()
{
	return Create<DDukeStatusBar>();
}

END_DUKE_NS
