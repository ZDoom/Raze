//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

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

#include "demo.h"
#include "duke3d.h"

#include "savegame.h"
#include "screens.h"

BEGIN_DUKE_NS

char g_firstDemoFile[BMAX_PATH];

FileWriter *g_demo_filePtr{};  // write
FileReader g_demo_recFilePtr;  // read

int32_t g_demo_cnt;
int32_t g_demo_goalCnt=0;
int32_t g_demo_totalCnt;
int32_t g_demo_paused=0;
int32_t g_demo_rewind=0;
int32_t g_demo_showStats=1;

int32_t demoplay_diffs=1;
int32_t demorec_seeds_cvar=1;
int32_t demoplay_showsync=1;

static int32_t demorec_seeds=1, demo_hasseeds;

////////////////////

int32_t G_PlaybackDemo(void)
{
    int32_t bigi, initsyncofs = 0, lastsyncofs = 0, lastsynctic = 0, lastsyncclock = 0;
    int32_t foundemo = 0, outofsync=0;
    static int32_t in_menu = 0;

    totalclock = 0;
    ototalclock = 0;
    lockclock = 0;

    if (ready2send)
        return 0;

RECHECK:
    in_menu = g_player[myconnectindex].ps->gm&MODE_MENU;

    pub = NUMPAGES;
    pus = NUMPAGES;

    renderFlushPerms();

    if (foundemo == 0)
    {
        ud.recstat = 0;

        //fadepal(0,0,0, 0,252,28);
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308
        drawbackground();
        //M_DisplayMenus();
        videoNextPage();
        //fadepal(0,0,0, 252,0,-28);
        ud.reccnt = 0;
    }

    if (foundemo == 0 || in_menu || inputState.CheckAllInput() || numplayers > 1)
    {
        FX_StopAllSounds();
		M_StartControlPanel(false);
	}

    ready2send = 0;
    bigi = 0;

    inputState.ClearAllInput();

    while (g_demo_cnt < g_demo_totalCnt || foundemo==0)
    {
        // Main loop here. It also runs when there's no demo to show,
        // so maybe a better name for this function would be
        // G_MainLoopWhenNotInGame()?

        if (G_FPSLimit())
        {
            if (foundemo == 0)
            {
                drawbackground();
            }

            if ((g_player[myconnectindex].ps->gm&MODE_MENU) && (g_player[myconnectindex].ps->gm&MODE_EOL))
            {
                videoNextPage();
                goto RECHECK;
            }

            else if (g_player[myconnectindex].ps->gm&MODE_TYPE)
            {
                Net_SendMessage();

                if ((g_player[myconnectindex].ps->gm&MODE_TYPE) != MODE_TYPE)
                {
                    g_player[myconnectindex].ps->gm = 0;
					M_StartControlPanel(false);
				}
            }
            else
            {
                //if (ud.recstat != 2)
                    //M_DisplayMenus();

                if ((g_netServer || ud.multimode > 1))//  && !Menu_IsTextInput(m_currentMenu))
                {
                    ControlInfo noshareinfo;
                    CONTROL_GetInput(&noshareinfo);
                }
            }

            if (ud.last_camsprite != ud.camerasprite)
                ud.last_camsprite = ud.camerasprite;

            if (VOLUMEONE)
            {
                if ((g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
                    rotatesprite_fs((320-50)<<16, 9<<16, 65536L, 0, TILE_BETAVERSION, 0, 0, 2+8+16+128);
            }

            videoNextPage();
        }

        G_HandleAsync();

        if (g_player[myconnectindex].ps->gm == MODE_GAME)
        {
            // user wants to play a game, quit showing demo!
            return 0;
        }
    }

    ud.multimode = numplayers;  // fixes 2 infinite loops after watching demo

    // if we're in the menu, try next demo immediately
    if (g_player[myconnectindex].ps->gm&MODE_MENU)
        goto RECHECK;


    // finished playing a demo and not in menu:
    // return so that e.g. the title can be shown
    return 1;
}

END_DUKE_NS
