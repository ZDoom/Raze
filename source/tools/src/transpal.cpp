// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)

#include "compat.h"
#include "pragmas.h"
#include "colmatch.h"

#define MAXPALOOKUPS 256

static int numshades, transratio;
static char const * const palettefilename = "palette.dat";
static uint8_t origpalookup[MAXPALOOKUPS<<8], palookup[MAXPALOOKUPS<<8], transluc[65536];
static uint8_t origpalette[768], palette[768];

static char getshade(char dashade, char dacol)
{
    int r, g, b, t;
    char *ptr;

    ptr = (char *)&palette[dacol*3];
    t = divscale16(numshades-dashade,numshades);
    r = ((ptr[0]*t+32768)>>16);
    g = ((ptr[1]*t+32768)>>16);
    b = ((ptr[2]*t+32768)>>16);
    return(paletteGetClosestColor(r,g,b));
}

static char gettrans(char dat1, char dat2, int datransratio)
{
    int r, g, b;
    char *ptr, *ptr2;

    ptr = (char *)&palette[dat1*3];
    ptr2 = (char *)&palette[dat2*3];
    r = ptr[0]; r += (((ptr2[0]-r)*datransratio+128)>>8);
    g = ptr[1]; g += (((ptr2[1]-g)*datransratio+128)>>8);
    b = ptr[2]; b += (((ptr2[2]-b)*datransratio+128)>>8);
    return(paletteGetClosestColor(r,g,b));
}

int main(int argc, char **argv)
{
    char col, ch;
    short orignumshades;
    int fil, i, j, rscale, gscale, bscale;

    ch = 13;
    if (argc>1) {
        if (argv[1][0] == '-') {
            if (argv[1][1] == 't') { ch = 32; puts("Updating translucency table ONLY"); }
            argc--;
            argv++;
        }
    }

    if ((argc != 3) && (argc != 6))
    {
        Bprintf("TRANSPAL [-t]<numshades><trans#(0-inv,256-opa)><r><g><b>  by Kenneth Silverman\n");
        Bprintf("   Ex #1: transpal 32 170 30 59 11      (I use these values in my BUILD demo)\n");
        Bprintf("                          \xc0\xc4\xc4\xc1\xc4\xc4\xc1\xc4\xc4\xc4 The RGB scales are optional\n");
        Bprintf("   Ex #2: transpal 64 160\n\n");
        Bprintf("Once tables are generated, the optional -t switch determines what to save:\n");
        Bprintf("   Exclude -t to update both the shade table and transluscent table\n");
        Bprintf("   Include -t to update the transluscent table ONLY\n");
        exit(0);
    }

    numshades = Batol(argv[1]);
    transratio = Batol(argv[2]);

    if (argc == 6)
    {
        rscale = Batol(argv[3]);
        gscale = Batol(argv[4]);
        bscale = Batol(argv[5]);
    }
    else
    {
        rscale = 30;
        gscale = 59;
        bscale = 11;
    }

    if ((numshades < 1) || (numshades > 256))
        { Bprintf("Invalid number of shades\n"); exit(0); }
    if ((transratio < 0) || (transratio > 256))
        { Bprintf("Invalid transluscent ratio\n"); exit(0); }

    if ((fil = Bopen(palettefilename,BO_BINARY|BO_RDONLY,BS_IREAD)) == -1)
    {
        Bprintf("%s not found",palettefilename);
        return(0);
    }
    Bread(fil,origpalette,768);
    Bread(fil,&orignumshades,2); orignumshades = B_LITTLE16(orignumshades);
    orignumshades = min(max<int>(orignumshades,1),256);
    Bread(fil,origpalookup,(int)orignumshades<<8);
    Bclose(fil);

    for (int k = 0; k < 768; k++)
        palette[k] = origpalette[k] << 2;

    initdivtables();
    initfastcolorlookup_scale(rscale,gscale,bscale);
    initfastcolorlookup_palette(palette);
    initfastcolorlookup_gridvectors();

    for(i=0;i<numshades;i++)
        for(j=0;j<256;j++)
        {
            col = getshade((char)i,(char)j);
            palookup[(i<<8)+j] = col;
        }

    for(i=0;i<256;i++)
        for(j=0;j<256;j++)
        {
            col = gettrans((char)i,(char)j,transratio);
            transluc[(i<<8)+j] = col;
        }

    if (ch == 13)
    {
        short s;
        if ((fil = Bopen(palettefilename,BO_BINARY|BO_TRUNC|BO_CREAT|BO_WRONLY,BS_IREAD|BS_IWRITE)) == -1)
            { Bprintf("Couldn't save file %s",palettefilename); return(0); }
        Bwrite(fil,origpalette,768);
        s = B_LITTLE16(numshades); Bwrite(fil,&s,2);
        Bwrite(fil,palookup,numshades<<8);
        Bwrite(fil,transluc,65536);
        Bclose(fil);
        Bprintf("Shade table AND transluscent table updated\n");
    }
    else if (ch == 32)
    {
        short s;
        if ((fil = Bopen(palettefilename,BO_BINARY|BO_TRUNC|BO_CREAT|BO_WRONLY,BS_IREAD|BS_IWRITE)) == -1)
            { Bprintf("Couldn't save file %s",palettefilename); return(0); }
        Bwrite(fil,origpalette,768);
        s = B_LITTLE16(orignumshades); Bwrite(fil,&s,2);
        Bwrite(fil,origpalookup,(int)orignumshades<<8);
        Bwrite(fil,transluc,65536);
        Bclose(fil);
        Bprintf("Transluscent table updated\n");
    }
    else
        Bprintf("Palette file wasn't touched\n");

    return 0;
}
