//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2020 - Christoph Oelckers 

This file is part of Enhanced Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms

*/
//-------------------------------------------------------------------------

#include "ns.h"	// Must come before everything else!

#include "g_input.h"
#include "duke3d.h"

#include "superfasthash.h"
#include "gamecvars.h"
#include "gamecontrol.h"
#include "c_bind.h"
#include "razemenu.h"
#include "gstrings.h"
#include "version.h"
#include "names.h"
#include "mapinfo.h"
#include "gamestate.h"
#include "dukeactor.h"


BEGIN_DUKE_NS

//----------------------------------------------------------------------------
//
// Menu related game interface functions
//
//----------------------------------------------------------------------------

void GameInterface::MenuOpened()
{
	StopCommentary();
	if (ud.multimode < 2)
	{
		screenpeek = myconnectindex;
	}
}

void GameInterface::MenuSound(EMenuSounds snd)
{
	switch (snd)
	{
	case ActivateSound:
		S_MenuSound();
		break;

	case ChooseSound:
	case CursorSound:
		S_PlaySound(isRR() ? 335 : KICK_HIT, CHAN_AUTO, CHANF_UI);
		break;

	case AdvanceSound:
		S_PlaySound(isRR() ? 341 : PISTOL_BODYHIT, CHAN_AUTO, CHANF_UI);
		break;

	case CloseSound:
	case BackSound:
		S_PlaySound(EXITMENUSOUND, CHAN_AUTO, CHANF_UI);
		break;

	default:
		return;
	}
}

bool GameInterface::CanSave()
{
	if (ud.recstat == 2 || gamestate != GS_LEVEL) return false;
	auto &myplayer = ps[myconnectindex];
	return (myplayer.GetActor()->s->extra > 0);
}

bool GameInterface::StartGame(FNewGameStartup& gs)
{
	int32_t skillsound = PISTOL_BODYHIT;

	static const uint16_t sounds_d[] = { JIBBED_ACTOR6, BONUS_SPEECH1, DUKE_GETWEAPON2, JIBBED_ACTOR5, JIBBED_ACTOR5 };
	static const uint16_t sounds_r[] = { 427, 428, 196, 195, 197 };
	if (gs.Skill >=0 && gs.Skill <= 5) skillsound = isRR()? sounds_r[gs.Skill] : sounds_d[gs.Skill];

	if (menu_sounds && skillsound >= 0 && SoundEnabled() && !netgame)
	{
		S_PlaySound(skillsound, CHAN_AUTO, CHANF_UI);

		while (S_CheckSoundPlaying(skillsound))
		{
			gi->UpdateSounds();
			soundEngine->UpdateSounds(I_GetTime());
			I_GetEvent();
		}
		Net_ClearFifo();
	}
	return true;
}

FSavegameInfo GameInterface::GetSaveSig()
{
	return { SAVESIG_DN3D, MINSAVEVER_DN3D, SAVEVER_DN3D };
}

void GameInterface::DrawPlayerSprite(const DVector2& origin, bool onteam)
{
	int mclock = I_GetBuildTime();
	int color = TRANSLATION(Translation_Remap, playercolor2lookup(playercolor));
	int tile = isRR() ? 3845 + 36 - ((((8 - (mclock >> 4))) & 7) * 5) : 1441 - ((((4 - (mclock >> 4))) & 3) * 5);
	auto tex = tileGetTexture(tile);
	if (!tex) return;
	double x = origin.X + 250, y = origin.Y + tex->GetDisplayHeight() * (isRR()? 0.25 : 0.5);
	double scale = isRR() ? 0.375 : 0.75;

	DrawTexture(twod, tex, x, y, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TranslationIndex, color, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);
}

END_DUKE_NS
