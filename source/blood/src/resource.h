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
#pragma once

#include "common.h"
#include "qheap.h"

#pragma pack(push, 1)

enum DICTFLAGS {
    DICT_ID = 1,
    DICT_EXTERNAL = 2,
    DICT_LOAD = 4,
    DICT_LOCK = 8,
    DICT_CRYPT = 16,
};

struct RFFHeader
{
    char sign[4];
    short version;
    short pad1;
    unsigned int offset;
    unsigned int filenum;
    int pad2[4];
};

struct DICTNODE_FILE
{
    char unused1[16];
    unsigned int offset;
    unsigned int size;
    char unused2[8];
    char flags;
    char type[3];
    char name[8];
    int id;
};

#pragma pack(pop)

struct CACHENODE
{
    void *ptr;
    CACHENODE *prev;
    CACHENODE *next;
    int lockCount;
};

struct DICTNODE : CACHENODE
{
    unsigned int offset;
    unsigned int size;
    char flags;
    //char type[3];
    //char name[8];
    char *type;
    char *name;
    unsigned int id;
};

class Resource
{
public:
    Resource(void);
    ~Resource(void);

    void Init(const char *filename);
    static void Flush(CACHENODE *h);
    void Purge(void);
    DICTNODE **Probe(const char *fname, const char *type);
    DICTNODE **Probe(unsigned int id, const char *type);
    void Reindex(void);
    void Grow(void);
    void AddExternalResource(const char *name, const char *type, int id = -1);
    static void *Alloc(int nSize);
    static void Free(void *p);
    DICTNODE *Lookup(const char *name, const char *type);
    DICTNODE *Lookup(unsigned int id, const char *type);
    void Read(DICTNODE *n);
    void Read(DICTNODE *n, void *p);
    void *Load(DICTNODE *h);
    void *Load(DICTNODE *h, void *p);
    void *Lock(DICTNODE *h);
    void Unlock(DICTNODE *h);
    void Crypt(void *p, int length, unsigned short key);
    static void RemoveMRU(CACHENODE *h);
    int Size(DICTNODE*h) { return h->size; }
    void FNAddFiles(fnlist_t *fnlist, const char *pattern);
    void PrecacheSounds(void);
    void PurgeCache(void);
    void RemoveNode(DICTNODE* pNode);

    DICTNODE *dict;
    DICTNODE **indexName;
    DICTNODE **indexId;
    unsigned int buffSize;
    unsigned int count;
    int handle;
    bool crypt;

#if USE_QHEAP
    static QHeap *heap;
#endif
    static CACHENODE purgeHead;
};
