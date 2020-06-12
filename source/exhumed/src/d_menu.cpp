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
#include "osd.h"
#include "osd.h"
#include "exhumed.h"
#include "engine.h"
#include "sound.h"
#include "names.h"
#include "version.h"
#include "raze_sound.h"


#include "menu/menu.h"

#include "../../glbackend/glbackend.h"


BEGIN_PS_NS

int handle1;


int MenuExitCondition;

int menu_Menu(int nVal)
{
	MenuExitCondition = -2;
	M_StartControlPanel(false);
	M_SetMenu(NAME_Mainmenu);
	while (M_Active())
	{
		auto nLogoTile = EXHUMED ? kExhumedLogo : kPowerslaveLogo;
		int dword_9AB5F = ((int)totalclock / 16) & 3;

		twod->ClearScreen();

		overwritesprite(160, 100, kSkullHead, 32, 3, kPalNormal);
		overwritesprite(161, 130, kSkullJaw, 32, 3, kPalNormal);

		overwritesprite(160, 40, nLogoTile, 32, 3, kPalNormal);

		// draw the fire urn/lamp thingies
		overwritesprite(50, 150, kTile3512 + dword_9AB5F, 32, 3, kPalNormal);
		overwritesprite(270, 150, kTile3512 + ((dword_9AB5F + 2) & 3), 32, 3, kPalNormal);

		HandleAsync();
		videoNextPage();

	}
	int me = MenuExitCondition;
	MenuExitCondition = -2;
	return me;
}


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
			overwritesprite(160, 40, nLogoTile, 32, 3, kPalNormal);
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

	rotatesprite(160 << 16, int((y + tilesiz[tilenum].y) *65536), zoomsize, 0, tilenum, shade, 0, RS_AUTO|RS_CENTER, 0, 0, xdim, ydim);

	// tilesizx is 51
	// tilesizy is 33

	if (state == NIT_SelectedState)
	{
		overwritesprite(62, short(ypos - 12), kMenuCursorTile, 0, 2, kPalNormal);
		overwritesprite(62 + 146, short(ypos - 12), kMenuCursorTile, 0, 10, kPalNormal);
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
	MenuExitCondition = gs.Episode;	// Gross hack. The main loop needs to be redone for better handling.
}

FSavegameInfo GameInterface::GetSaveSig()
{
	return { SAVESIG_PS, MINSAVEVER_PS, SAVEVER_PS };
}

void GameInterface::DrawCenteredTextScreen(const DVector2& origin, const char* text, int position, bool bg)
{
	if (text)
	{
		int height = 11;

		auto lines = FString(text).MakeUpper().Split("\n");
		int y = position - (height * lines.Size() / 2);
		for (auto& l : lines)
		{
			int width = MyGetStringWidth(l);
			myprintext(int(origin.X) + 160 - width / 2, int(origin.Y) + y, l, 0);
			y += height;
		}
	}
}

void GameInterface::DrawMenuCaption(const DVector2& origin, const char* text)
{
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
