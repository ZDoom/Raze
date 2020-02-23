
/*
** compositesavegame.cpp
** Container for savegame files with multiple sub-content
**
**---------------------------------------------------------------------------
** Copyright 2019 Christoph Oelckers
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
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OFf
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include <zlib.h>
#include "compositesaveame.h"
#include "file_zip.h"
#include "resourcefile.h"
#include "m_png.h"
#include "gamecontrol.h"


bool WriteZip(const char *filename, TArray<FString> &filenames, TArray<FCompressedBuffer> &content);


FileWriter &CompositeSavegameWriter::NewElement(const char *filename, bool compress)
{
	FCompressedBuffer b{};
	subfilenames.Push(filename);
	subbuffers.Push(b);
	isCompressed.Push(compress);
	auto bwr = new BufferWriter;
	subfiles.Push(bwr);
	return *bwr;
}

void CompositeSavegameWriter::AddCompressedElement(const char* filename, FCompressedBuffer& buffer)
{
	subfilenames.Push(filename);
	subbuffers.Push(buffer);
	buffer = {};
	subfiles.Push(nullptr);
	isCompressed.Push(true);
}

FCompressedBuffer CompositeSavegameWriter::CompressElement(BufferWriter *bw, bool compress)
{
	FCompressedBuffer buff;

	auto buffer =bw->GetBuffer();
	buff.mSize = buffer->Size();
	buff.mZipFlags = 0;
	buff.mCRC32 = crc32(0, (const Bytef*)buffer->Data(), buffer->Size());
	
	uint8_t *compressbuf = new uint8_t[buff.mSize+1];
	
	z_stream stream;
	int err;
	
	stream.next_in = (Bytef *)buffer->Data();
	stream.avail_in = buff.mSize;
	stream.next_out = (Bytef*)compressbuf;
	stream.avail_out = buff.mSize;
	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	stream.opaque = (voidpf)0;
	
	if (!compress) goto error;
	
	// create output in zip-compatible form as required by FCompressedBuffer
	err = deflateInit2(&stream, 8, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY);
	if (err != Z_OK)
	{
		goto error;
	}
	
	err = deflate(&stream, Z_FINISH);
	if (err != Z_STREAM_END)
	{
		deflateEnd(&stream);
		goto error;
	}
	buff.mCompressedSize = stream.total_out;
	
	err = deflateEnd(&stream);
	if (err == Z_OK)
	{
		buff.mBuffer = new char[buff.mCompressedSize];
		buff.mMethod = METHOD_DEFLATE;
		memcpy(buff.mBuffer, compressbuf, buff.mCompressedSize);
		delete[] compressbuf;
		return buff;
	}
	
error:
	if (buff.mSize) memcpy(compressbuf, buffer->Data(), buff.mSize + 1);
	buff.mBuffer = (char*)compressbuf;
	buff.mCompressedSize = buff.mSize;
	buff.mMethod = METHOD_STORED;
	return buff;
	
}

bool CompositeSavegameWriter::WriteToFile()
{
	if (subfiles.Size() == 0) return false;
	TArray<FCompressedBuffer> compressed(subfiles.Size(), 1);
	for (unsigned i = 0; i < subfiles.Size(); i++)
	{
		if (subfiles[i])
			compressed[i] = CompressElement(subfiles[i], isCompressed[i]);
		else
		{
			compressed[i] = subbuffers[i];
			subbuffers[i] = {};
		}
	}
	
	if (WriteZip(filename, subfilenames, compressed))
	{
		// Check whether the file is ok by trying to open it.
		//FResourceFile *test = FResourceFile::OpenResourceFile(filename, true);
		//if (test != nullptr)
		{
			Clear();
			//delete test;
			return true;
		}
	}

	Clear();
	return false;
}

