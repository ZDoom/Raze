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
#include "anim.h"
#include "menus.h"
#include "demo.h"
#include "savegame.h"
#include "cmdline.h"
#include "statistics.h"
#include "menu/menu.h"
#include "mapinfo.h"
#include "cmdlib.h"
#include "v_2ddrawer.h"
#include "secrets.h"
#include "glbackend/glbackend.h"

BEGIN_DUKE_NS

void lava_cleararrays();
void addjaildoor(int p1, int p2, int iht, int jlt, int p3, int h);
void addminecart(int p1, int p2, int i, int iht, int p3, int childsectnum);
void addtorch(int i);
void addlightning(int i);


extern int which_palookup;

static uint8_t precachehightile[2][MAXTILES>>3];
static int32_t g_precacheCount;
int32_t g_skillSoundVoice = -1;



static void G_DoLoadScreen(const char *statustext, int32_t percent)
{
    if (ud.recstat != 2)
    {
        int32_t i = 0;

        //g_player[myconnectindex].ps->palette = palette;
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308

        if (!statustext)
        {
            i = ud.screen_size;
            ud.screen_size = 0;
            G_UpdateScreenArea();
            videoClearScreen(0L);
        }

        videoClearScreen(0);
        
        int const loadScreenTile = VM_OnEventWithReturn(EVENT_GETLOADTILE, g_player[screenpeek].ps->i, screenpeek, DEER ? 7040 : TILE_LOADSCREEN);

        rotatesprite_fs(320<<15,200<<15,65536L,0,loadScreenTile,0,0,2+8+64+BGSTRETCH);

        int const textY = RRRA ? 140 : 90;

        if (boardfilename[0] != 0 && ud.level_number == 7 && ud.volume_number == 0)
        {
            menutext_center(textY, RR ? GStrings("TXT_ENTRUM") : GStrings("TXT_LOADUM"));
            if (RR)
                menutext_center(textY+20, boardfilename);
            else
                gametext_center_shade_pal(textY+10, boardfilename, 14, 2);
        }
        else if (RR && g_lastLevel)
        {
            menutext_center(textY,GStrings("TXT_ENTERIN"));
            menutext_center(textY+16+8,GStrings("TXT_CLOSEENCOUNTERS"));
        }
        else
        {
            menutext_center(textY, RR ? GStrings("TXT_ENTERIN") : GStrings("TXT_LOADING"));
            menutext_center(textY+16+8,mapList[(ud.volume_number*MAXLEVELS) + ud.level_number].DisplayName());
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

        videoNextPage();

        if (!statustext)
        {
            inputState.keyFlushChars();
            ud.screen_size = i;
        }
    }
    else
    {
        if (!statustext)
        {
            videoClearScreen(0L);
            //g_player[myconnectindex].ps->palette = palette;
            //G_FadePalette(0,0,0,0);
            P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308
        }
        /*Gv_SetVar(g_iReturnVarID,TILE_LOADSCREEN, -1, -1);*/

        rotatesprite_fs(320<<15,200<<15,65536L, 0,TILE_LOADSCREEN,0,0,2+8+64+BGSTRETCH);

        menutext_center(RRRA?155:105,RR? GStrings("TXT_LOADIN") : GStrings("TXT_Loading..."));
        if (statustext) gametext_center_number(180, statustext);
        videoNextPage();
    }
}




extern int32_t fragbarheight(void)
{
    if (ud.screen_size > 0 && !(ud.statusbarflags & STATUSBAR_NOFRAGBAR)
#ifdef SPLITSCREEN_MOD_HACKS
        && !g_fakeMultiMode
#endif
        && (g_netServer || ud.multimode > 1) && GTFLAGS(GAMETYPE_FRAGBAR))
    {
        int32_t i, j = 0;

        for (TRAVERSE_CONNECT(i))
            if (i > j)
                j = i;

        return ((j + 3) >> 2) << 3;
    }

    return 0;
}

void G_UpdateScreenArea(void)
{
    if (!in3dmode())
        return;

    ud.screen_size = clamp(ud.screen_size, 0, 64);
    if (ud.screen_size == 0)
        renderFlushPerms();

    {
        const int32_t ss = max(ud.screen_size-8,0);

        int32_t x1 = scale(ss,xdim,160);
        int32_t x2 = xdim-x1;

        int32_t y1 = scale(ss,(200 * 100) - ((tilesiz[TILE_BOTTOMSTATUSBAR].y >> (RR ? 1 : 0)) * ud.statusbarscale),200 - tilesiz[TILE_BOTTOMSTATUSBAR].y);
        int32_t y2 = 200*100-y1;

        if (RR && ud.screen_size <= 12)
        {
            x1 = 0;
            x2 = xdim;
            y1 = 0;
            if (ud.statusbarmode)
                y2 = 200*100;
        }

        y1 += fragbarheight()*100;
        if (ud.screen_size >= 8 && ud.statusbarmode==0)
            y2 -= (tilesiz[TILE_BOTTOMSTATUSBAR].y >> (RR ? 1 : 0))*ud.statusbarscale;
        y1 = scale(y1,ydim,200*100);
        y2 = scale(y2,ydim,200*100);

        videoSetViewableArea(x1,y1,x2-1,y2-1);
    }

    pub = NUMPAGES;
    pus = NUMPAGES;
}

static inline void P_ResetTintFade(DukePlayer_t *const pPlayer)
{
    pPlayer->pals.f = 0;
}


static inline int G_CheckExitSprite(int spriteNum) { return ((uint16_t)sprite[spriteNum].lotag == UINT16_MAX && (sprite[spriteNum].cstat & 16)); }

void G_InitRRRASkies(void)
{
    if (!RRRA)
        return;
    
    for (bssize_t i = 0; i < MAXSECTORS; i++)
    {
        if (sector[i].ceilingpicnum != TILE_LA && sector[i].ceilingpicnum != TILE_MOONSKY1 && sector[i].ceilingpicnum != TILE_BIGORBIT1)
        {
            int const picnum = sector[i].ceilingpicnum;
            if (tileWidth(picnum) == 512)
            {
                psky_t *sky = tileSetupSky(picnum);
                sky->horizfrac = 32768;
                sky->lognumtiles = 1;
                sky->tileofs[0] = 0;
                sky->tileofs[1] = 0;
            }
            else if (tileWidth(picnum) == 1024)
            {
                psky_t *sky = tileSetupSky(picnum);
                sky->horizfrac = 32768;
                sky->lognumtiles = 0;
                sky->tileofs[0] = 0;
            }
        }
    }
}

static void prelevel(char g)
{
    uint8_t *tagbitmap = (uint8_t *)Xcalloc(65536>>3, 1);
    int32_t p1 = 0, p2 = 0, p3 = 0;
    //DukePlayer_t *ps = g_player[screenpeek].ps;

    if (RRRA)
    {
        setmapfog(0);
        fogactive = 0;
        ufospawnsminion = 0;
        pistonsound = 0;
        enemysizecheat = 0;
        g_player[myconnectindex].ps->MamaEnd = 0;
        mamaspawn_count = 15;
        banjosound = 0;
        RRRA_ExitedLevel = 0;
        if (!DEER)
        {
            for (bssize_t TRAVERSE_CONNECT(playerNum))
            {
                DukePlayer_t *ps = g_player[playerNum].ps;
                ps->sea_sick_stat = 0;
                ps->SlotWin = 0;
                if (ud.level_number == 4 && ud.volume_number == 1)
                    ps->inv_amount[GET_STEROIDS] = 0;
            }
            if (ud.level_number == 3 && ud.volume_number == 0)
                mamaspawn_count = 5;
            else if (ud.level_number == 2 && ud.volume_number == 1)
                mamaspawn_count = 10;
            else if (ud.level_number == 6 && ud.volume_number == 1)
                mamaspawn_count = 15;
        }
    }

    Bmemset(g_spriteExtra, 0, sizeof(g_spriteExtra));
    Bmemset(g_sectorExtra, 0, sizeof(g_sectorExtra));
    Bmemset(shadedsector, 0, sizeof(shadedsector));
    Bmemset(g_geoSectorWarp, -1, sizeof(g_geoSectorWarp));
    Bmemset(g_geoSectorWarp2, -1, sizeof(g_geoSectorWarp2));
    Bmemset(ambienthitag, -1, sizeof(ambienthitag));
    Bmemset(ambientlotag, -1, sizeof(ambientlotag));
    show2dsector.Zero();
#ifdef LEGACY_ROR
    Bmemset(ror_protectedsectors, 0, MAXSECTORS);
#endif
    resetprestat(0,g);
    if (RR)
    {
        lava_cleararrays();
        g_geoSectorCnt = 0;
        g_ambientCnt = 0;
        g_thunderOn = 0;
        g_chickenPlant = 0;
        if (RRRA)
        {
            WindTime = 0;
            WindDir = 0;
            fakebubba_spawn = 0;
            RRRA_ExitedLevel = 0;
            mamaspawn_count = 15; // ???
            g_bellTime = 0;
            g_bellSprite = 0;

            for (bssize_t spriteNum = 0; spriteNum < MAXSPRITES; spriteNum++)
            {
                if (sprite[spriteNum].pal == 100)
                {
                    if (g_netServer || numplayers > 1)
                        A_DeleteSprite(spriteNum);
                    else
                        sprite[spriteNum].pal = 0;
                }
                else if (sprite[spriteNum].pal == 101)
                {
                    sprite[spriteNum].extra = 0;
                    sprite[spriteNum].hitag = 1;
                    sprite[spriteNum].pal = 0;
                    changespritestat(spriteNum, 118);
                }
            }
        }
    }
    g_cloudCnt = 0;

    int missedCloudSectors = 0;

    if (!DEER)
    for (bssize_t i=0; i<numsectors; i++)
    {
        if (RR && sector[i].ceilingpicnum == TILE_RRTILE2577)
            g_thunderOn = 1;
        sector[i].extra = 256;

        switch (sector[i].lotag)
        {
        case 41:
        {
            if (!RR) break;
            int k = headspritesect[i];
            while (k != -1)
            {
                int const nexti = nextspritesect[k];
                if (sprite[k].picnum == TILE_RRTILE11)
                {
                    p1 = sprite[k].lotag << 4;
                    p2 = sprite[k].hitag;
                    A_DeleteSprite(k);
                }
                if (sprite[k].picnum == TILE_RRTILE38)
                {
                    p3 = sprite[k].lotag;
                    A_DeleteSprite(k);
                }
                k = nexti;
            }
            for (bssize_t j = 0; j<numsectors; j++)
            {
                if (sector[i].hitag == sector[j].hitag && i != j)
                {
                    addjaildoor(p1, p2, sector[i].hitag, sector[j].lotag, p3, j);
                 }
            }
            break;
        }
        case 42:
        {
            if (!RR) break;
            int childsectnum = -1;
            int k = headspritesect[i];
            while (k != -1)
            {
                int const nexti = nextspritesect[k];
                if (sprite[k].picnum == TILE_RRTILE64)
                {
                    p1 = sprite[k].lotag << 4;
                    p2 = sprite[k].hitag;
                    for (bssize_t kk = 0; kk < MAXSPRITES; kk++)
                    {
                        if (sprite[kk].picnum == TILE_RRTILE66)
                            if (sprite[kk].lotag == sprite[k].sectnum)
                            {
                                childsectnum = sprite[kk].sectnum;
                                A_DeleteSprite(kk);
                            }
                    }
                    A_DeleteSprite(k);
                }
                if (sprite[k].picnum == TILE_RRTILE65)
                {
                    p3 = sprite[k].lotag;
                    A_DeleteSprite(k);
                }
                k = nexti;
            }
            addminecart(p1, p2, i, sector[i].hitag, p3, childsectnum);
            break;
        }
        case ST_20_CEILING_DOOR:
        case ST_22_SPLITTING_DOOR:
            if (sector[i].floorz > sector[i].ceilingz)
                sector[i].lotag |= 32768u;
            continue;
        }

        if (sector[i].ceilingstat&1)
        {
            if (!RR && sector[i].ceilingpicnum == TILE_CLOUDYSKIES)
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

        if ((uint16_t)sector[i].lotag == UINT16_MAX)
        {
            g_player[0].ps->exitx = wall[sector[i].wallptr].x;
            g_player[0].ps->exity = wall[sector[i].wallptr].y;
            continue;
        }
    }

    if (missedCloudSectors > 0)
        Printf(TEXTCOLOR_RED "Map warning: have %d unhandled TILE_CLOUDYSKIES ceilings.\n", missedCloudSectors);

    // NOTE: must be safe loop because callbacks could delete sprites.
    if (!DEER)
    for (bssize_t nextSprite, SPRITES_OF_STAT_SAFE(STAT_DEFAULT, i, nextSprite))
    {
        //A_LoadActor(i);
        if (G_CheckExitSprite(i))
        {
            g_player[0].ps->exitx = SX(i);
            g_player[0].ps->exity = SY(i);
        }
        else switch (DYNAMICTILEMAP(PN(i)))
            {
            case NUKEBUTTON__STATIC:
                if (RR) g_chickenPlant = 1;
                break;
            case GPSPEED__STATIC:
                // DELETE_AFTER_LOADACTOR. Must not change statnum.
                sector[SECT(i)].extra = SLT(i);
                A_DeleteSprite(i);
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
                A_DeleteSprite(i);
                break;

            case RRTILE18__STATICRR:
                if (!RR) break;
                addtorch(i);
                A_DeleteSprite(i);
                break;

            case RRTILE35__STATICRR:
                addlightning(i);
                A_DeleteSprite(i);
                break;

            case RRTILE68__STATICRR:
                shadedsector[SECT(i)] = 1;
                A_DeleteSprite(i);
                break;

            case RRTILE67__STATICRR:
                sprite[i].cstat |= 32768;
                break;

            case SOUNDFX__STATICRR:
                if (g_ambientCnt >= 64)
                    G_GameExit("\nToo many ambient effects");
                else
                {
                    ambienthitag[g_ambientCnt] = SHT(i);
                    ambientlotag[g_ambientCnt] = SLT(i);
                    sprite[i].ang = g_ambientCnt++;
                    sprite[i].lotag = 0;
                    sprite[i].hitag = 0;
                }
                break;

            //case SECTOREFFECTOR__STATIC:
            //case ACTIVATOR__STATIC:
            //case TOUCHPLATE__STATIC:
            //case ACTIVATORLOCKED__STATIC:
            //case MUSICANDSFX__STATIC:
            //case LOCATORS__STATIC:
            //case MASTERSWITCH__STATIC:
            //case RESPAWN__STATIC:
            //    sprite[i].cstat &= ~(1|16|32|256);
            //    break;
            }
    }
    if (RR && !DEER)
    {
        for (bssize_t i = 0; i < MAXSPRITES; i++)
        {
            if (sprite[i].picnum == TILE_RRTILE19)
            {
                if (sprite[i].hitag == 0)
                {
                    if (g_geoSectorCnt >= MAXGEOSECTORS)
                        G_GameExit("\nToo many geometry effects");
                    g_geoSector[g_geoSectorCnt] = sprite[i].sectnum;
                    for (bssize_t j = 0; j < MAXSPRITES; j++)
                    {
                        if (sprite[i].lotag == sprite[j].lotag && i != j && sprite[j].picnum == TILE_RRTILE19)
                        {
                            if (sprite[j].hitag == 1)
                            {
                                g_geoSectorWarp[g_geoSectorCnt] = sprite[j].sectnum;
                                g_geoSectorX[g_geoSectorCnt] = sprite[i].x - sprite[j].x;
                                g_geoSectorY[g_geoSectorCnt] = sprite[i].y - sprite[j].y;
                            }
                            if (sprite[j].hitag == 2)
                            {
                                g_geoSectorWarp2[g_geoSectorCnt] = sprite[j].sectnum;
                                g_geoSectorX2[g_geoSectorCnt] = sprite[i].x - sprite[j].x;
                                g_geoSectorY2[g_geoSectorCnt] = sprite[i].y - sprite[j].y;
                            }
                        }
                    }
                    g_geoSectorCnt++;
                }
            }
        }
    }

    for (size_t i = 0; i < MAXSPRITES; i++)
    {
        if (sprite[i].statnum < MAXSTATUS && (DEER || PN(i) != TILE_SECTOREFFECTOR || SLT(i) != SE_14_SUBWAY_CAR))
            fi.spawn(-1, i);
    }

    if (!DEER)
    for (size_t i = 0; i < MAXSPRITES; i++)
    {
        if (sprite[i].statnum < MAXSTATUS && PN(i) == TILE_SECTOREFFECTOR && SLT(i) == SE_14_SUBWAY_CAR)
            fi.spawn(-1, i);
        if (RR && sprite[i].picnum == TILE_RRTILE19)
            A_DeleteSprite(i);
        if (RR && sprite[i].picnum == TILE_RRTILE34)
        {
            g_sectorExtra[sprite[i].sectnum] = sprite[i].lotag;
            A_DeleteSprite(i);
        }
    }

    //G_SetupRotfixedSprites();

    if (!DEER)
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
            case RRTILE8464__STATICRR:
                if (RR && !RRRA && PN(i)-1+ii == (uint32_t)TILE_RRTILE8464) break;
                // the lower code only for the 'on' state (*)
                if (ii==0)
                {
                    uint16_t const tag = sprite[i].lotag;
                    tagbitmap[tag>>3] |= 1<<(tag&7);
                }

                break;
            }
        }
    }

    // initially 'on' SE 12 light (*)
    if (!DEER)
    for (bssize_t j=headspritestat[STAT_EFFECTOR]; j>=0; j=nextspritestat[j])
    {
        uint16_t const tag = sprite[j].hitag;

        if (sprite[j].lotag == SE_12_LIGHT_SWITCH && tagbitmap[tag>>3]&(1<<(tag&7)))
            actor[j].t_data[0] = 1;
    }

    Xfree(tagbitmap);

    g_mirrorCount = 0;

    for (bssize_t i = 0; i < numwalls; i++)
    {
        walltype * const pWall = &wall[i];

        if (!DEER && pWall->overpicnum == TILE_MIRROR && (pWall->cstat&32) != 0)
        {
            int const nextSectnum = pWall->nextsector;

            if ((nextSectnum >= 0) && sector[nextSectnum].ceilingpicnum != TILE_MIRROR)
            {
                if (g_mirrorCount > 63)
                {
                    G_GameExit("\nToo many mirrors (64 max.)");
                }

                sector[nextSectnum].ceilingpicnum = TILE_MIRROR;
                sector[nextSectnum].floorpicnum   = TILE_MIRROR;
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
                    if (RR) break;
                    fallthrough__;
                case FANSPRITE__STATIC:
                    wall->cstat |= 65;
                    animwall[g_animWallCnt].wallnum = i;
                    g_animWallCnt++;
                    break;

                case W_FORCEFIELD__STATIC:
                    if (RR) break;
                    if (pWall->overpicnum == W_FORCEFIELD__STATIC)
                        for (bsize_t j = 0; j < 3; j++) markTileForPrecache(TILE_W_FORCEFIELD + j, 0);
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
                    markTileForPrecache(pWall->picnum + j, 0);
                break;

            case RRTILE1814__STATICRR:
            case RRTILE1817__STATICRR:
                markTileForPrecache(pWall->picnum, 0);
                break;
            case RRTILE1939__STATICRR:
            case RRTILE1986__STATICRR:
            case RRTILE1987__STATICRR:
            case RRTILE1988__STATICRR:
            case RRTILE2004__STATICRR:
            case RRTILE2005__STATICRR:
            case RRTILE2123__STATICRR:
            case RRTILE2124__STATICRR:
            case RRTILE2125__STATICRR:
            case RRTILE2126__STATICRR:
            case RRTILE2636__STATICRR:
            case RRTILE2637__STATICRR:
            case RRTILE2878__STATICRR:
            case RRTILE2879__STATICRR:
            case RRTILE2898__STATICRR:
            case RRTILE2899__STATICRR:
                markTileForPrecache(pWall->picnum, 0);
                break;


            case TECHLIGHT2__STATIC:
            case TECHLIGHT4__STATIC: markTileForPrecache(pWall->picnum, 0); break;
            case W_TECHWALL1__STATIC:
            case W_TECHWALL2__STATIC:
            case W_TECHWALL3__STATIC:
            case W_TECHWALL4__STATIC:
                if (RR) break;
                animwall[g_animWallCnt].wallnum = i;
                //                animwall[g_numAnimWalls].tag = -1;
                g_animWallCnt++;
                break;
            case SCREENBREAK6__STATIC:
            case SCREENBREAK7__STATIC:
            case SCREENBREAK8__STATIC:
                for (bssize_t j = TILE_SCREENBREAK6; j < TILE_SCREENBREAK9; j++)
                    markTileForPrecache(j, 0);

                animwall[g_animWallCnt].wallnum = i;
                animwall[g_animWallCnt].tag     = -1;
                g_animWallCnt++;
                break;

            case FEMPIC1__STATIC:
            case FEMPIC2__STATIC:
            case FEMPIC3__STATIC:
                if (RR) break;
                pWall->extra                 = pWall->picnum;
                animwall[g_animWallCnt].tag = -1;

                if (adult_lockout)
                    pWall->picnum = (pWall->picnum == TILE_FEMPIC1) ? TILE_BLANKSCREEN : TILE_SCREENBREAK6;

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
                if (RR) break;
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
            wall[j].picnum     = TILE_MIRROR;
            wall[j].overpicnum = TILE_MIRROR;
        }
    }

    if (RR && !g_thunderOn)
    {
        videoSetPalette(BASEPAL,0);
        g_visibility = g_player[screenpeek].ps->visibility;
    }
    if (RR)
    {
		tileDelete(0);
    }

    G_SetupGlobalPsky();

}


void G_NewGame(int volumeNum, int levelNum, int skillNum)
{
    DukePlayer_t *const pPlayer = g_player[0].ps;

    G_HandleAsync();

    g_skillSoundVoice = -1;

    ready2send = 0;

    if (m_recstat != 2 && ud.last_level >= 0 &&
        (g_netServer || ud.multimode > 1) && (ud.coop&GAMETYPE_SCORESHEET))
    {
        if (!RRRA || g_mostConcurrentPlayers > 1 || g_netServer || numplayers > 1)
            G_BonusScreen(1);
        else
            G_BonusScreenRRRA(1);
    }

    if (RR && !RRRA && ud.level_number == 6 && ud.volume_number == 0)
        G_BonusScreen(0);

    g_showShareware = GAMETICSPERSEC*34;

    ud.level_number = levelNum;
    ud.volume_number = volumeNum;
    ud.player_skill = skillNum;
    ud.secretlevel = 0;
    ud.from_bonus = 0;

    ud.last_level = -1;
    
    int const UserMap = Menu_HaveUserMap();

    // we don't want the intro to play after the multiplayer setup screen
    if (!RR && (!g_netServer && ud.multimode < 2) && UserMap == 0 &&
        levelNum == 0 && volumeNum == 3 && adult_lockout == 0)
    {
        S_PlaySpecialMusic(MUS_BRIEFING);

        renderFlushPerms();
        videoSetViewableArea(0,0,xdim-1,ydim-1);
        twod->ClearScreen();
        videoNextPage();

        int animReturn = Anim_Play("vol41a.anm");
        twod->ClearScreen();
        videoNextPage();
        if (animReturn)
            goto end_vol4a;

        animReturn = Anim_Play("vol42a.anm");
        twod->ClearScreen();
        videoNextPage();
        if (animReturn)
            goto end_vol4a;

        Anim_Play("vol43a.anm");
        twod->ClearScreen();
        videoNextPage();

end_vol4a:
        FX_StopAllSounds();
    }

#ifdef EDUKE32_TOUCH_DEVICES
    pPlayer->zoom = 360;
#else
    pPlayer->zoom = 768;
#endif
    pPlayer->gm = 0;
	M_ClearMenus();

    ResetGameVars();

    //AddLog("Newgame");

    if (m_coop != 1)
    {
        for (bssize_t weaponNum = 0; weaponNum < 12/*MAX_WEAPONS*/; weaponNum++)
        {
            auto const worksLike = WW2GI ? PWEAPON(0, weaponNum, WorksLike) : weaponNum;
            if (worksLike == PISTOL_WEAPON)
            {
                pPlayer->curr_weapon = weaponNum;
                pPlayer->gotweapon.Set(weaponNum);
                pPlayer->ammo_amount[weaponNum] = min<int16_t>(max_ammo_amount[weaponNum], 48);
            }
            else if (worksLike == KNEE_WEAPON || (!RR && worksLike == HANDREMOTE_WEAPON) || (RRRA && worksLike == SLINGBLADE_WEAPON))
            {
                pPlayer->gotweapon.Set(weaponNum);
                if (RRRA)
                    pPlayer->ammo_amount[KNEE_WEAPON] = 1;
            }
        }
        pPlayer->last_weapon = -1;
    }

    display_mirror = 0;
}

void resetpspritevars(int gameMode);

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
    Net_ClearFIFO();

    memset(&localInput, 0, sizeof(input_t));
    memset(&inputfifo, 0, sizeof(input_t) * MOVEFIFOSIZ * MAXPLAYERS);

    for (bsize_t p = 0; p <= MAXPLAYERS - 1; ++p)
    {
        if (g_player[p].input != NULL)
            Bmemset(g_player[p].input, 0, sizeof(input_t));
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
             if (!mapList[volOffset + levelNum].fileName.CompareNoCase(fileName))
                return volOffset + levelNum;
        }
    }

    return MAXLEVELS * MAXVOLUMES;
}

static int G_TryMapHack(const char *mhkfile)
{
    int32_t failure = engineLoadMHK(mhkfile);

    if (!failure)
        Printf("Loaded map hack file \"%s\"\n", mhkfile);

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

void cacheit_d();
void cacheit_r();
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
    ud.coop              = m_coop;
    ud.marker            = m_marker;
    ud.ffire             = m_ffire;
    ud.noexits           = m_noexits;

    if ((gameMode & MODE_DEMO) != MODE_DEMO)
        ud.recstat = m_recstat;
    if ((gameMode & MODE_DEMO) == 0 && ud.recstat == 2)
        ud.recstat = 0;

    VM_OnEvent(EVENT_ENTERLEVEL);

    //if (g_networkMode != NET_DEDICATED_SERVER)
    {
        S_PauseSounds(false);
        FX_StopAllSounds();
        S_ClearSoundLocks();
        FX_SetReverb(0);
    }

    if (Menu_HaveUserMap())
    {
        int levelNum = G_FindLevelByFile(boardfilename);

        if (levelNum != MAXLEVELS*MAXVOLUMES)
        {
            int volumeNum = levelNum;

            levelNum &= MAXLEVELS-1;
            volumeNum = (volumeNum - levelNum) / MAXLEVELS;

            ud.level_number = m_level_number = levelNum;
            ud.volume_number = ud.m_volume_number = volumeNum;

            boardfilename[0] = 0;
        }
    }

    // Redirect the final RR level to a valid map record so that currentLevel can point to something.
    mii = (RR && g_lastLevel)? 127 : (ud.volume_number*MAXLEVELS)+ud.level_number;
	auto &mi = mapList[mii];

    if (mi.fileName.IsEmpty() && !Menu_HaveUserMap())
    {
        Printf(TEXTCOLOR_RED "Map E%dL%d not defined!\n", ud.volume_number+1, ud.level_number+1);
        return 1;
    }

    i = ud.screen_size;
    ud.screen_size = 0;

    FStringf msg("%s . . .", GStrings("TXT_LOADMAP"));
    G_DoLoadScreen(msg, -1);
    G_UpdateScreenArea();

    ud.screen_size = i;

    DukePlayer_t *const pPlayer = g_player[0].ps;
    int16_t lbang;

    if (!VOLUMEONE && Menu_HaveUserMap())
    {
        if (engineLoadBoard(boardfilename, 0, &pPlayer->pos, &lbang, &pPlayer->cursectnum) < 0)
        {
            Printf(TEXTCOLOR_RED "Map \"%s\" not found or invalid map version!\n", boardfilename);
            return 1;
        }
        userMapRecord.name = "";
        userMapRecord.SetFileName(boardfilename);
        currentLevel = &userMapRecord;
        SECRET_SetMapName(currentLevel->DisplayName(), currentLevel->name);
        STAT_NewLevel(boardfilename);
		G_LoadMapHack(levelName, boardfilename);
        userMapRecord.music = G_SetupFilenameBasedMusic(boardfilename, !RR? "dethtoll.mid" : nullptr);
    }
    else if (engineLoadBoard(mi.fileName, VOLUMEONE, &pPlayer->pos, &lbang, &pPlayer->cursectnum) < 0)
    {
        Printf(TEXTCOLOR_RED "Map \"%s\" not found or invalid map version!\n", mi.fileName.GetChars());
        return 1;
    }
    else
    {
        currentLevel = &mi;
        SECRET_SetMapName(currentLevel->DisplayName(), currentLevel->name);
        STAT_NewLevel(mi.fileName);
		G_LoadMapHack(levelName, mi.fileName);
    }

    if (RR && !RRRA && ud.volume_number == 1 && ud.level_number == 1)
    {
        for (bssize_t i = PISTOL_WEAPON; i < MAX_WEAPONS; i++)
            g_player[0].ps->ammo_amount[i] = 0;
        g_player[0].ps->gotweapon.Clear(KNEE_WEAPON);
    }

    pPlayer->q16ang = fix16_from_int(lbang);

    g_precacheCount = 0;
    Bmemset(gotpic, 0, sizeof(gotpic));
    Bmemset(precachehightile, 0, sizeof(precachehightile));

    prelevel(gameMode);

    G_InitRRRASkies();

    if (RRRA && ud.level_number == 2 && ud.volume_number == 0)
    {
        for (bssize_t i = PISTOL_WEAPON; i < MAX_WEAPONS; i++)
            g_player[0].ps->ammo_amount[i] = 0;
        g_player[0].ps->gotweapon.Clear(KNEE_WEAPON);
        g_player[0].ps->gotweapon.Set(SLINGBLADE_WEAPON);
        g_player[0].ps->ammo_amount[SLINGBLADE_WEAPON] = 1;
        g_player[0].ps->curr_weapon = SLINGBLADE_WEAPON;
    }

    allignwarpelevators();
    resetpspritevars(gameMode);

    ud.playerbest = CONFIG_GetMapBestTime(Menu_HaveUserMap() ? boardfilename : mi.fileName.GetChars(), g_loadedMapHack.md4);

    // G_FadeLoad(0,0,0, 252,0, -28, 4, -1);
    if (isRR()) cacheit_r(); else cacheit_d();
    //G_CacheMapData();
    // G_FadeLoad(0,0,0, 0,252, 28, 4, -2);

    // Try this first so that it can disable the CD player if no tracks are found.
    if (RR && !(gameMode & MODE_DEMO))
        S_PlayRRMusic();

    if (ud.recstat != 2)
    {
        if (Menu_HaveUserMap())
        {
            S_PlayLevelMusic(USERMAPMUSICFAKESLOT);
        }
        else S_PlayLevelMusic(mii);
    }

    if (gameMode & (MODE_GAME|MODE_EOL))
    {
        for (TRAVERSE_CONNECT(i))
        {
            g_player[i].ps->gm = MODE_GAME;
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
                resetweapons(i);
                resetinventory(i);

                g_player[i].ps->gotweapon.Clear(PISTOL_WEAPON);
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
    renderFlushPerms();

    everyothertime = 0;
    g_globalRandom = 0;

    ud.last_level = ud.level_number+1;

    G_ClearFIFO();

    for (i=numinterpolations-1; i>=0; i--) bakipos[i] = *curipos[i];

    g_player[myconnectindex].ps->over_shoulder_on = 0;

    clearfrags();

    G_ResetTimers(0);  // Here we go

    //Bsprintf(g_szBuf,"G_EnterLevel L=%d V=%d",ud.level_number, ud.volume_number);
    //AddLog(g_szBuf);
    // variables are set by pointer...


    if (G_HaveUserMap())
    {
        Printf(TEXTCOLOR_GOLD "%s: %s\n", GStrings("TXT_USERMAP"), boardfilename);
    }
    else
    {
        Printf(TEXTCOLOR_GOLD "E%dL%d: %s\n", ud.volume_number+1, ud.level_number+1,
                   mapList[mii].DisplayName());
    }

    restorepalette = -1;

    G_UpdateScreenArea();
    videoClearViewableArea(0L);
    G_DrawBackground();
    G_DrawRooms(myconnectindex,65536);

    Net_WaitForEverybody();
    return 0;
}

void setmapfog(int fogtype)
{
    GLInterface.SetMapFog(fogtype != 0);
}

END_DUKE_NS
