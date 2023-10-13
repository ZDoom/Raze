//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) NoOne

*****************************************************************
NoOne: A very basic string parser. Update or replace eventually.
*****************************************************************

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
#ifdef NOONE_EXTENSIONS
#pragma once
const char* enumStrGetChar(int offset, char* out, const char* astr, char expcr);
int enumStr(int nOffs, const char* str, char* key, char* val);
int enumStr(int nOffs, const char* str, char* val);
char isfix(const char* str, char flags = 0x03);
void removeSpaces(char* str);
char isufix(const char* str);
char isarray(const char* str, int* nLen = NULL);
char isbool(const char* str);
char isperc(const char* str);
int btoi(const char* str);
char isempty(const char* str);
#endif
