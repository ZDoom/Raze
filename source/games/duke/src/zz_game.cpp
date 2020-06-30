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
#include "osdcmds.h"
#include "net.h"
#include "menus.h"
#include "savegame.h"
#include "demo.h"

#include "cheats.h"
#include "sbar.h"
#include "screens.h"
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

# define GAME_INLINE inline
# define GAME_STATIC static

BEGIN_DUKE_NS

void SetDispatcher();
void checkcommandline();

int16_t max_ammo_amount[MAX_WEAPONS];

uint8_t shadedsector[MAXSECTORS];

int32_t g_fakeMultiMode = 0;
int32_t g_quitDeadline = 0;

int32_t g_cameraDistance = 0, g_cameraClock = 0;
static int32_t g_quickExit;

char boardfilename[BMAX_PATH] = {0};

int32_t voting = -1;
int32_t vote_map = -1, vote_episode = -1;

int32_t g_Debug = 0;

int32_t g_Shareware = 0;

int32_t tempwallptr;

static int32_t nonsharedtimer;

int32_t ticrandomseed;

GAME_STATIC GAME_INLINE int32_t G_MoveLoop(void);

int32_t g_levelTextTime = 0;

#if defined(RENDERTYPEWIN) && defined(USE_OPENGL)
extern char forcegl;
#endif

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
    T_MINPITCH,
    T_MAXPITCH,
    T_PRIORITY,
    T_TYPE,
    T_DISTANCE,
    T_VOLUME,
    T_DELAY,
    T_RENAMEFILE,
    T_GLOBALGAMEFLAGS,
    T_ASPECT,
    T_FORCEFILTER,
    T_FORCENOFILTER,
    T_TEXTUREFILTER,
};

static void gameTimerHandler(void)
{
    S_Update();
    G_HandleSpecialKeys();
}

void G_HandleSpecialKeys(void)
{
    // we need CONTROL_GetInput in order to pick up joystick button presses
    if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
    {
        ControlInfo noshareinfo;
        CONTROL_GetInput(&noshareinfo);
    }

    // only dispatch commands here when not in a game
    if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
        OSD_DispatchQueued();

    if (g_quickExit == 0 && inputState.GetKeyStatus(sc_LeftControl) && inputState.GetKeyStatus(sc_LeftAlt) && (inputState.GetKeyStatus(sc_Delete)||inputState.GetKeyStatus(sc_End)))
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
        g_quitDeadline = (int32_t) totalclock+120;
        //g_netDisconnect = 1;
    }

    if ((totalclock > g_quitDeadline) && (g_gameQuit == 1))
        g_netDisconnect = 1;
        //G_GameExit("Timed out.");
}




void G_GameExit(const char *msg)
{
    if (*msg != 0) g_player[myconnectindex].ps->palette = BASEPAL;

    // JBF: fixes crash on demo playback
    // PK: modified from original

    if (!g_quickExit)
    {
        if (g_mostConcurrentPlayers > 1 && g_player[myconnectindex].ps->gm&MODE_GAME && GTFLAGS(GAMETYPE_SCORESHEET) && *msg == ' ')
        {
            G_BonusScreen(1);
        }

        // shareware and TEN screens
        if (*msg != 0 && *(msg+1) != 'V' && *(msg+1) != 'Y' && !VOLUMEALL && !RR)
            showtwoscreens([](bool) {});
    }

	if (*msg != 0)
	{
		if (!(msg[0] == ' ' && msg[1] == 0))
		{
			I_Error("%s", msg);
		}
	}
    endoomName = RR? "redneck.bin" : VOLUMEALL ? "duke3d.bin" : "dukesw.bin";
    ST_Endoom();
}



static int32_t G_DoThirdPerson(const DukePlayer_t *pp, vec3_t *vect, int16_t *vsectnum, int16_t ang, int16_t horiz)
{
    spritetype *sp = &sprite[pp->i];
    int32_t i, hx, hy;
    int32_t bakcstat = sp->cstat;
    hitdata_t hit;

    vec3_t n = {
        sintable[(ang+1536)&2047]>>4,
        sintable[(ang+1024)&2047]>>4,
        (horiz-100) * 128
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
            int32_t daang = getangle(wall[wall[hit.wall].point2].x-wall[hit.wall].x,
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

    CAMERADIST = min(CAMERADIST+(((int32_t) totalclock-CAMERACLOCK)<<10),65536);
    CAMERACLOCK = (int32_t) totalclock;

    updatesectorz(vect->x,vect->y,vect->z,vsectnum);

    return 0;
}

int32_t SE150_TempSectorZ[MAXSECTORS];
int32_t SE150_TempSectorPicnum[MAXSECTORS];

static void G_SE150_Draw(int32_t spnum, int32_t x, int32_t y, int32_t z, int32_t a, int32_t h, int32_t smoothratio)
{
    int32_t i = 13, j, k = 0;
    int32_t floor1 = spnum, floor2 = 0, ok = 0, fofmode;
    int32_t offx, offy;

    if (sprite[spnum].ang != 512) return;

    tileDelete(13);
    if (!(gotpic[i >> 3] & (1 << (i & 7)))) return;
    gotpic[i >> 3] &= ~(1 << (i & 7));

    floor1 = spnum;

    if (sprite[spnum].lotag == 152) fofmode = 150;
    if (sprite[spnum].lotag == 153) fofmode = 151;
    if (sprite[spnum].lotag == 154) fofmode = 150;
    if (sprite[spnum].lotag == 155) fofmode = 151;

    ok++;

    for (j = 0; j < MAXSPRITES; j++)
    {
        if (
            sprite[j].picnum == 1 &&
            sprite[j].lotag == fofmode &&
            sprite[j].hitag == sprite[floor1].hitag
            ) {
            floor1 = j; fofmode = sprite[j].lotag; ok++; break;
        }
    }
    // if(ok==1) { Message("no floor1",RED); return; }

    if (fofmode == 150) k = 151; else k = 150;

    for (j = 0; j < MAXSPRITES; j++)
    {
        if (
            sprite[j].picnum == 1 &&
            sprite[j].lotag == k &&
            sprite[j].hitag == sprite[floor1].hitag
            ) {
            floor2 = j; ok++; break;
        }
    }

    // if(ok==2) { Message("no floor2",RED); return; }

    for (j = 0; j < MAXSPRITES; j++)  // raise ceiling or floor
    {
        if (sprite[j].picnum == 1 &&
            sprite[j].lotag == k + 2 &&
            sprite[j].hitag == sprite[floor1].hitag
            )
        {
            if (k == 150)
            {
                SE150_TempSectorZ[sprite[j].sectnum] = sector[sprite[j].sectnum].floorz;
                sector[sprite[j].sectnum].floorz += (((z - sector[sprite[j].sectnum].floorz) / 32768) + 1) * 32768;
                SE150_TempSectorPicnum[sprite[j].sectnum] = sector[sprite[j].sectnum].floorpicnum;
                sector[sprite[j].sectnum].floorpicnum = 13;
            }
            else if (k == 151)
            {
                SE150_TempSectorZ[sprite[j].sectnum] = sector[sprite[j].sectnum].ceilingz;
                sector[sprite[j].sectnum].ceilingz += (((z - sector[sprite[j].sectnum].ceilingz) / 32768) - 1) * 32768;
                SE150_TempSectorPicnum[sprite[j].sectnum] = sector[sprite[j].sectnum].ceilingpicnum;
                sector[sprite[j].sectnum].ceilingpicnum = 13;
            }
        }
    }

    i = floor1;
    offx = x - sprite[i].x;
    offy = y - sprite[i].y;
    i = floor2;
#ifdef POLYMER
    if (videoGetRenderMode() == REND_POLYMER)
        polymer_setanimatesprites(G_DoSpriteAnimations, offx + sprite[i].x, offy + sprite[i].y, z, fix16_to_int(a), smoothratio);
#endif

    renderDrawRoomsQ16(offx + sprite[i].x, offy + sprite[i].y, z, a, h, sprite[i].sectnum);
    G_DoSpriteAnimations(offx + sprite[i].x, offy + sprite[i].y, z, fix16_to_int(a), smoothratio);
    renderDrawMasks();

    for (j = 0; j < MAXSPRITES; j++)  // restore ceiling or floor
    {
        if (sprite[j].picnum == 1 &&
            sprite[j].lotag == k + 2 &&
            sprite[j].hitag == sprite[floor1].hitag
            )
        {
            if (k == 150)
            {
                sector[sprite[j].sectnum].floorz = SE150_TempSectorZ[sprite[j].sectnum];
                sector[sprite[j].sectnum].floorpicnum = SE150_TempSectorPicnum[sprite[j].sectnum];
            }
            else if (k == 151)
            {
                sector[sprite[j].sectnum].ceilingz = SE150_TempSectorZ[sprite[j].sectnum];
                sector[sprite[j].sectnum].ceilingpicnum = SE150_TempSectorPicnum[sprite[j].sectnum];
            }
        }
    }
}
void G_SE150(int32_t x, int32_t y, int32_t z, int32_t a, int32_t h, int32_t smoothratio)
{
    for (int i = headspritestat[STAT_RAROR]; i >= 0; i = nextspritestat[i])
    {
        switch(sprite[i].lotag)
        {
//            case 40:
//            case 41:
//                SE40_Draw(i,x,y,a,smoothratio);
//                break;
            case 152:
            case 153:
            case 154:
            case 155:
                if(g_player[screenpeek].ps->cursectnum == sprite[i].sectnum)
                    G_SE150_Draw(i,x,y,z,a,h,smoothratio);
                break;
        }
    }
}

#ifdef LEGACY_ROR
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

        if (sprite[k].picnum != TILE_SECTOREFFECTOR && sprite[k].z >= sp->z)
        {
            tspriteptr_t tsp = renderAddTSpriteFromSprite(k);

            tsp->x += (refsp->x - sp->x);
            tsp->y += (refsp->y - sp->y);
            tsp->z += -sp->z + actor[sp->yvel].ceilingz;
            tsp->sectnum = refsp->sectnum;

//            Printf("duped sprite of pic %d at %d %d %d\n",tsp->picnum,tsp->x,tsp->y,tsp->z);
        }
    }
}

static int16_t SE40backupStat[MAXSECTORS];
static int32_t SE40backupZ[MAXSECTORS];

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
            int32_t i;
            int32_t pix_diff, newz;
            //                Printf("drawing ror\n");

            if (level)
            {
                // renderz = sector[sprite[sprite2].sectnum].ceilingz;
                renderz = sprite[sprite2].z - (sprite[sprite2].yrepeat * tilesiz[sprite[sprite2].picnum].y<<1);
                picnum = sector[sprite[sprite2].sectnum].ceilingpicnum;
                sector[sprite[sprite2].sectnum].ceilingpicnum = 562;
				tileDelete(562);

                pix_diff = klabs(z) >> 8;
                newz = - ((pix_diff / 128) + 1) * (128<<8);

                for (i = 0; i < numsectors; i++)
                {
                    SE40backupStat[i] = sector[i].ceilingstat;
                    SE40backupZ[i] = sector[i].ceilingz;
                    if (sp->lotag == 41)
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
				tileDelete(562);

                pix_diff = klabs(z) >> 8;
                newz = ((pix_diff / 128) + 1) * (128<<8);

                for (i = 0; i < numsectors; i++)
                {
                    SE40backupStat[i] = sector[i].floorstat;
                    SE40backupZ[i] = sector[i].floorz;
                    if (sp->lotag == 41)
                    {
                        sector[i].floorstat = 1;
                        sector[i].floorz = +newz;
                    }
                }
            }

#ifdef POLYMER
            if (videoGetRenderMode() == REND_POLYMER)
                polymer_setanimatesprites(G_DoSpriteAnimations, CAMERA(pos.x), CAMERA(pos.y), CAMERA(pos.z), fix16_to_int(CAMERA(q16ang)), smoothratio);
#endif
            renderDrawRoomsQ16(sprite[sprite2].x + x, sprite[sprite2].y + y,
                      z + renderz, CAMERA(q16ang), CAMERA(q16horiz), sect);
            drawing_ror = 1 + level;

            if (drawing_ror == 2) // viewing from top
                G_OROR_DupeSprites(sp);

            G_DoSpriteAnimations(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),fix16_to_int(CAMERA(q16ang)),smoothratio);
            renderDrawMasks();

            if (level)
            {
                sector[sprite[sprite2].sectnum].ceilingpicnum = picnum;
                for (i = 0; i < numsectors; i++)
                {
                    sector[i].ceilingstat = SE40backupStat[i];
                    sector[i].ceilingz = SE40backupZ[i];
                }
            }
            else
            {
                sector[sprite[sprite2].sectnum].floorpicnum = picnum;

                for (i = 0; i < numsectors; i++)
                {
                    sector[i].floorstat = SE40backupStat[i];
                    sector[i].floorz = SE40backupZ[i];
                }
            }
        }
    }
}
#endif

void G_HandleMirror(int32_t x, int32_t y, int32_t z, fix16_t a, fix16_t q16horiz, int32_t smoothratio)
{
    if ((gotpic[TILE_MIRROR>>3]&(1<<(TILE_MIRROR&7)))
#ifdef POLYMER
        && (videoGetRenderMode() != REND_POLYMER)
#endif
        )
    {
        if (mirrorcnt == 0)
        {
            // NOTE: We can have mirrorcnt==0 but gotpic'd TILE_MIRROR,
            // for example in LNGA2.
            gotpic[TILE_MIRROR>>3] &= ~(1<<(TILE_MIRROR&7));
            return;
        }

        int32_t i = 0, dst = INT32_MAX;

        for (bssize_t k=mirrorcnt-1; k>=0; k--)
        {
            if (!wallvisible(x, y, mirrorwall[k]))
                continue;

            const int32_t j =
                klabs(wall[mirrorwall[k]].x - x) +
                klabs(wall[mirrorwall[k]].y - y);

            if (j < dst)
                dst = j, i = k;
        }

        if (wall[mirrorwall[i]].overpicnum != TILE_MIRROR)
        {
            // Try to find a new mirror wall in case the original one was broken.

            int32_t startwall = sector[mirrorsector[i]].wallptr;
            int32_t endwall = startwall + sector[mirrorsector[i]].wallnum;

            for (bssize_t k=startwall; k<endwall; k++)
            {
                int32_t j = wall[k].nextwall;
                if (j >= 0 && (wall[j].cstat&32) && wall[j].overpicnum==TILE_MIRROR)  // cmp. premap.c
                {
                    mirrorwall[i] = j;
                    break;
                }
            }
        }

        if (wall[mirrorwall[i]].overpicnum == TILE_MIRROR)
        {
            int32_t tposx, tposy;
            fix16_t tang;

            renderPrepareMirror(x, y, z, a, q16horiz, mirrorwall[i], &tposx, &tposy, &tang);

            int32_t j = g_visibility;
            g_visibility = (j>>1) + (j>>2);

            renderDrawRoomsQ16(tposx,tposy,z,tang,q16horiz,mirrorsector[i]+MAXSECTORS);
            display_mirror = 1;
            G_DoSpriteAnimations(tposx,tposy,z,fix16_to_int(tang),smoothratio);
            display_mirror = 0;

            renderDrawMasks();
            renderCompleteMirror();   //Reverse screen x-wise in this function
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
            gotpic[TILE_MIRROR>>3] &= ~(1<<(TILE_MIRROR&7));
        }
    }
}

void G_DrawRooms(int32_t playerNum, int32_t smoothRatio)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

    int yxAspect     = yxaspect;
    int viewingRange = viewingrange;

    //if (g_networkMode == NET_DEDICATED_SERVER) return;

    totalclocklock = totalclock;

    if (pub > 0 || videoGetRenderMode() >= REND_POLYMOST) // JBF 20040101: redraw background always
    {
        //videoClearScreen(0);
        if (ud.screen_size >= 8)
            G_DrawBackground();
        pub = 0;
    }

    if (ud.overhead_on == 2 || (pPlayer->cursectnum == -1 && videoGetRenderMode() != REND_CLASSIC))
        return;

    if (r_usenewaspect)
    {
        newaspect_enable = 1;
        videoSetCorrectedAspect();
    }

    if (ud.pause_on || pPlayer->on_crane > -1)
        smoothRatio = 65536;
    else
        smoothRatio = calc_smoothratio(totalclock, ototalclock);

    if (RRRA && fogactive)
        pPlayer->visibility = ud.const_visibility;

    int const playerVis = pPlayer->visibility;
    g_visibility        = (playerVis <= 0) ? 0 : (int32_t)(playerVis * (numplayers > 1 ? 1.f : r_ambientlightrecip));

    CAMERA(sect) = pPlayer->cursectnum;

    dointerpolations(smoothRatio);
    G_AnimateCamSprite(smoothRatio);

    if (ud.camerasprite >= 0)
    {
        spritetype *const pSprite = &sprite[ud.camerasprite];

        // XXX: what?
        if (pSprite->yvel < 0) pSprite->yvel = -100;
        else if (pSprite->yvel > 199) pSprite->yvel = 300;

        CAMERA(q16ang) = fix16_from_int(actor[ud.camerasprite].tempang
                                      + mulscale16(((pSprite->ang + 1024 - actor[ud.camerasprite].tempang) & 2047) - 1024, smoothRatio));

        if (!RR)
            G_SE40(smoothRatio);

        renderDrawRoomsQ16(pSprite->x, pSprite->y, pSprite->z - ZOFFSET6, CAMERA(q16ang), fix16_from_int(pSprite->yvel), pSprite->sectnum);
        G_DoSpriteAnimations(pSprite->x, pSprite->y, pSprite->z, fix16_to_int(CAMERA(q16ang)), smoothRatio);
        renderDrawMasks();
    }
    else
    {
        int32_t floorZ, ceilZ;

        int const vr            = divscale22(1, RR ? 64 : (sprite[pPlayer->i].yrepeat + 28));

        viewingRange = Blrintf(float(vr) * tanf(r_fov * (PI/360.f)));

        if (!RRRA || !pPlayer->DrugMode)
        {
            if (!r_usenewaspect)
                renderSetAspect(viewingRange, yxaspect);
            else
            {
                yxAspect     = tabledivide32_noinline(65536 * ydim * 8, xdim * 5);

                renderSetAspect(mulscale16(viewingRange,viewingrange), yxaspect);
            }
        }

        if (videoGetRenderMode() >= REND_POLYMOST && (ud.screen_tilting
#ifdef SPLITSCREEN_MOD_HACKS
        && !g_fakeMultiMode
#endif
        ))
        {
#ifdef USE_OPENGL
            renderSetRollAngle(pPlayer->orotscrnang + mulscale16(((pPlayer->rotscrnang - pPlayer->orotscrnang + 1024)&2047)-1024, smoothRatio));
#endif
            pPlayer->orotscrnang = pPlayer->rotscrnang;
        }

        if (RRRA && pPlayer->DrugMode > 0)
        {
            while (pPlayer->drug_timer < totalclock && !(pPlayer->gm & MODE_MENU) && !ud.pause_on && !System_WantGuiCapture())
            {
                int aspect;
                if (pPlayer->drug_stat[0] == 0)
                {
                    pPlayer->drug_stat[1]++;
                    aspect = viewingRange + pPlayer->drug_stat[1] * 5000;
                    if (viewingRange * 3 < aspect)
                    {
                        pPlayer->drug_aspect = viewingRange * 3;
                        pPlayer->drug_stat[0] = 2;
                    }
                    else
                    {
                        pPlayer->drug_aspect = aspect;
                    }
                    P_UpdateScreenPal(pPlayer);
                }
                else if (pPlayer->drug_stat[0] == 3)
                {
                    pPlayer->drug_stat[1]--;
                    aspect = viewingRange + pPlayer->drug_stat[1] * 5000;
                    if (aspect < viewingRange)
                    {
                        pPlayer->DrugMode = 0;
                        pPlayer->drug_stat[0] = 0;
                        pPlayer->drug_stat[2] = 0;
                        pPlayer->drug_stat[1] = 0;
                        pPlayer->drug_aspect = viewingRange;
                    }
                    else
                    {
                        pPlayer->drug_aspect = aspect;
                    }
                    P_UpdateScreenPal(pPlayer);
                }
                else if (pPlayer->drug_stat[0] == 2)
                {
                    if (pPlayer->drug_stat[2] > 30)
                    {
                        pPlayer->drug_stat[0] = 1;
                    }
                    else
                    {
                        pPlayer->drug_stat[2]++;
                        aspect = pPlayer->drug_stat[2] * 500 + viewingRange * 3;
                        pPlayer->drug_aspect = aspect;
                        P_UpdateScreenPal(pPlayer);
                    }
                }
                else
                {
                    if (pPlayer->drug_stat[2] < 1)
                    {
                        pPlayer->drug_stat[0] = 2;
                        pPlayer->DrugMode--;
                        if (pPlayer->DrugMode == 1)
                            pPlayer->drug_stat[0] = 3;
                    }
                    else
                    {
                        pPlayer->drug_stat[2]--;
                        aspect = pPlayer->drug_stat[2] * 500 + viewingRange * 3;
                        pPlayer->drug_aspect = aspect;
                        P_UpdateScreenPal(pPlayer);
                    }
                }

                pPlayer->drug_timer += TICSPERFRAME / 2;
            }
            if (!r_usenewaspect)
                renderSetAspect(pPlayer->drug_aspect, yxaspect);
            else
            {
                viewingRange = pPlayer->drug_aspect;
                yxAspect = tabledivide32_noinline(65536 * ydim * 8, xdim * 5);

                renderSetAspect(mulscale16(viewingRange, viewingrange), yxaspect);
            }
            P_UpdateScreenPal(pPlayer);
        }

        if (pPlayer->newowner < 0)
        {
            if (playerNum == myconnectindex && numplayers > 1)
            {
                vec3_t const camVect = { omypos.x + mulscale16(mypos.x - omypos.x, smoothRatio),
                                         omypos.y + mulscale16(mypos.y - omypos.y, smoothRatio),
                                         omypos.z + mulscale16(mypos.z - omypos.z, smoothRatio) };

                CAMERA(pos)      = camVect;
                CAMERA(q16ang)   = myang + fix16_from_int(pPlayer->look_ang);
                CAMERA(q16horiz) = myhoriz + myhorizoff;
                CAMERA(sect)     = mycursectnum;
            }
            else
            {
                vec3_t const camVect = { pPlayer->opos.x + mulscale16(pPlayer->pos.x - pPlayer->opos.x, smoothRatio),
                                         pPlayer->opos.y + mulscale16(pPlayer->pos.y - pPlayer->opos.y, smoothRatio),
                                         pPlayer->opos.z + mulscale16(pPlayer->pos.z - pPlayer->opos.z, smoothRatio) };

                CAMERA(pos)      = camVect;
                CAMERA(q16ang)   = pPlayer->q16ang + fix16_from_int(pPlayer->look_ang);
                CAMERA(q16horiz) = pPlayer->q16horiz + pPlayer->q16horizoff;
            }

            if (cl_viewbob)
            {
                int zAdd = (pPlayer->opyoff + mulscale16(pPlayer->pyoff-pPlayer->opyoff, smoothRatio));

                if (pPlayer->over_shoulder_on)
                    zAdd >>= 3;

                CAMERA(pos.z) += zAdd;
            }

            if (pPlayer->over_shoulder_on)
            {
                CAMERA(pos.z) -= 3072;

                if (G_DoThirdPerson(pPlayer, &CAMERA(pos), &CAMERA(sect), fix16_to_int(CAMERA(q16ang)), fix16_to_int(CAMERA(q16horiz))) < 0)
                {
                    CAMERA(pos.z) += 3072;
                    G_DoThirdPerson(pPlayer, &CAMERA(pos), &CAMERA(sect), fix16_to_int(CAMERA(q16ang)), fix16_to_int(CAMERA(q16horiz)));
                }
            }
        }
        else
        {
            vec3_t const camVect = G_GetCameraPosition(pPlayer->newowner, smoothRatio);

            // looking through viewscreen
            CAMERA(pos)      = camVect;
            CAMERA(q16ang)   = pPlayer->q16ang + fix16_from_int(pPlayer->look_ang);
            CAMERA(q16horiz) = fix16_from_int(100 + sprite[pPlayer->newowner].shade);
            CAMERA(sect)     = sprite[pPlayer->newowner].sectnum;
        }

        ceilZ  = actor[pPlayer->i].ceilingz;
        floorZ = actor[pPlayer->i].floorz;

        if (g_earthquakeTime > 0 && pPlayer->on_ground == 1)
        {
            CAMERA(pos.z) += 256 - (((g_earthquakeTime)&1) << 9);
            CAMERA(q16ang)   += fix16_from_int((2 - ((g_earthquakeTime)&2)) << 2);
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
                if (CAMERA(pos.z) < ceilZ+ZOFFSET6)
                    CAMERA(pos.z) = ceilZ+ZOFFSET6;

                if (CAMERA(pos.z) > floorZ-ZOFFSET6)
                    CAMERA(pos.z) = floorZ-ZOFFSET6;

            break;
        }

        CAMERA(q16horiz) = fix16_clamp(CAMERA(q16horiz), F16(HORIZ_MIN), F16(HORIZ_MAX));

        G_HandleMirror(CAMERA(pos.x), CAMERA(pos.y), CAMERA(pos.z), CAMERA(q16ang), CAMERA(q16horiz), smoothRatio);
        if (!RR)
            G_SE40(smoothRatio);

        if (RRRA)
            G_SE150(CAMERA(pos.x), CAMERA(pos.y), CAMERA(pos.z), CAMERA(q16ang), CAMERA(q16horiz), smoothRatio);
        // for G_PrintCoords
        dr_viewingrange = viewingrange;
        dr_yxaspect = yxaspect;
        if (RR && sector[CAMERA(sect)].lotag == 848)
        {
            renderDrawRoomsQ16(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),CAMERA(q16ang),CAMERA(q16horiz),CAMERA(sect));

            G_DoSpriteAnimations(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),fix16_to_int(CAMERA(q16ang)),smoothRatio);

            renderDrawMasks();

            int geoSector = 0;

            for (bsize_t gs = 0; gs < geocnt; gs++)
            {
                int spriteNum = headspritesect[geosector[gs]];
                while (spriteNum != -1)
                {
                    int spriteNext = nextspritesect[spriteNum];
                    changespritesect(spriteNum, geosectorwarp[gs]);
                    sprite[spriteNum].x -= geox[gs];
                    sprite[spriteNum].y -= geoy[gs];
                    setsprite(spriteNum, (vec3_t*)&sprite[spriteNum]);
                    spriteNum = spriteNext;
                }
                if (CAMERA(sect) == geosector[gs])
                    geoSector = gs;
            }

            CAMERA(pos.x) -= geox[geoSector];
            CAMERA(pos.y) -= geoy[geoSector];
            renderDrawRoomsQ16(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),CAMERA(q16ang),CAMERA(q16horiz),geosectorwarp[geoSector]);
            CAMERA(pos.x) += geox[geoSector];
            CAMERA(pos.y) += geoy[geoSector];
                
            for (bsize_t gs = 0; gs < geocnt; gs++)
            {
                int spriteNum = headspritesect[geosectorwarp[gs]];
                while (spriteNum != -1)
                {
                    int spriteNext = nextspritesect[spriteNum];
                    changespritesect(spriteNum, geosector[gs]);
                    sprite[spriteNum].x += geox[gs];
                    sprite[spriteNum].y += geoy[gs];
                    setsprite(spriteNum, (vec3_t*)&sprite[spriteNum]);
                    spriteNum = spriteNext;
                }
            }

            G_DoSpriteAnimations(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),fix16_to_int(CAMERA(q16ang)),smoothRatio);

            renderDrawMasks();

            for (bsize_t gs = 0; gs < geocnt; gs++)
            {
                int spriteNum = headspritesect[geosector[gs]];
                while (spriteNum != -1)
                {
                    int spriteNext = nextspritesect[spriteNum];
                    changespritesect(spriteNum, geosectorwarp2[gs]);
                    sprite[spriteNum].x -= geox2[gs];
                    sprite[spriteNum].y -= geoy2[gs];
                    setsprite(spriteNum, (vec3_t*)&sprite[spriteNum]);
                    spriteNum = spriteNext;
                }
                if (CAMERA(sect) == geosector[gs])
                    geoSector = gs;
            }

            CAMERA(pos.x) -= geox2[geoSector];
            CAMERA(pos.y) -= geoy2[geoSector];
            renderDrawRoomsQ16(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),CAMERA(q16ang),CAMERA(q16horiz),geosectorwarp2[geoSector]);
            CAMERA(pos.x) += geox2[geoSector];
            CAMERA(pos.y) += geoy2[geoSector];
                
            for (bsize_t gs = 0; gs < geocnt; gs++)
            {
                int spriteNum = headspritesect[geosectorwarp2[gs]];
                while (spriteNum != -1)
                {
                    int spriteNext = nextspritesect[spriteNum];
                    changespritesect(spriteNum, geosector[gs]);
                    sprite[spriteNum].x += geox2[gs];
                    sprite[spriteNum].y += geoy2[gs];
                    setsprite(spriteNum, (vec3_t*)&sprite[spriteNum]);
                    spriteNum = spriteNext;
                }
            }

            G_DoSpriteAnimations(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),fix16_to_int(CAMERA(q16ang)),smoothRatio);

            renderDrawMasks();
        }
        else
        {
            renderDrawRoomsQ16(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),CAMERA(q16ang),CAMERA(q16horiz),CAMERA(sect));
            if (!RR && (unsigned)ror_sprite < MAXSPRITES && drawing_ror == 1)  // viewing from bottom
                G_OROR_DupeSprites(&sprite[ror_sprite]);
            G_DoSpriteAnimations(CAMERA(pos.x),CAMERA(pos.y),CAMERA(pos.z),fix16_to_int(CAMERA(q16ang)),smoothRatio);
        }
        drawing_ror = 0;
        renderDrawMasks();
    }

    restoreinterpolations();

    if (!RRRA || !fogactive)
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
            lastvist = (int32_t) totalclock;
        }
    }

    if (r_usenewaspect)
    {
        newaspect_enable = 0;
        renderSetAspect(viewingRange, yxAspect);
    }
}

bool GameInterface::GenerateSavePic()
{
    G_DrawRooms(myconnectindex, 65536);
    return true;
}


void G_DumpDebugInfo(void)
{
    int32_t j,x;
    //    FILE * fp=fopen("condebug.log","w");

    for (x=0; x<MAXSTATUS; x++)
    {
        j = headspritestat[x];
        while (j >= 0)
        {
            buildprint("Sprite ", j, " (", TrackerCast(sprite[j].x), ",", TrackerCast(sprite[j].y), ",", TrackerCast(sprite[j].z),
                ") (picnum: ", TrackerCast(sprite[j].picnum), ")\n");
            buildprint("\n");
            j = nextspritestat[j];
        }
    }
}

// if <set_movflag_uncond> is true, set the moveflag unconditionally,
// else only if it equals 0.
static int32_t G_InitActor(int32_t i, int32_t tilenum, int32_t set_movflag_uncond)
{
    if (actorinfo[tilenum].scriptaddress)
    {
        auto sa = &ScriptCode[actorinfo[tilenum].scriptaddress];
        SH(i) = sa[0];
        AC_ACTION_ID(actor[i].t_data) = sa[1];
        AC_MOVE_ID(actor[i].t_data) = sa[2];

        if (set_movflag_uncond || (sa[3] && SHT(i) == 0))  // AC_MOVFLAGS
            SHT(i) = sa[3];

        return 1;
    }

    return 0;
}


static int G_MaybeTakeOnFloorPal(tspritetype *pSprite, int sectNum)
{
    int const floorPal = sector[sectNum].floorpal;

    if (floorPal && !lookups.noFloorPal(floorPal) && !actorflag(pSprite->owner, SFLAG_NOPAL))
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

void animatesprites_d(int x, int y, int a, int smoothratio);
void animatesprites_r(int x, int y, int a, int smoothratio);

void G_DoSpriteAnimations(int32_t ourx, int32_t oury, int32_t ourz, int32_t oura, int32_t smoothratio)
{
    if (!isRR()) animatesprites_d(ourx, oury, oura, smoothratio);
    else animatesprites_r(ourx, oury, oura, smoothratio);
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


static int32_t g_RTSPlaying;

// Returns: started playing?
int G_StartRTS(int lumpNum, int localPlayer)
{
    if (!adult_lockout && SoundEnabled() &&
        RTS_IsInitialized() && g_RTSPlaying == 0 && (snd_speech & (localPlayer ? 1 : 4)))
    {
        auto sid = RTS_GetSoundID(lumpNum - 1);
        if (sid != -1)
        {
            S_PlaySound(sid, CHAN_AUTO, CHANF_UI);
            g_RTSPlaying = 7;
            return 1;
        }
    }

    return 0;
}

// Trying to sanitize the mess of options and the mess of variables the mess was stored in. (Did I say this was a total mess before...? >) )
// Hopefully this is more comprehensible, at least it neatly stores everything useful in a single linear value...
bool GameInterface::validate_hud(int layout)
{
	if (layout <= (RR? 5: 6))	// Status bar with border
	{
		return !(ud.statusbarflags & STATUSBAR_NOSHRINK);
	}
	else if (layout <= 7) // Status bar fullscreen
	{
		return (!(ud.statusbarflags & STATUSBAR_NOFULL) || !(ud.statusbarflags & STATUSBAR_NOOVERLAY));
	}
	else if (layout == 8)	// Status bar overlay
	{
		return !(ud.statusbarflags & STATUSBAR_NOOVERLAY);
	}
	else if (layout == 9)	// Fullscreen HUD
	{
		return (!(ud.statusbarflags & STATUSBAR_NOMINI) || !(ud.statusbarflags & STATUSBAR_NOMODERN));
	}
	else if (layout == 10)
	{
		return !(ud.statusbarflags & STATUSBAR_NOMODERN);
	}
	else if (layout == 11)
	{
		return !(ud.statusbarflags & STATUSBAR_NONONE);
	}
	return false;
}

void GameInterface::set_hud_layout(int layout)
{
	static const uint8_t screen_size_vals[] = { 60, 54, 48, 40, 32, 24, 16, 8, 8, 4, 4, 0 };
	static const uint8_t screen_size_vals_rr[] = { 56, 48, 40, 32, 24, 16, 12, 8, 8, 4, 4, 0 };
	if (validate_hud(layout))
	{
		ud.screen_size = RR? screen_size_vals_rr[layout] : screen_size_vals[layout];
		ud.statusbarmode = layout >= 8;
		ud.althud = layout >= 10;
		G_UpdateScreenArea();
	}
}

void GameInterface::set_hud_scale(int scale)
{
    ud.statusbarscale = clamp(scale, 36, 100);
    G_UpdateScreenArea();
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
        if (inputState.UnboundKeyPressed(sc_F1) || inputState.UnboundKeyPressed(sc_F2) || cl_autovote)
        {
            G_AddUserQuote(GStrings("VoteCast"));
            Net_SendMapVote(inputState.UnboundKeyPressed(sc_F1) || cl_autovote ? cl_autovote-1 : 0);
            inputState.ClearKeyStatus(sc_F1);
            inputState.ClearKeyStatus(sc_F2);
        }
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
					S_PlaySound(RR ? 341 : THUD, CHAN_AUTO, CHANF_UI);
				}
            }
            else
            {
                hud_scale = hud_scale + 4;
            }

            G_UpdateScreenArea();
        }

        if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
        {
            buttonMap.ClearButton(gamefunc_Shrink_Screen);

            if (!SHIFTS_IS_PRESSED)
            {
				if (G_ChangeHudLayout(-1))
				{
					S_PlaySound(RR ? 341 : THUD, CHAN_AUTO, CHANF_UI);
				}
            }
            else
            {
                hud_scale = hud_scale - 4;
            }

            G_UpdateScreenArea();
        }
    }

    if (g_player[myconnectindex].ps->cheat_phase == 1 || (g_player[myconnectindex].ps->gm&(MODE_MENU|MODE_TYPE)) || System_WantGuiCapture())
        return;

    if (buttonMap.ButtonDown(gamefunc_See_Coop_View) && (GTFLAGS(GAMETYPE_COOPVIEW) || ud.recstat == 2))
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
        P_DoQuote(QUOTE_WEAPON_MODE_OFF-ud.showweapons,g_player[screenpeek].ps);
    }

    if (buttonMap.ButtonDown(gamefunc_Toggle_Crosshair))
    {
        buttonMap.ClearButton(gamefunc_Toggle_Crosshair);
        cl_crosshair = !cl_crosshair;
        P_DoQuote(QUOTE_CROSSHAIR_OFF-cl_crosshair,g_player[screenpeek].ps);
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
        P_DoQuote(QUOTE_MAP_FOLLOW_OFF+ud.scrollmode,g_player[myconnectindex].ps);
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
                G_AddUserQuote(*CombatMacros[ridiculeNum-1]);
				Net_SendTaunt(ridiculeNum);
				pus = NUMPAGES;
                pub = NUMPAGES;

                return;
            }

            // Not SHIFT -- that is, either some ALT or WIN.
            if (G_StartRTS(ridiculeNum, 1))
            {
				Net_SendRTS(ridiculeNum);
				pus = NUMPAGES;
                pub = NUMPAGES;

                return;
            }
        }
    }
    else
    {
        if (buttonMap.ButtonDown(gamefunc_Third_Person_View))
        {
            buttonMap.ClearButton(gamefunc_Third_Person_View);

            if (!RRRA || (!g_player[myconnectindex].ps->OnMotorcycle && !g_player[myconnectindex].ps->OnBoat))
            {
                g_player[myconnectindex].ps->over_shoulder_on = !g_player[myconnectindex].ps->over_shoulder_on;

                CAMERADIST  = 0;
                CAMERACLOCK = (int32_t) totalclock;

                P_DoQuote(QUOTE_VIEW_MODE_OFF + g_player[myconnectindex].ps->over_shoulder_on, g_player[myconnectindex].ps);
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
        G_UpdateScreenArea();
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
        G_UpdateScreenArea();
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
        G_GameExit("Error: too many labels defined!");

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

    VM_OnEvent(EVENT_INIT);
}

static inline void G_CheckGametype(void)
{
    m_coop = clamp(*m_coop, 0, g_gametypeCnt-1);
    Printf("%s\n",g_gametypeNames[m_coop]);
    if (g_gametypeFlags[m_coop] & GAMETYPE_ITEMRESPAWN)
        ud.m_respawn_items = ud.m_respawn_inventory = 1;
}

void G_PostCreateGameState(void)
{
    Net_SendClientInfo();
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

    G_InitDynamicTiles();
    //G_InitDynamicSounds();

    // These depend on having the dynamic tile and/or sound mappings set up:
    G_InitMultiPsky(TILE_CLOUDYOCEAN, TILE_MOONSKY1, TILE_BIGORBIT1, TILE_LA);
    G_PostCreateGameState();
    if (g_netServer || ud.multimode > 1) G_CheckGametype();

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
        G_GameExit("Failed loading art.");

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
    P_UpdateScreenPal(g_player[myconnectindex].ps);

    if (g_player[myconnectindex].ps->gm&MODE_EOL)
    {
        ready2send = 0;

        if (g_player[myconnectindex].ps->player_par > 0 && (g_player[myconnectindex].ps->player_par < ud.playerbest || ud.playerbest < 0) &&
            ud.display_bonus_screen == 1)
            CONFIG_SetMapBestTime(g_loadedMapHack.md4, g_player[myconnectindex].ps->player_par);

        if (ud.display_bonus_screen == 1)
        {
            int32_t i = ud.screen_size;
            ud.screen_size = 0;
            G_UpdateScreenArea();
            ud.screen_size = i;

            if (!RRRA || (g_mostConcurrentPlayers > 1 && numplayers > 1))
                G_BonusScreen(0);
            else
                G_BonusScreenRRRA(0);
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

// TODO: reorder (net)actor_t to eliminate slop and update assertion
EDUKE32_STATIC_ASSERT(sizeof(actor_t)%4 == 0);

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
    "Holo_Duke",			// Duke3D", RR
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
    ud.statusbarflags = 0;// STATUSBAR_NOSHRINK;
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
    bufferjitter = 1;
    initsynccrc();

    // This needs to happen before G_CheckCommandLine() because G_GameExit()
    // accesses g_player[0].
    G_MaybeAllocPlayer(0);

    checkcommandline();

    SetDefaults();


    hud_size.Callback();
    hud_scale.Callback();
    S_InitSound();

    
    if (RR)
    {
        g_cdTrack = -1;
    }

    G_SetupCheats();

    if (SHAREWARE)
        g_Shareware = 1;
    else
    {
		if (fileSystem.FileExists("DUKESW.BIN")) // JBF 20030810
        {
            g_Shareware = 1;
        }
    }

    numplayers = 1;
    g_mostConcurrentPlayers = ud.multimode;

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

    if (RR)
        tileDelete(0);
    if (RRRA)
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

    g_mostConcurrentPlayers = ud.multimode;  // XXX: redundant?

    ud.last_level = -1;
    registerosdcommands();

    videoInit();
    V_LoadTranslations();
    videoSetPalette(BASEPAL, 0);

    // check if the minifont will support lowercase letters (3136-3161)
    // there is room for them in tiles012.art between "[\]^_." and "{|}~"
    minitext_lowercase = 1;

    for (bssize_t i = TILE_MINIFONT + ('a'-'!'); minitext_lowercase && i < TILE_MINIFONT + ('z'-'!') + 1; ++i)
        minitext_lowercase &= (int)tileCheck(i);

    /*if (RRRA)
        playmve("REDINT.MVE"); still needs work on the audio side*/

    //if (g_networkMode != NET_DEDICATED_SERVER)
    {
        Menu_Init();
    }

    FX_StopAllSounds();
    S_ClearSoundLocks();
	app_loop();
	return 0;
}
	
int32_t G_PlaybackDemo(void);

void app_loop()
{
	auto &myplayer = g_player[myconnectindex].ps;

MAIN_LOOP_RESTART:
    totalclock = 0;
    ototalclock = 0;
    lockclock = 0;

    g_player[myconnectindex].ps->fta = 0;
    for (int & q : user_quote_time)
        q = 0;

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

            Net_WaitForEverybody();
        }
        else
        {
            fi.ShowLogo([](bool) {});
        }

        M_StartControlPanel(false);
		M_SetMenu(NAME_Mainmenu);
		if (G_PlaybackDemo())
        {
            FX_StopAllSounds();
            g_noLogoAnim = 1;
            goto MAIN_LOOP_RESTART;
        }
    }
    else G_UpdateScreenArea();

//    G_GameExit(" "); ///

    ud.showweapons = ud.config.ShowOpponentWeapons;
    P_SetupMiscInputSettings();
    g_player[myconnectindex].pteam = playerteam;

    if (g_gametypeFlags[ud.coop] & GAMETYPE_TDM)
        g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = G_GetTeamPalette(g_player[myconnectindex].pteam);
    else
    {
        if (playercolor) g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = G_CheckPlayerColor(playercolor);
        else g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor;
    }

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

        // only allow binds to function if the player is actually in a game (not in a menu, typing, et cetera) or demo
        inputState.SetBindsEnabled(!!(g_player[myconnectindex].ps->gm & (MODE_GAME|MODE_DEMO)));

        G_HandleLocalKeys();
 
        OSD_DispatchQueued();

        char gameUpdate = false;
        double const gameUpdateStartTime = timerGetHiTicks();
        
        while (((g_netClient || g_netServer) || !(g_player[myconnectindex].ps->gm & (MODE_MENU|MODE_DEMO))) && (int)(totalclock - ototalclock) >= TICSPERFRAME)
        {
            ototalclock += TICSPERFRAME;

            if (RRRA && g_player[myconnectindex].ps->OnMotorcycle)
                P_GetInputMotorcycle(myconnectindex);
            else if (RRRA && g_player[myconnectindex].ps->OnBoat)
                P_GetInputBoat(myconnectindex);
            else
                P_GetInput(myconnectindex);

            // this is where we fill the input_t struct that is actually processed by P_ProcessInput()
            auto const pPlayer = g_player[myconnectindex].ps;
            auto const q16ang  = fix16_to_int(pPlayer->q16ang);
            auto &     input   = inputfifo[g_player[myconnectindex].movefifoend&(MOVEFIFOSIZ-1)][myconnectindex];

            input = localInput;
            input.fvel = mulscale9(localInput.fvel, sintable[(q16ang + 2560) & 2047]) +
                         mulscale9(localInput.svel, sintable[(q16ang + 2048) & 2047]) +
                         pPlayer->fric.x;
            input.svel = mulscale9(localInput.fvel, sintable[(q16ang + 2048) & 2047]) +
                         mulscale9(localInput.svel, sintable[(q16ang + 1536) & 2047]) +
                         pPlayer->fric.y;
            localInput = {};

            g_player[myconnectindex].movefifoend++;

            if (((!System_WantGuiCapture() && (g_player[myconnectindex].ps->gm&MODE_MENU) != MODE_MENU) || ud.recstat == 2 || (g_netServer || ud.multimode > 1)) &&
                    (g_player[myconnectindex].ps->gm&MODE_GAME))
            {
                G_MoveLoop();
            }
        }

        gameUpdate = true;
        g_gameUpdateTime = timerGetHiTicks()-gameUpdateStartTime;
        if (g_gameUpdateAvgTime < 0.f)
            g_gameUpdateAvgTime = g_gameUpdateTime;
        g_gameUpdateAvgTime = ((GAMEUPDATEAVGTIMENUMSAMPLES-1.f)*g_gameUpdateAvgTime+g_gameUpdateTime)/((float) GAMEUPDATEAVGTIMENUMSAMPLES);

        G_DoCheats();

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
            if (RRRA && g_player[myconnectindex].ps->OnMotorcycle)
                P_GetInputMotorcycle(myconnectindex);
            else if (RRRA && g_player[myconnectindex].ps->OnBoat)
                P_GetInputBoat(myconnectindex);
            else
                P_GetInput(myconnectindex);

            int const smoothRatio = calc_smoothratio(totalclock, ototalclock);

            G_DrawRooms(screenpeek, smoothRatio);
            if (videoGetRenderMode() >= REND_POLYMOST)
                G_DrawBackground();
            G_DisplayRest(smoothRatio);
            videoNextPage();

            if (gameUpdate)
            {
                g_gameUpdateAndDrawTime = g_beforeSwapTime/* timerGetHiTicks()*/ - gameUpdateStartTime;
            }

        }

        if (g_player[myconnectindex].ps->gm&MODE_DEMO)
            goto MAIN_LOOP_RESTART;
    }
    while (1);
}

int32_t G_MoveLoop()
{
    int i;

    if (numplayers > 1)
        while (predictfifoplc < g_player[myconnectindex].movefifoend) Net_DoPrediction();

    Net_GetPackets();

    if (numplayers < 2) bufferjitter = 0;
    while (g_player[myconnectindex].movefifoend-movefifoplc > bufferjitter)
    {
        for(TRAVERSE_CONNECT(i))
        {
            if (movefifoplc == g_player[i].movefifoend) break;
        }
        if (i >= 0) break;
        if (G_DoMoveThings()) return 1;
    }


    return 0;
}

int G_DoMoveThings(void)
{
    ud.camerasprite = -1;
    lockclock += TICSPERFRAME;

    // Moved lower so it is restored correctly by demo diffs:
    //if (g_earthquakeTime > 0) g_earthquakeTime--;

    if (g_RTSPlaying > 0)
        g_RTSPlaying--;

    for (int & i : user_quote_time)
    {
        if (i)
        {
            if (--i > hud_messagetime)
                i = hud_messagetime;
            if (!i) pub = NUMPAGES;
        }
    }

    // Name display when aiming at opponents
    if (cl_idplayers && (g_netServer || ud.multimode > 1)
        )
    {
        hitdata_t hitData;
        DukePlayer_t *const pPlayer = g_player[screenpeek].ps;

        for (bssize_t TRAVERSE_CONNECT(i))
            if (g_player[i].ps->holoduke_on != -1)
                sprite[g_player[i].ps->holoduke_on].cstat ^= 256;

        hitscan((vec3_t *)pPlayer, pPlayer->cursectnum, sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047],
                sintable[fix16_to_int(pPlayer->q16ang) & 2047], fix16_to_int(F16(100) - pPlayer->q16horiz - pPlayer->q16horizoff) << 11, &hitData,
                0xffff0030);

        for (bssize_t TRAVERSE_CONNECT(i))
            if (g_player[i].ps->holoduke_on != -1)
                sprite[g_player[i].ps->holoduke_on].cstat ^= 256;

        if ((hitData.sprite >= 0) && !(g_player[myconnectindex].ps->gm & MODE_MENU) &&
                sprite[hitData.sprite].picnum == TILE_APLAYER)
        {
            int const playerNum = P_Get(hitData.sprite);

            if (playerNum != screenpeek && g_player[playerNum].ps->dead_flag == 0)
            {
                if (pPlayer->fta == 0 || pPlayer->ftq == QUOTE_RESERVED3)
                {
                    if (ldist(&sprite[pPlayer->i], &sprite[hitData.sprite]) < 9216)
                    {
						quoteMgr.FormatQuote(QUOTE_RESERVED3, "%s", &g_player[playerNum].user_name[0]);
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

    for (bssize_t TRAVERSE_CONNECT(i))
        Bmemcpy(g_player[i].input, &inputfifo[movefifoplc&(MOVEFIFOSIZ-1)][i], sizeof(input_t));

    movefifoplc++;

    updateinterpolations();

    g_moveThingsCount++;

    everyothertime++;
    if (g_earthquakeTime > 0) g_earthquakeTime--;

    if (ud.pause_on == 0)
    {
        g_globalRandom = krand2();
        movedummyplayers();//ST 13
    }

    for (bssize_t TRAVERSE_CONNECT(i))
    {
        if (g_player[i].input->extbits&(1<<6))
        {
            g_player[i].ps->team = g_player[i].pteam;
            if (g_gametypeFlags[ud.coop] & GAMETYPE_TDM)
            {
                actor[g_player[i].ps->i].picnum = TILE_APLAYERTOP;
                quickkill(g_player[i].ps);
            }
        }
        if (g_gametypeFlags[ud.coop] & GAMETYPE_TDM)
            g_player[i].ps->palookup = g_player[i].pcolor = G_GetTeamPalette(g_player[i].ps->team);

        if (sprite[g_player[i].ps->i].pal != 1)
            sprite[g_player[i].ps->i].pal = g_player[i].pcolor;

           hud_input(i);

        if (ud.pause_on == 0)
        {
            // todo: take HUD timer stuff out of the main game loop
            auto p = &ps[i];
            if (p->pals.f > 0)
                p->pals.f--;

            if (p->fta > 0)
            {
                p->fta--;
                if (p->fta == 0)
                {
                    p->ftq = 0;
                }
            }
            if (g_levelTextTime > 0)
                g_levelTextTime--;


            //P_ProcessInput(i);
            fi.processinput(i);
            fi.checksectors(i);
        }
    }

    if (ud.pause_on == 0)
        fi.think();

    Net_CorrectPrediction();

    if ((everyothertime&1) == 0)
    {
        {
            fi.animatewalls();
            movecyclers();
        }
    }

    if (RR && ud.recstat == 0 && ud.multimode < 2)
        dotorch();

    return 0;
}

void GameInterface::FreeGameData()
{
    setmapfog(0);
    G_Cleanup();
}

void GameInterface::UpdateScreenSize()
{
    G_UpdateScreenArea();
}

::GameInterface* CreateInterface()
{
	return new GameInterface;
}

// access wrappers that alias old names to current data.
psaccess ps;
actor_t* hittype = actor;


END_DUKE_NS
