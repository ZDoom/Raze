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
#include "common_game.h"
#include "blood.h"
#include "endgame.h"
#include "globals.h"
#include "levels.h"
#include "loadsave.h"
#include "player.h"
#include "sound.h"
#include "view.h"
#include "messages.h"
#include "statistics.h"
#include "gstrings.h"
#include "gamestate.h"
#include "raze_sound.h"
#include "d_net.h"
#include "screenjob.h"

BEGIN_BLD_NS

enum
{
	kLoadScreenCRC = -2051908571,
	kLoadScreenWideBackWidth = 256,
	kLoadScreenWideSideWidth = 128,

};

static int bLoadScreenCrcMatch = -1;

static void drawTextScreenBackground(void)
{
	if (bLoadScreenCrcMatch == -1) bLoadScreenCrcMatch = tileGetCRC32(kLoadScreen) == kLoadScreenCRC;

	if (bLoadScreenCrcMatch)
	{
		if (ActiveRatio(twod->GetWidth(), twod->GetHeight()) < 1.34f)
		{
			DrawTexture(twod, tileGetTexture(kLoadScreen), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, TAG_DONE);
		}
		else
		{
			int width = scale(xdim, 240, ydim);
			int nCount = (width + kLoadScreenWideBackWidth - 1) / kLoadScreenWideBackWidth;
			for (int i = 0; i < nCount; i++)
			{
				DrawTexture(twod, tileGetTexture(kLoadScreenWideBack), (i * kLoadScreenWideBackWidth), 0,
					DTA_VirtualWidth, width, DTA_VirtualHeight, 200, DTA_KeepRatio, true, TAG_DONE);
			}
			DrawTexture(twod, tileGetTexture(kLoadScreenWideLeft), 0, 0, DTA_VirtualWidth, width, DTA_VirtualHeight, 200, DTA_KeepRatio, true, DTA_TopLeft, true, TAG_DONE);
			DrawTexture(twod, tileGetTexture(kLoadScreenWideRight), width - tileWidth(kLoadScreenWideRight), 0, DTA_TopLeft, true,
				DTA_VirtualWidth, width, DTA_VirtualHeight, 200, DTA_KeepRatio, true, TAG_DONE);
			DrawTexture(twod, tileGetTexture(kLoadScreenWideMiddle), (width - tileWidth(kLoadScreenWideMiddle)) / 2, 0, DTA_TopLeft, true,
				DTA_VirtualWidth, width, DTA_VirtualHeight, 200, DTA_KeepRatio, true, TAG_DONE);
		}
	}
	else
	{
		DrawTexture(twod, tileGetTexture(kLoadScreen), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, TAG_DONE);
	}
}

// One these screens get scriptified this should use the version in BloodMenuDelegate.
static void DrawCaption(const char* text)
{
	double scalex = 1.; // Expand the box if the text is longer
	int width = BigFont->StringWidth(text);
	int boxwidth = tileWidth(2038);
	if (boxwidth - 10 < width) scalex = double(width) / (boxwidth - 10);

	DrawTexture(twod, tileGetTexture(2038, true), 160, 20, DTA_FullscreenScale, FSMode_Fit320x200Top, DTA_CenterOffsetRel, true, DTA_ScaleX, scalex, TAG_DONE);
	DrawText(twod, BigFont, CR_UNDEFINED, 160 - width / 2, 20 - tileHeight(4193) / 2, text, DTA_FullscreenScale, FSMode_Fit320x200Top, TAG_DONE);
}


class DBloodSummaryScreen : public DScreenJob
{
	void DrawKills(void)
	{
		char pBuffer[40];
		if (gGameOptions.nGameType == 0)
		{
			viewDrawText(1, FStringf("%s:", GStrings("KILLS")), 75, 50, -128, 0, 0, 1);
			mysnprintf(pBuffer, 40,"%2d", gKillMgr.Kills);
			viewDrawText(1, pBuffer, 160, 50, -128, 0, 0, 1);
			viewDrawText(1, GStrings("OF"), 190, 50, -128, 0, 0, 1);
			mysnprintf(pBuffer, 40, "%2d", gKillMgr.TotalKills);
			viewDrawText(1, pBuffer, 220, 50, -128, 0, 0, 1);
		}
		else
		{
			viewDrawText(3, "#", 85, 35, -128, 0, 0, 1);
			viewDrawText(3, GStrings("NAME"), 100, 35, -128, 0, 0, 1);
			viewDrawText(3, GStrings("FRAGS"), 210, 35, -128, 0, 0, 1);
			int nStart = 0;
			int nEnd = kMaxPlayers;

			for (int i = nStart; i < nEnd; i++) if (playeringame[i])
			{
				mysnprintf(pBuffer, 40, "%-2d", i);
				viewDrawText(3, pBuffer, 85, 50 + 8 * i, -128, 0, 0, 1);
				mysnprintf(pBuffer, 40, "%s", gProfile[i].name);
				viewDrawText(3, pBuffer, 100, 50 + 8 * i, -128, 0, 0, 1);
				mysnprintf(pBuffer, 40, "%d", gPlayer[i].fragCount);
				viewDrawText(3, pBuffer, 210, 50 + 8 * i, -128, 0, 0, 1);
			}
		}
	}

	void DrawSecrets(void)
	{
		char pBuffer[40];
		viewDrawText(1, FStringf("%s:", GStrings("TXT_SECRETS")), 75, 70, -128, 0, 0, 1);
		mysnprintf(pBuffer, 40, "%2d", gSecretMgr.Founds);
		viewDrawText(1, pBuffer, 160, 70, -128, 0, 0, 1);
		viewDrawText(1, GStrings("OF"), 190, 70, -128, 0, 0, 1);
		mysnprintf(pBuffer, 40, "%2d", gSecretMgr.Total);
		viewDrawText(1, pBuffer, 220, 70, -128, 0, 0, 1);
		if (gSecretMgr.Super > 0)
			viewDrawText(1, GStrings("TXT_SUPERSECRET"), 160, 100, -128, 2, 1, 1);
	}


	int Frame(uint64_t clock, bool skiprequest)
	{
		drawTextScreenBackground();
		if (gGameOptions.nGameType == 0)
		{
			DrawCaption(GStrings("TXTB_LEVELSTATS"));
			if (bPlayerCheated)
			{
				auto text = GStrings("TXTB_CHEATED");
				int font = 3;
				if (!SmallFont2->CanPrint(text)) font = 0;
				viewDrawText(font, text, 160, 32, -128, 0, 1, font == 3);
			}
			DrawKills();
			DrawSecrets();
		}
		else
		{
			DrawCaption(GStrings("TXTB_FRAGSTATS"));
			DrawKills();
		}
		int myclock = int(clock * 120 / 1'000'000'000);
		if ((myclock & 32))
		{
			auto text = GStrings("PRESSKEY");
			int font = 3;
			if (!SmallFont2->CanPrint(text)) font = 0;
			viewDrawText(font, text, 160, 134, -128, 0, 1, font == 3);
		}
		return skiprequest ? -1 : 1;
	}
};

void GameInterface::LevelCompleted(MapRecord *map, int skill)
{
	JobDesc job = { Create<DBloodSummaryScreen>() };
	sndStartSample(268, 128, -1, false, CHANF_UI);
	Mus_Stop();
	RunScreenJob(&job, 1, [=](bool)
		{
			soundEngine->StopAllChannels();
			gameaction = ga_nextlevel;
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

class DBloodLoadScreen : public DScreenJob
{
	const char* pzLoadingScreenText1;
	MapRecord* rec;

public:
	DBloodLoadScreen(const char* caption, MapRecord* maprec) : DScreenJob(), rec(maprec)
	{
		if (gGameOptions.nGameType == 0) pzLoadingScreenText1 = GStrings("TXTB_LLEVEL");
		else pzLoadingScreenText1 = GStrings(FStringf("TXTB_NETGT%d", gGameOptions.nGameType));
	}

	int Frame(uint64_t clock, bool skiprequest)
	{
		twod->ClearScreen();
		drawTextScreenBackground();
		DrawCaption(pzLoadingScreenText1);
		viewDrawText(1, rec->DisplayName(), 160, 50, -128, 0, 1, 1);

		auto text = GStrings("TXTB_PLSWAIT");
		int font = 3;
		if (!SmallFont2->CanPrint(text)) font = 0;

		viewDrawText(font, GStrings("TXTB_PLSWAIT"), 160, 134, -128, 0, 1, font == 3);
		return 0;
	}
};

void loadscreen(const char *caption, MapRecord* rec, CompletionFunc func)
{
	JobDesc job = { Create<DBloodLoadScreen>(caption, rec) };
	RunScreenJob(&job, 1, func);
}


END_BLD_NS
