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
