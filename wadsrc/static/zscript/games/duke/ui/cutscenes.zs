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

class DukeIntro : ScreenJobRunner
{
	void Init()
    {
		Super.Init();
		Raze.StopMusic();
		Raze.StopAllSounds();

		if (!userConfig.nologo)
		{
			if (!Raze.isShareware())
			{
				Array<int> soundinfo;
				soundinfo.Pushv(
					1, DukeSnd.FLY_BY+1,
					19, DukeSnd.PIPEBOMB_EXPLODE+1);
				jobs.Push(MoviePlayerJob.CreateWithSoundinfo("logo.anm", soundinfo, 0, 9, 9, 9));
			}
			if (!Raze.isNam()) jobs.Push(new("DRealmsScreen").Init());
		}
		jobs.Push(new("DukeTitleScreen").Init());
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeE1End : ScreenJobRunner
{
	void Init()
	{
		Super.Init();
		jobs.Push(new("Episode1End1").Init());
		jobs.Push(ImageScreen.CreateNamed("E1ENDSCREEN", ScreenJob.fadein|ScreenJob.fadeout|ScreenJob.stopmusic, 0x7fffffff));
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeE2End : ScreenJobRunner
{
	void Init()
	{
		Super.Init();
		Array<int> soundinfo;
		soundinfo.Pushv(
			1, DukeSnd.WIND_AMBIENCE+1,
			26, DukeSnd.ENDSEQVOL2SND1+1,
			36, DukeSnd.ENDSEQVOL2SND2+1,
			54, DukeSnd.THUD+1,
			62, DukeSnd.ENDSEQVOL2SND3+1,
			75, DukeSnd.ENDSEQVOL2SND4 + 1,
			81, DukeSnd.ENDSEQVOL2SND5 + 1,
			115, DukeSnd.ENDSEQVOL2SND6 + 1,
			124, DukeSnd.ENDSEQVOL2SND7 + 1);

		jobs.Push(MoviePlayerJob.CreateWithSoundinfo("cineov2.anm", soundinfo, 0, 18, 18, 18));
		jobs.Push(new("E2EndScreen").Init());
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeE3End : ScreenJobRunner
{
	void Init()
	{
		Super.Init();
		Array<int> soundinfo;
		soundinfo.Pushv(
			1, DukeSnd.WIND_REPEAT + 1,
			98, DukeSnd.DUKE_GRUNT + 1,
			102, DukeSnd.THUD + 1,
			102, DukeSnd.SQUISHED + 1,
			124, DukeSnd.ENDSEQVOL3SND3 + 1,
			134, DukeSnd.ENDSEQVOL3SND2 + 1,
			158, DukeSnd.PIPEBOMB_EXPLODE + 1);

		jobs.Push(MoviePlayerJob.CreateWithSoundinfo("cineov3.anm", soundinfo, 0, 10, 10, 10));
		jobs.Push(BlackScreen.Create(200, ScreenJob.stopsound));
		jobs.Push(new("Episode3End").Init());
		if (!Raze.isPlutoPak()) jobs.Push(ImageScreen.CreateNamed("DUKETEAM.ANM", ScreenJob.fadein | ScreenJob.fadeout | ScreenJob.stopsound, 0x7fffffff));
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeDCEnd : ScreenJobRunner
{
	void Init()
	{
		Super.Init();
		Array<int> soundinfo;
		soundinfo.Pushv(144, DukeSnd.ENDSEQVOL3SND3 + 1);
		jobs.Push(MoviePlayerJob.CreateWithSoundinfo("radlogo.anm", soundinfo, 0, 10, 10, 10));
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeE4End : ScreenJobRunner
{
	void Init()
	{
		Super.Init(true, true);
		Array<int> soundinfo;

		soundinfo.Pushv(
			3, DukeSnd.DUKE_UNDERWATER+1,
			35, DukeSnd.VOL4ENDSND1+1);
		jobs.Push(MoviePlayerJob.CreateWithSoundinfo("vol4e1.anm", soundinfo, 0, 10, 10, 10));

		soundinfo.Pushv(
			11, DukeSnd.DUKE_UNDERWATER+1,
			20, DukeSnd.VOL4ENDSND1+1,
			39, DukeSnd.VOL4ENDSND2+1,
			50, -1);
		jobs.Push(MoviePlayerJob.CreateWithSoundinfo("vol4e2.anm", soundinfo, 0, 10, 10, 10));

		soundinfo.Pushv(
			1, DukeSnd.BOSS4_DEADSPEECH+1,
			40, DukeSnd.VOL4ENDSND1+1,
			40, DukeSnd.DUKE_UNDERWATER+1,
			50, DukeSnd.BIGBANG+1);
		jobs.Push(MoviePlayerJob.CreateWithSoundinfo("vol4e3.anm", soundinfo, 0, 10, 10, 10));

		jobs.Push(new("Episode4Text").Init());
		jobs.Push(ImageScreen.CreateNamed("DUKETEAM.ANM", ScreenJob.fadein | ScreenJob.fadeout | ScreenJob.stopsound, 0x7fffffff));
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeE5End : ScreenJobRunner
{
	void Init()
	{
		Super.Init();
		jobs.Push(new("Episode5End").Init());
	}
}

