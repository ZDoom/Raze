#ifndef _polymost_h_
# define _polymost_h_

#ifdef USE_OPENGL

#include "hightile.h"

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
                             int8_t dashade, char dapalnum, int32_t dastat, uint8_t daalpha, int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2, int32_t uniqid);
void polymost_fillpolygon(int32_t npoints);
void polymost_initosdfuncs(void);
void polymost_drawrooms(void);

void polymost_glinit(void);
void polymost_glreset(void);

void gltexinvalidate(int32_t dapicnum, int32_t dapalnum, int32_t dameth);
void gltexinvalidateall(int32_t artonly);
int32_t polymost_printext256(int32_t xpos, int32_t ypos, int16_t col, int16_t backcol, const char *name, char fontsize);

extern float curpolygonoffset;

extern float shadescale;
extern int32_t shadescale_unbounded;
extern float alphahackarray[MAXTILES];

extern int32_t r_usenewshading;
extern int32_t r_usetileshades;

extern int16_t globalpicnum;
extern int32_t globalpal;

static inline float getshadefactor(int32_t shade)
{
    int32_t shadebound = (shadescale_unbounded || shade>=numshades) ? numshades : numshades-1;
    float clamped_shade = min(max(shade*shadescale, 0), shadebound);
    if (rendmode == REND_POLYMOST && r_usetileshades && (!usehightile || !hicfindsubst(globalpicnum, globalpal, 0))) return 1.f;
    return ((float)(numshades-clamped_shade))/(float)numshades;
}

typedef struct pthtyp_t
{
    struct pthtyp_t *next;
    uint32_t glpic;
    int16_t picnum;
    char palnum;
    char shade;
    char effects;
    char flags;      // 1 = clamped (dameth&4), 2 = hightile, 4 = skybox face, 8 = hasalpha, 16 = hasfullbright, 128 = invalidated
    char skyface;
    hicreplctyp *hicr;

    uint16_t sizx, sizy;
    float scalex, scaley;
    struct pthtyp_t *ofb; // only fullbright
} pthtyp;

extern int32_t gloadtile_art(int32_t,int32_t,int32_t,int32_t,pthtyp *,int32_t);
extern int32_t gloadtile_hi(int32_t,int32_t,int32_t,hicreplctyp *,int32_t,pthtyp *,int32_t,char);

extern int32_t globalnoeffect;
extern int32_t drawingskybox;
extern int32_t hicprecaching;
extern double gyxscale, gxyaspect, ghalfx, grhalfxdown10;

extern char ptempbuf[MAXWALLSB<<1];

#include "texcache.h"

#endif

#endif
