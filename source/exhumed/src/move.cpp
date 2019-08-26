
#include "engine.h"
#include "exhumed.h"
#include "move.h"
#include "init.h"
#include "lighting.h"
#include "bubbles.h"
#include "object.h"
#include "player.h"
#include "view.h"
#include "status.h"
#include "runlist.h"
#include "items.h"
#include "sound.h"
#include "trigdat.h"
#include "anims.h"
#include "random.h"
#include "bullet.h"
#include <string.h>
#include <assert.h>
#ifndef __WATCOMC__
//#include <cmath>
#else
#include <stdlib.h>
//#include <math.h>
#endif

short NearSector[kMaxSectors] = { 0 };

short nPushBlocks;

// TODO - moveme?
short overridesect;
short NearCount = -1;

short nBodySprite[50];

long hihit, sprceiling, sprfloor, lohit;

#define kMaxPushBlocks	100
#define kMaxChunks	75

// think this belongs in init.c?
BlockInfo sBlockInfo[kMaxPushBlocks];

short nChunkSprite[kMaxChunks];


signed int lsqrt(int a1)
{
	int v1; // edx@1
	int v2; // ebx@1
	signed int result; // eax@1

	v1 = a1;
	v2 = a1 - 0x40000000;
	result = 0;
	if (v2 >= 0)
	{
		result = 0x8000;
		v1 = v2;
	}
	if (v1 - ((result << 15) + 0x10000000) >= 0)
	{
		v1 -= (result << 15) + 0x10000000;
		result += 0x4000;
	}
	if (v1 - ((result << 14) + 0x4000000) >= 0)
	{
		v1 -= (result << 14) + 0x4000000;
		result += 0x2000;
	}
	if (v1 - ((result << 13) + 0x1000000) >= 0)
	{
		v1 -= (result << 13) + 0x1000000;
		result += 4096;
	}
	if (v1 - ((result << 12) + 0x400000) >= 0)
	{
		v1 -= (result << 12) + 0x400000;
		result += 2048;
	}
	if (v1 - ((result << 11) + 0x100000) >= 0)
	{
		v1 -= (result << 11) + 0x100000;
		result += 1024;
	}
	if (v1 - ((result << 10) + 0x40000) >= 0)
	{
		v1 -= (result << 10) + 0x40000;
		result += 512;
	}
	if (v1 - ((result << 9) + 0x10000) >= 0)
	{
		v1 -= (result << 9) + 0x10000;
		result += 256;
	}
	if (v1 - ((result << 8) + 0x4000) >= 0)
	{
		v1 -= (result << 8) + 0x4000;
		result += 128;
	}
	if (v1 - ((result << 7) + 4096) >= 0)
	{
		v1 -= (result << 7) + 4096;
		result += 64;
	}
	if (v1 - ((result << 6) + 1024) >= 0)
	{
		v1 -= (result << 6) + 1024;
		result += 32;
	}
	if (v1 - (32 * result + 256) >= 0)
	{
		v1 -= 32 * result + 256;
		result += 16;
	}
	if (v1 - (16 * result + 64) >= 0)
	{
		v1 -= 16 * result + 64;
		result += 8;
	}
	if (v1 - (8 * result + 16) >= 0)
	{
		v1 -= 8 * result + 16;
		result += 4;
	}
	if (v1 - (4 * result + 4) >= 0)
	{
		v1 -= 4 * result + 4;
		result += 2;
	}
	if (v1 - (2 * result + 1) >= 0)
		++result;
	return result;
}

void MoveThings()
{
	UndoFlashes();
	DoLights();

	short the_freeze = nFreeze;

	if (nFreeze)
	{
		if (nFreeze == 1 || nFreeze == 2) {
			DoSpiritHead();
		}
	}
	else
	{
		runlist_ExecObjects();
		runlist_CleanRunRecs();
	}

	MoveStatus();
	DoBubbleMachines();
	DoDrips();
	DoMovingSects();
	DoRegenerates();

	if (levelnum == kMap20)
	{
		DoFinale();
		if (lCountDown < 1800 && nDronePitch < 2400 && !lFinaleStart)
		{
			nDronePitch += 64;
			BendAmbientSound();
		}
	}
}

void ResetMoveFifo()
{
	localclock = totalclock;
	movefifoend = 0;
	movefifopos = 0;
}

// not used
void clipwall()
{

}

void BuildNear(int x, int y, int walldist, int nSector)
{
	NearSector[0] = nSector;
	NearCount = 1;

	int i = 0;

	while (i < NearCount)
	{
		short nSector = NearSector[i];

		short nWall = sector[nSector].wallptr;
		short nWallCount = sector[nSector].wallnum;

		while (1)
		{
			nWallCount--;
			if (nWallCount < 0)
			{
				i++;
				break;
			}

			short nNextSector = wall[nWall].nextsector;

			if (nNextSector >= 0)
			{
				int j = 0;
				for (; j < NearCount; j++)
				{
					// loc_14F4D:
					if (nNextSector == NearSector[j])
						break;
				}

				if (j >= NearCount)
				{
					if (clipinsidebox(x, y, nWall, walldist))
					{
						NearSector[NearCount] = wall[nWall].nextsector;
						NearCount++;
					}
				}
			}

			nWall++;
		}
	}
}

int BelowNear(short nSprite)
{
	short nSector = sprite[nSprite].sectnum;
	int z = sprite[nSprite].z;

	int var_24, z2;

	if ((lohit & 0xC000) == 0xC000)
	{
		var_24 = lohit & 0xC000;
		z2 = sprite[lohit & 0x3FFF].z;
	}
	else
	{
		var_24 = 0x20000;
		z2 = sector[nSector].floorz + SectDepth[nSector];

		if (NearCount > 0)
		{
			short edx;

			for (int i = 0; i < NearCount; i++)
			{
				int nSect2 = NearSector[i];

				while (nSect2 >= 0)
				{
					edx = nSect2;
					nSect2 = SectBelow[nSect2];
				}

				int ecx = sector[edx].floorz + SectDepth[edx];
				int eax = ecx - z;

				if (eax < 0 && eax >= -5120)
				{
					z2 = ecx;
					nSector = edx;
				}
			}
		}
	}

	if (z2 < sprite[nSprite].z)
	{
		sprite[nSprite].z = z2;
		overridesect = nSector;
		sprite[nSprite].zvel = 0;

		bTouchFloor = kTrue;

		return var_24;
	}
	else
	{
		return 0;
	}
}

int movespritez(short nSprite, int z, int height, long flordist, int clipdist)
{
	short nSector = sprite[nSprite].sectnum;
	assert(nSector >= 0 && nSector < kMaxSectors);

	overridesect = nSector;
	int edi = nSector;

	// backup cstat
	ushort cstat = sprite[nSprite].cstat;

	sprite[nSprite].cstat &= 0xFFFE;

	int nRet = 0;

	if (SectFlag[nSector] & kSectUnderwater) {
		z >>= 1;
	}

	int spriteZ = sprite[nSprite].z;
	int floorZ = sector[nSector].floorz;

	int ebp = spriteZ + z;
	int eax = sector[nSector].ceilingz + (height >> 1);

	if ((SectFlag[nSector] & kSectUnderwater) && ebp < eax) {
		ebp = eax;
	}

	// loc_151E7:
	while (1)
	{
		if (ebp <= sector[sprite[nSprite].sectnum].floorz || SectBelow[sprite[nSprite].sectnum] < 0)
			break;

		edi = SectBelow[sprite[nSprite].sectnum];

		mychangespritesect(nSprite, edi);
	}

	if (edi == nSector)
	{
		while (1)
		{
			if ((ebp >= sector[sprite[nSprite].sectnum].ceilingz) || (SectAbove[sprite[nSprite].sectnum] < 0))
				break;

			edi = SectAbove[sprite[nSprite].sectnum];

			mychangespritesect(nSprite, edi);
		}
	}
	else
	{
		sprite[nSprite].z = ebp;

		if (SectFlag[edi] & kSectUnderwater)
		{
			if (nSprite == PlayerList[nLocalPlayer].nSprite) {
				D3PlayFX(StaticSound[kSound2], nSprite);
			}

			if (sprite[nSprite].statnum <= 107) {
				sprite[nSprite].hitag = 0;
			}
		}
	}

	// This function will keep the player from falling off cliffs when you're too close to the edge.
	// This function finds the highest and lowest z coordinates that your clipping BOX can get to.
	getzrange(sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z - 256, sprite[nSprite].sectnum, 
		&sprceiling, &hihit, &sprfloor, &lohit, 128, CLIPMASK0);

	long mySprfloor = sprfloor;

	if ((lohit & 0xC000) != 0xC000) {
		mySprfloor += SectDepth[sprite[nSprite].sectnum];
	}

	if (ebp > mySprfloor)
	{
		if (z > 0)
		{
			bTouchFloor = kTrue;

			if ((lohit & 0xC000) != 0xC000)
			{
				// Path B
				if (SectBelow[sprite[nSprite].sectnum] == -1)
				{
					nRet |= 0x20000;

					short nSectDamage = SectDamage[sprite[nSprite].sectnum];

					if (nSectDamage != 0)
					{
						if (sprite[nSprite].hitag < 15)
						{
							IgniteSprite(nSprite);
							sprite[nSprite].hitag = 20;
						}
#if 1
						short dx = nSectDamage;
						dx >>= 2;
						int eax = dx;
						int edx = eax;
						edx >>= 2;
						eax -= edx;

						int outval;

						__asm
						{
							mov		dx, nSectDamage
							sar     dx, 2
							movsx   eax, dx
							mov     edx, eax
							sar     edx, 2; // >> 4
							sub     eax, edx
							//mov     edx, eax
							movsx   edx, ax
							mov		outval, edx
						}
#endif

						short nDamageVal = (nSectDamage / 4) - (nSectDamage / 8);
						if (nDamageVal) {
							runlist_DamageEnemy(nSprite, -1, nDamageVal);
						}
					}

					sprite[nSprite].zvel = 0;
				}
			}
			else
			{
				// Path A
				short nFloorSprite = lohit & 0x3FFF;

				if (sprite[nSprite].statnum != 100 || !sprite[nFloorSprite].statnum || sprite[nFloorSprite].statnum >= 100)
				{
					short nStat = sprite[nFloorSprite].statnum;
					if (!nStat || nStat > 199)
					{
						nRet |= 0x20000;
					}
					else
					{
						nRet |= lohit;
					}

					sprite[nSprite].zvel = 0;
				}
				else
				{
					if (z >> 9)
					{
						runlist_DamageEnemy((lohit & 0x3FFF), nSprite, (z >> 9) * 2);
					}

					sprite[nSprite].zvel = -z;
				}
			}
		}

		// loc_1543B:
		ebp = mySprfloor;
		sprite[nSprite].z = mySprfloor;
	}
	else
	{
		if ((ebp - height) < sprceiling && ((hihit & 0xC000) == 0xC000 || SectAbove[sprite[nSprite].sectnum] == -1))
		{
			ebp = sprceiling + height;
			nRet |= 0x10000;
		}
	}

	if (spriteZ <= floorZ && ebp > floorZ)
	{
		if ((SectDepth[nSector] != 0) || (edi != nSector && (SectFlag[edi] & kSectUnderwater)))
		{
			assert(nSector >= 0 && nSector < kMaxSectors);
			BuildSplash(nSprite, nSector);
		}
	}

	sprite[nSprite].cstat = cstat; // restore cstat
	sprite[nSprite].z = ebp;

	if (sprite[nSprite].statnum == 100)
	{
		BuildNear(sprite[nSprite].x, sprite[nSprite].y, clipdist + (clipdist / 2), sprite[nSprite].sectnum);
		nRet |= BelowNear(nSprite);
	}

	return nRet;
}

int GetSpriteHeight(int nSprite)
{
	return tilesizy[sprite[nSprite].picnum] * sprite[nSprite].yrepeat * 4;
}

// TODO - where is ceildist used?
int movesprite(short nSprite, long dx, long dy, long dz, long ceildist, long flordist, unsigned long clipmask)
{
	bTouchFloor = kFalse;

	int x = sprite[nSprite].x;
	int y = sprite[nSprite].y;
	int z = sprite[nSprite].z;
	
	int nSpriteHeight = GetSpriteHeight(nSprite);

	int nClipDist = sprite[nSprite].clipdist << 2;

	short nSector = sprite[nSprite].sectnum;
	assert(nSector >= 0 && nSector < kMaxSectors);

	int floorZ = sector[nSector].floorz;

	int nRet = 0;

	if ((SectFlag[nSector] & kSectUnderwater) || (floorZ < z))
	{
		dx >>= 1;
		dy >>= 1;
	}

	short nSprite2 = nSprite;

	nRet |= movespritez(nSprite, dz, nSpriteHeight, flordist, nClipDist);

	nSector = sprite[nSprite].sectnum; // modified in movespritez so re-grab this variable

	if (sprite[nSprite].statnum == 100)
	{
		short nPlayer = GetPlayerFromSprite(nSprite2);

		long varA = 0;
		long varB = 0;

		CheckSectorFloor(overridesect, sprite[nSprite].z, &varB, &varA);

		if (varB || varA)
		{
			nXDamage[nPlayer] = varB;
			nYDamage[nPlayer] = varA;
		}

		dx += nXDamage[nPlayer];
		dy += nYDamage[nPlayer];
	}
	else
	{
		CheckSectorFloor(overridesect, sprite[nSprite].z, &dx, &dy);
	}

	/* 
		The game masks off the top 16 bits of the return value.
	*/
	nRet |= clipmove(&sprite[nSprite].x, &sprite[nSprite].y, &sprite[nSprite].z, &nSector, dx, dy, nClipDist, nSpriteHeight, flordist, clipmask) & 0xFFFF;

	if ((nSector != sprite[nSprite].sectnum) && nSector >= 0)
	{
		if (nRet & 0x20000) {
			dz = 0;
		}
		
		if ((sector[nSector].floorz - z) < (dz + flordist))
		{
			sprite[nSprite].x = x;
			sprite[nSprite].y = y;
		}
		else
		{
			mychangespritesect(nSprite, nSector);

			if (sprite[nSprite].pal < 5 && !sprite[nSprite].hitag)
			{
				sprite[nSprite].pal = sector[sprite[nSprite].sectnum].ceilingpal;
			}
		}
	}

	return nRet;
}

// OK
void Gravity(short nSprite)
{
	short nSector = sprite[nSprite].sectnum;

	if (SectFlag[nSector] & kSectUnderwater)
	{
		if (sprite[nSprite].statnum != 100)
		{
			if (sprite[nSprite].zvel <= 1024)
			{
				if (sprite[nSprite].zvel < 2048) {
					sprite[nSprite].zvel += 512;
				}
			}
			else
			{
				sprite[nSprite].zvel -= 64;
			}
		}
		else
		{
			if (sprite[nSprite].zvel > 0)
			{
				sprite[nSprite].zvel -= 64;
				if (sprite[nSprite].zvel < 0) {
					sprite[nSprite].zvel = 0;
				}
			}
			else if (sprite[nSprite].zvel < 0)
			{
				sprite[nSprite].zvel += 64;
				if (sprite[nSprite].zvel > 0) {
					sprite[nSprite].zvel = 0;
				}
			}
		}
	}
	else
	{
		sprite[nSprite].zvel += 512;
		if (sprite[nSprite].zvel > 16384) {
			sprite[nSprite].zvel = 16384;
		}
	}
}

int MoveCreature(short nSprite)
{
	return movesprite(nSprite, sprite[nSprite].xvel << 8, sprite[nSprite].yvel << 8, sprite[nSprite].zvel, 15360, -5120, CLIPMASK0);
}

int MoveCreatureWithCaution(int nSprite)
{
	int x = sprite[nSprite].x;
	int y = sprite[nSprite].y;
	int z = sprite[nSprite].z;
	short nSectorPre = sprite[nSprite].sectnum;

	int ecx = MoveCreature(nSprite);

	short nSector = sprite[nSprite].sectnum;

	if (nSector != nSectorPre)
	{
		int zDiff = sector[nSectorPre].floorz - sector[nSector].floorz;
		if (zDiff < 0) {
			zDiff = -zDiff;
		}

		if (zDiff > 15360 || (SectFlag[nSector] & kSectUnderwater) || (SectBelow[nSector] > -1 && SectFlag[SectBelow[nSector]]) || SectDamage[nSector])
		{
			sprite[nSprite].x = x;
			sprite[nSprite].y = y;
			sprite[nSprite].z = z;

			mychangespritesect(nSprite, nSectorPre);

			sprite[nSprite].ang = (sprite[nSprite].ang + 256) & kAngleMask;
			sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512) >> 2;
			sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 2;
			return 0;
		}
	}

	return ecx;
}

int GetAngleToSprite(int nSprite1, int nSprite2)
{
	if (nSprite1 < 0 || nSprite2 < 0)
		return -1;

	return GetMyAngle(sprite[nSprite2].x - sprite[nSprite1].x, sprite[nSprite2].y - sprite[nSprite1].y);
}

int PlotCourseToSprite(int nSprite1, int nSprite2)
{
	if (nSprite1 < 0 || nSprite2 < 0)
		return -1;

	int x = sprite[nSprite2].x - sprite[nSprite1].x;
	int y = sprite[nSprite2].y - sprite[nSprite1].y;

	sprite[nSprite1].ang = GetMyAngle(x, y);

	return ksqrt(y * y + x * x);
}

int FindPlayer(int nSprite, int nVal)
{
	int var_18 = 0;
	if (nSprite >= 0)
		var_18 = 1;

	if (nSprite < 0)
		nSprite = -nSprite;

	if (nVal < 0)
		nVal = 100;

	int x = sprite[nSprite].x;
	int y = sprite[nSprite].y;
	short nSector = sprite[nSprite].sectnum;

	int z = sprite[nSprite].z - GetSpriteHeight(nSprite);

	nVal <<= 8;
	
	short nPlayerSprite;
	int i = 0;

	while (1)
	{
		if (i >= nTotalPlayers)
			return -1;

		nPlayerSprite = PlayerList[i].nSprite;

		if ((sprite[nPlayerSprite].cstat & 0x101) && (!(sprite[nPlayerSprite].cstat & 0x8000)))
		{
			int v9 = sprite[nPlayerSprite].x - x;
			if (v9 < 0) {
				v9 = -v9;
			}

			int v10 = sprite[nPlayerSprite].y - y;

			if (v9 < nVal)
			{
				if (v10 < 0) {
					v10 = -v10;
				}

				if (v10 < nVal && cansee(sprite[nPlayerSprite].x, sprite[nPlayerSprite].y, sprite[nPlayerSprite].z - 7680, sprite[nPlayerSprite].sectnum, x, y, z, nSector))
				{
					break;
				}
			}
		}

		i++;
	}

	if (var_18) {
		PlotCourseToSprite(nSprite, nPlayerSprite);
	}

	return nPlayerSprite;
}

void CheckSectorFloor(short nSector, int z, long *a, long *b)
{
	short nSpeed = SectSpeed[nSector];

	if (!nSpeed) {
		return;
	}

	short nFlag = SectFlag[nSector];
	short nAng = nFlag & kAngleMask;

	if (z >= sector[nSector].floorz)
	{
		*a += (Sin(nAng + 512) << 3) * nSpeed;
		*b += (sintable[nAng] << 3) * nSpeed;
	}
	else if (nFlag & 0x800)
	{
		*a += (Sin(nAng + 512) << 4) * nSpeed;
		*b += (sintable[nAng] << 4) * nSpeed;
	}
}

int GetUpAngle(short nSprite1, int nVal, short nSprite2, int ecx)
{
	int x = sprite[nSprite2].x - sprite[nSprite1].x;
	int y = sprite[nSprite2].y - sprite[nSprite1].y;

	int ebx = (sprite[nSprite2].z + ecx) - (sprite[nSprite1].z + nVal);
	int edx = (sprite[nSprite2].z + ecx) - (sprite[nSprite1].z + nVal);
	
	ebx >>= 4;
	edx >>= 8;

	ebx = -ebx;

	ebx -= edx;

	int nSqrt = lsqrt(x * x + y * y);

	return GetMyAngle(nSqrt, ebx);
}

void InitPushBlocks()
{
	nPushBlocks = 0;
}

int GrabPushBlock()
{
	if (nPushBlocks >= kMaxPushBlocks) {
		return -1;
	}

	return nPushBlocks++;
}

void CreatePushBlock(int nSector)
{
	int nBlock = GrabPushBlock();
	int i;

	int startwall = sector[nSector].wallptr;
	int nWalls = sector[nSector].wallnum;

	int ecx = 0;
	int ebx = 0;

	for (i = 0; i < nWalls; i++)
	{
		ecx += wall[startwall + i].x;
		ebx += wall[startwall + i].y;
	}

	int avgx = ecx / nWalls;
	int avgy = ebx / nWalls;

	sBlockInfo[nBlock].x = avgx;
	sBlockInfo[nBlock].y = avgy;

	int nSprite = insertsprite(nSector, 0);

	sBlockInfo[nBlock].nSprite = nSprite;

	sprite[nSprite].x = avgx;
	sprite[nSprite].y = avgy;
 	sprite[nSprite].z = sector[nSector].floorz - 256;
	sprite[nSprite].cstat = 0x8000;

	int var_28 = 0;

	for (i = 0; i < nWalls; i++)
	{
		int x = avgx - wall[startwall + i].x;
		int y = avgy - wall[startwall + i].y;

		int nSqrt = ksqrt(x * x + y * y);
		if (nSqrt > var_28) {
			var_28 = nSqrt;
		}
	}

	sBlockInfo[nBlock].field_8 = var_28;

	sprite[nSprite].clipdist = (var_28 & 0xFF) << 2;
	sector[nSector].extra = nBlock;
}

void MoveSector(short nSector, int nAngle, int *nXVel, int *nYVel)
{
	int i;

	if (nSector == -1) {
		return;
	}

	long nXVect, nYVect;

	if (nAngle < 0)
	{
		nXVect = *nXVel;
		nYVect = *nYVel;
		nAngle = GetMyAngle(nXVect, nYVect);
	}
	else
	{
		nXVect = Sin(nAngle + 512) << 6;
		nYVect = Sin(nAngle) << 6;
	}

	short nBlock = sector[nSector].extra;
	short nSectFlag = SectFlag[nSector];

	SECTOR *pSector = &sector[nSector];
	int nFloorZ = sector[nSector].floorz;
	int startwall = sector[nSector].wallptr;
	int nWalls = sector[nSector].wallnum;

	WALL *pStartWall = &wall[startwall];
	short nNextSector = wall[startwall].nextsector;

	BlockInfo *pBlockInfo = &sBlockInfo[nBlock];

	long x = sBlockInfo[nBlock].x;
	long x_b = sBlockInfo[nBlock].x;

	long y = sBlockInfo[nBlock].y;
	long y_b = sBlockInfo[nBlock].y;

	short nSectorB = nSector;

	int nZVal;
	long z;

	int bUnderwater = nSectFlag & kSectUnderwater;

	if (nSectFlag & kSectUnderwater)
	{
		nZVal = sector[nSector].ceilingz;
		z = sector[nNextSector].ceilingz + 256;

		sector[nSector].ceilingz = sector[nNextSector].ceilingz;
	}
	else
	{
		nZVal = sector[nSector].floorz;
		z = sector[nNextSector].floorz - 256;

		sector[nSector].floorz = sector[nNextSector].floorz;
	}

	clipmove(&x, &y, &z, &nSectorB, nXVect, nYVect, pBlockInfo->field_8, 0, 0, CLIPMASK1);

	int yvect = y - y_b;
	int xvect = x - x_b;

	if (nSectorB != nNextSector && nSectorB != nSector)
	{
		yvect = 0;
		xvect = 0;
	}
	else
	{
		if (!bUnderwater)
		{
			z = nZVal;
			x = x_b;
			y = y_b;

			clipmove(&x, &y, &z, &nSectorB, nXVect, nYVect, pBlockInfo->field_8, 0, 0, CLIPMASK1);

			int ebx = x;
			int ecx = x_b;
			int edx = y;
			int eax = xvect;
			int esi = y_b;

			if (eax < 0) {
				eax = -eax;
			}

			ebx -= ecx;
			ecx = eax;
			eax = ebx;
			edx -= esi;

			if (eax < 0) {
				eax = -eax;
			}

			if (ecx > eax)
			{
				xvect = ebx;
			}

			eax = yvect;
			if (eax < 0) {
				eax = -eax;
			}

			ebx = eax;
			eax = edx;

			if (eax < 0) {
				eax = -eax;
			}

			if (ebx > eax) {
				yvect = edx;
			}
		}
	}

	// GREEN
	if (yvect || xvect)
	{
		for (i = headspritesect[nSector]; i != -1; i = nextspritesect[i])
		{
			if (sprite[i].statnum < 99)
			{
				sprite[i].x += xvect;
				sprite[i].y += yvect;
			}
			else
			{
				z = sprite[i].z;

				if ((nSectFlag & kSectUnderwater) || z != nZVal || sprite[i].cstat & 0x8000)
				{
					x = sprite[i].x;
					y = sprite[i].y;
					nSectorB = nSector;

					clipmove(&x, &y, &z, &nSectorB, -xvect, -yvect, 4 * sprite[i].clipdist, 0, 0, CLIPMASK0);

					if (nSectorB >= 0 && nSectorB < kMaxSectors && nSectorB != nSector) {
						mychangespritesect(i, nSectorB);
					}
				}
			}
		}

		//int var_1C = nAngle & kAngleMask;
		int var_38 = yvect << 14;
		int var_58 = xvect << 14;

		for (i = headspritesect[nNextSector]; i != -1; i = nextspritesect[i])
		{
			if (sprite[i].statnum >= 99)
			{
				x = sprite[i].x;
				y = sprite[i].y;
				z = sprite[i].z;
				nSectorB = nNextSector;

				clipmove(&x, &y, &z, &nSectorB,
					-xvect - (Sin(nAngle + 512) * (4 * sprite[i].clipdist)),
					-yvect - (Sin(nAngle) * (4 * sprite[i].clipdist)),
					4 * sprite[i].clipdist, 0, 0, CLIPMASK0);


				if (nSectorB != nNextSector && (nSectorB == nSector || nNextSector == nSector))
				{
					if (nSectorB != nSector || nFloorZ >= sprite[i].z)
					{
						if (nSectorB >= 0 && nSectorB < kMaxSectors) {
							mychangespritesect(i, nSectorB);
						}
					}
					else
					{
						movesprite(i,
							(xvect << 14) + Sin(nAngle + 512) * sprite[i].clipdist,
							(yvect << 14) + Sin(nAngle) * sprite[i].clipdist,
							0, 0, 0, CLIPMASK0);
					}
				}
			}
		}

		for (int i = 0; i < nWalls; i++)
		{
			dragpoint(startwall, xvect + pStartWall->x, yvect + pStartWall->y);
			pStartWall++;
			startwall++;
		}

		pBlockInfo->x += xvect;
		pBlockInfo->y += yvect;
	}
	else {
		int gasd = 123;
	}

	// loc_163DD
	xvect <<= 14;
	yvect <<= 14;

	if (!(nSectFlag & kSectUnderwater))
	{
		for (i = headspritesect[nSector]; i != -1; i = nextspritesect[i])
		{
			if (sprite[i].statnum >= 99 && nZVal == sprite[i].z && !(sprite[i].cstat & 0x8000))
			{
				nSectorB = nSector;
				clipmove(&sprite[i].x, &sprite[i].y, &sprite[i].z, &nSectorB, xvect, yvect, 4 * sprite[i].clipdist, 5120, -5120, CLIPMASK0);
			}
		}
	}

	if (nSectFlag & kSectUnderwater) {
		pSector->ceilingz = nZVal;
	}
	else {
		pSector->floorz = nZVal;
	}

	*nXVel = xvect;
	*nYVel = yvect;
}

void SetQuake(short nSprite, int nVal)
{
	int x = sprite[nSprite].x;
	int y = sprite[nSprite].y;

	for (int i = 0; i < nTotalPlayers; i++)
	{
		int nPlayerSprite = PlayerList[i].nSprite;

		int nSqrt = ksqrt(((sprite[nPlayerSprite].x - x) >> 8) * ((sprite[nPlayerSprite].x - x) >> 8) + ((sprite[nPlayerSprite].y - y) >> 8)
			* ((sprite[nPlayerSprite].y - y) >> 8));

		int eax = nVal * 256;

		if (nSqrt)
		{
			eax = eax / nSqrt;

			if (eax >= 256)
			{
				if (eax > 3840) {
					eax = 3840;
				}
			}
			else
			{
				eax = 0;
			}
		}

		if (eax > nQuake[i]) {
			nQuake[i] = eax;
		}
	}
}

int AngleChase(int nSprite, int nSprite2, int ebx, int ecx, int push1)
{
	int nClipType = sprite[nSprite].statnum != 107;

	/* bjd - need to handle cliptype to clipmask change that occured in later build engine version */
	if (nClipType == 1) {
		nClipType = CLIPMASK1;
	}
	else {
		nClipType = CLIPMASK0;
	}

	short nAngle;

	if (nSprite2 < 0)
	{
		sprite[nSprite].zvel = 0;
		nAngle = sprite[nSprite].ang;
	}
	else
	{
		int nHeight = tilesizy[sprite[nSprite2].picnum] * sprite[nSprite2].yrepeat * 2;

		int nMyAngle = GetMyAngle(sprite[nSprite2].x - sprite[nSprite].x, sprite[nSprite2].y - sprite[nSprite].y);

		int nSqrt = ksqrt(
			(sprite[nSprite2].y - sprite[nSprite].y)
			*
			(sprite[nSprite2].y - sprite[nSprite].y)
			+
			(sprite[nSprite2].x - sprite[nSprite].x)
			*
			(sprite[nSprite2].x - sprite[nSprite].x)
		);

		int var_18 = GetMyAngle(nSqrt, ((sprite[nSprite2].z - nHeight) - sprite[nSprite].z) >> 8);

		int edx = nMyAngle;

		int nAngDelta = AngleDelta(sprite[nSprite].ang, nMyAngle, 1024);
		int nAngDelta2 = nAngDelta;

		if (nAngDelta2 < 0)
			nAngDelta2 = -nAngDelta2;

		if (nAngDelta2 > 63)
		{
			nAngDelta2 = nAngDelta;
			nAngDelta2 >>= 6;
			
			edx = ebx;

			if (nAngDelta2 < 0)
				nAngDelta2 = -nAngDelta2;

			ebx /= nAngDelta2;
			
			if (ebx < 5)
				ebx = 5;
		}

		int nAngDeltaC = nAngDelta;

		if (nAngDeltaC < 0)
			nAngDeltaC = -nAngDeltaC;
		
		if (nAngDeltaC > push1)
		{
			if (nAngDelta >= 0)
				nAngDelta = push1;
			else
				nAngDelta = -push1;
		}

		int nAngDeltaD = AngleDelta(sprite[nSprite].zvel, var_18, 24);
		nAngle = (nAngDelta + sprite[nSprite].ang) & kAngleMask;

		// TODO - CHECKME int ebx = 24;

		sprite[nSprite].zvel = (sprite[nSprite].zvel + nAngDeltaD) & kAngleMask;
	}

	sprite[nSprite].ang = nAngle;

	int eax = Sin(sprite[nSprite].zvel + 512);
	int edx = (nAngle + 512) & kAngleMask;

	if (eax < 0)
		eax = -eax;

	// rename this var. CHECKME
	int x = ((sintable[edx] * ebx) >> 14) * eax;

	int ceildist = x >> 8;

	int y = ((Sin(nAngle) * ebx) >> 14) * eax;

	int nVal = y >> 8;

	int z = Sin(sprite[nSprite].zvel) * ksqrt((nVal * nVal) + (ceildist * ceildist));

	return movesprite(nSprite, x >> 2, y >> 2, (z >> 13) + (Sin(ecx) >> 5), 0, 0, nClipType);
}

int GetWallNormal(short nWall)
{
	nWall &= 0x1FFF;

	int nWall2 = wall[nWall].point2;

	int nAngle = GetMyAngle(wall[nWall2].x - wall[nWall].x, wall[nWall2].y - wall[nWall].y);
	return (nAngle + 512) & kAngleMask;
}

void WheresMyMouth(int nPlayer, int *x, int *y, int *z, short *sectnum)
{
	int nSprite = PlayerList[nPlayer].nSprite;

	*x = sprite[nSprite].x;
	*y = sprite[nSprite].y;

	int height = GetSpriteHeight(nSprite) / 2;

	*z = sprite[nSprite].z - height;
	*sectnum = sprite[nSprite].sectnum;

	clipmove((long*)x, (long*)y, (long*)z, sectnum, 
		Sin(sprite[nSprite].ang + 512) << 7,
		Sin(sprite[nSprite].ang) << 7,
		5120, 1280, 1280, CLIPMASK1);
}

void InitChunks()
{
	nCurChunkNum = 0;
	memset(nChunkSprite,   -1, sizeof(nChunkSprite));
	memset(nBodyGunSprite, -1, sizeof(nBodyGunSprite));
	memset(nBodySprite,    -1, sizeof(nBodySprite));
	nCurBodyNum    = 0;
	nCurBodyGunNum = 0;
	nBodyTotal  = 0;
	nChunkTotal = 0;
}

int GrabBodyGunSprite()
{
	int nSprite = nBodyGunSprite[nCurBodyGunNum];

	if (nSprite == -1)
	{
		nSprite = insertsprite(0, 899);
		nBodyGunSprite[nCurBodyGunNum] = nSprite;

		sprite[nSprite].lotag = -1;
		sprite[nSprite].owner = -1;
	}
	else
	{
		int nAnim = sprite[nSprite].owner;

		if (nAnim != -1) {
			DestroyAnim(nAnim);
		}

		sprite[nSprite].lotag = -1;
		sprite[nSprite].owner = -1;
	}

	nCurBodyGunNum++;
	if (nCurBodyGunNum >= 50) { // TODO - enum/define
		nCurBodyGunNum = 0;
	}

	sprite[nSprite].cstat = 0;

	return nSprite;
}

int GrabBody()
{
	int nSprite;

	do
	{
		nSprite = nBodySprite[nCurBodyNum];

		if (nSprite == -1)
		{
			nSprite = insertsprite(0, 899);
			nBodySprite[nCurBodyNum] = nSprite;
			sprite[nSprite].cstat = 0x8000;
		}

		nCurBodyNum++;
		if (nCurBodyNum >= 50) {
			nCurBodyNum = 0;
		}
	}
	while (sprite[nSprite].cstat & 0x101);

	if (nBodyTotal < 50) {
		nBodyTotal++;
	}

	sprite[nSprite].cstat = 0;
	return nSprite;
}

int GrabChunkSprite()
{
	int nSprite = nChunkSprite[nCurChunkNum];

	if (nSprite == -1)
	{
		nSprite = insertsprite(0, 899);
		nChunkSprite[nCurChunkNum] = nSprite;
	}
	else if (sprite[nSprite].statnum)
	{
// TODO	MonoOut("too many chunks being used at once!\n");
		return -1;
	}

	changespritestat(nSprite, 899);

	nCurChunkNum++;
	if (nCurChunkNum >= kMaxChunks)
		nCurChunkNum = 0;

	if (nChunkTotal < kMaxChunks)
		nChunkTotal++;

	sprite[nSprite].cstat = 0x80;

	return nSprite;
}

int BuildCreatureChunk(int nVal, int nPic)
{
	int var_14;

	int nSprite = GrabChunkSprite();

	if (nSprite == -1) {
		return -1;
	}

	if (nVal & 0x4000)
	{
		nVal &= 0x3FFF;
		var_14 = 1;
	}
	else
	{
		var_14 = 0;
	}

	nVal &= 0xFFFF;

	sprite[nSprite].x = sprite[nVal].x;
	sprite[nSprite].y = sprite[nVal].y;
	sprite[nSprite].z = sprite[nVal].z;

	mychangespritesect(nSprite, sprite[nVal].sectnum);

	sprite[nSprite].cstat = 0x80;
	sprite[nSprite].shade = -12;
	sprite[nSprite].pal = 0;

	sprite[nSprite].xvel = (RandomSize(5) - 16) << 7;
	sprite[nSprite].yvel = (RandomSize(5) - 16) << 7;
	sprite[nSprite].zvel = (-(RandomSize(8) + 512)) << 3;

	if (var_14)
	{
		sprite[nSprite].xvel *= 4;
		sprite[nSprite].yvel *= 4;
		sprite[nSprite].zvel *= 2;
	}

	sprite[nSprite].xrepeat = 64;
	sprite[nSprite].yrepeat = 64;
	sprite[nSprite].xoffset = 0;
	sprite[nSprite].yoffset = 0;
	sprite[nSprite].picnum = nPic;
	sprite[nSprite].lotag = runlist_HeadRun() + 1;
	sprite[nSprite].clipdist = 40;

//	GrabTimeSlot(3);

	sprite[nSprite].extra = -1;
	sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nSprite | 0xD0000);
	sprite[nSprite].hitag = runlist_AddRunRec(NewRun, nSprite | 0xD0000);

	return nSprite | 0xD0000;
}

void FuncCreatureChunk(int a, int, int nRun)
{
	int nSprite = RunData[nRun].nVal;
	assert(nSprite >= 0 && nSprite < kMaxSprites);

	int nMessage = a & 0x7F0000;

	if (nMessage != 0x20000)
		return;

	Gravity(nSprite);

	int nSector = sprite[nSprite].sectnum;
	sprite[nSprite].pal = sector[nSector].ceilingpal;

	int nVal = movesprite(nSprite, sprite[nSprite].xvel << 10, sprite[nSprite].yvel << 10, sprite[nSprite].zvel, 2560, -2560, CLIPMASK1);

	if (sprite[nSprite].z >= sector[nSector].floorz)
	{
		// re-grab this variable as it may have changed in movesprite(). Note the check above is against the value *before* movesprite so don't change it.
		nSector = sprite[nSprite].sectnum;

		sprite[nSprite].zvel = 0;
		sprite[nSprite].yvel = 0;
		sprite[nSprite].xvel = 0;
		sprite[nSprite].z = sector[nSector].floorz;
	}
	else
	{
		if (!nVal)
			return;

		short nAngle;

		if (nVal & 0x20000)
		{
			sprite[nSprite].cstat = 0x8000;
		}
		else
		{
			if ((nVal & 0x3C000) == 0x10000)
			{
				sprite[nSprite].xvel >>= 1;
				sprite[nSprite].yvel >>= 1;
				sprite[nSprite].zvel = -sprite[nSprite].zvel;
				return;
			}
			else if ((nVal & 0x3C000) == 0xC000)
			{
				nAngle = sprite[nVal & 0x3FFF].ang;
			}
			else if ((nVal & 0x3C000) == 0x8000)
			{
				nAngle = GetWallNormal(nVal & 0x3FFF);
			}
			else
			{
				return;
			}

			// loc_16E0C
			int nSqrt = lsqrt(((sprite[nSprite].yvel >> 10) * (sprite[nSprite].yvel >> 10)
				+ (sprite[nSprite].xvel >> 10) * (sprite[nSprite].xvel >> 10)) >> 8);

			sprite[nSprite].xvel = Sin(nAngle + 512) * (nSqrt >> 1);
			sprite[nSprite].yvel = Sin(nAngle) * (nSqrt >> 1);
			return;
		}
	}

	runlist_DoSubRunRec(sprite[nSprite].owner);
	runlist_FreeRun(sprite[nSprite].lotag - 1);
	runlist_SubRunRec(sprite[nSprite].hitag);

	changespritestat(nSprite, 0);
	sprite[nSprite].hitag = 0;
	sprite[nSprite].lotag = 0;
}

short UpdateEnemy(short *nEnemy)
{
	if (*nEnemy >= 0)
	{
		if (!(sprite[*nEnemy].cstat & 0x101)) {
			*nEnemy = -1;
		}
	}

	return *nEnemy;
}
