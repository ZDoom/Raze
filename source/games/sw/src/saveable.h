//-------------------------------------------------------------------------
/*
 Copyright (C) 2005 Jonathon Fowler <jf@jonof.id.au>

 This file is part of JFShadowWarrior

 Shadow Warrior is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
//-------------------------------------------------------------------------

#ifndef SAVEABLE_H
#define SAVEABLE_H


struct saveable_code
{
    void* base;
    const char* name;
};

struct saveable_data
{
    void *base;
    const char* name;
    unsigned int size;
};

struct saveable_module
{
    saveable_code *code;
    unsigned int numcode;

    saveable_data *data;
    unsigned int numdata;
};

template <typename T>
constexpr std::enable_if_t<!std::is_pointer<T>::value, size_t> SAVE_SIZEOF(T const & obj) noexcept
{
    return sizeof(obj);
}

#define SAVE_CODE(s) { (void*)(s), #s }
#define SAVE_DATA(s) { (void*)&(s), #s, (int)SAVE_SIZEOF(s) }

#define NUM_SAVEABLE_ITEMS(x) countof(x)

struct savedcodesym
{
    FString name;
};

struct saveddatasym
{
    FString name;
    unsigned int offset;
};

void Saveable_Init(void);

int Saveable_FindCodeSym(void *ptr, savedcodesym *sym);
int Saveable_FindDataSym(void *ptr, saveddatasym *sym);

int Saveable_RestoreCodeSym(savedcodesym *sym, void **ptr);
int Saveable_RestoreDataSym(saveddatasym *sym, void **ptr);


#endif
