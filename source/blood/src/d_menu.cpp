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
#include "mouse.h"
#include "common_game.h"
#include "blood.h"
#include "config.h"
#include "gamemenu.h"
#include "globals.h"
#include "inifile.h"
#include "levels.h"
#include "menu.h"
#include "qav.h"
#include "resource.h"
#include "view.h"
#include "demo.h"
#include "network.h"
#include "c_bind.h"
#include "menu/menu.h"

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
		raw = kloadfile(name, 0);
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
		videoClearScreen(0);

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
/*
CGameMenuItemQAV itemCreditsQAV("", 3, 160, 100, "CREDITS", false, true);
CGameMenuItemQAV itemHelp3QAV("", 3, 160, 100, "HELP3", false, false);
CGameMenuItemQAV itemHelp3BQAV("", 3, 160, 100, "HELP3B", false, false);
CGameMenuItemQAV itemHelp4QAV("", 3, 160, 100, "HELP4", false, true);
CGameMenuItemQAV itemHelp5QAV("", 3, 160, 100, "HELP5", false, true);
*/


void UpdateNetworkMenus(void)
{
	// Kept as a reminder to reimplement later.
#if 0
	if (gGameOptions.nGameType > 0)
	{
		itemMain1.resource = &menuNetStart;
		itemMain1.data = 2;
	}
	else
	{
		itemMain1.resource = &menuEpisode;
		itemMain1.data = -1;
	}
	if (gGameOptions.nGameType > 0)
	{
		itemMainSave1.resource = &menuNetStart;
		itemMainSave1.data = 2;
	}
	else
	{
		itemMainSave1.resource = &menuEpisode;
		itemMainSave1.data = -1;
	}
#endif
}

void MenuSetupEpisodeInfo(void)
{
#if 0
	memset(zEpisodeNames, 0, sizeof(zEpisodeNames));
	memset(zLevelNames, 0, sizeof(zLevelNames));
	for (int i = 0; i < 6; i++)
	{
		if (i < gEpisodeCount)
		{
			EPISODEINFO* pEpisode = &gEpisodeInfo[i];
			zEpisodeNames[i] = pEpisode->at0;
			for (int j = 0; j < 16; j++)
			{
				if (j < pEpisode->nLevels)
				{
					zLevelNames[i][j] = pEpisode->data[j].at90;
				}
			}
		}
	}
#endif
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
#if 0
	S_PauseSounds(true);
	if ((!g_netServer && ud.multimode < 2))
	{
		ready2send = 0;
		totalclock = ototalclock;
		screenpeek = myconnectindex;
	}

	auto& gm = g_player[myconnectindex].ps->gm;
	if (gm & MODE_GAME)
	{
		gm |= MODE_MENU;
	}
#endif

	itemBloodQAV.reset(new CGameMenuItemQAV(160, 100, "BDRIP.QAV", true));
}

void GameInterface::MenuSound(EMenuSounds snd)
{
#if 0
	switch (snd)
	{
	case CursorSound:
		S_PlaySound(RR ? 335 : KICK_HIT);
		break;

	case AdvanceSound:
		S_PlaySound(RR ? 341 : PISTOL_BODYHIT);
		break;

	case CloseSound:
		S_PlaySound(EXITMENUSOUND);
		break;

	default:
		return;
	}
#endif
}

void GameInterface::MenuClosed()
{
	itemBloodQAV.reset();
#if 0
	auto& gm = g_player[myconnectindex].ps->gm;
	if (gm & MODE_GAME)
	{
		if (gm & MODE_MENU)
			I_ClearAllInput();

		// The following lines are here so that you cannot close the menu when no game is running.
		gm &= ~MODE_MENU;

		if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
		{
			ready2send = 1;
			totalclock = ototalclock;
			CAMERACLOCK = (int32_t)totalclock;
			CAMERADIST = 65536;

			// Reset next-viewscreen-redraw counter.
			// XXX: are there any other cases like that in need of handling?
			if (g_curViewscreen >= 0)
				actor[g_curViewscreen].t_data[0] = (int32_t)totalclock;
		}

		G_UpdateScreenArea();
		S_PauseSounds(false);
	}
#endif
}

bool GameInterface::CanSave()
{
#if 0
	if (ud.recstat == 2) return false;
	auto& myplayer = *g_player[myconnectindex].ps;
	if (sprite[myplayer.i].extra <= 0)
	{
		P_DoQuote(QUOTE_SAVE_DEAD, &myplayer);
		return false;
	}
#endif
	return true;
}

void GameInterface::StartGame(FGameStartup& gs)
{
#if 0
	int32_t skillsound = PISTOL_BODYHIT;

	switch (gs.Skill)
	{
	case 0:
		skillsound = 427;
		break;
	case 1:
		skillsound = 428;
		break;
	case 2:
		skillsound = 196;
		break;
	case 3:
		skillsound = 195;
		break;
	case 4:
		skillsound = 197;
		break;
	}

	ud.m_player_skill = gs.Skill + 1;
	if (menu_sounds) g_skillSoundVoice = S_PlaySound(skillsound);
	ud.m_respawn_monsters = (gs.Skill == 3);
	ud.m_monsters_off = ud.monsters_off = 0;
	ud.m_respawn_items = 0;
	ud.m_respawn_inventory = 0;
	ud.multimode = 1;
	ud.m_volume_number = gs.Episode;
	ud.m_level_number = gs.Level;
	G_NewGame_EnterLevel();
#endif
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

void GameInterface::DrawCenteredTextScreen(const DVector2& origin, const char* text, int position)
{
#if 0
	Menu_DrawBackground(origin);
	G_ScreenText(MF_Bluefont.tilenum, int((origin.X + 160) * 65536), int((origin.Y + position) * 65536), MF_Bluefont.zoom, 0, 0, text, 0, MF_Bluefont.pal,
		2 | 8 | 16 | ROTATESPRITE_FULL16, 0, MF_Bluefont.emptychar.x, MF_Bluefont.emptychar.y, MF_Bluefont.between.x, MF_Bluefont.between.y,
		MF_Bluefont.textflags | TEXT_XCENTER, 0, 0, xdim - 1, ydim - 1);
#endif
}


END_BLD_NS

//----------------------------------------------------------------------------
//
// Class registration
//
//----------------------------------------------------------------------------


static TMenuClassDescriptor<Blood::BloodListMenu> _lm("Blood.ListMenu");

void RegisterBloodMenus()
{
	menuClasses.Push(&_lm);
}
