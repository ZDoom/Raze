//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors
Copyright (C) 2019 Christoph Oelckers

This is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "ns.h"	// Must come before everything else!
#include "build.h"
#include "g_input.h"

#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "sector.h"
#include "sprite.h"
#include "weapon.h"
#include "player.h"
#include "jsector.h"
#include "menus.h"
#include "pal.h"
#include "keydef.h"
#include "d_net.h"

#include "gamecontrol.h"
#include "misc.h"
#include "version.h"
#include "network.h"

#include "misc.h"
#include "razemenu.h"
#include "raze_sound.h"
#include "sounds.h"
#include "gamestate.h"
#include "raze_music.h"
#include "v_draw.h"
#include "vm.h"

#include "../../glbackend/glbackend.h"


BEGIN_SW_NS

//----------------------------------------------------------------------------
//
// Implements the native looking menu used for the main menu
// and the episode/skill selection screens, i.e. the parts
// that need to look authentic
//
//----------------------------------------------------------------------------

static bool DidOrderSound;
static int zero = 0;

DEFINE_ACTION_FUNCTION(_SWMenuDelegate, PlayOrderSound)
{
	if (SW_SHAREWARE && !DidOrderSound)
	{
		DidOrderSound = true;
		int choose_snd = STD_RANDOM_RANGE(1000);
		if (choose_snd > 500)
			PlaySound(DIGI_WANGORDER1, v3df_dontpan, CHAN_BODY, CHANF_UI);
		else 
			PlaySound(DIGI_WANGORDER2, v3df_dontpan, CHAN_BODY, CHANF_UI);
	}
	return 0;
}

void GameInterface::QuitToTitle()
{
	Mus_Stop();
	gameaction = ga_mainmenu;
}


void GameInterface::MenuSound(EMenuSounds snd)
{
	switch (snd)
	{
		case CursorSound:
            PlaySound(DIGI_STAR, v3df_dontpan, CHAN_BODY, CHANF_UI);
			break;

		case AdvanceSound:
		case ChooseSound:
			PlaySound(DIGI_SWORDSWOOSH, v3df_dontpan, CHAN_BODY, CHANF_UI);
			break;
			
		case CloseSound:
		case BackSound:
			PlaySound(DIGI_STARCLINK, v3df_dontpan, CHAN_BODY, CHANF_UI);
			break;

		default:
			return;
	}
}

bool GameInterface::CanSave()
{
    return (gamestate == GS_LEVEL && !CommEnabled && numplayers ==1 && /*!DemoMode &&*/ !TEST(Player[myconnectindex].Flags, PF_DEAD));
}

bool GameInterface::StartGame(FNewGameStartup& gs)
{
    PLAYERp pp = Player + screenpeek;
    int handle = 0;
    int zero = 0;
	
	MapRecord* map;
	if (gs.Episode >= 1)
	{
		if (g_gameType & GAMEFLAG_SHAREWARE)
		{
			M_StartMessage(GStrings("BUYSW"), 1, NAME_None);
			return false;
		}
		map = FindMapByLevelNum(5);
	}
    else
		map = FindMapByLevelNum(1);

	if (!map) return false;
    CameraTestMode = false;
	StopFX();

    //InitNewGame();

	if (!netgame)
	{
		if (gs.Skill == 0)
			PlaySound(DIGI_TAUNTAI3, v3df_none, CHAN_VOICE, CHANF_UI);
		else if (gs.Skill == 1)
			PlaySound(DIGI_NOFEAR, v3df_none, CHAN_VOICE, CHANF_UI);
		else if (gs.Skill == 2)
			PlaySound(DIGI_WHOWANTSWANG, v3df_none, CHAN_VOICE, CHANF_UI);
		else if (gs.Skill == 3)
			PlaySound(DIGI_NOPAIN, v3df_none, CHAN_VOICE, CHANF_UI);

		while (soundEngine->IsSourcePlayingSomething(SOURCE_None, nullptr, CHAN_VOICE))
		{
			gi->UpdateSounds();
			soundEngine->UpdateSounds(I_GetTime());
			I_GetEvent();
		}
		Net_ClearFifo();
	}
	DeferedStartGame(map, gs.Skill);
	return true;
}

FSavegameInfo GameInterface::GetSaveSig()
{
	return { SAVESIG_SW, MINSAVEVER_SW, SAVEVER_SW };
}



END_SW_NS
