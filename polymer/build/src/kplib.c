/**************************************************************************************************
KPLIB.C: Ken's Picture LIBrary written by Ken Silverman
Copyright (c) 1998-2005 Ken Silverman
Ken Silverman's official web site: http://advsys.net/ken

Features of KPLIB.C:
	* Routines for decoding JPG, PNG, GIF, PCX, TGA, BMP, CEL.
		See kpgetdim(), kprender(), and optional helper function: kpzload().
	* Routines for ZIP decompression. All ZIP functions start with the letters "kz".
	* Multi-platform support: Dos/Windows/Linux/Mac/etc..
	* Compact code, all in a single source file. Yeah, bad design on my part... but makes life
		  easier for everyone else - you simply add a single C file to your project, throw a few
		  externs in there, add the function calls, and you're done!

Brief history:
1998?: Wrote KPEG, a JPEG viewer for DOS
2000: Wrote KPNG, a PNG viewer for DOS
2001: Combined KPEG & KPNG, ported to Visual C, and made it into a library called KPLIB.C
2002: Added support for TGA, GIF, CEL, ZIP
2003: Added support for BMP
05/18/2004: Added support for 8&24 bit PCX
12/09/2005: Added support for progressive JPEG

I offer this code to the community for free use - all I ask is that my name be included in the
credits.

-Ken S.
**************************************************************************************************/

#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__POWERPC__)
#define BIGENDIAN 1
#endif

#ifdef BIGENDIAN
static unsigned long LSWAPIB (unsigned long a) { return(((a>>8)&0xff00)+((a&0xff00)<<8)+(a<<24)+(a>>24)); }
static unsigned short SSWAPIB (unsigned short a) { return((a>>8)+(a<<8)); }
#define LSWAPIL(a) (a)
#define SSWAPIL(a) (a)
#else
#define LSWAPIB(a) (a)
#define SSWAPIB(a) (a)
static unsigned long LSWAPIL (unsigned long a) { return(((a>>8)&0xff00)+((a&0xff00)<<8)+(a<<24)+(a>>24)); }
static unsigned short SSWAPIL (unsigned short a) { return((a>>8)+(a<<8)); }
#endif

#if !defined(_WIN32) && !defined(__DOS__)
#include <unistd.h>
#include <dirent.h>
typedef long long __int64;
static __inline long _lrotl (long i, int sh)
	{ return((i>>(-sh))|(i<<sh)); }
static __inline long filelength (int h)
{
	struct stat st;
	if (fstat(h,&st) < 0) return(-1);
	return(st.st_size);
}
#define _fileno fileno
#else
#include <io.h>
#endif

#if defined(__DOS__)
#include <dos.h>
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif
#if !defined(max)
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#if !defined(min)
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#if defined(__GNUC__)
#define _inline inline
#endif

	//use GCC-specific extension to force symbol name to be something in particular to override underscoring.
#if defined(__GNUC__) && defined(__i386__) && !defined(NOASM)
#define ASMNAME(x) asm(x)
#else
#define ASMNAME(x)
#endif

static long frameplace, bytesperline, xres, yres, globxoffs, globyoffs;

static const long pow2mask[32] =
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
static const long pow2long[32] =
{
	0x00000001,0x00000002,0x00000004,0x00000008,
	0x00000010,0x00000020,0x00000040,0x00000080,
	0x00000100,0x00000200,0x00000400,0x00000800,
	0x00001000,0x00002000,0x00004000,0x00008000,
	0x00010000,0x00020000,0x00040000,0x00080000,
	0x00100000,0x00200000,0x00400000,0x00800000,
	0x01000000,0x02000000,0x04000000,0x08000000,
	0x10000000,0x20000000,0x40000000,0x80000000,
};

	//Hack for peekbits,getbits,suckbits (to prevent lots of duplicate code)
	//   0: PNG: do 12-byte chunk_header removal hack
	// !=0: ZIP: use 64K buffer (olinbuf)
static long zipfilmode;
typedef struct
{
	FILE *fil;    //0:no file open, !=0:open file (either stand-alone or zip)
	long comptyp; //0:raw data (can be ZIP or stand-alone), 8:PKZIP LZ77 *flate
	long seek0;   //0:stand-alone file, !=0: start of zip compressed stream data
	long compleng;//Global variable for compression FIFO
	long comptell;//Global variable for compression FIFO
	long leng;    //Uncompressed file size (bytes)
	long pos;     //Current uncompressed relative file position (0<=pos<=leng)
	long endpos;  //Temp global variable for kzread
	long jmpplc;  //Store place where decompression paused
	long i;       //For stand-alone/ZIP comptyp#0, this is like "uncomptell"
					  //For ZIP comptyp#8&btype==0 "<64K store", this saves i state
	long bfinal;  //LZ77 decompression state (for later calls)
} kzfilestate;
static kzfilestate kzfs;

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

long palcol[256] ASMNAME("palcol"), paleng;
unsigned char coltype, bitdepth;

//============================ KPNGILIB begins ===============================

#define PROCESSALPHAHERE 0  //Set to 1 for KPNG, 0 in all other cases

//07/31/2000: KPNG.C first ported to C from READPNG.BAS
//10/11/2000: KPNG.C split into 2 files: KPNG.C and PNGINLIB.C
//11/24/2000: Finished adding support for coltypes 4&6
//03/31/2001: Added support for Adam7-type interlaced images
//Currently, there is no support for:
//   * 16-bit color depth
//   * Some useless ancillary chunks, like: gAMA(gamma) & pHYs(aspect ratio)

	//.PNG specific variables:
static long bakcol = 0xff808080, bakr = 0x80, bakg = 0x80, bakb = 0x80; //this used to be public...
static long gslidew = 0, gslider = 0, xm, xmn[4], xr0, xr1, xplc, yplc, nfplace;
static long clen[320], cclen[19], bitpos, filt, xsiz, ysiz;
static long xsizbpl, ixsiz, ixoff, iyoff, ixstp, iystp, intlac, nbpl, trnsrgb ASMNAME("trnsrgb");
static long ccind[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
static long hxbit[59][2], ibuf0[288], nbuf0[32], ibuf1[32], nbuf1[32];
static const unsigned char *filptr;
static unsigned char slidebuf[32768], opixbuf0[4], opixbuf1[4];
static unsigned char pnginited = 0, olinbuf[65536] ASMNAME("olinbuf"); //WARNING:max xres is: 65536/bpp-1
static long gotcmov = -2, abstab10[1024] ASMNAME("abstab10");

	//Variables to speed up dynamic Huffman decoding:
#define LOGQHUFSIZ0 9
#define LOGQHUFSIZ1 6
static long qhufval0[1<<LOGQHUFSIZ0], qhufval1[1<<LOGQHUFSIZ1];
static unsigned char qhufbit0[1<<LOGQHUFSIZ0], qhufbit1[1<<LOGQHUFSIZ1];

#if defined(NOASM)

static inline unsigned long bswap (unsigned long a)
{
	return(((a&0xff0000)>>8) + ((a&0xff00)<<8) + (a<<24) + (a>>24));
}

static inline long bitrev (long b, long c)
{
	long i, j;
	for(i=1,j=0,c=(1<<c);i<c;i+=i) { j += j; if (b&i) j++; }
	return(j);
}

static inline long testflag (long c) { return(0); }

static inline void cpuid(long a, long *s) {}

#elif defined(__WATCOMC__)

long bswap (long);
#pragma aux bswap =\
	".586"\
	"bswap eax"\
	parm [eax]\
	modify nomemory exact [eax]\
	value [eax]

long bitrev (long, long);
#pragma aux bitrev =\
	"xor eax, eax"\
	"beg: shr ebx, 1"\
	"adc eax, eax"\
	"dec ecx"\
	"jnz short beg"\
	parm [ebx][ecx]\
	modify nomemory exact [eax ebx ecx]\
	value [eax]

long testflag (long);
#pragma aux testflag =\
	"pushfd"\
	"pop eax"\
	"mov ebx, eax"\
	"xor eax, ecx"\
	"push eax"\
	"popfd"\
	"pushfd"\
	"pop eax"\
	"xor eax, ebx"\
	"mov eax, 1"\
	"jne menostinx"\
	"xor eax, eax"\
	"menostinx:"\
	parm nomemory [ecx]\
	modify exact [eax ebx]\
	value [eax]

void cpuid (long, long *);
#pragma aux cpuid =\
	".586"\
	"cpuid"\
	"mov dword ptr [esi], eax"\
	"mov dword ptr [esi+4], ebx"\
	"mov dword ptr [esi+8], ecx"\
	"mov dword ptr [esi+12], edx"\
	parm [eax][esi]\
	modify exact [eax ebx ecx edx]\
	value

#elif defined(_MSC_VER)

static _inline unsigned long bswap (unsigned long a)
{
	_asm
	{
		mov eax, a
		bswap eax
	}
}

static _inline long bitrev (long b, long c)
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

static _inline long testflag (long c)
{
	_asm
	{
		mov ecx, c
		pushfd
		pop eax
		mov edx, eax
		xor eax, ecx
		push eax
		popfd
		pushfd
		pop eax
		xor eax, edx
		mov eax, 1
		jne menostinx
		xor eax, eax
		menostinx:
	}
}

static _inline void cpuid (long a, long *s)
{
	_asm
	{
		push ebx
		push esi
		mov eax, a
		cpuid
		mov esi, s
		mov dword ptr [esi+0], eax
		mov dword ptr [esi+4], ebx
		mov dword ptr [esi+8], ecx
		mov dword ptr [esi+12], edx
		pop esi
		pop ebx
	}
}

#elif defined(__GNUC__) && defined(__i386__)

static inline unsigned long bswap (unsigned long a)
{
	__asm__ __volatile__ ("bswap %0" : "+r" (a) : : "cc" );
	return a;
}

static inline long bitrev (long b, long c)
{
	long a;
	__asm__ __volatile__ (
		"xorl %%eax, %%eax\n\t0:\n\tshrl $1, %%ebx\n\tadcl %%eax, %%eax\n\tsubl $1, %%ecx\n\tjnz 0b"
		: "+a" (a), "+b" (b), "+c" (c) : : "cc");
	return a;
}

static inline long testflag (long c)
{
	long a;
	__asm__ __volatile__ (
		"pushf\n\tpopl %%eax\n\tmovl %%eax, %%ebx\n\txorl %%ecx, %%eax\n\tpushl %%eax\n\t"
		"popf\n\tpushf\n\tpopl %%eax\n\txorl %%ebx, %%eax\n\tmovl $1, %%eax\n\tjne 0f\n\t"
		"xorl %%eax, %%eax\n\t0:"
		: "=a" (a) : "c" (c) : "ebx","cc" );
	return a;
}

static inline void cpuid (long a, long *s)
{
	__asm__ __volatile__ (
		"cpuid\n\tmovl %%eax, (%%esi)\n\tmovl %%ebx, 4(%%esi)\n\t"
		"movl %%ecx, 8(%%esi)\n\tmovl %%edx, 12(%%esi)"
		: "+a" (a) : "S" (s) : "ebx","ecx","edx","memory","cc");
}

#else

#error Unsupported compiler or architecture.

#endif

	//Bit numbers of return value:
	//0:FPU, 4:RDTSC, 15:CMOV, 22:MMX+, 23:MMX, 25:SSE, 26:SSE2, 30:3DNow!+, 31:3DNow!
static long getcputype ()
{
	long i, cpb[4], cpid[4];
	if (!testflag(0x200000)) return(0);
	cpuid(0,cpid); if (!cpid[0]) return(0);
	cpuid(1,cpb); i = (cpb[3]&~((1<<22)|(1<<30)|(1<<31)));
	cpuid(0x80000000,cpb);
	if (((unsigned long)cpb[0]) > 0x8000000)
	{
		cpuid(0x80000001,cpb);
		i |= (cpb[3]&(1<<31));
		if (!((cpid[1]^0x68747541)|(cpid[3]^0x69746e65)|(cpid[2]^0x444d4163))) //AuthenticAMD
			i |= (cpb[3]&((1<<22)|(1<<30)));
	}
	if (i&(1<<25)) i |= (1<<22); //SSE implies MMX+ support
	return(i);
}

static unsigned char fakebuf[8], *nfilptr;
static long nbitpos;
static void suckbitsnextblock ()
{
	long n;

	if (!zipfilmode)
	{
		if (!nfilptr)
		{     //|===|===|crc|lng|typ|===|===|
				//        \  fakebuf: /
				//          |===|===|
				//----x     O---x     O--------
			nbitpos = LSWAPIL(*(long *)&filptr[8]);
			nfilptr = (unsigned char *)&filptr[nbitpos+12];
			*(long *)&fakebuf[0] = *(long *)&filptr[0]; //Copy last dword of IDAT chunk
			if (*(long *)&filptr[12] == LSWAPIB(0x54414449)) //Copy 1st dword of next IDAT chunk
				*(long *)&fakebuf[4] = *(long *)&filptr[16];
			filptr = &fakebuf[4]; bitpos -= 32;
		}
		else
		{
			filptr = nfilptr; nfilptr = 0;
			bitpos -= ((nbitpos-4)<<3);
		}
		//if (n_from_suckbits < 4) will it crash?
	}
	else
	{
			//NOTE: should only read bytes inside compsize, not 64K!!! :/
		*(long *)&olinbuf[0] = *(long *)&olinbuf[sizeof(olinbuf)-4];
		n = min((unsigned)(kzfs.compleng-kzfs.comptell),sizeof(olinbuf)-4);
		fread(&olinbuf[4],n,1,kzfs.fil);
		kzfs.comptell += n;
		bitpos -= ((sizeof(olinbuf)-4)<<3);
	}
}

static _inline long peekbits (long n) { return((LSWAPIB(*(long *)&filptr[bitpos>>3])>>(bitpos&7))&pow2mask[n]); }
static _inline void suckbits (long n) { bitpos += n; if (bitpos >= 0) suckbitsnextblock(); }
static _inline long getbits (long n) { long i = peekbits(n); suckbits(n); return(i); }

static long hufgetsym (long *hitab, long *hbmax)
{
	long v, n;

	v = n = 0;
	do { v = (v<<1)+getbits(1)+hbmax[n]-hbmax[n+1]; n++; } while (v >= 0);
	return(hitab[hbmax[n]+v]);
}

	//This did not result in a speed-up on P4-3.6Ghz (02/22/2005)
//static long hufgetsym_skipb (long *hitab, long *hbmax, long n, long addit)
//{
//   long v;
//
//   v = bitrev(getbits(n),n)+addit;
//   do { v = (v<<1)+getbits(1)+hbmax[n]-hbmax[n+1]; n++; } while (v >= 0);
//   return(hitab[hbmax[n]+v]);
//}

static void qhufgencode (long *hitab, long *hbmax, long *qhval, unsigned char *qhbit, long numbits)
{
	long i, j, k, n, r;

		//r is the bit reverse of i. Ex: if: i = 1011100111, r = 1110011101
	i = r = 0;
	for(n=1;n<=numbits;n++)
		for(k=hbmax[n-1];k<hbmax[n];k++)
			for(j=i+pow2mask[numbits-n];i<=j;i++)
			{
				r = bitrev(i,numbits);
				qhval[r] = hitab[k];
				qhbit[r] = (char)n;
			}
	for(j=pow2mask[numbits];i<=j;i++)
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
	//return(k);
}

	//inbuf[inum] : Bit length of each symbol
	//inum        : Number of indices
	//hitab[inum] : Indices from size-ordered list to original symbol
	//hbmax[0-31] : Highest index (+1) of n-bit symbol
static void hufgencode (long *inbuf, long inum, long *hitab, long *hbmax)
{
	long i, tbuf[31];

	for(i=30;i;i--) tbuf[i] = 0;
	for(i=inum-1;i>=0;i--) tbuf[inbuf[i]]++;
	tbuf[0] = hbmax[0] = 0; //Hack to remove symbols of length 0?
	for(i=0;i<31;i++) hbmax[i+1] = hbmax[i]+tbuf[i];
	for(i=0;i<inum;i++) if (inbuf[i]) hitab[hbmax[inbuf[i]]++] = i;
}

static long initpass () //Interlaced images have 7 "passes", non-interlaced have 1
{
	long i, j, k;

	do
	{
		i = (intlac<<2);
		ixoff = ((0x04020100>>i)&15);
		iyoff = ((0x00402010>>i)&15);
		if (((ixoff >= xsiz) || (iyoff >= ysiz)) && (intlac >= 2)) { i = -1; intlac--; }
	} while (i < 0);
	j = ((0x33221100>>i)&15); ixstp = (1<<j);
	k = ((0x33322110>>i)&15); iystp = (1<<k);

		//xsiz=12      0123456789ab
		//j=3,ixoff=0  0       1       ((12+(1<<3)-1 - 0)>>3) = 2
		//j=3,ixoff=4      2           ((12+(1<<3)-1 - 4)>>3) = 1
		//j=2,ixoff=2    3   4   5     ((12+(1<<2)-1 - 2)>>2) = 3
		//j=1,ixoff=1   6 7 8 9 a b    ((12+(1<<1)-1 - 1)>>1) = 6
	ixsiz = ((xsiz+ixstp-1-ixoff)>>j); //It's confusing! See the above example.
	nbpl = (bytesperline<<k);

		//Initialize this to make filters fast:
	xsizbpl = ((0x04021301>>(coltype<<2))&15)*ixsiz;
	switch (bitdepth)
	{
		case 1: xsizbpl = ((xsizbpl+7)>>3); break;
		case 2: xsizbpl = ((xsizbpl+3)>>2); break;
		case 4: xsizbpl = ((xsizbpl+1)>>1); break;
	}

	memset(olinbuf,0,(xsizbpl+1)*sizeof(olinbuf[0]));
	*(long *)&opixbuf0[0] = *(long *)&opixbuf1[0] = 0;
	xplc = xsizbpl; yplc = globyoffs+iyoff; xm = 0; filt = -1;

	i = globxoffs+ixoff; i = (((-(i>=0))|(ixstp-1))&i);
	k = (((-(yplc>=0))|(iystp-1))&yplc);
	nfplace = k*bytesperline + (i<<2) + frameplace;

		//Precalculate x-clipping to screen borders (speeds up putbuf)
		//Equation: (0 <= xr <= ixsiz) && (0 <= xr*ixstp+globxoffs+ixoff <= xres)
	xr0 = max((-globxoffs-ixoff+(1<<j)-1)>>j,0);
	xr1 = min((xres-globxoffs-ixoff+(1<<j)-1)>>j,ixsiz);
	xr0 = ixsiz-xr0;
	xr1 = ixsiz-xr1;

		  if (coltype == 4) { xr0 = xr0*2;   xr1 = xr1*2;   }
	else if (coltype == 2) { xr0 = xr0*3-2; xr1 = xr1*3-2; }
	else if (coltype == 6) { xr0 = xr0*4-2; xr1 = xr1*4-2; }
	else
	{
		switch(bitdepth)
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
	return(0);
}

static long Paeth (long a, long b, long c)
{
	long pa, pb, pc;

	pa = b-c; pb = a-c; pc = labs(pa+pb); pa = labs(pa); pb = labs(pb);
	if ((pa <= pb) && (pa <= pc)) return(a);
	if (pb <= pc) return(b); else return(c);
}

#if defined(NOASM)

static inline long Paeth686 (long a, long b, long c)
{
	return(Paeth(a,b,c));
}

static inline void rgbhlineasm (long x, long xr1, long p, long ixstp)
{
	long i;
	if (!trnsrgb)
	{
		for(;x>xr1;p+=ixstp,x-=3) *(long *)p = (*(long *)&olinbuf[x])|LSWAPIB(0xff000000);
		return;
	}
	for(;x>xr1;p+=ixstp,x-=3)
	{
		i = (*(long *)&olinbuf[x])|LSWAPIB(0xff000000);
		if (i == trnsrgb) i &= LSWAPIB(0xffffff);
		*(long *)p = i;
	}
}

static inline void pal8hlineasm (long x, long xr1, long p, long ixstp)
{
	for(;x>xr1;p+=ixstp,x--) *(long *)p = palcol[olinbuf[x]];
}

#elif defined(__WATCOMC__)

	//NOTE: cmov now has correctly ordered registers (thx to bug fix in 11.0c!)
long Paeth686 (long, long, long);
#pragma aux Paeth686 =\
	".686"\
	"mov edx, ecx"\
	"sub edx, eax"\
	"sub edx, ebx"\
	"lea edx, abstab10[edx*4+2048]"\
	"mov esi, [ebx*4+edx]"\
	"mov edi, [ecx*4+edx]"\
	"cmp edi, esi"\
	"cmovge edi, esi"\
	"cmovge ecx, ebx"\
	"cmp edi, [eax*4+edx]"\
	"cmovge ecx, eax"\
	parm nomemory [eax][ebx][ecx]\
	modify exact [ecx edx esi edi]\
	value [ecx]

	//Note: "cmove eax,?" may be faster than "jne ?:and eax,?" but who cares
void rgbhlineasm (long, long, long, long);
#pragma aux rgbhlineasm =\
	"sub ecx, edx"\
	"jle short endit"\
	"add edx, offset olinbuf"\
	"cmp dword ptr trnsrgb, 0"\
	"jz short begit2"\
	"begit: mov eax, dword ptr [ecx+edx]"\
	"or eax, 0xff000000"\
	"cmp eax, dword ptr trnsrgb"\
	"jne short skipit"\
	"and eax, 0xffffff"\
	"skipit: sub ecx, 3"\
	"mov [edi], eax"\
	"lea edi, [edi+ebx]"\
	"jnz short begit"\
	"jmp short endit"\
	"begit2: mov eax, dword ptr [ecx+edx]"\
	"or eax, 0xff000000"\
	"sub ecx, 3"\
	"mov [edi], eax"\
	"lea edi, [edi+ebx]"\
	"jnz short begit2"\
	"endit:"\
	parm [ecx][edx][edi][ebx]\
	modify exact [eax ecx edi]\
	value

void pal8hlineasm (long, long, long, long);
#pragma aux pal8hlineasm =\
	"sub ecx, edx"\
	"jle short endit"\
	"add edx, offset olinbuf"\
	"begit: movzx eax, byte ptr [ecx+edx]"\
	"mov eax, dword ptr palcol[eax*4]"\
	"dec ecx"\
	"mov [edi], eax"\
	"lea edi, [edi+ebx]"\
	"jnz short begit"\
	"endit:"\
	parm [ecx][edx][edi][ebx]\
	modify exact [eax ecx edi]\
	value

#elif defined(_MSC_VER)

static _inline long Paeth686 (long a, long b, long c)
{
	_asm
	{
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
	}
}

static _inline void rgbhlineasm (long c, long d, long t, long b)
{
	_asm
	{
		mov ecx, c
		mov edx, d
		mov edi, t
		mov ebx, b
		sub ecx, edx
		jle short endit
		add edx, offset olinbuf
		cmp dword ptr trnsrgb, 0
		jz short begit2
		begit: mov eax, dword ptr [ecx+edx]
		or eax, 0xff000000
		cmp eax, dword ptr trnsrgb
		jne short skipit
		and eax, 0xffffff
		skipit: sub ecx, 3
		mov [edi], eax
		lea edi, [edi+ebx]
		jnz short begit
		jmp short endit
		begit2: mov eax, dword ptr [ecx+edx]
		or eax, 0xff000000
		sub ecx, 3
		mov [edi], eax
		lea edi, [edi+ebx]
		jnz short begit2
		endit:
	}
}

static _inline void pal8hlineasm (long c, long d, long t, long b)
{
	_asm
	{
		mov ecx, c
		mov edx, d
		mov edi, t
		mov ebx, b
		sub ecx, edx
		jle short endit
		add edx, offset olinbuf
		begit: movzx eax, byte ptr [ecx+edx]
		mov eax, dword ptr palcol[eax*4]
		sub ecx, 1
		mov [edi], eax
		lea edi, [edi+ebx]
		jnz short begit
		endit:
	}
}

#elif defined(__GNUC__) && defined(__i386__)

static inline long Paeth686 (long a, long b, long c)
{
	__asm__ __volatile__ (
		"movl %%ecx, %%edx\n\tsubl %%eax, %%edx\n\tsubl %%ebx, %%edx\n\t"
		"leal (abstab10+2048)(,%%edx,4), %%edx\n\t"
		"movl (%%edx,%%ebx,4), %%esi\n\tmovl (%%edx,%%ecx,4), %%edi\n\t"
		"cmpl %%esi, %%edi\n\tcmovgel %%esi, %%edi\n\tcmovgel %%ebx, %%ecx\n\t"
		"cmpl (%%edx,%%eax,4), %%edi\n\tcmovgel %%eax, %%ecx"
		: "+c" (c) : "a" (a), "b" (b) : "esi","edi","memory","cc"
		);
	return c;
}

	//Note: "cmove eax,?" may be faster than "jne ?:and eax,?" but who cares
static inline void rgbhlineasm (long c, long d, long t, long b)
{
	__asm__ __volatile__ (
		"subl %%edx, %%ecx\n\tjle 3f\n\taddl $olinbuf, %%edx\n\t"
		"cmpl $0, trnsrgb(,1)\n\tjz 2f\n\t"
		"0: movl (%%ecx,%%edx,1), %%eax\n\torl $0xff000000, %%eax\n\tcmpl trnsrgb(,1), %%eax\n\t"
		"jne 1f\n\tandl $0xffffff, %%eax\n\t"
		"1: subl $3, %%ecx\n\tmovl %%eax, (%%edi)\n\tleal (%%edi,%%ebx,1), %%edi\n\t"
		"jnz 0b\n\tjmp 3f\n\t"
		"2: movl (%%ecx,%%edx,1), %%eax\n\torl $0xff000000, %%eax\n\tsubl $3, %%ecx\n\t"
		"movl %%eax, (%%edi)\n\tleal (%%edi,%%ebx,1), %%edi\n\tjnz 2b\n\t"
		"3:"
		: "+c" (c), "+D" (t) : "d" (d), "b" (b) : "eax","memory","cc"
		);
}

static inline void pal8hlineasm (long c, long d, long t, long b)
{
	__asm__ __volatile__ (
		"subl %%edx, %%ecx\n\tjle 1f\n\taddl $olinbuf, %%edx\n\t"
		"0: movzbl (%%ecx,%%edx,1), %%eax\n\tmovl palcol(,%%eax,4), %%eax\n\t"
		"subl $1, %%ecx\n\tmovl %%eax, (%%edi)\n\tleal (%%edi,%%ebx,1), %%edi\n\tjnz 0b\n\t"
		"1:"
		: "+c" (c), "+D" (t) : "d" (d), "b" (b) : "eax","memory","cc"
		);
}

#else

#error Unsupported compiler or architecture.

#endif

static void putbuf (const unsigned char *buf, long leng)
{
	long i, x, p;

	if (filt < 0)
	{
		if (leng <= 0) return;
		filt = buf[0]; if (filt == gotcmov) filt = 5;
		i = 1;
	} else i = 0;

	while (i < leng)
	{
		x = i+xplc; if (x > leng) x = leng;
		switch (filt)
		{
			case 0:
				while (i < x) { olinbuf[xplc] = buf[i]; xplc--; i++; }
				break;
			case 1:
				while (i < x)
				{
					olinbuf[xplc] = (opixbuf1[xm] += buf[i]);
					xm = xmn[xm]; xplc--; i++;
				}
				break;
			case 2:
				while (i < x) { olinbuf[xplc] += buf[i]; xplc--; i++; }
				break;
			case 3:
				while (i < x)
				{
					opixbuf1[xm] = olinbuf[xplc] = ((opixbuf1[xm]+olinbuf[xplc])>>1)+buf[i];
					xm = xmn[xm]; xplc--; i++;
				}
				break;
			case 4:
				while (i < x)
				{
					opixbuf1[xm] = (char)(Paeth(opixbuf1[xm],olinbuf[xplc],opixbuf0[xm])+buf[i]);
					opixbuf0[xm] = olinbuf[xplc];
					olinbuf[xplc] = opixbuf1[xm];
					xm = xmn[xm]; xplc--; i++;
				}
				break;
			case 5: //Special hack for Paeth686 (Doesn't have to be case 5)
				while (i < x)
				{
					opixbuf1[xm] = (char)(Paeth686(opixbuf1[xm],olinbuf[xplc],opixbuf0[xm])+buf[i]);
					opixbuf0[xm] = olinbuf[xplc];
					olinbuf[xplc] = opixbuf1[xm];
					xm = xmn[xm]; xplc--; i++;
				}
				break;
		}

		if (xplc > 0) return;

			//Draw line!
		if ((unsigned long)yplc < (unsigned long)yres)
		{
			x = xr0; p = nfplace;
			switch (coltype)
			{
				case 2:
					rgbhlineasm(x,xr1,p,ixstp);
					break;
				case 4:
					for(;x>xr1;p+=ixstp,x-=2)
					{
#if (PROCESSALPHAHERE == 1)
							//Enable this code to process alpha right here!
						if (olinbuf[x-1] == 255) { *(long *)p = palcol[olinbuf[x]]; continue; }
						if (!olinbuf[x-1]) { *(long *)p = bakcol; continue; }
							//I do >>8, but theoretically should be: /255
						*(char *)(p) = *(char *)(p+1) = *(char *)(p+2) = *(char *)(p+3) =
							(((((long)olinbuf[x])-bakr)*(long)olinbuf[x-1])>>8) + bakr;
#else
						*(long *)p = (palcol[olinbuf[x]]&LSWAPIB(0xffffff))|LSWAPIL((long)olinbuf[x-1]);
#endif
					}
					break;
				case 6:
					for(;x>xr1;p+=ixstp,x-=4)
					{
#if (PROCESSALPHAHERE == 1)
							//Enable this code to process alpha right here!
						if (olinbuf[x-1] == 255) { *(long *)p = *(long *)&olinbuf[x]; continue; }
						if (!olinbuf[x-1]) { *(long *)p = bakcol; continue; }
							//I do >>8, but theoretically should be: /255
						*(char *)(p  ) = (((((long)olinbuf[x  ])-bakr)*(long)olinbuf[x-1])>>8) + bakr;
						*(char *)(p+1) = (((((long)olinbuf[x+1])-bakg)*(long)olinbuf[x-1])>>8) + bakg;
						*(char *)(p+2) = (((((long)olinbuf[x+2])-bakb)*(long)olinbuf[x-1])>>8) + bakb;
#else
						*(char *)(p  ) = olinbuf[x  ]; //R
						*(char *)(p+1) = olinbuf[x+1]; //G
						*(char *)(p+2) = olinbuf[x+2]; //B
						*(char *)(p+3) = olinbuf[x-1]; //A
#endif
					}
					break;
				default:
					switch(bitdepth)
					{
						case 1: for(;x>xr1;p+=ixstp,x-- ) *(long *)p = palcol[olinbuf[x>>3]>>(x&7)]; break;
						case 2: for(;x>xr1;p+=ixstp,x-=2) *(long *)p = palcol[olinbuf[x>>3]>>(x&6)]; break;
						case 4: for(;x>xr1;p+=ixstp,x-=4) *(long *)p = palcol[olinbuf[x>>3]>>(x&4)]; break;
						case 8: pal8hlineasm(x,xr1,p,ixstp); break; //for(;x>xr1;p+=ixstp,x--) *(long *)p = palcol[olinbuf[x]]; break;
					}
					break;
			}
			nfplace += nbpl;
		}

		*(long *)&opixbuf0[0] = *(long *)&opixbuf1[0] = 0;
		xplc = xsizbpl; yplc += iystp;
		if ((intlac) && (yplc >= globyoffs+ysiz)) { intlac--; initpass(); }
		if (i < leng) { filt = buf[i++]; if (filt == gotcmov) filt = 5; } else filt = -1;
	}
}

static void initpngtables()
{
	long i, j, k;

		//hxbit[0-58][0-1] is a combination of 4 different tables:
		//   1st parameter: [0-29] are distances, [30-58] are lengths
		//   2nd parameter: [0]: extra bits, [1]: base number

	j = 1; k = 0;
	for(i=0;i<30;i++)
	{
		hxbit[i][1] = j; j += (1<<k);
		hxbit[i][0] = k; k += ((i&1) && (i >= 2));
	}
	j = 3; k = 0;
	for(i=257;i<285;i++)
	{
		hxbit[i+30-257][1] = j; j += (1<<k);
		hxbit[i+30-257][0] = k; k += ((!(i&3)) && (i >= 264));
	}
	hxbit[285+30-257][1] = 258; hxbit[285+30-257][0] = 0;

	k = getcputype();
	if (k&(1<<15))
	{
		gotcmov = 4;
		for(i=0;i<512;i++) abstab10[512+i] = abstab10[512-i] = i;
	}
}

static long kpngrend (const char *kfilebuf, long kfilength,
	long daframeplace, long dabytesperline, long daxres, long dayres,
	long daglobxoffs, long daglobyoffs)
{
	long i, j, k, bfinal, btype, hlit, hdist, leng;
	long slidew, slider;
	//long qhuf0v, qhuf1v;

	if (!pnginited) { pnginited = 1; initpngtables(); }

	if ((*(long *)&kfilebuf[0] != LSWAPIB(0x474e5089)) || (*(long *)&kfilebuf[4] != LSWAPIB(0x0a1a0a0d)))
		return(-1); //"Invalid PNG file signature"
	filptr = (unsigned char *)&kfilebuf[8];

	trnsrgb = 0;

	while (1)
	{
		leng = LSWAPIL(*(long *)&filptr[0]); i = *(long *)&filptr[4];
		filptr = &filptr[8];

		if (i == LSWAPIB(0x52444849)) //IHDR (must be first)
		{
			xsiz = LSWAPIL(*(long *)&filptr[0]); if (xsiz <= 0) return(-1);
			ysiz = LSWAPIL(*(long *)&filptr[4]); if (ysiz <= 0) return(-1);
			bitdepth = filptr[8]; if (!((1<<bitdepth)&0x116)) return(-1); //"Bit depth not supported"
			coltype = filptr[9]; if (!((1<<coltype)&0x5d)) return(-1); //"Color type not supported"
			if (filptr[10]) return(-1); //"Only *flate is supported"
			if (filptr[11]) return(-1); //"Filter not supported"
			if (filptr[12] >= 2) return(-1); //"Unsupported interlace type"
			intlac = filptr[12]*7; //0=no interlace/1=Adam7 interlace

				//Save code by making grayscale look like a palette color scheme
			if ((!coltype) || (coltype == 4))
			{
				j = 0xff000000; k = (255 / ((1<<bitdepth)-1))*0x10101;
				paleng = (1<<bitdepth);
				for(i=0;i<paleng;i++,j+=k) palcol[i] = LSWAPIB(j);
			}
		}
		else if (i == LSWAPIB(0x45544c50)) //PLTE (must be before IDAT)
		{
			paleng = leng/3;
			for(i=paleng-1;i>=0;i--) palcol[i] = LSWAPIB((LSWAPIL(*(long *)&filptr[i*3])>>8)|0xff000000);
		}
		else if (i == LSWAPIB(0x44474b62)) //bKGD (must be after PLTE and before IDAT)
		{
			switch(coltype)
			{
				case 0: case 4:
					bakcol = (((long)filptr[0]<<8)+(long)filptr[1])*255/((1<<bitdepth)-1);
					bakcol = bakcol*0x10101+0xff000000; break;
				case 2: case 6:
					if (bitdepth == 8)
						{ bakcol = (((long)filptr[0])<<16)+(((long)filptr[2])<<8)+((long)filptr[4])+0xff000000; }
					else
					{
						for(i=0,bakcol=0xff000000;i<3;i++)
							bakcol += ((((((long)filptr[i<<1])<<8)+((long)filptr[(i<<1)+1]))/257)<<(16-(i<<3)));
					}
					break;
				case 3:
					bakcol = palcol[filptr[0]]; break;
			}
			bakr = ((bakcol>>16)&255);
			bakg = ((bakcol>>8)&255);
			bakb = (bakcol&255);
			bakcol = LSWAPIB(bakcol);
		}
		else if (i == LSWAPIB(0x534e5274)) //tRNS (must be after PLTE and before IDAT)
		{
			switch(coltype)
			{
				case 0:
					if (bitdepth <= 8)
						palcol[(long)filptr[1]] &= LSWAPIB(0xffffff);
					//else {} // /c0 /d16 not yet supported
					break;
				case 2:
					if (bitdepth == 8)
						{ trnsrgb = LSWAPIB((((long)filptr[1])<<16)+(((long)filptr[3])<<8)+((long)filptr[5])+0xff000000); }
					//else {} //WARNING: PNG docs say: MUST compare all 48 bits :(
					break;
				case 3:
					for(i=min(leng,paleng)-1;i>=0;i--)
						palcol[i] &= LSWAPIB((((long)filptr[i])<<24)|0xffffff);
					break;
				default:;
			}
		}
		else if (i == LSWAPIB(0x54414449)) { break; }  //IDAT

		filptr = &filptr[leng+4]; //crc = LSWAPIL(*(long *)&filptr[-4]);
	}

		//Initialize this for the getbits() function
	zipfilmode = 0;
	filptr = &filptr[leng-4]; bitpos = -((leng-4)<<3); nfilptr = 0;
	//if (leng < 4) will it crash?

	frameplace = daframeplace;
	bytesperline = dabytesperline;
	xres = daxres;
	yres = dayres;
	globxoffs = daglobxoffs;
	globyoffs = daglobyoffs;

	switch (coltype)
	{
		case 4: xmn[0] = 1; xmn[1] = 0; break;
		case 2: xmn[0] = 1; xmn[1] = 2; xmn[2] = 0; break;
		case 6: xmn[0] = 1; xmn[1] = 2; xmn[2] = 3; xmn[3] = 0; break;
		default: xmn[0] = 0; break;
	}
	switch (bitdepth)
	{
		case 1: for(i=2;i<256;i++) palcol[i] = palcol[i&1]; break;
		case 2: for(i=4;i<256;i++) palcol[i] = palcol[i&3]; break;
		case 4: for(i=16;i<256;i++) palcol[i] = palcol[i&15]; break;
	}

		//coltype: bitdepth:  format:
		//  0     1,2,4,8,16  I
		//  2           8,16  RGB
		//  3     1,2,4,8     P
		//  4           8,16  IA
		//  6           8,16  RGBA
	xsizbpl = ((0x04021301>>(coltype<<2))&15)*xsiz;
	switch (bitdepth)
	{
		case 1: xsizbpl = ((xsizbpl+7)>>3); break;
		case 2: xsizbpl = ((xsizbpl+3)>>2); break;
		case 4: xsizbpl = ((xsizbpl+1)>>1); break;
	}
		//Tests to see if xsiz > allocated space in olinbuf
		//Note: xsizbpl gets re-written inside initpass()
	if ((xsizbpl+1)*sizeof(olinbuf[0]) > sizeof(olinbuf)) return(-1);

	initpass();

	slidew = 0; slider = 16384;
	suckbits(16); //Actually 2 fields: 8:compmethflags, 8:addflagscheck
	do
	{
		bfinal = getbits(1); btype = getbits(2);
		if (btype == 0)
		{
			  //Raw (uncompressed)
			suckbits((-bitpos)&7);  //Synchronize to start of next byte
			i = getbits(16); if ((getbits(16)^i) != 0xffff) return(-1);
			for(;i;i--)
			{
				if (slidew >= slider)
				{
					putbuf(&slidebuf[(slider-16384)&32767],16384); slider += 16384;
					if ((yplc >= yres) && (intlac < 2)) return(0);
				}
				slidebuf[(slidew++)&32767] = (char)getbits(8);
			}
			continue;
		}
		if (btype == 3) continue;

		if (btype == 1) //Fixed Huffman
		{
			hlit = 288; hdist = 32; i = 0;
			for(;i<144;i++) clen[i] = 8; //Fixed bit sizes (literals)
			for(;i<256;i++) clen[i] = 9; //Fixed bit sizes (literals)
			for(;i<280;i++) clen[i] = 7; //Fixed bit sizes (EOI,lengths)
			for(;i<288;i++) clen[i] = 8; //Fixed bit sizes (lengths)
			for(;i<320;i++) clen[i] = 5; //Fixed bit sizes (distances)
		}
		else  //Dynamic Huffman
		{
			hlit = getbits(5)+257; hdist = getbits(5)+1; j = getbits(4)+4;
			for(i=0;i<j;i++) cclen[ccind[i]] = getbits(3);
			for(;i<19;i++) cclen[ccind[i]] = 0;
			hufgencode(cclen,19,ibuf0,nbuf0);

			j = 0; k = hlit+hdist;
			while (j < k)
			{
				i = hufgetsym(ibuf0,nbuf0);
				if (i < 16) { clen[j++] = i; continue; }
				if (i == 16)
					{ for(i=getbits(2)+3;i;i--) { clen[j] = clen[j-1]; j++; } }
				else
				{
					if (i == 17) i = getbits(3)+3; else i = getbits(7)+11;
					for(;i;i--) clen[j++] = 0;
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
				if ((yplc >= yres) && (intlac < 2)) return(0);
			}

			k = peekbits(LOGQHUFSIZ0);
			if (qhufbit0[k]) { i = qhufval0[k]; suckbits((long)qhufbit0[k]); } else i = hufgetsym(ibuf0,nbuf0);
			//else i = hufgetsym_skipb(ibuf0,nbuf0,LOGQHUFSIZ0,qhuf0v); //hufgetsym_skipb related code

			if (i < 256) { slidebuf[(slidew++)&32767] = (char)i; continue; }
			if (i == 256) break;
			i = getbits(hxbit[i+30-257][0]) + hxbit[i+30-257][1];

			k = peekbits(LOGQHUFSIZ1);
			if (qhufbit1[k]) { j = qhufval1[k]; suckbits((long)qhufbit1[k]); } else j = hufgetsym(ibuf1,nbuf1);
			//else j = hufgetsym_skipb(ibuf1,nbuf1,LOGQHUFSIZ1,qhuf1v); //hufgetsym_skipb related code

			j = getbits(hxbit[j][0]) + hxbit[j][1];
			i += slidew; do { slidebuf[slidew&32767] = slidebuf[(slidew-j)&32767]; slidew++; } while (slidew < i);
		}
	} while (!bfinal);

	slider -= 16384;
	if (!((slider^slidew)&32768))
		putbuf(&slidebuf[slider&32767],slidew-slider);
	else
	{
		putbuf(&slidebuf[slider&32767],(-slider)&32767);
		putbuf(slidebuf,slidew&32767);
	}
	return(0);
}

//============================= KPNGILIB ends ================================
//============================ KPEGILIB begins ===============================

	//11/01/2000: This code was originally from KPEG.C
	//   All non 32-bit color drawing was removed
	//   "Motion" JPG code was removed
	//   A lot of parameters were added to kpeg() for library usage
static long kpeginited = 0;
static long clipxdim, clipydim;

static long hufmaxatbit[8][20], hufvalatbit[8][20], hufcnt[8];
static unsigned char hufnumatbit[8][20], huftable[8][256];
static long hufquickval[8][1024], hufquickbits[8][1024], hufquickcnt[8];
static long quantab[4][64], dct[12][64], lastdc[4], unzig[64], zigit[64]; //dct:10=MAX (says spec);+2 for hacks
static unsigned char gnumcomponents, dcflagor[64];
static long gcompid[4], gcomphsamp[4], gcompvsamp[4], gcompquantab[4], gcomphsampshift[4], gcompvsampshift[4];
static long lnumcomponents, lcompid[4], lcompdc[4], lcompac[4], lcomphsamp[4], lcompvsamp[4], lcompquantab[4];
static long lcomphvsamp0, lcomphsampshift0, lcompvsampshift0;
static long colclip[1024], colclipup8[1024], colclipup16[1024];
static unsigned char pow2char[8] = {1,2,4,8,16,32,64,128};

#if defined(NOASM)

static inline long mulshr24 (long a, long b)
{
	return((long)((((__int64)a)*((__int64)b))>>24));
}

static inline long mulshr32 (long a, long b)
{
	return((long)((((__int64)a)*((__int64)b))>>32));
}

#elif defined(__WATCOMC__)

long mulshr24 (long, long);
#pragma aux mulshr24 =\
	"imul edx"\
	"shrd eax, edx, 24"\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

long mulshr32 (long, long);
#pragma aux mulshr32 =\
	"imul edx"\
	parm nomemory [eax][edx]\
	modify exact [eax edx]\
	value [edx]

#elif defined(_MSC_VER)

static _inline long mulshr24 (long a, long d)
{
	_asm
	{
		mov eax, a
		imul d
		shrd eax, edx, 24
	}
}

static _inline long mulshr32 (long a, long d)
{
	_asm
	{
		mov eax, a
		imul d
		mov eax, edx
	}
}

#elif defined(__GNUC__) && defined(__i386__)

#define mulshr24(a,d) \
	({ long __a=(a), __d=(d); \
		__asm__ __volatile__ ("imull %%edx; shrdl $24, %%edx, %%eax" \
		: "+a" (__a), "+d" (__d) : : "cc"); \
	 __a; })

#define mulshr32(a,d) \
	({ long __a=(a), __d=(d); \
		__asm__ __volatile__ ("imull %%edx" \
		: "+a" (__a), "+d" (__d) : : "cc"); \
	 __d; })

#else

#error Unsupported compiler or architecture.

#endif

static long cosqr16[8] =    //cosqr16[i] = ((cos(PI*i/16)*sqrt(2))<<24);
  {23726566,23270667,21920489,19727919,16777216,13181774,9079764,4628823};
static long crmul[4096], cbmul[4096];

static void initkpeg ()
{
	long i, x, y;

	x = 0;  //Back & forth diagonal pattern (aligning bytes for best compression)
	for(i=0;i<16;i+=2)
	{
		for(y=8-1;y>=0;y--)
			if ((unsigned)(i-y) < (unsigned)8) unzig[x++] = (y<<3)+i-y;
		for(y=0;y<8;y++)
			if ((unsigned)(i+1-y) < (unsigned)8) unzig[x++] = (y<<3)+i+1-y;
	}
	for(i=64-1;i>=0;i--) zigit[unzig[i]] = i;
	for(i=64-1;i>=0;i--) dcflagor[i] = (unsigned char)(1<<(unzig[i]>>3));

	for(i=0;i<128;i++) colclip[i] = i+128;
	for(i=128;i<512;i++) colclip[i] = 255;
	for(i=512;i<896;i++) colclip[i] = 0;
	for(i=896;i<1024;i++) colclip[i] = i-896;
	for(i=0;i<1024;i++)
	{
		colclipup8[i] = (colclip[i]<<8);
		colclipup16[i] = (colclip[i]<<16)+0xff000000; //Hack: set alphas to 255
	}
#if defined(BIGENDIAN)
	for(i=0;i<1024;i++)
	{
		colclip[i] = bswap(colclip[i]);
		colclipup8[i] = bswap(colclipup8[i]);
		colclipup16[i] = bswap(colclipup16[i]);
	}
#endif

	for(i=0;i<2048;i++)
	{
		crmul[(i<<1)+0] = (i-1024)*1470104; //1.402*1048576
		crmul[(i<<1)+1] = (i-1024)*-748830; //-0.71414*1048576
		cbmul[(i<<1)+0] = (i-1024)*-360857; //-0.34414*1048576
		cbmul[(i<<1)+1] = (i-1024)*1858077; //1.772*1048576
	}

	memset((void *)&dct[10][0],0,64*2*sizeof(dct[0][0]));
}

static void huffgetval (long index, long curbits, long num, long *daval, long *dabits)
{
	long b, v, pow2, *hmax;

	hmax = &hufmaxatbit[index][0];
	pow2 = pow2long[curbits-1];
	if (num&pow2) v = 1; else v = 0;
	for(b=1;b<=16;b++)
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

static void invdct8x8 (long *dc, unsigned char dcflag)
{
	#define SQRT2 23726566   //(sqrt(2))<<24
	#define C182 31000253    //(cos(PI/8)*2)<<24
	#define C18S22 43840978  //(cos(PI/8)*sqrt(2)*2)<<24
	#define C38S22 18159528  //(cos(PI*3/8)*sqrt(2)*2)<<24
	long *edc, t0, t1, t2, t3, t4, t5, t6, t7;

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
	} while (dc < edc);
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
	} while (dc < edc);
}

static void yrbrend (long x, long y)
{
	long i, j, ox, oy, xx, yy, xxx, yyy, xxxend, yyyend, yv, cr, cb, p, pp, *odc, *dc, *dc2;

	odc = dct[0]; dc2 = dct[10];
	for(yy=0;yy<(lcompvsamp[0]<<3);yy+=8)
	{
		oy = y+yy+globyoffs; if ((unsigned)oy >= (unsigned)clipydim) { odc += (lcomphsamp[0]<<6); continue; }
		pp = oy*bytesperline + ((x+globxoffs)<<2) + frameplace;
		for(xx=0;xx<(lcomphsamp[0]<<3);xx+=8,odc+=64)
		{
			ox = x+xx+globxoffs; if ((unsigned)ox >= (unsigned)clipxdim) continue;
			p = pp+(xx<<2);
			dc = odc;
			if (lnumcomponents > 1) dc2 = &dct[lcomphvsamp0][((yy>>lcompvsampshift0)<<3)+(xx>>lcomphsampshift0)];
			xxxend = min(clipxdim-ox,8);
			yyyend = min(clipydim-oy,8);
			if ((lcomphsamp[0] == 1) && (xxxend == 8))
			{
				for(yyy=0;yyy<yyyend;yyy++)
				{
					for(xxx=0;xxx<8;xxx++)
					{
						yv = dc[xxx];
						cr = (dc2[xxx+64]>>13)&~1;
						cb = (dc2[xxx   ]>>13)&~1;
						((long *)p)[xxx] = colclipup16[(unsigned)(yv+crmul[cr+2048]               )>>22]+
												  colclipup8[(unsigned)(yv+crmul[cr+2049]+cbmul[cb+2048])>>22]+
													  colclip[(unsigned)(yv+cbmul[cb+2049]               )>>22];
					}
					p += bytesperline;
					dc += 8;
					if (!((yyy+1)&(lcompvsamp[0]-1))) dc2 += 8;
				}
			}
			else if ((lcomphsamp[0] == 2) && (xxxend == 8))
			{
				for(yyy=0;yyy<yyyend;yyy++)
				{
					for(xxx=0;xxx<8;xxx+=2)
					{
						yv = dc[xxx];
						cr = (dc2[(xxx>>1)+64]>>13)&~1;
						cb = (dc2[(xxx>>1)   ]>>13)&~1;
						i = crmul[cr+2049]+cbmul[cb+2048];
						cr = crmul[cr+2048];
						cb = cbmul[cb+2049];
						((long *)p)[xxx] = colclipup16[(unsigned)(yv+cr)>>22]+
												  colclipup8[(unsigned)(yv+ i)>>22]+
													  colclip[(unsigned)(yv+cb)>>22];
						yv = dc[xxx+1];
						((long *)p)[xxx+1] = colclipup16[(unsigned)(yv+cr)>>22]+
													 colclipup8[(unsigned)(yv+ i)>>22]+
														 colclip[(unsigned)(yv+cb)>>22];
					}
					p += bytesperline;
					dc += 8;
					if (!((yyy+1)&(lcompvsamp[0]-1))) dc2 += 8;
				}
			}
			else
			{
				for(yyy=0;yyy<yyyend;yyy++)
				{
					i = 0; j = 1;
					for(xxx=0;xxx<xxxend;xxx++)
					{
						yv = dc[xxx];
						j--;
						if (!j)
						{
							j = lcomphsamp[0];
							cr = (dc2[i+64]>>13)&~1;
							cb = (dc2[i   ]>>13)&~1;
							i++;
						}
						((long *)p)[xxx] = colclipup16[(unsigned)(yv+crmul[cr+2048]               )>>22]+
												  colclipup8[(unsigned)(yv+crmul[cr+2049]+cbmul[cb+2048])>>22]+
													  colclip[(unsigned)(yv+cbmul[cb+2049]               )>>22];
					}
					p += bytesperline;
					dc += 8;
					if (!((yyy+1)&(lcompvsamp[0]-1))) dc2 += 8;
				}
			}
		}
	}
}

static long kpegrend (const char *kfilebuf, long kfilength,
	long daframeplace, long dabytesperline, long daxres, long dayres,
	long daglobxoffs, long daglobyoffs)
{
	long i, j, p, v, leng, xdim, ydim, index, prec, restartcnt, restartinterval;
	long x, y, z, xx, yy, zz, *dc, *dc2, num, curbits, c, daval, dabits, *hqval, *hqbits, hqcnt, *quanptr;
	long passcnt = 0, ghsampmax, gvsampmax, glhsampmax, glvsampmax, glhstep, glvstep;
	long eobrun, Ss, Se, Ah, Al, Alut[2], dctx[12], dcty[12], ldctx[12], ldcty[12], lshx[4], lshy[4];
	short *dctbuf = 0, *dctptr[12], *ldctptr[12], *dcs;
	unsigned char ch, marker, dcflag;
	const unsigned char *kfileptr;

	if (!kpeginited) { kpeginited = 1; initkpeg(); }

	kfileptr = (unsigned char *)kfilebuf;

	if (*(unsigned short *)kfileptr == SSWAPIB(0xd8ff)) kfileptr += 2;
	else return(-1); //"%s is not a JPEG file\n",filename

	restartinterval = 0;
	for(i=0;i<4;i++) lastdc[i] = 0;
	for(i=0;i<8;i++) hufcnt[i] = 0;

	coltype = 0; bitdepth = 8; //For PNGOUT
	do
	{
		ch = *kfileptr++; if (ch != 255) continue;
		do { marker = *kfileptr++; } while (marker == 255);
		if (marker != 0xd9) //Don't read past end of buffer
		{
			leng = ((long)kfileptr[0]<<8)+(long)kfileptr[1]-2;
			kfileptr += 2;
		}
		//printf("fileoffs=%08x, marker=%02x,leng=%d",((long)kfileptr)-((long)kfilebuf)-2,marker,leng);
		switch(marker)
		{
			case 0xc0: case 0xc1: case 0xc2:
					//processit!
				kfileptr++; //numbits = *kfileptr++;

				ydim = SSWAPIL(*(unsigned short *)&kfileptr[0]);
				xdim = SSWAPIL(*(unsigned short *)&kfileptr[2]);
				//printf("%s: %ld / %ld = %ld\n",filename,xdim*ydim*3,kfilength,(xdim*ydim*3)/kfilength);

				frameplace = daframeplace;
				bytesperline = dabytesperline;
				xres = daxres;
				yres = dayres;
				globxoffs = daglobxoffs;
				globyoffs = daglobyoffs;

				gnumcomponents = kfileptr[4];
				kfileptr += 5;
				ghsampmax = gvsampmax = glhsampmax = glvsampmax = 0;
				for(z=0;z<gnumcomponents;z++)
				{
					gcompid[z] = kfileptr[0];
					gcomphsamp[z] = (kfileptr[1]>>4);
					gcompvsamp[z] = (kfileptr[1]&15);
					gcompquantab[z] = kfileptr[2];
					for(i=0;i<8;i++) if (gcomphsamp[z] == pow2long[i]) { gcomphsampshift[z] = i; break; }
					for(i=0;i<8;i++) if (gcompvsamp[z] == pow2long[i]) { gcompvsampshift[z] = i; break; }
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
					memcpy((void *)&hufnumatbit[index][1],(void *)kfileptr,16); kfileptr += 16;
					leng -= 16;

					v = 0; hufcnt[index] = 0;
					hufquickcnt[index] = 0;
					for(i=1;i<=16;i++)
					{
						hufmaxatbit[index][i] = v+hufnumatbit[index][i];
						hufvalatbit[index][i] = hufcnt[index]-v;
						memcpy((void *)&huftable[index][hufcnt[index]],(void *)kfileptr,(long)hufnumatbit[index][i]);
						if (i <= 10)
							for(c=0;c<hufnumatbit[index][i];c++)
								for(j=(1<<(10-i));j>0;j--)
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

				} while (leng > 0);
				break;
			case 0xdb:
				do
				{
					ch = *kfileptr++; leng--;
					index = (ch&15);
					prec = (ch>>4);
					for(z=0;z<64;z++)
					{
						v = (long)(*kfileptr++);
						if (prec) v = (v<<8)+((long)(*kfileptr++));
						v <<= 19;
						if (unzig[z]&7 ) v = mulshr24(v,cosqr16[unzig[z]&7 ]);
						if (unzig[z]>>3) v = mulshr24(v,cosqr16[unzig[z]>>3]);
						if (index) v >>= 6;
						quantab[index][unzig[z]] = v;
					}
					leng -= 64;
					if (prec) leng -= 64;
				} while (leng > 0);
				break;
			case 0xdd:
				restartinterval = SSWAPIL(*(unsigned short *)&kfileptr[0]);
				kfileptr += leng;
				break;
			case 0xda:
				if ((xdim <= 0) || (ydim <= 0)) { if (dctbuf) free(dctbuf); return(-1); }

				lnumcomponents = (long)(*kfileptr++); if (!lnumcomponents) { if (dctbuf) free(dctbuf); return(-1); }
				if (lnumcomponents > 1) coltype = 2;
				for(z=0;z<lnumcomponents;z++)
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
					for(z=zz=0;z<gnumcomponents;z++)
					{
						dctx[z] = ((xdim+(ghsampmax<<3)-1)>>(glhsampmax+3)) << gcomphsampshift[z];
						dcty[z] = ((ydim+(gvsampmax<<3)-1)>>(glvsampmax+3)) << gcompvsampshift[z];
						zz += dctx[z]*dcty[z];
					}
					z = zz*64*sizeof(short);
					dctbuf = (short *)malloc(z); if (!dctbuf) return(-1);
					memset(dctbuf,0,z);
					for(z=zz=0;z<gnumcomponents;z++) { dctptr[z] = &dctbuf[zz*64]; zz += dctx[z]*dcty[z]; }
				}

				glhstep = glvstep = 0x7fffffff;
				for(z=0;z<lnumcomponents;z++)
					for(zz=0;zz<gnumcomponents;zz++)
						if (lcompid[z] == gcompid[zz])
						{
							ldctptr[z] = dctptr[zz];
							ldctx[z] = dctx[zz];
							ldcty[z] = dcty[zz];
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

				clipxdim = min(xdim+globxoffs,xres);
				clipydim = min(ydim+globyoffs,yres);

				if ((max(globxoffs,0) >= xres) || (min(globxoffs+xdim,xres) <= 0) ||
					 (max(globyoffs,0) >= yres) || (min(globyoffs+ydim,yres) <= 0))
					{ if (dctbuf) free(dctbuf); return(0); }

				Alut[0] = (1<<Al); Alut[1] = -Alut[0];

				restartcnt = restartinterval; eobrun = 0; marker = 0xd0;
				num = 0; curbits = 0;
				for(y=0;y<ydim;y+=glvstep)
					for(x=0;x<xdim;x+=glhstep)
					{
						if (kfileptr-4-(unsigned char *)kfilebuf >= kfilength) goto kpegrend_break2; //rest of file is missing!

						if (!dctbuf) dc = dct[0];
						for(c=0;c<lnumcomponents;c++)
						{
							hqval = &hufquickval[lcompac[c]+4][0];
							hqbits = &hufquickbits[lcompac[c]+4][0];
							hqcnt = hufquickcnt[lcompac[c]+4];
							if (!dctbuf) quanptr = &quantab[lcompquantab[c]][0];
							for(yy=0;yy<(lcompvsamp[c]<<3);yy+=8)
								for(xx=0;xx<(lcomphsamp[c]<<3);xx+=8)
								{  //NOTE: Might help to split this code into firstime vs. refinement (!Ah vs. Ah!=0)

									if (dctbuf) dcs = &ldctptr[c][(((y+yy)>>lshy[c])*ldctx[c] + ((x+xx)>>lshx[c]))<<6];

										//Get DC
									if (!Ss)
									{
										while (curbits < 24) //Getbits
										{
											ch = *kfileptr++; if (ch == 255) kfileptr++;
											num = (num<<8)+((long)ch); curbits += 8;
										}

										if (!Ah)
										{
											i = ((num>>(curbits-10))&1023);
											if (i < hufquickcnt[lcompdc[c]])
												  { daval = hufquickval[lcompdc[c]][i]; curbits -= hufquickbits[lcompdc[c]][i]; }
											else { huffgetval(lcompdc[c],curbits,num,&daval,&dabits); curbits -= dabits; }

											if (daval)
											{
												while (curbits < 24) //Getbits
												{
													ch = *kfileptr++; if (ch == 255) kfileptr++;
													num = (num<<8)+((long)ch); curbits += 8;
												}

												curbits -= daval; v = ((unsigned)num >> curbits) & pow2mask[daval];
												if (v <= pow2mask[daval-1]) v -= pow2mask[daval];
												lastdc[c] += v;
											}
											if (!dctbuf) dc[0] = lastdc[c]; else dcs[0] = (short)(lastdc[c]<<Al);
										}
										else if (num&(pow2long[--curbits])) dcs[0] |= ((short)Alut[0]);
									}

										//Get AC
									if (!dctbuf) memset((void *)&dc[1],0,63*4);
									z = max(Ss,1); dcflag = 1;
									if (eobrun <= 0)
									{
										for(;z<=Se;z++)
										{
											while (curbits < 24) //Getbits
											{
												ch = *kfileptr++; if (ch == 255) kfileptr++;
												num = (num<<8)+((long)ch); curbits += 8;
											}
											i = ((num>>(curbits-10))&1023);
											if (i < hqcnt)
												  { daval = hqval[i]; curbits -= hqbits[i]; }
											else { huffgetval(lcompac[c]+4,curbits,num,&daval,&dabits); curbits -= dabits; }

											zz = (daval>>4); daval &= 15;
											if (daval)
											{
												if (Ah)
												{
													//NOTE: Getbits not needed here - buffer should have enough bits
													if (num&(pow2long[--curbits])) daval = Alut[0]; else daval = Alut[1];
												}
											}
											else if (zz < 15)
											{
												eobrun = pow2long[zz];
												if (zz)
												{
													while (curbits < 24) //Getbits
													{
														ch = *kfileptr++; if (ch == 255) kfileptr++;
														num = (num<<8)+((long)ch); curbits += 8;
													}
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
														while (curbits < 24) //Getbits
														{
															ch = *kfileptr++; if (ch == 255) kfileptr++;
															num = (num<<8)+((long)ch); curbits += 8;
														}
														if (num&(pow2long[--curbits])) dcs[z] += ((short)Alut[dcs[z] < 0]);
												  } else if (--zz < 0) break;
												  z++;
												} while (z <= Se);
												if (daval) dcs[z] = daval;
											}
											else
											{
												z += zz; if (z > Se) break;

												while (curbits < 24) //Getbits
												{
													ch = *kfileptr++; if (ch == 255) kfileptr++;
													num = (num<<8)+((long)ch); curbits += 8;
												}
												curbits -= daval; v = ((unsigned)num >> curbits) & pow2mask[daval];
												if (v <= pow2mask[daval-1]) v -= pow2mask[daval];
												dcflag |= dcflagor[z];
												if (!dctbuf) dc[unzig[z]] = v; else dcs[z] = (short)(v<<Al);
											}
										}
									} else if (!Ah) eobrun--;
									if ((Ah) && (eobrun > 0))
									{
										eobrun--;
										for(;z<=Se;z++)
										{
											if (!dcs[z]) continue;
											while (curbits < 24) //Getbits
											{
												ch = *kfileptr++; if (ch == 255) kfileptr++;
												num = (num<<8)+((long)ch); curbits += 8;
											}
											if (num&(pow2long[--curbits])) dcs[z] += ((short)Alut[dcs[z] < 0]);
										}
									}

									if (!dctbuf)
									{
										for(z=64-1;z>=0;z--) dc[z] *= quanptr[z];
										invdct8x8(dc,dcflag); dc += 64;
									}
								}
							}

						if (!dctbuf) yrbrend(x,y);

						restartcnt--;
						if (!restartcnt)
						{
							kfileptr += 1-(curbits>>3); curbits = 0;
							if ((kfileptr[-2] != 255) || (kfileptr[-1] != marker)) kfileptr--;
							marker++; if (marker >= 0xd8) marker = 0xd0;
							restartcnt = restartinterval;
							for(i=0;i<4;i++) lastdc[i] = 0;
							eobrun = 0;
						}
					}
kpegrend_break2:;
				if (!dctbuf) return(0);
				passcnt++; kfileptr -= ((curbits>>3)+1); break;
			case 0xd9: break;
			default: kfileptr += leng; break;
		}
	} while (kfileptr-(unsigned char *)kfilebuf < kfilength);

	if (!dctbuf) return(0);

	lnumcomponents = gnumcomponents;
	for(i=0;i<gnumcomponents;i++)
	{
		lcomphsamp[i] = gcomphsamp[i]; gcomphsamp[i] <<= 3;
		lcompvsamp[i] = gcompvsamp[i]; gcompvsamp[i] <<= 3;
		lshx[i] = glhsampmax-gcomphsampshift[i]+3;
		lshy[i] = glvsampmax-gcompvsampshift[i]+3;
	}
	lcomphsampshift0 = gcomphsampshift[0];
	lcompvsampshift0 = gcompvsampshift[0];
	lcomphvsamp0 = (lcomphsamp[0]<<lcompvsampshift0);
	for(y=0;y<ydim;y+=gcompvsamp[0])
		for(x=0;x<xdim;x+=gcomphsamp[0])
		{
			dc = dct[0];
			for(c=0;c<gnumcomponents;c++)
				for(yy=0;yy<gcompvsamp[c];yy+=8)
					for(xx=0;xx<gcomphsamp[c];xx+=8,dc+=64)
					{
						dcs = &dctptr[c][(((y+yy)>>lshy[c])*dctx[c] + ((x+xx)>>lshx[c]))<<6];
						quanptr = &quantab[gcompquantab[c]][0];
						for(z=0;z<64;z++) dc[z] = ((long)dcs[zigit[z]])*quanptr[z];
						invdct8x8(dc,-1);
					}
			yrbrend(x,y);
		}

	free(dctbuf); return(0);
}

//==============================  KPEGILIB ends ==============================
//================================ GIF begins ================================

static unsigned char suffix[4100], filbuffer[768], tempstack[4096];
static long prefix[4100];

static long kgifrend (const char *kfilebuf, long kfilelength,
	long daframeplace, long dabytesperline, long daxres, long dayres,
	long daglobxoffs, long daglobyoffs)
{
	long i, x, y, xsiz, ysiz, yinc, xend, xspan, yspan, currstr, numbitgoal;
	long lzcols, dat, blocklen, bitcnt, xoff, yoff, transcol, backcol, *lptr;
	char numbits, startnumbits, chunkind, ilacefirst;
	const unsigned char *ptr, *cptr;

	coltype = 3; bitdepth = 8; //For PNGOUT

	if ((kfilebuf[0] != 'G') || (kfilebuf[1] != 'I') ||
		 (kfilebuf[2] != 'F') || (kfilebuf[12])) return(-1);
	paleng = (1<<((kfilebuf[10]&7)+1));
	ptr = (unsigned char *)&kfilebuf[13];
	if (kfilebuf[10]&128) { cptr = ptr; ptr += paleng*3; }
	transcol = -1;
	while ((chunkind = *ptr++) == '!')
	{      //! 0xf9 leng flags ?? ?? transcol
		if (ptr[0] == 0xf9) { if (ptr[2]&1) transcol = (long)(((unsigned char)ptr[5])); }
		ptr++;
		do { i = *ptr++; ptr += i; } while (i);
	}
	if (chunkind != ',') return(-1);

	xoff = SSWAPIB(*(unsigned short *)&ptr[0]);
	yoff = SSWAPIB(*(unsigned short *)&ptr[2]);
	xspan = SSWAPIB(*(unsigned short *)&ptr[4]);
	yspan = SSWAPIB(*(unsigned short *)&ptr[6]); ptr += 9;
	if (ptr[-1]&64) { yinc = 8; ilacefirst = 1; }
				  else { yinc = 1; ilacefirst = 0; }
	if (ptr[-1]&128)
	{
		paleng = (1<<((ptr[-1]&7)+1));
		cptr = ptr; ptr += paleng*3;
	}

	for(i=0;i<paleng;i++)
		palcol[i] = LSWAPIB((((long)cptr[i*3])<<16) + (((long)cptr[i*3+1])<<8) + ((long)cptr[i*3+2]) + 0xff000000);
	for(;i<256;i++) palcol[i] = LSWAPIB(0xff000000);
	if (transcol >= 0) palcol[transcol] &= LSWAPIB(~0xff000000);

		//Handle GIF files with different logical&image sizes or non-0 offsets (added 05/15/2004)
	xsiz = SSWAPIB(*(unsigned short *)&kfilebuf[6]);
	ysiz = SSWAPIB(*(unsigned short *)&kfilebuf[8]);
	if ((xoff != 0) || (yoff != 0) || (xsiz != xspan) || (ysiz != yspan))
	{
		long xx[4], yy[4];
		if (kfilebuf[10]&128) backcol = palcol[(unsigned char)kfilebuf[11]]; else backcol = 0;

			//Fill border to backcol
		xx[0] = max(daglobxoffs           ,     0); yy[0] = max(daglobyoffs           ,     0);
		xx[1] = min(daglobxoffs+xoff      ,daxres); yy[1] = min(daglobyoffs+yoff      ,dayres);
		xx[2] = max(daglobxoffs+xoff+xspan,     0); yy[2] = min(daglobyoffs+yoff+yspan,dayres);
		xx[3] = min(daglobxoffs+xsiz      ,daxres); yy[3] = min(daglobyoffs+ysiz      ,dayres);

		lptr = (long *)(yy[0]*dabytesperline+daframeplace);
		for(y=yy[0];y<yy[1];y++,lptr=(long *)(((long)lptr)+dabytesperline))
			for(x=xx[0];x<xx[3];x++) lptr[x] = backcol;
		for(;y<yy[2];y++,lptr=(long *)(((long)lptr)+dabytesperline))
		{  for(x=xx[0];x<xx[1];x++) lptr[x] = backcol;
			for(x=xx[2];x<xx[3];x++) lptr[x] = backcol;
		}
		for(;y<yy[3];y++,lptr=(long *)(((long)lptr)+dabytesperline))
			for(x=xx[0];x<xx[3];x++) lptr[x] = backcol;

		daglobxoffs += xoff; //Offset bitmap image by extra amount
		daglobyoffs += yoff;
	}

	xspan += daglobxoffs;
	yspan += daglobyoffs;  //UGLY HACK
	y = daglobyoffs;
	if ((unsigned long)y < (unsigned long)dayres)
		{ yoff = y*dabytesperline+daframeplace; x = daglobxoffs; xend = xspan; }
	else
		{ x = daglobxoffs+0x80000000; xend = xspan+0x80000000; }

	lzcols = (1<<(*ptr)); startnumbits = (char)((*ptr)+1); ptr++;
	for(i=lzcols-1;i>=0;i--) { suffix[i] = (char)(prefix[i] = i); }
	currstr = lzcols+2; numbits = startnumbits; numbitgoal = (lzcols<<1);
	blocklen = *ptr++;
	memcpy(filbuffer,ptr,blocklen); ptr += blocklen;
	bitcnt = 0;
	while (1)
	{
		dat = (LSWAPIB(*(long *)&filbuffer[bitcnt>>3])>>(bitcnt&7)) & (numbitgoal-1);
		bitcnt += numbits;
		if ((bitcnt>>3) > blocklen-3)
		{
			*(short *)filbuffer = *(short *)&filbuffer[bitcnt>>3];
			i = blocklen-(bitcnt>>3);
			blocklen = (long)*ptr++;
			memcpy(&filbuffer[i],ptr,blocklen); ptr += blocklen;
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
		for(i=0;dat>=lzcols;dat=prefix[dat]) tempstack[i++] = suffix[dat];
		tempstack[i] = (char)prefix[dat];
		suffix[currstr-1] = suffix[currstr] = (char)dat;

		for(;i>=0;i--)
		{
			if ((unsigned long)x < (unsigned long)daxres)
				*(long *)(yoff+(x<<2)) = palcol[(long)tempstack[i]];
			x++;
			if (x == xend)
			{
				y += yinc;
				if (y >= yspan)
					switch(yinc)
					{
						case 8: if (!ilacefirst) { y = daglobyoffs+2; yinc = 4; break; }
								  ilacefirst = 0; y = daglobyoffs+4; yinc = 8; break;
						case 4: y = daglobyoffs+1; yinc = 2; break;
						case 2: case 1: return(0);
					}
				if ((unsigned long)y < (unsigned long)dayres)
					{ yoff = y*dabytesperline+daframeplace; x = daglobxoffs; xend = xspan; }
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
	//short id = 0x9119, xdim, ydim, xoff, yoff, id = 0x0008;
	//long imagebytes, filler[4];
	//char pal6bit[256][3], image[ydim][xdim];
static long kcelrend (const char *buf, long fleng,
	long daframeplace, long dabytesperline, long daxres, long dayres,
	long daglobxoffs, long daglobyoffs)
{
	long i, x, y, x0, x1, y0, y1, xsiz, ysiz;
	const char *cptr;

	if ((buf[0] != 0x19) || (buf[1] != 0x91) ||
		 (buf[10] != 8) || (buf[11] != 0)) return(-1);

	coltype = 3; bitdepth = 8; paleng = 256; //For PNGOUT

	xsiz = (long)SSWAPIB(*(unsigned short *)&buf[2]); if (xsiz <= 0) return(-1);
	ysiz = (long)SSWAPIB(*(unsigned short *)&buf[4]); if (ysiz <= 0) return(-1);

	cptr = &buf[32];
	for(i=0;i<256;i++)
	{
		palcol[i] = (((long)cptr[0])<<18) +
						(((long)cptr[1])<<10) +
						(((long)cptr[2])<< 2) + LSWAPIB(0xff000000);
		cptr += 3;
	}

	x0 = daglobyoffs; x1 = xsiz+daglobyoffs;
	y0 = daglobyoffs; y1 = ysiz+daglobyoffs;
	for(y=y0;y<y1;y++)
		for(x=x0;x<x1;x++)
		{
			if (((unsigned long)x < (unsigned long)daxres) && ((unsigned long)y < (unsigned long)dayres))
				*(long *)(y*dabytesperline+x*4+daframeplace) = palcol[cptr[0]];
			cptr++;
		}
	return(0);
}

//===============================  CEL ends ==================================
//=============================  TARGA begins ================================

static long ktgarend (const char *header, long fleng,
	long daframeplace, long dabytesperline, long daxres, long dayres,
	long daglobxoffs, long daglobyoffs)
{
	long i, p, x, y, pi, xi, yi, x0, x1, y0, y1, xsiz, ysiz, rlestat, colbyte, pixbyte;
	const unsigned char *fptr, *cptr, *nptr;

		//Ugly and unreliable identification for .TGA!
	if ((fleng < 20) || (header[1]&0xfe)) return(-1);
	if ((header[2] >= 12) || (!((1<<header[2])&0xe0e))) return(-1);
	if ((header[16]&7) || (header[16] == 0) || (header[16] > 32)) return(-1);
	if (header[17]&0xc0) return(-1);

	fptr = (unsigned char *)&header[header[0]+18];
	xsiz = (long)SSWAPIB(*(unsigned short *)&header[12]); if (xsiz <= 0) return(-1);
	ysiz = (long)SSWAPIB(*(unsigned short *)&header[14]); if (ysiz <= 0) return(-1);
	colbyte = ((((long)header[16])+7)>>3);

	if (header[1] == 1)
	{
		pixbyte = ((((long)header[7])+7)>>3);
		cptr = &fptr[-SSWAPIB(*(unsigned short *)&header[3])*pixbyte];
		fptr += SSWAPIB(*(unsigned short *)&header[5])*pixbyte;
	} else pixbyte = colbyte;

	switch(pixbyte) //For PNGOUT
	{
		case 1: coltype = 0; bitdepth = 8; palcol[0] = LSWAPIB(0xff000000);
				  for(i=1;i<256;i++) palcol[i] = palcol[i-1]+LSWAPIB(0x10101); break;
		case 2: case 3: coltype = 2; break;
		case 4: coltype = 6; break;
	}

	if (!(header[17]&16)) { x0 = 0;      x1 = xsiz; xi = 1; }
						  else { x0 = xsiz-1; x1 = -1;   xi =-1; }
	if (header[17]&32) { y0 = 0;      y1 = ysiz; yi = 1; pi = dabytesperline; }
					  else { y0 = ysiz-1; y1 = -1;   yi =-1; pi =-dabytesperline; }
	x0 += daglobxoffs; y0 += daglobyoffs;
	x1 += daglobxoffs; y1 += daglobyoffs;
	if (header[2] < 8) rlestat = -2; else rlestat = -1;

	p = y0*dabytesperline+daframeplace;
	for(y=y0;y!=y1;y+=yi,p+=pi)
		for(x=x0;x!=x1;x+=xi)
		{
			if (rlestat < 128)
			{
				if ((rlestat&127) == 127) { rlestat = (long)fptr[0]; fptr++; }
				if (header[1] == 1)
				{
					if (colbyte == 1) i = fptr[0];
									 else i = (long)SSWAPIB(*(unsigned short *)&fptr[0]);
					nptr = &cptr[i*pixbyte];
				} else nptr = fptr;

				switch(pixbyte)
				{
					case 1: i = palcol[(long)nptr[0]]; break;
					case 2: i = (long)SSWAPIB(*(unsigned short *)&nptr[0]);
						i = LSWAPIB(((i&0x7c00)<<9) + ((i&0x03e0)<<6) + ((i&0x001f)<<3) + 0xff000000);
						break;
					case 3: i = (*(long *)&nptr[0]) | LSWAPIB(0xff000000); break;
					case 4: i = (*(long *)&nptr[0]); break;
				}
				fptr += colbyte;
			}
			if (rlestat >= 0) rlestat--;

			if (((unsigned long)x < (unsigned long)daxres) && ((unsigned long)y < (unsigned long)dayres))
				*(long *)(x*4+p) = i;
		}
	return(0);
}

//==============================  TARGA ends =================================
//==============================  BMP begins =================================
	//TODO: handle BI_RLE8 and BI_RLE4 (compression types 1&2 respectively)
	//                        ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
	//                        ³  0(2): "BM"   ³
	// ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿³ 10(4): rastoff³ ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
	// ³headsiz=12 (OS/2 1.x)³³ 14(4): headsiz³ ³ All new formats: ³
	//ÚÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÁÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÁÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
	//³ 18(2): xsiz                         ³ 18(4): xsiz                                  ³
	//³ 20(2): ysiz                         ³ 22(4): ysiz                                  ³
	//³ 22(2): planes (always 1)            ³ 26(2): planes (always 1)                     ³
	//³ 24(2): cdim (1,4,8,24)              ³ 28(2): cdim (1,4,8,16,24,32)                 ³
	//³ if (cdim < 16)                      ³ 30(4): compression (0,1,2,3!?,4)             ³
	//³    26(rastoff-14-headsiz): pal(bgr) ³ 34(4): (bitmap data size+3)&3                ³
	//³                                     ³ 46(4): N colors (0=2^cdim)                   ³
	//³                                     ³ if (cdim < 16)                               ³
	//³                                     ³    14+headsiz(rastoff-14-headsiz): pal(bgr0) ³
	//ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
	//                      ³ rastoff(?): bitmap data ³
	//                      ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
static long kbmprend (const char *buf, long fleng,
	long daframeplace, long dabytesperline, long daxres, long dayres,
	long daglobxoffs, long daglobyoffs)
{
	long i, j, x, y, x0, x1, y0, y1, rastoff, headsiz, xsiz, ysiz, cdim, comp, cptrinc, *lptr;
	const char *cptr;

	headsiz = *(long *)&buf[14];
	if (headsiz == LSWAPIB(12)) //OS/2 1.x (old format)
	{
		if (*(short *)(&buf[22]) != SSWAPIB(1)) return(-1);
		xsiz = (long)SSWAPIB(*(unsigned short *)&buf[18]);
		ysiz = (long)SSWAPIB(*(unsigned short *)&buf[20]);
		cdim = (long)SSWAPIB(*(unsigned short *)&buf[24]);
		comp = 0;
	}
	else //All newer formats...
	{
		if (*(short *)(&buf[26]) != SSWAPIB(1)) return(-1);
		xsiz = LSWAPIB(*(long *)&buf[18]);
		ysiz = LSWAPIB(*(long *)&buf[22]);
		cdim = (long)SSWAPIB(*(unsigned short *)&buf[28]);
		comp = LSWAPIB(*(long *)&buf[30]);
	}
	if ((xsiz <= 0) || (!ysiz)) return(-1);
		//cdim must be: (1,4,8,16,24,32)
	if (((unsigned long)(cdim-1) >= (unsigned long)32) || (!((1<<cdim)&0x1010113))) return(-1);
	if ((comp != 0) && (comp != 3)) return(-1);

	rastoff = LSWAPIB(*(long *)&buf[10]);

	if (cdim < 16)
	{
		if (cdim == 2) { palcol[0] = 0xffffffff; palcol[1] = LSWAPIB(0xff000000); }
		if (headsiz == LSWAPIB(12)) j = 3; else j = 4;
		for(i=0,cptr=&buf[headsiz+14];cptr<&buf[rastoff];i++,cptr+=j)
			palcol[i] = ((*(long *)&cptr[0])|LSWAPIB(0xff000000));
		coltype = 3; bitdepth = cdim; paleng = i; //For PNGOUT
	}
	else if (!(cdim&15))
	{
		coltype = 2;
		switch(cdim)
		{
			case 16: palcol[0] = 10; palcol[1] = 5; palcol[2] = 0; palcol[3] = 5; palcol[4] = 5; palcol[5] = 5; break;
			case 32: palcol[0] = 16; palcol[1] = 8; palcol[2] = 0; palcol[3] = 8; palcol[4] = 8; palcol[5] = 8; break;
		}
		if (comp == 3) //BI_BITFIELD (RGB masks)
		{
			for(i=0;i<3;i++)
			{
				j = *(long *)&buf[headsiz+(i<<2)+14];
				for(palcol[i]=0;palcol[i]<32;palcol[i]++)
				{
					if (j&1) break;
					j = (((unsigned long)j)>>1);
				}
				for(palcol[i+3]=0;palcol[i+3]<32;palcol[i+3]++)
				{
					if (!(j&1)) break;
					j = (((unsigned long)j)>>1);
				}
			}
		}
		palcol[0] = 24-(palcol[0]+palcol[3]);
		palcol[1] = 16-(palcol[1]+palcol[4]);
		palcol[2] =  8-(palcol[2]+palcol[5]);
		palcol[3] = ((-1<<(24-palcol[3]))&0x00ff0000);
		palcol[4] = ((-1<<(16-palcol[4]))&0x0000ff00);
		palcol[5] = ((-1<<( 8-palcol[5]))&0x000000ff);
	}

	cptrinc = (((xsiz*cdim+31)>>3)&~3); cptr = &buf[rastoff];
	if (ysiz < 0) { ysiz = -ysiz; } else { cptr = &cptr[(ysiz-1)*cptrinc]; cptrinc = -cptrinc; }

	x0 = daglobxoffs; x1 = xsiz+daglobxoffs;
	y0 = daglobyoffs; y1 = ysiz+daglobyoffs;
	if ((x0 >= daxres) || (x1 <= 0) || (y0 >= dayres) || (y1 <= 0)) return(0);
	if (x0 < 0) x0 = 0;
	if (x1 > daxres) x1 = daxres;
	for(y=y0;y<y1;y++,cptr=&cptr[cptrinc])
	{
		if ((unsigned long)y >= (unsigned long)dayres) continue;
		lptr = (long *)(y*dabytesperline-(daglobyoffs<<2)+daframeplace);
		switch(cdim)
		{
			case  1: for(x=x0;x<x1;x++) lptr[x] = palcol[(long)((cptr[x>>3]>>((x&7)^7))&1)]; break;
			case  4: for(x=x0;x<x1;x++) lptr[x] = palcol[(long)((cptr[x>>1]>>(((x&1)^1)<<2))&15)]; break;
			case  8: for(x=x0;x<x1;x++) lptr[x] = palcol[(long)(cptr[x])]; break;
			case 16: for(x=x0;x<x1;x++)
						{
							i = ((long)(*(short *)&cptr[x<<1]));
							lptr[x] = (_lrotl(i,palcol[0])&palcol[3]) +
										 (_lrotl(i,palcol[1])&palcol[4]) +
										 (_lrotl(i,palcol[2])&palcol[5]) + LSWAPIB(0xff000000);
						} break;
			case 24: for(x=x0;x<x1;x++) lptr[x] = ((*(long *)&cptr[x*3])|LSWAPIB(0xff000000)); break;
			case 32: for(x=x0;x<x1;x++)
						{
							i = (*(long *)&cptr[x<<2]);
							lptr[x] = (_lrotl(i,palcol[0])&palcol[3]) +
										 (_lrotl(i,palcol[1])&palcol[4]) +
										 (_lrotl(i,palcol[2])&palcol[5]) + LSWAPIB(0xff000000);
						} break;
		}

	}
	return(0);
}

	//Note: currently only supports 8 and 24 bit PCX
static long kpcxrend (const char *buf, long fleng,
	long daframeplace, long dabytesperline, long daxres, long dayres,
	long daglobxoffs, long daglobyoffs)
{
	long i, j, x, y, p, nplanes, x0, x1, y0, y1, xsiz, ysiz;
	unsigned char c, *cptr;

	if (*(long *)buf != LSWAPIB(0x0801050a)) return(-1);
	xsiz = SSWAPIB(*(short *)&buf[ 8])-SSWAPIB(*(short *)&buf[4])+1; if (xsiz <= 0) return(-1);
	ysiz = SSWAPIB(*(short *)&buf[10])-SSWAPIB(*(short *)&buf[6])+1; if (ysiz <= 0) return(-1);

		//buf[3]: bpp/plane:{1,2,4,8}
		//bpl = *(short *)&buf[66];  //#bytes per scanline. Must be EVEN. May have unused data.
		//nplanes*bpl bytes per scanline; always be decoding break at the end of scan line

	nplanes = buf[65];
	if (nplanes == 1)
	{
		//if (buf[fleng-769] != 12) return(-1); //Some PCX are buggy!
		cptr = (unsigned char *)&buf[fleng-768];
		for(i=0;i<256;i++)
		{
			palcol[i] = (((long)cptr[0])<<16) +
							(((long)cptr[1])<< 8) +
							(((long)cptr[2])    ) + LSWAPIB(0xff000000);
			cptr += 3;
		}
		coltype = 3; bitdepth = 8; paleng = 256; //For PNGOUT
	}
	else if (nplanes == 3)
	{
		coltype = 2;

			//Make sure background is opaque (since 24-bit PCX renderer doesn't do it)
		x0 = max(daglobxoffs,0); x1 = min(xsiz+daglobxoffs,daxres);
		y0 = max(daglobyoffs,0); y1 = min(ysiz+daglobyoffs,dayres);
		i = y0*dabytesperline + daframeplace+3;
		for(y=y0;y<y1;y++,i+=dabytesperline)
			for(x=x0;x<x1;x++) *(char *)((x<<2)+i) = 255;
	}

	x = x0 = daglobxoffs; x1 = xsiz+daglobxoffs;
	y = y0 = daglobyoffs; y1 = ysiz+daglobyoffs;
	cptr = (unsigned char *)&buf[128];
	p = y*dabytesperline+daframeplace;

	j = nplanes-1; daxres <<= 2; x0 <<= 2; x1 <<= 2; x <<= 2; x += j;
	if (nplanes == 1) //8-bit PCX
	{
		do
		{
			c = *cptr++; if (c < 192) i = 1; else { i = (c&63); c = *cptr++; }
			j = palcol[(long)c];
			for(;i;i--)
			{
				if ((unsigned long)y < (unsigned long)dayres)
					if ((unsigned long)x < (unsigned long)daxres) *(long *)(x+p) = j;
				x += 4; if (x >= x1) { x = x0; y++; p += dabytesperline; }
			}
		} while (y < y1);
	}
	else if (nplanes == 3) //24-bit PCX
	{
		do
		{
			c = *cptr++; if (c < 192) i = 1; else { i = (c&63); c = *cptr++; }
			for(;i;i--)
			{
				if ((unsigned long)y < (unsigned long)dayres)
					if ((unsigned long)x < (unsigned long)daxres) *(char *)(x+p) = c;
				x += 4; if (x >= x1) { j--; if (j < 0) { j = 3-1; y++; p += dabytesperline; } x = x0+j; }
			}
		} while (y < y1);
	}

	return(0);
}

//===============================  BMP ends ==================================
//=================== External picture interface begins ======================

void kpgetdim (const char *buf, long leng, long *xsiz, long *ysiz)
{
	long *lptr;
	const unsigned char *cptr;
	unsigned char *ubuf = (unsigned char *)buf;

	(*xsiz) = (*ysiz) = 0; if (leng < 16) return;
	if ((ubuf[0] == 0x89) && (ubuf[1] == 0x50)) //.PNG
	{
		lptr = (long *)buf;
		if ((lptr[0] != LSWAPIB(0x474e5089)) || (lptr[1] != LSWAPIB(0x0a1a0a0d))) return;
		lptr = &lptr[2];
		while (((unsigned long)lptr-(unsigned long)buf) < (unsigned long)(leng-16))
		{
			if (lptr[1] == LSWAPIB(0x52444849)) //IHDR
				{ (*xsiz) = LSWAPIL(lptr[2]); (*ysiz) = LSWAPIL(lptr[3]); break; }
			lptr = (long *)((long)lptr + LSWAPIL(lptr[0]) + 12);
		}
	}
	else if ((ubuf[0] == 0xff) && (ubuf[1] == 0xd8)) //.JPG
	{
		cptr = (unsigned char *)&buf[2];
		while (((unsigned long)cptr-(unsigned long)buf) < (unsigned long)(leng-8))
		{
			if (cptr[0] != 255) { cptr = &cptr[1]; continue; }
			if ((unsigned long)(cptr[1]-0xc0) < 3)
			{
				(*ysiz) = SSWAPIL(*(unsigned short *)&cptr[5]);
				(*xsiz) = SSWAPIL(*(unsigned short *)&cptr[7]);
				break;
			}
			cptr = &cptr[SSWAPIL(*(unsigned short *)&cptr[2])+2];
		}
	}
	else if ((ubuf[0] == 'G') && (ubuf[1] == 'I') && (ubuf[2] == 'F') && (ubuf[12] == 0)) //.GIF
	{
		(*xsiz) = (long)SSWAPIB(*(unsigned short *)&buf[6]);
		(*ysiz) = (long)SSWAPIB(*(unsigned short *)&buf[8]);
	}
	else if ((ubuf[0] == 0x19) && (ubuf[1] == 0x91) && (ubuf[10] == 8) && (ubuf[11] == 0)) //old .CEL/.PIC
	{
		(*xsiz) = (long)SSWAPIB(*(unsigned short *)&buf[2]);
		(*ysiz) = (long)SSWAPIB(*(unsigned short *)&buf[4]);
	}
	else if ((ubuf[0] == 'B') && (ubuf[1] == 'M')) //.BMP
	{
		if (*(long *)(&buf[14]) == LSWAPIB(12)) //OS/2 1.x (old format)
		{
			if (*(short *)(&buf[22]) != SSWAPIB(1)) return;
			(*xsiz) = (long)SSWAPIB(*(unsigned short *)&buf[18]);
			(*ysiz) = (long)SSWAPIB(*(unsigned short *)&buf[20]);
		}
		else //All newer formats...
		{
			if (*(short *)(&buf[26]) != SSWAPIB(1)) return;
			(*xsiz) = LSWAPIB(*(long *)&buf[18]);
			(*ysiz) = LSWAPIB(*(long *)&buf[22]);
		}
	}
	else if (*(long *)ubuf == LSWAPIB(0x0801050a)) //.PCX
	{
		(*xsiz) = SSWAPIB(*(short *)&buf[ 8])-SSWAPIB(*(short *)&buf[4])+1;
		(*ysiz) = SSWAPIB(*(short *)&buf[10])-SSWAPIB(*(short *)&buf[6])+1;
	}
	else
	{     //Unreliable .TGA identification - this MUST be final case!
		if ((leng >= 20) && (!(ubuf[1]&0xfe)))
			if ((ubuf[2] < 12) && ((1<<ubuf[2])&0xe0e))
				if ((!(ubuf[16]&7)) && (ubuf[16] != 0) && (ubuf[16] <= 32))
					if (!(buf[17]&0xc0))
					{
						(*xsiz) = (long)SSWAPIB(*(unsigned short *)&buf[12]);
						(*ysiz) = (long)SSWAPIB(*(unsigned short *)&buf[14]);
					}
	}
}

long kprender (const char *buf, long leng, long frameptr, long bpl,
					long xdim, long ydim, long xoff, long yoff)
{
	unsigned char *ubuf = (unsigned char *)buf;

	if ((ubuf[0] == 0x89) && (ubuf[1] == 0x50)) //.PNG
		return(kpngrend(buf,leng,frameptr,bpl,xdim,ydim,xoff,yoff));

	if ((ubuf[0] == 0xff) && (ubuf[1] == 0xd8)) //.JPG
		return(kpegrend(buf,leng,frameptr,bpl,xdim,ydim,xoff,yoff));

	if ((ubuf[0] == 'G') && (ubuf[1] == 'I') && (ubuf[2] == 'F')) //.GIF
		return(kgifrend(buf,leng,frameptr,bpl,xdim,ydim,xoff,yoff));

	if ((ubuf[0] == 0x19) && (ubuf[1] == 0x91) && (ubuf[10] == 8) && (ubuf[11] == 0)) //old .CEL/.PIC
		return(kcelrend(buf,leng,frameptr,bpl,xdim,ydim,xoff,yoff));

	if ((ubuf[0] == 'B') && (ubuf[1] == 'M')) //.BMP
		return(kbmprend(buf,leng,frameptr,bpl,xdim,ydim,xoff,yoff));

	if (*(long *)ubuf == LSWAPIB(0x0801050a)) //.PCX
		return(kpcxrend(buf,leng,frameptr,bpl,xdim,ydim,xoff,yoff));

		//Unreliable .TGA identification - this MUST be final case!
	if ((leng >= 20) && (!(ubuf[1]&0xfe)))
		if ((ubuf[2] < 12) && ((1<<ubuf[2])&0xe0e))
			if ((!(ubuf[16]&7)) && (ubuf[16] != 0) && (ubuf[16] <= 32))
				if (!(ubuf[17]&0xc0))
					return(ktgarend(buf,leng,frameptr,bpl,xdim,ydim,xoff,yoff));

	return(-1);
}

//==================== External picture interface ends =======================

	//Brute-force case-insensitive, slash-insensitive, * and ? wildcard matcher
	//Given: string i and string j. string j can have wildcards
	//Returns: 1:matches, 0:doesn't match
static long wildmatch (const char *i, const char *j)
{
	const char *k;
	char c0, c1;

	if (!*j) return(1);
	do
	{
		if (*j == '*')
		{
			for(k=i,j++;*k;k++) if (wildmatch(k,j)) return(1);
			continue;
		}
		if (!*i) return(0);
		if (*j == '?') { i++; j++; continue; }
		c0 = *i; if ((c0 >= 'a') && (c0 <= 'z')) c0 -= 32;
		c1 = *j; if ((c1 >= 'a') && (c1 <= 'z')) c1 -= 32;
		if (c0 == '/') c0 = '\\';
		if (c1 == '/') c1 = '\\';
		if (c0 != c1) return(0);
		i++; j++;
	} while (*j);
	return(!*i);
}

	//Same as: stricmp(st0,st1) except: '/' == '\'
static long filnamcmp (const char *st0, const char *st1)
{
	long i;
	char ch0, ch1;

	for(i=0;st0[i];i++)
	{
		ch0 = st0[i]; if ((ch0 >= 'a') && (ch0 <= 'z')) ch0 -= 32;
		ch1 = st1[i]; if ((ch1 >= 'a') && (ch1 <= 'z')) ch1 -= 32;
		if (ch0 == '/') ch0 = '\\';
		if (ch1 == '/') ch1 = '\\';
		if (ch0 != ch1) return(-1);
	}
	if (!st1[i]) return(0);
	return(-1);
}

//===================== ZIP decompression code begins ========================

	//format: (used by kzaddstack/kzopen to cache file name&start info)
	//[char zipnam[?]\0]
	//[next hashindex/-1][next index/-1][zipnam index][zipseek][char filnam[?]\0]
	//[next hashindex/-1][next index/-1][zipnam index][zipseek][char filnam[?]\0]
	//...
	//[char zipnam[?]\0]
	//[next hashindex/-1][next index/-1][zipnam index][zipseek][char filnam[?]\0]
	//[next hashindex/-1][next index/-1][zipnam index][zipseek][char filnam[?]\0]
	//...
#define KZHASHINITSIZE 8192
static char *kzhashbuf = 0;
static long kzhashead[256], kzhashpos, kzlastfnam, kzhashsiz;

static long kzcheckhashsiz (long siz)
{
	long i;

	if (!kzhashbuf) //Initialize hash table on first call
	{
		memset(kzhashead,-1,sizeof(kzhashead));
		kzhashbuf = (char *)malloc(KZHASHINITSIZE); if (!kzhashbuf) return(0);
		kzhashpos = 0; kzlastfnam = -1; kzhashsiz = KZHASHINITSIZE;
	}
	if (kzhashpos+siz > kzhashsiz) //Make sure string fits in kzhashbuf
	{
		i = kzhashsiz; do { i <<= 1; } while (kzhashpos+siz > i);
		kzhashbuf = (char *)realloc(kzhashbuf,i); if (!kzhashbuf) return(0);
		kzhashsiz = i;
	}
	return(1);
}

static long kzcalchash (const char *st)
{
	long i, hashind;
	char ch;

	for(i=0,hashind=0;st[i];i++)
	{
		ch = st[i];
		if ((ch >= 'a') && (ch <= 'z')) ch -= 32;
		if (ch == '/') ch = '\\';
		hashind = (ch - hashind*3);
	}
	return(hashind%(sizeof(kzhashead)/sizeof(kzhashead[0])));
}

static long kzcheckhash (const char *filnam, char **zipnam, long *zipseek)
{
	long i;

	if (!kzhashbuf) return(0);
	if (filnam[0] == '|') filnam++;
	for(i=kzhashead[kzcalchash(filnam)];i>=0;i=(*(long *)&kzhashbuf[i]))
		if (!filnamcmp(filnam,&kzhashbuf[i+16]))
		{
			(*zipnam) = &kzhashbuf[*(long *)&kzhashbuf[i+8]];
			(*zipseek) = *(long *)&kzhashbuf[i+12];
			return(1);
		}
	return(0);
}

void kzuninit ()
{
	if (kzhashbuf) { free(kzhashbuf); kzhashbuf = 0; }
	kzhashpos = kzhashsiz = 0;
}

	//Load ZIP directory into memory (hash) to allow fast access later
long kzaddstack (const char *zipnam)
{
	FILE *fil;
	long i, j, hashind, zipnamoffs, numfiles;
	char tempbuf[260+46];

	fil = fopen(zipnam,"rb"); if (!fil) return(-1);

		//Write ZIP filename to hash
	i = strlen(zipnam)+1; if (!kzcheckhashsiz(i)) { fclose(fil); return(-1); }
	strcpy(&kzhashbuf[kzhashpos],zipnam);
	zipnamoffs = kzhashpos; kzhashpos += i;

	fseek(fil,-22,SEEK_END);
	fread(tempbuf,22,1,fil);
	if (*(long *)&tempbuf[0] == LSWAPIB(0x06054b50)) //Fast way of finding dir info
	{
		numfiles = SSWAPIB(*(short *)&tempbuf[10]);
		fseek(fil,LSWAPIB(*(long *)&tempbuf[16]),SEEK_SET);
	}
	else //Slow way of finding dir info (used when ZIP has junk at end)
	{
		fseek(fil,0,SEEK_SET); numfiles = 0;
		while (1)
		{
			if (!fread(&j,4,1,fil)) { numfiles = -1; break; }
			if (j == LSWAPIB(0x02014b50)) break; //Found central file header :)
			if (j != LSWAPIB(0x04034b50)) { numfiles = -1; break; }
			fread(tempbuf,26,1,fil);
			fseek(fil,LSWAPIB(*(long *)&tempbuf[14]) + SSWAPIB(*(short *)&tempbuf[24]) + SSWAPIB(*(short *)&tempbuf[22]),SEEK_CUR);
			numfiles++;
		}
		if (numfiles < 0) { fclose(fil); return(-1); }
		fseek(fil,-4,SEEK_CUR);
	}
	for(i=0;i<numfiles;i++)
	{
		fread(tempbuf,46,1,fil);
		if (*(long *)&tempbuf[0] != LSWAPIB(0x02014b50)) { fclose(fil); return(0); }

		j = SSWAPIB(*(short *)&tempbuf[28]); //filename length
		fread(&tempbuf[46],j,1,fil);
		tempbuf[j+46] = 0;

			//Write information into hash
		j = strlen(&tempbuf[46])+17; if (!kzcheckhashsiz(j)) { fclose(fil); return(-1); }
		hashind = kzcalchash(&tempbuf[46]);
		*(long *)&kzhashbuf[kzhashpos] = kzhashead[hashind];
		*(long *)&kzhashbuf[kzhashpos+4] = kzlastfnam;
		*(long *)&kzhashbuf[kzhashpos+8] = zipnamoffs;
		*(long *)&kzhashbuf[kzhashpos+12] = LSWAPIB(*(long *)&tempbuf[42]); //zipseek
		strcpy(&kzhashbuf[kzhashpos+16],&tempbuf[46]);
		kzhashead[hashind] = kzhashpos; kzlastfnam = kzhashpos; kzhashpos += j;

		j  = SSWAPIB(*(short *)&tempbuf[30]); //extra field length
		j += SSWAPIB(*(short *)&tempbuf[32]); //file comment length
		fseek(fil,j,SEEK_CUR);
	}
	fclose(fil);
	return(0);
}

long kzopen (const char *filnam)
{
	FILE *fil;
	long zipseek;
	char tempbuf[46+260], *zipnam;

	//kzfs.fil = 0;
	if (filnam[0] != '|')
	{
		kzfs.fil = fopen(filnam,"rb");
		if (kzfs.fil)
		{
			kzfs.comptyp = 0;
			kzfs.seek0 = 0;
			kzfs.leng = filelength(_fileno(kzfs.fil));
			kzfs.pos = 0;
			kzfs.i = 0;
			return((long)kzfs.fil);
		}
	}
	if (kzcheckhash(filnam,&zipnam,&zipseek))
	{
		fil = fopen(zipnam,"rb"); if (!fil) return(0);
		fseek(fil,zipseek,SEEK_SET);
		fread(tempbuf,30,1,fil);
		if (*(long *)&tempbuf[0] != LSWAPIB(0x04034b50)) { fclose(fil); return(0); }
		fseek(fil,SSWAPIB(*(short *)&tempbuf[26])+SSWAPIB(*(short *)&tempbuf[28]),SEEK_CUR);

		kzfs.fil = fil;
		kzfs.comptyp = SSWAPIB(*(short *)&tempbuf[8]);
		kzfs.seek0 = ftell(fil);
		kzfs.leng = LSWAPIB(*(long *)&tempbuf[22]);
		kzfs.pos = 0;
		switch(kzfs.comptyp) //Compression method
		{
			case 0: kzfs.i = 0; return((long)kzfs.fil);
			case 8:
				if (!pnginited) { pnginited = 1; initpngtables(); }
				kzfs.comptell = 0;
				kzfs.compleng = LSWAPIB(*(long *)&tempbuf[18]);

					//WARNING: No file in ZIP can be > 2GB-32K bytes
				gslidew = 0x7fffffff; //Force reload at beginning

				return((long)kzfs.fil);
			default: fclose(kzfs.fil); kzfs.fil = 0; return(0);
		}
	}
	return(0);
}

// --------------------------------------------------------------------------

static long srchstat = -1, wildstpathleng;

#if defined(__DOS__)
static char wildst[260] = "";
static struct find_t findata;
#elif defined(_WIN32)
static char wildst[MAX_PATH] = "";
static HANDLE hfind = INVALID_HANDLE_VALUE;
static WIN32_FIND_DATA findata;
#else
static char wildst[260] = "";
static DIR *hfind = NULL;
static struct dirent *findata = NULL;
#endif

void kzfindfilestart (const char *st)
{
#ifdef _WIN32
	if (hfind != INVALID_HANDLE_VALUE)
		{ FindClose(hfind); hfind = INVALID_HANDLE_VALUE; }
#else
	if (hfind) { closedir(hfind); hfind = NULL; }
#endif
	strcpy(wildst,st);
	srchstat = -3;
}

long kzfindfile (char *filnam)
{
	long i;

	filnam[0] = 0;
	if (srchstat == -3)
	{
		if (!wildst[0]) { srchstat = -1; return(0); }
		do
		{
			srchstat = -2;

				//Extract directory from wildcard string for pre-pending
			wildstpathleng = 0;
			for(i=0;wildst[i];i++)
				if ((wildst[i] == '/') || (wildst[i] == '\\'))
					wildstpathleng = i+1;

			memcpy(filnam,wildst,wildstpathleng);

#if defined(__DOS__)
			if (_dos_findfirst(wildst,_A_SUBDIR,&findata))
				{ if (!kzhashbuf) return(0); srchstat = kzlastfnam; continue; }
			i = wildstpathleng;
			if (findata.attrib&16)
				if ((findata.name[0] == '.') && (!findata.name[1])) continue;
			strcpy(&filnam[i],findata.name);
			if (findata.attrib&16) strcat(&filnam[i],"\\");
#elif defined(_WIN32)
			hfind = FindFirstFile(wildst,&findata);
			if (hfind == INVALID_HANDLE_VALUE)
				{ if (!kzhashbuf) return(0); srchstat = kzlastfnam; continue; }
			if (findata.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) continue;
			i = wildstpathleng;
			if (findata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				if ((findata.cFileName[0] == '.') && (!findata.cFileName[1])) continue;
			strcpy(&filnam[i],findata.cFileName);
			if (findata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) strcat(&filnam[i],"\\");
#else
			if (!hfind)
			{
				char *s = ".";
				if (wildstpathleng > 0) {
					filnam[wildstpathleng] = 0;
					s = filnam;
				}
				hfind = opendir(s);
				if (!hfind) { if (!kzhashbuf) return 0; srchstat = kzlastfnam; continue; }
			}
			break;   // process srchstat == -2
#endif
			return(1);
		} while (0);
	}
	if (srchstat == -2)
		while (1)
		{
			memcpy(filnam,wildst,wildstpathleng);
#if defined(__DOS__)
			if (_dos_findnext(&findata))
				{ if (!kzhashbuf) return(0); srchstat = kzlastfnam; break; }
			i = wildstpathleng;
			if (findata.attrib&16)
				if ((findata.name[0] == '.') && (!findata.name[1])) continue;
			strcpy(&filnam[i],findata.name);
			if (findata.attrib&16) strcat(&filnam[i],"\\");
#elif defined(_WIN32)
			if (!FindNextFile(hfind,&findata))
				{ FindClose(hfind); if (!kzhashbuf) return(0); srchstat = kzlastfnam; break; }
			if (findata.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) continue;
			i = wildstpathleng;
			if (findata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				if ((findata.cFileName[0] == '.') && (!findata.cFileName[1])) continue;
			strcpy(&filnam[i],findata.cFileName);
			if (findata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) strcat(&filnam[i],"\\");
#else
			{
			struct stat st;
			if ((findata = readdir(hfind)) == NULL)
				{ closedir(hfind); hfind = NULL; if (!kzhashbuf) return 0; srchstat = kzlastfnam; break; }
			i = wildstpathleng;
			strcpy(&filnam[i],findata->d_name);
			if (stat(filnam,&st) < 0) continue;
			if (st.st_mode & S_IFDIR)
				{ if (findata->d_name[0] == '.' && !findata->d_name[1]) continue; } //skip .
			else if ((st.st_mode & S_IFREG) || (st.st_mode & S_IFLNK))
				{ if (findata->d_name[0] == '.') continue; } //skip hidden (dot) files
			else continue; //skip devices and fifos and such
			if (!wildmatch(findata->d_name,&wildst[wildstpathleng])) continue;
			if (st.st_mode & S_IFDIR) strcat(&filnam[i],"/");
			}
#endif
			return(1);
		}
	while (srchstat >= 0)
	{
		if (wildmatch(&kzhashbuf[srchstat+16],wildst))
		{
			//strcpy(filnam,&kzhashbuf[srchstat+16]);
			filnam[0] = '|'; strcpy(&filnam[1],&kzhashbuf[srchstat+16]);
			srchstat = *(long *)&kzhashbuf[srchstat+4];
			return(1);
		}
		srchstat = *(long *)&kzhashbuf[srchstat+4];
	}
	return(0);
}

//File searching code (supports inside ZIP files!) How to use this code:
//   char filnam[MAX_PATH];
//   kzfindfilestart("vxl/*.vxl");
//   while (kzfindfile(filnam)) puts(filnam);
//NOTES:
// * Directory names end with '\' or '/' (depending on system)
// * Files inside zip begin with '|'

// --------------------------------------------------------------------------

static char *gzbufptr;
static void putbuf4zip (const unsigned char *buf, long uncomp0, long uncomp1)
{
	long i0, i1;
		//              uncomp0 ... uncomp1
		//  &gzbufptr[kzfs.pos] ... &gzbufptr[kzfs.endpos];
	i0 = max(uncomp0,kzfs.pos);
	i1 = min(uncomp1,kzfs.endpos);
	if (i0 < i1) memcpy(&gzbufptr[i0],&buf[i0-uncomp0],i1-i0);
}

	//returns number of bytes copied
long kzread (void *buffer, long leng)
{
	long i, j, k, bfinal, btype, hlit, hdist;

	if ((!kzfs.fil) || (leng <= 0)) return(0);

	if (kzfs.comptyp == 0)
	{
		if (kzfs.pos != kzfs.i) //Seek only when position changes
			fseek(kzfs.fil,kzfs.seek0+kzfs.pos,SEEK_SET);
		i = min(kzfs.leng-kzfs.pos,leng);
		fread(buffer,i,1,kzfs.fil);
		kzfs.i += i; //kzfs.i is a local copy of ftell(kzfs.fil);
	}
	else if (kzfs.comptyp == 8)
	{
		zipfilmode = 1;

			//Initialize for putbuf4zip
		gzbufptr = (char *)buffer; gzbufptr = &gzbufptr[-kzfs.pos];
		kzfs.endpos = min(kzfs.pos+leng,kzfs.leng);
		if (kzfs.endpos == kzfs.pos) return(0); //Guard against reading 0 length

		if (kzfs.pos < gslidew-32768) // Must go back to start :(
		{
			if (kzfs.comptell) fseek(kzfs.fil,kzfs.seek0,SEEK_SET);

			gslidew = 0; gslider = 16384;
			kzfs.jmpplc = 0;

				//Initialize for suckbits/peekbits/getbits
			kzfs.comptell = min((unsigned)kzfs.compleng,sizeof(olinbuf));
			fread(&olinbuf[0],kzfs.comptell,1,kzfs.fil);
				//Make it re-load when there are < 32 bits left in FIFO
			bitpos = -(((long)sizeof(olinbuf)-4)<<3);
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
			static long ouncomppos = 0, ocomppos = 0;
			if (kzfs.comptell == sizeof(olinbuf)) i = 0;
			else if (kzfs.comptell < kzfs.compleng) i = kzfs.comptell-(sizeof(olinbuf)-4);
			else i = kzfs.comptell-(kzfs.comptell%(sizeof(olinbuf)-4));
			i += ((long)&filptr[bitpos>>3])-((long)(&olinbuf[0]));
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
				i = getbits(16); if ((getbits(16)^i) != 0xffff) return(-1);
				for(;i;i--)
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
					slidebuf[(gslidew++)&32767] = (char)getbits(8);
				}
				continue;
			}
			if (btype == 3) continue;

			if (btype == 1) //Fixed Huffman
			{
				hlit = 288; hdist = 32; i = 0;
				for(;i<144;i++) clen[i] = 8; //Fixed bit sizes (literals)
				for(;i<256;i++) clen[i] = 9; //Fixed bit sizes (literals)
				for(;i<280;i++) clen[i] = 7; //Fixed bit sizes (EOI,lengths)
				for(;i<288;i++) clen[i] = 8; //Fixed bit sizes (lengths)
				for(;i<320;i++) clen[i] = 5; //Fixed bit sizes (distances)
			}
			else  //Dynamic Huffman
			{
				hlit = getbits(5)+257; hdist = getbits(5)+1; j = getbits(4)+4;
				for(i=0;i<j;i++) cclen[ccind[i]] = getbits(3);
				for(;i<19;i++) cclen[ccind[i]] = 0;
				hufgencode(cclen,19,ibuf0,nbuf0);

				j = 0; k = hlit+hdist;
				while (j < k)
				{
					i = hufgetsym(ibuf0,nbuf0);
					if (i < 16) { clen[j++] = i; continue; }
					if (i == 16)
						{ for(i=getbits(2)+3;i;i--) { clen[j] = clen[j-1]; j++; } }
					else
					{
						if (i == 17) i = getbits(3)+3; else i = getbits(7)+11;
						for(;i;i--) clen[j++] = 0;
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
				if (qhufbit0[k]) { i = qhufval0[k]; suckbits((long)qhufbit0[k]); }
				else i = hufgetsym(ibuf0,nbuf0);

				if (i < 256) { slidebuf[(gslidew++)&32767] = (char)i; continue; }
				if (i == 256) break;
				i = getbits(hxbit[i+30-257][0]) + hxbit[i+30-257][1];

				k = peekbits(LOGQHUFSIZ1);
				if (qhufbit1[k]) { j = qhufval1[k]; suckbits((long)qhufbit1[k]); }
				else j = hufgetsym(ibuf1,nbuf1);

				j = getbits(hxbit[j][0]) + hxbit[j][1];
				for(;i;i--,gslidew++) slidebuf[gslidew&32767] = slidebuf[(gslidew-j)&32767];
			}
		} while (!bfinal);

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
	return(kzfs.pos-i);
}

long kzfilelength ()
{
	if (!kzfs.fil) return(0);
	return(kzfs.leng);
}

	//WARNING: kzseek(<-32768,SEEK_CUR); or:
	//         kzseek(0,SEEK_END);       can make next kzread very slow!!!
long kzseek (long offset, long whence)
{
	if (!kzfs.fil) return(-1);
	switch (whence)
	{
		case SEEK_CUR: kzfs.pos += offset; break;
		case SEEK_END: kzfs.pos = kzfs.leng+offset; break;
		case SEEK_SET: default: kzfs.pos = offset;
	}
	if (kzfs.pos < 0) kzfs.pos = 0;
	if (kzfs.pos > kzfs.leng) kzfs.pos = kzfs.leng;
	return(kzfs.pos);
}

long kztell ()
{
	if (!kzfs.fil) return(-1);
	return(kzfs.pos);
}

long kzgetc ()
{
	char ch;
	if (!kzread(&ch,1)) return(-1);
	return((long)ch);
}

long kzeof ()
{
	if (!kzfs.fil) return(-1);
	return(kzfs.pos >= kzfs.leng);
}

void kzclose ()
{
	if (kzfs.fil) { fclose(kzfs.fil); kzfs.fil = 0; }
}

//====================== ZIP decompression code ends =========================
//===================== HANDY PICTURE function begins ========================

void kpzload (const char *filnam, long *pic, long *bpl, long *xsiz, long *ysiz)
{
	char *buf;
	long leng;

	(*pic) = 0;
	if (!kzopen(filnam)) return;
	leng = kzfilelength();
	buf = (char *)malloc(leng); if (!buf) return;
	kzread(buf,leng);
	kzclose();

	kpgetdim(buf,leng,xsiz,ysiz);
	(*bpl) = ((*xsiz)<<2);
	(*pic) = (long)malloc((*ysiz)*(*bpl)); if (!(*pic)) { free(buf); return; }
	if (kprender(buf,leng,*pic,*bpl,*xsiz,*ysiz,0,0) < 0) { free(buf); free((void *)*pic); (*pic) = 0; return; }
	free(buf);
}
//====================== HANDY PICTURE function ends =========================
