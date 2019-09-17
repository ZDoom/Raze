#ifndef texcache_h_
# define texcache_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "vfs.h"

#ifdef USE_OPENGL

#define GLTEXCACHEADSIZ 8192

typedef struct {
    pthtyp *list[GLTEXCACHEADSIZ];

} globaltexcache;

extern globaltexcache texcache;

extern pthtyp *texcache_fetch(int32_t dapicnum, int32_t dapalnum, int32_t dashade, int32_t dameth);

#endif

#ifdef __cplusplus
}
#endif

#endif
