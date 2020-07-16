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

BEGIN_DUKE_NS

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
