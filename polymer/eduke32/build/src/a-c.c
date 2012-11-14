// A.ASM replacement using C
// Mainly by Ken Silverman, with things melded with my port by
// Jonathon Fowler (jf@jonof.id.au)
//
// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.

#include "a.h"

#ifdef ENGINE_USING_A_C

int32_t krecip(int32_t num);	// from engine.c

#define BITSOFPRECISION 3
#define BITSOFPRECISIONPOW 8

extern intptr_t asm1, asm2, asm3, asm4;
extern int32_t fpuasm, globalx3, globaly3;
extern void *reciptable;

static int32_t bpl, transmode = 0;
static int32_t glogx, glogy, gbxinc, gbyinc, gpinc;
static char *gbuf, *gpal, *ghlinepal, *gtrans;
static char *gpal2;

//Global variable functions
void setvlinebpl(int32_t dabpl) { bpl = dabpl; }
void fixtransluscence(intptr_t datransoff) { gtrans = (char *)datransoff; }
void settransnormal(void) { transmode = 0; }
void settransreverse(void) { transmode = 1; }


///// Ceiling/floor horizontal line functions /////

void sethlinesizes(int32_t logx, int32_t logy, intptr_t bufplc)
{ glogx = logx; glogy = logy; gbuf = (char *)bufplc; }
void setpalookupaddress(char *paladdr) { ghlinepal = paladdr; }
void setuphlineasm4(int32_t bxinc, int32_t byinc) { gbxinc = bxinc; gbyinc = byinc; }
void hlineasm4(int32_t cnt, int32_t skiploadincs, int32_t paloffs, uint32_t by, uint32_t bx, intptr_t p)
{
    if (!skiploadincs) { gbxinc = asm1; gbyinc = asm2; }

    {
        const char *const palptr = &ghlinepal[paloffs];
        const char *const buf = gbuf;
        const int32_t bxinc = gbxinc, byinc = gbyinc;
        const int32_t logx = glogx, logy = glogy;
        char *pp = (char *)p;

        for (; cnt>=0; cnt--)
        {
            *pp = palptr[buf[((bx>>(32-logx))<<logy)+(by>>(32-logy))]];
            bx -= bxinc;
            by -= byinc;
            pp--;
        }
    }
}


///// Sloped ceiling/floor vertical line functions /////

void setupslopevlin(int32_t logylogx, intptr_t bufplc, int32_t pinc)
{
    glogx = (logylogx&255); glogy = (logylogx>>8);
    gbuf = (char *)bufplc; gpinc = pinc;
}
void slopevlin(intptr_t p, int32_t i, intptr_t slopaloffs, int32_t cnt, int32_t bx, int32_t by)
{
    intptr_t *slopalptr;
    int32_t bz, bzinc;
    uint32_t u, v;

    bz = asm3; bzinc = (asm1>>3);
    slopalptr = (intptr_t *)slopaloffs;
    for (; cnt>0; cnt--)
    {
        i = krecip(bz>>6); bz += bzinc;
        u = bx+(int64_t)globalx3*i;
        v = by+(int64_t)globaly3*i;
        (*(char *)p) = *(char *)(((intptr_t)slopalptr[0])+gbuf[((u>>(32-glogx))<<glogy)+(v>>(32-glogy))]);
        slopalptr--;
        p += gpinc;
    }
}


///// Wall,face sprite/wall sprite vertical line functions /////


extern int32_t globaltilesizy;

static inline uint32_t ourmulscale32(uint32_t a, uint32_t b)
{
    return ((uint64_t)a*b)>>32;
}

static inline int32_t getpix(int32_t logy, const char *buf, uint32_t vplc)
{
    if (logy != 0)
        return buf[vplc>>logy];
    else
        return buf[ourmulscale32(vplc,globaltilesizy)];
}

void setupvlineasm(int32_t neglogy) { glogy = neglogy; }
// cnt+1 loop iterations!
int32_t vlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p)
{
    const char *const buf = (char *)bufplc;
    const char *const pal = (char *)paloffs;
    const int32_t logy = glogy, ourbpl = bpl;
    char *pp = (char *)p;

    cnt++;

    do
    {
        if (logy != 0)
            *pp = pal[buf[vplc>>logy]];
        else
            *pp = pal[buf[ourmulscale32(vplc,globaltilesizy)]];

        pp += ourbpl;
        vplc += vinc;
    }
    while (--cnt);

    return vplc;
}


extern intptr_t palookupoffse[4];
extern uint32_t vplce[4];
extern int32_t vince[4];
extern intptr_t bufplce[4];

// cnt >= 1
void vlineasm4(int32_t cnt, char *p)
{
    char ch;
    int32_t i;
#if 1
    // this gives slightly more stuff in registers in the loop
    // (on x86_64 at least)
    char *const pal[4] = {(char *)palookupoffse[0], (char *)palookupoffse[1], (char *)palookupoffse[2], (char *)palookupoffse[3]};
    char *const buf[4] = {(char *)bufplce[0], (char *)bufplce[1], (char *)bufplce[2], (char *)bufplce[3]};
    const int32_t vinc[4] = {vince[0], vince[1], vince[2], vince[3]};
    uint32_t vplc[4] = {vplce[0], vplce[1], vplce[2], vplce[3]};
#else
    char *pal[4];
    char *buf[4];
    int32_t vinc[4];
    uint32_t vplc[4];

    Bmemcpy(pal, palookupoffse, sizeof(pal));
    Bmemcpy(buf, bufplce, sizeof(buf));
    Bmemcpy(vinc, vince, sizeof(vinc));
    Bmemcpy(vplc, vplce, sizeof(vplc));
#endif

    const int32_t logy = glogy, ourbpl = bpl;

    do
    {
        for (i=0; i<4; i++)
        {
            ch = getpix(logy, buf[i], vplc[i]);
            p[i] = pal[i][ch];
            vplc[i] += vinc[i];
        }
        p += ourbpl;
    }
    while (--cnt);

    Bmemcpy(vplce, vplc, sizeof(vplce));
}

void setupmvlineasm(int32_t neglogy) { glogy = neglogy; }
// cnt+1 loop iterations!
int32_t mvlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p)
{
    char ch;

    const char *const buf = (char *)bufplc;
    const char *const pal = (char *)paloffs;
    const int32_t logy = glogy, ourbpl = bpl;
    char *pp = (char *)p;

    cnt++;

    do
    {
        ch = getpix(logy, buf, vplc);
        if (ch != 255) *pp = pal[ch];
        pp += ourbpl;
        vplc += vinc;
    }
    while (--cnt);

    return vplc;
}

// cnt >= 1
void mvlineasm4(int32_t cnt, char *p)
{
    char ch;
    int32_t i;

    char *const pal[4] = {(char *)palookupoffse[0], (char *)palookupoffse[1], (char *)palookupoffse[2], (char *)palookupoffse[3]};
    char *const buf[4] = {(char *)bufplce[0], (char *)bufplce[1], (char *)bufplce[2], (char *)bufplce[3]};
    const int32_t vinc[4] = {vince[0], vince[1], vince[2], vince[3]};
    uint32_t vplc[4] = {vplce[0], vplce[1], vplce[2], vplce[3]};

    const int32_t logy = glogy, ourbpl = bpl;

    do
    {
        for (i=0; i<4; i++)
        {
            ch = getpix(logy, buf[i], vplc[i]);
            if (ch != 255) p[i] = pal[i][ch];
            vplc[i] += vinc[i];
        }
        p += ourbpl;
    }
    while (--cnt);

    Bmemcpy(vplce, vplc, sizeof(vplce));
}


void setuptvlineasm(int32_t neglogy) { glogy = neglogy; }
// cnt+1 loop iterations!
int32_t tvlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p)
{
    char ch;

    const char *const buf = (char *)bufplc;
    const char *const pal = (char *)paloffs;
    const char *const trans = (char *)gtrans;
    const int32_t logy = glogy, ourbpl = bpl, transm = transmode;
    char *pp = (char *)p;

    cnt++;

    if (transm)
    {
        do
        {
            ch = getpix(logy, buf, vplc);
            if (ch != 255) *pp = trans[(*pp)|(pal[ch]<<8)];
            pp += ourbpl;
            vplc += vinc;
        }
        while (--cnt);
    }
    else
    {
        do
        {
            ch = getpix(logy, buf, vplc);
            if (ch != 255) *pp = trans[((*pp)<<8)|pal[ch]];
            pp += ourbpl;
            vplc += vinc;
        }
        while (--cnt);
    }

    return vplc;
}

void setuptvlineasm2(int32_t neglogy, intptr_t paloffs1, intptr_t paloffs2)
{
    glogy = neglogy;
    gpal = (char *)paloffs1;
    gpal2 = (char *)paloffs2;
}
// Pass: asm1=vinc2, asm2=pend
// Return: asm1=vplc1, asm2=vplc2
void tvlineasm2(uint32_t vplc2, int32_t vinc1, intptr_t bufplc1, intptr_t bufplc2, uint32_t vplc1, intptr_t p)
{
    char ch;

    int32_t cnt = (asm2-p-1)/bpl;  // >= 1
    const int32_t vinc2 = asm1;

    const char *const buf1 = (char *)bufplc1;
    const char *const buf2 = (char *)bufplc2;
    const int32_t logy = glogy, ourbpl = bpl, transm = transmode;

    char *pp = (char *)p;

    cnt++;

    if (transm)
    {
        do
        {
            ch = getpix(logy, buf1, vplc1);
            if (ch != 255) pp[0] = gtrans[pp[0]|(gpal[ch]<<8)];
            vplc1 += vinc1;

            ch = getpix(logy, buf2, vplc2);
            if (ch != 255) pp[1] = gtrans[pp[1]|(gpal2[ch]<<8)];
            vplc2 += vinc2;

            pp += ourbpl;
        }
        while (--cnt > 0);
    }
    else
    {
        do
        {
            ch = getpix(logy, buf1, vplc1);
            if (ch != 255) pp[0] = gtrans[(pp[0]<<8)|gpal[ch]];
            vplc1 += vinc1;

            ch = getpix(logy, buf2, vplc2);
            if (ch != 255) pp[1] = gtrans[(pp[1]<<8)|gpal2[ch]];
            vplc2 += vinc2;

            pp += ourbpl;
        }
        while (--cnt);
    }

    asm1 = vplc1;
    asm2 = vplc2;
}


//Floor sprite horizontal line functions
void msethlineshift(int32_t logx, int32_t logy) { glogx = logx; glogy = logy; }
// cntup16>>16 + 1 iterations
void mhline(intptr_t bufplc, uint32_t bx, int32_t cntup16, int32_t junk, uint32_t by, intptr_t p)
{
    char ch;

    const int32_t xinc = asm1, yinc = asm2;

    UNREFERENCED_PARAMETER(junk);

    gbuf = (char *)bufplc;
    gpal = (char *)asm3;

    cntup16>>=16;
    cntup16++;
    do
    {
        ch = gbuf[((bx>>(32-glogx))<<glogy)+(by>>(32-glogy))];
        if (ch != 255) *((char *)p) = gpal[ch];
        bx += xinc;
        by += yinc;
        p++;
    }
    while (--cntup16);
}

void tsethlineshift(int32_t logx, int32_t logy) { glogx = logx; glogy = logy; }
// cntup16>>16 + 1 iterations
void thline(intptr_t bufplc, uint32_t bx, int32_t cntup16, int32_t junk, uint32_t by, intptr_t p)
{
    char ch;

    const int32_t xinc = asm1, yinc = asm2;

    UNREFERENCED_PARAMETER(junk);

    gbuf = (char *)bufplc;
    gpal = (char *)asm3;

    cntup16>>=16;
    cntup16++;

    if (transmode)
    {
        do
        {
            ch = gbuf[((bx>>(32-glogx))<<glogy)+(by>>(32-glogy))];
            if (ch != 255) *((char *)p) = gtrans[(*((char *)p))|(gpal[ch]<<8)];
            bx += xinc;
            by += yinc;
            p++;
        }
        while (--cntup16);
    }
    else
    {
        do
        {
            ch = gbuf[((bx>>(32-glogx))<<glogy)+(by>>(32-glogy))];
            if (ch != 255) *((char *)p) = gtrans[((*((char *)p))<<8)|gpal[ch]];
            bx += xinc;
            by += yinc;
            p++;
        }
        while (--cntup16);
    }
}


//Rotatesprite vertical line functions
void setupspritevline(intptr_t paloffs, int32_t bxinc, int32_t byinc, int32_t ysiz)
{
    gpal = (char *)paloffs;
    gbxinc = bxinc;
    gbyinc = byinc;
    glogy = ysiz;
}
void spritevline(int32_t bx, int32_t by, int32_t cnt, intptr_t bufplc, intptr_t p)
{
    gbuf = (char *)bufplc;
    for (; cnt>1; cnt--)
    {
        (*(char *)p) = gpal[gbuf[(bx>>16)*glogy+(by>>16)]];
        bx += gbxinc;
        by += gbyinc;
        p += bpl;
    }
}

//Rotatesprite vertical line functions
void msetupspritevline(intptr_t paloffs, int32_t bxinc, int32_t byinc, int32_t ysiz)
{
    gpal = (char *)paloffs;
    gbxinc = bxinc;
    gbyinc = byinc;
    glogy = ysiz;
}
void mspritevline(int32_t bx, int32_t by, int32_t cnt, intptr_t bufplc, intptr_t p)
{
    char ch;

    gbuf = (char *)bufplc;
    for (; cnt>1; cnt--)
    {
        ch = gbuf[(bx>>16)*glogy+(by>>16)];
        if (ch != 255)(*(char *)p) = gpal[ch];
        bx += gbxinc;
        by += gbyinc;
        p += bpl;
    }
}

void tsetupspritevline(intptr_t paloffs, int32_t bxinc, int32_t byinc, int32_t ysiz)
{
    gpal = (char *)paloffs;
    gbxinc = bxinc;
    gbyinc = byinc;
    glogy = ysiz;
}
void tspritevline(int32_t bx, int32_t by, int32_t cnt, intptr_t bufplc, intptr_t p)
{
    char ch;

    gbuf = (char *)bufplc;
    if (transmode)
    {
        for (; cnt>1; cnt--)
        {
            ch = gbuf[(bx>>16)*glogy+(by>>16)];
            if (ch != 255) *((char *)p) = gtrans[(*((char *)p))+(gpal[ch]<<8)];
            bx += gbxinc;
            by += gbyinc;
            p += bpl;
        }
    }
    else
    {
        for (; cnt>1; cnt--)
        {
            ch = gbuf[(bx>>16)*glogy+(by>>16)];
            if (ch != 255) *((char *)p) = gtrans[((*((char *)p))<<8)+gpal[ch]];
            bx += gbxinc;
            by += gbyinc;
            p += bpl;
        }
    }
}

void setupdrawslab(int32_t dabpl, intptr_t pal)
{ bpl = dabpl; gpal = (char *)pal; }

void drawslab(int32_t dx, int32_t v, int32_t dy, int32_t vi, intptr_t vptr, intptr_t p)
{
    int32_t x;

    while (dy > 0)
    {
        char c = gpal[(int32_t)(*(char *)((v>>16)+vptr))];
        for (x=0; x < dx; x++)
            ((char*)p)[x] = c;
        p += bpl;
        v += vi;
        dy--;
    }
}

#if 0
void stretchhline(intptr_t p0, int32_t u, int32_t cnt, int32_t uinc, intptr_t rptr, intptr_t p)
{
    p0 = p-(cnt<<2);
    do
    {
        p--;
        *(char *)p = *(char *)((u>>16)+rptr); u -= uinc;
    }
    while (p > p0);
}
#endif

void mmxoverlay() { }

#endif
/*
 * vim:ts=4:
 */

