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


class RedneckStatusBar : DukeCommonStatusBar
{
	override void Init()
	{
		numberFont = HudFont.Create(BigFont, 0, Mono_Off, 1, 1 );
		miniFont = HudFont.Create(SmallFont2, 0, Mono_Off, 1, 1 );
		digiFont = HudFont.Create(Font.FindFont("DigiFont"), 2, Mono_Off, 1, 1 );

		// optionally draw at the top of the screen.
		SetSize(tileHeight("BOTTOMSTATUSBAR"), 320, 200);
		scale = 0.5;

		ammo_sprites.PushV("", "AMMO", "SHOTGUNAMMO", "BATTERYAMMO", "HBOMBAMMO", "HBOMBAMMO", "SAWAMMO", "DEVISTATORAMMO", 
							"TRIPBOMBSPRITE", "GROWSPRITEICON", "HBOMBAMMO", "", "BOWLINGBALLSPRITE", "MOTOAMMO", "BOATAMMO", "", "RPG2SPRITE");
		item_icons.PushV("", "FIRSTAID_ICON", "STEROIDS_ICON", "HOLODUKE_ICON", "JETPACK_ICON", "HEAT_ICON", "AIRTANK_ICON", "BOOT_ICON" ); 
	}


	int getinvamount(DukePlayer p)
	{
		switch (p.inven_icon)
		{
		case Duke.ICON_FIRSTAID:
			return p.firstaid_amount;
		case Duke.ICON_STEROIDS:
			return (p.steroids_amount + 3) >> 2;
		case Duke.ICON_HOLODUKE:
			return (p.holoduke_amount) / 400;
		case Duke.ICON_JETPACK:
			return (p.jetpack_amount) / 100;
		case Duke.ICON_HEATS:
			return p.heat_amount / 12;
		case Duke.ICON_SCUBA:
			return (p.scuba_amount + 63) >> 6;
		case Duke.ICON_BOOTS:
			return (p.boot_amount / 10) >> 1;
		}

		return -1;
	}


	//==========================================================================
	//
	// Fullscreen HUD variant #1 for RR
	//
	//==========================================================================

	void FullscreenHUD1(DukePlayer p)
	{
		String format;
		TextureID img;
		double imgScale;
		double baseScale = (scale * numberFont.mFont.GetHeight()) * 0.76;

		//
		// Health
		//
		img = TexMan.CheckForTexture("SPINNINGNUKEICON1");
		let siz = TexMan.GetScaledSize(img);
		imgScale = baseScale / siz.Y;
		DrawTexture(img, (2, -2), DI_ITEM_LEFT_BOTTOM, scale:(imgScale, imgScale));

		if (!hud_flashing || p.last_extra > (Duke.MaxPlayerHealth() >> 2) || (PlayClock & 32) || (p.IsFrozen() && p.last_extra < 2))
		{
			int s = -8;
			if (hud_flashing && p.last_extra > Duke.MaxPlayerHealth())
				s += Raze.bsin(Raze.GetBuildTime() << 5) / 768;
			int intens = clamp(255 - 6 * s, 0, 255);
			format = String.Format("%d", p.last_extra);
			DrawString(numberFont, format, (26.5, -numberFont.mFont.GetHeight() * scale + 4), DI_TEXT_ALIGN_LEFT, Font.CR_UNTRANSLATED, intens / 255., 0, 0, (scale, scale));
		}

		//
		// drink
		//
		img = TexMan.CheckForTexture("BEER");
		siz = TexMan.GetScaledSize(img);
		imgScale = baseScale / siz.Y;
		DrawTexture(img, (74, -2), DI_ITEM_LEFT_BOTTOM, scale:(imgScale, imgScale));
		format = String.Format("%d", p.drink_amt);
		DrawString(numberFont, format, (86, -numberFont.mFont.GetHeight() * scale + 4), DI_TEXT_ALIGN_LEFT, scale:(scale, scale));

		//
		// eat
		//
		img = TexMan.CheckForTexture("COWPIE");
		siz = TexMan.GetScaledSize(img);
		imgScale = baseScale / siz.Y;
		DrawTexture(img, (133.5, -2), DI_ITEM_LEFT_BOTTOM, scale:(imgScale, imgScale));
		format = String.Format("%d", p.eat);
		DrawString(numberFont, format, (173, -numberFont.mFont.GetHeight() * scale + 4), DI_TEXT_ALIGN_LEFT, scale:(scale, scale));

		//
		// selected weapon
		//
		int weapon = p.curr_weapon;
		if (weapon == RRWpn.THROWINGDYNAMITE_WEAPON) weapon = RRWpn.DYNAMITE_WEAPON;

		let wicon = ammo_sprites[weapon];
		if (wicon.length() > 0)
		{
			int ammo = p.ammo_amount[weapon];
			bool reloadableWeapon = weapon == RRWpn.PISTOL_WEAPON || weapon == RRWpn.SHOTGUN_WEAPON;
			if (!reloadableWeapon || (reloadableWeapon && !cl_showmagamt))
			{
				format = String.Format("%d", ammo);
			}
			else
			{
				int clip;
				switch (weapon)
				{
					case RRWpn.PISTOL_WEAPON:
						clip = CalcMagazineAmount(ammo, 6, p.kickback_pic >= 1);
						break;
					case RRWpn.SHOTGUN_WEAPON:
						clip = CalcMagazineAmount(ammo, 2, p.kickback_pic >= 4);
						break;
				}
				format = String.Format("%d/%d", clip, ammo - clip);
			}
			img = TexMan.CheckForTexture(wicon, TexMan.TYPE_Any);
			siz = TexMan.GetScaledSize(img);
			imgScale = baseScale / siz.Y;
			let imgX = 22.5;
			let strlen = format.Length();

			if (strlen > 1)
			{
				imgX += (imgX * 0.755) * (strlen - 1);
			}

			if (weapon != RRWpn.KNEE_WEAPON && weapon != RRWpn.SLINGBLADE_WEAPON && (!hud_flashing || PlayClock & 32 || ammo > (Duke.MaxAmmoAmount(weapon) / 10)))
			{
				DrawString(numberFont, format, (-1, -numberFont.mFont.GetHeight() * scale + 4), DI_TEXT_ALIGN_RIGHT, scale:(scale, scale));
			}

			DrawTexture(img, (-imgX, -2), DI_ITEM_RIGHT_BOTTOM, scale:(imgScale, imgScale));
		}

		//
		// Selected inventory item
		//
		uint icon = p.inven_icon;
		if (icon > 0)
		{
			int x = -130;

			if (icon < Duke.ICON_MAX)
			{
				img = TexMan.CheckForTexture(item_icons[icon], TexMan.TYPE_Any);
				siz = TexMan.GetScaledSize(img);
				imgScale = baseScale / siz.Y;
				DrawTexture(img, (x, -2), DI_ITEM_RIGHT_BOTTOM, scale:(imgScale, imgScale));
			}

			int percentv = getinvamount(p);
			if (icon <= 2) format = String.Format("%d%%", percentv);
			else format = String.Format("%d", percentv);
			DrawString(miniFont, format, (x + 19, -miniFont.mFont.GetHeight() * scale - 1), DI_TEXT_ALIGN_RIGHT, scale:(scale, scale));

			let text = ontext(p);
			if (text.length() > 0) DrawString(miniFont, text, (x + 20, -miniFont.mFont.GetHeight() * scale - 15), DI_TEXT_ALIGN_RIGHT, scale:(scale, scale));
		}

		//
		// keys
		//
		if (p.keys[1]) DrawImage("ACCESSCARD", (-28.5,  -32    ), DI_ITEM_BOTTOM, 1, (-1, -1), (scale, scale), STYLE_Translucent, 0xffffffff, Translation.MakeID(Translation_Remap, 0));
		if (p.keys[3]) DrawImage("ACCESSCARD", (-21.25, -28.375), DI_ITEM_BOTTOM, 1, (-1, -1), (scale, scale), STYLE_Translucent, 0xffffffff, Translation.MakeID(Translation_Remap, 23));
		if (p.keys[2]) DrawImage("ACCESSCARD", (-14,    -24.75 ), DI_ITEM_BOTTOM, 1, (-1, -1), (scale, scale), STYLE_Translucent, 0xffffffff, Translation.MakeID(Translation_Remap, 21));
	}


	//==========================================================================
	//
	// Fullscreen HUD variant #2 for RR
	//
	//==========================================================================

	void FullscreenHUD2(DukePlayer p)
	{
		//
		// health
		//
		DrawImage("HEALTHBOX", (2, -2), DI_ITEM_LEFT_BOTTOM, scale:(scale, scale));
		int health = p.IsFrozen() ? 1 : p.last_extra;
		String format = String.Format("%d", health);
		DrawString(digiFont, format, (18.5, -digiFont.mFont.GetHeight() * scale - 7), DI_TEXT_ALIGN_CENTER, scale:(scale, scale));

		//
		// ammo
		//
		DrawImage("AMMOBOX", (41, -2), DI_ITEM_LEFT_BOTTOM, scale:(scale, scale));
		int wp = p.curr_weapon == RRWpn.THROWINGDYNAMITE_WEAPON? RRWpn.DYNAMITE_WEAPON : p.curr_weapon;
		format = String.Format("%d", p.ammo_amount[wp]);
		DrawString(digiFont, format, (56.5, -digiFont.mFont.GetHeight() * scale - 7), DI_TEXT_ALIGN_CENTER, scale:(scale, scale));

		//
		// inventory
		//
		uint icon = p.inven_icon;
		if (icon > 0)
		{
			int x = 84;
			DrawImage("INVENTORYBOX", (77, -2), DI_ITEM_LEFT_BOTTOM, scale:(scale, scale));
			if (icon < Duke.ICON_MAX)
				DrawImage(item_icons[icon], (x, -15.375), DI_ITEM_LEFT|DI_ITEM_VCENTER, scale:(scale, scale));

			int percentv = getinvamount(p);
			if (icon <= 2) format = String.Format("%d%%", percentv);
			else format = String.Format("%d", percentv);
			DrawString(miniFont, format, (x + 31.5, -miniFont.mFont.GetHeight() * scale - 6.5), DI_TEXT_ALIGN_RIGHT, scale:(scale, scale));
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
			double y = -40;
			//if (ud.multimode > 1) y -= 4;
			//if (ud.multimode > 4) y -= 4;
			DrawInventory(p, 0, y, DI_SCREEN_CENTER_BOTTOM);
			FullscreenHUD1(p);
			DoLevelStats(int(scale * tileHeight("BIGALPHANUM") + 10), info);
		}
		else if (style == 2)
		{
			DrawInventory(p, 56, -20, DI_SCREEN_CENTER_BOTTOM);
			FullscreenHUD2(p);
			DoLevelStats(int(scale * tileHeight("HEALTHBOX") + 4), info);
		}
		else
		{
			DrawInventory(p, 0, -20, DI_SCREEN_CENTER_BOTTOM);
			DoLevelStats(2, info);
		}
	}

	//==========================================================================
	//
	// Status bar drawer (RR)
	//
	//==========================================================================

	void DrawWeaponBar(DukePlayer p, int top)
	{
		double sbscale = 32800. / 65536.;

		DrawImage("WEAPONBAR", (0, 158), DI_ITEM_OFFSETS, scale:(sbscale, sbscale));

		for (int i = 0; i < 9; i++) 
		{
			String format, texname;

			if ((gameinfo.gameType & GAMEFLAG_RRRA) && i == 4 && p.curr_weapon == RRWpn.CHICKEN_WEAPON)
			{
				texname = "AMMO_ICON10";
				format = String.Format("%d", p.ammo_amount[RRWpn.CHICKEN_WEAPON]);
			}
			else
			{
				texname = "AMMO_ICON" .. i;
				format = String.Format("%d", p.ammo_amount[i+1]);
			}
			
			DrawImage(texname, (18 + i * 32, top - 6.5), DI_ITEM_OFFSETS, scale:(sbscale, sbscale));

			if (format.Length())
			{
				DrawString(miniFont, format, (38 + i * 32, 162.75 - miniFont.mFont.GetHeight() * scale * 0.5), DI_TEXT_ALIGN_CENTER, scale:(scale * .875, scale * .875));
			}
		}
	}


	//==========================================================================
	//
	// Status bar drawer (RR)
	//
	//==========================================================================

	void Statusbar(DukePlayer p)
	{
		let bsb = TexMan.CheckForTexture("BOTTOMSTATUSBAR", Texman.Type_Any);
		let siz = TexMan.GetScaledSize(bsb) * scale;

		double wh = 0;
		if (hud_size < Hud_Stbar) wh = tileHeight("WEAPONBAR") * scale;

		double h = siz.Y;
		double top = 200 - h;
		int left = (320 - int(siz.X)) / 2;
		BeginStatusBar(false, 320, 200, int(wh + h));
		DrawInventory(p, 160, hud_size <= Hud_Stbar? 148 : 154, 0);

		if (hud_size <= Hud_Stbar)
			DrawWeaponBar(p, int(top));

		if (hud_size == Hud_StbarOverlay) Set43ClipRect();
		DrawTexture(bsb, (left, top), DI_ITEM_LEFT_TOP, scale:(scale, scale));
		screen.ClearClipRect();

		String format;

		/*
		if (ud.multimode > 1 && !ud.coop)
		{
			DrawTexture("KILLSICON", (228, top + 8), DI_ITEM_OFFSETS, 1, 0, 0);
			format = String.Format("%d", max(p.frag - p.fraggedself, 0));
			DrawString(digiFont, format, (287, top + 17), DI_TEXT_ALIGN_CENTER, Font.CR_UNTRANSLATED, 1, 0, 0, (scale, scale));
		}
		else*/
		{
			let key = TexMan.CheckForTexture("ACCESS_ICON", TexMan.Type_Any);
			if (p.keys[3]) DrawTexture(key, (138, top + 13), DI_ITEM_OFFSETS, 1, (-1, -1), (scale, scale), STYLE_Translucent, 0xffffffff, Translation.MakeID(Translation_Remap, 23));
			if (p.keys[2]) DrawTexture(key, (152, top + 13), DI_ITEM_OFFSETS, 1, (-1, -1), (scale, scale), STYLE_Translucent, 0xffffffff, Translation.MakeID(Translation_Remap, 21));
			if (p.keys[1]) DrawTexture(key, (145, top + 21), DI_ITEM_OFFSETS, 1, (-1, -1), (scale, scale), STYLE_Translucent, 0xffffffff, Translation.MakeID(Translation_Remap, 0));
		}

		int num = (p.IsFrozen()) ? 1 : p.last_extra;
		format = String.Format("%d", num);
		DrawString(digiFont, format, (63.5, top + 15), DI_TEXT_ALIGN_CENTER, scale:(scale, scale));

		if (p.curr_weapon != RRWpn.KNEE_WEAPON)
		{
			int wep = (p.curr_weapon == RRWpn.THROWINGDYNAMITE_WEAPON) ? RRWpn.DYNAMITE_WEAPON : p.curr_weapon;
			format = String.Format("%d", p.ammo_amount[wep]);
			DrawString(digiFont, format, (106.25, top + 15), DI_TEXT_ALIGN_CENTER, scale:(scale, scale));
		}

		int icon = p.inven_icon;
		if (icon)
		{
			int x = 182;
			if (icon < Duke.ICON_MAX)
				DrawImage(item_icons[icon], (x, top + 20.125), DI_ITEM_LEFT | DI_ITEM_VCENTER, scale:(scale, scale));

			int percentv = getinvamount(p);
			if (icon <= 2) format = String.Format("%d%%", percentv);
			else format = String.Format("%d", percentv);
			DrawString(miniFont, format, (x + 38, top + 23.5), DI_TEXT_ALIGN_RIGHT, scale:(scale, scale));

			if (p.inven_icon == Duke.ICON_SCUBA || p.inven_icon == Duke.ICON_BOOTS) 
				DrawString(miniFont, "AUTO", (x + 39, top + 13), DI_TEXT_ALIGN_RIGHT, scale:(scale, scale));
		}

		p.drunkang = ((p.drink_amt * 8) + 1647) & 2047;
		if (p.drink_amt >= 100)
		{
			p.drink_amt = 100;
			p.drunkang = 400;
		}
		
		DrawImageRotated("GUTMETER", (256, top + 15), DI_ITEM_RELCENTER, p.drunkang * -Raze.BAngToDegree, 1, (scale, scale));
		DrawImageRotated("GUTMETER", (292, top + 15), DI_ITEM_RELCENTER, p.eatang * -Raze.BAngToDegree, 1, (scale, scale));

		if (p.drink_amt >= 0 && p.drink_amt <= 30)
		{
			DrawImage("GUTMETER_LIGHT1", (239, top + 24), DI_ITEM_OFFSETS, scale:(scale, scale));
		}
		else if (p.drink_amt >= 31 && p.drink_amt <= 65)
		{
			DrawImage("GUTMETER_LIGHT2", (248, top + 24), DI_ITEM_OFFSETS, scale:(scale, scale));
		}
		else if (p.drink_amt >= 66 && p.drink_amt <= 87)
		{
			DrawImage("GUTMETER_LIGHT3", (256, top + 24), DI_ITEM_OFFSETS, scale:(scale, scale));
		}
		else
		{
			DrawImage("GUTMETER_LIGHT4", (265, top + 24), DI_ITEM_OFFSETS, scale:(scale, scale));
		}

		if (p.eat >= 0 && p.eat <= 30)
		{
			DrawImage("GUTMETER_LIGHT1", (276, top + 24), DI_ITEM_OFFSETS, scale:(scale, scale));
		}
		else if (p.eat >= 31 && p.eat <= 65)
		{
			DrawImage("GUTMETER_LIGHT2", (285, top + 24), DI_ITEM_OFFSETS, scale:(scale, scale));
		}
		else if (p.eat >= 66 && p.eat <= 87)
		{
			DrawImage("GUTMETER_LIGHT3", (294, top + 24), DI_ITEM_OFFSETS, scale:(scale, scale));
		}
		else
		{
			DrawImage("GUTMETER_LIGHT4", (302, top + 24), DI_ITEM_OFFSETS, scale:(scale, scale));
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
