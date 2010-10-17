/**************************************************************************************************
"POLYMOST" code written by Ken Silverman
Ken Silverman's official web site: http://www.advsys.net/ken

Motivation:
When 3D Realms released the Duke Nukem 3D source code, I thought somebody would do a OpenGL or
Direct3D port. Well, after a few months passed, I saw no sign of somebody working on a true
hardware-accelerated port of Build, just people saying it wasn't possible. Eventually, I realized
the only way this was going to happen was for me to do it myself. First, I needed to port Build to
Windows. I could have done it myself, but instead I thought I'd ask my Australian buddy, Jonathon
Fowler, if he would upgrade his Windows port to my favorite compiler (MSVC) - which he did. Once
that was done, I was ready to start the "POLYMOST" project.

About:
This source file is basically a complete rewrite of the entire rendering part of the Build engine.
There are small pieces in ENGINE.C to activate this code, and other minor hacks in other source
files, but most of it is in here. If you're looking for polymost-related code in the other source
files, you should find most of them by searching for either "polymost" or "rendmode". Speaking of
rendmode, there are now 4 rendering modes in Build:

    rendmode 0: The original code I wrote from 1993-1997
    rendmode 1: Solid-color rendering: my debug code before I did texture mapping
    rendmode 2: Software rendering before I started the OpenGL code (Note: this is just a quick
                        hack to make testing easier - it's not optimized to my usual standards!)
    rendmode 3: The OpenGL code

The original Build engine did hidden surface removal by using a vertical span buffer on the tops
and bottoms of walls. This worked nice back in the day, but it it's not suitable for a polygon
engine. So I decided to write a brand new hidden surface removal algorithm - using the same idea
as the original Build - but one that worked with vectors instead of already rasterized data.

Brief history:
06/20/2000: I release Build Source code
04/01/2003: 3D Realms releases Duke Nukem 3D source code
10/04/2003: Jonathon Fowler gets his Windows port working in Visual C
10/04/2003: I start writing POLYMOST.BAS, a new hidden surface removal algorithm for Build that
                    works on a polygon level instead of spans.
10/16/2003: Ported POLYMOST.BAS to C inside JonoF KenBuild's ENGINE.C; later this code was split
                    out of ENGINE.C and put in this file, POLYMOST.C.
12/10/2003: Started OpenGL code for POLYMOST (rendmode 3)
12/23/2003: 1st public release
01/01/2004: 2nd public release: fixed stray lines, status bar, mirrors, sky, and lots of other bugs.

----------------------------------------------------------------------------------------------------

Todo list (in approximate chronological order):

High priority:
    *   BOTH: Do accurate software sorting/chopping for sprites: drawing in wrong order is bad :/
    *   BOTH: Fix hall of mirrors near "zenith". Call polymost_drawrooms twice?
    * OPENGL: drawmapview()

Low priority:
    * SOFT6D: Do back-face culling of sprites during up/down/tilt transformation (top of drawpoly)
    * SOFT6D: Fix depth shading: use saturation&LUT
    * SOFT6D: Optimize using hyperbolic mapping (similar to KUBE algo)
    * SOFT6D: Slab6-style voxel sprites. How to accelerate? :/
    * OPENGL: KENBUILD: Write flipping code for floor mirrors
    *   BOTH: KENBUILD: Parallaxing sky modes 1&2
    *   BOTH: Masked/1-way walls don't clip correctly to sectors of intersecting ceiling/floor slopes
    *   BOTH: Editart x-center is not working correctly with Duke's camera/turret sprites
    *   BOTH: Get rid of horizontal line above Duke full-screen status bar
    *   BOTH: Combine ceilings/floors into a single triangle strip (should lower poly count by 2x)
    *   BOTH: Optimize/clean up texture-map setup equations

**************************************************************************************************/


#ifdef POLYMOST

#include "compat.h"
#include "build.h"

#ifdef USE_OPENGL
# include "glbuild.h"
# include "mdsprite.h"
#endif

#include "pragmas.h"
#include "baselayer.h"
#include "osd.h"
#include "engine_priv.h"
#include "hightile.h"
#include "polymost.h"
#include "polymer.h"
#include "scriptfile.h"
#include "cache1d.h"
#include "kplib.h"

#ifndef _WIN32
extern int32_t filelength(int h); // kplib.c
#endif

extern char textfont[2048], smalltextfont[2048];

int32_t rendmode=0;
int32_t usemodels=1, usehightile=1;

#include <math.h> //<-important!
typedef struct { float x, cy[2], fy[2]; int32_t n, p, tag, ctag, ftag; } vsptyp;
#define VSPMAX 4096 //<- careful!
static vsptyp vsp[VSPMAX];
static int32_t vcnt, gtag;

static double dxb1[MAXWALLSB], dxb2[MAXWALLSB];

#define SCISDIST 1.0 //1.0: Close plane clipping distance
#define USEZBUFFER 1 //1:use zbuffer (slow, nice sprite rendering), 0:no zbuffer (fast, bad sprite rendering)
#define LINTERPSIZ 4 //log2 of interpolation size. 4:pretty fast&acceptable quality, 0:best quality/slow!
#define DEPTHDEBUG 0 //1:render distance instead of texture, for debugging only!, 0:default

float shadescale = 1.3f;

double gyxscale, gxyaspect, gviewxrange, ghalfx, grhalfxdown10, grhalfxdown10x, ghoriz;
double gcosang, gsinang, gcosang2, gsinang2;
double gchang, gshang, gctang, gstang, gvisibility;
float gtang = 0.0;
double guo, gux, guy; //Screen-based texture mapping parameters
double gvo, gvx, gvy;
double gdo, gdx, gdy;

static int32_t preview_mouseaim=0;  // when 1, displays a CROSSHAIR tsprite at the _real_ aimed position

#if (USEZBUFFER != 0)
int32_t  zbufysiz = 0, zbufbpl = 0, *zbufoff = 0;
intptr_t zbufmem = 0;
#endif

#ifdef USE_OPENGL
static int32_t srepeat = 0, trepeat = 0;

int32_t glredbluemode = 0;
static int32_t lastglredbluemode = 0, redblueclearcnt = 0;

struct glfiltermodes glfiltermodes[numglfiltermodes] =
{
    {"GL_NEAREST",GL_NEAREST,GL_NEAREST},
    {"GL_LINEAR",GL_LINEAR,GL_LINEAR},
    {"GL_NEAREST_MIPMAP_NEAREST",GL_NEAREST_MIPMAP_NEAREST,GL_NEAREST},
    {"GL_LINEAR_MIPMAP_NEAREST",GL_LINEAR_MIPMAP_NEAREST,GL_LINEAR},
    {"GL_NEAREST_MIPMAP_LINEAR",GL_NEAREST_MIPMAP_LINEAR,GL_NEAREST},
    {"GL_LINEAR_MIPMAP_LINEAR",GL_LINEAR_MIPMAP_LINEAR,GL_LINEAR}
};

int32_t glanisotropy = 1;            // 0 = maximum supported by card
int32_t glusetexcompr = 1;
int32_t gltexfiltermode = 2; // GL_NEAREST_MIPMAP_NEAREST
int32_t glusetexcache = 2;
int32_t glmultisample = 0, glnvmultisamplehint = 0;
int32_t gltexmaxsize = 0;      // 0 means autodetection on first run
int32_t gltexmiplevel = 0;		// discards this many mipmap levels
static int32_t lastglpolygonmode = 0; //FUK
int32_t glpolygonmode = 0;     // 0:GL_FILL,1:GL_LINE,2:GL_POINT //FUK
int32_t glwidescreen = 0;
int32_t glprojectionhacks = 1;
static GLuint polymosttext = 0;
extern char nofog;
int32_t glrendmode = 3;

// Those THREE globals control the drawing of fullbright tiles
static int32_t fullbrightloadingpass = 0;
static int32_t fullbrightdrawingpass = 0;
static int32_t shadeforfullbrightpass;

float curpolygonoffset;    // internal polygon offset stack for drawing flat sprites to avoid depth fighting

// Detail mapping cvar
int32_t r_detailmapping = 1;

// Glow mapping cvar
int32_t r_glowmapping = 1;

// Vertex Array model drawing cvar
int32_t r_vertexarrays = 1;

// Vertex Buffer Objects model drawing cvars
int32_t r_vbos = 1;
int32_t r_vbocount = 64;

// model animation smoothing cvar
int32_t r_animsmoothing = 1;

// line of sight checks before mddraw()
int32_t r_modelocclusionchecking = 0;

// fullbright cvar
int32_t r_fullbrights = 1;

// texture downsizing
// is medium quality a good default?
int32_t r_downsize = 1;
int32_t r_downsizevar = -1;

// used for fogcalc
float fogresult, fogcol[4], fogtable[4*MAXPALOOKUPS];
#endif

static char ptempbuf[MAXWALLSB<<1];

// polymost ART sky control
int32_t r_parallaxskyclamping = 1;
int32_t r_parallaxskypanning = 0;

extern int16_t editstatus;

static inline int32_t imod(int32_t a, int32_t b)
{
    if (a >= 0) return(a%b);
    return(((a+1)%b)+b-1);
}

void drawline2d(float x0, float y0, float x1, float y1, char col)
{
    float f, dx, dy, fxres, fyres;
    int32_t e, inc, x, y;
    uint32_t up16;

    dx = x1-x0; dy = y1-y0; if ((dx == 0) && (dy == 0)) return;
    fxres = (float)xdimen; fyres = (float)ydimen;
    if (x0 >= fxres) { if (x1 >= fxres) return; y0 += (fxres-x0)*dy/dx; x0 = fxres; }
    else if (x0 <      0) { if (x1 <      0) return; y0 += (0-x0)*dy/dx; x0 =     0; }
    if (x1 >= fxres) {                          y1 += (fxres-x1)*dy/dx; x1 = fxres; }
    else if (x1 <      0) {                          y1 += (0-x1)*dy/dx; x1 =     0; }
    if (y0 >= fyres) { if (y1 >= fyres) return; x0 += (fyres-y0)*dx/dy; y0 = fyres; }
    else if (y0 <      0) { if (y1 <      0) return; x0 += (0-y0)*dx/dy; y0 =     0; }
    if (y1 >= fyres) {                          x1 += (fyres-y1)*dx/dy; y1 = fyres; }
    else if (y1 <      0) {                          x1 += (0-y1)*dx/dy; y1 =     0; }

    if (fabs(dx) > fabs(dy))
    {
        if (x0 > x1) { f = x0; x0 = x1; x1 = f; f = y0; y0 = y1; y1 = f; }
        y = (int32_t)(y0*65536.f)+32768;
        inc = (int32_t)(dy/dx*65536.f+.5f);
        x = (int32_t)(x0+.5); if (x < 0) { y -= inc*x; x = 0; } //if for safety
        e = (int32_t)(x1+.5); if (e > xdimen) e = xdimen;       //if for safety
        up16 = (ydimen<<16);
        for (; x<e; x++,y+=inc) if ((uint32_t)y < up16) *(char *)(ylookup[y>>16]+x+frameoffset) = col;
    }
    else
    {
        if (y0 > y1) { f = x0; x0 = x1; x1 = f; f = y0; y0 = y1; y1 = f; }
        x = (int32_t)(x0*65536.f)+32768;
        inc = (int32_t)(dx/dy*65536.f+.5f);
        y = (int32_t)(y0+.5); if (y < 0) { x -= inc*y; y = 0; } //if for safety
        e = (int32_t)(y1+.5); if (e > ydimen) e = ydimen;       //if for safety
        up16 = (xdimen<<16);
        for (; y<e; y++,x+=inc) if ((uint32_t)x < up16) *(char *)(ylookup[y]+(x>>16)+frameoffset) = col;
    }
}

#ifdef USE_OPENGL
#include "md4.h"
#include "quicklz.h"

//--------------------------------------------------------------------------------------------------
//TEXTURE MANAGEMENT: treats same texture with different .PAL as a separate texture. This makes the
//   max number of virtual textures very large (MAXTILES*256). Instead of allocating a handle for
//   every virtual texture, I use a cache where indexing is managed through a hash table.
//

// moved into polymost.h
/*typedef struct pthtyp_t
{
    struct pthtyp_t *next;
    GLuint glpic;
    short picnum;
    char palnum;
    char effects;
    char flags;      // 1 = clamped (dameth&4), 2 = hightile, 4 = skybox face, 8 = hasalpha, 16 = hasfullbright, 128 = invalidated
    char skyface;
    hicreplctyp *hicr;

    unsigned short sizx, sizy;
    float scalex, scaley;
    struct pthtyp_t *wofb; // without fullbright
    struct pthtyp_t *ofb; // only fullbright
} pthtyp;*/

int32_t cachefilehandle = -1; // texture cache file handle
FILE *cacheindexptr = NULL;
uint8_t *memcachedata = NULL;
int32_t memcachesize = -1;
int32_t cachepos = 0;

hashtable_t h_texcache    = { 1024, NULL };

char TEXCACHEFILE[BMAX_PATH] = "textures";

int32_t mdtims, omdtims;
float alphahackarray[MAXTILES];

texcacheindex *firstcacheindex = NULL;
texcacheindex *curcacheindex = NULL;
texcacheindex *cacheptrs[MAXTILES<<1];
int32_t numcacheentries = 0;


#define GLTEXCACHEADSIZ 8192
pthtyp *gltexcachead[GLTEXCACHEADSIZ];

int32_t drawingskybox = 0;

int32_t gloadtile_art(int32_t,int32_t,int32_t,pthtyp*,int32_t);
int32_t gloadtile_hi(int32_t,int32_t,int32_t,hicreplctyp*,int32_t,pthtyp*,int32_t,char);
static int32_t hicprecaching = 0;
pthtyp * gltexcache(int32_t dapicnum, int32_t dapalnum, int32_t dameth)
{
    int32_t i, j;
    hicreplctyp *si;
    pthtyp *pth, *pth2;

    j = (dapicnum&(GLTEXCACHEADSIZ-1));

    si = usehightile ? hicfindsubst(dapicnum,dapalnum,drawingskybox) : NULL;

    if (!si)
    {
        if (drawingskybox || dapalnum >= (MAXPALOOKUPS - RESERVEDPALS)) return NULL;
        goto tryart;
    }

    /* if palette > 0 && replacement found
     *    no effects are applied to the texture
     * else if palette > 0 && no replacement found
     *    effects are applied to the palette 0 texture if it exists
     */

    // load a replacement
    for (pth=gltexcachead[j]; pth; pth=pth->next)
    {
        if (pth->picnum == dapicnum &&
                pth->palnum == si->palnum &&
                (si->palnum>0 ? 1 : (pth->effects == hictinting[dapalnum].f)) &&
                (pth->flags & (1+2+4)) == (((dameth&4)>>2)+2+((drawingskybox>0)<<2)) &&
                (drawingskybox>0 ? (pth->skyface == drawingskybox) : 1)
           )
        {
            if (pth->flags & 128)
            {
                pth->flags &= ~128;
                if (gloadtile_hi(dapicnum,dapalnum,drawingskybox,si,dameth,pth,0,
                                 (si->palnum>0) ? 0 : hictinting[dapalnum].f))    // reload tile
                {
                    if (drawingskybox) return NULL;
                    goto tryart;   // failed, so try for ART
                }
            }
            return(pth);
        }
    }


    pth = (pthtyp *)Bcalloc(1,sizeof(pthtyp));
    if (!pth) return NULL;

    // possibly fetch an already loaded multitexture :_)
    if (dapalnum >= (MAXPALOOKUPS - RESERVEDPALS))
        for (i = (GLTEXCACHEADSIZ - 1); i >= 0; i--)
            for (pth2=gltexcachead[i]; pth2; pth2=pth2->next)
            {
                if ((pth2->hicr) && (pth2->hicr->filename) && (Bstrcasecmp(pth2->hicr->filename, si->filename) == 0))
                {
                    Bmemcpy(pth, pth2, sizeof(pthtyp));
                    pth->picnum = dapicnum;
                    pth->flags = ((dameth&4)>>2) + 2 + ((drawingskybox>0)<<2);
                    if (pth2->flags & 8) pth->flags |= 8; //hasalpha
                    pth->hicr = si;
                    pth->next = gltexcachead[j];

                    gltexcachead[j] = pth;
                    return(pth);
                }
            }

    if (gloadtile_hi(dapicnum,dapalnum,drawingskybox,si,dameth,pth,1, (si->palnum>0) ? 0 : hictinting[dapalnum].f))
    {
        Bfree(pth);
        if (drawingskybox) return NULL;
        goto tryart;   // failed, so try for ART
    }
    pth->palnum = si->palnum;
    pth->next = gltexcachead[j];
    gltexcachead[j] = pth;
    return(pth);

tryart:
    if (hicprecaching) return NULL;

    // load from art
    for (pth=gltexcachead[j]; pth; pth=pth->next)
        if (pth->picnum == dapicnum &&
                pth->palnum == dapalnum &&
                (pth->flags & (1+2)) == ((dameth&4)>>2)
           )
        {
            if (pth->flags & 128)
            {
                pth->flags &= ~128;
                if (gloadtile_art(dapicnum,dapalnum,dameth,pth,0)) return NULL; //reload tile (for animations)
            }
            return(pth);
        }

    pth = (pthtyp *)Bcalloc(1,sizeof(pthtyp));
    if (!pth) return NULL;

    if (gloadtile_art(dapicnum,dapalnum,dameth,pth,1))
    {
        Bfree(pth);
        return NULL;
    }
    pth->next = gltexcachead[j];
    gltexcachead[j] = pth;

    return(pth);
}

static inline int32_t gltexmayhavealpha(int32_t dapicnum, int32_t dapalnum)
{
    int32_t j = (dapicnum&(GLTEXCACHEADSIZ-1));
    pthtyp *pth;

    for (pth=gltexcachead[j]; pth; pth=pth->next)
        if ((pth->picnum == dapicnum) && (pth->palnum == dapalnum))
            return((pth->flags&8) != 0);
    return(1);
}

void gltexinvalidate(int32_t dapicnum, int32_t dapalnum, int32_t dameth)
{
    int32_t j;
    pthtyp *pth;

    j = (dapicnum&(GLTEXCACHEADSIZ-1));
    for (pth=gltexcachead[j]; pth; pth=pth->next)
        if (pth->picnum == dapicnum && pth->palnum == dapalnum && (pth->flags & 1) == ((dameth&4)>>2))
        {
            pth->flags |= 128;
            if (pth->flags & 16)
                pth->ofb->flags |= 128;
        }
}

//Make all textures "dirty" so they reload, but not re-allocate
//This should be much faster than polymost_glreset()
//Use this for palette effects ... but not ones that change every frame!
void gltexinvalidateall()
{
    int32_t j;
    pthtyp *pth;

    for (j=GLTEXCACHEADSIZ-1; j>=0; j--)
        for (pth=gltexcachead[j]; pth; pth=pth->next)
        {
            pth->flags |= 128;
            if (pth->flags & 16)
                pth->ofb->flags |= 128;
        }
    clearskins();
#ifdef DEBUGGINGAIDS
    OSD_Printf("gltexinvalidateall()\n");
#endif
}

void gltexinvalidate8()
{
    int32_t j;
    pthtyp *pth;

    for (j=GLTEXCACHEADSIZ-1; j>=0; j--)
        for (pth=gltexcachead[j]; pth; pth=pth->next)
        {
            if (pth->hicr == NULL)
            {
                pth->flags |= 128;
                if (pth->flags & 16)
                    pth->ofb->flags |= 128;
            }
        }
#ifdef DEBUGGINGAIDS
    OSD_Printf("gltexinvalidate8()\n");
#endif
}

void gltexapplyprops(void)
{
    int32_t i;
    pthtyp *pth;

    if (glinfo.maxanisotropy > 1.0)
    {
        if (glanisotropy <= 0 || glanisotropy > glinfo.maxanisotropy) glanisotropy = (int32_t)glinfo.maxanisotropy;
    }

    if (gltexfiltermode < 0) gltexfiltermode = 0;
    else if (gltexfiltermode >= (int32_t)numglfiltermodes) gltexfiltermode = numglfiltermodes-1;
    for (i=GLTEXCACHEADSIZ-1; i>=0; i--)
    {
        for (pth=gltexcachead[i]; pth; pth=pth->next)
        {
            bglBindTexture(GL_TEXTURE_2D,pth->glpic);
            bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,glfiltermodes[gltexfiltermode].mag);
            bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,glfiltermodes[gltexfiltermode].min);
            if (glinfo.maxanisotropy > 1.0)
                bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,glanisotropy);
            if (r_fullbrights && pth->flags & 16)
            {
                bglBindTexture(GL_TEXTURE_2D,pth->ofb->glpic);
                bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,glfiltermodes[gltexfiltermode].mag);
                bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,glfiltermodes[gltexfiltermode].min);
                if (glinfo.maxanisotropy > 1.0)
                    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,glanisotropy);
            }
        }
    }

    {
        int32_t j;
        mdskinmap_t *sk;
        md2model_t *m;

        for (i=0; i<nextmodelid; i++)
        {
            m = (md2model_t *)models[i];
            if (m->mdnum < 2) continue;
            for (j=0; j<m->numskins*(HICEFFECTMASK+1); j++)
            {
                if (!m->texid[j]) continue;
                bglBindTexture(GL_TEXTURE_2D,m->texid[j]);
                bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,glfiltermodes[gltexfiltermode].mag);
                bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,glfiltermodes[gltexfiltermode].min);
                if (glinfo.maxanisotropy > 1.0)
                    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,glanisotropy);
            }

            for (sk=m->skinmap; sk; sk=sk->next)
                for (j=0; j<(HICEFFECTMASK+1); j++)
                {
                    if (!sk->texid[j]) continue;
                    bglBindTexture(GL_TEXTURE_2D,sk->texid[j]);
                    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,glfiltermodes[gltexfiltermode].mag);
                    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,glfiltermodes[gltexfiltermode].min);
                    if (glinfo.maxanisotropy > 1.0)
                        bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,glanisotropy);
                }
        }
    }
}

//--------------------------------------------------------------------------------------------------
static int32_t LoadCacheOffsets(void);
float glox1, gloy1, glox2, gloy2;

//Use this for both initialization and uninitialization of OpenGL.
static int32_t gltexcacnum = -1;
extern void freevbos(void);

void polymost_cachesync(void)
{
    if (memcachedata && cachefilehandle != -1 && filelength(cachefilehandle) > memcachesize)
    {
        size_t len = filelength(cachefilehandle);
        initprintf("Syncing memcache to texcache\n");
        memcachedata = (uint8_t *)Brealloc(memcachedata, len);
        Blseek(cachefilehandle, memcachesize, BSEEK_SET);
        Bread(cachefilehandle, memcachedata + memcachesize, len - memcachesize);
        memcachesize = len;
    }
}


void polymost_glreset()
{
    int32_t i;
    pthtyp *pth, *next;

    for (i=MAXPALOOKUPS-1; i>=0; i--)
    {
        fogtable[i<<2] = palookupfog[i].r / 63.f;
        fogtable[(i<<2)+1] = palookupfog[i].g / 63.f;
        fogtable[(i<<2)+2] = palookupfog[i].b / 63.f;
        fogtable[(i<<2)+3] = 0;
    }

    //Reset if this is -1 (meaning 1st texture call ever), or > 0 (textures in memory)
    if (gltexcacnum < 0)
    {
        gltexcacnum = 0;

        //Hack for polymost_dorotatesprite calls before 1st polymost_drawrooms()
        gcosang = gcosang2 = ((double)16384)/262144.0;
        gsinang = gsinang2 = ((double)    0)/262144.0;
    }
    else
    {
        for (i=GLTEXCACHEADSIZ-1; i>=0; i--)
        {
            for (pth=gltexcachead[i]; pth;)
            {
                next = pth->next;
                if (pth->flags & 16) // fullbright textures
                {
                    bglDeleteTextures(1,&pth->ofb->glpic);
                    Bfree(pth->ofb);
                }
                bglDeleteTextures(1,&pth->glpic);
                Bfree(pth);
                pth = next;
            }
            gltexcachead[i] = NULL;
        }
        clearskins();
    }

    if (polymosttext) bglDeleteTextures(1,&polymosttext);
    polymosttext=0;

    freevbos();

    memset(gltexcachead,0,sizeof(gltexcachead));
    glox1 = -1;

    /*
        if (cachefilehandle != -1)
        {
            Bclose(cachefilehandle);
            cachefilehandle = -1;
        }

        if (cacheindexptr)
        {
            Bfclose(cacheindexptr);
            cacheindexptr = NULL;
        }
    */

    for (i = numcacheentries-1; i >= 0; i--)
        if (cacheptrs[i])
        {
            int32_t ii;
            for (ii = numcacheentries-1; ii >= 0; ii--)
                if (i != ii && cacheptrs[ii] == cacheptrs[i])
                {
                    /*OSD_Printf("removing duplicate cacheptr %d\n",ii);*/
                    cacheptrs[ii] = NULL;
                }

                Bfree(cacheptrs[i]);
                cacheptrs[i] = NULL;
        }

    polymost_cachesync();
}

// one-time initialization of OpenGL for polymost
void polymost_glinit()
{
    GLfloat col[4];
    int32_t     i;

    if (!Bstrcmp(glinfo.vendor, "NVIDIA Corporation"))
    {
        bglHint(GL_FOG_HINT,GL_NICEST);
    }
    else
    {
        bglHint(GL_FOG_HINT,GL_DONT_CARE);
    }

    bglFogi(GL_FOG_MODE, GL_EXP2);
    bglFogf(GL_FOG_DENSITY,1.0); //must be > 0, default is 1
    /*    bglFogf(GL_FOG_START,0.0); //default is 0
        bglFogf(GL_FOG_END,1.0); //default is 1 */
    col[0] = 0; col[1] = 0; col[2] = 0; col[3] = 0; //range:0 to 1
    bglFogfv(GL_FOG_COLOR,col); //default is 0,0,0,0

    bglBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    //bglHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    //bglEnable(GL_LINE_SMOOTH);

    if (glmultisample > 0 && glinfo.multisample)
    {
        if (glinfo.nvmultisamplehint)
            bglHint(GL_MULTISAMPLE_FILTER_HINT_NV, glnvmultisamplehint ? GL_NICEST:GL_FASTEST);
        bglEnable(GL_MULTISAMPLE_ARB);
    }

    if (r_detailmapping && (!glinfo.multitex || !glinfo.envcombine))
    {
        OSD_Printf("Your OpenGL implementation doesn't support detail mapping. Disabling...\n");
        r_detailmapping = 0;
    }

    if (r_glowmapping && (!glinfo.multitex || !glinfo.envcombine))
    {
        OSD_Printf("Your OpenGL implementation doesn't support glow mapping. Disabling...\n");
        r_glowmapping = 0;
    }

    if (r_vbos && (!glinfo.vbos))
    {
        OSD_Printf("Your OpenGL implementation doesn't support Vertex Buffer Objects. Disabling...\n");
        r_vbos = 0;
    }

    bglEnableClientState(GL_VERTEX_ARRAY);
    bglEnableClientState(GL_TEXTURE_COORD_ARRAY);

    if (cachefilehandle != -1)
    {
        Bclose(cachefilehandle);
        cachefilehandle = -1;
    }

    if (cacheindexptr)
    {
        Bfclose(cacheindexptr);
        cacheindexptr = NULL;
    }

    if (memcachedata)
    {
        Bfree(memcachedata);
        memcachedata = NULL;
        memcachesize = -1;
    }

    for (i = numcacheentries-1; i >= 0; i--)
        if (cacheptrs[i])
        {
            int32_t ii;
            for (ii = numcacheentries-1; ii >= 0; ii--)
                if (i != ii && cacheptrs[ii] == cacheptrs[i])
                {
                    /*OSD_Printf("removing duplicate cacheptr %d\n",ii);*/
                    cacheptrs[ii] = NULL;
                }

            Bfree(cacheptrs[i]);
            cacheptrs[i] = NULL;
        }

    curcacheindex = firstcacheindex = (texcacheindex *)Bcalloc(1, sizeof(texcacheindex));
    numcacheentries = 0;

//    Bmemset(&firstcacheindex, 0, sizeof(texcacheindex));
//    Bmemset(&cacheptrs[0], 0, sizeof(cacheptrs));

    hash_init(&h_texcache);
    LoadCacheOffsets();

    Bstrcpy(ptempbuf,TEXCACHEFILE);
    Bstrcat(ptempbuf,".cache");
    cacheindexptr = Bfopen(ptempbuf, "at+");
    if (!cacheindexptr)
    {
        glusetexcache = 0;
        initprintf("Unable to open cache index: %s\n",strerror(errno));
        return;
    }

    fseek(cacheindexptr, 0, BSEEK_END);
    if (!ftell(cacheindexptr))
    {
        rewind(cacheindexptr);
        Bfprintf(cacheindexptr,"// automatically generated by EDuke32, DO NOT MODIFY!\n");
    }
    else rewind(cacheindexptr);

    cachefilehandle = Bopen(TEXCACHEFILE, BO_BINARY|BO_CREAT|BO_APPEND|BO_RDWR, BS_IREAD|BS_IWRITE);

    if (cachefilehandle < 0)
    {
        initprintf("Unable to open cache file: %s\n",strerror(errno));
        glusetexcache = 0;
        return;
    }

    memcachesize = filelength(cachefilehandle);

    if (memcachesize > 0)
    {
        memcachedata = (uint8_t *)Brealloc(memcachedata, memcachesize);

        if (!memcachedata)
        {
            initprintf("Failed allocating %d bytes for memcache\n", memcachesize);
            memcachesize = -1;
        }

        if (Bread(cachefilehandle, memcachedata, memcachesize) != memcachesize)
        {
            initprintf("Failed reading texcache into memcache!\n");
            Bfree(memcachedata);
            memcachesize = -1;
        }
    }

    i = 0;

    curcacheindex = firstcacheindex;
    while (curcacheindex->next)
    {
        i += curcacheindex->len;
        curcacheindex = curcacheindex->next;
    }

    i = Blseek(cachefilehandle, 0, BSEEK_END)-i;
    if (i)
        initprintf("Cache contains %d bytes of garbage data\n",i);
//    Blseek(cachefilehandle, 0, BSEEK_SET);
}

void invalidatecache(void)
{
    int32_t i;

    r_downsizevar = r_downsize; // update the cvar representation when the menu changes r_downsize

    polymost_glreset();

    if (cachefilehandle != -1)
    {
        Bclose(cachefilehandle);
        cachefilehandle = -1;
    }

    if (cacheindexptr)
    {
        Bfclose(cacheindexptr);
        cacheindexptr = NULL;
    }

    if (memcachedata)
    {
        Bfree(memcachedata);
        memcachedata = NULL;
        memcachesize = -1;
    }

    for (i = numcacheentries-1; i >= 0; i--)
        if (cacheptrs[i])
        {
            int32_t ii;
            for (ii = numcacheentries-1; ii >= 0; ii--)
                if (i != ii && cacheptrs[ii] == cacheptrs[i])
                {
                    /*OSD_Printf("removing duplicate cacheptr %d\n",ii);*/
                    cacheptrs[ii] = NULL;
                }

            Bfree(cacheptrs[i]);
            cacheptrs[i] = NULL;
        }

    curcacheindex = firstcacheindex = (texcacheindex *)Bcalloc(1, sizeof(texcacheindex));
    numcacheentries = 0;

//    Bmemset(&firstcacheindex, 0, sizeof(texcacheindex));
//    Bmemset(&cacheptrs[0], 0, sizeof(cacheptrs));

    hash_init(&h_texcache);
//    LoadCacheOffsets();

    Bstrcpy(ptempbuf,TEXCACHEFILE);
    unlink(ptempbuf);
    Bstrcat(ptempbuf,".cache");
    unlink(ptempbuf);
    cacheindexptr = Bfopen(ptempbuf, "at+");
    if (!cacheindexptr)
    {
        glusetexcache = 0;
        initprintf("Unable to open cache index: %s\n",strerror(errno));
        return;
    }

    Bfprintf(cacheindexptr,"// automatically generated by EDuke32, DO NOT MODIFY!\n");

    cachefilehandle = Bopen(TEXCACHEFILE,BO_BINARY|BO_CREAT|BO_TRUNC|BO_APPEND|BO_RDWR,BS_IREAD|BS_IWRITE);

    if (cachefilehandle < 0)
    {
        initprintf("Unable to open cache file: %s\n",strerror(errno));
        glusetexcache = 0;
        return;
    }
}

void resizeglcheck()
{
    float m[4][4];
    int32_t fovcorrect;

    if (glredbluemode < lastglredbluemode)
    {
        glox1 = -1;
        bglColorMask(1,1,1,1);
    }
    else if (glredbluemode != lastglredbluemode)
    {
        redblueclearcnt = 0;
    }
    lastglredbluemode = glredbluemode;

    //FUK
    if (lastglpolygonmode != glpolygonmode)
    {
        lastglpolygonmode = glpolygonmode;
        switch (glpolygonmode)
        {
        default:
        case 0:
            bglPolygonMode(GL_FRONT_AND_BACK,GL_FILL); break;
        case 1:
            bglPolygonMode(GL_FRONT_AND_BACK,GL_LINE); break;
        case 2:
            bglPolygonMode(GL_FRONT_AND_BACK,GL_POINT); break;
        }
    }
    if (glpolygonmode) //FUK
    {
        bglClearColor(1.0,1.0,1.0,0.0);
        bglClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        bglDisable(GL_TEXTURE_2D);
    }

    if ((glox1 != windowx1) || (gloy1 != windowy1) || (glox2 != windowx2) || (gloy2 != windowy2))
    {
        double ratio = 1.05;

        if (glwidescreen == 1)
            ratio = 1.2f;
        else if (glprojectionhacks == 1)
        {
            double mul = (gshang*gshang);
            ratio += mul*mul*mul*mul;
        }
        else if (glprojectionhacks == 2)
        {
            if (gshang > 0.7f)
                ratio += 4.f*(gshang-0.7f);
            if (gshang < -0.7f)
                ratio += 4.f*(-gshang-0.7f);
        }

        glox1 = (float)windowx1; gloy1 = (float)windowy1;
        glox2 = (float)windowx2; gloy2 = (float)windowy2;

        fovcorrect = (int32_t)(glprojectionhacks?(glwidescreen?0:(((windowx2-windowx1+1) * ratio) - (windowx2-windowx1+1))):0);

        bglViewport(windowx1 - (fovcorrect / 2), yres-(windowy2+1),windowx2-windowx1+1 + fovcorrect, windowy2-windowy1+1);

        bglMatrixMode(GL_PROJECTION);
        memset(m,0,sizeof(m));
        m[0][0] = (float)ydimen / (glprojectionhacks?ratio:1.f); m[0][2] = 1.0;
        m[1][1] = (float)xdimen; m[1][2] = 1.0;
        m[2][2] = 1.0; m[2][3] = (float)ydimen / (glprojectionhacks?ratio:1.f);
        m[3][2] =-1.0;
        bglLoadMatrixf(&m[0][0]);
        //bglLoadIdentity();

        bglMatrixMode(GL_MODELVIEW);
        bglLoadIdentity();

#ifdef USE_OPENGL
        if (!nofog) bglEnable(GL_FOG);
#endif

        //bglEnable(GL_TEXTURE_2D);
    }
}

void fixtransparency(coltype *dapic, int32_t daxsiz, int32_t daysiz, int32_t daxsiz2, int32_t daysiz2, int32_t dameth)
{
    coltype *wpptr;
    int32_t j, x, y, r, g, b, dox, doy, naxsiz2;

    dox = daxsiz2-1; doy = daysiz2-1;
    if (dameth&4) { dox = min(dox,daxsiz); doy = min(doy,daysiz); }
    else { daxsiz = daxsiz2; daysiz = daysiz2; } //Make repeating textures duplicate top/left parts

    daxsiz--; daysiz--; naxsiz2 = -daxsiz2; //Hacks for optimization inside loop

    //Set transparent pixels to average color of neighboring opaque pixels
    //Doing this makes bilinear filtering look much better for masked textures (I.E. sprites)
    for (y=doy; y>=0; y--)
    {
        wpptr = &dapic[y*daxsiz2+dox];
        for (x=dox; x>=0; x--,wpptr--)
        {
            if (wpptr->a) continue;
            r = g = b = j = 0;
            if ((x>     0) && (wpptr[     -1].a)) { r += (int32_t)wpptr[     -1].r; g += (int32_t)wpptr[     -1].g; b += (int32_t)wpptr[     -1].b; j++; }
            if ((x<daxsiz) && (wpptr[     +1].a)) { r += (int32_t)wpptr[     +1].r; g += (int32_t)wpptr[     +1].g; b += (int32_t)wpptr[     +1].b; j++; }
            if ((y>     0) && (wpptr[naxsiz2].a)) { r += (int32_t)wpptr[naxsiz2].r; g += (int32_t)wpptr[naxsiz2].g; b += (int32_t)wpptr[naxsiz2].b; j++; }
            if ((y<daysiz) && (wpptr[daxsiz2].a)) { r += (int32_t)wpptr[daxsiz2].r; g += (int32_t)wpptr[daxsiz2].g; b += (int32_t)wpptr[daxsiz2].b; j++; }
            switch (j)
            {
            case 1:
                wpptr->r =   r            ; wpptr->g =   g            ; wpptr->b =   b            ; break;
            case 2:
                wpptr->r = ((r   +  1)>>1); wpptr->g = ((g   +  1)>>1); wpptr->b = ((b   +  1)>>1); break;
            case 3:
                wpptr->r = ((r*85+128)>>8); wpptr->g = ((g*85+128)>>8); wpptr->b = ((b*85+128)>>8); break;
            case 4:
                wpptr->r = ((r   +  2)>>2); wpptr->g = ((g   +  2)>>2); wpptr->b = ((b   +  2)>>2); break;
            default:
                break;
            }
        }
    }
}

void uploadtexture(int32_t doalloc, int32_t xsiz, int32_t ysiz, int32_t intexfmt, int32_t texfmt, coltype *pic, int32_t tsizx, int32_t tsizy, int32_t dameth)
{
    coltype *wpptr, *rpptr;
    int32_t x2, y2, j, js=0, x3, y3, y, x, r, g, b, a, k;
    int32_t hi = (dameth&8192)?1:0;
    int32_t nocompress = (dameth&4096)?1:0;

    dameth &= ~(8192|4096);

    if (gltexmaxsize <= 0)
    {
        GLint i = 0;
        bglGetIntegerv(GL_MAX_TEXTURE_SIZE, &i);
        if (!i) gltexmaxsize = 6;   // 2^6 = 64 == default GL max texture size
        else
        {
            gltexmaxsize = 0;
            for (; i>1; i>>=1) gltexmaxsize++;
        }
    }

    js = max(0,min(gltexmaxsize-1,gltexmiplevel));
    gltexmiplevel = js;
    while ((xsiz>>js) > (1<<gltexmaxsize) || (ysiz>>js) > (1<<gltexmaxsize)) js++;

    if (hi && !nocompress) js = r_downsize;

    /*
    OSD_Printf("Uploading %dx%d %s as %s\n", xsiz,ysiz,
            (texfmt==GL_RGBA?"GL_RGBA":
             texfmt==GL_RGB?"GL_RGB":
             texfmt==GL_BGR?"GL_BGR":
             texfmt==GL_BGRA?"GL_BGRA":"other"),
            (intexfmt==GL_RGBA?"GL_RGBA":
             intexfmt==GL_RGB?"GL_RGB":
             intexfmt==GL_COMPRESSED_RGBA_ARB?"GL_COMPRESSED_RGBA_ARB":
             intexfmt==GL_COMPRESSED_RGB_ARB?"GL_COMPRESSED_RGB_ARB":"other"));
    */

    if (js == 0)
    {
        if (doalloc&1)
            bglTexImage2D(GL_TEXTURE_2D,0,intexfmt,xsiz,ysiz,0,texfmt,GL_UNSIGNED_BYTE,pic); //loading 1st time
        else
            bglTexSubImage2D(GL_TEXTURE_2D,0,0,0,xsiz,ysiz,texfmt,GL_UNSIGNED_BYTE,pic); //overwrite old texture
    }

#if 0
    gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8,xsiz,ysiz,texfmt,GL_UNSIGNED_BYTE,pic); //Needs C++ to link?
#elif 1
    x2 = xsiz; y2 = ysiz;
    for (j=1; (x2 > 1) || (y2 > 1); j++)
    {
        //x3 = ((x2+1)>>1); y3 = ((y2+1)>>1);
        x3 = max(1, x2 >> 1); y3 = max(1, y2 >> 1);		// this came from the GL_ARB_texture_non_power_of_two spec
        for (y=0; y<y3; y++)
        {
            wpptr = &pic[y*x3]; rpptr = &pic[(y<<1)*x2];
            for (x=0; x<x3; x++,wpptr++,rpptr+=2)
            {
                r = g = b = a = k = 0;
                if (rpptr[0].a)                  { r += (int32_t)rpptr[0].r; g += (int32_t)rpptr[0].g; b += (int32_t)rpptr[0].b; a += (int32_t)rpptr[0].a; k++; }
                if ((x+x+1 < x2) && (rpptr[1].a)) { r += (int32_t)rpptr[1].r; g += (int32_t)rpptr[1].g; b += (int32_t)rpptr[1].b; a += (int32_t)rpptr[1].a; k++; }
                if (y+y+1 < y2)
                {
                    if ((rpptr[x2].a)) { r += (int32_t)rpptr[x2  ].r; g += (int32_t)rpptr[x2  ].g; b += (int32_t)rpptr[x2  ].b; a += (int32_t)rpptr[x2  ].a; k++; }
                    if ((x+x+1 < x2) && (rpptr[x2+1].a)) { r += (int32_t)rpptr[x2+1].r; g += (int32_t)rpptr[x2+1].g; b += (int32_t)rpptr[x2+1].b; a += (int32_t)rpptr[x2+1].a; k++; }
                }
                switch (k)
                {
                case 0:
                case 1:
                    wpptr->r = r; wpptr->g = g; wpptr->b = b; wpptr->a = a; break;
                case 2:
                    wpptr->r = ((r+1)>>1); wpptr->g = ((g+1)>>1); wpptr->b = ((b+1)>>1); wpptr->a = ((a+1)>>1); break;
                case 3:
                    wpptr->r = ((r*85+128)>>8); wpptr->g = ((g*85+128)>>8); wpptr->b = ((b*85+128)>>8); wpptr->a = ((a*85+128)>>8); break;
                case 4:
                    wpptr->r = ((r+2)>>2); wpptr->g = ((g+2)>>2); wpptr->b = ((b+2)>>2); wpptr->a = ((a+2)>>2); break;
                default:
                    break;
                }
                //if (wpptr->a) wpptr->a = 255;
            }
        }
        if (tsizx >= 0) fixtransparency(pic,(tsizx+(1<<j)-1)>>j,(tsizy+(1<<j)-1)>>j,x3,y3,dameth);
        if (j >= js)
        {
            if (doalloc&1)
                bglTexImage2D(GL_TEXTURE_2D,j-js,intexfmt,x3,y3,0,texfmt,GL_UNSIGNED_BYTE,pic); //loading 1st time
            else
                bglTexSubImage2D(GL_TEXTURE_2D,j-js,0,0,x3,y3,texfmt,GL_UNSIGNED_BYTE,pic); //overwrite old texture
        }
        x2 = x3; y2 = y3;
    }
#endif
}

int32_t gloadtile_art(int32_t dapic, int32_t dapal, int32_t dameth, pthtyp *pth, int32_t doalloc)
{
    coltype *pic, *wpptr;
    int32_t x, y, x2, y2, xsiz, ysiz, dacol, tsizx, tsizy;
    char hasalpha = 0, hasfullbright = 0;

    tsizx = tilesizx[dapic];
    tsizy = tilesizy[dapic];
    if (!glinfo.texnpot)
    {
        for (xsiz=1; xsiz<tsizx; xsiz+=xsiz);
        for (ysiz=1; ysiz<tsizy; ysiz+=ysiz);
    }
    else
    {
        if ((tsizx|tsizy) == 0)
        {
            xsiz = ysiz = 1;
        }
        else
        {
            xsiz = tsizx;
            ysiz = tsizy;
        }
    }

    pic = (coltype *)Bmalloc(xsiz*ysiz*sizeof(coltype));
    if (!pic) return 1;

    if (!waloff[dapic])
    {
        //Force invalid textures to draw something - an almost purely transparency texture
        //This allows the Z-buffer to be updated for mirrors (which are invalidated textures)
        pic[0].r = pic[0].g = pic[0].b = 0; pic[0].a = 1;
        tsizx = tsizy = 1; hasalpha = 1;
    }
    else
    {
        for (y=0; y<ysiz; y++)
        {
            if (y < tsizy) y2 = y; else y2 = y-tsizy;
            wpptr = &pic[y*xsiz];
            for (x=0; x<xsiz; x++,wpptr++)
            {
                if ((dameth&4) && ((x >= tsizx) || (y >= tsizy))) //Clamp texture
                    { wpptr->r = wpptr->g = wpptr->b = wpptr->a = 0; continue; }
                if (x < tsizx) x2 = x; else x2 = x-tsizx;
                dacol = (int32_t)(*(char *)(waloff[dapic]+x2*tsizy+y2));
                if (!fullbrightloadingpass)
                {
                    // regular texture
                    if ((dacol > 239) && (dacol != 255))
                        hasfullbright = 1;
                    wpptr->a = 255;
                }
                else if (fullbrightloadingpass == 1)
                {
                    // texture with only fullbright areas
                    if (dacol < 240)    // regular colors
                    {
                        wpptr->a = 0; hasalpha = 1;
                    }
                    else   // fullbright
                    {
                        wpptr->a = 255;
                    }
                }
                if (dacol != 255)
                    dacol = (int32_t)((uint8_t)palookup[dapal][dacol]);
                else
                {
                    wpptr->a = 0; hasalpha = 1;
                }
                if (gammabrightness)
                {
                    wpptr->r = curpalette[dacol].r;
                    wpptr->g = curpalette[dacol].g;
                    wpptr->b = curpalette[dacol].b;
                }
                else
                {
                    wpptr->r = britable[curbrightness][ curpalette[dacol].r ];
                    wpptr->g = britable[curbrightness][ curpalette[dacol].g ];
                    wpptr->b = britable[curbrightness][ curpalette[dacol].b ];
                }
            }
        }
    }

    if (doalloc) bglGenTextures(1,(GLuint*)&pth->glpic);  //# of textures (make OpenGL allocate structure)
    bglBindTexture(GL_TEXTURE_2D,pth->glpic);

    fixtransparency(pic,tsizx,tsizy,xsiz,ysiz,dameth);
    uploadtexture(doalloc,xsiz,ysiz,hasalpha?GL_RGBA:GL_RGB,GL_RGBA,pic,tsizx,tsizy,dameth);

    if (gltexfiltermode < 0) gltexfiltermode = 0;
    else if (gltexfiltermode >= (int32_t)numglfiltermodes) gltexfiltermode = numglfiltermodes-1;
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,glfiltermodes[gltexfiltermode].mag);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,glfiltermodes[gltexfiltermode].min);

    if (glinfo.maxanisotropy > 1.0)
    {
        if (glanisotropy <= 0 || glanisotropy > glinfo.maxanisotropy) glanisotropy = (int32_t)glinfo.maxanisotropy;
        bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,glanisotropy);
    }

    if (!(dameth&4))
    {
        bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    }
    else
    {
        //For sprite textures, clamping looks better than wrapping
        bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
        bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
    }

    if (pic) Bfree(pic);

    pth->picnum = dapic;
    pth->palnum = dapal;
    pth->effects = 0;
    pth->flags = ((dameth&4)>>2) | (hasalpha<<3);
    pth->hicr = NULL;

    if ((hasfullbright) && (!fullbrightloadingpass))
    {
        // load the ONLY texture that'll be assembled with the regular one to make the final texture with fullbright pixels
        fullbrightloadingpass = 1;
        pth->ofb = (pthtyp *)Bcalloc(1,sizeof(pthtyp));
        if (!pth->ofb) return 1;
        pth->flags |= (hasfullbright<<4);
        if (gloadtile_art(dapic, dapal, dameth, pth->ofb, 1)) return 1;

        fullbrightloadingpass = 0;
    }
    return 0;
}

// JONOF'S COMPRESSED TEXTURE CACHE STUFF ---------------------------------------------------

static int32_t LoadCacheOffsets(void)
{
    int32_t foffset, fsize, i;
    char *fname;

    scriptfile *script;

    Bstrcpy(ptempbuf,TEXCACHEFILE);
    Bstrcat(ptempbuf,".cache");
    script = scriptfile_fromfile(ptempbuf);

    if (!script) return -1;

    while (!scriptfile_eof(script))
    {
        if (scriptfile_getstring(script, &fname)) break;	// filename
        if (scriptfile_getnumber(script, &foffset)) break;	// offset in cache
        if (scriptfile_getnumber(script, &fsize)) break;	// size

        i = hash_find(&h_texcache,fname);
        if (i > -1)
        {
            // update an existing entry
            texcacheindex *t = cacheptrs[i];
            t->offset = foffset;
            t->len = fsize;
            /*initprintf("%s %d got a match for %s offset %d\n",__FILE__, __LINE__, fname,foffset);*/
        }
        else
        {
            Bstrncpy(curcacheindex->name, fname, BMAX_PATH);
            curcacheindex->offset = foffset;
            curcacheindex->len = fsize;
            curcacheindex->next = (texcacheindex *)Bcalloc(1, sizeof(texcacheindex));
            hash_add(&h_texcache, Bstrdup(fname), numcacheentries, 1);
            cacheptrs[numcacheentries++] = curcacheindex;
            curcacheindex = curcacheindex->next;
        }
    }

    scriptfile_close(script);
    return 0;
}

void phex(char v, char *s)
{
    int32_t x;
    x = v>>4;
    s[0] = x<10 ? (x+'0') : (x-10+'a');
    x = v&15;
    s[1] = x<10 ? (x+'0') : (x-10+'a');
}

int32_t trytexcache(char *fn, int32_t len, int32_t dameth, char effect, texcacheheader *head)
{
    int32_t fp;
    char cachefn[BMAX_PATH], *cp;
    uint8_t mdsum[16];

    if (!glusetexcompr || !glusetexcache || !cacheindexptr || cachefilehandle < 0) return -1;
    if (!glinfo.texcompr || !bglCompressedTexImage2DARB || !bglGetCompressedTexImageARB)
    {
        // lacking the necessary extensions to do this
        OSD_Printf("Warning: the GL driver lacks necessary functions to use caching\n");
        glusetexcache = 0;
        return -1;
    }

    md4once((uint8_t *)fn, strlen(fn), mdsum);
//    for (cp = cachefn, fp = 0; (*cp = TEXCACHEFILE[fp]); cp++,fp++);
//    *(cp++) = '/';
    cp = cachefn;
    for (fp = 0; fp < 16; phex(mdsum[fp++], cp), cp+=2);
    Bsprintf(cp, "-%x-%x%x", len, dameth, effect);

    {
        int32_t offset = 0;
        int32_t len = 0;
        int32_t i;

        i = hash_find(&h_texcache,cachefn);
        if (i > -1)
        {
            texcacheindex *t = cacheptrs[i];
            if (!t)
                i = -1;
            else
            {
                len = t->len;
                offset = t->offset;
            }
            /*initprintf("%s %d got a match for %s offset %d\n",__FILE__, __LINE__, cachefn,offset);*/
        }

        if (i < 0)
            return -1; // didn't find it

        cachepos = offset;
    }

//    initprintf("Loading cached tex: %s\n", cachefn);

    if (memcachedata && memcachesize >= (signed)(cachepos + sizeof(texcacheheader)))
    {
//        initprintf("using memcache!\n");
        Bmemcpy(head, memcachedata + cachepos, sizeof(texcacheheader));
        cachepos += sizeof(texcacheheader);
    }
    else
    {
        Blseek(cachefilehandle, cachepos, BSEEK_SET);
        if (Bread(cachefilehandle, head, sizeof(texcacheheader)) < (int32_t)sizeof(texcacheheader))
        {
            cachepos += sizeof(texcacheheader); 
            goto failure;
        }
        cachepos += sizeof(texcacheheader);
    }

    if (Bmemcmp(head->magic, TEXCACHEMAGIC, 4)) goto failure;
    head->xdim = B_LITTLE32(head->xdim);
    head->ydim = B_LITTLE32(head->ydim);
    head->flags = B_LITTLE32(head->flags);
    head->quality = B_LITTLE32(head->quality);

    if ((head->flags & 4) && glusetexcache != 2) goto failure;
    if (!(head->flags & 4) && glusetexcache == 2) goto failure;

    if (!(head->flags & 8) && head->quality != r_downsize) return -1; // handle nocompress
    if (gltexmaxsize && (head->xdim > (1<<gltexmaxsize) || head->ydim > (1<<gltexmaxsize))) goto failure;
    if (!glinfo.texnpot && (head->flags & 1)) goto failure;

    return cachefilehandle;
failure:
    initprintf("cache miss\n");
    return -1;
}

void writexcache(char *fn, int32_t len, int32_t dameth, char effect, texcacheheader *head)
{
    int32_t fp;
    char cachefn[BMAX_PATH], *cp;
    uint8_t mdsum[16];
    texcachepicture pict;
    char *pic = NULL, *packbuf = NULL;
    void *midbuf = NULL;
    uint32_t alloclen=0, level, miplen;
    uint32_t padx=0, pady=0;
    GLint gi;
    int32_t offset = 0;

    if (!glinfo.texcompr || !glusetexcompr || !glusetexcache) return;
    if (!bglCompressedTexImage2DARB || !bglGetCompressedTexImageARB)
    {
        // lacking the necessary extensions to do this
        OSD_Printf("Warning: the GL driver lacks necessary functions to use caching\n");
        glusetexcache = 0;
        return;
    }
    if (!cacheindexptr || cachefilehandle < 0)
    {
        OSD_Printf("Warning: no active cache!\n");
        return;
    }

    gi = GL_FALSE;
    bglGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_ARB, (GLint *)&gi);
    if (gi != GL_TRUE)
    {
        OSD_Printf("Error: glGetTexLevelParameteriv returned GL_FALSE!\n");
        return;
    }

    md4once((uint8_t *)fn, strlen(fn), mdsum);

    cp = cachefn;
    for (fp = 0; fp < 16; phex(mdsum[fp++], cp), cp+=2);
    Bsprintf(cp, "-%x-%x%x", len, dameth, effect);

    Blseek(cachefilehandle, 0, BSEEK_END);

    offset = Blseek(cachefilehandle, 0, BSEEK_CUR);
//    OSD_Printf("Caching %s, offset 0x%x\n", cachefn, offset);

    Bmemcpy(head->magic, TEXCACHEMAGIC, 4);   // sizes are set by caller

    if (glusetexcache == 2) head->flags |= 4;

    head->xdim = B_LITTLE32(head->xdim);
    head->ydim = B_LITTLE32(head->ydim);
    head->flags = B_LITTLE32(head->flags);
    head->quality = B_LITTLE32(head->quality);

    if (Bwrite(cachefilehandle, head, sizeof(texcacheheader)) != sizeof(texcacheheader)) goto failure;

    bglGetError();
    for (level = 0; level==0 || (padx > 1 || pady > 1); level++)
    {
        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_COMPRESSED_ARB, (GLint *)&gi);
        if (bglGetError() != GL_NO_ERROR) goto failure;
        if (gi != GL_TRUE) goto failure;   // an uncompressed mipmap
        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_INTERNAL_FORMAT, (GLint *)&gi);
        if (bglGetError() != GL_NO_ERROR) goto failure;
#ifdef __APPLE__
        if (pr_ati_textureformat_one && gi == 1) gi = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
#endif
        pict.format = B_LITTLE32(gi);
        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_WIDTH, (GLint *)&gi);
        if (bglGetError() != GL_NO_ERROR) goto failure;
        padx = gi; pict.xdim = B_LITTLE32(gi);
        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_HEIGHT, (GLint *)&gi);
        if (bglGetError() != GL_NO_ERROR) goto failure;
        pady = gi; pict.ydim = B_LITTLE32(gi);
        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_BORDER, (GLint *)&gi);
        if (bglGetError() != GL_NO_ERROR) goto failure;
        pict.border = B_LITTLE32(gi);
        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_DEPTH, (GLint *)&gi);
        if (bglGetError() != GL_NO_ERROR) goto failure;
        pict.depth = B_LITTLE32(gi);
        bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, (GLint *)&gi);
        if (bglGetError() != GL_NO_ERROR) goto failure;
        miplen = (int32_t)gi; pict.size = B_LITTLE32(gi);

        if (alloclen < miplen)
        {
            void *picc = Brealloc(pic, miplen);
            if (!picc) goto failure; else pic = picc;
            alloclen = miplen;

            picc = Brealloc(packbuf, alloclen+16);
            if (!picc) goto failure; else packbuf = picc;

            picc = Brealloc(midbuf, miplen);
            if (!picc) goto failure; else midbuf = picc;
        }

        bglGetCompressedTexImageARB(GL_TEXTURE_2D, level, pic);
        if (bglGetError() != GL_NO_ERROR) goto failure;

        if (Bwrite(cachefilehandle, &pict, sizeof(texcachepicture)) != sizeof(texcachepicture)) goto failure;
        if (dxtfilter(cachefilehandle, &pict, pic, midbuf, packbuf, miplen)) goto failure;
    }

    {
        int32_t i = hash_find(&h_texcache,cachefn);
        if (i > -1)
        {
            // update an existing entry
            texcacheindex *t = cacheptrs[i];
            t->offset = offset;
            t->len = Blseek(cachefilehandle, 0, BSEEK_CUR) - t->offset;
            /*initprintf("%s %d got a match for %s offset %d\n",__FILE__, __LINE__, cachefn,offset);*/

            if (cacheindexptr)
            {
                fseek(cacheindexptr, 0, BSEEK_END);
                Bfprintf(cacheindexptr, "%s %d %d\n", t->name, t->offset, t->len);
            }
            else OSD_Printf("wtf?\n");
        }
        else
        {
            Bstrcpy(curcacheindex->name, cachefn);
            curcacheindex->offset = offset;
            curcacheindex->len = Blseek(cachefilehandle, 0, BSEEK_CUR) - curcacheindex->offset;
            curcacheindex->next = (texcacheindex *)Bcalloc(1, sizeof(texcacheindex));

            if (cacheindexptr)
            {
                fseek(cacheindexptr, 0, BSEEK_END);
                Bfprintf(cacheindexptr, "%s %d %d\n", curcacheindex->name, curcacheindex->offset, curcacheindex->len);
            }
            else OSD_Printf("wtf?\n");

            hash_add(&h_texcache, Bstrdup(cachefn), numcacheentries, 0);
            cacheptrs[numcacheentries++] = curcacheindex;
            curcacheindex = curcacheindex->next;
        }
    }

    goto success;

failure:
    initprintf("ERROR: cache failure!\n");
    curcacheindex->offset = 0;
    Bmemset(curcacheindex->name,0,sizeof(curcacheindex->name));

success:
    if (midbuf) Bfree(midbuf);
    if (pic) Bfree(pic);
    if (packbuf) Bfree(packbuf);
}

int32_t gloadtile_cached(int32_t fil, texcacheheader *head, int32_t *doalloc, pthtyp *pth,int32_t dapalnum)
{
    int32_t level, r;
    texcachepicture pict;
    char *pic = NULL, *packbuf = NULL;
    void *midbuf = NULL;
    int32_t alloclen=0;

    UNREFERENCED_PARAMETER(dapalnum);

    if (*doalloc&1)
    {
        bglGenTextures(1,(GLuint*)&pth->glpic);  //# of textures (make OpenGL allocate structure)
        *doalloc |= 2;   // prevents bglGenTextures being called again if we fail in here
    }
    bglBindTexture(GL_TEXTURE_2D,pth->glpic);

    pth->sizx = head->xdim;
    pth->sizy = head->ydim;

    bglGetError();

    // load the mipmaps
    for (level = 0; level==0 || (pict.xdim > 1 || pict.ydim > 1); level++)
    {
        if (memcachedata && memcachesize >= (signed)(cachepos + sizeof(texcachepicture)))
        {
            //        initprintf("using memcache!\n");
            Bmemcpy(&pict, memcachedata + cachepos, sizeof(texcachepicture));
            cachepos += sizeof(texcachepicture);
        }
        else
        {
            Blseek(fil, cachepos, BSEEK_SET);
            r = Bread(fil, &pict, sizeof(texcachepicture));
            cachepos += sizeof(texcachepicture);
            if (r < (int32_t)sizeof(texcachepicture)) goto failure;
        }

        pict.size = B_LITTLE32(pict.size);
        pict.format = B_LITTLE32(pict.format);
        pict.xdim = B_LITTLE32(pict.xdim);
        pict.ydim = B_LITTLE32(pict.ydim);
        pict.border = B_LITTLE32(pict.border);
        pict.depth = B_LITTLE32(pict.depth);

        if (alloclen < pict.size)
        {
            void *picc = Brealloc(pic, pict.size);
            if (!picc) goto failure; else pic = picc;
            alloclen = pict.size;

            picc = Brealloc(packbuf, alloclen+16);
            if (!picc) goto failure; else packbuf = picc;

            picc = Brealloc(midbuf, pict.size);
            if (!picc) goto failure; else midbuf = picc;
        }

        if (dedxtfilter(fil, &pict, pic, midbuf, packbuf, (head->flags&4)==4)) goto failure;

        bglCompressedTexImage2DARB(GL_TEXTURE_2D,level,pict.format,pict.xdim,pict.ydim,pict.border,
                                   pict.size,pic);
        if (bglGetError() != GL_NO_ERROR) goto failure;

        {
            GLint format;
            bglGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_INTERNAL_FORMAT, &format);
            if (bglGetError() != GL_NO_ERROR) goto failure;
//            format = B_LITTLE32(format);
            if (pict.format != format)
            {
                OSD_Printf("invalid texture cache file format %d %d\n",pict.format, format);
                goto failure;
            }
        }

    }

    if (midbuf) Bfree(midbuf);
    if (pic) Bfree(pic);
    if (packbuf) Bfree(packbuf);
    return 0;
failure:
    initprintf("failure!!!\n");
    if (midbuf) Bfree(midbuf);
    if (pic) Bfree(pic);
    if (packbuf) Bfree(packbuf);
    return -1;
}
// --------------------------------------------------- JONOF'S COMPRESSED TEXTURE CACHE STUFF
int32_t gloadtile_hi(int32_t dapic,int32_t dapalnum, int32_t facen, hicreplctyp *hicr, int32_t dameth, pthtyp *pth, int32_t doalloc, char effect)
{
    coltype *pic = NULL, *rpptr;
    int32_t j, x, y, xsiz=0, ysiz=0, tsizx, tsizy;
    int32_t r, g, b;

    char *picfil = NULL, *fn, hasalpha = 255;
    int32_t picfillen, texfmt = GL_RGBA, intexfmt = GL_RGBA, filh;

    int32_t cachefil = -1;
    texcacheheader cachead;

    static coltype *lastpic = NULL;
    static char *lastfn = NULL;
    static int32_t lastsize = 0;

    if (!hicr) return -1;
    if (facen > 0)
    {
        if (!hicr->skybox) return -1;
        if (facen > 6) return -1;
        if (!hicr->skybox->face[facen-1]) return -1;
        fn = hicr->skybox->face[facen-1];
    }
    else
    {
        if (!hicr->filename) return -1;
        fn = hicr->filename;
    }

    if ((filh = kopen4load(fn, 0)) < 0)
    {
        OSD_Printf("hightile: %s (pic %d) not found\n", fn, dapic);
        if (facen > 0)
            hicr->skybox->ignore = 1;
        else
            hicr->ignore = 1;
        return -1;
    }
    picfillen = kfilelength(filh);

    kclose(filh);	// FIXME: shouldn't have to do this. bug in cache1d.c

    cachefil = trytexcache(fn, picfillen+(dapalnum<<8), dameth, effect, &cachead);
    if (cachefil >= 0 && !gloadtile_cached(cachefil, &cachead, &doalloc, pth, dapalnum))
    {
        tsizx = cachead.xdim;
        tsizy = cachead.ydim;
        hasalpha = (cachead.flags & 2) ? 0 : 255;
    }
    else
    {
        cachefil = -1;	// the compressed version will be saved to disk

        if ((filh = kopen4load(fn, 0)) < 0) return -1;

        picfil = (char *)Bmalloc(picfillen+1); if (!picfil) { kclose(filh); return 1; }
        kread(filh, picfil, picfillen);
        kclose(filh);

        // tsizx/y = replacement texture's natural size
        // xsiz/y = 2^x size of replacement

        kpgetdim(picfil,picfillen,&tsizx,&tsizy);
        if (tsizx == 0 || tsizy == 0) { Bfree(picfil); return -1; }
        pth->sizx = tsizx;
        pth->sizy = tsizy;

        if (!glinfo.texnpot)
        {
            for (xsiz=1; xsiz<tsizx; xsiz+=xsiz);
            for (ysiz=1; ysiz<tsizy; ysiz+=ysiz);
        }
        else
        {
            xsiz = tsizx;
            ysiz = tsizy;
        }
        pic = (coltype *)Bcalloc(xsiz,ysiz*sizeof(coltype)); if (!pic) { Bfree(picfil); return 1; }

        if (lastpic && lastfn && !Bstrcmp(lastfn,fn))
        {
//            if (hicprecaching) OSD_Printf("use  %4d: p%-3d m%d e%d\n", dapic, dapalnum, dameth, effect);
            Bmemcpy(pic, lastpic, xsiz*ysiz*sizeof(coltype));
        }
        else
        {
//            if (hicprecaching) OSD_Printf("rend %4d: p%-3d m%d e%d: `%s'\n", dapic, dapalnum, dameth, effect, fn);
            if (kprender(picfil,picfillen,(intptr_t)pic,xsiz*sizeof(coltype),xsiz,ysiz,0,0)) { Bfree(picfil); Bfree(pic); return -2; }

            if (hicprecaching)
            {
                lastfn = fn;  // careful...
                if (!lastpic)
                {
                    lastpic = Bmalloc(xsiz*ysiz*sizeof(coltype));
                    lastsize = xsiz*ysiz;
                }
                else if (lastsize < xsiz*ysiz)
                {
                    Bfree(lastpic);
                    lastpic = Bmalloc(xsiz*ysiz*sizeof(coltype));
                }
                if (lastpic)
                    Bmemcpy(lastpic, pic, xsiz*ysiz*sizeof(coltype));
            }
            else if (lastpic)
            {
                Bfree(lastpic); lastpic=NULL;
                lastfn = NULL;
                lastsize = 0;
            }
        }

        r=(glinfo.bgra)?hictinting[dapalnum].r:hictinting[dapalnum].b;
        g=hictinting[dapalnum].g;
        b=(glinfo.bgra)?hictinting[dapalnum].b:hictinting[dapalnum].r;
        for (y=0,j=0; y<tsizy; y++,j+=xsiz)
        {
            coltype tcol;
            char *cptr = &britable[gammabrightness ? 0 : curbrightness][0];
            rpptr = &pic[j];

            for (x=0; x<tsizx; x++)
            {
                tcol.b = cptr[rpptr[x].b];
                tcol.g = cptr[rpptr[x].g];
                tcol.r = cptr[rpptr[x].r];
                tcol.a = rpptr[x].a; hasalpha &= rpptr[x].a;

                if (effect & 1)
                {
                    // greyscale
                    tcol.b = max(tcol.b, max(tcol.g, tcol.r));
                    tcol.g = tcol.r = tcol.b;
                }
                if (effect & 2)
                {
                    // invert
                    tcol.b = 255-tcol.b;
                    tcol.g = 255-tcol.g;
                    tcol.r = 255-tcol.r;
                }
                if (effect & 4)
                {
                    // colorize
                    tcol.b = min((int32_t)((tcol.b)*r)/64,255);
                    tcol.g = min((int32_t)((tcol.g)*g)/64,255);
                    tcol.r = min((int32_t)((tcol.r)*b)/64,255);
                }

                rpptr[x].b = tcol.b;
                rpptr[x].g = tcol.g;
                rpptr[x].r = tcol.r;
                rpptr[x].a = tcol.a;
            }
        }
        if ((!(dameth&4)) || (facen)) //Duplicate texture pixels (wrapping tricks for non power of 2 texture sizes)
        {
            if (xsiz > tsizx) //Copy left to right
            {
                int32_t *lptr = (int32_t *)pic;
                for (y=0; y<tsizy; y++,lptr+=xsiz)
                    Bmemcpy(&lptr[tsizx],lptr,(xsiz-tsizx)<<2);
            }
            if (ysiz > tsizy)  //Copy top to bottom
                Bmemcpy(&pic[xsiz*tsizy],pic,(ysiz-tsizy)*xsiz<<2);
        }
        if (!glinfo.bgra)
        {
            for (j=xsiz*ysiz-1; j>=0; j--)
            {
                swapchar(&pic[j].r, &pic[j].b);
            }
        }
        else texfmt = GL_BGRA;
        Bfree(picfil); picfil = 0;

        if (tsizx>>r_downsize <= tilesizx[dapic] || tsizy>>r_downsize <= tilesizy[dapic])
            hicr->flags |= 17;

        if (glinfo.texcompr && glusetexcompr && !(hicr->flags & 1))
            intexfmt = (hasalpha == 255) ? GL_COMPRESSED_RGBA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        else if (hasalpha == 255) intexfmt = GL_RGB;

        if ((doalloc&3)==1) bglGenTextures(1,(GLuint*)&pth->glpic);  //# of textures (make OpenGL allocate structure)
        bglBindTexture(GL_TEXTURE_2D,pth->glpic);

        fixtransparency(pic,tsizx,tsizy,xsiz,ysiz,dameth);
        uploadtexture(doalloc,xsiz,ysiz,intexfmt,texfmt,pic,-1,tsizy,dameth|8192|(hicr->flags & 16?4096:0));
    }

    // precalculate scaling parameters for replacement
    if (facen > 0)
    {
        pth->scalex = ((float)tsizx) / 64.0;
        pth->scaley = ((float)tsizy) / 64.0;
    }
    else
    {
        pth->scalex = ((float)tsizx) / ((float)tilesizx[dapic]);
        pth->scaley = ((float)tsizy) / ((float)tilesizy[dapic]);
    }

    if (gltexfiltermode < 0) gltexfiltermode = 0;
    else if (gltexfiltermode >= (int32_t)numglfiltermodes) gltexfiltermode = numglfiltermodes-1;
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,glfiltermodes[gltexfiltermode].mag);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,glfiltermodes[gltexfiltermode].min);

    if (glinfo.maxanisotropy > 1.0)
    {
        if (glanisotropy <= 0 || glanisotropy > glinfo.maxanisotropy) glanisotropy = (int32_t)glinfo.maxanisotropy;
        bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,glanisotropy);
    }

    if (!(dameth&4))
    {
        bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    }
    else
    {
        //For sprite textures, clamping looks better than wrapping
        bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
        bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
    }

    if (pic) Bfree(pic);

    if (tsizx>>r_downsize <= tilesizx[dapic] || tsizy>>r_downsize <= tilesizy[dapic])
        hicr->flags |= 17;

    pth->picnum = dapic;
    pth->effects = effect;
    pth->flags = ((dameth&4)>>2) + 2 + ((facen>0)<<2); if (hasalpha != 255) pth->flags |= 8;
    pth->skyface = facen;
    pth->hicr = hicr;

    if (glinfo.texcompr && glusetexcompr && glusetexcache && !(hicr->flags & 1))
        if (cachefil < 0)
        {
            // save off the compressed version
            if (hicr->flags & 16) cachead.quality = 0;
            else cachead.quality = r_downsize;
            cachead.xdim = tsizx>>cachead.quality;
            cachead.ydim = tsizy>>cachead.quality;
            x = 0;
            for (j=0; j<31; j++)
            {
                if (xsiz == pow2long[j]) { x |= 1; }
                if (ysiz == pow2long[j]) { x |= 2; }
            }
            cachead.flags = (x!=3) | (hasalpha != 255 ? 2 : 0) | (hicr->flags & 16?8:0); // handle nocompress
            OSD_Printf("Caching \"%s\"\n", fn);
            writexcache(fn, picfillen+(dapalnum<<8), dameth, effect, &cachead);
        }

    return 0;
}

#endif

//(dpx,dpy) specifies an n-sided polygon. The polygon must be a convex clockwise loop.
//    n must be <= 8 (assume clipping can double number of vertices)
//method: 0:solid, 1:masked(255 is transparent), 2:transluscent #1, 3:transluscent #2
//    +4 means it's a sprite, so wraparound isn't needed
static int32_t pow2xsplit = 0, skyclamphack = 0;

void drawpoly(double *dpx, double *dpy, int32_t n, int32_t method)
{
    double ngdx = 0.0, ngdy = 0.0, ngdo = 0.0, ngux = 0.0, nguy = 0.0, nguo = 0.0;
    double ngvx = 0.0, ngvy = 0.0, ngvo = 0.0, dp, up, vp, rdp, du0 = 0.0, du1 = 0.0, dui, duj;
    double ngdx2, ngux2, ngvx2;
    double f, r, ox, oy, oz, ox2, oy2, oz2, dd[16], uu[16], vv[16], px[16], py[16], uoffs;
    int32_t i, j, k, x, y, z, nn, ix0, ix1, mini, maxi, tsizx, tsizy, tsizxm1 = 0, tsizym1 = 0, ltsizy = 0;
    int32_t xx, yy, xi, d0, u0, v0, d1, u1, v1, xmodnice = 0, ymulnice = 0, dorot;
    char dacol = 0, *walptr, *palptr = NULL, *vidp, *vide;
#ifdef USE_OPENGL
    pthtyp *pth, *detailpth, *glowpth;
    int32_t texunits = GL_TEXTURE0_ARB;
#endif
    // backup of the n for possible redrawing of fullbright
    int32_t n_ = n, method_ = method;

    if (method == -1) return;

    if (n == 3)
    {
        if ((dpx[0]-dpx[1])*(dpy[2]-dpy[1]) >= (dpx[2]-dpx[1])*(dpy[0]-dpy[1])) return; //for triangle
    }
    else
    {
        f = 0; //f is area of polygon / 2
        for (i=n-2,j=n-1,k=0; k<n; i=j,j=k,k++) f += (dpx[i]-dpx[k])*dpy[j];
        if (f <= 0) return;
    }

    //Load texture (globalpicnum)
    if ((uint32_t)globalpicnum >= MAXTILES) globalpicnum = 0;
    setgotpic(globalpicnum);
    tsizx = tilesizx[globalpicnum];
    tsizy = tilesizy[globalpicnum];
    if (palookup[globalpal] == NULL)
        globalpal = 0;
    if (!waloff[globalpicnum])
    {
        loadtile(globalpicnum);
        if (!waloff[globalpicnum])
        {
            if (rendmode < 3) return;
            tsizx = tsizy = 1; method = 1; //Hack to update Z-buffer for invalid mirror textures
        }
    }
    walptr = (char *)waloff[globalpicnum];

    j = 0; dorot = ((gchang != 1.0) || (gctang != 1.0));
    if (dorot)
    {
        for (i=0; i<n; i++)
        {
            ox = dpx[i]-ghalfx;
            oy = dpy[i]-ghoriz;
            oz = ghalfx;

            //Up/down rotation
            ox2 = ox;
            oy2 = oy*gchang - oz*gshang;
            oz2 = oy*gshang + oz*gchang;

            //Tilt rotation
            ox = ox2*gctang - oy2*gstang;
            oy = ox2*gstang + oy2*gctang;
            oz = oz2;

            if ((oz < SCISDIST) && (rendmode < 3)) return; //annoying hack to avoid bugs in software rendering

            r = ghalfx / oz;

            dd[j] = (dpx[i]*gdx + dpy[i]*gdy + gdo)*r;
            uu[j] = (dpx[i]*gux + dpy[i]*guy + guo)*r;
            vv[j] = (dpx[i]*gvx + dpy[i]*gvy + gvo)*r;

            px[j] = ox*r + ghalfx;
            py[j] = oy*r + ghoriz;
            if ((!j) || (px[j] != px[j-1]) || (py[j] != py[j-1])) j++;
        }
    }
    else
    {
        for (i=0; i<n; i++)
        {
            px[j] = dpx[i];
            py[j] = dpy[i];
            if ((!j) || (px[j] != px[j-1]) || (py[j] != py[j-1])) j++;
        }
    }
    while ((j >= 3) && (px[j-1] == px[0]) && (py[j-1] == py[0])) j--;
    if (j < 3) return;
    n = j;

#ifdef USE_OPENGL
    if (rendmode >= 3)
    {
        float hackscx, hackscy;

        if (skyclamphack) method |= 4;
        pth = gltexcache(globalpicnum,globalpal,method&(~3));

        if (r_fullbrights && pth->flags & 16)
            if (indrawroomsandmasks)
            {
                if (!fullbrightdrawingpass)
                    fullbrightdrawingpass = 1;
                else if (fullbrightdrawingpass == 2)
                    pth = pth->ofb;
            }

        bglBindTexture(GL_TEXTURE_2D, pth ? pth->glpic : 0);

        if (srepeat)
            bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        if (trepeat)
            bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

        // texture scale by parkar request
        if (pth && pth->hicr && ((pth->hicr->xscale != 1.0f) || (pth->hicr->yscale != 1.0f)) && !drawingskybox)
        {
            bglMatrixMode(GL_TEXTURE);
            bglLoadIdentity();
            bglScalef(pth->hicr->xscale, pth->hicr->yscale, 1.0f);
            bglMatrixMode(GL_MODELVIEW);
        }

        // detail texture
        detailpth = NULL;
        if (r_detailmapping && usehightile && !drawingskybox &&
                hicfindsubst(globalpicnum, DETAILPAL, 0))
            detailpth = gltexcache(globalpicnum, DETAILPAL, method&(~3));

        if (detailpth && detailpth->hicr && (detailpth->hicr->palnum == DETAILPAL))
        {
            bglActiveTextureARB(++texunits);

            bglEnable(GL_TEXTURE_2D);
            bglBindTexture(GL_TEXTURE_2D, detailpth ? detailpth->glpic : 0);

            bglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
            bglTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);

            bglTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
            bglTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);

            bglTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
            bglTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

            bglTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
            bglTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
            bglTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

            bglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2.0f);

            bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
            bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);


            f = detailpth ? detailpth->hicr->xscale : 1.0;

            bglMatrixMode(GL_TEXTURE);
            bglLoadIdentity();

            if (pth && pth->hicr && ((pth->hicr->xscale != 1.0f) || (pth->hicr->yscale != 1.0f)))
                bglScalef(pth->hicr->xscale, pth->hicr->yscale, 1.0f);

            if (detailpth && detailpth->hicr && ((detailpth->hicr->xscale != 1.0f) || (detailpth->hicr->yscale != 1.0f)))
                bglScalef(detailpth->hicr->xscale, detailpth->hicr->yscale, 1.0f);

            bglMatrixMode(GL_MODELVIEW);
        }

        // glow texture
        glowpth = NULL;
        if (r_glowmapping && usehightile && !drawingskybox &&
                hicfindsubst(globalpicnum, GLOWPAL, 0))
            glowpth = gltexcache(globalpicnum, GLOWPAL, method&(~3));

        if (glowpth && glowpth->hicr && (glowpth->hicr->palnum == GLOWPAL))
        {
            bglActiveTextureARB(++texunits);

            bglEnable(GL_TEXTURE_2D);
            bglBindTexture(GL_TEXTURE_2D, glowpth ? glowpth->glpic : 0);

            bglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
            bglTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE_ARB);

            bglTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
            bglTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);

            bglTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
            bglTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

            bglTexEnvf(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_TEXTURE);
            bglTexEnvf(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_ONE_MINUS_SRC_ALPHA);

            bglTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
            bglTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
            bglTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

            bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
            bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        }

        if (pth && (pth->flags & 2))
        {
            hackscx = pth->scalex;
            hackscy = pth->scaley;
            tsizx = pth->sizx;
            tsizy = pth->sizy;
        }
        else { hackscx = 1.0; hackscy = 1.0; }

        if (!glinfo.texnpot)
        {
            for (xx=1; xx<tsizx; xx+=xx); ox2 = (double)1.0/(double)xx;
            for (yy=1; yy<tsizy; yy+=yy); oy2 = (double)1.0/(double)yy;
        }
        else
        {
            xx = tsizx; ox2 = (double)1.0/(double)xx;
            yy = tsizy; oy2 = (double)1.0/(double)yy;
        }

        if ((!(method&3)) && (!fullbrightdrawingpass))
        {
            bglDisable(GL_BLEND);
            bglDisable(GL_ALPHA_TEST);
        }
        else
        {
            float al = 0.0; // PLAG : default alphacut was 0.32 before goodalpha
            if (pth && pth->hicr && pth->hicr->alphacut >= 0.0) al = pth->hicr->alphacut;
            if (alphahackarray[globalpicnum])
                al=alphahackarray[globalpicnum];
            if (!waloff[globalpicnum]) al = 0.0;	// invalid textures ignore the alpha cutoff settings
            bglEnable(GL_BLEND);
            bglEnable(GL_ALPHA_TEST);
            bglAlphaFunc(GL_GREATER,al);
        }

        if (!dorot)
        {
            for (i=n-1; i>=0; i--)
            {
                dd[i] = px[i]*gdx + py[i]*gdy + gdo;
                uu[i] = px[i]*gux + py[i]*guy + guo;
                vv[i] = px[i]*gvx + py[i]*gvy + gvo;
            }
        }

        {
            float pc[4];
            f = ((float)(numpalookups-min(max((globalshade * shadescale),0),numpalookups)))/((float)numpalookups);
            pc[0] = pc[1] = pc[2] = f;
            switch (method&3)
            {
            default:
            case 0:
                pc[3] = 1.0f; break;
            case 1:
                pc[3] = 1.0f; break;
            case 2:
                pc[3] = 0.66f; break;
            case 3:
                pc[3] = 0.33f; break;
            }
            // tinting happens only to hightile textures, and only if the texture we're
            // rendering isn't for the same palette as what we asked for
            if (!(hictinting[globalpal].f&4))
            {
                if (pth && (pth->flags & 2))
                {
                    if (pth->palnum != globalpal)
                    {
                        // apply tinting for replaced textures
                        pc[0] *= (float)hictinting[globalpal].r / 255.0;
                        pc[1] *= (float)hictinting[globalpal].g / 255.0;
                        pc[2] *= (float)hictinting[globalpal].b / 255.0;
                    }
                    if (hictinting[MAXPALOOKUPS-1].r != 255 || hictinting[MAXPALOOKUPS-1].g != 255 || hictinting[MAXPALOOKUPS-1].b != 255)
                    {
                        pc[0] *= (float)hictinting[MAXPALOOKUPS-1].r / 255.0;
                        pc[1] *= (float)hictinting[MAXPALOOKUPS-1].g / 255.0;
                        pc[2] *= (float)hictinting[MAXPALOOKUPS-1].b / 255.0;
                    }
                }
                // hack: this is for drawing the 8-bit crosshair recolored in polymost
                else if (hictinting[globalpal].f & 8)
                {
                    pc[0] *= (float)hictinting[globalpal].r / 255.0;
                    pc[1] *= (float)hictinting[globalpal].g / 255.0;
                    pc[2] *= (float)hictinting[globalpal].b / 255.0;
                }
            }

            bglColor4f(pc[0],pc[1],pc[2],pc[3]);
        }

        //Hack for walls&masked walls which use textures that are not a power of 2
        if ((pow2xsplit) && (tsizx != xx))
        {
            if (!dorot)
            {
                ngdx = gdx; ngdy = gdy; ngdo = gdo+(ngdx+ngdy)*.5;
                ngux = gux; nguy = guy; nguo = guo+(ngux+nguy)*.5;
                ngvx = gvx; ngvy = gvy; ngvo = gvo+(ngvx+ngvy)*.5;
            }
            else
            {
                ox = py[1]-py[2]; oy = py[2]-py[0]; oz = py[0]-py[1];
                r = 1.0 / (ox*px[0] + oy*px[1] + oz*px[2]);
                ngdx = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
                ngux = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
                ngvx = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
                ox = px[2]-px[1]; oy = px[0]-px[2]; oz = px[1]-px[0];
                ngdy = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
                nguy = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
                ngvy = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
                ox = px[0]-.5; oy = py[0]-.5; //.5 centers texture nicely
                ngdo = dd[0] - ox*ngdx - oy*ngdy;
                nguo = uu[0] - ox*ngux - oy*nguy;
                ngvo = vv[0] - ox*ngvx - oy*ngvy;
            }

            ngux *= hackscx; nguy *= hackscx; nguo *= hackscx;
            ngvx *= hackscy; ngvy *= hackscy; ngvo *= hackscy;
            uoffs = ((double)(xx-tsizx)*.5);
            ngux -= ngdx*uoffs;
            nguy -= ngdy*uoffs;
            nguo -= ngdo*uoffs;

            //Find min&max u coordinates (du0...du1)
            for (i=0; i<n; i++)
            {
                ox = px[i]; oy = py[i];
                f = (ox*ngux + oy*nguy + nguo) / (ox*ngdx + oy*ngdy + ngdo);
                if (!i) { du0 = du1 = f; continue; }
                if (f < du0) du0 = f;
                else if (f > du1) du1 = f;
            }

            f = 1.0/(double)tsizx;
            ix0 = (int32_t)floor(du0*f);
            ix1 = (int32_t)floor(du1*f);
            for (; ix0<=ix1; ix0++)
            {
                du0 = (double)((ix0)*tsizx);   // + uoffs;
                du1 = (double)((ix0+1)*tsizx); // + uoffs;

                i = 0; nn = 0;
                duj = (px[i]*ngux + py[i]*nguy + nguo) / (px[i]*ngdx + py[i]*ngdy + ngdo);
                do
                {
                    j = i+1; if (j == n) j = 0;

                    dui = duj; duj = (px[j]*ngux + py[j]*nguy + nguo) / (px[j]*ngdx + py[j]*ngdy + ngdo);

                    if ((du0 <= dui) && (dui <= du1)) { uu[nn] = px[i]; vv[nn] = py[i]; nn++; }
                    if (duj <= dui)
                    {
                        if ((du1 < duj) != (du1 < dui))
                        {
                            //ox*(ngux-ngdx*du1) + oy*(nguy-ngdy*du1) + (nguo-ngdo*du1) = 0
                            //(px[j]-px[i])*f + px[i] = ox
                            //(py[j]-py[i])*f + py[i] = oy

                            ///Solve for f
                            //((px[j]-px[i])*f + px[i])*(ngux-ngdx*du1) +
                            //((py[j]-py[i])*f + py[i])*(nguy-ngdy*du1) + (nguo-ngdo*du1) = 0

                            f = -(px[i] *(ngux-ngdx*du1) +  py[i]       *(nguy-ngdy*du1) + (nguo-ngdo*du1)) /
                                ((px[j]-px[i])*(ngux-ngdx*du1) + (py[j]-py[i])*(nguy-ngdy*du1));
                            uu[nn] = (px[j]-px[i])*f + px[i];
                            vv[nn] = (py[j]-py[i])*f + py[i]; nn++;
                        }
                        if ((du0 < duj) != (du0 < dui))
                        {
                            f = -(px[i] *(ngux-ngdx*du0) +        py[i] *(nguy-ngdy*du0) + (nguo-ngdo*du0)) /
                                ((px[j]-px[i])*(ngux-ngdx*du0) + (py[j]-py[i])*(nguy-ngdy*du0));
                            uu[nn] = (px[j]-px[i])*f + px[i];
                            vv[nn] = (py[j]-py[i])*f + py[i]; nn++;
                        }
                    }
                    else
                    {
                        if ((du0 < duj) != (du0 < dui))
                        {
                            f = -(px[i] *(ngux-ngdx*du0) +        py[i] *(nguy-ngdy*du0) + (nguo-ngdo*du0)) /
                                ((px[j]-px[i])*(ngux-ngdx*du0) + (py[j]-py[i])*(nguy-ngdy*du0));
                            uu[nn] = (px[j]-px[i])*f + px[i];
                            vv[nn] = (py[j]-py[i])*f + py[i]; nn++;
                        }
                        if ((du1 < duj) != (du1 < dui))
                        {
                            f = -(px[i] *(ngux-ngdx*du1) +  py[i]       *(nguy-ngdy*du1) + (nguo-ngdo*du1)) /
                                ((px[j]-px[i])*(ngux-ngdx*du1) + (py[j]-py[i])*(nguy-ngdy*du1));
                            uu[nn] = (px[j]-px[i])*f + px[i];
                            vv[nn] = (py[j]-py[i])*f + py[i]; nn++;
                        }
                    }
                    i = j;
                }
                while (i);
                if (nn < 3) continue;

                bglBegin(GL_TRIANGLE_FAN);
                for (i=0; i<nn; i++)
                {
                    ox = uu[i]; oy = vv[i];
                    dp = ox*ngdx + oy*ngdy + ngdo;
                    up = ox*ngux + oy*nguy + nguo;
                    vp = ox*ngvx + oy*ngvy + ngvo;
                    r = 1.0/dp;
                    if (texunits > GL_TEXTURE0_ARB)
                    {
                        j = GL_TEXTURE0_ARB;
                        while (j <= texunits)
                            bglMultiTexCoord2dARB(j++, (up*r-du0+uoffs)*ox2,vp*r*oy2);
                    }
                    else
                        bglTexCoord2d((up*r-du0+uoffs)*ox2,vp*r*oy2);
                    bglVertex3d((ox-ghalfx)*r*grhalfxdown10x,(ghoriz-oy)*r*grhalfxdown10,r*(1.0/1024.0));
                }
                bglEnd();
            }
        }
        else
        {
            ox2 *= hackscx; oy2 *= hackscy;
            bglBegin(GL_TRIANGLE_FAN);
            for (i=0; i<n; i++)
            {
                r = 1.0/dd[i];
                if (texunits > GL_TEXTURE0_ARB)
                {
                    j = GL_TEXTURE0_ARB;
                    while (j <= texunits)
                        bglMultiTexCoord2dARB(j++, uu[i]*r*ox2,vv[i]*r*oy2);
                }
                else
                    bglTexCoord2d(uu[i]*r*ox2,vv[i]*r*oy2);
                bglVertex3d((px[i]-ghalfx)*r*grhalfxdown10x,(ghoriz-py[i])*r*grhalfxdown10,r*(1.0/1024.0));
            }
            bglEnd();
        }

        while (texunits >= GL_TEXTURE0_ARB)
        {
            bglActiveTextureARB(texunits);
            bglMatrixMode(GL_TEXTURE);
            bglLoadIdentity();
            bglMatrixMode(GL_MODELVIEW);
            if (texunits > GL_TEXTURE0_ARB)
            {
                bglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1.0f);
                bglDisable(GL_TEXTURE_2D);
            }
            texunits--;
        }

        if (srepeat)
            bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
        if (trepeat)
            bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);

        if (fullbrightdrawingpass == 1) // tile has fullbright colors ?
        {
            fullbrightdrawingpass = 2;
            shadeforfullbrightpass = globalshade; // save the current shade
            globalshade = -128; // fullbright
            bglDisable(GL_FOG);
            drawpoly(dpx, dpy, n_, method_); // draw them afterwards, then. :)
            bglEnable(GL_FOG);
            globalshade = shadeforfullbrightpass;
            fullbrightdrawingpass = 0;
        }
        return;
    }
#endif

    if (rendmode == 2)
    {
#if (USEZBUFFER != 0)
        if ((!zbufmem) || (zbufbpl != bytesperline) || (zbufysiz != ydim))
        {
            zbufbpl = bytesperline;
            zbufysiz = ydim;
            zbufmem = (intptr_t)Brealloc((void *)zbufmem,zbufbpl*zbufysiz*4);
        }
        zbufoff = (int32_t *)(zbufmem-(frameplace<<2));
#endif
        if ((!transluc)) method = (method&~3)+1; //In case translucent table doesn't exist

        if (!dorot)
        {
            ngdx = gdx; ngdy = gdy; ngdo = gdo+(ngdx+ngdy)*.5;
            ngux = gux; nguy = guy; nguo = guo+(ngux+nguy)*.5;
            ngvx = gvx; ngvy = gvy; ngvo = gvo+(ngvx+ngvy)*.5;
        }
        else
        {
            //General equations:
            //dd[i] = (px[i]*gdx + py[i]*gdy + gdo)
            //uu[i] = (px[i]*gux + py[i]*guy + guo)/dd[i]
            //vv[i] = (px[i]*gvx + py[i]*gvy + gvo)/dd[i]
            //
            //px[0]*gdx + py[0]*gdy + 1*gdo = dd[0]
            //px[1]*gdx + py[1]*gdy + 1*gdo = dd[1]
            //px[2]*gdx + py[2]*gdy + 1*gdo = dd[2]
            //
            //px[0]*gux + py[0]*guy + 1*guo = uu[0]*dd[0] (uu[i] premultiplied by dd[i] above)
            //px[1]*gux + py[1]*guy + 1*guo = uu[1]*dd[1]
            //px[2]*gux + py[2]*guy + 1*guo = uu[2]*dd[2]
            //
            //px[0]*gvx + py[0]*gvy + 1*gvo = vv[0]*dd[0] (vv[i] premultiplied by dd[i] above)
            //px[1]*gvx + py[1]*gvy + 1*gvo = vv[1]*dd[1]
            //px[2]*gvx + py[2]*gvy + 1*gvo = vv[2]*dd[2]
            ox = py[1]-py[2]; oy = py[2]-py[0]; oz = py[0]-py[1];
            r = 1.0 / (ox*px[0] + oy*px[1] + oz*px[2]);
            ngdx = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
            ngux = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
            ngvx = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
            ox = px[2]-px[1]; oy = px[0]-px[2]; oz = px[1]-px[0];
            ngdy = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
            nguy = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
            ngvy = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
            ox = px[0]-.5; oy = py[0]-.5; //.5 centers texture nicely
            ngdo = dd[0] - ox*ngdx - oy*ngdy;
            nguo = uu[0] - ox*ngux - oy*nguy;
            ngvo = vv[0] - ox*ngvx - oy*ngvy;
        }
        palptr = &palookup[globalpal][min(max((int32_t)(globalshade * shadescale),0),numpalookups-1)<<8]; //<-need to make shade not static!

        tsizxm1 = tsizx-1; xmodnice = (!(tsizxm1&tsizx));
        tsizym1 = tsizy-1; ymulnice = (!(tsizym1&tsizy));
        if ((method&4) && (!xmodnice)) //Sprites don't need a mod on texture coordinates
            { xmodnice = 1; for (tsizxm1=1; tsizxm1<tsizx; tsizxm1=(tsizxm1<<1)+1); }
        if (!ymulnice) { for (tsizym1=1; tsizym1+1<tsizy; tsizym1=(tsizym1<<1)+1); }
        ltsizy = (picsiz[globalpicnum]>>4);
    }
    else
    {
        dacol = palookup[0][(int32_t)(*(char *)(waloff[globalpicnum]))+(min(max((int32_t)(globalshade * shadescale),0),numpalookups-1)<<8)];
    }

    if (grhalfxdown10x < 0) //Hack for mirrors
    {
        for (i=((n-1)>>1); i>=0; i--)
        {
            r = px[i]; px[i] = ((double)xdimen)-px[n-1-i]; px[n-1-i] = ((double)xdimen)-r;
            r = py[i]; py[i] = py[n-1-i]; py[n-1-i] = r;
        }
        ngdo += ((double)xdimen)*ngdx; ngdx = -ngdx;
        nguo += ((double)xdimen)*ngux; ngux = -ngux;
        ngvo += ((double)xdimen)*ngvx; ngvx = -ngvx;
    }

    ngdx2 = ngdx*(1<<LINTERPSIZ);
    ngux2 = ngux*(1<<LINTERPSIZ);
    ngvx2 = ngvx*(1<<LINTERPSIZ);

    mini = (py[0] >= py[1]); maxi = 1-mini;
    for (z=2; z<n; z++)
    {
        if (py[z] < py[mini]) mini = z;
        if (py[z] > py[maxi]) maxi = z;
    }

    i = maxi; dtol(py[i],&yy); if (yy > ydimen) yy = ydimen;
    do
    {
        j = i+1; if (j == n) j = 0;
        dtol(py[j],&y); if (y < 0) y = 0;
        if (y < yy)
        {
            f = (px[j]-px[i])/(py[j]-py[i]); dtol(f*16384.0,&xi);
            dtol((((double)yy-.5-py[j])*f + px[j])*16384.0+8192.0,&x);
            for (; yy>y; yy--,x-=xi) lastx[yy-1] = (x>>14);
        }
        i = j;
    }
    while (i != mini);
    do
    {
        j = i+1; if (j == n) j = 0;
        dtol(py[j],&yy); if (yy > ydimen) yy = ydimen;
        if (y < yy)
        {
            f = (px[j]-px[i])/(py[j]-py[i]); dtol(f*16384.0,&xi);
            dtol((((double)y+.5-py[j])*f + px[j])*16384.0+8192.0,&x);
            for (; y<yy; y++,x+=xi)
            {
                ix0 = lastx[y]; if (ix0 < 0) ix0 = 0;
                ix1 = (x>>14); if (ix1 > xdimen) ix1 = xdimen;
                if (ix0 < ix1)
                {
                    if (rendmode == 1)
                        memset((void *)(ylookup[y]+ix0+frameoffset),dacol,ix1-ix0);
                    else
                    {
                        vidp = (char *)(ylookup[y]+frameoffset+ix0);
                        dp = ngdx*(double)ix0 + ngdy*(double)y + ngdo;
                        up = ngux*(double)ix0 + nguy*(double)y + nguo;
                        vp = ngvx*(double)ix0 + ngvy*(double)y + ngvo;
                        rdp = 65536.0/dp; dp += ngdx2;
                        dtol(rdp,&d0);
                        dtol(up*rdp,&u0); up += ngux2;
                        dtol(vp*rdp,&v0); vp += ngvx2;
                        rdp = 65536.0/dp;

                        switch (method&3)
                        {
                        case 0:
                            if (xmodnice&ymulnice) //both u&v texture sizes are powers of 2 :)
                            {
                                for (xx=ix0; xx<ix1; xx+=(1<<LINTERPSIZ))
                                {
                                    dtol(rdp,&d1); dp += ngdx2; d1 = ((d1-d0)>>LINTERPSIZ);
                                    dtol(up*rdp,&u1); up += ngux2; u1 = ((u1-u0)>>LINTERPSIZ);
                                    dtol(vp*rdp,&v1); vp += ngvx2; v1 = ((v1-v0)>>LINTERPSIZ);
                                    rdp = 65536.0/dp; vide = &vidp[min(ix1-xx,1<<LINTERPSIZ)];
                                    while (vidp < vide)
                                    {
#if (DEPTHDEBUG == 0)
#if (USEZBUFFER != 0)
                                        zbufoff[(intptr_t)vidp] = d0+16384; //+ hack so wall&floor sprites don't disappear
#endif
                                        vidp[0] = palptr[walptr[(((u0>>16)&tsizxm1)<<ltsizy) + ((v0>>16)&tsizym1)]]; //+((d0>>13)&0x3f00)];
#else
                                        vidp[0] = ((d0>>16)&255);
#endif
                                        d0 += d1; u0 += u1; v0 += v1; vidp++;
                                    }
                                }
                            }
                            else
                            {
                                for (xx=ix0; xx<ix1; xx+=(1<<LINTERPSIZ))
                                {
                                    dtol(rdp,&d1); dp += ngdx2; d1 = ((d1-d0)>>LINTERPSIZ);
                                    dtol(up*rdp,&u1); up += ngux2; u1 = ((u1-u0)>>LINTERPSIZ);
                                    dtol(vp*rdp,&v1); vp += ngvx2; v1 = ((v1-v0)>>LINTERPSIZ);
                                    rdp = 65536.0/dp; vide = &vidp[min(ix1-xx,1<<LINTERPSIZ)];
                                    while (vidp < vide)
                                    {
#if (DEPTHDEBUG == 0)
#if (USEZBUFFER != 0)
                                        zbufoff[(intptr_t)vidp] = d0;
#endif
                                        vidp[0] = palptr[walptr[imod(u0>>16,tsizx)*tsizy + ((v0>>16)&tsizym1)]]; //+((d0>>13)&0x3f00)];
#else
                                        vidp[0] = ((d0>>16)&255);
#endif
                                        d0 += d1; u0 += u1; v0 += v1; vidp++;
                                    }
                                }
                            }
                            break;
                        case 1:
                            if (xmodnice) //both u&v texture sizes are powers of 2 :)
                            {
                                for (xx=ix0; xx<ix1; xx+=(1<<LINTERPSIZ))
                                {
                                    dtol(rdp,&d1); dp += ngdx2; d1 = ((d1-d0)>>LINTERPSIZ);
                                    dtol(up*rdp,&u1); up += ngux2; u1 = ((u1-u0)>>LINTERPSIZ);
                                    dtol(vp*rdp,&v1); vp += ngvx2; v1 = ((v1-v0)>>LINTERPSIZ);
                                    rdp = 65536.0/dp; vide = &vidp[min(ix1-xx,1<<LINTERPSIZ)];
                                    while (vidp < vide)
                                    {
                                        dacol = walptr[(((u0>>16)&tsizxm1)*tsizy) + ((v0>>16)&tsizym1)];
#if (DEPTHDEBUG == 0)
#if (USEZBUFFER != 0)
                                        if ((dacol != 255) && (d0 <= zbufoff[(intptr_t)vidp]))
                                        {
                                            zbufoff[(intptr_t)vidp] = d0;
                                            vidp[0] = palptr[((int32_t)dacol)]; //+((d0>>13)&0x3f00)];
                                        }
#else
                                        if (dacol != 255) vidp[0] = palptr[((int32_t)dacol)]; //+((d0>>13)&0x3f00)];
#endif
#else
                                        if ((dacol != 255) && (vidp[0] > (d0>>16))) vidp[0] = ((d0>>16)&255);
#endif
                                        d0 += d1; u0 += u1; v0 += v1; vidp++;
                                    }
                                }
                            }
                            else
                            {
                                for (xx=ix0; xx<ix1; xx+=(1<<LINTERPSIZ))
                                {
                                    dtol(rdp,&d1); dp += ngdx2; d1 = ((d1-d0)>>LINTERPSIZ);
                                    dtol(up*rdp,&u1); up += ngux2; u1 = ((u1-u0)>>LINTERPSIZ);
                                    dtol(vp*rdp,&v1); vp += ngvx2; v1 = ((v1-v0)>>LINTERPSIZ);
                                    rdp = 65536.0/dp; vide = &vidp[min(ix1-xx,1<<LINTERPSIZ)];
                                    while (vidp < vide)
                                    {
                                        dacol = walptr[imod(u0>>16,tsizx)*tsizy + ((v0>>16)&tsizym1)];
#if (DEPTHDEBUG == 0)
#if (USEZBUFFER != 0)
                                        if ((dacol != 255) && (d0 <= zbufoff[(intptr_t)vidp]))
                                        {
                                            zbufoff[(intptr_t)vidp] = d0;
                                            vidp[0] = palptr[((int32_t)dacol)]; //+((d0>>13)&0x3f00)];
                                        }
#else
                                        if (dacol != 255) vidp[0] = palptr[((int32_t)dacol)]; //+((d0>>13)&0x3f00)];
#endif
#else
                                        if ((dacol != 255) && (vidp[0] > (d0>>16))) vidp[0] = ((d0>>16)&255);
#endif
                                        d0 += d1; u0 += u1; v0 += v1; vidp++;
                                    }
                                }
                            }
                            break;
                        case 2: //Transluscence #1
                            for (xx=ix0; xx<ix1; xx+=(1<<LINTERPSIZ))
                            {
                                dtol(rdp,&d1); dp += ngdx2; d1 = ((d1-d0)>>LINTERPSIZ);
                                dtol(up*rdp,&u1); up += ngux2; u1 = ((u1-u0)>>LINTERPSIZ);
                                dtol(vp*rdp,&v1); vp += ngvx2; v1 = ((v1-v0)>>LINTERPSIZ);
                                rdp = 65536.0/dp; vide = &vidp[min(ix1-xx,1<<LINTERPSIZ)];
                                while (vidp < vide)
                                {
                                    dacol = walptr[imod(u0>>16,tsizx)*tsizy + ((v0>>16)&tsizym1)];
                                    //dacol = walptr[(((u0>>16)&tsizxm1)<<ltsizy) + ((v0>>16)&tsizym1)];
#if (DEPTHDEBUG == 0)
#if (USEZBUFFER != 0)
                                    if ((dacol != 255) && (d0 <= zbufoff[(intptr_t)vidp]))
                                    {
                                        zbufoff[(intptr_t)vidp] = d0;
                                        vidp[0] = transluc[(((int32_t)vidp[0])<<8)+((int32_t)palptr[((int32_t)dacol)])]; //+((d0>>13)&0x3f00)])];
                                    }
#else
                                    if (dacol != 255)
                                        vidp[0] = transluc[(((int32_t)vidp[0])<<8)+((int32_t)palptr[((int32_t)dacol)])]; //+((d0>>13)&0x3f00)])];
#endif
#else
                                    if ((dacol != 255) && (vidp[0] > (d0>>16))) vidp[0] = ((d0>>16)&255);
#endif
                                    d0 += d1; u0 += u1; v0 += v1; vidp++;
                                }
                            }
                            break;
                        case 3: //Transluscence #2
                            for (xx=ix0; xx<ix1; xx+=(1<<LINTERPSIZ))
                            {
                                dtol(rdp,&d1); dp += ngdx2; d1 = ((d1-d0)>>LINTERPSIZ);
                                dtol(up*rdp,&u1); up += ngux2; u1 = ((u1-u0)>>LINTERPSIZ);
                                dtol(vp*rdp,&v1); vp += ngvx2; v1 = ((v1-v0)>>LINTERPSIZ);
                                rdp = 65536.0/dp; vide = &vidp[min(ix1-xx,1<<LINTERPSIZ)];
                                while (vidp < vide)
                                {
                                    dacol = walptr[imod(u0>>16,tsizx)*tsizy + ((v0>>16)&tsizym1)];
                                    //dacol = walptr[(((u0>>16)&tsizxm1)<<ltsizy) + ((v0>>16)&tsizym1)];
#if (DEPTHDEBUG == 0)
#if (USEZBUFFER != 0)
                                    if ((dacol != 255) && (d0 <= zbufoff[(intptr_t)vidp]))
                                    {
                                        zbufoff[(intptr_t)vidp] = d0;
                                        vidp[0] = transluc[((int32_t)vidp[0])+(((int32_t)palptr[((int32_t)dacol)/*+((d0>>13)&0x3f00)*/])<<8)];
                                    }
#else
                                    if (dacol != 255)
                                        vidp[0] = transluc[((int32_t)vidp[0])+(((int32_t)palptr[((int32_t)dacol)/*+((d0>>13)&0x3f00)*/])<<8)];
#endif
#else
                                    if ((dacol != 255) && (vidp[0] > (d0>>16))) vidp[0] = ((d0>>16)&255);
#endif
                                    d0 += d1; u0 += u1; v0 += v1; vidp++;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
        i = j;
    }
    while (i != maxi);

    if (rendmode == 1)
    {
        if (method&3) //Only draw border around sprites/maskwalls
        {
            for (i=0,j=n-1; i<n; j=i,i++) drawline2d(px[i],py[i],px[j],py[j],31); //hopefully color index 31 is white
        }

        //ox = 0; oy = 0;
        //for(i=0;i<n;i++) { ox += px[i]; oy += py[i]; }
        //ox /= (double)n; oy /= (double)n;
        //for(i=0,j=n-1;i<n;j=i,i++) drawline2d(px[i]+(ox-px[i])*.125,py[i]+(oy-py[i])*.125,px[j]+(ox-px[j])*.125,py[j]+(oy-py[j])*.125,31);
    }
}

/*Init viewport boundary (must be 4 point convex loop):
//      (px[0],py[0]).----.(px[1],py[1])
//                  /      \
//                /          \
// (px[3],py[3]).--------------.(px[2],py[2])
*/
void initmosts(double *px, double *py, int32_t n)
{
    int32_t i, j, k, imin;

    vcnt = 1; //0 is dummy solid node

    if (n < 3) return;
    imin = (px[1] < px[0]);
    for (i=n-1; i>=2; i--) if (px[i] < px[imin]) imin = i;


    vsp[vcnt].x = px[imin];
    vsp[vcnt].cy[0] = vsp[vcnt].fy[0] = py[imin];
    vcnt++;
    i = imin+1; if (i >= n) i = 0;
    j = imin-1; if (j < 0) j = n-1;
    do
    {
        if (px[i] < px[j])
        {
            if ((vcnt > 1) && (px[i] <= vsp[vcnt-1].x)) vcnt--;
            vsp[vcnt].x = px[i];
            vsp[vcnt].cy[0] = py[i];
            k = j+1; if (k >= n) k = 0;
            //(px[k],py[k])
            //(px[i],?)
            //(px[j],py[j])
            vsp[vcnt].fy[0] = (px[i]-px[k])*(py[j]-py[k])/(px[j]-px[k]) + py[k];
            vcnt++;
            i++; if (i >= n) i = 0;
        }
        else if (px[j] < px[i])
        {
            if ((vcnt > 1) && (px[j] <= vsp[vcnt-1].x)) vcnt--;
            vsp[vcnt].x = px[j];
            vsp[vcnt].fy[0] = py[j];
            k = i-1; if (k < 0) k = n-1;
            //(px[k],py[k])
            //(px[j],?)
            //(px[i],py[i])
            vsp[vcnt].cy[0] = (px[j]-px[k])*(py[i]-py[k])/(px[i]-px[k]) + py[k];
            vcnt++;
            j--; if (j < 0) j = n-1;
        }
        else
        {
            if ((vcnt > 1) && (px[i] <= vsp[vcnt-1].x)) vcnt--;
            vsp[vcnt].x = px[i];
            vsp[vcnt].cy[0] = py[i];
            vsp[vcnt].fy[0] = py[j];
            vcnt++;
            i++; if (i >= n) i = 0; if (i == j) break;
            j--; if (j < 0) j = n-1;
        }
    }
    while (i != j);
    if (px[i] > vsp[vcnt-1].x)
    {
        vsp[vcnt].x = px[i];
        vsp[vcnt].cy[0] = vsp[vcnt].fy[0] = py[i];
        vcnt++;
    }


    for (i=0; i<vcnt; i++)
    {
        vsp[i].cy[1] = vsp[i+1].cy[0]; vsp[i].ctag = i;
        vsp[i].fy[1] = vsp[i+1].fy[0]; vsp[i].ftag = i;
        vsp[i].n = i+1; vsp[i].p = i-1;
    }
    vsp[vcnt-1].n = 0; vsp[0].p = vcnt-1;
    gtag = vcnt;

    //VSPMAX-1 is dummy empty node
    for (i=vcnt; i<VSPMAX; i++) { vsp[i].n = i+1; vsp[i].p = i-1; }
    vsp[VSPMAX-1].n = vcnt; vsp[vcnt].p = VSPMAX-1;
}

static inline void vsdel(int32_t i)
{
    int32_t pi, ni;
    //Delete i
    pi = vsp[i].p;
    ni = vsp[i].n;
    vsp[ni].p = pi;
    vsp[pi].n = ni;

    //Add i to empty list
    vsp[i].n = vsp[VSPMAX-1].n;
    vsp[i].p = VSPMAX-1;
    vsp[vsp[VSPMAX-1].n].p = i;
    vsp[VSPMAX-1].n = i;
}

static inline int32_t vsinsaft(int32_t i)
{
    int32_t r;
    //i = next element from empty list
    r = vsp[VSPMAX-1].n;
    vsp[vsp[r].n].p = VSPMAX-1;
    vsp[VSPMAX-1].n = vsp[r].n;

    vsp[r] = vsp[i]; //copy i to r

    //insert r after i
    vsp[r].p = i; vsp[r].n = vsp[i].n;
    vsp[vsp[i].n].p = r; vsp[i].n = r;

    return(r);
}

static inline int32_t testvisiblemost(float x0, float x1)
{
    int32_t i, newi;

    for (i=vsp[0].n; i; i=newi)
    {
        newi = vsp[i].n;
        if ((x0 < vsp[newi].x) && (vsp[i].x < x1) && (vsp[i].ctag >= 0)) return(1);
    }
    return(0);
}

static int32_t domostpolymethod = 0;

void domost(float x0, float y0, float x1, float y1)
{
    double dpx[4], dpy[4];
    float d, f, n, t, slop, dx, dx0, dx1, nx, nx0, ny0, nx1, ny1;
    float spx[4], spy[4], cy[2], cv[2];
    int32_t i, j, k, z, ni, vcnt = 0, scnt, newi, dir, spt[4];

    if (x0 < x1)
    {
        dir = 1; //clip dmost (floor)
        y0 -= .01f; y1 -= .01f;
    }
    else
    {
        if (x0 == x1) return;
        f = x0; x0 = x1; x1 = f;
        f = y0; y0 = y1; y1 = f;
        dir = 0; //clip umost (ceiling)
        //y0 += .01; y1 += .01; //necessary?
    }

    slop = (y1-y0)/(x1-x0);
    for (i=vsp[0].n; i; i=newi)
    {
        newi = vsp[i].n; nx0 = vsp[i].x; nx1 = vsp[newi].x;
        if ((x0 >= nx1) || (nx0 >= x1) || (vsp[i].ctag <= 0)) continue;
        dx = nx1-nx0;
        cy[0] = vsp[i].cy[0]; cv[0] = vsp[i].cy[1]-cy[0];
        cy[1] = vsp[i].fy[0]; cv[1] = vsp[i].fy[1]-cy[1];

        scnt = 0;

        //Test if left edge requires split (x0,y0) (nx0,cy(0)),<dx,cv(0)>
        if ((x0 > nx0) && (x0 < nx1))
        {
            t = (x0-nx0)*cv[dir] - (y0-cy[dir])*dx;
            if (((!dir) && (t < 0)) || ((dir) && (t > 0)))
                { spx[scnt] = x0; spy[scnt] = y0; spt[scnt] = -1; scnt++; }
        }

        //Test for intersection on umost (j == 0) and dmost (j == 1)
        for (j=0; j<2; j++)
        {
            d = (y0-y1)*dx - (x0-x1)*cv[j];
            n = (y0-cy[j])*dx - (x0-nx0)*cv[j];
            if ((fabs(n) <= fabs(d)) && (d*n >= 0) && (d != 0))
            {
                t = n/d; nx = (x1-x0)*t + x0;
                if ((nx > nx0) && (nx < nx1))
                {
                    spx[scnt] = nx; spy[scnt] = (y1-y0)*t + y0;
                    spt[scnt] = j; scnt++;
                }
            }
        }

        //Nice hack to avoid full sort later :)
        if ((scnt >= 2) && (spx[scnt-1] < spx[scnt-2]))
        {
            f = spx[scnt-1]; spx[scnt-1] = spx[scnt-2]; spx[scnt-2] = f;
            f = spy[scnt-1]; spy[scnt-1] = spy[scnt-2]; spy[scnt-2] = f;
            j = spt[scnt-1]; spt[scnt-1] = spt[scnt-2]; spt[scnt-2] = j;
        }

        //Test if right edge requires split
        if ((x1 > nx0) && (x1 < nx1))
        {
            t = (x1-nx0)*cv[dir] - (y1-cy[dir])*dx;
            if (((!dir) && (t < 0)) || ((dir) && (t > 0)))
                { spx[scnt] = x1; spy[scnt] = y1; spt[scnt] = -1; scnt++; }
        }

        vsp[i].tag = vsp[newi].tag = -1;
        for (z=0; z<=scnt; z++,i=vcnt)
        {
            if (z < scnt)
            {
                vcnt = vsinsaft(i);
                t = (spx[z]-nx0)/dx;
                vsp[i].cy[1] = t*cv[0] + cy[0];
                vsp[i].fy[1] = t*cv[1] + cy[1];
                vsp[vcnt].x = spx[z];
                vsp[vcnt].cy[0] = vsp[i].cy[1];
                vsp[vcnt].fy[0] = vsp[i].fy[1];
                vsp[vcnt].tag = spt[z];
            }

            ni = vsp[i].n; if (!ni) continue; //this 'if' fixes many bugs!
            dx0 = vsp[i].x; if (x0 > dx0) continue;
            dx1 = vsp[ni].x; if (x1 < dx1) continue;
            ny0 = (dx0-x0)*slop + y0;
            ny1 = (dx1-x0)*slop + y0;

            //      dx0           dx1
            //       ~             ~
            //----------------------------
            //     t0+=0         t1+=0
            //   vsp[i].cy[0]  vsp[i].cy[1]
            //============================
            //     t0+=1         t1+=3
            //============================
            //   vsp[i].fy[0]    vsp[i].fy[1]
            //     t0+=2         t1+=6
            //
            //     ny0 ?         ny1 ?

            k = 1+3;
            if ((vsp[i].tag == 0) || (ny0 <= vsp[i].cy[0]+.01)) k--;
            if ((vsp[i].tag == 1) || (ny0 >= vsp[i].fy[0]-.01)) k++;
            if ((vsp[ni].tag == 0) || (ny1 <= vsp[i].cy[1]+.01)) k -= 3;
            if ((vsp[ni].tag == 1) || (ny1 >= vsp[i].fy[1]-.01)) k += 3;

            if (!dir)
            {
                switch (k)
                {
                case 1:
                case 2:
                    dpx[0] = dx0; dpy[0] = vsp[i].cy[0];
                    dpx[1] = dx1; dpy[1] = vsp[i].cy[1];
                    dpx[2] = dx0; dpy[2] = ny0; drawpoly(dpx,dpy,3,domostpolymethod);
                    vsp[i].cy[0] = ny0; vsp[i].ctag = gtag; break;
                case 3:
                case 6:
                    dpx[0] = dx0; dpy[0] = vsp[i].cy[0];
                    dpx[1] = dx1; dpy[1] = vsp[i].cy[1];
                    dpx[2] = dx1; dpy[2] = ny1; drawpoly(dpx,dpy,3,domostpolymethod);
                    vsp[i].cy[1] = ny1; vsp[i].ctag = gtag; break;
                case 4:
                case 5:
                case 7:
                    dpx[0] = dx0; dpy[0] = vsp[i].cy[0];
                    dpx[1] = dx1; dpy[1] = vsp[i].cy[1];
                    dpx[2] = dx1; dpy[2] = ny1;
                    dpx[3] = dx0; dpy[3] = ny0; drawpoly(dpx,dpy,4,domostpolymethod);
                    vsp[i].cy[0] = ny0; vsp[i].cy[1] = ny1; vsp[i].ctag = gtag; break;
                case 8:
                    dpx[0] = dx0; dpy[0] = vsp[i].cy[0];
                    dpx[1] = dx1; dpy[1] = vsp[i].cy[1];
                    dpx[2] = dx1; dpy[2] = vsp[i].fy[1];
                    dpx[3] = dx0; dpy[3] = vsp[i].fy[0]; drawpoly(dpx,dpy,4,domostpolymethod);
                    vsp[i].ctag = vsp[i].ftag = -1; break;
                default:
                    break;
                }
            }
            else
            {
                switch (k)
                {
                case 7:
                case 6:
                    dpx[0] = dx0; dpy[0] = ny0;
                    dpx[1] = dx1; dpy[1] = vsp[i].fy[1];
                    dpx[2] = dx0; dpy[2] = vsp[i].fy[0]; drawpoly(dpx,dpy,3,domostpolymethod);
                    vsp[i].fy[0] = ny0; vsp[i].ftag = gtag; break;
                case 5:
                case 2:
                    dpx[0] = dx0; dpy[0] = vsp[i].fy[0];
                    dpx[1] = dx1; dpy[1] = ny1;
                    dpx[2] = dx1; dpy[2] = vsp[i].fy[1]; drawpoly(dpx,dpy,3,domostpolymethod);
                    vsp[i].fy[1] = ny1; vsp[i].ftag = gtag; break;
                case 4:
                case 3:
                case 1:
                    dpx[0] = dx0; dpy[0] = ny0;
                    dpx[1] = dx1; dpy[1] = ny1;
                    dpx[2] = dx1; dpy[2] = vsp[i].fy[1];
                    dpx[3] = dx0; dpy[3] = vsp[i].fy[0]; drawpoly(dpx,dpy,4,domostpolymethod);
                    vsp[i].fy[0] = ny0; vsp[i].fy[1] = ny1; vsp[i].ftag = gtag; break;
                case 0:
                    dpx[0] = dx0; dpy[0] = vsp[i].cy[0];
                    dpx[1] = dx1; dpy[1] = vsp[i].cy[1];
                    dpx[2] = dx1; dpy[2] = vsp[i].fy[1];
                    dpx[3] = dx0; dpy[3] = vsp[i].fy[0]; drawpoly(dpx,dpy,4,domostpolymethod);
                    vsp[i].ctag = vsp[i].ftag = -1; break;
                default:
                    break;
                }
            }
        }
    }

    gtag++;

    //Combine neighboring vertical strips with matching collinear top&bottom edges
    //This prevents x-splits from propagating through the entire scan
    i = vsp[0].n;
    while (i)
    {
        ni = vsp[i].n;
        if ((vsp[i].cy[0] >= vsp[i].fy[0]) && (vsp[i].cy[1] >= vsp[i].fy[1])) { vsp[i].ctag = vsp[i].ftag = -1; }
        if ((vsp[i].ctag == vsp[ni].ctag) && (vsp[i].ftag == vsp[ni].ftag))
            { vsp[i].cy[1] = vsp[ni].cy[1]; vsp[i].fy[1] = vsp[ni].fy[1]; vsdel(ni); }
        else i = ni;
    }
}

static void polymost_scansector(int32_t sectnum);

static void polymost_drawalls(int32_t bunch)
{
    sectortype *sec, *nextsec;
    walltype *wal, *wal2, *nwal;
    double ox, oy, oz, ox2, oy2, px[3], py[3], dd[3], uu[3], vv[3];
    double fx, fy, x0, x1, cy0, cy1, fy0, fy1, xp0, yp0, xp1, yp1, ryp0, ryp1, nx0, ny0, nx1, ny1;
    double t, r, t0, t1, ocy0, ocy1, ofy0, ofy1, oxp0, oyp0, ft[4];
    double oguo, ogux, oguy;
    int32_t i, x, y, z, cz, fz, wallnum, sectnum, nextsectnum;
    int32_t ypan,yoffs; // for panning correction

    sectnum = thesector[bunchfirst[bunch]]; sec = &sector[sectnum];

#if 0 // USE_OPENGL
    if (!nofog)
    {
        if (rendmode >= 3)
        {
            float col[4];
            col[0] = (float)palookupfog[sec->floorpal].r / 63.f;
            col[1] = (float)palookupfog[sec->floorpal].g / 63.f;
            col[2] = (float)palookupfog[sec->floorpal].b / 63.f;
            col[3] = 0;
            bglFogfv(GL_FOG_COLOR,col);
            bglFogf(GL_FOG_DENSITY,fogcalc(sec->floorshade,sec->visibility));

            //            bglFogf(GL_FOG_DENSITY,gvisibility*((float)((uint8_t)(sec->visibility<240?sec->visibility+16:sec->visibility-239))));
        }
    }
#endif

    //DRAW WALLS SECTION!
    for (z=bunchfirst[bunch]; z>=0; z=p2[z])
    {
        wallnum = thewall[z]; wal = &wall[wallnum]; wal2 = &wall[wal->point2];
        nextsectnum = wal->nextsector; nextsec = &sector[nextsectnum];

        //Offset&Rotate 3D coordinates to screen 3D space
        x = wal->x-globalposx; y = wal->y-globalposy;
        xp0 = (double)y*gcosang  - (double)x*gsinang;
        yp0 = (double)x*gcosang2 + (double)y*gsinang2;
        x = wal2->x-globalposx; y = wal2->y-globalposy;
        xp1 = (double)y*gcosang  - (double)x*gsinang;
        yp1 = (double)x*gcosang2 + (double)y*gsinang2;

        oxp0 = xp0; oyp0 = yp0;

        //Clip to close parallel-screen plane
        if (yp0 < SCISDIST)
        {
            if (yp1 < SCISDIST) continue;
            t0 = (SCISDIST-yp0)/(yp1-yp0); xp0 = (xp1-xp0)*t0+xp0; yp0 = SCISDIST;
            nx0 = (wal2->x-wal->x)*t0+wal->x;
            ny0 = (wal2->y-wal->y)*t0+wal->y;
        }
        else { t0 = 0.f; nx0 = wal->x; ny0 = wal->y; }
        if (yp1 < SCISDIST)
        {
            t1 = (SCISDIST-oyp0)/(yp1-oyp0); xp1 = (xp1-oxp0)*t1+oxp0; yp1 = SCISDIST;
            nx1 = (wal2->x-wal->x)*t1+wal->x;
            ny1 = (wal2->y-wal->y)*t1+wal->y;
        }
        else { t1 = 1.f; nx1 = wal2->x; ny1 = wal2->y; }

        ryp0 = 1.f/yp0; ryp1 = 1.f/yp1;

        //Generate screen coordinates for front side of wall
        x0 = ghalfx*xp0*ryp0 + ghalfx;
        x1 = ghalfx*xp1*ryp1 + ghalfx;
        if (x1 <= x0) continue;

        ryp0 *= gyxscale; ryp1 *= gyxscale;

        getzsofslope(sectnum,(int32_t)nx0,(int32_t)ny0,&cz,&fz);
        cy0 = ((float)(cz-globalposz))*ryp0 + ghoriz;
        fy0 = ((float)(fz-globalposz))*ryp0 + ghoriz;
        getzsofslope(sectnum,(int32_t)nx1,(int32_t)ny1,&cz,&fz);
        cy1 = ((float)(cz-globalposz))*ryp1 + ghoriz;
        fy1 = ((float)(fz-globalposz))*ryp1 + ghoriz;


        globalpicnum = sec->floorpicnum; globalshade = sec->floorshade; globalpal = (int32_t)((uint8_t)sec->floorpal);
        globalorientation = sec->floorstat;
        if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,sectnum);
        if (!(globalorientation&1))
        {
            //(singlobalang/-16384*(sx-ghalfx) + 0*(sy-ghoriz) + (cosviewingrangeglobalang/16384)*ghalfx)*d + globalposx    = u*16
            //(cosglobalang/ 16384*(sx-ghalfx) + 0*(sy-ghoriz) + (sinviewingrangeglobalang/16384)*ghalfx)*d + globalposy    = v*16
            //(                  0*(sx-ghalfx) + 1*(sy-ghoriz) + (                             0)*ghalfx)*d + globalposz/16 = (sec->floorz/16)
            if (!(globalorientation&64))
                { ft[0] = globalposx; ft[1] = globalposy; ft[2] = cosglobalang; ft[3] = singlobalang; }
            else
            {
                //relative alignment
                fx = (double)(wall[wall[sec->wallptr].point2].x-wall[sec->wallptr].x);
                fy = (double)(wall[wall[sec->wallptr].point2].y-wall[sec->wallptr].y);
                r = 1.0/sqrt(fx*fx+fy*fy); fx *= r; fy *= r;
                ft[2] = cosglobalang*fx + singlobalang*fy;
                ft[3] = singlobalang*fx - cosglobalang*fy;
                ft[0] = ((double)(globalposx-wall[sec->wallptr].x))*fx + ((double)(globalposy-wall[sec->wallptr].y))*fy;
                ft[1] = ((double)(globalposy-wall[sec->wallptr].y))*fx - ((double)(globalposx-wall[sec->wallptr].x))*fy;
                if (!(globalorientation&4)) globalorientation ^= 32; else globalorientation ^= 16;
            }
            gdx = 0;
            gdy = gxyaspect; if (!(globalorientation&2)) gdy /= (double)(sec->floorz-globalposz);
            gdo = -ghoriz*gdy;
            if (globalorientation&8) { ft[0] /= 8; ft[1] /= -8; ft[2] /= 2097152; ft[3] /= 2097152; }
            else { ft[0] /= 16; ft[1] /= -16; ft[2] /= 4194304; ft[3] /= 4194304; }
            gux = (double)ft[3]*((double)viewingrange)/-65536.0;
            gvx = (double)ft[2]*((double)viewingrange)/-65536.0;
            guy = (double)ft[0]*gdy; gvy = (double)ft[1]*gdy;
            guo = (double)ft[0]*gdo; gvo = (double)ft[1]*gdo;
            guo += (double)(ft[2]-gux)*ghalfx;
            gvo -= (double)(ft[3]+gvx)*ghalfx;

            //Texture flipping
            if (globalorientation&4)
            {
                r = gux; gux = gvx; gvx = r;
                r = guy; guy = gvy; gvy = r;
                r = guo; guo = gvo; gvo = r;
            }
            if (globalorientation&16) { gux = -gux; guy = -guy; guo = -guo; }
            if (globalorientation&32) { gvx = -gvx; gvy = -gvy; gvo = -gvo; }

            //Texture panning
            fx = (float)sec->floorxpanning*((float)(1<<(picsiz[globalpicnum]&15)))/256.0;
            fy = (float)sec->floorypanning*((float)(1<<(picsiz[globalpicnum]>>4)))/256.0;
            if ((globalorientation&(2+64)) == (2+64)) //Hack for panning for slopes w/ relative alignment
            {
                r = (float)sec->floorheinum / 4096.0; r = 1.0/sqrt(r*r+1);
                if (!(globalorientation&4)) fy *= r; else fx *= r;
            }
            guy += gdy*fx; guo += gdo*fx;
            gvy += gdy*fy; gvo += gdo*fy;

            if (globalorientation&2) //slopes
            {
                px[0] = x0; py[0] = ryp0 + ghoriz;
                px[1] = x1; py[1] = ryp1 + ghoriz;

                //Pick some point guaranteed to be not collinear to the 1st two points
                ox = nx0 + (ny1-ny0);
                oy = ny0 + (nx0-nx1);
                ox2 = (double)(oy-globalposy)*gcosang  - (double)(ox-globalposx)*gsinang;
                oy2 = (double)(ox-globalposx)*gcosang2 + (double)(oy-globalposy)*gsinang2;
                oy2 = 1.0/oy2;
                px[2] = ghalfx*ox2*oy2 + ghalfx; oy2 *= gyxscale;
                py[2] = oy2 + ghoriz;

                for (i=0; i<3; i++)
                {
                    dd[i] = px[i]*gdx + py[i]*gdy + gdo;
                    uu[i] = px[i]*gux + py[i]*guy + guo;
                    vv[i] = px[i]*gvx + py[i]*gvy + gvo;
                }

                py[0] = fy0;
                py[1] = fy1;
                py[2] = (getflorzofslope(sectnum,(int32_t)ox,(int32_t)oy)-globalposz)*oy2 + ghoriz;

                ox = py[1]-py[2]; oy = py[2]-py[0]; oz = py[0]-py[1];
                r = 1.0 / (ox*px[0] + oy*px[1] + oz*px[2]);
                gdx = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
                gux = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
                gvx = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
                ox = px[2]-px[1]; oy = px[0]-px[2]; oz = px[1]-px[0];
                gdy = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
                guy = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
                gvy = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
                gdo = dd[0] - px[0]*gdx - py[0]*gdy;
                guo = uu[0] - px[0]*gux - py[0]*guy;
                gvo = vv[0] - px[0]*gvx - py[0]*gvy;

                if (globalorientation&64) //Hack for relative alignment on slopes
                {
                    r = (float)sec->floorheinum / 4096.0;
                    r = sqrt(r*r+1);
                    if (!(globalorientation&4)) { gvx *= r; gvy *= r; gvo *= r; }
                    else { gux *= r; guy *= r; guo *= r; }
                }
            }
            domostpolymethod = (globalorientation>>7)&3;
            if (globalposz >= getflorzofslope(sectnum,globalposx,globalposy)) domostpolymethod = -1; //Back-face culling
#ifdef USE_OPENGL
            if (!nofog)
            {
                fogcalc(sec->floorshade,sec->visibility,sec->floorpal);
                bglFogf(GL_FOG_DENSITY,fogresult);
                bglFogfv(GL_FOG_COLOR,fogcol);
            }
#endif
            pow2xsplit = 0; domost(x0,fy0,x1,fy1); //flor
            domostpolymethod = 0;
        }
        else if ((nextsectnum < 0) || (!(sector[nextsectnum].floorstat&1)))
        {
            //Parallaxing sky... hacked for Ken's mountain texture; paper-sky only :/
#ifdef USE_OPENGL
            if (rendmode >= 3)
            {
                /*                if (!nofog) {
                                    bglDisable(GL_FOG);
                                    //r = ((float)globalpisibility)*((float)((uint8_t)(sec->visibility<240?sec->visibility+16:sec->visibility-239)))*FOGSCALE;
                                    //r *= ((double)xdimscale*(double)viewingrange*gdo) / (65536.0*65536.0);
                                    //bglFogf(GL_FOG_DENSITY,r);
                                } */

                if (!nofog)
                {
                    fogcalc(sec->floorshade,sec->visibility,sec->floorpal);
                    bglFogf(GL_FOG_DENSITY,fogresult * 0.005);
                    bglFogfv(GL_FOG_COLOR,fogcol);
                }

                //Use clamping for tiled sky textures
                for (i=(1<<pskybits)-1; i>0; i--)
                    if (pskyoff[i] != pskyoff[i-1])
                        { skyclamphack = r_parallaxskyclamping; break; }
            }
#endif
            if (bpp == 8 || !usehightile || !hicfindsubst(globalpicnum,globalpal,1))
            {
                dd[0] = (float)xdimen*.0000001; //Adjust sky depth based on screen size!
                t = (double)((1<<(picsiz[globalpicnum]&15))<<pskybits);
                vv[1] = dd[0]*((double)xdimscale*(double)viewingrange)/(65536.0*65536.0);
                vv[0] = dd[0]*((double)((tilesizy[globalpicnum]>>1)+parallaxyoffs)) - vv[1]*ghoriz;
                i = (1<<(picsiz[globalpicnum]>>4)); if (i != tilesizy[globalpicnum]) i += i;
                vv[0] += dd[0]*((double)((r_parallaxskypanning)?sec->floorypanning:0))*((double)i)/256.0;


                //Hack to draw black rectangle below sky when looking down...
                gdx = 0; gdy = gxyaspect / 262144.0; gdo = -ghoriz*gdy;
                gux = 0; guy = 0; guo = 0;
                gvx = 0; gvy = (double)(tilesizy[globalpicnum]-1)*gdy; gvo = (double)(tilesizy[globalpicnum-1])*gdo;
                oy = (((double)tilesizy[globalpicnum])*dd[0]-vv[0])/vv[1];
                if ((oy > fy0) && (oy > fy1)) domost(x0,oy,x1,oy);
                else if ((oy > fy0) != (oy > fy1))
                {
                    //  fy0                      fy1
                    //     \                    /
                    //oy----------      oy----------
                    //        \              /
                    //         fy1        fy0
                    ox = (oy-fy0)*(x1-x0)/(fy1-fy0) + x0;
                    if (oy > fy0) { domost(x0,oy,ox,oy); domost(ox,oy,x1,fy1); }
                    else { domost(x0,fy0,ox,oy); domost(ox,oy,x1,oy); }
                }
                else domost(x0,fy0,x1,fy1);


                gdx = 0; gdy = 0; gdo = dd[0];
                gux = gdo*(t*((double)xdimscale)*((double)yxaspect)*((double)viewingrange))/(16384.0*65536.0*65536.0*5.0*1024.0);
                guy = 0; //guo calculated later
                gvx = 0; gvy = vv[1]; gvo = vv[0];

                i = globalpicnum; r = (fy1-fy0)/(x1-x0); //slope of line
                oy = ((double)viewingrange)/(ghalfx*256.0); oz = 1/oy;

                y = ((((int32_t)((x0-ghalfx)*oy))+globalang)>>(11-pskybits));
                fx = x0;
                do
                {
                    globalpicnum = pskyoff[y&((1<<pskybits)-1)]+i;
                    guo = gdo*(t*((double)(globalang-(y<<(11-pskybits))))/2048.0 + (double)((r_parallaxskypanning)?sec->floorxpanning:0)) - gux*ghalfx;
                    y++;
                    ox = fx; fx = ((double)((y<<(11-pskybits))-globalang))*oz+ghalfx;
                    if (fx > x1) { fx = x1; i = -1; }

                    pow2xsplit = 0; domost(ox,(ox-x0)*r+fy0,fx,(fx-x0)*r+fy0); //flor
                }
                while (i >= 0);

            }
            else  //NOTE: code copied from ceiling code... lots of duplicated stuff :/
            {
                //Skybox code for parallax ceiling!
                double _xp0, _yp0, _xp1, _yp1, _oxp0, _oyp0, _t0, _t1, _nx0, _ny0, _nx1, _ny1;
                double _ryp0, _ryp1, _x0, _x1, _cy0, _fy0, _cy1, _fy1, _ox0, _ox1;
                double nfy0, nfy1;
                int32_t skywalx[4] = {-512,512,512,-512}, skywaly[4] = {-512,-512,512,512};

                pow2xsplit = 0;
                skyclamphack = 1;

                for (i=0; i<4; i++)
                {
                    x = skywalx[i&3]; y = skywaly[i&3];
                    _xp0 = (double)y*gcosang  - (double)x*gsinang;
                    _yp0 = (double)x*gcosang2 + (double)y*gsinang2;
                    x = skywalx[(i+1)&3]; y = skywaly[(i+1)&3];
                    _xp1 = (double)y*gcosang  - (double)x*gsinang;
                    _yp1 = (double)x*gcosang2 + (double)y*gsinang2;

                    _oxp0 = _xp0; _oyp0 = _yp0;

                    //Clip to close parallel-screen plane
                    if (_yp0 < SCISDIST)
                    {
                        if (_yp1 < SCISDIST) continue;
                        _t0 = (SCISDIST-_yp0)/(_yp1-_yp0); _xp0 = (_xp1-_xp0)*_t0+_xp0; _yp0 = SCISDIST;
                        _nx0 = (skywalx[(i+1)&3]-skywalx[i&3])*_t0+skywalx[i&3];
                        _ny0 = (skywaly[(i+1)&3]-skywaly[i&3])*_t0+skywaly[i&3];
                    }
                    else { _t0 = 0.f; _nx0 = skywalx[i&3]; _ny0 = skywaly[i&3]; }
                    if (_yp1 < SCISDIST)
                    {
                        _t1 = (SCISDIST-_oyp0)/(_yp1-_oyp0); _xp1 = (_xp1-_oxp0)*_t1+_oxp0; _yp1 = SCISDIST;
                        _nx1 = (skywalx[(i+1)&3]-skywalx[i&3])*_t1+skywalx[i&3];
                        _ny1 = (skywaly[(i+1)&3]-skywaly[i&3])*_t1+skywaly[i&3];
                    }
                    else { _t1 = 1.f; _nx1 = skywalx[(i+1)&3]; _ny1 = skywaly[(i+1)&3]; }

                    _ryp0 = 1.f/_yp0; _ryp1 = 1.f/_yp1;

                    //Generate screen coordinates for front side of wall
                    _x0 = ghalfx*_xp0*_ryp0 + ghalfx;
                    _x1 = ghalfx*_xp1*_ryp1 + ghalfx;
                    if (_x1 <= _x0) continue;
                    if ((_x0 >= x1) || (x0 >= _x1)) continue;

                    _ryp0 *= gyxscale; _ryp1 *= gyxscale;

                    _cy0 = -8192.f*_ryp0 + ghoriz;
                    _fy0 =  8192.f*_ryp0 + ghoriz;
                    _cy1 = -8192.f*_ryp1 + ghoriz;
                    _fy1 =  8192.f*_ryp1 + ghoriz;

                    _ox0 = _x0; _ox1 = _x1;

                    //Make sure: x0<=_x0<_x1<=_x1
                    nfy0 = fy0; nfy1 = fy1;
                    if (_x0 < x0)
                    {
                        t = (x0-_x0)/(_x1-_x0);
                        _cy0 += (_cy1-_cy0)*t;
                        _fy0 += (_fy1-_fy0)*t;
                        _x0 = x0;
                    }
                    else if (_x0 > x0) nfy0 += (_x0-x0)*(fy1-fy0)/(x1-x0);
                    if (_x1 > x1)
                    {
                        t = (x1-_x1)/(_x1-_x0);
                        _cy1 += (_cy1-_cy0)*t;
                        _fy1 += (_fy1-_fy0)*t;
                        _x1 = x1;
                    }
                    else if (_x1 < x1) nfy1 += (_x1-x1)*(fy1-fy0)/(x1-x0);

                    //   (skybox floor)
                    //(_x0,_fy0)-(_x1,_fy1)
                    //   (skybox wall)
                    //(_x0,_cy0)-(_x1,_cy1)
                    //   (skybox ceiling)
                    //(_x0,nfy0)-(_x1,nfy1)

                    //ceiling of skybox
                    ft[0] = 512/16; ft[1] = 512/-16;
                    ft[2] = ((float)cosglobalang)*(1.f/2147483648.f);
                    ft[3] = ((float)singlobalang)*(1.f/2147483648.f);
                    gdx = 0;
                    gdy = gxyaspect*(1.f/4194304.f);
                    gdo = -ghoriz*gdy;
                    gux = (double)ft[3]*((double)viewingrange)/-65536.0;
                    gvx = (double)ft[2]*((double)viewingrange)/-65536.0;
                    guy = (double)ft[0]*gdy; gvy = (double)ft[1]*gdy;
                    guo = (double)ft[0]*gdo; gvo = (double)ft[1]*gdo;
                    guo += (double)(ft[2]-gux)*ghalfx;
                    gvo -= (double)(ft[3]+gvx)*ghalfx;
                    gvx = -gvx; gvy = -gvy; gvo = -gvo; //y-flip skybox floor
#ifdef USE_OPENGL
                    drawingskybox = 6; //ceiling/5th texture/index 4 of skybox
#endif
                    if ((_fy0 > nfy0) && (_fy1 > nfy1)) domost(_x0,_fy0,_x1,_fy1);
                    else if ((_fy0 > nfy0) != (_fy1 > nfy1))
                    {
                        //(ox,oy) is intersection of: (_x0,_cy0)-(_x1,_cy1)
                        //                            (_x0,nfy0)-(_x1,nfy1)
                        //ox = _x0 + (_x1-_x0)*t
                        //oy = _cy0 + (_cy1-_cy0)*t
                        //oy = nfy0 + (nfy1-nfy0)*t
                        t = (_fy0-nfy0)/(nfy1-nfy0-_fy1+_fy0);
                        ox = _x0 + (_x1-_x0)*t;
                        oy = _fy0 + (_fy1-_fy0)*t;
                        if (nfy0 > _fy0) { domost(_x0,nfy0,ox,oy); domost(ox,oy,_x1,_fy1); }
                        else { domost(_x0,_fy0,ox,oy); domost(ox,oy,_x1,nfy1); }
                    }
                    else domost(_x0,nfy0,_x1,nfy1);

                    //wall of skybox
#ifdef USE_OPENGL
                    drawingskybox = i+1; //i+1th texture/index i of skybox
#endif
                    gdx = (_ryp0-_ryp1)*gxyaspect*(1.f/512.f) / (_ox0-_ox1);
                    gdy = 0;
                    gdo = _ryp0*gxyaspect*(1.f/512.f) - gdx*_ox0;
                    gux = (_t0*_ryp0 - _t1*_ryp1)*gxyaspect*(64.f/512.f) / (_ox0-_ox1);
                    guo = _t0*_ryp0*gxyaspect*(64.f/512.f) - gux*_ox0;
                    guy = 0;
                    _t0 = -8192.0*_ryp0 + ghoriz;
                    _t1 = -8192.0*_ryp1 + ghoriz;
                    t = ((gdx*_ox0 + gdo)*8.f) / ((_ox1-_ox0) * _ryp0 * 2048.f);
                    gvx = (_t0-_t1)*t;
                    gvy = (_ox1-_ox0)*t;
                    gvo = -gvx*_ox0 - gvy*_t0;
                    if ((_cy0 > nfy0) && (_cy1 > nfy1)) domost(_x0,_cy0,_x1,_cy1);
                    else if ((_cy0 > nfy0) != (_cy1 > nfy1))
                    {
                        //(ox,oy) is intersection of: (_x0,_fy0)-(_x1,_fy1)
                        //                            (_x0,nfy0)-(_x1,nfy1)
                        //ox = _x0 + (_x1-_x0)*t
                        //oy = _fy0 + (_fy1-_fy0)*t
                        //oy = nfy0 + (nfy1-nfy0)*t
                        t = (_cy0-nfy0)/(nfy1-nfy0-_cy1+_cy0);
                        ox = _x0 + (_x1-_x0)*t;
                        oy = _cy0 + (_cy1-_cy0)*t;
                        if (nfy0 > _cy0) { domost(_x0,nfy0,ox,oy); domost(ox,oy,_x1,_cy1); }
                        else { domost(_x0,_cy0,ox,oy); domost(ox,oy,_x1,nfy1); }
                    }
                    else domost(_x0,nfy0,_x1,nfy1);
                }

                //Floor of skybox
#ifdef USE_OPENGL
                drawingskybox = 5; //floor/6th texture/index 5 of skybox
#endif
                ft[0] = 512/16; ft[1] = -512/-16;
                ft[2] = ((float)cosglobalang)*(1.f/2147483648.f);
                ft[3] = ((float)singlobalang)*(1.f/2147483648.f);
                gdx = 0;
                gdy = gxyaspect*(-1.f/4194304.f);
                gdo = -ghoriz*gdy;
                gux = (double)ft[3]*((double)viewingrange)/-65536.0;
                gvx = (double)ft[2]*((double)viewingrange)/-65536.0;
                guy = (double)ft[0]*gdy; gvy = (double)ft[1]*gdy;
                guo = (double)ft[0]*gdo; gvo = (double)ft[1]*gdo;
                guo += (double)(ft[2]-gux)*ghalfx;
                gvo -= (double)(ft[3]+gvx)*ghalfx;
                domost(x0,fy0,x1,fy1);

                skyclamphack = 0;
#ifdef USE_OPENGL
                drawingskybox = 0;
#endif
            }
#ifdef USE_OPENGL
            if (rendmode >= 3)
            {
                skyclamphack = 0;
                if (!nofog)
                {
                    bglEnable(GL_FOG);
                    //bglFogf(GL_FOG_DENSITY,gvisibility*((float)((uint8_t)(sec->visibility<240?sec->visibility+16:sec->visibility-239))));
                }
            }
#endif
        }

        globalpicnum = sec->ceilingpicnum; globalshade = sec->ceilingshade; globalpal = (int32_t)((uint8_t)sec->ceilingpal);
        globalorientation = sec->ceilingstat;
        if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,sectnum);
        if (!(globalorientation&1))
        {
            if (!(globalorientation&64))
                { ft[0] = globalposx; ft[1] = globalposy; ft[2] = cosglobalang; ft[3] = singlobalang; }
            else
            {
                //relative alignment
                fx = (double)(wall[wall[sec->wallptr].point2].x-wall[sec->wallptr].x);
                fy = (double)(wall[wall[sec->wallptr].point2].y-wall[sec->wallptr].y);
                r = 1.0/sqrt(fx*fx+fy*fy); fx *= r; fy *= r;
                ft[2] = cosglobalang*fx + singlobalang*fy;
                ft[3] = singlobalang*fx - cosglobalang*fy;
                ft[0] = ((double)(globalposx-wall[sec->wallptr].x))*fx + ((double)(globalposy-wall[sec->wallptr].y))*fy;
                ft[1] = ((double)(globalposy-wall[sec->wallptr].y))*fx - ((double)(globalposx-wall[sec->wallptr].x))*fy;
                if (!(globalorientation&4)) globalorientation ^= 32; else globalorientation ^= 16;
            }
            gdx = 0;
            gdy = gxyaspect;
            if (!(globalorientation&2)) gdy /= (double)(sec->ceilingz-globalposz);
            gdo = -ghoriz*gdy;
            if (globalorientation&8) { ft[0] /= 8; ft[1] /= -8; ft[2] /= 2097152; ft[3] /= 2097152; }
            else { ft[0] /= 16; ft[1] /= -16; ft[2] /= 4194304; ft[3] /= 4194304; }
            gux = (double)ft[3]*((double)viewingrange)/-65536.0;
            gvx = (double)ft[2]*((double)viewingrange)/-65536.0;
            guy = (double)ft[0]*gdy; gvy = (double)ft[1]*gdy;
            guo = (double)ft[0]*gdo; gvo = (double)ft[1]*gdo;
            guo += (double)(ft[2]-gux)*ghalfx;
            gvo -= (double)(ft[3]+gvx)*ghalfx;

            //Texture flipping
            if (globalorientation&4)
            {
                r = gux; gux = gvx; gvx = r;
                r = guy; guy = gvy; gvy = r;
                r = guo; guo = gvo; gvo = r;
            }
            if (globalorientation&16) { gux = -gux; guy = -guy; guo = -guo; }
            if (globalorientation&32) { gvx = -gvx; gvy = -gvy; gvo = -gvo; }

            //Texture panning
            fx = (float)sec->ceilingxpanning*((float)(1<<(picsiz[globalpicnum]&15)))/256.0;
            fy = (float)sec->ceilingypanning*((float)(1<<(picsiz[globalpicnum]>>4)))/256.0;
            if ((globalorientation&(2+64)) == (2+64)) //Hack for panning for slopes w/ relative alignment
            {
                r = (float)sec->ceilingheinum / 4096.0; r = 1.0/sqrt(r*r+1);
                if (!(globalorientation&4)) fy *= r; else fx *= r;
            }
            guy += gdy*fx; guo += gdo*fx;
            gvy += gdy*fy; gvo += gdo*fy;

            if (globalorientation&2) //slopes
            {
                px[0] = x0; py[0] = ryp0 + ghoriz;
                px[1] = x1; py[1] = ryp1 + ghoriz;

                //Pick some point guaranteed to be not collinear to the 1st two points
                ox = nx0 + (ny1-ny0);
                oy = ny0 + (nx0-nx1);
                ox2 = (double)(oy-globalposy)*gcosang  - (double)(ox-globalposx)*gsinang ;
                oy2 = (double)(ox-globalposx)*gcosang2 + (double)(oy-globalposy)*gsinang2;
                oy2 = 1.0/oy2;
                px[2] = ghalfx*ox2*oy2 + ghalfx; oy2 *= gyxscale;
                py[2] = oy2 + ghoriz;

                for (i=0; i<3; i++)
                {
                    dd[i] = px[i]*gdx + py[i]*gdy + gdo;
                    uu[i] = px[i]*gux + py[i]*guy + guo;
                    vv[i] = px[i]*gvx + py[i]*gvy + gvo;
                }

                py[0] = cy0;
                py[1] = cy1;
                py[2] = (getceilzofslope(sectnum,(int32_t)ox,(int32_t)oy)-globalposz)*oy2 + ghoriz;

                ox = py[1]-py[2]; oy = py[2]-py[0]; oz = py[0]-py[1];
                r = 1.0 / (ox*px[0] + oy*px[1] + oz*px[2]);
                gdx = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
                gux = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
                gvx = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
                ox = px[2]-px[1]; oy = px[0]-px[2]; oz = px[1]-px[0];
                gdy = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
                guy = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
                gvy = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
                gdo = dd[0] - px[0]*gdx - py[0]*gdy;
                guo = uu[0] - px[0]*gux - py[0]*guy;
                gvo = vv[0] - px[0]*gvx - py[0]*gvy;

                if (globalorientation&64) //Hack for relative alignment on slopes
                {
                    r = (float)sec->ceilingheinum / 4096.0;
                    r = sqrt(r*r+1);
                    if (!(globalorientation&4)) { gvx *= r; gvy *= r; gvo *= r; }
                    else { gux *= r; guy *= r; guo *= r; }
                }
            }
            domostpolymethod = (globalorientation>>7)&3;
            if (globalposz <= getceilzofslope(sectnum,globalposx,globalposy)) domostpolymethod = -1; //Back-face culling
#ifdef USE_OPENGL
            if (!nofog)
            {
                fogcalc(sec->ceilingshade,sec->visibility,sec->ceilingpal);
                bglFogf(GL_FOG_DENSITY,fogresult);
                bglFogfv(GL_FOG_COLOR,fogcol);
            }
#endif
            pow2xsplit = 0; domost(x1,cy1,x0,cy0); //ceil
            domostpolymethod = 0;
        }
        else if ((nextsectnum < 0) || (!(sector[nextsectnum].ceilingstat&1)))
        {
#ifdef USE_OPENGL
            if (rendmode >= 3)
            {
                /*                if (!nofog) {
                                    bglDisable(GL_FOG);
                                    //r = ((float)globalpisibility)*((float)((uint8_t)(sec->visibility<240?sec->visibility+16:sec->visibility-239)))*FOGSCALE;
                                    //r *= ((double)xdimscale*(double)viewingrange*gdo) / (65536.0*65536.0);
                                    //bglFogf(GL_FOG_DENSITY,r);
                                }
                */
                if (!nofog)
                {
                    fogcalc(sec->ceilingshade,sec->visibility,sec->ceilingpal);
                    bglFogf(GL_FOG_DENSITY,fogresult * 0.005);
                    bglFogfv(GL_FOG_COLOR,fogcol);
                }
                //Use clamping for tiled sky textures
                for (i=(1<<pskybits)-1; i>0; i--)
                    if (pskyoff[i] != pskyoff[i-1])
                        { skyclamphack = r_parallaxskyclamping; break; }
            }
#endif
            //Parallaxing sky...
            if (bpp == 8 || !usehightile || !hicfindsubst(globalpicnum,globalpal,1))
            {
                //Render for parallaxtype == 0 / paper-sky
                dd[0] = (float)xdimen*.0000001; //Adjust sky depth based on screen size!
                t = (double)((1<<(picsiz[globalpicnum]&15))<<pskybits);
                vv[1] = dd[0]*((double)xdimscale*(double)viewingrange)/(65536.0*65536.0);
                vv[0] = dd[0]*((double)((tilesizy[globalpicnum]>>1)+parallaxyoffs)) - vv[1]*ghoriz;
                i = (1<<(picsiz[globalpicnum]>>4)); if (i != tilesizy[globalpicnum]) i += i;
                vv[0] += dd[0]*((double)((r_parallaxskypanning)?sec->ceilingypanning:0))*((double)i)/256.0;

                //Hack to draw black rectangle below sky when looking down...
                gdx = 0; gdy = gxyaspect / -262144.0; gdo = -ghoriz*gdy;
                gux = 0; guy = 0; guo = 0;
                gvx = 0; gvy = 0; gvo = 0;
                oy = -vv[0]/vv[1];
                if ((oy < cy0) && (oy < cy1)) domost(x1,oy,x0,oy);
                else if ((oy < cy0) != (oy < cy1))
                {
                    /*         cy1        cy0
                                                                         //        /             \
                                                                         //oy----------      oy---------
                                                                         //    /                    \
                                                                         //  cy0                     cy1
                                                                         */
                    ox = (oy-cy0)*(x1-x0)/(cy1-cy0) + x0;
                    if (oy < cy0) { domost(ox,oy,x0,oy); domost(x1,cy1,ox,oy); }
                    else { domost(ox,oy,x0,cy0); domost(x1,oy,ox,oy); }
                }
                else domost(x1,cy1,x0,cy0);

                gdx = 0; gdy = 0; gdo = dd[0];
                gux = gdo*(t*((double)xdimscale)*((double)yxaspect)*((double)viewingrange))/(16384.0*65536.0*65536.0*5.0*1024.0);
                guy = 0; //guo calculated later
                gvx = 0; gvy = vv[1]; gvo = vv[0];

                i = globalpicnum; r = (cy1-cy0)/(x1-x0); //slope of line
                oy = ((double)viewingrange)/(ghalfx*256.0); oz = 1/oy;

                y = ((((int32_t)((x0-ghalfx)*oy))+globalang)>>(11-pskybits));
                fx = x0;
                do
                {
                    globalpicnum = pskyoff[y&((1<<pskybits)-1)]+i;
                    guo = gdo*(t*((double)(globalang-(y<<(11-pskybits))))/2048.0 + (double)((r_parallaxskypanning)?sec->ceilingxpanning:0)) - gux*ghalfx;
                    y++;
                    ox = fx; fx = ((double)((y<<(11-pskybits))-globalang))*oz+ghalfx;
                    if (fx > x1) { fx = x1; i = -1; }
                    pow2xsplit = 0; domost(fx,(fx-x0)*r+cy0,ox,(ox-x0)*r+cy0); //ceil
                }
                while (i >= 0);
            }
            else
            {
                //Skybox code for parallax ceiling!
                double _xp0, _yp0, _xp1, _yp1, _oxp0, _oyp0, _t0, _t1, _nx0, _ny0, _nx1, _ny1;
                double _ryp0, _ryp1, _x0, _x1, _cy0, _fy0, _cy1, _fy1, _ox0, _ox1;
                double ncy0, ncy1;
                int32_t skywalx[4] = {-512,512,512,-512}, skywaly[4] = {-512,-512,512,512};

                pow2xsplit = 0;
                skyclamphack = 1;

                for (i=0; i<4; i++)
                {
                    x = skywalx[i&3]; y = skywaly[i&3];
                    _xp0 = (double)y*gcosang  - (double)x*gsinang;
                    _yp0 = (double)x*gcosang2 + (double)y*gsinang2;
                    x = skywalx[(i+1)&3]; y = skywaly[(i+1)&3];
                    _xp1 = (double)y*gcosang  - (double)x*gsinang;
                    _yp1 = (double)x*gcosang2 + (double)y*gsinang2;

                    _oxp0 = _xp0; _oyp0 = _yp0;

                    //Clip to close parallel-screen plane
                    if (_yp0 < SCISDIST)
                    {
                        if (_yp1 < SCISDIST) continue;
                        _t0 = (SCISDIST-_yp0)/(_yp1-_yp0); _xp0 = (_xp1-_xp0)*_t0+_xp0; _yp0 = SCISDIST;
                        _nx0 = (skywalx[(i+1)&3]-skywalx[i&3])*_t0+skywalx[i&3];
                        _ny0 = (skywaly[(i+1)&3]-skywaly[i&3])*_t0+skywaly[i&3];
                    }
                    else { _t0 = 0.f; _nx0 = skywalx[i&3]; _ny0 = skywaly[i&3]; }
                    if (_yp1 < SCISDIST)
                    {
                        _t1 = (SCISDIST-_oyp0)/(_yp1-_oyp0); _xp1 = (_xp1-_oxp0)*_t1+_oxp0; _yp1 = SCISDIST;
                        _nx1 = (skywalx[(i+1)&3]-skywalx[i&3])*_t1+skywalx[i&3];
                        _ny1 = (skywaly[(i+1)&3]-skywaly[i&3])*_t1+skywaly[i&3];
                    }
                    else { _t1 = 1.f; _nx1 = skywalx[(i+1)&3]; _ny1 = skywaly[(i+1)&3]; }

                    _ryp0 = 1.f/_yp0; _ryp1 = 1.f/_yp1;

                    //Generate screen coordinates for front side of wall
                    _x0 = ghalfx*_xp0*_ryp0 + ghalfx;
                    _x1 = ghalfx*_xp1*_ryp1 + ghalfx;
                    if (_x1 <= _x0) continue;
                    if ((_x0 >= x1) || (x0 >= _x1)) continue;

                    _ryp0 *= gyxscale; _ryp1 *= gyxscale;

                    _cy0 = -8192.f*_ryp0 + ghoriz;
                    _fy0 =  8192.f*_ryp0 + ghoriz;
                    _cy1 = -8192.f*_ryp1 + ghoriz;
                    _fy1 =  8192.f*_ryp1 + ghoriz;

                    _ox0 = _x0; _ox1 = _x1;

                    //Make sure: x0<=_x0<_x1<=_x1
                    ncy0 = cy0; ncy1 = cy1;
                    if (_x0 < x0)
                    {
                        t = (x0-_x0)/(_x1-_x0);
                        _cy0 += (_cy1-_cy0)*t;
                        _fy0 += (_fy1-_fy0)*t;
                        _x0 = x0;
                    }
                    else if (_x0 > x0) ncy0 += (_x0-x0)*(cy1-cy0)/(x1-x0);
                    if (_x1 > x1)
                    {
                        t = (x1-_x1)/(_x1-_x0);
                        _cy1 += (_cy1-_cy0)*t;
                        _fy1 += (_fy1-_fy0)*t;
                        _x1 = x1;
                    }
                    else if (_x1 < x1) ncy1 += (_x1-x1)*(cy1-cy0)/(x1-x0);

                    //   (skybox ceiling)
                    //(_x0,_cy0)-(_x1,_cy1)
                    //   (skybox wall)
                    //(_x0,_fy0)-(_x1,_fy1)
                    //   (skybox floor)
                    //(_x0,ncy0)-(_x1,ncy1)

                    //ceiling of skybox
#ifdef USE_OPENGL
                    drawingskybox = 5; //ceiling/5th texture/index 4 of skybox
#endif
                    ft[0] = 512/16; ft[1] = -512/-16;
                    ft[2] = ((float)cosglobalang)*(1.f/2147483648.f);
                    ft[3] = ((float)singlobalang)*(1.f/2147483648.f);
                    gdx = 0;
                    gdy = gxyaspect*-(1.f/4194304.f);
                    gdo = -ghoriz*gdy;
                    gux = (double)ft[3]*((double)viewingrange)/-65536.0;
                    gvx = (double)ft[2]*((double)viewingrange)/-65536.0;
                    guy = (double)ft[0]*gdy; gvy = (double)ft[1]*gdy;
                    guo = (double)ft[0]*gdo; gvo = (double)ft[1]*gdo;
                    guo += (double)(ft[2]-gux)*ghalfx;
                    gvo -= (double)(ft[3]+gvx)*ghalfx;
                    if ((_cy0 < ncy0) && (_cy1 < ncy1)) domost(_x1,_cy1,_x0,_cy0);
                    else if ((_cy0 < ncy0) != (_cy1 < ncy1))
                    {
                        //(ox,oy) is intersection of: (_x0,_cy0)-(_x1,_cy1)
                        //                            (_x0,ncy0)-(_x1,ncy1)
                        //ox = _x0 + (_x1-_x0)*t
                        //oy = _cy0 + (_cy1-_cy0)*t
                        //oy = ncy0 + (ncy1-ncy0)*t
                        t = (_cy0-ncy0)/(ncy1-ncy0-_cy1+_cy0);
                        ox = _x0 + (_x1-_x0)*t;
                        oy = _cy0 + (_cy1-_cy0)*t;
                        if (ncy0 < _cy0) { domost(ox,oy,_x0,ncy0); domost(_x1,_cy1,ox,oy); }
                        else { domost(ox,oy,_x0,_cy0); domost(_x1,ncy1,ox,oy); }
                    }
                    else domost(_x1,ncy1,_x0,ncy0);

                    //wall of skybox
#ifdef USE_OPENGL
                    drawingskybox = i+1; //i+1th texture/index i of skybox
#endif
                    gdx = (_ryp0-_ryp1)*gxyaspect*(1.f/512.f) / (_ox0-_ox1);
                    gdy = 0;
                    gdo = _ryp0*gxyaspect*(1.f/512.f) - gdx*_ox0;
                    gux = (_t0*_ryp0 - _t1*_ryp1)*gxyaspect*(64.f/512.f) / (_ox0-_ox1);
                    guo = _t0*_ryp0*gxyaspect*(64.f/512.f) - gux*_ox0;
                    guy = 0;
                    _t0 = -8192.0*_ryp0 + ghoriz;
                    _t1 = -8192.0*_ryp1 + ghoriz;
                    t = ((gdx*_ox0 + gdo)*8.f) / ((_ox1-_ox0) * _ryp0 * 2048.f);
                    gvx = (_t0-_t1)*t;
                    gvy = (_ox1-_ox0)*t;
                    gvo = -gvx*_ox0 - gvy*_t0;
                    if ((_fy0 < ncy0) && (_fy1 < ncy1)) domost(_x1,_fy1,_x0,_fy0);
                    else if ((_fy0 < ncy0) != (_fy1 < ncy1))
                    {
                        //(ox,oy) is intersection of: (_x0,_fy0)-(_x1,_fy1)
                        //                            (_x0,ncy0)-(_x1,ncy1)
                        //ox = _x0 + (_x1-_x0)*t
                        //oy = _fy0 + (_fy1-_fy0)*t
                        //oy = ncy0 + (ncy1-ncy0)*t
                        t = (_fy0-ncy0)/(ncy1-ncy0-_fy1+_fy0);
                        ox = _x0 + (_x1-_x0)*t;
                        oy = _fy0 + (_fy1-_fy0)*t;
                        if (ncy0 < _fy0) { domost(ox,oy,_x0,ncy0); domost(_x1,_fy1,ox,oy); }
                        else { domost(ox,oy,_x0,_fy0); domost(_x1,ncy1,ox,oy); }
                    }
                    else domost(_x1,ncy1,_x0,ncy0);
                }

                //Floor of skybox
#ifdef USE_OPENGL
                drawingskybox = 6; //floor/6th texture/index 5 of skybox
#endif
                ft[0] = 512/16; ft[1] = 512/-16;
                ft[2] = ((float)cosglobalang)*(1.f/2147483648.f);
                ft[3] = ((float)singlobalang)*(1.f/2147483648.f);
                gdx = 0;
                gdy = gxyaspect*(1.f/4194304.f);
                gdo = -ghoriz*gdy;
                gux = (double)ft[3]*((double)viewingrange)/-65536.0;
                gvx = (double)ft[2]*((double)viewingrange)/-65536.0;
                guy = (double)ft[0]*gdy; gvy = (double)ft[1]*gdy;
                guo = (double)ft[0]*gdo; gvo = (double)ft[1]*gdo;
                guo += (double)(ft[2]-gux)*ghalfx;
                gvo -= (double)(ft[3]+gvx)*ghalfx;
                gvx = -gvx; gvy = -gvy; gvo = -gvo; //y-flip skybox floor
                domost(x1,cy1,x0,cy0);

                skyclamphack = 0;
#ifdef USE_OPENGL
                drawingskybox = 0;
#endif
            }
#ifdef USE_OPENGL
            if (rendmode >= 3)
            {
                skyclamphack = 0;
                if (!nofog)
                {
                    bglEnable(GL_FOG);
                    //bglFogf(GL_FOG_DENSITY,gvisibility*((float)((uint8_t)(sec->visibility<240?sec->visibility+16:sec->visibility-239))));
                }
            }
#endif
        }

        //(x0,cy0) == (u=             0,v=0,d=)
        //(x1,cy0) == (u=wal->xrepeat*8,v=0)
        //(x0,fy0) == (u=             0,v=v)
        //             u = (gux*sx + guy*sy + guo) / (gdx*sx + gdy*sy + gdo)
        //             v = (gvx*sx + gvy*sy + gvo) / (gdx*sx + gdy*sy + gdo)
        //             0 = (gux*x0 + guy*cy0 + guo) / (gdx*x0 + gdy*cy0 + gdo)
        //wal->xrepeat*8 = (gux*x1 + guy*cy0 + guo) / (gdx*x1 + gdy*cy0 + gdo)
        //             0 = (gvx*x0 + gvy*cy0 + gvo) / (gdx*x0 + gdy*cy0 + gdo)
        //             v = (gvx*x0 + gvy*fy0 + gvo) / (gdx*x0 + gdy*fy0 + gdo)
        //sx = x0, u = t0*wal->xrepeat*8, d = yp0;
        //sx = x1, u = t1*wal->xrepeat*8, d = yp1;
        //d = gdx*sx + gdo
        //u = (gux*sx + guo) / (gdx*sx + gdo)
        //yp0 = gdx*x0 + gdo
        //yp1 = gdx*x1 + gdo
        //t0*wal->xrepeat*8 = (gux*x0 + guo) / (gdx*x0 + gdo)
        //t1*wal->xrepeat*8 = (gux*x1 + guo) / (gdx*x1 + gdo)
        //gdx*x0 + gdo = yp0
        //gdx*x1 + gdo = yp1
        gdx = (ryp0-ryp1)*gxyaspect / (x0-x1);
        gdy = 0;
        gdo = ryp0*gxyaspect - gdx*x0;

        //gux*x0 + guo = t0*wal->xrepeat*8*yp0
        //gux*x1 + guo = t1*wal->xrepeat*8*yp1
        gux = (t0*ryp0 - t1*ryp1)*gxyaspect*(float)wal->xrepeat*8.f / (x0-x1);
        guo = t0*ryp0*gxyaspect*(float)wal->xrepeat*8.f - gux*x0;
        guo += (float)wal->xpanning*gdo;
        gux += (float)wal->xpanning*gdx;
        guy = 0;
        //Derivation for u:
        //   (gvx*x0 + gvy*cy0 + gvo) / (gdx*x0 + gdy*cy0 + gdo) = 0
        //   (gvx*x1 + gvy*cy1 + gvo) / (gdx*x1 + gdy*cy1 + gdo) = 0
        //   (gvx*x0 + gvy*fy0 + gvo) / (gdx*x0 + gdy*fy0 + gdo) = v
        //   (gvx*x1 + gvy*fy1 + gvo) / (gdx*x1 + gdy*fy1 + gdo) = v
        //   (gvx*x0 + gvy*cy0 + gvo*1) = 0
        //   (gvx*x1 + gvy*cy1 + gvo*1) = 0
        //   (gvx*x0 + gvy*fy0 + gvo*1) = t
        ogux = gux; oguy = guy; oguo = guo;

        if (nextsectnum >= 0)
        {
            getzsofslope(nextsectnum,(int32_t)nx0,(int32_t)ny0,&cz,&fz);
            ocy0 = ((float)(cz-globalposz))*ryp0 + ghoriz;
            ofy0 = ((float)(fz-globalposz))*ryp0 + ghoriz;
            getzsofslope(nextsectnum,(int32_t)nx1,(int32_t)ny1,&cz,&fz);
            ocy1 = ((float)(cz-globalposz))*ryp1 + ghoriz;
            ofy1 = ((float)(fz-globalposz))*ryp1 + ghoriz;

            if ((wal->cstat&48) == 16) maskwall[maskwallcnt++] = z;

            if (((cy0 < ocy0) || (cy1 < ocy1)) && (!((sec->ceilingstat&sector[nextsectnum].ceilingstat)&1)))
            {
                globalpicnum = wal->picnum; globalshade = wal->shade; globalpal = (int32_t)((uint8_t)wal->pal);
                if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,wallnum+16384);

                if (!(wal->cstat&4)) i = sector[nextsectnum].ceilingz; else i = sec->ceilingz;
                t0 = ((float)(i-globalposz))*ryp0 + ghoriz;
                t1 = ((float)(i-globalposz))*ryp1 + ghoriz;
                t = ((gdx*x0 + gdo) * (float)wal->yrepeat) / ((x1-x0) * ryp0 * 2048.f);
                i = (1<<(picsiz[globalpicnum]>>4)); if (i < tilesizy[globalpicnum]) i <<= 1;

                ypan = wal->ypanning;
                ftol((i-tilesizy[globalpicnum])*(255.f/i),&yoffs);
                if (wal->cstat&4)
                {
                    if (ypan>256-yoffs)
                    {
                        ypan-=yoffs;
                    }
                }
                else
                {
                    // not hacked yet
                }

                fy = (float)ypan * ((float)i) / 256.0;
                gvx = (t0-t1)*t;
                gvy = (x1-x0)*t;
                gvo = -gvx*x0 - gvy*t0 + fy*gdo; gvx += fy*gdx; gvy += fy*gdy;

                if (wal->cstat&8) //xflip
                {
                    t = (float)(wal->xrepeat*8 + wal->xpanning*2);
                    gux = gdx*t - gux;
                    guy = gdy*t - guy;
                    guo = gdo*t - guo;
                }
                if (wal->cstat&256) { gvx = -gvx; gvy = -gvy; gvo = -gvo; } //yflip
#ifdef USE_OPENGL
                if (!nofog)
                {
                    fogcalc(wal->shade,sec->visibility,sec->floorpal);
                    bglFogf(GL_FOG_DENSITY,fogresult);
                    bglFogfv(GL_FOG_COLOR,fogcol);
                }
#endif
                pow2xsplit = 1; domost(x1,ocy1,x0,ocy0);
                if (wal->cstat&8) { gux = ogux; guy = oguy; guo = oguo; }
            }
            if (((ofy0 < fy0) || (ofy1 < fy1)) && (!((sec->floorstat&sector[nextsectnum].floorstat)&1)))
            {
                if (!(wal->cstat&2)) nwal = wal;
                else
                {
                    nwal = &wall[wal->nextwall];
                    guo += (float)(nwal->xpanning-wal->xpanning)*gdo;
                    gux += (float)(nwal->xpanning-wal->xpanning)*gdx;
                    guy += (float)(nwal->xpanning-wal->xpanning)*gdy;
                }
                globalpicnum = nwal->picnum; globalshade = nwal->shade; globalpal = (int32_t)((uint8_t)nwal->pal);
                if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,wallnum+16384);

                if (!(nwal->cstat&4)) i = sector[nextsectnum].floorz; else i = sec->ceilingz;
                t0 = ((float)(i-globalposz))*ryp0 + ghoriz;
                t1 = ((float)(i-globalposz))*ryp1 + ghoriz;
                t = ((gdx*x0 + gdo) * (float)wal->yrepeat) / ((x1-x0) * ryp0 * 2048.f);
                i = (1<<(picsiz[globalpicnum]>>4)); if (i < tilesizy[globalpicnum]) i <<= 1;

                ypan = nwal->ypanning;
                ftol((i-tilesizy[globalpicnum])*(255.f/i),&yoffs);
                if (!(nwal->cstat&4))
                {
                    if (ypan>256-yoffs)
                    {
                        ypan-=yoffs;
                    }
                }
                else
                {
                    // Still need a hack, depending on the wall(height,ypanning,yrepeat,tilesizy)
                    // it should do "ypan-=yoffs" or "ypan+=yoffs" or [nothing].
                    // Example: the film projector in the E1L1.map needs "ypan-=yoffs"
                }

                fy = (float)ypan * ((float)i) / 256.0;
                gvx = (t0-t1)*t;
                gvy = (x1-x0)*t;
                gvo = -gvx*x0 - gvy*t0 + fy*gdo; gvx += fy*gdx; gvy += fy*gdy;

                if (wal->cstat&8) //xflip
                {
                    t = (float)(wal->xrepeat*8 + nwal->xpanning*2);
                    gux = gdx*t - gux;
                    guy = gdy*t - guy;
                    guo = gdo*t - guo;
                }
                if (nwal->cstat&256) { gvx = -gvx; gvy = -gvy; gvo = -gvo; } //yflip
#ifdef USE_OPENGL
                if (!nofog)
                {
                    fogcalc(nwal->shade,sec->visibility,sec->floorpal);
                    bglFogf(GL_FOG_DENSITY,fogresult);
                    bglFogfv(GL_FOG_COLOR,fogcol);
                }
#endif
                pow2xsplit = 1; domost(x0,ofy0,x1,ofy1);
                if (wal->cstat&(2+8)) { guo = oguo; gux = ogux; guy = oguy; }
            }
        }

        if ((nextsectnum < 0) || (wal->cstat&32))   //White/1-way wall
        {
            if (nextsectnum < 0) globalpicnum = wal->picnum; else globalpicnum = wal->overpicnum;
            globalshade = wal->shade; globalpal = (int32_t)((uint8_t)wal->pal);
            if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,wallnum+16384);

            if (nextsectnum >= 0) { if (!(wal->cstat&4)) i = nextsec->ceilingz; else i = sec->ceilingz; }
            else { if (!(wal->cstat&4)) i = sec->ceilingz;     else i = sec->floorz; }
            t0 = ((float)(i-globalposz))*ryp0 + ghoriz;
            t1 = ((float)(i-globalposz))*ryp1 + ghoriz;
            t = ((gdx*x0 + gdo) * (float)wal->yrepeat) / ((x1-x0) * ryp0 * 2048.f);
            i = (1<<(picsiz[globalpicnum]>>4)); if (i < tilesizy[globalpicnum]) i <<= 1;

            ypan = wal->ypanning;
            ftol((i-tilesizy[globalpicnum])*(255.f/i),&yoffs);
            if (!(wal->cstat&4))
            {
                if (ypan>256-yoffs)
                {
                    ypan-=yoffs;
                }
            }
            else
            {
                // not hacked yet
            }

            fy = (float)ypan * ((float)i) / 256.0;
            gvx = (t0-t1)*t;
            gvy = (x1-x0)*t;
            gvo = -gvx*x0 - gvy*t0 + fy*gdo; gvx += fy*gdx; gvy += fy*gdy;

            if (wal->cstat&8) //xflip
            {
                t = (float)(wal->xrepeat*8 + wal->xpanning*2);
                gux = gdx*t - gux;
                guy = gdy*t - guy;
                guo = gdo*t - guo;
            }
            if (wal->cstat&256) { gvx = -gvx; gvy = -gvy; gvo = -gvo; } //yflip
#ifdef USE_OPENGL
            if (!nofog)
            {
                fogcalc(wal->shade,sec->visibility,sec->floorpal);
                bglFogf(GL_FOG_DENSITY,fogresult);
                bglFogfv(GL_FOG_COLOR,fogcol);
            }
#endif
            pow2xsplit = 1; domost(x0,-10000,x1,-10000);
        }

        if (nextsectnum >= 0)
            if ((!(gotsector[nextsectnum>>3]&pow2char[nextsectnum&7])) && (testvisiblemost(x0,x1)))
                polymost_scansector(nextsectnum);
    }
}

static int32_t polymost_bunchfront(int32_t b1, int32_t b2)
{
    double x1b1, x1b2, x2b1, x2b2;
    int32_t b1f, b2f, i;

    b1f = bunchfirst[b1]; x1b1 = dxb1[b1f]; x2b2 = dxb2[bunchlast[b2]]; if (x1b1 >= x2b2) return(-1);
    b2f = bunchfirst[b2]; x1b2 = dxb1[b2f]; x2b1 = dxb2[bunchlast[b1]]; if (x1b2 >= x2b1) return(-1);

    if (x1b1 >= x1b2)
    {
        for (i=b2f; dxb2[i]<=x1b1; i=p2[i]);
        return(wallfront(b1f,i));
    }
    for (i=b1f; dxb2[i]<=x1b2; i=p2[i]);
    return(wallfront(i,b2f));
}

static void polymost_scansector(int32_t sectnum)
{
    double d, xp1, yp1, xp2, yp2;
    walltype *wal, *wal2;
    spritetype *spr;
    int32_t z, zz, startwall, endwall, numscansbefore, scanfirst, bunchfrst, nextsectnum;
    int32_t xs, ys, x1, y1, x2, y2;

    if (sectnum < 0) return;
    if (automapping) show2dsector[sectnum>>3] |= pow2char[sectnum&7];

    sectorborder[0] = sectnum, sectorbordercnt = 1;
    do
    {
        sectnum = sectorborder[--sectorbordercnt];

        for (z=headspritesect[sectnum]; z>=0; z=nextspritesect[z])
        {
            spr = &sprite[z];
            if ((((spr->cstat&0x8000) == 0) || (showinvisibility)) &&
                    (spr->xrepeat > 0) && (spr->yrepeat > 0) &&
                    (spritesortcnt < MAXSPRITESONSCREEN))
            {
                xs = spr->x-globalposx; ys = spr->y-globalposy;
                if ((spr->cstat&48) || (xs*gcosang+ys*gsinang > 0))
                {
                    copybufbyte(spr,&tsprite[spritesortcnt],sizeof(spritetype));
                    spriteext[z].tspr = (spritetype *)&tsprite[spritesortcnt];
                    tsprite[spritesortcnt++].owner = z;
                }
            }
        }

        gotsector[sectnum>>3] |= pow2char[sectnum&7];

        bunchfrst = numbunches;
        numscansbefore = numscans;

        startwall = sector[sectnum].wallptr; endwall = sector[sectnum].wallnum+startwall;
        scanfirst = numscans;
        xp2 = 0; yp2 = 0;
        for (z=startwall,wal=&wall[z]; z<endwall; z++,wal++)
        {
            wal2 = &wall[wal->point2];
            x1 = wal->x-globalposx; y1 = wal->y-globalposy;
            x2 = wal2->x-globalposx; y2 = wal2->y-globalposy;

            nextsectnum = wal->nextsector; //Scan close sectors
            if ((nextsectnum >= 0) && (!(wal->cstat&32)) && (!(gotsector[nextsectnum>>3]&pow2char[nextsectnum&7])))
            {
                d = (double)x1*(double)y2 - (double)x2*(double)y1; xp1 = (double)(x2-x1); yp1 = (double)(y2-y1);
                if (d*d <= (xp1*xp1 + yp1*yp1)*(SCISDIST*SCISDIST*260.0))
                    sectorborder[sectorbordercnt++] = nextsectnum;
            }

            if ((z == startwall) || (wall[z-1].point2 != z))
            {
                xp1 = ((double)y1*(double)cosglobalang             - (double)x1*(double)singlobalang)/64.0;
                yp1 = ((double)x1*(double)cosviewingrangeglobalang + (double)y1*(double)sinviewingrangeglobalang)/64.0;
            }
            else { xp1 = xp2; yp1 = yp2; }
            xp2 = ((double)y2*(double)cosglobalang             - (double)x2*(double)singlobalang)/64.0;
            yp2 = ((double)x2*(double)cosviewingrangeglobalang + (double)y2*(double)sinviewingrangeglobalang)/64.0;
            if ((yp1 >= SCISDIST) || (yp2 >= SCISDIST))
                if ((double)xp1*(double)yp2 < (double)xp2*(double)yp1) //if wall is facing you...
                {
                    if (yp1 >= SCISDIST)
                        dxb1[numscans] = (double)xp1*ghalfx/(double)yp1 + ghalfx;
                    else dxb1[numscans] = -1e32;

                    if (yp2 >= SCISDIST)
                        dxb2[numscans] = (double)xp2*ghalfx/(double)yp2 + ghalfx;
                    else dxb2[numscans] = 1e32;

                    if (dxb1[numscans] < dxb2[numscans])
                        { thesector[numscans] = sectnum; thewall[numscans] = z; p2[numscans] = numscans+1; numscans++; }
                }

            if ((wall[z].point2 < z) && (scanfirst < numscans))
                { p2[numscans-1] = scanfirst; scanfirst = numscans; }
        }

        for (z=numscansbefore; z<numscans; z++)
            if ((wall[thewall[z]].point2 != thewall[p2[z]]) || (dxb2[z] > dxb1[p2[z]]))
                { bunchfirst[numbunches++] = p2[z]; p2[z] = -1; }

        for (z=bunchfrst; z<numbunches; z++)
        {
            for (zz=bunchfirst[z]; p2[zz]>=0; zz=p2[zz]);
            bunchlast[z] = zz;
        }
    }
    while (sectorbordercnt > 0);
}

void polymost_drawrooms()
{
    int32_t i, j, n, n2, closest;
    double ox, oy, oz, ox2, oy2, oz2, r, px[6], py[6], pz[6], px2[6], py2[6], pz2[6], sx[6], sy[6];

    if (!rendmode) return;

    begindrawing();
    frameoffset = frameplace + windowy1*bytesperline + windowx1;

#ifdef USE_OPENGL
    if (rendmode >= 3)
    {
        resizeglcheck();

        if (editstatus)
            bglClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        bglDisable(GL_BLEND);
        bglEnable(GL_TEXTURE_2D);
        //bglTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE); //default anyway
        bglEnable(GL_DEPTH_TEST);
        bglDepthFunc(GL_ALWAYS); //NEVER,LESS,(,L)EQUAL,GREATER,(NOT,G)EQUAL,ALWAYS

        //bglPolygonOffset(1,1); //Supposed to make sprites pasted on walls or floors not disappear
        bglDepthRange(0.00001,1.0); //<- this is more widely supported than glPolygonOffset

        //Enable this for OpenGL red-blue glasses mode :)
        if (glredbluemode)
        {
            static int32_t grbfcnt = 0; grbfcnt++;
            if (redblueclearcnt < numpages) { redblueclearcnt++; bglColorMask(1,1,1,1); bglClear(GL_COLOR_BUFFER_BIT); }
            if (grbfcnt&1)
            {
                bglViewport(windowx1-16,yres-(windowy2+1),windowx2-(windowx1-16)+1,windowy2-windowy1+1);
                bglColorMask(1,0,0,1);
                globalposx += singlobalang/1024;
                globalposy -= cosglobalang/1024;
            }
            else
            {
                bglViewport(windowx1,yres-(windowy2+1),windowx2+16-windowx1+1,windowy2-windowy1+1);
                bglColorMask(0,1,1,1);
                globalposx -= singlobalang/1024;
                globalposy += cosglobalang/1024;
            }
        }
    }
#endif

    //Polymost supports true look up/down :) Here, we convert horizon to angle.
    //gchang&gshang are cos&sin of this angle (respectively)
    gyxscale = ((double)xdimenscale)/131072.0;
    gxyaspect = ((double)xyaspect*(double)viewingrange)*(5.0/(65536.0*262144.0));
    gviewxrange = ((double)viewingrange)*((double)xdimen)/(32768.0*1024.0);
    gcosang = ((double)cosglobalang)/262144.0;
    gsinang = ((double)singlobalang)/262144.0;
    gcosang2 = gcosang*((double)viewingrange)/65536.0;
    gsinang2 = gsinang*((double)viewingrange)/65536.0;
    ghalfx = (double)halfxdimen; grhalfxdown10 = 1.0/(((double)ghalfx)*1024);
    ghoriz = (double)globalhoriz;

    gvisibility = ((float)globalvisibility)*FOGSCALE;

    //global cos/sin height angle
    r = (double)((ydimen>>1)-ghoriz);
    gshang = r/sqrt(r*r+ghalfx*ghalfx);
    gchang = sqrt(1.0-gshang*gshang);
    ghoriz = (double)(ydimen>>1);

    //global cos/sin tilt angle
    gctang = cos(gtang);
    gstang = sin(gtang);
    if (fabs(gstang) < .001) //This hack avoids nasty precision bugs in domost()
        { gstang = 0; if (gctang > 0) gctang = 1.0; else gctang = -1.0; }

    if (inpreparemirror)
        gstang = -gstang;

    //Generate viewport trapezoid (for handling screen up/down)
    px[0] = px[3] = 0-1; px[1] = px[2] = windowx2+1-windowx1+2;
    py[0] = py[1] = 0-1; py[2] = py[3] = windowy2+1-windowy1+2; n = 4;
    for (i=0; i<n; i++)
    {
        ox = px[i]-ghalfx; oy = py[i]-ghoriz; oz = ghalfx;

        //Tilt rotation (backwards)
        ox2 = ox*gctang + oy*gstang;
        oy2 = oy*gctang - ox*gstang;
        oz2 = oz;

        //Up/down rotation (backwards)
        px[i] = ox2;
        py[i] = oy2*gchang + oz2*gshang;
        pz[i] = oz2*gchang - oy2*gshang;
    }

    //Clip to SCISDIST plane
    n2 = 0;
    for (i=0; i<n; i++)
    {
        j = i+1; if (j >= n) j = 0;
        if (pz[i] >= SCISDIST) { px2[n2] = px[i]; py2[n2] = py[i]; pz2[n2] = pz[i]; n2++; }
        if ((pz[i] >= SCISDIST) != (pz[j] >= SCISDIST))
        {
            r = (SCISDIST-pz[i])/(pz[j]-pz[i]);
            px2[n2] = (px[j]-px[i])*r + px[i];
            py2[n2] = (py[j]-py[i])*r + py[i];
            pz2[n2] = SCISDIST; n2++;
        }
    }
    if (n2 < 3) { enddrawing(); return; }
    for (i=0; i<n2; i++)
    {
        r = ghalfx / pz2[i];
        sx[i] = px2[i]*r + ghalfx;
        sy[i] = py2[i]*r + ghoriz;
    }
    initmosts(sx,sy,n2);

    if (searchit == 2)
    {
        int32_t vx, vy, vz;
        int32_t cz, fz;
        hitdata_t hitinfo;
        vec3_t vect;
        double ratio = 1.05;

#ifdef USE_OPENGL
        if (glwidescreen == 1)
            ratio = 1.2f;
        else if (glprojectionhacks == 1)
        {
            double mul = (gshang*gshang);
            ratio += mul*mul*mul*mul;
        }
        else if (glprojectionhacks == 2)
        {
            if (gshang > 0.7f)
                ratio += 4.f*(gshang-0.7f);
            if (gshang < -0.7f)
                ratio += 4.f*(-gshang-0.7f);
        }
#endif

        ox2 = (searchx-ghalfx)/ratio; oy2 = (searchy-ghoriz)/ ratio; oz2 = ghalfx;

        //Tilt rotation
        ox = ox2*gctang + oy2*gstang;
        oy = oy2*gctang - ox2*gstang;
        oz = oz2;

        //Up/down rotation
        ox2 = oz*gchang - oy*gshang;
        oy2 = ox;
        oz2 = oy*gchang + oz*gshang;

        //Standard Left/right rotation
        vx = (int32_t)(ox2*((float)cosglobalang) - oy2*((float)singlobalang));
        vy = (int32_t)(ox2*((float)singlobalang) + oy2*((float)cosglobalang));
        vz = (int32_t)(oz2*16384.0);

        vect.x = globalposx;
        vect.y = globalposy;
        vect.z = globalposz;

        hitallsprites = 1;
        hitscan((const vec3_t *)&vect,globalcursectnum, //Start position
            vx>>10,vy>>10,vz>>6,&hitinfo,0xffff0030);

        if (hitinfo.hitsect != -1) // if hitsect is -1, hitscan overflowed somewhere
        {
            getzsofslope(hitinfo.hitsect,hitinfo.pos.x,hitinfo.pos.y,&cz,&fz);
            hitallsprites = 0;

            searchsector = hitinfo.hitsect;
            if (hitinfo.pos.z<cz) searchstat = 1; else if (hitinfo.pos.z>fz) searchstat = 2; else if (hitinfo.hitwall >= 0)
            {
                searchbottomwall = searchwall = hitinfo.hitwall; searchstat = 0;
                if (wall[hitinfo.hitwall].nextwall >= 0)
                {
                    int32_t cz, fz;
                    getzsofslope(wall[hitinfo.hitwall].nextsector,hitinfo.pos.x,hitinfo.pos.y,&cz,&fz);
                    if (hitinfo.pos.z > fz)
                    {
                        if (wall[hitinfo.hitwall].cstat&2) //'2' bottoms of walls
                            searchbottomwall = wall[hitinfo.hitwall].nextwall;
                    }
                    else if ((hitinfo.pos.z > cz) && (wall[hitinfo.hitwall].cstat&(16+32))) //masking or 1-way
                        searchstat = 4;
                }
            }
            else if (hitinfo.hitsprite >= 0) { searchwall = hitinfo.hitsprite; searchstat = 3; }
            else
            {
                int32_t cz, fz;
                getzsofslope(hitinfo.hitsect,hitinfo.pos.x,hitinfo.pos.y,&cz,&fz);
                if ((hitinfo.pos.z<<1) < cz+fz) searchstat = 1; else searchstat = 2;
                //if (vz < 0) searchstat = 1; else searchstat = 2; //Won't work for slopes :/
            }

            if (preview_mouseaim && spritesortcnt < MAXSPRITESONSCREEN)
            {
                spritetype *tsp = &tsprite[spritesortcnt];
                double dadist, x,y,z;
                Bmemcpy(tsp, &hitinfo.pos, sizeof(vec3_t));
                x = tsp->x-globalposx; y=tsp->y-globalposy; z=(tsp->z-globalposz)/16.0;
                dadist = sqrt(x*x + y*y + z*z);
                tsp->sectnum = hitinfo.hitsect;
                tsp->picnum = 2523;  // CROSSHAIR
                tsp->cstat = 128;
                tsp->owner = MAXSPRITES-1;
                tsp->xrepeat = tsp->yrepeat = min(max(1, (int32_t)(dadist*48.0/3200.0)), 255);
                sprite[tsp->owner].xoffset = sprite[tsp->owner].yoffset = 0;
                tspriteptr[spritesortcnt++] = tsp;
            }

            if ((searchstat==1 || searchstat==2) && searchsector>=0)
            {
                int32_t scrv[2] = {(vx>>12), (vy>>12)};
                int32_t scrv_r[2] = {scrv[1], -scrv[0]};
                walltype *wal = &wall[sector[searchsector].wallptr];
                int32_t wdistsq, bestwdistsq=0x7fffffff;
                int16_t k, bestk=-1;

                for (k=0; k<sector[searchsector].wallnum; k++)
                {
                    int32_t w1[2] = {wal[k].x, wal[k].y};
                    int32_t w2[2] = {wall[wal[k].point2].x, wall[wal[k].point2].y};
                    int32_t w21[2] = {w1[0]-w2[0], w1[1]-w2[1]};
                    int32_t pw1[2] = {w1[0]-hitinfo.pos.x, w1[1]-hitinfo.pos.y};
                    int32_t pw2[2] = {w2[0]-hitinfo.pos.x, w2[1]-hitinfo.pos.y};
                    float w1d = (float)(scrv_r[0]*pw1[0] + scrv_r[1]*pw1[1]);
                    float w2d = (float)(scrv_r[0]*pw2[0] + scrv_r[1]*pw2[1]);
                    int32_t ptonline[2], scrp[2];

                    w2d = -w2d;
                    if ((w1d==0 && w2d==0) || (w1d<0 || w2d<0))
                        continue;
                    ptonline[0] = (int32_t)(w2[0]+(w2d/(w1d+w2d))*w21[0]);
                    ptonline[1] = (int32_t)(w2[1]+(w2d/(w1d+w2d))*w21[1]);
                    scrp[0] = ptonline[0]-vect.x;
                    scrp[1] = ptonline[1]-vect.y;
                    if (scrv[0]*scrp[0] + scrv[1]*scrp[1] <= 0)
                        continue;
                    wdistsq = scrp[0]*scrp[0] + scrp[1]*scrp[1];
                    if (wdistsq < bestwdistsq)
                    {
                        bestk = k;
                        bestwdistsq = wdistsq;
                    }
                }

                if (bestk >= 0)
                    searchwall = sector[searchsector].wallptr + bestk;
            }
        }
        searchit = 0;
    }

    numscans = numbunches = 0;

    if (globalcursectnum >= MAXSECTORS)
        globalcursectnum -= MAXSECTORS;
    else
    {
        i = globalcursectnum;
        updatesector(globalposx,globalposy,&globalcursectnum);
        if (globalcursectnum < 0) globalcursectnum = i;
    }

    polymost_scansector(globalcursectnum);

    if (inpreparemirror)
    {
        grhalfxdown10x = -grhalfxdown10;
        inpreparemirror = 0;
        polymost_drawalls(0);
        numbunches--;
        bunchfirst[0] = bunchfirst[numbunches];
        bunchlast[0] = bunchlast[numbunches];
    }
    else
        grhalfxdown10x = grhalfxdown10;

    while (numbunches > 0)
    {
        memset(ptempbuf,0,numbunches+3); ptempbuf[0] = 1;

        closest = 0;              //Almost works, but not quite :(
        for (i=1; i<numbunches; i++)
        {
            j = polymost_bunchfront(i,closest); if (j < 0) continue;
            ptempbuf[i] = 1;
            if (!j) { ptempbuf[closest] = 1; closest = i; }
        }
        for (i=0; i<numbunches; i++) //Double-check
        {
            if (ptempbuf[i]) continue;
            j = polymost_bunchfront(i,closest); if (j < 0) continue;
            ptempbuf[i] = 1;
            if (!j) { ptempbuf[closest] = 1; closest = i; i = 0; }
        }

        polymost_drawalls(closest);

        numbunches--;
        bunchfirst[closest] = bunchfirst[numbunches];
        bunchlast[closest] = bunchlast[numbunches];
    }
#ifdef USE_OPENGL
    if (rendmode >= 3)
    {
        bglDepthFunc(GL_LEQUAL); //NEVER,LESS,(,L)EQUAL,GREATER,(NOT,G)EQUAL,ALWAYS

        //bglPolygonOffset(0,0);
        bglDepthRange(0.0,0.99999); //<- this is more widely supported than glPolygonOffset
    }
#endif

    enddrawing();
}

void polymost_drawmaskwall(int32_t damaskwallcnt)
{
    double dpx[8], dpy[8], dpx2[8], dpy2[8];
    float fy, x0, x1, sx0, sy0, sx1, sy1, xp0, yp0, xp1, yp1, oxp0, oyp0, ryp0, ryp1;
    float r, t, t0, t1, csy[4], fsy[4];
    int32_t i, j, n, n2, z, sectnum, z1, z2, cz[4], fz[4], method;
    int32_t m0, m1;
    sectortype *sec, *nsec;
    walltype *wal, *wal2;

    z = maskwall[damaskwallcnt];
    wal = &wall[thewall[z]]; wal2 = &wall[wal->point2];
    sectnum = thesector[z]; sec = &sector[sectnum];
    nsec = &sector[wal->nextsector];
    z1 = max(nsec->ceilingz,sec->ceilingz);
    z2 = min(nsec->floorz,sec->floorz);

    globalpicnum = wal->overpicnum; if ((uint32_t)globalpicnum >= MAXTILES) globalpicnum = 0;
    if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,(int16_t)thewall[z]+16384);
    globalshade = (int32_t)wal->shade;
    globalpal = (int32_t)((uint8_t)wal->pal);
    globalorientation = (int32_t)wal->cstat;

    sx0 = (float)(wal->x-globalposx); sx1 = (float)(wal2->x-globalposx);
    sy0 = (float)(wal->y-globalposy); sy1 = (float)(wal2->y-globalposy);
    yp0 = sx0*gcosang2 + sy0*gsinang2;
    yp1 = sx1*gcosang2 + sy1*gsinang2;
    if ((yp0 < SCISDIST) && (yp1 < SCISDIST)) return;
    xp0 = sy0*gcosang - sx0*gsinang;
    xp1 = sy1*gcosang - sx1*gsinang;

    //Clip to close parallel-screen plane
    oxp0 = xp0; oyp0 = yp0;
    if (yp0 < SCISDIST) { t0 = (SCISDIST-yp0)/(yp1-yp0); xp0 = (xp1-xp0)*t0+xp0; yp0 = SCISDIST; }
    else t0 = 0.f;
    if (yp1 < SCISDIST) { t1 = (SCISDIST-oyp0)/(yp1-oyp0); xp1 = (xp1-oxp0)*t1+oxp0; yp1 = SCISDIST; }
    else { t1 = 1.f; }

    m0 = (int32_t)((wal2->x-wal->x)*t0+wal->x);
    m1 = (int32_t)((wal2->y-wal->y)*t0+wal->y);
    getzsofslope(sectnum,m0,m1,&cz[0],&fz[0]);
    getzsofslope(wal->nextsector,m0,m1,&cz[1],&fz[1]);
    m0 = (int32_t)((wal2->x-wal->x)*t1+wal->x);
    m1 = (int32_t)((wal2->y-wal->y)*t1+wal->y);
    getzsofslope(sectnum,m0,m1,&cz[2],&fz[2]);
    getzsofslope(wal->nextsector,m0,m1,&cz[3],&fz[3]);

    ryp0 = 1.f/yp0; ryp1 = 1.f/yp1;

    //Generate screen coordinates for front side of wall
    x0 = ghalfx*xp0*ryp0 + ghalfx;
    x1 = ghalfx*xp1*ryp1 + ghalfx;
    if (x1 <= x0) return;

    ryp0 *= gyxscale; ryp1 *= gyxscale;

    gdx = (ryp0-ryp1)*gxyaspect / (x0-x1);
    gdy = 0;
    gdo = ryp0*gxyaspect - gdx*x0;

    //gux*x0 + guo = t0*wal->xrepeat*8*yp0
    //gux*x1 + guo = t1*wal->xrepeat*8*yp1
    gux = (t0*ryp0 - t1*ryp1)*gxyaspect*(float)wal->xrepeat*8.f / (x0-x1);
    guo = t0*ryp0*gxyaspect*(float)wal->xrepeat*8.f - gux*x0;
    guo += (float)wal->xpanning*gdo;
    gux += (float)wal->xpanning*gdx;
    guy = 0;

    if (!(wal->cstat&4)) i = z1; else i = z2;
    i -= globalposz;
    t0 = ((float)i)*ryp0 + ghoriz;
    t1 = ((float)i)*ryp1 + ghoriz;
    t = ((gdx*x0 + gdo) * (float)wal->yrepeat) / ((x1-x0) * ryp0 * 2048.f);
    i = (1<<(picsiz[globalpicnum]>>4)); if (i < tilesizy[globalpicnum]) i <<= 1;
    fy = (float)wal->ypanning * ((float)i) / 256.0;
    gvx = (t0-t1)*t;
    gvy = (x1-x0)*t;
    gvo = -gvx*x0 - gvy*t0 + fy*gdo; gvx += fy*gdx; gvy += fy*gdy;

    if (wal->cstat&8) //xflip
    {
        t = (float)(wal->xrepeat*8 + wal->xpanning*2);
        gux = gdx*t - gux;
        guy = gdy*t - guy;
        guo = gdo*t - guo;
    }
    if (wal->cstat&256) { gvx = -gvx; gvy = -gvy; gvo = -gvo; } //yflip

    method = 1; pow2xsplit = 1;
    if (wal->cstat&128) { if (!(wal->cstat&512)) method = 2; else method = 3; }

#ifdef USE_OPENGL
    if (!nofog)
    {
        if (rendmode >= 3)
        {
            fogcalc(wal->shade,sec->visibility,sec->floorpal);
            bglFogf(GL_FOG_DENSITY,fogresult);
            bglFogfv(GL_FOG_COLOR,fogcol);
        }
    }
#endif

    for (i=0; i<2; i++)
    {
        csy[i] = ((float)(cz[i]-globalposz))*ryp0 + ghoriz;
        fsy[i] = ((float)(fz[i]-globalposz))*ryp0 + ghoriz;
        csy[i+2] = ((float)(cz[i+2]-globalposz))*ryp1 + ghoriz;
        fsy[i+2] = ((float)(fz[i+2]-globalposz))*ryp1 + ghoriz;
    }

    //Clip 2 quadrilaterals
    //               /csy3
    //             /   |
    // csy0------/----csy2
    //   |     /xxxxxxx|
    //   |   /xxxxxxxxx|
    // csy1/xxxxxxxxxxx|
    //   |xxxxxxxxxxx/fsy3
    //   |xxxxxxxxx/   |
    //   |xxxxxxx/     |
    // fsy0----/------fsy2
    //   |   /
    // fsy1/

    dpx[0] = x0; dpy[0] = csy[1];
    dpx[1] = x1; dpy[1] = csy[3];
    dpx[2] = x1; dpy[2] = fsy[3];
    dpx[3] = x0; dpy[3] = fsy[1];
    n = 4;

    //Clip to (x0,csy[0])-(x1,csy[2])
    n2 = 0; t1 = -((dpx[0]-x0)*(csy[2]-csy[0]) - (dpy[0]-csy[0])*(x1-x0));
    for (i=0; i<n; i++)
    {
        j = i+1; if (j >= n) j = 0;

        t0 = t1; t1 = -((dpx[j]-x0)*(csy[2]-csy[0]) - (dpy[j]-csy[0])*(x1-x0));
        if (t0 >= 0) { dpx2[n2] = dpx[i]; dpy2[n2] = dpy[i]; n2++; }
        if ((t0 >= 0) != (t1 >= 0))
        {
            r = t0/(t0-t1);
            dpx2[n2] = (dpx[j]-dpx[i])*r + dpx[i];
            dpy2[n2] = (dpy[j]-dpy[i])*r + dpy[i];
            n2++;
        }
    }
    if (n2 < 3) return;

    //Clip to (x1,fsy[2])-(x0,fsy[0])
    n = 0; t1 = -((dpx2[0]-x1)*(fsy[0]-fsy[2]) - (dpy2[0]-fsy[2])*(x0-x1));
    for (i=0; i<n2; i++)
    {
        j = i+1; if (j >= n2) j = 0;

        t0 = t1; t1 = -((dpx2[j]-x1)*(fsy[0]-fsy[2]) - (dpy2[j]-fsy[2])*(x0-x1));
        if (t0 >= 0) { dpx[n] = dpx2[i]; dpy[n] = dpy2[i]; n++; }
        if ((t0 >= 0) != (t1 >= 0))
        {
            r = t0/(t0-t1);
            dpx[n] = (dpx2[j]-dpx2[i])*r + dpx2[i];
            dpy[n] = (dpy2[j]-dpy2[i])*r + dpy2[i];
            n++;
        }
    }
    if (n < 3) return;

    drawpoly(dpx,dpy,n,method);
}

int32_t lastcullcheck = 0;
char cullmodel[MAXSPRITES];
int32_t cullcheckcnt = 0;

int32_t __fastcall polymost_checkcoordinates(int32_t x, int32_t y, spritetype *tspr)
{
    int16_t datempsectnum = tspr->sectnum;
    int32_t oldx = x, i, j = (tilesizy[tspr->picnum]*tspr->yrepeat);

RECHECK:
    updatesectorz(tspr->x+x,tspr->y+y,tspr->z,&datempsectnum);

    if (datempsectnum == -1)
    {
        if (x == y || x != oldx)
            return 0;
        swaplong(&x,&y);
        updatesector(tspr->x+x,tspr->y+y,&datempsectnum);
    }

    i = 4;
    do
    {
        cullcheckcnt += 2;
        if (cansee(globalposx, globalposy, globalposz, globalcursectnum,
                   tspr->x+x, tspr->y+y, tspr->z-(j*i)-512, datempsectnum))
            return 1;
        if (cansee(globalposx, globalposy, globalposz, globalcursectnum,
                   tspr->x+x, tspr->y+y, tspr->z-(j*(i-1))-512, datempsectnum))
            return 1;
        i -= 2;
    }
    while (i);

    cullcheckcnt++;
    if (cansee(globalposx, globalposy, globalposz, globalcursectnum,
               tspr->x+x, tspr->y+y, tspr->z-512, datempsectnum))
        return 1;

    if (x != y && x == oldx)
    {
        swaplong(&x,&y);
        goto RECHECK;
    }

    return 0;
}

void polymost_drawsprite(int32_t snum)
{
    double px[6], py[6];
    float f, c, s, fx, fy, sx0, sy0, sx1, xp0, yp0, xp1, yp1, oxp0, oyp0, ryp0, ryp1, ft[4];
    float x0, y0, x1, y1, sc0, sf0, sc1, sf1, px2[6], py2[6], xv, yv, t0, t1;
    int32_t i, j, spritenum, xoff=0, yoff=0, method, npoints;
    spritetype *tspr;
    int32_t posx,posy;
    int32_t oldsizx, oldsizy;
    int32_t tsizx, tsizy;
    tspr = tspriteptr[snum];
    if (tspr->owner < 0 || tspr->picnum < 0 || tspr->picnum >= MAXTILES) return;

    globalpicnum      = tspr->picnum;
    globalshade       = tspr->shade;
    globalpal         = tspr->pal;
    globalorientation = tspr->cstat;
    spritenum         = tspr->owner;

    if ((globalorientation&48) != 48)  	// only non-voxel sprites should do this
    {
        int32_t flag;
        if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,spritenum+32768);
        flag = usehightile&&h_xsize[globalpicnum];
        xoff = (int32_t)tspr->xoffset;
        yoff = (int32_t)tspr->yoffset;
        xoff += (int8_t)((flag)?(h_xoffs[globalpicnum]):((picanm[globalpicnum]>>8)&255));
        yoff += (int8_t)((flag)?(h_yoffs[globalpicnum]):((picanm[globalpicnum]>>16)&255));
    }

    method = 1+4;
    if (tspr->cstat&2) { if (!(tspr->cstat&512)) method = 2+4; else method = 3+4; }

#ifdef USE_OPENGL

    if (!nofog && rendmode >= 3)
    {
        fogcalc(globalshade,sector[tspr->sectnum].visibility,sector[tspr->sectnum].floorpal);
        bglFogf(GL_FOG_DENSITY,fogresult);
        bglFogfv(GL_FOG_COLOR,fogcol);
    }

    while (rendmode >= 3 && !(spriteext[tspr->owner].flags&SPREXT_NOTMD))
    {
        if (usemodels && tile2model[Ptile2tile(tspr->picnum,tspr->pal)].modelid >= 0 && tile2model[Ptile2tile(tspr->picnum,tspr->pal)].framenum >= 0)
        {
//            md2model *modelptr = (md2model *)models[tile2model[Ptile2tile(tspr->picnum,tspr->pal)].modelid];
            if (tspr->owner < 0 || tspr->owner >= MAXSPRITES || tspr->statnum == TSPR_MIRROR)
            {
                if (mddraw(tspr)) return;
                break;	// else, render as flat sprite
            }
            if (r_modelocclusionchecking)
            {
                if (totalclock >= lastcullcheck + CULL_DELAY && cullcheckcnt < MAXCULLCHECKS && (/*modelptr->usesalpha ||*/ tspr->yrepeat*tilesizy[sprite[tspr->owner].picnum] > 1536 || tspr->xrepeat*tilesizx[sprite[tspr->owner].picnum] > 1536))
                {
                    do // this is so gay
                    {
                        uint32_t t = getticks()+4;

                        // don't bother with shadows because processing its owner will take care of it
                        if (tspr->statnum == TSPR_TEMP)
                            break;
                        cullmodel[tspr->owner] = 1;
                        cullcheckcnt++;

                        if (cansee(globalposx, globalposy, globalposz, globalcursectnum,
                                   tspr->x, tspr->y, tspr->z,tspr->sectnum))
                            { cullmodel[tspr->owner] = 0; break; }

                        if (polymost_checkcoordinates(-CULL_OFFSET, 0, tspr) || getticks() > t || cullcheckcnt >= MAXCULLCHECKS)
                            { cullmodel[tspr->owner] = 0; break; }
                        if (polymost_checkcoordinates(-CULL_OFFSET, -CULL_OFFSET, tspr) || getticks() > t || cullcheckcnt >= MAXCULLCHECKS)
                            { cullmodel[tspr->owner] = 0; break; }

                        if (polymost_checkcoordinates(CULL_OFFSET, 0, tspr) || getticks() > t || cullcheckcnt >= MAXCULLCHECKS)
                            { cullmodel[tspr->owner] = 0; break; }
                        if (polymost_checkcoordinates(CULL_OFFSET, CULL_OFFSET, tspr) || getticks() > t || cullcheckcnt >= MAXCULLCHECKS)
                            { cullmodel[tspr->owner] = 0; break; }

                        if (polymost_checkcoordinates(-CULL_OFFSET, CULL_OFFSET, tspr) || getticks() > t || cullcheckcnt >= MAXCULLCHECKS)
                            { cullmodel[tspr->owner] = 0; break; }

                        if (polymost_checkcoordinates(0, 0, tspr) || getticks() > t || cullcheckcnt >= MAXCULLCHECKS)
                            { cullmodel[tspr->owner] = 0; break; }

                        break;
                    }
                    while (1);
                }
            }
            else cullmodel[tspr->owner] = 0;
            if (cullmodel[tspr->owner]) break;
            if (mddraw(tspr)) return;
            break;	// else, render as flat sprite
        }
        if (usevoxels && (tspr->cstat&48)!=48 && tiletovox[tspr->picnum] >= 0 && voxmodels[ tiletovox[tspr->picnum] ])
        {
            if (voxdraw(voxmodels[ tiletovox[tspr->picnum] ], tspr)) return;
            break;	// else, render as flat sprite
        }
        if ((tspr->cstat&48)==48 && voxmodels[ tspr->picnum ])
        {
            voxdraw(voxmodels[ tspr->picnum ], tspr);
            return;
        }
        break;
    }
    if (((tspr->cstat&2) || (gltexmayhavealpha(tspr->picnum,tspr->pal))))
    {
        curpolygonoffset += 0.01f;
        bglEnable(GL_POLYGON_OFFSET_FILL);
        bglPolygonOffset(-curpolygonoffset, -curpolygonoffset);
    }
#endif
    posx=tspr->x;
    posy=tspr->y;
    if (spriteext[tspr->owner].flags&SPREXT_AWAY1)
    {
        posx+=(sintable[(tspr->ang+512)&2047]>>13);
        posy+=(sintable[(tspr->ang)&2047]>>13);
    }
    else if (spriteext[tspr->owner].flags&SPREXT_AWAY2)
    {
        posx-=(sintable[(tspr->ang+512)&2047]>>13);
        posy-=(sintable[(tspr->ang)&2047]>>13);
    }
    oldsizx=tsizx=tilesizx[globalpicnum];
    oldsizy=tsizy=tilesizy[globalpicnum];
    if (usehightile&&h_xsize[globalpicnum])
    {
        tsizx=h_xsize[globalpicnum];
        tsizy=h_ysize[globalpicnum];
    }

    switch ((globalorientation>>4)&3)
    {
    case 0: //Face sprite
        //Project 3D to 2D
        sx0 = (float)(tspr->x-globalposx);
        sy0 = (float)(tspr->y-globalposy);
        xp0 = sy0*gcosang  - sx0*gsinang;
        yp0 = sx0*gcosang2 + sy0*gsinang2;
        if (yp0 <= SCISDIST) return;
        ryp0 = 1/yp0;
        sx0 = ghalfx*xp0*ryp0 + ghalfx;
        sy0 = ((float)(tspr->z-globalposz))*gyxscale*ryp0 + ghoriz;

        f = ryp0*(float)xdimen/160.0;
        fx = ((float)tspr->xrepeat)*f;
        fy = ((float)tspr->yrepeat)*f*((float)yxaspect/65536.0);
        sx0 -= fx*(float)xoff; if (tsizx&1) sx0 += fx*.5;
        sy0 -= fy*(float)yoff;
        fx *= ((float)tsizx);
        fy *= ((float)tsizy);

        px[0] = px[3] = sx0-fx*.5; px[1] = px[2] = sx0+fx*.5;
        if (!(globalorientation&128)) { py[0] = py[1] = sy0-fy; py[2] = py[3] = sy0; }
        else { py[0] = py[1] = sy0-fy*.5; py[2] = py[3] = sy0+fy*.5; }

        gdx = gdy = guy = gvx = 0; gdo = ryp0*gviewxrange;
        if (!(globalorientation&4))
            { gux = (float)tsizx*gdo/(px[1]-px[0]+.002); guo = -gux*(px[0]-.001); }
        else { gux = (float)tsizx*gdo/(px[0]-px[1]-.002); guo = -gux*(px[1]+.001); }
        if (!(globalorientation&8))
            { gvy = (float)tsizy*gdo/(py[3]-py[0]+.002); gvo = -gvy*(py[0]-.001); }
        else { gvy = (float)tsizy*gdo/(py[0]-py[3]-.002); gvo = -gvy*(py[3]+.001); }

        // sprite panning
        guy -= gdy*((float)(spriteext[spritenum].xpanning)/255.f)*tsizx;
        guo -= gdo*((float)(spriteext[spritenum].xpanning)/255.f)*tsizx;
        gvy -= gdy*((float)(spriteext[spritenum].ypanning)/255.f)*tsizy;
        gvo -= gdo*((float)(spriteext[spritenum].ypanning)/255.f)*tsizy;

        //Clip sprites to ceilings/floors when no parallaxing and not sloped
        if (!(sector[tspr->sectnum].ceilingstat&3))
        {
            sy0 = ((float)(sector[tspr->sectnum].ceilingz-globalposz))*gyxscale*ryp0 + ghoriz;
            if (py[0] < sy0) py[0] = py[1] = sy0;
        }
        if (!(sector[tspr->sectnum].floorstat&3))
        {
            sy0 = ((float)(sector[tspr->sectnum].floorz-globalposz))*gyxscale*ryp0 + ghoriz;
            if (py[2] > sy0) py[2] = py[3] = sy0;
        }

#ifdef USE_OPENGL
        if (spriteext[spritenum].xpanning)
            srepeat = 1;
        if (spriteext[spritenum].ypanning)
            trepeat = 1;
#endif
        tilesizx[globalpicnum] = tsizx;
        tilesizy[globalpicnum] = tsizy;
        pow2xsplit = 0; drawpoly(px,py,4,method);

#ifdef USE_OPENGL
        if (spriteext[spritenum].xpanning)
            srepeat = 0;
        if (spriteext[spritenum].ypanning)
            trepeat = 0;
#endif

        break;
    case 1: //Wall sprite

        //Project 3D to 2D
        if (globalorientation&4) xoff = -xoff;
        if (globalorientation&8) yoff = -yoff;

        xv = (float)tspr->xrepeat * (float)sintable[(tspr->ang)&2047] / 65536.0;
        yv = (float)tspr->xrepeat * (float)sintable[(tspr->ang+1536)&2047] / 65536.0;
        f = (float)(tsizx>>1) + (float)xoff;
        x0 = (float)(posx-globalposx) - xv*f; x1 = xv*(float)tsizx + x0;
        y0 = (float)(posy-globalposy) - yv*f; y1 = yv*(float)tsizx + y0;

        yp0 = x0*gcosang2 + y0*gsinang2;
        yp1 = x1*gcosang2 + y1*gsinang2;
        if ((yp0 <= SCISDIST) && (yp1 <= SCISDIST)) return;
        xp0 = y0*gcosang - x0*gsinang;
        xp1 = y1*gcosang - x1*gsinang;

        //Clip to close parallel-screen plane
        oxp0 = xp0; oyp0 = yp0;
        if (yp0 < SCISDIST) { t0 = (SCISDIST-yp0)/(yp1-yp0); xp0 = (xp1-xp0)*t0+xp0; yp0 = SCISDIST; }
        else { t0 = 0.f; }
        if (yp1 < SCISDIST) { t1 = (SCISDIST-oyp0)/(yp1-oyp0); xp1 = (xp1-oxp0)*t1+oxp0; yp1 = SCISDIST; }
        else { t1 = 1.f; }

        f = ((float)tspr->yrepeat) * (float)tsizy * 4;

        ryp0 = 1.0/yp0;
        ryp1 = 1.0/yp1;
        sx0 = ghalfx*xp0*ryp0 + ghalfx;
        sx1 = ghalfx*xp1*ryp1 + ghalfx;
        ryp0 *= gyxscale;
        ryp1 *= gyxscale;

        tspr->z -= ((yoff*tspr->yrepeat)<<2);
        if (globalorientation&128)
        {
            tspr->z += ((tsizy*tspr->yrepeat)<<1);
            if (tsizy&1) tspr->z += (tspr->yrepeat<<1); //Odd yspans
        }

        sc0 = ((float)(tspr->z-globalposz-f))*ryp0 + ghoriz;
        sc1 = ((float)(tspr->z-globalposz-f))*ryp1 + ghoriz;
        sf0 = ((float)(tspr->z-globalposz))*ryp0 + ghoriz;
        sf1 = ((float)(tspr->z-globalposz))*ryp1 + ghoriz;

        gdx = (ryp0-ryp1)*gxyaspect / (sx0-sx1);
        gdy = 0;
        gdo = ryp0*gxyaspect - gdx*sx0;

        //Original equations:
        //(gux*sx0 + guo)/(gdx*sx1 + gdo) = tsizx*t0
        //(gux*sx1 + guo)/(gdx*sx1 + gdo) = tsizx*t1
        //
        // gvx*sx0 + gvy*sc0 + gvo = 0
        // gvy*sx1 + gvy*sc1 + gvo = 0
        //(gvx*sx0 + gvy*sf0 + gvo)/(gdx*sx0 + gdo) = tsizy
        //(gvx*sx1 + gvy*sf1 + gvo)/(gdx*sx1 + gdo) = tsizy

        //gux*sx0 + guo = t0*tsizx*yp0
        //gux*sx1 + guo = t1*tsizx*yp1
        if (globalorientation&4) { t0 = 1.f-t0; t1 = 1.f-t1; }

        //sprite panning
        t0 -= ((float)(spriteext[spritenum].xpanning)/255.f);
        t1 -= ((float)(spriteext[spritenum].xpanning)/255.f);
        gux = (t0*ryp0 - t1*ryp1)*gxyaspect*(float)tsizx / (sx0-sx1);
        guy = 0;
        guo = t0*ryp0*gxyaspect*(float)tsizx - gux*sx0;

        //gvx*sx0 + gvy*sc0 + gvo = 0
        //gvx*sx1 + gvy*sc1 + gvo = 0
        //gvx*sx0 + gvy*sf0 + gvo = tsizy*(gdx*sx0 + gdo)
        f = ((float)tsizy)*(gdx*sx0 + gdo) / ((sx0-sx1)*(sc0-sf0));
        if (!(globalorientation&8))
        {
            gvx = (sc0-sc1)*f;
            gvy = (sx1-sx0)*f;
            gvo = -gvx*sx0 - gvy*sc0;
        }
        else
        {
            gvx = (sf1-sf0)*f;
            gvy = (sx0-sx1)*f;
            gvo = -gvx*sx0 - gvy*sf0;
        }

        // sprite panning
        gvx -= gdx*((float)(spriteext[spritenum].ypanning)/255.f)*tsizy;
        gvy -= gdy*((float)(spriteext[spritenum].ypanning)/255.f)*tsizy;
        gvo -= gdo*((float)(spriteext[spritenum].ypanning)/255.f)*tsizy;

        //Clip sprites to ceilings/floors when no parallaxing
        if (!(sector[tspr->sectnum].ceilingstat&1))
        {
            f = ((float)tspr->yrepeat) * (float)tsizy * 4;
            if (sector[tspr->sectnum].ceilingz > tspr->z-f)
            {
                sc0 = ((float)(sector[tspr->sectnum].ceilingz-globalposz))*ryp0 + ghoriz;
                sc1 = ((float)(sector[tspr->sectnum].ceilingz-globalposz))*ryp1 + ghoriz;
            }
        }
        if (!(sector[tspr->sectnum].floorstat&1))
        {
            if (sector[tspr->sectnum].floorz < tspr->z)
            {
                sf0 = ((float)(sector[tspr->sectnum].floorz-globalposz))*ryp0 + ghoriz;
                sf1 = ((float)(sector[tspr->sectnum].floorz-globalposz))*ryp1 + ghoriz;
            }
        }

        if (sx0 > sx1)
        {
            if (globalorientation&64) return; //1-sided sprite
            f = sx0; sx0 = sx1; sx1 = f;
            f = sc0; sc0 = sc1; sc1 = f;
            f = sf0; sf0 = sf1; sf1 = f;
        }

        px[0] = sx0; py[0] = sc0;
        px[1] = sx1; py[1] = sc1;
        px[2] = sx1; py[2] = sf1;
        px[3] = sx0; py[3] = sf0;

#ifdef USE_OPENGL
        if (spriteext[spritenum].xpanning)
            srepeat = 1;
        if (spriteext[spritenum].ypanning)
            trepeat = 1;
#endif

        tilesizx[globalpicnum] = tsizx;
        tilesizy[globalpicnum] = tsizy;
        pow2xsplit = 0; drawpoly(px,py,4,method);

#ifdef USE_OPENGL
        if (spriteext[spritenum].xpanning)
            srepeat = 0;
        if (spriteext[spritenum].ypanning)
            trepeat = 0;
#endif

        break;
    case 2: //Floor sprite

        if ((globalorientation&64) != 0)
            if ((globalposz > tspr->z) == (!(globalorientation&8)))
                return;
        if ((globalorientation&4) > 0) xoff = -xoff;
        if ((globalorientation&8) > 0) yoff = -yoff;

        i = (tspr->ang&2047);
        c = sintable[(i+512)&2047]/65536.0;
        s = sintable[i]/65536.0;
        x0 = (float)((tsizx>>1)-xoff)*tspr->xrepeat;
        y0 = (float)((tsizy>>1)-yoff)*tspr->yrepeat;
        x1 = (float)((tsizx>>1)+xoff)*tspr->xrepeat;
        y1 = (float)((tsizy>>1)+yoff)*tspr->yrepeat;

        //Project 3D to 2D
        for (j=0; j<4; j++)
        {
            sx0 = (float)(tspr->x-globalposx);
            sy0 = (float)(tspr->y-globalposy);
            if ((j+0)&2) { sy0 -= s*y0; sx0 -= c*y0; }
            else { sy0 += s*y1; sx0 += c*y1; }
            if ((j+1)&2) { sx0 -= s*x0; sy0 += c*x0; }
            else { sx0 += s*x1; sy0 -= c*x1; }
            px[j] = sy0*gcosang  - sx0*gsinang;
            py[j] = sx0*gcosang2 + sy0*gsinang2;
        }

        if (tspr->z < globalposz) //if floor sprite is above you, reverse order of points
        {
            f = px[0]; px[0] = px[1]; px[1] = f;
            f = py[0]; py[0] = py[1]; py[1] = f;
            f = px[2]; px[2] = px[3]; px[3] = f;
            f = py[2]; py[2] = py[3]; py[3] = f;
        }

        //Clip to SCISDIST plane
        npoints = 0;
        for (i=0; i<4; i++)
        {
            j = ((i+1)&3);
            if (py[i] >= SCISDIST) { px2[npoints] = px[i]; py2[npoints] = py[i]; npoints++; }
            if ((py[i] >= SCISDIST) != (py[j] >= SCISDIST))
            {
                f = (SCISDIST-py[i])/(py[j]-py[i]);
                px2[npoints] = (px[j]-px[i])*f + px[i];
                py2[npoints] = (py[j]-py[i])*f + py[i]; npoints++;
            }
        }
        if (npoints < 3) return;

        //Project rotated 3D points to screen
        f = ((float)(tspr->z-globalposz))*gyxscale;
        for (j=0; j<npoints; j++)
        {
            ryp0 = 1/py2[j];
            px[j] = ghalfx*px2[j]*ryp0 + ghalfx;
            py[j] = f*ryp0 + ghoriz;
        }

        //gd? Copied from floor rendering code
        gdx = 0;
        gdy = gxyaspect / (double)(tspr->z-globalposz);
        gdo = -ghoriz*gdy;
        //copied&modified from relative alignment
        xv = (float)tspr->x + s*x1 + c*y1; fx = (double)-(x0+x1)*s;
        yv = (float)tspr->y + s*y1 - c*x1; fy = (double)+(x0+x1)*c;
        f = 1.0/sqrt(fx*fx+fy*fy); fx *= f; fy *= f;
        ft[2] = singlobalang*fy + cosglobalang*fx;
        ft[3] = singlobalang*fx - cosglobalang*fy;
        ft[0] = ((double)(globalposy-yv))*fy + ((double)(globalposx-xv))*fx;
        ft[1] = ((double)(globalposx-xv))*fy - ((double)(globalposy-yv))*fx;
        gux = (double)ft[3]*((double)viewingrange)/(-65536.0*262144.0);
        gvx = (double)ft[2]*((double)viewingrange)/(-65536.0*262144.0);
        guy = (double)ft[0]*gdy; gvy = (double)ft[1]*gdy;
        guo = (double)ft[0]*gdo; gvo = (double)ft[1]*gdo;
        guo += (double)(ft[2]/262144.0-gux)*ghalfx;
        gvo -= (double)(ft[3]/262144.0+gvx)*ghalfx;
        f = 4.0/(float)tspr->xrepeat; gux *= f; guy *= f; guo *= f;
        f =-4.0/(float)tspr->yrepeat; gvx *= f; gvy *= f; gvo *= f;
        if (globalorientation&4)
        {
            gux = ((float)tsizx)*gdx - gux;
            guy = ((float)tsizx)*gdy - guy;
            guo = ((float)tsizx)*gdo - guo;
        }

        // sprite panning
        guy -= gdy*((float)(spriteext[spritenum].xpanning)/255.f)*tsizx;
        guo -= gdo*((float)(spriteext[spritenum].xpanning)/255.f)*tsizx;
        gvy -= gdy*((float)(spriteext[spritenum].ypanning)/255.f)*tsizy;
        gvo -= gdo*((float)(spriteext[spritenum].ypanning)/255.f)*tsizy;

#ifdef USE_OPENGL
        if (spriteext[spritenum].xpanning)
            srepeat = 1;
        if (spriteext[spritenum].ypanning)
            trepeat = 1;
#endif

        tilesizx[globalpicnum] = tsizx;
        tilesizy[globalpicnum] = tsizy;
        pow2xsplit = 0; drawpoly(px,py,npoints,method);

#ifdef USE_OPENGL
        if (spriteext[spritenum].xpanning)
            srepeat = 0;
        if (spriteext[spritenum].ypanning)
            trepeat = 0;
#endif

        break;

    case 3: //Voxel sprite
        break;
    }
    tilesizx[globalpicnum]=oldsizx;
    tilesizy[globalpicnum]=oldsizy;
}

//sx,sy       center of sprite; screen coods*65536
//z           zoom*65536. > is zoomed in
//a           angle (0 is default)
//dastat&1    1:translucence
//dastat&2    1:auto-scale mode (use 320*200 coordinates)
//dastat&4    1:y-flip
//dastat&8    1:don't clip to startumost/startdmost
//dastat&16   1:force point passed to be top-left corner, 0:Editart center
//dastat&32   1:reverse translucence
//dastat&64   1:non-masked, 0:masked
//dastat&128  1:draw all pages (permanent)
//cx1,...     clip window (actual screen coords)
void polymost_dorotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                             int8_t dashade, char dapalnum, int32_t dastat, int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2, int32_t uniqid)
{
    static int32_t onumframes = 0;
    int32_t n, nn, x, zz, xoff, yoff, xsiz, ysiz, method;
    int32_t ogpicnum, ogshade, ogpal, ofoffset, oxdimen, oydimen, oldviewingrange;
    double ogxyaspect;
    double ogchang, ogshang, ogctang, ogstang, oghalfx, oghoriz, fx, fy, x1, y1, z1, x2, y2;
    double ogrhalfxdown10, ogrhalfxdown10x;
    double d, cosang, sinang, cosang2, sinang2, px[8], py[8], px2[8], py2[8];
    float m[4][4];
    int32_t oxdim = xdim, oydim = ydim;


#ifdef USE_OPENGL
    if (rendmode >= 3 && usemodels && hudmem[(dastat&4)>>2][picnum].angadd)
    {
        if ((tile2model[Ptile2tile(picnum,dapalnum)].modelid >= 0) && (tile2model[Ptile2tile(picnum,dapalnum)].framenum >= 0))
        {
            spritetype tspr;
            memset(&tspr,0,sizeof(spritetype));

            if (hudmem[(dastat&4)>>2][picnum].flags&1) return; //"HIDE" is specified in DEF

            ogchang = gchang; gchang = 1.0;
            ogshang = gshang; gshang = 0.0; d = (double)z/(65536.0*16384.0);
            ogctang = gctang; gctang = (double)sintable[(a+512)&2047]*d;
            ogstang = gstang; gstang = (double)sintable[a&2047]*d;
            ogshade  = globalshade;  globalshade  = dashade;
            ogpal    = globalpal;    globalpal    = (int32_t)((uint8_t)dapalnum);
            ogxyaspect = gxyaspect; gxyaspect = 1.0;
            oldviewingrange = viewingrange; viewingrange = 65536;

            x1 = hudmem[(dastat&4)>>2][picnum].xadd;
            y1 = hudmem[(dastat&4)>>2][picnum].yadd;
            z1 = hudmem[(dastat&4)>>2][picnum].zadd;
            if (!(hudmem[(dastat&4)>>2][picnum].flags&2)) //"NOBOB" is specified in DEF
            {
                fx = ((double)sx)*(1.0/65536.0);
                fy = ((double)sy)*(1.0/65536.0);

                if (dastat&16)
                {
                    xsiz = tilesizx[picnum]; ysiz = tilesizy[picnum];
                    xoff = (int32_t)((int8_t)((picanm[picnum]>>8)&255))+(xsiz>>1);
                    yoff = (int32_t)((int8_t)((picanm[picnum]>>16)&255))+(ysiz>>1);

                    d = (double)z/(65536.0*16384.0);
                    cosang2 = cosang = (double)sintable[(a+512)&2047]*d;
                    sinang2 = sinang = (double)sintable[a&2047]*d;
                    if ((dastat&2) || (!(dastat&8))) //Don't aspect unscaled perms
                        { d = (double)xyaspect/65536.0; cosang2 *= d; sinang2 *= d; }
                    fx += -(double)xoff*cosang2+ (double)yoff*sinang2;
                    fy += -(double)xoff*sinang - (double)yoff*cosang;
                }

                if (!(dastat&2))
                {
                    x1 += fx/((double)(xdim<<15))-1.0; //-1: left of screen, +1: right of screen
                    y1 += fy/((double)(ydim<<15))-1.0; //-1: top of screen, +1: bottom of screen
                }
                else
                {
                    x1 += fx/160.0-1.0; //-1: left of screen, +1: right of screen
                    y1 += fy/100.0-1.0; //-1: top of screen, +1: bottom of screen
                }
            }
            tspr.ang = hudmem[(dastat&4)>>2][picnum].angadd+globalang;

            if (dastat&4) { x1 = -x1; y1 = -y1; }
            
            // In Polymost, we don't care if the model is very big
            if (rendmode < 4) {
                tspr.xrepeat = tspr.yrepeat = 32;

                tspr.x = (int32_t)(((double)gcosang*z1 - (double)gsinang*x1)*16384.0 + globalposx);
                tspr.y = (int32_t)(((double)gsinang*z1 + (double)gcosang*x1)*16384.0 + globalposy);
                tspr.z = (int32_t)(globalposz + y1*16384.0*0.8);
            } else {
                float x, y, z;

                tspr.xrepeat = tspr.yrepeat = 5;

                x = (float)(((double)gcosang*z1 - (double)gsinang*x1)*2560.0 + globalposx);
                y = (float)(((double)gsinang*z1 + (double)gcosang*x1)*2560.0 + globalposy);
                z = (float)(globalposz + y1*2560.0*0.8);
                
                memcpy(&tspr.x, &x, sizeof(float));
                memcpy(&tspr.y, &y, sizeof(float));
                memcpy(&tspr.z, &z, sizeof(float));
            }
            tspr.picnum = picnum;
            tspr.shade = dashade;
            tspr.pal = dapalnum;
            tspr.owner = uniqid+MAXSPRITES;
            globalorientation = (dastat&1)+((dastat&32)<<4)+((dastat&4)<<1);
            tspr.cstat = globalorientation;

            if ((dastat&10) == 2)
                bglViewport(windowx1,yres-(windowy2+1),windowx2-windowx1+1,windowy2-windowy1+1);
            else
            {
                bglViewport(0,0,xdim,ydim);
                glox1 = -1; //Force fullscreen (glox1=-1 forces it to restore)
            }

            if (rendmode < 4)
            {
                bglMatrixMode(GL_PROJECTION);
                memset(m,0,sizeof(m));
                if ((dastat&10) == 2)
                {
                    float ratioratio = (float)xdim/ydim;
                    m[0][0] = (float)ydimen*(ratioratio >= 1.6?1.2:1); m[0][2] = 1.0;
                    m[1][1] = (float)xdimen; m[1][2] = 1.0;
                    m[2][2] = 1.0; m[2][3] = (float)ydimen*(ratioratio >= 1.6?1.2:1);
                    m[3][2] =-1.0;
                }
                else { m[0][0] = m[2][3] = 1.0f; m[1][1] = ((float)xdim)/((float)ydim); m[2][2] = 1.0001f; m[3][2] = 1-m[2][2]; }
                bglLoadMatrixf(&m[0][0]);
                bglMatrixMode(GL_MODELVIEW);
                bglLoadIdentity();
            }

            if (hudmem[(dastat&4)>>2][picnum].flags&8) //NODEPTH flag
                bglDisable(GL_DEPTH_TEST);
            else
            {
                bglEnable(GL_DEPTH_TEST);
                if (onumframes != numframes)
                {
                    onumframes = numframes;
                    bglClear(GL_DEPTH_BUFFER_BIT);
                }
            }

#ifdef USE_OPENGL
            if (!nofog) bglDisable(GL_FOG);
            if (rendmode < 4)
                mddraw(&tspr);
            else {
                tspriteptr[MAXSPRITESONSCREEN] = &tspr;

                bglEnable(GL_ALPHA_TEST);
                bglEnable(GL_BLEND);
                
                spriteext[tspr.owner].roll = a;
                spriteext[tspr.owner].zoff = z;

                polymer_drawsprite(MAXSPRITESONSCREEN);

                spriteext[tspr.owner].zoff = 0;
                spriteext[tspr.owner].roll = 0;

                bglDisable(GL_BLEND);
                bglDisable(GL_ALPHA_TEST);
            }
            
            if (!nofog) bglEnable(GL_FOG);
#else
            mddraw(&tspr);
#endif

            viewingrange = oldviewingrange;
            gxyaspect = ogxyaspect;
            globalshade  = ogshade;
            globalpal    = ogpal;
            gchang = ogchang;
            gshang = ogshang;
            gctang = ogctang;
            gstang = ogstang;
            return;
        }
    }
#endif

    ogpicnum = globalpicnum; globalpicnum = picnum;
    ogshade  = globalshade;  globalshade  = dashade;
    ogpal    = globalpal;    globalpal    = (int32_t)((uint8_t)dapalnum);
    oghalfx  = ghalfx;       ghalfx       = (double)(xdim>>1);
    ogrhalfxdown10 = grhalfxdown10;    grhalfxdown10 = 1.0/(((double)ghalfx)*1024);
    ogrhalfxdown10x = grhalfxdown10x;  grhalfxdown10x = grhalfxdown10;
    oghoriz  = ghoriz;       ghoriz       = (double)(ydim>>1);
    ofoffset = frameoffset;  frameoffset  = frameplace;
    oxdimen  = xdimen;       xdimen       = xdim;
    oydimen  = ydimen;       ydimen       = ydim;
    ogchang = gchang; gchang = 1.0;
    ogshang = gshang; gshang = 0.0;
    ogctang = gctang; gctang = 1.0;
    ogstang = gstang; gstang = 0.0;

#ifdef USE_OPENGL
    if (rendmode >= 3)
    {
        bglViewport(0,0,xdim,ydim); glox1 = -1; //Force fullscreen (glox1=-1 forces it to restore)
        bglMatrixMode(GL_PROJECTION);
        memset(m,0,sizeof(m));
        m[0][0] = m[2][3] = 1.0f; m[1][1] = ((float)xdim)/((float)ydim); m[2][2] = 1.0001f; m[3][2] = 1-m[2][2];
        bglPushMatrix(); bglLoadMatrixf(&m[0][0]);
        bglMatrixMode(GL_MODELVIEW);
        bglPushMatrix();
        bglLoadIdentity();

        bglDisable(GL_DEPTH_TEST);
        bglDisable(GL_ALPHA_TEST);
        bglEnable(GL_TEXTURE_2D);
    }
#endif

    method = 0;
    if (!(dastat&64))
    {
        method = 1;
        if (dastat&1) { if (!(dastat&32)) method = 2; else method = 3; }
    }
    method |= 4; //Use OpenGL clamping - dorotatesprite never repeats

    xsiz = tilesizx[globalpicnum]; ysiz = tilesizy[globalpicnum];
    if (dastat&16) { xoff = 0; yoff = 0; }
    else
    {
        xoff = (int32_t)((int8_t)((picanm[globalpicnum]>>8)&255))+(xsiz>>1);
        yoff = (int32_t)((int8_t)((picanm[globalpicnum]>>16)&255))+(ysiz>>1);
    }
    if (dastat&4) yoff = ysiz-yoff;

    if (dastat&2)  //Auto window size scaling
    {
        // nasty hacks go here
        if (!(dastat&8))
        {
            x = xdimenscale;   //= scale(xdimen,yxaspect,320);

            if (!(dastat & 1024) && ((double)ydim/(double)xdim) <= .75f)
            {
                xdim = (int32_t)((double)ydim * 1.33333333333333334f);
                setaspect(65536L,(int32_t)divscale16(ydim*320L,xdim*200L));
            }

            if (dastat & 512)
                sx = ((cx1+cx2+2+scale((oxdim-xdim), cx1+cx2+2, oxdim))<<15)+scale(sx-(320<<15),scale(xdimen, xdim, oxdim),320);
            else if (dastat & 256)
                sx = ((cx1+cx2+2-scale((oxdim-xdim), cx1+cx2+2, oxdim))<<15)+scale(sx-(320<<15),scale(xdimen, xdim, oxdim),320);
            else 
                sx = ((cx1+cx2+2)<<15)+scale(sx-(320<<15),scale(xdimen, xdim, oxdim),320);

            sy = ((cy1+cy2+2)<<15)+mulscale16(sy-(200<<15),x);
        }
        else
        {
            //If not clipping to startmosts, & auto-scaling on, as a
            //hard-coded bonus, scale to full screen instead

            if (!(dastat & 1024) && ((double)ydim/(double)xdim) <= .75f)
            {
                xdim = (int32_t)((double)ydim * 1.33333333333333334f);
                setaspect(65536L,(int32_t)divscale16(ydim*320L,xdim*200L));
            }

            x = scale(xdim,yxaspect,320);
            sx = (xdim<<15)+32768+scale(sx-(320<<15),xdim,320);
            sy = (ydim<<15)+32768+mulscale16(sy-(200<<15),x);

            if (dastat & 512)
                sx += (oxdim-xdim)<<16;
            else if ((dastat & 256) == 0)
                sx += (oxdim-xdim)<<15;
        }
        z = mulscale16(z,x);
    }
    else if (!(dastat & 1024) && ((double)ydim/(double)xdim) <= .75f)
    {
        ydim = (int32_t)((double)xdim * 0.75f);
        setaspect(65536L,(int32_t)divscale16(ydim*320L,xdim*200L));
    }
    
    d = (double)z/(65536.0*16384.0);
    cosang2 = cosang = (double)sintable[(a+512)&2047]*d;
    sinang2 = sinang = (double)sintable[a&2047]*d;
    if ((dastat&2) || (!(dastat&8))) //Don't aspect unscaled perms
        { d = (double)xyaspect/65536.0; cosang2 *= d; sinang2 *= d; }
    px[0] = (double)sx/65536.0 - (double)xoff*cosang2+ (double)yoff*sinang2;
    py[0] = (double)sy/65536.0 - (double)xoff*sinang - (double)yoff*cosang;
    px[1] = px[0] + (double)xsiz*cosang2;
    py[1] = py[0] + (double)xsiz*sinang;
    px[3] = px[0] - (double)ysiz*sinang2;
    py[3] = py[0] + (double)ysiz*cosang;
    px[2] = px[1]+px[3]-px[0];
    py[2] = py[1]+py[3]-py[0];
    n = 4;

    gdx = 0; gdy = 0; gdo = 1.0;
    //px[0]*gux + py[0]*guy + guo = 0
    //px[1]*gux + py[1]*guy + guo = xsiz-.0001
    //px[3]*gux + py[3]*guy + guo = 0
    d = 1.0/(px[0]*(py[1]-py[3]) + px[1]*(py[3]-py[0]) + px[3]*(py[0]-py[1]));
    gux = (py[3]-py[0])*((double)xsiz-.0001)*d;
    guy = (px[0]-px[3])*((double)xsiz-.0001)*d;
    guo = 0 - px[0]*gux - py[0]*guy;

    if (!(dastat&4))
    {
        //px[0]*gvx + py[0]*gvy + gvo = 0
        //px[1]*gvx + py[1]*gvy + gvo = 0
        //px[3]*gvx + py[3]*gvy + gvo = ysiz-.0001
        gvx = (py[0]-py[1])*((double)ysiz-.0001)*d;
        gvy = (px[1]-px[0])*((double)ysiz-.0001)*d;
        gvo = 0 - px[0]*gvx - py[0]*gvy;
    }
    else
    {
        //px[0]*gvx + py[0]*gvy + gvo = ysiz-.0001
        //px[1]*gvx + py[1]*gvy + gvo = ysiz-.0001
        //px[3]*gvx + py[3]*gvy + gvo = 0
        gvx = (py[1]-py[0])*((double)ysiz-.0001)*d;
        gvy = (px[0]-px[1])*((double)ysiz-.0001)*d;
        gvo = (double)ysiz-.0001 - px[0]*gvx - py[0]*gvy;
    }

    cx2++; cy2++;
    //Clippoly4 (converted from int32_t to double)
    nn = z = 0;
    do
    {
        zz = z+1; if (zz == n) zz = 0;
        x1 = px[z]; x2 = px[zz]-x1; if ((cx1 <= x1) && (x1 <= cx2)) { px2[nn] = x1; py2[nn] = py[z]; nn++; }
        if (x2 <= 0) fx = cx2; else fx = cx1;  d = fx-x1;
        if ((d < x2) != (d < 0)) { px2[nn] = fx; py2[nn] = (py[zz]-py[z])*d/x2 + py[z]; nn++; }
        if (x2 <= 0) fx = cx1; else fx = cx2;  d = fx-x1;
        if ((d < x2) != (d < 0)) { px2[nn] = fx; py2[nn] = (py[zz]-py[z])*d/x2 + py[z]; nn++; }
        z = zz;
    }
    while (z);
    if (nn >= 3)
    {
        n = z = 0;
        do
        {
            zz = z+1; if (zz == nn) zz = 0;
            y1 = py2[z]; y2 = py2[zz]-y1; if ((cy1 <= y1) && (y1 <= cy2)) { py[n] = y1; px[n] = px2[z]; n++; }
            if (y2 <= 0) fy = cy2; else fy = cy1;  d = fy-y1;
            if ((d < y2) != (d < 0)) { py[n] = fy; px[n] = (px2[zz]-px2[z])*d/y2 + px2[z]; n++; }
            if (y2 <= 0) fy = cy1; else fy = cy2;  d = fy-y1;
            if ((d < y2) != (d < 0)) { py[n] = fy; px[n] = (px2[zz]-px2[z])*d/y2 + px2[z]; n++; }
            z = zz;
        }
        while (z);
#ifdef USE_OPENGL
        if (!nofog) bglDisable(GL_FOG);
        pow2xsplit = 0; drawpoly(px,py,n,method);
        if (!nofog) bglEnable(GL_FOG);
#else
        pow2xsplit = 0; drawpoly(px,py,n,method);
#endif
    }

#ifdef USE_OPENGL
    if (rendmode >= 3)
    {
        bglMatrixMode(GL_PROJECTION); bglPopMatrix();
        bglMatrixMode(GL_MODELVIEW); bglPopMatrix();
    }
#endif

    globalpicnum = ogpicnum;
    globalshade  = ogshade;
    globalpal    = ogpal;
    ghalfx       = oghalfx;
    grhalfxdown10 = ogrhalfxdown10;
    grhalfxdown10x = ogrhalfxdown10x;
    ghoriz       = oghoriz;
    frameoffset  = ofoffset;
    xdimen       = oxdimen;
    ydimen       = oydimen;
    gchang = ogchang;
    gshang = ogshang;
    gctang = ogctang;
    gstang = ogstang;
    xdim = oxdim;
    ydim = oydim;

    setaspect_new();
}

#ifdef USE_OPENGL
static float trapextx[2];
static void drawtrap(float x0, float x1, float y0, float x2, float x3, float y1)
{
    float px[4], py[4];
    int32_t i, n;

    if (y0 == y1) return;
    px[0] = x0; py[0] = y0;  py[2] = y1;
    if (x0 == x1) { px[1] = x3; py[1] = y1; px[2] = x2; n = 3; }
    else if (x2 == x3) { px[1] = x1; py[1] = y0; px[2] = x3; n = 3; }
    else               { px[1] = x1; py[1] = y0; px[2] = x3; px[3] = x2; py[3] = y1; n = 4; }

    bglBegin(GL_TRIANGLE_FAN);
    for (i=0; i<n; i++)
    {
        px[i] = min(max(px[i],trapextx[0]),trapextx[1]);
        bglTexCoord2f(px[i]*gux + py[i]*guy + guo,
                      px[i]*gvx + py[i]*gvy + gvo);
        bglVertex2f(px[i],py[i]);
    }
    bglEnd();
}

static void tessectrap(float *px, float *py, int32_t *point2, int32_t numpoints)
{
    float x0, x1, m0, m1;
    int32_t i, j, k, z, i0, i1, i2, i3, npoints, gap, numrst;

    static int32_t allocpoints = 0, *slist = 0, *npoint2 = 0;
    typedef struct { float x, y, xi; int32_t i; } raster;
    static raster *rst = 0;
    if (numpoints+16 > allocpoints) //16 for safety
    {
        allocpoints = numpoints+16;
        rst = (raster*)Brealloc(rst,allocpoints*sizeof(raster));
        slist = (int32_t*)Brealloc(slist,allocpoints*sizeof(int32_t));
        npoint2 = (int32_t*)Brealloc(npoint2,allocpoints*sizeof(int32_t));
    }

    //Remove unnecessary collinear points:
    for (i=0; i<numpoints; i++) npoint2[i] = point2[i];
    npoints = numpoints; z = 0;
    for (i=0; i<numpoints; i++)
    {
        j = npoint2[i]; if ((point2[i] < i) && (i < numpoints-1)) z = 3;
        if (j < 0) continue;
        k = npoint2[j];
        m0 = (px[j]-px[i])*(py[k]-py[j]);
        m1 = (py[j]-py[i])*(px[k]-px[j]);
        if (m0 < m1) { z |= 1; continue; }
        if (m0 > m1) { z |= 2; continue; }
        npoint2[i] = k; npoint2[j] = -1; npoints--; i--; //collinear
    }
    if (!z) return;
    trapextx[0] = trapextx[1] = px[0];
    for (i=j=0; i<numpoints; i++)
    {
        if (npoint2[i] < 0) continue;
        if (px[i] < trapextx[0]) trapextx[0] = px[i];
        if (px[i] > trapextx[1]) trapextx[1] = px[i];
        slist[j++] = i;
    }
    if (z != 3) //Simple polygon... early out
    {
        bglBegin(GL_TRIANGLE_FAN);
        for (i=0; i<npoints; i++)
        {
            j = slist[i];
            bglTexCoord2f(px[j]*gux + py[j]*guy + guo,
                          px[j]*gvx + py[j]*gvy + gvo);
            bglVertex2f(px[j],py[j]);
        }
        bglEnd();
        return;
    }

    //Sort points by y's
    for (gap=(npoints>>1); gap; gap>>=1)
        for (i=0; i<npoints-gap; i++)
            for (j=i; j>=0; j-=gap)
            {
                if (py[npoint2[slist[j]]] <= py[npoint2[slist[j+gap]]]) break;
                k = slist[j]; slist[j] = slist[j+gap]; slist[j+gap] = k;
            }

    numrst = 0;
    for (z=0; z<npoints; z++)
    {
        i0 = slist[z]; i1 = npoint2[i0]; if (py[i0] == py[i1]) continue;
        i2 = i1; i3 = npoint2[i1];
        if (py[i1] == py[i3]) { i2 = i3; i3 = npoint2[i3]; }

        //i0        i3
        //  \      /
        //   i1--i2
        //  /      \ ~
        //i0        i3

        if ((py[i1] < py[i0]) && (py[i2] < py[i3])) //Insert raster
        {
            for (i=numrst; i>0; i--)
            {
                if (rst[i-1].xi*(py[i1]-rst[i-1].y) + rst[i-1].x < px[i1]) break;
                rst[i+1] = rst[i-1];
            }
            numrst += 2;

            if (i&1) //split inside area
            {
                j = i-1;

                x0 = (py[i1] - rst[j  ].y)*rst[j  ].xi + rst[j  ].x;
                x1 = (py[i1] - rst[j+1].y)*rst[j+1].xi + rst[j+1].x;
                drawtrap(rst[j].x,rst[j+1].x,rst[j].y,x0,x1,py[i1]);
                rst[j  ].x = x0; rst[j  ].y = py[i1];
                rst[j+3].x = x1; rst[j+3].y = py[i1];
            }

            m0 = (px[i0]-px[i1]) / (py[i0]-py[i1]);
            m1 = (px[i3]-px[i2]) / (py[i3]-py[i2]);
            j = ((px[i1] > px[i2]) || ((i1 == i2) && (m0 >= m1))) + i;
            k = (i<<1)+1 - j;
            rst[j].i = i0; rst[j].xi = m0; rst[j].x = px[i1]; rst[j].y = py[i1];
            rst[k].i = i3; rst[k].xi = m1; rst[k].x = px[i2]; rst[k].y = py[i2];
        }
        else
        {
            //NOTE:don't count backwards!
            if (i1 == i2) { for (i=0; i<numrst; i++) if (rst[i].i == i1) break; }
            else { for (i=0; i<numrst; i++) if ((rst[i].i == i1) || (rst[i].i == i2)) break; }
            j = i&~1;

            if ((py[i1] > py[i0]) && (py[i2] > py[i3])) //Delete raster
            {
                for (; j<=i+1; j+=2)
                {
                    x0 = (py[i1] - rst[j  ].y)*rst[j  ].xi + rst[j  ].x;
                    if ((i == j) && (i1 == i2)) x1 = x0; else x1 = (py[i1] - rst[j+1].y)*rst[j+1].xi + rst[j+1].x;
                    drawtrap(rst[j].x,rst[j+1].x,rst[j].y,x0,x1,py[i1]);
                    rst[j  ].x = x0; rst[j  ].y = py[i1];
                    rst[j+1].x = x1; rst[j+1].y = py[i1];
                }
                numrst -= 2; for (; i<numrst; i++) rst[i] = rst[i+2];
            }
            else
            {
                x0 = (py[i1] - rst[j  ].y)*rst[j  ].xi + rst[j  ].x;
                x1 = (py[i1] - rst[j+1].y)*rst[j+1].xi + rst[j+1].x;
                drawtrap(rst[j].x,rst[j+1].x,rst[j].y,x0,x1,py[i1]);
                rst[j  ].x = x0; rst[j  ].y = py[i1];
                rst[j+1].x = x1; rst[j+1].y = py[i1];

                if (py[i0] < py[i3]) { rst[i].x = px[i2]; rst[i].y = py[i2]; rst[i].i = i3; }
                else { rst[i].x = px[i1]; rst[i].y = py[i1]; rst[i].i = i0; }
                rst[i].xi = (px[rst[i].i] - rst[i].x) / (py[rst[i].i] - py[i1]);
            }
        }
    }
}

void polymost_fillpolygon(int32_t npoints)
{
    pthtyp *pth;
    float f,a=0.0;
    int32_t i;

    globalx1 = mulscale16(globalx1,xyaspect);
    globaly2 = mulscale16(globaly2,xyaspect);
    gux = ((double)asm1)*(1.0/4294967296.0);
    gvx = ((double)asm2)*(1.0/4294967296.0);
    guy = ((double)globalx1)*(1.0/4294967296.0);
    gvy = ((double)globaly2)*(-1.0/4294967296.0);
    guo = (((double)xdim)*gux + ((double)ydim)*guy)*-.5 + ((double)globalposx)*(1.0/4294967296.0);
    gvo = (((double)xdim)*gvx + ((double)ydim)*gvy)*-.5 - ((double)globalposy)*(1.0/4294967296.0);

    //Convert int32_t to float (in-place)
    for (i=npoints-1; i>=0; i--)
    {
        ((float *)rx1)[i] = ((float)rx1[i])/4096.0;
        ((float *)ry1)[i] = ((float)ry1[i])/4096.0;
    }

    if (gloy1 != -1) setpolymost2dview(); //disables blending, texturing, and depth testing
    bglEnable(GL_ALPHA_TEST);
    bglEnable(GL_TEXTURE_2D);
    pth = gltexcache(globalpicnum,globalpal,0);
    bglBindTexture(GL_TEXTURE_2D, pth ? pth->glpic : 0);

    f = ((float)(numpalookups-min(max((globalshade * shadescale),0),numpalookups)))/((float)numpalookups);
    switch ((globalorientation>>7)&3)
    {
    case 0:
    case 1:
        a = 1.0f; bglDisable(GL_BLEND); break;
    case 2:
        a = 0.66f; bglEnable(GL_BLEND); break;
    case 3:
        a = 0.33f; bglEnable(GL_BLEND); break;
    }
    bglColor4f(f,f,f,a);

    tessectrap((float *)rx1,(float *)ry1,xb1,npoints);
}
#endif

int32_t polymost_drawtilescreen(int32_t tilex, int32_t tiley, int32_t wallnum, int32_t dimen, int32_t tilezoom)
{
#ifdef USE_OPENGL
    float xdime, ydime, xdimepad, ydimepad, scx, scy, ratio = 1.0;
    int32_t i;
    pthtyp *pth;

    if ((rendmode < 3) || (qsetmode != 200)) return(-1);

    if (!glinfo.texnpot)
    {
        i = (1<<(picsiz[wallnum]&15)); if (i < tilesizx[wallnum]) i += i; xdimepad = (float)i;
        i = (1<<(picsiz[wallnum]>>4)); if (i < tilesizy[wallnum]) i += i; ydimepad = (float)i;
    }
    else
    {
        xdimepad = (float)tilesizx[wallnum];
        ydimepad = (float)tilesizy[wallnum];
    }
    xdime = (float)tilesizx[wallnum]; xdimepad = xdime/xdimepad;
    ydime = (float)tilesizy[wallnum]; ydimepad = ydime/ydimepad;

    if ((xdime <= dimen) && (ydime <= dimen))
    {
        scx = xdime;
        scy = ydime;
    }
    else
    {
        scx = (float)dimen;
        scy = (float)dimen;
        if (xdime < ydime) scx *= xdime/ydime; else scy *= ydime/xdime;
    }

    pth = gltexcache(wallnum,0,4);
    bglBindTexture(GL_TEXTURE_2D,pth ? pth->glpic : 0);

    bglDisable(GL_ALPHA_TEST);

    if (tilezoom)
    {
        if (scx > scy) ratio = dimen/scx;
        else ratio = dimen/scy;
    }

    if (!pth || (pth->flags & 8))
    {
        bglDisable(GL_TEXTURE_2D);
        bglBegin(GL_TRIANGLE_FAN);
        if (gammabrightness)
            bglColor4f((float)curpalette[255].r/255.0,
                       (float)curpalette[255].g/255.0,
                       (float)curpalette[255].b/255.0,
                       1);
        else
            bglColor4f((float)britable[curbrightness][ curpalette[255].r ] / 255.0,
                       (float)britable[curbrightness][ curpalette[255].g ] / 255.0,
                       (float)britable[curbrightness][ curpalette[255].b ] / 255.0,
                       1);
        bglVertex2f((float)tilex    ,(float)tiley);
        bglVertex2f((float)tilex+(scx*ratio),(float)tiley);
        bglVertex2f((float)tilex+(scx*ratio),(float)tiley+(scy*ratio));
        bglVertex2f((float)tilex    ,(float)tiley+(scy*ratio));
        bglEnd();
    }

    bglColor4f(1,1,1,1);
    bglEnable(GL_TEXTURE_2D);
    bglEnable(GL_BLEND);
    bglBegin(GL_TRIANGLE_FAN);
    bglTexCoord2f(0,       0); bglVertex2f((float)tilex    ,(float)tiley);
    bglTexCoord2f(xdimepad,       0); bglVertex2f((float)tilex+(scx*ratio),(float)tiley);
    bglTexCoord2f(xdimepad,ydimepad); bglVertex2f((float)tilex+(scx*ratio),(float)tiley+(scy*ratio));
    bglTexCoord2f(0,ydimepad); bglVertex2f((float)tilex    ,(float)tiley+(scy*ratio));
    bglEnd();

    return(0);
#else
    return -1;
#endif
}

int32_t polymost_printext256(int32_t xpos, int32_t ypos, int16_t col, int16_t backcol, const char *name, char fontsize)
{
#ifndef USE_OPENGL
    return -1;
#else
    GLfloat tx, ty, txc, tyc;
    int32_t c;
    palette_t p,b;

    if (gammabrightness)
    {
        p = curpalette[col];
        b = curpalette[backcol];
    }
    else
    {
        p.r = britable[curbrightness][ curpalette[col].r ];
        p.g = britable[curbrightness][ curpalette[col].g ];
        p.b = britable[curbrightness][ curpalette[col].b ];
        b.r = britable[curbrightness][ curpalette[backcol].r ];
        b.g = britable[curbrightness][ curpalette[backcol].g ];
        b.b = britable[curbrightness][ curpalette[backcol].b ];
    }

    if ((rendmode < 3) || (qsetmode != 200)) return(-1);

    if (!polymosttext)
    {
        // construct a 256x128 8-bit alpha-only texture for the font glyph matrix
        char *tbuf, *cptr, *tptr;
        int32_t h,i,j;

        bglGenTextures(1,&polymosttext);
        if (!polymosttext) return -1;

        tbuf = (char *)Bmalloc(256*128);
        if (!tbuf)
        {
            bglDeleteTextures(1,&polymosttext);
            polymosttext = 0;
            return -1;
        }
        Bmemset(tbuf, 0, 256*128);

        cptr = (char*)textfont;
        for (h=0; h<256; h++)
        {
            tptr = tbuf + (h%32)*8 + (h/32)*256*8;
            for (i=0; i<8; i++)
            {
                for (j=0; j<8; j++)
                {
                    if (cptr[h*8+i] & pow2char[7-j]) tptr[j] = 255;
                }
                tptr += 256;
            }
        }

        cptr = (char*)smalltextfont;
        for (h=0; h<256; h++)
        {
            tptr = tbuf + 256*64 + (h%32)*8 + (h/32)*256*8;
            for (i=1; i<7; i++)
            {
                for (j=2; j<6; j++)
                {
                    if (cptr[h*8+i] & pow2char[7-j]) tptr[j-2] = 255;
                }
                tptr += 256;
            }
        }

        bglBindTexture(GL_TEXTURE_2D, polymosttext);
        bglTexImage2D(GL_TEXTURE_2D,0,GL_ALPHA,256,128,0,GL_ALPHA,GL_UNSIGNED_BYTE,(GLvoid*)tbuf);
        bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        Bfree(tbuf);
    }
    else bglBindTexture(GL_TEXTURE_2D, polymosttext);

    setpolymost2dview();	// disables blending, texturing, and depth testing
    bglDisable(GL_ALPHA_TEST);
    bglDepthMask(GL_FALSE);	// disable writing to the z-buffer

    if (backcol >= 0)
    {
        bglColor4ub(b.r,b.g,b.b,255);
        c = Bstrlen(name);

        bglBegin(GL_QUADS);
        bglVertex2i(xpos,ypos);
        bglVertex2i(xpos,ypos+(fontsize?6:8));
        bglVertex2i(xpos+(c<<(3-fontsize)),ypos+(fontsize?6:8));
        bglVertex2i(xpos+(c<<(3-fontsize)),ypos);
        bglEnd();
    }

    bglPushAttrib(GL_POLYGON_BIT);  // we want to have readable text in wireframe mode, too
    bglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    bglEnable(GL_TEXTURE_2D);
    bglEnable(GL_BLEND);
    bglColor4ub(p.r,p.g,p.b,255);
    txc = fontsize ? (4.0/256.0) : (8.0/256.0);
    tyc = fontsize ? (6.0/128.0) : (8.0/128.0);
    bglBegin(GL_QUADS);
    for (c=0; name[c]; c++)
    {
        if (name[c] == '^' && isdigit(name[c+1]))
        {
            char smallbuf[8];
            int32_t bi=0;
            while (isdigit(name[c+1]) && bi<8)
            {
                smallbuf[bi++]=name[c+1];
                c++;
            }
            smallbuf[bi++]=0;
            if (col)col = atol(smallbuf);

            if (gammabrightness)
            {
                p = curpalette[col];
            }
            else
            {
                p.r = britable[curbrightness][ curpalette[col].r ];
                p.g = britable[curbrightness][ curpalette[col].g ];
                p.b = britable[curbrightness][ curpalette[col].b ];
            }
            bglColor4ub(p.r,p.g,p.b,255);
            continue;
        }
        tx = (float)(name[c]%32)/32.0;
        ty = (float)((name[c]/32) + (fontsize*8))/16.0;

        bglTexCoord2f(tx,ty);         bglVertex2i(xpos,ypos);
        bglTexCoord2f(tx+txc,ty);     bglVertex2i(xpos+(8>>fontsize),ypos);
        bglTexCoord2f(tx+txc,ty+tyc); bglVertex2i(xpos+(8>>fontsize),ypos+(fontsize?6:8));
        bglTexCoord2f(tx,ty+tyc);     bglVertex2i(xpos,ypos+(fontsize?6:8));

        xpos += (8>>fontsize);
    }
    bglEnd();

    bglDepthMask(GL_TRUE);	// re-enable writing to the z-buffer
    bglPopAttrib();

    return 0;
#endif
}

// Console commands by JBF
#ifdef USE_OPENGL
static int32_t gltexturemode(const osdfuncparm_t *parm)
{
    int32_t m;
    const char *p;

    if (parm->numparms != 1)
    {
        OSD_Printf("Current texturing mode is %s\n", glfiltermodes[gltexfiltermode].name);
        OSD_Printf("  Vaild modes are:\n");
        for (m = 0; m < (int32_t)numglfiltermodes; m++)
            OSD_Printf("     %d - %s\n",m,glfiltermodes[m].name);

        return OSDCMD_OK;
    }

    m = Bstrtoul(parm->parms[0], (char **)&p, 10);
    if (p == parm->parms[0])
    {
        // string
        for (m = 0; m < (int32_t)numglfiltermodes; m++)
        {
            if (!Bstrcasecmp(parm->parms[0], glfiltermodes[m].name)) break;
        }
        if (m == numglfiltermodes) m = gltexfiltermode;   // no change
    }
    else
    {
        if (m < 0) m = 0;
        else if (m >= (int32_t)numglfiltermodes) m = numglfiltermodes - 1;
    }

    gltexfiltermode = m;
    gltexapplyprops();

    OSD_Printf("Texture filtering mode changed to %s\n", glfiltermodes[gltexfiltermode].name);

    return OSDCMD_OK;
}
#endif

static int32_t osdcmd_cvar_set_polymost(const osdfuncparm_t *parm)
{
    int32_t r = osdcmd_cvar_set(parm);

    if (xdim == 0 || ydim == 0 || bpp == 0) // video not set up yet
        return r;

#ifdef USE_OPENGL
    if (r == OSDCMD_OK)
    {
        if (!Bstrcasecmp(parm->name, "r_swapinterval"))
        {
            setvsync(vsync);
            return r;
        }
        else if (!Bstrcasecmp(parm->name, "r_downsize"))
        {
            if (r_downsize != r_downsizevar && r_downsizevar != -1)
            {
                invalidatecache();
                resetvideomode();
                if (setgamemode(fullscreen,xdim,ydim,bpp))
                    OSD_Printf("restartvid: Reset failed...\n");
            }
            else r_downsizevar = r_downsize;

            return r;
        }
        else if (!Bstrcasecmp(parm->name, "r_textureanisotropy"))
        {
            gltexapplyprops();
            return r;
        }
        else if (!Bstrcasecmp(parm->name, "r_texturemode"))
        {
            gltexturemode(parm);
            return r;
        }
#ifdef POLYMER
        else if (!Bstrcasecmp(parm->name, "r_pr_maxlightpasses"))
        {
            if (pr_maxlightpasses != r_pr_maxlightpasses)
            {
                polymer_invalidatelights();
                pr_maxlightpasses = r_pr_maxlightpasses;
            }
            return r;
        }
#endif
    }
#endif
    return r;
}

void polymost_initosdfuncs(void)
{
    uint32_t i;

    cvar_t cvars_polymost[] =
    {
#ifdef USE_OPENGL
        { "r_animsmoothing","r_animsmoothing: enable/disable model animation smoothing",(void *)&r_animsmoothing, CVAR_BOOL, 0, 1 },
        { "r_modelocclusionchecking","r_modelocclusionchecking: enable/disable hack to cull \"obstructed\" models",(void *)&r_modelocclusionchecking, CVAR_INT, 0, 2 },
        { "r_detailmapping","r_detailmapping: enable/disable detail mapping",(void *)&r_detailmapping, CVAR_BOOL, 0, 1 },
        { "r_downsize","r_downsize: controls downsizing factor for hires textures",(void *)&r_downsize, CVAR_INT|CVAR_FUNCPTR, 0, 5 },
        { "r_fullbrights","r_fullbrights: enable/disable fullbright textures",(void *)&r_fullbrights, CVAR_BOOL, 0, 1 },
        { "r_glowmapping","r_glowmapping: enable/disable glow mapping",(void *)&r_glowmapping, CVAR_BOOL, 0, 1 },
        /*
                { "r_multisample","r_multisample: sets the number of samples used for antialiasing (0 = off)",(void *)&r_glowmapping, CVAR_BOOL, 0, 1 }
                { "r_nvmultisamplehint","r_nvmultisamplehint: enable/disable Nvidia multisampling hinting",(void *)&glnvmultisamplehint, CVAR_BOOL, 0, 1 }
        */
        {
            "r_parallaxskyclamping","r_parallaxskyclamping: enable/disable parallaxed floor/ceiling sky texture clamping",
            (void *)&r_parallaxskyclamping, CVAR_BOOL, 0, 1
        },
        {
            "r_parallaxskypanning","r_parallaxskypanning: enable/disable parallaxed floor/ceiling panning when drawing a parallaxed sky",
            (void *)&r_parallaxskypanning, CVAR_BOOL, 0, 1
        },
        { "r_polygonmode","r_polygonmode: debugging feature",(void *)&glpolygonmode, CVAR_INT | CVAR_NOSAVE, 0, 3 },
        { "r_redbluemode","r_redbluemode: enable/disable experimental OpenGL red-blue glasses mode",(void *)&glredbluemode, CVAR_BOOL, 0, 1 },
        { "r_shadescale","r_shadescale: multiplier for lighting",(void *)&shadescale, CVAR_FLOAT, 0, 10 },
        { "r_swapinterval","r_swapinterval: sets the GL swap interval (VSync)",(void *)&vsync, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
        { "r_texcache","r_texcache: enable/disable OpenGL compressed texture cache",(void *)&glusetexcache, CVAR_INT, 0, 2 },
        { "r_texcompr","r_texcompr: enable/disable OpenGL texture compression",(void *)&glusetexcompr, CVAR_BOOL, 0, 1 },
        { "r_textureanisotropy", "r_textureanisotropy: changes the OpenGL texture anisotropy setting", (void *)&glanisotropy, CVAR_INT|CVAR_FUNCPTR, 0, 16 },
        { "r_texturemaxsize","r_texturemaxsize: changes the maximum OpenGL texture size limit",(void *)&gltexmaxsize, CVAR_INT | CVAR_NOSAVE, 0, 4096 },
        { "r_texturemiplevel","r_texturemiplevel: changes the highest OpenGL mipmap level used",(void *)&gltexmiplevel, CVAR_INT, 0, 6 },
        { "r_texturemode", "r_texturemode: changes the texture filtering settings", (void *)&gltexfiltermode, CVAR_INT|CVAR_FUNCPTR, 0, 5 },
        { "r_vbocount","r_vbocount: sets the number of Vertex Buffer Objects to use when drawing models",(void *)&r_vbocount, CVAR_INT, 1, 256 },
        { "r_vbos","r_vbos: enable/disable using Vertex Buffer Objects when drawing models",(void *)&r_vbos, CVAR_BOOL, 0, 1 },
        { "r_vertexarrays","r_vertexarrays: enable/disable using vertex arrays when drawing models",(void *)&r_vertexarrays, CVAR_BOOL, 0, 1 },
        { "r_anamorphic", "r_anamorphic: enable/disable widescreen mode", (void*)&glwidescreen, CVAR_BOOL, 0, 1 },
        { "r_projectionhack", "r_projectionhack: enable/disable projection hack", (void*)&glprojectionhacks, CVAR_INT, 0, 2 },

#ifdef POLYMER
        // polymer cvars
        { "r_pr_lighting", "r_pr_lighting: enable/disable dynamic lights", (void*)&pr_lighting, CVAR_BOOL, 0, 1 },
        { "r_pr_normalmapping", "r_pr_normalmapping: enable/disable virtual displacement mapping", (void*)&pr_normalmapping, CVAR_BOOL, 0, 1 },
        { "r_pr_specularmapping", "r_pr_specularmapping: enable/disable specular mapping", (void*)&pr_specularmapping, CVAR_BOOL, 0, 1 },
        { "r_pr_shadows", "r_pr_shadows: enable/disable dynamic shadows", (void*)&pr_shadows, CVAR_BOOL, 0, 1 },
        { "r_pr_shadowcount", "r_pr_shadowcount: maximal amount of shadow emitting lights on screen - you need to restart the renderer for it to take effect", (void*)&pr_shadowcount, CVAR_INT, 0, 64 },
        { "r_pr_shadowdetail", "r_pr_shadowdetail: sets the shadow map resolution - you need to restart the renderer for it to take effect", (void*)&pr_shadowdetail, CVAR_INT, 0, 5 },
        { "r_pr_shadowfiltering", "r_pr_shadowfiltering: enable/disable shadow edges filtering - you need to restart the renderer for it to take effect", (void*)&pr_shadowfiltering, CVAR_BOOL, 0, 1 },
        { "r_pr_maxlightpasses", "r_pr_maxlightpasses: the maximal amount of lights a single object can by affected by", (void*)&r_pr_maxlightpasses, CVAR_INT|CVAR_FUNCPTR, 0, PR_MAXLIGHTS },
        { "r_pr_maxlightpriority", "r_pr_maxlightpriority: lowering that value removes less meaningful lights from the scene", (void*)&pr_maxlightpriority, CVAR_INT, 0, PR_MAXLIGHTPRIORITY },
        { "r_pr_fov", "r_pr_fov: sets the field of vision in build angle", (void*)&pr_fov, CVAR_INT, 0, 1023},
        { "r_pr_customaspect", "r_pr_customaspect: if non-zero, forces the 3D view aspect ratio", (void*)&pr_customaspect, CVAR_FLOAT, 0, 3 },
        { "r_pr_billboardingmode", "r_pr_billboardingmode: face sprite display method. 0: classic mode; 1: polymost mode", (void*)&pr_billboardingmode, CVAR_INT, 0, 1 },
        { "r_pr_verbosity", "r_pr_verbosity: verbosity level of the polymer renderer", (void*)&pr_verbosity, CVAR_INT, 0, 3 },
        { "r_pr_wireframe", "r_pr_wireframe: toggles wireframe mode", (void*)&pr_wireframe, CVAR_INT | CVAR_NOSAVE, 0, 1 },
        { "r_pr_vbos", "r_pr_vbos: contols Vertex Buffer Object usage. 0: no VBOs. 1: VBOs for map data. 2: VBOs for model data.", (void*)&pr_vbos, CVAR_INT, 0, 2 },
        { "r_pr_gpusmoothing", "r_pr_gpusmoothing: toggles model animation interpolation", (void*)&pr_gpusmoothing, CVAR_INT, 0, 1 },
        { "r_pr_overrideparallax", "r_pr_overrideparallax: overrides parallax mapping scale and bias values with values from the pr_parallaxscale and pr_parallaxbias cvars; use it to fine-tune DEF tokens", (void*)&pr_overrideparallax, CVAR_BOOL | CVAR_NOSAVE, 0, 1 },
        { "r_pr_parallaxscale", "r_pr_parallaxscale: overriden parallax mapping offset scale", (void*)&pr_parallaxscale, CVAR_FLOAT | CVAR_NOSAVE, -10, 10 },
        { "r_pr_parallaxbias", "r_pr_parallaxbias: overriden parallax mapping offset bias", (void*)&pr_parallaxbias, CVAR_FLOAT | CVAR_NOSAVE, -10, 10 },
        { "r_pr_overridespecular", "r_pr_overridespecular: overrides specular material power and factor values with values from the pr_specularpower and pr_specularfactor cvars; use it to fine-tune DEF tokens", (void*)&pr_overridespecular, CVAR_BOOL | CVAR_NOSAVE, 0, 1 },
        { "r_pr_specularpower", "r_pr_specularpower: overriden specular material power", (void*)&pr_specularpower, CVAR_FLOAT | CVAR_NOSAVE, -10, 1000 },
        { "r_pr_specularfactor", "r_pr_specularfactor: overriden specular material factor", (void*)&pr_specularfactor, CVAR_FLOAT | CVAR_NOSAVE, -10, 1000 },
        { "r_pr_ati_fboworkaround", "r_pr_ati_fboworkaround: enable this to workaround an ATI driver bug that causes sprite shadows to be square - you need to restart the renderer for it to take effect", (void*)&pr_ati_fboworkaround, CVAR_BOOL | CVAR_NOSAVE, 0, 1 },
        { "r_pr_ati_nodepthoffset", "r_pr_ati_nodepthoffset: enable this to workaround an ATI driver bug that causes sprite drawing to freeze the game on Radeon X1x00 hardware - you need to restart the renderer for it to take effect", (void*)&pr_ati_nodepthoffset, CVAR_BOOL | CVAR_NOSAVE, 0, 1 },
#endif

        { "r_models","r_models: enable/disable model rendering",(void *)&usemodels, CVAR_BOOL, 0, 1 },
        { "r_hightile","r_hightile: enable/disable hightile texture rendering",(void *)&usehightile, CVAR_BOOL, 0, 1 },

        { "r_preview_mouseaim", "r_preview_mouseaim: toggles mouse aiming preview, use this to calibrate yxaspect in Polymost Mapster32", (void *)&preview_mouseaim, CVAR_BOOL, 0, 1 },
#endif
    };

    for (i=0; i<sizeof(cvars_polymost)/sizeof(cvars_polymost[0]); i++)
    {
        if (OSD_RegisterCvar(&cvars_polymost[i]))
            continue;

        OSD_RegisterFunction(cvars_polymost[i].name, cvars_polymost[i].helpstr,
                             cvars_polymost[i].type & CVAR_FUNCPTR ? osdcmd_cvar_set_polymost : osdcmd_cvar_set);
    }
}

void polymost_precache(int32_t dapicnum, int32_t dapalnum, int32_t datype)
{
#ifdef USE_OPENGL
    // dapicnum and dapalnum are like you'd expect
    // datype is 0 for a wall/floor/ceiling and 1 for a sprite
    //    basically this just means walls are repeating
    //    while sprites are clamped
    int32_t mid;

    if (rendmode < 3) return;

    if ((palookup[dapalnum] == NULL) && (dapalnum < (MAXPALOOKUPS - RESERVEDPALS))) return;//dapalnum = 0;

    //OSD_Printf("precached %d %d type %d\n", dapicnum, dapalnum, datype);
    hicprecaching = 1;


    gltexcache(dapicnum, dapalnum, (datype & 1) << 2);
    hicprecaching = 0;

    if (datype == 0 || !usemodels) return;

    mid = md_tilehasmodel(dapicnum,dapalnum);
    if (mid < 0 || models[mid]->mdnum < 2) return;

    {
        int32_t i,j=0;

        if (models[mid]->mdnum == 3)
            j = ((md3model_t *)models[mid])->head.numsurfs;

        for (i=0; i<=j; i++)
        {
            mdloadskin((md2model_t*)models[mid], 0, dapalnum, i);
        }
    }
#endif
}

static uint16_t hicosub(uint16_t c)
{
    int32_t r, g, b;
    g = ((c>> 5)&63);
    r = ((c>>11)-(g>>1))&31;
    b = ((c>> 0)-(g>>1))&31;
    return((r<<11)+(g<<5)+b);
}

static uint16_t hicoadd(uint16_t c)
{
    int32_t r, g, b;
    g = ((c>> 5)&63);
    r = ((c>>11)+(g>>1))&31;
    b = ((c>> 0)+(g>>1))&31;
    return((r<<11)+(g<<5)+b);
}

/*
Description of Ken's filter to improve LZW compression of DXT1 format by ~15%: (as tested with the HRP)

 To increase LZW patterns, I store each field of the DXT block structure separately.
 Here are the 3 DXT fields:
 1.  __int64 alpha_4x4; //DXT3 only (16 byte structure size when included)
 2.  short rgb0, rgb1;
 3.  int32_t index_4x4;

 Each field is then stored with its own specialized algorithm.
 1. I haven't done much testing with this field - I just copy it raw without any transform for now.

 2. For rgb0 and rgb1, I use a "green" filter like this:
 g = g;
 r = r-g;
 b = b-g;
 For grayscale, this makes the stream: x,0,0,x,0,0,x,0,0,... instead of x,x,x,x,x,x,x,x,...
 Q:what was the significance of choosing green? A:largest/most dominant component
 Believe it or not, this gave 1% better compression :P
 I tried subtracting each componenet with the previous pixel, but strangely it hurt compression.
 Oh, the joy of trial & error. :)

 3. For index_4x4, I transform the ordering of 2-bit indices in the DXT blocks from this:
 0123 0123 0123  ---- ---- ----
 4567 4567 4567  ---- ---- ----
 89ab 89ab 89ab  ---- ---- ----
 cdef cdef cdef  ---- ---- ----
 To this: (I swap x & y axes)
 048c 048c 048c  |||| |||| ||||
 159d 159d 159d  |||| |||| ||||
 26ae 26ae 26ae  |||| |||| ||||
 37bf 37bf 37bf  |||| |||| ||||
 The trick is: going from the bottom of the 4th line to the top of the 5th line
 is the exact same jump (geometrically) as from 5th to 6th, etc.. This is not true in the top case.
 These silly tricks will increase patterns and therefore make LZW compress better.
 I think this improved compression by a few % :)
 */

#if defined(POLYMOST) && defined(USE_OPENGL)
int32_t dxtfilter(int32_t fil, texcachepicture *pict, char *pic, void *midbuf, char *packbuf, uint32_t miplen)
{
    void *writebuf;
    uint32_t j, k, offs, stride, cleng;
    char *cptr;

    if ((pict->format == B_LITTLE32(GL_COMPRESSED_RGB_S3TC_DXT1_EXT)) ||
        (pict->format == B_LITTLE32(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT))) { offs = 0; stride = 8; }
    else if ((pict->format == B_LITTLE32(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)) ||
        (pict->format == B_LITTLE32(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT))) { offs = 8; stride = 16; }
    else { offs = 0; stride = 8; }

    if (stride == 16) //If DXT3...
    {
        //alpha_4x4
        cptr = midbuf;
        for (k=0; k<8; k++) *cptr++ = pic[k];
        for (j=stride; (unsigned)j<miplen; j+=stride)
            for (k=0; k<8; k++) *cptr++ = pic[j+k];
        if (glusetexcache == 2)
        {
            j = (miplen/stride)<<3;
            cleng = qlz_compress(midbuf,packbuf,j,state_compress);
            if (cleng == 0 || cleng > j-1)
            {
                cleng = j;
                writebuf = midbuf;
            }
            else writebuf = packbuf;
        }
        else
        {
            cleng = (miplen/stride)<<3;
            writebuf = midbuf;
        }
        j = B_LITTLE32(cleng);
        Bwrite(fil,&j,sizeof(j));
        Bwrite(fil,writebuf,cleng);
    }

    //rgb0,rgb1
    cptr = midbuf;
    for (k=0; k<=2; k+=2)
        for (j=0; (unsigned)j<miplen; j+=stride)
        { *(int16_t *)cptr = hicosub(*(int16_t *)(&pic[offs+j+k])); cptr += 2; }
        if (glusetexcache == 2)
        {
            j = (miplen/stride)<<2;
            cleng = qlz_compress(midbuf,packbuf,j,state_compress);
            if (cleng == 0 || cleng > j-1)
            {
                cleng = j;
                writebuf = midbuf;
            }
            else writebuf = packbuf;
        }
        else
        {
            cleng = (miplen/stride)<<2;
            writebuf = midbuf;
        }
        j = B_LITTLE32(cleng);
        Bwrite(fil,&j,sizeof(j));
        Bwrite(fil,writebuf,cleng);

        //index_4x4
        cptr = midbuf;
        for (j=0; (unsigned)j<miplen; j+=stride)
        {
            char *c2 = &pic[j+offs+4];
            cptr[0] = ((c2[0]>>0)&3) + (((c2[1]>>0)&3)<<2) + (((c2[2]>>0)&3)<<4) + (((c2[3]>>0)&3)<<6);
            cptr[1] = ((c2[0]>>2)&3) + (((c2[1]>>2)&3)<<2) + (((c2[2]>>2)&3)<<4) + (((c2[3]>>2)&3)<<6);
            cptr[2] = ((c2[0]>>4)&3) + (((c2[1]>>4)&3)<<2) + (((c2[2]>>4)&3)<<4) + (((c2[3]>>4)&3)<<6);
            cptr[3] = ((c2[0]>>6)&3) + (((c2[1]>>6)&3)<<2) + (((c2[2]>>6)&3)<<4) + (((c2[3]>>6)&3)<<6);
            cptr += 4;
        }
        if (glusetexcache == 2)
        {
            j = (miplen/stride)<<2;
            cleng = qlz_compress(midbuf,packbuf,j,state_compress);
            if (cleng == 0 || cleng > j-1)
            {
                cleng = j;
                writebuf = midbuf;
            }
            else writebuf = packbuf;
        }
        else
        {
            cleng = (miplen/stride)<<2;
            writebuf = midbuf;
        }
        j = B_LITTLE32(cleng);
        Bwrite(fil,&j,sizeof(j));
        Bwrite(fil,writebuf,cleng);
        return 0;
}

int32_t dedxtfilter(int32_t fil, texcachepicture *pict, char *pic, void *midbuf, char *packbuf, int32_t ispacked)
{
    void *inbuf;
    int32_t j, k, offs, stride, cleng;
    char *cptr;

    if (ispacked) inbuf = packbuf; else inbuf = midbuf;

    if ((pict->format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT) ||
            (pict->format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)) { offs = 0; stride = 8; }
    else if ((pict->format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) ||
             (pict->format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)) { offs = 8; stride = 16; }
    else { offs = 0; stride = 8; }

    if (stride == 16) //If DXT3...
    {
        //alpha_4x4
        if (memcachedata && memcachesize >= (signed)(cachepos + sizeof(int32_t)))
        {
            cleng = *(int32_t *)(memcachedata + cachepos);
            cachepos += sizeof(int32_t);
        }
        else
        {
            Blseek(fil, cachepos, BSEEK_SET);
            cachepos += sizeof(int32_t);
            if (Bread(fil,&cleng,sizeof(int32_t)) < (signed)sizeof(int32_t)) return -1;
            cleng = B_LITTLE32(cleng);
        }

        j = (pict->size/stride)*8;

        if (ispacked && cleng < j) inbuf = packbuf; else inbuf = midbuf;

        if (memcachedata && memcachesize >= (signed)(cachepos + cleng))
        {
            if (ispacked && cleng < j)
            {           
                if (qlz_decompress((const char *)memcachedata + cachepos,midbuf,state_decompress) == 0)
                {
                    cachepos += cleng;
                    return -1;
                }
            }
            else Bmemcpy(inbuf, memcachedata + cachepos, cleng);

            cachepos += cleng;
        }
        else
        {
            Blseek(fil, cachepos, BSEEK_SET);
            cachepos += cleng;
            if (Bread(fil,inbuf,cleng) < cleng) return -1;


            if (ispacked && cleng < j)
                if (qlz_decompress(packbuf,midbuf,state_decompress) == 0) return -1;
        }

        cptr = midbuf;
        for (k=0; k<8; k++) pic[k] = *cptr++;
        for (j=stride; j<pict->size; j+=stride)
            for (k=0; k<8; k++) pic[j+k] = (*cptr++);
    }

    //rgb0,rgb1
    if (memcachedata && memcachesize >= (signed)(cachepos + sizeof(int32_t)))
    {
        cleng = *(int32_t *)(memcachedata + cachepos);
        cachepos += sizeof(int32_t);
    }
    else
    {
        Blseek(fil, cachepos, BSEEK_SET);
        cachepos += sizeof(int32_t);
        if (Bread(fil,&cleng,sizeof(int32_t)) < (signed)sizeof(int32_t)) return -1;
        cleng = B_LITTLE32(cleng);
    }

    j = (pict->size/stride)*4;
    if (ispacked && cleng < j) inbuf = packbuf; else inbuf = midbuf;

    if (memcachedata && memcachesize >= (signed)(cachepos + cleng))
    {
        if (ispacked && cleng < j)
        {           
            if (qlz_decompress((const char *)memcachedata + cachepos,midbuf,state_decompress) == 0)
            {
                cachepos += cleng;
                return -1;
            }
        }
        else Bmemcpy(inbuf, memcachedata + cachepos, cleng);

        cachepos += cleng;
    }
    else
    {
        Blseek(fil, cachepos, BSEEK_SET);
        cachepos += cleng;
        if (Bread(fil,inbuf,cleng) < cleng) return -1;

        if (ispacked && cleng < j)
            if (qlz_decompress(packbuf,midbuf,state_decompress) == 0) return -1;
    }

    cptr = midbuf;
    for (k=0; k<=2; k+=2)
    {
        for (j=0; j<pict->size; j+=stride)
        {
            *(int16_t *)(&pic[offs+j+k]) = hicoadd(*(int16_t *)cptr);
            cptr += 2;
        }
    }

    //index_4x4:
    if (memcachedata && memcachesize >= (signed)(cachepos + sizeof(int32_t)))
    {
        cleng = *(int32_t *)(memcachedata + cachepos);
        cachepos += sizeof(int32_t);
    }
    else
    {  
        Blseek(fil, cachepos, BSEEK_SET);
        cachepos += sizeof(int32_t);
        if (Bread(fil,&cleng,sizeof(int32_t)) < (signed)sizeof(int32_t)) return -1; 
        cleng = B_LITTLE32(cleng);
    }

    j = (pict->size/stride)*4;
    if (ispacked && cleng < j) inbuf = packbuf; else inbuf = midbuf;

    if (memcachedata && memcachesize >= (signed)(cachepos + cleng))
    {
        if (ispacked && cleng < j)
        {           
            if (qlz_decompress((const char *)memcachedata + cachepos,midbuf,state_decompress) == 0)
            {
                cachepos += cleng;
                return -1;
            }
        }
        else Bmemcpy(inbuf, memcachedata + cachepos, cleng);

        cachepos += cleng;
    }
    else
    {
        Blseek(fil, cachepos, BSEEK_SET);
        cachepos += cleng;
        if (Bread(fil,inbuf,cleng) < cleng) return -1;

        if (ispacked && cleng < j)
            if (qlz_decompress(packbuf,midbuf,state_decompress) == 0) return -1;
    }

    cptr = midbuf;
    for (j=0; j<pict->size; j+=stride)
    {
        pic[j+offs+4] = ((cptr[0]>>0)&3) + (((cptr[1]>>0)&3)<<2) + (((cptr[2]>>0)&3)<<4) + (((cptr[3]>>0)&3)<<6);
        pic[j+offs+5] = ((cptr[0]>>2)&3) + (((cptr[1]>>2)&3)<<2) + (((cptr[2]>>2)&3)<<4) + (((cptr[3]>>2)&3)<<6);
        pic[j+offs+6] = ((cptr[0]>>4)&3) + (((cptr[1]>>4)&3)<<2) + (((cptr[2]>>4)&3)<<4) + (((cptr[3]>>4)&3)<<6);
        pic[j+offs+7] = ((cptr[0]>>6)&3) + (((cptr[1]>>6)&3)<<2) + (((cptr[2]>>6)&3)<<4) + (((cptr[3]>>6)&3)<<6);
        cptr += 4;
    }
    return 0;
}
#endif

#else /* POLYMOST */

#include "inttypes.h"
int32_t polymost_drawtilescreen(int32_t tilex, int32_t tiley, int32_t wallnum, int32_t dimen) { return -1; }

#endif

// vim:ts=4:sw=4:
