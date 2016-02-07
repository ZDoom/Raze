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

#define game_c_

#include "duke3d.h"
#include "compat.h"
#include "renderlayer.h"
#include "osdfuncs.h"
#include "osdcmds.h"
#include "crc32.h"
#include "net.h"
#include "menus.h"
#include "savegame.h"
#include "anim.h"
#include "demo.h"
#include "input.h"
#include "colmatch.h"
#include "cheats.h"
#include "sbar.h"

#ifdef __ANDROID__
#include "android.h"
#endif

#ifdef LUNATIC
# include "lunatic_game.h"
#endif

// Uncomment to prevent anything except mirrors from drawing. It is sensible to
// also uncomment ENGINE_CLEAR_SCREEN in build/src/engine_priv.h.
//#define DEBUG_MIRRORS_ONLY

#if KRANDDEBUG
# define GAME_INLINE
# define GAME_STATIC
#else
# define GAME_INLINE inline
# define GAME_STATIC static
#endif

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <shellapi.h>
# define UPDATEINTERVAL 604800 // 1w
# include "winbits.h"
#else
# ifndef GEKKO
#  include <sys/ioctl.h>
# endif
#endif /* _WIN32 */

const char* AppProperName = "EDuke32";
const char* AppTechnicalName = "eduke32";

int32_t g_quitDeadline = 0;

#ifdef LUNATIC
camera_t g_camera;
#else
int32_t g_cameraDistance = 0, g_cameraClock = 0;
#endif
static int32_t g_quickExit;
static int32_t g_commandSetup = 0;
int32_t g_noSetup = 0;
static int32_t g_noAutoLoad = 0;
static int32_t g_noSound = 0;
static int32_t g_noMusic = 0;
static const char *CommandMap = NULL;
static const char *CommandName = NULL;
int32_t g_forceWeaponChoice = 0;
int32_t g_fakeMultiMode = 0;

double g_moveActorsTime = 0;  // in ms, smoothed

char boardfilename[BMAX_PATH] = {0}, currentboardfilename[BMAX_PATH] = {0};

int32_t voting = -1;
int32_t vote_map = -1, vote_episode = -1;

int32_t g_Debug = 0;
static int32_t g_noLogoAnim = 0;
static int32_t g_noLogo = 0;

const char *defaultrtsfilename[GAMECOUNT] = { "DUKE.RTS", "NAM.RTS", "NAPALM.RTS", "WW2GI.RTS" };

// g_rtsNamePtr can point to an argv[] element
const char *g_rtsNamePtr = NULL;

int32_t g_Shareware = 0;

// This was 32 for a while, but I think lowering it to 24 will help things like the Dingoo.
// Ideally, we would look at our memory usage on our most cramped platform and figure out
// how much of that is needed for the underlying OS and things like SDL instead of guessing
#ifndef GEKKO
static int32_t MAXCACHE1DSIZE = (24*1048576);
#else
static int32_t MAXCACHE1DSIZE = (8*1048576);
#endif

int32_t tempwallptr;

static int32_t nonsharedtimer;

int32_t ticrandomseed;

static void G_DrawCameraText(int16_t i);
GAME_STATIC GAME_INLINE int32_t G_MoveLoop(void);
static void G_DoOrderScreen(void);

#define quotepulseshade (sintable[((uint32_t)totalclock<<5)&2047]>>11)

int32_t hud_showmapname = 1;

int32_t g_levelTextTime = 0;

int32_t r_maxfps = 0;
uint32_t g_frameDelay = 0;

#if defined(RENDERTYPEWIN) && defined(USE_OPENGL)
extern char forcegl;
#endif

void M32RunScript(const char *s) { UNREFERENCED_PARAMETER(s); };  // needed for linking since it's referenced from build/src/osd.c

const char *G_DefaultRtsFile(void)
{
    if (DUKE)
        return defaultrtsfilename[GAME_DUKE];
    else if (WW2GI)
        return defaultrtsfilename[GAME_WW2GI];
    else if (NAPALM)
    {
        if (!testkopen(defaultrtsfilename[GAME_NAPALM],0) && testkopen(defaultrtsfilename[GAME_NAM],0))
            return defaultrtsfilename[GAME_NAM]; // NAM/NAPALM Sharing
        else
            return defaultrtsfilename[GAME_NAPALM];
    }
    else if (NAM)
    {
        if (!testkopen(defaultrtsfilename[GAME_NAM],0) && testkopen(defaultrtsfilename[GAME_NAPALM],0))
            return defaultrtsfilename[GAME_NAPALM]; // NAM/NAPALM Sharing
        else
            return defaultrtsfilename[GAME_NAM];
    }

    return defaultrtsfilename[0];
}

enum gametokens
{
    T_INCLUDE = 0,
    T_INTERFACE = 0,
    T_LOADGRP = 1,
    T_MODE = 1,
    T_CACHESIZE = 2,
    T_ALLOW = 2,
    T_NOAUTOLOAD,
    T_INCLUDEDEFAULT,
    T_MUSIC,
    T_SOUND,
    T_FILE,
    T_CUTSCENE,
    T_ANIMSOUNDS,
    T_NOFLOORPALRANGE,
    T_ID,
    T_DELAY,
    T_RENAMEFILE,
    T_GLOBALGAMEFLAGS,
};

void P_SetGamePalette(DukePlayer_t *player, uint32_t palid, int32_t set)
{
    if (palid >= MAXBASEPALS)
        palid = 0;

    player->palette = palid;

    if (player != g_player[screenpeek].ps)
        return;

    setbrightness(ud.brightness>>2, palid, set);
}

void G_HandleSpecialKeys(void)
{
    // we need CONTROL_GetInput in order to pick up joystick button presses
    if (CONTROL_Started && !(g_player[myconnectindex].ps->gm & MODE_GAME))
    {
        ControlInfo noshareinfo;
        CONTROL_GetInput(&noshareinfo);
    }

//    CONTROL_ProcessBinds();

    if (g_networkMode != NET_DEDICATED_SERVER && ALT_IS_PRESSED && KB_KeyPressed(sc_Enter))
    {
        if (setgamemode(!ud.config.ScreenMode,ud.config.ScreenWidth,ud.config.ScreenHeight,ud.config.ScreenBPP))
        {
            OSD_Printf(OSD_ERROR "Failed setting fullscreen video mode.\n");
            if (setgamemode(ud.config.ScreenMode, ud.config.ScreenWidth, ud.config.ScreenHeight, ud.config.ScreenBPP))
                G_GameExit("Failed to recover from failure to set fullscreen video mode.\n");
        }
        else ud.config.ScreenMode = !ud.config.ScreenMode;
        KB_ClearKeyDown(sc_Enter);
        g_restorePalette = 1;
        G_UpdateScreenArea();
    }

    if (KB_UnBoundKeyPressed(sc_F12))
    {
        char titlebuf[256];
        Bsprintf(titlebuf,HEAD2 " %s",s_buildRev);

        KB_ClearKeyDown(sc_F12);
        screencapture("duke0000.tga",0,titlebuf);
        P_DoQuote(QUOTE_SCREEN_SAVED,g_player[myconnectindex].ps);
    }

    // only dispatch commands here when not in a game
    if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
        OSD_DispatchQueued();

    if (g_quickExit == 0 && KB_KeyPressed(sc_LeftControl) && KB_KeyPressed(sc_LeftAlt) && (KB_KeyPressed(sc_Delete)||KB_KeyPressed(sc_End)))
    {
        g_quickExit = 1;
        G_GameExit("Quick Exit.");
    }
}

void G_GameQuit(void)
{
    if (numplayers < 2)
        G_GameExit(" ");

    if (g_gameQuit == 0)
    {
        g_gameQuit = 1;
        g_quitDeadline = totalclock+120;
        g_netDisconnect = 1;
    }

    if ((totalclock > g_quitDeadline) && (g_gameQuit == 1))
        G_GameExit("Timed out.");
}

#if !defined DEBUG_ALLOCACHE_AS_MALLOC
extern int32_t cacnum;
extern cactype cac[];
#endif

#define COLOR_RED redcol
#define COLOR_WHITE whitecol

static void G_ShowCacheLocks(void)
{
    int16_t i,k;

    if (offscreenrendering)
        return;

    k = 0;
#if !defined DEBUG_ALLOCACHE_AS_MALLOC
    for (i=cacnum-1; i>=0; i--)
        if ((*cac[i].lock) >= 200)
        {
            if (k >= ydim-12)
                break;

            Bsprintf(tempbuf,"Locked- %d: Leng:%d, Lock:%d",i,cac[i].leng,*cac[i].lock);
            printext256(0L,k,COLOR_WHITE,-1,tempbuf,1);
            k += 6;
        }
#endif
    if (k < ydim-12)
        k += 6;

    for (i=10; i>=0; i--)
        if (rts_lumplockbyte[i] >= 200)
        {
            if (k >= ydim-12)
                break;

            Bsprintf(tempbuf,"RTS Locked %d:",i);
            printext256(0,k,COLOR_WHITE,-1,tempbuf,1);
            k += 6;
        }

    if (k >= ydim-12 && k<ydim-6)
        printext256(0,k,COLOR_WHITE,-1,"(MORE . . .)",1);

    // sounds
    if (xdim < 640)
        return;

    k = 18;
    for (i=0; i<=g_maxSoundPos; i++)
        if (g_sounds[i].num > 0)
        {
            int32_t j, n=g_sounds[i].num;

            for (j=0; j<n; j++)
            {
                if (k >= ydim-12)
                    break;

                Bsprintf(tempbuf, "snd #%d inst %d: voice %d, ow %d", i, j,
                         g_sounds[i].SoundOwner[j].voice, g_sounds[i].SoundOwner[j].ow);
                printext256(240,k,COLOR_WHITE,-1,tempbuf,0);

                k += 9;
            }
        }
}

int32_t A_CheckInventorySprite(spritetype *s)
{
    switch (DYNAMICTILEMAP(s->picnum))
    {
    case FIRSTAID__STATIC:
    case STEROIDS__STATIC:
    case HEATSENSOR__STATIC:
    case BOOTS__STATIC:
    case JETPACK__STATIC:
    case HOLODUKE__STATIC:
    case AIRTANK__STATIC:
        return 1;
    default:
        return 0;
    }
}

// MYOS* CON commands.
LUNATIC_EXTERN void G_DrawTileGeneric(int32_t x, int32_t y, int32_t zoom, int32_t tilenum,
                                      int32_t shade, int32_t orientation, int32_t p)
{
    int32_t a = 0;

    orientation &= (ROTATESPRITE_MAX-1);

    if (orientation&4)
        a = 1024;

    if (!(orientation&ROTATESPRITE_FULL16))
    {
        x<<=16;
        y<<=16;
    }

    rotatesprite_win(x,y,zoom,a,tilenum,shade,p,2|orientation);
}

#if !defined LUNATIC
void G_DrawTile(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation)
{
    DukePlayer_t *ps = g_player[screenpeek].ps;
    int32_t p = ps->cursectnum >= 0 ? sector[ps->cursectnum].floorpal : 0;

    G_DrawTileGeneric(x,y,65536, tilenum,shade,orientation, p);
}

void G_DrawTilePal(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation, int32_t p)
{
    G_DrawTileGeneric(x,y,65536, tilenum,shade,orientation, p);
}

void G_DrawTileSmall(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation)
{
    DukePlayer_t *ps = g_player[screenpeek].ps;
    int32_t p = ps->cursectnum >= 0 ? sector[ps->cursectnum].floorpal : 0;

    G_DrawTileGeneric(x,y,32768, tilenum,shade,orientation, p);
}

void G_DrawTilePalSmall(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation, int32_t p)
{
    G_DrawTileGeneric(x,y,32768, tilenum,shade,orientation, p);
}
#endif

#define LOW_FPS 30

#if defined GEKKO
# define FPS_YOFFSET 16
#else
# define FPS_YOFFSET 0
#endif

static void G_PrintFPS(void)
{
    // adapted from ZDoom because I like it better than what we had
    // applicable ZDoom code available under GPL from csDoom
    static int32_t FrameCount = 0, LastCount = 0, LastSec = 0, LastMS = 0;
    static int32_t MinFrames = INT32_MAX, MaxFrames = 0;

    int32_t ms = getticks();
    int32_t howlong = ms - LastMS;

    if (g_player[0].ps->player_par < 2)
    {
        MinFrames = INT32_MAX;
        MaxFrames = 0;
    }

    if (howlong >= 0)
    {
        int32_t thisSec = ms/1000;
        int32_t x = (xdim <= 640);

        if (ud.tickrate)
        {
            int32_t chars = Bsprintf(tempbuf, "%d ms (%3u fps)", howlong, LastCount);

            printext256(windowx2-(chars<<(3-x))+1,windowy1+2+FPS_YOFFSET,0,-1,tempbuf,x);
            printext256(windowx2-(chars<<(3-x)),windowy1+1+FPS_YOFFSET,
                        (LastCount < LOW_FPS) ? COLOR_RED : COLOR_WHITE,-1,tempbuf,x);

            if (ud.tickrate > 1)
            {
                chars = Bsprintf(tempbuf, "max fps: %3u", MaxFrames);

                printext256(windowx2-(chars<<(3-x))+1, windowy1+10+2+FPS_YOFFSET, 0, -1, tempbuf, x);
                printext256(windowx2-(chars<<(3-x)), windowy1+10+FPS_YOFFSET,
                    (MaxFrames < LOW_FPS) ? COLOR_RED : COLOR_WHITE, -1, tempbuf, x);

                chars = Bsprintf(tempbuf, "min fps: %3u", MinFrames);

                printext256(windowx2-(chars<<(3-x))+1, windowy1+20+2+FPS_YOFFSET, 0, -1, tempbuf, x);
                printext256(windowx2-(chars<<(3-x)), windowy1+20+FPS_YOFFSET,
                    (MinFrames < LOW_FPS) ? COLOR_RED : COLOR_WHITE, -1, tempbuf, x);
            }

            // lag meter
            if (g_netClientPeer)
            {
                chars = Bsprintf(tempbuf, "%d +- %d ms", (g_netClientPeer->lastRoundTripTime + g_netClientPeer->roundTripTime)/2,
                                 (g_netClientPeer->lastRoundTripTimeVariance + g_netClientPeer->roundTripTimeVariance)/2);

                printext256(windowx2-(chars<<(3-x))+1,windowy1+30+2+FPS_YOFFSET,0,-1,tempbuf,x);
                printext256(windowx2-(chars<<(3-x)),windowy1+30+1+FPS_YOFFSET,g_netClientPeer->lastRoundTripTime > 200 ? COLOR_RED : COLOR_WHITE,-1,tempbuf,x);
            }
        }

        if (thisSec - LastSec)
        {
            g_currentFrameRate = LastCount = tabledivide32_noinline(FrameCount, thisSec - LastSec);
            LastSec = thisSec;
            FrameCount = 0;

            if (!osdshown)
            {
                if (LastCount > MaxFrames) MaxFrames = LastCount;
                if (LastCount < MinFrames) MinFrames = LastCount;
            }
        }
        FrameCount++;
    }
    LastMS = ms;
}

// yxaspect and viewingrange just before the 'main' drawrooms call
static int32_t dr_yxaspect, dr_viewingrange;

#ifdef DEBUGGINGAIDS
static struct {
    uint32_t lastgtic;
    uint32_t lastnumins, numins;
    int32_t numonscreen;
} g_spriteStat;
#endif

#define printcoordsline(fmt, ...) do { \
    Bsprintf(tempbuf, fmt, ## __VA_ARGS__); \
    printext256(20, y+=9, COLOR_WHITE, -1, tempbuf, 0); \
} while (0)

static void G_PrintCoords(int32_t snum)
{
    const int32_t x = 250;
    int32_t y = 16;

    const DukePlayer_t *ps = g_player[snum].ps;
    const int32_t sectnum = ps->cursectnum;

    if ((GametypeFlags[ud.coop] & GAMETYPE_FRAGBAR))
    {
        if (ud.multimode > 4)
            y = 32;
        else if (g_netServer || ud.multimode > 1)
            y = 24;
    }
    Bsprintf(tempbuf, "XYZ= (%d, %d, %d)", ps->pos.x, ps->pos.y, ps->pos.z);
    printext256(x,y,COLOR_WHITE,-1,tempbuf,0);
    Bsprintf(tempbuf, "A/H/HO= %d, %d, %d", ps->ang, ps->horiz, ps->horizoff);
    printext256(x,y+9,COLOR_WHITE,-1,tempbuf,0);
    Bsprintf(tempbuf,"VEL= (%d, %d, %d) + (%d, %d, 0)",
             ps->vel.x>>14, ps->vel.y>>14, ps->vel.z, ps->fric.x>>5, ps->fric.y>>5);
    printext256(x,y+18,COLOR_WHITE,-1,tempbuf,0);
    Bsprintf(tempbuf,"OG= %d  SBRIDGE=%d SBS=%d", ps->on_ground, ps->spritebridge, ps->sbs);
    printext256(x,y+27,COLOR_WHITE,-1,tempbuf,0);
    if (sectnum >= 0)
        Bsprintf(tempbuf, "SECT= %d (LO=%d EX=%d)", sectnum,
                 TrackerCast(sector[sectnum].lotag), TrackerCast(sector[sectnum].extra));
    else
        Bsprintf(tempbuf,"SECT= %d", sectnum);
    printext256(x,y+36,COLOR_WHITE,-1,tempbuf,0);
//    Bsprintf(tempbuf,"SEED= %d",randomseed);
//    printext256(x,y+45,COLOR_WHITE,-1,tempbuf,0);
    y -= 9;

    y += 7;
    Bsprintf(tempbuf,"THOLD= %d", ps->transporter_hold);
    printext256(x,y+54,COLOR_WHITE,-1,tempbuf,0);
    Bsprintf(tempbuf,"GAMETIC= %d, TOTALCLOCK=%d", g_moveThingsCount, totalclock);
    printext256(x,y+63,COLOR_WHITE,-1,tempbuf,0);
#ifdef DEBUGGINGAIDS
    Bsprintf(tempbuf,"NUMSPRITES= %d", Numsprites);
    printext256(x,y+72,COLOR_WHITE,-1,tempbuf,0);
    if (g_moveThingsCount > g_spriteStat.lastgtic + REALGAMETICSPERSEC)
    {
        g_spriteStat.lastgtic = g_moveThingsCount;
        g_spriteStat.lastnumins = g_spriteStat.numins;
        g_spriteStat.numins = 0;
    }
    Bsprintf(tempbuf,"INSERTIONS/s= %u", g_spriteStat.lastnumins);
    printext256(x,y+81,COLOR_WHITE,-1,tempbuf,0);
    Bsprintf(tempbuf,"ONSCREEN= %d", g_spriteStat.numonscreen);
    printext256(x,y+90,COLOR_WHITE,-1,tempbuf,0);
    y += 3*9;
#endif
    y += 7;
    Bsprintf(tempbuf,"VR=%.03f  YX=%.03f",(double)dr_viewingrange/65536.0,(double)dr_yxaspect/65536.0);
    printext256(x,y+72,COLOR_WHITE,-1,tempbuf,0);
    Bsprintf(tempbuf, "MOVEACTORS [ms]= %.3e", g_moveActorsTime);
    printext256(x,y+81,COLOR_WHITE,-1,tempbuf,0);

#ifdef USE_OPENGL
    if (ud.coords == 2)
    {
        y=16;

        printcoordsline("rendmode = %d", getrendermode());
        printcoordsline("r_ambientlight = %.03f", r_ambientlight);

        if (rendmode >= 3)
        {
            if (rendmode==3)
                printcoordsline("r_usenewshading = %d", r_usenewshading);
# ifdef POLYMER
            else
                printcoordsline("r_pr_artmapping = %d", pr_artmapping);
#endif
            printcoordsline("r_usetileshades = %d", r_usetileshades);
        }
    }
#endif
}




////////// OFTEN-USED FEW-LINERS //////////
static void G_HandleEventsWhileNoInput(void)
{
    I_ClearAllInput();

    while (!I_CheckAllInput())
        G_HandleAsync();

    I_ClearAllInput();
}

static int32_t G_PlaySoundWhileNoInput(int32_t soundnum)
{
    S_PlaySound(soundnum);
    I_ClearAllInput();
    while (S_CheckSoundPlaying(-1, soundnum))
    {
        G_HandleAsync();
        if (I_CheckAllInput())
        {
            I_ClearAllInput();
            return 1;
        }
    }

    return 0;
}
//////////

void G_FadePalette(int32_t r,int32_t g,int32_t b,int32_t e)
{
    int32_t tc;
    setpalettefade(r,g,b,e);

    nextpage();
    tc = totalclock;
    while (totalclock < tc + 4)
        G_HandleAsync();
}

// START and END limits are always inclusive!
// STEP must evenly divide END-START, i.e. abs(end-start)%step == 0
void fadepal(int32_t r, int32_t g, int32_t b, int32_t start, int32_t end, int32_t step)
{
    if (getrendermode() >= REND_POLYMOST)
    {
        G_FadePalette(r, g, b, end);
        return;
    }

    // (end-start)/step + 1 iterations
    do
    {
        if (KB_KeyPressed(sc_Space))
        {
            KB_ClearKeyDown(sc_Space);
            setpalettefade(r,g,b,end);  // have to set to end fade value if we break!
            return;
        }

        G_FadePalette(r,g,b,start);

        start += step;
    }
    while (start != end+step);
}

// START and END limits are always inclusive!
static void fadepaltile(int32_t r, int32_t g, int32_t b, int32_t start, int32_t end, int32_t step, int32_t tile)
{
    // STEP must evenly divide END-START
    Bassert(klabs(end-start)%step == 0);

    clearallviews(0);

    // (end-start)/step + 1 iterations
    do
    {
#ifdef __ANDROID__ //Needed for N7 2013 to stop corruption while fading video
    	clearview(0);
#endif
        if (KB_KeyPressed(sc_Space))
        {
            KB_ClearKeyDown(sc_Space);
            setpalettefade(r,g,b,end);  // have to set to end fade value if we break!
            return;
        }
        rotatesprite_fs(160<<16,100<<16,65536L,0,tile,0,0,2+8+BGSTRETCH);
        G_FadePalette(r,g,b,start);

        start += step;
    }
    while (start != end+step);
}

#ifdef LUNATIC
int32_t g_logoFlags = 255;
#endif

#ifdef __ANDROID__
int inExtraScreens = 0;
#endif

static void G_DisplayExtraScreens(void)
{
    int32_t flags = G_GetLogoFlags();

    S_StopMusic();
    FX_StopAllSounds();

    if (!DUKEBETA && (!VOLUMEALL || flags & LOGO_SHAREWARESCREENS))
    {
#ifdef __ANDROID__
        inExtraScreens = 1;
#endif
        setview(0,0,xdim-1,ydim-1);
        flushperms();
        //g_player[myconnectindex].ps->palette = palette;
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 1);    // JBF 20040308
        fadepal(0,0,0, 0,252,28);
        I_ClearAllInput();
        rotatesprite_fs(160<<16,100<<16,65536L,0,3291,0,0,2+8+64+BGSTRETCH);
        fadepaltile(0,0,0, 252,0,-28, 3291);
        while (!I_CheckAllInput())
            G_HandleAsync();

        fadepaltile(0,0,0, 0,252,28, 3291);
        I_ClearAllInput();
        rotatesprite_fs(160<<16,100<<16,65536L,0,3290,0,0,2+8+64+BGSTRETCH);
        fadepaltile(0,0,0, 252,0,-28,3290);
        while (!I_CheckAllInput())
            G_HandleAsync();

#ifdef __ANDROID__
        inExtraScreens = 0;
#endif
    }

    if (flags & LOGO_TENSCREEN)
    {
#ifdef __ANDROID__
        inExtraScreens = 1;
#endif
        setview(0,0,xdim-1,ydim-1);
        flushperms();
        //g_player[myconnectindex].ps->palette = palette;
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 1);    // JBF 20040308
        fadepal(0,0,0, 0,252,28);
        I_ClearAllInput();
        totalclock = 0;
        rotatesprite_fs(160<<16,100<<16,65536L,0,TENSCREEN,0,0,2+8+64+BGSTRETCH);
        fadepaltile(0,0,0, 252,0,-28,TENSCREEN);
        while (!I_CheckAllInput() && totalclock < 2400)
            G_HandleAsync();

        fadepaltile(0,0,0, 0,252,28, TENSCREEN);
        I_ClearAllInput();
#ifdef __ANDROID__
        inExtraScreens = 0;
#endif
    }
}


extern int32_t g_doQuickSave;

void G_GameExit(const char *msg)
{
#ifdef LUNATIC
    El_PrintTimes();
    El_DestroyState(&g_ElState);
#endif
    if (*msg != 0) g_player[myconnectindex].ps->palette = BASEPAL;

    if (ud.recstat == 1)
        G_CloseDemoWrite();
    else if (ud.recstat == 2)
        MAYBE_FCLOSE_AND_NULL(g_demo_filePtr);
    // JBF: fixes crash on demo playback
    // PK: modified from original

    if (!g_quickExit)
    {
        if (playerswhenstarted > 1 && g_player[myconnectindex].ps->gm&MODE_GAME && GTFLAGS(GAMETYPE_SCORESHEET) && *msg == ' ')
        {
            G_BonusScreen(1);
            setgamemode(ud.config.ScreenMode,ud.config.ScreenWidth,ud.config.ScreenHeight,ud.config.ScreenBPP);
        }

        if (*msg != 0 && *(msg+1) != 'V' && *(msg+1) != 'Y')
            G_DisplayExtraScreens();
    }

    if (*msg != 0) initprintf("%s\n",msg);

    if (in3dmode())
        G_Shutdown();

    if (*msg != 0)
    {
        if (!(msg[0] == ' ' && msg[1] == 0))
        {
            char titlebuf[256];
            Bsprintf(titlebuf,HEAD2 " %s",s_buildRev);
            wm_msgbox(titlebuf, "%s", msg);
        }
    }

    Bfflush(NULL);

    exit(0);
}


static inline void G_MoveClouds(void)
{
    int32_t i;

    if (totalclock <= cloudtotalclock && totalclock >= (cloudtotalclock-7))
        return;

    cloudtotalclock = totalclock+6;

    cloudx += sintable[(g_player[screenpeek].ps->ang+512)&2047]>>9;
    cloudy += sintable[g_player[screenpeek].ps->ang&2047]>>9;

    for (i=g_numClouds-1; i>=0; i--)
    {
        sector[clouds[i]].ceilingxpanning = cloudx>>6;
        sector[clouds[i]].ceilingypanning = cloudy>>6;
    }
}

static void G_DrawOverheadMap(int32_t cposx, int32_t cposy, int32_t czoom, int16_t cang)
{
    int32_t i, j, k, l, x1, y1, x2=0, y2=0, x3, y3, x4, y4, ox, oy, xoff, yoff;
    int32_t dax, day, cosang, sinang, xspan, yspan, sprx, spry;
    int32_t xrepeat, yrepeat, z1, z2, startwall, endwall, tilenum, daang;
    int32_t xvect, yvect, xvect2, yvect2;
    int16_t p;
    char col;
    walltype *wal, *wal2;
    spritetype *spr;

    int32_t tmpydim = (xdim*5)/8;

    setaspect(65536, divscale16(tmpydim*320, xdim*200));

    xvect = sintable[(-cang)&2047] * czoom;
    yvect = sintable[(1536-cang)&2047] * czoom;
    xvect2 = mulscale16(xvect,yxaspect);
    yvect2 = mulscale16(yvect,yxaspect);

    push_nofog();

    //Draw red lines
    for (i=numsectors-1; i>=0; i--)
    {
        if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;

        startwall = sector[i].wallptr;
        endwall = sector[i].wallptr + sector[i].wallnum;

        z1 = sector[i].ceilingz;
        z2 = sector[i].floorz;

        for (j=startwall,wal=&wall[startwall]; j<endwall; j++,wal++)
        {
            k = wal->nextwall;
            if (k < 0) continue;

            if (sector[wal->nextsector].ceilingz == z1)
                if (sector[wal->nextsector].floorz == z2)
                    if (((wal->cstat|wall[wal->nextwall].cstat)&(16+32)) == 0) continue;

            col = 139; //red
            if ((wal->cstat|wall[wal->nextwall].cstat)&1) col = 234; //magenta

            if (!(show2dsector[wal->nextsector>>3]&(1<<(wal->nextsector&7))))
                col = 24;
            else continue;

            ox = wal->x-cposx;
            oy = wal->y-cposy;
            x1 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
            y1 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

            wal2 = &wall[wal->point2];
            ox = wal2->x-cposx;
            oy = wal2->y-cposy;
            x2 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
            y2 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

            drawline256(x1,y1,x2,y2,col);
        }
    }

    pop_nofog();

    //Draw sprites
    k = g_player[screenpeek].ps->i;
    for (i=numsectors-1; i>=0; i--)
    {
        if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;
        for (j=headspritesect[i]; j>=0; j=nextspritesect[j])
        {
            spr = &sprite[j];

            if (j == k || (spr->cstat&0x8000) || spr->cstat == 257 || spr->xrepeat == 0) continue;

            col = 71; //cyan
            if (spr->cstat&1) col = 234; //magenta

            sprx = spr->x;
            spry = spr->y;

            if ((spr->cstat&257) != 0) switch (spr->cstat&48)
                {
                case 0:
//                    break;

                    ox = sprx-cposx;
                    oy = spry-cposy;
                    x1 = dmulscale16(ox,xvect,-oy,yvect);
                    y1 = dmulscale16(oy,xvect2,ox,yvect2);

                    ox = (sintable[(spr->ang+512)&2047]>>7);
                    oy = (sintable[(spr->ang)&2047]>>7);
                    x2 = dmulscale16(ox,xvect,-oy,yvect);
                    y2 = dmulscale16(oy,xvect,ox,yvect);

                    x3 = mulscale16(x2,yxaspect);
                    y3 = mulscale16(y2,yxaspect);

                    drawline256(x1-x2+(xdim<<11),y1-y3+(ydim<<11),
                                x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                    drawline256(x1-y2+(xdim<<11),y1+x3+(ydim<<11),
                                x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                    drawline256(x1+y2+(xdim<<11),y1-x3+(ydim<<11),
                                x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                    break;

                case 16:
                    if (spr->picnum == LASERLINE)
                    {
                        x1 = sprx;
                        y1 = spry;
                        tilenum = spr->picnum;
                        xoff = picanm[tilenum].xofs + spr->xoffset;
                        if ((spr->cstat&4) > 0) xoff = -xoff;
                        k = spr->ang;
                        l = spr->xrepeat;
                        dax = sintable[k&2047]*l;
                        day = sintable[(k+1536)&2047]*l;
                        l = tilesiz[tilenum].x;
                        k = (l>>1)+xoff;
                        x1 -= mulscale16(dax,k);
                        x2 = x1+mulscale16(dax,l);
                        y1 -= mulscale16(day,k);
                        y2 = y1+mulscale16(day,l);

                        ox = x1-cposx;
                        oy = y1-cposy;
                        x1 = dmulscale16(ox,xvect,-oy,yvect);
                        y1 = dmulscale16(oy,xvect2,ox,yvect2);

                        ox = x2-cposx;
                        oy = y2-cposy;
                        x2 = dmulscale16(ox,xvect,-oy,yvect);
                        y2 = dmulscale16(oy,xvect2,ox,yvect2);

                        drawline256(x1+(xdim<<11),y1+(ydim<<11),
                                    x2+(xdim<<11),y2+(ydim<<11),col);
                    }

                    break;

                case 32:
                    tilenum = spr->picnum;
                    xoff = picanm[tilenum].xofs + spr->xoffset;
                    yoff = picanm[tilenum].yofs + spr->yoffset;
                    if ((spr->cstat&4) > 0) xoff = -xoff;
                    if ((spr->cstat&8) > 0) yoff = -yoff;

                    k = spr->ang;
                    cosang = sintable[(k+512)&2047];
                    sinang = sintable[k&2047];
                    xspan = tilesiz[tilenum].x;
                    xrepeat = spr->xrepeat;
                    yspan = tilesiz[tilenum].y;
                    yrepeat = spr->yrepeat;

                    dax = ((xspan>>1)+xoff)*xrepeat;
                    day = ((yspan>>1)+yoff)*yrepeat;
                    x1 = sprx + dmulscale16(sinang,dax,cosang,day);
                    y1 = spry + dmulscale16(sinang,day,-cosang,dax);
                    l = xspan*xrepeat;
                    x2 = x1 - mulscale16(sinang,l);
                    y2 = y1 + mulscale16(cosang,l);
                    l = yspan*yrepeat;
                    k = -mulscale16(cosang,l);
                    x3 = x2+k;
                    x4 = x1+k;
                    k = -mulscale16(sinang,l);
                    y3 = y2+k;
                    y4 = y1+k;

                    ox = x1-cposx;
                    oy = y1-cposy;
                    x1 = dmulscale16(ox,xvect,-oy,yvect);
                    y1 = dmulscale16(oy,xvect2,ox,yvect2);

                    ox = x2-cposx;
                    oy = y2-cposy;
                    x2 = dmulscale16(ox,xvect,-oy,yvect);
                    y2 = dmulscale16(oy,xvect2,ox,yvect2);

                    ox = x3-cposx;
                    oy = y3-cposy;
                    x3 = dmulscale16(ox,xvect,-oy,yvect);
                    y3 = dmulscale16(oy,xvect2,ox,yvect2);

                    ox = x4-cposx;
                    oy = y4-cposy;
                    x4 = dmulscale16(ox,xvect,-oy,yvect);
                    y4 = dmulscale16(oy,xvect2,ox,yvect2);

                    drawline256(x1+(xdim<<11),y1+(ydim<<11),
                                x2+(xdim<<11),y2+(ydim<<11),col);

                    drawline256(x2+(xdim<<11),y2+(ydim<<11),
                                x3+(xdim<<11),y3+(ydim<<11),col);

                    drawline256(x3+(xdim<<11),y3+(ydim<<11),
                                x4+(xdim<<11),y4+(ydim<<11),col);

                    drawline256(x4+(xdim<<11),y4+(ydim<<11),
                                x1+(xdim<<11),y1+(ydim<<11),col);

                    break;
                }
        }
    }

    push_nofog();

    //Draw white lines
    for (i=numsectors-1; i>=0; i--)
    {
        if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;

        startwall = sector[i].wallptr;
        endwall = sector[i].wallptr + sector[i].wallnum;

        k = -1;
        for (j=startwall,wal=&wall[startwall]; j<endwall; j++,wal++)
        {
            if (wal->nextwall >= 0) continue;

            if (tilesiz[wal->picnum].x == 0) continue;
            if (tilesiz[wal->picnum].y == 0) continue;

            if (j == k)
            {
                x1 = x2;
                y1 = y2;
            }
            else
            {
                ox = wal->x-cposx;
                oy = wal->y-cposy;
                x1 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
                y1 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);
            }

            k = wal->point2;
            wal2 = &wall[k];
            ox = wal2->x-cposx;
            oy = wal2->y-cposy;
            x2 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
            y2 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

            drawline256(x1,y1,x2,y2,24);
        }
    }

    pop_nofog();

    setaspect_new();

    for (TRAVERSE_CONNECT(p))
    {
        if (ud.scrollmode && p == screenpeek) continue;

        ox = sprite[g_player[p].ps->i].x-cposx;
        oy = sprite[g_player[p].ps->i].y-cposy;
        daang = (sprite[g_player[p].ps->i].ang-cang)&2047;
        if (p == screenpeek)
        {
            ox = 0;
            oy = 0;
            daang = 0;
        }
        x1 = mulscale(ox,xvect,16) - mulscale(oy,yvect,16);
        y1 = mulscale(oy,xvect2,16) + mulscale(ox,yvect2,16);

        if (p == screenpeek || GTFLAGS(GAMETYPE_OTHERPLAYERSINMAP))
        {
            if (sprite[g_player[p].ps->i].xvel > 16 && g_player[p].ps->on_ground)
                i = APLAYERTOP+((totalclock>>4)&3);
            else
                i = APLAYERTOP;

            j = klabs(g_player[p].ps->truefz-g_player[p].ps->pos.z)>>8;
            j = mulscale(czoom*(sprite[g_player[p].ps->i].yrepeat+j),yxaspect,16);

            if (j < 22000) j = 22000;
            else if (j > (65536<<1)) j = (65536<<1);

            rotatesprite_win((x1<<4)+(xdim<<15),(y1<<4)+(ydim<<15),j,daang,i,sprite[g_player[p].ps->i].shade,
                             (g_player[p].ps->cursectnum > -1)?sector[g_player[p].ps->cursectnum].floorpal:0, 0);
        }
    }
}


palette_t CrosshairColors = { 255, 255, 255, 0 };
palette_t DefaultCrosshairColors = { 0, 0, 0, 0 };
int32_t g_crosshairSum = 0;

void G_GetCrosshairColor(void)
{
    // use the brightest color in the original 8-bit tile
    int32_t bri = 0, j = 0, i;
    int32_t ii;
    char *ptr = (char *)waloff[CROSSHAIR];

    if (DefaultCrosshairColors.f)
        return;

    if (waloff[CROSSHAIR] == 0)
    {
        loadtile(CROSSHAIR);
        ptr = (char *)waloff[CROSSHAIR];
    }

    ii = tilesiz[CROSSHAIR].x * tilesiz[CROSSHAIR].y;

    if (ii <= 0) return;

    do
    {
        if (*ptr != 255)
        {
            i = curpalette[(int32_t)*ptr].r+curpalette[(int32_t)*ptr].g+curpalette[(int32_t)*ptr].b;
            if (i > j) { j = i; bri = *ptr; }
        }
        ptr++;
    }
    while (--ii);

    Bmemcpy(&CrosshairColors, &curpalette[bri], sizeof(palette_t));
    Bmemcpy(&DefaultCrosshairColors, &curpalette[bri], sizeof(palette_t));
    DefaultCrosshairColors.f = 1; // this flag signifies that the color has been detected
}

void G_SetCrosshairColor(int32_t r, int32_t g, int32_t b)
{
    int32_t i, ii;

    if (g_crosshairSum == r+(g<<8)+(b<<16)) return;

    if (!DefaultCrosshairColors.f)
        G_GetCrosshairColor();

    g_crosshairSum = r+(g<<8)+(b<<16);
    CrosshairColors.r = r;
    CrosshairColors.g = g;
    CrosshairColors.b = b;

    char *ptr = (char *) waloff[CROSSHAIR];

    if (waloff[CROSSHAIR] == 0)
    {
        loadtile(CROSSHAIR);
        ptr = (char *)waloff[CROSSHAIR];
    }

    ii = tilesiz[CROSSHAIR].x * tilesiz[CROSSHAIR].y;
    if (ii <= 0) return;

    if (getrendermode() == REND_CLASSIC)
        i = getclosestcol(CrosshairColors.r, CrosshairColors.g, CrosshairColors.b);
    else i = getclosestcol(255, 255, 255); // use white in GL so we can tint it to the right color

    do
    {
        if (*ptr != 255)
            *ptr = i;
        ptr++;
    }
    while (--ii);

    makepalookup(CROSSHAIR_PAL, NULL, CrosshairColors.r, CrosshairColors.g, CrosshairColors.b,1);

#ifdef USE_OPENGL
    // XXX: this makes us also load all hightile textures tinted with the crosshair color!
    Bmemcpy(&hictinting[CROSSHAIR_PAL], &CrosshairColors, sizeof(palette_t));
    hictinting[CROSSHAIR_PAL].f = HICTINT_USEONART | HICTINT_GRAYSCALE;
#endif
    invalidatetile(CROSSHAIR, -1, -1);
}

static inline int G_LastMapInfoIndex(void)
{
    Bassert(ud.last_level >= 1);  // NOTE: last_level is 1-based
    return ud.volume_number*MAXLEVELS + ud.last_level-1;
}

#define SCORESHEETOFFSET -20
static void G_ShowScores(void)
{
    int32_t t, i;

    if (playerswhenstarted > 1 && (GametypeFlags[ud.coop]&GAMETYPE_SCORESHEET))
    {
        gametext(160,SCORESHEETOFFSET+58+2,"Multiplayer Totals",0,2+8+16);
        gametext(160,SCORESHEETOFFSET+58+10,MapInfo[G_LastMapInfoIndex()].name,0,2+8+16);

        t = 0;
        minitext(70,SCORESHEETOFFSET+80,"Name",8,2+8+16+ROTATESPRITE_MAX);
        minitext(170,SCORESHEETOFFSET+80,"Frags",8,2+8+16+ROTATESPRITE_MAX);
        minitext(200,SCORESHEETOFFSET+80,"Deaths",8,2+8+16+ROTATESPRITE_MAX);
        minitext(235,SCORESHEETOFFSET+80,"Ping",8,2+8+16+ROTATESPRITE_MAX);

        for (i=playerswhenstarted-1; i>=0; i--)
        {
            if (!g_player[i].playerquitflag)
                continue;

            minitext(70,SCORESHEETOFFSET+90+t,g_player[i].user_name,g_player[i].ps->palookup,2+8+16+ROTATESPRITE_MAX);

            Bsprintf(tempbuf,"%-4d",g_player[i].ps->frag);
            minitext(170,SCORESHEETOFFSET+90+t,tempbuf,2,2+8+16+ROTATESPRITE_MAX);

            Bsprintf(tempbuf,"%-4d", g_player[i].frags[i] + g_player[i].ps->fraggedself);
            minitext(200,SCORESHEETOFFSET+90+t,tempbuf,2,2+8+16+ROTATESPRITE_MAX);

            Bsprintf(tempbuf,"%-4d",g_player[i].ping);
            minitext(235,SCORESHEETOFFSET+90+t,tempbuf,2,2+8+16+ROTATESPRITE_MAX);

            t += 7;
        }
    }
}
#undef SCORESHEETOFFSET

#ifdef YAX_DEBUG
// ugh...
char m32_debugstr[64][128];
int32_t m32_numdebuglines=0;

static void M32_drawdebug(void)
{
    int i, col=getclosestcol(255,255,255);
    int x=4, y=8;

    if (m32_numdebuglines>0)
    {
        begindrawing();
        for (i=0; i<m32_numdebuglines && y<ydim-8; i++, y+=8)
            printext256(x,y,col,0,m32_debugstr[i],xdim>640?0:1);
        enddrawing();
    }
    m32_numdebuglines=0;
}
#endif


////////// TINT ACCUMULATOR //////////

typedef struct {
    int32_t r,g,b;
    // f: 0-63 scale
    int32_t maxf, sumf;
} palaccum_t;

#define PALACCUM_INITIALIZER { 0, 0, 0, 0, 0 }

/* For a picture frame F and n tints C_1, C_2, ... C_n weighted a_1, a_2,
 * ... a_n (on a 0-1 scale), the faded frame is calculated as
 *
 *    F_new := (1-max_i(a_i))*F + d*sum_i(a_i), where
 *
 *    d := max_i(a_i)/sum_i(a_i).
 *
 * This means that
 *  1) tint application is independent of their order.
 *  2) going from n+1 to n tints is continuous when the leaving one has faded.
 *
 * But note that for more than one tint, the composite tint will in general
 * change its hue as the ratio of the weights of the individual ones changes.
 */
static void palaccum_add(palaccum_t *pa, const palette_t *pal, int32_t f)
{
    f = clamp(f, 0, 63);
    if (f == 0)
        return;

    pa->maxf = max(pa->maxf, f);
    pa->sumf += f;

    pa->r += f*clamp(pal->r, 0, 63);
    pa->g += f*clamp(pal->g, 0, 63);
    pa->b += f*clamp(pal->b, 0, 63);
}

static void G_FadePalaccum(const palaccum_t *pa)
{
    setpalettefade(tabledivide32_noinline(pa->r, pa->sumf)<<2,
                   tabledivide32_noinline(pa->g, pa->sumf)<<2,
                   tabledivide32_noinline(pa->b, pa->sumf)<<2, pa->maxf<<2);
}


static int32_t gtextsc(int32_t sc)
{
    return scale(sc, ud.textscale, 400);
}

////////// DISPLAYREST //////////

void G_DisplayRest(int32_t smoothratio)
{
    int32_t i, j;
    palaccum_t tint = PALACCUM_INITIALIZER;

    DukePlayer_t *const pp = g_player[screenpeek].ps;
#ifdef SPLITSCREEN_MOD_HACKS
    DukePlayer_t *const pp2 = g_fakeMultiMode==2 ? g_player[1].ps : NULL;
#endif
    int32_t cposx, cposy, cang;

#ifdef USE_OPENGL
    // this takes care of fullscreen tint for OpenGL
    if (getrendermode() >= REND_POLYMOST)
    {
        if (pp->palette == WATERPAL)
        {
            static const palette_t wp = { 224, 192, 255, 0 };
            Bmemcpy(&hictinting[MAXPALOOKUPS-1], &wp, sizeof(palette_t));
        }
        else if (pp->palette == SLIMEPAL)
        {
            static const palette_t sp = { 208, 255, 192, 0 };
            Bmemcpy(&hictinting[MAXPALOOKUPS-1], &sp, sizeof(palette_t));
        }
        else
        {
            hictinting[MAXPALOOKUPS-1].r = 255;
            hictinting[MAXPALOOKUPS-1].g = 255;
            hictinting[MAXPALOOKUPS-1].b = 255;
        }
    }
#endif  // USE_OPENGL

    palaccum_add(&tint, &pp->pals, pp->pals.f);
#ifdef SPLITSCREEN_MOD_HACKS
    if (pp2)
        palaccum_add(&tint, &pp2->pals, pp2->pals.f);
#endif
    {
        static const palette_t loogiepal = { 0, 63, 0, 0 };

        palaccum_add(&tint, &loogiepal, pp->loogcnt>>1);
#ifdef SPLITSCREEN_MOD_HACKS
        if (pp2)
            palaccum_add(&tint, &loogiepal, pp2->loogcnt>>1);
#endif
    }

    if (g_restorePalette)
    {
        // reset a normal palette
        static uint32_t omovethingscnt;

        if (g_restorePalette < 2 || omovethingscnt+1 == g_moveThingsCount)
        {
            int32_t pal = pp->palette;
#ifdef SPLITSCREEN_MOD_HACKS
            const int32_t opal = pal;

            if (pp2)  // splitscreen HACK: BASEPAL trumps all, then it's arbitrary.
                pal = min(pal, pp2->palette);
#endif

            // g_restorePalette < 0: reset tinting, too (e.g. when loading new game)
            P_SetGamePalette(pp, pal, 2 + (g_restorePalette>0)*16);

#ifdef SPLITSCREEN_MOD_HACKS
            if (pp2)  // keep first player's pal as its member!
                pp->palette = opal;
#endif

            g_restorePalette = 0;
        }
        else
        {
            // delay setting the palette by one game tic
            omovethingscnt = g_moveThingsCount;
        }
    }

    if (ud.show_help)
    {
        switch (ud.show_help)
        {
        case 1:
            rotatesprite_fs(160<<16,100<<16,65536L,0,TEXTSTORY,0,0,10+64);
            break;
        case 2:
            rotatesprite_fs(160<<16,100<<16,65536L,0,F1HELP,0,0,10+64);
            break;
        }

        if (I_ReturnTrigger())
        {
            I_ReturnTriggerClear();
            ud.show_help = 0;
            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }
            G_UpdateScreenArea();
        }

        return;
    }

    i = pp->cursectnum;
    if (i > -1)
    {
        const walltype *wal = &wall[sector[i].wallptr];

        show2dsector[i>>3] |= (1<<(i&7));
        for (j=sector[i].wallnum; j>0; j--,wal++)
        {
            i = wal->nextsector;
            if (i < 0) continue;
            if (wal->cstat&0x0071) continue;
            if (wall[wal->nextwall].cstat&0x0071) continue;
            if (sector[i].lotag == 32767) continue;
            if (sector[i].ceilingz >= sector[i].floorz) continue;
            show2dsector[i>>3] |= (1<<(i&7));
        }
    }

    if (ud.camerasprite == -1)
    {
        if (ud.overhead_on != 2)
        {
            if (pp->newowner >= 0)
                G_DrawCameraText(pp->newowner);
            else
            {
                P_DisplayWeapon();
#ifdef SPLITSCREEN_MOD_HACKS
                if (pp2)  // HACK
                {
                    const int32_t oscreenpeek = screenpeek;
                    screenpeek = 1;
                    P_DisplayWeapon();
                    screenpeek = oscreenpeek;
                }
#endif

                if (pp->over_shoulder_on == 0)
                    P_DisplayScuba();
#ifdef SPLITSCREEN_MOD_HACKS
                if (pp2 && pp2->over_shoulder_on == 0)  // HACK
                {
                    const int32_t oscreenpeek = screenpeek;
                    screenpeek = 1;
                    P_DisplayScuba();
                    screenpeek = oscreenpeek;
                }
#endif
            }
            G_MoveClouds();
        }

        if (ud.overhead_on > 0)
        {
            // smoothratio = min(max(smoothratio,0),65536);
            smoothratio = calc_smoothratio(totalclock, ototalclock);
            G_DoInterpolations(smoothratio);

            if (ud.scrollmode == 0)
            {
                if (pp->newowner == -1 && !ud.pause_on)
                {
                    cposx = pp->opos.x + mulscale16(pp->pos.x-pp->opos.x, smoothratio);
                    cposy = pp->opos.y + mulscale16(pp->pos.y-pp->opos.y, smoothratio);
                    cang = pp->oang + mulscale16(((pp->ang+1024-pp->oang)&2047)-1024, smoothratio);
                }
                else
                {
                    cposx = pp->opos.x;
                    cposy = pp->opos.y;
                    cang = pp->oang;
                }
            }
            else
            {
#ifdef __ANDROID__
                CONTROL_Android_ScrollMap(&ud.fola,& ud.folx,&ud.foly,&pp->zoom);
#else
                if (!ud.pause_on)
                {
                    ud.fola += ud.folavel>>3;
                    ud.folx += (ud.folfvel*sintable[(512+2048-ud.fola)&2047])>>14;
                    ud.foly += (ud.folfvel*sintable[(512+1024-512-ud.fola)&2047])>>14;
                }
#endif
                cposx = ud.folx;
                cposy = ud.foly;
                cang = ud.fola;
            }

            if (ud.overhead_on == 2)
            {
                clearview(0L);
                drawmapview(cposx,cposy,pp->zoom,cang);
            }
            G_DrawOverheadMap(cposx,cposy,pp->zoom,cang);

            G_RestoreInterpolations();

            if (ud.overhead_on == 2)
            {
                const int32_t a = (ud.screen_size > 0) ? 147 : 179;
                minitext(5,a+6,EpisodeNames[ud.volume_number],0,2+8+16+256);
                minitext(5,a+6+6,MapInfo[ud.volume_number*MAXLEVELS + ud.level_number].name,0,2+8+16+256);
            }
        }
    }

    if (pp->invdisptime > 0) G_DrawInventory(pp);

    if (VM_OnEvent(EVENT_DISPLAYSBAR, g_player[screenpeek].ps->i, screenpeek) == 0)
        G_DrawStatusBar(screenpeek);

#ifdef SPLITSCREEN_MOD_HACKS
    // HACK
    if (g_fakeMultiMode==2)
    {
        G_DrawStatusBar(1);
        G_PrintGameQuotes(1);
    }
#endif

    G_PrintGameQuotes(screenpeek);

    if (ud.show_level_text && hud_showmapname && g_levelTextTime > 1)
    {
        int32_t bits = 10+16;

        if (g_levelTextTime < 3)
            bits |= 1+32;
        else if (g_levelTextTime < 5)
            bits |= 1;

        if (MapInfo[(ud.volume_number*MAXLEVELS) + ud.level_number].name != NULL)
        {
            if (currentboardfilename[0] != 0 && ud.volume_number == 0 && ud.level_number == 7)
                menutext_(160,90+16+8,-g_levelTextTime+22/*quotepulseshade*/,0,currentboardfilename,bits);
            else menutext_(160,90+16+8,-g_levelTextTime+22/*quotepulseshade*/,0,MapInfo[(ud.volume_number*MAXLEVELS) + ud.level_number].name,bits);
        }
    }

    if (I_EscapeTrigger() && ud.overhead_on == 0
        && ud.show_help == 0
        && g_player[myconnectindex].ps->newowner == -1)
    {
        if ((g_player[myconnectindex].ps->gm&MODE_MENU) == MODE_MENU && g_currentMenu <= MENU_MAIN_INGAME)
        {
            I_EscapeTriggerClear();
            S_PlaySound(EXITMENUSOUND);
            M_ChangeMenu(MENU_CLOSE);
            S_PauseSounds(0);
        }
        else if ((g_player[myconnectindex].ps->gm&MODE_MENU) != MODE_MENU &&
                 g_player[myconnectindex].ps->newowner == -1 &&
                 (g_player[myconnectindex].ps->gm&MODE_TYPE) != MODE_TYPE)
        {
            I_EscapeTriggerClear();
            S_PauseSounds(1);
            S_MenuSound();

            M_OpenMenu(myconnectindex);

            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2) ready2send = 0;

            if (g_player[myconnectindex].ps->gm&MODE_GAME) M_ChangeMenu(MENU_MAIN_INGAME);
            else M_ChangeMenu(MENU_MAIN);
            screenpeek = myconnectindex;
        }
    }

    if (VM_HaveEvent(EVENT_DISPLAYREST))
    {
        int32_t vr=viewingrange, asp=yxaspect;
        VM_OnEvent_(EVENT_DISPLAYREST, g_player[screenpeek].ps->i, screenpeek);
        setaspect(vr, asp);
    }

    if (g_player[myconnectindex].ps->newowner == -1 && ud.overhead_on == 0 && ud.crosshair && ud.camerasprite == -1)
    {
        int32_t a = VM_OnEvent(EVENT_DISPLAYCROSSHAIR, g_player[screenpeek].ps->i, screenpeek);

        if ((unsigned)a < MAXTILES)
        {
            if (a == 0)
                a = CROSSHAIR;

            vec2_t crosshairpos = { 160<<16, 100<<16 };

            rotatesprite_win(crosshairpos.x-(g_player[myconnectindex].ps->look_ang<<15),crosshairpos.y,scale(65536,ud.crosshairscale,100),
                             0,a,0,CROSSHAIR_PAL,2+1);

#ifdef GEKKO
            if ((g_player[myconnectindex].ps->gm&MODE_MENU) == 0 && readmouseabsxy(&crosshairpos, &mouseabs))
                rotatesprite_win(crosshairpos.x,crosshairpos.y,scale(65536,ud.crosshairscale,100),0,a,0,CROSSHAIR_PAL,2+1);
#endif
        }
    }
#if 0
    if (GametypeFlags[ud.coop] & GAMETYPE_TDM)
    {
        for (i=0; i<ud.multimode; i++)
        {
            if (g_player[i].ps->team == g_player[myconnectindex].ps->team)
            {
                j = min(max((G_GetAngleDelta(getangle(g_player[i].ps->pos.x-g_player[myconnectindex].ps->pos.x,
                                                      g_player[i].ps->pos.y-g_player[myconnectindex].ps->pos.y),g_player[myconnectindex].ps->ang))>>1,-160),160);
                rotatesprite_win((160-j)<<16,100L<<16,65536L,0,DUKEICON,0,0,2+1);
            }
        }
    }
#endif

    if (ud.pause_on==1 && (g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
        menutext(160,100,0,0,"Game Paused");

    if (ud.coords)
        G_PrintCoords(screenpeek);

#ifdef YAX_DEBUG
    M32_drawdebug();
#endif

#ifdef USE_OPENGL
    extern int32_t mdpause;
    mdpause = (ud.pause_on || (ud.recstat==2 && (g_demo_paused && g_demo_goalCnt==0)) || (g_player[myconnectindex].ps->gm&MODE_MENU && numplayers < 2));
#endif

    G_PrintFPS();

    // JBF 20040124: display level stats in screen corner
    if (ud.overhead_on != 2 && ud.levelstats && VM_OnEvent(EVENT_DISPLAYLEVELSTATS, g_player[myconnectindex].ps->i, myconnectindex) == 0)
    {
        DukePlayer_t const * const myps = g_player[myconnectindex].ps;

        i = 198<<16;

        if (ud.screen_size == 4)
        {
            if (ud.althud != 2)
                i -= sbarsc(ud.althud ? (tilesiz[BIGALPHANUM].y+8)<<16 : tilesiz[INVENTORYBOX].y<<16);
        }
        else if (ud.screen_size > 2)
            i -= sbarsc(tilesiz[BOTTOMSTATUSBAR].y<<16);

        Bsprintf(tempbuf,"T:^15%d:%02d.%02d",
                 (myps->player_par/(REALGAMETICSPERSEC*60)),
                 (myps->player_par/REALGAMETICSPERSEC)%60,
                 ((myps->player_par%REALGAMETICSPERSEC)*33)/10
            );
        G_ScreenText(STARTALPHANUM, 2<<16, i-gtextsc(21<<16), gtextsc(65536), 0, 0, tempbuf, 0, 10, 2|8|16|256|ROTATESPRITE_FULL16, 0, 5<<16, 8<<16, 8<<16, 0, TEXT_XOFFSETZERO|TEXT_GAMETEXTNUMHACK, 0, 0, xdim-1, ydim-1);

        if (ud.player_skill > 3 || ((g_netServer || ud.multimode > 1) && !GTFLAGS(GAMETYPE_PLAYERSFRIENDLY)))
            Bsprintf(tempbuf,"K:^15%d",(ud.multimode>1 &&!GTFLAGS(GAMETYPE_PLAYERSFRIENDLY))?
                     myps->frag-myps->fraggedself:myps->actors_killed);
        else
        {
            if (myps->actors_killed >= myps->max_actors_killed)
                Bsprintf(tempbuf,"K:%d/%d",myps->actors_killed,
                         myps->max_actors_killed>myps->actors_killed?
                         myps->max_actors_killed:myps->actors_killed);
            else
                Bsprintf(tempbuf,"K:^15%d/%d",myps->actors_killed,
                         myps->max_actors_killed>myps->actors_killed?
                         myps->max_actors_killed:myps->actors_killed);
        }
        G_ScreenText(STARTALPHANUM, 2<<16, i-gtextsc(14<<16), gtextsc(65536), 0, 0, tempbuf, 0, 10, 2|8|16|256|ROTATESPRITE_FULL16, 0, 5<<16, 8<<16, 8<<16, 0, TEXT_XOFFSETZERO|TEXT_GAMETEXTNUMHACK, 0, 0, xdim-1, ydim-1);

        if (myps->secret_rooms == myps->max_secret_rooms)
            Bsprintf(tempbuf,"S:%d/%d", myps->secret_rooms, myps->max_secret_rooms);
        else Bsprintf(tempbuf,"S:^15%d/%d", myps->secret_rooms, myps->max_secret_rooms);
        G_ScreenText(STARTALPHANUM, 2<<16, i-gtextsc(7<<16), gtextsc(65536), 0, 0, tempbuf, 0, 10, 2|8|16|256|ROTATESPRITE_FULL16, 0, 5<<16, 8<<16, 8<<16, 0, TEXT_XOFFSETZERO|TEXT_GAMETEXTNUMHACK, 0, 0, xdim-1, ydim-1);
    }

    if (g_player[myconnectindex].gotvote == 0 && voting != -1 && voting != myconnectindex)
    {
        Bsprintf(tempbuf,"%s^00 has called a vote for map",g_player[voting].user_name);
        gametext(160,40,tempbuf,0,2+8+16);
        Bsprintf(tempbuf,"%s (E%dL%d)",MapInfo[vote_episode*MAXLEVELS + vote_map].name,vote_episode+1,vote_map+1);
        gametext(160,48,tempbuf,0,2+8+16);
        gametext(160,70,"Press F1 to Accept, F2 to Decline",0,2+8+16);
    }

    if (BUTTON(gamefunc_Show_DukeMatch_Scores))
        G_ShowScores();

    if (g_Debug)
        G_ShowCacheLocks();

#ifdef LUNATIC
    El_DisplayErrors();
#endif

#ifndef EDUKE32_TOUCH_DEVICES
    if (VOLUMEONE)
    {
        if (ud.show_help == 0 && g_showShareware > 0 && (g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
            rotatesprite_fs((320-50)<<16,9<<16,65536L,0,BETAVERSION,0,0,2+8+16+128);
    }
#endif

    if (!Demo_IsProfiling())
    {
        if (g_player[myconnectindex].ps->gm&MODE_TYPE)
            Net_SendMessage();
        else
            M_DisplayMenus();
    }

    {
        static int32_t applied = 0;

        if (tint.maxf)
        {
            G_FadePalaccum(&tint);
            applied = 1;
        }
        else if (applied)
        {
            // be sure to always un-apply a tint.
            setpalettefade(0,0,0, 0);
            applied = 0;
        }
    }
}

static int32_t G_DoThirdPerson(const DukePlayer_t *pp, vec3_t *vect, int16_t *vsectnum, int32_t ang, int32_t horiz)
{
    spritetype *sp = &sprite[pp->i];
    int32_t i, hx, hy;
    int32_t daang;
    int32_t bakcstat = sp->cstat;
    hitdata_t hit;
    vec3_t n = {
        sintable[(ang+1536)&2047]>>4,
        sintable[(ang+1024)&2047]>>4,
        (horiz-100)*128
    };

    updatesectorz(vect->x,vect->y,vect->z,vsectnum);

    sp->cstat &= ~0x101;
    hitscan(vect, *vsectnum, n.x,n.y,n.z, &hit, CLIPMASK1);
    sp->cstat = bakcstat;

    if (*vsectnum < 0)
        return -1;

    hx = hit.pos.x-(vect->x);
    hy = hit.pos.y-(vect->y);

    if (klabs(n.x)+klabs(n.y) > klabs(hx)+klabs(hy))
    {
        *vsectnum = hit.sect;

        if (hit.wall >= 0)
        {
            daang = getangle(wall[wall[hit.wall].point2].x-wall[hit.wall].x,
                             wall[wall[hit.wall].point2].y-wall[hit.wall].y);

            i = n.x*sintable[daang] + n.y*sintable[(daang+1536)&2047];

            if (klabs(n.x) > klabs(n.y))
                hx -= mulscale28(n.x,i);
            else hy -= mulscale28(n.y,i);
        }
        else if (hit.sprite < 0)
        {
            if (klabs(n.x) > klabs(n.y))
                hx -= (n.x>>5);
            else hy -= (n.y>>5);
        }

        if (klabs(n.x) > klabs(n.y))
            i = divscale16(hx,n.x);
        else i = divscale16(hy,n.y);

        if (i < CAMERADIST)
            CAMERADIST = i;
    }

    vect->x += mulscale16(n.x,CAMERADIST);
    vect->y += mulscale16(n.y,CAMERADIST);
    vect->z += mulscale16(n.z,CAMERADIST);

    CAMERADIST = min(CAMERADIST+((totalclock-CAMERACLOCK)<<10),65536);
    CAMERACLOCK = totalclock;

    updatesectorz(vect->x,vect->y,vect->z,vsectnum);

    return 0;
}

#ifdef LEGACY_ROR
char ror_protectedsectors[MAXSECTORS];
static int32_t drawing_ror = 0;
static int32_t ror_sprite = -1;

static void G_OROR_DupeSprites(const spritetype *sp)
{
    // dupe the sprites touching the portal to the other sector
    int32_t k;
    const spritetype *refsp;

    if ((unsigned)sp->yvel >= (unsigned)playerswhenstarted)
        return;

    refsp = &sprite[sp->yvel];

    for (SPRITES_OF_SECT(sp->sectnum, k))
    {
        if (spritesortcnt >= MAXSPRITESONSCREEN)
            break;

        if (sprite[k].picnum != SECTOREFFECTOR && sprite[k].z >= sp->z)
        {
            Bmemcpy(&tsprite[spritesortcnt], &sprite[k], sizeof(spritetype));

            tsprite[spritesortcnt].x += (refsp->x - sp->x);
            tsprite[spritesortcnt].y += (refsp->y - sp->y);
            tsprite[spritesortcnt].z = tsprite[spritesortcnt].z - sp->z + actor[sp->yvel].ceilingz;
            tsprite[spritesortcnt].sectnum = refsp->sectnum;
            tsprite[spritesortcnt].owner = k;
            tsprite[spritesortcnt].extra = 0;

//            OSD_Printf("duped sprite of pic %d at %d %d %d\n",tsprite[spritesortcnt].picnum,tsprite[spritesortcnt].x,tsprite[spritesortcnt].y,tsprite[spritesortcnt].z);
            spritesortcnt++;
        }
    }
}

static void G_SE40(int32_t smoothratio)
{
    if ((unsigned)ror_sprite < MAXSPRITES)
    {
        int32_t x, y, z;
        int16_t sect;
        int32_t level = 0;
        const spritetype *const sp = &sprite[ror_sprite];
        const int32_t sprite2 = sp->yvel;

        if ((unsigned)sprite2 >= MAXSPRITES)
            return;

        if (klabs(sector[sp->sectnum].floorz - sp->z) < klabs(sector[sprite[sprite2].sectnum].floorz - sprite[sprite2].z))
            level = 1;

        x = CAMERA(pos.x) - sp->x;
        y = CAMERA(pos.y) - sp->y;
        z = CAMERA(pos.z) - (level ? sector[sp->sectnum].floorz : sector[sp->sectnum].ceilingz);

        sect = sprite[sprite2].sectnum;
        updatesector(sprite[sprite2].x + x, sprite[sprite2].y + y, &sect);

        if (sect != -1)
        {
            int32_t renderz, picnum;
            // XXX: PK: too large stack allocation for my taste
            int16_t backupstat[MAXSECTORS];
            int32_t backupz[MAXSECTORS];
            int32_t i;
            int32_t pix_diff, newz;
            //                initprintf("drawing ror\n");

            if (level)
            {
                // renderz = sector[sprite[sprite2].sectnum].ceilingz;
                renderz = sprite[sprite2].z - (sprite[sprite2].yrepeat * tilesiz[sprite[sprite2].picnum].y<<1);
                picnum = sector[sprite[sprite2].sectnum].ceilingpicnum;
                sector[sprite[sprite2].sectnum].ceilingpicnum = 562;
                tilesiz[562].x = tilesiz[562].y = 0;

                pix_diff = klabs(z) >> 8;
                newz = - ((pix_diff / 128) + 1) * (128<<8);

                for (i = 0; i < numsectors; i++)
                {
                    backupstat[i] = sector[i].ceilingstat;
                    backupz[i] = sector[i].ceilingz;
                    if (!ror_protectedsectors[i] || (ror_protectedsectors[i] && sp->lotag == 41))
                    {
                        sector[i].ceilingstat = 1;
                        sector[i].ceilingz += newz;
                    }
                }
            }
            else
            {
                // renderz = sector[sprite[sprite2].sectnum].floorz;
                renderz = sprite[sprite2].z;
                picnum = sector[sprite[sprite2].sectnum].floorpicnum;
                sector[sprite[sprite2].sectnum].floorpicnum = 562;
                tilesiz[562].x = tilesiz[562].y = 0;

                pix_diff = klabs(z) >> 8;
                newz = ((pix_diff / 128) + 1) * (128<<8);

                for (i = 0; i < numsectors; i++)
                {
                    backupstat[i] = sector[i].floorstat;
                    backupz[i] = sector[i].floorz;
                    if (!ror_protectedsectors[i] || (ror_protectedsectors[i] && sp->lotag == 41))
                    {
                        sector[i].floorstat = 1;
                        sector[i].floorz = +newz;
                    }
                }
            }

#ifdef POLYMER
            if (getrendermode() == REND_POLYMER)
                polymer_setanimatesprites(G_DoSpriteAnimations, CAMERA(pos.x), CAMERA(pos.y), CAMERA(ang), smoothratio);
#endif
            drawrooms(sprite[sprite2].x + x, sprite[sprite2].y + y,
                      z + renderz, CAMERA(ang), CAMERA(horiz), sect);
            drawing_ror = 1 + level;

            if (drawing_ror == 2) // viewing from top
                G_OROR_DupeSprites(sp);

            G_DoSpriteAnimations(CAMERA(pos.x),CAMERA(pos.y),CAMERA(ang),smoothratio);
            drawmasks();

            if (level)
            {
                sector[sprite[sprite2].sectnum].ceilingpicnum = picnum;
                for (i = 0; i < numsectors; i++)
                {
                    sector[i].ceilingstat = backupstat[i];
                    sector[i].ceilingz = backupz[i];
                }
            }
            else
            {
                sector[sprite[sprite2].sectnum].floorpicnum = picnum;

                for (i = 0; i < numsectors; i++)
                {
                    sector[i].floorstat = backupstat[i];
                    sector[i].floorz = backupz[i];
                }
            }
        }
    }
}
#endif

void G_HandleMirror(int32_t x, int32_t y, int32_t z, int32_t a, int32_t horiz, int32_t smoothratio)
{
    if ((gotpic[MIRROR>>3]&(1<<(MIRROR&7)))
#ifdef POLYMER
        && (getrendermode() != REND_POLYMER)
#endif
        )
    {
        if (g_mirrorCount == 0)
        {
            // NOTE: We can have g_mirrorCount==0 but gotpic'd MIRROR,
            // for example in LNGA2.
            gotpic[MIRROR>>3] &= ~(1<<(MIRROR&7));
            return;
        }

        int32_t i = 0, dst = INT32_MAX;

        for (int32_t k=g_mirrorCount-1; k>=0; k--)
        {
            const int32_t j =
                klabs(wall[g_mirrorWall[k]].x - x) +
                klabs(wall[g_mirrorWall[k]].y - y);

            if (j < dst)
                dst = j, i = k;
        }

        if (wall[g_mirrorWall[i]].overpicnum != MIRROR)
        {
            // Try to find a new mirror wall in case the original one was broken.

            int32_t startwall = sector[g_mirrorSector[i]].wallptr;
            int32_t endwall = startwall + sector[g_mirrorSector[i]].wallnum;

            for (int32_t k=startwall; k<endwall; k++)
            {
                int32_t j = wall[k].nextwall;
                if (j >= 0 && (wall[j].cstat&32) && wall[j].overpicnum==MIRROR)  // cmp. premap.c
                {
                    g_mirrorWall[i] = j;
                    break;
                }
            }
        }

        if (wall[g_mirrorWall[i]].overpicnum == MIRROR)
        {
            int32_t tposx, tposy;
            int16_t tang;

            preparemirror(x, y, a, g_mirrorWall[i], &tposx, &tposy, &tang);

            int32_t j = g_visibility;
            g_visibility = (j>>1) + (j>>2);

            if (getrendermode() == REND_CLASSIC)
            {
                int32_t didmirror;

                yax_preparedrawrooms();
                didmirror = drawrooms(tposx,tposy,z,tang,horiz,g_mirrorSector[i]+MAXSECTORS);
                yax_drawrooms(G_DoSpriteAnimations, g_mirrorSector[i], didmirror, smoothratio);
            }
#ifdef USE_OPENGL
            else
                drawrooms(tposx,tposy,z,tang,horiz,g_mirrorSector[i]+MAXSECTORS);
            // XXX: Sprites don't get drawn with TROR/Polymost
#endif
            display_mirror = 1;
            G_DoSpriteAnimations(tposx,tposy,tang,smoothratio);
            display_mirror = 0;

            drawmasks();
            completemirror();   //Reverse screen x-wise in this function
            g_visibility = j;
        }

#ifdef SPLITSCREEN_MOD_HACKS
        if (!g_fakeMultiMode)
#endif
        {
            // HACK for splitscreen mod: this is so that mirrors will be drawn
            // from showview commands. Ugly, because we'll attempt do draw mirrors
            // each frame then. But it's better than not drawing them, I guess.
            // XXX: fix the sequence of setting/clearing this bit. Right now,
            // we always draw one frame without drawing the mirror, after which
            // the bit gets set and drawn subsequently.
            gotpic[MIRROR>>3] &= ~(1<<(MIRROR&7));
        }
    }
}

#ifdef USE_OPENGL
static void G_ReadGLFrame(void)
{
    // Save OpenGL screenshot with Duke3D palette
    // NOTE: maybe need to move this to the engine...
    palette_t *const frame = (palette_t *const)Xcalloc(xdim * ydim, sizeof(palette_t));
    char *const pic = (char *) waloff[TILE_SAVESHOT];

    int32_t x, y;
    const int32_t xf = divscale16(xdim, 320);  // (xdim<<16)/320
    const int32_t yf = divscale16(ydim, 200);  // (ydim<<16)/200

    tilesiz[TILE_SAVESHOT].x = 200;
    tilesiz[TILE_SAVESHOT].y = 320;

    if (!frame)
    {
        Bmemset(pic, 0, 320 * 200);
        return;
    }

    begindrawing();
    bglReadPixels(0, 0, xdim, ydim, GL_RGBA, GL_UNSIGNED_BYTE, frame);
    enddrawing();

    for (y = 0; y < 200; y++)
    {
        const int32_t base = mulscale16(200 - y - 1, yf)*xdim;

        for (x = 0; x < 320; x++)
        {
            const palette_t *pix = &frame[base + mulscale16(x, xf)];
            pic[320 * y + x] = getclosestcol(pix->r, pix->g, pix->b);
        }
    }

    Bfree(frame);
}
#endif

void G_DrawRooms(int32_t snum, int32_t smoothratio)
{
    int32_t i, dont_draw;
    DukePlayer_t *const p = g_player[snum].ps;

    int32_t tmpyx=yxaspect, tmpvr=viewingrange;

    if (g_networkMode == NET_DEDICATED_SERVER) return;

    totalclocklock = totalclock;

    if (pub > 0 || getrendermode() >= REND_POLYMOST) // JBF 20040101: redraw background always
    {
#ifndef EDUKE32_TOUCH_DEVICES
        if (ud.screen_size >= 8)
#endif
            G_DrawBackground();
        pub = 0;
    }

    VM_OnEvent(EVENT_DISPLAYSTART, p->i, snum);

    if (ud.overhead_on == 2 || ud.show_help || (p->cursectnum == -1 && getrendermode() != REND_CLASSIC))
        return;

    if (r_usenewaspect)
    {
        newaspect_enable = 1;
        setaspect_new();
    }

    if (ud.pause_on || p->on_crane > -1)
        smoothratio = 65536;
    else
        smoothratio = calc_smoothratio(totalclock, ototalclock);

    {
        int32_t vis = p->visibility;
        g_visibility = (vis <= 0) ? 0 : (int32_t)(vis * (numplayers > 1 ? 1.f : r_ambientlightrecip));
    }

    CAMERA(sect) = p->cursectnum;

    G_RefreshLights(smoothratio);
    G_DoInterpolations(smoothratio);
    G_AnimateCamSprite(smoothratio);

    if (ud.camerasprite >= 0)
    {
        spritetype *const s = &sprite[ud.camerasprite];

        // XXX: what?
        if (s->yvel < 0) s->yvel = -100;
        else if (s->yvel > 199) s->yvel = 300;

        CAMERA(ang) = actor[ud.camerasprite].tempang +
            mulscale16(((s->ang+1024-actor[ud.camerasprite].tempang)&2047)-1024, smoothratio);

        dont_draw = VM_OnEvent(EVENT_DISPLAYROOMSCAMERA, p->i, snum);

        if (dont_draw != 1)  // event return values other than 0 and 1 are reserved
        {
            if (EDUKE32_PREDICT_FALSE(dont_draw != 0))
                OSD_Printf(OSD_ERROR "ERROR: EVENT_DISPLAYROOMSCAMERA return value must be 0 or 1, "
                           "other values are reserved.\n");

#ifdef LEGACY_ROR
            G_SE40(smoothratio);
#endif
#ifdef POLYMER
            if (getrendermode() == REND_POLYMER)
                polymer_setanimatesprites(G_DoSpriteAnimations, s->x, s->y, CAMERA(ang), smoothratio);
#endif
            yax_preparedrawrooms();
            drawrooms(s->x,s->y,s->z-(4<<8),CAMERA(ang),s->yvel,s->sectnum);
            yax_drawrooms(G_DoSpriteAnimations, s->sectnum, 0, smoothratio);
            G_DoSpriteAnimations(s->x,s->y,CAMERA(ang),smoothratio);
            drawmasks();
        }
    }
    else
    {
        int32_t j,fz,cz;
        int32_t tiltcx, tiltcy, tiltcs=0;    // JBF 20030807

        const int32_t vr = divscale22(1,sprite[p->i].yrepeat+28);
        int32_t software_screen_tilting =
            (getrendermode() == REND_CLASSIC && ((ud.screen_tilting && p->rotscrnang
#ifdef SPLITSCREEN_MOD_HACKS
            && !g_fakeMultiMode
#endif
            )));
        int32_t pixelDoubling = 0;

        if (!r_usenewaspect)
        {
            setaspect(vr, yxaspect);
        }
        else
        {
            tmpvr = vr;
            tmpyx = tabledivide32_noinline(65536*ydim*8, xdim*5);

            setaspect(mulscale16(tmpvr,viewingrange), yxaspect);
        }

        if (g_screenCapture)
        {
            walock[TILE_SAVESHOT] = 199;
            if (waloff[TILE_SAVESHOT] == 0)
                allocache(&waloff[TILE_SAVESHOT],200*320,&walock[TILE_SAVESHOT]);

            if (getrendermode() == REND_CLASSIC)
                setviewtotile(TILE_SAVESHOT, 200, 320);
        }
        else if (software_screen_tilting)
        {
            int32_t oviewingrange = viewingrange;  // save it from setaspect()
            const int16_t tang = (ud.screen_tilting) ? p->rotscrnang : 0;

            if (tang == 1024)
            {
                software_screen_tilting = 2;
            }
            else
            {
                // Maximum possible allocation size passed to allocache() below
                // since there is no equivalent of free() for allocache().
#if MAXYDIM >= 640
                const int maxtiltallocsiz = 640*640;
#else
                const int maxtiltallocsiz = 320*320;
#endif
                // To render a tilted screen in high quality, we need at least
                // 640 pixels of *Y* dimension.
#if MAXYDIM >= 640
                // We also need
                //  * xdim >= 640 since tiltcx will be passed as setview()'s x2
                //    which must be less than xdim.
                //  * ydim >= 640 (sic!) since the tile-to-draw-to will be set
                //    up with dimension 400x640, but the engine's arrays like
                //    lastx[] are alloc'd with *xdim* elements! (This point is
                //    the dynamic counterpart of the #if above since we now
                //    allocate these engine arrays tightly.)
                // XXX: The engine should be in charge of setting up everything
                // so that no oob access occur.
                if (xdim >= 640 && ydim >= 640)
                {
                    tiltcs = 2;
                    tiltcx = 640;
                    tiltcy = 400;
                }
                else
#endif
                {
                    // JBF 20030807: Increased tilted-screen quality
                    tiltcs = 1;

                    // NOTE: The same reflections as above apply here, too.
                    // TILT_SETVIEWTOTILE_320.
                    tiltcx = 320;
                    tiltcy = 200;
                }

                {
                    // If the view is rotated (not 0 or 180 degrees modulo 360 degrees),
                    // we render onto a square tile and display a portion of that
                    // rotated on-screen later on.
                    const int32_t viewtilexsiz = (tang&1023) ? tiltcx : tiltcy;
                    const int32_t viewtileysiz = tiltcx;

                    walock[TILE_TILT] = 255;
                    if (waloff[TILE_TILT] == 0)
                        allocache(&waloff[TILE_TILT], maxtiltallocsiz, &walock[TILE_TILT]);

                    setviewtotile(TILE_TILT, viewtilexsiz, viewtileysiz);
                }

                if ((tang&1023) == 512)
                {
                    //Block off unscreen section of 90ø tilted screen
                    j = tiltcx-(60*tiltcs);
                    for (i=(60*tiltcs)-1; i>=0; i--)
                    {
                        startumost[i] = 1;
                        startumost[i+j] = 1;
                        startdmost[i] = 0;
                        startdmost[i+j] = 0;
                    }
                }

                i = (tang&511);
                if (i > 256)
                    i = 512-i;
                i = sintable[i+512]*8 + sintable[i]*5;

//                setaspect(i>>1, yxaspect);
                setaspect(mulscale16(oviewingrange,i>>1), yxaspect);

                tmpvr = i>>1;
                tmpyx = tabledivide32_noinline(65536*ydim*8, xdim*5);
            }
        }
        else if (getrendermode() >= REND_POLYMOST && (ud.screen_tilting
#ifdef SPLITSCREEN_MOD_HACKS
        && !g_fakeMultiMode
#endif
        ))
        {
#ifdef USE_OPENGL
            setrollangle(p->orotscrnang + mulscale16(((p->rotscrnang - p->orotscrnang + 1024)&2047)-1024, smoothratio));
#endif
            p->orotscrnang = p->rotscrnang; // JBF: save it for next time
        }
        else if (!ud.detail && getrendermode()==REND_CLASSIC)
        {
            pixelDoubling = 1;
            g_halveScreenArea = 1;
            G_UpdateScreenArea();
        }

        if (p->newowner < 0)
        {
            vec3_t cam = { p->opos.x+mulscale16(p->pos.x-p->opos.x, smoothratio),
                           p->opos.y+mulscale16(p->pos.y-p->opos.y, smoothratio),
                           p->opos.z+mulscale16(p->pos.z-p->opos.z, smoothratio)
                         };

            Bmemcpy(&CAMERA(pos), &cam, sizeof(vec3_t));
            CAMERA(ang) = p->oang + mulscale16(((p->ang+1024-p->oang)&2047)-1024, smoothratio);
            CAMERA(ang) += p->look_ang;
            CAMERA(horiz) = p->ohoriz+p->ohorizoff
                + mulscale16((p->horiz+p->horizoff-p->ohoriz-p->ohorizoff), smoothratio);

            if (ud.viewbob)
            {
                int32_t addz = (p->opyoff + mulscale16(p->pyoff-p->opyoff, smoothratio));
                if (p->over_shoulder_on)
                    addz >>= 3;

                CAMERA(pos.z) += addz;
            }

            if (p->over_shoulder_on)
            {
                CAMERA(pos.z) -= 3072;
                if (G_DoThirdPerson(p, &CAMERA(pos), &CAMERA(sect), CAMERA(ang), CAMERA(horiz)) < 0)
                {
                    CAMERA(pos.z) += 3072;
                    G_DoThirdPerson(p, &CAMERA(pos), &CAMERA(sect), CAMERA(ang), CAMERA(horiz));
                }
            }
        }
        else
        {
            vec3_t cam = G_GetCameraPosition(p->newowner, smoothratio);

            // looking through viewscreen
            Bmemcpy(&CAMERA(pos), &cam, sizeof(vec3_t));
            CAMERA(ang) = p->ang + p->look_ang;
            CAMERA(horiz) = 100+sprite[p->newowner].shade;
            CAMERA(sect) = sprite[p->newowner].sectnum;
        }

        cz = actor[p->i].ceilingz;
        fz = actor[p->i].floorz;

        if (g_earthquakeTime > 0 && p->on_ground == 1)
        {
            CAMERA(pos.z) += 256-(((g_earthquakeTime)&1)<<9);
            CAMERA(ang) += (2-((g_earthquakeTime)&2))<<2;
        }

        if (sprite[p->i].pal == 1)
            CAMERA(pos.z) -= (18<<8);

        if (p->newowner < 0 && p->spritebridge == 0)
        {
            // NOTE: when shrunk, p->pos.z can be below the floor.  This puts the
            // camera into the sector again then.

            if (CAMERA(pos.z) < (p->truecz + (4<<8)))
                CAMERA(pos.z) = cz + (4<<8);
            else if (CAMERA(pos.z) > (p->truefz - (4<<8)))
                CAMERA(pos.z) = fz - (4<<8);
        }

        while (CAMERA(sect) >= 0)  // if, really
        {
            getzsofslope(CAMERA(sect),CAMERA(pos.x),CAMERA(pos.y),&cz,&fz);
#ifdef YAX_ENABLE
            if (yax_getbunch(CAMERA(sect), YAX_CEILING) >= 0)
            {
                if (CAMERA(pos.z) < cz)
                {
                    updatesectorz(CAMERA(pos.x), CAMERA(pos.y), CAMERA(pos.z), &CAMERA(sect));
                    break;  // since CAMERA(sect) might have been updated to -1
                    // NOTE: fist discovered in WGR2 SVN r134, til' death level 1
                    //  (Lochwood Hollow).  A problem REMAINS with Polymost, maybe classic!
                }
            }
            else
#endif
                if (CAMERA(pos.z) < cz+(4<<8))
                    CAMERA(pos.z) = cz+(4<<8);

#ifdef YAX_ENABLE
            if (yax_getbunch(CAMERA(sect), YAX_FLOOR) >= 0)
            {
                if (CAMERA(pos.z) > fz)
                    updatesectorz(CAMERA(pos.x), CAMERA(pos.y), CAMERA(pos.z), &CAMERA(sect));
            }
            else
#endif
                if (CAMERA(pos.z) > fz-(4<<8))
                    CAMERA(pos.z) = fz-(4<<8);

            break;
        }

        // NOTE: might be rendering off-screen here, so CON commands that draw stuff
        //  like showview must cope with that situation or bail out!
        dont_draw = VM_OnEvent(EVENT_DISPLAYROOMS, p->i, snum);

        CAMERA(horiz) = clamp(CAMERA(horiz), HORIZ_MIN, HORIZ_MAX);

        if (dont_draw != 1)  // event return values other than 0 and 1 are reserved
        {
            if (EDUKE32_PREDICT_FALSE(dont_draw != 0))
                OSD_Printf(OSD_ERROR "ERROR: EVENT_DISPLAYROOMS return value must be 0 or 1, "
                           "other values are reserved.\n");

            G_HandleMirror(CAMERA(pos.x), CAMERA(pos.y), CAMERA(pos.z), CAMERA(ang), CAMERA(horiz), smoothratio);
#ifdef LEGACY_ROR
            G_SE40(smoothratio);
#endif
#ifdef POLYMER
            if (getrendermode() == REND_POLYMER)
                polymer_setanimatesprites(G_DoSpriteAnimations, CAMERA(pos.x),CAMERA(pos.y),CAMERA(ang),smoothratio);
#endif
            // for G_PrintCoords
            dr_viewingrange = viewingrange;
            dr_yxaspect = yxaspect;
#ifdef DEBUG_MIRRORS_ONLY
            gotpic[MIRROR>>3] |= (1<<(MIRROR&7));
#else
            yax_preparedrawrooms();
            drawrooms(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),CAMERA(ang),CAMERA(horiz),CAMERA(sect));
            yax_drawrooms(G_DoSpriteAnimations, CAMERA(sect), 0, smoothratio);
#ifdef LEGACY_ROR
            if ((unsigned)ror_sprite < MAXSPRITES && drawing_ror == 1)  // viewing from bottom
                G_OROR_DupeSprites(&sprite[ror_sprite]);
#endif
            G_DoSpriteAnimations(CAMERA(pos.x),CAMERA(pos.y),CAMERA(ang),smoothratio);
#ifdef LEGACY_ROR
            drawing_ror = 0;
#endif
            drawmasks();
#endif
        }

        if (g_screenCapture)
        {
            g_screenCapture = 0;

            if (getrendermode() == REND_CLASSIC)
            {
                setviewback();
//                walock[TILE_SAVESHOT] = 1;
            }
#ifdef USE_OPENGL
            else
                G_ReadGLFrame();
#endif
        }
        else if (software_screen_tilting)
        {
            const int16_t tang = (ud.screen_tilting) ? p->rotscrnang : 0;

            if (software_screen_tilting == 2)  // tang == 1024
            {
                begindrawing();
                {
                    const int32_t height = windowy2-windowy1+1;
                    const int32_t width = windowx2-windowx1+1;

                    uint8_t *f = (uint8_t *)(frameplace + ylookup[windowy1]);
                    int32_t x, y;

                    for (y=0; y < (height>>1); y++)
                        swapbufreverse(f + y*bytesperline + windowx2,
                                       f + (height-1-y)*bytesperline + windowx1,
                                       width);

                    f += (height>>1)*bytesperline + windowx1;

                    if (height&1)
                        for (x=0; x<(width>>1); x++)
                            swapchar(&f[x], &f[width-1-x]);
                }
                enddrawing();
            }
            else
            {
                setviewback();
                picanm[TILE_TILT].xofs = picanm[TILE_TILT].yofs = 0;

                i = (tang&511);
                if (i > 256)
                    i = 512-i;
                i = sintable[i+512]*8 + sintable[i]*5;
                i >>= tiltcs; // JBF 20030807

                rotatesprite_win(160<<16,100<<16,i,tang+512,TILE_TILT,0,0,4+2+64+1024);
                walock[TILE_TILT] = 199;
            }
        }
        else if (pixelDoubling)
        {
            Bassert(g_halfScreen.xdimen!=0);
            g_halveScreenArea = 0;
            G_UpdateScreenArea();

            begindrawing();
            {
                uint8_t *const f = (uint8_t *)frameplace;
                const int32_t x1=g_halfScreen.x1, y1=g_halfScreen.y1;
                const int32_t xd=g_halfScreen.xdimen, yd=g_halfScreen.ydimen;
                int32_t dx, dy;

                // Commented out: naive, per-byte access version.
                // Live: optimized version: may access memory unaligned, relies
                // on little-endian byte ordering.

                for (dy=2*yd-1; dy>=0; dy--)
//                    for (dx=2*xd-1; dx>=0; dx--)
                    for (dx=2*xd-4; dx>=0; dx-=4)
                    {
                        const int32_t ylsrc = ylookup[y1+(dy>>1)];
                        const int32_t yldst = ylookup[y1+dy];

//                        f[yldst+x1+dx] = f[ylsrc+x1+(dx>>1)];
                        uint8_t pixr = f[ylsrc+x1+((dx+3)>>1)];
                        uint8_t pixl = f[ylsrc+x1+((dx+1)>>1)];

                        B_BUF32(&f[yldst+x1+dx], pixl|(pixl<<8)|(pixr<<16)|(pixr<<24));
                    }
            }
            enddrawing();
        }
    }

    G_RestoreInterpolations();

    {
        // Totalclock count of last step of p->visibility converging towards
        // ud.const_visibility.
        static int32_t lastvist;
        const int32_t visdif = ud.const_visibility-p->visibility;

        // Check if totalclock was cleared (e.g. restarted game).
        if (totalclock < lastvist)
            lastvist = 0;

        // Every 2nd totalclock increment (each 1/60th second), ...
        while (totalclock >= lastvist+2)
        {
            // ... approximately three-quarter the difference between
            // p->visibility and ud.const_visibility.
            const int32_t visinc = visdif>>2;

            if (klabs(visinc) == 0)
            {
                p->visibility = ud.const_visibility;
                break;
            }

            p->visibility += visinc;
            lastvist = totalclock;
        }
    }

    if (r_usenewaspect)
    {
        newaspect_enable = 0;
        setaspect(tmpvr, tmpyx);
    }
}

void G_DumpDebugInfo(void)
{
#if !defined LUNATIC
    int32_t i,j,x;
    //    FILE * fp=fopen("condebug.log","w");

    VM_ScriptInfo(insptr, 64);
    OSD_Printf("\n");

    OSD_Printf("Current gamevar values:\n");

    for (i=0; i<MAX_WEAPONS; i++)
    {
        for (j=0; j<numplayers; j++)
        {
            OSD_Printf("Player %d\n\n",j);
            OSD_Printf("WEAPON%d_CLIP %" PRIdPTR "\n", i, PWEAPON(j, i, Clip));
            OSD_Printf("WEAPON%d_RELOAD %" PRIdPTR "\n", i, PWEAPON(j, i, Reload));
            OSD_Printf("WEAPON%d_FIREDELAY %" PRIdPTR "\n", i, PWEAPON(j, i, FireDelay));
            OSD_Printf("WEAPON%d_TOTALTIME %" PRIdPTR "\n", i, PWEAPON(j, i, TotalTime));
            OSD_Printf("WEAPON%d_HOLDDELAY %" PRIdPTR "\n", i, PWEAPON(j, i, HoldDelay));
            OSD_Printf("WEAPON%d_FLAGS %" PRIdPTR "\n", i, PWEAPON(j, i, Flags));
            OSD_Printf("WEAPON%d_SHOOTS %" PRIdPTR "\n", i, PWEAPON(j, i, Shoots));
            OSD_Printf("WEAPON%d_SPAWNTIME %" PRIdPTR "\n", i, PWEAPON(j, i, SpawnTime));
            OSD_Printf("WEAPON%d_SPAWN %" PRIdPTR "\n", i, PWEAPON(j, i, Spawn));
            OSD_Printf("WEAPON%d_SHOTSPERBURST %" PRIdPTR "\n", i, PWEAPON(j, i, ShotsPerBurst));
            OSD_Printf("WEAPON%d_WORKSLIKE %" PRIdPTR "\n", i, PWEAPON(j, i, WorksLike));
            OSD_Printf("WEAPON%d_INITIALSOUND %" PRIdPTR "\n", i, PWEAPON(j, i, InitialSound));
            OSD_Printf("WEAPON%d_FIRESOUND %" PRIdPTR "\n", i, PWEAPON(j, i, FireSound));
            OSD_Printf("WEAPON%d_SOUND2TIME %" PRIdPTR "\n", i, PWEAPON(j, i, Sound2Time));
            OSD_Printf("WEAPON%d_SOUND2SOUND %" PRIdPTR "\n", i, PWEAPON(j, i, Sound2Sound));
            OSD_Printf("WEAPON%d_RELOADSOUND1 %" PRIdPTR "\n", i, PWEAPON(j, i, ReloadSound1));
            OSD_Printf("WEAPON%d_RELOADSOUND2 %" PRIdPTR "\n", i, PWEAPON(j, i, ReloadSound2));
            OSD_Printf("WEAPON%d_SELECTSOUND %" PRIdPTR "\n", i, PWEAPON(j, i, SelectSound));
            OSD_Printf("WEAPON%d_FLASHCOLOR %" PRIdPTR "\n", i, PWEAPON(j, i, FlashColor));
        }
        OSD_Printf("\n");
    }

    for (x=0; x<MAXSTATUS; x++)
    {
        j = headspritestat[x];
        while (j >= 0)
        {
            OSD_Printf("Sprite %d (%d,%d,%d) (picnum: %d)\n",j,
                TrackerCast(sprite[j].x),TrackerCast(sprite[j].y),TrackerCast(sprite[j].z),TrackerCast(sprite[j].picnum));
            for (i=0; i<g_gameVarCount; i++)
            {
                if (aGameVars[i].dwFlags & (GAMEVAR_PERACTOR))
                {
                    if (aGameVars[i].val.plValues[j] != aGameVars[i].lDefault)
                    {
                        OSD_Printf("gamevar %s ",aGameVars[i].szLabel);
                        OSD_Printf("%" PRIdPTR "",aGameVars[i].val.plValues[j]);
                        OSD_Printf(" GAMEVAR_PERACTOR");
                        if (aGameVars[i].dwFlags != GAMEVAR_PERACTOR)
                        {
                            OSD_Printf(" // ");
                            if (aGameVars[i].dwFlags & (GAMEVAR_SYSTEM))
                            {
                                OSD_Printf(" (system)");
                            }
                        }
                        OSD_Printf("\n");
                    }
                }
            }
            OSD_Printf("\n");
            j = nextspritestat[j];
        }
    }
    Gv_DumpValues();
//    fclose(fp);
#endif
    saveboard("debug.map", &g_player[myconnectindex].ps->pos, g_player[myconnectindex].ps->ang,
              g_player[myconnectindex].ps->cursectnum);
}

// if <set_movflag_uncond> is true, set the moveflag unconditionally,
// else only if it equals 0.
static int32_t G_InitActor(int32_t i, int32_t tilenum, int32_t set_movflag_uncond)
{
#if !defined LUNATIC
    if (g_tile[tilenum].execPtr)
    {
        SH = *(g_tile[tilenum].execPtr);
        AC_ACTION_ID(actor[i].t_data) = *(g_tile[tilenum].execPtr+1);
        AC_MOVE_ID(actor[i].t_data) = *(g_tile[tilenum].execPtr+2);

        if (set_movflag_uncond || SHT == 0)  // AC_MOVFLAGS
            SHT = *(g_tile[tilenum].execPtr+3);

        return 1;
    }
#else
    if (El_HaveActor(tilenum))
    {
        // ^^^ C-CON takes precedence for now.
        const el_actor_t *a = &g_elActors[tilenum];
        uint16_t *movflagsptr = &AC_MOVFLAGS(&sprite[i], &actor[i]);

        SH = a->strength;
        AC_ACTION_ID(actor[i].t_data) = a->act.id;
        AC_MOVE_ID(actor[i].t_data) = a->mov.id;
        Bmemcpy(&actor[i].ac, &a->act.ac, sizeof(struct action));
        Bmemcpy(&actor[i].mv, &a->mov.mv, sizeof(struct move));

        if (set_movflag_uncond || *movflagsptr == 0)
            *movflagsptr = a->movflags;

        return 1;
    }
#endif

    return 0;
}

static actor_t NullActor;
static spriteext_t NullSprExt;
static spritesmooth_t NullSprSmooth;

int32_t A_InsertSprite(int16_t whatsect,int32_t s_x,int32_t s_y,int32_t s_z,int16_t s_pn,int8_t s_s,
                       uint8_t s_xr,uint8_t s_yr,int16_t s_a,int16_t s_ve,int16_t s_zv,int16_t s_ow,int16_t s_ss)
{
    int32_t i = Net_IsRelevantStat(s_ss) ? Net_InsertSprite(whatsect, s_ss) : insertsprite(whatsect, s_ss);

    if (EDUKE32_PREDICT_FALSE(i) < 0)
    {
        G_DumpDebugInfo();
        OSD_Printf("Failed spawning pic %d spr from pic %d spr %d at x:%d,y:%d,z:%d,sect:%d\n",
                          s_pn,s_ow < 0 ? -1 : TrackerCast(sprite[s_ow].picnum),s_ow,s_x,s_y,s_z,whatsect);
        G_GameExit("Too many sprites spawned.");
    }

    tspritetype spr_temp = { s_x, s_y,      s_z,  0,   s_pn, s_s,  0, 0,    0, s_xr, s_yr, 0,
                             0,   whatsect, s_ss, s_a, s_ow, s_ve, 0, s_zv, 0, 0,    0 };

#ifdef DEBUGGINGAIDS
    g_spriteStat.numins++;
#endif

    spritetype *s = &sprite[i];
    *s = *(spritetype *)&spr_temp;
    actor[i] = NullActor;
    actor[i].bpos = *(vec3_t *)s;

    if ((unsigned)s_ow < MAXSPRITES)
    {
        actor[i].picnum = sprite[s_ow].picnum;
        actor[i].floorz = actor[s_ow].floorz;
        actor[i].ceilingz = actor[s_ow].ceilingz;
    }

    actor[i].actorstayput = actor[i].extra = actor[i].lightId = -1;
    actor[i].owner = s_ow;

    G_InitActor(i, s_pn, 1);

    spriteext[i] = NullSprExt;
    spritesmooth[i] = NullSprSmooth;

#if defined LUNATIC
    if (!g_noResetVars)
#endif
        A_ResetVars(i);
#if defined LUNATIC
    g_noResetVars = 0;
#endif

    if (VM_HaveEvent(EVENT_EGS))
    {
        int32_t p, pl = A_FindPlayer(s, &p);

        block_deletesprite++;
        VM_OnEventWithDist_(EVENT_EGS, i, pl, p);
        block_deletesprite--;
    }

    return i;
}

#ifdef YAX_ENABLE
void Yax_SetBunchZs(int32_t sectnum, int32_t cf, int32_t daz)
{
    int32_t i, bunchnum = yax_getbunch(sectnum, cf);

    if (bunchnum < 0 || bunchnum >= numyaxbunches)
        return;

    for (SECTORS_OF_BUNCH(bunchnum, YAX_CEILING, i))
        SECTORFLD(i,z, YAX_CEILING) = daz;
    for (SECTORS_OF_BUNCH(bunchnum, YAX_FLOOR, i))
        SECTORFLD(i,z, YAX_FLOOR) = daz;
}

static void Yax_SetBunchInterpolation(int32_t sectnum, int32_t cf)
{
    int32_t i, bunchnum = yax_getbunch(sectnum, cf);

    if (bunchnum < 0 || bunchnum >= numyaxbunches)
        return;
    
    for (SECTORS_OF_BUNCH(bunchnum, YAX_CEILING, i))
        G_SetInterpolation(&sector[i].ceilingz);
    for (SECTORS_OF_BUNCH(bunchnum, YAX_FLOOR, i))
        G_SetInterpolation(&sector[i].floorz);
}
#else
# define Yax_SetBunchInterpolation(sectnum, cf)
#endif

// A_Spawn has two forms with arguments having different meaning:
//
// 1. j>=0: Spawn from parent sprite <j> with picnum <pn>
// 2. j<0: Spawn from already *existing* sprite <pn>
int32_t A_Spawn(int32_t j, int32_t pn)
{
    int32_t i, s, startwall, endwall, sect;
    spritetype *sp;

    if (j >= 0)
    {
        // spawn from parent sprite <j>
        i = A_InsertSprite(sprite[j].sectnum,sprite[j].x,sprite[j].y,sprite[j].z,
                           pn,0,0,0,0,0,0,j,0);
        actor[i].picnum = sprite[j].picnum;
    }
    else
    {
        // spawn from already existing sprite <pn>
        i = pn;

        Bmemset(&actor[i], 0, sizeof(actor_t));
        Bmemcpy(&actor[i].bpos, &sprite[i], sizeof(vec3_t));

        actor[i].picnum = PN;

        if (PN == SECTOREFFECTOR && SLT == 50)
            actor[i].picnum = OW;

        OW = actor[i].owner = i;

        actor[i].floorz = sector[SECT].floorz;
        actor[i].ceilingz = sector[SECT].ceilingz;

        actor[i].actorstayput = actor[i].lightId = actor[i].extra = -1;

        if ((CS&48) && PN != SPEAKER && PN != LETTER && PN != DUCK && PN != TARGET && PN != TRIPBOMB && PN != VIEWSCREEN && PN != VIEWSCREEN2)
            if (!(PN >= CRACK1 && PN <= CRACK4))
            {
                if (SS == 127)
                    goto SPAWN_END;

                if (A_CheckSwitchTile(i) && (CS&16))
                {
                    if (sprite[i].pal && PN != ACCESSSWITCH && PN != ACCESSSWITCH2)
                    {
                        if (((!g_netServer && ud.multimode < 2)) || ((g_netServer || ud.multimode > 1) && !GTFLAGS(GAMETYPE_DMSWITCHES)))
                        {
                            sprite[i].xrepeat = sprite[i].yrepeat = 0;
                            SLT = SHT = 0;
                            sprite[i].cstat = 32768;
                            goto SPAWN_END;
                        }
                    }

                    CS |= 257;

                    if (sprite[i].pal && PN != ACCESSSWITCH && PN != ACCESSSWITCH2)
                        sprite[i].pal = 0;
                    goto SPAWN_END;
                }

                if (SHT)
                {
                    changespritestat(i, STAT_FALLER);
                    CS |=  257;
                    SH = g_impactDamage;
                    goto SPAWN_END;
                }
            }

        if (CS&1)
            CS |= 256;

        if (!G_InitActor(i, sprite[i].picnum, 0))
            T2 = T5 = 0;  // AC_MOVE_ID, AC_ACTION_ID
    }

    sp = &sprite[i];
    sect = sp->sectnum;

    //some special cases that can't be handled through the dynamictostatic system.
    if ((sp->picnum >= BOLT1 && sp->picnum <= BOLT1+3) ||
            (sp->picnum >= SIDEBOLT1 && sp->picnum <= SIDEBOLT1+3))
    {
        T1 = sp->xrepeat;
        T2 = sp->yrepeat;
        sp->yvel = 0;
        changespritestat(i, STAT_STANDABLE);
    }
    else if ((sp->picnum >= CAMERA1 && sp->picnum <= CAMERA1+4) ||
                 sp->picnum==CAMERAPOLE || sp->picnum==GENERICPOLE)
    {
        if (sp->picnum != GENERICPOLE)
        {
            sp->extra = 1;

            sp->cstat &= 32768;
            if (g_damageCameras) sp->cstat |= 257;
        }
        if ((!g_netServer && ud.multimode < 2) && sp->pal != 0)
        {
            sp->xrepeat = sp->yrepeat = 0;
            changespritestat(i, STAT_MISC);
        }
        else
        {
            sp->pal = 0;
            if (!(sp->picnum == CAMERAPOLE || sp->picnum == GENERICPOLE))
            {
                sp->picnum = CAMERA1;
                changespritestat(i, STAT_ACTOR);
            }
        }
    }
    else switch (DYNAMICTILEMAP(sp->picnum))
        {
        default:
            if (G_HaveActor(sp->picnum))
            {
                if (j == -1 && sp->lotag > ud.player_skill)
                {
                    sp->xrepeat=sp->yrepeat=0;
                    changespritestat(i, STAT_MISC);
                    break;
                }

                //  Init the size
                if (sp->xrepeat == 0 || sp->yrepeat == 0)
                    sp->xrepeat = sp->yrepeat = 1;

                if (A_CheckSpriteFlags(i, SFLAG_BADGUY))
                {
                    if (ud.monsters_off == 1)
                    {
                        sp->xrepeat=sp->yrepeat=0;
                        changespritestat(i, STAT_MISC);
                        break;
                    }

                    A_Fall(i);

                    if (A_CheckSpriteFlags(i, SFLAG_BADGUYSTAYPUT))
                        actor[i].actorstayput = sp->sectnum;

                    g_player[myconnectindex].ps->max_actors_killed++;
                    sp->clipdist = 80;
                    if (j >= 0)
                    {
                        if (sprite[j].picnum == RESPAWN)
                            actor[i].tempang = sprite[i].pal = sprite[j].pal;
                        changespritestat(i, STAT_ACTOR);
                    }
                    else changespritestat(i, STAT_ZOMBIEACTOR);
                }
                else
                {
                    sp->clipdist = 40;
                    sp->owner = i;
                    changespritestat(i, STAT_ACTOR);
                }

                actor[i].timetosleep = 0;

                if (j >= 0)
                    sp->ang = sprite[j].ang;
            }
            break;
        case FOF__STATIC:
            sp->xrepeat = sp->yrepeat = 0;
            changespritestat(i, STAT_MISC);
            break;
        case WATERSPLASH2__STATIC:
            if (j >= 0)
            {
                setsprite(i,(vec3_t *)&sprite[j]);
                sp->xrepeat = sp->yrepeat = 8+(krand()&7);
            }
            else sp->xrepeat = sp->yrepeat = 16+(krand()&15);

            sp->shade = -16;
            sp->cstat |= 128;
            if (j >= 0)
            {
                if (sector[sprite[j].sectnum].lotag == ST_2_UNDERWATER)
                {
                    sp->z = getceilzofslope(SECT,SX,SY)+(16<<8);
                    sp->cstat |= 8;
                }
                else if (sector[sprite[j].sectnum].lotag == ST_1_ABOVE_WATER)
                    sp->z = getflorzofslope(SECT,SX,SY);
            }

            if (sector[sect].floorpicnum == FLOORSLIME ||
                    sector[sect].ceilingpicnum == FLOORSLIME)
                sp->pal = 7;
        case DOMELITE__STATIC:
            if (sp->picnum == DOMELITE)
                sp->cstat |= 257;
        case NEON1__STATIC:
        case NEON2__STATIC:
        case NEON3__STATIC:
        case NEON4__STATIC:
        case NEON5__STATIC:
        case NEON6__STATIC:
            if (sp->picnum != WATERSPLASH2)
                sp->cstat |= 257;
        case NUKEBUTTON__STATIC:
        case JIBS1__STATIC:
        case JIBS2__STATIC:
        case JIBS3__STATIC:
        case JIBS4__STATIC:
        case JIBS5__STATIC:
        case JIBS6__STATIC:
        case HEADJIB1__STATIC:
        case ARMJIB1__STATIC:
        case LEGJIB1__STATIC:
        case LIZMANHEAD1__STATIC:
        case LIZMANARM1__STATIC:
        case LIZMANLEG1__STATIC:
        case DUKETORSO__STATIC:
        case DUKEGUN__STATIC:
        case DUKELEG__STATIC:
            changespritestat(i, STAT_MISC);
            break;
        case TONGUE__STATIC:
            if (j >= 0)
                sp->ang = sprite[j].ang;
            sp->z -= 38<<8;
            sp->zvel = 256-(krand()&511);
            sp->xvel = 64-(krand()&127);
            changespritestat(i, STAT_PROJECTILE);
            break;
        case NATURALLIGHTNING__STATIC:
            sp->cstat &= ~257;
            sp->cstat |= 32768;
            break;
        case TRANSPORTERSTAR__STATIC:
        case TRANSPORTERBEAM__STATIC:
            if (j == -1) break;
            if (sp->picnum == TRANSPORTERBEAM)
            {
                sp->xrepeat = 31;
                sp->yrepeat = 1;
                sp->z = sector[sprite[j].sectnum].floorz-PHEIGHT;
            }
            else
            {
                if (sprite[j].statnum == STAT_PROJECTILE)
                    sp->xrepeat = sp->yrepeat = 8;
                else
                {
                    sp->xrepeat = 48;
                    sp->yrepeat = 64;
                    if (sprite[j].statnum == STAT_PLAYER || A_CheckEnemySprite(&sprite[j]))
                        sp->z -= (32<<8);
                }
            }

            sp->shade = -127;
            sp->cstat = 128|2;
            sp->ang = sprite[j].ang;

            sp->xvel = 128;
            changespritestat(i, STAT_MISC);
            A_SetSprite(i,CLIPMASK0);
            setsprite(i,(vec3_t *)sp);
            break;

        case FRAMEEFFECT1_13__STATIC:
            if (PLUTOPAK) break;
        case FRAMEEFFECT1__STATIC:
            if (j >= 0)
            {
                sp->xrepeat = sprite[j].xrepeat;
                sp->yrepeat = sprite[j].yrepeat;
                T2 = sprite[j].picnum;
            }
            else sp->xrepeat = sp->yrepeat = 0;

            changespritestat(i, STAT_MISC);

            break;

        case LASERLINE__STATIC:
            sp->yrepeat = 6;
            sp->xrepeat = 32;

            if (g_tripbombLaserMode == 1)
                sp->cstat = 16 + 2;
            else if (g_tripbombLaserMode == 0 || g_tripbombLaserMode == 2)
                sp->cstat = 16;
            else
            {
                sp->xrepeat = 0;
                sp->yrepeat = 0;
            }

            if (j >= 0) sp->ang = actor[j].t_data[5]+512;
            changespritestat(i, STAT_MISC);
            break;

        case FORCESPHERE__STATIC:
            if (j == -1)
            {
                sp->cstat = 32768;
                changespritestat(i, STAT_ZOMBIEACTOR);
            }
            else
            {
                sp->xrepeat = sp->yrepeat = 1;
                changespritestat(i, STAT_MISC);
            }
            break;

        case BLOOD__STATIC:
            sp->xrepeat = sp->yrepeat = 16;
            sp->z -= (26<<8);
            if (j >= 0 && sprite[j].pal == 6)
                sp->pal = 6;
            changespritestat(i, STAT_MISC);
            break;
        case BLOODPOOL__STATIC:
        case PUKE__STATIC:
        {
            int16_t s1;
            s1 = sp->sectnum;

            updatesector(sp->x+108,sp->y+108,&s1);
            if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
            {
                updatesector(sp->x-108,sp->y-108,&s1);
                if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
                {
                    updatesector(sp->x+108,sp->y-108,&s1);
                    if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
                    {
                        updatesector(sp->x-108,sp->y+108,&s1);
                        if (s1 >= 0 && sector[s1].floorz != sector[sp->sectnum].floorz)
                        {
                            sp->xrepeat = sp->yrepeat = 0;
                            changespritestat(i, STAT_MISC);
                            break;
                        }

                    }
                    else
                    {
                        sp->xrepeat = sp->yrepeat = 0;
                        changespritestat(i, STAT_MISC);
                        break;
                    }

                }
                else
                {
                    sp->xrepeat = sp->yrepeat = 0;
                    changespritestat(i, STAT_MISC);
                    break;
                }

            }
            else
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i, STAT_MISC);
                break;
            }

        }

        if (sector[SECT].lotag == ST_1_ABOVE_WATER)
        {
            changespritestat(i, STAT_MISC);
            break;
        }

        if (j >= 0 && sp->picnum != PUKE)
        {
            if (sprite[j].pal == 1)
                sp->pal = 1;
            else if (sprite[j].pal != 6 && sprite[j].picnum != NUKEBARREL && sprite[j].picnum != TIRE)
            {
                if (sprite[j].picnum == FECES)
                    sp->pal = 7; // Brown
                else sp->pal = 2; // Red
            }
            else sp->pal = 0;  // green

            if (sprite[j].picnum == TIRE)
                sp->shade = 127;
        }
        sp->cstat |= 32;
        case FECES__STATIC:
            if (j >= 0)
                sp->xrepeat = sp->yrepeat = 1;
            changespritestat(i, STAT_MISC);
            break;

        case BLOODSPLAT1__STATIC:
        case BLOODSPLAT2__STATIC:
        case BLOODSPLAT3__STATIC:
        case BLOODSPLAT4__STATIC:
            sp->cstat |= 16;
            sp->xrepeat = 7+(krand()&7);
            sp->yrepeat = 7+(krand()&7);
            sp->z += (tilesiz[sp->picnum].y*sp->yrepeat)>>2;
            if (j >= 0 && sprite[j].pal == 6)
                sp->pal = 6;
            A_AddToDeleteQueue(i);
            changespritestat(i, STAT_MISC);
            break;

        case TRIPBOMB__STATIC:
            if (sp->lotag > ud.player_skill)
            {
                sp->xrepeat=sp->yrepeat=0;
                changespritestat(i, STAT_MISC);
                break;
            }

            sp->xrepeat=4;
            sp->yrepeat=5;

            sp->owner = sp->hitag = i;

            sp->xvel = 16;
            A_SetSprite(i,CLIPMASK0);
            actor[i].t_data[0] = 17;
            actor[i].t_data[2] = 0;
            actor[i].t_data[5] = sp->ang;
            changespritestat(i, STAT_ZOMBIEACTOR);
            break;

        case SPACEMARINE__STATIC:
            sp->extra = 20;
            sp->cstat |= 257;
            changespritestat(i, STAT_ZOMBIEACTOR);
            break;

        case HYDRENT__STATIC:
        case PANNEL1__STATIC:
        case PANNEL2__STATIC:
        case SATELITE__STATIC:
        case FUELPOD__STATIC:
        case SOLARPANNEL__STATIC:
        case ANTENNA__STATIC:
        case GRATE1__STATIC:
        case CHAIR1__STATIC:
        case CHAIR2__STATIC:
        case CHAIR3__STATIC:
        case BOTTLE1__STATIC:
        case BOTTLE2__STATIC:
        case BOTTLE3__STATIC:
        case BOTTLE4__STATIC:
        case BOTTLE5__STATIC:
        case BOTTLE6__STATIC:
        case BOTTLE7__STATIC:
        case BOTTLE8__STATIC:
        case BOTTLE10__STATIC:
        case BOTTLE11__STATIC:
        case BOTTLE12__STATIC:
        case BOTTLE13__STATIC:
        case BOTTLE14__STATIC:
        case BOTTLE15__STATIC:
        case BOTTLE16__STATIC:
        case BOTTLE17__STATIC:
        case BOTTLE18__STATIC:
        case BOTTLE19__STATIC:
        case OCEANSPRITE1__STATIC:
        case OCEANSPRITE2__STATIC:
        case OCEANSPRITE3__STATIC:
        case OCEANSPRITE5__STATIC:
        case MONK__STATIC:
        case INDY__STATIC:
        case LUKE__STATIC:
        case JURYGUY__STATIC:
        case SCALE__STATIC:
        case VACUUM__STATIC:
        case FANSPRITE__STATIC:
        case CACTUS__STATIC:
        case CACTUSBROKE__STATIC:
        case HANGLIGHT__STATIC:
        case FETUS__STATIC:
        case FETUSBROKE__STATIC:
        case CAMERALIGHT__STATIC:
        case MOVIECAMERA__STATIC:
        case IVUNIT__STATIC:
        case POT1__STATIC:
        case POT2__STATIC:
        case POT3__STATIC:
        case TRIPODCAMERA__STATIC:
        case SUSHIPLATE1__STATIC:
        case SUSHIPLATE2__STATIC:
        case SUSHIPLATE3__STATIC:
        case SUSHIPLATE4__STATIC:
        case SUSHIPLATE5__STATIC:
        case WAITTOBESEATED__STATIC:
        case VASE__STATIC:
        case PIPE1__STATIC:
        case PIPE2__STATIC:
        case PIPE3__STATIC:
        case PIPE4__STATIC:
        case PIPE5__STATIC:
        case PIPE6__STATIC:
            sp->clipdist = 32;
            sp->cstat |= 257;
        case OCEANSPRITE4__STATIC:
            changespritestat(i, STAT_DEFAULT);
            break;
        case FEMMAG1__STATIC:
        case FEMMAG2__STATIC:
            sp->cstat &= ~257;
            changespritestat(i, STAT_DEFAULT);
            break;
        case DUKETAG__STATIC:
        case SIGN1__STATIC:
        case SIGN2__STATIC:
            if ((!g_netServer && ud.multimode < 2) && sp->pal)
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i, STAT_MISC);
            }
            else sp->pal = 0;
            break;
        case MASKWALL1__STATIC:
        case MASKWALL2__STATIC:
        case MASKWALL3__STATIC:
        case MASKWALL4__STATIC:
        case MASKWALL5__STATIC:
        case MASKWALL6__STATIC:
        case MASKWALL7__STATIC:
        case MASKWALL8__STATIC:
        case MASKWALL9__STATIC:
        case MASKWALL10__STATIC:
        case MASKWALL11__STATIC:
        case MASKWALL12__STATIC:
        case MASKWALL13__STATIC:
        case MASKWALL14__STATIC:
        case MASKWALL15__STATIC:
            j = sp->cstat & SPAWN_PROTECT_CSTAT_MASK;
            sp->cstat = j|CSTAT_SPRITE_BLOCK;
            changespritestat(i, STAT_DEFAULT);
            break;
        case FOOTPRINTS__STATIC:
        case FOOTPRINTS2__STATIC:
        case FOOTPRINTS3__STATIC:
        case FOOTPRINTS4__STATIC:
            if (j >= 0)
            {
                int16_t s1 = sp->sectnum;

                updatesector(sp->x+84,sp->y+84,&s1);
                if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
                {
                    updatesector(sp->x-84,sp->y-84,&s1);
                    if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
                    {
                        updatesector(sp->x+84,sp->y-84,&s1);
                        if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
                        {
                            updatesector(sp->x-84,sp->y+84,&s1);
                            if (s1 >= 0 && sector[s1].floorz != sector[sp->sectnum].floorz)
                            {
                                sp->xrepeat = sp->yrepeat = 0;
                                changespritestat(i, STAT_MISC);
                                break;
                            }
                        }
                        else
                        {
                            sp->xrepeat = sp->yrepeat = 0;
                            break;
                        }
                    }
                    else
                    {
                        sp->xrepeat = sp->yrepeat = 0;
                        break;
                    }
                }
                else
                {
                    sp->xrepeat = sp->yrepeat = 0;
                    break;
                }

                sp->cstat = 32+((g_player[P_Get(j)].ps->footprintcount&1)<<2);
                sp->ang = sprite[j].ang;
            }

            sp->z = sector[sect].floorz;
            if (sector[sect].lotag != ST_1_ABOVE_WATER && sector[sect].lotag != ST_2_UNDERWATER)
                sp->xrepeat = sp->yrepeat = 32;

            A_AddToDeleteQueue(i);
            changespritestat(i, STAT_MISC);
            break;

        case PODFEM1__STATIC:
            sp->extra <<= 1;
        case FEM1__STATIC:
        case FEM2__STATIC:
        case FEM3__STATIC:
        case FEM4__STATIC:
        case FEM5__STATIC:
        case FEM6__STATIC:
        case FEM7__STATIC:
        case FEM8__STATIC:
        case FEM9__STATIC:
        case FEM10__STATIC:
        case NAKED1__STATIC:
        case STATUE__STATIC:
        case TOUGHGAL__STATIC:
            sp->yvel = sp->hitag;
            sp->hitag = -1;
        case BLOODYPOLE__STATIC:
            sp->cstat |= 257;
            sp->clipdist = 32;
            changespritestat(i, STAT_ZOMBIEACTOR);
            break;

        case QUEBALL__STATIC:
        case STRIPEBALL__STATIC:
            sp->cstat = 256;
            sp->clipdist = 8;
            changespritestat(i, STAT_ZOMBIEACTOR);
            break;

        case DUKELYINGDEAD__STATIC:
            if (j >= 0 && sprite[j].picnum == APLAYER)
            {
                sp->xrepeat = sprite[j].xrepeat;
                sp->yrepeat = sprite[j].yrepeat;
                sp->shade = sprite[j].shade;
                sp->pal = g_player[P_Get(j)].ps->palookup;
            }
        case DUKECAR__STATIC:
        case HELECOPT__STATIC:
            //                if(sp->picnum == HELECOPT || sp->picnum == DUKECAR) sp->xvel = 1024;
            sp->cstat = 0;
            sp->extra = 1;
            sp->xvel = 292;
            sp->zvel = 360;
        case BLIMP__STATIC:
            sp->cstat |= 257;
            sp->clipdist = 128;
            changespritestat(i, STAT_ACTOR);
            break;

        case RESPAWNMARKERRED__STATIC:
            sp->xrepeat = sp->yrepeat = 24;
            if (j >= 0) sp->z = actor[j].floorz; // -(1<<4);
            changespritestat(i, STAT_ACTOR);
            break;

        case MIKE__STATIC:
            sp->yvel = sp->hitag;
            sp->hitag = 0;
            changespritestat(i, STAT_ACTOR);
            break;
        case WEATHERWARN__STATIC:
            changespritestat(i, STAT_ACTOR);
            break;

        case SPOTLITE__STATIC:
            T1 = sp->x;
            T2 = sp->y;
            break;
        case BULLETHOLE__STATIC:
            sp->xrepeat = sp->yrepeat = 3;
            sp->cstat = 16+(krand()&12);
            A_AddToDeleteQueue(i);
            changespritestat(i, STAT_MISC);
            break;

        case MONEY__STATIC:
        case MAIL__STATIC:
        case PAPER__STATIC:
            actor[i].t_data[0] = krand()&2047;
            sp->cstat = krand()&12;
            sp->xrepeat = sp->yrepeat = 8;
            sp->ang = krand()&2047;
            changespritestat(i, STAT_MISC);
            break;

        case VIEWSCREEN__STATIC:
        case VIEWSCREEN2__STATIC:
            sp->owner = i;
            sp->lotag = sp->extra = 1;
            changespritestat(i, STAT_STANDABLE);
            break;

        case SHELL__STATIC: //From the player
        case SHOTGUNSHELL__STATIC:
            if (j >= 0)
            {
                int32_t a;

                if (sprite[j].picnum == APLAYER)
                {
                    int32_t snum = P_Get(j);
                    const DukePlayer_t *const ps = g_player[snum].ps;

                    a = ps->ang-(krand()&63)+8;  //Fine tune

                    T1 = krand()&1;
                    sp->z = (3<<8) + ps->pyoff + ps->pos.z - ((ps->horizoff + ps->horiz-100)<<4);
                    if (sp->picnum == SHOTGUNSHELL)
                        sp->z += (3<<8);
                    sp->zvel = -(krand()&255);
                }
                else
                {
                    a = sp->ang;
                    sp->z = sprite[j].z-PHEIGHT+(3<<8);
                }

                sp->x = sprite[j].x+(sintable[(a+512)&2047]>>7);
                sp->y = sprite[j].y+(sintable[a&2047]>>7);

                sp->shade = -8;

                if (sp->yvel == 1 || NAM_WW2GI)
                {
                    sp->ang = a+512;
                    sp->xvel = 30;
                }
                else
                {
                    sp->ang = a-512;
                    sp->xvel = 20;
                }
                sp->xrepeat=sp->yrepeat=4;

                changespritestat(i, STAT_MISC);
            }
            break;

        case RESPAWN__STATIC:
            sp->extra = 66-13;
        case MUSICANDSFX__STATIC:
            if ((!g_netServer && ud.multimode < 2) && sp->pal == 1)
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i, STAT_MISC);
                break;
            }
            sp->cstat = 32768;
            changespritestat(i, STAT_FX);
            break;

        case EXPLOSION2__STATIC:
            if (sp->yrepeat > 32)
            {
                G_AddGameLight(0, i, ((sp->yrepeat*tilesiz[sp->picnum].y)<<1), 32768, 255+(95<<8),PR_LIGHT_PRIO_MAX_GAME, 65536);
                actor[i].lightcount = 2;
            }
        case EXPLOSION2BOT__STATIC:
        case BURNING__STATIC:
        case BURNING2__STATIC:
        case SMALLSMOKE__STATIC:
        case SHRINKEREXPLOSION__STATIC:
        case COOLEXPLOSION1__STATIC:

            if (j >= 0)
            {
                sp->ang = sprite[j].ang;
                sp->shade = -64;
                sp->cstat = 128|(krand()&4);
            }

            if (sp->picnum == EXPLOSION2 || sp->picnum == EXPLOSION2BOT)
            {
                sp->xrepeat = sp->yrepeat = 48;
                sp->shade = -127;
                sp->cstat |= 128;
            }
            else if (sp->picnum == SHRINKEREXPLOSION)
                sp->xrepeat = sp->yrepeat = 32;
            else if (sp->picnum == SMALLSMOKE)
            {
                // 64 "money"
                sp->xrepeat = sp->yrepeat = 24;
            }
            else if (sp->picnum == BURNING || sp->picnum == BURNING2)
                sp->xrepeat = sp->yrepeat = 4;

            sp->cstat |= 8192;

            if (j >= 0)
            {
                int32_t z = getflorzofslope(sp->sectnum,sp->x,sp->y);
                if (sp->z > z-(12<<8))
                    sp->z = z-(12<<8);
            }

            changespritestat(i, STAT_MISC);

            break;

        case PLAYERONWATER__STATIC:
            if (j >= 0)
            {
                sp->xrepeat = sprite[j].xrepeat;
                sp->yrepeat = sprite[j].yrepeat;
                sp->zvel = 128;
                if (sector[sp->sectnum].lotag != ST_2_UNDERWATER)
                    sp->cstat |= 32768;
            }
            changespritestat(i, STAT_DUMMYPLAYER);
            break;

        case APLAYER__STATIC:
            sp->xrepeat = sp->yrepeat = 0;
            sp->cstat = 32768;
            if ((!g_netServer && ud.multimode < 2) ||
                    ((GametypeFlags[ud.coop] & GAMETYPE_COOPSPAWN)/GAMETYPE_COOPSPAWN) != sp->lotag)
                changespritestat(i,STAT_MISC);
            else
                changespritestat(i,STAT_PLAYER);
            break;
        case WATERBUBBLE__STATIC:
            if (j >= 0 && sprite[j].picnum == APLAYER)
                sp->z -= (16<<8);
            if (sp->picnum == WATERBUBBLE)
            {
                if (j >= 0)
                    sp->ang = sprite[j].ang;
                sp->xrepeat = sp->yrepeat = 4;
            }
            else sp->xrepeat = sp->yrepeat = 32;

            changespritestat(i, STAT_MISC);
            break;

        case CRANE__STATIC:

            sp->cstat |= 64|257;

            sp->picnum += 2;
            sp->z = sector[sect].ceilingz+(48<<8);
            T5 = tempwallptr;

            g_origins[tempwallptr] = *(vec2_t *) sp;
            g_origins[tempwallptr+2].x = sp->z;

            s = headspritestat[STAT_DEFAULT];
            while (s >= 0)
            {
                if (sprite[s].picnum == CRANEPOLE && SHT == (sprite[s].hitag))
                {
                    g_origins[tempwallptr+2].y = s;

                    T2 = sprite[s].sectnum;

                    sprite[s].xrepeat = 48;
                    sprite[s].yrepeat = 128;

                    g_origins[tempwallptr+1] = *(vec2_t *)&sprite[s];
                    *(vec3_t *) &sprite[s] = *(vec3_t *) sp;
                    sprite[s].shade = sp->shade;

                    setsprite(s,(vec3_t *)&sprite[s]);
                    break;
                }
                s = nextspritestat[s];
            }

            tempwallptr += 3;
            sp->owner = -1;
            sp->extra = 8;
            changespritestat(i, STAT_STANDABLE);
            break;

        case TRASH__STATIC:
            sp->ang = krand()&2047;
            sp->xrepeat = sp->yrepeat = 24;
            changespritestat(i, STAT_STANDABLE);
            break;

        case WATERDRIP__STATIC:
            if (j >= 0 && (sprite[j].statnum == STAT_PLAYER || sprite[j].statnum == STAT_ACTOR))
            {
                sp->shade = 32;
                if (sprite[j].pal != 1)
                {
                    sp->pal = 2;
                    sp->z -= (18<<8);
                }
                else sp->z -= (13<<8);
                sp->ang = getangle(g_player[0].ps->pos.x-sp->x,g_player[0].ps->pos.y-sp->y);
                sp->xvel = 48-(krand()&31);
                A_SetSprite(i,CLIPMASK0);
            }
            else if (j == -1)
            {
                sp->z += (4<<8);
                T1 = sp->z;
                T2 = krand()&127;
            }
        case WATERDRIPSPLASH__STATIC:
            sp->xrepeat = sp->yrepeat = 24;
            changespritestat(i, STAT_STANDABLE);
            break;

        case PLUG__STATIC:
            sp->lotag = 9999;
            changespritestat(i, STAT_STANDABLE);
            break;
        case TOUCHPLATE__STATIC:
            T3 = sector[sect].floorz;
            if (sector[sect].lotag != ST_1_ABOVE_WATER && sector[sect].lotag != ST_2_UNDERWATER)
                sector[sect].floorz = sp->z;
            if (sp->pal && (g_netServer || ud.multimode > 1))
            {
                sp->xrepeat=sp->yrepeat=0;
                changespritestat(i, STAT_MISC);
                break;
            }
        case WATERBUBBLEMAKER__STATIC:
            if (EDUKE32_PREDICT_FALSE(sp->hitag && sp->picnum == WATERBUBBLEMAKER))
            {
                // JBF 20030913: Pisses off X_Move(), eg. in bobsp2
                OSD_Printf(OSD_ERROR "WARNING: WATERBUBBLEMAKER %d @ %d,%d with hitag!=0. Applying fixup.\n",
                           i,TrackerCast(sp->x),TrackerCast(sp->y));
                sp->hitag = 0;
            }
            sp->cstat |= 32768;
            changespritestat(i, STAT_STANDABLE);
            break;
        case MASTERSWITCH__STATIC:
            if (sp->picnum == MASTERSWITCH)
                sp->cstat |= 32768;
            sp->yvel = 0;
            changespritestat(i, STAT_STANDABLE);
            break;
        case TARGET__STATIC:
        case DUCK__STATIC:
        case LETTER__STATIC:
            sp->extra = 1;
            sp->cstat |= 257;
            changespritestat(i, STAT_ACTOR);
            break;
        case OCTABRAINSTAYPUT__STATIC:
        case LIZTROOPSTAYPUT__STATIC:
        case PIGCOPSTAYPUT__STATIC:
        case LIZMANSTAYPUT__STATIC:
        case BOSS1STAYPUT__STATIC:
        case PIGCOPDIVE__STATIC:
        case COMMANDERSTAYPUT__STATIC:
        case BOSS4STAYPUT__STATIC:
            actor[i].actorstayput = sp->sectnum;
        case BOSS1__STATIC:
        case BOSS2__STATIC:
        case BOSS3__STATIC:
        case BOSS4__STATIC:
        case ROTATEGUN__STATIC:
        case GREENSLIME__STATIC:
            if (sp->picnum == GREENSLIME)
                sp->extra = 1;
        case DRONE__STATIC:
        case LIZTROOPONTOILET__STATIC:
        case LIZTROOPJUSTSIT__STATIC:
        case LIZTROOPSHOOT__STATIC:
        case LIZTROOPJETPACK__STATIC:
        case LIZTROOPDUCKING__STATIC:
        case LIZTROOPRUNNING__STATIC:
        case LIZTROOP__STATIC:
        case OCTABRAIN__STATIC:
        case COMMANDER__STATIC:
        case PIGCOP__STATIC:
        case LIZMAN__STATIC:
        case LIZMANSPITTING__STATIC:
        case LIZMANFEEDING__STATIC:
        case LIZMANJUMP__STATIC:
        case ORGANTIC__STATIC:
        case RAT__STATIC:
        case SHARK__STATIC:

            if (sp->pal == 0)
            {
                switch (DYNAMICTILEMAP(sp->picnum))
                {
                case LIZTROOPONTOILET__STATIC:
                case LIZTROOPSHOOT__STATIC:
                case LIZTROOPJETPACK__STATIC:
                case LIZTROOPDUCKING__STATIC:
                case LIZTROOPRUNNING__STATIC:
                case LIZTROOPSTAYPUT__STATIC:
                case LIZTROOPJUSTSIT__STATIC:
                case LIZTROOP__STATIC:
                    sp->pal = 22;
                    break;
                }
            }
            else
            {
                if (!PLUTOPAK)
                    sp->extra <<= 1;
            }

            if (sp->picnum == BOSS4STAYPUT || sp->picnum == BOSS1 || sp->picnum == BOSS2 ||
                sp->picnum == BOSS1STAYPUT || sp->picnum == BOSS3 || sp->picnum == BOSS4)
            {
                if (j >= 0 && sprite[j].picnum == RESPAWN)
                    sp->pal = sprite[j].pal;
                if (sp->pal)
                {
                    sp->clipdist = 80;
                    sp->xrepeat = sp->yrepeat = 40;
                }
                else
                {
                    sp->xrepeat = sp->yrepeat = 80;
                    sp->clipdist = 164;
                }
            }
            else
            {
                if (sp->picnum != SHARK)
                {
                    sp->xrepeat = sp->yrepeat = 40;
                    sp->clipdist = 80;
                }
                else
                {
                    sp->xrepeat = sp->yrepeat = 60;
                    sp->clipdist = 40;
                }
            }

            // If spawned from parent sprite (as opposed to 'from premap'),
            // ignore skill.
            if (j >= 0) sp->lotag = 0;

            if ((sp->lotag > ud.player_skill) || ud.monsters_off == 1)
            {
                sp->xrepeat=sp->yrepeat=0;
                changespritestat(i, STAT_MISC);
                break;
            }
            else
            {
                A_Fall(i);

                if (sp->picnum == RAT)
                {
                    sp->ang = krand()&2047;
                    sp->xrepeat = sp->yrepeat = 48;
                    sp->cstat = 0;
                }
                else
                {
                    sp->cstat |= 257;

                    if (sp->picnum != SHARK)
                        g_player[myconnectindex].ps->max_actors_killed++;
                }

                if (sp->picnum == ORGANTIC) sp->cstat |= 128;

                if (j >= 0)
                {
                    actor[i].timetosleep = 0;
                    A_PlayAlertSound(i);
                    changespritestat(i, STAT_ACTOR);
                }
                else changespritestat(i, STAT_ZOMBIEACTOR);
            }

            if (sp->picnum == ROTATEGUN)
                sp->zvel = 0;

            break;

        case LOCATORS__STATIC:
            sp->cstat |= 32768;
            changespritestat(i, STAT_LOCATOR);
            break;

        case ACTIVATORLOCKED__STATIC:
        case ACTIVATOR__STATIC:
            sp->cstat = 32768;
            if (sp->picnum == ACTIVATORLOCKED)
                sector[sp->sectnum].lotag |= 16384;
            changespritestat(i, STAT_ACTIVATOR);
            break;

        case DOORSHOCK__STATIC:
            sp->cstat |= 1+256;
            sp->shade = -12;
            changespritestat(i, STAT_STANDABLE);
            break;

        case OOZ__STATIC:
        case OOZ2__STATIC:
            sp->shade = -12;

            if (j >= 0)
            {
                if (sprite[j].picnum == NUKEBARREL)
                    sp->pal = 8;
                A_AddToDeleteQueue(i);
            }

            changespritestat(i, STAT_ACTOR);

            A_GetZLimits(i);

            j = (actor[i].floorz-actor[i].ceilingz)>>9;

            sp->yrepeat = j;
            sp->xrepeat = 25-(j>>1);
            sp->cstat |= (krand()&4);

            break;

        case REACTOR2__STATIC:
        case REACTOR__STATIC:
            sp->extra = g_impactDamage;
            CS |= 257;
            if ((!g_netServer && ud.multimode < 2) && sp->pal != 0)
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i, STAT_MISC);
                break;
            }
            sp->pal = 0;
            SS = -17;

            changespritestat(i, STAT_ZOMBIEACTOR);
            break;

        case HEAVYHBOMB__STATIC:
            if (j >= 0)
                sp->owner = j;
            else sp->owner = i;

            sp->xrepeat = sp->yrepeat = 9;
            sp->yvel = 4;
            CS |= 257;

            if ((!g_netServer && ud.multimode < 2) && sp->pal != 0)
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i, STAT_MISC);
                break;
            }
            sp->pal = 0;
            SS = -17;

            changespritestat(i, STAT_ZOMBIEACTOR);
            break;

        case RECON__STATIC:
            if (sp->lotag > ud.player_skill)
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i, STAT_MISC);
                goto SPAWN_END;
            }
            g_player[myconnectindex].ps->max_actors_killed++;
            actor[i].t_data[5] = 0;
            if (ud.monsters_off == 1)
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i, STAT_MISC);
                break;
            }
            sp->extra = 130;
            CS |= 256; // Make it hitable

            if ((!g_netServer && ud.multimode < 2) && sp->pal != 0)
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i, STAT_MISC);
                break;
            }
            sp->pal = 0;
            SS = -17;

            changespritestat(i, STAT_ZOMBIEACTOR);
            break;

        case ATOMICHEALTH__STATIC:
        case STEROIDS__STATIC:
        case HEATSENSOR__STATIC:
        case SHIELD__STATIC:
        case AIRTANK__STATIC:
        case TRIPBOMBSPRITE__STATIC:
        case JETPACK__STATIC:
        case HOLODUKE__STATIC:

        case FIRSTGUNSPRITE__STATIC:
        case CHAINGUNSPRITE__STATIC:
        case SHOTGUNSPRITE__STATIC:
        case RPGSPRITE__STATIC:
        case SHRINKERSPRITE__STATIC:
        case FREEZESPRITE__STATIC:
        case DEVISTATORSPRITE__STATIC:

        case SHOTGUNAMMO__STATIC:
        case FREEZEAMMO__STATIC:
        case HBOMBAMMO__STATIC:
        case CRYSTALAMMO__STATIC:
        case GROWAMMO__STATIC:
        case BATTERYAMMO__STATIC:
        case DEVISTATORAMMO__STATIC:
        case RPGAMMO__STATIC:
        case BOOTS__STATIC:
        case AMMO__STATIC:
        case AMMOLOTS__STATIC:
        case COLA__STATIC:
        case FIRSTAID__STATIC:
        case SIXPAK__STATIC:

            if (j >= 0)
            {
                sp->lotag = 0;
                sp->z -= (32<<8);
                sp->zvel = -1024;
                A_SetSprite(i,CLIPMASK0);
                sp->cstat = krand()&4;
            }
            else
            {
                sp->owner = i;
                sp->cstat = 0;
            }

            if (((!g_netServer && ud.multimode < 2) && sp->pal != 0) || (sp->lotag > ud.player_skill))
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i, STAT_MISC);
                break;
            }

            sp->pal = 0;

        case ACCESSCARD__STATIC:

            if (sp->picnum == ATOMICHEALTH)
                sp->cstat |= 128;

            if ((g_netServer || ud.multimode > 1) && !GTFLAGS(GAMETYPE_ACCESSCARDSPRITES) && sp->picnum == ACCESSCARD)
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i, STAT_MISC);
                break;
            }
            else
            {
                if (sp->picnum == AMMO)
                    sp->xrepeat = sp->yrepeat = 16;
                else sp->xrepeat = sp->yrepeat = 32;
            }

            sp->shade = -17;

            if (j >= 0) changespritestat(i, STAT_ACTOR);
            else
            {
                changespritestat(i, STAT_ZOMBIEACTOR);
                A_Fall(i);
            }
            break;

        case WATERFOUNTAIN__STATIC:
            SLT = 1;

        case TREE1__STATIC:
        case TREE2__STATIC:
        case TIRE__STATIC:
        case CONE__STATIC:
        case BOX__STATIC:
            CS = 257; // Make it hitable
            sprite[i].extra = 1;
            changespritestat(i, STAT_STANDABLE);
            break;

        case FLOORFLAME__STATIC:
            sp->shade = -127;
            changespritestat(i, STAT_STANDABLE);
            break;

        case BOUNCEMINE__STATIC:
            sp->owner = i;
            sp->cstat |= 1+256; //Make it hitable
            sp->xrepeat = sp->yrepeat = 24;
            sp->shade = -127;
            sp->extra = g_impactDamage<<2;
            changespritestat(i, STAT_ZOMBIEACTOR);
            break;

        case STEAM__STATIC:
            if (j >= 0)
            {
                sp->ang = sprite[j].ang;
                sp->cstat = 16+128+2;
                sp->xrepeat=sp->yrepeat=1;
                sp->xvel = -8;
                A_SetSprite(i,CLIPMASK0);
            }
        case CEILINGSTEAM__STATIC:
            changespritestat(i, STAT_STANDABLE);
            break;

        case SECTOREFFECTOR__STATIC:
            sp->cstat |= 32768;
            sp->xrepeat = sp->yrepeat = 0;

            switch (sp->lotag)
            {
#ifdef LEGACY_ROR
            case 40:
            case 41:
                sp->cstat = 32;
                sp->xrepeat = sp->yrepeat = 64;
                changespritestat(i, STAT_EFFECTOR);
                for (j=0; j < MAXSPRITES; j++)
                    if (sprite[j].picnum == SECTOREFFECTOR && (sprite[j].lotag == 40 || sprite[j].lotag == 41) &&
                            sprite[j].hitag == sp->hitag && i != j)
                    {
//                        initprintf("found ror match\n");
                        sp->yvel = j;
                        break;
                    }
                goto SPAWN_END;
                break;
            case 46:
                ror_protectedsectors[sp->sectnum] = 1;
                /* XXX: fall-through intended? */
#endif
            case SE_49_POINT_LIGHT:
            case SE_50_SPOT_LIGHT:
            {
                int32_t j, nextj;

                for (TRAVERSE_SPRITE_SECT(headspritesect[sp->sectnum], j, nextj))
                    if (sprite[j].picnum == ACTIVATOR || sprite[j].picnum == ACTIVATORLOCKED)
                        actor[i].flags |= SFLAG_USEACTIVATOR;
            }
            changespritestat(i, sp->lotag==46 ? STAT_EFFECTOR : STAT_LIGHT);
            goto SPAWN_END;
            break;
            }

            sp->yvel = sector[sect].extra;

            switch (sp->lotag)
            {
            case SE_28_LIGHTNING:
                T6 = 65;// Delay for lightning
                break;
            case SE_7_TELEPORT: // Transporters!!!!
            case SE_23_ONE_WAY_TELEPORT:// XPTR END
                if (sp->lotag != SE_23_ONE_WAY_TELEPORT)
                {
                    for (j=0; j<MAXSPRITES; j++)
                        if (sprite[j].statnum < MAXSTATUS && sprite[j].picnum == SECTOREFFECTOR &&
                                (sprite[j].lotag == SE_7_TELEPORT || sprite[j].lotag == SE_23_ONE_WAY_TELEPORT) && i != j && sprite[j].hitag == SHT)
                        {
                            OW = j;
                            break;
                        }
                }
                else OW = i;

                T5 = (sector[sect].floorz == SZ);  // ONFLOORZ
                sp->cstat = 0;
                changespritestat(i, STAT_TRANSPORT);
                goto SPAWN_END;
            case SE_1_PIVOT:
                sp->owner = -1;
                T1 = 1;
                break;
            case SE_18_INCREMENTAL_SECTOR_RISE_FALL:

                if (sp->ang == 512)
                {
                    T2 = sector[sect].ceilingz;
                    if (sp->pal)
                        sector[sect].ceilingz = sp->z;
                }
                else
                {
                    T2 = sector[sect].floorz;
                    if (sp->pal)
                        sector[sect].floorz = sp->z;
                }

                sp->hitag <<= 2;
                break;

            case SE_19_EXPLOSION_LOWERS_CEILING:
                sp->owner = -1;
                break;
            case SE_25_PISTON: // Pistons
                T4 = sector[sect].ceilingz;
                T5 = 1;
                sector[sect].ceilingz = sp->z;
                G_SetInterpolation(&sector[sect].ceilingz);
                break;
            case SE_35:
                sector[sect].ceilingz = sp->z;
                break;
            case SE_27_DEMO_CAM:
                if (ud.recstat == 1)
                {
                    sp->xrepeat=sp->yrepeat=64;
                    sp->cstat &= 32768;
                }
                break;
            case SE_12_LIGHT_SWITCH:

                T2 = sector[sect].floorshade;
                T3 = sector[sect].ceilingshade;
                break;

            case SE_13_EXPLOSIVE:

                T1 = sector[sect].ceilingz;
                T2 = sector[sect].floorz;

                if (klabs(T1-sp->z) < klabs(T2-sp->z))
                    sp->owner = 1;
                else sp->owner = 0;

                if (sp->ang == 512)
                {
                    if (sp->owner)
                        sector[sect].ceilingz = sp->z;
                    else
                        sector[sect].floorz = sp->z;
#ifdef YAX_ENABLE
                    {
                        int16_t cf=!sp->owner, bn=yax_getbunch(sect, cf);
                        int32_t jj, daz=SECTORFLD(sect,z, cf);

                        if (bn >= 0)
                        {
                            for (SECTORS_OF_BUNCH(bn, cf, jj))
                            {
                                SECTORFLD(jj,z, cf) = daz;
                                SECTORFLD(jj,stat, cf) &= ~256;
                                SECTORFLD(jj,stat, cf) |= 128 + 512+2048;
                            }
                            for (SECTORS_OF_BUNCH(bn, !cf, jj))
                            {
                                SECTORFLD(jj,z, !cf) = daz;
                                SECTORFLD(jj,stat, !cf) &= ~256;
                                SECTORFLD(jj,stat, !cf) |= 128 + 512+2048;
                            }
                        }
                    }
#endif
                }
                else
                    sector[sect].ceilingz = sector[sect].floorz = sp->z;

                if (sector[sect].ceilingstat&1)
                {
                    sector[sect].ceilingstat ^= 1;
                    T4 = 1;

                    if (!sp->owner && sp->ang==512)
                    {
                        sector[sect].ceilingstat ^= 1;
                        T4 = 0;
                    }

                    sector[sect].ceilingshade =
                        sector[sect].floorshade;

                    if (sp->ang==512)
                    {
                        startwall = sector[sect].wallptr;
                        endwall = startwall+sector[sect].wallnum;
                        for (j=startwall; j<endwall; j++)
                        {
                            int32_t x = wall[j].nextsector;
                            if (x >= 0)
                                if (!(sector[x].ceilingstat&1))
                                {
                                    sector[sect].ceilingpicnum =
                                        sector[x].ceilingpicnum;
                                    sector[sect].ceilingshade =
                                        sector[x].ceilingshade;
                                    break; //Leave earily
                                }
                        }
                    }
                }

                break;

            case SE_17_WARP_ELEVATOR:

                T3 = sector[sect].floorz; //Stopping loc

                j = nextsectorneighborz(sect,sector[sect].floorz,-1,-1);

                if (EDUKE32_PREDICT_TRUE(j >= 0))
                    T4 = sector[j].ceilingz;
                else
                {
                    // use elevator sector's ceiling as heuristic
                    T4 = sector[sect].ceilingz;

                    OSD_Printf(OSD_ERROR "WARNING: SE17 sprite %d using own sector's ceilingz to "
                               "determine when to warp. Sector %d adjacent to a door?\n", i, sect);
                }

                j = nextsectorneighborz(sect,sector[sect].ceilingz,1,1);

                if (EDUKE32_PREDICT_TRUE(j >= 0))
                    T5 = sector[j].floorz;
                else
                {
                    // XXX: we should return to the menu for this and similar failures
                    Bsprintf(tempbuf, "SE 17 (warp elevator) setup failed: sprite %d at (%d, %d)",
                             i, TrackerCast(sprite[i].x), TrackerCast(sprite[i].y));
                    G_GameExit(tempbuf);
                }

                if (numplayers < 2 && !g_netServer)
                {
                    G_SetInterpolation(&sector[sect].floorz);
                    G_SetInterpolation(&sector[sect].ceilingz);
                }

                break;

            case SE_24_CONVEYOR:
                sp->yvel <<= 1;
            case SE_36_PROJ_SHOOTER:
                break;

            case SE_20_STRETCH_BRIDGE:
            {
                int32_t x, y, d, q = INT32_MAX;
                int32_t clostest=0;

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                //find the two most clostest wall x's and y's
                for (s=startwall; s<endwall; s++)
                {
                    x = wall[s].x;
                    y = wall[s].y;

                    d = FindDistance2D(sp->x-x,sp->y-y);
                    if (d < q)
                    {
                        q = d;
                        clostest = s;
                    }
                }

                T2 = clostest;

                q = INT32_MAX;

                for (s=startwall; s<endwall; s++)
                {
                    x = wall[s].x;
                    y = wall[s].y;

                    d = FindDistance2D(sp->x-x,sp->y-y);
                    if (d < q && s != T2)
                    {
                        q = d;
                        clostest = s;
                    }
                }

                T3 = clostest;
            }

            break;

            case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:

                T4=sector[sect].floorshade;

                sector[sect].floorshade = sp->shade;
                sector[sect].ceilingshade = sp->shade;

                sp->owner = sector[sect].ceilingpal<<8;
                sp->owner |= sector[sect].floorpal;

                //fix all the walls;

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                for (s=startwall; s<endwall; s++)
                {
                    if (!(wall[s].hitag&1))
                        wall[s].shade=sp->shade;
                    if ((wall[s].cstat&2) && wall[s].nextwall >= 0)
                        wall[wall[s].nextwall].shade = sp->shade;
                }
                break;

            case SE_31_FLOOR_RISE_FALL:
            {
                T2 = sector[sect].floorz;
                //    T3 = sp->hitag;
                if (sp->ang != 1536)
                {
                    sector[sect].floorz = sp->z;
                    Yax_SetBunchZs(sect, YAX_FLOOR, sp->z);
                }

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                for (s=startwall; s<endwall; s++)
                    if (wall[s].hitag == 0) wall[s].hitag = 9999;

                G_SetInterpolation(&sector[sect].floorz);
                Yax_SetBunchInterpolation(sect, YAX_FLOOR);
            }
            break;

            case SE_32_CEILING_RISE_FALL:
            {
                T2 = sector[sect].ceilingz;
                T3 = sp->hitag;
                if (sp->ang != 1536)
                {
                    sector[sect].ceilingz = sp->z;
                    Yax_SetBunchZs(sect, YAX_CEILING, sp->z);
                }

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                for (s=startwall; s<endwall; s++)
                    if (wall[s].hitag == 0) wall[s].hitag = 9999;

                G_SetInterpolation(&sector[sect].ceilingz);
                Yax_SetBunchInterpolation(sect, YAX_CEILING);
            }
            break;

            case SE_4_RANDOM_LIGHTS: //Flashing lights

                T3 = sector[sect].floorshade;

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                sp->owner = sector[sect].ceilingpal<<8;
                sp->owner |= sector[sect].floorpal;

                for (s=startwall; s<endwall; s++)
                    if (wall[s].shade > T4)
                        T4 = wall[s].shade;

                break;

            case SE_9_DOWN_OPEN_DOOR_LIGHTS:
                if (sector[sect].lotag &&
                        labs(sector[sect].ceilingz-sp->z) > 1024)
                    sector[sect].lotag |= 32768; //If its open
            case SE_8_UP_OPEN_DOOR_LIGHTS:
                //First, get the ceiling-floor shade

                T1 = sector[sect].floorshade;
                T2 = sector[sect].ceilingshade;

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                for (s=startwall; s<endwall; s++)
                    if (wall[s].shade > T3)
                        T3 = wall[s].shade;

                T4 = 1; //Take Out;

                break;

            case SE_11_SWINGING_DOOR://Pivitor rotater
                T4 = (sp->ang > 1024) ? 2 : -2;
                /* fall-through */
            case SE_0_ROTATING_SECTOR:
            case SE_2_EARTHQUAKE://Earthquakemakers
            case SE_5://Boss Creature
            case SE_6_SUBWAY://Subway
            case SE_14_SUBWAY_CAR://Caboos
            case SE_15_SLIDING_DOOR://Subwaytype sliding door
            case SE_16_REACTOR://That rotating blocker reactor thing
            case SE_26://ESCELATOR
            case SE_30_TWO_WAY_TRAIN://No rotational subways
                if (sp->lotag == SE_0_ROTATING_SECTOR)
                {
                    if (sector[sect].lotag == ST_30_ROTATE_RISE_BRIDGE)
                    {
                        sprite[i].clipdist = (sp->pal) ? 1 : 0;
                        T4 = sector[sect].floorz;
                        sector[sect].hitag = i;
                    }

                    for (j = MAXSPRITES-1; j>=0; j--)
                    {
                        if (sprite[j].statnum < MAXSTATUS)
                            if (sprite[j].picnum == SECTOREFFECTOR &&
                                    sprite[j].lotag == SE_1_PIVOT &&
                                    sprite[j].hitag == sp->hitag)
                            {
                                if (sp->ang == 512)
                                {
                                    sp->x = sprite[j].x;
                                    sp->y = sprite[j].y;
                                }
                                break;
                            }
                    }
                    if (EDUKE32_PREDICT_FALSE(j == -1))
                    {
                        OSD_Printf(OSD_ERROR "Found lonely Sector Effector (lotag 0) at (%d,%d)\n",
                            TrackerCast(sp->x),TrackerCast(sp->y));
                        changespritestat(i, STAT_ACTOR);
                        goto SPAWN_END;
                    }
                    sp->owner = j;
                }

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                T2 = tempwallptr;
                for (s=startwall; s<endwall; s++)
                {
                    g_origins[tempwallptr].x = wall[s].x-sp->x;
                    g_origins[tempwallptr].y = wall[s].y-sp->y;

                    tempwallptr++;
                    if (EDUKE32_PREDICT_FALSE(tempwallptr >= MAXANIMPOINTS))
                    {
                        Bsprintf(tempbuf, "Too many moving sectors at (%d,%d).\n",
                                        TrackerCast(wall[s].x),TrackerCast(wall[s].y));
                        G_GameExit(tempbuf);
                    }
                }

                if (sp->lotag == SE_5 || sp->lotag == SE_30_TWO_WAY_TRAIN ||
                        sp->lotag == SE_6_SUBWAY || sp->lotag == SE_14_SUBWAY_CAR)
                {
#ifdef YAX_ENABLE
                    int32_t outerwall=-1;
#endif
                    startwall = sector[sect].wallptr;
                    endwall = startwall+sector[sect].wallnum;

                    sp->extra = (sector[sect].hitag != UINT16_MAX);

                    // TRAIN_SECTOR_TO_SE_INDEX
                    sector[sect].hitag = i;

                    j = 0;

                    for (s=startwall; s<endwall; s++)
                    {
                        if (wall[ s ].nextsector >= 0 &&
                                sector[ wall[ s ].nextsector].hitag == 0 &&
                                sector[ wall[ s ].nextsector].lotag < 3)
                        {
#ifdef YAX_ENABLE
                            outerwall = wall[s].nextwall;
#endif
                            s = wall[s].nextsector;
                            j = 1;
                            break;
                        }
                    }
#ifdef YAX_ENABLE
                    actor[i].t_data[9] = -1;

                    if (outerwall >= 0)
                    {
                        int32_t uppersect = yax_vnextsec(outerwall, YAX_CEILING);

                        if (uppersect >= 0)
                        {
                            int32_t jj;
                            for (jj=headspritesect[uppersect]; jj>=0; jj=nextspritesect[jj])
                                if (sprite[jj].picnum==SECTOREFFECTOR && sprite[jj].lotag==sp->lotag)
                                    break;
                            if (jj < 0)
                            {
                                Sect_SetInterpolation(uppersect);
                                actor[i].t_data[9] = uppersect;
                            }
                        }
                    }
#endif
                    if (j == 0)
                    {
                        Bsprintf(tempbuf,"Subway found no zero'd sectors with locators\nat (%d,%d).\n",
                            TrackerCast(sp->x),TrackerCast(sp->y));
                        G_GameExit(tempbuf);
                    }

                    sp->owner = -1;
                    T1 = s;

                    if (sp->lotag != SE_30_TWO_WAY_TRAIN)
                        T4 = sp->hitag;
                }

                else if (sp->lotag == SE_16_REACTOR)
                    T4 = sector[sect].ceilingz;

                else if (sp->lotag == SE_26)
                {
                    T4 = sp->x;
                    T5 = sp->y;
                    if (sp->shade==sector[sect].floorshade) //UP
                        sp->zvel = -256;
                    else
                        sp->zvel = 256;

                    sp->shade = 0;
                }
                else if (sp->lotag == SE_2_EARTHQUAKE)
                {
                    T6 = sector[sp->sectnum].floorheinum;
                    sector[sp->sectnum].floorheinum = 0;
                }
            }

            switch (sp->lotag)
            {
            case SE_6_SUBWAY:
            case SE_14_SUBWAY_CAR:
                S_FindMusicSFX(sect, &j);
                if (j == -1) j = SUBWAY;
                actor[i].lastvx = j;
            case SE_30_TWO_WAY_TRAIN:
                if (g_netServer || numplayers > 1) break;
            case SE_0_ROTATING_SECTOR:
            case SE_1_PIVOT:
            case SE_5:
            case SE_11_SWINGING_DOOR:
            case SE_15_SLIDING_DOOR:
            case SE_16_REACTOR:
            case SE_26:
                Sect_SetInterpolation(sprite[i].sectnum);
                break;
            }

            changespritestat(i, STAT_EFFECTOR);
            break;

        case SEENINE__STATIC:
        case OOZFILTER__STATIC:
            sp->shade = -16;
            if (sp->xrepeat <= 8)
            {
                sp->cstat = 32768;
                sp->xrepeat=sp->yrepeat=0;
            }
            else sp->cstat = 1+256;
            sp->extra = g_impactDamage<<2;
            sp->owner = i;

            changespritestat(i, STAT_STANDABLE);
            break;

        case CRACK1__STATIC:
        case CRACK2__STATIC:
        case CRACK3__STATIC:
        case CRACK4__STATIC:
        case FIREEXT__STATIC:
            if (sp->picnum == FIREEXT)
            {
                sp->cstat = 257;
                sp->extra = g_impactDamage<<2;
            }
            else
            {
                sp->cstat |= (sp->cstat & 48) ? 1 : 17;
                sp->extra = 1;
            }

            if ((!g_netServer && ud.multimode < 2) && sp->pal != 0)
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i, STAT_MISC);
                break;
            }

            sp->pal = 0;
            sp->owner = i;
            changespritestat(i, STAT_STANDABLE);
            sp->xvel = 8;
            A_SetSprite(i,CLIPMASK0);
            break;

        case TOILET__STATIC:
        case STALL__STATIC:
            sp->lotag = 1;
            sp->cstat |= 257;
            sp->clipdist = 8;
            sp->owner = i;
            break;

        case CANWITHSOMETHING__STATIC:
        case CANWITHSOMETHING2__STATIC:
        case CANWITHSOMETHING3__STATIC:
        case CANWITHSOMETHING4__STATIC:
        case RUBBERCAN__STATIC:
            sp->extra = 0;
        case EXPLODINGBARREL__STATIC:
        case HORSEONSIDE__STATIC:
        case FIREBARREL__STATIC:
        case NUKEBARREL__STATIC:
        case FIREVASE__STATIC:
        case NUKEBARRELDENTED__STATIC:
        case NUKEBARRELLEAKED__STATIC:
        case WOODENHORSE__STATIC:
            if (j >= 0)
                sp->xrepeat = sp->yrepeat = 32;
            sp->clipdist = 72;
            A_Fall(i);
            if (j >= 0)
                sp->owner = j;
            else sp->owner = i;
        case EGG__STATIC:
            if (ud.monsters_off == 1 && sp->picnum == EGG)
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i, STAT_MISC);
            }
            else
            {
                if (sp->picnum == EGG)
                    sp->clipdist = 24;
                sp->cstat = 257|(krand()&4);
                changespritestat(i, STAT_ZOMBIEACTOR);
            }
            break;

        case TOILETWATER__STATIC:
            sp->shade = -16;
            changespritestat(i, STAT_STANDABLE);
            break;
        }

SPAWN_END:
    if (VM_HaveEvent(EVENT_SPAWN))
    {
        int32_t p;
        int32_t pl=A_FindPlayer(&sprite[i],&p);
        VM_OnEventWithDist_(EVENT_SPAWN,i, pl, p);
    }

    return i;
}

static int32_t G_MaybeTakeOnFloorPal(tspritetype *datspr, int32_t sect)
{
    int32_t dapal = sector[sect].floorpal;

    if (dapal && !g_noFloorPal[dapal]
            && !A_CheckSpriteFlags(datspr->owner,SFLAG_NOPAL))
    {
        datspr->pal = dapal;
        return 1;
    }

    return 0;
}

static int32_t getofs_viewtype5(const tspritetype *s, tspritetype *t, int32_t a, uint8_t invertp)
{
    int32_t angdif = invertp ? a-s->ang : s->ang-a;
    int32_t k = (((angdif+3072+128)&2047)>>8)&7;

    if (k>4)
    {
        k = 8-k;
        t->cstat |= 4;
    }
    else t->cstat &= ~4;

    return k;
}

static int32_t getofs_viewtype7(const tspritetype *s, tspritetype *t, int32_t a, uint8_t invertp)
{
    int32_t angdif = invertp ? a-s->ang : s->ang-a;
    int32_t k = ((angdif+3072+128)&2047)/170;

    if (k>6)
    {
        k = 12-k;
        t->cstat |= 4;
    }
    else t->cstat &= ~4;

    return k;
}

static int32_t G_CheckAdultTile(int32_t pic)
{
    switch (pic)
    {
    case FEM1__STATIC:
    case FEM2__STATIC:
    case FEM3__STATIC:
    case FEM4__STATIC:
    case FEM5__STATIC:
    case FEM6__STATIC:
    case FEM7__STATIC:
    case FEM8__STATIC:
    case FEM9__STATIC:
    case FEM10__STATIC:
    case MAN__STATIC:
    case MAN2__STATIC:
    case WOMAN__STATIC:
    case NAKED1__STATIC:
    case PODFEM1__STATIC:
    case FEMMAG1__STATIC:
    case FEMMAG2__STATIC:
    case FEMPIC1__STATIC:
    case FEMPIC2__STATIC:
    case FEMPIC3__STATIC:
    case FEMPIC4__STATIC:
    case FEMPIC5__STATIC:
    case FEMPIC6__STATIC:
    case FEMPIC7__STATIC:
    case BLOODYPOLE__STATIC:
    case FEM6PAD__STATIC:
    case STATUE__STATIC:
    case STATUEFLASH__STATIC:
    case OOZ__STATIC:
    case OOZ2__STATIC:
    case WALLBLOOD1__STATIC:
    case WALLBLOOD2__STATIC:
    case WALLBLOOD3__STATIC:
    case WALLBLOOD4__STATIC:
    case WALLBLOOD5__STATIC:
    case WALLBLOOD7__STATIC:
    case WALLBLOOD8__STATIC:
    case SUSHIPLATE1__STATIC:
    case SUSHIPLATE2__STATIC:
    case SUSHIPLATE3__STATIC:
    case SUSHIPLATE4__STATIC:
    case FETUS__STATIC:
    case FETUSJIB__STATIC:
    case FETUSBROKE__STATIC:
    case HOTMEAT__STATIC:
    case FOODOBJECT16__STATIC:
    case DOLPHIN1__STATIC:
    case DOLPHIN2__STATIC:
    case TOUGHGAL__STATIC:
    case TAMPON__STATIC:
    case XXXSTACY__STATIC:
    case 4946:
    case 4947:
    case 693:
    case 2254:
    case 4560:
    case 4561:
    case 4562:
    case 4498:
    case 4957:
        return 1;
    }

    return 0;
}

static inline void G_DoEventAnimSprites(int32_t j)
{
    const int32_t ow = tsprite[j].owner;

    if ((((unsigned)ow >= MAXSPRITES || (spriteext[ow].flags & SPREXT_TSPRACCESS) != SPREXT_TSPRACCESS)) &&
        tsprite[j].statnum != TSPR_TEMP)
        return;

    spriteext[ow].tspr = &tsprite[j];
    VM_OnEvent_(EVENT_ANIMATESPRITES, ow, screenpeek);
    spriteext[ow].tspr = NULL;
}

void G_DoSpriteAnimations(int32_t ourx, int32_t oury, int32_t oura, int32_t smoothratio)
{
    int32_t j, k, p;
    intptr_t l;

    if (spritesortcnt == 0)
    {
#ifdef DEBUGGINGAIDS
        g_spriteStat.numonscreen = 0;
#endif
        return;
    }
#ifdef LEGACY_ROR
    ror_sprite = -1;
#endif
    for (j=spritesortcnt-1; j>=0; j--)
    {
        tspritetype *const t = &tsprite[j];
        const int32_t i = t->owner;
        const spritetype *const s = &sprite[i];

        switch (DYNAMICTILEMAP(s->picnum))
        {
        case SECTOREFFECTOR__STATIC:
            if (s->lotag == 40 || s->lotag == 41)
            {
                t->cstat = 32768;
#ifdef LEGACY_ROR
                if (ror_sprite == -1)
                    ror_sprite = i;
#endif
            }

            if (t->lotag == SE_27_DEMO_CAM && ud.recstat == 1)
            {
                t->picnum = 11+((totalclock>>3)&1);
                t->cstat |= 128;
            }
            else
                t->xrepeat = t->yrepeat = 0;
            break;
        }
    }

    for (j=spritesortcnt-1; j>=0; j--)
    {
        tspritetype *const t = &tsprite[j];
        const int32_t i = t->owner;
        const spritetype *const s = &sprite[i];

        if (t->picnum < GREENSLIME || t->picnum > GREENSLIME+7)
            switch (DYNAMICTILEMAP(t->picnum))
            {
            case BLOODPOOL__STATIC:
            case PUKE__STATIC:
            case FOOTPRINTS__STATIC:
            case FOOTPRINTS2__STATIC:
            case FOOTPRINTS3__STATIC:
            case FOOTPRINTS4__STATIC:
                if (t->shade == 127) continue;
                break;
            case RESPAWNMARKERRED__STATIC:
            case RESPAWNMARKERYELLOW__STATIC:
            case RESPAWNMARKERGREEN__STATIC:
                if (ud.marker == 0)
                    t->xrepeat = t->yrepeat = 0;
                continue;
            case CHAIR3__STATIC:
#ifdef USE_OPENGL
                if (getrendermode() >= REND_POLYMOST && usemodels && md_tilehasmodel(t->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
                {
                    t->cstat &= ~4;
                    break;
                }
#endif
                k = getofs_viewtype5(t, t, oura, 0);
                t->picnum = s->picnum+k;
                break;
            case BLOODSPLAT1__STATIC:
            case BLOODSPLAT2__STATIC:
            case BLOODSPLAT3__STATIC:
            case BLOODSPLAT4__STATIC:
                if (ud.lockout) t->xrepeat = t->yrepeat = 0;
                else if (t->pal == 6)
                {
                    t->shade = -127;
                    continue;
                }
            case BULLETHOLE__STATIC:
            case CRACK1__STATIC:
            case CRACK2__STATIC:
            case CRACK3__STATIC:
            case CRACK4__STATIC:
                t->shade = 16;
                continue;
            case NEON1__STATIC:
            case NEON2__STATIC:
            case NEON3__STATIC:
            case NEON4__STATIC:
            case NEON5__STATIC:
            case NEON6__STATIC:
                continue;
            default:
                // NOTE: wall-aligned sprites will never take on ceiling/floor shade...
                if ((t->cstat&16) || (A_CheckEnemySprite((const spritetype *)t) &&
                    (unsigned)t->owner < MAXSPRITES && sprite[t->owner].extra > 0) || t->statnum == STAT_PLAYER)
                    continue;
            }

        // ... since this is not reached:
        if (A_CheckSpriteFlags(t->owner, SFLAG_NOSHADE) || (t->cstat&CSTAT_SPRITE_NOSHADE))
            l = sprite[t->owner].shade;
        else
        {
            if (sector[t->sectnum].ceilingstat&1)
                l = sector[t->sectnum].ceilingshade;
            else
                l = sector[t->sectnum].floorshade;

            if (l < -127)
                l = -127;
        }

        t->shade = l;
    }

    for (j=spritesortcnt-1; j>=0; j--) //Between drawrooms() and drawmasks()
    {
        int32_t switchpic;
        int32_t curframe;
#if !defined LUNATIC
        int32_t scrofs_action;
#else
        int32_t startframe, viewtype;
#endif
        //is the perfect time to animate sprites
        tspritetype *const t = &tsprite[j];
        const int32_t i = t->owner;
        // XXX: what's up with the (i < 0) check?
        // NOTE: not const spritetype because set at SET_SPRITE_NOT_TSPRITE (see below).
        tspritetype *const s = (i < 0) ? &tsprite[j] : (tspritetype *)&sprite[i];

        if (ud.lockout && G_CheckAdultTile(DYNAMICTILEMAP(s->picnum)))
        {
            t->xrepeat = t->yrepeat = 0;
            continue;
        }

        if (s->picnum == NATURALLIGHTNING)
        {
            t->shade = -127;
            t->cstat |= 8192;
        }

        if (t->statnum == TSPR_TEMP)
            continue;

        Bassert(i >= 0);

        const DukePlayer_t *const ps = (s->statnum != STAT_ACTOR && s->picnum == APLAYER && s->owner >= 0) ? g_player[P_GetP((const spritetype *)s)].ps : NULL;
        if (ps && ps->newowner == -1)
        {
            t->x -= mulscale16(65536-smoothratio,ps->pos.x-ps->opos.x);
            t->y -= mulscale16(65536-smoothratio,ps->pos.y-ps->opos.y);
            // dirty hack
            if (ps->dead_flag) t->z = ps->opos.z;
            t->z += mulscale16(smoothratio,ps->pos.z-ps->opos.z) -
                (ps->dead_flag ? 0 : PHEIGHT) + PHEIGHT;
        }
        else if ((s->statnum == STAT_DEFAULT && s->picnum != CRANEPOLE) || s->statnum == STAT_PLAYER ||
                 s->statnum == STAT_STANDABLE || s->statnum == STAT_PROJECTILE || s->statnum == STAT_MISC || s->statnum == STAT_ACTOR)
        {
            t->x -= mulscale16(65536-smoothratio,s->x-actor[i].bpos.x);
            t->y -= mulscale16(65536-smoothratio,s->y-actor[i].bpos.y);
            t->z -= mulscale16(65536-smoothratio,s->z-actor[i].bpos.z);
        }

        const int32_t sect = s->sectnum;

        curframe = AC_CURFRAME(actor[i].t_data);
#if !defined LUNATIC
        scrofs_action = AC_ACTION_ID(actor[i].t_data);
#else
        startframe = actor[i].ac.startframe;
        viewtype = actor[i].ac.viewtype;
#endif
        switchpic = s->picnum;
        // Some special cases because dynamictostatic system can't handle
        // addition to constants.
        if ((s->picnum >= SCRAP6) && (s->picnum<=SCRAP6+7))
            switchpic = SCRAP5;
        else if ((s->picnum==MONEY+1) || (s->picnum==MAIL+1) || (s->picnum==PAPER+1))
            switchpic--;

        switch (DYNAMICTILEMAP(switchpic))
        {
        case DUKELYINGDEAD__STATIC:
            t->z += (24<<8);
            break;
        case BLOODPOOL__STATIC:
        case FOOTPRINTS__STATIC:
        case FOOTPRINTS2__STATIC:
        case FOOTPRINTS3__STATIC:
        case FOOTPRINTS4__STATIC:
            if (t->pal == 6)
                t->shade = -127;
        case PUKE__STATIC:
        case MONEY__STATIC:
            //case MONEY+1__STATIC:
        case MAIL__STATIC:
            //case MAIL+1__STATIC:
        case PAPER__STATIC:
            //case PAPER+1__STATIC:
            if (ud.lockout && s->pal == 2)
            {
                t->xrepeat = t->yrepeat = 0;
                continue;
            }
            break;
        case TRIPBOMB__STATIC:
            continue;
        case FORCESPHERE__STATIC:
            if (t->statnum == STAT_MISC)
            {
                int16_t sqa,sqb;

                sqa =
                    getangle(
                        sprite[s->owner].x-g_player[screenpeek].ps->pos.x,
                        sprite[s->owner].y-g_player[screenpeek].ps->pos.y);
                sqb =
                    getangle(
                        sprite[s->owner].x-t->x,
                        sprite[s->owner].y-t->y);

                if (klabs(G_GetAngleDelta(sqa,sqb)) > 512)
                    if (ldist(&sprite[s->owner],(const spritetype *)t) < ldist(&sprite[g_player[screenpeek].ps->i],&sprite[s->owner]))
                        t->xrepeat = t->yrepeat = 0;
            }
            continue;
        case BURNING__STATIC:
        case BURNING2__STATIC:
            if (sprite[s->owner].statnum == STAT_PLAYER)
            {
                const int32_t snum = P_Get(s->owner);

                if (display_mirror == 0 && snum == screenpeek && g_player[snum].ps->over_shoulder_on == 0)
                    t->xrepeat = 0;
                else
                {
                    t->ang = getangle(ourx-t->x, oury-t->y);
                    t->x = sprite[s->owner].x + (sintable[(t->ang+512)&2047]>>10);
                    t->y = sprite[s->owner].y + (sintable[t->ang&2047]>>10);
                }
            }
            break;

        case ATOMICHEALTH__STATIC:
            t->z -= (4<<8);
            break;
        case CRYSTALAMMO__STATIC:
            t->shade = (sintable[(totalclock<<4)&2047]>>10);
            continue;
        case VIEWSCREEN__STATIC:
        case VIEWSCREEN2__STATIC:
        {
            const int viewscrShift = G_GetViewscreenSizeShift(t);
            const int viewscrTile = TILE_VIEWSCR-viewscrShift;

            if (g_curViewscreen >= 0 && actor[OW].t_data[0] == 1)
            {
                t->picnum = STATIC;
                t->cstat |= (rand()&12);
                t->xrepeat += 10;
                t->yrepeat += 9;
            }
            else if (g_curViewscreen == i && display_mirror != 3 && waloff[viewscrTile] && walock[viewscrTile] > 200)
            {
                // this exposes a sprite sorting issue which needs to be debugged further...
#if 0
                if (spritesortcnt < MAXSPRITESONSCREEN)
                {
                    spritetype *const newt = &tsprite[spritesortcnt++];

                    Bmemcpy(newt, t, sizeof(spritetype));

                    newt->cstat |= 2|512;
                    newt->x += (sintable[(newt->ang+512)&2047]>>12);
                    newt->y += (sintable[newt->ang&2047]>>12);
                    updatesector(newt->x, newt->y, &newt->sectnum);
                }
#endif
                t->picnum = viewscrTile;
#if VIEWSCREENFACTOR > 0
                t->xrepeat >>= viewscrShift;
                t->yrepeat >>= viewscrShift;
#endif
            }

            break;
        }

        case SHRINKSPARK__STATIC:
            t->picnum = SHRINKSPARK+((totalclock>>4)&3);
            break;
        case GROWSPARK__STATIC:
            t->picnum = GROWSPARK+((totalclock>>4)&3);
            break;
        case RPG__STATIC:
#ifdef USE_OPENGL
            if (getrendermode() >= REND_POLYMOST && usemodels && md_tilehasmodel(t->picnum,t->pal) >= 0 &&
                    !(spriteext[i].flags & SPREXT_NOTMD))
            {
                int32_t v = getangle(t->xvel, t->zvel>>4);

                spriteext[i].pitch = (v > 1023 ? v-2048 : v);
                t->cstat &= ~4;
                break;
            }
#endif
            k = getofs_viewtype7(s, t, getangle(s->x-ourx, s->y-oury), 0);
            t->picnum = RPG+k;
            break;

        case RECON__STATIC:
#ifdef USE_OPENGL
            if (getrendermode() >= REND_POLYMOST && usemodels && md_tilehasmodel(t->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                t->cstat &= ~4;
                break;
            }
#endif
            k = getofs_viewtype7(s, t, getangle(s->x-ourx, s->y-oury), 0);

            // RECON_T4
            if (klabs(curframe) > 64)
                k += 7;  // tilted recon car

            t->picnum = RECON+k;

            break;

        case APLAYER__STATIC:
            p = P_GetP((const spritetype *)s);

            if (t->pal == 1) t->z -= (18<<8);

            if (g_player[p].ps->over_shoulder_on > 0 && g_player[p].ps->newowner < 0)
            {
                t->ang = g_player[p].ps->ang +
                    mulscale16((((g_player[p].ps->ang+1024 - g_player[p].ps->oang)&2047)-1024),
                               smoothratio);
#ifdef USE_OPENGL
                if (bpp > 8 && usemodels && md_tilehasmodel(t->picnum, t->pal) >= 0)
                {
                    static int32_t targetang = 0;

                    if (g_player[p].sync->extbits&(1<<1))
                    {
                        if (g_player[p].sync->extbits&(1<<2))targetang += 16;
                        else if (g_player[p].sync->extbits&(1<<3)) targetang -= 16;
                        else if (targetang > 0) targetang -= targetang>>2;
                        else if (targetang < 0) targetang += (-targetang)>>2;
                    }
                    else
                    {
                        if (g_player[p].sync->extbits&(1<<2))targetang -= 16;
                        else if (g_player[p].sync->extbits&(1<<3)) targetang += 16;
                        else if (targetang > 0) targetang -= targetang>>2;
                        else if (targetang < 0) targetang += (-targetang)>>2;
                    }

                    targetang = clamp(targetang, -128, 128);
                    t->ang += targetang;
                }
                else
#endif
                    t->cstat |= 2;
            }

            if ((g_netServer || ud.multimode > 1) && (display_mirror || screenpeek != p || s->owner == -1))
            {
                if (ud.showweapons && sprite[g_player[p].ps->i].extra > 0 && g_player[p].ps->curr_weapon > 0
                        && spritesortcnt < MAXSPRITESONSCREEN)
                {
                    tspritetype *const newt = &tsprite[spritesortcnt];
                    int32_t curweap = g_player[p].ps->curr_weapon;

                    Bmemcpy(newt, t, sizeof(spritetype));

                    newt->statnum = TSPR_TEMP;
                    /*
                    newt->yrepeat = (t->yrepeat>>3);
                    if (t->yrepeat < 4) t->yrepeat = 4;
                    */

                    newt->cstat = newt->pal = 0;

                    newt->picnum = (curweap==GROW_WEAPON ? GROWSPRITEICON : WeaponPickupSprites[curweap]);

                    if (s->owner >= 0)
                        newt->z = g_player[p].ps->pos.z-(12<<8);
                    else
                        newt->z = s->z-(51<<8);

                    if (newt->picnum == HEAVYHBOMB)
                        newt->xrepeat = newt->yrepeat = 10;
                    else
                        newt->xrepeat = newt->yrepeat = 16;

                    spritesortcnt++;
                }

                if (g_player[p].sync->extbits & (1<<7) && !ud.pause_on && spritesortcnt<MAXSPRITESONSCREEN)
                {
                    tspritetype *const newt = &tsprite[spritesortcnt];

                    Bmemcpy(newt, t, sizeof(spritetype));

                    newt->statnum = TSPR_TEMP;

                    newt->yrepeat = (t->yrepeat>>3);
                    if (newt->yrepeat < 4) newt->yrepeat = 4;

                    newt->cstat = 0;
                    newt->picnum = RESPAWNMARKERGREEN;

                    if (s->owner >= 0)
                        newt->z = g_player[p].ps->pos.z-(20<<8);
                    else
                        newt->z = s->z-(96<<8);

                    newt->xrepeat = newt->yrepeat = 32;
                    newt->pal = 20;

                    spritesortcnt++;
                }
            }

            if (s->owner == -1)
            {
#ifdef USE_OPENGL
                if (getrendermode() >= REND_POLYMOST && usemodels && md_tilehasmodel(s->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
                {
                    k = 0;
                    t->cstat &= ~4;
                }
                else
#endif
                    k = getofs_viewtype5(s, t, oura, 0);

                if (sector[s->sectnum].lotag == ST_2_UNDERWATER) k += 1795-1405;
                else if ((actor[i].floorz-s->z) > (64<<8)) k += 60;

                t->picnum += k;
                t->pal = g_player[p].ps->palookup;

                goto PALONLY;
            }

            if (g_player[p].ps->on_crane == -1 && (sector[s->sectnum].lotag&0x7ff) != 1)  // ST_1_ABOVE_WATER ?
            {
                l = s->z-actor[g_player[p].ps->i].floorz+(3<<8);
                // SET_SPRITE_NOT_TSPRITE
                if (l > 1024 && s->yrepeat > 32 && s->extra > 0)
                    s->yoffset = (int8_t)tabledivide32_noinline(l, s->yrepeat<<2);
                else s->yoffset=0;
            }

            if (g_player[p].ps->newowner > -1)
            {
                // Display APLAYER sprites with action PSTAND when viewed through
                // a camera.  Not implemented for Lunatic.
#if !defined LUNATIC
                const intptr_t *aplayer_scr = g_tile[APLAYER].execPtr;
                // [0]=strength, [1]=actionofs, [2]=moveofs

                scrofs_action = aplayer_scr[1];
#endif
                curframe = 0;
            }

            if (ud.camerasprite == -1 && g_player[p].ps->newowner == -1)
                if (s->owner >= 0 && display_mirror == 0 && g_player[p].ps->over_shoulder_on == 0)
                    if ((!g_netServer && ud.multimode < 2) || ((g_netServer || ud.multimode > 1) && p == screenpeek))
                    {
                        if (getrendermode() == REND_POLYMER)
                            t->cstat |= 16384;
                        else
                        {
                            t->owner = -1;
                            t->xrepeat = t->yrepeat = 0;
                            continue;
                        }

#ifdef USE_OPENGL
                        if (getrendermode() >= REND_POLYMOST && usemodels && md_tilehasmodel(s->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
                        {
                            k = 0;
                            t->cstat &= ~4;
                        }
                        else
#endif
                            k = getofs_viewtype5(s, t, oura, 0);

                        if (sector[t->sectnum].lotag == ST_2_UNDERWATER) k += 1795-1405;
                        else if ((actor[i].floorz-s->z) > (64<<8)) k += 60;

                        t->picnum += k;
                        t->pal = g_player[p].ps->palookup;
                    }
PALONLY:
            G_MaybeTakeOnFloorPal(t, sect);

            if (s->owner == -1) continue;

            if (t->z > actor[i].floorz && t->xrepeat < 32)
                t->z = actor[i].floorz;

            break;

        case JIBS1__STATIC:
        case JIBS2__STATIC:
        case JIBS3__STATIC:
        case JIBS4__STATIC:
        case JIBS5__STATIC:
        case JIBS6__STATIC:
        case HEADJIB1__STATIC:
        case LEGJIB1__STATIC:
        case ARMJIB1__STATIC:
        case LIZMANHEAD1__STATIC:
        case LIZMANARM1__STATIC:
        case LIZMANLEG1__STATIC:
        case DUKELEG__STATIC:
        case DUKEGUN__STATIC:
        case DUKETORSO__STATIC:
            if (ud.lockout)
            {
                t->xrepeat = t->yrepeat = 0;
                continue;
            }
            if (t->pal == 6)
                t->shade = -120;
            /* fall-through */
        case SCRAP1__STATIC:
        case SCRAP2__STATIC:
        case SCRAP3__STATIC:
        case SCRAP4__STATIC:
        case SCRAP5__STATIC:
            if (actor[i].picnum == BLIMP && t->picnum == SCRAP1 && s->yvel >= 0)
                t->picnum = s->yvel < MAXUSERTILES ? s->yvel : 0;
            else t->picnum += T1;
            t->shade -= 6;

            G_MaybeTakeOnFloorPal(t, sect);
            break;

        case WATERBUBBLE__STATIC:
            if (sector[t->sectnum].floorpicnum == FLOORSLIME)
            {
                t->pal = 7;
                break;
            }
        default:
            G_MaybeTakeOnFloorPal(t, sect);
            break;
        }

        if (G_HaveActor(s->picnum))
        {
#if !defined LUNATIC
            if ((unsigned)scrofs_action + 2 >= (unsigned)g_scriptSize)
                goto skip;

            l = script[scrofs_action + 2];
#else
            l = viewtype;
#endif

#ifdef USE_OPENGL
            if (getrendermode() >= REND_POLYMOST && usemodels && md_tilehasmodel(s->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                k = 0;
                t->cstat &= ~4;
            }
            else
#endif
                switch (l)
                {
                case 2:
                    k = (((s->ang+3072+128-oura)&2047)>>8)&1;
                    break;

                case 3:
                case 4:
                    k = (((s->ang+3072+128-oura)&2047)>>7)&7;
                    if (k > 3)
                    {
                        t->cstat |= 4;
                        k = 7-k;
                    }
                    else t->cstat &= ~4;
                    break;

                case 5:
                case -5:
                    k = getofs_viewtype5(s, t, getangle(s->x-ourx, s->y-oury), l<0);
                    break;
                case 7:
                case -7:
                    k = getofs_viewtype7(s, t, getangle(s->x-ourx, s->y-oury), l<0);
                    break;
                case 8:
                case -8:
                    if (l > 0)
                        k = (((s->ang+3072+128-oura)&2047)>>8)&7;
                    else
                        k = (((oura+3072+128-s->ang)&2047)>>8)&7;
                    t->cstat &= ~4;
                    break;
                default:
                    k = 0;
                    break;
                }

            l = klabs(l);

#if !defined LUNATIC
            t->picnum += k + script[scrofs_action] + l*curframe;
#else
            t->picnum += k + startframe + l*curframe;
#endif
            // XXX: t->picnum can be out-of-bounds by bad user code.

            if (l > 0)
                while (tilesiz[t->picnum].x == 0 && t->picnum > 0)
                    t->picnum -= l;       //Hack, for actors

            if (actor[i].dispicnum >= 0)
                actor[i].dispicnum = t->picnum;
        }
//        else if (display_mirror == 1)
//            t->cstat |= 4;
        /* completemirror() already reverses the drawn frame, so the above isn't necessary.
         * Even Polymost's and Polymer's mirror seems to function correctly this way. */

#if !defined LUNATIC
skip:
#endif
        // Night vision goggles tsprite tinting.
        // XXX: Currently, for the splitscreen mod, sprites will be pal6-colored iff the first
        // player has nightvision on.  We should pass stuff like "from which player is this view
        // supposed to be" as parameters ("drawing context") instead of relying on globals.
        if (g_player[screenpeek].ps->inv_amount[GET_HEATS] > 0 && g_player[screenpeek].ps->heat_on &&
                (A_CheckEnemySprite((const spritetype *)s) || A_CheckSpriteFlags(t->owner,SFLAG_NVG) || s->picnum == APLAYER || s->statnum == STAT_DUMMYPLAYER))
        {
            t->pal = 6;
            t->shade = 0;
        }

        // Fake floor shadow, implemented by inserting a new tsprite.
        if (s->statnum == STAT_DUMMYPLAYER || A_CheckEnemySprite((const spritetype *)s) || A_CheckSpriteFlags(t->owner,SFLAG_SHADOW) || (s->picnum == APLAYER && s->owner >= 0))
            if (t->statnum != TSPR_TEMP && s->picnum != EXPLOSION2 && s->picnum != HANGLIGHT && s->picnum != DOMELITE && s->picnum != HOTMEAT)
            {
                if (actor[i].dispicnum < 0)
                {
#ifdef DEBUGGINGAIDS
                    // A negative actor[i].dispicnum used to mean 'no floor shadow please', but
                    // that was a bad hack since the value could propagate to sprite[].picnum.
                    OSD_Printf(OSD_ERROR "actor[%d].dispicnum = %d\n", i, actor[i].dispicnum);
#endif
                    actor[i].dispicnum=0;
                    continue;
                }

                if (actor[i].flags & SFLAG_NOFLOORSHADOW)
                    continue;

                if (ud.shadows && spritesortcnt < (MAXSPRITESONSCREEN-2) && getrendermode() != REND_POLYMER)
                {
                    int32_t daz;

                    if ((sector[sect].lotag&0xff) > 2 || s->statnum == STAT_PROJECTILE || s->statnum == STAT_MISC
                        || s->picnum == DRONE || s->picnum == COMMANDER)
                        daz = sector[sect].floorz;
                    else
                        daz = actor[i].floorz;

                    if ((s->z-daz) < (8<<8) && g_player[screenpeek].ps->pos.z < daz)
                    {
                        tspritetype *const newt = &tsprite[spritesortcnt];

                        Bmemcpy(newt, t, sizeof(spritetype));

                        newt->statnum = TSPR_TEMP;

                        newt->yrepeat = (t->yrepeat>>3);
                        if (t->yrepeat < 4) t->yrepeat = 4;

                        newt->shade = 127;
                        newt->cstat |= 2;

                        newt->z = daz;
                        newt->pal = 4;
#ifdef USE_OPENGL
                        if (getrendermode() >= REND_POLYMOST)
                        {
                            if (usemodels && md_tilehasmodel(t->picnum,t->pal) >= 0)
                            {
                                newt->yrepeat = 0;
                                // 512:trans reverse
                                //1024:tell MD2SPRITE.C to use Z-buffer hacks to hide overdraw issues
                                newt->extra |= TSPR_EXTRA_MDHACK;
                                newt->cstat |= 512;
                            }
                            else
                            {
                                int32_t ii;

                                ii = getangle(newt->x-g_player[screenpeek].ps->pos.x,
                                              newt->y-g_player[screenpeek].ps->pos.y);

                                newt->x += sintable[(ii+2560)&2047]>>9;
                                newt->y += sintable[(ii+2048)&2047]>>9;
                            }
                        }
#endif
                        spritesortcnt++;
                    }
                }
            }

        switch (DYNAMICTILEMAP(s->picnum))
        {
        case LASERLINE__STATIC:
            if (sector[t->sectnum].lotag == ST_2_UNDERWATER) t->pal = 8;
            t->z = sprite[s->owner].z-(3<<8);
            if (g_tripbombLaserMode == 2 && g_player[screenpeek].ps->heat_on == 0)
                t->yrepeat = 0;
        case EXPLOSION2__STATIC:
        case EXPLOSION2BOT__STATIC:
        case FREEZEBLAST__STATIC:
        case ATOMICHEALTH__STATIC:
        case FIRELASER__STATIC:
        case SHRINKSPARK__STATIC:
        case GROWSPARK__STATIC:
        case CHAINGUN__STATIC:
        case SHRINKEREXPLOSION__STATIC:
        case RPG__STATIC:
        case FLOORFLAME__STATIC:
            if (t->picnum == EXPLOSION2)
            {
                g_player[screenpeek].ps->visibility = -127;
                //g_restorePalette = 1;   // JBF 20040101: why?
            }
            t->shade = -127;
            t->cstat |= 8192+1024;
            break;
        case FIRE__STATIC:
        case FIRE2__STATIC:
            t->cstat |= 128;
        case BURNING__STATIC:
        case BURNING2__STATIC:
            if (sprite[s->owner].picnum != TREE1 && sprite[s->owner].picnum != TREE2)
                t->z = actor[t->owner].floorz;
            t->shade = -127;
        case SMALLSMOKE__STATIC:
            t->cstat |= 8192+1024;
            break;
        case COOLEXPLOSION1__STATIC:
            t->shade = -127;
            t->cstat |= 8192+1024;
            t->picnum += (s->shade>>1);
            break;
        case PLAYERONWATER__STATIC:
#ifdef USE_OPENGL
            if (getrendermode() >= REND_POLYMOST && usemodels && md_tilehasmodel(s->picnum,s->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                k = 0;
                t->cstat &= ~4;
            }
            else
#endif
                k = getofs_viewtype5(t, t, oura, 0);

            t->picnum = s->picnum+k+((T1<4)*5);
            t->shade = sprite[s->owner].shade;

            break;

        case WATERSPLASH2__STATIC:
            // WATERSPLASH_T2
            t->picnum = WATERSPLASH2+T2;
            break;
        case SHELL__STATIC:
            t->picnum = s->picnum+(T1&1);
        case SHOTGUNSHELL__STATIC:
            t->cstat |= 12;
            if (T1 > 2) t->cstat &= ~16;
            else if (T1 > 1) t->cstat &= ~4;
            break;
        case FRAMEEFFECT1_13__STATIC:
            if (PLUTOPAK) break;
        case FRAMEEFFECT1__STATIC:
            if (s->owner >= 0 && sprite[s->owner].statnum < MAXSTATUS)
            {
                if (sprite[s->owner].picnum == APLAYER)
                    if (ud.camerasprite == -1)
                        if (screenpeek == P_Get(s->owner) && display_mirror == 0)
                        {
                            t->owner = -1;
                            break;
                        }
                if ((sprite[s->owner].cstat&32768) == 0)
                {
                    if (!actor[s->owner].dispicnum)
                        t->picnum = actor[i].t_data[1];
                    else t->picnum = actor[s->owner].dispicnum;

                    if (!G_MaybeTakeOnFloorPal(t, sect))
                        t->pal = sprite[s->owner].pal;

                    t->shade = sprite[s->owner].shade;
                    t->ang = sprite[s->owner].ang;
                    t->cstat = 2|sprite[s->owner].cstat;
                }
            }
            break;

        case CAMERA1__STATIC:
        case RAT__STATIC:
#ifdef USE_OPENGL
            if (getrendermode() >= REND_POLYMOST && usemodels && md_tilehasmodel(s->picnum,s->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                t->cstat &= ~4;
                break;
            }
#endif
            k = getofs_viewtype5(t, t, oura, 0);
            t->picnum = s->picnum+k;
            break;
        }

        actor[i].dispicnum = t->picnum;
#if 0
        // why?
        if (sector[t->sectnum].floorpicnum == MIRROR)
            t->xrepeat = t->yrepeat = 0;
#endif
    }

    if (VM_HaveEvent(EVENT_ANIMATESPRITES))
    {
        for (j = spritesortcnt-1; j>=0; j--)
            G_DoEventAnimSprites(j);
    }

#ifdef LUNATIC
    VM_OnEvent(EVENT_ANIMATEALLSPRITES, -1, -1);
#endif
#ifdef DEBUGGINGAIDS
    g_spriteStat.numonscreen = spritesortcnt;
#endif
}

void G_SetViewportShrink(int32_t dir)
{
    if (ud.screen_size == 8 && dir!=0 && (dir>0)==(int32_t)ud.statusbarmode)
        ud.statusbarmode = !ud.statusbarmode;
    else
        ud.screen_size += dir;
    G_UpdateScreenArea();
}

void G_InitTimer(int32_t ticspersec)
{
    if (g_timerTicsPerSecond != ticspersec)
    {
        uninittimer();
        inittimer(ticspersec);
        g_timerTicsPerSecond = ticspersec;
    }
}


static int32_t g_RTSPlaying;

// Returns: started playing?
int32_t G_StartRTS(int32_t i, int localp)
{
    if (!ud.lockout && ud.config.SoundToggle &&
        RTS_IsInitialized() && g_RTSPlaying == 0 && (ud.config.VoiceToggle & (localp ? 1 : 4)))
    {
        char *sndptr = (char *)RTS_GetSound(i-1);

        if (sndptr != NULL)
        {
            FX_PlayAuto3D(sndptr, RTS_SoundLength(i-1), FX_ONESHOT, 0, 0, FX_VOLUME(1), 255, -i);
            g_RTSPlaying = 7;
            return 1;
        }
    }

    return 0;
}

void G_StartMusic(void)
{
    const int32_t i = g_musicIndex;
    Bassert(MapInfo[i].musicfn != NULL);

    {
        S_PlayMusic(MapInfo[i].musicfn);

        Bsnprintf(ScriptQuotes[QUOTE_MUSIC], MAXQUOTELEN, "Playing %s", MapInfo[i].musicfn);
        P_DoQuote(QUOTE_MUSIC, g_player[myconnectindex].ps);
    }
}

void G_HandleLocalKeys(void)
{
    int32_t i;
    int32_t j;

//    CONTROL_ProcessBinds();

    if (ud.recstat == 2)
    {
        ControlInfo noshareinfo;
        CONTROL_GetInput(&noshareinfo);
    }

    if (g_player[myconnectindex].gotvote == 0 && voting != -1 && voting != myconnectindex)
    {
        if (KB_UnBoundKeyPressed(sc_F1) || KB_UnBoundKeyPressed(sc_F2) || ud.autovote)
        {
            G_AddUserQuote("Vote Cast");
            Net_SendMapVote(KB_UnBoundKeyPressed(sc_F1) || ud.autovote ? ud.autovote-1 : 0);
            KB_ClearKeyDown(sc_F1);
            KB_ClearKeyDown(sc_F2);
        }
    }

    if (!ALT_IS_PRESSED && ud.overhead_on == 0 && (g_player[myconnectindex].ps->gm & MODE_TYPE) == 0)
    {
        if (BUTTON(gamefunc_Enlarge_Screen))
        {
            CONTROL_ClearButton(gamefunc_Enlarge_Screen);

            if (!SHIFTS_IS_PRESSED)
            {
                if (ud.screen_size > 0)
                {
                    S_PlaySound(THUD);
                    G_SetViewportShrink(-4);
                }
            }
            else
            {
                G_SetStatusBarScale(ud.statusbarscale+4);
            }

            G_UpdateScreenArea();
        }

        if (BUTTON(gamefunc_Shrink_Screen))
        {
            CONTROL_ClearButton(gamefunc_Shrink_Screen);

            if (!SHIFTS_IS_PRESSED)
            {
                if (ud.screen_size < 8 || (ud.screen_size == 8 && ud.statusbarmode))
                {
                    S_PlaySound(THUD);
                    G_SetViewportShrink(+4);
                }
            }
            else
            {
                G_SetStatusBarScale(ud.statusbarscale-4);
            }

            G_UpdateScreenArea();
        }
    }

    if (g_player[myconnectindex].ps->cheat_phase == 1 || (g_player[myconnectindex].ps->gm&(MODE_MENU|MODE_TYPE)))
        return;

    if (BUTTON(gamefunc_See_Coop_View) && (GTFLAGS(GAMETYPE_COOPVIEW) || ud.recstat == 2))
    {
        CONTROL_ClearButton(gamefunc_See_Coop_View);
        screenpeek = connectpoint2[screenpeek];
        if (screenpeek == -1) screenpeek = 0;
        g_restorePalette = -1;
    }

    if ((g_netServer || ud.multimode > 1) && BUTTON(gamefunc_Show_Opponents_Weapon))
    {
        CONTROL_ClearButton(gamefunc_Show_Opponents_Weapon);
        ud.config.ShowOpponentWeapons = ud.showweapons = 1-ud.showweapons;
        P_DoQuote(QUOTE_WEAPON_MODE_OFF-ud.showweapons,g_player[screenpeek].ps);
    }

    if (BUTTON(gamefunc_Toggle_Crosshair))
    {
        CONTROL_ClearButton(gamefunc_Toggle_Crosshair);
        ud.crosshair = !ud.crosshair;
        P_DoQuote(QUOTE_CROSSHAIR_OFF-ud.crosshair,g_player[screenpeek].ps);
    }

    if (ud.overhead_on && BUTTON(gamefunc_Map_Follow_Mode))
    {
        CONTROL_ClearButton(gamefunc_Map_Follow_Mode);
        ud.scrollmode = 1-ud.scrollmode;
        if (ud.scrollmode)
        {
            ud.folx = g_player[screenpeek].ps->opos.x;
            ud.foly = g_player[screenpeek].ps->opos.y;
            ud.fola = g_player[screenpeek].ps->oang;
        }
        P_DoQuote(QUOTE_MAP_FOLLOW_OFF+ud.scrollmode,g_player[myconnectindex].ps);
    }

    if (KB_UnBoundKeyPressed(sc_ScrollLock))
    {
        KB_ClearKeyDown(sc_ScrollLock);

        switch (ud.recstat)
        {
        case 0:
            if (SHIFTS_IS_PRESSED)
                G_OpenDemoWrite();
            break;
        case 1:
            G_CloseDemoWrite();
            break;
        }
    }

    if (ud.recstat == 2)
    {
        if (KB_KeyPressed(sc_Space))
        {
            KB_ClearKeyDown(sc_Space);

            g_demo_paused = !g_demo_paused;
            g_demo_rewind = 0;

            if (g_demo_paused)
            {
                FX_StopAllSounds();
                S_ClearSoundLocks();
            }
        }

        if (KB_KeyPressed(sc_Tab))
        {
            KB_ClearKeyDown(sc_Tab);
            g_demo_showStats = !g_demo_showStats;
        }

#if 0
        if (KB_KeyPressed(sc_kpad_Plus))
        {
            G_InitTimer(240);
        }
        else if (KB_KeyPressed(sc_kpad_Minus))
        {
            G_InitTimer(60);
        }
        else if (g_timerTicsPerSecond != 120)
        {
            G_InitTimer(120);
        }
#endif

        if (KB_KeyPressed(sc_kpad_6))
        {
            KB_ClearKeyDown(sc_kpad_6);
            j = (15<<(int)ALT_IS_PRESSED)<<(2*(int)SHIFTS_IS_PRESSED);
            g_demo_goalCnt = g_demo_paused ? g_demo_cnt+1 : g_demo_cnt+REALGAMETICSPERSEC*j;
            g_demo_rewind = 0;

            if (g_demo_goalCnt > g_demo_totalCnt)
                g_demo_goalCnt = 0;
            else
                Demo_PrepareWarp();
        }
        else if (KB_KeyPressed(sc_kpad_4))
        {
            KB_ClearKeyDown(sc_kpad_4);
            j = (15<<(int)ALT_IS_PRESSED)<<(2*(int)SHIFTS_IS_PRESSED);
            g_demo_goalCnt = g_demo_paused ? g_demo_cnt-1 : g_demo_cnt-REALGAMETICSPERSEC*j;
            g_demo_rewind = 1;

            if (g_demo_goalCnt <= 0)
                g_demo_goalCnt = 1;

            Demo_PrepareWarp();
        }

#if 0
        // Enter a game from within a demo.
        if (KB_KeyPressed(sc_Return) && ud.multimode==1)
        {
            KB_ClearKeyDown(sc_Return);
            g_demo_cnt = g_demo_goalCnt = ud.reccnt = ud.pause_on = ud.recstat = ud.m_recstat = 0;
            // XXX: probably redundant; this stuff needs an API anyway:
            kclose(g_demo_recFilePtr); g_demo_recFilePtr = -1;
            g_player[myconnectindex].ps->gm = MODE_GAME;
            ready2send=1;  // TODO: research this weird variable
            screenpeek=myconnectindex;
//            g_demo_paused=0;
        }
#endif
    }

    if (SHIFTS_IS_PRESSED || ALT_IS_PRESSED || WIN_IS_PRESSED)
    {
        i = 0;

        // NOTE: sc_F1 .. sc_F10 are contiguous. sc_F11 is not sc_F10+1.
        for (j=sc_F1; j<=sc_F10; j++)
            if (KB_UnBoundKeyPressed(j))
            {
                KB_ClearKeyDown(j);
                i = j - sc_F1 + 1;
                break;
            }

        if (i)
        {
            if (SHIFTS_IS_PRESSED)
            {
                if (i == 5 && g_player[myconnectindex].ps->fta > 0 && g_player[myconnectindex].ps->ftq == QUOTE_MUSIC)
                {
                    const int32_t maxi = VOLUMEALL ? MUS_FIRST_SPECIAL : 6;

                    do
                    {
                        g_musicIndex++;
                        if (g_musicIndex >= maxi)
                            g_musicIndex = 0;
                    }
                    while (MapInfo[g_musicIndex].musicfn == NULL);

                    G_StartMusic();

                    return;
                }

                G_AddUserQuote(ud.ridecule[i-1]);

#ifndef NETCODE_DISABLE
                tempbuf[0] = PACKET_MESSAGE;
                tempbuf[1] = 255;
                tempbuf[2] = 0;
                Bstrcat(tempbuf+2,ud.ridecule[i-1]);

                i = 2+strlen(ud.ridecule[i-1]);

                tempbuf[i++] = myconnectindex;

                if (g_netClient)
                    enet_peer_send(g_netClientPeer, CHAN_CHAT, enet_packet_create(tempbuf, i, 0));
                else if (g_netServer)
                    enet_host_broadcast(g_netServer, CHAN_CHAT, enet_packet_create(tempbuf, i, 0));
#endif
                pus = NUMPAGES;
                pub = NUMPAGES;

                return;
            }

            // Not SHIFT -- that is, either some ALT or WIN.
            if (G_StartRTS(i, 1))
            {
#ifndef NETCODE_DISABLE
                if ((g_netServer || ud.multimode > 1))
                {
                    tempbuf[0] = PACKET_RTS;
                    tempbuf[1] = i;
                    tempbuf[2] = myconnectindex;

                    if (g_netClient)
                        enet_peer_send(g_netClientPeer, CHAN_CHAT, enet_packet_create(tempbuf, 3, 0));
                    else if (g_netServer)
                        enet_host_broadcast(g_netServer, CHAN_CHAT, enet_packet_create(tempbuf, 3, 0));
                }
#endif
                pus = NUMPAGES;
                pub = NUMPAGES;

                return;
            }
        }
    }

    if (!ALT_IS_PRESSED && !SHIFTS_IS_PRESSED && !WIN_IS_PRESSED)
    {
        if ((g_netServer || ud.multimode > 1) && BUTTON(gamefunc_SendMessage))
        {
            KB_FlushKeyboardQueue();
            CONTROL_ClearButton(gamefunc_SendMessage);
            g_player[myconnectindex].ps->gm |= MODE_TYPE;
            typebuf[0] = 0;
        }

        if (KB_UnBoundKeyPressed(sc_F1)/* || (ud.show_help && I_AdvanceTrigger())*/)
        {
            KB_ClearKeyDown(sc_F1);
            M_ChangeMenu(MENU_STORY);

            S_PauseSounds(1);
            M_OpenMenu(myconnectindex);

            if ((!g_netServer && ud.multimode < 2))
            {
                ready2send = 0;
                totalclock = ototalclock;
                screenpeek = myconnectindex;
            }

            /*
            I_AdvanceTriggerClear();
            ud.show_help ++;

            if (ud.show_help > 2)
            {
                ud.show_help = 0;
                if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2) ready2send = 1;
                G_UpdateScreenArea();
            }
            else
            {
                setview(0,0,xdim-1,ydim-1);
                if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
                {
                    ready2send = 0;
                    totalclock = ototalclock;
                }
            }
            */
        }

        //        if((!net_server && ud.multimode < 2))
        {
            if (ud.recstat != 2 && KB_UnBoundKeyPressed(sc_F2))
            {
                KB_ClearKeyDown(sc_F2);

FAKE_F2:
                if (sprite[g_player[myconnectindex].ps->i].extra <= 0)
                {
                    P_DoQuote(QUOTE_SAVE_DEAD,g_player[myconnectindex].ps);
                    return;
                }
                M_ChangeMenu(MENU_SAVE);

                g_screenCapture = 1;
                G_DrawRooms(myconnectindex,65536);
                g_screenCapture = 0;

                S_PauseSounds(1);

                //                setview(0,0,xdim-1,ydim-1);
                M_OpenMenu(myconnectindex);

                if ((!g_netServer && ud.multimode < 2))
                {
                    ready2send = 0;
                    totalclock = ototalclock;
                    screenpeek = myconnectindex;
                }
            }

            if (KB_UnBoundKeyPressed(sc_F3))
            {
                KB_ClearKeyDown(sc_F3);

FAKE_F3:
                M_ChangeMenu(MENU_LOAD);

                S_PauseSounds(1);

                //                setview(0,0,xdim-1,ydim-1);
                M_OpenMenu(myconnectindex);
                if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
                {
                    ready2send = 0;
                    totalclock = ototalclock;
                }
                screenpeek = myconnectindex;
            }
        }

        if (KB_UnBoundKeyPressed(sc_F4))
        {
            KB_ClearKeyDown(sc_F4);
            S_PauseSounds(1);

            M_OpenMenu(myconnectindex);
            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
            {
                ready2send = 0;
                totalclock = ototalclock;
            }
            M_ChangeMenu(MENU_SOUND_INGAME);

        }

        if (KB_UnBoundKeyPressed(sc_F5) && ud.config.MusicToggle)
        {
            map_t *map = &MapInfo[g_musicIndex];
            char *const qmusic = ScriptQuotes[QUOTE_MUSIC];

            KB_ClearKeyDown(sc_F5);

            if (map->musicfn != NULL)
                Bsnprintf(qmusic, MAXQUOTELEN, "%s.  Use SHIFT-F5 to change.",
                map->musicfn);
            else
                qmusic[0] = '\0';

            P_DoQuote(QUOTE_MUSIC, g_player[myconnectindex].ps);
        }

        if ((KB_UnBoundKeyPressed(sc_F6) || g_doQuickSave == 1) && (g_player[myconnectindex].ps->gm&MODE_GAME))
        {
            KB_ClearKeyDown(sc_F6);
            g_doQuickSave = 0;

            if (g_lastSaveSlot == -1) goto FAKE_F2;

            KB_FlushKeyboardQueue();

            if (sprite[g_player[myconnectindex].ps->i].extra <= 0)
            {
                P_DoQuote(QUOTE_SAVE_DEAD,g_player[myconnectindex].ps);
                return;
            }
            g_screenCapture = 1;
            G_DrawRooms(myconnectindex,65536);
            g_screenCapture = 0;

            if (g_lastSaveSlot >= 0)
            {
                // dirty hack... char 127 in last position indicates an auto-filled name
                if (ud.savegame[g_lastSaveSlot][MAXSAVEGAMENAME-2] == 127)
                {
                    Bstrncpy(&ud.savegame[g_lastSaveSlot][0], MapInfo[ud.volume_number * MAXLEVELS + ud.level_number].name, 19);
                    ud.savegame[g_lastSaveSlot][MAXSAVEGAMENAME-2] = 127;
                }

                G_SavePlayerMaybeMulti(g_lastSaveSlot);
            }
        }

        if (KB_UnBoundKeyPressed(sc_F7))
        {
            KB_ClearKeyDown(sc_F7);
            if (g_player[myconnectindex].ps->over_shoulder_on)
                g_player[myconnectindex].ps->over_shoulder_on = 0;
            else
            {
                g_player[myconnectindex].ps->over_shoulder_on = 1;
                CAMERADIST = 0;
                CAMERACLOCK = totalclock;
            }
            P_DoQuote(QUOTE_VIEW_MODE_OFF+g_player[myconnectindex].ps->over_shoulder_on,g_player[myconnectindex].ps);
        }

        if (KB_UnBoundKeyPressed(sc_F8))
        {
            KB_ClearKeyDown(sc_F8);
            ud.fta_on = !ud.fta_on;
            if (ud.fta_on) P_DoQuote(QUOTE_MESSAGES_ON,g_player[myconnectindex].ps);
            else
            {
                ud.fta_on = 1;
                P_DoQuote(QUOTE_MESSAGES_OFF,g_player[myconnectindex].ps);
                ud.fta_on = 0;
            }
        }

        if ((KB_UnBoundKeyPressed(sc_F9) || g_doQuickSave == 2) && (g_player[myconnectindex].ps->gm&MODE_GAME))
        {
            KB_ClearKeyDown(sc_F9);
            g_doQuickSave = 0;

            if (g_lastSaveSlot == -1) goto FAKE_F3;

            if (g_lastSaveSlot >= 0)
            {
                KB_FlushKeyboardQueue();
                KB_ClearKeysDown();
                S_PauseSounds(1);
                G_LoadPlayerMaybeMulti(g_lastSaveSlot);
            }
        }

        if (KB_UnBoundKeyPressed(sc_F10))
        {
            KB_ClearKeyDown(sc_F10);
            M_ChangeMenu(MENU_QUIT_INGAME);
            S_PauseSounds(1);
            M_OpenMenu(myconnectindex);
            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
            {
                ready2send = 0;
                totalclock = ototalclock;
            }
        }

        if (KB_UnBoundKeyPressed(sc_F11))
        {
            KB_ClearKeyDown(sc_F11);
            M_ChangeMenu(MENU_COLCORR_INGAME);
            S_PauseSounds(1);
            M_OpenMenu(myconnectindex);
            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
            {
                ready2send = 0;
                totalclock = ototalclock;
            }
        }

        if (ud.overhead_on != 0)
        {

            j = totalclock-nonsharedtimer;
            nonsharedtimer += j;
            if (BUTTON(gamefunc_Enlarge_Screen))
                g_player[myconnectindex].ps->zoom += mulscale6(j,max(g_player[myconnectindex].ps->zoom,256));
            if (BUTTON(gamefunc_Shrink_Screen))
                g_player[myconnectindex].ps->zoom -= mulscale6(j,max(g_player[myconnectindex].ps->zoom,256));

            if (g_player[myconnectindex].ps->zoom > 2048)
                g_player[myconnectindex].ps->zoom = 2048;
            if (g_player[myconnectindex].ps->zoom < 48)
                g_player[myconnectindex].ps->zoom = 48;

        }
    }

    if (I_EscapeTrigger() && ud.overhead_on && g_player[myconnectindex].ps->newowner == -1)
    {
        I_EscapeTriggerClear();
        ud.last_overhead = ud.overhead_on;
        ud.overhead_on = 0;
        ud.scrollmode = 0;
        G_UpdateScreenArea();
    }

    if (BUTTON(gamefunc_AutoRun))
    {
        CONTROL_ClearButton(gamefunc_AutoRun);
        ud.auto_run = 1-ud.auto_run;
        P_DoQuote(QUOTE_RUN_MODE_OFF+ud.auto_run,g_player[myconnectindex].ps);
    }

    if (BUTTON(gamefunc_Map))
    {
        CONTROL_ClearButton(gamefunc_Map);
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

#ifdef __ANDROID__
        if (ud.overhead_on == 1)
            ud.scrollmode = 0;
        else if (ud.overhead_on == 2)
        {
            ud.scrollmode = 1;
            ud.folx = g_player[screenpeek].ps->opos.x;
            ud.foly = g_player[screenpeek].ps->opos.y;
            ud.fola = g_player[screenpeek].ps->oang;
        }
#endif
        g_restorePalette = 1;
        G_UpdateScreenArea();
    }
}

static void G_ShowParameterHelp(void)
{
    const char *s = "Usage: eduke32 [files] [options]\n"
              "Example: eduke32 -q4 -a -m -tx -map nukeland.map\n\n"
              "Files can be *.grp/zip/con/def/rts\n"
              "\n"
              "-cfg [file.cfg]\tUse an alternate configuration file\n"
#ifdef HAVE_CLIPSHAPE_FEATURE
              "-clipmap [file.map]\tLoad an additional clipping map for use with clipshape\n"
#endif
              "-connect [host]\tConnect to a multiplayer game\n"
              "-c#\t\tUse MP mode #, 1 = Dukematch, 2 = Coop, 3 = Dukematch(no spawn)\n"
              "-d [file.edm or demonum]\tPlay a demo\n"
              "-g [file.grp]\tLoad additional game data\n"
              "-h [file.def]\tLoad an alternate definitions file\n"
              "-j [dir]\t\tAdds a directory to EDuke32's search list\n"
              "-l#\t\tWarp to level #, see -v\n"
              "-map [file.map]\tLoads a map\n"
              "-mh [file.def]\tInclude an additional definitions module\n"
              "-mx [file.con]\tInclude an additional CON script module\n"
              "-m\t\tDisable monsters\n"
              "-nam\t\tRun in NAM compatibility mode\n"
              "-napalm\t\tRun in NAPALM compatibility mode\n"
              "-rts [file.rts]\tLoad a custom Remote Ridicule sound bank\n"
              "-r\t\tRecord demo\n"
              "-s#\t\tSet skill level (1-4)\n"
              "-server\t\tStart a multiplayer game for other players to join\n"
#ifdef STARTUP_SETUP_WINDOW
              "-setup/nosetup\tEnables/disables startup window\n"
#endif
              "-t#\t\tSet respawn mode: 1 = Monsters, 2 = Items, 3 = Inventory, x = All\n"
              "-usecwd\t\tRead game data and configuration file from working directory\n"
              "-u#########\tUser's favorite weapon order (default: 3425689071)\n"
              "-v#\t\tWarp to volume #, see -l\n"
              "-ww2gi\t\tRun in WWII GI compatibility mode\n"
              "-x [game.con]\tLoad custom CON script\n"
              "-#\t\tLoad and run a game from slot # (0-9)\n"
//              "\n-?/--help\tDisplay this help message and exit\n"
              "\nSee eduke32 -debughelp for debug parameters"
              ;
#ifdef WM_MSGBOX_WINDOW
    Bsnprintf(tempbuf, sizeof(tempbuf), HEAD2 " %s", s_buildRev);
    wm_msgbox(tempbuf,"%s",s);
#else
    initprintf("%s\n",s);
#endif
}

static void G_ShowDebugHelp(void)
{
    const char *s = "Usage: eduke32 [files] [options]\n"
              "\n"
              "-a\t\tUse fake player AI (fake multiplayer only)\n"
              "-cachesize #\tSets cache size, in Kb\n"
              "-game_dir [dir]\tDuke3d_w32 compatibility option, see -j\n"
              "-gamegrp   \tSelects which file to use as main grp\n"
              "-name [name]\tPlayer name in multiplay\n"
              "-noautoload\tDisable loading content from autoload dir\n"
#ifdef _WIN32
              "-nodinput\t\tDisable DirectInput (joystick) support\n"
#endif
              "-nologo\t\tSkip the logo anim\n"
              "-ns/-nm\t\tDisable sound or music\n"
              "-rotatesprite-no-widescreen\tpass bit 1024 to all CON rotatesprite calls\n"
              "-q#\t\tFake multiplayer with # (2-8) players\n"
              "-z#/-condebug\tEnable line-by-line CON compile debugging at level #\n"
              "-conversion YYYYMMDD\tSelects CON script version for compatibility with older mods\n"
#ifdef LUNATIC
              "-Lopts=<opt1>,<opt2>,...\n"
              "  Pass options to Lunatic, valid ones are:\n"
              "  diag, nojit, traces, dump, strict\n"
#endif
              ;
#ifdef WM_MSGBOX_WINDOW
    Bsnprintf(tempbuf, sizeof(tempbuf), HEAD2 " %s", s_buildRev);
    wm_msgbox(tempbuf,"%s",s);
#else
    initprintf("%s\n",s);
#endif
}

static int32_t S_DefineAudioIfSupported(char **fn, const char *name)
{
#if !defined HAVE_FLAC || !defined HAVE_VORBIS
    const char *extension = Bstrrchr(name, '.');
# if !defined HAVE_FLAC
    if (!Bstrcasecmp(extension, ".flac"))
        return -2;
# endif
# if !defined HAVE_VORBIS
    if (!Bstrcasecmp(extension, ".ogg"))
        return -2;
# endif
#endif
    realloc_copy(fn, name);
    return 0;
}

static int32_t S_DefineSound(int32_t ID, const char *name)
{
    if ((unsigned)ID >= MAXSOUNDS)
        return -1;

    return S_DefineAudioIfSupported(&g_sounds[ID].filename, name);
}

// Returns:
//   0: all OK
//  -1: ID declaration was invalid:
static int32_t S_DefineMusic(const char *ID, const char *name)
{
    int32_t sel = MUS_FIRST_SPECIAL;

    Bassert(ID != NULL);

    if (!Bstrcmp(ID,"intro"))
    {
        // nothing
    }
    else if (!Bstrcmp(ID,"briefing"))
    {
        sel++;
    }
    else if (!Bstrcmp(ID,"loading"))
    {
        sel += 2;
    }
    else
    {
        sel = G_GetMusicIdx(ID);
        if (sel < 0)
            return -1;
    }

    return S_DefineAudioIfSupported(&MapInfo[sel].musicfn, name);
}

static int32_t parsedefinitions_game(scriptfile *script, int32_t preload);

static void parsedefinitions_game_include(const char *fn, scriptfile *script, const char *cmdtokptr, const int32_t preload)
{
    scriptfile *included = scriptfile_fromfile(fn);

    if (!included)
    {
        if (!Bstrcasecmp(cmdtokptr,"null") || script == NULL) // this is a bit overboard to prevent unused parameter warnings
            {
           // initprintf("Warning: Failed including %s as module\n", fn);
            }
/*
        else
            {
            initprintf("Warning: Failed including %s on line %s:%d\n",
                       fn, script->filename,scriptfile_getlinum(script,cmdtokptr));
            }
*/
    }
    else
    {
        parsedefinitions_game(included, preload);
        scriptfile_close(included);
    }
}

static int32_t parsedefinitions_game(scriptfile *script, int32_t preload)
{
    int32_t tokn;
    char *cmdtokptr;

    static const tokenlist tokens[] =
    {
        { "include",         T_INCLUDE          },
        { "#include",        T_INCLUDE          },
        { "includedefault",  T_INCLUDEDEFAULT   },
        { "#includedefault", T_INCLUDEDEFAULT   },
        { "loadgrp",         T_LOADGRP          },
        { "cachesize",       T_CACHESIZE        },
        { "noautoload",      T_NOAUTOLOAD       },
        { "music",           T_MUSIC            },
        { "sound",           T_SOUND            },
        { "cutscene",        T_CUTSCENE         },
        { "animsounds",      T_ANIMSOUNDS       },
        { "renamefile",      T_RENAMEFILE       },
        { "globalgameflags", T_GLOBALGAMEFLAGS  },
    };

    static const tokenlist sound_musictokens[] =
    {
        { "id",   T_ID  },
        { "file", T_FILE },
    };

    static const tokenlist animtokens [] =
    {
        { "delay", T_DELAY },
    };

    while (1)
    {
        tokn = getatoken(script,tokens,ARRAY_SIZE(tokens));
        cmdtokptr = script->ltextptr;
        switch (tokn)
        {
        case T_LOADGRP:
        {
            char *fn;

            pathsearchmode = 1;
            if (!scriptfile_getstring(script,&fn) && preload)
            {
                int32_t j = initgroupfile(fn);

                if (j == -1)
                    initprintf("Could not find file \"%s\".\n",fn);
                else
                {
                    initprintf("Using file \"%s\" as game data.\n",fn);
                    if (!g_noAutoLoad && !ud.config.NoAutoLoad)
                        G_DoAutoload(fn);
                }

            }
            pathsearchmode = 0;
        }
        break;
        case T_CACHESIZE:
        {
            int32_t j;
            if (scriptfile_getnumber(script,&j) || !preload) break;

            if (j > 0) MAXCACHE1DSIZE = j<<10;
        }
        break;
        case T_INCLUDE:
        {
            char *fn;
            if (!scriptfile_getstring(script,&fn))
                parsedefinitions_game_include(fn, script, cmdtokptr, preload);
            break;
        }
        case T_INCLUDEDEFAULT:
        {
            parsedefinitions_game_include(G_DefaultDefFile(), script, cmdtokptr, preload);
            break;
        }
        case T_NOAUTOLOAD:
            if (preload)
                g_noAutoLoad = 1;
            break;
        case T_MUSIC:
        {
            char *tinttokptr = script->ltextptr;
            char *ID = NULL, *fn = NULL;
            char *musicend;

            if (scriptfile_getbraces(script,&musicend)) break;
            while (script->textptr < musicend)
            {
                switch (getatoken(script,sound_musictokens,ARRAY_SIZE(sound_musictokens)))
                {
                case T_ID:
                    scriptfile_getstring(script,&ID);
                    break;
                case T_FILE:
                    scriptfile_getstring(script,&fn);
                    break;
                }
            }
            if (!preload)
            {
                int32_t res;

                if (ID==NULL)
                {
                    initprintf("Error: missing ID for music definition near line %s:%d\n",
                               script->filename, scriptfile_getlinum(script,tinttokptr));
                    break;
                }

                if (fn == NULL || check_file_exist(fn))
                    break;

                res = S_DefineMusic(ID, fn);

                if (res == -1)
                    initprintf("Error: invalid music ID on line %s:%d\n",
                               script->filename, scriptfile_getlinum(script,tinttokptr));
            }
        }
        break;

        case T_CUTSCENE:
        {
            int32_t delay = 10;
            char *animend;
            dukeanim_t *anim = NULL;
            char *animname = NULL;

            scriptfile_getstring(script, &animname);

            if (scriptfile_getbraces(script, &animend))
                break;

            while (script->textptr < animend)
            {
                switch (getatoken(script, animtokens, ARRAY_SIZE(animtokens)))
                {
                    case T_DELAY: scriptfile_getnumber(script, &delay); break;
                }
            }

//            if (!preload)
            {
/*
                if (check_file_exist(animname))
                    break;
*/

                anim = Anim_Find(animname);

                if (!anim)
                    anim = Anim_Setup(animname, delay, NULL);
                else
                    anim->framedelay = delay;
            }
        }
        break;
        case T_ANIMSOUNDS:
        {
            char *otokptr = script->ltextptr;
            char *animsoundsend = NULL;
            int32_t numpairs = 0, allocsz = 4, bad = 1, lastframenum = INT32_MIN;
            dukeanim_t *anim = NULL;
            char *animname = NULL;

            scriptfile_getstring(script, &animname);

            if (animname)
                anim = Anim_Find(animname);

            if (!anim)
            {
                initprintf("Error: expected animation filename on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script, otokptr));
                break;
            }

            if (scriptfile_getbraces(script, &animsoundsend))
                break;

            if (anim->sounds)
            {
                initprintf("Warning: overwriting already defined hi-anim %s's sounds on line %s:%d\n", animname,
                           script->filename, scriptfile_getlinum(script, otokptr));
                Bfree(anim->sounds);
                anim->numsounds = 0;
            }

            if (!preload)
                anim->sounds = (uint16_t *)Xcalloc(allocsz, 2 * sizeof(uint16_t));
            while (script->textptr < animsoundsend)
            {
                int32_t framenum, soundnum;

                if (preload)
                {
                    // dummy
                    scriptfile_getstring(script, &animname);

                    if (animname)
                        anim = Anim_Find(animname);
                    continue;
                }

                // HACK: we've reached the end of the list
                //  (hack because it relies on knowledge of
                //   how scriptfile_* preprocesses the text)
                if (animsoundsend - script->textptr == 1)
                    break;

                // would produce error when it encounters the closing '}'
                // without the above hack
                if (scriptfile_getnumber(script, &framenum))
                    break;

                bad = 1;

                // TODO: look carefully at whether this can be removed.
                if (anim->sounds == NULL)  // Bcalloc check
                    break;

                if (scriptfile_getsymbol(script, &soundnum))
                    break;

                // frame numbers start at 1 for us
                if (framenum <= 0)
                {
                    initprintf("Error: frame number must be greater zero on line %s:%d\n", script->filename,
                               scriptfile_getlinum(script, script->ltextptr));
                    break;
                }

                if (framenum < lastframenum)
                {
                    initprintf("Error: frame numbers must be in (not necessarily strictly)"
                               " ascending order (line %s:%d)\n",
                               script->filename, scriptfile_getlinum(script, script->ltextptr));
                    break;
                }
                lastframenum = framenum;

                if ((unsigned)soundnum >= MAXSOUNDS)
                {
                    initprintf("Error: sound number #%d invalid on line %s:%d\n", soundnum, script->filename,
                               scriptfile_getlinum(script, script->ltextptr));
                    break;
                }

                if (numpairs >= allocsz)
                {
                    void *newptr;

                    allocsz *= 2;
                    newptr = Xrealloc(anim->sounds, allocsz * 2 * sizeof(uint16_t));

                    anim->sounds = (uint16_t *)newptr;
                }

                bad = 0;

                anim->sounds[2 * numpairs] = framenum;
                anim->sounds[2 * numpairs + 1] = soundnum;
                numpairs++;
            }

            if (!preload)
            {
                if (!bad)
                {
                    anim->numsounds = numpairs;
                    // initprintf("Defined sound sequence for hi-anim \"%s\" with %d frame/sound pairs\n",
                    //           hardcoded_anim_tokens[animnum].text, numpairs);
                }
                else
                {
                    DO_FREE_AND_NULL(anim->sounds);
                    initprintf("Failed defining sound sequence for hi-anim \"%s\".\n",
                               animname);
                }
            }
        }
        break;

        case T_SOUND:
        {
            char *tinttokptr = script->ltextptr;
            char *fn = NULL;
            int32_t num=-1;
            char *musicend;

            if (scriptfile_getbraces(script,&musicend)) break;
            while (script->textptr < musicend)
            {
                switch (getatoken(script,sound_musictokens,ARRAY_SIZE(sound_musictokens)))
                {
                case T_ID:
                    scriptfile_getsymbol(script,&num);
                    break;
                case T_FILE:
                    scriptfile_getstring(script,&fn);
                    break;
                }
            }
            if (!preload)
            {
                if (num==-1)
                {
                    initprintf("Error: missing ID for sound definition near line %s:%d\n", script->filename, scriptfile_getlinum(script,tinttokptr));
                    break;
                }

                if (fn == NULL || check_file_exist(fn))
                    break;

                if (S_DefineSound(num,fn) == -1)
                    initprintf("Error: invalid sound ID on line %s:%d\n", script->filename, scriptfile_getlinum(script,tinttokptr));
            }
        }
        break;
        case T_RENAMEFILE:
        {
            int32_t crcval = 0, filenum = -1;
            char *newname = NULL;
            if (scriptfile_getnumber(script,&crcval)) break;
            if (scriptfile_getnumber(script,&filenum)) break;
            if (scriptfile_getstring(script,&newname)) break;
            krename(crcval, filenum, newname);
        }
        break;
        case T_GLOBALGAMEFLAGS:
        {
            if (scriptfile_getnumber(script,&duke3d_globalflags)) break;
        }
        break;
        case T_EOF:
            return(0);
        default:
            break;
        }
    }
    return 0;
}

int32_t loaddefinitions_game(const char *fn, int32_t preload)
{
    scriptfile *script;
    int32_t i;

    script = scriptfile_fromfile(fn);
    if (script)
        parsedefinitions_game(script, preload);

    for (i=0; i < g_defModulesNum; ++i)
        parsedefinitions_game_include(g_defModules[i], NULL, "null", preload);

    if (script)
        scriptfile_close(script);

    scriptfile_clearsymbols();

    return 0;
}

#ifdef LUNATIC
char const * const * g_argv;
const char **g_elModules;
#endif

static void G_AddDemo(const char* param)
{
    Bstrncpy(tempbuf, param, sizeof(tempbuf));
    char * colon = (char *)Bstrchr(tempbuf, ':');
    int32_t framespertic=-1, numrepeats=1;

    if (colon && colon != tempbuf)
    {
        // -d<filename>:<num>[,<num>]
        // profiling options
        *(colon++) = 0;
        Bsscanf(colon, "%u,%u", &framespertic, &numrepeats);
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

static void G_CheckCommandLine(int32_t argc, char const * const * argv)
{
    int16_t i = 1, j;
    const char *c, *k;

#ifdef LUNATIC
    g_argv = argv;
    g_elModules = (const char **)Xcalloc(argc+1, sizeof(char *));
#endif
    ud.fta_on = 1;
    ud.god = 0;
    ud.m_respawn_items = 0;
    ud.m_respawn_monsters = 0;
    ud.m_respawn_inventory = 0;
    ud.warp_on = 0;
    ud.cashman = 0;
    ud.m_player_skill = ud.player_skill = 2;
    g_player[0].wchoice[0] = 3;
    g_player[0].wchoice[1] = 4;
    g_player[0].wchoice[2] = 5;
    g_player[0].wchoice[3] = 7;
    g_player[0].wchoice[4] = 8;
    g_player[0].wchoice[5] = 6;
    g_player[0].wchoice[6] = 0;
    g_player[0].wchoice[7] = 2;
    g_player[0].wchoice[8] = 9;
    g_player[0].wchoice[9] = 1;
    Bsprintf(ud.wchoice, "3457860291");

#ifdef HAVE_CLIPSHAPE_FEATURE
    // pre-form the default 10 clipmaps
    for (j = '0'; j<='9'; ++j)
    {
        char clipshape[16] = "_clipshape0.map";

        clipshape[10] = j;
        g_clipMapFiles = (char **)Xrealloc(g_clipMapFiles, (g_clipMapFilesNum+1) * sizeof(char *));
        g_clipMapFiles[g_clipMapFilesNum] = Xstrdup(clipshape);
        ++g_clipMapFilesNum;
    }
#endif

    if (argc > 1)
    {
#ifdef LUNATIC
        int32_t numlmods = 0;
#endif
        initprintf("Application parameters: ");
        while (i < argc)
            initprintf("%s ",argv[i++]);
        initprintf("\n");

        i = 1;
        do
        {
            const char *const oc = argv[i];
            int32_t shortopt = 0, ignored_short_opt = 0;

            c = oc;

            if ((*c == '-')
#ifdef _WIN32
                    || (*c == '/')
#endif
               )
            {
                shortopt = 0;

                if (!Bstrcasecmp(c+1,"?") || !Bstrcasecmp(c+1,"help") || !Bstrcasecmp(c+1,"-help"))
                {
                    G_ShowParameterHelp();
                    Bexit(0);
                }
                if (!Bstrcasecmp(c+1,"addon"))
                {
                    if (argc > i+1)
                    {
                        g_usingAddon = Batoi(argv[i+1]);

                        if (g_usingAddon > ADDON_NONE && g_usingAddon < NUMADDONS)
                            g_noSetup = 1;
                        else g_usingAddon = ADDON_NONE;

                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"debughelp") || !Bstrcasecmp(c+1,"-debughelp"))
                {
                    G_ShowDebugHelp();
                    Bexit(0);
                }
                if (!Bstrcasecmp(c+1,"grp") || !Bstrcasecmp(c+1,"g"))
                {
                    if (argc > i+1)
                    {
                        G_AddGroup(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"game_dir"))
                {
                    if (argc > i+1)
                    {
                        Bstrncpyz(g_modDir, argv[i+1], sizeof(g_modDir));
                        G_AddPath(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"cfg"))
                {
                    if (argc > i+1)
                    {
                        Bstrcpy(setupfilename,argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"gamegrp"))
                {
                    if (argc > i+1)
                    {
                        clearGrpNamePtr();
                        g_grpNamePtr = dup_filename(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"nam"))
                {
                    g_gameType = GAMEFLAG_NAM;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"napalm"))
                {
                    g_gameType = GAMEFLAG_NAM|GAMEFLAG_NAPALM;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"ww2gi"))
                {
                    g_gameType = GAMEFLAG_WW2GI;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"setup"))
                {
                    g_commandSetup = TRUE;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"nosetup"))
                {
                    g_noSetup = 1;
                    g_commandSetup = 0;
                    i++;
                    continue;
                }
#if defined RENDERTYPEWIN
                if (!Bstrcasecmp(c+1,"nodinput"))
                {
                    initprintf("DirectInput (joystick) support disabled\n");
                    di_disabled = 1;
                    i++;
                    continue;
                }
#endif
                if (!Bstrcasecmp(c+1,"noautoload"))
                {
                    initprintf("Autoload disabled\n");
                    g_noAutoLoad = 1;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"net"))
                {
                    G_GameExit("EDuke32 no longer supports legacy networking.\n\n"
                               "If using YANG or other launchers that only support legacy netplay, download an older build of EDuke32. "
                               "Otherwise, run the following:\n\n"
                               "eduke32 -server\n\n"
                               "Other clients can then connect by typing \"connect [host]\" in the console.\n\n"
                               "EDuke32 will now close.");
                }
                if (!Bstrcasecmp(c+1,"port"))
                {
                    if (argc > i+1)
                    {
                        g_netPort = Batoi(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"server"))
                {
                    g_networkMode = NET_SERVER;
                    g_noSetup = g_noLogo = TRUE;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"dedicated"))
                {
                    g_networkMode = NET_DEDICATED_SERVER;
                    g_noSetup = g_noLogo = TRUE;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"connect"))
                {
                    if (argc > i+1)
                    {
                        Net_Connect(argv[i+1]);
                        g_noSetup = g_noLogo = TRUE;
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"password"))
                {
                    if (argc > i+1)
                    {
                        Bstrncpyz(g_netPassword, argv[i+1], sizeof(g_netPassword));
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"name"))
                {
                    if (argc > i+1)
                    {
                        CommandName = argv[i+1];
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"map"))
                {
                    if (argc > i+1)
                    {
                        CommandMap = argv[i+1];
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"rts"))
                {
                    if (argc > i+1)
                    {
                        g_rtsNamePtr = argv[i+1];
                        Bstrncpyz(ud.rtsname, g_rtsNamePtr, sizeof(ud.rtsname));
                        initprintf("Using RTS file \"%s\".\n", ud.rtsname);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"x"))
                {
                    if (argc > i+1)
                    {
                        G_AddCon(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"mx"))
                {
                    if (argc > i+1)
                    {
                        G_AddConModule(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"h"))
                {
                    if (argc > i+1)
                    {
                        G_AddDef(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"mh"))
                {
                    if (argc > i+1)
                    {
                        G_AddDefModule(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"j"))
                {
                    if (argc > i+1)
                    {
                        G_AddPath(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"d"))
                {
                    if (argc > i+1)
                    {
                        G_AddDemo(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
#ifdef HAVE_CLIPSHAPE_FEATURE
                if (!Bstrcasecmp(c+1,"clipmap"))
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
                if (!Bstrcasecmp(c+1,"condebug"))
                {
                    g_scriptDebug = 1;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"nologo"))
                {
                    g_noLogo = 1;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"rotatesprite-no-widescreen"))
                {
                    g_rotatespriteNoWidescreen = 1;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"usecwd"))
                {
                    usecwd = 1;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"cachesize"))
                {
                    if (argc > i+1)
                    {
                        uint32_t j = Batol(argv[i+1]);
                        MAXCACHE1DSIZE = j<<10;
                        initprintf("Cache size: %dkB\n",j);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"noinstancechecking"))
                {
                    i++;
                    continue;
                }
#if defined(RENDERTYPEWIN) && defined(USE_OPENGL)
                if (!Bstrcasecmp(c+1,"forcegl"))
                {
                    forcegl = 1;
                    i++;
                    continue;
                }
#endif
                // the following two dummy entries allow us to serve as a drop-in replacement for NAM on Steam
                if (!Bstrcasecmp(c+1,"noconsole"))
                {
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"conf"))
                {
                    if (argc > i+1)
                        i++;
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
                case 'd':
                {
                    c++;
                    if (*c)
                        G_AddDemo(c);
                    break;
                }
#ifdef LUNATIC
                case 'f':
                    break;
#endif
                case 'g':
                    c++;
                    if (*c)
                        G_AddGroup(c);
                    break;
                case 'h':
                    c++;
                    if (*c)
                        G_AddDef(c);
                    break;
                case 'j':
                    c++;
                    if (*c)
                        G_AddPath(c);
                    break;
                case 'l':
                    // NOTE: Overlaid with -Lopts=... options for Lunatic, hence the check.
                    if (Bisdigit(c[1]))
                    {
                        ud.warp_on = 1;
                        c++;
                        ud.m_level_number = ud.level_number = ((unsigned)(Batoi(c)-1))%MAXLEVELS;
                    }
                    break;
                case 'm':
                    if (*(c+1) != 'a' && *(c+1) != 'A')
                    {
                        ud.m_monsters_off = 1;
                        ud.m_player_skill = ud.player_skill = 0;
                        initprintf("Monsters off.\n");
                    }
                    break;
                case 'n':
                    c++;
                    if (*c == 's' || *c == 'S')
                    {
                        g_noSound = 2;
                        initprintf("Sound off.\n");
                    }
                    else if (*c == 'm' || *c == 'M')
                    {
                        g_noMusic = 1;
                        initprintf("Music off.\n");
                    }
                    else
                    {
                        G_ShowParameterHelp();
                        exit(-1);
                    }
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
                case 'u':
                    g_forceWeaponChoice = 1;
                    c++;
                    j = 0;
                    if (*c)
                    {
                        initprintf("Using favorite weapon order(s).\n");
                        while (*c)
                        {
                            g_player[0].wchoice[j] = *c-'0';
                            ud.wchoice[j] = *c;
                            c++;
                            j++;
                        }

                        while (j < 10)
                        {
                            if (j == 9)
                            {
                                g_player[0].wchoice[9] = 1;
                                ud.wchoice[9] = '1';
                            }
                            else
                            {
                                g_player[0].wchoice[j] = 2;
                                ud.wchoice[j] = '2';
                            }

                            j++;
                        }
                    }
                    else
                    {
                        initprintf("Using default weapon orders.\n");
                        g_player[0].wchoice[0] = 3;
                        g_player[0].wchoice[1] = 4;
                        g_player[0].wchoice[2] = 5;
                        g_player[0].wchoice[3] = 7;
                        g_player[0].wchoice[4] = 8;
                        g_player[0].wchoice[5] = 6;
                        g_player[0].wchoice[6] = 0;
                        g_player[0].wchoice[7] = 2;
                        g_player[0].wchoice[8] = 9;
                        g_player[0].wchoice[9] = 1;

                        Bsprintf(ud.wchoice, "3457860291");
                    }
                    break;
                case 'v':
                    c++;
                    ud.warp_on = 1;
                    ud.m_volume_number = ud.volume_number = ((unsigned)(Batoi(c)-1))%MAXVOLUMES;
                    break;
                case 'w':
                    ud.coords = 1;
                    break;
#ifdef LUNATIC
                case 'W':
                    break;
#endif
                case 'x':
                    c++;
                    if (*c)
                        G_AddCon(c);
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    ud.warp_on = 2 + (*c) - '0';
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
            else
            {
                shortopt = 0;

                k = Bstrrchr(c,'.');
                if (k)
                {
                    if (!Bstrcasecmp(k,".map"))
                    {
                        CommandMap = argv[i++];
                        continue;
                    }
                    if (!Bstrcasecmp(k,".grp") || !Bstrcasecmp(k,".zip") || !Bstrcasecmp(k,".pk3") || !Bstrcasecmp(k,".pk4"))
                    {
                        G_AddGroup(argv[i++]);
                        continue;
                    }
                    if (!Bstrcasecmp(k,".con"))
                    {
                        clearScriptNamePtr();
                        g_scriptNamePtr = dup_filename(argv[i++]);
                        initprintf("Using CON file \"%s\".\n",g_scriptNamePtr);
                        continue;
                    }
                    if (!Bstrcasecmp(k,".def"))
                    {
                        clearDefNamePtr();
                        g_defNamePtr = dup_filename(argv[i++]);
                        initprintf("Using DEF file \"%s\".\n",g_defNamePtr);
                        continue;
                    }
                    if (!Bstrcasecmp(k,".rts"))
                    {
                        g_rtsNamePtr = argv[i++];
                        Bstrncpyz(ud.rtsname, g_rtsNamePtr, sizeof(ud.rtsname));
                        initprintf("Using RTS file \"%s\".\n", ud.rtsname);
                        continue;
                    }
#ifdef LUNATIC
                    if (!Bstrcmp(k,".lua"))  // NOTE: case sensitive!
                    {
                        g_elModules[numlmods++] = argv[i++];
                        continue;
                    }
#endif
                }
            }

            if (!shortopt || ignored_short_opt)
                initprintf("Warning: ignored application parameter \"%s\".\n", oc);

            i++;
        }
        while (i < argc);
    }
}

void G_UpdateAppTitle(void)
{
    if (g_gameNamePtr)
    {
        Bsprintf(tempbuf, "%s - " APPNAME, g_gameNamePtr);
        wm_setapptitle(tempbuf);
    }
    else
    {
        wm_setapptitle(APPNAME);
    }
}

static void G_DisplayLogo(void)
{
    int32_t soundanm = 0;
    int32_t logoflags = G_GetLogoFlags();

    ready2send = 0;

    I_ClearAllInput();

    setview(0,0,xdim-1,ydim-1);
    clearallviews(0L);
    G_FadePalette(0,0,0,252);

    flushperms();
    nextpage();

    G_UpdateAppTitle();

    S_StopMusic();
    FX_StopAllSounds(); // JBF 20031228
    S_ClearSoundLocks();  // JBF 20031228
    if ((!g_netServer && ud.multimode < 2) && (logoflags & LOGO_ENABLED) && !g_noLogo)
    {
        if (
#ifndef EDUKE32_TOUCH_DEVICES
            VOLUMEALL && 
#endif
            (logoflags & LOGO_PLAYANIM))
        {
            if (!I_CheckAllInput() && g_noLogoAnim == 0)
            {
                Net_GetPackets();
                Anim_Play("logo.anm");
                G_FadePalette(0,0,0,252);
                I_ClearAllInput();
            }

            clearallviews(0L);
            nextpage();

            if (logoflags & LOGO_STOPANIMSOUNDS)
            {
                FX_StopAllSounds();
                S_ClearSoundLocks();
            }
        }

        if (logoflags & LOGO_PLAYMUSIC)
        {
            g_musicIndex = MUS_INTRO;
            S_PlayMusic(MapInfo[g_musicIndex].musicfn);
        }

        if (!NAM)
        {
            //g_player[myconnectindex].ps->palette = drealms;
            //G_FadePalette(0,0,0,252);

            if (logoflags & LOGO_3DRSCREEN)
            {

                if (!I_CheckAllInput() && g_noLogoAnim == 0)
                {
                    int32_t i;
                    Net_GetPackets();

                    i = kopen4loadfrommod("3dr.ivf", 0);

                    if (i == -1)
                        i = kopen4loadfrommod("3dr.anm", 0);

                    if (i != -1)
                    {
                        kclose(i);
                        Anim_Play("3dr.anm");
                        G_FadePalette(0,0,0,252);
                        I_ClearAllInput();
                    }
                    else
                    {
                        clearallviews(0);

                        P_SetGamePalette(g_player[myconnectindex].ps, DREALMSPAL, 8 + 2 + 1);    // JBF 20040308
                        fadepal(0,0,0, 0,252,28);
                        flushperms();
                        rotatesprite_fs(160 << 16, 100 << 16, 65536L, 0, DREALMS, 0, 0, 2 + 8 + BGSTRETCH);
                        nextpage();
                        fadepaltile(0,0,0, 252,0,-28, DREALMS);
                        totalclock = 0;
                        while (totalclock < (120 * 7) && !I_CheckAllInput())
                        {
                            clearallviews(0);

                            rotatesprite_fs(160 << 16, 100 << 16, 65536L, 0, DREALMS, 0, 0, 2 + 8 + 64 + BGSTRETCH);

                            G_HandleAsync();

                            if (g_restorePalette)
                            {
                                P_SetGamePalette(g_player[myconnectindex].ps, g_player[myconnectindex].ps->palette, 0);
                                g_restorePalette = 0;
                            }
                            nextpage();
                        }
                        fadepaltile(0,0,0, 0,252,28, DREALMS);
                    }
                }

                clearallviews(0L);
                nextpage();
            }

            I_ClearAllInput();
        }

        clearallviews(0L);
        nextpage();

        if (logoflags & LOGO_TITLESCREEN)
        {
            clearallviews(0);

            //g_player[myconnectindex].ps->palette = titlepal;
            P_SetGamePalette(g_player[myconnectindex].ps, TITLEPAL, 8+2+1);   // JBF 20040308
            flushperms();
            rotatesprite_fs(160<<16,100<<16,65536L,0,BETASCREEN,0,0,2+8+64+BGSTRETCH);
            KB_FlushKeyboardQueue();
            fadepaltile(0,0,0, 252,0,-28,BETASCREEN);
            totalclock = 0;

            while (
#ifndef DROIDMENU
                totalclock < (860+120) &&
#endif
                !I_CheckAllInput())
            {
                clearallviews(0);

                rotatesprite_fs(160<<16,100<<16,65536L,0,BETASCREEN,0,0,2+8+64+BGSTRETCH);
                if (logoflags & LOGO_DUKENUKEM)
                {
                    if (totalclock > 120 && totalclock < (120+60))
                    {
                        if (soundanm == 0)
                        {
                            soundanm++;
                            S_PlaySound(PIPEBOMB_EXPLODE);
                        }
                        rotatesprite_fs(160<<16,104<<16,(totalclock-120)<<10,0,DUKENUKEM,0,0,2+8);
                    }
                    else if (totalclock >= (120+60))
                        rotatesprite_fs(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8);
                }
                else soundanm++;

                if (logoflags & LOGO_THREEDEE)
                {
                    if (totalclock > 220 && totalclock < (220+30))
                    {
                        if (soundanm == 1)
                        {
                            soundanm++;
                            S_PlaySound(PIPEBOMB_EXPLODE);
                        }

                        rotatesprite_fs(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8);
                        rotatesprite_fs(160<<16,(129)<<16,(totalclock - 220)<<11,0,THREEDEE,0,0,2+8);
                    }
                    else if (totalclock >= (220+30))
                        rotatesprite_fs(160<<16,(129)<<16,30<<11,0,THREEDEE,0,0,2+8);
                }
                else soundanm++;

                if (PLUTOPAK && (logoflags & LOGO_PLUTOPAKSPRITE))
                {
                    // JBF 20030804
                    if (totalclock >= 280 && totalclock < 395)
                    {
                        rotatesprite_fs(160<<16,(151)<<16,(410-totalclock)<<12,0,PLUTOPAKSPRITE+1,0,0,2+8);
                        if (soundanm == 2)
                        {
                            soundanm++;
                            S_PlaySound(FLY_BY);
                        }
                    }
                    else if (totalclock >= 395)
                    {
                        if (soundanm == 3)
                        {
                            soundanm++;
                            S_PlaySound(PIPEBOMB_EXPLODE);
                        }
                        rotatesprite_fs(160<<16,(151)<<16,30<<11,0,PLUTOPAKSPRITE+1,0,0,2+8);
                    }
                }

#ifdef LUNATIC
                g_elEventError = 0;
#endif
                VM_OnEvent(EVENT_LOGO, -1, screenpeek);
                G_HandleAsync();

                if (g_restorePalette)
                {
                    P_SetGamePalette(g_player[myconnectindex].ps,g_player[myconnectindex].ps->palette,0);
                    g_restorePalette = 0;
                }
#ifdef LUNATIC
                if (g_elEventError)
                    break;
#endif
                nextpage();
            }
        }
        I_ClearAllInput();
    }

    flushperms();
    clearallviews(0L);
    nextpage();

    //g_player[myconnectindex].ps->palette = palette;
    P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308
    S_PlaySound(NITEVISION_ONOFF);

    //G_FadePalette(0,0,0,0);
    clearallviews(0L);
}

static void G_FreeHashAnim(const char *UNUSED(string), intptr_t key)
{
    Bfree((void *)key);
}

static void G_Cleanup(void)
{
    int32_t i;

    for (i=(MAXLEVELS*(MAXVOLUMES+1))-1; i>=0; i--) // +1 volume for "intro", "briefing" music
    {
        Bfree(MapInfo[i].name);
        Bfree(MapInfo[i].filename);
        Bfree(MapInfo[i].musicfn);

        G_FreeMapState(i);
    }

    for (i=MAXQUOTES-1; i>=0; i--)
    {
        Bfree(ScriptQuotes[i]);
        Bfree(ScriptQuoteRedefinitions[i]);
    }

    for (i=MAXPLAYERS-1; i>=0; i--)
    {
        Bfree(g_player[i].ps);
        Bfree(g_player[i].sync);
    }

    for (i=MAXSOUNDS-1; i>=0; i--)
    {
        Bfree(g_sounds[i].filename);
    }
#if !defined LUNATIC
    if (label != (char *)&sprite[0]) Bfree(label);
    if (labelcode != (int32_t *)&sector[0]) Bfree(labelcode);
    Bfree(script);
    Bfree(bitptr);

//    Bfree(MusicPtr);

    hash_free(&h_gamevars);
    hash_free(&h_arrays);
    hash_free(&h_labels);
    hash_free(&h_gamefuncs);
#endif

    hash_loop(&h_dukeanim, G_FreeHashAnim);
    hash_free(&h_dukeanim);
}

/*
===================
=
= ShutDown
=
===================
*/

void G_Shutdown(void)
{
    CONFIG_WriteSetup(0);
    S_SoundShutdown();
    S_MusicShutdown();
    CONTROL_Shutdown();
    KB_Shutdown();
    uninitengine();
    G_Cleanup();
    FreeGroups();
    OSD_Cleanup();
    uninitgroupfile();
    Bfflush(NULL);
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
#if !defined LUNATIC
    int32_t psm = pathsearchmode;

    label     = (char *)&sprite[0];     // V8: 16384*44/64 = 11264  V7: 4096*44/64 = 2816
    labelcode = (int32_t *)&sector[0]; // V8: 4096*40/4 = 40960    V7: 1024*40/4 = 10240
    labeltype = (int32_t *)&wall[0];   // V8: 16384*32/4 = 131072  V7: 8192*32/4 = 65536
#endif

    if (g_scriptNamePtr != NULL)
        Bcorrectfilename(g_scriptNamePtr,0);

#if defined LUNATIC
    Gv_Init();
    C_InitProjectiles();
#else
    // if we compile for a V7 engine wall[] should be used for label names since it's bigger
    pathsearchmode = 1;

    C_Compile(G_ConFile());

    if (g_loadFromGroupOnly) // g_loadFromGroupOnly is true only when compiling fails and internal defaults are utilized
        C_Compile(G_ConFile());

    if ((uint32_t)g_numLabels > MAXSPRITES*sizeof(spritetype)/64)   // see the arithmetic above for why
        G_GameExit("Error: too many labels defined!");

    {
        char *newlabel;
        int32_t *newlabelcode;

        newlabel     = (char *)Xmalloc(g_numLabels<<6);
        newlabelcode = (int32_t *)Xmalloc(g_numLabels*sizeof(int32_t));

        Bmemcpy(newlabel, label, g_numLabels*64);
        Bmemcpy(newlabelcode, labelcode, g_numLabels*sizeof(int32_t));

        label = newlabel;
        labelcode = newlabelcode;
    }

    Bmemset(sprite, 0, MAXSPRITES*sizeof(spritetype));
    Bmemset(sector, 0, MAXSECTORS*sizeof(sectortype));
    Bmemset(wall, 0, MAXWALLS*sizeof(walltype));

    VM_OnEvent(EVENT_INIT, -1, -1);
    pathsearchmode = psm;
#endif
}

static inline void G_CheckGametype(void)
{
    ud.m_coop = clamp(ud.m_coop, 0, g_numGametypes-1);
    initprintf("%s\n",GametypeNames[ud.m_coop]);
    if (GametypeFlags[ud.m_coop] & GAMETYPE_ITEMRESPAWN)
        ud.m_respawn_items = ud.m_respawn_inventory = 1;
}

static void G_PostLoadPalette(void)
{
    if (!(duke3d_globalflags & DUKE3D_NO_PALETTE_CHANGES))
    {
        // Make color index 255 of default/water/slime palette black.
        if (basepaltable[BASEPAL] != NULL)
            Bmemset(&basepaltable[BASEPAL][255*3], 0, 3);
        if (basepaltable[WATERPAL] != NULL)
            Bmemset(&basepaltable[WATERPAL][255*3], 0, 3);
        if (basepaltable[SLIMEPAL] != NULL)
            Bmemset(&basepaltable[SLIMEPAL][255*3], 0, 3);
    }

    if (!(duke3d_globalflags & DUKE3D_NO_HARDCODED_FOGPALS))
        generatefogpals();

    if (!(duke3d_globalflags & DUKE3D_NO_PALETTE_CHANGES))
    {
        E_ReplaceTransparentColorWithBlack();
    }

    fillemptylookups();
}

#define SETFLAG(Tilenum, Flag) g_tile[Tilenum].flags |= Flag

// Has to be after setting the dynamic names (e.g. SHARK).
static void A_InitEnemyFlags(void)
{
    int32_t DukeEnemies[] = {
        SHARK, RECON, DRONE,
        LIZTROOPONTOILET, LIZTROOPJUSTSIT, LIZTROOPSTAYPUT, LIZTROOPSHOOT,
        LIZTROOPJETPACK, LIZTROOPSHOOT, LIZTROOPDUCKING, LIZTROOPRUNNING, LIZTROOP,
        OCTABRAIN, COMMANDER, COMMANDERSTAYPUT, PIGCOP, PIGCOPSTAYPUT, PIGCOPDIVE, EGG,
        LIZMAN, LIZMANSPITTING, LIZMANJUMP, ORGANTIC,
        BOSS1, BOSS2, BOSS3, BOSS4, RAT, ROTATEGUN };

    int32_t SolidEnemies[] = { TANK, BOSS1, BOSS2, BOSS3, BOSS4, RECON, ROTATEGUN };
    int32_t NoWaterDipEnemies[] = { OCTABRAIN, COMMANDER, DRONE };
    int32_t GreenSlimeFoodEnemies[] = { LIZTROOP, LIZMAN, PIGCOP, NEWBEAST };

    for (int i=GREENSLIME; i<=GREENSLIME+7; i++)
        SETFLAG(i, SFLAG_HARDCODED_BADGUY);

    for (int i=ARRAY_SIZE(DukeEnemies)-1; i>=0; i--)
        SETFLAG(DukeEnemies[i], SFLAG_HARDCODED_BADGUY);

    for (int i=ARRAY_SIZE(SolidEnemies)-1; i>=0; i--)
        SETFLAG(SolidEnemies[i], SFLAG_NODAMAGEPUSH);

    for (int i=ARRAY_SIZE(NoWaterDipEnemies)-1; i>=0; i--)
        SETFLAG(NoWaterDipEnemies[i], SFLAG_NOWATERDIP);

    for (int i=ARRAY_SIZE(GreenSlimeFoodEnemies)-1; i>=0; i--)
        SETFLAG(GreenSlimeFoodEnemies[i], SFLAG_GREENSLIMEFOOD);
}
#undef SETFLAG

static void G_SetupGameButtons(void);

#ifdef LUNATIC
// Will be used to store CON code translated to Lua.
int32_t g_elCONSize;
char *g_elCON;  // NOT 0-terminated!

LUNATIC_EXTERN void El_SetCON(const char *conluacode)
{
    int32_t slen = Bstrlen(conluacode);

    g_elCON = (char *)Xmalloc(slen);

    g_elCONSize = slen;
    Bmemcpy(g_elCON, conluacode, slen);
}

void El_CreateGameState(void)
{
    int32_t i;

    El_DestroyState(&g_ElState);

    if ((i = El_CreateState(&g_ElState, "game")))
    {
        initprintf("Lunatic: Error initializing global ELua state (code %d)\n", i);
    }
    else
    {
        extern const char luaJIT_BC_defs[];

        if ((i = L_RunString(&g_ElState, luaJIT_BC_defs,
                             LUNATIC_DEFS_BC_SIZE, "defs.ilua")))
        {
            initprintf("Lunatic: Error preparing global ELua state (code %d)\n", i);
            El_DestroyState(&g_ElState);
        }
    }

    if (i)
        G_GameExit("Failure setting up Lunatic!");

# if !defined DEBUGGINGAIDS
    El_ClearErrors();
# endif
}
#endif

// Throw in everything here that needs to be called after a Lua game state
// recreation (or on initial startup in a non-Lunatic build.)
void G_PostCreateGameState(void)
{
    Net_SendClientInfo();
    A_InitEnemyFlags();
}

static void G_HandleMemErr(int32_t line, const char *file, const char *func)
{
    static char msg[128];
    Bsnprintf(msg, sizeof(msg), "Out of memory in %s:%d (%s)\n", file, line, func);
#ifdef DEBUGGINGAIDS
    Bassert(0);
#endif
    G_GameExit(msg);
}

static void G_FatalEngineError(void)
{
    wm_msgbox("Build Engine Initialization Error",
              "There was a problem initializing the Build engine: %s", engineerrstr);
    G_Cleanup();
    ERRprintf("G_Startup: There was a problem initializing the Build engine: %s\n", engineerrstr);
    exit(6);
}

static void G_Startup(void)
{
    int32_t i;

    set_memerr_handler(&G_HandleMemErr);

    inittimer(TICRATE);

    initcrc32table();

    G_CompileScripts();

    if (initengine())
        G_FatalEngineError();

#ifdef LUNATIC
    El_CreateGameState();
    C_InitQuotes();
#endif

    G_InitDynamicTiles();
    G_InitDynamicSounds();

    // These depend on having the dynamic tile and/or sound mappings set up:
    G_InitMultiPsky(CLOUDYOCEAN, MOONSKY1, BIGORBIT1, LA);
    Gv_FinalizeWeaponDefaults();
    G_PostCreateGameState();
#ifdef LUNATIC
    // NOTE: This is only effective for CON-defined EVENT_INIT. See EVENT_INIT
    // not in defs.ilua.
    VM_OnEvent(EVENT_INIT, -1, -1);
#endif
    if (g_netServer || ud.multimode > 1) G_CheckGametype();

    if (g_noSound) ud.config.SoundToggle = 0;
    if (g_noMusic) ud.config.MusicToggle = 0;

    if (CommandName)
    {
        //        Bstrncpy(szPlayerName, CommandName, 9);
        //        szPlayerName[10] = '\0';
        Bstrcpy(tempbuf,CommandName);

        while (Bstrlen(OSD_StripColors(tempbuf,tempbuf)) > 10)
            tempbuf[Bstrlen(tempbuf)-1] = '\0';

        Bstrncpyz(szPlayerName, tempbuf, sizeof(szPlayerName));
    }

    if (CommandMap)
    {
        if (VOLUMEONE)
        {
            initprintf("The -map option is available in the registered version only!\n");
            boardfilename[0] = 0;
        }
        else
        {
            char *dot, *slash;

            boardfilename[0] = '/';
            boardfilename[1] = 0;
            Bstrcat(boardfilename, CommandMap);

            dot = Bstrrchr(boardfilename,'.');
            slash = Bstrrchr(boardfilename,'/');
            if (!slash) slash = Bstrrchr(boardfilename,'\\');

            if ((!slash && !dot) || (slash && dot < slash))
                Bstrcat(boardfilename,".map");

            Bcorrectfilename(boardfilename,0);

            i = kopen4loadfrommod(boardfilename,0);
            if (i!=-1)
            {
                initprintf("Using level: \"%s\".\n",boardfilename);
                kclose(i);
            }
            else
            {
                initprintf("Level \"%s\" not found.\n",boardfilename);
                boardfilename[0] = 0;
            }
        }
    }

    if (VOLUMEONE)
    {
        initprintf("*** You have run Duke Nukem 3D %d times. ***\n\n",ud.executions);

        if (ud.executions >= 50 && !DUKEBETA)
        {
            initprintf("IT IS NOW TIME TO UPGRADE TO THE COMPLETE VERSION!\n");

#ifdef _WIN32
            Bsprintf(tempbuf, "You have run Duke Nukem 3D shareware %d times.  It is now time to upgrade to the complete version!\n\n"
                     "Upgrade Duke Nukem 3D now?\n", ud.executions);

            if (wm_ynbox("Upgrade to the full version of Duke Nukem 3D","%s",tempbuf))
            {
                SHELLEXECUTEINFOA sinfo;
                char const *p = "http://store.steampowered.com/app/225140";

                Bmemset(&sinfo, 0, sizeof(sinfo));
                sinfo.cbSize = sizeof(sinfo);
                sinfo.fMask = SEE_MASK_CLASSNAME;
                sinfo.lpVerb = "open";
                sinfo.lpFile = p;
                sinfo.nShow = SW_SHOWNORMAL;
                sinfo.lpClass = "http";

                if (!ShellExecuteExA(&sinfo))
                    G_GameExit("Error launching default system browser!");

                quitevent = 1;
            }
#endif
        }
    }

    for (i=0; i<MAXPLAYERS; i++)
        g_player[i].pingcnt = 0;

    if (quitevent)
    {
        G_Shutdown();
        return;
    }

    Net_GetPackets();

    if (numplayers > 1)
        initprintf("Multiplayer initialized.\n");

    {
        char *cwd;

        if (g_modDir[0] != '/' && (cwd = getcwd(NULL, 0)))
        {
            Bchdir(g_modDir);
            if (loadpics("tiles000.art",MAXCACHE1DSIZE) < 0)
            {
                Bchdir(cwd);
                if (loadpics("tiles000.art",MAXCACHE1DSIZE) < 0)
                    G_GameExit("Failed loading art.");
            }
            Bchdir(cwd);
#ifndef __ANDROID__ //This crashes on *some* Android devices. Small onetime memory leak. TODO fix above function
            Bfree(cwd);
#endif

        }
        else if (loadpics("tiles000.art",MAXCACHE1DSIZE) < 0)
            G_GameExit("Failed loading art.");
    }

    // Make the fullscreen nuke logo background non-fullbright.  Has to be
    // after dynamic tile remapping (from C_Compile) and loading tiles.
    picanm[LOADSCREEN].sf |= PICANM_NOFULLBRIGHT_BIT;

//    initprintf("Loading palette/lookups...\n");
    G_LoadLookups();

    ReadSaveGameHeaders();

    tilesiz[MIRROR].x = tilesiz[MIRROR].y = 0;

    screenpeek = myconnectindex;

    Bfflush(NULL);
}

static void P_SetupMiscInputSettings(void)
{
    DukePlayer_t *ps = g_player[myconnectindex].ps;

    ps->aim_mode = ud.mouseaiming;
    ps->auto_aim = ud.config.AutoAim;
    ps->weaponswitch = ud.weaponswitch;
}

void G_UpdatePlayerFromMenu(void)
{
    if (ud.recstat != 0)
        return;

    if (numplayers > 1)
    {
        Net_SendClientInfo();
        if (sprite[g_player[myconnectindex].ps->i].picnum == APLAYER && sprite[g_player[myconnectindex].ps->i].pal != 1)
            sprite[g_player[myconnectindex].ps->i].pal = g_player[myconnectindex].pcolor;
    }
    else
    {
        /*int32_t j = g_player[myconnectindex].ps->team;*/

        P_SetupMiscInputSettings();
        g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = ud.color;

        g_player[myconnectindex].pteam = ud.team;

        if (sprite[g_player[myconnectindex].ps->i].picnum == APLAYER && sprite[g_player[myconnectindex].ps->i].pal != 1)
            sprite[g_player[myconnectindex].ps->i].pal = g_player[myconnectindex].pcolor;
    }
}

void G_BackToMenu(void)
{
    boardfilename[0] = 0;
    if (ud.recstat == 1) G_CloseDemoWrite();
    ud.warp_on = 0;
    g_player[myconnectindex].ps->gm = 0;
    M_OpenMenu(myconnectindex);
    M_ChangeMenu(MENU_MAIN);
    KB_FlushKeyboardQueue();
    G_UpdateAppTitle();
}

static int32_t G_EndOfLevel(void)
{
    P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);
    P_UpdateScreenPal(g_player[myconnectindex].ps);

    if (g_player[myconnectindex].ps->gm&MODE_EOL)
    {
        G_CloseDemoWrite();

        ready2send = 0;

        if (ud.display_bonus_screen == 1)
        {
            int32_t i = ud.screen_size;
            ud.screen_size = 0;
            G_UpdateScreenArea();
            ud.screen_size = i;
            G_BonusScreen(0);
        }

        // Clear potentially loaded per-map ART only after the bonus screens.
        E_MapArt_Clear();

        if (ud.eog)
        {
            ud.eog = 0;
            if ((!g_netServer && ud.multimode < 2))
            {
                if (!VOLUMEALL)
                    G_DoOrderScreen();
                g_player[myconnectindex].ps->gm = 0;
                M_OpenMenu(myconnectindex);
                M_ChangeMenu(MENU_MAIN);
                return 2;
            }
            else
            {
                ud.m_level_number = 0;
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
        G_BackToMenu();
        return 2;
    }

    Net_WaitForServer();
    return 1;

}

void app_crashhandler(void)
{
    G_CloseDemoWrite();
    VM_ScriptInfo(insptr, 64);
    G_GameQuit();
}

#if defined(_WIN32) && defined(DEBUGGINGAIDS)
// See FILENAME_CASE_CHECK in cache1d.c
static int32_t check_filename_casing(void)
{
    return !(g_player[myconnectindex].ps->gm&MODE_GAME);
}
#endif

#ifdef LUNATIC
const char *g_sizes_of_what[] = {
    "sectortype", "walltype", "spritetype", "spriteext_t",
    "actor_t", "DukePlayer_t", "playerdata_t",
    "user_defs", "tiledata_t", "weapondata_t",
    "projectile_t",
};
int32_t g_sizes_of[] = {
    sizeof(sectortype), sizeof(walltype), sizeof(spritetype), sizeof(spriteext_t),
    sizeof(actor_t), sizeof(DukePlayer_t), sizeof(playerdata_t),
    sizeof(user_defs), sizeof(tiledata_t), sizeof(weapondata_t),
    sizeof(projectile_t)
};

DukePlayer_t *g_player_ps[MAXPLAYERS];
#endif

void G_MaybeAllocPlayer(int32_t pnum)
{
    if (g_player[pnum].ps == NULL)
        g_player[pnum].ps = (DukePlayer_t *)Xcalloc(1, sizeof(DukePlayer_t));
    if (g_player[pnum].sync == NULL)
        g_player[pnum].sync = (input_t *)Xcalloc(1, sizeof(input_t));

#ifdef LUNATIC
    g_player_ps[pnum] = g_player[pnum].ps;
    g_player[pnum].ps->wa.idx = pnum;
#endif
}

EDUKE32_STATIC_ASSERT(sizeof(actor_t)==128);
EDUKE32_STATIC_ASSERT(sizeof(DukePlayer_t)%4 == 0);

int32_t app_main(int32_t argc, char const * const * argv)
{
    int32_t i = 0, j;

#ifndef NETCODE_DISABLE
    if (enet_initialize() != 0)
        initprintf("An error occurred while initializing ENet.\n");
    else atexit(enet_deinitialize);
#endif

#ifdef _WIN32
    if (!G_CheckCmdSwitch(argc, argv, "-noinstancechecking") && win_checkinstance())
    {
        if (!wm_ynbox("EDuke32","Another Build game is currently running. "
                      "Do you wish to continue starting this copy?"))
            return 3;
    }

    backgroundidle = 0;

#ifdef DEBUGGINGAIDS
    {
        extern int32_t (*check_filename_casing_fn)(void);
        check_filename_casing_fn = check_filename_casing;
    }
#endif
#endif

    G_ExtPreInit(argc, argv);

    OSD_SetParameters(0,0, 0,12, 2,12);

#ifdef __APPLE__
    if (!usecwd)
    {
    	char cwd[BMAX_PATH];
        char *homedir = Bgethomedir();
        if (homedir)
            Bsnprintf(cwd, sizeof(cwd), "%s/Library/Logs/eduke32.log", homedir);
        else
            Bstrcpy(cwd, "eduke32.log");
        OSD_SetLogFile(cwd);
        Bfree(homedir);
    }
    else
#endif
    OSD_SetLogFile("eduke32.log");

    OSD_SetFunctions(
        GAME_drawosdchar,
        GAME_drawosdstr,
        GAME_drawosdcursor,
        GAME_getcolumnwidth,
        GAME_getrowheight,
        GAME_clearbackground,
        BGetTime,
        GAME_onshowosd
    );

    wm_setapptitle(APPNAME);

    initprintf(HEAD2 " %s %s\n", s_buildRev, s_buildInfo);
    initprintf("Compiled %s\n", s_buildTimestamp);

    if (!usecwd)
        G_AddSearchPaths();

    g_numSkills = 4;
    ud.multimode = 1;

#ifdef __ANDROID__
    G_AddDef("duke3d-android.def");
#endif

    // This needs to happen before G_CheckCommandLine() because G_GameExit()
    // accesses g_player[0].
    G_MaybeAllocPlayer(0);

    G_CheckCommandLine(argc,argv);

    // This needs to happen afterwards, as G_CheckCommandLine() is where we set
    // up the command-line-provided search paths (duh).
    G_ExtInit();

#if defined(RENDERTYPEWIN) && defined(USE_OPENGL)
    if (forcegl) initprintf("GL driver blacklist disabled.\n");
#endif

    // used with binds for fast function lookup
    hash_init(&h_gamefuncs);
    for (i=NUMGAMEFUNCTIONS-1; i>=0; i--)
    {
        char *str = Bstrtolower(Xstrdup(gamefunctions[i]));
        hash_add(&h_gamefuncs,gamefunctions[i],i,0);
        hash_add(&h_gamefuncs,str,i,0);
        Bfree(str);
    }

#ifdef USE_OPENGL
    glusetexcache = -1;
#endif

    i = CONFIG_ReadSetup();

#ifdef _WIN32

//    initprintf("build %d\n",(uint8_t)Batoi(BUILDDATE));

    if (ud.config.CheckForUpdates == 1)
    {
        if (time(NULL) - ud.config.LastUpdateCheck > UPDATEINTERVAL)
        {
            initprintf("Checking for updates...\n");

            ud.config.LastUpdateCheck = time(NULL);

            if (G_GetVersionFromWebsite(tempbuf))
            {
                initprintf("Current version is %d",Batoi(tempbuf));

                if (Batoi(tempbuf) > atoi(s_buildDate))
                {
                    if (wm_ynbox("EDuke32","A new version of EDuke32 is available. "
                                 "Browse to http://www.eduke32.com now?"))
                    {
                        SHELLEXECUTEINFOA sinfo;
                        char const *p = "http://www.eduke32.com";

                        Bmemset(&sinfo, 0, sizeof(sinfo));
                        sinfo.cbSize = sizeof(sinfo);
                        sinfo.fMask = SEE_MASK_CLASSNAME;
                        sinfo.lpVerb = "open";
                        sinfo.lpFile = p;
                        sinfo.nShow = SW_SHOWNORMAL;
                        sinfo.lpClass = "http";

                        if (!ShellExecuteExA(&sinfo))
                            initprintf("update: error launching browser!\n");
                    }
                }
                else initprintf("... no updates available\n");
            }
            else initprintf("update: failed to check for updates\n");
        }
    }
#endif

#if defined(USE_OPENGL) && !defined(EDUKE32_GLES)
    if (glusetexcache == -1)
    {
        ud.config.useprecache = glusetexcompr = 1;
        glusetexcache = 2;
    }
#endif

    if (preinitengine())
    {
        wm_msgbox("Build Engine Initialization Error",
                  "There was a problem initializing the Build engine: %s", engineerrstr);
        ERRprintf("app_main: There was a problem initializing the Build engine: %s\n", engineerrstr);
        Bexit(2);
    }

    if (Bstrcmp(setupfilename, SETUPFILENAME))
        initprintf("Using config file \"%s\".\n",setupfilename);

    G_ScanGroups();

#ifdef STARTUP_SETUP_WINDOW
    if (i < 0 || (!g_noSetup && (ud.configversion != BYTEVERSION_EDUKE32 || ud.config.ForceSetup)) || g_commandSetup)
    {
        if (quitevent || !startwin_run())
        {
            uninitengine();
            Bexit(0);
        }
    }
#endif

    flushlogwindow = 0;
    G_LoadGroups(!g_noAutoLoad && !ud.config.NoAutoLoad);
//    flushlogwindow = 1;

    G_SetupCheats();

    if (!usecwd)
        G_CleanupSearchPaths();

    if (SHAREWARE)
        g_Shareware = 1;
    else
    {
        i = kopen4load("DUKESW.BIN",1); // JBF 20030810

        if (i != -1)
        {
            g_Shareware = 1;
            kclose(i);
        }
    }

    // gotta set the proper title after we compile the CONs if this is the full version

    G_UpdateAppTitle();

    if (g_scriptDebug)
        initprintf("CON debugging activated (level %d).\n",g_scriptDebug);

#ifndef NETCODE_DISABLE
    if (g_networkMode == NET_SERVER || g_networkMode == NET_DEDICATED_SERVER)
    {
        ENetAddress address = { ENET_HOST_ANY, g_netPort };
        g_netServer = enet_host_create(&address, MAXPLAYERS, CHAN_MAX, 0, 0);

        if (g_netServer == NULL)
            initprintf("An error occurred while trying to create an ENet server host.\n");
        else initprintf("Multiplayer server initialized\n");
    }
#endif
    numplayers = 1;
    playerswhenstarted = ud.multimode;  // Lunatic needs this (player[] bound)

    if (!g_fakeMultiMode)
    {
        connectpoint2[0] = -1;
    }
    else
    {
        for (i=0; i<ud.multimode-1; i++)
            connectpoint2[i] = i+1;
        connectpoint2[ud.multimode-1] = -1;

        for (i=1; i<ud.multimode; i++)
            g_player[i].playerquitflag = 1;
    }

    Net_GetPackets();

    // NOTE: Allocating the DukePlayer_t structs has to be before compiling scripts,
    // because in Lunatic, the {pipe,trip}bomb* members are initialized.
    for (i=0; i<MAXPLAYERS; i++)
        G_MaybeAllocPlayer(i);

    G_Startup(); // a bunch of stuff including compiling cons

    g_player[0].playerquitflag = 1;

    g_player[myconnectindex].ps->palette = BASEPAL;

    i = 1;
    for (j=numplayers; j<ud.multimode; j++)
    {
        Bsprintf(g_player[j].user_name,"PLAYER %d",j+1);
        g_player[j].ps->team = g_player[j].pteam = i;
        g_player[j].ps->weaponswitch = 3;
        g_player[j].ps->auto_aim = 0;
        i = 1-i;
    }

    if (quitevent) return 4;

    Anim_Init();

    const char *defsfile = G_DefFile();
    uint32_t stime = getticks();
    if (!loaddefinitionsfile(defsfile))
    {
        uint32_t etime = getticks();
        initprintf("Definitions file \"%s\" loaded in %d ms.\n", defsfile, etime-stime);
        loaddefinitions_game(defsfile, FALSE);
    }

    for (i=0; i < g_defModulesNum; ++i)
        Bfree(g_defModules[i]);
    DO_FREE_AND_NULL(g_defModules);
    g_defModulesNum = 0;

    if (E_PostInit())
        G_FatalEngineError();

    G_PostLoadPalette();

    Gv_ResetSystemDefaults(); // called here to populate our fake tilesizx and tilesizy arrays presented to CON with sizes generated by dummytiles

    if (numplayers == 1 && boardfilename[0] != 0)
    {
        ud.m_level_number = 7;
        ud.m_volume_number = 0;
        ud.warp_on = 1;
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
    ud.last_level = 0;

    // the point of this block is to avoid overwriting the default in the cfg while asserting our selection
    if (g_rtsNamePtr == NULL &&
            (!Bstrcasecmp(ud.rtsname,defaultrtsfilename[GAME_DUKE]) ||
            !Bstrcasecmp(ud.rtsname,defaultrtsfilename[GAME_WW2GI]) ||
            !Bstrcasecmp(ud.rtsname,defaultrtsfilename[GAME_NAM]) ||
            !Bstrcasecmp(ud.rtsname,defaultrtsfilename[GAME_NAPALM])))
    {
        // ud.last_level is used as a flag here to reset the string to default after load
        ud.last_level = (Bstrcpy(ud.rtsname, G_DefaultRtsFile()) == ud.rtsname);
    }

    RTS_Init(ud.rtsname);

    if (RTS_IsInitialized())
        initprintf("Using RTS file \"%s\".\n", ud.rtsname);

    if (ud.last_level)
        Bstrcpy(ud.rtsname, defaultrtsfilename[0]);

    ud.last_level = -1;

    initprintf("Initializing OSD...\n");

    Bsprintf(tempbuf, HEAD2 " %s", s_buildRev);
    OSD_SetVersion(tempbuf, 10,0);
    registerosdcommands();

    if (g_networkMode != NET_DEDICATED_SERVER)
    {
        if (CONTROL_Startup(controltype_keyboardandmouse, &BGetTime, TICRATE))
        {
            ERRprintf("There was an error initializing the CONTROL system.\n");
            uninitengine();
            Bexit(5);
        }

        G_SetupGameButtons();
        CONFIG_SetupMouse();
        CONFIG_SetupJoystick();

        CONTROL_JoystickEnabled = (ud.config.UseJoystick && CONTROL_JoyPresent);
        CONTROL_MouseEnabled = (ud.config.UseMouse && CONTROL_MousePresent);

        // JBF 20040215: evil and nasty place to do this, but joysticks are evil and nasty too
        for (i=0; i<joynumaxes; i++)
            setjoydeadzone(i,ud.config.JoystickAnalogueDead[i],ud.config.JoystickAnalogueSaturate[i]);
    }

#ifdef HAVE_CLIPSHAPE_FEATURE
    if ((i = clipmapinfo_load()) > 0)
        initprintf("There was an error loading the sprite clipping map (status %d).\n", i);

    for (i=0; i < g_clipMapFilesNum; ++i)
        Bfree(g_clipMapFiles[i]);
    DO_FREE_AND_NULL(g_clipMapFiles);
    g_clipMapFilesNum = 0;
#endif

    // check if the minifont will support lowercase letters (3136-3161)
    // there is room for them in tiles012.art between "[\]^_." and "{|}~"
    minitext_lowercase = 1;
    for (i = MINIFONT + ('a'-'!'); minitext_lowercase && i < MINIFONT + ('z'-'!') + 1; ++i)
        minitext_lowercase &= tile_exists(i);

    system_getcvars();

    char *ptr = Xstrdup(setupfilename), *p = strtok(ptr, ".");

    if (!Bstrcmp(setupfilename, SETUPFILENAME))
        Bsprintf(tempbuf, "settings.cfg");
    else
        Bsprintf(tempbuf, "%s_settings.cfg", p);

    OSD_Exec(tempbuf);
    Bfree(ptr);

    if (g_networkMode != NET_DEDICATED_SERVER)
    {
        if (setgamemode(ud.config.ScreenMode,ud.config.ScreenWidth,ud.config.ScreenHeight,ud.config.ScreenBPP) < 0)
        {
            int32_t i = 0;
            int32_t xres[] = {ud.config.ScreenWidth,800,640,320};
            int32_t yres[] = {ud.config.ScreenHeight,600,480,240};
            int32_t bpp[] = {32,16,8};

            initprintf("Failure setting video mode %dx%dx%d %s! Attempting safer mode...\n",
                       ud.config.ScreenWidth,ud.config.ScreenHeight,ud.config.ScreenBPP,ud.config.ScreenMode?"fullscreen":"windowed");

#ifdef USE_OPENGL
            {
                int32_t j = 0;
                while (setgamemode(0,xres[i],yres[i],bpp[j]) < 0)
                {
                    initprintf("Failure setting video mode %dx%dx%d windowed! Attempting safer mode...\n",xres[i],yres[i],bpp[i]);

                    if (++j == 3)
                    {
                        if (++i == 4)
                            G_GameExit("Unable to set failsafe video mode!");
                        j = 0;
                    }
                }
            }
#else
            while (setgamemode(0,xres[i],yres[i],8) < 0)
            {
                initprintf("Failure setting video mode %dx%dx%d windowed! Attempting safer mode...\n",xres[i],yres[i],8);
                i++;
            }
#endif
            ud.config.ScreenWidth = xres[i];
            ud.config.ScreenHeight = yres[i];
            ud.config.ScreenBPP = bpp[i];
        }

        setbrightness(ud.brightness>>2,g_player[myconnectindex].ps->palette,0);

        S_MusicStartup();
        S_SoundStartup();
    }
//    loadtmb();

    OSD_Exec("autoexec.cfg");

    M_Init();

    if (ud.warp_on > 1 && (!g_netServer && ud.multimode < 2))
    {
        clearview(0L);
        //g_player[myconnectindex].ps->palette = palette;
        //G_FadePalette(0,0,0,0);
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308
        rotatesprite_fs(160<<16,100<<16,65536L,0,LOADSCREEN,0,0,2+8+64+BGSTRETCH);
        menutext(160,105,0,0,"LOADING SAVED GAME...");
        nextpage();

        if (G_LoadPlayer(ud.warp_on-2))
            ud.warp_on = 0;
    }

    FX_StopAllSounds();
    S_ClearSoundLocks();

    //    getpackets();

MAIN_LOOP_RESTART:
    totalclock = 0;
    ototalclock = 0;
    lockclock = 0;

    g_player[myconnectindex].ps->fta = 0;

    M_ChangeMenu(MENU_MAIN);

    if (g_networkMode != NET_DEDICATED_SERVER)
    {
        G_GetCrosshairColor();
        G_SetCrosshairColor(CrosshairColors.r, CrosshairColors.g, CrosshairColors.b);
    }

    if (ud.warp_on == 0)
    {
        if ((g_netServer || ud.multimode > 1) && boardfilename[0] != 0)
        {
            ud.m_level_number = 7;
            ud.m_volume_number = 0;

            if (ud.m_player_skill == 4)
                ud.m_respawn_monsters = 1;
            else ud.m_respawn_monsters = 0;

            for (TRAVERSE_CONNECT(i))
            {
                P_ResetWeapons(i);
                P_ResetInventory(i);
            }

            G_NewGame_EnterLevel();

            Net_WaitForServer();
        }
        else if (g_networkMode != NET_DEDICATED_SERVER)
            G_DisplayLogo();

        if (g_networkMode != NET_DEDICATED_SERVER)
        {
            if (G_PlaybackDemo())
            {
                FX_StopAllSounds();
                S_ClearSoundLocks();
                g_noLogoAnim = 1;
                goto MAIN_LOOP_RESTART;
            }
        }
    }
    else if (ud.warp_on == 1)
    {
        G_NewGame_EnterLevel();
    }
    else G_UpdateScreenArea();

//    G_GameExit(" "); ///

//    ud.auto_run = ud.config.RunMode;
    ud.showweapons = ud.config.ShowOpponentWeapons;
    P_SetupMiscInputSettings();
    g_player[myconnectindex].pteam = ud.team;

    if (GametypeFlags[ud.coop] & GAMETYPE_TDM)
        g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = G_GetTeamPalette(g_player[myconnectindex].pteam);
    else
    {
        if (ud.color) g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = ud.color;
        else g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor;
    }

    ud.warp_on = 0;
    KB_KeyDown[sc_Pause] = 0;   // JBF: I hate the pause key

    do //main loop
    {
        static uint32_t nextrender = 0, framewaiting = 0;
        uint32_t j;

        if (handleevents() && quitevent)
        {
            KB_KeyDown[sc_Escape] = 1;
            quitevent = 0;
        }

        sampletimer();
        Net_GetPackets();

        // only allow binds to function if the player is actually in a game (not in a menu, typing, et cetera) or demo
        CONTROL_BindsEnabled = g_player[myconnectindex].ps->gm & (MODE_GAME|MODE_DEMO);

#ifndef _WIN32
        // stdin -> OSD input for dedicated server
        if (g_networkMode == NET_DEDICATED_SERVER)
        {
            int32_t nb;
            char ch;
            static uint32_t bufpos = 0;
            static char buf[128];
#ifndef GEKKO
            int32_t flag = 1;
            ioctl(0, FIONBIO, &flag);
#endif
            if ((nb = read(0, &ch, 1)) > 0 && bufpos < sizeof(buf))
            {
                if (ch != '\n')
                    buf[bufpos++] = ch;

                if (ch == '\n' || bufpos >= sizeof(buf))
                {
                    buf[bufpos] = 0;
                    OSD_Dispatch(buf);
                    bufpos = 0;
                }
            }
        }
        else
#endif
        {
            MUSIC_Update();
            G_HandleLocalKeys();
        }

        OSD_DispatchQueued();

        if (((g_netClient || g_netServer) || !(g_player[myconnectindex].ps->gm & (MODE_MENU|MODE_DEMO))) && totalclock >= ototalclock+TICSPERFRAME)
        {
            if (g_networkMode != NET_DEDICATED_SERVER)
            {
                CONTROL_ProcessBinds();
                P_GetInput(myconnectindex);
            }

            avg.fvel += loc.fvel;
            avg.svel += loc.svel;
            avg.avel += loc.avel;
            avg.horz += loc.horz;
            avg.bits |= loc.bits;
            avg.extbits |= loc.extbits;

            Bmemcpy(&inputfifo[0][myconnectindex], &avg, sizeof(input_t));
            Bmemset(&avg, 0, sizeof(input_t));

            do
            {
                int32_t clockbeforetic;

                sampletimer();

                if (ready2send == 0) break;

                ototalclock += TICSPERFRAME;

                clockbeforetic = totalclock;

                if (((ud.show_help == 0 && (g_player[myconnectindex].ps->gm&MODE_MENU) != MODE_MENU) || ud.recstat == 2 || (g_netServer || ud.multimode > 1)) &&
                        (g_player[myconnectindex].ps->gm&MODE_GAME))
                {
                    G_MoveLoop();
#ifdef __ANDROID__
                    inputfifo[0][myconnectindex].fvel = 0;
                    inputfifo[0][myconnectindex].svel = 0;
                    inputfifo[0][myconnectindex].avel = 0;
                    inputfifo[0][myconnectindex].horz = 0;
#endif
                }

                sampletimer();

                if (totalclock - clockbeforetic >= TICSPERFRAME)
                {
                    // computing a tic takes longer than a tic, so we're slowing
                    // the game down. rather than tightly spinning here, go draw
                    // a frame since we're fucked anyway
                    break;
                }
            }
            while (((g_netClient || g_netServer) || !(g_player[myconnectindex].ps->gm & (MODE_MENU|MODE_DEMO))) && totalclock >= ototalclock+TICSPERFRAME);
        }

        G_DoCheats();

        if (g_player[myconnectindex].ps->gm & (MODE_EOL|MODE_RESTART))
        {
            switch (G_EndOfLevel())
            {
            case 1: continue;
            case 2: goto MAIN_LOOP_RESTART;
            }
        }

        if (g_networkMode == NET_DEDICATED_SERVER)
        {
            idle();
            goto skipframe;
        }

        if (framewaiting)
        {
            framewaiting--;
            nextpage();
        }

        j = getticks();

        if (r_maxfps == 0 || j >= nextrender)
        {
            if (j > nextrender+g_frameDelay)
                nextrender = j;

            nextrender += g_frameDelay;

            if ((ud.show_help == 0 && (!g_netServer && ud.multimode < 2) && !(g_player[myconnectindex].ps->gm&MODE_MENU)) ||
                    (g_netServer || ud.multimode > 1) || ud.recstat == 2)
                i = calc_smoothratio(totalclock, ototalclock);
            else
                i = 65536;

            G_DrawRooms(screenpeek,i);
            if (getrendermode() >= REND_POLYMOST)
                G_DrawBackground();
            G_DisplayRest(i);

            framewaiting++;
        }

skipframe:
        if (g_player[myconnectindex].ps->gm&MODE_DEMO)
            goto MAIN_LOOP_RESTART;
    }
    while (1);

    G_GameExit(" ");
    return 0;  // not reached (duh)
}

GAME_STATIC GAME_INLINE int32_t G_MoveLoop()
{
    Net_GetPackets();

    return G_DoMoveThings();
}

int32_t G_DoMoveThings(void)
{
    int32_t i;

    ud.camerasprite = -1;
    lockclock += TICSPERFRAME;

    // Moved lower so it is restored correctly by demo diffs:
    //if (g_earthquakeTime > 0) g_earthquakeTime--;

    if (g_RTSPlaying > 0)
        g_RTSPlaying--;

    for (i=0; i<MAXUSERQUOTES; i++)
        if (user_quote_time[i])
        {
            user_quote_time[i]--;
            if (user_quote_time[i] > ud.msgdisptime)
                user_quote_time[i] = ud.msgdisptime;
            if (!user_quote_time[i]) pub = NUMPAGES;
        }

    // Name display when aiming at opponents
    if (ud.idplayers && (g_netServer || ud.multimode > 1)
#ifdef SPLITSCREEN_MOD_HACKS
        && !g_fakeMultiMode
#endif
        )
    {
        hitdata_t hit;
        DukePlayer_t *const p = g_player[screenpeek].ps;

        for (i=0; i<ud.multimode; i++)
            if (g_player[i].ps->holoduke_on != -1)
                sprite[g_player[i].ps->holoduke_on].cstat ^= 256;

        hitscan((vec3_t *)p,p->cursectnum,
                sintable[(p->ang+512)&2047],
                sintable[p->ang&2047],
                (100-p->horiz-p->horizoff)<<11,&hit,0xffff0030);

        for (i=0; i<ud.multimode; i++)
            if (g_player[i].ps->holoduke_on != -1)
                sprite[g_player[i].ps->holoduke_on].cstat ^= 256;

        if ((hit.sprite >= 0) && !(g_player[myconnectindex].ps->gm & MODE_MENU) &&
                sprite[hit.sprite].picnum == APLAYER)
        {
            const int32_t snum = P_Get(hit.sprite);

            if (snum != screenpeek && g_player[snum].ps->dead_flag == 0)
            {
                if (p->fta == 0 || p->ftq == QUOTE_RESERVED3)
                {
                    if (ldist(&sprite[p->i], &sprite[hit.sprite]) < 9216)
                    {
                        Bsprintf(ScriptQuotes[QUOTE_RESERVED3], "%s", &g_player[snum].user_name[0]);
                        p->fta = 12, p->ftq = QUOTE_RESERVED3;
                    }
                }
                else if (p->fta > 2) p->fta -= 3;
            }
        }
    }

    if (g_showShareware > 0)
    {
        g_showShareware--;
        if (g_showShareware == 0)
        {
            pus = NUMPAGES;
            pub = NUMPAGES;
        }
    }

    // Moved lower so it is restored correctly by diffs:
//    everyothertime++;

    if (g_netServer || g_netClient)
        randomseed = ticrandomseed;

    for (TRAVERSE_CONNECT(i))
        Bmemcpy(g_player[i].sync, &inputfifo[(g_netServer && myconnectindex == i)][i],
                sizeof(input_t));

    G_UpdateInterpolations();

    /*
        j = -1;
        for (TRAVERSE_CONNECT(i))
        {
            if (g_player[i].playerquitflag == 0 || TEST_SYNC_KEY(g_player[i].sync->bits,SK_GAMEQUIT) == 0)
            {
                j = i;
                continue;
            }

            G_CloseDemoWrite();

            g_player[i].playerquitflag = 0;
        }
    */

    g_moveThingsCount++;

    if (ud.recstat == 1) G_DemoRecord();

    everyothertime++;
    if (g_earthquakeTime > 0) g_earthquakeTime--;

    if (ud.pause_on == 0)
    {
        g_globalRandom = krand();
        A_MoveDummyPlayers();//ST 13
    }

    for (TRAVERSE_CONNECT(i))
    {
        if (g_player[i].sync->extbits&(1<<6))
        {
            g_player[i].ps->team = g_player[i].pteam;
            if (GametypeFlags[ud.coop] & GAMETYPE_TDM)
            {
                actor[g_player[i].ps->i].picnum = APLAYERTOP;
                P_QuickKill(g_player[i].ps);
            }
        }
        if (GametypeFlags[ud.coop] & GAMETYPE_TDM)
            g_player[i].ps->palookup = g_player[i].pcolor = G_GetTeamPalette(g_player[i].ps->team);

        if (sprite[g_player[i].ps->i].pal != 1)
            sprite[g_player[i].ps->i].pal = g_player[i].pcolor;

        P_HandleSharedKeys(i);

        if (ud.pause_on == 0)
        {
            P_ProcessInput(i);
            P_CheckSectors(i);
        }
    }

    if (ud.pause_on == 0)
        G_MoveWorld();

//    Net_CorrectPrediction();

    if (g_netServer)
        Net_SendServerUpdates();

    if ((everyothertime&1) == 0)
    {
        G_AnimateWalls();
        A_MoveCyclers();

        if (g_netServer && (everyothertime % 10) == 0)
		{
            Net_SendMapUpdate();
		}
    }

    if (g_netClient)   //Slave
        Net_SendClientUpdate();

    return 0;
}

static void G_DoOrderScreen(void)
{
    int32_t i;

    setview(0,0,xdim-1,ydim-1);

    P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 1);    // JBF 20040308

    for (i=0; i<4; i++)
    {
        fadepal(0,0,0, 0,252,28);
        I_ClearAllInput();
        rotatesprite_fs(160<<16,100<<16,65536L,0,ORDERING+i,0,0,2+8+64+BGSTRETCH);
        fadepal(0,0,0, 252,0,-28);
        while (!I_CheckAllInput())
            G_HandleAsync();
    }

    I_ClearAllInput();
}


static void G_BonusCutscenes(void)
{
    int32_t bonuscnt=0;
    int32_t t;

    int32_t breathe[] =
    {
         0,  30, VICTORY1+1, 176, 59,
        30,  60, VICTORY1+2, 176, 59,
        60,  90, VICTORY1+1, 176, 59,
        90, 120,          0, 176, 59
    };

    int32_t bossmove[] =
    {
          0, 120, VICTORY1+3, 86, 59,
        220, 260, VICTORY1+4, 86, 59,
        260, 290, VICTORY1+5, 86, 59,
        290, 320, VICTORY1+6, 86, 59,
        320, 350, VICTORY1+7, 86, 59,
        350, 380, VICTORY1+8, 86, 59,
        350, 380, VICTORY1+8, 86, 59 // duplicate row to alleviate overflow in the for loop below "boss"
    };

    if (!(numplayers < 2 && ud.eog && ud.from_bonus == 0))
        return;

    switch (ud.volume_number)
    {
    case 0:
        if ((G_GetLogoFlags() & LOGO_NOE1BONUSSCENE) && (G_GetLogoFlags() & LOGO_NOE1ENDSCREEN))
            return;

        if (ud.lockout == 0 && !(G_GetLogoFlags() & LOGO_NOE1BONUSSCENE))
        {
            P_SetGamePalette(g_player[myconnectindex].ps, ENDINGPAL, 8+2+1); // JBF 20040308
            clearallviews(0L);
            rotatesprite_fs(0,50<<16,65536L,0,VICTORY1,0,0,2+8+16+64+128+BGSTRETCH);
            nextpage();
            fadepal(0,0,0, 252,0,-4);

            I_ClearAllInput();
            totalclock = 0;

            while (1)
            {
                clearallviews(0L);
                rotatesprite_fs(0,50<<16,65536L,0,VICTORY1,0,0,2+8+16+64+128+BGSTRETCH);

                // boss
                if (totalclock > 390 && totalclock < 780)
                    for (t=0; t<35; t+=5) if (bossmove[t+2] && (totalclock%390) > bossmove[t] && (totalclock%390) <= bossmove[t+1])
                    {
                        if (t==10 && bonuscnt == 1)
                        {
                            S_PlaySound(SHOTGUN_FIRE);
                            S_PlaySound(SQUISHED);
                            bonuscnt++;
                        }
                        rotatesprite_fs(bossmove[t+3]<<16,bossmove[t+4]<<16,65536L,0,bossmove[t+2],0,0,2+8+16+64+128+BGSTRETCH);
                    }

                // Breathe
                if (totalclock < 450 || totalclock >= 750)
                {
                    if (totalclock >= 750)
                    {
                        rotatesprite_fs(86<<16,59<<16,65536L,0,VICTORY1+8,0,0,2+8+16+64+128+BGSTRETCH);
                        if (totalclock >= 750 && bonuscnt == 2)
                        {
                            S_PlaySound(DUKETALKTOBOSS);
                            bonuscnt++;
                        }

                    }
                    for (t=0; t<20; t+=5)
                        if (breathe[t+2] && (totalclock%120) > breathe[t] && (totalclock%120) <= breathe[t+1])
                        {
                            if (t==5 && bonuscnt == 0)
                            {
                                S_PlaySound(BOSSTALKTODUKE);
                                bonuscnt++;
                            }
                            rotatesprite_fs(breathe[t+3]<<16,breathe[t+4]<<16,65536L,0,breathe[t+2],0,0,2+8+16+64+128+BGSTRETCH);
                        }
                }

                G_HandleAsync();

                nextpage();
                if (I_CheckAllInput()) break;
            }

            fadepal(0,0,0, 0,252,4);
        }

        if (G_GetLogoFlags() & LOGO_NOE1ENDSCREEN)
            goto VOL1_END;

        I_ClearAllInput();
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 8+2+1);   // JBF 20040308

        rotatesprite_fs(160<<16,100<<16,65536L,0,3292,0,0,2+8+64+BGSTRETCH);
        fadepal(0,0,0, 252,0,-4);
        G_HandleEventsWhileNoInput();
        fadepal(0,0,0, 0,252,4);

VOL1_END:
        S_StopMusic();
        FX_StopAllSounds();
        S_ClearSoundLocks();
        break;

    case 1:
        if ((G_GetLogoFlags() & LOGO_NOE2BONUSSCENE) && (G_GetLogoFlags() & LOGO_NOE2ENDSCREEN))
            return;

        setview(0,0,xdim-1,ydim-1);

        S_StopMusic();
        clearallviews(0L);
        nextpage();

        if (ud.lockout == 0 && !(G_GetLogoFlags() & LOGO_NOE2BONUSSCENE))
        {
            fadepal(0,0,0, 252,0,-4);
            Anim_Play("cineov2.anm");
            I_ClearAllInput();
            clearallviews(0L);
            nextpage();

            S_PlaySound(PIPEBOMB_EXPLODE);
            fadepal(0,0,0, 0,252,4);
        }

        if (G_GetLogoFlags() & LOGO_NOE2ENDSCREEN)
            return;

        I_ClearAllInput();
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 8+2+1);   // JBF 20040308
        rotatesprite_fs(160<<16,100<<16,65536L,0,3293,0,0,2+8+64+BGSTRETCH);
        fadepal(0,0,0, 252,0,-4);
        G_HandleEventsWhileNoInput();
        fadepal(0,0,0, 0,252,4);

        break;

    case 3:
        if ((G_GetLogoFlags() & LOGO_NOE4BONUSSCENE) && (G_GetLogoFlags() & LOGO_NODUKETEAMTEXT) && (G_GetLogoFlags() & LOGO_NODUKETEAMPIC))
            return;

        setview(0,0,xdim-1,ydim-1);

        S_StopMusic();
        clearallviews(0L);
        nextpage();

        if (ud.lockout == 0 && !(G_GetLogoFlags() & LOGO_NOE4BONUSSCENE))
        {
            fadepal(0,0,0, 252,0,-4);

            I_ClearAllInput();
            t = Anim_Play("vol4e1.anm");
            clearallviews(0L);
            nextpage();
            if (t)
                goto end_vol4e;

            t = Anim_Play("vol4e2.anm");
            clearallviews(0L);
            nextpage();
            if (t)
                goto end_vol4e;

            Anim_Play("vol4e3.anm");
            clearallviews(0L);
            nextpage();
        }

end_vol4e:
        if ((G_GetLogoFlags() & LOGO_NODUKETEAMTEXT) && (G_GetLogoFlags() & LOGO_NODUKETEAMPIC))
            goto VOL4_END;

        FX_StopAllSounds();
        S_ClearSoundLocks();
        S_PlaySound(ENDSEQVOL3SND4);
        I_ClearAllInput();

        if (G_GetLogoFlags() & LOGO_NODUKETEAMTEXT)
            goto VOL4_DUKETEAM;

        G_FadePalette(0,0,0,0);
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 8+2+1);   // JBF 20040308
//        G_FadePalette(0,0,0,252);
        clearallviews(0L);
        menutext(160,60,0,0,"Thanks to all our");
        menutext(160,60+16,0,0,"fans for giving");
        menutext(160,60+16+16,0,0,"us big heads.");
        menutext(160,70+16+16+16,0,0,"Look for a Duke Nukem 3D");
        menutext(160,70+16+16+16+16,0,0,"sequel soon.");
        nextpage();

        fadepal(0,0,0, 252,0,-12);
        nextpage();
        I_ClearAllInput();
        G_HandleEventsWhileNoInput();
        fadepal(0,0,0, 0,252,12);

        if (G_GetLogoFlags() & LOGO_NODUKETEAMPIC)
            goto VOL4_END;

VOL4_DUKETEAM:
        clearallviews(0L);
        nextpage();

        Anim_Play("DUKETEAM.ANM");

        I_ClearAllInput();
        G_HandleEventsWhileNoInput();

        clearallviews(0L);
        nextpage();
        G_FadePalette(0,0,0,252);

VOL4_END:
        FX_StopAllSounds();
        S_ClearSoundLocks();
        I_ClearAllInput();

        break;

    case 2:
        if ((G_GetLogoFlags() & LOGO_NOE3BONUSSCENE) && (G_GetLogoFlags() & LOGO_NOE3RADLOGO) && (PLUTOPAK || (G_GetLogoFlags() & LOGO_NODUKETEAMPIC)))
            return;

        S_StopMusic();
        clearallviews(0L);
        nextpage();
        if (ud.lockout == 0 && !(G_GetLogoFlags() & LOGO_NOE3BONUSSCENE))
        {
            fadepal(0,0,0, 252,0,-4);
            Anim_Play("cineov3.anm");
            I_ClearAllInput();
            ototalclock = totalclock+200;
            while (totalclock < ototalclock)
                G_HandleAsync();
            clearallviews(0L);
            nextpage();

            FX_StopAllSounds();
            S_ClearSoundLocks();
        }

        if (G_GetLogoFlags() & LOGO_NOE3RADLOGO)
            goto ENDANM;

        Anim_Play("RADLOGO.ANM");

        if (ud.lockout == 0 && !I_CheckAllInput())
        {
            if (G_PlaySoundWhileNoInput(ENDSEQVOL3SND5)) goto ENDANM;
            if (G_PlaySoundWhileNoInput(ENDSEQVOL3SND6)) goto ENDANM;
            if (G_PlaySoundWhileNoInput(ENDSEQVOL3SND7)) goto ENDANM;
            if (G_PlaySoundWhileNoInput(ENDSEQVOL3SND8)) goto ENDANM;
            if (G_PlaySoundWhileNoInput(ENDSEQVOL3SND9)) goto ENDANM;
        }

        I_ClearAllInput();

        totalclock = 0;
        if (PLUTOPAK || (G_GetLogoFlags() & LOGO_NODUKETEAMPIC))
        {
            while (totalclock < 120 && !I_CheckAllInput())
                G_HandleAsync();

            I_ClearAllInput();
        }
        else
        {
            G_HandleEventsWhileNoInput();
        }

ENDANM:
        if (!PLUTOPAK && !(G_GetLogoFlags() & LOGO_NODUKETEAMPIC))
        {
            FX_StopAllSounds();
            S_ClearSoundLocks();
            S_PlaySound(ENDSEQVOL3SND4);

            clearallviews(0L);
            nextpage();

            Anim_Play("DUKETEAM.ANM");

            I_ClearAllInput();
            G_HandleEventsWhileNoInput();

            clearallviews(0L);
            nextpage();
            G_FadePalette(0,0,0,252);
        }

        I_ClearAllInput();
        FX_StopAllSounds();
        S_ClearSoundLocks();

        clearallviews(0L);

        break;
    }
}

static void G_DisplayMPResultsScreen(void)
{
    int32_t i, y, t = 0;

    rotatesprite_fs(160<<16,100<<16,65536L,0,MENUSCREEN,16,0,2+8+64+BGSTRETCH);
    rotatesprite_fs(160<<16,34<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10);
    if (PLUTOPAK)   // JBF 20030804
        rotatesprite_fs((260)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,0,0,2+8);
    gametext(160,58+2,"Multiplayer Totals",0,2+8+16);
    gametext(160,58+10,MapInfo[G_LastMapInfoIndex()].name,0,2+8+16);

    gametext(160,165,"Press any key or button to continue",quotepulseshade,2+8+16);

    minitext(23,80,"   Name                                         Kills",8,2+8+16+128);
    for (i=0; i<playerswhenstarted; i++)
    {
        Bsprintf(tempbuf,"%-4d",i+1);
        minitext(92+(i*23),80,tempbuf,3,2+8+16+128);
    }

    for (i=0; i<playerswhenstarted; i++)
    {
        int32_t xfragtotal = 0;
        Bsprintf(tempbuf,"%d",i+1);

        minitext(30,90+t,tempbuf,0,2+8+16+128);
        minitext(38,90+t,g_player[i].user_name,g_player[i].ps->palookup,2+8+16+128);

        for (y=0; y<playerswhenstarted; y++)
        {
            if (i == y)
            {
                Bsprintf(tempbuf,"%-4d",g_player[y].ps->fraggedself);
                minitext(92+(y*23),90+t,tempbuf,2,2+8+16+128);
                xfragtotal -= g_player[y].ps->fraggedself;
            }
            else
            {
                Bsprintf(tempbuf,"%-4d",g_player[i].frags[y]);
                minitext(92+(y*23),90+t,tempbuf,0,2+8+16+128);
                xfragtotal += g_player[i].frags[y];
            }
        }

        Bsprintf(tempbuf,"%-4d",xfragtotal);
        minitext(101+(8*23),90+t,tempbuf,2,2+8+16+128);

        t += 7;
    }

    for (y=0; y<playerswhenstarted; y++)
    {
        int32_t yfragtotal = 0;
        for (i=0; i<playerswhenstarted; i++)
        {
            if (i == y)
                yfragtotal += g_player[i].ps->fraggedself;
            yfragtotal += g_player[i].frags[y];
        }
        Bsprintf(tempbuf,"%-4d",yfragtotal);
        minitext(92+(y*23),96+(8*7),tempbuf,2,2+8+16+128);
    }

    minitext(45,96+(8*7),"Deaths",8,2+8+16+128);
}

static int32_t G_PrintTime_ClockPad(void)
{
    int32_t clockpad = 2;
    int32_t ii, ij;

    for (ii=g_player[myconnectindex].ps->player_par/(REALGAMETICSPERSEC*60), ij=1; ii>9; ii/=10, ij++) ;
    clockpad = max(clockpad,ij);
    if (!(ud.volume_number == 0 && ud.last_level-1 == 7 && boardfilename[0]))
    {
        for (ii=MapInfo[G_LastMapInfoIndex()].partime/(REALGAMETICSPERSEC*60), ij=1; ii>9; ii/=10, ij++) ;
        clockpad = max(clockpad,ij);
        if (!NAM_WW2GI && MapInfo[G_LastMapInfoIndex()].designertime)
        {
            for (ii=MapInfo[G_LastMapInfoIndex()].designertime/(REALGAMETICSPERSEC*60), ij=1; ii>9; ii/=10, ij++) ;
            clockpad = max(clockpad,ij);
        }
    }
    if (ud.playerbest > 0) for (ii=ud.playerbest/(REALGAMETICSPERSEC*60), ij=1; ii>9; ii/=10, ij++) ;
    clockpad = max(clockpad,ij);

    return clockpad;
}

static const char* G_PrintTime2(int32_t time)
{
    Bsprintf(tempbuf, "%0*d:%02d", G_PrintTime_ClockPad(), time/(REALGAMETICSPERSEC*60), (time/REALGAMETICSPERSEC)%60);
    return tempbuf;
}
static const char* G_PrintTime3(int32_t time)
{
    Bsprintf(tempbuf, "%0*d:%02d.%02d", G_PrintTime_ClockPad(), time/(REALGAMETICSPERSEC*60), (time/REALGAMETICSPERSEC)%60, ((time%REALGAMETICSPERSEC)*33)/10);
    return tempbuf;
}

const char* G_PrintYourTime(void)
{
    return G_PrintTime3(g_player[myconnectindex].ps->player_par);
}
const char* G_PrintParTime(void)
{
    if (ud.last_level < 1)
        return "<invalid>";
    return G_PrintTime2(MapInfo[G_LastMapInfoIndex()].partime);
}
const char* G_PrintDesignerTime(void)
{
    if (ud.last_level < 1)
        return "<invalid>";
    return G_PrintTime2(MapInfo[G_LastMapInfoIndex()].designertime);
}
const char* G_PrintBestTime(void)
{
    return G_PrintTime3(ud.playerbest);
}

void G_BonusScreen(int32_t bonusonly)
{
    int32_t gfx_offset;
    int32_t bonuscnt;
    int32_t clockpad = 2;
    char *lastmapname;

    if (g_networkMode == NET_DEDICATED_SERVER)
        return;

    G_UpdateAppTitle();

    if (ud.volume_number == 0 && ud.last_level == 8 && boardfilename[0])
    {
        lastmapname = Bstrrchr(boardfilename,'\\');
        if (!lastmapname) lastmapname = Bstrrchr(boardfilename,'/');
        if (!lastmapname) lastmapname = boardfilename;
    }
    else
    {
        lastmapname = MapInfo[G_LastMapInfoIndex()].name;
        if (!lastmapname) // this isn't right but it's better than no name at all
            lastmapname = MapInfo[G_LastMapInfoIndex()].name;
    }


    fadepal(0,0,0, 0,252,28);
    setview(0,0,xdim-1,ydim-1);
    clearallviews(0L);
    nextpage();
    flushperms();

    FX_StopAllSounds();
    S_ClearSoundLocks();
    FX_SetReverb(0L);
    CONTROL_BindsEnabled = 1; // so you can use your screenshot bind on the score screens

    if (!bonusonly)
        G_BonusCutscenes();

    P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 8+2+1);   // JBF 20040308
    G_FadePalette(0,0,0,252);   // JBF 20031228
    KB_FlushKeyboardQueue();
    totalclock = 0;
    bonuscnt = 0;

    S_StopMusic();
    FX_StopAllSounds();
    S_ClearSoundLocks();

    if (playerswhenstarted > 1 && (GametypeFlags[ud.coop]&GAMETYPE_SCORESHEET))
    {
        clearallviews(0);
        G_DisplayMPResultsScreen();

        if (ud.config.MusicToggle)
            S_PlaySound(BONUSMUSIC);

        nextpage();
        I_ClearAllInput();
        fadepal(0,0,0, 252,0,-28);
        totalclock = 0;

        while (totalclock < TICRATE*10)
        {
            G_HandleAsync();

            MUSIC_Update();

            clearallviews(0);
            G_DisplayMPResultsScreen();
            nextpage();

            if (I_CheckAllInput())
            {
                I_ClearAllInput();
                break;
            }
        }

        fadepal(0,0,0, 0,252,28);
    }

    if (bonusonly || (g_netServer || ud.multimode > 1)) return;

    gfx_offset = (ud.volume_number==1) ? 5 : 0;
    rotatesprite_fs(160<<16,100<<16,65536L,0,BONUSSCREEN+gfx_offset,0,0,2+8+64+128+BGSTRETCH);

    if (lastmapname)
        menutext(160,20-6,0,0,lastmapname);
    menutext(160,36-6,0,0,"Completed");

    gametext(160,192,"Press any key or button to continue",quotepulseshade,2+8+16);

    if (ud.config.MusicToggle)
        S_PlaySound(BONUSMUSIC);

    nextpage();
    I_ClearAllInput();
    fadepal(0,0,0, 252,0,-4);
    bonuscnt = 0;
    totalclock = 0;

    if (g_player[myconnectindex].ps->player_par > 0 && (g_player[myconnectindex].ps->player_par < ud.playerbest || ud.playerbest < 0))
        CONFIG_SetMapBestTime(g_loadedMapHack.md4, g_player[myconnectindex].ps->player_par);

    do
    {
        int32_t yy = 0, zz;

        G_HandleAsync();

        MUSIC_Update();

        if (g_player[myconnectindex].ps->gm&MODE_EOL)
        {
            clearallviews(0);

            rotatesprite_fs(160<<16,100<<16,65536L,0,BONUSSCREEN+gfx_offset,0,0,2+8+64+128+BGSTRETCH);

            if (totalclock >= 1000000000 && totalclock < 1000000320)
            {
                switch ((totalclock>>4)%15)
                {
                case 0:
                    if (bonuscnt == 6)
                    {
                        bonuscnt++;
                        S_PlaySound(SHOTGUN_COCK);
                        switch (rand()&3)
                        {
                        case 0:
                            S_PlaySound(BONUS_SPEECH1);
                            break;
                        case 1:
                            S_PlaySound(BONUS_SPEECH2);
                            break;
                        case 2:
                            S_PlaySound(BONUS_SPEECH3);
                            break;
                        case 3:
                            S_PlaySound(BONUS_SPEECH4);
                            break;
                        }
                    }
                case 1:
                case 4:
                case 5:
                    rotatesprite_fs(199<<16,31<<16,65536L,0,BONUSSCREEN+3+gfx_offset,0,0,2+8+16+64+128+BGSTRETCH);
                    break;
                case 2:
                case 3:
                    rotatesprite_fs(199<<16,31<<16,65536L,0,BONUSSCREEN+4+gfx_offset,0,0,2+8+16+64+128+BGSTRETCH);
                    break;
                }
            }
            else if (totalclock > (10240+120L)) break;
            else
            {
                switch ((totalclock>>5)&3)
                {
                case 1:
                case 3:
                    rotatesprite_fs(199<<16,31<<16,65536L,0,BONUSSCREEN+1+gfx_offset,0,0,2+8+16+64+128+BGSTRETCH);
                    break;
                case 2:
                    rotatesprite_fs(199<<16,31<<16,65536L,0,BONUSSCREEN+2+gfx_offset,0,0,2+8+16+64+128+BGSTRETCH);
                    break;
                }
            }

            if (lastmapname)
                menutext(160,20-6,0,0,lastmapname);
            menutext(160,36-6,0,0,"Completed");

            gametext(160,192,"Press any key or button to continue",quotepulseshade,2+8+16);

            if (totalclock > (60*3))
            {
                yy = zz = 59;

                gametext(10,yy+9,"Your Time:",0,2+8+16);

                yy+=10;
                if (!(ud.volume_number == 0 && ud.last_level-1 == 7 && boardfilename[0]))
                {
                    if (MapInfo[G_LastMapInfoIndex()].partime)
                    {
                        gametext(10,yy+9,"Par Time:",0,2+8+16);
                        yy+=10;
                    }
                    if (!NAM_WW2GI && !DUKEBETA && MapInfo[G_LastMapInfoIndex()].designertime)
                    {
                        // EDuke 2.0 / NAM source suggests "Green Beret's Time:"
                        gametext(10,yy+9,"3D Realms' Time:",0,2+8+16);
                        yy+=10;
                    }

                }
                if (ud.playerbest > 0)
                {
                    gametext(10,yy+9,(g_player[myconnectindex].ps->player_par > 0 && g_player[myconnectindex].ps->player_par < ud.playerbest)?"Prev Best Time:":"Your Best Time:",0,2+8+16);
                    yy += 10;
                }

                if (bonuscnt == 0)
                    bonuscnt++;

                yy = zz;
                if (totalclock > (60*4))
                {
                    if (bonuscnt == 1)
                    {
                        bonuscnt++;
                        S_PlaySound(PIPEBOMB_EXPLODE);
                    }

                    if (g_player[myconnectindex].ps->player_par > 0)
                    {
                        G_PrintYourTime();
                        gametext((320>>2)+71,yy+9,tempbuf,0,2+8+16);
                        if (g_player[myconnectindex].ps->player_par < ud.playerbest)
                            gametext((320>>2)+89+(clockpad*24),yy+9,"New record!",0,2+8+16);
                    }
                    else
                        gametextpalbits((320>>2)+71,yy+9,"Cheated!",0,2,2+8+16,0);
                    yy+=10;

                    if (!(ud.volume_number == 0 && ud.last_level-1 == 7 && boardfilename[0]))
                    {
                        if (MapInfo[G_LastMapInfoIndex()].partime)
                        {
                            G_PrintParTime();
                            gametext((320>>2)+71,yy+9,tempbuf,0,2+8+16);
                            yy+=10;
                        }
                        if (!NAM_WW2GI && !DUKEBETA && MapInfo[G_LastMapInfoIndex()].designertime)
                        {
                            G_PrintDesignerTime();
                            gametext((320>>2)+71,yy+9,tempbuf,0,2+8+16);
                            yy+=10;
                        }
                    }

                    if (ud.playerbest > 0)
                    {
                        G_PrintBestTime();
                        gametext((320>>2)+71,yy+9,tempbuf,0,2+8+16);
                        yy+=10;
                    }
                }
            }

            zz = yy += 5;
            if (totalclock > (60*6))
            {
                gametext(10,yy+9,"Enemies Killed:",0,2+8+16);
                yy += 10;
                gametext(10,yy+9,"Enemies Left:",0,2+8+16);
                yy += 10;

                if (bonuscnt == 2)
                {
                    bonuscnt++;
                    S_PlaySound(FLY_BY);
                }

                yy = zz;

                if (totalclock > (60*7))
                {
                    if (bonuscnt == 3)
                    {
                        bonuscnt++;
                        S_PlaySound(PIPEBOMB_EXPLODE);
                    }
                    Bsprintf(tempbuf,"%-3d",g_player[myconnectindex].ps->actors_killed);
                    gametext((320>>2)+70,yy+9,tempbuf,0,2+8+16);
                    yy += 10;
                    if (ud.player_skill > 3)
                    {
                        Bsprintf(tempbuf,"N/A");
                        gametext((320>>2)+70,yy+9,tempbuf,0,2+8+16);
                        yy += 10;
                    }
                    else
                    {
                        if ((g_player[myconnectindex].ps->max_actors_killed-g_player[myconnectindex].ps->actors_killed) < 0)
                            Bsprintf(tempbuf,"%-3d",0);
                        else Bsprintf(tempbuf,"%-3d",g_player[myconnectindex].ps->max_actors_killed-g_player[myconnectindex].ps->actors_killed);
                        gametext((320>>2)+70,yy+9,tempbuf,0,2+8+16);
                        yy += 10;
                    }
                }
            }

            zz = yy += 5;
            if (totalclock > (60*9))
            {
                gametext(10,yy+9,"Secrets Found:",0,2+8+16);
                yy += 10;
                gametext(10,yy+9,"Secrets Missed:",0,2+8+16);
                yy += 10;
                if (bonuscnt == 4) bonuscnt++;

                yy = zz;
                if (totalclock > (60*10))
                {
                    if (bonuscnt == 5)
                    {
                        bonuscnt++;
                        S_PlaySound(PIPEBOMB_EXPLODE);
                    }
                    Bsprintf(tempbuf,"%-3d",g_player[myconnectindex].ps->secret_rooms);
                    gametext((320>>2)+70,yy+9,tempbuf,0,2+8+16);
                    yy += 10;
                    if (g_player[myconnectindex].ps->secret_rooms > 0)
                        Bsprintf(tempbuf,"%-3d%%",(100*g_player[myconnectindex].ps->secret_rooms/g_player[myconnectindex].ps->max_secret_rooms));
                    Bsprintf(tempbuf,"%-3d",g_player[myconnectindex].ps->max_secret_rooms-g_player[myconnectindex].ps->secret_rooms);
                    gametext((320>>2)+70,yy+9,tempbuf,0,2+8+16);
                    yy += 10;
                }
            }

            if (totalclock > 10240 && totalclock < 10240+10240)
                totalclock = 1024;

            if (I_CheckAllInput() && totalclock > (60*2)) // JBF 20030809
            {
                I_ClearAllInput();
                if (totalclock < (60*13))
                {
                    KB_FlushKeyboardQueue();
                    totalclock = (60*13);
                }
                else if (totalclock < 1000000000)
                    totalclock = 1000000000;
            }
        }
        else
            break;

        VM_OnEvent(EVENT_DISPLAYBONUSSCREEN, g_player[screenpeek].ps->i, screenpeek);
        nextpage();
    }
    while (1);
}

static void G_DrawCameraText(int16_t i)
{
    if (VM_OnEvent(EVENT_DISPLAYCAMERAOSD, i, screenpeek) != 0)
        return;

    if (!T1)
    {
        rotatesprite_win(24<<16,33<<16,65536L,0,CAMCORNER,0,0,2);
        rotatesprite_win((320-26)<<16,34<<16,65536L,0,CAMCORNER+1,0,0,2);
        rotatesprite_win(22<<16,163<<16,65536L,512,CAMCORNER+1,0,0,2+4);
        rotatesprite_win((310-10)<<16,163<<16,65536L,512,CAMCORNER+1,0,0,2);

        if (totalclock&16)
            rotatesprite_win(46<<16,32<<16,65536L,0,CAMLIGHT,0,0,2);
    }
    else
    {
        int32_t flipbits = (totalclock<<1)&48;

        for (int32_t x=-64; x<394; x+=64)
            for (int32_t y=0; y<200; y+=64)
                rotatesprite_win(x<<16,y<<16,65536L,0,STATIC,0,0,2+flipbits);
    }
}

#if 0
void vglass(int32_t x,int32_t y,short a,short wn,short n)
{
    int32_t z, zincs;
    short sect;

    sect = wall[wn].nextsector;
    if (sect == -1) return;
    zincs = (sector[sect].floorz-sector[sect].ceilingz) / n;

    for (z = sector[sect].ceilingz; z < sector[sect].floorz; z += zincs)
        A_InsertSprite(sect,x,y,z-(krand()&8191),GLASSPIECES+(z&(krand()%3)),-32,36,36,a+128-(krand()&255),16+(krand()&31),0,-1,5);
}
#endif

void A_SpawnWallGlass(int32_t i,int32_t wallnum,int32_t n)
{
    int32_t j, xv, yv, z, x1, y1;
    int16_t sect;
    int32_t a;

    sect = -1;

    if (wallnum < 0)
    {
        for (j=n-1; j >= 0 ; j--)
        {
            a = SA-256+(krand()&511)+1024;
            A_InsertSprite(SECT,SX,SY,SZ,GLASSPIECES+(j%3),-32,36,36,a,32+(krand()&63),1024-(krand()&1023),i,5);
        }
        return;
    }

    j = n+1;

    x1 = wall[wallnum].x;
    y1 = wall[wallnum].y;

    xv = wall[wall[wallnum].point2].x-x1;
    yv = wall[wall[wallnum].point2].y-y1;

    x1 -= ksgn(yv);
    y1 += ksgn(xv);

    xv = tabledivide32_noinline(xv, j);
    yv = tabledivide32_noinline(yv, j);

    for (j=n; j>0; j--)
    {
        x1 += xv;
        y1 += yv;

        updatesector(x1,y1,&sect);
        if (sect >= 0)
        {
            z = sector[sect].floorz-(krand()&(klabs(sector[sect].ceilingz-sector[sect].floorz)));
            if (z < -(32<<8) || z > (32<<8))
                z = SZ-(32<<8)+(krand()&((64<<8)-1));
            a = SA-1024;
            A_InsertSprite(SECT,x1,y1,z,GLASSPIECES+(j%3),-32,36,36,a,32+(krand()&63),-(krand()&1023),i,5);
        }
    }
}

void A_SpawnGlass(int32_t i,int32_t n)
{
    for (; n>0; n--)
    {
        int32_t k = A_InsertSprite(SECT,SX,SY,SZ-((krand()&16)<<8),GLASSPIECES+(n%3),
                                   krand()&15,36,36,krand()&2047,32+(krand()&63),-512-(krand()&2047),i,5);
        sprite[k].pal = sprite[i].pal;
    }
}

void A_SpawnCeilingGlass(int32_t i,int32_t sectnum,int32_t n)
{
    int32_t j, xv, yv, z, x1, y1, a,s;
    int32_t startwall = sector[sectnum].wallptr;
    int32_t endwall = startwall+sector[sectnum].wallnum;

    for (s=startwall; s<(endwall-1); s++)
    {
        x1 = wall[s].x;
        y1 = wall[s].y;

        xv = tabledivide32_noinline(wall[s+1].x-x1, n+1);
        yv = tabledivide32_noinline(wall[s+1].y-y1, n+1);

        for (j=n; j>0; j--)
        {
            x1 += xv;
            y1 += yv;
            a = krand()&2047;
            z = sector[sectnum].ceilingz+((krand()&15)<<8);
            A_InsertSprite(sectnum,x1,y1,z,GLASSPIECES+(j%3),-32,36,36,a,(krand()&31),0,i,5);
        }
    }
}

void A_SpawnRandomGlass(int32_t i,int32_t wallnum,int32_t n)
{
    int32_t j, xv, yv, z, x1, y1;
    int16_t sect = -1;
    int32_t a, k;

    if (wallnum < 0)
    {
        for (j=n-1; j >= 0 ; j--)
        {
            a = krand()&2047;
            k = A_InsertSprite(SECT,SX,SY,SZ-(krand()&(63<<8)),GLASSPIECES+(j%3),-32,36,36,a,32+(krand()&63),1024-(krand()&2047),i,5);
            sprite[k].pal = krand()&15;
        }
        return;
    }

    j = n+1;
    x1 = wall[wallnum].x;
    y1 = wall[wallnum].y;

    xv = tabledivide32_noinline(wall[wall[wallnum].point2].x-wall[wallnum].x, j);
    yv = tabledivide32_noinline(wall[wall[wallnum].point2].y-wall[wallnum].y, j);

    for (j=n; j>0; j--)
    {
        x1 += xv;
        y1 += yv;

        updatesector(x1,y1,&sect);
        z = sector[sect].floorz-(krand()&(klabs(sector[sect].ceilingz-sector[sect].floorz)));
        if (z < -(32<<8) || z > (32<<8))
            z = SZ-(32<<8)+(krand()&((64<<8)-1));
        a = SA-1024;
        k = A_InsertSprite(SECT,x1,y1,z,GLASSPIECES+(j%3),-32,36,36,a,32+(krand()&63),-(krand()&2047),i,5);
        sprite[k].pal = krand()&7;
    }
}

static void G_SetupGameButtons(void)
{
    CONTROL_DefineFlag(gamefunc_Move_Forward,FALSE);
    CONTROL_DefineFlag(gamefunc_Move_Backward,FALSE);
    CONTROL_DefineFlag(gamefunc_Turn_Left,FALSE);
    CONTROL_DefineFlag(gamefunc_Turn_Right,FALSE);
    CONTROL_DefineFlag(gamefunc_Strafe,FALSE);
    CONTROL_DefineFlag(gamefunc_Fire,FALSE);
    CONTROL_DefineFlag(gamefunc_Open,FALSE);
    CONTROL_DefineFlag(gamefunc_Run,FALSE);
    CONTROL_DefineFlag(gamefunc_AutoRun,FALSE);
    CONTROL_DefineFlag(gamefunc_Jump,FALSE);
    CONTROL_DefineFlag(gamefunc_Crouch,FALSE);
    CONTROL_DefineFlag(gamefunc_Look_Up,FALSE);
    CONTROL_DefineFlag(gamefunc_Look_Down,FALSE);
    CONTROL_DefineFlag(gamefunc_Look_Left,FALSE);
    CONTROL_DefineFlag(gamefunc_Look_Right,FALSE);
    CONTROL_DefineFlag(gamefunc_Strafe_Left,FALSE);
    CONTROL_DefineFlag(gamefunc_Strafe_Right,FALSE);
    CONTROL_DefineFlag(gamefunc_Aim_Up,FALSE);
    CONTROL_DefineFlag(gamefunc_Aim_Down,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_1,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_2,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_3,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_4,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_5,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_6,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_7,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_8,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_9,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_10,FALSE);
    CONTROL_DefineFlag(gamefunc_Inventory,FALSE);
    CONTROL_DefineFlag(gamefunc_Inventory_Left,FALSE);
    CONTROL_DefineFlag(gamefunc_Inventory_Right,FALSE);
    CONTROL_DefineFlag(gamefunc_Holo_Duke,FALSE);
    CONTROL_DefineFlag(gamefunc_Jetpack,FALSE);
    CONTROL_DefineFlag(gamefunc_NightVision,FALSE);
    CONTROL_DefineFlag(gamefunc_MedKit,FALSE);
    CONTROL_DefineFlag(gamefunc_TurnAround,FALSE);
    CONTROL_DefineFlag(gamefunc_SendMessage,FALSE);
    CONTROL_DefineFlag(gamefunc_Map,FALSE);
    CONTROL_DefineFlag(gamefunc_Shrink_Screen,FALSE);
    CONTROL_DefineFlag(gamefunc_Enlarge_Screen,FALSE);
    CONTROL_DefineFlag(gamefunc_Center_View,FALSE);
    CONTROL_DefineFlag(gamefunc_Holster_Weapon,FALSE);
    CONTROL_DefineFlag(gamefunc_Show_Opponents_Weapon,FALSE);
    CONTROL_DefineFlag(gamefunc_Map_Follow_Mode,FALSE);
    CONTROL_DefineFlag(gamefunc_See_Coop_View,FALSE);
    CONTROL_DefineFlag(gamefunc_Mouse_Aiming,FALSE);
    CONTROL_DefineFlag(gamefunc_Toggle_Crosshair,FALSE);
    CONTROL_DefineFlag(gamefunc_Steroids,FALSE);
    CONTROL_DefineFlag(gamefunc_Quick_Kick,FALSE);
    CONTROL_DefineFlag(gamefunc_Next_Weapon,FALSE);
    CONTROL_DefineFlag(gamefunc_Previous_Weapon,FALSE);
}
