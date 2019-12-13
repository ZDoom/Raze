// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#include <stdint.h>
#include "tarray.h"
#include "cache1d.h"

// Only the sound code still uses this - but it never frees the data.
// So we may just toss out the cache and do regular allocations.
// The TArray is merely for taking down the data before shutdown.
static TArray<TArray<uint8_t>> pseudocache;

void cacheAllocateBlock(intptr_t *newhandle, int32_t newbytes, uint8_t *)
{
	pseudocache.Reserve(1);
	auto& buffer = pseudocache.Last();
	buffer.Resize(newbytes);
	*newhandle = reinterpret_cast<intptr_t>(buffer.Data());
}

