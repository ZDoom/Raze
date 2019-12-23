
#include "compat.h"

extern void paletteInitClosestColorScale(int32_t rscale, int32_t gscale, int32_t bscale);
extern void paletteInitClosestColorMap(uint8_t const * pal) ATTRIBUTE((nonnull(1)));
extern void paletteInitClosestColorGrid(void);

extern int32_t paletteGetClosestColorUpToIndex(int32_t r, int32_t g, int32_t b, int32_t lastokcol);
extern int32_t paletteGetClosestColorUpToIndexNoCache(int32_t r, int32_t g, int32_t b, int32_t lastokcol);
extern void paletteFlushClosestColor(void);

static FORCE_INLINE int32_t paletteGetClosestColor(int32_t r, int32_t g, int32_t b)
{
    return paletteGetClosestColorUpToIndex(r, g, b, 255);
}
