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

#include "build.h"

#include "blood.h"
#include "bloodactor.h"

BEGIN_BLD_NS

struct GIBFX
{
	FX_ID fxId;
	int at1;
	int chance;
	int at9;
	int atd;
	int at11;
};


struct GIBTHING
{
	int type;
	int Kills;
	int chance;
	int atc;
	int at10;
};

struct GIBLIST
{
	GIBFX* gibFX;
	int Kills;
	GIBTHING* at8;
	int atc;
	int at10;
};

GIBFX gibFxGlassT[] = {
	{ FX_18, 0, 65536, 3, 200, 400 },
	{ FX_31, 0, 32768, 5, 200, 400 }
};

GIBFX gibFxGlassS[] = {
	{ FX_18, 0, 65536, 8, 200, 400 }
};

GIBFX gibFxBurnShard[] = {
	{ FX_16, 0, 65536, 12, 500, 1000 }
};

GIBFX gibFxWoodShard[] = {
	{ FX_17, 0, 65536, 12, 500, 1000 }
};

GIBFX gibFxMetalShard[] = {
	{ FX_30, 0, 65536, 12, 500, 1000 }
};

GIBFX gibFxFireSpark[] = {
	{ FX_14, 0, 65536, 8, 500, 1000 }
};

GIBFX gibFxShockSpark[] = {
	{ FX_15, 0, 65536, 8, 500, 1000 }
};

GIBFX gibFxBloodChunks[] = {
	{ FX_13, 0, 65536, 8, 90, 600 }
};

GIBFX gibFxBubblesS[] = {
	{ FX_25, 0, 65536, 8, 200, 400 }
};

GIBFX gibFxBubblesM[] = {
	{ FX_24, 0, 65536, 8, 200, 400 }
};

GIBFX gibFxBubblesL[] = {
	{ FX_23, 0, 65536, 8, 200, 400 }
};

GIBFX gibFxIcicles[] = {
	{ FX_31, 0, 65536, 15, 200, 400 }
};

GIBFX gibFxGlassCombo1[] = {
	{ FX_18, 0, 65536, 15, 200, 400 },
	{ FX_31, 0, 65536, 10, 200, 400 }
};

GIBFX gibFxGlassCombo2[] = {
	{ FX_18, 0, 65536, 5, 200, 400 },
	{ FX_20, 0, 53248, 5, 200, 400 },
	{ FX_21, 0, 53248, 5, 200, 400 },
	{ FX_19, 0, 53248, 5, 200, 400 },
	{ FX_22, 0, 53248, 5, 200, 400 }
};

GIBFX gibFxWoodCombo[] = {
	{ FX_16, 0, 65536, 8, 500, 1000 },
	{ FX_17, 0, 65536, 8, 500, 1000 },
	{ FX_14, 0, 65536, 8, 500, 1000 }
};

GIBFX gibFxMedicCombo[] = {
	{ FX_18, 0, 32768, 7, 200, 400 },
	{ FX_30, 0, 65536, 7, 500, 1000 },
	{ FX_13, 0, 65536, 10, 90, 600 },
	{ FX_14, 0, 32768, 7, 500, 1000 }
};

GIBFX gibFxFlareSpark[] = {
	{ FX_28, 0, 32768, 15, 128, -128 }
};

GIBFX gibFxBloodBits[] = {
	{ FX_13, 0, 45056, 8, 90, 600 }
};

GIBFX gibFxRockShards[] = {
	{ FX_46, 0, 65536, 10, 300, 800 },
	{ FX_31, 0, 32768, 10, 200, 1000 }
};

GIBFX gibFxPaperCombo1[] = {
	{ FX_47, 0, 65536, 12, 300, 600 },
	{ FX_14, 0, 65536, 8, 500, 1000 }
};

GIBFX gibFxPlantCombo1[] = {
	{ FX_44, 0, 45056, 8, 400, 800 },
	{ FX_45, 0, 45056, 8, 300, 800 },
	{ FX_14, 0, 45056, 6, 500, 1000 }
};

GIBFX gibFx13BBA8[] = {
	{ FX_49, 0, 65536, 4, 80, 300 }
};

GIBFX gibFx13BBC0[] = {
	{ FX_50, 0, 65536, 4, 80, 0 }
};

GIBFX gibFx13BBD8[] = {
	{ FX_50, 0, 65536, 20, 800, -40 },
	{ FX_15, 0, 65536, 15, 400, 10 }
};

GIBFX gibFx13BC04[] = {
	{ FX_32, 0, 65536, 8, 100, 0 }
};

GIBFX gibFx13BC1C[] = {
	{ FX_56, 0, 65536, 8, 100, 0 }
};

GIBTHING gibHuman[] = {
	{ 425, 1454, 917504, 300, 900 },
	{ 425, 1454, 917504, 300, 900 },
	{ 425, 1267, 917504, 300, 900 },
	{ 425, 1267, 917504, 300, 900 },
	{ 425, 1268, 917504, 300, 900 },
	{ 425, 1269, 917504, 300, 900 },
	{ 425, 1456, 917504, 300, 900 }
};

GIBTHING gibMime[] = {
	{ 425, 2405, 917504, 300, 900 },
	{ 425, 2405, 917504, 300, 900 },
	{ 425, 2404, 917504, 300, 900 },
	{ 425, 1268, 32768, 300, 900 },
	{ 425, 1269, 32768, 300, 900 },
	{ 425, 1456, 32768, 300, 900 },
};

GIBTHING gibHound[] = {
	{ 425, 1326, 917504, 300, 900 },
	{ 425, 1268, 32768, 300, 900 },
	{ 425, 1269, 32768, 300, 900 },
	{ 425, 1456, 32768, 300, 900 }
};

GIBTHING gibFleshGargoyle[] = {
	{ 425, 1369, 917504, 300, 900 },
	{ 425, 1361, 917504, 300, 900 },
	{ 425, 1268, 32768, 300, 900 },
	{ 425, 1269, 32768, 300, 900 },
	{ 425, 1456, 32768, 300, 900 }
};

GIBTHING gibAxeZombieHead[] = {
	{ 427, 3405, 917504, 0, 0 }
};

GIBLIST gibList[] = {
	{ gibFxGlassT, 2, NULL, 0, 300 },
	{ gibFxGlassS, 1, NULL, 0, 300 },
	{ gibFxBurnShard, 1, NULL, 0, 0 },
	{ gibFxWoodShard, 1, NULL, 0, 0 },
	{ gibFxMetalShard, 1, NULL, 0, 0 },
	{ gibFxFireSpark, 1, NULL, 0, 0 },
	{ gibFxShockSpark, 1, NULL, 0, 0 },
	{ gibFxBloodChunks, 1, NULL, 0, 0 },
	{ gibFxBubblesS, 1, NULL, 0, 0 },
	{ gibFxBubblesM, 1, NULL, 0, 0 },
	{ gibFxBubblesL, 1, NULL, 0, 0 },
	{ gibFxIcicles, 1, NULL, 0, 0 },
	{ gibFxGlassCombo1, 2, NULL, 0, 300 },
	{ gibFxGlassCombo2, 5, NULL, 0, 300 },
	{ gibFxWoodCombo, 3, NULL, 0, 0 },
	{ NULL, 0, gibHuman, 7, 0 },
	{ gibFxMedicCombo, 4, NULL, 0, 0 },
	{ gibFxFlareSpark, 1, NULL, 0, 0 },
	{ gibFxBloodBits, 1, NULL, 0, 0 },
	{ gibFxRockShards, 2, NULL, 0, 0 },
	{ gibFxPaperCombo1, 2, NULL, 0, 0 },
	{ gibFxPlantCombo1, 3, NULL, 0, 0 },
	{ gibFx13BBA8, 1, NULL, 0, 0 },
	{ gibFx13BBC0, 1, NULL, 0, 0 },
	{ gibFx13BBD8, 2, NULL, 0, 0 },
	{ gibFx13BC04, 1, NULL, 0, 0 },
	{ gibFx13BC1C, 1, NULL, 0, 0 },
	{ NULL, 0, gibAxeZombieHead, 1, 0 },
	{ NULL, 0, gibMime, 6, 0 },
	{ NULL, 0, gibHound, 4, 0 },
	{ NULL, 0, gibFleshGargoyle, 5, 0 },
};

int ChanceToCount(int a1, int a2)
{
	int vb = a2;
	if (a1 < 0x10000)
	{
		for (int i = 0; i < a2; i++)
			if (!Chance(a1))
				vb--;
	}
	return vb;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GibFX(DBloodActor* actor, GIBFX* pGFX, DVector3* pPos, CGibVelocity* pVel)
{
	auto pSector = actor->sector();
	if (adult_lockout && gGameOptions.nGameType == 0 && pGFX->fxId == FX_13)
		return;
	
	auto gPos = pPos? *pPos : actor->spr.pos;

	int32_t ceilZ, floorZ;
	getzsofslopeptr(pSector, gPos.XY(), &ceilZ, &floorZ);
	int nCount = ChanceToCount(pGFX->chance, pGFX->at9);
	int dz1 = floorZ - gPos.Z * worldtoint;
	int dz2 = gPos.Z * worldtoint - ceilZ;
	double top, bottom;
	GetActorExtents(actor, &top, &bottom);
	for (int i = 0; i < nCount; i++)
	{
		if (!pPos && (actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == 0)
		{
			int nAngle = Random(2048);
			gPos.X = actor->spr.pos.X + MulScale(actor->int_clipdist(), Cos(nAngle), 30) * inttoworld;
			gPos.Y = actor->spr.pos.Y + MulScale(actor->int_clipdist(), Sin(nAngle), 30) * inttoworld;
			gPos.Z = bottom - Random(bottom - top);
		}
		auto pFX = gFX.fxSpawnActor(pGFX->fxId, pSector, gPos, 0);
		if (pFX)
		{
			if (pGFX->at1 < 0)
				pFX->spr.pal = actor->spr.pal;
			if (pVel)
			{
				pFX->set_int_bvel_x(pVel->vx + Random2(pGFX->atd));
				pFX->set_int_bvel_y(pVel->vy + Random2(pGFX->atd));
				pFX->set_int_bvel_z(pVel->vz - Random(pGFX->at11));
			}
			else
			{
				pFX->vel.X = Random2F((pGFX->atd << 18) / 120);
				pFX->vel.Y = Random2F((pGFX->atd << 18) / 120);
				switch (actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK)
				{
				case 16:
					pFX->vel.Z = Random2F((pGFX->at11 << 18) / 120);
					break;
				default:
					if (dz2 < dz1 && dz2 < 0x4000)
					{
						pFX->set_int_bvel_z(0);
					}
					else if (dz2 > dz1 && dz1 < 0x4000)
					{
						pFX->vel.Z = -Random2F((abs(pGFX->at11) << 18) / 120);
					}
					else
					{
						if ((pGFX->at11 << 18) / 120 < 0)
							pFX->vel.Z = -Random2F((abs(pGFX->at11) << 18) / 120);
						else
							pFX->vel.Z = Random2F((pGFX->at11 << 18) / 120);
					}
					break;
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GibThing(DBloodActor* actor, GIBTHING* pGThing, DVector3* pPos, CGibVelocity* pVel)
{
	if (adult_lockout && gGameOptions.nGameType <= 0)
		switch (pGThing->type) {
		case kThingBloodBits:
		case kThingZombieHead:
			return;
		}

	if (pGThing->chance == 65536 || Chance(pGThing->chance))
	{
		auto pSector = actor->sector();
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		int x, y, z;
		if (!pPos)
		{
			int nAngle = Random(2048);
			x = actor->int_pos().X + MulScale(actor->int_clipdist(), Cos(nAngle), 30);
			y = actor->int_pos().Y + MulScale(actor->int_clipdist(), Sin(nAngle), 30);
			z = bottom - Random(bottom - top);
		}
		else
		{
			x = pPos->X * worldtoint;
			y = pPos->Y * worldtoint;
			z = pPos->Z * zworldtoint;
		}
		int32_t ceilZ, floorZ;
		getzsofslopeptr(pSector, x, y, &ceilZ, &floorZ);
		int dz1 = floorZ - z;
		int dz2 = z - ceilZ;
		auto gibactor = actSpawnThing(pSector, x, y, z, pGThing->type);
		if (!gibactor) return;

		if (pGThing->Kills > -1)
			gibactor->spr.picnum = pGThing->Kills;
		if (pVel)
		{
			gibactor->set_int_bvel_x(pVel->vx + Random2(pGThing->atc));
			gibactor->set_int_bvel_y(pVel->vy + Random2(pGThing->atc));
			gibactor->set_int_bvel_z(pVel->vz - Random(pGThing->at10));
		}
		else
		{
			gibactor->vel.X = Random2F((pGThing->atc << 18) / 120);
			gibactor->vel.Y = Random2F((pGThing->atc << 18) / 120);
			switch (actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK)
			{
			case 16:
				gibactor->vel.Z = Random2F((pGThing->at10 << 18) / 120);
				break;
			default:
				if (dz2 < dz1 && dz2 < 0x4000)
				{
					gibactor->set_int_bvel_z(0);
				}
				else if (dz2 > dz1 && dz1 < 0x4000)
				{
					gibactor->vel.Z = -Random2F((pGThing->at10 << 18) / 120);
				}
				else
				{
					gibactor->vel.Z = Random2F((pGThing->at10 << 18) / 120);
				}
				break;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GibSprite(DBloodActor* actor, GIBTYPE nGibType, DVector3* pPos, CGibVelocity* pVel)
{
	assert(actor != NULL);
	assert(nGibType >= 0 && nGibType < kGibMax);

	if (!actor->insector())
		return;
	GIBLIST* pGib = &gibList[nGibType];
	for (int i = 0; i < pGib->Kills; i++)
	{
		GIBFX* pGibFX = &pGib->gibFX[i];
		assert(pGibFX->chance > 0);
		GibFX(actor, pGibFX, pPos, pVel);
	}
	for (int i = 0; i < pGib->atc; i++)
	{
		GIBTHING* pGibThing = &pGib->at8[i];
		assert(pGibThing->chance > 0);
		GibThing(actor, pGibThing, pPos, pVel);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GibFX(walltype* pWall, GIBFX* pGFX, int a3, int a4, int a5, int a6, CGibVelocity* pVel)
{
	assert(pWall);
	int nCount = ChanceToCount(pGFX->chance, pGFX->at9);
	auto pSector = pWall->sectorp();
	for (int i = 0; i < nCount; i++)
	{
		int r1 = Random(a6);
		int r2 = Random(a5);
		int r3 = Random(a4);
		auto pGib = gFX.fxSpawnActor(pGFX->fxId, pSector, pWall->wall_int_pos().X + r3, pWall->wall_int_pos().Y + r2, a3 + r1, 0);
		if (pGib)
		{
			if (pGFX->at1 < 0)
				pGib->spr.pal = pWall->pal;
			if (!pVel)
			{
				pGib->vel.X = Random2F((pGFX->atd << 18) / 120);
				pGib->vel.Y = Random2F((pGFX->atd << 18) / 120);
				pGib->vel.Z = -Random2F((pGFX->at11 << 18) / 120);
			}
			else
			{
				pGib->vel.X = Random2F((pVel->vx << 18) / 120);
				pGib->vel.Y = Random2F((pVel->vy << 18) / 120);
				pGib->vel.Z = -Random2F((pVel->vz << 18) / 120);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GibWall(walltype* pWall, GIBTYPE nGibType, CGibVelocity* pVel)
{
	assert(pWall);
	assert(nGibType >= 0 && nGibType < kGibMax);
	int cx, cy, cz, wx, wy, wz;

	cx = (pWall->wall_int_pos().X + pWall->point2Wall()->wall_int_pos().X) >> 1;
	cy = (pWall->wall_int_pos().Y + pWall->point2Wall()->wall_int_pos().Y) >> 1;
	auto pSector = pWall->sectorp();
	int32_t ceilZ, floorZ;
	getzsofslopeptr(pSector, cx, cy, &ceilZ, &floorZ);
	int32_t ceilZ2, floorZ2;
	getzsofslopeptr(pWall->nextSector(), cx, cy, &ceilZ2, &floorZ2);

	ceilZ = ClipLow(ceilZ, ceilZ2);
	floorZ = ClipHigh(floorZ, floorZ2);
	wz = floorZ - ceilZ;
	wx = pWall->point2Wall()->wall_int_pos().X - pWall->wall_int_pos().X;
	wy = pWall->point2Wall()->wall_int_pos().Y - pWall->wall_int_pos().Y;
	cz = (ceilZ + floorZ) >> 1;

	GIBLIST* pGib = &gibList[nGibType];
	sfxPlay3DSound(cx, cy, cz, pGib->at10, pSector);
	for (int i = 0; i < pGib->Kills; i++)
	{
		GIBFX* pGibFX = &pGib->gibFX[i];
		assert(pGibFX->chance > 0);
		GibFX(pWall, pGibFX, ceilZ, wx, wy, wz, pVel);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void gibPrecache()
{
	for (int i = 0; i < kGibMax; i++)
	{
		auto const pThing = gibList[i].at8;
		if (pThing)
		{
			for (int j = 0; j < gibList[i].atc; j++)
			{
				if (pThing[j].Kills >= 0)
					tilePrecacheTile(pThing[j].Kills, -1, 0);
			}
		}
	}
}
END_BLD_NS
