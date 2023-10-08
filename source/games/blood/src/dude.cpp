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

#include "blood.h"

BEGIN_BLD_NS

DUDEINFO gPlayerTemplate[4] =
{
	// normal human
	{
		0x2f00,
		100,
		70,
		1200,
		0x30,
		0,
		0x10,
		0x800,
		0xc800,
		0x155,
		0,
		10,
		10,
		0x100,
		0x10,
		0x8000,
		0x1,
		0,
		0,
		0,
		0x40,
		15, -1, -1,
		0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x120,
		0, 0, 0, 0, 0, 0, 0,
		0,
		0
	},

	// normal beast
	{
		0x2900,
		100,
		70,
		1200,
		0x30,
		0,
		0x14,
		0x800,
		0xc800,
		0x155,
		0,
		10,
		10,
		0x100,
		0x10,
		0x8000,
		0x1,
		0,
		0,
		0,
		0x40,
		7, -1, -1,
		0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x120,
		0, 0, 0, 0, 0, 0, 0,
		0,
		0
	},

	// shrink human
	{
		12032,
		100,
		10, // mass
		1200,
		16, // clipdist
		0,
		0x10,
		0x800,
		0xc800,
		0x155,
		0,
		10,
		10,
		0x100,
		0x10,
		0x8000,
		0x1,
		0,
		0,
		0,
		0x40,
		15, -1, -1, // gib type
		1024, 1024, 1024, 1024, 256, 1024, 1024, //damage shift
		0, 0, 0, 0, 0, 0, 0,
		0,
		0
	},

	// grown human
	{
		12032,
		100,
		1100, // mass
		1200,
		100, // clipdist
		0,
		0x10,
		0x800,
		0xc800,
		0x155,
		0,
		10,
		10,
		0x100,
		0x10,
		0x8000,
		0x1,
		0,
		0,
		0,
		0x40,
		15, 7, 7, // gib type
		64, 64, 64, 64, 256, 64, 64, // damage shift
		0, 0, 0, 0, 0, 0, 0,
		0,
		0
	},
};

END_BLD_NS
