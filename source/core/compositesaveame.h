#pragma once

#include <assert.h>
#include "files.h"
#include "zstring.h"
#include "tarray.h"
#include "filesystem/resourcefile.h"

class CompositeSavegameWriter
{
	FString filename;
	TDeletingArray<BufferWriter*> subfiles;
	TArray<FCompressedBuffer> subbuffers;
	TArray<FString> subfilenames;
	TArray<bool> isCompressed;

	FCompressedBuffer CompressElement(BufferWriter* element, bool compress);
public:
	void Clear()
	{
		for (auto& b : subbuffers) b.Clean();
		isCompressed.Clear();
		subfilenames.Clear();
		subfiles.DeleteAndClear();
		subbuffers.Clear();
		filename = "";
	}
	void SetFileName(const char* fn)
	{
		filename = fn;
	}
	void SetFileName(const FString& fn)
	{
		filename = fn;
	}
	~CompositeSavegameWriter()
	{
		assert(subfiles.Size() == 0);	// must be written out.
	}
	FileWriter& NewElement(const char* filename, bool compress = true);
	void AddCompressedElement(const char* filename, FCompressedBuffer& buffer);
	bool WriteToFile();
};

