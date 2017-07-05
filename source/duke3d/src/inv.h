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

#pragma once

enum dukeinv_t
{
    GET_STEROIDS,  // 0
    GET_SHIELD,
    GET_SCUBA,
    GET_HOLODUKE,
    GET_JETPACK,
    GET_DUMMY1,  // 5
    GET_ACCESS,
    GET_HEATS,
    GET_DUMMY2,
    GET_FIRSTAID,
    GET_BOOTS,  // 10
    GET_MAX
};

// these are not in the same order as the above, and it can't be changed for compat reasons. lame!
enum dukeinvicon_t
{
    ICON_NONE,  // 0
    ICON_FIRSTAID,
    ICON_STEROIDS,
    ICON_HOLODUKE,
    ICON_JETPACK,
    ICON_HEATS,  // 5
    ICON_SCUBA,
    ICON_BOOTS,
    ICON_MAX
};

extern int const icon_to_inv[ICON_MAX];

extern int const inv_to_icon[GET_MAX];

enum dukeweapon_t
{
    KNEE_WEAPON,  // 0
    PISTOL_WEAPON,
    SHOTGUN_WEAPON,
    CHAINGUN_WEAPON,
    RPG_WEAPON,
    HANDBOMB_WEAPON,  // 5
    SHRINKER_WEAPON,
    DEVISTATOR_WEAPON,
    TRIPBOMB_WEAPON,
    FREEZE_WEAPON,
    HANDREMOTE_WEAPON,  // 10
    GROW_WEAPON,
    MAX_WEAPONS
};
