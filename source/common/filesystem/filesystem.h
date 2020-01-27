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
#include "name.h"
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

	int InitMultipleFiles (TArray<FString> &filenames, const TArray<FString> &todelete, int maingamefiles);
	void DeleteStuff(const TArray<FString>& deletelumps, int numgamefiles);
	void Rehash();

	void AddFile (const char *filename, FileReader *wadinfo = NULL, bool nosubdirflag = false);
	void AddAdditionalFile(const char* filename, FileReader* wadinfo = NULL) {}
	int CheckIfResourceFileLoaded (const char *name) noexcept;

	const char *GetResourceFileName (int filenum) const noexcept;
	const char *GetResourceFileFullName (int filenum) const noexcept;

	int GetFirstEntry(int filenum) const noexcept;
	int GetLastEntry(int filenum) const noexcept;
    int GetEntryCount(int filenum) const noexcept;

	int FindFile (const char *name, ELookupMode lookupmode = ELookupMode::FullName, int filenum = -1) const noexcept;
	int FindFileWithExtensions(const char* name, const FName* exts, int count);
	int GetFile (const char *name, ELookupMode lookupmode = ELookupMode::FullName, int filenum = -1) const;	// Like FindFile, but throws an exception when it cannot find what it looks for.
	bool FileExists(const char* name)
	{
		return FindFile(name) >= 0;
	}

	int FindFile (const FString &name, ELookupMode lookupmode = ELookupMode::FullName, int filenum = -1) const noexcept { return FindFile(name.GetChars(), lookupmode, filenum); }
	int GetFile (const FString &name, ELookupMode lookupmode = ELookupMode::FullName, int filenum = -1) const { return GetFile(name.GetChars(), lookupmode, filenum); }
	bool FileExists(const FString & name)
	{
		return FindFile(name) >= 0;
	}

	int FindFile (const std::string &name, ELookupMode lookupmode = ELookupMode::FullName, int filenum = -1) const noexcept { return FindFile(name.c_str(), lookupmode, filenum); }
	int GetFile (const std::string &name, ELookupMode lookupmode = ELookupMode::FullName, int filenum = -1) const { return GetFile(name.c_str(), lookupmode, filenum); }
	bool FileExists(const std::string& name)
	{
		return FindFile(name) >= 0;
	}


	int FindResource (int resid, const char *type, int filenum = -1) const noexcept;
	int GetResource (int resid, const char *type, int filenum = -1) const;	// Like FindFile, but throws an exception when it cannot find what it looks for.

	int AddFromBuffer(const char* name, const char* type, char* data, int size, int id, int flags);


	TArray<FString> GetAllFilesOfType(FName type, bool withsubdirs = false);
	TArray<uint8_t> GetFileData(int file, int pad = 0);	// reads file into a writable buffer and optionally adds some padding at the end. (FileData isn't writable!)
	FileData ReadFile (int file);
	FileData ReadFile (const char *name) { return ReadFile (GetFile (name)); }

	inline TArray<uint8_t> LoadFile(const char* name, int padding)
	{
		auto lump = FindFile(name);
		if (lump < 0) return TArray<uint8_t>();
		return GetFileData(lump, padding);
	}


	
	const void *Lock(int lump);
	void Unlock(int lump, bool mayfree = false);
	const void *Get(int lump);
	
	// These are designed to be stand-ins for Blood's resource class.
	static const void *Lock(FResourceLump *lump);
	static void Unlock(FResourceLump *lump);
	static const void *Load(FResourceLump *lump);
	FResourceLump *Lookup(const char *name, const char *type);
	FResourceLump *Lookup(unsigned int id, const char *type);
	bool CreatePathlessCopy(const char *name, int id, int flags);

	FileReader OpenFileReader(int file);		// opens a reader that redirects to the containing file's one.
	FileReader ReopenFileReader(int file, bool alwayscache = false);		// opens an independent reader.
	FileReader OpenFileReader(const char* name, int where);

	int Iterate (const char *name, int *lastfile, ELookupMode lookupmode = ELookupMode::FullName);		// [RH] Find files with duplication

	int FileLength (int file) const;
	int GetFileOffset (int file);					// [RH] Returns offset of file in the wadfile
	int GetFileFlags (int file);					// Return the flags for this file
	const char *GetFileName (int file) const;
	FString GetFileFullPath (int file) const;		// [RH] Returns wad's name + file's full name
	int GetFileContainer (int file) const;			// [RH] Returns filenum for a specified file
	int GetRFFIndexNum (int file) const;			// Returns the RFF index number for this file
	unsigned GetFilesInFolder(const char *path, TArray<FolderEntry> &result, bool atomic) const;

	bool IsEncryptedFile(int file) const noexcept;
	int GetResourceId(int file) const;
	FName GetResourceType(int file) const;

	int GetNumResourceFiles() const { return NumFiles; }
	int GetNumEntries () const { return NumEntries; }
	FResourceLump* GetFileAt(int lump) const
	{
		return FileInfo[lump].lump;
	}
	void PrintDirectory();

protected:

	struct FileRecord
	{
		int			rfnum;
		FResourceLump* lump;
	};

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

extern FileSystem fileSystem;

