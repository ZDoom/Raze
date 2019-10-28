

#ifndef __RESFILE_H
#define __RESFILE_H

#include <stdint.h>
#include "files.h"
#include "zstring.h"

class FResourceFile;
class FTexture;

enum ELumpFlags
{
	LUMPF_ZIPFILE=1,		// contains a full path
	LUMPF_BLOODCRYPT = 2,	// encrypted
	LUMPF_COMPRESSED = 4,	// compressed
	LUMPF_SEQUENTIAL = 8,	// compressed but a sequential reader can be retrieved.
}; 

class FResourceFile;

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

	int				LumpSize = 0;
	int				RefCount = 0;
	int				Flags = 0;
	FString			FullName;	// Name with extension and path
	FString			BaseName;	// Name without extension and path
	FResourceFile *	Owner = nullptr;
	TArray<uint8_t> Cache;

	FResourceLump() = default;

	virtual ~FResourceLump();
	virtual FileReader *GetReader();
	virtual FileReader NewReader();
	virtual int GetFileOffset() { return -1; }
	virtual int GetIndexNum() const { return 0; }
	void LumpNameSetup(FString iname);
	virtual FCompressedBuffer GetRawData();

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
	static FResourceFile *OpenDirectory(const char *filename, bool quiet = false);
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

	virtual FileReader *GetReader();
	virtual int FillCache();
	virtual int GetFileOffset() { return Position; }

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
	virtual int FillCache();

};



#endif
