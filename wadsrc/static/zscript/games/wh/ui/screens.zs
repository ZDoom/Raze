
class WHSummaryScreen : SkippableScreenJob
{
	ScreenJob Init(MapRecord mr, SummaryInfo info) 
	{
		Super.Init(fadein|fadeout);
		SetParameters(mr, info); 
				
		//plr.score += bonus;
	}

	void drawText(int x, int y, String text)
	{
		Screen.DrawText(SmallFont, Font.CR_UNTRANSLATED, x, y, text, DTA_FullscreenScale, FSMode_Fit320x200);
	}

	override void Start()
	{
		Witchaven.PlaySound(WhSnd.S_CHAINDOOR1);
	}

	override void Draw()
	{
		static const String ratings[] = { "$txtw_poor", "$txtw_average", "$txtw_good", "$txtw_perfect" };

		Screen.DrawTexture(TexMan.CheckForTexture("VMAINBLANK", TexMan.Type_Any), 0, 0, DTA_Fullscreen, FSMode_ScaleToFit43);

		drawText(10, 13, currentLevel.DisplayName());
		drawText(10, 31, "$Level conquered");

		drawText(10, 64, "$Enemies killed");
		drawText(160 + 48 + 14, 64, String.Format("%d %s %d", stats.Kills, StringTable.Localize("$TXT_OF"), stats.MaxKills));

		drawText(10, 64 + 18, "$Treasures found");
		drawText(160 + 48 + 14, 64 + 18, String.Format("%d %s %d", stats.Secrets, "$TXT_OF", stats.MaxSecrets));

		drawText(10, 64 + 2 * 18, GStrings("$Experience gained"));
		drawText(160 + 48 + 14, 64 + 2 * 18, String.Format("%d", stats.score));

		drawText(10, 64 + 3 * 18, "$Rating");
		drawText(160 + 48 + 14, 64 + 3 * 18, ratings[stats.Supersecrets]);

		drawText(10, 64 + 4 * 18, "$TXT_Bonus");
		drawText(160 + 48 + 14, 64 + 4 * 18, String.Format("%d", bonus));
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class WHLoadScreen : ScreenJob
{
	MapRecord rec;
	
	ScreenJob Init(MapRecord maprec)
	{
		Super.Init(fadein);
		rec = maprec;
		return self;
	}

	override void OnTick()
	{
		if (fadestate == visible) jobstate = finished;
	}
	 	
	void drawText(int x, int y, const char* text)
	{
		Screen.DrawText(SmallFont, Font.CR_UNTRANSLATED, x - SmallFont->StringWidth(text)/2, y, text, DTA_FullscreenScale, FSMode_Fit320x200);
	}

	override void Draw()
	{
		Screen.DrawTexture(TexMan.CheckForTexture("MAINMENU", TexMan.Type_Any), 0, 0, DTA_Fullscreen, FSMode_ScaleToFit43);
		drawText(160, 100, "$TXT_LOADING");
		drawText(160, 114, "$TXTB_PLSWAIT");
		return 0;
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class VictoryScreen : ImageScreen
{
	int sound;

	ScreenJob Init(String tex, int snd)
	{
		sound = snd;
		Super.InitNamed(tex, fadein | fadeout, 0x7fffffff, 0);
		return self;
	}

	override void Start()
	{
		if (sound > 0) Witchaven.PlaySound(sound);
	}
}
 
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class WHCutscenes
{

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static void BuildIntro(ScreenJobRunner runner)
    {
		Mus_Stop();
		FX_StopAllSounds();

		if (!userConfig.nologo)
		{
			runner.Append(MoviePlayerJob.Create(gameinfo.gameType & GAMEFLAG_WH2? "smk/intro.smk" : "intro.smk", 0));
		}
	}

	static void BuildWH2Ending(ScreenJobRunner runner)
    {
		Mus_Stop();
		FX_StopAllSounds();

		if (!userConfig.nologo)
		{
			runner.Append(MoviePlayerJob.Create(g"smk/ending1.smk", 0));
			runner.Append(MoviePlayerJob.Create(g"smk/ending2.smk", 0));
			runner.Append(MoviePlayerJob.Create(g"smk/ending3.smk", 0));
		}
	}

	static void BuildVictoryScreen(ScreenJobRunner runner)
	{
		runner.Append(new("VictoryScreen").Init("VICTORYA", -1));
		runner.Append(new("VictoryScreen").Init("VICTORYB", WhSnd.S_DROPFLAG));
		runner.Append(new("VictoryScreen").Init("VICTORYC", WhSnd.S_WISP2));
	}

	static void BuildLoading(ScreenJobRunner runner, MapRecord map)
	{
		runner.Append(new("WhLoadScreen").Init(map));
	}
}