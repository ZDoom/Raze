// here lies the GREAT JUSTICE RENDERER
// TODO :
// - CORE STUFF
//   o put all the sector/wall geometry in VBOs
//   o there's still a texture alignment problem with slopes (waterfall in launch facility)
//   o wall palette problem (toxic waterfall in the abyss is blue)
//   o there's also the texture alignment problem Hunter reported (san andreas fault)
//   o also sliding doors are still fucked up sometimes (like under the bar in E1L2)
//   o port glowmaps and detail maps from hacked polymost (:(
//   o shading needs a lot of work
//   o proper mask texture coordinates and color/palette and shit
//   o remove all the IM matrix crap and write real functions now that it works
//   o polymer.c possibly needs to be split in several source files
//   o some crap really needs factorization
//   o only make parallaxes write into the depth buffer if map boundary (e1l2)
//   o ... possibly more important stuff I don't have in mind right now
// - SPRITES
//   o port sprite panning and fullbrights from hacked polymost (:(
//   o draw all opaques first, keep translucent for later with masks
// - SKIES
//   o figure a better way to handle ART skies - maybe add symetric caps that would fade to black like a big gem or something wow this is a long column lol ;0)
//   o implement polymost skyboxes
// - MIRRORS
//   o figure out how to get mirror data from game
//   o unified mirror transformation (not just walls)
// - MDSPRITES
//   o need to reimplement them - hopefully the loader can be reused without too much hassle
//   o need full translation and rotation support from CON to attach to game world or tags
//   o need to put frames into VBOs and blend between them
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
# include "pragmas.h"
# include <math.h>

// CVARS
extern int          pr_occlusionculling;
extern int          pr_fov;
extern int          pr_showportals;
extern int          pr_verbosity;
extern int          pr_wireframe;

extern int          glerror;

// DATA
typedef struct      s_prsector {
    // geometry
    GLdouble*       verts;
    GLfloat*        floorbuffer;
    GLfloat*        ceilbuffer;
    // attributes
    GLfloat         floorcolor[4], ceilcolor[4];
    GLuint          floorglpic, ceilglpic, floorfbglpic, ceilfbglpic;
    // elements
    GLushort*       floorindices;
    GLushort*       ceilindices;
    short           curindice;
    int             indicescount;
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
    char            drawingstate; // 0: fcuk, 1: in queue, 2: todraw, 3: drawn
    unsigned int    invalidid;
}                   _prsector;

typedef struct      s_prwall {
    // geometry
    GLfloat*        wallbuffer;
    GLfloat*        overbuffer;
    GLfloat*        portal;
    GLfloat*        bigportal;
    // attributes
    GLfloat         wallcolor[4], overcolor[4];
    GLfloat         wallglpic, overglpic, wallfbglpic, overfbglpic, maskglpic;
    // build wall data# ifdef POLYMER_C

    short           cstat, nwallcstat;
    short           picnum, overpicnum, nwallpicnum;
    signed char     shade;
    char            pal, xrepeat, yrepeat, xpanning, ypanning;
    char            nwallxpanning, nwallypanning;


    char            underover;
    unsigned int    invalidid;
    char            controlstate;
}                   _prwall;

typedef struct      s_cliplane {
    _equation       left, right, clip;
    _point2d        ref;
}                   _cliplane;

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

// SECTORS
static int          polymer_initsector(short sectnum);
static int          polymer_updatesector(short sectnum);
void PR_CALLBACK    polymer_tesscombine(GLdouble v[3], GLdouble *data[4], GLfloat weight[4], GLdouble **out);
void PR_CALLBACK    polymer_tesserror(GLenum error);
void PR_CALLBACK    polymer_tessedgeflag(GLenum error);
void PR_CALLBACK    polymer_tessvertex(void* vertex, void* sector);
static int          polymer_buildfloor(short sectnum);
static void         polymer_drawsector(short sectnum);
// WALLS
static int          polymer_initwall(short wallnum);
static void         polymer_updatewall(short wallnum);
static void         polymer_drawwall(short wallnum);
// HSR
static void         polymer_pokesector(short sectnum);
static void         polymer_extractfrustum(GLdouble* modelview, GLdouble* projection);
static int          polymer_portalinfrustum(short wallnum);
// SKIES
static void         polymer_initskybox(void);
static void         polymer_getsky(void);
static void         polymer_drawskyquad(int p1, int p2, GLfloat height);
static void         polymer_drawartsky(short tilenum);

# endif

#endif // !_polymer_h_
