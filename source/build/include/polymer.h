// Here lies the slow as shit renderer!

#ifndef polymer_h_
# define polymer_h_

# include "compat.h"
# include "baselayer.h"
# include "glbuild.h"
# include "build.h"
# include "osd.h"
# include "hightile.h"
# include "mdsprite.h"
# include "polymost.h"
# include "pragmas.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PR_LINEAR_FOG

// CVARS
extern int32_t      pr_lighting;
extern int32_t      pr_normalmapping;
extern int32_t      pr_specularmapping;
extern int32_t      pr_shadows;
extern int32_t      pr_shadowcount;
extern int32_t      pr_shadowdetail;
extern int32_t      pr_shadowfiltering;
extern int32_t      pr_maxlightpasses;
extern int32_t      pr_maxlightpriority;
extern int32_t      pr_fov;
extern double       pr_customaspect;
extern int32_t      pr_billboardingmode;
extern int32_t      pr_verbosity;
extern int32_t      pr_wireframe;
extern int32_t      pr_vbos;
extern int32_t      pr_buckets;
extern int32_t      pr_gpusmoothing;
extern int32_t      pr_overrideparallax;
extern float        pr_parallaxscale;
extern float        pr_parallaxbias;
extern int32_t      pr_overridespecular;
extern float        pr_specularpower;
extern float        pr_specularfactor;
extern int32_t      pr_highpalookups;
extern int32_t      pr_artmapping;
extern int32_t      pr_overridehud;
extern float        pr_hudxadd;
extern float        pr_hudyadd;
extern float        pr_hudzadd;
extern int32_t      pr_hudangadd;
extern int32_t      pr_hudfov;
extern float        pr_overridemodelscale;
extern int32_t      pr_ati_fboworkaround;
extern int32_t      pr_ati_nodepthoffset;
#ifdef __APPLE__
extern int32_t      pr_ati_textureformat_one;
#endif
extern int32_t      pr_nullrender;

extern int32_t      r_pr_maxlightpasses;

// MATERIAL
typedef enum {
                    PR_BIT_HEADER,              // must be first
                    PR_BIT_ANIM_INTERPOLATION,
                    PR_BIT_LIGHTING_PASS,
                    PR_BIT_NORMAL_MAP,
                    PR_BIT_ART_MAP,
                    PR_BIT_DIFFUSE_MAP,
                    PR_BIT_DIFFUSE_DETAIL_MAP,
                    PR_BIT_DIFFUSE_MODULATION,
                    PR_BIT_DIFFUSE_MAP2,
                    PR_BIT_HIGHPALOOKUP_MAP,
                    PR_BIT_SPECULAR_MAP,
                    PR_BIT_SPECULAR_MATERIAL,
                    PR_BIT_MIRROR_MAP,
                    PR_BIT_FOG,
                    PR_BIT_GLOW_MAP,
                    PR_BIT_PROJECTION_MAP,
                    PR_BIT_SHADOW_MAP,
                    PR_BIT_LIGHT_MAP,
                    PR_BIT_SPOT_LIGHT,
                    PR_BIT_POINT_LIGHT,
                    PR_BIT_FOOTER,              // must be just before last
                    PR_BIT_COUNT                // must be last
}                   prbittype;

typedef struct      s_prmaterial {
    // PR_BIT_ANIM_INTERPOLATION
    GLfloat         frameprogress;
    GLfloat*        nextframedata;
    // PR_BIT_NORMAL_MAP
    GLuint          normalmap;
    GLfloat         normalbias[2];
    GLfloat*        tbn;
    // PR_BIT_ART_MAP
    GLuint          artmap;
    GLuint          basepalmap;
    GLuint          lookupmap;
    GLint           shadeoffset;
    GLfloat         visibility;
    // PR_BIT_DIFFUSE_MAP
    GLuint          diffusemap;
    GLfloat         diffusescale[2];
    // PR_BIT_HIGHPALOOKUP_MAP
    GLuint          highpalookupmap;
    // PR_BIT_DIFFUSE_DETAIL_MAP
    GLuint          detailmap;
    GLfloat         detailscale[2];
    // PR_BIT_DIFFUSE_MODULATION
    GLubyte         diffusemodulation[4];
    // PR_BIT_SPECULAR_MAP
    GLuint          specmap;
    // PR_BIT_SPECULAR_MATERIAL
    GLfloat         specmaterial[2];
    // PR_BIT_MIRROR_MAP
    GLuint          mirrormap;
    // PR_BIT_GLOW_MAP
    GLuint          glowmap;
    // PR_BIT_SHADOW_MAP
    GLboolean       mdspritespace;
}                   _prmaterial;

typedef struct      s_prrograminfo {
    GLhandleARB     handle;
    // PR_BIT_ANIM_INTERPOLATION
    GLint           attrib_nextFrameData;
    GLint           attrib_nextFrameNormal;
    GLint           uniform_frameProgress;
    // PR_BIT_NORMAL_MAP
    GLint           attrib_T;
    GLint           attrib_B;
    GLint           attrib_N;
    GLint           uniform_eyePosition;
    GLint           uniform_normalMap;
    GLint           uniform_normalBias;
    // PR_BIT_ART_MAP
    GLuint          uniform_artMap;
    GLuint          uniform_basePalMap;
    GLuint          uniform_lookupMap;
    GLuint          uniform_shadeOffset;
    GLuint          uniform_visibility;
    // PR_BIT_DIFFUSE_MAP
    GLint           uniform_diffuseMap;
    GLint           uniform_diffuseScale;
    // PR_BIT_HIGHPALOOKUP_MAP
    GLuint          uniform_highPalookupMap;
    // PR_BIT_DIFFUSE_DETAIL_MAP
    GLint           uniform_detailMap;
    GLint           uniform_detailScale;
    // PR_BIT_SPECULAR_MAP
    GLint           uniform_specMap;
    // PR_BIT_SPECULAR_MATERIAL
    GLint           uniform_specMaterial;
    // PR_BIT_MIRROR_MAP
    GLint           uniform_mirrorMap;
#ifdef PR_LINEAR_FOG
    // PR_BIT_FOG
    GLint           uniform_linearFog;
#endif
    // PR_BIT_GLOW_MAP
    GLint           uniform_glowMap;
    // PR_BIT_PROJECTION_MAP
    GLint           uniform_shadowProjMatrix;
    // PR_BIT_SHADOW_MAP
    GLint           uniform_shadowMap;
    // PR_BIT_LIGHT_MAP
    GLint           uniform_lightMap;
    // PR_BIT_SPOT_LIGHT
    GLint           uniform_spotDir;
    GLint           uniform_spotRadius;
}                   _prprograminfo;

#define             PR_INFO_LOG_BUFFER_SIZE 8192

// Think about changing highPal[Scale|Bias] in the program bit if you change this
#define             PR_HIGHPALOOKUP_BIT_DEPTH 6
#define             PR_HIGHPALOOKUP_DIM (1 << PR_HIGHPALOOKUP_BIT_DEPTH)
#define             PR_HIGHPALOOKUP_DATA_SIZE (4 * PR_HIGHPALOOKUP_DIM * \
                                                   PR_HIGHPALOOKUP_DIM * \
                                                   PR_HIGHPALOOKUP_DIM)

typedef struct      s_prprogrambit {
    int32_t         bit;
    const char*           vert_def;
    const char*           vert_prog;
    const char*           frag_def;
    const char*           frag_prog;
}                   _prprogrambit;

typedef struct      s_prbucket {
    // index
    int16_t         tilenum;
    char            pal;

    _prmaterial     material;
    int32_t         invalidmaterial;

    // geom indices
    GLuint*          indices;
    uint32_t         count;
    uint32_t         buffersize;
    GLuint*          indiceoffset;

    struct s_prbucket* next;
}                   _prbucket;

#include "prlights.h"

// RENDER TARGETS
typedef struct      s_prrt {
    GLenum          target;
    GLuint          color;
    GLuint          z;
    GLuint          fbo;
    int32_t         xdim, ydim;
}                   _prrt;

// BUILD DATA

typedef struct      s_prvert {
    GLfloat         x;
    GLfloat         y;
    GLfloat         z;
    GLfloat         u;
    GLfloat         v;
    GLubyte         r;
    GLubyte         g;
    GLubyte         b;
    GLubyte         a;
}                   _prvert;

typedef struct      s_prplane {
    // geometry
    _prvert*        buffer;
    int32_t         vertcount;
    GLuint          vbo;
    uint32_t        mapvbo_vertoffset;
    _prbucket*      bucket;
    // attributes
    GLfloat         tbn[3][3];
    GLfloat         plane[4];
    _prmaterial     material;
    // elements
    GLushort*       indices;
    int32_t         indicescount;
    GLuint          ivbo;
    // lights
    int16_t         lights[PR_MAXLIGHTS];
    uint16_t        lightcount;
}                   _prplane;

typedef struct      s_prsector {
    // polymer data
    GLdouble*       verts;
    _prplane        floor;
    _prplane        ceil;
    int16_t         curindice;
    int32_t         indicescount;
    int32_t         oldindicescount;
    // stuff
    float           wallsproffset;
    float           floorsproffset;
    // build sector data
    int32_t         ceilingz, floorz;
    uint16_t        ceilingstat, floorstat;
    int16_t         ceilingpicnum, ceilingheinum;
    int8_t          ceilingshade;
    uint8_t         ceilingpal, ceilingxpanning, ceilingypanning;
    int16_t         floorpicnum, floorheinum;
    int8_t          floorshade;
    uint8_t         floorpal, floorxpanning, floorypanning;
    uint8_t         visibility;

    int16_t         floorpicnum_anim, ceilingpicnum_anim;

    struct          {
        int32_t     empty       : 1;
        int32_t     uptodate    : 1;
        int32_t     invalidtex  : 1;
    }               flags;
    uint32_t        invalidid;
    uint32_t        trackedrev;
}                   _prsector;

typedef struct      s_prwall {
    _prplane        wall;
    _prplane        over;
    _prplane        mask;

    // stuff
    GLfloat*        bigportal;
    //GLfloat*        cap;
    GLuint          stuffvbo;

    // build wall data
    uint16_t        cstat;
    int16_t         picnum, overpicnum;
    int8_t          shade;
    uint8_t         pal, xrepeat, yrepeat, xpanning, ypanning;

    // nextwall data
    int16_t         nwallpicnum, nwallcstat;
    int8_t          nwallxpanning, nwallypanning;
    int8_t          nwallshade;

    int16_t         picnum_anim, overpicnum_anim;

    char            underover;
    uint32_t        invalidid;
    struct          {
        int32_t     empty       : 1;
        int32_t     uptodate    : 1;
        int32_t     invalidtex  : 1;
    }               flags;
    uint32_t        trackedrev;
}                   _prwall;

typedef struct      s_prsprite {
    _prplane        plane;
    uint32_t        hash;
}                   _prsprite;

typedef struct      s_prmirror {
    _prplane        *plane;
    int16_t         sectnum;
    int16_t         wallnum;
}                   _prmirror;

typedef struct      s_prhighpalookup {
    char            *data;
    GLuint          map;
}                   _prhighpalookup;

typedef void    (*animatespritesptr)(int32_t, int32_t, int32_t, int32_t);

typedef struct      s_pranimatespritesinfo {
    animatespritesptr animatesprites;
    int32_t         x, y, a, smoothratio;
}                   _pranimatespritesinfo;

// this one has to be provided by the application
extern void G_Polymer_UnInit(void);

// EXTERNAL FUNCTIONS
int32_t             polymer_init(void);
void                polymer_uninit(void);
void                polymer_setaspect(int32_t);
void                polymer_glinit(void);
void                polymer_resetlights(void);
void                polymer_loadboard(void);
void                polymer_drawrooms(int32_t daposx, int32_t daposy, int32_t daposz, int16_t daang, int32_t dahoriz, int16_t dacursectnum);
void                polymer_drawmasks(void);
void                polymer_editorpick(void);
void                polymer_inb4rotatesprite(int16_t tilenum, char pal, int8_t shade, int32_t method);
void                polymer_postrotatesprite(void);
void                polymer_drawmaskwall(int32_t damaskwallcnt);
void                polymer_drawsprite(int32_t snum);
void                polymer_setanimatesprites(animatespritesptr animatesprites, int32_t x, int32_t y, int32_t a, int32_t smoothratio);
int16_t             polymer_addlight(_prlight* light);
void                polymer_deletelight(int16_t lighti);
void                polymer_invalidatelights(void);
void                polymer_texinvalidate(void);
void                polymer_definehighpalookup(char basepalnum, char palnum, char *fn);
int32_t             polymer_havehighpalookup(int32_t basepalnum, int32_t palnum);


extern _prsprite    *prsprites[MAXSPRITES];
static inline void polymer_invalidatesprite(int32_t i)
{
    if (prsprites[i])
        prsprites[i]->hash = 0xDEADBEEF;
}

extern GLuint       prartmaps[MAXTILES];
static inline void polymer_invalidateartmap(int32_t tilenum)
{
    if (prartmaps[tilenum])
    {
        bglDeleteTextures(1, &prartmaps[tilenum]);
        prartmaps[tilenum] = 0;
    }
}

// Compare with eligible_for_tileshades()
static inline int32_t polymer_eligible_for_artmap(int32_t tilenum, const pthtyp *pth)
{
    return ((!pth || !pth->hicr) && tilenum < (MAXTILES - 4));
}

# ifdef POLYMER_C

// CORE
static void         polymer_displayrooms(int16_t sectnum);
static void         polymer_emptybuckets(void);
static _prbucket*   polymer_findbucket(int16_t tilenum, char pal);
static void         polymer_bucketplane(_prplane* plane);
static void         polymer_drawplane(_prplane* plane);
static inline void  polymer_inb4mirror(_prvert* buffer, GLfloat* plane);
static void         polymer_animatesprites(void);
static void         polymer_freeboard(void);
// SECTORS
static int32_t      polymer_initsector(int16_t sectnum);
static int32_t      polymer_updatesector(int16_t sectnum);
void PR_CALLBACK    polymer_tesserror(GLenum error);
void PR_CALLBACK    polymer_tessedgeflag(GLenum error);
void PR_CALLBACK    polymer_tessvertex(void* vertex, void* sector);
static int32_t      polymer_buildfloor(int16_t sectnum);
static void         polymer_drawsector(int16_t sectnum, int32_t domasks);
// WALLS
static int32_t      polymer_initwall(int16_t wallnum);
static void         polymer_updatewall(int16_t wallnum);
static void         polymer_drawwall(int16_t sectnum, int16_t wallnum);
// HSR
static void         polymer_computeplane(_prplane* p);
static inline void  polymer_crossproduct(GLfloat* in_a, GLfloat* in_b, GLfloat* out);
static inline void  polymer_transformpoint(const float* inpos, float* pos, float* matrix);
static inline void  polymer_normalize(float* vec);
static inline void  polymer_pokesector(int16_t sectnum);
static void         polymer_extractfrustum(GLfloat* modelview, GLfloat* projection, float* frustum);
static inline int32_t polymer_planeinfrustum(_prplane *plane, float* frustum);
static inline void  polymer_scansprites(int16_t sectnum, uspritetype* tsprite, int32_t* spritesortcnt);
static void         polymer_updatesprite(int32_t snum);
// SKIES
static void         polymer_getsky(void);
static void         polymer_drawsky(int16_t tilenum, char palnum, int8_t shade);
static void         polymer_initartsky(void);
static void         polymer_drawartsky(int16_t tilenum, char palnum, int8_t shade);
static void         polymer_drawartskyquad(int32_t p1, int32_t p2, GLfloat height);
static void         polymer_drawskybox(int16_t tilenum, char palnum, int8_t shade);
// MDSPRITES
static void         polymer_drawmdsprite(uspritetype *tspr);
static void         polymer_loadmodelvbos(md3model_t* m);
// MATERIALS
static void         polymer_getscratchmaterial(_prmaterial* material);
static _prbucket*   polymer_getbuildmaterial(_prmaterial* material, int16_t tilenum, char pal, int8_t shade, int8_t vis, int32_t cmeth);
static int32_t      polymer_bindmaterial(const _prmaterial *material, int16_t* lights, int lightcount);
static void         polymer_unbindmaterial(int32_t programbits);
static void         polymer_compileprogram(int32_t programbits);
// LIGHTS
static void         polymer_removelight(int16_t lighti);
static void         polymer_updatelights(void);
static inline void  polymer_resetplanelights(_prplane* plane);
static void         polymer_addplanelight(_prplane* plane, int16_t lighti);
static inline void  polymer_deleteplanelight(_prplane* plane, int16_t lighti);
static int32_t      polymer_planeinlight(_prplane* plane, _prlight* light);
static void         polymer_invalidateplanelights(_prplane* plane);
static void         polymer_invalidatesectorlights(int16_t sectnum);
static void         polymer_processspotlight(_prlight* light);
static inline void  polymer_culllight(int16_t lighti);
static void         polymer_prepareshadows(void);
// RENDER TARGETS
static void         polymer_initrendertargets(int32_t count);
// DEBUG OUTPUT
void PR_CALLBACK    polymer_debugoutputcallback(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,GLvoid *userParam);

#define INDICE(n) ((p->indices) ? (p->indices[(i+n)%p->indicescount]) : (((i+n)%p->vertcount)))

#define SWITCH_CULL_DIRECTION { culledface = (culledface == GL_FRONT) ? GL_BACK : GL_FRONT; bglCullFace(culledface); }

static inline GLfloat dot2f(GLfloat *v1, GLfloat *v2)
{
    return v1[0]*v2[0] + v1[1]*v2[1];
}

static inline GLfloat dot3f(GLfloat *v1, GLfloat *v2)
{
    return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

static inline void relvec2f(GLfloat *v1, GLfloat *v2, GLfloat *out)
{
    out[0] = v2[0]-v1[0];
    out[1] = v2[1]-v1[1];
}

// the following from gle/vvector.h

/* ========================================================== */
/* determinant of matrix
 *
 * Computes determinant of matrix m, returning d
 */

#define DETERMINANT_3X3(d,m)                    \
{                                \
   d = m[0][0] * (m[1][1]*m[2][2] - m[1][2] * m[2][1]);        \
   d -= m[0][1] * (m[1][0]*m[2][2] - m[1][2] * m[2][0]);    \
   d += m[0][2] * (m[1][0]*m[2][1] - m[1][1] * m[2][0]);    \
}

/* ========================================================== */
/* i,j,th cofactor of a 4x4 matrix
 *
 */

#define COFACTOR_4X4_IJ(fac,m,i,j)                 \
{                                \
   int ii[4], jj[4], k;                        \
                                \
   /* compute which row, columnt to skip */            \
   for (k=0; k<i; k++) ii[k] = k;                \
   for (k=i; k<3; k++) ii[k] = k+1;                \
   for (k=0; k<j; k++) jj[k] = k;                \
   for (k=j; k<3; k++) jj[k] = k+1;                \
                                \
   (fac) = m[ii[0]][jj[0]] * (m[ii[1]][jj[1]]*m[ii[2]][jj[2]]     \
                            - m[ii[1]][jj[2]]*m[ii[2]][jj[1]]); \
   (fac) -= m[ii[0]][jj[1]] * (m[ii[1]][jj[0]]*m[ii[2]][jj[2]]    \
                             - m[ii[1]][jj[2]]*m[ii[2]][jj[0]]);\
   (fac) += m[ii[0]][jj[2]] * (m[ii[1]][jj[0]]*m[ii[2]][jj[1]]    \
                             - m[ii[1]][jj[1]]*m[ii[2]][jj[0]]);\
                                \
   /* compute sign */                        \
   k = i+j;                            \
   if ( k != (k/2)*2) {                        \
      (fac) = -(fac);                        \
   }                                \
}

/* ========================================================== */
/* determinant of matrix
 *
 * Computes determinant of matrix m, returning d
 */

#define DETERMINANT_4X4(d,m)                    \
{                                \
   double cofac;                        \
   COFACTOR_4X4_IJ (cofac, m, 0, 0);                \
   d = m[0][0] * cofac;                        \
   COFACTOR_4X4_IJ (cofac, m, 0, 1);                \
   d += m[0][1] * cofac;                    \
   COFACTOR_4X4_IJ (cofac, m, 0, 2);                \
   d += m[0][2] * cofac;                    \
   COFACTOR_4X4_IJ (cofac, m, 0, 3);                \
   d += m[0][3] * cofac;                    \
}

/* ========================================================== */
/* compute adjoint of matrix and scale
 *
 * Computes adjoint of matrix m, scales it by s, returning a
 */

#define SCALE_ADJOINT_3X3(a,s,m)                \
{                                \
   a[0][0] = (s) * (m[1][1] * m[2][2] - m[1][2] * m[2][1]);    \
   a[1][0] = (s) * (m[1][2] * m[2][0] - m[1][0] * m[2][2]);    \
   a[2][0] = (s) * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);    \
                                \
   a[0][1] = (s) * (m[0][2] * m[2][1] - m[0][1] * m[2][2]);    \
   a[1][1] = (s) * (m[0][0] * m[2][2] - m[0][2] * m[2][0]);    \
   a[2][1] = (s) * (m[0][1] * m[2][0] - m[0][0] * m[2][1]);    \
                                \
   a[0][2] = (s) * (m[0][1] * m[1][2] - m[0][2] * m[1][1]);    \
   a[1][2] = (s) * (m[0][2] * m[1][0] - m[0][0] * m[1][2]);    \
   a[2][2] = (s) * (m[0][0] * m[1][1] - m[0][1] * m[1][0]);    \
}

/* ========================================================== */
/* compute adjoint of matrix and scale
 *
 * Computes adjoint of matrix m, scales it by s, returning a
 */

#define SCALE_ADJOINT_4X4(a,s,m)                \
{                                \
   int i,j;                            \
                                \
   for (i=0; i<4; i++) {                    \
      for (j=0; j<4; j++) {                    \
         COFACTOR_4X4_IJ (a[j][i], m, i, j);            \
         a[j][i] *= s;                        \
      }                                \
   }                                \
}

/* ========================================================== */
/* inverse of matrix
 *
 * Compute inverse of matrix a, returning determinant m and
 * inverse b
 */

#define INVERT_3X3(b,det,a)            \
{                        \
   double tmp;                    \
   DETERMINANT_3X3 (det, a);            \
   tmp = 1.0 / (det);                \
   SCALE_ADJOINT_3X3 (b, tmp, a);        \
}

/* ========================================================== */
/* inverse of matrix
 *
 * Compute inverse of matrix a, returning determinant m and
 * inverse b
 */

#define INVERT_4X4(b,det,a)            \
{                        \
   double tmp;                    \
   DETERMINANT_4X4 (det, a);            \
   tmp = 1.0 / (det);                \
   SCALE_ADJOINT_4X4 (b, tmp, a);        \
}

# endif // !POLYMER_C

#ifdef __cplusplus
}
#endif

#endif // !polymer_h_
