

#ifndef __RESFILE_H
#define __RESFILE_H

#include <stdint.h>
#include "files.h"
#include "zstring.h"

class FResourceFile;
class FTexture;

enum ELumpFlags
{
	LUMPF_MAYBEFLAT=1,		// might be a flat outside F_START/END
	LUMPF_ZIPFILE=2,		// contains a full path
	LUMPF_EMBEDDED=4,		// from an embedded WAD
	LUMPF_BLOODCRYPT = 8,	// encrypted
	LUMPF_COMPRESSED = 16,	// compressed
	LUMPF_SEQUENTIAL = 32,	// compressed but a sequential reader can be retrieved.
}; 

// This holds a compresed Zip entry with all needed info to decompress it.
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
	friend class FResourceFile;

	int				LumpSize;
	FString			FullName;		// only valid for files loaded from a non-wad archive
	uint8_t			Flags;
	int8_t			RefCount;
	char *			Cache;
	FResourceFile *	Owner;
	FTexture *		LinkedTexture;
	int				Namespace;

	FResourceLump()
	{
		Cache = NULL;
		Owner = NULL;
		Flags = 0;
		RefCount = 0;
		Namespace = 0;	// ns_global
		LinkedTexture = NULL;
	}

	virtual ~FResourceLump();
	virtual FileReader NewReader();
	virtual int GetFileOffset() { return -1; }
	virtual int GetIndexNum() const { return 0; }
	void LumpNameSetup(FString iname);

	void *CacheLump();
	int ReleaseCache();

protected:
	virtual int FillCache() { return -1; }

};

class FResourceFile
{
public:
	FileReader Reader;
	FString FileName;
protected:
	uint32_t NumLumps;
	FString Hash;

	FResourceFile(const char *filename);
	FResourceFile(const char *filename, FileReader &r);

private:
	uint32_t FirstLump;

public:
	static FResourceFile *OpenResourceFile(const char *filename, FileReader &file, bool quiet = false, bool containeronly = false);
	static FResourceFile *OpenResourceFile(const char *filename, bool quiet = false, bool containeronly = false);
	virtual ~FResourceFile();
    // If this FResourceFile represents a directory, the Reader object is not usable so don't return it.
    FileReader *GetReader() { return Reader.isOpen()? &Reader : nullptr; }
	uint32_t LumpCount() const { return NumLumps; }
	uint32_t GetFirstLump() const { return FirstLump; }
	void SetFirstLump(uint32_t f) { FirstLump = f; }
	const FString &GetHash() const { return Hash; }


	virtual bool Open(bool quiet) = 0;
	virtual FResourceLump *GetLump(int no) = 0;
	FResourceLump *FindLump(const char *name);
};

struct FUncompressedLump : public FResourceLump
{
	int				Position;

	virtual FileReader *GetReader();
	virtual int FillCache();
	virtual int GetFileOffset() { return Position; }

};


// Base class for uncompressed resource files (WAD, GRP, PAK and single lumps)
class FUncompressedFile : public FResourceFile
{
protected:
	TArray<FUncompressedLump> Lumps;

	FUncompressedFile(const char *filename);
	FUncompressedFile(const char *filename, FileReader &r);
	virtual FResourceLump *GetLump(int no) { return ((unsigned)no < NumLumps)? &Lumps[no] : NULL; }
};

#endif
