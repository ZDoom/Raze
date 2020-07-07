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
#include "menu.h"
#include "mapinfo.h"
#include "cmdlib.h"
#include "v_2ddrawer.h"
#include "secrets.h"
#include "glbackend/glbackend.h"

BEGIN_RR_NS

static int32_t g_whichPalForPlayer = 9;

static uint8_t precachehightile[2][MAXTILES>>3];
static int32_t g_precacheCount;
int32_t g_skillSoundVoice = -1;
MapRecord userMapRecord;


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
    case RRTILE2121__STATICRR:
    case RRTILE2122__STATICRR:
        tloadtile(BROKEFIREHYDRENT, 1);
        break;
    case TOILET__STATIC:
        tloadtile(TOILETBROKE,1);
        for (j = TOILETWATER; j < (TOILETWATER+4); j++) tloadtile(j,1);
        break;
    case STALL__STATIC:
        tloadtile(STALLBROKE,1);
        for (j = TOILETWATER; j < (TOILETWATER+4); j++) tloadtile(j,1);
        break;
    case FORCERIPPLE__STATIC:
        if (!RR) break;
        maxc = 9;
        break;
    case RUBBERCAN__STATIC:
        maxc = 2;
        break;
    case TOILETWATER__STATIC:
        maxc = 4;
        break;
    case BUBBASTAND__STATICRR:
        for (j = BUBBASCRATCH; j < (BUBBASCRATCH+47); j++) tloadtile(j,1);
        maxc = 0;
        break;
    case SBSWIPE__STATICRR:
        if (!RRRA) break;
        for (j = BUBBASCRATCH; j <= (SBSWIPE+47); j++) tloadtile(j,1);
        maxc = 0;
        break;
    case COOT__STATICRR:
        for(j = COOT; j <= (COOT+217); j++) tloadtile(j,1);
        for(j = COOTJIBA; j < COOTJIBC+4; j++) tloadtile(j,1);
        maxc = 0;
        break;
    case LTH__STATICRR:
        maxc = 105;
        for (j = LTH; j < (LTH + maxc); j++) tloadtile(j,1);
        maxc = 0;
        break;
    case BILLYRAY__STATICRR:
        maxc = 144;
        for (j = BILLYWALK; j < (BILLYWALK + maxc); j++) tloadtile(j,1);
        for (j = BILLYJIBA; j <= BILLYJIBB + 4; j++) tloadtile(j,1);
        maxc = 0;
        break;
    case COW__STATICRR:
        maxc = 56;
        for (j = PN(i); j < (PN(i) + maxc); j++) tloadtile(j,1);
        maxc = 0;
        break;
    case DOGRUN__STATICRR:
        for (j = DOGATTACK; j <= DOGATTACK + 35; j++) tloadtile(j,1);
        for (j = DOGRUN; j <= DOGRUN + 121; j++) tloadtile(j,1);
        maxc = 0;
        break;
    case RABBIT__STATICRR:
        if (!RRRA) break;
        for (j = RABBIT; j <= RABBIT + 54; j++) tloadtile(j,1);
        for (j = RABBIT + 56; j <= RABBIT + 56 + 49; j++) tloadtile(j,1);
        for (j = RABBIT + 56; j <= RABBIT + 56 + 49; j++) tloadtile(j,1);
        maxc = 0;
        break;
    case BIKERB__STATICRR:
    case BIKERBV2__STATICRR:
        if (!RRRA) break;
        for (j = BIKERB; j <= BIKERB + 104; j++) tloadtile(j,1);
        maxc = 0;
        break;
    case BIKER__STATICRR:
        if (!RRRA) break;
        for (j = BIKER; j <= BIKER + 116; j++) tloadtile(j,1);
        for (j = BIKER + 150; j <= BIKER + 150 + 104; j++) tloadtile(j,1);
        maxc = 0;
        break;
    case CHEER__STATICRR:
        if (!RRRA) break;
        for (j = CHEER; j <= CHEER + 44; j++) tloadtile(j,1);
        for (j = CHEER + 47; j <= CHEER + 47 + 211; j++) tloadtile(j,1);
        for (j = CHEER + 262; j <= CHEER + 262 + 72; j++) tloadtile(j,1);
        maxc = 0;
        break;
    case CHEERB__STATICRR:
        if (!RRRA) break;
        for (j = CHEERB; j <= CHEERB + 157 + 83; j++) tloadtile(j,1);
        maxc = 0;
        break;
    case MAMA__STATICRR:
        if (!RRRA) break;
        for (j = MAMA; j <= MAMA + 78; j++) tloadtile(j,1);
        for (j = MAMA + 80; j <= MAMA + 80 + 7; j++) tloadtile(j,1);
        for (j = MAMA + 90; j <= MAMA + 90 + 94; j++) tloadtile(j,1);
        maxc = 0;
        break;
    case CHEERBOAT__STATICRR:
        if (!RRRA) break;
        tloadtile(CHEERBOAT,1);
        maxc = 0;
        break;
    case HULKBOAT__STATICRR:
        if (!RRRA) break;
        tloadtile(HULKBOAT,1);
        maxc = 0;
        break;
    case MINIONBOAT__STATICRR:
        if (!RRRA) break;
        tloadtile(MINIONBOAT,1);
        maxc = 0;
        break;
    case BILLYPLAY__STATICRR:
        if (!RRRA) break;
        for (j = BILLYPLAY; j <= BILLYPLAY + 2; j++) tloadtile(j,1);
        maxc = 0;
        break;
    case COOTPLAY__STATICRR:
        if (!RRRA) break;
        for (j = COOTPLAY; j <= COOTPLAY + 4; j++) tloadtile(j,1);
        maxc = 0;
        break;
    case PIG__STATICRR:
    case PIGSTAYPUT__STATICRR:
        maxc = 69;
        break;
    case TORNADO__STATICRR:
        maxc = 7;
        break;
    case HEN__STATICRR:
    case HENSTAND__STATICRR:
        maxc = 34;
        break;
    case FEMPIC1__STATIC:
        if (RR) break;
        maxc = 44;
        break;
    case LIZTROOP__STATIC:
    case LIZTROOPRUNNING__STATIC:
    case LIZTROOPSHOOT__STATIC:
    case LIZTROOPJETPACK__STATIC:
    case LIZTROOPONTOILET__STATIC:
    case LIZTROOPDUCKING__STATIC:
        if (RR) break;
        for (j = LIZTROOP; j < (LIZTROOP+72); j++) tloadtile(j,1);
        for (j=HEADJIB1; j<LEGJIB1+3; j++) tloadtile(j,1);
        maxc = 0;
        break;
    case WOODENHORSE__STATIC:
        if (RR) break;
        maxc = 5;
        for (j = HORSEONSIDE; j < (HORSEONSIDE+4); j++) tloadtile(j,1);
        break;
    case NEWBEAST__STATIC:
    case NEWBEASTSTAYPUT__STATIC:
        if (RR) break;
        maxc = 90;
        break;
    case BOSS1__STATIC:
    case BOSS2__STATIC:
    case BOSS3__STATIC:
        if (RR) break;
        maxc = 30;
        break;
    case OCTABRAIN__STATIC:
    case OCTABRAINSTAYPUT__STATIC:
    case COMMANDER__STATIC:
    case COMMANDERSTAYPUT__STATIC:
        if (RR) break;
        maxc = 38;
        break;
    case RECON__STATIC:
        if (RR) break;
        maxc = 13;
        break;
    case PIGCOP__STATIC:
    case PIGCOPDIVE__STATIC:
        if (RR) break;
        maxc = 61;
        break;
    case SHARK__STATIC:
        if (RR) break;
        maxc = 30;
        break;
    case LIZMAN__STATIC:
    case LIZMANSPITTING__STATIC:
    case LIZMANFEEDING__STATIC:
    case LIZMANJUMP__STATIC:
        if (RR) break;
        for (j=LIZMANHEAD1; j<LIZMANLEG1+3; j++) tloadtile(j,1);
        maxc = 80;
        break;
    case APLAYER__STATIC:
        maxc = 0;
        if ((g_netServer || ud.multimode > 1))
        {
            maxc = 5;
            if (RR)
            {
                for (j = APLAYER; j < APLAYER+220; j++) tloadtile(j,1);
                for (j = DUKEGUN; j < DUKELEG+4; j++) tloadtile(j,1);
            }
            else
                for (j = 1420; j < 1420+106; j++) tloadtile(j,1);
        }
        break;
    case ATOMICHEALTH__STATIC:
        maxc = 14;
        break;
    case DRONE__STATIC:
        maxc = RR ? 6 : 10;
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
        if (RR) break;
        for (j=CHAINGUN; j<=CHAINGUN+7; j++) tloadtile(j,1);
        break;
    case RPGSPRITE__STATIC:
        if (RR) break;
        for (j=RPGGUN; j<=RPGGUN+2; j++) tloadtile(j,1);
        break;
    case FREEZESPRITE__STATIC:
        if (RR) break;
        for (j=FREEZE; j<=FREEZE+5; j++) tloadtile(j,1);
        break;
    case GROWSPRITEICON__STATIC:
    case SHRINKERSPRITE__STATIC:
        if (RR) break;
        for (j=SHRINKER-2; j<=SHRINKER+5; j++) tloadtile(j,1);
        break;
    case HBOMBAMMO__STATIC:
    case HEAVYHBOMB__STATIC:
        if (RR) break;
        for (j=HANDREMOTE; j<=HANDREMOTE+5; j++) tloadtile(j,1);
        break;
    case TRIPBOMBSPRITE__STATIC:
        if (RR) break;
        for (j=HANDHOLDINGLASER; j<=HANDHOLDINGLASER+4; j++) tloadtile(j,1);
        break;
    case SHOTGUNSPRITE__STATIC:
        if (RR) break;
        tloadtile(SHOTGUNSHELL,1);
        for (j=SHOTGUN; j<=SHOTGUN+6; j++) tloadtile(j,1);
        break;
    case DEVISTATORSPRITE__STATIC:
        if (RR) break;
        for (j=DEVISTATOR; j<=DEVISTATOR+1; j++) tloadtile(j,1);
        break;
    case VIXEN__STATICRR:
        maxc = 214;
        for (j = PN(i); j < PN(i) + maxc; j++) tloadtile(j,1);
        maxc = 0;
        break;
    case SBMOVE__STATICRR:
        if (RRRA) break;
        maxc = 54;
        for (j = PN(i); j < PN(i) + maxc; j++) tloadtile(j,1);
        maxc = 100;
        for (j = SBMOVE; j < SBMOVE + maxc; j++) tloadtile(j,1);
        maxc = 0;
        break;
    case HULK__STATICRR:
        maxc = 40;
        for (j = PN(i) - 41; j < PN(i) + maxc - 41; j++) tloadtile(j,1);
        for (j = HULKJIBA; j <= HULKJIBC + 4; j++) tloadtile(j,1);
        maxc = 0;
        break;
    case MINION__STATICRR:
        maxc = 141;
        for (j = PN(i); j < PN(i) + maxc; j++) tloadtile(j,1);
        for (j = MINJIBA; j <= MINJIBC + 4; j++) tloadtile(j,1);
        maxc = 0;
        break;

    }

    for (j = PN(i); j < (PN(i)+maxc); j++) tloadtile(j,1);
}

static void G_PrecacheSprites(void)
{
    int32_t i;

    //for (i=0; i<MAXTILES; i++)
    //{
    //    if (g_tile[i].flags & SFLAG_PROJECTILE)
    //        tloadtile(i,1);
    //
    //    if (A_CheckSpriteTileFlags(i, SFLAG_CACHE))
    //        for (j = i; j <= g_tile[i].cacherange; j++)
    //            tloadtile(j,1);
    //}
    tloadtile(BOTTOMSTATUSBAR,1);
    if ((g_netServer || ud.multimode > 1))
        tloadtile(FRAGBAR,1);

    tloadtile(VIEWSCREEN,1);

    for (i=STARTALPHANUM; i<ENDALPHANUM+1; i++) tloadtile(i,1);
    for (i=BIGALPHANUM-11; i<BIGALPHANUM+82; i++) tloadtile(i,1);
    for (i=MINIFONT; i<MINIFONT+93; i++) tloadtile(i,1);

    for (i=FOOTPRINTS; i<FOOTPRINTS+3; i++) tloadtile(i,1);

    for (i = BURNING; i < BURNING+14; i++) tloadtile(i,1);

    for (i = FIRSTGUN; i < FIRSTGUN+(RR ? 10 : 3) ; i++) tloadtile(i,1);

    for (i = EXPLOSION2; i < EXPLOSION2+21 ; i++) tloadtile(i,1);

    for (i = COOLEXPLOSION1; i < COOLEXPLOSION1+21 ; i++) tloadtile(i,1);

    tloadtile(BULLETHOLE,1);
    tloadtile(BLOODPOOL,1);

    for (i = SMALLSMOKE; i < (SMALLSMOKE+4); i++) tloadtile(i,1);
    for (i = SHOTSPARK1; i < (SHOTSPARK1+4); i++) tloadtile(i,1);

    for (i = BLOOD; i < (BLOOD+4); i++) tloadtile(i,1);
    for (i = JIBS1; i < (JIBS5+5); i++) tloadtile(i,1);
    for (i = JIBS6; i < (JIBS6+8); i++) tloadtile(i,1);

    for (i = SCRAP1; i < (SCRAP1+(RR? 19 : 29)); i++) tloadtile(i,1);

    if (!RR)
    {
        for (i = BURNING2; i < BURNING2+14; i++) tloadtile(i,1);
        for (i = CRACKKNUCKLES; i < CRACKKNUCKLES+4; i++) tloadtile(i,1);
        for (i = FIRSTGUNRELOAD; i < FIRSTGUNRELOAD+8 ; i++) tloadtile(i,1);
        for (i = TRANSPORTERBEAM; i < (TRANSPORTERBEAM+6); i++) tloadtile(i,1);
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
    else
    {
        if (RRRA)
        {
            if (ud.volume_number == 0 && ud.level_number == 4)
                tloadtile(RRTILE2577, 1);
        }
        else
        {
            if (ud.volume_number == 1 && ud.level_number == 2)
            {
                tloadtile(RRTILE3190, 1);
                tloadtile(RRTILE3191, 1);
                tloadtile(RRTILE3192, 1);
                tloadtile(RRTILE3144, 1);
                tloadtile(RRTILE3139, 1);
                tloadtile(RRTILE3132, 1);
                tloadtile(RRTILE3120, 1);
                tloadtile(RRTILE3121, 1);
                tloadtile(RRTILE3122, 1);
                tloadtile(RRTILE3123, 1);
                tloadtile(RRTILE3124, 1);
            }
        }
        if (g_lastLevel)
        {
            tloadtile(UFO1, 1);
            tloadtile(UFO2, 1);
            tloadtile(UFO3, 1);
            tloadtile(UFO4, 1);
            tloadtile(UFO5, 1);
        }
    }
}

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
            twod->ClearScreen();
        }

        twod->ClearScreen();

        int const loadScreenTile = VM_OnEventWithReturn(EVENT_GETLOADTILE, g_player[screenpeek].ps->i, screenpeek, DEER ? 7040 : LOADSCREEN);

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
            twod->ClearScreen();
            //g_player[myconnectindex].ps->palette = palette;
            //G_FadePalette(0,0,0,0);
            P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308
        }
        /*Gv_SetVar(g_iReturnVarID,LOADSCREEN, -1, -1);*/

        rotatesprite_fs(320<<15,200<<15,65536L, 0,LOADSCREEN,0,0,2+8+64+BGSTRETCH);

        menutext_center(RRRA?155:105,RR? GStrings("TXT_LOADIN") : GStrings("TXT_Loading..."));
        if (statustext) gametext_center_number(180, statustext);
        videoNextPage();
    }
}




void G_CacheMapData(void)
{
    int32_t i,j,pc=0;
    int32_t tc;
    uint32_t starttime, endtime;

    if (ud.recstat == 2)
        return;

    S_TryPlaySpecialMusic(MUS_LOADING);

    starttime = timerGetTicks();

    cacheAllSounds();
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
        if (sector[i].ceilingpicnum == LA)  // JBF 20040509: if( w aloff[sector[i].ceilingpicnum] == LA) WTF?!?!?!?
        {
            tloadtile(LA+1, 0);
            tloadtile(LA+2, 0);
        }

        for (SPRITES_OF_SECT(i, j))
            if (sprite[j].xrepeat != 0 && sprite[j].yrepeat != 0 && (sprite[j].cstat&32768) == 0)
                G_CacheSpriteNum(j);
    }

    tc = (int32_t) totalclock;
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
			// For the hardware renderer precaching the raw pixel data is pointless.
			if (videoGetRenderMode() < REND_POLYMOST)
				tileLoad(i);

#ifdef USE_OPENGL
			if (r_precache) PrecacheHardwareTextures(i);
#endif
			j++;
            pc++;
        }
        else continue;

        if ((j&7) == 0)
            G_HandleAsync();

#if 0
        if (bpp > 8 && totalclock - tc > TICRATE/4)
        {
            /*Bsprintf(tempbuf,"%d resources remaining\n",g_precacheCount-pc+1);*/
            int percentage = min(100, tabledivide32_noinline(100 * pc, g_precacheCount));

            while (percentage > lpc)
            {
                G_HandleAsync();
                Bsprintf(tempbuf, "Loaded %d%% (%d/%d textures)\n", lpc, pc, g_precacheCount);
                G_DoLoadScreen(tempbuf, lpc);

                if (totalclock - tc >= 1)
                {
                    tc = (int32_t) totalclock;
                    lpc++;
                }

//                Printf("percentage %d lpc %d\n", percentage, lpc);
            }

            tc = (int32_t) totalclock;
        }
#endif
    }

    Bmemset(gotpic, 0, sizeof(gotpic));

    endtime = timerGetTicks();
    Printf("Cache time: %dms\n", endtime-starttime);
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

        int32_t y1 = scale(ss,(200 * 100) - ((tilesiz[BOTTOMSTATUSBAR].y >> (RR ? 1 : 0)) * ud.statusbarscale),200 - tilesiz[BOTTOMSTATUSBAR].y);
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
            y2 -= (tilesiz[BOTTOMSTATUSBAR].y >> (RR ? 1 : 0))*ud.statusbarscale;
        y1 = scale(y1,ydim,200*100);
        y2 = scale(y2,ydim,200*100);

        videoSetViewableArea(x1,y1,x2-1,y2-1);
    }

    pub = NUMPAGES;
    pus = NUMPAGES;
}

void P_RandomSpawnPoint(int playerNum)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

    int32_t i = playerNum;

    if ((g_netServer || ud.multimode > 1) && !(g_gametypeFlags[ud.coop] & GAMETYPE_FIXEDRESPAWN))
    {
        i = krand2() % g_playerSpawnCnt;

        if (g_gametypeFlags[ud.coop] & GAMETYPE_TDMSPAWN)
        {
            uint32_t pdist = -1;
            for (bssize_t j=0; j<ud.multimode; j++)
            {
                if (j != playerNum && g_player[j].ps->team == pPlayer->team && sprite[g_player[j].ps->i].extra > 0)
                {
                    for (bssize_t k=0; k<g_playerSpawnCnt; k++)
                    {
                        uint32_t dist = FindDistance2D(g_player[j].ps->pos.x - g_playerSpawnPoints[k].pos.x,
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
    pPlayer->q16ang       = fix16_from_int(g_playerSpawnPoints[i].ang);
    pPlayer->cursectnum = g_playerSpawnPoints[i].sect;

    sprite[pPlayer->i].cstat = 1 + 256;
}

static inline void P_ResetTintFade(DukePlayer_t *const pPlayer)
{
    pPlayer->pals.f = 0;
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
    pSprite->clipdist = RR ? 32 : 64;
    pSprite->xrepeat  = RR ? 24 : 42;
    pSprite->yrepeat  = RR ? 17 : 36;
    pSprite->owner    = pPlayer->i;
    pSprite->xoffset  = 0;
    pSprite->pal      = pPlayer->palookup;

    pPlayer->last_extra = pSprite->extra = pPlayer->max_player_health;

    pPlayer->wantweaponfire         = -1;
    pPlayer->q16horiz                  = F16(100);
    pPlayer->on_crane               = -1;
    pPlayer->frag_ps                = playerNum;
    pPlayer->q16horizoff               = 0;
    pPlayer->opyoff                 = 0;
    pPlayer->wackedbyactor          = -1;
    pPlayer->inv_amount[GET_SHIELD] = g_startArmorAmount;
    pPlayer->dead_flag              = 0;
    pPlayer->footprintcount         = 0;
    pPlayer->weapreccnt             = 0;
    pPlayer->fta                    = 0;
    pPlayer->ftq                    = 0;
    pPlayer->vel.x = pPlayer->vel.y = 0;
    if (!RR) pPlayer->q16rotscrnang          = 0;
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

    //pPlayer->reloading     = 0;
    pPlayer->movement_lock = 0;
}

void P_ResetStatus(int playerNum)
{
    auto &     thisPlayer = g_player[playerNum];
    auto const pPlayer    = thisPlayer.ps;

    gFullMap              = 0;
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
    pPlayer->q16angvel           = 0;
    pPlayer->weapon_sway       = 0;
    pPlayer->extra_extra8      = 0;
    pPlayer->show_empty_weapon = 0;
    pPlayer->dummyplayersprite = -1;
    pPlayer->crack_time        = 0;
    pPlayer->hbomb_hold_delay  = 0;
    pPlayer->transporter_hold  = 0;
    //pPlayer->clipdist          = 164;
    pPlayer->wantweaponfire    = -1;
    pPlayer->hurt_delay        = 0;
    if (RRRA)
        pPlayer->hurt_delay2   = 0;
    pPlayer->footprintcount    = 0;
    pPlayer->footprintpal      = 0;
    pPlayer->footprintshade    = 0;
    pPlayer->jumping_toggle    = 0;
    pPlayer->oq16horiz           = F16(140);
    pPlayer->q16horiz            = F16(140);
    pPlayer->q16horizoff         = 0;
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
    //pPlayer->sbs               = 0;
    pPlayer->palette           = BASEPAL;

    if (pPlayer->inv_amount[GET_STEROIDS] < 400)
    {
        pPlayer->inv_amount[GET_STEROIDS] = 0;
        pPlayer->inven_icon = ICON_NONE;
    }

    pPlayer->heat_on           = 0;
    pPlayer->jetpack_on        = 0;
    pPlayer->holoduke_on       = -1;
    pPlayer->q16look_ang       = fix16_from_int(512 - ((ud.level_number & 1) << 10));
    pPlayer->q16rotscrnang     = 0;
    pPlayer->oq16rotscrnang    = fix16_one;  // JBF 20031220
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

    pPlayer->kickback_pic = (pPlayer->curr_weapon == PISTOL_WEAPON) ? (RR ? 22 : 5) : 0;

    pPlayer->weapon_pos         = WEAPON_POS_START;
    pPlayer->walking_snd_toggle = 0;
    pPlayer->weapon_ang         = 0;
    pPlayer->knuckle_incs       = 1;
    pPlayer->fist_incs          = 0;
    pPlayer->knee_incs          = 0;
    //pPlayer->reloading          = 0;
    pPlayer->movement_lock      = 0;
    pPlayer->frag_ps            = playerNum;

    thisPlayer.smoothcamera     = false;
    thisPlayer.horizRecenter    = false;
    thisPlayer.horizSkew        = 0;
    thisPlayer.horizAngleAdjust = 0;

    P_UpdateScreenPal(pPlayer);

    if (RR)
    {
        pPlayer->stairs = 0;
        pPlayer->noise_x = 0;
        pPlayer->noise_y = 0;
        pPlayer->make_noise = 0;
        pPlayer->noise_radius = 0;
        if ((g_netServer || ud.multimode > 1) && (g_gametypeFlags[ud.coop] & GAMETYPE_ACCESSATSTART))
        {
            pPlayer->keys[0] = 1;
            pPlayer->keys[1] = 1;
            pPlayer->keys[2] = 1;
            pPlayer->keys[3] = 1;
            pPlayer->keys[4] = 1;
        }
        else
        {
            pPlayer->keys[0] = 0;
            pPlayer->keys[1] = 0;
            pPlayer->keys[2] = 0;
            pPlayer->keys[3] = 0;
            pPlayer->keys[4] = 0;
        }

        g_wupass = 0;
        pPlayer->drink_ang = pPlayer->eat_ang = 1647;
        pPlayer->drink_amt = pPlayer->eat_amt = 0;
        pPlayer->drink_timer = pPlayer->eat_timer = 4096;
        pPlayer->shotgun_state[0] = pPlayer->shotgun_state[1] = 0;
        pPlayer->hbomb_time = 0;
        pPlayer->hbomb_offset = 0;
        pPlayer->recoil = 0;
        pPlayer->yehaa_timer = 0;
        if (RRRA)
        {
            g_chickenWeaponTimer = 0;
            if (pPlayer->on_motorcycle)
            {
                pPlayer->on_motorcycle = 0;
                pPlayer->gotweapon &= ~(1 << MOTORCYCLE_WEAPON);
                pPlayer->curr_weapon = SLINGBLADE_WEAPON;
            }
            pPlayer->lotag800kill = 0;
            pPlayer->moto_do_bump = 0;
            pPlayer->moto_on_ground = 1;
            pPlayer->moto_underwater = 0;
            pPlayer->moto_speed = 0;
            pPlayer->tilt_status = 0;
            pPlayer->moto_drink = 0;
            pPlayer->moto_bump_target = 0;
            pPlayer->moto_bump = 0;
            pPlayer->moto_bump_fast = 0;
            pPlayer->moto_turb = 0;
            pPlayer->moto_on_mud = 0;
            pPlayer->moto_on_oil = 0;
            if (pPlayer->on_boat)
            {
                pPlayer->on_boat = 0;
                pPlayer->gotweapon &= ~(1 << BOAT_WEAPON);
                pPlayer->curr_weapon = SLINGBLADE_WEAPON;
            }
            pPlayer->not_on_water = 0;
            pPlayer->sea_sick = 0;
            pPlayer->nocheat = 0;
            pPlayer->drug_mode = 0;
            pPlayer->drug_stat[0] = 0;
            pPlayer->drug_stat[1] = 0;
            pPlayer->drug_stat[2] = 0;
            pPlayer->drug_aspect = 0;
        }
        A_ResetLanePics();
        if (!g_netServer && numplayers < 2)
        {
            g_ufoSpawn = min(RRRA ? 3 : (ud.m_player_skill*4+1), 32);
            g_ufoCnt = 0;
            g_hulkSpawn = ud.m_player_skill + 1;
        }
        else
        {
            g_ufoSpawn = 32;
            g_ufoCnt = 0;
            g_hulkSpawn = 2;
        }
    }
}

void P_ResetWeapons(int playerNum)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

    for (bssize_t weaponNum = PISTOL_WEAPON; weaponNum < MAX_WEAPONS; weaponNum++)
        pPlayer->ammo_amount[weaponNum] = 0;

    pPlayer->weapon_pos                 = WEAPON_POS_START;
    pPlayer->curr_weapon                = PISTOL_WEAPON;
    pPlayer->kickback_pic               = 5;
    pPlayer->gotweapon                  = ((1 << PISTOL_WEAPON) | (1 << KNEE_WEAPON) | (1 << HANDREMOTE_WEAPON));
    pPlayer->ammo_amount[PISTOL_WEAPON] = min<int16_t>(pPlayer->max_ammo_amount[PISTOL_WEAPON], 48);
    if (RRRA)
    {
        g_chickenWeaponTimer = 0;
        pPlayer->gotweapon |= (1 << SLINGBLADE_WEAPON);
        pPlayer->ammo_amount[KNEE_WEAPON] = 1;
        pPlayer->ammo_amount[SLINGBLADE_WEAPON] = 1;
        pPlayer->on_motorcycle = 0;
        pPlayer->moto_underwater = 0;
        pPlayer->on_boat = 0;
        pPlayer->lotag800kill = 0;
    }
    pPlayer->last_weapon                = -1;
    pPlayer->show_empty_weapon          = 0;
    pPlayer->last_pissed_time           = 0;
    pPlayer->holster_weapon             = 0;
    pPlayer->last_used_weapon           = -1;
    
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

    if (RR)
    {
        if ((g_netServer || ud.multimode > 1) && (g_gametypeFlags[ud.coop] & GAMETYPE_ACCESSATSTART))
        {
            pPlayer->keys[0] = 1;
            pPlayer->keys[1] = 1;
            pPlayer->keys[2] = 1;
            pPlayer->keys[3] = 1;
            pPlayer->keys[4] = 1;
        }
        else
        {
            pPlayer->keys[0] = 0;
            pPlayer->keys[1] = 0;
            pPlayer->keys[2] = 0;
            pPlayer->keys[3] = 0;
            pPlayer->keys[4] = 0;
        }

        pPlayer->drink_ang = pPlayer->eat_ang = 1647;
        pPlayer->drink_amt = pPlayer->eat_amt = 0;
        pPlayer->drink_timer = pPlayer->eat_timer = 4096;
        pPlayer->shotgun_state[0] = pPlayer->shotgun_state[1] = 0;
        pPlayer->hbomb_time = 0;
        pPlayer->hbomb_offset = 0;
        pPlayer->recoil = 0;
        pPlayer->yehaa_timer = 0;
        A_ResetLanePics();
        if (!g_netServer && numplayers < 2)
        {
            g_ufoSpawn = min(ud.m_player_skill*4+1, 32);
            g_ufoCnt = 0;
            g_hulkSpawn = ud.m_player_skill + 1;
        }
        else
        {
            g_ufoSpawn = 32;
            g_ufoCnt = 0;
            g_hulkSpawn = 2;
        }
    }

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

    pPlayer->kickback_pic = 5;

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
    randomseed         = 17;
    paused             = 0;
    ud.camerasprite    = -1;
    ud.eog             = 0;
    tempwallptr        = 0;
    g_curViewscreen    = -1;
    g_earthquakeTime   = 0;
    g_interpolationCnt = 0;

    if (RRRA)
    {
        g_windTime = 0;
        g_windDir = 0;
        g_fakeBubbaCnt = 0;
        g_RAendLevel = 0;
        g_bellTime = 0;
        g_bellSprite = 0;
    }

    if (((gameMode & MODE_EOL) != MODE_EOL && numplayers < 2 && !g_netServer)
        || (!(g_gametypeFlags[ud.coop] & GAMETYPE_PRESERVEINVENTORYDEATH) && numplayers > 1))
    {
        P_ResetWeapons(playerNum);
        P_ResetInventory(playerNum);
    }
    else if (pPlayer->curr_weapon == HANDREMOTE_WEAPON)
    {
        pPlayer->ammo_amount[HANDBOMB_WEAPON]++;
        pPlayer->curr_weapon = HANDBOMB_WEAPON;
    }

    pPlayer->timebeforeexit  = 0;
    pPlayer->customexitsound = 0;

    if (RR)
    {
        pPlayer->stairs = 0;
        pPlayer->noise_x = 131072;
        pPlayer->noise_y = 131072;
        pPlayer->make_noise = 0;
        pPlayer->noise_radius = 0;
        if ((g_netServer || ud.multimode > 1) && (g_gametypeFlags[ud.coop] & GAMETYPE_ACCESSATSTART))
        {
            pPlayer->keys[0] = 1;
            pPlayer->keys[1] = 1;
            pPlayer->keys[2] = 1;
            pPlayer->keys[3] = 1;
            pPlayer->keys[4] = 1;
        }
        else
        {
            pPlayer->keys[0] = 0;
            pPlayer->keys[1] = 0;
            pPlayer->keys[2] = 0;
            pPlayer->keys[3] = 0;
            pPlayer->keys[4] = 0;
        }

        pPlayer->drink_ang = pPlayer->eat_ang = 1647;
        pPlayer->drink_amt = pPlayer->eat_amt = 0;
        pPlayer->drink_timer = pPlayer->eat_timer = 4096;
        pPlayer->shotgun_state[0] = pPlayer->shotgun_state[1] = 0;
        pPlayer->hbomb_time = 0;
        pPlayer->hbomb_offset = 0;
        pPlayer->recoil = 0;
        pPlayer->yehaa_timer = 0;
        A_ResetLanePics();
        if (!g_netServer && numplayers < 2)
        {
            g_ufoSpawn = min(ud.m_player_skill*4+1, 32);
            g_ufoCnt = 0;
            g_hulkSpawn = ud.m_player_skill + 1;
        }
        else
        {
            g_ufoSpawn = 32;
            g_ufoCnt = 0;
            g_hulkSpawn = 2;
        }
    }
}

static inline int G_CheckExitSprite(int spriteNum) { return ((uint16_t)sprite[spriteNum].lotag == UINT16_MAX && (sprite[spriteNum].cstat & 16)); }

void G_InitRRRASkies(void)
{
    if (!RRRA)
        return;
    
    for (bssize_t i = 0; i < MAXSECTORS; i++)
    {
        if (sector[i].ceilingpicnum != LA && sector[i].ceilingpicnum != MOONSKY1 && sector[i].ceilingpicnum != BIGORBIT1)
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
        G_SetFog(0);
        g_fogType = 0;
        g_ufoSpawnMinion = 0;
        g_pistonSound = 0;
        g_slotWin = 0;
        g_changeEnemySize = 0;
        g_player[myconnectindex].ps->level_end_timer = 0;
        g_mamaSpawnCnt = 15;
        g_banjoSong = 0;
        g_RAendLevel = 0;
        if (!DEER)
        {
            for (bssize_t TRAVERSE_CONNECT(playerNum))
            {
                DukePlayer_t *ps = g_player[playerNum].ps;
                ps->sea_sick_stat = 0;
                if (ud.level_number == 4 && ud.volume_number == 1)
                    ps->inv_amount[GET_STEROIDS] = 0;
            }
            if (ud.level_number == 3 && ud.volume_number == 0)
                g_mamaSpawnCnt = 5;
            else if (ud.level_number == 2 && ud.volume_number == 1)
                g_mamaSpawnCnt = 10;
            else if (ud.level_number == 6 && ud.volume_number == 1)
                g_mamaSpawnCnt = 15;
        }
    }

    Bmemset(g_spriteExtra, 0, sizeof(g_spriteExtra));
    Bmemset(g_sectorExtra, 0, sizeof(g_sectorExtra));
    Bmemset(g_shadedSector, 0, sizeof(g_shadedSector));
    Bmemset(g_geoSectorWarp, -1, sizeof(g_geoSectorWarp));
    Bmemset(g_geoSectorWarp2, -1, sizeof(g_geoSectorWarp2));
    Bmemset(g_ambientHitag, -1, sizeof(g_ambientHitag));
    Bmemset(g_ambientLotag, -1, sizeof(g_ambientLotag));
    show2dsector.Zero();
#ifdef LEGACY_ROR
    Bmemset(ror_protectedsectors, 0, MAXSECTORS);
#endif
    resetprestat(0,g);
    if (RR)
    {
        g_lightninCnt = 0;
        g_torchCnt = 0;
        g_geoSectorCnt = 0;
        g_jailDoorCnt = 0;
        g_mineCartCnt = 0;
        g_ambientCnt = 0;
        thunderon = 0;
        g_chickenPlant = 0;
        if (RRRA)
        {
            g_windTime = 0;
            g_windDir = 0;
            g_fakeBubbaCnt = 0;
            g_RAendLevel = 0;
            g_mamaSpawnCnt = 15; // ???
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
        if (RR && sector[i].ceilingpicnum == RRTILE2577)
            thunderon = 1;
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
                if (sprite[k].picnum == RRTILE11)
                {
                    p1 = sprite[k].lotag << 4;
                    p2 = sprite[k].hitag;
                    A_DeleteSprite(k);
                }
                if (sprite[k].picnum == RRTILE38)
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
                    if (g_jailDoorCnt >= 32)
                        G_GameExit("\nToo many jaildoor sectors");
                    g_jailDoorDist[g_jailDoorCnt] = p1;
                    g_jailDoorSpeed[g_jailDoorCnt] = p2;
                    g_jailDoorSecHitag[g_jailDoorCnt] = sector[i].hitag;
                    g_jailDoorSect[g_jailDoorCnt] = j;
                    g_jailDoorDrag[g_jailDoorCnt] = 0;
                    g_jailDoorOpen[g_jailDoorCnt] = 0;
                    g_jailDoorDir[g_jailDoorCnt] = sector[j].lotag;
                    g_jailDoorSound[g_jailDoorCnt] = p3;
                    g_jailDoorCnt++;
                }
            }
            break;
        }
        case 42:
        {
            if (!RR) break;
            int k = headspritesect[i];
            while (k != -1)
            {
                int const nexti = nextspritesect[k];
                if (sprite[k].picnum == RRTILE64)
                {
                    p1 = sprite[k].lotag << 4;
                    p2 = sprite[k].hitag;
                    for (bssize_t kk = 0; kk < MAXSPRITES; kk++)
                    {
                        if (sprite[kk].picnum == RRTILE66)
                            if (sprite[kk].lotag == sprite[k].sectnum)
                            {
                                g_mineCartChildSect[g_mineCartCnt] = sprite[kk].sectnum;
                                A_DeleteSprite(kk);
                            }
                    }
                    A_DeleteSprite(k);
                }
                if (sprite[k].picnum == RRTILE65)
                {
                    p3 = sprite[k].lotag;
                    A_DeleteSprite(k);
                }
                k = nexti;
            }
            if (g_mineCartCnt >= 16)
                G_GameExit("\nToo many minecart sectors");
            g_mineCartDist[g_mineCartCnt] = p1;
            g_mineCartSpeed[g_mineCartCnt] = p2;
            g_mineCartSect[g_mineCartCnt] = i;
            g_mineCartDir[g_mineCartCnt] = sector[i].hitag;
            g_mineCartDrag[g_mineCartCnt] = p1;
            g_mineCartOpen[g_mineCartCnt] = 1;
            g_mineCartSound[g_mineCartCnt] = p3;
            g_mineCartCnt++;
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
            if (tilePtr(sector[i].ceilingpicnum) == nullptr)
            {
                if (sector[i].ceilingpicnum == LA)
                    for (bsize_t j = 0; j < 5; j++)
                        tloadtile(sector[i].ceilingpicnum + j, 0);
            }

            if (!RR && sector[i].ceilingpicnum == CLOUDYSKIES)
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
        Printf(TEXTCOLOR_RED "Map warning: have %d unhandled CLOUDYSKIES ceilings.\n", missedCloudSectors);

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
                if (g_torchCnt >= 64)
                    G_GameExit("\nToo many torch effects");
                g_torchSector[g_torchCnt] = SECT(i);
                g_torchSectorShade[g_torchCnt] = sector[SECT(i)].floorshade;
                g_torchType[g_torchCnt] = SLT(i);
                g_torchCnt++;
                A_DeleteSprite(i);
                break;

            case RRTILE35__STATICRR:
                if (g_lightninCnt >= 64)
                    G_GameExit("\nToo many lightnin effects");
                g_lightninSector[g_lightninCnt] = SECT(i);
                g_lightninSectorShade[g_lightninCnt] = sector[SECT(i)].floorshade;
                g_lightninCnt++;
                A_DeleteSprite(i);
                break;

            case RRTILE68__STATICRR:
                g_shadedSector[SECT(i)] = 1;
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
                    g_ambientHitag[g_ambientCnt] = SHT(i);
                    g_ambientLotag[g_ambientCnt] = SLT(i);
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
            if (sprite[i].picnum == RRTILE19)
            {
                if (sprite[i].hitag == 0)
                {
                    if (g_geoSectorCnt >= MAXGEOSECTORS)
                        G_GameExit("\nToo many geometry effects");
                    g_geoSector[g_geoSectorCnt] = sprite[i].sectnum;
                    for (bssize_t j = 0; j < MAXSPRITES; j++)
                    {
                        if (sprite[i].lotag == sprite[j].lotag && i != j && sprite[j].picnum == RRTILE19)
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
        if (sprite[i].statnum < MAXSTATUS && (DEER || PN(i) != SECTOREFFECTOR || SLT(i) != SE_14_SUBWAY_CAR))
            A_Spawn(-1, i);
    }

    if (!DEER)
    for (size_t i = 0; i < MAXSPRITES; i++)
    {
        if (sprite[i].statnum < MAXSTATUS && PN(i) == SECTOREFFECTOR && SLT(i) == SE_14_SUBWAY_CAR)
            A_Spawn(-1, i);
        if (RR && sprite[i].picnum == RRTILE19)
            A_DeleteSprite(i);
        if (RR && sprite[i].picnum == RRTILE34)
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
                if (RR && !RRRA && PN(i)-1+ii == (uint32_t)RRTILE8464) break;
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

        if (!DEER && pWall->overpicnum == MIRROR && (pWall->cstat&32) != 0)
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

        if (DEER)
        {
            pWall->extra = -1;
            continue;
        }

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

            case RRTILE1814__STATICRR:
            case RRTILE1817__STATICRR:
                tloadtile(pWall->picnum, 0);
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
                tloadtile(pWall->picnum, 0);
                break;


            case TECHLIGHT2__STATIC:
            case TECHLIGHT4__STATIC: tloadtile(pWall->picnum, 0); break;
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
                for (bssize_t j = SCREENBREAK6; j < SCREENBREAK9; j++)
                    tloadtile(j, 0);

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
            wall[j].picnum     = MIRROR;
            wall[j].overpicnum = MIRROR;
        }
    }

    if (RR && !thunderon)
    {
        videoSetPalette(BASEPAL);
        videoclearFade();
        g_visibility = g_player[screenpeek].ps->visibility;
    }
    if (RR)
    {
		tileDelete(0);
    }

    G_SetupGlobalPsky();

    if (DEER)
        sub_52BA8();
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

    if (RR && !RRRA && g_turdLevel && !g_lastLevel)
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
        S_PlaySpecialMusicOrNothing(MUS_BRIEFING);

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

    Gv_ResetVars();
    Gv_InitWeaponPointers();
    Gv_RefreshPointers();
    Gv_ResetSystemDefaults();

    //AddLog("Newgame");

    for (bssize_t i=0; i<(MAXVOLUMES*MAXLEVELS); i++)
        G_FreeMapState(i);

    if (m_coop != 1)
    {
        for (bssize_t weaponNum = 0; weaponNum < MAX_WEAPONS; weaponNum++)
        {
            auto const worksLike = WW2GI ? PWEAPON(0, weaponNum, WorksLike) : weaponNum;
            if (worksLike == PISTOL_WEAPON)
            {
                pPlayer->curr_weapon = weaponNum;
                pPlayer->gotweapon |= (1 << weaponNum);
                pPlayer->ammo_amount[weaponNum] = min<int16_t>(pPlayer->max_ammo_amount[weaponNum], 48);
            }
            else if (worksLike == KNEE_WEAPON || (!RR && worksLike == HANDREMOTE_WEAPON) || (RRRA && worksLike == SLINGBLADE_WEAPON))
            {
                pPlayer->gotweapon |= (1 << weaponNum);
                if (RRRA)
                    pPlayer->ammo_amount[KNEE_WEAPON] = 1;
            }
        }
        pPlayer->last_weapon = -1;
    }

    display_mirror = 0;
}

static void resetpspritevars(char gameMode)
{
    int16_t i, j; //circ;

    uint8_t aimmode[MAXPLAYERS],autoaim[MAXPLAYERS],weaponswitch[MAXPLAYERS];
    DukeStatus_t tsbar[MAXPLAYERS];

    if (g_player[0].ps->cursectnum >= 0)  // < 0 may happen if we start a map in void space (e.g. testing it)
    {
        A_InsertSprite(g_player[0].ps->cursectnum,g_player[0].ps->pos.x,g_player[0].ps->pos.y,g_player[0].ps->pos.z,
                       APLAYER,0,0,0,fix16_to_int(g_player[0].ps->q16ang),0,0,0,10);
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
            s->xrepeat = RR ? 24 : 42;
            s->yrepeat = RR ? 17 : 36;
            //s->xrepeat = 42;
            //s->yrepeat = 36;
            if (!g_fakeMultiMode)
                s->cstat = j < numplayers ? 1+256 : 32768;
            else
                s->cstat = j < ud.multimode ? 1+256 : 32768;
            s->xoffset = 0;
            s->clipdist = 64;

            if (j < g_mostConcurrentPlayers)
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
                g_player[j].ps->oq16ang = g_player[j].ps->q16ang = fix16_from_int(s->ang);

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

        if (inputState.GetKeyStatus(sc_Space))
        {
            inputState.ClearKeyStatus(sc_Space);
            return;
        }

        setpalettefade(r,g,b,start);
        flushperms();
        G_DoLoadScreen(" ", tc);
    }
}
#endif

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
        S_ResumeSound(false);
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
		G_LoadMapHack(boardfilename);
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
		G_LoadMapHack(mi.fileName);
    }

    if (RR && !RRRA && ud.volume_number == 1 && ud.level_number == 1)
    {
        for (bssize_t i = PISTOL_WEAPON; i < MAX_WEAPONS; i++)
            g_player[0].ps->ammo_amount[i] = 0;
        g_player[0].ps->gotweapon &= (1<<KNEE_WEAPON);
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
        g_player[0].ps->gotweapon &= (1<<KNEE_WEAPON);
        g_player[0].ps->gotweapon |= (1<<SLINGBLADE_WEAPON);
        g_player[0].ps->ammo_amount[SLINGBLADE_WEAPON] = 1;
        g_player[0].ps->curr_weapon = SLINGBLADE_WEAPON;
    }

    G_AlignWarpElevators();
    resetpspritevars(gameMode);

    ud.playerbest = CONFIG_GetMapBestTime(Menu_HaveUserMap() ? boardfilename : mi.fileName.GetChars(), g_loadedMapHack.md4);

    // G_FadeLoad(0,0,0, 252,0, -28, 4, -1);
    G_CacheMapData();
    // G_FadeLoad(0,0,0, 0,252, 28, 4, -2);

    // Try this first so that it can disable the CD player if no tracks are found.
    if (RR && !(gameMode & MODE_DEMO))
        S_PlayRRMusic();

    if (ud.recstat != 2)
    {
        if (Menu_HaveUserMap())
        {
            S_PlayLevelMusicOrNothing(USERMAPMUSICFAKESLOT);
        }
        else S_PlayLevelMusicOrNothing(mii);
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
    renderFlushPerms();

    // reset lastInputTicks.
    g_player[myconnectindex].lastInputTicks = 0;

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


    if (G_HaveUserMap())
    {
        Printf(TEXTCOLOR_GOLD "%s: %s\n", GStrings("TXT_USERMAP"), boardfilename);
    }
    else
    {
        Printf(TEXTCOLOR_GOLD "E%dL%d: %s\n", ud.volume_number+1, ud.level_number+1,
                   mapList[mii].DisplayName());
    }

    g_restorePalette = -1;

    G_UpdateScreenArea();
    videoClearViewableArea(0L);
    G_DrawBackground();
    G_DrawRooms(myconnectindex,65536);

    Net_WaitForEverybody();
    return 0;
}

void G_FreeMapState(int levelNum)
{
    map_t *const pMapInfo = &g_mapInfo[levelNum];

    if (pMapInfo->savedstate == NULL)
        return;

    ALIGNED_FREE_AND_NULL(pMapInfo->savedstate);
}

void G_SetFog(int fogtype)
{
    GLInterface.SetMapFog(fogtype != 0);
}

END_RR_NS
