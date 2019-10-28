/*
** filesystem.cpp
**
**---------------------------------------------------------------------------
** Copyright 1998-2009 Randy Heit
** Copyright 2005-2019 Christoph Oelckers
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


// HEADER FILES ------------------------------------------------------------

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "m_argv.h"
#include "cmdlib.h"
#include "printf.h"
//#include "c_dispatch.h"
#include "filesystem.h"
#include "superfasthash.h"
#include "resourcefile.h"
//#include "md5.h"
//#include "doomstat.h"

// MACROS ------------------------------------------------------------------

#define NULL_INDEX		(0xffffffff)

struct FileSystem::FileRecord
{
	int			rfnum;
	FResourceLump *lump;
};

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void PrintLastError ();

// PUBLIC DATA DEFINITIONS -------------------------------------------------

FileSystem Files;

// CODE --------------------------------------------------------------------


FileSystem::~FileSystem ()
{
	DeleteAll();
}

void FileSystem::DeleteAll ()
{
	FileInfo.Clear();
	NumEntries = 0;

	for (int i = Files.Size() - 1; i >= 0; --i)
	{
		delete Files[i];
	}
	Files.Clear();
}

//==========================================================================
//
// InitMultipleFiles
//
// Pass a null terminated list of files to use. All files are optional,
// but at least one file must be found. Lump names can appear multiple
// times. The name searcher looks backwards, so a later file can
// override an earlier one.
//
//==========================================================================

int FileSystem::InitMultipleFiles (TArray<FString> &filenames, const TArray<FString> &deletelumps)
{
	int numfiles;

	// open all the files, load headers, and count lumps
	DeleteAll();
	numfiles = 0;

	for(unsigned i=0;i<filenames.Size(); i++)
	{
		int baselump = NumEntries;
		AddFile (filenames[i]);
	}
	
	NumEntries = FileInfo.Size();
	if (NumEntries == 0)
	{
		return 0;
	}

	// [RH] Set up hash table
	Hashes.Resize(8 * NumEntries);
	FirstFileIndex_BaseName = &Hashes[0];
	NextFileIndex_BaseName = &Hashes[NumEntries];
	FirstFileIndex_FullName = &Hashes[NumEntries*2];
	NextFileIndex_FullName = &Hashes[NumEntries*3];
	FirstFileIndex_NoExt = &Hashes[NumEntries*4];
	NextFileIndex_NoExt = &Hashes[NumEntries*5];
	FirstFileIndex_BaseExt = &Hashes[NumEntries*6];
	NextFileIndex_BaseExt = &Hashes[NumEntries*7];
	InitHashChains ();
	FileInfo.ShrinkToFit();
	Files.ShrinkToFit();
	return NumEntries;
}

//==========================================================================
//
// AddFile
//
//==========================================================================

void FileSystem::AddFile (const char *filename, FileReader *filer)
{
	int startlump;
	bool isdir = false;
	FileReader fr;

	if (filer == nullptr)
	{
		// Does this exist? If so, is it a directory?
		if (!DirEntryExists(filename, &isdir))
		{
			Printf("%s: File or Directory not found\n", filename);
			PrintLastError();
			return;
		}

		if (!isdir)
		{
			if (!fr.OpenFile(filename))
			{ // Didn't find file
				Printf ("%s: File not found\n", filename);
				PrintLastError ();
				return;
			}
		}
	}
	else fr = std::move(*filer);

	Printf (" adding %s", filename);
	startlump = NumEntries;

	FResourceFile *resfile;
	
	if (!isdir)
		resfile = FResourceFile::OpenResourceFile(filename, fr);
	else
		resfile = FResourceFile::OpenDirectory(filename);

	if (resfile != NULL)
	{
		uint32_t lumpstart = FileInfo.Size();

		resfile->SetFirstLump(lumpstart);
		for (uint32_t i=0; i < resfile->LumpCount(); i++)
		{
			FResourceLump *lump = resfile->GetLump(i);
			FileSystem::FileRecord *lump_p = &FileInfo[FileInfo.Reserve(1)];

			lump_p->lump = lump;
			lump_p->rfnum = Files.Size();
		}

		Files.Push(resfile);
	}
}

//==========================================================================
//
// GetFileRecordCheckIfWadLoaded
//
// Returns true if the specified wad is loaded, false otherwise.
// If a fully-qualified path is specified, then the wad must match exactly.
// Otherwise, any wad with that name will work, whatever its path.
// Returns the wads index if found, or -1 if not.
//
//==========================================================================

int FileSystem::CheckIfResourceFileLoaded (const char *name) noexcept
{
	unsigned int i;

	if (strrchr (name, '/') != NULL)
	{
		for (i = 0; i < Files.Size(); ++i)
		{
			if (stricmp (GetResourceFileFullName (i), name) == 0)
			{
				return i;
			}
		}
	}
	else
	{
		for (i = 0; i < Files.Size(); ++i)
		{
			if (stricmp (GetResourceFileName (i), name) == 0)
			{
				return i;
			}
		}
	}
	return -1;
}

//==========================================================================
//
// GetFileRecordFindFile
//
// Same as above but looks for a fully qualified name from a .zip
// These don't care about namespaces though because those are part
// of the path.
//
//==========================================================================

int FileSystem::FindFile (const char *name, ELookupMode lookupmode, int filenum) const noexcept
{
	uint32_t i;

	if (name == NULL)
	{
		return -1;
	}
	uint32_t* fli;
	uint32_t* nli;

	switch (lookupmode)
	{
	case ELookupMode::FullName:
		fli = FirstFileIndex_FullName;
		nli = NextFileIndex_FullName;
		break;

	case ELookupMode::NoExtension:
		fli = FirstFileIndex_NoExt;
		nli = NextFileIndex_NoExt;
		break;

	case ELookupMode::BaseName:
		fli = FirstFileIndex_BaseName;
		nli = NextFileIndex_BaseName;
		break;

	case ELookupMode::BaseWithExtension:
		fli = FirstFileIndex_BaseExt;
		nli = NextFileIndex_BaseExt;
		break;

	}
	auto len = strlen(name);

	for (i = fli[MakeKey(name) % NumEntries]; i != NULL_INDEX; i = nli[i])
	{
		auto lump = FileInfo[i].lump;
		if (FileInfo[i].rfnum != filenum) continue;
		const char* fn = lump->FullName.GetChars();
		const char* fnstart, * fnend;
		if (lookupmode == ELookupMode::BaseName || lookupmode == ELookupMode::BaseWithExtension) fnstart = fn + lump->PathLen;
		else fnstart = fn;

		if ((lookupmode == ELookupMode::NoExtension || lookupmode == ELookupMode::BaseName) && lump->ExtStart >= 0) fnend = fn + lump->ExtStart;
		else fnend = fn + lump->FullName.Len();

		if ((fnend - fnstart) == (ptrdiff_t)len)
		{
			if (!strnicmp(name, fnstart, len))
			{
				return i;
			}
		}
	}
	return -1;
}

//==========================================================================
//
// GetFileRecordGetFile
//
// Calls GetFileRecordFindFile, but bombs out if not found.
//
//==========================================================================

int FileSystem::GetFile (const char *name, ELookupMode lookupmode, int filenum) const
{
	int	i;

	i = FindFile (name, lookupmode, filenum);

	if (i == -1)
	{
		FStringf error("GetFile: %s not found!", name);
		throw FileSystemError(error.GetChars());
	}
	return i;
}

//==========================================================================
//
// GetFileRecordLumpLength
//
// Returns the buffer size needed to load the given lump.
//
//==========================================================================

int FileSystem::FileLength (int lump) const
{
	if ((size_t)lump >= NumEntries)
	{
		return -1;
	}
	return FileInfo[lump].lump->LumpSize;
}

//==========================================================================
//
// GetLumpOffset
//
// Returns the offset from the beginning of the file to the lump.
// Returns -1 if the lump is compressed or can't be read directly
//
//==========================================================================

int FileSystem::GetFileOffset (int lump)
{
	if ((size_t)lump >= NumEntries)
	{
		return -1;
	}
	return FileInfo[lump].lump->GetFileOffset();
}

//==========================================================================
//
// GetLumpOffset
//
//==========================================================================

int FileSystem::GetFileFlags (int lump)
{
	if ((size_t)lump >= NumEntries)
	{
		return 0;
	}

	return FileInfo[lump].lump->Flags;
}

//==========================================================================
//
// GetFileRecordInitHashChains
//
// Prepares the lumpinfos for hashing.
// (Hey! This looks suspiciously like something from Boom! :-)
//
//==========================================================================

void FileSystem::InitHashChains (void)
{
	char name[8];
	unsigned int i, j;

	// Mark all buckets as empty
	memset(FirstFileIndex_BaseExt, 255, NumEntries * sizeof(FirstFileIndex_BaseExt[0]));
	memset(NextFileIndex_BaseExt, 255, NumEntries * sizeof(NextFileIndex_BaseExt[0]));
	memset (FirstFileIndex_BaseName, 255, NumEntries*sizeof(FirstFileIndex_BaseName[0]));
	memset (NextFileIndex_BaseName, 255, NumEntries*sizeof(NextFileIndex_BaseName[0]));
	memset (FirstFileIndex_FullName, 255, NumEntries*sizeof(FirstFileIndex_FullName[0]));
	memset (NextFileIndex_FullName, 255, NumEntries*sizeof(NextFileIndex_FullName[0]));
	memset(FirstFileIndex_NoExt, 255, NumEntries * sizeof(FirstFileIndex_NoExt[0]));
	memset(NextFileIndex_NoExt, 255, NumEntries * sizeof(NextFileIndex_NoExt[0]));

	// Now set up the chains
	for (i = 0; i < (unsigned)NumEntries; i++)
	{

		// Do the same for the full paths
		auto lump = FileInfo[i].lump;
		auto& Name = lump->FullName;
		if (Name.IsNotEmpty())
		{
			j = MakeKey(Name) % NumEntries;
			NextFileIndex_FullName[i] = FirstFileIndex_FullName[j];
			FirstFileIndex_FullName[j] = i;

			j = MakeKey(Name + lump->PathLen) % NumEntries;
			NextFileIndex_BaseExt[i] = FirstFileIndex_BaseExt[j];
			FirstFileIndex_BaseExt[j] = i;

			j = MakeKey(Name, lump->ExtStart) % NumEntries;
			NextFileIndex_NoExt[i] = FirstFileIndex_NoExt[j];
			FirstFileIndex_NoExt[j] = i;

			if (lump->ExtStart > lump->PathLen)
			{
				j = MakeKey(Name, lump->ExtStart) % NumEntries;
				NextFileIndex_NoExt[i] = FirstFileIndex_NoExt[j];
				FirstFileIndex_NoExt[j] = i;

				j = MakeKey(Name + lump->PathLen, lump->ExtStart - lump->PathLen) % NumEntries;
				NextFileIndex_BaseName[i] = FirstFileIndex_BaseName[j];
				FirstFileIndex_BaseName[j] = i;
			}
			else
			{
				NextFileIndex_NoExt[i] = NextFileIndex_FullName[i];
				FirstFileIndex_NoExt[i] = FirstFileIndex_FullName[i];

				NextFileIndex_BaseName[i] = NextFileIndex_BaseExt[i];
				FirstFileIndex_BaseName[j] = FirstFileIndex_BaseExt[i];
			}

			FString nameNoExt = Name;
			auto dot = nameNoExt.LastIndexOf('.');
			auto slash = nameNoExt.LastIndexOf('/');
			if (dot > slash) nameNoExt.Truncate(dot);

			j = MakeKey(nameNoExt) % NumEntries;
			NextFileIndex_NoExt[i] = FirstFileIndex_NoExt[j];
			FirstFileIndex_NoExt[j] = i;

		}
	}
}


//==========================================================================
//
// Iterate
//
// Find a named lump. Specifically allows duplicates for merging of e.g.
// SNDINFO lumps.
//
//==========================================================================

int FileSystem::Iterate (const char *name, int *lastlump, ELookupMode lookupmode)
{
	union
	{
		char name8[8];
		uint64_t qname;
	};
	FileRecord *lump_p;


	assert(lastlump != NULL && *lastlump >= 0);
	lump_p = &FileInfo[*lastlump];
	auto len = strlen(name);
	while (lump_p < &FileInfo[NumEntries])
	{
		auto lump = lump_p->lump;
		const char* fn = lump->FullName.GetChars();
		const char* fnstart, * fnend;
		if (lookupmode == ELookupMode::BaseName || lookupmode == ELookupMode::BaseWithExtension) fnstart = fn + lump->PathLen;
		else fnstart = fn;

		if ((lookupmode == ELookupMode::NoExtension || lookupmode == ELookupMode::BaseName) && lump->ExtStart >= 0) fnend = fn + lump->ExtStart;
		else fnend = fn + lump->FullName.Len();

		if ((fnend - fnstart) == (ptrdiff_t)len)
		{
			if (!strnicmp(name, fnstart, len))
			{
				int lump = int(lump_p - &FileInfo[0]);
				*lastlump = lump + 1;
				return lump;
			}
		}
		lump_p++;
	}
	*lastlump = NumEntries;
	return -1;
}

//==========================================================================
//
// GetFileRecordGetLumpName
//
//==========================================================================

const char *FileSystem::GetFileName (int lump) const
{
	if ((size_t)lump >= NumEntries)
		return nullptr;
	else
		return FileInfo[lump].lump->FullName;
}

//==========================================================================
//
// FileSystem :: GetLumpFullPath
//
// Returns the name of the lump's wad prefixed to the lump's full name.
//
//==========================================================================

FString FileSystem::GetFileFullPath(int lump) const
{
	FString foo;

	if ((size_t) lump <  NumEntries)
	{
		foo << GetResourceFileName(FileInfo[lump].rfnum) << ':' << GetFileName(lump);
	}
	return foo;
}

//==========================================================================
//
// FileSystem :: GetFileIndexNum
//
// Returns the index number for this lump. This is *not* the lump's position
// in the lump directory, but rather a special value that RFF can associate
// with files. Other archive types will return 0, since they don't have it.
//
//==========================================================================

int FileSystem::GetRFFIndexNum(int lump) const
{
	if ((size_t)lump >= NumEntries)
		return 0;
	else
		return FileInfo[lump].lump->GetIndexNum();
}

//==========================================================================
//
// GetFileRecordGetLumpFile
//
//==========================================================================

int FileSystem::GetFileContainer (int lump) const
{
	if ((size_t)lump >= FileInfo.Size())
		return -1;
	return FileInfo[lump].rfnum;
}

//==========================================================================
//
// GetLumpsInFolder
// 
// Gets all lumps within a single folder in the hierarchy.
// If 'atomic' is set, it treats folders as atomic, i.e. only the
// content of the last found resource file having the given folder name gets used.
//
//==========================================================================

static int folderentrycmp(const void *a, const void *b)
{
	auto A = (FolderEntry*)a;
	auto B = (FolderEntry*)b;
	return strcmp(A->name, B->name);
}

//==========================================================================
//
// 
//
//==========================================================================

unsigned FileSystem::GetFilesInFolder(const char *inpath, TArray<FolderEntry> &result, bool atomic) const
{
	FString path = inpath;
	path.Substitute("\\", "/");
	path.ToLower();
	if (path[path.Len() - 1] != '/') path += '/';
	result.Clear();
	for (unsigned i = 0; i < FileInfo.Size(); i++)
	{
		if (FileInfo[i].lump->FullName.IndexOf(path) == 0)
		{
			// Only if it hasn't been replaced.
			if ((unsigned)FindFile(FileInfo[i].lump->FullName) == i)
			{
				result.Push({ FileInfo[i].lump->FullName.GetChars(), i });
			}
		}
	}
	if (result.Size())
	{
		int maxfile = -1;
		if (atomic)
		{
			// Find the highest resource file having content in the given folder.
			for (auto & entry : result)
			{
				int thisfile = FileInfo[entry.lumpnum].rfnum;
				if (thisfile > maxfile) maxfile = thisfile;
			}
			// Delete everything from older files.
			for (int i = result.Size() - 1; i >= 0; i--)
			{
				if (FileInfo[result[i].lumpnum].rfnum != maxfile) result.Delete(i);
			}
		}
		qsort(result.Data(), result.Size(), sizeof(FolderEntry), folderentrycmp);
	}
	return result.Size();
}


//==========================================================================
//
// GetFileRecordReadFile
//
// Loads the lump into a TArray and returns it.
//
//==========================================================================

TArray<uint8_t> FileSystem::GetFileData(int lump, int pad)
{
	auto lumpr = OpenFileReader(lump);
	auto size = lumpr.GetLength();
	TArray<uint8_t> data(size + pad, true);
	auto numread = lumpr.Read(data.Data(), size);

	if (numread != size)
	{
		FStringf err("GetFileRecordReadFile: only read %ld of %ld on lump %i\n", numread, size, lump);
		throw FileSystemError(err);
	}
	if (pad > 0) memset(&data[size], 0, pad);
	return data;
}

//==========================================================================
//
// ReadFile - variant 2
//
// Loads the lump into a newly created buffer and returns it.
//
//==========================================================================

FileData FileSystem::ReadFile (int lump)
{
	return FileData(FString(ELumpNum(lump)));
}

//==========================================================================
//
// OpenLumpReader
//
// uses a more abstract interface to allow for easier low level optimization later
//
//==========================================================================


FileReader FileSystem::OpenFileReader(int lump)
{
	if ((unsigned)lump >= (unsigned)FileInfo.Size())
	{
		FStringf err("OpenFileReader: %u >= NumEntries", lump);
		throw FileSystemError(err);
	}

	auto rl = FileInfo[lump].lump;
	auto rd = rl->GetReader();

	if (rl->RefCount == 0 && rd != nullptr && !rd->GetBuffer() && !(rl->Flags & (LUMPF_BLOODCRYPT | LUMPF_COMPRESSED)))
	{
		FileReader rdr;
		rdr.OpenFilePart(*rd, rl->GetFileOffset(), rl->LumpSize);
		return rdr;
	}
	return rl->NewReader();	// This always gets a reader to the cache
}

FileReader FileSystem::ReopenFileReader(int lump, bool alwayscache)
{
	if ((unsigned)lump >= (unsigned)FileInfo.Size())
	{
		FStringf err("ReopenFileReader: %u >= NumEntries", lump);
		throw FileSystemError(err);
	}

	auto rl = FileInfo[lump].lump;
	auto rd = rl->GetReader();

	if (rl->RefCount == 0 && rd != nullptr && !rd->GetBuffer() && !alwayscache && !(rl->Flags & (LUMPF_BLOODCRYPT|LUMPF_COMPRESSED)))
	{
		int fileno = FileInfo[lump].rfnum;
		const char *filename = GetResourceFileFullName(fileno);
		FileReader fr;
		if (fr.OpenFile(filename, rl->GetFileOffset(), rl->LumpSize))
		{
			return fr;
		}
	}
	return rl->NewReader();	// This always gets a reader to the cache
}

//==========================================================================
//
// GetFileRecordGetResourceFileName
//
// Returns the name of the given wad.
//
//==========================================================================

const char *FileSystem::GetResourceFileName (int rfnum) const noexcept
{
	const char *name, *slash;

	if ((uint32_t)rfnum >= Files.Size())
	{
		return NULL;
	}

	name = Files[rfnum]->FileName;
	slash = strrchr (name, '/');
	return slash != NULL ? slash+1 : name;
}

//==========================================================================
//
//
//==========================================================================

int FileSystem::GetFirstEntry (int rfnum) const noexcept
{
	if ((uint32_t)rfnum >= Files.Size())
	{
		return 0;
	}

	return Files[rfnum]->GetFirstLump();
}

//==========================================================================
//
//
//==========================================================================

int FileSystem::GetLastEntry (int rfnum) const noexcept
{
	if ((uint32_t)rfnum >= Files.Size())
	{
		return 0;
	}

	return Files[rfnum]->GetFirstLump() + Files[rfnum]->LumpCount() - 1;
}

//==========================================================================
//
//
//==========================================================================

int FileSystem::GetEntryCount (int rfnum) const noexcept
{
	if ((uint32_t)rfnum >= Files.Size())
	{
		return 0;
	}
	
	return Files[rfnum]->LumpCount();
}


//==========================================================================
//
// GetFileRecordGetResourceFileFullName
//
// Returns the name of the given wad, including any path
//
//==========================================================================

const char *FileSystem::GetResourceFileFullName (int rfnum) const noexcept
{
	if ((unsigned int)rfnum >= Files.Size())
	{
		return nullptr;
	}

	return Files[rfnum]->FileName;
}


//==========================================================================
//
// IsEncryptedFile
//
// Returns true if the first 256 bytes of the lump are encrypted for Blood.
//
//==========================================================================

bool FileSystem::IsEncryptedFile(int lump) const noexcept
{
	if ((unsigned)lump >= (unsigned)NumEntries)
	{
		return false;
	}
	return !!(FileInfo[lump].lump->Flags & LUMPF_BLOODCRYPT);
}


// FileData -----------------------------------------------------------------

FileData::FileData ()
{
}

FileData::FileData (const FileData &copy)
{
	Block = copy.Block;
}

FileData &FileData::operator = (const FileData &copy)
{
	Block = copy.Block;
	return *this;
}

FileData::FileData (const FString &source)
: Block (source)
{
}

FileData::~FileData ()
{
}

FString::FString (ELumpNum lumpnum)
{
	auto lumpr = Files.OpenFileReader ((int)lumpnum);
	auto size = lumpr.GetLength ();
	AllocBuffer (1 + size);
	auto numread = lumpr.Read (&Chars[0], size);
	Chars[size] = '\0';

	if (numread != size)
	{
		FStringf err("ConstructStringFromLump: Only read %ld of %ld bytes on lump %i (%s)\n",
			numread, size, lumpnum, Files.GetFileName((int)lumpnum));
	}
}

//==========================================================================
//
// PrintLastError
//
//==========================================================================

#ifdef _WIN32
//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>

#if 0
extern "C" {
__declspec(dllimport) unsigned long __stdcall FormatMessageA(
    unsigned long dwFlags,
    const void *lpSource,
    unsigned long dwMessageId,
    unsigned long dwLanguageId,
    char **lpBuffer,
    unsigned long nSize,
    va_list *Arguments
    );
__declspec(dllimport) void * __stdcall LocalFree (void *);
__declspec(dllimport) unsigned long __stdcall GetLastError ();
}
#endif

static void PrintLastError ()
{
	char *lpMsgBuf;
	FormatMessageA(0x1300 /*FORMAT_MESSAGE_ALLOCATE_BUFFER | 
							FORMAT_MESSAGE_FROM_SYSTEM | 
							FORMAT_MESSAGE_IGNORE_INSERTS*/,
		NULL,
		GetLastError(),
		1 << 10 /*MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)*/, // Default language
		(LPSTR)&lpMsgBuf,
		0,
		NULL 
	);
	Printf (TEXTCOLOR_RED "  %s\n", lpMsgBuf);
	// Free the buffer.
	LocalFree( lpMsgBuf );
}
#else
static void PrintLastError ()
{
	Printf (TEXTCOLOR_RED "  %s\n", strerror(errno));
}
#endif

