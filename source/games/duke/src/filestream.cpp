//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2020 EDuke32 developers and contributors
Copyright (C) 2020 sirlemonhead
This file is part of Rednukem.
Rednukem is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "filestream.h"
#include <stdlib.h>

namespace RedNukem {

bool FileStream::Open(const char *fileName)
{
    file = kopen4loadfrommod(fileName, 0);
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

    if (nCount != nBytes) {
        return 0;
    }

    return (int32_t)nCount;
}

uint64_t FileStream::ReadUint64LE()
{
    uint64_t value;
    kread(file, &value, 8);
    return B_LITTLE64(value);
}

uint64_t FileStream::ReadUint64BE()
{
    uint64_t value;
    kread(file, &value, 8);
    return B_BIG64(value);
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

int32_t FileStream::Seek(int32_t offset, SeekDirection direction)
{
    int32_t nStatus = -1;
    if (kSeekStart == direction)
    {
        nStatus = klseek(file, offset, SEEK_SET);
    }
    else if (kSeekCurrent == direction)
    {
        nStatus = klseek(file, offset, SEEK_CUR);
    }
    else if (kSeekEnd == direction)
    {
        nStatus = klseek(file, offset, SEEK_END);
    }

    return nStatus;
}

int32_t FileStream::Skip(int32_t offset)
{
    return Seek(offset, kSeekCurrent);
}

int32_t FileStream::GetPosition()
{
    return ktell(file);
}

} // close namespace RedNukem
