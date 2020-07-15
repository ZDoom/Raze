//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
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

#define game_c_

#include "duke3d.h"
#include "compat.h"
#include "baselayer.h"
#include "savegame.h"

#include "sbar.h"
#include "palette.h"
#include "gamecvars.h"
#include "gameconfigfile.h"
#include "printf.h"
#include "m_argv.h"
#include "filesystem.h"
#include "statistics.h"
#include "c_dispatch.h"
#include "mapinfo.h"
#include "v_video.h"
#include "glbackend/glbackend.h"
#include "st_start.h"
#include "i_interface.h"

// Uncomment to prevent anything except mirrors from drawing. It is sensible to
// also uncomment ENGINE_CLEAR_SCREEN in build/src/engine_priv.h.
//#define DEBUG_MIRRORS_ONLY

BEGIN_DUKE_NS

void SetDispatcher();
void InitCheats();
void checkcommandline();
int registerosdcommands(void);
int32_t moveloop(void);
int menuloop(void);
void advancequeue(int myconnectindex);
input_t& nextinput(int myconnectindex);
void GetInput();

int16_t max_ammo_amount[MAX_WEAPONS];
int32_t spriteqamount = 64;

uint8_t shadedsector[MAXSECTORS];

int32_t cameradist = 0, cameraclock = 0;

int32_t g_Shareware = 0;

int32_t tempwallptr;
int32_t      actor_tog;

weaponhit hittype[MAXSPRITES];
ActorInfo actorinfo[MAXTILES];
player_struct ps[MAXPLAYERS];

static void gameTimerHandler(void)
{
    S_Update();

    // we need CONTROL_GetInput in order to pick up joystick button presses
    if (!(ps[myconnectindex].gm & MODE_GAME))
    {
        ControlInfo noshareinfo;
        CONTROL_GetInput(&noshareinfo);
        C_RunDelayedCommands();
    }
}

void G_InitTimer(int32_t ticspersec)
{
    if (g_timerTicsPerSecond != ticspersec)
    {
        timerUninit();
        timerInit(ticspersec);
        g_timerTicsPerSecond = ticspersec;
    }
}





static int parsedefinitions_game(scriptfile *, int);


/*
===================
=
= G_Startup
=
===================
*/

static void G_Startup(void)
{
    timerInit(TICRATE);
    timerSetCallback(gameTimerHandler);

    loadcons();
    fi.initactorflags();

    if (IsGameEvent(EVENT_INIT))
    {
        SetGameVarID(g_iReturnVarID, -1, -1, -1);
        OnEvent(EVENT_INIT);
    }

    enginecompatibility_mode = ENGINECOMPATIBILITY_19961112;

    if (engineInit())
        G_FatalEngineError();

    // These depend on having the dynamic tile and/or sound mappings set up:
    setupbackdrop();
    //Net_SendClientInfo();

	if (userConfig.CommandMap.IsNotEmpty())
	{
        if (VOLUMEONE)
        {
            Printf("The -map option is available in the registered version only!\n");
			userConfig.CommandMap = "";
        }
    }

    if (numplayers > 1)
        Printf("Multiplayer initialized.\n");

    if (TileFiles.artLoadFiles("tiles%03i.art") < 0)
        I_FatalError("Failed loading art.");

    fi.InitFonts();

    // Make the fullscreen nuke logo background non-fullbright.  Has to be
    // after dynamic tile remapping (from C_Compile) and loading tiles.
    picanm[TILE_LOADSCREEN].sf |= PICANM_NOFULLBRIGHT_BIT;

//    Printf("Loading palette/lookups...\n");
    genspriteremaps();
    TileFiles.PostLoadSetup();

    screenpeek = myconnectindex;
}

void app_loop();

// TODO: reorder (net)weaponhit to eliminate slop and update assertion
EDUKE32_STATIC_ASSERT(sizeof(weaponhit)%4 == 0);

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
    "Holo_Duke",			// Duke3D", isRR()
    "Jetpack",
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
    "Steroids",
    "Quick_Kick",
    "Next_Weapon",
    "Previous_Weapon",
    "Dpad_Select",
    "Dpad_Aiming",
    "Last_Weapon",
    "Alt_Weapon",
    "Third_Person_View",
    "Show_DukeMatch_Scores",
    "Toggle_Crouch",	// This is the last one used by EDuke32.
};

int GameInterface::app_main()
{
    for (int i = 0; i < MAXPLAYERS; i++)
    {
        for (int j = 0; j < 10; j++)    
        {
            const char* s = "3457860291";
            ud.wchoice[i][j] = s[j] - '0';
        }
    }

    SetDispatcher();
    buttonMap.SetButtons(actions, NUM_ACTIONS);
    g_skillCnt = 4;
    ud.multimode = 1;
	ud.m_monsters_off = userConfig.nomonsters;

    g_movesPerPacket = 1;
    //bufferjitter = 1;
    //initsynccrc();

    checkcommandline();

    ps[0].aim_mode = 1;
    ud.camerasprite = -1;
    ud.camera_time = 0;//4;
    
    S_InitSound();

    
    if (isRR())
    {
        g_cdTrack = -1;
    }

    InitCheats();

    if (VOLUMEONE)
        g_Shareware = 1;
    else
    {
		if (fileSystem.FileExists("DUKESW.BIN")) // JBF 20030810
        {
            g_Shareware = 1;
        }
    }

    numplayers = 1;
    playerswhenstarted = ud.multimode;

    connectpoint2[0] = -1;

    G_Startup(); // a bunch of stuff including compiling cons

    ps[myconnectindex].palette = BASEPAL;

    for (int i=1, j=numplayers; j<ud.multimode; j++)
    {
        Bsprintf(ud.user_name[j],"%s %d", GStrings("PLAYER"),j+1);
        ps[j].weaponswitch = 3;
        ps[j].auto_aim = 0;
        i = 1-i;
    }

    const char *defsfile = G_DefFile();
    uint32_t stime = timerGetTicks();
    if (!loaddefinitionsfile(defsfile))
    {
        uint32_t etime = timerGetTicks();
        Printf("Definitions file \"%s\" loaded in %d ms.\n", defsfile, etime-stime);
    }

	userConfig.AddDefs.reset();

    enginePostInit();
    hud_size.Callback();
    hud_scale.Callback();

    tileDelete(TILE_MIRROR);
    skiptile = TILE_W_FORCEFIELD + 1;

    if (isRR())
        tileDelete(0);

    tileDelete(13);

    // getnames();

    if (ud.multimode > 1)
    {
        ud.m_monsters_off = 1;
        ud.m_player_skill = 0;
    }

    playerswhenstarted = ud.multimode;  // XXX: redundant?

    ud.last_level = -1;
    registerosdcommands();

    videoInit();
    V_LoadTranslations();
    videoSetPalette(BASEPAL);

    FX_StopAllSounds();
	app_loop();
	return 0;
}
	
void app_loop()
{

MAIN_LOOP_RESTART:
    totalclock = 0;
    ototalclock = 0;
    lockclock = 0;

    ps[myconnectindex].ftq = 0;

    //if (ud.warp_on == 0)
    {
#if 0 // fixme once the game loop has been done.
        if ((ud.multimode > 1) && startupMap.IsNotEmpty())
        {
            auto maprecord = FindMap(startupMap);
            ud.m_respawn_monsters = ud.m_player_skill == 4;

            for (int i = 0; i != -1; i = connectpoint2[i])
            {
                resetweapons(i);
                resetinventory(i);
            }

            StartGame(maprecord);
        }
        else
#endif
        {
            fi.ShowLogo([](bool) {});
        }

        M_StartControlPanel(false);
		M_SetMenu(NAME_Mainmenu);
		if (menuloop())
        {
            FX_StopAllSounds();
            goto MAIN_LOOP_RESTART;
        }
    }

    ud.showweapons = cl_showweapon;
    setlocalplayerinput(&ps[myconnectindex]);
	PlayerColorChanged();
    inputState.ClearAllInput();

    do //main loop
    {
		handleevents();
		if (ps[myconnectindex].gm == MODE_DEMO)
		{
			M_ClearMenus();
			goto MAIN_LOOP_RESTART;
		}

        //Net_GetPackets();

        nonsharedkeys();
 
        C_RunDelayedCommands();

        char gameUpdate = false;
        gameupdatetime.Reset();
        gameupdatetime.Clock();
        
        while ((!(ps[myconnectindex].gm & (MODE_MENU|MODE_DEMO))) && (int)(totalclock - ototalclock) >= TICSPERFRAME)
        {
            ototalclock += TICSPERFRAME;

            GetInput();
            // this is where we fill the input_t struct that is actually processed by P_ProcessInput()
            auto const pPlayer = &ps[myconnectindex];
            auto const q16ang  = fix16_to_int(pPlayer->q16ang);
            auto& input = nextinput(myconnectindex);

            input = localInput;
            input.fvel = mulscale9(localInput.fvel, sintable[(q16ang + 2560) & 2047]) +
                         mulscale9(localInput.svel, sintable[(q16ang + 2048) & 2047]) +
                         pPlayer->fric.x;
            input.svel = mulscale9(localInput.fvel, sintable[(q16ang + 2048) & 2047]) +
                         mulscale9(localInput.svel, sintable[(q16ang + 1536) & 2047]) +
                         pPlayer->fric.y;
            localInput = {};

            advancequeue(myconnectindex);

            if (((!System_WantGuiCapture() && (ps[myconnectindex].gm&MODE_MENU) != MODE_MENU) || ud.recstat == 2 || (ud.multimode > 1)) &&
                    (ps[myconnectindex].gm&MODE_GAME))
            {
                moveloop();
            }
        }

        gameUpdate = true;
        gameupdatetime.Unclock();

        if (ps[myconnectindex].gm & (MODE_EOL|MODE_RESTART))
        {
            switch (exitlevel())
            {
                case 1: continue;
                case 2: goto MAIN_LOOP_RESTART;
            }
        }

        
        if (G_FPSLimit())
        {
            GetInput();

            int const smoothRatio = calc_smoothratio(totalclock, ototalclock);

            drawtime.Reset();
            drawtime.Clock();
            displayrooms(screenpeek, smoothRatio);
            displayrest(smoothRatio);
            drawtime.Unclock();
            videoNextPage();
        }

        if (ps[myconnectindex].gm&MODE_DEMO)
            goto MAIN_LOOP_RESTART;
    }
    while (1);
}

::GameInterface* CreateInterface()
{
	return new GameInterface;
}


END_DUKE_NS
