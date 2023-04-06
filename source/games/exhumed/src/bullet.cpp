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
#include "player.h"
#include "names.h"
#include <string.h>
#include <assert.h>

BEGIN_PS_NS

enum { kMaxBullets		= 500 };


// 32 bytes
struct Bullet
{
    TObjPtr<DExhumedActor*> pActor;
    TObjPtr<DExhumedActor*> pEnemy;

    int16_t nSeq;
    int16_t nFrame;
    int16_t nRunRec;
    int16_t nRunRec2;
    int16_t nType;
    int16_t nPitch;
    int16_t field_E;
    uint16_t field_10;
    uint8_t field_12;
    uint8_t nDoubleDamage;
	DVector3 vect;
};

FreeListArray<Bullet, kMaxBullets> BulletList;
DVector3 lasthit;
sectortype* lasthitsect;

size_t MarkBullets()
{
    for (int i = 0; i < kMaxBullets; i++)
    {
        GC::Mark(BulletList[i].pActor);
        GC::Mark(BulletList[i].pEnemy);
    }
    return 2 * kMaxBullets;
}

int nRadialBullet = 0;

FSerializer& Serialize(FSerializer& arc, const char* keyname, Bullet& w, Bullet* def)
{
    static Bullet nul;
    if (!def)
    {
        def = &nul;
        if (arc.isReading()) w = {};
    }
    if (arc.BeginObject(keyname))
    {
        arc("seq", w.nSeq, def->nSeq)
            ("frame", w.nFrame, def->nFrame)
            ("sprite", w.pActor, def->pActor)
            ("type", w.nType, def->nType)
            ("pos", w.vect, def->vect)
            ("at6", w.nRunRec, def->nRunRec)
            ("at8", w.nRunRec2, def->nRunRec2)
            ("atc", w.nPitch, def->nPitch)
            ("ate", w.field_E, def->field_E)
            ("at10", w.field_10, def->field_10)
            ("at12", w.field_12, def->field_12)
            ("at13", w.nDoubleDamage, def->nDoubleDamage)
            ("enemy", w.pEnemy, def->pEnemy)
            .EndObject();
    }
    return arc;
}

void SerializeBullet(FSerializer& arc)
{
    if (arc.BeginObject("bullets"))
    {
        arc ("list", BulletList)
            ("lasthit", lasthit)
            ("lasthitsect", lasthitsect)
            ("radialbullet", nRadialBullet)
            .EndObject();
    }
}

bulletInfo BulletInfo[] = {
    { 25,   1,    20, -1, -1, 13, 0,  0, -1 },
    { 25,  -1, 65000, -1, 31, 73, 0,  0, -1 },
    { 15,  -1, 60000, -1, 31, 73, 0,  0, -1 },
    { 5,   15,  2000, -1, 14, 38, 4,  5,  3 },
    { 250, 100, 2000, -1, 33, 34, 4, 20, -1 },
    { 200, -1,  2000, -1, 20, 23, 4, 10, -1 },
    { 200, -1, 60000, 68, 68, -1, -1, 0, -1 },
    { 300,  1,     0, -1, -1, -1, 0, 50, -1 },
    { 18,  -1,  2000, -1, 18, 29, 4,  0, -1 },
    { 20,  -1,  2000, 37, 11, 30, 4,  0, -1 },
    { 25,  -1,  3000, -1, 44, 36, 4, 15, 90 },
    { 30,  -1,  1000, -1, 52, 53, 4, 20, 48 },
    { 20,  -1,  3500, -1, 54, 55, 4, 30, -1 },
    { 10,  -1,  5000, -1, 57, 76, 4,  0, -1 },
    { 40,  -1,  1500, -1, 63, 38, 4, 10, 40 },
    { 20,  -1,  2000, -1, 60, 12, 0,  0, -1 },
    { 5,   -1, 60000, -1, 31, 76, 0,  0, -1 }
};


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitBullets()
{
    BulletList.Clear();
    lasthitsect = nullptr;
}

int GrabBullet()
{
    int grabbed = BulletList.Get();
    if (grabbed < 0) return -1;
    BulletList[grabbed].pEnemy = nullptr;
    return grabbed;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DestroyBullet(int nBullet)
{
    DExhumedActor* pActor = BulletList[nBullet].pActor;

    runlist_DoSubRunRec(BulletList[nBullet].nRunRec);
    runlist_DoSubRunRec(pActor->spr.lotag - 1);
    runlist_SubRunRec(BulletList[nBullet].nRunRec2);

    StopActorSound(pActor);

    DeleteActor(pActor);
    BulletList.Release(nBullet);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void IgniteSprite(DExhumedActor* pActor)
{
    pActor->spr.hitag += 2;

    auto pAnimActor = BuildAnim(nullptr, 38, 0, pActor->spr.pos, pActor->sector(), 0.625, 20);

    if (pAnimActor)
    {
        pAnimActor->pTarget = pActor;
        ChangeActorStat(pAnimActor, kStatIgnited);
        auto tex = TexMan.GetGameTexture(pAnimActor->spr.spritetexture());
        pAnimActor->spr.scale.Y = (max(1.f, (tex->GetDisplayHeight() * 32) / nFlameHeight) * REPEAT_SCALE);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void BulletHitsSprite(Bullet *pBullet, DExhumedActor* pBulletActor, DExhumedActor* pHitActor, const DVector3& pos, sectortype* pSector)
{
    assert(pSector != nullptr);

    bulletInfo *pBulletInfo = &BulletInfo[pBullet->nType];

    int nStat = pHitActor->spr.statnum;

    switch (pBullet->nType)
    {
        case 3:
        {
            if (nStat > 107 || nStat == kStatAnubisDrum) {
                return;
            }

            pHitActor->spr.hitag++;

            if (pHitActor->spr.hitag == 15) {
                IgniteSprite(pHitActor);
            }

            if (!RandomSize(2)) {
                BuildAnim(nullptr, pBulletInfo->field_C, 0, pos, pSector, 0.625, pBulletInfo->nFlags);
            }

            return;
        }
        case 14:
        {
            if (nStat > 107 || nStat == kStatAnubisDrum) {
                return;
            }
            // else - fall through to below cases
            [[fallthrough]];
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

            DExhumedActor* pActor = pBullet->pActor;

            if (nStat == kStatAnubisDrum)
            {
                auto nAngle = (pActor->spr.Angles.Yaw + DAngle22_5) - RandomAngle9();

				pHitActor->vel.XY() = nAngle.ToVector() * 2048;
                pHitActor->vel.Z = -(RandomSize(3) + 1);
            }
            else
            {
                auto Vel = pHitActor->vel.XY();
                pHitActor->VelFromAngle(-2);

                MoveCreature(pHitActor);
				pHitActor->vel.XY() = Vel;
            }

            break;
        }

        default:
            break;
    }

    // BHS_switchBreak:
    int nDamage = pBulletInfo->nDamage;

    if (pBullet->nDoubleDamage > 1) {
        nDamage *= 2;
    }

    runlist_DamageEnemy(pHitActor, pBulletActor, nDamage);

    if (nStat <= 90 || nStat >= 199)
    {
        BuildAnim(nullptr, pBulletInfo->field_C, 0, pos, pSector, 0.625, pBulletInfo->nFlags);
        return;
    }

    switch (nStat)
    {
        case kStatDestructibleSprite:
            break;
        case kStatAnubisDrum:
        case 102:
        case kStatExplodeTrigger:
        case kStatExplodeTarget:
            BuildAnim(nullptr, 12, 0, pos, pSector, 0.625, 0);
            break;
        default:
            BuildAnim(nullptr, 39, 0, pos, pSector, 0.625, 0);
            if (pBullet->nType > 2)
            {
                BuildAnim(nullptr, pBulletInfo->field_C, 0, pos, pSector, 0.625, pBulletInfo->nFlags);
            }
            break;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int MoveBullet(int nBullet)
{

    DExhumedActor* hitactor = nullptr;
    sectortype* pHitSect = nullptr;
    walltype* pHitWall = nullptr;

    Bullet *pBullet = &BulletList[nBullet];
    int nType = pBullet->nType;
    bulletInfo *pBulletInfo = &BulletInfo[nType];

    DExhumedActor* pActor = BulletList[nBullet].pActor;

	auto apos = pActor->spr.pos;
    int nSectFlag = pActor->sector()->Flag;

	DVector3 pos;

    int nVal = 0;
    Collision coll;

    if (pBullet->field_10 < 30000)
    {
        DExhumedActor* pEnemyActor = BulletList[nBullet].pEnemy;
        if (pEnemyActor)
        {
            if (!(pEnemyActor->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))
                BulletList[nBullet].pEnemy = nullptr;
            else
            {
                coll = AngleChase(pActor, pEnemyActor, pBullet->field_10, 0, DAngle22_5 / 8);
                goto MOVEEND;
            }
        }
        if (nType == 3)
        {
            if (pBullet->field_E < 8)
            {
                pActor->spr.scale.X -= REPEAT_SCALE;
				pActor->spr.scale.Y += (0.125);

                pBullet->vect.Z -= 200/256.;

                if (pActor->spr.shade < 90) {
                    pActor->spr.shade += 35;
                }

                if (pBullet->field_E == 3)
                {
                    pBullet->nSeq = 45;
                    pBullet->nFrame = 0;
                    pActor->spr.scale = DVector2(0.625, 0.625);
                    pActor->spr.shade = 0;
					pActor->spr.pos.Z += 2;
                }
            }
            else
            {
				pActor->spr.scale.X += (0.0625);
				pActor->spr.scale.Y += (0.0625);
            }
        }

        coll = movesprite(pActor, pBullet->vect.XY(), pBullet->vect.Z, pActor->clipdist / 128., CLIPMASK1);

MOVEEND:
        if (coll.type || coll.exbits)
        {
            nVal = 1;
			pos = pActor->spr.pos;
            pHitSect = pActor->sector();

            switch (coll.type)
            {
            case kHitWall:
                pHitWall = coll.hitWall;
                goto HITWALL;
            case kHitSprite:
                if (!coll.exbits)
                {
                    hitactor = coll.actor();
                    goto HITSPRITE;
                }
            default:
                if (coll.exbits)
                    goto HITSECT;
                break;

            }
        }

        nVal = coll.type || coll.exbits? 1:0;

        // pSprite's sector may have changed since we set nSectFlag ?
        int nFlagVal = nSectFlag ^ pActor->sector()->Flag;
        if (nFlagVal & kSectUnderwater)
        {
            DestroyBullet(nBullet);
            nVal = 1;
        }

        if (nVal == 0 && nType != 15 && nType != 3)
        {
            AddFlash(pActor->sector(), pActor->spr.pos, 0);

            if (pActor->spr.pal != 5) {
                pActor->spr.pal = 1;
            }
        }
    }
    else
    {
        nVal = 1;

        if (BulletList[nBullet].pEnemy)
        {
            hitactor = BulletList[nBullet].pEnemy;
            pos = hitactor->spr.pos.plusZ(-GetActorHeight(hitactor) * 0.5);
            pHitSect = hitactor->sector();
        }
        else
        {

            HitInfo hit{};
            double dz = -pBullet->nPitch *2;
			hitscan(apos, pActor->sector(), DVector3(pActor->spr.Angles.Yaw.ToVector() * 1024, dz), hit, CLIPMASK1);
            pos = hit.hitpos;
            hitactor = hit.actor();
            pHitSect = hit.hitSector;
            pHitWall = hit.hitWall;
        }

        lasthit = pos;
        lasthitsect = pHitSect;

        if (hitactor)
        {
HITSPRITE:
            if (pActor->spr.pal == 5 && hitactor->spr.statnum == 100)
            {
                int nPlayer = GetPlayerFromActor(hitactor);
                if (!PlayerList[nPlayer].bIsMummified)
                {
                    PlayerList[nPlayer].bIsMummified = true;
                    SetNewWeapon(nPlayer, kWeaponMummified);
                }
            }
            else
            {
                BulletHitsSprite(pBullet, pActor->pTarget, hitactor, pos, pHitSect);
            }
        }
        else if (pHitWall != nullptr)
        {
        HITWALL:
            if (pHitWall->walltexture == tileGetTextureID(kEnergy1))
            {
                if (pHitWall->twoSided())
                {
                    int nDamage = BulletInfo[pBullet->nType].nDamage;
                    if (pBullet->nDoubleDamage > 1) {
                        nDamage *= 2;
                    }

                    DExhumedActor* eb = EnergyBlocks[pHitWall->nextSector()->extra];
                    if (eb) runlist_DamageEnemy(eb, pActor, nDamage);
                }
            }
        }

    HITSECT:
        if (pHitSect != nullptr) // NOTE: hitsect can be -1. this check wasn't in original code. TODO: demo compatiblity?
        {
            if (hitactor == nullptr && pHitWall == nullptr)
            {
                if ((pHitSect->pBelow != nullptr && (pHitSect->pBelow->Flag & kSectUnderwater)) || pHitSect->Depth)
                {
					pActor->spr.pos = pos;
                    BuildSplash(pActor, pHitSect);
                }
                else
                {
                    BuildAnim(nullptr, pBulletInfo->field_C, 0, pos, pHitSect, 0.625, pBulletInfo->nFlags);
                }
            }
            else
            {
                if (pHitWall != nullptr)
                {
					pos.X -= pActor->spr.Angles.Yaw.Cos() * 0.5;
					pos.Y -= pActor->spr.Angles.Yaw.Sin() * 0.5;

                    if (nType != 3 || RandomSize(2) == 0)
                    {
                        double zOffset = RandomSize(8) / 32.;

                        if (!RandomBit()) {
                            zOffset = -zOffset;
                        }

                        // draws bullet puff on walls when they're shot
                        BuildAnim(nullptr, pBulletInfo->field_C, 0, pos.plusZ(zOffset - 16), pHitSect, 0.625, pBulletInfo->nFlags);
                    }
                }
                else
                {
					pActor->spr.pos = pos;

                    ChangeActorSect(pActor, pHitSect);
                }

                if (BulletInfo[nType].nRadius)
                {
                    nRadialBullet = nType;

                    runlist_RadialDamageEnemy(pActor, pBulletInfo->nDamage, pBulletInfo->nRadius);

                    nRadialBullet = -1;

                    AddFlash(pActor->sector(), pActor->spr.pos, 128);
                }
            }
        }

        DestroyBullet(nBullet);
    }

    return nVal;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SetBulletEnemy(int nBullet, DExhumedActor* pEnemy)
{
    if (nBullet >= 0) {
        BulletList[nBullet].pEnemy = pEnemy;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DExhumedActor* BuildBullet(DExhumedActor* pActor, int nType, double fZOffset, DAngle nAngle, DExhumedActor* pTarget, int nDoubleDamage, int nPitch)
{
    Bullet sBullet;
    bulletInfo *pBulletInfo = &BulletInfo[nType];

    if (pBulletInfo->field_4 > 30000)
    {
        if (pTarget)
        {
            if (pTarget->spr.cstat & CSTAT_SPRITE_BLOCK_ALL)
            {
                sBullet.nType = nType;
                sBullet.nDoubleDamage = nDoubleDamage;

                sBullet.pActor = insertActor(pActor->sector(), 200);
                sBullet.pActor->spr.Angles.Yaw = nAngle;

                double nHeight = GetActorHeight(pTarget);

				assert(pTarget->sector());

                BulletHitsSprite(&sBullet, pActor, pTarget, pTarget->spr.pos.plusZ(-(nHeight * 0.5)), pTarget->sector());
                DeleteActor(sBullet.pActor);
                return nullptr;
            }
            else
            {
                nPitch = 0;
            }
        }
    }

    int nBullet = GrabBullet();
    if (nBullet < 0) {
        return nullptr;
    }

    sectortype* pSector;

    if (pActor->spr.statnum == 100)
    {
        pSector = PlayerList[GetPlayerFromActor(pActor)].pPlayerViewSect;
    }
    else
    {
        pSector = pActor->sector();
    }

    auto pBulletActor = insertActor(pSector, 200);
	double fHeight = GetActorHeight(pActor) * 0.75;

    if (fZOffset == INT_MAX) {
        fZOffset = -fHeight;
    }

    pBulletActor->spr.pos = pActor->spr.pos;

    Bullet *pBullet = &BulletList[nBullet];

    pBullet->pEnemy = nullptr;

    pBulletActor->spr.cstat = 0;
    pBulletActor->spr.shade = -64;

    if (pBulletInfo->nFlags & 4) {
        pBulletActor->spr.pal = 4;
    }
    else {
        pBulletActor->spr.pal = 0;
    }

	pBulletActor->clipdist = 6.25;

    int nRepeat = pBulletInfo->xyRepeat;
    if (nRepeat < 0) {
        nRepeat = 30;
    }

    pBulletActor->spr.scale = DVector2(nRepeat * REPEAT_SCALE, nRepeat * REPEAT_SCALE);
    pBulletActor->spr.xoffset = 0;
    pBulletActor->spr.yoffset = 0;
    pBulletActor->spr.Angles.Yaw = nAngle;
    pBulletActor->vel.X = 0;
    pBulletActor->vel.Y = 0;
    pBulletActor->vel.Z = 0;
    pBulletActor->spr.lotag = runlist_HeadRun() + 1;
    pBulletActor->spr.extra = -1;
    pBulletActor->spr.hitag = 0;
    pBulletActor->pTarget = pActor;
    pBulletActor->nPhase = nBullet;

//	GrabTimeSlot(3);

    pBullet->field_10 = 0;
    pBullet->field_E = pBulletInfo->field_2;
    pBullet->nFrame  = 0;

    int nSeq;

    if (pBulletInfo->field_8 != -1)
    {
        pBullet->field_12 = 0;
        nSeq = pBulletInfo->field_8;
    }
    else
    {
        pBullet->field_12 = 1;
        nSeq = pBulletInfo->nSeq;
    }

    pBullet->nSeq = nSeq;

    pBulletActor->spr.picnum = seq_GetSeqPicnum(nSeq, 0, 0);

    if (nSeq == kSeqBullet) {
        pBulletActor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
    }

    pBullet->nPitch = nPitch;
    pBullet->nType = nType;
    pBullet->pActor = pBulletActor;
    pBullet->nRunRec = runlist_AddRunRec(pBulletActor->spr.lotag - 1, nBullet, 0xB0000);
    pBullet->nRunRec2 = runlist_AddRunRec(NewRun, nBullet, 0xB0000);
    pBullet->nDoubleDamage = nDoubleDamage;
    pBulletActor->spr.pos.Z += fZOffset;
    pBulletActor->backuppos();

    double nVertVel = 0;

    pSector = pBulletActor->sector();

    while (pBulletActor->spr.pos.Z < pSector->ceilingz)
    {
        if (pSector->pAbove == nullptr)
        {
            pBulletActor->spr.pos.Z = pSector->ceilingz;
            break;
        }

        pSector = pSector->pAbove;
        ChangeActorSect(pBulletActor, pSector);
    }

    if (pTarget == nullptr)
    {
        nVertVel = (-DAngle::fromBuild(nPitch).Sin() * (1. / 32) * pBulletInfo->field_4);
    }
    else
    {
        if ((unsigned int)pBulletInfo->field_4 > 30000)
        {
            BulletList[nBullet].pEnemy = pTarget;
        }
        else
        {
            fHeight = GetActorHeight(pTarget);

            if (pTarget->spr.statnum == 100)
            {
                fHeight *= 0.75;
            }
            else
            {
                fHeight *= 0.5;
            }

            double fTop = pTarget->spr.pos.Z - fHeight;

            DVector2 xy;

            if (pActor != nullptr && pActor->spr.statnum != 100)
            {
                xy = pTarget->spr.pos.XY();

                if (pTarget->spr.statnum != 100)
                {
                    xy += pTarget->vel.XY() * (10. / 32.);
                }
                else
                {
                    int nPlayer = GetPlayerFromActor(pTarget);
                    if (nPlayer > -1)
                    {
                        xy += PlayerList[nPlayer].nPlayerD * (15. / 16.);
                    }
                }

                xy -= pBulletActor->spr.pos.XY();

                nAngle = xy.Angle();
                pActor->spr.Angles.Yaw = nAngle;
            }
            else
            {
                // loc_2ABA3:
                xy = pTarget->spr.pos.XY() - pBulletActor->spr.pos.XY();
            }

            double nSqrt = xy.Length();
            if (nSqrt > 0)
            {
                nVertVel = ((fTop - pBulletActor->spr.pos.Z) * pBulletInfo->field_4) / (nSqrt * 16);
            }
            else
            {
                nVertVel = 0;
            }
        }
    }

    pBullet->vect.Z = 0;
    pBullet->vect.XY() = nAngle.ToVector() * pActor->clipdist;
    BulletList[nBullet].pEnemy = nullptr;


    if (MoveBullet(nBullet))
    {
        pBulletActor = nullptr;
    }
    else
    {
        pBullet->field_10 = pBulletInfo->field_4;
		pBullet->vect.XY() = nAngle.ToVector() * pBulletInfo->field_4 / 128.;
        pBullet->vect.Z = nVertVel * 0.125;
    }

    return pBulletActor;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIBullet::Tick(RunListEvent* ev)
{
    int nBullet = RunData[ev->nRun].nObjIndex;
    assert(nBullet >= 0 && nBullet < kMaxBullets);

    int nSeq = getSeqFromId(BulletList[nBullet].nSeq);
    DExhumedActor* pActor = BulletList[nBullet].pActor;

    int nFlag = FrameFlag[getSeqFrame(nSeq, BulletList[nBullet].nFrame)];

    seq_MoveSequence(pActor, nSeq, BulletList[nBullet].nFrame);

    if (nFlag & 0x80)
    {
        BuildAnim(nullptr, 45, 0, pActor->spr.pos, pActor->sector(), pActor->spr.scale.X, 0);
    }

    BulletList[nBullet].nFrame++;
    if (BulletList[nBullet].nFrame >= getSeqFrameCount(nSeq))
    {
        if (!BulletList[nBullet].field_12)
        {
            BulletList[nBullet].nSeq = BulletInfo[BulletList[nBullet].nType].nSeq;
            BulletList[nBullet].field_12++;
        }

        BulletList[nBullet].nFrame = 0;
    }

    if (BulletList[nBullet].field_E != -1 && --BulletList[nBullet].field_E == 0)
    {
        DestroyBullet(nBullet);
    }
    else
    {
        MoveBullet(nBullet);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIBullet::Draw(RunListEvent* ev)
{
    int nBullet = RunData[ev->nRun].nObjIndex;
    assert(nBullet >= 0 && nBullet < kMaxBullets);

    int nSeq = getSeqFromId(BulletList[nBullet].nSeq);

    ev->pTSprite->statnum = 1000;

    if (BulletList[nBullet].nType == 15)
    {
        seq_PlotArrowSequence(ev->nParam, nSeq, BulletList[nBullet].nFrame);
    }
    else
    {
        seq_PlotSequence(ev->nParam, nSeq, BulletList[nBullet].nFrame, 0);
        ev->pTSprite->ownerActor = nullptr;
    }
}

END_PS_NS
