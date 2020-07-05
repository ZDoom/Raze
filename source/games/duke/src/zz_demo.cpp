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

#include "duke3d.h"

#include "savegame.h"
#include "screens.h"

BEGIN_DUKE_NS

////////////////////

int32_t G_PlaybackDemo(void)
{
    int32_t foundemo = 0, outofsync=0;
    static int32_t in_menu = 0;

    totalclock = 0;
    ototalclock = 0;
    lockclock = 0;

    if (ready2send)
        return 0;

RECHECK:
    in_menu = g_player[myconnectindex].ps->gm&MODE_MENU;

    if (foundemo == 0)
    {
        ud.recstat = 0;

        //fadepal(0,0,0, 0,252,28);
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

    inputState.ClearAllInput();

    while (true)
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

                if (ud.multimode > 1)//  && !Menu_IsTextInput(m_currentMenu))
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
