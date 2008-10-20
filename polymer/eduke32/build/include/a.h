// Assembly-language function wrappers for a.asm functions
// for the Build Engine
// by Jonathon Fowler (jonof@edgenetwk.com)


#ifndef __a_h__
#define __a_h__

#if defined(__WATCOMC__) && !defined(NOASM)

extern int mmxoverlay();
#pragma aux mmxoverlay modify [eax ebx ecx edx];
extern int sethlinesizes(int,int,int);
#pragma aux sethlinesizes parm [eax][ebx][ecx];
extern int setpalookupaddress(char *);
#pragma aux setpalookupaddress parm [eax];
extern int setuphlineasm4(int,int);
#pragma aux setuphlineasm4 parm [eax][ebx];
extern int hlineasm4(int,int,int,int,int,int);
#pragma aux hlineasm4 parm [eax][ebx][ecx][edx][esi][edi];
extern int setuprhlineasm4(int,int,int,int,int,int);
#pragma aux setuprhlineasm4 parm [eax][ebx][ecx][edx][esi][edi];
extern int rhlineasm4(int,int,int,int,int,int);
#pragma aux rhlineasm4 parm [eax][ebx][ecx][edx][esi][edi];
extern int setuprmhlineasm4(int,int,int,int,int,int);
#pragma aux setuprmhlineasm4 parm [eax][ebx][ecx][edx][esi][edi];
extern int rmhlineasm4(int,int,int,int,int,int);
#pragma aux rmhlineasm4 parm [eax][ebx][ecx][edx][esi][edi];
extern int setupqrhlineasm4(int,int,int,int,int,int);
#pragma aux setupqrhlineasm4 parm [eax][ebx][ecx][edx][esi][edi];
extern int qrhlineasm4(int,int,int,int,int,int);
#pragma aux qrhlineasm4 parm [eax][ebx][ecx][edx][esi][edi];
extern int setvlinebpl(int);
#pragma aux setvlinebpl parm [eax];
extern int fixtransluscence(int);
#pragma aux fixtransluscence parm [eax];
extern int prevlineasm1(int,int,int,int,int,int);
#pragma aux prevlineasm1 parm [eax][ebx][ecx][edx][esi][edi];
extern int vlineasm1(int,int,int,int,int,int);
#pragma aux vlineasm1 parm [eax][ebx][ecx][edx][esi][edi];
extern int setuptvlineasm(int);
#pragma aux setuptvlineasm parm [eax];
extern int tvlineasm1(int,int,int,int,int,int);
#pragma aux tvlineasm1 parm [eax][ebx][ecx][edx][esi][edi];
extern int setuptvlineasm2(int,int,int);
#pragma aux setuptvlineasm2 parm [eax][ebx][ecx];
extern int tvlineasm2(int,int,int,int,int,int);
#pragma aux tvlineasm2 parm [eax][ebx][ecx][edx][esi][edi];
extern int mvlineasm1(int,int,int,int,int,int);
#pragma aux mvlineasm1 parm [eax][ebx][ecx][edx][esi][edi];
extern int setupvlineasm(int);
#pragma aux setupvlineasm parm [eax];
extern int vlineasm4(int,int);
#pragma aux vlineasm4 parm [ecx][edi] modify [eax ebx ecx edx esi edi];
extern int setupmvlineasm(int);
#pragma aux setupmvlineasm parm [eax];
extern int mvlineasm4(int,int);
#pragma aux mvlineasm4 parm [ecx][edi] modify [eax ebx ecx edx esi edi];
extern void setupspritevline(int,int,int,int,int,int);
#pragma aux setupspritevline parm [eax][ebx][ecx][edx][esi][edi];
extern void spritevline(int,int,int,int,int,int);
#pragma aux spritevline parm [eax][ebx][ecx][edx][esi][edi];
extern void msetupspritevline(int,int,int,int,int,int);
#pragma aux msetupspritevline parm [eax][ebx][ecx][edx][esi][edi];
extern void mspritevline(int,int,int,int,int,int);
#pragma aux mspritevline parm [eax][ebx][ecx][edx][esi][edi];
extern void tsetupspritevline(int,int,int,int,int,int);
#pragma aux tsetupspritevline parm [eax][ebx][ecx][edx][esi][edi];
extern void tspritevline(int,int,int,int,int,int);
#pragma aux tspritevline parm [eax][ebx][ecx][edx][esi][edi];
extern int mhline(int,int,int,int,int,int);
#pragma aux mhline parm [eax][ebx][ecx][edx][esi][edi];
extern int mhlineskipmodify(int,int,int,int,int,int);
#pragma aux mhlineskipmodify parm [eax][ebx][ecx][edx][esi][edi];
extern int msethlineshift(int,int);
#pragma aux msethlineshift parm [eax][ebx];
extern int thline(int,int,int,int,int,int);
#pragma aux thline parm [eax][ebx][ecx][edx][esi][edi];
extern int thlineskipmodify(int,int,int,int,int,int);
#pragma aux thlineskipmodify parm [eax][ebx][ecx][edx][esi][edi];
extern int tsethlineshift(int,int);
#pragma aux tsethlineshift parm [eax][ebx];
extern int setupslopevlin(int,int,int);
#pragma aux setupslopevlin parm [eax][ebx][ecx] modify [edx];
extern int slopevlin(int,int,int,int,int,int);
#pragma aux slopevlin parm [eax][ebx][ecx][edx][esi][edi];
extern int settransnormal();
#pragma aux settransnormal parm;
extern int settransreverse();
#pragma aux settransreverse parm;
extern int setupdrawslab(int,int);
#pragma aux setupdrawslab parm [eax][ebx];
extern int drawslab(int,int,int,int,int,int);
#pragma aux drawslab parm [eax][ebx][ecx][edx][esi][edi];

#elif defined(__GNUC__) && defined(__i386__) && !defined(NOASM)	// __WATCOMC__

#if defined(__linux) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__SYLLABLE__)
#define __cdecl
#endif

extern int __cdecl mmxoverlay();
extern int __cdecl sethlinesizes(int,int,int);
extern int __cdecl setpalookupaddress(char *);
extern int __cdecl setuphlineasm4(int,int);
extern int __cdecl hlineasm4(int,int,int,int,int,int);
extern int __cdecl setuprhlineasm4(int,int,int,int,int,int);
extern int __cdecl rhlineasm4(int,int,int,int,int,int);
extern int __cdecl setuprmhlineasm4(int,int,int,int,int,int);
extern int __cdecl rmhlineasm4(int,int,int,int,int,int);
extern int __cdecl setupqrhlineasm4(int,int,int,int,int,int);
extern int __cdecl qrhlineasm4(int,int,int,int,int,int);
extern int __cdecl setvlinebpl(int);
extern int __cdecl fixtransluscence(int);
extern int __cdecl prevlineasm1(int,int,int,int,int,int);
extern int __cdecl vlineasm1(int,int,int,int,int,int);
extern int __cdecl setuptvlineasm(int);
extern int __cdecl tvlineasm1(int,int,int,int,int,int);
extern int __cdecl setuptvlineasm2(int,int,int);
extern int __cdecl tvlineasm2(int,int,int,int,int,int);
extern int __cdecl mvlineasm1(int,int,int,int,int,int);
extern int __cdecl setupvlineasm(int);
extern int __cdecl vlineasm4(int,int);
extern int __cdecl setupmvlineasm(int);
extern int __cdecl mvlineasm4(int,int);
extern int __cdecl setupspritevline(int,int,int,int,int,int);
extern int __cdecl spritevline(int,int,int,int,int,int);
extern int __cdecl msetupspritevline(int,int,int,int,int,int);
extern int __cdecl mspritevline(int,int,int,int,int,int);
extern int __cdecl tsetupspritevline(int,int,int,int,int,int);
extern int __cdecl tspritevline(int,int,int,int,int,int);
extern int __cdecl mhline(int,int,int,int,int,int);
extern int __cdecl mhlineskipmodify(int,int,int,int,int,int);
extern int __cdecl msethlineshift(int,int);
extern int __cdecl thline(int,int,int,int,int,int);
extern int __cdecl thlineskipmodify(int,int,int,int,int,int);
extern int __cdecl tsethlineshift(int,int);
extern int __cdecl setupslopevlin(int,int,int);
extern int __cdecl slopevlin(int,int,int,int,int,int);
extern int __cdecl settransnormal();
extern int __cdecl settransreverse();
extern int __cdecl setupdrawslab(int,int);
extern int __cdecl drawslab(int,int,int,int,int,int);
extern void __cdecl stretchhline(int,int,int,int,int,int);

#elif defined(_MSC_VER)	&& !defined(NOASM)	// __GNUC__ && __i386__

extern int _cdecl mmxoverlay();
extern int _cdecl sethlinesizes(int,int,int);
extern int _cdecl setpalookupaddress(char *);
extern int _cdecl setuphlineasm4(int,int);
extern int _cdecl hlineasm4(int,int,int,int,int,int);
extern int _cdecl setuprhlineasm4(int,int,int,int,int,int);
extern int _cdecl rhlineasm4(int,int,int,int,int,int);
extern int _cdecl setuprmhlineasm4(int,int,int,int,int,int);
extern int _cdecl rmhlineasm4(int,int,int,int,int,int);
extern int _cdecl setupqrhlineasm4(int,int,int,int,int,int);
extern int _cdecl qrhlineasm4(int,int,int,int,int,int);
extern int _cdecl setvlinebpl(int);
extern int _cdecl fixtransluscence(int);
extern int _cdecl prevlineasm1(int,int,int,int,int,int);
extern int _cdecl vlineasm1(int,int,int,int,int,int);
extern int _cdecl setuptvlineasm(int);
extern int _cdecl tvlineasm1(int,int,int,int,int,int);
extern int _cdecl setuptvlineasm2(int,int,int);
extern int _cdecl tvlineasm2(int,int,int,int,int,int);
extern int _cdecl mvlineasm1(int,int,int,int,int,int);
extern int _cdecl setupvlineasm(int);
extern int _cdecl vlineasm4(int,int);
extern int _cdecl setupmvlineasm(int);
extern int _cdecl mvlineasm4(int,int);
extern int _cdecl setupspritevline(int,int,int,int,int,int);
extern int _cdecl spritevline(int,int,int,int,int,int);
extern int _cdecl msetupspritevline(int,int,int,int,int,int);
extern int _cdecl mspritevline(int,int,int,int,int,int);
extern int _cdecl tsetupspritevline(int,int,int,int,int,int);
extern int _cdecl tspritevline(int,int,int,int,int,int);
extern int _cdecl mhline(int,int,int,int,int,int);
extern int _cdecl mhlineskipmodify(int,int,int,int,int,int);
extern int _cdecl msethlineshift(int,int);
extern int _cdecl thline(int,int,int,int,int,int);
extern int _cdecl thlineskipmodify(int,int,int,int,int,int);
extern int _cdecl tsethlineshift(int,int);
extern int _cdecl setupslopevlin(int,int,int);
extern int _cdecl slopevlin(int,int,int,int,int,int);
extern int _cdecl settransnormal();
extern int _cdecl settransreverse();
extern int _cdecl setupdrawslab(int,int);
extern int _cdecl drawslab(int,int,int,int,int,int);
extern void _cdecl stretchhline(int,int,int,int,int,int);

#else				// _MSC_VER

#define ENGINE_USING_A_C
#include <stdint.h>

void setvlinebpl(int dabpl);
void fixtransluscence(intptr_t datransoff);
void settransnormal(void);
void settransreverse(void);

void sethlinesizes(int logx, int logy, intptr_t bufplc);
void setpalookupaddress(char *paladdr);
void setuphlineasm4(int bxinc, int byinc);
void hlineasm4(int cnt, int skiploadincs, int paloffs, unsigned int by, unsigned int bx, intptr_t p);

void setupslopevlin(int logylogx, intptr_t bufplc, int pinc);
void slopevlin(intptr_t p, int i, intptr_t slopaloffs, int cnt, int bx, int by);

void setupvlineasm(int neglogy);
void vlineasm1(int vinc, intptr_t paloffs, int cnt, unsigned int vplc, intptr_t bufplc, intptr_t p);

void setupmvlineasm(int neglogy);
void mvlineasm1(int vinc, intptr_t paloffs, int cnt, unsigned int vplc, intptr_t bufplc, intptr_t p);

void setuptvlineasm(int neglogy);
void tvlineasm1(int vinc, intptr_t paloffs, int cnt, unsigned int vplc, intptr_t bufplc, intptr_t p);

void msethlineshift(int logx, int logy);
void mhline(intptr_t bufplc, unsigned int bx, int cntup16, int junk, unsigned int by, intptr_t p);

void tsethlineshift(int logx, int logy);
void thline(intptr_t bufplc, unsigned int bx, int cntup16, int junk, unsigned int by, intptr_t p);

void setupspritevline(intptr_t paloffs, int bxinc, int byinc, int ysiz);
void spritevline(int bx, int by, int cnt, intptr_t bufplc, intptr_t p);

void msetupspritevline(intptr_t paloffs, int bxinc, int byinc, int ysiz);
void mspritevline(int bx, int by, int cnt, intptr_t bufplc, intptr_t p);

void tsetupspritevline(intptr_t paloffs, int bxinc, int byinc, int ysiz);
void tspritevline(int bx, int by, int cnt, intptr_t bufplc, intptr_t p);

void setupdrawslab (int dabpl, intptr_t pal);
void drawslab (int dx, int v, int dy, int vi, intptr_t vptr, intptr_t p);
void stretchhline (intptr_t p0, int u, int cnt, int uinc, intptr_t rptr, intptr_t p);

void mmxoverlay();

#endif	// else

#endif // __a_h__
