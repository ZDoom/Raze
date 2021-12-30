//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2020-2021 Christoph Oelckers

This file is part of Raze.

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
aint with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
( not much left of the original code, though... ;) )
*/
//-------------------------------------------------------------------------



//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DRealmsScreen : SkippableScreenJob
{
	ScreenJob Init()
	{
		Super.Init(fadein | fadeout);
		return self;
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

		screen.DrawTexture(tex, true, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_TranslationIndex, translation, DTA_LegacyRenderStyle, STYLE_Normal);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeTitleScreen : SkippableScreenJob
{
	int soundanm;

	ScreenJob Init()
	{
		Super.Init(fadein | fadeout);
		soundanm = 0;
		return self;
	}

	override void Start()
	{
		if (Raze.isNam() || userConfig.nologo) Duke.PlaySpecialMusic(Duke.MUS_INTRO);
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
			if (Raze.isPlutoPak()) Duke.PlaySound(DukeSnd.FLY_BY, CHAN_AUTO, CHANF_UI);
		}
		else if (soundanm == 3 && clock >= 395)
		{
			soundanm = 4;
			if (Raze.isPlutoPak()) Duke.PlaySound(DukeSnd.PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
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

		if (Raze.isPlutoPak()) 
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

	ScreenJob Init()
	{
		bonuscnt = 0;
		breathebg = false;
		bossani.SetInvalid();
		breatheani.SetInvalid();
		Super.Init(fadein | fadeout);
		return self;
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
	ScreenJob Init()
	{
		Super.InitNamed("E2ENDSCREEN", fadein | fadeout | stopsound, 0x7fffffff, 0);
		return self;
	}

	override void Start()
	{
		Duke.PlaySound(DukeSnd.PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class Episode3End : ImageScreen
{
	int soundstate;
	int finishtime;

	ScreenJob Init()
	{
		Super.InitNamed("radlogo.anm", fadein|fadeout, 0x7fffffff);
		soundstate = 0;
		finishtime = 0;
		return self;
	}

	override void OnSkip()
	{
		System.StopAllSounds();
	}

	override void OnTick()
	{
		switch (soundstate)
		{
		case 0:
			Duke.PlaySound(DukeSnd.ENDSEQVOL3SND5, CHAN_AUTO, CHANF_UI);
			soundstate++;
			break;

		case 1:
			if (!Duke.CheckSoundPlaying(DukeSnd.ENDSEQVOL3SND5))
			{
				Duke.PlaySound(DukeSnd.ENDSEQVOL3SND6, CHAN_AUTO, CHANF_UI);
				soundstate++;
			}
			break;

		case 2:
			if (!Duke.CheckSoundPlaying(DukeSnd.ENDSEQVOL3SND6))
			{
				Duke.PlaySound(DukeSnd.ENDSEQVOL3SND7, CHAN_AUTO, CHANF_UI);
				soundstate++;
			}
			break;

		case 3:
			if (!Duke.CheckSoundPlaying(DukeSnd.ENDSEQVOL3SND7))
			{
				Duke.PlaySound(DukeSnd.ENDSEQVOL3SND8, CHAN_AUTO, CHANF_UI);
				soundstate++;
			}
			break;

		case 4:
			if (!Duke.CheckSoundPlaying(DukeSnd.ENDSEQVOL3SND8))
			{
				Duke.PlaySound(DukeSnd.ENDSEQVOL3SND9, CHAN_AUTO, CHANF_UI);
				soundstate++;
			}
			break;

		case 5:
			if (!Duke.CheckSoundPlaying(DukeSnd.ENDSEQVOL3SND9))
			{
				soundstate++;
				finishtime = ticks + GameTicRate * (System.SoundEnabled() ? 1 : 5);	// if sound is off this wouldn't wait without a longer delay here.
			}
			break;

		case 6:
			if (Raze.isPlutoPak())
			{
				if (ticks > finishtime) jobstate = finished;
			}
			break;

		default:
			break;
		}
		if (jobstate != running) System.StopAllSounds();
	}

	override void OnDestroy()
	{
		if (!Raze.isPlutoPak()) Duke.PlaySound(DukeSnd.ENDSEQVOL3SND4, CHAN_AUTO, CHANF_UI);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class Episode4Text : SkippableScreenJob
{
	ScreenJob Init()
	{
		Super.Init(fadein|fadeout);
		return self;
	}


	override void Draw(double sm)
	{
		Duke.BigText(160, 60, "$Thanks to all our", 0);
		Duke.BigText(160, 60 + 16, "$fans for giving", 0);
		Duke.BigText(160, 60 + 16 + 16, "$us big heads.", 0);
		Duke.BigText(160, 70 + 16 + 16 + 16, "$Look for a Duke Nukem 3D", 0);
		Duke.BigText(160, 70 + 16 + 16 + 16 + 16, "$sequel soon.", 0);
	}

	override void Start()
	{
		Duke.PlaySound(DukeSnd.ENDSEQVOL3SND4, CHAN_AUTO, CHANF_UI);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class Episode5End : ImageScreen
{
	ScreenJob Init()
	{
		Super.InitNamed("FIREFLYGROWEFFECT", fadein|fadeout|stopsound);
		return self;
	}

	override void OnTick()
	{
		if (ticks == 1) Duke.PlaySound(DukeSnd.E5L7_DUKE_QUIT_YOU, CHAN_AUTO, CHANF_UI);
	}
}

//---------------------------------------------------------------------------
//
// This handles both Duke and RR.
//
//---------------------------------------------------------------------------

class DukeMultiplayerBonusScreen : SkippableScreenJob
{
	int playerswhenstarted;

	ScreenJob Init(int pws)
	{
		Super.Init(fadein|fadeout);
		playerswhenstarted = pws;
		return self;
	}

	override void Start()
	{
		if (!Raze.isRR()) Duke.PlayBonusMusic();
	}

	override void Draw(double smoothratio)
	{
		bool isRR = Raze.isRR();
		double titlescale = isRR? 0.36 : 1;

		String tempbuf;
		int currentclock = int((ticks + smoothratio) * 120 / GameTicRate);
		Screen.DrawTexture(TexMan.CheckForTexture("MENUSCREEN"), false, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_Color, 0xff808080, DTA_LegacyRenderStyle, STYLE_Normal);
		Screen.DrawTexture(TexMan.CheckForTexture("INGAMEDUKETHREEDEE"), true, 160, 34, DTA_FullscreenScale, FSMode_Fit320x200, DTA_CenterOffsetRel, true, DTA_ScaleX, titlescale, DTA_ScaleY, titlescale);
		if (Raze.isPlutoPak()) Screen.DrawTexture(TexMan.CheckForTexture("MENUPLUTOPAKSPRITE"), true, 260, 36, DTA_FullscreenScale, FSMode_Fit320x200, DTA_CenterOffsetRel, true);

		Raze.DrawScoreboard(60);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeLevelSummaryScreen : SummaryScreenBase
{
	String lastmapname;
	int speech;
	int displaystate;
	int dukeAnimStart;

	TextureID texBg;
	TextureID texOv[4];

	enum EScreenFlags
	{
		printTimeText = 1,
		printTimeVal = 2,
		printKillsText = 4,
		printKillsVal = 8,
		printSecretsText = 16,
		printSecretsVal = 32,
		printStatsAll = 63,
		dukeAnim = 64,
		dukeWait = 128,

	}

	ScreenJob Init(MapRecord m, SummaryInfo s)
	{
		Super.Init(fadein | fadeout);
		SetParameters(m, s);
		String basetex = level.InterBackground;
		if (basetex.length() == 0)
		{
			let cluster = level.GetCluster();
			if (cluster != null) basetex = cluster.InterBackground;
		}
		if (basetex.length() == 0) basetex = "BONUSSCREEN";
		texBg = TexMan.CheckForTexture(basetex);
		for(int i = 0; i < 4; i++)
		{
			String otex = String.Format("%s_O%d", basetex, i+1);
			texOv[i] = TexMan.CheckForTexture(otex);
		}
		lastmapname = level.DisplayName();
		speech = -1;
		displaystate = 0;
		return self;
	}

	override bool OnEvent(InputEvent ev)
	{
		if (ev.type == InputEvent.Type_KeyDown && !System.specialKeyEvent(ev))
		{
			if ((displaystate & printStatsAll) != printStatsAll)
			{
				Duke.PlaySound(DukeSnd.PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
				displaystate = printStatsAll;
			}
			else if (!(displaystate & dukeAnim))
			{
				displaystate |= dukeAnim;
				dukeAnimStart = ticks;
				Duke.PlaySound(DukeSnd.SHOTGUN_COCK, CHAN_AUTO, CHANF_UI);
				static const int speeches[] = { DukeSnd.BONUS_SPEECH1, DukeSnd.BONUS_SPEECH2, DukeSnd.BONUS_SPEECH3, DukeSnd.BONUS_SPEECH4 };
				speech = speeches[random(0, 3)];
				Duke.PlaySound(speech, CHAN_AUTO, CHANF_UI, 1);
			}
			return true;
		}
		return false;
	}

	override void Start()
	{
		Duke.PlayBonusMusic();
	}

	override void OnTick()
	{
		if ((displaystate & printStatsAll) != printStatsAll)
		{
			if (ticks == 15 * 3)
			{
				displaystate |= printTimeText;
			}
			else if (ticks == 15 * 4)
			{
				displaystate |= printTimeVal;
				Duke.PlaySound(DukeSnd.PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
			}
			else if (ticks == 15 * 6)
			{
				displaystate |= printKillsText;
				Duke.PlaySound(DukeSnd.FLY_BY, CHAN_AUTO, CHANF_UI);
			}
			else if (ticks == 15 * 7)
			{
				displaystate |= printKillsVal;
				Duke.PlaySound(DukeSnd.PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
			}
			else if (ticks == 15 * 9)
			{
				displaystate |= printSecretsText;
			}
			else if (ticks == 15 * 10)
			{
				displaystate |= printSecretsVal;
				Duke.PlaySound(DukeSnd.PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
			}
		}
		if (displaystate & dukeAnim)
		{
			if (ticks >= dukeAnimStart + 60)
			{
				displaystate ^= dukeAnim | dukeWait;
			}
		}
		if (displaystate & dukeWait)
		{
			if (speech <= 0 || !Duke.CheckSoundPlaying(speech)) 
				jobstate = finished;
		}
	}

	void PrintTime()
	{
		String tempbuf;
		Duke.GameText(10, 59 + 9, "$TXT_YourTime", 0);
		Duke.GameText(10, 69 + 9, "$TXT_ParTime", 0);
		if (!Raze.isNamWW2GI())
			Duke.GameText(10, 79 + 9, "$TXT_3DRTIME", 0);

		if (displaystate & printTimeVal)
		{
			tempbuf = FormatTime(stats.time);
			Duke.GameText((320 >> 2) + 71, 59 + 9, tempbuf, 0);

			tempbuf = FormatTime(level.parTime);
			Duke.GameText((320 >> 2) + 71, 69 + 9, tempbuf, 0);

			if (!Raze.isNamWW2GI())
			{
				tempbuf = FormatTime(level.designerTime);
				Duke.GameText((320 >> 2) + 71, 79 + 9, tempbuf, 0);
			}
		}
	}

	void PrintKills()
	{
		String tempbuf;
		Duke.GameText(10, 94 + 9, "$TXT_EnemiesKilled", 0);
		Duke.GameText(10, 104 + 9, "$TXT_EnemiesLeft", 0);

		if (displaystate & printKillsVal)
		{
			tempbuf = String.Format("%-3d", stats.kills);
			Duke.GameText((320 >> 2) + 70, 94 + 9, tempbuf, 0);

			if (stats.maxkills < 0)
			{
				tempbuf = "$TXT_N_A";
			}
			else
			{
				tempbuf = String.Format("%-3d", max(0, stats.maxkills - stats.kills));
			}
			Duke.GameText((320 >> 2) + 70, 104 + 9, tempbuf, 0);
		}
	}

	void PrintSecrets()
	{
		String tempbuf;
		Duke.GameText(10, 119 + 9, "$TXT_SECFND", 0);
		Duke.GameText(10, 129 + 9, "$TXT_SECMISS", 0);

		if (displaystate & printSecretsVal)
		{
			tempbuf = String.Format("%-3d", stats.secrets);
			Duke.GameText((320 >> 2) + 70, 119 + 9, tempbuf, 0);
			tempbuf = String.Format("%-3d", max(0, stats.maxsecrets - stats.secrets));
			Duke.GameText((320 >> 2) + 70, 129 + 9, tempbuf, 0);
		}
	}

	override void Draw(double sr) 
	{
		Screen.DrawTexture(texBg, true, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal);

		Duke.GameText(160, 190, "$PRESSKEY", 8 - (sin(ticks * 4) * 8), 0);

		if (displaystate & printTimeText)
		{
			PrintTime();
		}
		if (displaystate & printKillsText)
		{
			PrintKills();
		}
		if (displaystate & printSecretsText)
		{
			PrintSecrets();
		}

		if (displaystate & dukeAnim)
		{
			switch (((ticks - dukeAnimStart) >> 2) % 15)
			{
				case 0:
				case 1:
				case 4:
				case 5:
					Screen.DrawTexture(texOv[2], true, 199, 31, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true);
					break;
				case 2:
				case 3:
					Screen.DrawTexture(texOv[3], true, 199, 31, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true);
					break;
			}
		}
		else if (!(displaystate & dukeWait))
		{
			switch((ticks >> 3) & 3)
			{
				case 1:
				case 3:
					Screen.DrawTexture(texOv[0], true, 199, 31, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true);
					break;
				case 2:
					Screen.DrawTexture(texOv[1], true, 199, 31, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true);
					break;
			}
		}

		if (lastmapname) Duke.BigText(160, 20 - 6, lastmapname, 0);
		Duke.BigText(160, 36 - 6, "$Completed", 0);
	}

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class RRLevelSummaryScreen : SummaryScreenBase
{
	String lastmapname;
	int speech;
	int displaystate;
	int exitSoundStart;
	TextureID texBg;

	enum EFlags
	{
		printTimeText = 1,
		printTimeVal = 2,
		printKillsText = 4,
		printKillsVal = 8,
		printSecretsText = 16,
		printSecretsVal = 32,
		printStatsAll = 63,
		exitSound = 64,
		exitWait = 128,

	}

	ScreenJob Init(MapRecord m, SummaryInfo s, bool dofadeout = true)
	{
		Super.Init(dofadeout? (fadein | fadeout) : fadein);
		SetParameters(m, s);
		String basetex = level.InterBackground;
		if (basetex.length() == 0)
		{
			let cluster = level.GetCluster();
			if (cluster != null) basetex = cluster.InterBackground;
		}
		if (basetex.length() == 0) basetex = "BONUSPIC01";

		lastmapname = level.DisplayName();
		texBg = TexMan.CheckForTexture(basetex);
		return self;
	}

	override bool OnEvent(InputEvent ev)
	{
		if (ev.type == InputEvent.Type_KeyDown && !System.specialKeyEvent(ev))
		{
			if ((displaystate & printStatsAll) != printStatsAll)
			{
				Duke.PlaySound(RRSnd.FART1, CHAN_AUTO, CHANF_UI);
				displaystate = printStatsAll;
			}
			else if (!(displaystate & exitSound))
			{
				displaystate |= exitSound;
				exitSoundStart = ticks;
				Duke.PlaySound(RRSnd.CHUG, CHAN_AUTO, CHANF_UI);
				static const int speeches[] = { RRSnd.BNS_SPCH1, RRSnd.BNS_SPCH2, RRSnd.BNS_SPCH3, RRSnd.BNS_SPCH4 };
				speech = speeches[random(0, 3)];
				Duke.PlaySound(speech, CHAN_AUTO, CHANF_UI);
			}
			return true;
		}
		return false;
	}

	override void OnTick()
	{
		if ((displaystate & printStatsAll) != printStatsAll)
		{
			if (ticks == 15 * 3)
			{
				displaystate |= printTimeText;
			}
			else if (ticks == 15 * 4)
			{
				displaystate |= printTimeVal;
				Duke.PlaySound(RRSnd.FART1, CHAN_AUTO, CHANF_UI);
			}
			else if (ticks == 15 * 6)
			{
				displaystate |= printKillsText;
			}
			else if (ticks == 15 * 7)
			{
				displaystate |= printKillsVal;
				Duke.PlaySound(RRSnd.FART1, CHAN_AUTO, CHANF_UI);
			}
			else if (ticks == 15 * 9)
			{
				displaystate |= printSecretsText;
			}
			else if (ticks == 15 * 10)
			{
				displaystate |= printSecretsVal;
				Duke.PlaySound(RRSnd.FART1, CHAN_AUTO, CHANF_UI);
			}
		}
		if (displaystate & exitSound)
		{
			if (ticks >= exitSoundStart + 60)
			{
				displaystate ^= exitSound | exitWait;
			}
		}
		if (displaystate & exitWait)
		{
			if (speech <= 0 || !Duke.CheckSoundPlaying(speech))
				jobstate = finished;
		}
	}

	void PrintTime()
	{
		String tempbuf;
		Duke.BigText(30, 48, "$TXT_YerTime", -1);
		Duke.BigText(30, 64, "$TXT_ParTime", -1);
		Duke.BigText(30, 80, "$TXT_XTRTIME", -1);

		if (displaystate & printTimeVal)
		{
			tempbuf = FormatTime(stats.time);
			Duke.BigText(191, 48, tempbuf, -1);

			tempbuf = FormatTime(level.parTime);
			Duke.BigText(191, 64, tempbuf, -1);

			tempbuf = FormatTime(level.designerTime);
			Duke.BigText(191, 80, tempbuf, -1);
		}
	}

	void PrintKills()
	{
		String tempbuf;
		Duke.BigText(30, 112, "$TXT_VarmintsKilled", -1);
		Duke.BigText(30, 128, "$TXT_VarmintsLeft", -1);

		if (displaystate & printKillsVal)
		{
			tempbuf = String.Format("%-3d", stats.kills);
			Duke.BigText(231, 112, tempbuf, -1);
			if (stats.maxkills < 0)
			{
				tempbuf = "$TXT_N_A";
			}
			else
			{
				tempbuf = String.Format("%-3d", max(0, stats.maxkills - stats.kills));
			}
			Duke.BigText(231, 128, tempbuf, -1);
		}
	}

	void PrintSecrets()
	{
		String tempbuf;
		Duke.BigText(30, 144, "$TXT_SECFND", -1);
		Duke.BigText(30, 160, "$TXT_SECMISS", -1);

		if (displaystate & printSecretsVal)
		{
			tempbuf = String.Format("%-3d", stats.secrets);
			Duke.BigText(231, 144, tempbuf, -1);
			tempbuf = String.Format("%-3d", max(0, stats.maxsecrets - stats.secrets));
			Duke.BigText(231, 160, tempbuf, -1);
		}
	}

	override void Draw(double sr)
	{
		Screen.DrawTexture(texBg, true, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal);

		Duke.BigText(80, 16, lastmapname, -1);
		Duke.BigText(15, 192, "$PRESSKEY", -1);

		if (displaystate & printTimeText)
		{
			PrintTime();
		}
		if (displaystate & printKillsText)
		{
			PrintKills();
		}
		if (displaystate & printSecretsText)
		{
			PrintSecrets();
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class RRRAEndOfGame : SkippableScreenJob
{
	ScreenJob Init()
	{
		Super.Init(fadein|fadeout);
		return self;
	}

	override void OnSkip()
	{
		Duke.StopSound(RRSnd.LN_FINAL);
	}

	override void Start()
	{
		Duke.PlaySound(RRSnd.LN_FINAL, CHAN_AUTO, CHANF_UI);
	}

	override void OnTick()
	{
		if (!Duke.CheckSoundPlaying(RRSnd.LN_FINAL) && ticks > 15 * GameTicRate) jobstate = finished; // make sure it stays, even if sound is off.
	}

	override void Draw(double sr) 
	{
		let tex = TexMan.CheckForTexture(((ticks >> 2) & 1)? "ENDGAME2" : "ENDGAME");
		Screen.DrawTexture(tex, true, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLoadScreen : ScreenJob
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
		Screen.DrawTexture(TexMan.CheckForTexture("LOADSCREEN"), false, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal);

		if (!Raze.IsRR())
		{
			Duke.BigText(160, 90, (rec.flags & MapRecord.USERMAP)? "$TXT_LOADUM" : "$TXT_LOADING", 0);
			Duke.BigText(160, 114, rec.DisplayName(), 0);
		}
		else
		{
			int y = Raze.isRRRA()? 140 : 90;
			Duke.BigText(160, y, (rec.flags & MapRecord.USERMAP)? "$TXT_ENTRUM" : "$TXT_ENTERIN", 0);
			Duke.BigText(160, y+24, rec.DisplayName(), 0);
		}
	}
}

