/**************************************************************************************************
KPLIB.C: Ken's Picture LIBrary written by Ken Silverman
Copyright (c) 1998-2008 Ken Silverman
Ken Silverman's official web site: http://advsys.net/ken

Features of KPLIB.C:
    * Routines for reading files out of ZIP/GRP files. All ZIP/GRP functions start with "kz".
    * Multi-platform support: Dos/Windows/Linux/Mac/etc..
    * Compact code, all in a single source file. Yeah, bad design on my part... but makes life
          easier for everyone else - you simply add a single C file to your project, throw a few
          externs in there, add the function calls, and you're done!

Brief history:
1998?: Wrote KPEG, a JPEG viewer for DOS
2000: Wrote KPNG, a PNG viewer for DOS
2001: Combined KPEG & KPNG, ported to Visual C, and made it into a library called KPLIB.C
2002: Added support for TGA,GIF,CEL,ZIP
2003: Added support for BMP
05/18/2004: Added support for 8&24 bit PCX
12/09/2005: Added support for progressive JPEG
01/05/2006: Added support for DDS
07/28/2007: Added support for GRP (Build Engine archive)

I offer this code to the community for free use - all I ask is that my name be included in the
credits.

-Ken S.
**************************************************************************************************/

#include "compat.h"
#include "baselayer.h"
#include "kplib.h"
#include "pragmas.h"

#include "vfs.h"

#if !defined(_WIN32)
static FORCE_INLINE CONSTEXPR int32_t klrotl(int32_t i, int sh) { return (i >> (-sh)) | (i << sh); }
#else
# define klrotl(i, sh) _lrotl(i, sh)
# ifdef __clang__
#  include <emmintrin.h>
# else
#  include <intrin.h>
# endif
#endif

//use GCC-specific extension to force symbol name to be something in particular to override underscoring.
#if defined(__GNUC__) && defined(__i386__) && !defined(NOASM)
#define ASMNAME(x) asm(x)
#else
#define ASMNAME(x)
#endif

static intptr_t kp_frameplace;
static int32_t kp_bytesperline, kp_xres, kp_yres;

static CONSTEXPR const int32_t pow2mask[32] =
{
    0x00000000,0x00000001,0x00000003,0x00000007,
    0x0000000f,0x0000001f,0x0000003f,0x0000007f,
    0x000000ff,0x000001ff,0x000003ff,0x000007ff,
    0x00000fff,0x00001fff,0x00003fff,0x00007fff,
    0x0000ffff,0x0001ffff,0x0003ffff,0x0007ffff,
    0x000fffff,0x001fffff,0x003fffff,0x007fffff,
    0x00ffffff,0x01ffffff,0x03ffffff,0x07ffffff,
    0x0fffffff,0x1fffffff,0x3fffffff,0x7fffffff,
};
static CONSTEXPR const int32_t pow2long[32] =
{
    0x00000001,0x00000002,0x00000004,0x00000008,
    0x00000010,0x00000020,0x00000040,0x00000080,
    0x00000100,0x00000200,0x00000400,0x00000800,
    0x00001000,0x00002000,0x00004000,0x00008000,
    0x00010000,0x00020000,0x00040000,0x00080000,
    0x00100000,0x00200000,0x00400000,0x00800000,
    0x01000000,0x02000000,0x04000000,0x08000000,
    0x10000000,0x20000000,0x40000000,(int32_t)0x80000000,
};

//Hack for peekbits,getbits,suckbits (to prevent lots of duplicate code)
//   0: PNG: do 12-byte chunk_header removal hack
// !=0: ZIP: use 64K buffer (olinbuf)
static int32_t zipfilmode;
kzfilestate kzfs;

// GCC 4.6 LTO build fix
#ifdef USING_LTO
# define B_KPLIB_STATIC
#else
# define B_KPLIB_STATIC static
#endif

//Initialized tables (can't be in union)
//jpg:                png:
//   crmul      16384    abstab10    4096
//   cbmul      16384    hxbit        472
//   dct         4608    pow2mask     128*
//   colclip     4096
//   colclipup8  4096
//   colclipup16 4096
//   unzig        256
//   pow2mask     128*
//   dcflagor      64

B_KPLIB_STATIC int32_t ATTRIBUTE((used)) palcol[256] ASMNAME("palcol");
static int32_t paleng, bakcol, numhufblocks, zlibcompflags;
static int8_t kcoltype, filtype, bitdepth;

//============================ KPNGILIB begins ===============================

//07/31/2000: KPNG.C first ported to C from READPNG.BAS
//10/11/2000: KPNG.C split into 2 files: KPNG.C and PNGINLIB.C
//11/24/2000: Finished adding support for coltypes 4&6
//03/31/2001: Added support for Adam7-type interlaced images
//Currently, there is no support for:
//   * 16-bit color depth
//   * Some useless ancillary chunks, like: gAMA(gamma) & pHYs(aspect ratio)

//.PNG specific variables:
static int32_t bakr = 0x80, bakg = 0x80, bakb = 0x80; //this used to be public...
static int32_t gslidew = 0, gslider = 0, xm, xmn[4], xr0, xr1, xplc, yplc;
static intptr_t nfplace;
static int32_t clen[320], cclen[19], bitpos, filt, xsiz, ysiz;
int32_t xsizbpl, ixsiz, ixoff, iyoff, ixstp, iystp, intlac, nbpl;
B_KPLIB_STATIC int32_t ATTRIBUTE((used)) trnsrgb ASMNAME("trnsrgb");
static int32_t ccind[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
static int32_t hxbit[59][2], ibuf0[288], nbuf0[32], ibuf1[32], nbuf1[32];
static const uint8_t *filptr;
static uint8_t slidebuf[32768], opixbuf0[4], opixbuf1[4];
static uint8_t pnginited = 0;
B_KPLIB_STATIC uint8_t olinbuf[131072] ASMNAME("olinbuf"); //WARNING:max kp_xres is: 131072/bpp-1
B_KPLIB_STATIC int32_t ATTRIBUTE((used)) abstab10[1024] ASMNAME("abstab10");

//Variables to speed up dynamic Huffman decoding:
#define LOGQHUFSIZ0 9
#define LOGQHUFSIZ1 6
static int32_t qhufval0[1<<LOGQHUFSIZ0], qhufval1[1<<LOGQHUFSIZ1];
static uint8_t qhufbit0[1<<LOGQHUFSIZ0], qhufbit1[1<<LOGQHUFSIZ1];

#if defined(_MSC_VER) && !defined(NOASM)

static inline int32_t bitrev(int32_t b, int32_t c)
{
    _asm
    {
        mov edx, b
        mov ecx, c
        xor eax, eax
        beg: shr edx, 1
        adc eax, eax
        sub ecx, 1
        jnz short beg
    }
}

#elif defined(__GNUC__) && defined(__i386__) && !defined(NOASM)

static inline int32_t bitrev(int32_t b, int32_t c)
{
    int32_t a = 0;
    __asm__ __volatile__(
        "xorl %%eax, %%eax\n\t0:\n\tshrl $1, %%ebx\n\tadcl %%eax, %%eax\n\tsubl $1, %%ecx\n\tjnz 0b"
        : "+a"(a), "+b"(b), "+c"(c) : : "cc");
    return a;
}

#else

static inline int32_t bitrev(int32_t b, int32_t c)
{
    int32_t i, j;
    for (i=1,j=0,c=(1<<c); i<c; i+=i) { j += j; if (b&i) j++; }
    return j;
}

#endif

static uint8_t fakebuf[8];
static uint8_t const *nfilptr;
static int32_t nbitpos;
static void suckbitsnextblock()
{
    if (zipfilmode)
    {
        //NOTE: should only read bytes inside compsize, not 64K!!! :/
        B_BUF32(&olinbuf[0], B_UNBUF32(&olinbuf[sizeof(olinbuf)-4]));
        uint32_t n = min<uint32_t>(kzfs.compleng-kzfs.comptell, sizeof(olinbuf)-4);
        buildvfs_fread(&olinbuf[4], n, 1, kzfs.fil);
        kzfs.comptell += n;
        bitpos -= ((sizeof(olinbuf)-4)<<3);
        return;
    }

    if (nfilptr)
    {
        filptr = nfilptr; nfilptr = 0;
        bitpos -= ((nbitpos-4)<<3);
        return;
    }
    //if (n_from_suckbits < 4) will it crash?

    //|===|===|crc|lng|typ|===|===|
    //        \  fakebuf: /
    //          |===|===|
    //----x     O---x     O--------
    nbitpos = B_BIG32(B_UNBUF32(&filptr[8]));
    nfilptr = &filptr[nbitpos+12];
    B_BUF32(&fakebuf[0], B_UNBUF32(&filptr[0])); //Copy last dword of IDAT chunk
    if (B_UNBUF32(&filptr[12]) == B_LITTLE32(0x54414449)) //Copy 1st dword of next IDAT chunk
        B_BUF32(&fakebuf[4], B_UNBUF32(&filptr[16]));
    filptr = &fakebuf[4]; bitpos -= 32;
}

static inline int32_t peekbits(int32_t n) { return (B_LITTLE32(B_UNBUF32(&filptr[bitpos>>3]))>>(bitpos&7))&pow2mask[n]; }
static inline void suckbits(int32_t n) { bitpos += n; if (bitpos < 0) return; suckbitsnextblock(); }
static inline int32_t getbits(int32_t n) { int32_t i = peekbits(n); suckbits(n); return i; }

static int32_t hufgetsym(int32_t *hitab, const int32_t *hbmax)
{
    int32_t v, n;

    v = n = 0;
    do { v = (v<<1)+getbits(1)+hbmax[n]-hbmax[n+1]; n++; }
    while (v >= 0);
    return hitab[hbmax[n]+v];
}

//This did not result in a speed-up on P4-3.6Ghz (02/22/2005)
//static int32_t hufgetsym_skipb (int32_t *hitab, int32_t *hbmax, int32_t n, int32_t addit)
//{
//   int32_t v;
//
//   v = bitrev(getbits(n),n)+addit;
//   do { v = (v<<1)+getbits(1)+hbmax[n]-hbmax[n+1]; n++; } while (v >= 0);
//   return hitab[hbmax[n]+v];
//}

static void qhufgencode(const int32_t *hitab, const int32_t *hbmax, int32_t *qhval, uint8_t *qhbit, int32_t numbits)
{
    int32_t i, j, k, n, r;

    //r is the bit reverse of i. Ex: if: i = 1011100111, r = 1110011101
    i = r = 0;
    for (n=1; n<=numbits; n++)
        for (k=hbmax[n-1]; k<hbmax[n]; k++)
            for (j=i+pow2mask[numbits-n]; i<=j; i++)
            {
                r = bitrev(i,numbits);
                qhval[r] = hitab[k];
                qhbit[r] = (uint8_t)n;
            }
    for (j=pow2mask[numbits]; i<=j; i++)
    {
        r = bitrev(i,numbits);

        //k = 0;
        //for(n=0;n<numbits;n++)
        //   k = (k<<1) + ((r>>n)&1) + hbmax[n]-hbmax[n+1];
        //
        //n = numbits;
        //k = hbmax[n]-r;
        //
        //j = peekbits(LOGQHUFSIZ); i = qhufval[j]; j = qhufbit[j];
        //
        //i = j = 0;
        //do
        //{
        //   i = (i<<1)+getbits(1)+nbuf0[j]-nbuf0[j+1]; j++;
        //} while (i >= 0);
        //i = ibuf0[nbuf0[j]+i];
        //qhval[r] = k;

        qhbit[r] = 0; //n-32;
    }

    //   //hufgetsym_skipb related code:
    //for(k=n=0;n<numbits;n++) k = (k<<1)+hbmax[n]-hbmax[n+1];
    //return k;
}

//inbuf[inum] : Bit length of each symbol
//inum        : Number of indices
//hitab[inum] : Indices from size-ordered list to original symbol
//hbmax[0-31] : Highest index (+1) of n-bit symbol

static void hufgencode(const int32_t *inbuf, int32_t inum, int32_t *hitab, int32_t *hbmax)
{
    int32_t i, tbuf[31], *tbufptr, *hbmaxptr;

    Bmemset(tbuf, 0, sizeof(tbuf));
    for (i=inum-1; i>=0; i--) tbuf[inbuf[i]]++;
    tbuf[0] = hbmax[0] = 0; //Hack to remove symbols of length 0?
    for (i=0; i<28; i += 4)
    {
        tbufptr = &tbuf[i];
        hbmaxptr = &hbmax[i];

        *(hbmaxptr+1) = *hbmaxptr     + *tbufptr;
        *(hbmaxptr+2) = *(hbmaxptr+1) + *(tbufptr+1);
        *(hbmaxptr+3) = *(hbmaxptr+2) + *(tbufptr+2);
        *(hbmaxptr+4) = *(hbmaxptr+3) + *(tbufptr+3);
    }

    tbufptr = &tbuf[i];
    hbmaxptr = &hbmax[i];

    *(hbmaxptr+1) = *hbmaxptr     + *tbufptr;
    *(hbmaxptr+2) = *(hbmaxptr+1) + *(tbufptr+1);
    *(hbmaxptr+3) = *(hbmaxptr+2) + *(tbufptr+2);

    for (i=0; i<inum; i++) if (inbuf[i]) hitab[hbmax[inbuf[i]]++] = i;
}

static int32_t initpass()  //Interlaced images have 7 "passes", non-interlaced have 1
{
    int32_t i, j, k;

    do
    {
        i = (intlac<<2);
        ixoff = ((0x04020100>>i)&15);
        iyoff = ((0x00402010>>i)&15);
        if (((ixoff >= xsiz) || (iyoff >= ysiz)) && (intlac >= 2)) { i = -1; intlac--; }
    }
    while (i < 0);
    j = ((0x33221100>>i)&15); ixstp = (1<<j);
    k = ((0x33322110>>i)&15); iystp = (1<<k);

    //xsiz=12      0123456789ab
    //j=3,ixoff=0  0       1       ((12+(1<<3)-1 - 0)>>3) = 2
    //j=3,ixoff=4      2           ((12+(1<<3)-1 - 4)>>3) = 1
    //j=2,ixoff=2    3   4   5     ((12+(1<<2)-1 - 2)>>2) = 3
    //j=1,ixoff=1   6 7 8 9 a b    ((12+(1<<1)-1 - 1)>>1) = 6
    ixsiz = ((xsiz+ixstp-1-ixoff)>>j); //It's confusing! See the above example.
    nbpl = (kp_bytesperline<<k);

    //Initialize this to make filters fast:
    xsizbpl = ((0x04021301>>(kcoltype<<2))&15)*ixsiz;
    switch (bitdepth)
    {
    case 1: xsizbpl = ((xsizbpl+7)>>3); break;
    case 2: xsizbpl = ((xsizbpl+3)>>2); break;
    case 4: xsizbpl = ((xsizbpl+1)>>1); break;
    }

    Bmemset(olinbuf,0,(xsizbpl+1)*sizeof(olinbuf[0]));
    B_BUF32(&opixbuf0[0], 0);
    B_BUF32(&opixbuf1[0], 0);
    xplc = xsizbpl; yplc = iyoff; xm = 0; filt = -1;

    i = ixoff; i = (((-(i>=0))|(ixstp-1))&i);
    k = (((-(yplc>=0))|(iystp-1))&yplc);
    nfplace = k*kp_bytesperline + (i<<2) + kp_frameplace;

    //Precalculate x-clipping to screen borders (speeds up putbuf)
    //Equation: (0 <= xr <= ixsiz) && (0 <= xr*ixstp+globxoffs+ixoff <= kp_xres)
    xr0 = max((-ixoff+(1<<j)-1)>>j,0);
    xr1 = min((kp_xres-ixoff+(1<<j)-1)>>j,ixsiz);
    xr0 = ixsiz-xr0;
    xr1 = ixsiz-xr1;

    if (kcoltype == 4) { xr0 = xr0*2;   xr1 = xr1*2;   }
    else if (kcoltype == 2) { xr0 = xr0*3-2; xr1 = xr1*3-2; }
    else if (kcoltype == 6) { xr0 = xr0*4-2; xr1 = xr1*4-2; }
    else
    {
        switch (bitdepth)
        {
        case 1: xr0 += ((-ixsiz)&7)+7;
            xr1 += ((-ixsiz)&7)+7; break;
        case 2: xr0 = ((xr0+((-ixsiz)&3)+3)<<1);
            xr1 = ((xr1+((-ixsiz)&3)+3)<<1); break;
        case 4: xr0 = ((xr0+((-ixsiz)&1)+1)<<2);
            xr1 = ((xr1+((-ixsiz)&1)+1)<<2); break;
        }
    }
    ixstp <<= 2;
    return 0;
}


static inline int32_t Paeth686(int32_t const a, int32_t const b, int32_t c)
{
    int32_t const * const ptr = &abstab10[(c - a) - (b - 512)];
    int32_t const esi = *(ptr + b);
    int32_t edi = *(ptr + c);
    if (edi >= esi) edi = esi, c = b;
    return (edi < *(ptr + a)) ? c : a;
}

static inline void rgbhlineasm(int32_t x, int32_t xr1, intptr_t p, int32_t ixstp)
{
    if (!trnsrgb)
    {
        for (; x>xr1; p+=ixstp,x-=3) B_BUF32((void *) p, (B_UNBUF32(&olinbuf[x]))|B_LITTLE32(0xff000000));
        return;
    }
    for (; x>xr1; p+=ixstp,x-=3)
    {
        int32_t i = (B_UNBUF32(&olinbuf[x]))|B_LITTLE32(0xff000000);
        if (i == trnsrgb) i &= B_LITTLE32(0xffffff);
        B_BUF32((void *) p, i);
    }
}

static inline void pal8hlineasm(int32_t x, int32_t xr1, intptr_t p, int32_t ixstp)
{
    for (; x>xr1; p+=ixstp,x--) B_BUF32((void *) p, palcol[olinbuf[x]]);
}


//Autodetect filter
//    /f0: 0000000...
//    /f1: 1111111...
//    /f2: 2222222...
//    /f3: 1333333...
//    /f3: 3333333...
//    /f4: 4444444...
//    /f5: 0142321...
static int32_t filter1st, filterest;
static void putbuf(const uint8_t *buf, int32_t leng)
{
    int32_t i;
    intptr_t p;

    if (filt < 0)
    {
        if (leng <= 0) return;
        filt = buf[0];
        if (filter1st < 0) filter1st = filt; else filterest |= (1<<filt);
        i = 1;
    }
    else i = 0;

    while (i < leng)
    {
        int32_t x = i+xplc; if (x > leng) x = leng;
        switch (filt)
        {
        case 0:
                while (i < x) { olinbuf[xplc--] = buf[i++]; }
            break;
        case 1:
                while (i < x)
                {
                    olinbuf[xplc--] = (uint8_t)(opixbuf1[xm] += buf[i++]);
                    xm = xmn[xm];
                }
            break;
        case 2:
                while (i < x) { olinbuf[xplc--] += (uint8_t)buf[i++]; }
            break;
        case 3:
                while (i < x)
                {
                    opixbuf1[xm] = olinbuf[xplc] = (uint8_t)(((opixbuf1[xm]+olinbuf[xplc])>>1)+buf[i++]);
                    xm = xmn[xm]; xplc--;
                }
            break;
        case 4:
                while (i < x)
                {
                    opixbuf1[xm] = (uint8_t)(Paeth686(opixbuf1[xm],olinbuf[xplc],opixbuf0[xm])+buf[i++]);
                    opixbuf0[xm] = olinbuf[xplc];
                    olinbuf[xplc--] = opixbuf1[xm];
                    xm = xmn[xm];
                }
            break;
        }

        if (xplc > 0) return;

        //Draw line!
        if ((uint32_t)yplc < (uint32_t)kp_yres)
        {
            x = xr0; p = nfplace;
            switch (kcoltype)
            {
            case 2: rgbhlineasm(x,xr1,p,ixstp); break;
            case 4:
                    for (; x>xr1; p+=ixstp,x-=2)
                        B_BUF32((void *) p, (palcol[olinbuf[x]]&B_LITTLE32(0xffffff))|B_BIG32((int32_t)olinbuf[x-1]));
                break;
            case 6:
                    for (; x>xr1; p+=ixstp,x-=4)
                    {
                        *(char *)(p) = olinbuf[x  ];   //B
                        *(char *)(p+1) = olinbuf[x+1]; //G
                        *(char *)(p+2) = olinbuf[x+2]; //R
                        *(char *)(p+3) = olinbuf[x-1]; //A
                    }
                break;
            default:
                    switch (bitdepth)
                    {
                    case 1: for (; x>xr1; p+=ixstp,x--) B_BUF32((void *) p, palcol[olinbuf[x>>3]>>(x&7)]); break;
                    case 2: for (; x>xr1; p+=ixstp,x-=2) B_BUF32((void *) p, palcol[olinbuf[x>>3]>>(x&6)]); break;
                    case 4: for (; x>xr1; p+=ixstp,x-=4) B_BUF32((void *) p, palcol[olinbuf[x>>3]>>(x&4)]); break;
                    case 8: pal8hlineasm(x,xr1,p,ixstp); break; //for(;x>xr1;p+=ixstp,x-- ) B_BUF32((void *) p, palcol[olinbuf[x]]); break;
                    }
                break;
            }
            nfplace += nbpl;
        }

        B_BUF32(&opixbuf0[0], 0);
        B_BUF32(&opixbuf1[0], 0);
        xplc = xsizbpl; yplc += iystp;
        if ((intlac) && (yplc >= ysiz)) { intlac--; initpass(); }
        if (i < leng)
        {
            filt = buf[i++];
            if (filter1st < 0) filter1st = filt; else filterest |= (1<<filt);
        }
        else filt = -1;
    }
}

static void initpngtables()
{
    int32_t i, j, k;

    //hxbit[0-58][0-1] is a combination of 4 different tables:
    //   1st parameter: [0-29] are distances, [30-58] are lengths
    //   2nd parameter: [0]: extra bits, [1]: base number

    j = 1; k = 0;
    for (i=0; i<30; i++)
    {
        hxbit[i][1] = j; j += (1<<k);
        hxbit[i][0] = k; k += ((i&1) && (i >= 2));
    }
    j = 3; k = 0;
    for (i=257; i<285; i++)
    {
        hxbit[i+30-257][1] = j; j += (1<<k);
        hxbit[i+30-257][0] = k; k += ((!(i&3)) && (i >= 264));
    }
    hxbit[285+30-257][1] = 258; hxbit[285+30-257][0] = 0;

    for (i=0; i<512; i++) abstab10[512+i] = abstab10[512-i] = i;
}


//============================= KPNGILIB ends ================================

//==================== External picture interface ends =======================

//Brute-force case-insensitive, slash-insensitive, * and ? wildcard matcher
//Given: string i and string j. string j can have wildcards
//Returns: 1:matches, 0:doesn't match



int32_t wildmatch(const char *match, const char *wild)
{
    do
    {
        int const match_deref = *match, wild_deref = *wild;

        if (match_deref && (toupperlookup[wild_deref] == toupperlookup[match_deref] || wild_deref == '?'))
        {
            wild++, match++;
            continue;
        }
        else if ((match_deref|wild_deref) == '\0')
            return 1;
        else if (wild_deref == '*')
        {
            do { wild++; } while (*wild == '*');
            int const wild_deref = *wild;
            do
            {
                if (wild_deref == '\0')
                    return 1;
                while (*match && toupperlookup[*match] != toupperlookup[wild_deref]) match++;
                if (*match && *(match+1) && toupperlookup[*(match+1)] != toupperlookup[*(wild+1)])
                {
                    match++;
                    continue;
                }
                break;
            }
            while (1);
            if (toupperlookup[*match] == toupperlookup[wild_deref])
                continue;
        }
        return 0;
    }
    while (1);
}

//===================== ZIP decompression code begins ========================

//format: (used by kzaddstack/kzopen to cache file name&start info)
//[char zipnam[?]\0]
//[next hashindex/-1][next index/-1][zipnam index][fileoffs][fileleng][iscomp][char filnam[?]\0]
//[next hashindex/-1][next index/-1][zipnam index][fileoffs][fileleng][iscomp][char filnam[?]\0]
//...
//[char zipnam[?]\0]
//[next hashindex/-1][next index/-1][zipnam index][fileoffs][fileleng][iscomp][char filnam[?]\0]
//[next hashindex/-1][next index/-1][zipnam index][fileoffs][fileleng][iscomp][char filnam[?]\0]
//...
#define KZHASHINITSIZE 8192
static char *kzhashbuf = 0;
static int32_t kzhashead[256], kzhashpos, kzlastfnam = -1, kzhashsiz, kzdirnamhead = -1;

static int32_t kzcheckhashsiz(int32_t siz)
{
    if (!kzhashbuf) //Initialize hash table on first call
    {
        Bmemset(kzhashead,-1,sizeof(kzhashead));
        kzhashbuf = (char *)Xmalloc(KZHASHINITSIZE); if (!kzhashbuf) return 0;
        kzhashpos = 0; kzlastfnam = -1; kzhashsiz = KZHASHINITSIZE; kzdirnamhead = -1;
    }
    if (kzhashpos+siz > kzhashsiz) //Make sure string fits in kzhashbuf
    {
        int32_t i = kzhashsiz; do { i <<= 1; }
        while (kzhashpos+siz > i);
        kzhashbuf = (char *)Xrealloc(kzhashbuf,i); if (!kzhashbuf) return 0;
        kzhashsiz = i;
    }
    return 1;
}

static int32_t kzcalchash(const char *st)
{
    int32_t i, hashind;

    for (i=0,hashind=0; st[i]; i++)
        hashind = toupperlookup[st[i]]-((hashind<<1)+hashind);

    return hashind%ARRAY_SIZE(kzhashead);
}

static int32_t kzcheckhash(const char *filnam, char **zipnam, int32_t *fileoffs, int32_t *fileleng, char *iscomp)
{
    int32_t i;

    if (!kzhashbuf) return 0;
    if (filnam[0] == '|') filnam++;
    for (i=kzhashead[kzcalchash(filnam)]; i>=0; i=(B_UNBUF32(&kzhashbuf[i])))
        if (!filnamcmp(filnam,&kzhashbuf[i+21]))
        {
            (*zipnam) = &kzhashbuf[B_UNBUF32(&kzhashbuf[i+8])];
            (*fileoffs) = B_UNBUF32(&kzhashbuf[i+12]);
            (*fileleng) = B_UNBUF32(&kzhashbuf[i+16]);
            (*iscomp) = kzhashbuf[i+20];
            return 1;
        }
    return 0;
}

void kzuninit()
{
    DO_FREE_AND_NULL(kzhashbuf);
    kzhashpos = kzhashsiz = 0; kzdirnamhead = -1;
}

//If file found, loads internal directory from ZIP/GRP into memory (hash) to allow faster access later
//If file not found, assumes it's a directory and adds it to an internal list
int32_t kzaddstack(const char *filnam)
{
    buildvfs_FILE fil;
    int32_t i, j, k, leng, hashind, zipnamoffs, numfiles;
    char tempbuf[260+46];

    fil = buildvfs_fopen_read(filnam);
    if (!fil) //if file not found, assume it's a directory
    {
        //Add directory name to internal list (using kzhashbuf for convenience of dynamic allocation)
        i = strlen(filnam)+5; if (!kzcheckhashsiz(i)) return -1;
        B_BUF32(&kzhashbuf[kzhashpos], kzdirnamhead); kzdirnamhead = kzhashpos;
        strcpy(&kzhashbuf[kzhashpos+4],filnam);
        kzhashpos += i;

        return -1;
    }

    //Write ZIP/GRP filename to hash
    i = strlen(filnam)+1; if (!kzcheckhashsiz(i)) { buildvfs_fclose(fil); return -1; }
    strcpy(&kzhashbuf[kzhashpos],filnam);
    zipnamoffs = kzhashpos; kzhashpos += i;

    buildvfs_fread(&i,4,1,fil);
    if (i == (int32_t)B_LITTLE32(0x04034b50)) //'PK\3\4' is ZIP file id
    {
        buildvfs_fseek_abs(fil,buildvfs_flength(fil)-22);
        buildvfs_fread(tempbuf,22,1,fil);
        if (B_UNBUF32(&tempbuf[0]) == B_LITTLE32(0x06054b50)) //Fast way of finding dir info
        {
            numfiles = B_LITTLE16(B_UNBUF16(&tempbuf[10]));
            buildvfs_fseek_abs(fil,B_LITTLE32(B_UNBUF32(&tempbuf[16])));
        }
        else //Slow way of finding dir info (used when ZIP has junk at end)
        {
            buildvfs_fseek_abs(fil,0); numfiles = 0;
            while (1)
            {
                if (!buildvfs_fread(&j,4,1,fil)) { numfiles = -1; break; }
                if (j == (int32_t)B_LITTLE32(0x02014b50)) break; //Found central file header :)
                if (j != (int32_t)B_LITTLE32(0x04034b50)) { numfiles = -1; break; }
                buildvfs_fread(tempbuf,26,1,fil);
                buildvfs_fseek_rel(fil,B_LITTLE32(B_UNBUF32(&tempbuf[14])) + B_LITTLE16(B_UNBUF16(&tempbuf[24])) + B_LITTLE16(B_UNBUF16(&tempbuf[22])));
                numfiles++;
            }
            if (numfiles < 0) { buildvfs_fclose(fil); return -1; }
            buildvfs_fseek_rel(fil,-4);
        }
        for (i=0; i<numfiles; i++)
        {
            buildvfs_fread(tempbuf,46,1,fil);
            if (B_UNBUF32(&tempbuf[0]) != B_LITTLE32(0x02014b50)) { buildvfs_fclose(fil); return 0; }

            j = B_LITTLE16(B_UNBUF16(&tempbuf[28])); //filename length
            buildvfs_fread(&tempbuf[46],j,1,fil);
            tempbuf[j+46] = 0;

            //Write information into hash
            j = strlen(&tempbuf[46])+22; if (!kzcheckhashsiz(j)) { buildvfs_fclose(fil); return -1; }
            hashind = kzcalchash(&tempbuf[46]);
            B_BUF32(&kzhashbuf[kzhashpos], kzhashead[hashind]);
            B_BUF32(&kzhashbuf[kzhashpos+4], kzlastfnam);
            B_BUF32(&kzhashbuf[kzhashpos+8], zipnamoffs);
            B_BUF32(&kzhashbuf[kzhashpos+12], B_LITTLE32(B_UNBUF32(&tempbuf[42]))); //fileoffs
            B_BUF32(&kzhashbuf[kzhashpos+16], 0); //fileleng not used for ZIPs (reserve space for simplicity)
            kzhashbuf[kzhashpos+20] = 1; //iscomp
            strcpy(&kzhashbuf[kzhashpos+21],&tempbuf[46]);
            kzhashead[hashind] = kzhashpos; kzlastfnam = kzhashpos; kzhashpos += j;

            j  = B_LITTLE16(B_UNBUF16(&tempbuf[30])); //extra field length
            j += B_LITTLE16(B_UNBUF16(&tempbuf[32])); //file comment length
            buildvfs_fseek_rel(fil,j);
        }
    }
    else if (i == (int32_t)B_LITTLE32(0x536e654b)) //'KenS' is GRP file id
    {
        buildvfs_fread(tempbuf,12,1,fil);
        if ((B_UNBUF32(&tempbuf[0]) != B_LITTLE32(0x65766c69)) || //'ilve'
                (B_UNBUF32(&tempbuf[4]) != B_LITTLE32(0x6e616d72)))   //'rman'
            { buildvfs_fclose(fil); return 0; }
        numfiles = B_LITTLE32(B_UNBUF32(&tempbuf[8])); k = ((numfiles+1)<<4);
        for (i=0; i<numfiles; i++,k+=leng)
        {
            buildvfs_fread(tempbuf,16,1,fil);
            leng = B_LITTLE32(B_UNBUF32(&tempbuf[12])); //File length
            tempbuf[12] = 0;

            //Write information into hash
            j = strlen(tempbuf)+22; if (!kzcheckhashsiz(j)) { buildvfs_fclose(fil); return -1; }
            hashind = kzcalchash(tempbuf);
            B_BUF32(&kzhashbuf[kzhashpos], kzhashead[hashind]);
            B_BUF32(&kzhashbuf[kzhashpos+4], kzlastfnam);
            B_BUF32(&kzhashbuf[kzhashpos+8], zipnamoffs);
            B_BUF32(&kzhashbuf[kzhashpos+12], k); //fileoffs
            B_BUF32(&kzhashbuf[kzhashpos+16], leng); //fileleng
            kzhashbuf[kzhashpos+20] = 0; //iscomp
            strcpy(&kzhashbuf[kzhashpos+21],tempbuf);
            kzhashead[hashind] = kzhashpos; kzlastfnam = kzhashpos; kzhashpos += j;
        }
    }
    buildvfs_fclose(fil);
    return 0;
}

//this allows the use of kplib.c with a file that is already open
void kzsetfil(buildvfs_FILE fil)
{
    kzfs.fil = fil;
    kzfs.comptyp = 0;
    kzfs.seek0 = 0;
    kzfs.leng = buildvfs_flength(fil);
    kzfs.pos = 0;
    kzfs.i = 0;
}

intptr_t kzopen(const char *filnam)
{
    buildvfs_FILE fil{};
    int32_t i, fileoffs, fileleng;
    char tempbuf[46+260], *zipnam, iscomp;

    //kzfs.fil = 0;
    if (filnam[0] != '|') //Search standalone file first
    {
        kzfs.fil = buildvfs_fopen_read(filnam);
        if (kzfs.fil)
        {
            kzfs.comptyp = 0;
            kzfs.seek0 = 0;
            kzfs.leng = buildvfs_flength(fil);
            kzfs.pos = 0;
            kzfs.i = 0;
            return (intptr_t)kzfs.fil;
        }
    }
    if (kzcheckhash(filnam,&zipnam,&fileoffs,&fileleng,&iscomp)) //Then check mounted ZIP/GRP files
    {
        fil = buildvfs_fopen_read(zipnam); if (!fil) return 0;
        buildvfs_fseek_abs(fil,fileoffs);
        if (!iscomp) //Must be from GRP file
        {
            kzfs.fil = fil;
            kzfs.comptyp = 0;
            kzfs.seek0 = fileoffs;
            kzfs.leng = fileleng;
            kzfs.pos = 0;
            kzfs.i = 0;
            return (intptr_t)kzfs.fil;
        }
        else
        {
            buildvfs_fread(tempbuf,30,1,fil);
            if (B_UNBUF32(&tempbuf[0]) != B_LITTLE32(0x04034b50)) { buildvfs_fclose(fil); return 0; }
            buildvfs_fseek_rel(fil,B_LITTLE16(B_UNBUF16(&tempbuf[26]))+B_LITTLE16(B_UNBUF16(&tempbuf[28])));

            kzfs.fil = fil;
            kzfs.comptyp = B_LITTLE16(B_UNBUF16(&tempbuf[8]));
            kzfs.seek0 = buildvfs_ftell(fil);
            kzfs.leng = B_LITTLE32(B_UNBUF32(&tempbuf[22]));
            kzfs.pos = 0;
            switch (kzfs.comptyp) //Compression method
            {
            case 0: kzfs.i = 0; return (intptr_t)kzfs.fil;
            case 8:
                    if (!pnginited) { pnginited = 1; initpngtables(); }
                kzfs.comptell = 0;
                kzfs.compleng = (int32_t)B_LITTLE32(B_UNBUF32(&tempbuf[18]));

                //WARNING: No file in ZIP can be > 2GB-32K bytes
                gslidew = 0x7fffffff; //Force reload at beginning

                return (intptr_t)kzfs.fil;
            default: buildvfs_fclose(kzfs.fil); kzfs.fil = 0; return 0;
            }
        }
    }

    //Finally, check mounted dirs

    int const namlen = strlen(filnam);

    for (i=kzdirnamhead; i>=0; i=B_UNBUF32(&kzhashbuf[i]))
    {
        strcpy(tempbuf,&kzhashbuf[i+4]);
        uint32_t const j = strlen(tempbuf);
        if (namlen+1+j >= sizeof(tempbuf)) continue; //don't allow int32_t filenames to buffer overrun
        if ((j) && (tempbuf[j-1] != '/') && (tempbuf[j-1] != '\\') && (filnam[0] != '/') && (filnam[0] != '\\'))
#if defined(_WIN32)
            strcat(tempbuf,"\\");
#else
            strcat(tempbuf,"/");
#endif
        strcat(tempbuf,filnam);
        kzfs.fil = buildvfs_fopen_read(tempbuf);
        if (kzfs.fil)
        {
            kzfs.comptyp = 0;
            kzfs.seek0 = 0;
            kzfs.leng = buildvfs_flength(fil);
            kzfs.pos = 0;
            kzfs.i = 0;
            return (intptr_t)kzfs.fil;
        }
    }

    return 0;
}

#ifndef USE_PHYSFS
// --------------------------------------------------------------------------

#if defined(_WIN32)
static HANDLE hfind = INVALID_HANDLE_VALUE;
static WIN32_FIND_DATAA findata;
#else
#include <dirent.h>
#define MAX_PATH 260
static DIR *hfind = NULL;
static struct dirent *findata = NULL;
#endif

//File find state variables. Example sequence (read top->bot, left->right):
//   srchstat   srchzoff    srchdoff
//   0,1,2,3
//              500,200,-1
//           4              300
//   0,1,2,3,4              100
//   0,1,2,3,4              -1
static int32_t srchstat = -1, srchzoff = 0, srchdoff = -1, wildstpathleng;
static char wildst[MAX_PATH] = "", newildst[MAX_PATH] = "";

void kzfindfilestart(const char *st)
{
#if defined(_WIN32)
    if (hfind != INVALID_HANDLE_VALUE)
        { FindClose(hfind); hfind = INVALID_HANDLE_VALUE; }
#else
    if (hfind) { closedir(hfind); hfind = NULL; }
#endif
    strcpy(wildst,st); strcpy(newildst,st);
    srchstat = 0; srchzoff = kzlastfnam; srchdoff = kzdirnamhead;
}

int32_t kzfindfile(char *filnam)
{
    int32_t i;

    kzfindfile_beg:;
    filnam[0] = 0;
    if (srchstat == 0)
    {
        if (!newildst[0]) { srchstat = -1; return 0; }
        do
        {
            srchstat = 1;

            //Extract directory from wildcard string for pre-pending
            wildstpathleng = 0;
            for (i=0; newildst[i]; i++)
                if ((newildst[i] == '/') || (newildst[i] == '\\'))
                    wildstpathleng = i+1;

            Bmemcpy(filnam,newildst,wildstpathleng);

#if defined(_WIN32)
            hfind = FindFirstFileA(newildst,&findata);
            if (hfind == INVALID_HANDLE_VALUE)
                { if (!kzhashbuf) return 0; srchstat = 2; continue; }
            if (findata.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) continue;
            i = wildstpathleng;
            if (findata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
                if ((findata.cFileName[0] == '.') && (!findata.cFileName[1])) continue;
            strcpy(&filnam[i],findata.cFileName);
            if (findata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) strcat(&filnam[i],"\\");
#else
            if (!hfind)
            {
                char const *s = ".";
                if (wildstpathleng > 0)
                {
                    filnam[wildstpathleng] = 0;
                    s = filnam;
                }
                hfind = opendir(s);
                if (!hfind) { if (!kzhashbuf) return 0; srchstat = 2; continue; }
            }
            break;   // process srchstat == 1
#endif
            return 1;
        }
        while (0);
    }
    if (srchstat == 1)
    {
        while (1)
        {
            Bmemcpy(filnam,newildst,wildstpathleng);
#if defined(_WIN32)
            if (!FindNextFileA(hfind,&findata))
                { FindClose(hfind); hfind = INVALID_HANDLE_VALUE; if (!kzhashbuf) return 0; srchstat = 2; break; }
            if (findata.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) continue;
            i = wildstpathleng;
            if (findata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
                if ((findata.cFileName[0] == '.') && (!findata.cFileName[1])) continue;
            strcpy(&filnam[i],findata.cFileName);
            if (findata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) strcat(&filnam[i],"\\");
#else
            if ((findata = readdir(hfind)) == NULL)
                { closedir(hfind); hfind = NULL; if (!kzhashbuf) return 0; srchstat = 2; break; }
            i = wildstpathleng;
            if (findata->d_type == DT_DIR)
                { if (findata->d_name[0] == '.' && !findata->d_name[1]) continue; } //skip .
            else if ((findata->d_type == DT_REG) || (findata->d_type == DT_LNK))
                { if (findata->d_name[0] == '.') continue; } //skip hidden (dot) files
            else continue; //skip devices and fifos and such
            if (!wildmatch(findata->d_name,&newildst[wildstpathleng])) continue;
            strcpy(&filnam[i],findata->d_name);
            if (findata->d_type == DT_DIR) strcat(&filnam[i],"/");
#endif
            return 1;
        }
    }
    while (srchstat == 2)
    {
        if (srchzoff < 0) { srchstat = 3; break; }
        if (wildmatch(&kzhashbuf[srchzoff+21],newildst))
        {
            //strcpy(filnam,&kzhashbuf[srchzoff+21]);
            filnam[0] = '|'; strcpy(&filnam[1],&kzhashbuf[srchzoff+21]);
            srchzoff = B_UNBUF32(&kzhashbuf[srchzoff+4]);
            return 1;
        }
        srchzoff = B_UNBUF32(&kzhashbuf[srchzoff+4]);
    }
    while (srchstat == 3)
    {
        if (srchdoff < 0) { srchstat = -1; break; }
        strcpy(newildst,&kzhashbuf[srchdoff+4]);
        i = strlen(newildst);
        if ((i) && (newildst[i-1] != '/') && (newildst[i-1] != '\\') && (filnam[0] != '/') && (filnam[0] != '\\'))
#if defined(_WIN32)
            strcat(newildst,"\\");
#else
            strcat(newildst,"/");
#endif
        strcat(newildst,wildst);
        srchdoff = B_UNBUF32(&kzhashbuf[srchdoff]);
        srchstat = 0; goto kzfindfile_beg;
    }

    return 0;
}

//File searching code (supports inside ZIP files!) How to use this code:
//   char filnam[MAX_PATH];
//   kzfindfilestart("vxl/*.vxl");
//   while (kzfindfile(filnam)) puts(filnam);
//NOTES:
// * Directory names end with '\' or '/' (depending on system)
// * Files inside zip begin with '|'
#endif

// --------------------------------------------------------------------------

static char *gzbufptr;
static void putbuf4zip(const uint8_t *buf, int32_t uncomp0, int32_t uncomp1)
{
    int32_t i0, i1;
    //              uncomp0 ... uncomp1
    //  &gzbufptr[kzfs.pos] ... &gzbufptr[kzfs.endpos];
    i0 = max(uncomp0,kzfs.pos);
    i1 = min(uncomp1,kzfs.endpos);
    if (i0 < i1) Bmemcpy(&gzbufptr[i0],&buf[i0-uncomp0],i1-i0);
}

//returns number of bytes copied
int32_t kzread(void *buffer, int32_t leng)
{
    int32_t i, j, k, bfinal, btype, hlit, hdist;

    if ((!kzfs.fil) || (leng <= 0)) return 0;

    if (kzfs.comptyp == 0)
    {
        if (kzfs.pos != kzfs.i) //Seek only when position changes
            { buildvfs_fseek_abs(kzfs.fil,kzfs.seek0+kzfs.pos); kzfs.i = kzfs.pos; }
        i = min(kzfs.leng-kzfs.pos,leng);
        buildvfs_fread(buffer,i,1,kzfs.fil);
        kzfs.i += i; //kzfs.i is a local copy of buildvfs_ftell(kzfs.fil);
    }
    else if (kzfs.comptyp == 8)
    {
        zipfilmode = 1;

        //Initialize for putbuf4zip
        gzbufptr = (char *)buffer; gzbufptr = &gzbufptr[-kzfs.pos];
        kzfs.endpos = min(kzfs.pos+leng,kzfs.leng);
        if (kzfs.endpos == kzfs.pos) return 0; //Guard against reading 0 length

        if (kzfs.pos < gslidew-32768) // Must go back to start :(
        {
            if (kzfs.comptell) buildvfs_fseek_abs(kzfs.fil,kzfs.seek0);

            gslidew = 0; gslider = 16384;
            kzfs.jmpplc = 0;

            //Initialize for suckbits/peekbits/getbits
            kzfs.comptell = min<int32_t>(kzfs.compleng,sizeof(olinbuf));
            buildvfs_fread(&olinbuf[0],kzfs.comptell,1,kzfs.fil);
            //Make it re-load when there are < 32 bits left in FIFO
            bitpos = -(((int32_t)sizeof(olinbuf)-4)<<3);
            //Identity: filptr + (bitpos>>3) = &olinbuf[0]
            filptr = &olinbuf[-(bitpos>>3)];
        }
        else
        {
            i = max(gslidew-32768,0); j = gslider-16384;

            //HACK: Don't unzip anything until you have to...
            //   (keeps file pointer as low as possible)
            if (kzfs.endpos <= gslidew) j = kzfs.endpos;

            //write uncompoffs on slidebuf from: i to j
            if (!((i^j)&32768))
                putbuf4zip(&slidebuf[i&32767],i,j);
            else
            {
                putbuf4zip(&slidebuf[i&32767],i,j&~32767);
                putbuf4zip(slidebuf,j&~32767,j);
            }

            //HACK: Don't unzip anything until you have to...
            //   (keeps file pointer as low as possible)
            if (kzfs.endpos <= gslidew) goto retkzread;
        }

        switch (kzfs.jmpplc)
        {
        case 0: goto kzreadplc0;
        case 1: goto kzreadplc1;
        case 2: goto kzreadplc2;
        case 3: goto kzreadplc3;
        }
        kzreadplc0:;
        do
        {
            bfinal = getbits(1); btype = getbits(2);

#if 0
            //Display Huffman block offsets&lengths of input file - for debugging only!
            {
                static int32_t ouncomppos = 0, ocomppos = 0;
                if (kzfs.comptell == sizeof(olinbuf)) i = 0;
                else if (kzfs.comptell < kzfs.compleng) i = kzfs.comptell-(sizeof(olinbuf)-4);
                else i = kzfs.comptell-(kzfs.comptell%(sizeof(olinbuf)-4));
                i += ((int32_t)&filptr[bitpos>>3])-((int32_t)(&olinbuf[0]));
                i = (i<<3)+(bitpos&7)-3;
                if (gslidew) printf(" ULng:0x%08x CLng:0x%08x.%x",gslidew-ouncomppos,(i-ocomppos)>>3,((i-ocomppos)&7)<<1);
                printf("\ntype:%d, Uoff:0x%08x Coff:0x%08x.%x",btype,gslidew,i>>3,(i&7)<<1);
                if (bfinal)
                {
                    printf(" ULng:0x%08x CLng:0x%08x.%x",kzfs.leng-gslidew,((kzfs.compleng<<3)-i)>>3,(((kzfs.compleng<<3)-i)&7)<<1);
                    printf("\n        Uoff:0x%08x Coff:0x%08x.0",kzfs.leng,kzfs.compleng);
                    ouncomppos = ocomppos = 0;
                }
                else { ouncomppos = gslidew; ocomppos = i; }
            }
#endif

            if (btype == 0)
            {
                //Raw (uncompressed)
                suckbits((-bitpos)&7);  //Synchronize to start of next byte
                i = getbits(16); if ((getbits(16)^i) != 0xffff) return -1;
                for (; i; i--)
                {
                    if (gslidew >= gslider)
                    {
                        putbuf4zip(&slidebuf[(gslider-16384)&32767],gslider-16384,gslider); gslider += 16384;
                        if (gslider-16384 >= kzfs.endpos)
                        {
                            kzfs.jmpplc = 1; kzfs.i = i; kzfs.bfinal = bfinal;
                            goto retkzread;
                            kzreadplc1:;         i = kzfs.i; bfinal = kzfs.bfinal;
                        }
                    }
                    slidebuf[(gslidew++)&32767] = (uint8_t)getbits(8);
                }
                continue;
            }
            if (btype == 3) continue;

            if (btype == 1) //Fixed Huffman
            {
                hlit = 288; hdist = 32; i = 0;
                for (; i<144; i++) clen[i] = 8; //Fixed bit sizes (literals)
                for (; i<256; i++) clen[i] = 9; //Fixed bit sizes (literals)
                for (; i<280; i++) clen[i] = 7; //Fixed bit sizes (EOI,lengths)
                for (; i<288; i++) clen[i] = 8; //Fixed bit sizes (lengths)
                for (; i<320; i++) clen[i] = 5; //Fixed bit sizes (distances)
            }
            else  //Dynamic Huffman
            {
                hlit = getbits(5)+257; hdist = getbits(5)+1; j = getbits(4)+4;
                for (i=0; i<j; i++) cclen[ccind[i]] = getbits(3);
                for (; i<19; i++) cclen[ccind[i]] = 0;
                hufgencode(cclen,19,ibuf0,nbuf0);

                j = 0; k = hlit+hdist;
                while (j < k)
                {
                    i = hufgetsym(ibuf0,nbuf0);
                    if (i < 16) { clen[j++] = i; continue; }
                    if (i == 16)
                        { for (i=getbits(2)+3; i; i--) { clen[j] = clen[j-1]; j++; } }
                    else
                    {
                        if (i == 17) i = getbits(3)+3; else i = getbits(7)+11;
                        for (; i; i--) clen[j++] = 0;
                    }
                }
            }

            hufgencode(clen,hlit,ibuf0,nbuf0);
            qhufgencode(ibuf0,nbuf0,qhufval0,qhufbit0,LOGQHUFSIZ0);

            hufgencode(&clen[hlit],hdist,ibuf1,nbuf1);
            qhufgencode(ibuf1,nbuf1,qhufval1,qhufbit1,LOGQHUFSIZ1);

            while (1)
            {
                if (gslidew >= gslider)
                {
                    putbuf4zip(&slidebuf[(gslider-16384)&32767],gslider-16384,gslider); gslider += 16384;
                    if (gslider-16384 >= kzfs.endpos)
                    {
                        kzfs.jmpplc = 2; kzfs.bfinal = bfinal; goto retkzread;
                        kzreadplc2:;      bfinal = kzfs.bfinal;
                    }
                }

                k = peekbits(LOGQHUFSIZ0);
                if (qhufbit0[k]) { i = qhufval0[k]; suckbits((int32_t)qhufbit0[k]); }
                else i = hufgetsym(ibuf0,nbuf0);

                if (i < 256) { slidebuf[(gslidew++)&32767] = (uint8_t)i; continue; }
                if (i == 256) break;
                i = getbits(hxbit[i+30-257][0]) + hxbit[i+30-257][1];

                k = peekbits(LOGQHUFSIZ1);
                if (qhufbit1[k]) { j = qhufval1[k]; suckbits((int32_t)qhufbit1[k]); }
                else j = hufgetsym(ibuf1,nbuf1);

                j = getbits(hxbit[j][0]) + hxbit[j][1];
                for (; i; i--,gslidew++) slidebuf[gslidew&32767] = slidebuf[(gslidew-j)&32767];
            }
        }
        while (!bfinal);

        gslider -= 16384;
        if (!((gslider^gslidew)&32768))
            putbuf4zip(&slidebuf[gslider&32767],gslider,gslidew);
        else
        {
            putbuf4zip(&slidebuf[gslider&32767],gslider,gslidew&~32767);
            putbuf4zip(slidebuf,gslidew&~32767,gslidew);
        }
        kzreadplc3:; kzfs.jmpplc = 3;
    }

    retkzread:;
    i = kzfs.pos;
    kzfs.pos += leng; if (kzfs.pos > kzfs.leng) kzfs.pos = kzfs.leng;
    return kzfs.pos-i;
}

//WARNING: kzseek(<-32768,SEEK_CUR); or:
//         kzseek(0,SEEK_END);       can make next kzread very slow!!!
int32_t kzseek(int32_t offset, int32_t whence)
{
    if (!kzfs.fil) return -1;
    switch (whence)
    {
    case SEEK_CUR: kzfs.pos += offset; break;
    case SEEK_END: kzfs.pos = kzfs.leng+offset; break;
    case SEEK_SET: default: kzfs.pos = offset;
    }
    if (kzfs.pos < 0) kzfs.pos = 0;
    if (kzfs.pos > kzfs.leng) kzfs.pos = kzfs.leng;
    return kzfs.pos;
}

//====================== ZIP decompression code ends =========================
