//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2004, 2007 - EDuke32 developers

This file is part of EDuke32

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include "compat.h"
#include "osdcmds.h"
#include "osd.h"
#include "baselayer.h"
#include "duke3d.h"
#include "crc32.h"
#include <ctype.h>
#include <limits.h>

extern int voting, doquicksave;
struct osdcmd_cheatsinfo osdcmd_cheatsinfo_stat;

static inline int osdcmd_quit(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    sendquit();
    return OSDCMD_OK;
}

static int osdcmd_echo(const osdfuncparm_t *parm)
{
    int i;
    for (i = 0; i < parm->numparms; i++)
    {
        if (i > 0) OSD_Printf(" ");
        OSD_Printf("%s", parm->parms[i]);
    }
    OSD_Printf("\n");

    return OSDCMD_OK;
}

static int osdcmd_changelevel(const osdfuncparm_t *parm)
{
    int volume=0,level;
    char *p;

    if (!VOLUMEONE)
    {
        if (parm->numparms != 2) return OSDCMD_SHOWHELP;

        volume = strtol(parm->parms[0], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
        level = strtol(parm->parms[1], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
    }
    else
    {
        if (parm->numparms != 1) return OSDCMD_SHOWHELP;

        level = strtol(parm->parms[0], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
    }

    if (volume < 0) return OSDCMD_SHOWHELP;
    if (level < 0) return OSDCMD_SHOWHELP;

    if (!VOLUMEONE)
    {
        if (volume > num_volumes)
        {
            OSD_Printf("changelevel: invalid volume number (range 1-%d)\n",num_volumes);
            return OSDCMD_OK;
        }
    }

    if (level > MAXLEVELS || map[volume*MAXLEVELS+level].filename == NULL)
    {
        OSD_Printf("changelevel: invalid level number\n");
        return OSDCMD_SHOWHELP;
    }

    if (numplayers > 1)
    {
        if (myconnectindex == connecthead && networkmode == 0)
            mpchangemap(volume,level);
        else if (voting == -1)
        {
            ud.m_volume_number = volume;
            ud.m_level_number = level;

            if (g_player[myconnectindex].ps->i)
            {
                int i;

                for (i=0;i<MAXPLAYERS;i++)
                {
                    g_player[i].vote = 0;
                    g_player[i].gotvote = 0;
                }

                g_player[myconnectindex].vote = g_player[myconnectindex].gotvote = 1;

                voting = myconnectindex;

                tempbuf[0] = 18;
                tempbuf[1] = 1;
                tempbuf[2] = myconnectindex;
                tempbuf[3] = ud.m_volume_number;
                tempbuf[4] = ud.m_level_number;

                for (i=connecthead;i>=0;i=connectpoint2[i])
                {
                    if (i != myconnectindex) sendpacket(i,tempbuf,5);
                    if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
                }
            }
            if ((gametype_flags[ud.m_coop] & GAMETYPE_FLAG_PLAYERSFRIENDLY) && !(gametype_flags[ud.m_coop] & GAMETYPE_FLAG_TDM))
                ud.m_noexits = 0;

            g_player[myconnectindex].ps->gm |= MODE_MENU;
            cmenu(603);
        }
        return OSDCMD_OK;
    }
    if (g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        // in-game behave like a cheat
        osdcmd_cheatsinfo_stat.cheatnum = 2;
        osdcmd_cheatsinfo_stat.volume   = volume;
        osdcmd_cheatsinfo_stat.level    = level;
    }
    else
    {
        // out-of-game behave like a menu command
        osdcmd_cheatsinfo_stat.cheatnum = -1;

        ud.m_volume_number = volume;
        ud.m_level_number = level;

        ud.m_monsters_off = ud.monsters_off = 0;

        ud.m_respawn_items = 0;
        ud.m_respawn_inventory = 0;

        ud.multimode = 1;

        newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);
        if (enterlevel(MODE_GAME)) backtomenu();
    }

    return OSDCMD_OK;
}

static int osdcmd_map(const osdfuncparm_t *parm)
{
    int i;
    char filename[256];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

#if 0
    if (numplayers > 1)
    {
        OSD_Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }
#endif

    strcpy(filename,parm->parms[0]);
    if (strchr(filename,'.') == 0)
        strcat(filename,".map");

    if ((i = kopen4load(filename,0)) < 0)
    {
        OSD_Printf("map: file \"%s\" not found.\n", filename);
        return OSDCMD_OK;
    }
    kclose(i);

    boardfilename[0] = '/';
    boardfilename[1] = 0;
    strcat(boardfilename, filename);

    if (numplayers > 1)
    {
        if (myconnectindex == connecthead && networkmode == 0)
        {
            sendboardname();
            mpchangemap(0,7);
        }
        else if (voting == -1)
        {
            sendboardname();

            ud.m_volume_number = 0;
            ud.m_level_number = 7;

            if (g_player[myconnectindex].ps->i)
            {
                int i;

                for (i=0;i<MAXPLAYERS;i++)
                {
                    g_player[i].vote = 0;
                    g_player[i].gotvote = 0;
                }

                g_player[myconnectindex].vote = g_player[myconnectindex].gotvote = 1;
                voting = myconnectindex;

                tempbuf[0] = 18;
                tempbuf[1] = 1;
                tempbuf[2] = myconnectindex;
                tempbuf[3] = ud.m_volume_number;
                tempbuf[4] = ud.m_level_number;

                for (i=connecthead;i>=0;i=connectpoint2[i])
                {
                    if (i != myconnectindex) sendpacket(i,tempbuf,5);
                    if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
                }
            }
            if ((gametype_flags[ud.m_coop] & GAMETYPE_FLAG_PLAYERSFRIENDLY) && !(gametype_flags[ud.m_coop] & GAMETYPE_FLAG_TDM))
                ud.m_noexits = 0;

            g_player[myconnectindex].ps->gm |= MODE_MENU;
            cmenu(603);
        }
        return OSDCMD_OK;
    }

    osdcmd_cheatsinfo_stat.cheatnum = -1;
    ud.m_volume_number = 0;
    ud.m_level_number = 7;

    ud.m_monsters_off = ud.monsters_off = 0;

    ud.m_respawn_items = 0;
    ud.m_respawn_inventory = 0;

    ud.multimode = 1;

    newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);
    if (enterlevel(MODE_GAME)) backtomenu();

    return OSDCMD_OK;
}

static int osdcmd_god(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    if (numplayers == 1 && g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        osdcmd_cheatsinfo_stat.cheatnum = 0;
    }
    else
    {
        OSD_Printf("god: Not in a single-player game.\n");
    }

    return OSDCMD_OK;
}

static int osdcmd_noclip(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    if (numplayers == 1 && g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        osdcmd_cheatsinfo_stat.cheatnum = 20;
    }
    else
    {
        OSD_Printf("noclip: Not in a single-player game.\n");
    }

    return OSDCMD_OK;
}

static int osdcmd_fileinfo(const osdfuncparm_t *parm)
{
    unsigned int crc, length;
    int i,j;
    char buf[256];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    if ((i = kopen4load((char *)parm->parms[0],0)) < 0)
    {
        OSD_Printf("fileinfo: File \"%s\" not found.\n", parm->parms[0]);
        return OSDCMD_OK;
    }

    length = kfilelength(i);

    crc32init(&crc);
    do
    {
        j = kread(i,buf,256);
        crc32block(&crc,(unsigned char *)buf,j);
    }
    while (j == 256);
    crc32finish(&crc);

    kclose(i);

    OSD_Printf("fileinfo: %s\n"
               "  File size: %d\n"
               "  CRC-32:    %08X\n",
               parm->parms[0], length, crc);

    return OSDCMD_OK;
}

static int osdcmd_rate(const osdfuncparm_t *parm)
{
    int i;

    if (parm->numparms == 0)
    {
        OSD_Printf("\"rate\" is \"%d\"\n", packetrate);
        return OSDCMD_SHOWHELP;
    }
    else if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    i = Batol(parm->parms[0]);

    if (i >= 40 && i <= 1000)
    {
        packetrate = i;
        OSD_Printf("rate %d\n", packetrate);
    }
    else OSD_Printf("rate: value out of range\n");
    return OSDCMD_OK;
}

static int osdcmd_restartsound(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    SoundShutdown();
    MusicShutdown();

    initprintf("Initializing music...\n");
    MusicStartup();
    initprintf("Initializing sound...\n");
    SoundStartup();

    FX_StopAllSounds();
    clearsoundlocks();

    if (ud.config.MusicToggle == 1)
    {
        if (ud.recstat != 2 && g_player[myconnectindex].ps->gm&MODE_GAME)
        {
            if (map[(unsigned char)music_select].musicfn != NULL)
                playmusic(&map[(unsigned char)music_select].musicfn[0],music_select);
        }
        else playmusic(&env_music_fn[0][0],MAXVOLUMES*MAXLEVELS);
    }

    return OSDCMD_OK;
}

static int osdcmd_restartvid(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    resetvideomode();
    if (setgamemode(ud.config.ScreenMode,ud.config.ScreenWidth,ud.config.ScreenHeight,ud.config.ScreenBPP))
        gameexit("restartvid: Reset failed...\n");
    onvideomodechange(ud.config.ScreenBPP>8);
    vscrn();

    return OSDCMD_OK;
}

static int osdcmd_vidmode(const osdfuncparm_t *parm)
{
    int newbpp = ud.config.ScreenBPP, newwidth = ud.config.ScreenWidth,
                 newheight = ud.config.ScreenHeight, newfs = ud.config.ScreenMode;
    if (parm->numparms < 1 || parm->numparms > 4) return OSDCMD_SHOWHELP;

    switch (parm->numparms)
    {
    case 1: // bpp switch
        newbpp = Batol(parm->parms[0]);
        break;
    case 2: // res switch
        newwidth = Batol(parm->parms[0]);
        newheight = Batol(parm->parms[1]);
        break;
    case 3: // res & bpp switch
    case 4:
        newwidth = Batol(parm->parms[0]);
        newheight = Batol(parm->parms[1]);
        newbpp = Batol(parm->parms[2]);
        if (parm->numparms == 4)
            newfs = (Batol(parm->parms[3]) != 0);
        break;
    }

    if (setgamemode(newfs,newwidth,newheight,newbpp))
    {
        initprintf("vidmode: Mode change failed!\n");
        if (setgamemode(ud.config.ScreenMode, ud.config.ScreenWidth, ud.config.ScreenHeight, ud.config.ScreenBPP))
            gameexit("vidmode: Reset failed!\n");
    }
    ud.config.ScreenBPP = newbpp;
    ud.config.ScreenWidth = newwidth;
    ud.config.ScreenHeight = newheight;
    ud.config.ScreenMode = newfs;
    onvideomodechange(ud.config.ScreenBPP>8);
    vscrn();
    return OSDCMD_OK;
}

static int osdcmd_setstatusbarscale(const osdfuncparm_t *parm)
{
    if (parm->numparms == 0)
    {
        OSD_Printf("\"cl_statusbarscale\" is \"%d\"\n", ud.statusbarscale);
        return OSDCMD_SHOWHELP;
    }
    else if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    setstatusbarscale(Batol(parm->parms[0]));
    OSD_Printf("cl_statusbarscale %d\n", ud.statusbarscale);
    return OSDCMD_OK;
}

static int osdcmd_spawn(const osdfuncparm_t *parm)
{
    int x=0,y=0,z=0;
    unsigned short cstat=0,picnum=0;
    unsigned char pal=0;
    short ang=0;
    short set=0, idx;

    if (numplayers > 1 || !(g_player[myconnectindex].ps->gm & MODE_GAME))
    {
        OSD_Printf("spawn: Can't spawn sprites in multiplayer games or demos\n");
        return OSDCMD_OK;
    }

    switch (parm->numparms)
    {
    case 7: // x,y,z
        x = Batol(parm->parms[4]);
        y = Batol(parm->parms[5]);
        z = Batol(parm->parms[6]);
        set |= 8;
    case 4: // ang
        ang = Batol(parm->parms[3]) & 2047;
        set |= 4;
    case 3: // cstat
        cstat = (unsigned short)Batol(parm->parms[2]);
        set |= 2;
    case 2: // pal
        pal = (unsigned char)Batol(parm->parms[1]);
        set |= 1;
    case 1: // tile number
        if (isdigit(parm->parms[0][0]))
        {
            picnum = (unsigned short)Batol(parm->parms[0]);
        }
        else
        {
            int i,j;
            for (j=0; j<2; j++)
            {
                for (i=0; i<labelcnt; i++)
                {
                    if (
                        (j == 0 && !Bstrcmp(label+(i<<6),     parm->parms[0])) ||
                        (j == 1 && !Bstrcasecmp(label+(i<<6), parm->parms[0]))
                    )
                    {
                        picnum = (unsigned short)labelcode[i];
                        break;
                    }
                }
                if (i<labelcnt) break;
            }
            if (i==labelcnt)
            {
                OSD_Printf("spawn: Invalid tile label given\n");
                return OSDCMD_OK;
            }
        }

        if (picnum >= MAXTILES)
        {
            OSD_Printf("spawn: Invalid tile number\n");
            return OSDCMD_OK;
        }
        break;
    default:
        return OSDCMD_SHOWHELP;
    }

    idx = spawn(g_player[myconnectindex].ps->i, (short)picnum);
    if (set & 1) sprite[idx].pal = (char)pal;
    if (set & 2) sprite[idx].cstat = (short)cstat;
    if (set & 4) sprite[idx].ang = ang;
    if (set & 8)
    {
        if (setsprite(idx, x,y,z) < 0)
        {
            OSD_Printf("spawn: Sprite can't be spawned into null space\n");
            deletesprite(idx);
        }
    }

    return OSDCMD_OK;
}

static int osdcmd_setvar(const osdfuncparm_t *parm)
{
    int i, varval;
    char varname[256];

    if (parm->numparms != 2) return OSDCMD_SHOWHELP;

    if (numplayers > 1)
    {
        OSD_Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }

    strcpy(varname,parm->parms[1]);
    varval = Batol(varname);
    for (i=0;i<iGameVarCount;i++)
        if (aGameVars[i].szLabel != NULL)
            if (Bstrcmp(varname, aGameVars[i].szLabel) == 0)
                varval=GetGameVarID(i, g_player[myconnectindex].ps->i, myconnectindex);

    strcpy(varname,parm->parms[0]);
    for (i=0;i<iGameVarCount;i++)
        if (aGameVars[i].szLabel != NULL)
            if (Bstrcmp(varname, aGameVars[i].szLabel) == 0)
                SetGameVarID(i, varval, g_player[myconnectindex].ps->i, myconnectindex);
    return OSDCMD_OK;
}

static int osdcmd_addlogvar(const osdfuncparm_t *parm)
{
    int i;
    char varname[256];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    if (numplayers > 1)
    {
        OSD_Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }

    strcpy(varname,parm->parms[0]);
    for (i=0;i<iGameVarCount;i++)
        if (aGameVars[i].szLabel != NULL)
            if (Bstrcmp(varname, aGameVars[i].szLabel) == 0)
                OSD_Printf("%s = %d\n", varname, GetGameVarID(i, g_player[myconnectindex].ps->i, myconnectindex));
    return OSDCMD_OK;
}

static int osdcmd_setactorvar(const osdfuncparm_t *parm)
{
    int i, varval, ID;
    char varname[256];

    if (parm->numparms != 3) return OSDCMD_SHOWHELP;

    if (numplayers > 1)
    {
        OSD_Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }

    ID=Batol(parm->parms[0]);
    if (ID>=MAXSPRITES)
    {
        OSD_Printf("Invalid sprite ID\n");
        return OSDCMD_OK;
    }

    varval = Batol(parm->parms[2]);
    strcpy(varname,parm->parms[2]);
    varval = Batol(varname);
    for (i=0;i<iGameVarCount;i++)
        if (aGameVars[i].szLabel != NULL)
            if (Bstrcmp(varname, aGameVars[i].szLabel) == 0)
                varval=GetGameVarID(i, g_player[myconnectindex].ps->i, myconnectindex);

    strcpy(varname,parm->parms[1]);
    for (i=0;i<iGameVarCount;i++)
        if (aGameVars[i].szLabel != NULL)
            if (Bstrcmp(varname, aGameVars[i].szLabel) == 0)
                SetGameVarID(i, varval, ID, -1);
    return OSDCMD_OK;
}

static int osdcmd_addpath(const osdfuncparm_t *parm)
{
    char pathname[BMAX_PATH];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    strcpy(pathname,parm->parms[0]);
    addsearchpath(pathname);
    return OSDCMD_OK;
}

static int osdcmd_initgroupfile(const osdfuncparm_t *parm)
{
    char file[BMAX_PATH];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    strcpy(file,parm->parms[0]);
    initgroupfile(file);
    return OSDCMD_OK;
}

static int osdcmd_cmenu(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1) return OSDCMD_SHOWHELP;
    if (numplayers > 1)
    {
        OSD_Printf("cmenu: disallowed in multiplayer\n");
        return OSDCMD_OK;
    }
    else
    {
        cmenu(Batol(parm->parms[0]));
    }

    return OSDCMD_OK;
}

cvarmappings cvar[] =
{
    { "crosshair", "crosshair: enable/disable crosshair", (void*)&ud.crosshair, CVAR_BOOL, 0, 0, 1 },

    { "cl_autoaim", "cl_autoaim: enable/disable weapon autoaim", (void*)&ud.config.AutoAim, CVAR_INT|CVAR_MULTI, 0, 0, 2 },
    { "cl_automsg", "cl_automsg: enable/disable automatically sending messages to all players", (void*)&ud.automsg, CVAR_BOOL, 0, 0, 1 },
    { "cl_autovote", "cl_autovote: enable/disable automatic voting", (void*)&ud.autovote, CVAR_INT|CVAR_MULTI, 0, 0, 2 },


    { "cl_deathmessages", "cl_deathmessages: enable/disable multiplayer death messages", (void*)&ud.deathmsgs, CVAR_BOOL, 0, 0, 1 },
    { "cl_democams", "cl_democams: enable/disable demo playback cameras", (void*)&ud.democams, CVAR_BOOL, 0, 0, 1 },

    { "cl_idplayers", "cl_idplayers: enable/disable name display when aiming at opponents", (void*)&ud.idplayers, CVAR_BOOL, 0, 0, 1 },

    { "cl_messagetime", "cl_messagetime: length of time to display multiplayer chat messages\n", (void*)&ud.msgdisptime, CVAR_INT, 0, 0, 3600 },

    { "cl_showcoords", "cl_showcoords: show your position in the game world", (void*)&ud.coords, CVAR_BOOL, 0, 0, 1 },

    { "cl_viewbob", "cl_viewbob: enable/disable player head bobbing\n", (void*)&ud.viewbob, CVAR_BOOL, 0, 0, 1 },

    { "cl_weaponsway", "cl_weaponsway: enable/disable player weapon swaying\n", (void*)&ud.weaponsway, CVAR_BOOL, 0, 0, 1 },
    { "cl_weaponswitch", "cl_weaponswitch: enable/disable auto weapon switching", (void*)&ud.weaponswitch, CVAR_INT|CVAR_MULTI, 0, 0, 3 },
    { "cl_angleinterpolation", "cl_angleinterpolation: enable/disable angle interpolation", (void*)&ud.angleinterpolation, CVAR_INT, 0, 0, 256 },

    { "in_mousebias", "in_mousebias: emulates the original mouse code's weighting of input towards whichever axis is moving the most at any given time\n", (void*)&ud.config.MouseBias, CVAR_INT, 0, 0, 32 },
    { "in_mousefilter", "in_mousefilter: amount of mouse movement to filter out\n", (void*)&ud.config.MouseFilter, CVAR_INT, 0, 0, 512 },
    { "in_smoothinput", "in_smoothinput: enable/disable input smoothing\n", (void*)&ud.config.SmoothInput, CVAR_BOOL, 0, 0, 1 },

#if defined(POLYMOST) && defined(USE_OPENGL)
    { "r_anamorphic", "r_anamorphic: enable/disable widescreen mode", (void*)&glwidescreen, CVAR_BOOL, 0, 0, 1 },
    { "r_projectionhack", "r_projectionhack: enable/disable projection hack", (void*)&glprojectionhacks, CVAR_BOOL, 0, 0, 1 },
# ifdef POLYMER
    // polymer cvars
    { "pr_occlusionculling", "pr_occlusionculling: insert description that noone will ever read", (void*)&pr_occlusionculling, CVAR_INT, 0, 0, 512 },
    { "pr_fov", "pr_fov: sets the field of vision in build angle", (void*)&pr_fov, CVAR_INT, 0, 0, 1023},
    { "pr_billboardingmode", "pr_billboardingmode: face sprite display method. 0: classic mode; 1: polymost mode", (void*)&pr_billboardingmode, CVAR_INT, 0, 0, 1 },
    { "pr_verbosity", "pr_verbosity: verbosity level of the polymer renderer", (void*)&pr_verbosity, CVAR_INT, 0, 0, 3 },
    { "pr_wireframe", "pr_wireframe: toggles wireframe mode", (void*)&pr_wireframe, CVAR_INT, 0, 0, 1 },
    { "pr_vbos", "pr_vbos: contols Vertex Buffer Object usage. 0: no VBOs. 1: VBOs for map data. 2: VBOs for model data.", (void*)&pr_vbos, CVAR_INT, 0, 0, 2 },
    { "pr_gpusmoothing", "pr_gpusmoothing: toggles model animation interpolation", (void*)&pr_gpusmoothing, CVAR_INT, 0, 0, 1 },
#endif
#endif
    { "r_drawweapon", "r_drawweapon: enable/disable weapon drawing", (void*)&ud.drawweapon, CVAR_INT, 0, 0, 2 },
    { "r_showfps", "r_showfps: show the frame rate counter", (void*)&ud.tickrate, CVAR_BOOL, 0, 0, 1 },
    { "r_precache", "r_precache: enable/disable the pre-level caching routine", (void*)&ud.config.useprecache, CVAR_BOOL, 0, 0, 1 },
    { "r_visibility", "r_visibility: sets the global map visibility", (void*)&ud.const_visibility, CVAR_INT|128, 0, INT_MIN, INT_MAX },

    { "snd_ambience", "snd_ambience: enables/disables ambient sounds", (void*)&ud.config.AmbienceToggle, CVAR_BOOL, 0, 0, 1 },
    { "snd_duketalk", "snd_duketalk: enables/disables Duke's speech", (void*)&ud.config.VoiceToggle, CVAR_INT, 0, 0, 2 },
    { "snd_fxvolume", "snd_fxvolume: volume of sound effects", (void*)&ud.config.FXVolume, CVAR_INT, 0, 0, 255 },
    { "snd_mixrate", "snd_mixrate: sound mixing rate", (void*)&ud.config.MixRate, CVAR_INT, 0, 0, 48000 },
    { "snd_musvolume", "snd_musvolume: volume of midi music", (void*)&ud.config.MusicVolume, CVAR_INT, 0, 0, 255 },
    { "snd_numbits", "snd_numbits: sound bits", (void*)&ud.config.NumBits, CVAR_INT, 0, 8, 16 },
    { "snd_numchannels", "snd_numchannels: the number of sound channels", (void*)&ud.config.NumChannels, CVAR_INT, 0, 0, 2 },
    { "snd_numvoices", "snd_numvoices: the number of concurrent sounds", (void*)&ud.config.NumVoices, CVAR_INT, 0, 0, 32 },
    { "snd_reversestereo", "snd_reversestereo: reverses the stereo channels", (void*)&ud.config.ReverseStereo, CVAR_BOOL, 0, 0, 16 },
};

static int osdcmd_cvar_set(const osdfuncparm_t *parm)
{
    int showval = (parm->numparms == 0);
    unsigned int i;

    for (i = 0; i < sizeof(cvar)/sizeof(cvarmappings); i++)
    {
        if (!Bstrcasecmp(parm->name, cvar[i].name))
        {
            if ((cvar[i].type & CVAR_NOMULTI) && numplayers > 1)
            {
                // sound the alarm
                OSD_Printf("Cvar \"%s\" locked in multiplayer.\n",cvar[i].name);
                return OSDCMD_OK;
            }
            else
                switch (cvar[i].type&0x7f)
                {
                case CVAR_INT:
                case CVAR_UNSIGNEDINT:
                case CVAR_BOOL:
                {
                    int val;
                    if (showval)
                    {
                        OSD_Printf("\"%s\" is \"%d\"\n%s\n",cvar[i].name,*(int*)cvar[i].var,(char*)cvar[i].helpstr);
                        return OSDCMD_OK;
                    }

                    val = atoi(parm->parms[0]);
                    if (cvar[i].type == CVAR_BOOL) val = val != 0;

                    if (val < cvar[i].min || val > cvar[i].max)
                    {
                        OSD_Printf("%s value out of range\n",cvar[i].name);
                        return OSDCMD_OK;
                    }
                    *(int*)cvar[i].var = val;
                    OSD_Printf("%s %d",cvar[i].name,val);
                }
                break;
                case CVAR_STRING:
                {
                    if (showval)
                    {
                        OSD_Printf("\"%s\" is \"%s\"\n%s\n",cvar[i].name,(char*)cvar[i].var,(char*)cvar[i].helpstr);
                        return OSDCMD_OK;
                    }
                    else
                    {
                        Bstrncpy((char*)cvar[i].var, parm->parms[0], cvar[i].extra-1);
                        ((char*)cvar[i].var)[cvar[i].extra-1] = 0;
                        OSD_Printf("%s %s",cvar[i].name,(char*)cvar[i].var);
                    }
                }
                break;
                default:
                    break;
                }
            if (cvar[i].type&CVAR_MULTI)
                updateplayer();
        }
    }
    OSD_Printf("\n");
    return OSDCMD_OK;
}

static int osdcmd_sensitivity(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1)
    {
        OSD_Printf("\"sensitivity\" is \"%d\"\n",CONTROL_GetMouseSensitivity());
        return OSDCMD_SHOWHELP;
    }
    CONTROL_SetMouseSensitivity(atoi(parm->parms[0]));
    OSD_Printf("sensitivity %d\n",CONTROL_GetMouseSensitivity());
    return OSDCMD_OK;
}

static int osdcmd_setbrightness(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1)
    {
//        OSD_Printf("\"setbri\" \"%d\"\n",ud.brightness>>2);
        return OSDCMD_SHOWHELP;
    }
    ud.brightness = atoi(parm->parms[0])<<2;
    setbrightness(ud.brightness>>2,&g_player[screenpeek].ps->palette[0],0);
    OSD_Printf("setbrightness %d\n",ud.brightness>>2);
    return OSDCMD_OK;
}

static int osdcmd_give(const osdfuncparm_t *parm)
{
    int i;

    if (numplayers == 1 && g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        if (g_player[myconnectindex].ps->dead_flag != 0)
        {
            OSD_Printf("give: Cannot give while dead.\n");
            return OSDCMD_OK;
        }

        if (parm->numparms != 1) return OSDCMD_SHOWHELP;

        if (!Bstrcasecmp(parm->parms[0], "all"))
        {
            osdcmd_cheatsinfo_stat.cheatnum = 1;
            return OSDCMD_OK;
        }
        else if (!Bstrcasecmp(parm->parms[0], "health"))
        {
            sprite[g_player[myconnectindex].ps->i].extra = g_player[myconnectindex].ps->max_player_health<<1;
            return OSDCMD_OK;
        }
        else if (!Bstrcasecmp(parm->parms[0], "weapons"))
        {
            osdcmd_cheatsinfo_stat.cheatnum = 21;
            return OSDCMD_OK;
        }
        else if (!Bstrcasecmp(parm->parms[0], "ammo"))
        {
            for (i=PISTOL_WEAPON;i<MAX_WEAPONS-(VOLUMEONE?6:1);i++)
            {
                addammo(i,g_player[myconnectindex].ps,g_player[myconnectindex].ps->max_ammo_amount[i]);
            }
            return OSDCMD_OK;
        }
        else if (!Bstrcasecmp(parm->parms[0], "armor"))
        {
            g_player[myconnectindex].ps->shield_amount = 100;
            return OSDCMD_OK;
        }
        else if (!Bstrcasecmp(parm->parms[0], "keys"))
        {
            osdcmd_cheatsinfo_stat.cheatnum = 23;
            return OSDCMD_OK;
        }
        else if (!Bstrcasecmp(parm->parms[0], "inventory"))
        {
            osdcmd_cheatsinfo_stat.cheatnum = 22;
            return OSDCMD_OK;
        }
    }
    else
    {
        OSD_Printf("give: Not in a single-player game.\n");
        return OSDCMD_OK;
    }
    return OSDCMD_SHOWHELP;
}

void onvideomodechange(int newmode)
{
    char *pal;

    if (newmode)
    {
        if (g_player[screenpeek].ps->palette == palette ||
                g_player[screenpeek].ps->palette == waterpal ||
                g_player[screenpeek].ps->palette == titlepal ||
                g_player[screenpeek].ps->palette == animpal ||
                g_player[screenpeek].ps->palette == endingpal ||
                g_player[screenpeek].ps->palette == drealms ||
                g_player[screenpeek].ps->palette == slimepal)
            pal = g_player[screenpeek].ps->palette;
        else
            pal = palette;
    }
    else
    {
        pal = g_player[screenpeek].ps->palette;
    }

    setbrightness(ud.brightness>>2, pal, 0);
    restorepalette = 1;
}

static int osdcmd_usemousejoy(const osdfuncparm_t *parm)
{
    int showval = (parm->numparms < 1);
    if (!Bstrcasecmp(parm->name, "in_mouse"))
    {
        if (showval)
        {
            OSD_Printf("in_mouse is %d\n", ud.config.UseMouse);
        }
        else
        {
            ud.config.UseMouse = (atoi(parm->parms[0]) != 0);
            CONTROL_MouseEnabled = (ud.config.UseMouse && CONTROL_MousePresent);
        }
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->name, "in_joystick"))
    {
        if (showval)
        {
            OSD_Printf("in_joystick is %d\n", ud.config.UseJoystick);
        }
        else
        {
            ud.config.UseJoystick = (atoi(parm->parms[0]) != 0);
            CONTROL_JoystickEnabled = (ud.config.UseJoystick && CONTROL_JoyPresent);
        }
        return OSDCMD_OK;
    }
    return OSDCMD_SHOWHELP;
}

static int osdcmd_name(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1)
    {
        OSD_Printf("\"name\" is \"%s\"\n",myname);
        return OSDCMD_SHOWHELP;
    }

    Bstrcpy(tempbuf,parm->parms[0]);

    while (Bstrlen(stripcolorcodes(tempbuf)) > 10)
        tempbuf[Bstrlen(tempbuf)-1] = '\0';

    Bstrncpy(myname,tempbuf,sizeof(myname)-1);
    myname[sizeof(myname)-1] = '\0';

    OSD_Printf("name %s\n",myname);

    updateplayer();

    return OSDCMD_OK;
}

static int osdcmd_button(const osdfuncparm_t *parm)
{
    char *p = (char *)parm->name+9;  // skip "gamefunc_"
//    if (g_player[myconnectindex].ps->gm == MODE_GAME) // only trigger these if in game
    extinput[CONFIG_FunctionNameToNum(p)] = 1; // FIXME
    return OSDCMD_OK;
}

keydef keynames[]=
{
    { "Escape", 0x1 },
    { "1", 0x2 },
    { "2", 0x3 },
    { "3", 0x4 },
    { "4", 0x5 },
    { "5", 0x6 },
    { "6", 0x7 },
    { "7", 0x8 },
    { "8", 0x9 },
    { "9", 0xa },
    { "0", 0xb },
    { "-", 0xc },
    { "=", 0xd },
    { "BakSpc", 0xe },
    { "Tab", 0xf },
    { "Q", 0x10 },
    { "W", 0x11 },
    { "E", 0x12 },
    { "R", 0x13 },
    { "T", 0x14 },
    { "Y", 0x15 },
    { "U", 0x16 },
    { "I", 0x17 },
    { "O", 0x18 },
    { "P", 0x19 },
    { "[", 0x1a },
    { "]", 0x1b },
    { "Enter", 0x1c },
    { "LCtrl", 0x1d },
    { "A", 0x1e },
    { "S", 0x1f },
    { "D", 0x20 },
    { "F", 0x21 },
    { "G", 0x22 },
    { "H", 0x23 },
    { "J", 0x24 },
    { "K", 0x25 },
    { "L", 0x26 },
    { "SemiColon", 0x27 },
    { "'", 0x28 },
    { "Tilde", 0x29 },
    { "LShift", 0x2a },
    { "\\", 0x2b },
    { "Z", 0x2c },
    { "X", 0x2d },
    { "C", 0x2e },
    { "V", 0x2f },
    { "B", 0x30 },
    { "N", 0x31 },
    { "M", 0x32 },
    { ",", 0x33 },
    { ".", 0x34 },
    { "/", 0x35 },
    { "RShift", 0x36 },
    { "Kpad*", 0x37 },
    { "LAlt", 0x38 },
    { "Space", 0x39 },
    { "CapLck", 0x3a },
    { "F1", 0x3b },
    { "F2", 0x3c },
    { "F3", 0x3d },
    { "F4", 0x3e },
    { "F5", 0x3f },
    { "F6", 0x40 },
    { "F7", 0x41 },
    { "F8", 0x42 },
    { "F9", 0x43 },
    { "F10", 0x44 },
    { "NumLck", 0x45 },
    { "ScrLck", 0x46 },
    { "Kpad7", 0x47 },
    { "Kpad8", 0x48 },
    { "Kpad9", 0x49 },
    { "Kpad-", 0x4a },
    { "Kpad4", 0x4b },
    { "Kpad5", 0x4c },
    { "Kpad6", 0x4d },
    { "Kpad+", 0x4e },
    { "Kpad1", 0x4f },
    { "Kpad2", 0x50 },
    { "Kpad3", 0x51 },
    { "Kpad0", 0x52 },
    { "Kpad.", 0x53 },
    { "F11", 0x57 },
    { "F12", 0x58 },
    { "KpdEnt", 0x9c },
    { "RCtrl", 0x9d },
    { "Kpad/", 0xb5 },
    { "RAlt", 0xb8 },
    { "PrtScn", 0xb7 },
    { "Pause", 0xc5 },
    { "Home", 0xc7 },
    { "Up", 0xc8 },
    { "PgUp", 0xc9 },
    { "Left", 0xcb },
    { "Right", 0xcd },
    { "End", 0xcf },
    { "Down", 0xd0 },
    { "PgDn", 0xd1 },
    { "Insert", 0xd2 },
    { "Delete", 0xd3 },

    {0,0}
};

char *mousenames[] = { "mouse1", "mouse2", "mouse3", "mouse4", "mouse5", "mouse6" };

static int osdcmd_bind(const osdfuncparm_t *parm)
{
    int i, j;

    if (parm->numparms==1&&!Bstrcasecmp(parm->parms[0],"showkeys"))
    {
        for (i=0;keynames[i].name;i++)OSD_Printf("%s\n",keynames[i].name);
        for (i=0;i<MAXMOUSEBUTTONS;i++)OSD_Printf("%s\n",mousenames[i]);
        return OSDCMD_OK;
    }

    if (parm->numparms==0)
    {
        int j=0;

        OSD_Printf("Current key bindings:\n");
        for (i=0;i<MAXBOUNDKEYS;i++)
            if (boundkeys[i].cmd[0] && boundkeys[i].key)
            {
                j++;
                OSD_Printf("%-9s %s\"%s\"\n",boundkeys[i].key, boundkeys[i].repeat?"":"norepeat ", boundkeys[i].cmd);
            }

        for (i=0;i<MAXMOUSEBUTTONS;i++)
            if (mousebind[i].cmd[0] && mousebind[i].key)
            {
                j++;
                OSD_Printf("%-9s %s\"%s\"\n",mousebind[i].key, mousebind[i].repeat?"":"norepeat ",mousebind[i].cmd);
            }

        if (j == 0)
            OSD_Printf("No binds found.\n");

        return OSDCMD_OK;
    }

    for (i=0;keynames[i].name;i++)
        if (!Bstrcasecmp(parm->parms[0],keynames[i].name))
            break;

    if (!keynames[i].name)
    {
        for (i=0;i<MAXMOUSEBUTTONS;i++)
            if (!Bstrcasecmp(parm->parms[0],mousenames[i]))
                break;
        if (i >= MAXMOUSEBUTTONS)
            return OSDCMD_SHOWHELP;

        if (parm->numparms < 2)
        {
            OSD_Printf("%-9s %s\"%s\"\n",mousenames[i], mousebind[i].repeat?"":"norepeat ",mousebind[i].cmd);
            return OSDCMD_OK;
        }

        j = 1;

        mousebind[i].repeat = 1;
        if (parm->numparms >= 2 && !Bstrcasecmp(parm->parms[j],"norepeat"))
        {
            mousebind[i].repeat = 0;
            j++;
        }

        Bstrcpy(tempbuf,parm->parms[j++]);
        for (;j<parm->numparms;j++)
        {
            Bstrcat(tempbuf," ");
            Bstrcat(tempbuf,parm->parms[j++]);
        }
        Bstrncpy(mousebind[i].cmd,tempbuf, MAXBINDSTRINGLENGTH-1);

        mousebind[i].key=mousenames[i];
        if (!OSD_ParsingScript())
            OSD_Printf("%s\n",parm->raw);
        return OSDCMD_OK;
    }

    if (parm->numparms < 2)
    {
        OSD_Printf("%-9s %s\"%s\"\n",keynames[i].name, boundkeys[keynames[i].id].repeat?"":"norepeat ", boundkeys[keynames[i].id].cmd);
        return OSDCMD_OK;
    }

    j = 1;

    boundkeys[keynames[i].id].repeat = 1;
    if (parm->numparms >= 2 && !Bstrcasecmp(parm->parms[j],"norepeat"))
    {
        boundkeys[keynames[i].id].repeat = 0;
        j++;
    }

    Bstrcpy(tempbuf,parm->parms[j++]);
    for (;j<parm->numparms;j++)
    {
        Bstrcat(tempbuf," ");
        Bstrcat(tempbuf,parm->parms[j++]);
    }
    Bstrncpy(boundkeys[keynames[i].id].cmd,tempbuf, MAXBINDSTRINGLENGTH-1);

    boundkeys[keynames[i].id].key=keynames[i].name;
    if (!OSD_ParsingScript())
        OSD_Printf("%s\n",parm->raw);
    return OSDCMD_OK;
}

static int osdcmd_unbindall(const osdfuncparm_t *parm)
{
    int i;

    UNREFERENCED_PARAMETER(parm);

    for (i=0;i<MAXBOUNDKEYS;i++)
        if (boundkeys[i].cmd[0])
            boundkeys[i].cmd[0] = 0;
    for (i=0;i<MAXMOUSEBUTTONS;i++)
        if (mousebind[i].cmd[0])
            mousebind[i].cmd[0] = 0;
    OSD_Printf("unbound all keys\n");
    return OSDCMD_OK;
}

static int osdcmd_unbind(const osdfuncparm_t *parm)
{
    int i;

    if (parm->numparms < 1) return OSDCMD_SHOWHELP;
    for (i=0;keynames[i].name;i++)
        if (!Bstrcasecmp(parm->parms[0],keynames[i].name))
            break;
    if (!keynames[i].name)
    {
        for (i=0;i<MAXMOUSEBUTTONS;i++)
            if (!Bstrcasecmp(parm->parms[0],mousenames[i]))
                break;
        if (i >= MAXMOUSEBUTTONS)
            return OSDCMD_SHOWHELP;
        mousebind[i].repeat = 0;
        mousebind[i].cmd[0] = 0;
        OSD_Printf("unbound %s\n",mousenames[i]);
        return OSDCMD_OK;
    }
    boundkeys[keynames[i].id].repeat = 0;
    boundkeys[keynames[i].id].cmd[0] = 0;
    OSD_Printf("unbound key %s\n",keynames[i].name);
    return OSDCMD_OK;
}

static int osdcmd_quicksave(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
        OSD_Printf("quicksave: not in a game.\n");
    else doquicksave = 1;
    return OSDCMD_OK;
}

static int osdcmd_quickload(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
        OSD_Printf("quickload: not in a game.\n");
    else doquicksave = 2;
    return OSDCMD_OK;
}

static int osdcmd_screenshot(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
//    KB_ClearKeysDown();
    screencapture("duke0000.tga",0);
    return OSDCMD_OK;
}

extern void savemapstate(mapstate_t *save);
extern void restoremapstate(mapstate_t *save);

static int osdcmd_savestate(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    if (map[ud.volume_number*MAXLEVELS+ud.level_number].savedstate == NULL)
        map[ud.volume_number*MAXLEVELS+ud.level_number].savedstate = Bcalloc(1,sizeof(mapstate_t));
    savemapstate(map[ud.volume_number*MAXLEVELS+ud.level_number].savedstate);
    return OSDCMD_OK;
}

static int osdcmd_restorestate(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    if (map[ud.volume_number*MAXLEVELS+ud.level_number].savedstate)
        restoremapstate(map[ud.volume_number*MAXLEVELS+ud.level_number].savedstate);
    return OSDCMD_OK;
}

static int osdcmd_vid_gamma(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1)
    {
        OSD_Printf("\"vid_gamma\" is \"%.1f\"\n",vid_gamma);
        return OSDCMD_SHOWHELP;
    }
    vid_gamma = atof(parm->parms[0]);
    ud.brightness = min(max((float)((vid_gamma-1.0)*10.0),0),15);
    ud.brightness <<= 2;
    OSD_Printf("%s\n",parm->raw);
    setgamma();
    return OSDCMD_OK;
}

static int osdcmd_vid_brightness(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1)
    {
        OSD_Printf("\"vid_brightness\" is \"%.1f\"\n",vid_brightness);
        return OSDCMD_SHOWHELP;
    }
    vid_brightness = atof(parm->parms[0]);
    OSD_Printf("%s\n",parm->raw);
    setgamma();
    return OSDCMD_OK;
}

static int osdcmd_vid_contrast(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1)
    {
        OSD_Printf("\"vid_contrast\" is \"%.1f\"\n",vid_contrast);
        return OSDCMD_SHOWHELP;
    }
    vid_contrast = atof(parm->parms[0]);
    OSD_Printf("%s\n",parm->raw);
    setgamma();
    return OSDCMD_OK;
}

static int osdcmd_setcrosshairscale(const osdfuncparm_t *parm)
{
    if (parm->numparms == 0)
    {
        OSD_Printf("\"cl_crosshairscale\" is \"%d\"\n", ud.crosshairscale);
        return OSDCMD_SHOWHELP;
    }
    else if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    ud.crosshairscale = min(100,max(10,Batol(parm->parms[0])));
    OSD_Printf("cl_statusbarscale %d\n", ud.crosshairscale);
    return OSDCMD_OK;
}

int registerosdcommands(void)
{
    unsigned int i;

    osdcmd_cheatsinfo_stat.cheatnum = -1;

    for (i=0; i<sizeof(cvar)/sizeof(cvar[0]); i++)
    {
        OSD_RegisterFunction(cvar[i].name, cvar[i].helpstr, osdcmd_cvar_set);
    }

    if (VOLUMEONE)
        OSD_RegisterFunction("changelevel","changelevel <level>: warps to the given level", osdcmd_changelevel);
    else
    {
        OSD_RegisterFunction("changelevel","changelevel <volume> <level>: warps to the given level", osdcmd_changelevel);
        OSD_RegisterFunction("map","map <mapfile>: loads the given user map", osdcmd_map);
    }

    OSD_RegisterFunction("addpath","addpath <path>: adds path to game filesystem", osdcmd_addpath);

    OSD_RegisterFunction("bind","bind <key> <string>: associates a keypress with a string of console input. Type \"bind showkeys\" for a list of keys and \"listsymbols\" for a list of valid console commands.", osdcmd_bind);

    OSD_RegisterFunction("cl_statusbarscale","cl_statusbarscale: changes the status bar scale", osdcmd_setstatusbarscale);
    OSD_RegisterFunction("cl_crosshairscale","cl_crosshairscale: changes the crosshair scale", osdcmd_setcrosshairscale);
    OSD_RegisterFunction("cmenu","cmenu <#>: jumps to menu", osdcmd_cmenu);

    OSD_RegisterFunction("echo","echo [text]: echoes text to the console", osdcmd_echo);

    OSD_RegisterFunction("fileinfo","fileinfo <file>: gets a file's information", osdcmd_fileinfo);

    for (i=0;i<NUMGAMEFUNCTIONS-1;i++)
    {
        char *t;
        Bsprintf(tempbuf,"gamefunc_%s",gamefunctions[i]);
        t = Bstrdup(tempbuf);
        Bstrcat(tempbuf,": game button");
        OSD_RegisterFunction(t,Bstrdup(tempbuf),osdcmd_button);
    }

    OSD_RegisterFunction("setbrightness","setbrightness <value>: changes brightness (obsolete)", osdcmd_setbrightness);
    OSD_RegisterFunction("give","give <all|health|weapons|ammo|armor|keys|inventory>: gives requested item", osdcmd_give);
    OSD_RegisterFunction("god","god: toggles god mode", osdcmd_god);

    OSD_RegisterFunction("in_joystick","in_joystick: enables input from the joystick if it is present",osdcmd_usemousejoy);
    OSD_RegisterFunction("in_mouse","in_mouse: enables input from the mouse if it is present",osdcmd_usemousejoy);
    OSD_RegisterFunction("initgroupfile","initgroupfile <path>: adds a grp file into the game filesystem", osdcmd_initgroupfile);

    OSD_RegisterFunction("name","name: change your multiplayer nickname", osdcmd_name);
    OSD_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);

    OSD_RegisterFunction("quicksave","quicksave: performs a quick save", osdcmd_quicksave);
    OSD_RegisterFunction("quickload","quickload: performs a quick load", osdcmd_quickload);
    OSD_RegisterFunction("quit","quit: exits the game immediately", osdcmd_quit);
    OSD_RegisterFunction("exit","exit: exits the game immediately", osdcmd_quit);

    OSD_RegisterFunction("rate","rate: sets the multiplayer packet send rate, in packets/sec",osdcmd_rate);
    OSD_RegisterFunction("restartsound","restartsound: reinitializes the sound system",osdcmd_restartsound);
    OSD_RegisterFunction("restartvid","restartvid: reinitializes the video mode",osdcmd_restartvid);

    OSD_RegisterFunction("sensitivity","sensitivity <value>: changes the mouse sensitivity", osdcmd_sensitivity);
    OSD_RegisterFunction("addlogvar","addlogvar <gamevar>: prints the value of a gamevar", osdcmd_addlogvar);
    OSD_RegisterFunction("setvar","setvar <gamevar> <value>: sets the value of a gamevar", osdcmd_setvar);
    OSD_RegisterFunction("setvarvar","setvar <gamevar> <gamevar>: sets the value of a gamevar", osdcmd_setvar);
    OSD_RegisterFunction("setactorvar","setactorvar <actorID> <gamevar> <value>: sets the value of a gamevar", osdcmd_setactorvar);
    OSD_RegisterFunction("screenshot","screenshot: takes a screenshot.  See r_scrcaptureformat.", osdcmd_screenshot);

    OSD_RegisterFunction("spawn","spawn <picnum> [palnum] [cstat] [ang] [x y z]: spawns a sprite with the given properties",osdcmd_spawn);

    OSD_RegisterFunction("unbind","unbind <key>: unbinds a key.", osdcmd_unbind);
    OSD_RegisterFunction("unbindall","unbindall: unbinds all keys.", osdcmd_unbindall);

    OSD_RegisterFunction("vidmode","vidmode [xdim ydim] [bpp] [fullscreen]: change the video mode",osdcmd_vidmode);
    OSD_RegisterFunction("vid_gamma","vid_gamma <gamma>: adjusts gamma ramp",osdcmd_vid_gamma);
    OSD_RegisterFunction("vid_contrast","vid_contrast <gamma>: adjusts gamma ramp",osdcmd_vid_contrast);
    OSD_RegisterFunction("vid_brightness","vid_brightness <gamma>: adjusts gamma ramp",osdcmd_vid_brightness);
    OSD_RegisterFunction("savestate","",osdcmd_savestate);
    OSD_RegisterFunction("restorestate","",osdcmd_restorestate);
    //baselayer_onvideomodechange = onvideomodechange;

    return 0;
}

