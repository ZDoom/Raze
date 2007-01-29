#ifndef _polymost_h_
# define _polymost_h_

# include "glbuild.h"

struct hicskybox_t {
    long ignore;
    char *face[6];
};

typedef struct hicreplc_t {
    struct hicreplc_t *next;
    char palnum, ignore, flags, filler;
    char *filename;
    float alphacut;
    struct hicskybox_t *skybox;
} hicreplctyp;

typedef struct pthtyp_t
{
    struct pthtyp_t *next;
    GLuint glpic;
    short picnum;
    char palnum;
    char effects;
    char flags;      // 1 = clamped (dameth&4), 2 = hightile, 4 = skybox face, 8 = hasalpha, 16 = hasfullbright, 128 = invalidated
    char skyface;
    hicreplctyp *hicr;

    unsigned short sizx, sizy;
    float scalex, scaley;
    struct pthtyp_t *ofb; // only fullbright
} pthtyp;

pthtyp * gltexcache (long dapicnum, long dapalnum, long dameth);

extern palette_t hictinting[MAXPALOOKUPS];
extern float     gtang;

#endif // !_polymost_h_
