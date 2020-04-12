//
// Common non-engine code/data for EDuke32 and Mapster32
//
#include "ns.h"	// Must come before everything else!

#include "compat.h"
#include "build.h"
#include "baselayer.h"
#include "palette.h"
#include "cmdlib.h"
#include "gamecvars.h"
#include "rts.h"
#include "gamecontrol.h"
#include "palettecontainer.h"

#include "common.h"
#include "common_game.h"

BEGIN_RR_NS

//////////

// Set up new-style multi-psky handling.
void G_InitMultiPsky(int CLOUDYOCEAN__DYN, int MOONSKY1__DYN, int BIGORBIT1__DYN, int LA__DYN)
{
    // When adding other multi-skies, take care that the tileofs[] values are
    // <= PSKYOFF_MAX. (It can be increased up to MAXPSKYTILES, but should be
    // set as tight as possible.)

    // The default sky properties (all others are implicitly zero):
    psky_t *sky      = tileSetupSky(DEFAULTPSKY);
    sky->lognumtiles = 3;
    sky->horizfrac   = 32768;

    // CLOUDYOCEAN
    // Aligns with the drawn scene horizon because it has one itself.
    sky              = tileSetupSky(CLOUDYOCEAN__DYN);
    sky->lognumtiles = 3;
    sky->horizfrac   = 65536;

    // MOONSKY1
    //        earth          mountain   mountain         sun
    sky              = tileSetupSky(MOONSKY1__DYN);
    sky->lognumtiles = 3;
    sky->horizfrac   = 32768;
    sky->tileofs[6]  = 1;
    sky->tileofs[1]  = 2;
    sky->tileofs[4]  = 2;
    sky->tileofs[2]  = 3;

    // BIGORBIT1   // orbit
    //       earth1         2           3           moon/sun
    sky              = tileSetupSky(BIGORBIT1__DYN);
    sky->lognumtiles = 3;
    sky->horizfrac   = 32768;
    sky->tileofs[5]  = 1;
    sky->tileofs[6]  = 2;
    sky->tileofs[7]  = 3;
    sky->tileofs[2]  = 4;

    // LA // la city
    //       earth1         2           3           moon/sun
    sky              = tileSetupSky(LA__DYN);
    sky->lognumtiles = 3;
    sky->horizfrac   = 16384 + 1024;
    sky->tileofs[0]  = 1;
    sky->tileofs[1]  = 2;
    sky->tileofs[2]  = 1;
    sky->tileofs[3]  = 3;
    sky->tileofs[4]  = 4;
    sky->tileofs[5]  = 0;
    sky->tileofs[6]  = 2;
    sky->tileofs[7]  = 3;

#if 0
    // This assertion should hold. See note above.
    for (bssize_t i=0; i<pskynummultis; ++i)
        for (bssize_t j=0; j<(1<<multipsky[i].lognumtiles); ++j)
            Bassert(multipsky[i].tileofs[j] <= PSKYOFF_MAX);
#endif
}

void G_SetupGlobalPsky(void)
{
    int skyIdx = 0;

    // NOTE: Loop must be running backwards for the same behavior as the game
    // (greatest sector index with matching parallaxed sky takes precedence).
    for (bssize_t i = numsectors - 1; i >= 0; i--)
    {
        if (sector[i].ceilingstat & 1)
        {
            skyIdx = getpskyidx(sector[i].ceilingpicnum);
            if (skyIdx > 0)
                break;
        }
    }

    g_pskyidx = skyIdx;
}

//////////

void G_LoadLookups(void)
{
    int32_t j;

	auto fr = fileSystem.OpenFileReader("lookup.dat");
	if (!fr.isOpen())
           return;

    j = paletteLoadLookupTable(fr);

    if (j < 0)
    {
        if (j == -1)
            Printf("ERROR loading \"lookup.dat\": failed reading enough data.\n");

        return;
    }

    uint8_t paldata[768];

    for (j=1; j<=5; j++)
    {
        // Account for TITLE and REALMS swap between basepal number and on-disk order.
        int32_t basepalnum = (j == 3 || j == 4) ? 4+3-j : j;

		if (fr.Read(paldata, 768) != 768)
			return;

        for (bssize_t k = 0; k < 768; k++)
            paldata[k] <<= 2;

        paletteSetColorTable(basepalnum, paldata, basepalnum == DREALMSPAL || basepalnum == ENDINGPAL);
    }

    for (int i = 0; i < 256; i++)
    {
        // swap red and blue channels.
        paldata[i * 3] = GPalette.BaseColors[i].b;
        paldata[i * 3+1] = GPalette.BaseColors[i].g;
        paldata[i * 3+2] = GPalette.BaseColors[i].r;
    }
    paletteSetColorTable(DRUGPAL, paldata); // todo: implement this as a shader effect

    if (RR)
    {
        char table[256];
        for (bssize_t i = 0; i < 256; i++)
            table[i] = i;
        for (bssize_t i = 0; i < 32; i++)
            table[i] = i+32;

        paletteMakeLookupTable(7, table, 0, 0, 0, 0);

        for (bssize_t i = 0; i < 256; i++)
            table[i] = i;
        paletteMakeLookupTable(30, table, 0, 0, 0, 0);
        paletteMakeLookupTable(31, table, 0, 0, 0, 0);
        paletteMakeLookupTable(32, table, 0, 0, 0, 0);
        paletteMakeLookupTable(33, table, 0, 0, 0, 0);
        if (RRRA)
            paletteMakeLookupTable(105, table, 0, 0, 0, 0);

        j = 63;
        for (bssize_t i = 64; i < 80; i++)
        {
            j--;
            table[i] = j;
            table[i+16] = i-24;
        }
        table[80] = 80;
        table[81] = 81;
        for (bssize_t i = 0; i < 32; i++)
            table[i] = i+32;
        paletteMakeLookupTable(34, table, 0, 0, 0, 0);
        for (bssize_t i = 0; i < 256; i++)
            table[i] = i;
        for (bssize_t i = 0; i < 16; i++)
            table[i] = i+129;
        for (bssize_t i = 16; i < 32; i++)
            table[i] = i+192;
        paletteMakeLookupTable(35, table, 0, 0, 0, 0);
        if (RRRA)
        {
            paletteMakeLookupTable(50, NULL, 12 * 4, 12 * 4, 12 * 4, 0);
            paletteMakeLookupTable(51, NULL, 12 * 4, 12 * 4, 12 * 4, 0);
            paletteMakeLookupTable(54, lookuptables[8], 32 * 4, 32 * 4, 32 * 4, 0);
        }
    }
}


END_RR_NS
