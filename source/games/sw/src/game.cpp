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
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "swactor.h"
#include "interpso.h"
#include "tags.h"
#include "sector.h"
#include "sprite.h"
#include "weapon.h"
#include "player.h"
#include "lists.h"
#include "network.h"
#include "pal.h"
#include "automap.h"
#include "statusbar.h"
#include "texturemanager.h"
#include "st_start.h"
#include "i_interface.h"
#include "psky.h"
#include "startscreen.h"



#include "menus.h"

#include "gamecontrol.h"

#include "misc.h"

#include "misc.h"
#include "break.h"
#include "light.h"
#include "misc.h"
#include "jsector.h"

#include "gameconfigfile.h"
#include "printf.h"
#include "m_argv.h"
#include "debugbreak.h"
#include "razemenu.h"
#include "raze_music.h"
#include "statistics.h"
#include "gstrings.h"
#include "mapinfo.h"
#include "v_video.h"
#include "raze_sound.h"
#include "secrets.h"

#include "screenjob_.h"
#include "inputstate.h"
#include "gamestate.h"
#include "d_net.h"
#include "v_draw.h"
#include "interpolate.h"

//#include "crc32.h"

CVAR(Bool, sw_ninjahack, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG | CVAR_SERVERINFO);
CVAR(Bool, sw_darts, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG);
CVAR(Bool, sw_bunnyrockets, false, CVAR_SERVERINFO | CVAR_CHEAT);   // This is a cheat, so don't save.

BEGIN_SW_NS

IMPLEMENT_CLASS(DSWActor, false, true)
IMPLEMENT_POINTERS_START(DSWActor)
IMPLEMENT_POINTER(ownerActor)
IMPLEMENT_POINTER(user.lowActor)
IMPLEMENT_POINTER(user.lowActor)
IMPLEMENT_POINTER(user.highActor)
IMPLEMENT_POINTER(user.targetActor)
IMPLEMENT_POINTER(user.flameActor)
IMPLEMENT_POINTER(user.attachActor)
IMPLEMENT_POINTER(user.WpnGoalActor)
IMPLEMENT_POINTERS_END

void MarkSOInterp();

void markgcroots()
{
    MarkSOInterp();
    GC::MarkArray(StarQueue, MAX_STAR_QUEUE);
    GC::MarkArray(HoleQueue, MAX_HOLE_QUEUE);
    GC::MarkArray(WallBloodQueue, MAX_WALLBLOOD_QUEUE);
    GC::MarkArray(FloorBloodQueue, MAX_FLOORBLOOD_QUEUE);
    GC::MarkArray(GenericQueue, MAX_GENERIC_QUEUE);
    GC::MarkArray(LoWangsQueue, MAX_LOWANGS_QUEUE);
    GC::MarkArray(BossSpriteNum, 3);
    for (auto& pl : Player)
    {
        GC::Mark(pl.actor);
        GC::Mark(pl.lowActor);
        GC::Mark(pl.highActor);
        GC::Mark(pl.remoteActor);
        GC::Mark(pl.PlayerUnderActor);
        GC::Mark(pl.KillerActor);
        GC::Mark(pl.HitBy);
        GC::Mark(pl.last_camera_act);
    }
    for (auto& so : SectorObject)
    {
       GC::Mark(so.controller);
       GC::Mark(so.sp_child);
       GC::MarkArray(so.so_actors, MAX_SO_SPRITE);
       GC::Mark(so.match_event_actor);
    }
    for (int i = 0; i < AnimCnt; i++)
    {
        GC::Mark(Anim[i].animactor);
    }
    for (auto& mir : mirror)
    {
        GC::Mark(mir.cameraActor);
        GC::Mark(mir.camspriteActor);
    }
}


void pClearSpriteList(PLAYER* pp);

extern int sw_snd_scratch;

int GameVersion = 20;

bool NoMeters = false;
int FinishAnim = 0;
bool ReloadPrompt = false;
bool NewGame = false;
//Miscellaneous variables
bool FinishedLevel = false;
short screenpeek = 0;

int GodMode = false;
short Skill = 2;
int TotalKillable;

const GAME_SET gs_defaults =
{
// Network game settings
    0, // GameType
    0, // Monsters
    false, // HurtTeammate
    true, // SpawnMarkers Markers
    false, // TeamPlay
    0, // Kill Limit
    0, // Time Limit
    0, // Color
    true, // nuke
};
GAME_SET gs;

bool PlayerTrackingMode = false;
bool SlowMode = false;

bool DebugOperate = false;
void LoadingLevelScreen(void);

uint8_t FakeMultiNumPlayers;

int totalsynctics;

int OrigCommPlayers=0;
extern uint8_t CommPlayers;
extern bool CommEnabled;

bool CameraTestMode = false;

char ds[645];                           // debug string

extern short NormalVisibility;
bool CommandSetup = false;

char buffer[80], ch;

uint8_t DebugPrintColor = 255;

FString ThemeSongs[6];
int ThemeTrack[6];

/// L O C A L   P R O T O T Y P E S /////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

#define x(a, b) registerName(#a, b);
static void SetTileNames()
{
    auto registerName = [](const char* name, int index)
    {
        TexMan.AddAlias(name, tileGetTexture(index));
        TileFiles.addName(name, index);
    };
#include "namelist.h"
}
#undef x

void GameInterface::LoadGameTextures()
{
    LoadKVXFromScript("swvoxfil.txt");    // Load voxels from script file
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GameInterface::app_init()
{
    GC::AddMarkerFunc(markgcroots);

    GameTicRate = TICS_PER_SEC / synctics;
    InitCheats();
    automapping = 1;

    gs = gs_defaults;

    for (int i = 0; i < MAX_SW_PLAYERS; i++)
        INITLIST(&Player[i].PanelSpriteList);

    DebugOperate = true;
    enginecompatibility_mode = ENGINECOMPATIBILITY_19961112;

    if (SW_SHAREWARE)
        Printf("SHADOW WARRIOR(tm) Version 1.2 (Shareware Version)\n");
    else
        Printf("SHADOW WARRIOR(tm) Version 1.2\n");

    if (sw_snd_scratch == 0)    // This is always 0 at this point - this check is only here to prevent whole program optimization from eliminating the variable.
        Printf("Copyright (c) 1997 3D Realms Entertainment\n");

    registerosdcommands();

	numplayers = 1; myconnectindex = 0;
	connecthead = 0; connectpoint2[0] = -1;

    if (SW_SHAREWARE && numplayers > 4)
    {
        I_FatalError("To play a Network game with more than 4 players you must purchase "
            "the full version.  Read the Ordering Info screens for details.");
    }

    //Connect();
    SortBreakInfo();
    defineSky(DEFAULTPSKY, 1, nullptr);

    memset(Track, 0, sizeof(Track));
    memset(Player, 0, sizeof(Player));
    for (int i = 0; i < MAX_SW_PLAYERS; i++)
        INITLIST(&Player[i].PanelSpriteList);

    LoadCustomInfoFromScript("engine/swcustom.txt");	// load the internal definitions. These also apply to the shareware version.
    if (!SW_SHAREWARE)
        LoadCustomInfoFromScript("swcustom.txt");   // Load user customisation information

    SetTileNames();
    userConfig.AddDefs.reset();
    InitFX();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GameInterface::DrawBackground(void)
{
    const int TITLE_PIC = 2324;
    twod->ClearScreen();
    DrawTexture(twod, tileGetTexture(TITLE_PIC), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal,
        DTA_Color, shadeToLight(20), TAG_DONE);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitLevelGlobals(void)
{
    ChopTics = 0;
    automapMode = am_off;
    PlayerGravity = 24;
    wait_active_check_offset = 0;
    PlaxCeilGlobZadjust = PlaxFloorGlobZadjust = Z(500);
    FinishedLevel = false;
    AnimCnt = 0;
    left_foot = false;
    screenpeek = myconnectindex;
    ClearInterpolations();

    gNet.TimeLimitClock = gNet.TimeLimit;


    for (auto& b : bosswasseen) b = false;
    memset(BossSpriteNum,0,sizeof(BossSpriteNum));
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
    FinishAnim = false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void spawnactors(SpawnSpriteDef& sprites)
{
    InitSpriteLists();
    for (unsigned i = 0; i < sprites.sprites.Size(); i++)
    {
        if (sprites.sprites[i].statnum == MAXSTATUS)
        {
            continue;
        }
        auto sprt = &sprites.sprites[i];
        auto actor = insertActor(sprt->sectp, sprt->statnum);
        actor->spr = sprites.sprites[i];
        actor->time = i;
        if (sprites.sprext.Size()) actor->sprext = sprites.sprext[i];
        else actor->sprext = {};
        actor->spsmooth = {};
    }
    leveltimer = sprites.sprites.Size();
}


void InitLevel(MapRecord *maprec)
{
    Terminate3DSounds();

    // A few IMPORTANT GLOBAL RESETS
    InitLevelGlobals();

    Mus_Stop();

    if (!maprec)
    {
        I_Error("Attempt to start game without level");
        return;
    }
    InitLevelGlobals2();

    if (NewGame)
    {
        for (int i = 0; i < MAX_SW_PLAYERS; i++)
        {
            // don't jack with the playerreadyflag
            int ready_bak = Player[i].playerreadyflag;
            int ver_bak = Player[i].PlayerVersion;
            memset(&Player[i], 0, sizeof(Player[i]));
            Player[i].playerreadyflag = ready_bak;
            Player[i].PlayerVersion = ver_bak;
            INITLIST(&Player[i].PanelSpriteList);
        }

        memset(puser, 0, sizeof(puser));
    }

    int16_t ang;
    currentLevel = maprec;
    int cursect;
    SpawnSpriteDef sprites;
    DVector3 ppos;
    loadMap(maprec->fileName, SW_SHAREWARE ? 1 : 0, &ppos, &ang, &cursect, sprites);
    Player[0].pos = ppos;
    spawnactors(sprites);
    Player[0].cursector = &sector[cursect];

    SECRET_SetMapName(currentLevel->DisplayName(), currentLevel->name);
    STAT_NewLevel(currentLevel->fileName);
    TITLE_InformName(currentLevel->name);
    Player[0].angle.ang = DAngle::fromBuild(ang);

    auto vissect = &sector[0]; // hack alert!
    if (vissect->extra != -1)
    {
        NormalVisibility = g_visibility = vissect->extra;
        vissect->extra = 0;
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

    PlaceSectorObjectsOnTracks();
    PlaceActorsOnTracks();
    PostSetupSectorObject();
    SetupMirrorTiles();
    CollectPortals();

    // reset NewGame
    NewGame = false;
	setLevelStarted(maprec);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitRunLevel(void)
{
    Mus_Stop();

    DoTheCache();

    // send packets with player info
    InitNetPlayerOptions();

    // Initialize Game part of network code
    InitNetVars();

    if (currentLevel)
    {
        PlaySong(currentLevel->music, currentLevel->cdSongId);
    }

    InitPrediction(&Player[myconnectindex]);

    InitTimingVars();

    if (snd_ambience)
        StartAmbientSound();
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void TerminateLevel(void)
{
    videoFadePalette(0, 0, 0, 0);
    if (!currentLevel) return;

    int i, stat, pnum, ndx;

    // Free any track points
    for (ndx = 0; ndx < MAX_TRACKS; ndx++)
    {
        Track[ndx].FreeTrackPoints();
    }

    // Clear the tracks and other arrays holding pointers into the level data.
    memset(Track, 0, sizeof(Track));
    memset(SineWaveFloor, 0, sizeof(SineWaveFloor));
    memset(SineWall, 0, sizeof(SineWall));
    memset(SpringBoard, 0, sizeof(SpringBoard));

    StopFX();

    // Clear all anims and any memory associated with them
    // Clear before killing sprites - save a little time
    //AnimClear();

    for (stat = STAT_PLAYER0; stat < STAT_PLAYER0 + numplayers; stat++)
    {

        pnum = stat - STAT_PLAYER0;

        SWStatIterator it(stat);
        if (auto actor = it.Next())
        {
            if (actor->hasU()) puser[pnum].CopyFromUser(actor);
        }
    }

    // clear some pointers KillActor may operate upon.
    SWSpriteIterator it;
    while (auto actor = it.Next())
    {
        actor->user.targetActor = nullptr;
        actor->user.flameActor = nullptr;
    }
    // Kill User memory and delete sprites
    it.Reset();
    while (auto actor = it.Next())
    {
        KillActor(actor);
    }

    TRAVERSE_CONNECT(pnum)
    {
        PLAYER* pp = &Player[pnum];

        // Free panel sprites for players
        pClearSpriteList(pp);

        // clear *all* pointers in Player!
        pp->remote = {};
        pp->sop = pp->sop_remote = nullptr;
        pp->LadderSector = nullptr;
        pp->cookieTime = 0;
        pp->hi_sectp = pp->lo_sectp = nullptr;
        pp->cursector = pp->lastcursector = pp->lv_sector = nullptr;
        pp->sop_control = pp->sop_riding = nullptr;
        pp->PanelSpriteList = {};

        memset(pp->cookieQuote, 0, sizeof(pp->cookieQuote));
        pp->DoPlayerAction = nullptr;

        pp->actor = nullptr;

        pp->PlayerUnderActor = nullptr;

        memset(pp->HasKey, 0, sizeof(pp->HasKey));

        //pp->WpnFlags = 0;
        pp->CurWpn = nullptr;

        memset(pp->Wpn, 0, sizeof(pp->Wpn));
        memset(pp->InventoryTics, 0, sizeof(pp->InventoryTics));

        pp->KillerActor = nullptr;;

        INITLIST(&pp->PanelSpriteList);
    }
}


static bool DidOrderSound;
static int zero = 0;

static void PlayOrderSound()
{
    if (!DidOrderSound)
    {
        DidOrderSound = true;
        int choose_snd = StdRandomRange(1000);
        if (choose_snd > 500)
            PlaySound(DIGI_WANGORDER1, v3df_dontpan, CHAN_BODY, CHANF_UI);
        else
            PlaySound(DIGI_WANGORDER2, v3df_dontpan, CHAN_BODY, CHANF_UI);
    }
}



void GameInterface::LevelCompleted(MapRecord* map, int skill)
{
    //ResetPalette(mpp);
    COVER_SetReverb(0); // Reset reverb
    Player[myconnectindex].Reverb = 0;
    StopSound();
    STAT_Update(map == nullptr);

    SummaryInfo info{};

    info.kills = Player[screenpeek].Kills;
    info.maxkills = TotalKillable;
    info.secrets = Player[screenpeek].SecretsFound;
    info.maxsecrets = LevelSecrets;
    info.time = PlayClock / 120;

    ShowIntermission(currentLevel, map, &info, [=](bool)
        {
            if (map == nullptr)
            {
                FinishAnim = false;
                PlaySong(ThemeSongs[0], ThemeTrack[0]);
                if (isShareware())
                {
                    PlayOrderSound();
                    gameaction = ga_creditsmenu;
                }
                else gameaction = ga_mainmenu;
            }
            else gameaction = ga_nextlevel;
        });
}
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GameInterface::NextLevel(MapRecord *map, int skill)
{
	if (skill != -1) Skill = skill;
	ShadowWarrior::NewGame = false;
	InitLevel(map);
	InitRunLevel();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GameInterface::NewGame(MapRecord *map, int skill, bool)
{
	if (skill != -1) Skill = skill;
	ShadowWarrior::NewGame = true;
	InitLevel(map);
	InitRunLevel();
    gameaction = ga_level;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int GameInterface::GetCurrentSkill()
{
    return Skill;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GameInterface::Ticker(void)
{
    int i;
    TRAVERSE_CONNECT(i)
    {
        auto pp = Player + i;
        pp->lastinput = pp->input;
        pp->input = playercmds[i].ucmd;
        if (pp->lastinput.actions & SB_CENTERVIEW) pp->input.actions |= SB_CENTERVIEW;
    }

    domovethings();
    r_NoInterpolate = paused;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GameInterface::Render()
{
    if (paused)
    {
        smoothratio = MaxSmoothRatio;
    }
    else
    {
        smoothratio = !cl_interpolate || cl_capfps ? MaxSmoothRatio : I_GetTimeFrac() * MaxSmoothRatio;
    }

    drawtime.Reset();
    drawtime.Clock();
    drawscreen(Player + screenpeek, smoothratio, false);
    drawtime.Unclock();
}


void GameInterface::Startup()
{
    PlayLogos(ga_mainmenunostopsound, ga_mainmenu, false);
}


void GameInterface::ErrorCleanup()
{
    // Make sure we do not leave the game in an unstable state
    TerminateLevel();
    FinishAnim = false;
}

void GameInterface::ExitFromMenu()
{
    endoomName = !isShareware() ? "swreg.bin" : "shadsw.bin";
    ST_Endoom();
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
    value = (rand_num << 14) / ((65535U << 14) / range);

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

    rand_num = rand();

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
	PLAYER* pp = Player + myconnectindex;
	return { pp->Kills, TotalKillable, pp->SecretsFound, LevelSecrets, PlayClock / 120, 0 };
}

void GameInterface::FreeLevelData()
{
    TerminateLevel();
    ::GameInterface::FreeLevelData();
}

int GameInterface::Voxelize(int sprnum) 
{ 
    return (aVoxelArray[sprnum].Voxel);
}

END_SW_NS
