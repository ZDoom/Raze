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

#include "mapstructs.h"

BEGIN_BLD_NS

enum
{
	kAttrMove = 0x0001, // is affected by movement physics
	kAttrGravity = 0x0002, // is affected by gravity
	kAttrFalling = 0x0004, // in z motion
	kAttrAiming = 0x0008,
	kAttrRespawn = 0x0010,
	kAttrFree = 0x0020,
	kAttrSmoke = 0x0100, // receives tsprite smoke/steam 
};


#pragma pack(push, 1)

struct AISTATE;


struct MAPSIGNATURE {
	char signature[4];
	int16_t version;
};

struct MAPHEADER {
	int32_t x; // x
	int32_t y; // y
	int32_t z; // z
	int16_t ang; // ang
	int16_t sect; // sect
	int16_t pskybits; // pskybits
	int32_t visibility; // visibility
	int32_t mattid; // song id, Matt
	uint8_t parallax; // parallaxtype
	int32_t revision; // map revision
	int16_t numsectors; // numsectors
	int16_t numwalls; // numwalls
	int16_t numsprites; // numsprites
};

struct MAPHEADER2 {
	char name[64];
	int numxsprites; // xsprite size
	int numxwalls; // xwall size
	int numxsectors; // xsector size
	uint8_t pad[52];
};

#pragma pack(pop)

extern int gVisibility;
extern const char* gItemText[];
extern const char* gAmmoText[];
extern const char* gWeaponText[];
extern int gSkyCount;

void GetSpriteExtents(spritetypebase const* const pSprite, int* top, int* bottom)
{
	*top = *bottom = pSprite->pos.Z;
	if ((pSprite->cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != CSTAT_SPRITE_ALIGNMENT_FLOOR)
	{
		int height = tileHeight(pSprite->picnum);
		int center = height / 2 + tileTopOffset(pSprite->picnum);
		*top -= (pSprite->yrepeat << 2) * center;
		*bottom += (pSprite->yrepeat << 2) * (height - center);
	}
}

struct BloodSpawnSpriteDef : public SpawnSpriteDef
{
	TArray<XSPRITE> xspr;
};

DBloodActor* InsertSprite(sectortype* pSector, int nStat);
int DeleteSprite(DBloodActor* actor);

unsigned int dbReadMapCRC(const char* pPath);
void dbLoadMap(const char* pPath, int* pX, int* pY, int* pZ, short* pAngle, int* pSector, unsigned int* pCRC, BloodSpawnSpriteDef& sprites);


END_BLD_NS
