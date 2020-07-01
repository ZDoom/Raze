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

#include "cheats.h"
#include "compat.h"
#include "demo.h"
#include "duke3d.h"

#include "menus.h"
#include "osdcmds.h"
#include "savegame.h"
#include "game.h"
#include "superfasthash.h"
#include "gamecvars.h"
#include "gamecontrol.h"
#include "c_bind.h"
#include "menu/menu.h"
#include "gstrings.h"
#include "version.h"
#include "namesdyn.h"
#include "../../glbackend/glbackend.h"


BEGIN_DUKE_NS

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

static void Menu_DrawBackground(const DVector2 &origin)
{
	DrawTexture(twod, tileGetTexture(TILE_MENUSCREEN), origin.X + 160, origin.Y + 100, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_Color, 0xff808080, TAG_DONE);
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

static void Menu_DrawCursor(double x, double y, double scale, bool right)
{
	const int frames = isRR() ? 16 : 7;
	int picnum;
	if (!right) picnum = TILE_SPINNINGNUKEICON + (((int)totalclock >> 3) % frames);
	else picnum = TILE_SPINNINGNUKEICON + frames - 1 - ((frames - 1 + ((int)totalclock >> 3)) % frames);
	int light = int(224 + 31 * sin((int)totalclock / 20.));
	PalEntry pe(255, light, light, light);
	DrawTexture(twod, tileGetTexture(picnum), x, y, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_Color, pe, DTA_CenterOffset, true, TAG_DONE);
}

//----------------------------------------------------------------------------
//
// Implements the native looking menu used for the main menu
// and the episode/skill selection screens, i.e. the parts
// that need to look authentic
//
//----------------------------------------------------------------------------

class DukeListMenu : public DListMenu
{
	using Super = DListMenu;
protected:

	void Ticker() override
	{
		// Lay out the menu.
		int y_upper = mDesc->mYpos;
		int y_lower = y_upper + mDesc->mYbotton;
		int y = 0;
		int spacing = 0;
		int const height = 15; // cannot take value from the font because it would be inconsistent

		int totalheight = 0, numvalidentries = mDesc->mItems.Size();

		for (unsigned e = 0; e < mDesc->mItems.Size(); ++e)
		{
			auto entry = mDesc->mItems[e];
			entry->mHidden = false;
			entry->SetHeight(height);
			totalheight += height;
		}
		if (mDesc->mSpacing <= 0) spacing = std::max(0, (y_lower - y_upper - totalheight) / (numvalidentries > 1 ? numvalidentries - 1 : 1));
		if (spacing <= 0) spacing = mDesc->mSpacing;

		// totalHeight calculating pass
		int totalHeight;
		for (unsigned e = 0; e < mDesc->mItems.Size(); ++e)
		{
			auto entry = mDesc->mItems[e];
			if (!entry->mHidden)
			{
				entry->SetY(y_upper + y);
				y += height;
				totalHeight = y;
				y += spacing;
			}
		}
	}
};

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

class DukeMainMenu : public DukeListMenu
{
	virtual void Init(DMenu* parent = NULL, FListMenuDescriptor* desc = NULL) override
	{
		DukeListMenu::Init(parent, desc);
	}

	void PreDraw() override
	{
		DukeListMenu::PreDraw();
		if (RRRA)
		{
			rotatesprite_fs(int(origin.X * 65536) + ((160 - 5) << 16), int(origin.Y * 65536) + ((57) << 16), 16592L, 0, TILE_THREEDEE, 0, 0, 10);
		}
		else if (isRR())
		{
			rotatesprite_fs(int(origin.X * 65536) + ((160 + 5) << 16), int(origin.Y * 65536) + ((24) << 16), 23592L, 0, TILE_INGAMEDUKETHREEDEE, 0, 0, 10);
		}
		else
        {
            rotatesprite_fs(int(origin.X * 65536) + (160<<16), int(origin.Y * 65536) + ((28)<<16), 65536L,0,TILE_INGAMEDUKETHREEDEE,0,0,10);
            if (PLUTOPAK)   // JBF 20030804
                rotatesprite_fs(int(origin.X * 65536) + ((160+100)<<16), int(origin.Y * 65536) + (36<<16), 65536L,0,TILE_PLUTOPAKSPRITE+2,(sintable[((int32_t) totalclock<<4)&2047]>>11),0,2+8);
        }
		
	}
};


//----------------------------------------------------------------------------
//
// Menu related game interface functions
//
//----------------------------------------------------------------------------

void GameInterface::DrawNativeMenuText(int fontnum, int state, double oxpos, double ypos, float fontscale, const char* text, int flags)
{
	double xpos = oxpos;
	int trans;
	PalEntry pe;

	double scale = isRR() ? 0.4 : 1.;
	if (flags & LMF_Centered) xpos -= BigFont->StringWidth(text) * scale * 0.5;

	if (state == NIT_InactiveState)
	{
		trans = TRANSLATION(Translation_Remap, 1);
		pe = 0xffffffff;
	}
	else if (state == NIT_SelectedState)
	{
		trans = 0;
		int light = 224 + 31 * sin(int(totalclock) / 20.);
		pe = PalEntry(255, light, light, light);
	}
	else
	{
		trans = 0;
		pe = 0xffa0a0a0;
	}

	DrawText(twod, BigFont, CR_UNDEFINED, xpos, ypos, text, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_Color, pe,
		DTA_TranslationIndex, trans, TAG_DONE);

	if (state == NIT_SelectedState)
	{
		const int cursorOffset = 110;
		const double cursorScale = isRR() ? 0.2 : 1.0;
		const double ymid = ypos + 7;	// half height must be hardcoded or layouts will break.
		if (flags & LMF_Centered)
		{
			Menu_DrawCursor(oxpos + cursorOffset, ymid, cursorScale, false);
			Menu_DrawCursor(oxpos - cursorOffset, ymid, cursorScale, true);
		}
		else
			Menu_DrawCursor(oxpos - cursorOffset, ymid, cursorScale, false);
	}

}

void GameInterface::MenuOpened()
{
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
}

void GameInterface::MenuSound(EMenuSounds snd)
{
	switch (snd)
	{
	case ActivateSound:
		S_MenuSound();
		break;

	case CursorSound:
		S_PlaySound(isRR() ? 335 : KICK_HIT, CHAN_AUTO, CHANF_UI);
		break;

	case AdvanceSound:
		S_PlaySound(isRR() ? 341 : PISTOL_BODYHIT, CHAN_AUTO, CHANF_UI);
		break;

	case CloseSound:
		S_PlaySound(EXITMENUSOUND, CHAN_AUTO, CHANF_UI);
		break;

	default:
		return;
	}
}

void GameInterface::MenuClosed()
{

	auto& gm = g_player[myconnectindex].ps->gm;
	if (gm & MODE_GAME)
	{
		if (gm & MODE_MENU)
			inputState.ClearAllInput();

		// The following lines are here so that you cannot close the menu when no game is running.
		gm &= ~MODE_MENU;

		if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
		{
			ready2send = 1;
			totalclock = ototalclock;
			g_cameraClock = (int32_t)totalclock;
			g_cameraDistance = 65536;
		}

		G_UpdateScreenArea();
		S_PauseSounds(false);
	}
}

bool GameInterface::CanSave()
{
	if (ud.recstat == 2) return false;
	auto &myplayer = *g_player[myconnectindex].ps;
	if (sprite[myplayer.i].extra <= 0)
	{
		//P_DoQuote(QUOTE_SAVE_DEAD, &myplayer); // handled by the menu.
		return false;
	}
	return true;
}

void GameInterface::StartGame(FNewGameStartup& gs)
{
	int32_t skillsound = PISTOL_BODYHIT;

	soundEngine->StopAllChannels();

	if (!DEER)
	{

		switch (gs.Skill)
		{
		case 0:
			skillsound = isRR() ? 427 : JIBBED_ACTOR6;
			break;
		case 1:
			skillsound = isRR() ? 428 : BONUS_SPEECH1;
			break;
		case 2:
			skillsound = isRR() ? 196 : DUKE_GETWEAPON2;
			break;
		case 3:
			skillsound = isRR() ? 195 : JIBBED_ACTOR5;
			break;
		case 4:
			skillsound = isRR() ? 197 : JIBBED_ACTOR5; // Does not exist in DN3D.
			break;
		}
		ud.m_player_skill = gs.Skill + 1;
		if (menu_sounds && skillsound >= 0 && SoundEnabled())
		{
			S_PlaySound(skillsound, CHAN_AUTO, CHANF_UI);

			while (S_CheckSoundPlaying(skillsound))
			{
				S_Update();
				G_HandleAsync();
			}
		}
		ud.m_respawn_monsters = (gs.Skill == 3);
		ud.m_volume_number = gs.Episode;
		m_level_number = gs.Level;
	}
	else
	{
		ud.m_player_skill = 1;
		ud.m_respawn_monsters = 0;
		ud.m_volume_number = 0;
		m_level_number = gs.Episode; 
		g_player[myconnectindex].ps->dhat61f = gs.Skill;
	}

	ud.m_monsters_off = ud.monsters_off = 0;
	ud.m_respawn_items = 0;
	ud.m_respawn_inventory = 0;
	ud.multimode = 1;
	G_NewGame_EnterLevel();

}

FSavegameInfo GameInterface::GetSaveSig()
{
	return { SAVESIG_RR, MINSAVEVER_RR, SAVEVER_RR };
}

void GameInterface::DrawMenuCaption(const DVector2& origin, const char* text)
{
	DrawTexture(twod, tileGetTexture(TILE_MENUBAR), origin.X + 160, origin.Y + 19, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_Color, 0xff808080, DTA_CenterOffset, 1, TAG_DONE);

	FString t = text;
	size_t newlen = t.Len();
	if (t[t.Len() - 1] == ':') newlen--;
	if (newlen > 63) newlen = 63;
	t.Truncate(newlen);
	double scale = isRR() ? 0.4 : 1.0;
	double x = 160 + origin.X - BigFont->StringWidth(t) * scale * 0.5;
	DrawText(twod, BigFont, CR_UNTRANSLATED, x, origin.Y + 12, t, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);
}

void GameInterface::DrawCenteredTextScreen(const DVector2 &origin, const char *text, int position, bool bg)
{
	if (bg) Menu_DrawBackground(origin);
	else
	{
		// Only used for the confirmation screen.
		int lines = 1;
		for (int i = 0; text[i]; i++) if (text[i] == '\n') lines++;
		int height = lines * SmallFont->GetHeight();
		position -= height >> 17;
		if (!isRR()) Menu_DrawCursor(160, 130, 1, false);
	}
	G_ScreenText(MF_Bluefont.tilenum, int((origin.X + 160) * 65536), int((origin.Y + position) * 65536), MF_Bluefont.zoom, 0, 0, text, 0, MF_Bluefont.pal,
		2 | 8 | 16 | ROTATESPRITE_FULL16, 0, MF_Bluefont.emptychar.x, MF_Bluefont.emptychar.y, MF_Bluefont.between.x, MF_Bluefont.between.y,
		MF_Bluefont.textflags | TEXT_XCENTER, 0, 0, xdim - 1, ydim - 1);
}


void GameInterface::DrawPlayerSprite(const DVector2& origin, bool onteam)
{
	if (isRR())
		rotatesprite_fs(int(origin.X * 65536) + (260<<16), int(origin.Y * 65536) + ((24+(tilesiz[TILE_APLAYER].y>>2))<<16), 24576L,0,3845+36-((((8-((int32_t) totalclock>>4)))&7)*5),0,onteam ? G_GetTeamPalette(playerteam) : G_CheckPlayerColor(playercolor),10);
	else
		rotatesprite_fs(int(origin.X * 65536) + (260<<16), int(origin.Y * 65536) + ((24+(tilesiz[TILE_APLAYER].y>>1))<<16), 49152L,0,1441-((((4-((int32_t) totalclock>>4)))&3)*5),0,onteam ? G_GetTeamPalette(playerteam) : G_CheckPlayerColor(playercolor),10);
}

void GameInterface::QuitToTitle()
{
	g_player[myconnectindex].ps->gm = MODE_DEMO;
	artClearMapArt();
}

END_DUKE_NS

//----------------------------------------------------------------------------
//
// Class registration
//
//----------------------------------------------------------------------------


static TMenuClassDescriptor<Duke3d::DukeMainMenu> _mm("Duke.MainMenu");
static TMenuClassDescriptor<Duke3d::DukeListMenu> _lm("Duke.ListMenu");

static TMenuClassDescriptor<DImageScrollerMenu> _ism("Duke.ImageScrollerMenu"); // does not implement a new class, we only need the descriptor.

void RegisterDuke3dMenus()
{
	menuClasses.Push(&_mm);
	menuClasses.Push(&_lm);
	menuClasses.Push(&_ism);
}
