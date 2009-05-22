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
extern int32_t      pr_maxlightpasses;
extern int32_t      pr_maxlightpriority;
extern int32_t      pr_fov;
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
    GLsizei         nextframedatastride;
    // PR_BIT_NORMAL_MAP
    GLuint          normalmap;
    GLfloat         normalbias[2];
    // PR_BIT_DIFFUSE_MAP
    GLuint          diffusemap;
    GLfloat         diffusescale[2];
    // PR_BIT_DIFFUSE_DETAIL_MAP
    GLuint          detailmap;
    GLfloat         detailscale[2];
    // PR_BIT_DIFFUSE_MODULATION
    GLfloat         diffusemodulation[4];
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
    // PR_BIT_SHADOW_MAP
    GLint           uniform_shadowMap;
    GLint           uniform_shadowProjMatrix;
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

// LIGHTS
#define             PR_MAXLIGHTS            128
#define             SHADOW_DEPTH_OFFSET     30
#define             PR_MAXLIGHTPRIORITY     6

typedef struct      s_prlight {
    int32_t         x, y, z, horiz, range;
    int16_t         angle, faderadius, radius, sector;
    uint8_t         color[3], priority;
    int8_t          minshade, maxshade;
    int16_t         tilenum;
    // internal members
    GLfloat         proj[16];
    GLfloat         transform[16];
    float           frustum[5 * 4];
    int32_t         rtindex;
    char            isinview;
    GLuint          lightmap;
}                   _prlight;

extern _prlight     staticlights[PR_MAXLIGHTS];
extern int32_t      staticlightcount;

extern _prlight     gamelights[PR_MAXLIGHTS];
extern int32_t      gamelightcount;

extern _prlight     framelights[PR_MAXLIGHTS];
extern int32_t      framelightcount;

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
    GLfloat         t[3];
    GLfloat         b[3];
    GLfloat         n[3];
    GLfloat         plane[4];
    _prmaterial     material;
    // elements
    GLushort*       indices;
    int32_t         indicescount;
    GLuint          ivbo;
    // lights
    char            lights[PR_MAXLIGHTS];
    char            lightcount;
    char            drawn;
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

    char            controlstate; // 1: up to date, 2: just allocated
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
    char            controlstate;
}                   _prwall;

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
void                polymer_loadboard(void);
void                polymer_drawrooms(int32_t daposx, int32_t daposy, int32_t daposz, int16_t daang, int32_t dahoriz, int16_t dacursectnum);
void                polymer_drawmasks(void);
void                polymer_rotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum, int8_t dashade, char dapalnum, char dastat, int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2);
void                polymer_drawmaskwall(int32_t damaskwallcnt);
void                polymer_drawsprite(int32_t snum);
void                polymer_setanimatesprites(animatespritesptr animatesprites, int32_t x, int32_t y, int32_t a, int32_t smoothratio);

# ifdef POLYMER_C

// CORE
static void         polymer_displayrooms(int16_t sectnum);
static void         polymer_drawplane(_prplane* plane);
static inline void  polymer_inb4mirror(GLfloat* buffer, GLfloat* plane);
static void         polymer_animatesprites(void);
static void         polymer_freeboard(void);
static void         polymer_editorselect(void);
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
static inline void  polymer_transformpoint(float* inpos, float* pos, float* matrix);
static inline void  polymer_pokesector(int16_t sectnum);
static void         polymer_extractfrustum(GLfloat* modelview, GLfloat* projection, float* frustum);
static int32_t      polymer_planeinfrustum(_prplane *plane, float* frustum);
static inline void  polymer_scansprites(int16_t sectnum, spritetype* tsprite, int32_t* spritesortcnt);
// SKIES
static void         polymer_getsky(void);
static void         polymer_drawsky(int16_t tilenum);
static void         polymer_initartsky(void);
static void         polymer_drawartsky(int16_t tilenum);
static void         polymer_drawartskyquad(int32_t p1, int32_t p2, GLfloat height);
static void         polymer_drawskybox(int16_t tilenum);
// MDSPRITES
static void         polymer_drawmdsprite(spritetype *tspr);
static void         polymer_loadmodelvbos(md3model_t* m);
// MATERIALS
static void         polymer_getscratchmaterial(_prmaterial* material);
static void         polymer_getbuildmaterial(_prmaterial* material, int16_t tilenum, char pal, int8_t shade);
static int32_t      polymer_bindmaterial(_prmaterial material, char* lights, int lightcount);
static void         polymer_unbindmaterial(int32_t programbits);
static void         polymer_compileprogram(int32_t programbits);
// LIGHTS
static void         polymer_resetlights(void);
static void         polymer_addlight(_prlight light);
static int32_t      polymer_planeinlight(_prplane* plane, _prlight* light);
static inline void  polymer_culllight(char lightindex);
static void         polymer_prepareshadows(void);
static void         polymer_applylights(void);
// RENDER TARGETS
static void         polymer_initrendertargets(int32_t count);

# endif // !POLYMER_C

#endif // !_polymer_h_
