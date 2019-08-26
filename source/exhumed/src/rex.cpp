
#include "rex.h"
#include "exhumed.h"
#include "engine.h"
#include "runlist.h"
#include "move.h"
#include "sequence.h"
#include "sound.h"
#include "random.h"
#include "trigdat.h"
#include "player.h"
#include "aistuff.h"
#include <assert.h>

#define kMaxRex	50

short RexCount = 0;
short RexChan[kMaxRex];

struct Rex
{
	short nHealth;
	short field_2;
	short nAction;
	short nSprite;
	short nTarget;
	short field_A;
};

Rex RexList[kMaxRex];

static actionSeq ActionSeq[] = {
	{29, 0},
	{0,  0},
	{0,  0},
	{37, 0},
	{9,  0},
	{18, 0},
	{27, 1},
	{28, 1}
};


void InitRexs()
{
	RexCount = kMaxRex;
}

int BuildRex(short nSprite, int x, int y, int z, short nSector, short nAngle, int nVal)
{
	RexCount--;

	int nRex = RexCount;
	if (nRex < 0) {
		return -1;
	}

	if (nSprite == -1)
	{
		nSprite = insertsprite(nSector, 119);
	}
	else
	{
		changespritestat(nSprite, 119);
		x = sprite[nSprite].x;
		y = sprite[nSprite].y;
		z = sector[sprite[nSprite].sectnum].floorz;
		nAngle = sprite[nSprite].ang;
	}

	assert(nSprite >= 0 && nSprite < kMaxSprites);

	sprite[nSprite].x = x;
	sprite[nSprite].y = y;
	sprite[nSprite].cstat = 0x101;
	sprite[nSprite].z = z;
	sprite[nSprite].cstat = 0x101;
	sprite[nSprite].shade = -12;
	sprite[nSprite].clipdist = 80;
	sprite[nSprite].xvel = 0;
	sprite[nSprite].yvel = 0;
	sprite[nSprite].zvel = 0;
	sprite[nSprite].xrepeat = 64;
	sprite[nSprite].yrepeat = 64;
	sprite[nSprite].pal = sector[sprite[nSprite].sectnum].ceilingpal;
	sprite[nSprite].xoffset = 0;
	sprite[nSprite].yoffset = 0;
	sprite[nSprite].ang = nAngle;
	sprite[nSprite].picnum = 1;
	sprite[nSprite].hitag = 0;
	sprite[nSprite].lotag = runlist_HeadRun() + 1;
	sprite[nSprite].extra = -1;

	GrabTimeSlot(3);

	RexList[nRex].nAction = 0;
	RexList[nRex].nHealth = 4000;
	RexList[nRex].field_2 = 0;
	RexList[nRex].nSprite = nSprite;
	RexList[nRex].nTarget = -1;
	RexList[nRex].field_A = 0;
	RexChan[nRex] = nVal;

	sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nRex | 0x180000);

	// this isn't stored anywhere...
	runlist_AddRunRec(NewRun, nRex | 0x180000);

	nCreaturesLeft++;

	return nRex | 0x180000;
}

void FuncRex(int a, int nDamage, int nRun)
{
	short nRex = RunData[nRun].nVal;
	assert(nRex >= 0 && nRex < kMaxRex);

	int var_1C = 0;

	short nAction = RexList[nRex].nAction;
	short nSprite = RexList[nRex].nSprite;

	int nMessage = a & 0x7F0000;

	switch (nMessage)
	{
		default:
		{
			DebugOut("unknown msg %d for Rex\n", a & 0x7F0000);
			return;
		}

		case 0xA0000:
		{
			if (nAction == 5)
			{
				nDamage = runlist_CheckRadialDamage(nSprite);
			}
			// fall through to case 0x80000
		}

		case 0x80000:
		{
			if (nDamage)
			{
				short nTarget = a & 0xFFFF;
				if (nTarget >= 0 && sprite[nTarget].statnum == 100)
				{
					RexList[nRex].nTarget = nTarget;
				}

				if (RexList[nRex].nAction == 5 && RexList[nRex].nHealth > 0)
				{
					RexList[nRex].nHealth -= nDamage;

					if (RexList[nRex].nHealth <= 0)
					{
						sprite[nSprite].zvel = 0;
						sprite[nSprite].yvel = 0;
						sprite[nSprite].xvel = 0;
						RexList[nRex].nHealth = 0;
						sprite[nSprite].cstat &= 0xFEFE;
						nCreaturesLeft--;

						if (nAction < 6)
						{
							RexList[nRex].nAction = 6;
							RexList[nRex].field_2 = 0;
						}
					}
				}
			}
			return;
		}

		case 0x90000:
		{
			seq_PlotSequence(a & 0xFFFF, SeqOffsets[kSeqRex] + ActionSeq[nAction].a, RexList[nRex].field_2, ActionSeq[nAction].b);
			return;
		}

		case 0x20000:
		{
			Gravity(nSprite);

			int nSeq = SeqOffsets[kSeqRex] + ActionSeq[nAction].a;

			sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, RexList[nRex].field_2);

			int ecx;

			if (nAction != 2) {
				ecx = 1;
			}
			else {
				ecx = 2;
			}

			// moves the mouth open and closed as it's idle?
			do
			{
				ecx--;

				seq_MoveSequence(nSprite, nSeq, RexList[nRex].field_2);

				RexList[nRex].field_2++;
				if (RexList[nRex].field_2 >= SeqSize[nSeq])
				{
					RexList[nRex].field_2 = 0;
					var_1C = 1;
				}
			} while (ecx != -1);

			int nFlag = FrameFlag[SeqBase[nSeq] + RexList[nRex].field_2];

			short nTarget = RexList[nRex].nTarget;

			if (nAction > 7) {
				return;
			}

			switch (nAction)
			{
				default:
					return;

				// OK
				case 0:
				{
					if (!RexList[nRex].field_A)
					{
						if ((nRex & 0x1F) == (totalmoves & 0x1F))
						{
							if (nTarget < 0)
							{
								short nAngle = sprite[nSprite].ang; // make backup of this variable
								RexList[nRex].nTarget = FindPlayer(nSprite, 60);
								sprite[nSprite].ang = nAngle;
							}
							else
							{
								RexList[nRex].field_A = 60;
							}
						}
					}
					else
					{
						RexList[nRex].field_A--;
						if (RexList[nRex].field_A <= 0)
						{
							RexList[nRex].nAction = 1;
							RexList[nRex].field_2 = 0;

							sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512) >> 2;
							sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 2;

							D3PlayFX(StaticSound[kSound48], nSprite);

							RexList[nRex].field_A = 30;
						}
					}

					return;
				}

				// OK
				case 1:
				{
					if (RexList[nRex].field_A > 0)
					{
						RexList[nRex].field_A--;
					}

					if ((nRex & 0x0F) == (totalmoves & 0x0F))
					{
						if (!RandomSize(1))
						{
							RexList[nRex].nAction = 5;
							RexList[nRex].field_2 = 0;
							sprite[nSprite].yvel = 0;
							sprite[nSprite].xvel = 0;
							return;
						}
						else
						{
							if (((PlotCourseToSprite(nSprite, nTarget) >> 8) >= 60) || RexList[nRex].field_A > 0)
							{
								sprite[nSprite].xvel = Sin((sprite[nSprite].ang & 0xFFF8) + 512) >> 2;
								sprite[nSprite].yvel = Sin((sprite[nSprite].ang & 0xFFF8)) >> 2;
							}
							else
							{
								RexList[nRex].nAction = 2;
								RexList[nRex].field_A = 240;
								D3PlayFX(StaticSound[kSound48], nSprite);
								RexList[nRex].field_2 = 0;
								return;
							}
						}
					}

					int nVal = MoveCreatureWithCaution(nSprite);

					if ((nVal & 0xC000) == 0x8000)
					{
						sprite[nSprite].ang = (sprite[nSprite].ang + 256) & kAngleMask;
						sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512) >> 2;
						sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 2;
						RexList[nRex].nAction = 1;
						RexList[nRex].field_2 = 0;
						nAction = 1;
						break;
					}
					else if ((nVal & 0xC000) == 0xC000)
					{
						if ((nVal & 0x3FFF) == nTarget)
						{
							PlotCourseToSprite(nSprite, nTarget);
							RexList[nRex].nAction = 4;
							RexList[nRex].field_2 = 0;
							break;
						}
						else
						{
							sprite[nSprite].ang = (sprite[nSprite].ang + 256) & kAngleMask;
							sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512) >> 2;
							sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 2;
							RexList[nRex].nAction = 1;
							RexList[nRex].field_2 = 0;
							nAction = 1;
							break;
						}
					}

					break;
				}

				case 2:
				{
					RexList[nRex].field_A--;
					if (RexList[nRex].field_A > 0)
					{
						PlotCourseToSprite(nSprite, nTarget);

						sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512) >> 1;
						sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 1;

						int nVal = MoveCreatureWithCaution(nSprite);

						if ((nVal & 0x0C000) == 0x8000)
						{
							SetQuake(nSprite, 25);
							RexList[nRex].field_A = 60;

							sprite[nSprite].ang = (sprite[nSprite].ang + 256) & kAngleMask;
							sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512) >> 2;
							sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 2;
							RexList[nRex].nAction = 1;
							RexList[nRex].field_2 = 0;
							nAction = 1;
							break;
						}
						else if ((nVal & 0x0C000) == 0x0C000)
						{
							RexList[nRex].nAction = 3;
							RexList[nRex].field_2 = 0;

							short nSprite2 = nVal & 0x3FFF;

							if (sprite[nSprite2].statnum && sprite[nSprite2].statnum < 107)
							{
								short nAngle = sprite[nSprite].ang;

								runlist_DamageEnemy(nSprite2, nSprite, 15);

								int ebx = Sin(nAngle + 512) * 15;
								int edx = Sin(nAngle) * 15;

								if (sprite[nSprite2].statnum == 100)
								{
									short nPlayer = GetPlayerFromSprite(nSprite2);
									nXDamage[nPlayer] += (ebx << 4);
									nYDamage[nPlayer] += (edx << 4);
									sprite[nSprite2].zvel = -3584;
								}
								else
								{
									sprite[nSprite2].xvel += (ebx >> 3);
									sprite[nSprite2].yvel += (edx >> 3);
									sprite[nSprite2].zvel = -2880;
								}
							}

							RexList[nRex].field_A >>= 2;
						}
					}
					else
					{
						RexList[nRex].nAction = 1;
						RexList[nRex].field_2 = 0;
						RexList[nRex].field_A = 90;
					}

					return;
				}

				case 3:
				{
					if (var_1C)
					{
						RexList[nRex].nAction = 2;
					}
					return;
				}

				case 4:
				{
					if (nTarget != -1)
					{
						if (PlotCourseToSprite(nSprite, nTarget) < 768)
						{
							if (nFlag & 0x80)
							{
								runlist_DamageEnemy(nTarget, nSprite, 15);
							}

							break;
						}
					}
	
					RexList[nRex].nAction = 1;
					break;
				}

				case 5:
				{
					if (var_1C)
					{
						RexList[nRex].nAction = 1;
						RexList[nRex].field_A = 15;
					}
					return;
				}

				case 6:
				{
					if (var_1C)
					{
						RexList[nRex].nAction = 7;
						RexList[nRex].field_2 = 0;
						runlist_ChangeChannel(RexChan[nRex], 1);
					}
					return;
				}

				case 7:
				{
					sprite[nSprite].cstat &= 0xFEFE;
					return;
				}
			}

			// break-ed
			if (nAction > 0)
			{
				if ((nTarget != -1) && (!(sprite[nTarget].cstat & 0x101)))
				{
					RexList[nRex].nAction = 0;
					RexList[nRex].field_2 = 0;
					RexList[nRex].field_A = 0;
					RexList[nRex].nTarget = -1;
					sprite[nSprite].yvel = 0;
					sprite[nSprite].xvel = 0;
				}
			}
			return;
		}
	}
}
