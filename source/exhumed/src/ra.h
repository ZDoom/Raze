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

#ifndef __ra_h__
#define __ra_h__

#include "aistuff.h"

BEGIN_PS_NS

struct RA
{
    short nAction;
    short nFrame;
    short nRun;
    short nSprite;
    short nTarget;
    short field_A;
    short field_C;
    short nPlayer;
};

extern RA Ra[];

void FreeRa(short nPlayer);
int BuildRa(short nPlayer);
void InitRa();
void MoveRaToEnemy(short nPlayer);
void FuncRa(int, int, int);

END_PS_NS

#endif
