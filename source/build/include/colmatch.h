
#include "compat.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void initfastcolorlookup_scale(int32_t rscale, int32_t gscale, int32_t bscale);
extern void initfastcolorlookup_palette(uint8_t const * pal) ATTRIBUTE((nonnull(1)));
extern void initfastcolorlookup_gridvectors(void);

extern int32_t getclosestcol_lim(int32_t r, int32_t g, int32_t b, int32_t lastokcol);
extern int32_t getclosestcol_nocache_lim(int32_t r, int32_t g, int32_t b, int32_t lastokcol);
extern void getclosestcol_flush(void);

static FORCE_INLINE int32_t paletteGetClosestColor(int32_t r, int32_t g, int32_t b)
{
    return getclosestcol_lim(r, g, b, 255);
}
static FORCE_INLINE int32_t getclosestcol_nocache(int32_t r, int32_t g, int32_t b)
{
    return getclosestcol_nocache_lim(r, g, b, 255);
}

#ifdef __cplusplus
}
#endif
