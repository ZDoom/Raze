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
#include "screens.h"
#include "cmdline.h"
#include "palette.h"

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

const char* AppProperName = APPNAME;
const char* AppTechnicalName = APPBASENAME;

int32_t g_quitDeadline = 0;

#ifdef LUNATIC
camera_t g_camera;
#else
int32_t g_cameraDistance = 0, g_cameraClock = 0;
#endif
static int32_t g_quickExit;

char boardfilename[BMAX_PATH] = {0}, currentboardfilename[BMAX_PATH] = {0};

int32_t voting = -1;
int32_t vote_map = -1, vote_episode = -1;

int32_t g_Debug = 0;

const char *defaultrtsfilename[GAMECOUNT] = { "DUKE.RTS", "NAM.RTS", "NAPALM.RTS", "WW2GI.RTS" };

// g_rtsNamePtr can point to an argv[] element
const char *g_rtsNamePtr = NULL;

int32_t g_Shareware = 0;

// This was 32 for a while, but I think lowering it to 24 will help things like the Dingoo.
// Ideally, we would look at our memory usage on our most cramped platform and figure out
// how much of that is needed for the underlying OS and things like SDL instead of guessing
#ifndef GEKKO
int32_t MAXCACHE1DSIZE = (24*1048576);
#else
int32_t MAXCACHE1DSIZE = (8*1048576);
#endif

int32_t tempwallptr;

static int32_t nonsharedtimer;

int32_t ticrandomseed;

GAME_STATIC GAME_INLINE int32_t G_MoveLoop(void);

int32_t hud_showmapname = 1;

int32_t g_levelTextTime = 0;

int32_t r_maxfps = 60;
uint64_t g_frameDelay = 17;

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
    T_ASPECT,
    T_FORCEFILTER,
    T_FORCENOFILTER,
    T_TEXTUREFILTER,
};

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
        KB_ClearKeyDown(sc_F12);
        screencapture(
#ifndef EDUKE32_STANDALONE
        "duke0000.tga"
#else
        "capt0000.tga"
#endif
        ,
        0);
        P_DoQuote(QUOTE_SCREEN_SAVED, g_player[myconnectindex].ps);
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
        if (g_mostConcurrentPlayers > 1 && g_player[myconnectindex].ps->gm&MODE_GAME && GTFLAGS(GAMETYPE_SCORESHEET) && *msg == ' ')
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

    if ((unsigned)sp->yvel >= (unsigned)g_mostConcurrentPlayers)
        return;

    refsp = &sprite[sp->yvel];

    for (SPRITES_OF_SECT(sp->sectnum, k))
    {
        if (spritesortcnt >= maxspritesonscreen)
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

        for (bssize_t k=g_mirrorCount-1; k>=0; k--)
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

            for (bssize_t k=startwall; k<endwall; k++)
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

void G_DrawRooms(int32_t playerNum, int32_t smoothRatio)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

    int yxAspect     = yxaspect;
    int viewingRange = viewingrange;

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

    VM_OnEvent(EVENT_DISPLAYSTART, pPlayer->i, playerNum);

    if (ud.overhead_on == 2 || ud.show_help || (pPlayer->cursectnum == -1 && getrendermode() != REND_CLASSIC))
        return;

    if (r_usenewaspect)
    {
        newaspect_enable = 1;
        setaspect_new();
    }

    if (ud.pause_on || pPlayer->on_crane > -1)
        smoothRatio = 65536;
    else
        smoothRatio = calc_smoothratio(totalclock, ototalclock);

    int const playerVis = pPlayer->visibility;
    g_visibility        = (playerVis <= 0) ? 0 : (int32_t)(playerVis * (numplayers > 1 ? 1.f : r_ambientlightrecip));

    CAMERA(sect) = pPlayer->cursectnum;

    G_DoInterpolations(smoothRatio);
    G_AnimateCamSprite(smoothRatio);

    if (ud.camerasprite >= 0)
    {
        spritetype *const pSprite = &sprite[ud.camerasprite];

        // XXX: what?
        if (pSprite->yvel < 0) pSprite->yvel = -100;
        else if (pSprite->yvel > 199) pSprite->yvel = 300;

        CAMERA(ang) = actor[ud.camerasprite].tempang +
            mulscale16(((pSprite->ang+1024-actor[ud.camerasprite].tempang)&2047)-1024, smoothRatio);

        int const noDraw = VM_OnEventWithReturn(EVENT_DISPLAYROOMSCAMERA, pPlayer->i, playerNum, 0);

        if (noDraw != 1)  // event return values other than 0 and 1 are reserved
        {
            if (EDUKE32_PREDICT_FALSE(noDraw != 0))
                OSD_Printf(OSD_ERROR "ERROR: EVENT_DISPLAYROOMSCAMERA return value must be 0 or 1, "
                           "other values are reserved.\n");

#ifdef LEGACY_ROR
            G_SE40(smoothRatio);
#endif
#ifdef POLYMER
            if (getrendermode() == REND_POLYMER)
                polymer_setanimatesprites(G_DoSpriteAnimations, pSprite->x, pSprite->y, CAMERA(ang), smoothRatio);
#endif
            yax_preparedrawrooms();
            drawrooms(pSprite->x,pSprite->y,pSprite->z-ZOFFSET6,CAMERA(ang),pSprite->yvel,pSprite->sectnum);
            yax_drawrooms(G_DoSpriteAnimations, pSprite->sectnum, 0, smoothRatio);
            G_DoSpriteAnimations(pSprite->x,pSprite->y,CAMERA(ang),smoothRatio);
            drawmasks();
        }
    }
    else
    {
        int32_t floorZ, ceilZ;
        int32_t tiltcx, tiltcy, tiltcs=0;    // JBF 20030807

        int       pixelDoubling = 0;
        int const vr            = divscale22(1, sprite[pPlayer->i].yrepeat + 28);
        int       screenTilting = (getrendermode() == REND_CLASSIC && ((ud.screen_tilting && pPlayer->rotscrnang
#ifdef SPLITSCREEN_MOD_HACKS
                                                                  && !g_fakeMultiMode
#endif
                                                                  )));

        if (!r_usenewaspect)
            setaspect(vr, yxaspect);
        else
        {
            viewingRange = vr;
            yxAspect     = tabledivide32_noinline(65536 * ydim * 8, xdim * 5);

            setaspect(mulscale16(viewingRange,viewingrange), yxaspect);
        }

        if (g_screenCapture)
        {
            walock[TILE_SAVESHOT] = 199;
            if (waloff[TILE_SAVESHOT] == 0)
                allocache(&waloff[TILE_SAVESHOT],200*320,&walock[TILE_SAVESHOT]);

            if (getrendermode() == REND_CLASSIC)
                setviewtotile(TILE_SAVESHOT, 200, 320);
        }
        else if (screenTilting)
        {
            int32_t oviewingrange = viewingrange;  // save it from setaspect()
            const int16_t tang = (ud.screen_tilting) ? pPlayer->rotscrnang : 0;

            if (tang == 1024)
                screenTilting = 2;
            else
            {
                // Maximum possible allocation size passed to allocache() below
                // since there is no equivalent of free() for allocache().
#if MAXYDIM >= 640
                int const maxTiltSize = 640*640;
#else
                int const maxTiltSize = 320*320;
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

                // If the view is rotated (not 0 or 180 degrees modulo 360 degrees),
                // we render onto a square tile and display a portion of that
                // rotated on-screen later on.
                const int32_t viewtilexsiz = (tang&1023) ? tiltcx : tiltcy;
                const int32_t viewtileysiz = tiltcx;

                walock[TILE_TILT] = 255;
                if (waloff[TILE_TILT] == 0)
                    allocache(&waloff[TILE_TILT], maxTiltSize, &walock[TILE_TILT]);

                setviewtotile(TILE_TILT, viewtilexsiz, viewtileysiz);

                if ((tang&1023) == 512)
                {
                    //Block off unscreen section of 90ø tilted screen
                    int const j = tiltcx-(60*tiltcs);
                    for (bssize_t i=(60*tiltcs)-1; i>=0; i--)
                    {
                        startumost[i] = 1;
                        startumost[i+j] = 1;
                        startdmost[i] = 0;
                        startdmost[i+j] = 0;
                    }
                }

                int vRange = (tang & 511);

                if (vRange > 256)
                    vRange = 512 - vRange;

                vRange = sintable[vRange + 512] * 8 + sintable[vRange] * 5;

                //                setaspect(i>>1, yxaspect);
                setaspect(mulscale16(oviewingrange, vRange >> 1), yxaspect);

                viewingRange = vRange >> 1;
                yxAspect     = tabledivide32_noinline(65536 * ydim * 8, xdim * 5);
            }
        }
        else if (getrendermode() >= REND_POLYMOST && (ud.screen_tilting
#ifdef SPLITSCREEN_MOD_HACKS
        && !g_fakeMultiMode
#endif
        ))
        {
#ifdef USE_OPENGL
            setrollangle(pPlayer->orotscrnang + mulscale16(((pPlayer->rotscrnang - pPlayer->orotscrnang + 1024)&2047)-1024, smoothRatio));
#endif
            pPlayer->orotscrnang = pPlayer->rotscrnang;
        }
        else if (ud.detail && getrendermode()==REND_CLASSIC)
        {
            pixelDoubling = 1;
            g_halveScreenArea = 1;
            G_UpdateScreenArea();
        }

        if (pPlayer->newowner < 0)
        {
            vec3_t const camVect = { pPlayer->opos.x + mulscale16(pPlayer->pos.x - pPlayer->opos.x, smoothRatio),
                                     pPlayer->opos.y + mulscale16(pPlayer->pos.y - pPlayer->opos.y, smoothRatio),
                                     pPlayer->opos.z + mulscale16(pPlayer->pos.z - pPlayer->opos.z, smoothRatio) };

            CAMERA(pos) = camVect;
            CAMERA(ang) = pPlayer->oang + mulscale16(((pPlayer->ang + 1024 - pPlayer->oang) & 2047) - 1024, smoothRatio);
            CAMERA(ang) += pPlayer->look_ang;
            CAMERA(horiz) = pPlayer->ohoriz + pPlayer->ohorizoff
                            + mulscale16((pPlayer->horiz + pPlayer->horizoff - pPlayer->ohoriz - pPlayer->ohorizoff), smoothRatio);

            if (ud.viewbob)
            {
                int zAdd = (pPlayer->opyoff + mulscale16(pPlayer->pyoff-pPlayer->opyoff, smoothRatio));

                if (pPlayer->over_shoulder_on)
                    zAdd >>= 3;

                CAMERA(pos.z) += zAdd;
            }

            if (pPlayer->over_shoulder_on)
            {
                CAMERA(pos.z) -= 3072;

                if (G_DoThirdPerson(pPlayer, &CAMERA(pos), &CAMERA(sect), CAMERA(ang), CAMERA(horiz)) < 0)
                {
                    CAMERA(pos.z) += 3072;
                    G_DoThirdPerson(pPlayer, &CAMERA(pos), &CAMERA(sect), CAMERA(ang), CAMERA(horiz));
                }
            }
        }
        else
        {
            vec3_t const camVect = G_GetCameraPosition(pPlayer->newowner, smoothRatio);

            // looking through viewscreen
            CAMERA(pos)   = camVect;
            CAMERA(ang)   = pPlayer->ang + pPlayer->look_ang;
            CAMERA(horiz) = 100 + sprite[pPlayer->newowner].shade;
            CAMERA(sect)  = sprite[pPlayer->newowner].sectnum;
        }

        ceilZ  = actor[pPlayer->i].ceilingz;
        floorZ = actor[pPlayer->i].floorz;

        if (g_earthquakeTime > 0 && pPlayer->on_ground == 1)
        {
            CAMERA(pos.z) += 256 - (((g_earthquakeTime)&1) << 9);
            CAMERA(ang)   += (2 - ((g_earthquakeTime)&2)) << 2;
        }

        if (sprite[pPlayer->i].pal == 1)
            CAMERA(pos.z) -= (18<<8);

        if (pPlayer->newowner < 0 && pPlayer->spritebridge == 0)
        {
            // NOTE: when shrunk, p->pos.z can be below the floor.  This puts the
            // camera into the sector again then.

            if (CAMERA(pos.z) < (pPlayer->truecz + ZOFFSET6))
                CAMERA(pos.z) = ceilZ + ZOFFSET6;
            else if (CAMERA(pos.z) > (pPlayer->truefz - ZOFFSET6))
                CAMERA(pos.z) = floorZ - ZOFFSET6;
        }

        while (CAMERA(sect) >= 0)  // if, really
        {
            getzsofslope(CAMERA(sect),CAMERA(pos.x),CAMERA(pos.y),&ceilZ,&floorZ);
#ifdef YAX_ENABLE
            if (yax_getbunch(CAMERA(sect), YAX_CEILING) >= 0)
            {
                if (CAMERA(pos.z) < ceilZ)
                {
                    updatesectorz(CAMERA(pos.x), CAMERA(pos.y), CAMERA(pos.z), &CAMERA(sect));
                    break;  // since CAMERA(sect) might have been updated to -1
                    // NOTE: fist discovered in WGR2 SVN r134, til' death level 1
                    //  (Lochwood Hollow).  A problem REMAINS with Polymost, maybe classic!
                }
            }
            else
#endif
                if (CAMERA(pos.z) < ceilZ+ZOFFSET6)
                    CAMERA(pos.z) = ceilZ+ZOFFSET6;

#ifdef YAX_ENABLE
            if (yax_getbunch(CAMERA(sect), YAX_FLOOR) >= 0)
            {
                if (CAMERA(pos.z) > floorZ)
                    updatesectorz(CAMERA(pos.x), CAMERA(pos.y), CAMERA(pos.z), &CAMERA(sect));
            }
            else
#endif
                if (CAMERA(pos.z) > floorZ-ZOFFSET6)
                    CAMERA(pos.z) = floorZ-ZOFFSET6;

            break;
        }

        // NOTE: might be rendering off-screen here, so CON commands that draw stuff
        //  like showview must cope with that situation or bail out!
        int const noDraw = VM_OnEventWithReturn(EVENT_DISPLAYROOMS, pPlayer->i, playerNum, 0);

        CAMERA(horiz) = clamp(CAMERA(horiz), HORIZ_MIN, HORIZ_MAX);

        if (noDraw != 1)  // event return values other than 0 and 1 are reserved
        {
/*
            if (EDUKE32_PREDICT_FALSE(dont_draw != 0))
                OSD_Printf(OSD_ERROR "ERROR: EVENT_DISPLAYROOMS return value must be 0 or 1, "
                           "other values are reserved.\n");
*/

            G_HandleMirror(CAMERA(pos.x), CAMERA(pos.y), CAMERA(pos.z), CAMERA(ang), CAMERA(horiz), smoothRatio);
#ifdef LEGACY_ROR
            G_SE40(smoothRatio);
#endif
#ifdef POLYMER
            if (getrendermode() == REND_POLYMER)
                polymer_setanimatesprites(G_DoSpriteAnimations, CAMERA(pos.x),CAMERA(pos.y),CAMERA(ang),smoothRatio);
#endif
            // for G_PrintCoords
            dr_viewingrange = viewingrange;
            dr_yxaspect = yxaspect;
#ifdef DEBUG_MIRRORS_ONLY
            gotpic[MIRROR>>3] |= (1<<(MIRROR&7));
#else
            yax_preparedrawrooms();
            drawrooms(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),CAMERA(ang),CAMERA(horiz),CAMERA(sect));
            yax_drawrooms(G_DoSpriteAnimations, CAMERA(sect), 0, smoothRatio);
#ifdef LEGACY_ROR
            if ((unsigned)ror_sprite < MAXSPRITES && drawing_ror == 1)  // viewing from bottom
                G_OROR_DupeSprites(&sprite[ror_sprite]);
#endif
            G_DoSpriteAnimations(CAMERA(pos.x),CAMERA(pos.y),CAMERA(ang),smoothRatio);
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
        else if (screenTilting)
        {
            const int16_t tang = (ud.screen_tilting) ? pPlayer->rotscrnang : 0;

            if (screenTilting == 2)  // tang == 1024
            {
                begindrawing();
                {
                    const int32_t height = windowxy2.y-windowxy1.y+1;
                    const int32_t width = windowxy2.x-windowxy1.x+1;

                    uint8_t *f = (uint8_t *)(frameplace + ylookup[windowxy1.y]);
                    int32_t x, y;

                    for (y=0; y < (height>>1); y++)
                        swapbufreverse(f + y*bytesperline + windowxy2.x,
                                       f + (height-1-y)*bytesperline + windowxy1.x,
                                       width);

                    f += (height>>1)*bytesperline + windowxy1.x;

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

                int tiltZoom = (tang&511);

                if (tiltZoom > 256)
                    tiltZoom = 512 - tiltZoom;

                tiltZoom = sintable[tiltZoom + 512] * 8 + sintable[tiltZoom] * 5;
                tiltZoom >>= tiltcs;  // JBF 20030807

                rotatesprite_win(160 << 16, 100 << 16, tiltZoom, tang + 512, TILE_TILT, 0, 0, 4 + 2 + 64 + 1024);
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
        const int32_t visdif = ud.const_visibility-pPlayer->visibility;

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
                pPlayer->visibility = ud.const_visibility;
                break;
            }

            pPlayer->visibility += visinc;
            lastvist = totalclock;
        }
    }

    if (r_usenewaspect)
    {
        newaspect_enable = 0;
        setaspect(viewingRange, yxAspect);
    }

    VM_OnEvent(EVENT_DISPLAYROOMSEND, g_player[screenpeek].ps->i, screenpeek);
}

void G_DumpDebugInfo(void)
{
#if !defined LUNATIC
    static char const s_WEAPON[] = "WEAPON";
    int32_t i,j,x;
    //    FILE * fp=fopen("condebug.log","w");

    VM_ScriptInfo(insptr, 64);
    buildprint("\nCurrent gamevar values:\n");

    for (i=0; i<MAX_WEAPONS; i++)
    {
        for (j=0; j<numplayers; j++)
        {
            buildprint("Player ", j, "\n\n");
            buildprint(s_WEAPON, i, "_CLIP ", PWEAPON(j, i, Clip), "\n");
            buildprint(s_WEAPON, i, "_RELOAD ", PWEAPON(j, i, Reload), "\n");
            buildprint(s_WEAPON, i, "_FIREDELAY ", PWEAPON(j, i, FireDelay), "\n");
            buildprint(s_WEAPON, i, "_TOTALTIME ", PWEAPON(j, i, TotalTime), "\n");
            buildprint(s_WEAPON, i, "_HOLDDELAY ", PWEAPON(j, i, HoldDelay), "\n");
            buildprint(s_WEAPON, i, "_FLAGS ", PWEAPON(j, i, Flags), "\n");
            buildprint(s_WEAPON, i, "_SHOOTS ", PWEAPON(j, i, Shoots), "\n");
            buildprint(s_WEAPON, i, "_SPAWNTIME ", PWEAPON(j, i, SpawnTime), "\n");
            buildprint(s_WEAPON, i, "_SPAWN ", PWEAPON(j, i, Spawn), "\n");
            buildprint(s_WEAPON, i, "_SHOTSPERBURST ", PWEAPON(j, i, ShotsPerBurst), "\n");
            buildprint(s_WEAPON, i, "_WORKSLIKE ", PWEAPON(j, i, WorksLike), "\n");
            buildprint(s_WEAPON, i, "_INITIALSOUND ", PWEAPON(j, i, InitialSound), "\n");
            buildprint(s_WEAPON, i, "_FIRESOUND ", PWEAPON(j, i, FireSound), "\n");
            buildprint(s_WEAPON, i, "_SOUND2TIME ", PWEAPON(j, i, Sound2Time), "\n");
            buildprint(s_WEAPON, i, "_SOUND2SOUND ", PWEAPON(j, i, Sound2Sound), "\n");
            buildprint(s_WEAPON, i, "_RELOADSOUND1 ", PWEAPON(j, i, ReloadSound1), "\n");
            buildprint(s_WEAPON, i, "_RELOADSOUND2 ", PWEAPON(j, i, ReloadSound2), "\n");
            buildprint(s_WEAPON, i, "_SELECTSOUND ", PWEAPON(j, i, SelectSound), "\n");
            buildprint(s_WEAPON, i, "_FLASHCOLOR ", PWEAPON(j, i, FlashColor), "\n");
        }
        buildprint("\n");
    }

    for (x=0; x<MAXSTATUS; x++)
    {
        j = headspritestat[x];
        while (j >= 0)
        {
            buildprint("Sprite ", j, " (", TrackerCast(sprite[j].x), ",", TrackerCast(sprite[j].y), ",", TrackerCast(sprite[j].z),
                ") (picnum: ", TrackerCast(sprite[j].picnum), ")\n");
            for (i=0; i<g_gameVarCount; i++)
            {
                if (aGameVars[i].flags & (GAMEVAR_PERACTOR))
                {
                    if (aGameVars[i].pValues[j] != aGameVars[i].defaultValue)
                    {
                        buildprint("gamevar ", aGameVars[i].szLabel, " ", aGameVars[i].pValues[j], " GAMEVAR_PERACTOR");
                        if (aGameVars[i].flags != GAMEVAR_PERACTOR)
                        {
                            buildprint(" // ");
                            if (aGameVars[i].flags & (GAMEVAR_SYSTEM))
                            {
                                buildprint(" (system)");
                            }
                        }
                        buildprint("\n");
                    }
                }
            }
            buildprint("\n");
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
        SH(i) = *(g_tile[tilenum].execPtr);
        AC_ACTION_ID(actor[i].t_data) = *(g_tile[tilenum].execPtr+1);
        AC_MOVE_ID(actor[i].t_data) = *(g_tile[tilenum].execPtr+2);

        if (set_movflag_uncond || SHT(i) == 0)  // AC_MOVFLAGS
            SHT(i) = *(g_tile[tilenum].execPtr+3);

        return 1;
    }
#else
    if (El_HaveActor(tilenum))
    {
        // ^^^ C-CON takes precedence for now.
        const el_actor_t *a = &g_elActors[tilenum];
        uint16_t *movflagsptr = &AC_MOVFLAGS(&sprite[i], &actor[i]);

        SH(i) = a->strength;
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

    uspritetype spr_temp = { s_x, s_y,      s_z,  0,   s_pn, s_s,  0, 0,    0, s_xr, s_yr, 0,
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

    actor[i].actorstayput = actor[i].extra = -1;
#ifdef POLYMER
    actor[i].lightId = -1;
#endif
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
// 1. spriteNum>=0: Spawn from parent sprite <spriteNum> with picnum <tileNum>
// 2. spriteNum<0: Spawn from already *existing* sprite <tileNum>
int A_Spawn(int spriteNum, int tileNum)
{
    int         newSprite;
    spritetype *pSprite;
    actor_t *   pActor;
    int         sectNum;


    if (spriteNum >= 0)
    {
        // spawn from parent sprite <j>
        newSprite = A_InsertSprite(sprite[spriteNum].sectnum,sprite[spriteNum].x,sprite[spriteNum].y,sprite[spriteNum].z,
                           tileNum,0,0,0,0,0,0,spriteNum,0);
        actor[newSprite].picnum = sprite[spriteNum].picnum;
    }
    else
    {
        // spawn from already existing sprite <pn>
        newSprite                 = tileNum;
        spritetype *const pSprite = &sprite[newSprite];
        actor_t *const    pActor  = &actor[newSprite];

        Bmemset(&actor[newSprite], 0, sizeof(actor_t));
        Bmemcpy(&pActor->bpos, &sprite[newSprite], sizeof(vec3_t));

        pActor->picnum = pSprite->picnum;

        if (pSprite->picnum == SECTOREFFECTOR && pSprite->lotag == 50)
            pActor->picnum = pSprite->owner;

        pSprite->owner = pActor->owner = newSprite;

        pActor->floorz   = sector[pSprite->sectnum].floorz;
        pActor->ceilingz = sector[pSprite->sectnum].ceilingz;

        pActor->actorstayput = pActor->extra = -1;

#ifdef POLYMER
        pActor->lightId = -1;
#endif

        if ((pSprite->cstat & 48)
            && pSprite->picnum != SPEAKER
            && pSprite->picnum != LETTER
            && pSprite->picnum != DUCK
            && pSprite->picnum != TARGET
            && pSprite->picnum != TRIPBOMB
            && pSprite->picnum != VIEWSCREEN
            && pSprite->picnum != VIEWSCREEN2
            && (!(pSprite->picnum >= CRACK1 && pSprite->picnum <= CRACK4)))
        {
            if (pSprite->shade == 127)
                goto SPAWN_END;

            if (A_CheckSwitchTile(newSprite) && (pSprite->cstat & 16))
            {
                if (pSprite->pal && pSprite->picnum != ACCESSSWITCH && pSprite->picnum != ACCESSSWITCH2)
                {
                    if (((!g_netServer && ud.multimode < 2)) || ((g_netServer || ud.multimode > 1) && !GTFLAGS(GAMETYPE_DMSWITCHES)))
                    {
                        pSprite->xrepeat = pSprite->yrepeat = 0;
                        pSprite->lotag = pSprite->hitag = 0;
                        pSprite->cstat = 32768;
                        goto SPAWN_END;
                    }
                }

                pSprite->cstat |= 257;

                if (pSprite->pal && pSprite->picnum != ACCESSSWITCH && pSprite->picnum != ACCESSSWITCH2)
                    pSprite->pal = 0;

                goto SPAWN_END;
            }

            if (pSprite->hitag)
            {
                changespritestat(newSprite, STAT_FALLER);
                pSprite->cstat |= 257;
                pSprite->extra = g_impactDamage;
                goto SPAWN_END;
            }
        }

        if (pSprite->cstat & 1)
            pSprite->cstat |= 256;

        if (!G_InitActor(newSprite, pSprite->picnum, 0))
            T2(newSprite) = T5(newSprite) = 0;  // AC_MOVE_ID, AC_ACTION_ID
    }

    pSprite = &sprite[newSprite];
    pActor  = &actor[newSprite];
    sectNum = pSprite->sectnum;

    //some special cases that can't be handled through the dynamictostatic system.
    if ((pSprite->picnum >= BOLT1 && pSprite->picnum <= BOLT1 + 3) || (pSprite->picnum >= SIDEBOLT1 && pSprite->picnum <= SIDEBOLT1 + 3))
    {
        T1(newSprite) = pSprite->xrepeat;
        T2(newSprite) = pSprite->yrepeat;
        pSprite->yvel = 0;

        changespritestat(newSprite, STAT_STANDABLE);
    }
    else if ((pSprite->picnum >= CAMERA1 && pSprite->picnum <= CAMERA1 + 4) || pSprite->picnum == CAMERAPOLE || pSprite->picnum == GENERICPOLE)
    {
        if (pSprite->picnum != GENERICPOLE)
        {
            pSprite->extra = 1;
            pSprite->cstat &= 32768;

            if (g_damageCameras)
                pSprite->cstat |= 257;
        }

        if ((!g_netServer && ud.multimode < 2) && pSprite->pal != 0)
        {
            pSprite->xrepeat = pSprite->yrepeat = 0;
            changespritestat(newSprite, STAT_MISC);
        }
        else
        {
            pSprite->pal = 0;

            if (!(pSprite->picnum == CAMERAPOLE || pSprite->picnum == GENERICPOLE))
            {
                pSprite->picnum = CAMERA1;
                changespritestat(newSprite, STAT_ACTOR);
            }
        }
    }
    else
        switch (DYNAMICTILEMAP(pSprite->picnum))
        {
        default:
            if (G_HaveActor(pSprite->picnum))
            {
                if (spriteNum == -1 && pSprite->lotag > ud.player_skill)
                {
                    pSprite->xrepeat = pSprite->yrepeat = 0;
                    changespritestat(newSprite, STAT_MISC);
                    break;
                }

                //  Init the size
                if (pSprite->xrepeat == 0 || pSprite->yrepeat == 0)
                    pSprite->xrepeat = pSprite->yrepeat = 1;

                if (A_CheckSpriteFlags(newSprite, SFLAG_BADGUY))
                {
                    if (ud.monsters_off == 1)
                    {
                        pSprite->xrepeat = pSprite->yrepeat = 0;
                        changespritestat(newSprite, STAT_MISC);
                        break;
                    }

                    A_Fall(newSprite);

                    if (A_CheckSpriteFlags(newSprite, SFLAG_BADGUYSTAYPUT))
                        pActor->actorstayput = pSprite->sectnum;

                    g_player[myconnectindex].ps->max_actors_killed++;
                    pSprite->clipdist = 80;

                    if (spriteNum >= 0)
                    {
                        if (sprite[spriteNum].picnum == RESPAWN)
                            pActor->tempang = sprite[newSprite].pal = sprite[spriteNum].pal;

                        A_PlayAlertSound(newSprite);
                        changespritestat(newSprite, STAT_ACTOR);
                    }
                    else
                        changespritestat(newSprite, STAT_ZOMBIEACTOR);
                }
                else
                {
                    pSprite->clipdist = 40;
                    pSprite->owner    = newSprite;
                    changespritestat(newSprite, STAT_ACTOR);
                }

                pActor->timetosleep = 0;

                if (spriteNum >= 0)
                    pSprite->ang = sprite[spriteNum].ang;
            }
            break;
        case FOF__STATIC:
            pSprite->xrepeat = pSprite->yrepeat = 0;
            changespritestat(newSprite, STAT_MISC);
            break;
        case WATERSPLASH2__STATIC:
            if (spriteNum >= 0)
            {
                setsprite(newSprite, (vec3_t *)&sprite[spriteNum]);
                pSprite->xrepeat = pSprite->yrepeat = 8+(krand()&7);
            }
            else pSprite->xrepeat = pSprite->yrepeat = 16+(krand()&15);

            pSprite->shade = -16;
            pSprite->cstat |= 128;

            if (spriteNum >= 0)
            {
                if (sector[sprite[spriteNum].sectnum].lotag == ST_2_UNDERWATER)
                {
                    pSprite->z = getceilzofslope(sectNum, pSprite->x, pSprite->y) + (16 << 8);
                    pSprite->cstat |= 8;
                }
                else if (sector[sprite[spriteNum].sectnum].lotag == ST_1_ABOVE_WATER)
                    pSprite->z = getflorzofslope(sectNum, pSprite->x, pSprite->y);
            }

            if (sector[sectNum].floorpicnum == FLOORSLIME || sector[sectNum].ceilingpicnum == FLOORSLIME)
                pSprite->pal = 7;
            fallthrough__;
        case DOMELITE__STATIC:
            if (pSprite->picnum == DOMELITE)
                pSprite->cstat |= 257;
            fallthrough__;
        case NEON1__STATIC:
        case NEON2__STATIC:
        case NEON3__STATIC:
        case NEON4__STATIC:
        case NEON5__STATIC:
        case NEON6__STATIC:
            if (pSprite->picnum != WATERSPLASH2)
                pSprite->cstat |= 257;
            fallthrough__;
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
            changespritestat(newSprite, STAT_MISC);
            break;
        case TONGUE__STATIC:
            if (spriteNum >= 0)
                pSprite->ang = sprite[spriteNum].ang;
            pSprite->z -= 38<<8;
            pSprite->zvel = 256-(krand()&511);
            pSprite->xvel = 64-(krand()&127);
            changespritestat(newSprite, STAT_PROJECTILE);
            break;
        case NATURALLIGHTNING__STATIC:
            pSprite->cstat &= ~257;
            pSprite->cstat |= 32768;
            break;
        case TRANSPORTERSTAR__STATIC:
        case TRANSPORTERBEAM__STATIC:
            if (spriteNum == -1) break;
            if (pSprite->picnum == TRANSPORTERBEAM)
            {
                pSprite->xrepeat = 31;
                pSprite->yrepeat = 1;
                pSprite->z = sector[sprite[spriteNum].sectnum].floorz-PHEIGHT;
            }
            else
            {
                if (sprite[spriteNum].statnum == STAT_PROJECTILE)
                    pSprite->xrepeat = pSprite->yrepeat = 8;
                else
                {
                    pSprite->xrepeat = 48;
                    pSprite->yrepeat = 64;
                    if (sprite[spriteNum].statnum == STAT_PLAYER || A_CheckEnemySprite(&sprite[spriteNum]))
                        pSprite->z -= ZOFFSET5;
                }
            }

            pSprite->shade = -127;
            pSprite->cstat = 128|2;
            pSprite->ang = sprite[spriteNum].ang;

            pSprite->xvel = 128;
            changespritestat(newSprite, STAT_MISC);
            A_SetSprite(newSprite,CLIPMASK0);
            setsprite(newSprite,(vec3_t *)pSprite);
            break;

        case FRAMEEFFECT1_13__STATIC:
            if (PLUTOPAK) break;
            fallthrough__;
        case FRAMEEFFECT1__STATIC:
            if (spriteNum >= 0)
            {
                pSprite->xrepeat = sprite[spriteNum].xrepeat;
                pSprite->yrepeat = sprite[spriteNum].yrepeat;
                T2(newSprite) = sprite[spriteNum].picnum;
            }
            else pSprite->xrepeat = pSprite->yrepeat = 0;

            changespritestat(newSprite, STAT_MISC);

            break;

        case LASERLINE__STATIC:
            pSprite->yrepeat = 6;
            pSprite->xrepeat = 32;

            if (g_tripbombLaserMode == 1)
                pSprite->cstat = 16 + 2;
            else if (g_tripbombLaserMode == 0 || g_tripbombLaserMode == 2)
                pSprite->cstat = 16;
            else
            {
                pSprite->xrepeat = 0;
                pSprite->yrepeat = 0;
            }

            if (spriteNum >= 0) pSprite->ang = actor[spriteNum].t_data[5]+512;
            changespritestat(newSprite, STAT_MISC);
            break;

        case FORCESPHERE__STATIC:
            if (spriteNum == -1)
            {
                pSprite->cstat = 32768;
                changespritestat(newSprite, STAT_ZOMBIEACTOR);
            }
            else
            {
                pSprite->xrepeat = pSprite->yrepeat = 1;
                changespritestat(newSprite, STAT_MISC);
            }
            break;

        case BLOOD__STATIC:
            pSprite->xrepeat = pSprite->yrepeat = 16;
            pSprite->z -= (26<<8);
            if (spriteNum >= 0 && sprite[spriteNum].pal == 6)
                pSprite->pal = 6;
            changespritestat(newSprite, STAT_MISC);
            break;
        case BLOODPOOL__STATIC:
        case PUKE__STATIC:
        {
            int16_t pukeSect = pSprite->sectnum;

            updatesector(pSprite->x + 108, pSprite->y + 108, &pukeSect);
            if (pukeSect >= 0 && sector[pukeSect].floorz == sector[pSprite->sectnum].floorz)
            {
                updatesector(pSprite->x - 108, pSprite->y - 108, &pukeSect);
                if (pukeSect >= 0 && sector[pukeSect].floorz == sector[pSprite->sectnum].floorz)
                {
                    updatesector(pSprite->x + 108, pSprite->y - 108, &pukeSect);
                    if (pukeSect >= 0 && sector[pukeSect].floorz == sector[pSprite->sectnum].floorz)
                    {
                        updatesector(pSprite->x - 108, pSprite->y + 108, &pukeSect);
                        if (pukeSect >= 0 && sector[pukeSect].floorz != sector[pSprite->sectnum].floorz)
                            goto zero_puke;
                    }
                    else goto zero_puke;
                }
                else goto zero_puke;
            }
            else
            {
            zero_puke:
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }

            if (sector[sectNum].lotag == ST_1_ABOVE_WATER)
            {
                changespritestat(newSprite, STAT_MISC);
                break;
            }

            if (spriteNum >= 0 && pSprite->picnum != PUKE)
            {
                if (sprite[spriteNum].pal == 1)
                    pSprite->pal = 1;
                else if (sprite[spriteNum].pal != 6 && sprite[spriteNum].picnum != NUKEBARREL && sprite[spriteNum].picnum != TIRE)
                    pSprite->pal = (sprite[spriteNum].picnum == FECES) ? 7 : 2;  // Brown or red
                else
                    pSprite->pal = 0;  // green

                if (sprite[spriteNum].picnum == TIRE)
                    pSprite->shade = 127;
            }
            pSprite->cstat |= 32;
            fallthrough__;
        }
        case FECES__STATIC:
            if (spriteNum >= 0)
                pSprite->xrepeat = pSprite->yrepeat = 1;
            changespritestat(newSprite, STAT_MISC);
            break;

        case BLOODSPLAT1__STATIC:
        case BLOODSPLAT2__STATIC:
        case BLOODSPLAT3__STATIC:
        case BLOODSPLAT4__STATIC:
            pSprite->cstat |= 16;
            pSprite->xrepeat = 7 + (krand() & 7);
            pSprite->yrepeat = 7 + (krand() & 7);
            pSprite->z += (tilesiz[pSprite->picnum].y * pSprite->yrepeat) >> 2;

            if (spriteNum >= 0 && sprite[spriteNum].pal == 6)
                pSprite->pal = 6;

            A_AddToDeleteQueue(newSprite);
            changespritestat(newSprite, STAT_MISC);
            break;

        case TRIPBOMB__STATIC:
            if (pSprite->lotag > ud.player_skill)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }

            pSprite->xrepeat = 4;
            pSprite->yrepeat = 5;
            pSprite->hitag   = newSprite;
            pSprite->owner   = pSprite->hitag;
            pSprite->xvel    = 16;

            A_SetSprite(newSprite,CLIPMASK0);

            pActor->t_data[0] = 17;
            pActor->t_data[2] = 0;
            pActor->t_data[5] = pSprite->ang;

            changespritestat(newSprite, STAT_ZOMBIEACTOR);
            break;

        case SPACEMARINE__STATIC:
            pSprite->extra = 20;
            pSprite->cstat |= 257;
            changespritestat(newSprite, STAT_ZOMBIEACTOR);
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
            pSprite->clipdist = 32;
            pSprite->cstat |= 257;
            fallthrough__;
        case OCEANSPRITE4__STATIC:
            changespritestat(newSprite, STAT_DEFAULT);
            break;
        case FEMMAG1__STATIC:
        case FEMMAG2__STATIC:
            pSprite->cstat &= ~257;
            changespritestat(newSprite, STAT_DEFAULT);
            break;
        case DUKETAG__STATIC:
        case SIGN1__STATIC:
        case SIGN2__STATIC:
            if ((!g_netServer && ud.multimode < 2) && pSprite->pal)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
            }
            else pSprite->pal = 0;
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
        {
            int const j    = pSprite->cstat & SPAWN_PROTECT_CSTAT_MASK;
            pSprite->cstat = j | CSTAT_SPRITE_BLOCK;
            changespritestat(newSprite, STAT_DEFAULT);
            break;
        }
        case FOOTPRINTS__STATIC:
        case FOOTPRINTS2__STATIC:
        case FOOTPRINTS3__STATIC:
        case FOOTPRINTS4__STATIC:
            if (spriteNum >= 0)
            {
                int16_t footSect = pSprite->sectnum;

                updatesector(pSprite->x + 84, pSprite->y + 84, &footSect);
                if (footSect >= 0 && sector[footSect].floorz == sector[pSprite->sectnum].floorz)
                {
                    updatesector(pSprite->x - 84, pSprite->y - 84, &footSect);
                    if (footSect >= 0 && sector[footSect].floorz == sector[pSprite->sectnum].floorz)
                    {
                        updatesector(pSprite->x + 84, pSprite->y - 84, &footSect);
                        if (footSect >= 0 && sector[footSect].floorz == sector[pSprite->sectnum].floorz)
                        {
                            updatesector(pSprite->x - 84, pSprite->y + 84, &footSect);
                            if (footSect >= 0 && sector[footSect].floorz != sector[pSprite->sectnum].floorz)
                            {
                                pSprite->xrepeat = pSprite->yrepeat = 0;
                                changespritestat(newSprite, STAT_MISC);
                                break;
                            }
                        }
                        else goto zero_footprint;
                    }
                    else goto zero_footprint;
                }
                else
                {
                zero_footprint:
                    pSprite->xrepeat = pSprite->yrepeat = 0;
                    break;
                }

                pSprite->cstat = 32 + ((g_player[P_Get(spriteNum)].ps->footprintcount & 1) << 2);
                pSprite->ang   = sprite[spriteNum].ang;
            }

            pSprite->z = sector[sectNum].floorz;

            if (sector[sectNum].lotag != ST_1_ABOVE_WATER && sector[sectNum].lotag != ST_2_UNDERWATER)
                pSprite->xrepeat = pSprite->yrepeat = 32;

            A_AddToDeleteQueue(newSprite);
            changespritestat(newSprite, STAT_MISC);
            break;

        case PODFEM1__STATIC:
            pSprite->extra <<= 1;
            fallthrough__;
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
            pSprite->yvel  = pSprite->hitag;
            pSprite->hitag = -1;
            fallthrough__;
        case BLOODYPOLE__STATIC:
            pSprite->cstat   |= 257;
            pSprite->clipdist = 32;
            changespritestat(newSprite, STAT_ZOMBIEACTOR);
            break;

        case QUEBALL__STATIC:
        case STRIPEBALL__STATIC:
            pSprite->cstat    = 256;
            pSprite->clipdist = 8;
            changespritestat(newSprite, STAT_ZOMBIEACTOR);
            break;

        case DUKELYINGDEAD__STATIC:
            if (spriteNum >= 0 && sprite[spriteNum].picnum == APLAYER)
            {
                pSprite->xrepeat = sprite[spriteNum].xrepeat;
                pSprite->yrepeat = sprite[spriteNum].yrepeat;
                pSprite->shade   = sprite[spriteNum].shade;
                pSprite->pal     = g_player[P_Get(spriteNum)].ps->palookup;
            }
            fallthrough__;
        case DUKECAR__STATIC:
        case HELECOPT__STATIC:
            //                if(sp->picnum == HELECOPT || sp->picnum == DUKECAR) sp->xvel = 1024;
            pSprite->cstat = 0;
            pSprite->extra = 1;
            pSprite->xvel  = 292;
            pSprite->zvel  = 360;
            fallthrough__;
        case BLIMP__STATIC:
            pSprite->cstat   |= 257;
            pSprite->clipdist = 128;
            changespritestat(newSprite, STAT_ACTOR);
            break;

        case RESPAWNMARKERRED__STATIC:
            pSprite->xrepeat = pSprite->yrepeat = 24;
            if (spriteNum >= 0)
                pSprite->z = actor[spriteNum].floorz;  // -(1<<4);
            changespritestat(newSprite, STAT_ACTOR);
            break;

        case MIKE__STATIC:
            pSprite->yvel  = pSprite->hitag;
            pSprite->hitag = 0;
            changespritestat(newSprite, STAT_ACTOR);
            break;
        case WEATHERWARN__STATIC:
            changespritestat(newSprite, STAT_ACTOR);
            break;

        case SPOTLITE__STATIC:
            T1(newSprite) = pSprite->x;
            T2(newSprite) = pSprite->y;
            break;
        case BULLETHOLE__STATIC:
            pSprite->xrepeat = 3;
            pSprite->yrepeat = 3;
            pSprite->cstat   = 16 + (krand() & 12);

            A_AddToDeleteQueue(newSprite);
            changespritestat(newSprite, STAT_MISC);
            break;

        case MONEY__STATIC:
        case MAIL__STATIC:
        case PAPER__STATIC:
            pActor->t_data[0] = krand() & 2047;

            pSprite->cstat   = krand() & 12;
            pSprite->xrepeat = 8;
            pSprite->yrepeat = 8;
            pSprite->ang     = krand() & 2047;

            changespritestat(newSprite, STAT_MISC);
            break;

        case VIEWSCREEN__STATIC:
        case VIEWSCREEN2__STATIC:
            pSprite->owner = newSprite;
            pSprite->lotag = pSprite->extra = 1;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case SHELL__STATIC: //From the player
        case SHOTGUNSHELL__STATIC:
            if (spriteNum >= 0)
            {
                int shellAng;

                if (sprite[spriteNum].picnum == APLAYER)
                {
                    int const                 playerNum = P_Get(spriteNum);
                    const DukePlayer_t *const pPlayer   = g_player[playerNum].ps;

                    shellAng = pPlayer->ang - (krand() & 63) + 8;  // Fine tune

                    T1(newSprite) = krand() & 1;

                    pSprite->z = (3 << 8) + pPlayer->pyoff + pPlayer->pos.z - ((pPlayer->horizoff + pPlayer->horiz - 100) << 4);

                    if (pSprite->picnum == SHOTGUNSHELL)
                        pSprite->z += (3 << 8);

                    pSprite->zvel = -(krand() & 255);
                }
                else
                {
                    shellAng          = pSprite->ang;
                    pSprite->z = sprite[spriteNum].z - PHEIGHT + (3 << 8);
                }

                pSprite->x     = sprite[spriteNum].x + (sintable[(shellAng + 512) & 2047] >> 7);
                pSprite->y     = sprite[spriteNum].y + (sintable[shellAng & 2047] >> 7);
                pSprite->shade = -8;

                if (pSprite->yvel == 1 || NAM_WW2GI)
                {
                    pSprite->ang  = shellAng + 512;
                    pSprite->xvel = 30;
                }
                else
                {
                    pSprite->ang  = shellAng - 512;
                    pSprite->xvel = 20;
                }

                pSprite->xrepeat = pSprite->yrepeat = 4;

                changespritestat(newSprite, STAT_MISC);
            }
            break;

        case RESPAWN__STATIC:
            pSprite->extra = 66-13;
            fallthrough__;
        case MUSICANDSFX__STATIC:
            if ((!g_netServer && ud.multimode < 2) && pSprite->pal == 1)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }
            pSprite->cstat = 32768;
            changespritestat(newSprite, STAT_FX);
            break;

        case EXPLOSION2__STATIC:
#ifdef POLYMER
            if (pSprite->yrepeat > 32)
            {
                G_AddGameLight(0, newSprite, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), 32768, 255+(95<<8),PR_LIGHT_PRIO_MAX_GAME);
                pActor->lightcount = 2;
            }
            fallthrough__;
#endif
        case EXPLOSION2BOT__STATIC:
        case BURNING__STATIC:
        case BURNING2__STATIC:
        case SMALLSMOKE__STATIC:
        case SHRINKEREXPLOSION__STATIC:
        case COOLEXPLOSION1__STATIC:

            if (spriteNum >= 0)
            {
                pSprite->ang = sprite[spriteNum].ang;
                pSprite->shade = -64;
                pSprite->cstat = 128|(krand()&4);
            }

            if (pSprite->picnum == EXPLOSION2 || pSprite->picnum == EXPLOSION2BOT)
            {
                pSprite->xrepeat = pSprite->yrepeat = 48;
                pSprite->shade = -127;
                pSprite->cstat |= 128;
            }
            else if (pSprite->picnum == SHRINKEREXPLOSION)
                pSprite->xrepeat = pSprite->yrepeat = 32;
            else if (pSprite->picnum == SMALLSMOKE)
            {
                // 64 "money"
                pSprite->xrepeat = pSprite->yrepeat = 24;
            }
            else if (pSprite->picnum == BURNING || pSprite->picnum == BURNING2)
                pSprite->xrepeat = pSprite->yrepeat = 4;

            pSprite->cstat |= 8192;

            if (spriteNum >= 0)
            {
                int const floorZ = getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y);

                if (pSprite->z > floorZ-ZOFFSET4)
                    pSprite->z = floorZ-ZOFFSET4;
            }

            changespritestat(newSprite, STAT_MISC);

            break;

        case PLAYERONWATER__STATIC:
            if (spriteNum >= 0)
            {
                pSprite->xrepeat = sprite[spriteNum].xrepeat;
                pSprite->yrepeat = sprite[spriteNum].yrepeat;
                pSprite->zvel = 128;
                if (sector[pSprite->sectnum].lotag != ST_2_UNDERWATER)
                    pSprite->cstat |= 32768;
            }
            changespritestat(newSprite, STAT_DUMMYPLAYER);
            break;

        case APLAYER__STATIC:
            pSprite->xrepeat = 0;
            pSprite->yrepeat = 0;
            pSprite->cstat   = 32768;

            changespritestat(newSprite, ((!g_netServer && ud.multimode < 2)
                                         || ((g_gametypeFlags[ud.coop] & GAMETYPE_COOPSPAWN) / GAMETYPE_COOPSPAWN) != pSprite->lotag)
                                        ? STAT_MISC
                                        : STAT_PLAYER);
            break;

        case WATERBUBBLE__STATIC:
            if (spriteNum >= 0)
            {
                if (sprite[spriteNum].picnum == APLAYER)
                    pSprite->z -= (16 << 8);

                pSprite->ang = sprite[spriteNum].ang;
            }

            pSprite->xrepeat = pSprite->yrepeat = 4;
            changespritestat(newSprite, STAT_MISC);
            break;

        case CRANE__STATIC:

            pSprite->cstat |= 64|257;

            pSprite->picnum += 2;
            pSprite->z = sector[sectNum].ceilingz+(48<<8);
            T5(newSprite) = tempwallptr;

            g_origins[tempwallptr] = *(vec2_t *) pSprite;
            g_origins[tempwallptr+2].x = pSprite->z;


            if (headspritestat[STAT_DEFAULT] != -1)
            {
                int findSprite = headspritestat[STAT_DEFAULT];

                do
                {
                    if (sprite[findSprite].picnum == CRANEPOLE && pSprite->hitag == (sprite[findSprite].hitag))
                    {
                        g_origins[tempwallptr + 2].y = findSprite;

                        T2(newSprite) = sprite[findSprite].sectnum;

                        sprite[findSprite].xrepeat = 48;
                        sprite[findSprite].yrepeat = 128;

                        g_origins[tempwallptr + 1]     = *(vec2_t *)&sprite[findSprite];
                        *(vec3_t *)&sprite[findSprite] = *(vec3_t *)pSprite;
                        sprite[findSprite].shade       = pSprite->shade;

                        setsprite(findSprite, (vec3_t *)&sprite[findSprite]);
                        break;
                    }
                    findSprite = nextspritestat[findSprite];
                } while (findSprite >= 0);
            }

            tempwallptr += 3;
            pSprite->owner = -1;
            pSprite->extra = 8;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case TRASH__STATIC:
            pSprite->ang = krand()&2047;
            pSprite->xrepeat = pSprite->yrepeat = 24;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case WATERDRIP__STATIC:
            if (spriteNum >= 0 && (sprite[spriteNum].statnum == STAT_PLAYER || sprite[spriteNum].statnum == STAT_ACTOR))
            {
                if (sprite[spriteNum].pal != 1)
                {
                    pSprite->pal = 2;
                    pSprite->z -= (18<<8);
                }
                else pSprite->z -= (13<<8);

                pSprite->shade = 32;
                pSprite->ang   = getangle(g_player[0].ps->pos.x - pSprite->x, g_player[0].ps->pos.y - pSprite->y);
                pSprite->xvel  = 48 - (krand() & 31);

                A_SetSprite(newSprite,CLIPMASK0);
            }
            else if (spriteNum == -1)
            {
                pSprite->z += ZOFFSET6;
                T1(newSprite) = pSprite->z;
                T2(newSprite) = krand()&127;
            }
            fallthrough__;
        case WATERDRIPSPLASH__STATIC:
            pSprite->xrepeat = pSprite->yrepeat = 24;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case PLUG__STATIC:
            pSprite->lotag = 9999;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case TOUCHPLATE__STATIC:
            T3(newSprite) = sector[sectNum].floorz;

            if (sector[sectNum].lotag != ST_1_ABOVE_WATER && sector[sectNum].lotag != ST_2_UNDERWATER)
                sector[sectNum].floorz = pSprite->z;

            if (pSprite->pal && (g_netServer || ud.multimode > 1))
            {
                pSprite->xrepeat=pSprite->yrepeat=0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }
            fallthrough__;
        case WATERBUBBLEMAKER__STATIC:
            if (EDUKE32_PREDICT_FALSE(pSprite->hitag && pSprite->picnum == WATERBUBBLEMAKER))
            {
                // JBF 20030913: Pisses off X_Move(), eg. in bobsp2
                OSD_Printf(OSD_ERROR "WARNING: WATERBUBBLEMAKER %d @ %d,%d with hitag!=0. Applying fixup.\n",
                           newSprite,TrackerCast(pSprite->x),TrackerCast(pSprite->y));
                pSprite->hitag = 0;
            }
            pSprite->cstat |= 32768;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case MASTERSWITCH__STATIC:
            if (pSprite->picnum == MASTERSWITCH)
                pSprite->cstat |= 32768;
            pSprite->yvel = 0;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case TARGET__STATIC:
        case DUCK__STATIC:
        case LETTER__STATIC:
            pSprite->extra = 1;
            pSprite->cstat |= 257;
            changespritestat(newSprite, STAT_ACTOR);
            break;

        case OCTABRAINSTAYPUT__STATIC:
        case LIZTROOPSTAYPUT__STATIC:
        case PIGCOPSTAYPUT__STATIC:
        case LIZMANSTAYPUT__STATIC:
        case BOSS1STAYPUT__STATIC:
        case PIGCOPDIVE__STATIC:
        case COMMANDERSTAYPUT__STATIC:
        case BOSS4STAYPUT__STATIC:
            pActor->actorstayput = pSprite->sectnum;
            fallthrough__;
        case BOSS1__STATIC:
        case BOSS2__STATIC:
        case BOSS3__STATIC:
        case BOSS4__STATIC:
        case ROTATEGUN__STATIC:
        case GREENSLIME__STATIC:
            if (pSprite->picnum == GREENSLIME)
                pSprite->extra = 1;
            fallthrough__;
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

            if (pSprite->pal == 0)
            {
                switch (DYNAMICTILEMAP(pSprite->picnum))
                {
                    case LIZTROOPONTOILET__STATIC:
                    case LIZTROOPSHOOT__STATIC:
                    case LIZTROOPJETPACK__STATIC:
                    case LIZTROOPDUCKING__STATIC:
                    case LIZTROOPRUNNING__STATIC:
                    case LIZTROOPSTAYPUT__STATIC:
                    case LIZTROOPJUSTSIT__STATIC:
                    case LIZTROOP__STATIC: pSprite->pal = 22; break;
                }
            }
            else
            {
                if (!PLUTOPAK)
                    pSprite->extra <<= 1;
            }

            if (pSprite->picnum == BOSS4STAYPUT || pSprite->picnum == BOSS1 || pSprite->picnum == BOSS2 ||
                pSprite->picnum == BOSS1STAYPUT || pSprite->picnum == BOSS3 || pSprite->picnum == BOSS4)
            {
                if (spriteNum >= 0 && sprite[spriteNum].picnum == RESPAWN)
                    pSprite->pal = sprite[spriteNum].pal;

                if (pSprite->pal)
                {
                    pSprite->clipdist = 80;
                    pSprite->xrepeat  = pSprite->yrepeat = 40;
                }
                else
                {
                    pSprite->xrepeat  = pSprite->yrepeat = 80;
                    pSprite->clipdist = 164;
                }
            }
            else
            {
                if (pSprite->picnum != SHARK)
                {
                    pSprite->xrepeat  = pSprite->yrepeat = 40;
                    pSprite->clipdist = 80;
                }
                else
                {
                    pSprite->xrepeat  = pSprite->yrepeat = 60;
                    pSprite->clipdist = 40;
                }
            }

            // If spawned from parent sprite (as opposed to 'from premap'),
            // ignore skill.
            if (spriteNum >= 0)
                pSprite->lotag = 0;

            if ((pSprite->lotag > ud.player_skill) || ud.monsters_off == 1)
            {
                pSprite->xrepeat=pSprite->yrepeat=0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }
            else
            {
                A_Fall(newSprite);

                if (pSprite->picnum == RAT)
                {
                    pSprite->ang = krand()&2047;
                    pSprite->xrepeat = pSprite->yrepeat = 48;
                    pSprite->cstat = 0;
                }
                else
                {
                    pSprite->cstat |= 257;

                    if (pSprite->picnum != SHARK)
                        g_player[myconnectindex].ps->max_actors_killed++;
                }

                if (pSprite->picnum == ORGANTIC) pSprite->cstat |= 128;

                if (spriteNum >= 0)
                {
                    pActor->timetosleep = 0;
                    A_PlayAlertSound(newSprite);
                    changespritestat(newSprite, STAT_ACTOR);
                }
                else changespritestat(newSprite, STAT_ZOMBIEACTOR);
            }

            if (pSprite->picnum == ROTATEGUN)
                pSprite->zvel = 0;

            break;

        case LOCATORS__STATIC:
            pSprite->cstat |= 32768;
            changespritestat(newSprite, STAT_LOCATOR);
            break;

        case ACTIVATORLOCKED__STATIC:
        case ACTIVATOR__STATIC:
            pSprite->cstat = 32768;
            if (pSprite->picnum == ACTIVATORLOCKED)
                sector[pSprite->sectnum].lotag |= 16384;
            changespritestat(newSprite, STAT_ACTIVATOR);
            break;

        case DOORSHOCK__STATIC:
            pSprite->cstat |= 1+256;
            pSprite->shade = -12;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case OOZ__STATIC:
        case OOZ2__STATIC:
        {
            pSprite->shade = -12;

            if (spriteNum >= 0)
            {
                if (sprite[spriteNum].picnum == NUKEBARREL)
                    pSprite->pal = 8;
                A_AddToDeleteQueue(newSprite);
            }

            changespritestat(newSprite, STAT_ACTOR);

            A_GetZLimits(newSprite);

            int const oozSize = (pActor->floorz-pActor->ceilingz)>>9;

            pSprite->yrepeat = oozSize;
            pSprite->xrepeat = 25 - (oozSize >> 1);
            pSprite->cstat |= (krand() & 4);

            break;
        }

        case REACTOR2__STATIC:
        case REACTOR__STATIC:
            pSprite->extra = g_impactDamage;
            pSprite->cstat |= 257;
            if ((!g_netServer && ud.multimode < 2) && pSprite->pal != 0)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }

            pSprite->pal   = 0;
            pSprite->shade = -17;

            changespritestat(newSprite, STAT_ZOMBIEACTOR);
            break;

        case HEAVYHBOMB__STATIC:
            if (spriteNum >= 0)
                pSprite->owner = spriteNum;
            else pSprite->owner = newSprite;

            pSprite->xrepeat = pSprite->yrepeat = 9;
            pSprite->yvel = 4;
            pSprite->cstat |= 257;

            if ((!g_netServer && ud.multimode < 2) && pSprite->pal != 0)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }
            pSprite->pal   = 0;
            pSprite->shade = -17;

            changespritestat(newSprite, STAT_ZOMBIEACTOR);
            break;

        case RECON__STATIC:
            if (pSprite->lotag > ud.player_skill)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                goto SPAWN_END;
            }
            g_player[myconnectindex].ps->max_actors_killed++;
            pActor->t_data[5] = 0;
            if (ud.monsters_off == 1)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }
            pSprite->extra = 130;
            pSprite->cstat |= 256; // Make it hitable

            if ((!g_netServer && ud.multimode < 2) && pSprite->pal != 0)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }
            pSprite->pal   = 0;
            pSprite->shade = -17;

            changespritestat(newSprite, STAT_ZOMBIEACTOR);
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

            if (spriteNum >= 0)
            {
                pSprite->lotag = 0;
                pSprite->z -= ZOFFSET5;
                pSprite->zvel = -1024;
                A_SetSprite(newSprite,CLIPMASK0);
                pSprite->cstat = krand()&4;
            }
            else
            {
                pSprite->owner = newSprite;
                pSprite->cstat = 0;
            }

            if (((!g_netServer && ud.multimode < 2) && pSprite->pal != 0) || (pSprite->lotag > ud.player_skill))
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }

            pSprite->pal = 0;

            if (pSprite->picnum == ATOMICHEALTH)
                pSprite->cstat |= 128;

            fallthrough__;
        case ACCESSCARD__STATIC:
            if ((g_netServer || ud.multimode > 1) && !GTFLAGS(GAMETYPE_ACCESSCARDSPRITES) && pSprite->picnum == ACCESSCARD)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }
            else
            {
                if (pSprite->picnum == AMMO)
                    pSprite->xrepeat = pSprite->yrepeat = 16;
                else pSprite->xrepeat = pSprite->yrepeat = 32;
            }

            pSprite->shade = -17;

            changespritestat(newSprite, (spriteNum >= 0) ? STAT_ACTOR : STAT_ZOMBIEACTOR);
            A_Fall(newSprite);
            break;

        case WATERFOUNTAIN__STATIC:
            SLT(newSprite) = 1;
            fallthrough__;
        case TREE1__STATIC:
        case TREE2__STATIC:
        case TIRE__STATIC:
        case CONE__STATIC:
        case BOX__STATIC:
            pSprite->cstat = 257; // Make it hitable
            sprite[newSprite].extra = 1;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case FLOORFLAME__STATIC:
            pSprite->shade = -127;
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case BOUNCEMINE__STATIC:
            pSprite->owner = newSprite;
            pSprite->cstat |= 1+256; //Make it hitable
            pSprite->xrepeat = pSprite->yrepeat = 24;
            pSprite->shade = -127;
            pSprite->extra = g_impactDamage<<2;
            changespritestat(newSprite, STAT_ZOMBIEACTOR);
            break;

        case STEAM__STATIC:
            if (spriteNum >= 0)
            {
                pSprite->ang = sprite[spriteNum].ang;
                pSprite->cstat = 16+128+2;
                pSprite->xrepeat=pSprite->yrepeat=1;
                pSprite->xvel = -8;
                A_SetSprite(newSprite,CLIPMASK0);
            }
            fallthrough__;
        case CEILINGSTEAM__STATIC:
            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case SECTOREFFECTOR__STATIC:
            pSprite->cstat |= 32768;
            pSprite->xrepeat = pSprite->yrepeat = 0;

            switch (pSprite->lotag)
            {
#ifdef LEGACY_ROR
            case 40:
            case 41:
                pSprite->cstat = 32;
                pSprite->xrepeat = pSprite->yrepeat = 64;
                changespritestat(newSprite, STAT_EFFECTOR);
                for (spriteNum=0; spriteNum < MAXSPRITES; spriteNum++)
                    if (sprite[spriteNum].picnum == SECTOREFFECTOR && (sprite[spriteNum].lotag == 40 || sprite[spriteNum].lotag == 41) &&
                            sprite[spriteNum].hitag == pSprite->hitag && newSprite != spriteNum)
                    {
//                        initprintf("found ror match\n");
                        pSprite->yvel = spriteNum;
                        break;
                    }
                goto SPAWN_END;
                break;
            case 46:
                ror_protectedsectors[pSprite->sectnum] = 1;
                /* XXX: fall-through intended? */
                fallthrough__;
#endif
            case SE_49_POINT_LIGHT:
            case SE_50_SPOT_LIGHT:
            {
                int32_t j, nextj;

                for (TRAVERSE_SPRITE_SECT(headspritesect[pSprite->sectnum], j, nextj))
                    if (sprite[j].picnum == ACTIVATOR || sprite[j].picnum == ACTIVATORLOCKED)
                        pActor->flags |= SFLAG_USEACTIVATOR;
            }
            changespritestat(newSprite, pSprite->lotag==46 ? STAT_EFFECTOR : STAT_LIGHT);
            goto SPAWN_END;
            break;
            }

            pSprite->yvel = sector[sectNum].extra;

            switch (pSprite->lotag)
            {
            case SE_28_LIGHTNING:
                T6(newSprite) = 65;// Delay for lightning
                break;
            case SE_7_TELEPORT: // Transporters!!!!
            case SE_23_ONE_WAY_TELEPORT:// XPTR END
                if (pSprite->lotag != SE_23_ONE_WAY_TELEPORT)
                {
                    for (spriteNum=0; spriteNum<MAXSPRITES; spriteNum++)
                        if (sprite[spriteNum].statnum < MAXSTATUS && sprite[spriteNum].picnum == SECTOREFFECTOR &&
                                (sprite[spriteNum].lotag == SE_7_TELEPORT || sprite[spriteNum].lotag == SE_23_ONE_WAY_TELEPORT) && newSprite != spriteNum && sprite[spriteNum].hitag == SHT(newSprite))
                        {
                            OW(newSprite) = spriteNum;
                            break;
                        }
                }
                else OW(newSprite) = newSprite;

                T5(newSprite) = (sector[sectNum].floorz == SZ(newSprite));  // ONFLOORZ
                pSprite->cstat = 0;
                changespritestat(newSprite, STAT_TRANSPORT);
                goto SPAWN_END;
            case SE_1_PIVOT:
                pSprite->owner = -1;
                T1(newSprite) = 1;
                break;
            case SE_18_INCREMENTAL_SECTOR_RISE_FALL:

                if (pSprite->ang == 512)
                {
                    T2(newSprite) = sector[sectNum].ceilingz;
                    if (pSprite->pal)
                        sector[sectNum].ceilingz = pSprite->z;
                }
                else
                {
                    T2(newSprite) = sector[sectNum].floorz;
                    if (pSprite->pal)
                        sector[sectNum].floorz = pSprite->z;
                }

                pSprite->hitag <<= 2;
                break;

            case SE_19_EXPLOSION_LOWERS_CEILING:
                pSprite->owner = -1;
                break;
            case SE_25_PISTON: // Pistons
                T4(newSprite) = sector[sectNum].ceilingz;
                T5(newSprite) = 1;
                sector[sectNum].ceilingz = pSprite->z;
                G_SetInterpolation(&sector[sectNum].ceilingz);
                break;
            case SE_35:
                sector[sectNum].ceilingz = pSprite->z;
                break;
            case SE_27_DEMO_CAM:
                if (ud.recstat == 1)
                {
                    pSprite->xrepeat=pSprite->yrepeat=64;
                    pSprite->cstat &= 32768;
                }
                break;
            case SE_12_LIGHT_SWITCH:

                T2(newSprite) = sector[sectNum].floorshade;
                T3(newSprite) = sector[sectNum].ceilingshade;
                break;

            case SE_13_EXPLOSIVE:

                T1(newSprite) = sector[sectNum].ceilingz;
                T2(newSprite) = sector[sectNum].floorz;

                if (klabs(T1(newSprite)-pSprite->z) < klabs(T2(newSprite)-pSprite->z))
                    pSprite->owner = 1;
                else pSprite->owner = 0;

                if (pSprite->ang == 512)
                {
                    if (pSprite->owner)
                        sector[sectNum].ceilingz = pSprite->z;
                    else
                        sector[sectNum].floorz = pSprite->z;
#ifdef YAX_ENABLE
                    {
                        int16_t cf=!pSprite->owner, bn=yax_getbunch(sectNum, cf);
                        int32_t jj, daz=SECTORFLD(sectNum,z, cf);

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
                    sector[sectNum].ceilingz = sector[sectNum].floorz = pSprite->z;

                if (sector[sectNum].ceilingstat&1)
                {
                    sector[sectNum].ceilingstat ^= 1;
                    T4(newSprite) = 1;

                    if (!pSprite->owner && pSprite->ang==512)
                    {
                        sector[sectNum].ceilingstat ^= 1;
                        T4(newSprite) = 0;
                    }

                    sector[sectNum].ceilingshade =
                        sector[sectNum].floorshade;

                    if (pSprite->ang==512)
                    {
                        int const startwall = sector[sectNum].wallptr;
                        int const endwall   = startwall + sector[sectNum].wallnum;
                        for (bssize_t j = startwall; j < endwall; j++)
                        {
                            int const nextSect = wall[j].nextsector;

                            if (nextSect >= 0)
                            {
                                if (!(sector[nextSect].ceilingstat & 1))
                                {
                                    sector[sectNum].ceilingpicnum = sector[nextSect].ceilingpicnum;
                                    sector[sectNum].ceilingshade  = sector[nextSect].ceilingshade;
                                    break;  // Leave earily
                                }
                            }
                        }
                    }
                }

                break;

            case SE_17_WARP_ELEVATOR:
            {
                T3(newSprite) = sector[sectNum].floorz;  // Stopping loc

                int nextSectNum = nextsectorneighborz(sectNum, sector[sectNum].floorz, -1, -1);

                if (EDUKE32_PREDICT_TRUE(nextSectNum >= 0))
                    T4(newSprite) = sector[nextSectNum].ceilingz;
                else
                {
                    // use elevator sector's ceiling as heuristic
                    T4(newSprite) = sector[sectNum].ceilingz;

                    OSD_Printf(OSD_ERROR "WARNING: SE17 sprite %d using own sector's ceilingz to "
                                         "determine when to warp. Sector %d adjacent to a door?\n",
                               newSprite, sectNum);
                }

                nextSectNum = nextsectorneighborz(sectNum, sector[sectNum].ceilingz, 1, 1);

                if (EDUKE32_PREDICT_TRUE(nextSectNum >= 0))
                    T5(newSprite) = sector[nextSectNum].floorz;
                else
                {
                    // XXX: we should return to the menu for this and similar failures
                    Bsprintf(tempbuf, "SE 17 (warp elevator) setup failed: sprite %d at (%d, %d)", newSprite,
                             TrackerCast(sprite[newSprite].x), TrackerCast(sprite[newSprite].y));
                    G_GameExit(tempbuf);
                }

                if (numplayers < 2 && !g_netServer)
                {
                    G_SetInterpolation(&sector[sectNum].floorz);
                    G_SetInterpolation(&sector[sectNum].ceilingz);
                }
            }
            break;

            case SE_24_CONVEYOR:
                pSprite->yvel <<= 1;
            case SE_36_PROJ_SHOOTER:
                break;

            case SE_20_STRETCH_BRIDGE:
            {
                int       closestDist = INT32_MAX;
                int       closestWall = 0;
                int const startWall   = sector[sectNum].wallptr;
                int const endWall     = startWall + sector[sectNum].wallnum;

                for (bssize_t findWall=startWall; findWall<endWall; findWall++)
                {
                    int const x = wall[findWall].x;
                    int const y = wall[findWall].y;
                    int const d = FindDistance2D(pSprite->x - x, pSprite->y - y);

                    if (d < closestDist)
                    {
                        closestDist = d;
                        closestWall = findWall;
                    }
                }

                T2(newSprite) = closestWall;

                closestDist = INT32_MAX;

                for (bssize_t findWall=startWall; findWall<endWall; findWall++)
                {
                    int const x = wall[findWall].x;
                    int const y = wall[findWall].y;
                    int const d = FindDistance2D(pSprite->x - x, pSprite->y - y);

                    if (d < closestDist && findWall != T2(newSprite))
                    {
                        closestDist = d;
                        closestWall = findWall;
                    }
                }

                T3(newSprite) = closestWall;
            }

            break;

            case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:
            {

                T4(newSprite)=sector[sectNum].floorshade;

                sector[sectNum].floorshade   = pSprite->shade;
                sector[sectNum].ceilingshade = pSprite->shade;

                pSprite->owner = sector[sectNum].ceilingpal << 8;
                pSprite->owner |= sector[sectNum].floorpal;

                //fix all the walls;

                int const startWall = sector[sectNum].wallptr;
                int const endWall = startWall+sector[sectNum].wallnum;

                for (bssize_t w=startWall; w<endWall; ++w)
                {
                    if (!(wall[w].hitag & 1))
                        wall[w].shade = pSprite->shade;

                    if ((wall[w].cstat & 2) && wall[w].nextwall >= 0)
                        wall[wall[w].nextwall].shade = pSprite->shade;
                }
                break;
            }

            case SE_31_FLOOR_RISE_FALL:
            {
                T2(newSprite) = sector[sectNum].floorz;

                if (pSprite->ang != 1536)
                {
                    sector[sectNum].floorz = pSprite->z;
                    Yax_SetBunchZs(sectNum, YAX_FLOOR, pSprite->z);
                }

                int const startWall = sector[sectNum].wallptr;
                int const endWall   = startWall + sector[sectNum].wallnum;

                for (bssize_t w = startWall; w < endWall; ++w)
                    if (wall[w].hitag == 0)
                        wall[w].hitag = 9999;

                G_SetInterpolation(&sector[sectNum].floorz);
                Yax_SetBunchInterpolation(sectNum, YAX_FLOOR);
            }
            break;

            case SE_32_CEILING_RISE_FALL:
            {
                T2(newSprite) = sector[sectNum].ceilingz;
                T3(newSprite) = pSprite->hitag;

                if (pSprite->ang != 1536)
                {
                    sector[sectNum].ceilingz = pSprite->z;
                    Yax_SetBunchZs(sectNum, YAX_CEILING, pSprite->z);
                }

                int const startWall = sector[sectNum].wallptr;
                int const endWall   = startWall + sector[sectNum].wallnum;

                for (bssize_t w = startWall; w < endWall; ++w)
                    if (wall[w].hitag == 0)
                        wall[w].hitag = 9999;

                G_SetInterpolation(&sector[sectNum].ceilingz);
                Yax_SetBunchInterpolation(sectNum, YAX_CEILING);
            }
            break;

            case SE_4_RANDOM_LIGHTS: //Flashing lights
            {
                T3(newSprite) = sector[sectNum].floorshade;

                int const startWall = sector[sectNum].wallptr;
                int const endWall   = startWall + sector[sectNum].wallnum;

                pSprite->owner = sector[sectNum].ceilingpal << 8;
                pSprite->owner |= sector[sectNum].floorpal;

                for (bssize_t w = startWall; w < endWall; ++w)
                    if (wall[w].shade > T4(newSprite))
                        T4(newSprite) = wall[w].shade;
            }
            break;

            case SE_9_DOWN_OPEN_DOOR_LIGHTS:
                if (sector[sectNum].lotag &&
                        labs(sector[sectNum].ceilingz-pSprite->z) > 1024)
                    sector[sectNum].lotag |= 32768; //If its open
                fallthrough__;
            case SE_8_UP_OPEN_DOOR_LIGHTS:
                //First, get the ceiling-floor shade
                {
                    T1(newSprite) = sector[sectNum].floorshade;
                    T2(newSprite) = sector[sectNum].ceilingshade;

                    int const startWall = sector[sectNum].wallptr;
                    int const endWall   = startWall + sector[sectNum].wallnum;

                    for (bssize_t w = startWall; w < endWall; ++w)
                        if (wall[w].shade > T3(newSprite))
                            T3(newSprite) = wall[w].shade;

                    T4(newSprite) = 1;  // Take Out;
                }
                break;

            case SE_11_SWINGING_DOOR:  // Pivitor rotater
                T4(newSprite) = (pSprite->ang > 1024) ? 2 : -2;
            /* fall-through */
            case SE_0_ROTATING_SECTOR:
            case SE_2_EARTHQUAKE:      // Earthquakemakers
            case SE_5:                 // Boss Creature
            case SE_6_SUBWAY:          // Subway
            case SE_14_SUBWAY_CAR:     // Caboos
            case SE_15_SLIDING_DOOR:   // Subwaytype sliding door
            case SE_16_REACTOR:        // That rotating blocker reactor thing
            case SE_26:                // ESCELATOR
            case SE_30_TWO_WAY_TRAIN:  // No rotational subways
                if (pSprite->lotag == SE_0_ROTATING_SECTOR)
                {
                    if (sector[sectNum].lotag == ST_30_ROTATE_RISE_BRIDGE)
                    {
                        sprite[newSprite].clipdist = (pSprite->pal) ? 1 : 0;
                        T4(newSprite) = sector[sectNum].floorz;
                        sector[sectNum].hitag = newSprite;
                    }

                    for (spriteNum = MAXSPRITES-1; spriteNum>=0; spriteNum--)
                    {
                        if (sprite[spriteNum].statnum < MAXSTATUS)
                            if (sprite[spriteNum].picnum == SECTOREFFECTOR &&
                                    sprite[spriteNum].lotag == SE_1_PIVOT &&
                                    sprite[spriteNum].hitag == pSprite->hitag)
                            {
                                if (pSprite->ang == 512)
                                {
                                    pSprite->x = sprite[spriteNum].x;
                                    pSprite->y = sprite[spriteNum].y;
                                }
                                break;
                            }
                    }
                    if (EDUKE32_PREDICT_FALSE(spriteNum == -1))
                    {
                        OSD_Printf(OSD_ERROR "Found lonely Sector Effector (lotag 0) at (%d,%d)\n",
                            TrackerCast(pSprite->x),TrackerCast(pSprite->y));
                        changespritestat(newSprite, STAT_ACTOR);
                        goto SPAWN_END;
                    }
                    pSprite->owner = spriteNum;
                }

                {
                    int const startWall = sector[sectNum].wallptr;
                    int const endWall = startWall+sector[sectNum].wallnum;

                    T2(newSprite) = tempwallptr;
                    for (bssize_t w = startWall; w < endWall; ++w)
                    {
                        g_origins[tempwallptr].x = wall[w].x - pSprite->x;
                        g_origins[tempwallptr].y = wall[w].y - pSprite->y;

                        tempwallptr++;
                        if (EDUKE32_PREDICT_FALSE(tempwallptr >= MAXANIMPOINTS))
                        {
                            Bsprintf(tempbuf, "Too many moving sectors at (%d,%d).\n",
                                TrackerCast(wall[w].x), TrackerCast(wall[w].y));
                            G_GameExit(tempbuf);
                        }
                    }
                }

                if (pSprite->lotag == SE_5 || pSprite->lotag == SE_30_TWO_WAY_TRAIN ||
                        pSprite->lotag == SE_6_SUBWAY || pSprite->lotag == SE_14_SUBWAY_CAR)
                {
#ifdef YAX_ENABLE
                    int outerWall = -1;
#endif
                    int const startWall = sector[sectNum].wallptr;
                    int const endWall   = startWall + sector[sectNum].wallnum;

                    pSprite->extra = (sector[sectNum].hitag != UINT16_MAX);

                    // TRAIN_SECTOR_TO_SE_INDEX
                    sector[sectNum].hitag = newSprite;

                    spriteNum = 0;

                    int foundWall = startWall;

                    for (; foundWall<endWall; foundWall++)
                    {
                        if (wall[ foundWall ].nextsector >= 0 &&
                                sector[ wall[ foundWall ].nextsector].hitag == 0 &&
                                (int16_t)sector[ wall[ foundWall ].nextsector].lotag < 3)
                        {
#ifdef YAX_ENABLE
                            outerWall = wall[foundWall].nextwall;
#endif
                            foundWall = wall[foundWall].nextsector;
                            spriteNum = 1;
                            break;
                        }
                    }

#ifdef YAX_ENABLE
                    pActor->t_data[9] = -1;

                    if (outerWall >= 0)
                    {
                        int upperSect = yax_vnextsec(outerWall, YAX_CEILING);

                        if (upperSect >= 0)
                        {
                            int foundEffector = headspritesect[upperSect];

                            for (; foundEffector >= 0; foundEffector = nextspritesect[foundEffector])
                                if (sprite[foundEffector].picnum == SECTOREFFECTOR && sprite[foundEffector].lotag == pSprite->lotag)
                                    break;

                            if (foundEffector < 0)
                            {
                                Sect_SetInterpolation(upperSect);
                                pActor->t_data[9] = upperSect;
                            }
                        }
                    }
#endif
                    if (spriteNum == 0)
                    {
                        Bsprintf(tempbuf,"Subway found no zero'd sectors with locators\nat (%d,%d).\n",
                            TrackerCast(pSprite->x),TrackerCast(pSprite->y));
                        G_GameExit(tempbuf);
                    }

                    pSprite->owner = -1;
                    T1(newSprite) = foundWall;

                    if (pSprite->lotag != SE_30_TWO_WAY_TRAIN)
                        T4(newSprite) = pSprite->hitag;
                }
                else if (pSprite->lotag == SE_16_REACTOR)
                    T4(newSprite) = sector[sectNum].ceilingz;
                else if (pSprite->lotag == SE_26)
                {
                    T4(newSprite)  = pSprite->x;
                    T5(newSprite)  = pSprite->y;
                    pSprite->zvel  = (pSprite->shade == sector[sectNum].floorshade) ? -256 : 256;  // UP
                    pSprite->shade = 0;
                }
                else if (pSprite->lotag == SE_2_EARTHQUAKE)
                {
                    T6(newSprite) = sector[pSprite->sectnum].floorheinum;
                    sector[pSprite->sectnum].floorheinum = 0;
                }
            }

            switch (pSprite->lotag)
            {
                case SE_6_SUBWAY:
                case SE_14_SUBWAY_CAR:
                    S_FindMusicSFX(sectNum, &spriteNum);
                    // XXX: uh.. what?
                    if (spriteNum == -1)
                        spriteNum = SUBWAY;
                    pActor->lastv.x = spriteNum;
                    fallthrough__;
                case SE_30_TWO_WAY_TRAIN:
                    if (g_netServer || numplayers > 1)
                        break;
                    fallthrough__;
                case SE_0_ROTATING_SECTOR:
                case SE_1_PIVOT:
                case SE_5:
                case SE_11_SWINGING_DOOR:
                case SE_15_SLIDING_DOOR:
                case SE_16_REACTOR:
                case SE_26: Sect_SetInterpolation(sprite[newSprite].sectnum); break;
            }

            changespritestat(newSprite, STAT_EFFECTOR);
            break;

        case SEENINE__STATIC:
        case OOZFILTER__STATIC:
            pSprite->shade = -16;
            if (pSprite->xrepeat <= 8)
            {
                pSprite->cstat   = 32768;
                pSprite->xrepeat = 0;
                pSprite->yrepeat = 0;
            }
            else pSprite->cstat = 1+256;

            pSprite->extra = g_impactDamage << 2;
            pSprite->owner = newSprite;

            changespritestat(newSprite, STAT_STANDABLE);
            break;

        case CRACK1__STATIC:
        case CRACK2__STATIC:
        case CRACK3__STATIC:
        case CRACK4__STATIC:
        case FIREEXT__STATIC:
            if (pSprite->picnum == FIREEXT)
            {
                pSprite->cstat = 257;
                pSprite->extra = g_impactDamage<<2;
            }
            else
            {
                pSprite->cstat |= (pSprite->cstat & 48) ? 1 : 17;
                pSprite->extra = 1;
            }

            if ((!g_netServer && ud.multimode < 2) && pSprite->pal != 0)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
                break;
            }

            pSprite->pal   = 0;
            pSprite->owner = newSprite;
            pSprite->xvel  = 8;

            changespritestat(newSprite, STAT_STANDABLE);
            A_SetSprite(newSprite,CLIPMASK0);
            break;

        case TOILET__STATIC:
        case STALL__STATIC:
            pSprite->lotag = 1;
            pSprite->cstat |= 257;
            pSprite->clipdist = 8;
            pSprite->owner = newSprite;
            break;

        case CANWITHSOMETHING__STATIC:
        case CANWITHSOMETHING2__STATIC:
        case CANWITHSOMETHING3__STATIC:
        case CANWITHSOMETHING4__STATIC:
        case RUBBERCAN__STATIC:
            pSprite->extra = 0;
            fallthrough__;
        case EXPLODINGBARREL__STATIC:
        case HORSEONSIDE__STATIC:
        case FIREBARREL__STATIC:
        case NUKEBARREL__STATIC:
        case FIREVASE__STATIC:
        case NUKEBARRELDENTED__STATIC:
        case NUKEBARRELLEAKED__STATIC:
        case WOODENHORSE__STATIC:
            if (spriteNum >= 0)
                pSprite->xrepeat = pSprite->yrepeat = 32;
            pSprite->clipdist = 72;
            A_Fall(newSprite);
            if (spriteNum >= 0)
                pSprite->owner = spriteNum;
            else pSprite->owner = newSprite;
            fallthrough__;
        case EGG__STATIC:
            if (ud.monsters_off == 1 && pSprite->picnum == EGG)
            {
                pSprite->xrepeat = pSprite->yrepeat = 0;
                changespritestat(newSprite, STAT_MISC);
            }
            else
            {
                if (pSprite->picnum == EGG)
                    pSprite->clipdist = 24;
                pSprite->cstat = 257|(krand()&4);
                changespritestat(newSprite, STAT_ZOMBIEACTOR);
            }
            break;

        case TOILETWATER__STATIC:
            pSprite->shade = -16;
            changespritestat(newSprite, STAT_STANDABLE);
            break;
        }

SPAWN_END:
    if (VM_HaveEvent(EVENT_SPAWN))
    {
        int32_t p;
        int32_t pl=A_FindPlayer(&sprite[newSprite],&p);
        VM_OnEventWithDist_(EVENT_SPAWN,newSprite, pl, p);
    }

    return newSprite;
}

static int G_MaybeTakeOnFloorPal(uspritetype *pSprite, int sectNum)
{
    int const floorPal = sector[sectNum].floorpal;

    if (floorPal && !g_noFloorPal[floorPal] && !A_CheckSpriteFlags(pSprite->owner, SFLAG_NOPAL))
    {
        pSprite->pal = floorPal;
        return 1;
    }

    return 0;
}

template <int rotations>
static int getofs_viewtype(int angDiff)
{
    return ((((angDiff + 3072) & 2047) * rotations + 1024) >> 11) % rotations;
}

template <int rotations>
static int viewtype_mirror(uint16_t & cstat, int frameOffset)
{
    if (frameOffset > rotations / 2)
    {
        cstat |= 4;
        return rotations - frameOffset;
    }

    cstat &= ~4;
    return frameOffset;
}

template <int mirrored_rotations>
static int getofs_viewtype_mirrored(uint16_t & cstat, int angDiff)
{
    return viewtype_mirror<mirrored_rotations*2-2>(cstat, getofs_viewtype<mirrored_rotations*2-2>(angDiff));
}

// XXX: this fucking sucks and needs to be replaced with a SFLAG
static int G_CheckAdultTile(int tileNum)
{
    switch (tileNum)
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

static inline void G_DoEventAnimSprites(int tspriteNum)
{
    int const tsprOwner = tsprite[tspriteNum].owner;

    if ((((unsigned)tsprOwner >= MAXSPRITES || (spriteext[tsprOwner].flags & SPREXT_TSPRACCESS) != SPREXT_TSPRACCESS))
        && tsprite[tspriteNum].statnum != TSPR_TEMP)
        return;

    spriteext[tsprOwner].tspr = &tsprite[tspriteNum];
    VM_OnEvent_(EVENT_ANIMATESPRITES, tsprOwner, screenpeek);
    spriteext[tsprOwner].tspr = NULL;
}

void G_DoSpriteAnimations(int32_t ourx, int32_t oury, int32_t oura, int32_t smoothratio)
{
    int32_t j, frameOffset, playerNum;
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
        uspritetype *const t = &tsprite[j];
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
        uspritetype *const t = &tsprite[j];
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
                frameOffset = getofs_viewtype_mirrored<5>(t->cstat, t->ang - oura);
                t->picnum = s->picnum+frameOffset;
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
                fallthrough__;
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
                if ((t->cstat&16) || (A_CheckEnemySprite(t) &&
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
        uspritetype *const t = &tsprite[j];
        const int32_t i = t->owner;
        // XXX: what's up with the (i < 0) check?
        // NOTE: not const spritetype because set at SET_SPRITE_NOT_TSPRITE (see below).
        uspritetype *const pSprite = (i < 0) ? &tsprite[j] : (uspritetype *)&sprite[i];

        if (ud.lockout && G_CheckAdultTile(DYNAMICTILEMAP(pSprite->picnum)))
        {
            t->xrepeat = t->yrepeat = 0;
            continue;
        }

        if (pSprite->picnum == NATURALLIGHTNING)
        {
            t->shade = -127;
            t->cstat |= 8192;
        }

        if (t->statnum == TSPR_TEMP)
            continue;

        Bassert(i >= 0);

        const DukePlayer_t *const ps = (pSprite->statnum != STAT_ACTOR && pSprite->picnum == APLAYER && pSprite->owner >= 0) ? g_player[P_GetP(pSprite)].ps : NULL;
        if (ps && ps->newowner == -1)
        {
            t->x -= mulscale16(65536-smoothratio,ps->pos.x-ps->opos.x);
            t->y -= mulscale16(65536-smoothratio,ps->pos.y-ps->opos.y);
            // dirty hack
            if (ps->dead_flag) t->z = ps->opos.z;
            t->z += mulscale16(smoothratio,ps->pos.z-ps->opos.z) -
                (ps->dead_flag ? 0 : PHEIGHT) + PHEIGHT;
        }
        else if ((pSprite->statnum == STAT_DEFAULT && pSprite->picnum != CRANEPOLE) || pSprite->statnum == STAT_PLAYER ||
                 pSprite->statnum == STAT_STANDABLE || pSprite->statnum == STAT_PROJECTILE || pSprite->statnum == STAT_MISC || pSprite->statnum == STAT_ACTOR)
        {
            t->x -= mulscale16(65536-smoothratio,pSprite->x-actor[i].bpos.x);
            t->y -= mulscale16(65536-smoothratio,pSprite->y-actor[i].bpos.y);
            t->z -= mulscale16(65536-smoothratio,pSprite->z-actor[i].bpos.z);
        }

        const int32_t sect = pSprite->sectnum;

        curframe = AC_CURFRAME(actor[i].t_data);
#if !defined LUNATIC
        scrofs_action = AC_ACTION_ID(actor[i].t_data);
#else
        startframe = actor[i].ac.startframe;
        viewtype = actor[i].ac.viewtype;
#endif
        switchpic = pSprite->picnum;
        // Some special cases because dynamictostatic system can't handle
        // addition to constants.
        if ((pSprite->picnum >= SCRAP6) && (pSprite->picnum<=SCRAP6+7))
            switchpic = SCRAP5;
        else if ((pSprite->picnum==MONEY+1) || (pSprite->picnum==MAIL+1) || (pSprite->picnum==PAPER+1))
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
            fallthrough__;
        case PUKE__STATIC:
        case MONEY__STATIC:
            //case MONEY+1__STATIC:
        case MAIL__STATIC:
            //case MAIL+1__STATIC:
        case PAPER__STATIC:
            //case PAPER+1__STATIC:
            if (ud.lockout && pSprite->pal == 2)
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
                int16_t const sqa = getangle(sprite[pSprite->owner].x - g_player[screenpeek].ps->pos.x,
                                       sprite[pSprite->owner].y - g_player[screenpeek].ps->pos.y);
                int16_t const sqb = getangle(sprite[pSprite->owner].x - t->x, sprite[pSprite->owner].y - t->y);

                if (klabs(G_GetAngleDelta(sqa,sqb)) > 512)
                    if (ldist(&sprite[pSprite->owner],(const spritetype *)t) < ldist(&sprite[g_player[screenpeek].ps->i],&sprite[pSprite->owner]))
                        t->xrepeat = t->yrepeat = 0;
            }
            continue;
        case BURNING__STATIC:
        case BURNING2__STATIC:
            if (sprite[pSprite->owner].statnum == STAT_PLAYER)
            {
                int const playerNum = P_Get(pSprite->owner);

                if (display_mirror == 0 && playerNum == screenpeek && g_player[playerNum].ps->over_shoulder_on == 0)
                    t->xrepeat = 0;
                else
                {
                    t->ang = getangle(ourx - t->x, oury - t->y);
                    t->x   = sprite[pSprite->owner].x + (sintable[(t->ang + 512) & 2047] >> 10);
                    t->y   = sprite[pSprite->owner].y + (sintable[t->ang & 2047] >> 10);
                }
            }
            break;

        case ATOMICHEALTH__STATIC:
            t->z -= ZOFFSET6;
            break;
        case CRYSTALAMMO__STATIC:
            t->shade = (sintable[(totalclock<<4)&2047]>>10);
            continue;
        case VIEWSCREEN__STATIC:
        case VIEWSCREEN2__STATIC:
        {
            int const viewscrShift = G_GetViewscreenSizeShift(t);
            int const viewscrTile = TILE_VIEWSCR-viewscrShift;

            if (g_curViewscreen >= 0 && actor[OW(i)].t_data[0] == 1)
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
                if (spritesortcnt < maxspritesonscreen)
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
            frameOffset = getofs_viewtype_mirrored<7>(t->cstat, pSprite->ang - getangle(pSprite->x-ourx, pSprite->y-oury));
            t->picnum = RPG+frameOffset;
            break;

        case RECON__STATIC:
#ifdef USE_OPENGL
            if (getrendermode() >= REND_POLYMOST && usemodels && md_tilehasmodel(t->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                t->cstat &= ~4;
                break;
            }
#endif
            frameOffset = getofs_viewtype_mirrored<7>(t->cstat, pSprite->ang - getangle(pSprite->x-ourx, pSprite->y-oury));

            // RECON_T4
            if (klabs(curframe) > 64)
                frameOffset += 7;  // tilted recon car

            t->picnum = RECON+frameOffset;

            break;

        case APLAYER__STATIC:
            playerNum = P_GetP(pSprite);

            if (t->pal == 1) t->z -= (18<<8);

            if (g_player[playerNum].ps->over_shoulder_on > 0 && g_player[playerNum].ps->newowner < 0)
            {
                t->ang = g_player[playerNum].ps->ang +
                    mulscale16((((g_player[playerNum].ps->ang+1024 - g_player[playerNum].ps->oang)&2047)-1024),
                               smoothratio);
#ifdef USE_OPENGL
                if (bpp > 8 && usemodels && md_tilehasmodel(t->picnum, t->pal) >= 0)
                {
                    static int32_t targetang = 0;

                    if (g_player[playerNum].inputBits->extbits&(1<<1))
                    {
                        if (g_player[playerNum].inputBits->extbits&(1<<2))targetang += 16;
                        else if (g_player[playerNum].inputBits->extbits&(1<<3)) targetang -= 16;
                        else if (targetang > 0) targetang -= targetang>>2;
                        else if (targetang < 0) targetang += (-targetang)>>2;
                    }
                    else
                    {
                        if (g_player[playerNum].inputBits->extbits&(1<<2))targetang -= 16;
                        else if (g_player[playerNum].inputBits->extbits&(1<<3)) targetang += 16;
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

            if ((g_netServer || ud.multimode > 1) && (display_mirror || screenpeek != playerNum || pSprite->owner == -1))
            {
                if (ud.showweapons && sprite[g_player[playerNum].ps->i].extra > 0 && g_player[playerNum].ps->curr_weapon > 0
                        && spritesortcnt < maxspritesonscreen)
                {
                    uspritetype *const newTspr       = &tsprite[spritesortcnt];
                    int const          currentWeapon = g_player[playerNum].ps->curr_weapon;

                    *newTspr         = *t;
                    newTspr->statnum = TSPR_TEMP;
                    newTspr->cstat   = 0;
                    newTspr->pal     = 0;
                    newTspr->picnum  = (currentWeapon == GROW_WEAPON ? GROWSPRITEICON : WeaponPickupSprites[currentWeapon]);
                    newTspr->z       = (pSprite->owner >= 0) ? g_player[playerNum].ps->pos.z - ZOFFSET4 : pSprite->z - (51 << 8);
                    newTspr->xrepeat = (newTspr->picnum == HEAVYHBOMB) ? 10 : 16;
                    newTspr->yrepeat = newTspr->xrepeat;

                    spritesortcnt++;
                }

                if (g_player[playerNum].inputBits->extbits & (1 << 7) && !ud.pause_on && spritesortcnt < maxspritesonscreen)
                {
                    uspritetype *const playerTyping = t;

                    playerTyping->statnum = TSPR_TEMP;
                    playerTyping->yrepeat = (t->yrepeat >> 3);

                    if (playerTyping->yrepeat < 4)
                        playerTyping->yrepeat = 4;

                    playerTyping->cstat   = 0;
                    playerTyping->picnum  = RESPAWNMARKERGREEN;
                    playerTyping->z       = (pSprite->owner >= 0) ? (g_player[playerNum].ps->pos.z - (20 << 8)) : (pSprite->z - (96 << 8));
                    playerTyping->xrepeat = 32;
                    playerTyping->yrepeat = 32;
                    playerTyping->pal     = 20;

                    spritesortcnt++;
                }
            }

            if (pSprite->owner == -1)
            {
#ifdef USE_OPENGL
                if (getrendermode() >= REND_POLYMOST && usemodels && md_tilehasmodel(pSprite->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
                {
                    frameOffset = 0;
                    t->cstat &= ~4;
                }
                else
#endif
                    frameOffset = getofs_viewtype_mirrored<5>(t->cstat, pSprite->ang - oura);

                if (sector[pSprite->sectnum].lotag == ST_2_UNDERWATER) frameOffset += 1795-1405;
                else if ((actor[i].floorz-pSprite->z) > (64<<8)) frameOffset += 60;

                t->picnum += frameOffset;
                t->pal = g_player[playerNum].ps->palookup;

                goto PALONLY;
            }

            if (g_player[playerNum].ps->on_crane == -1 && (sector[pSprite->sectnum].lotag&0x7ff) != 1)  // ST_1_ABOVE_WATER ?
            {
                l = pSprite->z-actor[g_player[playerNum].ps->i].floorz+(3<<8);
                // SET_SPRITE_NOT_TSPRITE
                if (l > 1024 && pSprite->yrepeat > 32 && pSprite->extra > 0)
                    pSprite->yoffset = (int8_t)tabledivide32_noinline(l, pSprite->yrepeat<<2);
                else pSprite->yoffset=0;
            }

            if (g_player[playerNum].ps->newowner > -1)
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

            if (ud.camerasprite == -1 && g_player[playerNum].ps->newowner == -1)
            {
                if (pSprite->owner >= 0 && display_mirror == 0 && g_player[playerNum].ps->over_shoulder_on == 0)
                {
                    if ((!g_netServer && ud.multimode < 2) || ((g_netServer || ud.multimode > 1) && playerNum == screenpeek))
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
                        if (getrendermode() >= REND_POLYMOST && usemodels && md_tilehasmodel(pSprite->picnum, t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
                        {
                            frameOffset = 0;
                            t->cstat &= ~4;
                        }
                        else
#endif
                            frameOffset = getofs_viewtype_mirrored<5>(t->cstat, pSprite->ang - oura);

                        if (sector[t->sectnum].lotag == ST_2_UNDERWATER) frameOffset += 1795-1405;
                        else if ((actor[i].floorz-pSprite->z) > (64<<8)) frameOffset += 60;

                        t->picnum += frameOffset;
                        t->pal = g_player[playerNum].ps->palookup;
                    }
                }
            }
PALONLY:
            G_MaybeTakeOnFloorPal(t, sect);

            if (pSprite->owner == -1) continue;

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
            fallthrough__;
        case SCRAP1__STATIC:
        case SCRAP2__STATIC:
        case SCRAP3__STATIC:
        case SCRAP4__STATIC:
        case SCRAP5__STATIC:
            if (actor[i].picnum == BLIMP && t->picnum == SCRAP1 && pSprite->yvel >= 0)
                t->picnum = pSprite->yvel < MAXUSERTILES ? pSprite->yvel : 0;
            else t->picnum += T1(i);
            t->shade -= 6;

            G_MaybeTakeOnFloorPal(t, sect);
            break;

        case WATERBUBBLE__STATIC:
            if (sector[t->sectnum].floorpicnum == FLOORSLIME)
            {
                t->pal = 7;
                break;
            }
            fallthrough__;
        default:
            G_MaybeTakeOnFloorPal(t, sect);
            break;
        }

        if (G_HaveActor(pSprite->picnum))
        {
#if !defined LUNATIC
            if ((unsigned)scrofs_action + ACTION_VIEWTYPE >= (unsigned)g_scriptSize)
                goto skip;

            l = apScript[scrofs_action + ACTION_VIEWTYPE];
            uint16_t const action_flags = apScript[scrofs_action + ACTION_FLAGS];
#else
            l = viewtype;
            uint16_t const action_flags = actor[i].ac.flags;
#endif

            int const invertp = l < 0;
            l = klabs(l);

#ifdef USE_OPENGL
            if (getrendermode() >= REND_POLYMOST && usemodels && md_tilehasmodel(pSprite->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                frameOffset = 0;
                t->cstat &= ~4;
            }
            else
#endif
            {
                int const viewAng = ((l > 4 && l != 8) || action_flags & AF_VIEWPOINT) ? getangle(pSprite->x-ourx, pSprite->y-oury) : oura;
                int const angDiff = invertp ? viewAng - pSprite->ang : pSprite->ang - viewAng;

                switch (l)
                {
                case 2:
                    frameOffset = getofs_viewtype<8>(angDiff) & 1;
                    break;

                case 3:
                case 4:
                    frameOffset = viewtype_mirror<7>(t->cstat, getofs_viewtype<16>(angDiff) & 7);
                    break;

                case 5:
                    frameOffset = getofs_viewtype_mirrored<5>(t->cstat, angDiff);
                    break;
                case 7:
                    frameOffset = getofs_viewtype_mirrored<7>(t->cstat, angDiff);
                    break;
                case 8:
                    frameOffset = getofs_viewtype<8>(angDiff);
                    t->cstat &= ~4;
                    break;
                case 9:
                    frameOffset = getofs_viewtype_mirrored<9>(t->cstat, angDiff);
                    break;
                case 12:
                    frameOffset = getofs_viewtype<12>(angDiff);
                    t->cstat &= ~4;
                    break;
                case 16:
                    frameOffset = getofs_viewtype<16>(angDiff);
                    t->cstat &= ~4;
                    break;
                default:
                    frameOffset = 0;
                    break;
                }
            }

#if !defined LUNATIC
            t->picnum += frameOffset + apScript[scrofs_action + ACTION_STARTFRAME] + l*curframe;
#else
            t->picnum += frameOffset + startframe + l*curframe;
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
                (A_CheckEnemySprite(pSprite) || A_CheckSpriteFlags(t->owner,SFLAG_NVG) || pSprite->picnum == APLAYER || pSprite->statnum == STAT_DUMMYPLAYER))
        {
            t->pal = 6;
            t->shade = 0;
        }

        // Fake floor shadow, implemented by inserting a new tsprite.
        if (pSprite->statnum == STAT_DUMMYPLAYER || A_CheckEnemySprite(pSprite) || A_CheckSpriteFlags(t->owner,SFLAG_SHADOW) || (pSprite->picnum == APLAYER && pSprite->owner >= 0))
            if (t->statnum != TSPR_TEMP && pSprite->picnum != EXPLOSION2 && pSprite->picnum != HANGLIGHT && pSprite->picnum != DOMELITE && pSprite->picnum != HOTMEAT)
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

                if (ud.shadows && spritesortcnt < (maxspritesonscreen-2) && getrendermode() != REND_POLYMER)
                {
                    int const shadowZ = ((sector[sect].lotag & 0xff) > 2 || pSprite->statnum == STAT_PROJECTILE ||
                                   pSprite->statnum == STAT_MISC || pSprite->picnum == DRONE || pSprite->picnum == COMMANDER)
                                  ? sector[sect].floorz
                                  : actor[i].floorz;

                    if ((pSprite->z-shadowZ) < ZOFFSET3 && g_player[screenpeek].ps->pos.z < shadowZ)
                    {
                        uspritetype *const tsprShadow = &tsprite[spritesortcnt];

                        *tsprShadow         = *t;
                        tsprShadow->statnum = TSPR_TEMP;
                        tsprShadow->yrepeat = (t->yrepeat >> 3);

                        if (t->yrepeat < 4)
                            t->yrepeat = 4;

                        tsprShadow->shade   = 127;
                        tsprShadow->cstat  |= 2;
                        tsprShadow->z       = shadowZ;
                        tsprShadow->pal     = 4;


#ifdef USE_OPENGL
                        if (getrendermode() >= REND_POLYMOST)
                        {
                            if (usemodels && md_tilehasmodel(t->picnum,t->pal) >= 0)
                            {
                                tsprShadow->yrepeat = 0;
                                // 512:trans reverse
                                //1024:tell MD2SPRITE.C to use Z-buffer hacks to hide overdraw issues
                                tsprShadow->extra |= TSPR_EXTRA_MDHACK;
                                tsprShadow->cstat |= 512;
                            }
                            else
                            {
                                int const ii
                                = getangle(tsprShadow->x - g_player[screenpeek].ps->pos.x, tsprShadow->y - g_player[screenpeek].ps->pos.y);

                                tsprShadow->x += sintable[(ii+2560)&2047]>>9;
                                tsprShadow->y += sintable[(ii+2048)&2047]>>9;
                            }
                        }
#endif
                        spritesortcnt++;
                    }
                }
            }

        switch (DYNAMICTILEMAP(pSprite->picnum))
        {
        case LASERLINE__STATIC:
            if (sector[t->sectnum].lotag == ST_2_UNDERWATER) t->pal = 8;
            t->z = sprite[pSprite->owner].z-(3<<8);
            if (g_tripbombLaserMode == 2 && g_player[screenpeek].ps->heat_on == 0)
                t->yrepeat = 0;
            fallthrough__;
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
            fallthrough__;
        case BURNING__STATIC:
        case BURNING2__STATIC:
            if (sprite[pSprite->owner].picnum != TREE1 && sprite[pSprite->owner].picnum != TREE2)
                t->z = actor[t->owner].floorz;
            t->shade = -127;
            fallthrough__;
        case SMALLSMOKE__STATIC:
            t->cstat |= 8192+1024;
            break;
        case COOLEXPLOSION1__STATIC:
            t->shade = -127;
            t->cstat |= 8192+1024;
            t->picnum += (pSprite->shade>>1);
            break;
        case PLAYERONWATER__STATIC:
#ifdef USE_OPENGL
            if (getrendermode() >= REND_POLYMOST && usemodels && md_tilehasmodel(pSprite->picnum,pSprite->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                frameOffset = 0;
                t->cstat &= ~4;
            }
            else
#endif
                frameOffset = getofs_viewtype_mirrored<5>(t->cstat, t->ang - oura);

            t->picnum = pSprite->picnum+frameOffset+((T1(i)<4)*5);
            t->shade = sprite[pSprite->owner].shade;

            break;

        case WATERSPLASH2__STATIC:
            // WATERSPLASH_T2
            t->picnum = WATERSPLASH2+T2(i);
            break;
        case SHELL__STATIC:
            t->picnum = pSprite->picnum+(T1(i)&1);
            fallthrough__;
        case SHOTGUNSHELL__STATIC:
            t->cstat |= 12;
            if (T1(i) > 2) t->cstat &= ~16;
            else if (T1(i) > 1) t->cstat &= ~4;
            break;
        case FRAMEEFFECT1_13__STATIC:
            if (PLUTOPAK) break;
            fallthrough__;
        case FRAMEEFFECT1__STATIC:
            if (pSprite->owner >= 0 && sprite[pSprite->owner].statnum < MAXSTATUS)
            {
                if (sprite[pSprite->owner].picnum == APLAYER)
                    if (ud.camerasprite == -1)
                        if (screenpeek == P_Get(pSprite->owner) && display_mirror == 0)
                        {
                            t->owner = -1;
                            break;
                        }
                if ((sprite[pSprite->owner].cstat&32768) == 0)
                {
                    if (!actor[pSprite->owner].dispicnum)
                        t->picnum = actor[i].t_data[1];
                    else t->picnum = actor[pSprite->owner].dispicnum;

                    if (!G_MaybeTakeOnFloorPal(t, sect))
                        t->pal = sprite[pSprite->owner].pal;

                    t->shade = sprite[pSprite->owner].shade;
                    t->ang = sprite[pSprite->owner].ang;
                    t->cstat = 2|sprite[pSprite->owner].cstat;
                }
            }
            break;

        case CAMERA1__STATIC:
        case RAT__STATIC:
#ifdef USE_OPENGL
            if (getrendermode() >= REND_POLYMOST && usemodels && md_tilehasmodel(pSprite->picnum,pSprite->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                t->cstat &= ~4;
                break;
            }
#endif
            frameOffset = getofs_viewtype_mirrored<5>(t->cstat, t->ang - oura);
            t->picnum = pSprite->picnum+frameOffset;
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
    if (dir!=0)
    {
        if (ud.screen_size == 8 && (dir>0)==(int32_t)ud.statusbarmode)
            ud.statusbarmode = !ud.statusbarmode;
        else
        if (ud.screen_size == 4 && (dir>0)==(int32_t)ud.althud)
            ud.althud = !ud.althud;
        else
            ud.screen_size += dir;
    }
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
extern int G_StartRTS(int lumpNum, int localPlayer)
{
    if (!ud.lockout && ud.config.SoundToggle &&
        RTS_IsInitialized() && g_RTSPlaying == 0 && (ud.config.VoiceToggle & (localPlayer ? 1 : 4)))
    {
        char *const pData = (char *)RTS_GetSound(lumpNum - 1);

        if (pData != NULL)
        {
            FX_Play3D(pData, RTS_SoundLength(lumpNum - 1), FX_ONESHOT, 0, 0, FX_VOLUME(1), 255, -lumpNum);
            g_RTSPlaying = 7;
            return 1;
        }
    }

    return 0;
}

void G_StartMusic(void)
{
    int const levelNum = g_musicIndex;
    Bassert(g_mapInfo[levelNum].musicfn != NULL);

    S_PlayMusic(g_mapInfo[levelNum].musicfn);

    Bsnprintf(apStrings[QUOTE_MUSIC], MAXQUOTELEN, "Playing %s", g_mapInfo[levelNum].musicfn);
    P_DoQuote(QUOTE_MUSIC, g_player[myconnectindex].ps);
}

void G_HandleLocalKeys(void)
{
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

            int const fwdTics = (15 << (int)ALT_IS_PRESSED) << (2 * (int)SHIFTS_IS_PRESSED);
            g_demo_goalCnt    = g_demo_paused ? g_demo_cnt + 1 : g_demo_cnt + REALGAMETICSPERSEC * fwdTics;
            g_demo_rewind     = 0;

            if (g_demo_goalCnt > g_demo_totalCnt)
                g_demo_goalCnt = 0;
            else
                Demo_PrepareWarp();
        }
        else if (KB_KeyPressed(sc_kpad_4))
        {
            KB_ClearKeyDown(sc_kpad_4);

            int const rewindTics = (15 << (int)ALT_IS_PRESSED) << (2 * (int)SHIFTS_IS_PRESSED);
            g_demo_goalCnt       = g_demo_paused ? g_demo_cnt - 1 : g_demo_cnt - REALGAMETICSPERSEC * rewindTics;
            g_demo_rewind        = 1;

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
        int ridiculeNum = 0;

        // NOTE: sc_F1 .. sc_F10 are contiguous. sc_F11 is not sc_F10+1.
        for (bssize_t j=sc_F1; j<=sc_F10; j++)
            if (KB_UnBoundKeyPressed(j))
            {
                KB_ClearKeyDown(j);
                ridiculeNum = j - sc_F1 + 1;
                break;
            }

        if (ridiculeNum)
        {
            if (SHIFTS_IS_PRESSED)
            {
                if (ridiculeNum == 5 && g_player[myconnectindex].ps->fta > 0 && g_player[myconnectindex].ps->ftq == QUOTE_MUSIC)
                {
                    const int32_t maxi = VOLUMEALL ? MUS_FIRST_SPECIAL : 6;

                    do
                    {
                        g_musicIndex++;
                        if (g_musicIndex >= maxi)
                            g_musicIndex = 0;
                    }
                    while (g_mapInfo[g_musicIndex].musicfn == NULL);

                    G_StartMusic();

                    return;
                }

                G_AddUserQuote(ud.ridecule[ridiculeNum-1]);

#ifndef NETCODE_DISABLE
                tempbuf[0] = PACKET_MESSAGE;
                tempbuf[1] = 255;
                tempbuf[2] = 0;
                Bstrcat(tempbuf+2,ud.ridecule[ridiculeNum-1]);

                ridiculeNum = 2+strlen(ud.ridecule[ridiculeNum-1]);

                tempbuf[ridiculeNum++] = myconnectindex;

                if (g_netClient)
                    enet_peer_send(g_netClientPeer, CHAN_CHAT, enet_packet_create(tempbuf, ridiculeNum, 0));
                else if (g_netServer)
                    enet_host_broadcast(g_netServer, CHAN_CHAT, enet_packet_create(tempbuf, ridiculeNum, 0));
#endif
                pus = NUMPAGES;
                pub = NUMPAGES;

                return;
            }

            // Not SHIFT -- that is, either some ALT or WIN.
            if (G_StartRTS(ridiculeNum, 1))
            {
#ifndef NETCODE_DISABLE
                if ((g_netServer || ud.multimode > 1))
                {
                    tempbuf[0] = PACKET_RTS;
                    tempbuf[1] = ridiculeNum;
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

            Menu_Change(MENU_STORY);
            S_PauseSounds(1);
            Menu_Open(myconnectindex);

            if ((!g_netServer && ud.multimode < 2))
            {
                ready2send = 0;
                totalclock = ototalclock;
                screenpeek = myconnectindex;
            }
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

                Menu_Change(MENU_SAVE);

                g_screenCapture = 1;
                G_DrawRooms(myconnectindex,65536);
                g_screenCapture = 0;

                S_PauseSounds(1);
                Menu_Open(myconnectindex);

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
                Menu_Change(MENU_LOAD);
                S_PauseSounds(1);
                Menu_Open(myconnectindex);

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
            Menu_Open(myconnectindex);

            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
            {
                ready2send = 0;
                totalclock = ototalclock;
            }

            Menu_Change(MENU_SOUND_INGAME);
        }

        if (KB_UnBoundKeyPressed(sc_F5) && ud.config.MusicToggle)
        {
            map_t *const pMapInfo    = &g_mapInfo[g_musicIndex];
            char *const  musicString = apStrings[QUOTE_MUSIC];

            KB_ClearKeyDown(sc_F5);

            if (pMapInfo->musicfn != NULL)
                Bsnprintf(musicString, MAXQUOTELEN, "%s.  Use SHIFT-F5 to change.", pMapInfo->musicfn);
            else
                musicString[0] = '\0';

            P_DoQuote(QUOTE_MUSIC, g_player[myconnectindex].ps);
        }

        if ((KB_UnBoundKeyPressed(sc_F6) || g_doQuickSave == 1) && (g_player[myconnectindex].ps->gm&MODE_GAME))
        {
            KB_ClearKeyDown(sc_F6);

            g_doQuickSave = 0;

            if (!g_lastusersave.isValid())
                goto FAKE_F2;

            KB_FlushKeyboardQueue();

            if (sprite[g_player[myconnectindex].ps->i].extra <= 0)
            {
                P_DoQuote(QUOTE_SAVE_DEAD,g_player[myconnectindex].ps);
                return;
            }

            g_screenCapture = 1;
            G_DrawRooms(myconnectindex,65536);
            g_screenCapture = 0;

            if (g_lastusersave.isValid())
            {
                savebrief_t & sv = g_lastusersave;

                // dirty hack... char 127 in last position indicates an auto-filled name
                if (sv.name[MAXSAVEGAMENAME] == 127)
                {
                    strncpy(sv.name, g_mapInfo[ud.volume_number * MAXLEVELS + ud.level_number].name, MAXSAVEGAMENAME);
                    sv.name[MAXSAVEGAMENAME] = 127;
                }

                g_quickload = &sv;
                G_SavePlayerMaybeMulti(sv);
            }
        }

        if (KB_UnBoundKeyPressed(sc_F7))
        {
            KB_ClearKeyDown(sc_F7);

            g_player[myconnectindex].ps->over_shoulder_on = !g_player[myconnectindex].ps->over_shoulder_on;

            CAMERADIST  = 0;
            CAMERACLOCK = totalclock;

            P_DoQuote(QUOTE_VIEW_MODE_OFF + g_player[myconnectindex].ps->over_shoulder_on, g_player[myconnectindex].ps);
        }

        if (KB_UnBoundKeyPressed(sc_F8))
        {
            KB_ClearKeyDown(sc_F8);

            int const fta = !ud.fta_on;
            ud.fta_on     = 1;
            P_DoQuote(fta ? QUOTE_MESSAGES_ON : QUOTE_MESSAGES_OFF, g_player[myconnectindex].ps);
            ud.fta_on     = fta;
        }

        if ((KB_UnBoundKeyPressed(sc_F9) || g_doQuickSave == 2) && (g_player[myconnectindex].ps->gm&MODE_GAME))
        {
            KB_ClearKeyDown(sc_F9);

            g_doQuickSave = 0;

            if (g_quickload == nullptr || !g_quickload->isValid())
                goto FAKE_F3;
            else if (g_quickload->isValid())
            {
                KB_FlushKeyboardQueue();
                KB_ClearKeysDown();
                S_PauseSounds(1);
                G_LoadPlayerMaybeMulti(*g_quickload);
            }
        }

        if (KB_UnBoundKeyPressed(sc_F10))
        {
            KB_ClearKeyDown(sc_F10);

            Menu_Change(MENU_QUIT_INGAME);
            S_PauseSounds(1);
            Menu_Open(myconnectindex);

            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
            {
                ready2send = 0;
                totalclock = ototalclock;
            }
        }

        if (KB_UnBoundKeyPressed(sc_F11))
        {
            KB_ClearKeyDown(sc_F11);

            Menu_Change(MENU_COLCORR_INGAME);
            S_PauseSounds(1);
            Menu_Open(myconnectindex);

            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
            {
                ready2send = 0;
                totalclock = ototalclock;
            }
        }

        if (ud.overhead_on != 0)
        {
            int const timerOffset = (totalclock - nonsharedtimer);
            nonsharedtimer += timerOffset;

            if (BUTTON(gamefunc_Enlarge_Screen))
                g_player[myconnectindex].ps->zoom += mulscale6(timerOffset, max(g_player[myconnectindex].ps->zoom, 256));

            if (BUTTON(gamefunc_Shrink_Screen))
                g_player[myconnectindex].ps->zoom -= mulscale6(timerOffset, max(g_player[myconnectindex].ps->zoom, 256));

            g_player[myconnectindex].ps->zoom = clamp(g_player[myconnectindex].ps->zoom, 48, 2048);
        }
    }

    if (I_EscapeTrigger() && ud.overhead_on && g_player[myconnectindex].ps->newowner == -1)
    {
        I_EscapeTriggerClear();
        ud.last_overhead = ud.overhead_on;
        ud.overhead_on   = 0;
        ud.scrollmode    = 0;
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

    return S_DefineAudioIfSupported(&g_mapInfo[sel].musicfn, name);
}

static int parsedefinitions_game(scriptfile *, int);

static void parsedefinitions_game_include(const char *fileName, scriptfile *pScript, const char *cmdtokptr, int const firstPass)
{
    scriptfile *included = scriptfile_fromfile(fileName);

    if (!included)
    {
        if (!Bstrcasecmp(cmdtokptr,"null") || pScript == NULL) // this is a bit overboard to prevent unused parameter warnings
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
        parsedefinitions_game(included, firstPass);
        scriptfile_close(included);
    }
}

static void parsedefinitions_game_animsounds(scriptfile *pScript, char * blockEnd, char const * fileName, dukeanim_t * animPtr)
{
    Bfree(animPtr->sounds);

    size_t numPairs = 0, allocSize = 4;

    animPtr->sounds = (animsound_t *)Xmalloc(allocSize * sizeof(animsound_t));
    animPtr->numsounds = 0;

    int defError = 1;
    uint16_t lastFrameNum = 1;

    while (pScript->textptr < blockEnd)
    {
        int32_t frameNum;
        int32_t soundNum;

        // HACK: we've reached the end of the list
        //  (hack because it relies on knowledge of
        //   how scriptfile_* preprocesses the text)
        if (blockEnd - pScript->textptr == 1)
            break;

        // would produce error when it encounters the closing '}'
        // without the above hack
        if (scriptfile_getnumber(pScript, &frameNum))
            break;

        defError = 1;

        if (scriptfile_getsymbol(pScript, &soundNum))
            break;

        // frame numbers start at 1 for us
        if (frameNum <= 0)
        {
            initprintf("Error: frame number must be greater zero on line %s:%d\n", pScript->filename,
                       scriptfile_getlinum(pScript, pScript->ltextptr));
            break;
        }

        if (frameNum < lastFrameNum)
        {
            initprintf("Error: frame numbers must be in (not necessarily strictly)"
                       " ascending order (line %s:%d)\n",
                       pScript->filename, scriptfile_getlinum(pScript, pScript->ltextptr));
            break;
        }

        lastFrameNum = frameNum;

        if ((unsigned)soundNum >= MAXSOUNDS && soundNum != -1)
        {
            initprintf("Error: sound number #%d invalid on line %s:%d\n", soundNum, pScript->filename,
                       scriptfile_getlinum(pScript, pScript->ltextptr));
            break;
        }

        if (numPairs >= allocSize)
        {
            allocSize *= 2;
            animPtr->sounds = (animsound_t *)Xrealloc(animPtr->sounds, allocSize * sizeof(animsound_t));
        }

        defError = 0;

        animsound_t & sound = animPtr->sounds[numPairs];
        sound.frame = frameNum;
        sound.sound = soundNum;

        ++numPairs;
    }

    if (!defError)
    {
        animPtr->numsounds = numPairs;
        // initprintf("Defined sound sequence for hi-anim \"%s\" with %d frame/sound pairs\n",
        //           hardcoded_anim_tokens[animnum].text, numpairs);
    }
    else
    {
        DO_FREE_AND_NULL(animPtr->sounds);
        initprintf("Failed defining sound sequence for anim \"%s\".\n", fileName);
    }
}

static int parsedefinitions_game(scriptfile *pScript, int firstPass)
{
    int   token;
    char *pToken;

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

    static const tokenlist soundTokens[] =
    {
        { "id",   T_ID  },
        { "file", T_FILE },
    };

    static const tokenlist animTokens [] =
    {
        { "delay", T_DELAY },
        { "aspect", T_ASPECT },
        { "sounds", T_SOUND },
        { "forcefilter", T_FORCEFILTER },
        { "forcenofilter", T_FORCENOFILTER },
        { "texturefilter", T_TEXTUREFILTER },
    };

    do
    {
        token  = getatoken(pScript, tokens, ARRAY_SIZE(tokens));
        pToken = pScript->ltextptr;

        switch (token)
        {
        case T_LOADGRP:
        {
            char *fileName;

            pathsearchmode = 1;
            if (!scriptfile_getstring(pScript,&fileName) && firstPass)
            {
                if (initgroupfile(fileName) == -1)
                    initprintf("Could not find file \"%s\".\n", fileName);
                else
                {
                    initprintf("Using file \"%s\" as game data.\n", fileName);
                    if (!g_noAutoLoad && !ud.config.NoAutoLoad)
                        G_DoAutoload(fileName);
                }
            }

            pathsearchmode = 0;
        }
        break;
        case T_CACHESIZE:
        {
            int32_t cacheSize;

            if (scriptfile_getnumber(pScript, &cacheSize) || !firstPass)
                break;

            if (cacheSize > 0)
                MAXCACHE1DSIZE = cacheSize << 10;
        }
        break;
        case T_INCLUDE:
        {
            char *fileName;

            if (!scriptfile_getstring(pScript, &fileName))
                parsedefinitions_game_include(fileName, pScript, pToken, firstPass);

            break;
        }
        case T_INCLUDEDEFAULT:
        {
            parsedefinitions_game_include(G_DefaultDefFile(), pScript, pToken, firstPass);
            break;
        }
        case T_NOAUTOLOAD:
            if (firstPass)
                g_noAutoLoad = 1;
            break;
        case T_MUSIC:
        {
            char *tokenPtr = pScript->ltextptr;
            char *musicID  = NULL;
            char *fileName = NULL;
            char *musicEnd;

            if (scriptfile_getbraces(pScript, &musicEnd))
                break;

            while (pScript->textptr < musicEnd)
            {
                switch (getatoken(pScript, soundTokens, ARRAY_SIZE(soundTokens)))
                {
                    case T_ID: scriptfile_getstring(pScript, &musicID); break;
                    case T_FILE: scriptfile_getstring(pScript, &fileName); break;
                }
            }

            if (!firstPass)
            {
                if (musicID==NULL)
                {
                    initprintf("Error: missing ID for music definition near line %s:%d\n",
                               pScript->filename, scriptfile_getlinum(pScript,tokenPtr));
                    break;
                }

                if (fileName == NULL || check_file_exist(fileName))
                    break;

                if (S_DefineMusic(musicID, fileName) == -1)
                    initprintf("Error: invalid music ID on line %s:%d\n", pScript->filename, scriptfile_getlinum(pScript, tokenPtr));
            }
        }
        break;

        case T_CUTSCENE:
        {
            char *fileName = NULL;

            scriptfile_getstring(pScript, &fileName);

            char *animEnd;

            if (scriptfile_getbraces(pScript, &animEnd))
                break;

            if (!firstPass)
            {
                dukeanim_t *animPtr = Anim_Find(fileName);

                if (!animPtr)
                {
                    animPtr = Anim_Create(fileName);
                    animPtr->framedelay = 10;
                    animPtr->frameflags = 0;
                }

                int32_t temp;

                while (pScript->textptr < animEnd)
                {
                    switch (getatoken(pScript, animTokens, ARRAY_SIZE(animTokens)))
                    {
                        case T_DELAY:
                            scriptfile_getnumber(pScript, &temp);
                            animPtr->framedelay = temp;
                            break;
                        case T_ASPECT:
                        {
                            double dtemp, dtemp2;
                            scriptfile_getdouble(pScript, &dtemp);
                            scriptfile_getdouble(pScript, &dtemp2);
                            animPtr->frameaspect1 = dtemp;
                            animPtr->frameaspect2 = dtemp2;
                            break;
                        }
                        case T_SOUND:
                        {
                            char *animSoundsEnd = NULL;
                            if (scriptfile_getbraces(pScript, &animSoundsEnd))
                                break;
                            parsedefinitions_game_animsounds(pScript, animSoundsEnd, fileName, animPtr);
                            break;
                        }
                        case T_FORCEFILTER:
                            animPtr->frameflags |= CUTSCENE_FORCEFILTER;
                            break;
                        case T_FORCENOFILTER:
                            animPtr->frameflags |= CUTSCENE_FORCENOFILTER;
                            break;
                        case T_TEXTUREFILTER:
                            animPtr->frameflags |= CUTSCENE_TEXTUREFILTER;
                            break;
                    }
                }
            }
            else
                pScript->textptr = animEnd;
        }
        break;
        case T_ANIMSOUNDS:
        {
            char *tokenPtr     = pScript->ltextptr;
            char *fileName     = NULL;

            scriptfile_getstring(pScript, &fileName);
            if (!fileName)
                break;

            char *animSoundsEnd = NULL;

            if (scriptfile_getbraces(pScript, &animSoundsEnd))
                break;

            if (firstPass)
            {
                pScript->textptr = animSoundsEnd;
                break;
            }

            dukeanim_t *animPtr = Anim_Find(fileName);

            if (!animPtr)
            {
                initprintf("Error: expected animation filename on line %s:%d\n",
                    pScript->filename, scriptfile_getlinum(pScript, tokenPtr));
                break;
            }

            parsedefinitions_game_animsounds(pScript, animSoundsEnd, fileName, animPtr);
        }
        break;

        case T_SOUND:
        {
            char *  tokenPtr = pScript->ltextptr;
            char *  fileName = NULL;
            int32_t soundNum = -1;
            char *  musicEnd;

            if (scriptfile_getbraces(pScript, &musicEnd))
                break;

            while (pScript->textptr < musicEnd)
            {
                switch (getatoken(pScript, soundTokens, ARRAY_SIZE(soundTokens)))
                {
                    case T_ID: scriptfile_getsymbol(pScript, &soundNum); break;
                    case T_FILE: scriptfile_getstring(pScript, &fileName); break;
                }
            }

            if (!firstPass)
            {
                if (soundNum==-1)
                {
                    initprintf("Error: missing ID for sound definition near line %s:%d\n", pScript->filename, scriptfile_getlinum(pScript,tokenPtr));
                    break;
                }

                if (fileName == NULL || check_file_exist(fileName))
                    break;

                if (S_DefineSound(soundNum,fileName) == -1)
                    initprintf("Error: invalid sound ID on line %s:%d\n", pScript->filename, scriptfile_getlinum(pScript,tokenPtr));
            }
        }
        break;
        case T_GLOBALGAMEFLAGS: scriptfile_getnumber(pScript, &duke3d_globalflags); break;
        case T_EOF: return 0;
        default: break;
        }
    }
    while (1);

    return 0;
}

int loaddefinitions_game(const char *fileName, int32_t firstPass)
{
    scriptfile *pScript = scriptfile_fromfile(fileName);

    if (pScript)
        parsedefinitions_game(pScript, firstPass);

    for (bssize_t i=0; i < g_defModulesNum; ++i)
        parsedefinitions_game_include(g_defModules[i], NULL, "null", firstPass);

    if (pScript)
        scriptfile_close(pScript);

    scriptfile_clearsymbols();

    return 0;
}



void G_UpdateAppTitle(void)
{
    if (g_gameNamePtr)
    {
#ifdef EDUKE32_STANDALONE
        Bstrcpy(tempbuf, g_gameNamePtr);
#else
        Bsprintf(tempbuf, "%s - " APPNAME, g_gameNamePtr);
#endif
        wm_setapptitle(tempbuf);
    }
    else
    {
        wm_setapptitle(APPNAME);
    }
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
        Bfree(g_mapInfo[i].name);
        Bfree(g_mapInfo[i].filename);
        Bfree(g_mapInfo[i].musicfn);

        G_FreeMapState(i);
    }

    for (i=MAXQUOTES-1; i>=0; i--)
    {
        Bfree(apStrings[i]);
        Bfree(apXStrings[i]);
    }

    for (i=MAXPLAYERS-1; i>=0; i--)
    {
        Bfree(g_player[i].ps);
        Bfree(g_player[i].inputBits);
    }

    for (i=MAXSOUNDS-1; i>=0; i--)
    {
        Bfree(g_sounds[i].filename);
    }
#if !defined LUNATIC
    if (label != (char *)&sprite[0]) Bfree(label);
    if (labelcode != (int32_t *)&sector[0]) Bfree(labelcode);
    Bfree(apScript);
    Bfree(bitptr);

//    Bfree(MusicPtr);

    Gv_Clear();

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

    if ((uint32_t)g_labelCnt > MAXSPRITES*sizeof(spritetype)/64)   // see the arithmetic above for why
        G_GameExit("Error: too many labels defined!");

    {
        char *newlabel;
        int32_t *newlabelcode;

        newlabel     = (char *)Xmalloc(g_labelCnt<<6);
        newlabelcode = (int32_t *)Xmalloc(g_labelCnt*sizeof(int32_t));

        Bmemcpy(newlabel, label, g_labelCnt*64);
        Bmemcpy(newlabelcode, labelcode, g_labelCnt*sizeof(int32_t));

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
    ud.m_coop = clamp(ud.m_coop, 0, g_gametypeCnt-1);
    initprintf("%s\n",g_gametypeNames[ud.m_coop]);
    if (g_gametypeFlags[ud.m_coop] & GAMETYPE_ITEMRESPAWN)
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
    int DukeEnemies[] = {
        SHARK, RECON, DRONE,
        LIZTROOPONTOILET, LIZTROOPJUSTSIT, LIZTROOPSTAYPUT, LIZTROOPSHOOT,
        LIZTROOPJETPACK, LIZTROOPSHOOT, LIZTROOPDUCKING, LIZTROOPRUNNING, LIZTROOP,
        OCTABRAIN, COMMANDER, COMMANDERSTAYPUT, PIGCOP, PIGCOPSTAYPUT, PIGCOPDIVE, EGG,
        LIZMAN, LIZMANSPITTING, LIZMANJUMP, ORGANTIC,
        BOSS1, BOSS2, BOSS3, BOSS4, RAT, ROTATEGUN };

    int SolidEnemies[] = { TANK, BOSS1, BOSS2, BOSS3, BOSS4, RECON, ROTATEGUN };
    int NoWaterDipEnemies[] = { OCTABRAIN, COMMANDER, DRONE };
    int GreenSlimeFoodEnemies[] = { LIZTROOP, LIZMAN, PIGCOP, NEWBEAST };

    for (bssize_t i=GREENSLIME; i<=GREENSLIME+7; i++)
        SETFLAG(i, SFLAG_HARDCODED_BADGUY);

    for (bssize_t i=ARRAY_SIZE(DukeEnemies)-1; i>=0; i--)
        SETFLAG(DukeEnemies[i], SFLAG_HARDCODED_BADGUY);

    for (bssize_t i=ARRAY_SIZE(SolidEnemies)-1; i>=0; i--)
        SETFLAG(SolidEnemies[i], SFLAG_NODAMAGEPUSH);

    for (bssize_t i=ARRAY_SIZE(NoWaterDipEnemies)-1; i>=0; i--)
        SETFLAG(NoWaterDipEnemies[i], SFLAG_NOWATERDIP);

    for (bssize_t i=ARRAY_SIZE(GreenSlimeFoodEnemies)-1; i>=0; i--)
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
        extern const char luaJIT_BC__defs_game[];

        if ((i = L_RunString(&g_ElState, luaJIT_BC__defs_game,
                             LUNATIC_DEFS_BC_SIZE, "_defs_game.lua")))
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

static void G_HandleMemErr(int32_t lineNum, const char *fileName, const char *funcName)
{
    static char msg[128];
    Bsnprintf(msg, sizeof(msg), "Out of memory in %s:%d (%s)\n", fileName, lineNum, funcName);
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
    // not in _defs_game.lua.
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

#if 0//def _WIN32
        if (ud.executions >= 50 && !DUKEBETA)
        {
            initprintf("IT IS NOW TIME TO UPGRADE TO THE COMPLETE VERSION!\n");

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
        }
#endif
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
    Menu_Open(myconnectindex);
    Menu_Change(MENU_MAIN);
    KB_FlushKeyboardQueue();
    G_UpdateAppTitle();
}

static int G_EndOfLevel(void)
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
#ifndef EDUKE32_STANDALONE
                if (!VOLUMEALL)
                    G_DoOrderScreen();
#endif
                g_player[myconnectindex].ps->gm = 0;
                Menu_Open(myconnectindex);
                Menu_Change(MENU_MAIN);
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
#if !defined LUNATIC
    VM_ScriptInfo(insptr, 64);
#endif
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
    if (g_player[pnum].inputBits == NULL)
        g_player[pnum].inputBits = (input_t *)Xcalloc(1, sizeof(input_t));

#ifdef LUNATIC
    g_player_ps[pnum] = g_player[pnum].ps;
    g_player[pnum].ps->wa.idx = pnum;
#endif
}


int G_FPSLimit(void)
{
    static uint64_t nextPageTicks = 0;
    static unsigned frameWaiting  = 0;

    if (frameWaiting)
    {
        frameWaiting--;
        nextpage();
    }

    uint64_t const frameTicks = getu64ticks();

    if (!r_maxfps || frameTicks >= nextPageTicks)
    {
        if (frameTicks >= nextPageTicks + g_frameDelay)
            nextPageTicks = frameTicks;

        nextPageTicks += g_frameDelay;
        frameWaiting++;
    }

    return frameWaiting;
}

// TODO: reorder (net)actor_t to eliminate slop and update assertion
EDUKE32_STATIC_ASSERT(sizeof(actor_t)%4 == 0);
EDUKE32_STATIC_ASSERT(sizeof(DukePlayer_t)%4 == 0);

int app_main(int argc, char const * const * argv)
{
#ifndef NETCODE_DISABLE
    if (enet_initialize() != 0)
        initprintf("An error occurred while initializing ENet.\n");
    else atexit(enet_deinitialize);
#endif

#ifdef _WIN32
    if (!G_CheckCmdSwitch(argc, argv, "-noinstancechecking") && win_checkinstance())
    {
        if (!wm_ynbox(APPNAME, "Another Build game is currently running. "
                      "Do you wish to continue starting this copy?"))
            return 3;
    }

    backgroundidle = 0;

#ifdef DEBUGGINGAIDS
    extern int32_t (*check_filename_casing_fn)(void);
    check_filename_casing_fn = check_filename_casing;
#endif
#endif

    G_ExtPreInit(argc, argv);

#ifdef __APPLE__
    if (!g_useCwd)
    {
        char cwd[BMAX_PATH];
        char *homedir = Bgethomedir();
        if (homedir)
            Bsnprintf(cwd, sizeof(cwd), "%s/Library/Logs/" APPBASENAME ".log", homedir);
        else
            Bstrcpy(cwd, APPBASENAME ".log");
        OSD_SetLogFile(cwd);
        Bfree(homedir);
    }
    else
#endif
    OSD_SetLogFile(APPBASENAME ".log");

    OSD_SetFunctions(GAME_drawosdchar,
                     GAME_drawosdstr,
                     GAME_drawosdcursor,
                     GAME_getcolumnwidth,
                     GAME_getrowheight,
                     GAME_clearbackground,
                     BGetTime,
                     GAME_onshowosd);

    wm_setapptitle(APPNAME);

    initprintf(HEAD2 " %s\n", s_buildRev);
    PrintBuildInfo();

    if (!g_useCwd)
        G_AddSearchPaths();

    g_skillCnt = 4;
    ud.multimode = 1;

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
    for (bssize_t i=NUMGAMEFUNCTIONS-1; i>=0; i--)
    {
        if (gamefunctions[i][0] == '\0')
            continue;

        char *str = Bstrtolower(Xstrdup(gamefunctions[i]));
        hash_add(&h_gamefuncs,gamefunctions[i],i,0);
        hash_add(&h_gamefuncs,str,i,0);
        Bfree(str);
    }

#ifdef STARTUP_SETUP_WINDOW
    int const readSetup =
#endif
    CONFIG_ReadSetup();

#if defined(_WIN32) && !defined (EDUKE32_STANDALONE)

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
                        char const *      p = "http://www.eduke32.com";

                        Bmemset(&sinfo, 0, sizeof(sinfo));
                        sinfo.cbSize  = sizeof(sinfo);
                        sinfo.fMask   = SEE_MASK_CLASSNAME;
                        sinfo.lpVerb  = "open";
                        sinfo.lpFile  = p;
                        sinfo.nShow   = SW_SHOWNORMAL;
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

    if (preinitengine())
    {
        wm_msgbox("Build Engine Initialization Error",
                  "There was a problem initializing the Build engine: %s", engineerrstr);
        ERRprintf("app_main: There was a problem initializing the Build engine: %s\n", engineerrstr);
        Bexit(2);
    }

    if (Bstrcmp(g_setupFileName, SETUPFILENAME))
        initprintf("Using config file \"%s\".\n",g_setupFileName);

    G_ScanGroups();

#ifdef STARTUP_SETUP_WINDOW
    if (readSetup < 0 || (!g_noSetup && (ud.configversion != BYTEVERSION_EDUKE32 || ud.config.ForceSetup)) || g_commandSetup)
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

    if (!g_useCwd)
        G_CleanupSearchPaths();

#ifndef EDUKE32_STANDALONE
    G_SetupCheats();

    if (SHAREWARE)
        g_Shareware = 1;
    else
    {
        int const kFile = kopen4load("DUKESW.BIN",1); // JBF 20030810

        if (kFile != -1)
        {
            g_Shareware = 1;
            kclose(kFile);
        }
    }
#endif

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
    g_mostConcurrentPlayers = ud.multimode;  // Lunatic needs this (player[] bound)

    if (!g_fakeMultiMode)
    {
        connectpoint2[0] = -1;
    }
    else
    {
        for (bssize_t i=0; i<ud.multimode-1; i++)
            connectpoint2[i] = i+1;
        connectpoint2[ud.multimode-1] = -1;

        for (bssize_t i=1; i<ud.multimode; i++)
            g_player[i].playerquitflag = 1;
    }

    Net_GetPackets();

    // NOTE: Allocating the DukePlayer_t structs has to be before compiling scripts,
    // because in Lunatic, the {pipe,trip}bomb* members are initialized.
    for (bssize_t i=0; i<MAXPLAYERS; i++)
        G_MaybeAllocPlayer(i);

    G_Startup(); // a bunch of stuff including compiling cons

    g_player[0].playerquitflag = 1;

    g_player[myconnectindex].ps->palette = BASEPAL;

    for (int i=1, j=numplayers; j<ud.multimode; j++)
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
    }
    loaddefinitions_game(defsfile, FALSE);

    for (bssize_t i = 0; i < g_defModulesNum; ++i) Bfree(g_defModules[i]);

    DO_FREE_AND_NULL(g_defModules);
    g_defModulesNum = 0;

    if (E_PostInit())
        G_FatalEngineError();

    G_PostLoadPalette();

    E_UndefineTile(MIRROR);

    Gv_ResetSystemDefaults(); // called here to populate our fake tilesizx and tilesizy arrays presented to CON with sizes generated by dummytiles

    if (numplayers == 1 && boardfilename[0] != 0)
    {
        ud.m_level_number  = 7;
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

    g_mostConcurrentPlayers = ud.multimode;  // XXX: redundant?
    ud.last_level      = 0;

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

    CONFIG_WriteSetup(1);
    CONFIG_ReadSetup();

    RTS_Init(ud.rtsname);

    if (RTS_IsInitialized())
        initprintf("Using RTS file \"%s\".\n", ud.rtsname);

    if (ud.last_level)
        Bstrcpy(ud.rtsname, defaultrtsfilename[0]);

    ud.last_level = -1;

    initprintf("Initializing OSD...\n");

    Bsprintf(tempbuf, HEAD2 " %s", s_buildRev);
    OSD_SetVersion(tempbuf, 10,0);
    OSD_SetParameters(0, 0, 0, 12, 2, 12, OSD_ERROR, OSDTEXT_RED, gamefunctions[gamefunc_Show_Console][0] == '\0' ? OSD_PROTECTED : 0);
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
        CONTROL_MouseEnabled    = (ud.config.UseMouse && CONTROL_MousePresent);

        // JBF 20040215: evil and nasty place to do this, but joysticks are evil and nasty too
        for (bssize_t i=0; i<joynumaxes; i++)
            setjoydeadzone(i,ud.config.JoystickAnalogueDead[i],ud.config.JoystickAnalogueSaturate[i]);
    }

#ifdef HAVE_CLIPSHAPE_FEATURE
    int const clipMapError = clipmapinfo_load();
    if (clipMapError > 0)
        initprintf("There was an error loading the sprite clipping map (status %d).\n", clipMapError);

    for (bssize_t i=0; i < g_clipMapFilesNum; ++i)
        Bfree(g_clipMapFiles[i]);
    DO_FREE_AND_NULL(g_clipMapFiles);
    g_clipMapFilesNum = 0;
#endif

    // check if the minifont will support lowercase letters (3136-3161)
    // there is room for them in tiles012.art between "[\]^_." and "{|}~"
    minitext_lowercase = 1;
    for (bssize_t i = MINIFONT + ('a'-'!'); minitext_lowercase && i < MINIFONT + ('z'-'!') + 1; ++i)
        minitext_lowercase &= tile_exists(i);

    system_getcvars();

    char *const setupFileName = Xstrdup(g_setupFileName);
    char *const p             = strtok(setupFileName, ".");

    if (!Bstrcmp(g_setupFileName, SETUPFILENAME))
        Bsprintf(tempbuf, "settings.cfg");
    else
        Bsprintf(tempbuf, "%s_settings.cfg", p);

    Bfree(setupFileName);

    OSD_Exec(tempbuf);
    OSD_Exec("autoexec.cfg");

    if (g_networkMode != NET_DEDICATED_SERVER)
    {
        if (setgamemode(ud.config.ScreenMode,ud.config.ScreenWidth,ud.config.ScreenHeight,ud.config.ScreenBPP) < 0)
        {
            vec2_t const res[] = {
                { ud.config.ScreenWidth, ud.config.ScreenHeight }, { 800, 600 }, { 640, 480 }, { 320, 240 },
            };

#ifdef USE_OPENGL
            int const bpp[] = { 32, 16, 8 };
#else
            int const bpp[] = { 8 };
#endif

            initprintf("Failure setting video mode %dx%dx%d %s! Attempting safer mode...\n", ud.config.ScreenWidth, ud.config.ScreenHeight,
                       ud.config.ScreenBPP, ud.config.ScreenMode ? "fullscreen" : "windowed");

            int resIdx = 0;
            int bppIdx = 0;

            while (setgamemode(0, res[resIdx].x, res[resIdx].y, bpp[bppIdx]) < 0)
            {
                initprintf("Failure setting video mode %dx%dx%d windowed! Attempting safer mode...\n", res[resIdx].x, res[resIdx].y,
                           bpp[bppIdx]);

                if (++bppIdx == ARRAY_SIZE(bpp))
                {
                    if (++resIdx == ARRAY_SIZE(res))
                        G_GameExit("Unable to set failsafe video mode!");
                    bppIdx = 0;
                }
            }

            ud.config.ScreenWidth  = res[resIdx].x;
            ud.config.ScreenHeight = res[resIdx].y;
            ud.config.ScreenBPP    = bpp[bppIdx];
        }

        setbrightness(ud.brightness>>2,g_player[myconnectindex].ps->palette,0);

        S_MusicStartup();
        S_SoundStartup();
        Menu_Init();
    }

#if 0
    // previously, passing -0 through -9 on the command line would load the save in that slot #
    // this code should be reusable for a new parameter that takes a filename, if desired
    if (/* havesavename */ && (!g_netServer && ud.multimode < 2))
    {
        clearview(0L);
        //g_player[myconnectindex].ps->palette = palette;
        //G_FadePalette(0,0,0,0);
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308
        rotatesprite_fs(160<<16,100<<16,65536L,0,LOADSCREEN,0,0,2+8+64+BGSTRETCH);
        menutext_center(105,"Loading saved game...");
        nextpage();

        if (G_LoadPlayer(/* savefile */))
            /* havesavename = false; */
    }
#endif

    FX_StopAllSounds();
    S_ClearSoundLocks();

    //    getpackets();

MAIN_LOOP_RESTART:
    totalclock = 0;
    ototalclock = 0;
    lockclock = 0;

    g_player[myconnectindex].ps->fta = 0;

    Menu_Change(MENU_MAIN);

    if (g_networkMode != NET_DEDICATED_SERVER)
    {
        G_GetCrosshairColor();
        G_SetCrosshairColor(CrosshairColors.r, CrosshairColors.g, CrosshairColors.b);
    }

    if (ud.warp_on == 1)
    {
        G_NewGame_EnterLevel();
        // may change ud.warp_on in an error condition
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

            for (bssize_t TRAVERSE_CONNECT(i))
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
    else G_UpdateScreenArea();

//    G_GameExit(" "); ///

//    ud.auto_run = ud.config.RunMode;
    ud.showweapons = ud.config.ShowOpponentWeapons;
    P_SetupMiscInputSettings();
    g_player[myconnectindex].pteam = ud.team;

    if (g_gametypeFlags[ud.coop] & GAMETYPE_TDM)
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
        if (handleevents() && quitevent)
        {
            KB_KeyDown[sc_Escape] = 1;
            quitevent = 0;
        }

        sampletimer();
        Net_GetPackets();

        // only allow binds to function if the player is actually in a game (not in a menu, typing, et cetera) or demo
        CONTROL_BindsEnabled = !!(g_player[myconnectindex].ps->gm & (MODE_GAME|MODE_DEMO));

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

            Bmemcpy(&inputfifo[0][myconnectindex], &localInput, sizeof(input_t));

            do
            {
                sampletimer();

                if (ready2send == 0) break;

                ototalclock += TICSPERFRAME;

                int const moveClock = totalclock;

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

                if (totalclock - moveClock >= TICSPERFRAME)
                {
                    // computing a tic takes longer than a tic, so we're slowing
                    // the game down. rather than tightly spinning here, go draw
                    // a frame since we're fucked anyway
                    break;
                }
            }
            while (((g_netClient || g_netServer) || !(g_player[myconnectindex].ps->gm & (MODE_MENU|MODE_DEMO))) && totalclock >= ototalclock+TICSPERFRAME);
        }

        // handle CON_SAVE and CON_SAVENN
        if (g_saveRequested)
        {
            KB_FlushKeyboardQueue();

            g_screenCapture = 1;
            G_DrawRooms(myconnectindex, 65536);
            g_screenCapture = 0;

            G_SavePlayerMaybeMulti(g_lastautosave);
            g_quickload = &g_lastautosave;

            OSD_Printf("Saved: %s\n", g_lastautosave.path);

            g_saveRequested = false;
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

        if (G_FPSLimit())
        {
            int const smoothRatio
            = ((ud.show_help == 0 && (!g_netServer && ud.multimode < 2) && !(g_player[myconnectindex].ps->gm & MODE_MENU))
               || (g_netServer || ud.multimode > 1)
               || ud.recstat == 2)
              ? calc_smoothratio(totalclock, ototalclock)
              : 65536;

            G_DrawRooms(screenpeek, smoothRatio);
            if (getrendermode() >= REND_POLYMOST)
                G_DrawBackground();
            G_DisplayRest(smoothRatio);
        }

skipframe:
        if (g_player[myconnectindex].ps->gm&MODE_DEMO)
            goto MAIN_LOOP_RESTART;
    }
    while (1);

    return 0;  // not reached (duh)
}

GAME_STATIC GAME_INLINE int32_t G_MoveLoop()
{
    Net_GetPackets();

    return G_DoMoveThings();
}

int G_DoMoveThings(void)
{
    ud.camerasprite = -1;
    lockclock += TICSPERFRAME;

    // Moved lower so it is restored correctly by demo diffs:
    //if (g_earthquakeTime > 0) g_earthquakeTime--;

    if (g_RTSPlaying > 0)
        g_RTSPlaying--;

    for (bssize_t i=0; i<MAXUSERQUOTES; i++)
    {
        if (user_quote_time[i])
        {
            if (--user_quote_time[i] > ud.msgdisptime)
                user_quote_time[i] = ud.msgdisptime;
            if (!user_quote_time[i]) pub = NUMPAGES;
        }
    }

    // Name display when aiming at opponents
    if (ud.idplayers && (g_netServer || ud.multimode > 1)
#ifdef SPLITSCREEN_MOD_HACKS
        && !g_fakeMultiMode
#endif
        )
    {
        hitdata_t hitData;
        DukePlayer_t *const pPlayer = g_player[screenpeek].ps;

        for (bssize_t TRAVERSE_CONNECT(i))
            if (g_player[i].ps->holoduke_on != -1)
                sprite[g_player[i].ps->holoduke_on].cstat ^= 256;

        hitscan((vec3_t *)pPlayer, pPlayer->cursectnum, sintable[(pPlayer->ang + 512) & 2047], sintable[pPlayer->ang & 2047],
                (100 - pPlayer->horiz - pPlayer->horizoff) << 11, &hitData, 0xffff0030);

        for (bssize_t TRAVERSE_CONNECT(i))
            if (g_player[i].ps->holoduke_on != -1)
                sprite[g_player[i].ps->holoduke_on].cstat ^= 256;

        if ((hitData.sprite >= 0) && !(g_player[myconnectindex].ps->gm & MODE_MENU) &&
                sprite[hitData.sprite].picnum == APLAYER)
        {
            int const playerNum = P_Get(hitData.sprite);

            if (playerNum != screenpeek && g_player[playerNum].ps->dead_flag == 0)
            {
                if (pPlayer->fta == 0 || pPlayer->ftq == QUOTE_RESERVED3)
                {
                    if (ldist(&sprite[pPlayer->i], &sprite[hitData.sprite]) < 9216)
                    {
                        Bsprintf(apStrings[QUOTE_RESERVED3], "%s", &g_player[playerNum].user_name[0]);
                        pPlayer->fta = 12, pPlayer->ftq = QUOTE_RESERVED3;
                    }
                }
                else if (pPlayer->fta > 2) pPlayer->fta -= 3;
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

    if (g_netClient) // [75] The server should not overwrite its own randomseed
        randomseed = ticrandomseed;

    for (bssize_t TRAVERSE_CONNECT(i))
        Bmemcpy(g_player[i].inputBits, &inputfifo[(g_netServer && myconnectindex == i)][i], sizeof(input_t));

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

    for (bssize_t TRAVERSE_CONNECT(i))
    {
        if (g_player[i].inputBits->extbits&(1<<6))
        {
            g_player[i].ps->team = g_player[i].pteam;
            if (g_gametypeFlags[ud.coop] & GAMETYPE_TDM)
            {
                actor[g_player[i].ps->i].picnum = APLAYERTOP;
                P_QuickKill(g_player[i].ps);
            }
        }
        if (g_gametypeFlags[ud.coop] & GAMETYPE_TDM)
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

void A_SpawnWallGlass(int spriteNum, int wallNum, int glassCnt)
{
    if (wallNum < 0)
    {
        for (bssize_t j = glassCnt - 1; j >= 0; --j)
        {
            int const a = SA(spriteNum) - 256 + (krand() & 511) + 1024;
            A_InsertSprite(SECT(spriteNum), SX(spriteNum), SY(spriteNum), SZ(spriteNum), GLASSPIECES + (j % 3), -32, 36, 36, a,
                           32 + (krand() & 63), 1024 - (krand() & 1023), spriteNum, 5);
        }
        return;
    }

    vec2_t v1 = { wall[wallNum].x, wall[wallNum].y };
    vec2_t v  = { wall[wall[wallNum].point2].x - v1.x, wall[wall[wallNum].point2].y - v1.y };

    v1.x -= ksgn(v.y);
    v1.y += ksgn(v.x);

    v.x = tabledivide32_noinline(v.x, glassCnt+1);
    v.y = tabledivide32_noinline(v.y, glassCnt+1);

    int16_t sect = -1;

    for (bsize_t j = glassCnt; j > 0; --j)
    {
        v1.x += v.x;
        v1.y += v.y;

        updatesector(v1.x,v1.y,&sect);
        if (sect >= 0)
        {
            int z = sector[sect].floorz - (krand() & (klabs(sector[sect].ceilingz - sector[sect].floorz)));

            if (z < -ZOFFSET5 || z > ZOFFSET5)
                z = SZ(spriteNum) - ZOFFSET5 + (krand() & ((64 << 8) - 1));

            A_InsertSprite(SECT(spriteNum), v1.x, v1.y, z, GLASSPIECES + (j % 3), -32, 36, 36, SA(spriteNum) - 1024, 32 + (krand() & 63),
                           -(krand() & 1023), spriteNum, 5);
        }
    }
}

void A_SpawnGlass(int spriteNum, int glassCnt)
{
    for (; glassCnt>0; glassCnt--)
    {
        int const k
        = A_InsertSprite(SECT(spriteNum), SX(spriteNum), SY(spriteNum), SZ(spriteNum) - ((krand() & 16) << 8), GLASSPIECES + (glassCnt % 3),
                         krand() & 15, 36, 36, krand() & 2047, 32 + (krand() & 63), -512 - (krand() & 2047), spriteNum, 5);
        sprite[k].pal = sprite[spriteNum].pal;
    }
}

void A_SpawnCeilingGlass(int spriteNum, int sectNum, int glassCnt)
{
    int const startWall = sector[sectNum].wallptr;
    int const endWall = startWall+sector[sectNum].wallnum;

    for (bssize_t wallNum = startWall; wallNum < (endWall - 1); wallNum++)
    {
        vec2_t v1 = { wall[wallNum].x, wall[wallNum].y };
        vec2_t v  = { tabledivide32_noinline(wall[wallNum + 1].x - v1.x, glassCnt + 1),
                     tabledivide32_noinline(wall[wallNum + 1].y - v1.y, glassCnt + 1) };

        for (bsize_t j = glassCnt; j > 0; j--)
        {
            v1.x += v.x;
            v1.y += v.y;
            A_InsertSprite(sectNum, v1.x, v1.y, sector[sectNum].ceilingz + ((krand() & 15) << 8), GLASSPIECES + (j % 3), -32, 36, 36,
                           krand() & 2047, (krand() & 31), 0, spriteNum, 5);
        }
    }
}

void A_SpawnRandomGlass(int spriteNum, int wallNum, int glassCnt)
{
    if (wallNum < 0)
    {
        for (bssize_t j = glassCnt - 1; j >= 0; j--)
        {
            int const k
            = A_InsertSprite(SECT(spriteNum), SX(spriteNum), SY(spriteNum), SZ(spriteNum) - (krand() & (63 << 8)), GLASSPIECES + (j % 3),
                             -32, 36, 36, krand() & 2047, 32 + (krand() & 63), 1024 - (krand() & 2047), spriteNum, 5);
            sprite[k].pal = krand() & 15;
        }
        return;
    }

    vec2_t v1 = { wall[wallNum].x, wall[wallNum].y };
    vec2_t v  = { tabledivide32_noinline(wall[wall[wallNum].point2].x - wall[wallNum].x, glassCnt + 1),
                 tabledivide32_noinline(wall[wall[wallNum].point2].y - wall[wallNum].y, glassCnt + 1) };
    int16_t sectNum = sprite[spriteNum].sectnum;

    for (bsize_t j = glassCnt; j > 0; j--)
    {
        v1.x += v.x;
        v1.y += v.y;

        updatesector(v1.x, v1.y, &sectNum);

        int z = sector[sectNum].floorz - (krand() & (klabs(sector[sectNum].ceilingz - sector[sectNum].floorz)));

        if (z < -ZOFFSET5 || z > ZOFFSET5)
            z       = SZ(spriteNum) - ZOFFSET5 + (krand() & ((64 << 8) - 1));

        int const k = A_InsertSprite(SECT(spriteNum), v1.x, v1.y, z, GLASSPIECES + (j % 3), -32, 36, 36, SA(spriteNum) - 1024,
                                     32 + (krand() & 63), -(krand() & 2047), spriteNum, 5);
        sprite[k].pal = krand() & 7;
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
