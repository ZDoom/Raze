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

extern void G_DoCheats(void);
extern void G_SetupCheats(void);

extern char const * const g_NAMMattCheatQuote;

// Cheats
// KEEPINSYNC game.c: char CheatStrings[][]
enum cheatindex_t
{
    CHEAT_CORNHOLIO,  // 0
    CHEAT_STUFF,
    CHEAT_SCOTTY,
    CHEAT_COORDS,
    CHEAT_VIEW,
    CHEAT_TIME,  // 5
    CHEAT_UNLOCK,
    CHEAT_CASHMAN,
    CHEAT_ITEMS,
    CHEAT_RATE,
    CHEAT_SKILL,  // 10
    CHEAT_BETA,
    CHEAT_HYPER,
    CHEAT_MONSTERS,
    CHEAT_RESERVED,
    CHEAT_RESERVED2,  // 15
    CHEAT_TODD,
    CHEAT_SHOWMAP,
    CHEAT_KROZ,
    CHEAT_ALLEN,
    CHEAT_CLIP,  // 20
    CHEAT_WEAPONS,
    CHEAT_INVENTORY,
    CHEAT_KEYS,
    CHEAT_DEBUG,
    CHEAT_RESERVED3,  // 25
    CHEAT_COMEGETSOME,
    NUMCHEATS,
};

extern char CheatStrings[NUMCHEATS][MAXCHEATLEN];

// KEEPINSYNC game.c: uint8_t CheatFunctionIDs[]
// KEEPINSYNC menus.c: MenuEntry_t ME_CheatCodes[]
enum CheatCodeFunctions
{
    CHEATFUNC_CASHMAN,
    CHEATFUNC_GOD,
    CHEATFUNC_GIVEEVERYTHING,
    CHEATFUNC_GIVEWEAPONS,
    CHEATFUNC_GIVEALLITEMS,
    CHEATFUNC_GIVEINVENTORY,
    CHEATFUNC_GIVEKEYS,
    CHEATFUNC_HYPER,
    CHEATFUNC_VIEW,
    CHEATFUNC_SHOWMAP,
    CHEATFUNC_UNLOCK,
    CHEATFUNC_CLIP,
    CHEATFUNC_WARP,
    CHEATFUNC_SKILL,
    CHEATFUNC_MONSTERS,
    CHEATFUNC_FRAMERATE,
    CHEATFUNC_QUOTEBETA,
    CHEATFUNC_QUOTETODD,
    CHEATFUNC_QUOTEALLEN,
    CHEATFUNC_COORDS,
    CHEATFUNC_DEBUG,
    NUMCHEATFUNCS,
};
