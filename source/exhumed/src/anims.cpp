
#include "engine.h"
#include "anims.h"
#include "sequence.h"
#include "runlist.h"
#include "exhumed.h"
#include "sound.h"
#include "random.h"
#include "init.h"
#include <assert.h>

#define kMaxAnims	400

short nMagicSeq = -1;
short nPreMagicSeq  = -1;
short nSavePointSeq = -1;
short nAnimsFree = 0;

short AnimRunRec[kMaxAnims];
short AnimsFree[kMaxAnims];
Anim AnimList[kMaxAnims];
uint8_t AnimFlags[kMaxAnims];


void InitAnims()
{
    for (int i = 0; i < kMaxAnims; i++) {
        AnimsFree[i] = i;
    }

    nAnimsFree = kMaxAnims;

    nMagicSeq     = SeqOffsets[kSeqItems] + 21;
    nPreMagicSeq  = SeqOffsets[kSeqMagic2];
    nSavePointSeq = SeqOffsets[kSeqItems] + 12;
}

void DestroyAnim(int nAnim)
{
    short nSprite = AnimList[nAnim].nSprite;

    if (nSprite >= 0)
    {
        StopSpriteSound(nSprite);
        runlist_SubRunRec(AnimRunRec[nAnim]);
        runlist_DoSubRunRec(sprite[nSprite].extra);
        runlist_FreeRun(sprite[nSprite].lotag - 1);
    }

    AnimsFree[nAnimsFree] = nAnim;
    nAnimsFree++;
}

int BuildAnim(int nSprite, int val, int val2, int x, int y, int z, int nSector, int nRepeat, int nFlag)
{
	if (!nAnimsFree) {
		return -1;
	}

    nAnimsFree--;

    short nAnim = AnimsFree[nAnimsFree];

    if (nSprite == -1) {
        nSprite = insertsprite(nSector, 500);
    }

    sprite[nSprite].x = x;
    sprite[nSprite].y = y;
    sprite[nSprite].z = z;
    sprite[nSprite].cstat = 0;

    if (nFlag & 4)
    {
        sprite[nSprite].pal = 4;
        sprite[nSprite].shade = -64;
    }
    else
    {
        sprite[nSprite].pal = 0;
        sprite[nSprite].shade = -12;
    }

    sprite[nSprite].clipdist = 10;
    sprite[nSprite].xrepeat = nRepeat;
    sprite[nSprite].yrepeat = nRepeat;
    sprite[nSprite].picnum = 1;
    sprite[nSprite].ang = 0;
    sprite[nSprite].xoffset = 0;
    sprite[nSprite].yoffset = 0;
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = 0;

    // CHECKME - where is hitag set otherwise?
    if (sprite[nSprite].statnum < 900) {
        sprite[nSprite].hitag = -1;
    }

    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].owner = -1;
    sprite[nSprite].extra = runlist_AddRunRec(sprite[nSprite].lotag - 1, nAnim | 0x100000);

    AnimRunRec[nAnim] = runlist_AddRunRec(NewRun, nAnim | 0x100000);
    AnimList[nAnim].nSprite = nSprite;
    AnimFlags[nAnim] = nFlag;
    AnimList[nAnim].field_2 = 0;
    AnimList[nAnim].nSeq = SeqOffsets[val] + val2;
    AnimList[nAnim].field_4 = 256;
    
    if (nFlag & 0x80) {
        sprite[nSprite].cstat |= 0x2; // set transluscence
    }

    return nAnim;
}

short GetAnimSprite(short nAnim)
{
    return AnimList[nAnim].nSprite;
}

void FuncAnim(int a, int, int nRun)
{
    short nAnim = RunData[nRun].nVal;
    assert(nAnim >= 0 && nAnim < kMaxAnims);

    short nSprite = AnimList[nAnim].nSprite;
    short nSeq = AnimList[nAnim].nSeq;

    assert(nSprite != -1);

    int nMessage = a & 0x7F0000;

    switch (nMessage)
    {
        case 0x20000:
        {
            short var_1C = AnimList[nAnim].field_2;

            if (!(sprite[nSprite].cstat & 0x8000))
            {
                seq_MoveSequence(nSprite, nSeq, var_1C);
            }

            if (sprite[nSprite].statnum == kStatIgnited)
            {
                short nHitag = sprite[nSprite].hitag;
                if (nHitag > -1)
                {
                    sprite[nSprite].x = sprite[nHitag].x;
                    sprite[nSprite].y = sprite[nHitag].y;
                    sprite[nSprite].z = sprite[nHitag].z;

                    if (sprite[nHitag].sectnum != sprite[nSprite].sectnum)
                    {
                        if (sprite[nHitag].sectnum < 0 || sprite[nHitag].sectnum >= kMaxSectors)
                        {
                            DestroyAnim(nAnim);
                            mydeletesprite(nSprite);
                            return;
                        }
                        else
                        {
                            mychangespritesect(nSprite, sprite[nHitag].sectnum);
                        }
                    }

                    if (!var_1C)
                    {
                        if (sprite[nHitag].cstat != 0x8000)
                        {
                            short hitag2 = sprite[nHitag].hitag;
                            sprite[nHitag].hitag--;

                            if (hitag2 >= 15)
                            {
                                runlist_DamageEnemy(nHitag, -1, (sprite[nHitag].hitag - 14) * 2);
                                if (sprite[nHitag].shade < 100)
                                {
                                    sprite[nHitag].pal = 0;
                                    sprite[nHitag].shade++;
                                }

                                if (!(sprite[nHitag].cstat & 101))
                                {
                                    DestroyAnim(nAnim);
                                    mydeletesprite(nSprite);
                                    return;
                                }

                                goto loc_2D755;
                            }
                        }

                        sprite[nHitag].hitag = 1;
                        DestroyAnim(nAnim);
                        mydeletesprite(nSprite);
                    }
                }
            }

            // loc_2D755
loc_2D755:

            AnimList[nAnim].field_2++;
            if (AnimList[nAnim].field_2 >= SeqSize[nSeq])
            {
                if (AnimFlags[nAnim] & 0x10)
                {
                    AnimList[nAnim].field_2 = 0;
                }
                else if (nSeq == nPreMagicSeq)
                {
                    AnimList[nAnim].field_2 = 0;
                    AnimList[nAnim].nSeq = nMagicSeq;
                    short nAnimSprite = AnimList[nAnim].nSprite;
                    AnimFlags[nAnim] |= 0x10;
                    sprite[nAnimSprite].cstat |= 2;
                }
                else if (nSeq == nSavePointSeq)
                {
                    AnimList[nAnim].field_2 = 0;
                    AnimList[nAnim].nSeq++;
                    AnimFlags[nAnim] |= 0x10;
                }
                else
                {
                    DestroyAnim(nAnim);
                    mydeletesprite(nSprite);
                }
                return;
            }

            return;
        }

        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, nSeq, AnimList[nAnim].field_2, 0x101);
            tsprite[a & 0xFFFF].owner = -1;
            return;
        }

        case 0xA0000:
        {
            return;
        }

        default:
        {
            DebugOut("unknown msg %x for anim\n", a & 0x7F0000);
            return;
        }
    }
}

void BuildExplosion(short nSprite)
{
    short nSector = sprite[nSprite].sectnum;

    int edx = 36;

    if (SectFlag[nSector] & kSectUnderwater)
    {
        edx = 75;
    }
    else if (sprite[nSprite].z == sector[nSector].floorz)
    {
        edx = 34;
    }

    BuildAnim(-1, edx, 0, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum, sprite[nSprite].xrepeat, 4);
}

int BuildSplash(int nSprite, int nSector)
{
    int nRepeat, nSound;

    if (sprite[nSprite].statnum != 200)
    {
        nRepeat = sprite[nSprite].xrepeat + (RandomWord() % sprite[nSprite].xrepeat);
        nSound = kSound0;
    }
    else
    {
        nRepeat = 20;
        nSound = kSound1;
    }

    int bIsLava = SectFlag[nSector] & kSectLava;

    int edx, nFlag;

    if (bIsLava)
    {
        edx = 43;
        nFlag = 4;
    }
    else
    {
        edx = 35;
        nFlag = 0;
    }

    int nAnim = BuildAnim(-1, edx, 0, sprite[nSprite].x, sprite[nSprite].y, sector[nSector].floorz, nSector, nRepeat, nFlag);

    if (!bIsLava)
    {
        D3PlayFX(StaticSound[nSound] | 10, AnimList[nAnim].nSprite);
    }

    return AnimList[nAnim].nSprite;
}
