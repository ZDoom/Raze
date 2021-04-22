

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DRealmsScreen : SkippableScreenJob
{
	void Init()
	{
		Super.Init(fadein | fadeout);
	}

	override void Start()
	{
		Duke.PlaySpecialMusic(Duke.MUS_INTRO);
	}

	override void OnTick()
	{
		if (ticks >= 7 * GameTicRate) jobstate = finished;
	}

	override void Draw(double smoothratio)
	{
		let tex = TexMan.CheckForTexture("DREALMS"); 
		int translation = TexMan.UseGamePalette(tex)? Translation.MakeID(Translation_BasePalette, Duke.DREALMSPAL) : 0;

		screen.ClearScreen();
		screen.DrawTexture(tex, true, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_TranslationIndex, translation, DTA_LegacyRenderStyle, STYLE_Normal);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class TitleScreen : SkippableScreenJob
{
	int soundanm;

	void Init()
	{
		Super.Init(fadein | fadeout);
		soundanm = 0;
	}

	override void Start()
	{
		if (Build.isNam() || userConfig.nologo) Duke.PlaySpecialMusic(Duke.MUS_INTRO);
	}

	override void OnTick() 
	{
		int clock = ticks * 120 / GameTicRate;
		if (soundanm == 0 && clock >= 120 && clock < 120 + 60)
		{
			soundanm = 1;
			Duke.PlaySound(DukeSnd.PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
		}
		if (soundanm == 1 && clock > 220 && clock < (220 + 30))
		{
			soundanm = 2;
			Duke.PlaySound(DukeSnd.PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
		}
		if (soundanm == 2 && clock >= 280 && clock < 395)
		{
			soundanm = 3;
			if (Build.isPlutoPak()) Duke.PlaySound(DukeSnd.FLY_BY, CHAN_AUTO, CHANF_UI);
		}
		else if (soundanm == 3 && clock >= 395)
		{
			soundanm = 4;
			if (Build.isPlutoPak()) Duke.PlaySound(DukeSnd.PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
		}

		if (clock > (860 + 120))
		{
			jobstate = finished;
		}
	}

	override void Draw(double smoothratio)
	{
		int clock = (ticks + smoothratio) * 120 / GameTicRate;
		int etrans = Translation.MakeID(Translation_BasePalette, Duke.TITLEPAL);

		screen.ClearScreen();

		// Only translate if the image depends on the global palette.
		let tex = TexMan.CheckForTexture("BETASCREEN"); 
		int trans = TexMan.UseGamePalette(tex)? etrans : 0;
		screen.DrawTexture(tex, true, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_TranslationIndex, trans, DTA_LegacyRenderStyle, STYLE_Normal);

		double scale = clamp(clock - 120, 0, 60) / 64.;
		if (scale > 0.)
		{
			let tex = TexMan.CheckForTexture("DUKENUKEM"); 
			trans = TexMan.UseGamePalette(tex)? etrans : 0; // re-check for different texture!

			screen.DrawTexture(tex, true, 160, 104, DTA_FullscreenScale, FSMode_Fit320x200,
				DTA_CenterOffsetRel, true, DTA_TranslationIndex, trans, DTA_ScaleX, scale, DTA_ScaleY, scale);
		}

		scale = clamp(clock - 220, 0, 30) / 32.;
		if (scale > 0.)
		{
			let tex = TexMan.CheckForTexture("THREEDEE"); 
			trans = TexMan.UseGamePalette(tex)? etrans : 0; // re-check for different texture!

			screen.DrawTexture(tex, true, 160, 129, DTA_FullscreenScale, FSMode_Fit320x200,
				DTA_CenterOffsetRel, true, DTA_TranslationIndex, trans, DTA_ScaleX, scale, DTA_ScaleY, scale);
		}

		if (Build.isPlutoPak()) 
		{
			scale = (410 - clamp(clock, 280, 395)) / 16.;
			if (scale > 0. && clock > 280)
			{
				let tex = TexMan.CheckForTexture("TITLEPLUTOPAKSPRITE"); 
				trans = TexMan.UseGamePalette(tex)? etrans : 0; // re-check for different texture!

				screen.DrawTexture(tex, true, 160, 151, DTA_FullscreenScale, FSMode_Fit320x200,
					DTA_CenterOffsetRel, true, DTA_TranslationIndex, trans, DTA_ScaleX, scale, DTA_ScaleY, scale);
			}
		}
	}

	override void OnDestroy()
	{
		Duke.PlaySound(DukeSnd.NITEVISION_ONOFF, CHAN_AUTO, CHANF_UI);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class Episode1End1 : SkippableScreenJob
{
	int bonuscnt;
	TextureID bossani; 
	TextureID breatheani;
	bool breathebg;

	const breathe_x = 176;
	const breathe_y = 59;
	const boss_x = 86;
	const boss_y = 59;

	void Init()
	{
		bonuscnt = 0;
		breathebg = false;
		bossani.SetInvalid();
		breatheani.SetInvalid();
		Super.Init(fadein | fadeout);
	}
	

	override void OnTick()
	{
		static const int breathe_time[] = { 0, 30, 60, 90 };
		static const int breathe_time2[] = { 30, 60, 90, 120 };
		static const String breathe_tile[] = { "VICTORY2", "VICTORY3", "VICTORY2", "" };

		static const int boss_time[] = { 0, 220, 260, 290, 320, 350, 350 };
		static const int boss_time2[] = { 120, 260, 290, 320, 350, 380, 380 };
		static const String boss_tile[] = { "VICTORY4", "VICTORY5", "VICTORY6", "VICTORY7", "VICTORY8", "VICTORY9", "VICTORY9" };
		
		int currentclock = ticks * 120 / GameTicRate;

		bossani.SetInvalid();
		breathebg = false;
		breatheani.SetInvalid();

		// boss
		if (currentclock > 390 && currentclock < 780)
		{
			for (int t = 0, tt = 0; t < 35; t +=5, tt++) if ((currentclock % 390) > boss_time[tt] && (currentclock % 390) <= boss_time2[tt])
			{
				if (t == 10 && bonuscnt == 1)
				{
					Duke.PlaySound(DukeSnd.SHOTGUN_FIRE, CHAN_AUTO, CHANF_UI);
					Duke.PlaySound(DukeSnd.SQUISHED, CHAN_AUTO, CHANF_UI);
					bonuscnt++;
				}
				bossani = TexMan.CheckForTexture(boss_tile[tt]);
			}
		}

		// Breathe
		if (currentclock < 450 || currentclock >= 750)
		{
			if (currentclock >= 750)
			{
				breathebg = true;
				if (currentclock >= 750 && bonuscnt == 2)
				{
					Duke.PlaySound(DukeSnd.DUKETALKTOBOSS, CHAN_AUTO, CHANF_UI);
					bonuscnt++;
				}
			}
			for (int t = 0, tt = 0; t < 20; t += 5, tt++)
				if (breathe_tile[tt] != "" && (currentclock % 120) > breathe_time[tt] && (currentclock % 120) <= breathe_time2[tt])
				{
					if (t == 5 && bonuscnt == 0)
					{
						Duke.PlaySound(DukeSnd.BOSSTALKTODUKE, CHAN_AUTO, CHANF_UI);
						bonuscnt++;
					}
					breatheani = TexMan.CheckForTexture(breathe_tile[tt]);
				}
		}

	}

	override void Draw(double sr)
	{
		int etrans = Translation.MakeID(Translation_BasePalette, Duke.ENDINGPAL);

		screen.ClearScreen();
		let tex = TexMan.CheckForTexture("VICTORY1");
		int trans = TexMan.UseGamePalette(tex)? etrans : 0;
		screen.DrawTexture(tex, false, 0, 50, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TranslationIndex, trans, DTA_LegacyRenderStyle, STYLE_Normal, DTA_TopLeft, true);

		if (bossani.isValid())
		{
			trans = TexMan.UseGamePalette(tex)? etrans : 0; // re-check for different texture!
			screen.DrawTexture(bossani, false, boss_x, boss_y, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TranslationIndex, trans, DTA_TopLeft, true);
		}

		if (breathebg)
		{
			tex = TexMan.CheckForTexture("VICTORY9");
			trans = TexMan.UseGamePalette(tex)? etrans : 0; // re-check for different texture!
			screen.DrawTexture(tex, false, 86, 59, DTA_FullscreenScale, FSMode_Fit320x200,	DTA_TranslationIndex, trans, DTA_TopLeft, true);
		}

		if (breatheani.isValid())
		{
			trans = TexMan.UseGamePalette(tex)? etrans : 0; // re-check for different texture!
			screen.DrawTexture(breatheani, false, breathe_x, breathe_y, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TranslationIndex, trans, DTA_TopLeft, true);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class E2EndScreen : ImageScreen
{
	void Init()
	{
		Super.Init("E2ENDSCREEN", fadein | fadeout | stopsound, 0x7fffffff, 0);
	}

	override void Start()
	{
		Duke.PlaySound(DukeSnd.PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
	}
}
