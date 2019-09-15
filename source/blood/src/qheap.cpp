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
#ifdef USE_QHEAP

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "common_game.h"

#include "qheap.h"

void InstallFenceposts(HEAPNODE *n)
{
    char *address = (char*)n;
    memset(address + 0x20, 0xcc, 0x10);
    memset(address + n->size - 0x10, 0xcc, 0x10);
}

void CheckFenceposts(HEAPNODE *n)
{
    char *data = (char*)n + 0x20;
    for (int i = 0; i < 0x10; i++)
    {
        if (data[i] != 0xcc)
        {
            ThrowError("Block underwritten");
        }
    }
    data = (char*)n + n->size - 0x10;
    for (int i = 0; i < 0x10; i++)
    {
        if (data[i] != 0xcc)
        {
            ThrowError("Block overwritten");
        }
    }
}

QHeap::QHeap(int heapSize)
{
    dassert(heapSize > 0);
    size = heapSize;
    while (size > 0 && (heapPtr = malloc(size)) == NULL)
    {
        size -= 0x1000;
    }
    if (!heapPtr)
    {
        ThrowError("Allocation failure\n");
    }
    heap.isFree = false;
    freeHeap.isFree = false;
    HEAPNODE *node = (HEAPNODE*)(((intptr_t)heapPtr + 0xf) & ~0xf);
    heap.next = heap.prev = node;
    node->next = node->prev = &heap;
    freeHeap.freeNext = freeHeap.freePrev = node;
    node->freeNext = node->freePrev = &freeHeap;
    node->isFree = true;
    node->size = size & ~0xf;
}

QHeap::~QHeap(void)
{
    Check();
    free(heapPtr);
    heapPtr = NULL;
}

void QHeap::Check(void)
{
    HEAPNODE *node = heap.next;
    while (node != &heap)
    {
        if (!node->isFree)
        {
            CheckFenceposts(node);
        }
        node = node->next;
    }
}

void QHeap::Debug(void)
{
#if 0
    char s[4];
    FILE *f = fopen("MEMFRAG.TXT", "wt");
    if (!f)
    {
        return;
    }
    HEAPNODE *node = heap.next;
    while (node != &heap)
    {
        if (node->isFree)
        {
            fprintf(f, "%P %10d FREE", node, node->size);
        }
        else
        {
            char *data = (char*)node + 0x20;
            for (int i = 0; i < 4; i++)
            {
                if (isalpha(data[i]))
                {
                    s[i] = data[i];
                }
                else
                {
                    s[i] = '_';
                }
            }
            fprintf(f, "%P %10d %4s", node, node->size, s);
        }
    }
    fclose(f);
#endif
}

void *QHeap::Alloc(int blockSize)
{
    dassert(blockSize > 0);
    dassert(heapPtr != NULL);
    if (blockSize > 0)
    {
        blockSize = ((blockSize + 0xf) & ~0xf) + 0x40;
        HEAPNODE *freeNode = freeHeap.freeNext;
        while (freeNode != &freeHeap)
        {
            dassert(freeNode->isFree);
            if (blockSize <= freeNode->size)
            {
                if (blockSize + 0x30 <= freeNode->size)
                {
                    freeNode->size -= blockSize;
                    HEAPNODE *nextNode = (HEAPNODE *)((char*)freeNode + freeNode->size);
                    nextNode->size = blockSize;
                    nextNode->prev = freeNode;
                    nextNode->next = freeNode->next;
                    nextNode->prev->next = nextNode;
                    nextNode->next->prev = nextNode;
                    nextNode->isFree = false;
                    InstallFenceposts(nextNode);
                    return (void*)((char*)nextNode + 0x30);
                }
                else
                {
                    freeNode->freePrev->freeNext = freeNode->freeNext;
                    freeNode->freeNext->freePrev = freeNode->freePrev;
                    freeNode->isFree = false;
                    InstallFenceposts(freeNode);
                    return (void*)((char*)freeNode + 0x30);
                }
            }
            freeNode = freeNode->freeNext;
        }
    }
    return NULL;
}

int QHeap::Free(void *p)
{
    if (!p)
    {
        return 0;
    }
    dassert(heapPtr != NULL);
    HEAPNODE *node = (HEAPNODE*)((char*)p - 0x30);
    if (node->isFree)
    {
        ThrowError("Free on bad or freed block");
    }
    CheckFenceposts(node);
    if (node->prev->isFree)
    {
        node->prev->size += node->size;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        node = node->prev;
    }
    else
    {
        node->freeNext = freeHeap.freeNext;
        node->freePrev = &freeHeap;
        node->freePrev->freeNext = node;
        node->freeNext->freePrev = node;
        node->isFree = true;
    }
    HEAPNODE *nextNode = node->next;
    if (nextNode->isFree)
    {
        node->size += nextNode->size;
        nextNode->freePrev->freeNext = nextNode->freeNext;
        nextNode->freeNext->freePrev = nextNode->freePrev;
        nextNode->prev->next = nextNode->next;
        nextNode->next->prev = nextNode->prev;
    }
    return node->size - 0x40;
}

#endif
