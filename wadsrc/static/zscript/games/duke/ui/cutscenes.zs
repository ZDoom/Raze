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
		Raze.StopMusic();
		Raze.StopAllSounds();

		if (!userConfig.nologo)
		{
			if (!Raze.isShareware())
			{
				Array<int> soundinfo;
				soundinfo.Push(1);
				soundinfo.Push(DukeSnd.FLY_BY+1);
				soundinfo.Push(19);
				soundinfo.Push(DukeSnd.PIPEBOMB_EXPLODE+1);
				jobs.Push(MoviePlayerJob.CreateWithSoundinfo("logo.anm", soundinfo, 9, 9, 9));
			}
			if (!Raze.isNam()) jobs.Push(new("DRealmsScreen").Init());
		}
		jobs.Push(new("DukeTitleScreen").Init());
    }
}