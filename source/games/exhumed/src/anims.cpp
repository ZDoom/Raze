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
        StopSpriteSound(nSprite);
        runlist_SubRunRec(AnimList[nAnim].AnimRunRec);
        runlist_DoSubRunRec(sprite[nSprite].extra);
        runlist_FreeRun(sprite[nSprite].lotag - 1);
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
    sprite[nSprite].backuppos();

    // CHECKME - where is hitag set otherwise?
    if (sprite[nSprite].statnum < 900) {
        sprite[nSprite].hitag = -1;
    }

    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].owner = -1;
    sprite[nSprite].extra = runlist_AddRunRec(sprite[nSprite].lotag - 1, nAnim | 0x100000);

    AnimList[nAnim].AnimRunRec = runlist_AddRunRec(NewRun, nAnim | 0x100000);
    AnimList[nAnim].nSprite = nSprite;
    AnimList[nAnim].AnimFlags = nFlag;
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
                short nSpriteB = sprite[nSprite].hitag;
                if (nSpriteB > -1)
                {
                    sprite[nSprite].x = sprite[nSpriteB].x;
                    sprite[nSprite].y = sprite[nSpriteB].y;
                    sprite[nSprite].z = sprite[nSpriteB].z;

                    if (sprite[nSpriteB].sectnum != sprite[nSprite].sectnum)
                    {
                        if (sprite[nSpriteB].sectnum < 0 || sprite[nSpriteB].sectnum >= kMaxSectors)
                        {
                            DestroyAnim(nAnim);
                            mydeletesprite(nSprite);
                            return;
                        }
                        else
                        {
                            mychangespritesect(nSprite, sprite[nSpriteB].sectnum);
                        }
                    }

                    if (!var_1C)
                    {
                        if (sprite[nSpriteB].cstat != 0x8000)
                        {
                            short hitag2 = sprite[nSpriteB].hitag;
                            sprite[nSpriteB].hitag--;

                            if (hitag2 >= 15)
                            {
                                runlist_DamageEnemy(nSpriteB, -1, (sprite[nSpriteB].hitag - 14) * 2);

                                if (sprite[nSpriteB].shade < 100)
                                {
                                    sprite[nSpriteB].pal = 0;
                                    sprite[nSpriteB].shade++;
                                }

                                if (!(sprite[nSpriteB].cstat & 101))
                                {
                                    DestroyAnim(nAnim);
                                    mydeletesprite(nSprite);
                                    return;
                                }
                            }
                            else
                            {
                                sprite[nSpriteB].hitag = 1;
                                DestroyAnim(nAnim);
                                mydeletesprite(nSprite);
                            }
                        }
                        else
                        {
                            sprite[nSpriteB].hitag = 1;
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
            Printf("unknown msg %x for anim\n", a & 0x7F0000);
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
        D3PlayFX(StaticSound[nSound] | 0xa00, AnimList[nAnim].nSprite);
    }

    return AnimList[nAnim].nSprite;
}
END_PS_NS
