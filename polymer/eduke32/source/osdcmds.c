//-------------------------------------------------------------------------
/*
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

#include "osdcmds.h"
#include "osd.h"
#include "baselayer.h"
#include "duke3d.h"
#include "crc32.h"

#include <ctype.h>

struct osdcmd_cheatsinfo osdcmd_cheatsinfo_stat;

int osdcmd_quit(const osdfuncparm_t *parm)
{
    sendquit();
    return OSDCMD_OK;
}

int osdcmd_echo(const osdfuncparm_t *parm)
{
    int i;
    for (i = 0; i < parm->numparms; i++) {
        if (i > 0) OSD_Printf(" ");
        OSD_Printf("%s", parm->parms[i]);
    }
    OSD_Printf("\n");

    return OSDCMD_OK;
}

int osdcmd_changelevel(const osdfuncparm_t *parm)
{
    int volume=0,level;
    char *p;

    if (!VOLUMEONE) {
        if (parm->numparms != 2) return OSDCMD_SHOWHELP;

        volume = strtol(parm->parms[0], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
        level = strtol(parm->parms[1], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
    } else {
        if (parm->numparms != 1) return OSDCMD_SHOWHELP;

        level = strtol(parm->parms[0], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
    }

    if (volume < 0) return OSDCMD_SHOWHELP;
    if (level < 0) return OSDCMD_SHOWHELP;

    if (!VOLUMEONE) {
        if (volume > num_volumes) {
            OSD_Printf("changelevel: invalid volume number (range 1-%ld)\n",num_volumes);
            return OSDCMD_OK;
        }
    }

    if (volume == 0) {
        if (level > 6) {
            OSD_Printf("changelevel: invalid volume 1 level number (range 1-7)\n");
            return OSDCMD_OK;
        }
    } else {
        if (level > 10) {
            OSD_Printf("changelevel: invalid volume 2+ level number (range 1-11)\n");
            return OSDCMD_SHOWHELP;
        }
    }

    if (ps[myconnectindex].gm & MODE_GAME) {
        // in-game behave like a cheat
        osdcmd_cheatsinfo_stat.cheatnum = 2;
        osdcmd_cheatsinfo_stat.volume   = volume;
        osdcmd_cheatsinfo_stat.level    = level;
    } else {
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

int osdcmd_map(const osdfuncparm_t *parm)
{
    int i;
    char filename[256];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    strcpy(filename,parm->parms[0]);
    if( strchr(filename,'.') == 0)
        strcat(filename,".map");

    if ((i = kopen4load(filename,0)) < 0) {
        OSD_Printf("map: file \"%s\" not found.\n", filename);
        return OSDCMD_OK;
    }
    kclose(i);

    strcpy(boardfilename, filename);

    if (ps[myconnectindex].gm & MODE_GAME) {
        // in-game behave like a cheat
        osdcmd_cheatsinfo_stat.cheatnum = 2;
        osdcmd_cheatsinfo_stat.volume = 0;
        osdcmd_cheatsinfo_stat.level = 7;
    } else {
        // out-of-game behave like a menu command
        osdcmd_cheatsinfo_stat.cheatnum = -1;

        ud.m_volume_number = 0;
        ud.m_level_number = 7;

        ud.m_monsters_off = ud.monsters_off = 0;

        ud.m_respawn_items = 0;
        ud.m_respawn_inventory = 0;

        ud.multimode = 1;

        newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);
        if (enterlevel(MODE_GAME)) backtomenu();
    }

    return OSDCMD_OK;
}

int osdcmd_god(const osdfuncparm_t *parm)
{
    if (numplayers == 1 && ps[myconnectindex].gm & MODE_GAME) {
        osdcmd_cheatsinfo_stat.cheatnum = 0;
    } else {
        OSD_Printf("god: Not in a single-player game.\n");
    }

    return OSDCMD_OK;
}

int osdcmd_noclip(const osdfuncparm_t *parm)
{
    if (numplayers == 1 && ps[myconnectindex].gm & MODE_GAME) {
        osdcmd_cheatsinfo_stat.cheatnum = 20;
    } else {
        OSD_Printf("noclip: Not in a single-player game.\n");
    }

    return OSDCMD_OK;
}

int osdcmd_fileinfo(const osdfuncparm_t *parm)
{
    unsigned long crc, length;
    int i,j;
    char buf[256];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    if ((i = kopen4load((char *)parm->parms[0],0)) < 0) {
        OSD_Printf("fileinfo: File \"%s\" not found.\n", parm->parms[0]);
        return OSDCMD_OK;
    }

    length = kfilelength(i);

    crc32init(&crc);
    do {
        j = kread(i,buf,256);
        crc32block(&crc,buf,j);
    } while (j == 256);
    crc32finish(&crc);

    kclose(i);

    OSD_Printf("fileinfo: %s\n"
               "  File size: %d\n"
               "  CRC-32:    %08X\n",
               parm->parms[0], length, crc);

    return OSDCMD_OK;
}

static int osdcmd_restartvid(const osdfuncparm_t *parm)
{
    resetvideomode();
    if (setgamemode(ScreenMode,ScreenWidth,ScreenHeight,ScreenBPP))
        gameexit("restartvid: Reset failed...\n");
    onvideomodechange(ScreenBPP>8);
    vscrn();

    return OSDCMD_OK;
}

static int osdcmd_vidmode(const osdfuncparm_t *parm)
{
    int newbpp = ScreenBPP, newwidth = ScreenWidth,
                                       newheight = ScreenHeight, newfs = ScreenMode;
    if (parm->numparms < 1 || parm->numparms > 4) return OSDCMD_SHOWHELP;

    switch (parm->numparms) {
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

    if (setgamemode(newfs,newwidth,newheight,newbpp)) {
        initprintf("vidmode: Mode change failed!\n");
        if (setgamemode(ScreenMode, ScreenWidth, ScreenHeight, ScreenBPP))
            gameexit("vidmode: Reset failed!\n");
    }
    ScreenBPP = newbpp; ScreenWidth = newwidth; ScreenHeight = newheight;
    ScreenMode = newfs;
    onvideomodechange(ScreenBPP>8);
    vscrn();
    return OSDCMD_OK;
}

static int osdcmd_setstatusbarscale(const osdfuncparm_t *parm)
{
    if (parm->numparms == 0) {
        OSD_Printf("\"cl_statusbarscale\" is \"%d\"\n", ud.statusbarscale);
        return OSDCMD_SHOWHELP;
    } else if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    setstatusbarscale(Batol(parm->parms[0]));
    OSD_Printf("cl_statusbarscale %d\n", ud.statusbarscale);
    return OSDCMD_OK;
}

static int osdcmd_setstatusbarmode(const osdfuncparm_t *parm)
{
    if (parm->numparms == 0) {
        OSD_Printf("\"cl_statusbarmode\" is \"%d\"\n", ud.statusbarmode);
        return OSDCMD_SHOWHELP;
    } else if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    ud.statusbarmode = Batol(parm->parms[0]);
    vscrn();
    OSD_Printf("cl_statusbarmode %d\n", ud.statusbarmode);
    return OSDCMD_OK;
}

static int osdcmd_spawn(const osdfuncparm_t *parm)
{
    long x=0,y=0,z=0;
    unsigned short cstat=0,picnum=0;
    unsigned char pal=0;
    short ang=0;
    short set=0, idx;

    if (numplayers > 1 || !(ps[myconnectindex].gm & MODE_GAME)) {
        OSD_Printf("spawn: Can't spawn sprites in multiplayer games or demos\n");
        return OSDCMD_OK;
    }

    switch (parm->numparms) {
    case 7: // x,y,z
        x = Batol(parm->parms[4]);
        y = Batol(parm->parms[5]);
        z = Batol(parm->parms[6]);
        set |= 8;
    case 4: // ang
        ang = Batol(parm->parms[3]) & 2047; set |= 4;
    case 3: // cstat
        cstat = (unsigned short)Batol(parm->parms[2]); set |= 2;
    case 2: // pal
        pal = (unsigned char)Batol(parm->parms[1]); set |= 1;
    case 1: // tile number
        if (isdigit(parm->parms[0][0])) {
            picnum = (unsigned short)Batol(parm->parms[0]);
        } else {
            int i,j;
            for (j=0; j<2; j++) {
                for (i=0; i<labelcnt; i++) {
                    if (
                        (j == 0 && !Bstrcmp(label+(i<<6),     parm->parms[0])) ||
                        (j == 1 && !Bstrcasecmp(label+(i<<6), parm->parms[0]))
                    ) {
                        picnum = (unsigned short)labelcode[i];
                        break;
                    }
                }
                if (i<labelcnt) break;
            }
            if (i==labelcnt) {
                OSD_Printf("spawn: Invalid tile label given\n");
                return OSDCMD_OK;
            }
        }

        if (picnum >= MAXTILES) {
            OSD_Printf("spawn: Invalid tile number\n");
            return OSDCMD_OK;
        }
        break;
    default:
        return OSDCMD_SHOWHELP;
    }

    idx = spawn(ps[myconnectindex].i, (short)picnum);
    if (set & 1) sprite[idx].pal = (char)pal;
    if (set & 2) sprite[idx].cstat = (short)cstat;
    if (set & 4) sprite[idx].ang = ang;
    if (set & 8) {
        if (setsprite(idx, x,y,z) < 0) {
            OSD_Printf("spawn: Sprite can't be spawned into null space\n");
            deletesprite(idx);
        }
    }

    return OSDCMD_OK;
}

int osdcmd_setvar(const osdfuncparm_t *parm)
{
    int i, varval;
    char varname[256];

    if (parm->numparms != 2) return OSDCMD_SHOWHELP;

    if (ud.multimode != 1)
    {
        OSD_Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }

    strcpy(varname,parm->parms[0]);
    varval = Batol(parm->parms[1]);

    for(i=0;i<iGameVarCount;i++)
        if(aGameVars[i].szLabel != NULL)
            if(Bstrcmp(varname, aGameVars[i].szLabel) == 0)
                SetGameVarID(i, varval, ps[myconnectindex].i, myconnectindex);
    return OSDCMD_OK;
}

int osdcmd_addpath(const osdfuncparm_t *parm)
{
    char pathname[BMAX_PATH];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    strcpy(pathname,parm->parms[0]);
    addsearchpath(pathname);
    return OSDCMD_OK;
}

int osdcmd_cmenu(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1) return OSDCMD_SHOWHELP;
    if (numplayers > 1) {
        OSD_Printf("cmenu: disallowed in multiplayer\n");
        return OSDCMD_OK;
    } else {
        cmenu(Batol(parm->parms[0]));
    }

    return OSDCMD_OK;
}

int osdcmd_exec(const osdfuncparm_t *parm)
{
    char fn[BMAX_PATH];
    extern int load_script(char *szStartupScript);

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;
    Bstrcpy(fn,parm->parms[0]);

    if (load_script(fn)) {
        OSD_Printf("exec: file \"%s\" not found.\n", fn);
        return OSDCMD_OK;
    }
    return OSDCMD_OK;
}

enum cvartypes {
    CVAR_INT,
    CVAR_UNSIGNEDINT,
    CVAR_BOOL,
    CVAR_STRING
};

struct cvarmappings {
    char *name;
    char *helpstr;
    void *var;
    int type;       // 0 = integer, 1 = unsigned integer, 2 = boolean, 3 = string, |128 = not in multiplayer, |256 = update multi
    int extra;      // for string, is the length
    int min;
    int max;
} cvar[] =
    {
        { "crosshair", "crosshair: enable/disable crosshair", (void*)&ud.crosshair, CVAR_INT, 0, 0, 3 },

        { "cl_autoaim", "cl_autoaim: enable/disable weapon autoaim", (void*)&AutoAim, CVAR_INT|256, 0, 0, 2 },
        { "cl_automsg", "cl_automsg: enable/disable automatically sending messages to all players", (void*)&ud.automsg, CVAR_BOOL, 0, 0, 1 },
        { "cl_autovote", "cl_autovote: enable/disable automatic voting", (void*)&ud.autovote, CVAR_INT|256, 0, 0, 2 },

        { "cl_democams", "cl_democams: enable/disable demo playback cameras", (void*)&ud.democams, CVAR_BOOL, 0, 0, 1 },
        { "cl_drawweapon", "cl_drawweapon: enable/disable weapon drawing", (void*)&ud.drawweapon, CVAR_INT, 0, 0, 2 },

        { "cl_idplayers", "cl_idplayers: enable/disable name display when aiming at opponents", (void*)&ud.idplayers, CVAR_BOOL, 0, 0, 1 },

        { "cl_messagetime", "cl_messagetime: length of time to display multiplayer chat messages\n", (void*)&ud.msgdisptime, CVAR_INT, 0, 0, 3600 },

        { "cl_mousefilter", "cl_mousefilter: amount of mouse movement to filter out\n", (void*)&MouseFilter, CVAR_INT, 0, 0, 512 },

        { "cl_showcoords", "cl_showcoords: show your position in the game world", (void*)&ud.coords, CVAR_BOOL, 0, 0, 1 },
        { "cl_showfps", "cl_showfps: show the frame rate counter", (void*)&ud.tickrate, CVAR_BOOL, 0, 0, 1 },
        { "cl_smoothinput", "cl_smoothinput: enable/disable input smoothing\n", (void*)&SmoothInput, CVAR_BOOL, 0, 0, 1 },

        { "cl_weaponswitch", "cl_weaponswitch: enable/disable auto weapon switching", (void*)&ud.weaponswitch, CVAR_INT|256, 0, 0, 3 },

        { "r_arcorrection", "r_arcorrection: sets amount of aspect ratio correction", (void*)&glratiocorrection, CVAR_INT, 0, 0, 63 },
        { "r_hudcorrect", "r_hudcorrect: enable/disable correct HUD weapon rendering", (void*)&glhudcorrect, CVAR_BOOL, 0, 0, 1 },
        { "r_precache", "r_precache: enable/disable the pre-level caching routine", (void*)&useprecache, CVAR_BOOL, 0, 0, 1 }
    };

int osdcmd_cvar_set(const osdfuncparm_t *parm)
{
    int showval = (parm->numparms == 0);
    unsigned int i;

    for (i = 0; i < sizeof(cvar)/sizeof(struct cvarmappings); i++) {
        if (!Bstrcasecmp(parm->name, cvar[i].name)) {
            if ((cvar[i].type & 0x80) && ud.multimode != 1) {
                // sound the alarm
                OSD_Printf("Cvar \"%s\" locked in multiplayer.\n",cvar[i].name);
                return OSDCMD_OK;
            } else
                switch (cvar[i].type&0x7f) {
                case CVAR_INT:
                case CVAR_UNSIGNEDINT:
                case CVAR_BOOL:
                    {
                        int val;
                        if (showval) {
                            OSD_Printf("\"%s\" is \"%d\"\n%s\n",cvar[i].name,*(int*)cvar[i].var,(char*)cvar[i].helpstr);
                            return OSDCMD_OK;
                        }

                        val = atoi(parm->parms[0]);
                        if (cvar[i].type == CVAR_BOOL) val = val != 0;

                        if (val < cvar[i].min || val > cvar[i].max) {
                            OSD_Printf("%s value out of range\n",cvar[i].name);
                            return OSDCMD_OK;
                        }
                        *(int*)cvar[i].var = val;
                        OSD_Printf("%s %d",cvar[i].name,val);
                    } break;
                case CVAR_STRING:
                    {
                        if (showval) {
                            OSD_Printf("\"%s\" is \"%s\"\n%s\n",cvar[i].name,(char*)cvar[i].var,(char*)cvar[i].helpstr);
                            return OSDCMD_OK;
                        }
                        else {
                            Bstrncpy((char*)cvar[i].var, parm->parms[0], cvar[i].extra-1);
                            ((char*)cvar[i].var)[cvar[i].extra-1] = 0;
                            OSD_Printf("%s %s",cvar[i].name,(char*)cvar[i].var);
                        }
                    } break;
                default: break;
                }
            if(cvar[i].type&256)
                updatenames();
        }
    }
    OSD_Printf("\n");
    return OSDCMD_OK;
}

int osdcmd_sensitivity(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1) {
        OSD_Printf("\"sensitivity\" is \"%d\"\n",CONTROL_GetMouseSensitivity());
        return OSDCMD_SHOWHELP;
    }
    CONTROL_SetMouseSensitivity(atoi(parm->parms[0]));
    OSD_Printf("sensitivity %d\n",CONTROL_GetMouseSensitivity());
    return OSDCMD_OK;
}

int osdcmd_gamma(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1) {
        OSD_Printf("\"gamma\" \"%d\"\n",ud.brightness>>2);
        return OSDCMD_SHOWHELP;
    }
    ud.brightness = atoi(parm->parms[0])<<2;
    setbrightness(ud.brightness>>2,&ps[screenpeek].palette[0],0);
    OSD_Printf("gamma %d\n",ud.brightness>>2);
    return OSDCMD_OK;
}

int osdcmd_give(const osdfuncparm_t *parm)
{
    int i;

    if (numplayers == 1 && ps[myconnectindex].gm & MODE_GAME) {
        if (parm->numparms != 1) return OSDCMD_SHOWHELP;
        if (!Bstrcasecmp(parm->parms[0], "all")) {
            osdcmd_cheatsinfo_stat.cheatnum = 1;
            return OSDCMD_OK;
        }
        else if (!Bstrcasecmp(parm->parms[0], "health")) {
            sprite[ps[myconnectindex].i].extra = 200;
            return OSDCMD_OK;
        }
        else if (!Bstrcasecmp(parm->parms[0], "weapons")) {
            osdcmd_cheatsinfo_stat.cheatnum = 21;
            return OSDCMD_OK;
        }
        else if (!Bstrcasecmp(parm->parms[0], "ammo")) {
            for (i=PISTOL_WEAPON;i<MAX_WEAPONS-(VOLUMEONE?6:1);i++)
            {
                addammo(i,&ps[myconnectindex],max_ammo_amount[i]);
            }
            return OSDCMD_OK;
        }
        else if (!Bstrcasecmp(parm->parms[0], "armor")) {
            ps[myconnectindex].shield_amount = 100;
            return OSDCMD_OK;
        }
        else if (!Bstrcasecmp(parm->parms[0], "keys")) {
            osdcmd_cheatsinfo_stat.cheatnum = 23;
            return OSDCMD_OK;
        }
        else if (!Bstrcasecmp(parm->parms[0], "inventory")) {
            osdcmd_cheatsinfo_stat.cheatnum = 22;
            return OSDCMD_OK;
        }
    } else {
        OSD_Printf("give: Not in a single-player game.\n");
        return OSDCMD_OK;
    }
    return OSDCMD_SHOWHELP;
}

void onvideomodechange(int newmode)
{
    char *pal;

    if (newmode) {
        if (ps[screenpeek].palette == palette ||
                ps[screenpeek].palette == waterpal ||
                ps[screenpeek].palette == slimepal)
            pal = palette;
        else
            pal = ps[screenpeek].palette;
    } else {
        pal = ps[screenpeek].palette;
    }

    setbrightness(ud.brightness>>2, pal, 0);
    restorepalette = 1;
}

int osdcmd_usemousejoy(const osdfuncparm_t *parm)
{
    int showval = (parm->numparms < 1);
    if (!Bstrcasecmp(parm->name, "usemouse")) {
        if (showval) { OSD_Printf("usemouse is %d\n", UseMouse); }
        else {
            UseMouse = (atoi(parm->parms[0]) != 0);
            CONTROL_MouseEnabled = (UseMouse && CONTROL_MousePresent);
        }
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->name, "usejoystick")) {
        if (showval) { OSD_Printf("usejoystick is %d\n", UseJoystick); }
        else {
            UseJoystick = (atoi(parm->parms[0]) != 0);
            CONTROL_JoystickEnabled = (UseJoystick && CONTROL_JoyPresent);
        }
        return OSDCMD_OK;
    }
    return OSDCMD_SHOWHELP;
}

int osdcmd_mpmap(const osdfuncparm_t *parm)
{
    int i;
    char filename[256];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    strcpy(filename,parm->parms[0]);
    if( strchr(filename,'.') == 0)
        strcat(filename,".map");

    if ((i = kopen4load(filename,0)) < 0) {
        OSD_Printf("map: file \"%s\" not found.\n", filename);
        return OSDCMD_OK;
    }
    kclose(i);

    strcpy(boardfilename, filename);

    sendboardname();

    return OSDCMD_OK;
}

int osdcmd_name(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1)
    {
        OSD_Printf("\"name\" is \"%s\"\n",myname);
        return OSDCMD_SHOWHELP;
    }

    Bstrcpy(tempbuf,parm->parms[0]);

    while(Bstrlen(strip_color_codes(tempbuf)) > 10)
        tempbuf[Bstrlen(tempbuf)-1] = '\0';

    Bstrncpy(myname,tempbuf,sizeof(myname)-1);
    myname[sizeof(myname)] = '\0';

    OSD_Printf("name %s\n",myname);

    updatenames();

    return OSDCMD_OK;
}

int registerosdcommands(void)
{
    unsigned int i;

    osdcmd_cheatsinfo_stat.cheatnum = -1;

    for (i=0; i<sizeof(cvar)/sizeof(cvar[0]); i++) {
        OSD_RegisterFunction(cvar[i].name, cvar[i].helpstr, osdcmd_cvar_set);
    }

    OSD_RegisterFunction("echo","echo [text]: echoes text to the console", osdcmd_echo);

    if (VOLUMEONE) {
        OSD_RegisterFunction("changelevel","changelevel <level>: warps to the given level", osdcmd_changelevel);
    } else {
        OSD_RegisterFunction("changelevel","changelevel <volume> <level>: warps to the given level", osdcmd_changelevel);
        OSD_RegisterFunction("map","map <mapfile>: loads the given user map", osdcmd_map);
        OSD_RegisterFunction("mpmap","mpmap <mapfile>: sets user map name in multiplayer", osdcmd_mpmap);
    }
    OSD_RegisterFunction("cmenu","cmenu <#>: jumps to menu", osdcmd_cmenu);
    OSD_RegisterFunction("exec","exec <scriptfile>: executes a script", osdcmd_exec);
    OSD_RegisterFunction("god","god: toggles god mode", osdcmd_god);
    OSD_RegisterFunction("gamma","gamma <value>: changes brightness", osdcmd_gamma);
    OSD_RegisterFunction("give","give <all|health|weapons|ammo|armor|keys|inventory>: gives requested item", osdcmd_give);
    OSD_RegisterFunction("name","name: change your multiplayer nickname", osdcmd_name);
    OSD_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);

    OSD_RegisterFunction("cl_statusbarscale","cl_statusbarscale: changes the status bar scale", osdcmd_setstatusbarscale);
    OSD_RegisterFunction("cl_statusbarmode","cl_statusbarmode: enable/disable status bar drawing hack", osdcmd_setstatusbarmode);
    OSD_RegisterFunction("sensitivity","sensitivity <value>: changes the mouse sensitivity", osdcmd_sensitivity);
    OSD_RegisterFunction("spawn","spawn <picnum> [palnum] [cstat] [ang] [x y z]: spawns a sprite with the given properties",osdcmd_spawn);
    OSD_RegisterFunction("setvar","setvar <gamevar> <value>: sets the value of a gamevar", osdcmd_setvar);
    OSD_RegisterFunction("addpath","addpath <path>: adds path to game filesystem", osdcmd_addpath);

    OSD_RegisterFunction("fileinfo","fileinfo <file>: gets a file's information", osdcmd_fileinfo);
    OSD_RegisterFunction("quit","quit: exits the game immediately", osdcmd_quit);

    OSD_RegisterFunction("restartvid","restartvid: reinitialised the video mode",osdcmd_restartvid);
    OSD_RegisterFunction("vidmode","vidmode [xdim ydim] [bpp] [fullscreen]: immediately change the video mode",osdcmd_vidmode);

    OSD_RegisterFunction("usemouse","usemouse: enables input from the mouse if it is present",osdcmd_usemousejoy);
    OSD_RegisterFunction("usejoystick","usejoystick: enables input from the joystick if it is present",osdcmd_usemousejoy);

    //baselayer_onvideomodechange = onvideomodechange;

    return 0;
}

