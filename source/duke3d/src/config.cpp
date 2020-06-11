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

BEGIN_EDUKE_NS




int CONFIG_ReadSetup(void)
{
    ud.camera_time = 0;//4;

    g_player[0].ps->aim_mode = 1;

    ud.althud = 1;
    ud.angleinterpolation = 0;
    ud.camerasprite = -1;
    ud.config.ShowWeapons = 0;
    ud.display_bonus_screen = 1;

    hud_position = 0;
    ud.menubackground = 1;
    ud.screen_size = 4;
    ud.screen_tilting = 1;
    ud.screenfade = 1;
    ud.shadow_pal = 4;
    ud.show_level_text = 1;
    ud.slidebar_paldisabled = 1;
    ud.statusbarflags = 0;//STATUSBAR_NOSHRINK;
    ud.statusbarmode = 1;

    ud.god = 0;
    ud.m_respawn_items = 0;
    ud.m_respawn_monsters = 0;
    ud.m_respawn_inventory = 0;
    ud.warp_on = 0;
    ud.cashman = 0;
    m_ffire = 1;
    ud.m_player_skill = ud.player_skill = 2;
    memcpy(g_player[0].wchoice, "\3\4\5\7\0x8\6\0\2\0x9\1", 10);
    wchoice.Callback();
    return 0;
}

END_EDUKE_NS
