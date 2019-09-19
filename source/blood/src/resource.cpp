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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "crc32.h"
#include "compat.h"
#include "cache1d.h"
#ifdef WITHKPLIB
#include "kplib.h"
#endif
#include "common_game.h"

#include "misc.h"
#include "qheap.h"
#include "resource.h"

#if B_BIG_ENDIAN == 1
#include "qav.h"
#include "seq.h"
#include "sound.h"
#endif

CACHENODE Resource::purgeHead = { NULL, &purgeHead, &purgeHead, 0 };

#ifdef USE_QHEAP
QHeap *Resource::heap;
#endif
Resource::Resource(void)
{
    dict = NULL;
    indexName = NULL;
    indexId = NULL;
    buffSize = 0;
    count = 0;
    handle = -1;
    crypt = true;
}

Resource::~Resource(void)
{
    if (dict)
    {
        for (unsigned int i = 0; i < count; i++)
        {
            if (dict[i].type)
                Free(dict[i].type);
            if (dict[i].name)
                Free(dict[i].name);
            if (dict[i].path)
                Free(dict[i].path);
        }
        Free(dict);
        dict = NULL;
        buffSize = 0;
        count = 0;
    }
    if (handle != -1)
    {
        kclose(handle);
    }
}

void Resource::Init(const char *filename)
{
    RFFHeader header;
#ifdef USE_QHEAP
    dassert(heap != NULL);
#endif

    if (filename)
    {
        handle = kopen4loadfrommod(filename, 0);
        if (handle != -1)
        {
            int nFileLength = kfilelength(handle);
            dassert(nFileLength != -1);
            if (kread(handle, &header, sizeof(RFFHeader)) != sizeof(RFFHeader)
                || memcmp(header.sign, "RFF\x1a", 4))
            {
                ThrowError("RFF header corrupted");
            }
#if B_BIG_ENDIAN == 1
            header.version = B_LITTLE16(header.version);
            header.offset = B_LITTLE32(header.offset);
            header.filenum = B_LITTLE32(header.filenum);
#endif
            switch (header.version & 0xff00)
            {
            case 0x200:
                crypt = 0;
                break;
            case 0x300:
                crypt = 1;
                break;
            default:
                ThrowError("Unknown RFF version");
                break;
            }
            count = header.filenum;
            if (count)
            {
                buffSize = 1;
                while (count * 2 >= buffSize)
                {
                    buffSize *= 2;
                }
                dict = (DICTNODE*)Alloc(buffSize * sizeof(DICTNODE));
                memset(dict, 0, buffSize * sizeof(DICTNODE));
                DICTNODE_FILE *tdict = (DICTNODE_FILE*)Alloc(count*sizeof(DICTNODE_FILE));
                int r = klseek(handle, header.offset, SEEK_SET);
                dassert(r != -1);
                if ((uint32_t)kread(handle, tdict, count * sizeof(DICTNODE_FILE)) != count*sizeof(DICTNODE_FILE))
                {
                    ThrowError("RFF dictionary corrupted");
                }
                if (crypt)
                {
                    Crypt(tdict, count * sizeof(DICTNODE_FILE),
                        header.offset + (header.version & 0xff) * header.offset);
                }
                for (unsigned int i = 0; i < count; i++)
                {
                    dict[i].offset = B_LITTLE32(tdict[i].offset);
                    dict[i].size = B_LITTLE32(tdict[i].size);
                    dict[i].flags = tdict[i].flags;
                    int nTypeLength = strnlen(tdict[i].type, 3);
                    int nNameLength = strnlen(tdict[i].name, 8);
                    dict[i].type = (char*)Alloc(nTypeLength+1);
                    dict[i].name = (char*)Alloc(nNameLength+1);
                    strncpy(dict[i].type, tdict[i].type, min(3, nTypeLength));
                    strncpy(dict[i].name, tdict[i].name, min(8, nNameLength));
                    dict[i].path = NULL;
                    dict[i].type[nTypeLength] = 0;
                    dict[i].name[nNameLength] = 0;
                    dict[i].id = B_LITTLE32(tdict[i].id);
                }
                Free(tdict);
            }
        }
    }
    if (!dict)
    {
        buffSize = 16;
        dict = (DICTNODE*)Alloc(buffSize * sizeof(DICTNODE));
        memset(dict, 0, buffSize * sizeof(DICTNODE));
    }
    Reindex();
#if 0
    if (external)
    {
        char fname[BMAX_PATH];
        char type[BMAX_PATH];
        BDIR *dirr;
        struct Bdirent *dirent;
        dirr = Bopendir("./");
        if (dirr)
        {
            while (dirent = Breaddir(dirr))
            {
                if (!Bwildmatch(dirent->name, external))
                    continue;
                _splitpath(dirent->name, NULL, NULL, fname, type);
                if (type[0] == '.')
                {
                    AddExternalResource(fname, &type[1], dirent->size);
                }
                else
                {
                    AddExternalResource(fname, "", dirent->size);
                }
            }
            Bclosedir(dirr);
        }
#if 0
        _splitpath2(external, out, &dir, &node, NULL, NULL);
        _makepath(ext, dir, node, NULL, NULL);
        int status = _dos_findfirst(external, 0, &info);
        while (!status)
        {
            _splitpath2(info.name, out, NULL, NULL, &fname, &type);
            if (*type == '.')
            {
                AddExternalResource(*fname, (char*)(type + 1), info.size);
            }
            else
            {
                AddExternalResource(*fname, "", info.size);
            }
            status = _dos_findnext(&info);
        }
        _dos_findclose(&info);
#endif
    }
#endif
    for (unsigned int i = 0; i < count; i++)
    {
        if (dict[i].flags & DICT_LOCK)
        {
            Lock(&dict[i]);
        }
    }
    for (unsigned int i = 0; i < count; i++)
    {
        if (dict[i].flags & DICT_LOAD)
        {
            Load(&dict[i]);
        }
    }
}

void Resource::Flush(CACHENODE *h)
{
    if (h->ptr)
    {
#ifdef USE_QHEAP
        heap->Free(h->ptr);
#else
        delete[] (char*)h->ptr;
#endif
        
        h->ptr = NULL;
        if (h->lockCount == 0)
        {
            RemoveMRU(h);
            return;
        }
        h->lockCount = 0;
    }
}

void Resource::Purge(void)
{
    for (unsigned int i = 0; i < count; i++)
    {
        if (dict[i].ptr)
        {
            Flush((CACHENODE *)&dict[i]);
        }
    }
}

DICTNODE **Resource::Probe(const char *fname, const char *type)
{
    char name[BMAX_PATH];
    dassert(indexName != NULL);
    memset(name, 0, sizeof(name));
    strcpy(name, type);
    strcat(name, fname);
    dassert(dict != NULL);
    unsigned int hash = Bcrc32(name, strlen(name), 0) & (buffSize - 1);
    unsigned int i = hash;
    do
    {
        if (!indexName[i])
        {
            return &indexName[i];
        }
        if (!strcmp((*indexName[i]).type, type)
            && !strcmp((*indexName[i]).name, fname))
        {
            return &indexName[i];
        }
        if (++i == buffSize)
        {
            i = 0;
        }
    } while (i != hash);
    ThrowError("Linear probe failed to find match or unused node!");
    return NULL;
}

DICTNODE **Resource::Probe(unsigned int id, const char *type)
{
    struct {
        int id;
        char type[BMAX_PATH];
    } name;
    dassert(indexName != NULL);
    memset(&name, 0, sizeof(name));
    strcpy(name.type, type);
    name.id = id;
    dassert(dict != NULL);
    unsigned int hash = Bcrc32(&name, strlen(name.type)+sizeof(name.id), 0) & (buffSize - 1);
    unsigned int i = hash;
    do
    {
        if (!indexId[i])
        {
            return &indexId[i];
        }
        if (!strcmp((*indexId[i]).type, type)
            && (*indexId[i]).id == id)
        {
            return &indexId[i];
        }
        if (++i == buffSize)
        {
            i = 0;
        }
    } while (i != hash);
    ThrowError("Linear probe failed to find match or unused node!");
    return NULL;
}

void Resource::Reindex(void)
{
    if (indexName)
    {
        Free(indexName);
    }
    indexName = (DICTNODE **)Alloc(buffSize * sizeof(DICTNODE*));
    memset(indexName, 0, buffSize * sizeof(DICTNODE*));
    for (unsigned int i = 0; i < count; i++)
    {
        DICTNODE **node = Probe(dict[i].name, dict[i].type);
        *node = &dict[i];
    }

    if (indexId)
    {
        Free(indexId);
    }
    indexId = (DICTNODE **)Alloc(buffSize * sizeof(DICTNODE*));
    memset(indexId, 0, buffSize * sizeof(DICTNODE*));
    for (unsigned int i = 0; i < count; i++)
    {
        if (dict[i].flags & DICT_ID)
        {
            DICTNODE **node = Probe(dict[i].id, dict[i].type);
            *node = &dict[i];
        }
    }
}

void Resource::Grow(void)
{
    buffSize *= 2;
    void *p = Alloc(buffSize * sizeof(DICTNODE));
    memset(p, 0, buffSize * sizeof(DICTNODE));
    memcpy(p, dict, count * sizeof(DICTNODE));
    Free(dict);
    dict = (DICTNODE*)p;
    Reindex();
}

void Resource::AddExternalResource(const char *name, const char *type, int id, int flags, const char *pzDirectory)
{
    char name2[BMAX_PATH], type2[BMAX_PATH], filename[BMAX_PATH], path[BMAX_PATH];

    if (Bstrlen(type) > 0)
        Bsnprintf(filename, BMAX_PATH-1, "%s.%s", name, type);
    else
        Bsnprintf(filename, BMAX_PATH-1, "%s", name);

    if (pzDirectory)
        Bsnprintf(path, BMAX_PATH-1, "%s/%s", pzDirectory, filename);
    else
        Bstrncpy(path, filename, BMAX_PATH-1);

    int fhandle = kopen4loadfrommod(filename, 0);
    if (fhandle == -1)
        return;
    int size = kfilelength(fhandle);
    kclose(fhandle);
    strcpy(name2, name);
    strcpy(type2, type);
    Bstrupr(name2);
    Bstrupr(type2);
    dassert(dict != NULL);
    DICTNODE **index = Probe(name2, type2);
    dassert(index != NULL);
    DICTNODE *node = *index;
    if (node && (node->flags & DICT_EXTERNAL))
        return;
    if (!node)
    {
        if (2 * count >= buffSize)
        {
            Grow();
        }
        node = &dict[count++];
        index = Probe(name2, type2);
        *index = node;
        if (node->type)
        {
            Free(node->type);
            node->type = NULL;
        }
        if (node->name)
        {
            Free(node->name);
            node->name = NULL;
        }
        if (node->path)
        {
            Free(node->path);
            node->path = NULL;
        }
        int nTypeLength = strlen(type2);
        int nNameLength = strlen(name2);
        int nPathLength = strlen(path);
        node->type = (char*)Alloc(nTypeLength+1);
        node->name = (char*)Alloc(nNameLength+1);
        node->path = (char*)Alloc(nPathLength+1);
        strcpy(node->type, type2);
        strcpy(node->name, name2);
        strcpy(node->path, path);
    }
    node->size = size;
    node->flags |= DICT_EXTERNAL | flags;
    Flush(node);
    if (id >= 0)
    {
        index = Probe(id, type2);
        dassert(index != NULL);
        DICTNODE *node = *index;
        if (!node)
        {
            if (2 * count >= buffSize)
            {
                Grow();
            }
            node = &dict[count++];
            index = Probe(id, type2);
            *index = node;
        }
        if (node->type)
        {
            Free(node->type);
            node->type = NULL;
        }
        if (node->name)
        {
            Free(node->name);
            node->name = NULL;
        }
        if (node->path)
        {
            Free(node->path);
            node->path = NULL;
        }
        int nTypeLength = strlen(type2);
        int nNameLength = strlen(name2);
        int nPathLength = strlen(path);
        node->type = (char*)Alloc(nTypeLength+1);
        node->name = (char*)Alloc(nNameLength+1);
        node->path = (char*)Alloc(nPathLength+1);
        strcpy(node->type, type2);
        strcpy(node->name, name2);
        strcpy(node->path, path);
        node->id = id;
        node->size = size;
        node->flags |= DICT_EXTERNAL | flags;
        Flush(node);
    }
}

void *Resource::Alloc(int nSize)
{
#ifdef USE_QHEAP
    dassert(heap != NULL);
    dassert(nSize != 0);
    void *p = heap->Alloc(nSize);
    if (p)
    {
        return p;
    }
    for (CACHENODE *node = purgeHead.next; node != &purgeHead; node = node->next)
    {
        dassert(node->lockCount == 0);
        dassert(node->ptr != NULL);
        int nFree = heap->Free(node->ptr);
        node->ptr = NULL;
        RemoveMRU(node);
        if (nSize <= nFree)
        {
            p = Alloc(nSize);
            dassert(p != NULL);
            return p;
        }
    }
    ThrowError("Out of memory!");
    return NULL;
#else
    dassert(nSize != 0);
    void* p = new char[nSize];
    if (p)
    {
        return p;
    }
    for (CACHENODE *node = purgeHead.next; node != &purgeHead; node = node->next)
    {
        dassert(node->lockCount == 0);
        dassert(node->ptr != NULL);
        delete[] (char*)node->ptr;
        node->ptr = NULL;
        RemoveMRU(node);
        p = new char[nSize];
        if (p)
            return p;
    }
    ThrowError("Out of memory!");
    return NULL;
#endif
}

void Resource::Free(void *p)
{
#ifdef USE_QHEAP
    dassert(heap != NULL);
    dassert(p != NULL);
    heap->Free(p);
#else
    dassert(p != NULL);
    delete[] (char*)p;
#endif
}

DICTNODE *Resource::Lookup(const char *name, const char *type)
{
    char name2[BMAX_PATH], type2[BMAX_PATH];
    dassert(name != NULL);
    dassert(type != NULL);
    //if (strlen(name) > 8 || strlen(type) > 3) return NULL;
    // Try to load external resource first
    AddExternalResource(name, type);
    strcpy(name2, name);
    strcpy(type2, type);
    Bstrupr(type2);
    Bstrupr(name2);
    return *Probe(name2, type2);
}

DICTNODE *Resource::Lookup(unsigned int id, const char *type)
{
    char type2[BMAX_PATH];
    dassert(type != NULL);
    //if (strlen(type) > 3) return NULL;
    strcpy(type2, type);
    Bstrupr(type2);
    return *Probe(id, type2);
}

void Resource::Read(DICTNODE *n)
{
    dassert(n != NULL);
    Read(n, n->ptr);
}

void Resource::Read(DICTNODE *n, void *p)
{
    char filename[BMAX_PATH];
    dassert(n != NULL);
    if (n->flags & DICT_EXTERNAL)
    {
        if (n->path)
            Bstrncpy(filename, n->path, BMAX_PATH-1);
        else
            Bsnprintf(filename, MAX_PATH-1, "%s.%s", n->name, n->type);
        int fhandle = kopen4loadfrommod(filename, 0);
        if (fhandle == -1 || (uint32_t)kread(fhandle, p, n->size) != n->size)
        {
            ThrowError("Error reading external resource (%i)", errno);
        }
        kclose(fhandle);
    }
    else
    {
        int r = klseek(handle, n->offset, SEEK_SET);
        if (r == -1)
        {
            ThrowError("Error seeking to resource!");
        }
        if ((uint32_t)kread(handle, p, n->size) != n->size)
        {
            ThrowError("Error loading resource!");
        }
        if (n->flags & DICT_CRYPT)
        {
            int size;
            if (n->size > 0x100)
            {
                size = 0x100;
            }
            else
            {
                size = n->size;
            }
            Crypt(n->ptr, size, 0);
        }
#if B_BIG_ENDIAN == 1
        if (!Bstrcmp(n->type, "QAV"))
        {
            QAV *qav = (QAV*)p;
            qav->nFrames = B_LITTLE32(qav->nFrames);
            qav->ticksPerFrame = B_LITTLE32(qav->ticksPerFrame);
            qav->at10 = B_LITTLE32(qav->at10);
            qav->x = B_LITTLE32(qav->x);
            qav->y = B_LITTLE32(qav->y);
            qav->nSprite = B_LITTLE32(qav->nSprite);
            for (int i = 0; i < qav->nFrames; i++)
            {
                FRAMEINFO *pFrame = &qav->frames[i];
                SOUNDINFO *pSound = &pFrame->sound;
                pFrame->nCallbackId = B_LITTLE32(pFrame->nCallbackId);
                pSound->sound = B_LITTLE32(pSound->sound);
                for (int j = 0; j < 8; j++)
                {
                    TILE_FRAME *pTile = &pFrame->tiles[j];
                    pTile->picnum = B_LITTLE32(pTile->picnum);
                    pTile->x = B_LITTLE32(pTile->x);
                    pTile->y = B_LITTLE32(pTile->y);
                    pTile->z = B_LITTLE32(pTile->z);
                    pTile->stat = B_LITTLE32(pTile->stat);
                    pTile->angle = B_LITTLE16(pTile->angle);
                }
            }
        }
        else if (!Bstrcmp(n->type, "SEQ"))
        {
            Seq *pSeq = (Seq*)p;
            pSeq->version = B_LITTLE16(pSeq->version);
            pSeq->nFrames = B_LITTLE16(pSeq->nFrames);
            pSeq->at8 = B_LITTLE16(pSeq->at8);
            pSeq->ata = B_LITTLE16(pSeq->ata);
            pSeq->atc = B_LITTLE32(pSeq->atc);
            for (int i = 0; i < pSeq->nFrames; i++)
            {
                SEQFRAME *pFrame = &pSeq->frames[i];
                BitReader bitReader((char *)pFrame, sizeof(SEQFRAME));
                SEQFRAME swapFrame;
                swapFrame.tile = bitReader.readUnsigned(12);
                swapFrame.at1_4 = bitReader.readBit();
                swapFrame.at1_5 = bitReader.readBit();
                swapFrame.at1_6 = bitReader.readBit();
                swapFrame.at1_7 = bitReader.readBit();
                swapFrame.at2_0 = bitReader.readUnsigned(8);
                swapFrame.at3_0 = bitReader.readUnsigned(8);
                swapFrame.at4_0 = bitReader.readSigned(8);
                swapFrame.at5_0 = bitReader.readUnsigned(5);
                swapFrame.at5_5 = bitReader.readBit();
                swapFrame.at5_6 = bitReader.readBit();
                swapFrame.at5_7 = bitReader.readBit();
                swapFrame.at6_0 = bitReader.readBit();
                swapFrame.at6_1 = bitReader.readBit();
                swapFrame.at6_2 = bitReader.readBit();
                swapFrame.at6_3 = bitReader.readBit();
                swapFrame.at6_4 = bitReader.readBit();
                swapFrame.tile2 = bitReader.readUnsigned(4);
                swapFrame.pad = bitReader.readUnsigned(7);
                *pFrame = swapFrame;
            }
        }
        else if (!Bstrcmp(n->type, "SFX"))
        {
            SFX *pSFX = (SFX*)p;
            pSFX->relVol = B_LITTLE32(pSFX->relVol);
            pSFX->pitch = B_LITTLE32(pSFX->pitch);
            pSFX->pitchRange = B_LITTLE32(pSFX->pitchRange);
            pSFX->format = B_LITTLE32(pSFX->format);
            pSFX->loopStart = B_LITTLE32(pSFX->loopStart);
        }
#endif
    }
}

void *Resource::Load(DICTNODE *h)
{
    dassert(h != NULL);
    if (h->ptr)
    {
        if (!h->lockCount)
        {
            RemoveMRU(h);

            h->prev = purgeHead.prev;
            purgeHead.prev->next = h;
            h->next = &purgeHead;
            purgeHead.prev = h;
        }
    }
    else
    {
        h->ptr = Alloc(h->size);
        Read(h);

        h->prev = purgeHead.prev;
        purgeHead.prev->next = h;
        h->next = &purgeHead;
        purgeHead.prev = h;
    }
    return h->ptr;
}

void *Resource::Load(DICTNODE *h, void *p)
{
    dassert(h != NULL);
    if (p)
    {
        Read(h, p);
    }
    return p;
}

void *Resource::Lock(DICTNODE *h)
{
    dassert(h != NULL);
    if (h->ptr)
    {
        if (h->lockCount == 0)
        {
            RemoveMRU(h);
        }
    }
    else
    {
        h->ptr = Alloc(h->size);
        Read(h);
    }

    h->lockCount++;
    return h->ptr;
}

void Resource::Unlock(DICTNODE *h)
{
    dassert(h != NULL);
    dassert(h->ptr != NULL);
    if (h->lockCount > 0)
    {
        h->lockCount--;
        if (h->lockCount == 0)
        {
            h->prev = purgeHead.prev;
            purgeHead.prev->next = h;
            h->next = &purgeHead;
            purgeHead.prev = h;
        }
    }
}

void Resource::Crypt(void *p, int length, unsigned short key)
{
    char *cp = (char*)p;
    for (int i = 0; i < length; i++, key++)
    {
        cp[i] ^= (key >> 1);
    }
}

void Resource::RemoveMRU(CACHENODE *h)
{
    h->prev->next = h->next;
    h->next->prev = h->prev;
}

void Resource::FNAddFiles(fnlist_t * fnlist, const char *pattern)
{
    char filename[BMAX_PATH];
    for (unsigned int i = 0; i < count; i++)
    {
        DICTNODE *pNode = &dict[i];
        if (pNode->flags & DICT_EXTERNAL)
            continue;
        sprintf(filename, "%s.%s", pNode->name, pNode->type);
        if (!Bwildmatch(filename, pattern))
            continue;
        switch (klistaddentry(&fnlist->findfiles, filename, CACHE1D_FIND_FILE, CACHE1D_SOURCE_GRP))
        {
        case -1:
            return;
        case 0:
            fnlist->numfiles++;
            break;
        }
    }
}

void Resource::PurgeCache(void)
{
#ifndef USE_QHEAP
    for (CACHENODE *node = purgeHead.next; node != &purgeHead; node = node->next)
    {
        DICTNODE *pDict = (DICTNODE*)node;
        if (!(pDict->flags & DICT_LOAD))
        {
            dassert(pDict->lockCount == 0);
            dassert(pDict->ptr != NULL);
            Free(pDict->ptr);
            pDict->ptr = NULL;
            RemoveMRU(pDict);
        }
    }
#endif
}

void Resource::PrecacheSounds(void)
{
    for (unsigned int i = 0; i < count; i++)
    {
        DICTNODE *pNode = &dict[i];
        if ((!strcmp(pNode->type, "RAW") || !strcmp(pNode->type, "SFX")) && !pNode->ptr)
        {
            Load(pNode);
            G_HandleAsync();
        }
    }
}

void Resource::RemoveNode(DICTNODE* pNode)
{
    Flush(pNode);
    if (pNode->name)
    {
        Free(pNode->name);
        pNode->name = NULL;
    }
    if (pNode->type)
    {
        Free(pNode->type);
        pNode->type = NULL;
    }
    if (pNode->path)
    {
        Free(pNode->path);
        pNode->path = NULL;
    }
    *pNode = dict[--count];
    Bmemset(&dict[count], 0, sizeof(DICTNODE));
    if (pNode->ptr && !pNode->lockCount)
    {
        pNode->prev->next = pNode;
        pNode->next->prev = pNode;
    }
    Reindex();
}
