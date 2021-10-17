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
    short field_0;
    short field_2;
    short nSprite;
    short field_6;
    short field_8;
    short field_A;
    short field_C;
    short field_E;
    int field_10;
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
            ("at0", w.field_0, def->field_0)
            ("at2", w.field_2, def->field_2)
            ("at6", w.field_6, def->field_6)
            ("at8", w.field_8, def->field_8)
            ("ata", w.field_A, def->field_A)
            ("atc", w.field_C, def->field_C)
            ("ate", w.field_E, def->field_E)
            ("at10", w.field_10, def->field_10)
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
    runlist_DoSubRunRec(GrenadeList[nGrenade].field_6);
    runlist_SubRunRec(GrenadeList[nGrenade].field_8);
    runlist_DoSubRunRec(sprite[GrenadeList[nGrenade].nSprite].lotag - 1);

    mydeletesprite(GrenadeList[nGrenade].nSprite);
    GrenadeList.Release(nGrenade);
}

void BounceGrenade(short nGrenade, short nAngle)
{
    GrenadeList[nGrenade].field_10 >>= 1;

    GrenadeList[nGrenade].x = bcos(nAngle, -5) * GrenadeList[nGrenade].field_10;
    GrenadeList[nGrenade].y = bsin(nAngle, -5) * GrenadeList[nGrenade].field_10;

    D3PlayFX(StaticSound[kSound3], GrenadeList[nGrenade].nSprite);
}

int ThrowGrenade(short nPlayer, int, int, int ecx, int push1)
{
    if (nPlayerGrenade[nPlayer] < 0)
        return -1;

    short nGrenade = nPlayerGrenade[nPlayer];

    short nGrenadeSprite = GrenadeList[nGrenade].nSprite;
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

        GrenadeList[nGrenade].field_10 = ((90 - GrenadeList[nGrenade].field_E) * (90 - GrenadeList[nGrenade].field_E)) + nVel;
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
        GrenadeList[nGrenade].field_10 = 0;
        pGrenadeSprite->zvel = pPlayerSprite->zvel;
    }

    GrenadeList[nGrenade].x = bcos(nAngle, -4) * GrenadeList[nGrenade].field_10;
    GrenadeList[nGrenade].y = bsin(nAngle, -4) * GrenadeList[nGrenade].field_10;

    nPlayerGrenade[nPlayer] = -1;

    return nGrenadeSprite;
}

void BuildGrenade(int nPlayer)
{
    int nGrenade = GrabGrenade();
    if (nGrenade < 0) return;

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

    GrenadeList[nGrenade].field_E = 90;
    GrenadeList[nGrenade].field_2 = 0;
    GrenadeList[nGrenade].field_0 = 16;
    GrenadeList[nGrenade].field_10 = -1;
    GrenadeList[nGrenade].nSprite = nSprite;
    GrenadeList[nGrenade].field_A = 0;
    GrenadeList[nGrenade].field_C = 0;
    GrenadeList[nGrenade].field_6 = runlist_AddRunRec(pSprite->lotag - 1, nGrenade, 0x0F0000);
    GrenadeList[nGrenade].field_8 = runlist_AddRunRec(NewRun, nGrenade, 0x0F0000);

    nGrenadePlayer[nGrenade] = nPlayer;
    nPlayerGrenade[nPlayer] = nGrenade;
}

void ExplodeGrenade(short nGrenade)
{
    int var_28, var_20;

    short nPlayer = nGrenadePlayer[nGrenade];
    int nGrenadeSprite = GrenadeList[nGrenade].nSprite;
	auto pGrenadeSprite = &sprite[nGrenadeSprite];
    short nGrenadeSect = pGrenadeSprite->sectnum;

    GrenadeList[nGrenade].field_C = 1;

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

    if (GrenadeList[nGrenade].field_10 < 0)
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

    BuildAnim(-1, var_28, 0, pGrenadeSprite->x, pGrenadeSprite->y, pGrenadeSprite->z, pGrenadeSprite->sectnum, var_20, 4);
    AddFlash(pGrenadeSprite->sectnum, pGrenadeSprite->x, pGrenadeSprite->y, pGrenadeSprite->z, 128);

    nGrenadePlayer[nGrenade] = -1;
    DestroyGrenade(nGrenade);
}

void AIGrenade::Draw(RunListEvent* ev)
{
    short nGrenade = RunData[ev->nRun].nObjIndex;
    assert(nGrenade >= 0 && nGrenade < kMaxGrenades);
    short nSeq = GrenadeList[nGrenade].field_C ? SeqOffsets[kSeqGrenBoom] : SeqOffsets[kSeqGrenRoll] + GrenadeList[nGrenade].field_A;
    seq_PlotSequence(ev->nParam, nSeq, GrenadeList[nGrenade].field_2 >> 8, 1);
}


void AIGrenade::Tick(RunListEvent* ev)
{
    short nGrenade = RunData[ev->nRun].nObjIndex;
    assert(nGrenade >= 0 && nGrenade < kMaxGrenades);

    short nGrenadeSprite = GrenadeList[nGrenade].nSprite;
    auto pGrenadeSprite = &sprite[nGrenadeSprite];
    short nSeq = GrenadeList[nGrenade].field_C ? SeqOffsets[kSeqGrenBoom] : SeqOffsets[kSeqGrenRoll] + GrenadeList[nGrenade].field_A;

    seq_MoveSequence(nGrenadeSprite, nSeq, GrenadeList[nGrenade].field_2 >> 8);
    pGrenadeSprite->picnum = seq_GetSeqPicnum2(nSeq, GrenadeList[nGrenade].field_2 >> 8);

    GrenadeList[nGrenade].field_E--;
    if (!GrenadeList[nGrenade].field_E)
    {
        short nPlayer = nGrenadePlayer[nGrenade];

        if (GrenadeList[nGrenade].field_10 < 0)
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
        if (GrenadeList[nGrenade].field_10 < 0) {
            return;
        }

        int ebp = (GrenadeList[nGrenade].field_2 + GrenadeList[nGrenade].field_0) >> 8;

        GrenadeList[nGrenade].field_2 += GrenadeList[nGrenade].field_0;

        if (ebp < 0)
        {
            GrenadeList[nGrenade].field_2 += SeqSize[nSeq] << 8;
        }
        else
        {
            if (ebp >= SeqSize[nSeq])
            {
                if (GrenadeList[nGrenade].field_C)
                {
                    DestroyGrenade(nGrenade);
                    return;
                }
                else
                {
                    GrenadeList[nGrenade].field_2 = GrenadeList[nGrenade].field_C;
                }
            }
        }

        if (GrenadeList[nGrenade].field_C) {
            return;
        }

        int zVel = pGrenadeSprite->zvel;

        Gravity(nGrenadeSprite);
        int nMov = movesprite(nGrenadeSprite, GrenadeList[nGrenade].x, GrenadeList[nGrenade].y, pGrenadeSprite->zvel, pGrenadeSprite->clipdist >> 1, pGrenadeSprite->clipdist >> 1, CLIPMASK1);

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

                GrenadeList[nGrenade].field_0 = (uint8_t)totalmoves; // limit to 8bits?

                D3PlayFX(StaticSound[kSound3], nGrenadeSprite);

                pGrenadeSprite->zvel = -(zVel >> 1);

                if (pGrenadeSprite->zvel > -1280)
                {
                    D3PlayFX(StaticSound[kSound5], nGrenadeSprite);
                    GrenadeList[nGrenade].field_0 = 0;
                    GrenadeList[nGrenade].field_2 = 0;
                    pGrenadeSprite->zvel = 0;
                    GrenadeList[nGrenade].field_A = 1;
                }
            }

            GrenadeList[nGrenade].field_0 = 255 - (RandomByte() * 2);
            GrenadeList[nGrenade].x -= (GrenadeList[nGrenade].x >> 4);
            GrenadeList[nGrenade].y -= (GrenadeList[nGrenade].y >> 4);
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

        GrenadeList[nGrenade].field_2 = 0;
    }
}

void AIGrenade::RadialDamage(RunListEvent* ev)
{
    short nGrenade = RunData[ev->nRun].nObjIndex;
    assert(nGrenade >= 0 && nGrenade < kMaxGrenades);

    short nGrenadeSprite = GrenadeList[nGrenade].nSprite;
    auto pGrenadeSprite = &sprite[nGrenadeSprite];

    if (nGrenadeSprite != nRadialSpr && !GrenadeList[nGrenade].field_C)
    {
        if (runlist_CheckRadialDamage(nGrenadeSprite) > 280)
        {
            GrenadeList[nGrenade].field_E = RandomSize(4) + 1;
        }
    }
}

void FuncGrenade(int nObject, int nMessage, int nDamage, int nRun)
{
    AIGrenade ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

END_PS_NS
