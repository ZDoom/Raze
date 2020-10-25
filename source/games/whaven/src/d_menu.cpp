//-------------------------------------------------------------------------
/*
Copyright (C) 2020 Christoph Oelckers

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
#include "icorp.h"
#include "version.h"
#include "baselayer.h"
#include "menu/menu.h"

#include "../../glbackend/glbackend.h"


BEGIN_WH_NS

int handle1;
extern int gameactivated;


int MenuExitCondition;
int MenuStartCondition;

//----------------------------------------------------------------------------
//
// Implements the native looking menu used for the main menu
// and the episode/skill selection screens, i.e. the parts
// that need to look authentic
//
//----------------------------------------------------------------------------
void menu_DoPlasma();
int zoomsize = 0;

class WHMainMenu : public DListMenu
{

	void Init(DMenu* parent, FListMenuDescriptor* desc) override
	{
		DListMenu::Init(parent, desc);
		//PlayLocalSound(StaticSound[kSound31], 0);
	}

	void PreDraw() override
	{
		overwritesprite(127,58,MENUSELECTIONS,0, RS_AUTO,0);
	}
};

class WHSkillMenu : public DListMenu
{

	void Init(DMenu* parent, FListMenuDescriptor* desc) override
	{
		DListMenu::Init(parent, desc);
		//PlayLocalSound(StaticSound[kSound31], 0);
	}

	void PreDraw() override
	{
        ///overwritesprite(127,58,BLOODGOREGREEN,0,RS_AUTO,0);
        overwritesprite(148,114,DIFFICULTRED,0,RS_AUTO,0);
        overwritesprite(147,143,HORNYBACK,0,RS_AUTO,0);
	}
	
	//=============================================================================
	//
	// some hackery to make a horizontal menu work.
	//
	//=============================================================================

	bool MenuEvent (int key, bool fromcontroller)
	{
		switch (key)
		{
		case MKEY_Right:
			key = MKEY_Down;
			break;

		case MKEY_Left:
			key = MKEY_Up;
			break;

		default:
			break;	// Keep GCC quiet
		}
		return DListMenu::MenuEvent(key, fromcontroller);
	}

};


//----------------------------------------------------------------------------
//
// Menu related game interface functions
//
//----------------------------------------------------------------------------

void GameInterface::DrawNativeMenuText(int fontnum, int state, double xpos, double ypos, float fontscale, const char* text, int flags)
{
    static struct {
        int x;
        int y;
    }redpic1[5] = {
		{ 142,58 },
		{ 140,80 },
		{ 127,104 },
		{ 184,126 },
		{ 183,150 } };
		
	struct {
		int x;
		int y;
	}redpic2[4]={
		{ 148,146 },
		{ 181,146 },
		{ 215,144 },
		{ 257,143 } };
        
		
	int index = (int)strtol(text, nullptr, 0);
	if (state == NIT_SelectedState)
	{
		if (index < 5)
		{
			int redpicnum=NEWGAMEGREEN+index;
			overwritesprite(redpic1[index].x,redpic1[index].y,redpicnum,0, RS_AUTO,0);
		}
		else if (index < 9)
		{
			int redpicnum = HORNYSKULL1 + index-5;
			overwritesprite(redpic2[index-4].x,redpic2[index-4].y,redpicnum,0,RS_AUTO,0);
		}
	}
}


void GameInterface::MenuOpened()
{
	//GrabPalette();
	//StopAllSounds();
	//StopLocalSound();
}

void GameInterface::MenuSound(EMenuSounds snd)
{
	switch (snd)
	{
		case CursorSound:
		case AdvanceSound:
		case BackSound:
			SND_Sound( rand()%60 );	// Yes, it really is this dumb.
			break;

		default:
			return;
	}
}

void GameInterface::MenuClosed()
{

}


void GameInterface::StartGame(FGameStartup& gs)
{
	gameactivated=0;
	//JSA BLORB
	SND_Sting(S_STING1);
	SND_FadeMusic();
	srand((int)totalclock&30000);
	//tille maps are finished
	//if(loadedgame == 0 && netgame == 0)
	//   mapon=1;

	auto plr = &player[0];
	startnewgame(plr);
	gameactivated=1;
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
			int width = fancyfontsize(THEFONT, text);
			fancyfontscreen(int(origin.X) + 160 - width / 2,  position + int(origin.Y), THEFONT, text);
			y += tilesiz[THEFONT].y+2;
		}
	}
}

void GameInterface::DrawMenuCaption(const DVector2& origin, const char* text)
{
	DrawCenteredTextScreen(origin, text, 10, false);
}



END_WH_NS

//----------------------------------------------------------------------------
//
// Class registration
//
//----------------------------------------------------------------------------


static TMenuClassDescriptor<Witchaven::WHMainMenu> _mm("Witchaven.MainMenu");

void RegisterWHMenus()
{
	menuClasses.Push(&_mm);
}
