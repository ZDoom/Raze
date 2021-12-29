//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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
#include "ns.h"	// Must come before everything else!

#include <stdio.h>
#include <string.h>
#include "blood.h"

BEGIN_BLD_NS

unsigned int randSeed = 1;

unsigned int qrand(void)
{
	if (randSeed & 0x80000000)
		randSeed = ((randSeed << 1) ^ 0x20000004) | 0x1;
	else
		randSeed = randSeed << 1;
	return randSeed & 0x7fff;
}

int wRandSeed = 1;

int wrand(void)
{
	wRandSeed = (wRandSeed * 1103515245) + 12345;
	return FixedToInt(wRandSeed) & 0x7fff;
}

void wsrand(int seed)
{
	wRandSeed = seed;
}


END_BLD_NS
