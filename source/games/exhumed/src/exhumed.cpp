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
#include "input.h"
#include "sound.h"
#include "view.h"
#include "status.h"
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

TObjPtr<DExhumedActor*> bestTarget;

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

    GC::Mark(bestTarget);
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
    resettiming();
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

int nFontFirstChar;
int nBackgroundPic;
int nShadowPic;

int nCreaturesKilled = 0, nCreaturesTotal = 0;

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

	auto pixels = GetWritablePixels(tileGetTextureID(kTile3603));

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
        auto texid = tileGetTextureID(v2 + kClockSymbol1);
        auto tex = TexMan.GetGameTexture(texid);
        int yPos = 32 - tileHeight(v2 + kClockSymbol1) / 2;

        CopyTileToBitmap(texid, tileGetTextureID(kTile3603), ebp - tex->GetTexelWidth() / 2, yPos);

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

double calc_interpfrac()
{
    return bRecord || bPlayback || nFreeze != 0 || paused || cl_capfps || !cl_interpolate || EndLevel ? 1. : I_GetTimeFrac();
}

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

void GameMove(void)
{
    FixPalette();

	ExhumedSpriteIterator it;
    while (auto ac = it.Next())
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
        // Pink section
        lCountDown--;
        DrawClock();

        if (nRedTicks)
        {
            nRedTicks--;

            if (nRedTicks <= 0) {
                DoRedAlert(0);
            }
        }

        nAlarmTicks--;
        nButtonColor--;

        if (nAlarmTicks <= 0) {
            DoRedAlert(1);
        }
    }

    // YELLOW SECTION
    MoveThings();

    obobangle = bobangle;

    if (PlayerList[nLocalPlayer].totalvel == 0)
    {
        bobangle = 0;
    }
    else
    {
        bobangle += 56;
    }

    UpdateCreepySounds();

    // loc_120E9:
    totalmoves++;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void updatePlayerVelocity(Player* const pPlayer)
{
    const auto pInput = &pPlayer->input;

    if (pPlayer->nHealth > 0)
    {
        const auto inputvect = DVector2(pInput->fvel, pInput->svel).Rotated(pPlayer->pActor->spr.Angles.Yaw) * 0.375;

        for (int i = 0; i < 4; i++)
        {
            pPlayer->vel += inputvect;
            pPlayer->vel *= 0.953125;
        }
    }
    else
    {
        pInput->fvel = pInput->svel = pInput->avel = pInput->horz = 0;
        pPlayer->vel.Zero();
    }
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
    {
        pPlayer->input.setItemUsed(pPlayer->nItem);
    }

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

void GameInterface::Ticker()
{

	if (paused)
	{
		r_NoInterpolate = true;
	}
    else if (EndLevel == 0)
    {
        // Shorten some constant array accesses.
        const auto pPlayer = &PlayerList[nLocalPlayer];

        // this must be done before the view is backed up.
        pPlayer->Angles.resetCameraAngles();

        // disable synchronised input if set by game.
        resetForcedSyncInput();

        // set new player input.
        pPlayer->input = playercmds[nLocalPlayer].ucmd;

        UpdateInterpolations();

        updatePlayerVelocity(pPlayer);
        updatePlayerInventory(pPlayer);
        updatePlayerWeapon(pPlayer);

        pPlayer->pTarget = Ra[nLocalPlayer].pTarget = bestTarget;

        GameMove();

        PlayClock += 4;
        if (PlayClock == 8) gameaction = ga_autosave;	// let the game run for 1 frame before saving.
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
    SetTileNames(info);
    info.CreateWritable(kTile4092, kPlasmaWidth, kPlasmaHeight);
    info.CreateWritable(kTile4093, kPlasmaWidth, kPlasmaHeight);
    info.CreateWritable(kTileRamsesWorkTile, kSpiritY * 2, kSpiritX * 2);
    info.MakeWritable(kTileLoboLaptop);
    for(int i = kTile3603; i < kClockSymbol1 + 145; i++)
    info.MakeWritable(kTile3603);
    info.MakeWritable(kEnergy1);
    info.MakeWritable(kEnergy2);
    for (int i = 0; i < 16; i++)
        info.MakeWritable(kClockSymbol1);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GameInterface::app_init()
{
    GC::AddMarkerFunc(markgcroots);


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
    InitStatus();

    resettiming();
    GrabPalette();

    enginecompatibility_mode = ENGINECOMPATIBILITY_19961112;
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

    if (actor == bestTarget) {
        bestTarget = nullptr;
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
    return gamestate == GS_LEVEL && !bRecord && !bPlayback && !bInDemo && nTotalPlayers == 1 && nFreeze == 0;
}

::GameStats GameInterface::getStats()
{
    return { nCreaturesKilled, nCreaturesTotal, 0, 0, PlayClock / 120, 0 };
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

        arc ("besttarget", bestTarget)
            ("creaturestotal", nCreaturesTotal)
            ("creatureskilled", nCreaturesKilled)
            ("freeze", nFreeze)
            ("snakecam", nSnakeCam)
            ("clockval", nClockVal)  // kTile3603
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
