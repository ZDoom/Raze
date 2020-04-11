/*
** resourcefile.cpp
**
** Base classes for resource file management
**
**---------------------------------------------------------------------------
** Copyright 2009 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include <zlib.h>
#include "resourcefile.h"
#include "name.h"
#include "m_swap.h"
#include "gamecontrol.h"

//==========================================================================
//
// File reader that reads from a lump's cache
//
//==========================================================================

class FLumpReader : public MemoryReader
{
	FResourceLump *source;

public:
	FLumpReader(FResourceLump *src)
		: MemoryReader(NULL, src->LumpSize), source(src)
	{
		bufptr = (const char*)src->Lock();
		src->Cache.Data();
	}

	~FLumpReader()
	{
		source->Unlock(true);
	}
};


//==========================================================================
//
// Base class for resource lumps
//
//==========================================================================

FResourceLump::~FResourceLump()
{
	Owner = NULL;
}


//==========================================================================
//
// Sets up the file name information
// This is stored as FNames for various formats.
//
//==========================================================================

void FResourceLump::LumpNameSetup(FString iname)
{
	auto pathLen = iname.LastIndexOf('/') + 1;
	LumpName[FullNameType] = iname.GetChars();
	LumpName[BaseNameType] = iname.GetChars() + pathLen;
	
	auto extStart = iname.LastIndexOf('.');
	if (extStart <= pathLen) extStart = -1;
	if (extStart > 0)
	{
		LumpName[ExtensionType] = iname.GetChars() + extStart + 1;
		iname.Truncate(extStart);
	}
	LumpName[FullNameNoExtType] = iname.GetChars();
	LumpName[BaseNameNoExtType] = iname.GetChars() + pathLen;
}

//==========================================================================
//
// this is just for completeness. For non-Zips only an uncompressed lump can
// be returned.
//
//==========================================================================

FCompressedBuffer FResourceLump::GetRawData()
{
	FCompressedBuffer cbuf = { (unsigned)LumpSize, (unsigned)LumpSize, METHOD_STORED, 0, 0, new char[LumpSize] };
	memcpy(cbuf.mBuffer, Lock(), LumpSize);
	cbuf.mCRC32 = crc32(0, (uint8_t*)cbuf.mBuffer, LumpSize);
	Unlock(true);
	return cbuf;
}


//==========================================================================
//
// Returns the owner's FileReader if it can be used to access this lump
//
//==========================================================================

FileReader *FResourceLump::GetReader()
{
	return NULL;
}

//==========================================================================
//
// Returns a file reader to the lump's cache
//
//==========================================================================

FileReader FResourceLump::NewReader()
{
	return FileReader(new FLumpReader(this));
}

//==========================================================================
//
// Caches a lump's content and increases the reference counter
//
//==========================================================================

void *FResourceLump::Lock()
{
	if (Cache.Size())
	{
		if (RefCount > 0) RefCount++;
	}
	else if (LumpSize > 0)
	{
		ValidateCache();
		// NBlood has some endian conversion right in here which is extremely dangerous and needs to be handled differently.
		// Fortunately Big Endian platforms are mostly irrelevant so this is something to be sorted out later (if ever)
		RefCount++;
	}
	return Cache.Data();
}

//==========================================================================
//
// Caches a lump's content without increasing the reference counter
//
//==========================================================================

void *FResourceLump::Get()
{
	if (Cache.Size() == 0)
	{
		ValidateCache();
	}
	return Cache.Data();
}

//==========================================================================
//
// Decrements reference counter and frees lump if counter reaches 0
//
//==========================================================================

void FResourceLump::Unlock(bool mayfree)
{
	if (LumpSize > 0 && RefCount > 0)
	{
		if (--RefCount == 0)
		{
			if (mayfree) Cache.Reset();
		}
	}
}

//==========================================================================
//
// Opens a resource file
//
//==========================================================================

typedef FResourceFile * (*CheckFunc)(const char *filename, FileReader &file, bool quiet);

FResourceFile *CheckWad(const char *filename, FileReader &file, bool quiet);
FResourceFile *CheckGRP(const char *filename, FileReader &file, bool quiet);
FResourceFile *CheckRFF(const char *filename, FileReader &file, bool quiet);
FResourceFile *CheckPak(const char *filename, FileReader &file, bool quiet);
FResourceFile *CheckZip(const char *filename, FileReader &file, bool quiet);
FResourceFile *Check7Z(const char *filename,  FileReader &file, bool quiet);
FResourceFile *CheckLump(const char *filename,FileReader &file, bool quiet);
FResourceFile *CheckDir(const char *filename, bool quiet, bool nosubdirflag);

static CheckFunc funcs[] = { CheckGRP, CheckRFF, CheckZip, Check7Z, CheckPak, CheckLump };

FResourceFile *FResourceFile::DoOpenResourceFile(const char *filename, FileReader &file, bool quiet, bool containeronly)
{
	for(size_t i = 0; i < countof(funcs) - containeronly; i++)
	{
		FResourceFile *resfile = funcs[i](filename, file, quiet);
		if (resfile != NULL) return resfile;
	}
	return NULL;
}

FResourceFile *FResourceFile::OpenResourceFile(const char *filename, FileReader &file, bool quiet, bool containeronly)
{
	return DoOpenResourceFile(filename, file, quiet, containeronly);
}


FResourceFile *FResourceFile::OpenResourceFile(const char *filename, bool quiet, bool containeronly)
{
	FileReader file;
	if (!file.OpenFile(filename)) return nullptr;
	return DoOpenResourceFile(filename, file, quiet, containeronly);
}

/*
FResourceFile *FResourceFile::OpenResourceFileFromLump(int lumpnum, bool quiet, bool containeronly)
{
	FileReader file = Wads.ReopenLumpReader(lumpnum);
	return DoOpenResourceFile("internal", file, quiet, containeronly);
}
*/

FResourceFile *FResourceFile::OpenDirectory(const char *filename, bool quiet, bool nosubdirflag)
{
	return CheckDir(filename, quiet, nosubdirflag);
}

//==========================================================================
//
// Resource file base class
//
//==========================================================================

FResourceFile::FResourceFile(const char *filename)
	: FileName(filename)
{
}

FResourceFile::FResourceFile(const char *filename, FileReader &r)
	: FResourceFile(filename)
{
	Reader = std::move(r);
}

FResourceFile::~FResourceFile()
{
}

int lumpcmp(const void * a, const void * b)
{
	FResourceLump * rec1 = (FResourceLump *)a;
	FResourceLump * rec2 = (FResourceLump *)b;
	return stricmp(rec1->LumpName[FResourceLump::FullNameType].GetChars(), rec2->LumpName[FResourceLump::FullNameType].GetChars());
}

//==========================================================================
//
// FResourceFile :: PostProcessArchive
//
// Sorts files by name.
// For files named "filter/<game>/*": Using the same filter rules as config
// autoloading, move them to the end and rename them without the "filter/"
// prefix. Filtered files that don't match are deleted.
//
//==========================================================================

void FResourceFile::PostProcessArchive(void *lumps, size_t lumpsize)
{
	// Entries in archives are sorted alphabetically
	qsort(lumps, NumLumps, lumpsize, lumpcmp);
	

	// Filter out lumps using the same names as the Autoload.* sections
	// in the ini file use. We reduce the maximum lump concidered after
	// each one so that we don't risk refiltering already filtered lumps.
	uint32_t max = NumLumps;

	int lastpos = -1;
	FString file;

	auto segments = LumpFilter.Split(".");
	FString build;

	for (auto& segment : segments)
	{
		if (build.IsEmpty()) build = segment;
		else build << "." << segment;
		max -= FilterLumps(build, lumps, lumpsize, max);
	}
	JunkLeftoverFilters(lumps, lumpsize, max);
}

//==========================================================================
//
// FResourceFile :: FilterLumps
//
// Finds any lumps between [0,<max>) that match the pattern
// "filter/<filtername>/*" and moves them to the end of the lump list.
// Returns the number of lumps moved.
//
//==========================================================================

int FResourceFile::FilterLumps(FString filtername, void *lumps, size_t lumpsize, uint32_t max)
{
	FString filter;
	uint32_t start, end;

	if (filtername.IsEmpty())
	{
		return 0;
	}
	filter << "filter/" << filtername << '/';
	
	bool found = FindPrefixRange(filter, lumps, lumpsize, max, start, end);
	
	if (found)
	{
		void *from = (uint8_t *)lumps + start * lumpsize;

		// Remove filter prefix from every name
		void *lump_p = from;
		for (uint32_t i = start; i < end; ++i, lump_p = (uint8_t *)lump_p + lumpsize)
		{
			FResourceLump *lump = (FResourceLump *)lump_p;
			assert(filter.CompareNoCase(lump->FullName(), (int)filter.Len()) == 0);
			lump->LumpNameSetup(lump->FullName() + filter.Len());
		}

		// Move filtered lumps to the end of the lump list.
		size_t count = (end - start) * lumpsize;
		void *to = (uint8_t *)lumps + NumLumps * lumpsize - count;
		assert (to >= from);

		if (from != to)
		{
			// Copy filtered lumps to a temporary buffer.
			uint8_t *filteredlumps = new uint8_t[count];
			memcpy(filteredlumps, from, count);

			// Shift lumps left to make room for the filtered ones at the end.
			memmove(from, (uint8_t *)from + count, (NumLumps - end) * lumpsize);

			// Copy temporary buffer to newly freed space.
			memcpy(to, filteredlumps, count);

			delete[] filteredlumps;
		}
	}
	return end - start;
}

//==========================================================================
//
// FResourceFile :: JunkLeftoverFilters
//
// Deletes any lumps beginning with "filter/" that were not matched.
//
//==========================================================================

void FResourceFile::JunkLeftoverFilters(void *lumps, size_t lumpsize, uint32_t max)
{
	uint32_t start, end;
	if (FindPrefixRange("filter/", lumps, lumpsize, max, start, end))
	{
		// Since the resource lumps may contain non-POD data besides the
		// full name, we "delete" them by erasing their names so they
		// can't be found.
		void *stop = (uint8_t *)lumps + end * lumpsize;
		for (void *p = (uint8_t *)lumps + start * lumpsize; p < stop; p = (uint8_t *)p + lumpsize)
		{
			FResourceLump *lump = (FResourceLump *)p;
			for (auto &ln : lump->LumpName)
				ln = NAME_None;
		}
	}
}

//==========================================================================
//
// FResourceFile :: FindPrefixRange
//
// Finds a range of lumps that start with the prefix string. <start> is left
// indicating the first matching one. <end> is left at one plus the last
// matching one.
//
//==========================================================================

bool FResourceFile::FindPrefixRange(FString filter, void *lumps, size_t lumpsize, uint32_t maxlump, uint32_t &start, uint32_t &end)
{
	uint32_t min, max, mid, inside;
	FResourceLump *lump;
	int cmp;

	end = start = 0;

	// Pretend that our range starts at 1 instead of 0 so that we can avoid
	// unsigned overflow if the range starts at the first lump.
	lumps = (uint8_t *)lumps - lumpsize;

	// Binary search to find any match at all.
	min = 1, max = maxlump;
	while (min <= max)
	{
		mid = min + (max - min) / 2;
		lump = (FResourceLump *)((uint8_t *)lumps + mid * lumpsize);
		cmp = filter.CompareNoCase(lump->FullName(), (int)filter.Len());

		if (cmp == 0)
			break;
		else if (cmp > 0)
			min = mid + 1;
		else		
			max = mid - 1;
	}
	if (max < min)
	{ // matched nothing
		return false;
	}

	// Binary search to find first match.
	inside = mid;
	min = 1, max = mid;
	while (min <= max)
	{
		mid = min + (max - min) / 2;
		lump = (FResourceLump *)((uint8_t *)lumps + mid * lumpsize);
		cmp = filter.CompareNoCase(lump->FullName(), (int)filter.Len());
		// Go left on matches and right on misses.
		if (cmp == 0)
			max = mid - 1;
		else
			min = mid + 1;
	}
	start = mid + (cmp != 0) - 1;

	// Binary search to find last match.
	min = inside, max = maxlump;
	while (min <= max)
	{
		mid = min + (max - min) / 2;
		lump = (FResourceLump *)((uint8_t *)lumps + mid * lumpsize);
		cmp = filter.CompareNoCase(lump->FullName(), (int)filter.Len());
		// Go right on matches and left on misses.
		if (cmp == 0)
			min = mid + 1;
		else
			max = mid - 1;
	}
	end = mid - (cmp != 0);
	return true;
}

//==========================================================================
//
// Finds a lump by a given name. Used for savegames
//
//==========================================================================

FResourceLump *FResourceFile::FindLump(const char *name)
{
	FName lname(name, true);
	if (lname == NAME_None) return nullptr;
	for (unsigned i = 0; i < NumLumps; i++)
	{
		FResourceLump *lump = GetLump(i);
		if (lump->LumpName[FResourceLump::FullNameType] == lname)
		{
			return lump;
		}
	}
	return nullptr;
}

//==========================================================================
//
// Caches a lump's content and increases the reference counter
//
//==========================================================================

FileReader *FUncompressedLump::GetReader()
{
	Owner->Reader.Seek(Position, FileReader::SeekSet);
	return &Owner->Reader;
}

//==========================================================================
//
// Caches a lump's content and increases the reference counter
//
//==========================================================================

int FUncompressedLump::ValidateCache()
{
	Owner->Reader.Seek(Position, FileReader::SeekSet);
	Cache.Resize(LumpSize);
	Owner->Reader.Read(Cache.Data(), LumpSize);
	return 1;
}

//==========================================================================
//
// Base class for uncompressed resource files
//
//==========================================================================

FUncompressedFile::FUncompressedFile(const char *filename)
: FResourceFile(filename)
{}

FUncompressedFile::FUncompressedFile(const char *filename, FileReader &r)
	: FResourceFile(filename, r)
{}


//==========================================================================
//
// external lump
//
//==========================================================================

FExternalLump::FExternalLump(const char *_filename, int filesize)
	: Filename(_filename)
{
	if (filesize == -1)
	{
		FileReader f;

		if (f.OpenFile(_filename))
		{
			LumpSize = (int)f.GetLength();
		}
		else
		{
			LumpSize = 0;
		}
	}
	else
	{
		LumpSize = filesize;
	}
}


//==========================================================================
//
// Caches a lump's content and increases the reference counter
// For external lumps this reopens the file each time it is accessed
//
//==========================================================================

int FExternalLump::ValidateCache()
{
	Cache.Resize(LumpSize);
	FileReader f;

	if (f.OpenFile(Filename))
	{
		f.Read(Cache.Data(), LumpSize);
	}
	else
	{
		memset(Cache.Data(), 0, LumpSize);
	}
	return 1;
}
