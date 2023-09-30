/*
 * Copyright (C) 2018, 2022 nukeykt
 *
 * This file is part of Raze
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
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#include <string>
#include "inifile.h"
#include "misc.h"
#include "m_alloc.h"
#include "filesystem.h"


IniFile::IniFile(const char *file)
{
    head.next = &head;
    filename = file;
    Load();
}

void IniFile::Load(void)
{
    char line[256];
    curNode = &head;
    auto fr = fileSystem.OpenFileReader(filename.c_str());
    if (fr.isOpen())
    {
        while (fr.Gets(line, 256))
        {
            char* vs = strchr(line, '\n');
            if (vs)
                *vs = 0;
            vs = strchr(line, '\r');
            if (vs)
                *vs = 0;

            vs = line;
            while (isspace((uint8_t)*vs))
            {
                vs++;
            }
            curNode->next = (IniNode*)M_Malloc(sizeof(IniNode) + strlen(vs));
            f_9 = curNode;
            curNode = curNode->next;
            strcpy(curNode->data, vs);
            switch (*vs)
            {
                case 0:
                case ';':
                    break;
                case '[':
                    if (!strchr(vs, ']'))
                    {
                        free(curNode);
                        curNode = f_9;
                    }
                    break;
                default:
                    if (strchr(vs, '=') <= vs)
                    {
                        free(curNode);
                        curNode = f_9;
                    }
                    break;
            }
        }
    }
    curNode->next = &head;
}

bool IniFile::FindSection(const char* section)
{
    char buffer[256];
    curNode = f_9 = &head;
    if (section)
    {
        snprintf(buffer, 256, "[%s]", section);
        
        while (1)
        {
            f_9 = curNode;
            curNode = curNode->next;
            if (curNode == &head)
                return false;
            if (!stricmp(curNode->data, buffer))
                return true;
        }
    }
    return true;
}

bool IniFile::FindKey(const char* key)
{
    f_9 = curNode;
    curNode = curNode->next;
    while (curNode != &head)
    {
        if (curNode->data[0] == ';' || curNode->data[0] == '\0')
        {
            f_9 = curNode;
            curNode = curNode->next;
            continue;
        }
        if (curNode->data[0] == '[')
            break;

        char* pEqual = strchr(curNode->data, '=');
        if (pEqual == NULL)
        {
            I_Error("'=' expected");
        }
        char* vb = pEqual;
        while (isspace((uint8_t)*(vb-1)))
        {
            vb--;
        }
        char back = *vb;
        *vb = 0;
        if (!stricmp(key, curNode->data))
        {
            *vb = back;
            f_d = pEqual + 1;
            while (isspace((uint8_t)*f_d))
            {
                f_d++;
            }
            return true;
        }
        *vb = back;
        f_9 = curNode;
        curNode = curNode->next;
    }
    return false;
}

void IniFile::AddSection(const char* section)
{
    char buf[256];
    if (f_9 != &head)
    {
        IniNode *newNode = (IniNode*)M_Malloc(sizeof(IniNode));
        newNode->data[0] = 0;
        newNode->next = f_9->next;
        f_9->next = newNode;
        f_9 = newNode;
    }
    snprintf(buf, 256, "[%s]", section);
    IniNode *newNode = (IniNode*)M_Malloc(sizeof(IniNode) + strlen(buf));
    strcpy(newNode->data, buf);
    newNode->next = f_9->next;
    f_9->next = newNode;
    f_9 = newNode;
}

void IniFile::AddKeyString(const char* key, const char* val)
{
    char buf[256];
    snprintf(buf, 256, "%s=%s", key, val);
    IniNode* newNode = (IniNode*)M_Malloc(sizeof(IniNode) + strlen(buf));
    strcpy(newNode->data, buf);
    newNode->next = f_9->next;
    f_9->next = newNode;
    f_9 = newNode;
}

void IniFile::ChangeKeyString(const char* key, const char* val)
{
    char buf[256];
    snprintf(buf, 256, "%s=%s", key, val);
    IniNode *newNode = (IniNode*)M_Realloc(curNode, sizeof(IniNode) + strlen(buf));
    strcpy(newNode->data, buf);
    f_9->next = newNode;
}

bool IniFile::SectionExists(const char* section)
{
    return FindSection(section);
}

bool IniFile::KeyExists(const char* section, const char* key)
{
    if (FindSection(section) && FindKey(key))
        return true;
    return false;
}

const char* IniFile::GetKeyString(const char* section, const char* key, const char* val)
{
    if (FindSection(section) && FindKey(key))
        return f_d;
    return val;
}

int IniFile::GetKeyInt(const char* section, const char* key, int val)
{
    if (FindSection(section) && FindKey(key))
        return strtol(f_d, NULL, 0);
    return val;
}

bool IniFile::GetKeyBool(const char* section, const char* key, int val)
{
    return GetKeyInt(section, key, val);
}

int IniFile::GetKeyHex(const char* section, const char* key, int val)
{
    return GetKeyInt(section, key, val);
}


IniFile::~IniFile(void)
{
    curNode = head.next;
    while (curNode != &head)
    {
        auto node = curNode;
        curNode = curNode->next;
        free(node);
    }
}
