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

#include "vfs.h"

void	cacheAllocateBlock(intptr_t *newhandle, int32_t newbytes, uint8_t *newlockptr);

using buildvfs_kfd = int32_t;

extern int32_t pathsearchmode;	// 0 = gamefs mode (default), 1 = localfs mode (editor's mode)


// compression disabled pending a better process for saving. Per-block compression as done here was not that great.
int32_t kdfread_LZ4(void* buffer, int dasizeof, int count, buildvfs_kfd fil) = delete;

inline int32_t kdfread_LZ4(void* buffer, int dasizeof, int count, FileReader& fil)
{
	return fil.Read(buffer, dasizeof * count);
}

inline void dfwrite_LZ4(const void* buffer, int dasizeof, int count, buildvfs_FILE fil)
{
	fwrite(buffer, dasizeof, count, fil);
}


#include "filesystem/filesystem.h"

// Wrappers for the handle based API to get rid of the direct  calls without any actual changes to the implementation.
// These are now getting redirected to the file system so that the implementation here can be gutted without making changes to the calling code.
inline FileReader kopenFileReader(const char* name, int where)
{
	auto lump = fileSystem.FindFile(name);
	if (lump < 0) return FileReader();
	else return fileSystem.OpenFileReader(lump);
}

// This is only here to mark a file as not being part of the game assets (e.g. savegames)
// These should be handled differently (e.g read from a userdata directory or similar things.)
inline FileReader fopenFileReader(const char* name, int where)
{
	FileReader fr;
	fr.OpenFile(name);
	return fr;
}

inline bool testkopen(const char* name, int where)
{
	// todo: if backed by a single file, we must actually open it to make sure.
	return fileSystem.FindFile(name) >= 0;
}

inline TArray<uint8_t> kloadfile(const char* name, int where)
{
	auto lump = fileSystem.FindFile(name);
	if (lump < 0) return TArray<uint8_t>();
	return fileSystem.GetFileData(lump);
}

inline int32_t kfilesize(const char* name, int where)
{
	auto lump = fileSystem.FindFile(name);
	if (lump < 0) return -1;
	return fileSystem.FileLength(lump);
}

// checks from path and in ZIPs, returns 1 if NOT found
inline int32_t check_file_exist(const char* fn)
{
	return fileSystem.FindFile(fn) >= 0;
}

#endif // cache1d_h_

