// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#ifndef cache1d_h_
#define cache1d_h_

#include "compat.h"
#include "files.h"

void	cacheAllocateBlock(intptr_t *newhandle, int32_t newbytes, uint8_t *newlockptr);

extern int32_t pathsearchmode;	// 0 = gamefs mode (default), 1 = localfs mode (editor's mode)

#include "filesystem/filesystem.h"

// This is only here to mark a file as not being part of the game assets (e.g. savegames)
// These should be handled differently (e.g read from a userdata directory or similar things.)
inline FileReader fopenFileReader(const char* name, int where)
{
	FileReader fr;
	fr.OpenFile(name);
	return fr;
}

#endif // cache1d_h_

