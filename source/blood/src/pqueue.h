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
#define kPQueueSize 1024

struct queueItem
{
    unsigned int at0; // priority
    unsigned int at4; // data
    bool operator>(const queueItem& other) const
    {
        return at0 > other.at0;
    }
    bool operator<(const queueItem& other) const
    {
        return at0 < other.at0;
    }
    bool operator>=(const queueItem& other) const
    {
        return at0 >= other.at0;
    }
    bool operator<=(const queueItem& other) const
    {
        return at0 <= other.at0;
    }
    bool operator!=(const queueItem& other) const
    {
        return at0 != other.at0;
    }
    bool operator==(const queueItem& other) const
    {
        return at0 == other.at0;
    }
};

class PriorityQueue
{
public:
    virtual ~PriorityQueue() = 0;
    virtual unsigned int Size(void) = 0;
    virtual void Clear(void) = 0;
    virtual void Insert(unsigned int, unsigned int) = 0;
    virtual unsigned int Remove(void) = 0;
    virtual unsigned int LowestPriority(void) = 0;
    virtual void Kill(std::function<bool(unsigned int)> pMatch) = 0;
};

class VanillaPriorityQueue : public PriorityQueue
{
public:
    queueItem queueItems[kPQueueSize + 1];
    unsigned int fNodeCount; // at2008
    ~VanillaPriorityQueue();
    unsigned int Size(void) { return fNodeCount; };
    void Clear(void);
    void Upheap(void);
    void Downheap(unsigned int);
    void Delete(unsigned int);
    void Insert(unsigned int, unsigned int);
    unsigned int Remove(void);
    unsigned int LowestPriority(void);
    void Kill(std::function<bool(unsigned int)> pMatch);
};

class StdPriorityQueue : public PriorityQueue
{
public:
    std::multiset<queueItem> stdQueue;
    ~StdPriorityQueue();
    unsigned int Size(void) { return stdQueue.size(); };
    void Clear(void);
    void Insert(unsigned int, unsigned int);
    unsigned int Remove(void);
    unsigned int LowestPriority(void);
    void Kill(std::function<bool(unsigned int)> pMatch);
};
