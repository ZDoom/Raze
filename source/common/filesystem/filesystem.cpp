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
#include "name.h"
//#include "c_dispatch.h"
#include "filesystem.h"
#include "resourcefile.h"
#include "v_text.h"
#include "c_dispatch.h"
//#include "md5.h"
//#include "doomstat.h"

// MACROS ------------------------------------------------------------------

#define NULL_INDEX		(0xffffffff)

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void PrintLastError ();

// PUBLIC DATA DEFINITIONS -------------------------------------------------

FileSystem fileSystem;

// CODE --------------------------------------------------------------------


FileSystem::~FileSystem ()
{
	DeleteAll();
}

void FileSystem::DeleteAll ()
{
	NumEntries = 0;

	// explicitly delete all manually added lumps.
	for (auto &frec : FileInfo)
	{
		if (frec.rfnum == -1) delete frec.lump;
	}
	FileInfo.Clear();
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

int FileSystem::InitMultipleFiles(TArray<FString>& filenames, const TArray<FString>& deletelumps, int maingamefiles)
{
	int numfiles;

	// open all the files, load headers, and count lumps
	DeleteAll();
	numfiles = 0;

	for (unsigned i = 0; i < filenames.Size(); i++)
	{
		int baselump = NumEntries;
		bool nosubdirflag = false;
		const char* fn = filenames[i];
		if (*fn == '*')
		{
			fn++;
			nosubdirflag = true;
		}
		AddFile(filenames[i], nullptr, nosubdirflag);
	}

	NumEntries = FileInfo.Size();
	if (NumEntries == 0)
	{
		return 0;
	}
	DeleteStuff(deletelumps, maingamefiles);
	Rehash();
	return NumEntries;
}

void FileSystem::Rehash()
{
	// [RH] Set up hash table
	Hashes.Resize(NumLookupModes * 2 * NumEntries);
	for (int i = 0; i < NumLookupModes; i++)
	{
		FirstFileIndex[i] = &Hashes[i * 2 * NumEntries];
		NextFileIndex[i] = &Hashes[(i * 2 + 1) * NumEntries];
	}
	InitHashChains ();
	FileInfo.ShrinkToFit();
	Files.ShrinkToFit();
}

//==========================================================================
//
// Deletes unwanted content from the main game files
//
//==========================================================================

void FileSystem::DeleteStuff(const TArray<FString>& deletelumps, int numgamefiles)
{
	// This must account for the game directory being inserted at index 2.
	// Deletion may only occur in the main game file, the directory and the add-on, there are no secondary dependencies, i.e. more than two game files.
	numgamefiles++;
	for (auto str : deletelumps)
	{
		FString renameTo;
		auto ndx = str.IndexOf("*");
		if (ndx >= 0)
		{
			renameTo = FName(str.Mid(ndx + 1));
			str.Truncate(ndx);
		}
		FName check = FName(str);

		for (uint32_t i = 0; i < FileInfo.Size(); i++)
		{
			if (FileInfo[i].rfnum >= 1 && FileInfo[i].rfnum <= numgamefiles && check == FileInfo[i].lump->LumpName[FResourceLump::FullNameType])
			{
				if (renameTo.IsEmpty())
				{
					for (auto& n : FileInfo[i].lump->LumpName) n = NAME_None;
				}
				else FileInfo[i].lump->LumpNameSetup(renameTo);
			}
		}
	}
}

//==========================================================================
//
// AddFile
//
//==========================================================================

void FileSystem::AddFile (const char *filename, FileReader *filer, bool nosubdirflag)
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
		resfile = FResourceFile::OpenDirectory(filename, false, nosubdirflag);

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
// CheckIfResourceFileLoaded
//
// Returns true if the specified file is loaded, false otherwise.
// If a fully-qualified path is specified, then the file must match exactly.
// Otherwise, any file with that name will work, whatever its path.
// Returns the file's index if found, or -1 if not.
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
			auto pth = ExtractFileBase(GetResourceFileName(i), true);
			if (stricmp (pth.GetChars(), name) == 0)
			{
				return i;
			}
		}
	}
	return -1;
}

//==========================================================================
//
// FindFile
//
// Looks up a file by name, either eith or without path and extension
//
//==========================================================================

int FileSystem::FindFile (const char *name, ELookupMode lookupmode, int filenum) const noexcept
{
	uint32_t i;

	if (name == NULL)
	{
		return -1;
	}
	if (*name == '/') name++;	// maps get a '/' prepended to their name. No idea what's the point, but this must be removed here.
	FName lname(name, true);
	if (lname == NAME_None) return -1;

	if (lookupmode == ELookupMode::IdWithType) return -1;
	int lookupindex = (int)lookupmode;
	uint32_t* fli = FirstFileIndex[lookupindex];
	uint32_t* nli = NextFileIndex[lookupindex];
	
	for (i = fli[int(lname) % NumEntries]; i != NULL_INDEX; i = nli[i])
	{
		if (filenum > 0 && FileInfo[i].rfnum != filenum) continue;
		auto lump = FileInfo[i].lump;
		if (lump->LumpName[lookupindex] == lname) return i;
	}
	return -1;
}

//==========================================================================
//
// GetFile
//
// Calls FindFile, but bombs out if not found.
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
// FindFile
//
// Looks up a file by name, either eith or without path and extension
//
//==========================================================================

int FileSystem::FindFileWithExtensions(const char* name, const FName *exts, int count)
{
	uint32_t i;

	if (name == NULL)
	{
		return -1;
	}
	if (*name == '/') name++;	// maps get a '/' prepended to their name. No idea what's the point, but this must be removed here.
	FName lname(name, true);
	if (lname == NAME_None) return -1;

	const int lookupindex = FResourceLump::FullNameNoExtType;
	uint32_t* fli = FirstFileIndex[lookupindex];
	uint32_t* nli = NextFileIndex[lookupindex];

	for (i = fli[int(lname) % NumEntries]; i != NULL_INDEX; i = nli[i])
	{
		auto lump = FileInfo[i].lump;
		if (lump->LumpName[lookupindex] == lname)
		{
			for (int c = 0; c < count; c++)
			{
				if (lump->LumpName[FResourceLump::ExtensionType] == exts[c]) return i;
			}
		}
	}
	return -1;
}

//==========================================================================
//
// FindResource
//
// Looks for content based on Blood resource IDs.
//
//==========================================================================

int FileSystem::FindResource (int resid, const char *type, int filenum) const noexcept
{
	uint32_t i;

	if (type == NULL)
	{
		return -1;
	}
	FName lname(type, true);
	if (lname == NAME_None) return -1;

	const int lookuptype = (int)ELookupMode::IdWithType;
	uint32_t* fli = FirstFileIndex[lookuptype];
	uint32_t* nli = NextFileIndex[lookuptype];
	
	for (i = fli[int(resid) % NumEntries]; i != NULL_INDEX; i = nli[i])
	{
		if (filenum > 0 && FileInfo[i].rfnum != filenum) continue;
		if (FileInfo[i].lump->ResourceId != resid) continue;
		auto lump = FileInfo[i].lump;
		if (lump->LumpName[lookuptype] == lname) return i;
	}
	return -1;
}

//==========================================================================
//
// GetResource
//
// Calls GetResource, but bombs out if not found.
//
//==========================================================================

int FileSystem::GetResource (int resid, const char *type, int filenum) const
{
	int	i;

	i = FindResource (resid, type, filenum);

	if (i == -1)
	{
		FStringf error("GetResource: %d of type %s not found!", resid, type);
		throw FileSystemError(error.GetChars());
	}
	return i;
}

//==========================================================================
//
// LumpLength
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
// InitHashChains
//
// Prepares the lumpinfos for hashing.
// (Hey! This looks suspiciously like something from Boom! :-)
//
//==========================================================================

void FileSystem::InitHashChains (void)
{
	// Mark all buckets as empty
	memset(Hashes.Data(), 255, Hashes.Size() * sizeof(Hashes[0]));

	// Now set up the chains
	for (int i = 0; i < (unsigned)NumEntries; i++)
	{
		auto lump = FileInfo[i].lump;
		for (int l = 0; l < NumLookupModes; l++)
		{
			int hash;
			if (l != (int)ELookupMode::IdWithType && lump->LumpName[l] != NAME_None)
			{
				hash = int(lump->LumpName[l]) % NumEntries;
			}
			else if (l == (int)ELookupMode::IdWithType && lump->ResourceId >= 0)
			{
				hash = int(lump->ResourceId) % NumEntries;
			}
			else continue;
			NextFileIndex[l][i] = FirstFileIndex[l][hash];
			FirstFileIndex[l][hash] = i;
		}
	}
}

void FileSystem::AddLump(FResourceLump *lump)
{
	FileRecord rec = { -1, lump};
	FileInfo.Push(rec);
	NumEntries++;
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
	FileRecord *lump_p;

	int lookupindex = static_cast<int>(lookupmode);
	FName lname(name, true);
	assert(lastlump != NULL && *lastlump >= 0);
	if (lname == NAME_None)
	{
		*lastlump = NumEntries;
		return -1;
	}

	lump_p = &FileInfo[*lastlump];
	while (lump_p <= &FileInfo.Last())
	{
		auto lump = lump_p->lump;
		if (lump->LumpName[lookupindex] == lname)
		{
			int lump = int(lump_p - &FileInfo[0]);
			*lastlump = lump + 1;
			return lump;
		}
		lump_p++;
	}
	*lastlump = NumEntries;
	return -1;
}

//==========================================================================
//
// GetLumpName
//
//==========================================================================

const char *FileSystem::GetFileName (int lump) const
{
	if ((size_t)lump >= NumEntries)
		return nullptr;
	else
		return FileInfo[lump].lump->FullName();
}

//==========================================================================
//
// FileSystem :: GetFilrFullPath
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
// with files. Other archive types will return -1, since they don't have it.
//
//==========================================================================

int FileSystem::GetResourceId(int lump) const
{
	if ((size_t)lump >= NumEntries)
		return -1;
	else
		return FileInfo[lump].lump->ResourceId;
}

FName FileSystem::GetResourceType(int lump) const
{
	if ((size_t)lump >= NumEntries)
		return NAME_None;
	else
		return FileInfo[lump].lump->LumpName[FResourceLump::ExtensionType];
}


//==========================================================================
//
// GetLumpFile
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
		if (!strncmp(FileInfo[i].lump->FullName(), path, path.Len()))
		{
			// Only if it hasn't been replaced.
			if ((unsigned)FindFile(FileInfo[i].lump->FullName()) == i)
			{
				result.Push({ FileInfo[i].lump->FullName(), i });
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
// ReadFile
//
// Loads the lump into a TArray and returns it.
//
//==========================================================================

TArray<uint8_t> FileSystem::GetFileData(int lump, int pad)
{
	if ((size_t)lump >= FileInfo.Size())
		return TArray<uint8_t>();

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
// Interface to the lump cache
//
//==========================================================================

const void *FileSystem::Lock(int lump)
{
	if ((size_t)lump >= FileInfo.Size()) return nullptr;
	auto lumpp = FileInfo[lump].lump;
	return lumpp->Lock();
}

void FileSystem::Unlock(int lump, bool mayfree)
{
	if ((size_t)lump >= FileInfo.Size()) return;
	auto lumpp = FileInfo[lump].lump;
	lumpp->Unlock(mayfree);
}

const void *FileSystem::Get(int lump)
{
	if ((size_t)lump >= FileInfo.Size()) return nullptr;
	auto lumpp = FileInfo[lump].lump;
	return lumpp->Get();
}

//==========================================================================
//
// Stand-ins for Blood's resource class
//
//==========================================================================

const void *FileSystem::Lock(FResourceLump *lump)
{
	if (lump) return lump->Lock();
	else return nullptr;
}

void FileSystem::Unlock(FResourceLump *lump)
{
	if (lump) return lump->Unlock();
}

const void *FileSystem::Load(FResourceLump *lump)
{
	if (lump) return lump->Get();
	else return nullptr;
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

FileReader FileSystem::OpenFileReader(const char* name, int where)
{
	auto lump = FindFile(name);
	if (lump < 0) return FileReader();
	else return OpenFileReader(lump);
}

//==========================================================================
//
// GetAllFilesOfType
//
//==========================================================================

TArray<FString> FileSystem::GetAllFilesOfType(FName type, bool withsubdirs)
{
	TArray<FString> found;
	for (unsigned i = 0; i < FileInfo.Size(); i++)
	{
		auto& fi = FileInfo[i];
		if (fi.lump->ResType() == type)
		{
			if (!withsubdirs && fi.lump->LumpName[FResourceLump::BaseNameNoExtType] != fi.lump->LumpName[FResourceLump::FullNameNoExtType]) continue;
			auto check = FindFile(fi.lump->FullName());
			if (check == i) found.Push(fi.lump->FullName());
		}
	}
	return found;
}

//==========================================================================
//
// GetResourceFileName
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
// GetResourceFileFullName
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
// AddFromBuffer
//
// Adds an in-memory resource to the virtual directory
//
//==========================================================================

int FileSystem::AddFromBuffer(const char* name, const char* type, char* data, int size, int id, int flags)
{
	FStringf fullname("%s.%s", name, type);
	auto newlump = new FMemoryLump(data, size);
	newlump->LumpNameSetup(fullname);
	newlump->ResourceId = id;
	AddLump(newlump);
	return Files.Size()-1;
}

//==========================================================================
//
// Blood style lookup functions
//
//==========================================================================

FResourceLump *FileSystem::Lookup(const char *name, const char *type)
{
	FStringf fname("%s.%s", name, type);
	auto lump = FindFile(fname);
	if (lump >= 0) return FileInfo[lump].lump;
	else return nullptr;
}

FResourceLump *FileSystem::Lookup(unsigned int id, const char *type)
{
	auto lump = FindResource(id, type);
	if (lump >= 0) return FileInfo[lump].lump;
	else return nullptr;
}

//==========================================================================
//
// Clones an existing resource with different properties
//
//==========================================================================

bool FileSystem::CreatePathlessCopy(const char *name, int id, int flags)
{
	FString name2, type2, path;

	// The old code said 'filename' and ignored the path, this looked like a bug.
	auto lump = FindFile(name);
	if (lump < 0) return false;		// Does not exist.

	auto oldlump = FileInfo[lump].lump;
	FName filename = oldlump->LumpName[FResourceLump::BaseNameType];
	FName fullname = oldlump->LumpName[FResourceLump::FullNameType];

	// If the lump we are about to add already got the right properties, do nothing, aside from loading/locking as requested
	if (filename == fullname && (id == -1 || id == oldlump->ResourceId))
	{
		if (flags & DICT_LOCK) oldlump->Lock();
		else if (flags & DICT_LOAD) oldlump->Get();
		return true;
	}
	
	// Create a clone of the resource to give it new lookup properties.
	auto newlump = new FClonedLump(FileInfo[lump].lump);
	newlump->LumpNameSetup(filename.GetChars());
	newlump->ResourceId = id;
	if (flags & DICT_LOCK) newlump->Lock();
	else if (flags & DICT_LOAD) newlump->Get();
	AddLump(newlump);
	return true;
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
	auto lumpr = fileSystem.OpenFileReader ((int)lumpnum);
	auto size = lumpr.GetLength ();
	AllocBuffer (1 + size);
	auto numread = lumpr.Read (&Chars[0], size);
	Chars[size] = '\0';

	if (numread != size)
	{
		FStringf err("ConstructStringFromLump: Only read %ld of %ld bytes on lump %i (%s)\n",
			numread, size, lumpnum, fileSystem.GetFileName((int)lumpnum));
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

static void PrintLastError ()
{
	char *lpMsgBuf;
	FormatMessageA(0x1300 /*FORMAT_MESSAGE_ALLOCATE_BUFFER | 
							FORMAT_MESSAGE_FROM_SYSTEM | 
							FORMAT_MESSAGE_IGNORE_INSERTS*/,
		NULL,
		GetLastError(),
		1 << 10 /*MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)*/, // Default language
		(char **)&lpMsgBuf,
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

CCMD(printfs)
{
	fileSystem.PrintDirectory();
}

void FileSystem::PrintDirectory()
{
	for (int i = 0; i < NumEntries; i++)
	{
		auto lump = FileInfo[i].lump;
		auto f = GetFileContainer(i);
		auto n = GetResourceFileFullName(f);
		Printf("%5d: %9d    %64s %4s %3d    %s\n", i, lump->LumpSize, lump->LumpName[0].GetChars(), lump->LumpName[4].GetChars(), lump->ResourceId, n);
	}
}
