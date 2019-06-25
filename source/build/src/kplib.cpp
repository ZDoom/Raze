/**************************************************************************************************
KPLIB.C: Ken's Picture LIBrary written by Ken Silverman
Copyright (c) 1998-2008 Ken Silverman
Ken Silverman's official web site: http://advsys.net/ken

Features of KPLIB.C:
    * Routines for decoding JPG/PNG/GIF/PCX/TGA/BMP/DDS/CEL.
        See kpgetdim(), kprender(), and optional helper function: kpzload().
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

#if defined(_MSC_VER) && !defined(NOASM)

static inline int32_t Paeth686(int32_t a, int32_t b, int32_t c)
{
    _asm
    {
        push ebx
        push esi
        push edi
        mov eax, a
        mov ebx, b
        mov ecx, c
        mov edx, ecx
        sub edx, eax
        sub edx, ebx
        lea edx, abstab10[edx*4+2048]
        mov esi, [ebx*4+edx]
        mov edi, [ecx*4+edx]
        cmp edi, esi
        cmovge edi, esi
        cmovge ecx, ebx
        cmp edi, [eax*4+edx]
        cmovl eax, ecx
        pop edi
        pop esi
        pop ebx
    }
}

static inline void rgbhlineasm(int32_t c, int32_t d, int32_t t, int32_t b)
{
    _asm
    {
        push ebx
        push edi

        mov ecx, c
        mov edx, d
        mov edi, t
        mov ebx, b
        sub ecx, edx
        jle short endit
        add edx, offset olinbuf
        cmp dword ptr trnsrgb, 0
        jz short begit2
        begit:
        mov eax, dword ptr [ecx+edx]
        or eax, 0xff000000
        cmp eax, dword ptr trnsrgb
        jne short skipit
        and eax, 0xffffff
        skipit:
        sub ecx, 3
        mov [edi], eax
        lea edi, [edi+ebx]
        jnz short begit
        jmp short endit
        begit2:
        mov eax, dword ptr [ecx+edx]
        or eax, 0xff000000
        sub ecx, 3
        mov [edi], eax
        lea edi, [edi+ebx]
        jnz short begit2
        endit:
        pop edi
        pop ebx
    }
}

static inline void pal8hlineasm(int32_t c, int32_t d, int32_t t, int32_t b)
{
    _asm
    {
        mov ecx, c
        mov edx, d
        sub ecx, edx
        jle short endit

        push ebx
        push edi
        mov edi, t
        mov ebx, b
        add edx, offset olinbuf
        begit:movzx eax, byte ptr [ecx+edx]
        mov eax, dword ptr palcol[eax*4]
        sub ecx, 1
        mov [edi], eax
        lea edi, [edi+ebx]
        jnz short begit
        pop edi
        pop ebx
        endit:
    }
}

#elif defined(__GNUC__) && defined(__i386__) && !defined(NOASM)

static inline int32_t Paeth686(int32_t a, int32_t b, int32_t c)
{
    __asm__ __volatile__(
        "movl %%ecx, %%edx \n"
        "subl %%eax, %%edx \n"
        "subl %%ebx, %%edx \n"
        "leal (abstab10+2048)(,%%edx,4), %%edx \n"
        "movl (%%edx,%%ebx,4), %%esi \n"
        "movl (%%edx,%%ecx,4), %%edi \n"
        "cmpl %%esi, %%edi \n"
        "cmovgel %%esi, %%edi \n"
        "cmovgel %%ebx, %%ecx \n"
        "cmpl (%%edx,%%eax,4), %%edi \n"
        "cmovgel %%eax, %%ecx \n"
        : "+c"(c) : "a"(a), "b"(b) : "edx","esi","edi","memory","cc"
    );
    return c;
}

//Note: "cmove eax,?" may be faster than "jne ?:and eax,?" but who cares
static inline void rgbhlineasm(int32_t c, int32_t d, int32_t t, int32_t b)
{
    __asm__ __volatile__(
        "subl %%edx, %%ecx \n"
        "jle 3f \n"
        "addl $olinbuf, %%edx \n"
        "cmpl $0, trnsrgb(,1) \n"
        "jz 2f \n"
        "0: movl (%%ecx,%%edx,1), %%eax \n"
        "orl $0xff000000, %%eax \n"
        "cmpl trnsrgb(,1), %%eax \n"
        "jne 1f \n"
        "andl $0xffffff, %%eax \n"
        "1: subl $3, %%ecx \n"
        "movl %%eax, (%%edi) \n"
        "leal (%%edi,%%ebx,1), %%edi \n"
        "jnz 0b \n"
        "jmp 3f \n"
        "2: movl (%%ecx,%%edx,1), %%eax \n"
        "orl $0xff000000, %%eax \n"
        "subl $3, %%ecx \n"
        "movl %%eax, (%%edi) \n"
        "leal (%%edi,%%ebx,1), %%edi \n"
        "jnz 2b \n"
        "3: \n"
        : "+c"(c), "+d"(d), "+D"(t) : "b"(b) : "eax","memory","cc"
    );
}

static inline void pal8hlineasm(int32_t c, int32_t d, int32_t t, int32_t b)
{
    __asm__ __volatile__(
        "subl %%edx, %%ecx \n"
        "jle 1f \n"
        "addl $olinbuf, %%edx \n"
        "0: movzbl (%%ecx,%%edx,1), %%eax \n"
        "movl palcol(,%%eax,4), %%eax \n"
        "subl $1, %%ecx \n"
        "movl %%eax, (%%edi) \n"
        "leal (%%edi,%%ebx,1), %%edi \n"
        "jnz 0b \n"
        "1: \n"
        : "+c"(c), "+d"(d), "+D"(t) : "b"(b) : "eax","memory","cc"
    );
}

#else

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

#endif

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

static int32_t kpngrend(const char *kfilebuf, int32_t kfilength,
                        intptr_t dakpframeplace, int32_t dakpbytesperline, int32_t daxres, int32_t dayres)
{
    int32_t i, j, k, bfinal, btype, hlit, hdist, leng;
    int32_t slidew, slider;
    //int32_t qhuf0v, qhuf1v;

    UNREFERENCED_PARAMETER(kfilength);

    if (!pnginited) { pnginited = 1; initpngtables(); }

    if ((B_UNBUF32(&kfilebuf[0]) != B_LITTLE32(0x474e5089)) || (B_UNBUF32(&kfilebuf[4]) != B_LITTLE32(0x0a1a0a0d)))
        return -1; //"Invalid PNG file signature"
    filptr = (uint8_t const *)&kfilebuf[8];

    trnsrgb = 0; filter1st = -1; filterest = 0;

    while (1)
    {
        leng = B_BIG32(B_UNBUF32(&filptr[0])); i = B_UNBUF32(&filptr[4]);
        filptr = &filptr[8];

        if (i == (int32_t)B_LITTLE32(0x52444849)) //IHDR (must be first)
        {
            xsiz = B_BIG32(B_UNBUF32(&filptr[0])); if (xsiz <= 0) return -1;
            ysiz = B_BIG32(B_UNBUF32(&filptr[4])); if (ysiz <= 0) return -1;
            bitdepth = filptr[8]; if (!((1<<bitdepth)&0x116)) return -1; //"Bit depth not supported"
            kcoltype = filptr[9]; if (!((1<<kcoltype)&0x5d)) return -1; //"Color type not supported"
            if (filptr[10]) return -1; //"Only *flate is supported"
            if (filptr[11]) return -1; //"Filter not supported"
            if (filptr[12] >= 2) return -1; //"Unsupported interlace type"
            intlac = filptr[12]*7; //0=no interlace/1=Adam7 interlace

            //Save code by making grayscale look like a palette color scheme
            if ((!kcoltype) || (kcoltype == 4))
            {
                j = 0xff000000; k = (tabledivide32(255, ((1<<bitdepth)-1)))*0x10101;
                paleng = (1<<bitdepth);
                for (i=0; i<paleng; i++,j+=k) palcol[i] = B_LITTLE32(j);
            }
        }
        else if (i == (int32_t)B_LITTLE32(0x45544c50)) //PLTE (must be before IDAT)
        {
            paleng = leng/3;
            for (i=paleng-1; i>=0; i--) palcol[i] = B_LITTLE32((B_BIG32(B_UNBUF32(&filptr[i*3]))>>8)|0xff000000);
        }
        else if (i == (int32_t)B_LITTLE32(0x44474b62)) //bKGD (must be after PLTE and before IDAT)
        {
            switch (kcoltype)
            {
            case 0: case 4:
                        bakcol = (((int32_t)filptr[0]<<8)+(int32_t)filptr[1])*tabledivide32(255, ((1<<bitdepth)-1));
                bakcol = bakcol*0x10101+0xff000000; break;
            case 2: case 6:
                        if (bitdepth == 8)
                            { bakcol = (((int32_t)filptr[1])<<16)+(((int32_t)filptr[3])<<8)+((int32_t)filptr[5])+0xff000000; }
                        else
                        {
                            for (i=0,bakcol=0xff000000; i<3; i++)
                                bakcol += tabledivide32(((((int32_t)filptr[i<<1])<<8)+((int32_t)filptr[(i<<1)+1])), 257)<<(16-(i<<3));
                        }
                break;
            case 3:
                    bakcol = palcol[filptr[0]]; break;
            }
            bakr = ((bakcol>>16)&255);
            bakg = ((bakcol>>8)&255);
            bakb = (bakcol&255);
            bakcol = B_LITTLE32(bakcol);
        }
        else if (i == (int32_t)B_LITTLE32(0x534e5274)) //tRNS (must be after PLTE and before IDAT)
        {
            switch (kcoltype)
            {
            case 0:
                    if (bitdepth <= 8)
                        palcol[(int32_t)filptr[1]] &= B_LITTLE32(0xffffff);
                //else {} // /c0 /d16 not yet supported
                break;
            case 2:
                    if (bitdepth == 8)
                        { trnsrgb = B_LITTLE32((((int32_t)filptr[1])<<16)+(((int32_t)filptr[3])<<8)+((int32_t)filptr[5])+0xff000000); }
                //else {} //WARNING: PNG docs say: MUST compare all 48 bits :(
                break;
            case 3:
                    for (i=min(leng,paleng)-1; i>=0; i--)
                        palcol[i] &= B_LITTLE32((((int32_t)filptr[i])<<24)|0xffffff);
                break;
            default:;
                EDUKE32_UNREACHABLE_SECTION();
            }
        }
        else if (i == (int32_t)B_LITTLE32(0x54414449)) { break; }  //IDAT

        filptr = &filptr[leng+4]; //crc = B_BIG32(B_UNBUF32(&filptr[-4]));
    }

    //Initialize this for the getbits() function
    zipfilmode = 0;
    filptr = &filptr[leng-4]; bitpos = -((leng-4)<<3); nfilptr = 0;
    //if (leng < 4) will it crash?

    kp_frameplace = dakpframeplace;
    kp_bytesperline = dakpbytesperline;
    kp_xres = daxres;
    kp_yres = dayres;
    switch (kcoltype)
    {
    case 4: xmn[0] = 1; xmn[1] = 0; break;
    case 2: xmn[0] = 1; xmn[1] = 2; xmn[2] = 0; break;
    case 6: xmn[0] = 1; xmn[1] = 2; xmn[2] = 3; xmn[3] = 0; break;
    default: xmn[0] = 0; break;
    }
    switch (bitdepth)
    {
    case 1: for (i=2; i<256; i++) palcol[i] = palcol[i&1]; break;
    case 2: for (i=4; i<256; i++) palcol[i] = palcol[i&3]; break;
    case 4: for (i=16; i<256; i++) palcol[i] = palcol[i&15]; break;
    }

    //coltype: bitdepth:  format:
    //  0     1,2,4,8,16  I
    //  2           8,16  RGB
    //  3     1,2,4,8     P
    //  4           8,16  IA
    //  6           8,16  RGBA
    xsizbpl = ((0x04021301>>(kcoltype<<2))&15)*xsiz;
    switch (bitdepth)
    {
    case 1: xsizbpl = ((xsizbpl+7)>>3); break;
    case 2: xsizbpl = ((xsizbpl+3)>>2); break;
    case 4: xsizbpl = ((xsizbpl+1)>>1); break;
    }
    //Tests to see if xsiz > allocated space in olinbuf
    //Note: xsizbpl gets re-written inside initpass()
    if ((xsizbpl+1)*sizeof(olinbuf[0]) > sizeof(olinbuf)) return -1;

    initpass();

    slidew = 0; slider = 16384;
    zlibcompflags = getbits(16); //Actually 2 fields: 8:compmethflags, 8:addflagscheck
    do
    {
        bfinal = getbits(1); btype = getbits(2);
        if (btype == 0)
        {
            //Raw (uncompressed)
            suckbits((-bitpos)&7);  //Synchronize to start of next byte
            i = getbits(16); if ((getbits(16)^i) != 0xffff) return -1;
            for (; i; i--)
            {
                if (slidew >= slider)
                {
                    putbuf(&slidebuf[(slider-16384)&32767],16384); slider += 16384;
                    if ((yplc >= kp_yres) && (intlac < 2)) goto kpngrend_goodret;
                }
                slidebuf[(slidew++)&32767] = (uint8_t)getbits(8);
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
            numhufblocks++;
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
        //qhuf0v = //hufgetsym_skipb related code
        qhufgencode(ibuf0,nbuf0,qhufval0,qhufbit0,LOGQHUFSIZ0);

        hufgencode(&clen[hlit],hdist,ibuf1,nbuf1);
        //qhuf1v = //hufgetsym_skipb related code
        qhufgencode(ibuf1,nbuf1,qhufval1,qhufbit1,LOGQHUFSIZ1);

        while (1)
        {
            if (slidew >= slider)
            {
                putbuf(&slidebuf[(slider-16384)&32767],16384); slider += 16384;
                if ((yplc >= kp_yres) && (intlac < 2)) goto kpngrend_goodret;
            }

            k = peekbits(LOGQHUFSIZ0);
            if (qhufbit0[k]) { i = qhufval0[k]; suckbits((int32_t)qhufbit0[k]); }
            else i = hufgetsym(ibuf0,nbuf0);
            //else i = hufgetsym_skipb(ibuf0,nbuf0,LOGQHUFSIZ0,qhuf0v); //hufgetsym_skipb related code

            if (i < 256) { slidebuf[(slidew++)&32767] = (uint8_t)i; continue; }
            if (i == 256) break;
            i = getbits(hxbit[i+30-257][0]) + hxbit[i+30-257][1];

            k = peekbits(LOGQHUFSIZ1);
            if (qhufbit1[k]) { j = qhufval1[k]; suckbits((int32_t)qhufbit1[k]); }
            else j = hufgetsym(ibuf1,nbuf1);
            //else j = hufgetsym_skipb(ibuf1,nbuf1,LOGQHUFSIZ1,qhuf1v); //hufgetsym_skipb related code

            j = getbits(hxbit[j][0]) + hxbit[j][1];
            i += slidew; do { slidebuf[slidew&32767] = slidebuf[(slidew-j)&32767]; slidew++; }
            while (slidew < i);
        }
    }
    while (!bfinal);

    slider -= 16384;
    if (!((slider^slidew)&32768))
        putbuf(&slidebuf[slider&32767],slidew-slider);
    else
    {
        putbuf(&slidebuf[slider&32767],(-slider)&32767);
        putbuf(slidebuf,slidew&32767);
    }

kpngrend_goodret:
    if (!(filterest&~(1<<filter1st))) filtype = (int8_t)filter1st;
    else if ((filter1st == 1) && (!(filterest&~(1<<3)))) filtype = 3;
    else filtype = 5;
    if (kcoltype == 4) paleng = 0; //For /c4, palcol/paleng used as LUT for "*0x10101": alpha is invalid!
    return 0;
}

//============================= KPNGILIB ends ================================
//============================ KPEGILIB begins ===============================

//11/01/2000: This code was originally from KPEG.C
//   All non 32-bit color drawing was removed
//   "Motion" JPG code was removed
//   A lot of parameters were added to kpeg() for library usage
static int32_t kpeginited = 0;
static int32_t clipxdim, clipydim;

static int32_t hufmaxatbit[8][20], hufvalatbit[8][20], hufcnt[8];
static uint8_t hufnumatbit[8][20], huftable[8][256];
static int32_t hufquickval[8][1024], hufquickbits[8][1024], hufquickcnt[8];
static int32_t quantab[4][64], dct[12][64], lastdc[4], unzig[64], zigit[64]; //dct:10=MAX (says spec);+2 for hacks
static uint8_t gnumcomponents, dcflagor[64];
static int32_t gcompid[4], gcomphsamp[4], gcompvsamp[4], gcompquantab[4], gcomphsampshift[4], gcompvsampshift[4];
static int32_t lnumcomponents, lcompid[4], lcompdc[4], lcompac[4], lcomphsamp[4], lcompvsamp[4], lcompquantab[4];
static int32_t lcomphvsamp0, lcomphsampshift0, lcompvsampshift0;
static int32_t colclip[1024], colclipup8[1024], colclipup16[1024];
/*static uint8_t pow2char[8] = {1,2,4,8,16,32,64,128};*/

#if defined(_MSC_VER) && !defined(NOASM)

static inline int32_t mulshr24(int32_t a, int32_t d)
{
    _asm
    {
        mov eax, a
        imul d
        shrd eax, edx, 24
    }
}

static inline int32_t mulshr32(int32_t a, int32_t d)
{
    _asm
    {
        mov eax, a
        imul d
        mov eax, edx
    }
}

#elif defined(__GNUC__) && defined(__i386__) && !defined(NOASM)

#define mulshr24(a,d) \
    ({ int32_t __a=(a), __d=(d); \
        __asm__ __volatile__ ("imull %%edx; shrdl $24, %%edx, %%eax" \
        : "+a" (__a), "+d" (__d) : : "cc"); \
     __a; })

#define mulshr32(a,d) \
    ({ int32_t __a=(a), __d=(d); \
        __asm__ __volatile__ ("imull %%edx" \
        : "+a" (__a), "+d" (__d) : : "cc"); \
     __d; })

#else

static inline int32_t mulshr24(int32_t a, int32_t b)
{
    return (int32_t)((((int64_t)a)*((int64_t)b))>>24);
}

static inline int32_t mulshr32(int32_t a, int32_t b)
{
    return (int32_t)((((int64_t)a)*((int64_t)b))>>32);
}

#endif

static int32_t cosqr16[8] =    //cosqr16[i] = ((cos(PI*i/16)*sqrt(2))<<24);
{23726566,23270667,21920489,19727919,16777216,13181774,9079764,4628823};
static int32_t crmul[4096], cbmul[4096];

static void initkpeg()
{
    int32_t i, j, y;

    int32_t x = 0;  //Back & forth diagonal pattern (aligning bytes for best compression)
    for (i=0; i<16; i+=2)
    {
        for (y=8-1; y>=0; y--)
            if ((unsigned)(i-y) < (unsigned)8) unzig[x++] = (y<<3)+i-y;
        for (y=0; y<8; y++)
            if ((unsigned)(i+1-y) < (unsigned)8) unzig[x++] = (y<<3)+i+1-y;
    }
    for (i=64-1; i>=0; i--) zigit[unzig[i]] = i;
    for (i=64-1; i>=0; i--) dcflagor[i] = (uint8_t)(1<<(unzig[i]>>3));

    for (i=0; i<128; i++) colclip[i] = i+128;
    for (i=128; i<512; i++) colclip[i] = 255;
    for (i=512; i<896; i++) colclip[i] = 0;
    for (i=896; i<1024; i++) colclip[i] = i-896;
    for (i=0; i<1024; i++)
    {
        colclipup8[i] = (colclip[i]<<8);
        colclipup16[i] = (colclip[i]<<16)+0xff000000; //Hack: set alphas to 255
    }
#if B_BIG_ENDIAN == 1
    for (i=0; i<1024; i++)
    {
        colclip[i] = B_SWAP32(colclip[i]);
        colclipup8[i] = B_SWAP32(colclipup8[i]);
        colclipup16[i] = B_SWAP32(colclipup16[i]);
    }
#endif

    for (i=0; i<2048; i++)
    {
        j = i-1024;
        crmul[(i<<1)+0] = j*1470104; //1.402*1048576
        crmul[(i<<1)+1] = j*-748830; //-0.71414*1048576
        cbmul[(i<<1)+0] = j*-360857; //-0.34414*1048576
        cbmul[(i<<1)+1] = j*1858077; //1.772*1048576
    }

    Bmemset((void *)&dct[10][0],0,64*2*sizeof(dct[0][0]));
}

static void huffgetval(int32_t index, int32_t curbits, int32_t num, int32_t *daval, int32_t *dabits)
{
    int32_t b, v, pow2, *hmax;

    hmax = &hufmaxatbit[index][0];
    pow2 = pow2long[curbits-1];
    if (num&pow2) v = 1; else v = 0;
    for (b=1; b<=16; b++)
    {
        if (v < hmax[b])
        {
            *dabits = b;
            *daval = huftable[index][hufvalatbit[index][b]+v];
            return;
        }
        pow2 >>= 1; v <<= 1;
        if (num&pow2) v++;
    }
    *dabits = 16; *daval = 0;
}

static void invdct8x8(int32_t *dc, uint8_t dcflag)
{
#define SQRT2 23726566   //(sqrt(2))<<24
#define C182 31000253    //(cos(PI/8)*2)<<24
#define C18S22 43840978  //(cos(PI/8)*sqrt(2)*2)<<24
#define C38S22 18159528  //(cos(PI*3/8)*sqrt(2)*2)<<24
    int32_t *edc, t0, t1, t2, t3, t4, t5, t6, t7;

    edc = dc+64;
    do
    {
        if (dcflag&1) //pow2char[z])
        {
            t3 = dc[2] + dc[6];
            t2 = (mulshr32(dc[2]-dc[6],SQRT2<<6)<<2) - t3;
            t4 = dc[0] + dc[4]; t5 = dc[0] - dc[4];
            t0 = t4+t3; t3 = t4-t3; t1 = t5+t2; t2 = t5-t2;
            t4 = (mulshr32(dc[5]-dc[3]+dc[1]-dc[7],C182<<6)<<2);
            t7 = dc[1] + dc[7] + dc[5] + dc[3];
            t6 = (mulshr32(dc[3]-dc[5],C18S22<<5)<<3) + t4 - t7;
            t5 = (mulshr32(dc[1]+dc[7]-dc[5]-dc[3],SQRT2<<6)<<2) - t6;
            t4 = (mulshr32(dc[1]-dc[7],C38S22<<6)<<2) - t4 + t5;
            dc[0] = t0+t7; dc[7] = t0-t7; dc[1] = t1+t6; dc[6] = t1-t6;
            dc[2] = t2+t5; dc[5] = t2-t5; dc[4] = t3+t4; dc[3] = t3-t4;
        }
        dc += 8; dcflag >>= 1;
    }
    while (dc < edc);
    dc -= 32; edc -= 24;
    do
    {
        t3 = dc[2*8-32] + dc[6*8-32];
        t2 = (mulshr32(dc[2*8-32]-dc[6*8-32],SQRT2<<6)<<2) - t3;
        t4 = dc[0*8-32] + dc[4*8-32]; t5 = dc[0*8-32] - dc[4*8-32];
        t0 = t4+t3; t3 = t4-t3; t1 = t5+t2; t2 = t5-t2;
        t4 = (mulshr32(dc[5*8-32]-dc[3*8-32]+dc[1*8-32]-dc[7*8-32],C182<<6)<<2);
        t7 = dc[1*8-32] + dc[7*8-32] + dc[5*8-32] + dc[3*8-32];
        t6 = (mulshr32(dc[3*8-32]-dc[5*8-32],C18S22<<5)<<3) + t4 - t7;
        t5 = (mulshr32(dc[1*8-32]+dc[7*8-32]-dc[5*8-32]-dc[3*8-32],SQRT2<<6)<<2) - t6;
        t4 = (mulshr32(dc[1*8-32]-dc[7*8-32],C38S22<<6)<<2) - t4 + t5;
        dc[0*8-32] = t0+t7; dc[7*8-32] = t0-t7; dc[1*8-32] = t1+t6; dc[6*8-32] = t1-t6;
        dc[2*8-32] = t2+t5; dc[5*8-32] = t2-t5; dc[4*8-32] = t3+t4; dc[3*8-32] = t3-t4;
        dc++;
    }
    while (dc < edc);
}

static void yrbrend(int32_t x, int32_t y, int32_t *ldct)
{
    int32_t i, j, ox, oy, xx, yy, xxx, yyy, xxxend, yyyend, yv, cr = 0, cb = 0, *odc, *dc, *dc2;
    intptr_t p, pp;

    odc = ldct; dc2 = &ldct[10<<6];
    for (yy=0; yy<(lcompvsamp[0]<<3); yy+=8)
    {
        oy = y+yy; if ((unsigned)oy >= (unsigned)clipydim) { odc += (lcomphsamp[0]<<6); continue; }
        pp = oy*kp_bytesperline + ((x)<<2) + kp_frameplace;
        for (xx=0; xx<(lcomphsamp[0]<<3); xx+=8,odc+=64)
        {
            ox = x+xx; if ((unsigned)ox >= (unsigned)clipxdim) continue;
            p = pp+(xx<<2);
            dc = odc;
            if (lnumcomponents > 1) dc2 = &ldct[(lcomphvsamp0<<6)+((yy>>lcompvsampshift0)<<3)+(xx>>lcomphsampshift0)];
            xxxend = min(clipxdim-ox,8);
            yyyend = min(clipydim-oy,8);
            if ((lcomphsamp[0] == 1) && (xxxend == 8))
            {
                for (yyy=0; yyy<yyyend; yyy++)
                {
                    for (xxx=0; xxx<8; xxx++)
                    {
                        yv = dc[xxx];
                        cr = (dc2[xxx+64]>>(20-1))&~1;
                        cb = (dc2[xxx   ]>>(20-1))&~1;
                        ((int32_t *)p)[xxx] = colclipup16[(unsigned)(yv+crmul[cr+2048])>>22]+
                                              colclipup8[(unsigned)(yv+crmul[cr+2049]+cbmul[cb+2048])>>22]+
                                              colclip[(unsigned)(yv+cbmul[cb+2049])>>22];
                    }
                    p += kp_bytesperline;
                    dc += 8;
                    if (!((yyy+1)&(lcompvsamp[0]-1))) dc2 += 8;
                }
            }
            else if ((lcomphsamp[0] == 2) && (xxxend == 8))
            {
                for (yyy=0; yyy<yyyend; yyy++)
                {
                    for (xxx=0; xxx<8; xxx+=2)
                    {
                        yv = dc[xxx];
                        cr = (dc2[(xxx>>1)+64]>>(20-1))&~1;
                        cb = (dc2[(xxx>>1)]>>(20-1))&~1;
                        i = crmul[cr+2049]+cbmul[cb+2048];
                        cr = crmul[cr+2048];
                        cb = cbmul[cb+2049];
                        ((int32_t *)p)[xxx] = colclipup16[(unsigned)(yv+cr)>>22]+
                                              colclipup8[(unsigned)(yv+ i)>>22]+
                                              colclip[(unsigned)(yv+cb)>>22];
                        yv = dc[xxx+1];
                        ((int32_t *)p)[xxx+1] = colclipup16[(unsigned)(yv+cr)>>22]+
                                                colclipup8[(unsigned)(yv+ i)>>22]+
                                                colclip[(unsigned)(yv+cb)>>22];
                    }
                    p += kp_bytesperline;
                    dc += 8;
                    if (!((yyy+1)&(lcompvsamp[0]-1))) dc2 += 8;
                }
            }
            else
            {
                for (yyy=0; yyy<yyyend; yyy++)
                {
                    i = 0; j = 1;
                    for (xxx=0; xxx<xxxend; xxx++)
                    {
                        yv = dc[xxx];
                        j--;
                        if (!j)
                        {
                            j = lcomphsamp[0];
                            cr = (dc2[i+64]>>(20-1))&~1;
                            cb = (dc2[i   ]>>(20-1))&~1;
                            i++;
                        }
                        ((int32_t *)p)[xxx] = colclipup16[(unsigned)(yv+crmul[cr+2048])>>22]+
                                              colclipup8[(unsigned)(yv+crmul[cr+2049]+cbmul[cb+2048])>>22]+
                                              colclip[(unsigned)(yv+cbmul[cb+2049])>>22];
                    }
                    p += kp_bytesperline;
                    dc += 8;
                    if (!((yyy+1)&(lcompvsamp[0]-1))) dc2 += 8;
                }
            }
        }
    }
}
void (*kplib_yrbrend_func)(int32_t,int32_t,int32_t *) = yrbrend;

#define KPEG_GETBITS(curbits, minbits, num, kfileptr, kfileend)\
    while (curbits < minbits)\
    {\
        ch = *kfileptr++; num = (num<<8)+((int)ch); curbits += 8;\
        if (ch == 255) { kfileptr++; if (kfileptr >= kfileend) { num <<= 8; curbits += 8; /*Hack to prevent read overrun on valid JPG by stuffing extra byte*/ } }\
    }


static int32_t kpegrend(const char *kfilebuf, int32_t kfilength,
                        intptr_t dakpframeplace, int32_t dakpbytesperline, int32_t daxres, int32_t dayres)
{
    int32_t i, j, v, leng = 0, xdim = 0, ydim = 0, index, prec, restartcnt, restartinterval;
    int32_t x, y, z, xx, yy, zz, *dc = NULL, num, curbits, c, daval, dabits, *hqval, *hqbits, hqcnt, *quanptr = NULL;
    int32_t passcnt = 0, ghsampmax = 0, gvsampmax = 0, glhsampmax = 0, glvsampmax = 0, glhstep, glvstep;
    int32_t eobrun, Ss, Se, Ah, Al, Alut[2], dctx[12], dcty[12], ldctx[12], /* ldcty[12], */ lshx[4], lshy[4];
    int16_t *dctbuf = 0, *dctptr[12], *ldctptr[12], *dcs = NULL;
    uint8_t ch, marker, dcflag;
    const uint8_t *kfileptr, *kfileend;

    if (!kpeginited) { kpeginited = 1; initkpeg(); }

    kfileptr = (uint8_t const *)kfilebuf;
    kfileend = &kfileptr[kfilength];

    if (B_UNBUF16(kfileptr) == B_LITTLE16(0xD8FFu)) kfileptr += 2;
    else return -1; //"%s is not a JPEG file\n",filename

    restartinterval = 0;
    for (i=0; i<4; i++) lastdc[i] = 0;
    for (i=0; i<8; i++) hufcnt[i] = 0;

    kcoltype = 0; bitdepth = 8; //For PNGOUT
    do
    {
        ch = *kfileptr++; if (ch != 255) continue;
        do { marker = *kfileptr++; }
        while (marker == 255);
        if (marker != 0xd9) //Don't read past end of buffer
        {
            leng = ((intptr_t)kfileptr[0]<<8)+(intptr_t)kfileptr[1]-2;
            kfileptr += 2;
        }
        //printf("fileoffs=%08x, marker=%02x,leng=%d",((int32_t)kfileptr)-((int32_t)kfilebuf)-2,marker,leng);
        switch (marker)
        {
        case 0xc0: case 0xc1: case 0xc2:
                        //processit!
                        kfileptr++; //numbits = *kfileptr++;

            ydim = B_BIG16(B_UNBUF16(&kfileptr[0]));
            xdim = B_BIG16(B_UNBUF16(&kfileptr[2]));
            //printf("%s: %ld / %ld = %ld\n",filename,xdim*ydim*3,kfilength,(xdim*ydim*3)/kfilength);

            kp_frameplace = dakpframeplace;
            kp_bytesperline = dakpbytesperline;
            kp_xres = daxres;
            kp_yres = dayres;
            gnumcomponents = kfileptr[4];
            kfileptr += 5;
            ghsampmax = gvsampmax = glhsampmax = glvsampmax = 0;
            for (z=0; z<gnumcomponents; z++)
            {
                gcompid[z] = kfileptr[0];
                gcomphsamp[z] = (kfileptr[1]>>4);
                gcompvsamp[z] = (kfileptr[1]&15);
                gcompquantab[z] = kfileptr[2];
                for (i=0; i<8; i++) if (gcomphsamp[z] == pow2long[i]) { gcomphsampshift[z] = i; break; }
                for (i=0; i<8; i++) if (gcompvsamp[z] == pow2long[i]) { gcompvsampshift[z] = i; break; }
                if (gcomphsamp[z] > ghsampmax) { ghsampmax = gcomphsamp[z]; glhsampmax = gcomphsampshift[z]; }
                if (gcompvsamp[z] > gvsampmax) { gvsampmax = gcompvsamp[z]; glvsampmax = gcompvsampshift[z]; }
                kfileptr += 3;
            }

            break;
        case 0xc4:  //Huffman table
                do
                {
                    ch = *kfileptr++; leng--;
                    if (ch >= 16) { index = ch-12; }
                    else { index = ch; }
                    Bmemcpy((void *)&hufnumatbit[index][1],(void const *)kfileptr,16); kfileptr += 16;
                    leng -= 16;

                    v = 0; hufcnt[index] = 0;
                    hufquickcnt[index] = 0;
                    for (i=1; i<=16; i++)
                    {
                        hufmaxatbit[index][i] = v+hufnumatbit[index][i];
                        hufvalatbit[index][i] = hufcnt[index]-v;
                        Bmemcpy((void *)&huftable[index][hufcnt[index]],(void const *)kfileptr,(int32_t)hufnumatbit[index][i]);
                        if (i <= 10)
                            for (c=0; c<hufnumatbit[index][i]; c++)
                                for (j=(1<<(10-i)); j>0; j--)
                                {
                                    hufquickval[index][hufquickcnt[index]] = huftable[index][hufcnt[index]+c];
                                    hufquickbits[index][hufquickcnt[index]] = i;
                                    hufquickcnt[index]++;
                                }
                        kfileptr += hufnumatbit[index][i];
                        leng -= hufnumatbit[index][i];
                        hufcnt[index] += hufnumatbit[index][i];
                        v = ((v+hufnumatbit[index][i])<<1);
                    }

                }
                while (leng > 0);
            break;
        case 0xdb:
                do
                {
                    ch = *kfileptr++; leng--;
                    index = (ch&15);
                    prec = (ch>>4);
                    for (z=0; z<64; z++)
                    {
                        v = (int32_t)(*kfileptr++);
                        if (prec) v = (v<<8)+((int32_t)(*kfileptr++));
                        v <<= 19;
                        if (unzig[z]&7) v = mulshr24(v,cosqr16[unzig[z]&7 ]);
                        if (unzig[z]>>3) v = mulshr24(v,cosqr16[unzig[z]>>3]);
                        quantab[index][unzig[z]] = v;
                    }
                    leng -= 64;
                    if (prec) leng -= 64;
                }
                while (leng > 0);
            break;
        case 0xdd:
                restartinterval = B_BIG16(B_UNBUF16(&kfileptr[0]));
            kfileptr += leng;
            break;
        case 0xda:
                if ((xdim <= 0) || (ydim <= 0)) { Xfree(dctbuf); return -1; }

            lnumcomponents = (int32_t)(*kfileptr++); if (!lnumcomponents) { Xfree(dctbuf); return -1; }
            if (lnumcomponents > 1) kcoltype = 2;
            for (z=0; z<lnumcomponents; z++)
            {
                lcompid[z] = kfileptr[0];
                lcompdc[z] = (kfileptr[1]>>4);
                lcompac[z] = (kfileptr[1]&15);
                kfileptr += 2;
            }

            Ss = kfileptr[0];
            Se = kfileptr[1];
            Ah = (kfileptr[2]>>4);
            Al = (kfileptr[2]&15);
            kfileptr += 3;
            //printf("passcnt=%d, Ss=%d, Se=%d, Ah=%d, Al=%d\n",passcnt,Ss,Se,Ah,Al);

            if ((!passcnt) && ((Ss) || (Se != 63) || (Ah) || (Al)))
            {
                for (z=zz=0; z<gnumcomponents; z++)
                {
                    dctx[z] = ((xdim+(ghsampmax<<3)-1)>>(glhsampmax+3)) << gcomphsampshift[z];
                    dcty[z] = ((ydim+(gvsampmax<<3)-1)>>(glvsampmax+3)) << gcompvsampshift[z];
                    zz += dctx[z]*dcty[z];
                }
                z = zz*64*sizeof(int16_t);
                dctbuf = (int16_t *)Xmalloc(z); if (!dctbuf) return -1;
                Bmemset(dctbuf,0,z);
                for (z=zz=0; z<gnumcomponents; z++) { dctptr[z] = &dctbuf[zz*64]; zz += dctx[z]*dcty[z]; }
            }

            glhstep = glvstep = 0x7fffffff;
            for (z=0; z<lnumcomponents; z++)
                for (zz=0; zz<gnumcomponents; zz++)
                    if (lcompid[z] == gcompid[zz])
                    {
                        ldctptr[z] = dctptr[zz];
                        ldctx[z] = dctx[zz];
                       // ldcty[z] = dcty[zz];
                        lcomphsamp[z] = gcomphsamp[zz];
                        lcompvsamp[z] = gcompvsamp[zz];
                        lcompquantab[z] = gcompquantab[zz];
                        if (!z)
                        {
                            lcomphsampshift0 = gcomphsampshift[zz];
                            lcompvsampshift0 = gcompvsampshift[zz];
                        }
                        lshx[z] = glhsampmax-gcomphsampshift[zz]+3;
                        lshy[z] = glvsampmax-gcompvsampshift[zz]+3;
                        if (gcomphsampshift[zz] < glhstep) glhstep = gcomphsampshift[zz];
                        if (gcompvsampshift[zz] < glvstep) glvstep = gcompvsampshift[zz];
                    }
            glhstep = (ghsampmax>>glhstep); lcomphsamp[0] = min(lcomphsamp[0],glhstep); glhstep <<= 3;
            glvstep = (gvsampmax>>glvstep); lcompvsamp[0] = min(lcompvsamp[0],glvstep); glvstep <<= 3;
            lcomphvsamp0 = lcomphsamp[0]*lcompvsamp[0];

            clipxdim = min(xdim,kp_xres);
            clipydim = min(ydim,kp_yres);

            Alut[0] = (1<<Al); Alut[1] = -Alut[0];

            restartcnt = restartinterval; eobrun = 0; marker = 0xd0;
            num = 0; curbits = 0;
            for (y=0; y<ydim; y+=glvstep)
                for (x=0; x<xdim; x+=glhstep)
                {
                    if (kfileptr-4-(uint8_t const *)kfilebuf >= kfilength) goto kpegrend_break2; //rest of file is missing!

                    if (!dctbuf) dc = dct[0];
                    for (c=0; c<lnumcomponents; c++)
                    {
                        hqval = &hufquickval[lcompac[c]+4][0];
                        hqbits = &hufquickbits[lcompac[c]+4][0];
                        hqcnt = hufquickcnt[lcompac[c]+4];
                        if (!dctbuf) quanptr = &quantab[lcompquantab[c]][0];
                        for (yy=0; yy<(lcompvsamp[c]<<3); yy+=8)
                            for (xx=0; xx<(lcomphsamp[c]<<3); xx+=8)
                            {
                                //NOTE: Might help to split this code into firstime vs. refinement (!Ah vs. Ah!=0)

                                if (dctbuf) dcs = &ldctptr[c][(((y+yy)>>lshy[c])*ldctx[c] + ((x+xx)>>lshx[c]))<<6];

                                //Get DC
                                if (!Ss)
                                {
                                    KPEG_GETBITS(curbits, 16, num, kfileptr, kfileend);
                                    if (!Ah)
                                    {
                                        i = ((num>>(curbits-10))&1023);
                                        if (i < hufquickcnt[lcompdc[c]])
                                            { daval = hufquickval[lcompdc[c]][i]; curbits -= hufquickbits[lcompdc[c]][i]; }
                                        else { huffgetval(lcompdc[c],curbits,num,&daval,&dabits); curbits -= dabits; }

                                        if (daval)
                                        {
                                            KPEG_GETBITS(curbits, daval, num, kfileptr, kfileend);
                                            curbits -= daval; v = ((unsigned)num >> curbits) & pow2mask[daval];
                                            if (v <= pow2mask[daval-1]) v -= pow2mask[daval];
                                            lastdc[c] += v;
                                        }
                                        if (!dctbuf) dc[0] = lastdc[c]; else dcs[0] = (int16_t)(lastdc[c]<<Al);
                                    }
                                    else if (num&(pow2long[--curbits])) dcs[0] |= ((int16_t)Alut[0]);
                                }

                                //Get AC
                                if (!dctbuf) Bmemset((void *)&dc[1],0,63*4);
                                z = max(Ss,1); dcflag = 1;
                                if (eobrun <= 0)
                                {
                                    for (; z<=Se; z++)
                                    {
                                        KPEG_GETBITS(curbits, 16, num, kfileptr, kfileend);
                                        i = ((num>>(curbits-10))&1023);
                                        if (i < hqcnt)
                                            { daval = hqval[i]; curbits -= hqbits[i]; }
                                        else { huffgetval(lcompac[c]+4,curbits,num,&daval,&dabits); curbits -= dabits; }

                                        zz = (daval>>4); daval &= 15;
                                        if (daval)
                                        {
                                            if (Ah)
                                            {
                                                KPEG_GETBITS(curbits, 8, num, kfileptr, kfileend);
                                                if (num&(pow2long[--curbits])) daval = Alut[0]; else daval = Alut[1];
                                            }
                                        }
                                        else if (zz < 15)
                                        {
                                            eobrun = pow2long[zz];
                                            if (zz)
                                            {
                                                KPEG_GETBITS(curbits, zz, num, kfileptr, kfileend);
                                                curbits -= zz; eobrun += ((unsigned)num >> curbits) & pow2mask[zz];
                                            }
                                            if (!Ah) eobrun--;
                                            break;
                                        }
                                        if (Ah)
                                        {
                                            do
                                            {
                                                if (dcs[z])
                                                {
                                                    KPEG_GETBITS(curbits, 8, num, kfileptr, kfileend);
                                                    if (num&(pow2long[--curbits])) dcs[z] += (int16_t)Alut[dcs[z] < 0];
                                                }
                                                else if (--zz < 0) break;
                                                z++;
                                            }
                                            while (z <= Se);
                                            if (daval) dcs[z] = (int16_t)daval;
                                        }
                                        else
                                        {
                                            z += zz; if (z > Se) break;

                                            KPEG_GETBITS(curbits, daval, num, kfileptr, kfileend);
                                            curbits -= daval; v = ((unsigned)num >> curbits) & pow2mask[daval];
                                            if (daval>=1 /* FIXME ? */ && v <= pow2mask[daval-1]) v -= pow2mask[daval];
                                            dcflag |= dcflagor[z];
                                            if (!dctbuf) dc[unzig[z]] = v; else dcs[z] = (int16_t)(v<<Al);
                                        }
                                    }
                                }
                                else if (!Ah) eobrun--;
                                if ((Ah) && (eobrun > 0))
                                {
                                    eobrun--;
                                    for (; z<=Se; z++)
                                    {
                                        if (!dcs[z]) continue;
                                        KPEG_GETBITS(curbits, 8, num, kfileptr, kfileend);
                                        if (num&(pow2long[--curbits])) dcs[z] += ((int16_t)Alut[dcs[z] < 0]);
                                    }
                                }

                                if (!dctbuf)
                                {
                                    for (z=64-1; z>=0; z--) dc[z] *= quanptr[z];
                                    invdct8x8(dc,dcflag); dc += 64;
                                }
                            }
                    }

                    if (!dctbuf) kplib_yrbrend_func(x,y,&dct[0][0]);

                    restartcnt--;
                    if (!restartcnt)
                    {
                        kfileptr += 1-(curbits>>3); curbits = 0;
                        if ((kfileptr[-2] != 255) || (kfileptr[-1] != marker)) kfileptr--;
                        marker++; if (marker >= 0xd8) marker = 0xd0;
                        restartcnt = restartinterval;
                        for (i=0; i<4; i++) lastdc[i] = 0;
                        eobrun = 0;
                    }
                }
            kpegrend_break2:;
            if (!dctbuf) return 0;
            passcnt++; kfileptr -= ((curbits>>3)+1); break;
        case 0xd9: break;
        default: kfileptr += leng; break;
        }
    }
    while (kfileptr-(uint8_t const *)kfilebuf < kfilength);

    if (!dctbuf) return 0;

    lnumcomponents = gnumcomponents;
    for (i=0; i<gnumcomponents; i++)
    {
        lcomphsamp[i] = gcomphsamp[i]; gcomphsamp[i] <<= 3;
        lcompvsamp[i] = gcompvsamp[i]; gcompvsamp[i] <<= 3;
        lshx[i] = glhsampmax-gcomphsampshift[i]+3;
        lshy[i] = glvsampmax-gcompvsampshift[i]+3;
    }
    lcomphsampshift0 = gcomphsampshift[0];
    lcompvsampshift0 = gcompvsampshift[0];
    lcomphvsamp0 = (lcomphsamp[0]<<lcompvsampshift0);
    for (y=0; y<ydim; y+=gcompvsamp[0])
        for (x=0; x<xdim; x+=gcomphsamp[0])
        {
            dc = dct[0];
            for (c=0; c<gnumcomponents; c++)
                for (yy=0; yy<gcompvsamp[c]; yy+=8)
                    for (xx=0; xx<gcomphsamp[c]; xx+=8,dc+=64)
                    {
                        dcs = &dctptr[c][(((y+yy)>>lshy[c])*dctx[c] + ((x+xx)>>lshx[c]))<<6];
                        quanptr = &quantab[gcompquantab[c]][0];
                        for (z=0; z<64; z++) dc[z] = ((int32_t)dcs[zigit[z]])*quanptr[z];
                        invdct8x8(dc,0xff);
                    }
            kplib_yrbrend_func(x,y,&dct[0][0]);
        }

    Xfree(dctbuf); return 0;
}

//==============================  KPEGILIB ends ==============================
//================================ GIF begins ================================

static uint8_t suffix[4100], filbuffer[768], tempstack[4096];
static int32_t prefix[4100];

static int32_t kgifrend(const char *kfilebuf, int32_t kfilelength,
                        intptr_t dakpframeplace, int32_t dakpbytesperline, int32_t daxres, int32_t dayres)
{
    int32_t i, x, y, xsiz, ysiz, yinc, xend, xspan, yspan, currstr, numbitgoal;
    int32_t lzcols, dat, blocklen, bitcnt, xoff, transcol;
    intptr_t yoff;
    char numbits, startnumbits, chunkind, ilacefirst;
    const uint8_t *ptr, *cptr = NULL;
    int32_t daglobxoffs = 0, daglobyoffs = 0;

    UNREFERENCED_PARAMETER(kfilelength);

    kcoltype = 3; bitdepth = 8; //For PNGOUT

    if ((kfilebuf[0] != 'G') || (kfilebuf[1] != 'I') || (kfilebuf[2] != 'F')) return -1;
    paleng = (1<<((kfilebuf[10]&7)+1));
    ptr = (uint8_t const *)&kfilebuf[13];
    if (kfilebuf[10]&128) { cptr = ptr; ptr += paleng*3; }
    transcol = -1;
    while ((chunkind = *ptr++) == '!')
    {
        //! 0xf9 leng flags ? ? transcol
        if (ptr[0] == 0xf9) { if (ptr[2]&1) transcol = (int32_t)(((uint8_t)ptr[5])); }
        ptr++;
        do { i = *ptr++; ptr += i; }
        while (i);
    }
    if (chunkind != ',') return -1;

    xoff = B_LITTLE16(B_UNBUF16(&ptr[0]));
    yoff = B_LITTLE16(B_UNBUF16(&ptr[2]));
    xspan = B_LITTLE16(B_UNBUF16(&ptr[4]));
    yspan = B_LITTLE16(B_UNBUF16(&ptr[6])); ptr += 9;
    if (ptr[-1]&64) { yinc = 8; ilacefirst = 1; }
    else { yinc = 1; ilacefirst = 0; }
    if (ptr[-1]&128)
    {
        paleng = (1<<((ptr[-1]&7)+1));
        cptr = ptr; ptr += paleng*3;
    }

    for (i=0; i<paleng; i++)
        palcol[i] = B_LITTLE32((((int32_t)cptr[i*3])<<16) + (((int32_t)cptr[i*3+1])<<8) + ((int32_t)cptr[i*3+2]) + 0xff000000);
    for (; i<256; i++) palcol[i] = B_LITTLE32(0xff000000);
    if (transcol >= 0) palcol[transcol] &= B_LITTLE32(~0xff000000);

    //Handle GIF files with different logical&image sizes or non-0 offsets (added 05/15/2004)
    xsiz = B_LITTLE16(B_UNBUF16(&kfilebuf[6]));
    ysiz = B_LITTLE16(B_UNBUF16(&kfilebuf[8]));
    if ((xoff != 0) || (yoff != 0) || (xsiz != xspan) || (ysiz != yspan))
    {
        daglobxoffs += xoff; //Offset bitmap image by extra amount
        daglobyoffs += yoff;
    }

    xspan += daglobxoffs;
    yspan += daglobyoffs;  //UGLY HACK
    y = daglobyoffs;
    if ((uint32_t)y < (uint32_t)dayres)
        { yoff = y*dakpbytesperline+dakpframeplace; x = daglobxoffs; xend = xspan; }
    else
        { x = daglobxoffs+0x80000000; xend = xspan+0x80000000; }

    lzcols = (1<<(*ptr)); startnumbits = (uint8_t)((*ptr)+1); ptr++;
    for (i=lzcols-1; i>=0; i--) { suffix[i] = (uint8_t)(prefix[i] = i); }
    currstr = lzcols+2; numbits = startnumbits; numbitgoal = (lzcols<<1);
    blocklen = *ptr++;
    Bmemcpy(filbuffer,ptr,blocklen); ptr += blocklen;
    bitcnt = 0;
    while (1)
    {
        dat = (B_LITTLE32(B_UNBUF32(&filbuffer[bitcnt>>3]))>>(bitcnt&7)) & (numbitgoal-1);
        bitcnt += numbits;
        if ((bitcnt>>3) > blocklen-3)
        {
            B_BUF16(filbuffer, B_UNBUF16(&filbuffer[bitcnt>>3]));
            i = blocklen-(bitcnt>>3);
            blocklen = (int32_t)*ptr++;
            Bmemcpy(&filbuffer[i],ptr,blocklen); ptr += blocklen;
            bitcnt &= 7; blocklen += i;
        }
        if (dat == lzcols)
        {
            currstr = lzcols+2; numbits = startnumbits; numbitgoal = (lzcols<<1);
            continue;
        }
        if ((currstr == numbitgoal) && (numbits < 12))
            { numbits++; numbitgoal <<= 1; }

        prefix[currstr] = dat;
        for (i=0; dat>=lzcols; dat=prefix[dat]) tempstack[i++] = suffix[dat];
        tempstack[i] = (uint8_t)prefix[dat];
        suffix[currstr-1] = suffix[currstr] = (uint8_t)dat;

        for (; i>=0; i--)
        {
            if ((uint32_t)x < (uint32_t)daxres)
                B_BUF32((void *) (yoff+(x<<2)), palcol[(int32_t)tempstack[i]]);
            x++;
            if (x == xend)
            {
                y += yinc;
                if (y >= yspan)
                    switch (yinc)
                    {
                    case 8: if (!ilacefirst) { y = daglobyoffs+2; yinc = 4; break; }
                        ilacefirst = 0; y = daglobyoffs+4; yinc = 8; break;
                    case 4: y = daglobyoffs+1; yinc = 2; break;
                    case 2: case 1: return 0;
                    }
                if ((uint32_t)y < (uint32_t)dayres)
                    { yoff = y*dakpbytesperline+dakpframeplace; x = daglobxoffs; xend = xspan; }
                else
                    { x = daglobxoffs+0x80000000; xend = xspan+0x80000000; }
            }
        }
        currstr++;
    }
}

//===============================  GIF ends ==================================
//==============================  CEL begins =================================

//   //old .CEL format:
//int16_t id = 0x9119, xdim, ydim, xoff, yoff, id = 0x0008;
//int32_t imagebytes, filler[4];
//char pal6bit[256][3], image[ydim][xdim];
#ifdef KPCEL
static int32_t kcelrend(const char *buf, int32_t fleng,
                        intptr_t dakpframeplace, int32_t dakpbytesperline, int32_t daxres, int32_t dayres)
{
    int32_t i, x, y, x0, x1, y0, y1, xsiz, ysiz;
    const char *cptr;

    UNREFERENCED_PARAMETER(fleng);

    kcoltype = 3; bitdepth = 8; paleng = 256; //For PNGOUT

    xsiz = (int32_t)B_LITTLE16(B_UNBUF16(&buf[2])); if (xsiz <= 0) return -1;
    ysiz = (int32_t)B_LITTLE16(B_UNBUF16(&buf[4])); if (ysiz <= 0) return -1;

    cptr = &buf[32];
    for (i=0; i<256; i++)
    {
        palcol[i] = (((int32_t)cptr[0])<<18) +
                    (((int32_t)cptr[1])<<10) +
                    (((int32_t)cptr[2])<< 2) + B_LITTLE32(0xff000000);
        cptr += 3;
    }

    x0 = 0; x1 = xsiz;
    y0 = 0; y1 = ysiz;
    for (y=y0; y<y1; y++)
        for (x=x0; x<x1; x++)
        {
            if (((uint32_t)x < (uint32_t)daxres) && ((uint32_t)y < (uint32_t)dayres))
                B_BUF32(y*dakpbytesperline+x*4+dakpframeplace, palcol[cptr[0]]);
            cptr++;
        }
    return 0;
}
#endif

//===============================  CEL ends ==================================
//=============================  TARGA begins ================================

static int32_t ktgarend(const char *header, int32_t fleng,
                        intptr_t dakpframeplace, int32_t dakpbytesperline, int32_t daxres, int32_t dayres)
{
    int32_t i = 0, x, y, pi, xi, yi, x0, x1, y0, y1, xsiz, ysiz, rlestat, colbyte, pixbyte;
    intptr_t p;

    const uint8_t *fptr, *cptr = NULL, *nptr;

    //Ugly and unreliable identification for .TGA!
    if ((fleng < 19) || (header[1]&0xfe)) return -1;
    if ((header[2] >= 12) || (!((1<<header[2])&0xe0e))) return -1;
    if ((header[16]&7) || (header[16] == 0) || (header[16] > 32)) return -1;
    if (header[17]&0xc0) return -1;

    fptr = (uint8_t const *)&header[header[0]+18];
    xsiz = (int32_t)B_LITTLE16(B_UNBUF16(&header[12])); if (xsiz <= 0) return -1;
    ysiz = (int32_t)B_LITTLE16(B_UNBUF16(&header[14])); if (ysiz <= 0) return -1;
    colbyte = ((((int32_t)header[16])+7)>>3);

    if (header[1] == 1)
    {
        pixbyte = ((((int32_t)header[7])+7)>>3);
        cptr = &fptr[-B_LITTLE16(B_UNBUF16(&header[3]))*pixbyte];
        fptr += B_LITTLE16(B_UNBUF16(&header[5]))*pixbyte;
    }
    else pixbyte = colbyte;

    switch (pixbyte)  // For PNGOUT
    {
        case 1:
            kcoltype  = 0;
            bitdepth  = 8;
            palcol[0] = B_LITTLE32(0xff000000);
            for (i = 1; i < 256; i++) palcol[i] = palcol[i - 1] + B_LITTLE32(0x10101);
            break;
        case 2:
        case 3: kcoltype = 2; break;
        case 4: kcoltype = 6; break;
    }

    if (!(header[17]&16)) { x0 = 0;      x1 = xsiz; xi = 1; }
    else { x0 = xsiz-1; x1 = -1;   xi =-1; }
    if (header[17]&32) { y0 = 0;      y1 = ysiz; yi = 1; pi = dakpbytesperline; }
    else { y0 = ysiz-1; y1 = -1;   yi =-1; pi =-dakpbytesperline; }
    if (header[2] < 8) rlestat = -2; else rlestat = -1;

    p = y0*dakpbytesperline+dakpframeplace;
    for (y=y0; y!=y1; y+=yi,p+=pi)
        for (x=x0; x!=x1; x+=xi)
        {
            if (rlestat < 128)
            {
                if ((rlestat&127) == 127) { rlestat = (int32_t)fptr[0]; fptr++; }
                if (header[1] == 1)
                {
                    if (colbyte == 1) i = fptr[0];
                    else i = (int32_t)B_LITTLE16(B_UNBUF16(&fptr[0]));
                    nptr = &cptr[i*pixbyte];
                }
                else nptr = fptr;

                switch (pixbyte)
                {
                case 1: i = palcol[(int32_t)nptr[0]]; break;
                case 2: i = (int32_t)B_LITTLE16(B_UNBUF16(&nptr[0]));
                    i = B_LITTLE32(((i&0x7c00)<<9) + ((i&0x03e0)<<6) + ((i&0x001f)<<3) + 0xff000000);
                    break;
                case 3: i = (B_UNBUF32(&nptr[0])) | B_LITTLE32(0xff000000); break;
                case 4: i = (B_UNBUF32(&nptr[0])); break;
                }
                fptr += colbyte;
            }
            if (rlestat >= 0) rlestat--;

            if (((uint32_t)x < (uint32_t)daxres) && ((uint32_t)y < (uint32_t)dayres))
                B_BUF32((void *) (x*4+p), i);
        }
    return 0;
}

//==============================  TARGA ends =================================
//==============================  BMP begins =================================
//TODO: handle BI_RLE8 and BI_RLE4 (compression types 1&2 respectively)
//                        +---------------+
//                        |  0(2): "BM"   |
// +---------------------+| 10(4): rastoff| +------------------+
// |headsiz=12 (OS/2 1.x)|| 14(4): headsiz| | All new formats: |
//++---------------------++-------------+-+-+------------------+-----------------------+
//| 18(2): xsiz                         | 18(4): xsiz                                  |
//| 20(2): ysiz                         | 22(4): ysiz                                  |
//| 22(2): planes (always 1)            | 26(2): planes (always 1)                     |
//| 24(2): cdim (1,4,8,24)              | 28(2): cdim (1,4,8,16,24,32)                 |
//| if (cdim < 16)                      | 30(4): compression (0,1,2,3!?,4)             |
//|    26(rastoff-14-headsiz): pal(bgr) | 34(4): (bitmap data size+3)&3                |
//|                                     | 46(4): N colors (0=2^cdim)                   |
//|                                     | if (cdim < 16)                               |
//|                                     |    14+headsiz(rastoff-14-headsiz): pal(bgr0) |
//+---------------------+---------------+---------+------------------------------------+
//                      | rastoff(?): bitmap data |
//                      +-------------------------+
static int32_t kbmprend(const char *buf, int32_t fleng,
                        intptr_t dakpframeplace, int32_t dakpbytesperline, int32_t daxres, int32_t dayres)
{
    int32_t i, j, x, y, x0, x1, y0, y1, rastoff, headsiz, xsiz, ysiz, cdim, comp, cptrinc, *lptr;
    const char *cptr;

    UNREFERENCED_PARAMETER(fleng);

    headsiz = B_UNBUF32(&buf[14]);
    if (headsiz == (int32_t)B_LITTLE32(12)) //OS/2 1.x (old format)
    {
        if (B_UNBUF16(&buf[22]) != B_LITTLE16(1)) return -1;
        xsiz = (int32_t)B_LITTLE16(B_UNBUF16(&buf[18]));
        ysiz = (int32_t)B_LITTLE16(B_UNBUF16(&buf[20]));
        cdim = (int32_t)B_LITTLE16(B_UNBUF16(&buf[24]));
        comp = 0;
    }
    else //All newer formats...
    {
        if (B_UNBUF16(&buf[26]) != B_LITTLE16(1)) return -1;
        xsiz = B_LITTLE32(B_UNBUF32(&buf[18]));
        ysiz = B_LITTLE32(B_UNBUF32(&buf[22]));
        cdim = (int32_t)B_LITTLE16(B_UNBUF16(&buf[28]));
        comp = B_LITTLE32(B_UNBUF32(&buf[30]));
    }
    if ((xsiz <= 0) || (!ysiz)) return -1;
    //cdim must be: (1,4,8,16,24,32)
    if (((uint32_t)(cdim-1) >= (uint32_t)32) || (!((1<<cdim)&0x1010113))) return -1;
    if ((comp != 0) && (comp != 3)) return -1;

    rastoff = B_LITTLE32(B_UNBUF32(&buf[10]));

    if (cdim < 16)
    {
        if (cdim == 2) { palcol[0] = 0xffffffff; palcol[1] = B_LITTLE32(0xff000000); }
        if (headsiz == (int32_t)B_LITTLE32(12)) j = 3; else j = 4;
        for (i=0,cptr=&buf[headsiz+14]; cptr<&buf[rastoff]; i++,cptr+=j)
            palcol[i] = ((B_UNBUF32(&cptr[0]))|B_LITTLE32(0xff000000));
        kcoltype = 3; bitdepth = (int8_t)cdim; paleng = i; //For PNGOUT
    }
    else if (!(cdim&15))
    {
        kcoltype = 2;
        switch (cdim)
        {
        case 16: palcol[0] = 10; palcol[1] = 5; palcol[2] = 0; palcol[3] = 5; palcol[4] = 5; palcol[5] = 5; break;
        case 32: palcol[0] = 16; palcol[1] = 8; palcol[2] = 0; palcol[3] = 8; palcol[4] = 8; palcol[5] = 8; break;
        }
        if (comp == 3) //BI_BITFIELD (RGB masks)
        {
            for (i=0; i<3; i++)
            {
                j = B_UNBUF32(&buf[headsiz+(i<<2)+14]);
                for (palcol[i]=0; palcol[i]<32; palcol[i]++)
                {
                    if (j&1) break;
                    j = (((uint32_t)j)>>1);
                }
                for (palcol[i+3]=0; palcol[i+3]<32; palcol[i+3]++)
                {
                    if (!(j&1)) break;
                    j = (((uint32_t)j)>>1);
                }
            }
        }
        palcol[0] = 24-(palcol[0]+palcol[3]);
        palcol[1] = 16-(palcol[1]+palcol[4]);
        palcol[2] =  8-(palcol[2]+palcol[5]);
        palcol[3] = (-(1<<(24-palcol[3]))&0x00ff0000);
        palcol[4] = (-(1<<(16-palcol[4]))&0x0000ff00);
        palcol[5] = (-(1<<(8-palcol[5]))&0x000000ff);
    }

    cptrinc = (((xsiz*cdim+31)>>3)&~3); cptr = &buf[rastoff];
    if (ysiz < 0) { ysiz = -ysiz; }
    else { cptr = &cptr[(ysiz-1)*cptrinc]; cptrinc = -cptrinc; }

    x0 = 0; x1 = xsiz;
    y0 = 0; y1 = ysiz;
    if (x1 > daxres) x1 = daxres;
    for (y=y0; y<y1; y++,cptr=&cptr[cptrinc])
    {
        if ((uint32_t)y >= (uint32_t)dayres) continue;
        lptr = (int32_t *)(y*dakpbytesperline+dakpframeplace);
        switch (cdim)
        {
        case  1: for (x=x0; x<x1; x++) lptr[x] = palcol[(int32_t)((cptr[x>>3]>>((x&7)^7))&1)]; break;
        case  4: for (x=x0; x<x1; x++) lptr[x] = palcol[(int32_t)((cptr[x>>1]>>(((x&1)^1)<<2))&15)]; break;
        case  8: for (x=x0; x<x1; x++) lptr[x] = palcol[(int32_t)(cptr[x])]; break;
        case 16: for (x=x0; x<x1; x++)
                {
                    i = ((int32_t)(B_UNBUF16(&cptr[x<<1])));
                    lptr[x] = (klrotl(i,palcol[0])&palcol[3]) +
                              (klrotl(i,palcol[1])&palcol[4]) +
                              (klrotl(i,palcol[2])&palcol[5]) + B_LITTLE32(0xff000000);
                } break;
        case 24: for (x=x0; x<x1; x++) lptr[x] = ((B_UNBUF32(&cptr[x*3]))|B_LITTLE32(0xff000000)); break;
        case 32: for (x=x0; x<x1; x++)
                {
                    i = (B_UNBUF32(&cptr[x<<2]));
                    lptr[x] = (klrotl(i,palcol[0])&palcol[3]) +
                              (klrotl(i,palcol[1])&palcol[4]) +
                              (klrotl(i,palcol[2])&palcol[5]) + B_LITTLE32(0xff000000);
                } break;
        }

    }
    return 0;
}
//===============================  BMP ends ==================================
//==============================  PCX begins =================================
//Note: currently only supports 8 and 24 bit PCX
static int32_t kpcxrend(const char *buf, int32_t fleng,
                        intptr_t dakpframeplace, int32_t dakpbytesperline, int32_t daxres, int32_t dayres)
{
    int32_t  j, x, y, nplanes, x0, x1, y0, y1, bpl, xsiz, ysiz;
    intptr_t p,i;
    uint8_t c;
    uint8_t const *cptr;

    if (B_UNBUF32(buf) != B_LITTLE32(0x0801050a)) return -1;
    xsiz = B_LITTLE16(B_UNBUF16(&buf[ 8]))-B_LITTLE16(B_UNBUF16(&buf[4]))+1; if (xsiz <= 0) return -1;
    ysiz = B_LITTLE16(B_UNBUF16(&buf[10]))-B_LITTLE16(B_UNBUF16(&buf[6]))+1; if (ysiz <= 0) return -1;
    //buf[3]: bpp/plane:{1,2,4,8}
    nplanes = buf[65]; //nplanes*bpl bytes per scanline; always be decoding break at the end of scan line
    bpl = B_LITTLE16(B_UNBUF16(&buf[66])); //#bytes per scanline. Must be EVEN. May have unused data.
    if (nplanes == 1)
    {
        //if (buf[fleng-769] != 12) return -1; //Some PCX are buggy!
        cptr = (uint8_t const *)&buf[fleng-768];
        for (i=0; i<256; i++)
        {
            palcol[i] = (((int32_t)cptr[0])<<16) +
                        (((int32_t)cptr[1])<< 8) +
                        (((int32_t)cptr[2])) + B_LITTLE32(0xff000000);
            cptr += 3;
        }
        kcoltype = 3; bitdepth = 8; paleng = 256; //For PNGOUT
    }
    else if (nplanes == 3)
    {
        kcoltype = 2;

        //Make sure background is opaque (since 24-bit PCX renderer doesn't do it)
        x0 = 0; x1 = min(xsiz,daxres);
        y0 = 0; y1 = min(ysiz,dayres);
        i = y0*dakpbytesperline + dakpframeplace+3;
        for (y=y0; y<y1; y++,i+=dakpbytesperline)
            for (x=x0; x<x1; x++) *(char *)((x<<2)+i) = 255;
    }

    x = x0 = 0; x1 = xsiz;
    y = y0 = 0; y1 = ysiz;
    cptr = (uint8_t const *)&buf[128];
    p = y*dakpbytesperline+dakpframeplace;

    if (bpl > xsiz) { daxres = min(daxres,x1); x1 += bpl-xsiz; }

    j = nplanes-1; daxres <<= 2; x0 <<= 2; x1 <<= 2; x <<= 2; x += j;
    if (nplanes == 1) //8-bit PCX
    {
        do
        {
            c = *cptr++; if (c < 192) i = 1; else { i = (c&63); c = *cptr++; }
            j = palcol[(int32_t)c];
            for (; i; i--)
            {
                if ((uint32_t)y < (uint32_t)dayres)
                    if ((uint32_t)x < (uint32_t)daxres) B_BUF32((void *) (x+p), j);
                x += 4; if (x >= x1) { x = x0; y++; p += dakpbytesperline; }
            }
        }
        while (y < y1);
    }
    else if (nplanes == 3) //24-bit PCX
    {
        do
        {
            c = *cptr++; if (c < 192) i = 1; else { i = (c&63); c = *cptr++; }
            for (; i; i--)
            {
                if ((uint32_t)y < (uint32_t)dayres)
                    if ((uint32_t)x < (uint32_t)daxres) *(char *)(x+p) = c;
                x += 4; if (x >= x1) { j--; if (j < 0) { j = 3-1; y++; p += dakpbytesperline; } x = x0+j; }
            }
        }
        while (y < y1);
    }

    return 0;
}

//===============================  PCX ends ==================================
//==============================  DDS begins =================================

//Note:currently supports: DXT1,DXT2,DXT3,DXT4,DXT5,A8R8G8B8

#ifdef KPDDS
static int32_t kddsrend(const char *buf, int32_t leng,
                        intptr_t frameptr, int32_t bpl, int32_t xdim, int32_t ydim, int32_t xoff, int32_t yoff)
{
    int32_t x, y, z = 0, xx, yy, xsiz, ysiz, dxt, al[2], ai, k, v, c0, c1, stride;
    intptr_t j;
    uint32_t lut[256], r[4], g[4], b[4], a[8], rr, gg, bb;
    uint8_t *uptr, *wptr;

    UNREFERENCED_PARAMETER(leng);

    xsiz = B_LITTLE32(B_UNBUF32(&buf[16]));
    ysiz = B_LITTLE32(B_UNBUF32(&buf[12]));
    if ((B_UNBUF32(&buf[80]))&B_LITTLE32(64)) //Uncompressed supports only A8R8G8B8 for now
    {
        if ((B_UNBUF32(&buf[88])) != B_LITTLE32(32)) return -1;
        if ((B_UNBUF32(&buf[92])) != B_LITTLE32(0x00ff0000)) return -1;
        if ((B_UNBUF32(&buf[96])) != B_LITTLE32(0x0000ff00)) return -1;
        if ((B_UNBUF32(&buf[100])) != B_LITTLE32(0x000000ff)) return -1;
        if ((B_UNBUF32(&buf[104])) != B_LITTLE32(0xff000000)) return -1;
        buf += 128;

        j = yoff*bpl + (xoff<<2) + frameptr; xx = (xsiz<<2);
        if (xoff < 0) { j -= (xoff<<2); buf -= (xoff<<2); xsiz += xoff; }
        xsiz = (min(xsiz,xdim-xoff)<<2); ysiz = min(ysiz,ydim);
        for (y=0; y<ysiz; y++,j+=bpl,buf+=xx)
        {
            if ((uint32_t)(y+yoff) >= (uint32_t)ydim) continue;
            Bmemcpy((void *)j,(void *)buf,xsiz);
        }
        return 0;
    }
    if (!((B_UNBUF32(&buf[80]))&B_LITTLE32(4))) return -1; //FOURCC invalid
    dxt = buf[87]-'0';
    if ((buf[84] != 'D') || (buf[85] != 'X') || (buf[86] != 'T') || (dxt < 1) || (dxt > 5)) return -1;
    buf += 128;

    if (!(dxt&1))
    {
        for (z=256-1; z>0; z--) lut[z] = tabledivide32_noinline(255<<16, z);
        lut[0] = (1<<16);
    }
    if (dxt == 1) stride = (xsiz<<1); else stride = (xsiz<<2);

    for (y=0; y<ysiz; y+=4,buf+=stride)
        for (x=0; x<xsiz; x+=4)
        {
            if (dxt == 1) uptr = (uint8_t *)(((intptr_t)buf)+(x<<1));
            else uptr = (uint8_t *)(((intptr_t)buf)+(x<<2)+8);
            c0 = B_LITTLE16(B_UNBUF16(&uptr[0]));
            r[0] = ((c0>>8)&0xf8); g[0] = ((c0>>3)&0xfc); b[0] = ((c0<<3)&0xfc); a[0] = 255;
            c1 = B_LITTLE16(B_UNBUF16(&uptr[2]));
            r[1] = ((c1>>8)&0xf8); g[1] = ((c1>>3)&0xfc); b[1] = ((c1<<3)&0xfc); a[1] = 255;
            if ((c0 > c1) || (dxt != 1))
            {
                r[2] = (((r[0]*2 + r[1] + 1)*(65536/3))>>16);
                g[2] = (((g[0]*2 + g[1] + 1)*(65536/3))>>16);
                b[2] = (((b[0]*2 + b[1] + 1)*(65536/3))>>16); a[2] = 255;
                r[3] = (((r[0] + r[1]*2 + 1)*(65536/3))>>16);
                g[3] = (((g[0] + g[1]*2 + 1)*(65536/3))>>16);
                b[3] = (((b[0] + b[1]*2 + 1)*(65536/3))>>16); a[3] = 255;
            }
            else
            {
                r[2] = (r[0] + r[1])>>1;
                g[2] = (g[0] + g[1])>>1;
                b[2] = (b[0] + b[1])>>1; a[2] = 255;
                r[3] = g[3] = b[3] = a[3] = 0; //Transparent
            }
            v = B_LITTLE32(B_UNBUF32(&uptr[4]));
            if (dxt >= 4)
            {
                a[0] = uptr[-8]; a[1] = uptr[-7]; k = a[1]-a[0];
                if (k < 0)
                {
                    z = a[0]*6 + a[1] + 3;
                    for (j=2; j<8; j++) { a[j] = ((z*(65536/7))>>16); z += k; }
                }
                else
                {
                    z = a[0]*4 + a[1] + 2;
                    for (j=2; j<6; j++) { a[j] = ((z*(65536/5))>>16); z += k; }
                    a[6] = 0; a[7] = 255;
                }
                al[0] = B_LITTLE32(B_UNBUF32(&uptr[-6]));
                al[1] = B_LITTLE32(B_UNBUF32(&uptr[-3]));
            }
            wptr = (uint8_t *)((y+yoff)*bpl + ((x+xoff)<<2) + frameptr);
            ai = 0;
            for (yy=0; yy<4; yy++,wptr+=bpl)
            {
                if ((uint32_t)(y+yy+yoff) >= (uint32_t)ydim) { ai += 4; continue; }
                for (xx=0; xx<4; xx++,ai++)
                {
                    if ((uint32_t)(x+xx+xoff) >= (uint32_t)xdim) continue;

                    j = ((v>>(ai<<1))&3);
                    switch (dxt)
                    {
                    case 1: z = a[j]; break;
                    case 2: case 3: z = ((uptr[(ai>>1)-8] >> ((xx&1)<<2))&15)*17; break;
                    case 4: case 5: z = a[(al[yy>>1] >> ((ai&7)*3))&7]; break;
                    }
                    rr = r[j]; gg = g[j]; bb = b[j];
                    if (!(dxt&1))
                    {
                        bb = min((bb*lut[z])>>16,255);
                        gg = min((gg*lut[z])>>16,255);
                        rr = min((rr*lut[z])>>16,255);
                    }
                    wptr[(xx<<2)+0] = (uint8_t)bb;
                    wptr[(xx<<2)+1] = (uint8_t)gg;
                    wptr[(xx<<2)+2] = (uint8_t)rr;
                    wptr[(xx<<2)+3] = (uint8_t)z;
                }
            }
        }
    return 0;
}
#endif
//===============================  DDS ends ==================================
//=================== External picture interface begins ======================

static int32_t istarga(const uint8_t *buf, int32_t leng)
{
    return ((leng >= 19) && (!(buf[1]&0xfe)) && (buf[2] < 12) && ((1<<buf[2])&0xe0e) &&
        (!(buf[16]&7)) && (buf[16] != 0) && (buf[16] <= 32) && !(buf[17]&0xc0));
}


void kpgetdim(const char *buf, int32_t leng, int32_t *xsiz, int32_t *ysiz)
{
    int32_t const *lptr;
    const uint8_t *cptr;
    const uint8_t *ubuf = (uint8_t const *)buf;

    (*xsiz) = (*ysiz) = 0; if (leng < 16) return;
    if (B_UNBUF16(&ubuf[0]) == B_LITTLE16(0x5089)) //.PNG
    {
        lptr = (int32_t const *)buf;
        if ((lptr[0] != (int32_t)B_LITTLE32(0x474e5089)) || (lptr[1] != (int32_t)B_LITTLE32(0x0a1a0a0d))) return;
        lptr = &lptr[2];
        while (((uintptr_t)lptr-(uintptr_t)buf) < (uintptr_t)(leng-16))
        {
            if (lptr[1] == (int32_t)B_LITTLE32(0x52444849)) //IHDR
                {(*xsiz) = B_BIG32(lptr[2]); (*ysiz) = B_BIG32(lptr[3]); break; }
            lptr = (int32_t *)((intptr_t)lptr + B_BIG32(lptr[0]) + 12);
        }
    }
    else if (B_UNBUF16(&ubuf[0]) == B_LITTLE16(0xD8FFu)) //.JPG
    {
        cptr = (uint8_t const *)&buf[2];
        while (((uintptr_t)cptr-(uintptr_t)buf) < (uintptr_t)(leng-8))
        {
            if ((cptr[0] != 0xff) || (cptr[1] == 0xff)) { cptr++; continue; }
            if ((uint32_t)(cptr[1]-0xc0) < 3)
            {
                (*ysiz) = B_BIG16(B_UNBUF16(&cptr[5]));
                (*xsiz) = B_BIG16(B_UNBUF16(&cptr[7]));
                break;
            }
            cptr = &cptr[B_BIG16(B_UNBUF16(&cptr[2]))+2];
        }
    }
    else
    {
        if ((ubuf[0] == 'G') && (ubuf[1] == 'I') && (ubuf[2] == 'F')) //.GIF
        {
            (*xsiz) = (int32_t) B_LITTLE16(B_UNBUF16(&buf[6]));
            (*ysiz) = (int32_t) B_LITTLE16(B_UNBUF16(&buf[8]));
        }
        else if ((ubuf[0] == 'B') && (ubuf[1] == 'M')) //.BMP
        {
            if (B_UNBUF32(&buf[14]) == B_LITTLE32(12)) //OS/2 1.x (old format)
            {
                if (B_UNBUF16(&buf[22]) != B_LITTLE16(1)) return;
                (*xsiz) = (int32_t) B_LITTLE16(B_UNBUF16(&buf[18]));
                (*ysiz) = (int32_t) B_LITTLE16(B_UNBUF16(&buf[20]));
            }
            else //All newer formats...
            {
                if (B_UNBUF16(&buf[26]) != B_LITTLE16(1)) return;
                (*xsiz) = B_LITTLE32(B_UNBUF32(&buf[18]));
                (*ysiz) = B_LITTLE32(B_UNBUF32(&buf[22]));
            }
        }
        else if (B_UNBUF32(ubuf) == B_LITTLE32(0x0801050a)) //.PCX
        {
            (*xsiz) = B_LITTLE16(B_UNBUF16(&buf[8]))-B_LITTLE16(B_UNBUF16(&buf[4]))+1;
            (*ysiz) = B_LITTLE16(B_UNBUF16(&buf[10]))-B_LITTLE16(B_UNBUF16(&buf[6]))+1;
        }
#ifdef KPCEL
        else if ((ubuf[0] == 0x19) && (ubuf[1] == 0x91) && (ubuf[10] == 8) && (ubuf[11] == 0)) //old .CEL/.PIC
        {
            (*xsiz) = (int32_t) B_LITTLE16(B_UNBUF16(&buf[2]));
            (*ysiz) = (int32_t) B_LITTLE16(B_UNBUF16(&buf[4]));
        }
#endif
#ifdef KPDDS
        else if ((B_UNBUF32(ubuf) == B_LITTLE32(0x20534444)) && (B_UNBUF32(&ubuf[4]) == B_LITTLE32(124))) //.DDS
        {
            (*xsiz) = B_LITTLE32(B_UNBUF32(&buf[16]));
            (*ysiz) = B_LITTLE32(B_UNBUF32(&buf[12]));
        }
#endif
        else if (istarga(ubuf, leng))
        {
            //Unreliable .TGA identification - this MUST be final case!
            (*xsiz) = (int32_t) B_LITTLE16(B_UNBUF16(&buf[12]));
            (*ysiz) = (int32_t) B_LITTLE16(B_UNBUF16(&buf[14]));
        }
    }
}

int32_t kprender(const char *buf, int32_t leng, intptr_t frameptr, int32_t bpl,
                 int32_t xdim, int32_t ydim)
{
    uint8_t const *ubuf = (uint8_t const *)buf;

    paleng = 0; bakcol = 0; numhufblocks = zlibcompflags = 0; filtype = -1;

    if (B_UNBUF16(&ubuf[0]) == B_LITTLE16(0x5089)) //.PNG
        return kpngrend(buf,leng,frameptr,bpl,xdim,ydim);
    else if (B_UNBUF16(&ubuf[0]) == B_LITTLE16(0xD8FFu)) //.JPG
        return kpegrend(buf,leng,frameptr,bpl,xdim,ydim);
    else
    {
        if ((ubuf[0] == 'G') && (ubuf[1] == 'I') && (ubuf[2] == 'F')) //.GIF
            return kgifrend(buf, leng, frameptr, bpl, xdim, ydim);
        else if ((ubuf[0] == 'B') && (ubuf[1] == 'M')) //.BMP
            return kbmprend(buf, leng, frameptr, bpl, xdim, ydim);
        else if (B_UNBUF32(ubuf) == B_LITTLE32(0x0801050a)) //.PCX
            return kpcxrend(buf, leng, frameptr, bpl, xdim, ydim);
#ifdef KPCEL
        else if ((ubuf[0] == 0x19) && (ubuf[1] == 0x91) && (ubuf[10] == 8) && (ubuf[11] == 0)) //old .CEL/.PIC
            return kcelrend(buf, leng, frameptr, bpl, xdim, ydim, xoff, yoff);
#endif
#ifdef KPDDS
        else if ((B_UNBUF32(ubuf) == B_LITTLE32(0x20534444)) && (B_UNBUF32(&ubuf[4]) == B_LITTLE32(124))) //.DDS
            return kddsrend(buf, leng, frameptr, bpl, xdim, ydim, xoff, yoff);
#endif
        //Unreliable .TGA identification - this MUST be final case!
        else if (istarga(ubuf, leng))
            return ktgarend(buf, leng, frameptr, bpl, xdim, ydim);
        else return -1;
    }
}

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
static WIN32_FIND_DATA findata;
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
            hfind = FindFirstFile(newildst,&findata);
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
            if (!FindNextFile(hfind,&findata))
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
//===================== HANDY PICTURE function begins ========================
#include "cache1d.h"
void kpzdecode(int32_t const leng, intptr_t * const pic, int32_t * const xsiz, int32_t * const ysiz)
{
    *pic = 0;

    kpgetdim(kpzbuf, leng, xsiz, ysiz);

    *pic = (intptr_t)Xmalloc(*ysiz * ((*xsiz)<<2));
    if (!*pic)
        return;

    if (kprender(kpzbuf, leng, *pic, ((*xsiz)<<2), *xsiz, *ysiz) < 0)
    {
        Xfree((void *) *pic);
        *pic = (intptr_t)NULL;
    }
}

void kpzload(const char * const filnam, intptr_t * const pic, int32_t * const xsiz, int32_t * const ysiz)
{
    kpzdecode(kpzbufload(filnam), pic, xsiz, ysiz);
}
//====================== HANDY PICTURE function ends =========================
