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
        u = bx+globalx3*i;
        v = by+globaly3*i;
        (*(char *)p) = *(char *)(((intptr_t)slopalptr[0])+gbuf[((u>>(32-glogx))<<glogy)+(v>>(32-glogy))]);
        slopalptr--;
        p += gpinc;
    }
}


///// Wall,face sprite/wall sprite vertical line functions /////

void setupvlineasm(int32_t neglogy) { glogy = neglogy; }
// cnt+1 loop iterations!
void vlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p)
{
    const char *const buf = (char *)bufplc;
    const char *const pal = (char *)paloffs;
    const int32_t logy = glogy, ourbpl = bpl;
    char *pp = (char *)p;

    cnt++;

    do
    {
        *pp = pal[buf[vplc>>logy]];
        pp += ourbpl;
        vplc += vinc;
    }
    while (--cnt);
}

void setupmvlineasm(int32_t neglogy) { glogy = neglogy; }
// cnt+1 loop iterations!
void mvlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p)
{
    char ch;

    const char *const buf = (char *)bufplc;
    const char *const pal = (char *)paloffs;
    const int32_t logy = glogy, ourbpl = bpl;
    char *pp = (char *)p;

    cnt++;

    do
    {
        ch = buf[vplc>>logy]; if (ch != 255) *pp = pal[ch];
        pp += ourbpl;
        vplc += vinc;
    }
    while (--cnt);
}

void setuptvlineasm(int32_t neglogy) { glogy = neglogy; }
// cnt+1 loop iterations!
void tvlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p)
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
            ch = buf[vplc>>logy];
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
            ch = buf[vplc>>logy];
            if (ch != 255) *pp = trans[((*pp)<<8)|pal[ch]];
            pp += ourbpl;
            vplc += vinc;
        }
        while (--cnt);
    }
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
        for (x=0; x<dx; x++) *(char *)(p+x) = gpal[(int32_t)(*(char *)((v>>16)+vptr))];
        p += bpl; v += vi; dy--;
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

