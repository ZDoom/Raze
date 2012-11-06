// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)

#include "compat.h"
#include "pragmas.h"

#define MAXPALOOKUPS 256

static int numpalookups, transratio;
static char palettefilename[13], origpalookup[MAXPALOOKUPS<<8];
static char palette[768], palookup[MAXPALOOKUPS<<8], transluc[65536];
static char closestcol[64][64][64];

#define FASTPALGRIDSIZ 8
static int rdist[129], gdist[129], bdist[129];
static char colhere[((FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2))>>3];
static char colhead[(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)];
static int colnext[256];
static char coldist[8] = {0,1,2,3,4,3,2,1};
static int colscan[27];



char getclosestcol(int r, int g, int b)
{
    int i, j, k, dist, mindist, retcol;
    int *rlookup, *glookup, *blookup;
    char *ptr;

    if (closestcol[r][g][b] != 255) return(closestcol[r][g][b]);

    j = (r>>3)*FASTPALGRIDSIZ*FASTPALGRIDSIZ+(g>>3)*FASTPALGRIDSIZ+(b>>3)+FASTPALGRIDSIZ*FASTPALGRIDSIZ+FASTPALGRIDSIZ+1;
    mindist = min(rdist[coldist[r&7]+64+8],gdist[coldist[g&7]+64+8]);
    mindist = min(mindist,bdist[coldist[b&7]+64+8]);
    mindist++;

    rlookup = (int *)&rdist[64-r];
    glookup = (int *)&gdist[64-g];
    blookup = (int *)&bdist[64-b];

    retcol = -1;
    for(k=26;k>=0;k--)
    {
        i = colscan[k]+j; if ((colhere[i>>3]&(1<<(i&7))) == 0) continue;
        for(i=colhead[i];i>=0;i=colnext[i])
        {
            ptr = (char *)&palette[i*3];
            dist = glookup[ptr[1]]; if (dist >= mindist) continue;
            dist += rlookup[ptr[0]]; if (dist >= mindist) continue;
            dist += blookup[ptr[2]]; if (dist >= mindist) continue;
            mindist = dist; retcol = i;
        }
    }
    if (retcol < 0)
    {
        mindist = 0x7fffffff;
        ptr = (char *)&palette[768-3];
        for(i=255;i>=0;i--,ptr-=3)
        {
            dist = glookup[ptr[1]]; if (dist >= mindist) continue;
            dist += rlookup[ptr[0]]; if (dist >= mindist) continue;
            dist += blookup[ptr[2]]; if (dist >= mindist) continue;
            mindist = dist; retcol = i;
        }
    }
    ptr = (char *)&closestcol[r][g][b];
    *ptr = retcol;
    if ((r >= 4) && (ptr[(-2)<<12] == retcol)) ptr[(-3)<<12] = retcol, ptr[(-2)<<12] = retcol, ptr[(-1)<<12] = retcol;
    if ((g >= 4) && (ptr[(-2)<<6] == retcol)) ptr[(-3)<<6] = retcol, ptr[(-2)<<6] = retcol, ptr[(-1)<<6] = retcol;
    if ((b >= 4) && (ptr[(-2)] == retcol)) ptr[(-3)] = retcol, ptr[(-2)] = retcol, ptr[(-1)] = retcol;
    if ((r < 64-4) && (ptr[(2)<<12] == retcol)) ptr[(3)<<12] = retcol, ptr[(2)<<12] = retcol, ptr[(1)<<12] = retcol;
    if ((g < 64-4) && (ptr[(2)<<6] == retcol)) ptr[(3)<<6] = retcol, ptr[(2)<<6] = retcol, ptr[(1)<<6] = retcol;
    if ((b < 64-4) && (ptr[(2)] == retcol)) ptr[(3)] = retcol, ptr[(2)] = retcol, ptr[(1)] = retcol;
    if ((r >= 2) && (ptr[(-1)<<12] == retcol)) ptr[(-1)<<12] = retcol;
    if ((g >= 2) && (ptr[(-1)<<6] == retcol)) ptr[(-1)<<6] = retcol;
    if ((b >= 2) && (ptr[(-1)] == retcol)) ptr[(-1)] = retcol;
    if ((r < 64-2) && (ptr[(1)<<12] == retcol)) ptr[(1)<<12] = retcol;
    if ((g < 64-2) && (ptr[(1)<<6] == retcol)) ptr[(1)<<6] = retcol;
    if ((b < 64-2) && (ptr[(1)] == retcol)) ptr[(1)] = retcol;
    return(retcol);
}

char getpalookup(char dashade, char dacol)
{
    int r, g, b, t;
    char *ptr;

    ptr = (char *)&palette[dacol*3];
    t = divscale16(numpalookups-dashade,numpalookups);
    r = ((ptr[0]*t+32768)>>16);
    g = ((ptr[1]*t+32768)>>16);
    b = ((ptr[2]*t+32768)>>16);
    return(getclosestcol(r,g,b));
}

char gettrans(char dat1, char dat2, int datransratio)
{
    int r, g, b;
    char *ptr, *ptr2;

    ptr = (char *)&palette[dat1*3];
    ptr2 = (char *)&palette[dat2*3];
    r = ptr[0]; r += (((ptr2[0]-r)*datransratio+128)>>8);
    g = ptr[1]; g += (((ptr2[1]-g)*datransratio+128)>>8);
    b = ptr[2]; b += (((ptr2[2]-b)*datransratio+128)>>8);
    return(getclosestcol(r,g,b));
}

void initfastcolorlookup(int rscale, int gscale, int bscale)
{
    int i, j, x, y, z;
    char *ptr;

    j = 0;
    for(i=64;i>=0;i--)
    {
        //j = (i-64)*(i-64);
        rdist[i] = rdist[128-i] = j*rscale;
        gdist[i] = gdist[128-i] = j*gscale;
        bdist[i] = bdist[128-i] = j*bscale;
        j += 129-(i<<1);
    }

    Bmemset(colhere, 0, sizeof(colhere));
    Bmemset(colhead, 0, sizeof(colhead));

    ptr = (char *)&palette[768-3];
    for(i=255;i>=0;i--,ptr-=3)
    {
        j = (ptr[0]>>3)*FASTPALGRIDSIZ*FASTPALGRIDSIZ+(ptr[1]>>3)*FASTPALGRIDSIZ+(ptr[2]>>3)+FASTPALGRIDSIZ*FASTPALGRIDSIZ+FASTPALGRIDSIZ+1;
        if (colhere[j>>3]&(1<<(j&7))) colnext[i] = colhead[j]; else colnext[i] = -1;
        colhead[j] = i;
        colhere[j>>3] |= (1<<(j&7));
    }

    i = 0;
    for(x=-FASTPALGRIDSIZ*FASTPALGRIDSIZ;x<=FASTPALGRIDSIZ*FASTPALGRIDSIZ;x+=FASTPALGRIDSIZ*FASTPALGRIDSIZ)
        for(y=-FASTPALGRIDSIZ;y<=FASTPALGRIDSIZ;y+=FASTPALGRIDSIZ)
            for(z=-1;z<=1;z++)
                colscan[i++] = x+y+z;
    i = colscan[13]; colscan[13] = colscan[26]; colscan[26] = i;
}

int main(int argc, char **argv)
{
    char col, ch;
    short orignumpalookups;
    int fil, i, j, rscale, gscale, bscale;
    char buf[65536];

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

    Bstrcpy(palettefilename,"palette.dat");
    numpalookups = Batol(argv[1]);
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

    if ((numpalookups < 1) || (numpalookups > 256))
        { Bprintf("Invalid number of shades\n"); exit(0); }
    if ((transratio < 0) || (transratio > 256))
        { Bprintf("Invalid transluscent ratio\n"); exit(0); }

    if ((fil = Bopen(palettefilename,BO_BINARY|BO_RDONLY,BS_IREAD)) == -1)
    {
        Bprintf("%s not found",palettefilename);
        return(0);
    }
    Bread(fil,palette,768);
    Bread(fil,&orignumpalookups,2); orignumpalookups = B_LITTLE16(orignumpalookups);
    orignumpalookups = min(max(orignumpalookups,1),256);
    Bread(fil,origpalookup,(int)orignumpalookups<<8);
    Bclose(fil);

    clearbuf(buf,65536>>2,0L);

    initfastcolorlookup(rscale,gscale,bscale);
    clearbuf(closestcol,262144>>2,0xffffffff);

    for(i=0;i<numpalookups;i++)
        for(j=0;j<256;j++)
        {
            col = getpalookup((char)i,(char)j);
            palookup[(i<<8)+j] = col;

            drawpixel(((((i<<1)+0)*320+(j+8))>>2)+buf,(int)col);
            drawpixel(((((i<<1)+1)*320+(j+8))>>2)+buf,(int)col);
        }

    for(i=0;i<256;i++)
        for(j=0;j<6;j++)
        {
            drawpixel((((j+132+0)*320+(i+8))>>2)+buf,i);

            drawpixel((((i+132+8)*320+(j+0))>>2)+buf,i);
        }

    for(i=0;i<256;i++)
        for(j=0;j<256;j++)
        {
            col = gettrans((char)i,(char)j,transratio);
            transluc[(i<<8)+j] = col;

            drawpixel((((j+132+8)*320+(i+8))>>2)+buf,(int)col);
        }

    if (ch == 13)
    {
        short s;
        if ((fil = Bopen(palettefilename,BO_BINARY|BO_TRUNC|BO_CREAT|BO_WRONLY,BS_IREAD|BS_IWRITE)) == -1)
            { Bprintf("Couldn't save file %s",palettefilename); return(0); }
        Bwrite(fil,palette,768);
        s = B_LITTLE16(numpalookups); Bwrite(fil,&s,2);
        Bwrite(fil,palookup,numpalookups<<8);
        Bwrite(fil,transluc,65536);
        Bclose(fil);
        Bprintf("Shade table AND transluscent table updated\n");
    }
    else if (ch == 32)
    {
        short s;
        if ((fil = Bopen(palettefilename,BO_BINARY|BO_TRUNC|BO_CREAT|BO_WRONLY,BS_IREAD|BS_IWRITE)) == -1)
            { Bprintf("Couldn't save file %s",palettefilename); return(0); }
        Bwrite(fil,palette,768);
        s = B_LITTLE16(orignumpalookups); Bwrite(fil,&s,2);
        Bwrite(fil,origpalookup,(int)orignumpalookups<<8);
        Bwrite(fil,transluc,65536);
        Bclose(fil);
        Bprintf("Transluscent table updated\n");
    }
    else
        Bprintf("Palette file wasn't touched\n");

    return 0;
}

