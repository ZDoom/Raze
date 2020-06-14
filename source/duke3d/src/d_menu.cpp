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
#include "menu.h"
#include "gstrings.h"
#include "version.h"
#include "namesdyn.h"
#include "menus.h"
#include "../../glbackend/glbackend.h"

BEGIN_DUKE_NS

#define MENU_MARGIN_REGULAR 40
#define MENU_MARGIN_WIDE    32
#define MENU_MARGIN_CENTER  160
#define MENU_HEIGHT_CENTER  100


enum MenuTextFlags_t
{
	MT_Selected = 1 << 0,
	MT_Disabled = 1 << 1,
	MT_XCenter = 1 << 2,
	MT_XRight = 1 << 3,
	MT_YCenter = 1 << 4,
	MT_Literal = 1 << 5,
	MT_RightSide = 1 << 6,
};


// common font types
// tilenums are set after namesdyn runs.
// These are also modifiable by scripts.
//                                      emptychar x,y       between x,y         zoom                cursorLeft          cursorCenter        cursorScale         textflags
//                                      tilenum             shade_deselected    shade_disabled      pal                 pal_selected        pal_deselected      pal_disabled
MenuFont_t MF_Redfont =               { { 5<<16, 15<<16 },  { 0, 0 },           65536,              20<<16,             110<<16,            65536,              TEXT_BIGALPHANUM | TEXT_UPPERCASE,
                                        -1,                 10,                 0,                  0,                  0,                  0,                  1,
                                        0,                  0,                  1 };
MenuFont_t MF_Bluefont =              { { 5<<16, 7<<16 },   { 0, 0 },           65536,              10<<16,             110<<16,            32768,              0,
                                        -1,                 10,                 0,                  0,                  10,                 10,                 16,
                                        0,                  0,                  16 };
MenuFont_t MF_Minifont =              { { 4<<16, 5<<16 },   { 1<<16, 1<<16 },   65536,              10<<16,             110<<16,            32768,              0,
                                        -1,                 10,                 0,                  0,                  2,                  2,                  0,
                                        0,                  0,                  16 };


/*
This function prepares data after ART and CON have been processed.
It also initializes some data in loops rather than statically at compile time.
*/
void Menu_Init(void)
{

	// prepare menu fonts
	// check if tilenum is -1 in case it was set in EVENT_SETDEFAULTS
	if ((unsigned)MF_Redfont.tilenum >= MAXTILES) MF_Redfont.tilenum = BIGALPHANUM;
	if ((unsigned)MF_Bluefont.tilenum >= MAXTILES) MF_Bluefont.tilenum = STARTALPHANUM;
	if ((unsigned)MF_Minifont.tilenum >= MAXTILES) MF_Minifont.tilenum = MINIFONT;
	MF_Redfont.emptychar.y = tilesiz[MF_Redfont.tilenum].y << 16;
	MF_Bluefont.emptychar.y = tilesiz[MF_Bluefont.tilenum].y << 16;
	MF_Minifont.emptychar.y = tilesiz[MF_Minifont.tilenum].y << 16;
	if (!minitext_lowercase)
		MF_Minifont.textflags |= TEXT_UPPERCASE;


}

static void Menu_DrawBackground(const DVector2 &origin)
{
	rotatesprite_fs(int(origin.X * 65536) + (MENU_MARGIN_CENTER << 16), int(origin.Y * 65536) + (100 << 16), 65536L, 0, MENUSCREEN, 16, 0, 10 + 64);
}

static void Menu_DrawTopBar(const DVector2 &origin)
{
    if ((G_GetLogoFlags() & LOGO_NOTITLEBAR) == 0)
        rotatesprite_fs(int(origin.X*65536) + (MENU_MARGIN_CENTER<<16), int(origin.Y*65536) + (19<<16), MF_Redfont.cursorScale, 0,MENUBAR,16,0,10);
}

static void Menu_DrawTopBarCaption(const char *caption, const DVector2 &origin)
{
    static char t[64];
    size_t const srclen = strlen(caption);
    size_t const dstlen = min(srclen, ARRAY_SIZE(t)-1);
    memcpy(t, caption, dstlen);
    t[dstlen] = '\0';
    char *p = &t[dstlen-1];
    if (*p == ':')
        *p = '\0';
    captionmenutext(int(origin.X*65536) + (MENU_MARGIN_CENTER<<16), int(origin.Y*65536) + (24<<16) + ((15>>1)<<16), t);
}

static void Menu_GetFmt(const MenuFont_t* font, uint8_t const status, int32_t* s, int32_t* z)
{
	if (status & MT_Selected)
		*s = VM_OnEventWithReturn(EVENT_MENUSHADESELECTED, -1, myconnectindex, sintable[((int32_t)totalclock << 5) & 2047] >> 12);
	else
		*s = font->shade_deselected;
	// sum shade values
	if (status & MT_Disabled)
		*s += font->shade_disabled;

	if (FURY && status & MT_Selected)
		*z += (*z >> 4);
}

static vec2_t Menu_Text(int32_t x, int32_t y, const MenuFont_t* font, const char* t, uint8_t status, int32_t ydim_upper, int32_t ydim_lower)
{
	int32_t s, p, ybetween = font->between.y;
	int32_t f = font->textflags;
	if (status & MT_XCenter)
		f |= TEXT_XCENTER;
	if (status & MT_XRight)
		f |= TEXT_XRIGHT;
	if (status & MT_YCenter)
	{
		f |= TEXT_YCENTER | TEXT_YOFFSETZERO;
		ybetween = font->emptychar.y; // <^ the battle against 'Q'
	}
	if (status & MT_Literal)
		f |= TEXT_LITERALESCAPE;

	int32_t z = font->zoom;

	if (status & MT_Disabled)
		p = (status & MT_RightSide) ? font->pal_disabled_right : font->pal_disabled;
	else if (status & MT_Selected)
		p = (status & MT_RightSide) ? font->pal_selected_right : font->pal_selected;
	else
		p = (status & MT_RightSide) ? font->pal_deselected_right : font->pal_deselected;

	Menu_GetFmt(font, status, &s, &z);

	return G_ScreenText(font->tilenum, x, y, z, 0, 0, t, s, p, 2 | 8 | 16 | ROTATESPRITE_FULL16, 0, font->emptychar.x, font->emptychar.y, font->between.x, ybetween, f, 0, ydim_upper, xdim - 1, ydim_lower);
}

static vec2_t mgametextcenterat(int32_t x, int32_t y, char const* t, int32_t f = 0)
{
	return G_ScreenText(MF_Bluefont.tilenum, x, y, MF_Bluefont.zoom, 0, 0, t, 0, MF_Bluefont.pal, 2 | 8 | 16 | ROTATESPRITE_FULL16, 0, MF_Bluefont.emptychar.x, MF_Bluefont.emptychar.y, MF_Bluefont.between.x, MF_Bluefont.between.y, MF_Bluefont.textflags | f | TEXT_XCENTER, 0, 0, xdim - 1, ydim - 1);
}
static vec2_t mgametextcenter(int32_t x, int32_t y, char const* t, int32_t f = 0)
{
	return mgametextcenterat((MENU_MARGIN_CENTER << 16) + x, y, t, f);
}

static int32_t Menu_CursorShade(void)
{
	return VM_OnEventWithReturn(EVENT_MENUCURSORSHADE, -1, myconnectindex, 4 - (sintable[((int32_t)totalclock << 4) & 2047] >> 11));
}

static void Menu_DrawCursorCommon(int32_t x, int32_t y, int32_t z, int32_t picnum, int32_t ydim_upper = 0, int32_t ydim_lower = ydim - 1)
{
	rotatesprite_(x, y, z, 0, picnum, Menu_CursorShade(), 0, 2 | 8, 0, 0, 0, ydim_upper, xdim - 1, ydim_lower);
}

static void Menu_DrawCursorLeft(int32_t x, int32_t y, int32_t z)
{
	if (FURY) return;
	Menu_DrawCursorCommon(x, y, z, VM_OnEventWithReturn(EVENT_MENUCURSORLEFT, -1, myconnectindex, SPINNINGNUKEICON + (((int32_t)totalclock >> 3) % 7)));
}

static void Menu_DrawCursorRight(int32_t x, int32_t y, int32_t z)
{
	if (FURY) return;
	Menu_DrawCursorCommon(x, y, z, VM_OnEventWithReturn(EVENT_MENUCURSORRIGHT, -1, myconnectindex, SPINNINGNUKEICON + 6 - ((6 + ((int32_t)totalclock >> 3)) % 7)));
}

static int Menu_GetFontHeight(int fontnum)
{
	auto& font = fontnum == NIT_BigFont ? MF_Redfont : fontnum == NIT_SmallFont ? MF_Bluefont : MF_Minifont;
	return font.get_yline();
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

	void SelectionChanged() override
	{
		if (mDesc->mScriptId == 110)
		{
			// Hack alert: Ion Fury depends on the skill getting set globally when the selection changes because the script cannot detect actual selection changes.
			// Yuck!
			ud.m_player_skill = mDesc->mSelectedItem+1;
		}
	}

	virtual void CallScript(int event, bool getorigin = false)
	{
		ud.returnvar[0] = int(origin.X * 65536);
		ud.returnvar[1] = int(origin.Y * 65536);
		ud.returnvar[2] = mDesc->mSelectedItem;
		VM_OnEventWithReturn(event, g_player[screenpeek].ps->i, screenpeek, mDesc->mScriptId);
		if (getorigin)
		{
			origin.X = ud.returnvar[0] / 65536.;
			origin.Y = ud.returnvar[1] / 65536.;
		}
	}

	void Ticker() override
	{
		auto lf = G_GetLogoFlags();
		help_disabled = (lf & LOGO_NOHELP);
		credits_disabled = (lf & LOGO_NOCREDITS);

		// Lay out the menu. Since scripts are allowed to mess around with the font this needs to be redone each frame.
		int32_t y_upper = mDesc->mYpos;
		int32_t y_lower = y_upper + mDesc->mYbotton;
		int32_t y = 0;
		int32_t calculatedentryspacing = 0;
		int32_t const height = Menu_GetFontHeight(mDesc->mNativeFontNum) >> 16;

		int32_t totalheight = 0, numvalidentries = mDesc->mItems.Size();

		for (unsigned e = 0; e < mDesc->mItems.Size(); ++e)
		{
			auto entry = mDesc->mItems[e];
			entry->mHidden = false;
			if (entry->GetAction(nullptr) == NAME_HelpMenu && help_disabled)
			{
				entry->mHidden = true;
				numvalidentries--;
				continue;
			}
			else if (entry->GetAction(nullptr) == NAME_CreditsMenu && credits_disabled)
			{
				entry->mHidden = true;
				numvalidentries--;
				continue;
			}
			entry->SetHeight(height);
			totalheight += height;
		}
		if (mDesc->mSpacing <= 0) calculatedentryspacing = std::max(0, (y_lower - y_upper - totalheight) / (numvalidentries > 1 ? numvalidentries - 1 : 1));
		if (calculatedentryspacing <= 0) calculatedentryspacing = mDesc->mSpacing;


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
				y += calculatedentryspacing;
			}
		}
	}

	void PreDraw() override
	{
		CallScript(CurrentMenu == this ? EVENT_DISPLAYMENU : EVENT_DISPLAYINACTIVEMENU, true);
		Super::PreDraw();
	}

	void PostDraw() override
	{
		CallScript(CurrentMenu == this ? EVENT_DISPLAYMENUREST : EVENT_DISPLAYINACTIVEMENUREST, false);
	}
};

class DukeNewGameCustomSubMenu : public DukeListMenu
{
	virtual void CallScript(int event, bool getorigin) override
	{
		// This needs to get the secondary ID to the script.
		ud.returnvar[3] = mDesc->mSecondaryId;
		DukeListMenu::CallScript(event, getorigin);
	}
};

class DukeMainMenu : public DukeListMenu
{
	void PreDraw() override
	{
		DukeListMenu::PreDraw();
        if ((G_GetLogoFlags() & LOGO_NOGAMETITLE) == 0)
        {
            rotatesprite_fs(int(origin.X * 65536) + (MENU_MARGIN_CENTER<<16), int(origin.Y * 65536) + ((28)<<16), 65536L,0,INGAMEDUKETHREEDEE,0,0,10);
            if (PLUTOPAK)   // JBF 20030804
                rotatesprite_fs(int(origin.X * 65536) + ((MENU_MARGIN_CENTER+100)<<16), int(origin.Y * 65536) + (36<<16), 65536L,0,PLUTOPAKSPRITE+2,(sintable[((int32_t) totalclock<<4)&2047]>>11),0,2+8);
        }
	}	
};

//----------------------------------------------------------------------------
//
// Hack to display Ion Fury's credits screens
//
//----------------------------------------------------------------------------

class DukeImageScreen : public ImageScreen
{
public:
	DukeImageScreen(FImageScrollerDescriptor::ScrollerItem* desc)
		: ImageScreen(desc)
	{}

	void CallScript(int event, bool getorigin = false)
	{
		ud.returnvar[0] = int(origin.X * 65536);
		ud.returnvar[1] = int(origin.Y * 65536);
		ud.returnvar[2] = 0;
		VM_OnEventWithReturn(event, g_player[screenpeek].ps->i, screenpeek, mDesc->scriptID);
		if (getorigin)
		{
			origin.X = ud.returnvar[0] / 65536.;
			origin.Y = ud.returnvar[1] / 65536.;
		}
	}

	void Drawer() override
	{
		// Hack alert: The Ion Fury scripts - being true to the entire design here, take the current menu value
		// not from the passed variable but instead from the global current_menu, so we have to temporarily alter that here.
		// Ugh. (Talk about "broken by design"...)
		auto cm = g_currentMenu;
		g_currentMenu = mDesc->scriptID;
		auto o = origin;
		CallScript(EVENT_DISPLAYMENU, true);
		ImageScreen::Drawer();
		CallScript(EVENT_DISPLAYMENUREST, false);
		g_currentMenu = cm;
		origin = o;
	}
};

class DDukeImageScrollerMenu : public DImageScrollerMenu
{
	ImageScreen* newImageScreen(FImageScrollerDescriptor::ScrollerItem* desc) override
	{
		return new DukeImageScreen(desc);
	}
};

//----------------------------------------------------------------------------
//
// Menu related game interface functions
//
//----------------------------------------------------------------------------

void GameInterface::DrawNativeMenuText(int fontnum, int state, double xpos, double ypos, float fontscale, const char* text, int flags)
{
	int ydim_upper = 0;
	int ydim_lower = ydim - 1;
	//int32_t const indent = 0;	// not set for any relevant menu
	int x = int(xpos * 65536);

	uint8_t status = 0;
	if (state == NIT_SelectedState)
		status |= MT_Selected;
	if (state == NIT_InactiveState)
		status |= MT_Disabled;
	if (flags & LMF_Centered)
		status |= MT_XCenter;

	bool const dodraw = true;
	MenuFont_t& font = fontnum == NIT_BigFont ? MF_Redfont : fontnum == NIT_SmallFont ? MF_Bluefont : MF_Minifont;

	int32_t const height = font.get_yline();
	status |= MT_YCenter;
	int32_t const y_internal = int(ypos * 65536) + ((height >> 17) << 16);// -menu->scrollPos;

	vec2_t textsize;
	if (dodraw)
		textsize = Menu_Text(x, y_internal, &font, text, status, ydim_upper, ydim_lower);

	if (dodraw && (status & MT_Selected) && state != 1)
	{
		if (status & MT_XCenter)
		{
			Menu_DrawCursorLeft(x + font.cursorCenterPosition, y_internal, font.cursorScale);
			Menu_DrawCursorRight(x - font.cursorCenterPosition, y_internal, font.cursorScale);
		}
		else
			Menu_DrawCursorLeft(x /*+ indent*/ - font.cursorLeftPosition, y_internal, font.cursorScale);
	}

}

void GameInterface::MenuOpened()
{
	S_PauseSound(true, false);
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
		S_PlaySound(KICK_HIT, CHAN_AUTO, CHANF_UI);
		break;

	case AdvanceSound:
		S_PlaySound(PISTOL_BODYHIT, CHAN_AUTO, CHANF_UI);
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
			CAMERACLOCK = (int32_t)totalclock;
			CAMERADIST = 65536;

			// Reset next-viewscreen-redraw counter.
			// XXX: are there any other cases like that in need of handling?
			if (g_curViewscreen >= 0)
				actor[g_curViewscreen].t_data[0] = (int32_t)totalclock;
		}

		G_UpdateScreenArea();
		S_ResumeSound(false);
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

void GameInterface::CustomMenuSelection(int menu, int item)
{
	ud.returnvar[0] = item;
	ud.returnvar[1] = -1;
	VM_OnEventWithReturn(EVENT_NEWGAMECUSTOM, -1, myconnectindex, menu);
}

void GameInterface::StartGame(FNewGameStartup& gs)
{
	int32_t skillsound = PISTOL_BODYHIT;

	soundEngine->StopAllChannels();
	switch (gs.Skill)
	{
	case 0:
		skillsound = JIBBED_ACTOR6;
		break;
	case 1:
		skillsound = BONUS_SPEECH1;
		break;
	case 2:
		skillsound = DUKE_GETWEAPON2;
		break;
	case 3:
		skillsound = JIBBED_ACTOR5;
		break;
	}

	ud.m_player_skill = gs.Skill + 1;
	if (menu_sounds && skillsound >= 0 && SoundEnabled())
	{
		S_PlaySound(skillsound, CHAN_AUTO, CHANF_UI);

		while (S_CheckSoundPlaying(skillsound))
		{
			S_Update();
			gameHandleEvents();
		}
	}
	ud.m_respawn_monsters = (gs.Skill == 3);
	ud.m_monsters_off = ud.monsters_off = 0;
	ud.m_respawn_items = 0;
	ud.m_respawn_inventory = 0;
	ud.multimode = 1;
	ud.m_volume_number = gs.Episode;
	m_level_number = gs.Level;
	G_NewGame_EnterLevel();

}

FSavegameInfo GameInterface::GetSaveSig()
{
	return { SAVESIG_DN3D, MINSAVEVER_DN3D, SAVEVER_DN3D };
}

void GameInterface::DrawMenuCaption(const DVector2& origin, const char* text)
{
	Menu_DrawTopBar(origin);
	Menu_DrawTopBarCaption(text, origin);
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

static void shadowminitext(int32_t x, int32_t y, const char* t, int32_t p)
{
	int32_t f = 0;

	if (!minitext_lowercase)
		f |= TEXT_UPPERCASE;

	G_ScreenTextShadow(1, 1, MINIFONT, x, y, 65536, 0, 0, t, 0, p, 2 | 8 | 16 | ROTATESPRITE_FULL16, 0, 4 << 16, 8 << 16, 1 << 16, 0, f, 0, 0, xdim - 1, ydim - 1);
}

//----------------------------------------------------------------------------
//
// allows the front end to override certain fullscreen image menus
// with custom implementations.
//
// This is needed because the credits screens in Duke Nukem
// are either done by providing an image or by printing text, based on the version used.
//
//----------------------------------------------------------------------------

bool GameInterface::DrawSpecialScreen(const DVector2 &origin, int tilenum)
{
	// Older versions of Duke Nukem create the credits screens manually.
	// On the latest version there's real graphics for this.
	bool haveCredits = VOLUMEALL && PLUTOPAK;
	
	int32_t m, l;
	if (!haveCredits)
	{
		if (tilenum == CREDITSTEXT1)
		{
			Menu_DrawBackground(origin);
			m = int(origin.X * 65536) + (20 << 16);
			l = int(origin.Y * 65536) + (33 << 16);
			
			shadowminitext(m, l, "Original Concept", 12); l += 7 << 16;
			shadowminitext(m, l, "Todd Replogle and Allen H. Blum III", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "Produced & Directed By", 12); l += 7 << 16;
			shadowminitext(m, l, "Greg Malone", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "Executive Producer", 12); l += 7 << 16;
			shadowminitext(m, l, "George Broussard", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "BUILD Engine", 12); l += 7 << 16;
			shadowminitext(m, l, "Ken Silverman", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "Game Programming", 12); l += 7 << 16;
			shadowminitext(m, l, "Todd Replogle", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "3D Engine/Tools/Net", 12); l += 7 << 16;
			shadowminitext(m, l, "Ken Silverman", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "Network Layer/Setup Program", 12); l += 7 << 16;
			shadowminitext(m, l, "Mark Dochtermann", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "Map Design", 12); l += 7 << 16;
			shadowminitext(m, l, "Allen H. Blum III", 12); l += 7 << 16;
			shadowminitext(m, l, "Richard Gray", 12); l += 7 << 16;
			
			m = int(origin.X * 65536) + (180 << 16);
			l = int(origin.Y * 65536) + (33 << 16);
			
			shadowminitext(m, l, "3D Modeling", 12); l += 7 << 16;
			shadowminitext(m, l, "Chuck Jones", 12); l += 7 << 16;
			shadowminitext(m, l, "Sapphire Corporation", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "Artwork", 12); l += 7 << 16;
			shadowminitext(m, l, "Dirk Jones, Stephen Hornback", 12); l += 7 << 16;
			shadowminitext(m, l, "James Storey, David Demaret", 12); l += 7 << 16;
			shadowminitext(m, l, "Douglas R. Wood", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "Sound Engine", 12); l += 7 << 16;
			shadowminitext(m, l, "Jim Dose", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "Sound & Music Development", 12); l += 7 << 16;
			shadowminitext(m, l, "Robert Prince", 12); l += 7 << 16;
			shadowminitext(m, l, "Lee Jackson", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "Voice Talent", 12); l += 7 << 16;
			shadowminitext(m, l, "Lani Minella - Voice Producer", 12); l += 7 << 16;
			shadowminitext(m, l, "Jon St. John as \"Duke Nukem\"", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "Graphic Design", 12); l += 7 << 16;
			shadowminitext(m, l, "Packaging, Manual, Ads", 12); l += 7 << 16;
			shadowminitext(m, l, "Robert M. Atkins", 12); l += 7 << 16;
			shadowminitext(m, l, "Michael Hadwin", 12); l += 7 << 16;
			return true;
		}
		else if (tilenum == CREDITSTEXT2__STATIC)
		{
			Menu_DrawBackground(origin);
			m = int(origin.X * 65536) + (20 << 16);
			l = int(origin.Y * 65536) + (33 << 16);
			
			shadowminitext(m, l, "Special Thanks To", 12); l += 7 << 16;
			shadowminitext(m, l, "Steven Blackburn, Tom Hall", 12); l += 7 << 16;
			shadowminitext(m, l, "Scott Miller, Joe Siegler", 12); l += 7 << 16;
			shadowminitext(m, l, "Terry Nagy, Colleen Compton", 12); l += 7 << 16;
			shadowminitext(m, l, "HASH, Inc., FormGen, Inc.", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "The 3D Realms Beta Testers", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "Nathan Anderson, Wayne Benner", 12); l += 7 << 16;
			shadowminitext(m, l, "Glenn Brensinger, Rob Brown", 12); l += 7 << 16;
			shadowminitext(m, l, "Erik Harris, Ken Heckbert", 12); l += 7 << 16;
			shadowminitext(m, l, "Terry Herrin, Greg Hively", 12); l += 7 << 16;
			shadowminitext(m, l, "Hank Leukart, Eric Baker", 12); l += 7 << 16;
			shadowminitext(m, l, "Jeff Rausch, Kelly Rogers", 12); l += 7 << 16;
			shadowminitext(m, l, "Mike Duncan, Doug Howell", 12); l += 7 << 16;
			shadowminitext(m, l, "Bill Blair", 12); l += 7 << 16;
			
			m = int(origin.X * 65536) + (160 << 16);
			l = int(origin.Y * 65536) + (33 << 16);
			
			shadowminitext(m, l, "Company Product Support", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "The following companies were cool", 12); l += 7 << 16;
			shadowminitext(m, l, "enough to give us lots of stuff", 12); l += 7 << 16;
			shadowminitext(m, l, "during the making of Duke Nukem 3D.", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "Altec Lansing Multimedia", 12); l += 7 << 16;
			shadowminitext(m, l, "for tons of speakers and the", 12); l += 7 << 16;
			shadowminitext(m, l, "THX-licensed sound system.", 12); l += 7 << 16;
			shadowminitext(m, l, "For info call 1-800-548-0620", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "Creative Labs, Inc.", 12); l += 7 << 16;
			l += 3 << 16;
			shadowminitext(m, l, "Thanks for the hardware, guys.", 12); l += 7 << 16;
			return true;
		}
		else if (tilenum == CREDITSTEXT3)
		{
			Menu_DrawBackground(origin);
			mgametextcenter(int(origin.X * 65536), int(origin.Y * 65536) + (50 << 16), "Duke Nukem 3D is a trademark of\n"
							"3D Realms Entertainment"
							"\n"
							"Duke Nukem 3D\n"
							"(C) 1996 3D Realms Entertainment");
			
			if (VOLUMEONE)
			{
				mgametextcenter(int(origin.X * 65536), int(origin.Y * 65536) + (106 << 16), "Please read LICENSE.DOC for shareware\n"
								"distribution grants and restrictions.");
			}
			mgametextcenter(int(origin.X * 65536), int(origin.Y * 65536) + ((VOLUMEONE ? 134 : 115) << 16), "Made in Dallas, Texas USA");
			return true;
		}
	}
	return false;
}


void GameInterface::DrawCenteredTextScreen(const DVector2 &origin, const char *text, int position, bool bg)
{
	if (bg) Menu_DrawBackground(origin);
	else
	{
		// Only used for the confirmation screen.
		int lines = 1;
		for (int i = 0; text[i]; i++) if (text[i] == '\n') lines++;
		int height = lines * Menu_GetFontHeight(NIT_SmallFont);
		position -= height >> 17;
		Menu_DrawCursorLeft(160 << 16, 130 << 16, 65536);
	}
	mgametextcenter(int(origin.X * 65536), int((origin.Y + position) * 65536), text);
}

void GameInterface::DrawPlayerSprite(const DVector2& origin, bool onteam)
{
	rotatesprite_fs(int(origin.X * 65536) + (260<<16), int(origin.Y*65536) + ((24+(tilesiz[APLAYER].y>>1))<<16), 49152L,0,1441-((((4-((int32_t) totalclock>>4)))&3)*5),0,onteam ? G_GetTeamPalette(playerteam) : G_CheckPlayerColor(playercolor),10);
}

void GameInterface::QuitToTitle()
{
	g_player[myconnectindex].ps->gm = MODE_DEMO;
	if (ud.recstat == 1)
		G_CloseDemoWrite();
	artClearMapArt();
}
END_DUKE_NS

//----------------------------------------------------------------------------
//
// Class registration
//
//----------------------------------------------------------------------------


static TMenuClassDescriptor<Duke::DukeMainMenu> _mm("Duke.MainMenu");
static TMenuClassDescriptor<Duke::DukeListMenu> _lm("Duke.ListMenu");
static TMenuClassDescriptor<Duke::DukeNewGameCustomSubMenu> _ngcsm("Duke.NewGameCustomSubMenu");
static TMenuClassDescriptor<Duke::DDukeImageScrollerMenu> _ism("Duke.ImageScrollerMenu");

void RegisterDukeMenus()
{
	menuClasses.Push(&_mm);
	menuClasses.Push(&_lm);
	menuClasses.Push(&_ngcsm);
	menuClasses.Push(&_ism);
}
