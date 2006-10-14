// blah

#ifndef _polymer_h_
# define _polymer_h_

# ifdef _WIN32
#  define PR_CALLBACK __stdcall
# else
#  define PR_CALLBACK
# endif

# include "compat.h"
# include "build.h"
# include "glbuild.h"
# include "osd.h"
# include "polymost.h"
# include "pragmas.h"

// CVARS
extern unsigned int pr_fov;
extern char         pr_verbosity;
extern char         pr_wireframe;

// DATA
typedef struct      s_prsector {
    // geometry
    GLdouble*       verts;
    GLfloat*        floorbuffer;
    GLfloat*        ceilbuffer;
    // attributes
    GLfloat         floorcolor[4], ceilcolor[4];
    GLuint          floorglpic, ceilglpic;
    // elements
    GLushort*       floorindices;
    GLushort*       ceilindices;
    short           curindice;
    int             indicescount;

    short           wallcount;
    char            invalidate;
    char            drawingstate; // 0: fcuk, 1: in queue, 2: todraw, 3: drawn
}                   _prsector;

typedef struct      s_prwall {
    // geometry
    GLfloat*        wallbuffer;
    GLfloat*        overbuffer;
    // attributes
    GLfloat         wallcolor[4], overcolor[4];
    GLfloat         wallglpic, overglpic;

    char            underover;
    char            invalidate;
}                   _prwall;

typedef struct      s_cliplane {
    _equation       left, right, clip;
    _point2d        ref;
    char            clipsign;
}                   _cliplane;

extern _prsector*   prsectors[MAXSECTORS];
extern _prwall*     prwalls[MAXWALLS];

// EXTERNAL FUNCTIONS
int                 polymer_init(void);
void                polymer_glinit(void);
void                polymer_loadboard(void);
void                polymer_drawrooms(long daposx, long daposy, long daposz, short daang, long dahoriz, short dacursectnum, int root);
void                polymer_rotatesprite(long sx, long sy, long z, short a, short picnum, signed char dashade, char dapalnum, char dastat, long cx1, long cy1, long cx2, long cy2);
void                polymer_drawmaskwall(long damaskwallcnt);
void                polymer_drawsprite(long snum);
// SECTOR MANAGEMENT
int                 polymer_initsector(short sectnum);
int                 polymer_updatesector(short sectnum);
void PR_CALLBACK    polymer_tesscombine(GLdouble v[3], GLdouble *data[4], GLfloat weight[4], GLdouble **out);
void PR_CALLBACK    polymer_tesserror(GLenum error);
void PR_CALLBACK    polymer_tessedgeflag(GLenum error);
void PR_CALLBACK    polymer_tessvertex(void* vertex, void* sector);
int                 polymer_buildfloor(short sectnum);
void                polymer_drawsector(short sectnum);
// WALL MANAGEMENT
int                 polymer_initwall(short wallnum);
void                polymer_updatewall(short wallnum);
void                polymer_drawwall(short wallnum);
// HSR
int                 wallincliplane(short wallnum, _cliplane* cliplane);

#endif // !_polymer_h_
