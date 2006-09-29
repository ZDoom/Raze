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
    GLushort*       indices;
    short           curindice;
    int             indicescount;

    short           wallcount;
    char            invalidate;
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

extern _prsector*   prsectors[MAXSECTORS];
extern _prwall*     prwalls[MAXWALLS];

// EXTERNAL FUNCTIONS
int                 polymer_init(void);
void                polymer_glinit(void);
void                polymer_loadboard(void);
void                polymer_drawrooms(long daposx, long daposy, long daposz, short daang, long dahoriz, short dacursectnum);
// SECTOR MANAGEMENT
int                 polymer_initsector(short sectnum);
int                 polymer_updatesector(short sectnum);
void PR_CALLBACK    polymer_tesscombine(GLdouble v[3], GLdouble *data[4], GLfloat weight[4], GLdouble **out);
void PR_CALLBACK    polymer_tesserror(GLenum error);
void PR_CALLBACK    polymer_tessedgeflag(GLenum error);
void PR_CALLBACK    polymer_tessvertex(void* vertex, void* sector);
int                 polymer_buildfloor(short sectnum);
void                polymer_drawsector(long daposx, long daposy, long daposz, short daang, long dahoriz, short sectnum);
// WALL MANAGEMENT
int                 polymer_initwall(short wallnum);
void                polymer_updatewall(short wallnum);
void                polymer_drawwall(short wallnum);

#endif // !_polymer_h_
