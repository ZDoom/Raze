// Assembly-language function wrappers for a.asm functions
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)


#ifndef a_h_
#define a_h_

#include "compat.h"

#define CLASSIC_SLICE_BY_4
#define A_C_RESTRICT __restrict

#define CLASSIC_NONPOW2_YSIZE_SPRITES
#ifdef LUNATIC
# define CLASSIC_NONPOW2_YSIZE_WALLS
#endif

#define SLOPTABLESIZ 32768
#define HALFSLOPTABLESIZ (SLOPTABLESIZ>>1)

extern int32_t sloptable[SLOPTABLESIZ];

/** Definitions of high-precision integer types. **/
// Should be used for values that represent coordinates with which calculations
// like dot product are carried out. Substituting 32-bit ints for these will
// very likely yield issues in border cases:
typedef int64_t coord_t;
// Should be used for values that may overflow if 32-bit arithmetic were used,
// but where no other adverse effect (except being undefined behavior,
// obviously) is expected to result:
typedef int64_t inthi_t;
#define inthi_rintf llrintf


#define ENGINE_USING_A_C

#define prevlineasm1 vlineasm1

void setvlinebpl(int32_t dabpl);
void fixtransluscence(intptr_t datransoff);
void settransnormal(void);
void settransreverse(void);

void sethlinesizes(int32_t logx, int32_t logy, intptr_t bufplc);
void setpalookupaddress(char *paladdr);
void setuphlineasm4(int32_t bxinc, int32_t byinc);
void hlineasm4(bssize_t cnt, int32_t skiploadincs, int32_t paloffs, uint32_t by, uint32_t bx, intptr_t p);

void setupslopevlin(int32_t logylogx, intptr_t bufplc, int32_t pinc);
void slopevlin(intptr_t p, int32_t i, intptr_t slopaloffs, bssize_t cnt, int32_t bx, int32_t by);

void setupvlineasm(int32_t neglogy);
int32_t vlineasm1(int32_t vinc, intptr_t paloffs, bssize_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p);
void vlineasm4(bssize_t cnt, char *p);

void setupmvlineasm(int32_t neglogy, int32_t dosaturate);
int32_t mvlineasm1(int32_t vinc, intptr_t paloffs, bssize_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p);
void mvlineasm4(bssize_t cnt, char *p);

void setuptvlineasm(int32_t neglogy, int32_t dosaturate);
int32_t tvlineasm1(int32_t vinc, intptr_t paloffs, bssize_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p);

void setuptvlineasm2(int32_t neglogy, intptr_t paloffs1, intptr_t paloffs2);
void tvlineasm2(uint32_t vplc2, int32_t vinc1, intptr_t bufplc1, intptr_t bufplc2, uint32_t vplc1, intptr_t p);

void msethlineshift(int32_t logx, int32_t logy);
void mhline(intptr_t bufplc, uint32_t bx, int32_t cntup16, int32_t junk, uint32_t by, intptr_t p);

void tsethlineshift(int32_t logx, int32_t logy);
void thline(intptr_t bufplc, uint32_t bx, int32_t cntup16, int32_t junk, uint32_t by, intptr_t p);

void setupspritevline(intptr_t paloffs, int32_t bxinc, int32_t byinc, int32_t ysiz);
void spritevline(int32_t bx, int32_t by, bssize_t cnt, intptr_t bufplc, intptr_t p);

void msetupspritevline(intptr_t paloffs, int32_t bxinc, int32_t byinc, int32_t ysiz);
void mspritevline(int32_t bx, int32_t by, bssize_t cnt, intptr_t bufplc, intptr_t p);

void tsetupspritevline(intptr_t paloffs, int32_t bxinc, int32_t byinc, int32_t ysiz);
void tspritevline(int32_t bx, int32_t by, bssize_t cnt, intptr_t bufplc, intptr_t p);

void setupdrawslab (int32_t dabpl, intptr_t pal);
void drawslab (int32_t dx, int32_t v, int32_t dy, int32_t vi, intptr_t vptr, intptr_t p);
void stretchhline (intptr_t p0, int32_t u, bssize_t cnt, int32_t uinc, intptr_t rptr, intptr_t p);

void mmxoverlay();

#endif // a_h_
