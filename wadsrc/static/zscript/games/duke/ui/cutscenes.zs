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

class DukeCutscenes ui // Note: must be class, not struct, otherwise we cannot easily look up the methods from C++.
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
			if (!isShareware())
			{
				Array<int> soundinfo;
				soundinfo.Pushv(
					1, int(Sound("FLY_BY")),
					19, int(Sound("PIPEBOMB_EXPLODE")));
				runner.Append(MoviePlayerJob.CreateWithSoundinfo("logo.anm", soundinfo, 0, 9, 9, 9));
			}
			if (!isNam()) runner.Append(new("DRealmsScreen").Init());
		}
		runner.Append(new("DukeTitleScreen").Init());
    }

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static void BuildE1End(ScreenJobRunner runner)
	{
		runner.Append(new("Episode1End1").Init());
		runner.Append(ImageScreen.CreateNamed("E1ENDSCREEN", ScreenJob.fadein|ScreenJob.fadeout|ScreenJob.stopmusic, 0x7fffffff));
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static void BuildE2End(ScreenJobRunner runner)
	{
		Array<int> soundinfo;
		soundinfo.Pushv(
			1, int(Sound("WIND_AMBIENCE")),
			26, int(Sound("ENDSEQVOL2SND1")),
			36, int(Sound("ENDSEQVOL2SND2")),
			54, int(Sound("THUD")),
			62, int(Sound("ENDSEQVOL2SND3")),
			75, int(Sound("ENDSEQVOL2SND4")),
			81, int(Sound("ENDSEQVOL2SND5")),
			115, int(Sound("ENDSEQVOL2SND6")),
			124, int(Sound("ENDSEQVOL2SND7")));

		runner.Append(MoviePlayerJob.CreateWithSoundinfo("cineov2.anm", soundinfo, 0, 18, 18, 18));
		runner.Append(new("E2EndScreen").Init());
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static void BuildE3End(ScreenJobRunner runner)
	{
		if (gameinfo.gameType & GAMEFLAG_DUKEDC)
		{
			Array<int> soundinfo;
			soundinfo.Pushv(144, int(Sound("ENDSEQVOL3SND3")));
			runner.Append(MoviePlayerJob.CreateWithSoundinfo("radlogo.anm", soundinfo, 0, 10, 10, 10));
		}
		else
		{
			Array<int> soundinfo;
			soundinfo.Pushv(
				1, int(Sound("WIND_REPEAT")),
				98, int(Sound("PLAYER_GRUNT")),
				102, int(Sound("THUD")),
				102, int(Sound("SQUISHED")),
				124, int(Sound("ENDSEQVOL3SND3")),
				134, int(Sound("ENDSEQVOL3SND2")),
				158, int(Sound("PIPEBOMB_EXPLODE")));

			runner.Append(MoviePlayerJob.CreateWithSoundinfo("cineov3.anm", soundinfo, 0, 10, 10, 10));
			runner.Append(BlackScreen.Create(200, ScreenJob.stopsound));
			runner.Append(new("Episode3End").Init());
			if (!isPlutoPak()) runner.Append(ImageScreen.CreateNamed("DUKETEAM.ANM", ScreenJob.fadein | ScreenJob.fadeout | ScreenJob.stopsound, 0x7fffffff));
		}
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static void BuildE4End(ScreenJobRunner runner)
	{
		Array<int> soundinfo;

		soundinfo.Pushv(
			3, int(Sound("PLAYER_UNDERWATER")),
			35, int(Sound("VOL4ENDSND1")));
		runner.Append(MoviePlayerJob.CreateWithSoundinfo("vol4e1.anm", soundinfo, 0, 10, 10, 10));

		soundinfo.Pushv(
			11, int(Sound("PLAYER_UNDERWATER")),
			20, int(Sound("VOL4ENDSND1")),
			39, int(Sound("VOL4ENDSND2")),
			50, -1);
		runner.Append(MoviePlayerJob.CreateWithSoundinfo("vol4e2.anm", soundinfo, 0, 14, 14, 14));

		soundinfo.Pushv(
			1, int(Sound("BOSS4_DEADSPEECH")),
			40, int(Sound("VOL4ENDSND1")),
			40, int(Sound("PLAYER_UNDERWATER")),
			50, int(Sound("BIGBANG")));
		runner.Append(MoviePlayerJob.CreateWithSoundinfo("vol4e3.anm", soundinfo, 0, 10, 10, 10));

		runner.Append(new("Episode4Text").Init());
		runner.Append(ImageScreen.CreateNamed("DUKETEAM.ANM", ScreenJob.fadein | ScreenJob.fadeout | ScreenJob.stopsound, 0x7fffffff));
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static void BuildE5End(ScreenJobRunner runner)
	{
		runner.Append(new("Episode5End").Init());
	}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

	static void BuildE4Intro(ScreenJobRunner runner)
	{
		Array<int> soundinfo;

		Duke.PlaySpecialMusic(Duke.MUS_BRIEFING);
		soundinfo.Pushv(
			1, int(Sound("INTRO4_1")),
			7, int(Sound("INTRO4_3")),
			12, int(Sound("INTRO4_2")),
			26, int(Sound("INTRO4_4")));
		runner.Append(MoviePlayerJob.CreateWithSoundinfo("vol41a.anm", soundinfo, MoviePlayer.NOSOUNDCUTOFF, 14, 14, 14));

		soundinfo.Pushv(
			1, int(Sound("INTRO4_B")),
			12, int(Sound("SHORT_CIRCUIT")),
			18, int(Sound("INTRO4_5")),
			34, int(Sound("SHORT_CIRCUIT")));
		let m = MoviePlayerJob.CreateWithSoundinfo("vol42a.anm", soundinfo, MoviePlayer.NOSOUNDCUTOFF, 18, 18, 18);
		if (m) m.skipover = true;
		runner.Append(m);

		soundinfo.Pushv(
			10, int(Sound("INTRO4_6")));
		m = MoviePlayerJob.CreateWithSoundinfo("vol43a.anm", soundinfo, 0, 10, 10, 10);
		if (m) m.skipover = true;
		runner.Append(m);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	static void BuildMPSummary(ScreenJobRunner runner, MapRecord map, SummaryInfo stats)
	{
		runner.Append(new("DukeMultiplayerBonusScreen").Init(stats.playercount));
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	static void BuildSPSummary(ScreenJobRunner runner, MapRecord map, SummaryInfo stats)
	{
		let screen = new("DukeLevelSummaryScreen").Init(map, stats);
		runner.Append(screen);
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static void BuildSharewareExit(ScreenJobRunner runner)
	{
		runner.Append(ImageScreen.CreateNamed("SWEXIT1"));
		runner.Append(ImageScreen.CreateNamed("SWEXIT2"));
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	static void BuildSharewareEnd(ScreenJobRunner runner)
	{
		runner.Append(ImageScreen.CreateNamed("ORDERING"));
		runner.Append(ImageScreen.CreateNamed("ORDERING1"));
		runner.Append(ImageScreen.CreateNamed("ORDERING2"));
		runner.Append(ImageScreen.CreateNamed("ORDERING3"));
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	static void BuildLoading(ScreenJobRunner runner, MapRecord map)
	{
		runner.Append(new("DukeLoadScreen").Init(map));
	}

}

class RRCutscenes ui
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
			Array<int> soundinfo;
			soundinfo.Pushv(1, int(Sound("URANUS")));
			runner.Append(MoviePlayerJob.CreateWithSoundinfo("rr_intro.anm", soundinfo, 0, 9, 9, 9));

			soundinfo.Pushv(1, int(Sound("REDNECK2")));
			runner.Append(MoviePlayerJob.CreateWithSoundinfo("redneck.anm", soundinfo, 0, 9, 9, 9));

			soundinfo.Pushv(1, int(Sound("XATRIX")));
			runner.Append(MoviePlayerJob.CreateWithSoundinfo("xatlogo.anm", soundinfo, 0, 9, 9, 9));
		}
	}


	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static void BuildRAIntro(ScreenJobRunner runner)
    {
		if (!userConfig.nologo)
		{
			runner.Append(MoviePlayerJob.Create("redint.mve", 0));
		}
	}


	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static void BuildE1End(ScreenJobRunner runner)
    {
		if (!isRRRA())
		{
			Array<int> soundinfo;
			soundinfo.Pushv(1, int(Sound("CHKAMMO")));
			runner.Append(MoviePlayerJob.CreateWithSoundinfo("turdmov.anm", soundinfo, 0, 9, 9, 9));
		} 
	}


	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static void BuildE2End(ScreenJobRunner runner)
    {
		Array<int> soundinfo;
		soundinfo.Pushv(1, int(Sound("LN_FINAL")));
		runner.Append(MoviePlayerJob.CreateWithSoundinfo("rr_outro.anm", soundinfo, MoviePlayer.NOSOUNDCUTOFF, 9, 9, 9));
		runner.Append(ImageScreen.CreateNamed("TENSCREEN"));
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static void BuildRAE2End(ScreenJobRunner runner)
    {
		runner.Append(new("RRRAEndOfGame").Init());
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	static void BuildSPSummary(ScreenJobRunner runner, MapRecord map, SummaryInfo stats)
	{
		runner.Append(new("RRLevelSummaryScreen").Init(map, stats, !isRRRA() || stats.endOfGame));
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	static void BuildMapIntro(ScreenJobRunner runner, MapRecord map)
	{
		int ln = map.levelnumber - 1001;
		if (ln == 0) return;
		if (ln >= 1000) ln -= 1000-7;

		let fn = String.Format("lvl%d.anm", ln);
		Array<int> soundinfo;
		runner.Append(MoviePlayerJob.CreateWithSoundinfo(fn, soundinfo, 0, 20, 20, 2000)); // wait for a few seconds on the final frame so that the video doesn't stop  before the user notices.
	}
}