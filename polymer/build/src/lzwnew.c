//--------------------------------------------------------------------------------------------------
#if defined(__POWERPC__)
static unsigned int LSWAPIB(unsigned int a) { return(((a>>8)&0xff00)+((a&0xff00)<<8)+(a<<24)+(a>>24)); }
static unsigned short SSWAPIB(unsigned short a) { return((a>>8)+(a<<8)); }
#else
#define LSWAPIB(a) (a)
#define SSWAPIB(a) (a)
#endif

#define USENEW 1
int lzwcompress(unsigned char *ucompbuf, int ucompleng, unsigned char *compbuf)
{
    int i, j, numnodes, *lptr, bitcnt, nbits, oneupnbits, hmask, *child;
    int *sibly;
#if USENEW
    int *sibry;
#endif
    unsigned char *nodev, *cptr, *eptr;

    nodev = (unsigned char *)malloc((ucompleng+256)*sizeof(char)); if (!nodev) return(0);
    child = (int *)malloc((ucompleng+256)*sizeof(int)); if (!child) { free(nodev); return(0); }
    sibly = (int *)malloc((ucompleng+256)*sizeof(int)); if (!sibly) { free(child); free(nodev); return(0); }
#if USENEW
    sibry = (int *)malloc((ucompleng+256)*sizeof(int)); if (!sibry) { free(sibly); free(child); free(nodev); return(0); }
#endif

    for (i=255;i>=0;i--) { nodev[i] = i; child[i] = -1; }
    memset(compbuf,0,ucompleng+15);

    cptr = ucompbuf; eptr = &ucompbuf[ucompleng];

    numnodes = 256; bitcnt = (4<<3); nbits = 8; oneupnbits = (1<<8); hmask = ((oneupnbits>>1)-1);
    do
    {
        for (i=cptr[0];i>=0;i=j)
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
            for (;nodev[j]!=cptr[0];j=sibly[j])
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

        lptr = (int *)&compbuf[bitcnt>>3]; lptr[0] |= LSWAPIB(i<<(bitcnt&7));
        bitcnt += nbits; if ((i&hmask) > ((numnodes-1)&hmask)) bitcnt--;

        numnodes++; if (numnodes > oneupnbits) { nbits++; oneupnbits <<= 1; hmask = ((oneupnbits>>1)-1); }
    }
    while ((cptr < eptr) && (bitcnt < (ucompleng<<3)));

#if USENEW
    free(sibry);
#endif
    free(sibly);
    free(child); free(nodev);

    lptr = (int *)compbuf;
    if (((bitcnt+7)>>3) < ucompleng) { lptr[0] = LSWAPIB(numnodes); return((bitcnt+7)>>3); }
    memcpy(compbuf,ucompbuf,ucompleng); return(ucompleng);
}

int lzwuncompress(unsigned char *compbuf, int compleng, unsigned char *ucompbuf, int ucompleng)
{
    int i, dat, leng, bitcnt, *lptr, numnodes, totnodes, nbits, oneupnbits, hmask, *prefix;
    unsigned char ch, *ucptr, *suffix;
    int ucomp = (int)ucompbuf;

    if (compleng >= ucompleng) { memcpy(ucompbuf,compbuf,ucompleng); return ucompleng; }

    totnodes = LSWAPIB(((int *)compbuf)[0]); if (totnodes <= 0 || totnodes >= ucompleng+256) return 0;

    prefix = (int *)malloc(totnodes*sizeof(int)); if (!prefix) return 0;
    suffix = (unsigned char *)malloc(totnodes*sizeof(char)); if (!suffix) { free(prefix); return 0; }

    numnodes = 256; bitcnt = (4<<3); nbits = 8; oneupnbits = (1<<8); hmask = ((oneupnbits>>1)-1);
    do
    {
        lptr = (int *)&compbuf[bitcnt>>3]; dat = ((LSWAPIB(lptr[0])>>(bitcnt&7))&(oneupnbits-1));
        bitcnt += nbits; if ((dat&hmask) > ((numnodes-1)&hmask)) { dat &= hmask; bitcnt--; }

        prefix[numnodes] = dat;

        ucompbuf++;
        for (leng=0;dat>=256;dat=prefix[dat])
        {
            if ((int)ucompbuf+leng-ucomp > ucompleng) goto bail;
            ucompbuf[leng++] = suffix[dat];
        }

        ucptr = &ucompbuf[leng-1];
        for (i=(leng>>1)-1;i>=0;i--) { ch = ucompbuf[i]; ucompbuf[i] = ucptr[-i]; ucptr[-i] = ch; }
        ucompbuf[-1] = dat; ucompbuf += leng;

        suffix[numnodes-1] = suffix[numnodes] = dat;

        numnodes++; if (numnodes > oneupnbits) { nbits++; oneupnbits <<= 1; hmask = ((oneupnbits>>1)-1); }
    }
    while (numnodes < totnodes);

bail:
    free(suffix); free(prefix);

    return (int)ucompbuf-ucomp;
}
//--------------------------------------------------------------------------------------------------
