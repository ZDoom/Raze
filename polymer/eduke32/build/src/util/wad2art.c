// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)

#include "compat.h"
#include "pragmas.h"

#define MAXWADS 4096

static int artversion, localtilestart, localtileend;
static int fil1, fil2;
static char wadata[MAXWADS][8];
static int wadplc[MAXWADS], wadlen[MAXWADS], numwads;
static int xoffses[1024], ylookup[256], picanm[MAXWADS];
static short tilesizx[MAXWADS], tilesizy[MAXWADS];
static char pal[768], palookup[8192];
static char screen[65536], tempbuf[131072];


void loadwadheader(void)
{
    int i, j;

    Bread(fil1,&tempbuf[0],12);
    numwads = ((int)tempbuf[4])+(((int)tempbuf[5])<<8)+(((int)tempbuf[6])<<16)+(((int)tempbuf[7])<<24);
    i = ((int)tempbuf[8])+(((int)tempbuf[9])<<8)+(((int)tempbuf[10])<<16)+(((int)tempbuf[11])<<24);
    Blseek(fil1,i,BSEEK_SET);
    Bread(fil1,&tempbuf[0],numwads*16);
    j = 0;
    for(i=0;i<numwads;i++)
    {
        wadplc[i] = ((int)tempbuf[j])+(((int)tempbuf[j+1])<<8)+(((int)tempbuf[j+2])<<16)+(((int)tempbuf[j+3])<<24);
        j += 4;
        wadlen[i] = ((int)tempbuf[j])+(((int)tempbuf[j+1])<<8)+(((int)tempbuf[j+2])<<16)+(((int)tempbuf[j+3])<<24);
        j += 4;
        wadata[i][0] = tempbuf[j+0]; wadata[i][1] = tempbuf[j+1];
        wadata[i][2] = tempbuf[j+2]; wadata[i][3] = tempbuf[j+3];
        wadata[i][4] = tempbuf[j+4]; wadata[i][5] = tempbuf[j+5];
        wadata[i][6] = tempbuf[j+6]; wadata[i][7] = tempbuf[j+7];
        j += 8;
    }
}

void convpalette(void)
{
    int i, fil3;
    short danumshades;

    i = 0;
    while (Bstrncasecmp(wadata[i],"PLAYPAL",7) != 0) i++;
    Blseek(fil1,wadplc[i],BSEEK_SET);
    Bread(fil1,pal,768);
    for(i=0;i<768;i++) pal[i] >>= 2;

    i = 0;
    while (Bstrncasecmp(wadata[i],"COLORMAP",8) != 0) i++;
    Blseek(fil1,wadplc[i],BSEEK_SET);
    Bread(fil1,palookup,8192);

    if ((fil3 = Bopen("palette.dat",BO_BINARY|BO_TRUNC|BO_CREAT|BO_WRONLY,BS_IREAD|BS_IWRITE)) == -1)
        { Bprintf("Cannot save palette.dat\n"); exit(0); }
    Bwrite(fil3,pal,768);
    danumshades = 32;
    Bwrite(fil3,&danumshades,2);
    Bwrite(fil3,palookup,8192);
    Bclose(fil3);
}

void saveart (short tilenum, short xlen, short ylen)
{
    int i, x, p, pend;

    pend = ylookup[ylen];

    tilesizx[tilenum] = xlen;
    tilesizy[tilenum] = ylen;
    i = 0;
    for(x=0;x<xlen;x++)
        for(p=x;p<pend;p+=320)
            tempbuf[i++] = screen[p];
    if (Bwrite(fil2,&tempbuf[0],i) < 0)
    {
        Bprintf("NOT ENOUGH DISK SPACE!\n");
        exit(0);
    }
}

void savenames(void)
{
    BFILE * fil3;
    int i;

    fil3 = Bfopen("names.h","w");

    if (fil3 != NULL)
    {
        Bfprintf(fil3,"//Be careful when changing this file - it is parsed by Editart and Build.\n");

        for(i=0; i<numwads; i++)
            if (wadata[i][0] != 0)
                Bfprintf(fil3,"#define %s %d\n", wadata[i], i);

        Bfclose(fil3);
    }
}

void showart (char const * part)
{
    char yoff;
    short xsiz, ysiz;
    int i, z, zx, x, p, pend, curplc;

    curplc = -1;
    if ((Bstrncasecmp(part,"L_START",7) == 0) || (Bstrncasecmp(part,"S_START",7) == 0) || (Bstrncasecmp(part,"P_START",7) == 0))
    {
        if (Bstrncasecmp(part,"L_START",7) == 0)
            z = 462;
        else
        {
            z = 0;
            while (Bstrncasecmp(wadata[z],part,7) != 0) z++;
            z++;
        }

        do
        {
            if (Bstrncasecmp(wadata[z],"P1_START",8) == 0) z++;
            if (Bstrncasecmp(wadata[z],"P1_END",6) == 0) z++;
            if (Bstrncasecmp(wadata[z],"P2_START",8) == 0) z++;
            if (Bstrncasecmp(wadata[z],"S_START",7) == 0) break;
            if (Bstrncasecmp(wadata[z],"S_END",5) == 0) break;
            if (Bstrncasecmp(wadata[z],"P_END",5) == 0) break;

            if (curplc != wadplc[z]) Blseek(fil1,wadplc[z],BSEEK_SET);
            read(fil1,&tempbuf[0],wadlen[z]);
            curplc = wadplc[z]+wadlen[z];

            xsiz = (int)tempbuf[0]+(((int)tempbuf[1])<<8);
            ysiz = (int)tempbuf[2]+(((int)tempbuf[3])<<8);
            if ((xsiz <= 0) || (ysiz <= 0) || (xsiz > 320) || (ysiz > 200)) goto skipit;
            i = 8;
            for(zx=0;zx<xsiz;zx++)
            {
                xoffses[zx] = ((int)tempbuf[i])+(((int)tempbuf[i+1])<<8)+(((int)tempbuf[i+2])<<16)+(((int)tempbuf[i+3])<<24);
                i += 4;
            }

            clearbuf(screen,ylookup[ysiz]>>2,0xffffffff);

            for(x=0;x<xsiz;x++)
            {
                i = xoffses[x];
                yoff = tempbuf[i++];
                while (yoff != 255)
                {
                    p = ylookup[yoff]+x;
                    pend = p+ylookup[tempbuf[i]];
                    i += 2;
                    for(;p<pend;p+=320)
                        screen[p] = tempbuf[i++];
                    i++;
                    yoff = tempbuf[i++];
                }
            }

            saveart(z,xsiz,ysiz);
skipit:
            z++;
        } while (z < numwads);
    }
    else
    {
        z = 0;
        while (Bstrncasecmp(wadata[z],part,7) != 0) z++;
        z++;

        while (1)
        {
            if (Bstrncasecmp(wadata[z],"F1_START",8) == 0) z++;
            if (Bstrncasecmp(wadata[z],"F1_END",6) == 0) z++;
            if (Bstrncasecmp(wadata[z],"F_END",5) == 0) break;

            if (wadlen[z] == 4096)
            {
                if (curplc != wadplc[z]) Blseek(fil1,wadplc[z],BSEEK_SET);
                Bread(fil1,&tempbuf[0],4096);
                curplc = wadplc[z]+4096;
                i = 0;
                for(x=0;x<64;x++)
                    for(p=x;p<320*64;p+=320)
                        screen[p] = tempbuf[i++];

                saveart(z,64,64);
            }

            z++;
        }
    }
}

int main(int argc, char **argv)
{
    int i, j, endoffile;
    char wadfile[80];

    Bprintf("Wad2Art!                                       Copyright 1995 by Ken Silverman\n");

    if (argc != 2)
    {
        Bprintf("Command line parameters: Wad2Art <Doom IWAD file>\n");
        Bprintf("   Creates TILES000.ART, PALETTE.DAT, and NAMES.H in current directory.\n");
        Bprintf("   Ex: wad2art c:\\doom\\doom.wad\n");
        exit(0);
    }

    strcpy(wadfile,argv[1]);
    if (strchr(wadfile,'.') == 0) strcat(wadfile,".wad");
    if ((fil1 = Bopen(wadfile,BO_BINARY|BO_RDONLY,BS_IREAD)) == -1)
        { Bprintf("Wad not found\n"); exit(0); }
    if ((fil2 = Bopen("tiles000.art",BO_BINARY|BO_TRUNC|BO_CREAT|BO_WRONLY,BS_IREAD|BS_IWRITE)) == -1)
        { Bprintf("Can't open art file\n"); exit(0); }

    j = 0;
    for(i=0;i<256;i++) { ylookup[i] = j; j += 320; }

    Bprintf("Loading wad header...\n");
    loadwadheader();
    Blseek(fil2,16+(numwads<<3),SEEK_SET);
    for(i=0;i<numwads;i++)
        { tilesizx[i] = 0; tilesizy[i] = 0; picanm[i] = 0L; }

    Bprintf("Saving names.h\n");
    savenames();
    Bprintf("Converting palette\n");
    convpalette();

    Bprintf("Saving tiles000.art\n");
    showart("L_START");
    showart("S_START");
    showart("P_START");
    showart("F_START");

    Bprintf("Saving tiles000.art header\n");
    artversion = 1; localtilestart = 0; localtileend = numwads-1;

    endoffile = Btell(fil2);
    Blseek(fil2,0,BSEEK_SET);
    Bwrite(fil2,&artversion,4);
    Bwrite(fil2,&numwads,4);
    Bwrite(fil2,&localtilestart,4);
    Bwrite(fil2,&localtileend,4);
    Bwrite(fil2,&tilesizx[0],numwads<<1);
    Bwrite(fil2,&tilesizy[0],numwads<<1);
    Bwrite(fil2,&picanm[0],numwads<<2);
    Blseek(fil2,endoffile,BSEEK_SET);

    Bclose(fil2);
    Bclose(fil1);

    Bprintf("Congratulations!  Your disk actually had enough space this time!\n");

    return 0;
}

