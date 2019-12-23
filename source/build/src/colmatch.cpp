
#include "colmatch.h"

#define FASTPALCOLDEPTH 256
#define FASTPALRIGHTSHIFT 3
#define FASTPALRGBDIST (FASTPALCOLDEPTH*2+1)
static int32_t rdist[FASTPALRGBDIST], gdist[FASTPALRGBDIST], bdist[FASTPALRGBDIST];
#define FASTPALGRIDSIZ (FASTPALCOLDEPTH>>FASTPALRIGHTSHIFT)
static char colhere[((FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)+7)>>3];
static char colhead[(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)];
static int32_t colnext[256];
#define FASTPALCOLDIST (1<<FASTPALRIGHTSHIFT)
#define FASTPALCOLDISTMASK (FASTPALCOLDIST-1)
static uint8_t coldist[FASTPALCOLDIST];
static int32_t colscan[27];

static uint8_t const * colmatch_palette;

#define pow2char(x) (1u << (x))

//
// paletteInitClosestColor
//
void paletteInitClosestColorScale(int32_t rscale, int32_t gscale, int32_t bscale)
{
    int32_t j = 0;
    for (bssize_t i=256; i>=0; i--)
    {
        //j = (i-64)*(i-64);
        rdist[i] = rdist[FASTPALCOLDEPTH*2-i] = j*rscale;
        gdist[i] = gdist[FASTPALCOLDEPTH*2-i] = j*gscale;
        bdist[i] = bdist[FASTPALCOLDEPTH*2-i] = j*bscale;
        j += FASTPALRGBDIST-(i<<1);
    }
}
void paletteInitClosestColorMap(uint8_t const * const pal)
{
    Bmemset(colhere,0,sizeof(colhere));
    Bmemset(colhead,0,sizeof(colhead));

    colmatch_palette = pal;

    char const *pal1 = (char const *)&pal[768-3];
    for (bssize_t i=255; i>=0; i--,pal1-=3)
    {
        int32_t const j = (pal1[0]>>FASTPALRIGHTSHIFT)*FASTPALGRIDSIZ*FASTPALGRIDSIZ
            + (pal1[1]>>FASTPALRIGHTSHIFT)*FASTPALGRIDSIZ + (pal1[2]>>FASTPALRIGHTSHIFT)
            + FASTPALGRIDSIZ*FASTPALGRIDSIZ + FASTPALGRIDSIZ + 1;
        if (colhere[j>>3]&pow2char(j&7)) colnext[i] = colhead[j]; else colnext[i] = -1;
        colhead[j] = i;
        colhere[j>>3] |= pow2char(j&7);
    }

    paletteFlushClosestColor();
}
void paletteInitClosestColorGrid(void)
{
    int i = 0;
    int32_t x, y, z;
    for (x=-FASTPALGRIDSIZ*FASTPALGRIDSIZ; x<=FASTPALGRIDSIZ*FASTPALGRIDSIZ; x+=FASTPALGRIDSIZ*FASTPALGRIDSIZ)
        for (y=-FASTPALGRIDSIZ; y<=FASTPALGRIDSIZ; y+=FASTPALGRIDSIZ)
            for (z=-1; z<=1; z++)
                colscan[i++] = x+y+z;
    i = colscan[13]; colscan[13] = colscan[26]; colscan[26] = i;

    for (i = 0; i < FASTPALCOLDIST/2; i++)
        coldist[i] = i;
    for (; i < FASTPALCOLDIST; i++)
        coldist[i] = FASTPALCOLDIST-i;
}

#define COLRESULTSIZ 4096

static uint32_t colmatchresults[COLRESULTSIZ];
static int32_t numcolmatchresults;

void paletteFlushClosestColor(void)
{
    Bmemset(colmatchresults, 0, COLRESULTSIZ * sizeof(uint32_t));
    numcolmatchresults = 0;
}

#define checkbitfield(field, idx) ((field)[(idx)>>3] & (1u<<((idx)&7)))

// Finds a color index in [0 .. lastokcol] closest to (r, g, b).
// <lastokcol> must be in [0 .. 255].
int32_t paletteGetClosestColorWithBlacklist(int32_t const r, int32_t const g, int32_t const b, int32_t const lastokcol, uint8_t const * const blacklist)
{
#ifdef DEBUGGINGAIDS
    Bassert(lastokcol >= 0 && lastokcol <= 255);
#endif

    uint32_t const col = r | (g<<8) | (b<<16);

    int mindist = -1;

    int const k = (numcolmatchresults > COLRESULTSIZ) ? COLRESULTSIZ : numcolmatchresults;

    if (!numcolmatchresults) goto skip;

    if (col == (colmatchresults[(numcolmatchresults-1) & (COLRESULTSIZ-1)] & 0x00ffffff))
        return colmatchresults[(numcolmatchresults-1) & (COLRESULTSIZ-1)]>>24;

    int i;

    for (i = 0; i <= k-4; i+=4)
    {
        if (col == (colmatchresults[i]   & 0x00ffffff)) { mindist = i; break; }
        if (col == (colmatchresults[i+1] & 0x00ffffff)) { mindist = i+1; break; }
        if (col == (colmatchresults[i+2] & 0x00ffffff)) { mindist = i+2; break; }
        if (col == (colmatchresults[i+3] & 0x00ffffff)) { mindist = i+3; break; }
    }

    if (mindist == -1)
    for (; i < k; i++)
        if (col == (colmatchresults[i] & 0x00ffffff)) { mindist = i; break; }

    if (mindist != -1)
    {
        uint32_t const idx = colmatchresults[mindist]>>24;
        if (idx <= (unsigned)lastokcol && (blacklist == nullptr || !checkbitfield(blacklist, idx)))
            return idx;
    }

skip:
    i = paletteGetClosestColorWithBlacklistNoCache(r, g, b, lastokcol, blacklist);
    colmatchresults[numcolmatchresults++ & (COLRESULTSIZ-1)] = col | (i << 24);
    return i;
}

int32_t paletteGetClosestColorWithBlacklistNoCache(int32_t r, int32_t g, int32_t b, int32_t const lastokcol, uint8_t const * const blacklist)
{
#ifdef DEBUGGINGAIDS
    Bassert(lastokcol >= 0 && lastokcol <= 255);
#endif

    int const j = (r>>FASTPALRIGHTSHIFT)*FASTPALGRIDSIZ*FASTPALGRIDSIZ
        + (g>>FASTPALRIGHTSHIFT)*FASTPALGRIDSIZ + (b>>FASTPALRIGHTSHIFT)
        + FASTPALGRIDSIZ*FASTPALGRIDSIZ
        + FASTPALGRIDSIZ + 1;

    int const minrdist = rdist[coldist[r&FASTPALCOLDISTMASK]+FASTPALCOLDEPTH];
    int const mingdist = gdist[coldist[g&FASTPALCOLDISTMASK]+FASTPALCOLDEPTH];
    int const minbdist = bdist[coldist[b&FASTPALCOLDISTMASK]+FASTPALCOLDEPTH];

    int mindist = min(minrdist, mingdist);
    mindist = min(mindist, minbdist) + 1;

    r = FASTPALCOLDEPTH-r, g = FASTPALCOLDEPTH-g, b = FASTPALCOLDEPTH-b;

    int retcol = -1;

    for (bssize_t k=26; k>=0; k--)
    {
        int i = colscan[k]+j;

        if ((colhere[i>>3]&pow2char(i&7)) == 0)
            continue;

        i = colhead[i];

        do
        {
            char const * const pal1 = (char const *)&colmatch_palette[i*3];
            int dist = gdist[pal1[1]+g];

            if (dist >= mindist || i > lastokcol || (blacklist != nullptr && checkbitfield(blacklist, i))) continue;
            if ((dist += rdist[pal1[0]+r]) >= mindist) continue;
            if ((dist += bdist[pal1[2]+b]) >= mindist) continue;

            mindist = dist;
            retcol = i;
        }
        while ((i = colnext[i]) >= 0);
    }

    if (retcol >= 0)
        return retcol;

    mindist = INT32_MAX;

    for (bssize_t i = 0; i <= lastokcol; ++i)
    {
        if (blacklist != nullptr && checkbitfield(blacklist, i))
            continue;

        char const * const pal1 = (char const *)&colmatch_palette[i*3];
        int dist = gdist[pal1[1]+g];

        if (dist >= mindist) continue;
        if ((dist += rdist[pal1[0]+r]) >= mindist) continue;
        if ((dist += bdist[pal1[2]+b]) >= mindist) continue;

        mindist = dist;
        retcol = i;
    }

    return retcol;
}
