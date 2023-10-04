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
#include "aistuff.h"
#include "engine.h"
#include "player.h"
#include "exhumed.h"
#include "sound.h"
#include "sequence.h"
#include <assert.h>

BEGIN_PS_NS



//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DestroyGrenade(DExhumedActor* pActor)
{
    runlist_DoSubRunRec(pActor->nPhase);
    runlist_SubRunRec(pActor->nRun);
    runlist_DoSubRunRec(pActor->spr.lotag - 1);

    DeleteActor(pActor);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void BounceGrenade(DExhumedActor* pActor, DAngle nAngle)
{
    pActor->nTurn >>= 1;

    pActor->vec = nAngle.ToVector() * pActor->nTurn / 512.;
    D3PlayFX(StaticSound[kSound3], pActor);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ThrowGrenade(int nPlayer, double dz, double push1)
{
    if (getPlayer(nPlayer)->pPlayerGrenade == nullptr)
        return;

    DExhumedActor* pActor = getPlayer(nPlayer)->pPlayerGrenade;
    auto pPlayerActor = getPlayer(nPlayer)->GetActor();

    DAngle nAngle = pPlayerActor->spr.Angles.Yaw;

    ChangeActorSect(pActor, getPlayer(nPlayer)->pPlayerViewSect);

    pActor->spr.pos = pPlayerActor->spr.pos;

    if (nAngle < nullAngle) {
        nAngle = pPlayerActor->spr.Angles.Yaw;
    }

    pActor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
    pActor->spr.Angles.Yaw = nAngle;

    if (push1 <= 23.4375)
    {
        int nVel = (int)(getPlayer(nPlayer)->totalvel * 512.);

        pActor->nTurn = ((90 - pActor->nIndex2) * (90 - pActor->nIndex2)) + nVel;
        pActor->vel.Z = ((32. * push1) - 17);


        DVector2 vec = nAngle.ToVector() * pPlayerActor->clipdist *2; // == << 14 + 3 + 2 - 18
        auto nMov = movesprite(pActor, vec, dz, 0, CLIPMASK1);
        if (nMov.type == kHitWall)
        {
            nAngle = nMov.hitWall->normalAngle();
            BounceGrenade(pActor, nAngle);
        }
    }
    else
    {
        pActor->nTurn = 0;
		pActor->vel.Z = pPlayerActor->vel.Z;
    }

    pActor->vec = nAngle.ToVector() * pActor->nTurn / 256;

    getPlayer(nPlayer)->pPlayerGrenade = nullptr;

    return;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void BuildGrenade(int nPlayer)
{
    auto pActor = insertActor(getPlayer(nPlayer)->pPlayerViewSect, 201);

	auto pPlayerActor = getPlayer(nPlayer)->GetActor();

	pActor->spr.pos = pPlayerActor->spr.pos.plusZ(-15);
    pActor->spr.shade = -64;
    pActor->spr.scale = DVector2(0.3125, 0.3125);
    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    setvalidpic(pActor);
    pActor->spr.pal = 0;
	pActor->clipdist = 7.5;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->spr.Angles.Yaw = pPlayerActor->spr.Angles.Yaw;
    pActor->spr.intowner = nPlayer;
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = 0;
    pActor->spr.hitag = 0;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.extra = -1;
    pActor->backuppos();

//	GrabTimeSlot(3);

    pActor->nIndex2 = 90;
    pActor->nHealth = 0;
    pActor->nCount = 16;
    pActor->nTurn = -1;
    pActor->nIndex = 0;
    pActor->nFrame = 0;
    pActor->nPhase = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x0F0000);
    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x0F0000);

    getPlayer(nPlayer)->pPlayerGrenade = pActor;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ExplodeGrenade(DExhumedActor* pActor)
{
    FName animFile;
	double scale;

    int nPlayer = pActor->spr.intowner;
    auto pGrenadeSect = pActor->sector();

    pActor->nFrame = 1;

    if (pGrenadeSect->Flag & kSectUnderwater)
    {
        animFile = "grenbubb";
        scale = 0.9375;
    }
    else
    {
        if (pActor->spr.pos.Z < pGrenadeSect->floorz)
        {
            scale = 3.125;
            animFile = "grenpow";

// TODO		MonoOut("GRENPOW\n");
        }
        else
        {
            animFile = "grenboom";
            scale = 2.3475;

// TODO		MonoOut("GRENBOOM\n");
        }
    }

    if (pActor->nTurn < 0)
    {
        auto pPlayerActor = getPlayer(nPlayer)->GetActor();
        auto nAngle = pPlayerActor->spr.Angles.Yaw;
		
		DVector2 vect = nAngle.ToVector() * 32;
		pActor->spr.pos = pPlayerActor->spr.pos + vect;

        ChangeActorSect(pActor, pPlayerActor->sector());

        if (!getPlayer(nPlayer)->invincibility) {
            getPlayer(nPlayer)->nHealth = 1;
        }
    }

    int nDamage = BulletInfo[kWeaponGrenade].nDamage;

    if (getPlayer(nPlayer)->nDouble > 0) {
        nDamage *= 2;
    }

    runlist_RadialDamageEnemy(pActor, nDamage, BulletInfo[kWeaponGrenade].nRadius);

    BuildAnim(nullptr, animFile, 0, pActor->spr.pos, pActor->sector(), scale, 4);
    AddFlash(pActor->sector(), pActor->spr.pos, 128);

    DestroyGrenade(pActor);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIGrenade::Draw(RunListEvent* ev)
{
    if (const auto pActor = ev->pObjActor)
    {
        if (pActor->nFrame)
        {
            seq_PlotSequence(ev->nParam, "grenboom", 0, pActor->nHealth >> 8, 1);
        }
        else
        {
            seq_PlotSequence(ev->nParam, "grenroll", pActor->nIndex, pActor->nHealth >> 8, 1);
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIGrenade::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    const auto grenadeSeq = pActor->nFrame ? getSequence("grenboom") : getSequence("grenroll", pActor->nIndex);
    const auto& seqFrame = grenadeSeq->frames[pActor->nHealth >> 8];

    seqFrame.playSound(pActor);
    pActor->spr.setspritetexture(seqFrame.getFirstChunkTexture());

    pActor->nIndex2--;
    if (!pActor->nIndex2)
    {
        const auto pPlayer = getPlayer(pActor->spr.intowner);

        if (pActor->nTurn < 0)
        {
            pPlayer->nState = 0;
            pPlayer->nWeapFrame = 0;

            if (pPlayer->nAmmo[kWeaponGrenade])
            {
                pPlayer->bIsFiring = false;
            }
            else
            {
                SelectNewWeapon(pPlayer);

                pPlayer->nCurrentWeapon = pPlayer->nNextWeapon;
                pPlayer->nNextWeapon = -1;
            }
        }

        ExplodeGrenade(pActor);
        return;
    }
    else
    {
        if (pActor->nTurn < 0) {
            return;
        }

        int ebp = (pActor->nHealth + pActor->nCount) >> 8;

        pActor->nHealth += pActor->nCount;

        if (ebp < 0)
        {
            pActor->nHealth += grenadeSeq->frames.Size() << 8;
        }
        else
        {
            if (ebp >= (signed)grenadeSeq->frames.Size())
            {
                if (pActor->nFrame)
                {
                    DestroyGrenade(pActor);
                    return;
                }
                else
                {
                    pActor->nHealth = pActor->nFrame;
                }
            }
        }

        if (pActor->nFrame) {
            return;
        }

        double zVel = pActor->vel.Z;

        Gravity(pActor);
        auto nMov = movesprite(pActor, pActor->vec, pActor->vel.Z, pActor->clipdist / 128., CLIPMASK1);

        if (!nMov.type && !nMov.exbits)
            return;

        if (nMov.exbits & kHitAux2)
        {
            if (zVel)
            {
                if (pActor->sector()->Damage > 0)
                {
                    ExplodeGrenade(pActor);
                    return;
                }

                pActor->nCount = (uint8_t)totalmoves; // limit to 8bits?

                D3PlayFX(StaticSound[kSound3], pActor);

                pActor->vel.Z = -zVel * 0.5;

                if (pActor->vel.Z > -5)
                {
                    D3PlayFX(StaticSound[kSound5], pActor);
                    pActor->nCount = 0;
                    pActor->nHealth = 0;
                    pActor->vel.Z = 0;
                    pActor->nIndex = 1;
                }
            }

            pActor->nCount = 255 - (RandomByte() * 2);
            pActor->vec *= (15./16.);
        }

        // loc_2CF60:
        if (nMov.type == kHitWall)
        {
            BounceGrenade(pActor, nMov.hitWall->normalAngle());
        }
        else if (nMov.type == kHitSprite)
        {
            BounceGrenade(pActor, nMov.actor()->spr.Angles.Yaw);
        }

        pActor->nHealth = 0;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIGrenade::RadialDamage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    if (pActor != ev->pRadialActor && !pActor->nFrame)
    {
        if (runlist_CheckRadialDamage(pActor) > 280)
        {
            pActor->nIndex2 = RandomSize(4) + 1;
        }
    }
}


END_PS_NS
