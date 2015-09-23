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

#include "duke3d.h"
#include "game.h"
#include "common_game.h"
#include "osd.h"
#include "gamedef.h"
#include "premap.h"
#include "sounds.h"
#include "fx_man.h"
#include "gameexec.h"
#include "anim.h"
#include "menus.h"
#include "demo.h"

#ifdef LUNATIC
# include "lunatic_game.h"
#endif

halfdimen_t g_halfScreen;
int32_t g_halveScreenArea = 0;

static int32_t g_whichPalForPlayer = 9;

static uint8_t precachehightile[2][MAXTILES>>3];
static int32_t  g_precacheCount;

extern int32_t g_levelTextTime;

static void flag_precache(int32_t tile, int32_t type)
{
    if (!(gotpic[tile>>3] & pow2char[tile&7]))
        g_precacheCount++;
    gotpic[tile>>3] |= pow2char[tile&7];
    precachehightile[type][tile>>3] |= pow2char[tile&7];
}

static void tloadtile(int32_t tilenume, int32_t type)
{
    int32_t i,j;

    if ((picanm[tilenume].sf&PICANM_ANIMTYPE_MASK)==PICANM_ANIMTYPE_BACK)
    {
        i = tilenume - picanm[tilenume].num;
        j = tilenume;
    }
    else
    {
        i = tilenume;
        j = tilenume + picanm[tilenume].num;
    }

    for (; i<=j; i++)
        flag_precache(i, type);
}

static void G_CacheSpriteNum(int32_t i)
{
    char maxc;
    int32_t j;

    if (ud.monsters_off && A_CheckEnemySprite(&sprite[i])) return;

    maxc = 1;

    for (j = PN; j <= g_tile[PN].cacherange; j++)
        tloadtile(j,1);

    switch (DYNAMICTILEMAP(PN))
    {
    case HYDRENT__STATIC:
        tloadtile(BROKEFIREHYDRENT,1);
        for (j = TOILETWATER; j < (TOILETWATER+4); j++) tloadtile(j,1);
        break;
    case TOILET__STATIC:
        tloadtile(TOILETBROKE,1);
        for (j = TOILETWATER; j < (TOILETWATER+4); j++) tloadtile(j,1);
        break;
    case STALL__STATIC:
        tloadtile(STALLBROKE,1);
        for (j = TOILETWATER; j < (TOILETWATER+4); j++) tloadtile(j,1);
        break;
    case RUBBERCAN__STATIC:
        maxc = 2;
        break;
    case TOILETWATER__STATIC:
        maxc = 4;
        break;
    case FEMPIC1__STATIC:
        maxc = 44;
        break;
    case LIZTROOP__STATIC:
    case LIZTROOPRUNNING__STATIC:
    case LIZTROOPSHOOT__STATIC:
    case LIZTROOPJETPACK__STATIC:
    case LIZTROOPONTOILET__STATIC:
    case LIZTROOPDUCKING__STATIC:
        for (j = LIZTROOP; j < (LIZTROOP+72); j++) tloadtile(j,1);
        for (j=HEADJIB1; j<LEGJIB1+3; j++) tloadtile(j,1);
        maxc = 0;
        break;
    case WOODENHORSE__STATIC:
        maxc = 5;
        for (j = HORSEONSIDE; j < (HORSEONSIDE+4); j++) tloadtile(j,1);
        break;
    case NEWBEAST__STATIC:
    case NEWBEASTSTAYPUT__STATIC:
        maxc = 90;
        break;
    case BOSS1__STATIC:
    case BOSS2__STATIC:
    case BOSS3__STATIC:
        maxc = 30;
        break;
    case OCTABRAIN__STATIC:
    case OCTABRAINSTAYPUT__STATIC:
    case COMMANDER__STATIC:
    case COMMANDERSTAYPUT__STATIC:
        maxc = 38;
        break;
    case RECON__STATIC:
        maxc = 13;
        break;
    case PIGCOP__STATIC:
    case PIGCOPDIVE__STATIC:
        maxc = 61;
        break;
    case SHARK__STATIC:
        maxc = 30;
        break;
    case LIZMAN__STATIC:
    case LIZMANSPITTING__STATIC:
    case LIZMANFEEDING__STATIC:
    case LIZMANJUMP__STATIC:
        for (j=LIZMANHEAD1; j<LIZMANLEG1+3; j++) tloadtile(j,1);
        maxc = 80;
        break;
    case APLAYER__STATIC:
        maxc = 0;
        if ((g_netServer || ud.multimode > 1))
        {
            maxc = 5;
            for (j = 1420; j < 1420+106; j++) tloadtile(j,1);
        }
        break;
    case ATOMICHEALTH__STATIC:
        maxc = 14;
        break;
    case DRONE__STATIC:
        maxc = 10;
        break;
    case EXPLODINGBARREL__STATIC:
    case SEENINE__STATIC:
    case OOZFILTER__STATIC:
        maxc = 3;
        break;
    case NUKEBARREL__STATIC:
    case CAMERA1__STATIC:
        maxc = 5;
        break;
        // caching of HUD sprites for weapons that may be in the level
    case CHAINGUNSPRITE__STATIC:
        for (j=CHAINGUN; j<=CHAINGUN+7; j++) tloadtile(j,1);
        break;
    case RPGSPRITE__STATIC:
        for (j=RPGGUN; j<=RPGGUN+2; j++) tloadtile(j,1);
        break;
    case FREEZESPRITE__STATIC:
        for (j=FREEZE; j<=FREEZE+5; j++) tloadtile(j,1);
        break;
    case GROWSPRITEICON__STATIC:
    case SHRINKERSPRITE__STATIC:
        for (j=SHRINKER-2; j<=SHRINKER+5; j++) tloadtile(j,1);
        break;
    case HBOMBAMMO__STATIC:
    case HEAVYHBOMB__STATIC:
        for (j=HANDREMOTE; j<=HANDREMOTE+5; j++) tloadtile(j,1);
        break;
    case TRIPBOMBSPRITE__STATIC:
        for (j=HANDHOLDINGLASER; j<=HANDHOLDINGLASER+4; j++) tloadtile(j,1);
        break;
    case SHOTGUNSPRITE__STATIC:
        tloadtile(SHOTGUNSHELL,1);
        for (j=SHOTGUN; j<=SHOTGUN+6; j++) tloadtile(j,1);
        break;
    case DEVISTATORSPRITE__STATIC:
        for (j=DEVISTATOR; j<=DEVISTATOR+1; j++) tloadtile(j,1);
        break;

    }

    for (j = PN; j < (PN+maxc); j++) tloadtile(j,1);
}

static void G_PrecacheSprites(void)
{
    int32_t i,j;

    for (i=0; i<MAXTILES; i++)
    {
        if (g_tile[i].flags & SFLAG_PROJECTILE)
            tloadtile(i,1);

        if (A_CheckSpriteTileFlags(i, SFLAG_CACHE))
            for (j = i; j <= g_tile[i].cacherange; j++)
                tloadtile(j,1);
    }
    tloadtile(BOTTOMSTATUSBAR,1);
    if ((g_netServer || ud.multimode > 1))
        tloadtile(FRAGBAR,1);

    tloadtile(VIEWSCREEN,1);

    for (i=STARTALPHANUM; i<ENDALPHANUM+1; i++) tloadtile(i,1);
    for (i=BIGALPHANUM-11; i<BIGALPHANUM+82; i++) tloadtile(i,1);
    for (i=MINIFONT; i<MINIFONT+93; i++) tloadtile(i,1);

    for (i=FOOTPRINTS; i<FOOTPRINTS+3; i++) tloadtile(i,1);

    for (i = BURNING; i < BURNING+14; i++) tloadtile(i,1);
    for (i = BURNING2; i < BURNING2+14; i++) tloadtile(i,1);

    for (i = CRACKKNUCKLES; i < CRACKKNUCKLES+4; i++) tloadtile(i,1);

    for (i = FIRSTGUN; i < FIRSTGUN+3 ; i++) tloadtile(i,1);
    for (i = FIRSTGUNRELOAD; i < FIRSTGUNRELOAD+8 ; i++) tloadtile(i,1);

    for (i = EXPLOSION2; i < EXPLOSION2+21 ; i++) tloadtile(i,1);

    for (i = COOLEXPLOSION1; i < COOLEXPLOSION1+21 ; i++) tloadtile(i,1);

    tloadtile(BULLETHOLE,1);
    tloadtile(BLOODPOOL,1);
    for (i = TRANSPORTERBEAM; i < (TRANSPORTERBEAM+6); i++) tloadtile(i,1);

    for (i = SMALLSMOKE; i < (SMALLSMOKE+4); i++) tloadtile(i,1);
    for (i = SHOTSPARK1; i < (SHOTSPARK1+4); i++) tloadtile(i,1);

    for (i = BLOOD; i < (BLOOD+4); i++) tloadtile(i,1);
    for (i = JIBS1; i < (JIBS5+5); i++) tloadtile(i,1);
    for (i = JIBS6; i < (JIBS6+8); i++) tloadtile(i,1);

    for (i = SCRAP1; i < (SCRAP1+29); i++) tloadtile(i,1);

    tloadtile(FIRELASER,1);
    for (i=TRANSPORTERSTAR; i<TRANSPORTERSTAR+6; i++) tloadtile(i,1);
    for (i=FORCERIPPLE; i<(FORCERIPPLE+9); i++) tloadtile(i,1);

    for (i=MENUSCREEN; i<DUKECAR; i++) tloadtile(i,1);

    for (i=RPG; i<RPG+7; i++) tloadtile(i,1);
    for (i=FREEZEBLAST; i<FREEZEBLAST+3; i++) tloadtile(i,1);
    for (i=SHRINKSPARK; i<SHRINKSPARK+4; i++) tloadtile(i,1);
    for (i=GROWSPARK; i<GROWSPARK+4; i++) tloadtile(i,1);
    for (i=SHRINKEREXPLOSION; i<SHRINKEREXPLOSION+4; i++) tloadtile(i,1);
    for (i=MORTER; i<MORTER+4; i++) tloadtile(i,1);
    for (i=0; i<=60; i++) tloadtile(i,1);
}

// FIXME: this function is a piece of shit, needs specific sounds listed
static int32_t G_CacheSound(uint32_t num)
{
    int16_t fp = -1;
    int32_t   l;

    if (num >= MAXSOUNDS || !ud.config.SoundToggle) return 0;

    if (EDUKE32_PREDICT_FALSE(!g_sounds[num].filename)) return 0;

#if defined HAVE_FLAC || defined HAVE_VORBIS
    fp = S_UpgradeFormat(g_sounds[num].filename, g_loadFromGroupOnly);
    if (fp == -1)
#endif
        fp = kopen4loadfrommod(g_sounds[num].filename,g_loadFromGroupOnly);
    if (fp == -1)
    {
//        OSD_Printf(OSDTEXT_RED "Sound %s(#%d) not found!\n",g_sounds[num].filename,num);
        return 0;
    }

    l = kfilelength(fp);
    g_sounds[num].soundsiz = l;

    if ((ud.level_number == 0 && ud.volume_number == 0 && (num == 189 || num == 232 || num == 99 || num == 233 || num == 17)) ||
            (l < 12288))
    {
        g_soundlocks[num] = 199;
        allocache((intptr_t *)&g_sounds[num].ptr,l,(char *)&g_soundlocks[num]);
        if (g_sounds[num].ptr != NULL)
            kread(fp, g_sounds[num].ptr , l);
    }
    kclose(fp);
    return 1;
}

static void G_PrecacheSounds(void)
{
    int32_t i, j = 0;

    for (i=MAXSOUNDS-1; i>=0; i--)
        if (g_sounds[i].ptr == 0)
        {
            j++;
            if ((j&7) == 0)
                G_HandleAsync();

            G_CacheSound(i);
        }
}

static void G_DoLoadScreen(const char *statustext, int32_t percent)
{
    int32_t i=0,j;

    if (ud.recstat != 2)
    {
        j = VM_OnEventWithReturn(EVENT_GETLOADTILE, -1, myconnectindex, LOADSCREEN);

        //g_player[myconnectindex].ps->palette = palette;
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 1);    // JBF 20040308

        if (!statustext)
        {
            i = ud.screen_size;
            ud.screen_size = 0;
            G_UpdateScreenArea();
            clearallviews(0L);
        }

        if ((uint32_t)j < 2*MAXTILES)
        {
            clearallviews(0);

            rotatesprite_fs(320<<15,200<<15,65536L,0, j > MAXTILES-1?j-MAXTILES:j,0,0,
                            2+8+64+BGSTRETCH);
        }
        else
        {
            nextpage();
            return;
        }

        if (boardfilename[0] != 0 && ud.level_number == 7 && ud.volume_number == 0)
        {
            menutext(160,90,0,0,"Loading User Map");
            gametextpal(160,90+10,boardfilename,14,2);
        }
        else
        {
            menutext(160,90,0,0,"Loading");
            if (MapInfo[(ud.volume_number*MAXLEVELS) + ud.level_number].name != NULL)
                menutext(160,90+16+8,0,0,MapInfo[(ud.volume_number*MAXLEVELS) + ud.level_number].name);
        }

#ifndef EDUKE32_TOUCH_DEVICES
        if (statustext) gametext(160,180,statustext,0,2+8+16);
#endif

        if (percent != -1)
        {
            int32_t ii = scale(scale(xdim-1,288,320),percent,100);
            rotatesprite(31<<16,145<<16,65536,0,929,15,0,2+8+16,0,0,ii,ydim-1);
            rotatesprite(159<<16,145<<16,65536,0,929,15,0,2+8+16,0,0,ii,ydim-1);
            rotatesprite(30<<16,144<<16,65536,0,929,0,0,2+8+16,0,0,ii,ydim-1);
            rotatesprite(158<<16,144<<16,65536,0,929,0,0,2+8+16,0,0,ii,ydim-1);
        }

        VM_OnEventWithReturn(EVENT_DISPLAYLOADINGSCREEN, g_player[screenpeek].ps->i, screenpeek, percent);
        nextpage();

        if (!statustext)
        {
            KB_FlushKeyboardQueue();
            ud.screen_size = i;
        }
    }
    else
    {
        if (!statustext)
        {
            clearallviews(0L);
            //g_player[myconnectindex].ps->palette = palette;
            //G_FadePalette(0,0,0,0);
            P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308
        }
        /*Gv_SetVar(g_iReturnVarID,LOADSCREEN, -1, -1);*/

        j = VM_OnEventWithReturn(EVENT_GETLOADTILE, -1, myconnectindex, LOADSCREEN);

        if ((uint32_t)j < 2*MAXTILES)
        {
            rotatesprite_fs(320<<15,200<<15,65536L, 0,j > MAXTILES-1?j-MAXTILES:j,0,0,
                            2+8+64+BGSTRETCH);
        }
        else
        {
            nextpage();
            return;
        }

        menutext(160,105,0,0,"Loading...");
        if (statustext) gametext(160,180,statustext,0,2+8+16);
        VM_OnEventWithReturn(EVENT_DISPLAYLOADINGSCREEN, g_player[screenpeek].ps->i, screenpeek, percent);
        nextpage();
    }
}

extern void G_SetCrosshairColor(int32_t r, int32_t g, int32_t b);
extern palette_t CrosshairColors;

void G_CacheMapData(void)
{
    int32_t i,j,pc=0;
    int32_t tc;
    uint32_t starttime, endtime;

    if (ud.recstat == 2)
        return;

#ifndef EDUKE32_TOUCH_DEVICES
    S_PauseMusic(1);
#endif

    if (MapInfo[MUS_LOADING].musicfn)
    {
        S_StopMusic();
        S_PlayMusic(MapInfo[MUS_LOADING].musicfn);
    }

#if defined EDUKE32_TOUCH_DEVICES && defined USE_OPENGL
    polymost_glreset();
#endif

    starttime = getticks();

    G_PrecacheSounds();
    G_PrecacheSprites();

    for (i=0; i<numwalls; i++)
    {
        tloadtile(wall[i].picnum, 0);

        if (wall[i].overpicnum >= 0)
        {
            tloadtile(wall[i].overpicnum, 0);
        }
    }

    for (i=0; i<numsectors; i++)
    {
        tloadtile(sector[i].floorpicnum, 0);
        tloadtile(sector[i].ceilingpicnum, 0);
        if (sector[i].ceilingpicnum == LA)  // JBF 20040509: if( waloff[sector[i].ceilingpicnum] == LA) WTF?!?!?!?
        {
            tloadtile(LA+1, 0);
            tloadtile(LA+2, 0);
        }

        for (SPRITES_OF_SECT(i, j))
            if (sprite[j].xrepeat != 0 && sprite[j].yrepeat != 0 && (sprite[j].cstat&32768) == 0)
                G_CacheSpriteNum(j);
    }

    tc = totalclock;
    j = 0;

    int lpc = -1;

    for (i=0; i<MAXTILES; i++)
    {
        if (!(i&7) && !gotpic[i>>3])
        {
            i+=7;
            continue;
        }
        if (gotpic[i>>3] & pow2char[i&7])
        {
            if (waloff[i] == 0)
                loadtile((int16_t)i);

#ifdef USE_OPENGL
// PRECACHE
            if (ud.config.useprecache && bpp > 8)
            {
                int32_t k,type;

                for (type=0; type<=1; type++)
                    if (precachehightile[type][i>>3] & pow2char[i&7])
                    {
                        k = 0;
                        for (k=0; k<MAXPALOOKUPS-RESERVEDPALS && !KB_KeyPressed(sc_Space); k++)
                        {
                            // this is the CROSSHAIR_PAL, see comment in game.c
                            if (k == MAXPALOOKUPS-RESERVEDPALS-1)
                                break;
#ifdef POLYMER
                            if (getrendermode() != REND_POLYMER || !polymer_havehighpalookup(0, k))
#endif
                                polymost_precache(i,k,type);
                        }

#ifndef EDUKE32_GLES
                        if (r_detailmapping && !KB_KeyPressed(sc_Space))
                            polymost_precache(i,DETAILPAL,type);
                        if (r_glowmapping && !KB_KeyPressed(sc_Space))
                            polymost_precache(i,GLOWPAL,type);
#endif
#ifdef POLYMER
                        if (getrendermode() == REND_POLYMER)
                        {
                            if (pr_specularmapping && !KB_KeyPressed(sc_Space))
                                polymost_precache(i,SPECULARPAL,type);
                            if (pr_normalmapping && !KB_KeyPressed(sc_Space))
                                polymost_precache(i,NORMALPAL,type);
                        }
#endif
                    }
            }
#endif
            j++;
            pc++;
        }
        else continue;

        MUSIC_Update();

        if ((j&7) == 0)
            G_HandleAsync();

        if (bpp > 8 && totalclock - tc > TICRATE/4)
        {
            /*Bsprintf(tempbuf,"%d resources remaining\n",g_precacheCount-pc+1);*/
            int percentage = min(100, tabledivide32_noinline(100 * pc, g_precacheCount));

            while (percentage > lpc)
            {
                Bsprintf(tempbuf, "Loaded %d%% (%d/%d textures)\n", lpc, pc, g_precacheCount);
                G_DoLoadScreen(tempbuf, lpc);
                sampletimer();

                if (totalclock - tc >= 1)
                {
                    tc = totalclock;
                    lpc++;
                }

//                OSD_Printf("percentage %d lpc %d\n", percentage, lpc);
            }

            tc = totalclock;
        }
    }

    Bmemset(gotpic, 0, sizeof(gotpic));

    endtime = getticks();
    OSD_Printf("Cache time: %dms\n", endtime-starttime);
}

void G_UpdateScreenArea(void)
{
    if (!in3dmode())
        return;

    ud.screen_size = clamp(ud.screen_size, 0, 64);
    if (ud.screen_size == 0)
        flushperms();

    {
        const int32_t ss = max(ud.screen_size-8,0);

        const int32_t x1 = scale(ss,xdim,160);
        int32_t x2 = xdim-x1;

        int32_t y1 = ss;
        int32_t y2 = 200;

        if (ud.screen_size > 0 && (GametypeFlags[ud.coop]&GAMETYPE_FRAGBAR) && (g_netServer || ud.multimode > 1))
        {
            int32_t i, j = 0;

            for (TRAVERSE_CONNECT(i))
                if (i > j) j = i;

            if (j >= 1) y1 += 8;
            if (j >= 4) y1 += 8;
            if (j >= 8) y1 += 8;
            if (j >= 12) y1 += 8;
        }

        if (ud.screen_size >= 8 && ud.statusbarmode==0)
            y2 -= (ss+scale(tilesiz[BOTTOMSTATUSBAR].y,ud.statusbarscale,100));

        y1 = scale(y1,ydim,200);
        y2 = scale(y2,ydim,200)+(getrendermode() != REND_CLASSIC);

        if (g_halveScreenArea)
        {
            int32_t ourxdimen=x2-x1, ourydimen=y2-y1;

            g_halfScreen.x1 = x1;
            g_halfScreen.y1 = y1;
            g_halfScreen.xdimen = (ourxdimen>>1);
            g_halfScreen.ydimen = (ourydimen>>1);

            x2 = x1 + (ourxdimen>>1);
            y2 = y1 + (ourydimen>>1);
        }

        setview(x1,y1,x2-1,y2-1);
    }

    G_GetCrosshairColor();
    G_SetCrosshairColor(CrosshairColors.r, CrosshairColors.g, CrosshairColors.b);

    pub = NUMPAGES;
    pus = NUMPAGES;
}

void P_RandomSpawnPoint(int32_t snum)
{
    DukePlayer_t *p = g_player[snum].ps;
    int32_t i=snum,j,k;
    uint32_t dist,pdist = -1;

    if ((g_netServer || ud.multimode > 1) && !(GametypeFlags[ud.coop] & GAMETYPE_FIXEDRESPAWN))
    {
        i = krand()%g_numPlayerSprites;
        if (GametypeFlags[ud.coop] & GAMETYPE_TDMSPAWN)
        {
            for (j=0; j<ud.multimode; j++)
            {
                if (j != snum && g_player[j].ps->team == p->team && sprite[g_player[j].ps->i].extra > 0)
                {
                    for (k=0; k<g_numPlayerSprites; k++)
                    {
                        dist = FindDistance2D(g_player[j].ps->pos.x-g_playerSpawnPoints[k].pos.x,g_player[j].ps->pos.y-g_playerSpawnPoints[k].pos.y);
                        if (dist < pdist)
                            i = k, pdist = dist;
                    }
                    break;
                }
            }
        }
    }

    p->bobpos.x = p->opos.x = p->pos.x = g_playerSpawnPoints[i].pos.x;
    p->bobpos.y = p->opos.y = p->pos.y = g_playerSpawnPoints[i].pos.y;
    p->opos.z = p->pos.z = g_playerSpawnPoints[i].pos.z;
    p->ang = g_playerSpawnPoints[i].ang;
    p->cursectnum = g_playerSpawnPoints[i].sect;
    sprite[p->i].cstat = 1+256;
}

static inline void P_ResetTintFade(DukePlayer_t *ps)
{
    ps->pals.f = 0;
#ifdef LUNATIC
    ps->palsfadeprio = 0;
#endif
}

void P_ResetPlayer(int32_t snum)
{
    vec3_t tmpvect;
    DukePlayer_t *const pl = g_player[snum].ps;
    spritetype *const sp = &sprite[pl->i];

    tmpvect.x = pl->pos.x;
    tmpvect.y = pl->pos.y;
    tmpvect.z = pl->pos.z+PHEIGHT;
    P_RandomSpawnPoint(snum);
    sp->x = actor[pl->i].bpos.x = pl->bobpos.x = pl->opos.x = pl->pos.x;
    sp->y = actor[pl->i].bpos.y = pl->bobpos.y = pl->opos.y = pl->pos.y;
    sp->z = actor[pl->i].bpos.y = pl->opos.z =pl->pos.z;
    updatesector(pl->pos.x,pl->pos.y,&pl->cursectnum);
    setsprite(pl->i,&tmpvect);
    sp->cstat = 257;

    sp->shade = -12;
    sp->clipdist = 64;
    sp->xrepeat = 42;
    sp->yrepeat = 36;
    sp->owner = pl->i;
    sp->xoffset = 0;
    sp->pal = pl->palookup;

    pl->last_extra = sp->extra = pl->max_player_health;
    pl->wantweaponfire = -1;
    pl->horiz = 100;
    pl->on_crane = -1;
    pl->frag_ps = snum;
    pl->horizoff = 0;
    pl->opyoff = 0;
    pl->wackedbyactor = -1;
    pl->inv_amount[GET_SHIELD] = g_startArmorAmount;
    pl->dead_flag = 0;
    pl->footprintcount = 0;
    pl->weapreccnt = 0;
    pl->fta = 0;
    pl->ftq = 0;
    pl->vel.x = pl->vel.y = 0;
    pl->rotscrnang = 0;
    pl->runspeed = g_playerFriction;
    pl->falling_counter = 0;

    P_ResetTintFade(pl);

    actor[pl->i].extra = -1;
    actor[pl->i].owner = pl->i;

    actor[pl->i].cgg = 0;
    actor[pl->i].movflag = 0;
    actor[pl->i].tempang = 0;
    actor[pl->i].actorstayput = -1;
    actor[pl->i].dispicnum = 0;
    actor[pl->i].owner = pl->i;

    actor[pl->i].t_data[4] = 0;

    P_ResetInventory(snum);
    P_ResetWeapons(snum);

    pl->reloading = 0;

    pl->movement_lock = 0;

    VM_OnEvent(EVENT_RESETPLAYER, pl->i, snum);
}

void P_ResetStatus(int32_t snum)
{
    DukePlayer_t *p = g_player[snum].ps;

    ud.show_help        = 0;
    ud.showallmap       = 0;
    p->dead_flag        = 0;
    p->wackedbyactor    = -1;
    p->falling_counter  = 0;
    p->quick_kick       = 0;
    p->subweapon        = 0;
    p->last_full_weapon = 0;
    p->ftq              = 0;
    p->fta              = 0;
    p->tipincs          = 0;
    p->buttonpalette    = 0;
    p->actorsqu         =-1;
    p->invdisptime      = 0;
    p->refresh_inventory= 0;
    p->last_pissed_time = 0;
    p->holster_weapon   = 0;
    p->pycount          = 0;
    p->pyoff            = 0;
    p->opyoff           = 0;
    p->loogcnt          = 0;
    p->angvel           = 0;
    p->weapon_sway      = 0;
    p->extra_extra8     = 0;
    p->show_empty_weapon= 0;
    p->dummyplayersprite=-1;
    p->crack_time       = 0;
    p->hbomb_hold_delay = 0;
    p->transporter_hold = 0;
    p->wantweaponfire  = -1;
    p->hurt_delay       = 0;
    p->footprintcount   = 0;
    p->footprintpal     = 0;
    p->footprintshade   = 0;
    p->jumping_toggle   = 0;
    p->ohoriz = p->horiz= 140;
    p->horizoff         = 0;
    p->bobcounter       = 0;
    p->on_ground        = 0;
    p->player_par       = 0;
    p->return_to_center = 9;
    p->airleft          = 15*GAMETICSPERSEC;
    p->rapid_fire_hold  = 0;
    p->toggle_key_flag  = 0;
    p->access_spritenum = -1;
    if ((g_netServer || ud.multimode > 1) && (GametypeFlags[ud.coop] & GAMETYPE_ACCESSATSTART))
        p->got_access = 7;
    else p->got_access      = 0;
    p->random_club_frame= 0;
    pus = 1;
    p->on_warping_sector = 0;
    p->spritebridge      = 0;
    p->sbs          = 0;
    p->palette = BASEPAL;

    if (p->inv_amount[GET_STEROIDS] < 400)
    {
        p->inv_amount[GET_STEROIDS] = 0;
        p->inven_icon = ICON_NONE;
    }
    p->heat_on =            0;
    p->jetpack_on =         0;
    p->holoduke_on =       -1;

    p->look_ang          = 512 - ((ud.level_number&1)<<10);

    p->rotscrnang        = 0;
    p->orotscrnang       = 1;   // JBF 20031220
    p->newowner          =-1;
    p->jumping_counter   = 0;
    p->hard_landing      = 0;
    p->vel.x             = 0;
    p->vel.y             = 0;
    p->vel.z             = 0;
    p->fric.x        = 0;
    p->fric.y        = 0;
    p->somethingonplayer =-1;
    p->one_eighty_count  = 0;
    p->cheat_phase       = 0;

    p->on_crane          = -1;

    if ((PWEAPON(snum, p->curr_weapon, WorksLike) == PISTOL_WEAPON) &&
            (PWEAPON(snum, p->curr_weapon, Reload) > PWEAPON(snum, p->curr_weapon, TotalTime)))
        p->kickback_pic  = PWEAPON(snum, p->curr_weapon, TotalTime);
    else p->kickback_pic = 0;

    p->weapon_pos        = WEAPON_POS_START;
    p->walking_snd_toggle= 0;
    p->weapon_ang        = 0;

    p->knuckle_incs      = 1;
    p->fist_incs = 0;
    p->knee_incs         = 0;
    p->jetpack_on        = 0;
    p->reloading        = 0;

    p->movement_lock     = 0;

    p->frag_ps          = snum;

    P_UpdateScreenPal(p);
    VM_OnEvent(EVENT_RESETPLAYER, p->i, snum);
}

void P_ResetWeapons(int32_t snum)
{
    int32_t weapon;
    DukePlayer_t *p = g_player[snum].ps;

    for (weapon = PISTOL_WEAPON; weapon < MAX_WEAPONS; weapon++)
        p->ammo_amount[weapon] = 0;

    p->weapon_pos = WEAPON_POS_START;
    p->curr_weapon = PISTOL_WEAPON;
    p->kickback_pic = PWEAPON(snum, p->curr_weapon, TotalTime);
    p->gotweapon = ((1<<PISTOL_WEAPON) | (1<<KNEE_WEAPON) | (1<<HANDREMOTE_WEAPON));
    p->ammo_amount[PISTOL_WEAPON] = min(p->max_ammo_amount[PISTOL_WEAPON], 48);
    p->last_weapon = -1;

    p->show_empty_weapon= 0;
    p->last_pissed_time = 0;
    p->holster_weapon = 0;
    VM_OnEvent(EVENT_RESETWEAPONS, p->i, snum);
}

void P_ResetInventory(int32_t snum)
{
    DukePlayer_t *p = g_player[snum].ps;

    Bmemset(p->inv_amount, 0, sizeof(p->inv_amount));

    p->scuba_on =           0;
    p->heat_on = 0;
    p->jetpack_on =         0;
    p->holoduke_on = -1;
    p->inv_amount[GET_SHIELD] =      g_startArmorAmount;
    p->inven_icon = ICON_NONE;
    VM_OnEvent(EVENT_RESETINVENTORY, p->i, snum);
}

static void resetprestat(int32_t snum,int32_t g)
{
    DukePlayer_t *p = g_player[snum].ps;
    int32_t i;

    g_spriteDeleteQueuePos = 0;
    for (i=0; i<g_spriteDeleteQueueSize; i++) SpriteDeletionQueue[i] = -1;

    p->hbomb_on          = 0;
    p->cheat_phase       = 0;
    p->toggle_key_flag   = 0;
    p->secret_rooms      = 0;
    p->max_secret_rooms  = 0;
    p->actors_killed     = 0;
    p->max_actors_killed = 0;
    p->lastrandomspot    = 0;
    p->weapon_pos = WEAPON_POS_START;

    P_ResetTintFade(p);

    if ((PWEAPON(snum, p->curr_weapon, WorksLike) == PISTOL_WEAPON) &&
            (PWEAPON(snum, p->curr_weapon, Reload) > PWEAPON(snum, p->curr_weapon, TotalTime)))
        p->kickback_pic  = PWEAPON(snum, p->curr_weapon, TotalTime);
    else p->kickback_pic = 0;

    p->last_weapon = -1;
    p->weapreccnt = 0;
    p->interface_toggle_flag = 0;
    p->show_empty_weapon= 0;
    p->holster_weapon = 0;
    p->last_pissed_time = 0;

    p->one_parallax_sectnum = -1;
    p->visibility = ud.const_visibility;

    screenpeek              = myconnectindex;
    g_numAnimWalls            = 0;
    g_numCyclers              = 0;
    g_animateCount              = 0;
    parallaxtype            = 0;
    randomseed              = 1996;
    ud.pause_on             = 0;
    ud.camerasprite         =-1;
    ud.eog                  = 0;
    tempwallptr             = 0;
    g_curViewscreen               =-1;
    g_earthquakeTime          = 0;

    g_numInterpolations = 0;
    startofdynamicinterpolations = 0;

    if (((g&MODE_EOL) != MODE_EOL && numplayers < 2 && !g_netServer) ||
            (!(GametypeFlags[ud.coop]&GAMETYPE_PRESERVEINVENTORYDEATH) && numplayers > 1))
    {
        P_ResetWeapons(snum);
        P_ResetInventory(snum);
    }
    else if (PWEAPON(snum, p->curr_weapon, WorksLike) == HANDREMOTE_WEAPON)
    {
        p->ammo_amount[HANDBOMB_WEAPON]++;
        p->curr_weapon = HANDBOMB_WEAPON;
    }

    p->timebeforeexit   = 0;
    p->customexitsound  = 0;
}

// Tweak sprites contained in moving sectors with these SE lotags.
#define FIXSPR_SELOTAGP(k) \
    ((k)==SE_0_ROTATING_SECTOR \
     || (k)==SE_6_SUBWAY \
     || (k)==SE_14_SUBWAY_CAR)

// Set up sprites in moving sectors that are to be fixed wrt a certain pivot
// position and should not diverge from it due to roundoff error in the future.
// Has to be after the spawning stuff.
static void G_SetupRotfixedSprites(void)
{
    int32_t i;

    for (i=headspritestat[STAT_EFFECTOR]; i>=0; i=nextspritestat[i])
    {
        if (FIXSPR_SELOTAGP(sprite[i].lotag))
        {
#ifdef YAX_ENABLE
            int32_t firstrun = 1;
#endif
            int32_t j = headspritesect[sprite[i].sectnum];

            while (j>=0)
            {
                const spritetype *const spr = &sprite[j];

                // TRIPBOMB uses t_data[7] for its own purposes. Wouldn't be
                // too useful with moving sectors anyway
                if ((ROTFIXSPR_STATNUMP(spr->statnum) && spr->picnum!=TRIPBOMB) ||
                    ((spr->statnum==STAT_ACTOR || spr->statnum==STAT_ZOMBIEACTOR) &&
                     A_CheckSpriteTileFlags(spr->picnum, SFLAG_ROTFIXED)))
                {
                    int32_t pivot = i;

                    if (sprite[i].lotag==0)
                        pivot = sprite[i].owner;
                    if (j!=i && j!=pivot && pivot>=0 && pivot<MAXSPRITES)
                    {
                        // let's hope we don't step on anyone's toes here
                        actor[j].t_data[7] = ROTFIXSPR_MAGIC | pivot; // 'rs' magic + pivot SE sprite index
                        actor[j].t_data[8] = spr->x - sprite[pivot].x;
                        actor[j].t_data[9] = spr->y - sprite[pivot].y;
                    }
                }

                j = nextspritesect[j];
#ifdef YAX_ENABLE
                if (j<0 && firstrun)
                    if (sprite[i].lotag==SE_6_SUBWAY || sprite[i].lotag==SE_14_SUBWAY_CAR)
                    {
                        firstrun = 0;
                        j = actor[i].t_data[9];
                        if (j >= 0)
                            j = headspritesect[j];
                    }
#endif
            }
        }
    }
}

static inline int32_t G_CheckExitSprite(int32_t i)
{
    return (sprite[i].lotag == UINT16_MAX && (sprite[i].cstat&16));
}

static void prelevel(char g)
{
    int32_t i, nexti, j, startwall, endwall;
    int32_t switchpicnum;
    uint8_t *tagbitmap = (uint8_t *)Xcalloc(65536>>3, 1);

    Bmemset(show2dsector, 0, sizeof(show2dsector));
#ifdef LEGACY_ROR
    Bmemset(ror_protectedsectors, 0, MAXSECTORS);
#endif
    resetprestat(0,g);
    g_numClouds = 0;

    G_SetupGlobalPsky();

    VM_OnEvent(EVENT_PRELEVEL, -1, -1);

    int missedCloudSectors = 0;

    for (i=0; i<numsectors; i++)
    {
        sector[i].extra = 256;

        switch (sector[i].lotag)
        {
        case ST_20_CEILING_DOOR:
        case ST_22_SPLITTING_DOOR:
            if (sector[i].floorz > sector[i].ceilingz)
                sector[i].lotag |= 32768;
            continue;
        }

        if (sector[i].ceilingstat&1)
        {
            if (waloff[sector[i].ceilingpicnum] == 0)
            {
                if (sector[i].ceilingpicnum == LA)
                    for (j=0; j<5; j++)
                        tloadtile(sector[i].ceilingpicnum+j, 0);
            }

            if (sector[i].ceilingpicnum == CLOUDYSKIES)
            {
                if (g_numClouds < ARRAY_SSIZE(clouds))
                    clouds[g_numClouds++] = i;
                else
                    missedCloudSectors++;
            }

            if (g_player[0].ps->one_parallax_sectnum == -1)
                g_player[0].ps->one_parallax_sectnum = i;
        }

        if (sector[i].lotag == 32767) //Found a secret room
        {
            g_player[0].ps->max_secret_rooms++;
            continue;
        }

        if (sector[i].lotag == UINT16_MAX)
        {
            g_player[0].ps->exitx = wall[sector[i].wallptr].x;
            g_player[0].ps->exity = wall[sector[i].wallptr].y;
            continue;
        }
    }

    if (missedCloudSectors > 0)
        OSD_Printf(OSDTEXT_RED "Map warning: have %d unhandled CLOUDYSKIES ceilings.\n", missedCloudSectors);

    // NOTE: must be safe loop because callbacks could delete sprites.
    for (SPRITES_OF_STAT_SAFE(STAT_DEFAULT, i, nexti))
    {
        A_ResetVars(i);
#if !defined LUNATIC
        A_LoadActor(i);
#endif
        VM_OnEvent(EVENT_LOADACTOR, i, -1);
        if (G_CheckExitSprite(i))
        {
            g_player[0].ps->exitx = SX;
            g_player[0].ps->exity = SY;
        }
        else switch (DYNAMICTILEMAP(PN))
            {
            case GPSPEED__STATIC:
                // DELETE_AFTER_LOADACTOR. Must not change statnum.
                sector[SECT].extra = SLT;
                break;

            case CYCLER__STATIC:
                // DELETE_AFTER_LOADACTOR. Must not change statnum.
                if (g_numCyclers >= MAXCYCLERS)
                {
                    Bsprintf(tempbuf,"\nToo many cycling sectors (%d max).",MAXCYCLERS);
                    G_GameExit(tempbuf);
                }
                cyclers[g_numCyclers][0] = SECT;
                cyclers[g_numCyclers][1] = SLT;
                cyclers[g_numCyclers][2] = SS;
                cyclers[g_numCyclers][3] = sector[SECT].floorshade;
                cyclers[g_numCyclers][4] = SHT;
                cyclers[g_numCyclers][5] = (SA == 1536);
                g_numCyclers++;
                break;

            case SECTOREFFECTOR__STATIC:
            case ACTIVATOR__STATIC:
            case TOUCHPLATE__STATIC:
            case ACTIVATORLOCKED__STATIC:
            case MUSICANDSFX__STATIC:
            case LOCATORS__STATIC:
            case MASTERSWITCH__STATIC:
            case RESPAWN__STATIC:
                sprite[i].cstat &= ~(1|256);
                break;
            }
    }

    // Delete some effector / effector modifier sprites AFTER the loop running
    // the LOADACTOR events. DELETE_AFTER_LOADACTOR.
    for (SPRITES_OF_STAT_SAFE(STAT_DEFAULT, i, nexti))
        if (!G_CheckExitSprite(i))
            switch (DYNAMICTILEMAP(PN))
            {
            case GPSPEED__STATIC:
            case CYCLER__STATIC:
                A_DeleteSprite(i);
                break;
            }

    for (i = 0; i < MAXSPRITES; i++)
    {
        if (sprite[i].statnum < MAXSTATUS && (PN != SECTOREFFECTOR || SLT != SE_14_SUBWAY_CAR))
            A_Spawn(-1, i);
    }

    for (i = 0; i < MAXSPRITES; i++)
    {
        if (sprite[i].statnum < MAXSTATUS && PN == SECTOREFFECTOR && SLT == SE_14_SUBWAY_CAR)
            A_Spawn(-1, i);
    }

    G_SetupRotfixedSprites();

    for (i=headspritestat[STAT_DEFAULT]; i>=0; i=nextspritestat[i])
    {
        int32_t ii;

        if (PN <= 0)  // oob safety for switch below
            continue;

        for (ii=0; ii<2; ii++)
            switch (DYNAMICTILEMAP(PN-1+ii))
            {
            case DIPSWITCH__STATIC:
            case DIPSWITCH2__STATIC:
            case PULLSWITCH__STATIC:
            case HANDSWITCH__STATIC:
            case SLOTDOOR__STATIC:
            case LIGHTSWITCH__STATIC:
            case SPACELIGHTSWITCH__STATIC:
            case SPACEDOORSWITCH__STATIC:
            case FRANKENSTINESWITCH__STATIC:
            case LIGHTSWITCH2__STATIC:
            case POWERSWITCH1__STATIC:
            case LOCKSWITCH1__STATIC:
            case POWERSWITCH2__STATIC:
                // the lower code only for the 'on' state (*)
                if (ii==0)
                {
                    j = sprite[i].lotag;
                    tagbitmap[j>>3] |= 1<<(j&7);
                }

                break;
            }
    }

    // initially 'on' SE 12 light (*)
    for (j=headspritestat[STAT_EFFECTOR]; j>=0; j=nextspritestat[j])
    {
        int32_t t = sprite[j].hitag;

        if (sprite[j].lotag == SE_12_LIGHT_SWITCH && tagbitmap[t>>3]&(1<<(t&7)))
            actor[j].t_data[0] = 1;
    }

    Bfree(tagbitmap);

    g_mirrorCount = 0;

    for (i = 0; i < numwalls; i++)
    {
        walltype *wal;
        wal = &wall[i];

        if (wal->overpicnum == MIRROR && (wal->cstat&32) != 0)
        {
            j = wal->nextsector;

            if ((j >= 0) && sector[j].ceilingpicnum != MIRROR)
            {
                if (g_mirrorCount > 63)
                    G_GameExit("\nToo many mirrors (64 max.)");

                sector[j].ceilingpicnum = MIRROR;
                sector[j].floorpicnum = MIRROR;
                g_mirrorWall[g_mirrorCount] = i;
                g_mirrorSector[g_mirrorCount] = j;
                g_mirrorCount++;
                continue;
            }
        }

        if (g_numAnimWalls >= MAXANIMWALLS)
        {
            Bsprintf(tempbuf,"\nToo many 'anim' walls (%d max).",MAXANIMWALLS);
            G_GameExit(tempbuf);
        }

        animwall[g_numAnimWalls].tag = 0;
        animwall[g_numAnimWalls].wallnum = 0;

        switchpicnum = G_GetForcefieldPicnum(i);

        if (switchpicnum >= 0)
        {
            switch (DYNAMICTILEMAP(switchpicnum))
            {
            case FANSHADOW__STATIC:
            case FANSPRITE__STATIC:
                wall->cstat |= 65;
                animwall[g_numAnimWalls].wallnum = i;
                g_numAnimWalls++;
                break;

            case W_FORCEFIELD__STATIC:
                if (wal->overpicnum==W_FORCEFIELD__STATIC)
                    for (j=0; j<3; j++)
                        tloadtile(W_FORCEFIELD+j, 0);
                if (wal->shade > 31)
                    wal->cstat = 0;
                else wal->cstat |= FORCEFIELD_CSTAT|256;


                if (wal->lotag && wal->nextwall >= 0)
                    wall[wal->nextwall].lotag =
                        wal->lotag;

            case BIGFORCE__STATIC:
                animwall[g_numAnimWalls].wallnum = i;
                g_numAnimWalls++;

                continue;
            }
        }

        wal->extra = -1;

        switch (DYNAMICTILEMAP(wal->picnum))
        {
        case WATERTILE2__STATIC:
            for (j=0; j<3; j++)
                tloadtile(wal->picnum+j, 0);
            break;

        case TECHLIGHT2__STATIC:
        case TECHLIGHT4__STATIC:
            tloadtile(wal->picnum, 0);
            break;
        case W_TECHWALL1__STATIC:
        case W_TECHWALL2__STATIC:
        case W_TECHWALL3__STATIC:
        case W_TECHWALL4__STATIC:
            animwall[g_numAnimWalls].wallnum = i;
            //                animwall[g_numAnimWalls].tag = -1;
            g_numAnimWalls++;
            break;
        case SCREENBREAK6__STATIC:
        case SCREENBREAK7__STATIC:
        case SCREENBREAK8__STATIC:
            for (j=SCREENBREAK6; j<SCREENBREAK9; j++)
                tloadtile(j, 0);
            animwall[g_numAnimWalls].wallnum = i;
            animwall[g_numAnimWalls].tag = -1;
            g_numAnimWalls++;
            break;

        case FEMPIC1__STATIC:
        case FEMPIC2__STATIC:
        case FEMPIC3__STATIC:
            wal->extra = wal->picnum;
            animwall[g_numAnimWalls].tag = -1;
            if (ud.lockout)
            {
                if (wal->picnum == FEMPIC1)
                    wal->picnum = BLANKSCREEN;
                else wal->picnum = SCREENBREAK6;
            }

            animwall[g_numAnimWalls].wallnum = i;
            animwall[g_numAnimWalls].tag = wal->picnum;
            g_numAnimWalls++;
            break;

        case SCREENBREAK1__STATIC:
        case SCREENBREAK2__STATIC:
        case SCREENBREAK3__STATIC:
        case SCREENBREAK4__STATIC:
        case SCREENBREAK5__STATIC:
            //
        case SCREENBREAK9__STATIC:
        case SCREENBREAK10__STATIC:
        case SCREENBREAK11__STATIC:
        case SCREENBREAK12__STATIC:
        case SCREENBREAK13__STATIC:
        case SCREENBREAK14__STATIC:
        case SCREENBREAK15__STATIC:
        case SCREENBREAK16__STATIC:
        case SCREENBREAK17__STATIC:
        case SCREENBREAK18__STATIC:
        case SCREENBREAK19__STATIC:
            animwall[g_numAnimWalls].wallnum = i;
            animwall[g_numAnimWalls].tag = wal->picnum;
            g_numAnimWalls++;
            break;
        }
    }

    //Invalidate textures in sector behind mirror
    for (i=0; i<g_mirrorCount; i++)
    {
        startwall = sector[g_mirrorSector[i]].wallptr;
        endwall = startwall + sector[g_mirrorSector[i]].wallnum;
        for (j=startwall; j<endwall; j++)
        {
            wall[j].picnum = MIRROR;
            wall[j].overpicnum = MIRROR;
            if (wall[g_mirrorWall[i]].pal == 4)
                wall[j].pal = 4;
        }
    }
}


void G_NewGame(int32_t vn, int32_t ln, int32_t sk)
{
    DukePlayer_t *p = g_player[0].ps;
    int32_t i;

    G_HandleAsync();

    if (g_skillSoundVoice >= 0 && ud.config.SoundToggle)
    {
        while (FX_SoundActive(g_skillSoundVoice))
            G_HandleAsync();
    }

    g_skillSoundVoice = -1;

    ready2send = 0;

    if (ud.m_recstat != 2 && ud.last_level >= 0 && (g_netServer || ud.multimode > 1) && (ud.coop&GAMETYPE_SCORESHEET))
        G_BonusScreen(1);

    if (ln == 0 && vn == 3 && (!g_netServer && ud.multimode < 2) && ud.lockout == 0
            && (G_GetLogoFlags() & LOGO_NOE4CUTSCENE)==0)
    {
        S_PlayMusic(MapInfo[MUS_BRIEFING].musicfn);

        flushperms();
        setview(0,0,xdim-1,ydim-1);
        clearview(0L);
        nextpage();

        i = G_PlayAnim("vol41a.anm");
        clearview(0L);
        nextpage();
        if (i)
            goto end_vol4a;

        i = G_PlayAnim("vol42a.anm");
        clearview(0L);
        nextpage();
        if (i)
            goto end_vol4a;

        G_PlayAnim("vol43a.anm");
        clearview(0L);
        nextpage();

end_vol4a:
        FX_StopAllSounds();
    }

    g_showShareware = GAMETICSPERSEC*34;

    ud.level_number = ln;
    ud.volume_number = vn;
    ud.player_skill = sk;
    ud.secretlevel = 0;
    ud.from_bonus = 0;

    ud.last_level = -1;
    g_lastSaveSlot = -1;

#ifdef EDUKE32_TOUCH_DEVICES
    p->zoom = 360;
#else
    p->zoom = 768;
#endif
    p->gm = 0;
    M_CloseMenu(0);

#if !defined LUNATIC
    //AddLog("Newgame");
    Gv_ResetVars();

    Gv_InitWeaponPointers();

    // PK: Gv_ResetVars() might trip up the system (pointer) gamevars,
    // e.g. if some earlier-version CON code had been loaded before
    Gv_RefreshPointers();
#endif
    Gv_ResetSystemDefaults();

    for (i=0; i<(MAXVOLUMES*MAXLEVELS); i++)
        ALIGNED_FREE_AND_NULL(MapInfo[i].savedstate);

    if (ud.m_coop != 1)
    {
        for (i=0; i<MAX_WEAPONS; i++)
        {
            if (PWEAPON(0, i, WorksLike)==PISTOL_WEAPON)
            {
                p->curr_weapon = i;
                p->gotweapon |= (1<<i);
                p->ammo_amount[i] = min(p->max_ammo_amount[i], 48);
            }
            else if (PWEAPON(0, i, WorksLike)==KNEE_WEAPON)
                p->gotweapon |= (1<<i);
            else if (PWEAPON(0, i, WorksLike)==HANDREMOTE_WEAPON)
                p->gotweapon |= (1<<i);
        }
        p->last_weapon = -1;
    }

    display_mirror = 0;

#ifdef LUNATIC
    // NOTE: Lunatic state creation is relatively early. No map has yet been loaded.
    // XXX: What about the cases where G_EnterLevel() is called without a preceding G_NewGame()?
    El_CreateGameState();
    G_PostCreateGameState();
#endif
    VM_OnEvent(EVENT_NEWGAME, g_player[myconnectindex].ps->i, myconnectindex);
}

static void resetpspritevars(char g)
{
    int16_t i, j; //circ;

    uint8_t aimmode[MAXPLAYERS],autoaim[MAXPLAYERS],weaponswitch[MAXPLAYERS];
    DukeStatus_t tsbar[MAXPLAYERS];

    if (g_player[0].ps->cursectnum >= 0)  // < 0 may happen if we start a map in void space (e.g. testing it)
    {
        A_InsertSprite(g_player[0].ps->cursectnum,g_player[0].ps->pos.x,g_player[0].ps->pos.y,g_player[0].ps->pos.z,
                       APLAYER,0,0,0,g_player[0].ps->ang,0,0,0,10);
    }

    if (ud.recstat != 2)
        for (TRAVERSE_CONNECT(i))
        {
            aimmode[i] = g_player[i].ps->aim_mode;
            autoaim[i] = g_player[i].ps->auto_aim;
            weaponswitch[i] = g_player[i].ps->weaponswitch;
            if ((g_netServer || ud.multimode > 1) && (GametypeFlags[ud.coop]&GAMETYPE_PRESERVEINVENTORYDEATH) && ud.last_level >= 0)
            {
                for (j=0; j<MAX_WEAPONS; j++)
                    tsbar[i].ammo_amount[j] = g_player[i].ps->ammo_amount[j];

                tsbar[i].gotweapon = g_player[i].ps->gotweapon;
                Bmemcpy(tsbar[i].inv_amount, g_player[i].ps->inv_amount, sizeof(tsbar[i].inv_amount));
                tsbar[i].curr_weapon = g_player[i].ps->curr_weapon;
                tsbar[i].inven_icon = g_player[i].ps->inven_icon;
            }
        }

    P_ResetStatus(0);

    for (TRAVERSE_CONNECT(i))
        if (i) Bmemcpy(g_player[i].ps,g_player[0].ps,sizeof(DukePlayer_t));

    if (ud.recstat != 2)
        for (TRAVERSE_CONNECT(i))
        {
            g_player[i].ps->aim_mode = aimmode[i];
            g_player[i].ps->auto_aim = autoaim[i];
            g_player[i].ps->weaponswitch = weaponswitch[i];
            if ((g_netServer || ud.multimode > 1) && (GametypeFlags[ud.coop]&GAMETYPE_PRESERVEINVENTORYDEATH) && ud.last_level >= 0)
            {
                for (j=0; j<MAX_WEAPONS; j++)
                    g_player[i].ps->ammo_amount[j] = tsbar[i].ammo_amount[j];

                g_player[i].ps->gotweapon = tsbar[i].gotweapon;
                g_player[i].ps->curr_weapon = tsbar[i].curr_weapon;
                g_player[i].ps->inven_icon = tsbar[i].inven_icon;
                Bmemcpy(g_player[i].ps->inv_amount, tsbar[i].inv_amount, sizeof(tsbar[i].inv_amount));
            }
        }

    g_numPlayerSprites = 0;
//    circ = 2048/ud.multimode;

    g_whichPalForPlayer = 9;
    j = 0;
    i = headspritestat[STAT_PLAYER];
    while (i >= 0)
    {
        const int32_t nexti = nextspritestat[i];
        spritetype *const s = &sprite[i];

        if (g_numPlayerSprites == MAXPLAYERS)
            G_GameExit("\nToo many player sprites (max 16.)");

        g_playerSpawnPoints[g_numPlayerSprites].pos.x = s->x;
        g_playerSpawnPoints[g_numPlayerSprites].pos.y = s->y;
        g_playerSpawnPoints[g_numPlayerSprites].pos.z = s->z;
        g_playerSpawnPoints[g_numPlayerSprites].ang = s->ang;
        g_playerSpawnPoints[g_numPlayerSprites].sect = s->sectnum;

        g_numPlayerSprites++;

        if (j < MAXPLAYERS)
        {
            s->owner = i;
            s->shade = 0;
            s->xrepeat = 42;
            s->yrepeat = 36;
            if (!g_fakeMultiMode)
                s->cstat = j < numplayers ? 1+256 : 32768;
            else
                s->cstat = j < ud.multimode ? 1+256 : 32768;
            s->xoffset = 0;
            s->clipdist = 64;

//            if (j < playerswhenstarted)
            {
                if ((g&MODE_EOL) != MODE_EOL || g_player[j].ps->last_extra == 0)
                {
                    g_player[j].ps->last_extra = g_player[j].ps->max_player_health;
                    s->extra = g_player[j].ps->max_player_health;
                    g_player[j].ps->runspeed = g_playerFriction;
                }
                else s->extra = g_player[j].ps->last_extra;

                s->yvel = j;

                if (!g_player[j].pcolor && (g_netServer || ud.multimode > 1) && !(GametypeFlags[ud.coop] & GAMETYPE_TDM))
                {
                    if (s->pal == 0)
                    {
                        int32_t k = 0;

                        for (; k<ud.multimode; k++)
                        {
                            if (g_whichPalForPlayer == g_player[k].ps->palookup)
                            {
                                g_whichPalForPlayer++;
                                if (g_whichPalForPlayer >= 17)
                                    g_whichPalForPlayer = 9;
                                k=0;
                            }
                        }
                        g_player[j].pcolor = s->pal = g_player[j].ps->palookup = g_whichPalForPlayer++;
                        if (g_whichPalForPlayer >= 17)
                            g_whichPalForPlayer = 9;
                    }
                    else g_player[j].pcolor = g_player[j].ps->palookup = s->pal;
                }
                else
                {
                    int32_t k = g_player[j].pcolor;

                    if (GametypeFlags[ud.coop] & GAMETYPE_TDM)
                    {
                        k = G_GetTeamPalette(g_player[j].pteam);
                        g_player[j].ps->team = g_player[j].pteam;
                    }
                    s->pal = g_player[j].ps->palookup = k;
                }

                g_player[j].ps->i = i;
                g_player[j].ps->frag_ps = j;
                actor[i].owner = i;

                g_player[j].ps->autostep = (20L<<8);
                g_player[j].ps->autostep_sbw = (4L<<8);

                actor[i].bpos.x = g_player[j].ps->bobpos.x = g_player[j].ps->opos.x = g_player[j].ps->pos.x =        s->x;
                actor[i].bpos.y = g_player[j].ps->bobpos.y = g_player[j].ps->opos.y = g_player[j].ps->pos.y =        s->y;
                actor[i].bpos.z = g_player[j].ps->opos.z = g_player[j].ps->pos.z =        s->z;
                g_player[j].ps->oang  = g_player[j].ps->ang  =        s->ang;

                updatesector(s->x,s->y,&g_player[j].ps->cursectnum);
            }

            j++;
        }
        else A_DeleteSprite(i);

        i = nexti;
    }
}

static inline void clearfrags(void)
{
    int32_t i;

    for (i=0; i<ud.multimode; i++)
    {
        playerdata_t *p = &g_player[i];

        p->ps->frag = p->ps->fraggedself = 0;
        Bmemset(p->frags, 0, sizeof(p->frags));
    }
}

void G_ResetTimers(uint8_t keepgtics)
{
    totalclock = cloudtotalclock = ototalclock = lockclock = 0;
    ready2send = 1;
    g_levelTextTime = 85;

    if (!keepgtics)
        g_moveThingsCount = 0;

    if (g_curViewscreen >= 0)
        actor[g_curViewscreen].t_data[0] = 0;
}

void G_ClearFIFO(void)
{
    int32_t i = MAXPLAYERS-1;
    
    g_emuJumpTics = 0;

    Bmemset(&avg, 0, sizeof(input_t));

    clearbufbyte(&loc,sizeof(input_t),0L);
    clearbufbyte(&inputfifo,sizeof(input_t)*MOVEFIFOSIZ*MAXPLAYERS,0L);

    for (; i >= 0; i--)
    {
        if (g_player[i].sync != NULL)
            Bmemset(g_player[i].sync, 0, sizeof(input_t));
        g_player[i].vote = g_player[i].gotvote = 0;
    }
}

int32_t G_FindLevelByFile(const char *fn)
{
    for (int volume = 0; volume < MAXVOLUMES; volume++)
    {
        const int voloff = volume * MAXLEVELS;
        for (int level = 0; level < MAXLEVELS; level++)
        {
            if (MapInfo[voloff + level].filename == NULL)
                continue;

            if (!Bstrcasecmp(fn, MapInfo[voloff + level].filename))
                return voloff + level;
        }
    }

    return MAXLEVELS * MAXVOLUMES;
}

static void G_FadeLoad(int32_t r, int32_t g, int32_t b, int32_t start, int32_t end, int32_t step, int32_t ticwait, int32_t tc)
{
    int32_t m = (step < 0) ? -1 : 1;

    int32_t nexttic = totalclock;

    for (; m*start <= m*end; start += step)
    {
        while (totalclock < nexttic)
            sampletimer();
        nexttic += ticwait;

        if (KB_KeyPressed(sc_Space))
        {
            KB_ClearKeyDown(sc_Space);
            return;
        }

        setpalettefade(r,g,b,start);
        flushperms();
        G_DoLoadScreen(" ", tc);
    }
}

static int32_t G_TryMapHack(const char *mhkfile)
{
    int32_t retval = loadmaphack(mhkfile);

    if (!retval)
        initprintf("Loaded map hack file \"%s\"\n", mhkfile);

    return retval;
}

static void G_LoadMapHack(char *outbuf, const char *filename)
{
    if (filename != NULL)
        Bstrcpy(outbuf, filename);

    append_ext_UNSAFE(outbuf, ".mhk");

    if (G_TryMapHack(outbuf))
    {
        usermaphack_t *mapinfo = (usermaphack_t*)bsearch(
            &g_loadedMapHack, usermaphacks, num_usermaphacks, sizeof(usermaphack_t),
            compare_usermaphacks);

        if (mapinfo)
            G_TryMapHack(mapinfo->mhkfile);
    }
}

// levnamebuf should have at least size BMAX_PATH
void G_SetupFilenameBasedMusic(char *levnamebuf, const char *boardfilename, int32_t level_number)
{
    char *p, *exts[] = {
#ifdef HAVE_FLAC
                 "flac",
#endif
#ifdef HAVE_VORBIS
                 "ogg",
#endif
                 "mid"
             };

    Bstrncpy(levnamebuf, boardfilename, BMAX_PATH);

    Bcorrectfilename(levnamebuf, 0);

    if (NULL == (p = Bstrrchr(levnamebuf, '.')))
    {
        p = levnamebuf + Bstrlen(levnamebuf);
        p[0] = '.';
    }

    for (unsigned int i = 0; i < ARRAY_SIZE(exts); i++)
    {
        int32_t fil;

        Bmemcpy(p+1, exts[i], Bstrlen(exts[i]) + 1);

        if ((fil = kopen4loadfrommod(levnamebuf, 0)) != -1)
        {
            kclose(fil);
            realloc_copy(&MapInfo[level_number].musicfn, levnamebuf);
            return;
        }
    }

    realloc_copy(&MapInfo[level_number].musicfn, "dethtoll.mid");
}

static inline int G_HaveUserMap(void)
{
    return (boardfilename[0] != 0 && ud.m_level_number == 7 && ud.m_volume_number == 0);
}

int32_t G_EnterLevel(int32_t g)
{
    int32_t i, mii;
    char levname[BMAX_PATH];

//    flushpackets();
//    waitforeverybody();
    vote_map = vote_episode = voting = -1;

    ud.respawn_monsters = ud.m_respawn_monsters;
    ud.respawn_items    = ud.m_respawn_items;
    ud.respawn_inventory    = ud.m_respawn_inventory;
    ud.monsters_off = ud.m_monsters_off;
    ud.coop = ud.m_coop;
    ud.marker = ud.m_marker;
    ud.ffire = ud.m_ffire;
    ud.noexits = ud.m_noexits;

    if ((g&MODE_DEMO) != MODE_DEMO)
        ud.recstat = ud.m_recstat;
    if ((g&MODE_DEMO) == 0 && ud.recstat == 2)
        ud.recstat = 0;

    if (g_networkMode != NET_DEDICATED_SERVER)
    {
        FX_StopAllSounds();
        S_ClearSoundLocks();
        FX_SetReverb(0);
        setgamemode(ud.config.ScreenMode,ud.config.ScreenWidth,ud.config.ScreenHeight,ud.config.ScreenBPP);
    }	       

    if (G_HaveUserMap())
    {
        int32_t volume, level;

        Bcorrectfilename(boardfilename,0);

        volume = level = G_FindLevelByFile(boardfilename);

        if (level != MAXLEVELS*MAXVOLUMES)
        {
            level &= MAXLEVELS-1;
            volume = (volume - level) / MAXLEVELS;

            ud.level_number = ud.m_level_number = level;
            ud.volume_number = ud.m_volume_number = volume;
            boardfilename[0] = 0;
        }
    }

    mii = (ud.volume_number*MAXLEVELS)+ud.level_number;

    if (MapInfo[mii].name == NULL || MapInfo[mii].filename == NULL)
    {
        if (G_HaveUserMap())
        {
            if (MapInfo[mii].filename == NULL)
                MapInfo[mii].filename = (char *)Xcalloc(BMAX_PATH, sizeof(uint8_t));
            if (MapInfo[mii].name == NULL)
                MapInfo[mii].name = Xstrdup("User Map");
        }
        else
        {
            OSD_Printf(OSDTEXT_RED "Map E%dL%d not defined!\n", ud.volume_number+1, ud.level_number+1);
            return 1;
        }
    }

    i = ud.screen_size;
    ud.screen_size = 0;

    G_DoLoadScreen("Loading map . . .", -1);
    G_UpdateScreenArea();

    ud.screen_size = i;

    if (G_HaveUserMap())
    {
        Bstrcpy(levname, boardfilename);
        if (g_gameNamePtr)
            Bsprintf(apptitle,"%s - %s - " APPNAME,levname,g_gameNamePtr);
        else
            Bsprintf(apptitle,"%s - " APPNAME,levname);
    }
    else
    {
        if (g_gameNamePtr)
            Bsprintf(apptitle,"%s - %s - " APPNAME,MapInfo[mii].name,g_gameNamePtr);
        else
            Bsprintf(apptitle,"%s - " APPNAME,MapInfo[mii].name);
    }

    Bstrcpy(tempbuf,apptitle);
    wm_setapptitle(tempbuf);

    /***** Load the map *****/
    {
        DukePlayer_t *ps = g_player[0].ps;

        if (!VOLUMEONE && G_HaveUserMap())
        {
            if (loadboard(boardfilename, 0, &ps->pos, &ps->ang, &ps->cursectnum) < 0)
            {
                OSD_Printf(OSD_ERROR "Map \"%s\" not found or invalid map version!\n",boardfilename);
                return 1;
            }

            G_LoadMapHack(levname, boardfilename);
            G_SetupFilenameBasedMusic(levname, boardfilename, ud.m_level_number);
        }
        else if (loadboard(MapInfo[mii].filename, VOLUMEONE, &ps->pos, &ps->ang, &ps->cursectnum) < 0)
        {
            OSD_Printf(OSD_ERROR "Map \"%s\" not found or invalid map version!\n",
                       MapInfo[mii].filename);
            return 1;
        }
        else
        {
            G_LoadMapHack(levname, MapInfo[mii].filename);
        }
    }

    g_precacheCount = 0;
    Bmemset(gotpic, 0, sizeof(gotpic));
    Bmemset(precachehightile, 0, sizeof(precachehightile));

    //clearbufbyte(Actor,sizeof(Actor),0l); // JBF 20040531: yes? no?

    prelevel(g);

    G_AlignWarpElevators();
    resetpspritevars(g);

    ud.playerbest = CONFIG_GetMapBestTime(MapInfo[mii].filename);

    G_FadeLoad(0,0,0, 252,0, -28, 4, -1);
    G_CacheMapData();
    G_FadeLoad(0,0,0, 0,252, 28, 4, -2);

    if (ud.recstat != 2)
    {
        g_musicIndex = mii;
        if (MapInfo[g_musicIndex].musicfn != NULL)
            S_PlayMusic(MapInfo[g_musicIndex].musicfn);
    }

    if (g & (MODE_GAME|MODE_EOL))
    {
        for (TRAVERSE_CONNECT(i))
        {
            g_player[i].ps->gm = MODE_GAME;
            M_CloseMenu(i);
        }
    }
    else if (g & MODE_RESTART)
    {
        if (ud.recstat == 2)
            g_player[myconnectindex].ps->gm = MODE_DEMO;
        else g_player[myconnectindex].ps->gm = MODE_GAME;
    }

    if ((ud.recstat == 1) && (g&MODE_RESTART) != MODE_RESTART)
        G_OpenDemoWrite();

#ifndef EDUKE32_TOUCH_DEVICES
    if (VOLUMEONE && ud.level_number == 0 && ud.recstat != 2)
        P_DoQuote(QUOTE_F1HELP,g_player[myconnectindex].ps);
#endif

    for (TRAVERSE_CONNECT(i))
        switch (DYNAMICTILEMAP(sector[sprite[g_player[i].ps->i].sectnum].floorpicnum))
        {
        case HURTRAIL__STATIC:
        case FLOORSLIME__STATIC:
        case FLOORPLASMA__STATIC:
            P_ResetWeapons(i);
            P_ResetInventory(i);
            g_player[i].ps->gotweapon &= ~(1<<PISTOL_WEAPON);
            g_player[i].ps->ammo_amount[PISTOL_WEAPON] = 0;
            g_player[i].ps->curr_weapon = KNEE_WEAPON;
            g_player[i].ps->kickback_pic = 0;
            break;
        }

    //PREMAP.C - replace near the my's at the end of the file

    Net_NotifyNewGame();
    Net_ResetPrediction();

    //g_player[myconnectindex].ps->palette = palette;
    //G_FadePalette(0,0,0,0);
    P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308
    P_UpdateScreenPal(g_player[myconnectindex].ps);
    flushperms();

    everyothertime = 0;
    g_globalRandom = 0;

    ud.last_level = ud.level_number+1;

    G_ClearFIFO();

    for (i=g_numInterpolations-1; i>=0; i--) bakipos[i] = *curipos[i];

    g_restorePalette = -1;

    G_UpdateScreenArea();
    clearview(0L);
    G_DrawBackground();
    G_DrawRooms(myconnectindex,65536);

    g_player[myconnectindex].ps->over_shoulder_on = 0;

    clearfrags();

    G_ResetTimers(0);  // Here we go

    //Bsprintf(g_szBuf,"G_EnterLevel L=%d V=%d",ud.level_number, ud.volume_number);
    //AddLog(g_szBuf);
    // variables are set by pointer...

    Bmemcpy(currentboardfilename, boardfilename, BMAX_PATH);

    for (TRAVERSE_CONNECT(i))
    {
        const int32_t ret = VM_OnEventWithReturn(EVENT_ENTERLEVEL, g_player[i].ps->i, i, 0);
        if (ret == 0)
            break;
    }

    OSD_Printf(OSDTEXT_YELLOW "E%dL%d: %s\n", ud.volume_number+1, ud.level_number+1,
               MapInfo[mii].name);

    Net_WaitForServer();
    return 0;
}

void G_FreeMapState(int32_t mapnum)
{
    map_t *mapinfo = &MapInfo[mapnum];
#if !defined LUNATIC
    int32_t j;
#endif
    if (mapinfo->savedstate == NULL)
        return;

#if !defined LUNATIC
    for (j=0; j<g_gameVarCount; j++)
    {
        if (aGameVars[j].dwFlags & GAMEVAR_NORESET) continue;
        if (aGameVars[j].dwFlags & (GAMEVAR_PERPLAYER|GAMEVAR_PERACTOR))
            Baligned_free(mapinfo->savedstate->vars[j]);
    }
#else
    Bfree(mapinfo->savedstate->savecode);
#endif
    Baligned_free(mapinfo->savedstate);
    mapinfo->savedstate = NULL;
}
