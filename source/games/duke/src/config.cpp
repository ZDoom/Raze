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
#include "osdcmds.h"
#include "baselayer.h"
#include "cmdline.h"

// we load this in to get default button and key assignments
// as well as setting up function mappings

BEGIN_DUKE_NS

int32_t CONFIG_ReadSetup(void)
{
    g_player[0].ps->aim_mode = 1;
    ud.config.ShowOpponentWeapons = 0;
    ud.automsg = 0;
    ud.camerasprite = -1;

    ud.camera_time = 0;//4;

    ud.screen_tilting = 1;
    ud.statusbarflags = STATUSBAR_NOSHRINK;
    playerteam = 0;
    ud.angleinterpolation = 0;

    ud.display_bonus_screen = 1;
    ud.show_level_text = 1;
    ud.screenfade = 1;
    ud.menubackground = 1;
    ud.slidebar_paldisabled = 1;
    ud.shadow_pal = 4;
    return 0;
}

END_DUKE_NS
