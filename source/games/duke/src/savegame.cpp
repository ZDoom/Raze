//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "serializer.h"
#include "mapinfo.h"
#include "duke3d.h"


BEGIN_DUKE_NS



FSerializer& Serialize(FSerializer& arc, const char* keyname, weaponhit& w, weaponhit* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("cgg", w.cgg)
            ("picnum", w.picnum)
            ("ang", w.ang)
            ("extra", w.extra)
            ("owner", w.owner)
            ("movflag", w.movflag)
            ("tempang", w.tempang)
            ("actorstayput", w.actorstayput)
            ("dispicnum", w.dispicnum)
            ("timetosleep", w.timetosleep)
            ("floorz", w.floorz)
            ("ceilingz", w.ceilingz)
            ("lastvx", w.lastvx)
            ("lastvy", w.lastvy)
            ("bposx", w.bposx)
            ("bposy", w.bposy)
            ("bposz", w.bposz)
            ("aflags", w.aflags)
            .Array("temp_data", w.temp_data, 6)
            .EndObject();
    }
    return arc;
}


int SerializeGlobals(FSerializer &arc)
{
    if (arc.BeginObject("globals"))
    {
        arc("skill", ud.player_skill);
        ud.m_player_skill = ud.player_skill;
        arc.Array("spriteextra", spriteextra, MAXSPRITES)
            .Array("weaponhit", hittype, MAXSPRITES)
            .Array("sectorextra", sectorextra, numsectors)
            ("rtsplaying", rtsplaying)
            ("tempwallptr", tempwallptr)
            ("sound445done", sound445done)
            ("leveltexttime", levelTextTime)

/*
        extern player_struct ps[MAXPLAYERS];
        extern int spriteqamount;
        extern uint8_t shadedsector[MAXSECTORS];
        extern int lastvisinc;
        extern animwalltype animwall[MAXANIMWALLS];
        extern int numanimwalls;
        extern int animatecnt;
        extern int numclouds;
        extern int camsprite;
        extern int numcyclers;
        extern int earthquaketime;
        extern int freezerhurtowner;
        extern int global_random;
        extern int impact_damage;
        extern int mirrorcnt;
        extern int numplayersprites;
        extern int spriteqloc;

        extern int16_t animatesect[MAXANIMATES];
        extern int* animateptr[MAXANIMATES];
        extern int animategoal[MAXANIMATES];
        extern int animatevel[MAXANIMATES];

        extern int16_t clouds[256];
        extern int16_t cloudx;
        extern int16_t cloudy;
        extern ClockTicks cloudtotalclock;

        extern int16_t spriteq[1024];
        extern int16_t cyclers[MAXCYCLERS][6];
        extern int16_t mirrorsector[64];
        extern int16_t mirrorwall[64];

        extern ClockTicks lockclock;

        extern int wupass;
        extern int chickenplant;
        extern int thunderon;
        extern int ufospawn;
        extern int ufocnt;
        extern int hulkspawn;
        extern int lastlevel;

        extern int geosectorwarp[MAXGEOSECTORS];
        extern int geosectorwarp2[MAXGEOSECTORS];
        extern int geosector[MAXGEOSECTORS];
        extern int geox[MAXGEOSECTORS];
        extern int geoy[MAXGEOSECTORS];
        extern int geox2[MAXGEOSECTORS];
        extern int geoy2[MAXGEOSECTORS];
        extern int geocnt;

        extern short ambientlotag[64];
        extern short ambienthitag[64];
        extern unsigned ambientfx;
        extern int msx[MAXANIMPOINTS], msy[MAXANIMPOINTS];
        extern int WindTime, WindDir;
        extern short fakebubba_spawn, mamaspawn_count, banjosound;
        extern short BellTime, BellSprite;
        extern uint8_t enemysizecheat, ufospawnsminion, pistonsound, chickenphase, RRRA_ExitedLevel, fogactive;
        extern uint32_t everyothertime;
        extern player_orig po[MAXPLAYERS];

        extern uint16_t frags[MAXPLAYERS][MAXPLAYERS];

*/

        if (kdfread(&numcyclers, sizeof(numcyclers), 1, fil) != 1) goto corrupt;
        if (kdfread(&cyclers[0][0], 12, MAXCYCLERS, fil) != MAXCYCLERS) goto corrupt;
        if (kdfread(ps, sizeof(ps), 1, fil) != 1) goto corrupt;
        if (kdfread(po, sizeof(po), 1, fil) != 1) goto corrupt;
        if (kdfread(&numanimwalls, sizeof(numanimwalls), 1, fil) != 1) goto corrupt;
        if (kdfread(&animwall, sizeof(animwall), 1, fil) != 1) goto corrupt;
        if (kdfread(&msx[0], sizeof(int), sizeof(msx) / sizeof(int), fil) != sizeof(msx) / sizeof(int)) goto corrupt;
        if (kdfread(&msy[0], sizeof(int), sizeof(msy) / sizeof(int), fil) != sizeof(msy) / sizeof(int)) goto corrupt;
        if (kdfread((short*)&spriteqloc, sizeof(short), 1, fil) != 1) goto corrupt;
        if (kdfread((short*)&spriteqamount, sizeof(short), 1, fil) != 1) goto corrupt;
        if (kdfread((short*)&spriteq[0], sizeof(short), spriteqamount, fil) != spriteqamount) goto corrupt;
        if (kdfread(&mirrorcnt, sizeof(short), 1, fil) != 1) goto corrupt;
        if (kdfread(&mirrorwall[0], sizeof(short), 64, fil) != 64) goto corrupt;
        if (kdfread(&mirrorsector[0], sizeof(short), 64, fil) != 64) goto corrupt;
        if (kdfread(&show2dsector[0], sizeof(char), MAXSECTORS >> 3, fil) != (MAXSECTORS >> 3)) goto corrupt;
        if (kdfread(&actortype[0], sizeof(char), MAXTILES, fil) != MAXTILES) goto corrupt;

        if (kdfread(&numclouds, sizeof(numclouds), 1, fil) != 1) goto corrupt;
        if (kdfread(&clouds[0], sizeof(short) << 7, 1, fil) != 1) goto corrupt;
        if (kdfread(&cloudx[0], sizeof(short) << 7, 1, fil) != 1) goto corrupt;
        if (kdfread(&cloudy[0], sizeof(short) << 7, 1, fil) != 1) goto corrupt;

        if (kdfread(&script[0], 4, MAXSCRIPTSIZE, fil) != MAXSCRIPTSIZE) goto corrupt;

        if (kdfread(&ptrbuf[0], 4, MAXTILES, fil) != MAXTILES) goto corrupt;
        for (i = 0; i < MAXTILES; i++)
            if (ptrbuf[i])
            {
                actorscrptr[i] = (int*)((intptr_t)&script[0] + ptrbuf[i]);
            }

        if (kdfread(&lockclock, sizeof(lockclock), 1, fil) != 1) goto corrupt;
        if (kdfread(&pskybits, sizeof(pskybits), 1, fil) != 1) goto corrupt;
        if (kdfread(&pskyoff[0], sizeof(pskyoff[0]), MAXPSKYTILES, fil) != MAXPSKYTILES) goto corrupt;

        if (kdfread(&animatecnt, sizeof(animatecnt), 1, fil) != 1) goto corrupt;
        if (kdfread(&animatesect[0], 2, MAXANIMATES, fil) != MAXANIMATES) goto corrupt;
        if (kdfread(&ptrbuf[0], 4, MAXANIMATES, fil) != MAXANIMATES) goto corrupt;
        for (i = animatecnt - 1; i >= 0; i--) animateptr[i] = (int*)((intptr_t)&sector[0] + ptrbuf[i]);
        if (kdfread(&animategoal[0], 4, MAXANIMATES, fil) != MAXANIMATES) goto corrupt;
        if (kdfread(&animatevel[0], 4, MAXANIMATES, fil) != MAXANIMATES) goto corrupt;

        if (kdfread(&earthquaketime, sizeof(earthquaketime), 1, fil) != 1) goto corrupt;
        if (kdfread(&ud.from_bonus, sizeof(ud.from_bonus), 1, fil) != 1) goto corrupt;
        if (kdfread(&ud.secretlevel, sizeof(ud.secretlevel), 1, fil) != 1) goto corrupt;
        if (kdfread(&ud.respawn_monsters, sizeof(ud.respawn_monsters), 1, fil) != 1) goto corrupt;
        ud.m_respawn_monsters = ud.respawn_monsters;
        if (kdfread(&ud.respawn_items, sizeof(ud.respawn_items), 1, fil) != 1) goto corrupt;
        ud.m_respawn_items = ud.respawn_items;
        if (kdfread(&ud.respawn_inventory, sizeof(ud.respawn_inventory), 1, fil) != 1) goto corrupt;
        ud.m_respawn_inventory = ud.respawn_inventory;

        if (kdfread(&ud.god, sizeof(ud.god), 1, fil) != 1) goto corrupt;
        if (kdfread(&ud.auto_run, sizeof(ud.auto_run), 1, fil) != 1) goto corrupt;
        if (kdfread(&ud.crosshair, sizeof(ud.crosshair), 1, fil) != 1) goto corrupt;
        if (kdfread(&ud.monsters_off, sizeof(ud.monsters_off), 1, fil) != 1) goto corrupt;
        ud.m_monsters_off = ud.monsters_off;
        if (kdfread(&ud.last_level, sizeof(ud.last_level), 1, fil) != 1) goto corrupt;
        if (kdfread(&ud.eog, sizeof(ud.eog), 1, fil) != 1) goto corrupt;

        if (kdfread(&ud.coop, sizeof(ud.coop), 1, fil) != 1) goto corrupt;
        ud.m_coop = ud.coop;
        if (kdfread(&ud.marker, sizeof(ud.marker), 1, fil) != 1) goto corrupt;
        ud.m_marker = ud.marker;
        if (kdfread(&ud.ffire, sizeof(ud.ffire), 1, fil) != 1) goto corrupt;
        ud.m_ffire = ud.ffire;

        if (kdfread(&camsprite, sizeof(camsprite), 1, fil) != 1) goto corrupt;
        if (kdfread(&connecthead, sizeof(connecthead), 1, fil) != 1) goto corrupt;
        if (kdfread(connectpoint2, sizeof(connectpoint2), 1, fil) != 1) goto corrupt;
        if (kdfread(&numplayersprites, sizeof(numplayersprites), 1, fil) != 1) goto corrupt;
        if (kdfread((short*)&frags[0][0], sizeof(frags), 1, fil) != 1) goto corrupt;

        if (kdfread(&randomseed, sizeof(randomseed), 1, fil) != 1) goto corrupt;
        if (kdfread(&global_random, sizeof(global_random), 1, fil) != 1) goto corrupt;
        if (kdfread(&parallaxyscale, sizeof(parallaxyscale), 1, fil) != 1) goto corrupt;

        kdfread(&shadedsector[0], sizeof(shadedsector[0]), MAXSECTORS, fil);
        kdfread(&ambientfx, sizeof(ambientfx), 1, fil);
        kdfread(&ambienthitag[0], sizeof(ambienthitag[0]), 64, fil);
        kdfread(&ambientlotag[0], sizeof(ambientlotag[0]), 64, fil);
        kdfread(&ambientsprite[0], sizeof(ambientsprite[0]), 64, fil);
        kdfread(&ufospawn, sizeof(ufospawn), 1, fil);
        kdfread(&ufocnt, sizeof(ufocnt), 1, fil);
        kdfread(&hulkspawn, sizeof(hulkspawn), 1, fil);
        kdfread(&geosector[0], sizeof(geosector[0]), 64, fil);
        kdfread(&geosectorwarp[0], sizeof(geosectorwarp[0]), 64, fil);
        kdfread(&geox[0], sizeof(geox[0]), 64, fil);
        kdfread(&geoy[0], sizeof(geoy[0]), 64, fil);
        kdfread(&geoz[0], sizeof(geoz[0]), 64, fil);
        kdfread(&geosectorwarp2[0], sizeof(geosectorwarp2[0]), 64, fil);
        kdfread(&geox2[0], sizeof(geox2[0]), 64, fil);
        kdfread(&geoy2[0], sizeof(geoy2[0]), 64, fil);
        kdfread(&geoz2[0], sizeof(geoz2[0]), 64, fil);
        kdfread(&geocnt, sizeof(geocnt), 1, fil);
#ifdef RRRA
        kdfread(&WindTime, 4, 1, fil);
        kdfread(&WindDir, 4, 1, fil);
        kdfread(&word_119BD8, 2, 1, fil);
        kdfread(&word_119BDA, 2, 1, fil);
        if (ps[myconnectindex].fogtype > 1)
            sub_86730(ps[myconnectindex].fogtype);
        else if (ps[myconnectindex].fogtype == 0)
            sub_86730(0);
#else
        tilesizx[0] = tilesizy[0] = 0;
#endif
        kclose(fil);

    }

     if(ps[myconnectindex].over_shoulder_on != 0)
     {
         cameradist = 0;
         cameraclock = 0;
         ps[myconnectindex].over_shoulder_on = 1;
     }

     screenpeek = myconnectindex;

     clearbufbyte(gotpic,sizeof(gotpic),0L);
     clearsoundlocks();
         cacheit();

     music_select = (ud.volume_number*11) + ud.level_number;
     playmusic(&music_fn[0][music_select][0]);

     ps[myconnectindex].gm = MODE_GAME;
         ud.recstat = 0;

     if(ps[myconnectindex].jetpack_on)
         spritesound(DUKE_JETPACK_IDLE,ps[myconnectindex].i);

     restorepalette = 1;
     setpal(&ps[myconnectindex]);
     vscrn();

     FX_SetReverb(0);

     if(ud.lockout == 0)
     {
         for(x=0;x<numanimwalls;x++)
             if( wall[animwall[x].wallnum].extra >= 0 )
                 wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
     }
     else
     {
         for(x=0;x<numanimwalls;x++)
             switch(wall[animwall[x].wallnum].picnum)
         {
             case FEMPIC1:
                 wall[animwall[x].wallnum].picnum = BLANKSCREEN;
                 break;
             case FEMPIC2:
             case FEMPIC3:
                 wall[animwall[x].wallnum].picnum = SCREENBREAK6;
                 break;
         }
     }

     numinterpolations = 0;
     startofdynamicinterpolations = 0;

     k = headspritestat[3];
     while(k >= 0)
     {
        switch(sprite[k].lotag)
        {
            case 31:
                setinterpolation(&sector[sprite[k].sectnum].floorz);
                break;
            case 32:
                setinterpolation(&sector[sprite[k].sectnum].ceilingz);
                break;
            case 25:
                setinterpolation(&sector[sprite[k].sectnum].floorz);
                setinterpolation(&sector[sprite[k].sectnum].ceilingz);
                break;
            case 17:
                setinterpolation(&sector[sprite[k].sectnum].floorz);
                setinterpolation(&sector[sprite[k].sectnum].ceilingz);
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
                setsectinterpolate(k);
                break;
        }

        k = nextspritestat[k];
     }

     for(i=numinterpolations-1;i>=0;i--) bakipos[i] = *curipos[i];
     for(i = animatecnt-1;i>=0;i--)
         setinterpolation(animateptr[i]);

     show_shareware = 0;
     everyothertime = 0;

     clearbufbyte(playerquitflag,MAXPLAYERS,0x01010101);

     resetmys();

     ready2send = 1;

     flushpackets();
     clearfifo();
     waitforeverybody();

     resettimevars();

     return(0);
corrupt:
     Bsprintf(buf,"Save game file \"%s\" is corrupt.",fnptr);
     gameexit(buf);
     return -1;
}

int saveplayer(signed char spot)
{
    int i, j;
    char fn[13];
    char mpfn[13];
    char *fnptr;
    FILE *fil;
    int bv = BYTEVERSION;
    int ptrbuf[MAXTILES];

    assert(MAXTILES > MAXANIMATES);
    
    strcpy(fn, "game0.sav");
    strcpy(mpfn, "gameA_00.sav");

    if(spot < 0)
    {
        multiflag = 1;
        multiwhat = 1;
        multipos = -spot-1;
        return -1;
    }

    waitforeverybody();

    if( multiflag == 2 && multiwho != myconnectindex )
    {
        fnptr = mpfn;
        mpfn[4] = spot + 'A';

        if(ud.multimode > 9)
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

    if ((fil = fopen(fnptr,"wb")) == 0) return(-1);

    ready2send = 0;

    dfwrite(&bv,4,1,fil);
    dfwrite(&ud.multimode,sizeof(ud.multimode),1,fil);

    dfwrite(&ud.savegame[spot][0],19,1,fil);
    dfwrite(&ud.volume_number,sizeof(ud.volume_number),1,fil);
    dfwrite(&ud.level_number,sizeof(ud.level_number),1,fil);
    dfwrite(&ud.player_skill,sizeof(ud.player_skill),1,fil);
    dfwrite(&boardfilename[0],BMAX_PATH,1,fil);

    if (!waloff[TILE_SAVESHOT]) {
        walock[TILE_SAVESHOT] = 254;
        allocache((void **)&waloff[TILE_SAVESHOT],200*320,&walock[TILE_SAVESHOT]);
        clearbuf((void*)waloff[TILE_SAVESHOT],(200*320)/4,0);
        walock[TILE_SAVESHOT] = 1;
    }
    dfwrite((char *)waloff[TILE_SAVESHOT],320,200,fil);

    dfwrite(&numwalls,2,1,fil);
    dfwrite(&wall[0],sizeof(walltype),MAXWALLS,fil);
    dfwrite(&numsectors,2,1,fil);
    dfwrite(&sector[0],sizeof(sectortype),MAXSECTORS,fil);
    dfwrite(&sprite[0],sizeof(spritetype),MAXSPRITES,fil);
    dfwrite(&spriteext[0],sizeof(spriteexttype),MAXSPRITES,fil);
    dfwrite(&headspritesect[0],2,MAXSECTORS+1,fil);
    dfwrite(&prevspritesect[0],2,MAXSPRITES,fil);
    dfwrite(&nextspritesect[0],2,MAXSPRITES,fil);
    dfwrite(&headspritestat[0],2,MAXSTATUS+1,fil);
    dfwrite(&prevspritestat[0],2,MAXSPRITES,fil);
    dfwrite(&nextspritestat[0],2,MAXSPRITES,fil);
    dfwrite(&numcyclers,sizeof(numcyclers),1,fil);
    dfwrite(&cyclers[0][0],12,MAXCYCLERS,fil);
    dfwrite(ps,sizeof(ps),1,fil);
    dfwrite(po,sizeof(po),1,fil);
    dfwrite(&numanimwalls,sizeof(numanimwalls),1,fil);
    dfwrite(&animwall,sizeof(animwall),1,fil);
    dfwrite(&msx[0],sizeof(int),sizeof(msx)/sizeof(int),fil);
    dfwrite(&msy[0],sizeof(int),sizeof(msy)/sizeof(int),fil);
    dfwrite(&spriteqloc,sizeof(short),1,fil);
    dfwrite(&spriteqamount,sizeof(short),1,fil);
    dfwrite(&spriteq[0],sizeof(short),spriteqamount,fil);
    dfwrite(&mirrorcnt,sizeof(short),1,fil);
    dfwrite(&mirrorwall[0],sizeof(short),64,fil);
    dfwrite(&mirrorsector[0],sizeof(short),64,fil);
    dfwrite(&show2dsector[0],sizeof(char),MAXSECTORS>>3,fil);
    dfwrite(&actortype[0],sizeof(char),MAXTILES,fil);

    dfwrite(&numclouds,sizeof(numclouds),1,fil);
    dfwrite(&clouds[0],sizeof(short)<<7,1,fil);
    dfwrite(&cloudx[0],sizeof(short)<<7,1,fil);
    dfwrite(&cloudy[0],sizeof(short)<<7,1,fil);

    dfwrite(&script[0],4,MAXSCRIPTSIZE,fil);

    memset(ptrbuf, 0, sizeof(ptrbuf));
    for(i=0;i<MAXTILES;i++)
        if(actorscrptr[i])
        {
            ptrbuf[i] = (int)((intptr_t)actorscrptr[i] - (intptr_t)&script[0]);
        }
    dfwrite(&ptrbuf[0],4,MAXTILES,fil);

    dfwrite(&lockclock,sizeof(lockclock),1,fil);
    dfwrite(&pskybits,sizeof(pskybits),1,fil);
    dfwrite(&pskyoff[0],sizeof(pskyoff[0]),MAXPSKYTILES,fil);
    dfwrite(&animatecnt,sizeof(animatecnt),1,fil);
    dfwrite(&animatesect[0],2,MAXANIMATES,fil);
    for(i = animatecnt-1;i>=0;i--) ptrbuf[i] = (int)((intptr_t)animateptr[i] - (intptr_t)&sector[0]);
    dfwrite(&ptrbuf[0],4,MAXANIMATES,fil);
    dfwrite(&animategoal[0],4,MAXANIMATES,fil);
    dfwrite(&animatevel[0],4,MAXANIMATES,fil);

    dfwrite(&earthquaketime,sizeof(earthquaketime),1,fil);
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
    dfwrite(&numplayersprites,sizeof(numplayersprites),1,fil);
    dfwrite((short *)&frags[0][0],sizeof(frags),1,fil);

    dfwrite(&randomseed,sizeof(randomseed),1,fil);
    dfwrite(&global_random,sizeof(global_random),1,fil);
    dfwrite(&parallaxyscale,sizeof(parallaxyscale),1,fil);

    fclose(fil);

    if(ud.multimode < 2)
    {
    strcpy(fta_quotes[122],"GAME SAVED");
    FTA(122,&ps[myconnectindex]);
    }

    ready2send = 1;

    waitforeverybody();

    ototalclock = totalclock;

    return(0);
}

END_DUKE_NS
