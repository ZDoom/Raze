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
#include "sound.h"
#include <string.h>
#include <assert.h>
#include "v_2ddrawer.h"
#include "sequence.h"
#include "gamehud.h"

BEGIN_PS_NS


Weapon WeaponInfo[] = {
    { "sword",   { 0, 1, 3,  7, -1,  2,  4, 5, 6, 8, 9, 10 }, 0, 0, 0, true },
    { "pistol",  { 0, 3, 2,  4, -1,  1,  0, 0, 0, 0, 0, 0 },  1, 0, 1, false },
    { "m_60",     { 0, 5, 6, 16, -1, 21,  0, 0, 0, 0, 0, 0 },  2, 0, 1, false },
    { "flamer",  { 0, 2, 5,  5,  6,  1,  0, 0, 0, 0, 0, 0 },  3, 4, 1, false },
    { "grenade", { 0, 2, 3,  4, -1,  1,  0, 0, 0, 0, 0, 0 },  4, 0, 1, true },
    { "cobra",   { 0, 1, 2,  2, -1,  4,  0, 0, 0, 0, 0, 0 },  5, 0, 1, true },
    { "ravolt",  { 0, 1, 2,  3, -1,  4,  0, 0, 0, 0, 0, 0 },  6, 0, 1, true },
    { "rothands",{ 0, 1, 2, -1, -1, -1,  0, 0, 0, 0, 0, 0 },  7, 0, 0, true },
    { "dead",    { 1, 0, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0 },  0, 1, 0, false },
    { "deadex",  { 1, 0, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0 },  0, 1, 0, false },
    { "deadbrn", { 1, 0, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0 },  0, 1, 0, false }
};

static const uint8_t nMinAmmo[] = { 0, 24, 51, 50, 1, 0, 0 };
static float lastavel;
int isRed = 0;


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SerializeGun(FSerializer& arc)
{
    if (arc.BeginObject("gun"))
    {
        arc("isred", isRed).EndObject();
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ResetPlayerWeapons(int nPlayer)
{
    for (int i = 0; i < kMaxWeapons; i++)
    {
        PlayerList[nPlayer].nAmmo[i] = 0;
    }

    PlayerList[nPlayer].nCurrentWeapon = 0;
    PlayerList[nPlayer].nState = 0;
    PlayerList[nPlayer].nWeapFrame = 0;

    PlayerList[nPlayer].pPlayerGrenade = nullptr;
    PlayerList[nPlayer].nPlayerWeapons = 0x1; // turn on bit 1 only
}

void InitWeapons()
{
    for (auto& p : PlayerList) p.pPlayerGrenade = nullptr;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SetNewWeapon(int nPlayer, int nWeapon)
{
    if (nWeapon == kWeaponMummified)
    {
        PlayerList[nPlayer].nLastWeapon = PlayerList[nPlayer].nCurrentWeapon;
        PlayerList[nPlayer].bIsFiring = false;
        PlayerList[nPlayer].nState = 5;
        SetPlayerMummified(nPlayer, true);

        PlayerList[nPlayer].nWeapFrame = 0;
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
                PlayerList[nPlayer].nWeapFrame = 0;
            }
        }
        else {
            return;
        }
    }

    PlayerList[nPlayer].nNextWeapon = nWeapon;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SetNewWeaponImmediate(int nPlayer, int nWeapon)
{
    SetNewWeapon(nPlayer, nWeapon);

    PlayerList[nPlayer].nCurrentWeapon = nWeapon;
    PlayerList[nPlayer].nNextWeapon = -1;
    PlayerList[nPlayer].nWeapFrame = 0;
    PlayerList[nPlayer].nState = 0;
}

void SetNewWeaponIfBetter(int nPlayer, int nWeapon)
{
    if (nWeapon > PlayerList[nPlayer].nCurrentWeapon) {
        SetNewWeapon(nPlayer, nWeapon);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SetWeaponStatus(int nPlayer)
{
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

Collision CheckCloseRange(int nPlayer, DVector3& pos, sectortype* *ppSector)
{
    auto pActor = PlayerList[nPlayer].GetActor();

    HitInfo hit{};
    hitscan(pos, *ppSector, DVector3(pActor->spr.Angles.Yaw.ToVector() * 1024, 0 ), hit, CLIPMASK1);

	const double ecx = 56.84; // bsin(150, -3)
	double sqrtNum = (hit.hitpos.XY() - pos.XY()).LengthSquared();

    Collision c;
    c.setNone();

    if (sqrtNum >= ecx * ecx)
        return c;

	pos = hit.hitpos;
    *ppSector = hit.hitSector;

    if (hit.actor()) {
        c.setSprite(hit.actor());
    }
    if (hit.hitWall) {
        c.setWall(wallindex(hit.hitWall));
    }

    return c;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void CheckClip(int nPlayer)
{
    const auto pPlayer = &PlayerList[nPlayer];

    if (pPlayer->nPlayerClip <= 0)
        pPlayer->nPlayerClip = min(pPlayer->nAmmo[kWeaponM60], (int16_t)100);

    if (pPlayer->nPistolClip <= 0)
        pPlayer->nPistolClip = min(pPlayer->nAmmo[kWeaponPistol], (int16_t)6);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void MoveWeapons(int nPlayer)
{
    const auto pPlayer = &PlayerList[nPlayer];

    static int dword_96E22 = 0;

    int nSectFlag = pPlayer->pPlayerViewSect->Flag;

    if ((nSectFlag & kSectUnderwater) && (totalmoves & 1)) {
        return;
    }

    nPilotLightFrame++;

    if (nPilotLightFrame >= nPilotLightCount)
        nPilotLightFrame = 0;

    if (!pPlayer->bIsFiring || (nSectFlag & kSectUnderwater))
        pPlayer->nTemperature = 0;

    auto pPlayerActor = pPlayer->GetActor();
    int nWeapon = pPlayer->nCurrentWeapon;

    if (nWeapon < -1)
    {
        if (pPlayer->nNextWeapon != -1)
        {
            pPlayer->nCurrentWeapon = pPlayer->nNextWeapon;
            pPlayer->nState = 0;
            pPlayer->nWeapFrame = 0;
            pPlayer->nNextWeapon = -1;
        }

        return;
    }

    const auto nSeqFile = WeaponInfo[nWeapon].nSeqFile;
    auto weapSeq = getSequence(nSeqFile, WeaponInfo[nWeapon].b[pPlayer->nState]);
    auto seqFrame = weapSeq->frames.Data(pPlayer->nWeapFrame);

    int var_1C = (pPlayer->nDouble > 0) + 1;
    int frames = var_1C - 1;

    for (frames = var_1C; frames > 0; frames--)
    {
        seqFrame->playSound(pPlayerActor);

        pPlayer->nWeapFrame++;

        dword_96E22++;
        if (dword_96E22 >= 15) {
            dword_96E22 = 0;
        }

        if (pPlayer->nWeapFrame >= weapSeq->frames.Size())
        {
            if (pPlayer->nNextWeapon == -1)
            {
                switch (pPlayer->nState)
                {
                    default:
                        break;

                    case 0:
                    {
                        pPlayer->nState = 1;
                        SetWeaponStatus(nPlayer);
                        break;
                    }
                    case 1:
                    {
                        if (pPlayer->bIsFiring)
                        {
                            if (!WeaponCanFire(nPlayer))
                            {
                                if (!dword_96E22) {
                                    D3PlayFX(StaticSound[4], pPlayer->GetActor());
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

                                pPlayer->nState = 2;

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
                        if (nWeapon == kWeaponPistol && pPlayer->nPistolClip <= 0)
                        {
                            pPlayer->nState = 3;
                            pPlayer->nWeapFrame = 0;

                            pPlayer->nPistolClip = min<int>(6, pPlayer->nAmmo[kWeaponPistol]);
                            break;
                        }
                        else if (nWeapon == kWeaponGrenade)
                        {
                            if (!pPlayer->bIsFiring)
                            {
                                pPlayer->nState = 3;
                                break;
                            }
                            else
                            {
                                pPlayer->nWeapFrame = weapSeq->frames.Size() - 1;
                                continue;
                            }
                        }
                        else if (nWeapon == kWeaponMummified)
                        {
                            pPlayer->nState = 0;
                            pPlayer->nCurrentWeapon = pPlayer->nLastWeapon;

                            nWeapon = pPlayer->nCurrentWeapon;

                            SetPlayerMummified(nPlayer, false);
                            break;
                        }
                        else
                        {
                            // loc_26D88:
                            if (pPlayer->bIsFiring && WeaponCanFire(nPlayer))
                            {
                                if (nWeapon != kWeaponM60 && nWeapon != kWeaponPistol) {
                                    pPlayer->nState = 3;
                                }
                            }
                            else
                            {
                                if (WeaponInfo[nWeapon].b[4] == -1)
                                {
                                    pPlayer->nState = 1;
                                }
                                else
                                {
                                    if (nWeapon == kWeaponFlamer && (nSectFlag & kSectUnderwater))
                                    {
                                        pPlayer->nState = 1;
                                    }
                                    else
                                    {
                                        pPlayer->nState = 4;
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
                            pPlayer->nCurrentWeapon = pPlayer->nLastWeapon;

                            nWeapon = pPlayer->nCurrentWeapon;

                            pPlayer->nState = 0;
                            break;
                        }
                        else if (nWeapon == kWeaponRing)
                        {
                            if (!WeaponInfo[nWeapon].d || pPlayer->nAmmo[WeaponInfo[nWeapon].nAmmoType])
                            {
                                if (!pPlayer->bIsFiring) {
                                    pPlayer->nState = 1;
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
                            pPlayer->nState = 1;
                            break;
                        }
                        else if (nWeapon == kWeaponGrenade)
                        {
                            if (!WeaponInfo[nWeapon].d || pPlayer->nAmmo[WeaponInfo[nWeapon].nAmmoType])
                            {
                                pPlayer->nState = 0;
                                break;
                            }
                            else
                            {
                                SelectNewWeapon(nPlayer);
                                pPlayer->nState = 5;

                                pPlayer->nWeapFrame = getSequence(nSeqFile, WeaponInfo[kWeaponGrenade].b[0])->frames.Size() - 1; // CHECKME
                                goto loc_flag; // FIXME
                            }
                        }
                        else
                        {
                            if (pPlayer->bIsFiring && WeaponCanFire(nPlayer)) {
                                pPlayer->nState = 2;
                                break;
                            }

                            if (WeaponInfo[nWeapon].b[4] == -1)
                            {
                                pPlayer->nState = 1;
                                break;
                            }

                            if (nWeapon == kWeaponFlamer && (nSectFlag & kSectUnderwater))
                            {
                                pPlayer->nState = 1;
                            }
                            else
                            {
                                pPlayer->nState = 4;
                            }
                        }
                        break;
                    }

                    case 4:
                    {
                        pPlayer->nState = 1;
                        break;
                    }

                    case 5:
                    {
                        pPlayer->nCurrentWeapon = pPlayer->nNextWeapon;

                        nWeapon = pPlayer->nCurrentWeapon;

                        pPlayer->nState = 0;
                        pPlayer->nNextWeapon = -1;

                        SetWeaponStatus(nPlayer);
                        break;
                    }
                }

                weapSeq = getSequence(WeaponInfo[nWeapon].nSeqFile, WeaponInfo[nWeapon].b[pPlayer->nState]);
                pPlayer->nWeapFrame = 0;
            }
            else
            {
                if (pPlayer->nState == 5)
                {
                    pPlayer->nCurrentWeapon = pPlayer->nNextWeapon;

                    nWeapon = pPlayer->nCurrentWeapon;

                    pPlayer->nNextWeapon = -1;
                    pPlayer->nState = 0;
                }
                else
                {
                    pPlayer->nState = 5;
                }

                pPlayer->nWeapFrame = 0;
                continue;
            }
        }

loc_flag:
        seqFrame = weapSeq->frames.Data(pPlayer->nWeapFrame);

        if (((!(nSectFlag & kSectUnderwater)) || nWeapon == kWeaponRing) && (seqFrame->flags & 4))
        {
            BuildFlash(nPlayer, 512);
            AddFlash(
                pPlayerActor->sector(),
                pPlayerActor->spr.pos,
                0);
        }

        if (seqFrame->flags & 0x80)
        {
            int nAction = pPlayerActor->nAction;

            int var_38 = 1;

            if (nAction < 10 || nAction > 12) {
                var_38 = 0;
            }

            if (nPlayer == nLocalPlayer) {
                pPlayer->nPrevWeapBob = pPlayer->nWeapBob = 512;
            }

            if (nWeapon == kWeaponFlamer && (!(nSectFlag & kSectUnderwater)))
            {
                pPlayer->nTemperature++;

                if (pPlayer->nTemperature > 50)
                {
                    pPlayer->nTemperature = 0;
                    pPlayer->nState = 4;
                    pPlayer->nWeapFrame = 0;
                }
            }

            int nAmmoType = WeaponInfo[nWeapon].nAmmoType;
            DAngle nAngle = pPlayerActor->spr.Angles.Yaw;
			auto thePos = pPlayerActor->spr.pos;

            double nHeight = GetActorHeight(pPlayerActor) * -0.5;

            if (nAction < 6)
            {
                nHeight -= 7;
            }
            else
            {
                if (!var_38)
                {
                    nHeight += 4;
                }
                else {
                    nHeight -= 10;
                }
            }

            auto pSectorB = pPlayerActor->sector();

            switch (nWeapon)
            {
                // loc_27266:
                case kWeaponSword:
                {
                    nHeight += pPlayerActor->spr.Angles.Pitch.Tan() * 32.;

                    thePos.Z += nHeight;

                    int newState;

                    if (pPlayer->nState == 2) {
                        newState = 6;
                    }
                    else {
                        newState = 9;
                    }

                    auto cRange = CheckCloseRange(nPlayer, thePos, &pSectorB);

                    if (cRange.type != kHitNone)
                    {
                        int nDamage = BulletInfo[kWeaponSword].nDamage;

                        if (pPlayer->nDouble) {
                            nDamage *= 2;
                        }

                        //if (cRange.type != kHitNone)
                        {
                            if (cRange.type == kHitWall)
                            {
                                // loc_2730E:
                                newState += 2;
                            }
                            else if (cRange.type == kHitSprite)
                            {
                                auto pActor2 = cRange.actor();

                                if (pActor2->spr.cstat & (CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_ONE_SIDE))
                                {
                                    newState += 2;
                                }
                                else if (pActor2->spr.statnum > 90 && pActor2->spr.statnum <= 199)
                                {
                                    runlist_DamageEnemy(pActor2, pPlayerActor, nDamage);

                                    if (pActor2->spr.statnum < 102) {
                                        newState++;
                                    }
                                    else if (pActor2->spr.statnum == 102)
                                    {
                                        // loc_27370:
                                        BuildAnim(nullptr, "poof", 0, thePos, pSectorB, 0.46875, 0);
                                    }
                                    else if (pActor2->spr.statnum == kStatExplodeTrigger) {
                                        newState += 2;
                                    }
                                    else {
                                        newState++;
                                    }
                                }
                                else
                                {
                                    // loc_27370:
                                    BuildAnim(nullptr, "poof", 0, thePos, pSectorB, 0.46875, 0);
                                }
                            }
                        }
                    }

                    // loc_27399:
                    pPlayer->nState = newState;
                    pPlayer->nWeapFrame = 0;
                    break;
                }
                case kWeaponFlamer:
                {
                    if (nSectFlag & kSectUnderwater)
                    {
                        DoBubbles(nPlayer);
                        pPlayer->nState = 1;
                        pPlayer->nWeapFrame = 0;
                        StopActorSound(pPlayerActor);
                        break;
                    }
                    else
                    {
                        if (var_38) {
                            nHeight += 3;
                        }
                        else {
                            nHeight -= 10;
                        }

                        // fall through to case 1 (kWeaponPistol)
                        [[fallthrough]];
                    }
                }

                case kWeaponM60:
                {
                    if (nWeapon == kWeaponM60) { // hack(?) to do fallthrough from kWeapon3 into kWeaponPistol without doing the nQuake[] change
                        pPlayer->nQuake = 0.5;
                    }
                    // fall through
                    [[fallthrough]];
                }
                case kWeaponPistol:
                {
                    double h = pPlayerActor->spr.Angles.Pitch.Tan() * 2.;
                    nHeight += h;

                    DExhumedActor* target = nullptr;
                    if (pPlayer->pTarget != nullptr && Autoaim(nPlayer))
                    {
                        DExhumedActor* t = pPlayer->pTarget;
                        // only autoaim if target is in front of the player.
						assert(t->sector());
                        DAngle angletotarget = (t->spr.pos - pPlayerActor->spr.pos).Angle();
                        DAngle anglediff = absangle(pPlayerActor->spr.Angles.Yaw, angletotarget);
                        if (anglediff < DAngle90)
                        {
                            target = t;
                            h = 0;
                        }
                    }

                    BuildBullet(pPlayerActor, nAmmoType, nHeight, nAngle, target, var_1C, -int(h * zworldtoint));
                    break;
                }

                case kWeaponGrenade:
                {
                    ThrowGrenade(nPlayer, nHeight - 10, pPlayerActor->spr.Angles.Pitch.Tan());
                    break;
                }
                case kWeaponStaff:
                {
                    BuildSnake(nPlayer, nHeight);
                    pPlayer->nQuake = 2.;

                    pPlayer->nThrust -= pPlayerActor->spr.Angles.Yaw.ToVector() * 2;
                    break;
                }
                case kWeaponRing:
                    break;

                case kWeaponMummified:
                {
                    int nDamage = BulletInfo[kWeaponMummified].nDamage;
                    if (pPlayer->nDouble) {
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
                        pPlayer->nPlayerClip--;
                    }
                    else if (nWeapon == kWeaponPistol) {
                        pPlayer->nPistolClip--;
                    }
                }

                if (!WeaponInfo[nWeapon].d || pPlayer->nAmmo[WeaponInfo[nWeapon].nAmmoType])
                {
                    if (nWeapon == kWeaponM60 && pPlayer->nPlayerClip <= 0)
                    {
                        pPlayer->nState = 3;
                        pPlayer->nWeapFrame = 0;
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DrawWeapons(ExhumedPlayer* const pPlayer, double interpfrac)
{
    const auto pPlayerActor = pPlayer->GetActor();
    const int nWeapon = pPlayer->nCurrentWeapon;

    if (bCamera || nWeapon < -1)
        return;

    const int nState = pPlayer->nState;
    const int nShade = nWeapon < 0 ? pPlayerActor->spr.shade : pPlayerActor->sector()->ceilingshade;
    const double nAlpha = pPlayer->nInvisible ? 0.3 : 1;
    const auto weapSeqs = getFileSeqs(WeaponInfo[nWeapon].nSeqFile);

    int nPal = kPalNormal;

    if (pPlayer->nDouble)
    {
        if (isRed) {
            nPal = kPalRedBrite;
        }

        isRed = isRed == 0;
    }

    nPal = RemapPLU(nPal);

    const auto weaponOffsets = pPlayer->Angles.getWeaponOffsets(interpfrac);
    const auto nAngle = weaponOffsets.second;
    double xPos = 160 + weaponOffsets.first.X;
    double yPos = 100 + weaponOffsets.first.Y;

    double nFlameAng = interpolatedvalue(lastavel, pPlayer->input.avel, interpfrac);
    lastavel = pPlayer->input.avel;

    if (cl_weaponsway)
    {
        double nBobAngle = pPlayer->nWeapBob, nTotalVel = pPlayer->totalvel;

        if (cl_hudinterpolation)
        {
            nBobAngle = interpolatedvalue<double>(pPlayer->nPrevWeapBob, pPlayer->nWeapBob, interpfrac);
            nTotalVel = interpolatedvalue<double>(pPlayer->ototalvel, pPlayer->totalvel, interpfrac);
        }

        const auto xBob = nTotalVel * BobVal(nBobAngle + 512) * (nState == 1);
        const auto yBob = nTotalVel * fabs(BobVal(nBobAngle));

        nFlameAng += xBob;
        xPos += xBob * 2.;
        yPos += yBob;
    }
    else
    {
        pPlayer->nPrevWeapBob = pPlayer->nWeapBob = 512;
    }

    int nStat = false;

    if (nWeapon == 3 && nState == 1)
    {
        if (!(pPlayer->pPlayerViewSect->Flag & kSectUnderwater))
        {
            seq_DrawPilotLightSeq(xPos, yPos, nFlameAng);
        }
    }
	else if (nWeapon == 8 || nWeapon == 9)
	{
		nStat |= RS_ALIGN_R;
	}

    int nSeqOffset = WeaponInfo[nWeapon].b[nState];
    int nFrame = pPlayer->nWeapFrame;

    seq_DrawGunSequence(weapSeqs->Data(nSeqOffset)->frames[nFrame], xPos, yPos, nShade, nPal, nAngle, nAlpha, nStat);

    int nClip = pPlayer->nPlayerClip;

    if (nWeapon != kWeaponM60 || nClip <= 0)
        return;

    switch (nState)
    {
        case 0:
        case 5:
        {
            static constexpr int offsetTable[2][4] = {
                {1,   2,  3,  4},
                {20, 19, 18, 17}
            };

            const auto& offsets = offsetTable[nState == 5];

            if (nClip <= 3)
            {
                nSeqOffset = offsets[0];
            }
            else if (nClip <= 6)
            {
                nSeqOffset = offsets[1];
            }
            else if (nClip <= 25)
            {
                nSeqOffset = offsets[2];
            }
            else //if (nClip > 25)
            {
                nSeqOffset = offsets[3];
            }

            seq_DrawGunSequence(weapSeqs->Data(nSeqOffset)->frames[nFrame], xPos, yPos, nShade, nPal, nAngle, nAlpha);
            return;
        }
        case 1:
        case 2:
        {
            if (nState == 1)
                nFrame = (nClip % 3) * 4;

            seq_DrawGunSequence(weapSeqs->Data(8)->frames[nFrame], xPos, yPos, nShade, nPal, nAngle, nAlpha);

            if (nClip <= 3) {
                return;
            }

            seq_DrawGunSequence(weapSeqs->Data(9)->frames[nFrame], xPos, yPos, nShade, nPal, nAngle, nAlpha);

            if (nClip <= 6) {
                return;
            }

            seq_DrawGunSequence(weapSeqs->Data(10)->frames[nFrame], xPos, yPos, nShade, nPal, nAngle, nAlpha);

            if (nClip <= 25) {
                return;
            }

            seq_DrawGunSequence(weapSeqs->Data(11)->frames[nFrame], xPos, yPos, nShade, nPal, nAngle, nAlpha);
            return;
        }
        default:
        {
            return;
        }
    }
}
END_PS_NS
