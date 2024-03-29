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
#pragma once

BEGIN_BLD_NS

enum GIBTYPE {
	GIBTYPE_0 = 0,
	GIBTYPE_1,
	GIBTYPE_2,
	GIBTYPE_3,
	GIBTYPE_4,
	GIBTYPE_5,
	GIBTYPE_6,
	GIBTYPE_7,
	GIBTYPE_8,
	GIBTYPE_9,
	GIBTYPE_10,
	GIBTYPE_11,
	GIBTYPE_12,
	GIBTYPE_13,
	GIBTYPE_14,
	GIBTYPE_15,
	GIBTYPE_16,
	GIBTYPE_17,
	GIBTYPE_18,
	GIBTYPE_19,
	GIBTYPE_20,
	GIBTYPE_21,
	GIBTYPE_22,
	GIBTYPE_23,
	GIBTYPE_24,
	GIBTYPE_25,
	GIBTYPE_26,
	GIBTYPE_27,
	GIBTYPE_28,
	GIBTYPE_29,
	GIBTYPE_30,
	kGibMax
};


void GibSprite(DBloodActor* pSprite, GIBTYPE nGibType, DVector3* pPos, DVector3* pVel);
void GibWall(walltype* pWall, GIBTYPE nGibType, DVector3* pVel);
END_BLD_NS
