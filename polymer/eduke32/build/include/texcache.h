#ifndef _texcache_h_
# define _texcache_h_

#ifdef USE_OPENGL

#define TEXCACHEMAGIC "QLZ1"
#define GLTEXCACHEADSIZ 8192

struct texcacheitem_t
{
    char name[BMAX_PATH];
    int32_t offset;
    int32_t len;
    struct texcacheitem_t *next;
};

typedef struct texcacheitem_t texcacheindex;

extern char TEXCACHEFILE[BMAX_PATH];
extern pthtyp *texcache_head[GLTEXCACHEADSIZ];
extern FILE *texcache_indexptr;
extern int32_t texcache_noalloc;
extern int32_t texcache_memsize;
extern uint8_t *texcache_memptr;
extern int32_t texcache_filehandle;
extern int32_t texcache_offset;
extern texcacheindex *texcache_firstindex;
extern texcacheindex *texcache_currentindex;

extern void texcache_freeptrs(void);
extern void texcache_sync(void);
extern void texcache_init(void);
extern void texcache_clearmem(void);
extern int32_t texcache_loadoffsets(void);
extern int32_t texcache_readdata(void *dest, int32_t len);
extern pthtyp *texcache_fetch(int32_t dapicnum, int32_t dapalnum, int32_t dameth);
extern int32_t texcache_loadskin(const texcacheheader *head, int32_t *doalloc, GLuint *glpic, int32_t *xsiz, int32_t *ysiz);
extern int32_t texcache_loadtile(const texcacheheader *head, int32_t *doalloc, pthtyp *pth);
extern void texcache_writetex(const char *fn, int32_t len, int32_t dameth, char effect, texcacheheader *head);
extern int32_t texcache_readtexheader(const char *fn, int32_t len, int32_t dameth, char effect, texcacheheader *head, int32_t modelp);

#endif
#endif