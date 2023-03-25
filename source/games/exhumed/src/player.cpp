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
#include "player.h"
#include "aistuff.h"
#include "exhumed.h"
#include "names.h"
#include "engine.h"
#include "sequence.h"
#include "view.h"
#include "status.h"
#include "sound.h"
#include "sound.h"
#include "buildtiles.h"
#include "gstrings.h"
#include "gamestate.h"
#include "mapinfo.h"
#include "automap.h"
#include "interpolate.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

BEGIN_PS_NS

void doPlayerItemPickups(Player* const pPlayer);

static actionSeq PlayerSeq[] = {
    {18,  0}, {0,   0}, {9,   0}, {27,  0}, {63,  0},
    {72,  0}, {54,  0}, {45,  0}, {54,  0}, {81,  0},
    {90,  0}, {99,  0}, {108, 0}, {8,   0}, {0,   0},
    {139, 0}, {117, 1}, {119, 1}, {120, 1}, {121, 1},
    {122, 1}
};

static const uint8_t nHeightTemplate[] = { 0, 0, 0, 0, 0, 0, 7, 7, 7, 9, 9, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0 };

static constexpr double nActionEyeLevel[] = {
    -55.0,  -55.0,  -55.0,  -55.0,  -55.0,  -55.0,  -32.5,
    -32.5,  -32.5,  -32.5,  -32.5,  -32.5,  -32.5,  -55.0,
    -55.0,  -55.0,  -55.0,  -55.0,  -55.0,  -55.0,  -55.0
};

static const uint16_t nGunLotag[] = { 52, 53, 54, 55, 56, 57 };
static const uint16_t nGunPicnum[] = { 57, 488, 490, 491, 878, 899, 3455 };

static const int16_t nItemText[] = {
    -1, -1, -1, -1, -1, -1, 18, 20, 19, 13, -1, 10, 1, 0, 2, -1, 3,
    -1, 4, 5, 9, 6, 7, 8, -1, 11, -1, 13, 12, 14, 15, -1, 16, 17,
    -1, -1, -1, 21, 22, -1, -1, -1, -1, -1, -1, 23, 24, 25, 26, 27,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

extern int nStatusSeqOffset;
int obobangle = 0, bobangle  = 0;
int nLocalPlayer = 0;
Player PlayerList[kMaxPlayers];
TObjPtr<DExhumedActor*> nNetStartSprite[kMaxPlayers] = { };
double nStandHeight;
int PlayerCount;
int nNetStartSprites;
int nCurStartSprite;


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

size_t MarkPlayers()
{
    for (auto& p : PlayerList)
    {
        GC::Mark(p.pActor);
        GC::Mark(p.pDoppleSprite);
        GC::Mark(p.pPlayerFloorSprite);
        GC::Mark(p.pPlayerGrenade);
        GC::Mark(p.pTarget);
    }
    GC::MarkArray(nNetStartSprite, kMaxPlayers);
    return 6 * kMaxPlayers;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SetSavePoint(int nPlayer, const DVector3& pos, sectortype* pSector, DAngle nAngle)
{
    PlayerList[nPlayer].sPlayerSave.pos = pos;
    PlayerList[nPlayer].sPlayerSave.pSector = pSector;
    PlayerList[nPlayer].sPlayerSave.nAngle = nAngle;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitPlayer()
{
    for (int i = 0; i < kMaxPlayers; i++) {
        PlayerList[i].pActor = nullptr;
        PlayerList[i].Angles = {};
        PlayerList[i].pPlayerPushSect = nullptr;
        PlayerList[i].pPlayerViewSect = nullptr;
    }
}

void InitPlayerKeys(int nPlayer)
{
    PlayerList[nPlayer].keys = 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitPlayerInventory(int nPlayer)
{
    memset(&PlayerList[nPlayer], 0, sizeof(Player));

    PlayerList[nPlayer].nItem = -1;
    PlayerList[nPlayer].nPlayerSwear = 4;

    ResetPlayerWeapons(nPlayer);

    PlayerList[nPlayer].nLives = kDefaultLives;

    PlayerList[nPlayer].pActor = nullptr;
    PlayerList[nPlayer].Angles = {};
    PlayerList[nPlayer].nRun = -1;

    PlayerList[nPlayer].nPistolClip = 6;
    PlayerList[nPlayer].nPlayerClip = 0;

    PlayerList[nPlayer].nCurrentWeapon = 0;

    if (nPlayer == nLocalPlayer) {
        automapMode = am_off;
    }

    PlayerList[nPlayer].nPlayerScore = 0;

    auto pixels = GetRawPixels(tileGetTextureID(kTile3571 + nPlayer));

    PlayerList[nPlayer].nPlayerColor = pixels[tileWidth(nPlayer + kTile3571) * tileHeight(nPlayer + kTile3571) / 2];
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int GetPlayerFromActor(DExhumedActor* pActor)
{
    return RunData[pActor->spr.intowner].nObjIndex;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void RestartPlayer(int nPlayer)
{
	auto plr = &PlayerList[nPlayer];
    auto pActor = plr->pActor;
    DExhumedActor* pDopSprite = plr->pDoppleSprite;

    DExhumedActor* floorsprt;

    plr->nPlayer = nPlayer;

	if (pActor)
	{
        runlist_DoSubRunRec(pActor->spr.intowner);
		runlist_FreeRun(pActor->spr.lotag - 1);

		ChangeActorStat(pActor, 0);

        plr->pActor = nullptr;
        plr->Angles = {};

		DExhumedActor* pFloorSprite = plr->pPlayerFloorSprite;
		if (pFloorSprite != nullptr) {
			DeleteActor(pFloorSprite);
		}

		if (pDopSprite)
		{
			runlist_DoSubRunRec(pDopSprite->spr.intowner);
			runlist_FreeRun(pDopSprite->spr.lotag - 1);
            DeleteActor(pDopSprite);
		}
	}

    pActor = GrabBody();

	ChangeActorSect(pActor, plr->sPlayerSave.pSector);
	ChangeActorStat(pActor, 100);

	auto pDActor = insertActor(pActor->sector(), 100);
	plr->pDoppleSprite = pDActor;

	if (nTotalPlayers > 1)
	{
        DExhumedActor* nNStartSprite = nNetStartSprite[nCurStartSprite];
		nCurStartSprite++;

		if (nCurStartSprite >= nNetStartSprites) {
			nCurStartSprite = 0;
		}

		pActor->spr.pos = nNStartSprite->spr.pos;
		ChangeActorSect(pActor, nNStartSprite->sector());
        pActor->spr.Angles.Yaw = nNStartSprite->spr.Angles.Yaw;

		floorsprt = insertActor(pActor->sector(), 0);

		floorsprt->spr.pos = pActor->spr.pos;
		floorsprt->spr.scale = DVector2(1, 1);
		floorsprt->spr.cstat = CSTAT_SPRITE_ALIGNMENT_FLOOR;
		floorsprt->spr.picnum = nPlayer + kTile3571;
	}
	else
	{
        pActor->spr.pos.XY() = plr->sPlayerSave.pos.XY();
		pActor->spr.pos.Z = plr->sPlayerSave.pSector->floorz;
		pActor->spr.Angles.Yaw = plr->sPlayerSave.nAngle;

		floorsprt = nullptr;
	}

    pActor->backuploc();

	plr->pPlayerFloorSprite = floorsprt;

	pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
	pActor->spr.shade = -12;
	pActor->clipdist = 14.5;
	pActor->spr.pal = 0;
	pActor->spr.scale = DVector2(0.625, 0.625);
	pActor->spr.xoffset = 0;
	pActor->spr.yoffset = 0;
	pActor->spr.picnum = seq_GetSeqPicnum(kSeqJoe, 18, 0);

	pActor->vel.X = 0;
	pActor->vel.Y = 0;
	pActor->vel.Z = 0;

	nStandHeight = GetActorHeight(pActor);

	pActor->spr.hitag = 0;
	pActor->spr.extra = -1;
	pActor->spr.lotag = runlist_HeadRun() + 1;

    pDActor->spr.pos = pActor->spr.pos;
	pDActor->spr.scale = pActor->spr.scale;
	pDActor->spr.xoffset = 0;
	pDActor->spr.yoffset = 0;
	pDActor->spr.shade = pActor->spr.shade;
	pDActor->spr.Angles.Yaw = pActor->spr.Angles.Yaw;
	pDActor->spr.cstat = pActor->spr.cstat;

	pDActor->spr.lotag = runlist_HeadRun() + 1;

	plr->nAction = 0;
	plr->nHealth = 800; // TODO - define

	if (nNetPlayerCount) {
		plr->nHealth = 1600; // TODO - define
	}

	plr->nSeqSize = 0;
	plr->pActor = pActor;
    plr->Angles = {};
    plr->Angles.initialize(plr->pActor);
	plr->bIsMummified = false;

	if (plr->invincibility >= 0) {
		plr->invincibility = 0;
	}

	plr->nTorch = 0;
	plr->nMaskAmount = 0;

	SetTorch(nPlayer, 0);

	plr->nInvisible = 0;

	plr->bIsFiring = 0;
	plr->nSeqSize2 = 0;
	plr->pPlayerViewSect = plr->sPlayerSave.pSector;
	plr->nState = 0;

	plr->nDouble = 0;

	plr->nSeq = kSeqJoe;

	plr->nPlayerPushSound = -1;

	plr->nNextWeapon = -1;

	if (plr->nCurrentWeapon == 7) {
		plr->nCurrentWeapon = plr->nLastWeapon;
	}

	plr->nLastWeapon = 0;
	plr->nAir = 100;

	if (!(currentLevel->gameflags & LEVEL_EX_MULTI))
	{
		RestoreMinAmmo(nPlayer);
	}
	else
	{
		ResetPlayerWeapons(nPlayer);
		plr->nMagic = 0;
	}

	plr->pPlayerGrenade = nullptr;
	pActor->oviewzoffset = pActor->viewzoffset = -55.;
	plr->dVertPan = 0;

	nTemperature[nPlayer] = 0;

    plr->nThrust.Zero();

	plr->nDestVertPan = plr->pActor->PrevAngles.Pitch = plr->pActor->spr.Angles.Pitch = nullAngle;
	plr->nBreathTimer = 90;

	plr->nTauntTimer = RandomSize(3) + 3;

	pDActor->spr.intowner = runlist_AddRunRec(pDActor->spr.lotag - 1, nPlayer, 0xA0000);
	pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, nPlayer, 0xA0000);

	if (plr->nRun < 0) {
		plr->nRun = runlist_AddRunRec(NewRun, nPlayer, 0xA0000);
	}

	BuildRa(nPlayer);

	if (nPlayer == nLocalPlayer)
	{
		RestoreGreenPal();
	}

	plr->ototalvel = plr->totalvel = 0;

    PlayerList[nPlayer].nCurrentItem = -1;

	plr->nDeathType = 0;
	plr->nQuake = 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int GrabPlayer()
{
    if (PlayerCount >= kMaxPlayers) {
        return -1;
    }

    return PlayerCount++;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void StartDeathSeq(int nPlayer, int nVal)
{
    FreeRa(nPlayer);

    auto pActor = PlayerList[nPlayer].pActor;
    PlayerList[nPlayer].nHealth = 0;

    int nLotag = pActor->sector()->lotag;

    if (nLotag > 0) {
        runlist_SignalRun(nLotag - 1, nPlayer, &ExhumedAI::EnterSector);
    }

    if (PlayerList[nPlayer].pPlayerGrenade)
    {
        ThrowGrenade(nPlayer, 0, -10000);
    }
    else
    {
        if (nNetPlayerCount)
        {
            int nWeapon = PlayerList[nPlayer].nCurrentWeapon;

            if (nWeapon > kWeaponSword && nWeapon <= kWeaponRing)
            {
                auto pSector = pActor->sector();
                if (pSector->pBelow != nullptr) {
                    pSector = pSector->pBelow;
                }

                auto pGunActor = GrabBodyGunSprite();
                ChangeActorSect(pGunActor, pSector);

                pGunActor->spr.pos = { pActor->spr.pos.X, pActor->spr.pos.Y, pSector->floorz - 2 };

                ChangeActorStat(pGunActor, nGunLotag[nWeapon] + 900);

                pGunActor->spr.picnum = nGunPicnum[nWeapon];

                BuildItemAnim(pGunActor);
            }
        }
    }

    PlayerList[nPlayer].bIsFiring = false;

    PlayerList[nPlayer].pActor->PrevAngles.Pitch = PlayerList[nPlayer].pActor->spr.Angles.Pitch = nullAngle;
    pActor->oviewzoffset = pActor->viewzoffset = -55;
    PlayerList[nPlayer].nInvisible = 0;
    PlayerList[nPlayer].dVertPan = 15;

    pActor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;

    SetNewWeaponImmediate(nPlayer, -2);

    if (pActor->sector()->Damage <= 0)
    {
        PlayerList[nPlayer].nDeathType = nVal;
    }
    else
    {
        PlayerList[nPlayer].nDeathType = 2;
    }

    nVal *= 2;

    if (nVal || !(pActor->sector()->Flag & kSectUnderwater))
    {
        PlayerList[nPlayer].nAction = nVal + 17;
    }
    else {
        PlayerList[nPlayer].nAction = 16;
    }

    PlayerList[nPlayer].nSeqSize = 0;

    pActor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;

    if (nTotalPlayers == 1)
    {
        if (!(currentLevel->gameflags & LEVEL_EX_TRAINING)) { // if not on the training level
            PlayerList[nPlayer].nLives--;
        }

        if (PlayerList[nPlayer].nLives < 0) {
            PlayerList[nPlayer].nLives = 0;
        }
    }

    PlayerList[nPlayer].ototalvel = PlayerList[nPlayer].totalvel = 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int AddAmmo(int nPlayer, int nWeapon, int nAmmoAmount)
{
    if (!nAmmoAmount) {
        nAmmoAmount = 1;
    }

    int nCurAmmo = PlayerList[nPlayer].nAmmo[nWeapon];

    if (nCurAmmo >= 300 && nAmmoAmount > 0) {
        return 0;
    }

    nAmmoAmount = nCurAmmo + nAmmoAmount;
    if (nAmmoAmount > 300) {
        nAmmoAmount = 300;
    }

    PlayerList[nPlayer].nAmmo[nWeapon] = nAmmoAmount;

    if (nWeapon == 1)
    {
        if (!PlayerList[nPlayer].nPistolClip) {
            PlayerList[nPlayer].nPistolClip = 6;
        }
    }

    return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SetPlayerMummified(int nPlayer, int bIsMummified)
{
    DExhumedActor* pActor = PlayerList[nPlayer].pActor;

    pActor->vel.Y = 0;
    pActor->vel.X = 0;

    PlayerList[nPlayer].bIsMummified = bIsMummified;

    if (bIsMummified)
    {
        PlayerList[nPlayer].nAction = 13;
        PlayerList[nPlayer].nSeq = kSeqMummy;
    }
    else
    {
        PlayerList[nPlayer].nAction = 0;
        PlayerList[nPlayer].nSeq = kSeqJoe;
    }

    PlayerList[nPlayer].nSeqSize = 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ShootStaff(int nPlayer)
{
    PlayerList[nPlayer].nAction = 15;
    PlayerList[nPlayer].nSeqSize = 0;
    PlayerList[nPlayer].nSeq = kSeqJoe;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PlayAlert(const char *str)
{
    StatusMessage(300, str);
    PlayLocalSound(StaticSound[kSound63], 0);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void pickupMessage(int no)
{
    no = nItemText[no];
    if (no != -1)
    {
        FStringf label("TXT_EX_PICKUP%d", no + 1);
        auto str = GStrings[label];
        if (str) Printf(PRINT_NOTIFY, "%s\n", str);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIPlayer::Draw(RunListEvent* ev)
{
    int nPlayer = RunData[ev->nRun].nObjIndex;
    assert(nPlayer >= 0 && nPlayer < kMaxPlayers);
    int nAction = PlayerList[nPlayer].nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[PlayerList[nPlayer].nSeq] + PlayerSeq[nAction].a, PlayerList[nPlayer].nSeqSize, PlayerSeq[nAction].b);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIPlayer::RadialDamage(RunListEvent* ev)
{
    int nPlayer = RunData[ev->nRun].nObjIndex;
    assert(nPlayer >= 0 && nPlayer < kMaxPlayers);

    auto pPlayerActor = PlayerList[nPlayer].pActor;

    if (PlayerList[nPlayer].nHealth <= 0)
    {
        return;
    }

    ev->nDamage = runlist_CheckRadialDamage(pPlayerActor);
    Damage(ev);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIPlayer::Damage(RunListEvent* ev)
{
    int nDamage = ev->nDamage;
    int nPlayer = RunData[ev->nRun].nObjIndex;
    auto pPlayerActor = PlayerList[nPlayer].pActor;
    int nAction = PlayerList[nPlayer].nAction;
    DExhumedActor* pDopple = PlayerList[nPlayer].pDoppleSprite;

    if (!nDamage) {
        return;
    }

    DExhumedActor* pActor2 = (!ev->isRadialEvent()) ? ev->pOtherActor : ev->pRadialActor->pTarget.Get();

    // ok continue case 0x80000 as normal, loc_1C57C
    if (!PlayerList[nPlayer].nHealth) {
        return;
    }

    if (!PlayerList[nPlayer].invincibility)
    {
        PlayerList[nPlayer].nHealth -= nDamage;
        if (nPlayer == nLocalPlayer)
        {
            TintPalette(nDamage, 0, 0);
        }
    }

    if (PlayerList[nPlayer].nHealth > 0)
    {
        if (nDamage > 40 || (totalmoves & 0xF) < 2)
        {
            if (PlayerList[nPlayer].invincibility) {
                return;
            }

            if (pPlayerActor->sector()->Flag & kSectUnderwater)
            {
                if (nAction != 12)
                {
                    PlayerList[nPlayer].nSeqSize = 0;
                    PlayerList[nPlayer].nAction = 12;
                    return;
                }
            }
            else
            {
                if (nAction != 4)
                {
                    PlayerList[nPlayer].nSeqSize = 0;
                    PlayerList[nPlayer].nAction = 4;

                    if (pActor2)
                    {
                        PlayerList[nPlayer].nPlayerSwear--;
                        if (PlayerList[nPlayer].nPlayerSwear <= 0)
                        {
                            D3PlayFX(StaticSound[kSound52], pDopple);
                            PlayerList[nPlayer].nPlayerSwear = RandomSize(3) + 4;
                        }
                    }
                }
            }
        }

        return;
    }
    else
    {
        // player has died
        if (pActor2 && pActor2->spr.statnum == 100)
        {
            int nPlayer2 = GetPlayerFromActor(pActor2);

            if (nPlayer2 == nPlayer) // player caused their own death
            {
                PlayerList[nPlayer].nPlayerScore--;
            }
            else
            {
                PlayerList[nPlayer].nPlayerScore++;
            }
        }
        else if (pActor2 == nullptr)
        {
            PlayerList[nPlayer].nPlayerScore--;
        }

        if (ev->isRadialEvent())
        {
            for (int i = 122; i <= 131; i++)
            {
                BuildCreatureChunk(pPlayerActor, seq_GetSeqPicnum(kSeqJoe, i, 0));
            }

            StartDeathSeq(nPlayer, 1);
        }
        else
        {
            StartDeathSeq(nPlayer, 0);
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void updatePlayerTarget(Player* const pPlayer)
{
    const auto pRa = &Ra[pPlayer->nPlayer];
    const auto pPlayerActor = pPlayer->pActor;
    const auto nAngVect = (-pPlayerActor->spr.Angles.Yaw).ToVector();

    DExhumedActor* bestTarget = nullptr;
    double bestclose = 20;
    double bestside = 30000;

    ExhumedSpriteIterator it;
    while (const auto itActor = it.Next())
    {
        const bool validstatnum = (itActor->spr.statnum > 0) && (itActor->spr.statnum < 150);
        const bool validsprcstat = itActor->spr.cstat & CSTAT_SPRITE_BLOCK_ALL;

        if (validstatnum && validsprcstat && itActor != pPlayerActor)
        {
            const DVector2 delta = itActor->spr.pos.XY() - pPlayerActor->spr.pos.XY();
            const double fwd = abs((nAngVect.X * delta.Y) + (delta.X * nAngVect.Y));
            const double side = abs((nAngVect.X * delta.X) - (delta.Y * nAngVect.Y));

            if (!side)
                continue;

            const double close = fwd * 32 / side;
            if (side < 1000 / 16. && side < bestside && close < 10)
            {
                bestTarget = itActor;
                bestclose = close;
                bestside = side;
            }
            else if (side < 30000 / 16.)
            {
                const double t = bestclose - close;
                if (t > 3 || (side < bestside && abs(t) < 5))
                {
                    bestTarget = itActor;
                    bestclose = close;
                    bestside = side;
                }
            }
        }
    }

    if (bestTarget)
    {
        if (pPlayer->nPlayer == nLocalPlayer) nCreepyTimer = kCreepyCount;

        if (!cansee(pPlayerActor->spr.pos, pPlayerActor->sector(), bestTarget->spr.pos.plusZ(-GetActorHeight(bestTarget)), bestTarget->sector()))
        {
            bestTarget = nullptr;
        }
    }

    pPlayer->pTarget = pRa->pTarget = bestTarget;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void updatePlayerVelocity(Player* const pPlayer)
{
    const auto pInput = &pPlayer->input;

    if (pPlayer->nHealth > 0)
    {
        const auto inputvect = DVector2(pInput->fvel, pInput->svel).Rotated(pPlayer->pActor->spr.Angles.Yaw) * 0.375;

        for (int i = 0; i < 4; i++)
        {
            pPlayer->vel += inputvect;
            pPlayer->vel *= 0.953125;
        }
    }

    pPlayer->pActor->vel.XY() = pPlayer->vel;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void updatePlayerInventory(Player* const pPlayer)
{
    if (const auto invDir = !!(pPlayer->input.actions & SB_INVNEXT) - !!(pPlayer->input.actions & SB_INVPREV))
    {
        int nItem = pPlayer->nItem;

        int i;
        for (i = 6; i > 0; i--)
        {
            nItem += invDir;
            if (nItem < 0) nItem = 5;
            else if (nItem == 6) nItem = 0;

            if (pPlayer->items[nItem] != 0)
                break;
        }

        if (i > 0) pPlayer->nItem = nItem;
    }

    if ((pPlayer->input.actions & SB_INVUSE) && pPlayer->nItem != -1)
    {
        pPlayer->input.setItemUsed(pPlayer->nItem);
    }

    for (int i = 0; i < 6; i++)
    {
        if (pPlayer->input.isItemUsed(i))
        {
            pPlayer->input.clearItemUsed(i);
            if (pPlayer->items[i] > 0 && nItemMagic[i] <= pPlayer->nMagic)
            {
                pPlayer->nCurrentItem = i;
                break;
            }
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void updatePlayerWeapon(Player* const pPlayer)
{
    const auto currWeap = pPlayer->nCurrentWeapon;
    auto weap2 = pPlayer->input.getNewWeapon();

    if (const auto weapDir = (weap2 == WeaponSel_Next) - (weap2 == WeaponSel_Prev))
    {
        auto wrapFwd = weapDir > 0 && currWeap == 6;
        auto wrapBck = weapDir < 0 && currWeap == 0;
        auto newWeap = wrapFwd ? 0 : wrapBck ? 6 : (currWeap + weapDir);
        auto hasWeap = pPlayer->nPlayerWeapons & (1 << newWeap);

        while (newWeap && (!hasWeap || (hasWeap && !pPlayer->nAmmo[newWeap])))
        {
            newWeap += weapDir;
            if (newWeap > 6) newWeap = 0;
            hasWeap = pPlayer->nPlayerWeapons & (1 << newWeap);
        }

        pPlayer->input.setNewWeapon(newWeap + 1);
    }
    else if (weap2 == WeaponSel_Alt)
    {
        // todo
    }

    // make weapon selection persist until it gets used up.
    if (weap2 <= 0 || weap2 > 7)
        pPlayer->input.setNewWeapon(pPlayer->input.getNewWeapon());
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPlayerCounters(Player* const pPlayer)
{
    const auto pPlayerActor = pPlayer->pActor;

    if (pPlayer->nCurrentItem > -1)
    {
        UseItem(pPlayer->nPlayer, pPlayer->nCurrentItem);
        pPlayer->nCurrentItem = -1;
    }

    if (pPlayer->nTorch > 0)
    {
        pPlayer->nTorch--;

        if (pPlayer->nTorch == 0)
        {
            SetTorch(pPlayer->nPlayer, 0);
        }
        else if (pPlayer->nPlayer != nLocalPlayer)
        {
            nFlashDepth = 5;
            AddFlash(pPlayerActor->sector(), pPlayerActor->spr.pos, 0);
        }
    }

    if (pPlayer->nDouble > 0)
    {
        pPlayer->nDouble--;

        if (pPlayer->nDouble == 150 && pPlayer->nPlayer == nLocalPlayer)
        {
            PlayAlert(GStrings("TXT_EX_WEAPONEX"));
        }
    }

    if (pPlayer->nInvisible > 0)
    {
        pPlayer->nInvisible--;

        if (pPlayer->nInvisible == 0)
        {
            pPlayerActor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE; // set visible

            if (pPlayer->pPlayerFloorSprite) 
            {
                pPlayer->pPlayerFloorSprite->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE; // set visible
            }
        }
        else if (pPlayer->nInvisible == 150 && pPlayer->nPlayer == nLocalPlayer)
        {
            PlayAlert(GStrings("TXT_EX_INVISEX"));
        }
    }

    if (pPlayer->invincibility > 0)
    {
        pPlayer->invincibility--;

        if (pPlayer->invincibility == 150 && pPlayer->nPlayer == nLocalPlayer)
        {
            PlayAlert(GStrings("TXT_EX_INVINCEX"));
        }
    }

    if (pPlayer->nMaskAmount > 0 && pPlayer->nHealth > 0)
    {
        pPlayer->nMaskAmount--;

        if (pPlayer->nMaskAmount == 150 && pPlayer->nPlayer == nLocalPlayer)
        {
            PlayAlert(GStrings("TXT_EX_MASKEX"));
        }
    }

    if (pPlayer->nQuake != 0)
    {
        pPlayer->nQuake = -pPlayer->nQuake;

        if (pPlayer->nQuake > 0)
        {
            pPlayer->nQuake -= 2.;

            if (pPlayer->nQuake < 0)
            {
                pPlayer->nQuake = 0;
            }
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPlayerUnderwater(Player* const pPlayer)
{
    const auto pPlayerActor = pPlayer->pActor;
    const bool bUnderwater = pPlayer->pPlayerViewSect->Flag & kSectUnderwater;

    if (!pPlayer->invincibility)
    {
        pPlayer->nBreathTimer--;

        if (pPlayer->nBreathTimer <= 0)
        {
            pPlayer->nBreathTimer = 90;

            if (bUnderwater)
            {
                if (pPlayer->nMaskAmount > 0)
                {
                    D3PlayFX(StaticSound[kSound30], pPlayerActor);
                    pPlayer->nAir = 100;
                }
                else
                {
                    pPlayer->nAir -= 25;

                    if (pPlayer->nAir > 0)
                    {
                        D3PlayFX(StaticSound[kSound25], pPlayerActor);
                    }
                    else
                    {
                        pPlayer->nHealth += (pPlayer->nAir << 2);

                        if (pPlayer->nHealth <= 0)
                        {
                            pPlayer->nHealth = 0;
                            StartDeathSeq(pPlayer->nPlayer, 0);
                        }

                        pPlayer->nAir = 0;
                        D3PlayFX(StaticSound[(pPlayer->nHealth < 300) ? kSound79 : kSound19], pPlayerActor);
                    }
                }

                DoBubbles(pPlayer->nPlayer);
            }
        }
    }

    if (bUnderwater)
    {
        if (pPlayer->nTorch > 0)
        {
            pPlayer->nTorch = 0;
            SetTorch(pPlayer->nPlayer, 0);
        }
    }
    else
    {
        const auto pTmpSect = pPlayerActor->sector();

        if (pPlayer->totalvel > 25 && pPlayerActor->spr.pos.Z > pTmpSect->floorz)
        {
            if (pTmpSect->Depth && !pTmpSect->Speed && !pTmpSect->Damage)
            {
                D3PlayFX(StaticSound[kSound42], pPlayerActor);
            }
        }

        // Checked and confirmed.
        if (bUnderwater)
        {
            if (pPlayer->nAir < 50)
                D3PlayFX(StaticSound[kSound14], pPlayerActor);

            pPlayer->nBreathTimer = 1;
        }

        pPlayer->nBreathTimer--;

        if (pPlayer->nBreathTimer <= 0)
            pPlayer->nBreathTimer = 90;

        if (pPlayer->nAir < 100)
            pPlayer->nAir = 100;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPlayerRamses(Player* const pPlayer)
{
    setForcedSyncInput(pPlayer->nPlayer);

    if (nTotalPlayers <= 1)
    {
        const auto pPlayerActor = pPlayer->pActor;
        pPlayerActor->spr.Angles = DRotator(nullAngle, (pSpiritSprite->spr.pos.XY() - pPlayerActor->spr.pos.XY()).Angle(), nullAngle);
        pPlayerActor->backupang();
        pPlayerActor->vel.Zero();
        pPlayer->vel.Zero();

        if (nFreeze < 1)
        {
            nFreeze = 1;
            StopAllSounds();
            StopLocalSound();
            InitSpiritHead();

            pPlayerActor->spr.Angles.Pitch = currentLevel->ex_ramses_horiz;
            pPlayer->nDestVertPan = nullAngle;
        }
    }
    else
    {
        LevelFinished();
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPlayerGravity(DExhumedActor* const pPlayerActor)
{
    // Z vel is modified within Gravity() and needs backing up.
    const double zVel = pPlayerActor->vel.Z;
    Gravity(pPlayerActor);

    if (pPlayerActor->vel.Z >= 6500/256. && zVel < 6500 / 256.)
    {
        D3PlayFX(StaticSound[kSound17], pPlayerActor);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void updatePlayerFloorActor(Player* const pPlayer)
{
    if (nTotalPlayers > 1)
    {
        if (DExhumedActor* const pFloorActor = pPlayer->pPlayerFloorSprite)
        {
            const auto pPlayerActor = pPlayer->pActor;
            pFloorActor->spr.pos.XY() = pPlayerActor->spr.pos.XY();
            pFloorActor->spr.pos.Z = pPlayerActor->sector()->floorz;

            if (pFloorActor->sector() != pPlayerActor->sector())
            {
                ChangeActorSect(pFloorActor, pPlayerActor->sector());
            }
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void updatePlayerDoppleActor(Player* const pPlayer)
{
    const auto pPlayerActor = pPlayer->pActor;
    DExhumedActor* const pDopple = pPlayer->pDoppleSprite;
    pDopple->spr.pos = pPlayerActor->spr.pos;

    if (pPlayerActor->sector()->pAbove != nullptr)
    {
        pDopple->spr.Angles.Yaw = pPlayerActor->spr.Angles.Yaw;
        ChangeActorSect(pDopple, pPlayerActor->sector()->pAbove);
        pDopple->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    }
    else
    {
        pDopple->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void updatePlayerViewSector(Player* const pPlayer, const Collision& nMove, const DVector3& spr_pos, const DVector3& spr_vel, const bool bUnderwater)
{
    const auto pPlayerActor = pPlayer->pActor;
    const double EyeZ = pPlayerActor->getOffsetZ() + pPlayer->nQuake;
    auto pViewSect = pPlayerActor->sector();

    while (1)
    {
        double nCeilZ = pViewSect->ceilingz;

        if (EyeZ >= nCeilZ)
            break;

        if (pViewSect->pAbove == nullptr)
            break;

        pViewSect = pViewSect->pAbove;
    }

    // Do underwater sector check
    if (bUnderwater && pViewSect != pPlayerActor->sector() && nMove.type == kHitWall)
    {
        auto pos = pPlayerActor->spr.pos;
        ChangeActorSect(pPlayerActor, pViewSect);

        double fz = pViewSect->floorz - 20;
        pPlayerActor->spr.pos = DVector3(spr_pos.XY(), fz);

        auto coll = movesprite(pPlayerActor, spr_vel.XY(), 0, 0, CLIPMASK0);
        if (coll.type == kHitWall)
        {
            ChangeActorSect(pPlayerActor, pPlayerActor->sector());
            pPlayerActor->spr.pos = pos;
        }
        else
        {
            pPlayerActor->spr.pos.Z = fz-1;
            D3PlayFX(StaticSound[kSound42], pPlayerActor);
        }
    }

    pPlayer->pPlayerViewSect = pViewSect;

    if (nLocalPlayer == pPlayer->nPlayer)
    {
        pLocalEyeSect = pPlayer->pPlayerViewSect;
        CheckAmbience(pLocalEyeSect);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPlayerFloorDamage(Player* const pPlayer)
{
    const auto pPlayerActor = pPlayer->pActor;
    pPlayer->nThrust /= 2;

    if (pPlayer->nPlayer == nLocalPlayer && abs(pPlayerActor->vel.Z) > 2)
        pPlayer->nDestVertPan = nullAngle;

    if (pPlayerActor->vel.Z >= 6500 / 256.)
    {
        pPlayerActor->vel.XY() *= 0.25;

        runlist_DamageEnemy(pPlayerActor, nullptr, int(((pPlayerActor->vel.Z * 256) - 6500) * (1. / 128.)) + 10);

        if (pPlayer->nHealth <= 0)
        {
            pPlayerActor->vel.X = 0;
            pPlayerActor->vel.Y = 0;

            StopActorSound(pPlayerActor);
            PlayFXAtXYZ(StaticSound[kSoundJonFDie], pPlayerActor->spr.pos, CHANF_NONE, 1); // CHECKME
        }
        else
        {
            D3PlayFX(StaticSound[kSound27] | 0x2000, pPlayerActor);
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPlayerMovingBlocks(Player* const pPlayer, const Collision& nMove, const DVector3& spr_pos, const DVector3& spr_vel, sectortype* const spr_sect)
{
    const auto pPlayerActor = pPlayer->pActor;
    sectortype* sect;
    DAngle nNormal = nullAngle;

    if (nMove.type == kHitSector)
    {
        sect = nMove.hitSector;
        // Hm... Normal calculation here was broken.
    }
    else //if (nMove.type == kHitWall)
    {
        sect = nMove.hitWall->nextSector();
        nNormal = nMove.hitWall->normalAngle();
    }

    // moving blocks - move this to a separate function!
    if (sect != nullptr)
    {
        const auto nDiff = absangle(nNormal, pPlayerActor->spr.Angles.Yaw + DAngle180);

        if ((sect->hitag == 45) && bTouchFloor && nDiff <= DAngle45)
        {
            pPlayer->pPlayerPushSect = sect;

            DVector2 vel = pPlayer->vel;
            auto nMyAngle = vel.Angle().Normalized360();

            setsectinterpolate(sect);
            MoveSector(sect, nMyAngle, vel);

            if (pPlayer->nPlayerPushSound == -1)
            {
                const int nBlock = pPlayer->pPlayerPushSect->extra;
                pPlayer->nPlayerPushSound = nBlock;
                DExhumedActor* pBlockActor = sBlockInfo[nBlock].pActor;

                D3PlayFX(StaticSound[kSound23], pBlockActor, 0x4000);
            }
            else
            {
                pPlayerActor->spr.pos = spr_pos;
                ChangeActorSect(pPlayerActor, spr_sect);
            }

            movesprite(pPlayerActor, vel, spr_vel.Z, -20, CLIPMASK0);
        }
        else if (pPlayer->nPlayerPushSound != -1)
        {
            if (pPlayer->pPlayerPushSect != nullptr)
            {
                StopActorSound(sBlockInfo[pPlayer->pPlayerPushSect->extra].pActor);
            }

            pPlayer->nPlayerPushSound = -1;
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool doPlayerMovement(Player* const pPlayer)
{
    const auto pPlayerActor = pPlayer->pActor;
    const auto spr_vel = DVector3(pPlayerActor->vel.XY() * (pPlayer->bIsMummified ? 0.5 : 1.), pPlayerActor->vel.Z);
    const auto spr_pos = pPlayerActor->spr.pos;
    const auto spr_sect = pPlayerActor->sector();

    if (pPlayerActor->vel.Z > 32)
        pPlayerActor->vel.Z = 32;

    Collision nMove;
    nMove.setNone();

    if (bSlipMode)
    {
        SetActor(pPlayerActor, pPlayerActor->spr.pos + spr_vel.XY());
        pPlayerActor->spr.pos.Z = pPlayerActor->sector()->floorz;
    }
    else
    {
        nMove = movesprite(pPlayerActor, spr_vel.XY(), spr_vel.Z, -20, CLIPMASK0);

        auto pPlayerSect = pPlayerActor->sector();
        pushmove(pPlayerActor->spr.pos, &pPlayerSect, pPlayerActor->clipdist, 20, -20, CLIPMASK0);

        if (pPlayerSect != pPlayerActor->sector())
            ChangeActorSect(pPlayerActor, pPlayerSect);
    }

    if (inside(pPlayerActor->spr.pos.X, pPlayerActor->spr.pos.Y, pPlayerActor->sector()) != 1)
    {
        ChangeActorSect(pPlayerActor, spr_sect);
        pPlayerActor->spr.pos.XY() = spr_pos.XY();
    }

    const bool bUnderwater = pPlayerActor->sector()->Flag & kSectUnderwater;

    if (bUnderwater)
        pPlayer->nThrust /= 2;

    // Trigger Ramses?
    if ((pPlayerActor->sector()->Flag & 0x8000) && bTouchFloor)
        return false;

    if (nMove.type || nMove.exbits)
    {
        if (bTouchFloor)
        {
            doPlayerFloorDamage(pPlayer);
        }

        if (nMove.type == kHitSector || nMove.type == kHitWall)
        {
            doPlayerMovingBlocks(pPlayer, nMove, spr_pos, spr_vel, spr_sect);
        }
    }

    pPlayer->nDestVertPan = maphoriz((pPlayerActor->spr.pos.Z - spr_pos.Z) * 2.);
    pPlayer->ototalvel = pPlayer->totalvel;
    pPlayer->totalvel = int((spr_pos.XY() - pPlayerActor->spr.pos.XY()).Length() * worldtoint);

    updatePlayerViewSector(pPlayer, nMove, spr_pos, spr_vel, bUnderwater);

    pPlayer->nPlayerD = (pPlayerActor->spr.pos - spr_pos);

    return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPlayerRunlistSignals(Player* const pPlayer, sectortype* const pStartSect)
{
    const auto pPlayerActor = pPlayer->pActor;
    const auto pPlayerSect = pPlayerActor->sector();

    if (bTouchFloor && pPlayerSect->lotag > 0)
        runlist_SignalRun(pPlayerSect->lotag - 1, pPlayer->nPlayer, &ExhumedAI::TouchFloor);

    if (pStartSect != pPlayerSect)
    {
        if (pStartSect->lotag > 0)
            runlist_SignalRun(pStartSect->lotag - 1, pPlayer->nPlayer, &ExhumedAI::EnterSector);

        if (pPlayerSect->lotag > 0)
            runlist_SignalRun(pPlayerSect->lotag - 1, pPlayer->nPlayer, &ExhumedAI::LeaveSector);
    }

    if (!pPlayer->bIsMummified)
    {
        if (pPlayer->input.actions & SB_OPEN)
        {
            pPlayer->input.actions &= ~SB_OPEN;

            // neartag finds the nearest sector, wall, and sprite which has its hitag and/or lotag set to a value.
            HitInfo near;
            neartag(pPlayerActor->spr.pos, pPlayerSect, pPlayerActor->spr.Angles.Yaw, near, 128., NT_Hitag | NT_NoSpriteCheck);

            int tag;
            if (near.hitWall != nullptr && (tag = near.hitWall->lotag) > 0)
                runlist_SignalRun(tag - 1, pPlayer->nPlayer, &ExhumedAI::Use);

            if (near.hitSector != nullptr && (tag = near.hitSector->lotag) > 0)
                runlist_SignalRun(tag - 1, pPlayer->nPlayer, &ExhumedAI::Use);
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void updatePlayerAction(Player* const pPlayer)
{
    const auto pPlayerActor = pPlayer->pActor;
    const auto pPlayerSect = pPlayerActor->sector();
    const bool bUnderwater = pPlayerSect->Flag & kSectUnderwater;
    int nextAction = pPlayer->nAction;

    if (!pPlayer->bIsMummified)
    {
        if (pPlayer->input.actions & SB_JUMP)
        {
            if (bUnderwater)
            {
                pPlayerActor->vel.Z = -8;
                nextAction = 10;
            }
            else if (bTouchFloor && (pPlayer->nAction < 6 || pPlayer->nAction > 8))
            {
                pPlayerActor->vel.Z = -14;
                nextAction = 3;
            }
        }
        else if (pPlayer->input.actions & SB_CROUCH)
        {
            if (bUnderwater)
            {
                pPlayerActor->vel.Z = 8;
                nextAction = 10;
            }
            else
            {
                if (pPlayerActor->viewzoffset < -32.5)
                {
                    pPlayerActor->viewzoffset += ((-32.5 - pPlayerActor->viewzoffset) * 0.5);
                }

                nextAction = 7 - (pPlayer->totalvel < 1);
            }
        }
        else
        {
            if (pPlayer->nHealth > 0)
            {
                pPlayerActor->viewzoffset += (nActionEyeLevel[pPlayer->nAction] - pPlayerActor->viewzoffset) * 0.5;

                if (bUnderwater)
                {
                    nextAction = 10 - (pPlayer->totalvel <= 1);
                }
                else if (nStandHeight > (pPlayerSect->floorz - pPlayerSect->ceilingz))
                {
                    // CHECKME - confirm branching in this area is OK
                    // CHECKME - are we finished with 'nSector' variable at this point? if so, maybe set it to pPlayerActor->sector() so we can make this code a bit neater. Don't assume pPlayerActor->sector() == nSector here!!
                    nextAction = 7 - (pPlayer->totalvel < 1);
                }
                else
                {
                    const auto totalvel = pPlayer->totalvel;
                    nextAction = (totalvel <= 1) ? 0 : (totalvel <= 30) ? 2 : 1;
                }
            }

            if (pPlayer->input.actions & SB_FIRE) // was var_38
            {
                pPlayer->bIsFiring = true;

                if (bUnderwater)
                {
                    nextAction = 11;
                }
                else if (nextAction != 2 && nextAction != 1)
                {
                    nextAction = 5;
                }
            }
            else
            {
                pPlayer->bIsFiring = false;
            }
        }

        // Handle player pressing number keys to change weapon
        if (auto newWeap = pPlayer->input.getNewWeapon())
        {
            if (pPlayer->nPlayerWeapons & (1 << (newWeap--)))
            {
                SetNewWeapon(pPlayer->nPlayer, newWeap);
            }
        }
    }
    else // player is mummified
    {
        if (pPlayer->input.actions & SB_FIRE)
        {
            pPlayer->bIsFiring = true;
        }

        if (pPlayer->nAction != 15)
        {
            nextAction = 14 - (pPlayer->totalvel <= 1);
        }
    }

    if (nextAction != pPlayer->nAction && pPlayer->nAction != 4)
    {
        pPlayer->nAction = nextAction;
        pPlayer->nSeqSize = 0;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPlayerYaw(Player* const pPlayer)
{
    const auto pInput = &pPlayer->input;

    if (SyncInput())
    {
        pPlayer->pActor->spr.Angles.Yaw += DAngle::fromDeg(pInput->avel);
    }

    pPlayer->Angles.doYawKeys(pInput);
    pPlayer->Angles.doViewYaw(pInput);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPlayerPitch(Player* const pPlayer)
{
    const auto pPlayerActor = pPlayer->pActor;
    const auto pInput = &pPlayer->input;

    if (SyncInput())
    {
        pPlayerActor->spr.Angles.Pitch += DAngle::fromDeg(pInput->horz);
    }

    pPlayer->Angles.doPitchKeys(pInput);

    if (cl_slopetilting)
    {
        const double nVertPan = deltaangle(pPlayer->Angles.ViewAngles.Pitch, pPlayer->nDestVertPan).Tan() * 32.;
        pPlayer->Angles.ViewAngles.Pitch += maphoriz(abs(nVertPan) >= 4 ? Sgn(nVertPan) * 4. : nVertPan * 2.);
    }
    else
    {
        pPlayer->Angles.ViewAngles.Pitch = nullAngle;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool doPlayerDeathRestart(Player* const pPlayer)
{
    if (pPlayer->input.actions & SB_OPEN)
    {
        pPlayer->input.actions &= ~SB_OPEN;

        if (pPlayer->nAction >= 16)
        {
            if (pPlayer->nPlayer == nLocalPlayer)
            {
                StopAllSounds();
                StopLocalSound();
                GrabPalette();
            }

            pPlayer->nCurrentWeapon = pPlayer->nPlayerOldWeapon;

            if (pPlayer->nLives && nNetTime)
            {
                if (pPlayer->nAction != 20)
                {
                    const auto pPlayerActor = pPlayer->pActor;
                    pPlayerActor->spr.picnum = seq_GetSeqPicnum(kSeqJoe, 120, 0);
                    pPlayerActor->spr.cstat = 0;
                    pPlayerActor->spr.pos.Z = pPlayerActor->sector()->floorz;
                }

                // will invalidate nPlayerSprite
                RestartPlayer(pPlayer->nPlayer);
            }
            else
            {
                DoGameOverScene(currentLevel->levelNumber == 20);
                return false;
            }
        }
    }

    return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPlayerActionSequence(Player* const pPlayer)
{
    const auto pPlayerActor = pPlayer->pActor;
    const auto nSeq = SeqOffsets[pPlayer->nSeq] + PlayerSeq[pPlayer->nAction].a;

    seq_MoveSequence(pPlayerActor, nSeq, pPlayer->nSeqSize);
    pPlayer->nSeqSize++;

    if (pPlayer->nSeqSize >= SeqSize[nSeq])
    {
        pPlayer->nSeqSize = 0;

        switch (pPlayer->nAction)
        {
        default:
            break;
        case 3:
            pPlayer->nSeqSize = SeqSize[nSeq] - 1;
            break;
        case 4:
            pPlayer->nAction = 0;
            break;
        case 16:
            pPlayer->nSeqSize = SeqSize[nSeq] - 1;

            if (pPlayerActor->spr.pos.Z < pPlayerActor->sector()->floorz) 
                pPlayerActor->spr.pos.Z++;

            if (!RandomSize(5))
            {
                sectortype* mouthSect;
                const auto pos = WheresMyMouth(pPlayer->nPlayer, &mouthSect);
                BuildAnim(nullptr, 71, 0, DVector3(pos.XY(), pPlayerActor->spr.pos.Z + 15), mouthSect, 1.171875, 128);
            }
            break;
        case 17:
            pPlayer->nAction = 18;
            break;
        case 19:
            pPlayerActor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
            pPlayer->nAction = 20;
            break;
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPlayerDeathPitch(Player* const pPlayer)
{
    const auto pPlayerActor = pPlayer->pActor;
    pPlayer->nThrust.Zero();

    if (pPlayerActor->viewzoffset >= -11)
    {
        pPlayerActor->viewzoffset = -11;
        pPlayer->dVertPan = 0;
    }
    else
    {
        if (pPlayerActor->spr.Angles.Pitch.Sgn() > 0)
        {
            pPlayerActor->spr.Angles.Pitch = nullAngle;
            pPlayerActor->viewzoffset -= pPlayer->dVertPan;
        }
        else
        {
            pPlayerActor->spr.Angles.Pitch -= maphoriz(pPlayer->dVertPan);

            if (pPlayerActor->spr.Angles.Pitch.Degrees() <= -38)
            {
                pPlayerActor->spr.Angles.Pitch = DAngle::fromDeg(-37.72);
            }
            else if (pPlayerActor->spr.Angles.Pitch.Sgn() >= 0)
            {
                if (!(pPlayerActor->sector()->Flag & kSectUnderwater))
                {
                    SetNewWeapon(pPlayer->nPlayer, pPlayer->nDeathType + 8);
                }
            }

            pPlayer->dVertPan--;
        }
    }
}

//---------------------------------------------------------------------------
//
// this function is (no longer) pure spaghetti madness... :)
//
//---------------------------------------------------------------------------

void AIPlayer::Tick(RunListEvent* ev)
{
    const int nPlayer = RunData[ev->nRun].nObjIndex;
    assert(nPlayer >= 0 && nPlayer < kMaxPlayers);

    const auto pPlayer = &PlayerList[nPlayer];
    const auto pPlayerActor = pPlayer->pActor;
    const auto pStartSect = pPlayerActor->sector();

    pPlayerActor->spr.picnum = seq_GetSeqPicnum(pPlayer->nSeq, PlayerSeq[nHeightTemplate[pPlayer->nAction]].a, pPlayer->nSeqSize);
    pPlayer->pDoppleSprite->spr.picnum = pPlayerActor->spr.picnum;

    doPlayerCounters(pPlayer);
    doPlayerGravity(pPlayerActor);

    if (pPlayer->nHealth > 0)
    {
        updatePlayerVelocity(pPlayer);

        if (!doPlayerMovement(pPlayer))
        {
            doPlayerRamses(pPlayer);
            return;
        }

        updatePlayerTarget(pPlayer);
        doPlayerUnderwater(pPlayer);
        updatePlayerFloorActor(pPlayer);
        updatePlayerInventory(pPlayer);
        updatePlayerWeapon(pPlayer);
        doPlayerItemPickups(pPlayer);
        doPlayerRunlistSignals(pPlayer, pStartSect);
        updatePlayerAction(pPlayer);
        doPlayerPitch(pPlayer);
        doPlayerYaw(pPlayer);
    }
    else
    {
        setForcedSyncInput(nPlayer);
        doPlayerDeathPitch(pPlayer);

        if (!doPlayerDeathRestart(pPlayer))
        {
            return;
        }
    }

    doPlayerActionSequence(pPlayer);
    updatePlayerDoppleActor(pPlayer);
    MoveWeapons(nPlayer);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, Player& w, Player* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("health", w.nHealth)
            ("at2", w.nSeqSize)
            ("action", w.nAction)
            ("sprite", w.pActor)
            ("mummy", w.bIsMummified)
            ("invincible", w.invincibility)
            ("air", w.nAir)
            ("seq", w.nSeq)
            ("item", w.nItem)
            ("maskamount", w.nMaskAmount)
            ("keys", w.keys)
            ("magic", w.nMagic)
            .Array("items", w.items, countof(w.items))
            .Array("ammo", w.nAmmo, countof(w.nAmmo))
            ("weapon", w.nCurrentWeapon)
            ("isfiring", w.bIsFiring)
            ("field3f", w.nSeqSize2)
            ("field38", w.nNextWeapon)
            ("field3a", w.nState)
            ("field3c", w.nLastWeapon)
            ("seq", w.nSeq)
            ("horizon", w.Angles)
            ("angle", w.Angles)
            ("lives", w.nLives)
            ("double", w.nDouble)
            ("invisible", w.nInvisible)
            ("torch", w.nTorch)
            ("breathtimer", w.nBreathTimer)
            ("playerswear", w.nPlayerSwear)
            ("pushsect", w.pPlayerPushSect)
            ("deathtype", w.nDeathType)
            ("score", w.nPlayerScore)
            ("color", w.nPlayerColor)
            ("dx", w.nPlayerD.X)
            ("dy", w.nPlayerD.Y)
            ("pistolclip", w.nPistolClip)
            ("thrustx", w.nThrust.X)
            ("thrusty", w.nThrust.Y)
            ("dopplesprite", w.pDoppleSprite)
            ("oldweapon", w.nPlayerOldWeapon)
            ("clip", w.nPlayerClip)
            ("pushsound", w.nPlayerPushSound)
            ("taunttimer", w.nTauntTimer)
            ("weapons", w.nPlayerWeapons)
            ("viewsect", w.pPlayerViewSect)
            ("floorspr", w.pPlayerFloorSprite)
            ("save", w.sPlayerSave)
            ("totalvel", w.totalvel)
            ("grenade", w.pPlayerGrenade)

            .EndObject();
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerSave& w, PlayerSave* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("pos", w.pos)
            ("sector", w.pSector)
            ("angle", w.nAngle)
            .EndObject();
    }
    return arc;
}

void SerializePlayer(FSerializer& arc)
{
    if (arc.BeginObject("player"))
    {
        arc ("bobangle", bobangle)
            ("standheight", nStandHeight)
            ("playercount", PlayerCount)
            ("netstartsprites", nNetStartSprites)
            ("localplayer", nLocalPlayer)
            ("curstartsprite", nCurStartSprite)
            .Array("netstartsprite", nNetStartSprite, kMaxPlayers)
            .Array("list", PlayerList, PlayerCount);

        arc.EndObject();
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DEFINE_FIELD_X(ExhumedPlayer, Player, nHealth);
DEFINE_FIELD_X(ExhumedPlayer, Player, nLives);
DEFINE_FIELD_X(ExhumedPlayer, Player, nDouble);
DEFINE_FIELD_X(ExhumedPlayer, Player, nInvisible);
DEFINE_FIELD_X(ExhumedPlayer, Player, nTorch);
DEFINE_FIELD_X(ExhumedPlayer, Player, nSeqSize);
DEFINE_FIELD_X(ExhumedPlayer, Player, nAction);
DEFINE_FIELD_X(ExhumedPlayer, Player, pActor);
DEFINE_FIELD_X(ExhumedPlayer, Player, bIsMummified);
DEFINE_FIELD_X(ExhumedPlayer, Player, invincibility);
DEFINE_FIELD_X(ExhumedPlayer, Player, nAir);
DEFINE_FIELD_X(ExhumedPlayer, Player, nSeq);
DEFINE_FIELD_X(ExhumedPlayer, Player, nMaskAmount);
DEFINE_FIELD_X(ExhumedPlayer, Player, keys);
DEFINE_FIELD_X(ExhumedPlayer, Player, nMagic);
DEFINE_FIELD_X(ExhumedPlayer, Player, nItem);
DEFINE_FIELD_X(ExhumedPlayer, Player, items);
DEFINE_FIELD_X(ExhumedPlayer, Player, nAmmo); // TODO - kMaxWeapons? 
DEFINE_FIELD_X(ExhumedPlayer, Player, nPlayerWeapons);

DEFINE_FIELD_X(ExhumedPlayer, Player, nCurrentWeapon);
DEFINE_FIELD_X(ExhumedPlayer, Player, nSeqSize2);
DEFINE_FIELD_X(ExhumedPlayer, Player, bIsFiring);
DEFINE_FIELD_X(ExhumedPlayer, Player, nNextWeapon);
DEFINE_FIELD_X(ExhumedPlayer, Player, nState);
DEFINE_FIELD_X(ExhumedPlayer, Player, nLastWeapon);
DEFINE_FIELD_X(ExhumedPlayer, Player, nRun);

DEFINE_ACTION_FUNCTION(_Exhumed, GetViewPlayer)
{
    ACTION_RETURN_POINTER(&PlayerList[nLocalPlayer]);
}

DEFINE_ACTION_FUNCTION(_Exhumed, GetPistolClip)
{
    ACTION_RETURN_INT(PlayerList[nLocalPlayer].nPistolClip);
}

DEFINE_ACTION_FUNCTION(_Exhumed, GetPlayerClip)
{
    ACTION_RETURN_INT(PlayerList[nLocalPlayer].nPlayerClip);
}

DEFINE_ACTION_FUNCTION(_ExhumedPlayer, IsUnderwater)
{
    PARAM_SELF_STRUCT_PROLOGUE(Player);
    auto nLocalPlayer = self - PlayerList;
    ACTION_RETURN_BOOL(PlayerList[nLocalPlayer].pPlayerViewSect->Flag & kSectUnderwater);
}

DEFINE_ACTION_FUNCTION(_ExhumedPlayer, GetAngle)
{
    PARAM_SELF_STRUCT_PROLOGUE(Player);
    ACTION_RETURN_INT(self->pActor->spr.Angles.Yaw.Buildang());
}


END_PS_NS
