#ifdef USE_OPENGL

/*
Description of Ken's filter to improve LZW compression of DXT1 format by ~15%: (as tested with the HRP)

 To increase LZW patterns, I store each field of the DXT block structure separately.
 Here are the 3 DXT fields:
 1.  __int64 alpha_4x4; //DXT3 only (16 byte structure size when included)
 2.  short rgb0, rgb1;
 3.  int32_t index_4x4;

 Each field is then stored with its own specialized algorithm.
 1. I haven't done much testing with this field - I just copy it raw without any transform for now.

 2. For rgb0 and rgb1, I use a "green" filter like this:
 g = g;
 r = r-g;
 b = b-g;
 For grayscale, this makes the stream: x,0,0,x,0,0,x,0,0,... instead of x,x,x,x,x,x,x,x,...
 Q:what was the significance of choosing green? A:largest/most dominant component
 Believe it or not, this gave 1% better compression :P
 I tried subtracting each componenet with the previous pixel, but strangely it hurt compression.
 Oh, the joy of trial & error. :)

 3. For index_4x4, I transform the ordering of 2-bit indices in the DXT blocks from this:
 0123 0123 0123  ---- ---- ----
 4567 4567 4567  ---- ---- ----
 89ab 89ab 89ab  ---- ---- ----
 cdef cdef cdef  ---- ---- ----
 To this: (I swap x & y axes)
 048c 048c 048c  |||| |||| ||||
 159d 159d 159d  |||| |||| ||||
 26ae 26ae 26ae  |||| |||| ||||
 37bf 37bf 37bf  |||| |||| ||||
 The trick is: going from the bottom of the 4th line to the top of the 5th line
 is the exact same jump (geometrically) as from 5th to 6th, etc.. This is not true in the top case.
 These silly tricks will increase patterns and therefore make LZW compress better.
 I think this improved compression by a few % :)
 */

#include "compat.h"
#include "build.h"
#include "texcache.h"
#include "lz4.h"

#include <fcntl.h>
#ifdef _WIN32
# include <io.h>
#else
# include <unistd.h>
#endif

#ifndef EDUKE32_GLES
static uint16_t dxt_hicosub(uint16_t c)
{
    int32_t r, g, b;
    g = ((c>> 5)&63);
    r = ((c>>11)-(g>>1))&31;
    b = ((c>> 0)-(g>>1))&31;
    return ((r<<11)+(g<<5)+b);
}

static uint16_t dedxt_hicoadd(uint16_t c)
{
    int32_t r, g, b;
    g = ((c>> 5)&63);
    r = ((c>>11)+(g>>1))&31;
    b = ((c>> 0)+(g>>1))&31;
    return ((r<<11)+(g<<5)+b);
}
#endif

void dxt_handle_io(int32_t fil, int32_t len, void *midbuf, char *packbuf)
{
    void *writebuf;
    int32_t j, cleng;

    if (glusetexcache == 2)
    {
        cleng = LZ4_compress_default((const char*)midbuf, packbuf, len, len);

        if (cleng <= 0 || cleng > len-1)
        {
            cleng = len;
            writebuf = midbuf;
        }
        else writebuf = packbuf;
    }
    else
    {
        cleng = len;
        writebuf = midbuf;
    }

    // native -> external (little endian)
    j = B_LITTLE32(cleng);
    Bwrite(fil, &j, sizeof(j));
    Bwrite(fil, writebuf, cleng);
}

int32_t dedxt_handle_io(int32_t fil, int32_t j /* TODO: better name */,
                               void *midbuf, int32_t mbufsiz, char *packbuf, int32_t ispacked)
{
    void *inbuf;
    int32_t cleng;

    if (texcache_readdata(&cleng, sizeof(int32_t)))
        return -1;

    // external (little endian) -> native
    cleng = B_LITTLE32(cleng);

    inbuf = (ispacked && cleng < j) ? packbuf : midbuf;

    if (texcache.buf && texcache.memsize >= texcache.pos + cleng)
    {
        if (ispacked && cleng < j)
        {
            if (LZ4_decompress_safe((const char *)texcache.buf + texcache.pos, (char*)midbuf, cleng, mbufsiz) <= 0)
            {
                texcache.pos += cleng;
                return -1;
            }
        }
        else Bmemcpy(inbuf, texcache.buf + texcache.pos, cleng);

        texcache.pos += cleng;
    }
    else
    {
        Blseek(fil, texcache.pos, BSEEK_SET);
        texcache.pos += cleng;

        if (Bread(fil, inbuf, cleng) < cleng)
            return -1;

        if (ispacked && cleng < j)
            if (LZ4_decompress_safe(packbuf, (char*)midbuf, cleng, mbufsiz) <= 0)
                return -1;
    }

    return 0;
}

#ifndef EDUKE32_GLES
// NOTE: <pict> members are in external (little) endianness.
int32_t dxtfilter(int32_t fil, const texcachepicture *pict, const char *pic, void *midbuf, char *packbuf, uint32_t miplen)
{
    uint32_t j, k, offs, stride;
    char *cptr;

    if ((pict->format == (signed) B_LITTLE32(GL_COMPRESSED_RGB_S3TC_DXT1_EXT)) ||
            (pict->format == (signed) B_LITTLE32(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT))) { offs = 0; stride = 8; }
    else if ((pict->format == (signed) B_LITTLE32(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)) ||
             (pict->format == (signed) B_LITTLE32(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT))) { offs = 8; stride = 16; }
    else
        { offs = 0; stride = 8; }

    if (stride == 16) //If DXT3...
    {
        //alpha_4x4
        cptr = (char *)midbuf;
        for (k=0; k<8; k++) *cptr++ = pic[k];
        for (j=stride; (unsigned)j<miplen; j+=stride)
            for (k=0; k<8; k++) *cptr++ = pic[j+k];

        dxt_handle_io(fil, tabledivide32(miplen, stride)<<3, midbuf, packbuf);
    }

    //rgb0,rgb1
    cptr = (char *)midbuf;
    for (k=0; k<=2; k+=2)
        for (j=0; (unsigned)j<miplen; j+=stride)
            { B_BUF16(cptr, dxt_hicosub(B_UNBUF16(&pic[offs+j+k]))); cptr += 2; }

    dxt_handle_io(fil, tabledivide32(miplen, stride)<<2, midbuf, packbuf);

    //index_4x4
    cptr = (char *)midbuf;
    for (j=0; (unsigned)j<miplen; j+=stride)
    {
        const char *c2 = &pic[j+offs+4];
        cptr[0] = ((c2[0]>>0)&3) + (((c2[1]>>0)&3)<<2) + (((c2[2]>>0)&3)<<4) + (((c2[3]>>0)&3)<<6);
        cptr[1] = ((c2[0]>>2)&3) + (((c2[1]>>2)&3)<<2) + (((c2[2]>>2)&3)<<4) + (((c2[3]>>2)&3)<<6);
        cptr[2] = ((c2[0]>>4)&3) + (((c2[1]>>4)&3)<<2) + (((c2[2]>>4)&3)<<4) + (((c2[3]>>4)&3)<<6);
        cptr[3] = ((c2[0]>>6)&3) + (((c2[1]>>6)&3)<<2) + (((c2[2]>>6)&3)<<4) + (((c2[3]>>6)&3)<<6);
        cptr += 4;
    }

    dxt_handle_io(fil, tabledivide32(miplen, stride)<<2, midbuf, packbuf);

    return 0;
}

// NOTE: <pict> members are in native endianness.
int32_t dedxtfilter(int32_t fil, const texcachepicture *pict, char *pic, void *midbuf, char *packbuf, int32_t ispacked)
{
    int32_t j, k, offs, stride;
    char *cptr;

    if ((pict->format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT) ||
            (pict->format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)) { offs = 0; stride = 8; }
    else if ((pict->format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) ||
             (pict->format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)) { offs = 8; stride = 16; }
    else
        { offs = 0; stride = 8; }

    if (stride == 16) //If DXT3...
    {
        //alpha_4x4
        if (dedxt_handle_io(fil, tabledivide32(pict->size, stride)*8, midbuf, pict->size, packbuf, ispacked))
            return -1;

        cptr = (char *)midbuf;
        for (k=0; k<8; k++) pic[k] = *cptr++;
        for (j=stride; j<pict->size; j+=stride)
            for (k=0; k<8; k++) pic[j+k] = (*cptr++);
    }

    //rgb0,rgb1
    if (dedxt_handle_io(fil, tabledivide32(pict->size, stride)*4, midbuf, pict->size, packbuf, ispacked))
        return -1;

    cptr = (char *)midbuf;
    for (k=0; k<=2; k+=2)
    {
        for (j=0; j<pict->size; j+=stride)
        {
            B_BUF16(&pic[offs+j+k], dedxt_hicoadd(B_UNBUF16(cptr)));
            cptr += 2;
        }
    }

    //index_4x4:
    if (dedxt_handle_io(fil, tabledivide32(pict->size, stride)*4, midbuf, pict->size, packbuf, ispacked))
        return -1;

    cptr = (char *)midbuf;
    for (j=0; j<pict->size; j+=stride)
    {
        pic[j+offs+4] = ((cptr[0]>>0)&3) + (((cptr[1]>>0)&3)<<2) + (((cptr[2]>>0)&3)<<4) + (((cptr[3]>>0)&3)<<6);
        pic[j+offs+5] = ((cptr[0]>>2)&3) + (((cptr[1]>>2)&3)<<2) + (((cptr[2]>>2)&3)<<4) + (((cptr[3]>>2)&3)<<6);
        pic[j+offs+6] = ((cptr[0]>>4)&3) + (((cptr[1]>>4)&3)<<2) + (((cptr[2]>>4)&3)<<4) + (((cptr[3]>>4)&3)<<6);
        pic[j+offs+7] = ((cptr[0]>>6)&3) + (((cptr[1]>>6)&3)<<2) + (((cptr[2]>>6)&3)<<4) + (((cptr[3]>>6)&3)<<6);
        cptr += 4;
    }

    return 0;
}
#endif

#endif
