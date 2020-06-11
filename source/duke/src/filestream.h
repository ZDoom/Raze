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

#ifndef _RedNukemFileStream_h_
#define _RedNukemFileStream_h_

#include "vfs.h"
#include "compat.h"
#include <stdint.h>

namespace RedNukem {

class FileStream
{
    public:
        bool Open(const char *fileName);
        bool Is_Open();
        void Close();

        int32_t ReadBytes(uint8_t *data, uint32_t nBytes);

        uint64_t ReadUint64LE();
        uint64_t ReadUint64BE();

        uint32_t ReadUint32LE();
        uint32_t ReadUint32BE();

        uint16_t ReadUint16LE();
        uint16_t ReadUint16BE();

        uint8_t ReadByte();

        enum SeekDirection {
            kSeekCurrent = 0,
            kSeekStart   = 1,
            kSeekEnd     = 2
        };

        int32_t Seek(int32_t offset, SeekDirection = kSeekStart);
        int32_t Skip(int32_t offset);

        int32_t GetPosition();

    private:
        buildvfs_kfd file;
};

} // close namespace RedNukem

#endif
