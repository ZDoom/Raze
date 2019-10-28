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

#include "duke3d.h"
#include "demo.h"
#include "screens.h"
#include "renderlayer.h"
#include "cmdline.h"

BEGIN_DUKE_NS

int32_t g_commandSetup = 0;
int32_t g_noSetup = 0;
int32_t g_fakeMultiMode = 0;


static void G_AddDemo(const char* param)
{
    Bstrncpy(tempbuf, param, sizeof(tempbuf));
    char * colon = (char *) Bstrchr(tempbuf, ':');
    int32_t framespertic=-1, numrepeats=1;

    if (colon && colon != tempbuf)
    {
        // -d<filename>:<num>[,<num>]
        // profiling options
        *(colon++) = 0;
        Bsscanf(colon, "%d,%d", &framespertic, &numrepeats);
    }

    Demo_SetFirst(tempbuf);

    if (framespertic < 0)
    {
        initprintf("Play demo %s.\n", g_firstDemoFile);
    }
    else
    {
        framespertic = clamp(framespertic, 0, 8)+1;
        // TODO: repeat count and gathering statistics.
        initprintf("Profile demo %s, %d frames/gametic, repeated 1x.\n", g_firstDemoFile,
            framespertic-1);
        Demo_PlayFirst(framespertic, 1);
        g_noLogo = 1;
    }
}

void G_CheckCommandLine(int32_t argc, char const * const * argv)
{
	// stuff for later
#if 0
    int16_t i = 1, j;
    const char *c, *k;


    if (argc > 1)
    {
        initprintf("Application parameters: ");
        while (i < argc)
            initprintf("%s ", argv[i++]);
        initprintf("\n");

        i = 1;
        do
        {
            const char *const oc = argv[i];
            int32_t shortopt = 0, ignored_short_opt = 0;

            c = oc;

            if (*c == '-')
            {
                shortopt = 0;

                if (!Bstrcasecmp(c+1, "noffire"))
                {
                    ud.m_ffire = 0;
                    i++;
                    continue;
                }
#ifdef HAVE_CLIPSHAPE_FEATURE
                if (!Bstrcasecmp(c+1, "clipmap"))
                {
                    if (argc > i+1)
                    {
                        G_AddClipMap(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
#endif
                if (!Bstrcasecmp(c+1, "condebug"))
                {
                    g_scriptDebug = 1;
                    i++;
                    continue;
                }
            }

            if ((*c == '-')
#ifdef _WIN32
                || (*c == '/')
#endif
                )
            {
                shortopt = 1;

                c++;
                switch (Btolower(*c))
                {
                case 'a':
                    ud.playerai = 1;
                    initprintf("Other player AI.\n");
                    break;
                case 'c':
                    c++;
                    ud.m_coop = 0;
                    while ((*c >= '0')&&(*c <= '9'))
                    {
                        ud.m_coop *= 10;
                        ud.m_coop += *c - '0';
                        c++;
                    }
                    ud.m_coop--;
                    break;
                case 'q':
                    if (*(++c) == 0)
                    {
                        ud.multimode = 1;
                        initprintf("Fake multiplayer mode: expected number after -q, falling back to 1 player.\n");
                    }
                    else
                    {
                        int32_t numpl = Batoi(c);

                        if (numpl < 2 || numpl > MAXPLAYERS)
                        {
                            initprintf("Fake multiplayer mode: expected 2-%d players, falling back to 1.\n",
                                MAXPLAYERS);
                        }
                        else
                        {
                            ud.multimode = numpl;
                            initprintf("Fake multiplayer mode: %d players.\n", ud.multimode);

                            g_fakeMultiMode = numpl;
                        }
                    }

                    ud.m_coop = ud.coop = 0;
                    ud.m_marker = ud.marker = 1;
                    ud.m_respawn_monsters = ud.respawn_monsters = 1;
                    ud.m_respawn_items = ud.respawn_items = 1;
                    ud.m_respawn_inventory = ud.respawn_inventory = 1;
                    break;
                case 'r':
                    ud.m_recstat = 1;
                    initprintf("Demo record mode on.\n");
                    break;
                case 's':
                    c++;
                    ud.m_player_skill = ud.player_skill = (Batoi(c)%5);
                    if (ud.m_player_skill == 4)
                        ud.m_respawn_monsters = ud.respawn_monsters = 1;
                    break;
                case 't':
                    c++;
                    if (*c == '1') ud.m_respawn_monsters = 1;
                    else if (*c == '2') ud.m_respawn_items = 1;
                    else if (*c == '3') ud.m_respawn_inventory = 1;
                    else
                    {
                        ud.m_respawn_monsters = 1;
                        ud.m_respawn_items = 1;
                        ud.m_respawn_inventory = 1;
                    }
                    initprintf("Respawn on.\n");
                    break;
                case 'v':
                    c++;
                    ud.warp_on = 1;
                    ud.m_volume_number = ud.volume_number = ((unsigned) (Batoi(c)-1))%MAXVOLUMES;
                    break;
                case 'w':
                    cl_showcoords = 1;
                    break;
                case 'z':
                    c++;
                    g_scriptDebug = Batoi(c);
                    if (!g_scriptDebug)
                        g_scriptDebug = 1;
                    break;
                default:
                    ignored_short_opt = 1;
                    break;
                }
            }
            if (!shortopt || ignored_short_opt)
                initprintf("Warning: ignored application parameter \"%s\".\n", oc);

            i++;
        } while (i < argc);
    }
#endif
}

END_DUKE_NS
