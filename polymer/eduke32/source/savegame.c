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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include "duke3d.h"
#include "gamedef.h"
#include "premap.h"
#include "menus.h"
#include "prlights.h"

extern char *bitptr;

#define BITPTR_POINTER 1

void ReadSaveGameHeaders(void)
{
    int32_t dummy,j;
    int32_t i;
    char fn[13];
    int32_t fil;

    Bstrcpy(fn,"egam_.sav");

    for (i=0; i<10; i++)
    {
        fn[4] = i+'0';
        if ((fil = kopen4loadfrommod(fn,0)) == -1) continue;
        if (kdfread(&j,sizeof(int32_t),1,fil) != 1)
        {
            kclose(fil);
            continue;
        }
        if (kdfread(g_szBuf,j,1,fil) != 1)
        {
            kclose(fil);
            continue;
        }
        if (kdfread(&dummy,sizeof(dummy),1,fil) != 1)
        {
            kclose(fil);
            continue;
        }
        if (dummy != BYTEVERSION)
        {
            kclose(fil);
            continue;
        }
        if (kdfread(&dummy,sizeof(dummy),1,fil) != 1)
        {
            kclose(fil);
            continue;
        }
        if (kdfread(&ud.savegame[i][0],21,1,fil) != 1)
        {
            ud.savegame[i][0] = 0;
        }
        else ud.savegame[i][19] = 0;

        kclose(fil);
    }
}

int32_t G_LoadSaveHeader(char spot,struct savehead *saveh)
{
    char fn[13];
    int32_t fil;
    int32_t bv;

    strcpy(fn, "egam0.sav");
    fn[4] = spot+'0';

    if ((fil = kopen4loadfrommod(fn,0)) == -1) return(-1);

    walock[TILE_LOADSHOT] = 255;

    if (kdfread(&bv,sizeof(bv),1,fil) != 1) goto corrupt;
    if (kdfread(g_szBuf,bv,1,fil) != 1) goto corrupt;
    g_szBuf[bv]=0;
    //    AddLog(g_szBuf);

    if (kdfread(&bv,sizeof(bv),1,fil) != 1) goto corrupt;
    /*    if (bv != BYTEVERSION)
        {
            P_DoQuote(114,g_player[myconnectindex].ps);
            kclose(fil);
            return 1;
        }*/

    if (kdfread(&saveh->numplr,sizeof(int32_t),1,fil) != 1) goto corrupt;

    if (kdfread(saveh->name,21,1,fil) != 1) goto corrupt;
    if (kdfread(&saveh->volnum,sizeof(int32_t),1,fil) != 1) goto corrupt;
    if (kdfread(&saveh->levnum,sizeof(int32_t),1,fil) != 1) goto corrupt;
    if (kdfread(&saveh->plrskl,sizeof(int32_t),1,fil) != 1) goto corrupt;
    if (kdfread(saveh->boardfn,BMAX_PATH,1,fil) != 1) goto corrupt;

    if (waloff[TILE_LOADSHOT] == 0) allocache(&waloff[TILE_LOADSHOT],320*200,&walock[TILE_LOADSHOT]);
    tilesizx[TILE_LOADSHOT] = 200;
    tilesizy[TILE_LOADSHOT] = 320;
    if (kdfread((char *)waloff[TILE_LOADSHOT],320,200,fil) != 200) goto corrupt;
    invalidatetile(TILE_LOADSHOT,0,255);

    kclose(fil);

    return(0);
corrupt:
    kclose(fil);
    return 1;
}

int32_t G_LoadPlayer(int32_t spot)
{
    int32_t k;
    char fn[13];
    char mpfn[13];
    char *fnptr, *scriptptrs;
    int32_t fil, bv, i, x;
    intptr_t j;
    int32_t nump;

    strcpy(fn, "egam0.sav");
    strcpy(mpfn, "egamA_00.sav");

    fnptr = fn;
    fn[4] = spot + '0';

    if ((fil = kopen4loadfrommod(fnptr,0)) == -1) return(-1);

    ready2send = 0;

    if (kdfread(&bv,sizeof(bv),1,fil) != 1) goto corrupt;
    if (kdfread(g_szBuf,bv,1,fil) != 1) goto corrupt;
    g_szBuf[bv]=0;
    //    AddLog(g_szBuf);

    if (kdfread(&bv,sizeof(bv),1,fil) != 1) return -1;
    if (bv != BYTEVERSION)
    {
        P_DoQuote(114,g_player[myconnectindex].ps);
        kclose(fil);
        ototalclock = totalclock;
        ready2send = 1;
        return 1;
    }

    if (kdfread(&nump,sizeof(nump),1,fil) != 1) return -1;
    if (nump != ud.multimode)
    {
        kclose(fil);
        ototalclock = totalclock;
        ready2send = 1;
        P_DoQuote(124,g_player[myconnectindex].ps);
        return 1;
    }
    else ud.multimode = nump;

    if (numplayers > 1)
    {
        pub = NUMPAGES;
        pus = NUMPAGES;
        G_UpdateScreenArea();
        G_DrawBackground();
        menutext(160,100,0,0,"LOADING...");
        nextpage();
    }

    Net_WaitForServer();

    FX_StopAllSounds();
    S_ClearSoundLocks();

    if (numplayers > 1)
    {
        if (kdfread(&buf,21,1,fil) != 1) goto corrupt;
    }
    else
    {
        if (kdfread(&ud.savegame[spot][0],21,1,fil) != 1) goto corrupt;
        ud.savegame[spot][19] = 0;
    }


    if (kdfread(&ud.volume_number,sizeof(ud.volume_number),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.level_number,sizeof(ud.level_number),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.player_skill,sizeof(ud.player_skill),1,fil) != 1) goto corrupt;
    if (kdfread(&boardfilename[0],BMAX_PATH,1,fil) != 1) goto corrupt;

    currentboardfilename[0] = 0;

    if (boardfilename[0])
        strcpy(currentboardfilename, boardfilename);
    else if (MapInfo[(ud.volume_number * MAXLEVELS) + ud.level_number].filename)
        strcpy(currentboardfilename, MapInfo[(ud.volume_number * MAXLEVELS) + ud.level_number].filename);

    if (currentboardfilename[0])
    {
        char *p;

        p = Bstrrchr(currentboardfilename,'.');
        if (!p) strcat(currentboardfilename,".mhk");
        else
        {
            p[1]='m';
            p[2]='h';
            p[3]='k';
            p[4]=0;
        }

        loadmaphack(currentboardfilename);
    }

    Bmemcpy(&currentboardfilename[0],&boardfilename[0],BMAX_PATH);

    ud.m_level_number = ud.level_number;
    ud.m_volume_number = ud.volume_number;
    ud.m_player_skill = ud.player_skill;

    //Fake read because lseek won't work with compression
    walock[TILE_LOADSHOT] = 1;
    if (waloff[TILE_LOADSHOT] == 0) allocache(&waloff[TILE_LOADSHOT],320*200,&walock[TILE_LOADSHOT]);
    tilesizx[TILE_LOADSHOT] = 200;
    tilesizy[TILE_LOADSHOT] = 320;
    if (kdfread((char *)waloff[TILE_LOADSHOT],320,200,fil) != 200) goto corrupt;
    invalidatetile(TILE_LOADSHOT,0,255);

    if (kdfread(&numwalls,sizeof(numwalls),1,fil) != 1) goto corrupt;
    if (kdfread(&wall[0],sizeof(walltype),MAXWALLS,fil) != MAXWALLS) goto corrupt;
    if (kdfread(&numsectors,sizeof(numsectors),1,fil) != 1) goto corrupt;
    if (kdfread(&sector[0],sizeof(sectortype),MAXSECTORS,fil) != MAXSECTORS) goto corrupt;
    if (kdfread(&sprite[0],sizeof(spritetype),MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&spriteext[0],sizeof(spriteext_t),MAXSPRITES,fil) != MAXSPRITES) goto corrupt;

#ifdef POLYMER
    if (kdfread(&lightcount,sizeof(lightcount),1,fil) != 1) goto corrupt;
    if (kdfread(&prlights[0],sizeof(_prlight),lightcount,fil) != lightcount) goto corrupt;
#else
    if (kdfread(&i,sizeof(int32_t),1,fil) != 1) goto corrupt;
#endif // POLYMER

#if defined(POLYMOST) && defined(USE_OPENGL)
    for (i=0; i<MAXSPRITES; i++)
        if (spriteext[i].mdanimtims)
            spriteext[i].mdanimtims+=mdtims;
#endif
    if (kdfread(&headspritesect[0],sizeof(headspritesect[0]),MAXSECTORS+1,fil) != MAXSECTORS+1) goto corrupt;
    if (kdfread(&prevspritesect[0],sizeof(prevspritesect[0]),MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&nextspritesect[0],sizeof(nextspritesect[0]),MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&headspritestat[STAT_DEFAULT],sizeof(headspritestat[STAT_DEFAULT]),MAXSTATUS+1,fil) != MAXSTATUS+1) goto corrupt;
    if (kdfread(&prevspritestat[STAT_DEFAULT],sizeof(prevspritestat[STAT_DEFAULT]),MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&nextspritestat[STAT_DEFAULT],sizeof(nextspritestat[STAT_DEFAULT]),MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&g_numCyclers,sizeof(g_numCyclers),1,fil) != 1) goto corrupt;
    if (kdfread(&cyclers[0][0],sizeof(cyclers[0][0])*6,MAXCYCLERS,fil) != MAXCYCLERS) goto corrupt;
    for (i=0; i<nump; i++)
        if (kdfread(g_player[i].ps,sizeof(DukePlayer_t),1,fil) != 1) goto corrupt;
    if (kdfread(&g_playerSpawnPoints,sizeof(g_playerSpawnPoints),1,fil) != 1) goto corrupt;
    if (kdfread(&g_numAnimWalls,sizeof(g_numAnimWalls),1,fil) != 1) goto corrupt;
    if (kdfread(&animwall,sizeof(animwall),1,fil) != 1) goto corrupt;
    if (kdfread(&msx[0],sizeof(int32_t),sizeof(msx)/sizeof(int32_t),fil) != sizeof(msx)/sizeof(int32_t)) goto corrupt;
    if (kdfread(&msy[0],sizeof(int32_t),sizeof(msy)/sizeof(int32_t),fil) != sizeof(msy)/sizeof(int32_t)) goto corrupt;
    if (kdfread((int16_t *)&g_spriteDeleteQueuePos,sizeof(int16_t),1,fil) != 1) goto corrupt;
    if (kdfread((int16_t *)&g_spriteDeleteQueueSize,sizeof(int16_t),1,fil) != 1) goto corrupt;
    if (kdfread((int16_t *)&SpriteDeletionQueue[0],sizeof(int16_t),g_spriteDeleteQueueSize,fil) != g_spriteDeleteQueueSize) goto corrupt;
    if (kdfread(&g_mirrorCount,sizeof(int16_t),1,fil) != 1) goto corrupt;
    if (kdfread(&g_mirrorWall[0],sizeof(int16_t),64,fil) != 64) goto corrupt;
    if (kdfread(&g_mirrorSector[0],sizeof(int16_t),64,fil) != 64) goto corrupt;
    if (kdfread(&show2dsector[0],sizeof(uint8_t),MAXSECTORS>>3,fil) != (MAXSECTORS>>3)) goto corrupt;
    if (kdfread(&ActorType[0],sizeof(uint8_t),MAXTILES,fil) != MAXTILES) goto corrupt;

    if (kdfread(&g_numClouds,sizeof(g_numClouds),1,fil) != 1) goto corrupt;
    if (kdfread(&clouds[0],sizeof(int16_t)<<7,1,fil) != 1) goto corrupt;
    if (kdfread(&cloudx[0],sizeof(int16_t)<<7,1,fil) != 1) goto corrupt;
    if (kdfread(&cloudy[0],sizeof(int16_t)<<7,1,fil) != 1) goto corrupt;

    if (kdfread(&g_scriptSize,sizeof(g_scriptSize),1,fil) != 1) goto corrupt;
    if (!g_scriptSize) goto corrupt;
    scriptptrs = Bcalloc(1,g_scriptSize * sizeof(scriptptrs));
    Bfree(bitptr);
    bitptr = Bcalloc(1,(((g_scriptSize+7)>>3)+1) * sizeof(uint8_t));
    if (kdfread(&bitptr[0],sizeof(uint8_t),(g_scriptSize+7)>>3,fil) != ((g_scriptSize+7)>>3)) goto corrupt;
    if (script != NULL)
        Bfree(script);
    script = Bcalloc(1,g_scriptSize * sizeof(intptr_t));
    if (kdfread(&script[0],sizeof(script),g_scriptSize,fil) != g_scriptSize) goto corrupt;
    for (i=0; i<g_scriptSize; i++)
        if (bitptr[i>>3]&(BITPTR_POINTER<<(i&7)))
        {
            j = (intptr_t)script[i]+(intptr_t)&script[0];
            script[i] = j;
        }

    if (kdfread(&actorscrptr[0],sizeof(actorscrptr[0]),MAXTILES,fil) != MAXTILES) goto corrupt;
    for (i=0; i<MAXTILES; i++)
        if (actorscrptr[i])
        {
            j = (intptr_t)actorscrptr[i]+(intptr_t)&script[0];
            actorscrptr[i] = (intptr_t *)j;
        }
    if (kdfread(&actorLoadEventScrptr[0],sizeof(&actorLoadEventScrptr[0]),MAXTILES,fil) != MAXTILES) goto corrupt;
    for (i=0; i<MAXTILES; i++)
        if (actorLoadEventScrptr[i])
        {
            j = (intptr_t)actorLoadEventScrptr[i]+(intptr_t)&script[0];
            actorLoadEventScrptr[i] = (intptr_t *)j;
        }

    scriptptrs = Brealloc(scriptptrs, MAXSPRITES * sizeof(scriptptrs));

    if (kdfread(&scriptptrs[0],sizeof(scriptptrs),MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&actor[0],sizeof(actor_t),MAXSPRITES,fil) != MAXSPRITES) goto corrupt;

    for (i=0; i<MAXSPRITES; i++)
    {
        j = (intptr_t)(&script[0]);
        if (scriptptrs[i]&1) T2 += j;
        if (scriptptrs[i]&2) T5 += j;
        if (scriptptrs[i]&4) T6 += j;
        actor[i].projectile = &SpriteProjectile[i];
    }

    if (kdfread(&lockclock,sizeof(lockclock),1,fil) != 1) goto corrupt;
    if (kdfread(&pskybits,sizeof(pskybits),1,fil) != 1) goto corrupt;
    if (kdfread(&pskyoff[0],sizeof(pskyoff[0]),MAXPSKYTILES,fil) != MAXPSKYTILES) goto corrupt;

    if (kdfread(&g_animateCount,sizeof(g_animateCount),1,fil) != 1) goto corrupt;
    if (kdfread(&animatesect[0],sizeof(animatesect[0]),MAXANIMATES,fil) != MAXANIMATES) goto corrupt;
    if (kdfread(&animateptr[0],sizeof(animateptr[0]),MAXANIMATES,fil) != MAXANIMATES) goto corrupt;
    for (i = g_animateCount-1; i>=0; i--) animateptr[i] = (int32_t *)((intptr_t)animateptr[i]+(intptr_t)(&sector[0]));
    if (kdfread(&animategoal[0],sizeof(animategoal[0]),MAXANIMATES,fil) != MAXANIMATES) goto corrupt;
    if (kdfread(&animatevel[0],sizeof(animatevel[0]),MAXANIMATES,fil) != MAXANIMATES) goto corrupt;

    if (kdfread(&g_earthquakeTime,sizeof(g_earthquakeTime),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.from_bonus,sizeof(ud.from_bonus),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.secretlevel,sizeof(ud.secretlevel),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.respawn_monsters,sizeof(ud.respawn_monsters),1,fil) != 1) goto corrupt;
    ud.m_respawn_monsters = ud.respawn_monsters;
    if (kdfread(&ud.respawn_items,sizeof(ud.respawn_items),1,fil) != 1) goto corrupt;
    ud.m_respawn_items = ud.respawn_items;
    if (kdfread(&ud.respawn_inventory,sizeof(ud.respawn_inventory),1,fil) != 1) goto corrupt;
    ud.m_respawn_inventory = ud.respawn_inventory;

    if (kdfread(&ud.god,sizeof(ud.god),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.auto_run,sizeof(ud.auto_run),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.crosshair,sizeof(ud.crosshair),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.monsters_off,sizeof(ud.monsters_off),1,fil) != 1) goto corrupt;
    ud.m_monsters_off = ud.monsters_off;
    if (kdfread(&ud.last_level,sizeof(ud.last_level),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.eog,sizeof(ud.eog),1,fil) != 1) goto corrupt;

    if (kdfread(&ud.coop,sizeof(ud.coop),1,fil) != 1) goto corrupt;
    ud.m_coop = ud.coop;
    if (kdfread(&ud.marker,sizeof(ud.marker),1,fil) != 1) goto corrupt;
    ud.m_marker = ud.marker;
    if (kdfread(&ud.ffire,sizeof(ud.ffire),1,fil) != 1) goto corrupt;
    ud.m_ffire = ud.ffire;

    if (kdfread(&camsprite,sizeof(camsprite),1,fil) != 1) goto corrupt;
    if (kdfread(&connecthead,sizeof(connecthead),1,fil) != 1) goto corrupt;
    if (kdfread(connectpoint2,sizeof(connectpoint2),1,fil) != 1) goto corrupt;
    if (kdfread(&g_numPlayerSprites,sizeof(g_numPlayerSprites),1,fil) != 1) goto corrupt;
    for (i=0; i<MAXPLAYERS; i++)
        if (kdfread((int16_t *)&g_player[i].frags[0],sizeof(g_player[i].frags),1,fil) != 1) goto corrupt;

    if (kdfread(&randomseed,sizeof(randomseed),1,fil) != 1) goto corrupt;
    if (kdfread(&g_globalRandom,sizeof(g_globalRandom),1,fil) != 1) goto corrupt;
    if (kdfread(&parallaxyscale,sizeof(parallaxyscale),1,fil) != 1) goto corrupt;

    if (kdfread(&SpriteProjectile[0],sizeof(projectile_t),MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&ProjectileData[0],sizeof(projectile_t),MAXTILES,fil) != MAXTILES) goto corrupt;
    if (kdfread(&DefaultProjectileData[0],sizeof(projectile_t),MAXTILES,fil) != MAXTILES) goto corrupt;

    if (kdfread(&SpriteFlags[0],sizeof(SpriteFlags[0]),MAXTILES,fil) != MAXTILES) goto corrupt;

    if (kdfread(&SpriteCacheList[0],sizeof(SpriteCacheList[0]),MAXTILES,fil) != MAXTILES) goto corrupt;

    if (kdfread(&i,sizeof(int32_t),1,fil) != 1) goto corrupt;

    while (i != MAXQUOTES)
    {
        if (ScriptQuotes[i] != NULL)
            Bfree(ScriptQuotes[i]);

        ScriptQuotes[i] = Bcalloc(MAXQUOTELEN,sizeof(uint8_t));

        if (kdfread((char *)ScriptQuotes[i],MAXQUOTELEN,1,fil) != 1) goto corrupt;
        if (kdfread(&i,sizeof(int32_t),1,fil) != 1) goto corrupt;
    }

    if (kdfread(&g_numQuoteRedefinitions,sizeof(g_numQuoteRedefinitions),1,fil) != 1) goto corrupt;

    for (i=0; i<g_numQuoteRedefinitions; i++)
    {
        if (ScriptQuoteRedefinitions[i] != NULL)
            Bfree(ScriptQuoteRedefinitions[i]);

        ScriptQuoteRedefinitions[i] = Bcalloc(MAXQUOTELEN,sizeof(uint8_t));

        if (kdfread((char *)ScriptQuoteRedefinitions[i],MAXQUOTELEN,1,fil) != 1) goto corrupt;
    }

    if (kdfread(&DynamicTileMap[0],sizeof(DynamicTileMap[0]),MAXTILES,fil) != MAXTILES) goto corrupt;

    if (kdfread(&ud.noexits,sizeof(ud.noexits),1,fil) != 1) goto corrupt;
    ud.m_noexits = ud.noexits;


    if (Gv_ReadSave(fil, 0)) goto corrupt;

    kclose(fil);

    if (g_player[myconnectindex].ps->over_shoulder_on != 0)
    {
        g_cameraDistance = 0;
        g_cameraClock = 0;
        g_player[myconnectindex].ps->over_shoulder_on = 1;
    }

    screenpeek = myconnectindex;

    clearbufbyte(gotpic,sizeof(gotpic),0L);
    S_ClearSoundLocks();
    G_CacheMapData();

    i = g_musicIndex;
    g_musicIndex = (ud.volume_number*MAXLEVELS) + ud.level_number;

    if (boardfilename[0] != 0 && ud.level_number == 7 && ud.volume_number == 0)
    {
        char *p;
        char levname[BMAX_PATH];
        int32_t fil;

        strcpy(levname, boardfilename);
        // usermap music based on map filename
        Bcorrectfilename(levname,0);
        p = Bstrrchr(levname,'.');
        if (!p) strcat(levname,".ogg");
        else
        {
            p[1]='o';
            p[2]='g';
            p[3]='g';
            p[4]=0;
        }

        fil = kopen4loadfrommod(levname,0);

        if (fil > -1)
        {
            kclose(fil);
            if (MapInfo[ud.level_number].alt_musicfn == NULL)
                MapInfo[ud.level_number].alt_musicfn = Bcalloc(Bstrlen(levname)+1,sizeof(uint8_t));
            else if ((Bstrlen(levname)+1) > sizeof(MapInfo[ud.level_number].alt_musicfn))
                MapInfo[ud.level_number].alt_musicfn = Brealloc(MapInfo[ud.level_number].alt_musicfn,(Bstrlen(levname)+1));
            Bstrcpy(MapInfo[ud.level_number].alt_musicfn,levname);
        }
        else if (MapInfo[ud.level_number].alt_musicfn != NULL)
        {
            Bfree(MapInfo[ud.level_number].alt_musicfn);
            MapInfo[ud.level_number].alt_musicfn = NULL;
        }

        p[1]='m';
        p[2]='i';
        p[3]='d';
        p[4]=0;

        fil = kopen4loadfrommod(levname,0);

        if (fil == -1)
            Bsprintf(levname,"dethtoll.mid");
        else kclose(fil);

        if (MapInfo[ud.level_number].musicfn == NULL)
            MapInfo[ud.level_number].musicfn = Bcalloc(Bstrlen(levname)+1,sizeof(uint8_t));
        else if ((Bstrlen(levname)+1) > sizeof(MapInfo[ud.level_number].musicfn))
            MapInfo[ud.level_number].musicfn = Brealloc(MapInfo[ud.level_number].musicfn,(Bstrlen(levname)+1));
        Bstrcpy(MapInfo[ud.level_number].musicfn,levname);
    }

    if (MapInfo[(uint8_t)g_musicIndex].musicfn != NULL && (i != g_musicIndex || MapInfo[MAXVOLUMES*MAXLEVELS+2].alt_musicfn))
    {
        S_StopMusic();
        S_PlayMusic(&MapInfo[(uint8_t)g_musicIndex].musicfn[0],g_musicIndex);
    }
    S_PauseMusic(0);

    g_player[myconnectindex].ps->gm = MODE_GAME;
    ud.recstat = 0;

    if (g_player[myconnectindex].ps->jetpack_on)
        A_PlaySound(DUKE_JETPACK_IDLE,g_player[myconnectindex].ps->i);

    g_restorePalette = 1;
    P_UpdateScreenPal(g_player[myconnectindex].ps);
    G_UpdateScreenArea();

    FX_SetReverb(0);

    if (ud.lockout == 0)
    {
        for (x=0; x<g_numAnimWalls; x++)
            if (wall[animwall[x].wallnum].extra >= 0)
                wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
    }
    else
    {
        for (x=0; x<g_numAnimWalls; x++)
            switch (DynamicTileMap[wall[animwall[x].wallnum].picnum])
            {
            case FEMPIC1__STATIC:
                wall[animwall[x].wallnum].picnum = BLANKSCREEN;
                break;
            case FEMPIC2__STATIC:
            case FEMPIC3__STATIC:
                wall[animwall[x].wallnum].picnum = SCREENBREAK6;
                break;
            }
    }

    g_numInterpolations = 0;
    startofdynamicinterpolations = 0;

    k = headspritestat[STAT_EFFECTOR];
    while (k >= 0)
    {
        switch (sprite[k].lotag)
        {
        case 31:
            G_SetInterpolation(&sector[sprite[k].sectnum].floorz);
            break;
        case 32:
            G_SetInterpolation(&sector[sprite[k].sectnum].ceilingz);
            break;
        case 25:
            G_SetInterpolation(&sector[sprite[k].sectnum].floorz);
            G_SetInterpolation(&sector[sprite[k].sectnum].ceilingz);
            break;
        case 17:
            G_SetInterpolation(&sector[sprite[k].sectnum].floorz);
            G_SetInterpolation(&sector[sprite[k].sectnum].ceilingz);
            break;
        case 0:
        case 5:
        case 6:
        case 11:
        case 14:
        case 15:
        case 16:
        case 26:
        case 30:
            Sect_SetInterpolation(k);
            break;
        }

        k = nextspritestat[k];
    }

    for (i=g_numInterpolations-1; i>=0; i--) bakipos[i] = *curipos[i];
    for (i = g_animateCount-1; i>=0; i--)
        G_SetInterpolation(animateptr[i]);

    g_showShareware = 0;
    everyothertime = 0;

//    clearbufbyte(playerquitflag,MAXPLAYERS,0x01010101);

    for (i=0; i<MAXPLAYERS; i++)
        clearbufbyte(&g_player[i].playerquitflag,1,0x01010101);

    Net_ResetPrediction();

    ready2send = 1;

    clearfifo();
    Net_WaitForServer();

    G_ResetTimers();

#ifdef POLYMER
    if (getrendermode() == 4)
    {
        int32_t i = 0;

        polymer_loadboard();

        while (i < MAXSPRITES)
        {
            if (actor[i].lightptr)
            {
                actor[i].lightptr = NULL;
                actor[i].lightId = -1;
            }
            i++;
        }
    }
#endif

    return(0);
corrupt:
    Bsprintf(tempbuf,"Save game file \"%s\" is corrupt or of the wrong version.",fnptr);
    G_GameExit(tempbuf);
    return -1;
}

int32_t G_SavePlayer(int32_t spot)
{
    int32_t i;
    intptr_t j;
    char fn[13];
    char mpfn[13];
    char *fnptr, *scriptptrs;
    FILE *fil;
    int32_t bv = BYTEVERSION;

    strcpy(fn, "egam0.sav");
    strcpy(mpfn, "egamA_00.sav");

    Net_WaitForServer();

    fnptr = fn;
    fn[4] = spot + '0';

    {
        char temp[BMAX_PATH];
        if (g_modDir[0] != '/')
            Bsprintf(temp,"%s/%s",g_modDir,fnptr);
        else Bsprintf(temp,"%s",fnptr);
        if ((fil = fopen(temp,"wb")) == 0) return(-1);
    }

    ready2send = 0;

    Bsprintf(g_szBuf,"EDuke32");
    i=strlen(g_szBuf);
    dfwrite(&i,sizeof(i),1,fil);
    dfwrite(g_szBuf,i,1,fil);

    dfwrite(&bv,sizeof(bv),1,fil);
    dfwrite(&ud.multimode,sizeof(ud.multimode),1,fil);

    dfwrite(&ud.savegame[spot][0],21,1,fil);
    dfwrite(&ud.volume_number,sizeof(ud.volume_number),1,fil);
    dfwrite(&ud.level_number,sizeof(ud.level_number),1,fil);
    dfwrite(&ud.player_skill,sizeof(ud.player_skill),1,fil);
    dfwrite(&currentboardfilename[0],BMAX_PATH,1,fil);
    if (!waloff[TILE_SAVESHOT])
    {
        walock[TILE_SAVESHOT] = 254;
        allocache(&waloff[TILE_SAVESHOT],200*320,&walock[TILE_SAVESHOT]);
        clearbuf((void *)waloff[TILE_SAVESHOT],(200*320)/4,0);
        walock[TILE_SAVESHOT] = 1;
    }
    dfwrite((char *)waloff[TILE_SAVESHOT],320,200,fil);

    dfwrite(&numwalls,sizeof(numwalls),1,fil);
    dfwrite(&wall[0],sizeof(walltype),MAXWALLS,fil);
    dfwrite(&numsectors,sizeof(numsectors),1,fil);
    dfwrite(&sector[0],sizeof(sectortype),MAXSECTORS,fil);
    dfwrite(&sprite[0],sizeof(spritetype),MAXSPRITES,fil);
#if defined(POLYMOST) && defined(USE_OPENGL)
    for (i=0; i<MAXSPRITES; i++)
        if (spriteext[i].mdanimtims)
        {
            spriteext[i].mdanimtims=spriteext[i].mdanimtims-mdtims;
            if (!spriteext[i].mdanimtims)
                spriteext[i].mdanimtims++;
        }
#endif
    dfwrite(&spriteext[0],sizeof(spriteext_t),MAXSPRITES,fil);

#ifdef POLYMER
    dfwrite(&lightcount,sizeof(lightcount),1,fil);
    dfwrite(&prlights[0],sizeof(_prlight),lightcount,fil);
#else
    i = 0;
    dfwrite(&i,sizeof(int32_t),1,fil);
#endif // POLYMER

#if defined(POLYMOST) && defined(USE_OPENGL)
    for (i=0; i<MAXSPRITES; i++)if (spriteext[i].mdanimtims)spriteext[i].mdanimtims+=mdtims;
#endif
    dfwrite(&headspritesect[0],sizeof(headspritesect[0]),MAXSECTORS+1,fil);
    dfwrite(&prevspritesect[0],sizeof(prevspritesect[0]),MAXSPRITES,fil);
    dfwrite(&nextspritesect[0],sizeof(nextspritesect[0]),MAXSPRITES,fil);
    dfwrite(&headspritestat[STAT_DEFAULT],sizeof(headspritestat[0]),MAXSTATUS+1,fil);
    dfwrite(&prevspritestat[STAT_DEFAULT],sizeof(prevspritestat[0]),MAXSPRITES,fil);
    dfwrite(&nextspritestat[STAT_DEFAULT],sizeof(nextspritestat[0]),MAXSPRITES,fil);
    dfwrite(&g_numCyclers,sizeof(g_numCyclers),1,fil);
    dfwrite(&cyclers[0][0],sizeof(cyclers[0][0])*6,MAXCYCLERS,fil);
    for (i=0; i<ud.multimode; i++)
        dfwrite(g_player[i].ps,sizeof(DukePlayer_t),1,fil);
    dfwrite(&g_playerSpawnPoints,sizeof(g_playerSpawnPoints),1,fil);
    dfwrite(&g_numAnimWalls,sizeof(g_numAnimWalls),1,fil);
    dfwrite(&animwall,sizeof(animwall),1,fil);
    dfwrite(&msx[0],sizeof(int32_t),sizeof(msx)/sizeof(int32_t),fil);
    dfwrite(&msy[0],sizeof(int32_t),sizeof(msy)/sizeof(int32_t),fil);
    dfwrite(&g_spriteDeleteQueuePos,sizeof(int16_t),1,fil);
    dfwrite(&g_spriteDeleteQueueSize,sizeof(int16_t),1,fil);
    dfwrite(&SpriteDeletionQueue[0],sizeof(int16_t),g_spriteDeleteQueueSize,fil);
    dfwrite(&g_mirrorCount,sizeof(int16_t),1,fil);
    dfwrite(&g_mirrorWall[0],sizeof(int16_t),64,fil);
    dfwrite(&g_mirrorSector[0],sizeof(int16_t),64,fil);
    dfwrite(&show2dsector[0],sizeof(uint8_t),MAXSECTORS>>3,fil);
    dfwrite(&ActorType[0],sizeof(uint8_t),MAXTILES,fil);

    dfwrite(&g_numClouds,sizeof(g_numClouds),1,fil);
    dfwrite(&clouds[0],sizeof(int16_t)<<7,1,fil);
    dfwrite(&cloudx[0],sizeof(int16_t)<<7,1,fil);
    dfwrite(&cloudy[0],sizeof(int16_t)<<7,1,fil);

    dfwrite(&g_scriptSize,sizeof(g_scriptSize),1,fil);
    scriptptrs = Bcalloc(1, g_scriptSize * sizeof(scriptptrs));
    for (i=0; i<g_scriptSize; i++)
    {
        if (bitptr[i>>3]&(BITPTR_POINTER<<(i&7)))
        {
//            scriptptrs[i] = 1;
            j = (intptr_t)script[i] - (intptr_t)&script[0];
            script[i] = j;
        }
        //      else scriptptrs[i] = 0;
    }

//    dfwrite(&scriptptrs[0],sizeof(scriptptrs),g_scriptSize,fil);
    dfwrite(&bitptr[0],sizeof(bitptr[0]),(g_scriptSize+7)>>3,fil);
    dfwrite(&script[0],sizeof(script[0]),g_scriptSize,fil);

    for (i=0; i<g_scriptSize; i++)
        if (bitptr[i>>3]&(BITPTR_POINTER<<(i&7)))
        {
            j = script[i]+(intptr_t)&script[0];
            script[i] = j;
        }

    for (i=0; i<MAXTILES; i++)
        if (actorscrptr[i])
        {
            j = (intptr_t)actorscrptr[i]-(intptr_t)&script[0];
            actorscrptr[i] = (intptr_t *)j;
        }
    dfwrite(&actorscrptr[0],sizeof(actorscrptr[0]),MAXTILES,fil);
    for (i=0; i<MAXTILES; i++)
        if (actorscrptr[i])
        {
            j = (intptr_t)actorscrptr[i]+(intptr_t)&script[0];
            actorscrptr[i] = (intptr_t *)j;
        }

    for (i=0; i<MAXTILES; i++)
        if (actorLoadEventScrptr[i])
        {
            j = (intptr_t)actorLoadEventScrptr[i]-(intptr_t)&script[0];
            actorLoadEventScrptr[i] = (intptr_t *)j;
        }
    dfwrite(&actorLoadEventScrptr[0],sizeof(actorLoadEventScrptr[0]),MAXTILES,fil);
    for (i=0; i<MAXTILES; i++)
        if (actorLoadEventScrptr[i])
        {
            j = (intptr_t)actorLoadEventScrptr[i]+(intptr_t)&script[0];
            actorLoadEventScrptr[i] = (intptr_t *)j;
        }

    Bfree(scriptptrs);
    scriptptrs = Bcalloc(1, MAXSPRITES * sizeof(scriptptrs));

    for (i=0; i<MAXSPRITES; i++)
    {
        scriptptrs[i] = 0;

        if (actorscrptr[PN] == 0) continue;

        j = (intptr_t)&script[0];

        if (T2 >= j && T2 < (intptr_t)(&script[g_scriptSize]))
        {
            scriptptrs[i] |= 1;
            T2 -= j;
        }
        if (T5 >= j && T5 < (intptr_t)(&script[g_scriptSize]))
        {
            scriptptrs[i] |= 2;
            T5 -= j;
        }
        if (T6 >= j && T6 < (intptr_t)(&script[g_scriptSize]))
        {
            scriptptrs[i] |= 4;
            T6 -= j;
        }
    }

    dfwrite(&scriptptrs[0],sizeof(scriptptrs),MAXSPRITES,fil);
    dfwrite(&actor[0],sizeof(actor_t),MAXSPRITES,fil);

    for (i=0; i<MAXSPRITES; i++)
    {
        if (actorscrptr[PN] == 0) continue;
        j = (intptr_t)&script[0];

        if (scriptptrs[i]&1)
            T2 += j;
        if (scriptptrs[i]&2)
            T5 += j;
        if (scriptptrs[i]&4)
            T6 += j;
    }

    dfwrite(&lockclock,sizeof(lockclock),1,fil);
    dfwrite(&pskybits,sizeof(pskybits),1,fil);
    dfwrite(&pskyoff[0],sizeof(pskyoff[0]),MAXPSKYTILES,fil);
    dfwrite(&g_animateCount,sizeof(g_animateCount),1,fil);
    dfwrite(&animatesect[0],sizeof(animatesect[0]),MAXANIMATES,fil);
    for (i = g_animateCount-1; i>=0; i--) animateptr[i] = (int32_t *)((intptr_t)animateptr[i]-(intptr_t)(&sector[0]));
    dfwrite(&animateptr[0],sizeof(animateptr[0]),MAXANIMATES,fil);
    for (i = g_animateCount-1; i>=0; i--) animateptr[i] = (int32_t *)((intptr_t)animateptr[i]+(intptr_t)(&sector[0]));
    dfwrite(&animategoal[0],sizeof(animategoal[0]),MAXANIMATES,fil);
    dfwrite(&animatevel[0],sizeof(animatevel[0]),MAXANIMATES,fil);

    dfwrite(&g_earthquakeTime,sizeof(g_earthquakeTime),1,fil);
    dfwrite(&ud.from_bonus,sizeof(ud.from_bonus),1,fil);
    dfwrite(&ud.secretlevel,sizeof(ud.secretlevel),1,fil);
    dfwrite(&ud.respawn_monsters,sizeof(ud.respawn_monsters),1,fil);
    dfwrite(&ud.respawn_items,sizeof(ud.respawn_items),1,fil);
    dfwrite(&ud.respawn_inventory,sizeof(ud.respawn_inventory),1,fil);
    dfwrite(&ud.god,sizeof(ud.god),1,fil);
    dfwrite(&ud.auto_run,sizeof(ud.auto_run),1,fil);
    dfwrite(&ud.crosshair,sizeof(ud.crosshair),1,fil);
    dfwrite(&ud.monsters_off,sizeof(ud.monsters_off),1,fil);
    dfwrite(&ud.last_level,sizeof(ud.last_level),1,fil);
    dfwrite(&ud.eog,sizeof(ud.eog),1,fil);
    dfwrite(&ud.coop,sizeof(ud.coop),1,fil);
    dfwrite(&ud.marker,sizeof(ud.marker),1,fil);
    dfwrite(&ud.ffire,sizeof(ud.ffire),1,fil);
    dfwrite(&camsprite,sizeof(camsprite),1,fil);
    dfwrite(&connecthead,sizeof(connecthead),1,fil);
    dfwrite(connectpoint2,sizeof(connectpoint2),1,fil);
    dfwrite(&g_numPlayerSprites,sizeof(g_numPlayerSprites),1,fil);
    for (i=0; i<MAXPLAYERS; i++)
        dfwrite((int16_t *)&g_player[i].frags[0],sizeof(g_player[i].frags),1,fil);

    dfwrite(&randomseed,sizeof(randomseed),1,fil);
    dfwrite(&g_globalRandom,sizeof(g_globalRandom),1,fil);
    dfwrite(&parallaxyscale,sizeof(parallaxyscale),1,fil);

    dfwrite(&SpriteProjectile[0],sizeof(projectile_t),MAXSPRITES,fil);
    dfwrite(&ProjectileData[0],sizeof(projectile_t),MAXTILES,fil);
    dfwrite(&DefaultProjectileData[0],sizeof(projectile_t),MAXTILES,fil);

    dfwrite(&SpriteFlags[0],sizeof(SpriteFlags[0]),MAXTILES,fil);

    dfwrite(&SpriteCacheList[0],sizeof(SpriteCacheList[0]),MAXTILES,fil);

    for (i=0; i<MAXQUOTES; i++)
    {
        if (ScriptQuotes[i] != NULL)
        {
            dfwrite(&i,sizeof(int32_t),1,fil);
            dfwrite(ScriptQuotes[i],MAXQUOTELEN, 1, fil);
        }
    }
    dfwrite(&i,sizeof(int32_t),1,fil);

    dfwrite(&g_numQuoteRedefinitions,sizeof(g_numQuoteRedefinitions),1,fil);
    for (i=0; i<g_numQuoteRedefinitions; i++)
    {
        if (ScriptQuoteRedefinitions[i] != NULL)
            dfwrite(ScriptQuoteRedefinitions[i],MAXQUOTELEN, 1, fil);
    }

    dfwrite(&DynamicTileMap[0],sizeof(DynamicTileMap[0]),MAXTILES,fil);

    dfwrite(&ud.noexits,sizeof(ud.noexits),1,fil);

    Gv_WriteSave(fil, 0);

    fclose(fil);

    if ((!g_netServer && ud.multimode < 2))
    {
        strcpy(ScriptQuotes[122],"GAME SAVED");
        P_DoQuote(122,g_player[myconnectindex].ps);
    }

    ready2send = 1;

    Net_WaitForServer();

    ototalclock = totalclock;

    return(0);
}


////////// GENERIC SAVING/LOADING SYSTEM //////////

typedef struct dataspec_
{
    uint32_t flags;
    void *ptr;
    uint32_t size;
    intptr_t cnt;
} dataspec_t;

#define SV_MAJOR_VER 0
#define SV_MINOR_VER 2
#define SV_DEFAULTCOMPRTHRES 8
static uint8_t savegame_diffcompress;  // 0:none, 1:Ken's LZW in cache1d.c
static uint8_t savegame_comprthres;


#define DS_DYNAMIC 1  // dereference .ptr one more time
#define DS_STRING 2
#define DS_CMP 4
// 8
#define DS_CNT(x) ((sizeof(x))<<3)  // .cnt is pointer to...
#define DS_CNT16 16
#define DS_CNT32 32
#define DS_CNTMASK (8|DS_CNT16|DS_CNT32|64)
// 64
#define DS_LOADFN 128  // .ptr is function that is run when loading
#define DS_SAVEFN 256  // .ptr is function that is run when saving
#define DS_NOCHK 1024  // don't check for diffs (and don't write out in dump) since assumend constant throughout demo
#define DS_END (0x70000000)

static int32_t ds_getcnt(const dataspec_t *sp)
{
    switch (sp->flags&DS_CNTMASK)
    {
    case 0: return sp->cnt;
    case DS_CNT16: return *((int16_t *)sp->cnt);
    case DS_CNT32: return *((int32_t *)sp->cnt);
    default: return -1;
    }
}

static void ds_get(const dataspec_t *sp, const void **ptr, int32_t *cnt)
{
    *cnt = ds_getcnt(sp);

    if (sp->flags&DS_DYNAMIC)
        *ptr = *((void **)sp->ptr);
    else
        *ptr = sp->ptr;
}

// write state to file and/or to dump
static uint8_t *writespecdata(const dataspec_t *spec, FILE *fil, uint8_t *dump)
{
    int32_t cnt;
    const void *ptr;
    const dataspec_t *sp=spec;

    for (; sp->flags!=DS_END; sp++)
    {
        if (sp->flags&(DS_SAVEFN|DS_LOADFN))
        {
            if (sp->flags&DS_SAVEFN)
                (*(void ( *)(void))sp->ptr)();
            continue;
        }

        if (!fil && (sp->flags&(DS_NOCHK|DS_CMP|DS_STRING)))
            continue;

        if (sp->flags&DS_STRING)
        {
            fwrite(sp->ptr, Bstrlen(sp->ptr), 1, fil);  // not null-terminated!
            continue;
        }

        ds_get(sp, &ptr, &cnt);
        if (cnt < 0) { OSD_Printf("wsd: cnt=%d, f=0x%x.\n",cnt,sp->flags); continue; }

        if (fil)
        {
            if (((sp->flags&DS_CNTMASK)==0 && sp->size *cnt<=(int32_t)savegame_comprthres)
                    || (sp->flags&DS_CMP))
                fwrite(ptr, sp->size, cnt, fil);
            else
                dfwrite((void *)ptr, sp->size, cnt, fil);
        }

        if (dump && (sp->flags&(DS_NOCHK|DS_CMP))==0)
        {
            Bmemcpy(dump, ptr, sp->size*cnt);
            dump += sp->size*cnt;
        }
    }
    return dump;
}

// let havedump=dumpvar&&*dumpvar
// (fil>=0 && havedump): first restore dump from file, then restore state from dump
// (fil<0 && havedump): only restore state from dump
// (fil>=0 && !havedump): only restore state from file
static int32_t readspecdata(const dataspec_t *spec, int32_t fil, uint8_t **dumpvar)
{
    int32_t cnt, i, j;
    void *ptr;
    uint8_t *dump=dumpvar?*dumpvar:NULL, *mem;
    const dataspec_t *sp=spec;
    static char cmpstrbuf[32];

    for (; sp->flags!=DS_END; sp++)
    {
        if (fil < 0 && sp->flags&(DS_NOCHK|DS_STRING|DS_CMP))  // we're updating
            continue;

        if (sp->flags&(DS_LOADFN|DS_SAVEFN))
        {
            if (sp->flags&DS_LOADFN)
                (*(void ( *)())sp->ptr)();
            continue;
        }

        if (sp->flags&(DS_STRING|DS_CMP))  // DS_STRING and DS_CMP is for static data only
        {
            if (sp->flags&(DS_STRING))
                i = Bstrlen(sp->ptr);
            else
                i = sp->size*sp->cnt;

            j=kread(fil, cmpstrbuf, i);
            if (j!=i || Bmemcmp(sp->ptr, cmpstrbuf, i))
            {
                OSD_Printf("rds: spec=%p, sp=%p ", spec, sp);
                if (j!=i)
                    OSD_Printf("kread returned %d, expected %d.\n", j, i);
                else
                    OSD_Printf("sp->ptr and cmpstrbuf not identical!\n");
//                *(int32_t *)0 = 1;
                return -1;
            }
            continue;
        }

        ds_get(sp, (const void **)&ptr, &cnt);
        if (cnt < 0) { OSD_Printf("rsd: cnt<0... wtf?\n"); return -1; }

        if (fil>=0)
        {
            mem = (dump && (sp->flags&DS_NOCHK)==0) ? dump : ptr;

            if ((sp->flags&DS_CNTMASK)==0 && sp->size *cnt<=(int32_t)savegame_comprthres)
            {
                i = kread(fil, mem, cnt*sp->size);
                j = cnt*sp->size;
            }
            else
            {
                i = kdfread(mem, sp->size, cnt, fil);
                j = cnt;
            }
            if (i!=j)
            {
                OSD_Printf("rsd: spec=%p, sp=%p, mem=%p ", spec, sp, mem);
                OSD_Printf("rsd: %s: read %d, expected %d!\n",
                           ((sp->flags&DS_CNTMASK)==0 && sp->size *cnt<=(int32_t)savegame_comprthres)?
                           "UNCOMP":"COMPR", i, j);

                if (i==-1)
                    perror("read");
//            *(int32_t *)0 = 1;
                return -1;
            }
        }

        if (dump && (sp->flags&DS_NOCHK)==0)
        {
            Bmemcpy(ptr, dump, sp->size*cnt);
            dump += sp->size*cnt;
        }
    }

    if (dumpvar)
        *dumpvar = dump;
    return 0;
}

#define UINT(bits) uint##bits##_t
#define BYTES(bits) (bits>>3)
#define VAL(bits,p) (*(UINT(bits) *)(p))

static void docmpsd(const void *ptr, void *dump, uint32_t size, uint32_t cnt, uint8_t **diffvar)
{
    uint8_t *retdiff = *diffvar;

    // Hail to the C preprocessor, baby!
#define CPSINGLEVAL(Datbits) \
        if (VAL(Datbits, ptr) != VAL(Datbits, dump))  \
        {                                             \
            VAL(Datbits, retdiff) = VAL(Datbits, dump) = VAL(Datbits, ptr); \
            *diffvar = retdiff+BYTES(Datbits);        \
        }

    if (cnt==1)
        switch (size)
        {
        case 8: CPSINGLEVAL(64); return;
        case 4: CPSINGLEVAL(32); return;
        case 2: CPSINGLEVAL(16); return;
        case 1: CPSINGLEVAL(8); return;
        }

#define CPELTS(Idxbits, Datbits) do \
    {                                         \
        for (i=0; i<nelts; i++)               \
        {                                     \
            if (*p!=*op)                      \
            {                                 \
                *op = *p;                     \
                VAL(Idxbits, retdiff) = i;    \
                retdiff += BYTES(Idxbits);    \
                VAL(Datbits, retdiff) = *p;   \
                retdiff += BYTES(Datbits);    \
            }                                 \
            p++;                              \
            op++;                             \
        }                                     \
        VAL(Idxbits, retdiff) = -1;           \
        retdiff += BYTES(Idxbits);            \
    } while (0)

#define CPDATA(Datbits) do \
    { \
        const UINT(Datbits) *p=ptr;  \
        UINT(Datbits) *op=dump;      \
        uint32_t i, nelts=(size*cnt)/BYTES(Datbits); \
        if (nelts>65536)          \
            CPELTS(32,Datbits);   \
        else if (nelts>256)       \
            CPELTS(16,Datbits);   \
        else                      \
            CPELTS(8,Datbits);    \
    } while (0)

    if (size==8)
        CPDATA(64);
    else if ((size&3)==0)
        CPDATA(32);
    else if ((size&1)==0)
        CPDATA(16);
    else
        CPDATA(8);

    *diffvar = retdiff;
    return;

#undef CPELTS
#undef CPSINGLEVAL
#undef CPDATA
}

// get the number of elements to be monitored for changes
static int32_t getnumvar(const dataspec_t *spec)
{
    int32_t n=0;
    for (; spec->flags!=DS_END; spec++)
        n += (spec->flags&(DS_STRING|DS_CMP|DS_NOCHK|DS_SAVEFN|DS_LOADFN) ? 0 : 1);
    return n;
}

// update dump at *dumpvar with new state and write diff to *diffvar
static void cmpspecdata(const dataspec_t *spec, uint8_t **dumpvar, uint8_t **diffvar)
{
    const void *ptr;
    uint8_t *dump=*dumpvar, *diff=*diffvar, *tmptr;
    const dataspec_t *sp=spec;
    int32_t cnt, eltnum=0, nbytes=(getnumvar(spec)+7)>>3, l=Bstrlen(spec->ptr);

    Bmemcpy(diff, spec->ptr, l);
    diff+=l;

    while (nbytes--)
        *(diff++) = 0;  // the bitmap of indices which elements of spec have changed go here

    for (sp++; sp->flags!=DS_END; sp++)
    {
        if ((sp->flags&(DS_NOCHK|DS_STRING|DS_CMP)))
            continue;

        if (sp->flags&(DS_LOADFN|DS_SAVEFN))
        {
            (*(void ( *)())sp->ptr)();
            continue;
        }

        ds_get(sp, &ptr, &cnt);
        if (cnt < 0) { OSD_Printf("csd: cnt=%d, f=0x%x\n", cnt, sp->flags); continue; }

        tmptr = diff;
        docmpsd(ptr, dump, sp->size, cnt, &diff);
        if (diff != tmptr)
            (*diffvar + l)[eltnum>>3] |= 1<<(eltnum&7);
        dump += sp->size*cnt;
        eltnum++;
    }

    *diffvar = diff;
    *dumpvar = dump;
    return;
}

#define VALOFS(bits,p,ofs) (*(((UINT(bits) *)(p)) + (ofs)))

// apply diff to dump, not to state! state is restored from dump afterwards.
static int32_t applydiff(const dataspec_t *spec, uint8_t **dumpvar, uint8_t **diffvar)
{
    uint8_t *dumptr=*dumpvar, *diffptr=*diffvar;
    const dataspec_t *sp=spec;
    int32_t cnt, eltnum=-1, nbytes=(getnumvar(spec)+7)>>3, l=Bstrlen(spec->ptr);

    if (Bmemcmp(diffptr, spec->ptr, l))  // check STRING magic (sync check)
    {
//        *(int32_t *)0 = 1;
        return 1;
    }
    diffptr += l+nbytes;

    for (sp++; sp->flags!=DS_END; sp++)
    {
        if ((sp->flags&(DS_NOCHK|DS_STRING|DS_CMP|DS_LOADFN|DS_SAVEFN)))
            continue;

        cnt = ds_getcnt(sp);
        if (cnt < 0) return 1;

        eltnum++;
        if (((*diffvar + l)[eltnum>>3]&(1<<(eltnum&7))) == 0)
        {
            dumptr += sp->size*cnt;
            continue;
        }

// ----------
#define CPSINGLEVAL(Datbits) \
            VAL(Datbits, dumptr) = VAL(Datbits, diffptr); \
            diffptr += BYTES(Datbits); \
            dumptr += BYTES(Datbits)

        if (cnt==1)
        {
            switch (sp->size)
            {
            case 8: CPSINGLEVAL(64); continue;
            case 4: CPSINGLEVAL(32); continue;
            case 2: CPSINGLEVAL(16); continue;
            case 1: CPSINGLEVAL(8); continue;
            }
        }

#define CPELTS(Idxbits, Datbits) do \
        {                                    \
            UINT(Idxbits) idx;               \
            goto readidx_##Idxbits##_##Datbits; \
            do                               \
            {                                \
                VALOFS(Datbits, dumptr, idx) = VAL(Datbits, diffptr); \
                diffptr += BYTES(Datbits);   \
readidx_##Idxbits##_##Datbits:               \
                idx = VAL(Idxbits, diffptr); \
                diffptr += BYTES(Idxbits);   \
            } while ((int##Idxbits##_t)idx != -1);  \
        } while (0)

#define CPDATA(Datbits) do \
        {                             \
            uint32_t nelts=(sp->size*cnt)/BYTES(Datbits); \
            if (nelts>65536)          \
                CPELTS(32,Datbits);   \
            else if (nelts>256)       \
                CPELTS(16,Datbits);   \
            else                      \
                CPELTS(8,Datbits);    \
        } while (0)

        if (sp->size==8)
            CPDATA(64);
        else if ((sp->size&3)==0)
            CPDATA(32);
        else if ((sp->size&1)==0)
            CPDATA(16);
        else
            CPDATA(8);
        dumptr += sp->size*cnt;
// ----------

#undef CPELTS
#undef CPSINGLEVAL
#undef CPDATA
    }

    *diffvar = diffptr;
    *dumpvar = dumptr;
    return 0;
}

#undef VAL
#undef VALOFS
#undef BYTES
#undef UINT

// calculate size needed for dump
static uint32_t calcsz(const dataspec_t *spec)
{
    const dataspec_t *sp=spec;
    int32_t cnt;
    uint32_t dasiz=0;

    for (; sp->flags!=DS_END; sp++)
    {
        // DS_STRINGs are used as sync checks in the diffs but not in the dump
        if ((sp->flags&(DS_CMP|DS_NOCHK|DS_SAVEFN|DS_LOADFN|DS_STRING)))
            continue;

        cnt = ds_getcnt(sp);
        if (cnt<=0) continue;

        dasiz += cnt*sp->size;
    }

    return dasiz;
}

static void sv_postudload();
static void sv_prespriteextsave();
static void sv_postspriteext();
static void sv_calcbitptrsize();
static void sv_prescriptsave_once();
static void sv_prescriptload_once();
static void sv_postscript_once();
static void sv_preactordatasave();
static void sv_postactordata();
static void sv_preanimateptrsave();
static void sv_postanimateptr();
static void sv_prequote();
static void sv_quotesave();
static void sv_quoteload();
static void sv_prequoteredef();
static void sv_quoteredefsave();
static void sv_quoteredefload();
static void sv_postquoteredef();
static void sv_restsave();
static void sv_restload();

#define SVARDATALEN \
    ((sizeof(g_player[0].user_name)+sizeof(g_player[0].pcolor)+sizeof(g_player[0].pteam) \
      +sizeof(g_player[0].frags)+sizeof(DukePlayer_t))*MAXPLAYERS + sizeof(_prlight)*PR_MAXLIGHTS + sizeof(lightcount))

static uint8_t savegame_bitmap[MAXSPRITES>>3][3];
static uint32_t savegame_bitptrsize;
static uint8_t savegame_quotedef[MAXQUOTES>>3];
static char(*savegame_quotes)[MAXQUOTELEN];
static char(*savegame_quoteredefs)[MAXQUOTELEN];
static uint8_t savegame_restdata[SVARDATALEN];

static const dataspec_t svgm_udnetw[] =
{
    { DS_STRING, "blK:udnt", 0, 1 },
    { 0, &ud.multimode, sizeof(ud.multimode), 1 },
    { 0, &g_numPlayerSprites, sizeof(g_numPlayerSprites), 1 },
    { 0, &g_playerSpawnPoints, sizeof(g_playerSpawnPoints), 1 },

    { DS_NOCHK, &ud.volume_number, sizeof(ud.volume_number), 1 },
    { DS_NOCHK, &ud.level_number, sizeof(ud.level_number), 1 },
    { DS_NOCHK, &ud.player_skill, sizeof(ud.player_skill), 1 },

    { DS_NOCHK, &ud.from_bonus, sizeof(ud.from_bonus), 1 },
    { DS_NOCHK, &ud.secretlevel, sizeof(ud.secretlevel), 1 },
    { DS_NOCHK, &ud.respawn_monsters, sizeof(ud.respawn_monsters), 1 },
    { DS_NOCHK, &ud.respawn_items, sizeof(ud.respawn_items), 1 },
    { DS_NOCHK, &ud.respawn_inventory, sizeof(ud.respawn_inventory), 1 },
    { 0, &ud.god, sizeof(ud.god), 1 },
    { 0, &ud.auto_run, sizeof(ud.auto_run), 1 },
//    { DS_NOCHK, &ud.crosshair, sizeof(ud.crosshair), 1 },
    { DS_NOCHK, &ud.monsters_off, sizeof(ud.monsters_off), 1 },
    { DS_NOCHK, &ud.last_level, sizeof(ud.last_level), 1 },
    { 0, &ud.eog, sizeof(ud.eog), 1 },
    { DS_NOCHK, &ud.coop, sizeof(ud.coop), 1 },
    { DS_NOCHK, &ud.marker, sizeof(ud.marker), 1 },
    { DS_NOCHK, &ud.ffire, sizeof(ud.ffire), 1 },
    { DS_NOCHK, &ud.noexits, sizeof(ud.noexits), 1 },
    { DS_NOCHK, &ud.playerai, sizeof(ud.playerai), 1 },
    { 0, &ud.pause_on, sizeof(ud.pause_on), 1 },
    { DS_NOCHK, &currentboardfilename[0], BMAX_PATH, 1 },
    { DS_LOADFN, (void *)&sv_postudload, 0, 1 },
    { 0, &connecthead, sizeof(connecthead), 1 },
    { 0, connectpoint2, sizeof(connectpoint2), 1 },
    { 0, &randomseed, sizeof(randomseed), 1 },
    { 0, &g_globalRandom, sizeof(g_globalRandom), 1 },
//    { 0, &lockclock_dummy, sizeof(lockclock), 1 },
    { DS_END, 0, 0, 0 }
};

static const dataspec_t svgm_secwsp[] =
{
    { DS_STRING, "blK:swsp", 0, 1 },
    { DS_NOCHK, &numwalls, sizeof(numwalls), 1 },
    { DS_DYNAMIC|DS_CNT(numwalls), &wall, sizeof(walltype), (intptr_t)&numwalls },
    { DS_NOCHK, &numsectors, sizeof(numsectors), 1 },
    { DS_DYNAMIC|DS_CNT(numsectors), &sector, sizeof(sectortype), (intptr_t)&numsectors },
    { DS_DYNAMIC, &sprite, sizeof(spritetype), MAXSPRITES },
    { 0, &headspritesect[0], sizeof(headspritesect[0]), MAXSECTORS+1 },
    { 0, &prevspritesect[0], sizeof(prevspritesect[0]), MAXSPRITES },
    { 0, &nextspritesect[0], sizeof(nextspritesect[0]), MAXSPRITES },
    { 0, &headspritestat[0], sizeof(headspritestat[0]), MAXSTATUS+1 },
    { 0, &prevspritestat[0], sizeof(prevspritestat[0]), MAXSPRITES },
    { 0, &nextspritestat[0], sizeof(nextspritestat[0]), MAXSPRITES },
#if defined(POLYMOST) && defined(USE_OPENGL)
    { DS_SAVEFN, (void *)&sv_prespriteextsave, 0, 1 },
#endif
    { DS_DYNAMIC, &spriteext, sizeof(spriteext_t), MAXSPRITES },
#if defined(POLYMOST) && defined(USE_OPENGL)
    { DS_SAVEFN|DS_LOADFN, (void *)&sv_postspriteext, 0, 1 },
#endif
    { DS_NOCHK, &SpriteFlags[0], sizeof(SpriteFlags[0]), MAXTILES },
    { DS_NOCHK, &SpriteCacheList[0], sizeof(SpriteCacheList[0]), MAXTILES },
    { 0, &DynamicTileMap[0], sizeof(DynamicTileMap[0]), MAXTILES },  // NOCHK?
    { DS_NOCHK, &ActorType[0], sizeof(uint8_t), MAXTILES },
    { DS_NOCHK, &g_numCyclers, sizeof(g_numCyclers), 1 },
    { DS_CNT(g_numCyclers), &cyclers[0][0], sizeof(cyclers[0]), (intptr_t)&g_numCyclers },
    { DS_NOCHK, &g_numAnimWalls, sizeof(g_numAnimWalls), 1 },
    { DS_CNT(g_numAnimWalls), &animwall, sizeof(animwall[0]), (intptr_t)&g_numAnimWalls },
    { DS_NOCHK, &g_mirrorCount, sizeof(g_mirrorCount), 1 },
    { DS_NOCHK, &g_mirrorWall[0], sizeof(g_mirrorWall[0]), sizeof(g_mirrorWall)/sizeof(g_mirrorWall[0]) },
    { DS_NOCHK, &g_mirrorSector[0], sizeof(g_mirrorSector[0]), sizeof(g_mirrorSector)/sizeof(g_mirrorSector[0]) },
// projectiles
    { 0, &SpriteProjectile[0], sizeof(projectile_t), MAXSPRITES },
    { 0, &ProjectileData[0], sizeof(projectile_t), MAXTILES },
    { DS_NOCHK, &DefaultProjectileData[0], sizeof(projectile_t), MAXTILES },
    { 0, &everyothertime, sizeof(everyothertime), 1 },
    { DS_END, 0, 0, 0 }
};

static const dataspec_t svgm_script[] =
{
    { DS_STRING, "blK:scri", 0, 1 },
    { DS_NOCHK, &g_scriptSize, sizeof(g_scriptSize), 1 },
    { DS_SAVEFN|DS_LOADFN|DS_NOCHK, (void *)&sv_calcbitptrsize, 0, 1 },
    { DS_DYNAMIC|DS_CNT(savegame_bitptrsize)|DS_NOCHK, &bitptr, sizeof(bitptr[0]), (intptr_t)&savegame_bitptrsize },

    { DS_SAVEFN|DS_NOCHK, (void *)&sv_prescriptsave_once, 0, 1 },
    { DS_NOCHK, &actorscrptr[0], sizeof(actorscrptr[0]), MAXTILES },
    { DS_LOADFN|DS_NOCHK, (void *)&sv_prescriptload_once, 0, 1 },
    { DS_DYNAMIC|DS_CNT(g_scriptSize)|DS_NOCHK, &script, sizeof(script[0]), (intptr_t)&g_scriptSize },
    { DS_NOCHK, &actorLoadEventScrptr[0], sizeof(actorLoadEventScrptr[0]), MAXTILES },
//    { DS_NOCHK, &apScriptGameEvent[0], sizeof(apScriptGameEvent[0]), MAXGAMEEVENTS },
    { DS_SAVEFN|DS_LOADFN|DS_NOCHK, (void *)&sv_postscript_once, 0, 1 },

    { DS_SAVEFN, (void *)&sv_preactordatasave, 0, 1 },
    { 0, &savegame_bitmap, sizeof(savegame_bitmap), 1 },
    { 0, &actor[0], sizeof(actor_t), MAXSPRITES },
    { DS_SAVEFN|DS_LOADFN, (void *)&sv_postactordata, 0, 1 },

    { DS_END, 0, 0, 0 }
};

static const dataspec_t svgm_anmisc[] =
{
    { DS_STRING, "blK:anms", 0, 1 },
    { 0, &g_animateCount, sizeof(g_animateCount), 1 },
    { 0, &animatesect[0], sizeof(animatesect[0]), MAXANIMATES },
    { 0, &animategoal[0], sizeof(animategoal[0]), MAXANIMATES },
    { 0, &animatevel[0], sizeof(animatevel[0]), MAXANIMATES },
    { DS_SAVEFN, (void *)&sv_preanimateptrsave, 0, 1 },
    { 0, &animateptr[0], sizeof(animateptr[0]), MAXANIMATES },
    { DS_SAVEFN|DS_LOADFN , (void *)&sv_postanimateptr, 0, 1 },
    { 0, &camsprite, sizeof(camsprite), 1 },
    { 0, &msx[0], sizeof(msx[0]), sizeof(msx)/sizeof(msx[0]) },
    { 0, &msy[0], sizeof(msy[0]), sizeof(msy)/sizeof(msy[0]) },
    { 0, &g_spriteDeleteQueuePos, sizeof(g_spriteDeleteQueuePos), 1 },
    { DS_NOCHK, &g_spriteDeleteQueueSize, sizeof(g_spriteDeleteQueueSize), 1 },
    { DS_CNT(g_spriteDeleteQueueSize), &SpriteDeletionQueue[0], sizeof(int16_t), (intptr_t)&g_spriteDeleteQueueSize },
    { 0, &show2dsector[0], sizeof(uint8_t), MAXSECTORS>>3 },
    { DS_NOCHK, &g_numClouds, sizeof(g_numClouds), 1 },
    { 0, &clouds[0], sizeof(clouds), 1 },
    { 0, &cloudx[0], sizeof(cloudx), 1 },
    { 0, &cloudy[0], sizeof(cloudy), 1 },
    { DS_NOCHK, &parallaxyscale, sizeof(parallaxyscale), 1 },
    { 0, &pskybits, sizeof(pskybits), 1 },
    { 0, &pskyoff[0], sizeof(pskyoff[0]), MAXPSKYTILES },
    { 0, &g_earthquakeTime, sizeof(g_earthquakeTime), 1 },

    { DS_SAVEFN|DS_LOADFN|DS_NOCHK, (void *)sv_prequote, 0, 1 },
    { DS_SAVEFN, (void *)&sv_quotesave, 0, 1 },
    { DS_NOCHK, &savegame_quotedef, sizeof(savegame_quotedef), 1 },  // quotes can change during runtime, but new quote numbers cannot be allocated
    { DS_DYNAMIC, &savegame_quotes, MAXQUOTELEN, MAXQUOTES },
    { DS_LOADFN, (void *)&sv_quoteload, 0, 1 },

    { DS_NOCHK, &g_numQuoteRedefinitions, sizeof(g_numQuoteRedefinitions), 1 },
    { DS_NOCHK|DS_SAVEFN|DS_LOADFN, (void *)&sv_prequoteredef, 0, 1 },
    { DS_NOCHK|DS_SAVEFN, (void *)&sv_quoteredefsave, 0, 1 },  // quote redefinitions replace quotes at runtime, but cannot be changed after CON compilation
    { DS_NOCHK|DS_DYNAMIC|DS_CNT(g_numQuoteRedefinitions), &savegame_quoteredefs, MAXQUOTELEN, (intptr_t)&g_numQuoteRedefinitions },
    { DS_NOCHK|DS_LOADFN, (void *)&sv_quoteredefload, 0, 1 },
    { DS_NOCHK|DS_SAVEFN|DS_LOADFN, (void *)&sv_postquoteredef, 0, 1 },

    { DS_SAVEFN, (void *)&sv_restsave, 0, 1 },
    { 0, savegame_restdata, 1, sizeof(savegame_restdata) },  // sz/cnt swapped for kdfread
    { DS_LOADFN, (void *)&sv_restload, 0, 1 },

    { DS_STRING, "savegame_end", 0, 1 },
    { DS_END, 0, 0, 0 }
};

static dataspec_t *svgm_vars=NULL;
static uint8_t *dosaveplayer2(int32_t spot, FILE *fil, uint8_t *mem);
static int32_t doloadplayer2(int32_t spot, int32_t fil, uint8_t **memptr);
static void postloadplayer1();
static void postloadplayer2();

// SVGM snapshot system
static uint32_t svsnapsiz;
static uint8_t *svsnapshot, *svinitsnap;
static uint32_t svdiffsiz;
static uint8_t *svdiff;

#include "gamedef.h"

#define SV_SKIPMASK (/*GAMEVAR_SYSTEM|*/GAMEVAR_READONLY|GAMEVAR_INTPTR|    \
                     GAMEVAR_SHORTPTR|GAMEVAR_CHARPTR /*|GAMEVAR_NORESET*/ |GAMEVAR_SPECIAL)
// setup gamevar data spec for snapshotting and diffing... gamevars must be loaded when called
static void sv_makevarspec()
{
    static char *magic = "blK:vars";
    int32_t i, j, numsavedvars=0, per;

    if (svgm_vars)
        Bfree(svgm_vars);

    for (i=0; i<g_gameVarCount; i++)
        numsavedvars += (aGameVars[i].dwFlags&SV_SKIPMASK) ? 0 : 1;

    svgm_vars = Bmalloc((numsavedvars+g_gameArrayCount+2)*sizeof(dataspec_t));

    svgm_vars[0].flags = DS_STRING;
    svgm_vars[0].ptr = magic;
    svgm_vars[0].cnt = 1;

    j=1;
    for (i=0; i<g_gameVarCount; i++)
    {
        if (aGameVars[i].dwFlags&SV_SKIPMASK)
            continue;

        per = aGameVars[i].dwFlags&GAMEVAR_USER_MASK;

        svgm_vars[j].flags = 0;
        svgm_vars[j].ptr = (per==0) ? &aGameVars[i].val.lValue : aGameVars[i].val.plValues;
        svgm_vars[j].size = sizeof(intptr_t);
        svgm_vars[j].cnt = (per==0) ? 1 : (per==GAMEVAR_PERPLAYER ? MAXPLAYERS : MAXSPRITES);
        j++;
    }

    for (i=0; i<g_gameArrayCount; i++)
    {
        svgm_vars[j].flags = 0;
        svgm_vars[j].ptr = aGameArrays[i].plValues;
        svgm_vars[j].size = sizeof(aGameArrays[0].plValues[0]);
        svgm_vars[j].cnt = aGameArrays[i].size;  // assumed constant throughout demo, i.e. no RESIZEARRAY
        j++;
    }

    svgm_vars[j].flags = DS_END;
}

void sv_freemem()
{
    if (svsnapshot)
        Bfree(svsnapshot), svsnapshot=NULL;
    if (svinitsnap)
        Bfree(svinitsnap), svinitsnap=NULL;
    if (svdiff)
        Bfree(svdiff), svdiff=NULL;
//    if (svgm_vars)
//        Bfree(svgm_vars), svgm_vars=NULL;
}

static int32_t doallocsnap(int32_t allocinit)
{
    sv_freemem();

    svsnapshot = Bmalloc(svsnapsiz);
    if (allocinit)
        svinitsnap = Bmalloc(svsnapsiz);
    svdiffsiz = svsnapsiz;  // theoretically it's less than could be needed in the worst case, but practically it's overkill
    svdiff = Bmalloc(svdiffsiz);
    if (svsnapshot==NULL || (allocinit && svinitsnap==0) || svdiff==NULL)
    {
        sv_freemem();
        return 1;
    }
    return 0;
}

int32_t sv_saveandmakesnapshot(FILE *fil, int32_t recdiffs, int32_t diffcompress, int32_t synccompress)
{
    uint8_t *p, tb;
    uint16_t ts;

    fwrite("EDuke32demo", 11, 1, fil);//0 11b
    tb = SV_MAJOR_VER;
    fwrite(&tb, sizeof(tb), 1, fil);  //11 1b
    tb = SV_MINOR_VER;
    fwrite(&tb, sizeof(tb), 1, fil);  //12 1b
    tb = sizeof(intptr_t);
    fwrite(&tb, sizeof(tb), 1, fil);  //13 1b

    ts = BYTEVERSION;
    fwrite(&ts, sizeof(ts), 1, fil);  //14 2b

    savegame_comprthres = SV_DEFAULTCOMPRTHRES;
    fwrite(&savegame_comprthres, sizeof(savegame_comprthres), 1, fil);  //16 1b

    tb = recdiffs;
    fwrite(&tb, sizeof(tb), 1, fil);  //17 1b

    savegame_diffcompress = (uint8_t)diffcompress;
    fwrite(&savegame_diffcompress, sizeof(savegame_diffcompress), 1, fil);  //18 1b

    tb = (uint8_t)synccompress;
    fwrite(&tb, sizeof(tb), 1, fil);  //19 1b

    sv_makevarspec();
    svsnapsiz = 0;
    fwrite(&svsnapsiz, sizeof(svsnapsiz), 1, fil);  // 20 4b record count for demos
    svsnapsiz = calcsz(svgm_vars);
    svsnapsiz += calcsz(svgm_udnetw) + calcsz(svgm_secwsp) + calcsz(svgm_script) + calcsz(svgm_anmisc);
    fwrite(&svsnapsiz, sizeof(svsnapsiz), 1, fil);  // 24 4b

    OSD_Printf("sv_saveandmakesnapshot: size: %d bytes.\n", svsnapsiz);

    if (doallocsnap(0))
    {
        OSD_Printf("sv_saveandmakesnapshot: failed allocating memory.\n");
        return 1;
    }

    p = dosaveplayer2(-1, fil, svsnapshot);
    if (p != svsnapshot+svsnapsiz)
        OSD_Printf("sv_saveandmakesnapshot: ptr-(snapshot end)=%d!\n", (int32_t)(p-(svsnapshot+svsnapsiz)));

    return 0;
}

int32_t sv_loadsnapshot(int32_t fil, int32_t *ret_hasdiffs, int32_t *ret_demoticcnt, int32_t *ret_synccompress)
{
    uint8_t *p, tmpbuf[11];
    int32_t i;

    if (kread(fil, tmpbuf, 11) != 11) goto corrupt;
    if (Bmemcmp(tmpbuf, "EDuke32demo", 11))
    {
        OSD_Printf("Missing demo header.\n");
        return 1;
    }

    if (kread(fil, tmpbuf, 9) != 9) goto corrupt;
    if (tmpbuf[0] != SV_MAJOR_VER || tmpbuf[1] != SV_MINOR_VER || *(uint16_t *)&tmpbuf[3] != BYTEVERSION)
    {
        OSD_Printf("Incompatible demo version. Expected %d.%d.%d, found %d.%d.%d\n",
                   SV_MAJOR_VER, SV_MINOR_VER, BYTEVERSION,
                   (int)tmpbuf[0], (int)tmpbuf[1], (int)(*(uint16_t *)&tmpbuf[3]));
        return 2;
    }
    if (tmpbuf[2] != sizeof(intptr_t))
    {
        OSD_Printf("Demo incompatible. Expected pointer size %d, found %d.\n",
                   (int32_t)sizeof(intptr_t), (int32_t)tmpbuf[2]);
        return 3;
    }

    savegame_comprthres = tmpbuf[5];
    *ret_hasdiffs = (int32_t)tmpbuf[6];
    savegame_diffcompress = tmpbuf[7];
    *ret_synccompress = (int32_t)tmpbuf[8];

    if (kread(fil, ret_demoticcnt, sizeof(ret_demoticcnt)) != sizeof(ret_demoticcnt)) goto corrupt;
    if (kread(fil, &svsnapsiz, sizeof(svsnapsiz)) != sizeof(svsnapsiz)) goto corrupt;


    OSD_Printf("sv_loadsnapshot: size: %d bytes.\n", svsnapsiz);

    if (doallocsnap(1))
    {
        OSD_Printf("sv_loadsnapshot: failed allocating memory.\n");
        return 4;
    }

    p = svsnapshot;
    i = doloadplayer2(-1, fil, &p);
    if (i)
    {
        OSD_Printf("sv_loadsnapshot: doloadplayer2() returned %d.\n", i);
        sv_freemem();
        return 5;
    }

    Bmemcpy(svinitsnap, svsnapshot, svsnapsiz);

    postloadplayer1();
    postloadplayer2();

    if (p != svsnapshot+svsnapsiz)
    {
        OSD_Printf("sv_loadsnapshot: internal error: p-(snapshot end)=%d!\n",
                   p-(svsnapshot+svsnapsiz));
        sv_freemem();
        return 6;
    }

    return 0;
corrupt:
    OSD_Printf("Demo header corrupt.\n");
    return 8;
}

uint32_t sv_writediff(FILE *fil)
{
    uint8_t *p=svsnapshot, *d=svdiff;
    uint32_t diffsiz;

    cmpspecdata(svgm_udnetw, &p, &d);
    cmpspecdata(svgm_secwsp, &p, &d);
    cmpspecdata(svgm_script, &p, &d);
    cmpspecdata(svgm_anmisc, &p, &d);
    cmpspecdata(svgm_vars, &p, &d);

    if (p != svsnapshot+svsnapsiz)
        OSD_Printf("sv_writediff: dump+siz=%p, p=%p!\n", svsnapshot+svsnapsiz, p);

    diffsiz = d-svdiff;

    fwrite("dIfF",4,1,fil);
    fwrite(&diffsiz, sizeof(diffsiz), 1, fil);
    if (savegame_diffcompress)
        dfwrite(svdiff, 1, diffsiz, fil);  // cnt and sz swapped
    else
        fwrite(svdiff, 1, diffsiz, fil);

    return diffsiz;
}

int32_t sv_readdiff(int32_t fil)
{
    uint8_t *p=svsnapshot, *d=svdiff, i=0; //, tbuf[4];
    int32_t diffsiz;

#if 0  // handled by the caller
    if (kread(fil, tbuf, 4)!=4)
        return -1;
    if (Bmemcmp(tbuf, "dIfF", 4))
        return 4;
#endif
    if (kread(fil, &diffsiz, sizeof(uint32_t))!=sizeof(uint32_t))
        return -1;
    if (savegame_diffcompress)
    {
        if (kdfread(svdiff, 1, diffsiz, fil) != diffsiz)  // cnt and sz swapped
            return -2;
    }
    else
    {
        if (kread(fil, svdiff, diffsiz) != diffsiz)
            return -2;
    }

    if (applydiff(svgm_udnetw, &p, &d)) return -3;
    if (applydiff(svgm_secwsp, &p, &d)) return -4;
    if (applydiff(svgm_script, &p, &d)) return -5;
    if (applydiff(svgm_anmisc, &p, &d)) return -6;
    if (applydiff(svgm_vars, &p, &d)) return -7;

    if (p!=svsnapshot+svsnapsiz)
        i|=1;
    if (d!=svdiff+diffsiz)
        i|=2;
    if (i)
        OSD_Printf("sv_readdiff: p=%p, svsnapshot+svsnapsiz=%p; d=%p, svdiff+diffsiz=%p",
                   p, svsnapshot+svsnapsiz, d, svdiff+diffsiz);
    return i;
}

// SVGM data description
static void sv_postudload()
{
    Bmemcpy(&boardfilename[0], &currentboardfilename[0], BMAX_PATH);
#if 0
    ud.m_level_number = ud.level_number;
    ud.m_volume_number = ud.volume_number;
    ud.m_player_skill = ud.player_skill;
    ud.m_respawn_monsters = ud.respawn_monsters;
    ud.m_respawn_items = ud.respawn_items;
    ud.m_respawn_inventory = ud.respawn_inventory;
    ud.m_monsters_off = ud.monsters_off;
    ud.m_coop = ud.coop;
    ud.m_marker = ud.marker;
    ud.m_ffire = ud.ffire;
    ud.m_noexits = ud.noexits;
#endif
}
//static int32_t lockclock_dummy;

#if defined(POLYMOST) && defined(USE_OPENGL)
static void sv_prespriteextsave()
{
    int32_t i;
    for (i=0; i<MAXSPRITES; i++)
        if (spriteext[i].mdanimtims)
        {
            spriteext[i].mdanimtims -= mdtims;
            if (spriteext[i].mdanimtims==0)
                spriteext[i].mdanimtims++;
        }
}
static void sv_postspriteext()
{
    int32_t i;
    for (i=0; i<MAXSPRITES; i++)
        if (spriteext[i].mdanimtims)
            spriteext[i].mdanimtims += mdtims;
}
#endif

static void sv_calcbitptrsize()
{
    savegame_bitptrsize = (g_scriptSize+7)>>3;
}
static void sv_prescriptsave_once()
{
    int32_t i;
    for (i=0; i<g_scriptSize; i++)
        if (bitptr[i>>3]&(BITPTR_POINTER<<(i&7)))
            script[i] = (intptr_t)((intptr_t *)script[i] - &script[0]);
    for (i=0; i<MAXTILES; i++)
        if (actorscrptr[i])
            actorscrptr[i] = (intptr_t *)(actorscrptr[i]-&script[0]);
    for (i=0; i<MAXTILES; i++)
        if (actorLoadEventScrptr[i])
            actorLoadEventScrptr[i] = (intptr_t *)(actorLoadEventScrptr[i]-&script[0]);
}
static void sv_prescriptload_once()
{
    if (script)
        Bfree(script);
    script = Bmalloc(g_scriptSize * sizeof(script[0]));
}
static void sv_postscript_once()
{
    int32_t i;
    for (i=0; i<MAXTILES; i++)
        if (actorLoadEventScrptr[i])
            actorLoadEventScrptr[i] = (intptr_t)actorLoadEventScrptr[i] + &script[0];
    for (i=0; i<MAXTILES; i++)
        if (actorscrptr[i])
            actorscrptr[i] = (intptr_t)actorscrptr[i] + &script[0];
    for (i=0; i<g_scriptSize; i++)
        if (bitptr[i>>3]&(BITPTR_POINTER<<(i&7)))
            script[i] = (intptr_t)(script[i] + &script[0]);
}
static void sv_preactordatasave()
{
    int32_t i;
    intptr_t j=(intptr_t)&script[0], k=(intptr_t)&script[g_scriptSize];

    Bmemset(savegame_bitmap, 0, sizeof(savegame_bitmap));
    for (i=0; i<MAXSPRITES; i++)
    {
//        Actor[i].lightptr = NULL;
//        Actor[i].lightId = -1;

        if (sprite[i].statnum==MAXSTATUS || actorscrptr[PN]==NULL) continue;
        if (T2 >= j && T2 < k) savegame_bitmap[i>>3][0] |= 1<<(i&7), T2 -= j;
        if (T5 >= j && T5 < k) savegame_bitmap[i>>3][1] |= 1<<(i&7), T5 -= j;
        if (T6 >= j && T6 < k) savegame_bitmap[i>>3][2] |= 1<<(i&7), T6 -= j;
    }
}
static void sv_postactordata()
{
    int32_t i;
    intptr_t j=(intptr_t)&script[0];

#if POLYMER
    if (getrendermode() == 4)
        polymer_resetlights();
#endif

    for (i=0; i<MAXSPRITES; i++)
    {
//        Actor[i].lightptr = NULL;
//        Actor[i].lightId = -1;
        actor[i].projectile = &SpriteProjectile[i];

        if (sprite[i].statnum==MAXSTATUS || actorscrptr[PN]==NULL) continue;
        if (savegame_bitmap[i>>3][0]&(1<<(i&7))) T2 += j;
        if (savegame_bitmap[i>>3][1]&(1<<(i&7))) T5 += j;
        if (savegame_bitmap[i>>3][2]&(1<<(i&7))) T6 += j;
    }
}

static void sv_preanimateptrsave()
{
    int32_t i;
    for (i=g_animateCount-1; i>=0; i--)
        animateptr[i] = (int32_t *)((intptr_t)animateptr[i]-(intptr_t)&sector[0]);
}
static void sv_postanimateptr()
{
    int32_t i;
    for (i=g_animateCount-1; i>=0; i--)
        animateptr[i] = (int32_t *)((intptr_t)animateptr[i]+(intptr_t)&sector[0]);
}
static void sv_prequote()
{
    if (!savegame_quotes)
        savegame_quotes = Bcalloc(MAXQUOTES, MAXQUOTELEN);
}
static void sv_quotesave()
{
    int32_t i;
    Bmemset(savegame_quotedef, 0, sizeof(savegame_quotedef));
    for (i=0; i<MAXQUOTES; i++)
        if (ScriptQuotes[i])
        {
            savegame_quotedef[i>>3] |= 1<<(i&7);
            Bmemcpy(savegame_quotes[i], ScriptQuotes[i], MAXQUOTELEN);
        }
}
static void sv_quoteload()
{
    int32_t i;
    for (i=0; i<MAXQUOTES; i++)
    {
        if (savegame_quotedef[i>>3]&(1<<(i&7)))
        {
            if (!ScriptQuotes[i])
                ScriptQuotes[i] = Bcalloc(1,MAXQUOTELEN);
            Bmemcpy(ScriptQuotes[i], savegame_quotes[i], MAXQUOTELEN);
        }
    }
}
static void sv_prequoteredef()
{
    // "+1" needed for dfwrite which doesn't handle the src==NULL && cnt==0 case
    savegame_quoteredefs = Bcalloc(g_numQuoteRedefinitions+1, MAXQUOTELEN);
}
static void sv_quoteredefsave()
{
    int32_t i;
    for (i=0; i<g_numQuoteRedefinitions; i++)
        if (ScriptQuoteRedefinitions[i])
            Bmemcpy(savegame_quoteredefs[i], ScriptQuoteRedefinitions[i], MAXQUOTELEN);
}
static void sv_quoteredefload()
{
    int32_t i;
    for (i=0; i<g_numQuoteRedefinitions; i++)
    {
        if (!ScriptQuoteRedefinitions[i])
            ScriptQuoteRedefinitions[i] = Bcalloc(1,MAXQUOTELEN);
        Bmemcpy(ScriptQuoteRedefinitions[i], savegame_quoteredefs[i], MAXQUOTELEN);
    }
}
static void sv_postquoteredef()
{
    Bfree(savegame_quoteredefs), savegame_quoteredefs=NULL;
}
static void sv_restsave()
{
    int32_t i;
    uint8_t *mem = savegame_restdata;
    DukePlayer_t dummy_ps;

    Bmemset(&dummy_ps, 0, sizeof(DukePlayer_t));

#define CPDAT(ptr,sz) Bmemcpy(mem, ptr, sz), mem+=sz
    for (i=0; i<MAXPLAYERS; i++)
    {
        CPDAT(g_player[i].user_name, 32);
        CPDAT(&g_player[i].pcolor, sizeof(g_player[0].pcolor));
        CPDAT(&g_player[i].pteam, sizeof(g_player[0].pteam));
        CPDAT(&g_player[i].frags[0], sizeof(g_player[0].frags));
        if (g_player[i].ps)
            CPDAT(g_player[i].ps, sizeof(DukePlayer_t));
        else
            CPDAT(&dummy_ps, sizeof(DukePlayer_t));
    }
#if 0  // POLYMER  this will have to wait...
    CPDAT(&lightcount, sizeof(lightcount));
    for (i=0; i<lightcount; i++)
    {
        CPDAT(&prlights[i], sizeof(_prlight));
        ((prlight_ *)(mem-sizeof(_prlight)))->planelist = NULL;
    }
#endif
    Bmemset(mem, 0, (savegame_restdata+SVARDATALEN)-mem);
#undef CPDAT
}
static void sv_restload()
{
    int32_t i;
    uint8_t *mem = savegame_restdata;
    DukePlayer_t dummy_ps;

#define CPDAT(ptr,sz) Bmemcpy(ptr, mem, sz), mem+=sz
    for (i=0; i<MAXPLAYERS; i++)
    {
        CPDAT(g_player[i].user_name, 32);
        CPDAT(&g_player[i].pcolor, sizeof(g_player[0].pcolor));
        CPDAT(&g_player[i].pteam, sizeof(g_player[0].pteam));
        CPDAT(&g_player[i].frags[0], sizeof(g_player[0].frags));
        if (g_player[i].ps)
            CPDAT(g_player[i].ps, sizeof(DukePlayer_t));
        else
            CPDAT(&dummy_ps, sizeof(DukePlayer_t));
    }
#if 0  // POLYMER
    CPDAT(&lightcount, sizeof(lightcount));
    for (i=0; i<lightcount; i++)
        CPDAT(&prlights[i], sizeof(_prlight));
#endif
#undef CPDAT
}

#define SAVEWR(ptr, sz, cnt) do { if (fil) dfwrite(ptr,sz,cnt,fil); } while (0)
#define SAVEWRU(ptr, sz, cnt) do { if (fil) fwrite(ptr,sz,cnt,fil); } while (0)

#define PRINTSIZE(name) OSD_Printf(#name ": %d\n", mem-tmem), tmem=mem

static uint8_t *dosaveplayer2(int32_t spot, FILE *fil, uint8_t *mem)
{
    uint8_t *tmem = mem;
    mem=writespecdata(svgm_udnetw, fil, mem);  // user settings, players & net
    PRINTSIZE(ud);

    if (spot>=0)
    {
        SAVEWRU(&ud.savegame[spot][0], 21, 1);
        SAVEWRU("1", 1, 1);
        if (!waloff[TILE_SAVESHOT])
        {
            walock[TILE_SAVESHOT] = 254;
            allocache(&waloff[TILE_SAVESHOT],200*320,&walock[TILE_SAVESHOT]);
            clearbuf((void *)waloff[TILE_SAVESHOT],(200*320)/4,0);
            walock[TILE_SAVESHOT] = 1;
        }
        SAVEWR((char *)waloff[TILE_SAVESHOT], 320, 200);
    }
    else
    {
        char buf[21];
        const time_t t=time(NULL);
        struct tm *st;
        Bsprintf(buf, "Eduke32 demo");
        if (t>=0 && (st = localtime(&t)))
            Bsprintf(buf, "Edemo32 %04d%02d%02d", st->tm_year+1900, st->tm_mon+1, st->tm_mday);
        SAVEWRU(&buf, 21, 1);
        SAVEWRU("\0", 1, 1);  // demos don't save screenshot
    }

    mem=writespecdata(svgm_secwsp, fil, mem);  // sector, wall, sprite
    PRINTSIZE(sws);
    mem=writespecdata(svgm_script, fil, mem);  // script
    PRINTSIZE(script);
    mem=writespecdata(svgm_anmisc, fil, mem);  // animates, quotes & misc.
    PRINTSIZE(animisc);

    Gv_WriteSave(fil, 1);  // gamevars
    mem=writespecdata(svgm_vars, 0, mem);
    PRINTSIZE(vars);

    return mem;
}

#define LOADRD(ptr, sz, cnt) (kdfread(ptr,sz,cnt,fil)!=(cnt))
#define LOADRDU(ptr, sz, cnt) (kread(fil,ptr,(sz)*(cnt))!=(sz)*(cnt))

static int32_t doloadplayer2(int32_t spot, int32_t fil, uint8_t **memptr)
{
    uint8_t *mem = memptr ? *memptr : NULL, *tmem=mem;
    char tbuf[21];
    int32_t i;

    if (readspecdata(svgm_udnetw, fil, &mem))
        return -2;
    PRINTSIZE(ud);
    if (spot >= 0 && ud.multimode!=numplayers)
        return 2;

    if (spot<0 || numplayers > 1)
    {
        if (LOADRDU(&tbuf, 21, 1)) return -3;
    }
    else if (LOADRDU(&ud.savegame[spot][0], 21, 1)) return -3;
    else ud.savegame[spot][19] = 0;

    if (LOADRDU(tbuf, 1, 1)) return -3;
    if (tbuf[0])
    {
        //Fake read because lseek won't work with compression
        walock[TILE_LOADSHOT] = 1;
        if (waloff[TILE_LOADSHOT] == 0) allocache(&waloff[TILE_LOADSHOT],320*200,&walock[TILE_LOADSHOT]);
        tilesizx[TILE_LOADSHOT] = 200;
        tilesizy[TILE_LOADSHOT] = 320;
        if (LOADRD((char *)waloff[TILE_LOADSHOT], 320, 200)) return -3;
        invalidatetile(TILE_LOADSHOT,0,255);
    }

    if (readspecdata(svgm_secwsp, fil, &mem)) return -4;
    PRINTSIZE(sws);
    if (readspecdata(svgm_script, fil, &mem)) return -5;
    PRINTSIZE(script);
    if (readspecdata(svgm_anmisc, fil, &mem)) return -6;
    PRINTSIZE(animisc);

    if (Gv_ReadSave(fil, 1)) return -7;
    sv_makevarspec();

    for (i=1; svgm_vars[i].flags!=DS_END; i++)
    {
        Bmemcpy(mem, svgm_vars[i].ptr, svgm_vars[i].size*svgm_vars[i].cnt);  // careful! works because there are no DS_DYNAMIC's!
        mem += svgm_vars[i].size*svgm_vars[i].cnt;
    }
    PRINTSIZE(vars);

    if (memptr)
        *memptr = mem;
    return 0;
}

int32_t sv_updatestate(int32_t frominit)
{
    uint8_t *p = svsnapshot, *pbeg=p;

    if (frominit)
        Bmemcpy(svsnapshot, svinitsnap, svsnapsiz);

    if (readspecdata(svgm_udnetw, -1, &p)) return -2;
    if (readspecdata(svgm_secwsp, -1, &p)) return -4;
    if (readspecdata(svgm_script, -1, &p)) return -5;
    if (readspecdata(svgm_anmisc, -1, &p)) return -6;

    if (readspecdata(svgm_vars, -1, &p)) return -8;

    if (p != pbeg+svsnapsiz)
        OSD_Printf("sv_updatestate: ptr-(snapshot end)=%d\n", (int32_t)(p-(pbeg+svsnapsiz)));

    if (frominit)
    {
        postloadplayer1();
        postloadplayer2();
    }

    return 0;
}

static void postloadplayer1()
{
    int32_t i;
    //1
    if (g_player[myconnectindex].ps->over_shoulder_on != 0)
    {
        g_cameraDistance = 0;
        g_cameraClock = 0;
        g_player[myconnectindex].ps->over_shoulder_on = 1;
    }

    screenpeek = myconnectindex;
    //2
    //3
    P_UpdateScreenPal(g_player[myconnectindex].ps);
    //4
#if 0
    if (ud.lockout == 0)
    {
        for (i=0; i<g_numAnimWalls; i++)
            if (wall[animwall[i].wallnum].extra >= 0)
                wall[animwall[i].wallnum].picnum = wall[animwall[i].wallnum].extra;
    }
    else
    {
        for (i=0; i<g_numAnimWalls; i++)
            switch (DynamicTileMap[wall[animwall[i].wallnum].picnum])
            {
            case FEMPIC1__STATIC:
                wall[animwall[i].wallnum].picnum = BLANKSCREEN;
                break;
            case FEMPIC2__STATIC:
            case FEMPIC3__STATIC:
                wall[animwall[i].wallnum].picnum = SCREENBREAK6;
                break;
            }
    }
#endif

    //5
    g_numInterpolations = 0;
    startofdynamicinterpolations = 0;

    i = headspritestat[STAT_EFFECTOR];
    while (i >= 0)
    {
        switch (sprite[i].lotag)
        {
        case 31:
            G_SetInterpolation(&sector[sprite[i].sectnum].floorz);
            break;
        case 32:
            G_SetInterpolation(&sector[sprite[i].sectnum].ceilingz);
            break;
        case 25:
            G_SetInterpolation(&sector[sprite[i].sectnum].floorz);
            G_SetInterpolation(&sector[sprite[i].sectnum].ceilingz);
            break;
        case 17:
            G_SetInterpolation(&sector[sprite[i].sectnum].floorz);
            G_SetInterpolation(&sector[sprite[i].sectnum].ceilingz);
            break;
        case 0:
        case 5:
        case 6:
        case 11:
        case 14:
        case 15:
        case 16:
        case 26:
        case 30:
            Sect_SetInterpolation(i);
            break;
        }

        i = nextspritestat[i];
    }

    for (i=g_numInterpolations-1; i>=0; i--) bakipos[i] = *curipos[i];
    for (i = g_animateCount-1; i>=0; i--)
        G_SetInterpolation(animateptr[i]);

    //6
    g_showShareware = 0;
//    everyothertime = 0;
//    clearbufbyte(playerquitflag,MAXPLAYERS,0x01010101);
    for (i=0; i<MAXPLAYERS; i++)
        clearbufbyte(&g_player[i].playerquitflag,1,0x01010101);
}

static void postloadplayer2()
{
    //7
    G_ResetTimers();

#ifdef POLYMER
    if (getrendermode() == 4)
    {
        polymer_loadboard();
        polymer_resetlights();
    }
#elif 0
    if (getrendermode() == 4)
    {
        int32_t i = 0;

        polymer_loadboard();
        while (i < MAXSPRITES)
        {
            if (actor[i].lightptr)
            {
                polymer_deletelight(actor[i].lightId);
                actor[i].lightptr = NULL;
                actor[i].lightId = -1;
            }
            i++;
        }
    }
#endif
}

////////// END GENERIC SAVING/LOADING SYSTEM //////////
