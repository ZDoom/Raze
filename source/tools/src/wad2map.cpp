// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)

#include "compat.h"
#include "pragmas.h"

#define MAXWADS 4096
#define MAXPOINTS 8192
#define MAXLINES 8192
#define MAXSIDES 8192
#define MAXSECTS 2048
#define MAXTHINGS 4096
#define MAXTEXTS 1024
#define MAXPNAMES 1024
#define MAXTHINGTYPES 3072

#define MAXSECTORS 1024
#define MAXWALLS 8192
#define MAXSPRITES 4096

static unsigned short sqrtable[4096], shlookup[4096+256];

static char iwadata[MAXWADS][9];
static int inumwads, iwadstart, iwadplc[MAXWADS], iwadlen[MAXWADS];

static char pwadata[MAXWADS][9];
static int pnumwads, pwadstart, pwadplc[MAXWADS], pwadlen[MAXWADS];

static short px[MAXPOINTS], py[MAXPOINTS];

typedef struct
{
    short p1, p2, flags, special, tag;
    short side1, side2;   //If side2 = -1, no left
} linedeftype;
static linedeftype line[MAXLINES];

typedef struct
{
    short xoffset, yoffset;
    char uppertexture[8], lowertexture[8], middletexture[8];
    short sect;
} sidedeftype;
static sidedeftype side[MAXSIDES];
static short sidetoppic[MAXSIDES], sidebotpic[MAXSIDES], sidemidpic[MAXSIDES];

typedef struct
{
    short floorz, ceilingz;
    char floorpic[8], ceilingpic[8];
    short shade, type, tag;
} secttype;
static secttype sect[MAXSECTS];
static short sectspri[MAXSECTS][8];

typedef struct
{
    short x, y, ang, type, options;
} thingtype;
static thingtype thing[MAXTHINGS];

static char textname[MAXTEXTS][9];
static int textoffs[MAXTEXTS];
static short textpname[MAXTEXTS];

static char pname[MAXPNAMES][9];

    //Build map-linking variables
static short picindex[MAXPOINTS], linindex[MAXPOINTS];
static short wx[MAXPOINTS], wy[MAXPOINTS], wx2[MAXPOINTS], wy2[MAXPOINTS];
static short point2[MAXPOINTS], slist[MAXPOINTS], sectorofwall[MAXPOINTS];

typedef struct
{
    short wallptr, wallnum;
    int ceilingz, floorz;
    short ceilingstat, floorstat;
    short ceilingpicnum, ceilingheinum;
    signed char ceilingshade;
    char ceilingpal, ceilingxpanning, ceilingypanning;
    short floorpicnum, floorheinum;
    signed char floorshade;
    char floorpal, floorxpanning, floorypanning;
    char visibility, filler;
    short lotag, hitag, extra;
} sectortype;

typedef struct
{
    int x, y;
    short point2, nextwall, nextsector, cstat;
    short picnum, overpicnum;
    signed char shade;
    char pal, xrepeat, yrepeat, xpanning, ypanning;
    short lotag, hitag, extra;
} walltype;

typedef struct
{
    int x, y, z;
    short cstat, picnum;
    signed char shade;
    char pal, clipdist, filler;
    unsigned char xrepeat, yrepeat;
    signed char xoffset, yoffset;
    short sectnum, statnum;
    short ang, owner, xvel, yvel, zvel;
    short lotag, hitag, extra;
} spritetype;

static sectortype sector[MAXSECTORS];
static walltype wall[MAXWALLS];
static spritetype sprite[MAXSPRITES];

static char tempbuf[16384];
static short tempshort[4096];


    //SCRIPT parsing variables:
static char scriptname[16];
static char filebuf[16384];
static int filhandle, filpos, fileng;

static char definemode = 0, define[16384], *defineptr[4096];
static int defineval[16384], definecnt = 0, numdefines = 0;

static char thingtypemode = 0;
static int thingnum[1024], thingnum2[1024], thingoff[1024], numthings = 0;
static short thingfield[4096];
static char thingop[4096];
static int thingval[4096], thingopnum = 0;

static char texturelookupmode = 0;
static short texturelookup[4096];

static char tagtypemode = 0;
static int tagnum[1024], tagnum2[1024], tagoff[1024], numtags = 0;
static char tagop[4096];
static short tagfield[4096];
static int tagval[4096], tagopnum = 0;

static char sectypemode = 0;
static int secnum[1024], secnum2[1024], secoff[1024], numsecs = 0;
static short secfield[4096];
static char secop[4096];
static int secval[4096], secopnum = 0;

#define THINGLISTNUM 123
static short thinglookup[MAXTHINGTYPES];
typedef struct { short num; char name[9]; } thinglisttype;
static thinglisttype thinglist[THINGLISTNUM] =
{{1,"PLAYA1"},{2,"PLAYA1"},{3,"PLAYA1"},{4,"PLAYA1"},{11,"PLAYA1"},{14,""},{3004,"POSSA1"},
{84,"SSWVA1"},{9,"SPOSA1"},{65,"CPOSA1"},{3001,"TROOA1"},{3002,"SARGA1"},{58,"SARGA1"},
{3006,"SKULA1"},{3005,"HEADA1"},{69,"BOS2A1C1"},{3003,"BOSSA1"},{68,"BSPIA1D1"},
{71,"PAINA1"},{66,"SKELA1D1"},{67,"FATTA1"},{64,"VILEA1D1"},{7,"SPIDA1D1"},{16,"CYBRA1"},
{88,""},{89,""},{87,""},{2005,"CSAWA0"},{2001,"SHOTA0"},{82,"SGN2A0"},{2002,"MGUNA0"},
{2003,"LAUNA0"},{2004,"PLASA0"},{2006,"BFUGA0"},{2007,"CLIPA0"},{2008,"SHELA0"},
{2010,"ROCKA0"},{2047,"CELLA0"},{2048,"AMMOA0"},{2049,"SBOXA0"},{2046,"BROKA0"},
{17,"CELPA0"},{8,"BPAKA0"},{2011,"STIMA0"},{2012,"MEDIA0"},{2014,"BON1A0"},
{2015,"BON2A0"},{2018,"ARM1A0"},{2019,"ARM2A0"},{83,"MEGAA0"},{2013,"SOULA0"},
{2022,"PINVA0"},{2023,"PSTRA0"},{2024,"PINSA0"},{2025,"SUITA0"},{2026,"PMAPA0"},
{2045,"PVISA0"},{5,"BKEYA0"},{40,"BSKUA0"},{13,"RKEYA0"},{38,"RSKUA0"},{6,"YKEYA0"},
{39,"YSKUA0"},{2035,"BAR1A0"},{72,"KEENA0"},{48,"ELECA0"},{30,"COL1A0"},{32,"COL3A0"},
{31,"COL2A0"},{36,"COL5A0"},{33,"COL4A0"},{37,"COL6A0"},{47,"SMITA0"},{43,"TRE1A0"},
{54,"TRE2A0"},{2028,"COLUA0"},{85,"TLMPA0"},{86,"TLP2A0"},{34,"CANDA0"},{35,"CBRAA0"},
{44,"TBLUA0"},{45,"TGRNA0"},{46,"TREDA0"},{55,"SMBTA0"},{56,"SMGTA0"},{57,"SMRTA0"},
{70,"FCANA0"},{41,"CEYEA0"},{42,"FSKUA0"},{49,"GOR1A0"},{63,"GOR1A0"},{50,"GOR2A0"},
{59,"GOR2A0"},{52,"GOR4A0"},{60,"GOR4A0"},{51,"GOR5A0"},{61,"GOR5A0"},{53,"HDB1A0"},
{62,"HDB2A0"},{73,"GOR3A0"},{74,"GOR3A0"},{75,"HDB3A0"},{76,"HDB4A0"},{77,"HDB5A0"},
{78,"HDB6A0"},{25,"POL1A0"},{26,"BBRNA0"},{27,"POL4A0"},{28,"POL2A0"},{29,"POL3A0"},
{10,"PLAYW0"},{12,"PLAYW0"},{24,"POB1A0"},{79,"POB2A0"},{80,"POB2A0"},{81,"BRS1A0"},
{15,"PLAYN0"},{18,"POSSL0"},{19,"SPOSL0"},{20,"TROOM0"},{21,"SARGN0"},{22,"HEADL0"},
{23,""}};

// These three were nicked from Build's engine.c.
static inline unsigned int ksqrtasm(unsigned int a)
{
    unsigned short c;

    if (a & 0xff000000) c = shlookup[(a >> 24) + 4096];
    else c = shlookup[a >> 12];
    a >>= c&0xff;
    a = (a&0xffff0000)|(sqrtable[a]);
    a >>= ((c&0xff00) >> 8);

    return a;
}

static inline int msqrtasm(unsigned int c)
{
    unsigned int a,b;

    a = 0x40000000l;
    b = 0x20000000l;
    do {
        if (c >= a) {
            c -= a;
            a += b*4;
        }
        a -= b;
        a >>= 1;
        b >>= 2;
    } while (b);
    if (c >= a) a++;
    a >>= 1;
    return a;
}

static void initksqrt(void)
{
    int i, j, k;

    j = 1; k = 0;
    for(i=0;i<4096;i++)
    {
        if (i >= j) { j <<= 2; k++; }
        sqrtable[i] = (unsigned short)(msqrtasm((i<<18)+131072)<<1);
        shlookup[i] = (k<<1)+((10-k)<<8);
        if (i < 256) shlookup[i+4096] = ((k+6)<<1)+((10-(k+6))<<8);
    }
}


int inside(int x, int y, short sectnum)
{
    walltype *wal;
    int i, x1, y1, x2, y2;
    char cnt;

    cnt = 0;

    wal = &wall[sector[sectnum].wallptr];
    for(i=sector[sectnum].wallnum;i>0;i--)
    {
        y1 = wal->y-y; y2 = wall[wal->point2].y-y;
        if ((y1^y2) < 0)
        {
            x1 = wal->x-x; x2 = wall[wal->point2].x-x;

            if ((x1^x2) < 0)
                cnt ^= (x1*y2<x2*y1)^(y1<y2);
            else if (x1 >= 0)
                cnt ^= 1;
        }
        wal++;
    }
    return(cnt);
}

int readbyte(void)
{
    if (filpos >= fileng) return(-1);
    if ((filpos&16383) == 0) Bread(filhandle,filebuf,16384);
    filpos++;
    return((int)filebuf[(filpos-1)&16383]);
}

int readline(void)
{
    int i, ch;

    do
    {
        do
        {
            ch = readbyte();
            if (ch < 0) return(0);
        } while ((ch == 13) || (ch == 10));

        i = 0; tempbuf[0] = 0;
        while ((ch != 13) && (ch != 10))
        {
            if (ch < 0) return(0);
            if (ch == ';')
            {
                do
                {
                    if (ch < 0) return(0);
                    ch = readbyte();
                } while ((ch != 13) && (ch != 10));
                break;
            }
            if ((ch == 32) || (ch == 9)) ch = ',';
            if ((ch != ',') || (i == 0) || (tempbuf[i-1] != ','))
                { tempbuf[i++] = ch; tempbuf[i] = 0; }
            ch = readbyte();
        }
        if ((i > 0) && (tempbuf[i-1] == ',')) tempbuf[i-1] = 0;
    } while (i <= 0);
    return(i);
}

void parsescript(void)
{
    int i, j, k, l, lasti, breakout, tstart, tend, textnum = 0, frontbackstat;
    int spritenumstat, slen;
    char ch;

    Bmemset(sectspri, 0xffffffff, MAXSECTS*8*sizeof(short));

    if (scriptname[0] == 0)
    {
        for(i=0;i<4096;i++) texturelookup[i] = i;
        return;
    }

    if ((filhandle = Bopen(scriptname,BO_BINARY|BO_RDONLY,BS_IREAD)) == -1)
    {
        Bprintf("Could not find %s\n",scriptname);
        exit(0);
    }
    filpos = 0; fileng = Bfilelength(filhandle);
    while (readline() != 0)
    {
        i = 0; j = 0; lasti = 0;
        while (1)
        {
            if ((tempbuf[i] == ',') || (tempbuf[i] == 0))
            {
                if (tempbuf[i] == 0) { breakout = 1; }
                else { breakout = 0, tempbuf[i] = 0; }

                if (j == 0)
                {
                    if (tempbuf[lasti] == '[')
                    {
                        definemode = 0;
                        thingtypemode = 0;
                        texturelookupmode = 0;
                        tagtypemode = 0;
                        sectypemode = 0;
                    }

                    if (Bstrcasecmp(&tempbuf[lasti],"#define") == 0)
                        definemode = 1;

                    if (thingtypemode == 1)
                    {
                        thingoff[numthings] = thingopnum;

                        k = lasti;
                        while ((tempbuf[k] != 0) && (tempbuf[k] != '-')) k++;

                        if (tempbuf[k] == '-')
                        {
                            tempbuf[k] = 0;
                            thingnum[numthings] = Batol(&tempbuf[lasti]);
                            thingnum2[numthings] = Batol(&tempbuf[k+1]);
                        }
                        else
                        {
                            thingnum[numthings] = Batol(&tempbuf[lasti]);
                            thingnum2[numthings] = thingnum[numthings];
                        }

                        numthings++;
                    }
                    else if (Bstrcasecmp(&tempbuf[lasti],"[THINGTYPES]") == 0)
                        thingtypemode = 1;

                    if (texturelookupmode == 1)
                    {
                        textnum = 0;
                        if ((tempbuf[lasti] >= 48) && (tempbuf[lasti] <= 57))
                        {
                            k = lasti;
                            while ((tempbuf[k] != 0) && (tempbuf[k] != '-')) k++;

                            if (tempbuf[k] == '-')
                            {
                                tempbuf[k] = 0;
                                tstart = Batol(&tempbuf[lasti]);
                                tend = Batol(&tempbuf[k+1]);
                                for(k=tstart;k<=tend;k++)
                                    tempshort[textnum++] = k;
                            }
                            else
                                tempshort[textnum++] = Batol(&tempbuf[lasti]);
                        }
                        else
                        {
                            slen = 0;
                            while (tempbuf[lasti+slen] != 0)
                            {
                                ch = tempbuf[lasti+slen];
                                if ((ch >= 97) && (ch <= 122)) tempbuf[lasti+slen] -= 32;
                                slen++;
                            }
                            if (slen > 0)
                                for(k=inumwads-1;k>=0;k--)
                                    if ((iwadata[k][slen] == 0) || (iwadata[k][slen] == 32))
                                        if ((iwadata[k][slen-1] != 0) && (iwadata[k][slen-1] != 32))
                                        {
                                            for(l=slen-1;l>=0;l--)
                                                if (tempbuf[lasti+l] != '?')
                                                {
                                                    ch = iwadata[k][l];
                                                    if ((ch >= 97) && (ch <= 122)) ch -= 32;
                                                    if (tempbuf[lasti+l] != ch) break;
                                                }
                                            if (l < 0) tempshort[textnum++] = k;
                                        }
                        }
                    }
                    else if (Bstrcasecmp(&tempbuf[lasti],"[TEXTURELOOKUPS]") == 0)
                        texturelookupmode = 1;

                    if (tagtypemode == 1)
                    {
                        tagoff[numtags] = tagopnum;

                        k = lasti;
                        while ((tempbuf[k] != 0) && (tempbuf[k] != '-')) k++;

                        if (tempbuf[k] == '-')
                        {
                            tempbuf[k] = 0;
                            tagnum[numtags] = Batol(&tempbuf[lasti]);
                            tagnum2[numtags] = Batol(&tempbuf[k+1]);
                        }
                        else
                        {
                            tagnum[numtags] = Batol(&tempbuf[lasti]);
                            tagnum2[numtags] = tagnum[numtags];
                        }

                        numtags++;
                    }
                    else if (Bstrcasecmp(&tempbuf[lasti],"[TAGCONVERSIONS]") == 0)
                        tagtypemode = 1;

                    if (sectypemode == 1)
                    {
                        secoff[numsecs] = secopnum;

                        k = lasti;
                        while ((tempbuf[k] != 0) && (tempbuf[k] != '-')) k++;

                        if (tempbuf[k] == '-')
                        {
                            tempbuf[k] = 0;
                            secnum[numsecs] = Batol(&tempbuf[lasti]);
                            secnum2[numsecs] = Batol(&tempbuf[k+1]);
                        }
                        else
                        {
                            secnum[numsecs] = Batol(&tempbuf[lasti]);
                            secnum2[numsecs] = secnum[numsecs];
                        }
                        numsecs++;
                    }
                    else if (Bstrcasecmp(&tempbuf[lasti],"[SECTORCONVERSIONS]") == 0)
                        sectypemode = 1;

                }
                else if (j > 0)
                {
                    if (definemode == 1)
                    {
                        defineptr[numdefines] = (char *)(&define[definecnt]);
                        for(k=lasti;k<i;k++) define[definecnt++] = tempbuf[k];
                        define[definecnt++] = 0;
                        definemode = 2;
                    }
                    else if (definemode == 2)
                    {
                        defineval[numdefines++] = Batol(&tempbuf[lasti]);
                        definemode = 0;
                    }

                    if (thingtypemode == 1)
                    {
                        for(k=lasti;k<i;k++) if (tempbuf[k] == '=') break;
                        thingop[thingopnum] = 0;
                        if (tempbuf[k-1] == '+') thingop[thingopnum] = 1;
                        if (tempbuf[k-1] == '-') thingop[thingopnum] = 2;
                        if (tempbuf[k-1] == '|') thingop[thingopnum] = 3;
                        if (tempbuf[k-1] == '&') thingop[thingopnum] = 4;
                        if (tempbuf[k-1] == '^') thingop[thingopnum] = 5;
                        if (thingop[thingopnum] != 0) tempbuf[k-1] = 0;
                                                         else tempbuf[k] = 0;

                        if (Bstrcasecmp(&tempbuf[lasti],"x") == 0) thingfield[thingopnum] = 0;
                        if (Bstrcasecmp(&tempbuf[lasti],"y") == 0) thingfield[thingopnum] = 1;
                        if (Bstrcasecmp(&tempbuf[lasti],"z") == 0) thingfield[thingopnum] = 2;
                        if (Bstrcasecmp(&tempbuf[lasti],"cstat") == 0) thingfield[thingopnum] = 3;
                        if (Bstrcasecmp(&tempbuf[lasti],"shade") == 0) thingfield[thingopnum] = 4;
                        if (Bstrcasecmp(&tempbuf[lasti],"pal") == 0) thingfield[thingopnum] = 5;
                        if (Bstrcasecmp(&tempbuf[lasti],"clipdist") == 0) thingfield[thingopnum] = 6;
                        if (Bstrcasecmp(&tempbuf[lasti],"xrepeat") == 0) thingfield[thingopnum] = 7;
                        if (Bstrcasecmp(&tempbuf[lasti],"yrepeat") == 0) thingfield[thingopnum] = 8;
                        if (Bstrcasecmp(&tempbuf[lasti],"xoffset") == 0) thingfield[thingopnum] = 9;
                        if (Bstrcasecmp(&tempbuf[lasti],"yoffset") == 0) thingfield[thingopnum] = 10;
                        if (Bstrcasecmp(&tempbuf[lasti],"picnum") == 0) thingfield[thingopnum] = 11;
                        if (Bstrcasecmp(&tempbuf[lasti],"ang") == 0) thingfield[thingopnum] = 12;
                        if (Bstrcasecmp(&tempbuf[lasti],"xvel") == 0) thingfield[thingopnum] = 13;
                        if (Bstrcasecmp(&tempbuf[lasti],"yvel") == 0) thingfield[thingopnum] = 14;
                        if (Bstrcasecmp(&tempbuf[lasti],"zvel") == 0) thingfield[thingopnum] = 15;
                        if (Bstrcasecmp(&tempbuf[lasti],"owner") == 0) thingfield[thingopnum] = 16;
                        if (Bstrcasecmp(&tempbuf[lasti],"sectnum") == 0) thingfield[thingopnum] = 17;
                        if (Bstrcasecmp(&tempbuf[lasti],"statnum") == 0) thingfield[thingopnum] = 18;
                        if (Bstrcasecmp(&tempbuf[lasti],"lotag") == 0) thingfield[thingopnum] = 19;
                        if (Bstrcasecmp(&tempbuf[lasti],"hitag") == 0) thingfield[thingopnum] = 20;
                        if (Bstrcasecmp(&tempbuf[lasti],"extra") == 0) thingfield[thingopnum] = 21;

                        if ((tempbuf[k+1] >= 48) && (tempbuf[k+1] <= 57))
                            thingval[thingopnum] = Batol(&tempbuf[k+1]);
                        else
                        {
                            for(l=0;l<numdefines;l++)
                                if (Bstrcasecmp(defineptr[l],&tempbuf[k+1]) == 0)
                                    { thingval[thingopnum] = defineval[l]; break; }
                        }

                        thingopnum++;
                    }

                    if ((texturelookupmode == 1) && (j == 1))
                    {
                        if ((tempbuf[lasti] >= 48) && (tempbuf[lasti] <= 57))
                            l = Batol(&tempbuf[lasti]);
                        else
                        {
                            for(l=0;l<numdefines;l++)
                                if (Bstrcasecmp(defineptr[l],&tempbuf[lasti]) == 0)
                                    { l = defineval[l]; break; }
                        }
                        for(k=textnum-1;k>=0;k--) texturelookup[tempshort[k]] = l;
                    }

                    if (tagtypemode == 1)
                    {
                        for(k=lasti;k<i;k++) if (tempbuf[k] == '=') break;
                        tagop[tagopnum] = 0;
                        if (tempbuf[k-1] == '+') tagop[tagopnum] = 1;
                        if (tempbuf[k-1] == '-') tagop[tagopnum] = 2;
                        if (tempbuf[k-1] == '|') tagop[tagopnum] = 3;
                        if (tempbuf[k-1] == '&') tagop[tagopnum] = 4;
                        if (tempbuf[k-1] == '^') tagop[tagopnum] = 5;
                        if (tagop[tagopnum] != 0) tempbuf[k-1] = 0;
                                                         else tempbuf[k] = 0;

                            //Pick off first letter - is it f or b?
                        frontbackstat = 0;
                        if ((tempbuf[lasti] == 'b') || (tempbuf[lasti] == 'B')) frontbackstat = 1;
                        lasti++;

                        spritenumstat = 0;
                        if ((tempbuf[lasti] >= 48) && (tempbuf[lasti] <= 57))  //1 DIGIT ONLY!
                        {
                            spritenumstat = tempbuf[lasti]-48;
                            lasti++;
                        }

                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.x") == 0) tagfield[tagopnum] = 0;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.y") == 0) tagfield[tagopnum] = 1;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.z") == 0) tagfield[tagopnum] = 2;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.cstat") == 0) tagfield[tagopnum] = 3;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.shade") == 0) tagfield[tagopnum] = 4;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.pal") == 0) tagfield[tagopnum] = 5;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.clipdist") == 0) tagfield[tagopnum] = 6;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.xrepeat") == 0) tagfield[tagopnum] = 7;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.yrepeat") == 0) tagfield[tagopnum] = 8;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.xoffset") == 0) tagfield[tagopnum] = 9;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.yoffset") == 0) tagfield[tagopnum] = 10;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.picnum") == 0) tagfield[tagopnum] = 11;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.ang") == 0) tagfield[tagopnum] = 12;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.xvel") == 0) tagfield[tagopnum] = 13;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.yvel") == 0) tagfield[tagopnum] = 14;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.zvel") == 0) tagfield[tagopnum] = 15;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.owner") == 0) tagfield[tagopnum] = 16;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.sectnum") == 0) tagfield[tagopnum] = 17;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.statnum") == 0) tagfield[tagopnum] = 18;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.lotag") == 0) tagfield[tagopnum] = 19;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.hitag") == 0) tagfield[tagopnum] = 20;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.extra") == 0) tagfield[tagopnum] = 21;

                        if (Bstrcasecmp(&tempbuf[lasti],"sector.wallptr") == 0) tagfield[tagopnum] = 32;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.wallnum") == 0) tagfield[tagopnum] = 33;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.ceilingpicnum") == 0) tagfield[tagopnum] = 34;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.floorpicnum") == 0) tagfield[tagopnum] = 35;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.ceilingheinum") == 0) tagfield[tagopnum] = 36;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.floorheinum") == 0) tagfield[tagopnum] = 37;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.ceilingz") == 0) tagfield[tagopnum] = 38;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.floorz") == 0) tagfield[tagopnum] = 39;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.ceilingshade") == 0) tagfield[tagopnum] = 40;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.floorshade") == 0) tagfield[tagopnum] = 41;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.ceilingxpanning") == 0) tagfield[tagopnum] = 42;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.floorxpanning") == 0) tagfield[tagopnum] = 43;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.ceilingypanning") == 0) tagfield[tagopnum] = 44;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.floorypanning") == 0) tagfield[tagopnum] = 45;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.ceilingstat") == 0) tagfield[tagopnum] = 46;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.floorstat") == 0) tagfield[tagopnum] = 47;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.ceilingpal") == 0) tagfield[tagopnum] = 48;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.floorpal") == 0) tagfield[tagopnum] = 49;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.visibility") == 0) tagfield[tagopnum] = 50;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.lotag") == 0) tagfield[tagopnum] = 51;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.hitag") == 0) tagfield[tagopnum] = 52;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.extra") == 0) tagfield[tagopnum] = 53;

                        if (Bstrcasecmp(&tempbuf[lasti],"wall.x") == 0) tagfield[tagopnum] = 64;
                        if (Bstrcasecmp(&tempbuf[lasti],"wall.y") == 0) tagfield[tagopnum] = 65;
                        if (Bstrcasecmp(&tempbuf[lasti],"wall.point2") == 0) tagfield[tagopnum] = 66;
                        if (Bstrcasecmp(&tempbuf[lasti],"wall.nextsector") == 0) tagfield[tagopnum] = 67;
                        if (Bstrcasecmp(&tempbuf[lasti],"wall.nextwall") == 0) tagfield[tagopnum] = 68;
                        if (Bstrcasecmp(&tempbuf[lasti],"wall.picnum") == 0) tagfield[tagopnum] = 69;
                        if (Bstrcasecmp(&tempbuf[lasti],"wall.overpicnum") == 0) tagfield[tagopnum] = 70;
                        if (Bstrcasecmp(&tempbuf[lasti],"wall.shade") == 0) tagfield[tagopnum] = 71;
                        if (Bstrcasecmp(&tempbuf[lasti],"wall.pal") == 0) tagfield[tagopnum] = 72;
                        if (Bstrcasecmp(&tempbuf[lasti],"wall.cstat") == 0) tagfield[tagopnum] = 73;
                        if (Bstrcasecmp(&tempbuf[lasti],"wall.xrepeat") == 0) tagfield[tagopnum] = 74;
                        if (Bstrcasecmp(&tempbuf[lasti],"wall.yrepeat") == 0) tagfield[tagopnum] = 75;
                        if (Bstrcasecmp(&tempbuf[lasti],"wall.xpanning") == 0) tagfield[tagopnum] = 76;
                        if (Bstrcasecmp(&tempbuf[lasti],"wall.ypanning") == 0) tagfield[tagopnum] = 77;
                        if (Bstrcasecmp(&tempbuf[lasti],"wall.lotag") == 0) tagfield[tagopnum] = 78;
                        if (Bstrcasecmp(&tempbuf[lasti],"wall.hitag") == 0) tagfield[tagopnum] = 79;
                        if (Bstrcasecmp(&tempbuf[lasti],"wall.extra") == 0) tagfield[tagopnum] = 80;

                        tagfield[tagopnum] += (frontbackstat<<7) + (spritenumstat<<8);

                        if ((tempbuf[k+1] >= 48) && (tempbuf[k+1] <= 57))
                            tagval[tagopnum] = Batol(&tempbuf[k+1]);
                        else if (Bstrcasecmp("tag",&tempbuf[k+1]) == 0)
                            tagval[tagopnum] = INT32_MIN;
                        else
                        {
                            for(l=0;l<numdefines;l++)
                                if (Bstrcasecmp(defineptr[l],&tempbuf[k+1]) == 0)
                                    { tagval[tagopnum] = defineval[l]; break; }
                        }

                        tagopnum++;
                    }

                    if (sectypemode == 1)
                    {
                        for(k=lasti;k<i;k++) if (tempbuf[k] == '=') break;
                        secop[secopnum] = 0;
                        if (tempbuf[k-1] == '+') secop[secopnum] = 1;
                        if (tempbuf[k-1] == '-') secop[secopnum] = 2;
                        if (tempbuf[k-1] == '|') secop[secopnum] = 3;
                        if (tempbuf[k-1] == '&') secop[secopnum] = 4;
                        if (tempbuf[k-1] == '^') secop[secopnum] = 5;
                        if (secop[secopnum] != 0) tempbuf[k-1] = 0;
                                                     else tempbuf[k] = 0;

                        spritenumstat = 0;
                        if ((tempbuf[lasti] >= 48) && (tempbuf[lasti] <= 57))  //1 DIGIT ONLY!
                        {
                            spritenumstat = tempbuf[lasti]-48;
                            lasti++;
                        }

                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.x") == 0) secfield[secopnum] = 0;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.y") == 0) secfield[secopnum] = 1;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.z") == 0) secfield[secopnum] = 2;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.cstat") == 0) secfield[secopnum] = 3;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.shade") == 0) secfield[secopnum] = 4;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.pal") == 0) secfield[secopnum] = 5;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.clipdist") == 0) secfield[secopnum] = 6;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.xrepeat") == 0) secfield[secopnum] = 7;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.yrepeat") == 0) secfield[secopnum] = 8;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.xoffset") == 0) secfield[secopnum] = 9;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.yoffset") == 0) secfield[secopnum] = 10;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.picnum") == 0) secfield[secopnum] = 11;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.ang") == 0) secfield[secopnum] = 12;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.xvel") == 0) secfield[secopnum] = 13;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.yvel") == 0) secfield[secopnum] = 14;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.zvel") == 0) secfield[secopnum] = 15;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.owner") == 0) secfield[secopnum] = 16;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.sectnum") == 0) secfield[secopnum] = 17;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.statnum") == 0) secfield[secopnum] = 18;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.lotag") == 0) secfield[secopnum] = 19;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.hitag") == 0) secfield[secopnum] = 20;
                        if (Bstrcasecmp(&tempbuf[lasti],"sprite.extra") == 0) secfield[secopnum] = 21;

                        if (Bstrcasecmp(&tempbuf[lasti],"sector.wallptr") == 0) secfield[secopnum] = 32;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.wallnum") == 0) secfield[secopnum] = 33;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.ceilingpicnum") == 0) secfield[secopnum] = 34;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.floorpicnum") == 0) secfield[secopnum] = 35;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.ceilingheinum") == 0) secfield[secopnum] = 36;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.floorheinum") == 0) secfield[secopnum] = 37;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.ceilingz") == 0) secfield[secopnum] = 38;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.floorz") == 0) secfield[secopnum] = 39;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.ceilingshade") == 0) secfield[secopnum] = 40;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.floorshade") == 0) secfield[secopnum] = 41;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.ceilingxpanning") == 0) secfield[secopnum] = 42;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.floorxpanning") == 0) secfield[secopnum] = 43;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.ceilingypanning") == 0) secfield[secopnum] = 44;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.floorypanning") == 0) secfield[secopnum] = 45;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.ceilingstat") == 0) secfield[secopnum] = 46;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.floorstat") == 0) secfield[secopnum] = 47;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.ceilingpal") == 0) secfield[secopnum] = 48;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.floorpal") == 0) secfield[secopnum] = 49;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.visibility") == 0) secfield[secopnum] = 50;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.lotag") == 0) secfield[secopnum] = 51;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.hitag") == 0) secfield[secopnum] = 52;
                        if (Bstrcasecmp(&tempbuf[lasti],"sector.extra") == 0) secfield[secopnum] = 53;

                        secfield[secopnum] += (spritenumstat<<8);

                        if ((tempbuf[k+1] >= 48) && (tempbuf[k+1] <= 57))
                            secval[secopnum] = Batol(&tempbuf[k+1]);
                        else if (Bstrcasecmp("tag",&tempbuf[k+1]) == 0)
                            secval[secopnum] = INT32_MIN;
                        else
                        {
                            for(l=0;l<numdefines;l++)
                                if (Bstrcasecmp(defineptr[l],&tempbuf[k+1]) == 0)
                                    { secval[secopnum] = defineval[l]; break; }
                        }

                        secopnum++;
                    }

                }
                if (breakout == 1) break;
                lasti = i+1; j++;
            }
            i++;
        }
    }
    thingoff[numthings] = thingopnum;
    tagoff[numtags] = tagopnum;
    secoff[numsecs] = secopnum;
    close(filhandle);
}

int getspritefield(int i, int fieldnum)
{
    switch(fieldnum)
    {
        case 0: return((int)sprite[i].x);
        case 1: return((int)sprite[i].y);
        case 2: return((int)sprite[i].z);
        case 3: return((int)sprite[i].cstat);
        case 4: return((int)sprite[i].shade);
        case 5: return((int)sprite[i].pal);
        case 6: return((int)sprite[i].clipdist);
        case 7: return((int)sprite[i].xrepeat);
        case 8: return((int)sprite[i].yrepeat);
        case 9: return((int)sprite[i].xoffset);
        case 10: return((int)sprite[i].yoffset);
        case 11: return((int)sprite[i].picnum);
        case 12: return((int)sprite[i].ang);
        case 13: return((int)sprite[i].xvel);
        case 14: return((int)sprite[i].yvel);
        case 15: return((int)sprite[i].zvel);
        case 16: return((int)sprite[i].owner);
        case 17: return((int)sprite[i].sectnum);
        case 18: return((int)sprite[i].statnum);
        case 19: return((int)sprite[i].lotag);
        case 20: return((int)sprite[i].hitag);
        case 21: return((int)sprite[i].extra);
    }
    return(0);
}

void setspritefield(int i, int fieldnum, int newval)
{
    switch(fieldnum)
    {
        case 0: sprite[i].x = newval; break;
        case 1: sprite[i].y = newval; break;
        case 2: sprite[i].z = newval; break;
        case 3: sprite[i].cstat = newval; break;
        case 4: sprite[i].shade = newval; break;
        case 5: sprite[i].pal = newval; break;
        case 6: sprite[i].clipdist = newval; break;
        case 7: sprite[i].xrepeat = newval; break;
        case 8: sprite[i].yrepeat = newval; break;
        case 9: sprite[i].xoffset = newval; break;
        case 10: sprite[i].yoffset = newval; break;
        case 11: sprite[i].picnum = newval; break;
        case 12: sprite[i].ang = newval; break;
        case 13: sprite[i].xvel = newval; break;
        case 14: sprite[i].yvel = newval; break;
        case 15: sprite[i].zvel = newval; break;
        case 16: sprite[i].owner = newval; break;
        case 17: sprite[i].sectnum = newval; break;
        case 18: sprite[i].statnum = newval; break;
        case 19: sprite[i].lotag = newval; break;
        case 20: sprite[i].hitag = newval; break;
        case 21: sprite[i].extra = newval; break;
    }
}

int getwallfield(int i, int fieldnum)
{
    switch(fieldnum)
    {
        case 64: return((int)wall[i].x);
        case 65: return((int)wall[i].y);
        case 66: return((int)wall[i].point2);
        case 67: return((int)wall[i].nextsector);
        case 68: return((int)wall[i].nextwall);
        case 69: return((int)wall[i].picnum);
        case 70: return((int)wall[i].overpicnum);
        case 71: return((int)wall[i].shade);
        case 72: return((int)wall[i].pal);
        case 73: return((int)wall[i].cstat);
        case 74: return((int)wall[i].xrepeat);
        case 75: return((int)wall[i].yrepeat);
        case 76: return((int)wall[i].xpanning);
        case 77: return((int)wall[i].ypanning);
        case 78: return((int)wall[i].lotag);
        case 79: return((int)wall[i].hitag);
        case 80: return((int)wall[i].extra);
    }
    return(0);
}

void setwallfield(int i, int fieldnum, int newval)
{
    switch(fieldnum)
    {
        case 64: wall[i].x = newval; break;
        case 65: wall[i].y = newval; break;
        case 66: wall[i].point2 = newval; break;
        case 67: wall[i].nextsector = newval; break;
        case 68: wall[i].nextwall = newval; break;
        case 69: wall[i].picnum = newval; break;
        case 70: wall[i].overpicnum = newval; break;
        case 71: wall[i].shade = newval; break;
        case 72: wall[i].pal = newval; break;
        case 73: wall[i].cstat = newval; break;
        case 74: wall[i].xrepeat = newval; break;
        case 75: wall[i].yrepeat = newval; break;
        case 76: wall[i].xpanning = newval; break;
        case 77: wall[i].ypanning = newval; break;
        case 78: wall[i].lotag = newval; break;
        case 79: wall[i].hitag = newval; break;
        case 80: wall[i].extra = newval; break;
    }
}

int getsectorfield(int i, int fieldnum)
{
    switch(fieldnum)
    {
        case 32: return((int)sector[i].wallptr);
        case 33: return((int)sector[i].wallnum);
        case 34: return((int)sector[i].ceilingpicnum);
        case 35: return((int)sector[i].floorpicnum);
        case 36: return((int)sector[i].ceilingheinum);
        case 37: return((int)sector[i].floorheinum);
        case 38: return((int)sector[i].ceilingz);
        case 39: return((int)sector[i].floorz);
        case 40: return((int)sector[i].ceilingshade);
        case 41: return((int)sector[i].floorshade);
        case 42: return((int)sector[i].ceilingxpanning);
        case 43: return((int)sector[i].floorxpanning);
        case 44: return((int)sector[i].ceilingypanning);
        case 45: return((int)sector[i].floorypanning);
        case 46: return((int)sector[i].ceilingstat);
        case 47: return((int)sector[i].floorstat);
        case 48: return((int)sector[i].ceilingpal);
        case 49: return((int)sector[i].floorpal);
        case 50: return((int)sector[i].visibility);
        case 51: return((int)sector[i].lotag);
        case 52: return((int)sector[i].hitag);
        case 53: return((int)sector[i].extra);
    }
    return(0);
}

void setsectorfield(int i, int fieldnum, int newval)
{
    switch(fieldnum)
    {
        case 32: sector[i].wallptr = newval; break;
        case 33: sector[i].wallnum = newval; break;
        case 34: sector[i].ceilingpicnum = newval; break;
        case 35: sector[i].floorpicnum = newval; break;
        case 36: sector[i].ceilingheinum = newval; break;
        case 37: sector[i].floorheinum = newval; break;
        case 38: sector[i].ceilingz = newval; break;
        case 39: sector[i].floorz = newval; break;
        case 40: sector[i].ceilingshade = newval; break;
        case 41: sector[i].floorshade = newval; break;
        case 42: sector[i].ceilingxpanning = newval; break;
        case 43: sector[i].floorxpanning = newval; break;
        case 44: sector[i].ceilingypanning = newval; break;
        case 45: sector[i].floorypanning = newval; break;
        case 46: sector[i].ceilingstat = newval; break;
        case 47: sector[i].floorstat = newval; break;
        case 48: sector[i].ceilingpal = newval; break;
        case 49: sector[i].floorpal = newval; break;
        case 50: sector[i].visibility = newval; break;
        case 51: sector[i].lotag = newval; break;
        case 52: sector[i].hitag = newval; break;
        case 53: sector[i].extra = newval; break;
    }
}

int getwadindex(char const * nam)
{
    int i, j;

    i = 0;
    for(j=2048;j>0;j>>=1)
        if (i+j < inumwads)
            if (Bstrcasecmp(iwadata[slist[i+j]],nam) <= 0) i += j;
    if (Bstrcasecmp(iwadata[slist[i]],nam) == 0) return(slist[i]);
    return(-1);
}

int main(int argc, char **argv)
{
    char argstring[5][80];
    char iwadfil[80], pwadfil[80], doommap[16], buildmap[16], picstr[16];
    int w, numtexts, startnumtexts, danumtexts, numpnames;
    int x, y, z, zz, zzz, zzx, zx, cx, cy, gap, i, j, k, l, offs;
    int dnumpoints = 0, dnumlines = 0, dnumsides, dnumsectors, dnumthings;
    int minx, maxx, miny, maxy, numwalls, wadtype;
    int startnumwalls, l1, l2, startpoint2;
    int fil, ifil, pfil, x1, y1, x2, y2, startwall, endwall;
    int mapversion, posx, posy, posz, boardwadindex;
    short ang, cursectnum;

    Bprintf("Wad2Map!                                       Copyright 1995 by Ken Silverman\n");

    if ((argc < 3) || (argc > 5))
    {
        Bprintf("Command line parameters: Wad2Map (PWADName) IWADName MapName (ScriptName)\n");
        Bprintf("   Ex #1: wad2map c:\\doom\\doom.wad e1m1\n");
        Bprintf("   Ex #2: wad2map c:\\doom\\doom.wad e1m1 kenbuild.txt\n");
        Bprintf("   Ex #3: wad2map c:\\doom\\mypwad.wad c:\\doom\\doom.wad e1m1\n");
        Bprintf("   Ex #4: wad2map c:\\doom\\mypwad.wad c:\\doom\\doom.wad e1m1 kenbuild.txt\n");
        exit(0);
    }

    for(i=1;i<argc;i++) strcpy(argstring[i],argv[i]);

    strcpy(iwadfil,argstring[1]);     //"c:\games\doom\doom.wad"
    if (strchr(iwadfil,'.') == 0) strcat(iwadfil,".wad");
    if ((ifil = Bopen(iwadfil,BO_BINARY|BO_RDONLY,BS_IREAD)) == -1)
        { Bprintf("Could not find %s\n",iwadfil); exit(0); }

    Bread(ifil,&wadtype,4);
    if (wadtype == 0x44415749) wadtype = 0;       //IWAD
    else if (wadtype == 0x44415750) wadtype = 1;  //PWAD
    else { Bclose(ifil); Bprintf("Invalid WAD header\n"); exit(0); }

    strcpy(pwadfil,iwadfil); pfil = ifil;
    if (wadtype == 1)
    {
        strcpy(iwadfil,argstring[2]);
        if (strchr(iwadfil,'.') == 0) strcat(iwadfil,".wad");
        if ((ifil = Bopen(iwadfil,BO_BINARY|BO_RDONLY,BS_IREAD)) == -1)
            { Bclose(pfil); Bprintf("Could not find %s\n",iwadfil); exit(0); }

        Bread(ifil,&wadtype,4);
        if (wadtype != 0x44415749)        //!= IWAD
        {
            Bclose(ifil); Bclose(pfil);
            Bprintf("Wad a' you think I am?  That ain't no IWAD!\n");
            exit(0);
        }

        strcpy(doommap,argstring[3]);     //"E1M1"
        if (strchr(doommap,'.') != 0) *strchr(doommap,'.') = 0;
        scriptname[0] = 0;
        if (argc == 5)
        {
            strcpy(scriptname,argstring[4]);
            if (strchr(scriptname,'.') == 0) strcat(scriptname,".txt");
        }
    }
    else
    {
        strcpy(doommap,argstring[2]);     //"E1M1"
        if (strchr(doommap,'.') != 0) *strchr(doommap,'.') = 0;
        scriptname[0] = 0;
        if (argc == 4)
        {
            strcpy(scriptname,argstring[3]);
            if (strchr(scriptname,'.') == 0) strcat(scriptname,".txt");
        }
    }

    initksqrt();

    Bread(ifil,&inumwads,4); Bread(ifil,&iwadstart,4);
    Blseek(ifil,iwadstart,BSEEK_SET);
    for(z=0;z<inumwads;z++)
    {
        if ((z&1023) == 0) Bread(ifil,tempbuf,16384);
        zz = ((z&1023)<<4);
        iwadplc[z] = ((int)tempbuf[zz]) + (((int)tempbuf[zz+1])<<8) + (((int)tempbuf[zz+2])<<16) + (((int)tempbuf[zz+3])<<24);
        zz += 4;
        iwadlen[z] = ((int)tempbuf[zz]) + (((int)tempbuf[zz+1])<<8) + (((int)tempbuf[zz+2])<<16) + (((int)tempbuf[zz+3])<<24);
        zz += 4;
        iwadata[z][0] = tempbuf[zz+0]; iwadata[z][1] = tempbuf[zz+1];
        iwadata[z][2] = tempbuf[zz+2]; iwadata[z][3] = tempbuf[zz+3];
        iwadata[z][4] = tempbuf[zz+4]; iwadata[z][5] = tempbuf[zz+5];
        iwadata[z][6] = tempbuf[zz+6]; iwadata[z][7] = tempbuf[zz+7];
        iwadata[z][8] = 0;
    }

    parsescript();

    for(z=0;z<inumwads;z++) slist[z] = z;

    for(gap=(inumwads>>1);gap>0;gap>>=1)
        for(z=0;z<inumwads-gap;z++)
            for(zz=z;zz>=0;zz-=gap)
            {
                if (Bstrcasecmp(iwadata[slist[zz]],iwadata[slist[zz+gap]]) <= 0) break;
                i = slist[zz]; slist[zz] = slist[zz+gap]; slist[zz+gap] = i;
            }

    if (ifil != pfil)
    {
        Bread(pfil,&pnumwads,4);
        Bread(pfil,&pwadstart,4);
        Blseek(pfil,pwadstart,SEEK_SET);
        for(z=0;z<pnumwads;z++)
        {
            if ((z&1023) == 0) Bread(ifil,tempbuf,16384);
            zz = ((z&1023)<<4);
            pwadplc[z] = ((int)tempbuf[zz]) + (((int)tempbuf[zz+1])<<8) + (((int)tempbuf[zz+2])<<16) + (((int)tempbuf[zz+3])<<24);
            zz += 4;
            pwadlen[z] = ((int)tempbuf[zz]) + (((int)tempbuf[zz+1])<<8) + (((int)tempbuf[zz+2])<<16) + (((int)tempbuf[zz+3])<<24);
            zz += 4;
            pwadata[z][0] = tempbuf[zz+0]; pwadata[z][1] = tempbuf[zz+1];
            pwadata[z][2] = tempbuf[zz+2]; pwadata[z][3] = tempbuf[zz+3];
            pwadata[z][4] = tempbuf[zz+4]; pwadata[z][5] = tempbuf[zz+5];
            pwadata[z][6] = tempbuf[zz+6]; pwadata[z][7] = tempbuf[zz+7];
            pwadata[z][8] = 0;
        }
    }
    else
    {
        pnumwads = inumwads;
        pwadstart = iwadstart;
        for(z=0;z<inumwads;z++)
        {
            pwadplc[z] = iwadplc[z];
            pwadlen[z] = iwadlen[z];
            pwadata[z][0] = iwadata[z][0]; pwadata[z][1] = iwadata[z][1];
            pwadata[z][2] = iwadata[z][2]; pwadata[z][3] = iwadata[z][3];
            pwadata[z][4] = iwadata[z][4]; pwadata[z][5] = iwadata[z][5];
            pwadata[z][6] = iwadata[z][6]; pwadata[z][7] = iwadata[z][7];
            pwadata[z][8] = iwadata[z][8];
        }
    }

    Bmemset(sector, 0, MAXSECTORS*sizeof(sectortype));
    Bmemset(wall, 0, MAXWALLS*sizeof(walltype));
    Bmemset(sprite, 0, MAXSPRITES*sizeof(spritetype));
    for(i=0;i<MAXSECTORS;i++) sector[i].extra = -1;
    for(i=0;i<MAXWALLS;i++) wall[i].extra = -1;
    for(i=0;i<MAXSPRITES;i++) sprite[i].extra = -1;

    if ((w = getwadindex("TEXTURE1")) < 0)
        { Bprintf("TEXTURE1 not found!\n"); exit(0); }
    Blseek(ifil,iwadplc[w],BSEEK_SET);
    Bread(ifil,&numtexts,4);
    Bread(ifil,textoffs,numtexts*sizeof(int));
    for(z=0;z<numtexts;z++)
    {
        Blseek(ifil,iwadplc[w]+textoffs[z],BSEEK_SET);
        Bread(ifil,tempbuf,32);
        textname[z][0] = tempbuf[0]; textname[z][1] = tempbuf[1];
        textname[z][2] = tempbuf[2]; textname[z][3] = tempbuf[3];
        textname[z][4] = tempbuf[4]; textname[z][5] = tempbuf[5];
        textname[z][6] = tempbuf[6]; textname[z][7] = tempbuf[7];
        textname[z][8] = 0;
        textpname[z] = ((int)tempbuf[26]) + (((int)tempbuf[27])<<8);
    }

    if ((w = getwadindex("TEXTURE2")) >= 0)
    {
        Blseek(ifil,iwadplc[w],BSEEK_SET);
        startnumtexts = numtexts;
        Bread(ifil,&danumtexts,4); numtexts += danumtexts;
        Bread(ifil,&textoffs[startnumtexts],(numtexts-startnumtexts)*sizeof(int));
        for(z=startnumtexts;z<numtexts;z++)
        {
            Blseek(ifil,iwadplc[w]+textoffs[z],BSEEK_SET);
            Bread(ifil,tempbuf,32);
            textname[z][0] = tempbuf[0]; textname[z][1] = tempbuf[1];
            textname[z][2] = tempbuf[2]; textname[z][3] = tempbuf[3];
            textname[z][4] = tempbuf[4]; textname[z][5] = tempbuf[5];
            textname[z][6] = tempbuf[6]; textname[z][7] = tempbuf[7];
            textname[z][8] = 0;
            textpname[z] = ((int)tempbuf[26]) + (((int)tempbuf[27])<<8);
        }
    }

    if ((w = getwadindex("PNAMES")) < 0)
        { Bprintf("PNAMES not found!\n"); exit(0); }
    Blseek(ifil,iwadplc[w],BSEEK_SET);
    Bread(ifil,&numpnames,4);
    Bread(ifil,tempbuf,numpnames*8);
    for(z=0;z<numpnames;z++)
    {
        zz = (z<<3);
        pname[z][0] = tempbuf[zz+0]; pname[z][1] = tempbuf[zz+1];
        pname[z][2] = tempbuf[zz+2]; pname[z][3] = tempbuf[zz+3];
        pname[z][4] = tempbuf[zz+4]; pname[z][5] = tempbuf[zz+5];
        pname[z][6] = tempbuf[zz+6]; pname[z][7] = tempbuf[zz+7];
        pname[z][8] = 0;
    }

    if ((w = getwadindex(doommap)) < 0)
        { Bprintf("Board not found!\n"); exit(0); }
    boardwadindex = w;
    for(w=boardwadindex+10;w>=boardwadindex;w--)
    {
        Blseek(pfil,pwadplc[w],BSEEK_SET);

        if (Bstrcasecmp(pwadata[w],"VERTEXES") == 0)
        {
            dnumpoints = (pwadlen[w]>>2);
            Bread(pfil,tempbuf,pwadlen[w]);
            offs = 0;
            for(z=0;z<dnumpoints;z++)
            {
                px[z] = ((short)tempbuf[offs])+(((short)tempbuf[offs+1])<<8);
                py[z] = -(((short)tempbuf[offs+2])+(((short)tempbuf[offs+3])<<8));
                offs += 4;
            }
        }
        if (Bstrcasecmp(pwadata[w],"LINEDEFS") == 0)
        {
            dnumlines = pwadlen[w]/sizeof(linedeftype);
            Bread(pfil,line,sizeof(linedeftype)*dnumlines);
        }
        if (Bstrcasecmp(pwadata[w],"SIDEDEFS") == 0)
        {
            dnumsides = pwadlen[w]/sizeof(sidedeftype);
            Bread(pfil,side,pwadlen[w]);
            for(z=0;z<dnumsides;z++)
            {
                for(i=0;i<8;i++) picstr[i] = side[z].uppertexture[i];
                picstr[8] = 0;

                sidetoppic[z] = -1;
                if (picstr[0] != '-')
                    for(zx=0;zx<numtexts;zx++)
                        if (Bstrcasecmp(picstr,textname[zx]) == 0)
                            { sidetoppic[z] = getwadindex(pname[textpname[zx]]); break; }

                for(i=0;i<8;i++) picstr[i] = side[z].lowertexture[i];
                picstr[8] = 0;

                sidebotpic[z] = -1;
                if (picstr[0] != '-')
                    for(zx=0;zx<numtexts;zx++)
                        if (Bstrcasecmp(picstr,textname[zx]) == 0)
                            { sidebotpic[z] = getwadindex(pname[textpname[zx]]); break; }

                for(i=0;i<8;i++) picstr[i] = side[z].middletexture[i];
                picstr[8] = 0;

                sidemidpic[z] = -1;
                if (picstr[0] != '-')
                    for(zx=0;zx<numtexts;zx++)
                        if (Bstrcasecmp(picstr,textname[zx]) == 0)
                            { sidemidpic[z] = getwadindex(pname[textpname[zx]]); break; }
            }
        }
        if (Bstrcasecmp(pwadata[w],"SECTORS") == 0)
        {
            dnumsectors = pwadlen[w]/sizeof(secttype);
            Bread(pfil,sect,pwadlen[w]);
            for(z=0;z<dnumsectors;z++)
            {
                for(i=0;i<8;i++) picstr[i] = sect[z].floorpic[i];
                picstr[8] = 0;
                sector[z].floorpicnum = getwadindex(picstr);

                for(i=0;i<8;i++) picstr[i] = sect[z].ceilingpic[i];
                picstr[8] = 0;
                sector[z].ceilingpicnum = getwadindex(picstr);
            }
        }
        if (Bstrcasecmp(pwadata[w],"THINGS") == 0)
        {
            dnumthings = pwadlen[w]/sizeof(thingtype);
            Bread(pfil,thing,pwadlen[w]);
        }
    }
    close(ifil);
    if (ifil != pfil) close(pfil);


    minx = 32767; maxx = -32768;
    miny = 32767; maxy = -32768;
    for(z=0;z<dnumpoints;z++)
    {
        x = px[z]; y = py[z];
        if (x < minx) minx = x;
        if (x > maxx) maxx = x;
        if (y < miny) miny = y;
        if (y > maxy) maxy = y;
    }
    cx = (((minx+maxx)>>1)&0xffffffc0);
    cy = (((miny+maxy)>>1)&0xffffffc0);

    numwalls = 0;

    for(z=0;z<dnumsectors;z++)
    {
        startnumwalls = numwalls;
        for(zz=0;zz<dnumlines;zz++)
        {
            if (z == side[line[zz].side1].sect)
            {
                wx[numwalls] = px[line[zz].p1]; wy[numwalls] = py[line[zz].p1];
                wx2[numwalls] = px[line[zz].p2]; wy2[numwalls] = py[line[zz].p2];
                picindex[numwalls] = line[zz].side1; linindex[numwalls++] = zz;
            }
            if ((line[zz].side2 >= 0) && (z == side[line[zz].side2].sect))
            {
                wx[numwalls] = px[line[zz].p2]; wy[numwalls] = py[line[zz].p2];
                wx2[numwalls] = px[line[zz].p1]; wy2[numwalls] = py[line[zz].p1];
                picindex[numwalls] = line[zz].side2; linindex[numwalls++] = zz;
            }
        }

        startpoint2 = startnumwalls;
        for(zz=startnumwalls;zz<numwalls;zz++)
        {
            x = wx2[zz];
            y = wy2[zz];
            j = 0;
            for(zzz=zz+1;zzz<numwalls;zzz++)
                if ((wx[zzz] == x) && (wy[zzz] == y))
                {
                    if (j == 0)
                    {
                        i = wx[zz+1]; wx[zz+1] = wx[zzz]; wx[zzz] = i;
                        i = wy[zz+1]; wy[zz+1] = wy[zzz]; wy[zzz] = i;
                        i = wx2[zz+1]; wx2[zz+1] = wx2[zzz]; wx2[zzz] = i;
                        i = wy2[zz+1]; wy2[zz+1] = wy2[zzz]; wy2[zzz] = i;
                        i = picindex[zz+1]; picindex[zz+1] = picindex[zzz]; picindex[zzz] = i;
                        i = linindex[zz+1]; linindex[zz+1] = linindex[zzz]; linindex[zzz] = i;
                        j = 1;
                    }
                    else
                    {
                        j = 1;
                        l1 = (x-wx[zz])*(wy2[zz+1]-y) - (y-wy[zz])*(wx2[zz+1]-x);
                        l2 = (x-wx[zz])*(wy2[zzz]-y) - (y-wy[zz])*(wx2[zzz]-x);
                        if (l1 == 0) j = 2;       //Don't want collinear lines!
                        else if ((l1 < 0) && (l2 > 0)) j = 2;
                        else if ((wx2[zz+1]-x)*(wy2[zzz]-y) > (wy2[zz+1]-y)*(wx2[zzz]-x)) j = 2;

                        if (j == 2)
                        {
                            i = wx[zz+1]; wx[zz+1] = wx[zzz]; wx[zzz] = i;
                            i = wy[zz+1]; wy[zz+1] = wy[zzz]; wy[zzz] = i;
                            i = wx2[zz+1]; wx2[zz+1] = wx2[zzz]; wx2[zzz] = i;
                            i = wy2[zz+1]; wy2[zz+1] = wy2[zzz]; wy2[zzz] = i;
                            i = picindex[zz+1]; picindex[zz+1] = picindex[zzz]; picindex[zzz] = i;
                            i = linindex[zz+1]; linindex[zz+1] = linindex[zzz]; linindex[zzz] = i;
                        }
                    }
                    point2[zz] = zz+1;
                }
            if (j == 0) point2[zz] = startpoint2, startpoint2 = zz+1;
        }

        sector[z].wallptr = startnumwalls;
        sector[z].wallnum = numwalls-startnumwalls;
    }

        //Collect sectors
    for(z=0;z<dnumsectors;z++)
    {
        if (Bstrcasecmp(iwadata[sector[z].ceilingpicnum],"F_SKY1") == 0) sector[z].ceilingstat |= 1;
        if (Bstrcasecmp(iwadata[sector[z].floorpicnum],"F_SKY1") == 0) sector[z].floorstat |= 1;
        sector[z].ceilingz = -(sect[z].ceilingz<<8);
        sector[z].floorz = -(sect[z].floorz<<8);

        startwall = sector[z].wallptr;
        endwall = startwall + sector[z].wallnum;
        for(zz=startwall;zz<endwall;zz++) sectorofwall[zz] = z;

        j = 28-(sect[z].shade>>3);
        if ((sector[z].ceilingstat&1) == 0) sector[z].ceilingshade = j;
        if ((sector[z].floorstat&1) == 0) sector[z].floorshade = j;
        for(i=startwall;i<endwall;i++) wall[i].shade = j;
    }


    for(i=0;i<MAXTHINGTYPES;i++) thinglookup[i] = -1;

    for(i=0;i<THINGLISTNUM;i++)
        thinglookup[thinglist[i].num] = getwadindex(thinglist[i].name);

    for(i=0;i<MAXTHINGTYPES;i++)
        if (thinglookup[i] == -1)   //Make bad type numbers into cans
            thinglookup[i] = thinglookup[2035];


    for(z=0;z<numwalls;z++) slist[z] = z;

    for(gap=(numwalls>>1);gap>0;gap>>=1)
        for(z=0;z<numwalls-gap;z++)
            for(zz=z;zz>=0;zz-=gap)
            {
                if (wx[slist[zz]] <= wx[slist[zz+gap]]) break;
                i = slist[zz]; slist[zz] = slist[zz+gap]; slist[zz+gap] = i;
            }

    for(z=0;z<numwalls;z++)
    {
        wall[z].x = (((int)wx[z]-cx)<<4);
        wall[z].y = (((int)wy[z]-cy)<<4);
        wall[z].point2 = point2[z];

        wall[z].nextwall = -1;
        wall[z].nextsector = -1;

        x1 = wx[z]; x2 = wx[point2[z]];
        y1 = wy[z]; y2 = wy[point2[z]];

        zz = 0; zzz = 2048;
        do
        {
            if ((zz+zzz < numwalls) && (wx[slist[zz+zzz]] < x2)) zz += zzz;
            zzz >>= 1;
            if ((zz+zzz < numwalls) && (wx[slist[zz+zzz]] < x2)) zz += zzz;
            zzz >>= 1;
        } while (zzz > 0);

        do
        {
            zzx = slist[zz];
            if (wx[zzx] > x2) break;
            if (wy[zzx] == y2)
                if ((wx[point2[zzx]] == x1) && (wy[point2[zzx]] == y1))
                {
                    wall[z].nextwall = zzx;
                    wall[z].nextsector = sectorofwall[zzx];
                    break;
                }
            zz++;
        } while (zz < numwalls);

        if (wall[z].nextwall < 0)
        {
            wall[z].picnum = sidemidpic[picindex[z]];
            wall[z].overpicnum = 0;
        }
        else
        {
            wall[z].picnum = sidetoppic[picindex[z]];
            if (wall[z].picnum <= 0) wall[z].picnum = sidebotpic[picindex[z]];
            if ((wall[z].picnum <= 0) && (wall[z].nextwall >= 0))
            {
                zx = picindex[wall[z].nextwall];
                wall[z].picnum = sidetoppic[zx];
                if (wall[z].picnum <= 0) wall[z].picnum = sidebotpic[zx];
            }
            wall[z].overpicnum = sidemidpic[picindex[z]];
            if (wall[z].overpicnum >= 0) wall[z].cstat |= (1+4+16);
        }
        wall[z].xrepeat = 8;
        wall[z].yrepeat = 8;
        wall[z].xpanning = (char)((-side[picindex[z]].xoffset)&255);
        wall[z].ypanning = (char)(((side[picindex[z]].yoffset<<1))&255);

        if (line[linindex[z]].flags&1) wall[z].cstat |= 1;
        //if (wall[z].nextwall >= 0) wall[z].cstat |= 4;
        //if ((line[linindex[z]].flags&24) && (wall[z].nextwall >= 0))
        //   wall[z].cstat |= 4;
    }

    for(z=0;z<dnumthings;z++)
    {
        sprite[z].x = (((int)thing[z].x-cx)<<4);
        sprite[z].y = (((int)-thing[z].y-cy)<<4);

        sprite[z].sectnum = 0;
        for(zz=0;zz<dnumsectors;zz++)
            if (inside(sprite[z].x,sprite[z].y,(short)zz) != 0)
                { sprite[z].sectnum = zz; break; }

        sprite[z].z = -(sect[sprite[z].sectnum].floorz<<8);
        sprite[z].clipdist = 32;
        sprite[z].xrepeat = 64;
        sprite[z].yrepeat = 64;
        sprite[z].picnum = thinglookup[thing[z].type];
        sprite[z].ang = ((-((thing[z].ang<<11)/360))&2047);


        for(i=0;i<numthings;i++)
            if ((thing[z].type >= thingnum[i]) && (thing[z].type <= thingnum2[i]))
                for(j=thingoff[i];j<thingoff[i+1];j++)
                {
                    k = thingfield[j]; zz = thingval[j];
                    switch(thingop[j])
                    {
                        case 0: setspritefield(z,k,zz); break;
                        case 1: setspritefield(z,k,getspritefield(z,k)+zz); break;
                        case 2: setspritefield(z,k,getspritefield(z,k)-zz); break;
                        case 3: setspritefield(z,k,getspritefield(z,k)|zz); break;
                        case 4: setspritefield(z,k,getspritefield(z,k)&zz); break;
                        case 5: setspritefield(z,k,getspritefield(z,k)^zz); break;
                    }
                }
    }

    for(zx=0;zx<numwalls;zx++)
    {
        z = linindex[zx];
        for(i=0;i<numtags;i++)
        {
            if ((line[z].special < tagnum[i]) || (line[z].special > tagnum2[i])) continue;

            for(j=tagoff[i];j<tagoff[i+1];j++)
            {
                k = (tagfield[j]&127);
                zz = tagval[j]; if (zz == INT32_MIN) zz = line[z].tag;
                if (k < 32)
                {
                    l = sectorofwall[zx];
                    if (wall[zx].nextsector >= 0)
                        if ((tagfield[j]&128) ^ (picindex[zx] == line[z].side1))
                            l = wall[zx].nextsector;

                    zzz = sectspri[l][(tagfield[j]>>8)&7];
                    if (zzz < 0)
                    {
                        zzz = dnumthings++;
                        sectspri[l][(tagfield[j]>>8)&7] = zzz;
                        zzx = sector[l].wallptr; x1 = wall[zzx].x; y1 = wall[zzx].y;
                        zzx = wall[zzx].point2; x2 = wall[zzx].x; y2 = wall[zzx].y;
                        sprite[zzz].x = ((x1+x2)>>1) + ksgn(y1-y2);
                        sprite[zzz].y = ((y1+y2)>>1) + ksgn(x2-x1);
                        sprite[zzz].sectnum = l;
                        sprite[zzz].z = sector[l].floorz;
                        sprite[zzz].clipdist = 32;
                        sprite[zzz].xrepeat = 64;
                        sprite[zzz].yrepeat = 64;
                    }

                    switch(tagop[j])
                    {
                        case 0: setspritefield(zzz,k,zz); break;
                        case 1: setspritefield(zzz,k,getspritefield(zzz,k)+zz); break;
                        case 2: setspritefield(zzz,k,getspritefield(zzz,k)-zz); break;
                        case 3: setspritefield(zzz,k,getspritefield(zzz,k)|zz); break;
                        case 4: setspritefield(zzz,k,getspritefield(zzz,k)&zz); break;
                        case 5: setspritefield(zzz,k,getspritefield(zzz,k)^zz); break;
                    }
                }
                else if (k < 64)
                {
                    l = sectorofwall[zx];
                    if (wall[zx].nextsector >= 0)
                        if ((tagfield[j]&128) ^ (picindex[zx] == line[z].side1))
                            l = wall[zx].nextsector;

                    switch(tagop[j])
                    {
                        case 0: setsectorfield(l,k,zz); break;
                        case 1: setsectorfield(l,k,getsectorfield(l,k)+zz); break;
                        case 2: setsectorfield(l,k,getsectorfield(l,k)-zz); break;
                        case 3: setsectorfield(l,k,getsectorfield(l,k)|zz); break;
                        case 4: setsectorfield(l,k,getsectorfield(l,k)&zz); break;
                        case 5: setsectorfield(l,k,getsectorfield(l,k)^zz); break;
                    }
                }
                else if (k < 96)
                {
                    l = zx;
                    if (wall[zx].nextwall >= 0)
                        if ((tagfield[j]&128) ^ (picindex[zx] == line[z].side1))
                            l = wall[zx].nextwall;

                    switch(tagop[j])
                    {
                        case 0: setwallfield(l,k,zz); break;
                        case 1: setwallfield(l,k,getwallfield(l,k)+zz); break;
                        case 2: setwallfield(l,k,getwallfield(l,k)-zz); break;
                        case 3: setwallfield(l,k,getwallfield(l,k)|zz); break;
                        case 4: setwallfield(l,k,getwallfield(l,k)&zz); break;
                        case 5: setwallfield(l,k,getwallfield(l,k)^zz); break;
                    }
                }
            }
        }
    }

    for(l=0;l<dnumsectors;l++)
        for(i=0;i<numsecs;i++)
            if ((sect[l].type >= secnum[i]) && (sect[l].type <= secnum2[i]))
            {
                for(j=secoff[i];j<secoff[i+1];j++)
                {
                    k = (secfield[j]&127);
                    zz = secval[j]; if (zz == INT32_MIN) zz = sect[l].tag;
                    if (k < 32)
                    {
                        zzz = sectspri[l][(secfield[j]>>8)&7];
                        if (zzz < 0)
                        {
                            zzz = dnumthings++;
                            sectspri[l][(secfield[j]>>8)&7] = zzz;
                            zzx = sector[l].wallptr; x1 = wall[zzx].x; y1 = wall[zzx].y;
                            zzx = wall[zzx].point2; x2 = wall[zzx].x; y2 = wall[zzx].y;
                            sprite[zzz].x = ((x1+x2)>>1) + ksgn(y1-y2);
                            sprite[zzz].y = ((y1+y2)>>1) + ksgn(x2-x1);
                            sprite[zzz].sectnum = l;
                            sprite[zzz].z = sector[l].floorz;
                            sprite[zzz].clipdist = 32;
                            sprite[zzz].xrepeat = 64;
                            sprite[zzz].yrepeat = 64;
                        }

                        switch(secop[j])
                        {
                            case 0: setspritefield(zzz,k,zz); break;
                            case 1: setspritefield(zzz,k,getspritefield(zzz,k)+zz); break;
                            case 2: setspritefield(zzz,k,getspritefield(zzz,k)-zz); break;
                            case 3: setspritefield(zzz,k,getspritefield(zzz,k)|zz); break;
                            case 4: setspritefield(zzz,k,getspritefield(zzz,k)&zz); break;
                            case 5: setspritefield(zzz,k,getspritefield(zzz,k)^zz); break;
                        }
                    }
                    else if (k < 64)
                    {
                        switch(secop[j])
                        {
                            case 0: setsectorfield(l,k,zz); break;
                            case 1: setsectorfield(l,k,getsectorfield(l,k)+zz); break;
                            case 2: setsectorfield(l,k,getsectorfield(l,k)-zz); break;
                            case 3: setsectorfield(l,k,getsectorfield(l,k)|zz); break;
                            case 4: setsectorfield(l,k,getsectorfield(l,k)&zz); break;
                            case 5: setsectorfield(l,k,getsectorfield(l,k)^zz); break;
                        }
                    }
                }
            }

    for(z=0;z<dnumsectors;z++)
    {
        sector[z].ceilingpicnum = texturelookup[sector[z].ceilingpicnum];
        sector[z].floorpicnum = texturelookup[sector[z].floorpicnum];
    }
    for(z=0;z<numwalls;z++)
    {
        x = wall[wall[z].point2].x-wall[z].x;
        y = wall[wall[z].point2].y-wall[z].y;
        if ((klabs(x) >= 32768) || (klabs(y) >= 32768))
            wall[z].xrepeat = 255;
        else
        {
            zx = mulscale10(ksqrtasm(x*x+y*y),wall[z].yrepeat);
            wall[z].xrepeat = (char)min(max(zx,1),255);
        }

        wall[z].picnum = texturelookup[wall[z].picnum];
        wall[z].overpicnum = texturelookup[wall[z].overpicnum];
    }

    mapversion = 7; posx = 0; posy = 0; posz = 0; ang = 1536; cursectnum = 0;

        //WATCH OUT THAT FOR DNUMTHINGS BEING HIGHER THAN NUMBER ON DOOM MAP!
    for(i=0;i<dnumthings;i++)
        if (thing[i].type == 1)
        {
            posx = (((int)thing[i].x-cx)<<4);
            posy = (((int)-thing[i].y-cy)<<4);

            cursectnum = 0;
            for(zz=0;zz<dnumsectors;zz++)
                if (inside(posx,posy,(short)zz) != 0)
                    { cursectnum = zz; break; }

            posz = -(sect[cursectnum].floorz<<8) - (32<<8);
            ang = ((-((thing[i].ang<<11)/360))&2047);
            break;
        }

    //getch();

    //setvmode(0x3);

    strcpy(buildmap,doommap);
    if (strchr(buildmap,'.') == 0) strcat(buildmap,".map");
    if ((fil = Bopen(buildmap,BO_BINARY|BO_TRUNC|BO_CREAT|BO_WRONLY,BS_IREAD|BS_IWRITE)) == -1)
    {
        Bprintf("Could not write to %s\n",buildmap);
        exit(0);
    }
    Bwrite(fil,&mapversion,4);
    Bwrite(fil,&posx,4);
    Bwrite(fil,&posy,4);
    Bwrite(fil,&posz,4);
    Bwrite(fil,&ang,2);
    Bwrite(fil,&cursectnum,2);
    Bwrite(fil,&dnumsectors,2);
    Bwrite(fil,sector,sizeof(sectortype)*dnumsectors);
    Bwrite(fil,&numwalls,2);
    Bwrite(fil,wall,sizeof(walltype)*numwalls);
    Bwrite(fil,&dnumthings,2);
    Bwrite(fil,sprite,sizeof(spritetype)*dnumthings);
    Bclose(fil);

    Bprintf("Map converted.\n");

    return 0;
}


