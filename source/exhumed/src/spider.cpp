
#include "exhumed.h"
#include "spider.h"
#include "engine.h"
#include "runlist.h"
#include "move.h"
#include "sequence.h"
#include "random.h"
#include "sound.h"
#include "trigdat.h"
#include <assert.h>

short SpiderSprite = -1;
short SpiderCount = 0;

#define kMaxSpiders		100

struct Spider
{
	short nHealth;
	short b;
	short nAction;
	short nSprite;
	short nTarget;
	short f;
	short g;
	short h;
};

Spider SpiderList[kMaxSpiders];

static actionSeq ActionSeq[] = {
	{16, 0},
	{8, 0},
	{32, 0},
	{24, 0},
	{0, 0},
	{40, 1},
	{41, 1},
};

void InitSpider()
{
	SpiderCount = 0;
	SpiderSprite = 1;
}

int BuildSpider(int nSprite, int x, int y, int z, short nSector, int angle)
{
	SpiderCount++;

	int nSpider = SpiderCount;
	if (nSpider >= kMaxSpiders) {
		return -1;
	}

	if (nSprite == -1)
	{
		nSprite = insertsprite(nSector, 99);
	}
	else
	{
		changespritestat(nSprite, 99);
		x = sprite[nSprite].x;
		y = sprite[nSprite].y;
		z = sprite[nSprite].z;
		angle = sprite[nSprite].ang;
	}

	assert(nSprite >= 0 && nSprite < kMaxSprites);

	sprite[nSprite].x = x;
	sprite[nSprite].y = y;
	sprite[nSprite].z = z;
	sprite[nSprite].cstat = 257;
	sprite[nSprite].shade = -12;
	sprite[nSprite].clipdist = 15;
	sprite[nSprite].xvel = 0;
	sprite[nSprite].yvel = 0;
	sprite[nSprite].zvel = 0;
	sprite[nSprite].xrepeat = 40;
	sprite[nSprite].yrepeat = 40;
	sprite[nSprite].pal = sector[sprite[nSprite].sectnum].ceilingpal;
	sprite[nSprite].xoffset = 0;
	sprite[nSprite].yoffset = 0;
	sprite[nSprite].ang = angle;
	sprite[nSprite].picnum = 1;
	sprite[nSprite].hitag = 0;
	sprite[nSprite].lotag = runlist_HeadRun() + 1;
	sprite[nSprite].extra = -1;

//	GrabTimeSlot(3);

	SpiderList[nSpider].nAction = 0;
	SpiderList[nSpider].b = 0;
	SpiderList[nSpider].nSprite = nSprite;
	SpiderList[nSpider].nTarget = -1;
	SpiderList[nSpider].nHealth = 160;

	sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nSpider | 0xC0000);

	SpiderList[nSpider].h = runlist_AddRunRec(NewRun, nSpider | 0xC0000);
	
	nCreaturesLeft++;
		
	return nSpider | 0xC0000;
}

void FuncSpider(int a, int nDamage, int nRun)
{
	short nSpider = RunData[nRun].nVal;
	assert(nSpider >= 0 && nSpider < kMaxSpiders);

	int var_14;

	short nSprite = SpiderList[nSpider].nSprite;
	short nAction = SpiderList[nSpider].nAction;

	int nMessage = a & 0x7F0000;

	switch (nMessage)
	{
		case 0x20000:
		{
			var_14 = 6;

			if (SpiderList[nSpider].nHealth)
			{
				if (sprite[nSprite].cstat & 8)
				{
					sprite[nSprite].z = sector[sprite[nSprite].sectnum].ceilingz + GetSpriteHeight(nSprite);
				}
				else
				{
					Gravity(nSprite);
				}
			}

			int nSeq = SeqOffsets[kSeqSpider] + ActionSeq[nAction].a;

			sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, SpiderList[nSpider].b);

			seq_MoveSequence(nSprite, nSeq, SpiderList[nSpider].b);

			int nFrameFlag = FrameFlag[SeqBase[nSeq] + SpiderList[nSpider].b];

			SpiderList[nSpider].b++;
			if (SpiderList[nSpider].b >= SeqSize[nSeq]) {
				SpiderList[nSpider].b = 0;
			}

			short nTarget = SpiderList[nSpider].nTarget;

			if (nTarget <= -1 || sprite[nTarget].cstat & 0x101)
			{
				switch (nAction)
				{
					case 0:
					{
						if ((nSpider & 0x1F) == (totalmoves & 0x1F))
						{
							if (nTarget < 0) {
								nTarget = FindPlayer(nSprite, 100);
							}

							if (nTarget >= 0)
							{
								SpiderList[nSpider].nAction = 1;
								SpiderList[nSpider].b = 0;
								SpiderList[nSpider].nTarget = nTarget;

								sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512);
								sprite[nSprite].yvel = sintable[sprite[nSprite].ang];
								return;
							}
						}

						break;
					}
					case 1:
					{
						if (nTarget >= 0) {
							var_14++;
						}
goto case_3;
						break;
					}
					case 4:
					{
						if (!SpiderList[nSpider].b)
						{
							SpiderList[nSpider].b = 0;
							SpiderList[nSpider].nAction = 1;
						}
						//break; // fall through
					}
					case 3:
					{
case_3:
						short nSector = sprite[nSprite].sectnum;

						if (sprite[nSprite].cstat & 8)
						{
							sprite[nSprite].zvel = 0;
							sprite[nSprite].z = sector[nSector].ceilingz + (tilesizy[sprite[nSprite].picnum] << 5);

							if (sector[nSector].ceilingstat & 1)
							{
								sprite[nSprite].cstat ^= 8;
								sprite[nSprite].zvel = 1;

								SpiderList[nSpider].nAction = 3;
								SpiderList[nSpider].b = 0;
							}
						}

						if ((totalmoves & 0x1F) == (nSpider & 0x1F))
						{
							PlotCourseToSprite(nSprite, nTarget);

							if (RandomSize(3))
							{
								sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512);
								sprite[nSprite].yvel = Sin(sprite[nSprite].ang);
							}
							else
							{
								sprite[nSprite].yvel = 0;
								sprite[nSprite].xvel = 0;
							}

							if (SpiderList[nSpider].nAction == 1 && RandomBit())
							{
								if (sprite[nSprite].cstat & 8)
								{
									sprite[nSprite].cstat ^= 8u;
									sprite[nSprite].zvel = 1;
									sprite[nSprite].z = sector[nSector].ceilingz + GetSpriteHeight(nSprite);
								}
								else
								{
									sprite[nSprite].zvel = -5120;
								}

								SpiderList[nSpider].nAction = 3;
								SpiderList[nSpider].b = 0;

								if (!RandomSize(3)) {
									D3PlayFX(StaticSound[kSound29], nSprite);
								}
							}
						}
						break;
					}
					case 5:
					{
						if (!SpiderList[nSpider].b)
						{
							runlist_DoSubRunRec(sprite[nSprite].owner);
							runlist_FreeRun(sprite[nSprite].lotag - 1);
							runlist_SubRunRec(SpiderList[nSpider].h);
							sprite[nSprite].cstat = 0x8000;
							mydeletesprite(nSprite);
						}
						return;
					}
					case 2:
					{
						if (nTarget != -1)
						{
							if (nFrameFlag & 0x80)
							{
								runlist_DamageEnemy(nTarget, nSprite, 3);
								D3PlayFX(StaticSound[kSound38], nSprite);
							}

							if (PlotCourseToSprite(nSprite, nTarget) < 1024) {
								return;
							}

							SpiderList[nSpider].nAction = 1;
						}
						else
						{
							SpiderList[nSpider].nAction = 0;
							sprite[nSprite].xvel = 0;
							sprite[nSprite].yvel = 0;
						}

						SpiderList[nSpider].b = 0;
						break;
					}
				}
			}
			else
			{
				SpiderList[nSpider].nTarget = -1;
				SpiderList[nSpider].nAction = 0;
				sprite[nSprite].yvel = 0;
				sprite[nSprite].xvel = 0;
				SpiderList[nSpider].b = 0;
			}

			int nMov = movesprite(nSprite, sprite[nSprite].xvel << var_14, sprite[nSprite].yvel << var_14, sprite[nSprite].zvel, 1280, -1280, CLIPMASK0);

			if (!nMov)
				return;

			if (nMov & 0x10000
				&& sprite[nSprite].zvel < 0
				&& (hihit & 0xC000) != 0xC000
				&& !((sector[sprite[nSprite].sectnum].ceilingstat) & 1))
			{
				sprite[nSprite].cstat |= 8;
				sprite[nSprite].z = GetSpriteHeight(nSprite) + sector[sprite[nSprite].sectnum].ceilingz;
				sprite[nSprite].zvel = 0;

				SpiderList[nSpider].nAction = 1;
				SpiderList[nSpider].b = 0;
				return;
			}
			else
			{
				switch (nMov & 0xC000)
				{
					case 0x8000:
					{
						sprite[nSprite].ang = (sprite[nSprite].ang + 256) & 0x7EF;
						sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512);
						sprite[nSprite].yvel = Sin(sprite[nSprite].ang);
						return;
					}
					case 0xC000:
					{
						if ((nMov & 0x3FFF) == nTarget)
						{
							int nAng = getangle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].y - sprite[nSprite].y);
							if (AngleDiff(sprite[nSprite].ang, nAng) < 64)
							{
								SpiderList[nSpider].nAction = 2;
								SpiderList[nSpider].b = 0;
							}
						}
						return;
					}
				}

				if (SpiderList[nSpider].nAction == 3)
				{
					SpiderList[nSpider].nAction = 1;
					SpiderList[nSpider].b = 0;
				}
				return;
			}

			return;
		}

		case 0x90000:
		{
			seq_PlotSequence(a & 0xFFFF, SeqOffsets[kSeqSpider] + ActionSeq[nAction].a, SpiderList[nSpider].b, ActionSeq[nAction].b);
			break;
		}
		
		case 0xA0000:
		{
			if (SpiderList[nSpider].nHealth <= 0)
				return;

			nDamage = runlist_CheckRadialDamage(nSprite);
			// fall through
		}

		case 0x80000:
		{
			if (!nDamage)
				return;

			short nTarget = a & 0xFFFF;

			SpiderList[nSpider].nHealth -= nDamage;
			if (SpiderList[nSpider].nHealth > 0)
			{
				if (sprite[nTarget].statnum == 100)
				{
					SpiderList[nSpider].nTarget = nTarget;
				}

				SpiderList[nSpider].nAction = 4;
				SpiderList[nSpider].b = 0;
			}
			else
			{
				// creature is dead, make some chunks
				SpiderList[nSpider].nHealth = 0;
				sprite[nSprite].cstat &= 0x0FEFE;

				SpiderList[nSpider].nAction = 5;
				nCreaturesLeft--;
				SpiderList[nSpider].b = 0;

				for (int i = 0; i < 7; i++)
				{
					BuildCreatureChunk(nSprite, seq_GetSeqPicnum(kSeqSpider, i + 41, 0));
				}
			}

			return;
		}

		default:
		{
			DebugOut("unknown msg %d for Spider\n", a & 0x7F0000);
			break;
		}
	}
}
