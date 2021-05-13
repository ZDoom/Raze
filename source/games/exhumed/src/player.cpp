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

struct PlayerSave
{
    int x;
    int y;
    int z;
    short nSector;
    short nAngle;
};

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

short nBreathTimer[kMaxPlayers];
short nPlayerSwear[kMaxPlayers];
short nPlayerPushSect[kMaxPlayers];
short nDeathType[kMaxPlayers];
short nPlayerScore[kMaxPlayers];
short nPlayerColor[kMaxPlayers];
int nPlayerDY[kMaxPlayers];
int nPlayerDX[kMaxPlayers];
short nPistolClip[kMaxPlayers];
int nXDamage[kMaxPlayers];
int nYDamage[kMaxPlayers];
short nDoppleSprite[kMaxPlayers];
short nPlayerOldWeapon[kMaxPlayers];
short nPlayerClip[kMaxPlayers];
short nPlayerPushSound[kMaxPlayers];
short nTauntTimer[kMaxPlayers];
short nPlayerTorch[kMaxPlayers];
uint16_t nPlayerWeapons[kMaxPlayers]; // each set bit represents a weapon the player has
short nPlayerLives[kMaxPlayers];
short nPlayerItem[kMaxPlayers];
Player PlayerList[kMaxPlayers];
short nPlayerInvisible[kMaxPlayers];
short nPlayerDouble[kMaxPlayers];
short nPlayerViewSect[kMaxPlayers];
short nPlayerFloorSprite[kMaxPlayers];
PlayerSave sPlayerSave[kMaxPlayers];
int ototalvel[kMaxPlayers] = { 0 };
int totalvel[kMaxPlayers] = { 0 };
int16_t eyelevel[kMaxPlayers], oeyelevel[kMaxPlayers];
short nNetStartSprite[kMaxPlayers] = { 0 };

short nStandHeight;

short nPlayerGrenade[kMaxPlayers];
short nGrenadePlayer[50];

short word_D282A[32];


short PlayerCount;

short nNetStartSprites;
short nCurStartSprite;

void RestoreSavePoint(int nPlayer, int *x, int *y, int *z, short *nSector, short *nAngle)
{
    *x = sPlayerSave[nPlayer].x;
    *y = sPlayerSave[nPlayer].y;
    *z = sPlayerSave[nPlayer].z;
    *nSector = sPlayerSave[nPlayer].nSector;
    *nAngle  = sPlayerSave[nPlayer].nAngle;
}

void SetSavePoint(int nPlayer, int x, int y, int z, short nSector, short nAngle)
{
    sPlayerSave[nPlayer].x = x;
    sPlayerSave[nPlayer].y = y;
    sPlayerSave[nPlayer].z = z;
    sPlayerSave[nPlayer].nSector = nSector;
    sPlayerSave[nPlayer].nAngle = nAngle;
}

void feebtag(int x, int y, int z, int nSector, short *nSprite, int nVal2, int nVal3)
{
    *nSprite = -1;

    int startwall = sector[nSector].wallptr;

    int nWalls = sector[nSector].wallnum;

    int var_20 = nVal2 & 2;
    int var_14 = nVal2 & 1;

    while (1)
    {
        if (nSector != -1)
        {
            int i;
            SectIterator it(nSector);
            while ((i = it.NextIndex()) >= 0)
            {
                short nStat = sprite[i].statnum;

                if (nStat >= 900 && !(sprite[i].cstat & 0x8000))
                {
                    uint32_t xDiff = abs(sprite[i].x - x);
                    uint32_t yDiff = abs(sprite[i].y - y);
                    int zDiff = sprite[i].z - z;

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
                            *nSprite = i;
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
        PlayerList[i].nSprite = -1;
    }
}

void InitPlayerKeys(short nPlayer)
{
    PlayerList[nPlayer].keys = 0;
}

void InitPlayerInventory(short nPlayer)
{
    memset(&PlayerList[nPlayer], 0, sizeof(Player));

    nPlayerItem[nPlayer] = -1;
    nPlayerSwear[nPlayer] = 4;

    ResetPlayerWeapons(nPlayer);

    nPlayerLives[nPlayer] = kDefaultLives;

    PlayerList[nPlayer].nSprite = -1;
    PlayerList[nPlayer].nRun = -1;

    nPistolClip[nPlayer] = 6;
    nPlayerClip[nPlayer] = 0;

    PlayerList[nPlayer].nCurrentWeapon = 0;

    if (nPlayer == nLocalPlayer) {
        automapMode = am_off;
    }

    nPlayerScore[nPlayer] = 0;

    auto pixels = tilePtr(kTile3571 + nPlayer);

    nPlayerColor[nPlayer] = pixels[tileWidth(nPlayer + kTile3571) * tileHeight(nPlayer + kTile3571) / 2];
}

short GetPlayerFromSprite(short nSprite)
{
    return RunData[sprite[nSprite].owner].nVal;
}

void RestartPlayer(short nPlayer)
{
	auto plr = &PlayerList[nPlayer];
	int nSprite = plr->nSprite;
	auto nSpr = &sprite[nSprite];
	int nDopSprite = nDoppleSprite[nPlayer];

	int floorspr;

	if (nSprite > -1)
	{
		runlist_DoSubRunRec(nSpr->owner);
		runlist_FreeRun(nSpr->lotag - 1);

		changespritestat(nSprite, 0);

		plr->nSprite = -1;

		int nFloorSprite = nPlayerFloorSprite[nPlayer];
		if (nFloorSprite > -1) {
			mydeletesprite(nFloorSprite);
		}

		if (nDopSprite > -1)
		{
			runlist_DoSubRunRec(sprite[nDopSprite].owner);
			runlist_FreeRun(sprite[nDopSprite].lotag - 1);
			mydeletesprite(nDopSprite);
		}
	}

	nSprite = GrabBody();
	nSpr = &sprite[nSprite];

	mychangespritesect(nSprite, sPlayerSave[nPlayer].nSector);
	changespritestat(nSprite, 100);

	assert(nSprite >= 0 && nSprite < kMaxSprites);

	int nDSprite = insertsprite(nSpr->sectnum, 100);
	nDoppleSprite[nPlayer] = nDSprite;

	assert(nDSprite >= 0 && nDSprite < kMaxSprites);

	if (nTotalPlayers > 1)
	{
		int nNStartSprite = nNetStartSprite[nCurStartSprite];
		auto nstspr = &sprite[nNStartSprite];
		nCurStartSprite++;

		if (nCurStartSprite >= nNetStartSprites) {
			nCurStartSprite = 0;
		}

		nSpr->x = nstspr->x;
		nSpr->y = nstspr->y;
		nSpr->z = nstspr->z;
		mychangespritesect(nSprite, nstspr->sectnum);
		plr->angle.ang = buildang(nstspr->ang&kAngleMask);
		nSpr->ang = plr->angle.ang.asbuild();

		floorspr = insertsprite(nSpr->sectnum, 0);
		assert(floorspr >= 0 && floorspr < kMaxSprites);
		auto fspr = &sprite[floorspr];

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
		nSpr->x = sPlayerSave[nPlayer].x;
		nSpr->y = sPlayerSave[nPlayer].y;
		nSpr->z = sector[sPlayerSave[nPlayer].nSector].floorz;
		plr->angle.ang = buildang(sPlayerSave[nPlayer].nAngle&kAngleMask);
		nSpr->ang = plr->angle.ang.asbuild();

		floorspr = -1;
	}

	plr->angle.backup();
	plr->horizon.backup();

	nPlayerFloorSprite[nPlayer] = floorspr;

	nSpr->cstat = 0x101;
	nSpr->shade = -12;
	nSpr->clipdist = 58;
	nSpr->pal = 0;
	nSpr->xrepeat = 40;
	nSpr->yrepeat = 40;
	nSpr->xoffset = 0;
	nSpr->yoffset = 0;
	nSpr->picnum = seq_GetSeqPicnum(kSeqJoe, 18, 0);

	int nHeight = GetSpriteHeight(nSprite);
	nSpr->xvel = 0;
	nSpr->yvel = 0;
	nSpr->zvel = 0;

	nStandHeight = nHeight;

	nSpr->hitag = 0;
	nSpr->extra = -1;
	nSpr->lotag = runlist_HeadRun() + 1;

	auto nDSpr = &sprite[nDSprite];
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
	plr->nSprite = nSprite;
	plr->bIsMummified = false;

	if (plr->invincibility >= 0) {
		plr->invincibility = 0;
	}

	nPlayerTorch[nPlayer] = 0;
	plr->nMaskAmount = 0;

	SetTorch(nPlayer, 0);

	nPlayerInvisible[nPlayer] = 0;

	plr->bIsFiring = 0;
	plr->field_3FOUR = 0;
	nPlayerViewSect[nPlayer] = sPlayerSave[nPlayer].nSector;
	plr->field_3A = 0;

	nPlayerDouble[nPlayer] = 0;

	plr->nSeq = kSeqJoe;

	nPlayerPushSound[nPlayer] = -1;

	plr->field_38 = -1;

	if (plr->nCurrentWeapon == 7) {
		plr->nCurrentWeapon = plr->field_3C;
	}

	plr->field_3C = 0;
	plr->nAir = 100;
	airpages = 0;

	if (!(currentLevel->gameflags & LEVEL_EX_MULTI))
	{
		RestoreMinAmmo(nPlayer);
	}
	else
	{
		ResetPlayerWeapons(nPlayer);
		plr->nMagic = 0;
	}

	nPlayerGrenade[nPlayer] = -1;
	oeyelevel[nPlayer] = eyelevel[nPlayer] = -14080;
	dVertPan[nPlayer] = 0;

	nTemperature[nPlayer] = 0;

	nYDamage[nPlayer] = 0;
	nXDamage[nPlayer] = 0;

	plr->nDestVertPan = plr->horizon.ohoriz = plr->horizon.horiz = q16horiz(0);
	nBreathTimer[nPlayer] = 90;

	nTauntTimer[nPlayer] = RandomSize(3) + 3;

	nDSpr->owner = runlist_AddRunRec(nDSpr->lotag - 1, nPlayer | 0xA0000);
	nSpr->owner = runlist_AddRunRec(nSpr->lotag - 1, nPlayer | 0xA0000);

	if (plr->nRun < 0) {
		plr->nRun = runlist_AddRunRec(NewRun, nPlayer | 0xA0000);
	}

	BuildRa(nPlayer);

	if (nPlayer == nLocalPlayer)
	{
		nLocalSpr = nSprite;

		SetMagicFrame();
		RestoreGreenPal();

        plr->bPlayerPan = plr->bLockPan = false;
	}

	ototalvel[nPlayer] = totalvel[nPlayer] = 0;

	memset(&sPlayerInput[nPlayer], 0, sizeof(PlayerInput));
	sPlayerInput[nPlayer].nItem = -1;

	nDeathType[nPlayer] = 0;
	nQuake[nPlayer] = 0;

	if (nPlayer == nLocalPlayer) {
		SetHealthFrame(0);
	}
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

    short nSprite = PlayerList[nPlayer].nSprite;
    PlayerList[nPlayer].nHealth = 0;

    short nLotag = sector[sprite[nSprite].sectnum].lotag;

    if (nLotag > 0) {
        runlist_SignalRun(nLotag - 1, nPlayer | 0x70000);
    }

    if (nPlayerGrenade[nPlayer] >= 0)
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
                short nSector = sprite[nSprite].sectnum;
                if (SectBelow[nSector] > -1) {
                    nSector = SectBelow[nSector];
                }

                int nGunSprite = GrabBodyGunSprite();
                changespritesect(nGunSprite, nSector);

                sprite[nGunSprite].x = sprite[nSprite].x;
                sprite[nGunSprite].y = sprite[nSprite].y;
                sprite[nGunSprite].z = sector[nSector].floorz - 512;

                changespritestat(nGunSprite, nGunLotag[nWeapon] + 900);

                sprite[nGunSprite].picnum = nGunPicnum[nWeapon];

                BuildItemAnim(nGunSprite);
            }
        }
    }

    StopFiringWeapon(nPlayer);

    PlayerList[nPlayer].horizon.ohoriz = PlayerList[nPlayer].horizon.horiz = q16horiz(0);
    oeyelevel[nPlayer] = eyelevel[nPlayer] = -14080;
    nPlayerInvisible[nPlayer] = 0;
    dVertPan[nPlayer] = 15;

    sprite[nSprite].cstat &= 0x7FFF;

    SetNewWeaponImmediate(nPlayer, -2);

    if (SectDamage[sprite[nSprite].sectnum] <= 0)
    {
        nDeathType[nPlayer] = nVal;
    }
    else
    {
        nDeathType[nPlayer] = 2;
    }

    nVal *= 2;

    if (nVal || !(SectFlag[sprite[nSprite].sectnum] & kSectUnderwater))
    {
        PlayerList[nPlayer].nAction = nVal + 17;
    }
    else {
        PlayerList[nPlayer].nAction = 16;
    }

    PlayerList[nPlayer].field_2 = 0;

    sprite[nSprite].cstat &= 0xFEFE;

    if (nTotalPlayers == 1)
    {
        short nLives = nPlayerLives[nPlayer];

        if (nLives > 0) {
            BuildStatusAnim((3 * (nLives - 1)) + 7, 0);
        }

        if (!(currentLevel->gameflags & LEVEL_EX_TRAINING)) { // if not on the training level
            nPlayerLives[nPlayer]--;
        }

        if (nPlayerLives[nPlayer] < 0) {
            nPlayerLives[nPlayer] = 0;
        }
    }

    ototalvel[nPlayer] = totalvel[nPlayer] = 0;

    if (nPlayer == nLocalPlayer) {
        RefreshStatus();
    }
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

    if (nPlayer == nLocalPlayer)
    {
        if (nWeapon == nCounterBullet) {
            SetCounter(nAmmoAmount);
        }
    }

    if (nWeapon == 1)
    {
        if (!nPistolClip[nPlayer]) {
            nPistolClip[nPlayer] = 6;
        }
    }

    return 1;
}

void SetPlayerMummified(int nPlayer, int bIsMummified)
{
    int nSprite = PlayerList[nPlayer].nSprite;

    sprite[nSprite].yvel = 0;
    sprite[nSprite].xvel = 0;

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
    inita = sprite[pPlayer->nSprite].ang = pPlayer->angle.ang.asbuild();
}

void FuncPlayer(int a, int nDamage, int nRun)
{
    int var_48 = 0;
    int var_40;
	bool mplevel = (currentLevel->gameflags & LEVEL_EX_MULTI);

    short nPlayer = RunData[nRun].nVal;
    assert(nPlayer >= 0 && nPlayer < kMaxPlayers);

    short nPlayerSprite = PlayerList[nPlayer].nSprite;

    short nDopple = nDoppleSprite[nPlayer];

    short nAction = PlayerList[nPlayer].nAction;
    short nActionB = PlayerList[nPlayer].nAction;

    int nMessage = a & kMessageMask;

    short nSprite2;

    PlayerList[nPlayer].angle.backup();
    PlayerList[nPlayer].horizon.backup();
    PlayerList[nPlayer].angle.resetadjustment();
    PlayerList[nPlayer].horizon.resetadjustment();
    oeyelevel[nPlayer] = eyelevel[nPlayer];

    switch (nMessage)
    {
        case 0x90000:
        {
            seq_PlotSequence(a & 0xFFFF, SeqOffsets[PlayerList[nPlayer].nSeq] + PlayerSeq[nAction].a, PlayerList[nPlayer].field_2, PlayerSeq[nAction].b);
            return;
        }

        case 0xA0000:
        {
            if (PlayerList[nPlayer].nHealth <= 0) {
                return;
            }

            nDamage = runlist_CheckRadialDamage(nPlayerSprite);
            if (!nDamage) {
                return;
            }

            nSprite2 = nRadialOwner;
            // fall through to case 0x80000
            fallthrough__;
        }

        case 0x80000:
        {
            // Dunno how to do this otherwise... we fall through from above but don't want to do this check..
            if (nMessage != 0xA0000)
            {
                if (!nDamage) {
                    return;
                }

                nSprite2 = a & 0xFFFF;
            }

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
                    SetHealthFrame(-1);
                }
            }

            if (PlayerList[nPlayer].nHealth > 0)
            {
                if (nDamage > 40 || (totalmoves & 0xF) < 2)
                {
                    if (PlayerList[nPlayer].invincibility) {
                        return;
                    }

                    if (SectFlag[sprite[nPlayerSprite].sectnum] & kSectUnderwater)
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

                            if (nSprite2 > -1)
                            {
                                nPlayerSwear[nPlayer]--;
                                if (nPlayerSwear[nPlayer] <= 0)
                                {
                                    D3PlayFX(StaticSound[kSound52], nDopple);
                                    nPlayerSwear[nPlayer] = RandomSize(3) + 4;
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
                if (nSprite2 > -1 && sprite[nSprite2].statnum == 100)
                {
                    short nPlayer2 = GetPlayerFromSprite(nSprite2);

                    if (nPlayer2 == nPlayer) // player caused their own death
                    {
                        nPlayerScore[nPlayer]--;
                    }
                    else
                    {
                        nPlayerScore[nPlayer]++;
                    }
                }
                else if (nSprite2 < 0)
                {
                    nPlayerScore[nPlayer]--;
                }

                if (nMessage == 0xA0000)
                {
                    for (int i = 122; i <= 131; i++)
                    {
                        BuildCreatureChunk(nPlayerSprite, seq_GetSeqPicnum(kSeqJoe, i, 0));
                    }

                    StartDeathSeq(nPlayer, 1);
                }
                else
                {
                    StartDeathSeq(nPlayer, 0);
                }
            }

            return;
        }

        case 0x20000:
        {
            sprite[nPlayerSprite].xvel = sPlayerInput[nPlayer].xVel >> 14;
            sprite[nPlayerSprite].yvel = sPlayerInput[nPlayer].yVel >> 14;

            if (sPlayerInput[nPlayer].nItem > -1)
            {
                UseItem(nPlayer, sPlayerInput[nPlayer].nItem);
                sPlayerInput[nPlayer].nItem = -1;
            }

            int var_EC = PlayerList[nPlayer].field_2;

            sprite[nPlayerSprite].picnum = seq_GetSeqPicnum(PlayerList[nPlayer].nSeq, PlayerSeq[nHeightTemplate[nAction]].a, var_EC);
            sprite[nDopple].picnum = sprite[nPlayerSprite].picnum;

            if (nPlayerTorch[nPlayer] > 0)
            {
                nPlayerTorch[nPlayer]--;
                if (nPlayerTorch[nPlayer] == 0)
                {
                    SetTorch(nPlayer, 0);
                }
                else
                {
                    if (nPlayer != nLocalPlayer)
                    {
                        nFlashDepth = 5;
                        AddFlash(sprite[nPlayerSprite].sectnum,
                            sprite[nPlayerSprite].x,
                            sprite[nPlayerSprite].y,
                            sprite[nPlayerSprite].z, 0);
                    }
                }
            }

            if (nPlayerDouble[nPlayer] > 0)
            {
                nPlayerDouble[nPlayer]--;
                if (nPlayerDouble[nPlayer] == 150 && nPlayer == nLocalPlayer) {
                    PlayAlert("WEAPON POWER IS ABOUT TO EXPIRE");
                }
            }

            if (nPlayerInvisible[nPlayer] > 0)
            {
                nPlayerInvisible[nPlayer]--;
                if (nPlayerInvisible[nPlayer] == 0)
                {
                    sprite[nPlayerSprite].cstat &= 0x7FFF; // set visible
                    short nFloorSprite = nPlayerFloorSprite[nPlayerSprite];

                    if (nFloorSprite > -1) {
                        sprite[nFloorSprite].cstat &= 0x7FFF; // set visible
                    }
                }
                else if (nPlayerInvisible[nPlayer] == 150 && nPlayer == nLocalPlayer)
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

            // sprite[nPlayerSprite].zvel is modified within Gravity()
            short zVel = sprite[nPlayerSprite].zvel;

            Gravity(nPlayerSprite);

            if (sprite[nPlayerSprite].zvel >= 6500 && zVel < 6500)
            {
                D3PlayFX(StaticSound[kSound17], nPlayerSprite);
            }

            // loc_1A4E6
            short nSector = sprite[nPlayerSprite].sectnum;
            short nSectFlag = SectFlag[nPlayerViewSect[nPlayer]];

            int playerX = sprite[nPlayerSprite].x;
            int playerY = sprite[nPlayerSprite].y;

            int x = (sPlayerInput[nPlayer].xVel * 4) >> 2;
            int y = (sPlayerInput[nPlayer].yVel * 4) >> 2;
            int z = (sprite[nPlayerSprite].zvel * 4) >> 2;

            if (sprite[nPlayerSprite].zvel > 8192)
                sprite[nPlayerSprite].zvel = 8192;

            if (PlayerList[nPlayer].bIsMummified)
            {
                x /= 2;
                y /= 2;
            }

            int spr_x = sprite[nPlayerSprite].x;
            int spr_y = sprite[nPlayerSprite].y;
            int spr_z = sprite[nPlayerSprite].z;
            int spr_sectnum = sprite[nPlayerSprite].sectnum;

            // TODO
            // nSectFlag & kSectUnderwater;

            zVel = sprite[nPlayerSprite].zvel;

            int nMove = 0; // TEMP

            if (bSlipMode)
            {
                nMove = 0;

                sprite[nPlayerSprite].x += (x >> 14);
                sprite[nPlayerSprite].y += (y >> 14);

                vec3_t pos = { sprite[nPlayerSprite].x, sprite[nPlayerSprite].y, sprite[nPlayerSprite].z };
                setsprite(nPlayerSprite, &pos);

                sprite[nPlayerSprite].z = sector[sprite[nPlayerSprite].sectnum].floorz;
            }
            else
            {
                nMove = movesprite(nPlayerSprite, x, y, z, 5120, -5120, CLIPMASK0);

                short var_54 = sprite[nPlayerSprite].sectnum;

                pushmove_old(&sprite[nPlayerSprite].x, &sprite[nPlayerSprite].y, &sprite[nPlayerSprite].z, &var_54, sprite[nPlayerSprite].clipdist << 2, 5120, -5120, CLIPMASK0);
                if (var_54 != sprite[nPlayerSprite].sectnum) {
                    mychangespritesect(nPlayerSprite, var_54);
                }
            }

            // loc_1A6E4
            if (inside(sprite[nPlayerSprite].x, sprite[nPlayerSprite].y, sprite[nPlayerSprite].sectnum) != 1)
            {
                mychangespritesect(nPlayerSprite, spr_sectnum);

                sprite[nPlayerSprite].x = spr_x;
                sprite[nPlayerSprite].y = spr_y;

                if (zVel < sprite[nPlayerSprite].zvel) {
                    sprite[nPlayerSprite].zvel = zVel;
                }
            }

//			int _bTouchFloor = bTouchFloor;
            short bUnderwater = SectFlag[sprite[nPlayerSprite].sectnum] & kSectUnderwater;

            if (bUnderwater)
            {
                nXDamage[nPlayer] /= 2;
                nYDamage[nPlayer] /= 2;
            }

            // Trigger Ramses?
            if ((SectFlag[sprite[nPlayerSprite].sectnum] & 0x8000) && bTouchFloor)
            {
                if (nTotalPlayers <= 1)
                {
                    auto ang = GetAngleToSprite(nPlayerSprite, nSpiritSprite) & kAngleMask;
                    PlayerList[nPlayer].angle.settarget(ang, true);
                    sprite[nPlayerSprite].ang = ang;

                    PlayerList[nPlayer].horizon.settarget(0, true);

                    lPlayerXVel = 0;
                    lPlayerYVel = 0;

                    sprite[nPlayerSprite].xvel = 0;
                    sprite[nPlayerSprite].yvel = 0;
                    sprite[nPlayerSprite].zvel = 0;

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

            if (nMove & 0x3C000)
            {
                if (bTouchFloor)
                {
                    // Damage stuff..
                    nXDamage[nPlayer] /= 2;
                    nYDamage[nPlayer] /= 2;

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
                        sprite[nPlayerSprite].xvel >>= 2;
                        sprite[nPlayerSprite].yvel >>= 2;

                        runlist_DamageEnemy(nPlayerSprite, -1, ((zVel - 6500) >> 7) + 10);

                        if (PlayerList[nPlayer].nHealth <= 0)
                        {
                            sprite[nPlayerSprite].xvel = 0;
                            sprite[nPlayerSprite].yvel = 0;

                            StopSpriteSound(nPlayerSprite);
    					    PlayFXAtXYZ(StaticSound[kSoundJonFDie], sprite[nPlayerSprite].x, sprite[nPlayerSprite].y, sprite[nPlayerSprite].z, sprite[nPlayerSprite].sectnum |= 0x4000); // CHECKME
                        }
                        else
                        {
                            D3PlayFX(StaticSound[kSound27] | 0x2000, nPlayerSprite);
                        }
                    }
                }

                if (((nMove & 0xC000) == 0x4000) || ((nMove & 0xC000) == 0x8000))
                {
                    int sectnum = 0;

                    if ((nMove & 0xC000) == 0x4000)
                    {
                        sectnum = nMove & 0x3FFF;
                    }
                    else if ((nMove & 0xC000) == 0x8000)
                    {
                        sectnum = wall[nMove & 0x3FFF].nextsector;
                    }

                    if (sectnum >= 0)
                    {
                        if ((sector[sectnum].hitag == 45) && bTouchFloor)
                        {
                            int nNormal = GetWallNormal(nMove & 0x3FFF);
                            int nDiff = AngleDiff(nNormal, (sprite[nPlayerSprite].ang + 1024) & kAngleMask);

                            if (nDiff < 0) {
                                nDiff = -nDiff;
                            }

                            if (nDiff <= 256)
                            {
                                nPlayerPushSect[nPlayer] = sectnum;

                                int xvel = sPlayerInput[nPlayer].xVel;
                                int yvel = sPlayerInput[nPlayer].yVel;
                                int nMyAngle = GetMyAngle(xvel, yvel);

                                setsectinterpolate(sectnum);
                                MoveSector(sectnum, nMyAngle, &xvel, &yvel);

                                if (nPlayerPushSound[nPlayer] <= -1)
                                {
                                    nPlayerPushSound[nPlayer] = 1;
                                    short nBlock = sector[nPlayerPushSect[nPlayer]].extra;
                                    int nBlockSprite = sBlockInfo[nBlock].nSprite;

                                    D3PlayFX(StaticSound[kSound23], nBlockSprite | 0x4000);
                                }
                                else
                                {
                                    sprite[nPlayerSprite].x = spr_x;
                                    sprite[nPlayerSprite].y = spr_y;
                                    sprite[nPlayerSprite].z = spr_z;

                                    mychangespritesect(nPlayerSprite, spr_sectnum);
                                }

                                movesprite(nPlayerSprite, xvel, yvel, z, 5120, -5120, CLIPMASK0);
                                goto sectdone;
                            }
                        }
                    }
                }
            }

            // loc_1AB46:
            if (nPlayerPushSound[nPlayer] > -1)
            {
                if (nPlayerPushSect[nPlayer] > -1)
                {
                    StopSpriteSound(sBlockInfo[sector[nPlayerPushSect[nPlayer]].extra].nSprite);
                }

                nPlayerPushSound[nPlayer] = -1;
            }

sectdone:
            if (!PlayerList[nPlayer].bPlayerPan && !PlayerList[nPlayer].bLockPan)
            {
                PlayerList[nPlayer].nDestVertPan = q16horiz(clamp((spr_z - sprite[nPlayerSprite].z) << 9, gi->playerHorizMin(), gi->playerHorizMax()));
            }

            playerX -= sprite[nPlayerSprite].x;
            playerY -= sprite[nPlayerSprite].y;

            uint32_t sqrtNum = playerX * playerX + playerY * playerY;

            if (sqrtNum > INT_MAX)
            {
                DPrintf(DMSG_WARNING, "%s %d: overflow\n", __func__, __LINE__);
                sqrtNum = INT_MAX;
            }

            ototalvel[nPlayer] = totalvel[nPlayer];
            totalvel[nPlayer] = ksqrt(sqrtNum);

            int nViewSect = sprite[nPlayerSprite].sectnum;

            int EyeZ = eyelevel[nPlayer] + sprite[nPlayerSprite].z + nQuake[nPlayer];

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
                if (nViewSect != sprite[nPlayerSprite].sectnum)
                {
                    if ((nMove & 0xC000) == 0x8000)
                    {
                        int var_C4 = sprite[nPlayerSprite].x;
                        int var_D4 = sprite[nPlayerSprite].y;
                        int var_C8 = sprite[nPlayerSprite].z;

                        mychangespritesect(nPlayerSprite, nViewSect);

                        sprite[nPlayerSprite].x = spr_x;
                        sprite[nPlayerSprite].y = spr_y;

                        int var_FC = sector[nViewSect].floorz + (-5120);

                        sprite[nPlayerSprite].z = var_FC;

                        if ((movesprite(nPlayerSprite, x, y, 0, 5120, 0, CLIPMASK0) & 0xC000) == 0x8000)
                        {
                            mychangespritesect(nPlayerSprite, sprite[nPlayerSprite].sectnum);

                            sprite[nPlayerSprite].x = var_C4;
                            sprite[nPlayerSprite].y = var_D4;
                            sprite[nPlayerSprite].z = var_C8;
                        }
                        else
                        {
                            sprite[nPlayerSprite].z = var_FC - 256;
                            D3PlayFX(StaticSound[kSound42], nPlayerSprite);
                        }
                    }
                }
            }

            // loc_1ADAF
            nPlayerViewSect[nPlayer] = nViewSect;

            nPlayerDX[nPlayer] = sprite[nPlayerSprite].x - spr_x;
            nPlayerDY[nPlayer] = sprite[nPlayerSprite].y - spr_y;

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
                    nBreathTimer[nPlayer]--;

                    if (nBreathTimer[nPlayer] <= 0)
                    {
                        nBreathTimer[nPlayer] = 90;

                        // if underwater
                        if (var_5C)
                        {
                            airpages = 1;
                            if (PlayerList[nPlayer].nMaskAmount > 0)
                            {
                                if (nPlayer == nLocalPlayer) {
                                    BuildStatusAnim(132, 0);
                                }

                                D3PlayFX(StaticSound[kSound30], nPlayerSprite);

                                PlayerList[nPlayer].nAir = 100;
                            }
                            else
                            {
                                PlayerList[nPlayer].nAir -= 25;
                                if (PlayerList[nPlayer].nAir > 0)
                                {
                                    D3PlayFX(StaticSound[kSound25], nPlayerSprite);
                                }
                                else
                                {
                                    PlayerList[nPlayer].nHealth += (PlayerList[nPlayer].nAir << 2);
                                    if (PlayerList[nPlayer].nHealth <= 0)
                                    {
                                        PlayerList[nPlayer].nHealth = 0;
                                        StartDeathSeq(nPlayer, 0);
                                    }

                                    if (nPlayer == nLocalPlayer)
                                    {
                                        SetHealthFrame(-1);
                                    }

                                    PlayerList[nPlayer].nAir = 0;

                                    if (PlayerList[nPlayer].nHealth < 300)
                                    {
                                        D3PlayFX(StaticSound[kSound79], nPlayerSprite);
                                    }
                                    else
                                    {
                                        D3PlayFX(StaticSound[kSound19], nPlayerSprite);
                                    }
                                }
                            }

                            DoBubbles(nPlayer);
                            SetAirFrame();
                        }
                        else
                        {
                            if (nPlayer == nLocalPlayer)
                            {
                                BuildStatusAnim(132, 0);
                            }

                            airpages = 0;
                        }
                    }
                }

                // loc_1B0B9
                if (var_5C) // if underwater
                {
                    if (nPlayerTorch[nPlayer] > 0)
                    {
                        nPlayerTorch[nPlayer] = 0;
                        SetTorch(nPlayer, 0);
                    }
                }
                else
                {
                    int nTmpSectNum = sprite[nPlayerSprite].sectnum;

                    if (totalvel[nPlayer] > 25 && sprite[nPlayerSprite].z > sector[nTmpSectNum].floorz)
                    {
                        if (SectDepth[nTmpSectNum] && !SectSpeed[nTmpSectNum] && !SectDamage[nTmpSectNum])
                        {
                            D3PlayFX(StaticSound[kSound42], nPlayerSprite);
                        }
                    }

                    // CHECKME - wrong place?
                    if (nSectFlag & kSectUnderwater)
                    {
                        if (PlayerList[nPlayer].nAir < 50)
                        {
                            D3PlayFX(StaticSound[kSound14], nPlayerSprite);
                        }

                        nBreathTimer[nPlayer] = 1;
                    }

                    airpages = 0;

                    nBreathTimer[nPlayer]--;
                    if (nBreathTimer[nPlayer] <= 0)
                    {
                        nBreathTimer[nPlayer] = 90;
                        if (nPlayer == nLocalPlayer)
                        {
                            // animate lungs
                            BuildStatusAnim(132, 0);
                        }
                    }

                    if (PlayerList[nPlayer].nAir < 100)
                    {
                        PlayerList[nPlayer].nAir = 100;
                        SetAirFrame();
                    }
                }

                // loc_1B1EB
                if (nTotalPlayers > 1)
                {
                    int nFloorSprite = nPlayerFloorSprite[nPlayer];

                    sprite[nFloorSprite].x = sprite[nPlayerSprite].x;
                    sprite[nFloorSprite].y = sprite[nPlayerSprite].y;

                    if (sprite[nFloorSprite].sectnum != sprite[nPlayerSprite].sectnum)
                    {
                        mychangespritesect(nFloorSprite, sprite[nPlayerSprite].sectnum);
                    }

                    sprite[nFloorSprite].z = sector[sprite[nPlayerSprite].sectnum].floorz;
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

                short nValB;

                // neartag finds the nearest sector, wall, and sprite which has its hitag and/or lotag set to a value.
                neartag(sprite[nPlayerSprite].x, sprite[nPlayerSprite].y, sprite[nPlayerSprite].z, sprite[nPlayerSprite].sectnum, sprite[nPlayerSprite].ang,
                    &nearTagSector, &nearTagWall, &nearTagSprite, (int32_t*)&nearHitDist, 1024, 2, NULL);

                feebtag(sprite[nPlayerSprite].x, sprite[nPlayerSprite].y, sprite[nPlayerSprite].z, sprite[nPlayerSprite].sectnum,
                    &nValB, var_30, 768);

                // Item pickup code
                if (nValB >= 0 && sprite[nValB].statnum >= 900)
                {
                    int var_8C = 16;
                    int var_88 = 9;

                    int var_70 = sprite[nValB].statnum - 900;
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
                                    DestroyItemAnim(nValB);
                                    mydeletesprite(nValB);
                                }
                                else
                                {
                                    StartRegenerate(nValB);
                                }
do_default_b:
                                // loc_1BA74
                                if (nPlayer == nLocalPlayer)
                                {
                                    if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                                    {
                                        pickupMessage(var_70);
                                    }

                                    TintPalette(var_44*4, var_8C*4, 0);

                                    if (var_88 > -1)
                                    {
                                        PlayLocalSound(var_88, 0);
                                    }
                                }

                                break;
                            }
                            case 0: // Speed Loader
                            {
                                if (AddAmmo(nPlayer, 1, sprite[nValB].hitag))
                                {
                                    var_88 = StaticSound[kSoundAmmoPickup];
                                    goto do_default;
                                }

                                break;
                            }
                            case 1: // Fuel Canister
                            {
                                if (AddAmmo(nPlayer, 3, sprite[nValB].hitag))
                                {
                                    var_88 = StaticSound[kSoundAmmoPickup];
                                    goto do_default;
                                }
                                break;
                            }
                            case 2: // M - 60 Ammo Belt
                            {
                                if (AddAmmo(nPlayer, 2, sprite[nValB].hitag))
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
                                    if (!(nPlayerWeapons[nPlayer] & 0x10))
                                    {
                                        nPlayerWeapons[nPlayer] |= 0x10;
                                        SetNewWeaponIfBetter(nPlayer, 4);
                                    }

                                    if (var_70 == 55)
                                    {
                                        sprite[nValB].cstat = 0x8000;
                                        DestroyItemAnim(nValB);

                                        // loc_1BA74: - repeated block, see in default case
                                        if (nPlayer == nLocalPlayer)
                                        {
                                            if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                                            {
                                                pickupMessage(var_70);
                                            }

                                            TintPalette(var_44*4, var_8C*4, 0);

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
                                if (sprite[nValB].hitag == 0) {
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

                                    if (nLocalPlayer == nPlayer)
                                    {
                                        SetHealthFrame(1);
                                    }

                                    if (var_70 == 12)
                                    {
                                        sprite[nValB].hitag = 0;
                                        sprite[nValB].picnum++;

                                        changespritestat(nValB, 0);

                                        // loc_1BA74: - repeated block, see in default case
                                        if (nPlayer == nLocalPlayer)
                                        {
                                            if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                                            {
                                                pickupMessage(var_70);
                                            }

                                            TintPalette(var_44*4, var_8C*4, 0);

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

                                    if (nLocalPlayer == nPlayer)
                                    {
                                        SetHealthFrame(1);
                                    }

                                    if (var_70 == 12)
                                    {
                                        sprite[nValB].hitag = 0;
                                        sprite[nValB].picnum++;

                                        changespritestat(nValB, 0);

                                        // loc_1BA74: - repeated block, see in default case
                                        if (nPlayer == nLocalPlayer)
                                        {
                                            if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                                            {
                                                pickupMessage(var_70);
                                            }

                                            TintPalette(var_44*4, var_8C*4, 0);

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

                                    if (nLocalPlayer == nPlayer)
                                    {
                                        SetHealthFrame(1);
                                    }

                                    if (var_70 == 12)
                                    {
                                        sprite[nValB].hitag = 0;
                                        sprite[nValB].picnum++;

                                        changespritestat(nValB, 0);

                                        // loc_1BA74: - repeated block, see in default case
                                        if (nPlayer == nLocalPlayer)
                                        {
                                            if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                                            {
                                                pickupMessage(var_70);
                                            }

                                            TintPalette(var_44*4, var_8C*4, 0);

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

                                SetAirFrame();

                                if (nBreathTimer[nPlayer] < 89)
                                {
                                    D3PlayFX(StaticSound[kSound13], nPlayerSprite);
                                }

                                nBreathTimer[nPlayer] = 90;
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

                                if (nPlayerLives[nPlayer] >= kMaxPlayerLives) {
                                    break;
                                }

                                nPlayerLives[nPlayer]++;

                                if (nPlayer == nLocalPlayer) {
                                    BuildStatusAnim(146 + ((nPlayerLives[nPlayer] - 1) * 2), 0);
                                }

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

                                short weapons = nPlayerWeapons[nPlayer];

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

                                    nPlayerWeapons[nPlayer] |= var_18;

                                    AddAmmo(nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                                    var_88 = StaticSound[kSound72];
                                }

                                if (var_40 == 2) {
                                    CheckClip(nPlayer);
                                }

                                if (var_70 <= 50) {
                                    goto do_default;
                                }

                                sprite[nValB].cstat = 0x8000;
                                DestroyItemAnim(nValB);
////
                                // loc_1BA74: - repeated block, see in default case
                                if (nPlayer == nLocalPlayer)
                                {
                                    if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                                    {
                                        pickupMessage(var_70);
                                    }

                                    TintPalette(var_44*4, var_8C*4, 0);

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

                                short weapons = nPlayerWeapons[nPlayer];

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

                                    nPlayerWeapons[nPlayer] |= var_18;

                                    AddAmmo(nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                                    var_88 = StaticSound[kSound72];
                                }

                                if (var_40 == 2) {
                                    CheckClip(nPlayer);
                                }

                                if (var_70 <= 50) {
                                    goto do_default;
                                }

                                sprite[nValB].cstat = 0x8000;
                                DestroyItemAnim(nValB);
////
                                // loc_1BA74: - repeated block, see in default case
                                if (nPlayer == nLocalPlayer)
                                {
                                    if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                                    {
                                        pickupMessage(var_70);
                                    }

                                    TintPalette(var_44*4, var_8C*4, 0);

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

                                short weapons = nPlayerWeapons[nPlayer];

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

                                    nPlayerWeapons[nPlayer] |= var_18;

                                    AddAmmo(nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                                    var_88 = StaticSound[kSound72];
                                }

                                if (var_40 == 2) {
                                    CheckClip(nPlayer);
                                }

                                if (var_70 <= 50) {
                                    goto do_default;
                                }

                                sprite[nValB].cstat = 0x8000;
                                DestroyItemAnim(nValB);
////
                                // loc_1BA74: - repeated block, see in default case
                                if (nPlayer == nLocalPlayer)
                                {
                                    if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                                    {
                                        pickupMessage(var_70);
                                    }

                                    TintPalette(var_44*4, var_8C*4, 0);

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

                                short weapons = nPlayerWeapons[nPlayer];

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

                                    nPlayerWeapons[nPlayer] |= var_18;

                                    AddAmmo(nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                                    var_88 = StaticSound[kSound72];
                                }

                                if (var_40 == 2) {
                                    CheckClip(nPlayer);
                                }

                                if (var_70 <= 50) {
                                    goto do_default;
                                }

                                sprite[nValB].cstat = 0x8000;
                                DestroyItemAnim(nValB);
////
                                // loc_1BA74: - repeated block, see in default case
                                if (nPlayer == nLocalPlayer)
                                {
                                    if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                                    {
                                        pickupMessage(var_70);
                                    }

                                    TintPalette(var_44*4, var_8C*4, 0);

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

                                short weapons = nPlayerWeapons[nPlayer];

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

                                    nPlayerWeapons[nPlayer] |= var_18;

                                    AddAmmo(nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                                    var_88 = StaticSound[kSound72];
                                }

                                if (var_40 == 2) {
                                    CheckClip(nPlayer);
                                }

                                if (var_70 <= 50) {
                                    goto do_default;
                                }

                                sprite[nValB].cstat = 0x8000;
                                DestroyItemAnim(nValB);
////
                                // loc_1BA74: - repeated block, see in default case
                                if (nPlayer == nLocalPlayer)
                                {
                                    if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                                    {
                                        pickupMessage(var_70);
                                    }

                                    TintPalette(var_44*4, var_8C*4, 0);

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

                                short weapons = nPlayerWeapons[nPlayer];

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

                                    nPlayerWeapons[nPlayer] |= var_18;

                                    AddAmmo(nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                                    var_88 = StaticSound[kSound72];
                                }

                                if (var_40 == 2) {
                                    CheckClip(nPlayer);
                                }

                                if (var_70 <= 50) {
                                    goto do_default;
                                }

                                sprite[nValB].cstat = 0x8000;
                                DestroyItemAnim(nValB);
////
                                // loc_1BA74: - repeated block, see in default case
                                if (nPlayer == nLocalPlayer)
                                {
                                    if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                                    {
                                        pickupMessage(var_70);
                                    }

                                    TintPalette(var_44*4, var_8C*4, 0);

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
                                if (AddAmmo(nPlayer, 6, sprite[nValB].hitag)) {
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

                                if (nLocalPlayer == nPlayer)
                                {
                                    SetMagicFrame();
                                }

                                goto do_default;
                            }

                            case 53: // Scarab (Checkpoint)
                            {
                                if (nLocalPlayer == nPlayer)
                                {
                                    short nAnim = sprite[nValB].owner;
                                    AnimList[nAnim].nSeq++;
                                    AnimList[nAnim].AnimFlags &= 0xEF;
                                    AnimList[nAnim].field_2 = 0;

                                    changespritestat(nValB, 899);
                                }

                                SetSavePoint(nPlayer, sprite[nPlayerSprite].x, sprite[nPlayerSprite].y, sprite[nPlayerSprite].z, sprite[nPlayerSprite].sectnum, sprite[nPlayerSprite].ang);
                                break;
                            }

                            case 54: // Golden Sarcophagus (End Level)
                            {
                                if (!bInDemo) 
                                {
                                    LevelFinished();
                                }

                                DestroyItemAnim(nValB);
                                mydeletesprite(nValB);
                                break;
                            }
                        }
                    }
                }

                // CORRECT ? // loc_1BAF9:
                if (bTouchFloor)
                {
                    if (sector[sprite[nPlayerSprite].sectnum].lotag > 0)
                    {
                        runlist_SignalRun(sector[sprite[nPlayerSprite].sectnum].lotag - 1, nPlayer | 0x50000);
                    }
                }

                if (nSector != sprite[nPlayerSprite].sectnum)
                {
                    if (sector[nSector].lotag > 0)
                    {
                        runlist_SignalRun(sector[nSector].lotag - 1, nPlayer | 0x70000);
                    }

                    if (sector[sprite[nPlayerSprite].sectnum].lotag > 0)
                    {
                        runlist_SignalRun(sector[sprite[nPlayerSprite].sectnum].lotag - 1, nPlayer | 0x60000);
                    }
                }

                if (!PlayerList[nPlayer].bIsMummified)
                {
                    if (actions & SB_OPEN)
                    {
                        ClearSpaceBar(nPlayer);

                        if (nearTagWall >= 0 && wall[nearTagWall].lotag > 0)
                        {
                            runlist_SignalRun(wall[nearTagWall].lotag - 1, nPlayer | 0x40000);
                        }

                        if (nearTagSector >= 0 && sector[nearTagSector].lotag > 0)
                        {
                            runlist_SignalRun(sector[nearTagSector].lotag - 1, nPlayer | 0x40000);
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

                    // CHECKME - are we finished with 'nSector' variable at this point? if so, maybe set it to sprite[nPlayerSprite].sectnum so we can make this code a bit neater. Don't assume sprite[nPlayerSprite].sectnum == nSector here!!
                    if (nStandHeight > (sector[sprite[nPlayerSprite].sectnum].floorz - sector[sprite[nPlayerSprite].sectnum].ceilingz)) {
                        var_48 = 1;
                    }

                    // Jumping
                    if (actions & SB_JUMP)
                    {
                        if (bUnderwater)
                        {
                            sprite[nPlayerSprite].zvel = -2048;
                            nActionB = 10;
                        }
                        else if (bTouchFloor)
                        {
                            if (nAction < 6 || nAction > 8)
                            {
                                sprite[nPlayerSprite].zvel = -3584;
                                nActionB = 3;
                            }
                        }

                        // goto loc_1BE70:
                    }
                    else if (actions & SB_CROUCH)
                    {
                        if (bUnderwater)
                        {
                            sprite[nPlayerSprite].zvel = 2048;
                            nActionB = 10;
                        }
                        else
                        {
                            if (eyelevel[nPlayer] < -8320) {
                                eyelevel[nPlayer] += ((-8320 - eyelevel[nPlayer]) >> 1);
                            }

loc_1BD2E:
                            if (totalvel[nPlayer] < 1) {
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
                            eyelevel[nPlayer] += (var_EC - eyelevel[nPlayer]) >> 1;

                            if (bUnderwater)
                            {
                                if (totalvel[nPlayer] <= 1)
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
                                    if (totalvel[nPlayer] <= 1) {
                                        nActionB = 0;//bUnderwater; // this is just setting to 0
                                    }
                                    else if (totalvel[nPlayer] <= 30) {
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

                        if (nPlayerWeapons[nPlayer] & (1 << var_90))
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
                        if (totalvel[nPlayer] <= 1)
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

                if (actions & (SB_LOOK_UP | SB_LOOK_DOWN) || sPlayerInput[nPlayer].pan)
                {
                    pPlayer->nDestVertPan = pPlayer->horizon.horiz;
                    pPlayer->bPlayerPan = pPlayer->bLockPan = true;
                }
                else if (actions & (SB_AIM_UP | SB_AIM_DOWN | SB_CENTERVIEW))
                {
                    pPlayer->nDestVertPan = pPlayer->horizon.horiz;
                    pPlayer->bPlayerPan = pPlayer->bLockPan = false;
                }

                if (totalvel[nPlayer] > 20)
                {
                    pPlayer->bPlayerPan = false;
                }

                if (cl_slopetilting)
                {
                    double nVertPan = (pPlayer->nDestVertPan - pPlayer->horizon.horiz).asbuildf() * 0.25;
                    if (nVertPan != 0)
                    {
                        pPlayer->horizon.addadjustment(abs(nVertPan) >= 4 ? clamp(nVertPan, -4, 4) : nVertPan * 2.);
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

                        PlayerList[nPlayer].nCurrentWeapon = nPlayerOldWeapon[nPlayer];

                        if (nPlayerLives[nPlayer] && nNetTime)
                        {
                            if (nAction != 20)
                            {
                                sprite[nPlayerSprite].picnum = seq_GetSeqPicnum(kSeqJoe, 120, 0);
                                sprite[nPlayerSprite].cstat = 0;
                                sprite[nPlayerSprite].z = sector[sprite[nPlayerSprite].sectnum].floorz;
                            }

                            // will invalidate nPlayerSprite
                            RestartPlayer(nPlayer);

                            nPlayerSprite = PlayerList[nPlayer].nSprite;
                            nDopple = nDoppleSprite[nPlayer];
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
                nLocalEyeSect = nPlayerViewSect[nLocalPlayer];
                CheckAmbience(nLocalEyeSect);
            }

            int var_AC = SeqOffsets[PlayerList[nPlayer].nSeq] + PlayerSeq[nAction].a;

            seq_MoveSequence(nPlayerSprite, var_AC, PlayerList[nPlayer].field_2);
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

                        if (sprite[nPlayerSprite].z < sector[sprite[nPlayerSprite].sectnum].floorz) {
                            sprite[nPlayerSprite].z += 256;
                        }

                        if (!RandomSize(5))
                        {
                            int mouthX, mouthY, mouthZ;
                            short mouthSect;
                            WheresMyMouth(nPlayer, &mouthX, &mouthY, &mouthZ, &mouthSect);

                            BuildAnim(-1, 71, 0, mouthX, mouthY, sprite[nPlayerSprite].z + 3840, mouthSect, 75, 128);
                        }
                        break;
                    case 17:
                        PlayerList[nPlayer].nAction = 18;
                        break;
                    case 19:
                        sprite[nPlayerSprite].cstat |= 0x8000;
                        PlayerList[nPlayer].nAction = 20;
                        break;
                }
            }

            // loc_1C3B4:
            if (nPlayer == nLocalPlayer)
            {
                initx = sprite[nPlayerSprite].x;
                inity = sprite[nPlayerSprite].y;
                initz = sprite[nPlayerSprite].z;
                initsect = sprite[nPlayerSprite].sectnum;
                inita = sprite[nPlayerSprite].ang;
            }

            if (!PlayerList[nPlayer].nHealth)
            {
                nYDamage[nPlayer] = 0;
                nXDamage[nPlayer] = 0;

                if (eyelevel[nPlayer] >= -2816)
                {
                    eyelevel[nPlayer] = -2816;
                    dVertPan[nPlayer] = 0;
                }
                else
                {
                    if (PlayerList[nPlayer].horizon.horiz.asq16() < 0)
                    {
                        PlayerList[nPlayer].horizon.settarget(0);
                        eyelevel[nPlayer] -= (dVertPan[nPlayer] << 8);
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
                            if (!(SectFlag[sprite[nPlayerSprite].sectnum] & kSectUnderwater))
                            {
                                SetNewWeapon(nPlayer, nDeathType[nPlayer] + 8);
                            }
                        }

                        dVertPan[nPlayer]--;
                    }
                }
            }

            // loc_1C4E1
            sprite[nDopple].x = sprite[nPlayerSprite].x;
            sprite[nDopple].y = sprite[nPlayerSprite].y;
            sprite[nDopple].z = sprite[nPlayerSprite].z;

            if (SectAbove[sprite[nPlayerSprite].sectnum] > -1)
            {
                sprite[nDopple].ang = sprite[nPlayerSprite].ang;
                mychangespritesect(nDopple, SectAbove[sprite[nPlayerSprite].sectnum]);
                sprite[nDopple].cstat = 0x101;
            }
            else
            {
                sprite[nDopple].cstat = 0x8000;
            }

            MoveWeapons(nPlayer);

            return;
        }
    }
}


FSerializer& Serialize(FSerializer& arc, const char* keyname, Player& w, Player* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("health", w.nHealth)
            ("at2", w.field_2)
            ("action", w.nAction)
            ("sprite", w.nSprite)
            ("mummy", w.bIsMummified)
            ("invincible", w.invincibility)
            ("air", w.nAir)
            ("seq", w.nSeq)
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
            .Array("grenadeplayer", nGrenadePlayer, countof(nGrenadePlayer))
            .Array("curstartsprite", nNetStartSprite, PlayerCount)
            .Array("breathtimer", nBreathTimer, PlayerCount)
            .Array("playerswear", nPlayerSwear, PlayerCount)
            .Array("pushsect", nPlayerPushSect, PlayerCount)
            .Array("deathtype", nDeathType, PlayerCount)
            .Array("score", nPlayerScore, PlayerCount)
            .Array("color", nPlayerColor, PlayerCount)
            .Array("dx", nPlayerDX, PlayerCount)
            .Array("dy", nPlayerDY, PlayerCount)
            .Array("pistolclip", nPistolClip, PlayerCount)
            .Array("xdamage", nXDamage, PlayerCount)
            .Array("ydamage", nYDamage, PlayerCount)
            .Array("dopplesprite", nDoppleSprite, PlayerCount)
            .Array("oldweapon", nPlayerOldWeapon, PlayerCount)
            .Array("clip", nPlayerClip, PlayerCount)
            .Array("pushsound", nPlayerPushSound, PlayerCount)
            .Array("taunttimer", nTauntTimer, PlayerCount)
            .Array("torch", nPlayerTorch, PlayerCount)
            .Array("weapons", nPlayerWeapons, PlayerCount)
            .Array("lives", nPlayerLives, PlayerCount)
            .Array("item", nPlayerItem, PlayerCount)
            .Array("list", PlayerList, PlayerCount)
            .Array("invisible", nPlayerInvisible, PlayerCount)
            .Array("double", nPlayerDouble, PlayerCount)
            .Array("viewsect", nPlayerViewSect, PlayerCount)
            .Array("floorspr", nPlayerFloorSprite, PlayerCount)
            .Array("save", sPlayerSave, PlayerCount)
            .Array("totalvel", totalvel, PlayerCount)
            .Array("eyelevel", eyelevel, PlayerCount)
            .Array("netstartsprite", nNetStartSprite, PlayerCount)
            .Array("grenade", nPlayerGrenade, PlayerCount)
            .Array("d282a", word_D282A, PlayerCount)
            .EndObject();
    }
}

END_PS_NS
