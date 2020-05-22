/*
 * libsmackerdec - Smacker video decoder
 * Copyright (C) 2011 Barry Duncan
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "FileStream.h"
#include "filesystem.h"
#include <stdlib.h>
#include "cmdlib.h"

namespace SmackerCommon {

bool FileStream::Open(const char *fileName)
{
	FString fixedname = fileName;
	FixPathSeperator(fixedname);
    file = fileSystem.OpenFileReader(fixedname);
	if (!file.isOpen())
	{
		// log error
		return false;
	}

	return true;
}

bool FileStream::Is_Open()
{
	return file.isOpen();
}

void FileStream::Close()
{
	file.Close();
}

int32_t FileStream::ReadBytes(uint8_t *data, uint32_t nBytes)
{
	uint32_t nCount = (uint32_t)file.Read(data, static_cast<int32_t>(nBytes));

	if (nCount != nBytes)
	{
		return 0;
	}

	return (int32_t)nCount;
}

uint32_t FileStream::ReadUint32LE()
{
	return file.ReadInt32();
}

uint32_t FileStream::ReadUint32BE()
{
	return file.ReadInt32BE();
}

uint16_t FileStream::ReadUint16LE()
{
	return file.ReadInt16();
}

uint16_t FileStream::ReadUint16BE()
{
	return file.ReadInt16BE();
}

uint8_t FileStream::ReadByte()
{
	return file.ReadInt8();
}

int32_t FileStream::Seek(int32_t offset, SeekDirection direction)
{
    int32_t nStatus = -1;
	if (kSeekStart == direction) {
        nStatus = file.Seek(offset, FileReader::SeekSet);
	}
	else if (kSeekCurrent == direction) {
		nStatus = file.Seek(offset, FileReader::SeekCur);
	}
    else if (kSeekEnd == direction) {
        nStatus = file.Seek(offset, FileReader::SeekEnd);
    }

    return nStatus;
}

int32_t FileStream::Skip(int32_t offset)
{
	return Seek(offset, kSeekCurrent);
}

int32_t FileStream::GetPosition()
{
    return file.Tell();
}

} // close namespace SmackerCommon
