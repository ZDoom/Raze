

#ifndef __RESFILE_H
#define __RESFILE_H

#include <stdint.h>
#include "files.h"
#include "zstring.h"
#include "name.h"

class FResourceFile;
class FTexture;

typedef enum {
	ns_hidden = -1,

	ns_global = 0,
	ns_sprites,
	ns_flats,
	ns_colormaps,
	ns_acslibrary,
	ns_newtextures,
	ns_bloodraw,
	ns_bloodsfx,
	ns_bloodmisc,
	ns_strifevoices,
	ns_hires,
	ns_voxels,

	// These namespaces are only used to mark lumps in special subdirectories
	// so that their contents doesn't interfere with the global namespace.
	// searching for data in these namespaces works differently for lumps coming
	// from Zips or other files.
	ns_specialzipdirectory,
	ns_sounds,
	ns_patches,
	ns_graphics,
	ns_music,

	ns_firstskin,
} namespace_t;

enum ELumpFlags
{
	LUMPF_MAYBEFLAT=1,		// might be a flat outside F_START/END
	LUMPF_ZIPFILE=2,		// contains a full path
	LUMPF_EMBEDDED=4,		// from an embedded WAD
	LUMPF_BLOODCRYPT = 8,	// encrypted
	LUMPF_COMPRESSED = 16,	// compressed
	LUMPF_SEQUENTIAL = 32,	// compressed but a sequential reader can be retrieved.
};

class FResourceFile;

// This holds a compressed Zip entry with all needed info to decompress it.
struct FCompressedBuffer
{
	unsigned mSize;
	unsigned mCompressedSize;
	int mMethod;
	int mZipFlags;
	unsigned mCRC32;
	char *mBuffer;

	bool Decompress(char *destbuffer);
	void Clean()
	{
		mSize = mCompressedSize = 0;
		if (mBuffer != nullptr)
		{
			delete[] mBuffer;
			mBuffer = nullptr;
		}
	}
};


struct FResourceLump
{
	enum ENameType
	{
		FullNameType,
		FullNameNoExtType,
		BaseNameType,
		BaseNameNoExtType,
		ExtensionType,
		DoomLumpType,
		NUMNAMETYPES
	};
	
	friend class FResourceFile;
	friend struct FClonedLump;

	unsigned 		LumpSize = 0;
	int				RefCount = 0;
	int				Flags = 0;
	int				ResourceId = -1;
	int				Namespace = ns_global;
	FName			LumpName[NUMNAMETYPES] = {};
	FResourceFile *	Owner = nullptr;
	TArray<uint8_t> Cache;

	FResourceLump() = default;

	virtual ~FResourceLump();
	virtual FileReader *GetReader();
	virtual FileReader NewReader();
	virtual int GetFileOffset() { return -1; }
	void LumpNameSetup(FString iname);
	virtual FCompressedBuffer GetRawData();

	virtual void *Lock(); // validates the cache and increases the refcount.
	virtual void Unlock(bool freeunrefd = false); // recreases the refcount and optionally frees the buffer
	virtual void *Get(); // validates the cache and returns a pointer without locking
	
	// Wrappers for emulating Blood's resource system
	unsigned Size() const{ return LumpSize; }
	int LockCount() const { return RefCount; }
	const char *ResName() const { return LumpName[BaseNameNoExtType].GetChars(); }
	const FName ResType() { return LumpName[ExtensionType]; }
	const char *FullName() const { return LumpName[FullNameType].GetChars(); }

protected:
	virtual int ValidateCache() { return -1; }

};

// Map NBlood's resource system to our own.
using DICTNODE = FResourceLump;

class FResourceFile
{
public:
	FileReader Reader;
	FString FileName;
protected:
	uint32_t NumLumps;

	FResourceFile(const char *filename);
	FResourceFile(const char *filename, FileReader &r);

	// for archives that can contain directories
	void PostProcessArchive(void *lumps, size_t lumpsize);

private:
	uint32_t FirstLump;

	int FilterLumps(FString filtername, void *lumps, size_t lumpsize, uint32_t max);
	bool FindPrefixRange(FString filter, void *lumps, size_t lumpsize, uint32_t max, uint32_t &start, uint32_t &end);
	void JunkLeftoverFilters(void *lumps, size_t lumpsize, uint32_t max);
	static FResourceFile *DoOpenResourceFile(const char *filename, FileReader &file, bool quiet, bool containeronly);

public:
	static FResourceFile *OpenResourceFile(const char *filename, FileReader &file, bool quiet = false, bool containeronly = false);
	static FResourceFile *OpenResourceFile(const char *filename, bool quiet = false, bool containeronly = false);
	static FResourceFile* OpenDirectory(const char* filename, bool quiet = false, bool nosubdirs = false);
	virtual ~FResourceFile();
    // If this FResourceFile represents a directory, the Reader object is not usable so don't return it.
    FileReader *GetReader() { return Reader.isOpen()? &Reader : nullptr; }
	uint32_t LumpCount() const { return NumLumps; }
	uint32_t GetFirstLump() const { return FirstLump; }
	void SetFirstLump(uint32_t f) { FirstLump = f; }



	virtual bool Open(bool quiet) = 0;
	virtual FResourceLump *GetLump(int no) = 0;
	FResourceLump *FindLump(const char *name);
};

struct FUncompressedLump : public FResourceLump
{
	int				Position;

	FileReader *GetReader() override;
	int ValidateCache() override;
	virtual int GetFileOffset() override { return Position; }

};


// Base class for uncompressed resource files (GRP, PAK and single lumps)
class FUncompressedFile : public FResourceFile
{
protected:
	TArray<FUncompressedLump> Lumps;

	FUncompressedFile(const char *filename);
	FUncompressedFile(const char *filename, FileReader &r);
	virtual FResourceLump *GetLump(int no) { return ((unsigned)no < NumLumps)? &Lumps[no] : NULL; }
};


struct FExternalLump : public FResourceLump
{
	FString Filename;

	FExternalLump(const char *_filename, int filesize = -1);
	virtual int ValidateCache() override;

};

struct FMemoryLump : public FResourceLump
{
	FMemoryLump(const void* data, int length)
	{
		LumpSize = length;
		Cache.Resize(length);
		memcpy(Cache.Data(), data, length);
	}
	virtual int ValidateCache() override
	{
		RefCount = INT_MAX / 2; // Make sure it never counts down to 0 by resetting it to something high each time it is used.
		return 1;
	}
};

struct FClonedLump : public FResourceLump
{
	FResourceLump* parent;
	FClonedLump(FResourceLump* lump)
	{
		parent = lump;
	}
	void* Lock() override { return parent->Lock(); }
	void Unlock(bool mayfree) override { parent->Unlock(mayfree); }
	void* Get() override { return parent->Get(); }
	int ValidateCache() override { return parent->ValidateCache(); }
};



#endif
