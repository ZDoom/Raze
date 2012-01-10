//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#ifndef __anim_h__
#define __anim_h__

#define NUM_HARDCODED_ANIMS 11

# ifdef USE_LIBVPX
extern uint16_t anim_hi_numsounds[NUM_HARDCODED_ANIMS], *anim_hi_sounds[NUM_HARDCODED_ANIMS];
# endif

void G_PlayAnim(const char *fn,char t);
/*
void endanimvol43(int32_t fr);
void endanimvol42(int32_t fr);
void endanimvol41(int32_t fr);
void intro42animsounds(int32_t fr);
void first4animsounds(int32_t fr);
void intro4animsounds(int32_t fr);
void logoanimsounds(int32_t fr);
void endanimsounds(int32_t fr);
*/
#endif
