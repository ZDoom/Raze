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

#include "duke3d.h"
#include "anim.h"
#include "menus.h"
#include "demo.h"
#include "savegame.h"

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

    for (j = PN(i); j <= g_tile[PN(i)].cacherange; j++)
        tloadtile(j,1);

    switch (DYNAMICTILEMAP(PN(i)))
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

    for (j = PN(i); j < (PN(i)+maxc); j++) tloadtile(j,1);
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
    if (num >= MAXSOUNDS || !ud.config.SoundToggle) return 0;

    if (EDUKE32_PREDICT_FALSE(!g_sounds[num].filename)) return 0;

    int32_t fp = S_OpenAudio(g_sounds[num].filename, g_loadFromGroupOnly, 0);
    if (EDUKE32_PREDICT_FALSE(fp == -1))
    {
//        OSD_Printf(OSDTEXT_RED "Sound %s(#%d) not found!\n",g_sounds[num].filename,num);
        return 0;
    }

    int32_t l = kfilelength(fp);
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
        j = VM_OnEventWithReturn(EVENT_GETLOADTILE, g_player[screenpeek].ps->i, screenpeek, LOADSCREEN);

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
            menutext_center(90,"Loading User Map");
            gametext_center_shade_pal(90+10, boardfilename, 14, 2);
        }
        else
        {
            menutext_center(90,"Loading");
            if (g_mapInfo[(ud.volume_number*MAXLEVELS) + ud.level_number].name != NULL)
                menutext_center(90+16+8,g_mapInfo[(ud.volume_number*MAXLEVELS) + ud.level_number].name);
        }

#ifndef EDUKE32_TOUCH_DEVICES
        if (statustext) gametext_center_number(180, statustext);
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

        j = VM_OnEventWithReturn(EVENT_GETLOADTILE, g_player[screenpeek].ps->i, screenpeek, LOADSCREEN);

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

        menutext_center(105,"Loading...");
        if (statustext) gametext_center_number(180, statustext);
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

    if (g_mapInfo[MUS_LOADING].musicfn)
    {
        S_PlayMusic(g_mapInfo[MUS_LOADING].musicfn);
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

#ifdef USE_GLEXT
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

        int32_t x1 = scale(ss,xdim,160);
        int32_t x2 = xdim-x1;

        int32_t y1 = ss;
        int32_t y2 = 200-ss;

        if (ud.screen_size > 0 && (g_gametypeFlags[ud.coop]&GAMETYPE_FRAGBAR) && (g_netServer || ud.multimode > 1))
        {
            int32_t i, j = 0;

            for (TRAVERSE_CONNECT(i))
                if (i > j) j = i;

            if (j > 0) y1 += 8;
            if (j > 4) y1 += 8;
            if (j > 8) y1 += 8;
            if (j > 12) y1 += 8;
        }

        y2 *= 100;
        if (ud.screen_size >= 8 && ud.statusbarmode==0)
            y2 -= tilesiz[BOTTOMSTATUSBAR].y*ud.statusbarscale;

        y1 = scale(y1,ydim,200);
        y2 = scale(y2,ydim,200*100);

        if (VM_HaveEvent(EVENT_UPDATESCREENAREA))
        {
            ud.screenarea_x1 = x1;
            ud.screenarea_y1 = y1;
            ud.screenarea_x2 = x2;
            ud.screenarea_y2 = y2;
            VM_OnEvent(EVENT_UPDATESCREENAREA, g_player[screenpeek].ps->i, screenpeek);
            x1 = ud.screenarea_x1;
            y1 = ud.screenarea_y1;
            x2 = ud.screenarea_x2;
            y2 = ud.screenarea_y2;
        }

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

void P_RandomSpawnPoint(int playerNum)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

    int32_t  i = playerNum;
    uint32_t dist;
    uint32_t pdist = -1;

    if ((g_netServer || ud.multimode > 1) && !(g_gametypeFlags[ud.coop] & GAMETYPE_FIXEDRESPAWN))
    {
        i = krand() % g_playerSpawnCnt;

        if (g_gametypeFlags[ud.coop] & GAMETYPE_TDMSPAWN)
        {
            for (bssize_t j=0; j<ud.multimode; j++)
            {
                if (j != playerNum && g_player[j].ps->team == pPlayer->team && sprite[g_player[j].ps->i].extra > 0)
                {
                    for (bssize_t k=0; k<g_playerSpawnCnt; k++)
                    {
                        dist = FindDistance2D(g_player[j].ps->pos.x - g_playerSpawnPoints[k].pos.x,
                                              g_player[j].ps->pos.y - g_playerSpawnPoints[k].pos.y);

                        if (dist < pdist)
                            i = k, pdist = dist;
                    }
                    break;
                }
            }
        }
    }

    pPlayer->pos        = g_playerSpawnPoints[i].pos;
    pPlayer->opos       = pPlayer->pos;
    pPlayer->bobpos     = *(vec2_t *)&pPlayer->pos;
    pPlayer->ang        = g_playerSpawnPoints[i].ang;
    pPlayer->cursectnum = g_playerSpawnPoints[i].sect;

    sprite[pPlayer->i].cstat = 1 + 256;
}

static inline void P_ResetTintFade(DukePlayer_t *const pPlayer)
{
    pPlayer->pals.f = 0;
#ifdef LUNATIC
    pPlayer->palsfadeprio = 0;
#endif
}

void P_ResetPlayer(int playerNum)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;
    spritetype *const   pSprite = &sprite[pPlayer->i];
    vec3_t              tmpvect = pPlayer->pos;

    tmpvect.z += PHEIGHT;

    P_RandomSpawnPoint(playerNum);

    pPlayer->opos          = pPlayer->pos;
    pPlayer->bobpos        = *(vec2_t *)&pPlayer->pos;
    actor[pPlayer->i].bpos = pPlayer->pos;
    *(vec3_t *)pSprite     = pPlayer->pos;

    updatesector(pPlayer->pos.x, pPlayer->pos.y, &pPlayer->cursectnum);
    setsprite(pPlayer->i, &tmpvect);

    pSprite->cstat    = 257;
    pSprite->shade    = -12;
    pSprite->clipdist = 64;
    pSprite->xrepeat  = 42;
    pSprite->yrepeat  = 36;
    pSprite->owner    = pPlayer->i;
    pSprite->xoffset  = 0;
    pSprite->pal      = pPlayer->palookup;

    pPlayer->last_extra = pSprite->extra = pPlayer->max_player_health;

    pPlayer->wantweaponfire         = -1;
    pPlayer->horiz                  = 100;
    pPlayer->on_crane               = -1;
    pPlayer->frag_ps                = playerNum;
    pPlayer->horizoff               = 0;
    pPlayer->opyoff                 = 0;
    pPlayer->wackedbyactor          = -1;
    pPlayer->inv_amount[GET_SHIELD] = g_startArmorAmount;
    pPlayer->dead_flag              = 0;
    pPlayer->footprintcount         = 0;
    pPlayer->weapreccnt             = 0;
    pPlayer->fta                    = 0;
    pPlayer->ftq                    = 0;
    pPlayer->vel.x = pPlayer->vel.y = 0;
    pPlayer->rotscrnang             = 0;
    pPlayer->runspeed               = g_playerFriction;
    pPlayer->falling_counter        = 0;

    P_ResetTintFade(pPlayer);

    actor[pPlayer->i].extra        = -1;
    actor[pPlayer->i].owner        = pPlayer->i;
    actor[pPlayer->i].cgg          = 0;
    actor[pPlayer->i].movflag      = 0;
    actor[pPlayer->i].tempang      = 0;
    actor[pPlayer->i].actorstayput = -1;
    actor[pPlayer->i].dispicnum    = 0;
    actor[pPlayer->i].owner        = pPlayer->i;
    actor[pPlayer->i].t_data[4]    = 0;

    P_ResetInventory(playerNum);
    P_ResetWeapons(playerNum);

    pPlayer->reloading     = 0;
    pPlayer->movement_lock = 0;

    VM_OnEvent(EVENT_RESETPLAYER, pPlayer->i, playerNum);
}

void P_ResetStatus(int playerNum)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

    ud.show_help               = 0;
    ud.showallmap              = 0;
    pPlayer->dead_flag         = 0;
    pPlayer->wackedbyactor     = -1;
    pPlayer->falling_counter   = 0;
    pPlayer->quick_kick        = 0;
    pPlayer->subweapon         = 0;
    pPlayer->last_full_weapon  = 0;
    pPlayer->ftq               = 0;
    pPlayer->fta               = 0;
    pPlayer->tipincs           = 0;
    pPlayer->buttonpalette     = 0;
    pPlayer->actorsqu          = -1;
    pPlayer->invdisptime       = 0;
    pPlayer->refresh_inventory = 0;
    pPlayer->last_pissed_time  = 0;
    pPlayer->holster_weapon    = 0;
    pPlayer->pycount           = 0;
    pPlayer->pyoff             = 0;
    pPlayer->opyoff            = 0;
    pPlayer->loogcnt           = 0;
    pPlayer->angvel            = 0;
    pPlayer->weapon_sway       = 0;
    pPlayer->extra_extra8      = 0;
    pPlayer->show_empty_weapon = 0;
    pPlayer->dummyplayersprite = -1;
    pPlayer->crack_time        = 0;
    pPlayer->hbomb_hold_delay  = 0;
    pPlayer->transporter_hold  = 0;
    pPlayer->clipdist          = 164;
    pPlayer->wantweaponfire    = -1;
    pPlayer->hurt_delay        = 0;
    pPlayer->footprintcount    = 0;
    pPlayer->footprintpal      = 0;
    pPlayer->footprintshade    = 0;
    pPlayer->jumping_toggle    = 0;
    pPlayer->ohoriz            = 140;
    pPlayer->horiz             = 140;
    pPlayer->horizoff          = 0;
    pPlayer->bobcounter        = 0;
    pPlayer->on_ground         = 0;
    pPlayer->player_par        = 0;
    pPlayer->return_to_center  = 9;
    pPlayer->airleft           = 15 * GAMETICSPERSEC;
    pPlayer->rapid_fire_hold   = 0;
    pPlayer->toggle_key_flag   = 0;
    pPlayer->access_spritenum  = -1;
    pPlayer->got_access        = ((g_netServer || ud.multimode > 1) && (g_gametypeFlags[ud.coop] & GAMETYPE_ACCESSATSTART)) ? 7 : 0;
    pPlayer->random_club_frame = 0;
    pus                        = 1;
    pPlayer->on_warping_sector = 0;
    pPlayer->spritebridge      = 0;
    pPlayer->sbs               = 0;
    pPlayer->palette           = BASEPAL;

    if (pPlayer->inv_amount[GET_STEROIDS] < 400)
    {
        pPlayer->inv_amount[GET_STEROIDS] = 0;
        pPlayer->inven_icon = ICON_NONE;
    }

    pPlayer->heat_on           = 0;
    pPlayer->jetpack_on        = 0;
    pPlayer->holoduke_on       = -1;
    pPlayer->look_ang          = 512 - ((ud.level_number & 1) << 10);
    pPlayer->rotscrnang        = 0;
    pPlayer->orotscrnang       = 1;  // JBF 20031220
    pPlayer->newowner          = -1;
    pPlayer->jumping_counter   = 0;
    pPlayer->hard_landing      = 0;
    pPlayer->vel.x             = 0;
    pPlayer->vel.y             = 0;
    pPlayer->vel.z             = 0;
    pPlayer->fric.x            = 0;
    pPlayer->fric.y            = 0;
    pPlayer->somethingonplayer = -1;
    pPlayer->one_eighty_count  = 0;
    pPlayer->cheat_phase       = 0;
    pPlayer->on_crane          = -1;

    pPlayer->kickback_pic = ((PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == PISTOL_WEAPON)
                             && (PWEAPON(playerNum, pPlayer->curr_weapon, Reload) > PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime)))
                            ? PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime)
                            : 0;

    pPlayer->weapon_pos         = WEAPON_POS_START;
    pPlayer->walking_snd_toggle = 0;
    pPlayer->weapon_ang         = 0;
    pPlayer->knuckle_incs       = 1;
    pPlayer->fist_incs          = 0;
    pPlayer->knee_incs          = 0;
    pPlayer->jetpack_on         = 0;
    pPlayer->reloading          = 0;
    pPlayer->movement_lock      = 0;
    pPlayer->frag_ps            = playerNum;

    P_UpdateScreenPal(pPlayer);
    VM_OnEvent(EVENT_RESETPLAYER, pPlayer->i, playerNum);
}

void P_ResetWeapons(int playerNum)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

    for (bssize_t weaponNum = PISTOL_WEAPON; weaponNum < MAX_WEAPONS; weaponNum++)
        pPlayer->ammo_amount[weaponNum] = 0;

    pPlayer->weapon_pos                 = WEAPON_POS_START;
    pPlayer->curr_weapon                = PISTOL_WEAPON;
    pPlayer->kickback_pic               = PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime);
    pPlayer->gotweapon                  = ((1 << PISTOL_WEAPON) | (1 << KNEE_WEAPON) | (1 << HANDREMOTE_WEAPON));
    pPlayer->ammo_amount[PISTOL_WEAPON] = min(pPlayer->max_ammo_amount[PISTOL_WEAPON], 48);
    pPlayer->last_weapon                = -1;
    pPlayer->show_empty_weapon          = 0;
    pPlayer->last_pissed_time           = 0;
    pPlayer->holster_weapon             = 0;

    VM_OnEvent(EVENT_RESETWEAPONS, pPlayer->i, playerNum);
}

void P_ResetInventory(int playerNum)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

    Bmemset(pPlayer->inv_amount, 0, sizeof(pPlayer->inv_amount));

    pPlayer->scuba_on               = 0;
    pPlayer->heat_on                = 0;
    pPlayer->jetpack_on             = 0;
    pPlayer->holoduke_on            = -1;
    pPlayer->inven_icon             = ICON_NONE;
    pPlayer->inv_amount[GET_SHIELD] = g_startArmorAmount;

    VM_OnEvent(EVENT_RESETINVENTORY, pPlayer->i, playerNum);
}

static void resetprestat(int playerNum, int gameMode)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

    g_spriteDeleteQueuePos = 0;
    for (bssize_t i = 0; i < g_deleteQueueSize; i++) SpriteDeletionQueue[i] = -1;

    pPlayer->hbomb_on          = 0;
    pPlayer->cheat_phase       = 0;
    pPlayer->toggle_key_flag   = 0;
    pPlayer->secret_rooms      = 0;
    pPlayer->max_secret_rooms  = 0;
    pPlayer->actors_killed     = 0;
    pPlayer->max_actors_killed = 0;
    pPlayer->lastrandomspot    = 0;
    pPlayer->weapon_pos        = WEAPON_POS_START;

    P_ResetTintFade(pPlayer);

    pPlayer->kickback_pic = ((PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == PISTOL_WEAPON)
                             && (PWEAPON(playerNum, pPlayer->curr_weapon, Reload) > PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime)))
                            ? PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime)
                            : 0;

    pPlayer->last_weapon           = -1;
    pPlayer->weapreccnt            = 0;
    pPlayer->interface_toggle_flag = 0;
    pPlayer->show_empty_weapon     = 0;
    pPlayer->holster_weapon        = 0;
    pPlayer->last_pissed_time      = 0;
    pPlayer->one_parallax_sectnum  = -1;
    pPlayer->visibility            = ud.const_visibility;

    screenpeek         = myconnectindex;
    g_animWallCnt      = 0;
    g_cyclerCnt        = 0;
    g_animateCnt       = 0;
    parallaxtype       = 0;
    randomseed         = 1996;
    ud.pause_on        = 0;
    ud.camerasprite    = -1;
    ud.eog             = 0;
    tempwallptr        = 0;
    g_curViewscreen    = -1;
    g_earthquakeTime   = 0;
    g_interpolationCnt = 0;

    if (((gameMode & MODE_EOL) != MODE_EOL && numplayers < 2 && !g_netServer)
        || (!(g_gametypeFlags[ud.coop] & GAMETYPE_PRESERVEINVENTORYDEATH) && numplayers > 1))
    {
        P_ResetWeapons(playerNum);
        P_ResetInventory(playerNum);
    }
    else if (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == HANDREMOTE_WEAPON)
    {
        pPlayer->ammo_amount[HANDBOMB_WEAPON]++;
        pPlayer->curr_weapon = HANDBOMB_WEAPON;
    }

    pPlayer->timebeforeexit  = 0;
    pPlayer->customexitsound = 0;
}

// Tweak sprites contained in moving sectors with these SE lotags.
#define FIXSPR_SELOTAGP(k) ((k) == SE_0_ROTATING_SECTOR || (k) == SE_6_SUBWAY || (k) == SE_14_SUBWAY_CAR)

// Set up sprites in moving sectors that are to be fixed wrt a certain pivot
// position and should not diverge from it due to roundoff error in the future.
// Has to be after the spawning stuff.
static void G_SetupRotfixedSprites(void)
{
    int spriteNum, nextSpriteNum;

    for (SPRITES_OF_STAT_SAFE(STAT_EFFECTOR, spriteNum, nextSpriteNum))
    {
        if (FIXSPR_SELOTAGP(sprite[spriteNum].lotag))
        {
#ifdef YAX_ENABLE
            int firstrun = 1;
#endif
            int sectSprite = headspritesect[sprite[spriteNum].sectnum];

            do
            {
                const spritetype *const pSprite = &sprite[sectSprite];

                // TRIPBOMB uses t_data[7] for its own purposes. Wouldn't be
                // too useful with moving sectors anyway
                if ((ROTFIXSPR_STATNUMP(pSprite->statnum) && pSprite->picnum != TRIPBOMB)
                    || ((pSprite->statnum == STAT_ACTOR || pSprite->statnum == STAT_ZOMBIEACTOR)
                        && A_CheckSpriteFlags(sectSprite, SFLAG_ROTFIXED)))
                {
                    int pivotSprite = spriteNum;

                    if (sprite[spriteNum].lotag == 0)
                        pivotSprite = sprite[spriteNum].owner;

                    if (sectSprite != spriteNum && sectSprite != pivotSprite && pivotSprite >= 0 && pivotSprite < MAXSPRITES)
                    {
                        // let's hope we don't step on anyone's toes here
                        actor[sectSprite].t_data[7] = ROTFIXSPR_MAGIC | pivotSprite;  // 'rs' magic + pivot SE sprite index
                        actor[sectSprite].t_data[8] = pSprite->x - sprite[pivotSprite].x;
                        actor[sectSprite].t_data[9] = pSprite->y - sprite[pivotSprite].y;
                    }
                }

                sectSprite = nextspritesect[sectSprite];
#ifdef YAX_ENABLE
                if ((sectSprite < 0 && firstrun) &&
                    (sprite[spriteNum].lotag == SE_6_SUBWAY || sprite[spriteNum].lotag == SE_14_SUBWAY_CAR))
                {
                    firstrun   = 0;
                    sectSprite = actor[spriteNum].t_data[9];

                    if (sectSprite >= 0)
                        sectSprite = headspritesect[sectSprite];
                }
#endif
            } while (sectSprite>=0);
        }
    }
}

static inline int G_CheckExitSprite(int spriteNum) { return (sprite[spriteNum].lotag == UINT16_MAX && (sprite[spriteNum].cstat & 16)); }

static void prelevel(char g)
{
    uint8_t *tagbitmap = (uint8_t *)Xcalloc(65536>>3, 1);

    Bmemset(show2dsector, 0, sizeof(show2dsector));
#ifdef LEGACY_ROR
    Bmemset(ror_protectedsectors, 0, MAXSECTORS);
#endif
    resetprestat(0,g);
    g_cloudCnt = 0;

    G_SetupGlobalPsky();

    VM_OnEvent(EVENT_PRELEVEL, -1, -1);

    int missedCloudSectors = 0;

    for (bssize_t i=0; i<numsectors; i++)
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
                    for (bsize_t j = 0; j < 5; j++)
                        tloadtile(sector[i].ceilingpicnum + j, 0);
            }

            if (sector[i].ceilingpicnum == CLOUDYSKIES)
            {
                if (g_cloudCnt < ARRAY_SSIZE(g_cloudSect))
                    g_cloudSect[g_cloudCnt++] = i;
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
    for (bssize_t nextSprite, SPRITES_OF_STAT_SAFE(STAT_DEFAULT, i, nextSprite))
    {
        A_ResetVars(i);
#if !defined LUNATIC
        A_LoadActor(i);
#endif
        VM_OnEvent(EVENT_LOADACTOR, i, -1);
        if (G_CheckExitSprite(i))
        {
            g_player[0].ps->exitx = SX(i);
            g_player[0].ps->exity = SY(i);
        }
        else switch (DYNAMICTILEMAP(PN(i)))
            {
            case GPSPEED__STATIC:
                // DELETE_AFTER_LOADACTOR. Must not change statnum.
                sector[SECT(i)].extra = SLT(i);
                break;

            case CYCLER__STATIC:
                // DELETE_AFTER_LOADACTOR. Must not change statnum.
                if (g_cyclerCnt >= MAXCYCLERS)
                {
                    Bsprintf(tempbuf,"\nToo many cycling sectors (%d max).",MAXCYCLERS);
                    G_GameExit(tempbuf);
                }
                g_cyclers[g_cyclerCnt][0] = SECT(i);
                g_cyclers[g_cyclerCnt][1] = SLT(i);
                g_cyclers[g_cyclerCnt][2] = SS(i);
                g_cyclers[g_cyclerCnt][3] = sector[SECT(i)].floorshade;
                g_cyclers[g_cyclerCnt][4] = SHT(i);
                g_cyclers[g_cyclerCnt][5] = (SA(i) == 1536);
                g_cyclerCnt++;
                break;

            case SECTOREFFECTOR__STATIC:
            case ACTIVATOR__STATIC:
            case TOUCHPLATE__STATIC:
            case ACTIVATORLOCKED__STATIC:
            case MUSICANDSFX__STATIC:
            case LOCATORS__STATIC:
            case MASTERSWITCH__STATIC:
            case RESPAWN__STATIC:
                sprite[i].cstat &= ~(1|16|32|256);
                break;
            }
    }

    // Delete some effector / effector modifier sprites AFTER the loop running
    // the LOADACTOR events. DELETE_AFTER_LOADACTOR.
    for (bssize_t nextSprite, SPRITES_OF_STAT_SAFE(STAT_DEFAULT, i, nextSprite))
        if (!G_CheckExitSprite(i))
            switch (DYNAMICTILEMAP(PN(i)))
            {
            case GPSPEED__STATIC:
            case CYCLER__STATIC:
                A_DeleteSprite(i);
                break;
            }

    for (size_t i = 0; i < MAXSPRITES; i++)
    {
        if (sprite[i].statnum < MAXSTATUS && (PN(i) != SECTOREFFECTOR || SLT(i) != SE_14_SUBWAY_CAR))
            A_Spawn(-1, i);
    }

    for (size_t i = 0; i < MAXSPRITES; i++)
    {
        if (sprite[i].statnum < MAXSTATUS && PN(i) == SECTOREFFECTOR && SLT(i) == SE_14_SUBWAY_CAR)
            A_Spawn(-1, i);
    }

    G_SetupRotfixedSprites();

    for (bssize_t i=headspritestat[STAT_DEFAULT]; i>=0; i=nextspritestat[i])
    {
        if (PN(i) <= 0)  // oob safety for switch below
            continue;

        for (bsize_t ii=0; ii<2; ii++)
        {
            switch (DYNAMICTILEMAP(PN(i)-1+ii))
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
                    int const tag = sprite[i].lotag;
                    tagbitmap[tag>>3] |= 1<<(tag&7);
                }

                break;
            }
        }
    }

    // initially 'on' SE 12 light (*)
    for (bssize_t j=headspritestat[STAT_EFFECTOR]; j>=0; j=nextspritestat[j])
    {
        int const tag = sprite[j].hitag;

        if (sprite[j].lotag == SE_12_LIGHT_SWITCH && tagbitmap[tag>>3]&(1<<(tag&7)))
            actor[j].t_data[0] = 1;
    }

    Bfree(tagbitmap);

    g_mirrorCount = 0;

    for (bssize_t i = 0; i < numwalls; i++)
    {
        walltype * const pWall = &wall[i];

        if (pWall->overpicnum == MIRROR && (pWall->cstat&32) != 0)
        {
            int const nextSectnum = pWall->nextsector;

            if ((nextSectnum >= 0) && sector[nextSectnum].ceilingpicnum != MIRROR)
            {
                if (g_mirrorCount > 63)
                {
                    G_GameExit("\nToo many mirrors (64 max.)");
                }

                sector[nextSectnum].ceilingpicnum = MIRROR;
                sector[nextSectnum].floorpicnum   = MIRROR;
                g_mirrorWall[g_mirrorCount]       = i;
                g_mirrorSector[g_mirrorCount]     = nextSectnum;
                g_mirrorCount++;
                continue;
            }
        }

        if (g_animWallCnt >= MAXANIMWALLS)
        {
            Bsprintf(tempbuf,"\nToo many 'anim' walls (%d max).",MAXANIMWALLS);
            G_GameExit(tempbuf);
        }

        animwall[g_animWallCnt].tag = 0;
        animwall[g_animWallCnt].wallnum = 0;

        int const switchPic = G_GetForcefieldPicnum(i);

        if (switchPic >= 0)
        {
            switch (DYNAMICTILEMAP(switchPic))
            {
                case FANSHADOW__STATIC:
                case FANSPRITE__STATIC:
                    wall->cstat |= 65;
                    animwall[g_animWallCnt].wallnum = i;
                    g_animWallCnt++;
                    break;

                case W_FORCEFIELD__STATIC:
                    if (pWall->overpicnum == W_FORCEFIELD__STATIC)
                        for (bsize_t j = 0; j < 3; j++) tloadtile(W_FORCEFIELD + j, 0);
                    if (pWall->shade > 31)
                        pWall->cstat = 0;
                    else
                        pWall->cstat |= FORCEFIELD_CSTAT | 256;


                    if (pWall->lotag && pWall->nextwall >= 0)
                        wall[pWall->nextwall].lotag = pWall->lotag;
                    fallthrough__;
                case BIGFORCE__STATIC:
                    animwall[g_animWallCnt].wallnum = i;
                    g_animWallCnt++;

                    continue;
            }
        }

        pWall->extra = -1;

        switch (DYNAMICTILEMAP(pWall->picnum))
        {
            case WATERTILE2__STATIC:
                for (bsize_t j = 0; j < 3; j++)
                    tloadtile(pWall->picnum + j, 0);
                break;

            case TECHLIGHT2__STATIC:
            case TECHLIGHT4__STATIC: tloadtile(pWall->picnum, 0); break;
            case W_TECHWALL1__STATIC:
            case W_TECHWALL2__STATIC:
            case W_TECHWALL3__STATIC:
            case W_TECHWALL4__STATIC:
                animwall[g_animWallCnt].wallnum = i;
                //                animwall[g_numAnimWalls].tag = -1;
                g_animWallCnt++;
                break;
            case SCREENBREAK6__STATIC:
            case SCREENBREAK7__STATIC:
            case SCREENBREAK8__STATIC:
                for (bssize_t j = SCREENBREAK6; j < SCREENBREAK9; j++)
                    tloadtile(j, 0);

                animwall[g_animWallCnt].wallnum = i;
                animwall[g_animWallCnt].tag     = -1;
                g_animWallCnt++;
                break;

            case FEMPIC1__STATIC:
            case FEMPIC2__STATIC:
            case FEMPIC3__STATIC:
                pWall->extra                 = pWall->picnum;
                animwall[g_animWallCnt].tag = -1;

                if (ud.lockout)
                    pWall->picnum = (pWall->picnum == FEMPIC1) ? BLANKSCREEN : SCREENBREAK6;

                animwall[g_animWallCnt].wallnum = i;
                animwall[g_animWallCnt].tag     = pWall->picnum;
                g_animWallCnt++;
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
                animwall[g_animWallCnt].wallnum = i;
                animwall[g_animWallCnt].tag     = pWall->picnum;
                g_animWallCnt++;
                break;
        }
    }

    //Invalidate textures in sector behind mirror
    for (bssize_t i=0; i<g_mirrorCount; i++)
    {
        int const startWall = sector[g_mirrorSector[i]].wallptr;
        int const endWall   = startWall + sector[g_mirrorSector[i]].wallnum;

        for (bssize_t j = startWall; j < endWall; j++)
        {
            wall[j].picnum     = MIRROR;
            wall[j].overpicnum = MIRROR;

            if (wall[g_mirrorWall[i]].pal == 4)
                wall[j].pal = 4;
        }
    }
}


void G_NewGame(int volumeNum, int levelNum, int skillNum)
{
    DukePlayer_t *const pPlayer = g_player[0].ps;

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

    if (levelNum == 0 && volumeNum == 3 && (!g_netServer && ud.multimode < 2) && ud.lockout == 0
            && (G_GetLogoFlags() & LOGO_NOE4CUTSCENE)==0)
    {
        S_PlayMusic(g_mapInfo[MUS_BRIEFING].musicfn);

        flushperms();
        setview(0,0,xdim-1,ydim-1);
        clearview(0L);
        nextpage();

        int animReturn = Anim_Play("vol41a.anm");
        clearview(0L);
        nextpage();
        if (animReturn)
            goto end_vol4a;

        animReturn = Anim_Play("vol42a.anm");
        clearview(0L);
        nextpage();
        if (animReturn)
            goto end_vol4a;

        Anim_Play("vol43a.anm");
        clearview(0L);
        nextpage();

end_vol4a:
        FX_StopAllSounds();
    }

    g_showShareware = GAMETICSPERSEC*34;

    ud.level_number = levelNum;
    ud.volume_number = volumeNum;
    ud.player_skill = skillNum;
    ud.secretlevel = 0;
    ud.from_bonus = 0;

    ud.last_level = -1;
    g_lastAutoSaveArbitraryID = -1;
    g_lastautosave.reset();
    g_lastusersave.reset();
    g_quickload = nullptr;

#ifdef EDUKE32_TOUCH_DEVICES
    pPlayer->zoom = 360;
#else
    pPlayer->zoom = 768;
#endif
    pPlayer->gm = 0;
    Menu_Close(0);

#if !defined LUNATIC
    //AddLog("Newgame");
    Gv_ResetVars();

    Gv_InitWeaponPointers();

    // PK: Gv_ResetVars() might trip up the system (pointer) gamevars,
    // e.g. if some earlier-version CON code had been loaded before
    Gv_RefreshPointers();
#endif
    Gv_ResetSystemDefaults();

    for (bssize_t i=0; i<(MAXVOLUMES*MAXLEVELS); i++)
        G_FreeMapState(i);

    if (ud.m_coop != 1)
    {
        for (bssize_t weaponNum = 0; weaponNum < MAX_WEAPONS; weaponNum++)
        {
            if (PWEAPON(0, weaponNum, WorksLike) == PISTOL_WEAPON)
            {
                pPlayer->curr_weapon = weaponNum;
                pPlayer->gotweapon |= (1 << weaponNum);
                pPlayer->ammo_amount[weaponNum] = min(pPlayer->max_ammo_amount[weaponNum], 48);
            }
            else if (PWEAPON(0, weaponNum, WorksLike) == KNEE_WEAPON || PWEAPON(0, weaponNum, WorksLike) == HANDREMOTE_WEAPON)
                pPlayer->gotweapon |= (1 << weaponNum);
        }
        pPlayer->last_weapon = -1;
    }

    display_mirror = 0;

#ifdef LUNATIC
    // NOTE: Lunatic state creation is relatively early. No map has yet been loaded.
    // XXX: What about the cases where G_EnterLevel() is called without a preceding G_NewGame()?
    El_CreateGameState();
    G_PostCreateGameState();
#endif
    VM_OnEvent(EVENT_NEWGAME, g_player[screenpeek].ps->i, screenpeek);
}

static void resetpspritevars(char gameMode)
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
            if ((g_netServer || ud.multimode > 1) && (g_gametypeFlags[ud.coop]&GAMETYPE_PRESERVEINVENTORYDEATH) && ud.last_level >= 0)
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
            if ((g_netServer || ud.multimode > 1) && (g_gametypeFlags[ud.coop]&GAMETYPE_PRESERVEINVENTORYDEATH) && ud.last_level >= 0)
            {
                for (j=0; j<MAX_WEAPONS; j++)
                    g_player[i].ps->ammo_amount[j] = tsbar[i].ammo_amount[j];

                g_player[i].ps->gotweapon = tsbar[i].gotweapon;
                g_player[i].ps->curr_weapon = tsbar[i].curr_weapon;
                g_player[i].ps->inven_icon = tsbar[i].inven_icon;
                Bmemcpy(g_player[i].ps->inv_amount, tsbar[i].inv_amount, sizeof(tsbar[i].inv_amount));
            }
        }

    g_playerSpawnCnt = 0;
//    circ = 2048/ud.multimode;

    g_whichPalForPlayer = 9;
    j = 0;
    i = headspritestat[STAT_PLAYER];
    while (i >= 0)
    {
        const int32_t nexti = nextspritestat[i];
        spritetype *const s = &sprite[i];

        if (g_playerSpawnCnt == MAXPLAYERS)
            G_GameExit("\nToo many player sprites (max 16.)");

        g_playerSpawnPoints[g_playerSpawnCnt].pos.x = s->x;
        g_playerSpawnPoints[g_playerSpawnCnt].pos.y = s->y;
        g_playerSpawnPoints[g_playerSpawnCnt].pos.z = s->z;
        g_playerSpawnPoints[g_playerSpawnCnt].ang   = s->ang;
        g_playerSpawnPoints[g_playerSpawnCnt].sect  = s->sectnum;

        g_playerSpawnCnt++;

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

//            if (j < g_mostConcurrentPlayers)
            {
                if ((gameMode&MODE_EOL) != MODE_EOL || g_player[j].ps->last_extra == 0)
                {
                    g_player[j].ps->last_extra = g_player[j].ps->max_player_health;
                    s->extra = g_player[j].ps->max_player_health;
                    g_player[j].ps->runspeed = g_playerFriction;
                }
                else s->extra = g_player[j].ps->last_extra;

                s->yvel = j;

                if (!g_player[j].pcolor && (g_netServer || ud.multimode > 1) && !(g_gametypeFlags[ud.coop] & GAMETYPE_TDM))
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

                    if (g_gametypeFlags[ud.coop] & GAMETYPE_TDM)
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
    for (bssize_t i = 0; i < ud.multimode; i++)
    {
        playerdata_t *const pPlayerData = &g_player[i];
        pPlayerData->ps->frag = pPlayerData->ps->fraggedself = 0;
        Bmemset(pPlayerData->frags, 0, sizeof(pPlayerData->frags));
    }
}

void G_ResetTimers(uint8_t keepgtics)
{
    totalclock = g_cloudClock = ototalclock = lockclock = 0;
    ready2send = 1;
    g_levelTextTime = 85;

    if (!keepgtics)
        g_moveThingsCount = 0;

    if (g_curViewscreen >= 0)
        actor[g_curViewscreen].t_data[0] = 0;
}

void G_ClearFIFO(void)
{
    g_emuJumpTics = 0;

    clearbufbyte(&localInput, sizeof(input_t), 0L);
    clearbufbyte(&inputfifo, sizeof(input_t) * MOVEFIFOSIZ * MAXPLAYERS, 0L);

    for (bsize_t p = 0; p <= MAXPLAYERS - 1; ++p)
    {
        if (g_player[p].inputBits != NULL)
            Bmemset(g_player[p].inputBits, 0, sizeof(input_t));
        g_player[p].vote = g_player[p].gotvote = 0;
    }
}

int G_FindLevelByFile(const char *fileName)
{
    for (bssize_t volumeNum = 0; volumeNum < MAXVOLUMES; volumeNum++)
    {
        int const volOffset = volumeNum * MAXLEVELS;

        for (bssize_t levelNum = 0; levelNum < MAXLEVELS; levelNum++)
        {
            if (g_mapInfo[volOffset + levelNum].filename == NULL)
                continue;

            if (!Bstrcasecmp(fileName, g_mapInfo[volOffset + levelNum].filename))
                return volOffset + levelNum;
        }
    }

    return MAXLEVELS * MAXVOLUMES;
}

#if 0
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
#endif

static int G_TryMapHack(const char *mhkfile)
{
    int32_t failure = loadmaphack(mhkfile);

    if (!failure)
        initprintf("Loaded map hack file \"%s\"\n", mhkfile);

    return failure;
}

static void G_LoadMapHack(char *outbuf, const char *filename)
{
    if (filename != NULL)
        Bstrcpy(outbuf, filename);

    append_ext_UNSAFE(outbuf, ".mhk");

    if (G_TryMapHack(outbuf) && usermaphacks != NULL)
    {
        usermaphack_t *pMapInfo = (usermaphack_t*)bsearch(
            &g_loadedMapHack, usermaphacks, num_usermaphacks, sizeof(usermaphack_t),
            compare_usermaphacks);

        if (pMapInfo)
            G_TryMapHack(pMapInfo->mhkfile);
    }
}

// levnamebuf should have at least size BMAX_PATH
void G_SetupFilenameBasedMusic(char *nameBuf, const char *fileName, int levelNum)
{
    char *p;
    char const *exts[] = {
#ifdef HAVE_FLAC
        "flac",
#endif
#ifdef HAVE_VORBIS
        "ogg",
#endif
        "mid"
    };

    Bstrncpy(nameBuf, fileName, BMAX_PATH);

    Bcorrectfilename(nameBuf, 0);

    if (NULL == (p = Bstrrchr(nameBuf, '.')))
    {
        p = nameBuf + Bstrlen(nameBuf);
        p[0] = '.';
    }

    for (unsigned int i = 0; i < ARRAY_SIZE(exts); i++)
    {
        int32_t kFile;

        Bmemcpy(p+1, exts[i], Bstrlen(exts[i]) + 1);

        if ((kFile = kopen4loadfrommod(nameBuf, 0)) != -1)
        {
            kclose(kFile);
            realloc_copy(&g_mapInfo[levelNum].musicfn, nameBuf);
            return;
        }
    }

    realloc_copy(&g_mapInfo[levelNum].musicfn, "dethtoll.mid");
}

static inline int G_HaveUserMap(void)
{
    return (boardfilename[0] != 0 && ud.m_level_number == 7 && ud.m_volume_number == 0);
}

int G_EnterLevel(int gameMode)
{
    int32_t i, mii;
    char levelName[BMAX_PATH];

//    flushpackets();
//    waitforeverybody();
    vote_map = vote_episode = voting = -1;

    ud.respawn_monsters  = ud.m_respawn_monsters;
    ud.respawn_items     = ud.m_respawn_items;
    ud.respawn_inventory = ud.m_respawn_inventory;
    ud.monsters_off      = ud.m_monsters_off;
    ud.coop              = ud.m_coop;
    ud.marker            = ud.m_marker;
    ud.ffire             = ud.m_ffire;
    ud.noexits           = ud.m_noexits;

    if ((gameMode & MODE_DEMO) != MODE_DEMO)
        ud.recstat = ud.m_recstat;
    if ((gameMode & MODE_DEMO) == 0 && ud.recstat == 2)
        ud.recstat = 0;

    if (g_networkMode != NET_DEDICATED_SERVER)
    {
        S_PauseSounds(0);
        FX_StopAllSounds();
        S_ClearSoundLocks();
        FX_SetReverb(0);
        setgamemode(ud.config.ScreenMode, ud.config.ScreenWidth, ud.config.ScreenHeight, ud.config.ScreenBPP);
    }

    if (G_HaveUserMap())
    {
        Bcorrectfilename(boardfilename,0);

        int levelNum = G_FindLevelByFile(boardfilename);

        if (levelNum != MAXLEVELS*MAXVOLUMES)
        {
            int volumeNum = levelNum;

            levelNum &= MAXLEVELS-1;
            volumeNum = (volumeNum - levelNum) / MAXLEVELS;

            ud.level_number = ud.m_level_number = levelNum;
            ud.volume_number = ud.m_volume_number = volumeNum;

            boardfilename[0] = 0;
        }
    }

    mii = (ud.volume_number*MAXLEVELS)+ud.level_number;

    if (g_mapInfo[mii].name == NULL || g_mapInfo[mii].filename == NULL)
    {
        if (G_HaveUserMap())
        {
            if (g_mapInfo[mii].filename == NULL)
                g_mapInfo[mii].filename = (char *)Xcalloc(BMAX_PATH, sizeof(uint8_t));
            if (g_mapInfo[mii].name == NULL)
                g_mapInfo[mii].name = Xstrdup("User Map");
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
        Bstrcpy(levelName, boardfilename);

        if (g_gameNamePtr)
#ifdef EDUKE32_STANDALONE
            Bsprintf(apptitle,"%s - %s",levelName,g_gameNamePtr);
#else
            Bsprintf(apptitle, "%s - %s - " APPNAME, levelName, g_gameNamePtr);
#endif
        else
            Bsprintf(apptitle,"%s - " APPNAME,levelName);
    }
    else
    {
        if (g_gameNamePtr)
#ifdef EDUKE32_STANDALONE
            Bsprintf(apptitle,"%s - %s",g_mapInfo[mii].name,g_gameNamePtr);
#else
            Bsprintf(apptitle, "%s - %s - " APPNAME, g_mapInfo[mii].name, g_gameNamePtr);
#endif
        else
            Bsprintf(apptitle,"%s - " APPNAME,g_mapInfo[mii].name);
    }

    Bstrcpy(tempbuf,apptitle);
    wm_setapptitle(tempbuf);

    DukePlayer_t *const pPlayer = g_player[0].ps;

    if (!VOLUMEONE && G_HaveUserMap())
    {
        if (loadboard(boardfilename, 0, &pPlayer->pos, &pPlayer->ang, &pPlayer->cursectnum) < 0)
        {
            OSD_Printf(OSD_ERROR "Map \"%s\" not found or invalid map version!\n", boardfilename);
            return 1;
        }

        G_LoadMapHack(levelName, boardfilename);
        G_SetupFilenameBasedMusic(levelName, boardfilename, ud.m_level_number);
    }
    else if (loadboard(g_mapInfo[mii].filename, VOLUMEONE, &pPlayer->pos, &pPlayer->ang, &pPlayer->cursectnum) < 0)
    {
        OSD_Printf(OSD_ERROR "Map \"%s\" not found or invalid map version!\n", g_mapInfo[mii].filename);
        return 1;
    }
    else
    {
        G_LoadMapHack(levelName, g_mapInfo[mii].filename);
    }

    g_precacheCount = 0;
    Bmemset(gotpic, 0, sizeof(gotpic));
    Bmemset(precachehightile, 0, sizeof(precachehightile));

    //clearbufbyte(Actor,sizeof(Actor),0l); // JBF 20040531: yes? no?

    prelevel(gameMode);

    G_AlignWarpElevators();
    resetpspritevars(gameMode);

    ud.playerbest = CONFIG_GetMapBestTime(G_HaveUserMap() ? boardfilename : g_mapInfo[mii].filename, g_loadedMapHack.md4);

    // G_FadeLoad(0,0,0, 252,0, -28, 4, -1);
    G_CacheMapData();
    // G_FadeLoad(0,0,0, 0,252, 28, 4, -2);

    if (ud.recstat != 2)
    {
        if (g_mapInfo[mii].musicfn != NULL &&
                (g_mapInfo[g_musicIndex].musicfn == NULL ||
                Bstrcmp(g_mapInfo[g_musicIndex].musicfn, g_mapInfo[mii].musicfn) ||
                g_musicSize == 0 ||
                ud.last_level == -1))
            S_PlayMusic(g_mapInfo[mii].musicfn);

        g_musicIndex = mii;
    }

    if (gameMode & (MODE_GAME|MODE_EOL))
    {
        for (TRAVERSE_CONNECT(i))
        {
            g_player[i].ps->gm = MODE_GAME;
            Menu_Close(i);
        }
    }
    else if (gameMode & MODE_RESTART)
    {
        if (ud.recstat == 2)
            g_player[myconnectindex].ps->gm = MODE_DEMO;
        else g_player[myconnectindex].ps->gm = MODE_GAME;
    }

    if ((ud.recstat == 1) && (gameMode&MODE_RESTART) != MODE_RESTART)
        G_OpenDemoWrite();

#ifndef EDUKE32_TOUCH_DEVICES
    if (VOLUMEONE && ud.level_number == 0 && ud.recstat != 2)
        P_DoQuote(QUOTE_F1HELP,g_player[myconnectindex].ps);
#endif

    for (TRAVERSE_CONNECT(i))
    {
        switch (DYNAMICTILEMAP(sector[sprite[g_player[i].ps->i].sectnum].floorpicnum))
        {
            case HURTRAIL__STATIC:
            case FLOORSLIME__STATIC:
            case FLOORPLASMA__STATIC:
                P_ResetWeapons(i);
                P_ResetInventory(i);

                g_player[i].ps->gotweapon &= ~(1 << PISTOL_WEAPON);
                g_player[i].ps->ammo_amount[PISTOL_WEAPON] = 0;

                g_player[i].ps->curr_weapon  = KNEE_WEAPON;
                g_player[i].ps->kickback_pic = 0;
                break;
        }
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

    for (i=g_interpolationCnt-1; i>=0; i--) bakipos[i] = *curipos[i];

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
               g_mapInfo[mii].name);

    g_restorePalette = -1;

    G_UpdateScreenArea();
    clearview(0L);
    G_DrawBackground();
    G_DrawRooms(myconnectindex,65536);

    Net_WaitForServer();
    return 0;
}

void G_FreeMapState(int levelNum)
{
    map_t *const pMapInfo = &g_mapInfo[levelNum];

    if (pMapInfo->savedstate == NULL)
        return;

#if !defined LUNATIC
    for (bssize_t j=0; j<g_gameVarCount; j++)
    {
        if (aGameVars[j].flags & GAMEVAR_NORESET)
            continue;

        if (aGameVars[j].flags & (GAMEVAR_PERPLAYER|GAMEVAR_PERACTOR))
            ALIGNED_FREE_AND_NULL(pMapInfo->savedstate->vars[j]);
    }

    for (bssize_t j=0; j<g_gameArrayCount; j++)
    {
        if (aGameArrays[j].flags & GAMEARRAY_RESTORE)
            ALIGNED_FREE_AND_NULL(pMapInfo->savedstate->arrays[j]);
    }
#else
    Bfree(pMapInfo->savedstate->savecode);
#endif

    ALIGNED_FREE_AND_NULL(pMapInfo->savedstate);
}
