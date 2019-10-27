//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
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

#include "duke3d.h"
#include "scriplib.h"
#include "osdcmds.h"
#include "renderlayer.h"
#include "cmdline.h"

#if defined RENDERTYPESDL && defined SDL_TARGET && SDL_TARGET > 1
# include "sdl_inc.h"
#endif

// we load this in to get default button and key assignments
// as well as setting up function mappings

#define __SETUP__   // JBF 20031211
#include "_functio.h"

BEGIN_RR_NS






void CONFIG_SetDefaults(void)
{

# if defined RENDERTYPESDL && SDL_MAJOR_VERSION > 1
    uint32_t inited = SDL_WasInit(SDL_INIT_VIDEO);
    if (inited == 0)
        SDL_Init(SDL_INIT_VIDEO);
    else if (!(inited & SDL_INIT_VIDEO))
        SDL_InitSubSystem(SDL_INIT_VIDEO);

    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) == 0)
    {
        ScreenWidth = dm.w;
        ScreenHeight = dm.h;
    }
    else
# endif
    {
        ScreenWidth = 1024;
        ScreenHeight = 768;
    }

    ScreenBPP = 32;
    g_player[0].ps->aim_mode = 1;
    ud.config.ShowOpponentWeapons = 0;
    ud.althud = 1;
    ud.automsg = 0;
    ud.camerasprite = -1;

	ud.camera_time = 0;//4;

    ud.color = 0;
    ud.detail = 0;
    ud.m_ffire = 1;
    ud.m_marker = 1;
    ud.menu_slidebarz = 65536;
    ud.menu_slidebarmargin = RR ? 6 * 65536 : 65536;
    ud.menu_slidecursorz = RR ? 32768 : 65536;
    ud.runkey_mode = 0;
    ud.screen_size = 4;
	ud.screen_tilting = 1;
    ud.statusbarflags = STATUSBAR_NOSHRINK;
    ud.statusbarmode = 1;
    ud.statusbarscale = 100;
    ud.team = 0;
    ud.angleinterpolation = 0;

    ScreenMode = 1;

    ud.display_bonus_screen = 1;
    ud.show_level_text = 1;
    ud.screenfade = 1;
    ud.menubackground = 1;
    ud.default_skill = 1;
    ud.slidebar_paldisabled = 1;
    ud.shadow_pal = 4;
    ud.menu_scrollbartilenum = -1;
    ud.menu_scrollbarz = 65536;
    ud.menu_scrollcursorz = 65536;
}


int32_t CONFIG_ReadSetup(void)
{
    CONFIG_SetDefaults();

    ud.config.setupread = 1;
    pathsearchmode = 1;

    pathsearchmode = 0;


    if (ScreenBPP < 8) ScreenBPP = 32;

    ud.config.setupread = 1;
    return 0;
}


void CONFIG_WriteSettings(void) // save binds and aliases to <cfgname>_settings.cfg
{
}



END_RR_NS
