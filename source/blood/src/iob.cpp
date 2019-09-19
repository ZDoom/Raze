//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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
#include <stdlib.h>
#include <string.h>
#include "compat.h"
#include "common_game.h"
#include "iob.h"

IOBuffer::IOBuffer(int _nRemain, char *_pBuffer)
{
    nRemain = _nRemain;
    pBuffer =_pBuffer;
}

void IOBuffer::Read(void *pData, int nSize)
{
    if (nSize <= nRemain)
    {
        memcpy(pData, pBuffer, nSize);
        nRemain -= nSize;
        pBuffer += nSize;
    }
    else
    {
        ThrowError("Read buffer overflow");
    }
}

void IOBuffer::Write(void *pData, int nSize)
{
    if (nSize <= nRemain)
    {
        memcpy(pBuffer, pData, nSize);
        nRemain -= nSize;
        pBuffer += nSize;
    }
    else
    {
        ThrowError("Write buffer overflow");
    }
}

void IOBuffer::Skip(int nSize)
{
    if (nSize <= nRemain)
    {
        nRemain -= nSize;
        pBuffer += nSize;
    }
    else
    {
        ThrowError("Skip overflow");
    }
}
