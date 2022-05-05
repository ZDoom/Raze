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
#include "view.h"
#include "status.h"
#include "sound.h"
#include "input.h"
#include <string.h>
#include <assert.h>
#include "v_2ddrawer.h"
#include "sequence.h"

BEGIN_PS_NS


Weapon WeaponInfo[] = {
    { kSeqSword,   { 0, 1, 3,  7, -1,  2,  4, 5, 6, 8, 9, 10 }, 0, 0, 0, true },
    { kSeqPistol,  { 0, 3, 2,  4, -1,  1,  0, 0, 0, 0, 0, 0 },  1, 0, 1, false },
    { kSeqM60,     { 0, 5, 6, 16, -1, 21,  0, 0, 0, 0, 0, 0 },  2, 0, 1, false },
    { kSeqFlamer,  { 0, 2, 5,  5,  6,  1,  0, 0, 0, 0, 0, 0 },  3, 4, 1, false },
    { kSeqGrenade, { 0, 2, 3,  4, -1,  1,  0, 0, 0, 0, 0, 0 },  4, 0, 1, true },
    { kSeqCobra,   { 0, 1, 2,  2, -1,  4,  0, 0, 0, 0, 0, 0 },  5, 0, 1, true },
    { kSeqRavolt,  { 0, 1, 2,  3, -1,  4,  0, 0, 0, 0, 0, 0 },  6, 0, 1, true },
    { kSeqRothands,{ 0, 1, 2, -1, -1, -1,  0, 0, 0, 0, 0, 0 },  7, 0, 0, true },
    { kSeqDead,    { 1, 0, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0 },  0, 1, 0, false },
    { kSeqDeadEx,  { 1, 0, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0 },  0, 1, 0, false },
    { kSeqDeadBrn, { 1, 0, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0 },  0, 1, 0, false }
};

int16_t nTemperature[kMaxPlayers];
static const uint8_t nMinAmmo[] = { 0, 24, 51, 50, 1, 0, 0 };
int isRed = 0;


void SerializeGun(FSerializer& arc)
{
    if (arc.BeginObject("gun"))
    {
        arc.Array("temperature", nTemperature, kMaxPlayers)
            ("isred", isRed)
            .EndObject();

    }
}

void RestoreMinAmmo(int nPlayer)
{
    for (int i = 0; i < kMaxWeapons; i++)
    {
        if (i == kWeaponGrenade) {
            continue;
        }

        if ((1 << i) & PlayerList[nPlayer].nPlayerWeapons)
        {
            if (nMinAmmo[i] > PlayerList[nPlayer].nAmmo[i]) {
                PlayerList[nPlayer].nAmmo[i] = nMinAmmo[i];
            }
        }
    }

    CheckClip(nPlayer);
}

void FillWeapons(int nPlayer)
{
    PlayerList[nPlayer].nPlayerWeapons = 0xFFFF; // turn on all bits

    for (int i = 0; i < kMaxWeapons; i++)
    {
        if (WeaponInfo[i].d) {
            PlayerList[nPlayer].nAmmo[i] = 300;
        }
    }

    CheckClip(nPlayer);
}

void ResetPlayerWeapons(int nPlayer)
{
    for (int i = 0; i < kMaxWeapons; i++)
    {
        PlayerList[nPlayer].nAmmo[i] = 0;
    }

    PlayerList[nPlayer].nCurrentWeapon = 0;
    PlayerList[nPlayer].nState = 0;
    PlayerList[nPlayer].nSeqSize2 = 0;

    PlayerList[nPlayer].pPlayerGrenade = nullptr;
    PlayerList[nPlayer].nPlayerWeapons = 0x1; // turn on bit 1 only
}

void InitWeapons()
{
    for (auto& p : PlayerList) p.pPlayerGrenade = nullptr;
}

void SetNewWeapon(int nPlayer, int nWeapon)
{
    if (nWeapon == kWeaponMummified)
    {
        PlayerList[nPlayer].nLastWeapon = PlayerList[nPlayer].nCurrentWeapon;
        PlayerList[nPlayer].bIsFiring = false;
        PlayerList[nPlayer].nState = 5;
        SetPlayerMummified(nPlayer, true);

        PlayerList[nPlayer].nSeqSize2 = 0;
    }
    else
    {
        if (nWeapon < 0)
        {
            PlayerList[nPlayer].nPlayerOldWeapon = PlayerList[nPlayer].nCurrentWeapon;
        }
        else if (nWeapon != kWeaponGrenade || PlayerList[nPlayer].nAmmo[kWeaponGrenade] > 0)
        {
            int nCurrentWeapon = PlayerList[nPlayer].nCurrentWeapon;

            if (nCurrentWeapon != kWeaponMummified)
            {
                if (PlayerList[nPlayer].bIsFiring || nWeapon == nCurrentWeapon) {
                    return;
                }
            }
            else
            {
                PlayerList[nPlayer].nCurrentWeapon = nWeapon;
                PlayerList[nPlayer].nSeqSize2 = 0;
            }
        }
        else {
            return;
        }
    }

    PlayerList[nPlayer].nNextWeapon = nWeapon;
}

void SetNewWeaponImmediate(int nPlayer, int nWeapon)
{
    SetNewWeapon(nPlayer, nWeapon);

    PlayerList[nPlayer].nCurrentWeapon = nWeapon;
    PlayerList[nPlayer].nNextWeapon = -1;
    PlayerList[nPlayer].nSeqSize2 = 0;
    PlayerList[nPlayer].nState = 0;
}

void SetNewWeaponIfBetter(int nPlayer, int nWeapon)
{
    if (nWeapon > PlayerList[nPlayer].nCurrentWeapon) {
        SetNewWeapon(nPlayer, nWeapon);
    }
}

void SelectNewWeapon(int nPlayer)
{
    int nWeapon = kWeaponRing; // start at the highest weapon number

    uint16_t di = PlayerList[nPlayer].nPlayerWeapons;
    uint16_t dx = 0x40; // bit 7 turned on

    while (dx)
    {
        if (di & dx)
        {
            // we have this weapon
            if (!WeaponInfo[nWeapon].d || PlayerList[nPlayer].nAmmo[WeaponInfo[nWeapon].nAmmoType])
                break;
        }

        nWeapon--;
        dx >>= 1;
    }

    if (nWeapon < 0)
        nWeapon = kWeaponSword;

    PlayerList[nPlayer].bIsFiring = false;

    SetNewWeapon(nPlayer, nWeapon);
}

void StopFiringWeapon(int nPlayer)
{
    PlayerList[nPlayer].bIsFiring = false;
}

void FireWeapon(int nPlayer)
{
    if (!PlayerList[nPlayer].bIsFiring) {
        PlayerList[nPlayer].bIsFiring = true;
    }
}

void SetWeaponStatus(int nPlayer)
{
}

uint8_t WeaponCanFire(int nPlayer)
{
    int nWeapon = PlayerList[nPlayer].nCurrentWeapon;
    auto pSector =PlayerList[nPlayer].pPlayerViewSect;

    if (!(pSector->Flag & kSectUnderwater) || WeaponInfo[nWeapon].bFireUnderwater)
    {
        int nAmmoType = WeaponInfo[nWeapon].nAmmoType;

        if (WeaponInfo[nWeapon].d <= PlayerList[nPlayer].nAmmo[nAmmoType]) {
            return true;
        }
    }

    return false;
}

// UNUSED
void ResetSwordSeqs()
{
    WeaponInfo[kWeaponSword].b[2] = 3;
    WeaponInfo[kWeaponSword].b[3] = 7;
}

Collision CheckCloseRange(int nPlayer, int *x, int *y, int *z, sectortype* *ppSector)
{
    auto pActor = PlayerList[nPlayer].pActor;

    int ang = pActor->spr.ang;
    int xVect = bcos(ang);
    int yVect = bsin(ang);

    HitInfo hit{};
    hitscan({ *x, *y, *z }, *ppSector, { xVect, yVect, 0 }, hit, CLIPMASK1);

    int ecx = bsin(150, -3);

    uint32_t yDiff = abs(hit.hitpos.Y - *y);
    uint32_t xDiff = abs(hit.hitpos.X - *x);

    uint32_t sqrtNum = xDiff * xDiff + yDiff * yDiff;

    if (sqrtNum > INT_MAX)
    {
        DPrintf(DMSG_WARNING, "%s %d: overflow\n", __func__, __LINE__);
        sqrtNum = INT_MAX;
    }
    Collision c;
    c.setNone();

    if (ksqrt(sqrtNum) >= ecx)
        return c;

    *x = hit.hitpos.X;
    *y = hit.hitpos.Y;
    *z = hit.hitpos.Z;
    *ppSector = hit.hitSector;

    if (hit.actor()) {
        c.setSprite(hit.actor());
    }
    if (hit.hitWall) {
        c.setWall(wallnum(hit.hitWall));
    }

    return c;
}

void CheckClip(int nPlayer)
{
    if (PlayerList[nPlayer].nPlayerClip <= 0)
    {
        PlayerList[nPlayer].nPlayerClip = PlayerList[nPlayer].nAmmo[kWeaponM60];

        if (PlayerList[nPlayer].nPlayerClip > 100) {
            PlayerList[nPlayer].nPlayerClip = 100;
        }
    }

    // Reset pistol's clip amount.
    PlayerList[nPlayer].nPistolClip = PlayerList[nPlayer].nAmmo[kWeaponPistol] % 6;
}

void MoveWeapons(int nPlayer)
{
    static int dword_96E22 = 0;

    int nSectFlag = PlayerList[nPlayer].pPlayerViewSect->Flag;

    if ((nSectFlag & kSectUnderwater) && (totalmoves & 1)) {
        return;
    }

    nPilotLightFrame++;

    if (nPilotLightFrame >= nPilotLightCount)
        nPilotLightFrame = 0;

    if (!PlayerList[nPlayer].bIsFiring || (nSectFlag & kSectUnderwater))
        nTemperature[nPlayer] = 0;

    auto pPlayerActor = PlayerList[nPlayer].pActor;
    int nWeapon = PlayerList[nPlayer].nCurrentWeapon;

    if (nWeapon < -1)
    {
        if (PlayerList[nPlayer].nNextWeapon != -1)
        {
            PlayerList[nPlayer].nCurrentWeapon = PlayerList[nPlayer].nNextWeapon;
            PlayerList[nPlayer].nState = 0;
            PlayerList[nPlayer].nSeqSize2 = 0;
            PlayerList[nPlayer].nNextWeapon = -1;
        }

        return;
    }

    // loc_26ACC
    int eax = PlayerList[nPlayer].nState;
    int nSeq = WeaponInfo[nWeapon].nSeq;

    int var_3C = WeaponInfo[nWeapon].b[eax] + SeqOffsets[nSeq];

    int var_1C = (PlayerList[nPlayer].nDouble > 0) + 1;

    frames = var_1C - 1;

    for (frames = var_1C; frames > 0; frames--)
    {
        seq_MoveSequence(pPlayerActor, var_3C, PlayerList[nPlayer].nSeqSize2);

        PlayerList[nPlayer].nSeqSize2++;

        dword_96E22++;
        if (dword_96E22 >= 15) {
            dword_96E22 = 0;
        }

        if (PlayerList[nPlayer].nSeqSize2 >= SeqSize[var_3C])
        {
            if (PlayerList[nPlayer].nNextWeapon == -1)
            {
                switch (PlayerList[nPlayer].nState)
                {
                    default:
                        break;

                    case 0:
                    {
                        PlayerList[nPlayer].nState = 1;
                        SetWeaponStatus(nPlayer);
                        break;
                    }
                    case 1:
                    {
                        if (PlayerList[nPlayer].bIsFiring)
                        {
                            if (!WeaponCanFire(nPlayer))
                            {
                                if (!dword_96E22) {
                                    D3PlayFX(StaticSound[4], PlayerList[nPlayer].pActor);
                                }
                            }
                            else
                            {
                                if (nWeapon == kWeaponRing)
                                {
                                    if (Ra[nPlayer].pTarget == nullptr)
                                        break;

                                    Ra[nPlayer].nAction = 0;
                                    Ra[nPlayer].nFrame  = 0;
                                    Ra[nPlayer].nState = 1;
                                }

                                PlayerList[nPlayer].nState = 2;

                                if (nWeapon == 0)
                                    break;

                                if (nWeapon == kWeaponGrenade)
                                {
                                    BuildGrenade(nPlayer);
                                    AddAmmo(nPlayer, 4, -1);
                                }
                                else if (nWeapon == kWeaponMummified)
                                {
                                    ShootStaff(nPlayer);
                                }
                            }
                        }
                        break;
                    }

                    case 2:
                    case 6:
                    case 7:
                    case 8:
                    {
                        if (nWeapon == kWeaponPistol && PlayerList[nPlayer].nPistolClip <= 0)
                        {
                            PlayerList[nPlayer].nState = 3;
                            PlayerList[nPlayer].nSeqSize2 = 0;

                            PlayerList[nPlayer].nPistolClip = min<int>(6, PlayerList[nPlayer].nAmmo[kWeaponPistol]);
                            break;
                        }
                        else if (nWeapon == kWeaponGrenade)
                        {
                            if (!PlayerList[nPlayer].bIsFiring)
                            {
                                PlayerList[nPlayer].nState = 3;
                                break;
                            }
                            else
                            {
                                PlayerList[nPlayer].nSeqSize2 = SeqSize[var_3C] - 1;
                                continue;
                            }
                        }
                        else if (nWeapon == kWeaponMummified)
                        {
                            PlayerList[nPlayer].nState = 0;
                            PlayerList[nPlayer].nCurrentWeapon = PlayerList[nPlayer].nLastWeapon;

                            nWeapon = PlayerList[nPlayer].nCurrentWeapon;

                            SetPlayerMummified(nPlayer, false);
                            break;
                        }
                        else
                        {
                            // loc_26D88:
                            if (PlayerList[nPlayer].bIsFiring && WeaponCanFire(nPlayer))
                            {
                                if (nWeapon != kWeaponM60 && nWeapon != kWeaponPistol) {
                                    PlayerList[nPlayer].nState = 3;
                                }
                            }
                            else
                            {
                                if (WeaponInfo[nWeapon].b[4] == -1)
                                {
                                    PlayerList[nPlayer].nState = 1;
                                }
                                else
                                {
                                    if (nWeapon == kWeaponFlamer && (nSectFlag & kSectUnderwater))
                                    {
                                        PlayerList[nPlayer].nState = 1;
                                    }
                                    else
                                    {
                                        PlayerList[nPlayer].nState = 4;
                                    }
                                }
                            }

                            break;
                        }
                    }

                    case 3:
                    case 9:
                    case 10:
                    case 11:
                    {
                        if (nWeapon == kWeaponMummified)
                        {
                            PlayerList[nPlayer].nCurrentWeapon = PlayerList[nPlayer].nLastWeapon;

                            nWeapon = PlayerList[nPlayer].nCurrentWeapon;

                            PlayerList[nPlayer].nState = 0;
                            break;
                        }
                        else if (nWeapon == kWeaponRing)
                        {
                            if (!WeaponInfo[nWeapon].d || PlayerList[nPlayer].nAmmo[WeaponInfo[nWeapon].nAmmoType])
                            {
                                if (!PlayerList[nPlayer].bIsFiring) {
                                    PlayerList[nPlayer].nState = 1;
                                }
                                else {
                                    break;
                                }
                            }
                            else
                            {
                                SelectNewWeapon(nPlayer);
                            }

                            Ra[nPlayer].nState = 0;
                            break;
                        }
                        else if (nWeapon == kWeaponM60)
                        {
                            CheckClip(nPlayer);
                            PlayerList[nPlayer].nState = 1;
                            break;
                        }
                        else if (nWeapon == kWeaponGrenade)
                        {
                            if (!WeaponInfo[nWeapon].d || PlayerList[nPlayer].nAmmo[WeaponInfo[nWeapon].nAmmoType])
                            {
                                PlayerList[nPlayer].nState = 0;
                                break;
                            }
                            else
                            {
                                SelectNewWeapon(nPlayer);
                                PlayerList[nPlayer].nState = 5;

                                PlayerList[nPlayer].nSeqSize2 = SeqSize[WeaponInfo[kWeaponGrenade].b[0] + SeqOffsets[nSeq]] - 1; // CHECKME
                                goto loc_flag; // FIXME
                            }
                        }
                        else
                        {
                            if (PlayerList[nPlayer].bIsFiring && WeaponCanFire(nPlayer)) {
                                PlayerList[nPlayer].nState = 2;
                                break;
                            }

                            if (WeaponInfo[nWeapon].b[4] == -1)
                            {
                                PlayerList[nPlayer].nState = 1;
                                break;
                            }

                            if (nWeapon == kWeaponFlamer && (nSectFlag & kSectUnderwater))
                            {
                                PlayerList[nPlayer].nState = 1;
                            }
                            else
                            {
                                PlayerList[nPlayer].nState = 4;
                            }
                        }
                        break;
                    }

                    case 4:
                    {
                        PlayerList[nPlayer].nState = 1;
                        break;
                    }

                    case 5:
                    {
                        PlayerList[nPlayer].nCurrentWeapon = PlayerList[nPlayer].nNextWeapon;

                        nWeapon = PlayerList[nPlayer].nCurrentWeapon;

                        PlayerList[nPlayer].nState = 0;
                        PlayerList[nPlayer].nNextWeapon = -1;

                        SetWeaponStatus(nPlayer);
                        break;
                    }
                }

                // loc_26FC5
                var_3C = SeqOffsets[WeaponInfo[nWeapon].nSeq] + WeaponInfo[nWeapon].b[PlayerList[nPlayer].nState];
                PlayerList[nPlayer].nSeqSize2 = 0;
            }
            else
            {
                if (PlayerList[nPlayer].nState == 5)
                {
                    PlayerList[nPlayer].nCurrentWeapon = PlayerList[nPlayer].nNextWeapon;

                    nWeapon = PlayerList[nPlayer].nCurrentWeapon;

                    PlayerList[nPlayer].nNextWeapon = -1;
                    PlayerList[nPlayer].nState = 0;
                }
                else
                {
                    PlayerList[nPlayer].nState = 5;
                }

                PlayerList[nPlayer].nSeqSize2 = 0;
                continue;
            }
        } // end of if (PlayerList[nPlayer].field_34 >= SeqSize[var_3C])

loc_flag:

        // loc_27001
        int nFrameFlag = seq_GetFrameFlag(var_3C, PlayerList[nPlayer].nSeqSize2);

        if (((!(nSectFlag & kSectUnderwater)) || nWeapon == kWeaponRing) && (nFrameFlag & 4))
        {
            BuildFlash(nPlayer, 512);
            AddFlash(
                pPlayerActor->sector(),
                pPlayerActor->spr.pos.X,
                pPlayerActor->spr.pos.Y,
                pPlayerActor->spr.pos.Z,
                0);
        }

        if (nFrameFlag & 0x80)
        {
            int nAction = PlayerList[nPlayer].nAction;

            int var_38 = 1;

            if (nAction < 10 || nAction > 12) {
                var_38 = 0;
            }

            if (nPlayer == nLocalPlayer) {
                obobangle = bobangle = 512;
            }

            if (nWeapon == kWeaponFlamer && (!(nSectFlag & kSectUnderwater)))
            {
                nTemperature[nPlayer]++;

                if (nTemperature[nPlayer] > 50)
                {
                    nTemperature[nPlayer] = 0;
                    PlayerList[nPlayer].nState = 4;
                    PlayerList[nPlayer].nSeqSize2 = 0;
                }
            }

            int nAmmoType = WeaponInfo[nWeapon].nAmmoType;
            int nAngle = pPlayerActor->spr.ang;
            int theX = pPlayerActor->spr.pos.X;
            int theY = pPlayerActor->spr.pos.Y;
            int theZ = pPlayerActor->spr.pos.Z;

            int ebp = bcos(nAngle) * (pPlayerActor->spr.clipdist << 3);
            int ebx = bsin(nAngle) * (pPlayerActor->spr.clipdist << 3);

            if (WeaponInfo[nWeapon].c)
            {
                int ecx;

                int theVal = (totalmoves + 101) & (WeaponInfo[nWeapon].c - 1);
                if (theVal & 1)
                    ecx = -theVal;
                else
                    ecx = theVal;

                int var_44 = (nAngle + 512) & kAngleMask;
                ebp += bcos(var_44, -11) * ecx;
                ebx += bsin(var_44, -11) * ecx;
            }

            int nHeight = (-GetActorHeight(pPlayerActor)) >> 1;

            if (nAction < 6)
            {
                nHeight -= 1792;
            }
            else
            {
                if (!var_38)
                {
                    nHeight += 1024;
                }
                else {
                    nHeight -= 2560;
                }
            }

            auto pSectorB = pPlayerActor->sector();

            switch (nWeapon)
            {
                // loc_27266:
                case kWeaponSword:
                {
                    nHeight += -PlayerList[nLocalPlayer].horizon.horiz.asq16() >> 10;

                    theZ += nHeight;

                    int var_28;

                    if (PlayerList[nPlayer].nState == 2) {
                        var_28 = 6;
                    }
                    else {
                        var_28 = 9;
                    }

                    auto cRange = CheckCloseRange(nPlayer, &theX, &theY, &theZ, &pSectorB);

                    if (cRange.type != kHitNone)
                    {
                        int nDamage = BulletInfo[kWeaponSword].nDamage;

                        if (PlayerList[nPlayer].nDouble) {
                            nDamage *= 2;
                        }

                        //if (cRange.type != kHitNone)
                        {
                            if (cRange.type == kHitWall)
                            {
                                // loc_2730E:
                                var_28 += 2;
                            }
                            else if (cRange.type == kHitSprite)
                            {
                                auto pActor2 = cRange.actor();

                                if (pActor2->spr.cstat & (CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_ONE_SIDE))
                                {
                                    var_28 += 2;
                                }
                                else if (pActor2->spr.statnum > 90 && pActor2->spr.statnum <= 199)
                                {
                                    runlist_DamageEnemy(pActor2, pPlayerActor, nDamage);

                                    if (pActor2->spr.statnum < 102) {
                                        var_28++;
                                    }
                                    else if (pActor2->spr.statnum == 102)
                                    {
                                        // loc_27370:
                                        BuildAnim(nullptr, 12, 0, theX, theY, theZ, pSectorB, 30, 0);
                                    }
                                    else if (pActor2->spr.statnum == kStatExplodeTrigger) {
                                        var_28 += 2;
                                    }
                                    else {
                                        var_28++;
                                    }
                                }
                                else
                                {
                                    // loc_27370:
                                    BuildAnim(nullptr, 12, 0, theX, theY, theZ, pSectorB, 30, 0);
                                }
                            }
                        }
                    }

                    // loc_27399:
                    PlayerList[nPlayer].nState = var_28;
                    PlayerList[nPlayer].nSeqSize2 = 0;
                    break;
                }
                case kWeaponFlamer:
                {
                    if (nSectFlag & kSectUnderwater)
                    {
                        DoBubbles(nPlayer);
                        PlayerList[nPlayer].nState = 1;
                        PlayerList[nPlayer].nSeqSize2 = 0;
                        StopActorSound(pPlayerActor);
                        break;
                    }
                    else
                    {
                        if (var_38) {
                            nHeight += 768;
                        }
                        else {
                            nHeight -= 2560;
                        }

                        // fall through to case 1 (kWeaponPistol)
                        [[fallthrough]];
                    }
                }

                case kWeaponM60:
                {
                    if (nWeapon == kWeaponM60) { // hack(?) to do fallthrough from kWeapon3 into kWeaponPistol without doing the nQuake[] change
                        nQuake[nPlayer] = 128;
                    }
                    // fall through
                    [[fallthrough]];
                }
                case kWeaponPistol:
                {
                    int h = PlayerList[nLocalPlayer].horizon.horiz.asq16() >> 14;
                    nHeight -= h;

                    DExhumedActor* target = nullptr;
                    if (sPlayerInput[nPlayer].pTarget != nullptr && Autoaim(nPlayer))
                    {
                        DExhumedActor* t = sPlayerInput[nPlayer].pTarget;
                        // only autoaim if target is in front of the player.
						assert(t->sector());
                        int angletotarget = bvectangbam(t->spr.pos.X - pPlayerActor->spr.pos.X, t->spr.pos.Y - pPlayerActor->spr.pos.Y).asbuild();
                        int anglediff = (pPlayerActor->spr.ang - angletotarget) & 2047;
                        if (anglediff < 512 || anglediff > 1536)
                        {
                            target = t;
                        }
                    }

                    BuildBullet(pPlayerActor, nAmmoType, nHeight, nAngle, target, var_1C);
                    break;
                }

                case kWeaponGrenade:
                {
                    ThrowGrenade(nPlayer, ebp, ebx, nHeight - 2560, FixedToInt(PlayerList[nLocalPlayer].horizon.horiz.asq16()));
                    break;
                }
                case kWeaponStaff:
                {
                    BuildSnake(nPlayer, nHeight);
                    nQuake[nPlayer] = 512;

                    PlayerList[nPlayer].nDamage.X -= bcos(pPlayerActor->spr.ang, 9);
                    PlayerList[nPlayer].nDamage.Y -= bsin(pPlayerActor->spr.ang, 9);
                    break;
                }
                case kWeaponRing:
                    break;

                case kWeaponMummified:
                {
                    int nDamage = BulletInfo[kWeaponMummified].nDamage;
                    if (PlayerList[nPlayer].nDouble) {
                        nDamage *= 2;
                    }

                    runlist_RadialDamageEnemy(pPlayerActor, nDamage, BulletInfo[kWeaponMummified].nRadius);
                    break;
                }
            }

            // end of switch, loc_2753E:
            if (nWeapon < kWeaponMummified)
            {
                if (nWeapon != kWeaponGrenade)
                {   
                    if (WeaponInfo[nWeapon].d) {
                        AddAmmo(nPlayer, nAmmoType, -1);
                    }

                    if (nWeapon == kWeaponM60) {
                        PlayerList[nPlayer].nPlayerClip--;
                    }
                    else if (nWeapon == kWeaponPistol) {
                        PlayerList[nPlayer].nPistolClip--;
                    }
                }

                if (!WeaponInfo[nWeapon].d || PlayerList[nPlayer].nAmmo[WeaponInfo[nWeapon].nAmmoType])
                {
                    if (nWeapon == kWeaponM60 && PlayerList[nPlayer].nPlayerClip <= 0)
                    {
                        PlayerList[nPlayer].nState = 3;
                        PlayerList[nPlayer].nSeqSize2 = 0;
                        // goto loc_27609:
                    }
                }
                else if (nWeapon != kWeaponGrenade)
                {
                    SelectNewWeapon(nPlayer);
                    // go to loc_27609:
                }
            }
        }
    }
}

void DrawWeapons(double smooth)
{
    if (bCamera) {
        return;
    }

    int nWeapon = PlayerList[nLocalPlayer].nCurrentWeapon;
    if (nWeapon < -1) {
        return;
    }
    int var_34 = PlayerList[nLocalPlayer].nState;

    int var_30 = SeqOffsets[WeaponInfo[nWeapon].nSeq];

    int var_28 = var_30 + WeaponInfo[nWeapon].b[var_34];

    int8_t nShade = initsectp->ceilingshade;

    int nDouble = PlayerList[nLocalPlayer].nDouble;
    int nPal = kPalNormal;

    if (nDouble)
    {
        if (isRed) {
            nPal = kPalRedBrite;
        }

        isRed = isRed == 0;
    }

    nPal = RemapPLU(nPal);

    double xOffset = 0, yOffset = 0;
	bool screenalign = false;

    if (cl_weaponsway)
    {
        double nBobAngle, nVal;

        if (cl_hudinterpolation)
        {
            nBobAngle = interpolatedangle(buildang(obobangle), buildang(bobangle), smooth).asbuildf();
            nVal = interpolatedvaluef(PlayerList[nLocalPlayer].ototalvel, PlayerList[nLocalPlayer].totalvel, smooth, 16) * 0.5;
        }
        else
        {
            nBobAngle = bobangle;
            nVal = PlayerList[nLocalPlayer].totalvel;
        }

        yOffset = MulScaleF(nVal, bsinf(fmod(nBobAngle, 1024.), -8), 9);

        if (var_34 == 1)
        {
            xOffset = MulScaleF(bcosf(nBobAngle, -8), nVal, 8);
        }
    }
    else
    {
        obobangle = bobangle = 512;
    }

    if (nWeapon == 3 && var_34 == 1) {
        seq_DrawPilotLightSeq(xOffset, yOffset);
    }
	else if (nWeapon == 8 || nWeapon == 9)
	{
		screenalign = true;
	}

    if (nWeapon < 0) {
        nShade = PlayerList[nLocalPlayer].pActor->spr.shade;
    }

    double const look_anghalf = PlayerList[nLocalPlayer].angle.look_anghalf(smooth);
    double const looking_arc = PlayerList[nLocalPlayer].angle.looking_arc(smooth);

    xOffset -= look_anghalf;
    yOffset += looking_arc;

    seq_DrawGunSequence(var_28, PlayerList[nLocalPlayer].nSeqSize2, xOffset, yOffset, nShade, nPal, screenalign);

    if (nWeapon != kWeaponM60)
        return;

    switch (var_34)
    {
        default:
            return;

        case 0:
        {
            int nClip = PlayerList[nLocalPlayer].nPlayerClip;

            if (nClip <= 0)
                return;

            int nSeqOffset;

            if (nClip <= 3)
            {
                nSeqOffset = var_30 + 1;
            }
            else if (nClip <= 6)
            {
                nSeqOffset = var_30 + 2;
            }
            else if (nClip <= 25)
            {
                nSeqOffset = var_30 + 3;
            }
            else //if (nClip > 25)
            {
                nSeqOffset = var_30 + 4;
            }

            seq_DrawGunSequence(nSeqOffset, PlayerList[nLocalPlayer].nSeqSize2, xOffset, yOffset, nShade, nPal);
            return;
        }
        case 1:
        {
            int nClip = PlayerList[nLocalPlayer].nPlayerClip;

            int edx = (nClip % 3) * 4;

            if (nClip <= 0) {
                return;
            }

            seq_DrawGunSequence(var_30 + 8, edx, xOffset, yOffset, nShade, nPal);

            if (nClip <= 3) {
                return;
            }

            seq_DrawGunSequence(var_30 + 9, edx, xOffset, yOffset, nShade, nPal);

            if (nClip <= 6) {
                return;
            }

            seq_DrawGunSequence(var_30 + 10, edx, xOffset, yOffset, nShade, nPal);

            if (nClip <= 25) {
                return;
            }

            seq_DrawGunSequence(var_30 + 11, edx, xOffset, yOffset, nShade, nPal);
            return;
        }
        case 2:
        {
            int nClip = PlayerList[nLocalPlayer].nPlayerClip;

            int dx = PlayerList[nLocalPlayer].nSeqSize2;

            if (nClip <= 0) {
                return;
            }

            seq_DrawGunSequence(var_30 + 8, dx, xOffset, yOffset, nShade, nPal);

            if (nClip <= 3) {
                return;
            }

            seq_DrawGunSequence(var_30 + 9, dx, xOffset, yOffset, nShade, nPal);

            if (nClip <= 6) {
                return;
            }

            seq_DrawGunSequence(var_30 + 10, dx, xOffset, yOffset, nShade, nPal);

            if (nClip <= 25) {
                return;
            }

            seq_DrawGunSequence(var_30 + 11, dx, xOffset, yOffset, nShade, nPal);
            return;
        }

        case 3:
        case 4:
            return;

        case 5:
        {
            int nClip = PlayerList[nLocalPlayer].nPlayerClip;

            int ax = PlayerList[nLocalPlayer].nSeqSize2;

            if (nClip <= 0) {
                return;
            }

            int nSeqOffset;

            if (nClip <= 3)
            {
                nSeqOffset = var_30 + 20;
            }
            else if (nClip <= 6)
            {
                nSeqOffset = var_30 + 19;
            }
            else if (nClip <= 25)
            {
                nSeqOffset = var_30 + 18;
            }
            else
            {
                nSeqOffset = var_30 + 17;
            }

            seq_DrawGunSequence(nSeqOffset, ax, xOffset, yOffset, nShade, nPal);
            return;
        }
    }
}
END_PS_NS
