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


class SWStatusBar : RazeStatusBar
{
	static const String icons[] = {
		"PanelMedkit",
		"PanelRepairKit",
		"PanelCloak",
		"PanelNightVision",
		"PanelChemBomb",
		"PanelFlashBomb",
		"PanelCaltrops"
	};

	enum EConstants
	{
		PANEL_HEALTH_BOX_X = 20,
		PANEL_BOX_Y = (174 - 6),
		PANEL_HEALTH_XOFF = 2,
		PANEL_HEALTH_YOFF = 4,

		PANEL_AMMO_BOX_X = 197,
		PANEL_AMMO_XOFF = 1,
		PANEL_AMMO_YOFF = 4,

		WSUM_X = 93,
		WSUM_Y = PANEL_BOX_Y + 1,
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
		INVENTORY_BOX_Y = (176 - 8),

		INVENTORY_PIC_XOFF = 1,
		INVENTORY_PIC_YOFF = 1,

		INVENTORY_PERCENT_XOFF = 19,
		INVENTORY_PERCENT_YOFF = 13,

		INVENTORY_STATE_XOFF = 19,
		INVENTORY_STATE_YOFF = 1,

		MINI_BAR_Y = 174,
		MINI_BAR_HEALTH_BOX_X = 4,
		MINI_BAR_AMMO_BOX_X = 32,
		MINI_BAR_INVENTORY_BOX_X = 64,
		MINI_BAR_INVENTORY_BOX_Y = MINI_BAR_Y,

	}

	TextureID PanelFont[10];
	TextureID SmallSBFont[3][12];
	HUDFont numberFont, miniFont;

	override void Init()
	{
		numberFont = HudFont.Create(BigFont, 0, Mono_Off, 1, 1);
		miniFont = HudFont.Create(SmallFont2, 0, Mono_Off, 1, 1);
		for (int i = 0; i < 10; i++) PanelFont[i] = TexMan.CheckForTexture(String.Format("PANEL_FONT_G%d", i), TexMan.Type_Any);
		for (int i = 0; i < 12; i++) 
		{
			SmallSBFont[0][i] = TexMan.CheckForTexture(String.Format("PANEL_SM_FONT_G%d", i), TexMan.Type_Any);
			SmallSBFont[1][i] = TexMan.CheckForTexture(String.Format("PANEL_SM_FONT_Y%d", i), TexMan.Type_Any);
			SmallSBFont[2][i] = TexMan.CheckForTexture(String.Format("PANEL_SM_FONT_R%d", i), TexMan.Type_Any);
		}
	}

	int tileHeight(String tex)
	{
		let img = TexMan.CheckForTexture(tex, TexMan.TYPE_Any);
		let siz = TexMan.GetScaledSize(img);
		return int(siz.Y);
	}

	int tileWidth(String tex)
	{
		let img = TexMan.CheckForTexture(tex, TexMan.TYPE_Any);
		let siz = TexMan.GetScaledSize(img);
		return int(siz.X);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void DisplayPanelNumber(double x, double y, int number)
	{
		String buffer;

		buffer = String.Format("%03d", number);

		for (uint i = 0; i < buffer.length(); i++)
		{
			let c = buffer.ByteAt(i);
			if (c < "0" || c > "9")
			{
				continue;
			}
			let tex = PanelFont[c - 48];
			DrawTexture(tex, (x, y), DI_ITEM_LEFT_TOP);
			let siz = TexMan.GetScaledSize(tex);
			x += siz.X + 1;
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void DisplaySummaryString(double x, double y, int color, int shade, String buffer)
	{
		for (uint i = 0; i < buffer.length(); i++)
		{
			let ch = buffer.ByteAt(i);

			if (ch == "/") ch = 10;
			else if (ch == ":") ch = 11;
			else if (ch >= "0" && ch <= "9") ch -= 48;
			else
			{
				if (ch != " ") Console.Printf("Invalid char %c", ch);
				x += 4;
				continue;
			}

			let font_pic = SmallSBFont[color][ch];
			DrawTexture(font_pic, (x, y), DI_ITEM_LEFT_TOP, col:Raze.shadeToLight(shade));
			let siz = TexMan.GetScaledSize(font_pic);
			x += siz.X + 1;
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void DisplayTimeLimit(SWPlayer pp)
	{
		int seconds = 0;// gNet.TimeLimitClock / 120;
		let ds = String.Format("%03d:%02d", seconds / 60, seconds % 60);
		DisplaySummaryString(PANEL_KEYS_BOX_X + 1, PANEL_BOX_Y + 6, 0, 0, ds);
	}

	//---------------------------------------------------------------------------
	//
	// todo: migrate to FFont to support localization
	//
	//---------------------------------------------------------------------------

	void DisplayTinyString(double xs, double ys, String buffer, int pal)
	{
		DrawString(miniFont, buffer, (xs, ys), DI_ITEM_LEFT_TOP);
	}

	void DisplayFragString(SWPlayer pp, double xs, double ys, String buffer)
	{
		DisplayTinyString(xs, ys, buffer, 0/*pp.DisplayColor()*/);
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
			String buffer;
			int xs, ys;
			int frag_bar;

			static const int xoffs[] =
			{
				69, 147, 225, 303
			};

			ys = FRAG_YOFF;

			// frag bar 0 or 1
			frag_bar = ((pnum) / 4);
			// move y down according to frag bar number
			ys = ys + (tileHeight("FRAG_BAR") - 2) * frag_bar;

			// move x over according to the number of players
			xs = xoffs[pnum];

			buffer.Format("%03d", Raze.PlayerFrags(pnum, -1));
			DisplayFragString(null/*&Player[pnum]*/, xs, ys, buffer);
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
			int xs, ys;
			int frag_bar;

			static const int xoffs[] =
			{
				7, 85, 163, 241
			};

			ys = FRAG_YOFF;

			// frag bar 0 or 1
			frag_bar = ((pnum) / 4);
			// move y down according to frag bar number
			ys = ys + (tileHeight("FRAG_BAR") - 2) * frag_bar;

			// move x over according to the number of players
			xs = xoffs[pnum];

			DisplayFragString(null/*&Player[pnum]*/, xs, ys, Raze.PlayerName(pnum));
		}
	}

	//---------------------------------------------------------------------------
	//
	// This cannot remain as it is.
	//
	//---------------------------------------------------------------------------

	void DisplayFragBar(SWPlayer pp)
	{
		// must draw this in HUD mode and align to the top center
		int i, num_frag_bars;
		int y;

		int numplayers = 1;

		if (numplayers <= 1) return;
		//if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE) return;

		num_frag_bars = ((numplayers-1)/4)+1;

		for (i = 0, y = 0; i < num_frag_bars; i++)
		{
			DrawImage("FRAG_BAR", (0, y), DI_ITEM_LEFT_TOP);
			y += tileHeight("FRAG_BAR") - 2;
		}
		DisplayFragNames();
		DisplayFragNumbers();
	}


	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void PlayerUpdateWeaponSummary(SWPlayer pp, int UpdateWeaponNum)
	{
		int x, y;
		int pos;
		int column;
		int WeaponNum, wpntmp;
		int colr, shade;
		String ds;

		WeaponNum = SW.RealWeapon(UpdateWeaponNum);

		static const int wsum_xoff[] = { 0,36,66 };
		static const String wsum_fmt2[] = { "%3d/%-3d", "%2d/%-2d", "%2d/%-2d" };

		pos = WeaponNum - 1;
		column = pos / 3;
		if (column > 2) column = 2;
		x = WSUM_X + wsum_xoff[column];
		y = WSUM_Y + (WSUM_YOFF * (pos % 3));

		if (UpdateWeaponNum == pp.WeaponNum())
		{
			shade = 0;
			colr = 0;
		}
		else
		{
			shade = 11;
			colr = 0;
		}

		wpntmp = WeaponNum + 1;
		if (wpntmp > 9)
			wpntmp = 0;
		ds = String.Format("%d:", wpntmp);

		if (pp.WpnFlags & (1 << WeaponNum))
			DisplaySummaryString(x, y, 1, shade, ds);
		else
			DisplaySummaryString(x, y, 2, shade + 6, ds);

		ds = String.Format(wsum_fmt2[column], pp.WpnAmmo[WeaponNum], SW.WeaponMaxAmmo(WeaponNum));
		DisplaySummaryString(x + 6, y, colr, shade, ds);
	}

	void PlayerUpdateWeaponSummaryAll(SWPlayer pp)
	{
		for (int i = SW.WPN_STAR; i <= SW.WPN_HEART; i++)
		{
			PlayerUpdateWeaponSummary(pp, i);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void DisplayKeys(SWPlayer pp, double xs, double ys, double scalex = 1, double scaley = 1)
	{
		double x, y;
		int row, col;
		int i;

		static const String StatusKeyPics[] =
		{
			"PANEL_KEY_RED",
			"PANEL_KEY_BLUE",
			"PANEL_KEY_GREEN",
			"PANEL_KEY_YELLOW",
			"PANEL_SKELKEY_GOLD",
			"PANEL_SKELKEY_SILVER",
			"PANEL_SKELKEY_BRONZE",
			"PANEL_SKELKEY_RED"
		};
		let tex = TexMan.CheckForTexture("PANEL_KEY_RED", TexMan.Type_Any);
		let size = TexMan.GetScaledSize(tex) + (1, 2);

		i = 0;
		for (row = 0; row < 2; row++)
		{
			for (col = 0; col < 2; col++)
			{
				if (pp.HasKey[i])
				{
					x = xs + PANEL_KEYS_XOFF + (row * size.X);
					y = ys + PANEL_KEYS_YOFF + (col * size.Y);
					DrawImage(StatusKeyPics[i], (x, y), DI_ITEM_LEFT_TOP, scale:(scalex, scaley));
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
				if (pp.HasKey[i + 4])
				{
					x = xs + PANEL_KEYS_XOFF + (row * size.X);
					y = ys + PANEL_KEYS_YOFF + (col * size.Y);
					DrawImage(StatusKeyPics[i + 4], (x, y), DI_ITEM_LEFT_TOP, scale:(scalex, scaley));
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

	void DisplayAllKeys(SWPlayer pp, double xs, double ys, double scalex = 1, double scaley = 1)
	{
		double x, y;
		int i;

		static const String StatusKeyPics[] =
		{
			"PANEL_KEY_RED",
			"PANEL_KEY_BLUE",
			"PANEL_KEY_GREEN",
			"PANEL_KEY_YELLOW",
			"PANEL_SKELKEY_GOLD",
			"PANEL_SKELKEY_SILVER",
			"PANEL_SKELKEY_BRONZE",
			"PANEL_SKELKEY_RED"
		};
		let tex = TexMan.CheckForTexture("PANEL_KEY_RED", TexMan.Type_Any);
		let size = TexMan.GetScaledSize(tex) + (1, 2);

		i = 0;
		int row = 0;
		for (int key = 0; key < 8; key++)
		{
			if (pp.HasKey[key])
			{
				DrawImage(StatusKeyPics[key], (xs, ys), DI_ITEM_LEFT_TOP, scale:(scalex, scaley));
				if (row == 0) ys += PANEL_KEYS_YOFF + size.Y;
				else
				{
					ys -= PANEL_KEYS_YOFF + size.Y;
					xs -= PANEL_KEYS_XOFF + size.X;
				}
				row ^= 1;
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void PlayerUpdateInventoryPercent(SWPlayer pp, double InventoryBoxX, double InventoryBoxY, double InventoryXoff, double InventoryYoff)
	{
		String ds;

		double x = InventoryBoxX + INVENTORY_PERCENT_XOFF + InventoryXoff;
		double y = InventoryBoxY + INVENTORY_PERCENT_YOFF + InventoryYoff;

		if (SW.InventoryFlags(pp.InventoryNum) & SW.INVF_COUNT)
		{
			ds = String.Format("%d", pp.InventoryAmount[pp.InventoryNum]);
		}
		else
		{
			ds = String.Format("%d%%", pp.InventoryPercent[pp.InventoryNum]);
		}
		DisplayTinyString(x, y, ds, 0);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void PlayerUpdateInventoryPic(SWPlayer pp, int InventoryBoxX, int InventoryBoxY, int InventoryXoff, int InventoryYoff)
	{
		int x = InventoryBoxX + INVENTORY_PIC_XOFF + InventoryXoff;
		int y = InventoryBoxY + INVENTORY_PIC_YOFF + InventoryYoff;
		DrawImage(icons[pp.InventoryNum], (x, y), DI_ITEM_LEFT_TOP);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void PlayerUpdateInventoryState(SWPlayer pp, double InventoryBoxX, double InventoryBoxY, int InventoryXoff, int InventoryYoff)
	{
		double x = InventoryBoxX + INVENTORY_STATE_XOFF + InventoryXoff;
		double y = InventoryBoxY + INVENTORY_STATE_YOFF + InventoryYoff;

		let flags = SW.InventoryFlags(pp.InventoryNum);

		if (flags & SW.INVF_AUTO_USE)
		{
			DisplayTinyString(x, y, "AUTO", 0);
		}
		else if (flags & SW.INVF_TIMED)
		{
			DisplayTinyString(x, y, pp.InventoryActive[pp.InventoryNum] ? "ON" : "OFF", 0);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void DisplayBarInventory(SWPlayer pp)
	{
		int InventoryBoxX = INVENTORY_BOX_X;
		int InventoryBoxY = INVENTORY_BOX_Y;

		int InventoryXoff = -1;
		int InventoryYoff = 0;

		// put pic
		if (pp.InventoryAmount[pp.InventoryNum])
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

	int NORM_CANG(int ang) 
	{ 
		return (((ang)+32) & 31); 
	}

	enum EXY
	{
		COMPASS_X = 140,
		COMPASS_Y = (162 - 5),
	};


	void DrawCompass(SWPlayer pp)
	{
		int start_ang, ang;
		int x_size = tileWidth("COMPASS_NORTH");
		int x;
		int i;

		static const String CompassPic[] =
		{
			"COMPASS_EAST", "COMPASS_EAST2",
			"COMPASS_TIC", "COMPASS_TIC2",
			"COMPASS_MID_TIC", "COMPASS_MID_TIC2",
			"COMPASS_TIC", "COMPASS_TIC2",

			"COMPASS_SOUTH", "COMPASS_SOUTH2",
			"COMPASS_TIC", "COMPASS_TIC2",
			"COMPASS_MID_TIC", "COMPASS_MID_TIC2",
			"COMPASS_TIC", "COMPASS_TIC2",

			"COMPASS_WEST", "COMPASS_WEST2",
			"COMPASS_TIC", "COMPASS_TIC2",
			"COMPASS_MID_TIC", "COMPASS_MID_TIC2",
			"COMPASS_TIC", "COMPASS_TIC2",

			"COMPASS_NORTH", "COMPASS_NORTH2",
			"COMPASS_TIC", "COMPASS_TIC2",
			"COMPASS_MID_TIC", "COMPASS_MID_TIC2",
			"COMPASS_TIC", "COMPASS_TIC2"
		};

		static const int CompassShade[] =
		{
			//20, 16, 11, 6, 1, 1, 6, 11, 16, 20
			25, 19, 15, 9, 1, 1, 9, 15, 19, 25
		};

		ang = pp.GetBuildAngle();

		if (pp.sop_remote)
			ang = 0;

		start_ang = (ang + 32) >> 6;

		start_ang = NORM_CANG(start_ang - 4);

		for (i = 0, x = COMPASS_X; i < 10; i++)
		{
			DrawImage(CompassPic[NORM_CANG(start_ang + i)], (x, COMPASS_Y), DI_ITEM_LEFT_TOP, col:Raze.shadeToLight(CompassShade[i]));
			x += x_size;
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void DrawStatusBar(SWPlayer pp)
	{
		let wnum = pp.WeaponNum();
		BeginStatusBar(false, 320, 200, tileHeight("STATUS_BAR"));

		if (hud_size == Hud_StbarOverlay) Set43ClipRect();
		DrawImage("STATUS_BAR", (160, 200), DI_ITEM_CENTER_BOTTOM);
		screen.ClearClipRect();
		DisplayPanelNumber(PANEL_HEALTH_BOX_X + PANEL_HEALTH_XOFF, PANEL_BOX_Y + PANEL_HEALTH_YOFF, pp.Health());
		DisplayPanelNumber(PANEL_ARMOR_BOX_X + PANEL_ARMOR_XOFF, PANEL_BOX_Y + PANEL_ARMOR_YOFF, pp.Armor);
		if (wnum != SW.WPN_FIST && wnum != SW.WPN_SWORD) DisplayPanelNumber(PANEL_AMMO_BOX_X + PANEL_AMMO_XOFF, PANEL_BOX_Y + PANEL_AMMO_YOFF, pp.WpnAmmo[wnum]);
		PlayerUpdateWeaponSummaryAll(pp);

		/*
		if (gNet.MultiGameType != MULTI_GAME_COMMBAT)
			DisplayKeys(pp, PANEL_KEYS_BOX_X, PANEL_BOX_Y);
		else if (gNet.TimeLimit)
			DisplayTimeLimit(pp);
		*/
		DisplayKeys(pp, PANEL_KEYS_BOX_X, PANEL_BOX_Y);

		DisplayBarInventory(pp);
		DrawCompass(pp);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void DisplayMinibarInventory(SWPlayer pp)
	{
		int InventoryBoxX = MINI_BAR_INVENTORY_BOX_X;
		int InventoryBoxY = MINI_BAR_INVENTORY_BOX_Y - 200;

		int InventoryXoff = 0;
		int InventoryYoff = 1;

		if (pp.InventoryAmount[pp.InventoryNum])
		{
			PlayerUpdateInventoryPic(pp, InventoryBoxX, InventoryBoxY, InventoryXoff + 1, InventoryYoff);
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

	bool, int DoReloadStatus(int reloadstate, int ammo)
	{
		bool reloading = ammo == 0 && reloadstate != 2;

		if (ammo == 0 && reloadstate == 0)
		{
			reloadstate = 1;
		}
		else if (ammo)
		{
			reloadstate = 0;
		}

		return reloading, reloadstate;
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void DrawHUD1(SWPlayer pp)
	{
		BeginHUD(1, false, 320, 200);

		int x = MINI_BAR_HEALTH_BOX_X;
		int y = -26;

		DrawImage("MINI_BAR_HEALTH_BOX_PIC", (x, y), DI_ITEM_LEFT_TOP);

		x = MINI_BAR_HEALTH_BOX_X + 3;
		DisplayPanelNumber(x, y + 5, pp.Health());

		let wnum = pp.WeaponNum();
		if (wnum != SW.WPN_SWORD && wnum != SW.WPN_FIST)
		{
			x = MINI_BAR_AMMO_BOX_X;
			DrawImage("MINI_BAR_AMMO_BOX_PIC", (x, y), DI_ITEM_LEFT_TOP);

			x = MINI_BAR_AMMO_BOX_X + 3;
			DisplayPanelNumber(x, y + 5, pp.WpnAmmo[wnum]);
		}

		if (!pp.InventoryAmount[pp.InventoryNum])
			return;

		// Inventory Box
		x = MINI_BAR_INVENTORY_BOX_X;

		DrawImage("MINI_BAR_INVENTORY_BOX_PIC", (x, y), DI_ITEM_LEFT_TOP);
		DisplayMinibarInventory(pp);
	}

	//==========================================================================
	//
	// Fullscreen HUD variant #1
	//
	//==========================================================================

	void DrawHUD2(SWPlayer pp, SummaryInfo info)
	{
		BeginHUD(1, false, 320, 200);

		String format;
		double imgScale;
		double baseScale = numberFont.mFont.GetHeight();

		//
		// Health
		//
		let img = TexMan.CheckForTexture("ICON_SM_MEDKIT", TexMan.Type_Any);
		let siz = TexMan.GetScaledSize(img);
		imgScale = baseScale / siz.Y;
		DrawTexture(img, (1.5, -1), DI_ITEM_LEFT_BOTTOM, scale:(imgScale, imgScale));

		let Health = pp.Health();
		let MaxHealth = pp.MaxUserHealth();
		if (!hud_flashing || Health > (MaxHealth >> 2) || (PlayClock & 32))
		{
			int s = -8;
			if (hud_flashing && Health > MaxHealth)
				s += Raze.bsin(PlayClock << 5) >> 10;
			int intens = clamp(255 - 4 * s, 0, 255);
			let pe = Color(255, intens, intens, intens);
			format = String.Format("%d", Health);
			DrawString(numberFont, format, (24.25, -numberFont.mFont.GetHeight() - 0.5), DI_TEXT_ALIGN_LEFT);
		}

		//
		// Armor
		//
		img = TexMan.CheckForTexture("ICON_ARMOR", TexMan.Type_Any);
		siz = TexMan.GetScaledSize(img);
		imgScale = baseScale / siz.Y;
		DrawTexture(img, (80.75, -1), DI_ITEM_LEFT_BOTTOM, scale:(imgScale, imgScale));

		format = String.Format("%d", pp.Armor);
		DrawString(numberFont, format, (108.5, -numberFont.mFont.GetHeight() - 0.5), DI_TEXT_ALIGN_LEFT);

		//
		// Weapon
		//
		static const String ammo_sprites[] = { "", "ICON_STAR", "ICON_LG_SHOTSHELL", "ICON_LG_UZI_AMMO", "ICON_MICRO_BATTERY", "ICON_LG_GRENADE", "ICON_LG_MINE", "ICON_RAIL_AMMO",
			"ICON_FIREBALL_LG_AMMO", "ICON_HEART_LG_AMMO", "ICON_FIREBALL_LG_AMMO", "ICON_FIREBALL_LG_AMMO", "ICON_MICRO_BATTERY", "" };

		int weapon = pp.WeaponNum();
		String wicon = ammo_sprites[weapon];
		if (wicon.length() > 0)
		{
			int ammo = pp.WpnAmmo[weapon];
			bool reloadableWeapon = weapon == SW.WPN_SHOTGUN || weapon == SW.WPN_UZI;
			if (!reloadableWeapon || (reloadableWeapon && !cl_showmagamt))
			{
				format = String.Format("%d", ammo);
			}
			else
			{
				int capacity;
				switch (weapon)
				{
				case SW.WPN_SHOTGUN:
					capacity = 4;
					break;
				case SW.WPN_UZI:
					capacity = pp.WpnUziType ? 50 : 100;
					break;
				}
				bool reload;
				[reload, pp.WpnReloadState] = DoReloadStatus(pp.WpnReloadState, ammo % capacity);
				int clip = CalcMagazineAmount(ammo, capacity, reload);
				format = String.Format("%d/%d", clip, ammo - clip);
			}
			img = TexMan.CheckForTexture(wicon, TexMan.Type_Any);
			siz = TexMan.GetScaledSize(img);
			imgScale = baseScale / siz.Y;
			let imgX = 21.125;
			let strlen = format.Length();

			if (strlen > 1)
			{
				imgX += (imgX * 0.855) * (strlen - 1);
			}

			if ((!hud_flashing || PlayClock & 32 || ammo > (SW.WeaponMaxAmmo(weapon) / 10)))
			{
				DrawString(numberFont, format, (-1.5, -numberFont.mFont.GetHeight() - 0.5), DI_TEXT_ALIGN_RIGHT);
			}

			DrawTexture(img, (-imgX, -1), DI_ITEM_RIGHT_BOTTOM, scale:(imgScale, imgScale));
		}

		//
		// Selected inventory item
		//
		img = TexMan.CheckForTexture(icons[pp.InventoryNum], TexMan.Type_Any);
		siz = TexMan.GetScaledSize(img);
		imgScale = baseScale / siz.Y;
		int x = 165;
		DrawTexture(img, (x, -1), DI_ITEM_LEFT_BOTTOM, scale:(imgScale, imgScale));

		PlayerUpdateInventoryState(pp, x + 3.0, -18.0, 1, 1);
		PlayerUpdateInventoryPercent(pp, x + 3.5, -20.5, 1, 1);

		//
		// keys
		//
		DisplayAllKeys(pp, -12, -38, 0.8625, 0.8625);
		DoLevelStats(baseScale + 4, info);
	}


	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void DrawInventoryIcon(double xs, double ys, int align, int InventoryNum, int amount, bool selected)
	{
		double x, y;
		int INVENTORY_ICON_WIDTH = 28;


		x = xs + (InventoryNum * INVENTORY_ICON_WIDTH);
		y = ys;
		let tex = icons[InventoryNum];
		DrawImage(tex, (x, y), align | DI_ITEM_LEFT_TOP, amount ? 1. : 0.333);
		if (selected)
		{
			DrawImage("SelectionBox", (x - 5, y - 5), align | DI_ITEM_LEFT_TOP);
		}
	}

	//////////////////////////////////////////////////////////////////////
	//
	// INVENTORY BAR
	//
	//////////////////////////////////////////////////////////////////////

	void DrawInventory(SWPlayer pp,  double xs, double ys, int align)
	{
		if (!pp.InventoryBarTics)
		{
			return;
		}

		for (int i = 0; i < pp.InventoryAmount.size(); i++)
		{
			DrawInventoryIcon(xs, ys, align, i, pp.InventoryAmount[i], i == pp.InventoryNum);
		}
	}

	//==========================================================================
	//
	// Statistics output
	//
	//==========================================================================

	void DoLevelStats(int bottomy, SummaryInfo info)
	{
		StatsPrintInfo stats; 
		stats.fontscale = 1;
		stats.screenbottomspace = bottomy;

		int y = -1;
		int mask = 1;
		if (automapMode == am_full)
		{
			stats.letterColor = Font.TEXTCOLOR_SAPPHIRE;
			stats.standardColor = Font.TEXTCOLOR_UNTRANSLATED;

			stats.statfont = SmallFont2;
			stats.spacing = 6;
			stats.altspacing = SmallFont.GetHeight() + 1;
			y = PrintAutomapInfo(stats, info, false);
			mask = 2;
		}
		if (hud_stats & mask)
		{
			stats.statfont = SmallFont;
			stats.spacing = 7;
			stats.letterColor = Font.TEXTCOLOR_RED;
			stats.standardColor = Font.TEXTCOLOR_TAN;
			stats.completeColor = Font.TEXTCOLOR_FIRE;
			PrintLevelStats(stats, info, y);
		}
	}


	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	override void UpdateStatusBar(SummaryInfo info)
	{
		let pp = SW.GetViewPlayer();
		int nPalette = 0;
		double inv_x, inv_y;
		int align;

		if (hud_size == Hud_Nothing)
		{
			align = DI_SCREEN_RIGHT_BOTTOM;
			inv_x = -210;
			inv_y = -28;
			DoLevelStats(2, info);
		}
		else if (hud_size == Hud_full)
		{
			align = DI_SCREEN_CENTER_BOTTOM;
			inv_x = -80;
			inv_y = -40;
			DrawHUD2(pp, info);
		}
		else if (hud_size == Hud_Mini)
		{
			align = DI_SCREEN_RIGHT_BOTTOM;
			inv_x = -210;
			inv_y = -28;
			DrawHUD1(pp);
			DoLevelStats(30, info);
		}
		else
		{
			align = 0;
			inv_x = 80;
			inv_y = 130;
			DrawStatusBar(pp);
			DoLevelStats(-3, info);
		}
		DrawInventory(pp, inv_x, inv_y, align);
	}
}

