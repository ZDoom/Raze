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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#ifndef premap_h_
#define premap_h_

struct MapRecord;

BEGIN_DUKE_NS

extern int16_t ambientlotag[64];
extern int16_t ambienthitag[64];
int G_EnterLevel(MapRecord *mi, int gameMode);
int G_FindLevelByFile(const char *fileName);
void G_NewGame(int volumeNum, int levelNum, int skillNum);
void G_ResetTimers(uint8_t keepgtics);
void P_ResetPlayer(int pn);
void G_ResetInterpolations(void);
void G_InitRRRASkies(void);


END_DUKE_NS

#endif
