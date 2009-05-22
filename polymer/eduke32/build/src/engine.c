// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jonof@edgenetwk.com)


//#define POLYMOST
//#define SUPERBUILD
#define ENGINE

#include "compat.h"
#include "build.h"
#include "pragmas.h"
#include "cache1d.h"
#include "a.h"
#include "osd.h"
#include "crc32.h"

#include "baselayer.h"
#include "scriptfile.h"

#ifdef POLYMOST
# ifdef USE_OPENGL
#  include "glbuild.h"
#  include "mdsprite.h"
#  ifdef POLYMER
#   include "polymer.h"
#  endif
# endif
# include "hightile.h"
# include "polymost.h"
# ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
# endif
#endif

#include <math.h>

#include "engine_priv.h"

void *kmalloc(bsize_t size) { return(Bmalloc(size)); }
#define kkmalloc kmalloc

void kfree(void *buffer) { Bfree(buffer); }
#define kkfree kfree

#ifdef SUPERBUILD
void loadvoxel(int32_t voxindex) { voxindex=0; }
int32_t tiletovox[MAXTILES];
int32_t usevoxels = 1;
#define kloadvoxel loadvoxel

int32_t novoxmips = 0;
int32_t editorgridextent = 131072;

//These variables need to be copied into BUILD
#define MAXXSIZ 256
#define MAXYSIZ 256
#define MAXZSIZ 255
#define MAXVOXMIPS 5
intptr_t voxoff[MAXVOXELS][MAXVOXMIPS]; char voxlock[MAXVOXELS][MAXVOXMIPS];
int32_t voxscale[MAXVOXELS];

static int32_t ggxinc[MAXXSIZ+1], ggyinc[MAXXSIZ+1];
static int32_t lowrecip[1024], nytooclose, nytoofar;
static uint32_t distrecip[65536];
#endif

static intptr_t *lookups = NULL;
static char lookupsalloctype = 255;
int32_t dommxoverlay = 1, beforedrawrooms = 1, indrawroomsandmasks = 0;

static int32_t oxdimen = -1, oviewingrange = -1, oxyaspect = -1;

int32_t curbrightness = 0, gammabrightness = 0;

double vid_gamma = DEFAULT_GAMMA;
double vid_contrast = DEFAULT_CONTRAST;
double vid_brightness = DEFAULT_BRIGHTNESS;

//Textured Map variables
static char globalpolytype;
static int16_t *dotp1[MAXYDIM], *dotp2[MAXYDIM];

static int8_t tempbuf[MAXWALLS];

int32_t ebpbak, espbak;
int32_t slopalookup[16384];    // was 2048
#if defined(USE_OPENGL)
palette_t palookupfog[MAXPALOOKUPS];
#endif

static char permanentlock = 255;
int32_t artversion, mapversion=7L; // JBF 20040211: default mapversion to 7
void *pic = NULL;
char picsiz[MAXTILES], tilefilenum[MAXTILES];
int32_t lastageclock;
int32_t tilefileoffs[MAXTILES];

int32_t artsize = 0, cachesize = 0;

static int16_t radarang2[MAXXDIM];
static uint16_t sqrtable[4096], shlookup[4096+256];
char pow2char[8] = {1,2,4,8,16,32,64,128};
int32_t pow2long[32] =
{
    1L,2L,4L,8L,
    16L,32L,64L,128L,
    256L,512L,1024L,2048L,
    4096L,8192L,16384L,32768L,
    65536L,131072L,262144L,524288L,
    1048576L,2097152L,4194304L,8388608L,
    16777216L,33554432L,67108864L,134217728L,
    268435456L,536870912L,1073741824L,2147483647L
};
int32_t reciptable[2048], fpuasm;

char britable[16][256]; // JBF 20040207: full 8bit precision

extern char textfont[2048], smalltextfont[2048];

static char kensmessage[128];
char *engineerrstr = "No error";


#if defined(__WATCOMC__) && !defined(NOASM)

//
// Watcom Inline Assembly Routines
//

#pragma aux nsqrtasm =\
    "test eax, 0xff000000",\
    "mov ebx, eax",\
    "jnz short over24",\
    "shr ebx, 12",\
    "mov cx, word ptr shlookup[ebx*2]",\
    "jmp short under24",\
    "over24: shr ebx, 24",\
    "mov cx, word ptr shlookup[ebx*2+8192]",\
    "under24: shr eax, cl",\
    "mov cl, ch",\
    "mov ax, word ptr sqrtable[eax*2]",\
    "shr eax, cl",\
    parm nomemory [eax]\
    modify exact [eax ebx ecx]
uint32_t nsqrtasm(uint32_t);

#pragma aux msqrtasm =\
    "mov eax, 0x40000000",\
    "mov ebx, 0x20000000",\
    "begit: cmp ecx, eax",\
    "jl skip",\
    "sub ecx, eax",\
    "lea eax, [eax+ebx*4]",\
    "skip: sub eax, ebx",\
    "shr eax, 1",\
    "shr ebx, 2",\
    "jnz begit",\
    "cmp ecx, eax",\
    "sbb eax, -1",\
    "shr eax, 1",\
    parm nomemory [ecx]\
    modify exact [eax ebx ecx]
int32_t msqrtasm(uint32_t);

//0x007ff000 is (11<<13), 0x3f800000 is (127<<23)
#pragma aux krecipasm =\
    "mov fpuasm, eax",\
    "fild dword ptr fpuasm",\
    "add eax, eax",\
    "fstp dword ptr fpuasm",\
    "sbb ebx, ebx",\
    "mov eax, fpuasm",\
    "mov ecx, eax",\
    "and eax, 0x007ff000",\
    "shr eax, 10",\
    "sub ecx, 0x3f800000",\
    "shr ecx, 23",\
    "mov eax, dword ptr reciptable[eax]",\
    "sar eax, cl",\
    "xor eax, ebx",\
    parm [eax]\
    modify exact [eax ebx ecx]
int32_t krecipasm(int32_t);

#pragma aux getclipmask =\
    "sar eax, 31",\
    "add ebx, ebx",\
    "adc eax, eax",\
    "add ecx, ecx",\
    "adc eax, eax",\
    "add edx, edx",\
    "adc eax, eax",\
    "mov ebx, eax",\
    "shl ebx, 4",\
    "or al, 0xf0",\
    "xor eax, ebx",\
    parm [eax][ebx][ecx][edx]\
    modify exact [eax ebx ecx edx]
int32_t getclipmask(int32_t,int32_t,int32_t,int32_t);

#pragma aux getkensmessagecrc =\
    "xor eax, eax",\
    "mov ecx, 32",\
    "beg: mov edx, dword ptr [ebx+ecx*4-4]",\
    "ror edx, cl",\
    "adc eax, edx",\
    "bswap eax",\
    "loop short beg",\
    parm [ebx]\
    modify exact [eax ebx ecx edx]
int32_t getkensmessagecrc(int32_t);

#elif defined(_MSC_VER) && !defined(NOASM)	// __WATCOMC__

//
// Microsoft C Inline Assembly Routines
//

static inline int32_t nsqrtasm(int32_t a)
{
    _asm
    {
        push ebx
        mov eax, a
        test eax, 0xff000000
        mov ebx, eax
        jnz short over24
        shr ebx, 12
        mov cx, word ptr shlookup[ebx*2]
        jmp short under24
over24:
        shr ebx, 24
        mov cx, word ptr shlookup[ebx*2+8192]
under24:
        shr eax, cl
        mov cl, ch
        mov ax, word ptr sqrtable[eax*2]
        shr eax, cl
        pop ebx
    }
}

static inline int32_t msqrtasm(int32_t c)
{
    _asm
    {
        push ebx
        mov ecx, c
        mov eax, 0x40000000
        mov ebx, 0x20000000
begit:
        cmp ecx, eax
        jl skip
        sub ecx, eax
        lea eax, [eax+ebx*4]
skip:
        sub eax, ebx
        shr eax, 1
        shr ebx, 2
        jnz begit
        cmp ecx, eax
        sbb eax, -1
        shr eax, 1
        pop ebx
    }
}

//0x007ff000 is (11<<13), 0x3f800000 is (127<<23)
static inline int32_t krecipasm(int32_t a)
{
    _asm
    {
        push ebx
        mov eax, a
        mov fpuasm, eax
        fild dword ptr fpuasm
        add eax, eax
        fstp dword ptr fpuasm
        sbb ebx, ebx
        mov eax, fpuasm
        mov ecx, eax
        and eax, 0x007ff000
        shr eax, 10
        sub ecx, 0x3f800000
        shr ecx, 23
        mov eax, dword ptr reciptable[eax]
        sar eax, cl
        xor eax, ebx
        pop ebx
    }
}

static inline int32_t getclipmask(int32_t a, int32_t b, int32_t c, int32_t d)
{
    _asm
    {
        push ebx
        mov eax, a
        mov ebx, b
        mov ecx, c
        mov edx, d
        sar eax, 31
        add ebx, ebx
        adc eax, eax
        add ecx, ecx
        adc eax, eax
        add edx, edx
        adc eax, eax
        mov ebx, eax
        shl ebx, 4
        or al, 0xf0
        xor eax, ebx
        pop ebx
    }
}

static inline int32_t getkensmessagecrc(void *b)
{
    _asm
    {
        push ebx
        mov ebx, b
        xor eax, eax
        mov ecx, 32
beg:
        mov edx, dword ptr [ebx+ecx*4-4]
        ror edx, cl
        adc eax, edx
        bswap eax
        loop short beg
        pop ebx
    }
}

#elif defined(__GNUC__) && defined(__i386__) && !defined(NOASM)	// _MSC_VER

//
// GCC "Inline" Assembly Routines
//

#define nsqrtasm(a) \
    ({ int32_t __r, __a=(a); \
       __asm__ __volatile__ ( \
        "testl $0xff000000, %%eax\n\t" \
        "movl %%eax, %%ebx\n\t" \
        "jnz 0f\n\t" \
        "shrl $12, %%ebx\n\t" \
        "movw "ASMSYM("shlookup")"(,%%ebx,2), %%cx\n\t" \
        "jmp 1f\n\t" \
        "0:\n\t" \
        "shrl $24, %%ebx\n\t" \
        "movw ("ASMSYM("shlookup")"+8192)(,%%ebx,2), %%cx\n\t" \
        "1:\n\t" \
        "shrl %%cl, %%eax\n\t" \
        "movb %%ch, %%cl\n\t" \
        "movw "ASMSYM("sqrtable")"(,%%eax,2), %%ax\n\t" \
        "shrl %%cl, %%eax" \
        : "=a" (__r) : "a" (__a) : "ebx", "ecx", "cc"); \
     __r; })

// edx is blown by this code somehow?!
#define msqrtasm(c) \
    ({ int32_t __r, __c=(c); \
       __asm__ __volatile__ ( \
        "movl $0x40000000, %%eax\n\t" \
        "movl $0x20000000, %%ebx\n\t" \
        "0:\n\t" \
        "cmpl %%eax, %%ecx\n\t" \
        "jl 1f\n\t" \
        "subl %%eax, %%ecx\n\t" \
        "leal (%%eax,%%ebx,4), %%eax\n\t" \
        "1:\n\t" \
        "subl %%ebx, %%eax\n\t" \
        "shrl $1, %%eax\n\t" \
        "shrl $2, %%ebx\n\t" \
        "jnz 0b\n\t" \
        "cmpl %%eax, %%ecx\n\t" \
        "sbbl $-1, %%eax\n\t" \
        "shrl $1, %%eax" \
        : "=a" (__r) : "c" (__c) : "edx","ebx", "cc"); \
     __r; })

#define krecipasm(a) \
    ({ int32_t __a=(a); \
       __asm__ __volatile__ ( \
            "movl %%eax, ("ASMSYM("fpuasm")"); fildl ("ASMSYM("fpuasm")"); " \
            "addl %%eax, %%eax; fstps ("ASMSYM("fpuasm")"); sbbl %%ebx, %%ebx; " \
            "movl ("ASMSYM("fpuasm")"), %%eax; movl %%eax, %%ecx; " \
            "andl $0x007ff000, %%eax; shrl $10, %%eax; subl $0x3f800000, %%ecx; " \
            "shrl $23, %%ecx; movl "ASMSYM("reciptable")"(%%eax), %%eax; " \
            "sarl %%cl, %%eax; xorl %%ebx, %%eax" \
        : "=a" (__a) : "a" (__a) : "ebx", "ecx", "memory", "cc"); \
     __a; })

#define getclipmask(a,b,c,d) \
    ({ int32_t __a=(a), __b=(b), __c=(c), __d=(d); \
       __asm__ __volatile__ ("sarl $31, %%eax; addl %%ebx, %%ebx; adcl %%eax, %%eax; " \
                "addl %%ecx, %%ecx; adcl %%eax, %%eax; addl %%edx, %%edx; " \
                "adcl %%eax, %%eax; movl %%eax, %%ebx; shl $4, %%ebx; " \
                "orb $0xf0, %%al; xorl %%ebx, %%eax" \
        : "=a" (__a), "=b" (__b), "=c" (__c), "=d" (__d) \
        : "a" (__a), "b" (__b), "c" (__c), "d" (__d) : "cc"); \
     __a; })


#define getkensmessagecrc(b) \
    ({ int32_t __a, __b=(b); \
       __asm__ __volatile__ ( \
        "xorl %%eax, %%eax\n\t" \
        "movl $32, %%ecx\n\t" \
        "0:\n\t" \
        "movl -4(%%ebx,%%ecx,4), %%edx\n\t" \
        "rorl %%cl, %%edx\n\t" \
        "adcl %%edx, %%eax\n\t" \
        "bswapl %%eax\n\t" \
        "loop 0b" \
        : "=a" (__a) : "b" (__b) : "ecx", "edx" \
     __a; })

#else   // __GNUC__ && __i386__

static inline uint32_t nsqrtasm(uint32_t a)
{
    // JBF 20030901: This was a damn lot simpler to reverse engineer than
    // msqrtasm was. Really, it was just like simplifying an algebra equation.
    uint16_t c;

    if (a & 0xff000000)  			// test eax, 0xff000000  /  jnz short over24
    {
        c = shlookup[(a >> 24) + 4096];	// mov ebx, eax
        // over24: shr ebx, 24
        // mov cx, word ptr shlookup[ebx*2+8192]
    }
    else
    {
        c = shlookup[a >> 12];		// mov ebx, eax
        // shr ebx, 12
        // mov cx, word ptr shlookup[ebx*2]
        // jmp short under24
    }
    a >>= c&0xff;				// under24: shr eax, cl
    a = (a&0xffff0000)|(sqrtable[a]);	// mov ax, word ptr sqrtable[eax*2]
    a >>= ((c&0xff00) >> 8);		// mov cl, ch
    // shr eax, cl
    return a;
}

static inline int32_t msqrtasm(uint32_t c)
{
    uint32_t a,b;

    a = 0x40000000l;		// mov eax, 0x40000000
    b = 0x20000000l;		// mov ebx, 0x20000000
    do  				// begit:
    {
        if (c >= a)  		// cmp ecx, eax	 /  jl skip
        {
            c -= a;		// sub ecx, eax
            a += b*4;	// lea eax, [eax+ebx*4]
        }			// skip:
        a -= b;			// sub eax, ebx
        a >>= 1;		// shr eax, 1
        b >>= 2;		// shr ebx, 2
    }
    while (b);			// jnz begit
    if (c >= a)			// cmp ecx, eax
        a++;			// sbb eax, -1
    a >>= 1;			// shr eax, 1
    return a;
}

static inline int32_t krecipasm(int32_t i)
{
    // Ken did this
    float f = (float)i; i = *(int32_t *)&f;
    return((reciptable[(i>>12)&2047]>>(((i-0x3f800000)>>23)&31))^(i>>31));
}


static inline int32_t getclipmask(int32_t a, int32_t b, int32_t c, int32_t d)
{
    // Ken did this
    d = ((a<0)*8) + ((b<0)*4) + ((c<0)*2) + (d<0);
    return(((d<<4)^0xf0)|d);
}

inline int32_t getkensmessagecrc(int32_t b)
{
    return 0x56c764d4l;
    b=b;
}

#endif


int32_t xb1[MAXWALLSB], yb1[MAXWALLSB], xb2[MAXWALLSB], yb2[MAXWALLSB];
int32_t rx1[MAXWALLSB], ry1[MAXWALLSB], rx2[MAXWALLSB], ry2[MAXWALLSB];
int16_t p2[MAXWALLSB], thesector[MAXWALLSB];
int16_t thewall[MAXWALLSB];

int16_t bunchfirst[MAXWALLSB], bunchlast[MAXWALLSB];

static int16_t smost[MAXYSAVES], smostcnt;
static int16_t smoststart[MAXWALLSB];
static char smostwalltype[MAXWALLSB];
static int32_t smostwall[MAXWALLSB], smostwallcnt = -1L;

int16_t maskwall[MAXWALLSB], maskwallcnt;
static int32_t spritesx[MAXSPRITESONSCREEN];
static int32_t spritesy[MAXSPRITESONSCREEN+1];
static int32_t spritesz[MAXSPRITESONSCREEN];
spritetype *tspriteptr[MAXSPRITESONSCREEN];

int16_t umost[MAXXDIM], dmost[MAXXDIM];
static int16_t bakumost[MAXXDIM], bakdmost[MAXXDIM];
int16_t uplc[MAXXDIM], dplc[MAXXDIM];
static int16_t uwall[MAXXDIM], dwall[MAXXDIM];
static int32_t swplc[MAXXDIM], lplc[MAXXDIM];
static int32_t swall[MAXXDIM], lwall[MAXXDIM+4];
int32_t xdimen = -1, xdimenrecip, halfxdimen, xdimenscale, xdimscale;
int32_t wx1, wy1, wx2, wy2, ydimen;
intptr_t /*viewoffset,*/ frameoffset;

static int32_t nrx1[8], nry1[8], nrx2[8], nry2[8]; // JBF 20031206: Thanks Ken

static int32_t rxi[8], ryi[8], rzi[8], rxi2[8], ryi2[8], rzi2[8];
static int32_t xsi[8], ysi[8], horizycent;
static intptr_t *horizlookup=0, *horizlookup2=0;

int32_t globalposx, globalposy, globalposz, globalhoriz;
int16_t globalang, globalcursectnum;
int32_t globalpal, cosglobalang, singlobalang;
int32_t cosviewingrangeglobalang, sinviewingrangeglobalang;
char *globalpalwritten;
int32_t globaluclip, globaldclip, globvis;
int32_t globalvisibility, globalhisibility, globalpisibility, globalcisibility;
char globparaceilclip, globparaflorclip;

int32_t xyaspect, viewingrangerecip;

intptr_t asm1, asm2, asm3, asm4;
int32_t vplce[4], vince[4], palookupoffse[4], bufplce[4];
char globalxshift, globalyshift;
int32_t globalxpanning, globalypanning, globalshade;
int16_t globalpicnum, globalshiftval;
int32_t globalzd, globalyscale, globalorientation;
intptr_t globalbufplc;
int32_t globalx1, globaly1, globalx2, globaly2, globalx3, globaly3, globalzx;
int32_t globalx, globaly, globalz;

int16_t sectorborder[256], sectorbordercnt;
static char tablesloaded = 0;
int32_t pageoffset, ydim16, qsetmode = 0;
int32_t startposx, startposy, startposz;
int16_t startang, startsectnum;
int16_t pointhighlight, linehighlight, highlightcnt;
int32_t lastx[MAXYDIM];
char *transluc = NULL, paletteloaded = 0;

int32_t halfxdim16, midydim16;

#define FASTPALGRIDSIZ 8
static int32_t rdist[129], gdist[129], bdist[129];
static char colhere[((FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2))>>3];
static char colhead[(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)];
static int32_t colnext[256];
static char coldist[8] = {0,1,2,3,4,3,2,1};
static int32_t colscan[27];

static int16_t clipnum, hitwalls[4];
const int32_t hitscangoalx = (1<<29)-1, hitscangoaly = (1<<29)-1;
#ifdef POLYMOST
int32_t hitallsprites = 0;
#endif

typedef struct { int32_t x1, y1, x2, y2; } linetype;
static linetype clipit[MAXCLIPNUM];
static int16_t clipsectorlist[MAXCLIPNUM], clipsectnum;
static int16_t clipobjectval[MAXCLIPNUM];

typedef struct
{
    int32_t sx, sy, z;
    int16_t a, picnum;
    int8_t dashade;
    char dapalnum, dastat, pagesleft;
    int32_t cx1, cy1, cx2, cy2;
    int32_t uniqid;    //JF extension
} permfifotype;
static permfifotype permfifo[MAXPERMS];
static int32_t permhead = 0, permtail = 0;

int16_t numscans, numhits, numbunches;
int16_t capturecount = 0;

char vgapal16[4*256] =
{
    00,00,00,00, 42,00,00,00, 00,42,00,00, 42,42,00,00, 00,00,42,00,
    42,00,42,00, 00,21,42,00, 42,42,42,00, 21,21,21,00, 63,21,21,00,
    21,63,21,00, 63,63,21,00, 21,21,63,00, 63,21,63,00, 21,63,63,00,
    63,63,63,00
};

int16_t editstatus = 0;
int16_t searchit;
int32_t searchx = -1, searchy;                          //search input
int16_t searchsector, searchwall, searchstat;     //search output
double msens = 1.0;

static char artfilename[20];
static int32_t numtilefiles, artfil = -1, artfilnum, artfilplc;

char inpreparemirror = 0;
static int32_t mirrorsx1, mirrorsy1, mirrorsx2, mirrorsy2;

static int32_t setviewcnt = 0; // interface layers use this now
static int32_t bakframeplace[4], bakxsiz[4], bakysiz[4];
static int32_t bakwindowx1[4], bakwindowy1[4];
static int32_t bakwindowx2[4], bakwindowy2[4];
#ifdef POLYMOST
static int32_t bakrendmode,baktile;
#endif

int32_t totalclocklock;

char apptitle[256] = "Build Engine";
palette_t curpalette[256];			// the current palette, unadjusted for brightness or tint
palette_t curpalettefaded[256];		// the current palette, adjusted for brightness and tint (ie. what gets sent to the card)
palette_t palfadergb = { 0,0,0,0 };
char palfadedelta = 0;



//
// Internal Engine Functions
//
//int32_t cacheresets = 0,cacheinvalidates = 0;


//
// getpalookup (internal)
//
static inline int32_t getpalookup(int32_t davis, int32_t dashade)
{
    return(min(max(dashade+(davis>>8),0),numpalookups-1));
}


//
// scansector (internal)
//
static void scansector(int16_t sectnum)
{
    walltype *wal, *wal2;
    spritetype *spr;
    int32_t xs, ys, x1, y1, x2, y2, xp1, yp1, xp2=0, yp2=0, tempint;
    int16_t z, zz, startwall, endwall, numscansbefore, scanfirst, bunchfrst;
    int16_t nextsectnum;

    if (sectnum < 0) return;

    if (automapping) show2dsector[sectnum>>3] |= pow2char[sectnum&7];

    sectorborder[0] = sectnum, sectorbordercnt = 1;
    do
    {
        sectnum = sectorborder[--sectorbordercnt];

        for (z=headspritesect[sectnum]; z>=0; z=nextspritesect[z])
        {
            spr = &sprite[z];
            if ((((spr->cstat&0x8000) == 0) || (showinvisibility)) &&
                    (spr->xrepeat > 0) && (spr->yrepeat > 0) &&
                    (spritesortcnt < MAXSPRITESONSCREEN))
            {
                xs = spr->x-globalposx; ys = spr->y-globalposy;
                if ((spr->cstat&48) || (xs*cosglobalang+ys*singlobalang > 0))
                {
                    copybufbyte(spr,&tsprite[spritesortcnt],sizeof(spritetype));
                    spriteext[z].tspr = (spritetype *)&tsprite[spritesortcnt];
                    tsprite[spritesortcnt++].owner = z;
                }
            }
        }

        gotsector[sectnum>>3] |= pow2char[sectnum&7];

        bunchfrst = numbunches;
        numscansbefore = numscans;

        startwall = sector[sectnum].wallptr;
        endwall = startwall + sector[sectnum].wallnum;
        scanfirst = numscans;
        for (z=startwall,wal=&wall[z]; z<endwall; z++,wal++)
        {
            nextsectnum = wal->nextsector;

            wal2 = &wall[wal->point2];
            x1 = wal->x-globalposx; y1 = wal->y-globalposy;
            x2 = wal2->x-globalposx; y2 = wal2->y-globalposy;

            if ((nextsectnum >= 0) && ((wal->cstat&32) == 0))
                if ((gotsector[nextsectnum>>3]&pow2char[nextsectnum&7]) == 0)
                {
                    tempint = x1*y2-x2*y1;
                    if (((unsigned)tempint+262144) < 524288)
                        if (mulscale5(tempint,tempint) <= (x2-x1)*(x2-x1)+(y2-y1)*(y2-y1))
                            sectorborder[sectorbordercnt++] = nextsectnum;
                }

            if ((z == startwall) || (wall[z-1].point2 != z))
            {
                xp1 = dmulscale6(y1,cosglobalang,-x1,singlobalang);
                yp1 = dmulscale6(x1,cosviewingrangeglobalang,y1,sinviewingrangeglobalang);
            }
            else
            {
                xp1 = xp2;
                yp1 = yp2;
            }
            xp2 = dmulscale6(y2,cosglobalang,-x2,singlobalang);
            yp2 = dmulscale6(x2,cosviewingrangeglobalang,y2,sinviewingrangeglobalang);
            if ((yp1 < 256) && (yp2 < 256)) goto skipitaddwall;

            //If wall's NOT facing you
            if (dmulscale32(xp1,yp2,-xp2,yp1) >= 0) goto skipitaddwall;

            if (xp1 >= -yp1)
            {
                if ((xp1 > yp1) || (yp1 == 0)) goto skipitaddwall;
                xb1[numscans] = halfxdimen + scale(xp1,halfxdimen,yp1);
                if (xp1 >= 0) xb1[numscans]++;   //Fix for SIGNED divide
                if (xb1[numscans] >= xdimen) xb1[numscans] = xdimen-1;
                yb1[numscans] = yp1;
            }
            else
            {
                if (xp2 < -yp2) goto skipitaddwall;
                xb1[numscans] = 0;
                tempint = yp1-yp2+xp1-xp2;
                if (tempint == 0) goto skipitaddwall;
                yb1[numscans] = yp1 + scale(yp2-yp1,xp1+yp1,tempint);
            }
            if (yb1[numscans] < 256) goto skipitaddwall;

            if (xp2 <= yp2)
            {
                if ((xp2 < -yp2) || (yp2 == 0)) goto skipitaddwall;
                xb2[numscans] = halfxdimen + scale(xp2,halfxdimen,yp2) - 1;
                if (xp2 >= 0) xb2[numscans]++;   //Fix for SIGNED divide
                if (xb2[numscans] >= xdimen) xb2[numscans] = xdimen-1;
                yb2[numscans] = yp2;
            }
            else
            {
                if (xp1 > yp1) goto skipitaddwall;
                xb2[numscans] = xdimen-1;
                tempint = xp2-xp1+yp1-yp2;
                if (tempint == 0) goto skipitaddwall;
                yb2[numscans] = yp1 + scale(yp2-yp1,yp1-xp1,tempint);
            }
            if ((yb2[numscans] < 256) || (xb1[numscans] > xb2[numscans])) goto skipitaddwall;

            //Made it all the way!
            thesector[numscans] = sectnum; thewall[numscans] = z;
            rx1[numscans] = xp1; ry1[numscans] = yp1;
            rx2[numscans] = xp2; ry2[numscans] = yp2;
            p2[numscans] = numscans+1;
            numscans++;
skipitaddwall:

            if ((wall[z].point2 < z) && (scanfirst < numscans))
                p2[numscans-1] = scanfirst, scanfirst = numscans;
        }

        for (z=numscansbefore; z<numscans; z++)
            if ((wall[thewall[z]].point2 != thewall[p2[z]]) || (xb2[z] >= xb1[p2[z]]))
                bunchfirst[numbunches++] = p2[z], p2[z] = -1;

        for (z=bunchfrst; z<numbunches; z++)
        {
            for (zz=bunchfirst[z]; p2[zz]>=0; zz=p2[zz]);
            bunchlast[z] = zz;
        }
    }
    while (sectorbordercnt > 0);
}


//
// maskwallscan (internal)
//
static void maskwallscan(int32_t x1, int32_t x2, int16_t *uwal, int16_t *dwal, int32_t *swal, int32_t *lwal)
{
    int32_t x,/* startx,*/ xnice, ynice, fpalookup;
    intptr_t startx, p;
    int32_t y1ve[4], y2ve[4], /* p,*/ tsizx, tsizy;
#ifndef ENGINE_USING_A_C
    char bad;
    int32_t i, u4, d4, dax, z;
#endif

    tsizx = tilesizx[globalpicnum];
    tsizy = tilesizy[globalpicnum];
    setgotpic(globalpicnum);
    if ((tsizx <= 0) || (tsizy <= 0)) return;
    if ((uwal[x1] > ydimen) && (uwal[x2] > ydimen)) return;
    if ((dwal[x1] < 0) && (dwal[x2] < 0)) return;

    if (waloff[globalpicnum] == 0) loadtile(globalpicnum);

    startx = x1;

    xnice = (pow2long[picsiz[globalpicnum]&15] == tsizx);
    if (xnice) tsizx = (tsizx-1);
    ynice = (pow2long[picsiz[globalpicnum]>>4] == tsizy);
    if (ynice) tsizy = (picsiz[globalpicnum]>>4);

    if (palookup[globalpal] == NULL)
        globalpal = 0;

    fpalookup = FP_OFF(palookup[globalpal]);

    setupmvlineasm(globalshiftval);

#ifndef ENGINE_USING_A_C

    x = startx;
    while ((startumost[x+windowx1] > startdmost[x+windowx1]) && (x <= x2)) x++;

    p = x+frameoffset;

    for (; (x<=x2)&&(p&3); x++,p++)
    {
        y1ve[0] = max(uwal[x],startumost[x+windowx1]-windowy1);
        y2ve[0] = min(dwal[x],startdmost[x+windowx1]-windowy1);
        if (y2ve[0] <= y1ve[0]) continue;

        palookupoffse[0] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);

        bufplce[0] = lwal[x] + globalxpanning;
        if (bufplce[0] >= tsizx) { if (xnice == 0) bufplce[0] %= tsizx; else bufplce[0] &= tsizx; }
        if (ynice == 0) bufplce[0] *= tsizy; else bufplce[0] <<= tsizy;

        vince[0] = swal[x]*globalyscale;
        vplce[0] = globalzd + vince[0]*(y1ve[0]-globalhoriz+1);

        mvlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0]-1,vplce[0],bufplce[0]+waloff[globalpicnum],p+ylookup[y1ve[0]]);
    }
    for (; x<=x2-3; x+=4,p+=4)
    {
        bad = 0;
        for (z=3,dax=x+3; z>=0; z--,dax--)
        {
            y1ve[z] = max(uwal[dax],startumost[dax+windowx1]-windowy1);
            y2ve[z] = min(dwal[dax],startdmost[dax+windowx1]-windowy1)-1;
            if (y2ve[z] < y1ve[z]) { bad += pow2char[z]; continue; }

            i = lwal[dax] + globalxpanning;
            if (i >= tsizx) { if (xnice == 0) i %= tsizx; else i &= tsizx; }
            if (ynice == 0) i *= tsizy; else i <<= tsizy;
            bufplce[z] = waloff[globalpicnum]+i;

            vince[z] = swal[dax]*globalyscale;
            vplce[z] = globalzd + vince[z]*(y1ve[z]-globalhoriz+1);
        }
        if (bad == 15) continue;

        palookupoffse[0] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);
        palookupoffse[3] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x+3],globvis),globalshade)<<8);

        if ((palookupoffse[0] == palookupoffse[3]) && ((bad&0x9) == 0))
        {
            palookupoffse[1] = palookupoffse[0];
            palookupoffse[2] = palookupoffse[0];
        }
        else
        {
            palookupoffse[1] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x+1],globvis),globalshade)<<8);
            palookupoffse[2] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x+2],globvis),globalshade)<<8);
        }

        u4 = max(max(y1ve[0],y1ve[1]),max(y1ve[2],y1ve[3]));
        d4 = min(min(y2ve[0],y2ve[1]),min(y2ve[2],y2ve[3]));

        if ((bad > 0) || (u4 >= d4))
        {
            if (!(bad&1)) mvlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0],vplce[0],bufplce[0],ylookup[y1ve[0]]+p+0);
            if (!(bad&2)) mvlineasm1(vince[1],palookupoffse[1],y2ve[1]-y1ve[1],vplce[1],bufplce[1],ylookup[y1ve[1]]+p+1);
            if (!(bad&4)) mvlineasm1(vince[2],palookupoffse[2],y2ve[2]-y1ve[2],vplce[2],bufplce[2],ylookup[y1ve[2]]+p+2);
            if (!(bad&8)) mvlineasm1(vince[3],palookupoffse[3],y2ve[3]-y1ve[3],vplce[3],bufplce[3],ylookup[y1ve[3]]+p+3);
            continue;
        }

        if (u4 > y1ve[0]) vplce[0] = mvlineasm1(vince[0],palookupoffse[0],u4-y1ve[0]-1,vplce[0],bufplce[0],ylookup[y1ve[0]]+p+0);
        if (u4 > y1ve[1]) vplce[1] = mvlineasm1(vince[1],palookupoffse[1],u4-y1ve[1]-1,vplce[1],bufplce[1],ylookup[y1ve[1]]+p+1);
        if (u4 > y1ve[2]) vplce[2] = mvlineasm1(vince[2],palookupoffse[2],u4-y1ve[2]-1,vplce[2],bufplce[2],ylookup[y1ve[2]]+p+2);
        if (u4 > y1ve[3]) vplce[3] = mvlineasm1(vince[3],palookupoffse[3],u4-y1ve[3]-1,vplce[3],bufplce[3],ylookup[y1ve[3]]+p+3);

        if (d4 >= u4) mvlineasm4(d4-u4+1,ylookup[u4]+p);

        i = p+ylookup[d4+1];
        if (y2ve[0] > d4) mvlineasm1(vince[0],palookupoffse[0],y2ve[0]-d4-1,vplce[0],bufplce[0],i+0);
        if (y2ve[1] > d4) mvlineasm1(vince[1],palookupoffse[1],y2ve[1]-d4-1,vplce[1],bufplce[1],i+1);
        if (y2ve[2] > d4) mvlineasm1(vince[2],palookupoffse[2],y2ve[2]-d4-1,vplce[2],bufplce[2],i+2);
        if (y2ve[3] > d4) mvlineasm1(vince[3],palookupoffse[3],y2ve[3]-d4-1,vplce[3],bufplce[3],i+3);
    }
    for (; x<=x2; x++,p++)
    {
        y1ve[0] = max(uwal[x],startumost[x+windowx1]-windowy1);
        y2ve[0] = min(dwal[x],startdmost[x+windowx1]-windowy1);
        if (y2ve[0] <= y1ve[0]) continue;

        palookupoffse[0] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);

        bufplce[0] = lwal[x] + globalxpanning;
        if (bufplce[0] >= tsizx) { if (xnice == 0) bufplce[0] %= tsizx; else bufplce[0] &= tsizx; }
        if (ynice == 0) bufplce[0] *= tsizy; else bufplce[0] <<= tsizy;

        vince[0] = swal[x]*globalyscale;
        vplce[0] = globalzd + vince[0]*(y1ve[0]-globalhoriz+1);

        mvlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0]-1,vplce[0],bufplce[0]+waloff[globalpicnum],p+ylookup[y1ve[0]]);
    }

#else   // ENGINE_USING_A_C

    p = startx+frameoffset;
    for (x=startx; x<=x2; x++,p++)
    {
        y1ve[0] = max(uwal[x],startumost[x+windowx1]-windowy1);
        y2ve[0] = min(dwal[x],startdmost[x+windowx1]-windowy1);
        if (y2ve[0] <= y1ve[0]) continue;

        palookupoffse[0] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);

        bufplce[0] = lwal[x] + globalxpanning;
        if (bufplce[0] >= tsizx) { if (xnice == 0) bufplce[0] %= tsizx; else bufplce[0] &= tsizx; }
        if (ynice == 0) bufplce[0] *= tsizy; else bufplce[0] <<= tsizy;

        vince[0] = swal[x]*globalyscale;
        vplce[0] = globalzd + vince[0]*(y1ve[0]-globalhoriz+1);

        mvlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0]-1,vplce[0],bufplce[0]+waloff[globalpicnum],p+ylookup[y1ve[0]]);
    }

#endif

    faketimerhandler();
}


//
// wallfront (internal)
//
int32_t wallfront(int32_t l1, int32_t l2)
{
    walltype *wal;
    int32_t x11, y11, x21, y21, x12, y12, x22, y22, dx, dy, t1, t2;

    wal = &wall[thewall[l1]]; x11 = wal->x; y11 = wal->y;
    wal = &wall[wal->point2]; x21 = wal->x; y21 = wal->y;
    wal = &wall[thewall[l2]]; x12 = wal->x; y12 = wal->y;
    wal = &wall[wal->point2]; x22 = wal->x; y22 = wal->y;

    dx = x21-x11; dy = y21-y11;
    t1 = dmulscale2(x12-x11,dy,-dx,y12-y11); //p1(l2) vs. l1
    t2 = dmulscale2(x22-x11,dy,-dx,y22-y11); //p2(l2) vs. l1
    if (t1 == 0) { t1 = t2; if (t1 == 0) return(-1); }
    if (t2 == 0) t2 = t1;
    if ((t1^t2) >= 0)
    {
        t2 = dmulscale2(globalposx-x11,dy,-dx,globalposy-y11); //pos vs. l1
        return((t2^t1) >= 0);
    }

    dx = x22-x12; dy = y22-y12;
    t1 = dmulscale2(x11-x12,dy,-dx,y11-y12); //p1(l1) vs. l2
    t2 = dmulscale2(x21-x12,dy,-dx,y21-y12); //p2(l1) vs. l2
    if (t1 == 0) { t1 = t2; if (t1 == 0) return(-1); }
    if (t2 == 0) t2 = t1;
    if ((t1^t2) >= 0)
    {
        t2 = dmulscale2(globalposx-x12,dy,-dx,globalposy-y12); //pos vs. l2
        return((t2^t1) < 0);
    }
    return(-2);
}


//
// spritewallfront (internal)
//
static inline int32_t spritewallfront(spritetype *s, int32_t w)
{
    walltype *wal;
    int32_t x1, y1;

    wal = &wall[w]; x1 = wal->x; y1 = wal->y;
    wal = &wall[wal->point2];
    return (dmulscale32(wal->x-x1,s->y-y1,-(s->x-x1),wal->y-y1) >= 0);
}

//
//  spritebehindwall(internal)
//
#if 0
static int32_t spriteobstructswall(spritetype *s, int32_t w)
{
    walltype *wal;
    int32_t x, y;
    int32_t x1, y1;
    int32_t x2, y2;
    double a1, b1, c1;
    double a2, b2, c2;
    double d1, d2;

    // wall line equation
    wal = &wall[w]; x1 = wal->x - globalposx; y1 = wal->y - globalposy;
    wal = &wall[wal->point2]; x2 = wal->x - globalposx; y2 = wal->y - globalposy;
    if ((x2 - x1) != 0)
        a1 = (float)(y2 - y1)/(x2 - x1);
    else
        a1 = 1e+37; // not infinite, but almost ;)
    b1 = -1;
    c1 = (y1 - (a1 * x1));

    // player to sprite line equation
    if ((s->x - globalposx) != 0)
        a2 = (float)(s->y - globalposy)/(s->x - globalposx);
    else
        a2 = 1e+37;
    b2 = -1;
    c2 = 0;

    // intersection point
    d1 = (float)(1) / (a1*b2 - a2*b1);
    x = ((b1*c2 - b2*c1) * d1);
    y = ((a2*c1 - a1*c2) * d1);

    // distance between the sprite and the player
    a1 = s->x - globalposx;
    b1 = s->y - globalposy;
    d1 = (a1 * a1 + b1 * b1);

    // distance between the intersection point and the player
    d2 = (x * x + y * y);

    // check if the sprite obstructs the wall
    if ((d1 < d2) && (min(x1, x2) <= x) && (x <= max(x1, x2)) && (min(y1, y2) <= y) && (y <= max(y1, y2)))
        return (1);
    else
        return (0);
}
#endif
//
// bunchfront (internal)
//
static inline int32_t bunchfront(int32_t b1, int32_t b2)
{
    int32_t x1b1, x2b1, x1b2, x2b2, b1f, b2f, i;

    b1f = bunchfirst[b1]; x1b1 = xb1[b1f]; x2b2 = xb2[bunchlast[b2]]+1;
    if (x1b1 >= x2b2) return(-1);
    b2f = bunchfirst[b2]; x1b2 = xb1[b2f]; x2b1 = xb2[bunchlast[b1]]+1;
    if (x1b2 >= x2b1) return(-1);

    if (x1b1 >= x1b2)
    {
        for (i=b2f; xb2[i]<x1b1; i=p2[i]);
        return(wallfront(b1f,i));
    }
    for (i=b1f; xb2[i]<x1b2; i=p2[i]);
    return(wallfront(i,b2f));
}


//
// hline (internal)
//
static inline void hline(int32_t xr, int32_t yp)
{
    int32_t xl, r, s;

    xl = lastx[yp]; if (xl > xr) return;
    r = horizlookup2[yp-globalhoriz+horizycent];
    asm1 = globalx1*r;
    asm2 = globaly2*r;
    s = ((int32_t)getpalookup((int32_t)mulscale16(r,globvis),globalshade)<<8);

    hlineasm4(xr-xl,0,s,globalx2*r+globalypanning,globaly1*r+globalxpanning,
              ylookup[yp]+xr+frameoffset);
}


//
// slowhline (internal)
//
static inline void slowhline(int32_t xr, int32_t yp)
{
    int32_t xl, r;

    xl = lastx[yp]; if (xl > xr) return;
    r = horizlookup2[yp-globalhoriz+horizycent];
    asm1 = globalx1*r;
    asm2 = globaly2*r;

    asm3 = (intptr_t)globalpalwritten + ((intptr_t)getpalookup((int32_t)mulscale16(r,globvis),globalshade)<<8);
    if (!(globalorientation&256))
    {
        mhline(globalbufplc,globaly1*r+globalxpanning-asm1*(xr-xl),(xr-xl)<<16,0L,
               globalx2*r+globalypanning-asm2*(xr-xl),ylookup[yp]+xl+frameoffset);
        return;
    }
    thline(globalbufplc,globaly1*r+globalxpanning-asm1*(xr-xl),(xr-xl)<<16,0L,
           globalx2*r+globalypanning-asm2*(xr-xl),ylookup[yp]+xl+frameoffset);
}


//
// prepwall (internal)
//
static void prepwall(int32_t z, walltype *wal)
{
    int32_t i, l=0, ol=0, splc, sinc, x, topinc, top, botinc, bot, walxrepeat;

    walxrepeat = (wal->xrepeat<<3);

    //lwall calculation
    i = xb1[z]-halfxdimen;
    topinc = -(ry1[z]>>2);
    botinc = ((ry2[z]-ry1[z])>>8);
    top = mulscale5(rx1[z],xdimen)+mulscale2(topinc,i);
    bot = mulscale11(rx1[z]-rx2[z],xdimen)+mulscale2(botinc,i);

    splc = mulscale19(ry1[z],xdimscale);
    sinc = mulscale16(ry2[z]-ry1[z],xdimscale);

    x = xb1[z];
    if (bot != 0)
    {
        l = divscale12(top,bot);
        swall[x] = mulscale21(l,sinc)+splc;
        l *= walxrepeat;
        lwall[x] = (l>>18);
    }
    while (x+4 <= xb2[z])
    {
        top += topinc; bot += botinc;
        if (bot != 0)
        {
            ol = l; l = divscale12(top,bot);
            swall[x+4] = mulscale21(l,sinc)+splc;
            l *= walxrepeat;
            lwall[x+4] = (l>>18);
        }
        i = ((ol+l)>>1);
        lwall[x+2] = (i>>18);
        lwall[x+1] = ((ol+i)>>19);
        lwall[x+3] = ((l+i)>>19);
        swall[x+2] = ((swall[x]+swall[x+4])>>1);
        swall[x+1] = ((swall[x]+swall[x+2])>>1);
        swall[x+3] = ((swall[x+4]+swall[x+2])>>1);
        x += 4;
    }
    if (x+2 <= xb2[z])
    {
        top += (topinc>>1); bot += (botinc>>1);
        if (bot != 0)
        {
            ol = l; l = divscale12(top,bot);
            swall[x+2] = mulscale21(l,sinc)+splc;
            l *= walxrepeat;
            lwall[x+2] = (l>>18);
        }
        lwall[x+1] = ((l+ol)>>19);
        swall[x+1] = ((swall[x]+swall[x+2])>>1);
        x += 2;
    }
    if (x+1 <= xb2[z])
    {
        bot += (botinc>>2);
        if (bot != 0)
        {
            l = divscale12(top+(topinc>>2),bot);
            swall[x+1] = mulscale21(l,sinc)+splc;
            lwall[x+1] = mulscale18(l,walxrepeat);
        }
    }

    if (lwall[xb1[z]] < 0) lwall[xb1[z]] = 0;
    if ((lwall[xb2[z]] >= walxrepeat) && (walxrepeat)) lwall[xb2[z]] = walxrepeat-1;
    if (wal->cstat&8)
    {
        walxrepeat--;
        for (x=xb1[z]; x<=xb2[z]; x++) lwall[x] = walxrepeat-lwall[x];
    }
}


//
// animateoffs (internal)
//
inline int32_t animateoffs(int16_t tilenum, int16_t fakevar)
{
    int32_t i, k, offs;

    UNREFERENCED_PARAMETER(fakevar);

    offs = 0;
    i = (totalclocklock>>((picanm[tilenum]>>24)&15));
    if ((picanm[tilenum]&63) > 0)
    {
        switch (picanm[tilenum]&192)
        {
        case 64:
            k = (i%((picanm[tilenum]&63)<<1));
            if (k < (picanm[tilenum]&63))
                offs = k;
            else
                offs = (((picanm[tilenum]&63)<<1)-k);
            break;
        case 128:
            offs = (i%((picanm[tilenum]&63)+1));
            break;
        case 192:
            offs = -(i%((picanm[tilenum]&63)+1));
        }
    }
    return(offs);
}


//
// owallmost (internal)
//
static int32_t owallmost(int16_t *mostbuf, int32_t w, int32_t z)
{
    int32_t bad, inty, xcross, y, yinc;
    int32_t s1, s2, s3, s4, ix1, ix2, iy1, iy2, t;
    int32_t i;

    z <<= 7;
    s1 = mulscale20(globaluclip,yb1[w]); s2 = mulscale20(globaluclip,yb2[w]);
    s3 = mulscale20(globaldclip,yb1[w]); s4 = mulscale20(globaldclip,yb2[w]);
    bad = (z<s1)+((z<s2)<<1)+((z>s3)<<2)+((z>s4)<<3);

    ix1 = xb1[w]; iy1 = yb1[w];
    ix2 = xb2[w]; iy2 = yb2[w];

    if ((bad&3) == 3)
    {
        //clearbufbyte(&mostbuf[ix1],(ix2-ix1+1)*sizeof(mostbuf[0]),0L);
        for (i=ix1; i<=ix2; i++) mostbuf[i] = 0;
        return(bad);
    }

    if ((bad&12) == 12)
    {
        //clearbufbyte(&mostbuf[ix1],(ix2-ix1+1)*sizeof(mostbuf[0]),ydimen+(ydimen<<16));
        for (i=ix1; i<=ix2; i++) mostbuf[i] = ydimen;
        return(bad);
    }

    if (bad&3)
    {
        t = divscale30(z-s1,s2-s1);
        inty = yb1[w] + mulscale30(yb2[w]-yb1[w],t);
        xcross = xb1[w] + scale(mulscale30(yb2[w],t),xb2[w]-xb1[w],inty);

        if ((bad&3) == 2)
        {
            if (xb1[w] <= xcross) { iy2 = inty; ix2 = xcross; }
            //clearbufbyte(&mostbuf[xcross+1],(xb2[w]-xcross)*sizeof(mostbuf[0]),0L);
            for (i=xcross+1; i<=xb2[w]; i++) mostbuf[i] = 0;
        }
        else
        {
            if (xcross <= xb2[w]) { iy1 = inty; ix1 = xcross; }
            //clearbufbyte(&mostbuf[xb1[w]],(xcross-xb1[w]+1)*sizeof(mostbuf[0]),0L);
            for (i=xb1[w]; i<=xcross; i++) mostbuf[i] = 0;
        }
    }

    if (bad&12)
    {
        t = divscale30(z-s3,s4-s3);
        inty = yb1[w] + mulscale30(yb2[w]-yb1[w],t);
        xcross = xb1[w] + scale(mulscale30(yb2[w],t),xb2[w]-xb1[w],inty);

        if ((bad&12) == 8)
        {
            if (xb1[w] <= xcross) { iy2 = inty; ix2 = xcross; }
            //clearbufbyte(&mostbuf[xcross+1],(xb2[w]-xcross)*sizeof(mostbuf[0]),ydimen+(ydimen<<16));
            for (i=xcross+1; i<=xb2[w]; i++) mostbuf[i] = ydimen;
        }
        else
        {
            if (xcross <= xb2[w]) { iy1 = inty; ix1 = xcross; }
            //clearbufbyte(&mostbuf[xb1[w]],(xcross-xb1[w]+1)*sizeof(mostbuf[0]),ydimen+(ydimen<<16));
            for (i=xb1[w]; i<=xcross; i++) mostbuf[i] = ydimen;
        }
    }

    y = (scale(z,xdimenscale,iy1)<<4);
    yinc = ((scale(z,xdimenscale,iy2)<<4)-y) / (ix2-ix1+1);
    qinterpolatedown16short((intptr_t)&mostbuf[ix1],ix2-ix1+1,y+(globalhoriz<<16),yinc);

    if (mostbuf[ix1] < 0) mostbuf[ix1] = 0;
    if (mostbuf[ix1] > ydimen) mostbuf[ix1] = ydimen;
    if (mostbuf[ix2] < 0) mostbuf[ix2] = 0;
    if (mostbuf[ix2] > ydimen) mostbuf[ix2] = ydimen;

    return(bad);
}


//
// wallmost (internal)
//
int32_t wallmost(int16_t *mostbuf, int32_t w, int32_t sectnum, char dastat)
{
    int32_t bad, i, j, t, y, z, inty, intz, xcross, yinc, fw;
    int32_t x1, y1, z1, x2, y2, z2, xv, yv, dx, dy, dasqr, oz1, oz2;
    int32_t s1, s2, s3, s4, ix1, ix2, iy1, iy2;
    //char datempbuf[256];

    if (dastat == 0)
    {
        z = sector[sectnum].ceilingz-globalposz;
        if ((sector[sectnum].ceilingstat&2) == 0) return(owallmost(mostbuf,w,z));
    }
    else
    {
        z = sector[sectnum].floorz-globalposz;
        if ((sector[sectnum].floorstat&2) == 0) return(owallmost(mostbuf,w,z));
    }

    i = thewall[w];
    if (i == sector[sectnum].wallptr) return(owallmost(mostbuf,w,z));

    x1 = wall[i].x; x2 = wall[wall[i].point2].x-x1;
    y1 = wall[i].y; y2 = wall[wall[i].point2].y-y1;

    fw = sector[sectnum].wallptr; i = wall[fw].point2;
    dx = wall[i].x-wall[fw].x; dy = wall[i].y-wall[fw].y;
    dasqr = krecipasm(nsqrtasm(dx*dx+dy*dy));

    if (xb1[w] == 0)
        { xv = cosglobalang+sinviewingrangeglobalang; yv = singlobalang-cosviewingrangeglobalang; }
    else
        { xv = x1-globalposx; yv = y1-globalposy; }
    i = xv*(y1-globalposy)-yv*(x1-globalposx); j = yv*x2-xv*y2;
    if (klabs(j) > klabs(i>>3)) i = divscale28(i,j);
    if (dastat == 0)
    {
        t = mulscale15(sector[sectnum].ceilingheinum,dasqr);
        z1 = sector[sectnum].ceilingz;
    }
    else
    {
        t = mulscale15(sector[sectnum].floorheinum,dasqr);
        z1 = sector[sectnum].floorz;
    }
    z1 = dmulscale24(dx*t,mulscale20(y2,i)+((y1-wall[fw].y)<<8),
                     -dy*t,mulscale20(x2,i)+((x1-wall[fw].x)<<8))+((z1-globalposz)<<7);


    if (xb2[w] == xdimen-1)
        { xv = cosglobalang-sinviewingrangeglobalang; yv = singlobalang+cosviewingrangeglobalang; }
    else
        { xv = (x2+x1)-globalposx; yv = (y2+y1)-globalposy; }
    i = xv*(y1-globalposy)-yv*(x1-globalposx); j = yv*x2-xv*y2;
    if (klabs(j) > klabs(i>>3)) i = divscale28(i,j);
    if (dastat == 0)
    {
        t = mulscale15(sector[sectnum].ceilingheinum,dasqr);
        z2 = sector[sectnum].ceilingz;
    }
    else
    {
        t = mulscale15(sector[sectnum].floorheinum,dasqr);
        z2 = sector[sectnum].floorz;
    }
    z2 = dmulscale24(dx*t,mulscale20(y2,i)+((y1-wall[fw].y)<<8),
                     -dy*t,mulscale20(x2,i)+((x1-wall[fw].x)<<8))+((z2-globalposz)<<7);


    s1 = mulscale20(globaluclip,yb1[w]); s2 = mulscale20(globaluclip,yb2[w]);
    s3 = mulscale20(globaldclip,yb1[w]); s4 = mulscale20(globaldclip,yb2[w]);
    bad = (z1<s1)+((z2<s2)<<1)+((z1>s3)<<2)+((z2>s4)<<3);

    ix1 = xb1[w]; ix2 = xb2[w];
    iy1 = yb1[w]; iy2 = yb2[w];
    oz1 = z1; oz2 = z2;

    if ((bad&3) == 3)
    {
        //clearbufbyte(&mostbuf[ix1],(ix2-ix1+1)*sizeof(mostbuf[0]),0L);
        for (i=ix1; i<=ix2; i++) mostbuf[i] = 0;
        return(bad);
    }

    if ((bad&12) == 12)
    {
        //clearbufbyte(&mostbuf[ix1],(ix2-ix1+1)*sizeof(mostbuf[0]),ydimen+(ydimen<<16));
        for (i=ix1; i<=ix2; i++) mostbuf[i] = ydimen;
        return(bad);
    }

    if (bad&3)
    {
        //inty = intz / (globaluclip>>16)
        t = divscale30(oz1-s1,s2-s1+oz1-oz2);
        inty = yb1[w] + mulscale30(yb2[w]-yb1[w],t);
        intz = oz1 + mulscale30(oz2-oz1,t);
        xcross = xb1[w] + scale(mulscale30(yb2[w],t),xb2[w]-xb1[w],inty);

        //t = divscale30((x1<<4)-xcross*yb1[w],xcross*(yb2[w]-yb1[w])-((x2-x1)<<4));
        //inty = yb1[w] + mulscale30(yb2[w]-yb1[w],t);
        //intz = z1 + mulscale30(z2-z1,t);

        if ((bad&3) == 2)
        {
            if (xb1[w] <= xcross) { z2 = intz; iy2 = inty; ix2 = xcross; }
            //clearbufbyte(&mostbuf[xcross+1],(xb2[w]-xcross)*sizeof(mostbuf[0]),0L);
            for (i=xcross+1; i<=xb2[w]; i++) mostbuf[i] = 0;
        }
        else
        {
            if (xcross <= xb2[w]) { z1 = intz; iy1 = inty; ix1 = xcross; }
            //clearbufbyte(&mostbuf[xb1[w]],(xcross-xb1[w]+1)*sizeof(mostbuf[0]),0L);
            for (i=xb1[w]; i<=xcross; i++) mostbuf[i] = 0;
        }
    }

    if (bad&12)
    {
        //inty = intz / (globaldclip>>16)
        t = divscale30(oz1-s3,s4-s3+oz1-oz2);
        inty = yb1[w] + mulscale30(yb2[w]-yb1[w],t);
        intz = oz1 + mulscale30(oz2-oz1,t);
        xcross = xb1[w] + scale(mulscale30(yb2[w],t),xb2[w]-xb1[w],inty);

        //t = divscale30((x1<<4)-xcross*yb1[w],xcross*(yb2[w]-yb1[w])-((x2-x1)<<4));
        //inty = yb1[w] + mulscale30(yb2[w]-yb1[w],t);
        //intz = z1 + mulscale30(z2-z1,t);

        if ((bad&12) == 8)
        {
            if (xb1[w] <= xcross) { z2 = intz; iy2 = inty; ix2 = xcross; }
            //clearbufbyte(&mostbuf[xcross+1],(xb2[w]-xcross)*sizeof(mostbuf[0]),ydimen+(ydimen<<16));
            for (i=xcross+1; i<=xb2[w]; i++) mostbuf[i] = ydimen;
        }
        else
        {
            if (xcross <= xb2[w]) { z1 = intz; iy1 = inty; ix1 = xcross; }
            //clearbufbyte(&mostbuf[xb1[w]],(xcross-xb1[w]+1)*sizeof(mostbuf[0]),ydimen+(ydimen<<16));
            for (i=xb1[w]; i<=xcross; i++) mostbuf[i] = ydimen;
        }
    }

    y = (scale(z1,xdimenscale,iy1)<<4);
    yinc = ((scale(z2,xdimenscale,iy2)<<4)-y) / (ix2-ix1+1);
    qinterpolatedown16short((intptr_t)&mostbuf[ix1],ix2-ix1+1,y+(globalhoriz<<16),yinc);

    if (mostbuf[ix1] < 0) mostbuf[ix1] = 0;
    if (mostbuf[ix1] > ydimen) mostbuf[ix1] = ydimen;
    if (mostbuf[ix2] < 0) mostbuf[ix2] = 0;
    if (mostbuf[ix2] > ydimen) mostbuf[ix2] = ydimen;

    return(bad);
}


//
// ceilscan (internal)
//
static void ceilscan(int32_t x1, int32_t x2, int32_t sectnum)
{
    int32_t i, j, ox, oy, x, y1, y2, twall, bwall;
    sectortype *sec;

    sec = &sector[sectnum];
    if (palookup[sec->ceilingpal] != globalpalwritten)
    {
        globalpalwritten = palookup[sec->ceilingpal];
        if (!globalpalwritten) globalpalwritten = palookup[globalpal];  // JBF: fixes null-pointer crash
        setpalookupaddress(globalpalwritten);
    }

    globalzd = sec->ceilingz-globalposz;
    if (globalzd > 0) return;
    globalpicnum = sec->ceilingpicnum;
    if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;
    setgotpic(globalpicnum);
    if ((tilesizx[globalpicnum] <= 0) || (tilesizy[globalpicnum] <= 0)) return;
    if (picanm[globalpicnum]&192) globalpicnum += animateoffs((int16_t)globalpicnum,(int16_t)sectnum);

    if (waloff[globalpicnum] == 0) loadtile(globalpicnum);
    globalbufplc = waloff[globalpicnum];

    globalshade = (int32_t)sec->ceilingshade;
    globvis = globalcisibility;
    if (sec->visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t)(sec->visibility+16)));
    globalorientation = (int32_t)sec->ceilingstat;


    if ((globalorientation&64) == 0)
    {
        globalx1 = singlobalang; globalx2 = singlobalang;
        globaly1 = cosglobalang; globaly2 = cosglobalang;
        globalxpanning = (globalposx<<20);
        globalypanning = -(globalposy<<20);
    }
    else
    {
        j = sec->wallptr;
        ox = wall[wall[j].point2].x - wall[j].x;
        oy = wall[wall[j].point2].y - wall[j].y;
        i = nsqrtasm(ox*ox+oy*oy); if (i == 0) i = 1024; else i = 1048576/i;
        globalx1 = mulscale10(dmulscale10(ox,singlobalang,-oy,cosglobalang),i);
        globaly1 = mulscale10(dmulscale10(ox,cosglobalang,oy,singlobalang),i);
        globalx2 = -globalx1;
        globaly2 = -globaly1;

        ox = ((wall[j].x-globalposx)<<6); oy = ((wall[j].y-globalposy)<<6);
        i = dmulscale14(oy,cosglobalang,-ox,singlobalang);
        j = dmulscale14(ox,cosglobalang,oy,singlobalang);
        ox = i; oy = j;
        globalxpanning = globalx1*ox - globaly1*oy;
        globalypanning = globaly2*ox + globalx2*oy;
    }
    globalx2 = mulscale16(globalx2,viewingrangerecip);
    globaly1 = mulscale16(globaly1,viewingrangerecip);
    globalxshift = (8-(picsiz[globalpicnum]&15));
    globalyshift = (8-(picsiz[globalpicnum]>>4));
    if (globalorientation&8) { globalxshift++; globalyshift++; }

    if ((globalorientation&0x4) > 0)
    {
        i = globalxpanning; globalxpanning = globalypanning; globalypanning = i;
        i = globalx2; globalx2 = -globaly1; globaly1 = -i;
        i = globalx1; globalx1 = globaly2; globaly2 = i;
    }
    if ((globalorientation&0x10) > 0) globalx1 = -globalx1, globaly1 = -globaly1, globalxpanning = -globalxpanning;
    if ((globalorientation&0x20) > 0) globalx2 = -globalx2, globaly2 = -globaly2, globalypanning = -globalypanning;
    globalx1 <<= globalxshift; globaly1 <<= globalxshift;
    globalx2 <<= globalyshift;  globaly2 <<= globalyshift;
    globalxpanning <<= globalxshift; globalypanning <<= globalyshift;
    globalxpanning += (((int32_t)sec->ceilingxpanning)<<24);
    globalypanning += (((int32_t)sec->ceilingypanning)<<24);
    globaly1 = (-globalx1-globaly1)*halfxdimen;
    globalx2 = (globalx2-globaly2)*halfxdimen;

    sethlinesizes(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4,globalbufplc);

    globalx2 += globaly2*(x1-1);
    globaly1 += globalx1*(x1-1);
    globalx1 = mulscale16(globalx1,globalzd);
    globalx2 = mulscale16(globalx2,globalzd);
    globaly1 = mulscale16(globaly1,globalzd);
    globaly2 = mulscale16(globaly2,globalzd);
    globvis = klabs(mulscale10(globvis,globalzd));

    if (!(globalorientation&0x180))
    {
        y1 = umost[x1]; y2 = y1;
        for (x=x1; x<=x2; x++)
        {
            twall = umost[x]-1; bwall = min(uplc[x],dmost[x]);
            if (twall < bwall-1)
            {
                if (twall >= y2)
                {
                    while (y1 < y2-1) hline(x-1,++y1);
                    y1 = twall;
                }
                else
                {
                    while (y1 < twall) hline(x-1,++y1);
                    while (y1 > twall) lastx[y1--] = x;
                }
                while (y2 > bwall) hline(x-1,--y2);
                while (y2 < bwall) lastx[y2++] = x;
            }
            else
            {
                while (y1 < y2-1) hline(x-1,++y1);
                if (x == x2) { globalx2 += globaly2; globaly1 += globalx1; break; }
                y1 = umost[x+1]; y2 = y1;
            }
            globalx2 += globaly2; globaly1 += globalx1;
        }
        while (y1 < y2-1) hline(x2,++y1);
        faketimerhandler();
        return;
    }

    switch (globalorientation&0x180)
    {
    case 128:
        msethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    case 256:
        settransnormal();
        tsethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    case 384:
        settransreverse();
        tsethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    }

    y1 = umost[x1]; y2 = y1;
    for (x=x1; x<=x2; x++)
    {
        twall = umost[x]-1; bwall = min(uplc[x],dmost[x]);
        if (twall < bwall-1)
        {
            if (twall >= y2)
            {
                while (y1 < y2-1) slowhline(x-1,++y1);
                y1 = twall;
            }
            else
            {
                while (y1 < twall) slowhline(x-1,++y1);
                while (y1 > twall) lastx[y1--] = x;
            }
            while (y2 > bwall) slowhline(x-1,--y2);
            while (y2 < bwall) lastx[y2++] = x;
        }
        else
        {
            while (y1 < y2-1) slowhline(x-1,++y1);
            if (x == x2) { globalx2 += globaly2; globaly1 += globalx1; break; }
            y1 = umost[x+1]; y2 = y1;
        }
        globalx2 += globaly2; globaly1 += globalx1;
    }
    while (y1 < y2-1) slowhline(x2,++y1);
    faketimerhandler();
}


//
// florscan (internal)
//
static void florscan(int32_t x1, int32_t x2, int32_t sectnum)
{
    int32_t i, j, ox, oy, x, y1, y2, twall, bwall;
    sectortype *sec;

    sec = &sector[sectnum];
    if (palookup[sec->floorpal] != globalpalwritten)
    {
        globalpalwritten = palookup[sec->floorpal];
        if (!globalpalwritten) globalpalwritten = palookup[globalpal];  // JBF: fixes null-pointer crash
        setpalookupaddress(globalpalwritten);
    }

    globalzd = globalposz-sec->floorz;
    if (globalzd > 0) return;
    globalpicnum = sec->floorpicnum;
    if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;
    setgotpic(globalpicnum);
    if ((tilesizx[globalpicnum] <= 0) || (tilesizy[globalpicnum] <= 0)) return;
    if (picanm[globalpicnum]&192) globalpicnum += animateoffs((int16_t)globalpicnum,(int16_t)sectnum);

    if (waloff[globalpicnum] == 0) loadtile(globalpicnum);
    globalbufplc = waloff[globalpicnum];

    globalshade = (int32_t)sec->floorshade;
    globvis = globalcisibility;
    if (sec->visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t)(sec->visibility+16)));
    globalorientation = (int32_t)sec->floorstat;


    if ((globalorientation&64) == 0)
    {
        globalx1 = singlobalang; globalx2 = singlobalang;
        globaly1 = cosglobalang; globaly2 = cosglobalang;
        globalxpanning = (globalposx<<20);
        globalypanning = -(globalposy<<20);
    }
    else
    {
        j = sec->wallptr;
        ox = wall[wall[j].point2].x - wall[j].x;
        oy = wall[wall[j].point2].y - wall[j].y;
        i = nsqrtasm(ox*ox+oy*oy); if (i == 0) i = 1024; else i = 1048576/i;
        globalx1 = mulscale10(dmulscale10(ox,singlobalang,-oy,cosglobalang),i);
        globaly1 = mulscale10(dmulscale10(ox,cosglobalang,oy,singlobalang),i);
        globalx2 = -globalx1;
        globaly2 = -globaly1;

        ox = ((wall[j].x-globalposx)<<6); oy = ((wall[j].y-globalposy)<<6);
        i = dmulscale14(oy,cosglobalang,-ox,singlobalang);
        j = dmulscale14(ox,cosglobalang,oy,singlobalang);
        ox = i; oy = j;
        globalxpanning = globalx1*ox - globaly1*oy;
        globalypanning = globaly2*ox + globalx2*oy;
    }
    globalx2 = mulscale16(globalx2,viewingrangerecip);
    globaly1 = mulscale16(globaly1,viewingrangerecip);
    globalxshift = (8-(picsiz[globalpicnum]&15));
    globalyshift = (8-(picsiz[globalpicnum]>>4));
    if (globalorientation&8) { globalxshift++; globalyshift++; }

    if ((globalorientation&0x4) > 0)
    {
        i = globalxpanning; globalxpanning = globalypanning; globalypanning = i;
        i = globalx2; globalx2 = -globaly1; globaly1 = -i;
        i = globalx1; globalx1 = globaly2; globaly2 = i;
    }
    if ((globalorientation&0x10) > 0) globalx1 = -globalx1, globaly1 = -globaly1, globalxpanning = -globalxpanning;
    if ((globalorientation&0x20) > 0) globalx2 = -globalx2, globaly2 = -globaly2, globalypanning = -globalypanning;
    globalx1 <<= globalxshift; globaly1 <<= globalxshift;
    globalx2 <<= globalyshift;  globaly2 <<= globalyshift;
    globalxpanning <<= globalxshift; globalypanning <<= globalyshift;
    globalxpanning += (((int32_t)sec->floorxpanning)<<24);
    globalypanning += (((int32_t)sec->floorypanning)<<24);
    globaly1 = (-globalx1-globaly1)*halfxdimen;
    globalx2 = (globalx2-globaly2)*halfxdimen;

    sethlinesizes(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4,globalbufplc);

    globalx2 += globaly2*(x1-1);
    globaly1 += globalx1*(x1-1);
    globalx1 = mulscale16(globalx1,globalzd);
    globalx2 = mulscale16(globalx2,globalzd);
    globaly1 = mulscale16(globaly1,globalzd);
    globaly2 = mulscale16(globaly2,globalzd);
    globvis = klabs(mulscale10(globvis,globalzd));

    if (!(globalorientation&0x180))
    {
        y1 = max(dplc[x1],umost[x1]); y2 = y1;
        for (x=x1; x<=x2; x++)
        {
            twall = max(dplc[x],umost[x])-1; bwall = dmost[x];
            if (twall < bwall-1)
            {
                if (twall >= y2)
                {
                    while (y1 < y2-1) hline(x-1,++y1);
                    y1 = twall;
                }
                else
                {
                    while (y1 < twall) hline(x-1,++y1);
                    while (y1 > twall) lastx[y1--] = x;
                }
                while (y2 > bwall) hline(x-1,--y2);
                while (y2 < bwall) lastx[y2++] = x;
            }
            else
            {
                while (y1 < y2-1) hline(x-1,++y1);
                if (x == x2) { globalx2 += globaly2; globaly1 += globalx1; break; }
                y1 = max(dplc[x+1],umost[x+1]); y2 = y1;
            }
            globalx2 += globaly2; globaly1 += globalx1;
        }
        while (y1 < y2-1) hline(x2,++y1);
        faketimerhandler();
        return;
    }

    switch (globalorientation&0x180)
    {
    case 128:
        msethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    case 256:
        settransnormal();
        tsethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    case 384:
        settransreverse();
        tsethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
        break;
    }

    y1 = max(dplc[x1],umost[x1]); y2 = y1;
    for (x=x1; x<=x2; x++)
    {
        twall = max(dplc[x],umost[x])-1; bwall = dmost[x];
        if (twall < bwall-1)
        {
            if (twall >= y2)
            {
                while (y1 < y2-1) slowhline(x-1,++y1);
                y1 = twall;
            }
            else
            {
                while (y1 < twall) slowhline(x-1,++y1);
                while (y1 > twall) lastx[y1--] = x;
            }
            while (y2 > bwall) slowhline(x-1,--y2);
            while (y2 < bwall) lastx[y2++] = x;
        }
        else
        {
            while (y1 < y2-1) slowhline(x-1,++y1);
            if (x == x2) { globalx2 += globaly2; globaly1 += globalx1; break; }
            y1 = max(dplc[x+1],umost[x+1]); y2 = y1;
        }
        globalx2 += globaly2; globaly1 += globalx1;
    }
    while (y1 < y2-1) slowhline(x2,++y1);
    faketimerhandler();
}


//
// wallscan (internal)
//
static void wallscan(int32_t x1, int32_t x2, int16_t *uwal, int16_t *dwal, int32_t *swal, int32_t *lwal)
{
    int32_t x, xnice, ynice, fpalookup;
    int32_t y1ve[4], y2ve[4], tsizx, tsizy;
#ifndef ENGINE_USING_A_C
    char bad;
    int32_t i, u4, d4, z;
#endif

    if (x2 >= xdim) x2 = xdim-1;

    tsizx = tilesizx[globalpicnum];
    tsizy = tilesizy[globalpicnum];
    setgotpic(globalpicnum);
    if ((tsizx <= 0) || (tsizy <= 0)) return;
    if ((uwal[x1] > ydimen) && (uwal[x2] > ydimen)) return;
    if ((dwal[x1] < 0) && (dwal[x2] < 0)) return;

    if (waloff[globalpicnum] == 0) loadtile(globalpicnum);

    xnice = (pow2long[picsiz[globalpicnum]&15] == tsizx);
    if (xnice) tsizx--;
    ynice = (pow2long[picsiz[globalpicnum]>>4] == tsizy);
    if (ynice) tsizy = (picsiz[globalpicnum]>>4);

    fpalookup = FP_OFF(palookup[globalpal]);

    setupvlineasm(globalshiftval);

#ifndef ENGINE_USING_A_C

    x = x1;
    while ((umost[x] > dmost[x]) && (x <= x2)) x++;

    for (; (x<=x2)&&((x+frameoffset)&3); x++)
    {
        y1ve[0] = max(uwal[x],umost[x]);
        y2ve[0] = min(dwal[x],dmost[x]);
        if (y2ve[0] <= y1ve[0]) continue;

        palookupoffse[0] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);

        bufplce[0] = lwal[x] + globalxpanning;
        if (bufplce[0] >= tsizx) { if (xnice == 0) bufplce[0] %= tsizx; else bufplce[0] &= tsizx; }
        if (ynice == 0) bufplce[0] *= tsizy; else bufplce[0] <<= tsizy;

        vince[0] = swal[x]*globalyscale;
        vplce[0] = globalzd + vince[0]*(y1ve[0]-globalhoriz+1);

        vlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0]-1,vplce[0],bufplce[0]+waloff[globalpicnum],x+frameoffset+ylookup[y1ve[0]]);
    }
    for (; x<=x2-3; x+=4)
    {
        bad = 0;
        for (z=3; z>=0; z--)
        {
            y1ve[z] = max(uwal[x+z],umost[x+z]);
            y2ve[z] = min(dwal[x+z],dmost[x+z])-1;
            if (y2ve[z] < y1ve[z]) { bad += pow2char[z]; continue; }

            i = lwal[x+z] + globalxpanning;
            if (i >= tsizx) { if (xnice == 0) i %= tsizx; else i &= tsizx; }
            if (ynice == 0) i *= tsizy; else i <<= tsizy;
            bufplce[z] = waloff[globalpicnum]+i;

            vince[z] = swal[x+z]*globalyscale;
            vplce[z] = globalzd + vince[z]*(y1ve[z]-globalhoriz+1);
        }
        if (bad == 15) continue;

        palookupoffse[0] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);
        palookupoffse[3] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x+3],globvis),globalshade)<<8);

        if ((palookupoffse[0] == palookupoffse[3]) && ((bad&0x9) == 0))
        {
            palookupoffse[1] = palookupoffse[0];
            palookupoffse[2] = palookupoffse[0];
        }
        else
        {
            palookupoffse[1] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x+1],globvis),globalshade)<<8);
            palookupoffse[2] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x+2],globvis),globalshade)<<8);
        }

        u4 = max(max(y1ve[0],y1ve[1]),max(y1ve[2],y1ve[3]));
        d4 = min(min(y2ve[0],y2ve[1]),min(y2ve[2],y2ve[3]));

        if ((bad != 0) || (u4 >= d4))
        {
            if (!(bad&1)) prevlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0],vplce[0],bufplce[0],ylookup[y1ve[0]]+x+frameoffset+0);
            if (!(bad&2)) prevlineasm1(vince[1],palookupoffse[1],y2ve[1]-y1ve[1],vplce[1],bufplce[1],ylookup[y1ve[1]]+x+frameoffset+1);
            if (!(bad&4)) prevlineasm1(vince[2],palookupoffse[2],y2ve[2]-y1ve[2],vplce[2],bufplce[2],ylookup[y1ve[2]]+x+frameoffset+2);
            if (!(bad&8)) prevlineasm1(vince[3],palookupoffse[3],y2ve[3]-y1ve[3],vplce[3],bufplce[3],ylookup[y1ve[3]]+x+frameoffset+3);
            continue;
        }

        if (u4 > y1ve[0]) vplce[0] = prevlineasm1(vince[0],palookupoffse[0],u4-y1ve[0]-1,vplce[0],bufplce[0],ylookup[y1ve[0]]+x+frameoffset+0);
        if (u4 > y1ve[1]) vplce[1] = prevlineasm1(vince[1],palookupoffse[1],u4-y1ve[1]-1,vplce[1],bufplce[1],ylookup[y1ve[1]]+x+frameoffset+1);
        if (u4 > y1ve[2]) vplce[2] = prevlineasm1(vince[2],palookupoffse[2],u4-y1ve[2]-1,vplce[2],bufplce[2],ylookup[y1ve[2]]+x+frameoffset+2);
        if (u4 > y1ve[3]) vplce[3] = prevlineasm1(vince[3],palookupoffse[3],u4-y1ve[3]-1,vplce[3],bufplce[3],ylookup[y1ve[3]]+x+frameoffset+3);

        if (d4 >= u4) vlineasm4(d4-u4+1,ylookup[u4]+x+frameoffset);

        i = x+frameoffset+ylookup[d4+1];
        if (y2ve[0] > d4) prevlineasm1(vince[0],palookupoffse[0],y2ve[0]-d4-1,vplce[0],bufplce[0],i+0);
        if (y2ve[1] > d4) prevlineasm1(vince[1],palookupoffse[1],y2ve[1]-d4-1,vplce[1],bufplce[1],i+1);
        if (y2ve[2] > d4) prevlineasm1(vince[2],palookupoffse[2],y2ve[2]-d4-1,vplce[2],bufplce[2],i+2);
        if (y2ve[3] > d4) prevlineasm1(vince[3],palookupoffse[3],y2ve[3]-d4-1,vplce[3],bufplce[3],i+3);
    }
    for (; x<=x2; x++)
    {
        y1ve[0] = max(uwal[x],umost[x]);
        y2ve[0] = min(dwal[x],dmost[x]);
        if (y2ve[0] <= y1ve[0]) continue;

        palookupoffse[0] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);

        bufplce[0] = lwal[x] + globalxpanning;
        if (bufplce[0] >= tsizx) { if (xnice == 0) bufplce[0] %= tsizx; else bufplce[0] &= tsizx; }
        if (ynice == 0) bufplce[0] *= tsizy; else bufplce[0] <<= tsizy;

        vince[0] = swal[x]*globalyscale;
        vplce[0] = globalzd + vince[0]*(y1ve[0]-globalhoriz+1);

        vlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0]-1,vplce[0],bufplce[0]+waloff[globalpicnum],x+frameoffset+ylookup[y1ve[0]]);
    }

#else   // ENGINE_USING_A_C

    for (x=x1; x<=x2; x++)
    {
        y1ve[0] = max(uwal[x],umost[x]);
        y2ve[0] = min(dwal[x],dmost[x]);
        if (y2ve[0] <= y1ve[0]) continue;

        palookupoffse[0] = fpalookup+(getpalookup((int32_t)mulscale16(swal[x],globvis),globalshade)<<8);

        bufplce[0] = lwal[x] + globalxpanning;
        if (bufplce[0] >= tsizx) { if (xnice == 0) bufplce[0] %= tsizx; else bufplce[0] &= tsizx; }
        if (ynice == 0) bufplce[0] *= tsizy; else bufplce[0] <<= tsizy;

        vince[0] = swal[x]*globalyscale;
        vplce[0] = globalzd + vince[0]*(y1ve[0]-globalhoriz+1);

        vlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0]-1,vplce[0],bufplce[0]+waloff[globalpicnum],x+frameoffset+ylookup[y1ve[0]]);
    }

#endif

    faketimerhandler();
}


//
// transmaskvline (internal)
//
static void transmaskvline(int32_t x)
{
    int32_t vplc, vinc, i;
    intptr_t palookupoffs;
    intptr_t bufplc,p;
    int16_t y1v, y2v;

    if ((x < 0) || (x >= xdimen)) return;

    y1v = max(uwall[x],startumost[x+windowx1]-windowy1);
    y2v = min(dwall[x],startdmost[x+windowx1]-windowy1);
    y2v--;
    if (y2v < y1v) return;

    palookupoffs = FP_OFF(palookup[globalpal]) + (getpalookup((int32_t)mulscale16(swall[x],globvis),globalshade)<<8);

    vinc = swall[x]*globalyscale;
    vplc = globalzd + vinc*(y1v-globalhoriz+1);

    i = lwall[x]+globalxpanning;
    if (i >= tilesizx[globalpicnum]) i %= tilesizx[globalpicnum];
    bufplc = waloff[globalpicnum]+i*tilesizy[globalpicnum];

    p = ylookup[y1v]+x+frameoffset;

    tvlineasm1(vinc,palookupoffs,y2v-y1v,vplc,bufplc,p);
}


//
// transmaskvline2 (internal)
//
#ifndef ENGINE_USING_A_C
static void transmaskvline2(int32_t x)
{
    int32_t i, y1, y2, x2;
    int16_t y1ve[2], y2ve[2];

    if ((x < 0) || (x >= xdimen)) return;
    if (x == xdimen-1) { transmaskvline(x); return; }

    x2 = x+1;

    y1ve[0] = max(uwall[x],startumost[x+windowx1]-windowy1);
    y2ve[0] = min(dwall[x],startdmost[x+windowx1]-windowy1)-1;
    if (y2ve[0] < y1ve[0]) { transmaskvline(x2); return; }
    y1ve[1] = max(uwall[x2],startumost[x2+windowx1]-windowy1);
    y2ve[1] = min(dwall[x2],startdmost[x2+windowx1]-windowy1)-1;
    if (y2ve[1] < y1ve[1]) { transmaskvline(x); return; }

    palookupoffse[0] = FP_OFF(palookup[globalpal]) + (getpalookup((int32_t)mulscale16(swall[x],globvis),globalshade)<<8);
    palookupoffse[1] = FP_OFF(palookup[globalpal]) + (getpalookup((int32_t)mulscale16(swall[x2],globvis),globalshade)<<8);

    setuptvlineasm2(globalshiftval,palookupoffse[0],palookupoffse[1]);

    vince[0] = swall[x]*globalyscale;
    vince[1] = swall[x2]*globalyscale;
    vplce[0] = globalzd + vince[0]*(y1ve[0]-globalhoriz+1);
    vplce[1] = globalzd + vince[1]*(y1ve[1]-globalhoriz+1);

    i = lwall[x] + globalxpanning;
    if (i >= tilesizx[globalpicnum]) i %= tilesizx[globalpicnum];
    bufplce[0] = waloff[globalpicnum]+i*tilesizy[globalpicnum];

    i = lwall[x2] + globalxpanning;
    if (i >= tilesizx[globalpicnum]) i %= tilesizx[globalpicnum];
    bufplce[1] = waloff[globalpicnum]+i*tilesizy[globalpicnum];

    y1 = max(y1ve[0],y1ve[1]);
    y2 = min(y2ve[0],y2ve[1]);

    i = x+frameoffset;

    if (y1ve[0] != y1ve[1])
    {
        if (y1ve[0] < y1)
            vplce[0] = tvlineasm1(vince[0],palookupoffse[0],y1-y1ve[0]-1,vplce[0],bufplce[0],ylookup[y1ve[0]]+i);
        else
            vplce[1] = tvlineasm1(vince[1],palookupoffse[1],y1-y1ve[1]-1,vplce[1],bufplce[1],ylookup[y1ve[1]]+i+1);
    }

    if (y2 > y1)
    {
        asm1 = vince[1];
        asm2 = ylookup[y2]+i+1;
        tvlineasm2(vplce[1],vince[0],bufplce[0],bufplce[1],vplce[0],ylookup[y1]+i);
    }
    else
    {
        asm1 = vplce[0];
        asm2 = vplce[1];
    }

    if (y2ve[0] > y2ve[1])
        tvlineasm1(vince[0],palookupoffse[0],y2ve[0]-y2-1,asm1,bufplce[0],ylookup[y2+1]+i);
    else if (y2ve[0] < y2ve[1])
        tvlineasm1(vince[1],palookupoffse[1],y2ve[1]-y2-1,asm2,bufplce[1],ylookup[y2+1]+i+1);

    faketimerhandler();
}
#endif

//
// transmaskwallscan (internal)
//
static inline void transmaskwallscan(int32_t x1, int32_t x2)
{
    int32_t x;

    setgotpic(globalpicnum);
    if ((tilesizx[globalpicnum] <= 0) || (tilesizy[globalpicnum] <= 0)) return;

    if (waloff[globalpicnum] == 0) loadtile(globalpicnum);

    setuptvlineasm(globalshiftval);

    x = x1;
    while ((startumost[x+windowx1] > startdmost[x+windowx1]) && (x <= x2)) x++;
#ifndef ENGINE_USING_A_C
    if ((x <= x2) && (x&1)) transmaskvline(x), x++;
    while (x < x2) transmaskvline2(x), x += 2;
#endif
    while (x <= x2) transmaskvline(x), x++;
    faketimerhandler();
}


//
// ceilspritehline (internal)
//
static inline void ceilspritehline(int32_t x2, int32_t y)
{
    int32_t x1, v, bx, by;

    //x = x1 + (x2-x1)t + (y1-y2)u  ~  x = 160v
    //y = y1 + (y2-y1)t + (x2-x1)u  ~  y = (scrx-160)v
    //z = z1 = z2                   ~  z = posz + (scry-horiz)v

    x1 = lastx[y]; if (x2 < x1) return;

    v = mulscale20(globalzd,horizlookup[y-globalhoriz+horizycent]);
    bx = mulscale14(globalx2*x1+globalx1,v) + globalxpanning;
    by = mulscale14(globaly2*x1+globaly1,v) + globalypanning;
    asm1 = mulscale14(globalx2,v);
    asm2 = mulscale14(globaly2,v);

    asm3 = FP_OFF(palookup[globalpal]) + (getpalookup((int32_t)mulscale28(klabs(v),globvis),globalshade)<<8);

    if ((globalorientation&2) == 0)
        mhline(globalbufplc,bx,(x2-x1)<<16,0L,by,ylookup[y]+x1+frameoffset);
    else
    {
        thline(globalbufplc,bx,(x2-x1)<<16,0L,by,ylookup[y]+x1+frameoffset);
    }
}


//
// ceilspritescan (internal)
//
static inline void ceilspritescan(int32_t x1, int32_t x2)
{
    int32_t x, y1, y2, twall, bwall;

    y1 = uwall[x1]; y2 = y1;
    for (x=x1; x<=x2; x++)
    {
        twall = uwall[x]-1; bwall = dwall[x];
        if (twall < bwall-1)
        {
            if (twall >= y2)
            {
                while (y1 < y2-1) ceilspritehline(x-1,++y1);
                y1 = twall;
            }
            else
            {
                while (y1 < twall) ceilspritehline(x-1,++y1);
                while (y1 > twall) lastx[y1--] = x;
            }
            while (y2 > bwall) ceilspritehline(x-1,--y2);
            while (y2 < bwall) lastx[y2++] = x;
        }
        else
        {
            while (y1 < y2-1) ceilspritehline(x-1,++y1);
            if (x == x2) break;
            y1 = uwall[x+1]; y2 = y1;
        }
    }
    while (y1 < y2-1) ceilspritehline(x2,++y1);
    faketimerhandler();
}


//
// grouscan (internal)
//
#define BITSOFPRECISION 3  //Don't forget to change this in A.ASM also!
static void grouscan(int32_t dax1, int32_t dax2, int32_t sectnum, char dastat)
{
    int32_t i, j, l, x, y, dx, dy, wx, wy, y1, y2, daz;
    int32_t daslope, dasqr;
    int32_t shoffs, shinc, m1, m2, *mptr1, *mptr2, *nptr1, *nptr2;
    walltype *wal;
    sectortype *sec;

    sec = &sector[sectnum];

    if (dastat == 0)
    {
        if (globalposz <= getceilzofslope(sectnum,globalposx,globalposy))
            return;  //Back-face culling
        globalorientation = sec->ceilingstat;
        globalpicnum = sec->ceilingpicnum;
        globalshade = sec->ceilingshade;
        globalpal = sec->ceilingpal;
        daslope = sec->ceilingheinum;
        daz = sec->ceilingz;
    }
    else
    {
        if (globalposz >= getflorzofslope(sectnum,globalposx,globalposy))
            return;  //Back-face culling
        globalorientation = sec->floorstat;
        globalpicnum = sec->floorpicnum;
        globalshade = sec->floorshade;
        globalpal = sec->floorpal;
        daslope = sec->floorheinum;
        daz = sec->floorz;
    }

    if ((picanm[globalpicnum]&192) != 0) globalpicnum += animateoffs(globalpicnum,sectnum);
    setgotpic(globalpicnum);
    if ((tilesizx[globalpicnum] <= 0) || (tilesizy[globalpicnum] <= 0)) return;
    if (waloff[globalpicnum] == 0) loadtile(globalpicnum);

    wal = &wall[sec->wallptr];
    wx = wall[wal->point2].x - wal->x;
    wy = wall[wal->point2].y - wal->y;
    dasqr = krecipasm(nsqrtasm(wx*wx+wy*wy));
    i = mulscale21(daslope,dasqr);
    wx *= i; wy *= i;

    globalx = -mulscale19(singlobalang,xdimenrecip);
    globaly = mulscale19(cosglobalang,xdimenrecip);
    globalx1 = (globalposx<<8);
    globaly1 = -(globalposy<<8);
    i = (dax1-halfxdimen)*xdimenrecip;
    globalx2 = mulscale16(cosglobalang<<4,viewingrangerecip) - mulscale27(singlobalang,i);
    globaly2 = mulscale16(singlobalang<<4,viewingrangerecip) + mulscale27(cosglobalang,i);
    globalzd = (xdimscale<<9);
    globalzx = -dmulscale17(wx,globaly2,-wy,globalx2) + mulscale10(1-globalhoriz,globalzd);
    globalz = -dmulscale25(wx,globaly,-wy,globalx);

    if (globalorientation&64)  //Relative alignment
    {
        dx = mulscale14(wall[wal->point2].x-wal->x,dasqr);
        dy = mulscale14(wall[wal->point2].y-wal->y,dasqr);

        i = nsqrtasm(daslope*daslope+16777216);

        x = globalx; y = globaly;
        globalx = dmulscale16(x,dx,y,dy);
        globaly = mulscale12(dmulscale16(-y,dx,x,dy),i);

        x = ((wal->x-globalposx)<<8); y = ((wal->y-globalposy)<<8);
        globalx1 = dmulscale16(-x,dx,-y,dy);
        globaly1 = mulscale12(dmulscale16(-y,dx,x,dy),i);

        x = globalx2; y = globaly2;
        globalx2 = dmulscale16(x,dx,y,dy);
        globaly2 = mulscale12(dmulscale16(-y,dx,x,dy),i);
    }
    if (globalorientation&0x4)
    {
        i = globalx; globalx = -globaly; globaly = -i;
        i = globalx1; globalx1 = globaly1; globaly1 = i;
        i = globalx2; globalx2 = -globaly2; globaly2 = -i;
    }
    if (globalorientation&0x10) { globalx1 = -globalx1, globalx2 = -globalx2, globalx = -globalx; }
    if (globalorientation&0x20) { globaly1 = -globaly1, globaly2 = -globaly2, globaly = -globaly; }

    daz = dmulscale9(wx,globalposy-wal->y,-wy,globalposx-wal->x) + ((daz-globalposz)<<8);
    globalx2 = mulscale20(globalx2,daz); globalx = mulscale28(globalx,daz);
    globaly2 = mulscale20(globaly2,-daz); globaly = mulscale28(globaly,-daz);

    i = 8-(picsiz[globalpicnum]&15); j = 8-(picsiz[globalpicnum]>>4);
    if (globalorientation&8) { i++; j++; }
    globalx1 <<= (i+12); globalx2 <<= i; globalx <<= i;
    globaly1 <<= (j+12); globaly2 <<= j; globaly <<= j;

    if (dastat == 0)
    {
        globalx1 += (((int32_t)sec->ceilingxpanning)<<24);
        globaly1 += (((int32_t)sec->ceilingypanning)<<24);
    }
    else
    {
        globalx1 += (((int32_t)sec->floorxpanning)<<24);
        globaly1 += (((int32_t)sec->floorypanning)<<24);
    }

    asm1 = -(globalzd>>(16-BITSOFPRECISION));

    globvis = globalvisibility;
    if (sec->visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t)(sec->visibility+16)));
    globvis = mulscale13(globvis,daz);
    globvis = mulscale16(globvis,xdimscale);
    j = FP_OFF(palookup[globalpal]);

    setupslopevlin(((int32_t)(picsiz[globalpicnum]&15))+(((int32_t)(picsiz[globalpicnum]>>4))<<8),waloff[globalpicnum],-ylookup[1]);

    l = (globalzd>>16);

    shinc = mulscale16(globalz,xdimenscale);
    if (shinc > 0) shoffs = (4<<15); else shoffs = ((16380-ydimen)<<15);    // JBF: was 2044
    if (dastat == 0) y1 = umost[dax1]; else y1 = max(umost[dax1],dplc[dax1]);
    m1 = mulscale16(y1,globalzd) + (globalzx>>6);
    //Avoid visibility overflow by crossing horizon
    if (globalzd > 0) m1 += (globalzd>>16); else m1 -= (globalzd>>16);
    m2 = m1+l;
    mptr1 = (int32_t *)&slopalookup[y1+(shoffs>>15)]; mptr2 = mptr1+1;

    for (x=dax1; x<=dax2; x++)
    {
        if (dastat == 0) { y1 = umost[x]; y2 = min(dmost[x],uplc[x])-1; }
        else { y1 = max(umost[x],dplc[x]); y2 = dmost[x]-1; }
        if (y1 <= y2)
        {
            nptr1 = (int32_t *)&slopalookup[y1+(shoffs>>15)];
            nptr2 = (int32_t *)&slopalookup[y2+(shoffs>>15)];
            while (nptr1 <= mptr1)
            {
                *mptr1-- = j + (getpalookup((int32_t)mulscale24(krecipasm(m1),globvis),globalshade)<<8);
                m1 -= l;
            }
            while (nptr2 >= mptr2)
            {
                *mptr2++ = j + (getpalookup((int32_t)mulscale24(krecipasm(m2),globvis),globalshade)<<8);
                m2 += l;
            }

            globalx3 = (globalx2>>10);
            globaly3 = (globaly2>>10);
            asm3 = mulscale16(y2,globalzd) + (globalzx>>6);
            slopevlin(ylookup[y2]+x+frameoffset,krecipasm(asm3>>3),(intptr_t)nptr2,y2-y1+1,globalx1,globaly1);

            if ((x&15) == 0) faketimerhandler();
        }
        globalx2 += globalx;
        globaly2 += globaly;
        globalzx += globalz;
        shoffs += shinc;
    }
}


//
// parascan (internal)
//
static void parascan(int32_t dax1, int32_t dax2, int32_t sectnum, char dastat, int32_t bunch)
{
    sectortype *sec;
    int32_t j, k, l, m, n, x, z, wallnum, nextsectnum, globalhorizbak;
    int16_t *topptr, *botptr;

    UNREFERENCED_PARAMETER(dax1);
    UNREFERENCED_PARAMETER(dax2);

    sectnum = thesector[bunchfirst[bunch]]; sec = &sector[sectnum];

    globalhorizbak = globalhoriz;
    if (parallaxyscale != 65536)
        globalhoriz = mulscale16(globalhoriz-(ydimen>>1),parallaxyscale) + (ydimen>>1);
    globvis = globalpisibility;
    //globalorientation = 0L;
    if (sec->visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t)(sec->visibility+16)));

    if (dastat == 0)
    {
        globalpal = sec->ceilingpal;
        globalpicnum = sec->ceilingpicnum;
        globalshade = (int32_t)sec->ceilingshade;
        globalxpanning = (int32_t)sec->ceilingxpanning;
        globalypanning = (int32_t)sec->ceilingypanning;
        topptr = umost;
        botptr = uplc;
    }
    else
    {
        globalpal = sec->floorpal;
        globalpicnum = sec->floorpicnum;
        globalshade = (int32_t)sec->floorshade;
        globalxpanning = (int32_t)sec->floorxpanning;
        globalypanning = (int32_t)sec->floorypanning;
        topptr = dplc;
        botptr = dmost;
    }

    if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;
    if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,(int16_t)sectnum);
    globalshiftval = (picsiz[globalpicnum]>>4);
    if (pow2long[globalshiftval] != tilesizy[globalpicnum]) globalshiftval++;
    globalshiftval = 32-globalshiftval;
    globalzd = (((tilesizy[globalpicnum]>>1)+parallaxyoffs)<<globalshiftval)+(globalypanning<<24);
    globalyscale = (8<<(globalshiftval-19));
    //if (globalorientation&256) globalyscale = -globalyscale, globalzd = -globalzd;

    k = 11 - (picsiz[globalpicnum]&15) - pskybits;
    x = -1;

    for (z=bunchfirst[bunch]; z>=0; z=p2[z])
    {
        wallnum = thewall[z]; nextsectnum = wall[wallnum].nextsector;

        if (nextsectnum >= 0)  //else negative array access
        {
            if (dastat == 0) j = sector[nextsectnum].ceilingstat;
            else j = sector[nextsectnum].floorstat;
        }

        if ((nextsectnum < 0) || (wall[wallnum].cstat&32) || ((j&1) == 0))
        {
            if (x == -1) x = xb1[z];

            if (parallaxtype == 0)
            {
                n = mulscale16(xdimenrecip,viewingrange);
                for (j=xb1[z]; j<=xb2[z]; j++)
                    lplc[j] = (((mulscale23(j-halfxdimen,n)+globalang)&2047)>>k);
            }
            else
            {
                for (j=xb1[z]; j<=xb2[z]; j++)
                    lplc[j] = ((((int32_t)radarang2[j]+globalang)&2047)>>k);
            }
            if (parallaxtype == 2)
            {
                n = mulscale16(xdimscale,viewingrange);
                for (j=xb1[z]; j<=xb2[z]; j++)
                    swplc[j] = mulscale14(sintable[((int32_t)radarang2[j]+512)&2047],n);
            }
            else
                clearbuf(&swplc[xb1[z]],xb2[z]-xb1[z]+1,mulscale16(xdimscale,viewingrange));
        }
        else if (x >= 0)
        {
            l = globalpicnum; m = (picsiz[globalpicnum]&15);
            globalpicnum = l+pskyoff[lplc[x]>>m];

            if (((lplc[x]^lplc[xb1[z]-1])>>m) == 0)
                wallscan(x,xb1[z]-1,topptr,botptr,swplc,lplc);
            else
            {
                j = x;
                while (x < xb1[z])
                {
                    n = l+pskyoff[lplc[x]>>m];
                    if (n != globalpicnum)
                    {
                        wallscan(j,x-1,topptr,botptr,swplc,lplc);
                        j = x;
                        globalpicnum = n;
                    }
                    x++;
                }
                if (j < x)
                    wallscan(j,x-1,topptr,botptr,swplc,lplc);
            }

            globalpicnum = l;
            x = -1;
        }
    }

    if (x >= 0)
    {
        l = globalpicnum; m = (picsiz[globalpicnum]&15);
        globalpicnum = l+pskyoff[lplc[x]>>m];

        if (((lplc[x]^lplc[xb2[bunchlast[bunch]]])>>m) == 0)
            wallscan(x,xb2[bunchlast[bunch]],topptr,botptr,swplc,lplc);
        else
        {
            j = x;
            while (x <= xb2[bunchlast[bunch]])
            {
                n = l+pskyoff[lplc[x]>>m];
                if (n != globalpicnum)
                {
                    wallscan(j,x-1,topptr,botptr,swplc,lplc);
                    j = x;
                    globalpicnum = n;
                }
                x++;
            }
            if (j <= x)
                wallscan(j,x-1,topptr,botptr,swplc,lplc);
        }
        globalpicnum = l;
    }
    globalhoriz = globalhorizbak;
}


//
// drawalls (internal)
//
static void drawalls(int32_t bunch)
{
    sectortype *sec, *nextsec;
    walltype *wal;
    int32_t i, x, x1, x2, cz[5], fz[5];
    int32_t z, wallnum, sectnum, nextsectnum;
    int32_t startsmostwallcnt, startsmostcnt, gotswall;
    char andwstat1, andwstat2;

    z = bunchfirst[bunch];
    sectnum = thesector[z]; sec = &sector[sectnum];

    andwstat1 = 0xff; andwstat2 = 0xff;
    for (; z>=0; z=p2[z]) //uplc/dplc calculation
    {
        andwstat1 &= wallmost(uplc,z,sectnum,(uint8_t)0);
        andwstat2 &= wallmost(dplc,z,sectnum,(uint8_t)1);
    }

    if ((andwstat1&3) != 3)     //draw ceilings
    {
        if ((sec->ceilingstat&3) == 2)
            grouscan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum,0);
        else if ((sec->ceilingstat&1) == 0)
            ceilscan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum);
        else
            parascan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum,0,bunch);
    }
    if ((andwstat2&12) != 12)   //draw floors
    {
        if ((sec->floorstat&3) == 2)
            grouscan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum,1);
        else if ((sec->floorstat&1) == 0)
            florscan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum);
        else
            parascan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum,1,bunch);
    }

    //DRAW WALLS SECTION!
    for (z=bunchfirst[bunch]; z>=0; z=p2[z])
    {
        x1 = xb1[z]; x2 = xb2[z];
        if (umost[x2] >= dmost[x2])
        {
            for (x=x1; x<x2; x++)
                if (umost[x] < dmost[x]) break;
            if (x >= x2)
            {
                smostwall[smostwallcnt] = z;
                smostwalltype[smostwallcnt] = 0;
                smostwallcnt++;
                continue;
            }
        }

        wallnum = thewall[z]; wal = &wall[wallnum];
        nextsectnum = wal->nextsector; nextsec = &sector[nextsectnum];

        gotswall = 0;

        startsmostwallcnt = smostwallcnt;
        startsmostcnt = smostcnt;

        if ((searchit == 2) && (searchx >= x1) && (searchx <= x2))
        {
            if (searchy <= uplc[searchx]) //ceiling
            {
                searchsector = sectnum; searchwall = wallnum;
                searchstat = 1; searchit = 1;
            }
            else if (searchy >= dplc[searchx]) //floor
            {
                searchsector = sectnum; searchwall = wallnum;
                searchstat = 2; searchit = 1;
            }
        }

        if (nextsectnum >= 0)
        {
            getzsofslope((int16_t)sectnum,wal->x,wal->y,&cz[0],&fz[0]);
            getzsofslope((int16_t)sectnum,wall[wal->point2].x,wall[wal->point2].y,&cz[1],&fz[1]);
            getzsofslope((int16_t)nextsectnum,wal->x,wal->y,&cz[2],&fz[2]);
            getzsofslope((int16_t)nextsectnum,wall[wal->point2].x,wall[wal->point2].y,&cz[3],&fz[3]);
            getzsofslope((int16_t)nextsectnum,globalposx,globalposy,&cz[4],&fz[4]);

            if ((wal->cstat&48) == 16) maskwall[maskwallcnt++] = z;

            if (((sec->ceilingstat&1) == 0) || ((nextsec->ceilingstat&1) == 0))
            {
                if ((cz[2] <= cz[0]) && (cz[3] <= cz[1]))
                {
                    if (globparaceilclip)
                        for (x=x1; x<=x2; x++)
                            if (uplc[x] > umost[x])
                                if (umost[x] <= dmost[x])
                                {
                                    umost[x] = uplc[x];
                                    if (umost[x] > dmost[x]) numhits--;
                                }
                }
                else
                {
                    wallmost(dwall,z,nextsectnum,(uint8_t)0);
                    if ((cz[2] > fz[0]) || (cz[3] > fz[1]))
                        for (i=x1; i<=x2; i++) if (dwall[i] > dplc[i]) dwall[i] = dplc[i];

                    if ((searchit == 2) && (searchx >= x1) && (searchx <= x2))
                        if (searchy <= dwall[searchx]) //wall
                        {
                            searchsector = sectnum; searchwall = wallnum;
                            searchstat = 0; searchit = 1;
                        }

                    globalorientation = (int32_t)wal->cstat;
                    globalpicnum = wal->picnum;
                    if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;
                    globalxpanning = (int32_t)wal->xpanning;
                    globalypanning = (int32_t)wal->ypanning;
                    globalshiftval = (picsiz[globalpicnum]>>4);
                    if (pow2long[globalshiftval] != tilesizy[globalpicnum]) globalshiftval++;
                    globalshiftval = 32-globalshiftval;
                    if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,(int16_t)wallnum+16384);
                    globalshade = (int32_t)wal->shade;
                    globvis = globalvisibility;
                    if (sec->visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t)(sec->visibility+16)));
                    globalpal = (int32_t)wal->pal;
                    if (palookup[globalpal] == NULL) globalpal = 0;    // JBF: fixes crash
                    globalyscale = (wal->yrepeat<<(globalshiftval-19));
                    if ((globalorientation&4) == 0)
                        globalzd = (((globalposz-nextsec->ceilingz)*globalyscale)<<8);
                    else
                        globalzd = (((globalposz-sec->ceilingz)*globalyscale)<<8);
                    globalzd += (globalypanning<<24);
                    if (globalorientation&256) globalyscale = -globalyscale, globalzd = -globalzd;

                    if (gotswall == 0) { gotswall = 1; prepwall(z,wal); }
                    wallscan(x1,x2,uplc,dwall,swall,lwall);

                    if ((cz[2] >= cz[0]) && (cz[3] >= cz[1]))
                    {
                        for (x=x1; x<=x2; x++)
                            if (dwall[x] > umost[x])
                                if (umost[x] <= dmost[x])
                                {
                                    umost[x] = dwall[x];
                                    if (umost[x] > dmost[x]) numhits--;
                                }
                    }
                    else
                    {
                        for (x=x1; x<=x2; x++)
                            if (umost[x] <= dmost[x])
                            {
                                i = max(uplc[x],dwall[x]);
                                if (i > umost[x])
                                {
                                    umost[x] = i;
                                    if (umost[x] > dmost[x]) numhits--;
                                }
                            }
                    }
                }
                if ((cz[2] < cz[0]) || (cz[3] < cz[1]) || (globalposz < cz[4]))
                {
                    i = x2-x1+1;
                    if (smostcnt+i < MAXYSAVES)
                    {
                        smoststart[smostwallcnt] = smostcnt;
                        smostwall[smostwallcnt] = z;
                        smostwalltype[smostwallcnt] = 1;   //1 for umost
                        smostwallcnt++;
                        copybufbyte(&umost[x1],&smost[smostcnt],i*sizeof(smost[0]));
                        smostcnt += i;
                    }
                }
            }
            if (((sec->floorstat&1) == 0) || ((nextsec->floorstat&1) == 0))
            {
                if ((fz[2] >= fz[0]) && (fz[3] >= fz[1]))
                {
                    if (globparaflorclip)
                        for (x=x1; x<=x2; x++)
                            if (dplc[x] < dmost[x])
                                if (umost[x] <= dmost[x])
                                {
                                    dmost[x] = dplc[x];
                                    if (umost[x] > dmost[x]) numhits--;
                                }
                }
                else
                {
                    wallmost(uwall,z,nextsectnum,(uint8_t)1);
                    if ((fz[2] < cz[0]) || (fz[3] < cz[1]))
                        for (i=x1; i<=x2; i++) if (uwall[i] < uplc[i]) uwall[i] = uplc[i];

                    if ((searchit == 2) && (searchx >= x1) && (searchx <= x2))
                        if (searchy >= uwall[searchx]) //wall
                        {
                            searchsector = sectnum; searchwall = wallnum;
                            if ((wal->cstat&2) > 0) searchwall = wal->nextwall;
                            searchstat = 0; searchit = 1;
                        }

                    if ((wal->cstat&2) > 0)
                    {
                        wallnum = wal->nextwall; wal = &wall[wallnum];
                        globalorientation = (int32_t)wal->cstat;
                        globalpicnum = wal->picnum;
                        if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;
                        globalxpanning = (int32_t)wal->xpanning;
                        globalypanning = (int32_t)wal->ypanning;
                        if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,(int16_t)wallnum+16384);
                        globalshade = (int32_t)wal->shade;
                        globalpal = (int32_t)wal->pal;
                        wallnum = thewall[z]; wal = &wall[wallnum];
                    }
                    else
                    {
                        globalorientation = (int32_t)wal->cstat;
                        globalpicnum = wal->picnum;
                        if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;
                        globalxpanning = (int32_t)wal->xpanning;
                        globalypanning = (int32_t)wal->ypanning;
                        if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,(int16_t)wallnum+16384);
                        globalshade = (int32_t)wal->shade;
                        globalpal = (int32_t)wal->pal;
                    }
                    if (palookup[globalpal] == NULL) globalpal = 0;    // JBF: fixes crash
                    globvis = globalvisibility;
                    if (sec->visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t)(sec->visibility+16)));
                    globalshiftval = (picsiz[globalpicnum]>>4);
                    if (pow2long[globalshiftval] != tilesizy[globalpicnum]) globalshiftval++;
                    globalshiftval = 32-globalshiftval;
                    globalyscale = (wal->yrepeat<<(globalshiftval-19));
                    if ((globalorientation&4) == 0)
                        globalzd = (((globalposz-nextsec->floorz)*globalyscale)<<8);
                    else
                        globalzd = (((globalposz-sec->ceilingz)*globalyscale)<<8);
                    globalzd += (globalypanning<<24);
                    if (globalorientation&256) globalyscale = -globalyscale, globalzd = -globalzd;

                    if (gotswall == 0) { gotswall = 1; prepwall(z,wal); }
                    wallscan(x1,x2,uwall,dplc,swall,lwall);

                    if ((fz[2] <= fz[0]) && (fz[3] <= fz[1]))
                    {
                        for (x=x1; x<=x2; x++)
                            if (uwall[x] < dmost[x])
                                if (umost[x] <= dmost[x])
                                {
                                    dmost[x] = uwall[x];
                                    if (umost[x] > dmost[x]) numhits--;
                                }
                    }
                    else
                    {
                        for (x=x1; x<=x2; x++)
                            if (umost[x] <= dmost[x])
                            {
                                i = min(dplc[x],uwall[x]);
                                if (i < dmost[x])
                                {
                                    dmost[x] = i;
                                    if (umost[x] > dmost[x]) numhits--;
                                }
                            }
                    }
                }
                if ((fz[2] > fz[0]) || (fz[3] > fz[1]) || (globalposz > fz[4]))
                {
                    i = x2-x1+1;
                    if (smostcnt+i < MAXYSAVES)
                    {
                        smoststart[smostwallcnt] = smostcnt;
                        smostwall[smostwallcnt] = z;
                        smostwalltype[smostwallcnt] = 2;   //2 for dmost
                        smostwallcnt++;
                        copybufbyte(&dmost[x1],&smost[smostcnt],i*sizeof(smost[0]));
                        smostcnt += i;
                    }
                }
            }
            if (numhits < 0) return;
            if ((!(wal->cstat&32)) && ((gotsector[nextsectnum>>3]&pow2char[nextsectnum&7]) == 0))
            {
                if (umost[x2] < dmost[x2])
                    scansector(nextsectnum);
                else
                {
                    for (x=x1; x<x2; x++)
                        if (umost[x] < dmost[x])
                            { scansector(nextsectnum); break; }

                    //If can't see sector beyond, then cancel smost array and just
                    //store wall!
                    if (x == x2)
                    {
                        smostwallcnt = startsmostwallcnt;
                        smostcnt = startsmostcnt;
                        smostwall[smostwallcnt] = z;
                        smostwalltype[smostwallcnt] = 0;
                        smostwallcnt++;
                    }
                }
            }
        }
        if ((nextsectnum < 0) || (wal->cstat&32))   //White/1-way wall
        {
            globalorientation = (int32_t)wal->cstat;
            if (nextsectnum < 0) globalpicnum = wal->picnum;
            else globalpicnum = wal->overpicnum;
            if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;
            globalxpanning = (int32_t)wal->xpanning;
            globalypanning = (int32_t)wal->ypanning;
            if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,(int16_t)wallnum+16384);
            globalshade = (int32_t)wal->shade;
            globvis = globalvisibility;
            if (sec->visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t)(sec->visibility+16)));
            globalpal = (int32_t)wal->pal;
            if (palookup[globalpal] == NULL) globalpal = 0;    // JBF: fixes crash
            globalshiftval = (picsiz[globalpicnum]>>4);
            if (pow2long[globalshiftval] != tilesizy[globalpicnum]) globalshiftval++;
            globalshiftval = 32-globalshiftval;
            globalyscale = (wal->yrepeat<<(globalshiftval-19));
            if (nextsectnum >= 0)
            {
                if ((globalorientation&4) == 0) globalzd = globalposz-nextsec->ceilingz;
                else globalzd = globalposz-sec->ceilingz;
            }
            else
            {
                if ((globalorientation&4) == 0) globalzd = globalposz-sec->ceilingz;
                else globalzd = globalposz-sec->floorz;
            }
            globalzd = ((globalzd*globalyscale)<<8) + (globalypanning<<24);
            if (globalorientation&256) globalyscale = -globalyscale, globalzd = -globalzd;

            if (gotswall == 0) { gotswall = 1; prepwall(z,wal); }
            wallscan(x1,x2,uplc,dplc,swall,lwall);

            for (x=x1; x<=x2; x++)
                if (umost[x] <= dmost[x])
                    { umost[x] = 1; dmost[x] = 0; numhits--; }
            smostwall[smostwallcnt] = z;
            smostwalltype[smostwallcnt] = 0;
            smostwallcnt++;

            if ((searchit == 2) && (searchx >= x1) && (searchx <= x2))
            {
                searchit = 1; searchsector = sectnum; searchwall = wallnum;
                if (nextsectnum < 0) searchstat = 0; else searchstat = 4;
            }
        }
    }
}


//
// drawvox
//
#ifdef SUPERBUILD
static void drawvox(int32_t dasprx, int32_t daspry, int32_t dasprz, int32_t dasprang,
                    int32_t daxscale, int32_t dayscale, char daindex,
                    int8_t dashade, char dapal, int32_t *daumost, int32_t *dadmost)
{
    int32_t i, j, k, x, y, syoff, ggxstart, ggystart, nxoff;
    int32_t cosang, sinang, sprcosang, sprsinang, backx, backy, gxinc, gyinc;
    int32_t daxsiz, daysiz, dazsiz, daxpivot, daypivot, dazpivot;
    int32_t daxscalerecip, dayscalerecip, cnt, gxstart, gystart, odayscale;
    int32_t l1, l2, /*slabxoffs,*/ xyvoxoffs, *longptr;
    intptr_t slabxoffs;
    int32_t lx, rx, nx, ny, x1=0, y1=0, z1, x2=0, y2=0, z2, yplc, yinc=0;
    int32_t yoff, xs=0, ys=0, xe, ye, xi=0, yi=0, cbackx, cbacky, dagxinc, dagyinc;
    int16_t *shortptr;
    char *voxptr, *voxend, *davoxptr, oand, oand16, oand32;

    cosang = sintable[(globalang+512)&2047];
    sinang = sintable[globalang&2047];
    sprcosang = sintable[(dasprang+512)&2047];
    sprsinang = sintable[dasprang&2047];

    i = klabs(dmulscale6(dasprx-globalposx,cosang,daspry-globalposy,sinang));
    j = (int32_t)(getpalookup((int32_t)mulscale21(globvis,i),(int32_t)dashade)<<8);
    setupdrawslab(ylookup[1],FP_OFF(palookup[dapal])+j);
    j = 1310720;
    j *= min(daxscale,dayscale); j >>= 6;  //New hacks (for sized-down voxels)
    for (k=0; k<MAXVOXMIPS; k++)
    {
        if (i < j) { i = k; break; }
        j <<= 1;
    }
    if (k >= MAXVOXMIPS) i = MAXVOXMIPS-1;

    if (novoxmips) i = 0;
    davoxptr = (char *)voxoff[daindex][i];
    if (!davoxptr && i > 0) { davoxptr = (char *)voxoff[daindex][0]; i = 0; }
    if (!davoxptr) return;

    if (voxscale[daindex] == 65536)
        { daxscale <<= (i+8); dayscale <<= (i+8); }
    else
    {
        daxscale = mulscale8(daxscale<<i,voxscale[daindex]);
        dayscale = mulscale8(dayscale<<i,voxscale[daindex]);
    }

    odayscale = dayscale;
    daxscale = mulscale16(daxscale,xyaspect);
    daxscale = scale(daxscale,xdimenscale,xdimen<<8);
    dayscale = scale(dayscale,mulscale16(xdimenscale,viewingrangerecip),xdimen<<8);

    daxscalerecip = (1<<30)/daxscale;
    dayscalerecip = (1<<30)/dayscale;

    longptr = (int32_t *)davoxptr;
    daxsiz = B_LITTLE32(longptr[0]); daysiz = B_LITTLE32(longptr[1]); dazsiz = B_LITTLE32(longptr[2]);
    daxpivot = B_LITTLE32(longptr[3]); daypivot = B_LITTLE32(longptr[4]); dazpivot = B_LITTLE32(longptr[5]);
    davoxptr += (6<<2);

    x = mulscale16(globalposx-dasprx,daxscalerecip);
    y = mulscale16(globalposy-daspry,daxscalerecip);
    backx = ((dmulscale10(x,sprcosang,y,sprsinang)+daxpivot)>>8);
    backy = ((dmulscale10(y,sprcosang,x,-sprsinang)+daypivot)>>8);
    cbackx = min(max(backx,0),daxsiz-1);
    cbacky = min(max(backy,0),daysiz-1);

    sprcosang = mulscale14(daxscale,sprcosang);
    sprsinang = mulscale14(daxscale,sprsinang);

    x = (dasprx-globalposx) - dmulscale18(daxpivot,sprcosang,daypivot,-sprsinang);
    y = (daspry-globalposy) - dmulscale18(daypivot,sprcosang,daxpivot,sprsinang);

    cosang = mulscale16(cosang,dayscalerecip);
    sinang = mulscale16(sinang,dayscalerecip);

    gxstart = y*cosang - x*sinang;
    gystart = x*cosang + y*sinang;
    gxinc = dmulscale10(sprsinang,cosang,sprcosang,-sinang);
    gyinc = dmulscale10(sprcosang,cosang,sprsinang,sinang);

    x = 0; y = 0; j = max(daxsiz,daysiz);
    for (i=0; i<=j; i++)
    {
        ggxinc[i] = x; x += gxinc;
        ggyinc[i] = y; y += gyinc;
    }

    if ((klabs(globalposz-dasprz)>>10) >= klabs(odayscale)) return;
    syoff = divscale21(globalposz-dasprz,odayscale) + (dazpivot<<7);
    yoff = ((klabs(gxinc)+klabs(gyinc))>>1);
    longptr = (int32_t *)davoxptr;
    xyvoxoffs = ((daxsiz+1)<<2);

    begindrawing(); //{{{

    for (cnt=0; cnt<8; cnt++)
    {
        switch (cnt)
        {
        case 0:
            xs = 0;        ys = 0;        xi = 1;  yi = 1;  break;
        case 1:
            xs = daxsiz-1; ys = 0;        xi = -1; yi = 1;  break;
        case 2:
            xs = 0;        ys = daysiz-1; xi = 1;  yi = -1; break;
        case 3:
            xs = daxsiz-1; ys = daysiz-1; xi = -1; yi = -1; break;
        case 4:
            xs = 0;        ys = cbacky;   xi = 1;  yi = 2;  break;
        case 5:
            xs = daxsiz-1; ys = cbacky;   xi = -1; yi = 2;  break;
        case 6:
            xs = cbackx;   ys = 0;        xi = 2;  yi = 1;  break;
        case 7:
            xs = cbackx;   ys = daysiz-1; xi = 2;  yi = -1; break;
        }
        xe = cbackx; ye = cbacky;
        if (cnt < 4)
        {
            if ((xi < 0) && (xe >= xs)) continue;
            if ((xi > 0) && (xe <= xs)) continue;
            if ((yi < 0) && (ye >= ys)) continue;
            if ((yi > 0) && (ye <= ys)) continue;
        }
        else
        {
            if ((xi < 0) && (xe > xs)) continue;
            if ((xi > 0) && (xe < xs)) continue;
            if ((yi < 0) && (ye > ys)) continue;
            if ((yi > 0) && (ye < ys)) continue;
            xe += xi; ye += yi;
        }

        i = ksgn(ys-backy)+ksgn(xs-backx)*3+4;
        switch (i)
        {
        case 6:
        case 7:
            x1 = 0; y1 = 0; break;
        case 8:
        case 5:
            x1 = gxinc; y1 = gyinc; break;
        case 0:
        case 3:
            x1 = gyinc; y1 = -gxinc; break;
        case 2:
        case 1:
            x1 = gxinc+gyinc; y1 = gyinc-gxinc; break;
        }
        switch (i)
        {
        case 2:
        case 5:
            x2 = 0; y2 = 0; break;
        case 0:
        case 1:
            x2 = gxinc; y2 = gyinc; break;
        case 8:
        case 7:
            x2 = gyinc; y2 = -gxinc; break;
        case 6:
        case 3:
            x2 = gxinc+gyinc; y2 = gyinc-gxinc; break;
        }
        oand = pow2char[(xs<backx)+0]+pow2char[(ys<backy)+2];
        oand16 = oand+16;
        oand32 = oand+32;

        if (yi > 0) { dagxinc = gxinc; dagyinc = mulscale16(gyinc,viewingrangerecip); }
        else { dagxinc = -gxinc; dagyinc = -mulscale16(gyinc,viewingrangerecip); }

        //Fix for non 90 degree viewing ranges
        nxoff = mulscale16(x2-x1,viewingrangerecip);
        x1 = mulscale16(x1,viewingrangerecip);

        ggxstart = gxstart+ggyinc[ys];
        ggystart = gystart-ggxinc[ys];

        for (x=xs; x!=xe; x+=xi)
        {
            slabxoffs = (intptr_t)&davoxptr[B_LITTLE32(longptr[x])];
            shortptr = (int16_t *)&davoxptr[((x*(daysiz+1))<<1)+xyvoxoffs];

            nx = mulscale16(ggxstart+ggxinc[x],viewingrangerecip)+x1;
            ny = ggystart+ggyinc[x];
            for (y=ys; y!=ye; y+=yi,nx+=dagyinc,ny-=dagxinc)
            {
                if ((ny <= nytooclose) || (ny >= nytoofar)) continue;
                voxptr = (char *)(B_LITTLE16(shortptr[y])+slabxoffs);
                voxend = (char *)(B_LITTLE16(shortptr[y+1])+slabxoffs);
                if (voxptr == voxend) continue;

                lx = mulscale32(nx>>3,distrecip[(ny+y1)>>14])+halfxdimen;
                if (lx < 0) lx = 0;
                rx = mulscale32((nx+nxoff)>>3,distrecip[(ny+y2)>>14])+halfxdimen;
                if (rx > xdimen) rx = xdimen;
                if (rx <= lx) continue;
                rx -= lx;

                l1 = distrecip[(ny-yoff)>>14];
                l2 = distrecip[(ny+yoff)>>14];
                for (; voxptr<voxend; voxptr+=voxptr[1]+3)
                {
                    j = (voxptr[0]<<15)-syoff;
                    if (j < 0)
                    {
                        k = j+(voxptr[1]<<15);
                        if (k < 0)
                        {
                            if ((voxptr[2]&oand32) == 0) continue;
                            z2 = mulscale32(l2,k) + globalhoriz;     //Below slab
                        }
                        else
                        {
                            if ((voxptr[2]&oand) == 0) continue;    //Middle of slab
                            z2 = mulscale32(l1,k) + globalhoriz;
                        }
                        z1 = mulscale32(l1,j) + globalhoriz;
                    }
                    else
                    {
                        if ((voxptr[2]&oand16) == 0) continue;
                        z1 = mulscale32(l2,j) + globalhoriz;        //Above slab
                        z2 = mulscale32(l1,j+(voxptr[1]<<15)) + globalhoriz;
                    }

                    if (voxptr[1] == 1)
                    {
                        yplc = 0; yinc = 0;
                        if (z1 < daumost[lx]) z1 = daumost[lx];
                    }
                    else
                    {
                        if (z2-z1 >= 1024) yinc = divscale16(voxptr[1],z2-z1);
                        else if (z2 > z1) yinc = (lowrecip[z2-z1]*voxptr[1]>>8);
                        if (z1 < daumost[lx]) { yplc = yinc*(daumost[lx]-z1); z1 = daumost[lx]; }
                        else yplc = 0;
                    }
                    if (z2 > dadmost[lx]) z2 = dadmost[lx];
                    z2 -= z1; if (z2 <= 0) continue;

                    drawslab(rx,yplc,z2,yinc,(intptr_t)&voxptr[3],ylookup[z1]+lx+frameoffset);
                }
            }
        }
    }

    enddrawing();   //}}}
}
#endif

//
// drawsprite (internal)
//
static void drawsprite(int32_t snum)
{
    spritetype *tspr;
    sectortype *sec;
    int32_t startum, startdm, sectnum, xb, yp, cstat;
    int32_t siz, xsiz, ysiz, xoff, yoff, xspan, yspan;
    int32_t x1, y1, x2, y2, lx, rx, dalx2, darx2, i, j, k, x, linum, linuminc;
    int32_t yinc, z, z1, z2, xp1, yp1, xp2, yp2;
    int32_t xv, yv, top, topinc, bot, botinc, hplc, hinc;
    int32_t cosang, sinang, dax, day, lpoint, lmax, rpoint, rmax, dax1, dax2, y;
    int32_t npoints, npoints2, zz, t, zsgn, zzsgn, *longptr;
    int32_t tilenum, vtilenum = 0, spritenum;
    char swapped, daclip;

    //============================================================================= //POLYMOST BEGINS
#ifdef POLYMOST
    if (rendmode == 3)
    {
        polymost_drawsprite(snum);
# ifdef USE_OPENGL
        bglDisable(GL_POLYGON_OFFSET_FILL);
# endif
        return;
    }
# ifdef POLYMER
    if (rendmode == 4)
    {
        bglEnable(GL_ALPHA_TEST);
        bglEnable(GL_BLEND);

        polymer_drawsprite(snum);

        bglDisable(GL_BLEND);
        bglDisable(GL_ALPHA_TEST);
        return;
    }
# endif
#endif
    //============================================================================= //POLYMOST ENDS

    tspr = tspriteptr[snum];

    xb = spritesx[snum];
    yp = spritesy[snum];
    tilenum = tspr->picnum;
    spritenum = tspr->owner;
    cstat = tspr->cstat;

#ifdef SUPERBUILD
    if ((cstat&48)==48) vtilenum = tilenum; // if the game wants voxels, it gets voxels
    else if ((cstat&48)!=48 && (usevoxels) && (tiletovox[tilenum] != -1)
#if defined(POLYMOST) && defined(USE_OPENGL)
             && (!(spriteext[tspr->owner].flags&SPREXT_NOTMD))
#endif
            )
    {
        vtilenum = tiletovox[tilenum];
        cstat |= 48;
    }
#endif

    if ((cstat&48) != 48)
    {
        if (picanm[tilenum]&192) tilenum += animateoffs(tilenum,spritenum+32768);
        if ((tilesizx[tilenum] <= 0) || (tilesizy[tilenum] <= 0) || (spritenum < 0))
            return;
    }
    if ((tspr->xrepeat <= 0) || (tspr->yrepeat <= 0)) return;

    sectnum = tspr->sectnum; sec = &sector[sectnum];
    globalpal = tspr->pal;
    if (palookup[globalpal] == NULL) globalpal = 0;    // JBF: fixes null-pointer crash
    globalshade = tspr->shade;
    if (cstat&2)
    {
        if (cstat&512) settransreverse(); else settransnormal();
    }

    xoff = (int32_t)((int8_t)((picanm[tilenum]>>8)&255))+((int32_t)tspr->xoffset);
    yoff = (int32_t)((int8_t)((picanm[tilenum]>>16)&255))+((int32_t)tspr->yoffset);

    if ((cstat&48) == 0)
    {
        if (yp <= (4<<8)) return;

        siz = divscale19(xdimenscale,yp);

        xv = mulscale16(((int32_t)tspr->xrepeat)<<16,xyaspect);

        xspan = tilesizx[tilenum];
        yspan = tilesizy[tilenum];
        xsiz = mulscale30(siz,xv*xspan);
        ysiz = mulscale14(siz,tspr->yrepeat*yspan);

        if (((tilesizx[tilenum]>>11) >= xsiz) || (yspan >= (ysiz>>1)))
            return;  //Watch out for divscale overflow

        x1 = xb-(xsiz>>1);
        if (xspan&1) x1 += mulscale31(siz,xv);  //Odd xspans
        i = mulscale30(siz,xv*xoff);
        if ((cstat&4) == 0) x1 -= i; else x1 += i;

        y1 = mulscale16(tspr->z-globalposz,siz);
        y1 -= mulscale14(siz,tspr->yrepeat*yoff);
        y1 += (globalhoriz<<8)-ysiz;
        if (cstat&128)
        {
            y1 += (ysiz>>1);
            if (yspan&1) y1 += mulscale15(siz,tspr->yrepeat);  //Odd yspans
        }

        x2 = x1+xsiz-1;
        y2 = y1+ysiz-1;
        if ((y1|255) >= (y2|255)) return;

        lx = (x1>>8)+1; if (lx < 0) lx = 0;
        rx = (x2>>8); if (rx >= xdimen) rx = xdimen-1;
        if (lx > rx) return;

        yinc = divscale32(yspan,ysiz);

        if ((sec->ceilingstat&3) == 0)
            startum = globalhoriz+mulscale24(siz,sec->ceilingz-globalposz)-1;
        else
            startum = 0;
        if ((sec->floorstat&3) == 0)
            startdm = globalhoriz+mulscale24(siz,sec->floorz-globalposz)+1;
        else
            startdm = 0x7fffffff;
        if ((y1>>8) > startum) startum = (y1>>8);
        if ((y2>>8) < startdm) startdm = (y2>>8);

        if (startum < -32768) startum = -32768;
        if (startdm > 32767) startdm = 32767;
        if (startum >= startdm) return;

        if ((cstat&4) == 0)
        {
            linuminc = divscale24(xspan,xsiz);
            linum = mulscale8((lx<<8)-x1,linuminc);
        }
        else
        {
            linuminc = -divscale24(xspan,xsiz);
            linum = mulscale8((lx<<8)-x2,linuminc);
        }
        if ((cstat&8) > 0)
        {
            yinc = -yinc;
            i = y1; y1 = y2; y2 = i;
        }

        for (x=lx; x<=rx; x++)
        {
            uwall[x] = max(startumost[x+windowx1]-windowy1,(int16_t)startum);
            dwall[x] = min(startdmost[x+windowx1]-windowy1,(int16_t)startdm);
        }
        daclip = 0;
        for (i=smostwallcnt-1; i>=0; i--)
        {
            if (smostwalltype[i]&daclip) continue;
            j = smostwall[i];
            if ((xb1[j] > rx) || (xb2[j] < lx)) continue;
            if ((yp <= yb1[j]) && (yp <= yb2[j])) continue;
            if (spritewallfront(tspr,(int32_t)thewall[j]) && ((yp <= yb1[j]) || (yp <= yb2[j]))) continue;

            dalx2 = max(xb1[j],lx); darx2 = min(xb2[j],rx);

            switch (smostwalltype[i])
            {
            case 0:
                if (dalx2 <= darx2)
                {
                    if ((dalx2 == lx) && (darx2 == rx)) return;
                    //clearbufbyte(&dwall[dalx2],(darx2-dalx2+1)*sizeof(dwall[0]),0L);
                    for (k=dalx2; k<=darx2; k++) dwall[k] = 0;
                }
                break;
            case 1:
                k = smoststart[i] - xb1[j];
                for (x=dalx2; x<=darx2; x++)
                    if (smost[k+x] > uwall[x]) uwall[x] = smost[k+x];
                if ((dalx2 == lx) && (darx2 == rx)) daclip |= 1;
                break;
            case 2:
                k = smoststart[i] - xb1[j];
                for (x=dalx2; x<=darx2; x++)
                    if (smost[k+x] < dwall[x]) dwall[x] = smost[k+x];
                if ((dalx2 == lx) && (darx2 == rx)) daclip |= 2;
                break;
            }
        }

        if (uwall[rx] >= dwall[rx])
        {
            for (x=lx; x<rx; x++)
                if (uwall[x] < dwall[x]) break;
            if (x == rx) return;
        }

        //sprite
        if ((searchit >= 1) && (searchx >= lx) && (searchx <= rx))
            if ((searchy >= uwall[searchx]) && (searchy < dwall[searchx]))
            {
                searchsector = sectnum; searchwall = spritenum;
                searchstat = 3; searchit = 1;
            }

        z2 = tspr->z - ((yoff*tspr->yrepeat)<<2);
        if (cstat&128)
        {
            z2 += ((yspan*tspr->yrepeat)<<1);
            if (yspan&1) z2 += (tspr->yrepeat<<1);        //Odd yspans
        }
        z1 = z2 - ((yspan*tspr->yrepeat)<<2);

        globalorientation = 0;
        globalpicnum = tilenum;
        if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;
        globalxpanning = 0L;
        globalypanning = 0L;
        globvis = globalvisibility;
        if (sec->visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t)(sec->visibility+16)));
        globalshiftval = (picsiz[globalpicnum]>>4);
        if (pow2long[globalshiftval] != tilesizy[globalpicnum]) globalshiftval++;
        globalshiftval = 32-globalshiftval;
        globalyscale = divscale(512,tspr->yrepeat,globalshiftval-19);
        globalzd = (((globalposz-z1)*globalyscale)<<8);
        if ((cstat&8) > 0)
        {
            globalyscale = -globalyscale;
            globalzd = (((globalposz-z2)*globalyscale)<<8);
        }

        qinterpolatedown16((intptr_t)&lwall[lx],rx-lx+1,linum,linuminc);
        clearbuf(&swall[lx],rx-lx+1,mulscale19(yp,xdimscale));

        if ((cstat&2) == 0)
            maskwallscan(lx,rx,uwall,dwall,swall,lwall);
        else
            transmaskwallscan(lx,rx);
    }
    else if ((cstat&48) == 16)
    {
        if ((cstat&4) > 0) xoff = -xoff;
        if ((cstat&8) > 0) yoff = -yoff;

        xspan = tilesizx[tilenum]; yspan = tilesizy[tilenum];
        xv = tspr->xrepeat*sintable[(tspr->ang+2560+1536)&2047];
        yv = tspr->xrepeat*sintable[(tspr->ang+2048+1536)&2047];
        i = (xspan>>1)+xoff;
        x1 = tspr->x-globalposx-mulscale16(xv,i); x2 = x1+mulscale16(xv,xspan);
        y1 = tspr->y-globalposy-mulscale16(yv,i); y2 = y1+mulscale16(yv,xspan);

        yp1 = dmulscale6(x1,cosviewingrangeglobalang,y1,sinviewingrangeglobalang);
        yp2 = dmulscale6(x2,cosviewingrangeglobalang,y2,sinviewingrangeglobalang);
        if ((yp1 <= 0) && (yp2 <= 0)) return;
        xp1 = dmulscale6(y1,cosglobalang,-x1,singlobalang);
        xp2 = dmulscale6(y2,cosglobalang,-x2,singlobalang);

        x1 += globalposx; y1 += globalposy;
        x2 += globalposx; y2 += globalposy;

        swapped = 0;
        if (dmulscale32(xp1,yp2,-xp2,yp1) >= 0)  //If wall's NOT facing you
        {
            if ((cstat&64) != 0) return;
            i = xp1, xp1 = xp2, xp2 = i;
            i = yp1, yp1 = yp2, yp2 = i;
            i = x1, x1 = x2, x2 = i;
            i = y1, y1 = y2, y2 = i;
            swapped = 1;
        }

        if (xp1 >= -yp1)
        {
            if (xp1 > yp1) return;

            if (yp1 == 0) return;
            xb1[MAXWALLSB-1] = halfxdimen + scale(xp1,halfxdimen,yp1);
            if (xp1 >= 0) xb1[MAXWALLSB-1]++;   //Fix for SIGNED divide
            if (xb1[MAXWALLSB-1] >= xdimen) xb1[MAXWALLSB-1] = xdimen-1;
            yb1[MAXWALLSB-1] = yp1;
        }
        else
        {
            if (xp2 < -yp2) return;
            xb1[MAXWALLSB-1] = 0;
            i = yp1-yp2+xp1-xp2;
            if (i == 0) return;
            yb1[MAXWALLSB-1] = yp1 + scale(yp2-yp1,xp1+yp1,i);
        }
        if (xp2 <= yp2)
        {
            if (xp2 < -yp2) return;

            if (yp2 == 0) return;
            xb2[MAXWALLSB-1] = halfxdimen + scale(xp2,halfxdimen,yp2) - 1;
            if (xp2 >= 0) xb2[MAXWALLSB-1]++;   //Fix for SIGNED divide
            if (xb2[MAXWALLSB-1] >= xdimen) xb2[MAXWALLSB-1] = xdimen-1;
            yb2[MAXWALLSB-1] = yp2;
        }
        else
        {
            if (xp1 > yp1) return;

            xb2[MAXWALLSB-1] = xdimen-1;
            i = xp2-xp1+yp1-yp2;
            if (i == 0) return;
            yb2[MAXWALLSB-1] = yp1 + scale(yp2-yp1,yp1-xp1,i);
        }

        if ((yb1[MAXWALLSB-1] < 256) || (yb2[MAXWALLSB-1] < 256) || (xb1[MAXWALLSB-1] > xb2[MAXWALLSB-1]))
            return;

        topinc = -mulscale10(yp1,xspan);
        top = (((mulscale10(xp1,xdimen) - mulscale9(xb1[MAXWALLSB-1]-halfxdimen,yp1))*xspan)>>3);
        botinc = ((yp2-yp1)>>8);
        bot = mulscale11(xp1-xp2,xdimen) + mulscale2(xb1[MAXWALLSB-1]-halfxdimen,botinc);

        j = xb2[MAXWALLSB-1]+3;
        z = mulscale20(top,krecipasm(bot));
        lwall[xb1[MAXWALLSB-1]] = (z>>8);
        for (x=xb1[MAXWALLSB-1]+4; x<=j; x+=4)
        {
            top += topinc; bot += botinc;
            zz = z; z = mulscale20(top,krecipasm(bot));
            lwall[x] = (z>>8);
            i = ((z+zz)>>1);
            lwall[x-2] = (i>>8);
            lwall[x-3] = ((i+zz)>>9);
            lwall[x-1] = ((i+z)>>9);
        }

        if (lwall[xb1[MAXWALLSB-1]] < 0) lwall[xb1[MAXWALLSB-1]] = 0;
        if (lwall[xb2[MAXWALLSB-1]] >= xspan) lwall[xb2[MAXWALLSB-1]] = xspan-1;

        if ((swapped^((cstat&4)>0)) > 0)
        {
            j = xspan-1;
            for (x=xb1[MAXWALLSB-1]; x<=xb2[MAXWALLSB-1]; x++)
                lwall[x] = j-lwall[x];
        }

        rx1[MAXWALLSB-1] = xp1; ry1[MAXWALLSB-1] = yp1;
        rx2[MAXWALLSB-1] = xp2; ry2[MAXWALLSB-1] = yp2;

        hplc = divscale19(xdimenscale,yb1[MAXWALLSB-1]);
        hinc = divscale19(xdimenscale,yb2[MAXWALLSB-1]);
        hinc = (hinc-hplc)/(xb2[MAXWALLSB-1]-xb1[MAXWALLSB-1]+1);

        z2 = tspr->z - ((yoff*tspr->yrepeat)<<2);
        if (cstat&128)
        {
            z2 += ((yspan*tspr->yrepeat)<<1);
            if (yspan&1) z2 += (tspr->yrepeat<<1);        //Odd yspans
        }
        z1 = z2 - ((yspan*tspr->yrepeat)<<2);

        globalorientation = 0;
        globalpicnum = tilenum;
        if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;
        globalxpanning = 0L;
        globalypanning = 0L;
        globvis = globalvisibility;
        if (sec->visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t)(sec->visibility+16)));
        globalshiftval = (picsiz[globalpicnum]>>4);
        if (pow2long[globalshiftval] != tilesizy[globalpicnum]) globalshiftval++;
        globalshiftval = 32-globalshiftval;
        globalyscale = divscale(512,tspr->yrepeat,globalshiftval-19);
        globalzd = (((globalposz-z1)*globalyscale)<<8);
        if ((cstat&8) > 0)
        {
            globalyscale = -globalyscale;
            globalzd = (((globalposz-z2)*globalyscale)<<8);
        }

        if (((sec->ceilingstat&1) == 0) && (z1 < sec->ceilingz))
            z1 = sec->ceilingz;
        if (((sec->floorstat&1) == 0) && (z2 > sec->floorz))
            z2 = sec->floorz;

        owallmost(uwall,(int32_t)(MAXWALLSB-1),z1-globalposz);
        owallmost(dwall,(int32_t)(MAXWALLSB-1),z2-globalposz);
        for (i=xb1[MAXWALLSB-1]; i<=xb2[MAXWALLSB-1]; i++)
            { swall[i] = (krecipasm(hplc)<<2); hplc += hinc; }

        for (i=smostwallcnt-1; i>=0; i--)
        {
            j = smostwall[i];

            if ((xb1[j] > xb2[MAXWALLSB-1]) || (xb2[j] < xb1[MAXWALLSB-1])) continue;

            dalx2 = xb1[j]; darx2 = xb2[j];
            if (max(yb1[MAXWALLSB-1],yb2[MAXWALLSB-1]) > min(yb1[j],yb2[j]))
            {
                if (min(yb1[MAXWALLSB-1],yb2[MAXWALLSB-1]) > max(yb1[j],yb2[j]))
                {
                    x = 0x80000000;
                }
                else
                {
                    x = thewall[j]; xp1 = wall[x].x; yp1 = wall[x].y;
                    x = wall[x].point2; xp2 = wall[x].x; yp2 = wall[x].y;

                    z1 = (xp2-xp1)*(y1-yp1) - (yp2-yp1)*(x1-xp1);
                    z2 = (xp2-xp1)*(y2-yp1) - (yp2-yp1)*(x2-xp1);
                    if ((z1^z2) >= 0)
                        x = (z1+z2);
                    else
                    {
                        z1 = (x2-x1)*(yp1-y1) - (y2-y1)*(xp1-x1);
                        z2 = (x2-x1)*(yp2-y1) - (y2-y1)*(xp2-x1);

                        if ((z1^z2) >= 0)
                            x = -(z1+z2);
                        else
                        {
                            if ((xp2-xp1)*(tspr->y-yp1) == (tspr->x-xp1)*(yp2-yp1))
                            {
                                if (wall[thewall[j]].nextsector == tspr->sectnum)
                                    x = 0x80000000;
                                else
                                    x = 0x7fffffff;
                            }
                            else
                            {
                                //INTERSECTION!
                                x = (xp1-globalposx) + scale(xp2-xp1,z1,z1-z2);
                                y = (yp1-globalposy) + scale(yp2-yp1,z1,z1-z2);

                                yp1 = dmulscale14(x,cosglobalang,y,singlobalang);
                                if (yp1 > 0)
                                {
                                    xp1 = dmulscale14(y,cosglobalang,-x,singlobalang);

                                    x = halfxdimen + scale(xp1,halfxdimen,yp1);
                                    if (xp1 >= 0) x++;   //Fix for SIGNED divide

                                    if (z1 < 0)
                                        { if (dalx2 < x) dalx2 = x; }
                                    else
                                        { if (darx2 > x) darx2 = x; }
                                    x = 0x80000001;
                                }
                                else
                                    x = 0x7fffffff;
                            }
                        }
                    }
                }
                if (x < 0)
                {
                    if (dalx2 < xb1[MAXWALLSB-1]) dalx2 = xb1[MAXWALLSB-1];
                    if (darx2 > xb2[MAXWALLSB-1]) darx2 = xb2[MAXWALLSB-1];
                    switch (smostwalltype[i])
                    {
                    case 0:
                        if (dalx2 <= darx2)
                        {
                            if ((dalx2 == xb1[MAXWALLSB-1]) && (darx2 == xb2[MAXWALLSB-1])) return;
                            //clearbufbyte(&dwall[dalx2],(darx2-dalx2+1)*sizeof(dwall[0]),0L);
                            for (k=dalx2; k<=darx2; k++) dwall[k] = 0;
                        }
                        break;
                    case 1:
                        k = smoststart[i] - xb1[j];
                        for (x=dalx2; x<=darx2; x++)
                            if (smost[k+x] > uwall[x]) uwall[x] = smost[k+x];
                        break;
                    case 2:
                        k = smoststart[i] - xb1[j];
                        for (x=dalx2; x<=darx2; x++)
                            if (smost[k+x] < dwall[x]) dwall[x] = smost[k+x];
                        break;
                    }
                }
            }
        }

        //sprite
        if ((searchit >= 1) && (searchx >= xb1[MAXWALLSB-1]) && (searchx <= xb2[MAXWALLSB-1]))
            if ((searchy >= uwall[searchx]) && (searchy <= dwall[searchx]))
            {
                searchsector = sectnum; searchwall = spritenum;
                searchstat = 3; searchit = 1;
            }

        if ((cstat&2) == 0)
        {
            maskwallscan(xb1[MAXWALLSB-1],xb2[MAXWALLSB-1],uwall,dwall,swall,lwall);
        }
        else
        {
            transmaskwallscan(xb1[MAXWALLSB-1],xb2[MAXWALLSB-1]);
        }
    }
    else if ((cstat&48) == 32)
    {
        if ((cstat&64) != 0)
            if ((globalposz > tspr->z) == ((cstat&8)==0))
                return;

        if ((cstat&4) > 0) xoff = -xoff;
        if ((cstat&8) > 0) yoff = -yoff;
        xspan = tilesizx[tilenum];
        yspan = tilesizy[tilenum];

        //Rotate center point
        dax = tspr->x-globalposx;
        day = tspr->y-globalposy;
        rzi[0] = dmulscale10(cosglobalang,dax,singlobalang,day);
        rxi[0] = dmulscale10(cosglobalang,day,-singlobalang,dax);

        //Get top-left corner
        i = ((tspr->ang+2048-globalang)&2047);
        cosang = sintable[(i+512)&2047]; sinang = sintable[i];
        dax = ((xspan>>1)+xoff)*tspr->xrepeat;
        day = ((yspan>>1)+yoff)*tspr->yrepeat;
        rzi[0] += dmulscale12(sinang,dax,cosang,day);
        rxi[0] += dmulscale12(sinang,day,-cosang,dax);

        //Get other 3 corners
        dax = xspan*tspr->xrepeat;
        day = yspan*tspr->yrepeat;
        rzi[1] = rzi[0]-mulscale12(sinang,dax);
        rxi[1] = rxi[0]+mulscale12(cosang,dax);
        dax = -mulscale12(cosang,day);
        day = -mulscale12(sinang,day);
        rzi[2] = rzi[1]+dax; rxi[2] = rxi[1]+day;
        rzi[3] = rzi[0]+dax; rxi[3] = rxi[0]+day;

        //Put all points on same z
        ryi[0] = scale((tspr->z-globalposz),yxaspect,320<<8);
        if (ryi[0] == 0) return;
        ryi[1] = ryi[2] = ryi[3] = ryi[0];

        if ((cstat&4) == 0)
            { z = 0; z1 = 1; z2 = 3; }
        else
            { z = 1; z1 = 0; z2 = 2; }

        dax = rzi[z1]-rzi[z]; day = rxi[z1]-rxi[z];
        bot = dmulscale8(dax,dax,day,day);
        if (((klabs(dax)>>13) >= bot) || ((klabs(day)>>13) >= bot)) return;
        globalx1 = divscale18(dax,bot);
        globalx2 = divscale18(day,bot);

        dax = rzi[z2]-rzi[z]; day = rxi[z2]-rxi[z];
        bot = dmulscale8(dax,dax,day,day);
        if (((klabs(dax)>>13) >= bot) || ((klabs(day)>>13) >= bot)) return;
        globaly1 = divscale18(dax,bot);
        globaly2 = divscale18(day,bot);

        //Calculate globals for hline texture mapping function
        globalxpanning = (rxi[z]<<12);
        globalypanning = (rzi[z]<<12);
        globalzd = (ryi[z]<<12);

        rzi[0] = mulscale16(rzi[0],viewingrange);
        rzi[1] = mulscale16(rzi[1],viewingrange);
        rzi[2] = mulscale16(rzi[2],viewingrange);
        rzi[3] = mulscale16(rzi[3],viewingrange);

        if (ryi[0] < 0)   //If ceilsprite is above you, reverse order of points
        {
            i = rxi[1]; rxi[1] = rxi[3]; rxi[3] = i;
            i = rzi[1]; rzi[1] = rzi[3]; rzi[3] = i;
        }


        //Clip polygon in 3-space
        npoints = 4;

        //Clip edge 1
        npoints2 = 0;
        zzsgn = rxi[0]+rzi[0];
        for (z=0; z<npoints; z++)
        {
            zz = z+1; if (zz == npoints) zz = 0;
            zsgn = zzsgn; zzsgn = rxi[zz]+rzi[zz];
            if (zsgn >= 0)
            {
                rxi2[npoints2] = rxi[z]; ryi2[npoints2] = ryi[z]; rzi2[npoints2] = rzi[z];
                npoints2++;
            }
            if ((zsgn^zzsgn) < 0)
            {
                t = divscale30(zsgn,zsgn-zzsgn);
                rxi2[npoints2] = rxi[z] + mulscale30(t,rxi[zz]-rxi[z]);
                ryi2[npoints2] = ryi[z] + mulscale30(t,ryi[zz]-ryi[z]);
                rzi2[npoints2] = rzi[z] + mulscale30(t,rzi[zz]-rzi[z]);
                npoints2++;
            }
        }
        if (npoints2 <= 2) return;

        //Clip edge 2
        npoints = 0;
        zzsgn = rxi2[0]-rzi2[0];
        for (z=0; z<npoints2; z++)
        {
            zz = z+1; if (zz == npoints2) zz = 0;
            zsgn = zzsgn; zzsgn = rxi2[zz]-rzi2[zz];
            if (zsgn <= 0)
            {
                rxi[npoints] = rxi2[z]; ryi[npoints] = ryi2[z]; rzi[npoints] = rzi2[z];
                npoints++;
            }
            if ((zsgn^zzsgn) < 0)
            {
                t = divscale30(zsgn,zsgn-zzsgn);
                rxi[npoints] = rxi2[z] + mulscale30(t,rxi2[zz]-rxi2[z]);
                ryi[npoints] = ryi2[z] + mulscale30(t,ryi2[zz]-ryi2[z]);
                rzi[npoints] = rzi2[z] + mulscale30(t,rzi2[zz]-rzi2[z]);
                npoints++;
            }
        }
        if (npoints <= 2) return;

        //Clip edge 3
        npoints2 = 0;
        zzsgn = ryi[0]*halfxdimen + (rzi[0]*(globalhoriz-0));
        for (z=0; z<npoints; z++)
        {
            zz = z+1; if (zz == npoints) zz = 0;
            zsgn = zzsgn; zzsgn = ryi[zz]*halfxdimen + (rzi[zz]*(globalhoriz-0));
            if (zsgn >= 0)
            {
                rxi2[npoints2] = rxi[z];
                ryi2[npoints2] = ryi[z];
                rzi2[npoints2] = rzi[z];
                npoints2++;
            }
            if ((zsgn^zzsgn) < 0)
            {
                t = divscale30(zsgn,zsgn-zzsgn);
                rxi2[npoints2] = rxi[z] + mulscale30(t,rxi[zz]-rxi[z]);
                ryi2[npoints2] = ryi[z] + mulscale30(t,ryi[zz]-ryi[z]);
                rzi2[npoints2] = rzi[z] + mulscale30(t,rzi[zz]-rzi[z]);
                npoints2++;
            }
        }
        if (npoints2 <= 2) return;

        //Clip edge 4
        npoints = 0;
        zzsgn = ryi2[0]*halfxdimen + (rzi2[0]*(globalhoriz-ydimen));
        for (z=0; z<npoints2; z++)
        {
            zz = z+1; if (zz == npoints2) zz = 0;
            zsgn = zzsgn; zzsgn = ryi2[zz]*halfxdimen + (rzi2[zz]*(globalhoriz-ydimen));
            if (zsgn <= 0)
            {
                rxi[npoints] = rxi2[z];
                ryi[npoints] = ryi2[z];
                rzi[npoints] = rzi2[z];
                npoints++;
            }
            if ((zsgn^zzsgn) < 0)
            {
                t = divscale30(zsgn,zsgn-zzsgn);
                rxi[npoints] = rxi2[z] + mulscale30(t,rxi2[zz]-rxi2[z]);
                ryi[npoints] = ryi2[z] + mulscale30(t,ryi2[zz]-ryi2[z]);
                rzi[npoints] = rzi2[z] + mulscale30(t,rzi2[zz]-rzi2[z]);
                npoints++;
            }
        }
        if (npoints <= 2) return;

        //Project onto screen
        lpoint = -1; lmax = 0x7fffffff;
        rpoint = -1; rmax = 0x80000000;
        for (z=0; z<npoints; z++)
        {
            xsi[z] = scale(rxi[z],xdimen<<15,rzi[z]) + (xdimen<<15);
            ysi[z] = scale(ryi[z],xdimen<<15,rzi[z]) + (globalhoriz<<16);
            if (xsi[z] < 0) xsi[z] = 0;
            if (xsi[z] > (xdimen<<16)) xsi[z] = (xdimen<<16);
            if (ysi[z] < ((int32_t)0<<16)) ysi[z] = ((int32_t)0<<16);
            if (ysi[z] > ((int32_t)ydimen<<16)) ysi[z] = ((int32_t)ydimen<<16);
            if (xsi[z] < lmax) lmax = xsi[z], lpoint = z;
            if (xsi[z] > rmax) rmax = xsi[z], rpoint = z;
        }

        //Get uwall arrays
        for (z=lpoint; z!=rpoint; z=zz)
        {
            zz = z+1; if (zz == npoints) zz = 0;

            dax1 = ((xsi[z]+65535)>>16);
            dax2 = ((xsi[zz]+65535)>>16);
            if (dax2 > dax1)
            {
                yinc = divscale16(ysi[zz]-ysi[z],xsi[zz]-xsi[z]);
                y = ysi[z] + mulscale16((dax1<<16)-xsi[z],yinc);
                qinterpolatedown16short((intptr_t)(&uwall[dax1]),dax2-dax1,y,yinc);
            }
        }

        //Get dwall arrays
        for (; z!=lpoint; z=zz)
        {
            zz = z+1; if (zz == npoints) zz = 0;

            dax1 = ((xsi[zz]+65535)>>16);
            dax2 = ((xsi[z]+65535)>>16);
            if (dax2 > dax1)
            {
                yinc = divscale16(ysi[zz]-ysi[z],xsi[zz]-xsi[z]);
                y = ysi[zz] + mulscale16((dax1<<16)-xsi[zz],yinc);
                qinterpolatedown16short((intptr_t)(&dwall[dax1]),dax2-dax1,y,yinc);
            }
        }


        lx = ((lmax+65535)>>16);
        rx = ((rmax+65535)>>16);
        for (x=lx; x<=rx; x++)
        {
            uwall[x] = max(uwall[x],startumost[x+windowx1]-windowy1);
            dwall[x] = min(dwall[x],startdmost[x+windowx1]-windowy1);
        }

        //Additional uwall/dwall clipping goes here
        for (i=smostwallcnt-1; i>=0; i--)
        {
            j = smostwall[i];
            if ((xb1[j] > rx) || (xb2[j] < lx)) continue;
            if ((yp <= yb1[j]) && (yp <= yb2[j])) continue;

            //if (spritewallfront(tspr,thewall[j]) == 0)
            x = thewall[j]; xp1 = wall[x].x; yp1 = wall[x].y;
            x = wall[x].point2; xp2 = wall[x].x; yp2 = wall[x].y;
            x = (xp2-xp1)*(tspr->y-yp1)-(tspr->x-xp1)*(yp2-yp1);
            if ((yp > yb1[j]) && (yp > yb2[j])) x = -1;
            if ((x >= 0) && ((x != 0) || (wall[thewall[j]].nextsector != tspr->sectnum))) continue;

            dalx2 = max(xb1[j],lx); darx2 = min(xb2[j],rx);

            switch (smostwalltype[i])
            {
            case 0:
                if (dalx2 <= darx2)
                {
                    if ((dalx2 == lx) && (darx2 == rx)) return;
                    //clearbufbyte(&dwall[dalx2],(darx2-dalx2+1)*sizeof(dwall[0]),0L);
                    for (x=dalx2; x<=darx2; x++) dwall[x] = 0;
                }
                break;
            case 1:
                k = smoststart[i] - xb1[j];
                for (x=dalx2; x<=darx2; x++)
                    if (smost[k+x] > uwall[x]) uwall[x] = smost[k+x];
                break;
            case 2:
                k = smoststart[i] - xb1[j];
                for (x=dalx2; x<=darx2; x++)
                    if (smost[k+x] < dwall[x]) dwall[x] = smost[k+x];
                break;
            }
        }

        //sprite
        if ((searchit >= 1) && (searchx >= lx) && (searchx <= rx))
            if ((searchy >= uwall[searchx]) && (searchy <= dwall[searchx]))
            {
                searchsector = sectnum; searchwall = spritenum;
                searchstat = 3; searchit = 1;
            }

        globalorientation = cstat;
        globalpicnum = tilenum;
        if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;
        //if (picanm[globalpicnum]&192) globalpicnum += animateoffs((short)globalpicnum,spritenum+32768);

        if (waloff[globalpicnum] == 0) loadtile(globalpicnum);
        setgotpic(globalpicnum);
        globalbufplc = waloff[globalpicnum];

        globvis = mulscale16(globalhisibility,viewingrange);
        if (sec->visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t)(sec->visibility+16)));

        x = picsiz[globalpicnum]; y = ((x>>4)&15); x &= 15;
        if (pow2long[x] != xspan)
        {
            x++;
            globalx1 = mulscale(globalx1,xspan,x);
            globalx2 = mulscale(globalx2,xspan,x);
        }

        dax = globalxpanning; day = globalypanning;
        globalxpanning = -dmulscale6(globalx1,day,globalx2,dax);
        globalypanning = -dmulscale6(globaly1,day,globaly2,dax);

        globalx2 = mulscale16(globalx2,viewingrange);
        globaly2 = mulscale16(globaly2,viewingrange);
        globalzd = mulscale16(globalzd,viewingrangerecip);

        globalx1 = (globalx1-globalx2)*halfxdimen;
        globaly1 = (globaly1-globaly2)*halfxdimen;

        if ((cstat&2) == 0)
            msethlineshift(x,y);
        else
            tsethlineshift(x,y);

        //Draw it!
        ceilspritescan(lx,rx-1);
    }
#ifdef SUPERBUILD
    else if ((cstat&48) == 48)
    {
        int32_t nxrepeat, nyrepeat;

        lx = 0; rx = xdim-1;
        for (x=lx; x<=rx; x++)
        {
            lwall[x] = (int32_t)startumost[x+windowx1]-windowy1;
            swall[x] = (int32_t)startdmost[x+windowx1]-windowy1;
        }
        for (i=smostwallcnt-1; i>=0; i--)
        {
            j = smostwall[i];
            if ((xb1[j] > rx) || (xb2[j] < lx)) continue;
            if ((yp <= yb1[j]) && (yp <= yb2[j])) continue;
            if (spritewallfront(tspr,(int32_t)thewall[j]) && ((yp <= yb1[j]) || (yp <= yb2[j]))) continue;

            dalx2 = max(xb1[j],lx); darx2 = min(xb2[j],rx);

            switch (smostwalltype[i])
            {
            case 0:
                if (dalx2 <= darx2)
                {
                    if ((dalx2 == lx) && (darx2 == rx)) return;
                    //clearbufbyte(&swall[dalx2],(darx2-dalx2+1)*sizeof(swall[0]),0L);
                    for (x=dalx2; x<=darx2; x++) swall[x] = 0;
                }
                break;
            case 1:
                k = smoststart[i] - xb1[j];
                for (x=dalx2; x<=darx2; x++)
                    if (smost[k+x] > lwall[x]) lwall[x] = smost[k+x];
                break;
            case 2:
                k = smoststart[i] - xb1[j];
                for (x=dalx2; x<=darx2; x++)
                    if (smost[k+x] < swall[x]) swall[x] = smost[k+x];
                break;
            }
        }

        if (lwall[rx] >= swall[rx])
        {
            for (x=lx; x<rx; x++)
                if (lwall[x] < swall[x]) break;
            if (x == rx) return;
        }

        for (i=0; i<MAXVOXMIPS; i++)
            if (!voxoff[vtilenum][i])
            {
                kloadvoxel(vtilenum);
                break;
            }

        longptr = (int32_t *)voxoff[vtilenum][0];

        if (voxscale[vtilenum] == 65536)
        {
            nxrepeat = (((int32_t)tspr->xrepeat)<<16);
            nyrepeat = (((int32_t)tspr->yrepeat)<<16);
        }
        else
        {
            nxrepeat = ((int32_t)tspr->xrepeat)*voxscale[vtilenum];
            nyrepeat = ((int32_t)tspr->yrepeat)*voxscale[vtilenum];
        }

        if (!(cstat&128)) tspr->z -= mulscale22(B_LITTLE32(longptr[5]),nyrepeat);
        yoff = (int32_t)((int8_t)((picanm[sprite[tspr->owner].picnum]>>16)&255))+((int32_t)tspr->yoffset);
        tspr->z -= mulscale14(yoff,nyrepeat);

        globvis = globalvisibility;
        if (sec->visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t)(sec->visibility+16)));

        if ((searchit >= 1) && (yp > (4<<8)) && (searchy >= lwall[searchx]) && (searchy < swall[searchx]))
        {
            siz = divscale19(xdimenscale,yp);

            xv = mulscale16(nxrepeat,xyaspect);

            xspan = ((B_LITTLE32(longptr[0])+B_LITTLE32(longptr[1]))>>1);
            yspan = B_LITTLE32(longptr[2]);
            xsiz = mulscale30(siz,xv*xspan);
            ysiz = mulscale30(siz,nyrepeat*yspan);

            //Watch out for divscale overflow
            if (((xspan>>11) < xsiz) && (yspan < (ysiz>>1)))
            {
                x1 = xb-(xsiz>>1);
                if (xspan&1) x1 += mulscale31(siz,xv);  //Odd xspans
                i = mulscale30(siz,xv*xoff);
                if ((cstat&4) == 0) x1 -= i; else x1 += i;

                y1 = mulscale16(tspr->z-globalposz,siz);
                //y1 -= mulscale30(siz,nyrepeat*yoff);
                y1 += (globalhoriz<<8)-ysiz;
                //if (cstat&128)  //Already fixed up above
                y1 += (ysiz>>1);

                x2 = x1+xsiz-1;
                y2 = y1+ysiz-1;
                if (((y1|255) < (y2|255)) && (searchx >= (x1>>8)+1) && (searchx <= (x2>>8)))
                {
                    if ((sec->ceilingstat&3) == 0)
                        startum = globalhoriz+mulscale24(siz,sec->ceilingz-globalposz)-1;
                    else
                        startum = 0;
                    if ((sec->floorstat&3) == 0)
                        startdm = globalhoriz+mulscale24(siz,sec->floorz-globalposz)+1;
                    else
                        startdm = 0x7fffffff;

                    //sprite
                    if ((searchy >= max(startum,(y1>>8))) && (searchy < min(startdm,(y2>>8))))
                    {
                        searchsector = sectnum; searchwall = spritenum;
                        searchstat = 3; searchit = 1;
                    }
                }
            }
        }

        i = (int32_t)tspr->ang+1536;
#if defined(POLYMOST) && defined(USE_OPENGL)
        i += spriteext[tspr->owner].angoff;
#endif
        drawvox(tspr->x,tspr->y,tspr->z,i,(int32_t)tspr->xrepeat,(int32_t)tspr->yrepeat,vtilenum,tspr->shade,tspr->pal,lwall,swall);
    }
#endif
    if (automapping == 1) show2dsprite[spritenum>>3] |= pow2char[spritenum&7];
}


//
// drawmaskwall (internal)
//
static void drawmaskwall(int16_t damaskwallcnt)
{
    int32_t i, j, k, x, z, sectnum, z1, z2, lx, rx;
    sectortype *sec, *nsec;
    walltype *wal;

    //============================================================================= //POLYMOST BEGINS
#ifdef POLYMOST
    if (rendmode == 3) { polymost_drawmaskwall(damaskwallcnt); return; }
# ifdef POLYMER
    if (rendmode == 4)
    {
        bglEnable(GL_ALPHA_TEST);
        bglEnable(GL_BLEND);

        polymer_drawmaskwall(damaskwallcnt);

        bglDisable(GL_BLEND);
        bglDisable(GL_ALPHA_TEST);

        return;
    }
#endif
#endif
    //============================================================================= //POLYMOST ENDS

    z = maskwall[damaskwallcnt];
    wal = &wall[thewall[z]];
    sectnum = thesector[z]; sec = &sector[sectnum];
    nsec = &sector[wal->nextsector];
    z1 = max(nsec->ceilingz,sec->ceilingz);
    z2 = min(nsec->floorz,sec->floorz);

    wallmost(uwall,z,sectnum,(uint8_t)0);
    wallmost(uplc,z,(int32_t)wal->nextsector,(uint8_t)0);
    for (x=xb1[z]; x<=xb2[z]; x++) if (uplc[x] > uwall[x]) uwall[x] = uplc[x];
    wallmost(dwall,z,sectnum,(uint8_t)1);
    wallmost(dplc,z,(int32_t)wal->nextsector,(uint8_t)1);
    for (x=xb1[z]; x<=xb2[z]; x++) if (dplc[x] < dwall[x]) dwall[x] = dplc[x];
    prepwall(z,wal);

    globalorientation = (int32_t)wal->cstat;
    globalpicnum = wal->overpicnum;
    if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;
    globalxpanning = (int32_t)wal->xpanning;
    globalypanning = (int32_t)wal->ypanning;
    if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,(int16_t)thewall[z]+16384);
    globalshade = (int32_t)wal->shade;
    globvis = globalvisibility;
    if (sec->visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t)(sec->visibility+16)));
    globalpal = (int32_t)wal->pal;
    globalshiftval = (picsiz[globalpicnum]>>4);
    if (pow2long[globalshiftval] != tilesizy[globalpicnum]) globalshiftval++;
    globalshiftval = 32-globalshiftval;
    globalyscale = (wal->yrepeat<<(globalshiftval-19));
    if ((globalorientation&4) == 0)
        globalzd = (((globalposz-z1)*globalyscale)<<8);
    else
        globalzd = (((globalposz-z2)*globalyscale)<<8);
    globalzd += (globalypanning<<24);
    if (globalorientation&256) globalyscale = -globalyscale, globalzd = -globalzd;

    for (i=smostwallcnt-1; i>=0; i--)
    {
        j = smostwall[i];
        if ((xb1[j] > xb2[z]) || (xb2[j] < xb1[z])) continue;
        if (wallfront(j,z)) continue;

        lx = max(xb1[j],xb1[z]); rx = min(xb2[j],xb2[z]);

        switch (smostwalltype[i])
        {
        case 0:
            if (lx <= rx)
            {
                if ((lx == xb1[z]) && (rx == xb2[z])) return;
                //clearbufbyte(&dwall[lx],(rx-lx+1)*sizeof(dwall[0]),0L);
                for (x=lx; x<=rx; x++) dwall[x] = 0;
            }
            break;
        case 1:
            k = smoststart[i] - xb1[j];
            for (x=lx; x<=rx; x++)
                if (smost[k+x] > uwall[x]) uwall[x] = smost[k+x];
            break;
        case 2:
            k = smoststart[i] - xb1[j];
            for (x=lx; x<=rx; x++)
                if (smost[k+x] < dwall[x]) dwall[x] = smost[k+x];
            break;
        }
    }

    //maskwall
    if ((searchit >= 1) && (searchx >= xb1[z]) && (searchx <= xb2[z]))
        if ((searchy >= uwall[searchx]) && (searchy <= dwall[searchx]))
        {
            searchsector = sectnum; searchwall = thewall[z];
            searchstat = 4; searchit = 1;
        }

    if ((globalorientation&128) == 0)
        maskwallscan(xb1[z],xb2[z],uwall,dwall,swall,lwall);
    else
    {
        if (globalorientation&128)
        {
            if (globalorientation&512) settransreverse(); else settransnormal();
        }
        transmaskwallscan(xb1[z],xb2[z]);
    }
}


//
// fillpolygon (internal)
//
static void fillpolygon(int32_t npoints)
{
    int32_t z, zz, x1, y1, x2, y2, miny, maxy, y, xinc, cnt;
    int32_t ox, oy, bx, by, day1, day2;
    int16_t *ptr, *ptr2;
    intptr_t p;

#if defined POLYMOST && defined USE_OPENGL
    if (rendmode >= 3 && qsetmode == 200) { polymost_fillpolygon(npoints); return; }
#endif

    miny = 0x7fffffff; maxy = 0x80000000;
    for (z=npoints-1; z>=0; z--)
        { y = ry1[z]; miny = min(miny,y); maxy = max(maxy,y); }
    miny = (miny>>12); maxy = (maxy>>12);
    if (miny < 0) miny = 0;
    if (maxy >= ydim) maxy = ydim-1;
    ptr = smost;    //They're pointers! - watch how you optimize this thing
    for (y=miny; y<=maxy; y++)
    {
        dotp1[y] = ptr; dotp2[y] = ptr+(MAXNODESPERLINE>>1);
        ptr += MAXNODESPERLINE;
    }

    for (z=npoints-1; z>=0; z--)
    {
        zz = xb1[z];
        y1 = ry1[z]; day1 = (y1>>12);
        y2 = ry1[zz]; day2 = (y2>>12);
        if (day1 != day2)
        {
            x1 = rx1[z]; x2 = rx1[zz];
            xinc = divscale12(x2-x1,y2-y1);
            if (day2 > day1)
            {
                x1 += mulscale12((day1<<12)+4095-y1,xinc);
                for (y=day1; y<day2; y++) { if (!dotp2[y]) { x1 += xinc; continue; } *dotp2[y]++ = (x1>>12); x1 += xinc; }
            }
            else
            {
                x2 += mulscale12((day2<<12)+4095-y2,xinc);
                for (y=day2; y<day1; y++) { if (!dotp1[y]) { x2 += xinc; continue; } *dotp1[y]++ = (x2>>12); x2 += xinc; }
            }
        }
    }

    globalx1 = mulscale16(globalx1,xyaspect);
    globaly2 = mulscale16(globaly2,xyaspect);

    oy = miny+1-(ydim>>1);
    globalposx += oy*globalx1;
    globalposy += oy*globaly2;

    setuphlineasm4(asm1,asm2);

    ptr = smost;
    for (y=miny; y<=maxy; y++)
    {
        cnt = dotp1[y]-ptr; ptr2 = ptr+(MAXNODESPERLINE>>1);
        for (z=cnt-1; z>=0; z--)
        {
            day1 = 0; day2 = 0;
            for (zz=z; zz>0; zz--)
            {
                if (ptr[zz] < ptr[day1]) day1 = zz;
                if (ptr2[zz] < ptr2[day2]) day2 = zz;
            }
            x1 = ptr[day1]; ptr[day1] = ptr[z];
            x2 = ptr2[day2]-1; ptr2[day2] = ptr2[z];
            if (x1 > x2) continue;

            if (globalpolytype < 1)
            {
                //maphline
                ox = x2+1-(xdim>>1);
                bx = ox*asm1 + globalposx;
                by = ox*asm2 - globalposy;

                p = ylookup[y]+x2+frameplace;
                hlineasm4(x2-x1,-1L,globalshade<<8,by,bx,p);
            }
            else
            {
                //maphline
                ox = x1+1-(xdim>>1);
                bx = ox*asm1 + globalposx;
                by = ox*asm2 - globalposy;

                p = ylookup[y]+x1+frameplace;
                if (globalpolytype == 1)
                    mhline(globalbufplc,bx,(x2-x1)<<16,0L,by,p);
                else
                {
                    thline(globalbufplc,bx,(x2-x1)<<16,0L,by,p);
                }
            }
        }
        globalposx += globalx1;
        globalposy += globaly2;
        ptr += MAXNODESPERLINE;
    }
    faketimerhandler();
}


//
// clippoly (internal)
//
static int32_t clippoly(int32_t npoints, int32_t clipstat)
{
    int32_t z, zz, s1, s2, t, npoints2, start2, z1, z2, z3, z4, splitcnt;
    int32_t cx1, cy1, cx2, cy2;

    cx1 = windowx1;
    cy1 = windowy1;
    cx2 = windowx2+1;
    cy2 = windowy2+1;
    cx1 <<= 12; cy1 <<= 12; cx2 <<= 12; cy2 <<= 12;

    if (clipstat&0xa)   //Need to clip top or left
    {
        npoints2 = 0; start2 = 0; z = 0; splitcnt = 0;
        do
        {
            s2 = cx1-rx1[z];
            do
            {
                zz = xb1[z]; xb1[z] = -1;
                s1 = s2; s2 = cx1-rx1[zz];
                if (s1 < 0)
                {
                    rx2[npoints2] = rx1[z]; ry2[npoints2] = ry1[z];
                    xb2[npoints2] = npoints2+1; npoints2++;
                }
                if ((s1^s2) < 0)
                {
                    rx2[npoints2] = rx1[z]+scale(rx1[zz]-rx1[z],s1,s1-s2);
                    ry2[npoints2] = ry1[z]+scale(ry1[zz]-ry1[z],s1,s1-s2);
                    if (s1 < 0) p2[splitcnt++] = npoints2;
                    xb2[npoints2] = npoints2+1;
                    npoints2++;
                }
                z = zz;
            }
            while (xb1[z] >= 0);

            if (npoints2 >= start2+3)
                xb2[npoints2-1] = start2, start2 = npoints2;
            else
                npoints2 = start2;

            z = 1;
            while ((z < npoints) && (xb1[z] < 0)) z++;
        }
        while (z < npoints);
        if (npoints2 <= 2) return(0);

        for (z=1; z<splitcnt; z++)
            for (zz=0; zz<z; zz++)
            {
                z1 = p2[z]; z2 = xb2[z1]; z3 = p2[zz]; z4 = xb2[z3];
                s1  = klabs(rx2[z1]-rx2[z2])+klabs(ry2[z1]-ry2[z2]);
                s1 += klabs(rx2[z3]-rx2[z4])+klabs(ry2[z3]-ry2[z4]);
                s2  = klabs(rx2[z1]-rx2[z4])+klabs(ry2[z1]-ry2[z4]);
                s2 += klabs(rx2[z3]-rx2[z2])+klabs(ry2[z3]-ry2[z2]);
                if (s2 < s1)
                    { t = xb2[p2[z]]; xb2[p2[z]] = xb2[p2[zz]]; xb2[p2[zz]] = t; }
            }


        npoints = 0; start2 = 0; z = 0; splitcnt = 0;
        do
        {
            s2 = cy1-ry2[z];
            do
            {
                zz = xb2[z]; xb2[z] = -1;
                s1 = s2; s2 = cy1-ry2[zz];
                if (s1 < 0)
                {
                    rx1[npoints] = rx2[z]; ry1[npoints] = ry2[z];
                    xb1[npoints] = npoints+1; npoints++;
                }
                if ((s1^s2) < 0)
                {
                    rx1[npoints] = rx2[z]+scale(rx2[zz]-rx2[z],s1,s1-s2);
                    ry1[npoints] = ry2[z]+scale(ry2[zz]-ry2[z],s1,s1-s2);
                    if (s1 < 0) p2[splitcnt++] = npoints;
                    xb1[npoints] = npoints+1;
                    npoints++;
                }
                z = zz;
            }
            while (xb2[z] >= 0);

            if (npoints >= start2+3)
                xb1[npoints-1] = start2, start2 = npoints;
            else
                npoints = start2;

            z = 1;
            while ((z < npoints2) && (xb2[z] < 0)) z++;
        }
        while (z < npoints2);
        if (npoints <= 2) return(0);

        for (z=1; z<splitcnt; z++)
            for (zz=0; zz<z; zz++)
            {
                z1 = p2[z]; z2 = xb1[z1]; z3 = p2[zz]; z4 = xb1[z3];
                s1  = klabs(rx1[z1]-rx1[z2])+klabs(ry1[z1]-ry1[z2]);
                s1 += klabs(rx1[z3]-rx1[z4])+klabs(ry1[z3]-ry1[z4]);
                s2  = klabs(rx1[z1]-rx1[z4])+klabs(ry1[z1]-ry1[z4]);
                s2 += klabs(rx1[z3]-rx1[z2])+klabs(ry1[z3]-ry1[z2]);
                if (s2 < s1)
                    { t = xb1[p2[z]]; xb1[p2[z]] = xb1[p2[zz]]; xb1[p2[zz]] = t; }
            }
    }
    if (clipstat&0x5)   //Need to clip bottom or right
    {
        npoints2 = 0; start2 = 0; z = 0; splitcnt = 0;
        do
        {
            s2 = rx1[z]-cx2;
            do
            {
                zz = xb1[z]; xb1[z] = -1;
                s1 = s2; s2 = rx1[zz]-cx2;
                if (s1 < 0)
                {
                    rx2[npoints2] = rx1[z]; ry2[npoints2] = ry1[z];
                    xb2[npoints2] = npoints2+1; npoints2++;
                }
                if ((s1^s2) < 0)
                {
                    rx2[npoints2] = rx1[z]+scale(rx1[zz]-rx1[z],s1,s1-s2);
                    ry2[npoints2] = ry1[z]+scale(ry1[zz]-ry1[z],s1,s1-s2);
                    if (s1 < 0) p2[splitcnt++] = npoints2;
                    xb2[npoints2] = npoints2+1;
                    npoints2++;
                }
                z = zz;
            }
            while (xb1[z] >= 0);

            if (npoints2 >= start2+3)
                xb2[npoints2-1] = start2, start2 = npoints2;
            else
                npoints2 = start2;

            z = 1;
            while ((z < npoints) && (xb1[z] < 0)) z++;
        }
        while (z < npoints);
        if (npoints2 <= 2) return(0);

        for (z=1; z<splitcnt; z++)
            for (zz=0; zz<z; zz++)
            {
                z1 = p2[z]; z2 = xb2[z1]; z3 = p2[zz]; z4 = xb2[z3];
                s1  = klabs(rx2[z1]-rx2[z2])+klabs(ry2[z1]-ry2[z2]);
                s1 += klabs(rx2[z3]-rx2[z4])+klabs(ry2[z3]-ry2[z4]);
                s2  = klabs(rx2[z1]-rx2[z4])+klabs(ry2[z1]-ry2[z4]);
                s2 += klabs(rx2[z3]-rx2[z2])+klabs(ry2[z3]-ry2[z2]);
                if (s2 < s1)
                    { t = xb2[p2[z]]; xb2[p2[z]] = xb2[p2[zz]]; xb2[p2[zz]] = t; }
            }


        npoints = 0; start2 = 0; z = 0; splitcnt = 0;
        do
        {
            s2 = ry2[z]-cy2;
            do
            {
                zz = xb2[z]; xb2[z] = -1;
                s1 = s2; s2 = ry2[zz]-cy2;
                if (s1 < 0)
                {
                    rx1[npoints] = rx2[z]; ry1[npoints] = ry2[z];
                    xb1[npoints] = npoints+1; npoints++;
                }
                if ((s1^s2) < 0)
                {
                    rx1[npoints] = rx2[z]+scale(rx2[zz]-rx2[z],s1,s1-s2);
                    ry1[npoints] = ry2[z]+scale(ry2[zz]-ry2[z],s1,s1-s2);
                    if (s1 < 0) p2[splitcnt++] = npoints;
                    xb1[npoints] = npoints+1;
                    npoints++;
                }
                z = zz;
            }
            while (xb2[z] >= 0);

            if (npoints >= start2+3)
                xb1[npoints-1] = start2, start2 = npoints;
            else
                npoints = start2;

            z = 1;
            while ((z < npoints2) && (xb2[z] < 0)) z++;
        }
        while (z < npoints2);
        if (npoints <= 2) return(0);

        for (z=1; z<splitcnt; z++)
            for (zz=0; zz<z; zz++)
            {
                z1 = p2[z]; z2 = xb1[z1]; z3 = p2[zz]; z4 = xb1[z3];
                s1  = klabs(rx1[z1]-rx1[z2])+klabs(ry1[z1]-ry1[z2]);
                s1 += klabs(rx1[z3]-rx1[z4])+klabs(ry1[z3]-ry1[z4]);
                s2  = klabs(rx1[z1]-rx1[z4])+klabs(ry1[z1]-ry1[z4]);
                s2 += klabs(rx1[z3]-rx1[z2])+klabs(ry1[z3]-ry1[z2]);
                if (s2 < s1)
                    { t = xb1[p2[z]]; xb1[p2[z]] = xb1[p2[zz]]; xb1[p2[zz]] = t; }
            }
    }
    return(npoints);
}


//
// clippoly4 (internal)
//
//Assume npoints=4 with polygon on &nrx1,&nry1
//JBF 20031206: Thanks to Ken's hunting, s/(rx1|ry1|rx2|ry2)/n\1/ in this function
static int32_t clippoly4(int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2)
{
    int32_t n, nn, z, zz, x, x1, x2, y, y1, y2, t;

    nn = 0; z = 0;
    do
    {
        zz = ((z+1)&3);
        x1 = nrx1[z]; x2 = nrx1[zz]-x1;

        if ((cx1 <= x1) && (x1 <= cx2))
            nrx2[nn] = x1, nry2[nn] = nry1[z], nn++;

        if (x2 <= 0) x = cx2; else x = cx1;
        t = x-x1;
        if (((t-x2)^t) < 0)
            nrx2[nn] = x, nry2[nn] = nry1[z]+scale(t,nry1[zz]-nry1[z],x2), nn++;

        if (x2 <= 0) x = cx1; else x = cx2;
        t = x-x1;
        if (((t-x2)^t) < 0)
            nrx2[nn] = x, nry2[nn] = nry1[z]+scale(t,nry1[zz]-nry1[z],x2), nn++;

        z = zz;
    }
    while (z != 0);
    if (nn < 3) return(0);

    n = 0; z = 0;
    do
    {
        zz = z+1; if (zz == nn) zz = 0;
        y1 = nry2[z]; y2 = nry2[zz]-y1;

        if ((cy1 <= y1) && (y1 <= cy2))
            nry1[n] = y1, nrx1[n] = nrx2[z], n++;

        if (y2 <= 0) y = cy2; else y = cy1;
        t = y-y1;
        if (((t-y2)^t) < 0)
            nry1[n] = y, nrx1[n] = nrx2[z]+scale(t,nrx2[zz]-nrx2[z],y2), n++;

        if (y2 <= 0) y = cy1; else y = cy2;
        t = y-y1;
        if (((t-y2)^t) < 0)
            nry1[n] = y, nrx1[n] = nrx2[z]+scale(t,nrx2[zz]-nrx2[z],y2), n++;

        z = zz;
    }
    while (z != 0);
    return(n);
}


//
// dorotatesprite (internal)
//
//JBF 20031206: Thanks to Ken's hunting, s/(rx1|ry1|rx2|ry2)/n\1/ in this function
static void dorotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum, int8_t dashade, char dapalnum, char dastat, int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2, int32_t uniqid)
{
    int32_t cosang, sinang, v, nextv, dax1, dax2, oy, bx, by;
    int32_t i, x, y, x1, y1, x2, y2, gx1, gy1 ;
    intptr_t p, bufplc, palookupoffs;
    int32_t xsiz, ysiz, xoff, yoff, npoints, yplc, yinc, lx, rx;
    int32_t xv, yv, xv2, yv2;
#ifndef ENGINE_USING_A_C
    char bad;
    int32_t ny1, ny2, xx, xend;
    int32_t qlinemode=0, y1ve[4], y2ve[4], u4, d4;
#endif

    UNREFERENCED_PARAMETER(uniqid);
    //============================================================================= //POLYMOST BEGINS
#ifdef POLYMOST
    if (rendmode >= 3 && qsetmode == 200) { polymost_dorotatesprite(sx,sy,z,a,picnum,dashade,dapalnum,dastat,cx1,cy1,cx2,cy2,uniqid); return; }
# ifdef POLYMER
    if (rendmode == 4 && qsetmode == 200) { polymer_rotatesprite(sx,sy,z,a,picnum,dashade,dapalnum,dastat,cx1,cy1,cx2,cy2); return; }
#endif
#endif
    //============================================================================= //POLYMOST ENDS

    if (cx1 < 0) cx1 = 0;
    if (cy1 < 0) cy1 = 0;
    if (cx2 > xres-1) cx2 = xres-1;
    if (cy2 > yres-1) cy2 = yres-1;

    xsiz = tilesizx[picnum]; ysiz = tilesizy[picnum];
    if (dastat&16) { xoff = 0; yoff = 0; }
    else
    {
        xoff = (int32_t)((int8_t)((picanm[picnum]>>8)&255))+(xsiz>>1);
        yoff = (int32_t)((int8_t)((picanm[picnum]>>16)&255))+(ysiz>>1);
    }

    if (dastat&4) yoff = ysiz-yoff;

    cosang = sintable[(a+512)&2047]; sinang = sintable[a&2047];

    if ((dastat&2) != 0)  //Auto window size scaling
    {
        if ((dastat&8) == 0)
        {
            x = xdimenscale;   //= scale(xdimen,yxaspect,320);
            sx = ((cx1+cx2+2)<<15)+scale(sx-(320<<15),xdimen,320);
            sy = ((cy1+cy2+2)<<15)+mulscale16(sy-(200<<15),x);
        }
        else
        {
            //If not clipping to startmosts, & auto-scaling on, as a
            //hard-coded bonus, scale to full screen instead
            x = scale(xdim,yxaspect,320);
            sx = (xdim<<15)+32768+scale(sx-(320<<15),xdim,320);
            sy = (ydim<<15)+32768+mulscale16(sy-(200<<15),x);
        }
        z = mulscale16(z,x);
    }

    xv = mulscale14(cosang,z);
    yv = mulscale14(sinang,z);
    if (((dastat&2) != 0) || ((dastat&8) == 0)) //Don't aspect unscaled perms
    {
        xv2 = mulscale16(xv,xyaspect);
        yv2 = mulscale16(yv,xyaspect);
    }
    else
    {
        xv2 = xv;
        yv2 = yv;
    }

    nry1[0] = sy - (yv*xoff + xv*yoff);
    nry1[1] = nry1[0] + yv*xsiz;
    nry1[3] = nry1[0] + xv*ysiz;
    nry1[2] = nry1[1]+nry1[3]-nry1[0];
    i = (cy1<<16); if ((nry1[0]<i) && (nry1[1]<i) && (nry1[2]<i) && (nry1[3]<i)) return;
    i = (cy2<<16); if ((nry1[0]>i) && (nry1[1]>i) && (nry1[2]>i) && (nry1[3]>i)) return;

    nrx1[0] = sx - (xv2*xoff - yv2*yoff);
    nrx1[1] = nrx1[0] + xv2*xsiz;
    nrx1[3] = nrx1[0] - yv2*ysiz;
    nrx1[2] = nrx1[1]+nrx1[3]-nrx1[0];
    i = (cx1<<16); if ((nrx1[0]<i) && (nrx1[1]<i) && (nrx1[2]<i) && (nrx1[3]<i)) return;
    i = (cx2<<16); if ((nrx1[0]>i) && (nrx1[1]>i) && (nrx1[2]>i) && (nrx1[3]>i)) return;

    gx1 = nrx1[0]; gy1 = nry1[0];   //back up these before clipping

    if ((npoints = clippoly4(cx1<<16,cy1<<16,(cx2+1)<<16,(cy2+1)<<16)) < 3) return;

    lx = nrx1[0]; rx = nrx1[0];

    nextv = 0;
    for (v=npoints-1; v>=0; v--)
    {
        x1 = nrx1[v]; x2 = nrx1[nextv];
        dax1 = (x1>>16); if (x1 < lx) lx = x1;
        dax2 = (x2>>16); if (x1 > rx) rx = x1;
        if (dax1 != dax2)
        {
            y1 = nry1[v]; y2 = nry1[nextv];
            yinc = divscale16(y2-y1,x2-x1);
            if (dax2 > dax1)
            {
                yplc = y1 + mulscale16((dax1<<16)+65535-x1,yinc);
                qinterpolatedown16short((intptr_t)(&uplc[dax1]),dax2-dax1,yplc,yinc);
            }
            else
            {
                yplc = y2 + mulscale16((dax2<<16)+65535-x2,yinc);
                qinterpolatedown16short((intptr_t)(&dplc[dax2]),dax1-dax2,yplc,yinc);
            }
        }
        nextv = v;
    }

    if (waloff[picnum] == 0) loadtile(picnum);
    setgotpic(picnum);
    bufplc = waloff[picnum];

    if (palookup[dapalnum] == NULL) dapalnum = 0;
    palookupoffs = FP_OFF(palookup[dapalnum]) + (getpalookup(0L,(int32_t)dashade)<<8);

    i = divscale32(1L,z);
    xv = mulscale14(sinang,i);
    yv = mulscale14(cosang,i);
    if (((dastat&2) != 0) || ((dastat&8) == 0)) //Don't aspect unscaled perms
    {
        yv2 = mulscale16(-xv,yxaspect);
        xv2 = mulscale16(yv,yxaspect);
    }
    else
    {
        yv2 = -xv;
        xv2 = yv;
    }

    x1 = (lx>>16); x2 = (rx>>16);

    oy = 0;
    x = (x1<<16)-1-gx1; y = (oy<<16)+65535-gy1;
    bx = dmulscale16(x,xv2,y,xv);
    by = dmulscale16(x,yv2,y,yv);
    if (dastat&4) { yv = -yv; yv2 = -yv2; by = (ysiz<<16)-1-by; }

    /*  if (origbuffermode == 0)
        {
            if (dastat&128)
            {
                obuffermode = buffermode;
                buffermode = 0;
                setactivepage(activepage);
            }
        }
        else if (dastat&8)
             permanentupdate = 1; */

#ifndef ENGINE_USING_A_C

    if ((dastat&1) == 0)
    {
        if (((a&1023) == 0) && (ysiz <= 256))  //vlineasm4 has 256 high limit!
        {
            if (dastat&64) setupvlineasm(24L); else setupmvlineasm(24L);
            by <<= 8; yv <<= 8; yv2 <<= 8;

            palookupoffse[0] = palookupoffse[1] = palookupoffse[2] = palookupoffse[3] = palookupoffs;
            vince[0] = vince[1] = vince[2] = vince[3] = yv;

            for (x=x1; x<x2; x+=4)
            {
                bad = 15; xend = min(x2-x,4);
                for (xx=0; xx<xend; xx++)
                {
                    bx += xv2;

                    y1 = uplc[x+xx]; y2 = dplc[x+xx];
                    if ((dastat&8) == 0)
                    {
                        if (startumost[x+xx] > y1) y1 = startumost[x+xx];
                        if (startdmost[x+xx] < y2) y2 = startdmost[x+xx];
                    }
                    if (y2 <= y1) continue;

                    by += yv*(y1-oy); oy = y1;

                    bufplce[xx] = (bx>>16)*ysiz+bufplc;
                    vplce[xx] = by;
                    y1ve[xx] = y1;
                    y2ve[xx] = y2-1;
                    bad &= ~pow2char[xx];
                }

                p = x+frameplace;

                u4 = max(max(y1ve[0],y1ve[1]),max(y1ve[2],y1ve[3]));
                d4 = min(min(y2ve[0],y2ve[1]),min(y2ve[2],y2ve[3]));

                if (dastat&64)
                {
                    if ((bad != 0) || (u4 >= d4))
                    {
                        if (!(bad&1)) prevlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0],vplce[0],bufplce[0],ylookup[y1ve[0]]+p+0);
                        if (!(bad&2)) prevlineasm1(vince[1],palookupoffse[1],y2ve[1]-y1ve[1],vplce[1],bufplce[1],ylookup[y1ve[1]]+p+1);
                        if (!(bad&4)) prevlineasm1(vince[2],palookupoffse[2],y2ve[2]-y1ve[2],vplce[2],bufplce[2],ylookup[y1ve[2]]+p+2);
                        if (!(bad&8)) prevlineasm1(vince[3],palookupoffse[3],y2ve[3]-y1ve[3],vplce[3],bufplce[3],ylookup[y1ve[3]]+p+3);
                        continue;
                    }

                    if (u4 > y1ve[0]) vplce[0] = prevlineasm1(vince[0],palookupoffse[0],u4-y1ve[0]-1,vplce[0],bufplce[0],ylookup[y1ve[0]]+p+0);
                    if (u4 > y1ve[1]) vplce[1] = prevlineasm1(vince[1],palookupoffse[1],u4-y1ve[1]-1,vplce[1],bufplce[1],ylookup[y1ve[1]]+p+1);
                    if (u4 > y1ve[2]) vplce[2] = prevlineasm1(vince[2],palookupoffse[2],u4-y1ve[2]-1,vplce[2],bufplce[2],ylookup[y1ve[2]]+p+2);
                    if (u4 > y1ve[3]) vplce[3] = prevlineasm1(vince[3],palookupoffse[3],u4-y1ve[3]-1,vplce[3],bufplce[3],ylookup[y1ve[3]]+p+3);

                    if (d4 >= u4) vlineasm4(d4-u4+1,ylookup[u4]+p);

                    i = p+ylookup[d4+1];
                    if (y2ve[0] > d4) prevlineasm1(vince[0],palookupoffse[0],y2ve[0]-d4-1,vplce[0],bufplce[0],i+0);
                    if (y2ve[1] > d4) prevlineasm1(vince[1],palookupoffse[1],y2ve[1]-d4-1,vplce[1],bufplce[1],i+1);
                    if (y2ve[2] > d4) prevlineasm1(vince[2],palookupoffse[2],y2ve[2]-d4-1,vplce[2],bufplce[2],i+2);
                    if (y2ve[3] > d4) prevlineasm1(vince[3],palookupoffse[3],y2ve[3]-d4-1,vplce[3],bufplce[3],i+3);
                }
                else
                {
                    if ((bad != 0) || (u4 >= d4))
                    {
                        if (!(bad&1)) mvlineasm1(vince[0],palookupoffse[0],y2ve[0]-y1ve[0],vplce[0],bufplce[0],ylookup[y1ve[0]]+p+0);
                        if (!(bad&2)) mvlineasm1(vince[1],palookupoffse[1],y2ve[1]-y1ve[1],vplce[1],bufplce[1],ylookup[y1ve[1]]+p+1);
                        if (!(bad&4)) mvlineasm1(vince[2],palookupoffse[2],y2ve[2]-y1ve[2],vplce[2],bufplce[2],ylookup[y1ve[2]]+p+2);
                        if (!(bad&8)) mvlineasm1(vince[3],palookupoffse[3],y2ve[3]-y1ve[3],vplce[3],bufplce[3],ylookup[y1ve[3]]+p+3);
                        continue;
                    }

                    if (u4 > y1ve[0]) vplce[0] = mvlineasm1(vince[0],palookupoffse[0],u4-y1ve[0]-1,vplce[0],bufplce[0],ylookup[y1ve[0]]+p+0);
                    if (u4 > y1ve[1]) vplce[1] = mvlineasm1(vince[1],palookupoffse[1],u4-y1ve[1]-1,vplce[1],bufplce[1],ylookup[y1ve[1]]+p+1);
                    if (u4 > y1ve[2]) vplce[2] = mvlineasm1(vince[2],palookupoffse[2],u4-y1ve[2]-1,vplce[2],bufplce[2],ylookup[y1ve[2]]+p+2);
                    if (u4 > y1ve[3]) vplce[3] = mvlineasm1(vince[3],palookupoffse[3],u4-y1ve[3]-1,vplce[3],bufplce[3],ylookup[y1ve[3]]+p+3);

                    if (d4 >= u4) mvlineasm4(d4-u4+1,ylookup[u4]+p);

                    i = p+ylookup[d4+1];
                    if (y2ve[0] > d4) mvlineasm1(vince[0],palookupoffse[0],y2ve[0]-d4-1,vplce[0],bufplce[0],i+0);
                    if (y2ve[1] > d4) mvlineasm1(vince[1],palookupoffse[1],y2ve[1]-d4-1,vplce[1],bufplce[1],i+1);
                    if (y2ve[2] > d4) mvlineasm1(vince[2],palookupoffse[2],y2ve[2]-d4-1,vplce[2],bufplce[2],i+2);
                    if (y2ve[3] > d4) mvlineasm1(vince[3],palookupoffse[3],y2ve[3]-d4-1,vplce[3],bufplce[3],i+3);
                }

                faketimerhandler();
            }
        }
        else
        {
            if (dastat&64)
            {
                if ((xv2&0x0000ffff) == 0)
                {
                    qlinemode = 1;
                    setupqrhlineasm4(0L,yv2<<16,(xv2>>16)*ysiz+(yv2>>16),palookupoffs,0L,0L);
                }
                else
                {
                    qlinemode = 0;
                    setuprhlineasm4(xv2<<16,yv2<<16,(xv2>>16)*ysiz+(yv2>>16),palookupoffs,ysiz,0L);
                }
            }
            else
                setuprmhlineasm4(xv2<<16,yv2<<16,(xv2>>16)*ysiz+(yv2>>16),palookupoffs,ysiz,0L);

            y1 = uplc[x1];
            if (((dastat&8) == 0) && (startumost[x1] > y1)) y1 = startumost[x1];
            y2 = y1;
            for (x=x1; x<x2; x++)
            {
                ny1 = uplc[x]-1; ny2 = dplc[x];
                if ((dastat&8) == 0)
                {
                    if (startumost[x]-1 > ny1) ny1 = startumost[x]-1;
                    if (startdmost[x] < ny2) ny2 = startdmost[x];
                }

                if (ny1 < ny2-1)
                {
                    if (ny1 >= y2)
                    {
                        while (y1 < y2-1)
                        {
                            y1++; if ((y1&31) == 0) faketimerhandler();

                            //x,y1
                            bx += xv*(y1-oy); by += yv*(y1-oy); oy = y1;
                            if (dastat&64)
                            {
                                if (qlinemode) qrhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,0L    ,by<<16,ylookup[y1]+x+frameplace);
                                else rhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y1]+x+frameplace);
                            }
                            else rmhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y1]+x+frameplace);
                        }
                        y1 = ny1;
                    }
                    else
                    {
                        while (y1 < ny1)
                        {
                            y1++; if ((y1&31) == 0) faketimerhandler();

                            //x,y1
                            bx += xv*(y1-oy); by += yv*(y1-oy); oy = y1;
                            if (dastat&64)
                            {
                                if (qlinemode) qrhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,0L    ,by<<16,ylookup[y1]+x+frameplace);
                                else rhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y1]+x+frameplace);
                            }
                            else rmhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y1]+x+frameplace);
                        }
                        while (y1 > ny1) lastx[y1--] = x;
                    }
                    while (y2 > ny2)
                    {
                        y2--; if ((y2&31) == 0) faketimerhandler();

                        //x,y2
                        bx += xv*(y2-oy); by += yv*(y2-oy); oy = y2;
                        if (dastat&64)
                        {
                            if (qlinemode) qrhlineasm4(x-lastx[y2],(bx>>16)*ysiz+(by>>16)+bufplc,0L,0L    ,by<<16,ylookup[y2]+x+frameplace);
                            else rhlineasm4(x-lastx[y2],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y2]+x+frameplace);
                        }
                        else rmhlineasm4(x-lastx[y2],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y2]+x+frameplace);
                    }
                    while (y2 < ny2) lastx[y2++] = x;
                }
                else
                {
                    while (y1 < y2-1)
                    {
                        y1++; if ((y1&31) == 0) faketimerhandler();

                        //x,y1
                        bx += xv*(y1-oy); by += yv*(y1-oy); oy = y1;
                        if (dastat&64)
                        {
                            if (qlinemode) qrhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,0L    ,by<<16,ylookup[y1]+x+frameplace);
                            else rhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y1]+x+frameplace);
                        }
                        else rmhlineasm4(x-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y1]+x+frameplace);
                    }
                    if (x == x2-1) { bx += xv2; by += yv2; break; }
                    y1 = uplc[x+1];
                    if (((dastat&8) == 0) && (startumost[x+1] > y1)) y1 = startumost[x+1];
                    y2 = y1;
                }
                bx += xv2; by += yv2;
            }
            while (y1 < y2-1)
            {
                y1++; if ((y1&31) == 0) faketimerhandler();

                //x2,y1
                bx += xv*(y1-oy); by += yv*(y1-oy); oy = y1;
                if (dastat&64)
                {
                    if (qlinemode) qrhlineasm4(x2-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,0L,by<<16,ylookup[y1]+x2+frameplace);
                    else rhlineasm4(x2-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y1]+x2+frameplace);
                }
                else rmhlineasm4(x2-lastx[y1],(bx>>16)*ysiz+(by>>16)+bufplc,0L,bx<<16,by<<16,ylookup[y1]+x2+frameplace);
            }
        }
    }
    else
    {
        if ((dastat&1) == 0)
        {
            if (dastat&64)
                setupspritevline(palookupoffs,(xv>>16)*ysiz,xv<<16,ysiz,yv,0L);
            else
                msetupspritevline(palookupoffs,(xv>>16)*ysiz,xv<<16,ysiz,yv,0L);
        }
        else
        {
            tsetupspritevline(palookupoffs,(xv>>16)*ysiz,xv<<16,ysiz,yv,0L);
            if (dastat&32) settransreverse(); else settransnormal();
        }
        for (x=x1; x<x2; x++)
        {
            bx += xv2; by += yv2;

            y1 = uplc[x]; y2 = dplc[x];
            if ((dastat&8) == 0)
            {
                if (startumost[x] > y1) y1 = startumost[x];
                if (startdmost[x] < y2) y2 = startdmost[x];
            }
            if (y2 <= y1) continue;

            switch (y1-oy)
            {
            case -1:
                bx -= xv; by -= yv; oy = y1; break;
            case 0:
                break;
            case 1:
                bx += xv; by += yv; oy = y1; break;
            default:
                bx += xv*(y1-oy); by += yv*(y1-oy); oy = y1; break;
            }

            p = ylookup[y1]+x+frameplace;

            if ((dastat&1) == 0)
            {
                if (dastat&64)
                    spritevline(0L,by<<16,y2-y1+1,bx<<16,(bx>>16)*ysiz+(by>>16)+bufplc,p);
                else
                    mspritevline(0L,by<<16,y2-y1+1,bx<<16,(bx>>16)*ysiz+(by>>16)+bufplc,p);
            }
            else
            {
                tspritevline(0L,by<<16,y2-y1+1,bx<<16,(bx>>16)*ysiz+(by>>16)+bufplc,p);
            }
            faketimerhandler();
        }
    }

#else   // ENGINE_USING_A_C

    if ((dastat&1) == 0)
    {
        if (dastat&64)
            setupspritevline(palookupoffs,xv,yv,ysiz);
        else
            msetupspritevline(palookupoffs,xv,yv,ysiz);
    }
    else
    {
        tsetupspritevline(palookupoffs,xv,yv,ysiz);
        if (dastat&32) settransreverse(); else settransnormal();
    }
    for (x=x1; x<x2; x++)
    {
        bx += xv2; by += yv2;

        y1 = uplc[x]; y2 = dplc[x];
        if ((dastat&8) == 0)
        {
            if (startumost[x] > y1) y1 = startumost[x];
            if (startdmost[x] < y2) y2 = startdmost[x];
        }
        if (y2 <= y1) continue;

        switch (y1-oy)
        {
        case -1:
            bx -= xv; by -= yv; oy = y1; break;
        case 0:
            break;
        case 1:
            bx += xv; by += yv; oy = y1; break;
        default:
            bx += xv*(y1-oy); by += yv*(y1-oy); oy = y1; break;
        }

        p = ylookup[y1]+x+frameplace;

        if ((dastat&1) == 0)
        {
            if (dastat&64)
                spritevline(bx&65535,by&65535,y2-y1+1,(bx>>16)*ysiz+(by>>16)+bufplc,p);
            else
                mspritevline(bx&65535,by&65535,y2-y1+1,(bx>>16)*ysiz+(by>>16)+bufplc,p);
        }
        else
        {
            tspritevline(bx&65535,by&65535,y2-y1+1,(bx>>16)*ysiz+(by>>16)+bufplc,p);
            //transarea += (y2-y1);
        }
        faketimerhandler();
    }

#endif

    /*  if ((dastat&128) && (origbuffermode == 0))
        {
            buffermode = obuffermode;
            setactivepage(activepage);
        }*/
}


//
// initksqrt (internal)
//
static inline void initksqrt(void)
{
    int32_t i, j, k;

    j = 1; k = 0;
    for (i=0; i<4096; i++)
    {
        if (i >= j) { j <<= 2; k++; }
        sqrtable[i] = (uint16_t)(msqrtasm((i<<18)+131072)<<1);
        shlookup[i] = (k<<1)+((10-k)<<8);
        if (i < 256) shlookup[i+4096] = ((k+6)<<1)+((10-(k+6))<<8);
    }
}


//
// dosetaspect
//
static void dosetaspect(void)
{
    int32_t i, j, k, x, xinc;

    if (xyaspect != oxyaspect)
    {
        oxyaspect = xyaspect;
        j = xyaspect*320;
        horizlookup2[horizycent-1] = divscale26(131072,j);
        for (i=ydim*4-1; i>=0; i--)
            if (i != (horizycent-1))
            {
                horizlookup[i] = divscale28(1,i-(horizycent-1));
                horizlookup2[i] = divscale14(klabs(horizlookup[i]),j);
            }
    }
    if ((xdimen != oxdimen) || (viewingrange != oviewingrange))
    {
        oxdimen = xdimen;
        oviewingrange = viewingrange;
        xinc = mulscale32(viewingrange*320,xdimenrecip);
        x = (640<<16)-mulscale1(xinc,xdimen);
        for (i=0; i<xdimen; i++)
        {
            j = (x&65535); k = (x>>16); x += xinc;
            if (j != 0) j = mulscale16((int32_t)radarang[k+1]-(int32_t)radarang[k],j);
            radarang2[i] = (int16_t)(((int32_t)radarang[k]+j)>>6);
        }
#ifdef SUPERBUILD
        for (i=1; i<65536; i++) distrecip[i] = divscale20(xdimen,i);
        nytooclose = xdimen*2100;
        nytoofar = 65536*16384-1048576;
#endif
    }
}


//
// loadtables (internal)
//
static inline void calcbritable(void)
{
    int32_t i,j;
    double a,b;
    for (i=0; i<16; i++)
    {
        a = (double)8 / ((double)i+8);
        b = (double)255 / pow((double)255,a);
        for (j=0; j<256; j++) // JBF 20040207: full 8bit precision
            britable[i][j] = (uint8_t)(pow((double)j,a)*b);
    }
}

static int32_t loadtables(void)
{
    int32_t i, fil;

    if (tablesloaded == 0)
    {
        initksqrt();

        for (i=0; i<2048; i++) reciptable[i] = divscale30(2048L,i+2048);

        if ((fil = kopen4load("tables.dat",0)) != -1)
        {
            kread(fil,sintable,2048*2); for (i=2048-1; i>=0; i--) sintable[i] = B_LITTLE16(sintable[i]);
            kread(fil,radarang,640*2);  for (i=640-1;  i>=0; i--) radarang[i] = B_LITTLE16(radarang[i]);
            for (i=0; i<640; i++) radarang[1279-i] = -radarang[i];
            //kread(fil,textfont,1024);
            //kread(fil,smalltextfont,1024);
            //kread(fil,britable,1024);
            calcbritable();

            kclose(fil);
        }
        else
        {
            engineerrstr = "Failed to load TABLES.DAT!";
            initprintf("ERROR: %s\n", engineerrstr);
            return 1;
        }
        tablesloaded = 1;
    }

    return 0;
}


//
// initfastcolorlookup (internal)
//
static void initfastcolorlookup(int32_t rscale, int32_t gscale, int32_t bscale)
{
    int32_t i, j, x, y, z;
    char *pal1;

    j = 0;
    for (i=64; i>=0; i--)
    {
        //j = (i-64)*(i-64);
        rdist[i] = rdist[128-i] = j*rscale;
        gdist[i] = gdist[128-i] = j*gscale;
        bdist[i] = bdist[128-i] = j*bscale;
        j += 129-(i<<1);
    }

    //clearbufbyte(colhere,sizeof(colhere),0L);
    //clearbufbyte(colhead,sizeof(colhead),0L);
    Bmemset(colhere,0,sizeof(colhere));
    Bmemset(colhead,0,sizeof(colhead));

    pal1 = (char *)&palette[768-3];
    for (i=255; i>=0; i--,pal1-=3)
    {
        j = (pal1[0]>>3)*FASTPALGRIDSIZ*FASTPALGRIDSIZ+(pal1[1]>>3)*FASTPALGRIDSIZ+(pal1[2]>>3)+FASTPALGRIDSIZ*FASTPALGRIDSIZ+FASTPALGRIDSIZ+1;
        if (colhere[j>>3]&pow2char[j&7]) colnext[i] = colhead[j]; else colnext[i] = -1;
        colhead[j] = i;
        colhere[j>>3] |= pow2char[j&7];
    }

    i = 0;
    for (x=-FASTPALGRIDSIZ*FASTPALGRIDSIZ; x<=FASTPALGRIDSIZ*FASTPALGRIDSIZ; x+=FASTPALGRIDSIZ*FASTPALGRIDSIZ)
        for (y=-FASTPALGRIDSIZ; y<=FASTPALGRIDSIZ; y+=FASTPALGRIDSIZ)
            for (z=-1; z<=1; z++)
                colscan[i++] = x+y+z;
    i = colscan[13]; colscan[13] = colscan[26]; colscan[26] = i;
}


//
// loadpalette (internal)
//
static void loadpalette(void)
{
    int32_t fil;

    if (paletteloaded != 0) return;
    if ((fil = kopen4load("palette.dat",0)) == -1) return;

    kread(fil,palette,768);
    kread(fil,&numpalookups,2); numpalookups = B_LITTLE16(numpalookups);

    if ((palookup[0] = (char *)kkmalloc(numpalookups<<8)) == NULL)
        allocache((intptr_t*)&palookup[0],numpalookups<<8,&permanentlock);
    if ((transluc = (char *)kkmalloc(65536L)) == NULL)
        allocache((intptr_t*)&transluc,65536,&permanentlock);

    globalpalwritten = palookup[0]; globalpal = 0;
    setpalookupaddress(globalpalwritten);

    fixtransluscence(FP_OFF(transluc));

    kread(fil,palookup[globalpal],numpalookups<<8);
    kread(fil,transluc,65536);
    kclose(fil);

    initfastcolorlookup(30L,59L,11L);

    paletteloaded = 1;
}


//
// getclosestcol (internal)
//
int32_t getclosestcol(int32_t r, int32_t g, int32_t b)
{
    int32_t i, j, k, dist, mindist, retcol;
    char *pal1;

    j = (r>>3)*FASTPALGRIDSIZ*FASTPALGRIDSIZ+(g>>3)*FASTPALGRIDSIZ+(b>>3)+FASTPALGRIDSIZ*FASTPALGRIDSIZ+FASTPALGRIDSIZ+1;
    mindist = min(rdist[coldist[r&7]+64+8],gdist[coldist[g&7]+64+8]);
    mindist = min(mindist,bdist[coldist[b&7]+64+8]);
    mindist++;

    r = 64-r; g = 64-g; b = 64-b;

    retcol = -1;
    for (k=26; k>=0; k--)
    {
        i = colscan[k]+j; if ((colhere[i>>3]&pow2char[i&7]) == 0) continue;
        i = colhead[i];
        do
        {
            pal1 = (char *)&palette[i*3];
            dist = gdist[pal1[1]+g];
            if (dist < mindist)
            {
                dist += rdist[pal1[0]+r];
                if (dist < mindist)
                {
                    dist += bdist[pal1[2]+b];
                    if (dist < mindist) { mindist = dist; retcol = i; }
                }
            }
            i = colnext[i];
        }
        while (i >= 0);
    }
    if (retcol >= 0) return(retcol);

    mindist = 0x7fffffff;
    pal1 = (char *)&palette[768-3];
    for (i=255; i>=0; i--,pal1-=3)
    {
        dist = gdist[pal1[1]+g]; if (dist >= mindist) continue;
        dist += rdist[pal1[0]+r]; if (dist >= mindist) continue;
        dist += bdist[pal1[2]+b]; if (dist >= mindist) continue;
        mindist = dist; retcol = i;
    }
    return(retcol);
}


//
// insertspritesect (internal)
//
int32_t insertspritesect(int16_t sectnum)
{
    int16_t blanktouse;

    if ((sectnum >= MAXSECTORS) || (headspritesect[MAXSECTORS] == -1))
        return(-1);  //list full

    blanktouse = headspritesect[MAXSECTORS];

    headspritesect[MAXSECTORS] = nextspritesect[blanktouse];
    if (headspritesect[MAXSECTORS] >= 0)
        prevspritesect[headspritesect[MAXSECTORS]] = -1;

    prevspritesect[blanktouse] = -1;
    nextspritesect[blanktouse] = headspritesect[sectnum];
    if (headspritesect[sectnum] >= 0)
        prevspritesect[headspritesect[sectnum]] = blanktouse;
    headspritesect[sectnum] = blanktouse;

    sprite[blanktouse].sectnum = sectnum;

    return(blanktouse);
}


//
// insertspritestat (internal)
//
int32_t insertspritestat(int16_t statnum)
{
    int16_t blanktouse;

    if ((statnum >= MAXSTATUS) || (headspritestat[MAXSTATUS] == -1))
        return(-1);  //list full

    blanktouse = headspritestat[MAXSTATUS];

    headspritestat[MAXSTATUS] = nextspritestat[blanktouse];
    if (headspritestat[MAXSTATUS] >= 0)
        prevspritestat[headspritestat[MAXSTATUS]] = -1;

    prevspritestat[blanktouse] = -1;
    nextspritestat[blanktouse] = headspritestat[statnum];
    if (headspritestat[statnum] >= 0)
        prevspritestat[headspritestat[statnum]] = blanktouse;
    headspritestat[statnum] = blanktouse;

    sprite[blanktouse].statnum = statnum;

    return(blanktouse);
}


//
// deletespritesect (internal)
//
int32_t deletespritesect(int16_t deleteme)
{
    if (sprite[deleteme].sectnum == MAXSECTORS)
        return(-1);

    if (headspritesect[sprite[deleteme].sectnum] == deleteme)
        headspritesect[sprite[deleteme].sectnum] = nextspritesect[deleteme];

    if (prevspritesect[deleteme] >= 0) nextspritesect[prevspritesect[deleteme]] = nextspritesect[deleteme];
    if (nextspritesect[deleteme] >= 0) prevspritesect[nextspritesect[deleteme]] = prevspritesect[deleteme];

    if (headspritesect[MAXSECTORS] >= 0) prevspritesect[headspritesect[MAXSECTORS]] = deleteme;
    prevspritesect[deleteme] = -1;
    nextspritesect[deleteme] = headspritesect[MAXSECTORS];
    headspritesect[MAXSECTORS] = deleteme;

    sprite[deleteme].sectnum = MAXSECTORS;
    return(0);
}


//
// deletespritestat (internal)
//
int32_t deletespritestat(int16_t deleteme)
{
    if (sprite[deleteme].statnum == MAXSTATUS)
        return(-1);

    if (headspritestat[sprite[deleteme].statnum] == deleteme)
        headspritestat[sprite[deleteme].statnum] = nextspritestat[deleteme];

    if (prevspritestat[deleteme] >= 0) nextspritestat[prevspritestat[deleteme]] = nextspritestat[deleteme];
    if (nextspritestat[deleteme] >= 0) prevspritestat[nextspritestat[deleteme]] = prevspritestat[deleteme];

    if (headspritestat[MAXSTATUS] >= 0) prevspritestat[headspritestat[MAXSTATUS]] = deleteme;
    prevspritestat[deleteme] = -1;
    nextspritestat[deleteme] = headspritestat[MAXSTATUS];
    headspritestat[MAXSTATUS] = deleteme;

    sprite[deleteme].statnum = MAXSTATUS;
    return(0);
}


//
// lintersect (internal)
//
static inline int32_t lintersect(int32_t x1, int32_t y1, int32_t z1, int32_t x2, int32_t y2, int32_t z2, int32_t x3,
                                 int32_t y3, int32_t x4, int32_t y4, int32_t *intx, int32_t *inty, int32_t *intz)
{
    //p1 to p2 is a line segment
    int32_t x21, y21, x34, y34, x31, y31, bot, topt, topu, t;

    x21 = x2-x1; x34 = x3-x4;
    y21 = y2-y1; y34 = y3-y4;
    bot = x21*y34 - y21*x34;
    if (bot >= 0)
    {
        if (bot == 0) return(0);
        x31 = x3-x1; y31 = y3-y1;
        topt = x31*y34 - y31*x34; if ((topt < 0) || (topt >= bot)) return(0);
        topu = x21*y31 - y21*x31; if ((topu < 0) || (topu >= bot)) return(0);
    }
    else
    {
        x31 = x3-x1; y31 = y3-y1;
        topt = x31*y34 - y31*x34; if ((topt > 0) || (topt <= bot)) return(0);
        topu = x21*y31 - y21*x31; if ((topu > 0) || (topu <= bot)) return(0);
    }
    t = divscale24(topt,bot);
    *intx = x1 + mulscale24(x21,t);
    *inty = y1 + mulscale24(y21,t);
    *intz = z1 + mulscale24(z2-z1,t);
    return(1);
}


//
// rintersect (internal)
//
static inline int32_t rintersect(int32_t x1, int32_t y1, int32_t z1, int32_t vx, int32_t vy, int32_t vz, int32_t x3,
                                 int32_t y3, int32_t x4, int32_t y4, int32_t *intx, int32_t *inty, int32_t *intz)
{
    //p1 towards p2 is a ray
    int32_t x34, y34, x31, y31, bot, topt, topu, t;

    x34 = x3-x4; y34 = y3-y4;
    bot = vx*y34 - vy*x34;
    if (bot >= 0)
    {
        if (bot == 0) return(0);
        x31 = x3-x1; y31 = y3-y1;
        topt = x31*y34 - y31*x34; if (topt < 0) return(0);
        topu = vx*y31 - vy*x31; if ((topu < 0) || (topu >= bot)) return(0);
    }
    else
    {
        x31 = x3-x1; y31 = y3-y1;
        topt = x31*y34 - y31*x34; if (topt > 0) return(0);
        topu = vx*y31 - vy*x31; if ((topu > 0) || (topu <= bot)) return(0);
    }
    t = divscale16(topt,bot);
    *intx = x1 + mulscale16(vx,t);
    *inty = y1 + mulscale16(vy,t);
    *intz = z1 + mulscale16(vz,t);
    return(1);
}


//
// keepaway (internal)
//
static inline void keepaway(int32_t *x, int32_t *y, int32_t w)
{
    int32_t dx, dy, ox, oy, x1, y1;
    char first;

    x1 = clipit[w].x1; dx = clipit[w].x2-x1;
    y1 = clipit[w].y1; dy = clipit[w].y2-y1;
    ox = ksgn(-dy); oy = ksgn(dx);
    first = (klabs(dx) <= klabs(dy));
    while (1)
    {
        if (dx*(*y-y1) > (*x-x1)*dy) return;
        if (first == 0) *x += ox; else *y += oy;
        first ^= 1;
    }
}


//
// raytrace (internal)
//
static inline int32_t raytrace(int32_t x3, int32_t y3, int32_t *x4, int32_t *y4)
{
    int32_t x1, y1, x2, y2, bot, topu, nintx, ninty, cnt, z, hitwall;
    int32_t x21, y21, x43, y43;

    hitwall = -1;
    for (z=clipnum-1; z>=0; z--)
    {
        x1 = clipit[z].x1; x2 = clipit[z].x2; x21 = x2-x1;
        y1 = clipit[z].y1; y2 = clipit[z].y2; y21 = y2-y1;

        topu = x21*(y3-y1) - (x3-x1)*y21; if (topu <= 0) continue;
        if (x21*(*y4-y1) > (*x4-x1)*y21) continue;
        x43 = *x4-x3; y43 = *y4-y3;
        if (x43*(y1-y3) > (x1-x3)*y43) continue;
        if (x43*(y2-y3) <= (x2-x3)*y43) continue;
        bot = x43*y21 - x21*y43; if (bot == 0) continue;

        cnt = 256;
        do
        {
            cnt--; if (cnt < 0) { *x4 = x3; *y4 = y3; return(z); }
            nintx = x3 + scale(x43,topu,bot);
            ninty = y3 + scale(y43,topu,bot);
            topu--;
        }
        while (x21*(ninty-y1) <= (nintx-x1)*y21);

        if (klabs(x3-nintx)+klabs(y3-ninty) < klabs(x3-*x4)+klabs(y3-*y4))
            { *x4 = nintx; *y4 = ninty; hitwall = z; }
    }
    return(hitwall);
}



//
// Exported Engine Functions
//

#if !defined _WIN32 && defined DEBUGGINGAIDS
#include <signal.h>
static void sighandler(int32_t sig, const siginfo_t *info, void *ctx)
{
    const char *s;
    UNREFERENCED_PARAMETER(ctx);
    switch (sig)
    {
    case SIGFPE:
        switch (info->si_code)
        {
        case FPE_INTDIV:
            s = "FPE_INTDIV (integer divide by zero)"; break;
        case FPE_INTOVF:
            s = "FPE_INTOVF (integer overflow)"; break;
        case FPE_FLTDIV:
            s = "FPE_FLTDIV (floating-point divide by zero)"; break;
        case FPE_FLTOVF:
            s = "FPE_FLTOVF (floating-point overflow)"; break;
        case FPE_FLTUND:
            s = "FPE_FLTUND (floating-point underflow)"; break;
        case FPE_FLTRES:
            s = "FPE_FLTRES (floating-point inexact result)"; break;
        case FPE_FLTINV:
            s = "FPE_FLTINV (floating-point invalid operation)"; break;
        case FPE_FLTSUB:
            s = "FPE_FLTSUB (floating-point subscript out of range)"; break;
        default:
            s = "?! (unknown)"; break;
        }
        fprintf(stderr, "Caught SIGFPE at address %p, code %s. Aborting.\n", info->si_addr, s);
        break;
    default:
        break;
    }
    abort();
}
#endif

//
// preinitengine
//
static int32_t preinitcalled = 0;

#define DYNALLOC_ARRAYS

#ifndef DYNALLOC_ARRAYS
static spriteext_t spriteext_s[MAXSPRITES+MAXUNIQHUDID];
static spritesmooth_t spritesmooth_s[MAXSPRITES+MAXUNIQHUDID];
static sectortype sector_s[MAXSECTORS];
static walltype wall_s[MAXWALLS];
static spritetype sprite_s[MAXSPRITES];
static spritetype tsprite_s[MAXSPRITESONSCREEN];
#endif

int32_t preinitengine(void)
{
    char *e;
    if (initsystem()) exit(1);

    makeasmwriteable();

    // this shite is to help get around data segment size limits on some platforms

#ifdef DYNALLOC_ARRAYS
    sector = Bcalloc(MAXSECTORS,sizeof(sectortype));
    wall = Bcalloc(MAXWALLS,sizeof(walltype));
    sprite = Bcalloc(MAXSPRITES,sizeof(spritetype));
    tsprite = Bcalloc(MAXSPRITESONSCREEN,sizeof(spritetype));
    spriteext = Bcalloc(MAXSPRITES+MAXUNIQHUDID,sizeof(spriteext_t));
    spritesmooth = Bcalloc(MAXSPRITES+MAXUNIQHUDID,sizeof(spritesmooth_t));

    if (!sector || !wall || !sprite || !tsprite || !spriteext || !spritesmooth)
        return 1;
#else
    sector = sector_s;
    wall = wall_s;
    sprite = sprite_s;
    tsprite = tsprite_s;
    spriteext = spriteext_s;
    spritesmooth = spritesmooth_s;
#endif

    if ((e = Bgetenv("BUILD_NOP6")) != NULL)
        if (!Bstrcasecmp(e, "TRUE"))
        {
            Bprintf("Disabling P6 optimizations.\n");
            dommxoverlay = 0;
        }
    if (dommxoverlay) mmxoverlay();

    validmodecnt = 0;
    getvalidmodes();

    initcrc32table();

    preinitcalled = 1;
    return 0;
}


//
// initengine
//
int32_t initengine(void)
{
    int32_t i, j;

#if !defined _WIN32 && defined DEBUGGINGAIDS
    struct sigaction sigact, oldact;
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_sigaction = (void *)sighandler;
    sigact.sa_flags = SA_SIGINFO;
    sigaction(SIGFPE, &sigact, &oldact);
#endif

    if (!preinitcalled)
    {
        i = preinitengine();
        if (i) return i;
    }

    if (loadtables()) return 1;

    xyaspect = -1;

    pskyoff[0] = 0; pskybits = 0;

    parallaxtype = 2; parallaxyoffs = 0L; parallaxyscale = 65536;
    showinvisibility = 0;

#ifdef SUPERBUILD
    for (i=1; i<1024; i++) lowrecip[i] = ((1<<24)-1)/i;
    for (i=0; i<MAXVOXELS; i++)
        for (j=0; j<MAXVOXMIPS; j++)
        {
            voxoff[i][j] = 0L;
            voxlock[i][j] = 200;
        }
    for (i=0; i<MAXTILES; i++)
        tiletovox[i] = -1;
    clearbuf(&voxscale[0],sizeof(voxscale)>>2,65536L);
#endif

    paletteloaded = 0;

    searchit = 0; searchstat = -1;

    for (i=0; i<MAXPALOOKUPS; i++) palookup[i] = NULL;

    clearbuf(&waloff[0],(int32_t)MAXTILES,0L);

    clearbuf(&show2dsector[0],(int32_t)((MAXSECTORS+3)>>5),0L);
    clearbuf(&show2dsprite[0],(int32_t)((MAXSPRITES+3)>>5),0L);
    clearbuf(&show2dwall[0],(int32_t)((MAXWALLS+3)>>5),0L);
    automapping = 0;

    pointhighlight = -1;
    linehighlight = -1;
    highlightcnt = 0;

    totalclock = 0;
    visibility = 512;
    parallaxvisibility = 512;

    captureformat = 0;

    loadpalette();
#if defined(POLYMOST) && defined(USE_OPENGL)
    if (!hicfirstinit) hicinit();
    if (!mdinited) mdinit();
#endif

    return 0;
}


//
// uninitengine
//
void uninitengine(void)
{
    int32_t i;

    //OSD_Printf("cacheresets = %d, cacheinvalidates = %d\n", cacheresets, cacheinvalidates);

#if defined(POLYMOST) && defined(USE_OPENGL)
    polymost_glreset();
    hicinit();
    freeallmodels();
#ifdef POLYMER
    polymer_uninit();
#endif
    /*    if (cachefilehandle > -1)
            Bclose(cachefilehandle);
        if (cacheindexptr != NULL)
            Bfclose(cacheindexptr); */
#endif

    uninitsystem();
    if (artfil != -1) kclose(artfil);

    if (transluc != NULL) { kkfree(transluc); transluc = NULL; }
    if (pic != NULL) { kkfree(pic); pic = NULL; }
    if (lookups != NULL)
    {
        if (lookupsalloctype == 0) kkfree((void *)lookups);
        //if (lookupsalloctype == 1) suckcache(lookups);  //Cache already gone
        lookups = NULL;
    }

    for (i=0; i<MAXPALOOKUPS; i++)
        if (palookup[i] != NULL) { kkfree(palookup[i]); palookup[i] = NULL; }

#ifdef DYNALLOC_ARRAYS
    if (sector != NULL)
        Bfree(sector);
    if (wall != NULL)
        Bfree(wall);
    if (sprite != NULL)
        Bfree(sprite);
    if (tsprite != NULL)
        Bfree(tsprite);
    if (spriteext != NULL)
        Bfree(spriteext);
    if (spritesmooth != NULL)
        Bfree(spritesmooth);
#endif
}


//
// initspritelists
//
void initspritelists(void)
{
    int32_t i;

    for (i=0; i<MAXSECTORS; i++)   //Init doubly-linked sprite sector lists
        headspritesect[i] = -1;
    headspritesect[MAXSECTORS] = 0;
    for (i=0; i<MAXSPRITES; i++)
    {
        prevspritesect[i] = i-1;
        nextspritesect[i] = i+1;
        sprite[i].sectnum = MAXSECTORS;
    }
    prevspritesect[0] = -1;
    nextspritesect[MAXSPRITES-1] = -1;


    for (i=0; i<MAXSTATUS; i++)   //Init doubly-linked sprite status lists
        headspritestat[i] = -1;
    headspritestat[MAXSTATUS] = 0;
    for (i=0; i<MAXSPRITES; i++)
    {
        prevspritestat[i] = i-1;
        nextspritestat[i] = i+1;
        sprite[i].statnum = MAXSTATUS;
    }
    prevspritestat[0] = -1;
    nextspritestat[MAXSPRITES-1] = -1;
}


//
// drawrooms
//
void drawrooms(int32_t daposx, int32_t daposy, int32_t daposz,
               int16_t daang, int32_t dahoriz, int16_t dacursectnum)
{
    int32_t i, j, z, cz, fz, closest;
    int16_t *shortptr1, *shortptr2;

    beforedrawrooms = 0;
    indrawroomsandmasks = 1;

    globalposx = daposx; globalposy = daposy; globalposz = daposz;
    globalang = (daang&2047);

    globalhoriz = mulscale16(dahoriz-100,xdimenscale)+(ydimen>>1);

    globaluclip = (0-globalhoriz)*xdimscale;
    globaldclip = (ydimen-globalhoriz)*xdimscale;

    i = mulscale16(xdimenscale,viewingrangerecip);
    globalpisibility = mulscale16(parallaxvisibility,i);
    globalvisibility = mulscale16(visibility,i);
    globalhisibility = mulscale16(globalvisibility,xyaspect);
    globalcisibility = mulscale8(globalhisibility,320);

    globalcursectnum = dacursectnum;
    totalclocklock = totalclock;

    cosglobalang = sintable[(globalang+512)&2047];
    singlobalang = sintable[globalang&2047];
    cosviewingrangeglobalang = mulscale16(cosglobalang,viewingrange);
    sinviewingrangeglobalang = mulscale16(singlobalang,viewingrange);

    if ((xyaspect != oxyaspect) || (xdimen != oxdimen) || (viewingrange != oviewingrange))
        dosetaspect();

    //clearbufbyte(&gotsector[0],(int32_t)((numsectors+7)>>3),0L);
    Bmemset(&gotsector[0],0,(int32_t)((numsectors+7)>>3));

    shortptr1 = (int16_t *)&startumost[windowx1];
    shortptr2 = (int16_t *)&startdmost[windowx1];
    i = xdimen-1;
    do
    {
        umost[i] = shortptr1[i]-windowy1;
        dmost[i] = shortptr2[i]-windowy1;
        i--;
    }
    while (i != 0);
    umost[0] = shortptr1[0]-windowy1;
    dmost[0] = shortptr2[0]-windowy1;

#ifdef POLYMOST
# ifdef POLYMER
    if (rendmode == 4)
    {
        polymer_glinit();
        polymer_drawrooms(daposx, daposy, daposz, daang, dahoriz, dacursectnum);
        bglDisable(GL_CULL_FACE);
        gloy1 = 0;
        return;
    }
# endif

    //============================================================================= //POLYMOST BEGINS
    polymost_drawrooms();
    if (rendmode)
        return;
#endif
    //============================================================================= //POLYMOST ENDS

    begindrawing(); //{{{

    //frameoffset = frameplace + viewoffset;
    frameoffset = frameplace + windowy1*bytesperline + windowx1;

    //if (smostwallcnt < 0)
    //  if (getkensmessagecrc(FP_OFF(kensmessage)) != 0x56c764d4)
    //      { /* setvmode(0x3);*/ printOSD("Nice try.\n"); exit(0); }

    numhits = xdimen; numscans = 0; numbunches = 0;
    maskwallcnt = 0; smostwallcnt = 0; smostcnt = 0; spritesortcnt = 0;

    if (globalcursectnum >= MAXSECTORS)
        globalcursectnum -= MAXSECTORS;
    else
    {
        i = globalcursectnum;
        updatesector(globalposx,globalposy,&globalcursectnum);
        if (globalcursectnum < 0) globalcursectnum = i;
    }

    globparaceilclip = 1;
    globparaflorclip = 1;
    getzsofslope(globalcursectnum,globalposx,globalposy,&cz,&fz);
    if (globalposz < cz) globparaceilclip = 0;
    if (globalposz > fz) globparaflorclip = 0;

    scansector(globalcursectnum);

    if (inpreparemirror)
    {
        inpreparemirror = 0;
        mirrorsx1 = xdimen-1; mirrorsx2 = 0;
        for (i=numscans-1; i>=0; i--)
        {
            if (wall[thewall[i]].nextsector < 0) continue;
            if (xb1[i] < mirrorsx1) mirrorsx1 = xb1[i];
            if (xb2[i] > mirrorsx2) mirrorsx2 = xb2[i];
        }

        for (i=0; i<mirrorsx1; i++)
            if (umost[i] <= dmost[i])
                { umost[i] = 1; dmost[i] = 0; numhits--; }
        for (i=mirrorsx2+1; i<xdimen; i++)
            if (umost[i] <= dmost[i])
                { umost[i] = 1; dmost[i] = 0; numhits--; }

        drawalls(0L);
        numbunches--;
        bunchfirst[0] = bunchfirst[numbunches];
        bunchlast[0] = bunchlast[numbunches];

        mirrorsy1 = min(umost[mirrorsx1],umost[mirrorsx2]);
        mirrorsy2 = max(dmost[mirrorsx1],dmost[mirrorsx2]);
    }

    while ((numbunches > 0) && (numhits > 0))
    {
        clearbuf(&tempbuf[0],(int32_t)((numbunches+3)>>2),0L);
        tempbuf[0] = 1;

        closest = 0;              //Almost works, but not quite :(
        for (i=1; i<numbunches; i++)
        {
            if ((j = bunchfront(i,closest)) < 0) continue;
            tempbuf[i] = 1;
            if (j == 0) tempbuf[closest] = 1, closest = i;
        }
        for (i=0; i<numbunches; i++) //Double-check
        {
            if (tempbuf[i]) continue;
            if ((j = bunchfront(i,closest)) < 0) continue;
            tempbuf[i] = 1;
            if (j == 0) tempbuf[closest] = 1, closest = i, i = 0;
        }

        drawalls(closest);

        if (automapping)
        {
            for (z=bunchfirst[closest]; z>=0; z=p2[z])
                show2dwall[thewall[z]>>3] |= pow2char[thewall[z]&7];
        }

        numbunches--;
        bunchfirst[closest] = bunchfirst[numbunches];
        bunchlast[closest] = bunchlast[numbunches];
    }

    enddrawing();   //}}}
}

// UTILITY TYPES AND FUNCTIONS FOR DRAWMASKS OCCLUSION TREE
// typedef struct          s_maskleaf
// {
//     int32_t                index;
//     _point2d            p1, p2;
//     _equation           maskeq, p1eq, p2eq;
//     struct s_maskleaf*  branch[MAXWALLSB];
//     int32_t                 drawing;
// }                       _maskleaf;
// 
// _maskleaf               maskleaves[MAXWALLSB];

// returns equation of a line given two points
static inline _equation       equation(float x1, float y1, float x2, float y2)
{
    _equation   ret;

    if ((x2 - x1) != 0)
    {
        ret.a = (float)(y2 - y1)/(float)(x2 - x1);
        ret.b = -1;
        ret.c = (y1 - (ret.a * x1));
    }
    else // vertical
    {
        ret.a = 1;
        ret.b = 0;
        ret.c = -x1;
    }

    return (ret);
}

int32_t                 wallvisible(int16_t wallnum)
{
    // 1 if wall is in front of player 0 otherwise
    int32_t            a1, a2;
    walltype        *w1, *w2;

    w1 = &wall[wallnum];
    w2 = &wall[w1->point2];

    a1 = getangle(w1->x - globalposx, w1->y - globalposy);
    a2 = getangle(w2->x - globalposx, w2->y - globalposy);

    //if ((wallnum == 23) || (wallnum == 9))
    //    OSD_Printf("Wall %d : %d - sector %d - x %d - y %d.\n", wallnum, (a2 + (2048 - a1)) & 2047, globalcursectnum, globalposx, globalposy);

    if (((a2 + (2048 - a1)) & 2047) <= 1024)
        return (1);
    else
        return (0);
}
/*
// returns the intersection point between two lines
_point2d        intersection(_equation eq1, _equation eq2)
{
    _point2d    ret;
    float       det;

    det = (float)(1) / (eq1.a*eq2.b - eq2.a*eq1.b);
    ret.x = ((eq1.b*eq2.c - eq2.b*eq1.c) * det);
    ret.y = ((eq2.a*eq1.c - eq1.a*eq2.c) * det);

    return (ret);
}

// check if a point that's on the line is within the segment boundaries
int32_t             pointonmask(_point2d point, _maskleaf* wall)
{
    if ((min(wall->p1.x, wall->p2.x) <= point.x) && (point.x <= max(wall->p1.x, wall->p2.x)) && (min(wall->p1.y, wall->p2.y) <= point.y) && (point.y <= max(wall->p1.y, wall->p2.y)))
        return (1);
    return (0);
}

// returns 1 if wall2 is hidden by wall1
int32_t             wallobstructswall(_maskleaf* wall1, _maskleaf* wall2)
{
    _point2d    cross;

    cross = intersection(wall2->p1eq, wall1->maskeq);
    if (pointonmask(cross, wall1))
        return (1);

    cross = intersection(wall2->p2eq, wall1->maskeq);
    if (pointonmask(cross, wall1))
        return (1);

    cross = intersection(wall1->p1eq, wall2->maskeq);
    if (pointonmask(cross, wall2))
        return (1);

    cross = intersection(wall1->p2eq, wall2->maskeq);
    if (pointonmask(cross, wall2))
        return (1);

    return (0);
}

// recursive mask drawing function
static inline void    drawmaskleaf(_maskleaf* wall)
{
    int32_t i;

    wall->drawing = 1;
    i = 0;
    while (wall->branch[i] != NULL)
    {
        if (wall->branch[i]->drawing == 0)
        {
            //OSD_Printf("Drawing parent of %i : mask %i\n", wall->index, wall->branch[i]->index);
            drawmaskleaf(wall->branch[i]);
        }
        i++;
    }

    //OSD_Printf("Drawing mask %i\n", wall->index);
    drawmaskwall(wall->index);
}
*/

static inline int32_t         sameside(_equation* eq, _point2d* p1, _point2d* p2)
{
    float   sign1, sign2;

    sign1 = eq->a * p1->x + eq->b * p1->y + eq->c;
    sign2 = eq->a * p2->x + eq->b * p2->y + eq->c;

    sign1 = sign1 * sign2;
    if (sign1 > 0)
    {
        //OSD_Printf("SAME SIDE !\n");
        return (1);
    }
    //OSD_Printf("OPPOSITE SIDE !\n");
    return (0);
}

/*
#ifdef USE_OPENGL
void    drawpeel(int32_t peel)
{
    bglBindTexture(GL_TEXTURE_RECTANGLE, peels[peel]);
    bglBegin(GL_QUADS);
    bglTexCoord2f(0.0f, 0.0f);
    bglVertex2f(-1.0f, -1.0f);
    bglTexCoord2i(xdim, 0);
    bglVertex2f(1.0f, -1.0f);
    bglTexCoord2i(xdim, ydim);
    bglVertex2f(1.0f, 1.0f);
    bglTexCoord2i(0, ydim);
    bglVertex2f(-1.0f, 1.0f);
    bglEnd();
}
#endif
*/

//
// drawmasks
//
void drawmasks(void)
{
    int32_t i, j, k, l, gap, xs, ys, xp, yp, yoff, yspan;
    // PLAG: sorting stuff
    _equation maskeq, p1eq, p2eq;
    _point2d dot, dot2, middle, pos, spr;

#ifdef POLYMER
    if ((rendmode == 4) && 0)
    {
        polymer_drawmasks();
        return;
    }
#endif

    for (i=spritesortcnt-1; i>=0; i--) tspriteptr[i] = &tsprite[i];
    for (i=spritesortcnt-1; i>=0; i--)
    {
        xs = tspriteptr[i]->x-globalposx; ys = tspriteptr[i]->y-globalposy;
        yp = dmulscale6(xs,cosviewingrangeglobalang,ys,sinviewingrangeglobalang);
        if (yp > (4<<8))
        {
            xp = dmulscale6(ys,cosglobalang,-xs,singlobalang);
            if (mulscale24(labs(xp+yp),xdimen) >= yp) goto killsprite;
            spritesx[i] = scale(xp+yp,xdimen<<7,yp);
        }
        else if ((tspriteptr[i]->cstat&48) == 0)
        {
killsprite:
            spritesortcnt--;  //Delete face sprite if on wrong side!
            if (i != spritesortcnt)
            {
                tspriteptr[i] = tspriteptr[spritesortcnt];
                spritesx[i] = spritesx[spritesortcnt];
                spritesy[i] = spritesy[spritesortcnt];
            }
            continue;
        }
        spritesy[i] = yp;
    }

#ifdef USE_OPENGL
//    if ((!r_depthpeeling) || (rendmode < 3))
#endif
    {

        gap = 1; while (gap < spritesortcnt) gap = (gap<<1)+1;
        for (gap>>=1; gap>0; gap>>=1)   //Sort sprite list
            for (i=0; i<spritesortcnt-gap; i++)
                for (l=i; l>=0; l-=gap)
                {
                    if (spritesy[l] <= spritesy[l+gap]) break;
                    swaplong(&tspriteptr[l],&tspriteptr[l+gap]);
                    swaplong(&spritesx[l],&spritesx[l+gap]);
                    swaplong(&spritesy[l],&spritesy[l+gap]);
                }

        if (spritesortcnt > 0)
            spritesy[spritesortcnt] = (spritesy[spritesortcnt-1]^1);

        ys = spritesy[0]; i = 0;
        for (j=1; j<=spritesortcnt; j++)
        {
            if (spritesy[j] == ys) continue;
            ys = spritesy[j];
            if (j > i+1)
            {
                for (k=i; k<j; k++)
                {
                    spritesz[k] = tspriteptr[k]->z;
                    if ((tspriteptr[k]->cstat&48) != 32)
                    {
                        yoff = (int32_t)((int8_t)((picanm[tspriteptr[k]->picnum]>>16)&255))+((int32_t)tspriteptr[k]->yoffset);
                        spritesz[k] -= ((yoff*tspriteptr[k]->yrepeat)<<2);
                        yspan = (tilesizy[tspriteptr[k]->picnum]*tspriteptr[k]->yrepeat<<2);
                        if (!(tspriteptr[k]->cstat&128)) spritesz[k] -= (yspan>>1);
                        if (klabs(spritesz[k]-globalposz) < (yspan>>1)) spritesz[k] = globalposz;
                    }
                }
                for (k=i+1; k<j; k++)
                    for (l=i; l<k; l++)
                        if (klabs(spritesz[k]-globalposz) < klabs(spritesz[l]-globalposz))
                        {
                            swaplong(&tspriteptr[k],&tspriteptr[l]);
                            swaplong(&spritesx[k],&spritesx[l]);
                            swaplong(&spritesy[k],&spritesy[l]);
                            swaplong(&spritesz[k],&spritesz[l]);
                        }
                for (k=i+1; k<j; k++)
                    for (l=i; l<k; l++)
                        if (tspriteptr[k]->statnum < tspriteptr[l]->statnum)
                        {
                            swaplong(&tspriteptr[k],&tspriteptr[l]);
                            swaplong(&spritesx[k],&spritesx[l]);
                            swaplong(&spritesy[k],&spritesy[l]);
                        }
            }
            i = j;
        }
    }

    begindrawing(); //{{{

    /*for(i=spritesortcnt-1;i>=0;i--)
    {
        xs = tspriteptr[i].x-globalposx;
        ys = tspriteptr[i].y-globalposy;
        zs = tspriteptr[i].z-globalposz;

        xp = ys*cosglobalang-xs*singlobalang;
        yp = (zs<<1);
        zp = xs*cosglobalang+ys*singlobalang;

        xs = scale(xp,halfxdimen<<12,zp)+((halfxdimen+windowx1)<<12);
        ys = scale(yp,xdimenscale<<12,zp)+((globalhoriz+windowy1)<<12);

        drawline256(xs-65536,ys-65536,xs+65536,ys+65536,31);
        drawline256(xs+65536,ys-65536,xs-65536,ys+65536,31);
    }*/

#if defined(USE_OPENGL) && defined(POLYMOST)
//    if ((!r_depthpeeling) || (rendmode < 3))
#endif
    {
#if defined(USE_OPENGL) && defined(POLYMOST)
        curpolygonoffset = 0;
        cullcheckcnt = 0;
#endif
        pos.x = globalposx;
        pos.y = globalposy;

        while (maskwallcnt)
        {
            maskwallcnt--;
#if defined(USE_OPENGL) && defined(POLYMER)
            if (rendmode == 4)
            {
                dot.x = wall[maskwall[maskwallcnt]].x;
                dot.y = wall[maskwall[maskwallcnt]].y;
                dot2.x = wall[wall[maskwall[maskwallcnt]].point2].x;
                dot2.y = wall[wall[maskwall[maskwallcnt]].point2].y;
            }
            else
#endif
            {
                dot.x = wall[thewall[maskwall[maskwallcnt]]].x;
                dot.y = wall[thewall[maskwall[maskwallcnt]]].y;
                dot2.x = wall[wall[thewall[maskwall[maskwallcnt]]].point2].x;
                dot2.y = wall[wall[thewall[maskwall[maskwallcnt]]].point2].y;
            }

            maskeq = equation(dot.x, dot.y, dot2.x, dot2.y);
            p1eq = equation(pos.x, pos.y, dot.x, dot.y);
            p2eq = equation(pos.x, pos.y, dot2.x, dot2.y);

            middle.x = (dot.x + dot2.x) / 2;
            middle.y = (dot.y + dot2.y) / 2;

            i = spritesortcnt;
            while (i)
            {
                i--;
                if (tspriteptr[i] != NULL)
                {
                    spr.x = tspriteptr[i]->x;
                    spr.y = tspriteptr[i]->y;

                    if ((sameside(&maskeq, &spr, &pos) == 0) && sameside(&p1eq, &middle, &spr) && sameside(&p2eq, &middle, &spr))
                    {
                        drawsprite(i);
                        tspriteptr[i] = NULL;
                    }
                }
            }
            drawmaskwall(maskwallcnt);
        }

        while (spritesortcnt)
        {
            spritesortcnt--;
            if (tspriteptr[spritesortcnt] != NULL)
                drawsprite(spritesortcnt);
        }
#if defined(USE_OPENGL) && defined(POLYMOST)
        if (totalclock < lastcullcheck - CULL_DELAY)
            lastcullcheck = totalclock;
        if (totalclock >= lastcullcheck + CULL_DELAY)
            lastcullcheck = (totalclock + CULL_DELAY);
#endif
    } /* depthpeeling */
#if defined(USE_OPENGL) && defined(POLYMOST)
    /*
        else
        {
            curpolygonoffset = 0;
            cullcheckcnt = 0;
            j = spritesortcnt;
            k = maskwallcnt;

            while (j > 0) drawsprite(--j);
            while (k > 0) drawmaskwall(--k);
            if (totalclock < lastcullcheck - CULL_DELAY)
                lastcullcheck = totalclock;
            if (totalclock >= lastcullcheck + CULL_DELAY)
                lastcullcheck = (totalclock + CULL_DELAY);
        }
    */
#endif

#if defined(USE_OPENGL) && defined(POLYMOST)
    /*
        if ((r_depthpeeling) && (rendmode >= 3))
        {
            bglPopAttrib();
            bglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

            bglNewList(1, GL_COMPILE);

            peelcompiling = 1;
            curpolygonoffset = 0;

            while (spritesortcnt > 0) drawsprite(--spritesortcnt);
            while (maskwallcnt > 0) drawmaskwall(--maskwallcnt);

            peelcompiling = 0;

            bglEndList();

            bglDisable(GL_BLEND);
            bglEnable(GL_ALPHA_TEST);
            bglAlphaFunc(GL_GREATER, 0.0f);
            bglEnable(GL_FRAGMENT_PROGRAM_ARB);

            i = 0;
            while (i < r_peelscount)
            {

                if (i > 0)
                {
                    bglBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, peelprogram[1]);
                    bglActiveTextureARB(GL_TEXTURE1_ARB);
                    bglBindTexture(GL_TEXTURE_RECTANGLE, ztexture[(i - 1) % 2]);
                    bglActiveTextureARB(GL_TEXTURE2_ARB);
                    bglBindTexture(GL_TEXTURE_RECTANGLE, ztexture[2]);
                    bglActiveTextureARB(GL_TEXTURE0_ARB);
                }
                else
                {
                    bglBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, peelprogram[0]);
                    bglActiveTextureARB(GL_TEXTURE1_ARB);
                    bglBindTexture(GL_TEXTURE_RECTANGLE, ztexture[2]);
                    bglActiveTextureARB(GL_TEXTURE0_ARB);
                }

                if (i == (r_peelscount - 1))
                    bglEnable(GL_BLEND);

                bglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, peelfbos[i]);
                bglPushAttrib(GL_VIEWPORT_BIT);
                bglViewport(0, 0, xdim, ydim);

                bglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                bglCallList(1);

                bglPopAttrib();
                bglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

                i++;
            }

            bglDisable(GL_FRAGMENT_PROGRAM_ARB);
            bglEnable(GL_BLEND);
            bglDisable(GL_DEPTH_TEST);

            bglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

            // identity for screen aligned quads
            bglMatrixMode(GL_PROJECTION);
            bglPushMatrix();
            bglLoadIdentity();
            bglMatrixMode(GL_MODELVIEW);
            bglPushMatrix();
            bglLoadIdentity();

            bglEnable(GL_TEXTURE_RECTANGLE);

            // backbuffer
            drawpeel(r_peelscount);

            if (r_curpeel == -1)
            {
                i = r_peelscount - 1;
                while (i >= 0)
                    drawpeel(i--);
            }
            else
                drawpeel(r_curpeel);

            bglDisable(GL_TEXTURE_RECTANGLE);
            bglEnable(GL_TEXTURE_2D);

            // restore the polymost projection
            bglMatrixMode(GL_PROJECTION);
            bglPopMatrix();
            bglMatrixMode(GL_MODELVIEW);
            bglPopMatrix();

            bglEnable(GL_DEPTH_TEST);

            bglDeleteLists(1, 1);
        }
    */
#endif

    indrawroomsandmasks = 0;
    enddrawing();   //}}}
}

//
// drawmapview
//
void drawmapview(int32_t dax, int32_t day, int32_t zoome, int16_t ang)
{
    walltype *wal;
    sectortype *sec;
    spritetype *spr;
    int32_t tilenum, xoff, yoff, i, j, k, l, cosang, sinang, xspan, yspan;
    int32_t xrepeat, yrepeat, x, y, x1, y1, x2, y2, x3, y3, x4, y4, bakx1, baky1;
    int32_t s, w, ox, oy, startwall, cx1, cy1, cx2, cy2;
    int32_t bakgxvect, bakgyvect, sortnum, gap, npoints;
    int32_t xvect, yvect, xvect2, yvect2, daslope;

    beforedrawrooms = 0;

    clearbuf(&gotsector[0],(int32_t)((numsectors+31)>>5),0L);

    cx1 = (windowx1<<12); cy1 = (windowy1<<12);
    cx2 = ((windowx2+1)<<12)-1; cy2 = ((windowy2+1)<<12)-1;
    if (zoome == 2048) zoome = 2047; // FIXME
    zoome <<= 8;
    bakgxvect = divscale28(sintable[(1536-ang)&2047],zoome);
    bakgyvect = divscale28(sintable[(2048-ang)&2047],zoome);
    xvect = mulscale8(sintable[(2048-ang)&2047],zoome);
    yvect = mulscale8(sintable[(1536-ang)&2047],zoome);
    xvect2 = mulscale16(xvect,yxaspect);
    yvect2 = mulscale16(yvect,yxaspect);

    sortnum = 0;

    begindrawing(); //{{{

    for (s=0,sec=&sector[s]; s<numsectors; s++,sec++)
        if (show2dsector[s>>3]&pow2char[s&7])
        {
            npoints = 0; i = 0;
            startwall = sec->wallptr;
#if 0
            for (w=sec->wallnum,wal=&wall[startwall]; w>0; w--,wal++)
            {
                ox = wal->x - dax; oy = wal->y - day;
                x = dmulscale16(ox,xvect,-oy,yvect) + (xdim<<11);
                y = dmulscale16(oy,xvect2,ox,yvect2) + (ydim<<11);
                i |= getclipmask(x-cx1,cx2-x,y-cy1,cy2-y);
                rx1[npoints] = x;
                ry1[npoints] = y;
                xb1[npoints] = wal->point2 - startwall;
                npoints++;
            }
#else
            j = startwall; l = 0;
            for (w=sec->wallnum,wal=&wall[startwall]; w>0; w--,wal++,j++)
            {
                k = lastwall(j);
                if ((k > j) && (npoints > 0)) { xb1[npoints-1] = l; l = npoints; } //overwrite point2
                //wall[k].x wal->x wall[wal->point2].x
                //wall[k].y wal->y wall[wal->point2].y
                if (!dmulscale1(wal->x-wall[k].x,wall[wal->point2].y-wal->y,-(wal->y-wall[k].y),wall[wal->point2].x-wal->x)) continue;
                ox = wal->x - dax; oy = wal->y - day;
                x = dmulscale16(ox,xvect,-oy,yvect) + (xdim<<11);
                y = dmulscale16(oy,xvect2,ox,yvect2) + (ydim<<11);
                i |= getclipmask(x-cx1,cx2-x,y-cy1,cy2-y);
                rx1[npoints] = x;
                ry1[npoints] = y;
                xb1[npoints] = npoints+1;
                npoints++;
            }
            if (npoints > 0) xb1[npoints-1] = l; //overwrite point2
#endif
            if ((i&0xf0) != 0xf0) continue;
            bakx1 = rx1[0]; baky1 = mulscale16(ry1[0]-(ydim<<11),xyaspect)+(ydim<<11);
            if (i&0x0f)
            {
                npoints = clippoly(npoints,i);
                if (npoints < 3) continue;
            }

            //Collect floor sprites to draw
            for (i=headspritesect[s]; i>=0; i=nextspritesect[i])
                if ((sprite[i].cstat&48) == 32)
                {
                    if ((sprite[i].cstat&(64+8)) == (64+8)) continue;
                    tsprite[sortnum++].owner = i;
                }

            gotsector[s>>3] |= pow2char[s&7];

            globalorientation = (int32_t)sec->floorstat;
            if ((globalorientation&1) != 0) continue;

            globalpal = sec->floorpal;

            if (palookup[sec->floorpal] != globalpalwritten)
            {
                globalpalwritten = palookup[sec->floorpal];
                if (!globalpalwritten) globalpalwritten = palookup[0];	// JBF: fixes null-pointer crash
                setpalookupaddress(globalpalwritten);
            }
            globalpicnum = sec->floorpicnum;
            if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;
            setgotpic(globalpicnum);
            if ((tilesizx[globalpicnum] <= 0) || (tilesizy[globalpicnum] <= 0)) continue;
            if ((picanm[globalpicnum]&192) != 0) globalpicnum += animateoffs((int16_t)globalpicnum,s);
            if (waloff[globalpicnum] == 0) loadtile(globalpicnum);
            globalbufplc = waloff[globalpicnum];
            globalshade = max(min(sec->floorshade,numpalookups-1),0);
            globvis = globalhisibility;
            if (sec->visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t)(sec->visibility+16)));
            globalpolytype = 0;
            if ((globalorientation&64) == 0)
            {
                globalposx = dax; globalx1 = bakgxvect; globaly1 = bakgyvect;
                globalposy = day; globalx2 = bakgxvect; globaly2 = bakgyvect;
            }
            else
            {
                ox = wall[wall[startwall].point2].x - wall[startwall].x;
                oy = wall[wall[startwall].point2].y - wall[startwall].y;
                i = nsqrtasm(ox*ox+oy*oy); if (i == 0) continue;
                i = 1048576/i;
                globalx1 = mulscale10(dmulscale10(ox,bakgxvect,oy,bakgyvect),i);
                globaly1 = mulscale10(dmulscale10(ox,bakgyvect,-oy,bakgxvect),i);
                ox = (bakx1>>4)-(xdim<<7); oy = (baky1>>4)-(ydim<<7);
                globalposx = dmulscale28(-oy,globalx1,-ox,globaly1);
                globalposy = dmulscale28(-ox,globalx1,oy,globaly1);
                globalx2 = -globalx1;
                globaly2 = -globaly1;

                daslope = sector[s].floorheinum;
                i = nsqrtasm(daslope*daslope+16777216);
                globalposy = mulscale12(globalposy,i);
                globalx2 = mulscale12(globalx2,i);
                globaly2 = mulscale12(globaly2,i);
            }
            globalxshift = (8-(picsiz[globalpicnum]&15));
            globalyshift = (8-(picsiz[globalpicnum]>>4));
            if (globalorientation&8) {globalxshift++; globalyshift++; }

            sethlinesizes(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4,globalbufplc);

            if ((globalorientation&0x4) > 0)
            {
                i = globalposx; globalposx = -globalposy; globalposy = -i;
                i = globalx2; globalx2 = globaly1; globaly1 = i;
                i = globalx1; globalx1 = -globaly2; globaly2 = -i;
            }
            if ((globalorientation&0x10) > 0) globalx1 = -globalx1, globaly1 = -globaly1, globalposx = -globalposx;
            if ((globalorientation&0x20) > 0) globalx2 = -globalx2, globaly2 = -globaly2, globalposy = -globalposy;
            asm1 = (globaly1<<globalxshift);
            asm2 = (globalx2<<globalyshift);
            globalx1 <<= globalxshift;
            globaly2 <<= globalyshift;
            globalposx = (globalposx<<(20+globalxshift))+(((int32_t)sec->floorxpanning)<<24);
            globalposy = (globalposy<<(20+globalyshift))-(((int32_t)sec->floorypanning)<<24);

            fillpolygon(npoints);
        }

    //Sort sprite list
    gap = 1; while (gap < sortnum) gap = (gap<<1)+1;
    for (gap>>=1; gap>0; gap>>=1)
        for (i=0; i<sortnum-gap; i++)
            for (j=i; j>=0; j-=gap)
            {
                if (sprite[tsprite[j].owner].z <= sprite[tsprite[j+gap].owner].z) break;
                swapshort(&tsprite[j].owner,&tsprite[j+gap].owner);
            }

    for (s=sortnum-1; s>=0; s--)
    {
        spr = &sprite[tsprite[s].owner];
        if ((spr->cstat&48) == 32)
        {
            npoints = 0;

            tilenum = spr->picnum;
            xoff = (int32_t)((int8_t)((picanm[tilenum]>>8)&255))+((int32_t)spr->xoffset);
            yoff = (int32_t)((int8_t)((picanm[tilenum]>>16)&255))+((int32_t)spr->yoffset);
            if ((spr->cstat&4) > 0) xoff = -xoff;
            if ((spr->cstat&8) > 0) yoff = -yoff;

            k = spr->ang;
            cosang = sintable[(k+512)&2047]; sinang = sintable[k];
            xspan = tilesizx[tilenum]; xrepeat = spr->xrepeat;
            yspan = tilesizy[tilenum]; yrepeat = spr->yrepeat;

            ox = ((xspan>>1)+xoff)*xrepeat; oy = ((yspan>>1)+yoff)*yrepeat;
            x1 = spr->x + mulscale(sinang,ox,16) + mulscale(cosang,oy,16);
            y1 = spr->y + mulscale(sinang,oy,16) - mulscale(cosang,ox,16);
            l = xspan*xrepeat;
            x2 = x1 - mulscale(sinang,l,16);
            y2 = y1 + mulscale(cosang,l,16);
            l = yspan*yrepeat;
            k = -mulscale(cosang,l,16); x3 = x2+k; x4 = x1+k;
            k = -mulscale(sinang,l,16); y3 = y2+k; y4 = y1+k;

            xb1[0] = 1; xb1[1] = 2; xb1[2] = 3; xb1[3] = 0;
            npoints = 4;

            i = 0;

            ox = x1 - dax; oy = y1 - day;
            x = dmulscale16(ox,xvect,-oy,yvect) + (xdim<<11);
            y = dmulscale16(oy,xvect2,ox,yvect2) + (ydim<<11);
            i |= getclipmask(x-cx1,cx2-x,y-cy1,cy2-y);
            rx1[0] = x; ry1[0] = y;

            ox = x2 - dax; oy = y2 - day;
            x = dmulscale16(ox,xvect,-oy,yvect) + (xdim<<11);
            y = dmulscale16(oy,xvect2,ox,yvect2) + (ydim<<11);
            i |= getclipmask(x-cx1,cx2-x,y-cy1,cy2-y);
            rx1[1] = x; ry1[1] = y;

            ox = x3 - dax; oy = y3 - day;
            x = dmulscale16(ox,xvect,-oy,yvect) + (xdim<<11);
            y = dmulscale16(oy,xvect2,ox,yvect2) + (ydim<<11);
            i |= getclipmask(x-cx1,cx2-x,y-cy1,cy2-y);
            rx1[2] = x; ry1[2] = y;

            x = rx1[0]+rx1[2]-rx1[1];
            y = ry1[0]+ry1[2]-ry1[1];
            i |= getclipmask(x-cx1,cx2-x,y-cy1,cy2-y);
            rx1[3] = x; ry1[3] = y;

            if ((i&0xf0) != 0xf0) continue;
            bakx1 = rx1[0]; baky1 = mulscale16(ry1[0]-(ydim<<11),xyaspect)+(ydim<<11);
            if (i&0x0f)
            {
                npoints = clippoly(npoints,i);
                if (npoints < 3) continue;
            }

            globalpicnum = spr->picnum;
            if ((unsigned)globalpicnum >= (unsigned)MAXTILES) globalpicnum = 0;
            setgotpic(globalpicnum);
            if ((tilesizx[globalpicnum] <= 0) || (tilesizy[globalpicnum] <= 0)) continue;
            if ((picanm[globalpicnum]&192) != 0) globalpicnum += animateoffs((int16_t)globalpicnum,s);
            if (waloff[globalpicnum] == 0) loadtile(globalpicnum);
            globalbufplc = waloff[globalpicnum];
            if ((sector[spr->sectnum].ceilingstat&1) > 0)
                globalshade = ((int32_t)sector[spr->sectnum].ceilingshade);
            else
                globalshade = ((int32_t)sector[spr->sectnum].floorshade);
            globalshade = max(min(globalshade+spr->shade+6,numpalookups-1),0);
            asm3 = FP_OFF(palookup[spr->pal]+(globalshade<<8));
            globvis = globalhisibility;
            if (sec->visibility != 0) globvis = mulscale4(globvis,(int32_t)((uint8_t)(sec->visibility+16)));
            globalpolytype = ((spr->cstat&2)>>1)+1;

            //relative alignment stuff
            ox = x2-x1; oy = y2-y1;
            i = ox*ox+oy*oy; if (i == 0) continue; i = (65536*16384)/i;
            globalx1 = mulscale10(dmulscale10(ox,bakgxvect,oy,bakgyvect),i);
            globaly1 = mulscale10(dmulscale10(ox,bakgyvect,-oy,bakgxvect),i);
            ox = y1-y4; oy = x4-x1;
            i = ox*ox+oy*oy; if (i == 0) continue; i = (65536*16384)/i;
            globalx2 = mulscale10(dmulscale10(ox,bakgxvect,oy,bakgyvect),i);
            globaly2 = mulscale10(dmulscale10(ox,bakgyvect,-oy,bakgxvect),i);

            ox = picsiz[globalpicnum]; oy = ((ox>>4)&15); ox &= 15;
            if (pow2long[ox] != xspan)
            {
                ox++;
                globalx1 = mulscale(globalx1,xspan,ox);
                globaly1 = mulscale(globaly1,xspan,ox);
            }

            bakx1 = (bakx1>>4)-(xdim<<7); baky1 = (baky1>>4)-(ydim<<7);
            globalposx = dmulscale28(-baky1,globalx1,-bakx1,globaly1);
            globalposy = dmulscale28(bakx1,globalx2,-baky1,globaly2);

            if ((spr->cstat&2) == 0)
                msethlineshift(ox,oy);
            else
            {
                if (spr->cstat&512) settransreverse(); else settransnormal();
                tsethlineshift(ox,oy);
            }

            if ((spr->cstat&0x4) > 0) globalx1 = -globalx1, globaly1 = -globaly1, globalposx = -globalposx;
            asm1 = (globaly1<<2); globalx1 <<= 2; globalposx <<= (20+2);
            asm2 = (globalx2<<2); globaly2 <<= 2; globalposy <<= (20+2);

            globalorientation = ((spr->cstat&2)<<7) | ((spr->cstat&512)>>2);    // so polymost can get the translucency. ignored in software mode.
            fillpolygon(npoints);
        }
    }

    enddrawing();   //}}}
}


//
// loadboard
//
int32_t loadboard(char *filename, char fromwhere, int32_t *daposx, int32_t *daposy, int32_t *daposz,
                  int16_t *daang, int16_t *dacursectnum)
{
    int16_t fil, i, numsprites;

    i = strlen(filename)-1;
    if (filename[i] == 255) { filename[i] = 0; fromwhere = 1; } // JBF 20040119: "compatibility"
    if ((fil = kopen4load(filename,fromwhere)) == -1)
        { mapversion = 7L; return(-1); }

    kread(fil,&mapversion,4); mapversion = B_LITTLE32(mapversion);
    if (mapversion != 7L && mapversion != 8L) { kclose(fil); return(-2); }

    /*
    // Enable this for doing map checksum tests
    clearbufbyte(&wall,   sizeof(wall),   0);
    clearbufbyte(&sector, sizeof(sector), 0);
    clearbufbyte(&sprite, sizeof(sprite), 0);
    */

#ifdef POLYMER
    staticlightcount = 0;
#endif // POLYMER


    initspritelists();

#define MYMAXSECTORS (mapversion==7l?MAXSECTORSV7:MAXSECTORSV8)
#define MYMAXWALLS   (mapversion==7l?MAXWALLSV7:MAXWALLSV8)
#define MYMAXSPRITES (mapversion==7l?MAXSPRITESV7:MAXSPRITESV8)

    clearbuf(&show2dsector[0],(int32_t)((MAXSECTORS+3)>>5),0L);
    clearbuf(&show2dsprite[0],(int32_t)((MAXSPRITES+3)>>5),0L);
    clearbuf(&show2dwall[0],(int32_t)((MAXWALLS+3)>>5),0L);

    kread(fil,daposx,4); *daposx = B_LITTLE32(*daposx);
    kread(fil,daposy,4); *daposy = B_LITTLE32(*daposy);
    kread(fil,daposz,4); *daposz = B_LITTLE32(*daposz);
    kread(fil,daang,2);  *daang  = B_LITTLE16(*daang);
    kread(fil,dacursectnum,2); *dacursectnum = B_LITTLE16(*dacursectnum);

    kread(fil,&numsectors,2); numsectors = B_LITTLE16(numsectors);
    if (numsectors > MYMAXSECTORS) { kclose(fil); return(-1); }
    kread(fil,&sector[0],sizeof(sectortype)*numsectors);
    for (i=numsectors-1; i>=0; i--)
    {
        sector[i].wallptr       = B_LITTLE16(sector[i].wallptr);
        sector[i].wallnum       = B_LITTLE16(sector[i].wallnum);
        sector[i].ceilingz      = B_LITTLE32(sector[i].ceilingz);
        sector[i].floorz        = B_LITTLE32(sector[i].floorz);
        sector[i].ceilingstat   = B_LITTLE16(sector[i].ceilingstat);
        sector[i].floorstat     = B_LITTLE16(sector[i].floorstat);
        sector[i].ceilingpicnum = B_LITTLE16(sector[i].ceilingpicnum);
        sector[i].ceilingheinum = B_LITTLE16(sector[i].ceilingheinum);
        sector[i].floorpicnum   = B_LITTLE16(sector[i].floorpicnum);
        sector[i].floorheinum   = B_LITTLE16(sector[i].floorheinum);
        sector[i].lotag         = B_LITTLE16(sector[i].lotag);
        sector[i].hitag         = B_LITTLE16(sector[i].hitag);
        sector[i].extra         = B_LITTLE16(sector[i].extra);
    }

    kread(fil,&numwalls,2); numwalls = B_LITTLE16(numwalls);
    if (numwalls > MYMAXWALLS) { kclose(fil); return(-1); }
    kread(fil,&wall[0],sizeof(walltype)*numwalls);
    for (i=numwalls-1; i>=0; i--)
    {
        wall[i].x          = B_LITTLE32(wall[i].x);
        wall[i].y          = B_LITTLE32(wall[i].y);
        wall[i].point2     = B_LITTLE16(wall[i].point2);
        wall[i].nextwall   = B_LITTLE16(wall[i].nextwall);
        wall[i].nextsector = B_LITTLE16(wall[i].nextsector);
        wall[i].cstat      = B_LITTLE16(wall[i].cstat);
        wall[i].picnum     = B_LITTLE16(wall[i].picnum);
        wall[i].overpicnum = B_LITTLE16(wall[i].overpicnum);
        wall[i].lotag      = B_LITTLE16(wall[i].lotag);
        wall[i].hitag      = B_LITTLE16(wall[i].hitag);
        wall[i].extra      = B_LITTLE16(wall[i].extra);
    }

    kread(fil,&numsprites,2); numsprites = B_LITTLE16(numsprites);
    if (numsprites > MYMAXSPRITES) { kclose(fil); return(-1); }
    kread(fil,&sprite[0],sizeof(spritetype)*numsprites);
    for (i=numsprites-1; i>=0; i--)
    {
        sprite[i].x       = B_LITTLE32(sprite[i].x);
        sprite[i].y       = B_LITTLE32(sprite[i].y);
        sprite[i].z       = B_LITTLE32(sprite[i].z);
        sprite[i].cstat   = B_LITTLE16(sprite[i].cstat);
        sprite[i].picnum  = B_LITTLE16(sprite[i].picnum);
        sprite[i].sectnum = B_LITTLE16(sprite[i].sectnum);
        sprite[i].statnum = B_LITTLE16(sprite[i].statnum);
        sprite[i].ang     = B_LITTLE16(sprite[i].ang);
        sprite[i].owner   = B_LITTLE16(sprite[i].owner);
        sprite[i].xvel    = B_LITTLE16(sprite[i].xvel);
        sprite[i].yvel    = B_LITTLE16(sprite[i].yvel);
        sprite[i].zvel    = B_LITTLE16(sprite[i].zvel);
        sprite[i].lotag   = B_LITTLE16(sprite[i].lotag);
        sprite[i].hitag   = B_LITTLE16(sprite[i].hitag);
        sprite[i].extra   = B_LITTLE16(sprite[i].extra);

        if (sprite[i].sectnum<0||sprite[i].sectnum>=MYMAXSECTORS)
        {
            initprintf("Map error: sprite #%d(%d,%d) with an illegal sector(%d)\n",i,sprite[i].x,sprite[i].y,sprite[i].sectnum);
            sprite[i].sectnum=MYMAXSECTORS-1;
        }
    }

    for (i=0; i<numsprites; i++)
    {
        if ((sprite[i].cstat & 48) == 48) sprite[i].cstat &= ~48;
        insertsprite(sprite[i].sectnum,sprite[i].statnum);
    }

    //Must be after loading sectors, etc!
    updatesector(*daposx,*daposy,dacursectnum);

    kclose(fil);

#if defined(POLYMOST) && defined(USE_OPENGL)
    Bmemset(spriteext, 0, sizeof(spriteext_t) * MAXSPRITES);
    Bmemset(spritesmooth, 0, sizeof(spritesmooth_t) * (MAXSPRITES+MAXUNIQHUDID));

# ifdef POLYMER
    if (rendmode == 4)
        polymer_loadboard();
#endif
#endif
    guniqhudid = 0;

    startposx = *daposx;
    startposy = *daposy;
    startposz = *daposz;
    startang = *daang;
    startsectnum = *dacursectnum;

    return(0);
}


//
// loadboardv5/6
//
#ifdef __GNUC__
#define BPACK __attribute__ ((packed))
#else
#define BPACK
#endif

#ifdef _MSC_VER
#pragma pack(1)
#endif

#ifdef __WATCOMC__
#pragma pack(push,1);
#endif
struct BPACK sectortypev5
{
    uint16_t wallptr, wallnum;
    int16_t ceilingpicnum, floorpicnum;
    int16_t ceilingheinum, floorheinum;
    int32_t ceilingz, floorz;
    int8_t ceilingshade, floorshade;
    char ceilingxpanning, floorxpanning;
    char ceilingypanning, floorypanning;
    char ceilingstat, floorstat;
    char ceilingpal, floorpal;
    char visibility;
    int16_t lotag, hitag;
    int16_t extra;
};
struct BPACK walltypev5
{
    int32_t x, y;
    int16_t point2;
    int16_t picnum, overpicnum;
    int8_t shade;
    int16_t cstat;
    char xrepeat, yrepeat, xpanning, ypanning;
    int16_t nextsector1, nextwall1;
    int16_t nextsector2, nextwall2;
    int16_t lotag, hitag;
    int16_t extra;
};
struct BPACK spritetypev5
{
    int32_t x, y, z;
    char cstat;
    int8_t shade;
    char xrepeat, yrepeat;
    int16_t picnum, ang, xvel, yvel, zvel, owner;
    int16_t sectnum, statnum;
    int16_t lotag, hitag;
    int16_t extra;
};
struct BPACK sectortypev6
{
    uint16_t wallptr, wallnum;
    int16_t ceilingpicnum, floorpicnum;
    int16_t ceilingheinum, floorheinum;
    int32_t ceilingz, floorz;
    int8_t ceilingshade, floorshade;
    char ceilingxpanning, floorxpanning;
    char ceilingypanning, floorypanning;
    char ceilingstat, floorstat;
    char ceilingpal, floorpal;
    char visibility;
    int16_t lotag, hitag, extra;
};
struct BPACK walltypev6
{
    int32_t x, y;
    int16_t point2, nextsector, nextwall;
    int16_t picnum, overpicnum;
    int8_t shade;
    char pal;
    int16_t cstat;
    char xrepeat, yrepeat, xpanning, ypanning;
    int16_t lotag, hitag, extra;
};
struct BPACK spritetypev6
{
    int32_t x, y, z;
    int16_t cstat;
    int8_t shade;
    char pal, clipdist;
    char xrepeat, yrepeat;
    int8_t xoffset, yoffset;
    int16_t picnum, ang, xvel, yvel, zvel, owner;
    int16_t sectnum, statnum;
    int16_t lotag, hitag, extra;
};
#ifdef _MSC_VER
#pragma pack()
#endif

#ifdef __WATCOMC__
#pragma pack(pop)
#endif

#undef BPACK

static int16_t sectorofwallv5(int16_t theline)
{
    int16_t i, startwall, endwall, sucksect;

    sucksect = -1;
    for (i=0; i<numsectors; i++)
    {
        startwall = sector[i].wallptr;
        endwall = startwall + sector[i].wallnum - 1;
        if ((theline >= startwall) && (theline <= endwall))
        {
            sucksect = i;
            break;
        }
    }
    return(sucksect);
}

static void convertv5sectv6(struct sectortypev5 *from, struct sectortypev6 *to)
{
    to->wallptr = from->wallptr;
    to->wallnum = from->wallnum;
    to->ceilingpicnum = from->ceilingpicnum;
    to->floorpicnum = from->floorpicnum;
    to->ceilingheinum = from->ceilingheinum;
    to->floorheinum = from->floorheinum;
    to->ceilingz = from->ceilingz;
    to->floorz = from->floorz;
    to->ceilingshade = from->ceilingshade;
    to->floorshade = from->floorshade;
    to->ceilingxpanning = from->ceilingxpanning;
    to->floorxpanning = from->floorxpanning;
    to->ceilingypanning = from->ceilingypanning;
    to->floorypanning = from->floorypanning;
    to->ceilingstat = from->ceilingstat;
    to->floorstat = from->floorstat;
    to->ceilingpal = from->ceilingpal;
    to->floorpal = from->floorpal;
    to->visibility = from->visibility;
    to->lotag = from->lotag;
    to->hitag = from->hitag;
    to->extra = from->extra;
}

static void convertv5wallv6(struct walltypev5 *from, struct walltypev6 *to, int32_t i)
{
    to->x = from->x;
    to->y = from->y;
    to->point2 = from->point2;
    to->nextsector = from->nextsector1;
    to->nextwall = from->nextwall1;
    to->picnum = from->picnum;
    to->overpicnum = from->overpicnum;
    to->shade = from->shade;
    to->pal = sector[sectorofwallv5((int16_t)i)].floorpal;
    to->cstat = from->cstat;
    to->xrepeat = from->xrepeat;
    to->yrepeat = from->yrepeat;
    to->xpanning = from->xpanning;
    to->ypanning = from->ypanning;
    to->lotag = from->lotag;
    to->hitag = from->hitag;
    to->extra = from->extra;
}

static void convertv5sprv6(struct spritetypev5 *from, struct spritetypev6 *to)
{
    int16_t j;
    to->x = from->x;
    to->y = from->y;
    to->z = from->z;
    to->cstat = from->cstat;
    to->shade = from->shade;

    j = from->sectnum;
    if ((sector[j].ceilingstat&1) > 0)
        to->pal = sector[j].ceilingpal;
    else
        to->pal = sector[j].floorpal;

    to->clipdist = 32;
    to->xrepeat = from->xrepeat;
    to->yrepeat = from->yrepeat;
    to->xoffset = 0;
    to->yoffset = 0;
    to->picnum = from->picnum;
    to->ang = from->ang;
    to->xvel = from->xvel;
    to->yvel = from->yvel;
    to->zvel = from->zvel;
    to->owner = from->owner;
    to->sectnum = from->sectnum;
    to->statnum = from->statnum;
    to->lotag = from->lotag;
    to->hitag = from->hitag;
    to->extra = from->extra;
}

static void convertv6sectv7(struct sectortypev6 *from, sectortype *to)
{
    to->ceilingz = from->ceilingz;
    to->floorz = from->floorz;
    to->wallptr = from->wallptr;
    to->wallnum = from->wallnum;
    to->ceilingpicnum = from->ceilingpicnum;
    to->ceilingheinum = max(min(((int32_t)from->ceilingheinum)<<5,32767),-32768);
    if ((from->ceilingstat&2) == 0) to->ceilingheinum = 0;
    to->ceilingshade = from->ceilingshade;
    to->ceilingpal = from->ceilingpal;
    to->ceilingxpanning = from->ceilingxpanning;
    to->ceilingypanning = from->ceilingypanning;
    to->floorpicnum = from->floorpicnum;
    to->floorheinum = max(min(((int32_t)from->floorheinum)<<5,32767),-32768);
    if ((from->floorstat&2) == 0) to->floorheinum = 0;
    to->floorshade = from->floorshade;
    to->floorpal = from->floorpal;
    to->floorxpanning = from->floorxpanning;
    to->floorypanning = from->floorypanning;
    to->ceilingstat = from->ceilingstat;
    to->floorstat = from->floorstat;
    to->visibility = from->visibility;
    to->filler = 0;
    to->lotag = from->lotag;
    to->hitag = from->hitag;
    to->extra = from->extra;
}

static void convertv6wallv7(struct walltypev6 *from, walltype *to)
{
    to->x = from->x;
    to->y = from->y;
    to->point2 = from->point2;
    to->nextwall = from->nextwall;
    to->nextsector = from->nextsector;
    to->cstat = from->cstat;
    to->picnum = from->picnum;
    to->overpicnum = from->overpicnum;
    to->shade = from->shade;
    to->pal = from->pal;
    to->xrepeat = from->xrepeat;
    to->yrepeat = from->yrepeat;
    to->xpanning = from->xpanning;
    to->ypanning = from->ypanning;
    to->lotag = from->lotag;
    to->hitag = from->hitag;
    to->extra = from->extra;
}

static void convertv6sprv7(struct spritetypev6 *from, spritetype *to)
{
    to->x = from->x;
    to->y = from->y;
    to->z = from->z;
    to->cstat = from->cstat;
    to->picnum = from->picnum;
    to->shade = from->shade;
    to->pal = from->pal;
    to->clipdist = from->clipdist;
    to->filler = 0;
    to->xrepeat = from->xrepeat;
    to->yrepeat = from->yrepeat;
    to->xoffset = from->xoffset;
    to->yoffset = from->yoffset;
    to->sectnum = from->sectnum;
    to->statnum = from->statnum;
    to->ang = from->ang;
    to->owner = from->owner;
    to->xvel = from->xvel;
    to->yvel = from->yvel;
    to->zvel = from->zvel;
    to->lotag = from->lotag;
    to->hitag = from->hitag;
    to->extra = from->extra;
}

// Powerslave uses v6
// Witchaven 1 and TekWar and LameDuke use v5
int32_t loadoldboard(char *filename, char fromwhere, int32_t *daposx, int32_t *daposy, int32_t *daposz,
                     int16_t *daang, int16_t *dacursectnum)
{
    int16_t fil, i, numsprites;
    struct sectortypev5 v5sect;
    struct walltypev5   v5wall;
    struct spritetypev5 v5spr;
    struct sectortypev6 v6sect;
    struct walltypev6   v6wall;
    struct spritetypev6 v6spr;

    i = strlen(filename)-1;
    if (filename[i] == 255) { filename[i] = 0; fromwhere = 1; } // JBF 20040119: "compatibility"
    if ((fil = kopen4load(filename,fromwhere)) == -1)
        { mapversion = 5L; return(-1); }

    kread(fil,&mapversion,4); mapversion = B_LITTLE32(mapversion);
    if (mapversion != 5L && mapversion != 6L) { kclose(fil); return(-2); }

    initspritelists();

    clearbuf(&show2dsector[0],(int32_t)((MAXSECTORS+3)>>5),0L);
    clearbuf(&show2dsprite[0],(int32_t)((MAXSPRITES+3)>>5),0L);
    clearbuf(&show2dwall[0],(int32_t)((MAXWALLS+3)>>5),0L);

    kread(fil,daposx,4); *daposx = B_LITTLE32(*daposx);
    kread(fil,daposy,4); *daposy = B_LITTLE32(*daposy);
    kread(fil,daposz,4); *daposz = B_LITTLE32(*daposz);
    kread(fil,daang,2);  *daang  = B_LITTLE16(*daang);
    kread(fil,dacursectnum,2); *dacursectnum = B_LITTLE16(*dacursectnum);

    kread(fil,&numsectors,2); numsectors = B_LITTLE16(numsectors);
    if (numsectors > MAXSECTORS) { kclose(fil); return(-1); }
    for (i=0; i<numsectors; i++)
    {
        switch (mapversion)
        {
        case 5:
            kread(fil,&v5sect,sizeof(struct sectortypev5));
            v5sect.wallptr = B_LITTLE16(v5sect.wallptr);
            v5sect.wallnum = B_LITTLE16(v5sect.wallnum);
            v5sect.ceilingpicnum = B_LITTLE16(v5sect.ceilingpicnum);
            v5sect.floorpicnum = B_LITTLE16(v5sect.floorpicnum);
            v5sect.ceilingheinum = B_LITTLE16(v5sect.ceilingheinum);
            v5sect.floorheinum = B_LITTLE16(v5sect.floorheinum);
            v5sect.ceilingz = B_LITTLE32(v5sect.ceilingz);
            v5sect.floorz = B_LITTLE32(v5sect.floorz);
            v5sect.lotag = B_LITTLE16(v5sect.lotag);
            v5sect.hitag = B_LITTLE16(v5sect.hitag);
            v5sect.extra = B_LITTLE16(v5sect.extra);
            break;
        case 6:
            kread(fil,&v6sect,sizeof(struct sectortypev6));
            v6sect.wallptr = B_LITTLE16(v6sect.wallptr);
            v6sect.wallnum = B_LITTLE16(v6sect.wallnum);
            v6sect.ceilingpicnum = B_LITTLE16(v6sect.ceilingpicnum);
            v6sect.floorpicnum = B_LITTLE16(v6sect.floorpicnum);
            v6sect.ceilingheinum = B_LITTLE16(v6sect.ceilingheinum);
            v6sect.floorheinum = B_LITTLE16(v6sect.floorheinum);
            v6sect.ceilingz = B_LITTLE32(v6sect.ceilingz);
            v6sect.floorz = B_LITTLE32(v6sect.floorz);
            v6sect.lotag = B_LITTLE16(v6sect.lotag);
            v6sect.hitag = B_LITTLE16(v6sect.hitag);
            v6sect.extra = B_LITTLE16(v6sect.extra);
            break;
        }
        switch (mapversion)
        {
        case 5:
            convertv5sectv6(&v5sect,&v6sect);
        case 6:
            convertv6sectv7(&v6sect,&sector[i]);
        }
    }

    kread(fil,&numwalls,2); numwalls = B_LITTLE16(numwalls);
    if (numwalls > MAXWALLS) { kclose(fil); return(-1); }
    for (i=0; i<numwalls; i++)
    {
        switch (mapversion)
        {
        case 5:
            kread(fil,&v5wall,sizeof(struct walltypev5));
            v5wall.x = B_LITTLE32(v5wall.x);
            v5wall.y = B_LITTLE32(v5wall.y);
            v5wall.point2 = B_LITTLE16(v5wall.point2);
            v5wall.picnum = B_LITTLE16(v5wall.picnum);
            v5wall.overpicnum = B_LITTLE16(v5wall.overpicnum);
            v5wall.cstat = B_LITTLE16(v5wall.cstat);
            v5wall.nextsector1 = B_LITTLE16(v5wall.nextsector1);
            v5wall.nextwall1 = B_LITTLE16(v5wall.nextwall1);
            v5wall.nextsector2 = B_LITTLE16(v5wall.nextsector2);
            v5wall.nextwall2 = B_LITTLE16(v5wall.nextwall2);
            v5wall.lotag = B_LITTLE16(v5wall.lotag);
            v5wall.hitag = B_LITTLE16(v5wall.hitag);
            v5wall.extra = B_LITTLE16(v5wall.extra);
            break;
        case 6:
            kread(fil,&v6wall,sizeof(struct walltypev6));
            v6wall.x = B_LITTLE32(v6wall.x);
            v6wall.y = B_LITTLE32(v6wall.y);
            v6wall.point2 = B_LITTLE16(v6wall.point2);
            v6wall.nextsector = B_LITTLE16(v6wall.nextsector);
            v6wall.nextwall = B_LITTLE16(v6wall.nextwall);
            v6wall.picnum = B_LITTLE16(v6wall.picnum);
            v6wall.overpicnum = B_LITTLE16(v6wall.overpicnum);
            v6wall.cstat = B_LITTLE16(v6wall.cstat);
            v6wall.lotag = B_LITTLE16(v6wall.lotag);
            v6wall.hitag = B_LITTLE16(v6wall.hitag);
            v6wall.extra = B_LITTLE16(v6wall.extra);
            break;
        }
        switch (mapversion)
        {
        case 5:
            convertv5wallv6(&v5wall,&v6wall,i);
        case 6:
            convertv6wallv7(&v6wall,&wall[i]);
        }
    }

    kread(fil,&numsprites,2); numsprites = B_LITTLE16(numsprites);
    if (numsprites > MAXSPRITES) { kclose(fil); return(-1); }
    for (i=0; i<numsprites; i++)
    {
        switch (mapversion)
        {
        case 5:
            kread(fil,&v5spr,sizeof(struct spritetypev5));
            v5spr.x = B_LITTLE32(v5spr.x);
            v5spr.y = B_LITTLE32(v5spr.y);
            v5spr.z = B_LITTLE32(v5spr.z);
            v5spr.picnum = B_LITTLE16(v5spr.picnum);
            v5spr.ang = B_LITTLE16(v5spr.ang);
            v5spr.xvel = B_LITTLE16(v5spr.xvel);
            v5spr.yvel = B_LITTLE16(v5spr.yvel);
            v5spr.zvel = B_LITTLE16(v5spr.zvel);
            v5spr.owner = B_LITTLE16(v5spr.owner);
            v5spr.sectnum = B_LITTLE16(v5spr.sectnum);
            v5spr.statnum = B_LITTLE16(v5spr.statnum);
            v5spr.lotag = B_LITTLE16(v5spr.lotag);
            v5spr.hitag = B_LITTLE16(v5spr.hitag);
            v5spr.extra = B_LITTLE16(v5spr.extra);
            break;
        case 6:
            kread(fil,&v6spr,sizeof(struct spritetypev6));
            v6spr.x = B_LITTLE32(v6spr.x);
            v6spr.y = B_LITTLE32(v6spr.y);
            v6spr.z = B_LITTLE32(v6spr.z);
            v6spr.cstat = B_LITTLE16(v6spr.cstat);
            v6spr.picnum = B_LITTLE16(v6spr.picnum);
            v6spr.ang = B_LITTLE16(v6spr.ang);
            v6spr.xvel = B_LITTLE16(v6spr.xvel);
            v6spr.yvel = B_LITTLE16(v6spr.yvel);
            v6spr.zvel = B_LITTLE16(v6spr.zvel);
            v6spr.owner = B_LITTLE16(v6spr.owner);
            v6spr.sectnum = B_LITTLE16(v6spr.sectnum);
            v6spr.statnum = B_LITTLE16(v6spr.statnum);
            v6spr.lotag = B_LITTLE16(v6spr.lotag);
            v6spr.hitag = B_LITTLE16(v6spr.hitag);
            v6spr.extra = B_LITTLE16(v6spr.extra);
            break;
        }
        switch (mapversion)
        {
        case 5:
            convertv5sprv6(&v5spr,&v6spr);
        case 6:
            convertv6sprv7(&v6spr,&sprite[i]);
        }
    }

    for (i=0; i<numsprites; i++)
    {
        if ((sprite[i].cstat & 48) == 48) sprite[i].cstat &= ~48;
        insertsprite(sprite[i].sectnum,sprite[i].statnum);
    }

    //Must be after loading sectors, etc!
    updatesector(*daposx,*daposy,dacursectnum);

    kclose(fil);

#if defined(POLYMOST) && defined(USE_OPENGL)
    memset(spriteext, 0, sizeof(spriteext_t) * MAXSPRITES);
    memset(spritesmooth, 0, sizeof(spritesmooth_t) * (MAXSPRITES+MAXUNIQHUDID));
#endif
    guniqhudid = 0;

    return(0);
}


//
// loadmaphack
//
#if defined(POLYMOST) && defined(USE_OPENGL)
int32_t loadmaphack(char *filename)
{
    enum
    {
        T_EOF = -2,
        T_ERROR = -1,
        T_SPRITE = 0,
        T_ANGOFF,
        T_NOMODEL,
        T_NOANIM,
        T_PITCH,
        T_ROLL,
        T_MDXOFF,
        T_MDYOFF,
        T_MDZOFF,
        T_AWAY1,
        T_AWAY2,
        T_LIGHT,
    };

    static struct { char *text; int32_t tokenid; } legaltokens[] =
    {
        { "sprite", T_SPRITE },
        { "angleoff", T_ANGOFF },
        { "angoff", T_ANGOFF },
        { "notmd2", T_NOMODEL },
        { "notmd3", T_NOMODEL },
        { "notmd", T_NOMODEL },
        { "nomd2anim", T_NOANIM },
        { "nomd3anim", T_NOANIM },
        { "nomdanim", T_NOANIM },
        { "pitch", T_PITCH },
        { "roll", T_ROLL },
        { "mdxoff", T_MDXOFF },
        { "mdyoff", T_MDYOFF },
        { "mdzoff", T_MDZOFF },
        { "away1", T_AWAY1 },
        { "away2", T_AWAY2 },
        { "light", T_LIGHT },
        { NULL, -1 }
    };

    scriptfile *script;
    char *tok, *cmdtokptr;
    int32_t i;
    int32_t whichsprite = -1;

    script = scriptfile_fromfile(filename);
    if (!script) return -1;

    memset(spriteext, 0, sizeof(spriteext_t) * MAXSPRITES);
    memset(spritesmooth, 0, sizeof(spritesmooth_t) * (MAXSPRITES+MAXUNIQHUDID));

    while (1)
    {
        tok = scriptfile_gettoken(script);
        if (!tok) break;
        for (i=0; legaltokens[i].text; i++) if (!Bstrcasecmp(tok,legaltokens[i].text)) break;
        cmdtokptr = script->ltextptr;
        switch (legaltokens[i].tokenid)
        {
        case T_SPRITE:     // sprite <xx>
            if (scriptfile_getnumber(script, &whichsprite)) break;

            if ((unsigned)whichsprite >= (unsigned)MAXSPRITES)
            {
                // sprite number out of range
                initprintf("Sprite number out of range 0-%d on line %s:%d\n",
                           MAXSPRITES-1,script->filename, scriptfile_getlinum(script,cmdtokptr));
                whichsprite = -1;
                break;
            }

            break;
        case T_ANGOFF:     // angoff <xx>
        {
            int32_t ang;
            if (scriptfile_getnumber(script, &ang)) break;

            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring angle offset directive because of absent/invalid sprite number on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }
            spriteext[whichsprite].angoff = (int16_t)ang;
        }
        break;
        case T_NOMODEL:      // notmd
            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring not-MD2/MD3 directive because of absent/invalid sprite number on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }
            spriteext[whichsprite].flags |= SPREXT_NOTMD;
            break;
        case T_NOANIM:      // nomdanim
            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring no-MD2/MD3-anim directive because of absent/invalid sprite number on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }
            spriteext[whichsprite].flags |= SPREXT_NOMDANIM;
            break;
        case T_PITCH:     // pitch <xx>
        {
            int32_t pitch;
            if (scriptfile_getnumber(script, &pitch)) break;

            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring pitch directive because of absent/invalid sprite number on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }
            spriteext[whichsprite].pitch = (int16_t)pitch;
        }
        break;
        case T_ROLL:     // roll <xx>
        {
            int32_t roll;
            if (scriptfile_getnumber(script, &roll)) break;

            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring roll directive because of absent/invalid sprite number on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }
            spriteext[whichsprite].roll = (int16_t)roll;
        }
        break;
        case T_MDXOFF:     // mdxoff <xx>
        {
            int32_t i;
            if (scriptfile_getnumber(script, &i)) break;

            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring mdxoff directive because of absent/invalid sprite number on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }
            spriteext[whichsprite].xoff = i;
        }
        break;
        case T_MDYOFF:     // mdyoff <xx>
        {
            int32_t i;
            if (scriptfile_getnumber(script, &i)) break;

            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring mdyoff directive because of absent/invalid sprite number on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }
            spriteext[whichsprite].yoff = i;
        }
        break;
        case T_MDZOFF:     // mdzoff <xx>
        {
            int32_t i;
            if (scriptfile_getnumber(script, &i)) break;

            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring mdzoff directive because of absent/invalid sprite number on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }
            spriteext[whichsprite].zoff = i;
        }
        break;
        case T_AWAY1:      // away1
            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring moving away directive because of absent/invalid sprite number on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }
            spriteext[whichsprite].flags |= SPREXT_AWAY1;
            break;
        case T_AWAY2:      // away2
            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring moving away directive because of absent/invalid sprite number on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }
            spriteext[whichsprite].flags |= SPREXT_AWAY2;
            break;
#ifdef POLYMER
        case T_LIGHT:      // light sector x y z range r g b radius faderadius angle horiz minshade maxshade priority tilenum
        {
            int32_t value;

            scriptfile_getnumber(script, &value);
            staticlights[staticlightcount].sector = value;
            scriptfile_getnumber(script, &value);
            staticlights[staticlightcount].x = value;
            scriptfile_getnumber(script, &value);
            staticlights[staticlightcount].y = value;
            scriptfile_getnumber(script, &value);
            staticlights[staticlightcount].z = value;
            scriptfile_getnumber(script, &value);
            staticlights[staticlightcount].range = value;
            scriptfile_getnumber(script, &value);
            staticlights[staticlightcount].color[0] = value;
            scriptfile_getnumber(script, &value);
            staticlights[staticlightcount].color[1] = value;
            scriptfile_getnumber(script, &value);
            staticlights[staticlightcount].color[2] = value;
            scriptfile_getnumber(script, &value);
            staticlights[staticlightcount].radius = value;
            scriptfile_getnumber(script, &value);
            staticlights[staticlightcount].faderadius = value;
            scriptfile_getnumber(script, &value);
            staticlights[staticlightcount].angle = value;
            scriptfile_getnumber(script, &value);
            staticlights[staticlightcount].horiz = value;
            scriptfile_getnumber(script, &value);
            staticlights[staticlightcount].minshade = value;
            scriptfile_getnumber(script, &value);
            staticlights[staticlightcount].maxshade = value;
            scriptfile_getnumber(script, &value);
            staticlights[staticlightcount].priority = value;
            scriptfile_getnumber(script, &value);
            staticlights[staticlightcount].tilenum = value;

            staticlightcount++;
            break;
        }
#endif // POLYMER

        default:
            // unrecognised token
            break;
        }
    }

    scriptfile_close(script);
    return 0;
}
#else
int32_t loadmaphack(char *filename) { UNREFERENCED_PARAMETER(filename); return -1; }
#endif


//
// saveboard
//
int32_t saveboard(char *filename, int32_t *daposx, int32_t *daposy, int32_t *daposz,
                  int16_t *daang, int16_t *dacursectnum)
{
    int16_t fil, i, j, numsprites, ts;
    int32_t tl;
    sectortype *tsect = NULL;
    walltype   *twall = NULL;
    spritetype *tspri = NULL;
    sectortype *sec;
    walltype *wal;
    spritetype *spri;

    if ((fil = Bopen(filename,BO_BINARY|BO_TRUNC|BO_CREAT|BO_WRONLY,BS_IREAD|BS_IWRITE)) == -1)
        return(-1);

    for (j=0; j<MAXSPRITES; j++)if ((unsigned)sprite[j].statnum>MAXSTATUS)
        {
            initprintf("Map error: sprite #%d(%d,%d) with an illegal statnum(%d)\n",j,sprite[j].x,sprite[j].y,sprite[j].statnum);
            changespritestat(j,0);
        }

    numsprites = 0;
#if 0
    for (j=0; j<MAXSTATUS; j++)
    {
        i = headspritestat[j];
        while (i != -1)
        {
            numsprites++;
            i = nextspritestat[i];
        }
    }
#else
    for (j=0; j<MAXSPRITES; j++)
    {
        if (sprite[j].statnum != MAXSTATUS)
            numsprites++;
    }
#endif

    if (numsectors > MAXSECTORSV7 || numwalls > MAXWALLSV7 || numsprites > MAXSPRITESV7)
        mapversion = 8;
    else
        mapversion = 7;
    tl = B_LITTLE32(mapversion);    Bwrite(fil,&tl,4);

    tl = B_LITTLE32(*daposx);       Bwrite(fil,&tl,4);
    tl = B_LITTLE32(*daposy);       Bwrite(fil,&tl,4);
    tl = B_LITTLE32(*daposz);       Bwrite(fil,&tl,4);
    ts = B_LITTLE16(*daang);        Bwrite(fil,&ts,2);
    ts = B_LITTLE16(*dacursectnum); Bwrite(fil,&ts,2);

    ts = B_LITTLE16(numsectors);    Bwrite(fil,&ts,2);

    while (1)
    {
        tsect = (sectortype *)Bmalloc(sizeof(sectortype) * numsectors);

        if (tsect == NULL)
            break;

        Bmemcpy(&tsect[0], &sector[0], sizeof(sectortype) * numsectors);

        for (i=0; i<numsectors; i++)
        {
            sec = &tsect[i];
            sec->wallptr       = B_LITTLE16(sec->wallptr);
            sec->wallnum       = B_LITTLE16(sec->wallnum);
            sec->ceilingz      = B_LITTLE32(sec->ceilingz);
            sec->floorz        = B_LITTLE32(sec->floorz);
            sec->ceilingstat   = B_LITTLE16(sec->ceilingstat);
            sec->floorstat     = B_LITTLE16(sec->floorstat);
            sec->ceilingpicnum = B_LITTLE16(sec->ceilingpicnum);
            sec->ceilingheinum = B_LITTLE16(sec->ceilingheinum);
            sec->floorpicnum   = B_LITTLE16(sec->floorpicnum);
            sec->floorheinum   = B_LITTLE16(sec->floorheinum);
            sec->lotag         = B_LITTLE16(sec->lotag);
            sec->hitag         = B_LITTLE16(sec->hitag);
            sec->extra         = B_LITTLE16(sec->extra);
        }

        Bwrite(fil,&tsect[0],sizeof(sectortype) * numsectors);
        Bfree(tsect);

        ts = B_LITTLE16(numwalls);
        Bwrite(fil,&ts,2);

        twall = (walltype *)Bmalloc(sizeof(walltype) * numwalls);

        if (twall == NULL)
            break;

        Bmemcpy(&twall[0], &wall[0], sizeof(walltype) * numwalls);

        for (i=0; i<numwalls; i++)
        {
            wal = &twall[i];
            wal->x          = B_LITTLE32(wal->x);
            wal->y          = B_LITTLE32(wal->y);
            wal->point2     = B_LITTLE16(wal->point2);
            wal->nextwall   = B_LITTLE16(wal->nextwall);
            wal->nextsector = B_LITTLE16(wal->nextsector);
            wal->cstat      = B_LITTLE16(wal->cstat);
            wal->picnum     = B_LITTLE16(wal->picnum);
            wal->overpicnum = B_LITTLE16(wal->overpicnum);
            wal->lotag      = B_LITTLE16(wal->lotag);
            wal->hitag      = B_LITTLE16(wal->hitag);
            wal->extra      = B_LITTLE16(wal->extra);
        }

        Bwrite(fil,&twall[0],sizeof(walltype) * numwalls);
        Bfree(twall);

        ts = B_LITTLE16(numsprites);    Bwrite(fil,&ts,2);

        tspri = (spritetype *)Bmalloc(sizeof(spritetype) * numsprites);

        if (tspri == NULL)
            break;


        spri=tspri;

        for (j=0; j<MAXSPRITES; j++)
        {
            if (sprite[j].statnum != MAXSTATUS)
            {
                Bmemcpy(spri,&sprite[j],sizeof(spritetype));
                spri->x       = B_LITTLE32(spri->x);
                spri->y       = B_LITTLE32(spri->y);
                spri->z       = B_LITTLE32(spri->z);
                spri->cstat   = B_LITTLE16(spri->cstat);
                spri->picnum  = B_LITTLE16(spri->picnum);
                spri->sectnum = B_LITTLE16(spri->sectnum);
                spri->statnum = B_LITTLE16(spri->statnum);
                spri->ang     = B_LITTLE16(spri->ang);
                spri->owner   = B_LITTLE16(spri->owner);
                spri->xvel    = B_LITTLE16(spri->xvel);
                spri->yvel    = B_LITTLE16(spri->yvel);
                spri->zvel    = B_LITTLE16(spri->zvel);
                spri->lotag   = B_LITTLE16(spri->lotag);
                spri->hitag   = B_LITTLE16(spri->hitag);
                spri->extra   = B_LITTLE16(spri->extra);
                spri++;
            }
        }

        Bwrite(fil,&tspri[0],sizeof(spritetype) * numsprites);
        Bfree(tspri);

        Bclose(fil);
        return(0);
    }

    Bclose(fil);

    if (tsect != NULL)
        Bfree(tsect);

    if (twall != NULL)
        Bfree(twall);

    if (tspri != NULL)
        Bfree(tspri);

    return(-1);
}


//
// setgamemode
//
// JBF: davidoption now functions as a windowed-mode flag (0 == windowed, 1 == fullscreen)
extern char videomodereset;
int32_t setgamemode(char davidoption, int32_t daxdim, int32_t daydim, int32_t dabpp)
{
    int32_t i, j;

#if defined(USE_OPENGL) && defined(POLYMOST)
    extern char nogl;

    if (nogl) dabpp = 8;
#endif
    if ((qsetmode == 200) && (videomodereset == 0) &&
            (davidoption == fullscreen) && (xdim == daxdim) && (ydim == daydim) && (bpp == dabpp)
#ifdef POLYMER
            && glrendmode != 4
#endif // POLYMER
            )
        return(0);

    strcpy(kensmessage,"!!!! BUILD engine&tools programmed by Ken Silverman of E.G. RI.  (c) Copyright 1995 Ken Silverman.  Summary:  BUILD = Ken. !!!!");
    //  if (getkensmessagecrc(FP_OFF(kensmessage)) != 0x56c764d4)
    //      { printOSD("Nice try.\n"); exit(0); }

    //if (checkvideomode(&daxdim, &daydim, dabpp, davidoption)<0) return (-1);

    //bytesperline is set in this function

    j = bpp;
    if (setvideomode(daxdim,daydim,dabpp,davidoption) < 0) return(-1);

#if defined(POLYMOST) && defined(USE_OPENGL)
    if (dabpp > 8) rendmode = glrendmode;    // GL renderer
    else if (dabpp == 8 && j > 8) rendmode = 0; // going from GL to software activates softpolymost
#endif

    xdim = daxdim; ydim = daydim;

    j = ydim*4*sizeof(intptr_t);  //Leave room for horizlookup&horizlookup2

    if (lookups != NULL)
    {
        if (lookupsalloctype == 0) kkfree((void *)lookups);
        if (lookupsalloctype == 1) suckcache(lookups);
        lookups = NULL;
    }
    lookupsalloctype = 0;
    if ((lookups = (intptr_t *)kkmalloc(j<<1)) == NULL)
    {
        allocache((intptr_t *)&lookups,j<<1,&permanentlock);
        lookupsalloctype = 1;
    }

    horizlookup = lookups;
    horizlookup2 = (intptr_t *)((intptr_t)lookups+j); // FIXME_SA
    horizycent = ((ydim*4)>>1);

    //Force drawrooms to call dosetaspect & recalculate stuff
    oxyaspect = oxdimen = oviewingrange = -1;

    setvlinebpl(bytesperline);
    j = 0;
    for (i=0; i<=ydim; i++) ylookup[i] = j, j += bytesperline;

    setview(0L,0L,xdim-1,ydim-1);
    clearallviews(0L);
    setbrightness(curbrightness,palette,0);

    if (searchx < 0) { searchx = halfxdimen; searchy = (ydimen>>1); }

#if defined(POLYMOST) && defined(USE_OPENGL)
    if (rendmode >= 3)
    {
        polymost_glreset();
        polymost_glinit();
    }
# ifdef POLYMER
    if (rendmode == 4)
        polymer_init();
#endif
#endif
    qsetmode = 200;
    return(0);
}


//
// nextpage
//
void nextpage(void)
{
    int32_t i;
    permfifotype *per;

    //char snotbuf[32];
    //j = 0; k = 0;
    //for(i=0;i<4096;i++)
    //   if (waloff[i] != 0)
    //   {
    //      sprintf(snotbuf,"%d-%d",i,tilesizx[i]*tilesizy[i]);
    //      printext256((j>>5)*40+32,(j&31)*6,walock[i]>>3,-1,snotbuf,1);
    //      k += tilesizx[i]*tilesizy[i];
    //      j++;
    //   }
    //sprintf(snotbuf,"Total: %d",k);
    //printext256((j>>5)*40+32,(j&31)*6,31,-1,snotbuf,1);

    switch (qsetmode)
    {
    case 200:
        begindrawing(); //{{{
        for (i=permtail; i!=permhead; i=((i+1)&(MAXPERMS-1)))
        {
            per = &permfifo[i];
            if ((per->pagesleft > 0) && (per->pagesleft <= numpages))
                dorotatesprite(per->sx,per->sy,per->z,per->a,per->picnum,
                               per->dashade,per->dapalnum,per->dastat,
                               per->cx1,per->cy1,per->cx2,per->cy2,per->uniqid);
        }
        enddrawing();   //}}}

        OSD_Draw();
        showframe(0);

        begindrawing(); //{{{
        for (i=permtail; i!=permhead; i=((i+1)&(MAXPERMS-1)))
        {
            per = &permfifo[i];
            if (per->pagesleft >= 130)
                dorotatesprite(per->sx,per->sy,per->z,per->a,per->picnum,
                               per->dashade,per->dapalnum,per->dastat,
                               per->cx1,per->cy1,per->cx2,per->cy2,per->uniqid);

            if (per->pagesleft&127) per->pagesleft--;
            if (((per->pagesleft&127) == 0) && (i == permtail))
                permtail = ((permtail+1)&(MAXPERMS-1));
        }
        enddrawing();   //}}}
        break;

    case 350:
    case 480:
        break;
    }
    faketimerhandler();

    if ((totalclock >= lastageclock+8) || (totalclock < lastageclock))
        { lastageclock = totalclock; agecache(); }

#ifdef USE_OPENGL
    omdtims = mdtims; mdtims = getticks();

    {
        int32_t i;
        for (i=0; i<MAXSPRITES; i++)
            if ((mdpause&&spriteext[i].mdanimtims)||(spriteext[i].flags & SPREXT_NOMDANIM))
                spriteext[i].mdanimtims+=mdtims-omdtims;
    }
#endif

    beforedrawrooms = 1;
    numframes++;
}


//
// loadpics
//
int32_t loadpics(char *filename, int32_t askedsize)
{
    int32_t offscount, localtilestart, localtileend, dasiz;
    int16_t fil, i, j, k;

    Bstrcpy(artfilename,filename);

    for (i=0; i<MAXTILES; i++)
    {
        tilesizx[i] = 0;
        tilesizy[i] = 0;
        picanm[i] = 0L;
    }

    artsize = 0L;

    numtilefiles = 0;
    do
    {
        k = numtilefiles;

        artfilename[7] = (k%10)+48;
        artfilename[6] = ((k/10)%10)+48;
        artfilename[5] = ((k/100)%10)+48;
        if ((fil = kopen4load(artfilename,0)) != -1)
        {
            kread(fil,&artversion,4); artversion = B_LITTLE32(artversion);
            if (artversion != 1)
            {
                Bprintf("loadpics(): Invalid art file version in %s\n", artfilename);
                return(-1);
            }
            kread(fil,&numtiles,4);       numtiles       = B_LITTLE32(numtiles);
            kread(fil,&localtilestart,4); localtilestart = B_LITTLE32(localtilestart);
            kread(fil,&localtileend,4);   localtileend   = B_LITTLE32(localtileend);
            kread(fil,&tilesizx[localtilestart],(localtileend-localtilestart+1)<<1);
            kread(fil,&tilesizy[localtilestart],(localtileend-localtilestart+1)<<1);
            kread(fil,&picanm[localtilestart],(localtileend-localtilestart+1)<<2);
            for (i=localtilestart; i<=localtileend; i++)
            {
                tilesizx[i] = B_LITTLE16(tilesizx[i]);
                tilesizy[i] = B_LITTLE16(tilesizy[i]);
                picanm[i]   = B_LITTLE32(picanm[i]);
            }

            offscount = 4+4+4+4+((localtileend-localtilestart+1)<<3);
            for (i=localtilestart; i<=localtileend; i++)
            {
                tilefilenum[i] = k;
                tilefileoffs[i] = offscount;
                dasiz = (int32_t)(tilesizx[i]*tilesizy[i]);
                offscount += dasiz;
                artsize += ((dasiz+15)&0xfffffff0);
            }
            kclose(fil);
        }
        numtilefiles++;
    }
    while (k != numtilefiles && k < 64);

    clearbuf(&gotpic[0],(int32_t)((MAXTILES+31)>>5),0L);

    //try dpmi_DETERMINEMAXREALALLOC!

    //cachesize = min((int32_t)((Bgetsysmemsize()/100)*60),max(artsize,askedsize));
    if (Bgetsysmemsize() <= (uint32_t)askedsize)
        cachesize = (Bgetsysmemsize()/100)*60;
    else
        cachesize = askedsize;
    while ((pic = kkmalloc(cachesize)) == NULL)
    {
        cachesize -= 65536L;
        if (cachesize < 65536) return(-1);
    }
    initcache((intptr_t) pic, cachesize);

    for (i=0; i<MAXTILES; i++)
    {
        j = 15;
        while ((j > 1) && (pow2long[j] > tilesizx[i])) j--;
        picsiz[i] = ((uint8_t)j);
        j = 15;
        while ((j > 1) && (pow2long[j] > tilesizy[i])) j--;
        picsiz[i] += ((uint8_t)(j<<4));
    }

    artfil = -1;
    artfilnum = -1;
    artfilplc = 0L;

    return(0);
}


//
// loadtile
//
char cachedebug = 0;
char faketile[MAXTILES];
char *faketiledata[MAXTILES];
int32_t h_xsize[MAXTILES], h_ysize[MAXTILES];
int8_t h_xoffs[MAXTILES], h_yoffs[MAXTILES];

void loadtile(int16_t tilenume)
{
    char *ptr;
    int32_t i, dasiz;

    if ((unsigned)tilenume >= (unsigned)MAXTILES) return;
    dasiz = tilesizx[tilenume]*tilesizy[tilenume];
    if (dasiz <= 0) return;

    i = tilefilenum[tilenume];
    if (i != artfilnum)
    {
        if (artfil != -1) kclose(artfil);
        artfilnum = i;
        artfilplc = 0L;

        artfilename[7] = (i%10)+48;
        artfilename[6] = ((i/10)%10)+48;
        artfilename[5] = ((i/100)%10)+48;
        artfil = kopen4load(artfilename,0);
        faketimerhandler();
    }

    if (cachedebug) printOSD("Tile:%d\n",tilenume);

    if (waloff[tilenume] == 0)
    {
        walock[tilenume] = 199;
        allocache(&waloff[tilenume],dasiz,&walock[tilenume]);
    }

    if (!faketile[tilenume])
    {
        if (artfilplc != tilefileoffs[tilenume])
        {
            klseek(artfil,tilefileoffs[tilenume]-artfilplc,BSEEK_CUR);
            faketimerhandler();
        }
        ptr = (char *)waloff[tilenume];
        kread(artfil,ptr,dasiz);
        faketimerhandler();
        artfilplc = tilefileoffs[tilenume]+dasiz;
    }
    else
    {
        if (faketile[tilenume] == 1 || (faketile[tilenume] == 2 && faketiledata[tilenume] == NULL))
            Bmemset((char *)waloff[tilenume],0,dasiz);
        else if (faketile[tilenume] == 2)
            Bmemcpy((char *)waloff[tilenume],faketiledata[tilenume],dasiz);
        faketimerhandler();
    }
}

void checktile(int16_t tilenume)
{
    if (!waloff[tilenume])
        loadtile(tilenume);
}

//
// allocatepermanenttile
//
int32_t allocatepermanenttile(int16_t tilenume, int32_t xsiz, int32_t ysiz)
{
    int32_t j, dasiz;

    if ((xsiz <= 0) || (ysiz <= 0) || ((unsigned)tilenume >= (unsigned)MAXTILES))
        return(0);

    dasiz = xsiz*ysiz;

    walock[tilenume] = 255;
    allocache(&waloff[tilenume],dasiz,&walock[tilenume]);

    tilesizx[tilenume] = xsiz;
    tilesizy[tilenume] = ysiz;
    picanm[tilenume] = 0;

    j = 15; while ((j > 1) && (pow2long[j] > xsiz)) j--;
    picsiz[tilenume] = ((uint8_t)j);
    j = 15; while ((j > 1) && (pow2long[j] > ysiz)) j--;
    picsiz[tilenume] += ((uint8_t)(j<<4));

    return(waloff[tilenume]);
}


//
// copytilepiece
//
void copytilepiece(int32_t tilenume1, int32_t sx1, int32_t sy1, int32_t xsiz, int32_t ysiz,
                   int32_t tilenume2, int32_t sx2, int32_t sy2)
{
    char *ptr1, *ptr2, dat;
    int32_t xsiz1, ysiz1, xsiz2, ysiz2, i, j, x1, y1, x2, y2;

    xsiz1 = tilesizx[tilenume1]; ysiz1 = tilesizy[tilenume1];
    xsiz2 = tilesizx[tilenume2]; ysiz2 = tilesizy[tilenume2];
    if ((xsiz1 > 0) && (ysiz1 > 0) && (xsiz2 > 0) && (ysiz2 > 0))
    {
        if (waloff[tilenume1] == 0) loadtile(tilenume1);
        if (waloff[tilenume2] == 0) loadtile(tilenume2);

        x1 = sx1;
        for (i=0; i<xsiz; i++)
        {
            y1 = sy1;
            for (j=0; j<ysiz; j++)
            {
                x2 = sx2+i;
                y2 = sy2+j;
                if ((x2 >= 0) && (y2 >= 0) && (x2 < xsiz2) && (y2 < ysiz2))
                {
                    ptr1 = (char *)(waloff[tilenume1] + x1*ysiz1 + y1);
                    ptr2 = (char *)(waloff[tilenume2] + x2*ysiz2 + y2);
                    dat = *ptr1;
                    if (dat != 255)
                        *ptr2 = *ptr1;
                }

                y1++; if (y1 >= ysiz1) y1 = 0;
            }
            x1++; if (x1 >= xsiz1) x1 = 0;
        }
    }
}


//
// qloadkvx
//
#ifdef SUPERBUILD
int32_t qloadkvx(int32_t voxindex, char *filename)
{
    int32_t i, fil, dasiz, lengcnt, lengtot;
    char *ptr;

    if ((fil = kopen4load(filename,0)) == -1) return -1;

    lengcnt = 0;
    lengtot = kfilelength(fil);

    for (i=0; i<MAXVOXMIPS; i++)
    {
        kread(fil,&dasiz,4); dasiz = B_LITTLE32(dasiz);
        //Must store filenames to use cacheing system :(
        voxlock[voxindex][i] = 200;
        allocache(&voxoff[voxindex][i],dasiz,&voxlock[voxindex][i]);
        ptr = (char *)voxoff[voxindex][i];
        kread(fil,ptr,dasiz);

        lengcnt += dasiz+4;
        if (lengcnt >= lengtot-768) break;
    }
    kclose(fil);

#if defined POLYMOST && defined USE_OPENGL
    if (voxmodels[voxindex])
    {
        voxfree(voxmodels[voxindex]);
        voxmodels[voxindex] = NULL;
    }
    voxmodels[voxindex] = voxload(filename);
#endif
    return 0;
}
#endif


//
// clipinsidebox
//
int32_t clipinsidebox(int32_t x, int32_t y, int16_t wallnum, int32_t walldist)
{
    walltype *wal;
    int32_t x1, y1, x2, y2, r;

    r = (walldist<<1);
    wal = &wall[wallnum];     x1 = wal->x+walldist-x; y1 = wal->y+walldist-y;
    wal = &wall[wal->point2]; x2 = wal->x+walldist-x; y2 = wal->y+walldist-y;

    if ((x1 < 0) && (x2 < 0)) return(0);
    if ((y1 < 0) && (y2 < 0)) return(0);
    if ((x1 >= r) && (x2 >= r)) return(0);
    if ((y1 >= r) && (y2 >= r)) return(0);

    x2 -= x1; y2 -= y1;
    if (x2*(walldist-y1) >= y2*(walldist-x1))  //Front
    {
        if (x2 > 0) x2 *= (0-y1); else x2 *= (r-y1);
        if (y2 > 0) y2 *= (r-x1); else y2 *= (0-x1);
        return(x2 < y2);
    }
    if (x2 > 0) x2 *= (r-y1); else x2 *= (0-y1);
    if (y2 > 0) y2 *= (0-x1); else y2 *= (r-x1);
    return((x2 >= y2)<<1);
}


//
// clipinsideboxline
//
int32_t clipinsideboxline(int32_t x, int32_t y, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t walldist)
{
    int32_t r;

    r = (walldist<<1);

    x1 += walldist-x; x2 += walldist-x;
    if ((x1 < 0) && (x2 < 0)) return(0);
    if ((x1 >= r) && (x2 >= r)) return(0);

    y1 += walldist-y; y2 += walldist-y;
    if ((y1 < 0) && (y2 < 0)) return(0);
    if ((y1 >= r) && (y2 >= r)) return(0);

    x2 -= x1; y2 -= y1;
    if (x2*(walldist-y1) >= y2*(walldist-x1))  //Front
    {
        if (x2 > 0) x2 *= (0-y1); else x2 *= (r-y1);
        if (y2 > 0) y2 *= (r-x1); else y2 *= (0-x1);
        return(x2 < y2);
    }
    if (x2 > 0) x2 *= (r-y1); else x2 *= (0-y1);
    if (y2 > 0) y2 *= (0-x1); else y2 *= (r-x1);
    return((x2 >= y2)<<1);
}


//
// inside
//
int32_t inside(int32_t x, int32_t y, int16_t sectnum)
{
    walltype *wal;
    int32_t i, x1, y1, x2, y2;
    uint32_t cnt;

    if ((sectnum < 0) || (sectnum >= numsectors)) return(-1);

    cnt = 0;
    wal = &wall[sector[sectnum].wallptr];
    i = sector[sectnum].wallnum;
    do
    {
        y1 = wal->y-y; y2 = wall[wal->point2].y-y;
        if ((y1^y2) < 0)
        {
            x1 = wal->x-x; x2 = wall[wal->point2].x-x;
            if ((x1^x2) >= 0) cnt ^= x1; else cnt ^= (x1*y2-x2*y1)^y2;
        }
        wal++; i--;
    }
    while (i);
    return(cnt>>31);
}


//
// ksqrt
//
int32_t ksqrt(int32_t num)
{
    return(nsqrtasm(num));
}


//
// krecip
//
int32_t krecip(int32_t num)
{
    return(krecipasm(num));
}

//
// setsprite
//
int32_t setsprite(int16_t spritenum, const vec3_t *new)
{
    int16_t tempsectnum = sprite[spritenum].sectnum;

    if ((void *)new != (void *)&sprite[spritenum])
        Bmemcpy(&sprite[spritenum], new, sizeof(vec3_t));

    updatesector(new->x,new->y,&tempsectnum);

    if (tempsectnum < 0)
        return(-1);
    if (tempsectnum != sprite[spritenum].sectnum)
        changespritesect(spritenum,tempsectnum);

    return(0);
}

int32_t setspritez(int16_t spritenum, const vec3_t *new)
{
    int16_t tempsectnum = sprite[spritenum].sectnum;

    if ((void *)new != (void *)&sprite[spritenum])
        Bmemcpy(&sprite[spritenum], new, sizeof(vec3_t));

    updatesectorz(new->x,new->y,new->z,&tempsectnum);

    if (tempsectnum < 0)
        return(-1);
    if (tempsectnum != sprite[spritenum].sectnum)
        changespritesect(spritenum,tempsectnum);

    return(0);
}

//
// changespritesect
//
int32_t changespritesect(int16_t spritenum, int16_t newsectnum)
{
    if ((newsectnum < 0) || (newsectnum > MAXSECTORS)) return(-1);
    if (sprite[spritenum].sectnum == newsectnum) return(0);
    if (sprite[spritenum].sectnum == MAXSECTORS) return(-1);
    if (deletespritesect(spritenum) < 0) return(-1);
    insertspritesect(newsectnum);
    return(0);
}


//
// changespritestat
//
int32_t changespritestat(int16_t spritenum, int16_t newstatnum)
{
    if ((newstatnum < 0) || (newstatnum > MAXSTATUS)) return(-1);
    if (sprite[spritenum].statnum == newstatnum) return(0);
    if (sprite[spritenum].statnum == MAXSTATUS) return(-1);
    if (deletespritestat(spritenum) < 0) return(-1);
    insertspritestat(newstatnum);
    return(0);
}


//
// nextsectorneighborz
//
int32_t nextsectorneighborz(int16_t sectnum, int32_t thez, int16_t topbottom, int16_t direction)
{
    walltype *wal;
    int32_t i, testz, nextz;
    int16_t sectortouse;

    if (direction == 1) nextz = 0x7fffffff; else nextz = 0x80000000;

    sectortouse = -1;

    wal = &wall[sector[sectnum].wallptr];
    i = sector[sectnum].wallnum;
    do
    {
        if (wal->nextsector >= 0)
        {
            if (topbottom == 1)
            {
                testz = sector[wal->nextsector].floorz;
                if (direction == 1)
                {
                    if ((testz > thez) && (testz < nextz))
                    {
                        nextz = testz;
                        sectortouse = wal->nextsector;
                    }
                }
                else
                {
                    if ((testz < thez) && (testz > nextz))
                    {
                        nextz = testz;
                        sectortouse = wal->nextsector;
                    }
                }
            }
            else
            {
                testz = sector[wal->nextsector].ceilingz;
                if (direction == 1)
                {
                    if ((testz > thez) && (testz < nextz))
                    {
                        nextz = testz;
                        sectortouse = wal->nextsector;
                    }
                }
                else
                {
                    if ((testz < thez) && (testz > nextz))
                    {
                        nextz = testz;
                        sectortouse = wal->nextsector;
                    }
                }
            }
        }
        wal++;
        i--;
    }
    while (i != 0);

    return(sectortouse);
}


//
// cansee
//
int32_t cansee(int32_t x1, int32_t y1, int32_t z1, int16_t sect1, int32_t x2, int32_t y2, int32_t z2, int16_t sect2)
{
    sectortype *sec;
    walltype *wal, *wal2;
    int32_t i, cnt, nexts, x, y, z, cz, fz, dasectnum, dacnt, danum;
    int32_t x21, y21, z21, x31, y31, x34, y34, bot, t;

    if ((x1 == x2) && (y1 == y2)) return(sect1 == sect2);

    x21 = x2-x1; y21 = y2-y1; z21 = z2-z1;

    clipsectorlist[0] = sect1; danum = 1;
    for (dacnt=0; dacnt<danum; dacnt++)
    {
        dasectnum = clipsectorlist[dacnt]; sec = &sector[dasectnum];
        for (cnt=sec->wallnum,wal=&wall[sec->wallptr]; cnt>0; cnt--,wal++)
        {
            wal2 = &wall[wal->point2];
            x31 = wal->x-x1; x34 = wal->x-wal2->x;
            y31 = wal->y-y1; y34 = wal->y-wal2->y;

            bot = y21*x34-x21*y34; if (bot <= 0) continue;
            t = y21*x31-x21*y31; if ((unsigned)t >= (unsigned)bot) continue;
            t = y31*x34-x31*y34; if ((unsigned)t >= (unsigned)bot) continue;

            nexts = wal->nextsector;
            if ((nexts < 0) || (wal->cstat&32)) return(0);

            t = divscale24(t,bot);
            x = x1 + mulscale24(x21,t);
            y = y1 + mulscale24(y21,t);
            z = z1 + mulscale24(z21,t);

            getzsofslope((int16_t)dasectnum,x,y,&cz,&fz);
            if ((z <= cz) || (z >= fz)) return(0);
            getzsofslope((int16_t)nexts,x,y,&cz,&fz);
            if ((z <= cz) || (z >= fz)) return(0);

            for (i=danum-1; i>=0; i--) if (clipsectorlist[i] == nexts) break;
            if (i < 0) clipsectorlist[danum++] = nexts;
        }
    }
    for (i=danum-1; i>=0; i--) if (clipsectorlist[i] == sect2) return(1);
    return(0);
}


//
// hitscan
//
int32_t hitscan(const vec3_t *sv, int16_t sectnum, int32_t vx, int32_t vy, int32_t vz,
                hitdata_t *hitinfo, uint32_t cliptype)
{
    sectortype *sec;
    walltype *wal, *wal2;
    spritetype *spr;
    int32_t z, zz, x1, y1=0, z1=0, x2, y2, x3, y3, x4, y4, intx, inty, intz;
    int32_t topt, topu, bot, dist, offx, offy, cstat;
    int32_t i, j, k, l, tilenum, xoff, yoff, dax, day, daz, daz2;
    int32_t ang, cosang, sinang, xspan, yspan, xrepeat, yrepeat;
    int32_t dawalclipmask, dasprclipmask;
    int16_t tempshortcnt, tempshortnum, dasector, startwall, endwall;
    int16_t nextsector;
    char clipyou;

    hitinfo->hitsect = -1; hitinfo->hitwall = -1; hitinfo->hitsprite = -1;
    if (sectnum < 0) return(-1);

    hitinfo->pos.x = hitscangoalx; hitinfo->pos.y = hitscangoaly;

    dawalclipmask = (cliptype&65535);
    dasprclipmask = (cliptype>>16);

    clipsectorlist[0] = sectnum;
    tempshortcnt = 0; tempshortnum = 1;
    do
    {
        dasector = clipsectorlist[tempshortcnt]; sec = &sector[dasector];

        x1 = 0x7fffffff;
        if (sec->ceilingstat&2)
        {
            wal = &wall[sec->wallptr]; wal2 = &wall[wal->point2];
            dax = wal2->x-wal->x; day = wal2->y-wal->y;
            i = nsqrtasm(dax*dax+day*day); if (i == 0) continue;
            i = divscale15(sec->ceilingheinum,i);
            dax *= i; day *= i;

            j = (vz<<8)-dmulscale15(dax,vy,-day,vx);
            if (j != 0)
            {
                i = ((sec->ceilingz-sv->z)<<8)+dmulscale15(dax,sv->y-wal->y,-day,sv->x-wal->x);
                if (((i^j) >= 0) && ((klabs(i)>>1) < klabs(j)))
                {
                    i = divscale30(i,j);
                    x1 = sv->x + mulscale30(vx,i);
                    y1 = sv->y + mulscale30(vy,i);
                    z1 = sv->z + mulscale30(vz,i);
                }
            }
        }
        else if ((vz < 0) && (sv->z >= sec->ceilingz))
        {
            z1 = sec->ceilingz; i = z1-sv->z;
            if ((klabs(i)>>1) < -vz)
            {
                i = divscale30(i,vz);
                x1 = sv->x + mulscale30(vx,i);
                y1 = sv->y + mulscale30(vy,i);
            }
        }
        if ((x1 != 0x7fffffff) && (klabs(x1-sv->x)+klabs(y1-sv->y) < klabs((hitinfo->pos.x)-sv->x)+klabs((hitinfo->pos.y)-sv->y)))
            if (inside(x1,y1,dasector) != 0)
            {
                hitinfo->hitsect = dasector; hitinfo->hitwall = -1; hitinfo->hitsprite = -1;
                hitinfo->pos.x = x1; hitinfo->pos.y = y1; hitinfo->pos.z = z1;
            }

        x1 = 0x7fffffff;
        if (sec->floorstat&2)
        {
            wal = &wall[sec->wallptr]; wal2 = &wall[wal->point2];
            dax = wal2->x-wal->x; day = wal2->y-wal->y;
            i = nsqrtasm(dax*dax+day*day); if (i == 0) continue;
            i = divscale15(sec->floorheinum,i);
            dax *= i; day *= i;

            j = (vz<<8)-dmulscale15(dax,vy,-day,vx);
            if (j != 0)
            {
                i = ((sec->floorz-sv->z)<<8)+dmulscale15(dax,sv->y-wal->y,-day,sv->x-wal->x);
                if (((i^j) >= 0) && ((klabs(i)>>1) < klabs(j)))
                {
                    i = divscale30(i,j);
                    x1 = sv->x + mulscale30(vx,i);
                    y1 = sv->y + mulscale30(vy,i);
                    z1 = sv->z + mulscale30(vz,i);
                }
            }
        }
        else if ((vz > 0) && (sv->z <= sec->floorz))
        {
            z1 = sec->floorz; i = z1-sv->z;
            if ((klabs(i)>>1) < vz)
            {
                i = divscale30(i,vz);
                x1 = sv->x + mulscale30(vx,i);
                y1 = sv->y + mulscale30(vy,i);
            }
        }
        if ((x1 != 0x7fffffff) && (klabs(x1-sv->x)+klabs(y1-sv->y) < klabs((hitinfo->pos.x)-sv->x)+klabs((hitinfo->pos.y)-sv->y)))
            if (inside(x1,y1,dasector) != 0)
            {
                hitinfo->hitsect = dasector; hitinfo->hitwall = -1; hitinfo->hitsprite = -1;
                hitinfo->pos.x = x1; hitinfo->pos.y = y1; hitinfo->pos.z = z1;
            }

        startwall = sec->wallptr; endwall = startwall + sec->wallnum;
        for (z=startwall,wal=&wall[startwall]; z<endwall; z++,wal++)
        {
            wal2 = &wall[wal->point2];
            x1 = wal->x; y1 = wal->y; x2 = wal2->x; y2 = wal2->y;

            if ((x1-sv->x)*(y2-sv->y) < (x2-sv->x)*(y1-sv->y)) continue;
            if (rintersect(sv->x,sv->y,sv->z,vx,vy,vz,x1,y1,x2,y2,&intx,&inty,&intz) == 0) continue;

            if (klabs(intx-sv->x)+klabs(inty-sv->y) >= klabs((hitinfo->pos.x)-sv->x)+klabs((hitinfo->pos.y)-sv->y)) continue;

            nextsector = wal->nextsector;
            if ((nextsector < 0) || (wal->cstat&dawalclipmask))
            {
                hitinfo->hitsect = dasector; hitinfo->hitwall = z; hitinfo->hitsprite = -1;
                hitinfo->pos.x = intx; hitinfo->pos.y = inty; hitinfo->pos.z = intz;
                continue;
            }
            getzsofslope(nextsector,intx,inty,&daz,&daz2);
            if ((intz <= daz) || (intz >= daz2))
            {
                hitinfo->hitsect = dasector; hitinfo->hitwall = z; hitinfo->hitsprite = -1;
                hitinfo->pos.x = intx; hitinfo->pos.y = inty; hitinfo->pos.z = intz;
                continue;
            }

            for (zz=tempshortnum-1; zz>=0; zz--)
                if (clipsectorlist[zz] == nextsector) break;
            if (zz < 0) clipsectorlist[tempshortnum++] = nextsector;
        }

        for (z=headspritesect[dasector]; z>=0; z=nextspritesect[z])
        {
            spr = &sprite[z];
            cstat = spr->cstat;
#ifdef POLYMOST
            if (!hitallsprites)
#endif
                if ((cstat&dasprclipmask) == 0) continue;

            x1 = spr->x; y1 = spr->y; z1 = spr->z;
            switch (cstat&48)
            {
            case 0:
                topt = vx*(x1-sv->x) + vy*(y1-sv->y); if (topt <= 0) continue;
                bot = vx*vx + vy*vy; if (bot == 0) continue;

                intz = sv->z+scale(vz,topt,bot);

                i = (tilesizy[spr->picnum]*spr->yrepeat<<2);
                if (cstat&128) z1 += (i>>1);
                if (picanm[spr->picnum]&0x00ff0000) z1 -= ((int32_t)((int8_t)((picanm[spr->picnum]>>16)&255))*spr->yrepeat<<2);
                if ((intz > z1) || (intz < z1-i)) continue;
                topu = vx*(y1-sv->y) - vy*(x1-sv->x);

                offx = scale(vx,topu,bot);
                offy = scale(vy,topu,bot);
                dist = offx*offx + offy*offy;
                i = tilesizx[spr->picnum]*spr->xrepeat; i *= i;
                if (dist > (i>>7)) continue;
                intx = sv->x + scale(vx,topt,bot);
                inty = sv->y + scale(vy,topt,bot);

                if (klabs(intx-sv->x)+klabs(inty-sv->y) > klabs((hitinfo->pos.x)-sv->x)+klabs((hitinfo->pos.y)-sv->y)) continue;

                hitinfo->hitsect = dasector; hitinfo->hitwall = -1; hitinfo->hitsprite = z;
                hitinfo->pos.x = intx; hitinfo->pos.y = inty; hitinfo->pos.z = intz;
                break;
            case 16:
                //These lines get the 2 points of the rotated sprite
                //Given: (x1, y1) starts out as the center point
                tilenum = spr->picnum;
                xoff = (int32_t)((int8_t)((picanm[tilenum]>>8)&255))+((int32_t)spr->xoffset);
                if ((cstat&4) > 0) xoff = -xoff;
                k = spr->ang; l = spr->xrepeat;
                dax = sintable[k&2047]*l; day = sintable[(k+1536)&2047]*l;
                l = tilesizx[tilenum]; k = (l>>1)+xoff;
                x1 -= mulscale16(dax,k); x2 = x1+mulscale16(dax,l);
                y1 -= mulscale16(day,k); y2 = y1+mulscale16(day,l);

                if ((cstat&64) != 0)   //back side of 1-way sprite
                    if ((x1-sv->x)*(y2-sv->y) < (x2-sv->x)*(y1-sv->y)) continue;

                if (rintersect(sv->x,sv->y,sv->z,vx,vy,vz,x1,y1,x2,y2,&intx,&inty,&intz) == 0) continue;

                if (klabs(intx-sv->x)+klabs(inty-sv->y) > klabs((hitinfo->pos.x)-sv->x)+klabs((hitinfo->pos.y)-sv->y)) continue;

                k = ((tilesizy[spr->picnum]*spr->yrepeat)<<2);
                if (cstat&128) daz = spr->z+(k>>1); else daz = spr->z;
                if (picanm[spr->picnum]&0x00ff0000) daz -= ((int32_t)((int8_t)((picanm[spr->picnum]>>16)&255))*spr->yrepeat<<2);
                if ((intz < daz) && (intz > daz-k))
                {
                    hitinfo->hitsect = dasector; hitinfo->hitwall = -1; hitinfo->hitsprite = z;
                    hitinfo->pos.x = intx; hitinfo->pos.y = inty; hitinfo->pos.z = intz;
                }
                break;
            case 32:
                if (vz == 0) continue;
                intz = z1;
                if (((intz-sv->z)^vz) < 0) continue;
                if ((cstat&64) != 0)
                    if ((sv->z > intz) == ((cstat&8)==0)) continue;

#if 1           // Abyss crash prevention code ((intz-sv->z)*zx overflowing a 8-bit word)
                zz=(int32_t)((intz-sv->z)*vx);
                intx = sv->x+scale(zz,1,vz);
                zz=(int32_t)((intz-sv->z)*vy);
                inty = sv->y+scale(zz,1,vz);
#else
                intx = sv->x+scale(intz-sv->z,vx,vz);
                inty = sv->y+scale(intz-sv->z,vy,vz);
#endif

                if (klabs(intx-sv->x)+klabs(inty-sv->y) > klabs((hitinfo->pos.x)-sv->x)+klabs((hitinfo->pos.y)-sv->y)) continue;

                tilenum = spr->picnum;
                xoff = (int32_t)((int8_t)((picanm[tilenum]>>8)&255))+((int32_t)spr->xoffset);
                yoff = (int32_t)((int8_t)((picanm[tilenum]>>16)&255))+((int32_t)spr->yoffset);
                if ((cstat&4) > 0) xoff = -xoff;
                if ((cstat&8) > 0) yoff = -yoff;

                ang = spr->ang;
                cosang = sintable[(ang+512)&2047]; sinang = sintable[ang];
                xspan = tilesizx[tilenum]; xrepeat = spr->xrepeat;
                yspan = tilesizy[tilenum]; yrepeat = spr->yrepeat;

                dax = ((xspan>>1)+xoff)*xrepeat; day = ((yspan>>1)+yoff)*yrepeat;
                x1 += dmulscale16(sinang,dax,cosang,day)-intx;
                y1 += dmulscale16(sinang,day,-cosang,dax)-inty;
                l = xspan*xrepeat;
                x2 = x1 - mulscale16(sinang,l);
                y2 = y1 + mulscale16(cosang,l);
                l = yspan*yrepeat;
                k = -mulscale16(cosang,l); x3 = x2+k; x4 = x1+k;
                k = -mulscale16(sinang,l); y3 = y2+k; y4 = y1+k;

                clipyou = 0;
                if ((y1^y2) < 0)
                {
                    if ((x1^x2) < 0) clipyou ^= (x1*y2<x2*y1)^(y1<y2);
                    else if (x1 >= 0) clipyou ^= 1;
                }
                if ((y2^y3) < 0)
                {
                    if ((x2^x3) < 0) clipyou ^= (x2*y3<x3*y2)^(y2<y3);
                    else if (x2 >= 0) clipyou ^= 1;
                }
                if ((y3^y4) < 0)
                {
                    if ((x3^x4) < 0) clipyou ^= (x3*y4<x4*y3)^(y3<y4);
                    else if (x3 >= 0) clipyou ^= 1;
                }
                if ((y4^y1) < 0)
                {
                    if ((x4^x1) < 0) clipyou ^= (x4*y1<x1*y4)^(y4<y1);
                    else if (x4 >= 0) clipyou ^= 1;
                }

                if (clipyou != 0)
                {
                    hitinfo->hitsect = dasector; hitinfo->hitwall = -1; hitinfo->hitsprite = z;
                    hitinfo->pos.x = intx; hitinfo->pos.y = inty; hitinfo->pos.z = intz;
                }
                break;
            }
        }
        tempshortcnt++;
    }
    while (tempshortcnt < tempshortnum);
    return(0);
}


//
// neartag
//
int32_t neartag(int32_t xs, int32_t ys, int32_t zs, int16_t sectnum, int16_t ange, int16_t *neartagsector, int16_t *neartagwall, int16_t *neartagsprite, int32_t *neartaghitdist, int32_t neartagrange, char tagsearch)
{
    walltype *wal, *wal2;
    spritetype *spr;
    int32_t i, z, zz, xe, ye, ze, x1, y1, z1, x2, y2, intx, inty, intz;
    int32_t topt, topu, bot, dist, offx, offy, vx, vy, vz;
    int16_t tempshortcnt, tempshortnum, dasector, startwall, endwall;
    int16_t nextsector, good;

    *neartagsector = -1; *neartagwall = -1; *neartagsprite = -1;
    *neartaghitdist = 0;

    if (sectnum < 0) return(0);
    if ((tagsearch < 1) || (tagsearch > 3)) return(0);

    vx = mulscale14(sintable[(ange+2560)&2047],neartagrange); xe = xs+vx;
    vy = mulscale14(sintable[(ange+2048)&2047],neartagrange); ye = ys+vy;
    vz = 0; ze = 0;

    clipsectorlist[0] = sectnum;
    tempshortcnt = 0; tempshortnum = 1;

    do
    {
        dasector = clipsectorlist[tempshortcnt];

        startwall = sector[dasector].wallptr;
        endwall = startwall + sector[dasector].wallnum - 1;
        for (z=startwall,wal=&wall[startwall]; z<=endwall; z++,wal++)
        {
            wal2 = &wall[wal->point2];
            x1 = wal->x; y1 = wal->y; x2 = wal2->x; y2 = wal2->y;

            nextsector = wal->nextsector;

            good = 0;
            if (nextsector >= 0)
            {
                if ((tagsearch&1) && sector[nextsector].lotag) good |= 1;
                if ((tagsearch&2) && sector[nextsector].hitag) good |= 1;
            }
            if ((tagsearch&1) && wal->lotag) good |= 2;
            if ((tagsearch&2) && wal->hitag) good |= 2;

            if ((good == 0) && (nextsector < 0)) continue;
            if ((x1-xs)*(y2-ys) < (x2-xs)*(y1-ys)) continue;

            if (lintersect(xs,ys,zs,xe,ye,ze,x1,y1,x2,y2,&intx,&inty,&intz) == 1)
            {
                if (good != 0)
                {
                    if (good&1) *neartagsector = nextsector;
                    if (good&2) *neartagwall = z;
                    *neartaghitdist = dmulscale14(intx-xs,sintable[(ange+2560)&2047],inty-ys,sintable[(ange+2048)&2047]);
                    xe = intx; ye = inty; ze = intz;
                }
                if (nextsector >= 0)
                {
                    for (zz=tempshortnum-1; zz>=0; zz--)
                        if (clipsectorlist[zz] == nextsector) break;
                    if (zz < 0) clipsectorlist[tempshortnum++] = nextsector;
                }
            }
        }

        for (z=headspritesect[dasector]; z>=0; z=nextspritesect[z])
        {
            spr = &sprite[z];

            good = 0;
            if ((tagsearch&1) && spr->lotag) good |= 1;
            if ((tagsearch&2) && spr->hitag) good |= 1;
            if (good != 0)
            {
                x1 = spr->x; y1 = spr->y; z1 = spr->z;

                topt = vx*(x1-xs) + vy*(y1-ys);
                if (topt > 0)
                {
                    bot = vx*vx + vy*vy;
                    if (bot != 0)
                    {
                        intz = zs+scale(vz,topt,bot);
                        i = tilesizy[spr->picnum]*spr->yrepeat;
                        if (spr->cstat&128) z1 += (i<<1);
                        if (picanm[spr->picnum]&0x00ff0000) z1 -= ((int32_t)((int8_t)((picanm[spr->picnum]>>16)&255))*spr->yrepeat<<2);
                        if ((intz <= z1) && (intz >= z1-(i<<2)))
                        {
                            topu = vx*(y1-ys) - vy*(x1-xs);

                            offx = scale(vx,topu,bot);
                            offy = scale(vy,topu,bot);
                            dist = offx*offx + offy*offy;
                            i = (tilesizx[spr->picnum]*spr->xrepeat); i *= i;
                            if (dist <= (i>>7))
                            {
                                intx = xs + scale(vx,topt,bot);
                                inty = ys + scale(vy,topt,bot);
                                if (klabs(intx-xs)+klabs(inty-ys) < klabs(xe-xs)+klabs(ye-ys))
                                {
                                    *neartagsprite = z;
                                    *neartaghitdist = dmulscale14(intx-xs,sintable[(ange+2560)&2047],inty-ys,sintable[(ange+2048)&2047]);
                                    xe = intx;
                                    ye = inty;
                                    ze = intz;
                                }
                            }
                        }
                    }
                }
            }
        }

        tempshortcnt++;
    }
    while (tempshortcnt < tempshortnum);
    return(0);
}


//
// dragpoint
//
void dragpoint(int16_t pointhighlight, int32_t dax, int32_t day)
{
    int16_t cnt, tempshort;

    wall[pointhighlight].x = dax;
    wall[pointhighlight].y = day;
    wall[pointhighlight].cstat |= (1<<14);
    if (linehighlight >= 0 && linehighlight < MAXWALLS)
        wall[linehighlight].cstat |= (1<<14);
    wall[lastwall(pointhighlight)].cstat |= (1<<14);

    cnt = MAXWALLS;
    tempshort = pointhighlight;    //search points CCW
    do
    {
        if (wall[tempshort].nextwall >= 0)
        {
            tempshort = wall[wall[tempshort].nextwall].point2;
            wall[tempshort].x = dax;
            wall[tempshort].y = day;
            wall[tempshort].cstat |= (1<<14);
        }
        else
        {
            tempshort = pointhighlight;    //search points CW if not searched all the way around
            do
            {
                if (wall[lastwall(tempshort)].nextwall >= 0)
                {
                    tempshort = wall[lastwall(tempshort)].nextwall;
                    wall[tempshort].x = dax;
                    wall[tempshort].y = day;
                    wall[tempshort].cstat |= (1<<14);
                }
                else
                {
                    break;
                }
                cnt--;
            }
            while ((tempshort != pointhighlight) && (cnt > 0));
            break;
        }
        cnt--;
    }
    while ((tempshort != pointhighlight) && (cnt > 0));
}


//
// lastwall
//
int32_t lastwall(int16_t point)
{
    int32_t i, j, cnt;

    if ((point > 0) && (wall[point-1].point2 == point)) return(point-1);
    i = point;
    cnt = MAXWALLS;
    do
    {
        j = wall[i].point2;
        if (j == point) return(i);
        i = j;
        cnt--;
    }
    while (cnt > 0);
    return(point);
}



#define addclipline(dax1, day1, dax2, day2, daoval)      \
{                                                        \
    if (clipnum < MAXCLIPNUM) { \
    clipit[clipnum].x1 = dax1; clipit[clipnum].y1 = day1; \
    clipit[clipnum].x2 = dax2; clipit[clipnum].y2 = day2; \
    clipobjectval[clipnum] = daoval;                      \
    clipnum++;                                            \
    }                           \
}                                                        \
 
int32_t clipmoveboxtracenum = 3;

//
// clipmove
//
int32_t clipmove(vec3_t *vect, int16_t *sectnum,
                 int32_t xvect, int32_t yvect,
                 int32_t walldist, int32_t ceildist, int32_t flordist, uint32_t cliptype)
{
    walltype *wal, *wal2;
    spritetype *spr;
    sectortype *sec, *sec2;
    int32_t i, j, tempint1, tempint2;
    int32_t oxvect, oyvect, goalx, goaly, intx, inty, lx, ly, retval;
    int32_t k, l, clipsectcnt, startwall, endwall, cstat, dasect;
    int32_t x1, y1, x2, y2, cx, cy, rad, xmin, ymin, xmax, ymax, daz, daz2;
    int32_t bsz, dax, day, xoff, yoff, xspan, yspan, cosang, sinang, tilenum;
    int32_t xrepeat, yrepeat, gx, gy, dx, dy, dasprclipmask, dawalclipmask;
    int32_t hitwall, cnt, clipyou;

    if (((xvect|yvect) == 0) || (*sectnum < 0)) return(0);
    retval = 0;

    oxvect = xvect;
    oyvect = yvect;

    goalx = (vect->x) + (xvect>>14);
    goaly = (vect->y) + (yvect>>14);


    clipnum = 0;

    cx = (((vect->x)+goalx)>>1);
    cy = (((vect->y)+goaly)>>1);
    //Extra walldist for sprites on sector lines
    gx = goalx-(vect->x); gy = goaly-(vect->y);
    rad = nsqrtasm(gx*gx + gy*gy) + MAXCLIPDIST+walldist + 8;
    xmin = cx-rad; ymin = cy-rad;
    xmax = cx+rad; ymax = cy+rad;

    dawalclipmask = (cliptype&65535);        //CLIPMASK0 = 0x00010001
    dasprclipmask = (cliptype>>16);          //CLIPMASK1 = 0x01000040

    clipsectorlist[0] = (*sectnum);
    clipsectcnt = 0; clipsectnum = 1;
    do
    {
        dasect = clipsectorlist[clipsectcnt++];
        sec = &sector[dasect];
        startwall = sec->wallptr; endwall = startwall + sec->wallnum;
        for (j=startwall,wal=&wall[startwall]; j<endwall; j++,wal++)
        {
            wal2 = &wall[wal->point2];
            if ((wal->x < xmin) && (wal2->x < xmin)) continue;
            if ((wal->x > xmax) && (wal2->x > xmax)) continue;
            if ((wal->y < ymin) && (wal2->y < ymin)) continue;
            if ((wal->y > ymax) && (wal2->y > ymax)) continue;

            x1 = wal->x; y1 = wal->y; x2 = wal2->x; y2 = wal2->y;

            dx = x2-x1; dy = y2-y1;
            if (dx*((vect->y)-y1) < ((vect->x)-x1)*dy) continue;  //If wall's not facing you

            if (dx > 0) dax = dx*(ymin-y1); else dax = dx*(ymax-y1);
            if (dy > 0) day = dy*(xmax-x1); else day = dy*(xmin-x1);
            if (dax >= day) continue;

            clipyou = 0;
            if ((wal->nextsector < 0) || (wal->cstat&dawalclipmask)) clipyou = 1;
            else if (editstatus == 0)
            {
                if (rintersect(vect->x,vect->y,0,gx,gy,0,x1,y1,x2,y2,&dax,&day,&daz) == 0)
                    dax = vect->x, day = vect->y;
                daz = getflorzofslope((int16_t)dasect,dax,day);
                daz2 = getflorzofslope(wal->nextsector,dax,day);

                sec2 = &sector[wal->nextsector];
                if (daz2 < daz-(1<<8))
                    if ((sec2->floorstat&1) == 0)
                        if ((vect->z) >= daz2-(flordist-1)) clipyou = 1;
                if (clipyou == 0)
                {
                    daz = getceilzofslope((int16_t)dasect,dax,day);
                    daz2 = getceilzofslope(wal->nextsector,dax,day);
                    if (daz2 > daz+(1<<8))
                        if ((sec2->ceilingstat&1) == 0)
                            if ((vect->z) <= daz2+(ceildist-1)) clipyou = 1;
                }
            }

            if (clipyou)
            {
                //Add 2 boxes at endpoints
                bsz = walldist; if (gx < 0) bsz = -bsz;
                addclipline(x1-bsz,y1-bsz,x1-bsz,y1+bsz,(int16_t)j+32768);
                addclipline(x2-bsz,y2-bsz,x2-bsz,y2+bsz,(int16_t)j+32768);
                bsz = walldist; if (gy < 0) bsz = -bsz;
                addclipline(x1+bsz,y1-bsz,x1-bsz,y1-bsz,(int16_t)j+32768);
                addclipline(x2+bsz,y2-bsz,x2-bsz,y2-bsz,(int16_t)j+32768);

                dax = walldist; if (dy > 0) dax = -dax;
                day = walldist; if (dx < 0) day = -day;
                addclipline(x1+dax,y1+day,x2+dax,y2+day,(int16_t)j+32768);
            }
            else
            {
                for (i=clipsectnum-1; i>=0; i--)
                    if (wal->nextsector == clipsectorlist[i]) break;
                if (i < 0) clipsectorlist[clipsectnum++] = wal->nextsector;
            }
        }

        for (j=headspritesect[dasect]; j>=0; j=nextspritesect[j])
        {
            spr = &sprite[j];
            cstat = spr->cstat;
            if ((cstat&dasprclipmask) == 0) continue;
            x1 = spr->x; y1 = spr->y;
            switch (cstat&48)
            {
            case 0:
                if ((x1 >= xmin) && (x1 <= xmax) && (y1 >= ymin) && (y1 <= ymax))
                {
                    k = ((tilesizy[spr->picnum]*spr->yrepeat)<<2);
                    if (cstat&128) daz = spr->z+(k>>1); else daz = spr->z;
                    if (picanm[spr->picnum]&0x00ff0000) daz -= ((int32_t)((int8_t)((picanm[spr->picnum]>>16)&255))*spr->yrepeat<<2);
                    if (((vect->z) < daz+ceildist) && ((vect->z) > daz-k-flordist))
                    {
                        bsz = (spr->clipdist<<2)+walldist; if (gx < 0) bsz = -bsz;
                        addclipline(x1-bsz,y1-bsz,x1-bsz,y1+bsz,(int16_t)j+49152);
                        bsz = (spr->clipdist<<2)+walldist; if (gy < 0) bsz = -bsz;
                        addclipline(x1+bsz,y1-bsz,x1-bsz,y1-bsz,(int16_t)j+49152);
                    }
                }
                break;
            case 16:
                k = ((tilesizy[spr->picnum]*spr->yrepeat)<<2);
                if (cstat&128) daz = spr->z+(k>>1); else daz = spr->z;
                if (picanm[spr->picnum]&0x00ff0000) daz -= ((int32_t)((int8_t)((picanm[spr->picnum]>>16)&255))*spr->yrepeat<<2);
                daz2 = daz-k;
                daz += ceildist; daz2 -= flordist;
                if (((vect->z) < daz) && ((vect->z) > daz2))
                {
                    //These lines get the 2 points of the rotated sprite
                    //Given: (x1, y1) starts out as the center point
                    tilenum = spr->picnum;
                    xoff = (int32_t)((int8_t)((picanm[tilenum]>>8)&255))+((int32_t)spr->xoffset);
                    if ((cstat&4) > 0) xoff = -xoff;
                    k = spr->ang; l = spr->xrepeat;
                    dax = sintable[k&2047]*l; day = sintable[(k+1536)&2047]*l;
                    l = tilesizx[tilenum]; k = (l>>1)+xoff;
                    x1 -= mulscale16(dax,k); x2 = x1+mulscale16(dax,l);
                    y1 -= mulscale16(day,k); y2 = y1+mulscale16(day,l);
                    if (clipinsideboxline(cx,cy,x1,y1,x2,y2,rad) != 0)
                    {
                        dax = mulscale14(sintable[(spr->ang+256+512)&2047],walldist);
                        day = mulscale14(sintable[(spr->ang+256)&2047],walldist);

                        if ((x1-(vect->x))*(y2-(vect->y)) >= (x2-(vect->x))*(y1-(vect->y)))   //Front
                        {
                            addclipline(x1+dax,y1+day,x2+day,y2-dax,(int16_t)j+49152);
                        }
                        else
                        {
                            if ((cstat&64) != 0) continue;
                            addclipline(x2-dax,y2-day,x1-day,y1+dax,(int16_t)j+49152);
                        }

                        //Side blocker
                        if ((x2-x1)*((vect->x)-x1) + (y2-y1)*((vect->y)-y1) < 0)
                            { addclipline(x1-day,y1+dax,x1+dax,y1+day,(int16_t)j+49152); }
                        else if ((x1-x2)*((vect->x)-x2) + (y1-y2)*((vect->y)-y2) < 0)
                            { addclipline(x2+day,y2-dax,x2-dax,y2-day,(int16_t)j+49152); }
                    }
                }
                break;
            case 32:
                daz = spr->z+ceildist;
                daz2 = spr->z-flordist;
                if (((vect->z) < daz) && ((vect->z) > daz2))
                {
                    if ((cstat&64) != 0)
                        if (((vect->z) > spr->z) == ((cstat&8)==0)) continue;

                    tilenum = spr->picnum;
                    xoff = (int32_t)((int8_t)((picanm[tilenum]>>8)&255))+((int32_t)spr->xoffset);
                    yoff = (int32_t)((int8_t)((picanm[tilenum]>>16)&255))+((int32_t)spr->yoffset);
                    if ((cstat&4) > 0) xoff = -xoff;
                    if ((cstat&8) > 0) yoff = -yoff;

                    k = spr->ang;
                    cosang = sintable[(k+512)&2047]; sinang = sintable[k];
                    xspan = tilesizx[tilenum]; xrepeat = spr->xrepeat;
                    yspan = tilesizy[tilenum]; yrepeat = spr->yrepeat;

                    dax = ((xspan>>1)+xoff)*xrepeat; day = ((yspan>>1)+yoff)*yrepeat;
                    rxi[0] = x1 + dmulscale16(sinang,dax,cosang,day);
                    ryi[0] = y1 + dmulscale16(sinang,day,-cosang,dax);
                    l = xspan*xrepeat;
                    rxi[1] = rxi[0] - mulscale16(sinang,l);
                    ryi[1] = ryi[0] + mulscale16(cosang,l);
                    l = yspan*yrepeat;
                    k = -mulscale16(cosang,l); rxi[2] = rxi[1]+k; rxi[3] = rxi[0]+k;
                    k = -mulscale16(sinang,l); ryi[2] = ryi[1]+k; ryi[3] = ryi[0]+k;

                    dax = mulscale14(sintable[(spr->ang-256+512)&2047],walldist);
                    day = mulscale14(sintable[(spr->ang-256)&2047],walldist);

                    if ((rxi[0]-(vect->x))*(ryi[1]-(vect->y)) < (rxi[1]-(vect->x))*(ryi[0]-(vect->y)))
                    {
                        if (clipinsideboxline(cx,cy,rxi[1],ryi[1],rxi[0],ryi[0],rad) != 0)
                            addclipline(rxi[1]-day,ryi[1]+dax,rxi[0]+dax,ryi[0]+day,(int16_t)j+49152);
                    }
                    else if ((rxi[2]-(vect->x))*(ryi[3]-(vect->y)) < (rxi[3]-(vect->x))*(ryi[2]-(vect->y)))
                    {
                        if (clipinsideboxline(cx,cy,rxi[3],ryi[3],rxi[2],ryi[2],rad) != 0)
                            addclipline(rxi[3]+day,ryi[3]-dax,rxi[2]-dax,ryi[2]-day,(int16_t)j+49152);
                    }

                    if ((rxi[1]-(vect->x))*(ryi[2]-(vect->y)) < (rxi[2]-(vect->x))*(ryi[1]-(vect->y)))
                    {
                        if (clipinsideboxline(cx,cy,rxi[2],ryi[2],rxi[1],ryi[1],rad) != 0)
                            addclipline(rxi[2]-dax,ryi[2]-day,rxi[1]-day,ryi[1]+dax,(int16_t)j+49152);
                    }
                    else if ((rxi[3]-(vect->x))*(ryi[0]-(vect->y)) < (rxi[0]-(vect->x))*(ryi[3]-(vect->y)))
                    {
                        if (clipinsideboxline(cx,cy,rxi[0],ryi[0],rxi[3],ryi[3],rad) != 0)
                            addclipline(rxi[0]+dax,ryi[0]+day,rxi[3]+day,ryi[3]-dax,(int16_t)j+49152);
                    }
                }
                break;
            }
        }
    }
    while (clipsectcnt < clipsectnum);


    hitwall = 0;
    cnt = clipmoveboxtracenum;
    do
    {
        intx = goalx; inty = goaly;
        if ((hitwall = raytrace(vect->x, vect->y, &intx, &inty)) >= 0)
        {
            lx = clipit[hitwall].x2-clipit[hitwall].x1;
            ly = clipit[hitwall].y2-clipit[hitwall].y1;
            tempint2 = lx*lx + ly*ly;
            if (tempint2 > 0)
            {
                tempint1 = (goalx-intx)*lx + (goaly-inty)*ly;

                if ((klabs(tempint1)>>11) < tempint2)
                    i = divscale20(tempint1,tempint2);
                else
                    i = 0;
                goalx = mulscale20(lx,i)+intx;
                goaly = mulscale20(ly,i)+inty;
            }

            tempint1 = dmulscale6(lx,oxvect,ly,oyvect);
            for (i=cnt+1; i<=clipmoveboxtracenum; i++)
            {
                j = hitwalls[i];
                tempint2 = dmulscale6(clipit[j].x2-clipit[j].x1,oxvect,clipit[j].y2-clipit[j].y1,oyvect);
                if ((tempint1^tempint2) < 0)
                {
                    updatesector(vect->x,vect->y,sectnum);
                    return(retval);
                }
            }

            keepaway(&goalx, &goaly, hitwall);
            xvect = ((goalx-intx)<<14);
            yvect = ((goaly-inty)<<14);

            if (cnt == clipmoveboxtracenum) retval = clipobjectval[hitwall];
            hitwalls[cnt] = hitwall;
        }
        cnt--;

        vect->x = intx;
        vect->y = inty;
    }
    while (((xvect|yvect) != 0) && (hitwall >= 0) && (cnt > 0));

    for (j=0; j<clipsectnum; j++)
        if (inside(vect->x,vect->y,clipsectorlist[j]) == 1)
        {
            *sectnum = clipsectorlist[j];
            return(retval);
        }

    *sectnum = -1; tempint1 = 0x7fffffff;
    for (j=numsectors-1; j>=0; j--)
        if (inside(vect->x,vect->y,j) == 1)
        {
            if (sector[j].ceilingstat&2)
                tempint2 = (getceilzofslope((int16_t)j,vect->x,vect->y)-(vect->z));
            else
                tempint2 = (sector[j].ceilingz-(vect->z));

            if (tempint2 > 0)
            {
                if (tempint2 < tempint1)
                    { *sectnum = j; tempint1 = tempint2; }
            }
            else
            {
                if (sector[j].floorstat&2)
                    tempint2 = ((vect->z)-getflorzofslope((int16_t)j,vect->x,vect->y));
                else
                    tempint2 = ((vect->z)-sector[j].floorz);

                if (tempint2 <= 0)
                {
                    *sectnum = j;
                    return(retval);
                }
                if (tempint2 < tempint1)
                    { *sectnum = j; tempint1 = tempint2; }
            }
        }

    return(retval);
}


//
// pushmove
//
int32_t pushmove(vec3_t *vect, int16_t *sectnum,
                 int32_t walldist, int32_t ceildist, int32_t flordist, uint32_t cliptype)
{
    sectortype *sec, *sec2;
    walltype *wal;
    int32_t i, j, k, t, dx, dy, dax, day, daz, daz2, bad, dir;
    int32_t dasprclipmask, dawalclipmask;
    int16_t startwall, endwall, clipsectcnt;
    char bad2;

    if ((*sectnum) < 0) return(-1);

    dawalclipmask = (cliptype&65535);
    dasprclipmask = (cliptype>>16);

    k = 32;
    dir = 1;
    do
    {
        bad = 0;

        clipsectorlist[0] = *sectnum;
        clipsectcnt = 0; clipsectnum = 1;
        do
        {
            /*Push FACE sprites
            for(i=headspritesect[clipsectorlist[clipsectcnt]];i>=0;i=nextspritesect[i])
            {
            spr = &sprite[i];
            if (((spr->cstat&48) != 0) && ((spr->cstat&48) != 48)) continue;
            if ((spr->cstat&dasprclipmask) == 0) continue;

            dax = (vect->x)-spr->x; day = (vect->y)-spr->y;
            t = (spr->clipdist<<2)+walldist;
            if ((klabs(dax) < t) && (klabs(day) < t))
            {
            t = ((tilesizy[spr->picnum]*spr->yrepeat)<<2);
            if (spr->cstat&128) daz = spr->z+(t>>1); else daz = spr->z;
            if (picanm[spr->picnum]&0x00ff0000) daz -= ((int32_t)((int8_t)((picanm[spr->picnum]>>16)&255))*spr->yrepeat<<2);
            if (((vect->z) < daz+ceildist) && ((vect->z) > daz-t-flordist))
            {
            t = (spr->clipdist<<2)+walldist;

            j = getangle(dax,day);
            dx = (sintable[(j+512)&2047]>>11);
            dy = (sintable[(j)&2047]>>11);
            bad2 = 16;
            do
            {
            vect->x = (vect->x) + dx; vect->y = (vect->y) + dy;
            bad2--; if (bad2 == 0) break;
            } while ((klabs((vect->x)-spr->x) < t) && (klabs((vect->y)-spr->y) < t));
            bad = -1;
            k--; if (k <= 0) return(bad);
            updatesector(vect->x,vect->y,sectnum);
            }
            }
            }*/

            sec = &sector[clipsectorlist[clipsectcnt]];
            if (dir > 0)
                startwall = sec->wallptr, endwall = startwall + sec->wallnum;
            else
                endwall = sec->wallptr, startwall = endwall + sec->wallnum;

            for (i=startwall,wal=&wall[startwall]; i!=endwall; i+=dir,wal+=dir)
                if (clipinsidebox(vect->x,vect->y,i,walldist-4) == 1)
                {
                    j = 0;
                    if (wal->nextsector < 0) j = 1;
                    if (wal->cstat&dawalclipmask) j = 1;
                    if (j == 0)
                    {
                        sec2 = &sector[wal->nextsector];


                        //Find closest point on wall (dax, day) to (vect->x, vect->y)
                        dax = wall[wal->point2].x-wal->x;
                        day = wall[wal->point2].y-wal->y;
                        daz = dax*((vect->x)-wal->x) + day*((vect->y)-wal->y);
                        if (daz <= 0)
                            t = 0;
                        else
                        {
                            daz2 = dax*dax+day*day;
                            if (daz >= daz2) t = (1<<30); else t = divscale30(daz,daz2);
                        }
                        dax = wal->x + mulscale30(dax,t);
                        day = wal->y + mulscale30(day,t);


                        daz = getflorzofslope(clipsectorlist[clipsectcnt],dax,day);
                        daz2 = getflorzofslope(wal->nextsector,dax,day);
                        if ((daz2 < daz-(1<<8)) && ((sec2->floorstat&1) == 0))
                            if (vect->z >= daz2-(flordist-1)) j = 1;

                        daz = getceilzofslope(clipsectorlist[clipsectcnt],dax,day);
                        daz2 = getceilzofslope(wal->nextsector,dax,day);
                        if ((daz2 > daz+(1<<8)) && ((sec2->ceilingstat&1) == 0))
                            if (vect->z <= daz2+(ceildist-1)) j = 1;
                    }
                    if (j != 0)
                    {
                        j = getangle(wall[wal->point2].x-wal->x,wall[wal->point2].y-wal->y);
                        dx = (sintable[(j+1024)&2047]>>11);
                        dy = (sintable[(j+512)&2047]>>11);
                        bad2 = 16;
                        do
                        {
                            vect->x = (vect->x) + dx; vect->y = (vect->y) + dy;
                            bad2--; if (bad2 == 0) break;
                        }
                        while (clipinsidebox(vect->x,vect->y,i,walldist-4) != 0);
                        bad = -1;
                        k--; if (k <= 0) return(bad);
                        updatesector(vect->x,vect->y,sectnum);
                    }
                    else
                    {
                        for (j=clipsectnum-1; j>=0; j--)
                            if (wal->nextsector == clipsectorlist[j]) break;
                        if (j < 0) clipsectorlist[clipsectnum++] = wal->nextsector;
                    }
                }

            clipsectcnt++;
        }
        while (clipsectcnt < clipsectnum);
        dir = -dir;
    }
    while (bad != 0);

    return(bad);
}


//
// updatesector[z]
//
void updatesector(int32_t x, int32_t y, int16_t *sectnum)
{
    walltype *wal;
    int32_t i, j;

    if (inside(x,y,*sectnum) == 1) return;

    if ((*sectnum >= 0) && (*sectnum < numsectors))
    {
        wal = &wall[sector[*sectnum].wallptr];
        j = sector[*sectnum].wallnum;
        do
        {
            i = wal->nextsector;
            if (i >= 0)
                if (inside(x,y,(int16_t)i) == 1)
                {
                    *sectnum = i;
                    return;
                }
            wal++;
            j--;
        }
        while (j != 0);
    }

    for (i=numsectors-1; i>=0; i--)
        if (inside(x,y,(int16_t)i) == 1)
        {
            *sectnum = i;
            return;
        }

    *sectnum = -1;
}

void updatesectorz(int32_t x, int32_t y, int32_t z, int16_t *sectnum)
{
    walltype *wal;
    int32_t i, j, cz, fz;

    getzsofslope(*sectnum, x, y, &cz, &fz);
    if ((z >= cz) && (z <= fz))
        if (inside(x,y,*sectnum) != 0) return;

    if ((*sectnum >= 0) && (*sectnum < numsectors))
    {
        wal = &wall[sector[*sectnum].wallptr];
        j = sector[*sectnum].wallnum;
        do
        {
            i = wal->nextsector;
            if (i >= 0)
            {
                getzsofslope(i, x, y, &cz, &fz);
                if ((z >= cz) && (z <= fz))
                    if (inside(x,y,(int16_t)i) == 1)
                        { *sectnum = i; return; }
            }
            wal++; j--;
        }
        while (j != 0);
    }

    for (i=numsectors-1; i>=0; i--)
    {
        getzsofslope(i, x, y, &cz, &fz);
        if ((z >= cz) && (z <= fz))
            if (inside(x,y,(int16_t)i) == 1)
                { *sectnum = i; return; }
    }

    *sectnum = -1;
}


//
// rotatepoint
//
void rotatepoint(int32_t xpivot, int32_t ypivot, int32_t x, int32_t y, int16_t daang, int32_t *x2, int32_t *y2)
{
    int32_t dacos, dasin;

    dacos = sintable[(daang+2560)&2047];
    dasin = sintable[(daang+2048)&2047];
    x -= xpivot;
    y -= ypivot;
    *x2 = dmulscale14(x,dacos,-y,dasin) + xpivot;
    *y2 = dmulscale14(y,dacos,x,dasin) + ypivot;
}


//
// getmousevalues
//

void getmousevalues(int32_t *mousx, int32_t *mousy, int32_t *bstatus)
{
    readmousexy(mousx,mousy);
    readmousebstatus(bstatus);
}


//
// krand
//
int32_t krand(void)
{
//    randomseed = (randomseed*27584621)+1;
    randomseed = (randomseed * 1664525ul) + 221297ul;
    return(((uint32_t)randomseed)>>16);
}


//
// getzrange
//
void getzrange(const vec3_t *vect, int16_t sectnum,
               int32_t *ceilz, int32_t *ceilhit, int32_t *florz, int32_t *florhit,
               int32_t walldist, uint32_t cliptype)
{
    sectortype *sec;
    walltype *wal, *wal2;
    spritetype *spr;
    int32_t clipsectcnt, startwall, endwall, tilenum, xoff, yoff, dax, day;
    int32_t xmin, ymin, xmax, ymax, i, j, k, l, daz, daz2, dx, dy;
    int32_t x1, y1, x2, y2, x3, y3, x4, y4, ang, cosang, sinang;
    int32_t xspan, yspan, xrepeat, yrepeat, dasprclipmask, dawalclipmask;
    int16_t cstat;

    char clipyou;

    if (sectnum < 0)
    {
        *ceilz = 0x80000000; *ceilhit = -1;
        *florz = 0x7fffffff; *florhit = -1;
        return;
    }

    //Extra walldist for sprites on sector lines
    i = walldist+MAXCLIPDIST+1;
    xmin = vect->x-i; ymin = vect->y-i;
    xmax = vect->x+i; ymax = vect->y+i;

    getzsofslope(sectnum,vect->x,vect->y,ceilz,florz);
    *ceilhit = sectnum+16384; *florhit = sectnum+16384;

    dawalclipmask = (cliptype&65535);
    dasprclipmask = (cliptype>>16);

    clipsectorlist[0] = sectnum;
    clipsectcnt = 0; clipsectnum = 1;

    do  //Collect sectors inside your square first
    {
        sec = &sector[clipsectorlist[clipsectcnt]];
        startwall = sec->wallptr; endwall = startwall + sec->wallnum;
        for (j=startwall,wal=&wall[startwall]; j<endwall; j++,wal++)
        {
            k = wal->nextsector;
            if (k >= 0)
            {
                wal2 = &wall[wal->point2];
                x1 = wal->x; x2 = wal2->x;
                if ((x1 < xmin) && (x2 < xmin)) continue;
                if ((x1 > xmax) && (x2 > xmax)) continue;
                y1 = wal->y; y2 = wal2->y;
                if ((y1 < ymin) && (y2 < ymin)) continue;
                if ((y1 > ymax) && (y2 > ymax)) continue;

                dx = x2-x1; dy = y2-y1;
                if (dx*(vect->y-y1) < (vect->x-x1)*dy) continue; //back
                if (dx > 0) dax = dx*(ymin-y1); else dax = dx*(ymax-y1);
                if (dy > 0) day = dy*(xmax-x1); else day = dy*(xmin-x1);
                if (dax >= day) continue;

                if (wal->cstat&dawalclipmask) continue;
                sec = &sector[k];
                if (editstatus == 0)
                {
                    if (((sec->ceilingstat&1) == 0) && (vect->z <= sec->ceilingz+(3<<8))) continue;
                    if (((sec->floorstat&1) == 0) && (vect->z >= sec->floorz-(3<<8))) continue;
                }

                for (i=clipsectnum-1; i>=0; i--) if (clipsectorlist[i] == k) break;
                if (i < 0) clipsectorlist[clipsectnum++] = k;

                if ((x1 < xmin+MAXCLIPDIST) && (x2 < xmin+MAXCLIPDIST)) continue;
                if ((x1 > xmax-MAXCLIPDIST) && (x2 > xmax-MAXCLIPDIST)) continue;
                if ((y1 < ymin+MAXCLIPDIST) && (y2 < ymin+MAXCLIPDIST)) continue;
                if ((y1 > ymax-MAXCLIPDIST) && (y2 > ymax-MAXCLIPDIST)) continue;
                if (dx > 0) dax += dx*MAXCLIPDIST; else dax -= dx*MAXCLIPDIST;
                if (dy > 0) day -= dy*MAXCLIPDIST; else day += dy*MAXCLIPDIST;
                if (dax >= day) continue;

                //It actually got here, through all the continue's!!!
                getzsofslope((int16_t)k,vect->x,vect->y,&daz,&daz2);
                if (daz > *ceilz) { *ceilz = daz; *ceilhit = k+16384; }
                if (daz2 < *florz) { *florz = daz2; *florhit = k+16384; }
            }
        }
        clipsectcnt++;
    }
    while (clipsectcnt < clipsectnum);

    for (i=0; i<clipsectnum; i++)
    {
        for (j=headspritesect[clipsectorlist[i]]; j>=0; j=nextspritesect[j])
        {
            spr = &sprite[j];
            cstat = spr->cstat;
            if (cstat&dasprclipmask)
            {
                x1 = spr->x; y1 = spr->y;

                clipyou = 0;
                switch (cstat&48)
                {
                case 0:
                    k = walldist+(spr->clipdist<<2)+1;
                    if ((klabs(x1-vect->x) <= k) && (klabs(y1-vect->y) <= k))
                    {
                        daz = spr->z;
                        k = ((tilesizy[spr->picnum]*spr->yrepeat)<<1);
                        if (cstat&128) daz += k;
                        if (picanm[spr->picnum]&0x00ff0000) daz -= ((int32_t)((int8_t)((picanm[spr->picnum]>>16)&255))*spr->yrepeat<<2);
                        daz2 = daz - (k<<1);
                        clipyou = 1;
                    }
                    break;
                case 16:
                    tilenum = spr->picnum;
                    xoff = (int32_t)((int8_t)((picanm[tilenum]>>8)&255))+((int32_t)spr->xoffset);
                    if ((cstat&4) > 0) xoff = -xoff;
                    k = spr->ang; l = spr->xrepeat;
                    dax = sintable[k&2047]*l; day = sintable[(k+1536)&2047]*l;
                    l = tilesizx[tilenum]; k = (l>>1)+xoff;
                    x1 -= mulscale16(dax,k); x2 = x1+mulscale16(dax,l);
                    y1 -= mulscale16(day,k); y2 = y1+mulscale16(day,l);
                    if (clipinsideboxline(vect->x,vect->y,x1,y1,x2,y2,walldist+1) != 0)
                    {
                        daz = spr->z; k = ((tilesizy[spr->picnum]*spr->yrepeat)<<1);
                        if (cstat&128) daz += k;
                        if (picanm[spr->picnum]&0x00ff0000) daz -= ((int32_t)((int8_t)((picanm[spr->picnum]>>16)&255))*spr->yrepeat<<2);
                        daz2 = daz-(k<<1);
                        clipyou = 1;
                    }
                    break;
                case 32:
                    daz = spr->z; daz2 = daz;

                    if ((cstat&64) != 0)
                        if ((vect->z > daz) == ((cstat&8)==0)) continue;

                    tilenum = spr->picnum;
                    xoff = (int32_t)((int8_t)((picanm[tilenum]>>8)&255))+((int32_t)spr->xoffset);
                    yoff = (int32_t)((int8_t)((picanm[tilenum]>>16)&255))+((int32_t)spr->yoffset);
                    if ((cstat&4) > 0) xoff = -xoff;
                    if ((cstat&8) > 0) yoff = -yoff;

                    ang = spr->ang;
                    cosang = sintable[(ang+512)&2047]; sinang = sintable[ang];
                    xspan = tilesizx[tilenum]; xrepeat = spr->xrepeat;
                    yspan = tilesizy[tilenum]; yrepeat = spr->yrepeat;

                    dax = ((xspan>>1)+xoff)*xrepeat; day = ((yspan>>1)+yoff)*yrepeat;
                    x1 += dmulscale16(sinang,dax,cosang,day)-vect->x;
                    y1 += dmulscale16(sinang,day,-cosang,dax)-vect->y;
                    l = xspan*xrepeat;
                    x2 = x1 - mulscale16(sinang,l);
                    y2 = y1 + mulscale16(cosang,l);
                    l = yspan*yrepeat;
                    k = -mulscale16(cosang,l); x3 = x2+k; x4 = x1+k;
                    k = -mulscale16(sinang,l); y3 = y2+k; y4 = y1+k;

                    dax = mulscale14(sintable[(spr->ang-256+512)&2047],walldist+4);
                    day = mulscale14(sintable[(spr->ang-256)&2047],walldist+4);
                    x1 += dax; x2 -= day; x3 -= dax; x4 += day;
                    y1 += day; y2 += dax; y3 -= day; y4 -= dax;

                    if ((y1^y2) < 0)
                    {
                        if ((x1^x2) < 0) clipyou ^= (x1*y2<x2*y1)^(y1<y2);
                        else if (x1 >= 0) clipyou ^= 1;
                    }
                    if ((y2^y3) < 0)
                    {
                        if ((x2^x3) < 0) clipyou ^= (x2*y3<x3*y2)^(y2<y3);
                        else if (x2 >= 0) clipyou ^= 1;
                    }
                    if ((y3^y4) < 0)
                    {
                        if ((x3^x4) < 0) clipyou ^= (x3*y4<x4*y3)^(y3<y4);
                        else if (x3 >= 0) clipyou ^= 1;
                    }
                    if ((y4^y1) < 0)
                    {
                        if ((x4^x1) < 0) clipyou ^= (x4*y1<x1*y4)^(y4<y1);
                        else if (x4 >= 0) clipyou ^= 1;
                    }
                    break;
                }

                if (clipyou != 0)
                {
                    if ((vect->z > daz) && (daz > *ceilz)) { *ceilz = daz; *ceilhit = j+49152; }
                    if ((vect->z < daz2) && (daz2 < *florz)) { *florz = daz2; *florhit = j+49152; }
                }
            }
        }
    }
}


//
// setview
//
void setview(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    int32_t i;

    windowx1 = x1; wx1 = (x1<<12);
    windowy1 = y1; wy1 = (y1<<12);
    windowx2 = x2; wx2 = ((x2+1)<<12);
    windowy2 = y2; wy2 = ((y2+1)<<12);

    xdimen = (x2-x1)+1; halfxdimen = (xdimen>>1);
    xdimenrecip = divscale32(1L,xdimen);
    ydimen = (y2-y1)+1;

    setaspect(65536L,(int32_t)divscale16(ydim*320L,xdim*200L));

    for (i=0; i<windowx1; i++) { startumost[i] = 1, startdmost[i] = 0; }
    for (i=windowx1; i<=windowx2; i++)
        { startumost[i] = windowy1, startdmost[i] = windowy2+1; }
    for (i=windowx2+1; i<xdim; i++) { startumost[i] = 1, startdmost[i] = 0; }

    /*
    begindrawing(); //{{{
    viewoffset = windowy1*bytesperline + windowx1;
    enddrawing();   //}}}
    */
}


//
// setaspect
//
void setaspect(int32_t daxrange, int32_t daaspect)
{
    viewingrange = daxrange;
    viewingrangerecip = divscale32(1L,daxrange);

    yxaspect = daaspect;
    xyaspect = divscale32(1,yxaspect);
    xdimenscale = scale(xdimen,yxaspect,320);
    xdimscale = scale(320,xyaspect,xdimen);
}


//
// flushperms
//
void flushperms(void)
{
    permhead = permtail = 0;
}


//
// rotatesprite
//
void rotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum, int8_t dashade, char dapalnum, char dastat, int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2)
{
    int32_t i;
    permfifotype *per, *per2;

    if ((cx1 > cx2) || (cy1 > cy2)) return;
    if (z <= 16) return;
    if (picanm[picnum]&192) picnum += animateoffs(picnum,(int16_t)0xc000);
    if ((tilesizx[picnum] <= 0) || (tilesizy[picnum] <= 0)) return;

    if (((dastat&128) == 0) || (numpages < 2) || (beforedrawrooms != 0))
    {
        begindrawing(); //{{{
        dorotatesprite(sx,sy,z,a,picnum,dashade,dapalnum,dastat,cx1,cy1,cx2,cy2,guniqhudid);
        enddrawing();   //}}}
    }

    if ((dastat&64) && (cx1 <= 0) && (cy1 <= 0) && (cx2 >= xdim-1) && (cy2 >= ydim-1) &&
            (sx == (160<<16)) && (sy == (100<<16)) && (z == 65536L) && (a == 0) && ((dastat&1) == 0))
        permhead = permtail = 0;

    if ((dastat&128) == 0) return;
    if (numpages >= 2)
    {
        per = &permfifo[permhead];
        per->sx = sx; per->sy = sy; per->z = z; per->a = a;
        per->picnum = picnum;
        per->dashade = dashade; per->dapalnum = dapalnum;
        per->dastat = dastat;
        per->pagesleft = numpages+((beforedrawrooms&1)<<7);
        per->cx1 = cx1; per->cy1 = cy1; per->cx2 = cx2; per->cy2 = cy2;
        per->uniqid = guniqhudid;   //JF extension

        //Would be better to optimize out true bounding boxes
        if (dastat&64)  //If non-masking write, checking for overlapping cases
        {
            for (i=permtail; i!=permhead; i=((i+1)&(MAXPERMS-1)))
            {
                per2 = &permfifo[i];
                if ((per2->pagesleft&127) == 0) continue;
                if (per2->sx != per->sx) continue;
                if (per2->sy != per->sy) continue;
                if (per2->z != per->z) continue;
                if (per2->a != per->a) continue;
                if (tilesizx[per2->picnum] > tilesizx[per->picnum]) continue;
                if (tilesizy[per2->picnum] > tilesizy[per->picnum]) continue;
                if (per2->cx1 < per->cx1) continue;
                if (per2->cy1 < per->cy1) continue;
                if (per2->cx2 > per->cx2) continue;
                if (per2->cy2 > per->cy2) continue;
                per2->pagesleft = 0;
            }
            if ((per->z == 65536) && (per->a == 0))
                for (i=permtail; i!=permhead; i=((i+1)&(MAXPERMS-1)))
                {
                    per2 = &permfifo[i];
                    if ((per2->pagesleft&127) == 0) continue;
                    if (per2->z != 65536) continue;
                    if (per2->a != 0) continue;
                    if (per2->cx1 < per->cx1) continue;
                    if (per2->cy1 < per->cy1) continue;
                    if (per2->cx2 > per->cx2) continue;
                    if (per2->cy2 > per->cy2) continue;
                    if ((per2->sx>>16) < (per->sx>>16)) continue;
                    if ((per2->sy>>16) < (per->sy>>16)) continue;
                    if ((per2->sx>>16)+tilesizx[per2->picnum] > (per->sx>>16)+tilesizx[per->picnum]) continue;
                    if ((per2->sy>>16)+tilesizy[per2->picnum] > (per->sy>>16)+tilesizy[per->picnum]) continue;
                    per2->pagesleft = 0;
                }
        }

        permhead = ((permhead+1)&(MAXPERMS-1));
    }
}


//
// makepalookup
//
void makepalookup(int32_t palnum, char *remapbuf, int8_t r, int8_t g, int8_t b, char dastat)
{
    int32_t i, j, palscale;
    char *ptr, *ptr2;

    if (paletteloaded == 0) return;

    if (palookup[palnum] == NULL)
    {
        //Allocate palookup buffer
        if ((palookup[palnum] = (char *)kkmalloc(numpalookups<<8)) == NULL)
            allocache((intptr_t*)&palookup[palnum],numpalookups<<8,&permanentlock);
    }

    if (dastat == 0) return;
    if ((r|g|b|63) != 63) return;

    if ((r|g|b) == 0)
    {
        for (i=0; i<256; i++)
        {
            ptr = (char *)(FP_OFF(palookup[0])+remapbuf[i]);
            ptr2 = (char *)(FP_OFF(palookup[palnum])+i);
            for (j=0; j<numpalookups; j++)
                { *ptr2 = *ptr; ptr += 256; ptr2 += 256; }
        }
#if defined(USE_OPENGL)
        palookupfog[palnum].r = 0;
        palookupfog[palnum].g = 0;
        palookupfog[palnum].b = 0;
#endif
    }
    else
    {
        ptr2 = palookup[palnum];
        for (i=0; i<numpalookups; i++)
        {
            palscale = divscale16(i,numpalookups);
            for (j=0; j<256; j++)
            {
                ptr = (char *)&palette[remapbuf[j]*3];
                *ptr2++ = getclosestcol((int32_t)ptr[0]+mulscale16(r-ptr[0],palscale),
                                        (int32_t)ptr[1]+mulscale16(g-ptr[1],palscale),
                                        (int32_t)ptr[2]+mulscale16(b-ptr[2],palscale));
            }
        }
#if defined(USE_OPENGL)
        palookupfog[palnum].r = r;
        palookupfog[palnum].g = g;
        palookupfog[palnum].b = b;
#endif
    }
}


void setvgapalette(void)
{
    int32_t i;

    for (i=0; i<256; i++)
    {
        curpalettefaded[i].b = curpalette[i].b = vgapal16[4*i] << 2;
        curpalettefaded[i].g = curpalette[i].g = vgapal16[4*i+1] << 2;
        curpalettefaded[i].r = curpalette[i].r = vgapal16[4*i+2] << 2;
    }
    setpalette(0,256);
}

//
// setbrightness
//
void setbrightness(char dabrightness, uint8_t *dapal, char noapply)
{
    int32_t i, k, j;
//    uint32_t lastbright = curbrightness;

    if (!(noapply&4))
    {
        curbrightness = min(max((int32_t)dabrightness,0),15);
//        if (lastbright != (unsigned)curbrightness)
//            vid_gamma = 1.0 + ((float)curbrightness / 10.0);
    }

    if (setgamma()) j = curbrightness; else j = 0;

    for (k=i=0; i<256; i++)
    {
        // save palette without any brightness adjustment
        curpalette[i].r = dapal[i*3+0] << 2;
        curpalette[i].g = dapal[i*3+1] << 2;
        curpalette[i].b = dapal[i*3+2] << 2;
        curpalette[i].f = 0;

        // brightness adjust the palette
        curpalettefaded[i].b = britable[j][ curpalette[i].b ];
        curpalettefaded[i].g = britable[j][ curpalette[i].g ];
        curpalettefaded[i].r = britable[j][ curpalette[i].r ];
        curpalettefaded[i].f = 0;
    }

    if ((noapply&1) == 0) setpalette(0,256);

#if defined(POLYMOST) && defined(USE_OPENGL)
    if (rendmode >= 3)
    {
        static uint32_t lastpalettesum = 0;
        uint32_t newpalettesum = crc32once((uint8_t *)curpalettefaded, sizeof(curpalettefaded));

        // only reset the textures if the preserve flag (bit 1 of noapply) is clear and
        // either (a) the new palette is different to the last, or (b) the brightness
        // changed and we couldn't set it using hardware gamma
        if (!(noapply&2) && (newpalettesum != lastpalettesum))
            gltexinvalidateall();
        if (!(noapply&8) && (newpalettesum != lastpalettesum))
            gltexinvalidate8();
        lastpalettesum = newpalettesum;
    }
#endif

    palfadergb.r = palfadergb.g = palfadergb.b = 0;
    palfadedelta = 0;
}


//
// setpalettefade
//
void setpalettefade(char r, char g, char b, char offset)
{
    int32_t i,k;
    palette_t p;

    palfadergb.r = min(63,r) << 2;
    palfadergb.g = min(63,g) << 2;
    palfadergb.b = min(63,b) << 2;
    palfadedelta = min(63,offset) << 2;

    k = 0;
    for (i=0; i<256; i++)
    {
        if (gammabrightness) p = curpalette[i];
        else
        {
            p.b = britable[curbrightness][ curpalette[i].b ];
            p.g	= britable[curbrightness][ curpalette[i].g ];
            p.r = britable[curbrightness][ curpalette[i].r ];
        }

        curpalettefaded[i].b =
            p.b + (((palfadergb.b - p.b) * offset) >> 6);
        curpalettefaded[i].g =
            p.g + (((palfadergb.g - p.g) * offset) >> 6);
        curpalettefaded[i].r =
            p.r + (((palfadergb.r - p.r) * offset) >> 6);
        curpalettefaded[i].f = 0;
    }

    setpalette(0,256);
}


//
// clearview
//
void clearview(int32_t dacol)
{
    intptr_t p;
    int32_t  y, dx;

    if (qsetmode != 200) return;

#if defined(POLYMOST) && defined(USE_OPENGL)
    if (rendmode >= 3)
    {
        palette_t p;
        if (gammabrightness) p = curpalette[dacol];
        else
        {
            p.r = britable[curbrightness][ curpalette[dacol].r ];
            p.g = britable[curbrightness][ curpalette[dacol].g ];
            p.b = britable[curbrightness][ curpalette[dacol].b ];
        }
        bglClearColor(((float)p.r)/255.0,
                      ((float)p.g)/255.0,
                      ((float)p.b)/255.0,
                      0);
        bglClear(GL_COLOR_BUFFER_BIT);
        return;
    }
#endif

    begindrawing(); //{{{
    dx = windowx2-windowx1+1;
    //dacol += (dacol<<8); dacol += (dacol<<16);
    p = frameplace+ylookup[windowy1]+windowx1;
    for (y=windowy1; y<=windowy2; y++)
    {
        //clearbufbyte((void*)p,dx,dacol);
        Bmemset((void*)p,dacol,dx);
        p += ylookup[1];
    }
    enddrawing();   //}}}

    faketimerhandler();
}


//
// clearallviews
//
void clearallviews(int32_t dacol)
{
    if (qsetmode != 200) return;
    //dacol += (dacol<<8); dacol += (dacol<<16);

#if defined(POLYMOST) && defined(USE_OPENGL)
    if (rendmode >= 3)
    {
        palette_t p;
        if (gammabrightness) p = curpalette[dacol];
        else
        {
            p.r = britable[curbrightness][ curpalette[dacol].r ];
            p.g = britable[curbrightness][ curpalette[dacol].g ];
            p.b = britable[curbrightness][ curpalette[dacol].b ];
        }
        bglViewport(0,0,xdim,ydim); glox1 = -1;
        bglClearColor(((float)p.r)/255.0,
                      ((float)p.g)/255.0,
                      ((float)p.b)/255.0,
                      0);
        bglClear(GL_COLOR_BUFFER_BIT);
        return;
    }
#endif

    begindrawing(); //{{{
    //clearbufbyte((void*)frameplace,imageSize,0L);
    Bmemset((void*)frameplace,dacol,imageSize);
    enddrawing();   //}}}
    //nextpage();

    faketimerhandler();
}


//
// plotpixel
//
void plotpixel(int32_t x, int32_t y, char col)
{
#if defined(POLYMOST) && defined(USE_OPENGL)
    if (rendmode >= 3 && qsetmode == 200)
    {
        palette_t p;
        if (gammabrightness) p = curpalette[col];
        else
        {
            p.r = britable[curbrightness][ curpalette[col].r ];
            p.g = britable[curbrightness][ curpalette[col].g ];
            p.b = britable[curbrightness][ curpalette[col].b ];
        }

        setpolymost2dview();	// JBF 20040205: more efficient setup

//         bglBegin(GL_POINTS);
//         bglColor4ub(p.r,p.g,p.b,255);
//         bglVertex2i(x,y);
//         bglEnd();
        bglRasterPos4i(x, y, 0, 1);
        bglDrawPixels(1, 1, GL_RGB, GL_UNSIGNED_BYTE, &p);
        bglRasterPos4i(0, 0, 0, 1);

        return;
    }
#endif

    begindrawing(); //{{{
    drawpixel((void*)(ylookup[y]+x+frameplace),(int32_t)col);
    enddrawing();   //}}}
}


//
// getpixel
//
char getpixel(int32_t x, int32_t y)
{
    char r;

#if defined(POLYMOST) && defined(USE_OPENGL)
    if (rendmode >= 3 && qsetmode == 200) return 0;
#endif

    begindrawing(); //{{{
    r = readpixel((void*)(ylookup[y]+x+frameplace));
    enddrawing();   //}}}
    return(r);
}


//MUST USE RESTOREFORDRAWROOMS AFTER DRAWING

//
// setviewtotile
//
void setviewtotile(int16_t tilenume, int32_t xsiz, int32_t ysiz)
{
    int32_t i, j;

    //DRAWROOMS TO TILE BACKUP&SET CODE
    tilesizx[tilenume] = xsiz; tilesizy[tilenume] = ysiz;
    bakxsiz[setviewcnt] = xsiz; bakysiz[setviewcnt] = ysiz;
    bakframeplace[setviewcnt] = frameplace; frameplace = waloff[tilenume];
    bakwindowx1[setviewcnt] = windowx1; bakwindowy1[setviewcnt] = windowy1;
    bakwindowx2[setviewcnt] = windowx2; bakwindowy2[setviewcnt] = windowy2;
#ifdef POLYMOST
    if (setviewcnt == 0)
    {
        bakrendmode = rendmode;
        baktile = tilenume;
    }
    rendmode = 0;//2;
#endif
    copybufbyte(&startumost[windowx1],&bakumost[windowx1],(windowx2-windowx1+1)*sizeof(bakumost[0]));
    copybufbyte(&startdmost[windowx1],&bakdmost[windowx1],(windowx2-windowx1+1)*sizeof(bakdmost[0]));
    setviewcnt++;

    offscreenrendering = 1;
    setview(0,0,ysiz-1,xsiz-1);
    setaspect(65536,65536);
    j = 0; for (i=0; i<=xsiz; i++) { ylookup[i] = j, j += ysiz; }
    setvlinebpl(ysiz);
}


//
// setviewback
//
extern char modechange;
void setviewback(void)
{
    int32_t i, j, k;

    if (setviewcnt <= 0) return;
    setviewcnt--;

    offscreenrendering = (setviewcnt>0);
#ifdef POLYMOST
    if (setviewcnt == 0)
    {
        rendmode = bakrendmode;
        invalidatetile(baktile,-1,-1);
    }
#endif

    setview(bakwindowx1[setviewcnt],bakwindowy1[setviewcnt],
            bakwindowx2[setviewcnt],bakwindowy2[setviewcnt]);
    copybufbyte(&bakumost[windowx1],&startumost[windowx1],(windowx2-windowx1+1)*sizeof(startumost[0]));
    copybufbyte(&bakdmost[windowx1],&startdmost[windowx1],(windowx2-windowx1+1)*sizeof(startdmost[0]));
    frameplace = bakframeplace[setviewcnt];
    if (setviewcnt == 0)
        k = bakxsiz[0];
    else
        k = max(bakxsiz[setviewcnt-1],bakxsiz[setviewcnt]);
    j = 0; for (i=0; i<=k; i++) ylookup[i] = j, j += bytesperline;
    setvlinebpl(bytesperline);
    modechange=1;
}


//
// squarerotatetile
//
void squarerotatetile(int16_t tilenume)
{
    int32_t i, j, k, xsiz, ysiz;
    char *ptr1, *ptr2;

    xsiz = tilesizx[tilenume]; ysiz = tilesizy[tilenume];

    //supports square tiles only for rotation part
    if (xsiz == ysiz)
    {
        k = (xsiz<<1);
        for (i=xsiz-1; i>=0; i--)
        {
            ptr1 = (char *)(waloff[tilenume]+i*(xsiz+1)); ptr2 = ptr1;
            if ((i&1) != 0) { ptr1--; ptr2 -= xsiz; swapchar(ptr1,ptr2); }
            for (j=(i>>1)-1; j>=0; j--)
                { ptr1 -= 2; ptr2 -= k; swapchar2(ptr1,ptr2,xsiz); }
        }
    }
}


//
// preparemirror
//
void preparemirror(int32_t dax, int32_t day, int32_t daz, int16_t daang, int32_t dahoriz, int16_t dawall, int16_t dasector, int32_t *tposx, int32_t *tposy, int16_t *tang)
{
    int32_t i, j, x, y, dx, dy;

    UNREFERENCED_PARAMETER(daz);
    UNREFERENCED_PARAMETER(dahoriz);
    UNREFERENCED_PARAMETER(dasector);

    x = wall[dawall].x; dx = wall[wall[dawall].point2].x-x;
    y = wall[dawall].y; dy = wall[wall[dawall].point2].y-y;
    j = dx*dx + dy*dy; if (j == 0) return;
    i = (((dax-x)*dx + (day-y)*dy)<<1);
    *tposx = (x<<1) + scale(dx,i,j) - dax;
    *tposy = (y<<1) + scale(dy,i,j) - day;
    *tang = (((getangle(dx,dy)<<1)-daang)&2047);

    inpreparemirror = 1;
}


//
// completemirror
//
void completemirror(void)
{
    int32_t i, dy;
    intptr_t p;

#ifdef POLYMOST
    if (rendmode) return;
#endif

    //Can't reverse with uninitialized data
    if (inpreparemirror) { inpreparemirror = 0; return; }
    if (mirrorsx1 > 0) mirrorsx1--;
    if (mirrorsx2 < windowx2-windowx1-1) mirrorsx2++;
    if (mirrorsx2 < mirrorsx1) return;

    begindrawing();
    p = frameplace+ylookup[windowy1+mirrorsy1]+windowx1+mirrorsx1;
    i = windowx2-windowx1-mirrorsx2-mirrorsx1; mirrorsx2 -= mirrorsx1;
    for (dy=mirrorsy2-mirrorsy1-1; dy>=0; dy--)
    {
        copybufbyte((void*)(p+1),tempbuf,mirrorsx2+1);
        tempbuf[mirrorsx2] = tempbuf[mirrorsx2-1];
        copybufreverse(&tempbuf[mirrorsx2],(void*)(p+i),mirrorsx2+1);
        p += ylookup[1];
        faketimerhandler();
    }
    enddrawing();
}


//
// sectorofwall
//
int32_t sectorofwall(int16_t theline)
{
    int32_t i, gap;

    if ((theline < 0) || (theline >= numwalls)) return(-1);
    i = wall[theline].nextwall; if (i >= 0) return(wall[i].nextsector);

    gap = (numsectors>>1); i = gap;
    while (gap > 1)
    {
        gap >>= 1;
        if (sector[i].wallptr < theline) i += gap; else i -= gap;
    }
    while (sector[i].wallptr > theline) i--;
    while (sector[i].wallptr+sector[i].wallnum <= theline) i++;
    return(i);
}


//
// getceilzofslope
//
int32_t getceilzofslope(int16_t sectnum, int32_t dax, int32_t day)
{
    int32_t dx, dy, i, j;
    walltype *wal;

    if (!(sector[sectnum].ceilingstat&2)) return(sector[sectnum].ceilingz);
    wal = &wall[sector[sectnum].wallptr];
    dx = wall[wal->point2].x-wal->x; dy = wall[wal->point2].y-wal->y;
    i = (nsqrtasm(dx*dx+dy*dy)<<5); if (i == 0) return(sector[sectnum].ceilingz);
    j = dmulscale3(dx,day-wal->y,-dy,dax-wal->x);
    return(sector[sectnum].ceilingz+(scale(sector[sectnum].ceilingheinum,j>>1,i)<<1));
}


//
// getflorzofslope
//
int32_t getflorzofslope(int16_t sectnum, int32_t dax, int32_t day)
{
    int32_t dx, dy, i, j;
    walltype *wal;

    if (!(sector[sectnum].floorstat&2)) return(sector[sectnum].floorz);
    wal = &wall[sector[sectnum].wallptr];
    dx = wall[wal->point2].x-wal->x; dy = wall[wal->point2].y-wal->y;
    i = (nsqrtasm(dx*dx+dy*dy)<<5); if (i == 0) return(sector[sectnum].floorz);
    j = dmulscale3(dx,day-wal->y,-dy,dax-wal->x);
    return(sector[sectnum].floorz+(scale(sector[sectnum].floorheinum,j>>1,i)<<1));
}


//
// getzsofslope
//
void getzsofslope(int16_t sectnum, int32_t dax, int32_t day, int32_t *ceilz, int32_t *florz)
{
    int32_t dx, dy, i, j;
    walltype *wal, *wal2;
    sectortype *sec;

    sec = &sector[sectnum];
    *ceilz = sec->ceilingz; *florz = sec->floorz;
    if ((sec->ceilingstat|sec->floorstat)&2)
    {
        wal = &wall[sec->wallptr]; wal2 = &wall[wal->point2];
        dx = wal2->x-wal->x; dy = wal2->y-wal->y;
        i = (nsqrtasm(dx*dx+dy*dy)<<5); if (i == 0) return;
        j = dmulscale3(dx,day-wal->y,-dy,dax-wal->x);
        if (sec->ceilingstat&2) *ceilz = (*ceilz)+(scale(sec->ceilingheinum,j>>1,i)<<1);
        if (sec->floorstat&2) *florz = (*florz)+(scale(sec->floorheinum,j>>1,i)<<1);
    }
}


//
// alignceilslope
//
void alignceilslope(int16_t dasect, int32_t x, int32_t y, int32_t z)
{
    int32_t i, dax, day;
    walltype *wal;

    wal = &wall[sector[dasect].wallptr];
    dax = wall[wal->point2].x-wal->x;
    day = wall[wal->point2].y-wal->y;

    i = (y-wal->y)*dax - (x-wal->x)*day; if (i == 0) return;
    sector[dasect].ceilingheinum = scale((z-sector[dasect].ceilingz)<<8,
                                         nsqrtasm(dax*dax+day*day),i);

    if (sector[dasect].ceilingheinum == 0) sector[dasect].ceilingstat &= ~2;
    else sector[dasect].ceilingstat |= 2;
}


//
// alignflorslope
//
void alignflorslope(int16_t dasect, int32_t x, int32_t y, int32_t z)
{
    int32_t i, dax, day;
    walltype *wal;

    wal = &wall[sector[dasect].wallptr];
    dax = wall[wal->point2].x-wal->x;
    day = wall[wal->point2].y-wal->y;

    i = (y-wal->y)*dax - (x-wal->x)*day; if (i == 0) return;
    sector[dasect].floorheinum = scale((z-sector[dasect].floorz)<<8,
                                       nsqrtasm(dax*dax+day*day),i);

    if (sector[dasect].floorheinum == 0) sector[dasect].floorstat &= ~2;
    else sector[dasect].floorstat |= 2;
}


//
// loopnumofsector
//
int32_t loopnumofsector(int16_t sectnum, int16_t wallnum)
{
    int32_t i, numloops, startwall, endwall;

    numloops = 0;
    startwall = sector[sectnum].wallptr;
    endwall = startwall + sector[sectnum].wallnum;
    for (i=startwall; i<endwall; i++)
    {
        if (i == wallnum) return(numloops);
        if (wall[i].point2 < i) numloops++;
    }
    return(-1);
}


//
// setfirstwall
//
void setfirstwall(int16_t sectnum, int16_t newfirstwall)
{
    int32_t i, j, k, numwallsofloop;
    int32_t startwall, endwall, danumwalls, dagoalloop;

    startwall = sector[sectnum].wallptr;
    danumwalls = sector[sectnum].wallnum;
    endwall = startwall+danumwalls;
    if ((newfirstwall < startwall) || (newfirstwall >= startwall+danumwalls)) return;
    for (i=0; i<danumwalls; i++)
        Bmemcpy(&wall[i+numwalls],&wall[i+startwall],sizeof(walltype));

    numwallsofloop = 0;
    i = newfirstwall;
    do
    {
        numwallsofloop++;
        i = wall[i].point2;
    }
    while (i != newfirstwall);

    //Put correct loop at beginning
    dagoalloop = loopnumofsector(sectnum,newfirstwall);
    if (dagoalloop > 0)
    {
        j = 0;
        while (loopnumofsector(sectnum,j+startwall) != dagoalloop) j++;
        for (i=0; i<danumwalls; i++)
        {
            k = i+j; if (k >= danumwalls) k -= danumwalls;
            Bmemcpy(&wall[startwall+i],&wall[numwalls+k],sizeof(walltype));

            wall[startwall+i].point2 += danumwalls-startwall-j;
            if (wall[startwall+i].point2 >= danumwalls)
                wall[startwall+i].point2 -= danumwalls;
            wall[startwall+i].point2 += startwall;
        }
        newfirstwall += danumwalls-j;
        if (newfirstwall >= startwall+danumwalls) newfirstwall -= danumwalls;
    }

    for (i=0; i<numwallsofloop; i++)
        Bmemcpy(&wall[i+numwalls],&wall[i+startwall],sizeof(walltype));
    for (i=0; i<numwallsofloop; i++)
    {
        k = i+newfirstwall-startwall;
        if (k >= numwallsofloop) k -= numwallsofloop;
        Bmemcpy(&wall[startwall+i],&wall[numwalls+k],sizeof(walltype));

        wall[startwall+i].point2 += numwallsofloop-newfirstwall;
        if (wall[startwall+i].point2 >= numwallsofloop)
            wall[startwall+i].point2 -= numwallsofloop;
        wall[startwall+i].point2 += startwall;
    }

    for (i=startwall; i<endwall; i++)
        if (wall[i].nextwall >= 0) wall[wall[i].nextwall].nextwall = i;
}


//
// drawline256
//
void drawline256(int32_t x1, int32_t y1, int32_t x2, int32_t y2, char col)
{
    int32_t dx, dy, i, j, inc, plc, daend;
    intptr_t p;

    col = palookup[0][col];

#if defined(POLYMOST) && defined(USE_OPENGL)
    if (rendmode >= 3)
    {
        palette_t p;
        if (gammabrightness) p = curpalette[col];
        else
        {
            p.r = britable[curbrightness][ curpalette[col].r ];
            p.g = britable[curbrightness][ curpalette[col].g ];
            p.b = britable[curbrightness][ curpalette[col].b ];
        }

        setpolymost2dview();	// JBF 20040205: more efficient setup

        //bglEnable(GL_BLEND);	// When using line antialiasing, this is needed
        bglBegin(GL_LINES);
        bglColor4ub(p.r,p.g,p.b,255);
        bglVertex2f((float)x1/4096.0,(float)y1/4096.0);
        bglVertex2f((float)x2/4096.0,(float)y2/4096.0);
        bglEnd();
        //bglDisable(GL_BLEND);

        return;
    }
#endif

    dx = x2-x1; dy = y2-y1;
    if (dx >= 0)
    {
        if ((x1 >= wx2) || (x2 < wx1)) return;
        if (x1 < wx1) y1 += scale(wx1-x1,dy,dx), x1 = wx1;
        if (x2 > wx2) y2 += scale(wx2-x2,dy,dx), x2 = wx2;
    }
    else
    {
        if ((x2 >= wx2) || (x1 < wx1)) return;
        if (x2 < wx1) y2 += scale(wx1-x2,dy,dx), x2 = wx1;
        if (x1 > wx2) y1 += scale(wx2-x1,dy,dx), x1 = wx2;
    }
    if (dy >= 0)
    {
        if ((y1 >= wy2) || (y2 < wy1)) return;
        if (y1 < wy1) x1 += scale(wy1-y1,dx,dy), y1 = wy1;
        if (y2 > wy2) x2 += scale(wy2-y2,dx,dy), y2 = wy2;
    }
    else
    {
        if ((y2 >= wy2) || (y1 < wy1)) return;
        if (y2 < wy1) x2 += scale(wy1-y2,dx,dy), y2 = wy1;
        if (y1 > wy2) x1 += scale(wy2-y1,dx,dy), y1 = wy2;
    }

    if (klabs(dx) >= klabs(dy))
    {
        if (dx == 0) return;
        if (dx < 0)
        {
            i = x1; x1 = x2; x2 = i;
            i = y1; y1 = y2; y2 = i;
        }

        inc = divscale12(dy,dx);
        plc = y1+mulscale12((2047-x1)&4095,inc);
        i = ((x1+2048)>>12); daend = ((x2+2048)>>12);

        begindrawing(); //{{{
        for (; i<daend; i++)
        {
            j = (plc>>12);
            if ((j >= startumost[i]) && (j < startdmost[i]))
                drawpixel((void*)(frameplace+ylookup[j]+i),col);
            plc += inc;
        }
        enddrawing();   //}}}
    }
    else
    {
        if (dy < 0)
        {
            i = x1; x1 = x2; x2 = i;
            i = y1; y1 = y2; y2 = i;
        }

        inc = divscale12(dx,dy);
        plc = x1+mulscale12((2047-y1)&4095,inc);
        i = ((y1+2048)>>12); daend = ((y2+2048)>>12);

        begindrawing(); //{{{
        p = ylookup[i]+frameplace;
        for (; i<daend; i++)
        {
            j = (plc>>12);
            if ((i >= startumost[j]) && (i < startdmost[j]))
                drawpixel((void*)(j+p),col);
            plc += inc; p += ylookup[1];
        }
        enddrawing();   //}}}
    }
}

//
// drawline16
//
// JBF: Had to add extra tests to make sure x-coordinates weren't winding up -'ve
//   after clipping or crashes would ensue
uint32_t drawlinepat = 0xffffffff;

void drawline16(int32_t x1, int32_t y1, int32_t x2, int32_t y2, char col)
{
    int32_t i, dx, dy, pinc, d;
    uint32_t patc=0;
    intptr_t p;

    dx = x2-x1; dy = y2-y1;
    if (dx >= 0)
    {
        if ((x1 >= xres) || (x2 < 0)) return;
        if (x1 < 0) { if (dy) y1 += scale(0-x1,dy,dx); x1 = 0; }
        if (x2 >= xres) { if (dy) y2 += scale(xres-1-x2,dy,dx); x2 = xres-1; }
    }
    else
    {
        if ((x2 >= xres) || (x1 < 0)) return;
        if (x2 < 0) { if (dy) y2 += scale(0-x2,dy,dx); x2 = 0; }
        if (x1 >= xres) { if (dy) y1 += scale(xres-1-x1,dy,dx); x1 = xres-1; }
    }
    if (dy >= 0)
    {
        if ((y1 >= ydim16) || (y2 < 0)) return;
        if (y1 < 0) { if (dx) x1 += scale(0-y1,dx,dy); y1 = 0; if (x1 < 0) x1 = 0; }
        if (y2 >= ydim16) { if (dx) x2 += scale(ydim16-1-y2,dx,dy); y2 = ydim16-1; if (x2 < 0) x2 = 0; }
    }
    else
    {
        if ((y2 >= ydim16) || (y1 < 0)) return;
        if (y2 < 0) { if (dx) x2 += scale(0-y2,dx,dy); y2 = 0; if (x2 < 0) x2 = 0; }
        if (y1 >= ydim16) { if (dx) x1 += scale(ydim16-1-y1,dx,dy); y1 = ydim16-1; if (x1 < 0) x1 = 0; }
    }

    dx = klabs(x2-x1)+1; dy = klabs(y2-y1)+1;
    if (dx >= dy)
    {
        if (x2 < x1)
        {
            i = x1; x1 = x2; x2 = i;
            i = y1; y1 = y2; y2 = i;
        }
        d = 0;
        if (y2 > y1) pinc = bytesperline; else pinc = -bytesperline;

        begindrawing(); //{{{
        p = (y1*bytesperline)+x1+frameplace;
        if (dy == 0 && drawlinepat == 0xffffffff)
        {
            i = ((int32_t)col<<24)|((int32_t)col<<16)|((int32_t)col<<8)|col;
            clearbufbyte((void *)p, dx, i);
        }
        else
            for (i=dx; i>0; i--)
            {
                if (drawlinepat & pow2long[(patc++)&31])
                    drawpixel((char *)p, col);
                d += dy;
                if (d >= dx) { d -= dx; p += pinc; }
                p++;
            }
        enddrawing();   //}}}
        return;
    }

    if (y2 < y1)
    {
        i = x1; x1 = x2; x2 = i;
        i = y1; y1 = y2; y2 = i;
    }
    d = 0;
    if (x2 > x1) pinc = 1; else pinc = -1;

    begindrawing(); //{{{
    p = (y1*bytesperline)+x1+frameplace;
    for (i=dy; i>0; i--)
    {
        if (drawlinepat & pow2long[(patc++)&31])
            drawpixel((char *)p, col);
        d += dx;
        if (d >= dy) { d -= dy; p += pinc; }
        p += bytesperline;
    }
    enddrawing();   //}}}
}

void drawcircle16(int32_t x1, int32_t y1, int32_t r, char col)
{
#if 1
    intptr_t p;
    int32_t xp, yp, xpbpl, ypbpl, d, de, dse, patc=0;

    if (r < 0) r = -r;
    if (x1+r < 0 || x1-r >= xres) return;
    if (y1+r < 0 || y1-r >= ydim16) return;

    /*
     *      d
     *    6 | 7
     *   \  |  /
     *  5  \|/  8
     * c----+----a
     *  4  /|\  1
     *   /  |  \
     *    3 | 2
     *      b
     */

    xp = 0;
    yp = r;
    d = 1 - r;
    de = 2;
    dse = 5 - (r << 1);

    begindrawing();
    p = (y1*bytesperline)+x1+frameplace;

    if (drawlinepat & pow2long[(patc++)&31])
    {
        if ((uint32_t)y1 < (uint32_t)ydim16 && (uint32_t)(x1+r) < (uint32_t)xres)
            drawpixel((char *)(p+r), col);          // a
        if ((uint32_t)x1 < (uint32_t)xres   && (uint32_t)(y1+r) < (uint32_t)ydim16)
            drawpixel((char *)(p+(r*bytesperline)), col);   // b
        if ((uint32_t)y1 < (uint32_t)ydim16 && (uint32_t)(x1-r) < (uint32_t)xres)
            drawpixel((char *)(p-r), col);          // c
        if ((uint32_t)x1 < (uint32_t)xres   && (uint32_t)(y1-r) < (uint32_t)ydim16)
            drawpixel((char *)(p-(r*bytesperline)), col);   // d
    }

    do
    {
        if (d < 0)
        {
            d += de;
            de += 2;
            dse += 2;
            xp++;
        }
        else
        {
            d += dse;
            de += 2;
            dse += 4;
            xp++;
            yp--;
        }

        ypbpl = yp*bytesperline;
        xpbpl = xp*bytesperline;
        if (drawlinepat & pow2long[(patc++)&31])
        {
            if ((uint32_t)(x1+yp) < (uint32_t)xres && (uint32_t)(y1+xp) < (uint32_t)ydim16)
                drawpixel((char *)(p+yp+xpbpl), col);   // 1
            if ((uint32_t)(x1+xp) < (uint32_t)xres && (uint32_t)(y1+yp) < (uint32_t)ydim16)
                drawpixel((char *)(p+xp+ypbpl), col);   // 2
            if ((uint32_t)(x1-xp) < (uint32_t)xres && (uint32_t)(y1+yp) < (uint32_t)ydim16)
                drawpixel((char *)(p-xp+ypbpl), col);   // 3
            if ((uint32_t)(x1-yp) < (uint32_t)xres && (uint32_t)(y1+xp) < (uint32_t)ydim16)
                drawpixel((char *)(p-yp+xpbpl), col);   // 4
            if ((uint32_t)(x1-yp) < (uint32_t)xres && (uint32_t)(y1-xp) < (uint32_t)ydim16)
                drawpixel((char *)(p-yp-xpbpl), col);   // 5
            if ((uint32_t)(x1-xp) < (uint32_t)xres && (uint32_t)(y1-yp) < (uint32_t)ydim16)
                drawpixel((char *)(p-xp-ypbpl), col);   // 6
            if ((uint32_t)(x1+xp) < (uint32_t)xres && (uint32_t)(y1-yp) < (uint32_t)ydim16)
                drawpixel((char *)(p+xp-ypbpl), col);   // 7
            if ((uint32_t)(x1+yp) < (uint32_t)xres && (uint32_t)(y1-xp) < (uint32_t)ydim16)
                drawpixel((char *)(p+yp-xpbpl), col);   // 8
        }
    }
    while (yp > xp);
    enddrawing();
#else
    // JonoF's rough approximation of a circle
    int32_t l,spx,spy,lpx,lpy,px,py;

    spx = lpx = x1+mulscale14(r,sintable[0]);
    spy = lpy = y1+mulscale14(r,sintable[512]);

    for (l=64; l<2048; l+=64)
    {
        px = x1+mulscale14(r,sintable[l]);
        py = y1+mulscale14(r,sintable[(l+512)&2047]);

        drawline16(lpx,lpy,px,py,col);

        lpx = px;
        lpy = py;
    }

    drawline16(lpx,lpy,spx,spy,col);
#endif
}


//
// qsetmode640350
//
void qsetmode640350(void)
{
    if (qsetmode != 350)
    {
        if (setvideomode(640, 350, 8, fullscreen) < 0)
        {
            //fprintf(stderr, "Couldn't set 640x350 video mode for some reason.\n");
            return;
        }

        xdim = xres;
        ydim = yres;

//        setvgapalette();

        ydim16 = 350;
        halfxdim16 = 320;
        midydim16 = 146;

        begindrawing(); //{{{
        clearbuf((char *)frameplace, (bytesperline*350L) >> 2, 0);
        enddrawing();   //}}}
    }

    qsetmode = 350;
}


//
// qsetmode640480
//
void qsetmode640480(void)
{
    if (qsetmode != 480)
    {
        if (setvideomode(640, 480, 8, fullscreen) < 0)
        {
            //fprintf(stderr, "Couldn't set 640x480 video mode for some reason.\n");
            return;
        }

        xdim = xres;
        ydim = yres;

//        setvgapalette();

        ydim16 = 336;
        halfxdim16 = 320;
        midydim16 = 200;

        begindrawing(); //{{{
        clearbuf((char *)(frameplace + (336l*bytesperline)), (bytesperline*144L) >> 2, 0x08080808l);
        clearbuf((char *)frameplace, (bytesperline*336L) >> 2, 0L);
        enddrawing();   //}}}
    }

    qsetmode = 480;
}


//
// qsetmodeany
//
void qsetmodeany(int32_t daxdim, int32_t daydim)
{
    if (daxdim < 640) daxdim = 640;
    if (daydim < 480) daydim = 480;

    if (qsetmode != ((daxdim<<16)|(daydim&0xffff)))
    {
        if (setvideomode(daxdim, daydim, 8, fullscreen) < 0)
            return;

        xdim = xres;
        ydim = yres;

//        setvgapalette();

        ydim16 = yres - STATUS2DSIZ2;
        halfxdim16 = xres >> 1;
        midydim16 = ydim16 >> 1; // scale(200,yres,480);

        begindrawing(); //{{{
        clearbuf((char *)(frameplace + (ydim16*bytesperline)), (bytesperline*STATUS2DSIZ2) >> 2, 0x08080808l);
        clearbuf((char *)frameplace, (ydim16*bytesperline) >> 2, 0L);
        enddrawing();   //}}}
    }

    qsetmode = ((daxdim<<16)|(daydim&0xffff));
}


//
// clear2dscreen
//
void clear2dscreen(void)
{
    int32_t clearsz;

    begindrawing(); //{{{
    if (qsetmode == 350) clearsz = 350;
    else
    {
        if (ydim16 <= yres-STATUS2DSIZ2) clearsz = yres - STATUS2DSIZ2;
        else clearsz = yres;
    }
    clearbuf((char *)frameplace, (bytesperline*clearsz) >> 2, 0);
    enddrawing();   //}}}
}

//
// draw2dgrid
//
void draw2dgrid(int32_t posxe, int32_t posye, int16_t ange, int32_t zoome, int16_t gride)
{
    int64 i, xp1, yp1, xp2=0, yp2, tempy;

    UNREFERENCED_PARAMETER(ange);

    if (gride > 0)
    {
        begindrawing();	//{{{

        yp1 = midydim16-mulscale14(posye+editorgridextent,zoome);
        if (yp1 < 0) yp1 = 0;
        yp2 = midydim16-mulscale14(posye-editorgridextent,zoome);
        if (yp2 >= ydim16) yp2 = ydim16-1;

        if ((yp1 < ydim16) && (yp2 >= 0) && (yp2 >= yp1))
        {
            xp1 = halfxdim16-mulscale14(posxe+editorgridextent,zoome);
            for (i=-editorgridextent; i<=editorgridextent; i+=(2048>>gride))
            {
                xp2 = xp1;
                xp1 = halfxdim16-mulscale14(posxe-i,zoome);
                if (xp1 >= xdim) break;
                if (xp1 >= 0)
                {
                    if (xp1 != xp2)
                    {
                        drawline16(xp1,yp1,xp1,yp2,editorcolors[8]);
                    }
                }
            }
            if ((i >= editorgridextent) && (xp1 < xdim))
                xp2 = xp1;
            if ((xp2 >= 0) && (xp2 < xdim))
            {
                drawline16(xp2,yp1,xp2,yp2,editorcolors[8]);
            }
        }
        xp1 = mulscale14(posxe+editorgridextent,zoome);
        xp2 = mulscale14(posxe-editorgridextent,zoome);
        tempy = 0x80000000l;
        for (i=-editorgridextent; i<=editorgridextent; i+=(2048>>gride))
        {
            yp1 = (((posye-i)*zoome)>>14);
            if (yp1 != tempy)
            {
                if ((yp1 > midydim16-ydim16) && (yp1 <= midydim16))
                {
                    drawline16(halfxdim16-xp1,midydim16-yp1,halfxdim16-xp2,midydim16-yp1,editorcolors[8]);
                    tempy = yp1;
                }
            }
        }
        enddrawing();   //}}}
    }
}


//
// draw2dscreen
//

char spritecol2d[MAXTILES][2];
int32_t showfirstwall=0;
int32_t showheightindicators=2;
int32_t circlewall=-1;

void draw2dscreen(int32_t posxe, int32_t posye, int16_t ange, int32_t zoome, int16_t gride)
{
    walltype *wal;
    int32_t i, j, xp1, yp1, xp2, yp2;
    intptr_t tempint;
    char col;

    if (qsetmode == 200) return;

    begindrawing(); //{{{

    if (editstatus == 0)
    {
        faketimerhandler();
        clear2dscreen();

        faketimerhandler();
        draw2dgrid(posxe,posye,ange,zoome,gride);
    }

    faketimerhandler();
    for (i=numwalls-1,wal=&wall[i]; i>=0; i--,wal--)
    {
        int64 dist,dx,dy;
        if (editstatus == 0)
        {
            if ((show2dwall[i>>3]&pow2char[i&7]) == 0) continue;
            j = wal->nextwall;
            if ((j >= 0) && (i > j))
                if ((show2dwall[j>>3]&pow2char[j&7]) > 0) continue;
        }
        else
        {
            j = wal->nextwall;
            if ((j >= 0) && (i > j)) continue;
        }

        if (j < 0)
        {
            col = 15;
            if (i == linehighlight) if (totalclock & 16) col -= (2<<2);
        }
        else
        {
            col = 33;
            if ((wal->cstat&1) != 0) col = 5;
            if (wal->nextwall!=-1&&((wal->cstat^wall[wal->nextwall].cstat)&1)) col = 2;
            if ((i == linehighlight) || ((linehighlight >= 0) && (i == wall[linehighlight].nextwall)))
                if (totalclock & 16) col += (2<<2);
        }
        if (showfirstwall && (sector[searchsector].wallptr==i||sector[searchsector].wallptr==wall[i].nextwall))
        {
            col = 14;
            if (i == linehighlight) if (totalclock & 16) col -= (2<<2);
        }

        if (circlewall >= 0 && (i == circlewall || wal->nextwall == circlewall))
            col = 14;

        xp1 = mulscale14(wal->x-posxe,zoome);
        yp1 = mulscale14(wal->y-posye,zoome);
        xp2 = mulscale14(wall[wal->point2].x-posxe,zoome);
        yp2 = mulscale14(wall[wal->point2].y-posye,zoome);

        dx=wal->x-wall[wal->point2].x;
        dy=wal->y-wall[wal->point2].y;
        dist=dx*dx+dy*dy;
        if (dist>0xffffffff)
        {
            col=9;
            if (i == linehighlight || ((linehighlight >= 0) && (i == wall[linehighlight].nextwall)))
                if (totalclock & 16) col -= (2<<2);
        }

        if ((wal->cstat&64) > 0)
        {
            if (klabs(xp2-xp1) >= klabs(yp2-yp1))
            {
                drawline16(halfxdim16+xp1,midydim16+yp1+1,halfxdim16+xp2,midydim16+yp2+1,editorcolors[col]);
                drawline16(halfxdim16+xp1,midydim16+yp1-1,halfxdim16+xp2,midydim16+yp2-1,editorcolors[col]);
            }
            else
            {
                drawline16(halfxdim16+xp1+1,midydim16+yp1,halfxdim16+xp2+1,midydim16+yp2,editorcolors[col]);
                drawline16(halfxdim16+xp1-1,midydim16+yp1,halfxdim16+xp2-1,midydim16+yp2,editorcolors[col]);
            }
            col += 8;
        }

        drawline16(halfxdim16+xp1,midydim16+yp1,halfxdim16+xp2,midydim16+yp2,editorcolors[col]);
        {
            int32_t k = getangle(xp1-xp2, yp1-yp2);
            int32_t dax = mulscale14(((wal->x+wall[wal->point2].x)>>1)-posxe,zoome);
            int32_t day = mulscale14(((wal->y+wall[wal->point2].y)>>1)-posye,zoome);

            if (wal->nextsector >= 0 && showheightindicators)
            {
                int32_t ii = sector[sectorofwall(i)].floorz;
                int32_t jj = sector[wal->nextsector].floorz;

                if (jj == ii && showheightindicators > 1)
                {
                    int32_t dax3 = mulscale11(sintable[(k+1024)&2047],zoome) / 2560;
                    int32_t day3 = mulscale11(sintable[(k+512)&2047],zoome) / 2560;
                    int32_t dax2 = mulscale11(sintable[(k+2048)&2047],zoome) / 2560;
                    int32_t day2 = mulscale11(sintable[(k+1536)&2047],zoome) / 2560;
                    drawline16(halfxdim16+dax+dax3,midydim16+day+day3,halfxdim16+dax+dax2,midydim16+day+day2,editorcolors[col]);
                }
                else if (jj > ii)
                {
                    int32_t dax2 = mulscale11(sintable[(k+1024)&2047],zoome) / 2560;
                    int32_t day2 = mulscale11(sintable[(k+512)&2047],zoome) / 2560;
                    drawline16(halfxdim16+dax,midydim16+day,halfxdim16+dax+dax2,midydim16+day+day2,editorcolors[col]);
                }
                else if (jj < ii)
                {
                    int32_t dax2 = mulscale11(sintable[(k+2048)&2047],zoome) / 2560;
                    int32_t day2 = mulscale11(sintable[(k+1536)&2047],zoome) / 2560;
                    drawline16(halfxdim16+dax,midydim16+day,halfxdim16+dax+dax2,midydim16+day+day2,editorcolors[col]);
                }
            }
            else if (showheightindicators > 1)
            {
                int32_t dax2 = mulscale11(sintable[(k+2048)&2047],zoome) / 2560;
                int32_t day2 = mulscale11(sintable[(k+1536)&2047],zoome) / 2560;
                drawline16(halfxdim16+dax,midydim16+day,halfxdim16+dax+dax2,midydim16+day+day2,editorcolors[col]);
            }
        }
        if ((zoome >= 256) && (editstatus == 1))
            if (((halfxdim16+xp1) >= 2) && ((halfxdim16+xp1) <= xdim-3))
                if (((midydim16+yp1) >= 2) && ((midydim16+yp1) <= ydim16-3))
                {
                    int32_t pointsize = 1;
                    col = 15;
                    if (i == pointhighlight || ((pointhighlight < MAXWALLS) && (pointhighlight >= 0) && (wall[i].x == wall[pointhighlight].x) && (wall[i].y == wall[pointhighlight].y)))
                    {
                        if (totalclock & 16)
                        {
                            //col += (2<<2);  // JBF 20040116: two braces is all this needed. man I'm a fool sometimes.
                            pointsize += 1;
                        }
                    }
                    else if ((highlightcnt > 0) && (editstatus == 1))
                    {
                        if (show2dwall[i>>3]&pow2char[i&7])
                            if (totalclock & 16)
                            {
                                //  col += (2<<2);  // JBF 20040116: two braces is all this needed. man I'm a fool sometimes.
                                pointsize += 1;
                            }
                    }

                    tempint = ((midydim16+yp1)*bytesperline)+(halfxdim16+xp1)+frameplace;
#if 1
                    do
                    {
                        /*                        drawline16(halfxdim16+xp1-pointsize,midydim16+yp1+pointsize,halfxdim16+xp1+pointsize,midydim16+yp1+pointsize,col);
                                                drawline16(halfxdim16+xp1+pointsize,midydim16+yp1+pointsize,halfxdim16+xp1+pointsize,midydim16+yp1-pointsize,col);
                                                drawline16(halfxdim16+xp1+pointsize,midydim16+yp1-pointsize,halfxdim16+xp1-pointsize,midydim16+yp1-pointsize,col);
                                                drawline16(halfxdim16+xp1-pointsize,midydim16+yp1-pointsize,halfxdim16+xp1-pointsize,midydim16+yp1+pointsize,col); */
                        drawcircle16(halfxdim16+xp1, midydim16+yp1, pointsize, editorcolors[col]);
                    }
                    while (pointsize--);
#else
                    drawcircle16(halfxdim16+xp1, midydim16+yp1, pointsize, col);
#endif
                }
    }
    faketimerhandler();

    if ((zoome >= 256) || (editstatus == 0))
        for (i=0; i<numsectors; i++)
            for (j=headspritesect[i]; j>=0; j=nextspritesect[j])
                if ((editstatus == 1) || (show2dsprite[j>>3]&pow2char[j&7]))
                {
                    col = 3;
                    if (spritecol2d[sprite[j].picnum][0])
                        col = spritecol2d[sprite[j].picnum][0];
                    if ((sprite[j].cstat&1) > 0)
                    {
                        col = 5;
                        if (spritecol2d[sprite[j].picnum][1])
                            col = spritecol2d[sprite[j].picnum][1];
                    }
                    if (editstatus == 1)
                    {
                        if ((pointhighlight-16384) > 0 && (j+16384 == pointhighlight || ((sprite[j].x == sprite[pointhighlight-16384].x) && (sprite[j].y == sprite[pointhighlight-16384].y))))
                        {
                            if (totalclock & 32) col += (2<<2);
                        }
                        else if ((highlightcnt > 0) && (editstatus == 1))
                        {
                            if (show2dsprite[j>>3]&pow2char[j&7])
                                if (totalclock & 32) col += (2<<2);
                        }
                    }

                    xp1 = mulscale14(sprite[j].x-posxe,zoome);
                    yp1 = mulscale14(sprite[j].y-posye,zoome);
                    if (((halfxdim16+xp1) >= 4) && ((halfxdim16+xp1) <= xdim-6))
                        if (((midydim16+yp1) >= 4) && ((midydim16+yp1) <= ydim16-6))
                        {
                            tempint = ((midydim16+yp1)*bytesperline)+(halfxdim16+xp1)+frameplace;

                            drawcircle16(halfxdim16+xp1, midydim16+yp1, 4, editorcolors[col]);

                            xp2 = mulscale11(sintable[(sprite[j].ang+2560)&2047],zoome) / 768;
                            yp2 = mulscale11(sintable[(sprite[j].ang+2048)&2047],zoome) / 768;

                            drawline16(halfxdim16+xp1,midydim16+yp1,halfxdim16+xp1+xp2,midydim16+yp1+yp2,editorcolors[col]);

                            if ((sprite[j].cstat&256) > 0)
                            {
                                if (((sprite[j].ang+256)&512) == 0)
                                {
                                    drawline16(halfxdim16+xp1,midydim16+yp1+1,halfxdim16+xp1+xp2,midydim16+yp1+yp2+1,editorcolors[col]);
                                    drawline16(halfxdim16+xp1,midydim16+yp1-1,halfxdim16+xp1+xp2,midydim16+yp1+yp2-1,editorcolors[col]);
                                    drawline16(halfxdim16+xp1-1,midydim16+yp1,halfxdim16+xp1+xp2-1,midydim16+yp1+yp2,editorcolors[col]);
                                    drawline16(halfxdim16+xp1+1,midydim16+yp1,halfxdim16+xp1+xp2+1,midydim16+yp1+yp2,editorcolors[col]);

                                }
                                else
                                {
                                    drawline16(halfxdim16+xp1,midydim16+yp1+1,halfxdim16+xp1+xp2,midydim16+yp1+yp2+1,editorcolors[col]);
                                    drawline16(halfxdim16+xp1,midydim16+yp1-1,halfxdim16+xp1+xp2,midydim16+yp1+yp2-1,editorcolors[col]);
                                    drawline16(halfxdim16+xp1-1,midydim16+yp1,halfxdim16+xp1+xp2-1,midydim16+yp1+yp2,editorcolors[col]);
                                    drawline16(halfxdim16+xp1+1,midydim16+yp1,halfxdim16+xp1+xp2+1,midydim16+yp1+yp2,editorcolors[col]);

                                }

                                if ((sprite[j].cstat&32) > 0)
                                {
                                    int32_t fx = mulscale10(mulscale6(tilesizx[sprite[j].picnum], sprite[j].xrepeat),zoome) >> 1;
                                    int32_t fy = mulscale10(mulscale6(tilesizy[sprite[j].picnum], sprite[j].yrepeat),zoome) >> 1;
                                    int32_t co[4][2], ii;
                                    int32_t sinang = sintable[(sprite[j].ang+512+1024)&2047];
                                    int32_t cosang = sintable[(sprite[j].ang+1024)&2047];
                                    int32_t r,s;


                                    co[0][0] = co[3][0] = -fx;
                                    co[0][1] = co[1][1] = -fy;
                                    co[1][0] = co[2][0] = fx;
                                    co[2][1] = co[3][1] = fy;

                                    for (ii=3; ii>=0; ii--)
                                    {
                                        r = mulscale14(cosang,co[ii][0]) - mulscale14(sinang,co[ii][1]);
                                        s = mulscale14(sinang,co[ii][0]) + mulscale14(cosang,co[ii][1]);
                                        co[ii][0] = r;
                                        co[ii][1] = s;
                                    }
                                    drawlinepat = 0xcfcfcfcf;
                                    for (ii=3; ii>=0; ii--)
                                    {
                                        drawline16(halfxdim16 + xp1 + co[ii][0], midydim16 + yp1 - co[ii][1],
                                                   halfxdim16 + xp1 + co[(ii+1)&3][0], midydim16 + yp1 - co[(ii+1)&3][1],
                                                   editorcolors[col]);
                                        drawline16(halfxdim16 + xp1 + co[ii][0], midydim16 + yp1 - co[ii][1] + 1,
                                                   halfxdim16 + xp1 + co[(ii+1)&3][0], midydim16 + yp1 - co[(ii+1)&3][1] + 1,
                                                   editorcolors[col]);
                                        drawline16(halfxdim16 + xp1 + co[ii][0], midydim16 + yp1 - co[ii][1] - 1,
                                                   halfxdim16 + xp1 + co[(ii+1)&3][0], midydim16 + yp1 - co[(ii+1)&3][1] - 1,
                                                   editorcolors[col]);
                                        drawline16(halfxdim16 + xp1 + co[ii][0] + 1, midydim16 + yp1 - co[ii][1],
                                                   halfxdim16 + xp1 + co[(ii+1)&3][0] + 1, midydim16 + yp1 - co[(ii+1)&3][1],
                                                   editorcolors[col]);
                                        drawline16(halfxdim16 + xp1 + co[ii][0] - 1, midydim16 + yp1 - co[ii][1],
                                                   halfxdim16 + xp1 + co[(ii+1)&3][0] - 1, midydim16 + yp1 - co[(ii+1)&3][1],
                                                   editorcolors[col]);
                                        drawline16(halfxdim16 + xp1, midydim16 + yp1,
                                                   halfxdim16 + xp1 + co[(ii+1)&3][0], midydim16 + yp1 - co[(ii+1)&3][1],
                                                   editorcolors[col]);
                                    }
                                    drawlinepat = 0xffffffff;
                                }

                                else if ((sprite[j].cstat&16) > 0)
                                {
                                    int32_t fx = mulscale6(tilesizx[sprite[j].picnum], sprite[j].xrepeat);
                                    xp2 = mulscale11(sintable[(sprite[j].ang+2560)&2047],zoome) / 6144;
                                    yp2 = mulscale11(sintable[(sprite[j].ang+2048)&2047],zoome) / 6144;


                                    if (((sprite[j].ang+256)&512) == 0)
                                    {
                                        if (!(sprite[j].cstat&64))
                                        {
                                            drawline16(halfxdim16+xp1,midydim16+yp1-1,halfxdim16+xp1-xp2,midydim16+yp1-yp2-1,editorcolors[col]);
                                            drawline16(halfxdim16+xp1,midydim16+yp1,halfxdim16+xp1-xp2,midydim16+yp1-yp2,editorcolors[col]);
                                            drawline16(halfxdim16+xp1,midydim16+yp1+1,halfxdim16+xp1-xp2,midydim16+yp1-yp2+1,editorcolors[col]);
                                        }
                                        drawline16(halfxdim16+xp1,midydim16+yp1-1,halfxdim16+xp1+xp2,midydim16+yp1+yp2-1,editorcolors[col]);
                                        drawline16(halfxdim16+xp1,midydim16+yp1,halfxdim16+xp1+xp2,midydim16+yp1+yp2,editorcolors[col]);
                                        drawline16(halfxdim16+xp1,midydim16+yp1+1,halfxdim16+xp1+xp2,midydim16+yp1+yp2+1,editorcolors[col]);
                                        xp2 = mulscale13(sintable[(sprite[j].ang+1024)&2047],zoome) * fx / 4096;
                                        yp2 = mulscale13(sintable[(sprite[j].ang+512)&2047],zoome) * fx / 4096;
                                        drawline16(halfxdim16+xp1+1,midydim16+yp1,halfxdim16+xp1+xp2+1,midydim16+yp1+yp2,editorcolors[col]);
                                        drawline16(halfxdim16+xp1-1,midydim16+yp1,halfxdim16+xp1-xp2-1,midydim16+yp1-yp2,editorcolors[col]);
                                        drawline16(halfxdim16+xp1-1,midydim16+yp1,halfxdim16+xp1+xp2-1,midydim16+yp1+yp2,editorcolors[col]);
                                        drawline16(halfxdim16+xp1+1,midydim16+yp1,halfxdim16+xp1-xp2+1,midydim16+yp1-yp2,editorcolors[col]);

                                        drawline16(halfxdim16+xp1,midydim16+yp1,halfxdim16+xp1-xp2,midydim16+yp1-yp2,editorcolors[col]);
                                        drawline16(halfxdim16+xp1,midydim16+yp1,halfxdim16+xp1+xp2,midydim16+yp1+yp2,editorcolors[col]);

                                        drawline16(halfxdim16+xp1,midydim16+yp1-1,halfxdim16+xp1+xp2,midydim16+yp1+yp2-1,editorcolors[col]);
                                        drawline16(halfxdim16+xp1,midydim16+yp1+1,halfxdim16+xp1-xp2,midydim16+yp1-yp2+1,editorcolors[col]);
                                        drawline16(halfxdim16+xp1,midydim16+yp1+1,halfxdim16+xp1+xp2,midydim16+yp1+yp2+1,editorcolors[col]);
                                        drawline16(halfxdim16+xp1,midydim16+yp1-1,halfxdim16+xp1-xp2,midydim16+yp1-yp2-1,editorcolors[col]);
                                    }
                                    else
                                    {
                                        if (!(sprite[j].cstat&64))
                                        {
                                            drawline16(halfxdim16+xp1-1,midydim16+yp1,halfxdim16+xp1-xp2-1,midydim16+yp1-yp2,editorcolors[col]);
                                            drawline16(halfxdim16+xp1,midydim16+yp1,halfxdim16+xp1-xp2,midydim16+yp1-yp2,editorcolors[col]);
                                            drawline16(halfxdim16+xp1+1,midydim16+yp1,halfxdim16+xp1-xp2+1,midydim16+yp1-yp2,editorcolors[col]);
                                        }
                                        drawline16(halfxdim16+xp1-1,midydim16+yp1,halfxdim16+xp1+xp2-1,midydim16+yp1+yp2,editorcolors[col]);
                                        drawline16(halfxdim16+xp1,midydim16+yp1,halfxdim16+xp1+xp2,midydim16+yp1+yp2,editorcolors[col]);
                                        drawline16(halfxdim16+xp1+1,midydim16+yp1,halfxdim16+xp1+xp2+1,midydim16+yp1+yp2,editorcolors[col]);
                                        xp2 = mulscale13(sintable[(sprite[j].ang+1024)&2047],zoome) * fx / 4096;
                                        yp2 = mulscale13(sintable[(sprite[j].ang+512)&2047],zoome) * fx / 4096;
                                        drawline16(halfxdim16+xp1+1,midydim16+yp1,halfxdim16+xp1+xp2+1,midydim16+yp1+yp2,editorcolors[col]);
                                        drawline16(halfxdim16+xp1-1,midydim16+yp1,halfxdim16+xp1-xp2-1,midydim16+yp1-yp2,editorcolors[col]);
                                        drawline16(halfxdim16+xp1-1,midydim16+yp1,halfxdim16+xp1+xp2-1,midydim16+yp1+yp2,editorcolors[col]);
                                        drawline16(halfxdim16+xp1+1,midydim16+yp1,halfxdim16+xp1-xp2+1,midydim16+yp1-yp2,editorcolors[col]);

                                        drawline16(halfxdim16+xp1,midydim16+yp1,halfxdim16+xp1-xp2,midydim16+yp1-yp2,editorcolors[col]);
                                        drawline16(halfxdim16+xp1,midydim16+yp1,halfxdim16+xp1+xp2,midydim16+yp1+yp2,editorcolors[col]);

                                        drawline16(halfxdim16+xp1,midydim16+yp1-1,halfxdim16+xp1+xp2,midydim16+yp1+yp2-1,editorcolors[col]);
                                        drawline16(halfxdim16+xp1,midydim16+yp1+1,halfxdim16+xp1-xp2,midydim16+yp1-yp2+1,editorcolors[col]);
                                        drawline16(halfxdim16+xp1,midydim16+yp1+1,halfxdim16+xp1+xp2,midydim16+yp1+yp2+1,editorcolors[col]);
                                        drawline16(halfxdim16+xp1,midydim16+yp1-1,halfxdim16+xp1-xp2,midydim16+yp1-yp2-1,editorcolors[col]);
                                    }

                                }

                                col += 8;
                            }

                            else if ((sprite[j].cstat&16) > 0)
                            {
                                int32_t fx = mulscale6(tilesizx[sprite[j].picnum], sprite[j].xrepeat);

                                xp2 = mulscale11(sintable[(sprite[j].ang+2560)&2047],zoome) / 6144;
                                yp2 = mulscale11(sintable[(sprite[j].ang+2048)&2047],zoome) / 6144;

                                drawline16(halfxdim16+xp1,midydim16+yp1,halfxdim16+xp1+xp2,midydim16+yp1+yp2,editorcolors[col]);
                                if (!(sprite[j].cstat&64)) drawline16(halfxdim16+xp1,midydim16+yp1,halfxdim16+xp1-xp2,midydim16+yp1-yp2,editorcolors[col]);
                                xp2 = mulscale13(sintable[(sprite[j].ang+1024)&2047],zoome) * fx / 4096;
                                yp2 = mulscale13(sintable[(sprite[j].ang+512)&2047],zoome) * fx / 4096;

                                drawline16(halfxdim16+xp1,midydim16+yp1,halfxdim16+xp1+xp2,midydim16+yp1+yp2,editorcolors[col]);
                                drawline16(halfxdim16+xp1,midydim16+yp1,halfxdim16+xp1-xp2,midydim16+yp1-yp2,editorcolors[col]);


                                col += 8;
                            }

                            else if ((sprite[j].cstat&32) > 0)
                            {
                                int32_t fx = mulscale10(mulscale6(tilesizx[sprite[j].picnum], sprite[j].xrepeat),zoome) >> 1;
                                int32_t fy = mulscale10(mulscale6(tilesizy[sprite[j].picnum], sprite[j].yrepeat),zoome) >> 1;
                                int32_t co[4][2], ii;
                                int32_t sinang = sintable[(sprite[j].ang+512+1024)&2047];
                                int32_t cosang = sintable[(sprite[j].ang+1024)&2047];
                                int32_t r,s;

                                co[0][0] = co[3][0] = -fx;
                                co[0][1] = co[1][1] = -fy;
                                co[1][0] = co[2][0] = fx;
                                co[2][1] = co[3][1] = fy;

                                for (ii=3; ii>=0; ii--)
                                {
                                    r = mulscale14(cosang,co[ii][0]) - mulscale14(sinang,co[ii][1]);
                                    s = mulscale14(sinang,co[ii][0]) + mulscale14(cosang,co[ii][1]);
                                    co[ii][0] = r;
                                    co[ii][1] = s;
                                }

                                drawlinepat = 0xcfcfcfcf;
                                for (ii=3; ii>=0; ii--)
                                {
                                    drawline16(halfxdim16 + xp1 + co[ii][0], midydim16 + yp1 - co[ii][1],
                                               halfxdim16 + xp1 + co[(ii+1)&3][0], midydim16 + yp1 - co[(ii+1)&3][1],
                                               editorcolors[col]);
                                    drawline16(halfxdim16 + xp1, midydim16 + yp1,
                                               halfxdim16 + xp1 + co[(ii+1)&3][0], midydim16 + yp1 - co[(ii+1)&3][1],
                                               editorcolors[col]);
                                }
                                drawlinepat = 0xffffffff;
                            }
                        }
                }

    faketimerhandler();
    xp1 = mulscale11(sintable[(ange+2560)&2047],zoome) / 768; //Draw white arrow
    yp1 = mulscale11(sintable[(ange+2048)&2047],zoome) / 768;
    drawline16(halfxdim16+xp1,midydim16+yp1,halfxdim16-xp1,midydim16-yp1,editorcolors[15]);
    drawline16(halfxdim16+xp1,midydim16+yp1,halfxdim16+yp1,midydim16-xp1,editorcolors[15]);
    drawline16(halfxdim16+xp1,midydim16+yp1,halfxdim16-yp1,midydim16+xp1,editorcolors[15]);


    enddrawing();   //}}}
}


//
// printext16
//
int32_t printext16(int32_t xpos, int32_t ypos, int16_t col, int16_t backcol, char *name, char fontsize)
{
    int32_t stx, i, x, y, charxsiz, ocol = col, obackcol = backcol;
    char *fontptr, *letptr, *ptr;
    char smallbuf[4];
    stx = xpos;

    if (fontsize & 2) printext16(xpos+1, ypos+1, 0, -1, name, (fontsize & ~2) | 4);
    if (fontsize & 1) { fontptr = smalltextfont; charxsiz = 4; }
    else { fontptr = textfont; charxsiz = 8; }

    begindrawing(); //{{{
    for (i=0; name[i]; i++)
    {
        if (name[i] == '^')
        {
            i++;
            if (name[i] == 'O') // ^O resets formatting
            {
                if (fontsize & 4) continue;

                col = ocol;
                backcol = obackcol;
                continue;
            }
            if (isdigit(name[i]))
            {
                if (isdigit(name[i+1]))
                {
                    if (isdigit(name[i+2]))
                    {
                        Bmemcpy(&smallbuf[0],&name[i],3);
                        i += 2;
                        smallbuf[3] = '\0';
                    }
                    else
                    {
                        Bmemcpy(&smallbuf[0],&name[i],2);
                        i++;
                        smallbuf[2] = '\0';
                    }
                }
                else
                {
                    smallbuf[0] = name[i];
                    smallbuf[1] = '\0';
                }
                if (!(fontsize & 4))
                    col = editorcolors[atol(smallbuf)];

                if (name[i+1] == ',' && isdigit(name[i+2]))
                {
                    i+=2;
                    if (isdigit(name[i+1]))
                    {
                        if (isdigit(name[i+2]))
                        {
                            Bmemcpy(&smallbuf[0],&name[i],3);
                            i += 2;
                            smallbuf[3] = '\0';
                        }
                        else
                        {
                            Bmemcpy(&smallbuf[0],&name[i],2);
                            i++;
                            smallbuf[2] = '\0';
                        }
                    }
                    else
                    {
                        smallbuf[0] = name[i];
                        smallbuf[1] = '\0';
                    }

                    if (!(fontsize & 4))
                        backcol = editorcolors[atol(smallbuf)];
                }
                continue;
            }
        }

        letptr = &fontptr[name[i]<<3];
        ptr = (char *)(bytesperline*(ypos+7)+(stx-(fontsize&1))+frameplace);
        for (y=7; y>=0; y--)
        {
            for (x=charxsiz-1; x>=0; x--)
            {
                if (letptr[y]&pow2char[7-(fontsize&1)-x])
                    ptr[x] = (uint8_t)col;
                else if (backcol >= 0)
                    ptr[x] = (uint8_t)backcol;
            }
            ptr -= bytesperline;
        }
        stx += charxsiz;
    }
    enddrawing();   //}}}

    return stx;
}


//
// printext256
//
void printext256(int32_t xpos, int32_t ypos, int16_t col, int16_t backcol, char *name, char fontsize)
{
    int32_t stx, i, x, y, charxsiz;
    char *fontptr, *letptr, *ptr;

    stx = xpos;

    if (fontsize) { fontptr = smalltextfont; charxsiz = 4; }
    else { fontptr = textfont; charxsiz = 8; }

#if defined(POLYMOST) && defined(USE_OPENGL)
    if (!polymost_printext256(xpos,ypos,col,backcol,name,fontsize)) return;

    if (rendmode >= 3 && qsetmode == 200)
    {
        int32_t xx, yy;
        int32_t lc=-1;
        palette_t p,b;

        if (gammabrightness)
        {
            p = curpalette[col];
            b = curpalette[backcol];
        }
        else
        {
            p.r = britable[curbrightness][ curpalette[col].r ];
            p.g = britable[curbrightness][ curpalette[col].g ];
            p.b = britable[curbrightness][ curpalette[col].b ];
            b.r = britable[curbrightness][ curpalette[backcol].r ];
            b.g = britable[curbrightness][ curpalette[backcol].g ];
            b.b = britable[curbrightness][ curpalette[backcol].b ];
        }

        setpolymost2dview();
        bglDisable(GL_ALPHA_TEST);
        bglDepthMask(GL_FALSE);	// disable writing to the z-buffer

        bglBegin(GL_POINTS);

        for (i=0; name[i]; i++)
        {
            if (name[i] == '^' && isdigit(name[i+1]))
            {
                char smallbuf[8];
                int32_t bi=0;
                while (isdigit(name[i+1]) && bi<8)
                {
                    smallbuf[bi++]=name[i+1];
                    i++;
                }
                smallbuf[bi++]=0;
                if (col)col = atol(smallbuf);

                if (gammabrightness)
                {
                    p = curpalette[col];
                }
                else
                {
                    p.r = britable[curbrightness][ curpalette[col].r ];
                    p.g = britable[curbrightness][ curpalette[col].g ];
                    p.b = britable[curbrightness][ curpalette[col].b ];
                }
                continue;
            }
            letptr = &fontptr[name[i]<<3];
            xx = stx-fontsize;
            yy = ypos+7 + 2; //+1 is hack!
            for (y=7; y>=0; y--)
            {
                for (x=charxsiz-1; x>=0; x--)
                {
                    if (letptr[y]&pow2char[7-fontsize-x])
                    {
                        if (lc!=col)
                            bglColor4ub(p.r,p.g,p.b,255);
                        lc = col;
                        bglVertex2i(xx+x,yy);
                    }
                    else if (backcol >= 0)
                    {
                        if (lc!=backcol)
                            bglColor4ub(b.r,b.g,b.b,255);
                        lc = backcol;
                        bglVertex2i(xx+x,yy);
                    }
                }
                yy--;
            }
            stx += charxsiz;
        }

        bglEnd();
        bglDepthMask(GL_TRUE);  // re-enable writing to the z-buffer

        return;
    }
#endif

    begindrawing(); //{{{
    for (i=0; name[i]; i++)
    {
        if (name[i] == '^' && isdigit(name[i+1]))
        {
            char smallbuf[8];
            int32_t bi=0;
            while (isdigit(name[i+1]) && bi<8)
            {
                smallbuf[bi++]=name[i+1];
                i++;
            }
            smallbuf[bi++]=0;
            if (col)col = atol(smallbuf);
            continue;
        }
        letptr = &fontptr[name[i]<<3];
        ptr = (char *)(ylookup[ypos+7]+(stx-fontsize)+frameplace);
        for (y=7; y>=0; y--)
        {
            for (x=charxsiz-1; x>=0; x--)
            {
                if (letptr[y]&pow2char[7-fontsize-x])
                    ptr[x] = (uint8_t)col;
                else if (backcol >= 0)
                    ptr[x] = (uint8_t)backcol;
            }
            ptr -= ylookup[1];
        }
        stx += charxsiz;
    }
    enddrawing();   //}}}
}


//
// screencapture
//
int32_t screencapture_tga(char *filename, char inverseit)
{
    int32_t i,j;
    char *ptr, head[18] = { 0,1,1,0,0,0,1,24,0,0,0,0,0/*wlo*/,0/*whi*/,0/*hlo*/,0/*hhi*/,8,0 };
    //char palette[4*256];
    char *fn = Bstrdup(filename), *inversebuf;
    BFILE *fil;

    do      // JBF 2004022: So we don't overwrite existing screenshots
    {
        if (capturecount > 9999) return -1;

        i = Bstrrchr(fn,'.')-fn-4;
        fn[i++] = ((capturecount/1000)%10)+48;
        fn[i++] = ((capturecount/100)%10)+48;
        fn[i++] = ((capturecount/10)%10)+48;
        fn[i++] = (capturecount%10)+48;
        i++;
        fn[i++] = 't';
        fn[i++] = 'g';
        fn[i++] = 'a';

        if ((fil = Bfopen(fn,"rb")) == NULL) break;
        Bfclose(fil);
        capturecount++;
    }
    while (1);
    fil = Bfopen(fn,"wb");
    if (fil == NULL)
    {
        Bfree(fn);
        return -1;
    }

#if defined(POLYMOST) && defined(USE_OPENGL)
    if (rendmode >= 3 && qsetmode == 200)
    {
        head[1] = 0;    // no colourmap
        head[2] = 2;    // uncompressed truecolour
        head[3] = 0;    // (low) first colourmap index
        head[4] = 0;    // (high) first colourmap index
        head[5] = 0;    // (low) number colourmap entries
        head[6] = 0;    // (high) number colourmap entries
        head[7] = 0;    // colourmap entry size
        head[16] = 24;  // 24 bits per pixel
    }
#endif

    head[12] = xdim & 0xff;
    head[13] = (xdim >> 8) & 0xff;
    head[14] = ydim & 0xff;
    head[15] = (ydim >> 8) & 0xff;

    Bfwrite(head, 18, 1, fil);

    begindrawing(); //{{{
    ptr = (char *)frameplace;

    // palette first
#if defined(POLYMOST) && defined(USE_OPENGL)
    if (rendmode < 3 || (rendmode >= 3 && qsetmode != 200))
    {
#endif
        //getpalette(0,256,palette);
        for (i=0; i<256; i++)
        {
            Bfputc(curpalettefaded[i].b, fil);  // b
            Bfputc(curpalettefaded[i].g, fil);  // g
            Bfputc(curpalettefaded[i].r, fil);  // r
        }
#if defined(POLYMOST) && defined(USE_OPENGL)
    }
#endif

    // targa renders bottom to top, from left to right
    if (inverseit && qsetmode != 200)
    {
        inversebuf = (char *)kmalloc(bytesperline);
        if (inversebuf)
        {
            for (i=ydim-1; i>=0; i--)
            {
                copybuf(ptr+i*bytesperline, inversebuf, xdim >> 2);
                for (j=0; j < (bytesperline>>2); j++)((int32_t *)inversebuf)[j] ^= 0x0f0f0f0fL;
                Bfwrite(inversebuf, xdim, 1, fil);
            }
            kfree(inversebuf);
        }
    }
    else
    {
#if defined(POLYMOST) && defined(USE_OPENGL)
        if (rendmode >= 3 && qsetmode == 200)
        {
            char c;
            // 24bit
            inversebuf = (char *)kmalloc(xdim*ydim*3);
            if (inversebuf)
            {
                bglReadPixels(0,0,xdim,ydim,GL_RGB,GL_UNSIGNED_BYTE,inversebuf);
                j = xdim*ydim*3;
                for (i=0; i<j; i+=3)
                {
                    c = inversebuf[i];
                    inversebuf[i] = inversebuf[i+2];
                    inversebuf[i+2] = c;
                }
                Bfwrite(inversebuf, xdim*ydim, 3, fil);
                kfree(inversebuf);
            }
        }
        else
        {
#endif
            for (i=ydim-1; i>=0; i--)
                Bfwrite(ptr+i*bytesperline, xdim, 1, fil);
#if defined(POLYMOST) && defined(USE_OPENGL)
        }
#endif
    }

    enddrawing();   //}}}

    Bfclose(fil);
    OSD_Printf("Saved screenshot to %s\n", fn);
    Bfree(fn);
    capturecount++;
    return(0);
}

// PCX is nasty, which is why I've lifted these functions from the PCX spec by ZSoft
static int32_t writepcxbyte(char colour, char count, BFILE *fp)
{
    if (!count) return 0;
    if (count == 1 && (colour & 0xc0) != 0xc0)
    {
        Bfputc(colour, fp);
        return 1;
    }
    else
    {
        Bfputc(0xc0 | count, fp);
        Bfputc(colour, fp);
        return 2;
    }
}

static void writepcxline(char *buf, int32_t bytes, int32_t step, BFILE *fp)
{
    char ths, last;
    int32_t srcIndex;
    char runCount;

    runCount = 1;
    last = *buf;

    for (srcIndex=1; srcIndex<bytes; srcIndex++)
    {
        buf += step;
        ths = *buf;
        if (ths == last)
        {
            runCount++;
            if (runCount == 63)
            {
                writepcxbyte(last, runCount, fp);
                runCount = 0;
            }
        }
        else
        {
            if (runCount)
                writepcxbyte(last, runCount, fp);
            last = ths;
            runCount = 1;
        }
    }

    if (runCount) writepcxbyte(last, runCount, fp);
    if (bytes&1) writepcxbyte(0, 1, fp);
}

int32_t screencapture_pcx(char *filename, char inverseit)
{
    int32_t i,j,bpl;
    char *ptr, head[128];
    //char palette[4*256];
    char *fn = Bstrdup(filename), *inversebuf;
    BFILE *fil;

    do      // JBF 2004022: So we don't overwrite existing screenshots
    {
        if (capturecount > 9999) return -1;

        i = Bstrrchr(fn,'.')-fn-4;
        fn[i++] = ((capturecount/1000)%10)+48;
        fn[i++] = ((capturecount/100)%10)+48;
        fn[i++] = ((capturecount/10)%10)+48;
        fn[i++] = (capturecount%10)+48;
        i++;
        fn[i++] = 'p';
        fn[i++] = 'c';
        fn[i++] = 'x';

        if ((fil = Bfopen(fn,"rb")) == NULL) break;
        Bfclose(fil);
        capturecount++;
    }
    while (1);
    fil = Bfopen(fn,"wb");
    if (fil == NULL)
    {
        Bfree(fn);
        return -1;
    }

    memset(head,0,128);
    head[0] = 10;
    head[1] = 5;
    head[2] = 1;
    head[3] = 8;
    head[12] = 72; head[13] = 0;
    head[14] = 72; head[15] = 0;
    head[65] = 1;   // 8-bit
    head[68] = 1;

#if defined(POLYMOST) && defined(USE_OPENGL)
    if (rendmode >= 3 && qsetmode == 200)
    {
        head[65] = 3;   // 24-bit
    }
#endif

    head[8] = (xdim-1) & 0xff;
    head[9] = ((xdim-1) >> 8) & 0xff;
    head[10] = (ydim-1) & 0xff;
    head[11] = ((ydim-1) >> 8) & 0xff;

    bpl = xdim + (xdim&1);

    head[66] = bpl & 0xff;
    head[67] = (bpl >> 8) & 0xff;

    Bfwrite(head, 128, 1, fil);

    begindrawing(); //{{{
    ptr = (char *)frameplace;

    // targa renders bottom to top, from left to right
    if (inverseit && qsetmode != 200)
    {
        inversebuf = (char *)kmalloc(bytesperline);
        if (inversebuf)
        {
            for (i=0; i<ydim; i++)
            {
                copybuf(ptr+i*bytesperline, inversebuf, xdim >> 2);
                for (j=0; j < (bytesperline>>2); j++)((int32_t *)inversebuf)[j] ^= 0x0f0f0f0fL;
                writepcxline(inversebuf, xdim, 1, fil);
            }
            kfree(inversebuf);
        }
    }
    else
    {
#if defined(POLYMOST) && defined(USE_OPENGL)
        if (rendmode >= 3 && qsetmode == 200)
        {
            // 24bit
            inversebuf = (char *)kmalloc(xdim*ydim*3);
            if (inversebuf)
            {
                bglReadPixels(0,0,xdim,ydim,GL_RGB,GL_UNSIGNED_BYTE,inversebuf);
                for (i=ydim-1; i>=0; i--)
                {
                    writepcxline(inversebuf+i*xdim*3,   xdim, 3, fil);
                    writepcxline(inversebuf+i*xdim*3+1, xdim, 3, fil);
                    writepcxline(inversebuf+i*xdim*3+2, xdim, 3, fil);
                }
                kfree(inversebuf);
            }
        }
        else
        {
#endif
            for (i=0; i<ydim; i++)
                writepcxline(ptr+i*bytesperline, xdim, 1, fil);
#if defined(POLYMOST) && defined(USE_OPENGL)
        }
#endif
    }

    enddrawing();   //}}}

    // palette last
#if defined(POLYMOST) && defined(USE_OPENGL)
    if (rendmode < 3 || (rendmode >= 3 && qsetmode != 200))
    {
#endif
        //getpalette(0,256,palette);
        Bfputc(12,fil);
        for (i=0; i<256; i++)
        {
            Bfputc(curpalettefaded[i].r, fil);  // b
            Bfputc(curpalettefaded[i].g, fil);  // g
            Bfputc(curpalettefaded[i].b, fil);  // r
        }
#if defined(POLYMOST) && defined(USE_OPENGL)
    }
#endif

    Bfclose(fil);
    OSD_Printf("Saved screenshot to %s\n", fn);
    Bfree(fn);
    capturecount++;
    return(0);
}

int32_t screencapture(char *filename, char inverseit)
{
    if (captureformat == 0) return screencapture_tga(filename,inverseit);
    else return screencapture_pcx(filename,inverseit);
}



//
// setrendermode
//
int32_t setrendermode(int32_t renderer)
{
    UNREFERENCED_PARAMETER(renderer);
#if defined(POLYMOST) && defined(USE_OPENGL)
    if (bpp == 8) renderer = 0;
    else renderer = min(4,max(3,renderer));
# ifdef POLYMER
    if (renderer == 4)
        polymer_init();
# else
    if (renderer == 4)
        renderer = 3;
# endif

    rendmode = renderer;
    if (rendmode >= 3)
        glrendmode = rendmode;
#endif

    return 0;
}

//
// setrollangle
//
#ifdef POLYMOST
void setrollangle(int32_t rolla)
{
    UNREFERENCED_PARAMETER(rolla);
    if (rolla == 0) gtang = 0.0;
    else gtang = PI * (double)rolla / 1024.0;
}
#endif


//
// invalidatetile
//  pal: pass -1 to invalidate all palettes for the tile, or >=0 for a particular palette
//  how: pass -1 to invalidate all instances of the tile in texture memory, or a bitfield
//         bit 0: opaque or masked (non-translucent) texture, using repeating
//         bit 1: ignored
//         bit 2: ignored (33% translucence, using repeating)
//         bit 3: ignored (67% translucence, using repeating)
//         bit 4: opaque or masked (non-translucent) texture, using clamping
//         bit 5: ignored
//         bit 6: ignored (33% translucence, using clamping)
//         bit 7: ignored (67% translucence, using clamping)
//       clamping is for sprites, repeating is for walls
//
void invalidatetile(int16_t tilenume, int32_t pal, int32_t how)
{
#if defined(POLYMOST) && defined(USE_OPENGL)
    int32_t numpal, firstpal, np;
    int32_t hp;

    if (rendmode < 3) return;

    if (pal < 0)
    {
        numpal = MAXPALOOKUPS;
        firstpal = 0;
    }
    else
    {
        numpal = 1;
        firstpal = pal % MAXPALOOKUPS;
    }

    for (hp = 0; hp < 8; hp+=4)
    {
        if (!(how & pow2long[hp])) continue;

        for (np = firstpal; np < firstpal+numpal; np++)
        {
            gltexinvalidate(tilenume, np, hp);
        }
    }
#endif
    UNREFERENCED_PARAMETER(tilenume);
    UNREFERENCED_PARAMETER(pal);
    UNREFERENCED_PARAMETER(how);
}


//
// setpolymost2dview
//  Sets OpenGL for 2D drawing
//
void setpolymost2dview(void)
{
#if defined(POLYMOST) && defined(USE_OPENGL)
    if (rendmode < 3) return;

    if (gloy1 != -1)
    {
        bglViewport(0,0,xres,yres);
        bglMatrixMode(GL_PROJECTION);
        bglLoadIdentity();
        bglOrtho(0,xres,yres,0,-1,1);
        bglMatrixMode(GL_MODELVIEW);
        bglLoadIdentity();
    }

    gloy1 = -1;

    bglDisable(GL_DEPTH_TEST);
    bglDisable(GL_TEXTURE_2D);
    bglDisable(GL_BLEND);
#endif
}

void hash_init(hashtable_t *t)
{
    hash_free(t);
    t->items=Bcalloc(1, t->size * sizeof(hashitem_t));
}

void hash_free(hashtable_t *t)
{
    hashitem_t *cur, *tmp;
    int32_t i;
    int32_t num;

    if (t->items == NULL)
        return;
//    initprintf("*free, num:%d\n",t->size);
    i= t->size-1;
    do
    {
        cur = t->items[i];
        num = 0;
        while (cur)
        {
            tmp = cur;
            cur = cur->next;
//          initprintf("Free %4d '%s'\n",tmp->key,(tmp->string)?tmp->string:".");
            if (tmp->string)
            {
                Bfree(tmp->string);
                tmp->string = NULL;
            }
            Bfree(tmp);
            num++;
        }
//        initprintf("#%4d: %3d\t",i,num);
    }
    while (--i > -1);
    Bfree(t->items);
    t->items = 0;
}

#if 1
// djb3 algorithm
inline uint32_t HASH_getcode(const char *s)
{
    uint32_t h = 5381;
    int32_t ch;

    while ((ch = *s++) != '\0')
        h = ((h << 5) + h) ^ ch;

    return h;
}
#else
inline uint32_t HASH_getcode(const char *s)
{
    int32_t i=0, fact=1;
    while (*s)
    {
        i+=*s;
        i+=1<<fact;
        s++;
    }
    return i;
}
#endif

void hash_add(hashtable_t *t, const char *s, int32_t key)
{
    hashitem_t *cur, *prev=NULL;
    int32_t code;

    if (!s)
        return;
    if (t->items == NULL)
    {
        initprintf("hash_add(): table not initialized!\n");
        return;
    }
    code = HASH_getcode(s)%t->size;
    cur = t->items[code];

    if (!cur)
    {
        cur=Bcalloc(1,sizeof(hashitem_t));
        cur->string=Bstrdup(s);
        cur->key=key;
        cur->next=NULL;
        t->items[code]=cur;
        return;
    }

    do
    {
        if (Bstrcmp(s,cur->string)==0)
            return;
        prev=cur;
        cur=cur->next;
    }
    while (cur);

    cur=Bcalloc(1,sizeof(hashitem_t));
    cur->string=Bstrdup(s);
    cur->key=key;
    cur->next=NULL;
    prev->next=cur;
}

void hash_replace(hashtable_t *t, const char *s, int32_t key)
{
    hashitem_t *cur, *prev=NULL;
    int32_t code;

    if (t->items==NULL)
    {
        initprintf("hash_replace(): table not initialized!\n");
        return;
    }
    code=HASH_getcode(s)%t->size;
    cur=t->items[code];

    if (!cur)
    {
        cur=Bcalloc(1,sizeof(hashitem_t));
        cur->string=Bstrdup(s);
        cur->key=key;
        cur->next=NULL;
        t->items[code]=cur;
        return;
    }

    do
    {
        if (Bstrcmp(s,cur->string)==0)
        {
            cur->key=key;
            return;
        }
        prev=cur;
        cur=cur->next;
    }
    while (cur);

    cur=Bcalloc(1,sizeof(hashitem_t));
    cur->string=Bstrdup(s);
    cur->key=key;
    cur->next=NULL;
    prev->next=cur;
}

int32_t hash_find(hashtable_t *t, const char *s)
{
    hashitem_t *cur;

    if (t->items==NULL)
    {
        initprintf("hash_find(): table not initialized!\n");
        return -1;
    }
    cur=t->items[HASH_getcode(s)%t->size];
    while (cur)
    {
        if (Bstrcmp(s,cur->string) == 0)
            return cur->key;
        cur=cur->next;
    }
    return -1;
}

int32_t hash_findcase(hashtable_t *t, const char *s)
{
    hashitem_t *cur;

    if (t->items==NULL)
    {
        initprintf("hash_findcase(): table not initialized!\n");
        return -1;
    }
    cur=t->items[HASH_getcode(s)%t->size];
    while (cur)
    {
        if (Bstrcasecmp(s,cur->string) == 0)
            return cur->key;
        cur=cur->next;
    }
    return -1;
}

/*
 * vim:ts=8:
 */

