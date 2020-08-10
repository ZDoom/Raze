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
#include "mmulti.h"
#include "c_bind.h"
#include "menu.h"
#include "gamestate.h"

#include "blood.h"
#include "globals.h"
#include "qav.h"
#include "view.h"
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
				data->Draw(twod, data->at10 - at2c, 10 + kQavOrientationLeft, 0, 0, 0, false);
				data->x += 320;
			}
			data->x = backX;
		}
		else
			data->Draw(twod, data->at10 - at2c, 10, 0, 0, 0, false);

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
	auto gamefont = fontnum == NIT_BigFont ? BigFont : fontnum == NIT_SmallFont ? SmallFont : SmallFont2;

	if (flags & LMF_Centered)
	{
		int width = gamefont->StringWidth(text);
		xpos -= width / 2;
	}
	DrawText(twod, gamefont, CR_UNDEFINED, xpos+1, ypos+1, text, DTA_Color, 0xff000000, //DTA_Alpha, 0.5,
			 DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, TAG_DONE);

	DrawText(twod, gamefont, CR_UNDEFINED, xpos, ypos, text, DTA_TranslationIndex, TRANSLATION(Translation_Remap, pal), DTA_Color, shadeToLight(shade),
			 DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, TAG_DONE);
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
	return (gamestate == GS_LEVEL && gPlayer[myconnectindex].pXSprite->health != 0);
}

void GameInterface::StartGame(FNewGameStartup& gs)
{
	sfxKillAllSounds();
	gGameOptions.nDifficulty = gs.Skill;
	gGameOptions.nEpisode = gs.Episode;
	gSkill = gs.Skill;
	gGameOptions.nLevel = gs.Level;
	gStartNewGame = true;
	cheatReset();
}

FSavegameInfo GameInterface::GetSaveSig()
{
	return { SAVESIG_BLD, MINSAVEVER_BLD, SAVEVER_BLD };
}

// This also gets used by the summary and the loading screen
void DrawMenuCaption(const char* text)
{
	double scalex = 1.; // Expand the box if the text is longer
	int width = BigFont->StringWidth(text);
	int boxwidth = tileWidth(2038);
	if (boxwidth - 10 < width) scalex = double(width) / (boxwidth - 10);
	
	DrawTexture(twod, tileGetTexture(2038, true), 160, 20, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_CenterOffsetRel, true, DTA_ScaleX, scalex, TAG_DONE);
	DrawText(twod, BigFont, CR_UNDEFINED, 160 - width/2, 20 - tileHeight(4193) / 2, text, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, TAG_DONE);
}

void GameInterface::DrawMenuCaption(const DVector2& origin, const char* text)
{
	Blood::DrawMenuCaption(text);
}

void GameInterface::DrawCenteredTextScreen(const DVector2& origin, const char* text, int position, bool bg)
{
	if (text)
	{
		int height = SmallFont->GetHeight();

		auto lines = FString(text).Split("\n");
		int y = 100 - (height * lines.Size() / 2);
		for (auto& l : lines)
		{
			int width = SmallFont->StringWidth(l);
			int x = 160 - width / 2;
			DrawText(twod, SmallFont, CR_UNTRANSLATED, x, y, l, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, TAG_DONE);
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
