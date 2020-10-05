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
#include "razemenu.h"
#include "gamestate.h"

#include "blood.h"
#include "globals.h"
#include "qav.h"
#include "view.h"
#include "sound.h"
#include "v_video.h"
#include "v_draw.h"

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
			lastTick = I_GetBuildTime();
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
		int backFC = gFrameClock;
		int currentclock = I_GetBuildTime();
		gFrameClock = currentclock;
		int nTicks = currentclock - lastTick;
		lastTick = currentclock;
		at2c -= nTicks;
		if (at2c <= 0 || at2c > data->at10)
		{
			at2c = data->at10;
		}
		data->Play(data->at10 - at2c - nTicks, data->at10 - at2c, -1, NULL);

		if (bWideScreen)
		{
			int xdim43 = scale(ydim, 4, 3);
			int nCount = (xdim + xdim43 - 1) / xdim43;
			int backX = data->x;
			for (int i = 0; i < nCount; i++)
			{
				data->Draw(data->at10 - at2c, 10 + kQavOrientationLeft, 0, 0, false);
				data->x += 320;
			}
			data->x = backX;
		}
		else
			data->Draw(data->at10 - at2c, 10, 0, 0, false);

		gFrameClock = backFC;
	}
}



static std::unique_ptr<CGameMenuItemQAV> itemBloodQAV;	// This must be global to ensure that the animation remains consistent across menus.


void UpdateNetworkMenus(void)
{
#if 0
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
#endif
}


#if 0
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
		// For narrow screens this would be mispositioned so skip drawing it there.
		double ratio = screen->GetWidth() / double(screen->GetHeight());
		if (ratio > 1.32) itemBloodQAV->Draw();
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

#endif

//----------------------------------------------------------------------------
//
// Menu related game interface functions
//
//----------------------------------------------------------------------------

void GameInterface::DrawNativeMenuText(int fontnum, int state, double xpos, double ypos, float fontscale, const char* text, int flags)
{
#if 0
	if (!text) return;
	int shade = (state != NIT_InactiveState) ? 32 : 48;
	int pal = (state != NIT_InactiveState) ? 5 : 5;
	if (state == NIT_SelectedState)	shade = 32 - (I_GetBuildTime() & 63);
	auto gamefont = fontnum == NIT_BigFont ? BigFont : SmallFont;

	if (flags & LMF_Centered)
	{
		int width = gamefont->StringWidth(text);
		xpos -= width / 2;
	}
	DrawText(twod, gamefont, CR_UNDEFINED, xpos+1, ypos+1, text, DTA_Color, 0xff000000, //DTA_Alpha, 0.5,
			 DTA_FullscreenScale, FSMode_Fit320x200, TAG_DONE);

	DrawText(twod, gamefont, CR_UNDEFINED, xpos, ypos, text, DTA_TranslationIndex, TRANSLATION(Translation_Remap, pal), DTA_Color, shadeToLight(shade),
			 DTA_FullscreenScale, FSMode_Fit320x200, TAG_DONE);
#endif
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

bool GameInterface::StartGame(FNewGameStartup& gs)
{
	if (gs.Episode >= 1)
	{
		if (g_gameType & GAMEFLAG_SHAREWARE)
		{
			M_StartMessage(GStrings("BUYBLOOD"), 1, NAME_None); // unreachable because we do not support Blood SW versions yet.
			return false;
		}
	}

	sfxKillAllSounds();
	auto map = FindMapByLevelNum(levelnum(gs.Episode, gs.Level));
	DeferedStartGame(map, gs.Skill);
	return true;
}

FSavegameInfo GameInterface::GetSaveSig()
{
	return { SAVESIG_BLD, MINSAVEVER_BLD, SAVEVER_BLD };
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
			DrawText(twod, SmallFont, CR_UNTRANSLATED, x, y, l, DTA_FullscreenScale, FSMode_Fit320x200, TAG_DONE);
			y += height;
		}
	}
}

void GameInterface::QuitToTitle()
{
	Mus_Stop();
	gameaction = ga_mainmenu;
}

END_BLD_NS

