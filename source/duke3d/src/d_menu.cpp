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
#include "input.h"
#include "menus.h"
#include "osdcmds.h"
#include "savegame.h"
#include "game.h"
#include "superfasthash.h"
#include "gamecvars.h"
#include "gamecontrol.h"
#include "c_bind.h"
#include "menu/menu.h"
#include "../../glbackend/glbackend.h"

BEGIN_DUKE_NS

#define MENU_MARGIN_REGULAR 40
#define MENU_MARGIN_WIDE    32
#define MENU_MARGIN_CENTER  160
#define MENU_HEIGHT_CENTER  100


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


static void Menu_DrawTopBar(const vec2_t origin)
{
    if ((G_GetLogoFlags() & LOGO_NOTITLEBAR) == 0)
        rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16), origin.y + (19<<16), MF_Redfont.cursorScale, 0,MENUBAR,16,0,10);
}

static void Menu_DrawTopBarCaption(const char *caption, const vec2_t origin)
{
    static char t[64];
    size_t const srclen = strlen(caption);
    size_t const dstlen = min(srclen, ARRAY_SIZE(t)-1);
    memcpy(t, caption, dstlen);
    t[dstlen] = '\0';
    char *p = &t[dstlen-1];
    if (*p == ':')
        *p = '\0';
    captionmenutext(origin.x + (MENU_MARGIN_CENTER<<16), origin.y + (24<<16) + ((15>>1)<<16), t);
}

int GetMenuFontHeight(int fontnum)
{

}

int GetMenuTextWidth(int fontnum, const char* text)
{
}

void GameInterface::DrawNativeMenuText(int fontnum, int palnum, int xpos, int ypos, float fontscale, const char* text, int orientation)
{
}



class DukeListMenu : public DListMenu
{
	using Super = DListMenu;
protected:

	virtual void CallScript(int event, bool getorigin = false)
	{
		ud.returnvar[0] = origin.x;
		ud.returnvar[1] = origin.y;
		ud.returnvar[2] = mDesc->mSelectedItem;
		VM_OnEventWithReturn(event, g_player[screenpeek].ps->i, screenpeek, mDesc->mScriptId);
		if (getorigin)
		{
			origin.x = ud.returnvar[0];
			origin.y = ud.returnvar[1];
		}
	}

	void PreDraw() override
	{
		CallScript(CurrentMenu == this ? EVENT_DISPLAYMENU : EVENT_DISPLAYMENUREST, true);
	}

	void PostDraw() override
	{
		CallScript(CurrentMenu == this ? EVENT_DISPLAYMENUREST : EVENT_DISPLAYINACTIVEMENUREST, false);
	}

	void Drawer() override
	{
		auto v = origin;
		Super::Drawer();
		origin = v;
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

class MainMenu : public DukeListMenu
{
	void PreDraw() override
	{
		DukeListMenu::PreDraw();
        if ((G_GetLogoFlags() & LOGO_NOGAMETITLE) == 0)
        {
            rotatesprite_fs(origin.x + (MENU_MARGIN_CENTER<<16), origin.y + ((28)<<16), 65536L,0,INGAMEDUKETHREEDEE,0,0,10);
            if (PLUTOPAK)   // JBF 20030804
                rotatesprite_fs(origin.x + ((MENU_MARGIN_CENTER+100)<<16), origin.y + (36<<16), 65536L,0,PLUTOPAKSPRITE+2,(sintable[((int32_t) totalclock<<4)&2047]>>11),0,2+8);
        }
		else if (mDesc->mCaption.IsNotEmpty())
		{
			Menu_DrawTopBar(origin);
			Menu_DrawTopBarCaption(mDesc->mCaption, origin);
		}
	}	
};


END_DUKE_NS

static TMenuClassDescriptor<Duke::MainMenu> _mm("Duke.MainMenu");
static TMenuClassDescriptor<Duke::DukeListMenu> _lm("Duke.ListMenu");
static TMenuClassDescriptor<Duke::DukeNewGameCustomSubMenu> _ngcsm("Duke.NewGameCustomSubMenu");

void RegisterDukeMenus()
{
	menuClasses.Push(&_mm);
	menuClasses.Push(&_lm);
	menuClasses.Push(&_ngcsm);
}
