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
#include "common_game.h"
#include "pqueue.h"

PriorityQueue::~PriorityQueue()
{
}

VanillaPriorityQueue::~VanillaPriorityQueue()
{
}

void VanillaPriorityQueue::Clear(void)
{
    fNodeCount = 0;
    memset(queueItems, 0, sizeof(queueItems));
}

void VanillaPriorityQueue::Upheap(void)
{
    queueItem item = queueItems[fNodeCount];
    queueItems[0].at0 = 0;
    unsigned int x = fNodeCount;
    while (queueItems[x>>1] > item)
    {
        queueItems[x] = queueItems[x>>1];
        x >>= 1;
    }
    queueItems[x] = item;
}

void VanillaPriorityQueue::Downheap(unsigned int n)
{
    queueItem item = queueItems[n];
    while (fNodeCount/2 >= n)
    {
        unsigned int t = n*2;
        if (t < fNodeCount && queueItems[t] > queueItems[t+1])
            t++;
        if (item <= queueItems[t])
            break;
        queueItems[n] = queueItems[t];
        n = t;
    }
    queueItems[n] = item;
}

void VanillaPriorityQueue::Delete(unsigned int k)
{
    dassert(k <= fNodeCount);
    queueItems[k] = queueItems[fNodeCount--];
    Downheap(k);
}

void VanillaPriorityQueue::Insert(unsigned int a1, unsigned int a2)
{
    dassert(fNodeCount < kPQueueSize);
    fNodeCount++;
    queueItems[fNodeCount].at0 = a1;
    queueItems[fNodeCount].at4 = a2;
    Upheap();
}

unsigned int VanillaPriorityQueue::Remove(void)
{
    unsigned int data = queueItems[1].at4;
    queueItems[1] = queueItems[fNodeCount--];
    Downheap(1);
    return data;
}

unsigned int VanillaPriorityQueue::LowestPriority(void)
{
    dassert(fNodeCount > 0);
    return queueItems[1].at0;
}

void VanillaPriorityQueue::Kill(std::function<bool(unsigned int)> pMatch)
{
    for (unsigned int i = 1; i <= fNodeCount;)
    {
        if (pMatch(queueItems[i].at4))
            Delete(i);
        else
            i++;
    }
}

StdPriorityQueue::~StdPriorityQueue()
{
    stdQueue.clear();
}

void StdPriorityQueue::Clear(void)
{
    stdQueue.clear();
}

void StdPriorityQueue::Insert(unsigned int nPriority, unsigned int nData)
{
    stdQueue.insert({ nPriority, nData });
}

unsigned int StdPriorityQueue::Remove(void)
{
    dassert(stdQueue.size() > 0);
    int nData = stdQueue.begin()->at4;
    stdQueue.erase(stdQueue.begin());
    return nData;
}

unsigned int StdPriorityQueue::LowestPriority(void)
{
    return stdQueue.begin()->at0;
}

void StdPriorityQueue::Kill(std::function<bool(unsigned int)> pMatch)
{
    for (auto i = stdQueue.begin(); i != stdQueue.end();)
    {
        if (pMatch(i->at4))
            i = stdQueue.erase(i);
        else
            i++;
    }
}
