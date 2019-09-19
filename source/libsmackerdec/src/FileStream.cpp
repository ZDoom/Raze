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
#include <stdlib.h>

namespace SmackerCommon {

bool FileStream::Open(const std::string &fileName)
{
    file = kopen4loadfrommod(fileName.c_str(), 0);
	if (file == -1)
	{
		// log error
		return false;
	}

	return true;
}

bool FileStream::Is_Open()
{
	return file != -1;
}

void FileStream::Close()
{
    kclose(file);
    file = -1;
}

int32_t FileStream::ReadBytes(uint8_t *data, uint32_t nBytes)
{
	uint32_t nCount = (uint32_t)kread(file, data, static_cast<int32_t>(nBytes));

	if (nCount != nBytes)
	{
		return 0;
	}

	return (int32_t)nCount;
}

uint32_t FileStream::ReadUint32LE()
{
	uint32_t value;
	kread(file, &value, 4);
	return B_LITTLE32(value);
}

uint32_t FileStream::ReadUint32BE()
{
	uint32_t value;
    kread(file, &value, 4);
	return B_BIG32(value);
}

uint16_t FileStream::ReadUint16LE()
{
	uint16_t value;
    kread(file, &value, 2);
	return B_LITTLE16(value);
}

uint16_t FileStream::ReadUint16BE()
{
	uint16_t value;
    kread(file, &value, 2);
	return B_BIG16(value);
}

uint8_t FileStream::ReadByte()
{
	uint8_t value;
    kread(file, &value, 1);
	return value;
}

bool FileStream::Seek(int32_t offset, SeekDirection direction)
{
    int32_t nStatus = -1;
	if (kSeekStart == direction) {
        nStatus = klseek(file, offset, SEEK_SET);
	}
	else if (kSeekCurrent == direction) {
        nStatus = klseek(file, offset, SEEK_CUR);
	}

	// TODO - end seek
	if (nStatus < 0)
	{
		// todo
		return false;
	}

	return true;
}

bool FileStream::Skip(int32_t offset)
{
	return Seek(offset, kSeekCurrent);
}

bool FileStream::Is_Eos()
{
    // TODO:
    return false;
}

int32_t FileStream::GetPosition()
{
    return ktell(file);
}

} // close namespace SmackerCommon
