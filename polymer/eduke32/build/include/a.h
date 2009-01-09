// Assembly-language function wrappers for a.asm functions
// for the Build Engine
// by Jonathon Fowler (jonof@edgenetwk.com)


#ifndef __a_h__
#define __a_h__

#include "compat.h"

#if defined(__WATCOMC__) && !defined(NOASM)

extern int32_t mmxoverlay();
#pragma aux mmxoverlay modify [eax ebx ecx edx];
extern int32_t sethlinesizes(int32_t,int32_t,int32_t);
#pragma aux sethlinesizes parm [eax][ebx][ecx];
extern int32_t setpalookupaddress(char *);
#pragma aux setpalookupaddress parm [eax];
extern int32_t setuphlineasm4(int32_t,int32_t);
#pragma aux setuphlineasm4 parm [eax][ebx];
extern int32_t hlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux hlineasm4 parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t setuprhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux setuprhlineasm4 parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t rhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux rhlineasm4 parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t setuprmhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux setuprmhlineasm4 parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t rmhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux rmhlineasm4 parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t setupqrhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux setupqrhlineasm4 parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t qrhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux qrhlineasm4 parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t setvlinebpl(int32_t);
#pragma aux setvlinebpl parm [eax];
extern int32_t fixtransluscence(int32_t);
#pragma aux fixtransluscence parm [eax];
extern int32_t prevlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux prevlineasm1 parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t vlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux vlineasm1 parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t setuptvlineasm(int32_t);
#pragma aux setuptvlineasm parm [eax];
extern int32_t tvlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux tvlineasm1 parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t setuptvlineasm2(int32_t,int32_t,int32_t);
#pragma aux setuptvlineasm2 parm [eax][ebx][ecx];
extern int32_t tvlineasm2(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux tvlineasm2 parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t mvlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux mvlineasm1 parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t setupvlineasm(int32_t);
#pragma aux setupvlineasm parm [eax];
extern int32_t vlineasm4(int32_t,int32_t);
#pragma aux vlineasm4 parm [ecx][edi] modify [eax ebx ecx edx esi edi];
extern int32_t setupmvlineasm(int32_t);
#pragma aux setupmvlineasm parm [eax];
extern int32_t mvlineasm4(int32_t,int32_t);
#pragma aux mvlineasm4 parm [ecx][edi] modify [eax ebx ecx edx esi edi];
extern void setupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux setupspritevline parm [eax][ebx][ecx][edx][esi][edi];
extern void spritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux spritevline parm [eax][ebx][ecx][edx][esi][edi];
extern void msetupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux msetupspritevline parm [eax][ebx][ecx][edx][esi][edi];
extern void mspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux mspritevline parm [eax][ebx][ecx][edx][esi][edi];
extern void tsetupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux tsetupspritevline parm [eax][ebx][ecx][edx][esi][edi];
extern void tspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux tspritevline parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t mhline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux mhline parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t mhlineskipmodify(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux mhlineskipmodify parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t msethlineshift(int32_t,int32_t);
#pragma aux msethlineshift parm [eax][ebx];
extern int32_t thline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux thline parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t thlineskipmodify(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux thlineskipmodify parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t tsethlineshift(int32_t,int32_t);
#pragma aux tsethlineshift parm [eax][ebx];
extern int32_t setupslopevlin(int32_t,int32_t,int32_t);
#pragma aux setupslopevlin parm [eax][ebx][ecx] modify [edx];
extern int32_t slopevlin(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux slopevlin parm [eax][ebx][ecx][edx][esi][edi];
extern int32_t settransnormal();
#pragma aux settransnormal parm;
extern int32_t settransreverse();
#pragma aux settransreverse parm;
extern int32_t setupdrawslab(int32_t,int32_t);
#pragma aux setupdrawslab parm [eax][ebx];
extern int32_t drawslab(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
#pragma aux drawslab parm [eax][ebx][ecx][edx][esi][edi];

#elif defined(__GNUC__) && defined(__i386__) && !defined(NOASM)	// __WATCOMC__

#if defined(__linux) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__SYLLABLE__)
#define __cdecl
#endif

extern int32_t __cdecl mmxoverlay();
extern int32_t __cdecl sethlinesizes(int32_t,int32_t,int32_t);
extern int32_t __cdecl setpalookupaddress(char *);
extern int32_t __cdecl setuphlineasm4(int32_t,int32_t);
extern int32_t __cdecl hlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl setuprhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl rhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl setuprmhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl rmhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl setupqrhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl qrhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl setvlinebpl(int32_t);
extern int32_t __cdecl fixtransluscence(int32_t);
extern int32_t __cdecl prevlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl vlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl setuptvlineasm(int32_t);
extern int32_t __cdecl tvlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl setuptvlineasm2(int32_t,int32_t,int32_t);
extern int32_t __cdecl tvlineasm2(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl mvlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl setupvlineasm(int32_t);
extern int32_t __cdecl vlineasm4(int32_t,int32_t);
extern int32_t __cdecl setupmvlineasm(int32_t);
extern int32_t __cdecl mvlineasm4(int32_t,int32_t);
extern int32_t __cdecl setupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl spritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl msetupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl mspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl tsetupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl tspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl mhline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl mhlineskipmodify(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl msethlineshift(int32_t,int32_t);
extern int32_t __cdecl thline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl thlineskipmodify(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl tsethlineshift(int32_t,int32_t);
extern int32_t __cdecl setupslopevlin(int32_t,int32_t,int32_t);
extern int32_t __cdecl slopevlin(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl settransnormal();
extern int32_t __cdecl settransreverse();
extern int32_t __cdecl setupdrawslab(int32_t,int32_t);
extern int32_t __cdecl drawslab(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern void __cdecl stretchhline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);

#elif defined(_MSC_VER)	&& !defined(NOASM)	// __GNUC__ && __i386__

extern int32_t _cdecl mmxoverlay();
extern int32_t _cdecl sethlinesizes(int32_t,int32_t,int32_t);
extern int32_t _cdecl setpalookupaddress(char *);
extern int32_t _cdecl setuphlineasm4(int32_t,int32_t);
extern int32_t _cdecl hlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl setuprhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl rhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl setuprmhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl rmhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl setupqrhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl qrhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl setvlinebpl(int32_t);
extern int32_t _cdecl fixtransluscence(int32_t);
extern int32_t _cdecl prevlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl vlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl setuptvlineasm(int32_t);
extern int32_t _cdecl tvlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl setuptvlineasm2(int32_t,int32_t,int32_t);
extern int32_t _cdecl tvlineasm2(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl mvlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl setupvlineasm(int32_t);
extern int32_t _cdecl vlineasm4(int32_t,int32_t);
extern int32_t _cdecl setupmvlineasm(int32_t);
extern int32_t _cdecl mvlineasm4(int32_t,int32_t);
extern int32_t _cdecl setupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl spritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl msetupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl mspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl tsetupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl tspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl mhline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl mhlineskipmodify(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl msethlineshift(int32_t,int32_t);
extern int32_t _cdecl thline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl thlineskipmodify(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl tsethlineshift(int32_t,int32_t);
extern int32_t _cdecl setupslopevlin(int32_t,int32_t,int32_t);
extern int32_t _cdecl slopevlin(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl settransnormal();
extern int32_t _cdecl settransreverse();
extern int32_t _cdecl setupdrawslab(int32_t,int32_t);
extern int32_t _cdecl drawslab(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern void _cdecl stretchhline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);

#else				// _MSC_VER

#define ENGINE_USING_A_C
#include <stdint.h>

void setvlinebpl(int32_t dabpl);
void fixtransluscence(intptr_t datransoff);
void settransnormal(void);
void settransreverse(void);

void sethlinesizes(int32_t logx, int32_t logy, intptr_t bufplc);
void setpalookupaddress(char *paladdr);
void setuphlineasm4(int32_t bxinc, int32_t byinc);
void hlineasm4(int32_t cnt, int32_t skiploadincs, int32_t paloffs, uint32_t by, uint32_t bx, intptr_t p);

void setupslopevlin(int32_t logylogx, intptr_t bufplc, int32_t pinc);
void slopevlin(intptr_t p, int32_t i, intptr_t slopaloffs, int32_t cnt, int32_t bx, int32_t by);

void setupvlineasm(int32_t neglogy);
void vlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p);

void setupmvlineasm(int32_t neglogy);
void mvlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p);

void setuptvlineasm(int32_t neglogy);
void tvlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p);

void msethlineshift(int32_t logx, int32_t logy);
void mhline(intptr_t bufplc, uint32_t bx, int32_t cntup16, int32_t junk, uint32_t by, intptr_t p);

void tsethlineshift(int32_t logx, int32_t logy);
void thline(intptr_t bufplc, uint32_t bx, int32_t cntup16, int32_t junk, uint32_t by, intptr_t p);

void setupspritevline(intptr_t paloffs, int32_t bxinc, int32_t byinc, int32_t ysiz);
void spritevline(int32_t bx, int32_t by, int32_t cnt, intptr_t bufplc, intptr_t p);

void msetupspritevline(intptr_t paloffs, int32_t bxinc, int32_t byinc, int32_t ysiz);
void mspritevline(int32_t bx, int32_t by, int32_t cnt, intptr_t bufplc, intptr_t p);

void tsetupspritevline(intptr_t paloffs, int32_t bxinc, int32_t byinc, int32_t ysiz);
void tspritevline(int32_t bx, int32_t by, int32_t cnt, intptr_t bufplc, intptr_t p);

void setupdrawslab (int32_t dabpl, intptr_t pal);
void drawslab (int32_t dx, int32_t v, int32_t dy, int32_t vi, intptr_t vptr, intptr_t p);
void stretchhline (intptr_t p0, int32_t u, int32_t cnt, int32_t uinc, intptr_t rptr, intptr_t p);

void mmxoverlay();

#endif	// else

#endif // __a_h__
