//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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
#include "ns.h"
#include "engine.h"
#include "aistuff.h"
#include "sequence.h"
#include "exhumed.h"
#include "sound.h"
#include <assert.h>

BEGIN_PS_NS

short nMagicSeq = -1;
short nPreMagicSeq  = -1;
short nSavePointSeq = -1;
FreeListArray<Anim, kMaxAnims> AnimList;


FSerializer& Serialize(FSerializer& arc, const char* keyname, Anim& w, Anim* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("seq", w.nSeq)
            ("val1", w.field_2)
            ("val2", w.field_4)
            ("sprite", w.nSprite)
            ("runrec", w.AnimRunRec)
            ("flags", w.AnimFlags)
            .EndObject();
    }
    return arc;
}

void SerializeAnim(FSerializer& arc)
{
    if (arc.BeginObject("anims"))
    {
        arc("magic", nMagicSeq)
            ("premagic", nPreMagicSeq)
            ("savepoint", nSavePointSeq)
            ("list", AnimList)
            .EndObject();
    }
}

void InitAnims()
{
    AnimList.Clear();
    nMagicSeq     = SeqOffsets[kSeqItems] + 21;
    nPreMagicSeq  = SeqOffsets[kSeqMagic2];
    nSavePointSeq = SeqOffsets[kSeqItems] + 12;
}

void DestroyAnim(int nAnim)
{
    short nSprite = AnimList[nAnim].nSprite;

    if (nSprite >= 0)
    {
		auto pSprite = &sprite[nSprite];
        StopSpriteSound(nSprite);
        runlist_SubRunRec(AnimList[nAnim].AnimRunRec);
        runlist_DoSubRunRec(pSprite->extra);
        runlist_FreeRun(pSprite->lotag - 1);
    }

    AnimList.Release(nAnim);
}

int BuildAnim(int nSprite, int val, int val2, int x, int y, int z, int nSector, int nRepeat, int nFlag)
{
    int nAnim = AnimList.Get();
	if (nAnim < 0) {
		return -1;
	}

    if (nSprite == -1) {
        nSprite = insertsprite(nSector, 500);
    }
	auto pSprite = &sprite[nSprite];

    pSprite->x = x;
    pSprite->y = y;
    pSprite->z = z;
    pSprite->cstat = 0;

    if (nFlag & 4)
    {
        pSprite->pal = 4;
        pSprite->shade = -64;
    }
    else
    {
        pSprite->pal = 0;
        pSprite->shade = -12;
    }

    pSprite->clipdist = 10;
    pSprite->xrepeat = nRepeat;
    pSprite->yrepeat = nRepeat;
    pSprite->picnum = 1;
    pSprite->ang = 0;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->backuppos();

    // CHECKME - where is hitag set otherwise?
    if (pSprite->statnum < 900) {
        pSprite->hitag = -1;
    }

    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->owner = -1;
    pSprite->extra = runlist_AddRunRec(pSprite->lotag - 1, nAnim, 0x100000);

    AnimList[nAnim].AnimRunRec = runlist_AddRunRec(NewRun, nAnim, 0x100000);
    AnimList[nAnim].nSprite = nSprite;
    AnimList[nAnim].AnimFlags = nFlag;
    AnimList[nAnim].field_2 = 0;
    AnimList[nAnim].nSeq = SeqOffsets[val] + val2;
    AnimList[nAnim].field_4 = 256;

    if (nFlag & 0x80) {
        pSprite->cstat |= 0x2; // set transluscence
    }

    return nAnim;
}

short GetAnimSprite(short nAnim)
{
    return AnimList[nAnim].nSprite;
}

void AIAnim::Tick(RunListEvent* ev)
{
    short nAnim = RunData[ev->nRun].nObjIndex;
    assert(nAnim >= 0 && nAnim < kMaxAnims);

    short nSprite = AnimList[nAnim].nSprite;
    short nSeq = AnimList[nAnim].nSeq;
    auto pSprite = &sprite[nSprite];

    assert(nSprite != -1);

    short var_1C = AnimList[nAnim].field_2;

    if (!(pSprite->cstat & 0x8000))
    {
        seq_MoveSequence(nSprite, nSeq, var_1C);
    }

    if (pSprite->statnum == kStatIgnited)
    {
        short nSpriteB = pSprite->hitag;
        if (nSpriteB > -1)
        {
            auto pSpriteB = &sprite[nSpriteB];
            pSprite->x = pSpriteB->x;
            pSprite->y = pSpriteB->y;
            pSprite->z = pSpriteB->z;

            if (pSpriteB->sectnum != pSprite->sectnum)
            {
                if (pSpriteB->sectnum < 0 || pSpriteB->sectnum >= kMaxSectors)
                {
                    DestroyAnim(nAnim);
                    mydeletesprite(nSprite);
                    return;
                }
                else
                {
                    mychangespritesect(nSprite, pSpriteB->sectnum);
                }
            }

            if (!var_1C)
            {
                if (pSpriteB->cstat != 0x8000)
                {
                    short hitag2 = pSpriteB->hitag;
                    pSpriteB->hitag--;

                    if (hitag2 >= 15)
                    {
                        runlist_DamageEnemy(nSpriteB, -1, (pSpriteB->hitag - 14) * 2);

                        if (pSpriteB->shade < 100)
                        {
                            pSpriteB->pal = 0;
                            pSpriteB->shade++;
                        }

                        if (!(pSpriteB->cstat & 101))
                        {
                            DestroyAnim(nAnim);
                            mydeletesprite(nSprite);
                            return;
                        }
                    }
                    else
                    {
                        pSpriteB->hitag = 1;
                        DestroyAnim(nAnim);
                        mydeletesprite(nSprite);
                    }
                }
                else
                {
                    pSpriteB->hitag = 1;
                    DestroyAnim(nAnim);
                    mydeletesprite(nSprite);
                }
            }
        }
    }

    AnimList[nAnim].field_2++;
    if (AnimList[nAnim].field_2 >= SeqSize[nSeq])
    {
        if (AnimList[nAnim].AnimFlags & 0x10)
        {
            AnimList[nAnim].field_2 = 0;
        }
        else if (nSeq == nPreMagicSeq)
        {
            AnimList[nAnim].field_2 = 0;
            AnimList[nAnim].nSeq = nMagicSeq;
            short nAnimSprite = AnimList[nAnim].nSprite;
            AnimList[nAnim].AnimFlags |= 0x10;
            sprite[nAnimSprite].cstat |= 2;
        }
        else if (nSeq == nSavePointSeq)
        {
            AnimList[nAnim].field_2 = 0;
            AnimList[nAnim].nSeq++;
            AnimList[nAnim].AnimFlags |= 0x10;
        }
        else
        {
            DestroyAnim(nAnim);
            mydeletesprite(nSprite);
        }
    }
}

void AIAnim::Draw(RunListEvent* ev)
{
    short nAnim = RunData[ev->nRun].nObjIndex;
    assert(nAnim >= 0 && nAnim < kMaxAnims);
    short nSeq = AnimList[nAnim].nSeq;

    seq_PlotSequence(ev->nParam, nSeq, AnimList[nAnim].field_2, 0x101);
    ev->pTSprite->owner = -1;
}

void  FuncAnim(int nObject, int nMessage, int nDamage, int nRun)
{
    AIAnim ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void BuildExplosion(short nSprite)
{
    auto pSprite = &sprite[nSprite];
 
    short nSector = pSprite->sectnum;

    int edx = 36;

    if (SectFlag[nSector] & kSectUnderwater)
    {
        edx = 75;
    }
    else if (pSprite->z == sector[nSector].floorz)
    {
        edx = 34;
    }

    BuildAnim(-1, edx, 0, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, pSprite->xrepeat, 4);
}

int BuildSplash(int nSprite, int nSector)
{
    auto pSprite = &sprite[nSprite];
    int nRepeat, nSound;

    if (pSprite->statnum != 200)
    {
        nRepeat = pSprite->xrepeat + (RandomWord() % pSprite->xrepeat);
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

    int nAnim = BuildAnim(-1, edx, 0, pSprite->x, pSprite->y, sector[nSector].floorz, nSector, nRepeat, nFlag);

    if (!bIsLava)
    {
        D3PlayFX(StaticSound[nSound] | 0xa00, AnimList[nAnim].nSprite);
    }

    return AnimList[nAnim].nSprite;
}
END_PS_NS
