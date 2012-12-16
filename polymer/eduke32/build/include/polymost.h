#ifndef _polymost_h_
# define _polymost_h_

#ifdef USE_OPENGL

#include "hightile.h"

#define MODEL_OCCLUSION_CHECKING

#ifdef MODEL_OCCLUSION_CHECKING
# define CULL_OFFSET 384
# define CULL_DELAY 2
# define MAXCULLCHECKS 1024

extern int32_t lastcullcheck;
extern char cullmodel[MAXSPRITES];
extern int32_t cullcheckcnt;
#endif

extern char TEXCACHEFILE[BMAX_PATH];

typedef struct { char r, g, b, a; } coltype;

extern int32_t rendmode;
extern float gtang;
extern float glox1, gloy1;
extern double gxyaspect, grhalfxdown10x;
extern double gcosang, gsinang, gcosang2, gsinang2;
extern double gchang, gshang, gctang, gstang, gvisibility;

struct glfiltermodes {
	const char *name;
	int32_t min,mag;
};
#define NUMGLFILTERMODES 6
extern struct glfiltermodes glfiltermodes[NUMGLFILTERMODES];

//void phex(char v, char *s);
void uploadtexture(int32_t doalloc, int32_t xsiz, int32_t ysiz, int32_t intexfmt, int32_t texfmt, coltype *pic, int32_t tsizx, int32_t tsizy, int32_t dameth);
void polymost_drawsprite(int32_t snum);
void polymost_drawmaskwall(int32_t damaskwallcnt);
void polymost_dorotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                             int8_t dashade, char dapalnum, int32_t dastat, int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2, int32_t uniqid);
void polymost_fillpolygon(int32_t npoints);
void polymost_initosdfuncs(void);
void polymost_drawrooms(void);

void polymost_glinit(void);
void polymost_glreset(void);
void polymost_cachesync(void);

void gltexinvalidate(int32_t dapicnum, int32_t dapalnum, int32_t dameth);
void gltexinvalidateall(void);
void gltexinvalidate8(void);
int32_t polymost_printext256(int32_t xpos, int32_t ypos, int16_t col, int16_t backcol, const char *name, char fontsize);

extern float curpolygonoffset;

extern int32_t cachefilehandle;
extern FILE *cacheindexptr;
extern hashtable_t h_texcache;
extern uint8_t *memcachedata;
extern int32_t memcachesize;
extern int32_t cachepos;

struct cacheitem_t
{
    char name[BMAX_PATH];
    int32_t offset;
    int32_t len;
    struct cacheitem_t *next;
};

typedef struct cacheitem_t texcacheindex;

#define TEXCACHEMAGIC "QLZ1"

extern texcacheindex *cacheptrs[MAXTILES<<1];

int32_t dxtfilter(int32_t fil, const texcachepicture *pict, const char *pic, void *midbuf, char *packbuf, uint32_t miplen);
int32_t dedxtfilter(int32_t fil, const texcachepicture *pict, char *pic, void *midbuf, char *packbuf, int32_t ispacked);

void writexcache(const char *fn, int32_t len, int32_t dameth, char effect, texcacheheader *head);
int32_t polymost_trytexcache(const char *fn, int32_t len, int32_t dameth, char effect,
                             texcacheheader *head, int32_t modelp);

extern float shadescale;
extern int32_t shadescale_unbounded;
extern float alphahackarray[MAXTILES];

extern int32_t r_usenewshading;

static inline float getshadefactor(int32_t shade)
{
    int32_t shadebound = (shadescale_unbounded || shade>=numshades) ? numshades : numshades-1;
    float clamped_shade = min(max(shade*shadescale, 0), shadebound);
    return ((float)(numshades-clamped_shade))/(float)numshades;
}

typedef struct pthtyp_t
{
    struct pthtyp_t *next;
    uint32_t glpic;
    int16_t picnum;
    char palnum;
    char effects;
    char flags;      // 1 = clamped (dameth&4), 2 = hightile, 4 = skybox face, 8 = hasalpha, 16 = hasfullbright, 128 = invalidated
    char skyface;
    hicreplctyp *hicr;

    uint16_t sizx, sizy;
    float scalex, scaley;
    struct pthtyp_t *ofb; // only fullbright
} pthtyp;

pthtyp *gltexcache(int32_t dapicnum, int32_t dapalnum, int32_t dameth);

extern int32_t globalnoeffect;
extern int32_t drawingskybox;

extern double gyxscale, gxyaspect, ghalfx, grhalfxdown10;

// For GL_EXP2 fog:
#define FOGSCALE 0.0000768
// For GL_LINEAR fog:
#define FOGDISTCONST 150
#define FULLVIS_BEGIN 2.9e38
#define FULLVIS_END 3.0e38

extern char nofog;  // in windows/SDL layers
extern float fogresult, fogresult2, fogcol[4], fogtable[4*MAXPALOOKUPS];
extern int32_t g_visibility;

static inline void fogcalc(int32_t shade, int32_t vis, int32_t pal)
{
    Bmemcpy(fogcol, &fogtable[pal<<2], sizeof(fogcol));

    if (r_usenewshading==2)
    {
        float combvis = (float)(g_visibility * (uint8_t)(vis+16));

        bglFogi(GL_FOG_MODE, GL_LINEAR);

        if (combvis == 0)
        {
            fogresult = FULLVIS_BEGIN;
            fogresult2 = FULLVIS_END;
            return;
        }

        fogresult = -(FOGDISTCONST * shade)/combvis;
        fogresult2 = (FOGDISTCONST * (numshades-1-shade))/combvis;
    }
    else
    {
        float f;

        bglFogi(GL_FOG_MODE, GL_EXP2);

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
}

static inline void calc_and_apply_fog(int32_t shade, int32_t vis, int32_t pal)
{
    if (!nofog)
    {
        fogcalc(shade, vis, pal);
        bglFogfv(GL_FOG_COLOR, fogcol);

        if (r_usenewshading==2)
        {
            bglFogf(GL_FOG_START, fogresult);
            bglFogf(GL_FOG_END, fogresult2);
        }
        else
        {
            bglFogf(GL_FOG_DENSITY, fogresult);
        }
    }
}

static inline void calc_and_apply_fog_factor(int32_t shade, int32_t vis, int32_t pal, float factor)
{
    if (!nofog)
    {
        fogcalc(shade, vis, pal);
        bglFogfv(GL_FOG_COLOR, fogcol);

        if (r_usenewshading==2)
        {
            bglFogf(GL_FOG_START, FULLVIS_BEGIN);
            bglFogf(GL_FOG_END, FULLVIS_END);
        }
        else
        {
            bglFogf(GL_FOG_DENSITY, fogresult*factor);
        }
    }
}
#endif

#endif
