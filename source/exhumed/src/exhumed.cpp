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
#include "baselayer.h"
#include "common.h"
#include "engine.h"
#include "exhumed.h"
#include "sequence.h"
#include "names.h"
#include "menu.h"
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
#include "core/menu/menu.h"

BEGIN_PS_NS


    extern const char* s_buildRev;
    extern const char* s_buildTimestamp;


void uploadCinemaPalettes();
int32_t registerosdcommands(void);
void registerinputcommands();
void InitFonts();

int htimer = 0;
int EndLevel = false;


PlayerInput localInput;

////////

void ResetEngine()
{
    EraseScreen(-1);

    resettiming();

    totalclock  = 0;
    ototalclock = totalclock;
    localclock  = (int)totalclock;

    numframes = 0;
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
    videoInit();

    enginecompatibility_mode = ENGINECOMPATIBILITY_19950829;
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
void mysetbrightness(char al);

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
short fps;
short nRedTicks;
short bInMove;
short nAlarmTicks;
short nButtonColor;
short nEnergyChan;


short bModemPlay = false;
int lCountDown = 0;
short nEnergyTowers = 0;

short nCfgNetPlayers = 0;
FILE *vcrfp = NULL;

int lLocalButtons = 0;
int lLocalCodes = 0;

short bCoordinates = false;

int nNetTime = -1;

short nCodeMin = 0;
short nCodeMax = 0;
short nCodeIndex = 0;

short levelnum = -1;
//short nScreenWidth = 320;
//short nScreenHeight = 200;
int moveframes;
int flash;
int localclock;
int totalmoves;

short nCurBodyNum = 0;

short nBodyTotal = 0;

short lastfps;

short nMapMode = 0;

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

ClockTicks tclocks;

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

void timerhandler()
{
    UpdateSounds();
    scan_char++;
    if (scan_char == kTimerTicks)
    {
        scan_char = 0;
        lastfps = fps;
        fps = 0;
    }

    if (!bInMove) {
        C_RunDelayedCommands();
    }
}

void HandleAsync()
{
    handleevents();

}

static bool HollyCheat(cheatseq_t* c)
{
    // Do the closest thing to this cheat that's available.
    C_ToggleConsole();
    return true;
}

static bool KimberlyCheat(cheatseq_t* c)
{
    Printf(PRINT_NOTIFY, "%s\n", GStrings("TXT_EX_SWEETIE"));
    return true;
}

static bool CopCheat(cheatseq_t* c)
{
    lLocalCodes |= kButtonCheatGuns;
    return true;
}

static bool GodCheat(cheatseq_t* c)
{
    lLocalCodes |= kButtonCheatGodMode;
    return true;
}

static bool LiteCheat(cheatseq_t* c)
{
    Printf(PRINT_NOTIFY, "%s\n", GStrings("TXT_EX_FLASHES"));
    bDoFlashes = !bDoFlashes;
    return true;
}

static bool KeyCheat(cheatseq_t* c)
{
    lLocalCodes |= kButtonCheatKeys;
    return true;
}

bool SlipCheat(cheatseq_t* c)
{
    if (!nNetPlayerCount)
    {
        if (bSlipMode == false)
        {
            bSlipMode = true;
            Printf(PRINT_NOTIFY, "%s\n", GStrings("TXT_EX_SLIPON"));
        }
        else {
            bSlipMode = false;
            Printf(PRINT_NOTIFY, "%s\n", GStrings("TXT_EX_SLIPOFF"));
        }
    }
    return true;
}

static bool SnakeCheat(cheatseq_t* c)
{
    if (!nNetPlayerCount)
    {
        if (bSnakeCam == false)
        {
            bSnakeCam = true;
            Printf(PRINT_NOTIFY, "%s\n", GStrings("TXT_EX_SNAKEON"));
        }
        else {
            bSnakeCam = false;
            Printf(PRINT_NOTIFY, "%s\n", GStrings("TXT_EX_SNAKEOFF"));
        }
    }
    return true;
}

static bool SphereCheat(cheatseq_t* c)
{
    Printf(PRINT_NOTIFY, "%s\n", GStrings("TXT_EX_FULLMAP"));
    GrabMap();
    bShowTowers = true;
    return true;
}

static bool SwagCheat(cheatseq_t* c)
{
    lLocalCodes |= kButtonCheatItems;
    return true;
}

static bool CoordCheat(cheatseq_t* c)
{
    C_DoCommand("stat printcoords");
    return true;
}


static cheatseq_t excheats[] = {
    {"holly",       HollyCheat, 0},
    {"kimberly",    KimberlyCheat, 0},
    {"lobocop",     CopCheat, 0},
    {"lobodeity",   GodCheat, 0},
    {"lobolite",    LiteCheat, 0},
    {"lobopick",    KeyCheat, 0},
    {"loboslip",    SlipCheat, 0},
    {"lobosnake",   SnakeCheat, 0},
    {"lobosphere",  SphereCheat, 0},
    {"loboswag",    SwagCheat, 0},
    {"loboxy",      CoordCheat, true},
};


void mysetbrightness(char nBrightness)
{
    g_visibility = 2048 - (nBrightness << 9);
}

// Replicate original DOS EXE behaviour when pointer is null
static const char *safeStrtok(char *s, const char *d)
{
    const char *r = strtok(s, d);
    return r ? r : "";
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

int32_t calc_smoothratio(ClockTicks totalclk, ClockTicks ototalclk)
{
    if (bRecord || bPlayback || nFreeze != 0 || bCamera || paused)
        return 65536;

    return CalcSmoothRatio(totalclk, ototalclk, 30);
}

FString GameInterface::statFPS()
{
    FString out;
    static int32_t frameCount;
    static double cumulativeFrameDelay;
    static double lastFrameTime;
    static float lastFPS; // , minFPS = std::numeric_limits<float>::max(), maxFPS;
    //static double minGameUpdate = std::numeric_limits<double>::max(), maxGameUpdate;

    double frameTime = timerGetHiTicks();
    double frameDelay = frameTime - lastFrameTime;
    cumulativeFrameDelay += frameDelay;

    if (frameDelay >= 0)
    {
       out.Format("%.1f ms, %5.1f fps", frameDelay, lastFPS);

        if (cumulativeFrameDelay >= 1000.0)
        {
            lastFPS = 1000.f * frameCount / cumulativeFrameDelay;
            // g_frameRate = Blrintf(lastFPS);
            frameCount = 0;
            cumulativeFrameDelay = 0.0;
        }
        frameCount++;
    }
    lastFrameTime = frameTime;
    return out;
}

void GameMove(void)
{
    FixPalette();

    if (levelnum == kMap20)
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
    moveframes--;
}


void GameTicker()
{
    bInMove = true;

    if (paused)
    {
        tclocks = totalclock - 4;
        buttonMap.ResetButtonStates();
    }
    else
    {
        while ((totalclock - ototalclock) >= 1 || !bInMove)
        {
            ototalclock = ototalclock + 1;

            if (!((int)ototalclock & 3) && moveframes < 4)
                moveframes++;

            GetLocalInput();
            PlayerInterruptKeys();

            nPlayerDAng = fix16_sadd(nPlayerDAng, localInput.nAngle);
            inita &= kAngleMask;

            lPlayerXVel += localInput.yVel * Cos(inita) + localInput.xVel * Sin(inita);
            lPlayerYVel += localInput.yVel * Sin(inita) - localInput.xVel * Cos(inita);
            lPlayerXVel -= (lPlayerXVel >> 5) + (lPlayerXVel >> 6);
            lPlayerYVel -= (lPlayerYVel >> 5) + (lPlayerYVel >> 6);

            sPlayerInput[nLocalPlayer].xVel = lPlayerXVel;
            sPlayerInput[nLocalPlayer].yVel = lPlayerYVel;
            sPlayerInput[nLocalPlayer].buttons = lLocalButtons | lLocalCodes;
            sPlayerInput[nLocalPlayer].nAngle = nPlayerDAng;
            sPlayerInput[nLocalPlayer].nTarget = besttarget;

            Ra[nLocalPlayer].nTarget = besttarget;

            lLocalCodes = 0;
            nPlayerDAng = 0;

            sPlayerInput[nLocalPlayer].horizon = PlayerList[nLocalPlayer].q16horiz;

            while (!EndLevel && totalclock >= tclocks + 4)
            {
                tclocks += 4;
                leveltime++;
                GameMove();
            }
        }
        if (nPlayerLives[nLocalPlayer] <= 0) {
            startmainmenu();
        }
    }
    bInMove = false;


}
int32_t r_maxfpsoffset = 0;


void ExitGame()
{
    ShutDown();
    throw CExitEvent(0);
}

void InitTimer()
{
    htimer = 1;

    timerInit(kTimerTicks);
    timerSetCallback(timerhandler);
}

static const char* actions[] =
{
    "Move_Forward",
    "Move_Backward",
    "Turn_Left",
    "Turn_Right",
    "Strafe",
    "Fire",
    "Open",
    "Run",
    "Alt_Fire",	// Duke3D", Blood
    "Jump",
    "Crouch",
    "Look_Up",
    "Look_Down",
    "Look_Left",
    "Look_Right",
    "Strafe_Left",
    "Strafe_Right",
    "Aim_Up",
    "Aim_Down",
    "SendMessage",
    "Map",
    "Shrink_Screen",
    "Enlarge_Screen",
    "Show_Opponents_Weapon",
    "Map_Follow_Mode",
    "See_Coop_View",
    "Mouse_Aiming",
    "Dpad_Select",
    "Dpad_Aiming",
    "Last_Weapon",
    "Alt_Weapon",
    "Third_Person_View",
    "Toggle_Crouch",	// This is the last one used by EDuke32.
};




void InitGame()
{
    int i;
    //int esi = 1;
    //int edi = esi;

    buttonMap.SetButtons(actions, NUM_ACTIONS);

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

    SetCheats(excheats, countof(excheats));
    registerosdcommands();
    registerinputcommands();
    if (nNetPlayerCount == -1)
    {
        nNetPlayerCount = nCfgNetPlayers - 1;
        nTotalPlayers += nNetPlayerCount;
    }

    // temp - moving InstallEngine(); before FadeOut as we use nextpage() in FadeOut
    InstallEngine();

    const char* defsfile = G_DefFile();
    uint32_t stime = timerGetTicks();
    if (!loaddefinitionsfile(defsfile))
    {
        uint32_t etime = timerGetTicks();
        Printf("Definitions file \"%s\" loaded in %d ms.\n", defsfile, etime - stime);
    }


    enginePostInit();

    InitView();
    InitFX();
    seq_LoadSequences();
    InitStatus();
    InitTimer();

    for (i = 0; i < kMaxPlayers; i++) {
        nPlayerLives[i] = kDefaultLives;
    }

    ResetEngine();
    ResetView();
    GrabPalette();
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
    SV(levelnum),
    SV(moveframes),
    SV(totalmoves),
    SV(nCurBodyNum),
    SV(nBodyTotal),
    SV(bSnakeCam),
    SV(bSlipMode),
    SV(localclock),
    SV(tclocks),
    SV(totalclock),
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

END_PS_NS
