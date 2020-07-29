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
#include "common.h"
#include "mmulti.h"
#include "common_game.h"
#include "blood.h"
#include "endgame.h"
#include "globals.h"
#include "levels.h"
#include "loadsave.h"
#include "network.h"
#include "player.h"
#include "sound.h"
#include "view.h"
#include "messages.h"
#include "statistics.h"
#include "gstrings.h"
#include "gamestate.h"
#include "raze_sound.h"

BEGIN_BLD_NS

CEndGameMgr::CEndGameMgr()
{
    at0 = 0;
}

void CEndGameMgr::Draw(void)
{
    viewLoadingScreenWide();
    int nHeight;
    viewGetFontInfo(1, NULL, NULL, &nHeight);
    rotatesprite(160<<16, 20<<16, 65536, 0, 2038, -128, 0, 6, 0, 0, xdim-1, ydim-1);
    int nY = 20 - nHeight / 2;
    if (gGameOptions.nGameType == 0)
    {
        viewDrawText(1, GStrings("TXTB_LEVELSTATS"), 160, nY, -128, 0, 1, 0);
        if (CCheatMgr::m_bPlayerCheated)
        {
            viewDrawText(3, GStrings("TXTB_CHEATED"), 160, 32, -128, 0, 1, 1);
        }
        gKillMgr.Draw();
        gSecretMgr.Draw();
    }
    else
    {
        viewDrawText(1, GStrings("TXTB_FRAGSTATS"), 160, nY, -128, 0, 1, 0);
        gKillMgr.Draw();
    }
    if (/*dword_28E3D4 != 1 && */((int)totalclock&32))
    {
        viewDrawText(3, GStrings("PRESSKEY"), 160, 134, -128, 0, 1, 1);
    }
}

void CEndGameMgr::ProcessKeys(void)
{
    //if (dword_28E3D4 == 1)
    //{
    //    if (gGameOptions.gameType >= 0 || numplayers > 1)
    //        netWaitForEveryone(0);
    //    Finish();
    //}
    //else
    {
        if (!inputState.CheckAllInput())
            return;
        if (gGameOptions.nGameType > 0 || numplayers > 1)
            netWaitForEveryone(0);
        Finish();
    }
}

extern void EndLevel(void);

void CEndGameMgr::Setup(void)
{
	gamestate = GS_FINALE;
    at0 = 1;
	STAT_Update(false);
    EndLevel();
    sndStartSample(268, 128, -1, 1);
    inputState.keyFlushScans();
}

//int gNextLevel;

extern int gInitialNetPlayers;
extern bool gStartNewGame;

void CEndGameMgr::Finish(void)
{
    levelSetupOptions(gGameOptions.nEpisode, gNextLevel);
    gInitialNetPlayers = numplayers;
    soundEngine->StopAllChannels();
    gStartNewGame = 1;
    at0 = 0;
}

CKillMgr::CKillMgr()
{
    Clear();
}

void CKillMgr::SetCount(int nCount)
{
    at0 = nCount;
}

void CKillMgr::sub_263E0(int nCount)
{
    at0 += nCount;
}

void CKillMgr::AddKill(spritetype* pSprite)
{
    if (pSprite->statnum == kStatDude && pSprite->type != kDudeBat && pSprite->type != kDudeRat && pSprite->type != kDudeInnocent && pSprite->type != kDudeBurningInnocent)
        at4++;
}

void CKillMgr::sub_2641C(void)
{
    at0 = 0;
    for (int nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        spritetype* pSprite = &sprite[nSprite];
        if (pSprite->type < kDudeBase || pSprite->type >= kDudeMax)
            ThrowError("Non-enemy sprite (%d) in the enemy sprite list.", nSprite);
        if (pSprite->statnum == kStatDude && pSprite->type != kDudeBat && pSprite->type != kDudeRat && pSprite->type != kDudeInnocent && pSprite->type != kDudeBurningInnocent)
            at0++;
    }
}

void CKillMgr::Draw(void)
{
    char pBuffer[40];
    if (gGameOptions.nGameType == 0)
    {
        viewDrawText(1, FStringf("%s:", GStrings("KILLS")), 75, 50, -128, 0, 0, 1);
        sprintf(pBuffer, "%2d", at4);
        viewDrawText(1, pBuffer, 160, 50, -128, 0, 0, 1);
        viewDrawText(1, GStrings("OF"), 190, 50, -128, 0, 0, 1);
        sprintf(pBuffer, "%2d", at0);
        viewDrawText(1, pBuffer, 220, 50, -128, 0, 0, 1);
    }
    else
    {
        viewDrawText(3, "#", 85, 35, -128, 0, 0, 1);
        viewDrawText(3, GStrings("NAME"), 100, 35, -128, 0, 0, 1);
        viewDrawText(3, GStrings("FRAGS"), 210, 35, -128, 0, 0, 1);
        int nStart = 0;
        int nEnd = gInitialNetPlayers;
        //if (dword_28E3D4 == 1)
        //{
        //    nStart++;
        //    nEnd++;
        //}
        for (int i = nStart; i < nEnd; i++)
        {
            sprintf(pBuffer, "%-2d", i);
            viewDrawText(3, pBuffer, 85, 50+8*i, -128, 0, 0, 1);
            sprintf(pBuffer, "%s", gProfile[i].name);
            viewDrawText(3, pBuffer, 100, 50+8*i, -128, 0, 0, 1);
            sprintf(pBuffer, "%d", gPlayer[i].fragCount);
            viewDrawText(3, pBuffer, 210, 50+8*i, -128, 0, 0, 1);
        }
    }
}

void CKillMgr::Clear(void)
{
    at0 = at4 = 0;
}

CSecretMgr::CSecretMgr(void)
{
    Clear();
}

void CSecretMgr::SetCount(int nCount)
{
    at0 = nCount;
}

void CSecretMgr::Found(int nType)
{
    if (nType == 0) at4++;
    else if (nType < 0) {
        viewSetSystemMessage("Invalid secret type %d triggered.", nType);
        return;
    } else at8++;

    if (gGameOptions.nGameType == 0) {
		viewSetMessage(GStrings(FStringf("TXTB_SECRET%d", Random(2))),  0, MESSAGE_PRIORITY_SECRET);
    }
}

void CSecretMgr::Draw(void)
{
    char pBuffer[40];
    viewDrawText(1, FStringf("%s:", GStrings("TXT_SECRETS")), 75, 70, -128, 0, 0, 1);
    sprintf(pBuffer, "%2d", at4);
    viewDrawText(1, pBuffer, 160, 70, -128, 0, 0, 1);
    viewDrawText(1, GStrings("OF"), 190, 70, -128, 0, 0, 1);
    sprintf(pBuffer, "%2d", at0);
    viewDrawText(1, pBuffer, 220, 70, -128, 0, 0, 1);
    if (at8 > 0)
        viewDrawText(1, GStrings("TXT_SUPERSECRET"), 160, 100, -128, 2, 1, 1);
}

void CSecretMgr::Clear(void)
{
    at0 = at4 = at8 = 0;
}

class EndGameLoadSave : public LoadSave {
public:
    virtual void Load(void);
    virtual void Save(void);
};

void EndGameLoadSave::Load(void)
{
    Read(&gSecretMgr.at0, 4);
    Read(&gSecretMgr.at4, 4);
    Read(&gSecretMgr.at8, 4);
    Read(&gKillMgr.at0, 4);
    Read(&gKillMgr.at4, 4);
}

void EndGameLoadSave::Save(void)
{
    Write(&gSecretMgr.at0, 4);
    Write(&gSecretMgr.at4, 4);
    Write(&gSecretMgr.at8, 4);
    Write(&gKillMgr.at0, 4);
    Write(&gKillMgr.at4, 4);
}

CEndGameMgr gEndGameMgr;
CSecretMgr gSecretMgr;
CKillMgr gKillMgr;
static EndGameLoadSave *myLoadSave;

void EndGameLoadSaveConstruct(void)
{
    myLoadSave = new EndGameLoadSave();
}

END_BLD_NS
