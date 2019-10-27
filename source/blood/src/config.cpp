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


#include "baselayer.h"
#include "common_game.h"
#include "build.h"
#include "cache1d.h"
#include "sndcards.h"
#include "hash.h"
#include "scriplib.h"
#include "renderlayer.h"
#include "gamecontrol.h"
#include "blood.h"
#include "config.h"
#include "gamedefs.h"
#include "globals.h"
#include "screen.h"
#include "sound.h"
#include "tile.h"
#include "view.h"

#if defined RENDERTYPESDL && defined SDL_TARGET && SDL_TARGET > 1
# include "sdl_inc.h"
#endif

// we load this in to get default button and key assignments
// as well as setting up function mappings

#define __SETUP__   // JBF 20031211
#include "_functio.h"

BEGIN_BLD_NS


int32_t setupread;
int32_t mus_restartonload;
char szPlayerName[MAXPLAYERNAME];
int32_t gTurnSpeed;
int32_t gDetail;
int32_t cl_weaponswitch;
int32_t gAutoRun;
int32_t gFollowMap;
int32_t gOverlayMap;
int32_t gRotateMap;
int32_t gMessageCount;
int32_t gMessageTime;
int32_t gMessageFont;
int32_t gbAdultContent;
char gzAdultPassword[9];
int32_t gMouseSensitivity;
bool gNoClip;
bool gInfiniteAmmo;
bool gFullMap;
int32_t gUpscaleFactor;
int32_t gDeliriumBlur;

//////////
int gWeaponsV10x;
/////////






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
    {
        ScreenWidth = 1024;
        ScreenHeight = 768;
    }
#endif

#ifdef USE_OPENGL
    ScreenBPP = 32;
#else
    ScreenBPP = 8;
#endif
	
    ScreenMode       = 1;

    //snd_ambience  = 1;
    //ud.config.AutoAim         = 1;
    //ud.config.ShowWeapons     = 0;

    //ud.crosshair              = 1;
    //ud.default_skill          = 1;
    gUpscaleFactor = 0;
    //ud.display_bonus_screen   = 1;
    //adult_lockout                = 0;
    //ud.m_marker               = 1;
    //ud.maxautosaves           = 5;
    //ud.menu_scrollbartilenum  = -1;
    //ud.menu_scrollbarz        = 65536;
    //ud.menu_scrollcursorz     = 65536;
    //ud.menu_slidebarmargin    = 65536;
    //ud.menu_slidebarz         = 65536;
    //ud.menu_slidecursorz      = 65536;
    //ud.screen_size            = 4;
    //ud.screen_tilting         = 1;
    //ud.screenfade             = 1;
    //ud.shadow_pal             = 4;
    //ud.show_level_text        = 1;
    //ud.slidebar_paldisabled   = 1;
    //ud.statusbarflags         = STATUSBAR_NOSHRINK;
    //ud.statusbarmode          = 1;
    //ud.statusbarscale         = 100;
    //ud.team                   = 0;
    //cl_weaponswitch           = 3;  // new+empty
    gDeliriumBlur = 1;
    gViewSize = 2;
    gTurnSpeed = 92;
    gDetail = 4;
    gAutoRun = 0;
    gFollowMap = 1;
    gOverlayMap = 0;
    gRotateMap = 0;

    gMessageCount = 4;
    gMessageTime = 5;
    gMessageFont = 0;
    gbAdultContent = 0;
    gzAdultPassword[0] = 0;

    cl_weaponswitch = 1;

    Bstrcpy(szPlayerName, "Player");
}






int CONFIG_ReadSetup(void)
{
    CONFIG_SetDefaults();

    setupread = 1;
    pathsearchmode = 1;

    pathsearchmode = 0;

    if (ScreenBPP < 8) ScreenBPP = 32;

    setupread = 1;
    return 0;
}


void CONFIG_WriteSettings(void) // save binds and aliases to <cfgname>_settings.cfg
{
}



END_BLD_NS
