#pragma once
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//	File system  I/O functions.
//
//-----------------------------------------------------------------------------


#include <stdexcept>
#include "files.h"
#include "tarray.h"
#include "zstring.h"

#ifdef FindResource
#undef FindResource
#endif

// We do not want to expose the resource file interface here.
class FResourceFile;
struct FResourceLump;

class FileSystemError : std::runtime_error
{
public:
	FileSystemError(const char* err) : std::runtime_error(err) {}
};

// A file in memory.
class FileData
{
public:
	FileData ();

	FileData (const FileData &copy);
	FileData &operator= (const FileData &copy);
	~FileData ();
	void *GetMem () { return Block.Len() == 0 ? NULL : (void *)Block.GetChars(); }
	size_t GetSize () { return Block.Len(); }
	FString GetString () { return Block; }

private:
	FileData (const FString &source);

	FString Block;

	friend class FileSystem;
};

struct FolderEntry
{
	const char *name;
	unsigned lumpnum;
};

enum DICTFLAGS {
    DICT_LOAD = 4,
    DICT_LOCK = 8,
};

enum class ELookupMode // Todo: Merge with FResourceLump::ENameType
{
	FullName,
	NoExtension,
	BaseName,
	BaseWithExtension,
	IdWithType,
	NumLookupModes
};

enum
{
	NumLookupModes = (int)ELookupMode::NumLookupModes + 1
};


class FileSystem
{
public:
	FileSystem () = default;
	~FileSystem ();

	int InitMultipleFiles (TArray<FString> &filenames, const TArray<FString> &todelete);
	void AddFile (const char *filename, FileReader *wadinfo = NULL);
	int CheckIfResourceFileLoaded (const char *name) noexcept;

	const char *GetResourceFileName (int filenum) const noexcept;
	const char *GetResourceFileFullName (int filenum) const noexcept;

	int GetFirstEntry(int filenum) const noexcept;
	int GetLastEntry(int filenum) const noexcept;
    int GetEntryCount(int filenum) const noexcept;

	int FindFile (const char *name, ELookupMode lookupmode = ELookupMode::FullName, int filenum = -1) const noexcept;
	int GetFile (const char *name, ELookupMode lookupmode = ELookupMode::FullName, int filenum = -1) const;	// Like FindFile, but throws an exception when it cannot find what it looks for.

	int FindFile (const FString &name, ELookupMode lookupmode = ELookupMode::FullName, int filenum = -1) const noexcept { return FindFile(name.GetChars(), lookupmode, filenum); }
	int GetFile (const FString &name, ELookupMode lookupmode = ELookupMode::FullName, int filenum = -1) const { return GetFile(name.GetChars(), lookupmode, filenum); }

	int FindFile (const std::string &name, ELookupMode lookupmode = ELookupMode::FullName, int filenum = -1) const noexcept { return FindFile(name.c_str(), lookupmode, filenum); }
	int GetFile (const std::string &name, ELookupMode lookupmode = ELookupMode::FullName, int filenum = -1) const { return GetFile(name.c_str(), lookupmode, filenum); }

	int FindResource (int resid, const char *type, int filenum = -1) const noexcept;
	int GetResource (int resid, const char *type, int filenum = -1) const;	// Like FindFile, but throws an exception when it cannot find what it looks for.

	void AddFromBuffer(const char* name, const char* type, char* data, int size, int id, int flags);


	TArray<uint8_t> GetFileData(int file, int pad = 0);	// reads file into a writable buffer and optionally adds some padding at the end. (FileData isn't writable!)
	FileData ReadFile (int file);
	FileData ReadFile (const char *name) { return ReadFile (GetFile (name)); }
	
	const void *Lock(int lump);
	void Unlock(int lump, bool mayfree = false);
	const void *Get(int lump);
	
	// These are designed to be stand-ins for Blood's resource class.
	static const void *Lock(FResourceLump *lump);
	static void Unlock(FResourceLump *lump);
	static const void *Load(FResourceLump *lump);
	static void Read(FResourceLump *lump) { Load(lump); }
	static void Read(FResourceLump *n, void *p);
	FResourceLump *Lookup(const char *name, const char *type);
	FResourceLump *Lookup(unsigned int id, const char *type);
	void AddExternalResource(const char *name, const char *type, int id, int flags, const char *pzDirectory);

	FileReader OpenFileReader(int file);		// opens a reader that redirects to the containing file's one.
	FileReader ReopenFileReader(int file, bool alwayscache = false);		// opens an independent reader.

	int Iterate (const char *name, int *lastfile, ELookupMode lookupmode = ELookupMode::FullName);		// [RH] Find files with duplication

	static uint32_t FileNameHash (const char *name);		// [RH] Create hash key from a given name

	int FileLength (int file) const;
	int GetFileOffset (int file);					// [RH] Returns offset of file in the wadfile
	int GetFileFlags (int file);					// Return the flags for this file
	void GetFileName (char *to, int file) const;	// [RH] Copies the file name to to using uppercopy
	const char *GetFileName (int file) const;
	FString GetFileFullPath (int file) const;		// [RH] Returns wad's name + file's full name
	int GetFileContainer (int file) const;			// [RH] Returns filenum for a specified file
	int GetRFFIndexNum (int file) const;			// Returns the RFF index number for this file
	unsigned GetFilesInFolder(const char *path, TArray<FolderEntry> &result, bool atomic) const;

	bool IsEncryptedFile(int file) const noexcept;
	int GetResourceId(int file) const;

	int GetNumResourceFiles() const { return NumFiles; }
	int GetNumEntries () const { return NumEntries; }

protected:

	struct FileRecord;

	TArray<FResourceFile *> Files;
	TArray<FileRecord> FileInfo;

	TArray<uint32_t> Hashes;			// one allocation for all hash lists.
	uint32_t *FirstFileIndex[NumLookupModes];	// Hash information for the base name (no path and no extension)
	uint32_t *NextFileIndex[NumLookupModes];

	uint32_t NumFiles = 0;					// Not necessarily the same as FileInfo.Size()
	uint32_t NumEntries;					// Hash modulus. Can be smaller than NumFiles if things get added at run time.

	void InitHashChains ();								// [RH] Set up the lumpinfo hashing
	void AddLump(FResourceLump* lump);

private:
	void DeleteAll();
};

extern FileSystem Files;

