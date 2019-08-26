
#include "engine.h"
#include "bullet.h"
#include "runlist.h"
#include "anims.h"
#include "sequence.h"
#include "exhumed.h"
#include "sound.h"
#include "init.h"
#include "move.h"
#include "player.h"
#include "trigdat.h"
#include "random.h"
#include "gun.h"
#include "names.h"
#include "lighting.h"
#include <string.h>
#include <assert.h>
#ifndef __WATCOMC__
//#include <cmath>
#else
//#include <math.h>
#include <stdlib.h>
#endif

#define kMaxBullets		500

short BulletFree[kMaxBullets];

// 32 bytes
struct Bullet
{
	short nSeq; // 0
	short field_2; // 2
	short nSprite; // 4
	short field_6;
	short field_8;
	short nType;
	short field_C;
	short field_E;
	short field_10;
	uchar field_12;
	uchar field_13;
	int x;
	int y;
	int z;
};

Bullet BulletList[kMaxBullets];
short nBulletEnemy[kMaxBullets];
int nBulletsFree;
int lasthitz, lasthitx, lasthity;
short lasthitsect, lasthitsprite, lasthitwall;

int nBulletCount = 0;
short nRadialBullet = 0;

bulletInfo BulletInfo[] = {
	{ 25,   1,    20, -1, -1, 13, 0,  0, -1, 0 },
	{ 25,  -1, 65000, -1, 31, 73, 0,  0, -1, 0 },
	{ 15,  -1, 60000, -1, 31, 73, 0,  0, -1, 0 },
	{ 5,   15,  2000, -1, 14, 38, 4,  5,  3, 0 },
	{ 250, 100, 2000, -1, 33, 34, 4, 20, -1, 0 },
	{ 200, -1,  2000, -1, 20, 23, 4, 10, -1, 0 },
	{ 200, -1, 60000, 68, 68, -1, -1, 0, -1, 0 },
	{ 300,  1,     0, -1, -1, -1, 0, 50, -1, 0 },
	{ 18,  -1,  2000, -1, 18, 29, 4,  0, -1, 0 },
	{ 20,  -1,  2000, 37, 11, 30, 4,  0, -1, 0 },
	{ 25,  -1,  3000, -1, 44, 36, 4, 15, 90, 0 },
	{ 30,  -1,  1000, -1, 52, 53, 4, 20, 48, 0 },
	{ 20,  -1,  3500, -1, 54, 55, 4, 30, -1, 0 },
	{ 10,  -1,  5000, -1, 57, 76, 4,  0, -1, 0 },
	{ 40,  -1,  1500, -1, 63, 38, 4, 10, 40, 0 },
	{ 20,  -1,  2000, -1, 60, 12, 0,  0, -1, 0 },
	{ 5,   -1, 60000, -1, 31, 76, 0,  0, -1, 0 }
};


void InitBullets()
{
	nBulletCount = 0;

	for (int i = 0; i < kMaxBullets; i++) {
		BulletFree[i] = i;
	}

	nBulletsFree = kMaxBullets;

	memset(nBulletEnemy, -1, sizeof(nBulletEnemy));
}

short GrabBullet()
{
	nBulletsFree--;
	return BulletFree[nBulletsFree];
}

void DestroyBullet(short nBullet)
{
	short nSprite = BulletList[nBullet].nSprite;

	runlist_DoSubRunRec(BulletList[nBullet].field_6);
	runlist_DoSubRunRec(sprite[nSprite].lotag - 1);
	runlist_SubRunRec(BulletList[nBullet].field_8);

	StopSpriteSound(nSprite);

	mydeletesprite(nSprite);

	BulletFree[nBulletsFree] = nBullet;
	nBulletsFree++;
}

void IgniteSprite(int nSprite)
{
	sprite[nSprite].hitag += 2;

	int nAnim = BuildAnim(-1, 38, 0, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum, 40, 20);//276);
	short nAnimSprite = GetAnimSprite(nAnim);

	sprite[nAnimSprite].hitag = nSprite;
	changespritestat(nAnimSprite, kStatIgnited);

	short yRepeat = (tilesiz[sprite[nAnimSprite].picnum].y * 32) / nFlameHeight;
	if (yRepeat < 1)
		yRepeat = 1;

	sprite[nAnimSprite].yrepeat = (uchar)yRepeat;
}

void BulletHitsSprite(Bullet *pBullet, short nBulletSprite, short nHitSprite, int x, int y, int z, int nSector)
{
	assert(nSector >= 0 && nSector < kMaxSectors);

	bulletInfo *pBulletInfo = &BulletInfo[pBullet->nType];

	short nStat = sprite[nHitSprite].statnum;

	switch (pBullet->nType)
	{
		case 0:
		case 4:
		case 5:
		case 6:
		case 7:
		case 10:
		case 11:
		{
			break;
		}

		case 14:
		{
			if (nStat > 107 || nStat == 98) {
				return;
			}
			// else - fall through to below cases
		}
		case 1:
		case 2:
		case 8:
		case 9:
		case 12:
		case 13:
		case 15:
		case 16:
		{
			// loc_29E59
			if (!nStat || nStat > 98) {
				break;
			}

			short nSprite = pBullet->nSprite;

			if (nStat == 98)
			{
				short nAngle = sprite[nSprite].ang + 256;
				int nRand = RandomSize(9);

				sprite[nHitSprite].xvel = Sin((nAngle - nRand) + 512) * 2;
				sprite[nHitSprite].yvel = Sin((nAngle - nRand)) * 2;
				sprite[nHitSprite].zvel = (-(RandomSize(3) + 1)) << 8;
			}
			else
			{
				int xVel = sprite[nHitSprite].xvel;
				int yVel = sprite[nHitSprite].yvel;

				sprite[nHitSprite].xvel = Sin(sprite[nSprite].ang + 512) >> 2;
				sprite[nHitSprite].yvel = Sin(sprite[nSprite].ang) >> 2;

				MoveCreature(nHitSprite);

				sprite[nHitSprite].xvel = xVel;
				sprite[nHitSprite].yvel = yVel;
			}

			break;
		}

		case 3:
		{
			if (nStat > 107 || nStat == 98) {
				return;
			}

			sprite[nHitSprite].hitag++;

			if (sprite[nHitSprite].hitag == 15) {
				IgniteSprite(nHitSprite);
			}

			if (!RandomSize(2)) {
				BuildAnim(-1, pBulletInfo->field_C, 0, x, y, z, nSector, 40, pBulletInfo->nFlags);
			}

			return;
		}

		default:
			break;
	}

	// BHS_switchBreak:
	short nDamage = pBulletInfo->nDamage;

	if (pBullet->field_13 > 1) {
		nDamage *= 2;
	}

	runlist_DamageEnemy(nHitSprite, nBulletSprite, nDamage);

	if (nStat <= 90 || nStat >= 199)
	{
		BuildAnim(-1, pBulletInfo->field_C, 0, x, y, z, nSector, 40, pBulletInfo->nFlags);
		return;
	}

	if (nStat < 98)
	{
		if (nStat <= 0)
		{
			BuildAnim(-1, 12, 0, x, y, z, nSector, 40, 0);
		}
		else if (nStat == 97)
		{
			return;
		}
		else
		{
			// BHS_B
			BuildAnim(-1, 39, 0, x, y, z, nSector, 40, 0);
			if (pBullet->nType > 2)
			{
				BuildAnim(-1, pBulletInfo->field_C, 0, x, y, z, nSector, 40, pBulletInfo->nFlags);
			}
		}

		return;
	}
	else
	{ 
		if (nStat == 98 || nStat == 141 || nStat == 152 || nStat == 102)
		{
			BuildAnim(-1, 12, 0, x, y, z, nSector, 40, 0);
		}
		else
		{
			// BHS_B
			BuildAnim(-1, 39, 0, x, y, z, nSector, 40, 0);
			if (pBullet->nType > 2)
			{
				BuildAnim(-1, pBulletInfo->field_C, 0, x, y, z, nSector, 40, pBulletInfo->nFlags);
			}
		}

		return;
	}
}


void BackUpBullet(int *x, int *y, short nAngle)
{
	*x -= Sin(nAngle + 512) >> 11;
	*y -= Sin(nAngle) >> 11;
}

int MoveBullet(short nBullet)
{
	short hitsect = -1;
	short hitwall = -1;
	short hitsprite = -1;

	short nType = BulletList[nBullet].nType;
	short nSprite = BulletList[nBullet].nSprite;

	int x = sprite[nSprite].x;
	int y = sprite[nSprite].y;
	int z = sprite[nSprite].z; // ebx
	short nSectFlag = SectFlag[sprite[nSprite].sectnum];

	int x2, y2, z2;

	short var_3C = BulletList[nBullet].field_10;

	int nVal;

	if (var_3C < 30000)
	{
		short nEnemySprite = nBulletEnemy[nBullet];

		if (nBulletEnemy[nBullet] <= -1) {
			goto MoveBullet_goto_A; // FIXME
		}

		if (!(sprite[nEnemySprite].cstat & 0x101))
		{
			nBulletEnemy[nBullet] = -1;
MoveBullet_goto_A:
			if (nType == 3)
			{
				if (BulletList[nBullet].field_E >= 8)
				{
					sprite[nSprite].xrepeat += 4;
					sprite[nSprite].yrepeat += 4;
				}
				else
				{
					sprite[nSprite].xrepeat -= 1;
					sprite[nSprite].yrepeat += 8;

					BulletList[nBullet].z -= 200;

					if (sprite[nSprite].shade < 90) {
						sprite[nSprite].shade += 35;
					}

					if (BulletList[nBullet].field_E == 3)
					{
						BulletList[nBullet].nSeq = 45;
						BulletList[nBullet].field_2 = 0;
						sprite[nSprite].xrepeat = 40;
						sprite[nSprite].yrepeat = 40;
						sprite[nSprite].shade = 0;
						sprite[nSprite].z += 512;
					}
				}
			}

			// loc_2A1DD
			nVal = movesprite(nSprite, BulletList[nBullet].x, BulletList[nBullet].y, BulletList[nBullet].z, sprite[nSprite].clipdist >> 1, sprite[nSprite].clipdist >> 1, CLIPMASK1);
		}
		else
		{
			nVal = AngleChase(nSprite, nEnemySprite, var_3C, 0, 16);
		}

		if (nVal)
		{
			x2 = sprite[nSprite].x;
			y2 = sprite[nSprite].y;
			z2 = sprite[nSprite].z;
			hitsect = sprite[nSprite].sectnum;

			if (nVal & 0x30000)
			{
				hitwall = nVal & 0x3FFF;
				goto loc_2A48A;
			}
			else
			{
				if ((nVal & 0xC000) == 0x8000)
				{
					hitwall = nVal & 0x3FFF;
					goto loc_2A48A;
				}
				else if ((nVal & 0xC000) == 0xC000)
				{
					hitsprite = nVal & 0x3FFF;
					goto loc_2A405;
				}
				else {
					goto loc_2A25F;
				}
			}
		}
		else
		{
loc_2A25F:
			// sprite[nSprite].sectnum may have changed since we set nSectFlag ?
			short nFlagVal = nSectFlag ^ SectFlag[sprite[nSprite].sectnum];
			if (nFlagVal & kSectUnderwater)
			{
				DestroyBullet(nBullet);
				nVal = 1;
			}

			if (nVal) {
				return nVal;
			}

			if (nType == 15)
			{
				return nVal;
			}

			if (nType != 3)
			{
				AddFlash(sprite[nSprite].sectnum, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, nVal);

				if (sprite[nSprite].pal != 5) {
					sprite[nSprite].pal = 1;
				}
			}
		}
	}
	else
	{
		nVal = 1;

		if (nBulletEnemy[nBullet] > -1)
		{
			hitsprite = nBulletEnemy[nBullet];
			x2 = sprite[hitsprite].x;
			y2 = sprite[hitsprite].y;
			z2 = sprite[hitsprite].z - (GetSpriteHeight(hitsprite) >> 1);
			hitsect = sprite[hitsprite].sectnum;
		}
		else
		{
            vec3_t startPos = { x, y, z };
            hitdata_t hitData = { { x2, y2, z2 }, hitsprite, hitsect, hitwall };
			hitscan(&startPos, sprite[nSprite].sectnum, Sin(sprite[nSprite].ang + 512), Sin(sprite[nSprite].ang), (-Sin(BulletList[nBullet].field_C)) << 3, &hitData, CLIPMASK1);
            x2 = hitData.pos.x;
            y2 = hitData.pos.y;
            z2 = hitData.pos.z;
            hitsprite = hitData.sprite;
            hitsect = hitData.sect;
            hitwall = hitData.wall;
		}

		lasthitx = x2;
		lasthity = y2;
		lasthitz = z2;
		lasthitsect = hitsect;
		lasthitwall = hitwall;
		lasthitsprite = hitsprite;

		if (lasthitsprite > -1)
		{
loc_2A405:
			if (sprite[nSprite].pal != 5 || sprite[hitsprite].statnum != 100)
			{
//				assert(hitsect <= kMaxSectors);

				BulletHitsSprite(&BulletList[nBullet], sprite[nSprite].owner, hitsprite, x2, y2, z2, hitsect);
			}
			else
			{
				short nPlayer = GetPlayerFromSprite(hitsprite);
				if (!PlayerList[nPlayer].bIsMummified)
				{
					PlayerList[nPlayer].bIsMummified = kTrue;
					SetNewWeapon(nPlayer, kWeaponMummified);
				}
			}
		}
		else if (hitwall > -1)
		{
loc_2A48A:
			if (wall[hitwall].picnum == kEnergy1)
			{
				short nSector = wall[hitwall].nextsector;
				if (nSector > -1)
				{
					short nDamage = BulletInfo[BulletList[nBullet].nType].nDamage;
					if (BulletList[nBullet].field_13 > 1) {
						nDamage *= 2;
					}

					short nNormal = GetWallNormal(hitwall) & 0x7FF;

					runlist_DamageEnemy(sector[nSector].extra, nNormal, nDamage);
				}
			}
		}

		// loc_2A4F5:?
		if (hitsprite < 0 && hitwall < 0)
		{
			if ((SectBelow[hitsect] >= 0 && (SectFlag[SectBelow[hitsect]] & kSectUnderwater)) || SectDepth[hitsect])
			{
				sprite[nSprite].x = x2;
				sprite[nSprite].y = y2;
				sprite[nSprite].z = z2;
				BuildSplash(nSprite, hitsect);
			}
			else
			{
				BuildAnim(-1, BulletInfo[nType].field_C, 0, x2, y2, z2, hitsect, 40, BulletInfo[nType].nFlags);
			}
		}
		else
		{
//			short nType = BulletList[nBullet].nType;
			
			if (hitwall < 0)
			{
				sprite[nSprite].x = x2;
				sprite[nSprite].y = y2;
				sprite[nSprite].z = z2;

				mychangespritesect(nSprite, hitsect);
			}
			else
			{
				BackUpBullet(&x2, &y2, sprite[nSprite].ang);

				if (nType != 3 || !RandomSize(2))
				{
					int zOffset = RandomSize(8) << 3;

					if (!RandomBit()) {
						zOffset = -zOffset;
					}

					// draws bullet puff on walls when they're shot
					BuildAnim(-1, BulletInfo[nType].field_C, 0, x2, y2, z2 + zOffset, hitsect, 40, BulletInfo[nType].nFlags);
				}
			}

			// loc_2A639:
			if (BulletInfo[nType].field_10)
			{
				nRadialBullet = nType;

				runlist_RadialDamageEnemy(nSprite, BulletInfo[nType].nDamage, BulletInfo[nType].field_10);

				nRadialBullet = -1;

				AddFlash(sprite[nSprite].sectnum, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, 128);
			}
		}

		DestroyBullet(nBullet);
	}

	return nVal;
}

void SetBulletEnemy(short nBullet, short nEnemy)
{
	if (nBullet >= 0) {
		nBulletEnemy[nBullet] = nEnemy;
	}
}

int BuildBullet(short nSprite, int nType, int ebx, int ecx, int val1, int nAngle, int val2, int val3)
{
	Bullet sBullet;

	if (BulletInfo[nType].field_4 > 30000)
	{
		if (val2 >= 10000)
		{
			val2 -= 10000;

			short nTargetSprite = val2;

//			assert(sprite[nTargetSprite].sectnum <= kMaxSectors);

			if (sprite[nTargetSprite].cstat & 0x101)
			{
				sBullet.nType = nType;
				sBullet.field_13 = val3;

				sBullet.nSprite = insertsprite(sprite[nSprite].sectnum, 200);
				sprite[sBullet.nSprite].ang = nAngle;

				int nHeight = GetSpriteHeight(nTargetSprite);

				assert(sprite[nTargetSprite].sectnum >= 0 && sprite[nTargetSprite].sectnum < kMaxSectors);

				BulletHitsSprite(&sBullet, nSprite, nTargetSprite, sprite[nTargetSprite].x, sprite[nTargetSprite].y, sprite[nTargetSprite].z - (nHeight >> 1), sprite[nTargetSprite].sectnum);
				mydeletesprite(sBullet.nSprite);
				return -1;
			}
			else
			{
				val2 = 0;
			}
		}
	}

	if (!nBulletsFree) {
		return -1;
	}

	short nSector;

	if (sprite[nSprite].statnum != 100)
	{
		nSector = sprite[nSprite].sectnum;
	}
	else
	{
		nSector = nPlayerViewSect[GetPlayerFromSprite(nSprite)];
	}

	int nBulletSprite = insertsprite(nSector, 200);
	int nHeight = GetSpriteHeight(nSprite);
	nHeight = nHeight - (nHeight >> 2);

	if (val1 == -1) {
		val1 = -nHeight;
	}

	sprite[nBulletSprite].x = sprite[nSprite].x;
	sprite[nBulletSprite].y = sprite[nSprite].y;
	sprite[nBulletSprite].z = sprite[nSprite].z;

	// why is this done here???
	assert(nBulletSprite >= 0 && nBulletSprite < kMaxSprites);

	short nBullet = GrabBullet();

	nBulletEnemy[nBullet] = -1;

	sprite[nBulletSprite].cstat = 0;
	sprite[nBulletSprite].shade = -64;

	if (BulletInfo[nType].nFlags & 4) {
		sprite[nBulletSprite].pal = 4;
	}
	else {
		sprite[nBulletSprite].pal = 0;
	}

	sprite[nBulletSprite].clipdist = 25;

	short nRepeat = BulletInfo[nType].xyRepeat;
	if (nRepeat < 0) {
		nRepeat = 30;
	}

	sprite[nBulletSprite].xrepeat = nRepeat;
	sprite[nBulletSprite].yrepeat = nRepeat;
	sprite[nBulletSprite].xoffset = 0;
	sprite[nBulletSprite].yoffset = 0;
	sprite[nBulletSprite].ang = nAngle;
	sprite[nBulletSprite].xvel = 0;
	sprite[nBulletSprite].yvel = 0;
	sprite[nBulletSprite].zvel = 0;
	sprite[nBulletSprite].owner = nSprite;
	sprite[nBulletSprite].lotag = runlist_HeadRun() + 1;
	sprite[nBulletSprite].extra = -1;
	sprite[nBulletSprite].hitag = 0;

//	GrabTimeSlot(3);

	BulletList[nBullet].field_10 = 0;
	BulletList[nBullet].field_E = BulletInfo[nType].field_2;
	BulletList[nBullet].field_2 = 0;

	short nSeq;

	if (BulletInfo[nType].field_8 == -1)
	{
		BulletList[nBullet].field_12 = 1;
		nSeq = BulletInfo[nType].nSeq;
	}
	else
	{
		BulletList[nBullet].field_12 = 0;
		nSeq = BulletInfo[nType].field_8;
	}

	BulletList[nBullet].nSeq = nSeq;

	sprite[nBulletSprite].picnum = seq_GetSeqPicnum(nSeq, 0, 0);

	if (nSeq == kSeqBullet) {
		sprite[nBulletSprite].cstat |= 0x8000;
	}

	BulletList[nBullet].field_C = val2 & 0x7FF; // TODO - anglemask?
	BulletList[nBullet].nType = nType;
	BulletList[nBullet].nSprite = nBulletSprite;
	BulletList[nBullet].field_6 = runlist_AddRunRec(sprite[nBulletSprite].lotag - 1, nBullet | 0xB0000);
	BulletList[nBullet].field_8 = runlist_AddRunRec(NewRun, nBullet | 0xB0000);
	BulletList[nBullet].field_13 = val3;
	sprite[nBulletSprite].z += val1;

	int var_18;

	nSector = sprite[nBulletSprite].sectnum;

	while (1)
	{
		if (sprite[nBulletSprite].z >= sector[nSector].ceilingz) {
			break;
		}

		if (SectAbove[nSector] == -1)
		{
			sprite[nBulletSprite].z = sector[nSector].ceilingz;
			break;
		}

		nSector = SectAbove[nSector];
		mychangespritesect(nBulletSprite, nSector);
	}

	if (val2 < 10000)
	{
		var_18 = ((-Sin(val2)) * BulletInfo[nType].field_4) >> 11;
	}
	else
	{
		val2 -= 10000;

		short nTargetSprite = val2;

		if (BulletInfo[nType].field_4 > 30000)
		{
			nBulletEnemy[nBullet] = nTargetSprite;
		}
		else
		{
			nHeight = GetSpriteHeight(nTargetSprite);
			int nHeightAdjust;

			if (sprite[nTargetSprite].statnum == 100)
			{
				nHeightAdjust = nHeight >> 2;
			}
			else
			{
				nHeightAdjust = nHeight >> 1;
			}

			nHeight -= nHeightAdjust;

			int var_20 = sprite[nTargetSprite].z - nHeight;

			int x, y;

			if (nSprite == -1 || sprite[nSprite].statnum == 100)
			{
				// loc_2ABA3:
				x = sprite[nTargetSprite].x - sprite[nBulletSprite].x;
				y = sprite[nTargetSprite].y - sprite[nBulletSprite].y;
			}
			else
			{
				x = sprite[nTargetSprite].x;
				y = sprite[nTargetSprite].y;

				if (sprite[nTargetSprite].statnum == 100)
				{
					int nPlayer = GetPlayerFromSprite(nTargetSprite);
					if (nPlayer > -1)
					{
						x += ((nPlayerDX[nPlayer] << 4) - nPlayerDX[nPlayer]);
						y += ((nPlayerDY[nPlayer] << 4) - nPlayerDY[nPlayer]);
					}
				}
				else
				{
					short nXVel = sprite[nTargetSprite].xvel;
					short nYVel = sprite[nTargetSprite].yvel;

					x += (((nXVel << 2) + nXVel) << 2) >> 6;
					y += (((nYVel << 2) + nYVel) << 2) >> 6;
				}

				y -= sprite[nBulletSprite].y;
				x -= sprite[nBulletSprite].x;

				nAngle = GetMyAngle(x, y);
				sprite[nSprite].ang = nAngle;
			}

			int nSqrt = lsqrt(y*y + x*x);
			if (nSqrt <= 0)
			{
				var_18 = 0;
			}
			else
			{
				var_18 = ((var_20 - sprite[nBulletSprite].z) * BulletInfo[nType].field_4) / nSqrt;
			}
		}
	}

	BulletList[nBullet].z = 0;
	BulletList[nBullet].x = (sprite[nSprite].clipdist << 2) * Sin(nAngle + 512);
	BulletList[nBullet].y = (sprite[nSprite].clipdist << 2) * Sin(nAngle);
	nBulletEnemy[nBullet] = -1;

	if (MoveBullet(nBullet))
	{
		nBulletSprite = -1;
	}
	else
	{
		BulletList[nBullet].field_10 = BulletInfo[nType].field_4;
		BulletList[nBullet].x = (Sin(nAngle + 512) >> 3) * BulletInfo[nType].field_4;
		BulletList[nBullet].y = (Sin(nAngle) >> 3) * BulletInfo[nType].field_4;
		BulletList[nBullet].z = var_18 >> 3;
	}

	return (nBulletSprite & 0xFFFF) | (nBullet << 16);
}

void FuncBullet(int a, int b, int nRun)
{
	short nBullet = RunData[nRun].nVal;
	assert(nBullet >= 0 && nBullet < kMaxBullets);

	short nSeq = SeqOffsets[BulletList[nBullet].nSeq];
	short nSprite = BulletList[nBullet].nSprite;

	int nMessage = a & 0x7F0000;

	switch (nMessage)
	{
		default:
		{
			DebugOut("unknown msg %x for bullet\n", a & 0x7F0000);
			return;
		}

		case 0xA0000:
			return;

		case 0x90000:
		{
			short nSprite2 = a & 0xFFFF;
			tsprite[nSprite2].statnum = 1000;

			if (BulletList[nBullet].nType == 15)
			{
				seq_PlotArrowSequence(nSprite2, nSeq, BulletList[nBullet].field_2);
			}
			else
			{
				seq_PlotSequence(nSprite2, nSeq, BulletList[nBullet].field_2, 0);
				tsprite[nSprite2].owner = -1;
			}
			return;
		}

		case 0x20000:
		{
			short nFlag = FrameFlag[SeqBase[nSeq] + BulletList[nBullet].field_2];

			seq_MoveSequence(nSprite, nSeq, BulletList[nBullet].field_2);

			if (nFlag & 0x80)
			{
				BuildAnim(-1, 45, 0, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum, sprite[nSprite].xrepeat, 0);
			}

			BulletList[nBullet].field_2++;
			if (BulletList[nBullet].field_2 >= SeqSize[nSeq])
			{
				if (!BulletList[nBullet].field_12)
				{
					BulletList[nBullet].nSeq = BulletInfo[BulletList[nBullet].nType].nSeq;
					BulletList[nBullet].field_12++;
				}

				BulletList[nBullet].field_2 = 0;
			}

			if (BulletList[nBullet].field_E == -1)
			{
				MoveBullet(nBullet);
			}
			else
			{
				BulletList[nBullet].field_E--;
				if (!BulletList[nBullet].field_E) {
					DestroyBullet(nBullet);
				}
				else {
					MoveBullet(nBullet);
				}
			}
			return;
		}
	}
}
