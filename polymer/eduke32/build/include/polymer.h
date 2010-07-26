// here lies the GREAT JUSTICE RENDERER
// TODO :
// - CORE STUFF
//   o there's also the texture alignment problem Hunter reported (san andreas fault)
//   o RTT portals (water)
//   o clip mirrors/portals to their planes
//   o merge mirrors/portals from the same plane
// - SPRITES
//   o sprite panning
// - SKIES
//   o skyview
// - MDSPRITES
//   o need full translation and rotation support from CON to attach to game world or tags
//
// the renderer should hopefully be pretty solid after all that
// the rest will be a bliss :)

#ifndef _polymer_h_
# define _polymer_h_

# include "compat.h"
# include "baselayer.h"
# include "build.h"
# include "glbuild.h"
# include "osd.h"
# include "hightile.h"
# include "mdsprite.h"
# include "polymost.h"
# include "pragmas.h"
# include <math.h>

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
extern float        pr_customaspect;
extern int32_t      pr_billboardingmode;
extern int32_t      pr_verbosity;
extern int32_t      pr_wireframe;
extern int32_t      pr_vbos;
extern int32_t      pr_gpusmoothing;
extern int32_t      pr_overrideparallax;
extern float        pr_parallaxscale;
extern float        pr_parallaxbias;
extern int32_t      pr_overridespecular;
extern float        pr_specularpower;
extern float        pr_specularfactor;
extern int32_t      pr_ati_fboworkaround;
extern int32_t      pr_ati_nodepthoffset;
#ifdef __APPLE__
extern int32_t      pr_ati_textureformat_one;
#endif

extern int32_t      r_pr_maxlightpasses;

// MATERIAL
typedef enum {
                    PR_BIT_HEADER,              // must be first
                    PR_BIT_ANIM_INTERPOLATION,
                    PR_BIT_LIGHTING_PASS,
                    PR_BIT_NORMAL_MAP,
                    PR_BIT_DIFFUSE_MAP,
                    PR_BIT_DIFFUSE_DETAIL_MAP,
                    PR_BIT_DIFFUSE_MODULATION,
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
    // PR_BIT_DIFFUSE_MAP
    GLuint          diffusemap;
    GLfloat         diffusescale[2];
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
    // PR_BIT_DIFFUSE_MAP
    GLint           uniform_diffuseMap;
    GLint           uniform_diffuseScale;
    // PR_BIT_DIFFUSE_DETAIL_MAP
    GLint           uniform_detailMap;
    GLint           uniform_detailScale;
    // PR_BIT_SPECULAR_MAP
    GLint           uniform_specMap;
    // PR_BIT_SPECULAR_MATERIAL
    GLint           uniform_specMaterial;
    // PR_BIT_MIRROR_MAP
    GLint           uniform_mirrorMap;
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

typedef struct      s_prprogrambit {
    int32_t         bit;
    char*           vert_def;
    char*           vert_prog;
    char*           frag_def;
    char*           frag_prog;
}                   _prprogrambit;

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
typedef struct      s_prplane {
    // geometry
    GLfloat*        buffer;
    int32_t         vertcount;
    GLuint          vbo;
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
    int16_t         ceilingstat, floorstat;
    int16_t         ceilingpicnum, ceilingheinum;
    int8_t          ceilingshade;
    char            ceilingpal, ceilingxpanning, ceilingypanning;
    int16_t         floorpicnum, floorheinum;
    int8_t          floorshade;
    char            floorpal, floorxpanning, floorypanning;

    struct          {
        int32_t     empty       : 1;
        int32_t     uptodate    : 1;
        int32_t     invalidtex  : 1;
    }               flags;
    uint32_t        invalidid;
}                   _prsector;

typedef struct      s_prwall {
    _prplane        wall;
    _prplane        over;
    _prplane        mask;
    // stuff
    GLfloat*        bigportal;
    GLfloat*        cap;
    GLuint          stuffvbo;
    // build wall data
    int16_t         cstat, nwallcstat;
    int16_t         picnum, overpicnum, nwallpicnum;
    int8_t          shade;
    char            pal, xrepeat, yrepeat, xpanning, ypanning;
    char            nwallxpanning, nwallypanning;

    char            underover;
    uint32_t        invalidid;
    struct          {
        int32_t     empty       : 1;
        int32_t     uptodate    : 1;
        int32_t     invalidtex  : 1;
    }               flags;
}                   _prwall;

typedef struct      s_prsprite {
    _prplane        plane;
    uint32_t        crc;
}                   _prsprite;

typedef struct      s_prmirror {
    _prplane        *plane;
    int16_t         sectnum;
    int16_t         wallnum;
}                   _prmirror;

typedef void    (*animatespritesptr)(int32_t, int32_t, int32_t, int32_t);

typedef struct      s_pranimatespritesinfo {
    animatespritesptr animatesprites;
    int32_t         x, y, a, smoothratio;
}                   _pranimatespritesinfo;

// EXTERNAL FUNCTIONS
int32_t             polymer_init(void);
void                polymer_uninit(void);
void                polymer_glinit(void);
void                polymer_resetlights(void);
void                polymer_loadboard(void);
void                polymer_drawrooms(int32_t daposx, int32_t daposy, int32_t daposz, int16_t daang, int32_t dahoriz, int16_t dacursectnum);
void                polymer_drawmasks(void);
void                polymer_editorpick(void);
void                polymer_rotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum, int8_t dashade, char dapalnum, int32_t dastat, int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2);
void                polymer_drawmaskwall(int32_t damaskwallcnt);
void                polymer_drawsprite(int32_t snum);
void                polymer_setanimatesprites(animatespritesptr animatesprites, int32_t x, int32_t y, int32_t a, int32_t smoothratio);
int16_t             polymer_addlight(_prlight* light);
void                polymer_deletelight(int16_t lighti);
void                polymer_invalidatelights(void);
void                polymer_texinvalidate(void);

# ifdef POLYMER_C

// CORE
static void         polymer_displayrooms(int16_t sectnum);
static void         polymer_drawplane(_prplane* plane);
static inline void  polymer_inb4mirror(GLfloat* buffer, GLfloat* plane);
static void         polymer_animatesprites(void);
static void         polymer_freeboard(void);
// SECTORS
static int32_t      polymer_initsector(int16_t sectnum);
static int32_t      polymer_updatesector(int16_t sectnum);
void PR_CALLBACK    polymer_tesserror(GLenum error);
void PR_CALLBACK    polymer_tessedgeflag(GLenum error);
void PR_CALLBACK    polymer_tessvertex(void* vertex, void* sector);
static int32_t      polymer_buildfloor(int16_t sectnum);
static void         polymer_drawsector(int16_t sectnum);
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
static inline void  polymer_scansprites(int16_t sectnum, spritetype* tsprite, int32_t* spritesortcnt);
static inline void  polymer_updatesprite(int32_t snum);
// SKIES
static void         polymer_getsky(void);
static void         polymer_drawsky(int16_t tilenum, char palnum, int8_t shade);
static void         polymer_initartsky(void);
static void         polymer_drawartsky(int16_t tilenum, char palnum, int8_t shade);
static void         polymer_drawartskyquad(int32_t p1, int32_t p2, GLfloat height);
static void         polymer_drawskybox(int16_t tilenum, char palnum, int8_t shade);
// MDSPRITES
static void         polymer_drawmdsprite(spritetype *tspr);
static void         polymer_loadmodelvbos(md3model_t* m);
// MATERIALS
static void         polymer_getscratchmaterial(_prmaterial* material);
static void         polymer_getbuildmaterial(_prmaterial* material, int16_t tilenum, char pal, int8_t shade, int32_t cmeth);
static int32_t      polymer_bindmaterial(_prmaterial material, int16_t* lights, int lightcount);
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

#endif // !_polymer_h_
