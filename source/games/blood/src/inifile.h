/*
 * Copyright (C) 2018, 2022 nukeykt
 *
 * This file is part of Blood-RE.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _INIFILE_H_
#define _INIFILE_H_


struct IniNode {
    IniNode *next;
    char data[1];
};

class IniFile
{
public:
    IniFile(const char *);
    void Load();
    bool FindSection(const char *);
    bool FindKey(const char*);
    void AddSection(const char *);
    void AddKeyString(const char *, const char *);
    void ChangeKeyString(const char *, const char *);
    bool SectionExists(const char *);
    bool KeyExists(const char *, const char *);
    const char *GetKeyString(const char *, const char *, const char *);
    int GetKeyInt(const char *, const char *, int);
    bool GetKeyBool(const char *, const char *, int);
    int GetKeyHex(const char *, const char *, int);
    ~IniFile(void);

    IniNode head;
    IniNode *curNode = nullptr;
    IniNode *f_9 = nullptr;

    char *f_d;

    std::string filename;

};


#endif
