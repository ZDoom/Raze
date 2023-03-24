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
        plr->bPlayerPan = plr->bLockPan = false;
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

bool CheckMovingBlocks(int nPlayer, Collision& nMove, DVector3& spr_pos, sectortype* spr_sect)
{
    auto pPlayerActor = PlayerList[nPlayer].pActor;
    double const zz = pPlayerActor->vel.Z;

    if (nMove.type == kHitSector || nMove.type == kHitWall)
    {
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
            nNormal = GetWallNormal(nMove.hitWall);
        }

        // moving blocks - move this to a separate function!
        if (sect != nullptr)
        {
            if ((sect->hitag == 45) && bTouchFloor)
            {
                auto nDiff = absangle(nNormal, pPlayerActor->spr.Angles.Yaw + DAngle180);

                if (nDiff <= DAngle45)
                {
                    PlayerList[nPlayer].pPlayerPushSect = sect;

                    DVector2 vel = PlayerList[nPlayer].vel;
                    auto nMyAngle = vel.Angle().Normalized360();

                    setsectinterpolate(sect);
                    MoveSector(sect, nMyAngle, vel);

                    if (PlayerList[nPlayer].nPlayerPushSound <= -1)
                    {
                        PlayerList[nPlayer].nPlayerPushSound = 1;
                        int nBlock = PlayerList[nPlayer].pPlayerPushSect->extra;
                        DExhumedActor* pBlockActor = sBlockInfo[nBlock].pActor;

                        D3PlayFX(StaticSound[kSound23], pBlockActor, 0x4000);
                    }
                    else
                    {
                        pPlayerActor->spr.pos = spr_pos;
                        ChangeActorSect(pPlayerActor, spr_sect);
                    }

                    movesprite(pPlayerActor, vel, zz, -20, CLIPMASK0);
                    return true;
                }
            }
        }
    }
    return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPlayerCurrentItem(Player* const pPlayer)
{
    UseItem(pPlayer->nPlayer, pPlayer->nCurrentItem);
    pPlayer->nCurrentItem = -1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPlayerTorch(Player* const pPlayer)
{
    pPlayer->nTorch--;

    if (pPlayer->nTorch == 0)
    {
        SetTorch(pPlayer->nPlayer, 0);
    }
    else if (pPlayer->nPlayer != nLocalPlayer)
    {
        nFlashDepth = 5;
        AddFlash(pPlayer->pActor->sector(), pPlayer->pActor->spr.pos, 0);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPlayerDouble(Player* const pPlayer)
{
    pPlayer->nDouble--;

    if (pPlayer->nDouble == 150 && pPlayer->nPlayer == nLocalPlayer)
    {
        PlayAlert(GStrings("TXT_EX_WEAPONEX"));
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPlayerInvisibility(Player* const pPlayer)
{
    pPlayer->nInvisible--;

    if (pPlayer->nInvisible == 0)
    {
        pPlayer->pActor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE; // set visible

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPlayerInvincibility(Player* const pPlayer)
{
    pPlayer->invincibility--;

    if (pPlayer->invincibility == 150 && pPlayer->nPlayer == nLocalPlayer)
    {
        PlayAlert(GStrings("TXT_EX_INVINCEX"));
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPlayerQuake(Player* const pPlayer)
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void updatePlayerAction(Player* const pPlayer)
{
    const auto pPlayerActor = pPlayer->pActor;
    const bool bUnderwater = pPlayerActor->sector()->Flag & kSectUnderwater;
    int nActionB = pPlayer->nAction;
    int var_48 = 0;

    if (!pPlayer->bIsMummified)
    {
        // CHECKME - are we finished with 'nSector' variable at this point? if so, maybe set it to pPlayerActor->spr.sector so we can make this code a bit neater. Don't assume pPlayerActor->spr.sector == nSector here!!
        if (nStandHeight > (pPlayerActor->sector()->floorz - pPlayerActor->sector()->ceilingz)) {
            var_48 = 1;
        }

        // Jumping
        if (pPlayer->input.actions & SB_JUMP)
        {
            if (bUnderwater)
            {
                pPlayerActor->vel.Z = -8;
                nActionB = 10;
            }
            else if (bTouchFloor)
            {
                if (pPlayer->nAction < 6 || pPlayer->nAction > 8)
                {
                    pPlayerActor->vel.Z = -14;
                    nActionB = 3;
                }
            }
        }
        else if (pPlayer->input.actions & SB_CROUCH)
        {
            if (bUnderwater)
            {
                pPlayerActor->vel.Z = 8;
                nActionB = 10;
            }
            else
            {
                if (pPlayerActor->viewzoffset < -32.5) {
                    pPlayerActor->viewzoffset += ((-32.5 - pPlayerActor->viewzoffset) * 0.5);
                }

            loc_1BD2E:
                if (pPlayer->totalvel < 1) {
                    nActionB = 6;
                }
                else {
                    nActionB = 7;
                }
            }
        }
        else
        {
            if (pPlayer->nHealth > 0)
            {
                pPlayerActor->viewzoffset += (nActionEyeLevel[pPlayer->nAction] - pPlayerActor->viewzoffset) * 0.5;

                if (bUnderwater)
                {
                    if (pPlayer->totalvel <= 1)
                        nActionB = 9;
                    else
                        nActionB = 10;
                }
                else
                {
                    // CHECKME - confirm branching in this area is OK
                    if (var_48)
                    {
                        goto loc_1BD2E;
                    }
                    else
                    {
                        if (pPlayer->totalvel <= 1) {
                            nActionB = 0;//bUnderwater; // this is just setting to 0
                        }
                        else if (pPlayer->totalvel <= 30) {
                            nActionB = 2;
                        }
                        else
                        {
                            nActionB = 1;
                        }
                    }
                }
            }
            // loc_1BE30
            if (pPlayer->input.actions & SB_FIRE) // was var_38
            {
                if (bUnderwater)
                {
                    nActionB = 11;
                }
                else
                {
                    if (nActionB != 2 && nActionB != 1)
                    {
                        nActionB = 5;
                    }
                }
            }
        }

        // loc_1BE70:
        // Handle player pressing number keys to change weapon
        if (uint8_t var_90 = pPlayer->input.getNewWeapon())
        {
            var_90--;

            if (pPlayer->nPlayerWeapons & (1 << var_90))
            {
                SetNewWeapon(pPlayer->nPlayer, var_90);
            }
        }
    }
    else // player is mummified
    {
        if (pPlayer->input.actions & SB_FIRE)
        {
            FireWeapon(pPlayer->nPlayer);
        }

        if (pPlayer->nAction != 15)
        {
            if (pPlayer->totalvel <= 1)
            {
                nActionB = 13;
            }
            else
            {
                nActionB = 14;
            }
        }
    }

    // loc_1BF09
    if (nActionB != pPlayer->nAction && pPlayer->nAction != 4)
    {
        pPlayer->nAction = nActionB;
        pPlayer->nSeqSize = 0;
    }
}

//---------------------------------------------------------------------------
//
// this function is pure spaghetti madness... :(
//
//---------------------------------------------------------------------------

void AIPlayer::Tick(RunListEvent* ev)
{
    const int nPlayer = RunData[ev->nRun].nObjIndex;
    assert(nPlayer >= 0 && nPlayer < kMaxPlayers);

    const auto pPlayer = &PlayerList[nPlayer];
    const auto pPlayerActor = pPlayer->pActor;

    DExhumedActor* pDopple = pPlayer->pDoppleSprite;
    pDopple->spr.picnum = pPlayerActor->spr.picnum;
    pPlayerActor->spr.picnum = seq_GetSeqPicnum(pPlayer->nSeq, PlayerSeq[nHeightTemplate[pPlayer->nAction]].a, pPlayer->nSeqSize);
    pPlayerActor->vel.XY() = pPlayer->vel;

    if (pPlayer->nCurrentItem > -1)
        doPlayerCurrentItem(pPlayer);

    if (pPlayer->nTorch > 0)
        doPlayerTorch(pPlayer);

    if (pPlayer->nDouble > 0)
        doPlayerDouble(pPlayer);

    if (pPlayer->nInvisible > 0)
        doPlayerInvisibility(pPlayer);

    if (pPlayer->invincibility > 0)
        doPlayerInvincibility(pPlayer);

    if (pPlayer->nQuake != 0)
        doPlayerQuake(pPlayer);

    pPlayer->Angles.doViewYaw(&pPlayer->input);

    // loc_1A494:
    if (SyncInput())
    {
        pPlayer->pActor->spr.Angles.Yaw += DAngle::fromDeg(pPlayer->input.avel);
    }

    pPlayer->Angles.doYawKeys(&pPlayer->input);

    // player.zvel is modified within Gravity()
	double zVel = pPlayerActor->vel.Z;

    Gravity(pPlayerActor);

    if (pPlayerActor->vel.Z >= 6500/256. && zVel < 6500 / 256.)
    {
        D3PlayFX(StaticSound[kSound17], pPlayerActor);
    }

    // loc_1A4E6
    auto pSector = pPlayerActor->sector();
    int nSectFlag = pPlayer->pPlayerViewSect->Flag;

    auto playerPos = pPlayerActor->spr.pos.XY();

    DVector2 vect = pPlayer->vel;
    double zz = pPlayerActor->vel.Z;

    if (pPlayerActor->vel.Z > 32)
        pPlayerActor->vel.Z = 32;

    if (pPlayer->bIsMummified)
    {
        vect *= 0.5;
    }

	auto spr_pos = pPlayerActor->spr.pos;
    auto spr_sect = pPlayerActor->sector();

    // TODO
    // nSectFlag & kSectUnderwater;

    zVel = pPlayerActor->vel.Z;

    Collision nMove;
    nMove.setNone();
    if (bSlipMode)
    {
        pPlayerActor->spr.pos += vect;

        SetActor(pPlayerActor, pPlayerActor->spr.pos);
        pPlayerActor->spr.pos.Z = pPlayerActor->sector()->floorz;
    }
    else
    {
        nMove = movesprite(pPlayerActor, vect, zz, -20, CLIPMASK0);

        auto pPlayerSect = pPlayerActor->sector();

        pushmove(pPlayerActor->spr.pos, &pPlayerSect, pPlayerActor->clipdist, 20, -20, CLIPMASK0);
        if (pPlayerSect != pPlayerActor->sector()) {
            ChangeActorSect(pPlayerActor, pPlayerSect);
        }
    }

    // loc_1A6E4
    if (inside(pPlayerActor->spr.pos.X, pPlayerActor->spr.pos.Y, pPlayerActor->sector()) != 1)
    {
        ChangeActorSect(pPlayerActor, spr_sect);

		pPlayerActor->spr.pos.XY() = spr_pos.XY();

        if (zVel < pPlayerActor->vel.Z) {
            pPlayerActor->vel.Z = zVel;
        }
    }

    //			int _bTouchFloor = bTouchFloor;
    int bUnderwater = pPlayerActor->sector()->Flag & kSectUnderwater;

    if (bUnderwater)
    {
        pPlayer->nThrust /= 2;
    }

    // Trigger Ramses?
    if ((pPlayerActor->sector()->Flag & 0x8000) && bTouchFloor)
    {
        if (nTotalPlayers <= 1)
        {
            setForcedSyncInput(nPlayer);
            pPlayerActor->spr.Angles = DRotator(nullAngle, GetAngleToSprite(pPlayerActor, pSpiritSprite), nullAngle);
            pPlayerActor->backupang();

            pPlayer->vel.Zero();
            pPlayerActor->vel.Zero();

            if (nFreeze < 1)
            {
                nFreeze = 1;
                StopAllSounds();
                StopLocalSound();
                InitSpiritHead();

                pPlayer->nDestVertPan = nullAngle;
                pPlayerActor->spr.Angles.Pitch = currentLevel->ex_ramses_horiz;
            }
        }
        else
        {
            LevelFinished();
        }

        return;
    }

    if (nMove.type || nMove.exbits)
    {
        if (bTouchFloor)
        {
            // Damage stuff..
            pPlayer->nThrust /= 2;

            if (nPlayer == nLocalPlayer)
            {
                double zVelB = zVel;

                if (zVelB < 0) {
                    zVelB = -zVelB;
                }

                if (zVelB > 2 && !pPlayer->pActor->spr.Angles.Pitch.Sgn() && cl_slopetilting) {
                    pPlayer->nDestVertPan = nullAngle;
                }
            }

            if (zVel >= 6500 / 256.)
            {
                pPlayerActor->vel.XY() *= 0.25;

                runlist_DamageEnemy(pPlayerActor, nullptr, ((int(zVel * 256) - 6500) >> 7) + 10);

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

        if (CheckMovingBlocks(nPlayer, nMove, spr_pos, spr_sect))
            goto sectdone;
    }

    // loc_1AB46:
    if (pPlayer->nPlayerPushSound > -1)
    {
        if (pPlayer->pPlayerPushSect != nullptr)
        {
            StopActorSound(sBlockInfo[pPlayer->pPlayerPushSect->extra].pActor);
        }

        pPlayer->nPlayerPushSound = -1;
    }

sectdone:
    if (!pPlayer->bPlayerPan && !pPlayer->bLockPan)
    {
        pPlayer->nDestVertPan = maphoriz((pPlayerActor->spr.pos.Z - spr_pos.Z) * 2.);
    }

    playerPos -= pPlayerActor->spr.pos.XY();


    pPlayer->ototalvel = pPlayer->totalvel;
    pPlayer->totalvel = int(playerPos.Length() * worldtoint);

    auto pViewSect = pPlayerActor->sector();

    double EyeZ = pPlayerActor->getOffsetZ() + pPlayer->nQuake;

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
    if (bUnderwater)
    {
        if (pViewSect != pPlayerActor->sector())
        {
            if (nMove.type == kHitWall)
            {
				auto pos = pPlayerActor->spr.pos;

                ChangeActorSect(pPlayerActor, pViewSect);


                double fz = pViewSect->floorz - 20;
                pPlayerActor->spr.pos = DVector3(spr_pos.XY(), fz);

                auto coll = movesprite(pPlayerActor, vect, 0, 0, CLIPMASK0);
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
        }
    }

    // loc_1ADAF
    pPlayer->pPlayerViewSect = pViewSect;

    pPlayer->nPlayerD = (pPlayerActor->spr.pos - spr_pos);

    int var_5C = pViewSect->Flag & kSectUnderwater;

    // loc_1AEF5:
    if (pPlayer->nHealth > 0)
    {
        if (pPlayer->nMaskAmount > 0)
        {
            pPlayer->nMaskAmount--;
            if (pPlayer->nMaskAmount == 150 && nPlayer == nLocalPlayer) {
				PlayAlert(GStrings("TXT_EX_MASKEX"));
            }
        }

        if (!pPlayer->invincibility)
        {
            // Handle air
            pPlayer->nBreathTimer--;

            if (pPlayer->nBreathTimer <= 0)
            {
                pPlayer->nBreathTimer = 90;

                // if underwater
                if (var_5C)
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
                                StartDeathSeq(nPlayer, 0);
                            }

                            pPlayer->nAir = 0;

                            if (pPlayer->nHealth < 300)
                            {
                                D3PlayFX(StaticSound[kSound79], pPlayerActor);
                            }
                            else
                            {
                                D3PlayFX(StaticSound[kSound19], pPlayerActor);
                            }
                        }
                    }

                    DoBubbles(nPlayer);
                }
            }
        }

        // loc_1B0B9
        if (var_5C) // if underwater
        {
            if (pPlayer->nTorch > 0)
            {
                pPlayer->nTorch = 0;
                SetTorch(nPlayer, 0);
            }
        }
        else
        {
            auto pTmpSect = pPlayerActor->sector();

            if (pPlayer->totalvel > 25 && pPlayerActor->spr.pos.Z > pTmpSect->floorz)
            {
                if (pTmpSect->Depth && !pTmpSect->Speed && !pTmpSect->Damage)
                {
                    D3PlayFX(StaticSound[kSound42], pPlayerActor);
                }
            }

            // CHECKME - wrong place?
            if (nSectFlag & kSectUnderwater)
            {
                if (pPlayer->nAir < 50)
                {
                    D3PlayFX(StaticSound[kSound14], pPlayerActor);
                }

                pPlayer->nBreathTimer = 1;
            }

            pPlayer->nBreathTimer--;
            if (pPlayer->nBreathTimer <= 0)
            {
                pPlayer->nBreathTimer = 90;
            }

            if (pPlayer->nAir < 100)
            {
                pPlayer->nAir = 100;
            }
        }

        // loc_1B1EB
        DExhumedActor* pFloorActor = pPlayer->pPlayerFloorSprite;
        if (nTotalPlayers > 1 && pFloorActor)
        {
            pFloorActor->spr.pos.XY() = pPlayerActor->spr.pos.XY();

            if (pFloorActor->sector() != pPlayerActor->sector())
            {
                ChangeActorSect(pFloorActor, pPlayerActor->sector());
            }

			pFloorActor->spr.pos.Z = pPlayerActor->sector()->floorz;
        }

        doPlayerItemPickups(pPlayer);

        // CORRECT ? // loc_1BAF9:
        if (bTouchFloor)
        {
            if (pPlayerActor->sector()->lotag > 0)
            {
                runlist_SignalRun(pPlayerActor->sector()->lotag - 1, nPlayer, &ExhumedAI::TouchFloor);
            }
        }

        if (pSector != pPlayerActor->sector())
        {
            if (pSector->lotag > 0)
            {
                runlist_SignalRun(pSector->lotag - 1, nPlayer, &ExhumedAI::EnterSector);
            }

            if (pPlayerActor->sector()->lotag > 0)
            {
                runlist_SignalRun(pPlayerActor->sector()->lotag - 1, nPlayer, &ExhumedAI::LeaveSector);
            }
        }

        if (!pPlayer->bIsMummified)
        {
            if (pPlayer->input.actions & SB_OPEN)
            {
                pPlayer->input.actions &= ~SB_OPEN;

                // code to handle item pickup?
                HitInfo near;

                // neartag finds the nearest sector, wall, and sprite which has its hitag and/or lotag set to a value.
                neartag(pPlayerActor->spr.pos, pPlayerActor->sector(), pPlayerActor->spr.Angles.Yaw, near, 128., NT_Hitag | NT_NoSpriteCheck);

                int tag;
                if (near.hitWall != nullptr && (tag = near.hitWall->lotag) > 0)
                {
                    runlist_SignalRun(tag - 1, nPlayer, &ExhumedAI::Use);
                }

                if (near.hitSector != nullptr && (tag = near.hitSector->lotag) > 0)
                {
                    runlist_SignalRun(tag - 1, nPlayer, &ExhumedAI::Use);
                }
            }

            pPlayer->bIsFiring = !!(pPlayer->input.actions & SB_FIRE);
        }

        updatePlayerAction(pPlayer);

        if (SyncInput())
        {
            pPlayer->pActor->spr.Angles.Pitch += DAngle::fromDeg(pPlayer->input.horz);
        }

        pPlayer->Angles.doPitchKeys(&pPlayer->input);

        if (pPlayer->input.actions & (SB_AIM_UP | SB_AIM_DOWN) || pPlayer->input.horz)
        {
            pPlayer->nDestVertPan = pPlayer->pActor->spr.Angles.Pitch;
            pPlayer->bPlayerPan = pPlayer->bLockPan = true;
        }
        else if (pPlayer->input.actions & (SB_LOOK_UP | SB_LOOK_DOWN | SB_CENTERVIEW))
        {
            pPlayer->nDestVertPan = pPlayer->pActor->spr.Angles.Pitch;
            pPlayer->bPlayerPan = pPlayer->bLockPan = false;
        }

        if (pPlayer->totalvel > 20)
        {
            pPlayer->bPlayerPan = false;
        }

        if (cl_slopetilting && !pPlayer->bPlayerPan && !pPlayer->bLockPan)
        {
            if (double nVertPan = deltaangle(pPlayer->pActor->spr.Angles.Pitch, pPlayer->nDestVertPan).Tan() * 32.)
            {
                pPlayer->pActor->spr.Angles.Pitch += maphoriz(abs(nVertPan) >= 4 ? clamp(nVertPan, -4., 4.) : nVertPan * 2.);
            }
        }
    }
    else // else, player's health is less than 0
    {
        setForcedSyncInput(nPlayer);

        // loc_1C0E9
        if (pPlayer->input.actions & SB_OPEN)
        {
            pPlayer->input.actions &= ~SB_OPEN;

            if (pPlayer->nAction >= 16)
            {
                if (nPlayer == nLocalPlayer)
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
                        pPlayerActor->spr.picnum = seq_GetSeqPicnum(kSeqJoe, 120, 0);
                        pPlayerActor->spr.cstat = 0;
						pPlayerActor->spr.pos.Z = pPlayerActor->sector()->floorz;
                    }

                    // will invalidate nPlayerSprite
                    RestartPlayer(nPlayer);
                    pDopple = pPlayer->pDoppleSprite;
                }
                else
                {
                    DoGameOverScene(currentLevel->levelNumber == 20);
                    return;
                }
            }
        }
    }

    // loc_1C201:
    if (nLocalPlayer == nPlayer)
    {
        pLocalEyeSect = PlayerList[nLocalPlayer].pPlayerViewSect;
        CheckAmbience(pLocalEyeSect);
    }

    int var_AC = SeqOffsets[pPlayer->nSeq] + PlayerSeq[pPlayer->nAction].a;

    seq_MoveSequence(pPlayerActor, var_AC, pPlayer->nSeqSize);
    pPlayer->nSeqSize++;

    if (pPlayer->nSeqSize >= SeqSize[var_AC])
    {
        pPlayer->nSeqSize = 0;

        switch (pPlayer->nAction)
        {
        default:
            break;

        case 3:
            pPlayer->nSeqSize = SeqSize[var_AC] - 1;
            break;
        case 4:
            pPlayer->nAction = 0;
            break;
        case 16:
            pPlayer->nSeqSize = SeqSize[var_AC] - 1;

            if (pPlayerActor->spr.pos.Z < pPlayerActor->sector()->floorz) 
            {
				pPlayerActor->spr.pos.Z++;
            }

            if (!RandomSize(5))
            {
                sectortype* mouthSect;
                auto pos = WheresMyMouth(nPlayer, &mouthSect);

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

    if (!pPlayer->nHealth)
    {
        pPlayer->nThrust.Zero();

        if (pPlayerActor->viewzoffset >= -11)
        {
            pPlayerActor->viewzoffset = -11;
            pPlayer->dVertPan = 0;
        }
        else
        {
            if (pPlayer->pActor->spr.Angles.Pitch.Sgn() > 0)
            {
                pPlayerActor->spr.Angles.Pitch = nullAngle;
                pPlayerActor->viewzoffset -= pPlayer->dVertPan;
            }
            else
            {
                pPlayer->pActor->spr.Angles.Pitch -= maphoriz(pPlayer->dVertPan);

                if (pPlayer->pActor->spr.Angles.Pitch.Degrees() <= -38)
                {
                    pPlayer->pActor->spr.Angles.Pitch = DAngle::fromDeg(-37.72);
                }
                else if (pPlayer->pActor->spr.Angles.Pitch.Sgn() >= 0)
                {
                    if (!(pPlayerActor->sector()->Flag & kSectUnderwater))
                    {
                        SetNewWeapon(nPlayer, pPlayer->nDeathType + 8);
                    }
                }

                pPlayer->dVertPan--;
            }
        }
    }

    // loc_1C4E1
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
DEFINE_FIELD_X(ExhumedPlayer, Player, bPlayerPan);
DEFINE_FIELD_X(ExhumedPlayer, Player, bLockPan);

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
