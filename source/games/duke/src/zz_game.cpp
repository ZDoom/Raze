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
#include "net.h"
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

int16_t max_ammo_amount[MAX_WEAPONS];
int32_t spriteqamount = 64;

uint8_t shadedsector[MAXSECTORS];

int32_t cameradist = 0, cameraclock = 0;

char boardfilename[BMAX_PATH] = {0};

int32_t g_Shareware = 0;

int32_t tempwallptr;
int32_t      actor_tog;

static int32_t nonsharedtimer;
weaponhit hittype[MAXSPRITES];
ActorInfo actorinfo[MAXTILES];

static void gameTimerHandler(void)
{
    S_Update();

    // we need CONTROL_GetInput in order to pick up joystick button presses
    if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
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



void G_HandleLocalKeys(void)
{
//    CONTROL_ProcessBinds();

    if (ud.recstat == 2)
    {
        ControlInfo noshareinfo;
        CONTROL_GetInput(&noshareinfo);
    }

    if (!ALT_IS_PRESSED && ud.overhead_on == 0 && (g_player[myconnectindex].ps->gm & MODE_TYPE) == 0)
    {
        if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
        {
            buttonMap.ClearButton(gamefunc_Enlarge_Screen);

            if (!SHIFTS_IS_PRESSED)
            {
				if (G_ChangeHudLayout(1))
				{
					S_PlaySound(isRR() ? 341 : THUD, CHAN_AUTO, CHANF_UI);
				}
            }
            else
            {
                hud_scale = hud_scale + 4;
            }
        }

        if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
        {
            buttonMap.ClearButton(gamefunc_Shrink_Screen);

            if (!SHIFTS_IS_PRESSED)
            {
				if (G_ChangeHudLayout(-1))
				{
					S_PlaySound(isRR() ? 341 : THUD, CHAN_AUTO, CHANF_UI);
				}
            }
            else
            {
                hud_scale = hud_scale - 4;
            }
        }
    }

    if (g_player[myconnectindex].ps->cheat_phase == 1 || (g_player[myconnectindex].ps->gm&(MODE_MENU|MODE_TYPE)) || System_WantGuiCapture())
        return;

    if (buttonMap.ButtonDown(gamefunc_See_Coop_View) && (ud.coop || ud.recstat == 2))
    {
        buttonMap.ClearButton(gamefunc_See_Coop_View);
        screenpeek = connectpoint2[screenpeek];
        if (screenpeek == -1) screenpeek = 0;
        restorepalette = -1;
    }

    if ((g_netServer || ud.multimode > 1) && buttonMap.ButtonDown(gamefunc_Show_Opponents_Weapon))
    {
        buttonMap.ClearButton(gamefunc_Show_Opponents_Weapon);
        ud.config.ShowOpponentWeapons = ud.showweapons = 1-ud.showweapons;
        FTA(QUOTE_WEAPON_MODE_OFF-ud.showweapons,g_player[screenpeek].ps);
    }

    if (buttonMap.ButtonDown(gamefunc_Toggle_Crosshair))
    {
        buttonMap.ClearButton(gamefunc_Toggle_Crosshair);
        cl_crosshair = !cl_crosshair;
        FTA(QUOTE_CROSSHAIR_OFF-cl_crosshair,g_player[screenpeek].ps);
    }

    if (ud.overhead_on && buttonMap.ButtonDown(gamefunc_Map_Follow_Mode))
    {
        buttonMap.ClearButton(gamefunc_Map_Follow_Mode);
        ud.scrollmode = 1-ud.scrollmode;
        if (ud.scrollmode)
        {
            ud.folx = g_player[screenpeek].ps->opos.x;
            ud.foly = g_player[screenpeek].ps->opos.y;
            ud.fola = fix16_to_int(g_player[screenpeek].ps->oq16ang);
        }
        FTA(QUOTE_MAP_FOLLOW_OFF+ud.scrollmode,g_player[myconnectindex].ps);
    }


    if (SHIFTS_IS_PRESSED || ALT_IS_PRESSED || WIN_IS_PRESSED)
    {
        int ridiculeNum = 0;

        // NOTE: sc_F1 .. sc_F10 are contiguous. sc_F11 is not sc_F10+1.
        for (bssize_t j=sc_F1; j<=sc_F10; j++)
            if (inputState.UnboundKeyPressed(j))
            {
                inputState.ClearKeyStatus(j);
                ridiculeNum = j - sc_F1 + 1;
                break;
            }

        if (ridiculeNum)
        {
            if (SHIFTS_IS_PRESSED)
            {
                Printf(PRINT_NOTIFY, *CombatMacros[ridiculeNum-1]);
				//Net_SendTaunt(ridiculeNum);
                return;
            }

            // Not SHIFT -- that is, either some ALT or WIN.
            if (startrts(ridiculeNum, 1))
            {
				//Net_SendRTS(ridiculeNum);
                return;
            }
        }
    }
    else
    {
        if (buttonMap.ButtonDown(gamefunc_Third_Person_View))
        {
            buttonMap.ClearButton(gamefunc_Third_Person_View);

            if (!isRRRA() || (!g_player[myconnectindex].ps->OnMotorcycle && !g_player[myconnectindex].ps->OnBoat))
            {
                g_player[myconnectindex].ps->over_shoulder_on = !g_player[myconnectindex].ps->over_shoulder_on;

                cameradist  = 0;
                cameraclock = (int32_t) totalclock;

                FTA(QUOTE_VIEW_MODE_OFF + g_player[myconnectindex].ps->over_shoulder_on, g_player[myconnectindex].ps);
            }
        }

        if (ud.overhead_on != 0)
        {
            int const timerOffset = ((int) totalclock - nonsharedtimer);
            nonsharedtimer += timerOffset;

            if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
                g_player[myconnectindex].ps->zoom += mulscale6(timerOffset, max<int>(g_player[myconnectindex].ps->zoom, 256));

            if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
                g_player[myconnectindex].ps->zoom -= mulscale6(timerOffset, max<int>(g_player[myconnectindex].ps->zoom, 256));

            g_player[myconnectindex].ps->zoom = clamp(g_player[myconnectindex].ps->zoom, 48, 2048);
        }
    }

#if 0 // fixme: We should not query Esc here, this needs to be done differently
    if (I_EscapeTrigger() && ud.overhead_on && g_player[myconnectindex].ps->newowner == -1)
    {
        I_EscapeTriggerClear();
        ud.last_overhead = ud.overhead_on;
        ud.overhead_on   = 0;
        ud.scrollmode    = 0;
    }
#endif

    if (buttonMap.ButtonDown(gamefunc_Map))
    {
        buttonMap.ClearButton(gamefunc_Map);
        if (ud.last_overhead != ud.overhead_on && ud.last_overhead)
        {
            ud.overhead_on = ud.last_overhead;
            ud.last_overhead = 0;
        }
        else
        {
            ud.overhead_on++;
            if (ud.overhead_on == 3) ud.overhead_on = 0;
            ud.last_overhead = ud.overhead_on;
        }

        restorepalette = 1;
    }
}


static int parsedefinitions_game(scriptfile *, int);


static void G_Cleanup(void)
{
    int32_t i;

    for (i=MAXPLAYERS-1; i>=0; i--)
    {
        Xfree(g_player[i].ps);
        Xfree(g_player[i].input);
    }

    if (label != (char *)&sprite[0]) Xfree(label);
    if (labelcode != (int32_t *)&sector[0]) Xfree(labelcode);
}

/*
===================
=
= G_Startup
=
===================
*/

static void G_CompileScripts(void)
{
    label     = (char *)&sprite[0];     // V8: 16384*44/64 = 11264  V7: 4096*44/64 = 2816
    labelcode = (int32_t *)&sector[0]; // V8: 4096*40/4 = 40960    V7: 1024*40/4 = 10240

    loadcons(G_ConFile());
	fi.initactorflags();

    if ((uint32_t)labelcnt > MAXSPRITES*sizeof(spritetype)/64)   // see the arithmetic above for why
        I_FatalError("Error: too many labels defined!");

    {
        char *newlabel;
        int32_t *newlabelcode;
        int32_t *newlabeltype;

        newlabel     = (char *)Xmalloc(labelcnt << 6);
        newlabelcode = (int32_t *)Xmalloc(labelcnt * sizeof(int32_t));
        newlabeltype = (int32_t *)Xmalloc(labelcnt * sizeof(int32_t));

        Bmemcpy(newlabel, label, labelcnt*64);
        Bmemcpy(newlabelcode, labelcode, labelcnt*sizeof(int32_t));

        label = newlabel;
        labelcode = newlabelcode;
    }

    Bmemset(sprite, 0, MAXSPRITES*sizeof(spritetype));
    Bmemset(sector, 0, MAXSECTORS*sizeof(sectortype));
    Bmemset(wall, 0, MAXWALLS*sizeof(walltype));

    if (IsGameEvent(EVENT_INIT))
    {
        SetGameVarID(g_iReturnVarID, -1, -1, -1);
        OnEvent(EVENT_INIT);
    }
}

inline int G_CheckPlayerColor(int color)
{
    static int32_t player_pals[] = { 0, 9, 10, 11, 12, 13, 14, 15, 16, 21, 23, };
    if (color >= 0 && color < 10) return player_pals[color];
    return 0;
}


static void G_Startup(void)
{
    int32_t i;

    timerInit(TICRATE);
    timerSetCallback(gameTimerHandler);

    G_CompileScripts();

    enginecompatibility_mode = ENGINECOMPATIBILITY_19961112;

    if (engineInit())
        G_FatalEngineError();

    // These depend on having the dynamic tile and/or sound mappings set up:
    G_InitMultiPsky(TILE_CLOUDYOCEAN, TILE_MOONSKY1, TILE_BIGORBIT1, TILE_LA);
    Net_SendClientInfo();

	if (userConfig.CommandMap.IsNotEmpty())
	{
		FString startupMap;
        if (VOLUMEONE)
        {
            Printf("The -map option is available in the registered version only!\n");
        }
        else
        {
			startupMap = userConfig.CommandMap;
			if (startupMap.IndexOfAny("/\\") < 0) startupMap.Insert(0, "/");
			DefaultExtension(startupMap, ".map");
			startupMap.Substitute("\\", "/");
			NormalizeFileName(startupMap);

			if (fileSystem.FileExists(startupMap))
			{
                Printf("Using level: \"%s\".\n",startupMap.GetChars());
            }
            else
            {
                Printf("Level \"%s\" not found.\n",startupMap.GetChars());
                boardfilename[0] = 0;
            }
        }
		strncpy(boardfilename, startupMap, BMAX_PATH);
    }

    for (i=0; i<MAXPLAYERS; i++)
        g_player[i].playerreadyflag = 0;

    Net_GetPackets();

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

static void P_SetupMiscInputSettings(void)
{
    DukePlayer_t *ps = g_player[myconnectindex].ps;

    ps->aim_mode = in_mousemode;
    ps->auto_aim = cl_autoaim;
    ps->weaponswitch = cl_weaponswitch;
}

void G_UpdatePlayerFromMenu(void)
{
    if (ud.recstat != 0)
        return;

    if (numplayers > 1)
    {
        Net_SendClientInfo();
        if (sprite[g_player[myconnectindex].ps->i].picnum == TILE_APLAYER && sprite[g_player[myconnectindex].ps->i].pal != 1)
            sprite[g_player[myconnectindex].ps->i].pal = g_player[myconnectindex].pcolor;
    }
    else
    {
        /*int32_t j = g_player[myconnectindex].ps->team;*/

        P_SetupMiscInputSettings();
        g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = G_CheckPlayerColor(playercolor);

        g_player[myconnectindex].pteam = playerteam;

        if (sprite[g_player[myconnectindex].ps->i].picnum == TILE_APLAYER && sprite[g_player[myconnectindex].ps->i].pal != 1)
            sprite[g_player[myconnectindex].ps->i].pal = g_player[myconnectindex].pcolor;
    }
}

void G_BackToMenu(void)
{
    boardfilename[0] = 0;
    ud.warp_on = 0;
    g_player[myconnectindex].ps->gm = 0;
	M_StartControlPanel(false);
	M_SetMenu(NAME_Mainmenu);
	inputState.keyFlushChars();
}

static int G_EndOfLevel(void)
{
	STAT_Update(ud.eog || (currentLevel->flags & MI_FORCEEOG));
	P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);
    setpal(g_player[myconnectindex].ps);

    if (g_player[myconnectindex].ps->gm&MODE_EOL)
    {
        ready2send = 0;

        if (ud.display_bonus_screen == 1)
        {
            G_BonusScreen(0);
        }

        // Clear potentially loaded per-map ART only after the bonus screens.
        artClearMapArt();

        if (ud.eog || (currentLevel->flags & MI_FORCEEOG))
        {
            ud.eog = 0;
            if ((!g_netServer && ud.multimode < 2))
            {
                if (!VOLUMEALL)
                    doorders([](bool) {});
                g_player[myconnectindex].ps->gm = 0;
				return 2;
            }
            else
            {
                m_level_number = 0;
                ud.level_number = 0;
            }
        }
    }

    ud.display_bonus_screen = 1;
    ready2send = 0;

    if (numplayers > 1)
        g_player[myconnectindex].ps->gm = MODE_GAME;

    if (G_EnterLevel(g_player[myconnectindex].ps->gm))
    {
        return 2;
    }

    Net_WaitForEverybody();
    return 1;
}

void G_MaybeAllocPlayer(int32_t pnum)
{
    if (g_player[pnum].ps == NULL)
        g_player[pnum].ps = (DukePlayer_t *)Xcalloc(1, sizeof(DukePlayer_t));
    if (g_player[pnum].input == NULL)
        g_player[pnum].input = (input_t *)Xcalloc(1, sizeof(input_t));
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

int32_t SetDefaults(void)
{
    g_player[0].ps->aim_mode = 1;
    ud.config.ShowOpponentWeapons = 0;
    ud.automsg = 0;
    ud.camerasprite = -1;

    ud.camera_time = 0;//4;

    ud.screen_tilting = 1;
    playerteam = 0;
    ud.angleinterpolation = 0;

    ud.display_bonus_screen = 1;
    ud.show_level_text = 1;
    ud.screenfade = 1;
    ud.menubackground = 1;
    ud.slidebar_paldisabled = 1;
    ud.shadow_pal = 4;
    return 0;
}

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
    playing_rr = 1;
    g_skillCnt = 4;
    ud.multimode = 1;
	ud.m_monsters_off = userConfig.nomonsters;

    g_movesPerPacket = 1;
    //bufferjitter = 1;
    initsynccrc();

    // This needs to happen before G_CheckCommandLine() because G_GameExit()
    // accesses g_player[0].
    G_MaybeAllocPlayer(0);

    checkcommandline();

    SetDefaults();


    hud_size.Callback();
    hud_scale.Callback();
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

    Net_GetPackets();

    for (bssize_t i=0; i<MAXPLAYERS; i++)
        G_MaybeAllocPlayer(i);

    G_Startup(); // a bunch of stuff including compiling cons

    g_player[0].playerquitflag = 1;

    g_player[myconnectindex].ps->palette = BASEPAL;

    for (int i=1, j=numplayers; j<ud.multimode; j++)
    {
        Bsprintf(g_player[j].user_name,"%s %d", GStrings("PLAYER"),j+1);
        g_player[j].ps->team = g_player[j].pteam = i;
        g_player[j].ps->weaponswitch = 3;
        g_player[j].ps->auto_aim = 0;
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

    tileDelete(TILE_MIRROR);
    skiptile = TILE_W_FORCEFIELD + 1;

    if (isRR())
        tileDelete(0);

    tileDelete(13);

    if (numplayers == 1 && boardfilename[0] != 0)
    {
        m_level_number  = 7;
        ud.m_volume_number = 0;
        ud.warp_on         = 1;
    }

    // getnames();

    if (g_netServer || ud.multimode > 1)
    {
        if (ud.warp_on == 0)
        {
            ud.m_monsters_off = 1;
            ud.m_player_skill = 0;
        }
    }

    playerswhenstarted = ud.multimode;  // XXX: redundant?

    ud.last_level = -1;
    registerosdcommands();

    videoInit();
    V_LoadTranslations();
    videoSetPalette(BASEPAL, 0);

    FX_StopAllSounds();
    S_ClearSoundLocks();
	app_loop();
	return 0;
}
	
void app_loop()
{
	auto &myplayer = g_player[myconnectindex].ps;

MAIN_LOOP_RESTART:
    totalclock = 0;
    ototalclock = 0;
    lockclock = 0;

    g_player[myconnectindex].ps->ftq = 0;

    if (ud.warp_on == 1)
    {
        G_NewGame_EnterLevel();
        // may change ud.warp_on in an error condition
    }

    if (ud.warp_on == 0)
    {
        if ((g_netServer || ud.multimode > 1) && boardfilename[0] != 0)
        {
            m_level_number = 7;
            ud.m_volume_number = 0;

            if (ud.m_player_skill == 4)
                ud.m_respawn_monsters = 1;
            else ud.m_respawn_monsters = 0;

            for (bssize_t TRAVERSE_CONNECT(i))
            {
                resetweapons(i);
                resetinventory(i);
            }

            G_NewGame_EnterLevel();
        }
        else
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

    ud.showweapons = ud.config.ShowOpponentWeapons;
    P_SetupMiscInputSettings();
    g_player[myconnectindex].pteam = playerteam;

    if (playercolor) g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = G_CheckPlayerColor(playercolor);
    else g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor;

    ud.warp_on = 0;
	inputState.ClearKeyStatus(sc_Pause);   // JBF: I hate the pause key

    do //main loop
    {
		handleevents();
		if (g_player[myconnectindex].ps->gm == MODE_DEMO)
		{
			M_ClearMenus();
			goto MAIN_LOOP_RESTART;
		}

        Net_GetPackets();

        G_HandleLocalKeys();
 
        C_RunDelayedCommands();

        char gameUpdate = false;
        gameupdatetime.Reset();
        gameupdatetime.Clock();
        
        while ((!(g_player[myconnectindex].ps->gm & (MODE_MENU|MODE_DEMO))) && (int)(totalclock - ototalclock) >= TICSPERFRAME)
        {
            ototalclock += TICSPERFRAME;

            if (isRRRA() && g_player[myconnectindex].ps->OnMotorcycle)
                P_GetInputMotorcycle(myconnectindex);
            else if (isRRRA() && g_player[myconnectindex].ps->OnBoat)
                P_GetInputBoat(myconnectindex);
            else
                P_GetInput(myconnectindex);

            // this is where we fill the input_t struct that is actually processed by P_ProcessInput()
            auto const pPlayer = g_player[myconnectindex].ps;
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

            if (((!System_WantGuiCapture() && (g_player[myconnectindex].ps->gm&MODE_MENU) != MODE_MENU) || ud.recstat == 2 || (g_netServer || ud.multimode > 1)) &&
                    (g_player[myconnectindex].ps->gm&MODE_GAME))
            {
                moveloop();
            }
        }

        gameUpdate = true;
        gameupdatetime.Unclock();

        if (g_player[myconnectindex].ps->gm & (MODE_EOL|MODE_RESTART))
        {
            switch (G_EndOfLevel())
            {
                case 1: continue;
                case 2: goto MAIN_LOOP_RESTART;
            }
        }

        
        if (G_FPSLimit())
        {
            if (isRRRA() && g_player[myconnectindex].ps->OnMotorcycle)
                P_GetInputMotorcycle(myconnectindex);
            else if (isRRRA() && g_player[myconnectindex].ps->OnBoat)
                P_GetInputBoat(myconnectindex);
            else
                P_GetInput(myconnectindex);

            int const smoothRatio = calc_smoothratio(totalclock, ototalclock);

            drawtime.Reset();
            drawtime.Clock();
            displayrooms(screenpeek, smoothRatio);
            G_DisplayRest(smoothRatio);
            drawtime.Unclock();
            videoNextPage();
        }

        if (g_player[myconnectindex].ps->gm&MODE_DEMO)
            goto MAIN_LOOP_RESTART;
    }
    while (1);
}

void GameInterface::FreeGameData()
{
    setmapfog(0);
    G_Cleanup();
}

::GameInterface* CreateInterface()
{
	return new GameInterface;
}

// access wrappers that alias old names to current data.
psaccess ps;


END_DUKE_NS
