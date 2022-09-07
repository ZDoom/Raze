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


class DukeStatusBar : DukeCommonStatusBar
{
	int fontheight[2];
	TextureID ThreeByFive[12];
	HUDFont indxfont;

	override void Init()
	{
		numberFont = HUDFont.Create(BigFont, 0, Mono_Off, 1, 1 );
		indxfont = HUDFont.Create(Font.FindFont("IndexFont"), 4, Mono_CellRight, 1, 1 );
		miniFont = HUDFont.Create(SmallFont2, 0, Mono_Off, 1, 1 );
		digiFont = HUDFont.Create(Font.FindFont("DigiFont"), 1, Mono_Off, 1, 1 );

		// optionally draw at the top of the screen.
		SetSize(tileHeight("BOTTOMSTATUSBAR"), 320, 200);
		scale = 1;

		ammo_sprites.PushV("", "AMMO", "SHOTGUNAMMO", "BATTERYAMMO", "RPGAMMO", "HBOMBAMMO", "CRYSTALAMMO", "DEVISTATORAMMO", "TRIPBOMBSPRITE", "FREEZEAMMO1", "HBOMBAMMO", "GROWAMMO", "FLAMETHROWERAMMO1" );
		item_icons.PushV("", "FIRSTAID_ICON", "STEROIDS_ICON", "HOLODUKE_ICON", "JETPACK_ICON", "HEAT_ICON", "AIRTANK_ICON", "BOOT_ICON" );

		// get the true size of the font.
		fontheight[1] = fontheight[0] = 0;
		for (int i = 0; i <= 9; i++)
		{
			let name = String.Format("BIGALPHANUM%d", i);
			let zerotex = TexMan.CheckForTexture(name, TexMan.Type_Any);
			if (zerotex.IsValid())
			{
				int fh0 = TexMan.CheckRealHeight(zerotex);
				int fh1 = fh0;
				let hitex = Raze.PickTexture(zerotex);
				if (hitex.IsValid())
				{
					let osize = TexMan.GetScaledSize(zerotex);
					let dsize = TexMan.GetScaledSize(hitex);
					int dReal = TexMan.CheckRealHeight(hitex);
					fh1 = int(dReal * osize.Y / dsize.Y);
				}
				if (fh0 > fontheight[0]) fontheight[0] = fh0;
				if (fh1 > fontheight[1]) fontheight[1] = fh1;
			}
		}
		for (int i = 0; i <= 11; i++)
		{
			let str = String.Format("THREEBYFIVE%d", i);
			ThreeByFive[i] = TexMan.CheckForTexture(str, TexMan.Type_Any);
		}
	}

	//==========================================================================
	//
	// Helpers
	//
	//==========================================================================

	int getinvamount(DukePlayer p)
	{
		switch (p.inven_icon)
		{
		case Duke.ICON_FIRSTAID:
			return p.firstaid_amount;
		case Duke.ICON_STEROIDS:
			return (p.steroids_amount + 3) >> 2;
		case Duke.ICON_HOLODUKE:
			return (p.holoduke_amount + 15) / 24;
		case Duke.ICON_JETPACK:
			return (p.jetpack_amount + 15) >> 4;
		case Duke.ICON_HEATS:
			return p.heat_amount / 12;
		case Duke.ICON_SCUBA:
			return (p.scuba_amount + 63) >> 6;
		case Duke.ICON_BOOTS:
			return p.boot_amount >> 1;
		}

		return -1;
	}

	int GetMoraleOrShield(DukePlayer p)
	{
		// special handling for WW2GI
		int lAmount = p.GetGameVar("PLR_MORALE", -1);
		if (lAmount == -1) lAmount = p.shield_amount;
		return lAmount;
	}


	//==========================================================================
	//
	// Fullscreen HUD variant #1
	//
	//==========================================================================

	void FullscreenHUD1(DukePlayer p)
	{
		int fh = fontheight[hw_hightile ? 1 : 0];
		String format;
		TextureID img;
		double imgScale;
		double baseScale = (scale * (fh+1));
		double texty = -fh - 2.5;

		//
		// Health
		//
		img = TexMan.CheckForTexture(Raze.isNamWW2GI()? "FIRSTAID_ICON" : "COLA");
		let siz = TexMan.GetScaledSize(img);
		imgScale = baseScale / siz.Y;
		DrawTexture(img, (2, -1.5), DI_ITEM_LEFT_BOTTOM, scale:(imgScale, imgScale));

		if (!hud_flashing || p.last_extra > (Duke.MaxPlayerHealth() >> 2) || (PlayClock & 32) || (p.IsFrozen() && p.last_extra < 2))
		{
			int s = -8;
			if (hud_flashing && p.last_extra > Duke.MaxPlayerHealth())
				s += Raze.bsin(Raze.GetBuildTime() << 5) >> 10;
			int intens = clamp(255 - 6 * s, 0, 255);
			format = String.Format("%d", p.last_extra);
			DrawString(numberFont, format, (25, texty), DI_TEXT_ALIGN_LEFT, Font.CR_UNTRANSLATED, intens / 255., 0, 0);
		}

		//
		// Armor
		//
		img = TexMan.CheckForTexture("SHIELD");
		siz = TexMan.GetScaledSize(img);
		imgScale = baseScale / siz.Y;
		DrawTexture(img, (67.375, -1.5), DI_ITEM_LEFT_BOTTOM, scale:(imgScale, imgScale));

		format = String.Format("%d", GetMoraleOrShield(p));
		DrawString(numberFont, format, (85, texty), DI_TEXT_ALIGN_LEFT, Font.CR_UNTRANSLATED, 1, 0, 0);

		//
		// Weapon
		//
		int weapon = p.curr_weapon;
		if (weapon == DukeWpn.HANDREMOTE_WEAPON) weapon = DukeWpn.HANDBOMB_WEAPON;

		let wicon = ammo_sprites[weapon];
		if (wicon.length() > 0)
		{
			int ammo = p.ammo_amount[weapon];
			if (weapon != DukeWpn.PISTOL_WEAPON || (weapon == DukeWpn.PISTOL_WEAPON && !cl_showmagamt))
			{
				format = String.Format("%d", ammo);
			}
			else
			{
				int clip = CalcMagazineAmount(ammo, Raze.isNam() ? 20 : 12, p.kickback_pic >= 1);
				format = String.Format("%d/%d", clip, ammo - clip);
			}
			img = TexMan.CheckForTexture(wicon, TexMan.TYPE_Any);
			siz = TexMan.GetScaledSize(img);
			imgScale = baseScale / siz.Y;
			let imgX = 20.;
			let strlen = format.length();

			if (strlen > 1)
			{
				imgX += (imgX * 0.6) * (strlen - 1);
			}

			if (weapon != DukeWpn.KNEE_WEAPON && (!hud_flashing || PlayClock & 32 || ammo > (Duke.MaxAmmoAmount(weapon) / 10)))
			{
				DrawString(numberFont, format, (-3, texty), DI_TEXT_ALIGN_RIGHT, Font.CR_UNTRANSLATED);
			}
			if (weapon != 7 || !Raze.isNam())
				DrawTexture(img, (-imgX, -1.5), DI_ITEM_RIGHT_BOTTOM, scale:(imgScale, imgScale));
		}

		//
		// Selected inventory item
		//
		uint icon = p.inven_icon;
		if (icon > 0)
		{
			int x = 128;

			if (icon < Duke.ICON_MAX)
			{
				img = TexMan.CheckForTexture(item_icons[icon], TexMan.TYPE_Any);
				siz = TexMan.GetScaledSize(img);
				imgScale = baseScale / siz.Y;
				DrawTexture(img, (x, -1.5), DI_ITEM_LEFT_BOTTOM, scale:(imgScale, imgScale));
			}

			int percentv = getinvamount(p);
			format = String.Format("%3d%%", percentv);
			int color = percentv > 50 ? 11 : percentv > 25 ? 23 : 2;
			DrawString(miniFont, format, (x + 36.5, -indxfont.mFont.GetHeight() + 0.5), DI_TEXT_ALIGN_RIGHT, pt:Translation.MakeID(Translation_Remap, color));

			String text;
			int pal;
			[text, pal] = ontext(p);
			if (text.length() > 0) DrawString(miniFont, text, (x + 36.5, -miniFont.mFont.GetHeight() - 9.5), DI_TEXT_ALIGN_RIGHT, pt:Translation.MakeID(Translation_Remap, pal));
		}

		//
		// keys
		//
		if (p.got_access & 1) DrawImage("ACCESSCARD", (-12, -23.5), DI_ITEM_RIGHT, 1, (-1, -1), (0.5, 0.5), STYLE_Translucent, 0xffffffff, Translation.MakeID(Translation_Remap, 0));
		if (p.got_access & 4) DrawImage("ACCESSCARD", (-7 , -21.5), DI_ITEM_RIGHT, 1, (-1, -1), (0.5, 0.5), STYLE_Translucent, 0xffffffff, Translation.MakeID(Translation_Remap, 23));
		if (p.got_access & 2) DrawImage("ACCESSCARD", (-2 , -19.5), DI_ITEM_RIGHT, 1, (-1, -1), (0.5, 0.5), STYLE_Translucent, 0xffffffff, Translation.MakeID(Translation_Remap, 21));
	}


	//==========================================================================
	//
	// Fullscreen HUD variant #2
	//
	//==========================================================================

	void FullscreenHUD2(DukePlayer p)
	{
		//
		// health
		//
		DrawImage("HEALTHBOX", (5, -2), DI_ITEM_LEFT_BOTTOM, scale:(scale, scale));
		int health = p.IsFrozen() ? 1 : p.last_extra;
		String format = String.Format("%d", health);
		DrawString(digiFont, format, (20, -digiFont.mFont.GetHeight() * scale - 3), DI_TEXT_ALIGN_CENTER, Font.CR_UNTRANSLATED, scale:(scale, scale));

		//
		// ammo
		//
		DrawImage("AMMOBOX", (37, -2), DI_ITEM_LEFT_BOTTOM, scale:(scale, scale));
		int wp = (p.curr_weapon == DukeWpn.HANDREMOTE_WEAPON) ? DukeWpn.HANDBOMB_WEAPON : p.curr_weapon;
		format = String.Format("%d", p.ammo_amount[wp]);
		DrawString(digiFont, format, (52, -digiFont.mFont.GetHeight() * scale - 3), DI_TEXT_ALIGN_CENTER, Font.CR_UNTRANSLATED, scale:(scale, scale));

		//
		// inventory
		//
		uint icon = p.inven_icon;
		if (icon > 0)
		{
			int x = 73;
			DrawImage("INVENTORYBOX", (69, -2), DI_ITEM_LEFT_BOTTOM, scale:(scale, scale));
			if (icon < Duke.ICON_MAX)
				DrawImage(item_icons[icon], (x, -13.5), DI_ITEM_LEFT|DI_ITEM_VCENTER, scale:(scale, scale));

			int percentv = getinvamount(p);
			format = String.Format("%3d%%", percentv);
			int color = percentv > 50 ? 11 : percentv > 25 ? 23 : 2;
			DrawString(miniFont, format, (x + 34, -indxfont.mFont.GetHeight() - 3), DI_TEXT_ALIGN_RIGHT, pt:Translation.MakeID(Translation_Remap, color));

			String text;
			int pal;
			[text, pal] = ontext(p);
			if (text.length() > 0) DrawString(miniFont, text, (x + 34, -miniFont.mFont.GetHeight() - 14), DI_TEXT_ALIGN_RIGHT, pt:Translation.MakeID(Translation_Remap, pal));
		}
	}

	//==========================================================================
	//
	// Fullscreen HUD drawer
	//
	//==========================================================================

	void DrawHud(DukePlayer p, int style, SummaryInfo info)
	{
		BeginHUD(1, false, 320, 200);
		if (style == 1)
		{
			DrawInventory(p, 0, -46, DI_SCREEN_CENTER_BOTTOM);
			FullscreenHUD1(p);
			DoLevelStats(fontheight[hw_hightile ? 1 : 0] + 6, info);
		}
		else if (style == 2)
		{
			DrawInventory(p, netgame ? 56 : 65, -28, DI_SCREEN_CENTER_BOTTOM);
			FullscreenHUD2(p);
			DoLevelStats(tileHeight("HEALTHBOX") + 4, info);
		}
		else
		{
			DrawInventory(p, 0, -28, DI_SCREEN_CENTER_BOTTOM);
			DoLevelStats(2, info);
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
		if (isShareware() && (ind > DukeWpn.HANDBOMB_WEAPON || ind < 0))
		{
			minitextshade(x + 1, y - 4, "ORDER", 20, 11, 2 + 8 + 16 + ROTATESPRITE_MAX);
			return;
		}
		*/
		String format;
		bool parsedDivisor = false;

		if (numdigits == 2)
		{
			if (num1 > 99) num1 = 99;
			if (num2 > 99) num2 = 99;
			format = String.Format("%2d/%d", num1, num2);
		}
		else
		{
			if (num1 > 999) num1 = 999;
			if (num2 > 999) num2 = 999;
			format = String.Format("%3d/%d", num1, num2);
		}
		y--;
		DrawTexture(ThreeByFive[index], (x - 7, y), DI_ITEM_LEFT|DI_ITEM_VCENTER, 1, (-1, -1), (1, 1), STYLE_Translucent, Raze.ShadeToLight(shade - 10), Translation.MakeID(Translation_Remap, 7));
		let pe = Raze.ShadeToLight(shade);
		DrawTexture(ThreeByFive[10], (x - 3, y), DI_ITEM_LEFT | DI_ITEM_VCENTER, 1, (-1, -1), (1, 1), STYLE_Translucent, pe);
		for (uint i = 0; i < format.Length(); i++) 
		{
			int cc = format.ByteAt(i);
			if (cc != " ")
			{
				int c = cc == "/" ? 11 : cc - int("0");
				DrawTexture(ThreeByFive[c], (x + 4 * i + (parsedDivisor ? 1 : 0), y), DI_ITEM_LEFT | DI_ITEM_VCENTER, col:pe);
			}
			if (cc == "/")
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

	int ShadeForWeapon(DukePlayer p, int weapon, int optweapon = -1)
	{
		// Headache-inducing math at play here.
		return (((!p.ammo_amount[weapon]) | (!p.gotweapon[weapon])) * 9) + 12 - 18 * ((p.curr_weapon == weapon) || (optweapon != -1 && p.curr_weapon == optweapon));
	}


	void DrawWeaponAmounts(DukePlayer p, double x, double y)
	{
		int cw = p.curr_weapon;

		DrawWeaponNum(2, x, y, p.ammo_amount[DukeWpn.PISTOL_WEAPON], Duke.MaxAmmoAmount(DukeWpn.PISTOL_WEAPON), 12 - 20 * (cw == DukeWpn.PISTOL_WEAPON), 3);
		DrawWeaponNum(3, x, y + 6, p.ammo_amount[DukeWpn.SHOTGUN_WEAPON], Duke.MaxAmmoAmount(DukeWpn.SHOTGUN_WEAPON), ShadeForWeapon(p, DukeWpn.SHOTGUN_WEAPON), 3);
		DrawWeaponNum(4, x, y + 12, p.ammo_amount[DukeWpn.CHAINGUN_WEAPON], Duke.MaxAmmoAmount(DukeWpn.CHAINGUN_WEAPON), ShadeForWeapon(p, DukeWpn.CHAINGUN_WEAPON), 3);
		DrawWeaponNum(5, x + 39, y, p.ammo_amount[DukeWpn.RPG_WEAPON], Duke.MaxAmmoAmount(DukeWpn.RPG_WEAPON), ShadeForWeapon(p, DukeWpn.RPG_WEAPON), 2);
		DrawWeaponNum(6, x + 39, y + 6, p.ammo_amount[DukeWpn.HANDBOMB_WEAPON], Duke.MaxAmmoAmount(DukeWpn.HANDBOMB_WEAPON), ShadeForWeapon(p, DukeWpn.HANDBOMB_WEAPON, DukeWpn.HANDREMOTE_WEAPON), 2);
		if (p.subweapon & (1 << DukeWpn.GROW_WEAPON)) // original code says: if(!p.ammo_amount[SHRINKER_WEAPON] || cw == GROW_WEAPON)
			DrawWeaponNum(7, x + 39, y + 12, p.ammo_amount[DukeWpn.GROW_WEAPON], Duke.MaxAmmoAmount(DukeWpn.GROW_WEAPON), ShadeForWeapon(p, DukeWpn.GROW_WEAPON), 2);
		else
			DrawWeaponNum(7, x + 39, y + 12, p.ammo_amount[DukeWpn.SHRINKER_WEAPON], Duke.MaxAmmoAmount(DukeWpn.SHRINKER_WEAPON), ShadeForWeapon(p, DukeWpn.SHRINKER_WEAPON), 2);
		DrawWeaponNum(8, x + 70, y, p.ammo_amount[DukeWpn.DEVISTATOR_WEAPON], Duke.MaxAmmoAmount(DukeWpn.DEVISTATOR_WEAPON), ShadeForWeapon(p, DukeWpn.DEVISTATOR_WEAPON), 2);
		DrawWeaponNum(9, x + 70, y + 6, p.ammo_amount[DukeWpn.TRIPBOMB_WEAPON], Duke.MaxAmmoAmount(DukeWpn.TRIPBOMB_WEAPON), ShadeForWeapon(p, DukeWpn.TRIPBOMB_WEAPON), 2);
		DrawWeaponNum(0, x + 70, y + 12, p.ammo_amount[DukeWpn.FREEZE_WEAPON], Duke.MaxAmmoAmount(DukeWpn.FREEZE_WEAPON), ShadeForWeapon(p, DukeWpn.FREEZE_WEAPON), 2);
	}

	//==========================================================================
	//
	// Status bar drawer
	//
	//==========================================================================

	void Statusbar(DukePlayer p)
	{
		let bsb = TexMan.CheckForTexture("BOTTOMSTATUSBAR", Texman.Type_Any);
		let siz = TexMan.GetScaledSize(bsb);
		int h = int(siz.Y);
		int top = 200 - h;
		int left = (320 - int(siz.X)) / 2;
		BeginStatusBar(false, 320, 200, h);
		DrawInventory(p, 160, 154, 0);
		if (hud_size == Hud_StbarOverlay) Set43ClipRect();
		DrawTexture(bsb, (left, top), DI_ITEM_LEFT_TOP, 1);
		screen.ClearClipRect();

		String format;

		/*
		if (ud.multimode > 1 && !ud.coop)
		{
			DrawTexture("KILLSICON", (228, top + 8), DI_ITEM_OFFSETS, 1, 0, 0);
			format = String.Format("%d", max(p.frag - p.fraggedself, 0));
			DrawString(digiFont, format, (287, top + 17), DI_TEXT_ALIGN_CENTER);
		}
		else*/
		{
			let key = TexMan.CheckForTexture("ACCESS_ICON", TexMan.Type_Any);
			if (p.got_access & 4) DrawTexture(key, (275.5, top + 16), DI_ITEM_OFFSETS, style:STYLE_Translucent, translation:Translation.MakeID(Translation_Remap, 23));
			if (p.got_access & 2) DrawTexture(key, (288.5, top + 16), DI_ITEM_OFFSETS, style:STYLE_Translucent, translation:Translation.MakeID(Translation_Remap, 21));
			if (p.got_access & 1) DrawTexture(key, (282, top + 23), DI_ITEM_OFFSETS, style:STYLE_Translucent, translation:Translation.MakeID(Translation_Remap, 0));
		}
		DrawWeaponAmounts(p, 96, top + 15.5);

		int num = (p.IsFrozen()) ? 1 : p.last_extra;
		format = String.Format("%d", num);
		DrawString(digiFont, format, (31, top + 17), DI_TEXT_ALIGN_CENTER);
		format = String.Format("%d", GetMoraleOrShield(p));
		DrawString(digiFont, format, (63, top + 17), DI_TEXT_ALIGN_CENTER);

		if (p.curr_weapon != DukeWpn.KNEE_WEAPON)
		{
			int wep = (p.curr_weapon == DukeWpn.HANDREMOTE_WEAPON)? DukeWpn.HANDBOMB_WEAPON : p.curr_weapon;
			format = String.Format("%d", p.ammo_amount[wep]);
			DrawString(digiFont, format, (207, top + 17), DI_TEXT_ALIGN_CENTER);
		}

		int icon = p.inven_icon;
		if (icon)
		{
			int x = 232;
			if (icon < Duke.ICON_MAX)
				DrawImage(item_icons[icon], (x, top + 20.5), DI_ITEM_LEFT | DI_ITEM_VCENTER);

			int percentv = getinvamount(p);
			format = String.Format("%3d%%", percentv);
			int color = percentv > 50 ? 11 : percentv > 25 ? 23 : 2;
			DrawString(miniFont, format, (x + 34, top + 24), DI_TEXT_ALIGN_RIGHT, Font.CR_NATIVEPAL, 1, 0, 0, (1, 1), Translation.MakeID(Translation_Remap, color));

			String text;
			int pal;
			[text, pal] = ontext(p);
			if (text.length() > 0) DrawString(miniFont, text, (x + 34, top + 14), DI_TEXT_ALIGN_RIGHT, Font.CR_NATIVEPAL, 1, 0, 0, (1, 1), Translation.MakeID(Translation_Remap, pal));
		}
	}

	override void UpdateStatusBar(SummaryInfo info)
	{
		let p = Duke.GetViewPlayer();
		if (hud_size >= Hud_Mini)
		{
			DrawHud(p, hud_size == Hud_Nothing ? 0 : hud_size == Hud_full ? 1 : 2, info);
		}
		else
		{
			Statusbar(p);
			DoLevelStats(-1, info);
		}
	}
}
