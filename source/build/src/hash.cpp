
#include "compat.h"
#include "hash.h"
#include "baselayer.h"

void hash_init(hashtable_t *t)
{
    hash_free(t);
    t->items = (hashitem_t **) Xaligned_calloc(16, t->size, sizeof(hashitem_t));
}

void hash_loop(hashtable_t *t, void(*func)(const char *, intptr_t))
{
    if (t->items == nullptr)
        return;

    for (native_t i=0; i < t->size; i++)
        for (auto item = t->items[i]; item != nullptr; item = item->next)
            func(item->string, item->key);
}

void hash_free(hashtable_t *t)
{
    if (t->items == nullptr)
        return;

    int remaining = t->size - 1;

    do
    {
        auto cur = t->items[remaining];

        while (cur)
        {
            auto tmp = cur;
            cur = cur->next;

            Xfree(tmp->string);
            Xaligned_free(tmp);
        }
    } while (--remaining >= 0);

    ALIGNED_FREE_AND_NULL(t->items);
}

void hash_add(hashtable_t *t, const char *s, intptr_t key, int32_t replace)
{
#ifdef DEBUGGINGAIDS
    Bassert(t->items != nullptr);
#endif
    uint32_t const code = hash_getcode(s) % t->size;
    auto cur = t->items[code];

    if (!cur)
    {
        cur = (hashitem_t *) Xaligned_alloc(16, sizeof(hashitem_t));
        cur->string = Xstrdup(s);
        cur->key    = key;
        cur->next   = nullptr;

        t->items[code] = cur;
        return;
    }

    hashitem_t *prev = nullptr;

    do
    {
        if (Bstrcmp(s, cur->string) == 0)
        {
            if (replace) cur->key = key;
            return;
        }
        prev = cur;
    } while ((cur = cur->next));

    cur = (hashitem_t *) Xaligned_alloc(16, sizeof(hashitem_t));
    cur->string = Xstrdup(s);
    cur->key    = key;
    cur->next   = nullptr;

    prev->next = cur;
}

// delete at most once
void hash_delete(hashtable_t *t, const char *s)
{
#ifdef DEBUGGINGAIDS
    Bassert(t->items != nullptr);
#endif
    uint32_t const code = hash_getcode(s) % t->size;
    auto cur = t->items[code];

    if (!cur)
        return;

    hashitem_t *prev = nullptr;

    do
    {
        if (Bstrcmp(s, cur->string) == 0)
        {
            Xfree(cur->string);

            if (!prev)
                t->items[code] = cur->next;
            else
                prev->next = cur->next;

            Xaligned_free(cur);

            break;
        }
        prev = cur;
    } while ((cur = cur->next));
}

intptr_t hash_find(const hashtable_t * const t, char const * const s)
{
#ifdef DEBUGGINGAIDS
    Bassert(t->items != nullptr);
#endif
    auto cur = t->items[hash_getcode(s) % t->size];

    if (!cur)
        return -1;

    do
        if (Bstrcmp(s, cur->string) == 0)
            return cur->key;
    while ((cur = cur->next));

    return -1;
}

intptr_t hash_findcase(const hashtable_t * const t, char const * const s)
{
#ifdef DEBUGGINGAIDS
    Bassert(t->items != nullptr);
#endif
    auto cur = t->items[hash_getcode(s) % t->size];

    if (!cur)
        return -1;

    do
        if (Bstrcasecmp(s, cur->string) == 0)
            return cur->key;
    while ((cur = cur->next));

    return -1;
}


void inthash_free(inthashtable_t *t) { ALIGNED_FREE_AND_NULL(t->items); }

void inthash_init(inthashtable_t *t)
{
    inthash_free(t);
    t->items = (inthashitem_t *) Xaligned_calloc(16, t->count, sizeof(inthashitem_t));
}

void inthash_loop(inthashtable_t const *t, void(*func)(intptr_t, intptr_t))
{
    if (t->items == nullptr)
        return;

    for (auto *item = t->items, *const items_end = t->items + t->count; item < items_end; ++item)
        func(item->key, item->value);
}


void inthash_add(inthashtable_t *t, intptr_t key, intptr_t value, int32_t replace)
{
#ifdef DEBUGGINGAIDS
    Bassert(t->items != nullptr);
#endif
    auto seeker = t->items + inthash_getcode(key) % t->count;

    if (seeker->collision == nullptr)
    {
        seeker->key = key;
        seeker->value = value;
        seeker->collision = seeker;
        return;
    }

    if (seeker->key == key)
    {
        if (replace)
            seeker->value = value;
        return;
    }

    while (seeker != seeker->collision)
    {
        seeker = seeker->collision;

        if (seeker->key == key)
        {
            if (replace)
                seeker->value = value;
            return;
        }
    }

    auto tail = seeker;

    do
        tail = t->items + (tail - t->items + 1) % t->count;
    while (tail->collision != nullptr && tail != seeker);

    if (EDUKE32_PREDICT_FALSE(tail == seeker))
        fatal_exit("inthash_add(): table full!\n");

    tail->key = key;
    tail->value = value;
    tail->collision = seeker->collision = tail;
}

// delete at most once
void inthash_delete(inthashtable_t *t, intptr_t key)
{
#ifdef DEBUGGINGAIDS
    Bassert(t->items != nullptr);
#endif
    auto seeker = t->items + inthash_getcode(key) % t->count;

    if (seeker->collision == nullptr || seeker->key == key)
    {
        seeker->collision = nullptr;
        return;
    }

    while (seeker != seeker->collision)
    {
        auto prev = seeker;
        seeker = seeker->collision;

        if (seeker->key == key)
        {
            prev->collision = seeker == seeker->collision ? prev : seeker->collision;
            seeker->collision = nullptr;
            return;
        }
    }
}

intptr_t inthash_find(inthashtable_t const *t, intptr_t key)
{
#ifdef DEBUGGINGAIDS
    Bassert(t->items != nullptr);
#endif
    auto seeker = t->items + inthash_getcode(key) % t->count;

    if (seeker->collision == nullptr)
        return -1;

    if (seeker->key == key)
        return seeker->value;

    while (seeker != seeker->collision)
    {
        seeker = seeker->collision;

        if (seeker->key == key)
            return seeker->value;
    }

    return -1;
}
