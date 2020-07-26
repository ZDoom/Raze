//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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
#include "compat.h"
#include "common_game.h"
#include "blood.h"
#include "globals.h"
#include "inifile.h"
#include "levels.h"
#include "qav.h"
#include "view.h"
#include "demo.h"
#include "network.h"
#include "mmulti.h"
#include "c_bind.h"
#include "menu.h"
#include "sound.h"

bool ShowOptionMenu();

BEGIN_BLD_NS

class CGameMenuItemQAV
{
public:
	int m_nX, m_nY;
	TArray<uint8_t> raw;
	int at2c;
	int lastTick;
	bool bWideScreen;
	bool bClearBackground;
	CGameMenuItemQAV(int, int, const char*, bool widescreen = false, bool clearbackground = false);
	void Draw(void);
};

CGameMenuItemQAV::CGameMenuItemQAV(int a3, int a4, const char* name, bool widescreen, bool clearbackground)
{
	m_nY = a4;
	m_nX = a3;
	bWideScreen = widescreen;
	bClearBackground = clearbackground;

	if (name)
	{
		// NBlood read this directly from the file system cache, but let's better store the data locally for robustness.
		raw = fileSystem.LoadFile(name, 0);
		if (raw.Size() != 0)
		{
			auto data = (QAV*)raw.Data();
			data->nSprite = -1;
			data->x = m_nX;
			data->y = m_nY;
			data->Preload();
			at2c = data->at10;
			lastTick = (int)totalclock;
		}
	}
}

void CGameMenuItemQAV::Draw(void)
{
	if (bClearBackground)
		twod->ClearScreen();

	if (raw.Size() > 0)
	{
		auto data = (QAV*)raw.Data();
		ClockTicks backFC = gFrameClock;
		gFrameClock = totalclock;
		int nTicks = (int)totalclock - lastTick;
		lastTick = (int)totalclock;
		at2c -= nTicks;
		if (at2c <= 0 || at2c > data->at10)
		{
			at2c = data->at10;
		}
		data->Play(data->at10 - at2c - nTicks, data->at10 - at2c, -1, NULL);
		int wx1, wy1, wx2, wy2;
		wx1 = windowxy1.x;
		wy1 = windowxy1.y;
		wx2 = windowxy2.x;
		wy2 = windowxy2.y;
		windowxy1.x = 0;
		windowxy1.y = 0;
		windowxy2.x = xdim - 1;
		windowxy2.y = ydim - 1;
		if (bWideScreen)
		{
			int xdim43 = scale(ydim, 4, 3);
			int nCount = (xdim + xdim43 - 1) / xdim43;
			int backX = data->x;
			for (int i = 0; i < nCount; i++)
			{
				data->Draw(data->at10 - at2c, 10 + kQavOrientationLeft, 0, 0);
				data->x += 320;
			}
			data->x = backX;
		}
		else
			data->Draw(data->at10 - at2c, 10, 0, 0);

		windowxy1.x = wx1;
		windowxy1.y = wy1;
		windowxy2.x = wx2;
		windowxy2.y = wy2;
		gFrameClock = backFC;
	}
}



static std::unique_ptr<CGameMenuItemQAV> itemBloodQAV;	// This must be global to ensure that the animation remains consistent across menus.


void UpdateNetworkMenus(void)
{
	// For now disable the network menu item as it is not yet functional.
	for (auto name : { NAME_Mainmenu, NAME_IngameMenu })
	{
		FMenuDescriptor** desc = MenuDescriptors.CheckKey(name);
		if (desc != NULL && (*desc)->mType == MDESC_ListMenu)
		{
			FListMenuDescriptor* ld = static_cast<FListMenuDescriptor*>(*desc);
			for (auto& li : ld->mItems)
			{
				if (li->GetAction(nullptr) == NAME_MultiMenu)
				{
					li->mEnabled = false;
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//
// Implements the native looking menu used for the main menu
// and the episode/skill selection screens, i.e. the parts
// that need to look authentic
//
//----------------------------------------------------------------------------

class BloodListMenu : public DListMenu
{
	using Super = DListMenu;
protected:

	void PostDraw()
	{
		itemBloodQAV->Draw();
	}

};


//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

class BloodImageScreen : public ImageScreen
{
	CGameMenuItemQAV anim;
public:
	BloodImageScreen(FImageScrollerDescriptor::ScrollerItem* desc)
		: ImageScreen(desc), anim(169, 100, mDesc->text.GetChars(), false, true)
	{

	}

	void Drawer() override
	{
		anim.Draw();
	}
};

class DBloodImageScrollerMenu : public DImageScrollerMenu
{
	ImageScreen* newImageScreen(FImageScrollerDescriptor::ScrollerItem* desc) override
	{
		if (desc->type >= 0) return DImageScrollerMenu::newImageScreen(desc);
		return new BloodImageScreen(desc);
	}
};


//----------------------------------------------------------------------------
//
// Menu related game interface functions
//
//----------------------------------------------------------------------------

void GameInterface::DrawNativeMenuText(int fontnum, int state, double xpos, double ypos, float fontscale, const char* text, int flags)
{
	if (!text) return;
	int shade = (state != NIT_InactiveState) ? 32 : 48;
	int pal = (state != NIT_InactiveState) ? 5 : 5;
	if (state == NIT_SelectedState)	shade = 32 - ((int)totalclock & 63);
	int width, height;
	int gamefont = fontnum == NIT_BigFont ? 1 : fontnum == NIT_SmallFont ? 2 : 3;

	int x = int(xpos);
	int y = int(ypos);
	viewGetFontInfo(gamefont, text, &width, &height);

	if (flags & LMF_Centered)
	{
		x -= width / 2;
	}

	viewDrawText(gamefont, text, x, y, shade, pal, 0, true);
}


void GameInterface::MenuOpened()
{
	itemBloodQAV.reset(new CGameMenuItemQAV(160, 100, "BDRIP.QAV", true));
}

void GameInterface::MenuClosed()
{
	itemBloodQAV.reset();
}

bool GameInterface::CanSave()
{
	return (gGameStarted && gPlayer[myconnectindex].pXSprite->health != 0);
}

void GameInterface::StartGame(FNewGameStartup& gs)
{
	sfxKillAllSounds();
	gGameOptions.nDifficulty = gs.Skill;
	gGameOptions.nEpisode = gs.Episode;
	gSkill = gs.Skill;
	gGameOptions.nLevel = gs.Level;
	if (gDemo.at1)
		gDemo.StopPlayback();
	gStartNewGame = true;
	gCheatMgr.sub_5BCF4();
}

FSavegameInfo GameInterface::GetSaveSig()
{
	return { SAVESIG_BLD, MINSAVEVER_BLD, SAVEVER_BLD };
}

void GameInterface::DrawMenuCaption(const DVector2& origin, const char* text)
{
	int height;
	// font #1, tile #2038.
	viewGetFontInfo(1, NULL, NULL, &height);
	rotatesprite(int(origin.X * 65536) + (320 << 15), 20 << 16, 65536, 0, 2038, -128, 0, 78, 0, 0, xdim - 1, ydim - 1);
	viewDrawText(1, text, 160, 20 - height / 2, -128, 0, 1, false);
}

void GameInterface::DrawCenteredTextScreen(const DVector2& origin, const char* text, int position, bool bg)
{
	if (text)
	{
		int width, height = 0;
		viewGetFontInfo(0, "T", &width, &height);

		auto lines = FString(text).Split("\n");
		int y = 100 - (height * lines.Size() / 2);
		for (auto& l : lines)
		{
			int lheight = 0;
			viewGetFontInfo(0, l, &width, &lheight);
			int x = 160 - width / 2;
			viewDrawText(0, l, x, y, 0, 0, 0, false);
			y += height;
		}
	}
}

void GameInterface::QuitToTitle()
{
	if (gGameOptions.nGameType == 0 || numplayers == 1)
	{
		gQuitGame = true;
		gRestartGame = true;
	}
	else
		gQuitRequest = 2;
}

END_BLD_NS

//----------------------------------------------------------------------------
//
// Class registration
//
//----------------------------------------------------------------------------


static TMenuClassDescriptor<Blood::BloodListMenu> _lm("Blood.ListMenu");
static TMenuClassDescriptor<Blood::BloodListMenu> _mm("Blood.MainMenu");
static TMenuClassDescriptor<Blood::DBloodImageScrollerMenu> _im("Blood.ImageScrollerMenu");

void RegisterBloodMenus()
{
	menuClasses.Push(&_lm);
	menuClasses.Push(&_mm);
	menuClasses.Push(&_im);
}
