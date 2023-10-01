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
#include "engine.h"
#include "exhumed.h"
#include "aistuff.h"
#include "sequence.h"
#include "names.h"
#include "player.h"
#include "sound.h"
#include "view.h"
#include "version.h"
#include "aistuff.h"
#include "mapinfo.h"
#include <string.h>
#include <cstdio> // for printf
#include <cstdlib>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>
#include "gamecvars.h"
#include "savegamehelp.h"
#include "c_dispatch.h"
#include "raze_sound.h"
#include "gamestate.h"
#include "screenjob.h"
#include "c_console.h"
#include "cheathandler.h"
#include "inputstate.h"
#include "d_protocol.h"
#include "texturemanager.h"
#include "razemenu.h"
#include "v_draw.h"
#include "interpolate.h"
#include "tilesetbuilder.h"
#include "psky.h"

BEGIN_PS_NS

IMPLEMENT_CLASS(DExhumedActor, false, true)
IMPLEMENT_POINTERS_START(DExhumedActor)
IMPLEMENT_POINTER(pTarget)
IMPLEMENT_POINTERS_END

size_t MarkMove();
size_t MarkBullets();
size_t MarkItems();
size_t MarkLighting();
size_t MarkObjects();
size_t MarkPlayers();
size_t MarkQueen();
size_t MarkRa();
size_t MarkSnake();
size_t MarkRunlist();


static void markgcroots()
{
    MarkBullets();
    MarkItems();
    MarkLighting();
    MarkObjects();
    MarkPlayers();
    MarkQueen();
    MarkRa();
    MarkSnake();
    MarkRunlist();

    GC::Mark(pSpiritSprite);
}

static MapRecord* NextMap;

void uploadCinemaPalettes();
int32_t registerosdcommands(void);
void InitCheats();

int EndLevel = 0;


////////

void ResetEngine()
{
    EraseScreen(-1);
}

void GameInterface::loadPalette()
{
    paletteLoadFromDisk();
    uploadCinemaPalettes();
    LoadPaletteLookups();
}

void CopyTileToBitmap(FTextureID nSrcTile, FTextureID nDestTile, int xPos, int yPos);

// void TestSaveLoad();
void LoadStatus();
void MySetView(int x1, int y1, int x2, int y2);

char sHollyStr[40];

int nFreeze;

int nSnakeCam = -1;

int nNetPlayerCount = 0;

int nClockVal;
int nRedTicks;
int nAlarmTicks;
int nButtonColor;
int nEnergyChan;

int lCountDown = 0;
int nEnergyTowers = 0;

int nCfgNetPlayers = 0;

bool bCoordinates = false;

int nNetTime = -1;

int flash;
int totalmoves;

int nCurBodyNum = 0;

int nBodyTotal = 0;

int nTotalPlayers = 1;
// TODO: Rename this (or make it static) so it doesn't conflict with library function


bool bSnakeCam = false;
bool bRecord = false;
bool bPlayback = false;
bool bInDemo = false;
bool bSlipMode = false;
bool bDoFlashes = true;


int nStartLevel;
int nTimeLimit;

int bVanilla = 0;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DebugOut(const char *fmt, ...)
{
#ifdef _DEBUG
    va_list args;
    va_start(args, fmt);
	VPrintf(PRINT_HIGH, fmt, args);
#endif
}

void DoClockBeep()
{
    ExhumedStatIterator it(407);
    while (auto i = it.Next())
    {
        PlayFX2(StaticSound[kSound74], i);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoRedAlert(int nVal)
{
    if (nVal)
    {
        nAlarmTicks = 69;
        nRedTicks = 30;
    }

    ExhumedStatIterator it(405);
    while (auto ac = it.Next())
    {
        if (nVal)
        {
            PlayFXAtXYZ(StaticSound[kSoundAlarm], ac->spr.pos);
            AddFlash(ac->sector(), ac->spr.pos, 192);
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DrawClock()
{
    int ebp = 49;

	auto pixels = GetWritablePixels(aTexIds[kTexClockTile]);

    memset(pixels, TRANSPARENT_INDEX, 4096);

    if (lCountDown / 30 != nClockVal)
    {
        nClockVal = lCountDown / 30;
        DoClockBeep();
    }

    int nVal = nClockVal;

    while (nVal)
    {
        int v2 = nVal & 0xF;
        auto texid = aTexIds[v2 + kTexClockSymbol1];
        auto tex = TexMan.GetGameTexture(texid);
        int yPos = 32 - tex->GetTexelHeight() / 2;

        CopyTileToBitmap(texid, aTexIds[kTexClockTile], ebp - tex->GetTexelWidth() / 2, yPos);

        ebp -= 15;

        nVal /= 16;
    }

    DoEnergyTile();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoGameOverScene(bool finallevel)
{
    // todo: make these customizable later.
    StartCutscene(finallevel ? "ExhumedCutscenes.BuildCinemaLose" : "ExhumedCutscenes.BuildGameoverScene", 0, [](bool)
        {
            gameaction = ga_mainmenu;
        });
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void GameMove(void)
{
    UpdateInterpolations();
    FixPalette();

	ExhumedSpriteIterator it;
    while (const auto ac = it.Next())
    {
		ac->backuploc();
    }

    if (currentLevel->gameflags & LEVEL_EX_COUNTDOWN)
    {
        if (lCountDown <= 0)
        {
            DoGameOverScene(true);
            return;
        }

        nButtonColor--;
        lCountDown--;
        DrawClock();

        if (nRedTicks && --nRedTicks <= 0)
            DoRedAlert(0);

        if (--nAlarmTicks <= 0)
            DoRedAlert(1);
    }

    MoveThings();
    totalmoves++;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GameInterface::Ticker(const ticcmd_t* playercmds)
{
	if (paused)
	{
		r_NoInterpolate = true;
	}
    else if (EndLevel == 0)
    {
        // disable synchronised input if set by game.
        resetForcedSyncInput();

        for (int i = connecthead; i >= 0; i = connectpoint2[i])
        {
            const auto pPlayer = &PlayerList[i];
            pPlayer->Angles.resetCameraAngles();
            pPlayer->input = playercmds[i].ucmd;
            updatePlayerTarget(pPlayer);
        }

        GameMove();

        PlayClock += 4;
        r_NoInterpolate = false;
    }
	else
	{
		// Wait for the end of level sound to play out, but stop updating the playsim.
        if (EndLevel == 13)
            PlayLocalSound(StaticSound[59], 0, true, CHANF_UI);

        if (EndLevel > 1) EndLevel--;
        int flash = 7 - abs(EndLevel - 7);
        videoTintBlood(flash * 30, flash * 30, flash * 30);
        if (EndLevel == 1)
        {
            if (!soundEngine->GetSoundPlayingInfo(SOURCE_None, nullptr, FSoundID::fromInt(StaticSound[59] + 1)))
            {
                videoTintBlood(0, 0, 0);
                CompleteLevel(NextMap);
                NextMap = nullptr;
                EndLevel = 0;
            }
        }
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void LevelFinished()
{
    NextMap = FindNextMap(currentLevel);
    EndLevel = 13;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

#define x(a, b) registerName(#a, b);
static void SetTileNames(TilesetBuildInfo& info)
{
    auto registerName = [&](const char* name, int index)
    {
        info.addName(name, index);
    };
#include "namelist.h"
}
#undef x

void GameInterface::SetupSpecialTextures(TilesetBuildInfo& info)
{
    // This is the ONLY place that should use tile indices!
    enum
    {
        kPlasmaTile1 = 4092,
        kPlasmaTile2 = 4093,
        kClockTile = 3603,
        kEnergy1 = 3604,
        kEnergy2 = 3605,
        kTileLoboLaptop = 3623,
        kTileRamsesWorkTile = 591,
    };

    SetTileNames(info);
    info.CreateWritable(kPlasmaTile1, kPlasmaWidth, kPlasmaHeight);
    info.CreateWritable(kPlasmaTile2, kPlasmaWidth, kPlasmaHeight);
    info.CreateWritable(kTileRamsesWorkTile, kSpiritY * 2, kSpiritX * 2);
    info.MakeWritable(kTileLoboLaptop);
    info.MakeWritable(kClockTile);
    info.MakeWritable(kEnergy1);
    info.MakeWritable(kEnergy2);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GameInterface::app_init()
{
    GC::AddMarkerFunc(markgcroots);
	InitTextureIDs();

#if 0
    help_disabled = true;
#endif

	InitCheats();
    registerosdcommands();
    if (nNetPlayerCount == -1)
    {
        nNetPlayerCount = nCfgNetPlayers - 1;
        nTotalPlayers += nNetPlayerCount;
    }

    defineSky(nullptr, 2, nullptr, 256, 1.f);

    InitFX();
    seq_LoadSequences();

    GrabPalette();

    enginecompatibility_mode = ENGINECOMPATIBILITY_19961112;

    myconnectindex = connecthead = 0;
    numplayers = 1;
    connectpoint2[0] = -1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DeleteActor(DExhumedActor* actor)
{
    if (!actor) 
    {
        return;
    }

    UnlinkIgnitedAnim(actor);
    actor->Destroy();
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void CopyTileToBitmap(FTextureID nSrcTile,  FTextureID nDestTile, int xPos, int yPos)
{
    auto pSrcTex = TexMan.GetGameTexture(nSrcTile);
    auto pDestTex = TexMan.GetGameTexture(nDestTile);
    int nOffs = pDestTex->GetTexelHeight() * xPos;

	auto pixels = GetWritablePixels(nDestTile);
    if (!pixels) return;
    uint8_t *pDest = pixels + nOffs + yPos;
    uint8_t *pDestB = pDest;

    int destYSize = pDestTex->GetTexelHeight();
    int srcYSize = pSrcTex->GetTexelHeight();

    const uint8_t *pSrc = GetRawPixels(nSrcTile);
    if (!pSrc) return;

    for (int x = 0; x < pSrcTex->GetTexelWidth(); x++)
    {
        pDest += destYSize;

        for (int y = 0; y < srcYSize; y++)
        {
            uint8_t val = *pSrc;
            if (val != TRANSPARENT_INDEX) {
                *pDestB = val;
            }

            pDestB++;
            pSrc++;
        }

        // reset pDestB
        pDestB = pDest;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void EraseScreen(int nVal)
{
    // There's no other values than 0 ever coming through here.
    twod->ClearScreen();
}

bool GameInterface::CanSave()
{
    return !bRecord && !bPlayback && !bInDemo && nTotalPlayers == 1 && nFreeze == 0;
}

::GameStats GameInterface::getStats()
{
    return { Level.kills.got, Level.kills.max, Level.secrets.got, Level.secrets.max, PlayClock / 120, 0 };
}

::GameInterface* CreateInterface()
{
    return new GameInterface;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DExhumedActor::Serialize(FSerializer& arc)
{
    Super::Serialize(arc);
    arc("phase", nPhase)
        ("health", nHealth)
        ("frame", nFrame)
        ("action", nAction)
        ("target", pTarget)
        ("count", nCount)
        ("run", nRun)
        ("index", nIndex)
        ("index2", nIndex2)
        ("channel", nChannel)
        ("damage", nDamage)
        ("seqidx", nSeqIndex)
        ("seqfile", nSeqFile)
        ("flags", nFlags)
        ("angle2", pitch)

        ("turn", nTurn)
        ("vec", vec);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SerializeState(FSerializer& arc)
{
    if (arc.BeginObject("state"))
    {
        if (arc.isReading() && (currentLevel->gameflags & LEVEL_EX_COUNTDOWN))
        {
            InitEnergyTile();
    }

        arc ("freeze", nFreeze)
            ("snakecam", nSnakeCam)
            ("clockval", nClockVal)
            ("redticks", nRedTicks)
            ("alarmticks", nAlarmTicks)
            ("buttoncolor", nButtonColor)
            ("energychan", nEnergyChan)
            ("countdown", lCountDown)
            ("energytowers", nEnergyTowers)
            ("totalmoves", totalmoves)
            ("curbodynum", nCurBodyNum)
            ("bodytotal", nBodyTotal)
            ("bsnakecam", bSnakeCam)
            ("slipmode", bSlipMode)
            ("PlayClock", PlayClock)
            ("spiritsprite", pSpiritSprite)
            .EndObject();
    }
}

END_PS_NS
