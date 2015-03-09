/**************************************************************************************************
"POLYMOST" code originally written by Ken Silverman
Ken Silverman's official web site: http://www.advsys.net/ken
**************************************************************************************************/


#ifdef USE_OPENGL

#include "compat.h"
#include "build.h"
#include "glbuild.h"
#include "mdsprite.h"
#include "pragmas.h"
#include "baselayer.h"
#include "osd.h"
#include "engine_priv.h"
#include "hightile.h"
#include "polymost.h"
#include "polymer.h"
#include "cache1d.h"
#include "kplib.h"
#include "texcache.h"
#include "common.h"

#ifdef EDUKE32_GLES
#include "jwzgles.h"
#endif

#ifndef _WIN32
extern int32_t filelength(int h); // kplib.c
#endif

extern char textfont[2048], smalltextfont[2048];

int32_t rendmode=0;
#ifdef __ANDROID__
int32_t usemodels=0;
#else
int32_t usemodels=1;
#endif
int32_t usehightile=1;
int32_t vsync=0;

#include <math.h> //<-important!
#include <float.h>

typedef struct { float x, cy[2], fy[2]; int32_t tag; int16_t n, p, ctag, ftag; } vsptyp;
#define VSPMAX 4096 //<- careful!
static vsptyp vsp[VSPMAX];
static int32_t gtag;

static float dxb1[MAXWALLSB], dxb2[MAXWALLSB];

#define SCISDIST .9999f  //1.0: Close plane clipping distance

float shadescale = 1.0f;
int32_t shadescale_unbounded = 0;

int32_t r_usenewshading = 3;
int32_t r_usetileshades = 2;
int32_t r_npotwallmode = 0;

static float gviewxrange;
static float ghoriz;
float gxyaspect;
float gyxscale, ghalfx, grhalfxdown10, grhalfxdown10x;
float gcosang, gsinang, gcosang2, gsinang2;
float gchang, gshang, gctang, gstang, gvisibility;
float gtang = 0.f;
static double guo, gux, guy; //Screen-based texture mapping parameters
static double gvo, gvx, gvy;
static double gdo, gdx, gdy;
float fcosglobalang, fsinglobalang;
float fxdim, fydim, fydimen, fviewingrange;
static int32_t preview_mouseaim=0;  // when 1, displays a CROSSHAIR tsprite at the _real_ aimed position

#ifdef USE_OPENGL
static int32_t srepeat = 0, trepeat = 0;

#ifdef REDBLUEMODE
int32_t glredbluemode = 0;
static int32_t lastglredbluemode = 0, redblueclearcnt = 0;
#endif

struct glfiltermodes glfiltermodes[NUMGLFILTERMODES] =
{
    {"GL_NEAREST",GL_NEAREST,GL_NEAREST},
    {"GL_LINEAR",GL_LINEAR,GL_LINEAR},
    {"GL_NEAREST_MIPMAP_NEAREST",GL_NEAREST_MIPMAP_NEAREST,GL_NEAREST},
    {"GL_LINEAR_MIPMAP_NEAREST",GL_LINEAR_MIPMAP_NEAREST,GL_LINEAR},
    {"GL_NEAREST_MIPMAP_LINEAR",GL_NEAREST_MIPMAP_LINEAR,GL_NEAREST},
    {"GL_LINEAR_MIPMAP_LINEAR",GL_LINEAR_MIPMAP_LINEAR,GL_LINEAR}
};

int32_t glanisotropy = 1;            // 0 = maximum supported by card
int32_t gltexfiltermode = 2; // GL_NEAREST_MIPMAP_NEAREST

#ifdef EDUKE32_GLES
int32_t glusetexcompr = 0;
int32_t glusetexcache = 0, glusememcache = 0;
#else
int32_t glusetexcompr = 1;
int32_t glusetexcache = 2, glusememcache = 1;
int32_t glpolygonmode = 0;     // 0:GL_FILL,1:GL_LINE,2:GL_POINT //FUK
int32_t glmultisample = 0, glnvmultisamplehint = 0;
static int32_t lastglpolygonmode = 0; //FUK
int32_t r_detailmapping = 1;
int32_t r_glowmapping = 1;
#endif

int32_t gltexmaxsize = 0;      // 0 means autodetection on first run
int32_t gltexmiplevel = 0;		// discards this many mipmap levels
int32_t glprojectionhacks = 1;
static GLuint polymosttext = 0;
int32_t glrendmode = REND_POLYMOST;

// This variable, and 'shadeforfullbrightpass' control the drawing of
// fullbright tiles.  Also see 'fullbrightloadingpass'.
static int32_t fullbrightdrawingpass = 0;

int32_t r_vertexarrays = 1;
int32_t r_vbos = 1;
int32_t r_vbocount = 64;
int32_t r_animsmoothing = 1;
int32_t r_fullbrights = 1;
int32_t r_downsize = 0;
int32_t r_downsizevar = -1;

// used for fogcalc
static float fogresult, fogresult2, fogcol[4], fogtable[4*MAXPALOOKUPS];
#endif

static const float float_trans[4] = { 1.0f, 1.0f, 0.66f, 0.33f };

char ptempbuf[MAXWALLSB<<1];

// polymost ART sky control
int32_t r_parallaxskyclamping = 1;
int32_t r_parallaxskypanning = 0;

#define MIN_CACHETIME_PRINT 10

// this was faster in MSVC but slower with GCC... currently unknown on ARM where both
// the FPU and possibly the optimization path in the compiler need improvement
#if 0
static inline int32_t __float_as_int(float f) { return *(int32_t *) &f; }
static inline float __int_as_float(int32_t d) { return *(float *) &d; }
static inline float Bfabsf(float f) { return __int_as_float(__float_as_int(f)&0x7fffffff); }
#else
#define Bfabsf fabsf
#endif

#ifdef USE_OPENGL
int32_t mdtims, omdtims;
float alphahackarray[MAXTILES];
int32_t drawingskybox = 0;
int32_t hicprecaching = 0;

#if 0
static inline int32_t gltexmayhavealpha(int32_t dapicnum, int32_t dapalnum)
{
    const int32_t j = (dapicnum&(GLTEXCACHEADSIZ-1));
    pthtyp *pth;

    for (pth=texcache.list[j]; pth; pth=pth->next)
        if (pth->picnum == dapicnum && pth->palnum == dapalnum)
            return ((pth->flags&PTH_HASALPHA) != 0);

    return 1;
}
#endif

void gltexinvalidate(int32_t dapicnum, int32_t dapalnum, int32_t dameth)
{
    const int32_t j = (dapicnum&(GLTEXCACHEADSIZ-1));
    pthtyp *pth;

    for (pth=texcache.list[j]; pth; pth=pth->next)
        if (pth->picnum == dapicnum && pth->palnum == dapalnum &&
            (pth->flags & PTH_CLAMPED) == TO_PTH_CLAMPED(dameth))
        {
            pth->flags |= PTH_INVALIDATED;
            if (pth->flags & PTH_HASFULLBRIGHT)
                pth->ofb->flags |= PTH_INVALIDATED;
        }
}

//Make all textures "dirty" so they reload, but not re-allocate
//This should be much faster than polymost_glreset()
//Use this for palette effects ... but not ones that change every frame!
void gltexinvalidatetype(int32_t type)
{
    int32_t j;
    pthtyp *pth;

    for (j=0; j<=GLTEXCACHEADSIZ-1; j++)
    {
        for (pth=texcache.list[j]; pth; pth=pth->next)
        {
            if (type == INVALIDATE_ALL || (type == INVALIDATE_ART && pth->hicr == NULL))
            {
                pth->flags |= PTH_INVALIDATED;
                if (pth->flags & PTH_HASFULLBRIGHT)
                    pth->ofb->flags |= PTH_INVALIDATED;
            }
        }
    }

    if (type == INVALIDATE_ALL)
        clearskins();
#ifdef DEBUGGINGAIDS
    OSD_Printf("gltexinvalidateall()\n");
#endif
}

static void bind_2d_texture(GLuint texture, int filter)
{
    if (filter == -1)
        filter = gltexfiltermode;

    bglBindTexture(GL_TEXTURE_2D, texture);
    bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glfiltermodes[filter].mag);
    bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glfiltermodes[filter].min);
#ifndef EDUKE32_GLES
    if (glinfo.maxanisotropy > 1.f)
        bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, glanisotropy);
#endif
}

void gltexapplyprops(void)
{
    int32_t i;
    pthtyp *pth;

    if (getrendermode() == REND_CLASSIC)
        return;

    if (glinfo.maxanisotropy > 1.f)
    {
        if (glanisotropy <= 0 || glanisotropy > glinfo.maxanisotropy)
            glanisotropy = (int32_t)glinfo.maxanisotropy;
    }

    gltexfiltermode = clamp(gltexfiltermode, 0, NUMGLFILTERMODES-1);

    for (i=0; i<=GLTEXCACHEADSIZ-1; i++)
    {
        for (pth=texcache.list[i]; pth; pth=pth->next)
        {
#ifndef EDUKE32_TOUCH_DEVICES
            bind_2d_texture(pth->glpic, -1);

            if (r_fullbrights && pth->flags & PTH_HASFULLBRIGHT)
                bind_2d_texture(pth->ofb->glpic, -1);
#else
            bind_2d_texture(pth->glpic, pth->flags & PTH_HIGHTILE ? 5 : -1);

            if (r_fullbrights && pth->flags & PTH_HASFULLBRIGHT)
                bind_2d_texture(pth->ofb->glpic, pth->flags & PTH_HIGHTILE ? 5 : -1);
#endif
        }
    }

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
            bind_2d_texture(m->texid[j], -1);
        }

        for (sk=m->skinmap; sk; sk=sk->next)
            for (j=0; j<(HICEFFECTMASK+1); j++)
            {
                if (!sk->texid[j]) continue;
                bind_2d_texture(sk->texid[j], -1);
            }
    }
}

//--------------------------------------------------------------------------------------------------

float glox1, gloy1, glox2, gloy2;

//Use this for both initialization and uninitialization of OpenGL.
static int32_t gltexcacnum = -1;

void polymost_glreset()
{
    int32_t i;
    pthtyp *pth, *next;

    for (i=0; i<=MAXPALOOKUPS-1; i++)
    {
        fogtable[i<<2] = palookupfog[i].r * (1.f/63.f);
        fogtable[(i<<2)+1] = palookupfog[i].g * (1.f/63.f);
        fogtable[(i<<2)+2] = palookupfog[i].b * (1.f/63.f);
        fogtable[(i<<2)+3] = 0;
    }

    //Reset if this is -1 (meaning 1st texture call ever), or > 0 (textures in memory)
    if (gltexcacnum < 0)
    {
        gltexcacnum = 0;

        //Hack for polymost_dorotatesprite calls before 1st polymost_drawrooms()
        gcosang = gcosang2 = 16384.f/262144.f;
        gsinang = gsinang2 = 0.f/262144.f;
    }
    else
    {
        for (i=0; i<=GLTEXCACHEADSIZ-1; i++)
        {
            for (pth=texcache.list[i]; pth;)
            {
                next = pth->next;
                if (pth->flags & PTH_HASFULLBRIGHT)
                {
                    bglDeleteTextures(1,&pth->ofb->glpic);
                    Bfree(pth->ofb);
                }

                bglDeleteTextures(1,&pth->glpic);
                Bfree(pth);
                pth = next;
            }

            texcache.list[i] = NULL;
        }
        clearskins();
    }

    if (polymosttext)
        bglDeleteTextures(1,&polymosttext);
    polymosttext=0;

    md_freevbos();

    Bmemset(texcache.list,0,sizeof(texcache.list));
    glox1 = -1;

    texcache_freeptrs();
    texcache_syncmemcache();

#ifdef DEBUGGINGAIDS
    OSD_Printf("polymost_glreset()\n");
#endif
}


// one-time initialization of OpenGL for polymost
void polymost_glinit()
{
    bglHint(GL_FOG_HINT, GL_NICEST);
    bglFogi(GL_FOG_MODE, (r_usenewshading < 2) ? GL_EXP2 : GL_LINEAR);
    bglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bglPixelStorei(GL_PACK_ALIGNMENT, 1);
    bglPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    //bglHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    //bglEnable(GL_LINE_SMOOTH);

#ifndef EDUKE32_GLES
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
#endif

    if (r_vbos && (!glinfo.vbos))
    {
        OSD_Printf("Your OpenGL implementation doesn't support Vertex Buffer Objects. Disabling...\n");
        r_vbos = 0;
    }

    bglEnableClientState(GL_VERTEX_ARRAY);
    bglEnableClientState(GL_TEXTURE_COORD_ARRAY);

    texcache_init();
    texcache_loadoffsets();
    texcache_openfiles();
    
    texcache_setupmemcache();
    texcache_checkgarbage();
}

////////// VISIBILITY FOG ROUTINES //////////
extern int32_t nofog;  // in windows/SDL layers

// only for r_usenewshading < 2 (not preferred)
static void fogcalc_old(int32_t shade, int32_t vis)
{
    float f;

    if (r_usenewshading==1)
    {
        f = 0.9f * shade;
        f = (vis > 239) ? (float)(gvisibility*((vis-240+f))) :
            (float)(gvisibility*(vis+16+f));
    }
    else
    {
        f = (shade < 0) ? shade * 3.5f : shade * .66f;
        f = (vis > 239) ? (float)(gvisibility*((vis-240+f)/(klabs(vis-256)))) :
            (float)(gvisibility*(vis+16+f));
    }

    if (f < 0.001f)
        f = 0.001f;
    else if (f > 100.0f)
        f = 100.0f;

    fogresult = f;
}

// For GL_LINEAR fog:
#define FOGDISTCONST 600
#define FULLVIS_BEGIN 2.9e30
#define FULLVIS_END 3.0e30

static inline void fogcalc(int32_t tile, int32_t shade, int32_t vis, int32_t pal)
{
    if (shade > 0 && getrendermode() == REND_POLYMOST && r_usetileshades == 1 &&
        !(globalflags & GLOBAL_NO_GL_TILESHADES) &&
        (!usehightile || !hicfindsubst(tile, pal)) &&
        (!usemodels || md_tilehasmodel(tile, pal) < 0))
        shade >>= 1;

    Bmemcpy(fogcol, &fogtable[pal<<2], sizeof(fogcol));

    if (r_usenewshading < 2)
        fogcalc_old(shade, vis);
    else
    {
        float combvis = (float) globalvisibility * (uint8_t) (vis+16);

        if (combvis == 0)
        {
            if (r_usenewshading == 2 && shade > 0)
            {
                // beg = -D*shade, end = D*(NUMSHADES-1-shade)
                //  => end/beg = -(NUMSHADES-1-shade)/shade
                fogresult = (float) -FULLVIS_BEGIN;
                fogresult2 = FULLVIS_BEGIN * (float) (numshades-1-shade)/shade;
            }
            else
            {
                fogresult = (float) FULLVIS_BEGIN;
                fogresult2 = (float) FULLVIS_END;
            }
        }
        else if (r_usenewshading == 3 && shade >= numshades-1)
        {
            fogresult = -1;
            fogresult2 = 0;
        }
        else
        {
            combvis = 1.f/combvis;
            fogresult = (r_usenewshading == 3 && shade > 0) ? 0 : -(FOGDISTCONST * shade) * combvis;
            fogresult2 = (FOGDISTCONST * (numshades-1-shade)) * combvis;
        }
    }
}

void calc_and_apply_fog(int32_t tile, int32_t shade, int32_t vis, int32_t pal)
{
    fogcalc(tile, shade, vis, pal);
    bglFogfv(GL_FOG_COLOR, fogcol);

    if (r_usenewshading < 2)
        bglFogf(GL_FOG_DENSITY, fogresult);
    else
    {
        bglFogf(GL_FOG_START, fogresult);
        bglFogf(GL_FOG_END, fogresult2);
    }
}

void calc_and_apply_fog_factor(int32_t tile, int32_t shade, int32_t vis, int32_t pal, float factor)
{
    // NOTE: for r_usenewshading >= 2, the fog beginning/ending distance results are
    // unused.
    fogcalc(tile, shade, vis, pal);
    bglFogfv(GL_FOG_COLOR, fogcol);

    if (r_usenewshading < 2)
        bglFogf(GL_FOG_DENSITY, fogresult*factor);
    else
    {
        bglFogf(GL_FOG_START, (GLfloat) FULLVIS_BEGIN);
        bglFogf(GL_FOG_END, (GLfloat) FULLVIS_END);
    }
}
////////////////////


static float get_projhack_ratio(void)
{
    float rv;

    if (glprojectionhacks == 1)
    {
        float const mul = (gshang * gshang);
        rv = 1.05f + mul * mul * mul * mul;
    }
    else if (glprojectionhacks == 2)
    {
        float const abs_shang = Bfabsf(gshang);
        rv = (abs_shang > 0.7f) ?
            1.05f + 4.f * (abs_shang - 0.7f) : 1.f;
    }
    else
        rv = 1.f;

    // No projection hacks (legacy or new-aspect)
    return rv;
}

static void resizeglcheck(void)
{
#ifdef REDBLUEMODE
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
#endif

#ifndef EDUKE32_GLES
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
#else
    bglPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
#endif

    if ((glox1 != windowx1) || (gloy1 != windowy1) || (glox2 != windowx2) || (gloy2 != windowy2))
    {
        const int32_t ourxdimen = (windowx2-windowx1+1);
        float ratio = get_projhack_ratio();
        const int32_t fovcorrect = (ratio==0) ? 0 : (int32_t)(ourxdimen*ratio - ourxdimen);
        float m[4][4];

        glox1 = (float)windowx1; gloy1 = (float)windowy1;
        glox2 = (float)windowx2; gloy2 = (float)windowy2;

        bglViewport(windowx1-(fovcorrect/2), yres-(windowy2+1),
                    ourxdimen+fovcorrect, windowy2-windowy1+1);

        bglMatrixMode(GL_PROJECTION);
        Bmemset(m,0,sizeof(m));
        ratio = 1.f/ratio;
        m[0][0] = fydimen * ratio; m[0][2] = 1.f;
        m[1][1] = fxdimen; m[1][2] = 1.f;
        m[2][2] = 1.f; m[2][3] = fydimen * ratio;
        m[3][2] =-1.f;
        bglLoadMatrixf(&m[0][0]);

        bglMatrixMode(GL_MODELVIEW);
        bglLoadIdentity();

#ifdef USE_OPENGL
        if (!nofog) bglEnable(GL_FOG);
#endif

        //bglEnable(GL_TEXTURE_2D);
    }
}

static void fixtransparency(coltype *dapic, vec2_t dasiz, vec2_t dasiz2, int32_t dameth)
{
    int32_t y, naxsiz2;
    vec2_t doxy = { dasiz2.x-1, dasiz2.y-1 };

    if (dameth&4) { doxy.x = min(doxy.x, dasiz.x); doxy.y = min(doxy.y, dasiz.y); }
    else { dasiz = dasiz2; } //Make repeating textures duplicate top/left parts

    dasiz.x--; dasiz.y--; naxsiz2 = -dasiz2.x; //Hacks for optimization inside loop

    //Set transparent pixels to average color of neighboring opaque pixels
    //Doing this makes bilinear filtering look much better for masked textures (I.E. sprites)
    for (y=doxy.y; y>=0; y--)
    {
        int32_t x;
        coltype * wpptr = &dapic[y*dasiz2.x+doxy.x];

        for (x=doxy.x; x>=0; x--,wpptr--)
        {
            int32_t r=0, g=0, b=0, j=0;

            if (wpptr->a) continue;

            r = g = b = j = 0;
            if ((x>     0) && (wpptr[     -1].a)) { r += wpptr[     -1].r; g += wpptr[     -1].g; b += wpptr[     -1].b; j++; }
            if ((x<dasiz.x) && (wpptr[     +1].a)) { r += wpptr[     +1].r; g += wpptr[     +1].g; b += wpptr[     +1].b; j++; }
            if ((y>     0) && (wpptr[naxsiz2].a)) { r += wpptr[naxsiz2].r; g += wpptr[naxsiz2].g; b += wpptr[naxsiz2].b; j++; }
            if ((y<dasiz.y) && (wpptr[dasiz2.x].a)) { r += wpptr[dasiz2.x].r; g += wpptr[dasiz2.x].g; b += wpptr[dasiz2.x].b; j++; }
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
            }
        }
    }
}

void uploadtexture(int32_t doalloc, int32_t xsiz, int32_t ysiz, int32_t intexfmt, int32_t texfmt,
                   coltype *pic, int32_t tsizx, int32_t tsizy, int32_t dameth)
{
    int32_t x2, y2, j, js=0;
    const int32_t hi = (dameth & DAMETH_HI) ? 1 : 0;
    const int32_t nocompress = (dameth & DAMETH_NOCOMPRESS) ? 1 : 0;

    dameth &= ~(DAMETH_HI|DAMETH_NOCOMPRESS);

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
        int32_t y;
        int32_t x3 = max(1, x2 >> 1), y3 = max(1, y2 >> 1);  // this came from the GL_ARB_texture_non_power_of_two spec
        //x3 = ((x2+1)>>1); y3 = ((y2+1)>>1);

        for (y=0; y<y3; y++)
        {
            int32_t x;
            coltype *wpptr = &pic[y*x3], *rpptr = &pic[(y<<1)*x2];

            for (x=0; x<x3; x++,wpptr++,rpptr+=2)
            {
                int32_t r=0, g=0, b=0, a=0, k=0;

                if (rpptr[0].a)                  { r += rpptr[0].r; g += rpptr[0].g; b += rpptr[0].b; a += rpptr[0].a; k++; }
                if ((x+x+1 < x2) && (rpptr[1].a)) { r += rpptr[1].r; g += rpptr[1].g; b += rpptr[1].b; a += rpptr[1].a; k++; }
                if (y+y+1 < y2)
                {
                    if ((rpptr[x2].a)) { r += rpptr[x2  ].r; g += rpptr[x2  ].g; b += rpptr[x2  ].b; a += rpptr[x2  ].a; k++; }
                    if ((x+x+1 < x2) && (rpptr[x2+1].a)) { r += rpptr[x2+1].r; g += rpptr[x2+1].g; b += rpptr[x2+1].b; a += rpptr[x2+1].a; k++; }
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
                    EDUKE32_UNREACHABLE_SECTION(break);
                }
                //if (wpptr->a) wpptr->a = 255;
            }
        }

        if (tsizx >= 0)
        {
            vec2_t tsizzle;
            vec2_t mnizzle = { x3, y3 };

            tsizzle.x = (tsizx+(1<<j)-1)>>j;
            tsizzle.y = (tsizy+(1<<j)-1)>>j;
            fixtransparency(pic, tsizzle, mnizzle, dameth);
        }

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


#if 0
// TODO: make configurable
static int32_t tile_is_sky(int32_t tilenum)
{
    return return (tilenum >= 78 /*CLOUDYOCEAN*/ && tilenum <= 99 /*REDSKY2*/);
}
#else
# define tile_is_sky(x) (0)
#endif

static void texture_setup(const int32_t dameth, int filter)
{
    const GLuint clamp_mode = glinfo.clamptoedge ? GL_CLAMP_TO_EDGE : GL_CLAMP;

    if (filter == -1)
        filter = gltexfiltermode;

    gltexfiltermode = clamp(gltexfiltermode, 0, NUMGLFILTERMODES - 1);
    bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glfiltermodes[filter].mag);
    bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glfiltermodes[filter].min);

#ifndef EDUKE32_GLES
    if (glinfo.maxanisotropy > 1.f)
    {
        uint32_t i = (unsigned)Blrintf(glinfo.maxanisotropy);

        if ((unsigned)glanisotropy > i)
            glanisotropy = i;

        bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, glanisotropy);
    }
#endif

    if (!(dameth & DAMETH_CLAMPED))
    {
        bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, !tile_is_sky(dapic) ? GL_REPEAT : clamp_mode);
        bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    else
    {
        // For sprite textures, clamping looks better than wrapping
        bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp_mode);
        bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp_mode);
    }
}

void gloadtile_art(int32_t dapic, int32_t dapal, int32_t dashade, int32_t dameth, pthtyp *pth, int32_t doalloc)
{
    static int32_t fullbrightloadingpass = 0;

    vec2_t siz, tsiz = tilesiz[dapic];

    if (!glinfo.texnpot)
    {
        for (siz.x = 1; siz.x < tsiz.x; siz.x += siz.x);
        for (siz.y = 1; siz.y < tsiz.y; siz.y += siz.y);
    }
    else
    {
        if ((tsiz.x|tsiz.y) == 0)
            siz.x = siz.y = 1;
        else
        {
            siz.x = tsiz.x;
            siz.y = tsiz.y;
        }
    }

    coltype *pic = (coltype *)Xmalloc(siz.x*siz.y*sizeof(coltype));
    char hasalpha = 0, hasfullbright = 0;

    if (!waloff[dapic])
    {
        //Force invalid textures to draw something - an almost purely transparency texture
        //This allows the Z-buffer to be updated for mirrors (which are invalidated textures)
        pic[0].r = pic[0].g = pic[0].b = 0; pic[0].a = 1;
        tsiz.x = tsiz.y = 1; hasalpha = 1;
    }
    else
    {
        const int dofullbright = !(picanm[dapic].sf & PICANM_NOFULLBRIGHT_BIT) && !(globalflags & GLOBAL_NO_GL_FULLBRIGHT);

        for (int y = 0; y < siz.y; y++)
        {
            coltype *wpptr = &pic[y * siz.x];
            int32_t y2 = (y < tsiz.y) ? y : y - tsiz.y;

            for (int x = 0; x < siz.x; x++, wpptr++)
            {
                int32_t dacol;
                int32_t x2 = (x < tsiz.x) ? x : x-tsiz.x;

                if ((dameth & DAMETH_CLAMPED) && (x >= tsiz.x || y >= tsiz.y)) //Clamp texture
                {
                    wpptr->r = wpptr->g = wpptr->b = wpptr->a = 0;
                    continue;
                }

                dacol = *(char *)(waloff[dapic]+x2*tsiz.y+y2);

                if (!fullbrightloadingpass)
                {
                    // regular texture
                    if (dacol > 239 && dacol != 255 && dofullbright)
                        hasfullbright = 1;

                    wpptr->a = 255;
                }
                else
                {
                    // texture with only fullbright areas
                    if (dacol < 240)    // regular colors
                    {
                        wpptr->a = 0;
                        hasalpha = 1;
                    }
                    else   // fullbright
                    {
                        wpptr->a = 255;
                    }
                }

                if (dacol != 255)
                {
                    char *p = (char *)(palookup[dapal])+(int32_t)(dashade<<8);
                    dacol = (uint8_t)p[dacol];
                }
                else
                {
                    wpptr->a = 0;
                    hasalpha = 1;
                }

                bricolor((palette_t *)wpptr, dacol);
            }
        }
    }

    if (doalloc) bglGenTextures(1,(GLuint *)&pth->glpic); //# of textures (make OpenGL allocate structure)
    bglBindTexture(GL_TEXTURE_2D,pth->glpic);

    fixtransparency(pic,tsiz,siz,dameth);

    int32_t npoty = 0;

    if (polymost_want_npotytex(dameth, siz.y) &&
            tsiz.x==siz.x && tsiz.y==siz.y)  // XXX
    {
        const int32_t nextpoty = 1<<((picsiz[dapic]>>4)+1);
        const int32_t ydif = nextpoty - siz.y;
        coltype *paddedpic;

        Bassert(ydif > 0 && ydif < siz.y);

        paddedpic = (coltype*) Xrealloc(pic, siz.x*nextpoty*sizeof(coltype));

        pic = paddedpic;
        Bmemcpy(&pic[siz.x*siz.y], pic, siz.x*ydif*sizeof(coltype));
        siz.y = tsiz.y = nextpoty;

        npoty = PTH_NPOTWALL;
    }

    uploadtexture(doalloc, siz.x, siz.y, hasalpha ? GL_RGBA : GL_RGB, GL_RGBA, pic, tsiz.x, tsiz.y, dameth);

    Bfree(pic);

    texture_setup(dameth, -1);

    pth->picnum = dapic;
    pth->palnum = dapal;
    pth->shade = dashade;
    pth->effects = 0;
    pth->flags = TO_PTH_CLAMPED(dameth) | (hasalpha*PTH_HASALPHA) | npoty;
    pth->hicr = NULL;

    if (hasfullbright && !fullbrightloadingpass)
    {
        // Load the ONLY texture that'll be assembled with the regular one to
        // make the final texture with fullbright pixels.
        fullbrightloadingpass = 1;

        pth->ofb = (pthtyp *)Xcalloc(1,sizeof(pthtyp));
        pth->flags |= PTH_HASFULLBRIGHT;

        gloadtile_art(dapic, dapal, 0, dameth, pth->ofb, 1);

        fullbrightloadingpass = 0;
    }
}

int32_t gloadtile_hi(int32_t dapic,int32_t dapalnum, int32_t facen, hicreplctyp *hicr,
                            int32_t dameth, pthtyp *pth, int32_t doalloc, char effect)
{
    if (!hicr) return -1;

    coltype *pic = NULL;

    char *picfil = NULL, *fn;
    int32_t picfillen, intexfmt = GL_RGBA, filh;

    int32_t startticks=0, willprint=0;

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

    if (EDUKE32_PREDICT_FALSE((filh = kopen4load(fn, 0)) < 0))
    {
        OSD_Printf("hightile: %s (pic %d) not found\n", fn, dapic);
        return -2;
    }

    picfillen = kfilelength(filh);

    kclose(filh);	// FIXME: shouldn't have to do this. bug in cache1d.c

    char hasalpha = 255;
    texcacheheader cachead;
    int32_t gotcache = texcache_readtexheader(fn, picfillen+(dapalnum<<8), dameth, effect, &cachead, 0);
    vec2_t siz ={ 0, 0 }, tsiz;

    if (gotcache && !texcache_loadtile(&cachead, &doalloc, pth))
    {
        tsiz.x = cachead.xdim;
        tsiz.y = cachead.ydim;
        hasalpha = (cachead.flags & CACHEAD_HASALPHA) ? 0 : 255;
    }
    else
    {
        int32_t r, g, b;
        int32_t j, y;

        gotcache = 0;	// the compressed version will be saved to disk

        if ((filh = kopen4load(fn, 0)) < 0) return -1;

        picfil = (char *)Xmalloc(picfillen+1);
        if (EDUKE32_PREDICT_FALSE(kread(filh, picfil, picfillen) != picfillen))
            initprintf("warning: didn't fully read %s\n", fn);
        // prevent
        // Conditional jump or move depends on uninitialised value(s)
        //  at kpegrend (kplib.c:1655)
        picfil[picfillen] = 0;
        kclose(filh);

        // tsizx/y = replacement texture's natural size
        // xsiz/y = 2^x size of replacement

        kpgetdim(picfil,picfillen,&tsiz.x,&tsiz.y);
        if (tsiz.x == 0 || tsiz.y == 0) { Bfree(picfil); return -1; }

        pth->siz.x = tsiz.x;
        pth->siz.y = tsiz.y;

        if (!glinfo.texnpot)
        {
            for (siz.x=1; siz.x<tsiz.x; siz.x+=siz.x);
            for (siz.y=1; siz.y<tsiz.y; siz.y+=siz.y);
        }
        else
        {
            siz.x = tsiz.x;
            siz.y = tsiz.y;
        }
        pic = (coltype *)Xcalloc(siz.x,siz.y*sizeof(coltype));

        startticks = getticks();

        static coltype *lastpic = NULL;
        static char *lastfn = NULL;
        static int32_t lastsize = 0;

        if (lastpic && lastfn && !Bstrcmp(lastfn,fn))
        {
            willprint=1;
            Bmemcpy(pic, lastpic, siz.x*siz.y*sizeof(coltype));
        }
        else
        {
            if (kprender(picfil,picfillen,(intptr_t)pic,siz.x*sizeof(coltype),siz.x,siz.y)) { Bfree(picfil); Bfree(pic); return -2; }
            willprint=2;

            if (hicprecaching)
            {
                lastfn = fn;  // careful...
                if (!lastpic)
                {
                    lastpic = (coltype *)Bmalloc(siz.x*siz.y*sizeof(coltype));
                    lastsize = siz.x*siz.y;
                }
                else if (lastsize < siz.x*siz.y)
                {
                    Bfree(lastpic);
                    lastpic = (coltype *)Bmalloc(siz.x*siz.y*sizeof(coltype));
                }
                if (lastpic)
                    Bmemcpy(lastpic, pic, siz.x*siz.y*sizeof(coltype));
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
        for (y=0,j=0; y<tsiz.y; y++,j+=siz.x)
        {
            coltype tcol;
            char *cptr = britable[gammabrightness ? 0 : curbrightness];
            coltype *rpptr = &pic[j];

            int32_t x;

            for (x=0; x<tsiz.x; x++)
            {
                tcol.b = cptr[rpptr[x].b];
                tcol.g = cptr[rpptr[x].g];
                tcol.r = cptr[rpptr[x].r];
                tcol.a = rpptr[x].a;
                hasalpha &= rpptr[x].a;

                if (effect & HICTINT_GRAYSCALE)
                {
                    tcol.g = tcol.r = tcol.b = (uint8_t) ((tcol.r * GRAYSCALE_COEFF_RED) + (tcol.g * GRAYSCALE_COEFF_GREEN) +
                                                          (tcol.b * GRAYSCALE_COEFF_BLUE));
                }

                if (effect & HICTINT_INVERT)
                {
                    tcol.b = 255-tcol.b;
                    tcol.g = 255-tcol.g;
                    tcol.r = 255-tcol.r;
                }

                if (effect & HICTINT_COLORIZE)
                {
                    tcol.b = min((int32_t)((tcol.b)*r)/64,255);
                    tcol.g = min((int32_t)((tcol.g)*g)/64,255);
                    tcol.r = min((int32_t)((tcol.r)*b)/64,255);
                }

                rpptr[x] = tcol;
            }
        }

        if ((!(dameth & DAMETH_CLAMPED)) || facen) //Duplicate texture pixels (wrapping tricks for non power of 2 texture sizes)
        {
            if (siz.x > tsiz.x) //Copy left to right
            {
                int32_t *lptr = (int32_t *)pic;
                for (y=0; y<tsiz.y; y++,lptr+=siz.x)
                    Bmemcpy(&lptr[tsiz.x],lptr,(siz.x-tsiz.x)<<2);
            }
            if (siz.y > tsiz.y)  //Copy top to bottom
                Bmemcpy(&pic[siz.x*tsiz.y],pic,(siz.y-tsiz.y)*siz.x<<2);
        }

        int32_t texfmt;

        if (!glinfo.bgra)
        {
            texfmt = GL_RGBA;

            for (j=siz.x*siz.y-1; j>=0; j--)
                swapchar(&pic[j].r, &pic[j].b);
        }
        else texfmt = GL_BGRA;

        Bfree(picfil); picfil = 0;

        if (tsiz.x>>r_downsize <= tilesiz[dapic].x || tsiz.y>>r_downsize <= tilesiz[dapic].y)
            hicr->flags |= (HICR_NOCOMPRESS + HICR_NOSAVE);

        if (glinfo.texcompr && glusetexcompr && !(hicr->flags & HICR_NOSAVE))
            intexfmt = (hasalpha == 255) ? GL_COMPRESSED_RGB_ARB : GL_COMPRESSED_RGBA_ARB;
        else if (hasalpha == 255) intexfmt = GL_RGB;

        if ((doalloc&3)==1)
            bglGenTextures(1, &pth->glpic); //# of textures (make OpenGL allocate structure)
        bglBindTexture(GL_TEXTURE_2D,pth->glpic);

        fixtransparency(pic,tsiz,siz,dameth);
        uploadtexture(doalloc,siz.x,siz.y,intexfmt,texfmt,pic,-1,tsiz.y,
                      dameth | DAMETH_HI | (hicr->flags & HICR_NOCOMPRESS ? DAMETH_NOCOMPRESS : 0));
    }

    // precalculate scaling parameters for replacement
    if (facen > 0)
    {
        pth->scale.x = (float)tsiz.x * (1.0f/64.f);
        pth->scale.y = (float)tsiz.y * (1.0f/64.f);
    }
    else
    {
        pth->scale.x = (float)tsiz.x / (float)tilesiz[dapic].x;
        pth->scale.y = (float)tsiz.y / (float)tilesiz[dapic].y;
    }

#ifdef EDUKE32_TOUCH_DEVICES
    texture_setup(dameth, 5);
#else
    texture_setup(dameth, -1);
#endif

    DO_FREE_AND_NULL(pic);

    if (tsiz.x>>r_downsize <= tilesiz[dapic].x || tsiz.y>>r_downsize <= tilesiz[dapic].y)
        hicr->flags |= HICR_NOCOMPRESS | HICR_NOSAVE;

    pth->picnum = dapic;
    pth->effects = effect;
    pth->flags = TO_PTH_CLAMPED(dameth) | PTH_HIGHTILE | ((facen>0) * PTH_SKYBOX) | ((hasalpha != 255) ? PTH_HASALPHA : 0);
    pth->skyface = facen;
    pth->hicr = hicr;

    if (!gotcache && glinfo.texcompr && glusetexcompr && glusetexcache && !(hicr->flags & HICR_NOSAVE))
    {
        const int32_t nonpow2 = check_nonpow2(siz.x) || check_nonpow2(siz.y);

        // save off the compressed version
        cachead.quality = (hicr->flags & HICR_NOCOMPRESS) ? 0 : r_downsize;
        cachead.xdim = tsiz.x >> cachead.quality;
        cachead.ydim = tsiz.y >> cachead.quality;

        // handle nocompress:
        cachead.flags = nonpow2 * CACHEAD_NONPOW2 | (hasalpha != 255 ? CACHEAD_HASALPHA : 0) |
                        (hicr->flags & HICR_NOCOMPRESS ? CACHEAD_NOCOMPRESS : 0);

        ///            OSD_Printf("Caching \"%s\"\n", fn);
        texcache_writetex(fn, picfillen + (dapalnum << 8), dameth, effect, &cachead);

        if (willprint)
        {
            int32_t etime = getticks() - startticks;
            if (etime >= MIN_CACHETIME_PRINT)
                OSD_Printf("Load tile %4d: p%d-m%d-e%d %s... cached... %d ms\n", dapic, dapalnum, dameth, effect,
                           willprint == 2 ? fn : "", etime);
            willprint = 0;
        }
        else
            OSD_Printf("Cached \"%s\"\n", fn);
    }

    if (willprint)
    {
        int32_t etime = getticks()-startticks;
        if (etime>=MIN_CACHETIME_PRINT)
            OSD_Printf("Load tile %4d: p%d-m%d-e%d %s... %d ms\n", dapic, dapalnum, dameth, effect,
                       willprint==2 ? fn : "", etime);
    }

    return 0;
}

void polymost_setupdetailtexture(const int32_t texunits, const int32_t tex)
{
    bglActiveTextureARB(texunits);

    bglEnable(GL_TEXTURE_2D);
    bglBindTexture(GL_TEXTURE_2D, tex);

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

    bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void polymost_setupglowtexture(const int32_t texunits, const int32_t tex)
{
    bglActiveTextureARB(texunits);

    bglEnable(GL_TEXTURE_2D);
    bglBindTexture(GL_TEXTURE_2D, tex);

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

    bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
#endif

//(dpx,dpy) specifies an n-sided polygon. The polygon must be a convex clockwise loop.
//    n must be <= 8 (assume clipping can double number of vertices)
//method: 0:solid, 1:masked(255 is transparent), 2:transluscent #1, 3:transluscent #2
//    +4 means it's a sprite, so wraparound isn't needed

// drawpoly's hack globals
static int32_t pow2xsplit = 0, skyclamphack = 0;
static float alpha = 0.f;

static inline pthtyp *our_texcache_fetch(int32_t dameth)
{
    // r_usetileshades 1 is TX's method.
    return texcache_fetch(globalpicnum, globalpal, getpalookup((r_usetileshades == 1 && !(globalflags & GLOBAL_NO_GL_TILESHADES)) ? globvis>>3 : 0, globalshade), dameth);
}

static void drawpoly(vec2f_t *dpxy, int32_t n, int32_t method)
{
    const int32_t method_ = method;
    int32_t i, j, k;
    vec2_t tsiz;
    float f, r, ox, oy, oz, ox2, oy2, oz2, dd[16], uu[16], vv[16], px[16], py[16];

#ifdef YAX_ENABLE
    if (g_nodraw) return;
#endif

    if (method == -1 || (uint32_t)globalpicnum >= MAXTILES) return;

    if (n == 3)
    {
        if ((dpxy[0].x-dpxy[1].x) * (dpxy[2].y-dpxy[1].y) >=
            (dpxy[2].x-dpxy[1].x) * (dpxy[0].y-dpxy[1].y)) return; //for triangle
    }
    else
    {
        f = 0; //f is area of polygon / 2
        for (i=n-2,j=n-1,k=0; k<n; i=j,j=k,k++)
            f += (dpxy[i].x-dpxy[k].x)*dpxy[j].y;
        if (f <= 0) return;
    }

    //Load texture (globalpicnum)
    setgotpic(globalpicnum);
    tsiz = tilesiz[globalpicnum];

    if (palookup[globalpal] == NULL)
        globalpal = 0;
    if (!waloff[globalpicnum])
    {
        loadtile(globalpicnum);
        if (!waloff[globalpicnum])
        {
            if (getrendermode() < REND_POLYMOST) return;
            tsiz.x = tsiz.y = 1;
            method = 1; //Hack to update Z-buffer for invalid mirror textures
        }
    }

    j = 0;

    for (i=0; i<n; i++)
    {
        ox = dpxy[i].x-ghalfx;
        oy = dpxy[i].y-ghoriz;
        oz = ghalfx;

        //Up/down rotation
        ox2 = ox;
        oy2 = oy*gchang - oz*gshang;
        oz2 = oy*gshang + oz*gchang;

        //Tilt rotation
        ox = ox2*gctang - oy2*gstang;
        oy = ox2*gstang + oy2*gctang;
        oz = oz2;

        r = ghalfx / oz;

        dd[j] = (dpxy[i].x*gdx + dpxy[i].y*gdy + gdo)*r;
        uu[j] = (dpxy[i].x*gux + dpxy[i].y*guy + guo)*r;
        vv[j] = (dpxy[i].x*gvx + dpxy[i].y*gvy + gvo)*r;

        px[j] = ox*r + ghalfx;
        py[j] = oy*r + ghoriz;
        if ((!j) || (px[j] != px[j-1]) || (py[j] != py[j-1])) j++;
    }

    while ((j >= 3) && (px[j-1] == px[0]) && (py[j-1] == py[0])) j--;
    if (j < 3) return;

#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST)
    {
        float hackscx = 1.f, hackscy = 1.f;
        int32_t texunits = GL_TEXTURE0_ARB;
        int32_t xx, yy;

        int32_t jj = j;

        if (skyclamphack) method |= DAMETH_CLAMPED;
        pthtyp *pth = our_texcache_fetch(method&(~3));

        if (!pth)
        {
            if (editstatus)
            {
                Bsprintf(ptempbuf, "pth==NULL! (bad pal?) pic=%d pal=%d", globalpicnum, globalpal);
                polymost_printext256(8,8, editorcolors[15],editorcolors[5], ptempbuf, 0);
            }
            return;
        }

        if (pth->flags & PTH_HASFULLBRIGHT && r_fullbrights && indrawroomsandmasks)
        {
            if (!fullbrightdrawingpass)
                fullbrightdrawingpass = 1;
            else if (fullbrightdrawingpass == 2)
                pth = pth->ofb;
        }

        // If we aren't rendmode 3, we're in Polymer, which means this code is
        // used for rotatesprite only. Polymer handles all the material stuff,
        // just submit the geometry and don't mess with textures.
        if (getrendermode() == REND_POLYMOST)
        {
            bglBindTexture(GL_TEXTURE_2D, pth ? pth->glpic : 0);

            if (srepeat)
                bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
            if (trepeat)
                bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        }

        // texture scale by parkar request
        if (pth && pth->hicr && !drawingskybox && ((pth->hicr->scale.x != 1.0f) || (pth->hicr->scale.y != 1.0f)))
        {
            bglMatrixMode(GL_TEXTURE);
            bglLoadIdentity();
            bglScalef(pth->hicr->scale.x, pth->hicr->scale.y, 1.0f);
            bglMatrixMode(GL_MODELVIEW);
        }

#ifndef EDUKE32_GLES
        // detail texture
        pthtyp *detailpth = NULL;
        if (r_detailmapping && usehightile && !drawingskybox && hicfindsubst(globalpicnum, DETAILPAL))
            detailpth = texcache_fetch(globalpicnum, DETAILPAL, 0, method&(~3));

        if (detailpth && detailpth->hicr && (detailpth->hicr->palnum == DETAILPAL))
        {
            polymost_setupdetailtexture(++texunits, detailpth ? detailpth->glpic : 0);

            f = detailpth ? detailpth->hicr->scale.x : 1.f;

            bglMatrixMode(GL_TEXTURE);
            bglLoadIdentity();

            if (pth && pth->hicr && ((pth->hicr->scale.x != 1.0f) || (pth->hicr->scale.y != 1.0f)))
                bglScalef(pth->hicr->scale.x, pth->hicr->scale.y, 1.0f);

            if (detailpth && detailpth->hicr && ((detailpth->hicr->scale.x != 1.0f) || (detailpth->hicr->scale.y != 1.0f)))
                bglScalef(detailpth->hicr->scale.x, detailpth->hicr->scale.y, 1.0f);

            bglMatrixMode(GL_MODELVIEW);
        }

        // glow texture
        pthtyp *glowpth = NULL;
        if (r_glowmapping && usehightile && !drawingskybox && hicfindsubst(globalpicnum, GLOWPAL))
            glowpth = texcache_fetch(globalpicnum, GLOWPAL, 0, method&(~3));

        if (glowpth && glowpth->hicr && (glowpth->hicr->palnum == GLOWPAL))
            polymost_setupglowtexture(++texunits, glowpth ? glowpth->glpic : 0);
#endif

        if (pth && (pth->flags & PTH_HIGHTILE))
        {
            hackscx = pth->scale.x;
            hackscy = pth->scale.y;
            tsiz.x = pth->siz.x;
            tsiz.y = pth->siz.y;
        }

        xx = tsiz.x;
        yy = tsiz.y;

        if (!glinfo.texnpot)
        {
            for (xx=1; xx<tsiz.x; xx+=xx)
            {
                /* do nothing */
            }
            for (yy=1; yy<tsiz.y; yy+=yy)
            {
                /* do nothing */
            }
        }

        ox2 = 1.0f/xx;
        oy2 = 1.0f/yy;

        if ((!(method&3)) && (!fullbrightdrawingpass))
        {
            bglDisable(GL_BLEND);
            bglDisable(GL_ALPHA_TEST);
        }
        else
        {
            float al = 0.f; // PLAG : default alphacut was 0.32 before goodalpha
            if (pth && pth->hicr && pth->hicr->alphacut >= 0.f) al = pth->hicr->alphacut;
            if (alphahackarray[globalpicnum])
                al=alphahackarray[globalpicnum];
            if (!waloff[globalpicnum]) al = 0.f;	// invalid textures ignore the alpha cutoff settings
            bglEnable(GL_BLEND);
            bglEnable(GL_ALPHA_TEST);
            bglAlphaFunc(GL_GREATER,al);
        }

        {
            float pc[4];

#ifdef POLYMER
            if (getrendermode() == REND_POLYMER && pr_artmapping && !(globalflags & GLOBAL_NO_GL_TILESHADES) && polymer_eligible_for_artmap(globalpicnum, pth))
                pc[0] = pc[1] = pc[2] = 1.0f;
            else
#endif
                pc[0] = pc[1] = pc[2] = getshadefactor(globalshade);

            // spriteext full alpha control
            pc[3] = float_trans[method&3] * (1.f - alpha);

            // tinting happens only to hightile textures, and only if the texture we're
            // rendering isn't for the same palette as what we asked for
            if (!(hictinting[globalpal].f & HICTINT_COLORIZE))
            {
                if (pth && (pth->flags & PTH_HIGHTILE))
                {
                    if (pth->palnum != globalpal || (hictinting[globalpal].f & HICTINT_APPLYOVERALTPAL))
                        hictinting_apply(pc, globalpal);

                    if (have_basepal_tint())
                        hictinting_apply(pc, MAXPALOOKUPS-1);
                }
                // hack: this is for drawing the 8-bit crosshair recolored in polymost
                else if (hictinting[globalpal].f & HICTINT_USEONART)
                    hictinting_apply(pc, globalpal);
            }

            bglColor4f(pc[0],pc[1],pc[2],pc[3]);
        }

        //Hack for walls&masked walls which use textures that are not a power of 2
        if ((pow2xsplit) && (tsiz.x != xx))
        {
            int32_t nn, ix0, ix1;

            float ngdx = 0.f, ngdy = 0.f, ngdo = 0.f, ngux = 0.f, nguy = 0.f, nguo = 0.f, uoffs;
            float ngvx = 0.f, ngvy = 0.f, ngvo = 0.f, dp, up, vp, du0 = 0.f, du1 = 0.f, dui, duj;

            ox = py[1]-py[2]; oy = py[2]-py[0]; oz = py[0]-py[1];
            r = 1.f / (ox*px[0] + oy*px[1] + oz*px[2]);
            ngdx = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
            ngux = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
            ngvx = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
            ox = px[2]-px[1]; oy = px[0]-px[2]; oz = px[1]-px[0];
            ngdy = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
            nguy = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
            ngvy = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
            ox = px[0]-0.5f; oy = py[0]-0.5f; //.5 centers texture nicely
            ngdo = dd[0] - ox*ngdx - oy*ngdy;
            nguo = uu[0] - ox*ngux - oy*nguy;
            ngvo = vv[0] - ox*ngvx - oy*ngvy;

            ngux *= hackscx; nguy *= hackscx; nguo *= hackscx;
            ngvx *= hackscy; ngvy *= hackscy; ngvo *= hackscy;
            uoffs = ((float)(xx-tsiz.x)*0.5f);
            ngux -= ngdx*uoffs;
            nguy -= ngdy*uoffs;
            nguo -= ngdo*uoffs;

            //Find min&max u coordinates (du0...du1)
            for (i=0; i<jj; i++)
            {
                ox = px[i]; oy = py[i];
                f = (ox*ngux + oy*nguy + nguo) / (ox*ngdx + oy*ngdy + ngdo);
                if (!i) { du0 = du1 = f; continue; }
                if (f < du0) du0 = f;
                else if (f > du1) du1 = f;
            }

            f = 1.0f/tsiz.x;
            ix0 = Blrintf(floorf(du0*f));
            ix1 = Blrintf(floorf(du1*f));
            for (; ix0<=ix1; ix0++)
            {
                du0 = (float)(ix0*tsiz.x);   // + uoffs;
                du1 = (float)((ix0+1)*tsiz.x); // + uoffs;

                i = 0; nn = 0;
                duj = (px[i]*ngux + py[i]*nguy + nguo) / (px[i]*ngdx + py[i]*ngdy + ngdo);
                do
                {
                    j = i+1; if (j == jj) j = 0;

                    dui = duj;
                    duj = (px[j]*ngux + py[j]*nguy + nguo) / (px[j]*ngdx + py[j]*ngdy + ngdo);

                    if ((du0 <= dui) && (dui <= du1))
                    {
                        uu[nn] = px[i];
                        vv[nn] = py[i];
                        nn++;
                    }

                    //ox*(ngux-ngdx*du1) + oy*(nguy-ngdy*du1) + (nguo-ngdo*du1) = 0
                    //(px[j]-px[i])*f + px[i] = ox
                    //(py[j]-py[i])*f + py[i] = oy

                    ///Solve for f
                    //((px[j]-px[i])*f + px[i])*(ngux-ngdx*du1) +
                    //((py[j]-py[i])*f + py[i])*(nguy-ngdy*du1) + (nguo-ngdo*du1) = 0

#define DRAWPOLY_MATH_BULLSHIT(X) do { f = -(px[i] *(ngux-ngdx*X) + py[i] *(nguy-ngdy*X) + (nguo-ngdo*X)) / ((px[j]-px[i])*(ngux-ngdx*X) + (py[j]-py[i])*(nguy-ngdy*X)); \
                                        uu[nn] = (px[j]-px[i])*f + px[i]; vv[nn] = (py[j]-py[i])*f + py[i]; nn++; } while (0)

                    if (duj <= dui)
                    {
                        if ((du1 < duj) != (du1 < dui))
                            DRAWPOLY_MATH_BULLSHIT(du1);
                        if ((du0 < duj) != (du0 < dui))
                            DRAWPOLY_MATH_BULLSHIT(du0);
                    }
                    else
                    {
                        if ((du0 < duj) != (du0 < dui))
                            DRAWPOLY_MATH_BULLSHIT(du0);
                        if ((du1 < duj) != (du1 < dui))
                            DRAWPOLY_MATH_BULLSHIT(du1);
                    }
                    i = j;

#undef DRAWPOLY_MATH_BULLSHIT

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
                    r = 1.f/dp;
                    if (texunits > GL_TEXTURE0_ARB)
                    {
                        j = GL_TEXTURE0_ARB;
                        while (j <= texunits)
                            bglMultiTexCoord2fARB(j++, (up*r-du0+uoffs)*ox2,vp*r*oy2);
                    }
                    else
                        bglTexCoord2f((up*r-du0+uoffs)*ox2,vp*r*oy2);
                    bglVertex3f((ox-ghalfx)*r*grhalfxdown10x,(ghoriz-oy)*r*grhalfxdown10,r*(1.f/1024.f));
                }
                bglEnd();
            }
        }
        else
        {
            ox2 *= hackscx; oy2 *= hackscy;
            bglBegin(GL_TRIANGLE_FAN);
            for (i=0; i<jj; i++)
            {
                r = 1.f/dd[i];
                if (texunits > GL_TEXTURE0_ARB)
                {
                    j = GL_TEXTURE0_ARB;
                    while (j <= texunits)
                        bglMultiTexCoord2fARB(j++, uu[i]*r*ox2,vv[i]*r*oy2);
                }
                else
                    bglTexCoord2f(uu[i]*r*ox2,vv[i]*r*oy2);
                bglVertex3f((px[i]-ghalfx)*r*grhalfxdown10x,(ghoriz-py[i])*r*grhalfxdown10,r*(1.f/1024.f));
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

        if (getrendermode() == REND_POLYMOST)
        {
            if (srepeat)
                bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
            if (trepeat)
                bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
        }

        if (fullbrightdrawingpass == 1) // tile has fullbright colors ?
        {
            int32_t shadeforfullbrightpass = globalshade; // save the current shade
            fullbrightdrawingpass = 2;
            globalshade = -128; // fullbright
            bglDisable(GL_FOG);
            drawpoly(dpxy, n, method_); // draw them afterwards, then. :)
            if (!nofog) bglEnable(GL_FOG);
            globalshade = shadeforfullbrightpass;
            fullbrightdrawingpass = 0;
        }
        return;
    }
#endif
}


static inline void vsp_finalize_init(vsptyp *vsp, int32_t vcnt)
{
    int32_t i;

    for (i=0; i<vcnt; i++)
    {
        vsp[i].cy[1] = vsp[i+1].cy[0]; vsp[i].ctag = i;
        vsp[i].fy[1] = vsp[i+1].fy[0]; vsp[i].ftag = i;
        vsp[i].n = i+1; vsp[i].p = i-1;
//        vsp[i].tag = -1;
    }
    vsp[vcnt-1].n = 0; vsp[0].p = vcnt-1;

    //VSPMAX-1 is dummy empty node
    for (i=vcnt; i<VSPMAX; i++) { vsp[i].n = i+1; vsp[i].p = i-1; }
    vsp[VSPMAX-1].n = vcnt; vsp[vcnt].p = VSPMAX-1;
}

static inline void vsdel(vsptyp *vsp, int32_t i)
{
    //Delete i
    vsp[vsp[i].n].p = vsp[i].p;
    vsp[vsp[i].p].n = vsp[i].n;

    //Add i to empty list
    vsp[i].n = vsp[VSPMAX-1].n;
    vsp[i].p = VSPMAX-1;
    vsp[vsp[VSPMAX-1].n].p = i;
    vsp[VSPMAX-1].n = i;
}

static inline int32_t vsinsaft(vsptyp *vsp, int32_t i)
{
    //i = next element from empty list
    int32_t r = vsp[VSPMAX-1].n;
    vsp[vsp[r].n].p = VSPMAX-1;
    vsp[VSPMAX-1].n = vsp[r].n;

    vsp[r] = vsp[i]; //copy i to r

    //insert r after i
    vsp[r].p = i; vsp[r].n = vsp[i].n;
    vsp[vsp[i].n].p = r; vsp[i].n = r;

    return(r);
}

static int32_t domostpolymethod = 0;

void domost(float x0, float y0, float x1, float y1)
{
    vec2f_t dpxy[4], n0, n1;
    float slop, dx;
    float spx[4], /*spy[4],*/ cy[2], cv[2];
    int32_t i, k, z, vcnt = 0, scnt, newi, dir = 0, spt[4];

    alpha = 0.f;

    if (x0 < x1)
    {
        dir = 1; //clip dmost (floor)
        y0 -= .01f;
        y1 -= .01f;
    }
    else
    {
        if (x0 == x1) return;
        swapfloat(&x0, &x1);
        swapfloat(&y0, &y1);
        //dir = 0; //clip umost (ceiling)
        //y0 += .01; y1 += .01; //necessary?
    }

    slop = (y1-y0)/(x1-x0);
    for (i=vsp[0].n; i; i=newi)
    {
        newi = vsp[i].n; n0.x = vsp[i].x; n1.x = vsp[newi].x;
        if ((x0 >= n1.x) || (n0.x >= x1) || (vsp[i].ctag <= 0)) continue;
        dx = n1.x-n0.x;
        cy[0] = vsp[i].cy[0]; cv[0] = vsp[i].cy[1]-cy[0];
        cy[1] = vsp[i].fy[0]; cv[1] = vsp[i].fy[1]-cy[1];

        scnt = 0;

        //Test if left edge requires split (x0,y0) (nx0,cy(0)),<dx,cv(0)>
        if ((x0 > n0.x) && (x0 < n1.x))
        {
            const float t = (x0-n0.x)*cv[dir] - (y0-cy[dir])*dx;
            if (((!dir) && (t < 0.f)) || ((dir) && (t > 0.f)))
                { spx[scnt] = x0; /*spy[scnt] = y0;*/ spt[scnt] = -1; scnt++; }
        }

        //Test for intersection on umost (0) and dmost (1)

        {
            const float d[2] ={
                ((y0-y1) * dx) - ((x0-x1) * cv[0]),
                ((y0-y1) * dx) - ((x0-x1) * cv[1])
            };

            const float n[2] ={
                ((y0-cy[0]) * dx) - ((x0-n0.x) * cv[0]),
                ((y0-cy[1]) * dx) - ((x0-n0.x) * cv[1])
            };

            const float fnx[2] ={
                x0 + ((n[0]/d[0]) * (x1-x0)),
                x0 + ((n[1]/d[1]) * (x1-x0))
            };

            if ((Bfabsf(d[0]) > Bfabsf(n[0])) && (d[0] * n[0] >= 0.f) && (fnx[0] > n0.x) && (fnx[0] < n1.x))
                spx[scnt] = fnx[0], spt[scnt++] = 0;

            if ((Bfabsf(d[1]) > Bfabsf(n[1])) && (d[1] * n[1] >= 0.f) && (fnx[1] > n0.x) && (fnx[1] < n1.x))
                spx[scnt] = fnx[1], spt[scnt++] = 1;
        }

        //Nice hack to avoid full sort later :)
        if ((scnt >= 2) && (spx[scnt-1] < spx[scnt-2]))
        {
            swapfloat(&spx[scnt-1], &spx[scnt-2]);
            swaplong(&spt[scnt-1], &spt[scnt-2]);
            /* f = spy[scnt-1]; spy[scnt-1] = spy[scnt-2]; spy[scnt-2] = f; */
        }

        //Test if right edge requires split
        if ((x1 > n0.x) && (x1 < n1.x))
        {
            const float t = (x1-n0.x)*cv[dir] - (y1-cy[dir])*dx;
            if (((!dir) && (t < 0)) || ((dir) && (t > 0)))
                { spx[scnt] = x1; /* spy[scnt] = y1; */ spt[scnt] = -1; scnt++; }
        }

        vsp[i].tag = vsp[newi].tag = -1;

        dx = 1.f/dx;

        for (z=0; z<=scnt; z++,i=vcnt)
        {
            float t;

            if (z == scnt)
                goto skip;

            t = (spx[z]-n0.x)*dx;
            vcnt = vsinsaft(vsp, i);
            vsp[i].cy[1] = t*cv[0] + cy[0];
            vsp[i].fy[1] = t*cv[1] + cy[1];
            vsp[vcnt].x = spx[z];
            vsp[vcnt].cy[0] = vsp[i].cy[1];
            vsp[vcnt].fy[0] = vsp[i].fy[1];
            vsp[vcnt].tag = spt[z];

skip: ;
            int32_t ni = vsp[i].n; if (!ni) continue; //this 'if' fixes many bugs!
            float dx0 = vsp[i].x; if (x0 > dx0) continue;
            float dx1 = vsp[ni].x; if (x1 < dx1) continue;
            n0.y = (dx0-x0)*slop + y0;
            n1.y = (dx1-x0)*slop + y0;

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
            if ((vsp[i].tag == 0) || (n0.y <= vsp[i].cy[0]+.01f)) k--;
            if ((vsp[i].tag == 1) || (n0.y >= vsp[i].fy[0]-.01f)) k++;
            if ((vsp[ni].tag == 0) || (n1.y <= vsp[i].cy[1]+.01f)) k -= 3;
            if ((vsp[ni].tag == 1) || (n1.y >= vsp[i].fy[1]-.01f)) k += 3;

            dpxy[0].x = dx0;
            dpxy[1].x = dx1;

            if (!dir)
            {
                dpxy[0].y = vsp[i].cy[0];
                dpxy[1].y = vsp[i].cy[1];

                switch (k)
                {
                case 4:
                case 5:
                case 7:
                    dpxy[2].x = dx1; dpxy[3].x = dx0;
                    dpxy[2].y = n1.y; dpxy[3].y = n0.y;
                    vsp[i].cy[0] = n0.y; vsp[i].cy[1] = n1.y; vsp[i].ctag = gtag;
                    drawpoly(dpxy, 4, domostpolymethod);
                    break;
                case 1:
                case 2:
                    dpxy[2].x = dx0;
                    dpxy[2].y = n0.y;
                    vsp[i].cy[0] = n0.y; vsp[i].ctag = gtag;
                    drawpoly(dpxy, 3, domostpolymethod);
                    break;
                case 3:
                case 6:
                    dpxy[2].x = dx1;
                    dpxy[2].y = n1.y;
                    vsp[i].cy[1] = n1.y; vsp[i].ctag = gtag;
                    drawpoly(dpxy, 3, domostpolymethod);
                    break;
                case 8:
                    dpxy[2].x = dx1; dpxy[2].y = vsp[i].fy[1];
                    dpxy[3].x = dx0; dpxy[3].y = vsp[i].fy[0];
                    vsp[i].ctag = vsp[i].ftag = -1;
                    drawpoly(dpxy, 4, domostpolymethod);
                default:
                    break;
                }
            }
            else
            {
                switch (k)
                {
                case 4:
                case 3:
                case 1:
                    dpxy[2].x = dx1; dpxy[3].x = dx0;
                    dpxy[0].y = n0.y; dpxy[1].y = n1.y; dpxy[2].y = vsp[i].fy[1]; dpxy[3].y = vsp[i].fy[0];
                    vsp[i].fy[0] = n0.y; vsp[i].fy[1] = n1.y; vsp[i].ftag = gtag;
                    drawpoly(dpxy, 4, domostpolymethod);
                    break;
                case 7:
                case 6:
                    dpxy[2].x = dx0;
                    dpxy[0].y = n0.y; dpxy[1].y = vsp[i].fy[1]; dpxy[2].y = vsp[i].fy[0];
                    vsp[i].fy[0] = n0.y; vsp[i].ftag = gtag;
                    drawpoly(dpxy, 3, domostpolymethod);
                    break;
                case 5:
                case 2:
                    dpxy[2].x = dx1;
                    dpxy[0].y = vsp[i].fy[0]; dpxy[1].y = n1.y; dpxy[2].y = vsp[i].fy[1]; 
                    vsp[i].fy[1] = n1.y; vsp[i].ftag = gtag;
                    drawpoly(dpxy, 3, domostpolymethod);
                    break;
                case 0:
                    dpxy[2].x = dx1; dpxy[3].x = dx0;
                    dpxy[0].y = vsp[i].cy[0]; dpxy[1].y = vsp[i].cy[1]; dpxy[2].y = vsp[i].fy[1]; dpxy[3].y = vsp[i].fy[0];
                    vsp[i].ctag = vsp[i].ftag = -1;
                    drawpoly(dpxy, 4, domostpolymethod);
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
        int32_t ni = vsp[i].n;

        if ((vsp[i].cy[0] >= vsp[i].fy[0]) && (vsp[i].cy[1] >= vsp[i].fy[1]))
            vsp[i].ctag = vsp[i].ftag = -1;

        if ((vsp[i].ctag == vsp[ni].ctag) && (vsp[i].ftag == vsp[ni].ftag))
        {
            vsp[i].cy[1] = vsp[ni].cy[1];
            vsp[i].fy[1] = vsp[ni].fy[1];
            vsdel(vsp, ni);
        }
        else i = ni;
    }
}

void polymost_editorfunc(void)
{
    vec3_t v;
    vec3f_t o, o2;
    int32_t cz, fz;
    hitdata_t hit;
    vec3_t vect;
    const float ratio = get_projhack_ratio();

    o2.x = (searchx-ghalfx)/ratio;
    o2.y = (searchy-ghoriz)/ratio;  // ghoriz is (ydimen>>1) here
    o2.z = ghalfx;

    //Tilt rotation
    o.x = o2.x*gctang + o2.y*gstang;
    o.y = o2.y*gctang - o2.x*gstang;
    o.z = o2.z;

    //Up/down rotation
    o2.x = o.z*gchang - o.y*gshang;
    o2.y = o.x;
    o2.z = o.y*gchang + o.z*gshang;

    //Standard Left/right rotation
    v.x = Blrintf (o2.x*fcosglobalang - o2.y*fsinglobalang);
    v.y = Blrintf (o2.x*fsinglobalang + o2.y*fcosglobalang);
    v.z = Blrintf (o2.z*16384.f);

    vect.x = globalposx;
    vect.y = globalposy;
    vect.z = globalposz;

    hitallsprites = 1;
    hitscan((const vec3_t *) &vect, globalcursectnum, //Start position
        v.x>>10, v.y>>10, v.z>>6, &hit, 0xffff0030);

    if (hit.sect != -1) // if hitsect is -1, hitscan overflowed somewhere
    {
        getzsofslope(hit.sect, hit.pos.x, hit.pos.y, &cz, &fz);
        hitallsprites = 0;

        searchsector = hit.sect;
        if (hit.pos.z<cz) searchstat = 1;
        else if (hit.pos.z>fz) searchstat = 2;
        else if (hit.wall >= 0)
        {
            searchbottomwall = searchwall = hit.wall; searchstat = 0;
            if (wall[hit.wall].nextwall >= 0)
            {
                int32_t cz, fz;
                getzsofslope(wall[hit.wall].nextsector, hit.pos.x, hit.pos.y, &cz, &fz);
                if (hit.pos.z > fz)
                {
                    searchisbottom = 1;
                    if (wall[hit.wall].cstat&2) //'2' bottoms of walls
                        searchbottomwall = wall[hit.wall].nextwall;
                }
                else
                {
                    searchisbottom = 0;
                    if ((hit.pos.z > cz) && (wall[hit.wall].cstat&(16+32))) //masking or 1-way
                        searchstat = 4;
                }
            }
        }
        else if (hit.sprite >= 0) { searchwall = hit.sprite; searchstat = 3; }
        else
        {
            int32_t cz, fz;
            getzsofslope(hit.sect, hit.pos.x, hit.pos.y, &cz, &fz);
            if ((hit.pos.z<<1) < cz+fz) searchstat = 1; else searchstat = 2;
            //if (vz < 0) searchstat = 1; else searchstat = 2; //Won't work for slopes :/
        }

        if (preview_mouseaim && spritesortcnt < MAXSPRITESONSCREEN)
        {
            tspritetype *tsp = &tsprite[spritesortcnt];
            double dadist, x, y, z;
            Bmemcpy(tsp, &hit.pos, sizeof(vec3_t));
            x = tsp->x-globalposx; y=tsp->y-globalposy; z=(tsp->z-globalposz)/16.0;
            dadist = Bsqrt(x*x + y*y + z*z);
            tsp->sectnum = hit.sect;
            tsp->picnum = 2523;  // CROSSHAIR
            tsp->cstat = 128;
            tsp->owner = MAXSPRITES-1;
            tsp->xrepeat = tsp->yrepeat = min(max(1, (int32_t) (dadist*48.0/3200.0)), 255);
            sprite[tsp->owner].xoffset = sprite[tsp->owner].yoffset = 0;
            tspriteptr[spritesortcnt++] = tsp;
        }

        if ((searchstat==1 || searchstat==2) && searchsector>=0)
        {
            int32_t scrv[2] ={ (v.x>>12), (v.y>>12) };
            int32_t scrv_r[2] ={ scrv[1], -scrv[0] };
            walltype *wal = &wall[sector[searchsector].wallptr];
            uint64_t wdistsq, bestwdistsq=0x7fffffff;
            int32_t k, bestk=-1;

            for (k=0; k<sector[searchsector].wallnum; k++)
            {
                int32_t w1[2] ={ wal[k].x, wal[k].y };
                int32_t w2[2] ={ wall[wal[k].point2].x, wall[wal[k].point2].y };
                int32_t w21[2] ={ w1[0]-w2[0], w1[1]-w2[1] };
                int32_t pw1[2] ={ w1[0]-hit.pos.x, w1[1]-hit.pos.y };
                int32_t pw2[2] ={ w2[0]-hit.pos.x, w2[1]-hit.pos.y };
                float w1d = (float) (scrv_r[0]*pw1[0] + scrv_r[1]*pw1[1]);
                float w2d = (float) (scrv_r[0]*pw2[0] + scrv_r[1]*pw2[1]);
                int32_t ptonline[2], scrp[2];
                int64_t t1, t2;

                w2d = -w2d;
                if ((w1d==0 && w2d==0) || (w1d<0 || w2d<0))
                    continue;
                ptonline[0] = (int32_t) (w2[0]+(w2d/(w1d+w2d))*w21[0]);
                ptonline[1] = (int32_t) (w2[1]+(w2d/(w1d+w2d))*w21[1]);
                scrp[0] = ptonline[0]-vect.x;
                scrp[1] = ptonline[1]-vect.y;
                if (scrv[0]*scrp[0] + scrv[1]*scrp[1] <= 0)
                    continue;
                t1=scrp[0]; t2=scrp[1];
                wdistsq = t1*t1 + t2*t2;
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

void polymost_scansector(int32_t sectnum);

// variables that are set to ceiling- or floor-members, depending
// on which one is processed right now
static int32_t global_cf_z;
static float global_cf_xpanning, global_cf_ypanning, global_cf_heinum;
static int32_t global_cf_shade, global_cf_pal, global_cf_fogpal;
static int32_t (*global_getzofslope_func)(int16_t, int32_t, int32_t);

static void polymost_internal_nonparallaxed(float nx0, float ny0, float nx1, float ny1, float ryp0, float ryp1,
                                            float x0, float x1, float cf_y0, float cf_y1, int32_t have_floor,
                                            int32_t sectnum)
{
    float ft[4], fx, fy, ox, oy, oz, ox2, oy2;
    int32_t i;

    const sectortype *sec = &sector[sectnum];

    // comments from floor code:
            //(singlobalang/-16384*(sx-ghalfx) + 0*(sy-ghoriz) + (cosviewingrangeglobalang/16384)*ghalfx)*d + globalposx    = u*16
            //(cosglobalang/ 16384*(sx-ghalfx) + 0*(sy-ghoriz) + (sinviewingrangeglobalang/16384)*ghalfx)*d + globalposy    = v*16
            //(                  0*(sx-ghalfx) + 1*(sy-ghoriz) + (                             0)*ghalfx)*d + globalposz/16 = (sec->floorz/16)
    if (!(globalorientation&64))
        { ft[0] = fglobalposx; ft[1] = fglobalposy; ft[2] = fcosglobalang; ft[3] = fsinglobalang; }
    else
    {
        float r;
        //relative alignment
        fx = (float)(wall[wall[sec->wallptr].point2].x-wall[sec->wallptr].x);
        fy = (float)(wall[wall[sec->wallptr].point2].y-wall[sec->wallptr].y);
        r = polymost_invsqrt(fx*fx+fy*fy); fx *= r; fy *= r;
        ft[2] = fcosglobalang*fx + fsinglobalang*fy;
        ft[3] = fsinglobalang*fx - fcosglobalang*fy;
        ft[0] = ((float)(globalposx-wall[sec->wallptr].x))*fx + ((float)(globalposy-wall[sec->wallptr].y))*fy;
        ft[1] = ((float)(globalposy-wall[sec->wallptr].y))*fx - ((float)(globalposx-wall[sec->wallptr].x))*fy;
        if (!(globalorientation&4)) globalorientation ^= 32; else globalorientation ^= 16;
    }
    gdx = 0;
    gdy = gxyaspect;
    if (!(globalorientation&2))
        if (global_cf_z-globalposz)  // PK 2012: don't allow div by zero
            gdy /= (double)(global_cf_z-globalposz);
    gdo = -ghoriz*gdy;
    if (globalorientation&8) { ft[0] *= (1.f/8.f); ft[1] *= (-1.f/8.f); ft[2] *= (1.f/2097152.f); ft[3] *= (1.0f/2097152.f); }
    else { ft[0] *= (1.0f/16.f); ft[1] *= (-1.0f/16.f); ft[2] *= (1.0f/4194304.f); ft[3] *= (1.0f/4194304.f); }
    gux = ft[3]*((double)viewingrange)*(-1.0/65536.0);
    gvx = ft[2]*((double)viewingrange)*(-1.0/65536.0);
    guy = ft[0]*gdy; gvy = ft[1]*gdy;
    guo = ft[0]*gdo; gvo = ft[1]*gdo;
    guo += (ft[2]-gux)*ghalfx;
    gvo -= (ft[3]+gvx)*ghalfx;

    //Texture flipping
    if (globalorientation&4)
    {
        double r;

        r = gux; gux = gvx; gvx = r;
        r = guy; guy = gvy; gvy = r;
        r = guo; guo = gvo; gvo = r;
    }
    if (globalorientation&16) { gux = -gux; guy = -guy; guo = -guo; }
    if (globalorientation&32) { gvx = -gvx; gvy = -gvy; gvo = -gvo; }

    //Texture panning
    fx = global_cf_xpanning*((float)(1<<(picsiz[globalpicnum]&15)))*(1.0f/256.f);
    fy = global_cf_ypanning*((float)(1<<(picsiz[globalpicnum]>>4)))*(1.0f/256.f);
    if ((globalorientation&(2+64)) == (2+64)) //Hack for panning for slopes w/ relative alignment
    {
        float r = global_cf_heinum * (1.0f/4096.f); r = polymost_invsqrt(r*r+1);
        if (!(globalorientation&4)) fy *= r; else fx *= r;
    }
    guy += gdy*fx; guo += gdo*fx;
    gvy += gdy*fy; gvo += gdo*fy;

    if (globalorientation&2) //slopes
    {
        double px[3], py[3];
        float dd[3], uu[3], vv[3];
        float r;

        px[0] = x0; py[0] = ryp0 + (double)ghoriz;
        px[1] = x1; py[1] = ryp1 + (double)ghoriz;

        //Pick some point guaranteed to be not collinear to the 1st two points
        ox = nx0 + (ny1-ny0);
        oy = ny0 + (nx0-nx1);
        ox2 = (oy-fglobalposy)*gcosang  - (ox-fglobalposx)*gsinang ;
        oy2 = (ox-fglobalposx)*gcosang2 + (oy-fglobalposy)*gsinang2;
        oy2 = 1.f/oy2;
        px[2] = ghalfx*ox2*oy2 + ghalfx; oy2 *= gyxscale;
        py[2] = oy2 + (double)ghoriz;

        for (i=0; i<3; i++)
        {
            dd[i] = px[i]*gdx + py[i]*gdy + gdo;
            uu[i] = px[i]*gux + py[i]*guy + guo;
            vv[i] = px[i]*gvx + py[i]*gvy + gvo;
        }

        py[0] = cf_y0;
        py[1] = cf_y1;
        py[2] = (double)((float)(global_getzofslope_func(sectnum,Blrintf(ox),Blrintf(oy))-globalposz)*oy2 + ghoriz);

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
            r = global_cf_heinum * (1.0f / 4096.f);
            r = Bsqrtf(r*r+1);
            if (!(globalorientation&4)) { gvx *= r; gvy *= r; gvo *= r; }
            else { gux *= r; guy *= r; guo *= r; }
        }
    }

    domostpolymethod = (globalorientation>>7)&3;

    pow2xsplit = 0;
    alpha = 0.f;

    if (!nofog) calc_and_apply_fog(globalpicnum, fogpal_shade(sec, global_cf_shade), sec->visibility,
        POLYMOST_CHOOSE_FOG_PAL(global_cf_fogpal, global_cf_pal));

    if (have_floor)
    {
        if (globalposz >= getflorzofslope(sectnum, globalposx, globalposy))
            domostpolymethod = -1; //Back-face culling

        domost(x0, cf_y0, x1, cf_y1); //flor
    }
    else
    {
        if (globalposz <= getceilzofslope(sectnum, globalposx, globalposy))
            domostpolymethod = -1; //Back-face culling

        domost(x1, cf_y1, x0, cf_y0); //ceil
    }

    domostpolymethod = 0;
}

static void calc_ypanning(int32_t refposz, float ryp0, float ryp1,
                          float x0, float x1, uint8_t ypan, uint8_t yrepeat,
                          int32_t dopancor)
{
    int32_t i;
    float t0, t1, t, fy;

    t0 = ((float)(refposz-globalposz))*ryp0 + ghoriz;
    t1 = ((float)(refposz-globalposz))*ryp1 + ghoriz;
    t = ((gdx*x0 + gdo) * (float)yrepeat) / ((x1-x0) * ryp0 * 2048.f);
    i = (1<<(picsiz[globalpicnum]>>4)); if (i < tilesiz[globalpicnum].y) i <<= 1;

#ifdef NEW_MAP_FORMAT
    if (g_loadedMapVersion >= 10)
        i = tilesiz[globalpicnum].y;
    else
#endif
    if (polymost_is_npotmode())
    {
        t *= (float)tilesiz[globalpicnum].y / i;
        i = tilesiz[globalpicnum].y;
    }
    else if (dopancor)
    {
        // Carry out panning "correction" to make it look like classic in some
        // cases, but failing in the general case.
        int32_t yoffs = Blrintf((i-tilesiz[globalpicnum].y)*(255.f/i));

        if (ypan > 256-yoffs)
            ypan -= yoffs;
    }

    fy = (float) (ypan * i) * (1.f/256.f);
    gvx = (t0-t1)*t;
    gvy = (x1-x0)*t;
    gvo = -gvx*x0 - gvy*t0 + fy*gdo; gvx += fy*gdx; gvy += fy*gdy;
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

static void polymost_drawalls(int32_t bunch)
{
    sectortype *sec, *nextsec;
    walltype *wal, *wal2, *nwal;
    float ox, oy, oz, dd[3], vv[3];
    float fx, x0, x1, cy0, cy1, fy0, fy1, xp0, yp0, xp1, yp1, ryp0, ryp1, nx0, ny0, nx1, ny1;
    float t, r, t0, t1, ocy0, ocy1, ofy0, ofy1, oxp0, oyp0, ft[4];
    float oguo, ogux, oguy;
    int32_t i, x, y, z, cz, fz, wallnum, sectnum, nextsectnum;

    int32_t dapskybits;
    const int8_t *dapskyoff;

    alpha = 0.f;

    sectnum = thesector[bunchfirst[bunch]]; sec = &sector[sectnum];

    //DRAW WALLS SECTION!
    for (z=bunchfirst[bunch]; z>=0; z=bunchp2[z])
    {
        wallnum = thewall[z]; wal = &wall[wallnum]; wal2 = &wall[wal->point2];
        nextsectnum = wal->nextsector;
        nextsec = nextsectnum>=0 ? &sector[nextsectnum] : NULL;

#ifdef YAX_ENABLE
        if (yax_nomaskpass==1 && yax_isislandwall(wallnum, !yax_globalcf) && (yax_nomaskdidit=1))
            continue;
#endif

        //Offset&Rotate 3D coordinates to screen 3D space
        x = wal->x-globalposx; y = wal->y-globalposy;
        xp0 = (float)y*gcosang  - (float)x*gsinang;
        yp0 = (float)x*gcosang2 + (float)y*gsinang2;
        x = wal2->x-globalposx; y = wal2->y-globalposy;
        xp1 = (float)y*gcosang  - (float)x*gsinang;
        yp1 = (float)x*gcosang2 + (float)y*gsinang2;

        oxp0 = xp0; oyp0 = yp0;

        //Clip to close parallel-screen plane
        if (yp0 < SCISDIST)
        {
            if (yp1 < SCISDIST) continue;
            t0 = (SCISDIST-yp0)/(yp1-yp0); xp0 = (xp1-xp0)*t0+xp0; yp0 = SCISDIST;
            nx0 = (wal2->x-wal->x)*t0+wal->x;
            ny0 = (wal2->y-wal->y)*t0+wal->y;
        }
        else { t0 = 0.f; nx0 = (float)wal->x; ny0 = (float)wal->y; }
        if (yp1 < SCISDIST)
        {
            t1 = (SCISDIST-oyp0)/(yp1-oyp0); xp1 = (xp1-oxp0)*t1+oxp0; yp1 = SCISDIST;
            nx1 = (wal2->x-wal->x)*t1+wal->x;
            ny1 = (wal2->y-wal->y)*t1+wal->y;
        }
        else { t1 = 1.f; nx1 = (float)wal2->x; ny1 = (float)wal2->y; }

        ryp0 = 1.f/yp0; ryp1 = 1.f/yp1;

        //Generate screen coordinates for front side of wall
        x0 = ghalfx*xp0*ryp0 + ghalfx;
        x1 = ghalfx*xp1*ryp1 + ghalfx;
        if (x1 <= x0) continue;

        ryp0 *= gyxscale; ryp1 *= gyxscale;

        getzsofslope(sectnum,Blrintf(nx0),Blrintf(ny0),&cz,&fz);
        cy0 = ((float)(cz-globalposz))*ryp0 + ghoriz;
        fy0 = ((float)(fz-globalposz))*ryp0 + ghoriz;
        getzsofslope(sectnum,Blrintf(nx1),Blrintf(ny1),&cz,&fz);
        cy1 = ((float)(cz-globalposz))*ryp1 + ghoriz;
        fy1 = ((float)(fz-globalposz))*ryp1 + ghoriz;

        globalpicnum = sec->floorpicnum; globalshade = sec->floorshade; globalpal = (int32_t)((uint8_t)sec->floorpal);
        globalorientation = sec->floorstat;
        globvis = globalcisibility;
        if (sector[sectnum].visibility != 0) globvis = mulscale4(globvis, (uint8_t)(sector[sectnum].visibility+16));

        DO_TILE_ANIM(globalpicnum, sectnum);

        dapskyoff = getpsky(globalpicnum, NULL, &dapskybits);

        global_cf_fogpal = sec->fogpal;
        global_cf_shade = sec->floorshade, global_cf_pal = sec->floorpal; global_cf_z = sec->floorz;  // REFACT
        global_cf_xpanning = sec->floorxpanning; global_cf_ypanning = sec->floorypanning, global_cf_heinum = sec->floorheinum;
        global_getzofslope_func = &getflorzofslope;

        if (!(globalorientation&1))
        {
#ifdef YAX_ENABLE
            if (globalposz <= sec->floorz || yax_getbunch(sectnum, YAX_FLOOR) < 0 || yax_getnextwall(wallnum, YAX_FLOOR) >= 0)
#endif
                polymost_internal_nonparallaxed(nx0, ny0, nx1, ny1, ryp0, ryp1, x0, x1, fy0, fy1, 1, sectnum);
        }
        else if ((nextsectnum < 0) || (!(sector[nextsectnum].floorstat&1)))
        {
            //Parallaxing sky... hacked for Ken's mountain texture; paper-sky only :/
#ifdef USE_OPENGL
            if (getrendermode() >= REND_POLYMOST)
            {
                if (!nofog) calc_and_apply_fog_factor(sec->floorpicnum, sec->floorshade, sec->visibility, sec->floorpal, 0.005f);

                //Use clamping for tiled sky textures
                for (i=(1<<dapskybits)-1; i>0; i--)
                    if (dapskyoff[i] != dapskyoff[i-1])
                        { skyclamphack = r_parallaxskyclamping; break; }
            }
#endif
            if (bpp == 8 || !usehightile || !hicfindskybox(globalpicnum, globalpal))
            {
//                g_nodraw = 1;

                dd[0] = fxdimen*.0000001f; //Adjust sky depth based on screen size!
                t = (float)((1<<(picsiz[globalpicnum]&15))<<dapskybits);
                vv[1] = dd[0]*((float)xdimscale*fviewingrange) * (1.f/(65536.f*65536.f));
                vv[0] = dd[0]*((float) ((tilesiz[globalpicnum].y>>1)/*+g_psky.yoffs*/)) - vv[1]*ghoriz;
                i = (1<<(picsiz[globalpicnum]>>4)); if (i != tilesiz[globalpicnum].y) i += i;

                //Hack to draw black rectangle below sky when looking down...
                gdx = 0; gdy = gxyaspect * (1.f/262144.f); gdo = -ghoriz*gdy;
                gux = 0; guy = 0; guo = 0;
                gvx = 0; gvy = (float)(tilesiz[globalpicnum].y-1)*gdy; gvo = (float)(tilesiz[globalpicnum].y-1)*gdo;
                oy = ((float)tilesiz[globalpicnum].y*dd[0]-vv[0])/vv[1];
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

                if (r_parallaxskypanning)
                    vv[0] += dd[0]*((float)sec->floorypanning)*((float)i)*(1.f/256.f);

                gdx = 0; gdy = 0; gdo = dd[0];
                gux = gdo *
                    (t * (float) ((uint64_t) (xdimscale * yxaspect) * viewingrange)) * (1.f/(16384.0*65536.0*65536.0*5.0*1024.0));
                guy = 0; //guo calculated later
                gvx = 0; gvy = vv[1]; gvo = vv[0];

                i = globalpicnum; r = (fy1-fy0)/(x1-x0); //slope of line
                oy = fviewingrange/(ghalfx*256.f); oz = 1.f/oy;

                y = ((((int32_t)((x0-ghalfx)*oy))+globalang)>>(11-dapskybits));
                fx = x0;
                do
                {
                    globalpicnum = dapskyoff[y&((1<<dapskybits)-1)]+i;
                    guo = gdo*(t*((float)(globalang-(y<<(11-dapskybits)))) * (1.f/2048.f) + (float)((r_parallaxskypanning)?sec->floorxpanning:0)) - gux*ghalfx;
                    y++;
                    ox = fx; fx = ((float)((y<<(11-dapskybits))-globalang))*oz+ghalfx;
                    if (fx > x1) { fx = x1; i = -1; }

                    pow2xsplit = 0; domost(ox,(ox-x0)*r+fy0,fx,(fx-x0)*r+fy0); //flor
                }
                while (i >= 0);

//                g_nodraw = 0;
            }
            else  //NOTE: code copied from ceiling code... lots of duplicated stuff :/
            {
                //Skybox code for parallax ceiling!
                float _xp0, _yp0, _xp1, _yp1, _oxp0, _oyp0, _t0, _t1; // _nx0, _ny0, _nx1, _ny1;
                float _ryp0, _ryp1, _x0, _x1, _cy0, _fy0, _cy1, _fy1, _ox0, _ox1;
                float nfy0, nfy1;
                int32_t skywalx[4] = {-512,512,512,-512}, skywaly[4] = {-512,-512,512,512};

                pow2xsplit = 0;
                skyclamphack = 1;

                for (i=0; i<4; i++)
                {
                    x = skywalx[i&3]; y = skywaly[i&3];
                    _xp0 = (float)y*gcosang  - (float)x*gsinang;
                    _yp0 = (float)x*gcosang2 + (float)y*gsinang2;
                    x = skywalx[(i+1)&3]; y = skywaly[(i+1)&3];
                    _xp1 = (float)y*gcosang  - (float)x*gsinang;
                    _yp1 = (float)x*gcosang2 + (float)y*gsinang2;

                    _oxp0 = _xp0; _oyp0 = _yp0;

                    //Clip to close parallel-screen plane
                    if (_yp0 < SCISDIST)
                    {
                        if (_yp1 < SCISDIST) continue;
                        _t0 = (SCISDIST-_yp0)/(_yp1-_yp0); _xp0 = (_xp1-_xp0)*_t0+_xp0; _yp0 = SCISDIST;
//                        _nx0 = (skywalx[(i+1)&3]-skywalx[i&3])*_t0+skywalx[i&3];
//                        _ny0 = (skywaly[(i+1)&3]-skywaly[i&3])*_t0+skywaly[i&3];
                    }
                    else { _t0 = 0.f; /*_nx0 = skywalx[i&3]; _ny0 = skywaly[i&3];*/ }
                    if (_yp1 < SCISDIST)
                    {
                        _t1 = (SCISDIST-_oyp0)/(_yp1-_oyp0); _xp1 = (_xp1-_oxp0)*_t1+_oxp0; _yp1 = SCISDIST;
//                        _nx1 = (skywalx[(i+1)&3]-skywalx[i&3])*_t1+skywalx[i&3];
//                        _ny1 = (skywaly[(i+1)&3]-skywaly[i&3])*_t1+skywaly[i&3];
                    }
                    else { _t1 = 1.f; /*_nx1 = skywalx[(i+1)&3]; _ny1 = skywaly[(i+1)&3];*/ }

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
                    ft[2] = fcosglobalang*(1.f/2147483648.f);
                    ft[3] = fsinglobalang*(1.f/2147483648.f);
                    gdx = 0;
                    gdy = gxyaspect*(1.f/4194304.f);
                    gdo = -ghoriz*gdy;
                    gux = (double)ft[3]*fviewingrange*(-1.0/65536.0);
                    gvx = (double)ft[2]*fviewingrange*(-1.0/65536.0);
                    guy = ft[0]*gdy; gvy = ft[1]*gdy;
                    guo = ft[0]*gdo; gvo = ft[1]*gdo;
                    guo += (ft[2]-gux)*ghalfx;
                    gvo -= (ft[3]+gvx)*ghalfx;
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
                    _t0 = -8192.f*_ryp0 + ghoriz;
                    _t1 = -8192.f*_ryp1 + ghoriz;
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
                ft[2] = fcosglobalang*(1.f/2147483648.f);
                ft[3] = fsinglobalang*(1.f/2147483648.f);
                gdx = 0;
                gdy = gxyaspect*(-1.f/4194304.f);
                gdo = -ghoriz*gdy;
                gux = ft[3]*((double)viewingrange)*(-1.0/65536.0);
                gvx = ft[2]*((double)viewingrange)*(-1.0/65536.0);
                guy = ft[0]*gdy; gvy = ft[1]*gdy;
                guo = ft[0]*gdo; gvo = ft[1]*gdo;
                guo += (ft[2]-gux)*ghalfx;
                gvo -= (ft[3]+gvx)*ghalfx;
                domost(x0,fy0,x1,fy1);

                skyclamphack = 0;
#ifdef USE_OPENGL
                drawingskybox = 0;
#endif
            }
#ifdef USE_OPENGL
            if (getrendermode() >= REND_POLYMOST)
            {
                skyclamphack = 0;
                if (!nofog)
                    bglEnable(GL_FOG);
            }
#endif
        }

        globalpicnum = sec->ceilingpicnum; globalshade = sec->ceilingshade; globalpal = (int32_t)((uint8_t)sec->ceilingpal);
        globalorientation = sec->ceilingstat;
        globvis = globalcisibility;
        if (sector[sectnum].visibility != 0) globvis = mulscale4(globvis, (uint8_t)(sector[sectnum].visibility+16));

        DO_TILE_ANIM(globalpicnum, sectnum);

        dapskyoff = getpsky(globalpicnum, NULL, &dapskybits);

        global_cf_fogpal = sec->fogpal;
        global_cf_shade = sec->ceilingshade, global_cf_pal = sec->ceilingpal; global_cf_z = sec->ceilingz;  // REFACT
        global_cf_xpanning = sec->ceilingxpanning; global_cf_ypanning = sec->ceilingypanning, global_cf_heinum = sec->ceilingheinum;
        global_getzofslope_func = &getceilzofslope;

        if (!(globalorientation&1))
        {
#ifdef YAX_ENABLE
            if (globalposz >= sec->ceilingz || yax_getbunch(sectnum, YAX_CEILING) < 0 || yax_getnextwall(wallnum, YAX_CEILING) >= 0)
#endif
                polymost_internal_nonparallaxed(nx0, ny0, nx1, ny1, ryp0, ryp1, x0, x1, cy0, cy1, 0, sectnum);
        }
        else if ((nextsectnum < 0) || (!(sector[nextsectnum].ceilingstat&1)))
        {
#ifdef USE_OPENGL
            if (getrendermode() >= REND_POLYMOST)
            {
                if (!nofog) calc_and_apply_fog_factor(sec->ceilingpicnum, sec->ceilingshade, sec->visibility, sec->ceilingpal, 0.005f);

                //Use clamping for tiled sky textures
                for (i=(1<<dapskybits)-1; i>0; i--)
                    if (dapskyoff[i] != dapskyoff[i-1])
                        { skyclamphack = r_parallaxskyclamping; break; }
            }
#endif
            //Parallaxing sky...
            if (bpp == 8 || !usehightile || !hicfindskybox(globalpicnum, globalpal))
            {
//                g_nodraw = 1;

                //Render for parallaxtype == 0 / paper-sky
                dd[0] = fxdimen*.0000001f; //Adjust sky depth based on screen size!
                t = (float)((1<<(picsiz[globalpicnum]&15))<<dapskybits);
                vv[1] = dd[0]*((float)xdimscale*viewingrange) * (1.f/(65536.f*65536.f));
                vv[0] = dd[0]*((float)((tilesiz[globalpicnum].y>>1)/*+g_psky.yoffs*/)) - vv[1]*ghoriz;
                i = (1<<(picsiz[globalpicnum]>>4)); if (i != tilesiz[globalpicnum].y) i += i;

                //Hack to draw black rectangle below sky when looking down...
                gdx = 0; gdy = gxyaspect * (1.f/-262144.f); gdo = -ghoriz*gdy;
                gux = 0; guy = 0; guo = 0;
                gvx = 0; gvy = 0; gvo = 0;
                oy = -vv[0]/vv[1];

                if ((oy < cy0) && (oy < cy1)) domost(x1,oy,x0,oy);
                else if ((oy < cy0) != (oy < cy1))
                {
                    /*         cy1        cy0
                    //        /              \
                    //oy----------      oy---------
                    //    /                   \
                    //  cy0                     cy1
                    */
                    ox = (oy-cy0)*(x1-x0)/(cy1-cy0) + x0;
                    if (oy < cy0) { domost(ox,oy,x0,oy); domost(x1,cy1,ox,oy); }
                    else { domost(ox,oy,x0,cy0); domost(x1,oy,ox,oy); }
                }
                else domost(x1,cy1,x0,cy0);

                if (r_parallaxskypanning)
                    vv[0] += dd[0]*(float)sec->ceilingypanning*(float)i*(1.f/256.f);
                
                gdx = 0; gdy = 0; gdo = dd[0];
                gux = gdo * 
                    (t * (float) ((uint64_t)(xdimscale * yxaspect) * viewingrange)) * (1.f/(16384.0*65536.0*65536.0*5.0*1024.0));
                guy = 0; //guo calculated later
                gvx = 0; gvy = vv[1]; gvo = vv[0];

                i = globalpicnum; r = (cy1-cy0)/(x1-x0); //slope of line
                oy = fviewingrange/(ghalfx*256.f); oz = 1.f/oy;

                y = ((((int32_t)((x0-ghalfx)*oy))+globalang)>>(11-dapskybits));
                fx = x0;
                do
                {
                    globalpicnum = dapskyoff[y&((1<<dapskybits)-1)]+i;
                    guo = gdo*(t*((float)(globalang-(y<<(11-dapskybits)))) * 1.f/2048.f + (float)((r_parallaxskypanning)?sec->ceilingxpanning:0)) - gux*ghalfx;
                    y++;
                    ox = fx; fx = ((float)((y<<(11-dapskybits))-globalang))*oz+ghalfx;
                    if (fx > x1) { fx = x1; i = -1; }
                    pow2xsplit = 0; domost(fx,(fx-x0)*r+cy0,ox,(ox-x0)*r+cy0); //ceil
                }
                while (i >= 0);

//                g_nodraw = 0;
            }
            else
            {
                //Skybox code for parallax ceiling!
                float _xp0, _yp0, _xp1, _yp1, _oxp0, _oyp0, _t0, _t1; // _nx0, _ny0, _nx1, _ny1;
                float _ryp0, _ryp1, _x0, _x1, _cy0, _fy0, _cy1, _fy1, _ox0, _ox1;
                float ncy0, ncy1;
                int32_t skywalx[4] = {-512,512,512,-512}, skywaly[4] = {-512,-512,512,512};

                pow2xsplit = 0;
                skyclamphack = 1;

                for (i=0; i<4; i++)
                {
                    x = skywalx[i&3]; y = skywaly[i&3];
                    _xp0 = (float)y*gcosang  - (float)x*gsinang;
                    _yp0 = (float)x*gcosang2 + (float)y*gsinang2;
                    x = skywalx[(i+1)&3]; y = skywaly[(i+1)&3];
                    _xp1 = (float)y*gcosang  - (float)x*gsinang;
                    _yp1 = (float)x*gcosang2 + (float)y*gsinang2;

                    _oxp0 = _xp0; _oyp0 = _yp0;

                    //Clip to close parallel-screen plane
                    if (_yp0 < SCISDIST)
                    {
                        if (_yp1 < SCISDIST) continue;
                        _t0 = (SCISDIST-_yp0)/(_yp1-_yp0); _xp0 = (_xp1-_xp0)*_t0+_xp0; _yp0 = SCISDIST;
//                        _nx0 = (skywalx[(i+1)&3]-skywalx[i&3])*_t0+skywalx[i&3];
//                        _ny0 = (skywaly[(i+1)&3]-skywaly[i&3])*_t0+skywaly[i&3];
                    }
                    else { _t0 = 0.f; /*_nx0 = skywalx[i&3]; _ny0 = skywaly[i&3];*/ }
                    if (_yp1 < SCISDIST)
                    {
                        _t1 = (SCISDIST-_oyp0)/(_yp1-_oyp0); _xp1 = (_xp1-_oxp0)*_t1+_oxp0; _yp1 = SCISDIST;
//                        _nx1 = (skywalx[(i+1)&3]-skywalx[i&3])*_t1+skywalx[i&3];
//                        _ny1 = (skywaly[(i+1)&3]-skywaly[i&3])*_t1+skywaly[i&3];
                    }
                    else { _t1 = 1.f; /*_nx1 = skywalx[(i+1)&3]; _ny1 = skywaly[(i+1)&3];*/ }

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
                    ft[2] = fcosglobalang*(1.f/2147483648.f);
                    ft[3] = fsinglobalang*(1.f/2147483648.f);
                    gdx = 0;
                    gdy = gxyaspect*(-1.f/4194304.f);
                    gdo = -ghoriz*gdy;
                    gux = ft[3]*fviewingrange*(-1.0/65536.0);
                    gvx = ft[2]*fviewingrange*(-1.0/65536.0);
                    guy = ft[0]*gdy; gvy = ft[1]*gdy;
                    guo = ft[0]*gdo; gvo = ft[1]*gdo;
                    guo += (ft[2]-gux)*ghalfx;
                    gvo -= (ft[3]+gvx)*ghalfx;
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
                    _t0 = -8192.f*_ryp0 + ghoriz;
                    _t1 = -8192.f*_ryp1 + ghoriz;
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
                ft[2] = fcosglobalang*(1.f/2147483648.f);
                ft[3] = fsinglobalang*(1.f/2147483648.f);
                gdx = 0;
                gdy = gxyaspect*(1.f/4194304.f);
                gdo = -ghoriz*gdy;
                gux = ft[3]*fviewingrange*(-1.0/65536.0);
                gvx = ft[2]*fviewingrange*(-1.0/65536.0);
                guy = ft[0]*gdy; gvy = ft[1]*gdy;
                guo = ft[0]*gdo; gvo = ft[1]*gdo;
                guo += (ft[2]-gux)*ghalfx;
                gvo -= (ft[3]+gvx)*ghalfx;
                gvx = -gvx; gvy = -gvy; gvo = -gvo; //y-flip skybox floor
                domost(x1,cy1,x0,cy0);

                skyclamphack = 0;
#ifdef USE_OPENGL
                drawingskybox = 0;
#endif
            }
#ifdef USE_OPENGL
            if (getrendermode() >= REND_POLYMOST)
            {
                skyclamphack = 0;
                if (!nofog)
                    bglEnable(GL_FOG);
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

        Bassert(domostpolymethod == 0);
        domostpolymethod = DAMETH_WALL;

        if (nextsectnum >= 0)
        {
            getzsofslope(nextsectnum,Blrintf(nx0),Blrintf(ny0),&cz,&fz);
            ocy0 = ((float)(cz-globalposz))*ryp0 + ghoriz;
            ofy0 = ((float)(fz-globalposz))*ryp0 + ghoriz;
            getzsofslope(nextsectnum,Blrintf(nx1),Blrintf(ny1),&cz,&fz);
            ocy1 = ((float)(cz-globalposz))*ryp1 + ghoriz;
            ofy1 = ((float)(fz-globalposz))*ryp1 + ghoriz;

            if ((wal->cstat&48) == 16) maskwall[maskwallcnt++] = z;

            if (((cy0 < ocy0) || (cy1 < ocy1)) && (!((sec->ceilingstat&sector[nextsectnum].ceilingstat)&1)))
            {
                globalpicnum = wal->picnum; globalshade = wal->shade; globalpal = (int32_t)((uint8_t)wal->pal);
                globvis = globalvisibility;
                if (sector[sectnum].visibility != 0) globvis = mulscale4(globvis, (uint8_t)(sector[sectnum].visibility+16));

                DO_TILE_ANIM(globalpicnum, wallnum+16384);

                if (!(wal->cstat&4)) i = sector[nextsectnum].ceilingz; else i = sec->ceilingz;

                // over
                calc_ypanning(i, ryp0, ryp1, x0, x1, wal->ypanning, wal->yrepeat, wal->cstat&4);

                if (wal->cstat&8) //xflip
                {
                    t = (float)(wal->xrepeat*8 + wal->xpanning*2);
                    gux = gdx*t - gux;
                    guy = gdy*t - guy;
                    guo = gdo*t - guo;
                }
                if (wal->cstat&256) { gvx = -gvx; gvy = -gvy; gvo = -gvo; } //yflip

                if (!nofog) calc_and_apply_fog(wal->picnum, fogpal_shade(sec, wal->shade), sec->visibility, get_floor_fogpal(sec));

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
                globvis = globalvisibility;
                if (sector[sectnum].visibility != 0) globvis = mulscale4(globvis, (uint8_t)(sector[sectnum].visibility+16));

                DO_TILE_ANIM(globalpicnum, wallnum+16384);

                if (!(nwal->cstat&4)) i = sector[nextsectnum].floorz; else i = sec->ceilingz;

                // under
                calc_ypanning(i, ryp0, ryp1, x0, x1, nwal->ypanning, wal->yrepeat, !(nwal->cstat&4));

                if (wal->cstat&8) //xflip
                {
                    t = (float)(wal->xrepeat*8 + nwal->xpanning*2);
                    gux = gdx*t - gux;
                    guy = gdy*t - guy;
                    guo = gdo*t - guo;
                }
                if (nwal->cstat&256) { gvx = -gvx; gvy = -gvy; gvo = -gvo; } //yflip

                if (!nofog) calc_and_apply_fog(nwal->picnum, fogpal_shade(sec, nwal->shade), sec->visibility, get_floor_fogpal(sec));

                pow2xsplit = 1; domost(x0,ofy0,x1,ofy1);
                if (wal->cstat&(2+8)) { guo = oguo; gux = ogux; guy = oguy; }
            }
        }

        if ((nextsectnum < 0) || (wal->cstat&32))   //White/1-way wall
        {
            const int32_t maskingOneWay = (nextsectnum >= 0 && (wal->cstat&32));

            if (nextsectnum < 0) globalpicnum = wal->picnum; else globalpicnum = wal->overpicnum;
            globalshade = wal->shade; globalpal = (int32_t)((uint8_t)wal->pal);
            globvis = globalvisibility;
            if (sector[sectnum].visibility != 0) globvis = mulscale4(globvis, (uint8_t)(sector[sectnum].visibility+16));

            DO_TILE_ANIM(globalpicnum, wallnum+16384);

            if (nextsectnum >= 0) { if (!(wal->cstat&4)) i = nextsec->ceilingz; else i = sec->ceilingz; }
            else { if (!(wal->cstat&4)) i = sec->ceilingz;     else i = sec->floorz; }

            // white / 1-way
            calc_ypanning(i, ryp0, ryp1, x0, x1, wal->ypanning, wal->yrepeat, !maskingOneWay && !(wal->cstat&4));

            if (wal->cstat&8) //xflip
            {
                t = (float)(wal->xrepeat*8 + wal->xpanning*2);
                gux = gdx*t - gux;
                guy = gdy*t - guy;
                guo = gdo*t - guo;
            }
            if (wal->cstat&256) { gvx = -gvx; gvy = -gvy; gvo = -gvo; } //yflip

            if (!nofog) calc_and_apply_fog(wal->picnum, fogpal_shade(sec, wal->shade), sec->visibility, get_floor_fogpal(sec));

            pow2xsplit = 1; domost(x0,-10000,x1,-10000);
        }

        domostpolymethod = 0;

        if (nextsectnum >= 0)
            if ((!(gotsector[nextsectnum>>3]&pow2char[nextsectnum&7])) && (testvisiblemost(x0,x1)))
                polymost_scansector(nextsectnum);
    }
}

static int32_t polymost_bunchfront(const int32_t b1, const int32_t b2)
{
    const int32_t b1f = bunchfirst[b1];
    const float x2b2 = dxb2[bunchlast[b2]];
    const float x1b1 = dxb1[b1f];

    if (x1b1 >= x2b2)
        return -1;

    {
        const int32_t b2f = bunchfirst[b2];
        int32_t i;
        const float x1b2 = dxb1[b2f];

        if (x1b2 >= dxb2[bunchlast[b1]])
            return -1;

        if (x1b1 >= x1b2)
        {
            for (i=b2f; dxb2[i]<=x1b1; i=bunchp2[i]);
            return wallfront(b1f, i);
        }

        for (i=b1f; dxb2[i]<=x1b2; i=bunchp2[i]);
        return wallfront(i, b2f);
    }
}

void polymost_scansector(int32_t sectnum)
{
    walltype *wal, *wal2;
    spritetype *spr;
    int32_t z, zz, startwall, endwall, numscansbefore, scanfirst, bunchfrst, nextsectnum, sectorbordercnt;
    vec2f_t p1, p2, fp1, fp2;
    float d;

    if (sectnum < 0) return;

    sectorborder[0] = sectnum, sectorbordercnt = 1;
    do
    {
        sectnum = sectorborder[--sectorbordercnt];

        for (z=headspritesect[sectnum]; z>=0; z=nextspritesect[z])
        {
            spr = &sprite[z];
            if ((((spr->cstat&0x8000) == 0) || (showinvisibility)) &&
                    (spr->xrepeat > 0) && (spr->yrepeat > 0))
            {
                vec2_t s = { spr->x-globalposx, spr->y-globalposy };

                if ((spr->cstat&48) || (usemodels && tile2model[spr->picnum].modelid>=0) || ((s.x * gcosang) + (s.y * gsinang) > 0))
                {
                    if ((spr->cstat&(64+48))!=(64+16) || dmulscale6(sintable[(spr->ang+512)&2047],-s.x, sintable[spr->ang&2047],-s.y) > 0)
                        if (engine_addtsprite(z, sectnum))
                            break;
                }
            }
        }

        gotsector[sectnum>>3] |= pow2char[sectnum&7];

        bunchfrst = numbunches;
        numscansbefore = numscans;

        startwall = sector[sectnum].wallptr; endwall = sector[sectnum].wallnum+startwall;
        scanfirst = numscans;
        p2.x = p2.y = 0;

        for (z=startwall,wal=&wall[z]; z<endwall; z++,wal++)
        {
            wal2 = &wall[wal->point2];
            fp1.x = (float)(wal->x-globalposx); fp1.y = (float)(wal->y-globalposy);
            fp2.x = (float)(wal2->x-globalposx); fp2.y = (float)(wal2->y-globalposy);

            nextsectnum = wal->nextsector; //Scan close sectors

            if (nextsectnum >= 0 && !(wal->cstat&32) && sectorbordercnt < ARRAY_SSIZE(sectorborder))
#ifdef YAX_ENABLE
            if (yax_nomaskpass==0 || !yax_isislandwall(z, !yax_globalcf) || (yax_nomaskdidit=1, 0))
#endif
            if ((gotsector[nextsectnum>>3]&pow2char[nextsectnum&7]) == 0)
            {
                d = fp1.x*fp2.y - fp2.x*fp1.y;
                p1.x = fp2.x-fp1.x;
                p1.y = fp2.y-fp1.y;

                if (d*d <= (p1.x*p1.x + p1.y*p1.y) * (SCISDIST*SCISDIST*260.f))
                {
                    sectorborder[sectorbordercnt++] = nextsectnum;
                    gotsector[nextsectnum>>3] |= pow2char[nextsectnum&7];
                }
            }

            if ((z == startwall) || (wall[z-1].point2 != z))
            {
                p1.x = ((fp1.y * fcosglobalang) - (fp1.x * fsinglobalang)) * (1.0f/64.f);
                p1.y = ((fp1.x * (float)cosviewingrangeglobalang) + (fp1.y * (float)sinviewingrangeglobalang)) * (1.0f/64.f);
            }
            else { p1 = p2; }

            p2.x = ((fp2.y * fcosglobalang) - (fp2.x * fsinglobalang)) * (1.0f/64.f);
            p2.y = ((fp2.x * (float) cosviewingrangeglobalang) + (fp2.y * (float) sinviewingrangeglobalang)) * (1.0f/64.f);

            if ((p1.y >= SCISDIST) || (p2.y >= SCISDIST))
                if (p1.x*p2.y < p2.x*p1.y) //if wall is facing you...
                {
                    if (p1.y >= SCISDIST)
                        dxb1[numscans] = p1.x*ghalfx/p1.y + ghalfx;
                    else dxb1[numscans] = -1e32f;

                    if (p2.y >= SCISDIST)
                        dxb2[numscans] = p2.x*ghalfx/p2.y + ghalfx;
                    else dxb2[numscans] = 1e32f;

                    if (dxb1[numscans] < dxb2[numscans])
                        { thesector[numscans] = sectnum; thewall[numscans] = z; bunchp2[numscans] = numscans+1; numscans++; }
                }

            if ((wall[z].point2 < z) && (scanfirst < numscans))
                { bunchp2[numscans-1] = scanfirst; scanfirst = numscans; }
        }

        for (z=numscansbefore; z<numscans; z++)
            if ((wall[thewall[z]].point2 != thewall[bunchp2[z]]) || (dxb2[z] > dxb1[bunchp2[z]]))
            {
                bunchfirst[numbunches++] = bunchp2[z]; bunchp2[z] = -1;
#ifdef YAX_ENABLE
                if (scansector_retfast)
                    return;
#endif
            }

        for (z=bunchfrst; z<numbunches; z++)
        {
            for (zz=bunchfirst[z]; bunchp2[zz]>=0; zz=bunchp2[zz]);
            bunchlast[z] = zz;
        }
    }
    while (sectorbordercnt > 0);
}

/*Init viewport boundary (must be 4 point convex loop):
//      (px[0],py[0]).----.(px[1],py[1])
//                  /      \
//                /          \
// (px[3],py[3]).--------------.(px[2],py[2])
*/

void initmosts(const float * px, const float * py, int32_t n)
{
    int32_t i, j, k, imin, vcnt;

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
    } while (i != j);

    if (px[i] > vsp[vcnt-1].x)
    {
        vsp[vcnt].x = px[i];
        vsp[vcnt].cy[0] = vsp[vcnt].fy[0] = py[i];
        vcnt++;
    }

    vsp_finalize_init(vsp, vcnt);
    gtag = vcnt;
}

void polymost_drawrooms()
{
    int32_t i, j, n, n2, closest;
    float ox, oy, oz, ox2, oy2, oz2, r, px[6], py[6], pz[6], px2[6], py2[6], pz2[6], sx[6], sy[6];

    if (getrendermode() == REND_CLASSIC) return;

    begindrawing();
    frameoffset = frameplace + windowy1*bytesperline + windowx1;

#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST)
    {
        resizeglcheck();
#ifdef YAX_ENABLE
        if (numyaxbunches==0)
#endif
        if (editstatus)
            bglClear(GL_COLOR_BUFFER_BIT);

        bglClear(GL_DEPTH_BUFFER_BIT);

        bglDisable(GL_BLEND);
        bglEnable(GL_TEXTURE_2D);
        bglEnable(GL_DEPTH_TEST);
        bglDepthFunc(GL_LESS); //NEVER,LESS,(,L)EQUAL,GREATER,(NOT,G)EQUAL,ALWAYS
//        bglDepthRange(0.0, 1.0); //<- this is more widely supported than glPolygonOffset

        //Enable this for OpenGL red-blue glasses mode :)
#ifdef REDBLUEMODE
        if (glredbluemode)
        {
            static int32_t grbfcnt = 0; grbfcnt++;
            if (redblueclearcnt < numpages) { redblueclearcnt++; bglColorMask(1,1,1,1); bglClear(GL_COLOR_BUFFER_BIT); }
            if (grbfcnt&1)
            {
                bglViewport(windowx1-16,yres-(windowy2+1),windowx2-(windowx1-16)+1,windowy2-windowy1+1);
                bglColorMask(1,0,0,1);
                globalposx += singlobalang>>10;
                globalposy -= cosglobalang>>10;
            }
            else
            {
                bglViewport(windowx1,yres-(windowy2+1),windowx2+16-windowx1+1,windowy2-windowy1+1);
                bglColorMask(0,1,1,1);
                globalposx -= singlobalang>>10;
                globalposy += cosglobalang>>10;
            }
        }
#endif
    }
#endif
    
    //Polymost supports true look up/down :) Here, we convert horizon to angle.
    //gchang&gshang are cos&sin of this angle (respectively)
    fviewingrange = (float) viewingrange;
    gyxscale = ((float)xdimenscale)*(1.0f/131072.f);
    gxyaspect = ((float)xyaspect*fviewingrange)*(5.f/(65536.f*262144.f));
    gviewxrange = fviewingrange * fxdimen * (1.f/(32768.f*1024.f));
    fcosglobalang = (float) cosglobalang;
    gcosang = fcosglobalang*(1.0f/262144.f);
    fsinglobalang = (float) singlobalang;
    gsinang = fsinglobalang*(1.0f/262144.f);
    gcosang2 = gcosang * (fviewingrange * (1.0f/65536.f));
    gsinang2 = gsinang * (fviewingrange * (1.0f/65536.f));
    ghalfx = fxdimen * .5f;
    grhalfxdown10 = 1.f/(ghalfx*1024.f);
    ghoriz = (float)globalhoriz;

    gvisibility = ((float)globalvisibility)*FOGSCALE;

    //global cos/sin height angle
    r = fydimen * .5f - ghoriz;
    gshang = r/Bsqrtf(r*r+ghalfx*ghalfx);
    gchang = Bsqrtf(1.f-gshang*gshang);
    ghoriz = fydimen * .5f;

    //global cos/sin tilt angle
    gctang = cos(gtang);
    gstang = sin(gtang);
    if (Bfabsf(gstang) < .001f) //This hack avoids nasty precision bugs in domost()
        { gstang = 0.f; if (gctang > 0.f) gctang = 1.f; else gctang = -1.f; }

    if (inpreparemirror)
        gstang = -gstang;

    //Generate viewport trapezoid (for handling screen up/down)
    px[0] = px[3] = 0-1; px[1] = px[2] = (float)(windowx2+1-windowx1+2);
    py[0] = py[1] = 0-1; py[2] = py[3] = (float)(windowy2+1-windowy1+2);
    
    n = 4;

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
        polymost_editorfunc();

    numscans = numbunches = 0;

    // MASKWALL_BAD_ACCESS
    // Fixes access of stale maskwall[maskwallcnt] (a "scan" index, in BUILD lingo):
    maskwallcnt = 0;

    polymost_scansector(globalcursectnum);

    if (inpreparemirror)
    {
        grhalfxdown10x = -grhalfxdown10;
        inpreparemirror = 0;

        // see engine.c: INPREPAREMIRROR_NO_BUNCHES
        if (numbunches > 0)
        {
            polymost_drawalls(0);
            numbunches--;
            bunchfirst[0] = bunchfirst[numbunches];
            bunchlast[0] = bunchlast[numbunches];
        }
    }
    else
        grhalfxdown10x = grhalfxdown10;

    while (numbunches > 0)
    {
        Bmemset(ptempbuf,0,numbunches+3); ptempbuf[0] = 1;

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
    if (getrendermode() >= REND_POLYMOST)
    {
        bglDepthFunc(GL_LESS); //NEVER,LESS,(,L)EQUAL,GREATER,(NOT,G)EQUAL,ALWAYS
//        bglDepthRange(0.0, 1.0); //<- this is more widely supported than glPolygonOffset
    }
#endif

    enddrawing();
}

void polymost_drawmaskwall(int32_t damaskwallcnt)
{
    vec2f_t dpxy[8];
    float dpx2[8], dpy2[8];
    float x0, x1, sx0, sy0, sx1, sy1, xp0, yp0, xp1, yp1, oxp0, oyp0, ryp0, ryp1;
    float r, t, t0, t1, csy[4], fsy[4];
    int32_t i, j, n, n2, z, sectnum, z1, z2, cz[4], fz[4], method;
    int32_t m0, m1;
    sectortype *sec, *nsec;
    walltype *wal, *wal2;

    z = maskwall[damaskwallcnt];
    wal = &wall[thewall[z]]; wal2 = &wall[wal->point2];
    sectnum = thesector[z]; sec = &sector[sectnum];

//    if (wal->nextsector < 0) return;
    // Without MASKWALL_BAD_ACCESS fix:
    // wal->nextsector is -1, WGR2 SVN Lochwood Hollow (Til' Death L1)  (or trueror1.map)

    nsec = &sector[wal->nextsector];
    z1 = max(nsec->ceilingz,sec->ceilingz);
    z2 = min(nsec->floorz,sec->floorz);

    globalpicnum = wal->overpicnum; if ((uint32_t)globalpicnum >= MAXTILES) globalpicnum = 0;
    globvis = globalvisibility;
    if (sector[sectnum].visibility != 0) globvis = mulscale4(globvis, (uint8_t)(sector[sectnum].visibility+16));

    DO_TILE_ANIM(globalpicnum, (int16_t)thewall[z]+16384);
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

    // mask
    calc_ypanning(i, ryp0, ryp1, x0, x1, wal->ypanning, wal->yrepeat, 0);

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
    method |= DAMETH_WALL;

    if (!nofog) calc_and_apply_fog(wal->picnum, fogpal_shade(sec, wal->shade), sec->visibility, get_floor_fogpal(sec));

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

    dpxy[0].x = x0; dpxy[0].y = csy[1];
    dpxy[1].x = x1; dpxy[1].y = csy[3];
    dpxy[2].x = x1; dpxy[2].y = fsy[3];
    dpxy[3].x = x0; dpxy[3].y = fsy[1];
    n = 4;

    //Clip to (x0,csy[0])-(x1,csy[2])
    n2 = 0; t1 = -((dpxy[0].x-x0)*(csy[2]-csy[0]) - (dpxy[0].y-csy[0])*(x1-x0));
    for (i=0; i<n; i++)
    {
        j = i+1; if (j >= n) j = 0;

        t0 = t1; t1 = -((dpxy[j].x-x0)*(csy[2]-csy[0]) - (dpxy[j].y-csy[0])*(x1-x0));
        if (t0 >= 0) { dpx2[n2] = dpxy[i].x; dpy2[n2] = dpxy[i].y; n2++; }
        if ((t0 >= 0) != (t1 >= 0))
        {
            r = t0/(t0-t1);
            dpx2[n2] = (dpxy[j].x-dpxy[i].x)*r + dpxy[i].x;
            dpy2[n2] = (dpxy[j].y-dpxy[i].y)*r + dpxy[i].y;
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
        if (t0 >= 0) { dpxy[n].x = dpx2[i]; dpxy[n].y = dpy2[i]; n++; }
        if ((t0 >= 0) != (t1 >= 0))
        {
            r = t0/(t0-t1);
            dpxy[n].x = (dpx2[j]-dpx2[i])*r + dpx2[i];
            dpxy[n].y = (dpy2[j]-dpy2[i])*r + dpy2[i];
            n++;
        }
    }
    if (n < 3) return;

    pow2xsplit = 0;
    skyclamphack = 0;
    alpha = 0.f;
    drawpoly(dpxy, n, method);
}

#define POINT2(i) (wall[wall[i].point2])


static inline int polymost_getclosestpointonwall(vec2_t const * const pos, int32_t dawall, vec2_t * const n)
{
    vec2_t w = { wall[dawall].x, wall[dawall].y };
    vec2_t p2 = { POINT2(dawall).x, POINT2(dawall).y };
    vec2_t d = { p2.x - w.x, p2.y - w.y };
    int64_t i = d.x * (pos->x - w.x) + d.y * (pos->y - w.y);

    if (i < 0)
        return 1;

    int64_t j = d.x * d.x + d.y * d.y;

    if (i > j)
        return 1;

    i = tabledivide64((i << 15), j) << 15;

    n->x = w.x + ((d.x * i) >> 30);
    n->y = w.y + ((d.y * i) >> 30);

    return 0;
}

typedef struct
{
    uint32_t wrev;
    uint32_t srev;
    int16_t wall;
    int8_t wdist;
    int8_t filler;
} wallspriteinfo_t;

wallspriteinfo_t wsprinfo[MAXSPRITES];

static inline int32_t polymost_findwall(tspritetype const * const tspr, int32_t * rd)
{
    int32_t dist = 8, closest = -1, dst;
    sectortype const * const sect = &sector[tspr->sectnum];
    vec2_t n;

    for (int i=sect->wallptr; i<sect->wallptr + sect->wallnum; i++)
    {
        if (!polymost_getclosestpointonwall((const vec2_t *)tspr, i, &n))
        {
            dst = klabs(tspr->x - n.x) + klabs(tspr->y - n.y);

            if (dst <= dist)
            {
                dist = dst;
                closest = i;
            }
        }
    }

    *rd = dist;
    return closest;
}

int32_t polymost_lintersect(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                            int32_t x3, int32_t y3, int32_t x4, int32_t y4)
{
    // p1 to p2 is a line segment
    int32_t const x21 = x2 - x1, x34 = x3 - x4;
    int32_t const y21 = y2 - y1, y34 = y3 - y4;
    int32_t const bot = x21 * y34 - y21 * x34;
    int32_t const x31 = x3 - x1, y31 = y3 - y1;
    int32_t const topt = x31 * y34 - y31 * x34;

    if (bot == 0)
        return 0;
    else if (bot > 0)
    {
        if ((unsigned)topt >= (unsigned)bot)
            return 0;

        int32_t topu = x21 * y31 - y21 * x31;

        if ((unsigned)topu >= (unsigned)bot)
            return 0;
    }
    else
    {
        if ((unsigned)topt <= (unsigned)bot)
            return 0;

        int32_t topu = x21 * y31 - y21 * x31;

        if ((unsigned)topu <= (unsigned)bot)
            return 0;
    }

    return 1;
}


void polymost_drawsprite(int32_t snum)
{
    vec2f_t pxy[6];
    float f, c, s, fx, fy, sx0, sy0, sx1, xp0, yp0, xp1, yp1, oxp0, oyp0, ryp0, ryp1, ft[4];
    float x0, y0, x1, y1, sc0, sf0, sc1, sf1, px2[6], py2[6], xv, yv, t0, t1;
    int32_t i, j, spritenum, xoff=0, yoff=0, method, npoints;
    int32_t posx,posy;
    int32_t oldsizx, oldsizy;
    int32_t tsizx, tsizy;

    tspritetype *const tspr = tspriteptr[snum];
    const sectortype *sec;

    if (EDUKE32_PREDICT_FALSE(bad_tspr(tspr)))
        return;

    spritenum         = tspr->owner;

    DO_TILE_ANIM(tspr->picnum, spritenum+32768);

    globalpicnum      = tspr->picnum;
    globalshade       = tspr->shade;
    globalpal         = tspr->pal;
    globalorientation = tspr->cstat;
    globvis = globalvisibility;
    if (sector[tspr->sectnum].visibility != 0) globvis = mulscale4(globvis, (uint8_t)(sector[tspr->sectnum].visibility+16));
    if ((globalorientation&48) != 48)  	// only non-voxel sprites should do this
    {
        int32_t flag;
        flag = usehightile && h_xsize[globalpicnum];
        xoff = (int32_t)tspr->xoffset;
        yoff = (int32_t)tspr->yoffset;
        xoff += flag ? h_xoffs[globalpicnum] : picanm[globalpicnum].xofs;
        yoff += flag ? h_yoffs[globalpicnum] : picanm[globalpicnum].yofs;
    }

    method = 1+4;
    if (tspr->cstat&2) { if (!(tspr->cstat&512)) method = 2+4; else method = 3+4; }

    alpha = spriteext[spritenum].alpha;

#ifdef USE_OPENGL
    sec = &sector[tspr->sectnum];
    if (!nofog) calc_and_apply_fog(tspr->picnum, fogpal_shade(sec, globalshade),
                       sec->visibility, get_floor_fogpal(sec));

    while (!(spriteext[spritenum].flags&SPREXT_NOTMD))
    {
        if (usemodels && tile2model[Ptile2tile(tspr->picnum,tspr->pal)].modelid >= 0 && tile2model[Ptile2tile(tspr->picnum,tspr->pal)].framenum >= 0)
        {
            if (polymost_mddraw(tspr))
                return;

            break;	// else, render as flat sprite
        }

        if (usevoxels && (tspr->cstat&48)!=48 && tiletovox[tspr->picnum] >= 0 && voxmodels[tiletovox[tspr->picnum]])
        {
            if (polymost_voxdraw(voxmodels[tiletovox[tspr->picnum]], tspr))
                return;
            break;	// else, render as flat sprite
        }

        if ((tspr->cstat&48)==48 && voxmodels[tspr->picnum])
        {
            polymost_voxdraw(voxmodels[tspr->picnum], tspr);
            return;
        }
        break;
    }
#endif

    posx=tspr->x;
    posy=tspr->y;
    if (spriteext[spritenum].flags&SPREXT_AWAY1)
    {
        posx+=(sintable[(tspr->ang+512)&2047]>>13);
        posy+=(sintable[(tspr->ang)&2047]>>13);
    }
    else if (spriteext[spritenum].flags&SPREXT_AWAY2)
    {
        posx-=(sintable[(tspr->ang+512)&2047]>>13);
        posy-=(sintable[(tspr->ang)&2047]>>13);
    }

    oldsizx=tsizx=tilesiz[globalpicnum].x;
    oldsizy=tsizy=tilesiz[globalpicnum].y;
    if (usehightile && h_xsize[globalpicnum])
    {
        tsizx = h_xsize[globalpicnum];
        tsizy = h_ysize[globalpicnum];
    }

    if (tsizx<=0 || tsizy<=0)
        return;

    switch ((globalorientation>>4)&3)
    {
    case 0: //Face sprite
        //Project 3D to 2D
        if (globalorientation&4) xoff = -xoff;
        // NOTE: yoff not negated not for y flipping, unlike wall and floor
        // aligned sprites.

        sx0 = (float)(tspr->x-globalposx);
        sy0 = (float)(tspr->y-globalposy);
        xp0 = sy0*gcosang  - sx0*gsinang;
        yp0 = sx0*gcosang2 + sy0*gsinang2;
        if (yp0 <= SCISDIST) return;
        ryp0 = 1.f/yp0;
        sx0 = ghalfx*xp0*ryp0 + ghalfx;
        sy0 = ((float)(tspr->z-globalposz))*gyxscale*ryp0 + ghoriz;

        f = ryp0*fxdimen*(1.0f/160.f);
        fx = ((float)tspr->xrepeat)*f;
        fy = ((float)tspr->yrepeat)*f*((float)yxaspect*(1.0f/65536.f));
        sx0 -= fx*(float)xoff; if (tsizx&1) sx0 += fx*0.5f;
        sy0 -= fy*(float)yoff;
        fx *= ((float)tsizx);
        fy *= ((float)tsizy);
        
        pxy[0].x = pxy[3].x = sx0-fx*0.5f; pxy[1].x = pxy[2].x = sx0+fx*0.5f;
        if (!(globalorientation&128)) { pxy[0].y = pxy[1].y = sy0-fy; pxy[2].y = pxy[3].y = sy0; }
        else { pxy[0].y = pxy[1].y = sy0-fy*0.5f; pxy[2].y = pxy[3].y = sy0+fy*0.5f; }

        gdx = gdy = guy = gvx = 0; gdo = ryp0*gviewxrange;
        if (!(globalorientation&4))
            { gux = (float)tsizx*gdo/(pxy[1].x-pxy[0].x+.002f); guo = -gux*(pxy[0].x-.001f); }
        else { gux = (float)tsizx*gdo/(pxy[0].x-pxy[1].x-.002f); guo = -gux*(pxy[1].x+.001f); }
        if (!(globalorientation&8))
            { gvy = (float)tsizy*gdo/(pxy[3].y-pxy[0].y+.002f); gvo = -gvy*(pxy[0].y-.001f); }
        else { gvy = (float)tsizy*gdo/(pxy[0].y-pxy[3].y-.002f); gvo = -gvy*(pxy[3].y+.001f); }

        // sprite panning
#ifdef USE_OPENGL
        if (spriteext[spritenum].xpanning)
        {
            guy -= gdy*((float) (spriteext[spritenum].xpanning)*(1.0f/255.f))*tsizx;
            guo -= gdo*((float) (spriteext[spritenum].xpanning)*(1.0f/255.f))*tsizx;
            srepeat = 1;
        }

        if (spriteext[spritenum].ypanning)
        {
            gvy -= gdy*((float) (spriteext[spritenum].ypanning)*(1.0f/255.f))*tsizy;
            gvo -= gdo*((float) (spriteext[spritenum].ypanning)*(1.0f/255.f))*tsizy;
            trepeat = 1;
        }
#endif

        //Clip sprites to ceilings/floors when no parallaxing and not sloped
        if (!(sector[tspr->sectnum].ceilingstat&3))
        {
            sy0 = ((float)(sector[tspr->sectnum].ceilingz-globalposz))*gyxscale*ryp0 + ghoriz;
            if (pxy[0].y < sy0) pxy[0].y = pxy[1].y = sy0;
        }
        if (!(sector[tspr->sectnum].floorstat&3))
        {
            sy0 = ((float)(sector[tspr->sectnum].floorz-globalposz))*gyxscale*ryp0 + ghoriz;
            if (pxy[2].y > sy0) pxy[2].y = pxy[3].y = sy0;
        }

        tilesiz[globalpicnum].x = tsizx;
        tilesiz[globalpicnum].y = tsizy;
        pow2xsplit = 0; drawpoly(pxy, 4, method);

#ifdef USE_OPENGL
        srepeat = 0;
        trepeat = 0;
#endif
        break;
    case 1: //Wall sprite

        //Project 3D to 2D
        if (globalorientation&4) xoff = -xoff;
        if (globalorientation&8) yoff = -yoff;


        xv = (float)tspr->xrepeat * (float)sintable[(tspr->ang)&2047] * (1.0f/65536.f);
        yv = (float)tspr->xrepeat * (float)sintable[(tspr->ang+1536)&2047] * (1.0f/65536.f);
        f = (float)(tsizx>>1) + (float)xoff;

#define WSPR_OFFSET .025f

        {
            vec2f_t vf = { xv * f, yv * f };
            // this handles wall sprites z-fighting with other wall sprites
            float const foffs = tspr->owner * (WSPR_OFFSET * 0.05f);

            x0 = (float)(posx - globalposx) - vf.x - foffs;
            y0 = (float)(posy - globalposy) - vf.y - foffs;

            int32_t const s = tspr->owner;
            int32_t walldist = 1;
            int32_t w = (s == -1) ? -1 : wsprinfo[s].wall;

            // find the wall most likely to be what the sprite is supposed to be ornamented against
            // this is really slow, so cache the result
            if (s == -1 || !wsprinfo[s].wall || (spritechanged[s] != wsprinfo[s].srev) ||
                (w != -1 && wallchanged[w] != wsprinfo[s].wrev))
            {
                w = polymost_findwall(tspr, &walldist);

                if (s != -1 && w != -1)
                {
                    wallspriteinfo_t *ws = &wsprinfo[s];

                    ws->wall = w;
                    ws->wdist = walldist;
                    ws->wrev = wallchanged[w];
                    ws->srev = spritechanged[s];
                }
            }
            else if (s != -1)
                walldist = wsprinfo[s].wdist;

            // detect if the sprite is either on the wall line or the wall line and sprite intersect
            if (w != -1)
            {
                vec2_t v = { Blrintf(vf.x), Blrintf(vf.y) };

                if (walldist <= 0 || ((posx - v.x) + (posx + v.x)) == (wall[w].x + POINT2(w).x) ||
                    ((posy - v.y) + (posy + v.y)) == (wall[w].y + POINT2(w).y) ||
                    polymost_lintersect(posx - v.x, posy - v.y, posx + v.x, posy + v.y,
                               wall[w].x, wall[w].y, POINT2(w).x, POINT2(w).y))
                {
                    int32_t ang = getangle(wall[w].x - POINT2(w).x, wall[w].y - POINT2(w).y);
                    vec2f_t offs = { (float)(sintable[(ang + 1024) & 2047] >> 6) * WSPR_OFFSET,
                                     (float)(sintable[(ang + 512) & 2047] >> 6) * WSPR_OFFSET };

                    x0 -= offs.x;
                    y0 -= offs.y;
                }
            }

        }

        x1 = xv * (float)tsizx + x0;
        y1 = yv * (float)tsizx + y0;

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

        ryp0 = 1.f/yp0;
        ryp1 = 1.f/yp1;
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
#ifdef USE_OPENGL
        if (spriteext[spritenum].xpanning)
        {
            t0 -= ((float) (spriteext[spritenum].xpanning)*(1.0f/255.f));
            t1 -= ((float) (spriteext[spritenum].xpanning)*(1.0f/255.f));
            srepeat = 1;
        }
#endif

        gux = (t0*ryp0 - t1*ryp1)*gxyaspect*(float) tsizx / (sx0-sx1);
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
#ifdef USE_OPENGL
        if (spriteext[spritenum].ypanning)
        {
            gvx -= gdx*((float) (spriteext[spritenum].ypanning)*(1.0f/255.f))*tsizy;
            gvy -= gdy*((float) (spriteext[spritenum].ypanning)*(1.0f/255.f))*tsizy;
            gvo -= gdo*((float) (spriteext[spritenum].ypanning)*(1.0f/255.f))*tsizy;
            trepeat = 1;
        }
#endif

        //Clip sprites to ceilings/floors when no parallaxing
        if (!(sector[tspr->sectnum].ceilingstat&1))
        {
            f = (float)((tspr->yrepeat * tsizy)<<2);

            if (sector[tspr->sectnum].ceilingz > tspr->z-f)
            {
                sc0 = (float)(sector[tspr->sectnum].ceilingz-globalposz)*ryp0 + ghoriz;
                sc1 = (float)(sector[tspr->sectnum].ceilingz-globalposz)*ryp1 + ghoriz;
            }
        }
        if (!(sector[tspr->sectnum].floorstat&1))
        {
            if (sector[tspr->sectnum].floorz < tspr->z)
            {
                sf0 = (float)(sector[tspr->sectnum].floorz-globalposz)*ryp0 + ghoriz;
                sf1 = (float)(sector[tspr->sectnum].floorz-globalposz)*ryp1 + ghoriz;
            }
        }

        if (sx0 > sx1)
        {
            if (globalorientation&64) return; //1-sided sprite
            swapfloat(&sx0, &sx1);
            swapfloat(&sc0, &sc1);
            swapfloat(&sf0, &sf1);
        }

        pxy[0].x = sx0; pxy[0].y = sc0;
        pxy[1].x = sx1; pxy[1].y = sc1;
        pxy[2].x = sx1; pxy[2].y = sf1;
        pxy[3].x = sx0; pxy[3].y = sf0;

        tilesiz[globalpicnum].x = tsizx;
        tilesiz[globalpicnum].y = tsizy;
        pow2xsplit = 0; drawpoly(pxy, 4,method);

#ifdef USE_OPENGL
        srepeat = 0;
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
        c = sintable[(i+512)&2047]*(1.0f/65536.f);
        s = sintable[i]*(1.0f/65536.f);
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
            pxy[j].x = sy0*gcosang  - sx0*gsinang;
            pxy[j].y = sx0*gcosang2 + sy0*gsinang2;
        }

        if (tspr->z < globalposz) //if floor sprite is above you, reverse order of points
        {
            swapfloat(&pxy[0].x, &pxy[1].x);
            swapfloat(&pxy[2].x, &pxy[3].x);
            swapfloat(&pxy[0].y, &pxy[1].y);
            swapfloat(&pxy[2].y, &pxy[3].y);
        }

        //Clip to SCISDIST plane
        npoints = 0;
        for (i=0; i<4; i++)
        {
            j = ((i+1)&3);
            if (pxy[i].y >= SCISDIST) { px2[npoints] = pxy[i].x; py2[npoints] = pxy[i].y; npoints++; }
            if ((pxy[i].y >= SCISDIST) != (pxy[j].y >= SCISDIST))
            {
                f = (SCISDIST-pxy[i].y)/(pxy[j].y-pxy[i].y);
                px2[npoints] = (pxy[j].x-pxy[i].x)*f + pxy[i].x;
                py2[npoints] = (pxy[j].y-pxy[i].y)*f + pxy[i].y; npoints++;
            }
        }
        if (npoints < 3) return;

#define FSPR_OFFSET .5f

        //Project rotated 3D points to screen

        i = 1;

        for (SPRITES_OF_SECT(tspr->sectnum, j), i++)
            if (j == tspr->owner)
                break;

        f = ((float)(tspr->z-globalposz) - (i * FSPR_OFFSET))*gyxscale;
        for (j=0; j<npoints; j++)
        {
            ryp0 = 1/py2[j];
            pxy[j].x = ghalfx*px2[j]*ryp0 + ghalfx;
            pxy[j].y = f*ryp0 + ghoriz;
        }

        //gd? Copied from floor rendering code
        gdx = 0;
        gdy = gxyaspect / (double)(tspr->z-globalposz - (i * FSPR_OFFSET));
        gdo = -ghoriz*gdy;
        //copied&modified from relative alignment
        xv = (float)tspr->x + s*x1 + c*y1; fx = (float)-(x0+x1)*s;
        yv = (float)tspr->y + s*y1 - c*x1; fy = (float)+(x0+x1)*c;
        f = polymost_invsqrt(fx*fx+fy*fy); fx *= f; fy *= f;
        ft[2] = singlobalang*fy + cosglobalang*fx;
        ft[3] = singlobalang*fx - cosglobalang*fy;
        ft[0] = ((float)(globalposy-yv))*fy + ((float)(globalposx-xv))*fx;
        ft[1] = ((float)(globalposx-xv))*fy - ((float)(globalposy-yv))*fx;
        gux = (float)ft[3]*fviewingrange * (1.f/(-65536.f*262144.f));
        gvx = (float)ft[2]*fviewingrange * (1.f/(-65536.f*262144.f));
        guy = (double)ft[0]*gdy; gvy = (double)ft[1]*gdy;
        guo = (double)ft[0]*gdo; gvo = (double)ft[1]*gdo;
        guo += (double)(ft[2]*(1.0f/262144.f)-gux)*ghalfx;
        gvo -= (double)(ft[3]*(1.0f/262144.f)+gvx)*ghalfx;
        f = 4.f/(float)tspr->xrepeat; gux *= f; guy *= f; guo *= f;
        f =-4.f/(float)tspr->yrepeat; gvx *= f; gvy *= f; gvo *= f;
        if (globalorientation&4)
        {
            gux = ((float)tsizx)*gdx - gux;
            guy = ((float)tsizx)*gdy - guy;
            guo = ((float)tsizx)*gdo - guo;
        }

        // sprite panning
#ifdef USE_OPENGL
        if (spriteext[spritenum].xpanning)
        {
            guy -= gdy*((float) (spriteext[spritenum].xpanning)*(1.0f/255.f))*tsizx;
            guo -= gdo*((float) (spriteext[spritenum].xpanning)*(1.0f/255.f))*tsizx;
            srepeat = 1;
        }

        if (spriteext[spritenum].ypanning)
        {
            gvy -= gdy*((float) (spriteext[spritenum].ypanning)*(1.0f/255.f))*tsizy;
            gvo -= gdo*((float) (spriteext[spritenum].ypanning)*(1.0f/255.f))*tsizy;
            trepeat = 1;
        }
#endif

        tilesiz[globalpicnum].x = tsizx;
        tilesiz[globalpicnum].y = tsizy;
        pow2xsplit = 0; drawpoly(pxy, npoints,method);

#ifdef USE_OPENGL
        srepeat = 0;
        trepeat = 0;
#endif

        break;

    case 3: //Voxel sprite
        break;
    }
    tilesiz[globalpicnum].x=oldsizx;
    tilesiz[globalpicnum].y=oldsizy;
}

EDUKE32_STATIC_ASSERT((int)RS_YFLIP == (int)HUDFLAG_FLIPPED);

//sx,sy       center of sprite; screen coords*65536
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

#ifdef USE_OPENGL
void polymost_dorotatespritemodel(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
    int8_t dashade, char dapalnum, int32_t dastat, uint8_t daalpha, int32_t uniqid)
{
    int32_t xoff, yoff, xsiz, ysiz;
    int32_t ogshade, ogpal;

    float ogchang, ogshang, ogctang, ogstang;
    float d, cosang, sinang, cosang2, sinang2;
    float m[4][4];

        const int32_t tilenum = Ptile2tile(picnum, dapalnum);

        if (tile2model[tilenum].modelid >= 0 &&
            tile2model[tilenum].framenum >= 0)
        {
            int32_t oldviewingrange;
            float ogxyaspect;
            vec3f_t vec1;

            tspritetype tspr;
            Bmemset(&tspr, 0, sizeof(spritetype));

            if (hudmem[(dastat&4)>>2][picnum].flags & HUDFLAG_HIDE)
                return;

            ogchang = gchang; gchang = 1.f;
            ogshang = gshang; gshang = 0.f; d = (float) z*(1.0f/(65536.f*16384.f));
            ogctang = gctang; gctang = (float) sintable[(a+512)&2047]*d;
            ogstang = gstang; gstang = (float) sintable[a&2047]*d;
            ogshade  = globalshade;  globalshade  = dashade;
            ogpal    = globalpal;    globalpal    = (int32_t) ((uint8_t) dapalnum);
            ogxyaspect = gxyaspect; gxyaspect = 1.f;
            oldviewingrange = viewingrange; viewingrange = 65536;

            vec1 = hudmem[(dastat&4)>>2][picnum].add;

#ifdef POLYMER
            if (pr_overridehud) {
                vec1.x = pr_hudxadd;
                vec1.y = pr_hudyadd;
                vec1.z = pr_hudzadd;
            }
#endif
            if (!(hudmem[(dastat&4)>>2][picnum].flags & HUDFLAG_NOBOB))
            {
                float fx = ((float) sx)*(1.f/65536.f);
                float fy = ((float) sy)*(1.f/65536.f);

                if (dastat & RS_TOPLEFT)
                {
                    xsiz = tilesiz[picnum].x; ysiz = tilesiz[picnum].y;
                    xoff = picanm[picnum].xofs + (xsiz>>1);
                    yoff = picanm[picnum].yofs + (ysiz>>1);

                    d = (float) z*(1.0f/(65536.f*16384.f));
                    cosang2 = cosang = (float) sintable[(a+512)&2047]*d;
                    sinang2 = sinang = (float) sintable[a&2047]*d;
                    if ((dastat&RS_AUTO) || (!(dastat&RS_NOCLIP))) //Don't aspect unscaled perms
                    {
                        d = (float) xyaspect*(1.0f/65536.f); cosang2 *= d; sinang2 *= d;
                    }
                    fx += -(float) xoff*cosang2+ (float) yoff*sinang2;
                    fy += -(float) xoff*sinang - (float) yoff*cosang;
                }

                if (!(dastat & RS_AUTO))
                {
                    vec1.x += fx/((float) (xdim<<15))-1.f; //-1: left of screen, +1: right of screen
                    vec1.y += fy/((float) (ydim<<15))-1.f; //-1: top of screen, +1: bottom of screen
                }
                else
                {
                    vec1.x += fx*(1.0f/160.f)-1.f; //-1: left of screen, +1: right of screen
                    vec1.y += fy*(1.0f/100.f)-1.f; //-1: top of screen, +1: bottom of screen
                }
            }
            tspr.ang = hudmem[(dastat&4)>>2][picnum].angadd+globalang;

#ifdef POLYMER
            if (pr_overridehud) {
                tspr.ang = pr_hudangadd + globalang;
            }
#endif

            if (dastat & RS_YFLIP) { vec1.x = -vec1.x; vec1.y = -vec1.y; }

            // In Polymost, we don't care if the model is very big
            if (getrendermode() < REND_POLYMER)
            {
                tspr.xrepeat = tspr.yrepeat = 32;

                tspr.x = globalposx + Blrintf((gcosang*vec1.z - gsinang*vec1.x)*16384.f);
                tspr.y = globalposy + Blrintf((gsinang*vec1.z + gcosang*vec1.x)*16384.f);
                tspr.z = globalposz + Blrintf(vec1.y * (16384.f * 0.8f));
            }
            else
            {
                vec3f_t vec2;

                tspr.xrepeat = tspr.yrepeat = 5;

                vec2.x = fglobalposx + (gcosang*vec1.z - gsinang*vec1.x)*2560.f;
                vec2.y = fglobalposy + (gsinang*vec1.z + gcosang*vec1.x)*2560.f;
                vec2.z = fglobalposz + (vec1.y*(2560.f*0.8f));

                Bmemcpy(&tspr.x, &vec2, sizeof(vec3f_t));
            }
            tspr.picnum = picnum;
            tspr.shade = dashade;
            tspr.pal = dapalnum;
            tspr.owner = uniqid+MAXSPRITES;
            // 1 -> 1
            // 32 -> 32*16 = 512
            // 4 -> 8
            tspr.cstat = globalorientation = (dastat&RS_TRANS1) | ((dastat&RS_TRANS2)<<4) | ((dastat&RS_YFLIP)<<1);

            if ((dastat&(RS_AUTO|RS_NOCLIP)) == RS_AUTO)
                bglViewport(windowx1, yres-(windowy2+1), windowx2-windowx1+1, windowy2-windowy1+1);
            else
            {
                bglViewport(0, 0, xdim, ydim);
                glox1 = -1; //Force fullscreen (glox1=-1 forces it to restore)
            }

            if (getrendermode() < REND_POLYMER)
            {
                bglMatrixMode(GL_PROJECTION);
                Bmemset(m, 0, sizeof(m));

                if ((dastat&(RS_AUTO|RS_NOCLIP)) == RS_AUTO)
                {
                    float f = 1.f;
                    int32_t fov = hudmem[(dastat&4)>>2][picnum].fov;
#ifdef POLYMER
                    if (pr_overridehud)
                        fov = pr_hudfov;
#endif
                    if (fov != -1)
                        f = 1.f/tanf(((float)fov * 2.56f) * ((.5f * PI) * (1.0f/2048.f)));

                    m[0][0] = f*fydimen; m[0][2] = 1.f;
                    m[1][1] = f*fxdimen; m[1][2] = 1.f;
                    m[2][2] = 1.f; m[2][3] = fydimen;
                    m[3][2] =-1.f;
                }
                else
                {
                    m[0][0] = m[2][3] = 1.f;
                    m[1][1] = fxdim/fydim;
                    m[2][2] = 1.0001f;
                    m[3][2] = 1-m[2][2];
                }

                bglLoadMatrixf(&m[0][0]);

                bglMatrixMode(GL_MODELVIEW);
                bglLoadIdentity();
            }

            if (hudmem[(dastat&4)>>2][picnum].flags & HUDFLAG_NODEPTH)
                bglDisable(GL_DEPTH_TEST);
            else
            {
                static int32_t onumframes = 0;

                bglEnable(GL_DEPTH_TEST);

                if (onumframes != numframes)
                {
                    onumframes = numframes;
                    bglClear(GL_DEPTH_BUFFER_BIT);
                }
            }

#ifdef USE_OPENGL
            spriteext[tspr.owner].alpha = daalpha * (1.0f / 255.0f);

            bglDisable(GL_FOG);

            if (getrendermode() == REND_POLYMOST)
                polymost_mddraw(&tspr);
# ifdef POLYMER
            else
            {
                int32_t fov;

                tspriteptr[MAXSPRITESONSCREEN] = &tspr;

                bglEnable(GL_ALPHA_TEST);
                bglEnable(GL_BLEND);

                spriteext[tspr.owner].roll = a;
                spriteext[tspr.owner].offset.z = z;

                fov = hudmem[(dastat&4)>>2][picnum].fov;

                if (fov == -1)
                    fov = pr_fov;

                if (pr_overridehud)
                    fov = pr_hudfov;

                polymer_setaspect(fov);

                polymer_drawsprite(MAXSPRITESONSCREEN);

                polymer_setaspect(pr_fov);

                spriteext[tspr.owner].offset.z = 0;
                spriteext[tspr.owner].roll = 0;

                bglDisable(GL_BLEND);
                bglDisable(GL_ALPHA_TEST);
            }
# endif
            if (!nofog) bglEnable(GL_FOG);
#else
            mddraw(&tspr);

            spriteext[tspr.owner].alpha = 0.f;
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

void polymost_dorotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                             int8_t dashade, char dapalnum, int32_t dastat, uint8_t daalpha,
                             int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2, int32_t uniqid)
{
    int32_t n, nn, xoff, yoff, xsiz, ysiz, method;
    int32_t ogpicnum, ogshade, ogpal, ofoffset;
    float ogchang, ogshang, ogctang, ogstang, oghalfx, oghoriz;
    float ogrhalfxdown10, ogrhalfxdown10x;
    float d, cosang, sinang, cosang2, sinang2, px2[8], py2[8];
    float m[4][4];
    vec2f_t pxy[8];

    int32_t ourxyaspect;

#if defined(USE_OPENGL) && defined(POLYMER)
    const int32_t olddetailmapping = r_detailmapping, oldglowmapping = r_glowmapping;
    const int32_t oldnormalmapping = pr_normalmapping;
#endif

#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST && usemodels && hudmem[(dastat&4)>>2][picnum].angadd)
    {
        polymost_dorotatespritemodel(sx, sy, z, a, picnum, dashade, dapalnum, dastat, daalpha, uniqid);
        return;
    }
#endif

    globvis = 0;
    ogpicnum = globalpicnum; globalpicnum = picnum;
    ogshade  = globalshade;  globalshade  = dashade;
    ogpal    = globalpal;    globalpal    = (int32_t)((uint8_t)dapalnum);
    oghalfx  = ghalfx;       ghalfx       = fxdim * .5f;
    ogrhalfxdown10 = grhalfxdown10;    grhalfxdown10 = 1.f/(ghalfx*1024.f);
    ogrhalfxdown10x = grhalfxdown10x;  grhalfxdown10x = grhalfxdown10;
    oghoriz  = ghoriz;       ghoriz       = fydim * .5f;
    ofoffset = frameoffset;  frameoffset  = frameplace;
    ogchang = gchang; gchang = 1.f;
    ogshang = gshang; gshang = 0.f;
    ogctang = gctang; gctang = 1.f;
    ogstang = gstang; gstang = 0.f;

#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST)
    {
        bglViewport(0,0,xdim,ydim); glox1 = -1; //Force fullscreen (glox1=-1 forces it to restore)
        bglMatrixMode(GL_PROJECTION);
        Bmemset(m,0,sizeof(m));
        m[0][0] = m[2][3] = 1.0f; m[1][1] = fxdim/fydim; m[2][2] = 1.0001f; m[3][2] = 1-m[2][2];
        bglPushMatrix(); bglLoadMatrixf(&m[0][0]);
        bglMatrixMode(GL_MODELVIEW);
        bglPushMatrix();
        bglLoadIdentity();

        bglDisable(GL_DEPTH_TEST);
        bglDisable(GL_ALPHA_TEST);
        bglEnable(GL_TEXTURE_2D);
        
# ifdef POLYMER
        if (getrendermode() == REND_POLYMER) {
            pr_normalmapping = 0;
            polymer_inb4rotatesprite(picnum, dapalnum, dashade);
            r_detailmapping = 0;
            r_glowmapping = 0;
        }
# endif
    }
#endif

    method = 0;
    if (!(dastat & RS_NOMASK))
    {
        if (dastat & RS_TRANS1)
            method = (dastat & RS_TRANS2) ? 3 : 2;
        else
            method = 1;
    }
    method |= 4; //Use OpenGL clamping - dorotatesprite never repeats

    alpha = daalpha * (1.0f / 255.0f);

    xsiz = tilesiz[globalpicnum].x;
    ysiz = tilesiz[globalpicnum].y;

    if (dastat & RS_TOPLEFT)
    {
        xoff = 0;
        yoff = 0;
    }
    else
    {
        xoff = picanm[globalpicnum].xofs + (xsiz>>1);
        yoff = picanm[globalpicnum].yofs + (ysiz>>1);
    }

    if (dastat & RS_YFLIP)
        yoff = ysiz-yoff;

    {
        int32_t temp;
        dorotspr_handle_bit2(&sx, &sy, &z, dastat, cx1+cx2, cy1+cy2, &temp, &ourxyaspect);
    }

    d = (float)z*(1.0f/(65536.f*16384.f));
    cosang2 = cosang = (float)sintable[(a+512)&2047]*d;
    sinang2 = sinang = (float)sintable[a&2047]*d;
    if ((dastat&RS_AUTO) || (!(dastat&RS_NOCLIP))) //Don't aspect unscaled perms
    {
        d = (float)ourxyaspect*(1.0f/65536.f);
        cosang2 *= d;
        sinang2 *= d;
    }

    pxy[0].x = (float)sx*(1.0f/65536.f) - (float)xoff*cosang2 + (float)yoff*sinang2;
    pxy[0].y = (float)sy*(1.0f/65536.f) - (float)xoff*sinang  - (float)yoff*cosang;
    pxy[1].x = pxy[0].x + (float)xsiz*cosang2;
    pxy[1].y = pxy[0].y + (float)xsiz*sinang;
    pxy[3].x = pxy[0].x - (float)ysiz*sinang2;
    pxy[3].y = pxy[0].y + (float)ysiz*cosang;
    pxy[2].x = pxy[1].x+pxy[3].x-pxy[0].x;
    pxy[2].y = pxy[1].y+pxy[3].y-pxy[0].y;
    n = 4;

    gdx = 0; gdy = 0; gdo = 1.f;
    //px[0]*gux + py[0]*guy + guo = 0
    //px[1]*gux + py[1]*guy + guo = xsiz-.0001
    //px[3]*gux + py[3]*guy + guo = 0
    d = 1.f/(pxy[0].x*(pxy[1].y-pxy[3].y) + pxy[1].x*(pxy[3].y-pxy[0].y) + pxy[3].x*(pxy[0].y-pxy[1].y));
    gux = (pxy[3].y-pxy[0].y)*((float)xsiz-.0001f)*d;
    guy = (pxy[0].x-pxy[3].x)*((float)xsiz-.0001f)*d;
    guo = 0 - pxy[0].x*gux - pxy[0].y*guy;

    if (!(dastat & RS_YFLIP))
    {
        //px[0]*gvx + py[0]*gvy + gvo = 0
        //px[1]*gvx + py[1]*gvy + gvo = 0
        //px[3]*gvx + py[3]*gvy + gvo = ysiz-.0001
        gvx = (pxy[0].y-pxy[1].y)*((float)ysiz-.0001f)*d;
        gvy = (pxy[1].x-pxy[0].x)*((float)ysiz-.0001f)*d;
        gvo = 0 - pxy[0].x*gvx - pxy[0].y*gvy;
    }
    else
    {
        //px[0]*gvx + py[0]*gvy + gvo = ysiz-.0001
        //px[1]*gvx + py[1]*gvy + gvo = ysiz-.0001
        //px[3]*gvx + py[3]*gvy + gvo = 0
        gvx = (pxy[1].y-pxy[0].y)*((float)ysiz-.0001f)*d;
        gvy = (pxy[0].x-pxy[1].x)*((float)ysiz-.0001f)*d;
        gvo = (float)ysiz-.0001f - pxy[0].x*gvx - pxy[0].y*gvy;
    }

    cx2++; cy2++;
    //Clippoly4 (converted from int32_t to double)
    nn = z = 0;
    do
    {
        float fx, x1, x2;
        int32_t zz = z+1; if (zz == n) zz = 0;
        x1 = pxy[z].x; x2 = pxy[zz].x-x1; if (((float)cx1 <= x1) && (x1 <= (float)cx2)) { px2[nn] = x1; py2[nn] = pxy[z].y; nn++; }
        if (x2 <= 0) fx = (float)cx2; else fx = (float)cx1;  d = fx-x1;
        if ((d < x2) != (d < 0)) { px2[nn] = fx; py2[nn] = (pxy[zz].y-pxy[z].y)*d/x2 + pxy[z].y; nn++; }
        if (x2 <= 0) fx = (float)cx1; else fx = (float)cx2;  d = fx-x1;
        if ((d < x2) != (d < 0)) { px2[nn] = fx; py2[nn] = (pxy[zz].y-pxy[z].y)*d/x2 + pxy[z].y; nn++; }
        z = zz;
    }
    while (z);

    if (nn >= 3)
    {
        n = z = 0;
        do
        {
            float fy, y1, y2;
            int32_t zz = z+1; if (zz == nn) zz = 0;
            y1 = py2[z]; y2 = py2[zz]-y1; if ((cy1 <= y1) && (y1 <= cy2)) { pxy[n].y = y1; pxy[n].x = px2[z]; n++; }
            if (y2 <= 0) fy = (float)cy2; else fy = (float)cy1;  d = fy-y1;
            if ((d < y2) != (d < 0)) { pxy[n].y = fy; pxy[n].x = (px2[zz]-px2[z])*d/y2 + px2[z]; n++; }
            if (y2 <= 0) fy = (float)cy1; else fy = (float)cy2;  d = fy-y1;
            if ((d < y2) != (d < 0)) { pxy[n].y = fy; pxy[n].x = (px2[zz]-px2[z])*d/y2 + px2[z]; n++; }
            z = zz;
        }
        while (z);

#ifdef USE_OPENGL
        bglDisable(GL_FOG);
        pow2xsplit = 0; drawpoly(pxy, n,method);
        if (!nofog) bglEnable(GL_FOG);
#else
        pow2xsplit = 0; drawpoly(pxy, n,method);
#endif
    }

#ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST)
    {
# ifdef POLYMER
        if (getrendermode() == REND_POLYMER) {
            r_detailmapping = olddetailmapping;
            r_glowmapping = oldglowmapping;
            polymer_postrotatesprite();
            pr_normalmapping = oldnormalmapping;
        }
# endif
        bglPopMatrix();
        bglMatrixMode(GL_PROJECTION); bglPopMatrix();
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
    gchang = ogchang;
    gshang = ogshang;
    gctang = ogctang;
    gstang = ogstang;
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

static void tessectrap(const float *px, const float *py, const int32_t *point2, int32_t numpoints)
{
    float x0, x1, m0, m1;
    int32_t i, j, k, z, i0, i1, i2, i3, npoints, gap, numrst;

    static int32_t allocpoints = 0, *slist = 0, *npoint2 = 0;
    typedef struct { float x, y, xi; int32_t i; } raster;
    static raster *rst = 0;
    if (numpoints+16 > allocpoints) //16 for safety
    {
        allocpoints = numpoints+16;
        rst = (raster *)Xrealloc(rst,allocpoints*sizeof(raster));
        slist = (int32_t *)Xrealloc(slist,allocpoints*sizeof(int32_t));
        npoint2 = (int32_t *)Xrealloc(npoint2,allocpoints*sizeof(int32_t));
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
    float f,a=0.f;
    int32_t i;

    globalx1 = mulscale16(globalx1,xyaspect);
    globaly2 = mulscale16(globaly2,xyaspect);
    gux = ((float)asm1)*(1.f/4294967296.f);
    gvx = ((float)asm2)*(1.f/4294967296.f);
    guy = ((float)globalx1)*(1.f/4294967296.f);
    gvy = ((float)globaly2)*(-1.f/4294967296.f);
    guo = (fxdim*gux + fydim*guy)*-0.5f + fglobalposx * (1.f/4294967296.f);
    gvo = (fxdim*gvx + fydim*gvy)*-0.5f - fglobalposy * (1.f/4294967296.f);
    //Convert int32_t to float (in-place)
    for (i=npoints-1; i>=0; i--)
    {
        ((float *)rx1)[i] = ((float)rx1[i])*(1.0f/4096.f);
        ((float *)ry1)[i] = ((float)ry1[i])*(1.0f/4096.f);
    }

    if (gloy1 != -1) setpolymost2dview(); //disables blending, texturing, and depth testing
    bglEnable(GL_ALPHA_TEST);
    bglEnable(GL_TEXTURE_2D);
    pth = our_texcache_fetch(0);
    bglBindTexture(GL_TEXTURE_2D, pth ? pth->glpic : 0);

    f = getshadefactor(globalshade);

    if (((globalorientation>>7)&3) > 1)
    {
        bglEnable(GL_BLEND);
        a = float_trans[(globalorientation>>7)&3];
        bglColor4f(f, f, f, a);
    }
    else
    {
        bglDisable(GL_BLEND);
        bglColor3f(f, f, f);
    }

    tessectrap((float *)rx1,(float *)ry1,xb1,npoints);
}
#endif

int32_t polymost_drawtilescreen(int32_t tilex, int32_t tiley, int32_t wallnum, int32_t dimen, int32_t tilezoom,
                                int32_t usehitile, uint8_t *loadedhitile)
{
#ifdef USE_OPENGL
    float xdime, ydime, xdimepad, ydimepad, scx, scy, ratio = 1.f;
    int32_t i;
    pthtyp *pth;

    if (getrendermode() < REND_POLYMOST || !in3dmode()) return(-1);

    if (!glinfo.texnpot)
    {
        i = (1<<(picsiz[wallnum]&15)); if (i < tilesiz[wallnum].x) i += i; xdimepad = (float)i;
        i = (1<<(picsiz[wallnum]>>4)); if (i < tilesiz[wallnum].y) i += i; ydimepad = (float)i;
    }
    else
    {
        xdimepad = (float)tilesiz[wallnum].x;
        ydimepad = (float)tilesiz[wallnum].y;
    }
    xdime = (float)tilesiz[wallnum].x; xdimepad = xdime/xdimepad;
    ydime = (float)tilesiz[wallnum].y; ydimepad = ydime/ydimepad;

    if ((xdime <= dimen) && (ydime <= dimen))
    {
        scx = xdime;
        scy = ydime;
    }
    else
    {
        scx = (float)dimen;
        scy = (float)dimen;
        if (xdime < ydime)
            scx *= xdime/ydime;
        else
            scy *= ydime/xdime;
    }

    {
        int32_t ousehightile = usehightile;
        usehightile = usehitile && usehightile;
        pth = texcache_fetch(wallnum, 0, 0, DAMETH_CLAMPED);
        if (usehightile)
            loadedhitile[wallnum>>3] |= (1<<(wallnum&7));
        usehightile = ousehightile;
    }

    bglBindTexture(GL_TEXTURE_2D, pth ? pth->glpic : 0);

    bglDisable(GL_ALPHA_TEST);

    if (tilezoom)
    {
        if (scx > scy) ratio = dimen/scx;
        else ratio = dimen/scy;
    }

    if (!pth || (pth->flags & PTH_HASALPHA))
    {
        bglDisable(GL_TEXTURE_2D);
        bglBegin(GL_TRIANGLE_FAN);
        if (gammabrightness)
            bglColor3f((float)curpalette[255].r*(1.0f/255.f),
                       (float)curpalette[255].g*(1.0f/255.f),
                       (float)curpalette[255].b*(1.0f/255.f));
        else
            bglColor3f((float)britable[curbrightness][ curpalette[255].r ] * (1.0f/255.f),
                       (float)britable[curbrightness][ curpalette[255].g ] * (1.0f/255.f),
                       (float)britable[curbrightness][ curpalette[255].b ] * (1.0f/255.f));
        bglVertex2f((float)tilex            ,(float)tiley);
        bglVertex2f((float)tilex+(scx*ratio),(float)tiley);
        bglVertex2f((float)tilex+(scx*ratio),(float)tiley+(scy*ratio));
        bglVertex2f((float)tilex            ,(float)tiley+(scy*ratio));
        bglEnd();
    }

    bglColor3f(1,1,1);
    bglEnable(GL_TEXTURE_2D);
    bglEnable(GL_BLEND);
    bglBegin(GL_TRIANGLE_FAN);
    bglTexCoord2f(0,              0); bglVertex2f((float)tilex            ,(float)tiley);
    bglTexCoord2f(xdimepad,       0); bglVertex2f((float)tilex+(scx*ratio),(float)tiley);
    bglTexCoord2f(xdimepad,ydimepad); bglVertex2f((float)tilex+(scx*ratio),(float)tiley+(scy*ratio));
    bglTexCoord2f(0,       ydimepad); bglVertex2f((float)tilex            ,(float)tiley+(scy*ratio));
    bglEnd();

    return(0);
#else
    return -1;
#endif
}

static int32_t gen_font_glyph_tex(void)
{
    // construct a 256x128 8-bit alpha-only texture for the font glyph matrix
    char *tbuf, *cptr, *tptr;
    int32_t h,i,j;

    bglGenTextures(1,&polymosttext);
    if (!polymosttext) return -1;

    tbuf = (char *)Xmalloc(256*128);
    Bmemset(tbuf, 0, 256*128);

    cptr = (char *)textfont;
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

    cptr = (char *)smalltextfont;
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
    bglTexImage2D(GL_TEXTURE_2D,0,GL_ALPHA,256,128,0,GL_ALPHA,GL_UNSIGNED_BYTE,(GLvoid *)tbuf);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    Bfree(tbuf);

    return 0;
}

int32_t polymost_printext256(int32_t xpos, int32_t ypos, int16_t col, int16_t backcol, const char *name, char fontsize)
{
#ifndef USE_OPENGL
    return -1;
#else
    GLfloat tx, ty, txc, tyc;
    int32_t c;
    palette_t p,b;
    int32_t arbackcol = (unsigned)backcol < 256 ? backcol : 0;

    // FIXME?
    if (col < 0)
        col = 0;

    bricolor(&p, col);
    bricolor(&b, arbackcol);

    if (getrendermode() < REND_POLYMOST || !in3dmode()) return(-1);

    if (!polymosttext)
    {
        if (gen_font_glyph_tex() < 0)
            return -1;
    }
    else
    {
        bglBindTexture(GL_TEXTURE_2D, polymosttext);
    }

    setpolymost2dview();	// disables blending, texturing, and depth testing
    bglDisable(GL_ALPHA_TEST);
    bglDepthMask(GL_FALSE);	// disable writing to the z-buffer

//    bglPushAttrib(GL_POLYGON_BIT|GL_ENABLE_BIT);
    // XXX: Don't fogify the OSD text in Mapster32 with r_usenewshading >= 2.
    bglDisable(GL_FOG);
    // We want to have readable text in wireframe mode, too:
    bglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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

    bglEnable(GL_TEXTURE_2D);
    bglEnable(GL_BLEND);
    bglColor4ub(p.r,p.g,p.b,255);
    txc = fontsize ? (4.f/256.f) : (8.f/256.f);
    tyc = fontsize ? (6.f/128.f) : (8.f/128.f);
    bglBegin(GL_QUADS);
    for (c=0; name[c]; c++)
    {
        if (name[c] == '^' && isdigit(name[c+1]))
        {
            char smallbuf[8];
            int32_t bi=0;
            while (isdigit(name[c+1]) && bi<3)
            {
                smallbuf[bi++]=name[c+1];
                c++;
            }
            smallbuf[bi++]=0;
            if (col)col = Batol(smallbuf);
            if ((unsigned)col >= 256)
                col = 0;

            bricolor(&p, col);

            bglColor4ub(p.r,p.g,p.b,255);
            continue;
        }
        tx = (float)(name[c]%32) * (1.0f/32.f);
        ty = (float)((name[c]/32) + (fontsize*8))*(1.0f/16.f);

        bglTexCoord2f(tx,ty);         bglVertex2i(xpos,ypos);
        bglTexCoord2f(tx+txc,ty);     bglVertex2i(xpos+(8>>fontsize),ypos);
        bglTexCoord2f(tx+txc,ty+tyc); bglVertex2i(xpos+(8>>fontsize),ypos+(fontsize?6:8));
        bglTexCoord2f(tx,ty+tyc);     bglVertex2i(xpos,ypos+(fontsize?6:8));

        xpos += (8>>fontsize);
    }
    bglEnd();

    bglDepthMask(GL_TRUE);	// re-enable writing to the z-buffer
//    bglPopAttrib();
    if (!nofog) bglEnable(GL_FOG);

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
        for (m = 0; m < NUMGLFILTERMODES; m++)
            OSD_Printf("     %d - %s\n", m, glfiltermodes[m].name);

        return OSDCMD_OK;
    }

    m = Bstrtoul(parm->parms[0], (char **)&p, 10);
    if (p == parm->parms[0])
    {
        // string
        for (m = 0; m < NUMGLFILTERMODES; m++)
        {
            if (!Bstrcasecmp(parm->parms[0], glfiltermodes[m].name))
                break;
        }

        if (m == NUMGLFILTERMODES)
            m = gltexfiltermode;   // no change
    }
    else
    {
        m = clamp(m, 0, NUMGLFILTERMODES-1);
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
    {
        if (r == OSDCMD_OK)
        {
#ifdef POLYMER
            if (!Bstrcasecmp(parm->name, "r_pr_maxlightpasses"))
            {
                pr_maxlightpasses = r_pr_maxlightpasses;
                return r;
            }
#endif
        }

        return r;
    }

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
            if (r_downsizevar == -1)
                r_downsizevar = r_downsize;

            if (in3dmode() && r_downsize != r_downsizevar)
            {
                texcache_invalidate();
                resetvideomode();
                if (setgamemode(fullscreen,xdim,ydim,bpp))
                    OSD_Printf("restartvid: Reset failed...\n");
            }

            r_downsizevar = r_downsize;

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
        else if (!Bstrcasecmp(parm->name, "r_usenewshading"))
        {
            bglFogi(GL_FOG_MODE, (r_usenewshading < 2) ? GL_EXP2 : GL_LINEAR);
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
        { "r_animsmoothing","enable/disable model animation smoothing",(void *) &r_animsmoothing, CVAR_BOOL, 0, 1 },
        { "r_downsize","controls downsizing factor (quality) for hires textures",(void *) &r_downsize, CVAR_INT|CVAR_FUNCPTR, 0, 5 },
        { "r_fullbrights","enable/disable fullbright textures",(void *) &r_fullbrights, CVAR_BOOL, 0, 1 },
        { "r_parallaxskyclamping","enable/disable parallaxed floor/ceiling sky texture clamping", (void *) &r_parallaxskyclamping, CVAR_BOOL, 0, 1 },
        { "r_parallaxskypanning","enable/disable parallaxed floor/ceiling panning when drawing a parallaxing sky", (void *) &r_parallaxskypanning, CVAR_BOOL, 0, 1 },

#ifndef EDUKE32_GLES
        { "r_detailmapping","enable/disable detail mapping",(void *) &r_detailmapping, CVAR_BOOL, 0, 1 },
        { "r_glowmapping","enable/disable glow mapping",(void *) &r_glowmapping, CVAR_BOOL, 0, 1 },
        { "r_polygonmode","debugging feature",(void *) &glpolygonmode, CVAR_INT | CVAR_NOSAVE, 0, 3 },
        { "r_texcache","enable/disable OpenGL compressed texture cache",(void *) &glusetexcache, CVAR_INT, 0, 2 },
        { "r_memcache","enable/disable texture cache memory cache",(void *) &glusememcache, CVAR_BOOL, 0, 1 },
        { "r_texcompr","enable/disable OpenGL texture compression",(void *) &glusetexcompr, CVAR_BOOL, 0, 1 },
#endif

#ifdef REDBLUEMODE
        { "r_redbluemode","enable/disable experimental OpenGL red-blue glasses mode",(void *) &glredbluemode, CVAR_BOOL, 0, 1 },
#endif
        { "r_shadescale","multiplier for shading",(void *) &shadescale, CVAR_FLOAT, 0, 10 },
        { "r_shadescale_unbounded","enable/disable allowance of complete blackness",(void *) &shadescale_unbounded, CVAR_BOOL, 0, 1 },
        { "r_swapinterval","sets the GL swap interval (VSync)",(void *) &vsync, CVAR_INT|CVAR_FUNCPTR, -1, 1 },
        {
            "r_npotwallmode", "enable/disable emulation of walls with non-power-of-two height textures (Polymost, r_hightile 0)",
            (void *) &r_npotwallmode, CVAR_BOOL, 0, 1
        },
        { "r_textureanisotropy", "changes the OpenGL texture anisotropy setting", (void *) &glanisotropy, CVAR_INT|CVAR_FUNCPTR, 0, 16 },
        { "r_texturemaxsize","changes the maximum OpenGL texture size limit",(void *) &gltexmaxsize, CVAR_INT | CVAR_NOSAVE, 0, 4096 },
        { "r_texturemiplevel","changes the highest OpenGL mipmap level used",(void *) &gltexmiplevel, CVAR_INT, 0, 6 },
        { "r_texturemode", "changes the texture filtering settings", (void *) &gltexfiltermode, CVAR_INT|CVAR_FUNCPTR, 0, 5 },

        { "r_usenewshading",
          "visibility/fog code: 0: orig. Polymost   1: 07/2011   2: linear 12/2012   3: no neg. start 03/2014",
          (void *) &r_usenewshading, CVAR_INT|CVAR_FUNCPTR, 0, 3
        },

        { "r_usetileshades", "enable/disable Polymost tile shade textures", (void *) &r_usetileshades, CVAR_INT | CVAR_INVALIDATEART, 0, 2 },
        { "r_vbocount","sets the number of Vertex Buffer Objects to use when drawing models",(void *) &r_vbocount, CVAR_INT, 1, 256 },
        { "r_vbos"," enable/disable using Vertex Buffer Objects when drawing models",(void *) &r_vbos, CVAR_BOOL, 0, 1 },
        { "r_vertexarrays","enable/disable using vertex arrays when drawing models",(void *) &r_vertexarrays, CVAR_BOOL, 0, 1 },
        { "r_projectionhack", "enable/disable projection hack", (void *) &glprojectionhacks, CVAR_INT, 0, 2 },

#ifdef POLYMER
        // polymer cvars
        { "r_pr_lighting", "enable/disable dynamic lights - restarts renderer", (void *) &pr_lighting, CVAR_BOOL | CVAR_RESTARTVID, 0, 1 },
        { "r_pr_normalmapping", "enable/disable virtual displacement mapping", (void *) &pr_normalmapping, CVAR_BOOL, 0, 1 },
        { "r_pr_specularmapping", "enable/disable specular mapping", (void *) &pr_specularmapping, CVAR_BOOL, 0, 1 },
        { "r_pr_shadows", "enable/disable dynamic shadows", (void *) &pr_shadows, CVAR_BOOL, 0, 1 },
        { "r_pr_shadowcount", "maximal amount of shadow emitting lights on screen - you need to restart the renderer for it to take effect", (void *) &pr_shadowcount, CVAR_INT, 0, 64 },
        { "r_pr_shadowdetail", "sets the shadow map resolution - you need to restart the renderer for it to take effect", (void *) &pr_shadowdetail, CVAR_INT, 0, 5 },
        { "r_pr_shadowfiltering", "enable/disable shadow edges filtering - you need to restart the renderer for it to take effect", (void *) &pr_shadowfiltering, CVAR_BOOL, 0, 1 },
        { "r_pr_maxlightpasses", "the maximal amount of lights a single object can by affected by", (void *) &r_pr_maxlightpasses, CVAR_INT|CVAR_FUNCPTR, 0, PR_MAXLIGHTS },
        { "r_pr_maxlightpriority", "lowering that value removes less meaningful lights from the scene", (void *) &pr_maxlightpriority, CVAR_INT, 0, PR_MAXLIGHTPRIORITY },
        { "r_pr_fov", "sets the field of vision in build angle", (void *) &pr_fov, CVAR_INT, 0, 1023},
        { "r_pr_customaspect", "if non-zero, forces the 3D view aspect ratio", (void *) &pr_customaspect, CVAR_DOUBLE, 0, 3 },
        { "r_pr_billboardingmode", "face sprite display method. 0: classic mode; 1: polymost mode", (void *) &pr_billboardingmode, CVAR_INT, 0, 1 },
        { "r_pr_verbosity", "verbosity level of the polymer renderer", (void *) &pr_verbosity, CVAR_INT, 0, 3 },
        { "r_pr_wireframe", "toggles wireframe mode", (void *) &pr_wireframe, CVAR_INT | CVAR_NOSAVE, 0, 1 },
        { "r_pr_vbos", "contols Vertex Buffer Object usage. 0: no VBOs. 1: VBOs for map data. 2: VBOs for model data.", (void *) &pr_vbos, CVAR_INT | CVAR_RESTARTVID, 0, 2 },
        { "r_pr_gpusmoothing", "toggles model animation interpolation", (void *) &pr_gpusmoothing, CVAR_INT, 0, 1 },
        { "r_pr_overrideparallax", "overrides parallax mapping scale and bias values with values from the pr_parallaxscale and pr_parallaxbias cvars; use it to fine-tune DEF tokens", (void *) &pr_overrideparallax, CVAR_BOOL | CVAR_NOSAVE, 0, 1 },
        { "r_pr_parallaxscale", "overriden parallax mapping offset scale", (void *) &pr_parallaxscale, CVAR_FLOAT | CVAR_NOSAVE, -10, 10 },
        { "r_pr_parallaxbias", "overriden parallax mapping offset bias", (void *) &pr_parallaxbias, CVAR_FLOAT | CVAR_NOSAVE, -10, 10 },
        { "r_pr_overridespecular", "overrides specular material power and factor values with values from the pr_specularpower and pr_specularfactor cvars; use it to fine-tune DEF tokens", (void *) &pr_overridespecular, CVAR_BOOL | CVAR_NOSAVE, 0, 1 },
        { "r_pr_specularpower", "overriden specular material power", (void *) &pr_specularpower, CVAR_FLOAT | CVAR_NOSAVE, -10, 1000 },
        { "r_pr_specularfactor", "overriden specular material factor", (void *) &pr_specularfactor, CVAR_FLOAT | CVAR_NOSAVE, -10, 1000 },
        { "r_pr_highpalookups", "enable/disable highpalookups", (void *) &pr_highpalookups, CVAR_BOOL, 0, 1 },
        { "r_pr_artmapping", "enable/disable art mapping", (void *) &pr_artmapping, CVAR_BOOL | CVAR_INVALIDATEART, 0, 1 },
        { "r_pr_overridehud", "overrides hud model parameters with values from the pr_hud* cvars; use it to fine-tune DEF tokens", (void *) &pr_overridehud, CVAR_BOOL | CVAR_NOSAVE, 0, 1 },
        { "r_pr_hudxadd", "overriden HUD xadd; see r_pr_overridehud", (void *) &pr_hudxadd, CVAR_FLOAT | CVAR_NOSAVE, -100, 100 },
        { "r_pr_hudyadd", "overriden HUD yadd; see r_pr_overridehud", (void *) &pr_hudyadd, CVAR_FLOAT | CVAR_NOSAVE, -100, 100 },
        { "r_pr_hudzadd", "overriden HUD zadd; see r_pr_overridehud", (void *) &pr_hudzadd, CVAR_FLOAT | CVAR_NOSAVE, -100, 100 },
        { "r_pr_hudangadd", "overriden HUD angadd; see r_pr_overridehud", (void *) &pr_hudangadd, CVAR_INT | CVAR_NOSAVE, -1024, 1024 },
        { "r_pr_hudfov", "overriden HUD fov; see r_pr_overridehud", (void *) &pr_hudfov, CVAR_INT | CVAR_NOSAVE, 0, 1023 },
        { "r_pr_overridemodelscale", "overrides model scale if non-zero; use it to fine-tune DEF tokens", (void *) &pr_overridemodelscale, CVAR_FLOAT | CVAR_NOSAVE, 0, 500 },
        { "r_pr_ati_fboworkaround", "enable this to workaround an ATI driver bug that causes sprite shadows to be square - you need to restart the renderer for it to take effect", (void *) &pr_ati_fboworkaround, CVAR_BOOL | CVAR_NOSAVE, 0, 1 },
        { "r_pr_ati_nodepthoffset", "enable this to workaround an ATI driver bug that causes sprite drawing to freeze the game on Radeon X1x00 hardware - you need to restart the renderer for it to take effect", (void *) &pr_ati_nodepthoffset, CVAR_BOOL | CVAR_NOSAVE, 0, 1 },
#endif

#ifdef __ANDROID__
        { "r_models","enable/disable model rendering",(void *) &usemodels, CVAR_BOOL | CVAR_NOSAVE, 0, 1 },
#else
        { "r_models","enable/disable model rendering",(void *) &usemodels, CVAR_BOOL, 0, 1 },
#endif
        { "r_nofog", "enable/disable GL fog", (void *)&nofog, CVAR_BOOL, 0, 1},
        { "r_hightile","enable/disable hightile texture rendering",(void *) &usehightile, CVAR_BOOL, 0, 1 },

        { "r_preview_mouseaim", "toggles mouse aiming preview, use this to calibrate yxaspect in Polymost Mapster32", (void *) &preview_mouseaim, CVAR_BOOL, 0, 1 },
#endif
    };

    for (i=0; i<ARRAY_SIZE(cvars_polymost); i++)
    {
        // can't do this: editstatus is set after this function
//        if (editstatus==0 && !Bstrcmp(cvars_polymost[i].name, "r_preview_mouseaim"))
//            continue;

        if (OSD_RegisterCvar(&cvars_polymost[i]))
            continue;

        OSD_RegisterFunction(cvars_polymost[i].name, cvars_polymost[i].desc,
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

    if (getrendermode() < REND_POLYMOST) return;

    if ((palookup[dapalnum] == NULL) && (dapalnum < (MAXPALOOKUPS - RESERVEDPALS))) return;//dapalnum = 0;

    //OSD_Printf("precached %d %d type %d\n", dapicnum, dapalnum, datype);
    hicprecaching = 1;


    texcache_fetch(dapicnum, dapalnum, 0, (datype & 1)*DAMETH_CLAMPED);
    hicprecaching = 0;

    if (datype == 0 || !usemodels) return;

    mid = md_tilehasmodel(dapicnum,dapalnum);
    if (mid < 0 || models[mid]->mdnum < 2) return;

    {
        int32_t i,j=0;

        if (models[mid]->mdnum == 3)
            j = ((md3model_t *)models[mid])->head.numsurfs;

        for (i=0; i<=j; i++)
            mdloadskin((md2model_t *)models[mid], 0, dapalnum, i);
    }
#endif
}

#else /* if !defined USE_OPENGL */

#include "compat.h"

int32_t polymost_drawtilescreen(int32_t tilex, int32_t tiley, int32_t wallnum, int32_t dimen,
                                int32_t usehitile, uint8_t *loadedhitile)
{
    UNREFERENCED_PARAMETER(tilex);
    UNREFERENCED_PARAMETER(tiley);
    UNREFERENCED_PARAMETER(wallnum);
    UNREFERENCED_PARAMETER(dimen);
    UNREFERENCED_PARAMETER(usehitile);
    UNREFERENCED_PARAMETER(loadedhitile);
    return -1;
}

#endif

// vim:ts=4:sw=4:
