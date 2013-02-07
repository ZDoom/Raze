#ifndef _mdsprite_h_
# define _mdsprite_h_

#ifdef USE_OPENGL
#include "hightile.h"

#define SHIFTMOD32(a) ((a)&31)

typedef struct
{
    int32_t mdnum; //VOX=1, MD2=2, MD3=3. NOTE: must be first in structure!
    int32_t shadeoff;
    float scale, bscale, zadd, yoffset;
    GLuint *texid;	// skins
    int32_t flags;
} mdmodel_t;

typedef struct _mdanim_t
{
    int32_t startframe, endframe;
    int32_t fpssc, flags;
    struct _mdanim_t *next;
} mdanim_t;
#define MDANIM_LOOP 0
#define MDANIM_ONESHOT 1

typedef struct _mdskinmap_t
{
    uint8_t palette, filler[3]; // Build palette number
    int32_t skinnum, surfnum;   // Skin identifier, surface number
    char *fn;   // Skin filename
    GLuint texid[HICEFFECTMASK+1];   // OpenGL texture numbers for effect variations
    struct _mdskinmap_t *next;
    float param, specpower, specfactor;
} mdskinmap_t;


//This MD2 code is based on the source code from David Henry (tfc_duke(at)hotmail.com)
//   Was at http://tfc.duke.free.fr/us/tutorials/models/md2.htm
//   Available from http://web.archive.org/web/20030816010242/http://tfc.duke.free.fr/us/tutorials/models/md2.htm
//   Now at http://tfc.duke.free.fr/coding/md2.html (in French)
//He probably wouldn't recognize it if he looked at it though :)
typedef struct { float x, y, z; } point3d;

typedef struct
{
    int32_t id, vers, skinxsiz, skinysiz, framebytes; //id:"IPD2", vers:8
    int32_t numskins, numverts, numuv, numtris, numglcmds, numframes;
    int32_t ofsskins, ofsuv, ofstris, ofsframes, ofsglcmds, ofseof; //ofsskins: skin names (64 bytes each)
} md2head_t;

typedef struct { uint8_t v[3], ni; } md2vert_t; //compressed vertex coords (x,y,z)
typedef struct
{
    point3d mul, add; //scale&translation vector
    char name[16];    //frame name
    md2vert_t verts[1]; //first vertex of this frame
} md2frame_t;

typedef struct { int16_t u, v; } md2uv_t;
typedef struct
{
    uint16_t v[3];
    uint16_t u[3];
} md2tri_t;

typedef struct
{
    //WARNING: This top block is a union between md2model&md3model: Make sure it matches!
    int32_t mdnum; //VOX=1, MD2=2, MD3=3. NOTE: must be first in structure!
    int32_t shadeoff;
    float scale, bscale, zadd, yoffset;
    GLuint *texid;   // texture ids for base skin if no mappings defined
    int32_t flags;

    int32_t numframes, cframe, nframe, fpssc, usesalpha;
    float oldtime, curtime, interpol;
    mdanim_t *animations;
    mdskinmap_t *skinmap;
    int32_t numskins, skinloaded;   // set to 1+numofskin when a skin is loaded and the tex coords are modified,

    //MD2 specific stuff:
    int32_t numverts, numglcmds, framebytes, *glcmds;
    char *frames;
    char *basepath;   // pointer to string of base path
    char *skinfn;   // pointer to first of numskins 64-char strings
    md2uv_t *uv;
    md2tri_t* tris;
} md2model_t;


typedef struct { char nam[64]; int32_t i; } md3shader_t; //ascz path of shader, shader index
typedef struct { int32_t i[3]; } md3tri_t; //indices of tri
typedef struct { float u, v; } md3uv_t;
typedef struct { int16_t x, y, z; uint8_t nlat, nlng; } md3xyzn_t; //xyz are [10:6] ints

typedef struct
{
    point3d min, max, cen; //bounding box&origin
    float r; //radius of bounding sphere
    char nam[16]; //ascz frame name
} md3frame_t;

typedef struct
{
    char nam[64]; //ascz tag name
    point3d p, x, y, z; //tag object pos&orient
} md3tag_t;

typedef struct
{
    int32_t id; //IDP3(0x33806873)
    char nam[64]; //ascz surface name
    int32_t flags; //?
    int32_t numframes, numshaders, numverts, numtris; //numframes same as md3head,max shade=~256,vert=~4096,tri=~8192
    int32_t ofstris;
    int32_t ofsshaders;
    int32_t ofsuv;
    int32_t ofsxyzn;
    int32_t ofsend;
    // DO NOT read directly to this structure
    // the following block is NOT in the file format
    // be sure to use the SIZEOF_MD3SURF_T macro
    md3tri_t *tris;
    md3shader_t *shaders;
    md3uv_t *uv;
    md3xyzn_t *xyzn;
    float *geometry;  // used by Polymer
} md3surf_t;

#define SIZEOF_MD3SURF_T (11*sizeof(int32_t) + 64*sizeof(char))

typedef struct
{
    int32_t id, vers; //id=IDP3(0x33806873), vers=15
    char nam[64]; //ascz path in PK3
    int32_t flags; //?
    int32_t numframes, numtags, numsurfs, numskins; //max=~1024,~16,~32,numskins=artifact of MD2; use shader field instead
    int32_t ofsframes;
    int32_t ofstags;
    int32_t ofssurfs;
    int32_t eof;
    // DO NOT read directly to this structure
    // the following block is NOT in the file format
    // be sure to use the SIZEOF_MD3HEAD_T macro
    md3frame_t *frames;
    md3tag_t *tags;
    md3surf_t *surfs;
} md3head_t;

#define SIZEOF_MD3HEAD_T (sizeof(md3head_t)-3*sizeof(void*))

typedef struct
{
    //WARNING: This top block is a union between md2model&md3model: Make sure it matches!
    int32_t mdnum; //VOX=1, MD2=2, MD3=3. NOTE: must be first in structure!
    int32_t shadeoff;
    float scale, bscale, zadd, yoffset;
    uint32_t *texid;   // texture ids for base skin if no mappings defined
    int32_t flags;

    int32_t numframes, cframe, nframe, fpssc, usesalpha;
    float oldtime, curtime, interpol;
    mdanim_t *animations;
    mdskinmap_t *skinmap;
    int32_t numskins, skinloaded;   // set to 1+numofskin when a skin is loaded and the tex coords are modified,

    //MD3 specific
    md3head_t head;
    point3d *muladdframes;
    uint16_t      *indexes;
    uint16_t      *vindexes;
    float               *maxdepths;
    GLuint*             vbos;
    // polymer VBO names after that, allocated per surface
    GLuint*             indices;
    GLuint*             texcoords;
    GLuint*             geometry;
} md3model_t;

#define VOXBORDWIDTH 1 //use 0 to save memory, but has texture artifacts; 1 looks better...
#define VOXUSECHAR 0
#if (VOXUSECHAR != 0)
typedef struct { uint8_t x, y, z, u, v; } vert_t;
#else
typedef struct { uint16_t x, y, z, u, v; } vert_t;
#endif
typedef struct { vert_t v[4]; } voxrect_t;
typedef struct
{
    //WARNING: This top block is a union of md2model,md3model,voxmodel: Make sure it matches!
    int32_t mdnum; //VOX=1, MD2=2, MD3=3. NOTE: must be first in structure!
    int32_t shadeoff;
    float scale, bscale, zadd;
    uint32_t *texid;    // skins for palettes
    int32_t flags;

    //VOX specific stuff:
    voxrect_t *quad; int32_t qcnt, qfacind[7];
    int32_t *mytex, mytexx, mytexy;
    int32_t xsiz, ysiz, zsiz;
    float xpiv, ypiv, zpiv;
    int32_t is8bit;
} voxmodel_t;

EXTERN mdmodel_t **models;

void updateanimation(md2model_t *m, const spritetype *tspr, uint8_t lpal);
int32_t mdloadskin(md2model_t *m, int32_t number, int32_t pal, int32_t surf);
void mdinit(void);
void freeallmodels(void);
void clearskins(void);
int32_t mddraw(const spritetype *tspr);

typedef struct { float xadd, yadd, zadd; int16_t angadd, flags, fov; } hudtyp;

EXTERN hudtyp hudmem[2][MAXTILES];

EXTERN int32_t mdpause;
EXTERN int32_t nextmodelid;
EXTERN voxmodel_t *voxmodels[MAXVOXELS];

void voxfree(voxmodel_t *m);
voxmodel_t *voxload(const char *filnam);
int32_t voxdraw(voxmodel_t *m, const spritetype *tspr);

int      md3postload_polymer(md3model_t* m);
//int32_t md_thinoutmodel(int32_t modelid, uint8_t *usedframebitmap);

#endif // defined USE_OPENGL
#endif // !_mdsprite_h_
