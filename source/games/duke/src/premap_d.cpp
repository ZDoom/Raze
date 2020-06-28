//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2020 - Christoph Oelckers

This file is part of Enhanced Duke Nukem 3D version 1.5 - Atomic Edition

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

EDuke enhancements integrated: 04/13/2003 - Matt Saettler

Note: EDuke source was in transition.  Changes are in-progress in the
source as it is released.

*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "build.h"
#include "names.h"

BEGIN_DUKE_NS 

inline void tloadtile(int tilenum, int palnum = 0)
{
    assert(tilenum < MAXTILES);
	markTileForPrecache(tilenum, palnum);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void cachespritenum(int i)
{
    int maxc;
    int j;
	int pal = sprite[i].pal;

    if(ud.monsters_off && badguy(&sprite[i])) return;

    maxc = 1;

    switch(sprite[i].picnum)
    {
        case HYDRENT:
            tloadtile(BROKEFIREHYDRENT);
            for(j = TOILETWATER; j < (TOILETWATER+4); j++)
                tloadtile(j, pal);
            break;
        case TOILET:
            tloadtile(TOILETBROKE);
            for(j = TOILETWATER; j < (TOILETWATER+4); j++)
                tloadtile(j, pal);
            break;
        case STALL:
            tloadtile(STALLBROKE);
            for(j = TOILETWATER; j < (TOILETWATER+4); j++)
                tloadtile(j, pal);
            break;
        case RUBBERCAN:
            maxc = 2;
            break;
        case TOILETWATER:
            maxc = 4;
            break;
        case FEMPIC1:
            maxc = 44;
            break;
        case LIZTROOP:
        case LIZTROOPRUNNING:
        case LIZTROOPSHOOT:
        case LIZTROOPJETPACK:
        case LIZTROOPONTOILET:
        case LIZTROOPDUCKING:
            for(j = LIZTROOP; j < (LIZTROOP+72); j++)
                   tloadtile(j, pal);
            for(j=HEADJIB1;j<LEGJIB1+3;j++)
                    tloadtile(j, pal);
            maxc = 0;
            break;
        case WOODENHORSE:
            maxc = 5;
            for(j = HORSEONSIDE; j < (HORSEONSIDE+4); j++)
                    tloadtile(j, pal);
            break;
        case NEWBEAST:
        case NEWBEASTSTAYPUT:
            maxc = 90;
            break;
        case BOSS1:
        case BOSS2:
        case BOSS3:
            maxc = 30;
            break;
        case OCTABRAIN:
        case OCTABRAINSTAYPUT:
        case COMMANDER:
        case COMMANDERSTAYPUT:
            maxc = 38;
            break;
        case RECON:
            maxc = 13;
            break;
        case PIGCOP:
        case PIGCOPDIVE:
            maxc = 61;
            break;
        case SHARK:
            maxc = 30;
            break;
        case LIZMAN:
        case LIZMANSPITTING:
        case LIZMANFEEDING:
        case LIZMANJUMP:
            for(j=LIZMANHEAD1;j<LIZMANLEG1+3;j++)
                    tloadtile(j, pal);
            maxc = 80;
            break;
        case APLAYER:
            maxc = 0;
            if(ud.multimode > 1)
            {
                maxc = 5;
                for(j = 1420;j < 1420+106; j++)
                        tloadtile(j, pal);
            }
            break;
        case ATOMICHEALTH:
            maxc = 14;
            break;
        case DRONE:
            maxc = 10;
            break;
        case EXPLODINGBARREL:
        case SEENINE:
        case OOZFILTER:
            maxc = 3;
            break;
        case NUKEBARREL:
        case CAMERA1:
            maxc = 5;
            break;
    }

    for(j = sprite[i].picnum; j < (sprite[i].picnum+maxc); j++)
            tloadtile(j, pal);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void cachegoodsprites(void)
{
    int i;
	
    if (ud.screen_size >= 8)
    {
        tloadtile(BOTTOMSTATUSBAR);
        if (ud.multimode > 1)
        {
            tloadtile(FRAGBAR);
            for (i = MINIFONT; i < MINIFONT + 63; i++)
                tloadtile(i);
        }
    }

    tloadtile(VIEWSCREEN);

    for(i=FOOTPRINTS;i<FOOTPRINTS+3;i++)
            tloadtile(i);

    for( i = BURNING; i < BURNING+14; i++)
            tloadtile(i);

    for( i = BURNING2; i < BURNING2+14; i++)
            tloadtile(i);

    for( i = CRACKKNUCKLES; i < CRACKKNUCKLES+4; i++)
            tloadtile(i);

    for( i = FIRSTGUN; i < FIRSTGUN+3 ; i++ )
            tloadtile(i);

    for( i = EXPLOSION2; i < EXPLOSION2+21 ; i++ )
            tloadtile(i);

    tloadtile(BULLETHOLE);

    for( i = FIRSTGUNRELOAD; i < FIRSTGUNRELOAD+8 ; i++ )
            tloadtile(i);

    tloadtile(FOOTPRINTS);

    for( i = JIBS1; i < (JIBS5+5); i++)
            tloadtile(i);

    for( i = SCRAP1; i < (SCRAP1+19); i++)
            tloadtile(i);

    for( i = SMALLSMOKE; i < (SMALLSMOKE+4); i++)
            tloadtile(i);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void cacheit_d(void)
{
    int i, j;

    cachegoodsprites();

    for (i = 0; i < numwalls; i++)
    {
        tloadtile(wall[i].picnum, wall[i].pal);
        if (wall[i].overpicnum >= 0)
            tloadtile(wall[i].overpicnum, wall[i].pal);
    }

    for (i = 0; i < numsectors; i++)
    {
        tloadtile(sector[i].floorpicnum, sector[i].floorpal);
        tloadtile(sector[i].ceilingpicnum, sector[i].ceilingpal);
        if (sector[i].ceilingpicnum == LA)
        {
            tloadtile(LA + 1);
            tloadtile(LA + 2);
        }
    }

    j = headspritesect[i];
    while (j >= 0)
    {
        if (sprite[j].xrepeat != 0 && sprite[j].yrepeat != 0 && (sprite[j].cstat & 32768) == 0)
            cachespritenum(j);
        j = nextspritesect[j];
    }

    precacheMarkedTiles();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void prelevel_d(int g)
{
    short i, nexti, j, startwall, endwall, lotaglist;
    short lotags[65];

    prelevel_common(g);

    i = headspritestat[0];
    while (i >= 0)
    {
        nexti = nextspritestat[i];

        if (sprite[i].lotag == -1 && (sprite[i].cstat & 16))
        {
            ps[0].exitx = sprite[i].x;
            ps[0].exity = sprite[i].y;
        }
        else switch (sprite[i].picnum)
        {
        case GPSPEED:
            sector[sprite[i].sectnum].extra = sprite[i].lotag;
            deletesprite(i);
            break;

        case CYCLER:
            if (numcyclers >= MAXCYCLERS)
                I_Error("Too many cycling sectors.");
            cyclers[numcyclers][0] = sprite[i].sectnum;
            cyclers[numcyclers][1] = sprite[i].lotag;
            cyclers[numcyclers][2] = sprite[i].shade;
            cyclers[numcyclers][3] = sector[sprite[i].sectnum].floorshade;
            cyclers[numcyclers][4] = sprite[i].hitag;
            cyclers[numcyclers][5] = (sprite[i].ang == 1536);
            numcyclers++;
            deletesprite(i);
            break;
        }
        i = nexti;
    }

    for (i = 0; i < MAXSPRITES; i++)
    {
        if (sprite[i].statnum < MAXSTATUS)
        {
            if (sprite[i].picnum == SECTOREFFECTOR && sprite[i].lotag == 14)
                continue;
            fi.spawn(-1, i);
        }
    }

    for (i = 0; i < MAXSPRITES; i++)
        if (sprite[i].statnum < MAXSTATUS)
        {
            if (sprite[i].picnum == SECTOREFFECTOR && sprite[i].lotag == 14)
                fi.spawn(-1, i);
        }

    lotaglist = 0;

    i = headspritestat[0];
    while (i >= 0)
    {
        switch (sprite[i].picnum)
        {
        case DIPSWITCH + 1:
        case DIPSWITCH2 + 1:
        case PULLSWITCH + 1:
        case HANDSWITCH + 1:
        case SLOTDOOR + 1:
        case LIGHTSWITCH + 1:
        case SPACELIGHTSWITCH + 1:
        case SPACEDOORSWITCH + 1:
        case FRANKENSTINESWITCH + 1:
        case LIGHTSWITCH2 + 1:
        case POWERSWITCH1 + 1:
        case LOCKSWITCH1 + 1:
        case POWERSWITCH2 + 1:
            for (j = 0; j < lotaglist; j++)
                if (sprite[i].lotag == lotags[j])
                    break;

            if (j == lotaglist)
            {
                lotags[lotaglist] = sprite[i].lotag;
                lotaglist++;
                if (lotaglist > 64)
                    I_Error("Too many switches (64 max).");

                j = headspritestat[3];
                while (j >= 0)
                {
                    if (sprite[j].lotag == 12 && sprite[j].hitag == sprite[i].lotag)
                        hittype[j].temp_data[0] = 1;
                    j = nextspritestat[j];
                }
            }
            break;
        }
        i = nextspritestat[i];
    }

    mirrorcnt = 0;

    for (i = 0; i < numwalls; i++)
    {
        walltype* wal;
        wal = &wall[i];

        if (wal->overpicnum == MIRROR && (wal->cstat & 32) != 0)
        {
            j = wal->nextsector;

            if (mirrorcnt > 63)
                I_Error("Too many mirrors (64 max.)");
            if ((j >= 0) && sector[j].ceilingpicnum != MIRROR)
            {
                sector[j].ceilingpicnum = MIRROR;
                sector[j].floorpicnum = MIRROR;
                mirrorwall[mirrorcnt] = i;
                mirrorsector[mirrorcnt] = j;
                mirrorcnt++;
                continue;
            }
        }

        if (numanimwalls >= MAXANIMWALLS)
            I_Error("Too many 'anim' walls (max 512.)");

        animwall[numanimwalls].tag = 0;
        animwall[numanimwalls].wallnum = 0;

        switch (wal->overpicnum)
        {
        case FANSHADOW:
        case FANSPRITE:
            wall->cstat |= 65;
            animwall[numanimwalls].wallnum = i;
            numanimwalls++;
            break;

        case W_FORCEFIELD:
            for (j = 0; j < 3; j++)
                tloadtile(W_FORCEFIELD + j);
        case W_FORCEFIELD + 1:
        case W_FORCEFIELD + 2:
            if (wal->shade > 31)
                wal->cstat = 0;
            else wal->cstat |= 85 + 256;

            if (wal->lotag && wal->nextwall >= 0)
                wall[wal->nextwall].lotag = wal->lotag;

        case BIGFORCE:

            animwall[numanimwalls].wallnum = i;
            numanimwalls++;

            continue;
        }

        wal->extra = -1;

        switch (wal->picnum)
        {
        case W_TECHWALL1:
        case W_TECHWALL2:
        case W_TECHWALL3:
        case W_TECHWALL4:
            animwall[numanimwalls].wallnum = i;
            //                animwall[numanimwalls].tag = -1;
            numanimwalls++;
            break;
        case SCREENBREAK6:
        case SCREENBREAK7:
        case SCREENBREAK8:
            for (j = SCREENBREAK6; j < SCREENBREAK9; j++)
                tloadtile(j);
            animwall[numanimwalls].wallnum = i;
            animwall[numanimwalls].tag = -1;
            numanimwalls++;
            break;

        case FEMPIC1:
        case FEMPIC2:
        case FEMPIC3:

            wal->extra = wal->picnum;
            animwall[numanimwalls].tag = -1;
            if (ud.lockout)
            {
                if (wal->picnum == FEMPIC1)
                    wal->picnum = BLANKSCREEN;
                else wal->picnum = SCREENBREAK6;
            }

            animwall[numanimwalls].wallnum = i;
            animwall[numanimwalls].tag = wal->picnum;
            numanimwalls++;
            break;

        case SCREENBREAK1:
        case SCREENBREAK2:
        case SCREENBREAK3:
        case SCREENBREAK4:
        case SCREENBREAK5:

        case SCREENBREAK9:
        case SCREENBREAK10:
        case SCREENBREAK11:
        case SCREENBREAK12:
        case SCREENBREAK13:
        case SCREENBREAK14:
        case SCREENBREAK15:
        case SCREENBREAK16:
        case SCREENBREAK17:
        case SCREENBREAK18:
        case SCREENBREAK19:

            animwall[numanimwalls].wallnum = i;
            animwall[numanimwalls].tag = wal->picnum;
            numanimwalls++;
            break;
        }
    }

    //Invalidate textures in sector behind mirror
    for (i = 0; i < mirrorcnt; i++)
    {
        startwall = sector[mirrorsector[i]].wallptr;
        endwall = startwall + sector[mirrorsector[i]].wallnum;
        for (j = startwall; j < endwall; j++)
        {
            wall[j].picnum = MIRROR;
            wall[j].overpicnum = MIRROR;
        }
    }
}


#if 0

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void enterlevel(char g)
{
    short i,j;
    long l;
    char levname[256];

    if( (g&MODE_DEMO) != MODE_DEMO ) ud.recstat = ud.m_recstat;
    ud.respawn_monsters = ud.m_respawn_monsters;
    ud.respawn_items    = ud.m_respawn_items;
    ud.respawn_inventory    = ud.m_respawn_inventory;
    ud.monsters_off = ud.m_monsters_off;
    ud.coop = ud.m_coop;
    ud.marker = ud.m_marker;
    ud.ffire = ud.m_ffire;

#ifdef WW2
//sprintf(g_szBuf,"ENTERLEVEL L=%d V=%d",ud.level_number, ud.volume_number);
//AddLog(g_szBuf);
	 // variables are set by pointer...
    OnEvent(EVENT_ENTERLEVEL, -1, -1, -1);
#endif	 
    if( (g&MODE_DEMO) == 0 && ud.recstat == 2)
        ud.recstat = 0;

    FX_StopAllSounds();
    clearsoundlocks();
    FX_SetReverb(0);

    i = ud.screen_size;
    ud.screen_size = 0;
    dofrontscreens();
    vscrn();
    ud.screen_size = i;

#ifndef VOLUMEONE

    if( boardfilename[0] != 0 && ud.m_level_number == 7 && ud.m_volume_number == 0 )
    {
        if ( loadboard( boardfilename,&ps[0].posx, &ps[0].posy, &ps[0].posz, &ps[0].ang,&ps[0].cursectnum ) == -1 )
        {
            initprintf("Map %s not found!\n",boardfilename);
            //gameexit(tempbuf);
	    return 1;
        } else {
            char *p;
            strcpy(levname, boardfilename);
	    p = Bstrrchr(levname,'.');
	    if (!p) strcat(levname,".mhk");
	    else { p[1]='m'; p[2]='h'; p[3]='k'; p[4]=0; }
	    if (!loadmaphack(levname)) initprintf("Loaded map hack file %s\n",levname);
	}
    }
    else if ( loadboard( level_file_names[ (ud.volume_number*11)+ud.level_number],&ps[0].posx, &ps[0].posy, &ps[0].posz, &ps[0].ang,&ps[0].cursectnum ) == -1)
    {
        initprintf("Map %s not found!\n",level_file_names[(ud.volume_number*11)+ud.level_number]);
        //gameexit(tempbuf);
	return 1;
    } else {
        char *p;
        strcpy(levname, level_file_names[ (ud.volume_number*11)+ud.level_number]);
	p = Bstrrchr(levname,'.');
	if (!p) strcat(levname,".mhk");
	else { p[1]='m'; p[2]='h'; p[3]='k'; p[4]=0; }
	if (!loadmaphack(levname)) initprintf("Loaded map hack file %s\n",levname);
    }

#else

    l = strlen(level_file_names[ (ud.volume_number*11)+ud.level_number]);
    copybufbyte( level_file_names[ (ud.volume_number*11)+ud.level_number],&levname[0],l);
    levname[l] = 255;
    levname[l+1] = 0;

    if ( loadboard( levname,&ps[0].posx, &ps[0].posy, &ps[0].posz, &ps[0].ang,&ps[0].cursectnum ) == -1)
    {
        initprintf("Map %s not found!\n",level_file_names[(ud.volume_number*11)+ud.level_number]);
        //gameexit(tempbuf);
	return 1;
    } else {
        char *p;
	p = Bstrrchr(levname,'.');
	if (!p) strcat(levname,".mhk");
	else { p[1]='m'; p[2]='h'; p[3]='k'; p[4]=0; }
	if (!loadmaphack(levname)) initprintf("Loaded map hack file %s\n",levname);
    }
#endif

    clearbufbyte(gotpic,sizeof(gotpic),0L);

    prelevel(g);

    allignwarpelevators();
    resetpspritevars(g);

    cachedebug = 0;
    automapping = 0;

    if(ud.recstat != 2) MUSIC_StopSong();

    cacheit();

    if(ud.recstat != 2)
    {
        music_select = (ud.volume_number*11) + ud.level_number;
        playmusic(&music_fn[0][music_select][0]);
    }

    if( (g&MODE_GAME) || (g&MODE_EOL) )
        ps[myconnectindex].gm = MODE_GAME;
    else if(g&MODE_RESTART)
    {
        if(ud.recstat == 2)
            ps[myconnectindex].gm = MODE_DEMO;
        else ps[myconnectindex].gm = MODE_GAME;
    }

    if( (ud.recstat == 1) && (g&MODE_RESTART) != MODE_RESTART )
        opendemowrite();

#ifdef VOLUMEONE
    if(ud.level_number == 0 && ud.recstat != 2) FTA(40,&ps[myconnectindex]);
#endif

    for(i=connecthead;i>=0;i=connectpoint2[i])
        switch(sector[sprite[ps[i].i].sectnum].floorpicnum)
        {
            case HURTRAIL:
            case FLOORSLIME:
            case FLOORPLASMA:
                resetweapons(i);
                resetinventory(i);
                ps[i].gotweapon[PISTOL_WEAPON] = 0;
                ps[i].ammo_amount[PISTOL_WEAPON] = 0;
                ps[i].curr_weapon = KNEE_WEAPON;
                ps[i].kickback_pic = 0;
                break;
        }

      //PREMAP.C - replace near the my's at the end of the file

     resetmys();

     ps[myconnectindex].palette = palette;
     palto(0,0,0,0);

     setpal(&ps[myconnectindex]);
     flushperms();

     everyothertime = 0;
     global_random = 0;

     ud.last_level = ud.level_number+1;

     clearfifo();

     for(i=numinterpolations-1;i>=0;i--) bakipos[i] = *curipos[i];

     restorepalette = 1;

     flushpackets();
     waitforeverybody();

     palto(0,0,0,0);
     vscrn();
     clearview(0L);
     drawbackground();
     displayrooms(myconnectindex,65536);

     clearbufbyte(playerquitflag,MAXPLAYERS,0x01010101);
     ps[myconnectindex].over_shoulder_on = 0;

     clearfrags();

     resettimevars();  // Here we go
}
#endif

END_DUKE_NS