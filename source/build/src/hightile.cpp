/*
 * High-colour textures support for Polymost
 * by Jonathon Fowler
 * See the included license file "BUILDLIC.TXT" for license info.
 */

#include "build.h"
#include "compat.h"
#include "hightile.h"
#include "baselayer.h"


polytint_t hictinting[MAXPALOOKUPS];


//
// hicinit()
//   Initialize the high-colour stuff to default.
//
void hicinit(void)
{
    int32_t i;

    for (i=0; i<MAXPALOOKUPS; i++)  	// all tints should be 100%
    {
        polytint_t & tint = hictinting[i];
        tint.tint = 0xffffff;
        tint.f = 0;
    }
}


//
// hicsetpalettetint(pal,r,g,b,sr,sg,sb,effect)
//   The tinting values represent a mechanism for emulating the effect of global sector
//   palette shifts on true-colour textures and only true-colour textures.
//   effect bitset: 1 = greyscale, 2 = invert
//
void hicsetpalettetint(int32_t palnum, int r, int g, int b, int sr, int sg, int sb, polytintflags_t effect)
{
    if ((uint32_t)palnum >= (uint32_t)MAXPALOOKUPS) return;

    polytint_t & tint = hictinting[palnum];
    tint.tint.r = (uint8_t)r;
    tint.tint.g = (uint8_t)g;
    tint.tint.b = (uint8_t)b;
    tint.shade.r = (uint8_t)sr;
    tint.shade.g = (uint8_t)sg;
    tint.shade.b = (uint8_t)sb;
    tint.f = effect|HICTINT_ENABLE;
}



