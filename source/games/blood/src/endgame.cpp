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

#include "build.h"
#include "v_draw.h"
#include "mmulti.h"
#include "statistics.h"
#include "gstrings.h"
#include "gamestate.h"
#include "raze_sound.h"
#include "d_net.h"
#include "screenjob.h"
#include "blood.h"

BEGIN_BLD_NS


void GameInterface::LevelCompleted(MapRecord *map, int skill)
{
	EndLevel();
	Mus_Stop();

	SummaryInfo info{};

	info.kills = gKillMgr.Kills;
	info.maxkills = gKillMgr.TotalKills;
	info.secrets = gSecretMgr.Founds;
	info.maxsecrets = gSecretMgr.Total;
	info.time = gSecretMgr.Super;
	info.endofgame = map == nullptr;

	ShowIntermission(currentLevel, map, &info, [=](bool)
		{
			soundEngine->StopAllChannels();
			gameaction = map? ga_nextlevel : ga_creditsmenu;
			if (!map) gGameOptions.uGameFlags &= ~(GF_AdvanceLevel | GF_EndGame);
		});
}


CKillMgr::CKillMgr()
{
	Clear();
}

void CKillMgr::SetCount(int nCount)
{
	TotalKills = nCount;
}

void CKillMgr::AddNewKill(int nCount)
{
	TotalKills += nCount;
}

void CKillMgr::AddKill(spritetype* pSprite)
{
	if (pSprite->statnum == kStatDude && pSprite->type != kDudeBat && pSprite->type != kDudeRat && pSprite->type != kDudeInnocent && pSprite->type != kDudeBurningInnocent)
		Kills++;
}

void CKillMgr::CountTotalKills(void)
{
	TotalKills = 0;
	int nSprite;
	StatIterator it(kStatDude);
	while ((nSprite = it.NextIndex()) >= 0)
	{
		spritetype* pSprite = &sprite[nSprite];
		if (pSprite->type < kDudeBase || pSprite->type >= kDudeMax)
			I_Error("Non-enemy sprite (%d) in the enemy sprite list.", nSprite);
		if (pSprite->statnum == kStatDude && pSprite->type != kDudeBat && pSprite->type != kDudeRat && pSprite->type != kDudeInnocent && pSprite->type != kDudeBurningInnocent)
			TotalKills++;
	}
}

void CKillMgr::Clear(void)
{
	TotalKills = Kills = 0;
}

CSecretMgr::CSecretMgr(void)
{
	Clear();
}

void CSecretMgr::SetCount(int nCount)
{
	Total = nCount;
}

void CSecretMgr::Found(int nType)
{
	if (nType == 0) Founds++;
	else if (nType < 0) {
		viewSetSystemMessage("Invalid secret type %d triggered.", nType);
		return;
	}
	else Super++;

	if (gGameOptions.nGameType == 0) {
		viewSetMessage(GStrings(FStringf("TXTB_SECRET%d", Random(2))), 0, MESSAGE_PRIORITY_SECRET);
	}
}

void CSecretMgr::Clear(void)
{
	Total = Founds = Super = 0;
}

void SerializeGameStats(FSerializer& arc)
{
	if (arc.BeginObject("gamestats"))
	{
		arc("secrets", gSecretMgr.Total)
			("secretsfound", gSecretMgr.Founds)
			("super", gSecretMgr.Super)
			("totalkills", gKillMgr.TotalKills)
			("kills", gKillMgr.Kills)
			.EndObject();
	}
}


CSecretMgr gSecretMgr;
CKillMgr gKillMgr;

END_BLD_NS
