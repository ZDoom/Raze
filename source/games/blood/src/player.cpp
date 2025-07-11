//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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
#include "ns.h"	// Must come before everything else!

#include <stdlib.h>
#include <string.h>
#include "automap.h"
#include "build.h"

#include "blood.h"
#include "gstrings.h"
#include "gamestate.h"
#include "automap.h"

BEGIN_BLD_NS

bool gBlueFlagDropped = false;
bool gRedFlagDropped = false;

// V = has effect in game, X = no effect in game
POWERUPINFO gPowerUpInfo[kMaxPowerUps] = {
	{ -1, 1, 1, 1 },            // 00: V keys
	{ -1, 1, 1, 1 },            // 01: V keys
	{ -1, 1, 1, 1 },            // 02: V keys
	{ -1, 1, 1, 1 },            // 03: V keys
	{ -1, 1, 1, 1 },            // 04: V keys
	{ -1, 1, 1, 1 },            // 05: V keys
	{ -1, 1, 1, 1 },            // 06: V keys
	{ -1, 0, 100, 100 },        // 07: V doctor's bag
	{ -1, 0, 50, 100 },         // 08: V medicine pouch
	{ -1, 0, 20, 100 },         // 09: V life essense
	{ -1, 0, 100, 200 },        // 10: V life seed
	{ -1, 0, 2, 200 },          // 11: V red potion
	{ 783, 0, 3600, 432000 },   // 12: V feather fall
	{ 896, 0, 3600, 432000 },   // 13: V cloak of invisibility
	{ 825, 1, 3600, 432000 },   // 14: V death mask (invulnerability)
	{ 827, 0, 3600, 432000 },   // 15: V jump boots
	{ 828, 0, 3600, 432000 },   // 16: X raven flight
	{ 829, 0, 3600, 1728000 },  // 17: V guns akimbo
	{ 830, 0, 3600, 432000 },   // 18: V diving suit
	{ 831, 0, 3600, 432000 },   // 19: V gas mask
	{ -1, 0, 3600, 432000 },    // 20: X clone
	{ 2566, 0, 3600, 432000 },  // 21: V crystal ball
	{ 836, 0, 3600, 432000 },   // 22: X decoy
	{ 853, 0, 3600, 432000 },   // 23: V doppleganger
	{ 2428, 0, 3600, 432000 },  // 24: V reflective shots
	{ 839, 0, 3600, 432000 },   // 25: V beast vision
	{ 768, 0, 3600, 432000 },   // 26: X cloak of shadow (useless)
	{ 840, 0, 3600, 432000 },   // 27: X rage shroom
	{ 841, 0, 900, 432000 },    // 28: V delirium shroom
	{ 842, 0, 3600, 432000 },   // 29: V grow shroom (gModernMap only)
	{ 843, 0, 3600, 432000 },   // 30: V shrink shroom (gModernMap only)
	{ -1, 0, 3600, 432000 },    // 31: X death mask (useless)
	{ -1, 0, 3600, 432000 },    // 32: X wine goblet
	{ -1, 0, 3600, 432000 },    // 33: X wine bottle
	{ -1, 0, 3600, 432000 },    // 34: X skull grail
	{ -1, 0, 3600, 432000 },    // 35: X silver grail
	{ -1, 0, 3600, 432000 },    // 36: X tome
	{ -1, 0, 3600, 432000 },    // 37: X black chest
	{ -1, 0, 3600, 432000 },    // 38: X wooden chest
	{ 837, 1, 3600, 432000 },   // 39: V asbestos armor
	{ -1, 0, 1, 432000 },       // 40: V basic armor
	{ -1, 0, 1, 432000 },       // 41: V body armor
	{ -1, 0, 1, 432000 },       // 42: V fire armor
	{ -1, 0, 1, 432000 },       // 43: V spirit armor
	{ -1, 0, 1, 432000 },       // 44: V super armor
	{ 0, 0, 0, 0 },             // 45: ? unknown
	{ 0, 0, 0, 0 },             // 46: ? unknown
	{ 0, 0, 0, 0 },             // 47: ? unknown
	{ 0, 0, 0, 0 },             // 48: ? unknown
	{ 0, 0, 0, 0 },             // 49: X dummy
	{ 833, 1, 1, 1 }            // 50: V kModernItemLevelMap (gModernMap only)
};

int Handicap[] = {
	144, 208, 256, 304, 368
};

POSTURE gPostureDefaults[kModeMax][kPostureMax] = {

	// normal human
	{
		{ FixedToFloat<14>(0x4000), FixedToFloat<14>(0x4000), FixedToFloat<14>(0x4000), 14, 17, 0.09375, 0.0625, 0.125, 0.3125, 22, 18, 36, 12, -FixedToFloat(0xbaaaa), -FixedToFloat(0x175555) },
		{ FixedToFloat<14>(0x1200), FixedToFloat<14>(0x1200), FixedToFloat<14>(0x1200), 14, 17, 0.09375, 0.0625, 0.125, 0.3125, 20, 16, 44, -6, FixedToFloat(0x5b05), 0 },
		{ FixedToFloat<14>(0x2000), FixedToFloat<14>(0x2000), FixedToFloat<14>(0x2000), 22, 28, 0.09375, 0.0625, 0.0625, 0.15625, 8, 6, 44, -6, 0, 0 },
	},

	// normal beast
	{
		{ FixedToFloat<14>(0x4000), FixedToFloat<14>(0x4000), FixedToFloat<14>(0x4000), 14, 17, 0.09375, 0.0625, 0.125, 0.3125, 22, 18, 36, 12, -FixedToFloat(0xbaaaa), -FixedToFloat(0x175555) },
		{ FixedToFloat<14>(0x1200), FixedToFloat<14>(0x1200), FixedToFloat<14>(0x1200), 14, 17, 0.09375, 0.0625, 0.125, 0.3125, 20, 16, 44, -6, FixedToFloat(0x5b05), 0 },
		{ FixedToFloat<14>(0x2000), FixedToFloat<14>(0x2000), FixedToFloat<14>(0x2000), 22, 28, 0.09375, 0.0625, 0.0625, 0.15625, 8, 6, 44, -6, 0, 0 },
	},

	// shrink human
	{
		{ FixedToFloat<14>(10384), FixedToFloat<14>(10384), FixedToFloat<14>(10384), 14, 17, 0.09375, 0.0625, 0.125, 0.3125, 22, 18, 36, 12, -FixedToFloat(564586), -FixedToFloat(1329173) },
		{ FixedToFloat<14>(2108), FixedToFloat<14>(2108), FixedToFloat<14>(2108), 14, 17, 0.09375, 0.0625, 0.125, 0.3125, 20, 16, 44, -6, FixedToFloat(0x5b05), 0 },
		{ FixedToFloat<14>(2192), FixedToFloat<14>(2192), FixedToFloat<14>(2192), 22, 28, 0.09375, 0.0625, 0.0625, 0.15625, 8, 6, 44, -6, 0, 0 },
	},

	// grown human
	{
		{ FixedToFloat<14>(19384), FixedToFloat<14>(19384), FixedToFloat<14>(19384), 14, 17, 0.09375, 0.0625, 0.125, 0.3125, 22, 18, 36, 12, -FixedToFloat(1014586), -FixedToFloat(1779173) },
		{ FixedToFloat<14>(5608), FixedToFloat<14>(5608), FixedToFloat<14>(5608), 14, 17, 0.09375, 0.0625, 0.125, 0.3125, 20, 16, 44, -6, FixedToFloat(0x5b05), 0 },
		{ FixedToFloat<14>(11192), FixedToFloat<14>(11192), FixedToFloat<14>(11192), 22, 28, 0.09375, 0.0625, 0.0625, 0.15625, 8, 6, 44, -6, 0, 0 },
	},
};

AMMOINFO gAmmoInfo[] = {
	{ 0, -1 },
	{ 100, -1 },
	{ 100, 4 },
	{ 500, 5 },
	{ 100, -1 },
	{ 50, -1 },
	{ 2880, -1 },
	{ 250, -1 },
	{ 100, -1 },
	{ 100, -1 },
	{ 50, -1 },
	{ 50, -1 },
};

struct ARMORDATA {
	int armor0;
	int armor0max;
	int armor1;
	int armor1max;
	int armor2;
	int armor2max;
};
ARMORDATA armorData[5] = {
	{ 0x320, 0x640, 0x320, 0x640, 0x320, 0x640 },
	{ 0x640, 0x640, 0, 0x640, 0, 0x640 },
	{ 0, 0x640, 0x640, 0x640, 0, 0x640 },
	{ 0, 0x640, 0, 0x640, 0x640, 0x640 },
	{ 0xc80, 0xc80, 0xc80, 0xc80, 0xc80, 0xc80 }
};



struct VICTORY {
	const char* message;
	int Kills;
};

VICTORY gVictory[] = {
	{ "%s boned %s like a fish", 4100 },
	{ "%s castrated %s", 4101 },
	{ "%s creamed %s", 4102 },
	{ "%s destroyed %s", 4103 },
	{ "%s diced %s", 4104 },
	{ "%s disemboweled %s", 4105 },
	{ "%s flattened %s", 4106 },
	{ "%s gave %s Anal Justice", 4107 },
	{ "%s gave AnAl MaDnEsS to %s", 4108 },
	{ "%s hurt %s real bad", 4109 },
	{ "%s killed %s", 4110 },
	{ "%s made mincemeat out of %s", 4111 },
	{ "%s massacred %s", 4112 },
	{ "%s mutilated %s", 4113 },
	{ "%s reamed %s", 4114 },
	{ "%s ripped %s a new orifice", 4115 },
	{ "%s slaughtered %s", 4116 },
	{ "%s sliced %s", 4117 },
	{ "%s smashed %s", 4118 },
	{ "%s sodomized %s", 4119 },
	{ "%s splattered %s", 4120 },
	{ "%s squashed %s", 4121 },
	{ "%s throttled %s", 4122 },
	{ "%s wasted %s", 4123 },
	{ "%s body bagged %s", 4124 },
};

struct SUICIDE {
	const char* message;
	int Kills;
};

SUICIDE gSuicide[] = {
	{ "%s is excrement", 4202 },
	{ "%s is hamburger", 4203 },
	{ "%s suffered scrotum separation", 4204 },
	{ "%s volunteered for population control", 4206 },
	{ "%s has suicided", 4207 },
};

struct DAMAGEINFO {
	int armorType;
	int Kills[3];
	int at10[3];
};

DAMAGEINFO damageInfo[7] = {
	{ -1, 731, 732, 733, 710, 710, 710 },
	{ 1, 742, 743, 744, 711, 711, 711 },
	{ 0, 731, 732, 733, 712, 712, 712 },
	{ 1, 731, 732, 733, 713, 713, 713 },
	{ -1, 724, 724, 724, 714, 714, 714 },
	{ 2, 731, 732, 733, 715, 715, 715 },
	{ 0, 0, 0, 0, 0, 0, 0 }
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

inline bool IsTargetTeammate(DBloodPlayer* pSourcePlayer, DBloodActor* target)
{
	if (pSourcePlayer == nullptr)
		return false;
	if (!target->IsPlayerActor())
		return false;
	if (gGameOptions.nGameType == 1 || gGameOptions.nGameType == 3)
	{
		DBloodPlayer* pTargetPlayer = getPlayer(target->spr.type - kDudePlayer1);
		if (pSourcePlayer != pTargetPlayer)
		{
			if (gGameOptions.nGameType == 1)
				return true;
			if (gGameOptions.nGameType == 3 && (pSourcePlayer->teamId & 3) == (pTargetPlayer->teamId & 3))
				return true;
		}
	}

	return false;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int powerupCheck(DBloodPlayer* pPlayer, int nPowerUp)
{
	assert(pPlayer != NULL);
	assert(nPowerUp >= 0 && nPowerUp < kMaxPowerUps);
	int nPack = powerupToPackItem(nPowerUp);
	if (nPack >= 0 && !packItemActive(pPlayer, nPack))
		return 0;
	return pPlayer->pwUpTime[nPowerUp];
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool powerupActivate(DBloodPlayer* pPlayer, int nPowerUp)
{
	if (powerupCheck(pPlayer, nPowerUp) > 0 && gPowerUpInfo[nPowerUp].pickupOnce)
		return 0;
	if (!pPlayer->pwUpTime[nPowerUp])
		pPlayer->pwUpTime[nPowerUp] = gPowerUpInfo[nPowerUp].bonusTime;
	int nPack = powerupToPackItem(nPowerUp);
	if (nPack >= 0)
		pPlayer->packSlots[nPack].isActive = 1;

	switch (nPowerUp + kItemBase) {
#ifdef NOONE_EXTENSIONS
	case kItemModernMapLevel:
		if (gModernMap) gFullMap = true;
		break;
	case kItemShroomShrink:
		if (!gModernMap) break;
		else if (isGrown(pPlayer->GetActor())) playerDeactivateShrooms(pPlayer);
		else playerSizeShrink(pPlayer, 2);
		break;
	case kItemShroomGrow:
		if (!gModernMap) break;
		else if (isShrunk(pPlayer->GetActor())) playerDeactivateShrooms(pPlayer);
		else {
			playerSizeGrow(pPlayer, 2);
			if (powerupCheck(getPlayer(pPlayer->GetActor()->spr.type - kDudePlayer1), kPwUpShadowCloak) > 0) {
				powerupDeactivate(pPlayer, kPwUpShadowCloak);
				pPlayer->pwUpTime[kPwUpShadowCloak] = 0;
			}

			if (ceilIsTooLow(pPlayer->GetActor()))
				actDamageSprite(pPlayer->GetActor(), pPlayer->GetActor(), kDamageExplode, 65535);
		}
		break;
#endif
	case kItemFeatherFall:
	case kItemJumpBoots:
		pPlayer->damageControl[0]++;
		break;
	case kItemReflectShots: // reflective shots
		if (pPlayer->pnum == myconnectindex && gGameOptions.nGameType == 0)
			sfxSetReverb2(1);
		break;
	case kItemDeathMask:
		for (int i = 0; i < 7; i++)
			pPlayer->damageControl[i]++;
		break;
	case kItemDivingSuit: // diving suit
		pPlayer->damageControl[4]++;
		if (pPlayer->pnum == myconnectindex && gGameOptions.nGameType == 0)
			sfxSetReverb(1);
		break;
	case kItemGasMask:
		pPlayer->damageControl[4]++;
		break;
	case kItemArmorAsbest:
		pPlayer->damageControl[1]++;
		break;
	case kItemTwoGuns:
		pPlayer->newWeapon = pPlayer->curWeapon;
		WeaponRaise(pPlayer);
		break;
	}
	sfxPlay3DSound(pPlayer->GetActor(), 776, -1, 0);
	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void powerupDeactivate(DBloodPlayer* pPlayer, int nPowerUp)
{
	int nPack = powerupToPackItem(nPowerUp);
	if (nPack >= 0)
		pPlayer->packSlots[nPack].isActive = 0;

	switch (nPowerUp + kItemBase) {
#ifdef NOONE_EXTENSIONS
	case kItemShroomShrink:
		if (gModernMap) {
			playerSizeReset(pPlayer);
			if (ceilIsTooLow(pPlayer->GetActor()))
				actDamageSprite(pPlayer->GetActor(), pPlayer->GetActor(), kDamageExplode, 65535);
		}
		break;
	case kItemShroomGrow:
		if (gModernMap) playerSizeReset(pPlayer);
		break;
#endif
	case kItemFeatherFall:
	case kItemJumpBoots:
		pPlayer->damageControl[0]--;
		break;
	case kItemDeathMask:
		for (int i = 0; i < 7; i++)
			pPlayer->damageControl[i]--;
		break;
	case kItemDivingSuit:
		pPlayer->damageControl[4]--;
		if (pPlayer && pPlayer->pnum == myconnectindex && VanillaMode() ? true : pPlayer->pwUpTime[24] == 0)
			sfxSetReverb(0);
		break;
	case kItemReflectShots:
		if (pPlayer && pPlayer->pnum == myconnectindex && VanillaMode() ? true : pPlayer->packSlots[1].isActive == 0)
			sfxSetReverb(0);
		break;
	case kItemGasMask:
		pPlayer->damageControl[4]--;
		break;
	case kItemArmorAsbest:
		pPlayer->damageControl[1]--;
		break;
	case kItemTwoGuns:
		pPlayer->newWeapon = pPlayer->curWeapon;
		WeaponRaise(pPlayer);
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void powerupSetState(DBloodPlayer* pPlayer, int nPowerUp, bool bState)
{
	if (!bState)
		powerupActivate(pPlayer, nPowerUp);
	else
		powerupDeactivate(pPlayer, nPowerUp);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void powerupProcess(DBloodPlayer* pPlayer)
{
	pPlayer->packItemTime = ClipLow(pPlayer->packItemTime - 4, 0);
	for (int i = kMaxPowerUps - 1; i >= 0; i--)
	{
		int nPack = powerupToPackItem(i);
		if (nPack >= 0)
		{
			if (pPlayer->packSlots[nPack].isActive)
			{
				pPlayer->pwUpTime[i] = ClipLow(pPlayer->pwUpTime[i] - 4, 0);
				if (pPlayer->pwUpTime[i])
					pPlayer->packSlots[nPack].curAmount = (100 * pPlayer->pwUpTime[i]) / gPowerUpInfo[i].bonusTime;
				else
				{
					powerupDeactivate(pPlayer, i);
					if (pPlayer->packItemId == nPack)
						pPlayer->packItemId = 0;
				}
			}
		}
		else if (pPlayer->pwUpTime[i] > 0)
		{
			pPlayer->pwUpTime[i] = ClipLow(pPlayer->pwUpTime[i] - 4, 0);
			if (!pPlayer->pwUpTime[i])
				powerupDeactivate(pPlayer, i);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void powerupClear(DBloodPlayer* pPlayer)
{
	for (int i = kMaxPowerUps - 1; i >= 0; i--)
	{
		pPlayer->pwUpTime[i] = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int packItemToPowerup(int nPack)
{
	int nPowerUp = -1;
	switch (nPack) 
	{
		case kPackMedKit:
			break;
		case kPackDivingSuit:
			nPowerUp = kPwUpDivingSuit;
			break;
		case kPackCrystalBall:
			nPowerUp = kPwUpCrystalBall;
			break;
		case kPackBeastVision:
			nPowerUp = kPwUpBeastVision;
			break;
		case kPackJumpBoots:
			nPowerUp = kPwUpJumpBoots;
			break;
		default:
		I_Error("Unhandled pack item %d", nPack);
		break;
	}
	return nPowerUp;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int powerupToPackItem(int nPowerUp)
{
	switch (nPowerUp) 
	{
		case kPwUpDivingSuit:
			return kPackDivingSuit;
		case kPwUpCrystalBall:
			return kPackCrystalBall;
		case kPwUpBeastVision:
			return kPackBeastVision;
		case kPwUpJumpBoots:
			return kPackJumpBoots;
	}
	return -1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool packAddItem(DBloodPlayer* pPlayer, unsigned int nPack)
{
	if (nPack <= 4)
	{
		if (pPlayer->packSlots[nPack].curAmount >= 100)
			return 0;
		pPlayer->packSlots[nPack].curAmount = 100;
		int nPowerUp = packItemToPowerup(nPack);
		if (nPowerUp >= 0)
			pPlayer->pwUpTime[nPowerUp] = gPowerUpInfo[nPowerUp].bonusTime;
		if (pPlayer->packItemId == -1)
			pPlayer->packItemId = nPack;
		if (!pPlayer->packSlots[pPlayer->packItemId].curAmount)
			pPlayer->packItemId = nPack;
	}
	else
		I_Error("Unhandled pack item %d", nPack);
	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int packCheckItem(DBloodPlayer* pPlayer, int nPack)
{
	return pPlayer->packSlots[nPack].curAmount;
}

bool packItemActive(DBloodPlayer* pPlayer, int nPack)
{
	return pPlayer->packSlots[nPack].isActive;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void packUseItem(DBloodPlayer* pPlayer, int nPack)
{
	bool v4 = 0;
	int nPowerUp = -1;
	if (pPlayer->packSlots[nPack].curAmount > 0)
	{
		pPlayer->packItemId = nPack;

		switch (nPack)
		{
		case 0:
		{
			unsigned int health = pPlayer->GetActor()->xspr.health >> 4;
			if (health < 100)
			{
				int heal = ClipHigh(100 - health, pPlayer->packSlots[0].curAmount);
				actHealDude(pPlayer->GetActor(), heal, 100);
				pPlayer->packSlots[0].curAmount -= heal;
			}
			break;
		}
		case 1:
			v4 = 1;
			nPowerUp = kPwUpDivingSuit;
			break;
		case 2:
			v4 = 1;
			nPowerUp = kPwUpCrystalBall;
			break;
		case 3:
			v4 = 1;
			nPowerUp = kPwUpBeastVision;
			break;
		case 4:
			v4 = 1;
			nPowerUp = kPwUpJumpBoots;
			break;
		default:
			I_Error("Unhandled pack item %d", nPack);
			return;
		}
	}
	pPlayer->packItemTime = 0;
	if (v4)
		powerupSetState(pPlayer, nPowerUp, pPlayer->packSlots[nPack].isActive);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void packPrevItem(DBloodPlayer* pPlayer)
{
	if (pPlayer->packItemTime > 0)
	{
		for (int i = 0; i < 2; i++)
		{
			for (int nPrev = pPlayer->packItemId - 1; nPrev >= 0; nPrev--)
			{
				if (pPlayer->packSlots[nPrev].curAmount)
				{
					pPlayer->packItemId = nPrev;
					pPlayer->packItemTime = 600;
					return;
				}
			}
			pPlayer->packItemId = 4;
			if (pPlayer->packSlots[4].curAmount) break;
		}
	}

	pPlayer->packItemTime = 600;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void packNextItem(DBloodPlayer* pPlayer)
{
	if (pPlayer->packItemTime > 0)
	{
		for (int i = 0; i < 2; i++)
		{
			for (int nNext = pPlayer->packItemId + 1; nNext < 5; nNext++)
			{
				if (pPlayer->packSlots[nNext].curAmount)
				{
					pPlayer->packItemId = nNext;
					pPlayer->packItemTime = 600;
					return;
				}
			}
			pPlayer->packItemId = 0;
			if (pPlayer->packSlots[0].curAmount) break;
		}
	}
	pPlayer->packItemTime = 600;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool playerSeqPlaying(DBloodPlayer* pPlayer, int nSeq)
{
	int nCurSeq = seqGetID(pPlayer->GetActor());
	if (pPlayer->pDudeInfo->seqStartID + nSeq == nCurSeq && seqGetStatus(pPlayer->GetActor()) >= 0)
		return 1;
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void playerSetRace(DBloodPlayer* pPlayer, int nLifeMode)
{
	assert(nLifeMode >= kModeHuman && nLifeMode <= kModeHumanGrown);
	DUDEINFO* pDudeInfo = pPlayer->pDudeInfo;
	*pDudeInfo = gPlayerTemplate[nLifeMode];
	pPlayer->lifeMode = nLifeMode;

	// By NoOne: don't forget to change clipdist for grow and shrink modes
	pPlayer->GetActor()->clipdist = pDudeInfo->fClipdist();

	for (int i = 0; i < 7; i++)
		pDudeInfo->damageVal[i] = MulScale(Handicap[gSkill], pDudeInfo->startDamage[i], 8);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void playerSetGodMode(DBloodPlayer* pPlayer, bool bGodMode)
{
	pPlayer->godMode = bGodMode;
}

void playerResetInertia(DBloodPlayer* pPlayer)
{
	POSTURE* pPosture = &pPlayer->pPosture[pPlayer->lifeMode][pPlayer->posture];
	pPlayer->zView = pPlayer->GetActor()->spr.pos.Z - pPosture->eyeAboveZ;
	pPlayer->zWeapon = pPlayer->GetActor()->spr.pos.Z - pPosture->weaponAboveZ;
	pPlayer->GetActor()->oviewzoffset = pPlayer->GetActor()->viewzoffset = pPlayer->zView - pPlayer->GetActor()->spr.pos.Z;
	viewBackupView(pPlayer);
}

void playerCorrectInertia(DBloodPlayer* pPlayer, const DVector3& oldpos)
{
	auto zAdj = pPlayer->GetActor()->spr.pos.Z - oldpos.Z;
	pPlayer->zView += zAdj;
	pPlayer->zWeapon += zAdj;
	pPlayer->GetActor()->opos += pPlayer->GetActor()->spr.pos.XY() - oldpos.XY();
	pPlayer->ozView += zAdj;
	pPlayer->GetActor()->opos.Z += zAdj;
}

void playerResetPowerUps(DBloodPlayer* pPlayer)
{
	for (int i = 0; i < kMaxPowerUps; i++) {
		if (!VanillaMode() && (i == kPwUpJumpBoots || i == kPwUpDivingSuit || i == kPwUpCrystalBall || i == kPwUpBeastVision))
			continue;
		pPlayer->pwUpTime[i] = 0;
	}
}

void playerResetPosture(DBloodPlayer* pPlayer) {
	memcpy(pPlayer->pPosture, gPostureDefaults, sizeof(gPostureDefaults));
	if (!VanillaMode()) {
		pPlayer->bobPhase = 0;
		pPlayer->bobAmp = 0;
		pPlayer->swayAmp = 0;
		pPlayer->bobHeight = 0;
		pPlayer->bobWidth = 0;
		pPlayer->swayHeight = 0;
		pPlayer->swayWidth = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void playerStart(int nPlayer, int bNewLevel)
{
	DBloodPlayer* pPlayer = getPlayer(nPlayer);
	InputPacket* pInput = &pPlayer->cmd.ucmd;
	ZONE* pStartZone = NULL;

	// normal start position
	if (gGameOptions.nGameType <= 1)
		pStartZone = &gStartZone[nPlayer];

#ifdef NOONE_EXTENSIONS
	// let's check if there is positions of teams is specified
	// if no, pick position randomly, just like it works in vanilla.
	else if (gModernMap && gGameOptions.nGameType == 3 && gTeamsSpawnUsed == true) {
		int maxRetries = 5;
		while (maxRetries-- > 0) {
			if (pPlayer->teamId == 0) pStartZone = &gStartZoneTeam1[Random(3)];
			else pStartZone = &gStartZoneTeam2[Random(3)];

			if (maxRetries != 0) {
				// check if there is no spawned player in selected zone
				BloodSectIterator it(pStartZone->sector);
				while (auto act = it.Next())
				{
					if (pStartZone->pos.XY() == act->spr.pos.XY() && act->IsPlayerActor()) {
						pStartZone = NULL;
						break;
					}
				}
			}

			if (pStartZone != NULL)
				break;
		}

	}
#endif
	else {
		pStartZone = &gStartZone[Random(8)];
	}

	auto actor = actSpawnSprite(pStartZone->sector, pStartZone->pos, 6, 1);
	assert(actor->hasX());
	pPlayer->actor = actor;
	pPlayer->InitAngles();
	DUDEINFO* pDudeInfo = &dudeInfo[kDudePlayer1 + nPlayer - kDudeBase];
	pPlayer->pDudeInfo = pDudeInfo;
	playerSetRace(pPlayer, kModeHuman);
	playerResetPosture(pPlayer);
	seqSpawn(pDudeInfo->seqStartID, actor, -1);
	if (nPlayer == myconnectindex)
		actor->spr.cstat2 |= CSTAT2_SPRITE_MAPPED;
	double top, bottom;
	GetActorExtents(actor, &top, &bottom);
	actor->spr.pos.Z -= bottom - actor->spr.pos.Z;
	actor->spr.pal = 11 + (pPlayer->teamId & 3);
	actor->spr.Angles.Yaw = pStartZone->angle;
	actor->spr.type = kDudePlayer1 + nPlayer;
	actor->clipdist = pDudeInfo->fClipdist();
	actor->spr.flags = 15;
	actor->xspr.burnTime = 0;
	actor->SetBurnSource(nullptr);
	pPlayer->GetActor()->xspr.health = pDudeInfo->startHealth << 4;
	pPlayer->GetActor()->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
	pPlayer->GetActor()->spr.Angles.Pitch = pPlayer->ViewAngles.Pitch = nullAngle;
	pPlayer->slope = 0;
	pPlayer->fragger = nullptr;
	pPlayer->underwaterTime = 1200;
	pPlayer->bubbleTime = 0;
	pPlayer->restTime = 0;
	pPlayer->kickPower = 0;
	pPlayer->laughCount = 0;
	pPlayer->YawSpin = nullAngle;
	pPlayer->posture = 0;
	pPlayer->voodooTarget = nullptr;
	pPlayer->voodooTargets = 0;
	pPlayer->voodooVar1 = 0;
	pPlayer->vodooVar2 = 0;
	playerResetInertia(pPlayer);
	pPlayer->zWeaponVel = 0;
	pPlayer->relAim.X = 1;
	pPlayer->relAim.Y = 0;
	pPlayer->relAim.Z = 0;
	pPlayer->aimTarget = nullptr;
	pPlayer->zViewVel = pPlayer->zWeaponVel;
	if (!(gGameOptions.nGameType == 1 && gGameOptions.bKeepKeysOnRespawn && !bNewLevel))
		for (int i = 0; i < 8; i++)
			pPlayer->hasKey[i] = gGameOptions.nGameType >= 2;
	pPlayer->hasFlag = 0;
	for (int i = 0; i < 2; i++)
		pPlayer->ctfFlagState[i] = nullptr;
	for (int i = 0; i < 7; i++)
		pPlayer->damageControl[i] = 0;
	if (pPlayer->godMode)
		playerSetGodMode(pPlayer, 1);
	gInfiniteAmmo = 0;
	gFullMap = 0;
	pPlayer->throwPower = 0;
	pPlayer->deathTime = 0;
	pPlayer->nextWeapon = kWeapNone;
	actor->vel.Zero();
	pInput->actions = 0;
	pInput->vel.Zero();
	pInput->ang.Zero();
	pPlayer->flickerEffect = 0;
	pPlayer->quakeEffect = 0;
	pPlayer->tiltEffect = 0;
	pPlayer->visibility = 0;
	pPlayer->painEffect = 0;
	pPlayer->blindEffect = 0;
	pPlayer->chokeEffect = 0;
	pPlayer->handTime = 0;
	pPlayer->weaponTimer = 0;
	pPlayer->weaponState = 0;
	pPlayer->weaponQav = kQAVNone;
	pPlayer->qavLastTick = 0;
	pPlayer->qavTimer = 0;
#ifdef NOONE_EXTENSIONS
	playerQavSceneReset(pPlayer); // reset qav scene

	// assign or update player's sprite index for conditions
	if (gModernMap) {

		BloodStatIterator it(kStatModernPlayerLinker);
		while (auto iactor = it.Next())
		{
			if (!iactor->xspr.data1 || iactor->xspr.data1 == pPlayer->pnum + 1)
			{
				DBloodActor* SpriteOld = iactor->prevmarker;
				trPlayerCtrlLink(iactor, pPlayer, (SpriteOld == nullptr)); // this modifies iactor's prevmarker field!
				if (SpriteOld)
					conditionsUpdateIndex(SpriteOld, iactor->prevmarker);
			}
		}

	}

#endif
	pPlayer->hand = 0;
	pPlayer->nWaterPal = 0;
	playerResetPowerUps(pPlayer);

	if (nPlayer == myconnectindex)
	{
		viewInitializePrediction();
	}
	if (IsUnderwaterSector(actor->sector()))
	{
		pPlayer->posture = 1;
		pPlayer->GetActor()->xspr.medium = kMediumWater;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void playerReset(DBloodPlayer* pPlayer)
{
	static int dword_136400[] = {
		3, 4, 2, 8, 9, 10, 7, 1, 1, 1, 1, 1, 1, 1
	};
	static int dword_136438[] = {
		3, 4, 2, 8, 9, 10, 7, 1, 1, 1, 1, 1, 1, 1
	};
	assert(pPlayer != NULL);
	for (int i = 0; i < 14; i++)
	{
		pPlayer->hasWeapon[i] = gInfiniteAmmo;
		pPlayer->weaponMode[i] = 0;
	}
	pPlayer->hasWeapon[kWeapPitchFork] = 1;
	pPlayer->curWeapon = kWeapNone;
	pPlayer->qavCallback = -1;
	pPlayer->newWeapon = kWeapPitchFork;
	for (int i = 0; i < 14; i++)
	{
		pPlayer->weaponOrder[0][i] = dword_136400[i];
		pPlayer->weaponOrder[1][i] = dword_136438[i];
	}
	for (int i = 0; i < 12; i++)
	{
		if (gInfiniteAmmo)
			pPlayer->ammoCount[i] = gAmmoInfo[i].max;
		else
			pPlayer->ammoCount[i] = 0;
	}
	for (int i = 0; i < 3; i++)
		pPlayer->armor[i] = 0;
	pPlayer->weaponTimer = 0;
	pPlayer->weaponState = 0;
	pPlayer->weaponQav = kQAVNone;
	pPlayer->qavLoop = 0;
	pPlayer->qavLastTick = 0;
	pPlayer->qavTimer = 0;
	pPlayer->packItemId = -1;

	for (int i = 0; i < 5; i++) {
		pPlayer->packSlots[i].isActive = 0;
		pPlayer->packSlots[i].curAmount = 0;
	}
#ifdef NOONE_EXTENSIONS
	playerQavSceneReset(pPlayer);
#endif
	// reset posture (mainly required for resetting movement speed and jump height)
	playerResetPosture(pPlayer);

}

int team_score[8];
int team_ticker[8];

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void playerInit(int nPlayer, unsigned int a2)
{
	if (!(a2 & 1))
		getPlayer(nPlayer)->Clear();

	DBloodPlayer* pPlayer = getPlayer(nPlayer);
	pPlayer->pnum = nPlayer;
	pPlayer->teamId = nPlayer;
	if (gGameOptions.nGameType == 3)
		pPlayer->teamId = nPlayer & 1;
	pPlayer->fragCount = 0;
	memset(team_score, 0, sizeof(team_score));
	memset(team_ticker, 0, sizeof(team_ticker));
	memset(pPlayer->fragInfo, 0, sizeof(pPlayer->fragInfo));

	if (!(a2 & 1))
		playerReset(pPlayer);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool findDroppedLeech(DBloodPlayer* a1, DBloodActor* a2)
{
	BloodStatIterator it(kStatThing);
	while (auto actor = it.Next())
	{
		if (a2 == actor)
			continue;
		if (actor->spr.type == kThingDroppedLifeLeech && actor->GetOwner() == a1->GetActor())
			return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool PickupItem(DBloodPlayer* pPlayer, DBloodActor* itemactor)
{
	char buffer[80];
	int pickupSnd = 775;
	int nType = itemactor->spr.type - kItemBase;
	auto plActor = pPlayer->GetActor();

	switch (itemactor->spr.type) {
	case kItemShadowCloak:
#ifdef NOONE_EXTENSIONS
		if (isGrown(pPlayer->GetActor()) || !powerupActivate(pPlayer, nType)) return false;
#else
		if (!powerupActivate(pPlayer, nType)) return false;
#endif
		break;
#ifdef NOONE_EXTENSIONS
	case kItemShroomShrink:
	case kItemShroomGrow:

		if (gModernMap) {
			switch (itemactor->spr.type) {
			case kItemShroomShrink:
				if (isShrunk(pPlayer->GetActor())) return false;
				break;
			case kItemShroomGrow:
				if (isGrown(pPlayer->GetActor())) return false;
				break;
			}

			powerupActivate(pPlayer, nType);
		}

		break;
#endif
	case kItemFlagABase:
	case kItemFlagBBase: {
		if (gGameOptions.nGameType != 3 || !itemactor->hasX()) return 0;
		if (itemactor->spr.type == kItemFlagABase) {
			if (pPlayer->teamId == 1) {
				if ((pPlayer->hasFlag & 1) == 0 && itemactor->xspr.state) {
					pPlayer->hasFlag |= 1;
					pPlayer->ctfFlagState[0] = itemactor;
					trTriggerSprite(itemactor, kCmdOff);
					snprintf(buffer, sizeof(buffer), "%s stole Blue Flag", PlayerName(pPlayer->pnum));
					sndStartSample(8007, 255, 2, 0);
					viewSetMessage(buffer);
				}
			}

			if (pPlayer->teamId == 0) {

				if ((pPlayer->hasFlag & 1) != 0 && !itemactor->xspr.state) {
					pPlayer->hasFlag &= ~1;
					pPlayer->ctfFlagState[0] = nullptr;
					trTriggerSprite(itemactor, kCmdOn);
					snprintf(buffer, sizeof(buffer), "%s returned Blue Flag", PlayerName(pPlayer->pnum));
					sndStartSample(8003, 255, 2, 0);
					viewSetMessage(buffer);
				}

				if ((pPlayer->hasFlag & 2) != 0 && itemactor->xspr.state) {
					pPlayer->hasFlag &= ~2;
					pPlayer->ctfFlagState[1] = nullptr;
					team_score[pPlayer->teamId] += 10;
					team_ticker[pPlayer->teamId] += 240;
					evSendGame(81, kCmdOn);
					snprintf(buffer, sizeof(buffer), "%s captured Red Flag!", PlayerName(pPlayer->pnum));
					sndStartSample(8001, 255, 2, 0);
					viewSetMessage(buffer);
				}
			}

		}
		else if (itemactor->spr.type == kItemFlagBBase) {

			if (pPlayer->teamId == 0) {
				if ((pPlayer->hasFlag & 2) == 0 && itemactor->xspr.state) {
					pPlayer->hasFlag |= 2;
					pPlayer->ctfFlagState[1] = itemactor;
					trTriggerSprite(itemactor, kCmdOff);
					snprintf(buffer, sizeof(buffer), "%s stole Red Flag", PlayerName(pPlayer->pnum));
					sndStartSample(8006, 255, 2, 0);
					viewSetMessage(buffer);
				}
			}

			if (pPlayer->teamId == 1) {
				if ((pPlayer->hasFlag & 2) != 0 && !itemactor->xspr.state)
				{
					pPlayer->hasFlag &= ~2;
					pPlayer->ctfFlagState[1] = nullptr;
					trTriggerSprite(itemactor, kCmdOn);
					snprintf(buffer, sizeof(buffer), "%s returned Red Flag", PlayerName(pPlayer->pnum));
					sndStartSample(8002, 255, 2, 0);
					viewSetMessage(buffer);
				}
				if ((pPlayer->hasFlag & 1) != 0 && itemactor->xspr.state)
				{
					pPlayer->hasFlag &= ~1;
					pPlayer->ctfFlagState[0] = nullptr;
					team_score[pPlayer->teamId] += 10;
					team_ticker[pPlayer->teamId] += 240;
					evSendGame(80, kCmdOn);
					snprintf(buffer, sizeof(buffer), "%s captured Blue Flag!", PlayerName(pPlayer->pnum));
					sndStartSample(8000, 255, 2, 0);
					viewSetMessage(buffer);
				}
			}
		}
	}
					   return 0;
	case kItemFlagA: {
		if (gGameOptions.nGameType != 3) return 0;
		gBlueFlagDropped = false;
		const bool enemyTeam = (pPlayer->teamId & 1) == 1;
		if (!enemyTeam && itemactor->GetOwner())
		{
			pPlayer->hasFlag &= ~1;
			pPlayer->ctfFlagState[0] = nullptr;
			trTriggerSprite(itemactor->GetOwner(), kCmdOn);
			snprintf(buffer, sizeof(buffer), "%s returned Blue Flag", PlayerName(pPlayer->pnum));
			sndStartSample(8003, 255, 2, 0);
			viewSetMessage(buffer);
			break;
		}
		pPlayer->hasFlag |= 1;
		pPlayer->ctfFlagState[0] = itemactor->GetOwner();
		if (enemyTeam)
		{
			snprintf(buffer, sizeof(buffer), "%s stole Blue Flag", PlayerName(pPlayer->pnum));
			sndStartSample(8007, 255, 2, 0);
			viewSetMessage(buffer);
		}
		break;
	}
	case kItemFlagB: {
		if (gGameOptions.nGameType != 3) return 0;
		gRedFlagDropped = false;
		const bool enemyTeam = (pPlayer->teamId & 1) == 0;
		if (!enemyTeam && itemactor->GetOwner())
		{
			pPlayer->hasFlag &= ~2;
			pPlayer->ctfFlagState[1] = nullptr;
			trTriggerSprite(itemactor->GetOwner(), kCmdOn);
			snprintf(buffer, sizeof(buffer), "%s returned Red Flag", PlayerName(pPlayer->pnum));
			sndStartSample(8002, 255, 2, 0);
			viewSetMessage(buffer);
			break;
		}
		pPlayer->hasFlag |= 2;
		pPlayer->ctfFlagState[1] = itemactor->GetOwner();
		if (enemyTeam)
		{
			snprintf(buffer, sizeof(buffer), "%s stole Red Flag", PlayerName(pPlayer->pnum));
			sndStartSample(8006, 255, 2, 0);
			viewSetMessage(buffer);
		}
		break;
	}
	case kItemArmorBasic:
	case kItemArmorBody:
	case kItemArmorFire:
	case kItemArmorSpirit:
	case kItemArmorSuper: {
		ARMORDATA* pArmorData = &armorData[itemactor->spr.type - kItemArmorBasic]; bool pickedUp = false;
		if (pPlayer->armor[1] < pArmorData->armor1max) {
			pPlayer->armor[1] = ClipHigh(pPlayer->armor[1] + pArmorData->armor1, pArmorData->armor1max);
			pickedUp = true;
		}

		if (pPlayer->armor[0] < pArmorData->armor0max) {
			pPlayer->armor[0] = ClipHigh(pPlayer->armor[0] + pArmorData->armor0, pArmorData->armor0max);
			pickedUp = true;
		}

		if (pPlayer->armor[2] < pArmorData->armor2max) {
			pPlayer->armor[2] = ClipHigh(pPlayer->armor[2] + pArmorData->armor2, pArmorData->armor2max);
			pickedUp = true;
		}

		if (!pickedUp) return 0;
		pickupSnd = 779;
		break;
	}
	case kItemCrystalBall:
		if (gGameOptions.nGameType == 0 || !packAddItem(pPlayer, gItemData[nType].packSlot)) return 0;
		break;
	case kItemKeySkull:
	case kItemKeyEye:
	case kItemKeyFire:
	case kItemKeyDagger:
	case kItemKeySpider:
	case kItemKeyMoon:
	case kItemKeyKey7:
		if (pPlayer->hasKey[itemactor->spr.type - 99]) return 0;
		pPlayer->hasKey[itemactor->spr.type - 99] = 1;
		pickupSnd = 781;
		break;
	case kItemHealthMedPouch:
	case kItemHealthLifeEssense:
	case kItemHealthLifeSeed:
	case kItemHealthRedPotion: {
		int addPower = gPowerUpInfo[nType].bonusTime;
#ifdef NOONE_EXTENSIONS
		// allow custom amount for item
		if (gModernMap && itemactor->hasX() && itemactor->xspr.data1 > 0)
			addPower = itemactor->xspr.data1;
#endif

		if (!actHealDude(pPlayer->GetActor(), addPower, gPowerUpInfo[nType].maxTime)) return 0;
		return 1;
	}
		[[fallthrough]];
	case kItemHealthDoctorBag:
	case kItemJumpBoots:
	case kItemDivingSuit:
	case kItemBeastVision:
		if (!packAddItem(pPlayer, gItemData[nType].packSlot)) return 0;
		break;
	default:
		if (!powerupActivate(pPlayer, nType)) return 0;
		return 1;
	}

	sfxPlay3DSound(plActor->spr.pos, pickupSnd, plActor->sector());
	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool PickupAmmo(DBloodPlayer* pPlayer, DBloodActor* ammoactor)
{
	const AMMOITEMDATA* pAmmoItemData = &gAmmoItemData[ammoactor->spr.type - kItemAmmoBase];
	int nAmmoType = pAmmoItemData->type;

	if (pPlayer->ammoCount[nAmmoType] >= gAmmoInfo[nAmmoType].max) return 0;
#ifdef NOONE_EXTENSIONS
	else if (gModernMap && ammoactor->hasX() && ammoactor->xspr.data1 > 0) // allow custom amount for item
		pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType] + ammoactor->xspr.data1, gAmmoInfo[nAmmoType].max);
#endif
	else
		pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType] + pAmmoItemData->count, gAmmoInfo[nAmmoType].max);

	if (pAmmoItemData->weaponType)  pPlayer->hasWeapon[pAmmoItemData->weaponType] = 1;
	sfxPlay3DSound(pPlayer->GetActor(), 782, -1, 0);
	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool PickupWeapon(DBloodPlayer* pPlayer, DBloodActor* weaponactor)
{
	const WEAPONITEMDATA* pWeaponItemData = &gWeaponItemData[weaponactor->spr.type - kItemWeaponBase];
	int nWeaponType = pWeaponItemData->type;
	int nAmmoType = pWeaponItemData->ammoType;
	if (!pPlayer->hasWeapon[nWeaponType] || gGameOptions.nWeaponSettings == 2 || gGameOptions.nWeaponSettings == 3) {
		if (weaponactor->spr.type == kItemWeaponLifeLeech && gGameOptions.nGameType > 1 && findDroppedLeech(pPlayer, NULL))
			return 0;
		pPlayer->hasWeapon[nWeaponType] = 1;
		if (nAmmoType == -1) return 0;
		// allow to set custom ammo count for weapon pickups
#ifdef NOONE_EXTENSIONS
		else if (gModernMap && weaponactor->hasX() && weaponactor->xspr.data1 > 0)
			pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType] + weaponactor->xspr.data1, gAmmoInfo[nAmmoType].max);
#endif
		else
			pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType] + pWeaponItemData->count, gAmmoInfo[nAmmoType].max);

		int nNewWeapon = WeaponUpgrade(pPlayer, nWeaponType);
		if (nNewWeapon != pPlayer->curWeapon) {
			pPlayer->weaponState = 0;
			pPlayer->nextWeapon = nNewWeapon;
		}
		sfxPlay3DSound(pPlayer->GetActor(), 777, -1, 0);
		return 1;
	}

	if (!actGetRespawnTime(weaponactor) || nAmmoType == -1 || pPlayer->ammoCount[nAmmoType] >= gAmmoInfo[nAmmoType].max) return 0;
#ifdef NOONE_EXTENSIONS
	else if (gModernMap && weaponactor->hasX() && weaponactor->xspr.data1 > 0)
		pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType] + weaponactor->xspr.data1, gAmmoInfo[nAmmoType].max);
#endif
	else
		pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType] + pWeaponItemData->count, gAmmoInfo[nAmmoType].max);

	sfxPlay3DSound(pPlayer->GetActor(), 777, -1, 0);
	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PickUp(DBloodPlayer* pPlayer, DBloodActor* actor)
{
	const char* msg = nullptr;
	int nType = actor->spr.type;
	bool pickedUp = 0;
	int customMsg = -1;
#ifdef NOONE_EXTENSIONS
	if (gModernMap && actor->hasX()) { // allow custom INI message instead "Picked up"
		if (actor->xspr.txID != 3 && actor->xspr.lockMsg > 0)
			customMsg = actor->xspr.lockMsg;
	}
#endif

	if (nType >= kItemBase && nType <= kItemMax) {
		pickedUp = PickupItem(pPlayer, actor);
		if (pickedUp && customMsg == -1) msg = GStrings.GetString(FStringf("TXTB_ITEM%02d", int(nType - kItemBase + 1)));

	}
	else if (nType >= kItemAmmoBase && nType < kItemAmmoMax) {
		pickedUp = PickupAmmo(pPlayer, actor);
		if (pickedUp && customMsg == -1) msg = GStrings.GetString(FStringf("TXTB_AMMO%02d", int(nType - kItemAmmoBase + 1)));

	}
	else if (nType >= kItemWeaponBase && nType < kItemWeaponMax) {
		pickedUp = PickupWeapon(pPlayer, actor);
		if (pickedUp && customMsg == -1) msg = GStrings.GetString(FStringf("TXTB_WPN%02d", int(nType - kItemWeaponBase + 1)));
	}

	if (!pickedUp) return;
	else if (actor->hasX())
	{
		if (actor->xspr.Pickup)
			trTriggerSprite(actor, kCmdSpritePickup);
	}

	if (!actCheckRespawn(actor))
		actPostSprite(actor, kStatFree);

	pPlayer->pickupEffect = 30;
	if (pPlayer->pnum == myconnectindex) {
		if (customMsg > 0) trTextOver(customMsg - 1);
		else if (msg) viewSetMessage(msg, nullptr, MESSAGE_PRIORITY_PICKUP);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void CheckPickUp(DBloodPlayer* pPlayer)
{
	auto plActor = pPlayer->GetActor();
	auto ppos = plActor->spr.pos;
	auto pSector = plActor->sector();
	BloodStatIterator it(kStatItem);
	while (auto itemactor = it.Next())
	{
		if (itemactor->spr.flags & 32)
			continue;
		double dx = abs(ppos.X - itemactor->spr.pos.X);
		if (dx > 48)
			continue;
		double dy = abs(ppos.Y - itemactor->spr.pos.Y);
		if (dy > 48)
			continue;
		double top, bottom;
		GetActorExtents(plActor, &top, &bottom);
		double vb = 0;
		if (itemactor->spr.pos.Z < top)
			vb = (top - itemactor->spr.pos.Z);
		else if (itemactor->spr.pos.Z > bottom)
			vb = (itemactor->spr.pos.Z - bottom);
		if (vb > 32)
			continue;
		if (DVector2(dx, dy).LengthSquared() > 48*48)
			continue;
		GetActorExtents(itemactor, &top, &bottom);
		if (cansee(ppos, pSector, itemactor->spr.pos, itemactor->sector())
			|| cansee(ppos, pSector, DVector3(itemactor->spr.pos.XY(), top), itemactor->sector())
			|| cansee(ppos, pSector, DVector3(itemactor->spr.pos.XY(), bottom), itemactor->sector()))
			PickUp(pPlayer, itemactor);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int ActionScan(DBloodPlayer* pPlayer, HitInfo* out)
{
	auto plActor = pPlayer->GetActor();
	*out = {};
	auto pos = DVector3(plActor->spr.Angles.Yaw.ToVector(), pPlayer->slope);
	int hit = HitScan(pPlayer->GetActor(), pPlayer->zView, pos, 0x10000040, 128);
	double hitDist = (plActor->spr.pos.XY() - gHitInfo.hitpos.XY()).Length();
	if (hitDist < 64)
	{
		switch (hit)
		{
		case 3:
		{
			auto hitactor = gHitInfo.actor();
			if (!hitactor || !hitactor->hasX()) return -1;
			out->hitActor = hitactor;
			if (hitactor->spr.statnum == kStatThing)
			{
				if (hitactor->spr.type == kThingDroppedLifeLeech)
				{
					if (gGameOptions.nGameType > 1 && findDroppedLeech(pPlayer, hitactor))
						return -1;
					hitactor->xspr.data4 = pPlayer->pnum;
					hitactor->xspr.isTriggered = 0;
				}
			}
			if (hitactor->xspr.Push)
				return 3;
			if (hitactor->spr.statnum == kStatDude)
			{
				int nMass = getDudeInfo(hitactor->spr.type)->mass;
				if (nMass)
				{
					hitactor->vel += pos * (FixedToFloat<10>(0xccccc) / nMass);
				}
				if (hitactor->xspr.Push && !hitactor->xspr.state && !hitactor->xspr.isTriggered)
					trTriggerSprite(hitactor, kCmdSpritePush);
			}
			break;
		}
		case 0:
		case 4:
		{
			auto pWall = gHitInfo.hitWall;
			out->hitWall = gHitInfo.hitWall;
			if (pWall->hasX() && pWall->xw().triggerPush)
				return 0;
			if (pWall->twoSided())
			{
				auto sect = pWall->nextSector();
				out->hitWall = nullptr;
				out->hitSector = sect;
				if (sect->hasX() && sect->xs().Wallpush)
					return 6;
			}
			break;
		}
		case 1:
		case 2:
		{
			auto pSector = gHitInfo.hitSector;
			out->hitSector = gHitInfo.hitSector;
			if (pSector->hasX() && pSector->xs().Push)
				return 6;
			break;
		}
		}
	}
	out->hitSector = plActor->sector();
	if (plActor->sector()->hasX() && plActor->sector()->xs().Push)
		return 6;
	return -1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ProcessInput(DBloodPlayer* pPlayer)
{
	enum
	{
		Item_MedKit = 0,
		Item_CrystalBall = 1,
		Item_BeastVision = 2,
		Item_JumpBoots = 3
	};

	DBloodActor* actor = pPlayer->GetActor();
	POSTURE* pPosture = &pPlayer->pPosture[pPlayer->lifeMode][pPlayer->posture];
	InputPacket* pInput = &pPlayer->cmd.ucmd;

	// Originally, this was never able to be true due to sloppy input code in the original game.
	// Allow it to become true behind a CVAR to offer an alternate playing experience if desired.
	pPlayer->isRunning = !!(pInput->actions & SB_RUN) && !cl_bloodvanillarun;

	if ((pInput->actions & SB_BUTTON_MASK) || !pInput->vel.XY().isZero() || pInput->ang.Yaw.Degrees())
		pPlayer->restTime = 0;
	else if (pPlayer->restTime >= 0)
		pPlayer->restTime += 4;
	WeaponProcess(pPlayer);
	if (actor->xspr.health == 0)
	{
		// force synchronised input upon death.
		gameInput.ForceInputSync(pPlayer->pnum);

		bool bSeqStat = playerSeqPlaying(pPlayer, 16);
		DBloodActor* fragger = pPlayer->fragger;
		if (fragger)
		{
			pPlayer->GetActor()->spr.Angles.Yaw = (fragger->spr.pos.XY() - actor->spr.pos.XY()).Angle();
		}
		pPlayer->deathTime += 4;
		if (!bSeqStat)
			pPlayer->GetActor()->spr.Angles.Pitch = gi->playerPitchMax() * (1. - BobVal(min((pPlayer->deathTime << 3) + 512, 1536))) * 0.5;
		if (pPlayer->curWeapon)
			pInput->setNewWeapon(pPlayer->curWeapon);
		if (pInput->actions & SB_OPEN)
		{
			if (bSeqStat)
			{
				if (pPlayer->deathTime > 360)
					seqSpawn(pPlayer->pDudeInfo->seqStartID + 14, pPlayer->GetActor(), nPlayerSurviveClient);
			}
			else if (seqGetStatus(pPlayer->GetActor()) < 0)
			{
				if (pPlayer->GetActor())
					pPlayer->GetActor()->spr.type = kThingBloodChunks;
				actPostSprite(pPlayer->GetActor(), kStatThing);
				seqSpawn(pPlayer->pDudeInfo->seqStartID + 15, pPlayer->GetActor(), -1);
				playerReset(pPlayer);
				if (gGameOptions.nGameType == 0 && numplayers == 1)
				{
					gameaction = ga_autoloadgame;
				}
				else
					playerStart(pPlayer->pnum);
			}
			pInput->actions &= ~SB_OPEN;
		}
		return;
	}

	if (!pInput->vel.XY().isZero() && (pPlayer->posture == 1 || actor->xspr.height < 256))
	{
		const double speed = pPlayer->posture == 1? 1. : 1. - (actor->xspr.height * (1. / 256.) * (actor->xspr.height < 256));
		pInput->vel.X *= pInput->vel.X > 0 ? pPosture->frontAccel : pPosture->backAccel;
		pInput->vel.Y *= pPosture->sideAccel;
		actor->vel += pInput->vel.XY().Rotated(actor->spr.Angles.Yaw) * speed;
		pPlayer->RollVel += pInput->vel.Y * speed;
	}

	pPlayer->doViewYaw();
	pPlayer->doYawInput();

	if (!(pInput->actions & SB_JUMP))
		pPlayer->cantJump = 0;

	switch (pPlayer->posture) {
	case kPostureSwim:
	{
		const auto kbdDir = !!(pInput->actions & SB_JUMP) - !!(pInput->actions & SB_CROUCH);
		const double dist = pPosture->normalJumpZ;
		actor->vel.Z -= clamp(dist * kbdDir + dist * pInput->vel.Z, -dist, dist);
		break;
	}
	case kPostureCrouch:
		if (!(pInput->actions & SB_CROUCH))
			pPlayer->posture = kPostureStand;
		break;
	default:
		if (!pPlayer->cantJump && (pInput->actions & SB_JUMP) && actor->xspr.height == 0) {
#ifdef NOONE_EXTENSIONS
			if ((packItemActive(pPlayer, kPackJumpBoots) && pPosture->pwupJumpZ != 0) || pPosture->normalJumpZ != 0)
#endif
				sfxPlay3DSound(actor, 700, 0, 0);

			if (packItemActive(pPlayer, 4)) actor->vel.Z = pPosture->pwupJumpZ; //-0x175555;
			else actor->vel.Z = pPosture->normalJumpZ; //-0xbaaaa;
			pPlayer->cantJump = 1;
		}

		if (pInput->actions & SB_CROUCH)
			pPlayer->posture = kPostureCrouch;
		break;
	}
	if (pInput->actions & SB_OPEN)
	{
		HitInfo result;

		int hit = ActionScan(pPlayer, &result);
		switch (hit)
		{
		case 6:
		{
			auto pSector = result.hitSector;
			auto pXSector = &pSector->xs();
			int key = pXSector->Key;
			if (pXSector->locked && pPlayer->pnum == myconnectindex)
			{
				viewSetMessage(GStrings.GetString("TXTB_LOCKED"));
				auto snd = 3062;
				if (sndCheckPlaying(snd))
					sndStopSample(snd);
				sndStartSample(snd, 255, 2, 0);
			}
			if (!key || pPlayer->hasKey[key])
				trTriggerSector(pSector, kCmdSpritePush);
			else if (pPlayer->pnum == myconnectindex)
			{
				viewSetMessage(GStrings.GetString("TXTB_KEY"));
				auto snd = 3063;
				if (sndCheckPlaying(snd))
					sndStopSample(snd);
				sndStartSample(snd, 255, 2, 0);
			}
			break;
		}
		case 0:
		{
			auto pWall = result.hitWall;
			auto pXWall = &pWall->xw();
			int key = pXWall->key;
			if (pXWall->locked && pPlayer->pnum == myconnectindex)
			{
				viewSetMessage(GStrings.GetString("TXTB_LOCKED"));
				auto snd = 3062;
				if (sndCheckPlaying(snd))
					sndStopSample(snd);
				sndStartSample(snd, 255, 2, 0);
			}
			if (!key || pPlayer->hasKey[key])
				trTriggerWall(pWall, kCmdWallPush);
			else if (pPlayer->pnum == myconnectindex)
			{
				viewSetMessage(GStrings.GetString("TXTB_KEY"));
				auto snd = 3063;
				if (sndCheckPlaying(snd))
					sndStopSample(snd);
				sndStartSample(snd, 255, 2, 0);
			}
			break;
		}
		case 3:
		{
			auto act = result.actor();
			int key = act->xspr.key;
			if (actor->xspr.locked && pPlayer->pnum == myconnectindex && act->xspr.lockMsg)
				trTextOver(act->xspr.lockMsg);
			if (!key || pPlayer->hasKey[key])
				trTriggerSprite(act, kCmdSpritePush);
			else if (pPlayer->pnum == myconnectindex)
			{
				viewSetMessage(GStrings.GetString("TXTB_KEY"));
				sndStartSample(3063, 255, 2, 0);
			}
			break;
		}
		}
		if (pPlayer->handTime > 0)
			pPlayer->handTime = ClipLow(pPlayer->handTime - 4 * (6 - gGameOptions.nDifficulty), 0);
		if (pPlayer->handTime <= 0 && pPlayer->hand)
		{
			DBloodActor* pactor = pPlayer->GetActor();
			auto spawned = actSpawnDude(pactor, kDudeHand, pPlayer->GetActor()->clipdist * 0.5);
			if (spawned)
			{
				spawned->spr.Angles.Yaw += DAngle180;
				spawned->vel = DVector3(
					pPlayer->GetActor()->vel.XY() + (64. / 3.) * pPlayer->GetActor()->spr.Angles.Yaw.ToVector(),
					pPlayer->GetActor()->vel.Z
				);
			}
			pPlayer->hand = 0;
		}
		pInput->actions &= ~SB_OPEN;
	}

	pPlayer->doViewPitch();
	pPlayer->doPitchInput();

	pPlayer->slope = pPlayer->GetActor()->spr.Angles.Pitch.Tan();
	if (pInput->actions & SB_INVPREV)
	{
		pInput->actions &= ~SB_INVPREV;
		packPrevItem(pPlayer);
	}
	if (pInput->actions & SB_INVNEXT)
	{
		pInput->actions &= ~SB_INVNEXT;
		packNextItem(pPlayer);
	}
	if (pInput->actions & SB_INVUSE)
	{
		pInput->actions &= ~SB_INVUSE;
		if (pPlayer->packSlots[pPlayer->packItemId].curAmount > 0)
			packUseItem(pPlayer, pPlayer->packItemId);
	}
	if (pInput->isItemUsed(Item_BeastVision))
	{
		pInput->clearItemUsed(Item_BeastVision);
		if (pPlayer->packSlots[3].curAmount > 0)
			packUseItem(pPlayer, 3);
	}
	if (pInput->isItemUsed(Item_CrystalBall))
	{
		pInput->clearItemUsed(Item_CrystalBall);
		if (pPlayer->packSlots[2].curAmount > 0)
			packUseItem(pPlayer, 2);
	}
	if (pInput->isItemUsed(Item_JumpBoots))
	{
		pInput->clearItemUsed(Item_JumpBoots);
		if (pPlayer->packSlots[4].curAmount > 0)
			packUseItem(pPlayer, 4);
	}
	if (pInput->isItemUsed(Item_MedKit))
	{
		pInput->clearItemUsed(Item_MedKit);
		if (pPlayer->packSlots[0].curAmount > 0)
			packUseItem(pPlayer, 0);
	}
	if (pInput->actions & SB_HOLSTER)
	{
		pInput->actions &= ~SB_HOLSTER;
		if (pPlayer->curWeapon)
		{
			WeaponLower(pPlayer);
			viewSetMessage("Holstering weapon");
		}
	}
	CheckPickUp(pPlayer);
}

void playerProcess(DBloodPlayer* pPlayer)
{
	DBloodActor* actor = pPlayer->GetActor();
	POSTURE* pPosture = &pPlayer->pPosture[pPlayer->lifeMode][pPlayer->posture];
	powerupProcess(pPlayer);
	double top, bottom;
	GetActorExtents(actor, &top, &bottom);
	double dzflor = (bottom - actor->spr.pos.Z) / 4;
	double dzceil = (actor->spr.pos.Z - top) / 4;

	if (!gNoClip)
	{
		auto pSector = actor->sector();
		if (pushmove(actor->spr.pos, &pSector, actor->clipdist, dzceil, dzflor, CLIPMASK0) == -1)
			actDamageSprite(actor, actor, kDamageFall, 500 << 4);
		if (actor->sector() != pSector)
		{
			if (pSector == nullptr)
			{
				pSector = actor->sector();
				actDamageSprite(actor, actor, kDamageFall, 500 << 4);
			}
			else
				ChangeActorSect(actor, pSector);
		}
	}
	ProcessInput(pPlayer);
	pPlayer->zViewVel = interpolatedvalue(pPlayer->zViewVel, actor->vel.Z, FixedToFloat(0x7000));
	double dz = pPlayer->GetActor()->spr.pos.Z - pPosture->eyeAboveZ - pPlayer->zView;
	if (dz > 0)
		pPlayer->zViewVel += dz * FixedToFloat(0xa000);
	else
		pPlayer->zViewVel += dz * FixedToFloat(0x1800);
	pPlayer->zView += pPlayer->zViewVel;
	pPlayer->zWeaponVel = interpolatedvalue(pPlayer->zWeaponVel, actor->vel.Z, FixedToFloat(0x5000));
	dz = pPlayer->GetActor()->spr.pos.Z - pPosture->weaponAboveZ - pPlayer->zWeapon;
	if (dz > 0)
		pPlayer->zWeaponVel += dz * FixedToFloat(0x8000);
	else
		pPlayer->zWeaponVel += dz * FixedToFloat(0xc00);
	pPlayer->zWeapon += pPlayer->zWeaponVel;
	pPlayer->bobPhase = max(pPlayer->bobPhase - 4, 0.);
	double nSpeed = actor->vel.XY().Length();
	if (pPlayer->posture == 1)
	{
		pPlayer->bobAmp = (pPlayer->bobAmp + 17) & 2047;
		pPlayer->swayAmp = (pPlayer->swayAmp + 17) & 2047;
		pPlayer->bobHeight = pPosture->bobV * 10 * BobVal(pPlayer->bobAmp * 2);
		pPlayer->bobWidth = pPosture->bobH * pPlayer->bobPhase * BobVal(pPlayer->bobAmp - 256);
		pPlayer->swayHeight = pPosture->swayV * pPlayer->bobPhase * BobVal(pPlayer->swayAmp * 2);
		pPlayer->swayWidth = pPosture->swayH * pPlayer->bobPhase * BobVal(pPlayer->swayAmp - 0x155);
	}
	else
	{
		if (actor->xspr.height < 256)
		{
			bool running = pPlayer->isRunning && !cl_bloodvanillabobbing;
			pPlayer->bobAmp = (pPlayer->bobAmp + pPosture->pace[running] * 4) & 2047;
			pPlayer->swayAmp = (pPlayer->swayAmp + (pPosture->pace[running] * 4) / 2) & 2047;
			if (running)
			{
				if (pPlayer->bobPhase < 60)
					pPlayer->bobPhase = min(pPlayer->bobPhase + nSpeed, 60.);
			}
			else
			{
				if (pPlayer->bobPhase < 30)
					pPlayer->bobPhase = min(pPlayer->bobPhase + nSpeed, 30.);
			}
		}
		pPlayer->bobHeight = pPosture->bobV * pPlayer->bobPhase * BobVal(pPlayer->bobAmp * 2);
		pPlayer->bobWidth = pPosture->bobH * pPlayer->bobPhase * BobVal(pPlayer->bobAmp - 256);
		pPlayer->swayHeight = pPosture->swayV * pPlayer->bobPhase * BobVal(pPlayer->swayAmp * 2);
		pPlayer->swayWidth = pPosture->swayH * pPlayer->bobPhase * BobVal(pPlayer->swayAmp - 0x155);
	}
	pPlayer->flickerEffect = 0;
	pPlayer->quakeEffect = ClipLow(pPlayer->quakeEffect - 4, 0);
	pPlayer->tiltEffect = ClipLow(pPlayer->tiltEffect - 4, 0);
	pPlayer->visibility = ClipLow(pPlayer->visibility - 4, 0);
	pPlayer->painEffect = ClipLow(pPlayer->painEffect - 4, 0);
	pPlayer->blindEffect = ClipLow(pPlayer->blindEffect - 4, 0);
	pPlayer->pickupEffect = ClipLow(pPlayer->pickupEffect - 4, 0);
	if (pPlayer->pnum == myconnectindex && pPlayer->GetActor()->xspr.health == 0)
		pPlayer->hand = 0;
	if (!actor->xspr.health)
		return;
	pPlayer->isUnderwater = 0;
	if (pPlayer->posture == 1)
	{
		pPlayer->isUnderwater = 1;
		auto link = actor->sector()->lowerLink;
		if (link && (link->spr.type == kMarkerLowGoo || link->spr.type == kMarkerLowWater))
		{
			if (getceilzofslopeptr(actor->sector(), actor->spr.pos) > pPlayer->zView)
				pPlayer->isUnderwater = 0;
		}
	}
	if (!pPlayer->isUnderwater)
	{
		pPlayer->underwaterTime = 1200;
		pPlayer->chokeEffect = 0;
		if (packItemActive(pPlayer, 1))
			packUseItem(pPlayer, 1);
	}
	int nType = kDudePlayer1 - kDudeBase;
	switch (pPlayer->posture)
	{
	case 1:
		seqSpawn(dudeInfo[nType].seqStartID + 9, actor, -1);
		break;
	case 2:
		seqSpawn(dudeInfo[nType].seqStartID + 10, actor, -1);
		break;
	default:
		if (!nSpeed)
			seqSpawn(dudeInfo[nType].seqStartID, actor, -1);
		else
			seqSpawn(dudeInfo[nType].seqStartID + 8, actor, -1);
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DBloodActor* playerFireMissile(DBloodPlayer* pPlayer, double xyoff, const DVector3& dv, int nType)
{
	return actFireMissile(pPlayer->GetActor(), xyoff, pPlayer->zWeapon - pPlayer->GetActor()->spr.pos.Z, dv, nType);
}

DBloodActor* playerFireThing(DBloodPlayer* pPlayer, double xyoff, double zvel, int thingType, double nSpeed)
{
	return actFireThing(pPlayer->GetActor(), xyoff, pPlayer->zWeapon - pPlayer->GetActor()->spr.pos.Z, pPlayer->slope * 0.25 + zvel, thingType, nSpeed);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void playerFrag(DBloodPlayer* pKiller, DBloodPlayer* pVictim)
{
	assert(pKiller != NULL);
	assert(pVictim != NULL);

	char buffer[128] = "";
	int nKiller = pKiller->GetActor()->spr.type - kDudePlayer1;
	assert(nKiller >= 0 && nKiller < kMaxPlayers);
	int nVictim = pVictim->GetActor()->spr.type - kDudePlayer1;
	assert(nVictim >= 0 && nVictim < kMaxPlayers);
	if (nKiller == nVictim)
	{
		pVictim->fragger = nullptr;
		if (VanillaMode() || gGameOptions.nGameType != 1)
		{
			pVictim->fragCount--;
			pVictim->fragInfo[nVictim]--;
		}
		if (gGameOptions.nGameType == 3)
			team_score[pVictim->teamId]--;
		int nMessage = Random(5);
		int nSound = gSuicide[nMessage].Kills;
		if (pVictim->pnum == myconnectindex && pVictim->handTime <= 0)
		{
			strcpy(buffer, GStrings.GetString("TXTB_KILLSELF"));
			if (gGameOptions.nGameType > 0 && nSound >= 0)
				sndStartSample(nSound, 255, 2, 0);
		}
		else
		{
			snprintf(buffer, sizeof(buffer), gSuicide[nMessage].message, PlayerName(nVictim));
		}
	}
	else
	{
		if (VanillaMode() || gGameOptions.nGameType != 1)
		{
			pKiller->fragCount++;
			pKiller->fragInfo[nKiller]++;
		}
		if (gGameOptions.nGameType == 3)
		{
			if (pKiller->teamId == pVictim->teamId)
				team_score[pKiller->teamId]--;
			else
			{
				team_score[pKiller->teamId]++;
				team_ticker[pKiller->teamId] += 120;
			}
		}
		int nMessage = Random(25);
		int nSound = gVictory[nMessage].Kills;
		const char* pzMessage = gVictory[nMessage].message;
		snprintf(buffer, sizeof(buffer), pzMessage, PlayerName(nKiller), PlayerName(nVictim));
		if (gGameOptions.nGameType > 0 && nSound >= 0 && pKiller->pnum == myconnectindex)
			sndStartSample(nSound, 255, 2, 0);
	}
	viewSetMessage(buffer);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void FragPlayer(DBloodPlayer* pPlayer, DBloodActor* killer)
{
	if (killer && killer->IsPlayerActor())
	{
		DBloodPlayer* pKiller = getPlayer(killer->spr.type - kDudePlayer1);
		playerFrag(pKiller, pPlayer);
		int nTeam1 = pKiller->teamId & 1;
		int nTeam2 = pPlayer->teamId & 1;
		if (nTeam1 == 0)
		{
			if (nTeam1 != nTeam2)
				evSendGame(15, kCmdToggle);
			else
				evSendGame(16, kCmdToggle);
		}
		else
		{
			if (nTeam1 == nTeam2)
				evSendGame(16, kCmdToggle);
			else
				evSendGame(15, kCmdToggle);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int playerDamageArmor(DBloodPlayer* pPlayer, DAMAGE_TYPE nType, int nDamage)
{
	DAMAGEINFO* pDamageInfo = &damageInfo[nType];
	int nArmorType = pDamageInfo->armorType;
	if (nArmorType >= 0 && pPlayer->armor[nArmorType])
	{
#if 0
		int vbp = (nDamage * 7) / 8 - nDamage / 4;
		int v8 = pPlayer->at33e[nArmorType];
		int t = nDamage / 4 + vbp * v8 / 3200;
		v8 -= t;
#endif
		int v8 = pPlayer->armor[nArmorType];
		int t = scale(v8, 0, 3200, nDamage / 4, (nDamage * 7) / 8);
		v8 -= t;
		nDamage -= t;
		pPlayer->armor[nArmorType] = ClipLow(v8, 0);
	}
	return nDamage;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void flagDropped(DBloodPlayer* pPlayer, int a2)
{
	DBloodActor* playeractor = pPlayer->GetActor();
	DBloodActor* actor;
	char buffer[80];
	switch (a2)
	{
	case kItemFlagA:
		pPlayer->hasFlag &= ~1;
		actor = actDropObject(playeractor, kItemFlagA);
		if (actor) actor->SetOwner(pPlayer->ctfFlagState[0]);
		gBlueFlagDropped = true;
		snprintf(buffer, sizeof(buffer), "%s dropped Blue Flag", PlayerName(pPlayer->pnum));
		sndStartSample(8005, 255, 2, 0);
		viewSetMessage(buffer);
		break;
	case kItemFlagB:
		pPlayer->hasFlag &= ~2;
		actor = actDropObject(playeractor, kItemFlagB);
		if (actor) actor->SetOwner(pPlayer->ctfFlagState[1]);
		gRedFlagDropped = true;
		snprintf(buffer, sizeof(buffer), "%s dropped Red Flag", PlayerName(pPlayer->pnum));
		sndStartSample(8004, 255, 2, 0);
		viewSetMessage(buffer);
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int playerDamageSprite(DBloodActor* source, DBloodPlayer* pPlayer, DAMAGE_TYPE nDamageType, int nDamage)
{
	assert(pPlayer != NULL);
	if (pPlayer->damageControl[nDamageType] || pPlayer->godMode)
		return 0;
	nDamage = playerDamageArmor(pPlayer, nDamageType, nDamage);
	pPlayer->painEffect = ClipHigh(pPlayer->painEffect + (nDamage >> 3), 600);

	DBloodActor* pActor = pPlayer->GetActor();
	DUDEINFO* pDudeInfo = getDudeInfo(pActor->spr.type);
	int nDeathSeqID = -1;
	int nKneelingPlayer = -1;
	bool va = playerSeqPlaying(pPlayer, 16);
	if (!pActor->xspr.health)
	{
		if (va)
		{
			switch (nDamageType)
			{
			case kDamageSpirit:
				nDeathSeqID = 18;
				sfxPlay3DSound(pPlayer->GetActor(), 716, 0, 0);
				break;
			case kDamageExplode:
				GibSprite(pActor, GIBTYPE_7, NULL, NULL);
				GibSprite(pActor, GIBTYPE_15, NULL, NULL);
				pPlayer->GetActor()->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
				nDeathSeqID = 17;
				break;
			default:
			{
				double top, bottom;
				GetActorExtents(pActor, &top, &bottom);
				DVector3 gibPos(pActor->spr.pos.XY(), top);
				DVector3 gibVel(pActor->vel.XY() * 0.5, -FixedToFloat(0xccccc));
				GibSprite(pActor, GIBTYPE_27, &gibPos, &gibVel);
				GibSprite(pActor, GIBTYPE_7, NULL, NULL);
				fxSpawnBlood(pActor, nDamage << 4);
				fxSpawnBlood(pActor, nDamage << 4);
				nDeathSeqID = 17;
				break;
			}
			}
		}
	}
	else
	{
		int nHealth = pActor->xspr.health - nDamage;
		pActor->xspr.health = ClipLow(nHealth, 0);
		if (pActor->xspr.health > 0 && pActor->xspr.health < 16)
		{
			nDamageType = kDamageBullet;
			pActor->xspr.health = 0;
			nHealth = -25;
		}
		if (pActor->xspr.health > 0)
		{
			DAMAGEINFO* pDamageInfo = &damageInfo[nDamageType];
			int nSound;
			if (nDamage >= (10 << 4))
				nSound = pDamageInfo->Kills[0];
			else
				nSound = pDamageInfo->Kills[Random(3)];
			if (nDamageType == kDamageDrown && pActor->xspr.medium == kMediumWater && !pPlayer->hand)
				nSound = 714;
			sfxPlay3DSound(pPlayer->GetActor(), nSound, 0, 6);
			return nDamage;
		}
		sfxKill3DSound(pPlayer->GetActor(), -1, 441);
		if (gGameOptions.nGameType == 3 && pPlayer->hasFlag) {
			if (pPlayer->hasFlag & 1) flagDropped(pPlayer, kItemFlagA);
			if (pPlayer->hasFlag & 2) flagDropped(pPlayer, kItemFlagB);
		}
		pPlayer->deathTime = 0;
		pPlayer->qavLoop = 0;
		pPlayer->curWeapon = kWeapNone;
		pPlayer->fragger = source;
		pPlayer->voodooTargets = 0;
		if (nDamageType == kDamageExplode && nDamage < (9 << 4))
			nDamageType = kDamageFall;
		switch (nDamageType)
		{
		case kDamageExplode:
			sfxPlay3DSound(pPlayer->GetActor(), 717, 0, 0);
			GibSprite(pActor, GIBTYPE_7, NULL, NULL);
			GibSprite(pActor, GIBTYPE_15, NULL, NULL);
			pPlayer->GetActor()->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
			nDeathSeqID = 2;
			break;
		case kDamageBurn:
			sfxPlay3DSound(pPlayer->GetActor(), 718, 0, 0);
			nDeathSeqID = 3;
			break;
		case kDamageDrown:
			nDeathSeqID = 1;
			break;
		default:
			if (nHealth < -20 && gGameOptions.nGameType >= 2 && Chance(0x4000))
			{
				DAMAGEINFO* pDamageInfo = &damageInfo[nDamageType];
				sfxPlay3DSound(pPlayer->GetActor(), pDamageInfo->at10[0], 0, 2);
				nDeathSeqID = 16;
				nKneelingPlayer = nPlayerKneelClient;
				powerupActivate(pPlayer, kPwUpDeliriumShroom);
				pActor->SetTarget(source);
				evPostActor(pPlayer->GetActor(), 15, kCallbackFinishHim);
			}
			else
			{
				sfxPlay3DSound(pPlayer->GetActor(), 716, 0, 0);
				nDeathSeqID = 1;
			}
			break;
		}
	}
	if (nDeathSeqID < 0)
		return nDamage;
	if (nDeathSeqID != 16)
	{
		powerupClear(pPlayer);
		if (pActor->sector()->hasX() && pActor->sector()->xs().Exit)
			trTriggerSector(pActor->sector(), kCmdSectorExit);
		pActor->spr.flags |= 7;
		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			if (getPlayer(p)->fragger == pPlayer->GetActor() && getPlayer(p)->deathTime > 0)
				getPlayer(p)->fragger = nullptr;
		}
		FragPlayer(pPlayer, source);
		trTriggerSprite(pActor, kCmdOff);

#ifdef NOONE_EXTENSIONS
		// allow drop items and keys in multiplayer
		if (gModernMap && gGameOptions.nGameType != kGameTypeSinglePlayer && pPlayer->GetActor()->xspr.health <= 0) {

			DBloodActor* pItem = nullptr;
			if (pPlayer->GetActor()->xspr.dropMsg && (pItem = actDropItem(pActor, pPlayer->GetActor()->xspr.dropMsg)) != NULL)
				evPostActor(pItem, 500, kCallbackRemove);

			if (pPlayer->GetActor()->xspr.key) {

				int i; // if all players have this key, don't drop it
				for (i = connecthead; i >= 0; i = connectpoint2[i]) {
					if (!getPlayer(i)->hasKey[pPlayer->GetActor()->xspr.key])
						break;
				}

				if (i == 0 && (pItem = actDropKey(pActor, (pPlayer->GetActor()->xspr.key + kItemKeyBase) - 1)) != NULL)
					evPostActor(pItem, 500, kCallbackRemove);

			}


		}
#endif

	}
	assert(getSequence(pDudeInfo->seqStartID + nDeathSeqID) != NULL);
	seqSpawn(pDudeInfo->seqStartID + nDeathSeqID, pPlayer->GetActor(), nKneelingPlayer);
	return nDamage;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int UseAmmo(DBloodPlayer* pPlayer, int nAmmoType, int nDec)
{
	if (gInfiniteAmmo)
		return 9999;
	if (nAmmoType == -1)
		return 9999;
	pPlayer->ammoCount[nAmmoType] = ClipLow(pPlayer->ammoCount[nAmmoType] - nDec, 0);
	return pPlayer->ammoCount[nAmmoType];
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void voodooTarget(DBloodPlayer* pPlayer)
{
	DBloodActor* actor = pPlayer->GetActor();
	double aimz = pPlayer->aim.Z;
	double dz = pPlayer->zWeapon - pPlayer->GetActor()->spr.pos.Z;
	if (UseAmmo(pPlayer, 9, 0) < 8)
	{
		pPlayer->voodooTargets = 0;
		return;
	}
	for (int i = 0; i < 4; i++)
	{
		// both voodooVar's are always 0. This is only kept in case someone implements an extension.
		DAngle ang1 = DAngle::fromBuild(pPlayer->voodooVar1 + pPlayer->vodooVar2);
		actFireVector(actor, 0, dz, DVector3(ang1.ToVector(), aimz), kVectorVoodoo10);
		DAngle ang2 = DAngle::fromBuild(pPlayer->voodooVar1 - pPlayer->vodooVar2);
		actFireVector(actor, 0, dz, DVector3(ang1.ToVector(), aimz), kVectorVoodoo10);
	}
	pPlayer->voodooTargets = ClipLow(pPlayer->voodooTargets - 1, 0);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void playerLandingSound(DBloodPlayer* pPlayer)
{
	static int surfaceSound[] = {
		-1,
		600,
		601,
		602,
		603,
		604,
		605,
		605,
		605,
		600,
		605,
		605,
		605,
		604,
		603
	};
	SPRITEHIT* pHit = &pPlayer->GetActor()->hit;
	if (pHit->florhit.type != kHitNone)
	{
		if (!gGameOptions.bFriendlyFire && pHit->florhit.type == kHitSprite && IsTargetTeammate(pPlayer, pHit->florhit.actor()))
			return;
		int nSurf = tileGetSurfType(pHit->florhit);
		if (nSurf)
			sfxPlay3DSound(pPlayer->GetActor(), surfaceSound[nSurf], -1, 0);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PlayerSurvive(int, DBloodActor* actor)
{
	char buffer[80];
	actHealDude(actor, 1, 2);
	if (gGameOptions.nGameType > 0 && numplayers > 1)
	{
		sfxPlay3DSound(actor, 3009, 0, 6);
		if (actor->IsPlayerActor())
		{
			DBloodPlayer* pPlayer = getPlayer(actor->spr.type - kDudePlayer1);
			if (pPlayer->pnum == myconnectindex)
				viewSetMessage(GStrings.GetString("TXT_LIVEAGAIM"));
			else
			{
				snprintf(buffer, sizeof(buffer), "%s lives again!", PlayerName(pPlayer->pnum));
				viewSetMessage(buffer);
			}
			pPlayer->newWeapon = kWeapPitchFork;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PlayerKneelsOver(int, DBloodActor* actor)
{
	for (int p = connecthead; p >= 0; p = connectpoint2[p])
	{
		if (getPlayer(p)->GetActor() == actor)
		{
			DBloodPlayer* pPlayer = getPlayer(p);
			playerDamageSprite(pPlayer->fragger, pPlayer, kDamageSpirit, 500 << 4);
			return;
		}
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, PACKINFO& w, PACKINFO* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("isactive", w.isActive)
			("curamount", w.curAmount)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, POSTURE& w, POSTURE* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("frontaccel", w.frontAccel, def->frontAccel)
			("sideaccel", w.sideAccel, def->sideAccel)
			("backaccel", w.backAccel, def->backAccel)
			("pace0", w.pace[0], def->pace[0])
			("pace1", w.pace[1], def->pace[1])
			("bobv", w.bobV, def->bobV)
			("bobh", w.bobH, def->bobH)
			("swayv", w.swayV, def->swayV)
			("swayh", w.swayH, def->swayH)
			("eyeabovez", w.eyeAboveZ, def->eyeAboveZ)
			("weaponabovez", w.weaponAboveZ, def->weaponAboveZ)
			("xoffset", w.xOffset, def->xOffset)
			("zoffset", w.zOffset, def->zOffset)
			("normaljumpz", w.normalJumpZ, def->normalJumpZ)
			("pwupjumpz", w.pwupJumpZ, def->pwupJumpZ)
			.EndObject();
	}
	return arc;
}

void DBloodPlayer::Serialize(FSerializer& arc)
{
	Super::Serialize(arc);
	arc("newweapon", newWeapon)
		("weaponqav", weaponQav)
		("qavcallback", qavCallback)
		("isrunning", isRunning)
		("posture", posture)
		("sceneqav", sceneQav)
		("bobphase", bobPhase)
		("bobamp", bobAmp)
		("bobheight", bobHeight)
		("bobwidth", bobWidth)
		("swayamp", swayAmp)
		("swayheight", swayHeight)
		("swaywidth", swayWidth)
		("lifemode", lifeMode)
		("zview", zView)
		("zviewvel", zViewVel)
		("zweapon", zWeapon)
		("zweaponvel", zWeaponVel)
		("slope", slope)
		("underwater", isUnderwater)
		.Array("haskey", hasKey, 8)
		("hasflag", hasFlag)
		.Array("ctfflagstate", ctfFlagState, 2)
		.Array("dmgcontrol", damageControl, 7)
		("curweapon", curWeapon)
		("nextweapon", nextWeapon)
		("weapontimer", weaponTimer)
		("weaponstate", weaponState)
		("weaponammo", weaponAmmo)
		.Array("hasweapon", hasWeapon, countof(hasWeapon))
		.Array("weaponmode", weaponMode, countof(weaponMode))
		.Array("weaponorder", &weaponOrder[0][kWeapNone], +kWeapMax * 2)
		.Array("ammocount", ammoCount, countof(ammoCount))
		("qavloop", qavLoop)
		("qavlastTick", qavLastTick)
		("qavtimer", qavTimer)
		("fusetime", fuseTime)
		("throwtime", throwTime)
		("throwpower", throwPower)
		("aim", aim)
		("relaim", relAim)
		("aimtarget", aimTarget)
		("aimtargetscount", aimTargetsCount)
		.Array("aimtargets", aimTargets, countof(aimTargets))
		("deathtime", deathTime)
		.Array("pwuptime", pwUpTime, countof(pwUpTime))
		("fragcount", fragCount)
		.Array("fraginfo", fragInfo, countof(fragInfo))
		("teamid", teamId)
		("fraggerid", fragger)
		("undserwatertime", underwaterTime)
		("bubbletime", bubbleTime)
		("resttime", restTime)
		("kickpower", kickPower)
		("laughcount", laughCount)
		("godmode", godMode)
		("fallscream", fallScream)
		("cantjump", cantJump)
		("packitemtime", packItemTime)
		("packitemid", packItemId)
		.Array("packslots", packSlots, countof(packSlots))
		.Array("armor", armor, countof(armor))
		("voodootarget", voodooTarget)
		("voodootargets", voodooTargets)
		("voodoovar1", voodooVar1)
		("voodoovar2", vodooVar2)
		("flickereffect", flickerEffect)
		("tilteffect", tiltEffect)
		("visibility", visibility)
		("paineffect", painEffect)
		("blindeffect", blindEffect)
		("chokeeffect", chokeEffect)
		("handtime", handTime)
		("hand", hand)
		("pickupeffect", pickupEffect)
		("flasheffect", flashEffect)
		("quakeeffect", quakeEffect)
		("player_par", player_par)
		("waterpal", nWaterPal)
		.Array("posturedata", &pPosture[0][0], &gPostureDefaults[0][0], kModeMax * kPostureMax) // only save actual changes in this.
		;

	if (arc.isReading())
	{
		playerResetPosture(this);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

#ifdef NOONE_EXTENSIONS
FSerializer& Serialize(FSerializer& arc, const char* keyname, TRPLAYERCTRL& w, TRPLAYERCTRL* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("index", w.qavScene.initiator)
			("dummy", w.qavScene.dummy)
			.EndObject();
	}
	if (arc.isReading()) w.qavScene.qavResrc = nullptr;
	return arc;
}
#endif


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SerializePlayers(FSerializer& arc)
{
	if (arc.BeginObject("players"))
	{
		arc("numplayers", gNetPlayers)
			.Array("teamscore", team_score, gNetPlayers)
#ifdef NOONE_EXTENSIONS
			.Array("playerctrl", gPlayerCtrl, gNetPlayers)
#endif
			.EndObject();
	}

	if (arc.isReading())
	{
		for (int i = 0; i < gNetPlayers; i++)
		{
			getPlayer(i)->pDudeInfo = &dudeInfo[getPlayer(i)->GetActor()->spr.type - kDudeBase];

#ifdef NOONE_EXTENSIONS
			// load qav scene
			if (getPlayer(i)->sceneQav != -1)
			{
				QAV* pQav = playerQavSceneLoad(getPlayer(i)->sceneQav);
				if (pQav)
				{
					gPlayerCtrl[i].qavScene.qavResrc = pQav;
					//gPlayerCtrl[i].qavScene.qavResrc->Preload();
				}
				else
				{
					getPlayer(i)->sceneQav = -1;
				}
			}
#endif
		}
	}
}



DEFINE_FIELD(DBloodPlayer, actor)
DEFINE_FIELD(DBloodPlayer, newWeapon)
DEFINE_FIELD(DBloodPlayer, weaponQav)
DEFINE_FIELD(DBloodPlayer, qavCallback)
DEFINE_FIELD(DBloodPlayer, isRunning)
DEFINE_FIELD(DBloodPlayer, posture)   // stand, crouch, swim
DEFINE_FIELD(DBloodPlayer, sceneQav)  // by NoOne: used to keep qav id
DEFINE_FIELD(DBloodPlayer, bobPhase)
DEFINE_FIELD(DBloodPlayer, bobAmp)
DEFINE_FIELD(DBloodPlayer, bobHeight)
DEFINE_FIELD(DBloodPlayer, bobWidth)
DEFINE_FIELD(DBloodPlayer, swayAmp)
DEFINE_FIELD(DBloodPlayer, swayHeight)
DEFINE_FIELD(DBloodPlayer, swayWidth)
DEFINE_FIELD(DBloodPlayer, pnum)  // Connect id
DEFINE_FIELD(DBloodPlayer, lifeMode)
DEFINE_FIELD(DBloodPlayer, zView)
DEFINE_FIELD(DBloodPlayer, zViewVel)
DEFINE_FIELD(DBloodPlayer, zWeapon)
DEFINE_FIELD(DBloodPlayer, zWeaponVel)
DEFINE_FIELD(DBloodPlayer, slope)
DEFINE_FIELD(DBloodPlayer, isUnderwater)
DEFINE_FIELD(DBloodPlayer, hasKey)
DEFINE_FIELD(DBloodPlayer, hasFlag)
DEFINE_FIELD(DBloodPlayer, damageControl)
DEFINE_FIELD(DBloodPlayer, curWeapon)
DEFINE_FIELD(DBloodPlayer, nextWeapon)
DEFINE_FIELD(DBloodPlayer, weaponTimer)
DEFINE_FIELD(DBloodPlayer, weaponState)
DEFINE_FIELD(DBloodPlayer, weaponAmmo)  //rename
DEFINE_FIELD(DBloodPlayer, hasWeapon)
DEFINE_FIELD(DBloodPlayer, weaponMode)
DEFINE_FIELD(DBloodPlayer, weaponOrder)
DEFINE_FIELD(DBloodPlayer, ammoCount)
DEFINE_FIELD(DBloodPlayer, qavLoop)
DEFINE_FIELD(DBloodPlayer, fuseTime)
DEFINE_FIELD(DBloodPlayer, throwTime)
DEFINE_FIELD(DBloodPlayer, throwPower)
//DEFINE_FIELD(DBloodPlayer, aim)  // world
DEFINE_FIELD(DBloodPlayer, aimTargetsCount)
//DEFINE_FIELD(DBloodPlayer, aimTargets)
DEFINE_FIELD(DBloodPlayer, deathTime)
DEFINE_FIELD(DBloodPlayer, pwUpTime)
DEFINE_FIELD(DBloodPlayer, teamId)
DEFINE_FIELD(DBloodPlayer, fragCount)
DEFINE_FIELD(DBloodPlayer, fragInfo)
DEFINE_FIELD(DBloodPlayer, underwaterTime)
DEFINE_FIELD(DBloodPlayer, bubbleTime)
DEFINE_FIELD(DBloodPlayer, restTime)
DEFINE_FIELD(DBloodPlayer, kickPower)
DEFINE_FIELD(DBloodPlayer, laughCount)
DEFINE_FIELD(DBloodPlayer, godMode)
DEFINE_FIELD(DBloodPlayer, fallScream)
DEFINE_FIELD(DBloodPlayer, cantJump)
DEFINE_FIELD(DBloodPlayer, packItemTime)  // pack timer
DEFINE_FIELD(DBloodPlayer, packItemId)    // pack id 1: diving suit, 2: crystal ball, 3:
DEFINE_FIELD(DBloodPlayer, packSlots)  // at325 1]: diving suit, [2]: crystal ball, 
DEFINE_FIELD(DBloodPlayer, armor)      // armor
//DEFINE_FIELD(DBloodPlayer, voodooTarget)
DEFINE_FIELD(DBloodPlayer, flickerEffect)
DEFINE_FIELD(DBloodPlayer, tiltEffect)
DEFINE_FIELD(DBloodPlayer, visibility)
DEFINE_FIELD(DBloodPlayer, painEffect)
DEFINE_FIELD(DBloodPlayer, blindEffect)
DEFINE_FIELD(DBloodPlayer, chokeEffect)
DEFINE_FIELD(DBloodPlayer, handTime)
DEFINE_FIELD(DBloodPlayer, hand)  // if true, there is hand start choking the player
DEFINE_FIELD(DBloodPlayer, pickupEffect)
DEFINE_FIELD(DBloodPlayer, flashEffect)  // if true, reduce pPlayer->visibility counter
DEFINE_FIELD(DBloodPlayer, quakeEffect)
DEFINE_FIELD(DBloodPlayer, player_par)
DEFINE_FIELD(DBloodPlayer, nWaterPal)

END_BLD_NS
