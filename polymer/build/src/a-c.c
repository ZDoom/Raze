// A.ASM replacement using C
// Mainly by Ken Silverman, with things melded with my port by
// Jonathon Fowler (jonof@edgenetwork.org)
//
// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.

#include "a.h"

#ifndef ENGINE_USING_A_C
# error A.H has indicated that optimized code has been requested. This means a-c.c is being compiled \
	when USE_A_C has not been defined.
#endif

long krecip(long num);	// from engine.c

#define BITSOFPRECISION 3
#define BITSOFPRECISIONPOW 8

extern long asm1, asm2, asm3, asm4, fpuasm, globalx3, globaly3;
extern void *reciptable;

static long bpl, transmode = 0;
static long glogx, glogy, gbxinc, gbyinc, gpinc;
static char *gbuf, *gpal, *ghlinepal, *gtrans;

	//Global variable functions
void setvlinebpl(long dabpl) { bpl = dabpl; }
void fixtransluscence(long datransoff) { gtrans = (char *)datransoff; }
void settransnormal(void) { transmode = 0; }
void settransreverse(void) { transmode = 1; }


	//Ceiling/floor horizontal line functions
void sethlinesizes(long logx, long logy, long bufplc)
	{ glogx = logx; glogy = logy; gbuf = (char *)bufplc; }
void setpalookupaddress(char *paladdr) { ghlinepal = paladdr; }
void setuphlineasm4(long bxinc, long byinc) { gbxinc = bxinc; gbyinc = byinc; }
void hlineasm4(long cnt, long skiploadincs, long paloffs, unsigned long by, unsigned long bx, long p)
{
	char *palptr;

	palptr = (char *)&ghlinepal[paloffs];
	if (!skiploadincs) { gbxinc = asm1; gbyinc = asm2; }
	for(;cnt>=0;cnt--)
	{
		*((char *)p) = palptr[gbuf[((bx>>(32-glogx))<<glogy)+(by>>(32-glogy))]];
		bx -= gbxinc;
		by -= gbyinc;
		p--;
	}
}


	//Sloped ceiling/floor vertical line functions
void setupslopevlin(long logylogx, long bufplc, long pinc)
{
	glogx = (logylogx&255); glogy = (logylogx>>8);
	gbuf = (char *)bufplc; gpinc = pinc;
}
void slopevlin(long p, long i, long slopaloffs, long cnt, long bx, long by)
{
	long *slopalptr, bz, bzinc;
	unsigned long u, v;

	bz = asm3; bzinc = (asm1>>3);
	slopalptr = (long *)slopaloffs;
	for(;cnt>0;cnt--)
	{
		i = krecip(bz>>6); bz += bzinc;
		u = bx+globalx3*i;
		v = by+globaly3*i;
		(*(char *)p) = *(char *)(slopalptr[0]+gbuf[((u>>(32-glogx))<<glogy)+(v>>(32-glogy))]);
		slopalptr--;
		p += gpinc;
	}
}


	//Wall,face sprite/wall sprite vertical line functions
void setupvlineasm(long neglogy) { glogy = neglogy; }
void vlineasm1(long vinc, long paloffs, long cnt, unsigned long vplc, long bufplc, long p)
{
	gbuf = (char *)bufplc;
	gpal = (char *)paloffs;
	for(;cnt>=0;cnt--)
	{
		*((char *)p) = gpal[gbuf[vplc>>glogy]];
		p += bpl;
		vplc += vinc;
	}
}

void setupmvlineasm(long neglogy) { glogy = neglogy; }
void mvlineasm1(long vinc, long paloffs, long cnt, unsigned long vplc, long bufplc, long p)
{
	char ch;

	gbuf = (char *)bufplc;
	gpal = (char *)paloffs;
	for(;cnt>=0;cnt--)
	{
		ch = gbuf[vplc>>glogy]; if (ch != 255) *((char *)p) = gpal[ch];
		p += bpl;
		vplc += vinc;
	}
}

void setuptvlineasm(long neglogy) { glogy = neglogy; }
void tvlineasm1(long vinc, long paloffs, long cnt, unsigned long vplc, long bufplc, long p)
{
	char ch;

	gbuf = (char *)bufplc;
	gpal = (char *)paloffs;
	if (transmode)
	{
		for(;cnt>=0;cnt--)
		{
			ch = gbuf[vplc>>glogy];
			if (ch != 255) *((char *)p) = gtrans[(*((char *)p))+(gpal[ch]<<8)];
			p += bpl;
			vplc += vinc;
		}
	}
	else
	{
		for(;cnt>=0;cnt--)
		{
			ch = gbuf[vplc>>glogy];
			if (ch != 255) *((char *)p) = gtrans[((*((char *)p))<<8)+gpal[ch]];
			p += bpl;
			vplc += vinc;
		}
	}
}

	//Floor sprite horizontal line functions
void msethlineshift(long logx, long logy) { glogx = logx; glogy = logy; }
void mhline(long bufplc, unsigned long bx, long cntup16, long junk, unsigned long by, long p)
{
	char ch;

	gbuf = (char *)bufplc;
	gpal = (char *)asm3;
	for(cntup16>>=16;cntup16>0;cntup16--)
	{
		ch = gbuf[((bx>>(32-glogx))<<glogy)+(by>>(32-glogy))];
		if (ch != 255) *((char *)p) = gpal[ch];
		bx += asm1;
		by += asm2;
		p++;
	}
}

void tsethlineshift(long logx, long logy) { glogx = logx; glogy = logy; }
void thline(long bufplc, unsigned long bx, long cntup16, long junk, unsigned long by, long p)
{
	char ch;

	gbuf = (char *)bufplc;
	gpal = (char *)asm3;
	if (transmode)
	{
		for(cntup16>>=16;cntup16>0;cntup16--)
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
		for(cntup16>>=16;cntup16>0;cntup16--)
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
void setupspritevline(long paloffs, long bxinc, long byinc, long ysiz)
{
	gpal = (char *)paloffs;
	gbxinc = bxinc;
	gbyinc = byinc;
	glogy = ysiz;
}
void spritevline(long bx, long by, long cnt, long bufplc, long p)
{
	gbuf = (char *)bufplc;
	for(;cnt>1;cnt--)
	{
		(*(char *)p) = gpal[gbuf[(bx>>16)*glogy+(by>>16)]];
		bx += gbxinc;
		by += gbyinc;
		p += bpl;
	}
}

	//Rotatesprite vertical line functions
void msetupspritevline(long paloffs, long bxinc, long byinc, long ysiz)
{
	gpal = (char *)paloffs;
	gbxinc = bxinc;
	gbyinc = byinc;
	glogy = ysiz;
}
void mspritevline(long bx, long by, long cnt, long bufplc, long p)
{
	char ch;

	gbuf = (char *)bufplc;
	for(;cnt>1;cnt--)
	{
		ch = gbuf[(bx>>16)*glogy+(by>>16)];
		if (ch != 255) (*(char *)p) = gpal[ch];
		bx += gbxinc;
		by += gbyinc;
		p += bpl;
	}
}

void tsetupspritevline(long paloffs, long bxinc, long byinc, long ysiz)
{
	gpal = (char *)paloffs;
	gbxinc = bxinc;
	gbyinc = byinc;
	glogy = ysiz;
}
void tspritevline(long bx, long by, long cnt, long bufplc, long p)
{
	char ch;

	gbuf = (char *)bufplc;
	if (transmode)
	{
		for(;cnt>1;cnt--)
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
		for(;cnt>1;cnt--)
		{
			ch = gbuf[(bx>>16)*glogy+(by>>16)];
			if (ch != 255) *((char *)p) = gtrans[((*((char *)p))<<8)+gpal[ch]];
			bx += gbxinc;
			by += gbyinc;
			p += bpl;
		}
	}
}

void setupdrawslab (long dabpl, long pal)
	{ bpl = dabpl; gpal = (char *)pal; }

void drawslab (long dx, long v, long dy, long vi, long vptr, long p)
{
	long x;
	
	while (dy > 0)
	{
		for(x=0;x<dx;x++) *(char *)(p+x) = gpal[(long)(*(char *)((v>>16)+vptr))];
		p += bpl; v += vi; dy--;
	}
}

void stretchhline (long p0, long u, long cnt, long uinc, long rptr, long p)
{
	p0 = p-(cnt<<2);
	do
	{
		p--;
		*(char *)p = *(char *)((u>>16)+rptr); u -= uinc;
	} while (p > p0);
}


void mmxoverlay() { }

/*
 * vim:ts=4:
 */

