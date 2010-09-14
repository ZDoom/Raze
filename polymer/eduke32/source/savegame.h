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

#ifndef __savegame_h__
#define __savegame_h__

#pragma pack(push,1)
struct savehead 
{
    char name[21];
    int32_t numplr,volnum,levnum,plrskl;
    char boardfn[BMAX_PATH];
};
#pragma pack(pop)

int32_t sv_updatestate(int32_t frominit);
int32_t sv_readdiff(int32_t fil);
uint32_t sv_writediff(FILE *fil);
int32_t sv_loadsnapshot(int32_t fil,int32_t *ret_hasdiffs,int32_t *ret_demoticcnt,int32_t *ret_synccompress);
int32_t sv_saveandmakesnapshot(FILE *fil,int32_t recdiffs,int32_t diffcompress,int32_t synccompress);
void sv_freemem();
int32_t G_SavePlayer(int32_t spot);
int32_t G_LoadPlayer(int32_t spot);
int32_t G_LoadSaveHeader(char spot,struct savehead *saveh);
void ReadSaveGameHeaders(void);
extern char *bitptr;
#endif
