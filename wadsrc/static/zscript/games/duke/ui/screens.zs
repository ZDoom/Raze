

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

