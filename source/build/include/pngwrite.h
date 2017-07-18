#ifndef __PNGWRITE_H__
#define __PNGWRITE_H__

#include "miniz.h"

#define CHUNK_COMPRESSED 1
#define CHUNK_ROW 2

enum
{
    PNG_TRUECOLOR = 2,
    PNG_INDEXED   = 3,
};

#pragma pack(push, 1)
typedef struct
{
    z_stream *zs;
    FILE *file;
    uint8_t *pal_data;
    uint16_t pal_entries;
    uint8_t *text;
    uint8_t textlen;
} pngwrite_t;

typedef struct
{
    uint32_t width, height;
    uint8_t  depth, type, filler[3];
} png_ihdr_t;
#pragma pack(pop)

void png_set_pal(uint8_t const * const data, int numentries);
void png_set_text(char const * const keyword, char const * const text);
void png_write(FILE * const file, uint32_t const width, uint32_t const height, uint8_t type, uint8_t const * const data);

#endif
