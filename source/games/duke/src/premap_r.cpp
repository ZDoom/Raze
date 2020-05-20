//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT

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
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "names_rr.h"

BEGIN_DUKE_NS 

static inline void tloadtile(int tilenum, int palnum = 0)
{
	markTileForPrecache(tilenum, palnum);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void cachespritenum(short i)
{
    char maxc;
    short j;
    int pal = sprite[i].pal;

    if (ud.monsters_off && badguy(&sprite[i])) return;

    maxc = 1;

    switch (sprite[i].picnum)
    {
    case HYDRENT:
        tloadtile(BROKEFIREHYDRENT);
        for (j = TOILETWATER; j < (TOILETWATER + 4); j++)
            tloadtile(j, pal);
        break;
    case RRTILE2121:
    case RRTILE2122:
        tloadtile(sprite[i].picnum, pal);
        break;
    case TOILET:
        tloadtile(TOILETBROKE);
        for (j = TOILETWATER; j < (TOILETWATER + 4); j++)
            tloadtile(j, pal);
        break;
    case STALL:
        tloadtile(STALLBROKE);
        for (j = TOILETWATER; j < (TOILETWATER + 4); j++)
            tloadtile(j, pal);
        break;
    case FORCERIPPLE:
        maxc = 9;
        break;
    case RUBBERCAN:
        maxc = 2;
        break;
    case TOILETWATER:
        maxc = 4;
        break;
    case BUBBASTAND:
        for (j = BUBBASCRATCH; j <= (BUBBASCRATCH + 47); j++)
            tloadtile(j, pal);
        maxc = 0;
        break;
    case SBSWIPE:
        if (isRRRA())
            for (j = SBSWIPE; j <= (SBSWIPE + 29); j++)
                tloadtile(j, pal);
        maxc = 0;
        break;

    case COOT:
        for (j = COOT; j <= (COOT + 217); j++)
            tloadtile(j, pal);
        for (j = COOTJIBA; j < COOTJIBC + 4; j++)
            tloadtile(j, pal);
        maxc = 0;
        break;
    case LTH:
        maxc = 105;
        for (j = LTH; j < (LTH + maxc); j++)
            tloadtile(j, pal);
        maxc = 0;
        break;
    case BILLYRAY:
        maxc = 144;
        for (j = BILLYWALK; j < (BILLYWALK + maxc); j++)
            tloadtile(j, pal);
        for (j = BILLYJIBA; j <= BILLYJIBB + 4; j++)
            tloadtile(j, pal);
        maxc = 0;
        break;
    case COW:
        maxc = 56;
        for (j = sprite[i].picnum; j < (sprite[i].picnum + maxc); j++)
            tloadtile(j, pal);
        maxc = 0;
        break;
    case DOGRUN:
        for (j = DOGATTACK; j <= DOGATTACK + 35; j++)
            tloadtile(j, pal);
        for (j = DOGRUN; j <= DOGRUN + 80; j++)
            tloadtile(j, pal);
        maxc = 0;
        break;
    case RABBIT:
        if (isRRRA())
        {
            for (j = RABBIT; j <= RABBIT + 54; j++)
                tloadtile(j, pal);
            for (j = RABBIT + 56; j <= RABBIT + 56 + 49; j++)
                tloadtile(j, pal);
            for (j = RABBIT + 56; j <= RABBIT + 56 + 49; j++)
                tloadtile(j, pal);
            maxc = 0;
        }
        break;
    case BIKERB:
    case BIKERBV2:
        if (isRRRA())
        {
            for (j = BIKERB; j <= BIKERB + 104; j++)
                tloadtile(j, pal);
            maxc = 0;
        }
        break;
    case BIKER:
        if (isRRRA())
        {
            for (j = BIKER; j <= BIKER + 116; j++)
                tloadtile(j, pal);
            for (j = BIKER + 150; j <= BIKER + 150 + 104; j++)
                tloadtile(j, pal);
            maxc = 0;
        }
        break;
    case CHEER:
        if (isRRRA())
        {
            for (j = CHEER; j <= CHEER + 44; j++)
                tloadtile(j, pal);
            for (j = CHEER + 47; j <= CHEER + 47 + 211; j++)
                tloadtile(j, pal);
            for (j = CHEER + 262; j <= CHEER + 262 + 72; j++)
                tloadtile(j, pal);
            maxc = 0;
        }
        break;
    case CHEERB:
        if (isRRRA())
        {
            for (j = CHEERB; j <= CHEERB + 83; j++)
                tloadtile(j, pal);
            for (j = CHEERB + 157; j <= CHEERB + 157 + 83; j++)
                tloadtile(j, pal);
            maxc = 0;
        }
        break;
    case MAMA:
        if (isRRRA())
        {
            for (j = MAMA; j <= MAMA + 78; j++)
                tloadtile(j, pal);
            for (j = MAMA + 80; j <= MAMA + 80 + 7; j++)
                tloadtile(j, pal);
            for (j = MAMA + 90; j <= MAMA + 90 + 94; j++)
                tloadtile(j, pal);
            maxc = 0;
        }
        break;
    case CHEERBOAT:
        if (isRRRA())
        {
            tloadtile(CHEERBOAT);
            maxc = 0;
        }
        break;
    case HULKBOAT:
        if (isRRRA())
        {
            tloadtile(HULKBOAT);
            maxc = 0;
        }
        break;
    case MINIONBOAT:
        if (isRRRA())
        {
            tloadtile(MINIONBOAT);
            maxc = 0;
        }
        break;
    case BILLYPLAY:
        if (isRRRA())
        {
            for (j = BILLYPLAY; j <= BILLYPLAY + 2; j++)
                tloadtile(j, pal);
            maxc = 0;
        }
        break;
    case COOTPLAY:
        if (isRRRA())
        {
            for (j = COOTPLAY; j <= COOTPLAY + 4; j++)
                tloadtile(j, pal);
            maxc = 0;
        }
        break;
    case PIG:
    case PIGSTAYPUT:
        maxc = 68;
        break;
    case TORNADO:
        maxc = 7;
        break;
    case HEN:
    case HENSTAND:
        maxc = 34;
        break;
    case APLAYER:
        maxc = 0;
        if (ud.multimode > 1)
        {
            maxc = 5;
            for (j = APLAYER; j < APLAYER + 220; j++)
                tloadtile(j, pal);
            for (j = DUKEGUN; j < DUKELEG + 4; j++)
                tloadtile(j, pal);
        }
        break;
    case ATOMICHEALTH:
        maxc = 14;
        break;
    case DRONE:
        maxc = 6;
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
    case VIXEN:
        maxc = 214;
        for (j = sprite[i].picnum; j < sprite[i].picnum + maxc; j++)
            tloadtile(j, pal);
        maxc = 0;
        break;
    case SBMOVE:
        if (!isRRRA())
        {

            maxc = 54;
            for (j = sprite[i].picnum; j < sprite[i].picnum + maxc; j++)
                tloadtile(j, pal);
            maxc = 100;
            for (j = SBMOVE; j < SBMOVE + maxc; j++)
                tloadtile(j, pal);
            maxc = 0;
        }
        break;
    case HULK:
        maxc = 40;
        for (j = sprite[i].picnum - 41; j < sprite[i].picnum + maxc - 41; j++)
            tloadtile(j, pal);
        for (j = HULKJIBA; j <= HULKJIBC + 4; j++)
            tloadtile(j, pal);
        maxc = 0;
        break;
    case MINION:
        maxc = 141;
        for (j = sprite[i].picnum; j < sprite[i].picnum + maxc; j++)
            tloadtile(j, pal);
        for (j = MINJIBA; j <= MINJIBC + 4; j++)
            tloadtile(j, pal);
        maxc = 0;
        break;


    }

    for (j = sprite[i].picnum; j < (sprite[i].picnum + maxc); j++)
        tloadtile(j, pal);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void cachegoodsprites(void)
{
    short i;

    if (ud.screen_size >= 8)
    {
        tloadtile(BOTTOMSTATUSBAR);
        if (ud.multimode > 1)
        {
            tloadtile(FRAGBAR);
        }
    }

    //tloadtile(VIEWSCREEN);

    for (i = FOOTPRINTS; i < FOOTPRINTS + 3; i++)
        tloadtile(i);

    for (i = BURNING; i < BURNING + 14; i++)
        tloadtile(i);

    for (i = FIRSTGUN; i < FIRSTGUN + 10; i++)
        tloadtile(i);

    for (i = EXPLOSION2; i < EXPLOSION2 + 21; i++)
        tloadtile(i);

    tloadtile(BULLETHOLE);

    for (i = SHOTGUN; i < SHOTGUN + 8; i++)
        tloadtile(i);

    tloadtile(FOOTPRINTS);

    for (i = JIBS1; i < (JIBS5 + 5); i++)
        tloadtile(i);

    for (i = SCRAP1; i < (SCRAP1 + 19); i++)
        tloadtile(i);

    for (i = SMALLSMOKE; i < (SMALLSMOKE + 4); i++)
        tloadtile(i);

    if (isRRRA() && ud.volume_number == 0 && ud.level_number == 4)
    {
        tloadtile(RRTILE2577);
    }
    if (!isRRRA() && ud.volume_number == 1 && ud.level_number == 2)
    {
        tloadtile(RRTILE3190);
        tloadtile(RRTILE3191);
        tloadtile(RRTILE3192);
        tloadtile(RRTILE3144);
        tloadtile(RRTILE3139);
        tloadtile(RRTILE3132);
        tloadtile(RRTILE3120);
        tloadtile(RRTILE3121);
        tloadtile(RRTILE3122);
        tloadtile(RRTILE3123);
        tloadtile(RRTILE3124);
    }
    if (lastlevel)
    {
        i = isRRRA() ? UFO1_RRRA : UFO1_RR;
        tloadtile(i);
        i = UFO2;
        tloadtile(i);
        i = UFO3;
        tloadtile(i);
        i = UFO4;
        tloadtile(i);
        i = UFO5;
        tloadtile(i);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void cacheit_r(void)
{
    short i,j;

    cachegoodsprites();

    for(i=0;i<numwalls;i++)
    {
            tloadtile(wall[i].picnum, wall[i].pal);
        if(wall[i].overpicnum >= 0)
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
    while(j >= 0)
    {
        if(sprite[j].xrepeat != 0 && sprite[j].yrepeat != 0 && (sprite[j].cstat&32768) == 0)
                cachespritenum(j);
        j = nextspritesect[j];
    }
    precacheMarkedTiles();
}

#if 0
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void prelevel(char g)
{
    struct player_struct* p;
    short i;
    short nexti;
    short j;
    short startwall;
    short endwall;
    short lotaglist;
    short k;
    short lotags[65];
    int speed;
    int dist;
    short sound;
    sound = 0;

    p = &ps[screenpeek];


#ifdef RRRA
    sub_86730(0);
    p->fogtype = 0;
    p->raat5dd = 0;
    p->raat5fd = 0;
    p->raat601 = 0;
    p->SlotWin = 0;
    p->raat607 = 0;
    p->raat609 = 0;
    mamaspawn_count = 15;
    word_119BDC = 0;
    word_119BE2 = 0;
    if (ud.level_number == 3 && ud.volume_number == 0)
        mamaspawn_count = 5;
    else if (ud.level_number == 2 && ud.volume_number == 1)
        mamaspawn_count = 10;
    else if (ud.level_number == 6 && ud.volume_number == 1)
        mamaspawn_count = 15;
    else if (ud.level_number == 4 && ud.volume_number == 1)
        ps[myconnectindex].steroids_amount = 0;
#endif

    clearbufbyte(show2dsector, sizeof(show2dsector), 0L);
    clearbufbyte(show2dwall, sizeof(show2dwall), 0L);
    clearbufbyte(show2dsprite, sizeof(show2dsprite), 0L);

    for (i = 0; i < MAXSECTORS; i++)
        shadedsector[i] = 0;

    for (i = 0; i < 64; i++)
    {
        geosectorwarp[i] = -1;
        geosectorwarp2[i] = -1;
    }

    for (i = 0; i < 64; i++)
    {
        ambienthitag[i] = -1;
        ambientlotag[i] = -1;
        ambientsprite[i] = -1;
    }

    resetprestat(0, g);
    lightnincnt = 0;
    torchcnt = 0;
    geocnt = 0;
    jaildoorcnt = 0;
    minecartcnt = 0;
    ambientfx = 0;
    crashcnt = 0;
    thunderon = 0;
    chickenplant = 0;
#ifdef RRRA
    WindTime = 0;
    WindDir = 0;
    fakebubba_spawn = 0;
    word_119BE2 = 0;
    mamaspawn_count = 15;
    BellTime = 0;
    word_119BE0 = 0;

    for (j = 0; j < MAXSPRITES; j++)
    {
        if (sprite[j].pal == 100)
        {
            if (numplayers > 1)
                deletesprite(j);
            else
                sprite[j].pal = 0;
        }
        else if (sprite[j].pal == 101)
        {
            sprite[j].extra = 0;
            sprite[j].hitag = 1;
            sprite[j].pal = 0;
            changespritestat(j, 118);
        }
    }
#endif

    for (i = 0; i < numsectors; i++)
    {
        if (sector[i].ceilingpicnum == RRTILE2577)
            thunderon = 1;
        sector[i].extra = 256;

        switch (sector[i].lotag)
        {
        case 41:
            k = headspritesect[i];
            while (k != -1)
            {
                nexti = nextspritesect[k];
                if (sprite[k].picnum == RRTILE11)
                {
                    dist = sprite[k].lotag << 4;
                    speed = sprite[k].hitag;
                    deletesprite(k);
                }
                if (sprite[k].picnum == RRTILE38)
                {
                    sound = sprite[k].lotag;
                    deletesprite(k);
                }
                k = nexti;
            }
            for (j = 0; j < numsectors; j++)
            {
                if (sector[i].hitag == sector[j].hitag && j != i)
                {
                    if (jaildoorcnt > 32)
                        gameexit("\nToo many jaildoor sectors");
                    jaildoordist[jaildoorcnt] = dist;
                    jaildoorspeed[jaildoorcnt] = speed;
                    jaildoorsecthtag[jaildoorcnt] = sector[i].hitag;
                    jaildoorsect[jaildoorcnt] = j;
                    jaildoordrag[jaildoorcnt] = 0;
                    jaildooropen[jaildoorcnt] = 0;
                    jaildoordir[jaildoorcnt] = sector[j].lotag;
                    jaildoorsound[jaildoorcnt] = sound;
                    jaildoorcnt++;
                }
            }
            break;
        case 42:
        {
            short ii;
            k = headspritesect[i];
            while (k != -1)
            {
                nexti = nextspritesect[k];
                if (sprite[k].picnum == RRTILE64)
                {
                    dist = sprite[k].lotag << 4;
                    speed = sprite[k].hitag;
                    for (ii = 0; ii < MAXSPRITES; ii++)
                    {
                        if (sprite[ii].picnum == RRTILE66)
                            if (sprite[ii].lotag == sprite[k].sectnum)
                            {
                                minecartchildsect[minecartcnt] = sprite[ii].sectnum;
                                deletesprite(ii);
                            }
                    }
                    deletesprite(k);
                }
                if (sprite[k].picnum == RRTILE65)
                {
                    sound = sprite[k].lotag;
                    deletesprite(k);
                }
                k = nexti;
            }
            if (minecartcnt > 16)
                gameexit("\nToo many minecart sectors");
            minecartdist[minecartcnt] = dist;
            minecartspeed[minecartcnt] = speed;
            minecartsect[minecartcnt] = i;
            minecartdir[minecartcnt] = sector[i].hitag;
            minecartdrag[minecartcnt] = dist;
            minecartopen[minecartcnt] = 1;
            minecartsound[minecartcnt] = sound;
            minecartcnt++;
            break;
        }
        case 20:
        case 22:
            if (sector[i].floorz > sector[i].ceilingz)
                sector[i].lotag |= 32768;
            continue;
        }

        if (sector[i].ceilingstat & 1)
        {
            if (waloff[sector[i].ceilingpicnum] == 0)
            {
                if (sector[i].ceilingpicnum == LA)
                    for (j = 0; j < 5; j++)
                        if (waloff[sector[i].ceilingpicnum + j] == 0)
                            tloadtile(sector[i].ceilingpicnum + j);
            }
            setupbackdrop(sector[i].ceilingpicnum);

            if (ps[0].one_parallax_sectnum == -1)
                ps[0].one_parallax_sectnum = i;
        }

        if (sector[i].lotag == 32767) //Found a secret room
        {
            ps[0].max_secret_rooms++;
            continue;
        }

        if (sector[i].lotag == -1)
        {
            ps[0].exitx = wall[sector[i].wallptr].x;
            ps[0].exity = wall[sector[i].wallptr].y;
            continue;
        }
    }

    i = headspritestat[0];
    while (i >= 0)
    {
        nexti = nextspritestat[i];

        if (sprite[i].lotag == -1 && (sprite[i].cstat & 16))
        {
            ps[0].exitx = SX;
            ps[0].exity = SY;
        }
        else switch (sprite[i].picnum)
        {
        case NUKEBUTTON:
            chickenplant = 1;
            break;

        case GPSPEED:
            sector[SECT].extra = SLT;
            deletesprite(i);
            break;

        case CYCLER:
            if (numcyclers >= MAXCYCLERS)
                gameexit("\nToo many cycling sectors.");
            cyclers[numcyclers][0] = SECT;
            cyclers[numcyclers][1] = SLT;
            cyclers[numcyclers][2] = SS;
            cyclers[numcyclers][3] = sector[SECT].floorshade;
            cyclers[numcyclers][4] = SHT;
            cyclers[numcyclers][5] = (SA == 1536);
            numcyclers++;
            deletesprite(i);
            break;

        case RRTILE18:
            if (torchcnt > 64)
                gameexit("\nToo many torch effects");
            torchsector[torchcnt] = SECT;
            torchsectorshade[torchcnt] = sector[SECT].floorshade;
            torchtype[torchcnt] = SLT;
            torchcnt++;
            deletesprite(i);
            break;

        case RRTILE35:
            if (lightnincnt > 64)
                gameexit("\nToo many lightnin effects");
            lightninsector[lightnincnt] = SECT;
            lightninsectorshade[lightnincnt] = sector[SECT].floorshade;
            lightnincnt++;
            deletesprite(i);
            break;

        case RRTILE68:
            shadedsector[SECT] = 1;
            deletesprite(i);
            break;

        case RRTILE67:
            sprite[i].cstat |= 32768;
            break;

        case SOUNDFX:
            if (ambientfx >= 64)
                gameexit("\nToo many ambient effects");
            else
            {
                ambienthitag[ambientfx] = SHT;
                ambientlotag[ambientfx] = SLT;
                ambientsprite[ambientfx] = i;
                sprite[i].ang = ambientfx;
                ambientfx++;
                sprite[i].lotag = 0;
                sprite[i].hitag = 0;
            }
            break;
        }
        i = nexti;
    }

    for (i = 0; i < MAXSPRITES; i++)
    {
        if (sprite[i].picnum == RRTILE19)
        {
            if (geocnt > 64)
                gameexit("\nToo many geometry effects");
            if (sprite[i].hitag == 0)
            {
                geosector[geocnt] = sprite[i].sectnum;
                for (j = 0; j < MAXSPRITES; j++)
                {
                    if (sprite[i].lotag == sprite[j].lotag && j != i && sprite[j].picnum == RRTILE19)
                    {
                        if (sprite[j].hitag == 1)
                        {
                            geosectorwarp[geocnt] = sprite[j].sectnum;
                            geox[geocnt] = sprite[i].x - sprite[j].x;
                            geoy[geocnt] = sprite[i].y - sprite[j].y;
                            geoz[geocnt] = sprite[i].z - sprite[j].z;
                        }
                        if (sprite[j].hitag == 2)
                        {
                            geosectorwarp2[geocnt] = sprite[j].sectnum;
                            geox2[geocnt] = sprite[i].x - sprite[j].x;
                            geoy2[geocnt] = sprite[i].y - sprite[j].y;
                            geoz2[geocnt] = sprite[i].z - sprite[j].z;
                        }
                    }
                }
                geocnt++;
            }
        }
    }

    for (i = 0; i < MAXSPRITES; i++)
    {
        if (sprite[i].statnum < MAXSTATUS)
        {
            if (sprite[i].picnum == SECTOREFFECTOR && SLT == 14)
                continue;
            spawn(-1, i);
        }
    }

    for (i = 0; i < MAXSPRITES; i++)
    {
        if (sprite[i].statnum < MAXSTATUS)
        {
            if (sprite[i].picnum == SECTOREFFECTOR && SLT == 14)
                spawn(-1, i);
        }
        if (sprite[i].picnum == RRTILE19)
            deletesprite(i);
        if (sprite[i].picnum == RRTILE34)
        {
            sector[sprite[i].sectnum].filler = sprite[i].lotag;
            deletesprite(i);
        }
    }

    lotaglist = 0;

    i = headspritestat[0];
    while (i >= 0)
    {
        switch (sprite[i].picnum)
        {
        case DIPSWITCH:
        case DIPSWITCH2:
        case ACCESSSWITCH:
        case PULLSWITCH:
        case HANDSWITCH:
        case SLOTDOOR:
        case LIGHTSWITCH:
        case SPACELIGHTSWITCH:
        case SPACEDOORSWITCH:
        case FRANKENSTINESWITCH:
        case LIGHTSWITCH2:
        case POWERSWITCH1:
        case LOCKSWITCH1:
        case POWERSWITCH2:
#ifdef RRRA
        case RRTILE8464:
#endif
            break;
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
        case NUKEBUTTON:
        case NUKEBUTTON + 1:
#ifdef RRRA
        case RRTILE8464 + 1:
#endif
            for (j = 0; j < lotaglist; j++)
                if (SLT == lotags[j])
                    break;

            if (j == lotaglist)
            {
                lotags[lotaglist] = SLT;
                lotaglist++;
                if (lotaglist > 64)
                    gameexit("\nToo many switches (64 max).");

                j = headspritestat[3];
                while (j >= 0)
                {
                    if (sprite[j].lotag == 12 && sprite[j].hitag == SLT)
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
                gameexit("\nToo many mirrors (64 max.)");
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
            gameexit("\nToo many 'anim' walls (max 512.)");

        animwall[numanimwalls].tag = 0;
        animwall[numanimwalls].wallnum = 0;

        switch (wal->overpicnum)
        {
        case FANSPRITE:
            wall->cstat |= 65;
            animwall[numanimwalls].wallnum = i;
            numanimwalls++;
            break;
        case BIGFORCE:
            animwall[numanimwalls].wallnum = i;
            numanimwalls++;
            continue;
        }

        wal->extra = -1;

        switch (wal->picnum)
        {
        case WATERTILE2:
            for (j = 0; j < 3; j++)
                if (waloff[wal->picnum + j] == 0)
                    tloadtile(wal->picnum + j);
            break;

        case RRTILE1814:
        case RRTILE1817:
            if (waloff[wal->picnum] == 0)
                tloadtile(wal->picnum);
            break;
        case RRTILE1939:
        case RRTILE1986:
        case RRTILE1987:
        case RRTILE1988:
        case RRTILE2004:
        case RRTILE2005:
        case RRTILE2123:
        case RRTILE2124:
        case RRTILE2125:
        case RRTILE2126:
        case RRTILE2636:
        case RRTILE2637:
        case RRTILE2878:
        case RRTILE2879:
        case RRTILE2898:
        case RRTILE2899:
            if (waloff[wal->picnum] == 0)
                tloadtile(wal->picnum);
            break;
        case TECHLIGHT2:
        case TECHLIGHT4:
            if (waloff[wal->picnum] == 0)
                tloadtile(wal->picnum);
            break;
        case SCREENBREAK6:
        case SCREENBREAK7:
        case SCREENBREAK8:
            if (waloff[SCREENBREAK6] == 0)
                for (j = SCREENBREAK6; j <= SCREENBREAK8; j++)
                    tloadtile(j, pal);
            animwall[numanimwalls].wallnum = i;
            animwall[numanimwalls].tag = -1;
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
    if (!thunderon)
    {
        char brightness = ud.brightness >> 2;
        setbrightness(brightness, palette);
        visibility = p->visibility;
    }
    tilesizx[0] = tilesizy[0] = 0;
}




#if 0
void enterlevel(char g)
{
    short i, j;
    long l;
    char levname[256];

    if ((g & MODE_DEMO) != MODE_DEMO) ud.recstat = ud.m_recstat;
    ud.respawn_monsters = ud.m_respawn_monsters;
    ud.respawn_items = ud.m_respawn_items;
    ud.respawn_inventory = ud.m_respawn_inventory;
    ud.monsters_off = ud.m_monsters_off;
    ud.coop = ud.m_coop;
    ud.marker = ud.m_marker;
    ud.ffire = ud.m_ffire;

    if ((g & MODE_DEMO) == 0 && ud.recstat == 2)
        ud.recstat = 0;

    i = ud.screen_size;
    ud.screen_size = 0;
    dofrontscreens();
    vscrn();
    ud.screen_size = i;

    if (lastlevel)
    {
        if (loadboard("endgame.map", &ps[0].posx, &ps[0].posy, &ps[0].posz, &ps[0].ang, &ps[0].cursectnum) == -1)
        {
            sprintf(tempbuf, "Map %s not found!", boardfilename);
            gameexit(tempbuf);
        }
    }
    else
    {
        if (boardfilename[0] != 0 && ud.m_level_number == 7 && ud.m_volume_number == 0)
        {
            if (loadboard(boardfilename, &ps[0].posx, &ps[0].posy, &ps[0].posz, &ps[0].ang, &ps[0].cursectnum) == -1)
            {
                sprintf(tempbuf, "Map %s not found!", boardfilename);
                gameexit(tempbuf);
            }
        }
        else if (loadboard(level_file_names[(ud.volume_number * 7) + ud.level_number], &ps[0].posx, &ps[0].posy, &ps[0].posz, &ps[0].ang, &ps[0].cursectnum) == -1)
        {
            sprintf(tempbuf, "Map %s not found!", level_file_names[(ud.volume_number * 8) + ud.level_number]);
            gameexit(tempbuf);
        }
    }

}
#endif


void loadlevel(const char *filename)


#ifndef RRRA
    if (ud.volume_number == 1 && ud.level_number == 1)
    {
        short ii;
        for (ii = PISTOL_WEAPON; ii < MAX_WEAPONS; ii++)
            ps[0].gotweapon[ii] = 0;
        for (ii = PISTOL_WEAPON; ii < MAX_WEAPONS; ii++)
            ps[0].ammo_amount[ii] = 0;
    }
#endif

    clearbufbyte(gotpic,sizeof(gotpic),0L);

    prelevel(g);

#ifdef RRRA
    if (ud.level_number == 2 && ud.volume_number == 0)
    {
        short ii;
        for (ii = PISTOL_WEAPON; ii < MAX_WEAPONS; ii++)
            ps[0].gotweapon[ii] = 0;
        for (ii = PISTOL_WEAPON; ii < MAX_WEAPONS; ii++)
            ps[0].ammo_amount[ii] = 0;
        ps[0].gotweapon[RA15_WEAPON] = 1;
        ps[0].ammo_amount[RA15_WEAPON] = 1;
        ps[0].curr_weapon = RA15_WEAPON;
    }
#endif

    allignwarpelevators();
    resetpspritevars(g);

    cachedebug = 0;
    automapping = 0;

    cacheit();
    docacheit();

    if (globalskillsound >= 0)
    {
        while (Sound[globalskillsound].lock >= 200);
    }
    globalskillsound = -1;

    FX_StopAllSounds();
    clearsoundlocks();
    FX_SetReverb(0);

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
      myx = omyx = ps[myconnectindex].posx;
	  myy = omyy = ps[myconnectindex].posy;
	  myz = omyz = ps[myconnectindex].posz;
	  myxvel = myyvel = myzvel = 0;
	  myang = omyang = ps[myconnectindex].ang;
	  myhoriz = omyhoriz = ps[myconnectindex].horiz;
	  myhorizoff = omyhorizoff = ps[myconnectindex].horizoff;
	  mycursectnum = ps[myconnectindex].cursectnum;
	  myjumpingcounter = ps[myconnectindex].jumping_counter;
	  myjumpingtoggle = ps[myconnectindex].jumping_toggle;
      myonground = ps[myconnectindex].on_ground;
      myhardlanding = ps[myconnectindex].hard_landing;
      myreturntocenter = ps[myconnectindex].return_to_center;

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
     displayrooms(screenpeek,65536);
     displayrest(screenpeek);
     nextpage();

     clearbufbyte(playerquitflag,MAXPLAYERS,0x01010101);
     if (waitabort == 1)
         gameexit(" ");
     ps[myconnectindex].over_shoulder_on = 0;

     clearfrags();

     resettimevars();  // Here we go
}
#endif



END_DUKE_NS