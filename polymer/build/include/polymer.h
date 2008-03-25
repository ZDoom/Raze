// here lies the GREAT JUSTICE RENDERER
// TODO :
// - recursive drawrooms with cliplane stack (support for full portal engine with occlusion queries)
// - crossed walls (tier drops stones)
// - skies
// - mirrors
// - fullbright (multitexture OR)
// - masks and sprites ! (use sortcnt and regular drawmask or recode it ?)
// - classic shading
// --------------------- CLASSIC LIMIT ---------------------
// - mdsprites (tags)
// - lights (dynamic phong)
// - dynamic shadowmaps
// - normalmap palette (unified gpu program, parallax mapping)
// - shadow volumes
// - hardware particles
// - multitextured decals ? (on models too)
// --------------- FIRST PUBLIC RELEASE LIMIT --------------
// - horizon mapping (precalculate the horizon maps and store them into the gl cache ?)
// - post processing ([HDR-]bloom and possibly DOF)
// - MD5 (hardware skinning ?)

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
extern int          pr_scissorstest;
extern int          pr_fov;
extern int          pr_showportals;
extern int          pr_verbosity;
extern int          pr_wireframe;

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
void                polymer_portaltofrustum(GLfloat* portal, int portalpointcount, GLfloat* pos, GLfloat* frustum);
void                polymer_triangletoplane(GLfloat* triangle, GLfloat* plane);
void                polymer_crossproduct(GLfloat* in_a, GLfloat* in_b, GLfloat* out);
void                polymer_extractfrustum(GLdouble* modelview, GLdouble* projection);
void                polymer_drawroom(short sectnum);
int                 polymer_portalinfrustum(short wallnum);
float               polymer_pointdistancetoplane(GLfloat* point, GLfloat* plane);
float               polymer_pointdistancetopoint(GLfloat* point1, GLfloat* point2);
void                polymer_lineplaneintersection(GLfloat *point1, GLfloat *point2, float dist1, float dist2, GLfloat *output);
int                 polymer_cliptofrustum(short wallnum);
void                polymer_getportal(GLfloat* portalpoints, int portalpointcount, GLint* output);
void                polymer_drawportal(int *portal);
// SKIES
void                polymer_initskybox(void);
void                polymer_getsky(void);
void                polymer_drawskyquad(int p1, int p2, GLfloat height);
void                polymer_drawartsky(short tilenum);

#endif // !_polymer_h_
