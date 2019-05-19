// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#include "compat.h"
#include "klzw.h"

//Internal LZW variables
#define LZWSIZE 16384           //Watch out for shorts!
#define LZWSIZEPAD (LZWSIZE+(LZWSIZE>>4))

// lzwrawbuf LZWSIZE+1 (formerly): see (*) below
// XXX: lzwrawbuf size increased again :-/
static char lzwtmpbuf[LZWSIZEPAD], lzwrawbuf[LZWSIZEPAD], lzwcompbuf[LZWSIZEPAD];
static int16_t lzwbuf2[LZWSIZEPAD], lzwbuf3[LZWSIZEPAD];


////////// CORE COMPRESSION FUNCTIONS //////////

static int32_t lzwcompress(const char *lzwinbuf, int32_t uncompleng, char *lzwoutbuf)
{
    int32_t i, addr, addrcnt, *intptr;
    int32_t bytecnt1, bitcnt, numbits, oneupnumbits;
    int16_t *shortptr;

    int16_t *const lzwcodehead = lzwbuf2;
    int16_t *const lzwcodenext = lzwbuf3;

    for (i=255; i>=4; i-=4)
    {
        lzwtmpbuf[i]   = i,   lzwcodenext[i]   = (i+1)&255;
        lzwtmpbuf[i-1] = i-1, lzwcodenext[i-1] = (i)  &255;
        lzwtmpbuf[i-2] = i-2, lzwcodenext[i-2] = (i-1)&255;
        lzwtmpbuf[i-3] = i-3, lzwcodenext[i-3] = (i-2)&255;
        lzwcodehead[i] = lzwcodehead[i-1] = lzwcodehead[i-2] = lzwcodehead[i-3] = -1;
    }

    for (; i>=0; i--)
    {
        lzwtmpbuf[i] = i;
        lzwcodenext[i] = (i+1)&255;
        lzwcodehead[i] = -1;
    }

    Bmemset(lzwoutbuf, 0, 4+uncompleng+1);
//    clearbuf(lzwoutbuf,((uncompleng+15)+3)>>2,0L);

    addrcnt = 256; bytecnt1 = 0; bitcnt = (4<<3);
    numbits = 8; oneupnumbits = (1<<8);
    do
    {
        addr = lzwinbuf[bytecnt1];
        do
        {
            int32_t newaddr;

            if (++bytecnt1 == uncompleng)
                break;  // (*) see XXX below

            if (lzwcodehead[addr] < 0)
            {
                lzwcodehead[addr] = addrcnt;
                break;
            }

            newaddr = lzwcodehead[addr];
            while (lzwtmpbuf[newaddr] != lzwinbuf[bytecnt1])
            {
                if (lzwcodenext[newaddr] < 0)
                {
                    lzwcodenext[newaddr] = addrcnt;
                    break;
                }
                newaddr = lzwcodenext[newaddr];
            }

            if (lzwcodenext[newaddr] == addrcnt)
                break;
            addr = newaddr;
        }
        while (addr >= 0);

        lzwtmpbuf[addrcnt] = lzwinbuf[bytecnt1];  // XXX: potential oob access of lzwinbuf via (*) above
        lzwcodehead[addrcnt] = -1;
        lzwcodenext[addrcnt] = -1;

        intptr = (int32_t *)&lzwoutbuf[bitcnt>>3];
        intptr[0] |= B_LITTLE32(addr<<(bitcnt&7));
        bitcnt += numbits;
        if ((addr&((oneupnumbits>>1)-1)) > ((addrcnt-1)&((oneupnumbits>>1)-1)))
            bitcnt--;

        addrcnt++;
        if (addrcnt > oneupnumbits)
            { numbits++; oneupnumbits <<= 1; }
    }
    while ((bytecnt1 < uncompleng) && (bitcnt < (uncompleng<<3)));

    intptr = (int32_t *)&lzwoutbuf[bitcnt>>3];
    intptr[0] |= B_LITTLE32(addr<<(bitcnt&7));
    bitcnt += numbits;
    if ((addr&((oneupnumbits>>1)-1)) > ((addrcnt-1)&((oneupnumbits>>1)-1)))
        bitcnt--;

    shortptr = (int16_t *)lzwoutbuf;
    shortptr[0] = B_LITTLE16((int16_t)uncompleng);

    if (((bitcnt+7)>>3) < uncompleng)
    {
        shortptr[1] = B_LITTLE16((int16_t)addrcnt);
        return (bitcnt+7)>>3;
    }

    // Failed compressing, mark this in the stream.
    shortptr[1] = 0;

    for (i=0; i<uncompleng-4; i+=4)
    {
        lzwoutbuf[i+4] = lzwinbuf[i];
        lzwoutbuf[i+5] = lzwinbuf[i+1];
        lzwoutbuf[i+6] = lzwinbuf[i+2];
        lzwoutbuf[i+7] = lzwinbuf[i+3];
    }

    for (; i<uncompleng; i++)
        lzwoutbuf[i+4] = lzwinbuf[i];

    return uncompleng+4;
}

static int32_t lzwuncompress(const char *lzwinbuf, int32_t compleng, char *lzwoutbuf)
{
    int32_t currstr, numbits, oneupnumbits;
    int32_t i, bitcnt, outbytecnt;

    const int16_t *const shortptr = (const int16_t *)lzwinbuf;
    const int32_t strtot = B_LITTLE16(shortptr[1]);
    const int32_t uncompleng = B_LITTLE16(shortptr[0]);

    if (strtot == 0)
    {
        if (lzwoutbuf==lzwrawbuf && lzwinbuf==lzwcompbuf)
        {
            Bassert((compleng-4)+3+0u < sizeof(lzwrawbuf));
            Bassert((compleng-4)+3+0u < sizeof(lzwcompbuf)-4);
        }

        Bmemcpy(lzwoutbuf, lzwinbuf+4, (compleng-4)+3);
        return uncompleng;
    }

    for (i=255; i>=4; i-=4)
    {
        lzwbuf2[i]   = lzwbuf3[i]   = i;
        lzwbuf2[i-1] = lzwbuf3[i-1] = i-1;
        lzwbuf2[i-2] = lzwbuf3[i-2] = i-2;
        lzwbuf2[i-3] = lzwbuf3[i-3] = i-3;
    }

    lzwbuf2[i]   = lzwbuf3[i]   = i;
    lzwbuf2[i-1] = lzwbuf3[i-1] = i-1;
    lzwbuf2[i-2] = lzwbuf3[i-2] = i-2;

    currstr = 256; bitcnt = (4<<3); outbytecnt = 0;
    numbits = 8; oneupnumbits = (1<<8);
    do
    {
        const int32_t *const intptr = (const int32_t *)&lzwinbuf[bitcnt>>3];

        int32_t dat = ((B_LITTLE32(intptr[0])>>(bitcnt&7)) & (oneupnumbits-1));
        int32_t leng;

        bitcnt += numbits;
        if ((dat&((oneupnumbits>>1)-1)) > ((currstr-1)&((oneupnumbits>>1)-1)))
            { dat &= ((oneupnumbits>>1)-1); bitcnt--; }

        lzwbuf3[currstr] = dat;

        for (leng=0; dat>=256; leng++,dat=lzwbuf3[dat])
            lzwtmpbuf[leng] = lzwbuf2[dat];

        lzwoutbuf[outbytecnt++] = dat;

        for (i=leng-1; i>=4; i-=4, outbytecnt+=4)
        {
            lzwoutbuf[outbytecnt]   = lzwtmpbuf[i];
            lzwoutbuf[outbytecnt+1] = lzwtmpbuf[i-1];
            lzwoutbuf[outbytecnt+2] = lzwtmpbuf[i-2];
            lzwoutbuf[outbytecnt+3] = lzwtmpbuf[i-3];
        }

        for (; i>=0; i--)
            lzwoutbuf[outbytecnt++] = lzwtmpbuf[i];

        lzwbuf2[currstr-1] = dat; lzwbuf2[currstr] = dat;
        currstr++;
        if (currstr > oneupnumbits)
            { numbits++; oneupnumbits <<= 1; }
    }
    while (currstr < strtot);

    return uncompleng;
}


////////// COMPRESSED READ //////////

struct decompress_info
{
    klzw_readfunc readfunc;
    intptr_t f;
    int32_t kgoal;
};

static int decompress_part(struct decompress_info * x)
{
    intptr_t const f = x->f;
    auto readfunc = x->readfunc;

    // Read compressed length first.
    int16_t leng;
    if (readfunc(f, &leng, sizeof(leng)) != sizeof(leng))
        return 1;
    leng = B_LITTLE16(leng);

    if (readfunc(f, lzwcompbuf, leng) != leng)
        return 1;

    x->kgoal = lzwuncompress(lzwcompbuf, leng, lzwrawbuf);

    return 0;
}

// Read from 'f' into 'buffer'.
int32_t klzw_read_compressed(void *buffer, int dasizeof, int count, intptr_t const f, klzw_readfunc readfunc)
{
    char *ptr = (char *)buffer;

    if (dasizeof > LZWSIZE)
    {
        count *= dasizeof;
        dasizeof = 1;
    }

    struct decompress_info x;
    x.readfunc = readfunc;
    x.f = f;

    if (decompress_part(&x))
        return -1;

    Bmemcpy(ptr, lzwrawbuf, (int32_t)dasizeof);

    for (int i=1, k=dasizeof; i<count; i++)
    {
        if (k >= x.kgoal)
        {
            k = decompress_part(&x);
            if (k) return -1;
        }

        int j = 0;

        if (dasizeof >= 4)
        {
            for (; j<dasizeof-4; j+=4)
            {
                ptr[j+dasizeof] = ((ptr[j]+lzwrawbuf[j+k])&255);
                ptr[j+1+dasizeof] = ((ptr[j+1]+lzwrawbuf[j+1+k])&255);
                ptr[j+2+dasizeof] = ((ptr[j+2]+lzwrawbuf[j+2+k])&255);
                ptr[j+3+dasizeof] = ((ptr[j+3]+lzwrawbuf[j+3+k])&255);
            }
        }

        for (; j<dasizeof; j++)
            ptr[j+dasizeof] = ((ptr[j]+lzwrawbuf[j+k])&255);

        k += dasizeof;
        ptr += dasizeof;
    }

    return count;
}


////////// COMPRESSED WRITE //////////

struct compress_info
{
    klzw_writefunc writefunc;
    intptr_t f;
    int32_t k;
};

static void compress_part(struct compress_info * x)
{
    const int16_t leng = (int16_t)lzwcompress(lzwrawbuf, x->k, lzwcompbuf);
    const int16_t swleng = B_LITTLE16(leng);

    intptr_t const f = x->f;
    auto writefunc = x->writefunc;

    x->k = 0;

    writefunc(f, &swleng, sizeof(swleng));
    writefunc(f, lzwcompbuf, leng);
}

// Write from 'buffer' to 'f'.
void klzw_write_compressed(const void * const buffer, int dasizeof, int count, intptr_t const f, klzw_writefunc writefunc)
{
    char const *ptr = (char const *)buffer;

    if (dasizeof > LZWSIZE)
    {
        count *= dasizeof;
        dasizeof = 1;
    }

    Bmemcpy(lzwrawbuf, ptr, (int32_t)dasizeof);

    struct compress_info x;
    x.writefunc = writefunc;
    x.f = f;

    if ((x.k = dasizeof) > LZWSIZE-dasizeof)
        compress_part(&x);

    for (int i=1; i<count; i++)
    {
        int j = 0;

        if (dasizeof >= 4)
        {
            for (; j<dasizeof-4; j+=4)
            {
                lzwrawbuf[j+x.k] = ((ptr[j+dasizeof]-ptr[j])&255);
                lzwrawbuf[j+1+x.k] = ((ptr[j+1+dasizeof]-ptr[j+1])&255);
                lzwrawbuf[j+2+x.k] = ((ptr[j+2+dasizeof]-ptr[j+2])&255);
                lzwrawbuf[j+3+x.k] = ((ptr[j+3+dasizeof]-ptr[j+3])&255);
            }
        }

        for (; j<dasizeof; j++)
            lzwrawbuf[j+x.k] = ((ptr[j+dasizeof]-ptr[j])&255);

        if ((x.k += dasizeof) > LZWSIZE-dasizeof)
            compress_part(&x);

        ptr += dasizeof;
    }

    if (x.k > 0)
        compress_part(&x);
}
