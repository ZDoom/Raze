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

#ifndef __lavadude_h__
#define __lavadude_h__

#include "aistuff.h"

BEGIN_PS_NS

void InitLava();
int BuildLava(short nSprite, int x, int y, int z, short nSector, short nAngle, int nChannel);
int BuildLavaLimb(int nSprite, int edx, int ebx);
void FuncLavaLimb(int, int, int);
void FuncLava(int, int, int);

END_PS_NS

#endif
