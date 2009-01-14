// here lies the GREAT JUSTICE RENDERER
// TODO :
// - CORE STUFF
//   o there's also the texture alignment problem Hunter reported (san andreas fault)
//   o also sliding doors are still fucked up sometimes (like under the bar in E1L2)
//   o port glowmaps and detail maps from hacked polymost (:(
//   o shading needs a lot of work
//   o remove all the IM matrix crap and write real functions now that it works
// - SPRITES
//   o port sprite panning and fullbrights from hacked polymost (:(
// - SKIES
//   o figure a better way to handle ART skies - maybe add symetric caps that would fade to black like a big gem or something wow this is a long column lol ;0)
//   o implement polymost skyboxes
// - MDSPRITES
//   o need to truly convert MD2s to MD3s with proper scale offset to just dump the data into VRAM
//   o need full translation and rotation support from CON to attach to game world or tags
//   o need to blend between frames
//
// the renderer should hopefully be pretty solid after all that
// the rest will be a bliss :)

#ifndef _polymer_h_
# define _polymer_h_

# include "compat.h"
# include "build.h"
# include "glbuild.h"
# include "osd.h"
# include "hightile.h"
# include "mdsprite.h"
# include "polymost.h"
# include "pragmas.h"
# include <math.h>

// CVARS
extern int32_t          pr_occlusionculling;
extern int32_t          pr_fov;
extern int32_t          pr_billboardingmode;
extern int32_t          pr_verbosity;
extern int32_t          pr_wireframe;
extern int32_t          pr_vbos;
extern int32_t          pr_gpusmoothing;

extern int32_t          glerror;

// MATERIAL
typedef enum {
                    PR_BIT_ANIM_INTERPOLATION,
                    PR_BIT_DIFFUSE_MAP,
                    PR_BIT_DIFFUSE_DETAIL_MAP,
                    PR_BIT_DIFFUSE_MODULATION,
                    PR_BIT_POINT_LIGHT,
                    PR_BIT_DIFFUSE_GLOW_MAP,
                    PR_BIT_DEFAULT, // must be just before last
                    PR_BIT_COUNT    // must be last
}                   prbittype;

typedef struct      s_prmaterial {
    // PR_BIT_ANIM_INTERPOLATION
    GLfloat         frameprogress;
    GLfloat*        nextframedata;
    GLsizei         nextframedatastride;
    // PR_BIT_DIFFUSE_MAP
    GLuint          diffusemap;
    GLfloat         diffusescale[2];
    // PR_BIT_DIFFUSE_DETAIL_MAP
    GLuint          detailmap;
    GLfloat         detailscale[2];
    // PR_BIT_DIFFUSE_MODULATION
    GLfloat         diffusemodulation[4];
    // PR_BIT_DIFFUSE_GLOW_MAP
    GLuint          glowmap;
}                   _prmaterial;

typedef struct      s_prrograminfo {
    GLhandleARB     handle;
    // PR_BIT_ANIM_INTERPOLATION
    GLint           attrib_nextFrameData;
    GLint           uniform_frameProgress;
    // PR_BIT_DIFFUSE_MAP
    GLint           uniform_diffuseMap;
    GLint           uniform_diffuseScale;
    // PR_BIT_DIFFUSE_DETAIL_MAP
    GLint           uniform_detailMap;
    GLint           uniform_detailScale;
    // PR_BIT_POINT_LIGHT
    GLint           uniform_lightCount;
    GLint           uniform_pointLightPosition;
    GLint           uniform_pointLightColor;
    GLint           uniform_pointLightRange;
    // PR_BIT_DIFFUSE_GLOW_MAP
    GLint           uniform_glowMap;
}                   _prprograminfo;

#define             PR_INFO_LOG_BUFFER_SIZE 16384

typedef struct      s_prprogrambit {
    int32_t             bit;
    char*           vert_def;
    char*           vert_prog;
    char*           frag_def;
    char*           frag_prog;
}                   _prprogrambit;

// BUILD DATA
typedef struct      s_prplane {
    // geometry
    GLfloat*        buffer;
    GLuint          vbo;
    // attributes
    GLdouble        plane[4];
    _prmaterial     material;
    // elements
    GLushort*       indices;
    GLuint          ivbo;
}                   _prplane;

typedef struct      s_prsector {
    // polymer data
    GLdouble*       verts;
    _prplane        floor;
    _prplane        ceil;
    int16_t           curindice;
    int32_t             indicescount;
    int32_t             oldindicescount;
    // stuff
    float           wallsproffset;
    float           floorsproffset;
    // build sector data
    int32_t             ceilingz, floorz;
    int16_t           ceilingstat, floorstat;
    int16_t           ceilingpicnum, ceilingheinum;
    int8_t     ceilingshade;
    char            ceilingpal, ceilingxpanning, ceilingypanning;
    int16_t           floorpicnum, floorheinum;
    int8_t     floorshade;
    char            floorpal, floorxpanning, floorypanning;

    char            controlstate; // 1: up to date, 2: just allocated
    uint32_t    invalidid;
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
    int16_t           cstat, nwallcstat;
    int16_t           picnum, overpicnum, nwallpicnum;
    int8_t     shade;
    char            pal, xrepeat, yrepeat, xpanning, ypanning;
    char            nwallxpanning, nwallypanning;


    char            underover;
    uint32_t    invalidid;
    char            controlstate;
}                   _prwall;

typedef struct      s_pranimatespritesinfo {
    animatespritesptr animatesprites;
    int32_t             x, y, a, smoothratio;
}                   _pranimatespritesinfo;

// LIGHTS
#define             PR_MAXLIGHTS 128

typedef enum {
                    PR_LIGHT_POINT,
                    PR_LIGHT_SPOT,
                    PR_LIGHT_DIRECTIONAL
}                   prlighttype;

typedef struct      s_prlight {
    int32_t             x, y, z, horiz, faderange, range;
    int16_t           angle, sector;
    prlighttype     type;
}                   _prlight;

// PROGRAMS

// CONTROL
extern int32_t          updatesectors;

// EXTERNAL FUNCTIONS
int32_t                 polymer_init(void);
void                polymer_glinit(void);
void                polymer_loadboard(void);
void                polymer_drawrooms(int32_t daposx, int32_t daposy, int32_t daposz, int16_t daang, int32_t dahoriz, int16_t dacursectnum);
void                polymer_drawmasks(void);
void                polymer_rotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum, int8_t dashade, char dapalnum, char dastat, int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2);
void                polymer_drawmaskwall(int32_t damaskwallcnt);
void                polymer_drawsprite(int32_t snum);
void                polymer_resetlights(void);
void                polymer_addlight(_prlight light);

# ifdef POLYMER_C

// CORE
static void         polymer_displayrooms(int16_t sectnum);
static void         polymer_drawplane(int16_t sectnum, int16_t wallnum, _prplane* plane, int32_t indicecount);
static void         polymer_inb4mirror(GLfloat* buffer, GLdouble* plane);
static void         polymer_animatesprites(void);
// SECTORS
static int32_t          polymer_initsector(int16_t sectnum);
static int32_t          polymer_updatesector(int16_t sectnum);
void PR_CALLBACK    polymer_tesserror(GLenum error);
void PR_CALLBACK    polymer_tessedgeflag(GLenum error);
void PR_CALLBACK    polymer_tessvertex(void* vertex, void* sector);
static int32_t          polymer_buildfloor(int16_t sectnum);
static void         polymer_drawsector(int16_t sectnum);
// WALLS
static int32_t          polymer_initwall(int16_t wallnum);
static void         polymer_updatewall(int16_t wallnum);
static void         polymer_drawwall(int16_t sectnum, int16_t wallnum);
// HSR
static void         polymer_buffertoplane(GLfloat* buffer, GLushort* indices, int32_t indicecount, GLdouble* plane);
static void         polymer_crossproduct(GLfloat* in_a, GLfloat* in_b, GLdouble* out);
static void         polymer_pokesector(int16_t sectnum);
static void         polymer_extractfrustum(GLfloat* modelview, GLfloat* projection, float* frustum);
static int32_t          polymer_portalinfrustum(int16_t wallnum, float* frustum);
static void         polymer_scansprites(int16_t sectnum, spritetype* tsprite, int32_t* spritesortcnt);
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
static int32_t          polymer_bindmaterial(_prmaterial material);
static void         polymer_unbindmaterial(int32_t programbits);
static void         polymer_compileprogram(int32_t programbits);

# endif // !POLYMER_C

#endif // !_polymer_h_
