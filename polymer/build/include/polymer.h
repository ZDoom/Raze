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

void    polymer_glinit(void);
int     polymer_init(void);
void    polymer_drawrooms(long daposx, long daposy, long daposz, short daang, long dahoriz, short dacursectnum);

typedef struct  s_prvertex {
    double      v[3];
    short       wallnum;
}               _prvertex;

typedef struct      s_prsector {
    _prvertex*      verts;
    short           wallcount;
    char            invalidate;
}               _prsector;

extern _prsector*      prsectors[MAXSECTORS];

// Polymer cvars
extern char pr_verbosity;
extern char pr_wireframe;

#endif // !_polymer_h_
