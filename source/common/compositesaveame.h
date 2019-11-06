#pragma once

#include "files.h"
#include "zstring.h"
#include "tarray.h"

class CompositeSavegameWriter
{
	TDeletingArray<BufferWriter*> subfiles;
	TArray<FString> subfilenames;
	TArray<bool> isCompressed;
	
	FCompressedBuffer CompressElement(BufferWriter *element, bool compress);
public:
	
	FileWriter &NewEleemnt(const char *filename, bool compress = true);
	bool WriteToFile(const char *filename);
};

