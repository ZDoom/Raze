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
#include "ps_input.h"
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
#include "core/menu/menu.h"

BEGIN_PS_NS

extern short bPlayerPan;
extern short bLockPan;


static MapRecord* NextMap;

void uploadCinemaPalettes();
int32_t registerosdcommands(void);
void InitFonts();
void InitCheats();

int EndLevel = 0;


InputPacket localInput;

////////

void ResetEngine()
{
    EraseScreen(-1);
    resettiming();
}

void InstallEngine()
{
	TileFiles.LoadArtSet("tiles%03d.art");

    if (engineInit())
    {
        G_FatalEngineError();
    }
    uploadCinemaPalettes();
    LoadPaletteLookups();
    InitFonts();
}

void RemoveEngine()
{
    engineUnInit();
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
int leveltime;

short nFreeze;

short nSnakeCam = -1;

short nLocalSpr;

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

short besttarget;

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

void ShutDown(void)
{
    StopCD();

    RemoveEngine();
    //UnInitFX();
}

void DoClockBeep()
{
    for (int i = headspritestat[407]; i != -1; i = nextspritestat[i]) {
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

    for (int i = headspritestat[405]; i != -1; i = nextspritestat[i])
    {
        if (nVal)
        {
            PlayFXAtXYZ(StaticSound[kSoundAlarm], sprite[i].x, sprite[i].y, sprite[i].z, sprite[i].sectnum);
            AddFlash(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z, 192);
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
        int yPos = 32 - tilesiz[v2 + kClockSymbol1].y / 2;

        CopyTileToBitmap(v2 + kClockSymbol1, kTile3603, ebp - tilesiz[v2 + kClockSymbol1].x / 2, yPos);

        ebp -= 15;

        nVal /= 16;
    }

    DoEnergyTile();
}

double calc_smoothratio()
{
    if (bRecord || bPlayback || nFreeze != 0 || bCamera || paused)
        return MaxSmoothRatio;

    return I_GetTimeFrac() * MaxSmoothRatio;
}

void GameMove(void)
{
    FixPalette();

    if (currentLevel->levelNumber == kMap20)
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

    if (totalvel[nLocalPlayer] == 0)
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

static int SelectNextWeapon(int weap2)
{
    // todo
    return 0;
}

static int SelectPrevWeapon(int weap2)
{
    // todo
    return 0;
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
        nPlayerDAng += localInput.q16avel;
        inita &= kAngleMask;

        for (int i = 0; i < 4; i++)
        {
            lPlayerXVel += localInput.fvel * Cos(inita) + localInput.svel * Sin(inita);
            lPlayerYVel += localInput.fvel * Sin(inita) - localInput.svel * Cos(inita);
            lPlayerXVel -= (lPlayerXVel >> 5) + (lPlayerXVel >> 6);
            lPlayerYVel -= (lPlayerYVel >> 5) + (lPlayerYVel >> 6);
        }
        int weap2 = localInput.getNewWeapon();
        if (weap2 == WeaponSel_Next)
        {
            weap2 = SelectNextWeapon(weap2);
        }
        else if (weap2 == WeaponSel_Prev)
        {
            weap2 = SelectPrevWeapon(weap2);
        }
        else if (weap2 == WeaponSel_Alt)
        {
            weap2 = SelectAltWeapon(weap2);
        }

        if (localInput.actions & SB_INVPREV)
        {
            int nItem = nPlayerItem[nLocalPlayer];

            int i;
            for (i = 6; i > 0; i--)
            {
                nItem--;
                if (nItem < 0) nItem = 5;

                if (PlayerList[nLocalPlayer].items[nItem] != 0)
                    break;
            }

            if (i > 0) SetPlayerItem(nLocalPlayer, nItem);
        }

        if (localInput.actions & SB_INVNEXT)
        {
            int nItem = nPlayerItem[nLocalPlayer];

            int i;
            for (i = 6; i > 0; i--)
            {
                nItem++;
                if (nItem == 6) nItem = 0;

                if (PlayerList[nLocalPlayer].items[nItem] != 0)
                    break;
            }

            if (i > 0) SetPlayerItem(nLocalPlayer, nItem);
        }

        if (localInput.actions & SB_INVUSE)
        {
            if (nPlayerItem[nLocalPlayer] != -1)
            {
                localInput.setItemUsed(nPlayerItem[nLocalPlayer]);
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

        if (localInput.actions & SB_CENTERVIEW)
        {
            bLockPan = false;
            bPlayerPan = false;
            //PlayerList[nLocalPlayer].q16horiz = IntToFixed(92);
            nDestVertPan[nLocalPlayer] = IntToFixed(92);
        }
        if (localInput.actions & SB_TURNAROUND)
        {
            // todo
        }


        sPlayerInput[nLocalPlayer].xVel = lPlayerXVel;
        sPlayerInput[nLocalPlayer].yVel = lPlayerYVel;
        // make weapon selection persist until it gets used up.
        sPlayerInput[nLocalPlayer].buttons = lLocalCodes;
        int weap = sPlayerInput[nLocalPlayer].getNewWeapon();
        sPlayerInput[nLocalPlayer].actions = localInput.actions;
        if (weap2 <= 0 || weap2 > 7) sPlayerInput[nLocalPlayer].SetNewWeapon(weap);
        sPlayerInput[nLocalPlayer].nTarget = besttarget;

        Ra[nLocalPlayer].nTarget = besttarget;

        lLocalCodes = 0;
        nPlayerDAng = 0;

        sPlayerInput[nLocalPlayer].horizon = PlayerList[nLocalPlayer].q16horiz;

        leveltime++;
        if (leveltime == 2) gameaction = ga_autosave;	// let the game run for 1 frame before saving.
        GameMove();
        r_NoInterpolate = false;
    }
	else
	{
		// Wait for the end of level sound to play out, but stop updating the playsim.
        if (EndLevel == 13)
            PlayLocalSound(StaticSound[59], 0, true, CHANF_UI);

        if (EndLevel > 1) EndLevel--;
		r_NoInterpolate = false;
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
    NextMap = currentLevel->levelNumber == 20 ? nullptr : FindMapByLevelNum(currentLevel->levelNumber + 1); // todo: Use the map record for progression
    EndLevel = 13;
}

void ExitGame()
{
    ShutDown();
    throw CExitEvent(0);
}

void GameInterface::app_init()
{
    int i;
    //int esi = 1;
    //int edi = esi;

    help_disabled = true;
    // Create the global level table. Parts of the engine need it, even though the game itself does not.
    for (int i = 0; i <= 32; i++)
    {
        auto mi = AllocateMap();
        mi->fileName.Format("LEV%d.MAP", i);
        mi->labelName.Format("LEV%d", i);
        mi->name.Format("$TXT_EX_MAP%02d", i);
        mi->levelNumber = i;

        int nTrack = i;
        if (nTrack != 0) nTrack--;
        mi->cdSongId = (nTrack % 8) + 11;
    }

	InitCheats();
    registerosdcommands();
    if (nNetPlayerCount == -1)
    {
        nNetPlayerCount = nCfgNetPlayers - 1;
        nTotalPlayers += nNetPlayerCount;
    }

    // temp - moving InstallEngine(); before FadeOut as we use nextpage() in FadeOut
    InstallEngine();

    const char* defsfile = G_DefFile();
    uint32_t stime = I_msTime();
    if (!loaddefinitionsfile(defsfile))
    {
        uint32_t etime = I_msTime();
        Printf(PRINT_NONOTIFY, "Definitions file \"%s\" loaded in %d ms.\n", defsfile, etime - stime);
    }
    TileFiles.SetBackup();

    InitView();
    InitFX();
    seq_LoadSequences();
    InitStatus();
    
    for (i = 0; i < kMaxPlayers; i++) {
        nPlayerLives[i] = kDefaultLives;
    }
    resettiming();
    GrabPalette();

    enginecompatibility_mode = ENGINECOMPATIBILITY_19950829;
}

void mychangespritesect(int nSprite, int nSector)
{
    DoKenTest();
    changespritesect(nSprite, nSector);
    DoKenTest();
}

void mydeletesprite(int nSprite)
{
    if (nSprite < 0 || nSprite > kMaxSprites) {
        I_Error("bad sprite value %d handed to mydeletesprite", nSprite);
    }

    deletesprite(nSprite);

    if (nSprite == besttarget) {
        besttarget = -1;
    }
}



void CopyTileToBitmap(short nSrcTile,  short nDestTile, int xPos, int yPos)
{
    int nOffs = tilesiz[nDestTile].y * xPos;

	auto pixels = TileFiles.tileMakeWritable(nDestTile);
    uint8_t *pDest = pixels + nOffs + yPos;
    uint8_t *pDestB = pDest;

    tileLoad(nSrcTile);

    int destYSize = tilesiz[nDestTile].y;
    int srcYSize = tilesiz[nSrcTile].y;

    const uint8_t *pSrc = tilePtr(nSrcTile);

    for (int x = 0; x < tilesiz[nSrcTile].x; x++)
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
    return !bRecord && !bPlayback && !paused && !bInDemo && nTotalPlayers == 1;
}

::GameStats GameInterface::getStats()
{
    return { nCreaturesKilled, nCreaturesTotal, 0, 0, leveltime / 30, 0 };
}

::GameInterface* CreateInterface()
{
    return new GameInterface;
}


// This is only the static global data.
static SavegameHelper sgh("exhumed",
    SV(besttarget),
    SV(nCreaturesTotal),
    SV(nCreaturesKilled),
    SV(nFreeze),
    SV(nSnakeCam),
    SV(nLocalSpr),
    SV(nClockVal),  // kTile3603
    SV(nRedTicks),
    SV(nAlarmTicks),
    SV(nButtonColor),
    SV(nEnergyChan),
    SV(lCountDown),
    SV(nEnergyTowers),
    SV(totalmoves),
    SV(nCurBodyNum),
    SV(nBodyTotal),
    SV(bSnakeCam),
    SV(bSlipMode),
    SV(leveltime),
    nullptr);

extern short cPupData[300];
extern uint8_t* Worktile;
extern int lHeadStartClock;
extern short* pPupData;


void SaveTextureState()
{
    auto fw = WriteSavegameChunk("texture");
    int pupOffset = pPupData? int(pPupData - cPupData) : -1;

    // There is really no good way to restore these tiles, so it's probably best to save them as well, so that they can be reloaded with the exact state they were left in
    fw->Write(&pupOffset, 4);
    uint8_t loaded = !!Worktile;
    fw->Write(&loaded, 1);
    if (Worktile) fw->Write(Worktile, WorktileSize);
    auto pixels = TileFiles.tileMakeWritable(kTile3603);
    fw->Write(pixels, tilesiz[kTile3603].x * tilesiz[kTile3603].y);
    pixels = TileFiles.tileMakeWritable(kEnergy1);
    fw->Write(pixels, tilesiz[kEnergy1].x * tilesiz[kEnergy1].y);
    pixels = TileFiles.tileMakeWritable(kEnergy2);
    fw->Write(pixels, tilesiz[kEnergy2].x * tilesiz[kEnergy2].y);
    
}

void LoadTextureState()
{
    auto fr = ReadSavegameChunk("texture");
    int pofs;
    fr.Read(&pofs, 4);
    pPupData = pofs == -1 ? nullptr : cPupData + pofs;
    uint8_t loaded;
    fr.Read(&loaded, 1);
    if (loaded)
    {
        Worktile = TileFiles.tileCreate(kTileRamsesWorkTile, kSpiritX * 2, kSpiritY * 2);
        fr.Read(Worktile, WorktileSize);
    }
    auto pixels = TileFiles.tileMakeWritable(kTile3603);
    fr.Read(pixels, tilesiz[kTile3603].x * tilesiz[kTile3603].y);
    pixels = TileFiles.tileMakeWritable(kEnergy1);
    fr.Read(pixels, tilesiz[kEnergy1].x * tilesiz[kEnergy1].y);
    pixels = TileFiles.tileMakeWritable(kEnergy2);
    fr.Read(pixels, tilesiz[kEnergy2].x * tilesiz[kEnergy2].y);
    TileFiles.InvalidateTile(kTileRamsesWorkTile);
    TileFiles.InvalidateTile(kTile3603);
    TileFiles.InvalidateTile(kEnergy1);
    TileFiles.InvalidateTile(kEnergy2);
}


CCMD(endit)
{
    LevelFinished();
}
END_PS_NS
