#ifndef _mdsprite_h_
# define _mdsprite_h_

#ifdef __POWERPC__
#define SHIFTMOD32(a) ((a)&31)
#else
#define SHIFTMOD32(a) (a)
#endif

typedef struct
{
    int mdnum; //VOX=1, MD2=2, MD3=3. NOTE: must be first in structure!
    int shadeoff;
    float scale, bscale, zadd;
    GLuint *texid;	// skins
    int flags;
} mdmodel;

typedef struct _mdanim_t
{
    int startframe, endframe;
    int fpssc, flags;
    struct _mdanim_t *next;
} mdanim_t;
#define MDANIM_LOOP 0
#define MDANIM_ONESHOT 1

typedef struct _mdskinmap_t
{
    unsigned char palette, filler[3]; // Build palette number
    int skinnum, surfnum;   // Skin identifier, surface number
    char *fn;   // Skin filename
    GLuint texid[HICEFFECTMASK+1];   // OpenGL texture numbers for effect variations
    struct _mdskinmap_t *next;
    float param;
    char *palmap;int size;
} mdskinmap_t;


//This MD2 code is based on the source code from David Henry (tfc_duke(at)hotmail.com)
//   Was at http://tfc.duke.free.fr/us/tutorials/models/md2.htm
//   Available from http://web.archive.org/web/20030816010242/http://tfc.duke.free.fr/us/tutorials/models/md2.htm
//   Now at http://tfc.duke.free.fr/coding/md2.html (in French)
//He probably wouldn't recognize it if he looked at it though :)
typedef struct { float x, y, z; } point3d;

typedef struct
{
    int id, vers, skinxsiz, skinysiz, framebytes; //id:"IPD2", vers:8
    int numskins, numverts, numuv, numtris, numglcmds, numframes;
    int ofsskins, ofsuv, ofstris, ofsframes, ofsglcmds, ofseof; //ofsskins: skin names (64 bytes each)
} md2head_t;

typedef struct { unsigned char v[3], ni; } md2vert_t; //compressed vertex coords (x,y,z)
typedef struct
{
    point3d mul, add; //scale&translation vector
    char name[16];    //frame name
    md2vert_t verts[1]; //first vertex of this frame
} md2frame_t;

typedef struct { short u, v; } md2uv_t;
typedef struct
{
    unsigned short v[3];
    unsigned short u[3];
} md2tri_t;

typedef struct
{
    //WARNING: This top block is a union between md2model&md3model: Make sure it matches!
    int mdnum; //VOX=1, MD2=2, MD3=3. NOTE: must be first in structure!
    int shadeoff;
    float scale, bscale, zadd;
    GLuint *texid;   // texture ids for base skin if no mappings defined
    int flags;

    int numframes, cframe, nframe, fpssc, usesalpha;
    float oldtime, curtime, interpol;
    mdanim_t *animations;
    mdskinmap_t *skinmap;
    int numskins, skinloaded;   // set to 1+numofskin when a skin is loaded and the tex coords are modified,

    //MD2 specific stuff:
    int numverts, numglcmds, framebytes, *glcmds;
    char *frames;
    char *basepath;   // pointer to string of base path
    char *skinfn;   // pointer to first of numskins 64-char strings
    md2uv_t *uv;
    md2tri_t* tris;
} md2model;


typedef struct { char nam[64]; int i; } md3shader_t; //ascz path of shader, shader index
typedef struct { int i[3]; } md3tri_t; //indices of tri
typedef struct { float u, v; } md3uv_t;
typedef struct { signed short x, y, z; unsigned char nlat, nlng; } md3xyzn_t; //xyz are [10:6] ints

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
    int id; //IDP3(0x33806873)
    char nam[64]; //ascz surface name
    int flags; //?
    int numframes, numshaders, numverts, numtris; //numframes same as md3head,max shade=~256,vert=~4096,tri=~8192
    int ofstris;
    int ofsshaders;
    int ofsuv;
    int ofsxyzn;
    int ofsend;
    // DO NOT read directly to this structure
    // the following block is NOT in the file format
    // be sure to use the SIZEOF_MD3SURF_T macro
    md3tri_t *tris;
    md3shader_t *shaders;
    md3uv_t *uv;
    md3xyzn_t *xyzn;
} md3surf_t;

#define SIZEOF_MD3SURF_T (sizeof(md3surf_t)-4*sizeof(void*))

typedef struct
{
    int id, vers; //id=IDP3(0x33806873), vers=15
    char nam[64]; //ascz path in PK3
    int flags; //?
    int numframes, numtags, numsurfs, numskins; //max=~1024,~16,~32,numskins=artifact of MD2; use shader field instead
    int ofsframes;
    int ofstags;
    int ofssurfs;
    int eof;
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
    int mdnum; //VOX=1, MD2=2, MD3=3. NOTE: must be first in structure!
    int shadeoff;
    float scale, bscale, zadd;
    unsigned int *texid;   // texture ids for base skin if no mappings defined
    int flags;

    int numframes, cframe, nframe, fpssc, usesalpha;
    float oldtime, curtime, interpol;
    mdanim_t *animations;
    mdskinmap_t *skinmap;
    int numskins, skinloaded;   // set to 1+numofskin when a skin is loaded and the tex coords are modified,

    //MD3 specific
    md3head_t head;
    point3d *muladdframes;
    unsigned short      *indexes;
    unsigned short      *vindexes;
    float               *maxdepths;
    GLuint*             vbos;
    // polymer VBO names after that, allocated per surface
    GLuint*             indices;
    GLuint*             texcoords;
    GLuint*             geometry;
} md3model;

#define VOXBORDWIDTH 1 //use 0 to save memory, but has texture artifacts; 1 looks better...
#define VOXUSECHAR 0
#if (VOXUSECHAR != 0)
typedef struct { unsigned char x, y, z, u, v; } vert_t;
#else
typedef struct { unsigned short x, y, z, u, v; } vert_t;
#endif
typedef struct { vert_t v[4]; } voxrect_t;
typedef struct
{
    //WARNING: This top block is a union of md2model,md3model,voxmodel: Make sure it matches!
    int mdnum; //VOX=1, MD2=2, MD3=3. NOTE: must be first in structure!
    int shadeoff;
    float scale, bscale, zadd;
    unsigned int *texid;    // skins for palettes
    int flags;

    //VOX specific stuff:
    voxrect_t *quad; int qcnt, qfacind[7];
    int *mytex, mytexx, mytexy;
    int xsiz, ysiz, zsiz;
    float xpiv, ypiv, zpiv;
    int is8bit;
} voxmodel;

typedef struct
{
    // maps build tiles to particular animation frames of a model
    int     modelid;
    int     skinnum;
    int     framenum;   // calculate the number from the name when declaring
    float   smoothduration;
    int     next;
    char    pal;
} tile2model_t;

#define EXTRATILES MAXTILES
EXTERN tile2model_t tile2model[MAXTILES+EXTRATILES];
EXTERN mdmodel **models;

void updateanimation(md2model *m, spritetype *tspr);
int mdloadskin(md2model *m, int number, int pal, int surf);
void mdinit(void);
void freeallmodels(void);
void clearskins(void);
int mddraw(spritetype *tspr);

typedef struct { float xadd, yadd, zadd; short angadd, flags; } hudtyp;

EXTERN hudtyp hudmem[2][MAXTILES]; //~320KB ... ok for now ... could replace with dynamic alloc

EXTERN int mdinited;
EXTERN int mdpause;
EXTERN int nummodelsalloced, nextmodelid;
EXTERN voxmodel *voxmodels[MAXVOXELS];

void voxfree(voxmodel *m);
voxmodel *voxload(const char *filnam);
int voxdraw(voxmodel *m, spritetype *tspr);


#endif // !_mdsprite_h_
