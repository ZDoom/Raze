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


#include "menu/menu.h"	// to override the local menu.h

#include "../../glbackend/glbackend.h"


BEGIN_PS_NS

//----------------------------------------------------------------------------
//
// Implements the native looking menu used for the main menu
// and the episode/skill selection screens, i.e. the parts
// that need to look authentic
//
//----------------------------------------------------------------------------
void menu_DoPlasma();
int zoomsize = 0;

class PSMainMenu : public DListMenu
{

	void Init(DMenu* parent, FListMenuDescriptor* desc) override
	{
		DListMenu::Init(parent, desc);
		PlayLocalSound(StaticSound[kSound31], 0, false, CHANF_UI);
	}

	void Ticker() override
	{
		// handle the menu zoom-in
		if (zoomsize < 0x10000)
		{
			zoomsize += 4096;
			if (zoomsize >= 0x10000) {
				zoomsize = 0x10000;
			}
		}
	}

	void PreDraw() override
	{
		if (mDesc->mMenuName == NAME_Mainmenu)
			menu_DoPlasma();
		else
		{
			auto nLogoTile = EXHUMED ? kExhumedLogo : kPowerslaveLogo;
			DrawRel(nLogoTile, 160, 40);
		}
	}
};


//----------------------------------------------------------------------------
//
// Menu related game interface functions
//
//----------------------------------------------------------------------------

void GameInterface::DrawNativeMenuText(int fontnum, int state, double xpos, double ypos, float fontscale, const char* text, int flags)
{
	int tilenum = (int)strtoll(text, nullptr, 0);
	double y = ypos - tilesiz[tilenum].y / 2;

	int8_t shade;

	if (state == NIT_SelectedState) 
	{ // currently selected menu item
		shade = Sin((int)totalclock << 4) >> 9;
	}
	else if (state == NIT_ActiveState) {
		shade = 0;
	}
	else {
		shade = 25;
	}

	// Todo: Replace the boxes with an empty one and draw the text with a font.
	auto tex = tileGetTexture(tilenum);

	DrawTexture(twod, tex, 160, y + tex->GetDisplayHeight(), DTA_FullscreenScale, FSMode_ScaleToFit43, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_CenterOffset, true, 
		DTA_Color, shadeToLight(shade), TAG_DONE);

	// tilesizx is 51
	// tilesizy is 33

	if (state == NIT_SelectedState)
	{
		tex = tileGetTexture(kMenuCursorTile);
		DrawTexture(twod, tex, 62, ypos - 12, DTA_FullscreenScale, FSMode_ScaleToFit43, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_TopLeft, true, TAG_DONE);
		DrawTexture(twod, tex, 207, ypos - 12, DTA_FullscreenScale, FSMode_ScaleToFit43, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_TopLeft, true, DTA_FlipX, true, TAG_DONE);
	}
}


void GameInterface::MenuOpened()
{
	GrabPalette();
	zoomsize = 0;
	StopAllSounds();
	StopLocalSound();
}

void GameInterface::MenuSound(EMenuSounds snd)
{
	switch (snd)
	{
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

void GameInterface::MenuClosed()
{

}


void GameInterface::StartGame(FNewGameStartup& gs)
{
	GameAction = gs.Episode;	// 0 is training, 1 is the regular game
}

FSavegameInfo GameInterface::GetSaveSig()
{
	return { SAVESIG_PS, MINSAVEVER_PS, SAVEVER_PS };
}

void GameInterface::DrawMenuCaption(const DVector2& origin, const char* text)
{
	// Fixme: should use the extracted font from the menu items
	DrawCenteredTextScreen(origin, text, 10, false);
}



END_PS_NS

//----------------------------------------------------------------------------
//
// Class registration
//
//----------------------------------------------------------------------------


static TMenuClassDescriptor<Powerslave::PSMainMenu> _mm("Exhumed.MainMenu");

void RegisterPSMenus()
{
	menuClasses.Push(&_mm);
}
