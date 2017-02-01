
#include "compat.h"
#include "hash.h"
#include "baselayer.h"

void hash_init(hashtable_t *t)
{
    hash_free(t);
    t->items=(hashitem_t **) Xcalloc(1, t->size * sizeof(hashitem_t));
}

void hash_loop(hashtable_t *t, void(*func)(const char *, intptr_t))
{
    if (t->items == NULL)
        return;

    for (bssize_t i=0; i < t->size; i++)
        for (hashitem_t *item=t->items[i]; item != NULL; item = item->next)
            func(item->string, item->key);
}

void hash_free(hashtable_t *t)
{
    if (t->items == NULL)
        return;

    int remaining = t->size - 1;

    do
    {
        hashitem_t *cur = t->items[remaining];

        int num = 0;

        while (cur)
        {
            hashitem_t * const tmp = cur;
            cur = cur->next;

            Bfree(tmp->string);
            Bfree(tmp);
            num++;
        }
    } while (--remaining >= 0);

    DO_FREE_AND_NULL(t->items);
}

// djb3 algorithm
static inline uint32_t hash_getcode(const char *s)
{
    uint32_t h = 5381;
    int32_t ch;

    while ((ch = *s++) != '\0')
        h = ((h << 5) + h) ^ ch;

    return h;
}

void hash_add(hashtable_t *t, const char *s, intptr_t key, int32_t replace)
{
#ifdef DEBUGGINGAIDS
    Bassert(t->items != NULL);
#else
    if (EDUKE32_PREDICT_FALSE(t->items == NULL))
    {
        initprintf("hash_add(): table not initialized!\n");
        return;
    }
#endif

    uint32_t code = hash_getcode(s) % t->size;
    hashitem_t *cur = t->items[code];

    if (!cur)
    {
        cur = (hashitem_t *) Xcalloc(1, sizeof(hashitem_t));
        cur->string = Xstrdup(s);
        cur->key = key;
        cur->next = NULL;
        t->items[code] = cur;
        return;
    }

    hashitem_t *prev = NULL;

    do
    {
        if (Bstrcmp(s, cur->string) == 0)
        {
            if (replace) cur->key = key;
            return;
        }
        prev = cur;
    } while ((cur = cur->next));

    cur = (hashitem_t *) Xcalloc(1, sizeof(hashitem_t));
    cur->string = Xstrdup(s);
    cur->key = key;
    cur->next = NULL;
    prev->next = cur;
}

// delete at most once
void hash_delete(hashtable_t *t, const char *s)
{
#ifdef DEBUGGINGAIDS
    Bassert(t->items != NULL);
#else
    if (t->items == NULL)
    {
        initprintf("hash_delete(): table not initialized!\n");
        return;
    }
#endif

    uint32_t code = hash_getcode(s) % t->size;
    hashitem_t *cur = t->items[code];

    if (!cur)
        return;

    hashitem_t *prev = NULL;

    do
    {
        if (Bstrcmp(s, cur->string) == 0)
        {
            Bfree(cur->string);

            if (!prev)
                t->items[code] = cur->next;
            else
                prev->next = cur->next;

            Bfree(cur);

            break;
        }
        prev = cur;
    } while ((cur = cur->next));
}

intptr_t hash_find(const hashtable_t * const t, char const * const s)
{
#ifdef DEBUGGINGAIDS
    Bassert(t->items != NULL);
#else
    if (t->items == NULL)
    {
        initprintf("hash_find(): table not initialized!\n");
        return -1;
    }
#endif

    hashitem_t *cur = t->items[hash_getcode(s) % t->size];

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
    Bassert(t->items != NULL);
#else
    if (t->items == NULL)
    {
        initprintf("hash_findcase(): table not initialized!\n");
        return -1;
    }
#endif

    hashitem_t *cur = t->items[hash_getcode(s) % t->size];

    if (!cur)
        return -1;

    do
        if (Bstrcasecmp(s, cur->string) == 0)
            return cur->key;
    while ((cur = cur->next));

    return -1;
}


void inthash_init(inthashtable_t *t)
{
    if (EDUKE32_PREDICT_FALSE(!t->count))
    {
        initputs("inthash_add(): count is zero!\n");
        return;
    }

    inthash_free(t);

    t->items = (inthashitem_t *) Xcalloc(t->count, sizeof(inthashitem_t));
}

void inthash_loop(inthashtable_t const *t, void(*func)(intptr_t, intptr_t))
{
#ifdef DEBUGGINGAIDS
    Bassert(t->items != NULL);
#else
    if (EDUKE32_PREDICT_FALSE(t->items == NULL))
    {
        initputs("inthash_loop(): table not initialized!\n");
        return;
    }
#endif

    for (inthashitem_t const * item = t->items, *const items_end = t->items + t->count; item < items_end; ++item)
        func(item->key, item->value);
}

void inthash_free(inthashtable_t *t)
{
    DO_FREE_AND_NULL(t->items);
}

// djb3 algorithm
static inline uint32_t inthash_getcode(intptr_t key)
{
    uint32_t h = 5381;

    for (uint8_t const * keybuf = (uint8_t *) &key, *const keybuf_end = keybuf + sizeof(intptr_t); keybuf < keybuf_end; ++keybuf)
        h = ((h << 5) + h) ^ (uint32_t) *keybuf;

    return h;
}

void inthash_add(inthashtable_t *t, intptr_t key, intptr_t value, int32_t replace)
{
#ifdef DEBUGGINGAIDS
    Bassert(t->items != NULL);
#else
    if (EDUKE32_PREDICT_FALSE(t->items == NULL))
    {
        initputs("inthash_add(): table not initialized!\n");
        return;
    }
#endif

    inthashitem_t * seeker = t->items + inthash_getcode(key) % t->count;

    if (seeker->collision == NULL)
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

    inthashitem_t *tail = seeker;

    do
        tail = t->items + (tail - t->items + 1) % t->count;
    while (tail->collision != NULL && tail != seeker);

    if (EDUKE32_PREDICT_FALSE(tail == seeker))
    {
        initputs("inthash_add(): table full!\n");
        return;
    }

    tail->key = key;
    tail->value = value;
    tail->collision = seeker->collision = tail;
}

// delete at most once
void inthash_delete(inthashtable_t *t, intptr_t key)
{
#ifdef DEBUGGINGAIDS
    Bassert(t->items != NULL);
#else
    if (EDUKE32_PREDICT_FALSE(t->items == NULL))
    {
        initputs("inthash_delete(): table not initialized!\n");
        return;
    }
#endif

    inthashitem_t * seeker = t->items + inthash_getcode(key) % t->count;

    if (seeker->collision == NULL)
        return;

    if (seeker->key == key)
    {
        seeker->collision = NULL;
        return;
    }

    while (seeker != seeker->collision)
    {
        inthashitem_t * const prev = seeker;
        seeker = seeker->collision;

        if (seeker->key == key)
        {
            prev->collision = seeker == seeker->collision ? prev : seeker->collision;
            seeker->collision = NULL;
            return;
        }
    }
}

intptr_t inthash_find(inthashtable_t const *t, intptr_t key)
{
#ifdef DEBUGGINGAIDS
    Bassert(t->items != NULL);
#else
    if (EDUKE32_PREDICT_FALSE(t->items == NULL))
    {
        initputs("inthash_find(): table not initialized!\n");
        return -1;
    }
#endif

    inthashitem_t const * seeker = t->items + inthash_getcode(key) % t->count;

    if (seeker->collision == NULL)
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
