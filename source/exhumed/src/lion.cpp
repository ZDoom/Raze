
#include "lion.h"
#include "engine.h"
#include "runlist.h"
#include "exhumed.h"
#include "sequence.h"
#include "move.h"
#include "sound.h"
#include "random.h"
#include "trigdat.h"
#include "items.h"
#include <assert.h>

#define kMaxLions	40

short LionCount = -1;

short MoveHook[kMaxLions];

static actionSeq ActionSeq[] = {{54, 1}, {18, 1}, {0, 0}, {10, 0}, {44, 0}, {18, 0}, {26, 0}, {34, 0}, {8, 1}, {9, 1}, {52, 1}, {53, 1}};

struct Lion
{
    short nHealth;
    short _b;
    short nAction;
    short nSprite;
    short nTarget;
    short _f;
    short _g;
    short _h;
};

Lion LionList[kMaxLions];


void InitLion()
{
    LionCount = kMaxLions;
}

int BuildLion(short nSprite, int x, int y, int z, short nSector, short nAngle)
{
    LionCount--;
    short nLion = LionCount;

    if (LionCount < 0) {
        return -1;
    }

    if (nSprite == -1)
    {
        nSprite = insertsprite(nSector, 104);
    }
    else
    {
        changespritestat(nSprite, 104);
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
    sprite[nSprite].clipdist = 60;
    sprite[nSprite].shade = -12;
    sprite[nSprite].xrepeat = 40;
    sprite[nSprite].yrepeat = 40;
    sprite[nSprite].picnum = 1;
    sprite[nSprite].pal = sector[sprite[nSprite].sectnum].ceilingpal;
    sprite[nSprite].xoffset = 0;
    sprite[nSprite].yoffset = 0;
    sprite[nSprite].ang = nAngle;
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = 0;
    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].hitag = 0;
    sprite[nSprite].extra = -1;

//	GrabTimeSlot(3);

    LionList[nLion].nAction = 0;
    LionList[nLion].nHealth = 500;
    LionList[nLion]._b = 0;
    LionList[nLion].nSprite = nSprite;
    LionList[nLion].nTarget = -1;
    LionList[nLion]._g = 0;
    LionList[nLion]._f = nLion;

    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nLion | 0x130000);

    MoveHook[nLion] = runlist_AddRunRec(NewRun, nLion | 0x130000);

    nCreaturesLeft++;

    return nLion | 0x130000;
}

void FuncLion(int a, int nDamage, int nRun)
{
    int var_18 = 0;

    short nLion = RunData[nRun].nVal;
    assert(nLion >= 0 && nLion < kMaxLions);

    short nSprite = LionList[nLion].nSprite;
    short nAction = LionList[nLion].nAction;

    int nMessage = a & 0x7F0000;

    switch (nMessage)
    {
        default:
        {
            DebugOut("unknown msg %d for Lion\n", nMessage);
            return;
        }

        case 0x90000:
        {
            seq_PlotSequence(a, SeqOffsets[kSeqLion] + ActionSeq[nAction].a, LionList[nLion]._b, ActionSeq[nAction].b);
            return;
        }

        case 0xA0000:
        {
            nDamage = runlist_CheckRadialDamage(nSprite);
            // now fall through to 0x80000
        }
        case 0x80000:
        {
            if (nDamage && LionList[nLion].nHealth > 0)
            {
                LionList[nLion].nHealth -= nDamage;
                if (LionList[nLion].nHealth <= 0)
                {
                    // R.I.P.
                    sprite[nSprite].xvel = 0;
                    sprite[nSprite].yvel = 0;
                    sprite[nSprite].zvel = 0;
                    LionList[nLion].nHealth = 0;
                    sprite[nSprite].cstat &= 0xFEFE;

                    nCreaturesLeft--;

                    if (nAction < 10)
                    {
                        DropMagic(nSprite);

                        if (nMessage == 0xA0000) {
                            LionList[nLion].nAction = 11;
                        }
                        else
                        {
                            LionList[nLion].nAction = 10;
                        }

                        LionList[nLion]._b = 0;
                        return;
                    }
                }
                else
                {
                    if ((a & 0xFFFF) >= 0)
                    {
                        short nTarget = a & 0xFFFF;

                        if (sprite[nTarget].statnum < 199) {
                            LionList[nLion].nTarget = nTarget;
                        }

                        if (nAction != 6)
                        {
                            if (RandomSize(8) <= (LionList[nLion].nHealth >> 2))
                            {
                                LionList[nLion].nAction = 4;
                                sprite[nSprite].xvel = 0;
                                sprite[nSprite].yvel = 0;
                            }
                            else if (RandomSize(1))
                            {
                                PlotCourseToSprite(nSprite, nTarget);
                                LionList[nLion].nAction = 5;
                                LionList[nLion]._g = RandomSize(3);

                                sprite[nSprite].ang = (sprite[nSprite].ang - (RandomSize(1) << 8)) + (RandomSize(1) << 8);
                            }
                            else
                            {
                                sprite[nSprite].xvel = 0;
                                sprite[nSprite].yvel = 0;
                                LionList[nLion].nAction = 8;
                                sprite[nSprite].cstat &= 0xFEFE;
                            }

                            LionList[nLion]._b = 0;
                        }
                    }
                }
            }
            return;
        }

        case 0x20000:
        {
            if (nAction != 7) {
                Gravity(nSprite);
            }

            short nSeq = SeqOffsets[kSeqLion] + ActionSeq[nAction].a;

            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, LionList[nLion]._b);

            seq_MoveSequence(nSprite, nSeq, LionList[nLion]._b);

            LionList[nLion]._b++;
            if (LionList[nLion]._b >= SeqSize[nSeq])
            {
                LionList[nLion]._b = 0;
                var_18 = 1;
            }

            short nFlag = FrameFlag[SeqBase[nSeq] + LionList[nLion]._b];
            short nTarget = LionList[nLion].nTarget;

            int nVal = MoveCreatureWithCaution(nSprite);

            switch (nAction)
            {
                default:
                    return;

                case 0:
                case 1:
                {
                    if ((LionList[nLion]._f & 31) == (totalmoves & 31))
                    {
                        if (nTarget < 0)
                        {
                            nTarget = FindPlayer(nSprite, 40);
                            if (nTarget >= 0)
                            {
                                D3PlayFX(StaticSound[kSound24], nSprite);
                                LionList[nLion].nAction = 2;
                                LionList[nLion]._b = 0;

                                sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512) >> 1;
                                sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 1;
                                LionList[nLion].nTarget = nTarget;
                                return;
                            }
                        }
                    }
                    
                    if (nAction)
                    {
                        LionList[nLion]._g--;
                        if (LionList[nLion]._g <= 0)
                        {
                            if (RandomBit())
                            {
                                sprite[nSprite].ang = RandomWord() & kAngleMask;
                                sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512) >> 1;
                                sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 1;
                            }
                            else
                            {
                                sprite[nSprite].xvel = 0;
                                sprite[nSprite].yvel = 0;            
                            }

                            LionList[nLion]._g = 100;
                        }
                    }

                    return;
                }

                case 2:
                {
                    if ((totalmoves & 0x1F) == (LionList[nLion]._f & 0x1F))
                    {
                        PlotCourseToSprite(nSprite, nTarget);

                        short nAng = sprite[nSprite].ang & 0xFFF8;

                        if (sprite[nSprite].cstat & 0x8000)
                        {
                            sprite[nSprite].xvel = Sin(nAng + 512) * 2;
                            sprite[nSprite].yvel = Sin(nAng) * 2;
                        }
                        else
                        {
                            sprite[nSprite].xvel = Sin(nAng + 512) >> 1;
                            sprite[nSprite].yvel = Sin(nAng) >> 1;
                        }
                    }

                    if ((nVal & 0xC000) < 0x8000)
                    {
                        break;
                    }
                    else if ((nVal & 0xC000) == 0x8000)
                    {
                        // loc_378FA:
                        sprite[nSprite].ang = (sprite[nSprite].ang + 256) & kAngleMask;
                        sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512) >> 1;
                        sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 1;
                        break;
                    }
                    else if ((nVal & 0xC000) == 0xC000)
                    {
                        if ((nVal & 0x3FFF) == nTarget)
                        {
                            if (sprite[nSprite].cstat & 0x8000)
                            {
                                LionList[nLion].nAction = 9;
                                sprite[nSprite].cstat &= 0x7FFF;
                                sprite[nSprite].xvel = 0;
                                sprite[nSprite].yvel = 0;
                            }
                            else
                            {
                                int nAng = getangle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].y - sprite[nSprite].y);

                                if (AngleDiff(sprite[nSprite].ang, nAng) < 64)
                                {
                                    LionList[nLion].nAction = 3;
                                }
                            }

                            LionList[nLion]._b = 0;
                            break;
                        }
                        else
                        {
                            // loc_378FA:
                            sprite[nSprite].ang = (sprite[nSprite].ang + 256) & kAngleMask;
                            sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512) >> 1;
                            sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 1;
                            break;
                        }
                    }

                    break;
                }

                case 3:
                {
                    if (nTarget == -1)
                    {
                        LionList[nLion].nAction = 1;
                        LionList[nLion]._g = 50;
                    }
                    else
                    {
                        if (PlotCourseToSprite(nSprite, nTarget) >= 768)
                        {
                            LionList[nLion].nAction = 2;
                        }
                        else if (nFlag & 0x80)
                        {
                            runlist_DamageEnemy(nTarget, nSprite, 10);
                        }
                    }

                    break;
                }

                case 4:
                {
                    if (var_18)
                    {
                        LionList[nLion].nAction = 2;
                        LionList[nLion]._b = 0;
                    }

                    if (nVal & 0x20000)
                    {
                        sprite[nSprite].xvel >>= 1;
                        sprite[nSprite].yvel >>= 1;
                    }

                    return;
                }

                case 5:
                {
                    LionList[nLion]._g--;
                    if (LionList[nLion]._g <= 0)
                    {
                        sprite[nSprite].zvel = -4000;
                        LionList[nLion]._g = 0;

                        int x = sprite[nSprite].x;
                        int y = sprite[nSprite].y;
                        int z = sprite[nSprite].z - (GetSpriteHeight(nSprite) >> 1);

                        int var_40 = 0x7FFFFFFF;
                        
                        short nSector = sprite[nSprite].sectnum;
                        short var_28 = sprite[nSprite].ang;

                        short nAng = (sprite[nSprite].ang - 512) & kAngleMask;

                        for (int i = 0; i < 5; i++)
                        {
                            short hitsect, hitwall, hitsprite;
                            int hitx, hity, hitz;
                            vec3_t startPos = { x, y, z };
                            hitdata_t hitData;
                            hitscan(&startPos, sprite[nSprite].sectnum, Sin(nAng + 512), Sin(nAng), 0, &hitData, CLIPMASK1);
                            hitx = hitData.pos.x;
                            hity = hitData.pos.y;
                            hitz = hitData.pos.z;
                            hitsprite = hitData.sprite;
                            hitsect = hitData.sect;
                            hitwall = hitData.wall;

                            if (hitwall > -1)
                            {
                                int ebx = klabs(hitx - x);
                                int eax = klabs(hity - y);

                                ebx += eax;

                                if (ebx < var_40)
                                {
                                    var_40 = ebx;
                                    var_28 = nAng;
                                }
                            }

                            nAng += 256;
                            nAng &= kAngleMask;
                        }

                        sprite[nSprite].ang = var_28;

                        LionList[nLion].nAction = 6;
                        sprite[nSprite].xvel = (Sin(sprite[nSprite].ang + 512)) - (Sin(sprite[nSprite].ang + 512) >> 3);
                        sprite[nSprite].yvel = (Sin(sprite[nSprite].ang)) - (Sin(sprite[nSprite].ang) >> 3);
                        D3PlayFX(StaticSound[kSound24], nSprite);
                    }

                    return;
                }

                case 6:
                {
                    if (nVal & 0x30000)
                    {
                        LionList[nLion].nAction = 2;
                        LionList[nLion]._b = 0;
                        return;
                    }

                    if ((nVal & 0xC000) == 0x8000)
                    {
                        LionList[nLion].nAction = 7;
                        sprite[nSprite].ang = (GetWallNormal(nVal & 0x3FFF) + 1024) & kAngleMask;
                        LionList[nLion]._g = RandomSize(4);
                        return;
                    }
                    else if ((nVal & 0xC000) == 0xC000)
                    {
                        if ((nVal & 0x3FFF) == nTarget)
                        {
                            int nAng = getangle(sprite[nTarget].x - sprite[nSprite].x, sprite[nTarget].y - sprite[nSprite].y);
                            if (AngleDiff(sprite[nSprite].ang, nAng) < 64)
                            {
                                LionList[nLion].nAction = 3;
                                LionList[nLion]._b = 0;
                            }
                        }
                        else
                        {
                            // loc_378FA:
                            sprite[nSprite].ang = (sprite[nSprite].ang + 256) & kAngleMask;
                            sprite[nSprite].xvel = Sin(sprite[nSprite].ang + 512) >> 1;
                            sprite[nSprite].yvel = Sin(sprite[nSprite].ang) >> 1;
                            break;
                        }
                    }
                    
                    return;
                }

                case 7:
                {
                    LionList[nLion]._g--;

                    if (LionList[nLion]._g <= 0)
                    {
                        LionList[nLion]._g = 0;
                        if (nTarget > -1)
                        {
                            PlotCourseToSprite(nSprite, nTarget);
                        }
                        else
                        {
                            sprite[nSprite].ang = (RandomSize(9) + (sprite[nSprite].ang + 768)) & kAngleMask;
                        }

                        sprite[nSprite].zvel = -1000;

                        LionList[nLion].nAction = 6;
                        sprite[nSprite].xvel = (Sin(sprite[nSprite].ang + 512)) - (Sin(sprite[nSprite].ang + 512) >> 3);
                        sprite[nSprite].yvel = (Sin(sprite[nSprite].ang)) - (Sin(sprite[nSprite].ang) >> 3);
                        D3PlayFX(StaticSound[kSound24], nSprite);
                    }

                    return;
                }

                case 8:
                {
                    if (var_18)
                    {
                        LionList[nLion].nAction = 2;
                        LionList[nLion]._b = 0;
                        sprite[nSprite].cstat |= 0x8000;
                    }
                    return;
                }

                case 9:
                {
                    if (var_18)
                    {
                        LionList[nLion]._b = 0;
                        LionList[nLion].nAction = 2;
                        sprite[nSprite].cstat |= 0x101;
                    }
                    return;
                }

                case 10:
                case 11:
                {
                    if (var_18)
                    {
                        runlist_SubRunRec(sprite[nSprite].owner);
                        runlist_SubRunRec(MoveHook[nLion]);
                        sprite[nSprite].cstat = 0x8000;
                    }
                    return;
                }
            }

            // loc_379AD: ?
            if (nAction != 1 && nTarget != -1)
            {
                if (!(sprite[nTarget].cstat & 0x101))
                {
                    LionList[nLion].nAction = 1;
                    LionList[nLion]._b = 0;
                    LionList[nLion]._g = 100;
                    LionList[nLion].nTarget = -1;
                    sprite[nSprite].xvel = 0;
                    sprite[nSprite].yvel = 0;
                }
            }

            return;
        }
    }
}
