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
#include "exhumed.h"
#include "engine.h"
#include "sound.h"
#include "names.h"
#include "version.h"
#include "raze_sound.h"
#include "gamestate.h"
#include "mapinfo.h"
#include "gamecontrol.h"
#include "v_draw.h"
#include "vm.h"
#include "razemenu.h"

#include "../../glbackend/glbackend.h"


BEGIN_PS_NS


DEFINE_ACTION_FUNCTION(_ListMenuItemExhumedPlasma, Draw)
{
	menu_DoPlasma();
	return 0;
}

DEFINE_ACTION_FUNCTION(_ListMenuItemExhumedLogo, Draw)
{
	auto nLogoTile = EXHUMED ? kExhumedLogo : kPowerslaveLogo;
	DrawRel(nLogoTile, 160, 40);
	return 0;
}



void GameInterface::MenuOpened()
{
	GrabPalette();
	menuDelegate->FloatVar(NAME_zoomsize) = 0;
	StopAllSounds();
	StopLocalSound();
}

void GameInterface::MenuSound(EMenuSounds snd)
{
	switch (snd)
	{
	case ActivateSound:
		PlayLocalSound(StaticSound[kSound31], 0, false, CHANF_UI);
		break;

	case CursorSound:
		PlayLocalSound(StaticSound[kSound35], 0, false, CHANF_UI);
		break;

	case AdvanceSound:
	case BackSound:
		PlayLocalSound(StaticSound[kSound33], 0, false, CHANF_UI);
		break;

	default:
		return;
	}
}

void GameInterface::QuitToTitle()
{
	gameaction = ga_mainmenu;
}

void GameInterface::MenuClosed()
{

}


bool GameInterface::StartGame(FNewGameStartup& gs)
{
	auto map = FindMapByLevelNum(gs.Episode);
	DeferedStartGame(map, gs.Skill);	// 0 is training, 1 is the regular game - the game does not have skill levels.
	return true;
}

FSavegameInfo GameInterface::GetSaveSig()
{
	return { SAVESIG_PS, MINSAVEVER_PS, SAVEVER_PS };
}



END_PS_NS
