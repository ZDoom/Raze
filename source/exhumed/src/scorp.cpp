
#include "engine.h"
#include "scorp.h"
#include "runlist.h"
#include "exhumed.h"
#include "move.h"
#include "sequence.h"
#include "sound.h"
#include "random.h"
#include "trigdat.h"
#include "bullet.h"
#include "spider.h"
#include <assert.h>

/*
	Selkis Boss AI code
*/

short ScorpCount = -1;

Scorpion scorpion[kMaxScorpions];
short ScorpChan[kMaxScorpions];

static actionSeq ActionSeq[] = { 
	{0, 0},
	{8, 0},
	{29, 0},
	{19, 0},
	{45, 1},
	{46, 1},
	{47, 1},
	{48, 1},
	{50, 1},
	{53, 1}
};


void InitScorp()
{
	ScorpCount = kMaxScorpions;
}

int BuildScorp(short nSprite, int x, int y, int z, short nSector, short nAngle, int nChannel)
{
	ScorpCount--;
	if (ScorpCount < 0) {
		return -1;
	}

	short nScorp = ScorpCount;

	if (nSprite == -1)
	{
		nSprite = insertsprite(nSector, 122);
	}
	else
	{
		changespritestat(nSprite, 122);

		x = sprite[nSprite].x;
		y = sprite[nSprite].y;
		z = sector[sprite[nSprite].sectnum].floorz;
		nAngle = sprite[nSprite].ang;
	}

	assert(nSprite >= 0 && nSprite < kMaxSprites);

	sprite[nSprite].x = x;
	sprite[nSprite].y = y;
	sprite[nSprite].z = z;
	sprite[nSprite].cstat = 0x101;
	sprite[nSprite].clipdist = 70;
	sprite[nSprite].shade = -12;
	sprite[nSprite].xrepeat = 80;
	sprite[nSprite].yrepeat = 80;
	sprite[nSprite].picnum = 1;
	sprite[nSprite].pal = sector[sprite[nSprite].sectnum].ceilingpal;
	sprite[nSprite].xoffset = 0;
	sprite[nSprite].yoffset = 0;
	sprite[nSprite].ang = nAngle;
	sprite[nSprite].zvel = 0;
	sprite[nSprite].xvel = 0;
	sprite[nSprite].yvel = 0;
	sprite[nSprite].lotag = runlist_HeadRun() + 1;
	sprite[nSprite].extra = -1;
	sprite[nSprite].hitag = 0;

//	GrabTimeSlot(3);

	scorpion[ScorpCount].nHealth = 20000;
	scorpion[ScorpCount].nFrame = 0;
	scorpion[ScorpCount].nAction = 0;
	scorpion[ScorpCount].nSprite = nSprite;
	scorpion[ScorpCount].nTarget = -1;
	scorpion[ScorpCount].g = 0;
	scorpion[ScorpCount].i = 1;

	ScorpChan[nScorp] = nChannel;

	sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nScorp | 0x220000);
	scorpion[nScorp].f = runlist_AddRunRec(NewRun, nScorp | 0x220000);

	nCreaturesLeft++;

	return nScorp | 0x220000;
}

void FuncScorp(int a, int nDamage, int nRun)
{
	short nScorp = RunData[nRun].nVal;
	assert(nScorp >= 0 && nScorp < kMaxScorpions);

	int edi = 0;

	short nSprite = scorpion[nScorp].nSprite;
	short nAction = scorpion[nScorp].nAction;

	short nTarget = -1;

	int nMessage = a & 0x7F0000;

	switch (nMessage)
	{
		default:
		{
			DebugOut("unknown msg %d for Scorp\n", a & 0x7F0000);
			return;
		}

		case 0x90000:
		{
			seq_PlotSequence(a & 0xFFFF, SeqOffsets[kSeqScorp] + ActionSeq[nAction].a, scorpion[nScorp].nFrame, ActionSeq[nAction].b);
			return;
		}

		case 0xA0000:
		{
			nDamage = runlist_CheckRadialDamage(nSprite);
			if (!nDamage) {
				return;
			}
			// else fall through to case 0x80000
		}

		case 0x80000:
		{
			if (scorpion[nScorp].nHealth <= 0) {
				return;
			}

			scorpion[nScorp].nHealth -= nDamage;

			if (scorpion[nScorp].nHealth <= 0)
			{
				scorpion[nScorp].nHealth = 0;

				sprite[nSprite].xvel = 0;
				sprite[nSprite].yvel = 0;
				sprite[nSprite].zvel = 0;

				scorpion[nScorp].nAction = 4;
				scorpion[nScorp].nFrame = 0;

				sprite[nSprite].cstat &= 0xFEFE;

				nCreaturesLeft--;

				scorpion[nScorp].g = 10;
				return;
			}
			else
			{
				nTarget = a & 0xFFFF;

				if (nTarget >= 0)
				{
					if (sprite[nSprite].statnum == 100 || (sprite[nSprite].statnum < 199 && !RandomSize(5))) 
					{
						scorpion[nScorp].nTarget = nTarget;
					}
				}

				if (!RandomSize(5))
				{
					scorpion[nScorp].nAction = RandomSize(2) + 4;
					scorpion[nScorp].nFrame = 0;
					return;
				}

				if (RandomSize(2)) {
					return;
				}
				
				D3PlayFX(StaticSound[kSound41], nSprite);

				goto FS_Pink_A;
			}
		}

		case 0x20000:
		{
			if (scorpion[nScorp].nHealth) {
				Gravity(nSprite);
			}

			int nSeq = SeqOffsets[kSeqScorp] + ActionSeq[nAction].a;

			sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, scorpion[nScorp].nFrame);
			seq_MoveSequence(nSprite, nSeq, scorpion[nScorp].nFrame);
			
			scorpion[nScorp].nFrame++;

			if (scorpion[nScorp].nFrame >= SeqSize[nSeq])
			{
				scorpion[nScorp].nFrame = 0;
				edi = 1;
			}

			int nFlag = FrameFlag[SeqBase[nSeq] + scorpion[nScorp].nFrame];
			nTarget = scorpion[nScorp].nTarget;

			switch (nAction)
			{
				default:
					return;

				case 0:
				{
					if (scorpion[nScorp].g > 0)
					{
						scorpion[nScorp].g--;
						return;
					}

					if ((nScorp & 31) == (totalmoves & 31))
					{
						if (nTarget < 0)
						{
							nTarget = FindPlayer(nSprite, 500);

							if (nTarget >= 0)
							{
								D3PlayFX(StaticSound[kSound41], nSprite);

								scorpion[nScorp].nFrame = 0;
								sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512);
								sprite[nSprite].yvel = Sin(sprite[nSprite].ang);

								scorpion[nScorp].nAction = 1;
								scorpion[nScorp].nTarget = nTarget;
							}
						}
					}

					return;
				}

				case 1:
				{
					scorpion[nScorp].i--;

					if (scorpion[nScorp].i <= 0)
					{
						scorpion[nScorp].i = RandomSize(5);
						// GOTO FS_Pink_A:
						goto FS_Pink_A;
					}
					else
					{
						int nMove = MoveCreatureWithCaution(nSprite);
						if ((nMove & 0xC000) == 0xC000)
						{
							if (nTarget == (nMove & 0x3FFF))
							{
								int nAngle = getangle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].y - sprite[nSprite].y);
								if (AngleDiff(sprite[nSprite].ang, nAngle) < 64)
								{
									scorpion[nScorp].nAction = 2;
									scorpion[nScorp].nFrame = 0;
								}

								// GOTO FS_Red
								goto FS_Red;
							}
							else
							{
//								GOTO FS_Pink_A
								goto FS_Pink_A;
							}
						}
						else if ((nMove & 0xC000) == 0x8000)
						{
							// GOTO FS_Pink_A
							goto FS_Pink_A;
						}
						else
						{
							// GOTO FS_Pink_B
							goto FS_Pink_B;
						}
					}
				}

				case 2:
				{
					if (nTarget == -1)
					{
						scorpion[nScorp].nAction = 0;
						scorpion[nScorp].g = 5;
					}
					else
					{
						if (PlotCourseToSprite(nSprite, nTarget) >= 768)
						{
							scorpion[nScorp].nAction = 1;
						}
						else if (nFlag & 0x80)
						{
							runlist_DamageEnemy(nTarget, nSprite, 7);
						}
					}

					// GOTO FS_Red
					goto FS_Red;
				}

				case 3:
				{
					if (edi)
					{
						scorpion[nScorp].h--;
						if (scorpion[nScorp].h <= 0)
						{
							scorpion[nScorp].nAction = 1;

							sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512);
							sprite[nSprite].yvel = Sin(sprite[nSprite].ang);

							scorpion[nScorp].nFrame = 0;
							return;
						}
					}

					if (!(nFlag & 0x80)) {
						return;
					}

					short nBulletSprite = BuildBullet(nSprite, 16, 0, 0, -1, sprite[nSprite].ang, nTarget + 10000, 1) & 0xFFFF;
					if (nBulletSprite > -1)
					{
						PlotCourseToSprite(nBulletSprite, nTarget);
					}

					return;
				}

				case 4:
				case 5:
				case 6:
				case 7:
				{
					if (!edi) {
						return;
					}

					if (scorpion[nScorp].nHealth > 0)
					{
						scorpion[nScorp].nAction = 1;
						scorpion[nScorp].nFrame = 0;
						scorpion[nScorp].g = 0;
						return;
					}

					scorpion[nScorp].g--;
					if (scorpion[nScorp].g <= 0)
					{
						scorpion[nScorp].nAction = 8;
					}
					else
					{
						scorpion[nScorp].nAction = RandomBit() + 6;
					}

					return;
				}

				case 8:
				{
					if (edi)
					{
						scorpion[nScorp].nAction++; // set to 9
						scorpion[nScorp].nFrame = 0;

						runlist_ChangeChannel(ScorpChan[nScorp], 1);
						return;
					}

					int nSpider = BuildSpider(-1, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum, sprite[nSprite].ang);
					if (nSpider != -1)
					{
						short nSpiderSprite = nSpider & 0xFFFF;

						sprite[nSpiderSprite].ang = RandomSize(11);

						int nRnd = RandomSize(5) + 1;

						sprite[nSpiderSprite].xvel = (Sin(sprite[nSpiderSprite].ang + 512) >> 8) * nRnd;
						sprite[nSpiderSprite].yvel = (Sin(sprite[nSpiderSprite].ang) >> 8) * nRnd;
						sprite[nSpiderSprite].zvel = (-(RandomSize(5) + 3)) << 8;
					}

					return;
				}

				case 9:
				{
					sprite[nSprite].cstat &= 0xFEFE;

					if (edi)
					{
						runlist_SubRunRec(scorpion[nScorp].f);
						runlist_DoSubRunRec(sprite[nSprite].owner);
						runlist_FreeRun(sprite[nSprite].lotag - 1);

						mydeletesprite(nSprite);
					}

					return;
				}
			}

			break;
		}
	}

FS_Pink_A:
	PlotCourseToSprite(nSprite, nTarget);
	sprite[nSprite].ang += RandomSize(7) - 63;
	sprite[nSprite].ang &= kAngleMask;

	sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512);
	sprite[nSprite].yvel = Sin(sprite[nSprite].ang);

FS_Pink_B:
	if (scorpion[nScorp].g)
	{
		scorpion[nScorp].g--;
	}
	else
	{
		scorpion[nScorp].g = 45;

		if (cansee(sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z - GetSpriteHeight(nSprite), sprite[nSprite].sectnum,
			sprite[nTarget].x, sprite[nTarget].y, sprite[nTarget].z - GetSpriteHeight(nTarget), sprite[nTarget].sectnum))
		{
			sprite[nSprite].yvel = 0;
			sprite[nSprite].xvel = 0;
			sprite[nSprite].ang = GetMyAngle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].y - sprite[nSprite].y);
			
			scorpion[nScorp].h = RandomSize(2) + RandomSize(3);

			if (!scorpion[nScorp].h) {
				scorpion[nScorp].g = RandomSize(5);
			}
			else
			{
				scorpion[nScorp].nAction = 3;
				scorpion[nScorp].nFrame = 0;
			}
		}
	}

FS_Red:
	if (!nAction || nTarget == -1) {
		return;
	}

	if (!(sprite[nTarget].cstat & 0x101))
	{
		scorpion[nScorp].nAction = 0;
		scorpion[nScorp].nFrame = 0;
		scorpion[nScorp].g = 30;
		scorpion[nScorp].nTarget = -1;

		sprite[nSprite].xvel = 0;
		sprite[nSprite].yvel = 0;
	}
}
