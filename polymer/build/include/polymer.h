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
# include "polymost.h"
# include "mdsprite.h"
# include "pragmas.h"
# include <math.h>

// CVARS
extern int          pr_occlusionculling;
extern int          pr_fov;
extern int          pr_billboardingmode;
extern int          pr_verbosity;
extern int          pr_wireframe;
extern int          pr_vbos;
extern int          pr_gpusmoothing;

extern int          glerror;

// DATA
typedef struct      s_prplane {
    // geometry
    GLfloat*        buffer;
    GLuint          vbo;
    // attributes
    GLdouble        plane[4];
    GLfloat         color[4];
    GLuint          glpic;
    GLuint          fbglpic;
    // elements
    GLushort*       indices;
    GLuint          ivbo;
}                   _prplane;

typedef struct      s_prsector {
    // polymer data
    GLdouble*       verts;
    _prplane        floor;
    _prplane        ceil;
    short           curindice;
    int             indicescount;
    int             oldindicescount;
    // stuff
    float           wallsproffset;
    float           floorsproffset;
    // build sector data
    int             ceilingz, floorz;
    short           ceilingstat, floorstat;
    short           ceilingpicnum, ceilingheinum;
    signed char     ceilingshade;
    char            ceilingpal, ceilingxpanning, ceilingypanning;
    short           floorpicnum, floorheinum;
    signed char     floorshade;
    char            floorpal, floorxpanning, floorypanning;

    char            controlstate; // 1: up to date, 2: just allocated
    unsigned int    invalidid;
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
    short           cstat, nwallcstat;
    short           picnum, overpicnum, nwallpicnum;
    signed char     shade;
    char            pal, xrepeat, yrepeat, xpanning, ypanning;
    char            nwallxpanning, nwallypanning;


    char            underover;
    unsigned int    invalidid;
    char            controlstate;
}                   _prwall;

typedef struct      s_pranimatespritesinfo {
    animatespritesptr animatesprites;
    int             x, y, a, smoothratio;
}                   _pranimatespritesinfo;

extern _prsector*   prsectors[MAXSECTORS];
extern _prwall*     prwalls[MAXWALLS];

// CONTROL
extern int          updatesectors;

// EXTERNAL FUNCTIONS
int                 polymer_init(void);
void                polymer_glinit(void);
void                polymer_loadboard(void);
void                polymer_drawrooms(int daposx, int daposy, int daposz, short daang, int dahoriz, short dacursectnum);
void                polymer_drawmasks(void);
void                polymer_rotatesprite(int sx, int sy, int z, short a, short picnum, signed char dashade, char dapalnum, char dastat, int cx1, int cy1, int cx2, int cy2);
void                polymer_drawmaskwall(int damaskwallcnt);
void                polymer_drawsprite(int snum);

# ifdef POLYMER_C

// CORE
static void         polymer_displayrooms(short sectnum);
static void         polymer_drawplane(short sectnum, short wallnum, _prplane* plane, int indicecount);
static void         polymer_inb4mirror(GLfloat* buffer, GLdouble* plane);
static void         polymer_animatesprites(void);
// SECTORS
static int          polymer_initsector(short sectnum);
static int          polymer_updatesector(short sectnum);
void PR_CALLBACK    polymer_tesserror(GLenum error);
void PR_CALLBACK    polymer_tessedgeflag(GLenum error);
void PR_CALLBACK    polymer_tessvertex(void* vertex, void* sector);
static int          polymer_buildfloor(short sectnum);
static void         polymer_drawsector(short sectnum);
// WALLS
static int          polymer_initwall(short wallnum);
static void         polymer_updatewall(short wallnum);
static void         polymer_drawwall(short sectnum, short wallnum);
// HSR
static void         polymer_buffertoplane(GLfloat* buffer, GLushort* indices, int indicecount, GLdouble* plane);
static void         polymer_crossproduct(GLfloat* in_a, GLfloat* in_b, GLdouble* out);
static void         polymer_pokesector(short sectnum);
static void         polymer_extractfrustum(GLdouble* modelview, GLdouble* projection, float* frustum);
static int          polymer_portalinfrustum(short wallnum, float* frustum);
static void         polymer_scansprites(short sectnum, spritetype* tsprite, int* spritesortcnt);
// SKIES
static void         polymer_initskybox(void);
static void         polymer_getsky(void);
static void         polymer_drawskyquad(int p1, int p2, GLfloat height);
static void         polymer_drawartsky(short tilenum);
// MDSPRITES
static void         polymer_drawmdsprite(spritetype *tspr);
static void         polymer_loadmodelvbos(md3model* m);

# endif // !POLYMER_C

#endif // !_polymer_h_
