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
#include <stdio.h>
#include <string.h>
#include "common_game.h"

#include "misc.h"

void *ResReadLine(char *buffer, unsigned int nBytes, void **pRes)
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

bool FileRead(FILE *handle, void *buffer, unsigned int size)
{
    return fread(buffer, 1, size, handle) == size;
}

bool FileWrite(FILE *handle, void *buffer, unsigned int size)
{
    return fwrite(buffer, 1, size, handle) == size;
}

bool FileLoad(const char *name, void *buffer, unsigned int size)
{
    dassert(buffer != NULL);

    FILE *handle = fopen(name, "rb");
    if (!handle)
        return false;

    unsigned int nread = fread(buffer, 1, size, handle);
    fclose(handle);
    return nread == size;
}

int FileLength(FILE *handle)
{
    if (!handle)
        return 0;
    int nPos = ftell(handle);
    fseek(handle, 0, SEEK_END);
    int nLength = ftell(handle);
    fseek(handle, nPos, SEEK_SET);
    return nLength;
}

unsigned int randSeed = 1;

unsigned int qrand(void)
{
    if (randSeed&0x80000000)
        randSeed = ((randSeed<<1)^0x20000004)|0x1;
    else
        randSeed = randSeed<<1;
    return randSeed&0x7fff;
}

int wRandSeed = 1;

int wrand(void)
{
    wRandSeed = (wRandSeed*1103515245)+12345;
    return (wRandSeed>>16)&0x7fff;
}

void wsrand(int seed)
{
    wRandSeed = seed;
}

void ChangeExtension(char *pzFile, const char *pzExt)
{
#if 0
    char drive[BMAX_PATH];
    char dir[BMAX_PATH];
    char filename[BMAX_PATH];
    _splitpath(pzFile, drive, dir, filename, NULL);
    _makepath(pzFile, drive, dir, filename, pzExt);
#else
    int const nLength = Bstrlen(pzFile);
    char * pDot = pzFile+nLength;
    for (int i = nLength-1; i >= 0; i--)
    {
        if (pzFile[i] == '/' || pzFile[i] == '\\')
            break;
        if (pzFile[i] == '.')
        {
            pDot = pzFile+i;
            break;
        }
    }
    *pDot = '\0';
    Bstrcat(pDot, pzExt);
#endif
}

void SplitPath(const char *pzPath, char *pzDirectory, char *pzFile, char *pzType)
{
    int const nLength = Bstrlen(pzFile);
    const char *pDirectory = pzFile+nLength;
    const char *pDot = NULL;
    for (int i = nLength-1; i >= 0; i--)
    {
        if (pzFile[i] == '/' || pzFile[i] == '\\')
        {
            Bstrncpy(pzDirectory, pzPath, i);
            pzDirectory[i] = 0;
            if (!pDot)
            {
                Bstrcpy(pzFile, pzPath+i+1);
                Bstrcpy(pzType, "");
            }
            else
            {
                Bstrncpy(pzFile, pzPath+i+1, pDot-(pzPath+i+1));
                Bstrcpy(pzType, pDot+1);
            }
           
            return;
        }
        else if (pzFile[i] == '.')
        {
            pDot = pzFile+i;
        }
    }
    Bstrcpy(pzDirectory, "/");
    if (!pDot)
    {
        Bstrcpy(pzFile, pzPath);
        Bstrcpy(pzType, "");
    }
    else
    {
        Bstrncpy(pzFile, pzPath, pDot-pzPath);
        Bstrcpy(pzType, pDot+1);
    }
}

void ConcatPath(const char *pzPath1, const char *pzPath2, char *pzConcatPath)
{
    int n1 = Bstrlen(pzPath1), n2 = Bstrlen(pzPath2);
    int i = n1, j = 0;
    while (i > 0 && (pzPath1[i-1] == '/' || pzPath1[i-1] == '\\'))
    {
        i--;
    }
    while (j < n2 && (pzPath2[j] == '/' || pzPath1[j] == '\\'))
    {
        j++;
    }
    Bstrncpy(pzConcatPath, pzPath1, i);
    pzConcatPath[i] = 0;
    Bstrcpy(pzConcatPath, pzPath2+j);
}


