//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#include "ns.h"
// CTW NOTE
/*
Known remaining issues:
- Audio stuttering.
- CD Audio not looping properly (currently hard coded to restart about every 200 seconds.
- Hitting F5 to change resolution causes a crash (currently disabled).
- Multiplayer untested.

Things required to make savegames work:
- Load makesym.wpj and build it.
- In a DOS prompt, run "makesym sw.map swdata.map swcode.map"
- Copy swcode.map to swcode.sym and swdata.map to swdata.sym
*/
// CTW NOTE END

#define MAIN
#define QUIET
#include "build.h"
#include "baselayer.h"

#include "osd.h"
#include "baselayer.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "sector.h"
#include "sprite.h"
#include "weapon.h"
#include "player.h"
#include "lists.h"
#include "network.h"
#include "pal.h"


#include "mytypes.h"
//#include "config.h"

#include "menus.h"

#include "gamecontrol.h"
#include "gamedefs.h"
#include "config.h"

#include "demo.h"
#include "cache.h"
//#include "exports.h"

#include "anim.h"

#include "colormap.h"
#include "break.h"
#include "ninja.h"
#include "light.h"
#include "track.h"
#include "jsector.h"
#include "text.h"

#include "common.h"
#include "common_game.h"
#include "gameconfigfile.h"
#include "printf.h"
#include "m_argv.h"
#include "debugbreak.h"
#include "menu/menu.h"
#include "raze_music.h"
#include "statistics.h"
#include "gstrings.h"
#include "mapinfo.h"
#include "rendering/v_video.h"
#include "raze_sound.h"
#include "secrets.h"

#include "osdcmds.h"

//#include "crc32.h"

BEGIN_SW_NS

void pClearSpriteList(PLAYERp pp);
signed char MNU_InputSmallString(char*, short);
signed char MNU_InputString(char*, short);
SWBOOL IsCommand(const char* str);
SWBOOL MNU_StartNetGame(void);
extern SWBOOL mapcheat;

extern SWBOOL MultiPlayQuitFlag;

extern int sw_snd_scratch;


#if DEBUG
#define BETA 0
#endif

#define STAT_SCREEN_PIC 5114
#define TITLE_PIC 2324
#define THREED_REALMS_PIC 2325
#define TITLE_ROT_FLAGS (ROTATE_SPRITE_CORNER|ROTATE_SPRITE_SCREEN_CLIP|ROTATE_SPRITE_NON_MASK)
#define PAL_SIZE (256*3)

char DemoName[15][16];

// Stupid WallMart version!
//#define PLOCK_VERSION TRUE

#if PLOCK_VERSION
SWBOOL Global_PLock = TRUE;
#else
SWBOOL Global_PLock = FALSE;
#endif

// 12 was original source release. For future releases increment by two.
int GameVersion = 15;

char DemoText[3][64];
int DemoTextYstart = 0;

SWBOOL DoubleInitAWE32 = FALSE;

SWBOOL NoMeters = FALSE;
short IntroAnimCount = 0;
short PlayingLevel = -1;
SWBOOL GraphicsMode = FALSE;
char CacheLastLevel[32] = "";
char PlayerNameArg[32] = "";
SWBOOL CleanExit = FALSE;
SWBOOL DemoModeMenuInit = FALSE;
SWBOOL FinishAnim = 0;
SWBOOL ShortGameMode = FALSE;
SWBOOL ReloadPrompt = FALSE;
SWBOOL ReloadPromptMode = FALSE;
SWBOOL NewGame = TRUE;
SWBOOL InMenuLevel = FALSE;
SWBOOL LoadGameOutsideMoveLoop = FALSE;
SWBOOL LoadGameFromDemo = FALSE;
SWBOOL ArgCheat = FALSE;
extern SWBOOL NetBroadcastMode, NetModeOverride;
SWBOOL MultiPlayQuitFlag = FALSE;
//Miscellaneous variables
char MessageInputString[256];
char MessageOutputString[256];
SWBOOL MessageInputMode = FALSE;
SWBOOL ConInputMode = FALSE;
SWBOOL ConPanel = FALSE;
SWBOOL FinishedLevel = FALSE;
SWBOOL HelpInputMode = FALSE;
SWBOOL PanelUpdateMode = TRUE;
short HelpPage = 0;
short HelpPagePic[] = { 5115, 5116, 5117 };
SWBOOL InputMode = FALSE;
SWBOOL MessageInput = FALSE;
extern SWBOOL GamePaused;
short screenpeek = 0;
SWBOOL NoDemoStartup = FALSE;
SWBOOL FirstTimeIntoGame;

SWBOOL BorderAdjust = FALSE;
SWBOOL LocationInfo = 0;
void drawoverheadmap(int cposx, int cposy, int czoom, short cang);
int DispFrameRate = FALSE;
int DispMono = TRUE;
int Fog = FALSE;
int FogColor;
SWBOOL PreCaching = TRUE;
int GodMode = FALSE;
SWBOOL BotMode = FALSE;
short Skill = 2;
short BetaVersion = 900;
short TotalKillable;

AUTO_NET Auto;
SWBOOL AutoNet = FALSE;
SWBOOL HasAutoColor = FALSE;
uint8_t AutoColor;

const GAME_SET gs_defaults =
{
    2, // border
    0, // border tile

// Network game settings
    0, // GameType
    0, // Level
    0, // Monsters
    FALSE, // HurtTeammate
    TRUE, // SpawnMarkers Markers
    FALSE, // TeamPlay
    0, // Kill Limit
    0, // Time Limit
    0, // Color
    TRUE, // nuke
};
GAME_SET gs;

SWBOOL PlayerTrackingMode = FALSE;
SWBOOL PauseMode = FALSE;
SWBOOL PauseKeySet = FALSE;
SWBOOL SlowMode = FALSE;
SWBOOL FrameAdvanceTics = 3;
SWBOOL ScrollMode2D = FALSE;

SWBOOL DebugSO = FALSE;
SWBOOL DebugPanel = FALSE;
SWBOOL DebugSector = FALSE;
SWBOOL DebugActor = FALSE;
SWBOOL DebugAnim = FALSE;
SWBOOL DebugOperate = FALSE;
SWBOOL DebugActorFreeze = FALSE;
void LoadingLevelScreen(void);

uint8_t FakeMultiNumPlayers;

int totalsynctics;
int turn_scale = 256;
int move_scale = 256;

short Level = 0;
SWBOOL ExitLevel = FALSE;
int16_t OrigCommPlayers=0;
extern uint8_t CommPlayers;
extern SWBOOL CommEnabled;
extern int bufferjitter;

SWBOOL CameraTestMode = FALSE;

char ds[645];                           // debug string

extern short NormalVisibility;

extern int quotebot, quotebotgoal;     // Multiplayer typing buffer
char recbuf[80];                        // Used as a temp buffer to hold typing text

#define ACT_STATUE 0

int score;
SWBOOL QuitFlag = FALSE;
SWBOOL InGame = FALSE;

SWBOOL CommandSetup = FALSE;

char UserMapName[80]="", buffer[80], ch;
char LevelName[20];

uint8_t DebugPrintColor = 255;

int krandcount;

SW_PACKET localInput;

/// L O C A L   P R O T O T Y P E S /////////////////////////////////////////////////////////
void BOT_DeleteAllBots(void);
void BotPlayerInsert(PLAYERp pp);
void SybexScreen(void);
void PlayTheme(void);
void MenuLevel(void);
void StatScreen(PLAYERp mpp);
void InitRunLevel(void);
void RunLevel(void);
void getinput(int playerNum);
/////////////////////////////////////////////////////////////////////////////////////////////

static FILE *debug_fout = NULL;

void DebugWriteString(char *string)
{

#if BETA || !DEBUG
    return;
#endif

    if (!debug_fout)
    {
        if ((debug_fout = fopen("dbg.foo", "ab+")) == NULL)
            return;
    }

    fprintf(debug_fout, "%s\n", string);

    //fclose(debug_fout);
    //debug_fout = NULL;

    fflush(debug_fout);
}

void DebugWriteLoc(char *fname, int line)
{

#if BETA || !DEBUG
    return;
#endif

    if (!debug_fout)
    {
        if ((debug_fout = fopen("dbg.foo", "ab+")) == NULL)
            return;
    }

    fprintf(debug_fout, "%s, %d\n", fname, line);

    //fclose(debug_fout);
    //debug_fout = NULL;

    fflush(debug_fout);
}

void Mono_Print(char *str)
{
    MONO_PRINT(str);
}


extern SWBOOL DrawScreen;
#if RANDOM_DEBUG
FILE *fout_err;
SWBOOL RandomPrint;
int krand1(char *file, unsigned line)
{
    ASSERT(!DrawScreen);
    if (RandomPrint && !Prediction)
    {
        extern uint32_t MoveThingsCount;
        sprintf(ds,"mtc %d, %s, line %d, %d",MoveThingsCount,file,line,randomseed);
        DebugWriteString(ds);
    }
    randomseed = ((randomseed * 21 + 1) & 65535);
    return randomseed;
}

int krand2()
{
    ASSERT(!DrawScreen);
    randomseed = ((randomseed * 21 + 1) & 65535);
    return randomseed;
}

#else
int krand1(void)
{
    ASSERT(!DrawScreen);
    krandcount++;
    randomseed = ((randomseed * 21 + 1) & 65535);
    return randomseed;
}

#endif

#if DEBUG
SWBOOL
ValidPtr(void *ptr)
{
    MEM_HDRp mhp;
    uint8_t* check;

    ASSERT(ptr != NULL);

    mhp = (MEM_HDRp)(((uint8_t*) ptr) - sizeof(MEM_HDR));

    if (mhp->size == 0 || mhp->checksum == 0)
    {
        printf("ValidPtr(): Size or Checksum == 0!\n");
        return FALSE;
    }

    check = (uint8_t*) & mhp->size;

    if (mhp->checksum == check[0] + check[1] + check[2] + check[3])
        return TRUE;

    printf("ValidPtr(): Checksum bad!\n");
    return FALSE;
}

void
PtrCheckSum(void *ptr, unsigned int *stored, unsigned int *actual)
{
    MEM_HDRp mhp;
    uint8_t* check;

    ASSERT(ptr != NULL);

    mhp = (MEM_HDRp)(((uint8_t*) ptr) - sizeof(MEM_HDR));

    check = (uint8_t*) & mhp->size;

    *stored = mhp->checksum;
    *actual = check[0] + check[1] + check[2] + check[3];
}

void *
AllocMem(int size)
{
    uint8_t* bp;
    MEM_HDRp mhp;
    uint8_t* check;

    ASSERT(size != 0);

    bp = (uint8_t*) malloc(size + sizeof(MEM_HDR));

    // Used for debugging, we can remove this at ship time
    if (bp == NULL)
    {
        I_FatalError("Memory could NOT be allocated in AllocMem: size = %d\n",size);
    }

    ASSERT(bp != NULL);

    mhp = (MEM_HDRp) bp;

    mhp->size = size;
    check = (uint8_t*) & mhp->size;
    mhp->checksum = check[0] + check[1] + check[2] + check[3];

    bp += sizeof(MEM_HDR);

    return bp;
}

void *
ReAllocMem(void *ptr, int size)
{
    if (ptr == nullptr)
        return AllocMem(size);

    if (size == 0)
    {
        FreeMem(ptr);
        return nullptr;
    }

    uint8_t* bp;
    MEM_HDRp mhp;
    uint8_t* check;

    ASSERT(ValidPtr(ptr));

    mhp = (MEM_HDRp)(((uint8_t*) ptr) - sizeof(MEM_HDR));

    bp = (uint8_t*) realloc(mhp, size + sizeof(MEM_HDR));

    ASSERT(bp != NULL);

    mhp = (MEM_HDRp) bp;

    mhp->size = size;
    check = (uint8_t*) & mhp->size;
    mhp->checksum = check[0] + check[1] + check[2] + check[3];

    bp += sizeof(MEM_HDR);

    ASSERT(ValidPtr(bp));

    return bp;
}


void *
CallocMem(int size, int num)
{
    uint8_t* bp;
    MEM_HDRp mhp;
    uint8_t* check;
    int num_bytes;

    ASSERT(size != 0 && num != 0);

    num_bytes = (size * num) + sizeof(MEM_HDR);
    bp = (uint8_t*) calloc(num_bytes, 1);

    // Used for debugging, we can remove this at ship time
    if (bp == NULL)
    {
        I_FatalError("Memory could NOT be allocated in CallocMem: size = %d, num = %d\n",size,num);
    }

    ASSERT(bp != NULL);

    mhp = (MEM_HDRp) bp;

    mhp->size = size;
    check = (uint8_t*) & mhp->size;
    mhp->checksum = check[0] + check[1] + check[2] + check[3];

    bp += sizeof(MEM_HDR);

    return bp;
}

void
FreeMem(void *ptr)
{
    if (ptr == nullptr)
        return;

    MEM_HDRp mhp;
    uint8_t* check;

    ASSERT(ValidPtr(ptr));

    mhp = (MEM_HDRp)(((uint8_t*) ptr) - sizeof(MEM_HDR));
    check = (uint8_t*)&mhp->size;

    memset(mhp, 0xCC, mhp->size + sizeof(MEM_HDR));

    free(mhp);
}

#endif

int PointOnLine(int x, int y, int x1, int y1, int x2, int y2)
{
    // the closer to 0 the closer to the line the point is
    return ((x2 - x1) * (y - y1)) - ((y2 - y1) * (x - x1));
}

int
Distance(int x1, int y1, int x2, int y2)
{
    int min;

    if ((x2 = x2 - x1) < 0)
        x2 = -x2;

    if ((y2 = y2 - y1) < 0)
        y2 = -y2;

    if (x2 > y2)
        min = y2;
    else
        min = x2;

    return x2 + y2 - DIV2(min);
}

void
setup2dscreen(void)
{
    // qsetmode640350();
}



void TerminateGame(void)
{
    DemoTerm();

    ErrorCorrectionQuit();

 //   uninitmultiplayers();

    if (CleanExit)
    {
        SybexScreen();
        //TenScreen();
    }
    throw CExitEvent(3);
}

bool LoadLevel(const char *filename)
{
    int16_t ang;
    if (engineLoadBoard(filename, SW_SHAREWARE ? 1 : 0, (vec3_t *)&Player[0], &ang, &Player[0].cursectnum) == -1)
    {
		Printf("Level not found: %s", filename);
		return false;
        }
	currentLevel = &mapList[Level];
    SECRET_SetMapName(currentLevel->DisplayName(), currentLevel->name);
    STAT_NewLevel(currentLevel->labelName);
    Player[0].q16ang = fix16_from_int(ang);
	return true;
}

void LoadDemoRun(void)
{
    short i;
    FILE *fin;

    fin = fopen("demos.run","r");
    if (fin)
    {
        memset(DemoName,'\0',sizeof(DemoName));
        for (i = 0; TRUE; i++)
        {
            if (fscanf(fin, "%s", DemoName[i]) == EOF)
                break;
        }

        fclose(fin);
    }

    memset(DemoText,'\0',sizeof(DemoText));
    fin = fopen("demotxt.run","r");
    if (fin)
    {
        fgets(ds, 6, fin);
        sscanf(ds,"%d",&DemoTextYstart);
        for (i = 0; TRUE; i++)
        {
            if (fgets(DemoText[i], SIZ(DemoText[0])-1, fin) == NULL)
                break;
        }

        fclose(fin);
    }
}

void DisplayDemoText(void)
{
    short w,h;
    short i;

    for (i = 0; i < 3; i++)
    {
        MNU_MeasureString(DemoText[i], &w, &h);
        PutStringTimer(Player, TEXT_TEST_COL(w), DemoTextYstart+(i*12), DemoText[i], 999);
    }
}

void MultiSharewareCheck(void)
{
    if (!SW_SHAREWARE) return;
    if (numplayers > 4)
    {
        I_FatalError("To play a Network game with more than 4 players you must purchase "
                  "the full version.  Read the Ordering Info screens for details.");
    }
}


// Some mem crap for Jim
// I reserve 1 meg of heap space for our use out side the cache
int TotalMemory = 0;
int ActualHeap = 0;

void InitAutoNet(void)
{
    if (!AutoNet)
        return;

    gs.NetGameType      = Auto.Rules;
    gs.NetLevel         = Auto.Level;
    gs.NetMonsters      = Auto.Enemy;
    gs.NetSpawnMarkers  = Auto.Markers;
    gs.NetTeamPlay      = Auto.Team;
    gs.NetHurtTeammate  = Auto.HurtTeam;
    gs.NetKillLimit     = Auto.Kill;
    gs.NetTimeLimit     = Auto.Time;
    gs.NetColor         = Auto.Color;
    gs.NetNuke          = Auto.Nuke;
}


void AnimateCacheCursor(void)
{
#if 0
    struct rccoord old_pos;
    static short cursor_num = 0;
    static char cache_cursor[] =  {'|','/','-','\\'};

    if (GraphicsMode)
        return;

    cursor_num++;
    if (cursor_num > 3)
        cursor_num = 0;

    //old_pos = _gettextposition();
    //_settextposition( old_pos.row, old_pos.col );
    //_settextposition( 24,  25);
    _settextposition(25,  0);
    sprintf(ds,"Loading sound and graphics %c", cache_cursor[cursor_num]);
    _outtext(ds);
    //_settextposition( old_pos.row, old_pos.col );
#endif
}

static int firstnet = 0;    // JBF


bool InitGame()
{
    extern int MovesPerPacket;
    //void *ReserveMem=NULL;
    int i;

    DSPRINTF(ds,"InitGame...");
    MONO_PRINT(ds);

    engineInit();

    InitAutoNet();

    {
        auto pal = fileSystem.LoadFile("3drealms.pal", 0);
        if (pal.Size() >= 768)
        {
            for (auto& c : pal)
                c <<= 2;

            paletteSetColorTable(DREALMSPAL, pal.Data(), true, true);
        }
    }


    timerInit(120);

    InitPalette();
    // sets numplayers, connecthead, connectpoint2, myconnectindex

#if 0
    if (!firstnet)
        initmultiplayers(0, NULL, 0, 0, 0);
    else if (initmultiplayersparms(argc - firstnet, &argv[firstnet]))
    {
        Printf("Waiting for players...\n");
        while (initmultiplayerscycle())
        {
            handleevents();
            }
        }
#else
	numplayers = 1; myconnectindex = 0;
	connecthead = 0; connectpoint2[0] = -1;
#endif
    initsynccrc();

    // code to duplicate packets
    if (numplayers > 4 && MovesPerPacket == 1)
    {
        MovesPerPacket = 2;
    }

    MultiSharewareCheck();

    if (numplayers > 1)
    {
        CommPlayers = numplayers;
        OrigCommPlayers = CommPlayers;
        CommEnabled = TRUE;
        if (!BotMode)
            gNet.MultiGameType = MULTI_GAME_COMMBAT;
        else
            gNet.MultiGameType = MULTI_GAME_AI_BOTS;

#if 0 //def NET_MODE_MASTER_SLAVE
        if (!NetModeOverride)
        {
            if (numplayers <= 4)
                NetBroadcastMode = TRUE;
            else
                NetBroadcastMode = FALSE;
        }
#endif
    }

    LoadDemoRun();
    // Save off total heap for later calculations
    //TotalMemory = Z_AvailHeap();
    //DSPRINTF(ds,"Available Heap before LoadImages =  %d", TotalMemory);
    //MONO_PRINT(ds);
    // Reserve 1.5 megs for normal program use
    // Generally, SW is consuming about a total of 11 megs including
    // all the cached in graphics, etc. per level, so even on a 16 meg
    // system, reserving 1.5 megs is fine.
    // Note that on a 16 meg machine, Ken was leaving us about
    // 24k for use outside the cache!  This was causing out of mem problems
    // when songs, etc., greater than the remaining heap were being loaded.
    // Even if you pre-cache songs, etc. to help, reserving some heap is
    // a very smart idea since the game uses malloc throughout execution.
    //ReserveMem = AllocMem(1L<<20);
    //if(ReserveMem == 0) MONO_PRINT("Could not allocate 1.5 meg reserve!");

    // LoadImages will now proceed to steal all the remaining heap space
    //_outtext("\n\n\n\n\n\n\n\n");
    //AnimateCacheCursor();
	TileFiles.LoadArtSet("tiles%03d.art");

    // Now free it up for later use
    /*
    if(ReserveMem)
        {
        // Recalc TotalMemory for later reference
        ActualHeap = Z_AvailHeap() + 1536000L;
        FreeMem(ReserveMem);
        }
    */

    Connect();
    SortBreakInfo();
    parallaxtype = 1;
    SW_InitMultiPsky();

    memset(Track, 0, sizeof(Track));

    memset(Player, 0, sizeof(Player));
    for (i = 0; i < MAX_SW_PLAYERS; i++)
        INITLIST(&Player[i].PanelSpriteList);

    LoadKVXFromScript("swvoxfil.txt");    // Load voxels from script file
    LoadPLockFromScript("swplock.txt");   // Get Parental Lock setup info
	
	LoadCustomInfoFromScript("engine/swcustom.txt");	// load the internal definitions. These also apply to the shareware version.
    if (!SW_SHAREWARE)
	{
        LoadCustomInfoFromScript("swcustom.txt");   // Load user customisation information
	}
 
    if (!loaddefinitionsfile(G_DefFile())) Printf("Definitions file loaded.\n");

	userConfig.AddDefs.reset();

    enginePostInit();

    paletteFixTranslucencyMask();
    V_Init2();

    DemoModeMenuInit = TRUE;
    // precache as much stuff as you can
    if (UserMapName[0] == '\0')
    {
        AnimateCacheCursor();
        if (!LoadLevel("$dozer.map")) return false;
        AnimateCacheCursor();
        SetupPreCache();
        DoTheCache();
    }
    else
    {
        AnimateCacheCursor();
		if (!LoadLevel(UserMapName)) return false;
        AnimateCacheCursor();
        SetupPreCache();
        DoTheCache();
    }

    GraphicsMode = TRUE;
    SetupAspectRatio();

    InitFX();   // JBF: do it down here so we get a hold of the window handle
	return true;
}


/*
Directory of C:\DEV\SW\MIDI
EXECUT11 MID
HROSHMA6 MID
HOSHIA02 MID
INTRO131 MID
KOTEC2   MID
KOTOKI12 MID
NIPPON34 MID
NOKI41   MID
SANAI    MID
SIANRA23 MID
TKYO2007 MID
TYTAIK16 MID
YOKOHA03 MID
*/

short SongLevelNum;

FString ThemeSongs[6];
int ThemeTrack[6];

void InitNewGame(void)
{
    int i, ready_bak;
    int ver_bak;

    //waitforeverybody();           // since ready flag resets after this point, need to carefully sync

    for (i = 0; i < MAX_SW_PLAYERS; i++)
    {
        // don't jack with the playerreadyflag
        ready_bak = Player[i].playerreadyflag;
        ver_bak = Player[i].PlayerVersion;
        memset(&Player[i], 0, sizeof(Player[i]));
        Player[i].playerreadyflag = ready_bak;
        Player[i].PlayerVersion = ver_bak;
        INITLIST(&Player[i].PanelSpriteList);
    }

    memset(puser, 0, sizeof(puser));
}

void FindLevelInfo(char *map_name, short *level)
{
    short j;

    for (j = 1; j <= MAX_LEVELS; j++)
    {
        if (Bstrcasecmp(map_name, mapList[j].fileName.GetChars()) == 0)
        {
                *level = j;
                return;
            }
        }

    *level = 0;
    return;
}

int ChopTics;
void InitLevelGlobals(void)
{
    extern char PlayerGravity;
    extern short wait_active_check_offset;
    //extern short Zombies;
    extern int PlaxCeilGlobZadjust, PlaxFloorGlobZadjust;
    extern SWBOOL left_foot;
    extern SWBOOL serpwasseen;
    extern SWBOOL sumowasseen;
    extern SWBOOL zillawasseen;
    extern short BossSpriteNum[3];

    ChopTics = 0;
    dimensionmode = 3;
    zoom = 768;
    PlayerGravity = 24;
    wait_active_check_offset = 0;
    PlaxCeilGlobZadjust = PlaxFloorGlobZadjust = Z(500);
    FinishedLevel = FALSE;
    AnimCnt = 0;
    left_foot = FALSE;
    screenpeek = myconnectindex;

    gNet.TimeLimitClock = gNet.TimeLimit;

    serpwasseen = FALSE;
    sumowasseen = FALSE;
    zillawasseen = FALSE;
    memset(BossSpriteNum,-1,sizeof(BossSpriteNum));
}

void InitLevelGlobals2(void)
{
    extern short Bunny_Count;
    // GLOBAL RESETS NOT DONE for LOAD GAME
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    InitTimingVars();
    TotalKillable = 0;
    Bunny_Count = 0;
}

void
InitLevel(void)
{
    if (LoadGameOutsideMoveLoop)
    {
        InitLevelGlobals();
        return;
    }

    static int DemoNumber = 0;

    Terminate3DSounds();

    // A few IMPORTANT GLOBAL RESETS
    InitLevelGlobals();

    if (!DemoMode)
        Mus_Stop();

    InitLevelGlobals2();
    if (DemoMode)
    {
        Level = 0;
        NewGame = TRUE;
        DemoInitOnce = FALSE;
        strcpy(DemoFileName, DemoName[DemoNumber]);
        DemoNumber++;
        if (!DemoName[DemoNumber][0])
            DemoNumber = 0;

        // read header and such
        DemoPlaySetup();

        strcpy(LevelName, DemoLevelName);

        FindLevelInfo(LevelName, &Level);
        if (Level > 0)
        {
            strcpy(LevelName, mapList[Level].fileName);
            UserMapName[0] = '\0';
        }
        else
        {
            strcpy(UserMapName, DemoLevelName);
            Level = 0;
        }

    }
    else
    {
        if (Level < 0)
            Level = 0;

        if (Level > MAX_LEVELS)
            Level = 1;

        // extra code in case something is resetting these values
        if (NewGame)
        {
            //Level = 1;
            //DemoPlaying = FALSE;
            DemoMode = FALSE;
            //DemoRecording = FALSE;
            //DemoEdit = FALSE;
        }

        if (UserMapName[0])
        {
            strcpy(LevelName, UserMapName);

            Level = 0;
            FindLevelInfo(UserMapName, &Level);

            if (Level > 0)
            {
                // user map is part of game - treat it as such
                strcpy(LevelName, mapList[Level].fileName);
                UserMapName[0] = '\0';
            }
        }
        else
        {
            strcpy(LevelName, mapList[Level].fileName);
        }
    }

    PlayingLevel = Level;

    if (NewGame)
        InitNewGame();

    LoadingLevelScreen();
    MONO_PRINT("LoadintLevelScreen");
    if (!DemoMode && !DemoInitOnce)
        DemoPlaySetup();

    if (!LoadLevel(LevelName))
	{
		NewGame = false;
		return;
	}
	STAT_NewLevel(LevelName);

    if (Bstrcasecmp(CacheLastLevel, LevelName) != 0)
        // clears gotpic and does some bit setting
        SetupPreCache();
    else
        memset(gotpic,0,sizeof(gotpic));

    if (sector[0].extra != -1)
    {
        NormalVisibility = g_visibility = sector[0].extra;
        sector[0].extra = 0;
    }
    else
        NormalVisibility = g_visibility;

    //
    // Do Player stuff first
    //

    InitAllPlayers();

#if DEBUG
    // fake Multi-player game setup
    if (FakeMultiNumPlayers && !BotMode)
    {
        uint8_t i;

        // insert all needed players except the first one - its already tere
        for (i = 0; i < FakeMultiNumPlayers - 1; i++)
        {
            ManualPlayerInsert(Player);
            // reset control back to 1st player
            myconnectindex = 0;
            screenpeek = 0;
        }
    }
#endif

    // Put in the BOTS if called for
    if (FakeMultiNumPlayers && BotMode)
    {
        uint8_t i;

        // insert all needed players except the first one - its already tere
        for (i = 0; i < FakeMultiNumPlayers; i++)
        {
            BotPlayerInsert(Player);
            // reset control back to 1st player
            myconnectindex = 0;
            screenpeek = 0;
        }
    }

    QueueReset();
    PreMapCombineFloors();
    InitMultiPlayerInfo();
    InitAllPlayerSprites();

    //
    // Do setup for sprite, track, panel, sector, etc
    //

    // Set levels up
    InitTimingVars();

    SpriteSetup();
    SpriteSetupPost(); // post processing - already gone once through the loop
    InitLighting();

    TrackSetup();

    PlayerPanelSetup();
    SectorSetup();
    JS_InitMirrors();
    JS_InitLockouts();   // Setup the lockout linked lists
    JS_ToggleLockouts(); // Init lockouts on/off

    PlaceSectorObjectsOnTracks();
    PlaceActorsOnTracks();
    PostSetupSectorObject();
    SetupMirrorTiles();
    initlava();

    SongLevelNum = Level;

    if (DemoMode)
    {
        DisplayDemoText();
    }


    if (ArgCheat)
    {
        int bak = hud_messages;
        hud_messages = 0;
        EveryCheatToggle(&Player[0],NULL);
        hud_messages = bak;
        GodMode = TRUE;
    }

    // reset NewGame
    NewGame = FALSE;

    DSPRINTF(ds,"End of InitLevel...");
    MONO_PRINT(ds);

#if 0
#if DEBUG
    if (!cansee(43594, -92986, 0x3fffffff, 290,
                43180, -91707, 0x3fffffff, 290))
    {
        DSPRINTF(ds,"cansee failed");
        MONO_PRINT(ds);
    }
#endif
#endif

}


void
TerminateLevel(void)
{
    int i, nexti, stat, pnum, ndx;
    SECT_USERp *sectu;

//HEAP_CHECK();

    DemoTerm();

    // Free any track points
    for (ndx = 0; ndx < MAX_TRACKS; ndx++)
    {
        if (Track[ndx].TrackPoint)
        {
            FreeMem(Track[ndx].TrackPoint);
            // !JIM! I added null assigner
            Track[ndx].TrackPoint = NULL;
        }
    }

    // Clear the tracks
    memset(Track, 0, sizeof(Track));

    StopFX();
    
    // Clear all anims and any memory associated with them
    // Clear before killing sprites - save a little time
    //AnimClear();

    for (stat = STAT_PLAYER0; stat < STAT_PLAYER0 + numplayers; stat++)
    {

        pnum = stat - STAT_PLAYER0;

        TRAVERSE_SPRITE_STAT(headspritestat[stat], i, nexti)
        {
            if (User[i])
                memcpy(&puser[pnum], User[i], sizeof(USER));
        }
    }

    // Kill User memory and delete sprites
    // for (stat = 0; stat < STAT_ALL; stat++)
    for (stat = 0; stat < MAXSTATUS; stat++)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[stat], i, nexti)
        {
            KillSprite(i);
        }
    }

    // Free SectUser memory
    for (sectu = &SectUser[0];
         sectu < &SectUser[MAXSECTORS];
         sectu++)
    {
        if (*sectu)
        {
            ////DSPRINTF(ds,"Sect User Free %d",sectu-SectUser);
            //MONO_PRINT(ds);
            FreeMem(*sectu);
            *sectu = NULL;
        }
    }

    //memset(&User[0], 0, sizeof(User));
    memset(&SectUser[0], 0, sizeof(SectUser));

    TRAVERSE_CONNECT(pnum)
    {
        PLAYERp pp = Player + pnum;

        // Free panel sprites for players
        pClearSpriteList(pp);

        pp->DoPlayerAction = NULL;

        pp->SpriteP = NULL;
        pp->PlayerSprite = -1;

        pp->UnderSpriteP = NULL;
        pp->PlayerUnderSprite = -1;

        memset(pp->HasKey, 0, sizeof(pp->HasKey));

        //pp->WpnFlags = 0;
        pp->CurWpn = NULL;

        memset(pp->Wpn, 0, sizeof(pp->Wpn));
        memset(pp->InventorySprite, 0, sizeof(pp->InventorySprite));
        memset(pp->InventoryTics, 0, sizeof(pp->InventoryTics));

        pp->Killer = -1;

        INITLIST(&pp->PanelSpriteList);
    }

    JS_UnInitLockouts();

//HEAP_CHECK();
}

void NewLevel(void)
{
    if (DemoPlaying)
    {
        InitLevel();
        InitRunLevel();

        DemoInitOnce = FALSE;
        if (DemoMode)
        {
            if (DemoModeMenuInit)
            {
                DemoModeMenuInit = FALSE;
				inputState.ClearKeyStatus(sc_Escape);
			}
        }

        DemoPlayBack();

        if (DemoRecording && DemoEdit)
        {
            RunLevel();
        }
    }
    else
    {
        do
        {
            InitLevel();
            RunLevel();
        }
        while (LoadGameOutsideMoveLoop);
		STAT_Update(false);

        if (!QuitFlag)
        {
            // for good measure do this
            ready2send = 0;
            waitforeverybody();
        }

        StatScreen(&Player[myconnectindex]);
    }

    if (LoadGameFromDemo)
        LoadGameFromDemo = FALSE;
    else
        TerminateLevel();

    InGame = FALSE;

    if (SW_SHAREWARE)
    {
        if (FinishAnim)
        {
            PlayTheme();
            MenuLevel();
			STAT_Update(true);
    }
    }
    else
    {
        if (FinishAnim == ANIM_ZILLA || FinishAnim == ANIM_SERP)
        {
            PlayTheme();
            MenuLevel();
			STAT_Update(true);
    }
    }
    FinishAnim = 0;
}


uint8_t* KeyPressedRange(uint8_t* kb, uint8_t* ke)
{
    uint8_t* k;

    for (k = kb; k <= ke; k++)
    {
        if (*k)
            return k;
    }

    return NULL;
}

void ResetKeyRange(uint8_t* kb, uint8_t* ke)
{
    uint8_t* k;

    for (k = kb; k <= ke; k++)
    {
        *k = 0;
    }
}

void PlayTheme()
{
    // start music at logo
    PlaySong(nullptr, ThemeSongs[0], ThemeTrack[0]);

    DSPRINTF(ds,"After music stuff...");
    MONO_PRINT(ds);
}

void LogoLevel(void)
{
    int fin;

    if (userConfig.nologo) return;
    DSPRINTF(ds,"LogoLevel...");
    MONO_PRINT(ds);

    MONO_PRINT(ds);

    //FadeOut(0, 0);
    ready2send = 0;
    totalclock = 0;
    ototalclock = 0;

    DSPRINTF(ds,"About to display 3drealms pic...");
    MONO_PRINT(ds);

    //FadeIn(0, 3);

    inputState.ClearAllInput();
    while (TRUE)
    {
        twod->ClearScreen();
        rotatesprite(0, 0, RS_SCALE, 0, THREED_REALMS_PIC, 0, 0, TITLE_ROT_FLAGS, 0, 0, xdim - 1, ydim - 1, nullptr, DREALMSPAL);
        videoNextPage();

        handleevents();

        // taken from top of faketimerhandler
        // limits checks to max of 40 times a second
        if (totalclock >= ototalclock + synctics)
        {
            ototalclock += synctics;
        }

        if (totalclock > 5*120 || inputState.CheckAllInput())
        {
			inputState.ClearAllInput();
            break;
        }
    }

    twod->ClearScreen();
    videoNextPage();
    videoSetPalette(0, BASEPAL, 0);

    // put up a blank screen while loading

    DSPRINTF(ds,"End of LogoLevel...");
    MONO_PRINT(ds);

}

void CreditsLevel(void)
{
    int curpic;
    int handle;
    uint32_t timer = 0;
    int zero=0;
    short save;
#define CREDITS1_PIC 5111
#define CREDITS2_PIC 5118

    // put up a blank screen while loading

    // get rid of all PERM sprites!
    renderFlushPerms();
    save = gs.BorderNum;
    gs.BorderNum = save;
    twod->ClearScreen();
    videoNextPage();
    inputState.ClearAllInput();

    // Lo Wang feel like singing!
    PlaySound(DIGI_JG95012, v3df_none, CHAN_VOICE, CHANF_UI);
    while (soundEngine->IsSourcePlayingSomething(SOURCE_None, nullptr, CHAN_VOICE))
    {
        DoUpdateSounds();
        handleevents();
        if (inputState.CheckAllInput())
            break;
        videoNextPage();
    }
    StopSound();

    // try 14 then 2 then quit
    if (!PlaySong(nullptr, ThemeSongs[5], ThemeTrack[5], true))
    {
        PlaySong(nullptr, nullptr, 2, true);
    }

    ready2send = 0;
    totalclock = 0;
    ototalclock = 0;

    inputState.ClearAllInput();
    curpic = CREDITS1_PIC;

    while (!inputState.CheckAllInput())
    {
        handleevents();

        // taken from top of faketimerhandler
        // limits checks to max of 40 times a second
        if (totalclock >= ototalclock + synctics)
        {
            ototalclock += synctics;
            timer += synctics;
        }

        rotatesprite(0, 0, RS_SCALE, 0, curpic, 0, 0, TITLE_ROT_FLAGS, 0, 0, xdim - 1, ydim - 1);

        videoNextPage();

        if (timer > 8*120)
        {
            curpic = CREDITS2_PIC;
        }

        if (timer > 16*120)
        {
            timer = 0;
            curpic = CREDITS1_PIC;
        }
		handleevents();
    }

    // put up a blank screen while loading
    twod->ClearScreen();
    videoNextPage();
    inputState.ClearAllInput();
    Mus_Stop();
}


void SybexScreen(void)
{
    if (!SW_SHAREWARE) return;

    if (CommEnabled)
        return;

    rotatesprite(0, 0, RS_SCALE, 0, 5261, 0, 0, TITLE_ROT_FLAGS, 0, 0, xdim - 1, ydim - 1);
    videoNextPage();

    inputState.ClearAllInput();
    while (!inputState.CheckAllInput()) handleevents();
}

// CTW REMOVED END

void
TitleLevel(void)
{
    twod->ClearScreen();
    videoNextPage();

	ready2send = 0;
    totalclock = 0;
    ototalclock = 0;

    rotatesprite(0, 0, RS_SCALE, 0, TITLE_PIC, 0, 0, TITLE_ROT_FLAGS, 0, 0, xdim - 1, ydim - 1);
    videoNextPage();
    //FadeIn(0, 3);

    inputState.ClearAllInput();
    while (TRUE)
    {
        handleevents();
        OSD_DispatchQueued();

        // taken from top of faketimerhandler
        // limits checks to max of 40 times a second
        if (totalclock >= ototalclock + synctics)
        {
            //void MNU_CheckForMenusAnyKey( void );

            ototalclock += synctics;
            //MNU_CheckForMenusAnyKey();
        }

        //if (M_Active())
        //    MNU_DrawMenu();

        //drawscreen as fast as you can
        rotatesprite(0, 0, RS_SCALE, 0, TITLE_PIC, 0, 0, TITLE_ROT_FLAGS, 0, 0, xdim - 1, ydim - 1);

        videoNextPage();

        if (totalclock > 5*120 || inputState.CheckAllInput())
        {
            DemoMode = TRUE;
            DemoPlaying = TRUE;
            break;
        }
    }

//    clearview(0);
//    nextpage();
    //SetPaletteToVESA(backup_pal);

    // put up a blank screen while loading
//    clearview(0);
//    nextpage();
}


void DrawMenuLevelScreen(void)
{
    twod->ClearScreen();
    rotatesprite(0, 0, RS_SCALE, 0, TITLE_PIC, 20, 0, TITLE_ROT_FLAGS, 0, 0, xdim - 1, ydim - 1);
}

void DrawStatScreen(void)
{
    twod->ClearScreen();
    rotatesprite(0, 0, RS_SCALE, 0, STAT_SCREEN_PIC, 0, 0, TITLE_ROT_FLAGS, 0, 0, xdim - 1, ydim - 1);
}

void DrawLoadLevelScreen(void)
{
    twod->ClearScreen();
    rotatesprite(0, 0, RS_SCALE, 0, TITLE_PIC, 20, 0, TITLE_ROT_FLAGS, 0, 0, xdim - 1, ydim - 1);
}

short PlayerQuitMenuLevel = -1;

void IntroAnimLevel(void)
{
    if (userConfig.nologo) return;
    DSPRINTF(ds,"IntroAnimLevel");
    MONO_PRINT(ds);
    playanm(0);
}

void MenuLevel(void)
{
    short w,h;

    M_StartControlPanel(false);
    M_SetMenu(NAME_Mainmenu);
    // do demos only if not playing multi play
    if (!CommEnabled && numplayers <= 1 && !FinishAnim && !NoDemoStartup)
    {
        // demos exist - do demo instead
        if (DemoName[0][0] != '\0')
        {
            DemoMode = TRUE;
            DemoPlaying = TRUE;
            return;
        }
    }

    DemoMode = FALSE;
    DemoPlaying = FALSE;

    twod->ClearScreen();
    videoNextPage();

    //FadeOut(0, 0);
    ready2send = 0;
    totalclock = 0;
    ototalclock = 0;
    ExitLevel = FALSE;
    InMenuLevel = TRUE;

    DrawMenuLevelScreen();

    if (CommEnabled)
    {
        sprintf(ds,"Lo Wang is waiting for other players...");
        MNU_MeasureString(ds, &w, &h);
        MNU_DrawString(TEXT_TEST_COL(w), 170, ds, 1, 16);

        sprintf(ds,"They are afraid!");
        MNU_MeasureString(ds, &w, &h);
        MNU_DrawString(TEXT_TEST_COL(w), 180, ds, 1, 16);
    }

    videoNextPage();
    //FadeIn(0, 3);

    waitforeverybody();

    // don't allow BorderAdjusting in these menus
    BorderAdjust = FALSE;

    inputState.ClearAllInput();

    if (SW_SHAREWARE)
    {
        // go to ordering menu only if shareware
        if (FinishAnim)
        {
			inputState.ClearKeyStatus(sc_Escape);
			M_StartControlPanel(false);
			M_SetMenu(NAME_CreditsMenu);
            FinishAnim = 0;
        }
    }
    else
    {
        FinishAnim = 0;
    }

    while (TRUE)
    {
        handleevents();
        OSD_DispatchQueued();

        // taken from top of faketimerhandler
        // limits checks to max of 40 times a second
        if (totalclock >= ototalclock + synctics)
        {
            ototalclock += synctics;
            if (CommEnabled)
                getpackets();
        }

        if (CommEnabled)
        {
            if (MultiPlayQuitFlag)
            {
                uint8_t pbuf[1];
                QuitFlag = TRUE;
                pbuf[0] = PACKET_TYPE_MENU_LEVEL_QUIT;
                netbroadcastpacket(pbuf, 1);                      // TENSW
                break;
            }

            if (PlayerQuitMenuLevel >= 0)
            {
                MenuCommPlayerQuit(PlayerQuitMenuLevel);
                PlayerQuitMenuLevel = -1;
            }
        }

        if (ExitLevel)
        {
            // Quiting Level
            ExitLevel = FALSE;
            break;
        }

        if (QuitFlag)
        {
            // Quiting Game
            break;
        }

        // must lock the clock for drawing so animations will happen
        totalclocklock = totalclock;

        //drawscreen as fast as you can
        DrawMenuLevelScreen();
        DoUpdateSounds();

        videoNextPage();
    }

    BorderAdjust = TRUE;
    //LoadGameOutsideMoveLoop = FALSE;
	inputState.ClearAllInput();
	M_ClearMenus();
    InMenuLevel = FALSE;
    twod->ClearScreen();
    videoNextPage();
}

void SceneLevel(void)
{
    SWBOOL dp_bak;
    SWBOOL dm_bak;
    FILE *fin;
#define CINEMATIC_DEMO_FILE "$scene.dmo"

    // make sure it exists
    if ((fin = fopen(CINEMATIC_DEMO_FILE,"rb")) == NULL)
        return;
    else
        fclose(fin);

    strcpy(DemoFileName,CINEMATIC_DEMO_FILE);

    dp_bak = DemoPlaying;
    dm_bak = DemoMode;

    DemoMode = TRUE;
    DemoPlaying = TRUE;
    DemoOverride = TRUE;
    InitLevel();
    DemoOverride = FALSE;

    ScenePlayBack();
    TerminateLevel();
    DemoMode = dm_bak;
    DemoPlaying = dp_bak;
}

void
LoadingLevelScreen(void)
{
    short w,h;
    extern SWBOOL DemoMode;
    DrawLoadLevelScreen();

    if (DemoMode)
        sprintf(ds,"DEMO");
    else
        sprintf(ds,"ENTERING");

    MNU_MeasureString(ds, &w, &h);
    MNU_DrawString(TEXT_TEST_COL(w), 170, ds,1,16);

	auto ds = currentLevel->DisplayName();
    MNU_MeasureString(ds, &w, &h);
    MNU_DrawString(TEXT_TEST_COL(w), 180, ds,1,16);

    videoNextPage();
}

void gNextState(STATEp *State)
{
    // Transition to the next state
    *State = (*State)->NextState;

    if (TEST((*State)->Tics, SF_QUICK_CALL))
    {
        (*(*State)->Animator)(0);
        *State = (*State)->NextState;
    }
}

// Generic state control
void gStateControl(STATEp *State, int *tics)
{
    *tics += synctics;

    // Skip states if too much time has passed
    while (*tics >= (*State)->Tics)
    {
        // Set Tics
        *tics -= (*State)->Tics;
        gNextState(State);
    }

    // Call the correct animator
    if ((*State)->Animator)
        (*(*State)->Animator)(0);
}

int BonusPunchSound(short UNUSED(SpriteNum))
{
    PLAYERp pp = Player + myconnectindex;
    PlaySound(DIGI_PLAYERYELL3, pp, v3df_none);
    return 0;
}

int BonusKickSound(short UNUSED(SpriteNum))
{
    PLAYERp pp = Player + myconnectindex;
    PlaySound(DIGI_PLAYERYELL2, pp, v3df_none);
    return 0;
}

int BonusGrabSound(short UNUSED(SpriteNum))
{
    PLAYERp pp = Player + myconnectindex;
    PlaySound(DIGI_BONUS_GRAB, pp, v3df_none);
    return 0;
}

extern SWBOOL FinishedLevel;
extern int PlayClock;
extern short LevelSecrets;
extern short TotalKillable;

void BonusScreen()
{
    int minutes,seconds,second_tics;

    short w,h;
    short limit;


#define BONUS_SCREEN_PIC 5120
#define BONUS_ANIM 5121
#define BONUS_ANIM_FRAMES (5159-5121)

#define BREAK_LIGHT_RATE 18

#define BONUS_PUNCH 5121
#define BONUS_KICK 5136
#define BONUS_GRAB 5151
#define BONUS_REST 5121

#define BONUS_TICS 8
#define BONUS_GRAB_TICS 20
#define BONUS_REST_TICS 50

    static STATE s_BonusPunch[] =
    {
        {BONUS_PUNCH + 0, BONUS_TICS, NULL, &s_BonusPunch[1]},
        {BONUS_PUNCH + 1, BONUS_TICS, NULL, &s_BonusPunch[2]},
        {BONUS_PUNCH + 2, BONUS_TICS, NULL, &s_BonusPunch[3]},
        {BONUS_PUNCH + 2, 0|SF_QUICK_CALL, BonusPunchSound, &s_BonusPunch[4]},
        {BONUS_PUNCH + 3, BONUS_TICS, NULL, &s_BonusPunch[5]},
        {BONUS_PUNCH + 4, BONUS_TICS, NULL, &s_BonusPunch[6]},
        {BONUS_PUNCH + 5, BONUS_TICS, NULL, &s_BonusPunch[7]},
        {BONUS_PUNCH + 6, BONUS_TICS, NULL, &s_BonusPunch[8]},
        {BONUS_PUNCH + 7, BONUS_TICS, NULL, &s_BonusPunch[9]},
        {BONUS_PUNCH + 8, BONUS_TICS, NULL, &s_BonusPunch[10]},
        {BONUS_PUNCH + 9, BONUS_TICS, NULL, &s_BonusPunch[11]},
        {BONUS_PUNCH + 10, BONUS_TICS, NULL, &s_BonusPunch[12]},
        {BONUS_PUNCH + 11, BONUS_TICS, NULL, &s_BonusPunch[13]},
        {BONUS_PUNCH + 12, BONUS_TICS, NULL, &s_BonusPunch[14]},
        {BONUS_PUNCH + 14, 90,        NULL, &s_BonusPunch[15]},
        {BONUS_PUNCH + 14, BONUS_TICS, NULL, &s_BonusPunch[15]},
    };

    static STATE s_BonusKick[] =
    {
        {BONUS_KICK + 0, BONUS_TICS, NULL, &s_BonusKick[1]},
        {BONUS_KICK + 1, BONUS_TICS, NULL, &s_BonusKick[2]},
        {BONUS_KICK + 2, BONUS_TICS, NULL, &s_BonusKick[3]},
        {BONUS_KICK + 2, 0|SF_QUICK_CALL, BonusKickSound, &s_BonusKick[4]},
        {BONUS_KICK + 3, BONUS_TICS, NULL, &s_BonusKick[5]},
        {BONUS_KICK + 4, BONUS_TICS, NULL, &s_BonusKick[6]},
        {BONUS_KICK + 5, BONUS_TICS, NULL, &s_BonusKick[7]},
        {BONUS_KICK + 6, BONUS_TICS, NULL, &s_BonusKick[8]},
        {BONUS_KICK + 7, BONUS_TICS, NULL, &s_BonusKick[9]},
        {BONUS_KICK + 8, BONUS_TICS, NULL, &s_BonusKick[10]},
        {BONUS_KICK + 9, BONUS_TICS, NULL, &s_BonusKick[11]},
        {BONUS_KICK + 10, BONUS_TICS, NULL, &s_BonusKick[12]},
        {BONUS_KICK + 11, BONUS_TICS, NULL, &s_BonusKick[13]},
        {BONUS_KICK + 12, BONUS_TICS, NULL, &s_BonusKick[14]},
        {BONUS_KICK + 14, 90,        NULL, &s_BonusKick[15]},
        {BONUS_KICK + 14, BONUS_TICS, NULL, &s_BonusKick[15]},
    };

    static STATE s_BonusGrab[] =
    {
        {BONUS_GRAB + 0, BONUS_GRAB_TICS, NULL, &s_BonusGrab[1]},
        {BONUS_GRAB + 1, BONUS_GRAB_TICS, NULL, &s_BonusGrab[2]},
        {BONUS_GRAB + 2, BONUS_GRAB_TICS, NULL, &s_BonusGrab[3]},
        {BONUS_GRAB + 2, 0|SF_QUICK_CALL, BonusGrabSound, &s_BonusGrab[4]},
        {BONUS_GRAB + 3, BONUS_GRAB_TICS, NULL, &s_BonusGrab[5]},
        {BONUS_GRAB + 4, BONUS_GRAB_TICS, NULL, &s_BonusGrab[6]},
        {BONUS_GRAB + 5, BONUS_GRAB_TICS, NULL, &s_BonusGrab[7]},
        {BONUS_GRAB + 6, BONUS_GRAB_TICS, NULL, &s_BonusGrab[8]},
        {BONUS_GRAB + 7, BONUS_GRAB_TICS, NULL, &s_BonusGrab[9]},
        {BONUS_GRAB + 8, BONUS_GRAB_TICS, NULL, &s_BonusGrab[10]},
        {BONUS_GRAB + 9, 90,             NULL, &s_BonusGrab[11]},
        {BONUS_GRAB + 9, BONUS_GRAB_TICS, NULL, &s_BonusGrab[11]},
    };

#if 1 // Turned off the standing animate because he looks like a FAG!
    static STATE s_BonusRest[] =
    {
        {BONUS_REST + 0, BONUS_REST_TICS, NULL, &s_BonusRest[1]},
        {BONUS_REST + 1, BONUS_REST_TICS, NULL, &s_BonusRest[2]},
        {BONUS_REST + 2, BONUS_REST_TICS, NULL, &s_BonusRest[3]},
        {BONUS_REST + 1, BONUS_REST_TICS, NULL, &s_BonusRest[0]},
    };
#else
    static STATE s_BonusRest[] =
    {
        {BONUS_REST + 0, BONUS_REST_TICS, NULL, &s_BonusRest[1]},
        {BONUS_REST + 0, BONUS_REST_TICS, NULL, &s_BonusRest[0]},
    };
#endif

    static STATEp s_BonusAnim[] =
    {
        s_BonusPunch,
        s_BonusKick,
        s_BonusGrab
    };

    STATEp State = s_BonusRest;

    int Tics = 0;
    int line = 0;
    SWBOOL BonusDone;

    if (Level < 0) Level = 0;

    twod->ClearScreen();
    videoNextPage();

    inputState.ClearAllInput();

    totalclock = ototalclock = 0;
    limit = synctics;

    PlaySong(nullptr, ThemeSongs[1], ThemeTrack[1]);

    // special case code because I don't care any more!
    if (FinishAnim)
    {
        renderFlushPerms();
        rotatesprite(0, 0, RS_SCALE, 0, 5120, 0, 0, TITLE_ROT_FLAGS, 0, 0, xdim - 1, ydim - 1);
        rotatesprite(158<<16, 86<<16, RS_SCALE, 0, State->Pic, 0, 0, TITLE_ROT_FLAGS, 0, 0, xdim - 1, ydim - 1);
        videoNextPage();
        //FadeIn(0,0);
    }

    BonusDone = FALSE;
    while (!BonusDone)
    {
        handleevents();

        // taken from top of faketimerhandler
        if (totalclock < ototalclock + limit)
        {
            continue;
        }
        ototalclock += limit;

        if (inputState.CheckAllInput())
        {
            if (State >= s_BonusRest && State < &s_BonusRest[SIZ(s_BonusRest)])
            {
                State = s_BonusAnim[STD_RANDOM_RANGE(SIZ(s_BonusAnim))];
                Tics = 0;
            }
        }

        gStateControl(&State, &Tics);

        twod->ClearScreen();
        rotatesprite(0, 0, RS_SCALE, 0, 5120, 0, 0, TITLE_ROT_FLAGS, 0, 0, xdim - 1, ydim - 1);

        if (UserMapName[0])
        {
            sprintf(ds,"%s",UserMapName);
            MNU_MeasureString(ds, &w, &h);
            MNU_DrawString(TEXT_TEST_COL(w), 20, ds,1,19);
        }
        else
        {
            if (PlayingLevel <= 1)
                PlayingLevel = 1;
			auto ds = currentLevel->DisplayName();
            MNU_MeasureString(ds, &w, &h);
            MNU_DrawString(TEXT_TEST_COL(w), 20, ds,1,19);
        }

        sprintf(ds,"Completed");
        MNU_MeasureString(ds, &w, &h);
        MNU_DrawString(TEXT_TEST_COL(w), 30, ds,1,19);

        rotatesprite(158<<16, 86<<16, RS_SCALE, 0, State->Pic, 0, 0, TITLE_ROT_FLAGS, 0, 0, xdim - 1, ydim - 1);

#define BONUS_LINE(i) (50 + ((i)*20))

        line = 0;
        second_tics = (PlayClock/120);
        minutes = (second_tics/60);
        seconds = (second_tics%60);
        sprintf(ds,"Your Time:  %2d : %02d", minutes, seconds);
        MNU_MeasureString(ds, &w, &h);
        MNU_DrawString(60, BONUS_LINE(line), ds,1,16);

        if (!UserMapName[0])
        {
            line++;
			sprintf(ds,"3D Realms Best Time:  %d:%02d", currentLevel->designerTime/60, currentLevel->designerTime%60);
            MNU_MeasureString(ds, &w, &h);
            MNU_DrawString(40, BONUS_LINE(line), ds,1,16);

            line++;
			sprintf(ds,"Par Time:  %d:%02d", currentLevel->parTime/ 60, currentLevel->parTime%60);
            MNU_MeasureString(ds, &w, &h);
            MNU_DrawString(40, BONUS_LINE(line), ds,1,16);
        }


        // always read secrets and kills from the first player
        line++;
        sprintf(ds,"Secrets:  %d / %d", Player->SecretsFound, LevelSecrets);
        MNU_MeasureString(ds, &w, &h);
        MNU_DrawString(60, BONUS_LINE(line), ds,1,16);

        line++;
        sprintf(ds,"Kills:  %d / %d", Player->Kills, TotalKillable);
        MNU_MeasureString(ds, &w, &h);
        MNU_DrawString(60, BONUS_LINE(line), ds,1,16);


        sprintf(ds,"Press SPACE to continue");
        MNU_MeasureString(ds, &w, &h);
        MNU_DrawString(TEXT_TEST_COL(w), 185, ds,1,19);

        videoNextPage();

        if (State == State->NextState)
            BonusDone = TRUE;
    }

    StopSound();
}

void EndGameSequence(void)
{
    SWBOOL anim_ok = TRUE;
    //FadeOut(0, 5);
	StopSound();

    if ((adult_lockout || Global_PLock) && FinishAnim == ANIM_SUMO)
        anim_ok = FALSE;

    if (anim_ok)
        playanm(FinishAnim);

    BonusScreen();

    ExitLevel = FALSE;
    QuitFlag = FALSE;
    AutoNet = FALSE;

    if (FinishAnim == ANIM_ZILLA)
        CreditsLevel();

    ExitLevel = FALSE;
    QuitFlag = FALSE;
    AutoNet = FALSE;

    if (SW_SHAREWARE)
    {
        Level = 0;
    }
    else
    {
        if (Level == 4 || Level == 20)
        {
            Level=0;
        }
        else
            Level++;
    }
}

void StatScreen(PLAYERp mpp)
{
    extern SWBOOL FinishedLevel;
    short w,h;

    short rows,cols,i,j;
    PLAYERp pp = NULL;
    int x,y;
    short death_total[MAX_SW_PLAYERS_REG];
    short kills[MAX_SW_PLAYERS_REG];
    short pal;

#define STAT_START_X 20
#define STAT_START_Y 85
#define STAT_OFF_Y 9
#define STAT_HEADER_Y 14

#define SM_SIZ(num) ((num)*4)

#define STAT_TABLE_X (STAT_START_X + SM_SIZ(15))
#define STAT_TABLE_XOFF SM_SIZ(6)

    // No stats in bot games
    //if (BotMode) return;

    //ResetPalette(mpp);
    COVER_SetReverb(0); // Reset reverb
    mpp->Reverb = 0;
    StopSound();
    soundEngine->UpdateSounds((int)totalclock);

    if (FinishAnim)
    {
        EndGameSequence();
        return;
    }

    if (gNet.MultiGameType != MULTI_GAME_COMMBAT)
    {
        if (!FinishedLevel)
            return;
        BonusScreen();
        return;
    }

    renderFlushPerms();
    DrawStatScreen();

    memset(death_total,0,sizeof(death_total));
    memset(kills,0,sizeof(kills));

    auto c = GStrings("MULTIPLAYER TOTALS");
    MNU_MeasureString(c, &w, &h);
    MNU_DrawString(TEXT_TEST_COL(w), 68, c, 0, 0);

    c = GStrings("TXTS_PRESSSPACE");
    MNU_MeasureString(c, &w, &h);
    MNU_DrawString(TEXT_TEST_COL(w), 189, c, 0, 0);

    x = STAT_START_X;
    y = STAT_START_Y;

    // Hm.... how to translate this without messing up the formatting?
    sprintf(ds,"  NAME         1     2     3     4     5     6     7    8     KILLS");
    DisplayMiniBarSmString(mpp, x, y, 0, ds);
    rows = OrigCommPlayers;
    cols = OrigCommPlayers;
    mpp = Player + myconnectindex;

    y += STAT_HEADER_Y;

    for (i = 0; i < rows; i++)
    {
        x = STAT_START_X;
        pp = Player + i;

        sprintf(ds,"%d", i+1);
        DisplayMiniBarSmString(mpp, x, y, 0, ds);

        sprintf(ds,"  %-13s", pp->PlayerName);
        DisplayMiniBarSmString(mpp, x, y, User[pp->PlayerSprite]->spal, ds);

        x = STAT_TABLE_X;
        for (j = 0; j < cols; j++)
        {
            pal = 0;
            death_total[j] += pp->KilledPlayer[j];

            if (i == j)
            {
                // don't add kill for self or team player
                pal = PALETTE_PLAYER0 + 4;
                kills[i] -= pp->KilledPlayer[j];  // subtract self kills
            }
            else if (gNet.TeamPlay)
            {
                if (User[pp->PlayerSprite]->spal == User[Player[j].PlayerSprite]->spal)
                {
                    // don't add kill for self or team player
                    pal = PALETTE_PLAYER0 + 4;
                    kills[i] -= pp->KilledPlayer[j];  // subtract self kills
                }
                else
                    kills[i] += pp->KilledPlayer[j];  // kills added here
            }
            else
            {
                kills[i] += pp->KilledPlayer[j];  // kills added here
            }

            sprintf(ds,"%d", pp->KilledPlayer[j]);
            DisplayMiniBarSmString(mpp, x, y, pal, ds);
            x += STAT_TABLE_XOFF;
        }

        y += STAT_OFF_Y;
    }


    // Deaths

    x = STAT_START_X;
    y += STAT_OFF_Y;

    sprintf(ds,"   %s", GStrings("DEATHS"));
    DisplayMiniBarSmString(mpp, x, y, 0, ds);
    x = STAT_TABLE_X;

    for (j = 0; j < cols; j++)
    {
        sprintf(ds,"%d",death_total[j]);
        DisplayMiniBarSmString(mpp, x, y, 0, ds);
        x += STAT_TABLE_XOFF;
    }

    x = STAT_START_X;
    y += STAT_OFF_Y;

    // Kills
    x = STAT_TABLE_X + SM_SIZ(50);
    y = STAT_START_Y + STAT_HEADER_Y;

    for (i = 0; i < rows; i++)
    {
        pp = Player + i;

        sprintf(ds,"%d", kills[i]); //pp->Kills);
        DisplayMiniBarSmString(mpp, x, y, 0, ds);

        y += STAT_OFF_Y;
    }

    videoNextPage();

    inputState.ClearAllInput();

    PlaySong(nullptr, ThemeSongs[1], ThemeTrack[1]);

    while (!inputState.GetKeyStatus(KEYSC_SPACE) && !inputState.GetKeyStatus(KEYSC_ENTER))
    {
        handleevents();
    }

    StopSound();
}

void GameIntro(void)
{

    DSPRINTF(ds,"GameIntro...");
    MONO_PRINT(ds);

    if (DemoPlaying)
        return;

    // this could probably be taken out and you could select skill level
    // from menu to start the game
    if (!CommEnabled && UserMapName[0])
        return;

    Level = 1;



    PlayTheme();

    if (!AutoNet)
    {
        LogoLevel();
        //CreditsLevel();
        //SceneLevel();
        //TitleLevel();
        IntroAnimLevel();
        IntroAnimCount = 0;
    }

    MenuLevel();
}

void Control()
{
	InitGame();

    MONO_PRINT("InitGame done");
    //MNU_InitMenus();
    InGame = TRUE;
    GameIntro();

    while (!QuitFlag)
    {
        handleevents();
        OSD_DispatchQueued();

        NewLevel();
    }

    CleanExit = TRUE;
    throw CExitEvent(0);
}


void _Assert(const char *expr, const char *strFile, unsigned uLine)
{
    I_FatalError("Assertion failed: %s %s, line %u", expr, strFile, uLine);
}



void dsprintf(char *str, const char *format, ...)
{
    va_list arglist;

    va_start(arglist, format);
    vsprintf(str, format, arglist);
    va_end(arglist);
}

void dsprintf_null(char *str, const char *format, ...)
{
    va_list arglist;
}


void InitPlayerGameSettings(void)
{
    int pnum;

    // don't jack with auto aim settings if DemoMode is going
    // what the hell did I do this for?????????
    //if (DemoMode)
    //    return;

    if (CommEnabled)
    {
        // everyone gets the same Auto Aim
        TRAVERSE_CONNECT(pnum)
        {
            if (gNet.AutoAim)
                SET(Player[pnum].Flags, PF_AUTO_AIM);
            else
                RESET(Player[pnum].Flags, PF_AUTO_AIM);
        }
    }
    else
    {
        if (cl_autoaim)
            SET(Player[myconnectindex].Flags, PF_AUTO_AIM);
        else
            RESET(Player[myconnectindex].Flags, PF_AUTO_AIM);
    }
}


void InitRunLevel(void)
{
    if (DemoEdit)
        return;

    if (LoadGameOutsideMoveLoop)
    {
        int SavePlayClock;
        extern int PlayClock;
        LoadGameOutsideMoveLoop = FALSE;
        // contains what is needed from calls below
        if (snd_ambience)
            StartAmbientSound();
        SetCrosshair();
        SetRedrawScreen(Player + myconnectindex);
        // crappy little hack to prevent play clock from being overwritten
        // for load games
        SavePlayClock = PlayClock;
        InitTimingVars();
        PlayClock = SavePlayClock;
        MONO_PRINT("Done with InitRunLevel");
        return;
    }

#if 0
    // ensure we are through the initialization code before sending the game
    // version. Otherwise, it is possible to send this too early and have it
    // blown away on the other side.
    waitforeverybody();
#endif

    SendVersion(GameVersion);

    waitforeverybody();

    Mus_Stop();

    if (Bstrcasecmp(CacheLastLevel, LevelName) != 0)
        DoTheCache();

    // auto aim / auto run / etc
    InitPlayerGameSettings();

    // send packets with player info
    InitNetPlayerOptions();

    // Initialize Game part of network code (When ready2send != 0)
    InitNetVars();

    {
        if (Level == 0)
        {
			PlaySong(nullptr, currentLevel->music, 1 + RANDOM_RANGE(10));
        }
        else
        {
			PlaySong(currentLevel->labelName, currentLevel->music, currentLevel->cdSongId);
        }
    }

    InitPrediction(&Player[myconnectindex]);

    if (!DemoInitOnce)
        DemoRecordSetup();

    // everything has been inited at least once for RECORD
    DemoInitOnce = TRUE;

//DebugWriteLoc(__FILE__, __LINE__);
    waitforeverybody();

    CheckVersion(GameVersion);

    // IMPORTANT - MUST be right before game loop AFTER waitforeverybody
    InitTimingVars();

    SetRedrawScreen(Player + myconnectindex);
	
    if (snd_ambience)
        StartAmbientSound();
}

void RunLevel(void)
{
    InitRunLevel();

#if 0
    waitforeverybody();
#endif
    ready2send = 1;

    while (TRUE)
    {
        handleevents();
        OSD_DispatchQueued();
		D_ProcessEvents();
        if (LoadGameOutsideMoveLoop)
        {
            return; // Stop the game loop if a savegame was loaded from the menu.
        }

        if (M_Active())
        {
            ototalclock = (int)totalclock;
        }
        else
        {
            while ((totalclock - ototalclock) >= synctics)
            {
                ototalclock += synctics;

                getinput(myconnectindex);

                PLAYERp const pp     = Player + myconnectindex;
                auto    const q16ang = fix16_to_int(pp->q16ang);
                auto &        input  = pp->inputfifo[Player[myconnectindex].movefifoend & (MOVEFIFOSIZ - 1)];

                input = localInput;
                input.vel =  mulscale9(localInput.vel,  sintable[NORM_ANGLE(q16ang + 512)]) +
                             mulscale9(localInput.svel, sintable[NORM_ANGLE(q16ang)]);
                input.svel = mulscale9(localInput.vel,  sintable[NORM_ANGLE(q16ang)]) + 
                             mulscale9(localInput.svel, sintable[NORM_ANGLE(q16ang + 1536)]);

                pp->movefifoend++;
                localInput = {};

                domovethings();
            }
        }

        if (!ScrollMode2D)
            getinput(myconnectindex);

        drawscreen(Player + screenpeek);

        if (QuitFlag)
            break;

        if (ExitLevel)
        {
            ExitLevel = FALSE;
            break;
        }
    }

    ready2send = 0;
}

typedef struct
{
    char    notshareware;
    const char    *arg_switch;
    short   arg_match_len;
    const char    *arg_fmt;
    const char    *arg_descr;
} CLI_ARG;




CLI_ARG cli_arg[] =
{
    {0, "/?",                  2,      "-?",                   "This help message"                     },
//#ifndef SW_SHAREWARE
//{"/l",                  2,      "-l#",                  "Level (1-11)"                          },
//{"/v",                  2,      "-v#",                  "Volume (1-3)"                          },
    {1, "/map",                4,      "-map [mapname]",       "Load a map"                            },
    {1, "/nocdaudio",          5,      "-nocd<audio>",         "No CD Red Book Audio"                  },
//#endif

    {0, "/name",               5,      "-name [playername]",   "Player Name"                           },
    {0, "/s",                  2,      "-s#",                  "Skill (1-4)"                           },
    {0, "/f#",                 3,      "-f#",                  "Packet Duplication - 2, 4, 8"          },
    {0, "/nopredict",          7,      "-nopred<ict>",         "Disable Net Prediction Method"         },
    {0, "/level#",             5,      "-level#",              "Start at level# (Shareware: 1-4, full version 1-28)"      },
    {0, "/dr",                 3,      "-dr[filename.dmo]",    "Demo record. NOTE: Must use -level# with this option."           },
    {0, "/dp",                 3,      "-dp[filename.dmo]",    "Demo playback. NOTE: Must use -level# with this option."         },
    {0, "/m",                  6,      "-monst<ers>",          "No Monsters"                           },
    {0, "/nodemo",             6,      "-nodemo",              "No demos on game startup"              },
    {0, "/nometers",           9,      "-nometers",            "Don't show air or boss meter bars in game"},
    {0, "/movescale #",        9,      "-movescale",           "Adjust movement scale: 256 = 1 unit"},
    {0, "/turnscale #",        9,      "-turnscale",           "Adjust turning scale: 256 = 1 unit"},
    {0, "/extcompat",          9,      "-extcompat",           "Controller compatibility mode (with Duke 3D)"},
    {1, "/g#",                 2,      "-g[filename.grp]",     "Load an extra GRP or ZIP file"},
    {1, "/h#",                 2,      "-h[filename.def]",     "Use filename.def instead of SW.DEF"},
    {0, "/setup",              5,      "-setup",               "Displays the configuration dialogue box"},
#if DEBUG
    {0, "/coop",               5,      "-coop#",               "Single Player Cooperative Mode"        },
    {0, "/commbat",            8,      "-commbat#",            "Single Player Commbat Mode"            },
    {0, "/debug",              6,      "-debug",               "Debug Help Options"                    },
#endif

#if 0 //def NET_MODE_MASTER_SLAVE
    {0, "/broadcast",          6,      "-broad<cast>",         "Broadcast network method (default)"    },
    {0, "/masterslave",        7,      "-master<slave>",       "Master/Slave network method"           },
#endif
};

#if 0
Map       ->    User Map Name
Auto      ->    Auto Start Game
Rules     ->    0=WangBang 1=WangBang (No Respawn) 2=CoOperative
Level     ->    0 to 24(?)
Enemy     ->    0=None 1=Easy 2=Norm 3=Hard 4=Insane
Markers   ->    0=Off 1=On
Team      ->    0=Off 1=On
HurtTeam  ->    0=Off 1=On
KillLimit ->    0=Infinite 1=10 2=20 3=30 4=40 5=50 6=60 7=70 8=80 9=90 10=100
TimeLimit ->    0=Infinite 1=3 2=5 3=10 4=20 5=30 6=45 7=60
Color     ->    0=Brown 1=Purple 2=Red 3=Yellow 4=Olive 5=Green
Nuke      ->    0=Off 1=On

                                                                                                                                                                                                                                                                                                                                   Example Command Line :
                                                                                                                                                                                                                                                                                                                                   sw -map testmap.map -autonet 0,0,1,1,1,0,3,2,1,1 -f4 -name 1234567890 -net 12345678
commit -map grenade -autonet 0,0,1,1,1,0,3,2,1,1 -name frank
#endif

char isShareware = FALSE;

int DetectShareware(void)
{
    return (isShareware = !!(g_gameType & GAMEFLAG_SHAREWARE));
}


void CommandLineHelp(char const * const * argv)
{
}

static const char* actions[] = {
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
    "Weapon_1",
    "Weapon_2",
    "Weapon_3",
    "Weapon_4",
    "Weapon_5",
    "Weapon_6",
    "Weapon_7",
    "Weapon_8",
    "Weapon_9",
    "Weapon_10",
    "Inventory",
    "Inventory_Left",
    "Inventory_Right",
    "NightVision",
    "MedKit",
    "TurnAround",
    "SendMessage",
    "Map",
    "Shrink_Screen",
    "Enlarge_Screen",
    "Center_View",
    "Holster_Weapon",
    "Show_Opponents_Weapon",
    "Map_Follow_Mode",
    "See_Coop_View",
    "Mouse_Aiming",
    "Toggle_Crosshair",
    "Next_Weapon",
    "Previous_Weapon",
    "Dpad_Select",
    "Dpad_Aiming",
    "Last_Weapon",
    "Alt_Weapon",
    "Third_Person_View",
    "Toggle_Crouch",	// This is the last one used by EDuke32"",
    "Smoke_Bomb",
    "Gas_Bomb",
    "Flash_Bomb",
    "Caltrops",

};

int32_t GameInterface::app_main()
{
    int i;
    extern int MovesPerPacket;
    void DoSector(void);
    void gameinput(void);
    int cnt = 0;

    buttonMap.SetButtons(actions, NUM_ACTIONS);
    automapping = 1;
    BorderAdjust = true;
    SW_ExtInit();

    CONFIG_ReadSetup();

    hud_size.Callback();

    if (!DetectShareware())
    {
        if (SW_SHAREWARE) Printf("Detected shareware GRP\n");
        else Printf("Detected registered GRP\n");
    }

    for (i = 0; i < MAX_SW_PLAYERS; i++)
        INITLIST(&Player[i].PanelSpriteList);

    DebugOperate = TRUE;
    enginecompatibility_mode = ENGINECOMPATIBILITY_19961112;

    if (SW_SHAREWARE)
        Printf("SHADOW WARRIOR(tm) Version 1.2 (Shareware Version)\n");
    else
        Printf("SHADOW WARRIOR(tm) Version 1.2\n");

    if (sw_snd_scratch == 0)    // This is always 0 at this point - this check is only here to prevent whole program optimization from eliminating the variable.
        Printf("Copyright (c) 1997 3D Realms Entertainment\n");

    UserMapName[0] = '\0';

    registerosdcommands();

    Control();

    return 0;
}

void ManualPlayerInsert(PLAYERp pp)
{
    PLAYERp npp = Player + numplayers;

    if (numplayers < MAX_SW_PLAYERS)
    {
        connectpoint2[numplayers - 1] = numplayers;
        connectpoint2[numplayers] = -1;

        npp->posx = pp->posx;
        npp->posy = pp->posy;
        npp->posz = pp->posz;
        npp->q16ang = pp->q16ang;
        npp->cursectnum = pp->cursectnum;

        myconnectindex = numplayers;
        screenpeek = numplayers;

        sprintf(Player[myconnectindex].PlayerName,"PLAYER %d",myconnectindex+1);

        Player[numplayers].movefifoend = Player[0].movefifoend;

        // If IsAI = TRUE, new player will be a bot
        Player[myconnectindex].IsAI = FALSE;

        numplayers++;
    }

}

void BotPlayerInsert(PLAYERp pp)
{
    PLAYERp npp = Player + numplayers;

    if (numplayers < MAX_SW_PLAYERS)
    {
        connectpoint2[numplayers - 1] = numplayers;
        connectpoint2[numplayers] = -1;

        npp->posx = pp->posx;
        npp->posy = pp->posy;
        npp->posz = pp->posz-Z(100);
        npp->q16ang = pp->q16ang;
        npp->cursectnum = pp->cursectnum;

        //myconnectindex = numplayers;
        //screenpeek = numplayers;

        sprintf(Player[numplayers].PlayerName,"BOT %d",numplayers+1);

        Player[numplayers].movefifoend = Player[0].movefifoend;

        // If IsAI = TRUE, new player will be a bot
        Player[numplayers].IsAI = TRUE;

        numplayers++;
    }

//    SetFragBar(pp);
}

void
ManualPlayerDelete(void)
{
    short i, nexti;
    USERp u;
    PLAYERp pp;

    if (numplayers > 1)
    {
        numplayers--;
        connectpoint2[numplayers - 1] = -1;

        pp = Player + numplayers;

        KillSprite(pp->PlayerSprite);
        pp->PlayerSprite = -1;

        // Make sure enemys "forget" about deleted player
        TRAVERSE_SPRITE_STAT(headspritestat[STAT_ENEMY], i, nexti)
        {
            u = User[i];
            if (u->tgt_sp == pp->SpriteP)
                u->tgt_sp = Player[0].SpriteP;
        }

        if (myconnectindex >= numplayers)
            myconnectindex = 0;

        if (screenpeek >= numplayers)
            screenpeek = 0;
    }
}



char WangBangMacro[10][64];

void
FunctionKeys(PLAYERp pp)
{
    static int rts_delay = 0;
    int fn_key = 0;

    rts_delay++;

    if (inputState.GetKeyStatus(sc_F1))   { fn_key = 1; }
    if (inputState.GetKeyStatus(sc_F2))   { fn_key = 2; }
    if (inputState.GetKeyStatus(sc_F3))   { fn_key = 3; }
    if (inputState.GetKeyStatus(sc_F4))   { fn_key = 4; }
    if (inputState.GetKeyStatus(sc_F5))   { fn_key = 5; }
    if (inputState.GetKeyStatus(sc_F6))   { fn_key = 6; }
    if (inputState.GetKeyStatus(sc_F7))   { fn_key = 7; }
    if (inputState.GetKeyStatus(sc_F8))   { fn_key = 8; }
    if (inputState.GetKeyStatus(sc_F9))   { fn_key = 9; }
    if (inputState.GetKeyStatus(sc_F10))  { fn_key = 10; }

    if (inputState.AltPressed())
    {
        if (rts_delay > 16 && fn_key && CommEnabled && !adult_lockout && !Global_PLock)
        {
			inputState.ClearKeyStatus(sc_F1 + fn_key - 1);

            rts_delay = 0;

            PlaySoundRTS(fn_key);

            if (CommEnabled)
            {
                PACKET_RTS p;

                p.PacketType = PACKET_TYPE_RTS;
                p.RTSnum = fn_key;

                netbroadcastpacket((uint8_t*)(&p), sizeof(p));            // TENSW
            }
        }

        return;
    }

    if (inputState.ShiftPressed())
    {
        if (fn_key && CommEnabled)
        {
			inputState.ClearKeyStatus(sc_Escape);
			inputState.ClearKeyStatus(sc_F1 + fn_key - 1);

            if (CommEnabled)
            {
                short pnum;

                sprintf(ds,"SENT: %s",**CombatMacros[fn_key-1]);
                adduserquote(ds);

                TRAVERSE_CONNECT(pnum)
                {
                    if (pnum != myconnectindex)
                    {
                        sprintf(ds,"%s: %s",pp->PlayerName, **CombatMacros[fn_key - 1]);
                        SW_SendMessage(pnum, ds);
                    }
                }
            }
        }

        return;
    }

    // F7 VIEW control
	if (buttonMap.ButtonDown(gamefunc_Third_Person_View))
    {
		buttonMap.ClearButton(gamefunc_Third_Person_View);

        if (inputState.GetKeyStatus(KEYSC_LSHIFT) || inputState.GetKeyStatus(KEYSC_RSHIFT))
        {
            if (TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE))
                pp->view_outside_dang = NORM_ANGLE(pp->view_outside_dang + 256);
        }
        else
        {
            if (TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE))
            {
                RESET(pp->Flags, PF_VIEW_FROM_OUTSIDE);
            }
            else
            {
                SET(pp->Flags, PF_VIEW_FROM_OUTSIDE);
                pp->camera_dist = 0;
            }
        }
    }


}

void PauseKey(PLAYERp pp)
{
    extern SWBOOL GamePaused,CheatInputMode;

    if (inputState.GetKeyStatus(sc_Pause) && !CommEnabled && !InputMode && !M_Active() && !CheatInputMode && !ConPanel)
    {
		inputState.ClearKeyStatus(sc_Pause);

        PauseKeySet ^= 1;

        if (PauseKeySet)
            GamePaused = TRUE;
        else
            GamePaused = FALSE;

        if (GamePaused)
        {
            short w,h;
#define MSG_GAME_PAUSED "Game Paused"
            MNU_MeasureString(MSG_GAME_PAUSED, &w, &h);
            PutStringTimer(pp, TEXT_TEST_COL(w), 100, MSG_GAME_PAUSED, 999);
            Mus_SetPaused(true);
        }
        else
        {
            pClearTextLine(pp, 100);
            Mus_SetPaused(false);
        }
    }

    if (!CommEnabled && TEST(pp->Flags, PF_DEAD))
    {
        if (ReloadPrompt)
        {
                ReloadPrompt = FALSE;
		   /*
            }
            else
            {
				inputState.SetKeyStatus(sc_Escape);
				ControlPanelType = ct_quickloadmenu;
            }
			*/
        }
    }
}

short MirrorDelay;

void getinput(int const playerNum)
{
    int i;
    PLAYERp pp = Player + playerNum;
    int inv_hotkey = 0;

#define TURBOTURNTIME (120/8)
#define NORMALTURN   (12+6)
#define RUNTURN      (28)
#define PREAMBLETURN 3
#define NORMALKEYMOVE 35
#define MAXVEL       ((NORMALKEYMOVE*2)+10)
#define MAXSVEL      ((NORMALKEYMOVE*2)+10)
#define MAXANGVEL    1024
#define MAXHORIZVEL  256
#define HORIZ_SPEED  (16)
#define TURN_SHIFT   4
#define SET_LOC_KEY(bits, sync_num, key_test) SET(bits, ((!!(key_test)) << (sync_num)))

    static int32_t turnheldtime;

    // reset objects.
    SW_PACKET input {};
    localInput = {};
    localInput.bits = 0;

    extern SWBOOL MenuButtonAutoAim;

    if (Prediction && CommEnabled)
    {
        pp = ppp;
    }

    // MAKE SURE THIS WILL GET SET
    SET_LOC_KEY(localInput.bits, SK_QUIT_GAME, MultiPlayQuitFlag);

	bool mouseaim = in_mousemode || buttonMap.ButtonDown(gamefunc_Mouse_Aiming);

	if (!CommEnabled)
	{
		// Go back to the source to set this - the old code here was catastrophically bad.
		// this needs to be fixed properly - as it is this can never be compatible with demo playback.

		if (mouseaim)
			SET(Player[playerNum].Flags, PF_MOUSE_AIMING_ON);
		else
			RESET(Player[playerNum].Flags, PF_MOUSE_AIMING_ON);

		if (cl_autoaim)
			SET(Player[playerNum].Flags, PF_AUTO_AIM);
		else
			RESET(Player[playerNum].Flags, PF_AUTO_AIM);
		}

    ControlInfo info;
    CONTROL_GetInput(&info);

    int const running = G_CheckAutorun(buttonMap.ButtonDown(gamefunc_Run));
    int32_t turnamount;
    int32_t keymove;
    constexpr int analogTurnAmount   = (NORMALTURN << 1);
    constexpr int const analogExtent = 32767; // KEEPINSYNC sdlayer.cpp

    if (running)
    {
        if (pp->sop_control)
            turnamount = RUNTURN * 3;
        else
            turnamount = RUNTURN;

        keymove = NORMALKEYMOVE << 1;
    }
    else
    {
        if (pp->sop_control)
            turnamount = NORMALTURN * 3;
        else
            turnamount = NORMALTURN;

        keymove = NORMALKEYMOVE;
    }

    PauseKey(pp);

    if (PauseKeySet)
        return;

    // MAP KEY
    if (buttonMap.ButtonDown(gamefunc_Map))
    {
        buttonMap.ClearButton(gamefunc_Map);

        // Init follow coords
        pp->mfposx = pp->posx;
        pp->mfposy = pp->posy;

        if (dimensionmode == 3)
            dimensionmode = 5;
        else if (dimensionmode == 5)
            dimensionmode = 6;
        else
        {
            MirrorDelay = 1;
            dimensionmode = 3;
            SetFragBar(pp);
            ScrollMode2D = FALSE;
            SetRedrawScreen(pp);
        }
    }

    // Toggle follow map mode on/off
    if (dimensionmode == 5 || dimensionmode == 6)
    {
        if (buttonMap.ButtonDown(gamefunc_Map_Follow_Mode))
        {
			buttonMap.ClearButton(gamefunc_Map_Follow_Mode);
            ScrollMode2D = !ScrollMode2D;
            pp->mfposx = pp->posx;
            pp->mfposy = pp->posy;
        }
    }

    // If in 2D follow mode, scroll around.
    if (ScrollMode2D && !Prediction)
    {
        keymove = keymove / 2;

        if (M_Active())
            return;

        // Recenter view if told
        if (buttonMap.ButtonDown(gamefunc_Center_View))
        {
            pp->mfposx = pp->posx;
            pp->mfposy = pp->posy;
        }

        // Toggle follow map mode on/off
        if (buttonMap.ButtonDown(gamefunc_Map_Follow_Mode))
        {
            buttonMap.ClearButton(gamefunc_Map_Follow_Mode);
            ScrollMode2D = !ScrollMode2D;
            // Reset coords
            pp->mfposx = pp->posx;
            pp->mfposy = pp->posy;
        }

        if (buttonMap.ButtonDown(gamefunc_Strafe))
            input.svel -= info.dyaw>>2;

        input.svel -= info.dx>>2;
        input.vel = -info.dz>>2;

        if (!ConPanel)
        {
            if (!HelpInputMode)
            {
                if (buttonMap.ButtonDown(gamefunc_Turn_Left))
                    input.svel += keymove;

                if (buttonMap.ButtonDown(gamefunc_Turn_Right))
                    input.svel += -keymove;

                if (buttonMap.ButtonDown(gamefunc_Move_Forward))
                    input.vel += keymove;

                if (buttonMap.ButtonDown(gamefunc_Move_Backward))
                    input.vel += -keymove;
            }

            if (!InputMode)
            {
                if (buttonMap.ButtonDown(gamefunc_Strafe_Left))
                    input.svel += keymove;

                if (buttonMap.ButtonDown(gamefunc_Strafe_Right))
                    input.svel += -keymove;
            }
        }

        input.vel  = clamp(input.vel, -MAXVEL, MAXVEL);
        input.svel = clamp(input.svel, -MAXSVEL, MAXSVEL);

        pp->mfposx += mulscale9(input.vel,  sintable[NORM_ANGLE(fix16_to_int(pp->q16ang) + 512)]) +
                      mulscale9(input.svel, sintable[NORM_ANGLE(fix16_to_int(pp->q16ang))]);
        pp->mfposy += mulscale9(input.vel,  sintable[NORM_ANGLE(fix16_to_int(pp->q16ang))]) +
                      mulscale9(input.svel, sintable[NORM_ANGLE(fix16_to_int(pp->q16ang) + 1536)]);

        pp->mfposx = max(pp->mfposx, x_min_bound);
        pp->mfposy = max(pp->mfposy, y_min_bound);
        pp->mfposx = min(pp->mfposx, x_max_bound);
        pp->mfposy = min(pp->mfposy, y_max_bound);
    }

    // !JIM! Added M_Active() so that you don't move at all while using menus
    if (M_Active() || ScrollMode2D || InputMode)
        return;

    SET_LOC_KEY(localInput.bits, SK_SPACE_BAR, ((!!inputState.GetKeyStatus(KEYSC_SPACE)) | buttonMap.ButtonDown(gamefunc_Open)));

    info.dz = (info.dz * move_scale)>>8;
    info.dyaw = (info.dyaw * turn_scale)>>8;

    if (buttonMap.ButtonDown(gamefunc_Strafe) && !pp->sop)
    {
        input.svel = -info.mousex;
        input.svel -= info.dyaw * keymove / analogExtent;
    }
    else
    {
        input.q16avel = fix16_sadd(input.q16avel, fix16_sdiv(fix16_from_int(info.mousex), fix16_from_int(32)));
        input.q16avel = fix16_sadd(input.q16avel, fix16_from_int(info.dyaw / analogExtent * (analogTurnAmount << 1)));
    }

    if (mouseaim)
        input.q16horz = fix16_sadd(input.q16horz, fix16_sdiv(fix16_from_int(info.mousey), fix16_from_int(64)));
    else
        input.vel = -(info.mousey >> 6);

    if (!in_mouseflip)
        input.q16horz = -input.q16horz;

    input.q16horz = fix16_ssub(input.q16horz, fix16_from_int(info.dpitch * analogTurnAmount / analogExtent));
    input.svel -= info.dx * keymove / analogExtent;
    input.vel -= info.dz * keymove / analogExtent;

    static double lastInputTicks;
    auto const    currentHiTicks    = timerGetHiTicks();
    double const  elapsedInputTicks = currentHiTicks - lastInputTicks;

    lastInputTicks = currentHiTicks;

    auto scaleAdjustmentToInterval = [=](double x) { return x * (120 / synctics) / (1000.0 / elapsedInputTicks); };

    if (buttonMap.ButtonDown(gamefunc_Strafe) && !pp->sop)
    {
        if (!localInput.svel)
        {
            if (buttonMap.ButtonDown(gamefunc_Turn_Left) && !localInput.svel)
                input.svel = keymove;
            if (buttonMap.ButtonDown(gamefunc_Turn_Right) && !localInput.svel)
                input.svel = -keymove;
        }
    }
    else
    {
        if (buttonMap.ButtonDown(gamefunc_Turn_Left))
        {
            turnheldtime += synctics;
            input.q16avel = fix16_ssub(input.q16avel, fix16_from_float(scaleAdjustmentToInterval((turnheldtime >= TURBOTURNTIME) ? turnamount : PREAMBLETURN)));
        }
        else if (buttonMap.ButtonDown(gamefunc_Turn_Right))
        {
            turnheldtime += synctics;
            input.q16avel = fix16_sadd(input.q16avel, fix16_from_float(scaleAdjustmentToInterval((turnheldtime >= TURBOTURNTIME) ? turnamount : PREAMBLETURN)));
        }
        else
        {
            turnheldtime = 0;
        }
    }

    if (localInput.svel < keymove && localInput.svel > -keymove)
    {
        if (buttonMap.ButtonDown(gamefunc_Strafe_Left) && !pp->sop)
            input.svel += keymove;

        if (buttonMap.ButtonDown(gamefunc_Strafe_Right) && !pp->sop)
            input.svel += -keymove;
    }

    if (localInput.vel < keymove && localInput.vel > -keymove)
    {
        if (buttonMap.ButtonDown(gamefunc_Move_Forward))
            input.vel += keymove;

        if (buttonMap.ButtonDown(gamefunc_Move_Backward))
            input.vel += -keymove;
    }

    localInput.vel  = clamp(localInput.vel + input.vel, -MAXVEL, MAXVEL);
    localInput.svel = clamp(localInput.svel + input.svel, -MAXSVEL, MAXSVEL);

    localInput.q16avel = fix16_clamp(fix16_sadd(localInput.q16avel, input.q16avel), fix16_from_int(-MAXANGVEL), fix16_from_int(MAXANGVEL));
    localInput.q16horz = fix16_clamp(fix16_sadd(localInput.q16horz, input.q16horz), fix16_from_int(-MAXHORIZVEL), fix16_from_int(MAXHORIZVEL));

    if (!TEST(pp->Flags, PF_DEAD))
    {
        if (!TEST(pp->Flags, PF_CLIMBING))
        {
            if (!TEST(pp->Flags, PF_TURN_180))
            {
                if (TEST_SYNC_KEY(pp, SK_TURN_180))
                {
                    if (FLAG_KEY_PRESSED(pp, SK_TURN_180))
                    {
                        fix16_t delta_q16ang;

                        FLAG_KEY_RELEASE(pp, SK_TURN_180);

                        pp->turn180_target = fix16_sadd(pp->q16ang, fix16_from_int(1024)) & 0x7FFFFFF;

                        // make the first turn in the clockwise direction
                        // the rest will follow
                        delta_q16ang = GetDeltaAngleQ16(pp->turn180_target, pp->q16ang);

                        pp->q16ang = fix16_sadd(pp->q16ang, fix16_max(fix16_one,fix16_from_float(scaleAdjustmentToInterval(fix16_to_int(fix16_sdiv(fix16_abs(delta_q16ang), fix16_from_int(TURN_SHIFT))))))) & 0x7FFFFFF;

                        SET(pp->Flags, PF_TURN_180);
                    }
                }
                else
                {
                    FLAG_KEY_RESET(pp, SK_TURN_180);
                }
            }

            if (TEST(pp->Flags, PF_TURN_180))
            {
                fix16_t delta_q16ang;

                delta_q16ang = GetDeltaAngleQ16(pp->turn180_target, pp->q16ang);
                pp->q16ang = fix16_sadd(pp->q16ang, fix16_from_float(scaleAdjustmentToInterval(fix16_to_int(fix16_sdiv(fix16_abs(delta_q16ang), fix16_from_int(TURN_SHIFT)))))) & 0x7FFFFFF;

                sprite[pp->PlayerSprite].ang = fix16_to_int(pp->q16ang);
                if (!Prediction)
                {
                    if (pp->PlayerUnderSprite >= 0)
                        sprite[pp->PlayerUnderSprite].ang = fix16_to_int(pp->q16ang);
                }

                // get new delta to see how close we are
                delta_q16ang = GetDeltaAngleQ16(pp->turn180_target, pp->q16ang);

                if (fix16_abs(delta_q16ang) < fix16_from_int(3 * TURN_SHIFT))
                {
                    pp->q16ang = pp->turn180_target;
                    RESET(pp->Flags, PF_TURN_180);
                }
                else
                    return;
            }

            if (input.q16avel != 0)
            {
               pp->q16ang = fix16_sadd(pp->q16ang, input.q16avel) & 0x7FFFFFF;

                // update players sprite angle
                // NOTE: It's also updated in UpdatePlayerSprite, but needs to be
                // here to cover
                // all cases.
                sprite[pp->PlayerSprite].ang = fix16_to_int(pp->q16ang);
                if (!Prediction)
                {
                    if (pp->PlayerUnderSprite >= 0)
                        sprite[pp->PlayerUnderSprite].ang = fix16_to_int(pp->q16ang);
                }
            }
        }

        // Fixme: This should probably be made optional.
        if (cl_slopetilting)
        {
            int x,y,k,j;
            short tempsect;

            if (!TEST(pp->Flags, PF_FLYING|PF_SWIMMING|PF_DIVING|PF_CLIMBING|PF_JUMPING|PF_FALLING))
            {
                if (!TEST(pp->Flags, PF_MOUSE_AIMING_ON) && TEST(sector[pp->cursectnum].floorstat, FLOOR_STAT_SLOPE)) // If the floor is sloped
                {
                    // Get a point, 512 units ahead of player's position
                    x = pp->posx + (sintable[(fix16_to_int(pp->q16ang) + 512) & 2047] >> 5);
                    y = pp->posy + (sintable[fix16_to_int(pp->q16ang) & 2047] >> 5);
                    tempsect = pp->cursectnum;
                    COVERupdatesector(x, y, &tempsect);

                    if (tempsect >= 0)              // If the new point is inside a valid
                    // sector...
                    {
                        // Get the floorz as if the new (x,y) point was still in
                        // your sector
                        j = getflorzofslope(pp->cursectnum, pp->posx, pp->posy);
                        k = getflorzofslope(pp->cursectnum, x, y);

                        // If extended point is in same sector as you or the slopes
                        // of the sector of the extended point and your sector match
                        // closely (to avoid accidently looking straight out when
                        // you're at the edge of a sector line) then adjust horizon
                        // accordingly
                        if ((pp->cursectnum == tempsect) ||
                            (klabs(getflorzofslope(tempsect, x, y) - k) <= (4 << 8)))
                        {
                            pp->q16horizoff = fix16_sadd(pp->q16horizoff, fix16_from_float(scaleAdjustmentToInterval(mulscale16((j - k), 160))));
                        }
                    }
                }
            }

            if (TEST(pp->Flags, PF_CLIMBING))
            {
                // tilt when climbing but you can't even really tell it
                if (pp->q16horizoff < fix16_from_int(100))
                    pp->q16horizoff = fix16_sadd(pp->q16horizoff, fix16_from_float(scaleAdjustmentToInterval(fix16_to_float(((fix16_from_int(100) - pp->q16horizoff) >> 3) + fix16_one))));
            }
            else
            {
                // Make q16horizoff grow towards 0 since q16horizoff is not modified when
                // you're not on a slope
                if (pp->q16horizoff > 0)
                {
                    pp->q16horizoff = fix16_ssub(pp->q16horizoff, fix16_from_float(scaleAdjustmentToInterval(fix16_to_float((pp->q16horizoff >> 3) + fix16_one))));
                    pp->q16horizoff = fix16_max(pp->q16horizoff, 0);
                }
                else if (pp->q16horizoff < 0)
                {
                    pp->q16horizoff = fix16_sadd(pp->q16horizoff, fix16_from_float(scaleAdjustmentToInterval(fix16_to_float((-pp->q16horizoff >> 3) + fix16_one))));
                    pp->q16horizoff = fix16_min(pp->q16horizoff, 0);
                }
            }
        }

        if (input.q16horz)
        {
            pp->q16horizbase = fix16_sadd(pp->q16horizbase, input.q16horz);
            SET(pp->Flags, PF_LOCK_HORIZ | PF_LOOKING);
        }

        if (TEST_SYNC_KEY(pp, SK_CENTER_VIEW))
        {
            pp->q16horiz = pp->q16horizbase = fix16_from_int(100);
            pp->q16horizoff = 0;
        }

        // this is the locked type
        if (TEST_SYNC_KEY(pp, SK_SNAP_UP) || TEST_SYNC_KEY(pp, SK_SNAP_DOWN))
        {
            // set looking because player is manually looking
            SET(pp->Flags, PF_LOCK_HORIZ | PF_LOOKING);

            // adjust pp->q16horiz negative
            if (TEST_SYNC_KEY(pp, SK_SNAP_DOWN))
                pp->q16horizbase = fix16_ssub(pp->q16horizbase, fix16_from_float(scaleAdjustmentToInterval((HORIZ_SPEED/2))));

            // adjust pp->q16horiz positive
            if (TEST_SYNC_KEY(pp, SK_SNAP_UP))
                pp->q16horizbase = fix16_sadd(pp->q16horizbase, fix16_from_float(scaleAdjustmentToInterval((HORIZ_SPEED/2))));
        }

        // this is the unlocked type
        if (TEST_SYNC_KEY(pp, SK_LOOK_UP) || TEST_SYNC_KEY(pp, SK_LOOK_DOWN))
        {
            RESET(pp->Flags, PF_LOCK_HORIZ);
            SET(pp->Flags, PF_LOOKING);

            // adjust pp->q16horiz negative
            if (TEST_SYNC_KEY(pp, SK_LOOK_DOWN))
                pp->q16horizbase = fix16_ssub(pp->q16horizbase, fix16_from_float(scaleAdjustmentToInterval(HORIZ_SPEED)));

            // adjust pp->q16horiz positive
            if (TEST_SYNC_KEY(pp, SK_LOOK_UP))
                pp->q16horizbase = fix16_sadd(pp->q16horizbase, fix16_from_float(scaleAdjustmentToInterval(HORIZ_SPEED)));
        }

        if (!TEST(pp->Flags, PF_LOCK_HORIZ))
        {
            if (!(TEST_SYNC_KEY(pp, SK_LOOK_UP) || TEST_SYNC_KEY(pp, SK_LOOK_DOWN)))
            {
                // not pressing the pp->q16horiz keys
                if (pp->q16horizbase != fix16_from_int(100))
                {
                    int i;

                    // move pp->q16horiz back to 100
                    for (i = 1; i; i--)
                    {
                        // this formula does not work for pp->q16horiz = 101-103
                        pp->q16horizbase = fix16_sadd(pp->q16horizbase, fix16_from_float(scaleAdjustmentToInterval(fix16_to_float(fix16_ssub(fix16_from_int(25), fix16_sdiv(pp->q16horizbase, fix16_from_int(4)))))));
                    }
                }
                else
                {
                    // not looking anymore because pp->q16horiz is back at 100
                    RESET(pp->Flags, PF_LOOKING);
                }
            }
        }

        // bound the base
        pp->q16horizbase = fix16_max(pp->q16horizbase, fix16_from_int(PLAYER_HORIZ_MIN));
        pp->q16horizbase = fix16_min(pp->q16horizbase, fix16_from_int(PLAYER_HORIZ_MAX));

        // bound adjust q16horizoff
        if (pp->q16horizbase + pp->q16horizoff < fix16_from_int(PLAYER_HORIZ_MIN))
            pp->q16horizoff = fix16_ssub(fix16_from_float(scaleAdjustmentToInterval(PLAYER_HORIZ_MIN)), pp->q16horizbase);
        else if (pp->q16horizbase + pp->q16horizoff > fix16_from_int(PLAYER_HORIZ_MAX))
            pp->q16horizoff = fix16_ssub(fix16_from_float(scaleAdjustmentToInterval(PLAYER_HORIZ_MAX)), pp->q16horizbase);

        // add base and offsets
        pp->q16horiz = fix16_clamp((pp->q16horizbase + pp->q16horizoff), fix16_from_int(PLAYER_HORIZ_MIN), fix16_from_int(PLAYER_HORIZ_MAX));
    }

    if (!CommEnabled)
    {
		// What a mess...:?
#if 0
        if (MenuButtonAutoAim)
        {
            MenuButtonAutoAim = FALSE;
            if ((!!TEST(pp->Flags, PF_AUTO_AIM)) != !!cl_autoaim)
                SET_LOC_KEY(localInput.bits, SK_AUTO_AIM, TRUE);
        }
#endif
    }
    else if (inputState.GetKeyStatus(sc_Pause))
    {
        SET_LOC_KEY(localInput.bits, SK_PAUSE, inputState.GetKeyStatus(sc_Pause));
		inputState.ClearKeyStatus(sc_Pause);
	}

    SET_LOC_KEY(localInput.bits, SK_CENTER_VIEW, buttonMap.ButtonDown(gamefunc_Center_View));

    SET_LOC_KEY(localInput.bits, SK_RUN, buttonMap.ButtonDown(gamefunc_Run));
    SET_LOC_KEY(localInput.bits, SK_SHOOT, buttonMap.ButtonDown(gamefunc_Fire));

    // actually snap
    SET_LOC_KEY(localInput.bits, SK_SNAP_UP, buttonMap.ButtonDown(gamefunc_Aim_Up));
    SET_LOC_KEY(localInput.bits, SK_SNAP_DOWN, buttonMap.ButtonDown(gamefunc_Aim_Down));

    // actually just look
    SET_LOC_KEY(localInput.bits, SK_LOOK_UP, buttonMap.ButtonDown(gamefunc_Look_Up));
    SET_LOC_KEY(localInput.bits, SK_LOOK_DOWN, buttonMap.ButtonDown(gamefunc_Look_Down));

    for (i = 0; i < MAX_WEAPONS_KEYS; i++)
    {
        if (buttonMap.ButtonDown(gamefunc_Weapon_1 + i))
        {
            SET(localInput.bits, i + 1);
            break;
        }
    }

    if (buttonMap.ButtonDown(gamefunc_Next_Weapon))
    {
        USERp u = User[pp->PlayerSprite];
        short next_weapon = u->WeaponNum + 1;
        short start_weapon;

        buttonMap.ClearButton(gamefunc_Next_Weapon);

        start_weapon = u->WeaponNum + 1;

        if (u->WeaponNum == WPN_SWORD)
            start_weapon = WPN_STAR;

        if (u->WeaponNum == WPN_FIST)
        {
            next_weapon = 14;
        }
        else
        {
            next_weapon = -1;
            for (i = start_weapon; TRUE; i++)
            {
                if (i >= MAX_WEAPONS_KEYS)
                {
                    next_weapon = 13;
                    break;
                }

                if (TEST(pp->WpnFlags, BIT(i)) && pp->WpnAmmo[i])
                {
                    next_weapon = i;
                    break;
                }
            }
        }

        SET(localInput.bits, next_weapon + 1);
    }


    if (buttonMap.ButtonDown(gamefunc_Previous_Weapon))
    {
        USERp u = User[pp->PlayerSprite];
        short prev_weapon = u->WeaponNum - 1;
        short start_weapon;

        buttonMap.ClearButton(gamefunc_Previous_Weapon);

        start_weapon = u->WeaponNum - 1;

        if (u->WeaponNum == WPN_SWORD)
        {
            prev_weapon = 13;
        }
        else if (u->WeaponNum == WPN_STAR)
        {
            prev_weapon = 14;
        }
        else
        {
            prev_weapon = -1;
            for (i = start_weapon; TRUE; i--)
            {
                if (i <= -1)
                    i = WPN_HEART;

                if (TEST(pp->WpnFlags, BIT(i)) && pp->WpnAmmo[i])
                {
                    prev_weapon = i;
                    break;
                }
            }
        }

        SET(localInput.bits, prev_weapon + 1);
    }

    if (buttonMap.ButtonDown(gamefunc_Alt_Weapon))
    {
        buttonMap.ClearButton(gamefunc_Alt_Weapon);
        USERp u = User[pp->PlayerSprite];
        short const which_weapon = u->WeaponNum + 1;
        SET(localInput.bits, which_weapon);
    }


    inv_hotkey = 0;
    if (buttonMap.ButtonDown(gamefunc_MedKit))
        inv_hotkey = INVENTORY_MEDKIT+1;
    if (buttonMap.ButtonDown(gamefunc_Smoke_Bomb))
        inv_hotkey = INVENTORY_CLOAK+1;
    if (buttonMap.ButtonDown(gamefunc_NightVision))
        inv_hotkey = INVENTORY_NIGHT_VISION+1;
    if (buttonMap.ButtonDown(gamefunc_Gas_Bomb))
        inv_hotkey = INVENTORY_CHEMBOMB+1;
    if (buttonMap.ButtonDown(gamefunc_Flash_Bomb) && dimensionmode == 3)
        inv_hotkey = INVENTORY_FLASHBOMB+1;
    if (buttonMap.ButtonDown(gamefunc_Caltrops))
        inv_hotkey = INVENTORY_CALTROPS+1;

    SET(localInput.bits, inv_hotkey<<SK_INV_HOTKEY_BIT0);

    SET_LOC_KEY(localInput.bits, SK_INV_USE, buttonMap.ButtonDown(gamefunc_Inventory));

    SET_LOC_KEY(localInput.bits, SK_OPERATE, buttonMap.ButtonDown(gamefunc_Open));
    SET_LOC_KEY(localInput.bits, SK_JUMP, buttonMap.ButtonDown(gamefunc_Jump));
    SET_LOC_KEY(localInput.bits, SK_CRAWL, buttonMap.ButtonDown(gamefunc_Crouch));

    SET_LOC_KEY(localInput.bits, SK_TURN_180, buttonMap.ButtonDown(gamefunc_TurnAround));

    SET_LOC_KEY(localInput.bits, SK_INV_LEFT, buttonMap.ButtonDown(gamefunc_Inventory_Left));
    SET_LOC_KEY(localInput.bits, SK_INV_RIGHT, buttonMap.ButtonDown(gamefunc_Inventory_Right));

    SET_LOC_KEY(localInput.bits, SK_HIDE_WEAPON, buttonMap.ButtonDown(gamefunc_Holster_Weapon));

    // need BUTTON
    SET_LOC_KEY(localInput.bits, SK_CRAWL_LOCK, inputState.GetKeyStatus(KEYSC_NUM));

    if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
    {
        if (buttonMap.ButtonDown(gamefunc_See_Coop_View))
        {
            buttonMap.ClearButton(gamefunc_See_Coop_View);

            screenpeek = connectpoint2[screenpeek];

            if (screenpeek < 0)
                screenpeek = connecthead;

            if (dimensionmode != 2 && screenpeek == playerNum)
            {
                // JBF: figure out what's going on here
                DoPlayerDivePalette(pp);  // Check Dive again
                DoPlayerNightVisionPalette(pp);  // Check Night Vision again
            }
            else
            {
                PLAYERp tp = Player+screenpeek;
                DoPlayerDivePalette(tp);
                DoPlayerNightVisionPalette(tp);
            }
        }
    }

#if DEBUG
    DebugKeys(pp);

    if (!CommEnabled)                   // Single player only keys
        SinglePlayInput(pp);
#endif

    FunctionKeys(pp);

    if (buttonMap.ButtonDown(gamefunc_Toggle_Crosshair))
    {
        buttonMap.ClearButton(gamefunc_Toggle_Crosshair);
        pToggleCrosshair();
    }
}

#define MAP_WHITE_SECTOR    (LT_GREY + 2)
#define MAP_RED_SECTOR      (RED + 6)
#define MAP_FLOOR_SPRITE    (RED + 8)
#define MAP_ENEMY           (RED + 10)
#define MAP_SPRITE          (FIRE + 8)
#define MAP_PLAYER          (GREEN + 6)

#define MAP_BLOCK_SPRITE    (DK_BLUE + 6)

void drawoverheadmap(int cposx, int cposy, int czoom, short cang)
{
    int i, j, k, l, x1, y1, x2, y2, x3, y3, x4, y4, ox, oy, xoff, yoff;
    int dax, day, cosang, sinang, xspan, yspan, sprx, spry;
    int xrepeat, yrepeat, z1, z2, startwall, endwall, tilenum, daang;
    int xvect, yvect, xvect2, yvect2;
    char col;
    walltype *wal, *wal2;
    spritetype *spr;
    short p;
    static int pspr_ndx[8]= {0,0,0,0,0,0,0,0};
    SWBOOL sprisplayer = FALSE;
    short txt_x, txt_y;

    int32_t tmpydim = (xdim * 5) / 8;
    renderSetAspect(65536, divscale16(tmpydim * 320, xdim * 200));

    // draw location text
    if (gs.BorderNum <= BORDER_BAR-1)
    {
        txt_x = 7;
        txt_y = 168;
    }
    else
    {
        txt_x = 7;
        txt_y = 147;
    }

    if (ScrollMode2D)
    {
        minigametext(txt_x,txt_y-7,"Follow Mode",2+8);
    }

    sprintf(ds,"%s",currentLevel->DisplayName());

    minigametext(txt_x,txt_y,ds,2+8);

    //////////////////////////////////

    xvect = sintable[(2048 - cang) & 2047] * czoom;
    yvect = sintable[(1536 - cang) & 2047] * czoom;
    xvect2 = mulscale16(xvect, yxaspect);
    yvect2 = mulscale16(yvect, yxaspect);

    // Draw red lines
    for (i = 0; i < numsectors; i++)
    {
        startwall = sector[i].wallptr;
        endwall = sector[i].wallptr + sector[i].wallnum - 1;

        z1 = sector[i].ceilingz;
        z2 = sector[i].floorz;

        for (j = startwall, wal = &wall[startwall]; j <= endwall; j++, wal++)
        {
            k = wal->nextwall;
            if ((unsigned)k >= MAXWALLS)
                continue;

            if (!mapcheat)
            {
                if ((show2dwall[j >> 3] & (1 << (j & 7))) == 0)
                    continue;
                if ((k > j) && ((show2dwall[k >> 3] & (1 << (k & 7))) > 0))
                    continue;
            }

            if (sector[wal->nextsector].ceilingz == z1)
                if (sector[wal->nextsector].floorz == z2)
                    if (((wal->cstat | wall[wal->nextwall].cstat) & (16 + 32)) == 0)
                        continue;

            col = 152;

            //if (dimensionmode == 2)
            if (dimensionmode == 6)
            {
                if (sector[i].floorz != sector[i].ceilingz)
                    if (sector[wal->nextsector].floorz != sector[wal->nextsector].ceilingz)
                        if (((wal->cstat | wall[wal->nextwall].cstat) & (16 + 32)) == 0)
                            if (sector[i].floorz == sector[wal->nextsector].floorz)
                                continue;
                if (sector[i].floorpicnum != sector[wal->nextsector].floorpicnum)
                    continue;
                if (sector[i].floorshade != sector[wal->nextsector].floorshade)
                    continue;
                col = 12;  // 1=white / 31=black / 44=green / 56=pink / 128=yellow / 210=blue / 248=orange / 255=purple
            }

            ox = wal->x - cposx;
            oy = wal->y - cposy;
            x1 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
            y1 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

            wal2 = &wall[wal->point2];
            ox = wal2->x - cposx;
            oy = wal2->y - cposy;
            x2 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
            y2 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

            renderDrawLine(x1 + (xdim << 11), y1 + (ydim << 11), x2 + (xdim << 11), y2 + (ydim << 11), col);
        }
    }

    // Draw sprites
    k = Player[screenpeek].PlayerSprite;
    for (i = 0; i < numsectors; i++)
        for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
        {
            for (p=connecthead; p >= 0; p=connectpoint2[p])
            {
                if (Player[p].PlayerSprite == j)
                {
                    if (sprite[Player[p].PlayerSprite].xvel > 16)
                        pspr_ndx[myconnectindex] = (((int32_t) totalclock>>4)&3);
                    sprisplayer = TRUE;

                    goto SHOWSPRITE;
                }
            }
            if (mapcheat || (show2dsprite[j >> 3] & (1 << (j & 7))) > 0)
            {
SHOWSPRITE:
                spr = &sprite[j];

                col = 56; // 1=white / 31=black / 44=green / 56=pink / 128=yellow / 210=blue / 248=orange / 255=purple
                if ((spr->cstat & 1) > 0)
                    col = 248;
                if (j == k)
                    col = 31;

                sprx = spr->x;
                spry = spr->y;

                k = spr->statnum;
                if ((k >= 1) && (k <= 8) && (k != 2))   // Interpolate moving
                {
                    sprx = sprite[j].x;
                    spry = sprite[j].y;
                }

                switch (spr->cstat & 48)
                {
                case 0:  // Regular sprite
                    if (Player[p].PlayerSprite == j)
                    {
                        ox = sprx - cposx;
                        oy = spry - cposy;
                        x1 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
                        y1 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

                        if (dimensionmode == 5 && (gNet.MultiGameType != MULTI_GAME_COMMBAT || j == Player[screenpeek].PlayerSprite))
                        {
                            ox = (sintable[(spr->ang + 512) & 2047] >> 7);
                            oy = (sintable[(spr->ang) & 2047] >> 7);
                            x2 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
                            y2 = mulscale16(oy, xvect) + mulscale16(ox, yvect);

                            if (j == Player[screenpeek].PlayerSprite)
                            {
                                x2 = 0L;
                                y2 = -(czoom << 5);
                            }

                            x3 = mulscale16(x2, yxaspect);
                            y3 = mulscale16(y2, yxaspect);

                            renderDrawLine(x1 - x2 + (xdim << 11), y1 - y3 + (ydim << 11),
                                           x1 + x2 + (xdim << 11), y1 + y3 + (ydim << 11), col);
                            renderDrawLine(x1 - y2 + (xdim << 11), y1 + x3 + (ydim << 11),
                                           x1 + x2 + (xdim << 11), y1 + y3 + (ydim << 11), col);
                            renderDrawLine(x1 + y2 + (xdim << 11), y1 - x3 + (ydim << 11),
                                           x1 + x2 + (xdim << 11), y1 + y3 + (ydim << 11), col);
                        }
                        else
                        {
                            if (((gotsector[i >> 3] & (1 << (i & 7))) > 0) && (czoom > 192))
                            {
                                daang = (spr->ang - cang) & 2047;
                                if (j == Player[screenpeek].PlayerSprite)
                                {
                                    x1 = 0;
                                    //y1 = (yxaspect << 2);
                                    y1 = 0;
                                    daang = 0;
                                }

                                // Special case tiles
                                if (spr->picnum == 3123) break;

                                if (sprisplayer)
                                {
                                    if (gNet.MultiGameType != MULTI_GAME_COMMBAT || j == Player[screenpeek].PlayerSprite)
                                        rotatesprite((x1 << 4) + (xdim << 15), (y1 << 4) + (ydim << 15), mulscale16(czoom * (spr->yrepeat), yxaspect), daang, 1196+pspr_ndx[myconnectindex], spr->shade, spr->pal, (spr->cstat & 2) >> 1, windowxy1.x, windowxy1.y, windowxy2.x, windowxy2.y);
                                }
                                else
                                    rotatesprite((x1 << 4) + (xdim << 15), (y1 << 4) + (ydim << 15), mulscale16(czoom * (spr->yrepeat), yxaspect), daang, spr->picnum, spr->shade, spr->pal, (spr->cstat & 2) >> 1, windowxy1.x, windowxy1.y, windowxy2.x, windowxy2.y);
                            }
                        }
                    }
                    break;
                case 16: // Rotated sprite
                    x1 = sprx;
                    y1 = spry;
                    tilenum = spr->picnum;
                    xoff = (int)picanm[tilenum].xofs + (int)spr->xoffset;
                    if ((spr->cstat & 4) > 0)
                        xoff = -xoff;
                    k = spr->ang;
                    l = spr->xrepeat;
                    dax = sintable[k & 2047] * l;
                    day = sintable[(k + 1536) & 2047] * l;
                    l = tilesiz[tilenum].x;
                    k = (l >> 1) + xoff;
                    x1 -= mulscale16(dax, k);
                    x2 = x1 + mulscale16(dax, l);
                    y1 -= mulscale16(day, k);
                    y2 = y1 + mulscale16(day, l);

                    ox = x1 - cposx;
                    oy = y1 - cposy;
                    x1 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
                    y1 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

                    ox = x2 - cposx;
                    oy = y2 - cposy;
                    x2 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
                    y2 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

                    renderDrawLine(x1 + (xdim << 11), y1 + (ydim << 11),
                                   x2 + (xdim << 11), y2 + (ydim << 11), col);

                    break;
                case 32:    // Floor sprite
                    if (dimensionmode == 5)
                    {
                        tilenum = spr->picnum;
                        xoff = (int)picanm[tilenum].xofs + (int)spr->xoffset;
                        yoff = (int)picanm[tilenum].yofs + (int)spr->yoffset;
                        if ((spr->cstat & 4) > 0)
                            xoff = -xoff;
                        if ((spr->cstat & 8) > 0)
                            yoff = -yoff;

                        k = spr->ang;
                        cosang = sintable[(k + 512) & 2047];
                        sinang = sintable[k];
                        xspan = tilesiz[tilenum].x;
                        xrepeat = spr->xrepeat;
                        yspan = tilesiz[tilenum].y;
                        yrepeat = spr->yrepeat;

                        dax = ((xspan >> 1) + xoff) * xrepeat;
                        day = ((yspan >> 1) + yoff) * yrepeat;
                        x1 = sprx + mulscale16(sinang, dax) + mulscale16(cosang, day);
                        y1 = spry + mulscale16(sinang, day) - mulscale16(cosang, dax);
                        l = xspan * xrepeat;
                        x2 = x1 - mulscale16(sinang, l);
                        y2 = y1 + mulscale16(cosang, l);
                        l = yspan * yrepeat;
                        k = -mulscale16(cosang, l);
                        x3 = x2 + k;
                        x4 = x1 + k;
                        k = -mulscale16(sinang, l);
                        y3 = y2 + k;
                        y4 = y1 + k;

                        ox = x1 - cposx;
                        oy = y1 - cposy;
                        x1 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
                        y1 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

                        ox = x2 - cposx;
                        oy = y2 - cposy;
                        x2 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
                        y2 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

                        ox = x3 - cposx;
                        oy = y3 - cposy;
                        x3 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
                        y3 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

                        ox = x4 - cposx;
                        oy = y4 - cposy;
                        x4 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
                        y4 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

                        renderDrawLine(x1 + (xdim << 11), y1 + (ydim << 11),
                                       x2 + (xdim << 11), y2 + (ydim << 11), col);

                        renderDrawLine(x2 + (xdim << 11), y2 + (ydim << 11),
                                       x3 + (xdim << 11), y3 + (ydim << 11), col);

                        renderDrawLine(x3 + (xdim << 11), y3 + (ydim << 11),
                                       x4 + (xdim << 11), y4 + (ydim << 11), col);

                        renderDrawLine(x4 + (xdim << 11), y4 + (ydim << 11),
                                       x1 + (xdim << 11), y1 + (ydim << 11), col);

                    }
                    break;
                }
            }
        }
    // Draw white lines
    for (i = 0; i < numsectors; i++)
    {
        startwall = sector[i].wallptr;
        endwall = sector[i].wallptr + sector[i].wallnum - 1;

        for (j = startwall, wal = &wall[startwall]; j <= endwall; j++, wal++)
        {
            if ((uint16_t)wal->nextwall < MAXWALLS)
                continue;

            if (!mapcheat && (show2dwall[j >> 3] & (1 << (j & 7))) == 0)
                continue;

            if (tilesiz[wal->picnum].x == 0)
                continue;
            if (tilesiz[wal->picnum].y == 0)
                continue;

            ox = wal->x - cposx;
            oy = wal->y - cposy;
            x1 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
            y1 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

            wal2 = &wall[wal->point2];
            ox = wal2->x - cposx;
            oy = wal2->y - cposy;
            x2 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
            y2 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

            renderDrawLine(x1 + (xdim << 11), y1 + (ydim << 11), x2 + (xdim << 11), y2 + (ydim << 11), 24);
        }
    }

    videoSetCorrectedAspect();

}


#if RANDOM_DEBUG
int RandomRange(int range, char *file, unsigned line)
{
    uint32_t rand_num;
    uint32_t value;
    extern FILE *fout_err;
    extern uint32_t MoveThingsCount;

    if (RandomPrint && !Prediction)
    {
        sprintf(ds,"mtc %d, %s, line %d, %d",MoveThingsCount,file,line,randomseed);
        DebugWriteString(ds);
    }

    if (range <= 0)
        return 0;

    rand_num = krand2();

    if (rand_num == 65535U)
        rand_num--;

    // shift values to give more precision
    value = (rand_num << 14) / ((65535UL << 14) / range);

    if (value >= range)
        value = range - 1;

    return value;
}
#else
int RandomRange(int range)
{
    uint32_t rand_num;
    uint32_t value;

    if (range <= 0)
        return 0;

    rand_num = RANDOM();

    if (rand_num == 65535U)
        rand_num--;

    // shift values to give more precision
    value = (rand_num << 14) / ((65535UL << 14) / range);

    if (value >= (uint32_t)range)
        value = range - 1;

    return value;
}
#endif

int StdRandomRange(int range)
{
    uint32_t rand_num;
    uint32_t value;

    if (range <= 0)
        return 0;

    rand_num = STD_RANDOM();

    if (rand_num == RAND_MAX)
        rand_num--;

    // shift values to give more precision
#if (RAND_MAX > 0x7fff)
    value = rand_num / (((int)RAND_MAX) / range);
#else
    value = (rand_num << 14) / ((((int)RAND_MAX) << 14) / range);
#endif

    if (value >= (uint32_t)range)
        value = range - 1;

    return value;
}

// [JM] Probably will need some doing over. !CHECKME!
void G_Polymer_UnInit(void) { }


#include "saveable.h"

saveable_module saveable_build{};

void Saveable_Init_Dynamic()
{
    static saveable_data saveable_build_data[] =
    {
        {sector, MAXSECTORS*sizeof(sectortype)},
        {sprite, MAXSPRITES*sizeof(spritetype)},
        {wall, MAXWALLS*sizeof(walltype)},
    };

    saveable_build.data = saveable_build_data;
    saveable_build.numdata = NUM_SAVEABLE_ITEMS(saveable_build_data);
}

void GameInterface::set_hud_layout(int requested_size) 
{
    gs.BorderNum = 9 - requested_size;
    SetBorder(Player + myconnectindex, gs.BorderNum);
    SetRedrawScreen(Player + myconnectindex);
}
/*extern*/ void GameInterface::set_hud_scale(int requested_size) {  }

::GameInterface* CreateInterface()
{
	return new GameInterface;
}


GameStats GameInterface::getStats()
{
	PLAYERp pp = Player + myconnectindex;
	return { pp->Kills, TotalKillable, pp->SecretsFound, LevelSecrets, PlayClock / 120, 0 };
}

void GameInterface::FreeGameData()
{
    TerminateLevel();
}

void GameInterface::UpdateScreenSize()
{
    SetupAspectRatio();
}


#if 0 // the message input needs to be moved out of the game code!
void GetMessageInput(PLAYERp pp)
{
    int pnum = myconnectindex;
    short w, h;
    static SWBOOL cur_show;
    static SWBOOL TeamSendAll, TeamSendTeam;
#define TEAM_MENU "A - Send to ALL,  T - Send to TEAM"
    static char HoldMessageInputString[256];
    int i;

    if (!MessageInputMode && !ConInputMode)
    {
        if (buttonMap.ButtonDown(gamefunc_SendMessage))
        {
            buttonMap.ClearButton(gamefunc_SendMessage);
            inputState.keyFlushChars();
            MessageInputMode = TRUE;
            InputMode = TRUE;
            TeamSendTeam = FALSE;
            TeamSendAll = FALSE;

            if (MessageInputMode)
            {
                memset(MessageInputString, '\0', sizeof(MessageInputString));
            }
        }
    }

    else if (MessageInputMode && !ConInputMode)
    {
        if (gs.BorderNum > BORDER_BAR + 1)
            SetRedrawScreen(pp);

        // get input
        switch (MNU_InputSmallString(MessageInputString, 320 - 20))
        {
        case -1: // Cancel Input (pressed ESC) or Err
            MessageInputMode = FALSE;
            InputMode = FALSE;
            inputState.ClearAllInput();
            break;
        case FALSE: // Input finished (RETURN)
            if (MessageInputString[0] == '\0')
            {
                // no input
                MessageInputMode = FALSE;
                InputMode = FALSE;
                inputState.ClearAllInput();
                buttonMap.ClearButton(gamefunc_Inventory);
            }
            else
            {
                if (gNet.TeamPlay)
                {
                    if (memcmp(MessageInputString, TEAM_MENU, sizeof(TEAM_MENU)) != 0)
                    {
                        {
                            strcpy(HoldMessageInputString, MessageInputString);
                            strcpy(MessageInputString, TEAM_MENU);
                            break;
                        }
                    }
                    else if (memcmp(MessageInputString, TEAM_MENU, sizeof(TEAM_MENU)) == 0)
                    {
                        strcpy(MessageInputString, HoldMessageInputString);
                        TeamSendAll = TRUE;
                    }
                }

            SEND_MESSAGE:

                // broadcast message
                MessageInputMode = FALSE;
                InputMode = FALSE;
                inputState.ClearAllInput();

                for (i = 0; i < NUMGAMEFUNCTIONS; i++)
                    buttonMap.ClearButton(i);

                // Put who sent this
                sprintf(ds, "%s: %s", pp->PlayerName, MessageInputString);

                if (gNet.TeamPlay)
                {
                    TRAVERSE_CONNECT(pnum)
                    {
                        if (pnum != myconnectindex)
                        {
                            if (TeamSendAll)
                                SW_SendMessage(pnum, ds);
                            else if (User[pp->PlayerSprite]->spal == User[Player[pnum].PlayerSprite]->spal)
                                SW_SendMessage(pnum, ds);
                        }
                    }
                }
                else
                    TRAVERSE_CONNECT(pnum)
                {
                    if (pnum != myconnectindex)
                    {
                        SW_SendMessage(pnum, ds);
                    }
                }
                adduserquote(MessageInputString);
                quotebot += 8;
                quotebotgoal = quotebot;
            }
            break;

        case TRUE: // Got input

            if (gNet.TeamPlay)
            {
                if (memcmp(MessageInputString, TEAM_MENU "a", sizeof(TEAM_MENU) + 1) == 0)
                {
                    strcpy(MessageInputString, HoldMessageInputString);
                    TeamSendAll = TRUE;
                    goto SEND_MESSAGE;
                }
                else if (memcmp(MessageInputString, TEAM_MENU "t", sizeof(TEAM_MENU) + 1) == 0)
                {
                    strcpy(MessageInputString, HoldMessageInputString);
                    TeamSendTeam = TRUE;
                    goto SEND_MESSAGE;
                }
                else
                {
                    // reset the string if anything else is typed
                    if (strlen(MessageInputString) + 1 > sizeof(TEAM_MENU))
                    {
                        strcpy(MessageInputString, TEAM_MENU);
                    }
                }
            }

            break;
        }
    }
}
#endif

END_SW_NS
