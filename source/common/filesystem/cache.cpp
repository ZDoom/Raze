

struct CacheNode
{
	size_t size;
	CacheNode *next, *prev;
	int lockCount;
	int lastusetick;	// This is to ensure that a node lives for the duration of the frame it is last axxessed on
	
	virtual ~CacheNode();
	virtual void Purge() = 0;	// needs to be implemented by the child class to allow different types of menory to be used.
};

class Cache
{
	size_t maxSize;
	size_t currentSize;
	CacheNode purgeHead;
	int currenttick;

public:
	Cache()
	{
		maxSize = 100'000'000;
		currentSize = 0;
		purgeHead = { 0, &purgeHead, &purgeHead, 0 };
	}
	
	SetSize(size_t newsize)
	{
		if (newsize < maxSize) Purge();
		maxSize = newsize;
	}
	
	void AddToPurgeList(CacheNode *h)
	{
		h->prev = purgeHead.prev;
		purgeHead.prev->next = h;
		h->next = &purgeHead;
		purgeHead.prev = h;
	}
	
	void RemoveFromPurgeList(CacheNode *h)
	{
		h->prev->next = h->next;
		h->next->prev = h->prev;
	}
	
	void Alloc(CacheNode *h)
	{
		currentSize += h->size;
		if (currentSize > maxSize)
		{
			Purge();
		}
		AddToPurgeList(h);
	}
	
	void Release(CacheNode *h)
	{
		currentSize -= h->size;
		RemoveFromPurgeList(h);
	}
	
	void Validata(CacheNode *h)
	{
		if (h->LockCount == 0)
		{
			// Move node to the top of the linked list.
			RemoveFromPurgeList(h);
			AddToPurgeList(h);
		}
	}
	
	void *Lock(CacheNode *h)
	{
		// This merely locks the node. Allocation must be reported separately.
		assert(h != NULL);
		if (h->lockCount == 0)
		{
			RemoveFromPurgeList(h);
		}

		h->lockCount++;
		return h->ptr;
	}

	void Unlock(CacheNode *h)
	{
		assert(h != NULL);
		if (h->lockCount > 0)
		{
			h->lockCount--;
			if (h->lockCount == 0)
			{
				AddToPurgeList();
				if (currentSize > maxSize)
				{	
					Release(h);
					h->Purge();
				}
			}
		}
	}
	
	void PurgeCache(bool all = false)
	{
		// Do not delete from the list while it's being iterated. Better store in a temporary list and delete from there.
		TArray<CacheNode *> nodesToPurge(10);
		int purgeSize = 0;
		for (CacheNode *node = purgeHead.next; node != &purgeHead; node = node->next)
		{
			if (node->lastusetick < currenttick)
			{
				nodesToPurge.Push(npde);
				purgeSize += node->size;
				if (currentSize - purgeSize < maxSize && !all) break;
			}
		}
		for (auto h : nodesToPurge)
		{
			Release(h);
			h->Purge();
		}			
	}
};



