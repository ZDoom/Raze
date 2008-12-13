#ifndef _polymost_h_
# define _polymost_h_

#ifdef POLYMOST

#define CULL_OFFSET 384
#define CULL_DELAY 2
#define MAXCULLCHECKS 1024

extern int lastcullcheck;
extern char cullmodel[MAXSPRITES];
extern int cullcheckcnt;

#define PI 3.14159265358979323

typedef struct { unsigned char r, g, b, a; } coltype;

extern int rendmode;
extern float gtang;
extern float glox1, gloy1;
extern double gxyaspect, grhalfxdown10x;
extern double gcosang, gsinang, gcosang2, gsinang2;
extern double gchang, gshang, gctang, gstang;

struct glfiltermodes {
	char *name;
	int min,mag;
};
#define numglfiltermodes 6
extern struct glfiltermodes glfiltermodes[numglfiltermodes];

extern const char *TEXCACHEDIR;
void phex(unsigned char v, char *s);
void uploadtexture(int doalloc, int xsiz, int ysiz, int intexfmt, int texfmt, coltype *pic, int tsizx, int tsizy, int dameth);
void polymost_drawsprite(int snum);
void polymost_drawmaskwall(int damaskwallcnt);
void polymost_dorotatesprite(int sx, int sy, int z, short a, short picnum,
                             signed char dashade, char dapalnum, char dastat, int cx1, int cy1, int cx2, int cy2, int uniqid);
void polymost_fillpolygon(int npoints);
void polymost_initosdfuncs(void);
void polymost_drawrooms(void);

void polymost_glinit(void);
void polymost_glreset(void);

void gltexinvalidate(int dapicnum, int dapalnum, int dameth);
void gltexinvalidateall(void);
void gltexinvalidate8(void);
int polymost_printext256(int xpos, int ypos, short col, short backcol, char *name, char fontsize);

// Depth peeling control
extern int r_curpeel;
extern float curpolygonoffset;
extern int peelcompiling;

// Depth peeling data
extern GLuint ztexture[3];
extern GLuint *peels;
extern GLuint *peelfbos;
extern GLuint peelprogram[2];

extern int cachefilehandle;
extern FILE *cacheindexptr;
extern struct HASH_table cacheH;

struct cacheitem_t
{
    char name[BMAX_PATH];
    int offset;
    int len;
    struct cacheitem_t *next;
};

typedef struct cacheitem_t texcacheindex;

extern texcacheindex firstcacheindex;
extern texcacheindex *curcacheindex;
extern texcacheindex *cacheptrs[MAXTILES<<2];
extern int numcacheentries;

int dxtfilter(int fil, texcachepicture *pict, char *pic, void *midbuf, char *packbuf, unsigned int miplen);
int dedxtfilter(int fil, texcachepicture *pict, char *pic, void *midbuf, char *packbuf, int ispacked);

void writexcache(char *fn, int len, int dameth, char effect, texcacheheader *head);

extern float shadescale;
extern float alphahackarray[MAXTILES];

typedef struct pthtyp_t
{
    struct pthtyp_t *next;
    unsigned int glpic;
    short picnum;
    char palnum;
    char effects;
    char flags;      // 1 = clamped (dameth&4), 2 = hightile, 4 = skybox face, 8 = hasalpha, 16 = hasfullbright, 128 = invalidated
    char skyface;
    hicreplctyp *hicr;

    unsigned short sizx, sizy;
    float scalex, scaley;
    struct pthtyp_t *ofb; // only fullbright

    char *palmap;int size;
} pthtyp;

pthtyp * gltexcache (int dapicnum, int dapalnum, int dameth);

extern palette_t hictinting[MAXPALOOKUPS];
extern float     gtang;
extern int globalposx, globalposy, globalposz, globalhoriz;
extern short globalang, globalcursectnum;
extern int globalpal, cosglobalang, singlobalang;
extern int cosviewingrangeglobalang, sinviewingrangeglobalang;
extern float shadescale;
extern int globalnoeffect;
extern int drawingskybox;

#endif

#endif
