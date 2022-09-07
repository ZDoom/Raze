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
#include "input.h"
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

extern int nStatusSeqOffset;

int lPlayerXVel = 0;
int lPlayerYVel = 0;
int obobangle = 0, bobangle  = 0;

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


int nLocalPlayer = 0;

Player PlayerList[kMaxPlayers];

TObjPtr<DExhumedActor*> nNetStartSprite[kMaxPlayers] = { };

int nStandHeight;



int PlayerCount;
int nNetStartSprites;
int nCurStartSprite;


size_t MarkPlayers()
{
    for (auto& p : PlayerList)
    {
        GC::Mark(p.pActor);
        GC::Mark(p.pDoppleSprite);
        GC::Mark(p.pPlayerFloorSprite);
        GC::Mark(p.pPlayerGrenade);
    }
    GC::MarkArray(nNetStartSprite, kMaxPlayers);
    return 5 * kMaxPlayers;
}

void SetSavePoint(int nPlayer, const DVector3& pos, sectortype* pSector, DAngle nAngle)
{
    PlayerList[nPlayer].sPlayerSave.pos = pos;
    PlayerList[nPlayer].sPlayerSave.pSector = pSector;
    PlayerList[nPlayer].sPlayerSave.nAngle = nAngle;
}

void feebtag(int x, int y, int z, sectortype* pSector, DExhumedActor **nSprite, int nVal2, int nVal3)
{
    *nSprite = nullptr;

    int startwall = pSector->wallptr;

    int nWalls = pSector->wallnum;

    int var_20 = nVal2 & 2;
    int var_14 = nVal2 & 1;

    while (1)
    {
        if (pSector != nullptr)
        {
            ExhumedSectIterator it(pSector);
            while (auto pActor = it.Next())
            {
                int nStat = pActor->spr.statnum;

                if (nStat >= 900 && !(pActor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
                {
                    uint32_t xDiff = abs(pActor->int_pos().X - x);
                    uint32_t yDiff = abs(pActor->int_pos().Y - y);
                    int zDiff = pActor->int_pos().Z - z;

                    if (zDiff < 5120 && zDiff > -25600)
                    {
                        uint32_t diff = xDiff * xDiff + yDiff * yDiff;

                        if (diff > INT_MAX)
                        {
                            DPrintf(DMSG_WARNING, "%s %d: overflow\n", __func__, __LINE__);
                            diff = INT_MAX;
                        }

                        int theSqrt = ksqrt(diff);

                        if (theSqrt < nVal3 && ((nStat != 950 && nStat != 949) || !(var_14 & 1)) && ((nStat != 912 && nStat != 913) || !(var_20 & 2)))
                        {
                            nVal3 = theSqrt;
                            *nSprite = pActor;
                        }
                    }
                }
            }
        }

        nWalls--;
        if (nWalls < -1)
            return;

        pSector = wall[startwall].nextSector();
        startwall++;
    }
}

void InitPlayer()
{
    for (int i = 0; i < kMaxPlayers; i++) {
        PlayerList[i].pActor = nullptr;
        PlayerList[i].pPlayerPushSect = nullptr;
        PlayerList[i].pPlayerViewSect = nullptr;
    }
}

void InitPlayerKeys(int nPlayer)
{
    PlayerList[nPlayer].keys = 0;
}

void InitPlayerInventory(int nPlayer)
{
    memset(&PlayerList[nPlayer], 0, sizeof(Player));

    PlayerList[nPlayer].nItem = -1;
    PlayerList[nPlayer].nPlayerSwear = 4;

    ResetPlayerWeapons(nPlayer);

    PlayerList[nPlayer].nLives = kDefaultLives;

    PlayerList[nPlayer].pActor = nullptr;
    PlayerList[nPlayer].nRun = -1;

    PlayerList[nPlayer].nPistolClip = 6;
    PlayerList[nPlayer].nPlayerClip = 0;

    PlayerList[nPlayer].nCurrentWeapon = 0;

    if (nPlayer == nLocalPlayer) {
        automapMode = am_off;
    }

    PlayerList[nPlayer].nPlayerScore = 0;

    auto pixels = tilePtr(kTile3571 + nPlayer);

    PlayerList[nPlayer].nPlayerColor = pixels[tileWidth(nPlayer + kTile3571) * tileHeight(nPlayer + kTile3571) / 2];
}

int GetPlayerFromActor(DExhumedActor* pActor)
{
    return RunData[pActor->spr.intowner].nObjIndex;
}

void RestartPlayer(int nPlayer)
{
	auto plr = &PlayerList[nPlayer];
    auto pActor = plr->pActor;
    DExhumedActor* pDopSprite = plr->pDoppleSprite;

    DExhumedActor* floorsprt;

	if (pActor)
	{
        runlist_DoSubRunRec(pActor->spr.intowner);
		runlist_FreeRun(pActor->spr.lotag - 1);

		ChangeActorStat(pActor, 0);

        plr->pActor = nullptr;

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
		plr->angle.ang = nNStartSprite->spr.angle;
		pActor->spr.angle = plr->angle.ang;

		floorsprt = insertActor(pActor->sector(), 0);

		floorsprt->spr.pos = pActor->spr.pos;
		floorsprt->spr.yrepeat = 64;
		floorsprt->spr.xrepeat = 64;
		floorsprt->spr.cstat = CSTAT_SPRITE_ALIGNMENT_FLOOR;
		floorsprt->spr.picnum = nPlayer + kTile3571;
	}
	else
	{
        pActor->spr.pos.XY() = plr->sPlayerSave.pos.XY();
		pActor->spr.pos.Z = plr->sPlayerSave.pSector->floorz;
		plr->angle.ang = plr->sPlayerSave.nAngle;
		pActor->spr.angle = plr->angle.ang;

		floorsprt = nullptr;
	}

	plr->angle.backup();
	plr->horizon.backup();

	plr->pPlayerFloorSprite = floorsprt;

	pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
	pActor->spr.shade = -12;
	pActor->spr.clipdist = 58;
	pActor->spr.pal = 0;
	pActor->spr.xrepeat = 40;
	pActor->spr.yrepeat = 40;
	pActor->spr.xoffset = 0;
	pActor->spr.yoffset = 0;
	pActor->spr.picnum = seq_GetSeqPicnum(kSeqJoe, 18, 0);

	int nHeight = GetActorHeight(pActor);
	pActor->vel.X = 0;
	pActor->vel.Y = 0;
	pActor->vel.Z = 0;

	nStandHeight = nHeight;

	pActor->spr.hitag = 0;
	pActor->spr.extra = -1;
	pActor->spr.lotag = runlist_HeadRun() + 1;

    pDActor->spr.pos = pActor->spr.pos;
	pDActor->spr.xrepeat = pActor->spr.xrepeat;
	pDActor->spr.yrepeat = pActor->spr.yrepeat;
	pDActor->spr.xoffset = 0;
	pDActor->spr.yoffset = 0;
	pDActor->spr.shade = pActor->spr.shade;
	pDActor->spr.angle = pActor->spr.angle;
	pDActor->spr.cstat = pActor->spr.cstat;

	pDActor->spr.lotag = runlist_HeadRun() + 1;

	plr->nAction = 0;
	plr->nHealth = 800; // TODO - define

	if (nNetPlayerCount) {
		plr->nHealth = 1600; // TODO - define
	}

	plr->nSeqSize = 0;
	plr->pActor = pActor;
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
	plr->oeyelevel = plr->eyelevel = -55.;
	dVertPan[nPlayer] = 0;

	nTemperature[nPlayer] = 0;

	plr->nDamage.Y = 0;
	plr->nDamage.X = 0;

	plr->nDestVertPan = plr->horizon.ohoriz = plr->horizon.horiz = q16horiz(0);
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

	memset(&sPlayerInput[nPlayer], 0, sizeof(PlayerInput));
	sPlayerInput[nPlayer].nItem = -1;

	plr->nDeathType = 0;
	nQuake[nPlayer] = 0;
}

int GrabPlayer()
{
    if (PlayerCount >= kMaxPlayers) {
        return -1;
    }

    return PlayerCount++;
}

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
        ThrowGrenade(nPlayer, 0, 0, 0, -10000);
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

    StopFiringWeapon(nPlayer);

    PlayerList[nPlayer].horizon.ohoriz = PlayerList[nPlayer].horizon.horiz = q16horiz(0);
    PlayerList[nPlayer].oeyelevel = PlayerList[nPlayer].eyelevel = -55;
    PlayerList[nPlayer].nInvisible = 0;
    dVertPan[nPlayer] = 15;

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

void ShootStaff(int nPlayer)
{
    PlayerList[nPlayer].nAction = 15;
    PlayerList[nPlayer].nSeqSize = 0;
    PlayerList[nPlayer].nSeq = kSeqJoe;
}

void PlayAlert(const char *str)
{
    StatusMessage(300, str);
    PlayLocalSound(StaticSound[kSound63], 0);
}


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

void UpdatePlayerSpriteAngle(Player* pPlayer)
{
    inita = pPlayer->angle.ang;
    if (pPlayer->pActor) pPlayer->pActor->spr.angle = inita;
}

void AIPlayer::Draw(RunListEvent* ev)
{
    int nPlayer = RunData[ev->nRun].nObjIndex;
    assert(nPlayer >= 0 && nPlayer < kMaxPlayers);
    int nAction = PlayerList[nPlayer].nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[PlayerList[nPlayer].nSeq] + PlayerSeq[nAction].a, PlayerList[nPlayer].nSeqSize, PlayerSeq[nAction].b);
}

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


void AIPlayer::Tick(RunListEvent* ev)
{
    int var_48 = 0;
    int var_40;
    bool mplevel = (currentLevel->gameflags & LEVEL_EX_MULTI);

    int nPlayer = RunData[ev->nRun].nObjIndex;
    assert(nPlayer >= 0 && nPlayer < kMaxPlayers);

    auto pPlayerActor = PlayerList[nPlayer].pActor;

    DExhumedActor* pDopple = PlayerList[nPlayer].pDoppleSprite;

    int nAction = PlayerList[nPlayer].nAction;
    int nActionB = PlayerList[nPlayer].nAction;

    PlayerList[nPlayer].angle.backup();
    PlayerList[nPlayer].horizon.backup();
    PlayerList[nPlayer].angle.resetadjustment();
    PlayerList[nPlayer].horizon.resetadjustment();
    PlayerList[nPlayer].oeyelevel = PlayerList[nPlayer].eyelevel;

    pPlayerActor->set_int_xvel(sPlayerInput[nPlayer].xVel >> 14);
    pPlayerActor->set_int_yvel(sPlayerInput[nPlayer].yVel >> 14);

    if (sPlayerInput[nPlayer].nItem > -1)
    {
        UseItem(nPlayer, sPlayerInput[nPlayer].nItem);
        sPlayerInput[nPlayer].nItem = -1;
    }

    pPlayerActor->spr.picnum = seq_GetSeqPicnum(PlayerList[nPlayer].nSeq, PlayerSeq[nHeightTemplate[nAction]].a, PlayerList[nPlayer].nSeqSize);
    pDopple->spr.picnum = pPlayerActor->spr.picnum;

    if (PlayerList[nPlayer].nTorch > 0)
    {
        PlayerList[nPlayer].nTorch--;
        if (PlayerList[nPlayer].nTorch == 0)
        {
            SetTorch(nPlayer, 0);
        }
        else
        {
            if (nPlayer != nLocalPlayer)
            {
                nFlashDepth = 5;
                AddFlash(pPlayerActor->sector(), pPlayerActor->spr.pos, 0);
            }
        }
    }

    if (PlayerList[nPlayer].nDouble > 0)
    {
        PlayerList[nPlayer].nDouble--;
        if (PlayerList[nPlayer].nDouble == 150 && nPlayer == nLocalPlayer) {
            PlayAlert(GStrings("TXT_EX_WEAPONEX"));
        }
    }

    if (PlayerList[nPlayer].nInvisible > 0)
    {
        PlayerList[nPlayer].nInvisible--;
        if (PlayerList[nPlayer].nInvisible == 0)
        {
            pPlayerActor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE; // set visible
            DExhumedActor* pFloorSprite = PlayerList[nPlayer].pPlayerFloorSprite;

            if (pFloorSprite != nullptr) {
                pFloorSprite->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE; // set visible
            }
        }
        else if (PlayerList[nPlayer].nInvisible == 150 && nPlayer == nLocalPlayer)
        {
            PlayAlert(GStrings("TXT_EX_INVISEX"));
        }
    }

    if (PlayerList[nPlayer].invincibility > 0)
    {
        PlayerList[nPlayer].invincibility--;
        if (PlayerList[nPlayer].invincibility == 150 && nPlayer == nLocalPlayer) {
            PlayAlert(GStrings("TXT_EX_INVINCEX"));
        }
    }

    if (nQuake[nPlayer] != 0)
    {
        nQuake[nPlayer] = -nQuake[nPlayer];
        if (nQuake[nPlayer] > 0)
        {
            nQuake[nPlayer] -= 2.;
            if (nQuake[nPlayer] < 0)
                nQuake[nPlayer] = 0;
        }
    }

    // loc_1A494:
    if (SyncInput())
    {
        Player* pPlayer = &PlayerList[nPlayer];
        pPlayer->angle.applyinput(sPlayerInput[nPlayer].nAngle, &sPlayerInput[nLocalPlayer].actions);
        UpdatePlayerSpriteAngle(pPlayer);
    }

    // player.zvel is modified within Gravity()
    int zVel = pPlayerActor->int_zvel();

    Gravity(pPlayerActor);

    if (pPlayerActor->vel.Z >= 6500/256. && zVel < 6500)
    {
        D3PlayFX(StaticSound[kSound17], pPlayerActor);
    }

    // loc_1A4E6
    auto pSector = pPlayerActor->sector();
    int nSectFlag = PlayerList[nPlayer].pPlayerViewSect->Flag;

    int playerX = pPlayerActor->int_pos().X;
    int playerY = pPlayerActor->int_pos().Y;

    int x = (sPlayerInput[nPlayer].xVel * 4) >> 2;
    int y = (sPlayerInput[nPlayer].yVel * 4) >> 2;
    int z = (pPlayerActor->int_zvel() * 4) >> 2;

    if (pPlayerActor->vel.Z > 32)
        pPlayerActor->set_int_zvel(8192);

    if (PlayerList[nPlayer].bIsMummified)
    {
        x /= 2;
        y /= 2;
    }

	auto spr_pos = pPlayerActor->spr.pos;
    auto spr_sect = pPlayerActor->sector();

    // TODO
    // nSectFlag & kSectUnderwater;

    zVel = pPlayerActor->int_zvel();

    Collision nMove;
    nMove.setNone();
    if (bSlipMode)
    {
        pPlayerActor->add_int_pos({ (x >> 14), (y >> 14), 0 });

        SetActor(pPlayerActor, pPlayerActor->spr.pos);
        pPlayerActor->spr.pos.Z = pPlayerActor->sector()->floorz;
    }
    else
    {
        nMove = movesprite(pPlayerActor, x, y, z, 5120, -5120, CLIPMASK0);

        auto pPlayerSect = pPlayerActor->sector();

        pushmove(pPlayerActor, &pPlayerSect, pPlayerActor->spr.clipdist << 2, 5120, -5120, CLIPMASK0);
        if (pPlayerSect != pPlayerActor->sector()) {
            ChangeActorSect(pPlayerActor, pPlayerSect);
        }
    }

    // loc_1A6E4
    if (inside(pPlayerActor->int_pos().X, pPlayerActor->int_pos().Y, pPlayerActor->sector()) != 1)
    {
        ChangeActorSect(pPlayerActor, spr_sect);

		pPlayerActor->spr.pos.XY() = spr_pos.XY();

        if (zVel < pPlayerActor->int_zvel()) {
            pPlayerActor->set_int_zvel(zVel);
        }
    }

    //			int _bTouchFloor = bTouchFloor;
    int bUnderwater = pPlayerActor->sector()->Flag & kSectUnderwater;

    if (bUnderwater)
    {
        PlayerList[nPlayer].nDamage.X /= 2;
        PlayerList[nPlayer].nDamage.Y /= 2;
    }

    // Trigger Ramses?
    if ((pPlayerActor->sector()->Flag & 0x8000) && bTouchFloor)
    {
        if (nTotalPlayers <= 1)
        {
            auto ang = GetAngleToSprite(pPlayerActor, pSpiritSprite) & kAngleMask;
            PlayerList[nPlayer].angle.settarget(DAngle::fromBuild(ang), true);
            pPlayerActor->set_int_ang(ang);

            PlayerList[nPlayer].horizon.settarget(buildhoriz(0), true);

            lPlayerXVel = 0;
            lPlayerYVel = 0;

            pPlayerActor->vel.X = 0;
            pPlayerActor->vel.Y = 0;
            pPlayerActor->vel.Z = 0;

            if (nFreeze < 1)
            {
                nFreeze = 1;
                StopAllSounds();
                StopLocalSound();
                InitSpiritHead();

                PlayerList[nPlayer].nDestVertPan = q16horiz(0);
                PlayerList[nPlayer].horizon.settarget(buildhoriz(currentLevel->ex_ramses_horiz));
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
            PlayerList[nPlayer].nDamage.X /= 2;
            PlayerList[nPlayer].nDamage.Y /= 2;

            if (nPlayer == nLocalPlayer)
            {
                int zVelB = zVel;

                if (zVelB < 0) {
                    zVelB = -zVelB;
                }

                if (zVelB > 512 && !PlayerList[nPlayer].horizon.horiz.asq16() && cl_slopetilting) {
                    PlayerList[nPlayer].nDestVertPan = q16horiz(0);
                }
            }

            if (zVel >= 6500)
            {
                pPlayerActor->vel.XY() *= 0.25;

                runlist_DamageEnemy(pPlayerActor, nullptr, ((zVel - 6500) >> 7) + 10);

                if (PlayerList[nPlayer].nHealth <= 0)
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

        if (nMove.type == kHitSector || nMove.type == kHitWall)
        {
            sectortype* sect;
            int nNormal = 0;

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

            if (sect != nullptr)
            {
                if ((sect->hitag == 45) && bTouchFloor)
                {
                    int nDiff = AngleDiff(DAngle::fromBuild(nNormal), DAngle::fromBuild((pPlayerActor->int_ang() + 1024) & kAngleMask));

                    if (nDiff < 0) {
                        nDiff = -nDiff;
                    }

                    if (nDiff <= 256)
                    {
                        PlayerList[nPlayer].pPlayerPushSect = sect;

                        int xvel = sPlayerInput[nPlayer].xVel;
                        int yvel = sPlayerInput[nPlayer].yVel;
                        int nMyAngle = getangle(xvel, yvel);

                        setsectinterpolate(sect);
                        MoveSector(sect, nMyAngle, &xvel, &yvel);

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

                        movesprite(pPlayerActor, xvel, yvel, z, 5120, -5120, CLIPMASK0);
                        goto sectdone;
                    }
                }
            }
        }
    }

    // loc_1AB46:
    if (PlayerList[nPlayer].nPlayerPushSound > -1)
    {
        if (PlayerList[nPlayer].pPlayerPushSect != nullptr)
        {
            StopActorSound(sBlockInfo[PlayerList[nPlayer].pPlayerPushSect->extra].pActor);
        }

        PlayerList[nPlayer].nPlayerPushSound = -1;
    }

sectdone:
    if (!PlayerList[nPlayer].bPlayerPan && !PlayerList[nPlayer].bLockPan)
    {
        PlayerList[nPlayer].nDestVertPan = q16horiz(clamp((int(spr_pos.Z * zworldtoint) - pPlayerActor->int_pos().Z) << 9, gi->playerHorizMin(), gi->playerHorizMax()));
    }

    playerX -= pPlayerActor->int_pos().X;
    playerY -= pPlayerActor->int_pos().Y;

    uint32_t sqrtNum = playerX * playerX + playerY * playerY;

    if (sqrtNum > INT_MAX)
    {
        DPrintf(DMSG_WARNING, "%s %d: overflow\n", __func__, __LINE__);
        sqrtNum = INT_MAX;
    }

    PlayerList[nPlayer].ototalvel = PlayerList[nPlayer].totalvel;
    PlayerList[nPlayer].totalvel = ksqrt(sqrtNum);

    auto pViewSect = pPlayerActor->sector();

    double EyeZ = PlayerList[nPlayer].eyelevel + pPlayerActor->spr.pos.Z + nQuake[nPlayer];

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

                auto coll = movesprite(pPlayerActor, x, y, 0, 5120, 0, CLIPMASK0);
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
    PlayerList[nPlayer].pPlayerViewSect = pViewSect;

    PlayerList[nPlayer].nPlayerD.X = int((pPlayerActor->spr.pos.X - spr_pos.X) * worldtoint);
    PlayerList[nPlayer].nPlayerD.Y = int((pPlayerActor->spr.pos.Y - spr_pos.Y) * worldtoint);

    int var_5C = pViewSect->Flag & kSectUnderwater;

    auto actions = sPlayerInput[nPlayer].actions;

    // loc_1AEF5:
    if (PlayerList[nPlayer].nHealth > 0)
    {
        if (PlayerList[nPlayer].nMaskAmount > 0)
        {
            PlayerList[nPlayer].nMaskAmount--;
            if (PlayerList[nPlayer].nMaskAmount == 150 && nPlayer == nLocalPlayer) {
				PlayAlert(GStrings("TXT_EX_MASKEX"));
            }
        }

        if (!PlayerList[nPlayer].invincibility)
        {
            // Handle air
            PlayerList[nPlayer].nBreathTimer--;

            if (PlayerList[nPlayer].nBreathTimer <= 0)
            {
                PlayerList[nPlayer].nBreathTimer = 90;

                // if underwater
                if (var_5C)
                {
                    if (PlayerList[nPlayer].nMaskAmount > 0)
                    {
                        D3PlayFX(StaticSound[kSound30], pPlayerActor);

                        PlayerList[nPlayer].nAir = 100;
                    }
                    else
                    {
                        PlayerList[nPlayer].nAir -= 25;
                        if (PlayerList[nPlayer].nAir > 0)
                        {
                            D3PlayFX(StaticSound[kSound25], pPlayerActor);
                        }
                        else
                        {
                            PlayerList[nPlayer].nHealth += (PlayerList[nPlayer].nAir << 2);
                            if (PlayerList[nPlayer].nHealth <= 0)
                            {
                                PlayerList[nPlayer].nHealth = 0;
                                StartDeathSeq(nPlayer, 0);
                            }

                            PlayerList[nPlayer].nAir = 0;

                            if (PlayerList[nPlayer].nHealth < 300)
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
            if (PlayerList[nPlayer].nTorch > 0)
            {
                PlayerList[nPlayer].nTorch = 0;
                SetTorch(nPlayer, 0);
            }
        }
        else
        {
            auto pTmpSect = pPlayerActor->sector();

            if (PlayerList[nPlayer].totalvel > 25 && pPlayerActor->spr.pos.Z > pTmpSect->floorz)
            {
                if (pTmpSect->Depth && !pTmpSect->Speed && !pTmpSect->Damage)
                {
                    D3PlayFX(StaticSound[kSound42], pPlayerActor);
                }
            }

            // CHECKME - wrong place?
            if (nSectFlag & kSectUnderwater)
            {
                if (PlayerList[nPlayer].nAir < 50)
                {
                    D3PlayFX(StaticSound[kSound14], pPlayerActor);
                }

                PlayerList[nPlayer].nBreathTimer = 1;
            }

            PlayerList[nPlayer].nBreathTimer--;
            if (PlayerList[nPlayer].nBreathTimer <= 0)
            {
                PlayerList[nPlayer].nBreathTimer = 90;
            }

            if (PlayerList[nPlayer].nAir < 100)
            {
                PlayerList[nPlayer].nAir = 100;
            }
        }

        // loc_1B1EB
        DExhumedActor* pFloorActor = PlayerList[nPlayer].pPlayerFloorSprite;
        if (nTotalPlayers > 1 && pFloorActor)
        {
            pFloorActor->spr.pos.XY() = pPlayerActor->spr.pos.XY();

            if (pFloorActor->sector() != pPlayerActor->sector())
            {
                ChangeActorSect(pFloorActor, pPlayerActor->sector());
            }

			pFloorActor->spr.pos.Z = pPlayerActor->sector()->floorz;
        }

        int var_30 = 0;

        if (PlayerList[nPlayer].nHealth >= 800)
        {
            var_30 = 2;
        }

        if (PlayerList[nPlayer].nMagic >= 1000)
        {
            var_30 |= 1;
        }

        // code to handle item pickup?
        HitInfo near;

        // neartag finds the nearest sector, wall, and sprite which has its hitag and/or lotag set to a value.
        neartag(pPlayerActor->int_pos(), pPlayerActor->sector(), pPlayerActor->int_ang(), near, 1024, 2);

        DExhumedActor* pActorB;
        feebtag(pPlayerActor->int_pos().X, pPlayerActor->int_pos().Y, pPlayerActor->int_pos().Z, pPlayerActor->sector(), &pActorB, var_30, 768);

        // Item pickup code
        if (pActorB != nullptr && pActorB->spr.statnum >= 900)
        {
            int var_8C = 16;
            int var_88 = 9;

            int var_70 = pActorB->spr.statnum - 900;
            int var_44 = 0;

            // item lotags start at 6 (1-5 reserved?) so 0-offset them
            int itemtype = var_70 - 6;

            if (itemtype <= 54)
            {
                switch (itemtype)
                {
                do_default:
                default:
                {
                    // loc_1B3C7

                    // CHECKME - is order of evaluation correct?
                    if (!mplevel || (var_70 >= 25 && (var_70 <= 25 || var_70 == 50)))
                    {
                        // If this is an anim we need to properly destroy it so we need to do some proper detection and not wild guesses.
                        if (pActorB->nRun == pActorB->nDamage && pActorB->nRun != 0 && pActorB->nPhase == ITEM_MAGIC)
                            DestroyAnim(pActorB);
                        else
                            DeleteActor(pActorB);
                    }
                    else
                    {
                        StartRegenerate(pActorB);
                    }
                do_default_b:
                    // loc_1BA74
                    if (nPlayer == nLocalPlayer)
                    {
                        if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                        {
                            pickupMessage(var_70);
                        }

                        TintPalette(var_44 * 4, var_8C * 4, 0);

                        if (var_88 > -1)
                        {
                            PlayLocalSound(var_88, 0);
                        }
                    }

                    break;
                }
                case 0: // Speed Loader
                {
                    if (AddAmmo(nPlayer, 1, pActorB->spr.hitag))
                    {
                        var_88 = StaticSound[kSoundAmmoPickup];
                        goto do_default;
                    }

                    break;
                }
                case 1: // Fuel Canister
                {
                    if (AddAmmo(nPlayer, 3, pActorB->spr.hitag))
                    {
                        var_88 = StaticSound[kSoundAmmoPickup];
                        goto do_default;
                    }
                    break;
                }
                case 2: // M - 60 Ammo Belt
                {
                    if (AddAmmo(nPlayer, 2, pActorB->spr.hitag))
                    {
                        var_88 = StaticSound[kSoundAmmoPickup];
                        CheckClip(nPlayer);
                        goto do_default;
                    }
                    break;
                }
                case 3: // Grenade
                case 21:
                case 49:
                {
                    if (AddAmmo(nPlayer, 4, 1))
                    {
                        var_88 = StaticSound[kSoundAmmoPickup];
                        if (!(PlayerList[nPlayer].nPlayerWeapons & 0x10))
                        {
                            PlayerList[nPlayer].nPlayerWeapons |= 0x10;
                            SetNewWeaponIfBetter(nPlayer, 4);
                        }

                        if (var_70 == 55)
                        {
                            pActorB->spr.cstat = CSTAT_SPRITE_INVISIBLE;
                            DestroyItemAnim(pActorB);

                            // loc_1BA74: - repeated block, see in default case
                            if (nPlayer == nLocalPlayer)
                            {
                                if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                                {
                                    pickupMessage(var_70);
                                }

                                TintPalette(var_44 * 4, var_8C * 4, 0);

                                if (var_88 > -1)
                                {
                                    PlayLocalSound(var_88, 0);
                                }
                            }
                            break;
                        }
                        else
                        {
                            goto do_default;
                        }
                    }
                    break;
                }

                case 4: // Pickable item
                case 9: // Pickable item
                case 10: // Reserved
                case 18:
                case 25:
                case 28:
                case 29:
                case 30:
                case 33:
                case 34:
                case 35:
                case 36:
                case 37:
                case 38:
                case 45:
                case 52:
                {
                    goto do_default;
                }

                case 5: // Map
                {
                    GrabMap();
                    goto do_default;
                }

                case 6: // Berry Twig
                {
                    if (pActorB->spr.hitag == 0) {
                        break;
                    }

                    var_88 = 20;
                    int edx = 40;

                    if (edx <= 0 || (!(var_30 & 2)))
                    {
                        if (!PlayerList[nPlayer].invincibility || edx > 0)
                        {
                            PlayerList[nPlayer].nHealth += edx;
                            if (PlayerList[nPlayer].nHealth > 800)
                            {
                                PlayerList[nPlayer].nHealth = 800;
                            }
                            else
                            {
                                if (PlayerList[nPlayer].nHealth < 0)
                                {
                                    var_88 = -1;
                                    StartDeathSeq(nPlayer, 0);
                                }
                            }
                        }

                        if (var_70 == 12)
                        {
                            pActorB->spr.hitag = 0;
                            pActorB->spr.picnum++;

                            ChangeActorStat(pActorB, 0);

                            // loc_1BA74: - repeated block, see in default case
                            if (nPlayer == nLocalPlayer)
                            {
                                if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                                {
                                    pickupMessage(var_70);
                                }

                                TintPalette(var_44 * 4, var_8C * 4, 0);

                                if (var_88 > -1)
                                {
                                    PlayLocalSound(var_88, 0);
                                }
                            }

                            break;
                        }
                        else
                        {
                            if (var_70 != 14)
                            {
                                var_88 = 21;
                            }
                            else
                            {
                                var_44 = var_8C;
                                var_88 = 22;
                                var_8C = 0;
                            }

                            goto do_default;
                        }
                    }

                    break;
                }

                case 7: // Blood Bowl
                {
                    int edx = 160;

                    // Same code as case 6 now till break
                    if (edx <= 0 || (!(var_30 & 2)))
                    {
                        if (!PlayerList[nPlayer].invincibility || edx > 0)
                        {
                            PlayerList[nPlayer].nHealth += edx;
                            if (PlayerList[nPlayer].nHealth > 800)
                            {
                                PlayerList[nPlayer].nHealth = 800;
                            }
                            else
                            {
                                if (PlayerList[nPlayer].nHealth < 0)
                                {
                                    var_88 = -1;
                                    StartDeathSeq(nPlayer, 0);
                                }
                            }
                        }

                        if (var_70 == 12)
                        {
                            pActorB->spr.hitag = 0;
                            pActorB->spr.picnum++;

                            ChangeActorStat(pActorB, 0);

                            // loc_1BA74: - repeated block, see in default case
                            if (nPlayer == nLocalPlayer)
                            {
                                if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                                {
                                    pickupMessage(var_70);
                                }

                                TintPalette(var_44 * 4, var_8C * 4, 0);

                                if (var_88 > -1)
                                {
                                    PlayLocalSound(var_88, 0);
                                }
                            }

                            break;
                        }
                        else
                        {
                            if (var_70 != 14)
                            {
                                var_88 = 21;
                            }
                            else
                            {
                                var_44 = var_8C;
                                var_88 = 22;
                                var_8C = 0;
                            }

                            goto do_default;
                        }
                    }

                    break;
                }

                case 8: // Cobra Venom Bowl
                {
                    int edx = -200;

                    // Same code as case 6 and 7 from now till break
                    if (edx <= 0 || (!(var_30 & 2)))
                    {
                        if (!PlayerList[nPlayer].invincibility || edx > 0)
                        {
                            PlayerList[nPlayer].nHealth += edx;
                            if (PlayerList[nPlayer].nHealth > 800)
                            {
                                PlayerList[nPlayer].nHealth = 800;
                            }
                            else
                            {
                                if (PlayerList[nPlayer].nHealth < 0)
                                {
                                    var_88 = -1;
                                    StartDeathSeq(nPlayer, 0);
                                }
                            }
                        }

                        if (var_70 == 12)
                        {
                            pActorB->spr.hitag = 0;
                            pActorB->spr.picnum++;

                            ChangeActorStat(pActorB, 0);

                            // loc_1BA74: - repeated block, see in default case
                            if (nPlayer == nLocalPlayer)
                            {
                                if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                                {
                                    pickupMessage(var_70);
                                }

                                TintPalette(var_44 * 4, var_8C * 4, 0);

                                if (var_88 > -1)
                                {
                                    PlayLocalSound(var_88, 0);
                                }
                            }

                            break;
                        }
                        else
                        {
                            if (var_70 != 14)
                            {
                                var_88 = 21;
                            }
                            else
                            {
                                var_44 = var_8C;
                                var_88 = 22;
                                var_8C = 0;
                            }

                            goto do_default;
                        }
                    }

                    break;
                }

                case 11: // Bubble Nest
                {
                    PlayerList[nPlayer].nAir += 10;
                    if (PlayerList[nPlayer].nAir > 100) {
                        PlayerList[nPlayer].nAir = 100; // TODO - constant
                    }

                    if (PlayerList[nPlayer].nBreathTimer < 89)
                    {
                        D3PlayFX(StaticSound[kSound13], pPlayerActor);
                    }

                    PlayerList[nPlayer].nBreathTimer = 90;
                    break;
                }

                case 12: // Still Beating Heart
                {
                    if (GrabItem(nPlayer, kItemHeart)) {
                        goto do_default;
                    }

                    break;
                }

                case 13: // Scarab amulet(Invicibility)
                {
                    if (GrabItem(nPlayer, kItemInvincibility)) {
                        goto do_default;
                    }

                    break;
                }

                case 14: // Severed Slave Hand(double damage)
                {
                    if (GrabItem(nPlayer, kItemDoubleDamage)) {
                        goto do_default;
                    }

                    break;
                }

                case 15: // Unseen eye(Invisibility)
                {
                    if (GrabItem(nPlayer, kItemInvisibility)) {
                        goto do_default;
                    }

                    break;
                }

                case 16: // Torch
                {
                    if (GrabItem(nPlayer, kItemTorch)) {
                        goto do_default;
                    }

                    break;
                }

                case 17: // Sobek Mask
                {
                    if (GrabItem(nPlayer, kItemMask)) {
                        goto do_default;
                    }

                    break;
                }

                case 19: // Extra Life
                {
                    var_88 = -1;

                    if (PlayerList[nPlayer].nLives >= kMaxPlayerLives) {
                        break;
                    }

                    PlayerList[nPlayer].nLives++;

                    var_8C = 32;
                    var_44 = 32;
                    goto do_default;
                }

                // FIXME - lots of repeated code from here down!!
                case 20: // sword pickup??
                {
                    var_40 = 0;
                    int ebx = 0;

                    // loc_1B75D
                    int var_18 = 1 << var_40;

                    int weapons = PlayerList[nPlayer].nPlayerWeapons;

                    if (weapons & var_18)
                    {
                        if (mplevel)
                        {
                            AddAmmo(nPlayer, WeaponInfo[var_40].nAmmoType, ebx);
                        }
                    }
                    else
                    {
                        weapons = var_40;

                        SetNewWeaponIfBetter(nPlayer, weapons);

                        PlayerList[nPlayer].nPlayerWeapons |= var_18;

                        AddAmmo(nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                        var_88 = StaticSound[kSound72];
                    }

                    if (var_40 == 2) {
                        CheckClip(nPlayer);
                    }

                    if (var_70 <= 50) {
                        goto do_default;
                    }

                    pActorB->spr.cstat = CSTAT_SPRITE_INVISIBLE;
                    DestroyItemAnim(pActorB);
                    ////
                            // loc_1BA74: - repeated block, see in default case
                    if (nPlayer == nLocalPlayer)
                    {
                        if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                        {
                            pickupMessage(var_70);
                        }

                        TintPalette(var_44 * 4, var_8C * 4, 0);

                        if (var_88 > -1)
                        {
                            PlayLocalSound(var_88, 0);
                        }
                    }

                    break;
                    /////
                }

                case 22: // .357 Magnum Revolver
                case 46:
                {
                    var_40 = 1;
                    int ebx = 6;

                    // loc_1B75D
                    int var_18 = 1 << var_40;

                    int weapons = PlayerList[nPlayer].nPlayerWeapons;

                    if (weapons & var_18)
                    {
                        if (mplevel)
                        {
                            AddAmmo(nPlayer, WeaponInfo[var_40].nAmmoType, ebx);
                        }
                    }
                    else
                    {
                        weapons = var_40;

                        SetNewWeaponIfBetter(nPlayer, weapons);

                        PlayerList[nPlayer].nPlayerWeapons |= var_18;

                        AddAmmo(nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                        var_88 = StaticSound[kSound72];
                    }

                    if (var_40 == 2) {
                        CheckClip(nPlayer);
                    }

                    if (var_70 <= 50) {
                        goto do_default;
                    }

                    pActorB->spr.cstat = CSTAT_SPRITE_INVISIBLE;
                    DestroyItemAnim(pActorB);
                    ////
                            // loc_1BA74: - repeated block, see in default case
                    if (nPlayer == nLocalPlayer)
                    {
                        if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                        {
                            pickupMessage(var_70);
                        }

                        TintPalette(var_44 * 4, var_8C * 4, 0);

                        if (var_88 > -1)
                        {
                            PlayLocalSound(var_88, 0);
                        }
                    }

                    break;
                    /////
                }

                case 23: // M - 60 Machine Gun
                case 47:
                {
                    var_40 = 2;
                    int ebx = 24;

                    // loc_1B75D
                    int var_18 = 1 << var_40;

                    int weapons = PlayerList[nPlayer].nPlayerWeapons;

                    if (weapons & var_18)
                    {
                        if (mplevel)
                        {
                            AddAmmo(nPlayer, WeaponInfo[var_40].nAmmoType, ebx);
                        }
                    }
                    else
                    {
                        weapons = var_40;

                        SetNewWeaponIfBetter(nPlayer, weapons);

                        PlayerList[nPlayer].nPlayerWeapons |= var_18;

                        AddAmmo(nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                        var_88 = StaticSound[kSound72];
                    }

                    if (var_40 == 2) {
                        CheckClip(nPlayer);
                    }

                    if (var_70 <= 50) {
                        goto do_default;
                    }

                    pActorB->spr.cstat = CSTAT_SPRITE_INVISIBLE;
                    DestroyItemAnim(pActorB);
                    ////
                            // loc_1BA74: - repeated block, see in default case
                    if (nPlayer == nLocalPlayer)
                    {
                        if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                        {
                            pickupMessage(var_70);
                        }

                        TintPalette(var_44 * 4, var_8C * 4, 0);

                        if (var_88 > -1)
                        {
                            PlayLocalSound(var_88, 0);
                        }
                    }

                    break;
                    /////
                }

                case 24: // Flame Thrower
                case 48:
                {
                    var_40 = 3;
                    int ebx = 100;

                    // loc_1B75D
                    int var_18 = 1 << var_40;

                    int weapons = PlayerList[nPlayer].nPlayerWeapons;

                    if (weapons & var_18)
                    {
                        if (mplevel)
                        {
                            AddAmmo(nPlayer, WeaponInfo[var_40].nAmmoType, ebx);
                        }
                    }
                    else
                    {
                        weapons = var_40;

                        SetNewWeaponIfBetter(nPlayer, weapons);

                        PlayerList[nPlayer].nPlayerWeapons |= var_18;

                        AddAmmo(nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                        var_88 = StaticSound[kSound72];
                    }

                    if (var_40 == 2) {
                        CheckClip(nPlayer);
                    }

                    if (var_70 <= 50) {
                        goto do_default;
                    }

                    pActorB->spr.cstat = CSTAT_SPRITE_INVISIBLE;
                    DestroyItemAnim(pActorB);
                    ////
                            // loc_1BA74: - repeated block, see in default case
                    if (nPlayer == nLocalPlayer)
                    {
                        if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                        {
                            pickupMessage(var_70);
                        }

                        TintPalette(var_44 * 4, var_8C * 4, 0);

                        if (var_88 > -1)
                        {
                            PlayLocalSound(var_88, 0);
                        }
                    }

                    break;
                    /////
                }

                case 26: // Cobra Staff
                case 50:
                {
                    var_40 = 5;
                    int ebx = 20;

                    // loc_1B75D
                    int var_18 = 1 << var_40;

                    int weapons = PlayerList[nPlayer].nPlayerWeapons;

                    if (weapons & var_18)
                    {
                        if (mplevel)
                        {
                            AddAmmo(nPlayer, WeaponInfo[var_40].nAmmoType, ebx);
                        }
                    }
                    else
                    {
                        weapons = var_40;

                        SetNewWeaponIfBetter(nPlayer, weapons);

                        PlayerList[nPlayer].nPlayerWeapons |= var_18;

                        AddAmmo(nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                        var_88 = StaticSound[kSound72];
                    }

                    if (var_40 == 2) {
                        CheckClip(nPlayer);
                    }

                    if (var_70 <= 50) {
                        goto do_default;
                    }

                    pActorB->spr.cstat = CSTAT_SPRITE_INVISIBLE;
                    DestroyItemAnim(pActorB);
                    ////
                            // loc_1BA74: - repeated block, see in default case
                    if (nPlayer == nLocalPlayer)
                    {
                        if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                        {
                            pickupMessage(var_70);
                        }

                        TintPalette(var_44 * 4, var_8C * 4, 0);

                        if (var_88 > -1)
                        {
                            PlayLocalSound(var_88, 0);
                        }
                    }

                    break;
                    /////
                }

                case 27: // Eye of Ra Gauntlet
                case 51:
                {
                    var_40 = 6;
                    int ebx = 2;

                    // loc_1B75D
                    int var_18 = 1 << var_40;

                    int weapons = PlayerList[nPlayer].nPlayerWeapons;

                    if (weapons & var_18)
                    {
                        if (mplevel)
                        {
                            AddAmmo(nPlayer, WeaponInfo[var_40].nAmmoType, ebx);
                        }
                    }
                    else
                    {
                        weapons = var_40;

                        SetNewWeaponIfBetter(nPlayer, weapons);

                        PlayerList[nPlayer].nPlayerWeapons |= var_18;

                        AddAmmo(nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                        var_88 = StaticSound[kSound72];
                    }

                    if (var_40 == 2) {
                        CheckClip(nPlayer);
                    }

                    if (var_70 <= 50) {
                        goto do_default;
                    }

                    pActorB->spr.cstat = CSTAT_SPRITE_INVISIBLE;
                    DestroyItemAnim(pActorB);
                    ////
                            // loc_1BA74: - repeated block, see in default case
                    if (nPlayer == nLocalPlayer)
                    {
                        if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                        {
                            pickupMessage(var_70);
                        }

                        TintPalette(var_44 * 4, var_8C * 4, 0);

                        if (var_88 > -1)
                        {
                            PlayLocalSound(var_88, 0);
                        }
                    }

                    break;
                    /////
                }

                case 31: // Cobra staff ammo
                {
                    if (AddAmmo(nPlayer, 5, 1)) {
                        var_88 = StaticSound[kSoundAmmoPickup];
                        goto do_default;
                    }

                    break;
                }

                case 32: // Raw Energy
                {
                    if (AddAmmo(nPlayer, 6, pActorB->spr.hitag)) {
                        var_88 = StaticSound[kSoundAmmoPickup];
                        goto do_default;
                    }

                    break;
                }

                case 39: // Power key
                case 40: // Time key
                case 41: // War key
                case 42: // Earth key
                {
                    int keybit = 4096 << (itemtype - 39);

                    var_88 = -1;

                    if (!(PlayerList[nPlayer].keys & keybit))
                    {
                        PlayerList[nPlayer].keys |= keybit;

                        if (nTotalPlayers > 1)
                        {
                            goto do_default_b;
                        }
                        else
                        {
                            goto do_default;
                        }
                    }

                    break;
                }

                case 43: // Magical Essence
                case 44: // ?
                {
                    if (PlayerList[nPlayer].nMagic >= 1000) {
                        break;
                    }

                    var_88 = StaticSound[kSoundMana1];

                    PlayerList[nPlayer].nMagic += 100;
                    if (PlayerList[nPlayer].nMagic >= 1000) {
                        PlayerList[nPlayer].nMagic = 1000;
                    }

                    goto do_default;
                }

                case 53: // Scarab (Checkpoint)
                {
                    if (nLocalPlayer == nPlayer)
                    {
                        pActorB->nIndex2++;
                        pActorB->nAction &= 0xEF;
                        pActorB->nIndex = 0;

                        ChangeActorStat(pActorB, 899);
                    }

                    SetSavePoint(nPlayer, pPlayerActor->spr.pos, pPlayerActor->sector(), pPlayerActor->spr.angle);
                    break;
                }

                case 54: // Golden Sarcophagus (End Level)
                {
                    if (!bInDemo)
                    {
                        LevelFinished();
                    }

                    DestroyItemAnim(pActorB);
                    DeleteActor(pActorB);
                    break;
                }
                }
            }
        }

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

        if (!PlayerList[nPlayer].bIsMummified)
        {
            if (actions & SB_OPEN)
            {
                ClearSpaceBar(nPlayer);

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

            // was int var_38 = buttons & 0x8
            if (actions & SB_FIRE)
            {
                FireWeapon(nPlayer);
            }
            else
            {
                StopFiringWeapon(nPlayer);
            }

            // loc_1BC57:

            // CHECKME - are we finished with 'nSector' variable at this point? if so, maybe set it to pPlayerActor->spr.sector so we can make this code a bit neater. Don't assume pPlayerActor->spr.sector == nSector here!!
            if (nStandHeight > (pPlayerActor->sector()->int_floorz() - pPlayerActor->sector()->int_ceilingz())) {
                var_48 = 1;
            }

            // Jumping
            if (actions & SB_JUMP)
            {
                if (bUnderwater)
                {
                    pPlayerActor->set_int_zvel(-2048);
                    nActionB = 10;
                }
                else if (bTouchFloor)
                {
                    if (nAction < 6 || nAction > 8)
                    {
                        pPlayerActor->set_int_zvel(-3584);
                        nActionB = 3;
                    }
                }

                // goto loc_1BE70:
            }
            else if (actions & SB_CROUCH)
            {
                if (bUnderwater)
                {
                    pPlayerActor->set_int_zvel(2048);
                    nActionB = 10;
                }
                else
                {
                    if (PlayerList[nPlayer].eyelevel < -32.5) {
                        PlayerList[nPlayer].eyelevel += ((-32.5 - PlayerList[nPlayer].eyelevel) * 0.5);
                    }

                loc_1BD2E:
                    if (PlayerList[nPlayer].totalvel < 1) {
                        nActionB = 6;
                    }
                    else {
                        nActionB = 7;
                    }
                }

                // goto loc_1BE70:
            }
            else
            {
                if (PlayerList[nPlayer].nHealth > 0)
                {
                    PlayerList[nPlayer].eyelevel += (nActionEyeLevel[nAction] - PlayerList[nPlayer].eyelevel) * 0.5;

                    if (bUnderwater)
                    {
                        if (PlayerList[nPlayer].totalvel <= 1)
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
                            if (PlayerList[nPlayer].totalvel <= 1) {
                                nActionB = 0;//bUnderwater; // this is just setting to 0
                            }
                            else if (PlayerList[nPlayer].totalvel <= 30) {
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
                if (actions & SB_FIRE) // was var_38
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
            uint8_t var_90 = sPlayerInput[nPlayer].getNewWeapon();

            if (var_90)
            {
                var_90--;

                if (PlayerList[nPlayer].nPlayerWeapons & (1 << var_90))
                {
                    SetNewWeapon(nPlayer, var_90);
                }
            }
        }
        else // player is mummified
        {
            if (actions & SB_FIRE)
            {
                FireWeapon(nPlayer);
            }

            if (nAction != 15)
            {
                if (PlayerList[nPlayer].totalvel <= 1)
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
        if (nActionB != nAction && nAction != 4)
        {
            nAction = nActionB;
            PlayerList[nPlayer].nAction = nActionB;
            PlayerList[nPlayer].nSeqSize = 0;
        }

        Player* pPlayer = &PlayerList[nPlayer];

        if (SyncInput())
        {
            pPlayer->horizon.applyinput(sPlayerInput[nPlayer].pan, &sPlayerInput[nLocalPlayer].actions);
        }

        if (actions & (SB_AIM_UP | SB_AIM_DOWN) || sPlayerInput[nPlayer].pan)
        {
            pPlayer->nDestVertPan = pPlayer->horizon.horiz;
            pPlayer->bPlayerPan = pPlayer->bLockPan = true;
        }
        else if (actions & (SB_LOOK_UP | SB_LOOK_DOWN | SB_CENTERVIEW))
        {
            pPlayer->nDestVertPan = pPlayer->horizon.horiz;
            pPlayer->bPlayerPan = pPlayer->bLockPan = false;
        }

        if (PlayerList[nPlayer].totalvel > 20)
        {
            pPlayer->bPlayerPan = false;
        }

        if (cl_slopetilting && !pPlayer->bPlayerPan && !pPlayer->bLockPan)
        {
            double nVertPan = (pPlayer->nDestVertPan - pPlayer->horizon.horiz).asbuildf() * 0.25;
            if (nVertPan != 0)
            {
                pPlayer->horizon.addadjustment(buildfhoriz(abs(nVertPan) >= 4 ? clamp(nVertPan, -4., 4.) : nVertPan * 2.));
            }
        }
    }
    else // else, player's health is less than 0
    {
        // loc_1C0E9
        if (actions & SB_OPEN)
        {
            ClearSpaceBar(nPlayer);

            if (nAction >= 16)
            {
                if (nPlayer == nLocalPlayer)
                {
                    StopAllSounds();
                    StopLocalSound();
                    GrabPalette();
                }

                PlayerList[nPlayer].nCurrentWeapon = PlayerList[nPlayer].nPlayerOldWeapon;

                if (PlayerList[nPlayer].nLives && nNetTime)
                {
                    if (nAction != 20)
                    {
                        pPlayerActor->spr.picnum = seq_GetSeqPicnum(kSeqJoe, 120, 0);
                        pPlayerActor->spr.cstat = 0;
						pPlayerActor->spr.pos.Z = pPlayerActor->sector()->floorz;
                    }

                    // will invalidate nPlayerSprite
                    RestartPlayer(nPlayer);

                    pPlayerActor = PlayerList[nPlayer].pActor;
                    pDopple = PlayerList[nPlayer].pDoppleSprite;
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

    int var_AC = SeqOffsets[PlayerList[nPlayer].nSeq] + PlayerSeq[nAction].a;

    seq_MoveSequence(pPlayerActor, var_AC, PlayerList[nPlayer].nSeqSize);
    PlayerList[nPlayer].nSeqSize++;

    if (PlayerList[nPlayer].nSeqSize >= SeqSize[var_AC])
    {
        PlayerList[nPlayer].nSeqSize = 0;

        switch (PlayerList[nPlayer].nAction)
        {
        default:
            break;

        case 3:
            PlayerList[nPlayer].nSeqSize = SeqSize[var_AC] - 1;
            break;
        case 4:
            PlayerList[nPlayer].nAction = 0;
            break;
        case 16:
            PlayerList[nPlayer].nSeqSize = SeqSize[var_AC] - 1;

            if (pPlayerActor->spr.pos.Z < pPlayerActor->sector()->floorz) 
            {
				pPlayerActor->spr.pos.Z++;
            }

            if (!RandomSize(5))
            {
                sectortype* mouthSect;
                auto pos = WheresMyMouth(nPlayer, &mouthSect);

                BuildAnim(nullptr, 71, 0, DVector3(pos.XY(), pPlayerActor->spr.pos.Z + 15), mouthSect, 75, 128);
            }
            break;
        case 17:
            PlayerList[nPlayer].nAction = 18;
            break;
        case 19:
            pPlayerActor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
            PlayerList[nPlayer].nAction = 20;
            break;
        }
    }

    // loc_1C3B4:
    if (nPlayer == nLocalPlayer)
    {
        initpos = pPlayerActor->spr.pos;
        initsectp = pPlayerActor->sector();
        inita = pPlayerActor->spr.angle;
    }

    if (!PlayerList[nPlayer].nHealth)
    {
        PlayerList[nPlayer].nDamage.Y = 0;
        PlayerList[nPlayer].nDamage.X = 0;

        if (PlayerList[nPlayer].eyelevel >= -11)
        {
            PlayerList[nPlayer].eyelevel = -11;
            dVertPan[nPlayer] = 0;
        }
        else
        {
            if (PlayerList[nPlayer].horizon.horiz.asq16() < 0)
            {
                PlayerList[nPlayer].horizon.settarget(buildhoriz(0));
                PlayerList[nPlayer].eyelevel -= dVertPan[nPlayer];
            }
            else
            {
                PlayerList[nPlayer].horizon.addadjustment(buildhoriz(dVertPan[nPlayer]));

                if (PlayerList[nPlayer].horizon.horiz.asq16() > gi->playerHorizMax())
                {
                    PlayerList[nPlayer].horizon.settarget(q16horiz(gi->playerHorizMax()));
                }
                else if (PlayerList[nPlayer].horizon.horiz.asq16() <= 0)
                {
                    if (!(pPlayerActor->sector()->Flag & kSectUnderwater))
                    {
                        SetNewWeapon(nPlayer, PlayerList[nPlayer].nDeathType + 8);
                    }
                }

                dVertPan[nPlayer]--;
            }
        }
    }

    // loc_1C4E1
    pDopple->spr.pos = pPlayerActor->spr.pos;

    if (pPlayerActor->sector()->pAbove != nullptr)
    {
        pDopple->spr.angle = pPlayerActor->spr.angle;
        ChangeActorSect(pDopple, pPlayerActor->sector()->pAbove);
        pDopple->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
    }
    else
    {
        pDopple->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    }

    MoveWeapons(nPlayer);
}


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
            ("horizon", w.horizon)
            ("angle", w.angle)
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
            ("xdamage", w.nDamage.X)
            ("ydamage", w.nDamage.Y)
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
            ("eyelevel", w.eyelevel)
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
        arc("lxvel", lPlayerXVel)
            ("lyvel", lPlayerYVel)
            ("bobangle", bobangle)
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
    ACTION_RETURN_POINTER(&PlayerList[nLocalPlayer].nPistolClip);
}

DEFINE_ACTION_FUNCTION(_Exhumed, GetPlayerClip)
{
    ACTION_RETURN_POINTER(&PlayerList[nLocalPlayer].nPlayerClip);
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
    ACTION_RETURN_INT(self->pActor->int_ang());
}


END_PS_NS
