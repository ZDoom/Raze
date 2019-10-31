CACHENODE Resource::purgeHead = { NULL, &purgeHead, &purgeHead, 0 };


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


void Resource::RemoveMRU(CACHENODE *h)
{
    h->prev->next = h->next;
    h->next->prev = h->prev;
}


void Resource::PrecacheSounds(void)
{
    for (unsigned int i = 0; i < count; i++)
    {
        DICTNODE *pNode = &dict[i];
        if ((!strcmp(pNode->type, "RAW") || !strcmp(pNode->type, "SFX")) && !pNode->ptr)
        {
            Load(pNode);
            gameHandleEvents();
        }
    }
}
