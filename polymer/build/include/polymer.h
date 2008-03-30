// here lies the GREAT JUSTICE RENDERER
// TODO :
// - CORE STUFF
//   o put all the sector/wall geometry in VBOs
//   o optimize the update[sector|wall] functions to diff the changes
//   o figure a way to get the interpolations from the game
//   o make occlusion queries every n frames (cvar)
//   o there's still a texture alignment problem with slopes (waterfall in launch facility)
//   o there's also the texture alignment problem Hunter reported (san andreas fault)
//   o port glowmaps and detail maps from hacked polymost (:(
//   o shading needs a lot of work
//   o make the portal smaller
//   o one-way walls and masks
//   o remove all the IM matrix crap and write real functions now that it works
//   o polymer.c possibly needs to be split in several source files
//   o some crap really needs factorization
//   o ... possibly more important stuff I don't have in mind right now
// - SPRITES
//   o stop using IM
//   o stop using Get for every face sprite (do the calculation by hand)
//   o port sprite panning and fullbrights from hacked polymost (:(
//   o draw all opaques first, keep translucent for later with masks
//   o need smart PolygonOffset for floor and wall sprites
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

    char            controlstate; // bit 1: up-to-date, bit 2: geometry invalidated
    char            drawingstate; // 0: fcuk, 1: in queue, 2: todraw, 3: drawn
}                   _prsector;

typedef struct      s_prwall {
    // geometry
    GLfloat*        wallbuffer;
    GLfloat*        overbuffer;
    GLfloat*        portal;
    // attributes
    GLfloat         wallcolor[4], overcolor[4];
    GLfloat         wallglpic, overglpic, wallfbglpic, overfbglpic;

    char            underover;
    char            invalidate;
    char            drawn;
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
// SECTORS
int                 polymer_initsector(short sectnum);
int                 polymer_updatesector(short sectnum);
void PR_CALLBACK    polymer_tesscombine(GLdouble v[3], GLdouble *data[4], GLfloat weight[4], GLdouble **out);
void PR_CALLBACK    polymer_tesserror(GLenum error);
void PR_CALLBACK    polymer_tessedgeflag(GLenum error);
void PR_CALLBACK    polymer_tessvertex(void* vertex, void* sector);
int                 polymer_buildfloor(short sectnum);
void                polymer_drawsector(short sectnum);
// WALLS
int                 polymer_initwall(short wallnum);
void                polymer_updatewall(short wallnum);
void                polymer_drawwall(short wallnum);
// HSR
void                polymer_extractfrustum(GLdouble* modelview, GLdouble* projection);
int                 polymer_portalinfrustum(short wallnum);
// SKIES
void                polymer_initskybox(void);
void                polymer_getsky(void);
void                polymer_drawskyquad(int p1, int p2, GLfloat height);
void                polymer_drawartsky(short tilenum);

#endif // !_polymer_h_
