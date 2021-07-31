//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) 2020-2021 Christoph Oelckers

This file is part of Raze.

This is free software; you can redistribute it and/or
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

class BloodStatusBar : RazeStatusBar
{
	static const String gPackIcons[] = { "PackIcon1", "PackIcon2", "PackIcon3", "PackIcon4", "PackIcon5" };

	HUDFont smallf, tinyf;
	int team_score[2], team_ticker[2]; // placeholders for MP display
	bool gBlueFlagDropped, gRedFlagDropped; // also placeholders until we know where MP will go.

	override void Init()
	{
		smallf = HUDFont.Create(SmallFont, 0, Mono_Off, 0, 0);
		tinyf = HUDFont.Create(Font.FindFont("DIGIFONT"), 4, Mono_CellRight, 0, 0);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	int texWidth(String name)
	{
		let tex = TexMan.CheckForTexture(name, TexMan.Type_Any);
		let siz = TexMan.GetScaledSize(tex);
		return siz.x;
	}

	int texHeight(String name)
	{
		let tex = TexMan.CheckForTexture(name, TexMan.Type_Any);
		let siz = TexMan.GetScaledSize(tex);
		return siz.y;
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void DrawStatNumber(String pFormat, int nNumber, String nametemplate, double x, double y, int nShade, int nPalette, double nScale = 1, int align = 0)
	{
		String texname = String.Format("%s%d", nametemplate, 1);
		double width = (texWidth(texname) + 1) * nScale;

		String tempbuf = String.Format(pFormat, nNumber);
		x += 0.5;
		y += 0.5;   // This is needed because due to using floating point math, this code rounds slightly differently which for the numbers can be a problem.
		for (uint i = 0; i < tempbuf.length(); i++, x += width)
		{
			int c = tempbuf.ByteAt(i);
			if (c < "0" || c > "9") continue;
			texname = String.Format("%s%d", nametemplate, c - int("0"));
			int flags = align | DI_ITEM_RELCENTER;
			DrawImage(texname, (x, y), flags, 1, (-1, -1), (nScale, nScale), STYLE_Translucent, Raze.shadeToLight(nShade), Translation.MakeID(Translation_Remap, nPalette));
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void DrawCharArray(String text, String nametemplate, double x, double y)
	{
		String texname = String.Format("%s%d", nametemplate, 1);
		double width = (texWidth(texname) + 1);

		x += 0.5;
		y += 0.5;   // This is needed because due to using floating point math, this code rounds slightly differently which for the numbers can be a problem.

		for (uint i = 0; i < text.length(); i++, x += width)
		{
			int c = text.ByteAt(i);
			// Hackasaurus rex to give me a slash when drawing the weapon count of a reloadable gun.
			if (c == 47)
			{
				DrawImage("SBarSlash", (x, y), DI_ITEM_RELCENTER);
			}
			else
			{
				if (c < "0" || c > "9") continue;
				texname = String.Format("%s%d", nametemplate, c - int("0"));
				DrawImage(texname, (x, y), DI_ITEM_RELCENTER);
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void TileHGauge(String nTile, double x, double y, int nMult, int nDiv, double sc = 1)
	{
		int w = texWidth(nTile);
		double bx = w * sc * nMult / nDiv + x;
		double scale = (bx - x) / w;
		DrawImage(nTile, (x, y), DI_ITEM_LEFT_TOP, 1., (-1, -1), (sc, sc), clipwidth:scale);
	}



	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	void DoLevelStats(BloodPlayer pPlayer, int bottomy, Summaryinfo summary)
	{
		StatsPrintInfo stats;

		stats.fontscale = 1.;
		stats.screenbottomspace = bottomy;
		stats.statfont = SmallFont;
		stats.letterColor = TEXTCOLOR_DARKRED;
		stats.standardColor = TEXTCOLOR_DARKGRAY;

		if (automapMode == am_full)
		{
			stats.statfont = SmallFont2;
			stats.spacing = 6;
			stats.altspacing = SmallFont.GetHeight() + 2;
			if (hud_size <= Hud_StbarOverlay) stats.screenbottomspace = 56;
			PrintAutomapInfo(stats, false);
		}
		if (automapMode == am_off && hud_stats)
		{
			stats.completeColor = TEXTCOLOR_DARKGREEN;
			stats.spacing = SmallFont.GetHeight() + 2;

			PrintLevelStats(stats, summary);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
	const nPowerUps = 11;

	void drawPowerUps(BloodPlayer pPlayer)
	{
		static const float powerScale[] = { 0.4f, 0.4f, 0.3f, 0.3f, 0.4f, 0.3f, 0.4f, 0.5f, 0.3f, 0.4f, 0.4f };
		static const int powerYoffs[] = { 0, 5, 9, 5, 9, 7, 4, 5, 9, 4, 4 };

		static const int powerOrder[] = { Blood.kPwUpShadowCloak, Blood.kPwUpReflectShots, Blood.kPwUpDeathMask, Blood.kPwUpTwoGuns, Blood.kPwUpShadowCloakUseless, Blood.kPwUpFeatherFall,
										Blood.kPwUpGasMask, Blood.kPwUpDoppleganger, Blood.kPwUpAsbestArmor, Blood.kPwUpGrowShroom, Blood.kPwUpShrinkShroom };

		if (!hud_powerupduration)
			return;

		int powersort[nPowerUps];

		for (int i = 0; i < nPowerUps; i++) powersort[i] = i;

		for (int i = 0; i < nPowerUps; i++)
		{
			for (int j = i + 1; j < nPowerUps; j++)
			{
				int power1 = powersort[i];
				int power2 = powersort[j];
				if (pPlayer.pwUpTime[powerOrder[power1]] > pPlayer.pwUpTime[powerOrder[power2]])
				{
					powersort[i] = power2;
					powersort[j] = power1;
				}
			}
		}

		int warningTime = 5;
		int x = 15;
		int y = -50;
		for (int i = 0; i < nPowerUps; i++)
		{
			int order = powersort[i];
			int power = powerOrder[order];
			int time = pPlayer.pwUpTime[power];
			if (time > 0)
			{
				int remainingSeconds = time / 100;
				if (remainingSeconds > warningTime || (PlayClock & 32))
				{
					DrawTexture(Blood.PowerUpIcon(power), (x, y + powerYoffs[order]), DI_SCREEN_LEFT_CENTER | DI_ITEM_RELCENTER, scale:(powerScale[order], powerScale[order]));
				}

				DrawStatNumber("%d", remainingSeconds, "SBarNumberInv", x + 15, y, 0, remainingSeconds > warningTime ? 0 : Translation.MakeID(Translation_Remap, 2), 0.5, DI_SCREEN_LEFT_CENTER);
				y += 20;
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void drawInventory(BloodPlayer pPlayer, int x, int y)
	{
		int packs[5];
		if (pPlayer.packItemTime)
		{
			int nPacks = 0;
			int width = 0;
			for (int i = 0; i < 5; i++)
			{
				if (pPlayer.packSlots[i].curAmount)
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
				DrawImage("PackBG", (x + 1, y - 8), DI_ITEM_RELCENTER, style:STYLE_Normal);
				DrawImage("PackBG", (x + 1, y - 6), DI_ITEM_RELCENTER, style:STYLE_Normal);
				DrawImage(gPackIcons[nPack], (x + 1, y + 1), DI_ITEM_RELCENTER, style:STYLE_Normal);
				if (nPack == pPlayer.packItemId)
					DrawImage("PackSelect", (x + 1, y + 1), DI_ITEM_RELCENTER);
				int nShade;
				if (pPlayer.packSlots[nPack].isActive)
					nShade = 4;
				else
					nShade = 24;
				DrawStatNumber("%3d", pPlayer.packSlots[nPack].curAmount, "SBarPackAmount", x - 4, y - 13, nShade, 0);
				x += texWidth(gPackIcons[nPack]) + 1;
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void DrawPackItemInStatusBar(BloodPlayer pPlayer, int x, int y, int x2, int y2)
	{
		let id = pPlayer.packItemId;
		//id = 0;
		if (id < 0) return;

		DrawImage(gPackIcons[id], (x, y), DI_ITEM_RELCENTER, style:STYLE_Normal);
		DrawStatNumber("%3d", pPlayer.packSlots[id].curAmount, "SBarPackAmount", x2, y2, 0, 0);
	}

	void DrawPackItemInStatusBar2(BloodPlayer pPlayer, int x, int y, int x2, int y2, double nScale)
	{
		static const String packIcons2[] = { "Pack2Icon1", "Pack2Icon2", "Pack2Icon3", "Pack2Icon4", "Pack2Icon5" };
		static const float packScale[] = { 0.5f, 0.3f, 0.6f, 0.5f, 0.4f };
		static const int packYoffs[] = { 0, 0, 0, -4, 0 };

		if (pPlayer.packItemId < 0) return;
		let sc = packScale[pPlayer.packItemId];
		DrawImage(packIcons2[pPlayer.packItemId], (x, y + packYoffs[pPlayer.packItemId]), DI_ITEM_RELCENTER, scale:(sc, sc), style:STYLE_Normal);
		DrawStatNumber("%3d", pPlayer.packSlots[pPlayer.packItemId].curAmount, "SBarNumberInv", x2, y2, 0, 0, nScale);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void drawPlayerSlots(Array<BloodPlayer> players)
	{
		for (int nRows = (players.Size() - 1) / 4; nRows >= 0; nRows--)
		{
			for (int nCol = 0; nCol < 4; nCol++)
			{
				DrawImage("SBPlayerSlot", (-120 + nCol * 80, 4 + nRows * 9), DI_ITEM_RELCENTER|DI_SCREEN_CENTER_TOP, style:STYLE_Normal, col:0xffc0c0c0);
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------


	void drawPlayerFrags(Array<BloodPlayer> players)
	{
		drawPlayerSlots(players);
		for (int i = 0; i < players.Size(); i++)
		{
			int x = -160 + 80 * (i & 3);
			int y = 9 * (i / 4);
			int col = players[i].teamId & 3;
			int cr = col == 0? Font.CR_UNTRANSLATED : col == 1? Font.CR_BLUE : Font.CR_RED;
			DrawString(tinyf, Raze.PlayerName(i), (x + 4, y), DI_SCREEN_CENTER_TOP, cr, 1., -1, -1);
			String gTempStr = String.Format("%2d", players[i].fragCount);
			DrawString(tinyf, gTempStr, (x + 76, y), DI_SCREEN_CENTER_TOP, cr, 1., -1, -1);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void drawPlayerFlags(Array<BloodPlayer> players)
	{
		String gTempStr;
		drawPlayerSlots(players);
		for (int i = 0; i < players.Size(); i++)
		{
			int x = -160 + 80 * (i & 3);
			int y = 9 * (i / 4);
			int col = players[i].teamId & 3;
			gTempStr = String.Format("%s", Raze.PlayerName(i));
			int cr = col == 0? Font.CR_UNTRANSLATED : col == 1? Font.CR_BLUE : Font.CR_RED;
			DrawString(tinyf, gTempStr.MakeUpper(), (x + 4, y), DI_SCREEN_CENTER_TOP, cr, 1., -1, -1);

			x += 76;
			if (players[i].hasFlag & 2)
			{
				DrawString(tinyf, "F", (x, y), DI_SCREEN_CENTER_TOP, Font.CR_BLUE/*12*/, 1., -1, -1);
				x -= 6;
			}

			if (players[i].hasFlag & 1)
				DrawString(tinyf, "F", (x, y), DI_SCREEN_CENTER_TOP, Font.CR_RED/*11*/, 1., -1, -1);
		}
	}


	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void drawCtfHudVanilla(Array<BloodPlayer> players)
	{
		String gTempStr;
		int x = 1, y = 1;
		if (team_ticker[0] == 0 || (PlayClock & 8))
		{
			DrawString(smallf, "$TXT_COLOR_BLUE", (x, y), 0, Font.CR_LIGHTBLUE, 1., -1, -1);
			gTempStr = String.Format("%-3d", team_score[0]);
			DrawString(smallf, gTempStr, (x, y + 10), 0, Font.CR_LIGHTBLUE, 1., -1, -1);
		}
		x = -2;
		if (team_ticker[1] == 0 || (PlayClock & 8))
		{
			DrawString(smallf, "$TXT_COLOR_RED", (x, y), DI_TEXT_ALIGN_RIGHT, Font.CR_BRICK, 1., -1, -1);
			gTempStr = String.Format("%3d", team_score[1]);
			DrawString(smallf, gTempStr, (x, y + 10), DI_TEXT_ALIGN_RIGHT, Font.CR_BRICK, 1., -1, -1);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void flashTeamScore(int team, bool show)
	{
		if (team_ticker[team] == 0 || (PlayClock & 8))
		{
			if (show)
				DrawStatNumber("%d", team_score[team], "SBarNumberInv", -30, team ? 25 : -10, 0, team ? 2 : 10, 0.75, DI_SCREEN_RIGHT_CENTER);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void drawCtfHud(BloodPlayer pPlayer, Array<BloodPlayer> players)
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
		for (int i = 0; i < players.Size(); i++)
		{
			if ((players[i].hasFlag & 1) != 0)
			{
				blueFlagTaken = true;
				blueFlagCarrierColor = players[i].teamId & 3;
			}
			if ((players[i].hasFlag & 2) != 0)
			{
				redFlagTaken = true;
				redFlagCarrierColor = players[i].teamId & 3;
			}
		}

		bool meHaveBlueFlag = pPlayer.hasFlag & 1;
		int trans10 = Translation.MakeID(Translation_Remap, 10);
		int trans2 = Translation.MakeID(Translation_Remap, 2);
		DrawImage(meHaveBlueFlag ? "FlagHave" : "FlagHaveNot", (0, 75 - 100), DI_SCREEN_RIGHT_CENTER|DI_ITEM_RELCENTER, scale:(0.35, 0.35), translation:trans10);
		if (gBlueFlagDropped)
			DrawImage("FlagDropped", (305 - 320, 83 - 100), DI_SCREEN_RIGHT_CENTER|DI_ITEM_RELCENTER, translation:trans10);
		else if (blueFlagTaken)
			DrawImage("FlagTaken", (307 - 320, 77 - 100), DI_SCREEN_RIGHT_CENTER|DI_ITEM_RELCENTER, translation:blueFlagCarrierColor ? trans2 : trans10);
		flashTeamScore(0, true);

		bool meHaveRedFlag = pPlayer.hasFlag & 2;
		DrawImage(meHaveRedFlag ? "FlagHave" : "FlagHaveNot", (0, 10), DI_SCREEN_RIGHT_CENTER|DI_ITEM_RELCENTER, scale:(0.35, 0.35), translation:trans2);
		if (gRedFlagDropped)
			DrawImage("FlagDropped", (305 - 320, 17), DI_SCREEN_RIGHT_CENTER|DI_ITEM_RELCENTER, translation:trans2);
		else if (redFlagTaken)
			DrawImage("FlagTaken", (307 - 320, 11), DI_SCREEN_RIGHT_CENTER|DI_ITEM_RELCENTER, translation:redFlagCarrierColor ? trans2 : trans10);
		flashTeamScore(1, true);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void drawMultiHUD(BloodPlayer pPlayer, int nGameType)
	{
		if (nGameType >= 1)
		{
			Array<BloodPlayer> players;
			Blood.GetPlayers(players);
			if (nGameType == 3)
			{
				if (hud_ctf_Vanilla)
				{
					drawCtfHudVanilla(players);
				}
				else
				{
					drawCtfHud(pPlayer, players);
					drawPlayerFlags(players);
				}
			}
			else
			{
				drawPlayerFrags(players);
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	int DrawStatusBar(BloodPlayer pPlayer, int nPalette)
	{
		int th = texHeight("Statusbar");
		BeginStatusBar(false, 320, 200, th);


		int health = pPlayer.GetHealth();
		DrawImage("Statusbar", (160, 200), DI_ITEM_CENTER_BOTTOM);
		DrawPackItemInStatusBar(pPlayer, 265, 186, 260, 172);

		if (health >= 16 || (PlayClock & 16) || health == 0)
		{
			DrawStatNumber("%3d", health >> 4, "SBarHealthAmount", 86, 183, 0, 0);
		}
		if (pPlayer.curWeapon && pPlayer.weaponAmmo != -1)
		{
			int num = pPlayer.ammoCount[pPlayer.weaponAmmo];
			if (pPlayer.weaponAmmo == 6)
				num /= 10;
			DrawStatNumber("%3d", num, "SBarNumberAmmo", 216, 183, 0, 0);
		}
		for (int i = 9; i >= 1; i--)
		{
			int x = 135 + ((i - 1) / 3) * 23;
			int y = 182 + ((i - 1) % 3) * 6;
			int num = pPlayer.ammoCount[i];
			if (i == 6)
				num /= 10;
			DrawStatNumber("%3d", num, "SBarAmmoAmount", x, y, i == pPlayer.weaponAmmo ? -128 : 32, 10);
		}
		DrawStatNumber("%2d", pPlayer.ammoCount[10], "SBarAmmoAmount", 291, 194, pPlayer.weaponAmmo == 10 ? -128 : 32, 10);
		DrawStatNumber("%2d", pPlayer.ammoCount[11], "SBarAmmoAmount", 309, 194, pPlayer.weaponAmmo == 11 ? -128 : 32, 10);

		if (pPlayer.armor[1])
		{
			TileHGauge("Armor1Gauge", 44, 174, pPlayer.armor[1], 3200);
			DrawStatNumber("%3d", pPlayer.armor[1] >> 4, "SBarAmmoAmount", 50, 177, 0, 0);
		}
		if (pPlayer.armor[0])
		{
			TileHGauge("Armor3Gauge", 44, 182, pPlayer.armor[0], 3200);
			DrawStatNumber("%3d", pPlayer.armor[0] >> 4, "SBarAmmoAmount", 50, 185, 0, 0);
		}
		if (pPlayer.armor[2])
		{
			TileHGauge("Armor2Gauge", 44, 190, pPlayer.armor[2], 3200);
			DrawStatNumber("%3d", pPlayer.armor[2] >> 4, "SBarAmmoAmount", 50, 193, 0, 0);
		}

		for (int i = 0; i < 6; i++)
		{
			String nTile = String.Format("KEYICON%d", i + 1);
			double x = 73.5 + (i & 1) * 173;
			double y = 171.5 + (i >> 1) * 11;
			if (pPlayer.hasKey[i + 1])
				DrawImage(nTile, (x, y), DI_ITEM_RELCENTER, style:STYLE_Normal);
			else
				DrawImage(nTile, (x, y), DI_ITEM_RELCENTER, style:STYLE_Normal, col: 0xff606060, translation:Translation.MakeID(Translation_Remap, 5));
		}
		DrawImage("BlinkIcon", (118.5, 185.5), DI_ITEM_RELCENTER, col:Raze.shadeToLight(pPlayer.isRunning || cl_bloodvanillarun ? 16 : 40));
		DrawImage("BlinkIcon", (201.5, 185.5), DI_ITEM_RELCENTER, col:Raze.shadeToLight(pPlayer.isRunning || cl_bloodvanillarun ? 16 : 40));
		if (pPlayer.throwPower)
		{
			TileHGauge("ThrowGauge", 124, 175.5, pPlayer.throwPower, 65536);
		}
		drawInventory(pPlayer, 166, 200 - th);
		// Depending on the scale we can lower the stats display. This needs some tweaking but this catches the important default case already.
		return (hud_statscale <= 0.501f || hud_scalefactor < 0.7) && screen.GetAspectRatio() > 1.6 ? 28 : 56;
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	int DrawHUD1(BloodPlayer pPlayer, int nPalette)
	{
		BeginHUD(1, false, 320, 200);
		DrawImage("FullHUD", (34, 187 - 200), DI_ITEM_RELCENTER, style:STYLE_Normal, col:0xffc0c0c0, translation:nPalette);
		int health = pPlayer.GetHealth();
		if (health >= 16 || (PlayClock & 16) || health == 0)
		{
			DrawStatNumber("%3d", health >> 4, "SBarHealthAmount", 8, 183 - 200, 0, 0);
		}
		if (pPlayer.curWeapon && pPlayer.weaponAmmo != -1)
		{
			int num = pPlayer.ammoCount[pPlayer.weaponAmmo];
			if (pPlayer.weaponAmmo == 6)
				num /= 10;
			DrawStatNumber("%3d", num, "SBarNumberAmmo", 42, 183 - 200, 0, 0);
		}
		DrawImage("ArmorBox", (284 - 320, 187 - 200), DI_ITEM_RELCENTER, style:STYLE_Normal, col:0xffc0c0c0, translation:nPalette);
		if (pPlayer.armor[1])
		{
			TileHGauge("Armor1Gauge", 250 - 320, 175 - 200, pPlayer.armor[1], 3200);
			DrawStatNumber("%3d", pPlayer.armor[1] >> 4, "SBarAmmoAmount", 255 - 320, 178 - 200, 0, 0);
		}
		if (pPlayer.armor[0])
		{
			TileHGauge("Armor3Gauge", 250 - 320, 183 - 200, pPlayer.armor[0], 3200);
			DrawStatNumber("%3d", pPlayer.armor[0] >> 4, "SBarAmmoAmount", 255 - 320, 186 - 200, 0, 0);
		}
		if (pPlayer.armor[2])
		{
			TileHGauge("Armor2Gauge", 250 - 320, 191 - 200, pPlayer.armor[2], 3200);
			DrawStatNumber("%3d", pPlayer.armor[2] >> 4, "SBarAmmoAmount", 255 - 320, 194 - 200, 0, 0);
		}

		DrawPackItemInStatusBar(pPlayer, 286 - 320, 186 - 200, 302 - 320, 183 - 200);

		for (int i = 0; i < 6; i++)
		{
			String nTile = String.Format("KEYICON%d", i + 1);
			int x;
			int y = -6;
			if (i & 1)
			{
				x = -(78 + (i >> 1) * 10);
			}
			else
			{
				x = 73 + (i >> 1) * 10;
			}
			if (pPlayer.hasKey[i + 1])
				DrawImage(nTile, (x, y), DI_ITEM_RELCENTER, style:STYLE_Normal);
		}
		return 28;
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	int DrawHUD2(BloodPlayer pPlayer)
	{
		static const String ammoIcons[] = { "", "AmmoIcon1", "AmmoIcon2", "AmmoIcon3", "AmmoIcon4", "AmmoIcon5", "AmmoIcon6",
						"AmmoIcon7", "AmmoIcon8", "AmmoIcon9", "AmmoIcon10", "AmmoIcon11" };

		static const float ammoScale[] = { 0, 0.5f, 0.8f, 0.7f, 0.5f, 0.7f, 0.5f, 0.3f, 0.3f, 0.6f, 0.5f, 0.45f };
		static const int ammoYoffs[] = { 0, 0, 0, 3, -6, 2, 4, -6, -6, -6, 2, 2 };

		int health = pPlayer.GetHealth();
		BeginHUD(1, false, 320, 200);
		DrawImage("HealthIcon", (12, 195 - 200), 0, scale:(0.56, 0.56));
		DrawStatNumber("%d", health >> 4, "SBarNumberHealth", 28, 187 - 200, 0, 0);
		if (pPlayer.armor[1])
		{
			DrawImage("Armor1Icon", (70, 186 - 200), DI_ITEM_RELCENTER, scale:(0.5, 0.5));
			DrawStatNumber("%3d", pPlayer.armor[1] >> 4, "SBarNumberArmor2_", 83, 187 - 200, 0, 0, 0.65);
		}
		if (pPlayer.armor[0])
		{
			DrawImage("Armor3Icon", (112, 195 - 200), DI_ITEM_RELCENTER, scale:(0.5, 0.5));
			DrawStatNumber("%3d", pPlayer.armor[0] >> 4, "SBarNumberArmor1_", 125, 187 - 200, 0, 0, 0.65);
		}
		if (pPlayer.armor[2])
		{
			DrawImage("Armor2Icon", (155, 196 - 200), DI_ITEM_RELCENTER, scale:(0.5, 0.5));
			DrawStatNumber("%3d", pPlayer.armor[2] >> 4, "SBarNumberArmor3_", 170, 187 - 200, 0, 0, 0.65);
		}

		DrawPackItemInStatusBar2(pPlayer, 216 - 320, 194 - 200, 231 - 320, 187 - 200, 0.7);

		if (pPlayer.curWeapon && pPlayer.weaponAmmo != -1)
		{
			int num = pPlayer.ammoCount[pPlayer.weaponAmmo];
			if (pPlayer.weaponAmmo == 6)
				num /= 10;
			if (ammoIcons[pPlayer.weaponAmmo])
			{
				let scale = ammoScale[pPlayer.weaponAmmo];
				DrawImage(ammoIcons[pPlayer.weaponAmmo], (304 - 320, -8 + ammoYoffs[pPlayer.weaponAmmo]), DI_ITEM_RELCENTER, scale:(scale, scale));
			}

			bool reloadableWeapon = pPlayer.curWeapon == 3 && !pPlayer.powerupCheck(Blood.kPwUpTwoGuns);
			if (!reloadableWeapon || (reloadableWeapon && !cl_showmagamt))
			{
				DrawStatNumber("%3d", num, "SBarNumberAmmo", 267 - 320, 187 - 200, 0, 0);
			}
			else
			{
				int clip = CalcMagazineAmount(num, 2, pPlayer.weaponState == 1);
				int total = num - clip;
				String format = String.Format("%d/%d", clip, num - clip);

				DrawCharArray(format, "SBarNumberAmmo", (total < 10 ? 267 : 258) - 320, 187 - 200);
			}
		}

		for (int i = 0; i < 6; i++)
		{
			if (pPlayer.hasKey[i + 1])
			{
				let tile = String.Format("HUDKEYICON%d", i + 1);
				DrawImage(tile, (-60 + 10 * i, 170 - 200), DI_ITEM_RELCENTER, scale:(0.25, 0.25));
			}
		}

		BeginStatusBar(false, 320, 200, 28);
		if (pPlayer.throwPower)
			TileHGauge("ThrowGauge", 124, 175, pPlayer.throwPower, 65536);
		else
			drawInventory(pPlayer, 166, 200 - texHeight("FULLHUD") / 2 - 30);
		return 28;
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	override void UpdateStatusBar(SummaryInfo summary)
	{
		int nPalette = 0;
		let pPlayer = Blood.GetViewPlayer();
		int y = 0;

		int nGameType = Blood.getGameType();
		if (nGameType == 3)
		{
			if (pPlayer.teamId & 1)
				nPalette = 7;
			else
				nPalette = 10;

			nPalette = Translation.MakeID(Translation_Remap, nPalette);
		}

		if (hud_size == Hud_full)
		{
			y = DrawHUD2(pPlayer);
		}
		else if (hud_size > Hud_Stbar)
		{
			BeginStatusBar(false, 320, 200, 28);
			if (pPlayer.throwPower)
				TileHGauge("ThrowGauge", 124, 175, pPlayer.throwPower, 65536);
			else if (hud_size > Hud_StbarOverlay)
				drawInventory(pPlayer, 166, 200 - texHeight("FullHUD") / 2);
		}
		if (hud_size == Hud_Mini)
		{
			y = DrawHUD1(pPlayer, nPalette);
		}
		else if (hud_size <= Hud_StbarOverlay)
		{
			y = DrawStatusBar(pPlayer, nPalette);
		}
		DoLevelStats(pPlayer, y, summary);

		// All remaining parts must be done with HUD alignment rules, even when showing a status bar.
		BeginHUD(1, false, 320, 200);
		drawPowerUps(pPlayer);

		drawMultiHUD(pPlayer, nGameType);
	}
}

