//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment
Copyright (C) 2019-2021 Christoph Oelckers

This file is part of Raze

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


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class SWDRealmsScreen : SkippableScreenJob
{
	const DREALMSPAL = 1;

	ScreenJob Init()
	{
		Super.Init(fadein | fadeout);
		return self;
	}

	override void Start()
	{
		SW.PlaySong(0);
	}

	override void OnTick()
	{
		if (ticks > 5 * GameTicRate) jobstate = finished;
	}

	override void Draw(double sm)
	{
		let tex = TexMan.CheckForTexture("THREED_REALMS_PIC", TexMan.Type_Any);
		int translation = TexMan.UseGamePalette(tex) ? Translation.MakeID(Translation_BasePalette, DREALMSPAL) : 0;
		Screen.DrawTexture(tex, false, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_TranslationIndex, translation, DTA_LegacyRenderStyle, STYLE_Normal);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class SWCreditsScreen : SkippableScreenJob
{
	int mystate;
	int starttime;
	TextureID curpic;
	TextureID pic1, pic2;

	ScreenJob Init()
	{
		Super.Init(fadein|fadeout);
		pic1 = TexMan.CheckForTexture("CREDITS1", TexMan.Type_Any);
		pic2 = TexMan.CheckForTexture("CREDITS2", TexMan.Type_Any);
		return self;

	}
	override void OnSkip() 
	{
		SW.StopSound();
	}

	override void Start() 
	{
		// Lo Wang feel like singing!
		SW.PlaySound(SWSnd.DIGI_JG95012,  SW.v3df_none, CHAN_VOICE, CHANF_UI);
	}

	override void OnTick()
	{
		if (mystate == 0)
		{
			if (!SW.IsSoundPlaying(CHAN_VOICE))
			{
				starttime = ticks;
				mystate = 1;
				SW.StopSound();
				curpic = pic1;
				SW.PlaySong(5); //) PlaySong(2);
			}
		}
		else
		{
			if (ticks >= starttime + 8 * GameTicRate)
			{
				curpic = curpic == pic1? pic2 : pic1;
				starttime = ticks;
			}
		}
	}

	override void Draw(double sr)
	{
		if (mystate == 1)
			Screen.DrawTexture(curpic, false, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal);
	}
}

//---------------------------------------------------------------------------
//
// Summary screen animation
//
//---------------------------------------------------------------------------

struct SWSummaryAnimation
{
	TextureID frames[15];
	Sound snd;
	int delay, length;
	int tics;

	int curframe;

	void Init(String nametemplate, int length_, Sound sound_, int delay_)
	{
		for (int i = 0; i < length_; i++)
		{
			String name = String.Format(nametemplate, i);
			frames[i] = TexMan.CheckForTexture(name, TexMan.Type_Any);
		}
		snd = sound_;
		delay = delay_;
		length = length_;
	}

	bool Tick()
	{
		tics += 3;
		if (curframe < length-1)
		{
			if (tics >= delay)
			{
				tics -= delay;
				curframe++;
				if (curframe == 3) SW.PlaySound(snd,  SW.v3df_none);
			}
			return false;
		}
		return tics >= 90;
	}

	TextureID getTex()
	{
		return frames[curframe];
	}
}

//---------------------------------------------------------------------------
//
// Summary screen
//
//---------------------------------------------------------------------------

class SWSummaryScreen : SummaryScreenBase
{
	SWSummaryAnimation anim;
	int animstate;
	TextureID rest[4];

	ScreenJob Init(MapRecord mr, SummaryInfo info)
	{
		Super.Init(fadein|fadeout);
		SetParameters(mr, info);
		switch (random(0, 2))
		{
			case 0:
				anim.Init("BONUS_PUNCH%02d", 15, SWSnd.DIGI_PLAYERYELL3, 8);
				break;
			case 1:
				anim.Init("BONUS_KICK%02d", 15, SWSnd.DIGI_PLAYERYELL2, 8);
				break;
			case 2:
				anim.Init("BONUS_GRAB%02d", 15, SWSnd.DIGI_BONUS_GRAB, 20);
				break;
		}
		rest[0] = TexMan.CheckForTexture("BONUS_PUNCH00", TexMan.Type_Any);
		rest[3] = rest[1] = TexMan.CheckForTexture("BONUS_PUNCH01", TexMan.Type_Any);
		rest[2] = TexMan.CheckForTexture("BONUS_PUNCH02", TexMan.Type_Any);
		return self;
	}

	override bool OnEvent(InputEvent ev)
	{
		if (ev.type == InputEvent.Type_KeyDown && !System.specialKeyEvent(ev)) 
		{
			if (animstate == 0) animstate = 1;
		}
		return true;
	}

	override void Start()
	{
		System.StopAllSounds();
		SW.PlaySong(1);
	}

	override void OnTick()
	{
		if (animstate == 1)
		{
			if (anim.Tick())
			{
				SW.StopSound();
				jobstate = finished;
			}
		}
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	private static int BONUS_LINE(int i) { return (50 + ((i) * 20)); }

	override void Draw(double sm)
	{
		Screen.DrawTexture(TexMan.CheckForTexture("BONUS_SCREEN_PIC", TexMan.Type_Any), true, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal);
		SW.DrawString(160, 20, currentLevel.DisplayName(), 1, 19, 0);
		SW.DrawString(170, 30, "$COMPLETED", 1, 19, 0);

		Textureid animtex;
		if (animstate == 0) animtex = rest[(ticks / 17) & 3];
		else animtex = anim.getTex();

		Screen.DrawTexture(animtex, false, 158, 86, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true, DTA_LegacyRenderStyle, STYLE_Normal);

		int line = 0;
		String ds;

		ds = String.Format("%s  %s", StringTable.Localize("$TXT_YOURTIME"), FormatTime(stats.time));
		SW.DrawString(60, BONUS_LINE(line++), ds, 1, 16);

		if (currentLevel.designerTime > 0)
		{
			ds = String.Format("%s  %d:%02d", StringTable.Localize("$TXT_3DRTIME"), currentLevel.designerTime / 60, currentLevel.designerTime % 60);
			SW.DrawString(40, BONUS_LINE(line++), ds, 1, 16);
		}

		if (currentLevel.parTime > 0)
		{
			ds = String.Format("%s  %d:%02d", StringTable.Localize("$TXT_PARTIME"), currentLevel.parTime / 60, currentLevel.parTime % 60);
			SW.DrawString(40, BONUS_LINE(line++), ds, 1, 16);
		}

		// always read secrets and kills from the first player
		ds = String.Format("%s:  %d / %d", StringTable.Localize("$TXT_SECRETS"), stats.Secrets, stats.MaxSecrets);
		SW.DrawString(60, BONUS_LINE(line++), ds, 1, 16);

			ds = String.Format("%s:  %d / %d", StringTable.Localize("$KILLS"), stats.Kills, stats.MaxKills);
		SW.DrawString(60, BONUS_LINE(line), ds, 1, 16);

		SW.DrawString(160, 185, "$PRESSKEY", 1, 19, 0);
	}

}

//---------------------------------------------------------------------------
//
// Deathmatch summary screen
//
//---------------------------------------------------------------------------

class SWMultiSummaryScreen : SkippableScreenJob
{
	enum EConst
	{

		STAT_START_X = 20,
		STAT_START_Y = 85,
		STAT_OFF_Y = 9,
		STAT_HEADER_Y = 14,
		STAT_TABLE_X = (STAT_START_X + 15*4),
		STAT_TABLE_XOFF = 6*4,
		MAXPLAYERS = 8,
		PALETTE_PLAYER0 = 16

	}

	int numplayers;

	ScreenJob Init(int numplayer_)
	{
		numplayers = numplayer_;
		Super.Init(fadein | fadeout);
		return self;
	}

	override void Start()
	{
		SW.PlaySong(1);
	}

	override void OnSkip()
	{
		SW.StopSound();
	}

	override void Draw(double sr)
	{
		Screen.DrawTexture(TexMan.CheckForTexture("STAT_SCREEN_PIC", TexMan.Type_Any), true, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal);
		Raze.DrawScoreboard(60);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class SWLoadScreen : ScreenJob
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

	override void Draw(double sr)
	{
		Screen.DrawTexture(TexMan.CheckForTexture("TITLE_PIC", TexMan.Type_Any), true, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal);
		SW.DrawString(160, 170, "$TXT_ENTERING", 1, 16, 0);
		SW.DrawString(160, 180, rec.DisplayName(), 1, 16, 0);
	}
}


class SWCutscenes ui
{
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static void BuildIntro(ScreenJobRunner runner)
	{
		if (!userConfig.nologo)
		{
			SW.StopSound();
			SW.PlaySong(0);
			Array<int> soundinfo;
			soundinfo.Pushv(
				1, SWSnd.DIGI_NOMESSWITHWANG,
				5, SWSnd.DIGI_INTRO_SLASH,
				15, SWSnd.DIGI_INTRO_WHIRL);
			runner.Append(new("SWDRealmsScreen").Init());
			runner.Append(MoviePlayerJob.CreateWithSoundinfo("sw.anm", soundinfo, MoviePlayer.NOSOUNDCUTOFF, 8, 360, 128)); 
		}
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static void BuildSerpentAnim(ScreenJobRunner runner)
	{
		Array<int> soundinfo;
		soundinfo.Pushv(
			1, SWSnd.DIGI_SERPTAUNTWANG,
			16, SWSnd.DIGI_SHAREND_TELEPORT,
			35, SWSnd.DIGI_WANGTAUNTSERP1,
			51, SWSnd.DIGI_SHAREND_UGLY1,
			64, SWSnd.DIGI_SHAREND_UGLY2);
		runner.Append(MoviePlayerJob.CreateWithSoundinfo("swend.anm", soundinfo, MoviePlayer.NOSOUNDCUTOFF, 16, 16, 140)); 
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static void BuildSumoAnim(ScreenJobRunner runner)
	{
		Array<int> soundinfo;
		soundinfo.Pushv(
			2, SWSnd.DIGI_JG41012,
			30, SWSnd.DIGI_HOTHEADSWITCH,
			42, SWSnd.DIGI_HOTHEADSWITCH,
			59, SWSnd.DIGI_JG41028);
		runner.Append(MoviePlayerJob.CreateWithSoundinfo("sumocinm.anm", soundinfo, MoviePlayer.NOSOUNDCUTOFF, 10, 40, 130)); 
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static void BuildZillaAnim(ScreenJobRunner runner)
	{
		Array<int> soundinfo;
		soundinfo.Pushv(
			1, SWSnd.DIGI_ZC1,
			5, SWSnd.DIGI_JG94024,
			14, SWSnd.DIGI_ZC2,
			30, SWSnd.DIGI_ZC3,
			32, SWSnd.DIGI_ZC4,
			37, SWSnd.DIGI_ZC5,
			63, SWSnd.DIGI_Z16043,
			63, SWSnd.DIGI_ZC6,
			63, SWSnd.DIGI_ZC7,
			72, SWSnd.DIGI_ZC7,
			73, SWSnd.DIGI_ZC4,
			77, SWSnd.DIGI_ZC5,
			87, SWSnd.DIGI_ZC8,
			103, SWSnd.DIGI_ZC7,
			108, SWSnd.DIGI_ZC9,
			120, SWSnd.DIGI_JG94039);
		runner.Append(MoviePlayerJob.CreateWithSoundinfo("zfcin.anm", soundinfo, MoviePlayer.NOSOUNDCUTOFF, 16, 16, 140)); 
		runner.Append(new("SWCreditsScreen").Init());
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	static void BuildSybexScreen(ScreenJobRunner runner)
	{
		if (Raze.isShareware() && !netgame)
			runner.Append(ImageScreen.CreateNamed("#05261", TexMan.Type_Any));
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	static void BuildMPSummary(ScreenJobRunner runner, MapRecord map, SummaryInfo stats)
	{
		runner.Append(new("SWMultiSummaryScreen").Init(stats.playercount));
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	static void BuildSPSummary(ScreenJobRunner runner, MapRecord map, SummaryInfo stats)
	{
		runner.Append(new("SWSummaryScreen").Init(map, stats));
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	static void BuildLoading(ScreenJobRunner runner, MapRecord map)
	{
		runner.Append(new("SWLoadScreen").Init(map));
	} 
}
