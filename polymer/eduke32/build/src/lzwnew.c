//--------------------------------------------------------------------------------------------------
#include "compat.h"
#if defined(__POWERPC__) || defined(GEKKO)
static uint32_t LSWAPIB(uint32_t a) { return(((a>>8)&0xff00)+((a&0xff00)<<8)+(a<<24)+(a>>24)); }
static uint16_t SSWAPIB(uint16_t a) { return((a>>8)+(a<<8)); }
#else
#define LSWAPIB(a) (a)
#define SSWAPIB(a) (a)
#endif

#define USENEW 1
int32_t lzwcompress(char *ucompbuf, int32_t ucompleng, char *compbuf)
{
    int32_t i, j, numnodes, *lptr, bitcnt, nbits, oneupnbits, hmask, *child;
    int32_t *sibly;
#if USENEW
    int32_t *sibry;
#endif
    char *nodev, *cptr, *eptr;

    nodev = (char *)Bmalloc((ucompleng+256)*sizeof(uint8_t)); if (!nodev) return(0);
    child = (int32_t *)Bmalloc((ucompleng+256)*sizeof(int32_t)); if (!child) { Bfree(nodev); return(0); }
    sibly = (int32_t *)Bmalloc((ucompleng+256)*sizeof(int32_t)); if (!sibly) { Bfree(child); Bfree(nodev); return(0); }
#if USENEW
    sibry = (int32_t *)Bmalloc((ucompleng+256)*sizeof(int32_t)); if (!sibry) { Bfree(sibly); Bfree(child); Bfree(nodev); return(0); }
#endif

    for (i=255; i>=0; i--) { nodev[i] = i; child[i] = -1; }
    memset(compbuf,0,ucompleng+15);

    cptr = ucompbuf; eptr = &ucompbuf[ucompleng];

    numnodes = 256; bitcnt = (4<<3); nbits = 8; oneupnbits = (1<<8); hmask = ((oneupnbits>>1)-1);
    do
    {
        for (i=cptr[0]; i>=0; i=j)
        {
            cptr++; if (cptr >= eptr) goto lzwcompbreak2b;
            j = child[i]; if (j < 0) { child[i] = numnodes; break; }
#if USENEW
            //This is about 2x faster when ucompbuf is more random, 5% slower when very compressible
            while (cptr[0] != nodev[j])
            {
                if (cptr[0] < nodev[j])
                    { if (sibly[j] < 0) { sibly[j] = numnodes; goto lzwcompbreak2a; } j = sibly[j]; }
                else { if (sibry[j] < 0) { sibry[j] = numnodes; goto lzwcompbreak2a; } j = sibry[j]; }
            }
#else
            for (; nodev[j]!=cptr[0]; j=sibly[j])
                if (sibly[j] < 0) { sibly[j] = numnodes; goto lzwcompbreak2a; }
#endif
        }
lzwcompbreak2a:
        nodev[numnodes] = cptr[0];
lzwcompbreak2b:
        child[numnodes] = sibly[numnodes] = -1;
#if USENEW
        sibry[numnodes] = -1;
#endif

        lptr = (int32_t *)&compbuf[bitcnt>>3]; lptr[0] |= LSWAPIB(i<<(bitcnt&7));
        bitcnt += nbits; if ((i&hmask) > ((numnodes-1)&hmask)) bitcnt--;

        numnodes++; if (numnodes > oneupnbits) { nbits++; oneupnbits <<= 1; hmask = ((oneupnbits>>1)-1); }
    }
    while ((cptr < eptr) && (bitcnt < (ucompleng<<3)));

#if USENEW
    Bfree(sibry);
#endif
    Bfree(sibly);
    Bfree(child); Bfree(nodev);

    lptr = (int32_t *)compbuf;
    if (((bitcnt+7)>>3) < ucompleng) { lptr[0] = LSWAPIB(numnodes); return((bitcnt+7)>>3); }
    Bmemcpy(compbuf,ucompbuf,ucompleng); return(ucompleng);
}

int32_t lzwuncompress(char *compbuf, int32_t compleng, char *ucompbuf, int32_t ucompleng)
{
    int32_t i, dat, leng, bitcnt, *lptr, numnodes, totnodes, nbits, oneupnbits, hmask, *prefix;
    char ch, *ucptr, *suffix;
    int32_t ucomp = (int32_t)ucompbuf;

    if (compleng >= ucompleng) { Bmemcpy(ucompbuf,compbuf,ucompleng); return ucompleng; }

    totnodes = LSWAPIB(((int32_t *)compbuf)[0]); if (totnodes <= 0 || totnodes >= ucompleng+256) return 0;

    prefix = (int32_t *)Bmalloc(totnodes*sizeof(int32_t)); if (!prefix) return 0;
    suffix = (char *)Bmalloc(totnodes*sizeof(uint8_t)); if (!suffix) { Bfree(prefix); return 0; }

    numnodes = 256; bitcnt = (4<<3); nbits = 8; oneupnbits = (1<<8); hmask = ((oneupnbits>>1)-1);
    do
    {
        lptr = (int32_t *)&compbuf[bitcnt>>3]; dat = ((LSWAPIB(lptr[0])>>(bitcnt&7))&(oneupnbits-1));
        bitcnt += nbits; if ((dat&hmask) > ((numnodes-1)&hmask)) { dat &= hmask; bitcnt--; }

        prefix[numnodes] = dat;

        ucompbuf++;
        for (leng=0; dat>=256; dat=prefix[dat])
        {
            if ((int32_t)ucompbuf+leng-ucomp > ucompleng) goto bail;
            ucompbuf[leng++] = suffix[dat];
        }

        ucptr = &ucompbuf[leng-1];
        for (i=(leng>>1)-1; i>=0; i--) { ch = ucompbuf[i]; ucompbuf[i] = ucptr[-i]; ucptr[-i] = ch; }
        ucompbuf[-1] = dat; ucompbuf += leng;

        suffix[numnodes-1] = suffix[numnodes] = dat;

        numnodes++; if (numnodes > oneupnbits) { nbits++; oneupnbits <<= 1; hmask = ((oneupnbits>>1)-1); }
    }
    while (numnodes < totnodes);

bail:
    Bfree(suffix); Bfree(prefix);

    return (int32_t)ucompbuf-ucomp;
}
//--------------------------------------------------------------------------------------------------
