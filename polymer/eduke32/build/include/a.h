// Assembly-language function wrappers for a.asm functions
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)


#ifndef __a_h__
#define __a_h__

#include "compat.h"

#if defined(__GNUC__) && defined(__i386__) && !defined(NOASM)

#if defined(__linux) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__SYLLABLE__)
#define __cdecl
#endif

#ifdef __cplusplus
extern "C"
{
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
extern int32_t __cdecl vlineasm1nonpow2(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl setuptvlineasm(int32_t);
extern int32_t __cdecl tvlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl tvlineasm1nonpow2(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl setuptvlineasm2(int32_t,int32_t,int32_t);
extern int32_t __cdecl tvlineasm2(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl mvlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl mvlineasm1nonpow2(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t __cdecl setupvlineasm(int32_t);
extern int32_t __cdecl vlineasm4(int32_t,char *);
extern int32_t __cdecl setupmvlineasm(int32_t);
extern int32_t __cdecl mvlineasm4(int32_t,char *);
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

#ifdef __cplusplus
}
#endif

#elif defined(_MSC_VER)	&& !defined(NOASM)	// __GNUC__ && __i386__

#ifdef __cplusplus
extern "C"
{
#endif

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
extern int32_t _cdecl vlineasm1nonpow2(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl setuptvlineasm(int32_t);
extern int32_t _cdecl tvlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl tvlineasm1nonpow2(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl setuptvlineasm2(int32_t,int32_t,int32_t);
extern int32_t _cdecl tvlineasm2(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl mvlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl mvlineasm1nonpow2(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
extern int32_t _cdecl setupvlineasm(int32_t);
extern int32_t _cdecl vlineasm4(int32_t,char *);
extern int32_t _cdecl setupmvlineasm(int32_t);
extern int32_t _cdecl mvlineasm4(int32_t,char *);
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

#ifdef __cplusplus
}
#endif

#else				// _MSC_VER

#define ENGINE_USING_A_C
#include <stdint.h>

#define prevlineasm1 vlineasm1

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
int32_t vlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p);
void vlineasm4(int32_t cnt, char *p);

void setupmvlineasm(int32_t neglogy);
int32_t mvlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p);
void mvlineasm4(int32_t cnt, char *p);

void setuptvlineasm(int32_t neglogy);
int32_t tvlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p);

void setuptvlineasm2(int32_t neglogy, intptr_t paloffs1, intptr_t paloffs2);
void tvlineasm2(uint32_t vplc2, int32_t vinc1, intptr_t bufplc1, intptr_t bufplc2, uint32_t vplc1, intptr_t p);

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
