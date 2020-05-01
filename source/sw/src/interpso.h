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

#ifndef INTERPSO_H
#define INTERPSO_H

#include "mfile.h"

extern int32_t so_numinterpolations;

void so_addinterpolation(SECTOR_OBJECTp sop);
void so_setspriteinterpolation(SECTOR_OBJECTp sop, spritetype *sp);
void so_stopspriteinterpolation(SECTOR_OBJECTp sop, spritetype *sp);
void so_setinterpolationangdiff(SECTOR_OBJECTp sop, int16_t angdiff);
void so_setinterpolationtics(SECTOR_OBJECTp sop, int16_t locktics);
void so_updateinterpolations(void);
void so_dointerpolations(int32_t smoothratio);
void so_restoreinterpolations(void);
SWBOOL so_writeinterpolations(MFILE_WRITE fil);
SWBOOL so_readinterpolations(MFILE_READ fil);

#endif
