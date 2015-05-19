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

typedef void *saveable_code;

typedef struct
{
    void *base;
    unsigned int size;
} saveable_data;

typedef struct
{
    saveable_code *code;
    unsigned int numcode;

    saveable_data *data;
    unsigned int numdata;
} saveable_module;

#define SAVE_CODE(s) (void*)s
#define SAVE_DATA(s) { (void*)&s, sizeof(s) }

#define NUM_SAVEABLE_ITEMS(x) (sizeof(x)/sizeof(x[0]))

typedef struct
{
    unsigned int module;
    unsigned int index;
} savedcodesym;

typedef struct
{
    unsigned int module;
    unsigned int index;
    unsigned int offset;
} saveddatasym;

void Saveable_Init(void);

int Saveable_FindCodeSym(void *ptr, savedcodesym *sym);
int Saveable_FindDataSym(void *ptr, saveddatasym *sym);

int Saveable_RestoreCodeSym(savedcodesym *sym, void **ptr);
int Saveable_RestoreDataSym(saveddatasym *sym, void **ptr);

#endif
