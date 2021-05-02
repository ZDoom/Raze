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

#pragma once 

enum
{
kTorch1 = 338,
kTorch2 = 350,
kTileRamsesGold = 590,
kTileRamsesWorkTile = 591,
kTileRamsesNormal = 592,
kTileStatusBar = 657,
kTile985 = 985,
kTile986 = 986,
kTile3000 = 3000,
kQueenChunk = 3117,
kTile3126 = 3126,
kTile3512 = 3512,
kTile3571 = 3571,
kTile3593 = 3593,
kTile3603 = 3603,
kTile4092 = 4092,
kTile4093 = 4093,
};

BEGIN_PS_NS

#define x(a, b) k##a = b,
enum
{
#include "namelist.h"
};
#undef x

END_PS_NS

