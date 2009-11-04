// A.ASM replacement using C
// Mainly by Ken Silverman, with things melded with my port by
// Jonathon Fowler (jonof@edgenetwork.org)
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


//Ceiling/floor horizontal line functions
void sethlinesizes(int32_t logx, int32_t logy, intptr_t bufplc)
{ glogx = logx; glogy = logy; gbuf = (char *)bufplc; }
void setpalookupaddress(char *paladdr) { ghlinepal = paladdr; }
void setuphlineasm4(int32_t bxinc, int32_t byinc) { gbxinc = bxinc; gbyinc = byinc; }
void hlineasm4(int32_t cnt, int32_t skiploadincs, int32_t paloffs, uint32_t by, uint32_t bx, intptr_t p)
{
    char *palptr;

    palptr = (char *)&ghlinepal[paloffs];
    if (!skiploadincs) { gbxinc = asm1; gbyinc = asm2; }
    for (; cnt>=0; cnt--)
    {
        *((char *)p) = palptr[gbuf[((bx>>(32-glogx))<<glogy)+(by>>(32-glogy))]];
        bx -= gbxinc;
        by -= gbyinc;
        p--;
    }
}


//Sloped ceiling/floor vertical line functions
void setupslopevlin(int32_t logylogx, intptr_t bufplc, int32_t pinc)
{
    glogx = (logylogx&255); glogy = (logylogx>>8);
    gbuf = (char *)bufplc; gpinc = pinc;
}
void slopevlin(intptr_t p, int32_t i, intptr_t slopaloffs, int32_t cnt, int32_t bx, int32_t by)
{
    intptr_t* slopalptr;
    int32_t bz, bzinc;
    uint32_t u, v;

    bz = asm3; bzinc = (asm1>>3);
    slopalptr = (intptr_t*)slopaloffs;
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


//Wall,face sprite/wall sprite vertical line functions
void setupvlineasm(int32_t neglogy) { glogy = neglogy; }
void vlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p)
{
    gbuf = (char *)bufplc;
    gpal = (char *)paloffs;
    for (; cnt>=0; cnt--)
    {
        *((char *)p) = gpal[gbuf[vplc>>glogy]];
        p += bpl;
        vplc += vinc;
    }
}

void setupmvlineasm(int32_t neglogy) { glogy = neglogy; }
void mvlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p)
{
    char ch;

    gbuf = (char *)bufplc;
    gpal = (char *)paloffs;
    for (; cnt>=0; cnt--)
    {
        ch = gbuf[vplc>>glogy]; if (ch != 255) *((char *)p) = gpal[ch];
        p += bpl;
        vplc += vinc;
    }
}

void setuptvlineasm(int32_t neglogy) { glogy = neglogy; }
void tvlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, uint32_t vplc, intptr_t bufplc, intptr_t p)
{
    char ch;

    gbuf = (char *)bufplc;
    gpal = (char *)paloffs;
    if (transmode)
    {
        for (; cnt>=0; cnt--)
        {
            ch = gbuf[vplc>>glogy];
            if (ch != 255) *((char *)p) = gtrans[(*((char *)p))+(gpal[ch]<<8)];
            p += bpl;
            vplc += vinc;
        }
    }
    else
    {
        for (; cnt>=0; cnt--)
        {
            ch = gbuf[vplc>>glogy];
            if (ch != 255) *((char *)p) = gtrans[((*((char *)p))<<8)+gpal[ch]];
            p += bpl;
            vplc += vinc;
        }
    }
}

//Floor sprite horizontal line functions
void msethlineshift(int32_t logx, int32_t logy) { glogx = logx; glogy = logy; }
void mhline(intptr_t bufplc, uint32_t bx, int32_t cntup16, int32_t junk, uint32_t by, intptr_t p)
{
    char ch;

    gbuf = (char *)bufplc;
    gpal = (char *)asm3;
    for (cntup16>>=16; cntup16>0; cntup16--)
    {
        ch = gbuf[((bx>>(32-glogx))<<glogy)+(by>>(32-glogy))];
        if (ch != 255) *((char *)p) = gpal[ch];
        bx += asm1;
        by += asm2;
        p++;
    }
}

void tsethlineshift(int32_t logx, int32_t logy) { glogx = logx; glogy = logy; }
void thline(intptr_t bufplc, uint32_t bx, int32_t cntup16, int32_t junk, uint32_t by, intptr_t p)
{
    char ch;

    gbuf = (char *)bufplc;
    gpal = (char *)asm3;
    if (transmode)
    {
        for (cntup16>>=16; cntup16>0; cntup16--)
        {
            ch = gbuf[((bx>>(32-glogx))<<glogy)+(by>>(32-glogy))];
            if (ch != 255) *((char *)p) = gtrans[(*((char *)p))+(gpal[ch]<<8)];
            bx += asm1;
            by += asm2;
            p++;
        }
    }
    else
    {
        for (cntup16>>=16; cntup16>0; cntup16--)
        {
            ch = gbuf[((bx>>(32-glogx))<<glogy)+(by>>(32-glogy))];
            if (ch != 255) *((char *)p) = gtrans[((*((char *)p))<<8)+gpal[ch]];
            bx += asm1;
            by += asm2;
            p++;
        }
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


void mmxoverlay() { }

#endif
/*
 * vim:ts=4:
 */

