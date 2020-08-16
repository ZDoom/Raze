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

#define MAIN
#define QUIET
#include "build.h"
#include "baselayer.h"

#include "baselayer.h"

#include "names2.h"
#include "panel.h"
#include "game.h"
#include "interp.h"
#include "interpso.h"
#include "tags.h"
#include "sector.h"
#include "sprite.h"
#include "weapon.h"
#include "player.h"
#include "lists.h"
#include "network.h"
#include "pal.h"


#include "mytypes.h"

#include "menus.h"

#include "gamecontrol.h"

#include "misc.h"

#include "misc.h"
#include "break.h"
#include "ninja.h"
#include "light.h"
#include "misc.h"
#include "jsector.h"

#include "common.h"
#include "gameconfigfile.h"
#include "printf.h"
#include "m_argv.h"
#include "debugbreak.h"
#include "menu.h"
#include "raze_music.h"
#include "statistics.h"
#include "gstrings.h"
#include "mapinfo.h"
#include "v_video.h"
#include "raze_sound.h"
#include "secrets.h"

#include "osdcmds.h"
#include "screenjob.h"
#include "inputstate.h"
#include "gamestate.h"

//#include "crc32.h"

CVAR(Bool, sw_ninjahack, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG);
CVAR(Bool, sw_darts, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG);

BEGIN_SW_NS

void pClearSpriteList(PLAYERp pp);

extern int sw_snd_scratch;

int GameVersion = 20;

int Follow_posx=0,Follow_posy=0;

SWBOOL NoMeters = FALSE;
SWBOOL FinishAnim = 0;
SWBOOL ReloadPrompt = FALSE;
SWBOOL NewGame = TRUE;
SWBOOL InMenuLevel = FALSE;
SWBOOL LoadGameOutsideMoveLoop = FALSE;
//Miscellaneous variables
SWBOOL FinishedLevel = FALSE;
short screenpeek = 0;

SWBOOL PedanticMode;

SWBOOL LocationInfo = 0;
void drawoverheadmap(int cposx, int cposy, int czoom, short cang);
SWBOOL PreCaching = TRUE;
int GodMode = FALSE;
short Skill = 2;
short TotalKillable;

const GAME_SET gs_defaults =
{
// Network game settings
    0, // GameType
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
SWBOOL SlowMode = FALSE;
SWBOOL FrameAdvanceTics = 3;
SWBOOL ScrollMode2D = FALSE;

SWBOOL DebugOperate = FALSE;
void LoadingLevelScreen(void);

uint8_t FakeMultiNumPlayers;

int totalsynctics;

MapRecord* NextLevel = nullptr;
SWBOOL ExitLevel = FALSE;
int OrigCommPlayers=0;
extern uint8_t CommPlayers;
extern SWBOOL CommEnabled;
extern int bufferjitter;

SWBOOL CameraTestMode = FALSE;

char ds[645];                           // debug string

extern short NormalVisibility;
SWBOOL CommandSetup = FALSE;

char buffer[80], ch;

uint8_t DebugPrintColor = 255;

FString ThemeSongs[6];
int ThemeTrack[6];

/// L O C A L   P R O T O T Y P E S /////////////////////////////////////////////////////////
void SybexScreen(void);
void MenuLevel(void);
void StatScreen(PLAYERp mpp);
void InitRunLevel(void);
void RunLevel(void);
/////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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
    "Toggle_Crouch",	// This is the last one used by EDuke32"",

};

bool InitGame()
{
    InitCheats();
    buttonMap.SetButtons(actions, NUM_ACTIONS);
    automapping = 1;

    gs = gs_defaults;

    for (int i = 0; i < MAX_SW_PLAYERS; i++)
        INITLIST(&Player[i].PanelSpriteList);

    DebugOperate = TRUE;
    enginecompatibility_mode = ENGINECOMPATIBILITY_19961112;

    if (SW_SHAREWARE)
        Printf("SHADOW WARRIOR(tm) Version 1.2 (Shareware Version)\n");
    else
        Printf("SHADOW WARRIOR(tm) Version 1.2\n");

    if (sw_snd_scratch == 0)    // This is always 0 at this point - this check is only here to prevent whole program optimization from eliminating the variable.
        Printf("Copyright (c) 1997 3D Realms Entertainment\n");

    registerosdcommands();
    registerinputcommands();

    engineInit();
    auto pal = fileSystem.LoadFile("3drealms.pal", 0);
    if (pal.Size() >= 768)
    {
        for (auto& c : pal)
            c <<= 2;

        paletteSetColorTable(DREALMSPAL, pal.Data(), true, true);
    }
    timerInit(120);
    InitPalette();
    // sets numplayers, connecthead, connectpoint2, myconnectindex

	numplayers = 1; myconnectindex = 0;
	connecthead = 0; connectpoint2[0] = -1;

    if (SW_SHAREWARE && numplayers > 4)
    {
        I_FatalError("To play a Network game with more than 4 players you must purchase "
            "the full version.  Read the Ordering Info screens for details.");
    }

    TileFiles.LoadArtSet("tiles%03d.art");
    InitFonts();

    //Connect();
    SortBreakInfo();
    parallaxtype = 1;
    SW_InitMultiPsky();

    memset(Track, 0, sizeof(Track));
    memset(Player, 0, sizeof(Player));
    for (int i = 0; i < MAX_SW_PLAYERS; i++)
        INITLIST(&Player[i].PanelSpriteList);

    LoadKVXFromScript("swvoxfil.txt");    // Load voxels from script file
    LoadPLockFromScript("swplock.txt");   // Get Parental Lock setup info
	LoadCustomInfoFromScript("engine/swcustom.txt");	// load the internal definitions. These also apply to the shareware version.
    if (!SW_SHAREWARE)
        LoadCustomInfoFromScript("swcustom.txt");   // Load user customisation information
 
    if (!loaddefinitionsfile(G_DefFile())) Printf("Definitions file loaded.\n");
	userConfig.AddDefs.reset();
    enginePostInit();
    videoInit();
    InitFX();
	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool LoadLevel(MapRecord* maprec)
{
    int16_t ang;
    if (engineLoadBoard(maprec->fileName, SW_SHAREWARE ? 1 : 0, (vec3_t*)&Player[0], &ang, &Player[0].cursectnum) == -1)
    {
        Printf("Map not found: %s", maprec->fileName.GetChars());
        return false;
    }
    currentLevel = maprec;
    SECRET_SetMapName(currentLevel->DisplayName(), currentLevel->name);
    STAT_NewLevel(currentLevel->fileName);
    Player[0].q16ang = fix16_from_int(ang);
    return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitLevelGlobals(void)
{
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
    numinterpolations = short_numinterpolations = 0;

    gNet.TimeLimitClock = gNet.TimeLimit;

    serpwasseen = FALSE;
    sumowasseen = FALSE;
    zillawasseen = FALSE;
    memset(BossSpriteNum,-1,sizeof(BossSpriteNum));

    PedanticMode = false;
}

//---------------------------------------------------------------------------
//
// GLOBAL RESETS NOT DONE for LOAD GAME
//
//---------------------------------------------------------------------------

void InitLevelGlobals2(void)
{
    InitTimingVars();
    TotalKillable = 0;
    Bunny_Count = 0;
    FinishAnim = 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitPlayerGameSettings(void)
{
    int pnum;

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitRunLevel(void)
{
    if (LoadGameOutsideMoveLoop)
    {
        int SavePlayClock;
        extern int PlayClock;
        LoadGameOutsideMoveLoop = FALSE;
        // contains what is needed from calls below
        if (snd_ambience)
            StartAmbientSound();
        // crappy little hack to prevent play clock from being overwritten
        // for load games
        SavePlayClock = PlayClock;
        InitTimingVars();
        PlayClock = SavePlayClock;
        return;
    }

    //SendVersion(GameVersion);
    //waitforeverybody();

    Mus_Stop();

    DoTheCache();

    // auto aim / auto run / etc
    InitPlayerGameSettings();

    // send packets with player info
    InitNetPlayerOptions();

    // Initialize Game part of network code (When ready2send != 0)
    InitNetVars();

    if (currentLevel)
    {
        PlaySong(currentLevel->labelName, currentLevel->music, currentLevel->cdSongId);
    }

    InitPrediction(&Player[myconnectindex]);

    waitforeverybody();

    //CheckVersion(GameVersion);

    // IMPORTANT - MUST be right before game loop AFTER waitforeverybody
    InitTimingVars();

    if (snd_ambience)
        StartAmbientSound();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void MoveTicker(void)
{
    int pnum;

    //getpackets();

    if (PredictionOn && CommEnabled)
    {
        while (predictmovefifoplc < Player[myconnectindex].movefifoend)
        {
            DoPrediction(ppp);
        }
    }

    //While you have new input packets to process...
    if (!CommEnabled)
        bufferjitter = 0;

    while (Player[myconnectindex].movefifoend - movefifoplc > bufferjitter)
    {
        //Make sure you have at least 1 packet from everyone else
        for (pnum = connecthead; pnum >= 0; pnum = connectpoint2[pnum])
        {
            if (movefifoplc == Player[pnum].movefifoend)
            {
                break;
            }
        }

        //Pnum is >= 0 only if last loop was broken, meaning a player wasn't caught up
        if (pnum >= 0)
            break;

        domovethings();

    }
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------


void InitLevel(void)
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

    Mus_Stop();

    auto maprec = NextLevel;
    NextLevel = nullptr;
    if (!maprec) maprec = currentLevel;
    if (!maprec)
    {
        NewGame = false;
        return;
    }
    InitLevelGlobals2();

    if (NewGame)
        InitNewGame();

    if (!LoadLevel(maprec))
	{
		NewGame = false;
		return;
	}

    SetupPreCache();

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
    
    // reset NewGame
    NewGame = FALSE;
}


void
TerminateLevel(void)
{
    if (!currentLevel) return;

    int i, nexti, stat, pnum, ndx;
    SECT_USERp *sectu;

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
        memset(pp->InventoryTics, 0, sizeof(pp->InventoryTics));

        pp->Killer = -1;

        INITLIST(&pp->PanelSpriteList);
    }

    JS_UnInitLockouts();

//HEAP_CHECK();
}

void NewLevel(void)
{
    do
    {
        InitLevel();
        RunLevel();
    }
    while (LoadGameOutsideMoveLoop);
	STAT_Update(false);

    // for good measure do this
    ready2send = 0;
    waitforeverybody();

    StatScreen(&Player[myconnectindex]);

    TerminateLevel();

    if (SW_SHAREWARE)
    {
        if (FinishAnim)
        {
            PlaySong(nullptr, ThemeSongs[0], ThemeTrack[0]);
            MenuLevel();
			STAT_Update(true);
    }
    }
    else
    {
        if (FinishAnim == ANIM_ZILLA || FinishAnim == ANIM_SERP)
        {
            PlaySong(nullptr, ThemeSongs[0], ThemeTrack[0]);
            MenuLevel();
			STAT_Update(true);
    }
    }
}


// CTW REMOVED END

void DrawMenuLevelScreen(void)
{
    const int TITLE_PIC = 2324;
    twod->ClearScreen();
    DrawTexture(twod, tileGetTexture(TITLE_PIC), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal,
        DTA_Color, shadeToLight(20), TAG_DONE);
}

short PlayerQuitMenuLevel = -1;

void MenuLevel(void)
{
    short w,h;

    M_StartControlPanel(false);
    M_SetMenu(NAME_Mainmenu);

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
        MNU_DrawString(160, 170, ds, 1, 16, 0);

        sprintf(ds,"They are afraid!");
        MNU_DrawString(160, 180, ds, 1, 16, 0);
    }

    videoNextPage();
    //FadeIn(0, 3);

    waitforeverybody();

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
        D_ProcessEvents();
        C_RunDelayedCommands();

        // limits checks to max of 40 times a second
        if (totalclock >= ototalclock + synctics)
        {
            ototalclock += synctics;
        }

        if (ExitLevel)
        {
            ExitLevel = FALSE;
            break;
        }

        // must lock the clock for drawing so animations will happen
        totalclocklock = totalclock;

        //drawscreen as fast as you can
        DrawMenuLevelScreen();
        DoUpdateSounds();

        videoNextPage();
    }

    inputState.ClearAllInput();
	M_ClearMenus();
    InMenuLevel = FALSE;
    twod->ClearScreen();
    videoNextPage();
}


extern SWBOOL FinishedLevel;


void EndGameSequence(void)
{
	StopSound();

    //playanm(FinishAnim);

    //BonusScreen();

    ExitLevel = FALSE;

    //if (FinishAnim == ANIM_ZILLA)
      //      CreditsLevel();

    ExitLevel = FALSE;

    if (currentLevel->levelNumber != 4 && currentLevel->levelNumber != 20)
    {
        NextLevel = FindMapByLevelNum(currentLevel->levelNumber + 1);
    }
}

void StatScreen(PLAYERp mpp)
{
    extern SWBOOL FinishedLevel;
    short w,h;

    short rows,cols,i,j;
    PLAYERp pp = NULL;
    int x,y;
    short pal;

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
        //BonusScreen();
        return;
    }
    //MPBonusScreen();
}


// Transitioning helper.
void Logo(const CompletionFunc& completion);

int SyncScreenJob()
{
    while (gamestate == GS_INTERMISSION || gamestate == GS_INTRO)
    {
        DoUpdateSounds();
        handleevents();
        updatePauseStatus();
        D_ProcessEvents();
        ControlInfo info;
        CONTROL_GetInput(&info);
        C_RunDelayedCommands();

        RunScreenJobFrame();	// This handles continuation through its completion callback.
        videoNextPage();
    }
    return 0;
}



void GameIntro(void)
{
    Logo([](bool) { gamestate = GS_LEVEL; });
    SyncScreenJob();
    MenuLevel();
}

void getinput(SW_PACKET *, SWBOOL);


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
        C_RunDelayedCommands();
		D_ProcessEvents();
        updatePauseStatus();

        if (LoadGameOutsideMoveLoop)
        {
            return; // Stop the game loop if a savegame was loaded from the menu.
        }


        if (paused)
        {
            ototalclock = (int)totalclock - (120 / synctics);
            buttonMap.ResetButtonStates();
        }
        else
        {
            while (ready2send && (totalclock >= ototalclock + synctics))
            {
                UpdateInputs();
                MoveTicker();
            }

            // Get input again to update q16ang/q16horiz.
            if (!PedanticMode)
                getinput(&loc, TRUE);
        }

        drawscreen(Player + screenpeek);
        videoNextPage();

        if (ExitLevel)
        {
            ExitLevel = FALSE;
            break;
        }
    }

    ready2send = 0;
}

int32_t GameInterface::app_main()
{
    InitGame();

    GameIntro();

    while (true)
    {
        handleevents();
        C_RunDelayedCommands();

        NewLevel();
    }

    //SybexScreen();
    throw CExitEvent(0);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

ReservedSpace GameInterface::GetReservedScreenSpace(int viewsize)
{
    return { 0, 48 };
}

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

END_SW_NS
