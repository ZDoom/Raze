

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

		screen.ClearScreen();

		// Only translate if the image depends on the global palette.
		let tex = TexMan.CheckForTexture("BETASCREEN"); 
		int trans = TexMan.UseGamePalette(tex)? Translation.MakeID(Translation_BasePalette, Duke.TITLEPAL) : 0;
		screen.DrawTexture(tex, true, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_TranslationIndex, trans, DTA_LegacyRenderStyle, STYLE_Normal);

		double scale = clamp(clock - 120, 0, 60) / 64.;
		if (scale > 0.)
		{
			let tex = TexMan.CheckForTexture("DUKENUKEM"); 
			trans = TexMan.UseGamePalette(tex)? Translation.MakeID(Translation_BasePalette, Duke.TITLEPAL) : 0; // re-check for different texture!

			screen.DrawTexture(tex, true, 160, 104, DTA_FullscreenScale, FSMode_Fit320x200,
				DTA_CenterOffsetRel, true, DTA_TranslationIndex, trans, DTA_ScaleX, scale, DTA_ScaleY, scale);
		}

		scale = clamp(clock - 220, 0, 30) / 32.;
		if (scale > 0.)
		{
			let tex = TexMan.CheckForTexture("THREEDEE"); 
			trans = TexMan.UseGamePalette(tex)? Translation.MakeID(Translation_BasePalette, Duke.TITLEPAL) : 0; // re-check for different texture!

			screen.DrawTexture(tex, true, 160, 129, DTA_FullscreenScale, FSMode_Fit320x200,
				DTA_CenterOffsetRel, true, DTA_TranslationIndex, trans, DTA_ScaleX, scale, DTA_ScaleY, scale);
		}

		if (Build.isPlutoPak()) 
		{
			scale = (410 - clamp(clock, 280, 395)) / 16.;
			if (scale > 0. && clock > 280)
			{
				let tex = TexMan.CheckForTexture("TITLEPLUTOPAKSPRITE"); 
				trans = TexMan.UseGamePalette(tex)? Translation.MakeID(Translation_BasePalette, Duke.TITLEPAL) : 0; // re-check for different texture!

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
