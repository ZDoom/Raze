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
		src->CacheLump();
		bufptr = src->Cache;
	}

	~FLumpReader()
	{
		source->ReleaseCache();
	}
};


//==========================================================================
//
// Base class for resource lumps
//
//==========================================================================

FResourceLump::~FResourceLump()
{
	if (Cache != NULL && RefCount >= 0)
	{
		delete [] Cache;
		Cache = NULL;
	}
	Owner = NULL;
}


//==========================================================================
//
// Sets up the lump name information for anything not coming from a WAD file.
//
//==========================================================================

void FResourceLump::LumpNameSetup(FString iname)
{
	long slash = iname.LastIndexOf('/');
	FString base = (slash >= 0) ? iname.Mid(slash + 1) : iname;
	auto dot = base.LastIndexOf('.');
	if (dot >= 0) base.Truncate(dot);
	FullName = iname;
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

void *FResourceLump::CacheLump()
{
	if (Cache != NULL)
	{
		if (RefCount > 0) RefCount++;
	}
	else if (LumpSize > 0)
	{
		FillCache();
	}
	return Cache;
}

//==========================================================================
//
// Decrements reference counter and frees lump if counter reaches 0
//
//==========================================================================

int FResourceLump::ReleaseCache()
{
	if (LumpSize > 0 && RefCount > 0)
	{
		if (--RefCount == 0)
		{
			delete [] Cache;
			Cache = NULL;
		}
	}
	return RefCount;
}

//==========================================================================
//
// Opens a resource file
//
//==========================================================================

typedef FResourceFile * (*CheckFunc)(const char *filename, FileReader &file, bool quiet);

FResourceFile *CheckZip(const char *filename, FileReader &file, bool quiet);


FResourceFile *FResourceFile::OpenResourceFile(const char *filename, FileReader &file, bool quiet, bool containeronly)
{
	return CheckZip(filename, file, quiet);
}


FResourceFile *FResourceFile::OpenResourceFile(const char *filename, bool quiet, bool containeronly)
{
	FileReader file;
	if (!file.OpenFile(filename)) return nullptr;
	return OpenResourceFile(filename, file, quiet, containeronly);
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

	return rec1->FullName.CompareNoCase(rec2->FullName);
}


//==========================================================================
//
// Finds a lump by a given name. Used for savegames
//
//==========================================================================

FResourceLump *FResourceFile::FindLump(const char *name)
{
	for (unsigned i = 0; i < NumLumps; i++)
	{
		FResourceLump *lump = GetLump(i);
		if (!stricmp(name, lump->FullName))
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

int FUncompressedLump::FillCache()
{
	const char * buffer = Owner->Reader.GetBuffer();

	if (buffer != NULL)
	{
		// This is an in-memory file so the cache can point directly to the file's data.
		Cache = const_cast<char*>(buffer) + Position;
		RefCount = -1;
		return -1;
	}

	Owner->Reader.Seek(Position, FileReader::SeekSet);
	Cache = new char[LumpSize];
	Owner->Reader.Read(Cache, LumpSize);
	RefCount = 1;
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


