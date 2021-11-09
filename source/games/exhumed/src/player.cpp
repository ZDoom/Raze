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
#include "compat.h"
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

extern short nStatusSeqOffset;

int lPlayerXVel = 0;
int lPlayerYVel = 0;
short obobangle = 0, bobangle  = 0;

static actionSeq PlayerSeq[] = {
    {18,  0}, {0,   0}, {9,   0}, {27,  0}, {63,  0},
    {72,  0}, {54,  0}, {45,  0}, {54,  0}, {81,  0},
    {90,  0}, {99,  0}, {108, 0}, {8,   0}, {0,   0},
    {139, 0}, {117, 1}, {119, 1}, {120, 1}, {121, 1},
    {122, 1}
};

static short nHeightTemplate[] = { 0, 0, 0, 0, 0, 0, 7, 7, 7, 9, 9, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0 };

short nActionEyeLevel[] = {
    -14080, -14080, -14080, -14080, -14080, -14080, -8320,
    -8320,  -8320,  -8320,  -8320,  -8320,  -8320,  -14080,
    -14080, -14080, -14080, -14080, -14080, -14080, -14080
};

uint16_t nGunLotag[] = { 52, 53, 54, 55, 56, 57 };
uint16_t nGunPicnum[] = { 57, 488, 490, 491, 878, 899, 3455 };

int16_t nItemText[] = {
    -1, -1, -1, -1, -1, -1, 18, 20, 19, 13, -1, 10, 1, 0, 2, -1, 3,
    -1, 4, 5, 9, 6, 7, 8, -1, 11, -1, 13, 12, 14, 15, -1, 16, 17,
    -1, -1, -1, 21, 22, -1, -1, -1, -1, -1, -1, 23, 24, 25, 26, 27,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};


int nLocalPlayer = 0;

Player PlayerList[kMaxPlayers];

DExhumedActor* nNetStartSprite[kMaxPlayers] = { };

short nStandHeight;



short PlayerCount;

short nNetStartSprites;
short nCurStartSprite;

void RestoreSavePoint(int nPlayer, int *x, int *y, int *z, short *nSector, short *nAngle)
{
    *x = PlayerList[nPlayer].sPlayerSave.x;
    *y = PlayerList[nPlayer].sPlayerSave.y;
    *z = PlayerList[nPlayer].sPlayerSave.z;
    *nSector = PlayerList[nPlayer].sPlayerSave.nSector;
    *nAngle  = PlayerList[nPlayer].sPlayerSave.nAngle;
}

void SetSavePoint(int nPlayer, int x, int y, int z, int nSector, short nAngle)
{
    PlayerList[nPlayer].sPlayerSave.x = x;
    PlayerList[nPlayer].sPlayerSave.y = y;
    PlayerList[nPlayer].sPlayerSave.z = z;
    PlayerList[nPlayer].sPlayerSave.nSector = nSector;
    PlayerList[nPlayer].sPlayerSave.nAngle = nAngle;
}

void feebtag(int x, int y, int z, int nSector, DExhumedActor **nSprite, int nVal2, int nVal3)
{
    *nSprite = nullptr;

    int startwall = sector[nSector].wallptr;

    int nWalls = sector[nSector].wallnum;

    int var_20 = nVal2 & 2;
    int var_14 = nVal2 & 1;

    while (1)
    {
        if (nSector != -1)
        {
            ExhumedSectIterator it(nSector);
            while (auto pActor = it.Next())
            {
                auto pSprite = &pActor->s();
                short nStat = pSprite->statnum;

                if (nStat >= 900 && !(pSprite->cstat & 0x8000))
                {
                    uint32_t xDiff = abs(pSprite->x - x);
                    uint32_t yDiff = abs(pSprite->y - y);
                    int zDiff = pSprite->z - z;

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

        nSector = wall[startwall].nextsector;
        startwall++;
    }
}

void InitPlayer()
{
    for (int i = 0; i < kMaxPlayers; i++) {
        PlayerList[i].pActor = nullptr;
    }
}

void InitPlayerKeys(short nPlayer)
{
    PlayerList[nPlayer].keys = 0;
}

void InitPlayerInventory(short nPlayer)
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

short GetPlayerFromActor(DExhumedActor* pActor)
{
	auto pSprite = &pActor->s();
    return RunData[pSprite->owner].nObjIndex;
}

void RestartPlayer(short nPlayer)
{
	auto plr = &PlayerList[nPlayer];
    auto pActor = plr->Actor();
	auto pDopSprite = plr->pDoppleSprite;

    DExhumedActor* floorsprt;

	if (pActor)
	{
        auto nSpr = &pActor->s();
        runlist_DoSubRunRec(nSpr->owner);
		runlist_FreeRun(nSpr->lotag - 1);

		ChangeActorStat(pActor, 0);

        plr->pActor = nullptr;

		auto pFloorSprite = plr->pPlayerFloorSprite;
		if (pFloorSprite != nullptr) {
			DeleteActor(pFloorSprite);
		}

		if (pDopSprite)
		{
            auto sp = &pDopSprite->s();
			runlist_DoSubRunRec(sp->owner);
			runlist_FreeRun(sp->lotag - 1);
            DeleteActor(pDopSprite);
		}
	}

    pActor = GrabBody();
	auto nSpr = &pActor->s();

	ChangeActorSect(pActor, plr->sPlayerSave.nSector);
	ChangeActorStat(pActor, 100);

	auto pDActor = insertActor(nSpr->sectnum, 100);
	plr->pDoppleSprite = pDActor;

	if (nTotalPlayers > 1)
	{
		auto nNStartSprite = nNetStartSprite[nCurStartSprite];
		auto nstspr = &nNStartSprite->s();
		nCurStartSprite++;

		if (nCurStartSprite >= nNetStartSprites) {
			nCurStartSprite = 0;
		}

		nSpr->x = nstspr->x;
		nSpr->y = nstspr->y;
		nSpr->z = nstspr->z;
		ChangeActorSect(pActor, nstspr->sectnum);
		plr->angle.ang = buildang(nstspr->ang&kAngleMask);
		nSpr->ang = plr->angle.ang.asbuild();

		floorsprt = insertActor(nSpr->sectnum, 0);
		auto fspr = &floorsprt->s();

		fspr->x = nSpr->x;
		fspr->y = nSpr->y;
		fspr->z = nSpr->z;
		fspr->yrepeat = 64;
		fspr->xrepeat = 64;
		fspr->cstat = 32;
		fspr->picnum = nPlayer + kTile3571;
	}
	else
	{
		nSpr->x = plr->sPlayerSave.x;
		nSpr->y = plr->sPlayerSave.y;
		nSpr->z = sector[plr->sPlayerSave.nSector].floorz;
		plr->angle.ang = buildang(plr->sPlayerSave.nAngle&kAngleMask);
		nSpr->ang = plr->angle.ang.asbuild();

		floorsprt = nullptr;
	}

	plr->angle.backup();
	plr->horizon.backup();

	plr->pPlayerFloorSprite = floorsprt;

	nSpr->cstat = 0x101;
	nSpr->shade = -12;
	nSpr->clipdist = 58;
	nSpr->pal = 0;
	nSpr->xrepeat = 40;
	nSpr->yrepeat = 40;
	nSpr->xoffset = 0;
	nSpr->yoffset = 0;
	nSpr->picnum = seq_GetSeqPicnum(kSeqJoe, 18, 0);

	int nHeight = GetActorHeight(pActor);
	nSpr->xvel = 0;
	nSpr->yvel = 0;
	nSpr->zvel = 0;

	nStandHeight = nHeight;

	nSpr->hitag = 0;
	nSpr->extra = -1;
	nSpr->lotag = runlist_HeadRun() + 1;

    auto nDSpr = &pDActor->s();
	nDSpr->x = nSpr->x;
	nDSpr->y = nSpr->y;
	nDSpr->z = nSpr->z;
	nDSpr->xrepeat = nSpr->xrepeat;
	nDSpr->yrepeat = nSpr->yrepeat;
	nDSpr->xoffset = 0;
	nDSpr->yoffset = 0;
	nDSpr->shade = nSpr->shade;
	nDSpr->ang = nSpr->ang;
	nDSpr->cstat = nSpr->cstat;

	nDSpr->lotag = runlist_HeadRun() + 1;

	plr->nAction = 0;
	plr->nHealth = 800; // TODO - define

	if (nNetPlayerCount) {
		plr->nHealth = 1600; // TODO - define
	}

	plr->field_2 = 0;
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
	plr->field_3FOUR = 0;
	plr->nPlayerViewSect = plr->sPlayerSave.nSector;
	plr->field_3A = 0;

	plr->nDouble = 0;

	plr->nSeq = kSeqJoe;

	plr->nPlayerPushSound = -1;

	plr->field_38 = -1;

	if (plr->nCurrentWeapon == 7) {
		plr->nCurrentWeapon = plr->field_3C;
	}

	plr->field_3C = 0;
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
	plr->oeyelevel = plr->eyelevel = -14080;
	dVertPan[nPlayer] = 0;

	nTemperature[nPlayer] = 0;

	plr->nYDamage = 0;
	plr->nXDamage = 0;

	plr->nDestVertPan = plr->horizon.ohoriz = plr->horizon.horiz = q16horiz(0);
	plr->nBreathTimer = 90;

	plr->nTauntTimer = RandomSize(3) + 3;

	nDSpr->owner = runlist_AddRunRec(nDSpr->lotag - 1, nPlayer, 0xA0000);
	nSpr->owner = runlist_AddRunRec(nSpr->lotag - 1, nPlayer, 0xA0000);

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

    auto pActor = PlayerList[nPlayer].Actor();
	auto pSprite = &pActor->s();
    PlayerList[nPlayer].nHealth = 0;

    short nLotag = pSprite->sector()->lotag;

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
                int nSector =pSprite->sectnum;
                if (SectBelow[nSector] > -1) {
                    nSector = SectBelow[nSector];
                }

                auto pGunActor = GrabBodyGunSprite();
                ChangeActorSect(pGunActor, nSector);
				auto pGunSprite = &pGunActor->s();

                pGunSprite->x = pSprite->x;
                pGunSprite->y = pSprite->y;
                pGunSprite->z = sector[nSector].floorz - 512;

                ChangeActorStat(pGunActor, nGunLotag[nWeapon] + 900);

                pGunSprite->picnum = nGunPicnum[nWeapon];

                BuildItemAnim(pGunActor);
            }
        }
    }

    StopFiringWeapon(nPlayer);

    PlayerList[nPlayer].horizon.ohoriz = PlayerList[nPlayer].horizon.horiz = q16horiz(0);
    PlayerList[nPlayer].oeyelevel = PlayerList[nPlayer].eyelevel = -14080;
    PlayerList[nPlayer].nInvisible = 0;
    dVertPan[nPlayer] = 15;

    pSprite->cstat &= 0x7FFF;

    SetNewWeaponImmediate(nPlayer, -2);

    if (SectDamage[pSprite->sectnum] <= 0)
    {
        PlayerList[nPlayer].nDeathType = nVal;
    }
    else
    {
        PlayerList[nPlayer].nDeathType = 2;
    }

    nVal *= 2;

    if (nVal || !(SectFlag[pSprite->sectnum] & kSectUnderwater))
    {
        PlayerList[nPlayer].nAction = nVal + 17;
    }
    else {
        PlayerList[nPlayer].nAction = 16;
    }

    PlayerList[nPlayer].field_2 = 0;

    pSprite->cstat &= 0xFEFE;

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

    short nCurAmmo = PlayerList[nPlayer].nAmmo[nWeapon];

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
    auto pActor = PlayerList[nPlayer].pActor;
	auto pSprite = &pActor->s();

    pSprite->yvel = 0;
    pSprite->xvel = 0;

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

    PlayerList[nPlayer].field_2 = 0;
}

void ShootStaff(int nPlayer)
{
    PlayerList[nPlayer].nAction = 15;
    PlayerList[nPlayer].field_2 = 0;
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
    inita = pPlayer->angle.ang.asbuild();
    if (pPlayer->Actor()) pPlayer->Actor()->s().ang = inita;
}

void AIPlayer::Draw(RunListEvent* ev)
{
    short nPlayer = RunData[ev->nRun].nObjIndex;
    assert(nPlayer >= 0 && nPlayer < kMaxPlayers);
    short nAction = PlayerList[nPlayer].nAction;

    seq_PlotSequence(ev->nParam, SeqOffsets[PlayerList[nPlayer].nSeq] + PlayerSeq[nAction].a, PlayerList[nPlayer].field_2, PlayerSeq[nAction].b);
}

void AIPlayer::RadialDamage(RunListEvent* ev)
{
    short nPlayer = RunData[ev->nRun].nObjIndex;
    assert(nPlayer >= 0 && nPlayer < kMaxPlayers);

    auto pPlayerActor = PlayerList[nPlayer].Actor();

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
    short nPlayer = RunData[ev->nRun].nObjIndex;
    auto pPlayerActor = PlayerList[nPlayer].Actor();
    short nAction = PlayerList[nPlayer].nAction;
    auto pPlayerSprite = &pPlayerActor->s();
    auto pDopple = PlayerList[nPlayer].pDoppleSprite;

    if (!nDamage) {
        return;
    }

    DExhumedActor* pActor2 = (!ev->isRadialEvent()) ? ev->pOtherActor : ev->pRadialActor->pTarget;

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

            if (SectFlag[pPlayerSprite->sectnum] & kSectUnderwater)
            {
                if (nAction != 12)
                {
                    PlayerList[nPlayer].field_2 = 0;
                    PlayerList[nPlayer].nAction = 12;
                    return;
                }
            }
            else
            {
                if (nAction != 4)
                {
                    PlayerList[nPlayer].field_2 = 0;
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
        if (pActor2 && pActor2->s().statnum == 100)
        {
            short nPlayer2 = GetPlayerFromActor(pActor2);

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

    short nPlayer = RunData[ev->nRun].nObjIndex;
    assert(nPlayer >= 0 && nPlayer < kMaxPlayers);

    auto pPlayerActor = PlayerList[nPlayer].Actor();
    auto pPlayerSprite = &pPlayerActor->s();

    auto pDopple = PlayerList[nPlayer].pDoppleSprite;

    short nAction = PlayerList[nPlayer].nAction;
    short nActionB = PlayerList[nPlayer].nAction;

    PlayerList[nPlayer].angle.backup();
    PlayerList[nPlayer].horizon.backup();
    PlayerList[nPlayer].angle.resetadjustment();
    PlayerList[nPlayer].horizon.resetadjustment();
    PlayerList[nPlayer].oeyelevel = PlayerList[nPlayer].eyelevel;

    pPlayerSprite->xvel = sPlayerInput[nPlayer].xVel >> 14;
    pPlayerSprite->yvel = sPlayerInput[nPlayer].yVel >> 14;

    if (sPlayerInput[nPlayer].nItem > -1)
    {
        UseItem(nPlayer, sPlayerInput[nPlayer].nItem);
        sPlayerInput[nPlayer].nItem = -1;
    }

    int var_EC = PlayerList[nPlayer].field_2;

    pPlayerSprite->picnum = seq_GetSeqPicnum(PlayerList[nPlayer].nSeq, PlayerSeq[nHeightTemplate[nAction]].a, var_EC);
    pDopple->s().picnum = pPlayerSprite->picnum;

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
                AddFlash(pPlayerSprite->sectnum,
                    pPlayerSprite->x,
                    pPlayerSprite->y,
                    pPlayerSprite->z, 0);
            }
        }
    }

    if (PlayerList[nPlayer].nDouble > 0)
    {
        PlayerList[nPlayer].nDouble--;
        if (PlayerList[nPlayer].nDouble == 150 && nPlayer == nLocalPlayer) {
            PlayAlert("WEAPON POWER IS ABOUT TO EXPIRE");
        }
    }

    if (PlayerList[nPlayer].nInvisible > 0)
    {
        PlayerList[nPlayer].nInvisible--;
        if (PlayerList[nPlayer].nInvisible == 0)
        {
            pPlayerSprite->cstat &= 0x7FFF; // set visible
            auto pFloorSprite = PlayerList[nPlayer].pPlayerFloorSprite;

            if (pFloorSprite != nullptr) {
                pFloorSprite->s().cstat &= 0x7FFF; // set visible
            }
        }
        else if (PlayerList[nPlayer].nInvisible == 150 && nPlayer == nLocalPlayer)
        {
            PlayAlert("INVISIBILITY IS ABOUT TO EXPIRE");
        }
    }

    if (PlayerList[nPlayer].invincibility > 0)
    {
        PlayerList[nPlayer].invincibility--;
        if (PlayerList[nPlayer].invincibility == 150 && nPlayer == nLocalPlayer) {
            PlayAlert("INVINCIBILITY IS ABOUT TO EXPIRE");
        }
    }

    if (nQuake[nPlayer] != 0)
    {
        nQuake[nPlayer] = -nQuake[nPlayer];
        if (nQuake[nPlayer] > 0)
        {
            nQuake[nPlayer] -= 512;
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

    // pPlayerSprite->zvel is modified within Gravity()
    short zVel = pPlayerSprite->zvel;

    Gravity(pPlayerActor);

    if (pPlayerSprite->zvel >= 6500 && zVel < 6500)
    {
        D3PlayFX(StaticSound[kSound17], pPlayerActor);
    }

    // loc_1A4E6
    int nSector =pPlayerSprite->sectnum;
    short nSectFlag = SectFlag[PlayerList[nPlayer].nPlayerViewSect];

    int playerX = pPlayerSprite->x;
    int playerY = pPlayerSprite->y;

    int x = (sPlayerInput[nPlayer].xVel * 4) >> 2;
    int y = (sPlayerInput[nPlayer].yVel * 4) >> 2;
    int z = (pPlayerSprite->zvel * 4) >> 2;

    if (pPlayerSprite->zvel > 8192)
        pPlayerSprite->zvel = 8192;

    if (PlayerList[nPlayer].bIsMummified)
    {
        x /= 2;
        y /= 2;
    }

    int spr_x = pPlayerSprite->x;
    int spr_y = pPlayerSprite->y;
    int spr_z = pPlayerSprite->z;
    int spr_sectnum = pPlayerSprite->sectnum;

    // TODO
    // nSectFlag & kSectUnderwater;

    zVel = pPlayerSprite->zvel;

    Collision nMove(0);
    if (bSlipMode)
    {
        pPlayerSprite->x += (x >> 14);
        pPlayerSprite->y += (y >> 14);

        vec3_t pos = { pPlayerSprite->x, pPlayerSprite->y, pPlayerSprite->z };
        setActorPos(pPlayerActor, &pos);

        pPlayerSprite->z = pPlayerSprite->sector()->floorz;
    }
    else
    {
        nMove = movesprite(pPlayerActor, x, y, z, 5120, -5120, CLIPMASK0);

        int var_54 = pPlayerSprite->sectnum;

        pushmove(&pPlayerSprite->pos, &var_54, pPlayerSprite->clipdist << 2, 5120, -5120, CLIPMASK0);
        if (var_54 != pPlayerSprite->sectnum) {
            ChangeActorSect(pPlayerActor, var_54);
        }
    }

    // loc_1A6E4
    if (inside(pPlayerSprite->x, pPlayerSprite->y, pPlayerSprite->sectnum) != 1)
    {
        ChangeActorSect(pPlayerActor, spr_sectnum);

        pPlayerSprite->x = spr_x;
        pPlayerSprite->y = spr_y;

        if (zVel < pPlayerSprite->zvel) {
            pPlayerSprite->zvel = zVel;
        }
    }

    //			int _bTouchFloor = bTouchFloor;
    short bUnderwater = SectFlag[pPlayerSprite->sectnum] & kSectUnderwater;

    if (bUnderwater)
    {
        PlayerList[nPlayer].nXDamage /= 2;
        PlayerList[nPlayer].nYDamage /= 2;
    }

    // Trigger Ramses?
    if ((SectFlag[pPlayerSprite->sectnum] & 0x8000) && bTouchFloor)
    {
        if (nTotalPlayers <= 1)
        {
            auto ang = GetAngleToSprite(pPlayerActor, pSpiritSprite) & kAngleMask;
            PlayerList[nPlayer].angle.settarget(ang, true);
            pPlayerSprite->ang = ang;

            PlayerList[nPlayer].horizon.settarget(0, true);

            lPlayerXVel = 0;
            lPlayerYVel = 0;

            pPlayerSprite->xvel = 0;
            pPlayerSprite->yvel = 0;
            pPlayerSprite->zvel = 0;

            if (nFreeze < 1)
            {
                nFreeze = 1;
                StopAllSounds();
                StopLocalSound();
                InitSpiritHead();

                PlayerList[nPlayer].nDestVertPan = q16horiz(0);
                PlayerList[nPlayer].horizon.settarget(currentLevel->ex_ramses_horiz);
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
            PlayerList[nPlayer].nXDamage /= 2;
            PlayerList[nPlayer].nYDamage /= 2;

            if (nPlayer == nLocalPlayer)
            {
                short zVelB = zVel;

                if (zVelB < 0) {
                    zVelB = -zVelB;
                }

                if (zVelB > 512 && !PlayerList[nPlayer].horizon.horiz.asq16() && cl_slopetilting) {
                    PlayerList[nPlayer].nDestVertPan = q16horiz(0);
                }
            }

            if (zVel >= 6500)
            {
                pPlayerSprite->xvel >>= 2;
                pPlayerSprite->yvel >>= 2;

                runlist_DamageEnemy(pPlayerActor, nullptr, ((zVel - 6500) >> 7) + 10);

                if (PlayerList[nPlayer].nHealth <= 0)
                {
                    pPlayerSprite->xvel = 0;
                    pPlayerSprite->yvel = 0;

                    StopActorSound(pPlayerActor);
                    PlayFXAtXYZ(StaticSound[kSoundJonFDie], pPlayerSprite->x, pPlayerSprite->y, pPlayerSprite->z, pPlayerSprite->sectnum, CHANF_NONE, 1); // CHECKME
                }
                else
                {
                    D3PlayFX(StaticSound[kSound27] | 0x2000, pPlayerActor);
                }
            }
        }

        if (nMove.type == kHitSector || nMove.type == kHitWall)
        {
            int sectnum = 0;
            int nNormal = 0;

            if (nMove.type == kHitSector)
            {
                sectnum = nMove.index;
                // Hm... Normal calculation here was broken.
            }
            else if (nMove.type == kHitWall)
            {
                sectnum = wall[nMove.index].nextsector;
                nNormal = GetWallNormal(nMove.index);
            }

            if (sectnum >= 0)
            {
                if ((sector[sectnum].hitag == 45) && bTouchFloor)
                {
                    int nDiff = AngleDiff(nNormal, (pPlayerSprite->ang + 1024) & kAngleMask);

                    if (nDiff < 0) {
                        nDiff = -nDiff;
                    }

                    if (nDiff <= 256)
                    {
                        PlayerList[nPlayer].nPlayerPushSect = sectnum;

                        int xvel = sPlayerInput[nPlayer].xVel;
                        int yvel = sPlayerInput[nPlayer].yVel;
                        int nMyAngle = GetMyAngle(xvel, yvel);

                        setsectinterpolate(sectnum);
                        MoveSector(sectnum, nMyAngle, &xvel, &yvel);

                        if (PlayerList[nPlayer].nPlayerPushSound <= -1)
                        {
                            PlayerList[nPlayer].nPlayerPushSound = 1;
                            short nBlock = sector[PlayerList[nPlayer].nPlayerPushSect].extra;
                            auto pBlockActor = sBlockInfo[nBlock].pActor;

                            D3PlayFX(StaticSound[kSound23], pBlockActor, 0x4000);
                        }
                        else
                        {
                            pPlayerSprite->x = spr_x;
                            pPlayerSprite->y = spr_y;
                            pPlayerSprite->z = spr_z;

                            ChangeActorSect(pPlayerActor, spr_sectnum);
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
        if (PlayerList[nPlayer].nPlayerPushSect > -1)
        {
            StopActorSound(sBlockInfo[sector[PlayerList[nPlayer].nPlayerPushSect].extra].pActor);
        }

        PlayerList[nPlayer].nPlayerPushSound = -1;
    }

sectdone:
    if (!PlayerList[nPlayer].bPlayerPan && !PlayerList[nPlayer].bLockPan)
    {
        PlayerList[nPlayer].nDestVertPan = q16horiz(clamp((spr_z - pPlayerSprite->z) << 9, gi->playerHorizMin(), gi->playerHorizMax()));
    }

    playerX -= pPlayerSprite->x;
    playerY -= pPlayerSprite->y;

    uint32_t sqrtNum = playerX * playerX + playerY * playerY;

    if (sqrtNum > INT_MAX)
    {
        DPrintf(DMSG_WARNING, "%s %d: overflow\n", __func__, __LINE__);
        sqrtNum = INT_MAX;
    }

    PlayerList[nPlayer].ototalvel = PlayerList[nPlayer].totalvel;
    PlayerList[nPlayer].totalvel = ksqrt(sqrtNum);

    int nViewSect = pPlayerSprite->sectnum;

    int EyeZ = PlayerList[nPlayer].eyelevel + pPlayerSprite->z + nQuake[nPlayer];

    while (1)
    {
        int nCeilZ = sector[nViewSect].ceilingz;

        if (EyeZ >= nCeilZ)
            break;

        if (SectAbove[nViewSect] <= -1)
            break;

        nViewSect = SectAbove[nViewSect];
    }

    // Do underwater sector check
    if (bUnderwater)
    {
        if (nViewSect != pPlayerSprite->sectnum)
        {
            if (nMove.type == kHitWall)
            {
                int var_C4 = pPlayerSprite->x;
                int var_D4 = pPlayerSprite->y;
                int var_C8 = pPlayerSprite->z;

                ChangeActorSect(pPlayerActor, nViewSect);

                pPlayerSprite->x = spr_x;
                pPlayerSprite->y = spr_y;

                int var_FC = sector[nViewSect].floorz + (-5120);

                pPlayerSprite->z = var_FC;

                auto coll = movesprite(pPlayerActor, x, y, 0, 5120, 0, CLIPMASK0);
                if (coll.type == kHitWall)
                {
                    ChangeActorSect(pPlayerActor, pPlayerSprite->sectnum);

                    pPlayerSprite->x = var_C4;
                    pPlayerSprite->y = var_D4;
                    pPlayerSprite->z = var_C8;
                }
                else
                {
                    pPlayerSprite->z = var_FC - 256;
                    D3PlayFX(StaticSound[kSound42], pPlayerActor);
                }
            }
        }
    }

    // loc_1ADAF
    PlayerList[nPlayer].nPlayerViewSect = nViewSect;

    PlayerList[nPlayer].nPlayerDX = pPlayerSprite->x - spr_x;
    PlayerList[nPlayer].nPlayerDY = pPlayerSprite->y - spr_y;

    int var_5C = SectFlag[nViewSect] & kSectUnderwater;

    uint16_t buttons = sPlayerInput[nPlayer].buttons;
    auto actions = sPlayerInput[nPlayer].actions;

    // loc_1AEF5:
    if (PlayerList[nPlayer].nHealth > 0)
    {
        if (PlayerList[nPlayer].nMaskAmount > 0)
        {
            PlayerList[nPlayer].nMaskAmount--;
            if (PlayerList[nPlayer].nMaskAmount == 150 && nPlayer == nLocalPlayer) {
                PlayAlert("MASK IS ABOUT TO EXPIRE");
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
            int nTmpSectNum = pPlayerSprite->sectnum;

            if (PlayerList[nPlayer].totalvel > 25 && pPlayerSprite->z > sector[nTmpSectNum].floorz)
            {
                if (SectDepth[nTmpSectNum] && !SectSpeed[nTmpSectNum] && !SectDamage[nTmpSectNum])
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
        auto pFloorActor = PlayerList[nPlayer].pPlayerFloorSprite;
        if (nTotalPlayers > 1 && pFloorActor)
        {
            auto pFloorSprite = &pFloorActor->s();

            pFloorSprite->x = pPlayerSprite->x;
            pFloorSprite->y = pPlayerSprite->y;

            if (pFloorSprite->sectnum != pPlayerSprite->sectnum)
            {
                ChangeActorSect(pFloorActor, pPlayerSprite->sectnum);
            }

            pFloorSprite->z = pPlayerSprite->sector()->floorz;
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
        short nearTagSector, nearTagWall, nearTagSprite;
        int nearHitDist;

        // neartag finds the nearest sector, wall, and sprite which has its hitag and/or lotag set to a value.
        neartag(pPlayerSprite->x, pPlayerSprite->y, pPlayerSprite->z, pPlayerSprite->sectnum, pPlayerSprite->ang,
            &nearTagSector, &nearTagWall, &nearTagSprite, (int32_t*)&nearHitDist, 1024, 2, nullptr);

        DExhumedActor* pActorB;
        feebtag(pPlayerSprite->x, pPlayerSprite->y, pPlayerSprite->z, pPlayerSprite->sectnum, &pActorB, var_30, 768);

        // Item pickup code
        if (pActorB != nullptr && pActorB->s().statnum >= 900)
        {
            auto pSpriteB = &pActorB->s();
            int var_8C = 16;
            int var_88 = 9;

            int var_70 = pSpriteB->statnum - 900;
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
                    if (AddAmmo(nPlayer, 1, pSpriteB->hitag))
                    {
                        var_88 = StaticSound[kSoundAmmoPickup];
                        goto do_default;
                    }

                    break;
                }
                case 1: // Fuel Canister
                {
                    if (AddAmmo(nPlayer, 3, pSpriteB->hitag))
                    {
                        var_88 = StaticSound[kSoundAmmoPickup];
                        goto do_default;
                    }
                    break;
                }
                case 2: // M - 60 Ammo Belt
                {
                    if (AddAmmo(nPlayer, 2, pSpriteB->hitag))
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
                            pSpriteB->cstat = 0x8000;
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
                    if (pSpriteB->hitag == 0) {
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
                            pSpriteB->hitag = 0;
                            pSpriteB->picnum++;

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
                            pSpriteB->hitag = 0;
                            pSpriteB->picnum++;

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
                            pSpriteB->hitag = 0;
                            pSpriteB->picnum++;

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

                    short weapons = PlayerList[nPlayer].nPlayerWeapons;

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

                    pSpriteB->cstat = 0x8000;
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

                    short weapons = PlayerList[nPlayer].nPlayerWeapons;

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

                    pSpriteB->cstat = 0x8000;
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

                    short weapons = PlayerList[nPlayer].nPlayerWeapons;

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

                    pSpriteB->cstat = 0x8000;
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

                    short weapons = PlayerList[nPlayer].nPlayerWeapons;

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

                    pSpriteB->cstat = 0x8000;
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

                    short weapons = PlayerList[nPlayer].nPlayerWeapons;

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

                    pSpriteB->cstat = 0x8000;
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

                    short weapons = PlayerList[nPlayer].nPlayerWeapons;

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

                    pSpriteB->cstat = 0x8000;
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
                    if (AddAmmo(nPlayer, 6, pSpriteB->hitag)) {
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

                    SetSavePoint(nPlayer, pPlayerSprite->x, pPlayerSprite->y, pPlayerSprite->z, pPlayerSprite->sectnum, pPlayerSprite->ang);
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
            if (pPlayerSprite->sector()->lotag > 0)
            {
                runlist_SignalRun(pPlayerSprite->sector()->lotag - 1, nPlayer, &ExhumedAI::TouchFloor);
            }
        }

        if (nSector != pPlayerSprite->sectnum)
        {
            if (sector[nSector].lotag > 0)
            {
                runlist_SignalRun(sector[nSector].lotag - 1, nPlayer, &ExhumedAI::EnterSector);
            }

            if (pPlayerSprite->sector()->lotag > 0)
            {
                runlist_SignalRun(pPlayerSprite->sector()->lotag - 1, nPlayer, &ExhumedAI::LeaveSector);
            }
        }

        if (!PlayerList[nPlayer].bIsMummified)
        {
            if (actions & SB_OPEN)
            {
                ClearSpaceBar(nPlayer);

                if (nearTagWall >= 0 && wall[nearTagWall].lotag > 0)
                {
                    runlist_SignalRun(wall[nearTagWall].lotag - 1, nPlayer, &ExhumedAI::Use);
                }

                if (nearTagSector >= 0 && sector[nearTagSector].lotag > 0)
                {
                    runlist_SignalRun(sector[nearTagSector].lotag - 1, nPlayer, &ExhumedAI::Use);
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

            // CHECKME - are we finished with 'nSector' variable at this point? if so, maybe set it to pPlayerSprite->sectnum so we can make this code a bit neater. Don't assume pPlayerSprite->sectnum == nSector here!!
            if (nStandHeight > (pPlayerSprite->sector()->floorz - pPlayerSprite->sector()->ceilingz)) {
                var_48 = 1;
            }

            // Jumping
            if (actions & SB_JUMP)
            {
                if (bUnderwater)
                {
                    pPlayerSprite->zvel = -2048;
                    nActionB = 10;
                }
                else if (bTouchFloor)
                {
                    if (nAction < 6 || nAction > 8)
                    {
                        pPlayerSprite->zvel = -3584;
                        nActionB = 3;
                    }
                }

                // goto loc_1BE70:
            }
            else if (actions & SB_CROUCH)
            {
                if (bUnderwater)
                {
                    pPlayerSprite->zvel = 2048;
                    nActionB = 10;
                }
                else
                {
                    if (PlayerList[nPlayer].eyelevel < -8320) {
                        PlayerList[nPlayer].eyelevel += ((-8320 - PlayerList[nPlayer].eyelevel) >> 1);
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
                    int var_EC = nActionEyeLevel[nAction];
                    PlayerList[nPlayer].eyelevel += (var_EC - PlayerList[nPlayer].eyelevel) >> 1;

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
            PlayerList[nPlayer].field_2 = 0;
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
                pPlayer->horizon.addadjustment(abs(nVertPan) >= 4 ? clamp(nVertPan, -4., 4.) : nVertPan * 2.);
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
                        pPlayerSprite->picnum = seq_GetSeqPicnum(kSeqJoe, 120, 0);
                        pPlayerSprite->cstat = 0;
                        pPlayerSprite->z = pPlayerSprite->sector()->floorz;
                    }

                    // will invalidate nPlayerSprite
                    RestartPlayer(nPlayer);

                    pPlayerActor = PlayerList[nPlayer].Actor();
                    pDopple = PlayerList[nPlayer].pDoppleSprite;
                }
                else
                {
                    DoGameOverScene(mplevel);
                    return;
                }
            }
        }
    }

    // loc_1C201:
    if (nLocalPlayer == nPlayer)
    {
        nLocalEyeSect = PlayerList[nLocalPlayer].nPlayerViewSect;
        CheckAmbience(nLocalEyeSect);
    }

    int var_AC = SeqOffsets[PlayerList[nPlayer].nSeq] + PlayerSeq[nAction].a;

    seq_MoveSequence(pPlayerActor, var_AC, PlayerList[nPlayer].field_2);
    PlayerList[nPlayer].field_2++;

    if (PlayerList[nPlayer].field_2 >= SeqSize[var_AC])
    {
        PlayerList[nPlayer].field_2 = 0;

        switch (PlayerList[nPlayer].nAction)
        {
        default:
            break;

        case 3:
            PlayerList[nPlayer].field_2 = SeqSize[var_AC] - 1;
            break;
        case 4:
            PlayerList[nPlayer].nAction = 0;
            break;
        case 16:
            PlayerList[nPlayer].field_2 = SeqSize[var_AC] - 1;

            if (pPlayerSprite->z < pPlayerSprite->sector()->floorz) {
                pPlayerSprite->z += 256;
            }

            if (!RandomSize(5))
            {
                vec3_t pos;
                int mouthSect;
                WheresMyMouth(nPlayer, &pos, &mouthSect);

                BuildAnim(nullptr, 71, 0, pos.x, pos.y, pPlayerSprite->z + 3840, mouthSect, 75, 128);
            }
            break;
        case 17:
            PlayerList[nPlayer].nAction = 18;
            break;
        case 19:
            pPlayerSprite->cstat |= 0x8000;
            PlayerList[nPlayer].nAction = 20;
            break;
        }
    }

    // loc_1C3B4:
    if (nPlayer == nLocalPlayer)
    {
        initx = pPlayerSprite->x;
        inity = pPlayerSprite->y;
        initz = pPlayerSprite->z;
        initsect = pPlayerSprite->sectnum;
        inita = pPlayerSprite->ang;
    }

    if (!PlayerList[nPlayer].nHealth)
    {
        PlayerList[nPlayer].nYDamage = 0;
        PlayerList[nPlayer].nXDamage = 0;

        if (PlayerList[nPlayer].eyelevel >= -2816)
        {
            PlayerList[nPlayer].eyelevel = -2816;
            dVertPan[nPlayer] = 0;
        }
        else
        {
            if (PlayerList[nPlayer].horizon.horiz.asq16() < 0)
            {
                PlayerList[nPlayer].horizon.settarget(0);
                PlayerList[nPlayer].eyelevel -= (dVertPan[nPlayer] << 8);
            }
            else
            {
                PlayerList[nPlayer].horizon.addadjustment(dVertPan[nPlayer]);

                if (PlayerList[nPlayer].horizon.horiz.asq16() > gi->playerHorizMax())
                {
                    PlayerList[nPlayer].horizon.settarget(gi->playerHorizMax());
                }
                else if (PlayerList[nPlayer].horizon.horiz.asq16() <= 0)
                {
                    if (!(SectFlag[pPlayerSprite->sectnum] & kSectUnderwater))
                    {
                        SetNewWeapon(nPlayer, PlayerList[nPlayer].nDeathType + 8);
                    }
                }

                dVertPan[nPlayer]--;
            }
        }
    }

    // loc_1C4E1
    pDopple->s().pos = pPlayerSprite->pos;

    if (SectAbove[pPlayerSprite->sectnum] > -1)
    {
        pDopple->s().ang = pPlayerSprite->ang;
        ChangeActorSect(pDopple, SectAbove[pPlayerSprite->sectnum]);
        pDopple->s().cstat = 0x101;
    }
    else
    {
        pDopple->s().cstat = 0x8000;
    }

    MoveWeapons(nPlayer);
}


FSerializer& Serialize(FSerializer& arc, const char* keyname, Player& w, Player* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("health", w.nHealth)
            ("at2", w.field_2)
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
            ("field3f", w.field_3FOUR)
            ("field38", w.field_38)
            ("field3a", w.field_3A)
            ("field3c", w.field_3C)
            ("seq", w.nSeq)
            ("horizon", w.horizon)
            ("angle", w.angle)
            ("lives", w.nLives)
            ("double", w.nDouble)
            ("invisible", w.nInvisible)
            ("torch", w.nTorch)
            ("breathtimer", w.nBreathTimer)
            ("playerswear", w.nPlayerSwear)
            ("pushsect", w.nPlayerPushSect)
            ("deathtype", w.nDeathType)
            ("score", w.nPlayerScore)
            ("color", w.nPlayerColor)
            ("dx", w.nPlayerDX)
            ("dy", w.nPlayerDY)
            ("pistolclip", w.nPistolClip)
            ("xdamage", w.nXDamage)
            ("ydamage", w.nYDamage)
            ("dopplesprite", w.pDoppleSprite)
            ("oldweapon", w.nPlayerOldWeapon)
            ("clip", w.nPlayerClip)
            ("pushsound", w.nPlayerPushSound)
            ("taunttimer", w.nTauntTimer)
            ("weapons", w.nPlayerWeapons)
            ("viewsect", w.nPlayerViewSect)
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
        arc("x", w.x)
            ("y", w.y)
            ("z", w.z)
            ("sector", w.nSector)
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
DEFINE_FIELD_X(ExhumedPlayer, Player, field_2);
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
DEFINE_FIELD_X(ExhumedPlayer, Player, field_3FOUR);
DEFINE_FIELD_X(ExhumedPlayer, Player, bIsFiring);
DEFINE_FIELD_X(ExhumedPlayer, Player, field_38);
DEFINE_FIELD_X(ExhumedPlayer, Player, field_3A);
DEFINE_FIELD_X(ExhumedPlayer, Player, field_3C);
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
    ACTION_RETURN_BOOL(SectFlag[PlayerList[nLocalPlayer].nPlayerViewSect] & kSectUnderwater);
}

DEFINE_ACTION_FUNCTION(_ExhumedPlayer, GetAngle)
{
    PARAM_SELF_STRUCT_PROLOGUE(Player);
    ACTION_RETURN_INT(self->Actor()->s().ang);
}


END_PS_NS
