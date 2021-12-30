//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT

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
// Note: This module is based on the sirlemonhead's work
#pragma once

BEGIN_BLD_NS

struct FNODE
{
    FNODE *next;
    char name[1];
};

// 161 bytes
class IniFile
{
public:
    IniFile(const char *fileName);
    IniFile(void *res);
    ~IniFile();

    void Save(void);
    bool FindSection(const char *section);
    bool SectionExists(const char *section);
    bool FindKey(const char *key);
    void AddSection(const char *section);
    void AddKeyString(const char *key, const char *value);
    void ChangeKeyString(const char *key, const char *value);
    bool KeyExists(const char *section, const char *key);
    void PutKeyString(const char *section, const char *key, const char *value);
    const char* GetKeyString(const char *section, const char *key, const char *defaultValue);
    void PutKeyInt(const char *section, const char *key, const int value);
    int GetKeyInt(const char *section, const char *key, int defaultValue);
    void PutKeyHex(const char *section, const char *key, int value);
    int GetKeyHex(const char *section, const char *key, int defaultValue);
    bool GetKeyBool(const char *section, const char *key, int defaultValue);
    void RemoveKey(const char *section, const char *key);
    void RemoveSection(const char *section);

private:
    FNODE head;
    FNODE *curNode;
    FNODE *anotherNode;

    char *_13;

    char fileName[BMAX_PATH]; // watcom maxpath

    void LoadRes(void *);
    void Load();
};

END_BLD_NS
