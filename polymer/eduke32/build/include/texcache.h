#ifndef texcache_h_
# define texcache_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_OPENGL

#define TEXCACHEMAGIC "LZ40"
#define GLTEXCACHEADSIZ 8192
#define TEXCACHEHASHSIZE 1024

enum texcacherr_t
{
    TEXCACHERR_NOERROR,
    TEXCACHERR_OUTOFMEMORY,  /* unused */
    TEXCACHERR_BUFFERUNDERRUN,
    TEXCACHERR_DEDXT,
    TEXCACHERR_COMPTEX,
    TEXCACHERR_GETTEXLEVEL,
    TEXCACHEERRORS
};

struct texcacheitem_t
{
    char name[BMAX_PATH];
    int32_t offset;
    int32_t len;
    struct texcacheitem_t *next;
};

typedef struct texcacheitem_t texcacheindex;

typedef struct {
    int32_t filehandle, filepos, numentries, iptrcnt;
    FILE *index;
    pthtyp *list[GLTEXCACHEADSIZ];
    texcacheindex *firstindex, *currentindex, **iptrs;
    hashtable_t hashes;
    struct {
        uint8_t *ptr;
        int32_t size;
        // Set to 1 when we failed (re)allocating space for the memcache or failing to
        // read into it (which would presumably generate followup errors spamming the
        // log otherwise):
        int32_t noalloc;
    } memcache;
} globaltexcache;

extern globaltexcache texcache;

extern char TEXCACHEFILE[BMAX_PATH];

extern void texcache_freeptrs(void);
extern void texcache_syncmemcache(void);
extern void texcache_init(void);
extern int32_t texcache_loadoffsets(void);
extern int32_t texcache_readdata(void *dest, int32_t len);
extern pthtyp *texcache_fetch(int32_t dapicnum, int32_t dapalnum, int32_t dashade, int32_t dameth);
extern int32_t texcache_loadskin(const texcacheheader *head, int32_t *doalloc, GLuint *glpic, int32_t *xsiz, int32_t *ysiz);
extern int32_t texcache_loadtile(const texcacheheader *head, int32_t *doalloc, pthtyp *pth);
extern void texcache_writetex(const char *fn, int32_t len, int32_t dameth, char effect, texcacheheader *head);
extern int32_t texcache_readtexheader(const char *fn, int32_t len, int32_t dameth, char effect, texcacheheader *head, int32_t modelp);
extern void texcache_openfiles(void);
extern void texcache_setupmemcache(void);
extern void texcache_checkgarbage(void);
extern void texcache_setupindex(void);

#endif

#ifdef __cplusplus
}
#endif

#endif
