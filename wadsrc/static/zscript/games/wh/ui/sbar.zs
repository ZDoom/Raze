
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
 
 	void levelpic(WhPlayer plr) 
	 {
		 String format;
		if (plr.selectedgun == 6) 
		{
			format = String.Format("%d", plr.ammo[6]);
			DrawImage("SARROWS", (6,400 -46), DI_ITEM_RELCENTER); // GDX uses 1916 for WH2.
			drawScoreText(42, -41, format);
		}
		else if (plr.selectedgun == 7 && plr.weapon[7] == 2) 
		{
			format = String.Format("%d", plr.ammo[7]);
			DrawImage("SPIKES", (6,400 -46), DI_ITEM_RELCENTER);
			drawScoreText(42, -41, format);
		}
		else 
		{
			format = "SPLAYERLVL" .. (plr.lvl - 1); // GDX uses 1917 + lvl for WH2
			DrawImage("SPIKES", (6,400 -44), DI_ITEM_RELCENTER);
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
 
	void drawscore(WhPlayer plr) 
	{
		String format = String.Format("%d", plr.score);
		DrawImage("SSCOREBACKPIC", (6,400 -85), DI_ITEM_RELCENTER);
		drawScoreText(61,400 -81, format);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
 
	void drawarmor(WhPlayer plr) 
	{
		String format = String.Format("%d", plr.armor);

		DrawImage("SHEALTHBACK", (401, 400 -75), DI_ITEM_RELCENTER);
		drawScoreText(409, 400 - 70, format);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
 
	void drawhealth(WhPlayer plr) 
	{
		String format = String.Format("%d", plr.health);
		double alpha = 1;
		if (plr.poisoned == 1)  alpha = sin((10 * 360. / 2048.) * PlayClock) * 0.5 + 0.5;

		DrawImage("SHEALTHBACK", (320 + 171, 400 -75), DI_ITEM_RELCENTER);
		drawHealthText(320 - 167, 400 - 70, format);
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
 
	void keyspic(WhPlayer plr) 
	{
		static const String keypics[] = { "SKEYBRASS", "SKEYBLACK", "SKYGLASS", "SKEYIVORY"};
		int y = 400 - 85;
		for (int i = 0; i < 4; i++)
		{
			DrawImage(plr.treasure[Witchaven.TBRASSKEY + i]? keypics[i] : "SKEYBLANK", (320 + 180, y), DI_ITEM_RELCENTER );
			y += 22;
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------
 
	void potionpic(WhPlayer plr, int currentpotion) 
	{
		static const String potionpic[] = { "SFLASKBLUE", "SFLASKGREEN", "SFLASKOCHRE", "SFLASKRED", "SFLASKTAN"};
			
		double x = 320 + 200;
		double y = 400 - 94;

		DrawImage("SPOTIONBACKPIC", (x, y), DI_ITEM_RELCENTER);
		DrawImage("SPOTIONARROW" .. currentpotion, (x - 4, y - 7), DI_ITEM_RELCENTER);

		x += 4;
		for(int i = 0; i < Witchaven.MAXPOTIONS; i++) 
		{
			if(plr.potion[i] < 0) plr.potion[i] = 0;
			if(plr.potion[i] > 0) 
			{
				DrawImage(potionpic[i], (x + i*20, y + 19), DI_ITEM_RELCENTER);
				drawPotionText(x + 7 + i*20, y + 7, String.Format("%d", plr.potion[i]));
			}
			else  DrawImage("SFLASKBLACK", (x + i*20, y + 19), DI_ITEM_RELCENTER);
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
		drawscore(plr);
		if (netgame) 
		{
			//if (game.nNetMode == NetMode.Multiplayer) captureflagpic(scale);
			//else fragspic(plr, scale);
		} 
		else potionpic(plr, plr.currentpotion);

		levelpic(plr);
		drawhealth(plr);
		drawarmor(plr);
		keyspic(plr);
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
		int amposy = 40;//Raze.GetMessageBottomY();

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
		stats.fontscale = 1;
		stats.spacing = 7;
		stats.screenbottomspace = bottomy;
		stats.statfont = SmallFont;

		if (automapMode == am_full)
		{
			stats.letterColor = Font.TEXTCOLOR_DARKRED;
			stats.standardColor = Font.TEXTCOLOR_TAN;

			bool textfont = am_textfont;
			if (!am_textfont)
			{
				// For non-English languages force use of the text font. The tiny one is simply too small to ever add localized characters to it.
				let p = StringTable.Localize("$REQUIRED_CHARACTERS");
				if (p.length() > 0) textfont = true; 
			}

			if (!textfont)
			{
				stats.statfont = SmallFont2;
				stats.spacing = 6;
			} 
			else stats.spacing = SmallFont.GetHeight() + 1;
			PrintAutomapInfo(stats, textfont);
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
	// todo: alternative displays
	//
	//---------------------------------------------------------------------------

	override void UpdateStatusBar(SummaryInfo info)
	{
		let plr = Witchaven.GetViewPlayer();
		DrawImage("SSTATUSBAR", (0, 0), DI_ITEM_CENTER_BOTTOM | DI_SCREEN_CENTER_BOTTOM);
		updatepics(plr);
		int bottomy = tileHeight("SSTATUSBAR") * 200 / 480; //??
		DoLevelStats(bottomy, info);
	}
}

