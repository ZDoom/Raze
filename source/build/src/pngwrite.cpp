#include "compat.h"
#include "pngwrite.h"
#include "crc32.h"

pngwrite_t png;

#define png_write_buf(p, size) Bfwrite(p, size, 1, png.file)

static void png_write_uint32(uint32_t const in)
{
    uint8_t buf[4];
    *(uint32_t *)buf = B_BIG32(in);
    png_write_buf(buf, sizeof(uint32_t));
}

static void png_write_chunk(uint32_t const size, char const *const type,
                            uint8_t const *const data, uint32_t flags)
{
    mz_ulong chunk_size = flags & CHUNK_COMPRESSED ? compressBound(size) : size;
    uint8_t * const chunk = (uint8_t *) Xcalloc(1, 4 + chunk_size);

    Bmemcpy(chunk, type, 4);

    if (flags & CHUNK_COMPRESSED)
        compress(chunk + 4, (mz_ulong *) &chunk_size, data, size);
    else
        Bmemcpy(chunk + 4, data, size);

    png_write_uint32(chunk_size);
    png_write_buf(chunk, chunk_size + 4);

    uint32_t crc = Bcrc32(NULL, 0, 0L);
    crc = Bcrc32(chunk, chunk_size + 4, crc);
    png_write_uint32(crc);

    Bfree(chunk);
}

void png_set_pal(uint8_t const * const data, int numentries)
{
    png.pal_entries = numentries;
    png.pal_data    = (uint8_t *)Xmalloc(numentries * 3);

    Bmemcpy(png.pal_data, data, numentries * 3);
}

void png_set_text(char const * const keyword, char const * const text)
{
    unsigned const keylen  = Bstrlen(keyword);
    Bassert(keylen < 79);
    unsigned const textlen = Bstrlen(text);

    png.textlen = keylen + textlen + 1;
    png.text = (uint8_t *) Xrealloc(png.text, png.textlen);

    Bmemcpy(png.text, keyword, keylen);
    *(png.text + keylen) = 0;
    Bmemcpy(png.text + keylen + 1, text, textlen);
}

void png_write(FILE * const file, uint32_t const width, uint32_t const height,
               uint8_t type, uint8_t const * const data)
{
    png.file = file;

    png_write_buf("\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8);

    png_ihdr_t const png_header = { B_BIG32(width), B_BIG32(height), 8, type, 0  };
    png_write_chunk(sizeof(png_ihdr_t), "IHDR", (uint8_t const *)&png_header, 0);

    if (png.text)
    {
        png_write_chunk(png.textlen, "tEXt", png.text, 0);
        DO_FREE_AND_NULL(png.text);
    }

    uint32_t const bytesPerPixel = (type == PNG_TRUECOLOR ? 3 : 1);
    uint32_t const bytesPerLine  = width * bytesPerPixel;

    if (type == PNG_INDEXED)
        png_write_chunk(png.pal_entries * 3, "PLTE", png.pal_data, 0);

    unsigned const linesiz = height * bytesPerLine + height;
    uint8_t *lines = (uint8_t *) Xcalloc(1, linesiz);

    for (unsigned i = 0; i < height; i++)
        Bmemcpy(lines + i * bytesPerLine + i + 1, data + i * bytesPerLine, bytesPerLine);

    png_write_chunk(linesiz, "IDAT", lines, CHUNK_COMPRESSED);
    png_write_chunk(0,       "IEND", NULL,  0);

    Bfree(lines);
}
