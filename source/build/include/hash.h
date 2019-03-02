
#pragma once

#ifndef hash_h_
#define hash_h_

#ifdef __cplusplus
extern "C" {
#endif

// Hash functions

typedef struct hashitem_ // size is 12/24 bytes.
{
    char *string;
    intptr_t key;
    struct hashitem_ *next;
} hashitem_t;

typedef struct
{
    int32_t size;
    hashitem_t **items;
} hashtable_t;

// djb3 algorithm
static inline uint32_t hash_getcode(const char *s)
{
    uint32_t h = 5381;
    int32_t ch;

    while ((ch = Btolower(*s++)) != '\0')
        h = ((h << 5) + h) ^ ch;

    return h;
}

void hash_init(hashtable_t *t);
void hash_loop(hashtable_t *t, void(*func)(const char *, intptr_t));
void hash_free(hashtable_t *t);
intptr_t  hash_findcase(hashtable_t const * t, char const * s);
intptr_t  hash_find(hashtable_t const * t, char const * s);
void hash_add(hashtable_t *t, const char *s, intptr_t key, int32_t replace);
void hash_delete(hashtable_t *t, const char *s);


// Hash functions
// modified for raw binary keys and one big allocation, and maximum find() performance

typedef struct inthashitem_
{
    intptr_t key;
    intptr_t value;
    struct inthashitem_ *collision; // use NULL to signify empty and pointer identity to signify end of linked list
} inthashitem_t;

typedef struct
{
    inthashitem_t * items;
    uint32_t count;
} inthashtable_t;

// djb3 algorithm
static inline uint32_t inthash_getcode(intptr_t key)
{
    uint32_t h = 5381;

    for (uint8_t const * keybuf = (uint8_t *) &key, *const keybuf_end = keybuf + sizeof(intptr_t); keybuf < keybuf_end; ++keybuf)
        h = ((h << 5) + h) ^ (uint32_t) *keybuf;

    return h;
}

void inthash_init(inthashtable_t *t);
void inthash_loop(inthashtable_t const *t, void(*func)(intptr_t, intptr_t));
void inthash_free(inthashtable_t *t);
intptr_t  inthash_find(inthashtable_t const *t, intptr_t key);
void inthash_add(inthashtable_t *t, intptr_t key, intptr_t value, int32_t replace);
void inthash_delete(inthashtable_t *t, intptr_t key);
// keep the load factor below 0.75 and make sure the size is odd
// ideally we would find the next largest prime number
#define INTHASH_SIZE(size) ((size * 4u / 3u) | 1u)

#ifdef __cplusplus
}
#endif

#endif
