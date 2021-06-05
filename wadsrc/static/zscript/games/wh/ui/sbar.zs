
class WHStatusBar : RazeStatusBar
{
	TextureID healthfont[10];
	TextureID potionfont[10];
	TextureID scorefont[10];
	int displaytime;

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
 
 	override void Init()
	{
		for (int i = 0;i < 10;i++)
		{
			healthfont[i] = TexMan.CheckForTexture(String.Format("SHEALTHFONT%d", i), TexMan.TYPE_ANY);
			potionfont[i] = TexMan.CheckForTexture(String.Format("SPOTIONFONT%d", i), TexMan.TYPE_ANY);
			scorefont[i] = TexMan.CheckForTexture(String.Format("SSCOREFONT%d", i), TexMan.TYPE_ANY);
		}
		displaytime = -1;
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
	
	void drawHealthText(double x, double y, String text, int shade = 0, int pal = 0, bool shadow = true) 
	{
		for (int i = 0; i < text.Length(); i++)
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
					DrawTexture(tex, (x+1, y+1), DI_ITEM_LEFT_TOP, 0.5, col:0xff000000);
				DrawTexture(tex, (x, y), DI_ITEM_LEFT_TOP, col:Raze.shadeToLight(shade), Translation.MakeID(Translation_Remap, pal));
				let siz = TexMan.GetScaledSize(tex);
				x += siz.X;
			}

		}
	}

	void drawPotionText(double x, double y, String text, int shade = 0, int pal = 0, bool shadow = true) 
	{
		for (int i = 0; i < text.Length(); i++)
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
					DrawTexture(tex, (x+1, y+1), DI_ITEM_LEFT_TOP, 0.5, col:0xff000000);
				DrawTexture(tex, (x, y), DI_ITEM_LEFT_TOP, col:Raze.shadeToLight(shade), Translation.MakeID(Translation_Remap, pal));
				let siz = TexMan.GetScaledSize(tex);
				x += siz.X;
			}

		}
	}

	void drawScoreText(double x, double y, String text, int shade = 0, int pal = 0, bool shadow = true) 
	{
		for (int i = 0; i < text.Length(); i++)
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
					DrawTexture(tex, (x+1, y+1), DI_ITEM_LEFT_TOP, 0.5, col:0xff000000);
				DrawTexture(tex, (x, y), DI_ITEM_LEFT_TOP, col:Raze.shadeToLight(shade), Translation.MakeID(Translation_Remap, pal));
				let siz = TexMan.GetScaledSize(tex);
				x += siz.X;
			}

		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
 
 	void levelpic(WhPlayer plr, double x, double y) 
	 {
		 String format;
		if (plr.selectedgun == 6) 
		{
			format = String.Format("%d", plr.ammo[6]);
			DrawImage("SARROWS", (x, y), DI_ITEM_LEFT_TOP); // GDX uses 1916 for WH2.
			drawScoreText(x + 36, y + 5, format);
		}
		else if (plr.selectedgun == 7 && plr.weapon[7] == 2) 
		{
			format = String.Format("%d", plr.ammo[7]);
			DrawImage("SPIKES", (x, y), DI_ITEM_LEFT_TOP);
			drawScoreText(x + 36, y + 5, format);
		}
		else 
		{
			format = "SPLAYERLVL1";// .. (plr.lvl - 1); // GDX uses 1917 + lvl for WH2
			DrawImage(format, (x, y), DI_ITEM_LEFT_TOP);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
 
	void drawscore(WhPlayer plr, double x, double y) 
	{
		String format = String.Format("%d", plr.score);
		DrawImage("SSCOREBACKPIC", (x, y), DI_ITEM_LEFT_TOP);
		drawScoreText(x + 55, y + 4, format);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
 
	void drawscore2(WhPlayer plr, double x, double y) 
	{
		String format = String.Format("%d", plr.score);
		drawHealthText(x + 8, y + 5, format);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
 
	void drawarmor(WhPlayer plr, double x, double y, bool drawbg = true) 
	{
		String format = String.Format("%d", plr.armor);

		if (drawbg) DrawImage("SHEALTHBACK", (x, y), DI_ITEM_LEFT_TOP);
		drawHealthText(x + 8, y + 5, format);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
 
	void drawhealth(WhPlayer plr, double x, double y, bool drawbg = true) 
	{
		String format = String.Format("%d", plr.health);
		double alpha = 1;
		if (plr.poisoned == 1)  alpha = sin((10 * 360. / 2048.) * PlayClock) * 0.5 + 0.5;

		if (drawbg) DrawImage("SHEALTHBACK", (x, y), DI_ITEM_LEFT_TOP);
		drawHealthText(x + 4, y + 5, format, numshades - (numshades * alpha));
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
 
	void captureflagpic() 
	{
		int i;
		DrawImage("SPOTIONBACKPIC", (260, 387), DI_ITEM_RELCENTER);
		/*
		for (i = 0; i < 4; i++) {
			if( teaminplay[i] ) { XXX
			overwritesprite(((int) sflag[i].x << 1) + 6, (int) sflag[i].y + 8, STHEFLAG, 0, 0, (int) sflag[i].z);
			String format = String.Format("%d", teamscore[i],tempchar);
			fancyfont(((int) sflag[i].x << 1) + 16, (int) sflag[i].y + 16, SPOTIONFONT - 26, tempchar, 0);
			}
		}
		*/
	}

	void fragspic(WhPlayer plr) 
	{
		/*
		if (whcfg.gViewSize == 320) {

			int x = windowx2 / 2 + 200;
			int y = windowy2 - 94;
			overwritesprite(x, y, SPOTIONBACKPIC, 0, 0, 0);

//			Bitoa(teamscore[pyrn],tempchar); XXX
			game.getFont(2).drawText(x + 10, y + 10, tempchar, 0, 0, TextAlign.Left, 0, false);
		}
		*/
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
 
	void keyspic(WhPlayer plr, double x, double y) 
	{
		static const String keypics[] = { "SKEYBRASS", "SKEYBLACK", "SKEYGLASS", "SKEYIVORY"};
		for (int i = 0; i < 4; i++)
		{
			DrawImage(plr.treasure[Witchaven.TBRASSKEY + i]? keypics[i] : "SKEYBLANK", (x, y), DI_ITEM_RELCENTER );
			y += 22;
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
 
	void potionpic(WhPlayer plr, int currentpotion, double x, double y, bool drawbg = true) 
	{
		static const String potionpic[] = { "SFLASKBLUE", "SFLASKGREEN", "SFLASKOCHRE", "SFLASKRED", "SFLASKTAN"};
			
		if (drawbg) DrawImage("SPOTIONBACKPIC", (x, y), DI_ITEM_LEFT_TOP);
		DrawImage("SPOTIONARROW" .. currentpotion, (x - 4, y - 7), DI_ITEM_LEFT_TOP);

		x += 4;
		for(int i = 0; i < Witchaven.MAXPOTIONS; i++) 
		{
			if(plr.potion[i] < 0) plr.potion[i] = 0;
			if(plr.potion[i] > 0) 
			{
				DrawImage(potionpic[i], (x + i*20, y + 19), DI_ITEM_LEFT_TOP);
				drawPotionText(x + 7 + i*20, y + 7, String.Format("%d", plr.potion[i]));
			}
			else  DrawImage("SFLASKBLACK", (x + i*20, y + 19), DI_ITEM_LEFT_TOP);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
 
	void orbpic(WhPlayer plr, int currentorb) 
	{
		if (plr.orbammo[currentorb] < 0)
			plr.orbammo[currentorb] = 0;

		String format = String.Format("%d", plr.orbammo[currentorb]);

		int bookpic = plr.spellbook;
		if (plr.spellbookflip == 0)	bookpic = 8;
		let spellbookanim = plr.GetSpellBookAnim();
		if (spellbookanim != null && (plr.orbammo[plr.currentorb] > 0 || plr.currweaponfired == 4)) 
		{
			int y = 382;// was 389 originally.
			if (currentorb == 2) y = 381;
			if (currentorb == 3) y = 383;
			if (currentorb == 6) y = 383;
			if (currentorb == 7) y = 380;
			
			plr.spellbookframe = spellbookanim.daweaponframe;
			DrawImage(String.Format("#%05d", plr.spellbookframe), (320 + spellbookanim.currx, y + spellbookanim.curry), DI_ITEM_RELCENTER);
			drawScoreText(320 + 126, 439, String.Format("%d", plr.orbammo[currentorb]));
		}

		/* original placement for comparison

		let spellbookpage = sspellbookanim[currentorb][8].daweaponframe;
		DrawTexture(spellbookpage, (320 + 121, y), DI_ITEM_RELCENTER);
		*/
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
 
	void updatepics(WhPlayer plr) 
	{
		drawscore(plr, 6,480 -85);
		if (netgame) 
		{
			//if (game.nNetMode == NetMode.Multiplayer) captureflagpic(scale);
			//else fragspic(plr, scale);
		} 
		else potionpic(plr, plr.currentpotion, 320 + 200, 480 - 94);

		levelpic(plr, 6,480 -46);
		drawhealth(plr, 320 - 171, 480 -75);
		drawarmor(plr, 401, 480 -75);
		keyspic(plr, 320 + 180, 480 - 85);
		orbpic(plr, plr.currentOrb);
	}

	//---------------------------------------------------------------------------
	//
	// this is a bit different than originally due to competing screen space with
	// other elements.
	//
	//---------------------------------------------------------------------------

	void displayStatus(WhPlayer plr)
	{
		string s;
		if (plr.potion[0] == 0 && plr.health > 0 && plr.health < 21)
		{
			s = StringTable.Localize("$health critical");
		}
		else
		{
			if (displaytime > 0)
				displaytime -= Witchaven.TICSPERFRAME;

			if (displaytime <= 0) 
			{
				if (plr.manatime > 0) 
				{
					if (plr.manatime < 512) 
					{
						if ((plr.manatime % 64) > 32) 
						{
							return;
						}
					}
					s = "%FIRE RESISTANCE";
				} 
				else if (plr.poisoned == 1) 
				{
					s = "$POISONED";
				} 
				else if (plr.orbactive[5] > 0) 
				{
					if (plr.orbactive[5] < 512) 
					{
						if ((plr.orbactive[5] % 64) > 32) 
						{
							return;
						}
					}
					s = "$FLYING";
				} else if (plr.vampiretime > 0) 
				{
					s = "$ORNATE HORN";
				}
			}
		}
		if (s.length() > 0)
		{
			let siz = SmallFont.StringWidth(s);
			Screen.DrawText(SmallFont, Font.CR_NATIVEPAL, 320-siz/2, 40, s, DTA_FullscreenScale, FSMode_Fit640x400, DTA_TranslationIndex, Translation.MakeID(Translation_Remap, 7));
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void drawOverlays(WhPlayer plr)
	{
		displayStatus(plr);

		if (plr.justwarpedfx > 0)
			DrawImage("ANNIHILATE", (0,0), DI_ITEM_RELCENTER|DI_SCREEN_CENTER, scale:(plr.justwarpedcnt/128., plr.justwarpedcnt/128.));
		
		double pwpos = 0;
		
		if (plr.helmettime > 0) 
		{
			pwpos += tileHeight("Helmet") / 4;
			DrawImage("Helmet", (-20, pwpos), DI_ITEM_RELCENTER, scale:(0.25, 0.25));
			pwpos += 10;
		}
			
		if(plr.vampiretime > 0) 
		{
			pwpos += tileHeight("THEHORN") / 4;
			DrawImage("THEHORN", (-20, pwpos), DI_ITEM_RELCENTER, scale:(0.25, 0.25));
			pwpos += 10;
		}

		if(plr.orbactive[5] > 0) 
		{
			pwpos += tileHeight("SCROLLFLY") / 4;
			DrawImage("SCROLLFLY", (-20, pwpos), DI_ITEM_RELCENTER, scale:(0.25, 0.25));
			pwpos += 10;
		}
	
		if(plr.shadowtime > 0) 
		{
			pwpos += tileHeight("SCROLLSCARE") / 4;
			DrawImage("SCROLLSCARE", (-20, pwpos), DI_ITEM_RELCENTER, scale:(0.25, 0.25));
			pwpos += 10;
		}
			
		if(plr.nightglowtime > 0) 
		{
			pwpos += tileHeight("SCROLLSCARE") / 4;
			DrawImage("SCROLLSCARE", (-20, pwpos), DI_ITEM_RELCENTER, scale:(0.25, 0.25));
			pwpos += 10;
		}

		int amposx = 10;
		int amposy = Raze.GetMessageBottomY();

		if(plr.treasure[Witchaven.TONYXRING] != 0)
		{
			DrawImage("ONYXRING", (amposx, amposy), DI_ITEM_RELCENTER, scale:(0.25, 0.25));
			amposx += 20;
		}
		
		if(plr.treasure[Witchaven.TAMULETOFTHEMIST] != 0 && plr.invisibletime > 0)
		{
			DrawImage("AMULETOFTHEMIST", (amposx, amposy), DI_ITEM_RELCENTER, scale:(0.25, 0.25));
			amposx += 20;
		}
		
		if(plr.treasure[Witchaven.TADAMANTINERING] != 0)
		{
			DrawImage("ADAMANTINERING", (amposx, amposy), DI_ITEM_RELCENTER, scale:(0.25, 0.25));
			amposx += 20;
		}
		
		if(plr.treasure[Witchaven.TBLUESCEPTER] != 0)
		{
			DrawImage("BLUESCEPTER", (amposx, amposy), DI_ITEM_RELCENTER, scale:(0.25, 0.25));
			amposx += 20;
		}
		
		if(plr.treasure[Witchaven.TYELLOWSCEPTER] != 0)
		{
			DrawImage("YELLOWSCEPTER", (amposx, amposy), DI_ITEM_RELCENTER, scale:(0.25, 0.25));
			amposx += 20;
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
		stats.fontscale = Raze.isWh2()? 1. : 0.6;
		stats.spacing = stats.fontScale * SmallFont.GetHeight() + 1;
		stats.screenbottomspace = bottomy;
		stats.statfont = SmallFont;

		if (automapMode == am_full)
		{
			stats.letterColor = Font.TEXTCOLOR_DARKRED;
			stats.standardColor = Font.TEXTCOLOR_TAN;
			PrintAutomapInfo(stats, true);
		}
		// JBF 20040124: display level stats in screen corner
		else if (hud_stats && !(netgame /*|| numplayers > 1*/))
		{
			stats.letterColor = Font.TEXTCOLOR_DARKRED;
			stats.standardColor = Font.TEXTCOLOR_TAN;
			stats.completeColor = Font.TEXTCOLOR_RED;
			PrintLevelStats(stats, info);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void DrawStatusBar(WhPlayer plr, SummaryInfo info)
	{
		BeginStatusBar(false, 640, 480, tileHeight("SSTATUSBAR"));

		if (hud_size == Hud_StbarOverlay) Set43ClipRect();
		DrawImage("SSTATUSBAR", (320, 480), DI_ITEM_CENTER_BOTTOM); 		
		updatepics(plr);
		int bottomy = tileHeight("SSTATUSBAR") * 200 / 480;
		DoLevelStats(bottomy, info);
	}
	
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
 
 	void DrawHud1(WhPlayer plr, SummaryInfo info)
	{
		BeginHUD(1, false, 640, 480);
 		levelpic(plr, 3, -40);
		drawscore(plr, 3, -80);
		drawhealth(plr, 130, -75);
		drawarmor(plr, 215, -75);
		if (!netgame) potionpic(plr, plr.currentpotion, -180, -80);
		keyspic(plr, -30, -85);
		DoLevelStats(90 * 200 / 480, info);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
 
 	void DrawHud2(WhPlayer plr, SummaryInfo info)
	{
		BeginHUD(1, false, 640, 480);
		
		DrawImage("SFLASKBLUE", (4, -3), DI_ITEM_LEFT_BOTTOM);
		drawhealth(plr, 30, -50, false);

		DrawImage("CHAINMAIL", (120, -3), DI_ITEM_LEFT_BOTTOM|DI_DONTANIMATE, scale:(0.5, 0.5));
		drawarmor(plr, 170, -50, false);

		DrawImage("#00513" /*"HORNYSKULL4"*/, (260, -3), DI_ITEM_LEFT_BOTTOM|DI_DONTANIMATE, scale:(1.5, 1.5));	// something's wrong with the names...
		drawscore2(plr, 320, -50);
		
		//drawscore(plr, 260, -40);
 		levelpic(plr, -320, -40);

		if (!netgame) potionpic(plr, plr.currentpotion, -180, -80, false);
		keyspic(plr, -30, -85);
		DoLevelStats(60 * 200 / 480, info);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	override void UpdateStatusBar(SummaryInfo info)
	{
		let plr = Witchaven.GetViewPlayer();
		
		if (hud_size == Hud_Nothing)
		{
			DoLevelStats(2, info);
		}
		else if (hud_size == Hud_full)
		{
			DrawHUD2(plr, info);
		}
		else if (hud_size == Hud_Mini)
		{
			DrawHUD1(plr, info);
		}
		else
		{
			DrawStatusBar(plr, info);
		}
	}
}

