//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "ns.h"

#include "build.h"
#include "baselayer.h"
#include "osd.h"
#include "gamecvars.h"

#include "settings.h"
#include "mytypes.h"
#include "gamedefs.h"
#include "keyboard.h"
#include "gamecontrol.h"
#include "control.h"
#include "fx_man.h"
#include "sounds.h"
#include "config.h"
#include "common_game.h"

// we load this in to get default button and key assignments
// as well as setting up function mappings

#if defined RENDERTYPESDL && defined SDL_TARGET && SDL_TARGET > 1
# include "sdl_inc.h"
#endif

BEGIN_SW_NS

//
// Comm variables
//

int32_t NumberPlayers,CommPort,PortSpeed,IrqNumber,UartAddress;

//
// Sound variables
//


int32_t UseMouse = 1, UseJoystick = 0;


//
// Screen variables
//





/*
===================
=
= CONFIG_SetDefaults
=
===================
*/

void CONFIG_SetDefaults(void)
{
    ScreenMode = 1;

#if defined RENDERTYPESDL && SDL_MAJOR_VERSION > 1
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
#endif
    {
        ScreenWidth = 1024;
        ScreenHeight = 768;
    }

    memcpy(&gs, &gs_defaults, sizeof(gs));
}


void SetDefaultKeyDefinitions(int style)
{
	CONFIG_SetDefaultKeys(style ? "demolition/defbinds.txt" : "demolition/origbinds.txt");
}

/*
===================
=
= CONFIG_ReadSetup
=
===================
*/

int32_t CONFIG_ReadSetup(void)
{
    //char ret;
    extern char ds[];
    extern char PlayerNameArg[32];

    char waveformtrackname[MAXWAVEFORMTRACKLENGTH] = {0};

    CONFIG_SetDefaults();

    return 0;
}

END_SW_NS
