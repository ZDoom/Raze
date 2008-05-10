// A.ASM replacement using C
// Mainly by Ken Silverman, with things melded with my port by
// Jonathon Fowler (jonof@edgenetwork.org)
//
// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.

#include "a.h"

#ifdef ENGINE_USING_A_C

int krecip(int num);	// from engine.c

#define BITSOFPRECISION 3
#define BITSOFPRECISIONPOW 8

extern intptr_t asm1, asm2, asm3, asm4;
extern int fpuasm, globalx3, globaly3;
extern void *reciptable;

static int bpl, transmode = 0;
static int glogx, glogy, gbxinc, gbyinc, gpinc;
static char *gbuf, *gpal, *ghlinepal, *gtrans;

//Global variable functions
void setvlinebpl(int dabpl) { bpl = dabpl; }
void fixtransluscence(intptr_t datransoff) { gtrans = (char *)datransoff; }
void settransnormal(void) { transmode = 0; }
void settransreverse(void) { transmode = 1; }


//Ceiling/floor horizontal line functions
void sethlinesizes(int logx, int logy, intptr_t bufplc)
{ glogx = logx; glogy = logy; gbuf = (char *)bufplc; }
void setpalookupaddress(char *paladdr) { ghlinepal = paladdr; }
void setuphlineasm4(int bxinc, int byinc) { gbxinc = bxinc; gbyinc = byinc; }
void hlineasm4(int cnt, int skiploadincs, int paloffs, unsigned int by, unsigned int bx, intptr_t p)
{
    char *palptr;

    palptr = (char *)&ghlinepal[paloffs];
    if (!skiploadincs) { gbxinc = asm1; gbyinc = asm2; }
    for (;cnt>=0;cnt--)
    {
        *((char *)p) = palptr[gbuf[((bx>>(32-glogx))<<glogy)+(by>>(32-glogy))]];
        bx -= gbxinc;
        by -= gbyinc;
        p--;
    }
}


//Sloped ceiling/floor vertical line functions
void setupslopevlin(int logylogx, intptr_t bufplc, int pinc)
{
    glogx = (logylogx&255); glogy = (logylogx>>8);
    gbuf = (char *)bufplc; gpinc = pinc;
}
void slopevlin(intptr_t p, int i, intptr_t slopaloffs, int cnt, int bx, int by)
{
    int *slopalptr, bz, bzinc;
    unsigned int u, v;

    bz = asm3; bzinc = (asm1>>3);
    slopalptr = (int*)slopaloffs;
    for (;cnt>0;cnt--)
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
void setupvlineasm(int neglogy) { glogy = neglogy; }
void vlineasm1(int vinc, intptr_t paloffs, int cnt, unsigned int vplc, intptr_t bufplc, intptr_t p)
{
    gbuf = (char *)bufplc;
    gpal = (char *)paloffs;
    for (;cnt>=0;cnt--)
    {
        *((char *)p) = gpal[gbuf[vplc>>glogy]];
        p += bpl;
        vplc += vinc;
    }
}

void setupmvlineasm(int neglogy) { glogy = neglogy; }
void mvlineasm1(int vinc, intptr_t paloffs, int cnt, unsigned int vplc, intptr_t bufplc, intptr_t p)
{
    char ch;

    gbuf = (char *)bufplc;
    gpal = (char *)paloffs;
    for (;cnt>=0;cnt--)
    {
        ch = gbuf[vplc>>glogy]; if (ch != 255) *((char *)p) = gpal[ch];
        p += bpl;
        vplc += vinc;
    }
}

void setuptvlineasm(int neglogy) { glogy = neglogy; }
void tvlineasm1(int vinc, intptr_t paloffs, int cnt, unsigned int vplc, intptr_t bufplc, intptr_t p)
{
    char ch;

    gbuf = (char *)bufplc;
    gpal = (char *)paloffs;
    if (transmode)
    {
        for (;cnt>=0;cnt--)
        {
            ch = gbuf[vplc>>glogy];
            if (ch != 255) *((char *)p) = gtrans[(*((char *)p))+(gpal[ch]<<8)];
            p += bpl;
            vplc += vinc;
        }
    }
    else
    {
        for (;cnt>=0;cnt--)
        {
            ch = gbuf[vplc>>glogy];
            if (ch != 255) *((char *)p) = gtrans[((*((char *)p))<<8)+gpal[ch]];
            p += bpl;
            vplc += vinc;
        }
    }
}

//Floor sprite horizontal line functions
void msethlineshift(int logx, int logy) { glogx = logx; glogy = logy; }
void mhline(intptr_t bufplc, unsigned int bx, int cntup16, int junk, unsigned int by, intptr_t p)
{
    char ch;

    gbuf = (char *)bufplc;
    gpal = (char *)asm3;
    for (cntup16>>=16;cntup16>0;cntup16--)
    {
        ch = gbuf[((bx>>(32-glogx))<<glogy)+(by>>(32-glogy))];
        if (ch != 255) *((char *)p) = gpal[ch];
        bx += asm1;
        by += asm2;
        p++;
    }
}

void tsethlineshift(int logx, int logy) { glogx = logx; glogy = logy; }
void thline(intptr_t bufplc, unsigned int bx, int cntup16, int junk, unsigned int by, intptr_t p)
{
    char ch;

    gbuf = (char *)bufplc;
    gpal = (char *)asm3;
    if (transmode)
    {
        for (cntup16>>=16;cntup16>0;cntup16--)
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
        for (cntup16>>=16;cntup16>0;cntup16--)
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
void setupspritevline(intptr_t paloffs, int bxinc, int byinc, int ysiz)
{
    gpal = (char *)paloffs;
    gbxinc = bxinc;
    gbyinc = byinc;
    glogy = ysiz;
}
void spritevline(int bx, int by, int cnt, intptr_t bufplc, intptr_t p)
{
    gbuf = (char *)bufplc;
    for (;cnt>1;cnt--)
    {
        (*(char *)p) = gpal[gbuf[(bx>>16)*glogy+(by>>16)]];
        bx += gbxinc;
        by += gbyinc;
        p += bpl;
    }
}

//Rotatesprite vertical line functions
void msetupspritevline(intptr_t paloffs, int bxinc, int byinc, int ysiz)
{
    gpal = (char *)paloffs;
    gbxinc = bxinc;
    gbyinc = byinc;
    glogy = ysiz;
}
void mspritevline(int bx, int by, int cnt, intptr_t bufplc, intptr_t p)
{
    char ch;

    gbuf = (char *)bufplc;
    for (;cnt>1;cnt--)
    {
        ch = gbuf[(bx>>16)*glogy+(by>>16)];
        if (ch != 255)(*(char *)p) = gpal[ch];
        bx += gbxinc;
        by += gbyinc;
        p += bpl;
    }
}

void tsetupspritevline(intptr_t paloffs, int bxinc, int byinc, int ysiz)
{
    gpal = (char *)paloffs;
    gbxinc = bxinc;
    gbyinc = byinc;
    glogy = ysiz;
}
void tspritevline(int bx, int by, int cnt, intptr_t bufplc, intptr_t p)
{
    char ch;

    gbuf = (char *)bufplc;
    if (transmode)
    {
        for (;cnt>1;cnt--)
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
        for (;cnt>1;cnt--)
        {
            ch = gbuf[(bx>>16)*glogy+(by>>16)];
            if (ch != 255) *((char *)p) = gtrans[((*((char *)p))<<8)+gpal[ch]];
            bx += gbxinc;
            by += gbyinc;
            p += bpl;
        }
    }
}

void setupdrawslab(int dabpl, intptr_t pal)
{ bpl = dabpl; gpal = (char *)pal; }

void drawslab(int dx, int v, int dy, int vi, intptr_t vptr, intptr_t p)
{
    int x;

    while (dy > 0)
    {
        for (x=0;x<dx;x++) *(char *)(p+x) = gpal[(int)(*(char *)((v>>16)+vptr))];
        p += bpl; v += vi; dy--;
    }
}

void stretchhline(intptr_t p0, int u, int cnt, int uinc, intptr_t rptr, intptr_t p)
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

