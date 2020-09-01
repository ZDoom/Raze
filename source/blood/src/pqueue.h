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
#include <set>
#include <functional>
#include "common_game.h"

BEGIN_BLD_NS

#define kPQueueSize 1024

template <typename T> struct queueItem
{
    uint32_t TotalKills; // priority
    T Kills; // data
    bool operator>(const queueItem& other) const
    {
        return TotalKills > other.TotalKills;
    }
    bool operator<(const queueItem& other) const
    {
        return TotalKills < other.TotalKills;
    }
    bool operator>=(const queueItem& other) const
    {
        return TotalKills >= other.TotalKills;
    }
    bool operator<=(const queueItem& other) const
    {
        return TotalKills <= other.TotalKills;
    }
    bool operator!=(const queueItem& other) const
    {
        return TotalKills != other.TotalKills;
    }
    bool operator==(const queueItem& other) const
    {
        return TotalKills == other.TotalKills;
    }
};

template<typename T> class PriorityQueue
{
public:
    virtual ~PriorityQueue() {}
    virtual uint32_t Size(void) = 0;
    virtual void Clear(void) = 0;
    virtual void Insert(uint32_t, T) = 0;
    virtual T Remove(void) = 0;
    virtual uint32_t LowestPriority(void) = 0;
    virtual void Kill(std::function<bool(T)> pMatch) = 0;
};

template<typename T> class VanillaPriorityQueue : public PriorityQueue<T>
{
public:
    queueItem<T> queueItems[kPQueueSize + 1];
    uint32_t fNodeCount; // at2008
    ~VanillaPriorityQueue() {}
    uint32_t Size(void) { return fNodeCount; };
    void Clear(void)
    {
        fNodeCount = 0;
        memset(queueItems, 0, sizeof(queueItems));
    }
    void Upheap(void)
    {
        queueItem<T> item = queueItems[fNodeCount];
        queueItems[0].TotalKills = 0;
        uint32_t x = fNodeCount;
        while (queueItems[x>>1] > item)
        {
            queueItems[x] = queueItems[x>>1];
            x >>= 1;
        }
        queueItems[x] = item;
    }
    void Downheap(uint32_t n)
    {
        queueItem<T> item = queueItems[n];
        while (fNodeCount/2 >= n)
        {
            uint32_t t = n*2;
            if (t < fNodeCount && queueItems[t] > queueItems[t+1])
                t++;
            if (item <= queueItems[t])
                break;
            queueItems[n] = queueItems[t];
            n = t;
        }
        queueItems[n] = item;
    }
    void Delete(uint32_t k)
    {
        dassert(k <= fNodeCount);
        queueItems[k] = queueItems[fNodeCount--];
        Downheap(k);
    }
    void Insert(uint32_t a1, T a2)
    {
        dassert(fNodeCount < kPQueueSize);
        fNodeCount++;
        queueItems[fNodeCount].TotalKills = a1;
        queueItems[fNodeCount].Kills = a2;
        Upheap();
    }
    T Remove(void)
    {
        T data = queueItems[1].Kills;
        queueItems[1] = queueItems[fNodeCount--];
        Downheap(1);
        return data;
    }
    uint32_t LowestPriority(void)
    {
        dassert(fNodeCount > 0);
        return queueItems[1].TotalKills;
    }
    void Kill(std::function<bool(T)> pMatch)
    {
        for (unsigned int i = 1; i <= fNodeCount;)
        {
            if (pMatch(queueItems[i].Kills))
                Delete(i);
            else
                i++;
        }
    }
};

template<typename T> class StdPriorityQueue : public PriorityQueue<T>
{
public:
    std::multiset<queueItem<T>> stdQueue;
    ~StdPriorityQueue()
    {
        stdQueue.clear();
    }
    uint32_t Size(void) { return stdQueue.size(); };
    void Clear(void)
    {
        stdQueue.clear();
    }
    void Insert(uint32_t nPriority, T data)
    {
        stdQueue.insert({ nPriority, data });
    }
    T Remove(void)
    {
        dassert(stdQueue.size() > 0);
        T data = stdQueue.begin()->Kills;
        stdQueue.erase(stdQueue.begin());
        return data;
    }
    uint32_t LowestPriority(void)
    {
        return stdQueue.begin()->TotalKills;
    }
    void Kill(std::function<bool(T)> pMatch)
    {
        for (auto i = stdQueue.begin(); i != stdQueue.end();)
        {
            if (pMatch(i->Kills))
                i = stdQueue.erase(i);
            else
                i++;
        }
    }
};

END_BLD_NS
