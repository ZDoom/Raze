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

struct Grenade
{
    short nCount;
    short nHealth;
    short nSprite;
    short nPhase;
    short nRun;
    short nIndex;
    short nFrame;
    short nIndex2;
    int nTurn;
    int x;
    int y;
};

FreeListArray<Grenade, kMaxGrenades> GrenadeList;

FSerializer& Serialize(FSerializer& arc, const char* keyname, Grenade& w, Grenade* def)
{
    static Grenade nul;
    if (!def)
    {
        def = &nul;
        if (arc.isReading()) w = {};
    }
    if (arc.BeginObject(keyname))
    {
        arc("sprite", w.nSprite, def->nSprite)
            ("at0", w.nCount, def->nCount)
            ("at2", w.nHealth, def->nHealth)
            ("at6", w.nPhase, def->nPhase)
            ("at8", w.nRun, def->nRun)
            ("ata", w.nIndex, def->nIndex)
            ("atc", w.nFrame, def->nFrame)
            ("ate", w.nIndex2, def->nIndex2)
            ("at10", w.nTurn, def->nTurn)
            ("x", w.x, def->x)
            ("y", w.y, def->y)
            .EndObject();
    }
    return arc;
}

void SerializeGrenade(FSerializer& arc)
{
    arc("grenades", GrenadeList);
}


void InitGrenades()
{
    GrenadeList.Clear();
}

short GrabGrenade()
{
    return GrenadeList.Get();
}

void DestroyGrenade(short nGrenade)
{
    auto pActor = &GrenadeList[nGrenade];
    runlist_DoSubRunRec(pActor->nPhase);
    runlist_SubRunRec(pActor->nRun);
    runlist_DoSubRunRec(sprite[pActor->nSprite].lotag - 1);

    mydeletesprite(pActor->nSprite);
    GrenadeList.Release(nGrenade);
}

void BounceGrenade(short nGrenade, short nAngle)
{
    auto pActor = &GrenadeList[nGrenade];
    pActor->nTurn >>= 1;

    pActor->x = bcos(nAngle, -5) * pActor->nTurn;
    pActor->y = bsin(nAngle, -5) * pActor->nTurn;

    D3PlayFX(StaticSound[kSound3], pActor->nSprite);
}

int ThrowGrenade(short nPlayer, int, int, int ecx, int push1)
{
    if (nPlayerGrenade[nPlayer] < 0)
        return -1;

    short nGrenade = nPlayerGrenade[nPlayer];
    auto pActor = &GrenadeList[nGrenade];

    short nGrenadeSprite = pActor->nSprite;
    short nPlayerSprite = PlayerList[nPlayer].nSprite;
	auto pGrenadeSprite = &sprite[nGrenadeSprite];
	auto pPlayerSprite = &sprite[nPlayerSprite];

    short nAngle = pPlayerSprite->ang;

    mychangespritesect(nGrenadeSprite, nPlayerViewSect[nPlayer]);

    pGrenadeSprite->x = pPlayerSprite->x;
    pGrenadeSprite->y = pPlayerSprite->y;
    pGrenadeSprite->z = pPlayerSprite->z;

    if (nAngle < 0) {
        nAngle = pPlayerSprite->ang;
    }

    pGrenadeSprite->cstat &= 0x7FFF;
    pGrenadeSprite->ang = nAngle;

    if (push1 >= -3000)
    {
        int nVel = totalvel[nPlayer] << 5;

        pActor->nTurn = ((90 - pActor->nIndex2) * (90 - pActor->nIndex2)) + nVel;
        pGrenadeSprite->zvel = (-64 * push1) - 4352;

        int nMov = movesprite(nGrenadeSprite, bcos(nAngle) * (pPlayerSprite->clipdist << 3), bsin(nAngle) * (pPlayerSprite->clipdist << 3), ecx, 0, 0, CLIPMASK1);
        if (nMov & 0x8000)
        {
            nAngle = GetWallNormal(nMov & 0x3FFF);
            BounceGrenade(nGrenade, nAngle);
        }
    }
    else
    {
        pActor->nTurn = 0;
        pGrenadeSprite->zvel = pPlayerSprite->zvel;
    }

    pActor->x = bcos(nAngle, -4) * pActor->nTurn;
    pActor->y = bsin(nAngle, -4) * pActor->nTurn;

    nPlayerGrenade[nPlayer] = -1;

    return nGrenadeSprite;
}

void BuildGrenade(int nPlayer)
{
    int nGrenade = GrabGrenade();
    if (nGrenade < 0) return;

    auto pActor = &GrenadeList[nGrenade];

    int nSprite = insertsprite(nPlayerViewSect[nPlayer], 201);
    assert(nSprite >= 0 && nSprite < kMaxSprites);
	auto pSprite = &sprite[nSprite];

    int nPlayerSprite = PlayerList[nPlayer].nSprite;
	auto pPlayerSprite = &sprite[nPlayerSprite];

    pSprite->x = pPlayerSprite->x;
    pSprite->y = pPlayerSprite->y;
    pSprite->z = pPlayerSprite->z - 3840;
    pSprite->shade = -64;
    pSprite->xrepeat = 20;
    pSprite->yrepeat = 20;
    pSprite->cstat = 0x8000;
    pSprite->picnum = 1;
    pSprite->pal = 0;
    pSprite->clipdist = 30;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->ang = pPlayerSprite->ang;
    pSprite->owner = nPlayerSprite;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->hitag = 0;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->extra = -1;
    pSprite->backuppos();

//	GrabTimeSlot(3);

    pActor->nIndex2 = 90;
    pActor->nHealth = 0;
    pActor->nCount = 16;
    pActor->nTurn = -1;
    pActor->nSprite = nSprite;
    pActor->nIndex = 0;
    pActor->nFrame = 0;
    pActor->nPhase = runlist_AddRunRec(pSprite->lotag - 1, nGrenade, 0x0F0000);
    pActor->nRun = runlist_AddRunRec(NewRun, nGrenade, 0x0F0000);

    nGrenadePlayer[nGrenade] = nPlayer;
    nPlayerGrenade[nPlayer] = nGrenade;
}

void ExplodeGrenade(short nGrenade)
{
    int var_28, var_20;
    auto pActor = &GrenadeList[nGrenade];

    short nPlayer = nGrenadePlayer[nGrenade];
    int nGrenadeSprite = pActor->nSprite;
	auto pGrenadeSprite = &sprite[nGrenadeSprite];
    short nGrenadeSect = pGrenadeSprite->sectnum;

    pActor->nFrame = 1;

    if (SectFlag[nGrenadeSect] & kSectUnderwater)
    {
        var_28 = 75;
        var_20 = 60;
    }
    else
    {
        if (pGrenadeSprite->z < sector[nGrenadeSect].floorz)
        {
            var_20 = 200;
            var_28 = 36;

// TODO		MonoOut("GRENPOW\n");
        }
        else
        {
            var_28 = 34;
            var_20 = 150;

// TODO		MonoOut("GRENBOOM\n");
        }
    }

    if (pActor->nTurn < 0)
    {
        short nPlayerSprite = PlayerList[nPlayer].nSprite;
		auto pPlayerSprite = &sprite[nPlayerSprite];
        short nAngle = pPlayerSprite->ang;

        pGrenadeSprite->z = pPlayerSprite->z;
        pGrenadeSprite->x = bcos(nAngle, -5) + pPlayerSprite->x;
        pGrenadeSprite->y = bsin(nAngle, -5) + pPlayerSprite->y;

        changespritesect(nGrenadeSprite, pPlayerSprite->sectnum);

        if (!PlayerList[nPlayer].invincibility) {
            PlayerList[nPlayer].nHealth = 1;
        }
    }

    short nDamage = BulletInfo[kWeaponGrenade].nDamage;

    if (PlayerList[nPlayer].nDouble > 0) {
        nDamage *= 2;
    }

    runlist_RadialDamageEnemy(nGrenadeSprite, nDamage, BulletInfo[kWeaponGrenade].nRadius);

    BuildAnim(nullptr, var_28, 0, pGrenadeSprite->x, pGrenadeSprite->y, pGrenadeSprite->z, pGrenadeSprite->sectnum, var_20, 4);
    AddFlash(pGrenadeSprite->sectnum, pGrenadeSprite->x, pGrenadeSprite->y, pGrenadeSprite->z, 128);

    nGrenadePlayer[nGrenade] = -1;
    DestroyGrenade(nGrenade);
}

void AIGrenade::Draw(RunListEvent* ev)
{
    short nGrenade = RunData[ev->nRun].nObjIndex;
    auto pActor = &GrenadeList[nGrenade];
    assert(nGrenade >= 0 && nGrenade < kMaxGrenades);
    short nSeq = pActor->nFrame ? SeqOffsets[kSeqGrenBoom] : SeqOffsets[kSeqGrenRoll] + pActor->nIndex;
    seq_PlotSequence(ev->nParam, nSeq, pActor->nHealth >> 8, 1);
}


void AIGrenade::Tick(RunListEvent* ev)
{
    short nGrenade = RunData[ev->nRun].nObjIndex;
    auto pActor = &GrenadeList[nGrenade];
    assert(nGrenade >= 0 && nGrenade < kMaxGrenades);

    short nGrenadeSprite = pActor->nSprite;
    auto pGrenadeSprite = &sprite[nGrenadeSprite];
    short nSeq = pActor->nFrame ? SeqOffsets[kSeqGrenBoom] : SeqOffsets[kSeqGrenRoll] + pActor->nIndex;

    seq_MoveSequence(nGrenadeSprite, nSeq, pActor->nHealth >> 8);
    pGrenadeSprite->picnum = seq_GetSeqPicnum2(nSeq, pActor->nHealth >> 8);

    pActor->nIndex2--;
    if (!pActor->nIndex2)
    {
        short nPlayer = nGrenadePlayer[nGrenade];

        if (pActor->nTurn < 0)
        {
            PlayerList[nPlayer].field_3A = 0;
            PlayerList[nPlayer].field_3FOUR = 0;

            if (PlayerList[nPlayer].nAmmo[kWeaponGrenade])
            {
                PlayerList[nPlayer].bIsFiring = false;
            }
            else
            {
                SelectNewWeapon(nPlayer);

                PlayerList[nPlayer].nCurrentWeapon = PlayerList[nPlayer].field_38;
                PlayerList[nPlayer].field_38 = -1;
            }
        }

        ExplodeGrenade(nGrenade);
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
            pActor->nHealth += SeqSize[nSeq] << 8;
        }
        else
        {
            if (ebp >= SeqSize[nSeq])
            {
                if (pActor->nFrame)
                {
                    DestroyGrenade(nGrenade);
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

        int zVel = pGrenadeSprite->zvel;

        Gravity(nGrenadeSprite);
        int nMov = movesprite(nGrenadeSprite, pActor->x, pActor->y, pGrenadeSprite->zvel, pGrenadeSprite->clipdist >> 1, pGrenadeSprite->clipdist >> 1, CLIPMASK1);

        if (!nMov)
            return;

        if (nMov & 0x20000)
        {
            if (zVel)
            {
                if (SectDamage[pGrenadeSprite->sectnum] > 0)
                {
                    ExplodeGrenade(nGrenade);
                    return;
                }

                pActor->nCount = (uint8_t)totalmoves; // limit to 8bits?

                D3PlayFX(StaticSound[kSound3], nGrenadeSprite);

                pGrenadeSprite->zvel = -(zVel >> 1);

                if (pGrenadeSprite->zvel > -1280)
                {
                    D3PlayFX(StaticSound[kSound5], nGrenadeSprite);
                    pActor->nCount = 0;
                    pActor->nHealth = 0;
                    pGrenadeSprite->zvel = 0;
                    pActor->nIndex = 1;
                }
            }

            pActor->nCount = 255 - (RandomByte() * 2);
            pActor->x -= (pActor->x >> 4);
            pActor->y -= (pActor->y >> 4);
        }

        // loc_2CF60:
        if ((nMov & 0xC000) >= 0x8000)
        {
            if ((nMov & 0xC000) <= 0x8000)
            {
                BounceGrenade(nGrenade, GetWallNormal(nMov & 0x3FFF));
            }
            else if ((nMov & 0xC000) == 0xC000)
            {
                BounceGrenade(nGrenade, sprite[nMov & 0x3FFF].ang);
            }
        }

        pActor->nHealth = 0;
    }
}

void AIGrenade::RadialDamage(RunListEvent* ev)
{
    short nGrenade = RunData[ev->nRun].nObjIndex;
    auto pActor = &GrenadeList[nGrenade];
    assert(nGrenade >= 0 && nGrenade < kMaxGrenades);

    short nGrenadeSprite = pActor->nSprite;
    auto pGrenadeSprite = &sprite[nGrenadeSprite];

    if (nGrenadeSprite != nRadialSpr && !pActor->nFrame)
    {
        if (runlist_CheckRadialDamage(nGrenadeSprite) > 280)
        {
            pActor->nIndex2 = RandomSize(4) + 1;
        }
    }
}

void FuncGrenade(int nObject, int nMessage, int nDamage, int nRun)
{
    AIGrenade ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

END_PS_NS
