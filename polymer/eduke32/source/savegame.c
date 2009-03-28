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

#include "duke3d.h"

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
        if (kdfread(&dummy,4,1,fil) != 1)
        {
            kclose(fil);
            continue;
        }
        if (dummy != BYTEVERSION)
        {
            kclose(fil);
            continue;
        }
        if (kdfread(&dummy,4,1,fil) != 1)
        {
            kclose(fil);
            continue;
        }
        if (kdfread(&ud.savegame[i][0],19,1,fil) != 1)
        {
            ud.savegame[i][0] = 0;
        }
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

    if (kdfread(&bv,4,1,fil) != 1) goto corrupt;
    /*    if (bv != BYTEVERSION)
        {
            P_DoQuote(114,g_player[myconnectindex].ps);
            kclose(fil);
            return 1;
        }*/

    if (kdfread(&saveh->numplr,sizeof(int32),1,fil) != 1) goto corrupt;

    if (kdfread(saveh->name,19,1,fil) != 1) goto corrupt;
    if (kdfread(&saveh->volnum,sizeof(int32),1,fil) != 1) goto corrupt;
    if (kdfread(&saveh->levnum,sizeof(int32),1,fil) != 1) goto corrupt;
    if (kdfread(&saveh->plrskl,sizeof(int32),1,fil) != 1) goto corrupt;
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
    int32 nump;

    strcpy(fn, "egam0.sav");
    strcpy(mpfn, "egamA_00.sav");

    if (spot < 0)
    {
        multiflag = 1;
        multiwhat = 0;
        multipos = -spot-1;
        return -1;
    }

    if (multiflag == 2 && multiwho != myconnectindex)
    {
        fnptr = mpfn;
        mpfn[4] = spot + 'A';

        if (ud.multimode > 9)
        {
            mpfn[6] = (multiwho/10) + '0';
            mpfn[7] = (multiwho%10) + '0';
        }
        else mpfn[7] = multiwho + '0';
    }
    else
    {
        fnptr = fn;
        fn[4] = spot + '0';
    }

    if ((fil = kopen4loadfrommod(fnptr,0)) == -1) return(-1);

    ready2send = 0;

    if (kdfread(&bv,sizeof(bv),1,fil) != 1) goto corrupt;
    if (kdfread(g_szBuf,bv,1,fil) != 1) goto corrupt;
    g_szBuf[bv]=0;
    //    AddLog(g_szBuf);

    if (kdfread(&bv,4,1,fil) != 1) return -1;
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

    Net_WaitForEverybody();

    FX_StopAllSounds();
    S_ClearSoundLocks();

    if (numplayers > 1)
    {
        if (kdfread(&buf,19,1,fil) != 1) goto corrupt;
    }
    else
    {
        if (kdfread(&ud.savegame[spot][0],19,1,fil) != 1) goto corrupt;
    }


    if (kdfread(&ud.volume_number,sizeof(ud.volume_number),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.level_number,sizeof(ud.level_number),1,fil) != 1) goto corrupt;
    if (kdfread(&ud.player_skill,sizeof(ud.player_skill),1,fil) != 1) goto corrupt;
    if (kdfread(&boardfilename[0],BMAX_PATH,1,fil) != 1) goto corrupt;
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

    if (kdfread(&numwalls,2,1,fil) != 1) goto corrupt;
    if (kdfread(&wall[0],sizeof(walltype),MAXWALLS,fil) != MAXWALLS) goto corrupt;
    if (kdfread(&numsectors,2,1,fil) != 1) goto corrupt;
    if (kdfread(&sector[0],sizeof(sectortype),MAXSECTORS,fil) != MAXSECTORS) goto corrupt;
    if (kdfread(&sprite[0],sizeof(spritetype),MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&spriteext[0],sizeof(spriteext_t),MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
#if defined(POLYMOST) && defined(USE_OPENGL)
    for (i=0; i<MAXSPRITES; i++)
        if (spriteext[i].mdanimtims)
            spriteext[i].mdanimtims+=mdtims;
#endif
    if (kdfread(&headspritesect[0],2,MAXSECTORS+1,fil) != MAXSECTORS+1) goto corrupt;
    if (kdfread(&prevspritesect[0],2,MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&nextspritesect[0],2,MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&headspritestat[STAT_DEFAULT],2,MAXSTATUS+1,fil) != MAXSTATUS+1) goto corrupt;
    if (kdfread(&prevspritestat[STAT_DEFAULT],2,MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&nextspritestat[STAT_DEFAULT],2,MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&g_numCyclers,sizeof(g_numCyclers),1,fil) != 1) goto corrupt;
    if (kdfread(&cyclers[0][0],12,MAXCYCLERS,fil) != MAXCYCLERS) goto corrupt;
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

    if (kdfread(&actorscrptr[0],4,MAXTILES,fil) != MAXTILES) goto corrupt;
    for (i=0; i<MAXTILES; i++)
        if (actorscrptr[i])
        {
            j = (intptr_t)actorscrptr[i]+(intptr_t)&script[0];
            actorscrptr[i] = (intptr_t *)j;
        }
    if (kdfread(&actorLoadEventScrptr[0],4,MAXTILES,fil) != MAXTILES) goto corrupt;
    for (i=0; i<MAXTILES; i++)
        if (actorLoadEventScrptr[i])
        {
            j = (intptr_t)actorLoadEventScrptr[i]+(intptr_t)&script[0];
            actorLoadEventScrptr[i] = (intptr_t *)j;
        }

    scriptptrs = Brealloc(scriptptrs, MAXSPRITES * sizeof(scriptptrs));

    if (kdfread(&scriptptrs[0],sizeof(scriptptrs),MAXSPRITES,fil) != MAXSPRITES) goto corrupt;
    if (kdfread(&ActorExtra[0],sizeof(ActorData_t),MAXSPRITES,fil) != MAXSPRITES) goto corrupt;

    for (i=0; i<MAXSPRITES; i++)
    {
        j = (intptr_t)(&script[0]);
        if (scriptptrs[i]&1) T2 += j;
        if (scriptptrs[i]&2) T5 += j;
        if (scriptptrs[i]&4) T6 += j;
    }

    if (kdfread(&lockclock,sizeof(lockclock),1,fil) != 1) goto corrupt;
    if (kdfread(&pskybits,sizeof(pskybits),1,fil) != 1) goto corrupt;
    if (kdfread(&pskyoff[0],sizeof(pskyoff[0]),MAXPSKYTILES,fil) != MAXPSKYTILES) goto corrupt;

    if (kdfread(&g_animateCount,sizeof(g_animateCount),1,fil) != 1) goto corrupt;
    if (kdfread(&animatesect[0],2,MAXANIMATES,fil) != MAXANIMATES) goto corrupt;
    if (kdfread(&animateptr[0],sizeof(int32_t),MAXANIMATES,fil) != MAXANIMATES) goto corrupt;
    for (i = g_animateCount-1; i>=0; i--) animateptr[i] = (int32_t *)((intptr_t)animateptr[i]+(intptr_t)(&sector[0]));
    if (kdfread(&animategoal[0],4,MAXANIMATES,fil) != MAXANIMATES) goto corrupt;
    if (kdfread(&animatevel[0],4,MAXANIMATES,fil) != MAXANIMATES) goto corrupt;

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


    if (Gv_ReadSave(fil)) goto corrupt;

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
            if (MapInfo[ud.level_number].musicfn1 == NULL)
                MapInfo[ud.level_number].musicfn1 = Bcalloc(Bstrlen(levname)+1,sizeof(uint8_t));
            else if ((Bstrlen(levname)+1) > sizeof(MapInfo[ud.level_number].musicfn1))
                MapInfo[ud.level_number].musicfn1 = Brealloc(MapInfo[ud.level_number].musicfn1,(Bstrlen(levname)+1));
            Bstrcpy(MapInfo[ud.level_number].musicfn1,levname);
        }
        else if (MapInfo[ud.level_number].musicfn1 != NULL)
        {
            Bfree(MapInfo[ud.level_number].musicfn1);
            MapInfo[ud.level_number].musicfn1 = NULL;
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

    if (MapInfo[(uint8_t)g_musicIndex].musicfn != NULL && (i != g_musicIndex || MapInfo[MAXVOLUMES*MAXLEVELS+2].musicfn1))
    {
        MUSIC_StopSong();
        S_PlayMusic(&MapInfo[(uint8_t)g_musicIndex].musicfn[0],g_musicIndex);
    }
    MUSIC_Continue();

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

    mmulti_flushpackets();
    clearfifo();
    Net_WaitForEverybody();

    G_ResetTimers();

#ifdef POLYMER
    if (getrendermode() >= 4)
        polymer_loadboard();
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

    if (spot < 0)
    {
        multiflag = 1;
        multiwhat = 1;
        multipos = -spot-1;
        return -1;
    }

    Net_WaitForEverybody();

    if (multiflag == 2 && multiwho != myconnectindex)
    {
        fnptr = mpfn;
        mpfn[4] = spot + 'A';

        if (ud.multimode > 9)
        {
            mpfn[6] = (multiwho/10) + '0';
            mpfn[7] = multiwho + '0';
        }
        else mpfn[7] = multiwho + '0';
    }
    else
    {
        fnptr = fn;
        fn[4] = spot + '0';
    }

    {
        char temp[BMAX_PATH];
        if (mod_dir[0] != '/')
            Bsprintf(temp,"%s/%s",mod_dir,fnptr);
        else Bsprintf(temp,"%s",fnptr);
        if ((fil = fopen(temp,"wb")) == 0) return(-1);
    }

    ready2send = 0;

    Bsprintf(g_szBuf,"EDuke32");
    i=strlen(g_szBuf);
    dfwrite(&i,sizeof(i),1,fil);
    dfwrite(g_szBuf,i,1,fil);

    dfwrite(&bv,4,1,fil);
    dfwrite(&ud.multimode,sizeof(ud.multimode),1,fil);

    dfwrite(&ud.savegame[spot][0],19,1,fil);
    dfwrite(&ud.volume_number,sizeof(ud.volume_number),1,fil);
    dfwrite(&ud.level_number,sizeof(ud.level_number),1,fil);
    dfwrite(&ud.player_skill,sizeof(ud.player_skill),1,fil);
    dfwrite(&currentboardfilename[0],BMAX_PATH,1,fil);
    if (!waloff[TILE_SAVESHOT])
    {
        walock[TILE_SAVESHOT] = 254;
        allocache(&waloff[TILE_SAVESHOT],200*320,&walock[TILE_SAVESHOT]);
        clearbuf((void*)waloff[TILE_SAVESHOT],(200*320)/4,0);
        walock[TILE_SAVESHOT] = 1;
    }
    dfwrite((char *)waloff[TILE_SAVESHOT],320,200,fil);

    dfwrite(&numwalls,2,1,fil);
    dfwrite(&wall[0],sizeof(walltype),MAXWALLS,fil);
    dfwrite(&numsectors,2,1,fil);
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
#if defined(POLYMOST) && defined(USE_OPENGL)
    for (i=0; i<MAXSPRITES; i++)if (spriteext[i].mdanimtims)spriteext[i].mdanimtims+=mdtims;
#endif
    dfwrite(&headspritesect[0],2,MAXSECTORS+1,fil);
    dfwrite(&prevspritesect[0],2,MAXSPRITES,fil);
    dfwrite(&nextspritesect[0],2,MAXSPRITES,fil);
    dfwrite(&headspritestat[STAT_DEFAULT],2,MAXSTATUS+1,fil);
    dfwrite(&prevspritestat[STAT_DEFAULT],2,MAXSPRITES,fil);
    dfwrite(&nextspritestat[STAT_DEFAULT],2,MAXSPRITES,fil);
    dfwrite(&g_numCyclers,sizeof(g_numCyclers),1,fil);
    dfwrite(&cyclers[0][0],12,MAXCYCLERS,fil);
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
    dfwrite(&bitptr[0],sizeof(uint8_t),(g_scriptSize+7)>>3,fil);
    dfwrite(&script[0],sizeof(script),g_scriptSize,fil);

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
    dfwrite(&actorscrptr[0],4,MAXTILES,fil);
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
    dfwrite(&actorLoadEventScrptr[0],4,MAXTILES,fil);
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
    dfwrite(&ActorExtra[0],sizeof(ActorData_t),MAXSPRITES,fil);

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
    dfwrite(&animatesect[0],2,MAXANIMATES,fil);
    for (i = g_animateCount-1; i>=0; i--) animateptr[i] = (int32_t *)((intptr_t)animateptr[i]-(intptr_t)(&sector[0]));
    dfwrite(&animateptr[0],4,MAXANIMATES,fil);
    for (i = g_animateCount-1; i>=0; i--) animateptr[i] = (int32_t *)((intptr_t)animateptr[i]+(intptr_t)(&sector[0]));
    dfwrite(&animategoal[0],4,MAXANIMATES,fil);
    dfwrite(&animatevel[0],4,MAXANIMATES,fil);

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

    Gv_WriteSave(fil);

    fclose(fil);

    if (ud.multimode < 2)
    {
        strcpy(ScriptQuotes[122],"GAME SAVED");
        P_DoQuote(122,g_player[myconnectindex].ps);
    }

    ready2send = 1;

    Net_WaitForEverybody();

    ototalclock = totalclock;

    return(0);
}
