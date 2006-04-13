// Function-wrapped Watcom pragmas
// by Jonathon Fowler (jonof@edgenetwk.com)
//
// These functions represent some of the more longer-winded pragmas
// from the original pragmas.h wrapped into functions for easier
// use since many jumps and whatnot make it harder to write macro-
// inline versions. I'll eventually convert these to macro-inline
// equivalents.		--Jonathon

//#include "pragmas.h"
#include "compat.h"

long dmval;

#if defined(NOASM)

//
// Generic C version
//

void qinterpolatedown16(long bufptr, long num, long val, long add)
{ // gee, I wonder who could have provided this...
    long i, *lptr = (long *)bufptr;
    for(i=0;i<num;i++) { lptr[i] = (val>>16); val += add; }
}

void qinterpolatedown16short(long bufptr, long num, long val, long add)
{ // ...maybe the same person who provided this too?
    long i; short *sptr = (short *)bufptr;
    for(i=0;i<num;i++) { sptr[i] = (short)(val>>16); val += add; }
}

void clearbuf(void *d, long c, long a)
{
	long *p = (long*)d;
	while ((c--) > 0) *(p++) = a;
}

void copybuf(void *s, void *d, long c)
{
	long *p = (long*)s, *q = (long*)d;
	while ((c--) > 0) *(q++) = *(p++);
}

void swapbuf4(void *a, void *b, long c)
{
	long *p = (long*)a, *q = (long*)b;
	long x, y;
	while ((c--) > 0) {
		x = *q;
		y = *p;
		*(q++) = y;
		*(p++) = x;
	}
}

void clearbufbyte(void *D, long c, long a)
{ // Cringe City
	char *p = (char*)D;
	long m[4] = { 0xffl,0xff00l,0xff0000l,0xff000000l };
	long n[4] = { 0,8,16,24 };
	long z=0;
	while ((c--) > 0) {
		*(p++) = (char)((a & m[z])>>n[z]);
		z=(z+1)&3;
	}
}

void copybufbyte(void *S, void *D, long c)
{
	char *p = (char*)S, *q = (char*)D;
	while((c--) > 0) *(q++) = *(p++);
}

void copybufreverse(void *S, void *D, long c)
{
	char *p = (char*)S, *q = (char*)D;
	while((c--) > 0) *(q++) = *(p--);
}

#elif defined(__GNUC__) && defined(__i386__)	// NOASM

//
// GCC Inline Assembler version
//

#define ASM __asm__ __volatile__


long boundmulscale(long a, long b, long c)
{
	ASM (
		"imull %%ebx\n\t"
		"movl %%edx, %%ebx\n\t"		// mov ebx, edx
		"shrdl %%cl, %%edx, %%eax\n\t"	// mov eax, edx, cl
		"sarl %%cl, %%edx\n\t"		// sar edx, cl
		"xorl %%eax, %%edx\n\t"		// xor edx, eax
		"js 0f\n\t"			// js checkit
		"xorl %%eax, %%edx\n\t"		// xor edx, eax
		"jz 1f\n\t"			// js skipboundit
		"cmpl $0xffffffff, %%edx\n\t"	// cmp edx, 0xffffffff
		"je 1f\n\t"			// je skipboundit
		"0:\n\t"			// checkit:
		"movl %%ebx, %%eax\n\t"		// mov eax, ebx
		"sarl $31, %%eax\n\t"		// sar eax, 31
		"xorl $0x7fffffff, %%eax\n\t"	// xor eax, 0x7fffffff
		"1:"				// skipboundit:
		: "+a" (a), "+b" (b), "+c" (c)	// input eax ebx ecx
		:
		: "edx", "cc"
	);
	return a;
}


void clearbufbyte(void *D, long c, long a)
{
	ASM (
		"cmpl $4, %%ecx\n\t"
		"jae 1f\n\t"
		"testb $1, %%cl\n\t"
		"jz 0f\n\t"			// jz preskip
		"stosb\n\t"
		"0:\n\t"			// preskip:
		"shrl $1, %%ecx\n\t"
		"rep\n\t"
		"stosw\n\t"
		"jmp 5f\n\t"			// jmp endit
		"1:\n\t"			// intcopy:
		"testl $1, %%edi\n\t"
		"jz 2f\n\t"			// jz skip1
		"stosb\n\t"
		"decl %%ecx\n\t"
		"2:\n\t"			// skip1:
		"testl $2, %%edi\n\t"
		"jz 3f\n\t"			// jz skip2
		"stosw\n\t"
		"subl $2, %%ecx\n\t"
		"3:\n\t"			// skip2:
		"movl %%ecx, %%ebx\n\t"
		"shrl $2, %%ecx\n\t"
		"rep\n\t"
		"stosl\n\t"
		"testb $2, %%bl\n\t"
		"jz 4f\n\t"			// jz skip3
		"stosw\n\t"
		"4:\n\t"			// skip3:
		"testb $1, %%bl\n\t"
		"jz 5f\n\t"			// jz endit
		"stosb\n\t"
		"5:"				// endit
		: "+D" (D), "+c" (c), "+a" (a) :
		: "ebx", "memory", "cc"
	);
}

void copybufbyte(void *S, void *D, long c)
{
	ASM (
		"cmpl $4, %%ecx\n\t"		// cmp ecx, 4
		"jae 1f\n\t"
		"testb $1, %%cl\n\t"		// test cl, 1
		"jz 0f\n\t"
		"movsb\n\t"
		"0:\n\t"			// preskip:
		"shrl $1, %%ecx\n\t"		// shr ecx, 1
		"rep\n\t"
		"movsw\n\t"
		"jmp 5f\n\t"
		"1:\n\t"			// intcopy:
		"testl $1, %%edi\n\t"		// test edi, 1
		"jz 2f\n\t"
		"movsb\n\t"
		"decl %%ecx\n\t"
		"2:\n\t"			// skip1:
		"testl $2, %%edi\n\t"		// test edi, 2
		"jz 3f\n\t"
		"movsw\n\t"
		"subl $2, %%ecx\n\t"		// sub ecx, 2
		"3:\n\t"			// skip2:
		"movl %%ecx, %%ebx\n\t"		// mov ebx, ecx
		"shrl $2, %%ecx\n\t"		// shr ecx ,2
		"rep\n\t"
		"movsl\n\t"
		"testb $2, %%bl\n\t"		// test bl, 2
		"jz 4f\n\t"
		"movsw\n\t"
		"4:\n\t"			// skip3:
		"testb $1, %%bl\n\t"		// test bl, 1
		"jz 5f\n\t"
		"movsb\n\t"
		"5:"				// endit:
		: "+c" (c), "+S" (S), "+D" (D) :
		: "ebx", "memory", "cc"
	);
}

void copybufreverse(void *S, void *D, long c)
{
	ASM (
		"shrl $1, %%ecx\n\t"
		"jnc 0f\n\t"		// jnc skipit1
		"movb (%%esi), %%al\n\t"
		"decl %%esi\n\t"
		"movb %%al, (%%edi)\n\t"
		"incl %%edi\n\t"
		"0:\n\t"		// skipit1:
		"shrl $1, %%ecx\n\t"
		"jnc 1f\n\t"		// jnc skipit2
		"movw -1(%%esi), %%ax\n\t"
		"subl $2, %%esi\n\t"
		"rorw $8, %%ax\n\t"
		"movw %%ax, (%%edi)\n\t"
		"addl $2, %%edi\n\t"
		"1:\n\t"		// skipit2
		"testl %%ecx, %%ecx\n\t"
		"jz 3f\n\t"		// jz endloop
		"2:\n\t"		// begloop
		"movl -3(%%esi), %%eax\n\t"
		"subl $4, %%esi\n\t"
		"bswapl %%eax\n\t"
		"movl %%eax, (%%edi)\n\t"
		"addl $4, %%edi\n\t"
		"decl %%ecx\n\t"
		"jnz 2b\n\t"		// jnz begloop
		"3:"
		: "+S" (S), "+D" (D), "+c" (c) :
		: "eax", "memory", "cc"
	);
}

#elif defined(__WATCOMC__)	// __GNUC__ && __i386__

//
// Watcom C Inline Assembler version
//

#elif defined(_MSC_VER)		// __WATCOMC__

//
// Microsoft C Inline Assembler version
//

#else				// _MSC_VER

#error Unsupported compiler or architecture.

#endif


