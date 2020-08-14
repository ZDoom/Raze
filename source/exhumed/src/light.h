//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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

#ifndef __light_h__
#define __light_h__

#include "compat.h"

BEGIN_PS_NS

int LoadPaletteLookups();
void WaitVBL();
void SetGreenPal();
void RestoreGreenPal();
void FixPalette();
void FadeToWhite();
int HavePLURemap();
uint8_t RemapPLU(uint8_t pal);

//extern unsigned char kenpal[];
extern short overscanindex;

extern char *origpalookup[];

extern short nPalDiff;

END_PS_NS

#endif
