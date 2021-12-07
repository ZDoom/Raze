#pragma once

/*
** freelistarray.cpp
**	fixed size array with free list management
**
**---------------------------------------------------------------------------
** Copyright 2020 Christoph Oelckers
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

#include "serializer.h"
#include "tarray.h"

template<class Type, int size> class FreeListArray
{
    int freecount;
    int16_t FreeList[size];
    Type DataList[size];
public:
    enum { count = size };
    void Clear()
    {
        memset(DataList, 0, sizeof(DataList));
        freecount = size;
        for (int i = 0; i < size; i++)
        {
            FreeList[i] = size - i - 1;
        }
    }

    int Get()
    {
        if (freecount == 0) return -1;
        return FreeList[--freecount];
    }

    void Release(int i)
    {
        // Important: This must not clear the released objects because several 
        // linked list loops are written to access the 'next' field after the Release call!
        FreeList[freecount++] = i;
    }

    Type& operator[](size_t index)
    {
        return DataList[index];
    }

    void Serialize(FSerializer& arc, const char* key)
    {
        if (arc.BeginObject(key))
        {
            FixedBitArray<size> check;

            if (arc.isWriting())
            {
                check.SetAll(true);
                for (int i = 0; i < freecount; i++) check.Clear(FreeList[i]);
                arc.SerializeMemory("used", check.Storage(), check.StorageSize());
                arc.SparseArray("data", DataList, size, check);
            }
            else
            {
                arc.SerializeMemory("used", check.Storage(), check.StorageSize());
                arc.SparseArray("data", DataList, size, check);
                freecount = 0;
                for (int i = 0; i < size; i++)
                {
                    if (!check[i]) FreeList[freecount++] = i;
                }
            }
            arc.EndObject();
        }
    }
};

template<class Type, int size> 
FSerializer& Serialize(FSerializer& arc, const char* key, FreeListArray<Type, size> &w, FreeListArray<Type, size> * def)
{
	w.Serialize(arc, key);
	return arc;
}

