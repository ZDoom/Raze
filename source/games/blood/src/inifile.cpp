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

#include "ns.h"	// Must come before everything else!

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "blood.h"

#include "inifile.h"

BEGIN_BLD_NS

static void *ResReadLine(char *buffer, unsigned int nBytes, void **pRes)
{
    unsigned int i;
    char ch;
    if (!pRes || !*pRes || *((char*)*pRes) == 0)
        return NULL;
    for (i = 0; i < nBytes; i++)
    {
        ch = *((char*)*pRes);
        if(ch == 0 || ch == '\n')
            break;
        buffer[i] = ch;
        *pRes = ((char*)*pRes)+1;
    }
    if (*((char*)*pRes) == '\n' && i < nBytes)
    {
        ch = *((char*)*pRes);
        buffer[i] = ch;
        *pRes = ((char*)*pRes)+1;
        i++;
    }
    else
    {
        while (true)
        {
            ch = *((char*)*pRes);
            if (ch == 0 || ch == '\n')
                break;
            *pRes = ((char*)*pRes)+1;
        }
        if (*((char*)*pRes) == '\n')
            *pRes = ((char*)*pRes)+1;
    }
    if (i < nBytes)
        buffer[i] = 0;
    return *pRes;
}



IniFile::IniFile(const char *fileName)
{
    head.next = &head;

    // debug stuff
    // curNode = NULL;
    // anotherNode = NULL;
    //_13 = NULL;

    strcpy(this->fileName, fileName);
    Load();
}

IniFile::IniFile(void *res)
{
    head.next = &head;
    strcpy(fileName, "menus.mat");
    LoadRes(res);
}

void IniFile::LoadRes(void *res)
{
    char buffer[256];

    curNode = &head;

    while (ResReadLine(buffer, sizeof(buffer), &res) != 0)
    {
        char *ch = strchr(buffer, '\n');
        if (ch != NULL) {
            ch[0] = '\0';
        }

        // do the same for carriage return?
        ch = strchr(buffer, '\r');
        if (ch != NULL) {
            ch[0] = '\0';
        }

        char *pBuffer = buffer;

        // remove whitespace from buffer
        while (isspace(*pBuffer)) {
            pBuffer++;
        }

        curNode->next = (FNODE*)malloc(strlen(pBuffer) + sizeof(FNODE));
        assert(curNode->next != NULL);

        anotherNode = curNode;
        curNode = curNode->next;


        strcpy(curNode->name, pBuffer);

        /*
            check for:
            ; - comment line. continue and grab a new line  (59)
            [ - start of section marker                     (91)
            ] - end of section marker                       (93)
            = - key and value seperator                     (61)
        */

        switch (*pBuffer)
        {
        case 0:
        case ';': // comment line
            break;
        case '[':
            if (!strchr(pBuffer, ']'))
            {
                free(curNode);
                curNode = anotherNode;
            }
            break;
        default:

            if (strchr(pBuffer, '=') <= pBuffer) {
                free(curNode);
                curNode = anotherNode;
            }
            break;
        }
    }

    curNode->next = &head;
}

void IniFile::Load()
{
    // char buffer[256];

    curNode = &head;

    auto fp = fileSystem.OpenFileReader(fileName);
    if (fp.isOpen())
    {
		auto pBuffer = fp.ReadPadded(1);
        LoadRes(pBuffer.Data());
    }
    else
        curNode->next = &head;
}

void IniFile::Save(void)
{
    char buffer[256];
    FILE *hFile = fopen(fileName, "w");
    assert(hFile != NULL);
    curNode = head.next;
    while (curNode != &head)
    {
        sprintf(buffer, "%s\n", curNode->name);
        fwrite(buffer, 1, strlen(buffer), hFile);
        curNode = curNode->next;
    }
    fclose(hFile);
}

bool IniFile::FindSection(const char *section)
{
    char buffer[256];
    curNode = anotherNode = &head;
    if (section)
    {
        sprintf(buffer, "[%s]", section);
        do
        {
            anotherNode = curNode;
            curNode = curNode->next;
            if (curNode == &head)
                return false;
        } while(stricmp(curNode->name, buffer) != 0);
    }
    return true;
}

bool IniFile::SectionExists(const char *section)
{
    return FindSection(section);
}

bool IniFile::FindKey(const char *key)
{
    anotherNode = curNode;
    curNode = curNode->next;
    while (curNode != &head)
    {
        char c = curNode->name[0];

        if (c == ';' || c == '\0') {
            anotherNode = curNode;
            curNode = curNode->next;
            continue;
        }

        if (c == '[') {
            return 0;
        }

        char *pEqual = strchr(curNode->name, '=');
        char *pEqualStart = pEqual;
        assert(pEqual != NULL);

        // remove whitespace
        while (isspace(*(pEqual - 1))) {
            pEqual--;
        }

        c = *pEqual;
        *pEqual = '\0';

        if (strcmp(key, curNode->name) == 0)
        {
            // strings match
            *pEqual = c;
            _13 = ++pEqualStart;
            while (isspace(*_13)) {
                _13++;
            }

            return true;
        }
        *pEqual = c;
        anotherNode = curNode;
        curNode = curNode->next;
    }

    return false;
}

void IniFile::AddSection(const char *section)
{
    char buffer[256];

    if (anotherNode != &head)
    {
        FNODE *newNode = (FNODE*)malloc(sizeof(FNODE));
        assert(newNode != NULL);

        newNode->name[0] = 0;
        newNode->next = anotherNode->next;
        anotherNode->next = newNode;
        anotherNode = newNode;
    }

    sprintf(buffer, "[%s]", section);
    FNODE *newNode = (FNODE*)malloc(strlen(buffer) + sizeof(FNODE));
    assert(newNode != NULL);

    strcpy(newNode->name, buffer);

    newNode->next = anotherNode->next;
    anotherNode->next = newNode;
    anotherNode = newNode;
}

void IniFile::AddKeyString(const char *key, const char *value)
{
    char buffer[256];

    sprintf(buffer, "%s=%s", key, value);

    FNODE *newNode = (FNODE*)malloc(strlen(buffer) + sizeof(FNODE));
    assert(newNode != NULL);

    strcpy(newNode->name, buffer);

    newNode->next = anotherNode->next;
    anotherNode->next = newNode;
    curNode = newNode;
}

void IniFile::ChangeKeyString(const char *key, const char *value)
{
    char buffer[256];

    sprintf(buffer, "%s=%s", key, value);

    FNODE *newNode = (FNODE*)realloc(curNode, strlen(buffer) + sizeof(FNODE));
    assert(newNode != NULL);

    strcpy(newNode->name, buffer);

    anotherNode->next = newNode;
}

bool IniFile::KeyExists(const char *section, const char *key)
{
    if (FindSection(section) && FindKey(key))
        return true;

    return false;
}

void IniFile::PutKeyString(const char *section, const char *key, const char *value)
{
    if (FindSection(section))
    {
        if (FindKey(key))
        {
            ChangeKeyString(key, value);
            return;
        }
    }
    else
    {
        AddSection(section);
    }

    AddKeyString(key, value);
}

const char* IniFile::GetKeyString(const char *section, const char *key, const char *defaultValue)
{
    if (FindSection(section) && FindKey(key))
        return _13;
    return defaultValue;
}

void IniFile::PutKeyInt(const char *section, const char *key, int value)
{
    char buffer[256];

    // convert int to string
    sprintf(buffer,"%d",value);

    PutKeyString(section, key, buffer);
}

int IniFile::GetKeyInt(const char *section, const char *key, int defaultValue)
{
    if (FindSection(section) && FindKey(key))
    {
        // convert string to int int
        return strtol(_13, NULL, 0);
    }
    return defaultValue;
}

void IniFile::PutKeyHex(const char *section, const char *key, int value)
{
    char buffer[256] = "0x";

    // convert int to string
    sprintf(buffer,"%x",value);

    PutKeyString(section, key, buffer);
}

int IniFile::GetKeyHex(const char *section, const char *key, int defaultValue)
{
    return GetKeyInt(section, key, defaultValue);
}

bool IniFile::GetKeyBool(const char *section, const char *key, int defaultValue)
{
    return (bool)GetKeyInt(section, key, defaultValue);
}

void IniFile::RemoveKey(const char *section, const char *key)
{
    if (FindSection(section) && FindKey(key))
    {
        anotherNode->next = curNode->next;
        free(curNode);
        curNode = anotherNode->next;
    }
}

void IniFile::RemoveSection(const char *section)
{
    if (FindSection(section))
    {
        anotherNode = curNode;

        curNode = curNode->next;

        while (curNode != &head)
        {
            if (curNode->name[0] == '[') {
                return;
            }

            anotherNode->next = curNode->next;
            free(curNode);
            curNode = anotherNode->next;
        }
    }
}

IniFile::~IniFile()
{
    curNode = head.next;

    while (curNode != &head)
    {
        anotherNode = curNode;
        curNode = curNode->next;
        free(anotherNode);
    }
}


END_BLD_NS
