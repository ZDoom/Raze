
class WHStatusBar : RazeStatusBar
{
	int healthfont[10];
	int potionfont[10];
	int scorefont[10];

	override void Init()
	{
		for (int i = 0;i < 10;i++)
		{
			healthfont[i] = TexMan.CheckForTexture(String.Format("SHEALTHFONT%d", i), TexMan.TYPE_ANY);
			potionfont[i] = TexMan.CheckForTexture(String.Format("SPOTIONFONT%d", i), TexMan.TYPE_ANY);
			scorefont[i] = TexMan.CheckForTexture(String.Format("SSCOREFONT%d", i), TexMan.TYPE_ANY);
		}
	}

	void drawHealthText(double x, double y, String text, int shade = 0, int pal = 0, bool shadow = true) 
	{
		for (int i = 0; i < text.Length())
		{
			int c = text.ByteAt(i);
			if (c < "0" || c > "9")
			{
				x += 4;
			}
			else
			{
				let tex = healthfont[c - 48];
				if(shadow)
					DrawTexture(tex, (x+1, y+1), DI_ITEM_LEFT_TOP, 0.5 col:0xff000000);
				DrawTexture(tex, (x, y), DI_ITEM_LEFT_TOP, col:Raze.shadeToLight(shade), pt:Translation.MakeID(Translation_Remap, pal));
				let siz = TexMan.GetScaledSize(tex);
				x += siz.X;
			}

		}
	}

	void drawPotionText(double x, double y, String text, int shade = 0, int pal = 0, bool shadow = true) 
	{
		for (int i = 0; i < text.Length())
		{
			int c = text.ByteAt(i);
			if (c < "0" || c > "9")
			{
				x += 4;
			}
			else
			{
				let tex = potionfont[c - 48];
				if(shadow)
					DrawTexture(tex, (x+1, y+1), DI_ITEM_LEFT_TOP, 0.5 col:0xff000000);
				DrawTexture(tex, (x, y), DI_ITEM_LEFT_TOP, col:Raze.shadeToLight(shade), pt:Translation.MakeID(Translation_Remap, pal));
				let siz = TexMan.GetScaledSize(tex);
				x += siz.X;
			}

		}
	}

	void drawScoreText(double x, double y, String text, int shade = 0, int pal = 0, bool shadow = true) 
	{
		for (int i = 0; i < text.Length())
		{
			int c = text.ByteAt(i);
			if (c < "0" || c > "9")
			{
				x += 4;
			}
			else
			{
				let tex = scorefont[c - 48];
				if(shadow)
					DrawTexture(tex, (x+1, y+1), DI_ITEM_LEFT_TOP, 0.5 col:0xff000000);
				DrawTexture(tex, (x, y), DI_ITEM_LEFT_TOP, col:Raze.shadeToLight(shade), pt:Translation.MakeID(Translation_Remap, pal));
				let siz = TexMan.GetScaledSize(tex);
				x += siz.X;
			}

		}
	}


}


/*

#pragma message("displaytext")
#if 0
	if (displaytime > 0)
		displaytime -= TICSPERFRAME;

	if (displaytime <= 0) {
		if (plr.manatime > 0) {
			if (plr.manatime < 512) {
				if ((plr.manatime % 64) > 32) {
					return;
				}
			}
			drawText(1, 18,24,  GStrings("FIRE RESISTANCE"), 0, 0, TextAlign.Left, 2, false);
		} else if (plr.poisoned == 1) {
			drawText(1, 18,24,  GStrings("POISONED"), 0, 0, TextAlign.Left, 2, false);
		} else if (plr.orbactive[5] > 0) {
			if (plr.orbactive[5] < 512) {
				if ((plr.orbactive[5] % 64) > 32) {
					return;
				}
			}
			drawText(1, 18,24, GStrings("FLYING"), 0, 0, TextAlign.Left, 2, false);
		} else if (plr.vampiretime > 0) {
			drawText(1, 18,24,  GStrings("ORNATE HORN"), 0, 0, TextAlign.Left, 2, false);
		}
	}
#endif



	public static void overwritesprite(int thex, int they, int tilenum, int shade, int stat, int dapalnum) {

		engine.rotatesprite(thex << 16, they << 16, 65536, (stat & 8) << 7, tilenum, shade, dapalnum,
				(((stat & 1) ^ 1) << 4) + (stat & 2) + ((stat & 4) >> 2) + (((stat & 16) >> 2) ^ ((stat & 8) >> 1)) + 8
						+ (stat & 256) + (stat & 512),
				windowx1, windowy1, windowx2, windowy2);
	}

	public static void levelpic(PLAYER plr, int x, int y, int scale) {
		if (plr.selectedgun == 6) {
			Bitoa(plr.ammo[6], tempchar);

			if(game.WH2) {
				int px = (x - mulscale(314, scale, 16));
				int py = (y - mulscale(43, scale, 16));
				engine.rotatesprite(px << 16, py << 16, 2*scale, 0,
						1916, 0, 0, 8 | 16, px + mulscale(4, scale, 16), py + mulscale(4, scale, 16), xdim, py + mulscale(20, scale, 16));
			} else
			engine.rotatesprite(x - mulscale(313, scale, 16) << 16, y - mulscale(43, scale, 16) << 16, scale, 0,
					SARROWS, 0, 0, 8 | 16, 0, 0, xdim, ydim - 1);
			game.getFont(4).drawText(x - mulscale(235, scale, 16), y - mulscale(40, scale, 16), tempchar, scale, 0, 0,
					TextAlign.Left, 0, false);
		} else if (plr.selectedgun == 7 && plr.weapon[7] == 2) {
			Bitoa(plr.ammo[7], tempchar);
			engine.rotatesprite(x - mulscale(game.WH2 ? 314 : 313, scale, 16) << 16, y - mulscale(46, scale, 16) << 16, scale, 0, SPIKES,
					0, 0, 8 | 16, 0, 0, xdim, ydim - 1);
			game.getFont(4).drawText(x - mulscale(235, scale, 16), y - mulscale(40, scale, 16), tempchar, scale, 0, 0,
					TextAlign.Left, 0, false);
		} else {
			if(game.WH2) {
				int tilenum = 1917 + (plr.lvl - 1);
				int px = (x - mulscale(314, scale, 16));
				int py = (y - mulscale(43, scale, 16));
				engine.rotatesprite(px << 16, py << 16, 2*scale, 0,
						tilenum, 0, 0, 8 | 16, px + mulscale(4, scale, 16), py + mulscale(4, scale, 16), xdim, py + mulscale(20, scale, 16));
			} else {
				int tilenum = SPLAYERLVL + (plr.lvl - 1);
				engine.rotatesprite((x - mulscale(313, scale, 16)) << 16, (y - mulscale(43, scale, 16)) << 16, scale, 0,
						tilenum, 0, 0, 8 | 16, 0, 0, xdim, ydim - 1);
			}
		}
	}

	public static void drawscore(PLAYER plr, int x, int y, int scale) {
		Bitoa(plr.score, scorebuf);
		engine.rotatesprite((x - mulscale(game.WH2 ? 314 : 313, scale, 16)) << 16, (y - mulscale(85, scale, 16)) << 16, scale, 0,
				SSCOREBACKPIC, 0, 0, 8 | 16, 0, 0, xdim, ydim - 1);
		game.getFont(4).drawText(x - mulscale(259, scale, 16), y - mulscale(81, scale, 16), scorebuf, scale, 0, 0,
				TextAlign.Left, 0, false);
	}

	public static void updatepics(PLAYER plr, int x, int y, int scale) {
		drawscore(plr, x, y, scale);
		if (netgame) {
			if (game.nNetMode == NetMode.Multiplayer)
				captureflagpic(scale);
			else
				fragspic(plr, scale);
		} else
			potionpic(plr, plr.currentpotion, x, y, scale);

		levelpic(plr, x, y, scale);
		drawhealth(plr, x, y, scale);
		drawarmor(plr, x, y, scale);
		keyspic(plr, x, y, scale);
	}

	public static void captureflagpic(int scale) {

		int i;
		overwritesprite(260 << 1, 387, SPOTIONBACKPIC, 0, 0, 0);

		for (i = 0; i < 4; i++) {
//			if( teaminplay[i] ) { XXX
			overwritesprite(((int) sflag[i].x << 1) + 6, (int) sflag[i].y + 8, STHEFLAG, 0, 0, (int) sflag[i].z);
//				 Bitoa(teamscore[i],tempchar);
//			fancyfont(((int) sflag[i].x << 1) + 16, (int) sflag[i].y + 16, SPOTIONFONT - 26, tempchar, 0);
//			}
		}
	}

	public static void fragspic(PLAYER plr, int scale) {
		if (whcfg.gViewSize == 320) {

			int x = windowx2 / 2 + 200;
			int y = windowy2 - 94;
			overwritesprite(x, y, SPOTIONBACKPIC, 0, 0, 0);

//			Bitoa(teamscore[pyrn],tempchar); XXX
			game.getFont(2).drawText(x + 10, y + 10, tempchar, 0, 0, TextAlign.Left, 0, false);
		}
	}

	public static void keyspic(PLAYER plr, int x, int y, int scale) {
		y -= mulscale(85, scale, 16);
		if (plr.treasure[TBRASSKEY] == 1)
			engine.rotatesprite((x + mulscale(180, scale, 16)) << 16, y << 16, scale, 0, SKEYBRASS, 0, 0, 8, 0, 0, xdim,
					ydim - 1);
		else
			engine.rotatesprite((x + mulscale(180, scale, 16)) << 16, y << 16, scale, 0, SKEYBLANK, 0, 0, 8, 0, 0, xdim,
					ydim - 1);

		y += mulscale(22, scale, 16);
		if (plr.treasure[TBLACKKEY] == 1)
			engine.rotatesprite((x + mulscale(180, scale, 16)) << 16, y << 16, scale, 0, SKEYBLACK, 0, 0, 8, 0, 0, xdim,
					ydim - 1);
		else
			engine.rotatesprite((x + mulscale(180, scale, 16)) << 16, y << 16, scale, 0, SKEYBLANK, 0, 0, 8, 0, 0, xdim,
					ydim - 1);

		y += mulscale(22, scale, 16);
		if (plr.treasure[TGLASSKEY] == 1)
			engine.rotatesprite((x + mulscale(180, scale, 16)) << 16, y << 16, scale, 0, SKEYGLASS, 0, 0, 8, 0, 0, xdim,
					ydim - 1);
		else
			engine.rotatesprite((x + mulscale(180, scale, 16)) << 16, y << 16, scale, 0, SKEYBLANK, 0, 0, 8, 0, 0, xdim,
					ydim - 1);

		y += mulscale(22, scale, 16);
		if (plr.treasure[TIVORYKEY] == 1)
			engine.rotatesprite((x + mulscale(180, scale, 16)) << 16, y << 16, scale, 0, SKEYIVORY, 0, 0, 8, 0, 0, xdim,
					ydim - 1);
		else
			engine.rotatesprite((x + mulscale(180, scale, 16)) << 16, y << 16, scale, 0, SKEYBLANK, 0, 0, 8, 0, 0, xdim,
					ydim - 1);
	}

	public static void drawhealth(PLAYER plr, int x, int y, int scale) {
		Bitoa(plr.health, healthbuf);
		if (plr.poisoned == 1) {
			int flag = 0;
			switch (sintable[(10 * totalclock) & 2047] / 4096) {
			case 0:
				flag = 0;
				break;
			case 1:
			case -1:
				flag = 1;
				break;
			case -2:
			case 2:
				flag = 33;
				break;
			default:
				game.getFont(2).drawText(x - mulscale(167, scale, 16), y - mulscale(70, scale, 16), healthbuf, scale, 0,
						0, TextAlign.Left, 0, false);
				return;
			}

			engine.rotatesprite((x - mulscale(171, scale, 16)) << 16, (y - mulscale(75, scale, 16)) << 16, scale, 0,
					SHEALTHBACK, 0, 6, 8 | 16 | flag, windowx1, windowy1, windowx2, windowy2);
			game.getFont(2).drawText(x - mulscale(167, scale, 16), y - mulscale(70, scale, 16), healthbuf, scale, 0, 0,
					TextAlign.Left, 0, false);
		} else {
			engine.rotatesprite((x - mulscale(171, scale, 16)) << 16, (y - mulscale(75, scale, 16)) << 16, scale, 0,
					SHEALTHBACK, 0, 0, 8 | 16, 0, 0, xdim, ydim - 1);
			game.getFont(2).drawText(x - mulscale(167, scale, 16), y - mulscale(70, scale, 16), healthbuf, scale, 0, 0,
					TextAlign.Left, 0, false);
		}
	}

	public static void drawarmor(PLAYER plr, int x, int y, int scale) {
		Bitoa(plr.armor, armorbuf);

		engine.rotatesprite((x + mulscale(81, scale, 16)) << 16, (y - mulscale(75, scale, 16)) << 16, scale, 0,
				SHEALTHBACK, 0, 0, 8 | 16, 0, 0, xdim, ydim - 1);
		game.getFont(2).drawText(x + mulscale(89, scale, 16), y - mulscale(70, scale, 16), armorbuf, scale, 0, 0,
				TextAlign.Left, 0, false);
	}



	public static void drawInterface(PLAYER plr) {
		int hudscale = whcfg.gHudScale;
		if (whcfg.gViewSize == 1)
			drawhud(plr, windowx2 / 2, windowy2 + 1, hudscale);
		if (plr.potion[0] == 0 && plr.health > 0 && plr.health < 21)
			game.getFont(1).drawText(160, 5, toCharArray("health critical"), 0, 7, TextAlign.Center, 2, false);

		if (justwarpedfx > 0)
			engine.rotatesprite(320 << 15, 200 << 15, justwarpedcnt << 9, 0, ANNIHILATE, 0, 0, 1 + 2, 0, 0, xdim - 1,
					ydim - 1);
		
		int pwpos = 0;
		
		if (plr.helmettime > 0) 
		{
			engine.rotatesprite(300 << 16, (pwpos += tilesizy[HELMET] >> 2) << 16, 16384, 0, HELMET, 0, 0, 2 + 8 + 512, 0, 0,
					xdim - 1, ydim - 1);
			pwpos += 10;
		}
			
		if(plr.vampiretime > 0) 
		{
			engine.rotatesprite(300 << 16, (pwpos += tilesizy[THEHORN] / 6) << 16, 12000, 0, THEHORN, 0, 0, 2 + 8 + 512, 0, 0,
					xdim - 1, ydim - 1);
			pwpos += 10;
		}

		if(plr.orbactive[5] > 0) 
		{
			engine.rotatesprite(300 << 16, (pwpos += tilesizy[SCROLLFLY] >> 2) << 16, 16384, 0, SCROLLFLY, 0, 0, 2 + 8 + 512, 0, 0,
					xdim - 1, ydim - 1);
			pwpos += 10;
		}
	
		if(plr.shadowtime > 0) 
		{
			engine.rotatesprite(300 << 16, (pwpos += tilesizy[SCROLLSCARE] >> 2) << 16, 16384, 0, SCROLLSCARE, 0, 0, 2 + 8 + 512, 0, 0,
					xdim - 1, ydim - 1);
			pwpos += 10;
		}
			
		if(plr.nightglowtime > 0) 
		{
			engine.rotatesprite(300 << 16, (pwpos += tilesizy[SCROLLNIGHT] >> 2) << 16, 16384, 0, SCROLLNIGHT, 0, 0, 2 + 8 + 512, 0, 0,
					xdim - 1, ydim - 1);
			pwpos += 10;
		}
		
		boolean message = whcfg.MessageState && displaytime > 0;
			
		if (message)
			game.getFont(1).drawText(5, 5, displaybuf, 0, 7, TextAlign.Left, 2 | 256, false);

		int amposx = 10;
		int amposy = 7;
		if(message) {
			amposy += 15;
			if(dimension == 2)
				amposy += 10;
			if(dimension == 2 && followmode)
				amposy += 10;
		}
		
		if(plr.treasure[TONYXRING] != 0)
		{
			engine.rotatesprite(amposx << 16, amposy << 16, 16384, 0, ONYXRING, 0, 0, 2 | 8 | 256, 0, 0,
					xdim - 1, ydim - 1);
			amposx += 20;
		}
		
		if(plr.treasure[TAMULETOFTHEMIST] != 0 && plr.invisibletime > 0)
		{
			engine.rotatesprite(amposx << 16, amposy << 16, 16384, 0, AMULETOFTHEMIST, 0, 0, 2 | 8 | 256, 0, 0,
					xdim - 1, ydim - 1);
			amposx += 20;
		}
		
		if(plr.treasure[TADAMANTINERING] != 0)
		{
			engine.rotatesprite(amposx << 16, amposy << 16, 16384, 0, ADAMANTINERING, 0, 0, 2 | 8 | 256, 0, 0,
					xdim - 1, ydim - 1);
			amposx += 20;
		}
		
		if(plr.treasure[TBLUESCEPTER] != 0)
		{
			engine.rotatesprite(amposx << 16, (amposy + 9) << 16, 16384, 0, BLUESCEPTER, 0, 0, 2 | 8 | 256, 0, 0,
					xdim - 1, Gameutils.coordsConvertYScaled(amposy + 4));
			amposx += 20;
		}
		
		if(plr.treasure[TYELLOWSCEPTER] != 0)
		{
			engine.rotatesprite(amposx << 16, (amposy + 9) << 16, 16384, 0, YELLOWSCEPTER
					, 0, 0, 2 | 8 | 256, 0, 0,
					xdim - 1, Gameutils.coordsConvertYScaled(amposy + 4));
			amposx += 20;
		}

		if (whcfg.gCrosshair) {
			int col = 17;
			engine.getrender().drawline256((xdim - mulscale(whcfg.gCrossSize, 16, 16)) << 11, ydim << 11,
					(xdim - mulscale(whcfg.gCrossSize, 4, 16)) << 11, ydim << 11, col);
			engine.getrender().drawline256((xdim + mulscale(whcfg.gCrossSize, 4, 16)) << 11, ydim << 11,
					(xdim + mulscale(whcfg.gCrossSize, 16, 16)) << 11, ydim << 11, col);
			engine.getrender().drawline256(xdim << 11, (ydim - mulscale(whcfg.gCrossSize, 16, 16)) << 11, xdim << 11,
					(ydim - mulscale(whcfg.gCrossSize, 4, 16)) << 11, col);
			engine.getrender().drawline256(xdim << 11, (ydim + mulscale(whcfg.gCrossSize, 4, 16)) << 11, xdim << 11,
					(ydim + mulscale(whcfg.gCrossSize, 16, 16)) << 11, col);
		}

		if (plr.spiked != 0)
			spikeanimation(plr);

		int y = windowy2 - 20;
		if (whcfg.gViewSize == 1)
			y -= mulscale(tilesizy[SSTATUSBAR], hudscale, 16);

		if (whcfg.gShowStat == 1 || (whcfg.gShowStat == 2 && dimension == 2))
			drawStatistics(10, y, whcfg.gStatSize);

		if (game.gPaused)
			game.getFont(0).drawText(160, 5, toCharArray("Pause"), 0, 0, TextAlign.Center, 2, true);
		
		if (game.isCurrentScreen(gGameScreen) && totalclock < gNameShowTime) {
			int transp = 0;
			if (totalclock > gNameShowTime - 20)
				transp = 1;
			if (totalclock > gNameShowTime - 10)
				transp = 33;

			if (whcfg.showMapInfo && !game.menu.gShowMenu) {
				if(gCurrentEpisode != null && gCurrentEpisode.getMap(mapon) != null)
					game.getFont(1).drawText(160, 114, gCurrentEpisode.getMap(mapon).title, 0, 0, TextAlign.Center, 2 | transp, true);
				else if (boardfilename != null)
					game.getFont(1).drawText(160, 114, boardfilename, 0, 0, TextAlign.Center, 2 | transp, true);
			}
		}

		
	}

	public static void drawStatistics(int x, int y, int zoom) {
		float viewzoom = (zoom / 65536.0f);
		BuildFont font = game.getFont(1);

		buildString(buffer, 0, "kills: ");

		int yoffset = (int) (4 * (font.getHeight()) * viewzoom);
		y -= yoffset;

		int statx = x;
		int staty = y;

		font.drawText(statx, staty, buffer, zoom, 0, 7, TextAlign.Left, 256, false);

		int alignx = font.getWidth(buffer, zoom);

		int offs = Bitoa(kills, buffer);
		offs = buildString(buffer, offs, " / ", killcnt);
		font.drawText(statx += (alignx + 2), staty, buffer, zoom, 0, 0, TextAlign.Left, 256, false);

		statx = x;
		staty = y + (int) (15 * viewzoom);

		buildString(buffer, 0, "treasures: ");

		font.drawText(statx, staty, buffer, zoom, 0, 7, TextAlign.Left, 256, false);

		alignx = font.getWidth(buffer, zoom);

		offs = Bitoa(treasuresfound, buffer, 2);
		offs = buildString(buffer, offs, " / ", treasurescnt, 2);

		font.drawText(statx += (alignx + 2), staty, buffer, zoom, 0, 0, TextAlign.Left, 256, false);

		statx = x;
		staty = y + (int) (30 * viewzoom);

		buildString(buffer, 0, "time: ");

		font.drawText(statx, staty, buffer, zoom, 0, 7, TextAlign.Left, 256, false);
		alignx = font.getWidth(buffer, zoom);

		offs = Bitoa(minutes, buffer, 2);
		offs = buildString(buffer, offs, " : ", seconds, 2);

		font.drawText(statx += (alignx + 2), staty, buffer, zoom, 0, 0, TextAlign.Left, 256, false);
	}

	public static void drawhud(PLAYER plr, int x, int y, int scale) {


		engine.rotatesprite(x << 16, (y << 16) - tilesizy[SSTATUSBAR] * scale / 2, scale, 0, SSTATUSBAR, 0, 0, 8, 0, 0,
				xdim, ydim - 1);
		updatepics(plr, x, y, scale);

		int bookpic = plr.spellbook;
		if (plr.spellbookflip == 0)
			bookpic = 8;

		if (bookpic < sspellbookanim[plr.currentorb].length && (plr.orbammo[plr.currentorb] > 0 || plr.currweaponfired == 4)) {
			plr.spellbookframe = sspellbookanim[plr.currentorb][bookpic].daweaponframe;
			int dax = x + mulscale(sspellbookanim[plr.currentorb][bookpic].currx, scale, 16);
			int day = y + mulscale(sspellbookanim[plr.currentorb][bookpic].curry, scale, 16);

			engine.rotatesprite(dax << 16, day << 16, scale, 0, plr.spellbookframe, 0, 0, 8 | 16, 0, 0, xdim, ydim - 1);
			Bitoa(plr.orbammo[plr.currentorb], tempchar);
			game.getFont(4).drawText(x - mulscale(67, scale, 16), y - mulscale(39, scale, 16), tempchar, scale, 0, 0,
					TextAlign.Left, 0, false);
		}
	}

	public static int coordsConvertXScaled(int coord, int bits) {
		int oxdim = xdim;
		int xdim = (4 * ydim) / 3;
		int offset = oxdim - xdim;

		int normxofs = coord - (320 << 15);
		int wx = (xdim << 15) + scale(normxofs, xdim, 320);
		wx += (oxdim - xdim) / 2;

		if ((bits & 256) == 256)
			return wx - offset / 2 - 1;
		if ((bits & 512) == 512)
			return wx + offset / 2 - 1;

		return wx - 1;
	}

	public static int coordsConvertYScaled(int coord) {
		int ydim = (3 * xdim) / 4;
		int buildim = 200 * ydim / Engine.ydim;
		int normxofs = coord - (buildim << 15);
		int wy = (ydim << 15) + scale(normxofs, ydim, buildim);

		return wy;
	}

#if 0
void orbpic(PLAYER& plr, int currentorb) {
	if (plr.orbammo[currentorb] < 0)
		plr.orbammo[currentorb] = 0;

#pragma message("fix orbpic")
#if 0
	itoa(plr->orbammo[currentorb],tempbuf,10);

	int y = 382;// was 389 originally.
	if (currentorb == 2)
		y = 381;
	if (currentorb == 3)
		y = 383;
	if (currentorb == 6)
		y = 383;
	if (currentorb == 7)
		y = 380;

	int spellbookpage = sspellbookanim[currentorb][8].daweaponframe;
	overwritesprite(121 << 1, y, spellbookpage, 0, 0, 0);
	fancyfont(126<<1,439,SSCOREFONT-26,tempbuf,0);
#endif
}


void potionpic(PLAYER& plr, int currentpotion, int x, int y, int scale) {
	int tilenum = SFLASKBLUE;
		
	if( netgame )
		return;
#pragma message("fix potionpic")
#if 0
	x = x + MulScale(200, scale, 16);
	y = y - MulScale(94, scale, 16);
	engine.rotatesprite(x<<16,y<<16,scale,0,SPOTIONBACKPIC,0, 0, 8 | 16, 0, 0, xdim, ydim-1);
	engine.rotatesprite((x - MulScale(4, scale, 16))<<16,(y - MulScale(7, scale, 16))<<16,scale,0,SPOTIONARROW+currentpotion,0, 0, 8 | 16, 0, 0, xdim, ydim-1);
	
	x += MulScale(4, scale, 16);
	for(int i = 0; i < MAXPOTIONS; i++) {
		if(plr.potion[i] < 0)
			plr.potion[i] = 0;
		if(plr.potion[i] > 0) {
			switch(i) {
				case 1:
					tilenum=SFLASKGREEN;
				break;
				case 2:
					tilenum=SFLASKOCHRE;
				break;
				case 3:
					tilenum=SFLASKRED;
				break;
				case 4:
					tilenum=SFLASKTAN;
				break;
			}
			potiontilenum=tilenum;

			engine.rotatesprite((x + MulScale(i*20, scale, 16))<<16,(y + MulScale(19, scale, 16))<<16,scale,0,potiontilenum,0, 0, 8 | 16, 0, 0, xdim, ydim-1);
			char potionbuf[50];
			Bitoa(plr.potion[i],potionbuf);

			fancyfont((266<<1)+(i*20),394,SPOTIONFONT-26,potionbuf,0);
			//game.getFont(3).drawText(x + MulScale(7 +(i*20), scale, 16),y+MulScale(7, scale, 16), potionbuf, scale, 0, 0, TextAlign.Left, 0, false);
		}
		else 
			engine.rotatesprite((x + MulScale(i*20, scale, 16))<<16,(y + MulScale(19, scale, 16))<<16,scale,0,SFLASKBLACK,0, 0, 8 | 16, 0, 0, xdim, ydim-1);
	}
#endif
}
#endif	

*/
