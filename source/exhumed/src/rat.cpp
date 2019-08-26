
#include "engine.h"
#include "rat.h"
#include "sequence.h"
#include "runlist.h"
#include "random.h"
#include "view.h"
#include "init.h"
#include "exhumed.h"
#include "move.h"
#include <assert.h>

#define kMaxRats	50

short nMinChunk;
short nPlayerPic; // why is this here?
short nRatCount;
short nMaxChunk;

struct Rat
{
	short a;
	short nAction;
	short nSprite;
	short d;
	short nTarget;
	short f;
	short g;
	short _pad;
};

Rat RatList[kMaxRats];

static actionSeq ActionSeq[] = { { 0, 1}, {1, 0}, {1, 0}, {9, 1}, {0, 1} };


void InitRats()
{
	nRatCount = 0;
	nMinChunk = 9999;
	nMaxChunk = -1;

	for (int i = 122; i < 131; i++)
	{
		int nPic = seq_GetSeqPicnum(kSeqJoe, i, 0);

		if (nPic < nMinChunk)
			nMinChunk = nPic;

		if (nPic > nMaxChunk)
			nMaxChunk = nPic;
	}

	nPlayerPic = seq_GetSeqPicnum(kSeqJoe, 120, 0);
}

void SetRatVel(short nSprite)
{
	sprite[nSprite].xvel = (short)Sin(sprite[nSprite].ang + 512) >> 2;
	sprite[nSprite].yvel = (short)Sin(sprite[nSprite].ang) >> 2;
}

int BuildRat(short nSprite, int x, int y, int z, short nSector, int nAngle)
{
	if (nRatCount >= kMaxRats) {
		return -1;
	}

	short nRat = nRatCount++;

	if (nSprite < 0)
	{
		nSprite = insertsprite(nSector, 108);
		assert(nSprite >= 0 && nSprite < kMaxSprites);

		sprite[nSprite].x = x;
		sprite[nSprite].y = y;
		sprite[nSprite].z = z;
	}
	else
	{
		sprite[nSprite].ang = nAngle;
		changespritestat(nSprite, 108);
	}

	sprite[nSprite].cstat = 0x101;
	sprite[nSprite].shade = -12;
	sprite[nSprite].xoffset = 0;
	sprite[nSprite].yoffset = 0;
	sprite[nSprite].picnum = 1;
	sprite[nSprite].pal = sector[sprite[nSprite].sectnum].ceilingpal;
	sprite[nSprite].clipdist = 30;
	sprite[nSprite].ang = nAngle;
	sprite[nSprite].xrepeat = 50;
	sprite[nSprite].yrepeat = 50;
	sprite[nSprite].yvel = 0;
	sprite[nSprite].xvel = 0;
	sprite[nSprite].zvel = 0;
	sprite[nSprite].lotag = runlist_HeadRun() + 1;
	sprite[nSprite].hitag = 0;
	sprite[nSprite].extra = -1;

	if (nAngle >= 0) {
		RatList[nRat].nAction = 2;
	}
	else {
		RatList[nRat].nAction = 4;
	}

	RatList[nRat].a = 0;
	RatList[nRat].nSprite = nSprite;
	RatList[nRat].nTarget = -1;
	RatList[nRat].f = RandomSize(5);
	RatList[nRat].g = RandomSize(3);

	sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nRat | 0x240000);

	RatList[nRat].d = runlist_AddRunRec(NewRun, nRat | 0x240000);
	return 0;
}

int FindFood(short nSprite)
{
	short nSector = sprite[nSprite].sectnum;
	int x = sprite[nSprite].x;
	int y = sprite[nSprite].y;
	int z = sprite[nSprite].z;

	int z2 = z + sector[nSector].ceilingz;

	if (nChunkTotal)
	{
		int nSprite2 = nChunkSprite[RandomSize(7) % nChunkTotal];
		if (nSprite2 != -1)
		{
			if (cansee(x, y, z2, nSector, sprite[nSprite2].x, sprite[nSprite2].y, sprite[nSprite2].z, sprite[nSprite2].sectnum)) {
				return nSprite2;
			}
		}
	}

	if (!nBodyTotal) {
		return -1;
	}

	int nSprite2 = nChunkSprite[RandomSize(7) % nBodyTotal];
	if (nSprite2 != -1)
	{
		if (nPlayerPic == sprite[nSprite2].picnum)
		{
			if (cansee(x, y, z, nSector, sprite[nSprite2].x, sprite[nSprite2].y, sprite[nSprite2].z, sprite[nSprite2].sectnum)) {
				return nSprite2;
			}
		}
	}

	return -1;
}

void FuncRat(int a, int nDamage, int nRun)
{
	short nRat = RunData[nRun].nVal;
	short nSprite = RatList[nRat].nSprite;

	short nAction = RatList[nRat].nAction;

	int var_20 = 0;

	int nMessage = a & 0x7F0000;

	switch (nMessage)
	{
		default:
		{
			DebugOut("unknown msg %d for Rathead\n", nMessage);
			return;
		}

		case 0xA0000:
		{
			nDamage = runlist_CheckRadialDamage(nSprite);
			// fall through to 0x80000
		}
		case 0x80000:
		{
			if (nDamage)
			{
				sprite[nSprite].cstat = 0;
				sprite[nSprite].yvel = 0;
				sprite[nSprite].xvel = 0;
				RatList[nRat].nAction = 3;
				RatList[nRat].a = 0;
			}
			break;
		}

		case 0x90000:
		{
			seq_PlotSequence(a, SeqOffsets[kSeqRat] + ActionSeq[nAction].a, RatList[nRat].a, ActionSeq[nAction].b);
			return;
		}

		case 0x20000:
		{
			int nSeq = SeqOffsets[kSeqRat] + ActionSeq[nAction].a;
			sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, RatList[nRat].a);
			
			seq_MoveSequence(nSprite, nSeq, RatList[nRat].a);

			RatList[nRat].a++;
			if (RatList[nRat].a >= SeqSize[nSeq])
			{
				var_20 = 1;
				RatList[nRat].a = 0;
			}

			short nTarget = RatList[nRat].nTarget;

			Gravity(nSprite);

			switch (nAction)
			{
				default:
				{
					return;
				}

				case 0:
				{
					RatList[nRat].f--;
					if (RatList[nRat].f > 0) {
						return;
					}

					int xVal = sprite[nSprite].x - sprite[nTarget].x;
					if (xVal < 0) {
						xVal = -xVal;
					}

					int yVal = sprite[nSprite].y - sprite[nTarget].y;
					if (yVal < 0) {
						yVal = -yVal;
					}

					if (xVal > 50 || yVal > 50)
					{
						RatList[nRat].nAction = 2;
						RatList[nRat].a = 0;
						RatList[nRat].nTarget = -1;

						sprite[nSprite].yvel = 0;
						sprite[nSprite].xvel = 0;
						return;
					}

					RatList[nRat].a ^= 1;
					RatList[nRat].f = RandomSize(5) + 4;
					RatList[nRat].g--;

					if (RatList[nRat].g <= 0)
					{
						short nFoodSprite = FindFood(nSprite);
						if (nFoodSprite == -1) {
							return;
						}

						RatList[nRat].nTarget = nFoodSprite;

						PlotCourseToSprite(nSprite, nFoodSprite);
						SetRatVel(nSprite);

						RatList[nRat].nAction = 1;
						RatList[nRat].g = 900;
						RatList[nRat].a = 0;
					}

					return;
				}
				case 1:
				{
					RatList[nRat].g--;

					if (RatList[nRat].g <= 0)
					{
						RatList[nRat].nAction = 2;
						RatList[nRat].a = 0;
						sprite[nSprite].yvel = 0;
						RatList[nRat].nTarget = -1;
						sprite[nSprite].xvel = 0;
					}

					MoveCreature(nSprite);

					int xVal = sprite[nSprite].x - sprite[nTarget].x;
					if (xVal < 0) {
						xVal = -xVal;
					}

					int yVal = sprite[nSprite].y - sprite[nTarget].y;
					if (yVal < 0) {
						yVal = -yVal;
					}

					if (xVal >= 50 || yVal >= 50)
					{
						RatList[nRat].f--;
						if (RatList[nRat].f < 0)
						{
							PlotCourseToSprite(nSprite, nTarget);
							SetRatVel(nSprite);

							RatList[nRat].f = 32;
						}

						return;
					}

					RatList[nRat].nAction = 0;
					RatList[nRat].a = 0;

					sprite[nSprite].yvel = 0;
					sprite[nSprite].xvel = 0;

					RatList[nRat].g = RandomSize(3);
					return;
				}
				case 2:
				{
					if (sprite[nSprite].xvel | sprite[nSprite].yvel | sprite[nSprite].zvel) {
						MoveCreature(nSprite);
					}

					RatList[nRat].f--;
					if (RatList[nRat].f <= 0)
					{
						RatList[nRat].nTarget = FindFood(nSprite);

						if (RatList[nRat].nTarget <= -1)
						{
							RatList[nRat].f = RandomSize(6);
							if (sprite[nSprite].xvel | sprite[nSprite].yvel)
							{
								sprite[nSprite].yvel = 0;
								sprite[nSprite].xvel = 0;
								return;
							}

							sprite[nSprite].ang = RandomSize(11);
							SetRatVel(nSprite);
							return;
						}
						else
						{
							PlotCourseToSprite(nSprite, RatList[nRat].nTarget);
							SetRatVel(nSprite);
							RatList[nRat].nAction = 1;
							RatList[nRat].g = 900;
							RatList[nRat].a = 0;
							return;
						}
					}

					return;
				}
				case 3:
				{
					if (var_20 != 0)
					{
						runlist_DoSubRunRec(sprite[nSprite].owner);
						runlist_FreeRun(sprite[nSprite].lotag - 1);
						runlist_SubRunRec(RatList[nRat].d);

						sprite[nSprite].cstat = 0x8000;
						mydeletesprite(nSprite);
					}
					return;
				}
			}

			break;
		}
	}
}
