//-------------------------------------------------------------------------
/*
Copyright (C) 2011 EDuke32 developers and contributors

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

#ifndef quotes_h_
#define quotes_h_

#define MAXQUOTES                   16384
#define MAXQUOTELEN                 128
#define OBITQUOTEINDEX              (MAXQUOTES-128)
#define SUICIDEQUOTEINDEX           (MAXQUOTES-32)

#define NOBETAQUOTE(x)              (DUKEBETA ? -1 : x)

#define QUOTE_SHOW_MAP_OFF          NOBETAQUOTE(1)
#define QUOTE_ACTIVATED             2
#define QUOTE_MEDKIT                3
#define QUOTE_LOCKED                4
#define QUOTE_CHEAT_EVERYTHING      5
#define QUOTE_BOOTS                 6
#define QUOTE_WASTED                7
#define QUOTE_UNLOCKED              8
#define QUOTE_FOUND_SECRET          9
#define QUOTE_SQUISHED              10
#define QUOTE_USED_STEROIDS         NOBETAQUOTE(12)
#define QUOTE_DEAD                  13
#define QUOTE_DEACTIVATED           15
#define QUOTE_CHEAT_GODMODE_ON      17
#define QUOTE_CHEAT_GODMODE_OFF     18
#define QUOTE_CROSSHAIR_OFF         NOBETAQUOTE(21)
#define QUOTE_CHEATS_DISABLED       22
#define QUOTE_MESSAGES_ON           23
#define QUOTE_MESSAGES_OFF          24
#define QUOTE_MUSIC                 26
#define QUOTE_CHEAT_STEROIDS        37
#define QUOTE_F1HELP                40
#define QUOTE_MOUSE_AIMING_OFF      44
#define QUOTE_HOLODUKE_ON           47
#define QUOTE_HOLODUKE_OFF          48
#define QUOTE_HOLODUKE_NOT_FOUND    49
#define QUOTE_JETPACK_NOT_FOUND     50
#define QUOTE_JETPACK_ON            52
#define QUOTE_JETPACK_OFF           53
#define QUOTE_NEED_BLUE_KEY         70
#define QUOTE_NEED_RED_KEY          71
#define QUOTE_NEED_YELLOW_KEY       72
#define QUOTE_WEAPON_LOWERED        73
#define QUOTE_WEAPON_RAISED         74
#define QUOTE_BOOTS_ON              75
#define QUOTE_SCUBA_ON              76
#define QUOTE_CHEAT_ALLEN           NOBETAQUOTE(79)
#define QUOTE_MIGHTY_FOOT           80
#define QUOTE_WEAPON_MODE_OFF       NOBETAQUOTE(82)
#define QUOTE_MAP_FOLLOW_OFF        83
#define QUOTE_RUN_MODE_OFF          85
#define QUOTE_JETPACK               88
#define QUOTE_SCUBA                 89
#define QUOTE_STEROIDS              90
#define QUOTE_HOLODUKE              91
#define QUOTE_CHEAT_TODD            NOBETAQUOTE(99)
#define QUOTE_CHEAT_UNLOCK          NOBETAQUOTE(100)
#define QUOTE_NVG                   101
#define QUOTE_WEREGONNAFRYYOURASS   102
#define QUOTE_SCREEN_SAVED          103
#define QUOTE_CHEAT_BETA            NOBETAQUOTE(105)
#define QUOTE_NVG_OFF               107
#define QUOTE_VIEW_MODE_OFF         NOBETAQUOTE(109)
#define QUOTE_SHOW_MAP_ON           NOBETAQUOTE(111)
#define QUOTE_CHEAT_NOCLIP          NOBETAQUOTE(113)
#define QUOTE_SAVE_BAD_VERSION      114
#define QUOTE_RESERVED              115
#define QUOTE_RESERVED2             116
#define QUOTE_RESERVED3             117
#define QUOTE_SAVE_DEAD             NOBETAQUOTE(118)
#define QUOTE_CHEAT_ALL_WEAPONS     NOBETAQUOTE(119)
#define QUOTE_CHEAT_ALL_INV         NOBETAQUOTE(120)
#define QUOTE_CHEAT_ALL_KEYS        NOBETAQUOTE(121)
#define QUOTE_RESERVED4             122
#define QUOTE_SAVE_BAD_PLAYERS      124

#endif
