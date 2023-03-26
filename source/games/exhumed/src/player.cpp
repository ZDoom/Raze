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
	const auto pPlayer = &PlayerList[nPlayer];
    DExhumedActor* pPlayerActor = pPlayer->pActor;
    DExhumedActor* pDopSprite = pPlayer->pDoppleSprite;
    DExhumedActor* pFloorSprite = pPlayer->pPlayerFloorSprite;

	if (pPlayerActor)
	{
        runlist_DoSubRunRec(pPlayerActor->spr.intowner);
		runlist_FreeRun(pPlayerActor->spr.lotag - 1);

		ChangeActorStat(pPlayerActor, 0);

        pPlayer->pActor = nullptr;
        pPlayer->Angles = {};

		if (pFloorSprite)
			DeleteActor(pFloorSprite);

		if (pDopSprite)
		{
			runlist_DoSubRunRec(pDopSprite->spr.intowner);
			runlist_FreeRun(pDopSprite->spr.lotag - 1);
            DeleteActor(pDopSprite);
		}
	}

    pPlayerActor = GrabBody();
    pPlayerActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    pPlayerActor->spr.shade = -12;
    pPlayerActor->spr.pal = 0;
    pPlayerActor->spr.scale = DVector2(0.625, 0.625);
    pPlayerActor->spr.xoffset = 0;
    pPlayerActor->spr.yoffset = 0;
    pPlayerActor->spr.picnum = seq_GetSeqPicnum(kSeqJoe, 18, 0);
    pPlayerActor->spr.hitag = 0;
    pPlayerActor->spr.extra = -1;
    pPlayerActor->spr.lotag = runlist_HeadRun() + 1;
    pPlayerActor->clipdist = 14.5;
    pPlayerActor->viewzoffset = -55.;
    pPlayerActor->vel.X = 0;
    pPlayerActor->vel.Y = 0;
    pPlayerActor->vel.Z = 0;
    pPlayerActor->spr.intowner = runlist_AddRunRec(pPlayerActor->spr.lotag - 1, nPlayer, 0xA0000);
    ChangeActorStat(pPlayerActor, 100);

	if (nTotalPlayers > 1)
	{
        DExhumedActor* nNStartSprite = nNetStartSprite[nCurStartSprite];
		nCurStartSprite++;

		if (nCurStartSprite >= nNetStartSprites)
			nCurStartSprite = 0;

		pPlayerActor->spr.pos = nNStartSprite->spr.pos;
        pPlayerActor->spr.Angles.Yaw = nNStartSprite->spr.Angles.Yaw;
        ChangeActorSect(pPlayerActor, nNStartSprite->sector());

		pFloorSprite = insertActor(pPlayerActor->sector(), 0);
		pFloorSprite->spr.pos = pPlayerActor->spr.pos;
		pFloorSprite->spr.scale = DVector2(1, 1);
		pFloorSprite->spr.cstat = CSTAT_SPRITE_ALIGNMENT_FLOOR;
		pFloorSprite->spr.picnum = nPlayer + kTile3571;
	}
	else
	{
        pPlayerActor->spr.pos.XY() = pPlayer->sPlayerSave.pos.XY();
		pPlayerActor->spr.pos.Z = pPlayer->sPlayerSave.pSector->floorz;
		pPlayerActor->spr.Angles.Yaw = pPlayer->sPlayerSave.nAngle;
        ChangeActorSect(pPlayerActor, pPlayer->sPlayerSave.pSector);
		pFloorSprite = nullptr;
	}

    pPlayerActor->backuploc();

    pDopSprite = insertActor(pPlayerActor->sector(), 100);
    pDopSprite->spr.pos = pPlayerActor->spr.pos;
	pDopSprite->spr.scale = pPlayerActor->spr.scale;
	pDopSprite->spr.xoffset = 0;
	pDopSprite->spr.yoffset = 0;
	pDopSprite->spr.shade = pPlayerActor->spr.shade;
	pDopSprite->spr.Angles.Yaw = pPlayerActor->spr.Angles.Yaw;
	pDopSprite->spr.cstat = pPlayerActor->spr.cstat;
	pDopSprite->spr.lotag = runlist_HeadRun() + 1;
    pDopSprite->spr.intowner = runlist_AddRunRec(pDopSprite->spr.lotag - 1, nPlayer, 0xA0000);

    pPlayer->pActor = pPlayerActor;
    pPlayer->pDoppleSprite = pDopSprite;
    pPlayer->pPlayerFloorSprite = pFloorSprite;
    pPlayer->nPlayer = nPlayer;
    pPlayer->nAction = 0;
    pPlayer->nHealth = 800; // TODO - define
	pPlayer->nSeqSize = 0;
    pPlayer->Angles = {};
    pPlayer->Angles.initialize(pPlayerActor);
	pPlayer->bIsMummified = false;
	pPlayer->nTorch = 0;
	pPlayer->nMaskAmount = 0;
	pPlayer->nInvisible = 0;
	pPlayer->bIsFiring = 0;
	pPlayer->nSeqSize2 = 0;
	pPlayer->pPlayerViewSect = pPlayer->sPlayerSave.pSector;
	pPlayer->nState = 0;
	pPlayer->nDouble = 0;
	pPlayer->nSeq = kSeqJoe;
	pPlayer->nPlayerPushSound = -1;
	pPlayer->nNextWeapon = -1;
	pPlayer->nLastWeapon = 0;
	pPlayer->nAir = 100;
    pPlayer->pPlayerGrenade = nullptr;
    pPlayer->dVertPan = 0;
    pPlayer->vel.Zero();
    pPlayer->nThrust.Zero();
    pPlayer->nDestVertPan = pPlayerActor->PrevAngles.Pitch = pPlayerActor->spr.Angles.Pitch = nullAngle;
    pPlayer->nBreathTimer = 90;
    pPlayer->nTauntTimer = RandomSize(3) + 3;
    pPlayer->ototalvel = pPlayer->totalvel = 0;
    pPlayer->nCurrentItem = -1;
    pPlayer->nDeathType = 0;
    pPlayer->nQuake = 0;
    pPlayer->nTemperature = 0;
    pPlayer->nStandHeight = GetActorHeight(pPlayerActor);
    SetTorch(nPlayer, 0);

    if (nNetPlayerCount)
        pPlayer->nHealth = 1600; // TODO - define

    if (pPlayer->invincibility >= 0)
        pPlayer->invincibility = 0;

    if (pPlayer->nCurrentWeapon == 7)
        pPlayer->nCurrentWeapon = pPlayer->nLastWeapon;

    if (nPlayer == nLocalPlayer)
        RestoreGreenPal();

    if (pPlayer->nRun < 0)
        pPlayer->nRun = runlist_AddRunRec(NewRun, nPlayer, 0xA0000);

	if (!(currentLevel->gameflags & LEVEL_EX_MULTI))
	{
		RestoreMinAmmo(nPlayer);
	}
	else
	{
		ResetPlayerWeapons(nPlayer);
		pPlayer->nMagic = 0;
	}

	BuildRa(nPlayer);

    // clear out all input before finishing here
    inputState.ClearAllInput();
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
    const int nPlayer = RunData[ev->nRun].nObjIndex;
    assert(nPlayer >= 0 && nPlayer < kMaxPlayers);

    const auto pPlayer = &PlayerList[nPlayer];
    const auto nAction = pPlayer->nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[pPlayer->nSeq] + PlayerSeq[nAction].a, pPlayer->nSeqSize, PlayerSeq[nAction].b);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIPlayer::RadialDamage(RunListEvent* ev)
{
    const int nPlayer = RunData[ev->nRun].nObjIndex;
    assert(nPlayer >= 0 && nPlayer < kMaxPlayers);

    const auto pPlayer = &PlayerList[nPlayer];

    if (pPlayer->nHealth <= 0)
        return;

    ev->nDamage = runlist_CheckRadialDamage(pPlayer->pActor);
    Damage(ev);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AIPlayer::Damage(RunListEvent* ev)
{
    const int nPlayer = RunData[ev->nRun].nObjIndex;
    assert(nPlayer >= 0 && nPlayer < kMaxPlayers);

    const auto pPlayer = &PlayerList[nPlayer];
    const auto nDamage = ev->nDamage;

    if (!nDamage || !pPlayer->nHealth)
        return;

    const auto pPlayerActor = pPlayer->pActor;
    const auto pDamageActor = (!ev->isRadialEvent()) ? ev->pOtherActor : ev->pRadialActor->pTarget.Get();

    if (!pPlayer->invincibility)
    {
        pPlayer->nHealth -= nDamage;

        if (nPlayer == nLocalPlayer)
            TintPalette(nDamage, 0, 0);
    }

    if (pPlayer->nHealth > 0)
    {
        if (nDamage > 40 || (totalmoves & 0xF) < 2)
        {
            if (pPlayer->invincibility)
                return;

            const auto nAction = pPlayer->nAction;

            if (pPlayerActor->sector()->Flag & kSectUnderwater)
            {
                if (nAction != 12)
                {
                    pPlayer->nSeqSize = 0;
                    pPlayer->nAction = 12;
                    return;
                }
            }
            else
            {
                if (nAction != 4)
                {
                    pPlayer->nSeqSize = 0;
                    pPlayer->nAction = 4;

                    if (pDamageActor)
                    {
                        pPlayer->nPlayerSwear--;

                        if (pPlayer->nPlayerSwear <= 0)
                        {
                            D3PlayFX(StaticSound[kSound52], pPlayer->pDoppleSprite);
                            pPlayer->nPlayerSwear = RandomSize(3) + 4;
                        }
                    }
                }
            }
        }
    }
    else // player has died
    {
        if (pDamageActor && pDamageActor->spr.statnum == 100)
        {
            if (GetPlayerFromActor(pDamageActor) == nPlayer) // player caused their own death
            {
                pPlayer->nPlayerScore--;
            }
            else
            {
                pPlayer->nPlayerScore++;
            }
        }
        else if (pDamageActor == nullptr)
        {
            pPlayer->nPlayerScore--;
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

static DExhumedActor* feebtag(const DVector3& pos, sectortype* pSector, int nMagic, int nHealth, double deflen)
{
    DExhumedActor* pPickupActor = nullptr;
    auto startwall = pSector->walls.Data();
    int nWalls = pSector->walls.Size();

    while (1)
    {
        if (pSector != nullptr)
        {
            ExhumedSectIterator it(pSector);
            while (const auto itActor = it.Next())
            {
                const int nStat = itActor->spr.statnum;
                const auto diff = itActor->spr.pos - pos;

                if (nStat < 900 || nStat > 960 || (itActor->spr.cstat & CSTAT_SPRITE_INVISIBLE) || diff.Z >= 20 || diff.Z <= -100)
                    continue;

                const auto len = diff.XY().Length();
                const bool needsMagic = (nStat != 950 && nStat != 949) || nMagic < 1000;
                const bool needsHealth = (nStat != 912 && nStat != 913) || nHealth < 800;

                if (len < deflen && needsMagic && needsHealth)
                {
                    deflen = len;
                    pPickupActor = itActor;
                }
            }
        }

        if ((nWalls--) < 0)
            return pPickupActor;

        pSector = startwall->nextSector();
        startwall++;
    }

    return pPickupActor;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPickupNotification(Player* const pPlayer, const int nItem, const int nSound = -1, const int tintRed = 0, const int tintGreen = 16)
{
    if (pPlayer->nPlayer != nLocalPlayer)
        return;

    if (nItemText[nItem] > -1 && nTotalPlayers == 1)
        pickupMessage(nItem);

    if (nSound > -1)
        PlayLocalSound(nSound, 0);

    TintPalette(tintRed * 4, tintGreen * 4, 0);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPickupDestroy(DExhumedActor* const pPickupActor, const int nItem)
{
    if (!(currentLevel->gameflags & LEVEL_EX_MULTI) || (nItem >= 25 && (nItem <= 25 || nItem == 50)))
    {
        // If this is an anim we need to properly destroy it so we need to do some proper detection and not wild guesses.
        if (pPickupActor->nRun == pPickupActor->nDamage && pPickupActor->nRun != 0 && pPickupActor->nPhase == ITEM_MAGIC)
        {
            DestroyAnim(pPickupActor);
        }
        else
        {
            DeleteActor(pPickupActor);
        }
    }
    else
    {
        StartRegenerate(pPickupActor);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPickupWeapon(Player* pPlayer, DExhumedActor* pPickupActor, int nItem, int nWeapon, int nAmount, int nSound = kSound72)
{
    const int weapFlag = 1 << nWeapon;

    if (pPlayer->nPlayerWeapons & weapFlag)
    {
        if (currentLevel->gameflags & LEVEL_EX_MULTI)
            AddAmmo(pPlayer->nPlayer, WeaponInfo[nWeapon].nAmmoType, nAmount);
    }
    else
    {
        SetNewWeaponIfBetter(pPlayer->nPlayer, nWeapon);
        pPlayer->nPlayerWeapons |= weapFlag;
        AddAmmo(pPlayer->nPlayer, WeaponInfo[nWeapon].nAmmoType, nAmount);
    }

    if (nWeapon == 2)
        CheckClip(pPlayer->nPlayer);

    if (nItem > 50)
    {
        pPickupActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
        DestroyItemAnim(pPickupActor);
    }
    else
    {
        doPickupDestroy(pPickupActor, nItem);
    }

    doPickupNotification(pPlayer, nItem, StaticSound[nSound]);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPickupHealth(Player* pPlayer, DExhumedActor* pPickupActor, int nItem, const int nAmount, int nSound)
{
    if (!pPickupActor->spr.hitag || nAmount > 0 && pPlayer->nHealth >= 800)
        return;

    int tintRed = 0, tintGreen = 16;

    if (!pPlayer->invincibility || nAmount > 0)
    {
        pPlayer->nHealth += nAmount;

        if (pPlayer->nHealth > 800)
        {
            pPlayer->nHealth = 800;
        }
        else if (pPlayer->nHealth < 0)
        {
            nSound = -1;
            StartDeathSeq(pPlayer->nPlayer, 0);
        }
    }

    if (nItem == 12)
    {
        pPickupActor->spr.hitag = 0;
        pPickupActor->spr.picnum++;
        ChangeActorStat(pPickupActor, 0);
    }
    else
    {
        if (nItem == 14)
        {
            tintRed = tintGreen;
            tintGreen = 0;
        }

        doPickupDestroy(pPickupActor, nItem);
    }

    doPickupNotification(pPlayer, nItem, nSound, tintRed, tintGreen);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void doPlayerItemPickups(Player* const pPlayer)
{
    const auto pPlayerActor = pPlayer->pActor;
    const auto pPlayerSect = pPlayerActor->sector();

    if (const auto pPickupActor = feebtag(pPlayerActor->spr.pos, pPlayerSect, pPlayer->nMagic, pPlayer->nHealth, 48))
    {
        static constexpr int itemArray[] = {kItemHeart, kItemInvincibility, kItemDoubleDamage, kItemInvisibility, kItemTorch, kItemMask};
        static constexpr int weapArray[] = {6, 24, 100, 20, 2};
        static constexpr int healArray[] = {40, 160, -200};
        static constexpr int ammoArray[] = {1, 3, 2};

        switch (const int nItem = pPickupActor->spr.statnum - 900)
        {
        case 6: // Speed Loader
        case 7: // Fuel Canister
        case 8: // M - 60 Ammo Belt
            if (AddAmmo(pPlayer->nPlayer, ammoArray[nItem - 6], pPickupActor->spr.hitag))
            {
                if (nItem == 8) CheckClip(pPlayer->nPlayer);
                doPickupDestroy(pPickupActor, nItem);
                doPickupNotification(pPlayer, nItem, StaticSound[kSoundAmmoPickup]);
            }
            break;

        case 9: // Grenade
        case 27: // May not be grenade, needs confirmation
        case 55:
            doPickupWeapon(pPlayer, pPickupActor, nItem, 4, 1, kSoundAmmoPickup);
            break;

        case 10: // Pickable item
        case 15: // Pickable item
        case 16: // Reserved
        case 24:
        case 31: // Check whether is grenade or not as it matches sequence for weapons below
        case 34:
        case 35:
        case 36:
        case 39:
        case 40:
        case 41:
        case 42:
        case 43:
        case 44:
        case 51:
        case 58:
            doPickupDestroy(pPickupActor, nItem);
            doPickupNotification(pPlayer, nItem);
            break;

        case 11: // Map
            GrabMap();
            doPickupDestroy(pPickupActor, nItem);
            doPickupNotification(pPlayer, nItem);
            break;

        case 12: // Berry Twig
        case 13: // Blood Bowl
        case 14: // Cobra Venom Bowl
            doPickupHealth(pPlayer, pPickupActor, nItem, healArray[nItem - 12], nItem + 8);
            break;

        case 17: // Bubble Nest
            pPlayer->nAir += 10;

            if (pPlayer->nAir > 100)
                pPlayer->nAir = 100; // TODO - constant

            if (pPlayer->nBreathTimer < 89)
                D3PlayFX(StaticSound[kSound13], pPlayerActor);

            pPlayer->nBreathTimer = 90;
            break;

        case 18: // Still Beating Heart
        case 19: // Scarab amulet(Invicibility)
        case 20: // Severed Slave Hand(double damage)
        case 21: // Unseen eye(Invisibility)
        case 22: // Torch
        case 23: // Sobek Mask
            if (GrabItem(pPlayer->nPlayer, itemArray[nItem - 18]))
            {
                doPickupDestroy(pPickupActor, nItem);
                doPickupNotification(pPlayer, nItem);
            }
            break;

        case 25: // Extra Life
            if (pPlayer->nLives < kMaxPlayerLives)
            {
                pPlayer->nLives++;
                doPickupDestroy(pPickupActor, nItem);
                doPickupNotification(pPlayer, nItem, -1, 32, 32);
            }
            break;

        case 26: // sword pickup??
            doPickupWeapon(pPlayer, pPickupActor, nItem, 0, 0);
            break;

        case 28: // .357 Magnum Revolver
        case 52:
        case 29: // M - 60 Machine Gun
        case 53:
        case 30: // Flame Thrower
        case 54:
        case 32: // Cobra Staff
        case 56:
        case 33: // Eye of Ra Gauntlet
        case 57:
        {
            const int index = nItem - 28 - 24 * (nItem > 50);
            doPickupWeapon(pPlayer, pPickupActor, nItem, index + 1, weapArray[index]);
            break;
        }

        case 37: // Cobra staff ammo
        case 38: // Raw Energy
            if (AddAmmo(pPlayer->nPlayer, nItem - 32, (nItem == 38) ? pPickupActor->spr.hitag : 1))
            {
                doPickupDestroy(pPickupActor, nItem);
                doPickupNotification(pPlayer, nItem, StaticSound[kSoundAmmoPickup]);
            }
            break;

        case 45: // Power key
        case 46: // Time key
        case 47: // War key
        case 48: // Earth key
        {
            const int keybit = 4096 << (nItem - 45);
            if (!(pPlayer->keys & keybit))
            {
                pPlayer->keys |= keybit;
                doPickupDestroy(pPickupActor, nItem);
                doPickupNotification(pPlayer, nItem);
            }
            break;
        }

        case 49: // Magical Essence
        case 50: // ?
            if (pPlayer->nMagic < 1000)
            {
                pPlayer->nMagic += 100;

                if (pPlayer->nMagic >= 1000)
                    pPlayer->nMagic = 1000;

                doPickupDestroy(pPickupActor, nItem);
                doPickupNotification(pPlayer, nItem, StaticSound[kSoundMana1]);
            }
            break;

        case 59: // Scarab (Checkpoint)
            if (nLocalPlayer == pPlayer->nPlayer)
            {
                pPickupActor->nIndex2++;
                pPickupActor->nAction &= 0xEF;
                pPickupActor->nIndex = 0;
                ChangeActorStat(pPickupActor, 899);
            }
            SetSavePoint(pPlayer->nPlayer, pPlayerActor->spr.pos, pPlayerSect, pPlayerActor->spr.Angles.Yaw);
            break;

        case 60: // Golden Sarcophagus (End Level)
            if (!bInDemo) LevelFinished();
            DestroyItemAnim(pPickupActor);
            DeleteActor(pPickupActor);
            break;
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
    const auto pPlayerActor = pPlayer->pActor;
    const auto pRa = &Ra[pPlayer->nPlayer];
    const auto nAngVect = (-pPlayerActor->spr.Angles.Yaw).ToVector();

    DExhumedActor* bestTarget = nullptr;
    double bestclose = 20;
    double bestside = 30000;

    ExhumedSpriteIterator it;
    while (const auto itActor = it.Next())
    {
        const bool validstatnum = (itActor->spr.statnum > 0) && (itActor->spr.statnum < 150);
        const bool validsprcstat = itActor->spr.cstat & CSTAT_SPRITE_BLOCK_ALL;

        if (!validstatnum || !validsprcstat || itActor == pPlayerActor)
            continue;

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

    if (bestTarget)
    {
        if (pPlayer->nPlayer == nLocalPlayer)
            nCreepyTimer = kCreepyCount;

        if (!cansee(pPlayerActor->spr.pos, pPlayerActor->sector(), bestTarget->spr.pos.plusZ(-GetActorHeight(bestTarget)), bestTarget->sector()))
            bestTarget = nullptr;
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
    const auto pPlayerActor = pPlayer->pActor;

    if (pPlayer->nHealth > 0)
    {
        const auto pInput = &pPlayer->input;
        const auto inputvect = DVector2(pInput->fvel, pInput->svel).Rotated(pPlayerActor->spr.Angles.Yaw) * 0.375;

        for (int i = 0; i < 4; i++)
        {
            pPlayer->vel += inputvect;
            pPlayer->vel *= 0.953125;
        }
    }

    pPlayerActor->vel.XY() = pPlayer->vel;
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
        pPlayer->input.setItemUsed(pPlayer->nItem);

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
    const bool bConsolePlayer = pPlayer->nPlayer == nLocalPlayer;

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
        else if (!bConsolePlayer)
        {
            nFlashDepth = 5;
            AddFlash(pPlayerActor->sector(), pPlayerActor->spr.pos, 0);
        }
    }

    if (pPlayer->nDouble > 0)
    {
        pPlayer->nDouble--;

        if (pPlayer->nDouble == 150 && bConsolePlayer)
            PlayAlert(GStrings("TXT_EX_WEAPONEX"));
    }

    if (pPlayer->nInvisible > 0)
    {
        pPlayer->nInvisible--;

        if (pPlayer->nInvisible == 0)
        {
            pPlayerActor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;

            if (pPlayer->pPlayerFloorSprite) 
                pPlayer->pPlayerFloorSprite->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
        }
        else if (pPlayer->nInvisible == 150 && bConsolePlayer)
        {
            PlayAlert(GStrings("TXT_EX_INVISEX"));
        }
    }

    if (pPlayer->invincibility > 0)
    {
        pPlayer->invincibility--;

        if (pPlayer->invincibility == 150 && bConsolePlayer)
            PlayAlert(GStrings("TXT_EX_INVINCEX"));
    }

    if (pPlayer->nMaskAmount > 0 && pPlayer->nHealth > 0)
    {
        pPlayer->nMaskAmount--;

        if (pPlayer->nMaskAmount == 150 && bConsolePlayer)
            PlayAlert(GStrings("TXT_EX_MASKEX"));
    }

    if (pPlayer->nQuake != 0)
    {
        pPlayer->nQuake = -pPlayer->nQuake;

        if (pPlayer->nQuake > 0)
        {
            pPlayer->nQuake -= 2.;

            if (pPlayer->nQuake < 0)
                pPlayer->nQuake = 0;
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
        const auto pPlayerSect = pPlayerActor->sector();
        const auto highSpeed = pPlayer->totalvel > 25;
        const auto belowFloor = pPlayerActor->spr.pos.Z > pPlayerSect->floorz;

        if (highSpeed && belowFloor && pPlayerSect->Depth && !pPlayerSect->Speed && !pPlayerSect->Damage)
            D3PlayFX(StaticSound[kSound42], pPlayerActor);

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
            pPlayerActor->spr.Angles.Pitch = currentLevel->ex_ramses_horiz;
            pPlayer->nDestVertPan = nullAngle;
            StopAllSounds();
            StopLocalSound();
            InitSpiritHead();
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
    const double nStartVelZ = pPlayerActor->vel.Z;
    Gravity(pPlayerActor);

    if (pPlayerActor->vel.Z >= (6500 / 256.) && nStartVelZ < (6500 / 256.))
        D3PlayFX(StaticSound[kSound17], pPlayerActor);
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
            const auto pPlayerSect = pPlayerActor->sector();
            pFloorActor->spr.pos.XY() = pPlayerActor->spr.pos.XY();
            pFloorActor->spr.pos.Z = pPlayerSect->floorz;

            if (pFloorActor->sector() != pPlayerSect)
                ChangeActorSect(pFloorActor, pPlayerSect);
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
    const auto pPlayerSect = pPlayerActor->sector();
    DExhumedActor* const pDopple = pPlayer->pDoppleSprite;
    pDopple->spr.pos = pPlayerActor->spr.pos;

    if (pPlayerSect->pAbove != nullptr)
    {
        pDopple->spr.Angles.Yaw = pPlayerActor->spr.Angles.Yaw;
        pDopple->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
        ChangeActorSect(pDopple, pPlayerSect->pAbove);
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
        if (EyeZ >= pViewSect->ceilingz || !pViewSect->pAbove)
            break;

        pViewSect = pViewSect->pAbove;
    }

    // Do underwater sector check
    if (bUnderwater && pViewSect != pPlayerActor->sector() && nMove.type == kHitWall)
    {
        const auto pos = pPlayerActor->spr.pos;
        const auto fz = pViewSect->floorz - 20;
        pPlayerActor->spr.pos = DVector3(spr_pos.XY(), fz);
        ChangeActorSect(pPlayerActor, pViewSect);
        const auto coll = movesprite(pPlayerActor, spr_vel.XY(), 0, 0, CLIPMASK0);

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

static void doPlayerFloorDamage(Player* const pPlayer, const double nStartVelZ)
{
    const auto pPlayerActor = pPlayer->pActor;
    pPlayer->nThrust *= 0.5;

    if (nStartVelZ < (6500 / 256.))
        return;

    pPlayerActor->vel.XY() *= 0.25;
    runlist_DamageEnemy(pPlayerActor, nullptr, int(((nStartVelZ * 256) - 6500) * (1. / 128.)) + 10);

    if (pPlayer->nHealth <= 0)
    {
        pPlayerActor->vel.Zero();
        StopActorSound(pPlayerActor);
        PlayFXAtXYZ(StaticSound[kSoundJonFDie], pPlayerActor->spr.pos, CHANF_NONE, 1); // CHECKME
    }
    else
    {
        D3PlayFX(StaticSound[kSound27] | 0x2000, pPlayerActor);
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

    if (!sect)
    {
        return;
    }
    else if ((sect->hitag == 45) && bTouchFloor && absangle(nNormal, pPlayerActor->spr.Angles.Yaw + DAngle180) <= DAngle45)
    {
        pPlayer->pPlayerPushSect = sect;
        DVector2 vel = pPlayer->vel;
        const auto nMyAngle = vel.Angle().Normalized360();

        setsectinterpolate(sect);
        MoveSector(sect, nMyAngle, vel);

        if (pPlayer->nPlayerPushSound == -1)
        {
            pPlayer->nPlayerPushSound = sect->extra;
            D3PlayFX(StaticSound[kSound23], sBlockInfo[pPlayer->nPlayerPushSound].pActor, 0x4000);
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
            StopActorSound(sBlockInfo[pPlayer->pPlayerPushSect->extra].pActor);

        pPlayer->nPlayerPushSound = -1;
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

    // we need a separate zvel backup taken before a move,
    // but after we've clamped its value. Exhumed is a mess...
    const auto nStartVelZ = pPlayerActor->vel.Z;

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

        if (inside(pPlayerActor->spr.pos.X, pPlayerActor->spr.pos.Y, pPlayerActor->sector()) != 1)
        {
            ChangeActorSect(pPlayerActor, spr_sect);
            pPlayerActor->spr.pos.XY() = spr_pos.XY();

            if (nStartVelZ < pPlayerActor->vel.Z)
                pPlayerActor->vel.Z = nStartVelZ;
        }
    }

    const bool bUnderwater = pPlayerActor->sector()->Flag & kSectUnderwater;

    if (bUnderwater)
        pPlayer->nThrust *= 0.5;

    // Trigger Ramses?
    if ((pPlayerActor->sector()->Flag & 0x8000) && bTouchFloor)
        return false;

    if (nMove.type || nMove.exbits)
    {
        if (bTouchFloor)
            doPlayerFloorDamage(pPlayer, nStartVelZ);

        if (nMove.type == kHitSector || nMove.type == kHitWall)
            doPlayerMovingBlocks(pPlayer, nMove, spr_pos, spr_vel, spr_sect);
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

    if (!pPlayer->bIsMummified && (pPlayer->input.actions & SB_OPEN))
    {
        pPlayer->input.actions &= ~SB_OPEN;

        // neartag finds the nearest sector, wall, and sprite which has its hitag and/or lotag set to a value.
        HitInfo near;
        neartag(pPlayerActor->spr.pos, pPlayerSect, pPlayerActor->spr.Angles.Yaw, near, 128., NT_Hitag | NT_NoSpriteCheck);

        if (near.hitWall != nullptr && near.hitWall->lotag > 0)
            runlist_SignalRun(near.hitWall->lotag - 1, pPlayer->nPlayer, &ExhumedAI::Use);

        if (near.hitSector != nullptr && near.hitSector->lotag > 0)
            runlist_SignalRun(near.hitSector->lotag - 1, pPlayer->nPlayer, &ExhumedAI::Use);
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
                    pPlayerActor->viewzoffset += ((-32.5 - pPlayerActor->viewzoffset) * 0.5);

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
                else if (pPlayer->nStandHeight > (pPlayerSect->floorz - pPlayerSect->ceilingz))
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
                if (bUnderwater)
                {
                    nextAction = 11;
                }
                else if (nextAction != 2 && nextAction != 1)
                {
                    nextAction = 5;
                }
            }
        }

        // Handle player pressing number keys to change weapon
        const unsigned newWeap = pPlayer->input.getNewWeapon() - 1;
        if (pPlayer->nPlayerWeapons & (1 << newWeap))
            SetNewWeapon(pPlayer->nPlayer, newWeap);

        pPlayer->bIsFiring = !!(pPlayer->input.actions & SB_FIRE);
    }
    else // player is mummified
    {
        if (pPlayer->input.actions & SB_FIRE)
            pPlayer->bIsFiring = true;

        if (pPlayer->nAction != 15)
            nextAction = 14 - (pPlayer->totalvel <= 1);
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

static void doPlayerAngles(Player* const pPlayer)
{
    const auto pPlayerActor = pPlayer->pActor;
    const auto pInput = &pPlayer->input;
    const auto nDestVertPan = cl_slopetilting ? pPlayer->nDestVertPan : nullAngle;
    const auto nVertPan = deltaangle(pPlayer->Angles.ViewAngles.Pitch, nDestVertPan).Tan() * 32.;

    if (SyncInput())
    {
        pPlayerActor->spr.Angles.Yaw += DAngle::fromDeg(pInput->avel);
        pPlayerActor->spr.Angles.Pitch += DAngle::fromDeg(pInput->horz);
    }

    pPlayer->Angles.doYawKeys(pInput);
    pPlayer->Angles.doViewYaw(pInput);
    pPlayer->Angles.doPitchKeys(pInput);
    pPlayer->Angles.ViewAngles.Pitch += maphoriz(abs(nVertPan) >= 4 ? Sgn(nVertPan) * 4. : nVertPan * 2.);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool doPlayerDeathRestart(Player* const pPlayer)
{
    if (!(pPlayer->input.actions & SB_OPEN) || pPlayer->nAction < 16)
        return true;

    pPlayer->input.actions &= ~SB_OPEN;

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

    if (pPlayer->nSeqSize < SeqSize[nSeq])
        return;

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
    else if (pPlayerActor->spr.Angles.Pitch.Sgn() > 0)
    {
        pPlayerActor->spr.Angles.Pitch = nullAngle;
        pPlayerActor->viewzoffset -= pPlayer->dVertPan;
    }
    else
    {
        pPlayerActor->spr.Angles.Pitch -= maphoriz(pPlayer->dVertPan);

        if (pPlayerActor->spr.Angles.Pitch.Degrees() <= -38)
        {
            pPlayerActor->spr.Angles.Pitch = DAngle::fromDeg(-38);
        }
        else if (pPlayerActor->spr.Angles.Pitch.Sgn() >= 0 && !(pPlayerActor->sector()->Flag & kSectUnderwater))
        {
            SetNewWeapon(pPlayer->nPlayer, pPlayer->nDeathType + 8);
        }

        pPlayer->dVertPan--;
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
        doPlayerAngles(pPlayer);

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
