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
#include "engine.h"
#include "exhumed.h"
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

BEGIN_PS_NS

DExhumedActor exhumedActors[MAXSPRITES];

static MapRecord* NextMap;

void uploadCinemaPalettes();
int32_t registerosdcommands(void);
void InitCheats();

int EndLevel = 0;


InputPacket localInput;

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

void CopyTileToBitmap(short nSrcTile, short nDestTile, int xPos, int yPos);

// void TestSaveLoad();
void EraseScreen(int nVal);
void LoadStatus();
void MySetView(int x1, int y1, int x2, int y2);

char sHollyStr[40];

short nFontFirstChar;
short nBackgroundPic;
short nShadowPic;

short nCreaturesKilled = 0, nCreaturesTotal = 0;

short nFreeze;

short nSnakeCam = -1;

int nNetPlayerCount = 0;

short nClockVal;
short nRedTicks;
short nAlarmTicks;
short nButtonColor;
short nEnergyChan;


short bModemPlay = false;
int lCountDown = 0;
short nEnergyTowers = 0;

short nCfgNetPlayers = 0;
FILE *vcrfp = NULL;

int lLocalCodes = 0;

short bCoordinates = false;

int nNetTime = -1;

short nCodeMin = 0;
short nCodeMax = 0;
short nCodeIndex = 0;

//short nScreenWidth = 320;
//short nScreenHeight = 200;
int flash;
int totalmoves;

short nCurBodyNum = 0;

short nBodyTotal = 0;

short lastfps;
short nTotalPlayers = 1;
// TODO: Rename this (or make it static) so it doesn't conflict with library function
//short socket = 0;

short nFirstPassword = 0;
short nFirstPassInfo = 0;
short nPasswordCount = 0;

short bSnakeCam = false;
short bRecord = false;
short bPlayback = false;
short bInDemo = false;
short bSlipMode = false;
short bDoFlashes = true;

DExhumedActor* bestTarget;

short scan_char = 0;

int nStartLevel;
int nTimeLimit;

int bVanilla = 0;

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
			auto spri = &ac->s();
            PlayFXAtXYZ(StaticSound[kSoundAlarm], spri->x, spri->y, spri->z, spri->sectnum);
            AddFlash(spri->sectnum, spri->x, spri->y, spri->z, 192);
        }
    }
}

void DrawClock()
{
    int ebp = 49;

	auto pixels = TileFiles.tileMakeWritable(kTile3603);

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
        int yPos = 32 - tileHeight(v2 + kClockSymbol1) / 2;

        CopyTileToBitmap(v2 + kClockSymbol1, kTile3603, ebp - tileWidth(v2 + kClockSymbol1) / 2, yPos);

        ebp -= 15;

        nVal /= 16;
    }

    DoEnergyTile();
}

double calc_smoothratio()
{
    if (bRecord || bPlayback || nFreeze != 0 || paused)
        return MaxSmoothRatio;

    return I_GetTimeFrac() * MaxSmoothRatio;
}

void DoGameOverScene(bool finallevel)
{
    // todo: make these customizable later.
    StartCutscene(finallevel ? "ExhumedCutscenes.BuildCinemaLose" : "ExhumedCutscenes.BuildGameoverScene", 0, [](bool)
        {
            gameaction = ga_mainmenu;
        });
}

void GameMove(void)
{
    FixPalette();

	ExhumedSpriteIterator it;
    while (auto ac = it.Next())
    {
		ac->s().backuploc();
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
        bobangle &= kAngleMask;
    }

    UpdateCreepySounds();

    // loc_120E9:
    totalmoves++;
}

static int SelectAltWeapon(int weap2)
{
    // todo
    return 0;
}


void GameInterface::Ticker()
{

	if (paused)
	{
		r_NoInterpolate = true;
	}
    else if (EndLevel == 0)
    {
        inita &= kAngleMask;

        for (int i = 0; i < 4; i++)
        {
            lPlayerXVel += localInput.fvel * bcos(inita) + localInput.svel * bsin(inita);
            lPlayerYVel += localInput.fvel * bsin(inita) - localInput.svel * bcos(inita);
            lPlayerXVel -= (lPlayerXVel >> 5) + (lPlayerXVel >> 6);
            lPlayerYVel -= (lPlayerYVel >> 5) + (lPlayerYVel >> 6);
        }
        UpdateInterpolations();

        if (localInput.actions & SB_INVPREV)
        {
            int nItem = PlayerList[nLocalPlayer].nItem;

            int i;
            for (i = 6; i > 0; i--)
            {
                nItem--;
                if (nItem < 0) nItem = 5;

                if (PlayerList[nLocalPlayer].items[nItem] != 0)
                    break;
            }

            if (i > 0) PlayerList[nLocalPlayer].nItem = nItem;
        }

        if (localInput.actions & SB_INVNEXT)
        {
            int nItem = PlayerList[nLocalPlayer].nItem;

            int i;
            for (i = 6; i > 0; i--)
            {
                nItem++;
                if (nItem == 6) nItem = 0;

                if (PlayerList[nLocalPlayer].items[nItem] != 0)
                    break;
            }

            if (i > 0) PlayerList[nLocalPlayer].nItem = nItem;
        }

        if (localInput.actions & SB_INVUSE)
        {
            if (PlayerList[nLocalPlayer].nItem != -1)
            {
                localInput.setItemUsed(PlayerList[nLocalPlayer].nItem);
            }
        }

        for (int i = 0; i < 6; i++)
        {
            if (localInput.isItemUsed(i))
            {
                localInput.clearItemUsed(i);
                if (PlayerList[nLocalPlayer].items[i] > 0)
                {
                    if (nItemMagic[i] <= PlayerList[nLocalPlayer].nMagic)
                    {
                        sPlayerInput[nLocalPlayer].nItem = i;
                        break;
                    }
                }
            }
        }

        auto currWeap = PlayerList[nLocalPlayer].nCurrentWeapon;
        int weap2 = localInput.getNewWeapon();
        if (weap2 == WeaponSel_Next)
        {
            auto newWeap = currWeap == 6 ? 0 : currWeap + 1;
            while (newWeap != 0 && (!(PlayerList[nLocalPlayer].nPlayerWeapons & (1 << newWeap)) || (PlayerList[nLocalPlayer].nPlayerWeapons & (1 << newWeap) && PlayerList[nLocalPlayer].nAmmo[newWeap] == 0)))
            {
                newWeap++;
                if (newWeap > 6) newWeap = 0;
            }
            localInput.setNewWeapon(newWeap + 1);
        }
        else if (weap2 == WeaponSel_Prev)
        {
            auto newWeap = currWeap == 0 ? 6 : currWeap - 1;
            while (newWeap != 0 && ((!(PlayerList[nLocalPlayer].nPlayerWeapons & (1 << newWeap)) || (PlayerList[nLocalPlayer].nPlayerWeapons & (1 << newWeap) && PlayerList[nLocalPlayer].nAmmo[newWeap] == 0))))
            {
                newWeap--;
            }
            localInput.setNewWeapon(newWeap + 1);
        }
        else if (weap2 == WeaponSel_Alt)
        {
            weap2 = SelectAltWeapon(weap2);
        }

        // make weapon selection persist until it gets used up.
        int weap = sPlayerInput[nLocalPlayer].getNewWeapon();
        if (weap2 <= 0 || weap2 > 7) sPlayerInput[nLocalPlayer].SetNewWeapon(weap);

        auto oldactions = sPlayerInput[nLocalPlayer].actions;
        sPlayerInput[nLocalPlayer].actions = localInput.actions;
        if (oldactions & SB_CENTERVIEW) sPlayerInput[nLocalPlayer].actions |= SB_CENTERVIEW;        

        sPlayerInput[nLocalPlayer].xVel = lPlayerXVel;
        sPlayerInput[nLocalPlayer].yVel = lPlayerYVel;
        sPlayerInput[nLocalPlayer].buttons = lLocalCodes;
        sPlayerInput[nLocalPlayer].pTarget = bestTarget;
        sPlayerInput[nLocalPlayer].nAngle = localInput.avel;
        sPlayerInput[nLocalPlayer].pan = localInput.horz;

        Ra[nLocalPlayer].pTarget = bestTarget;

        lLocalCodes = 0;

        PlayClock += 4;
        if (PlayClock == 8) gameaction = ga_autosave;	// let the game run for 1 frame before saving.
        GameMove();
        r_NoInterpolate = false;
    }
	else
	{
		// Wait for the end of level sound to play out, but stop updating the playsim.
        if (EndLevel == 13)
            PlayLocalSound(StaticSound[59], 0, true, CHANF_UI);

        if (EndLevel > 1) EndLevel--;
		r_NoInterpolate = true;
        int flash = 7 - abs(EndLevel - 7);
        videoTintBlood(flash * 30, flash * 30, flash * 30);
        if (EndLevel == 1)
        {
            if (!soundEngine->GetSoundPlayingInfo(SOURCE_None, nullptr, StaticSound[59] + 1))
            {
                videoTintBlood(0, 0, 0);
                CompleteLevel(NextMap);
                NextMap = nullptr;
                EndLevel = 0;
            }
        }
	}
}

void LevelFinished()
{
    NextMap = FindNextMap(currentLevel);
    EndLevel = 13;
}

#define x(a, b) registerName(#a, b);
static void SetTileNames()
{
    auto registerName = [](const char* name, int index)
    {
        TexMan.AddAlias(name, tileGetTexture(index));
    };
#include "namelist.h"
}
#undef x

void GameInterface::app_init()
{
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

    SetTileNames();

    InitFX();
    seq_LoadSequences();
    InitStatus();
    
    resettiming();
    GrabPalette();

    enginecompatibility_mode = ENGINECOMPATIBILITY_19950829;
}

void DeleteActor(DExhumedActor* actor) 
{
    if (!actor) 
    {
        return;
    }

    FVector3 pos = GetSoundPos(&actor->s().pos);
    soundEngine->RelinkSound(SOURCE_Actor, &actor->s(), nullptr, &pos);

    deletesprite(actor->GetSpriteIndex());
    actor->s().ox = 0x80000000;

    if (actor == bestTarget) {
        bestTarget = nullptr;
    }
}



void CopyTileToBitmap(short nSrcTile,  short nDestTile, int xPos, int yPos)
{
    int nOffs = tileHeight(nDestTile) * xPos;

	auto pixels = TileFiles.tileMakeWritable(nDestTile);
    uint8_t *pDest = pixels + nOffs + yPos;
    uint8_t *pDestB = pDest;

    tileLoad(nSrcTile);

    int destYSize = tileHeight(nDestTile);
    int srcYSize = tileHeight(nSrcTile);

    const uint8_t *pSrc = tilePtr(nSrcTile);

    for (int x = 0; x < tileWidth(nSrcTile); x++)
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

    TileFiles.InvalidateTile(nDestTile);
}

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

extern short cPupData[300];
extern uint8_t* Worktile;
extern int lHeadStartClock;
extern short* pPupData;

FSerializer& Serialize(FSerializer& arc, const char* keyname, DExhumedActor& w, DExhumedActor* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("phase", w.nPhase)
            ("health", w.nHealth)
            ("frame", w.nFrame)
            ("action", w.nAction)
            ("target", w.pTarget)
            ("count", w.nCount)
            ("run", w.nRun)
            ("index", w.nIndex)
			("index2", w.nIndex2)
			("channel", w.nChannel)
            ("damage", w.nDamage)

            ("turn", w.nTurn)
            ("x", w.x)
            ("y", w.y)
            .EndObject();
    }
    return arc;
}

void SerializeState(FSerializer& arc)
{
    int loaded = 0;
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
