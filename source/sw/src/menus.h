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

// MENUS.H
// Contains type definitions for all pop up menus

#ifndef MENUS_PUBLIC_
#define MENUS_PUBLIC_

BEGIN_SW_NS

void MNU_DrawString(int x, int y, const char* string, int shade, int pal, int align = -1);
void MNU_DrawSmallString(int x, int y, const char* string, int shade, int pal, int align = -1, double alpha = 1);
void MNU_DrawStringLarge(int x, int y, const char* string, int shade = 0, int align = -1);

#define pic_none 0
#define pic_radiobuttn1 2816
#define pic_radiobuttn2 2817
#define pic_newgame 2819
#define pic_load 2820
#define pic_save 2821
#define pic_options 2822
#define pic_orderinfo 2823
#define pic_todemo 2824
#define pic_togame 2825
#define pic_quit 2826
#define pic_newgametitl 2827
#define pic_training 2828
#define pic_easy 2829
#define pic_normal 2830
#define pic_hard 2831
#define pic_impossible 2832
#define pic_optionstitl 2833
#define pic_endgame 2834
#define pic_detail 2835
#define pic_high 2836
#define pic_low 2837
#define pic_mousesense 2838
#define pic_soundvol 2839
#define pic_toggles 2845
#define pic_togglestitl 2844
#define pic_mousenable 2840
#define pic_joyenable 2841
#define pic_bobbing 2842
#define pic_slidelend 2846
#define pic_slidebar 2847
#define pic_sliderend 2848
#define pic_sliderknob 2849
#define pic_shuriken1 2850
#define pic_yinyang 2870
#define pic_soundtitl 2870
#define pic_sndfxvol 2871
#define pic_musicvol 2872
#define pic_episode1 2873
#define pic_episode2 2874
#define pic_episode3 2875
#define pic_modem 2876
#define pic_scrsize 2877
#define pic_loadsavecursor 2918
#define pic_loadgame 2915
#define pic_savegame 2916
#define pic_loading 2917
#define pic_loadsavescreen 2919
#define pic_loadsavescreenbak 2922
#define pic_savedescr 2924
#define pic_shadow_warrior 2366

#define m_defshade      2
extern SWBOOL SavegameLoaded;


END_SW_NS

#endif
