#ifndef _polymost_h_
# define _polymost_h_

#ifdef POLYMOST

#include "hightile.h"

#define CULL_OFFSET 384
#define CULL_DELAY 2
#define MAXCULLCHECKS 1024

extern int32_t lastcullcheck;
extern char cullmodel[MAXSPRITES];
extern int32_t cullcheckcnt;

#define PI 3.14159265358979323
extern char TEXCACHEFILE[BMAX_PATH];

typedef struct { char r, g, b, a; } coltype;

extern int32_t rendmode;
extern float gtang;
extern float glox1, gloy1;
extern double gxyaspect, grhalfxdown10x;
extern double gcosang, gsinang, gcosang2, gsinang2;
extern double gchang, gshang, gctang, gstang, gvisibility;

struct glfiltermodes {
	char *name;
	int32_t min,mag;
};
#define numglfiltermodes 6
extern struct glfiltermodes glfiltermodes[numglfiltermodes];

extern const char *TEXCACHEDIR;
void phex(char v, char *s);
void uploadtexture(int32_t doalloc, int32_t xsiz, int32_t ysiz, int32_t intexfmt, int32_t texfmt, coltype *pic, int32_t tsizx, int32_t tsizy, int32_t dameth);
void polymost_drawsprite(int32_t snum);
void polymost_drawmaskwall(int32_t damaskwallcnt);
void polymost_dorotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                             int8_t dashade, char dapalnum, char dastat, int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2, int32_t uniqid);
void polymost_fillpolygon(int32_t npoints);
void polymost_initosdfuncs(void);
void polymost_drawrooms(void);

void polymost_glinit(void);
void polymost_glreset(void);

void gltexinvalidate(int32_t dapicnum, int32_t dapalnum, int32_t dameth);
void gltexinvalidateall(void);
void gltexinvalidate8(void);
int32_t polymost_printext256(int32_t xpos, int32_t ypos, int16_t col, int16_t backcol, char *name, char fontsize);

extern float curpolygonoffset;

/*
// Depth peeling control
extern int32_t r_curpeel;
extern int32_t peelcompiling;

// Depth peeling data
extern GLuint ztexture[3];
extern GLuint *peels;
extern GLuint *peelfbos;
extern GLuint peelprogram[2];
*/

extern int32_t cachefilehandle;
extern FILE *cacheindexptr;
extern hashtable_t cacheH;

struct cacheitem_t
{
    char name[BMAX_PATH];
    int32_t offset;
    int32_t len;
    struct cacheitem_t *next;
};

typedef struct cacheitem_t texcacheindex;

#define TEXCACHEMAGIC "QLZ1"

extern texcacheindex *firstcacheindex;
extern texcacheindex *curcacheindex;
extern texcacheindex *cacheptrs[MAXTILES<<1];
extern int32_t numcacheentries;

int32_t dxtfilter(int32_t fil, texcachepicture *pict, char *pic, void *midbuf, char *packbuf, uint32_t miplen);
int32_t dedxtfilter(int32_t fil, texcachepicture *pict, char *pic, void *midbuf, char *packbuf, int32_t ispacked);

void writexcache(char *fn, int32_t len, int32_t dameth, char effect, texcacheheader *head);

extern float shadescale;
extern float alphahackarray[MAXTILES];

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

pthtyp * gltexcache (int32_t dapicnum, int32_t dapalnum, int32_t dameth);

extern palette_t hictinting[MAXPALOOKUPS];
extern float     gtang;
extern int32_t globalposx, globalposy, globalposz, globalhoriz;
extern int16_t globalang, globalcursectnum;
extern int32_t globalpal, cosglobalang, singlobalang;
extern int32_t cosviewingrangeglobalang, sinviewingrangeglobalang;
extern float shadescale;
extern int32_t globalnoeffect;
extern int32_t drawingskybox;

extern double gyxscale, gxyaspect, gviewxrange, ghalfx, grhalfxdown10, grhalfxdown10x, ghoriz;
extern double gcosang, gsinang, gcosang2, gsinang2;
extern double gchang, gshang, gctang, gstang, gvisibility;

#define FOGSCALE 0.0000768

extern float fogresult, fogcol[4], fogtable[4*MAXPALOOKUPS];

static inline void fogcalc(const int32_t shade, const int32_t vis, const int32_t pal)
{
    float f = shade * 1.75f;

    if (vis > 239) f = (float)(gvisibility*((vis-240+f)/(klabs(vis-256))));
    else f = (float)(gvisibility*(vis+16+f));

    fogresult = clamp(f, 0.01f, 100.f);

    Bmemcpy(fogcol, &fogtable[pal<<2], sizeof(fogcol));
}


#endif

#endif
