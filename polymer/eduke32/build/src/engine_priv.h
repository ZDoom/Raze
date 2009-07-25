#ifndef ENGINE_PRIV_H
#define ENGINE_PRIV_H

#define MAXCLIPNUM 1024
#define MAXPERMS 512
#define MAXTILEFILES 256
#define MAXYSAVES ((MAXXDIM*MAXSPRITES)>>7)
#define MAXNODESPERLINE 42   //Warning: This depends on MAXYSAVES & MAXYDIM!
#define MAXCLIPDIST 1024

extern char pow2char[8];
extern int32_t pow2int[32];

extern int16_t thesector[MAXWALLSB], thewall[MAXWALLSB];
extern int16_t bunchfirst[MAXWALLSB], bunchlast[MAXWALLSB];
extern int16_t maskwall[MAXWALLSB], maskwallcnt;
extern spritetype *tspriteptr[MAXSPRITESONSCREEN];
extern int32_t xdimen, xdimenrecip, halfxdimen, xdimenscale, xdimscale, ydimen;
extern intptr_t frameoffset;
extern int32_t globalposx, globalposy, globalposz, globalhoriz;
extern int16_t globalang, globalcursectnum;
extern int32_t globalpal, cosglobalang, singlobalang;
extern int32_t cosviewingrangeglobalang, sinviewingrangeglobalang;
extern int32_t globalvisibility;
extern int32_t xyaspect;
extern intptr_t asm1, asm2, asm3, asm4;
extern int32_t globalshade;
extern int16_t globalpicnum;
extern int32_t globalx1, globaly2;
extern int32_t globalorientation;

extern int16_t searchit;
extern int32_t searchx, searchy;
extern int16_t searchsector, searchwall, searchstat;
extern int16_t searchbottomwall;

extern char inpreparemirror;

extern int32_t curbrightness, gammabrightness;
extern char britable[16][256];
extern char picsiz[MAXTILES];
extern int32_t lastx[MAXYDIM];
extern char *transluc;
extern int16_t sectorborder[256], sectorbordercnt;
extern int32_t qsetmode;
extern int32_t hitallsprites;

extern int32_t xb1[MAXWALLSB];
extern int32_t rx1[MAXWALLSB], ry1[MAXWALLSB];
extern int16_t p2[MAXWALLSB];
extern int16_t numscans, numhits, numbunches;

#ifdef USE_OPENGL
extern palette_t palookupfog[MAXPALOOKUPS];
#endif

int32_t wallmost(int16_t *mostbuf, int32_t w, int32_t sectnum, char dastat);
int32_t wallfront(int32_t l1, int32_t l2);
int32_t animateoffs(int16_t tilenum, int16_t fakevar);

extern int32_t indrawroomsandmasks;


#if defined(_MSC_VER) && !defined(NOASM)

static inline void setgotpic(int32_t a)
{
	_asm {
		push ebx
		mov eax, a
		mov ebx, eax
		cmp byte ptr walock[eax], 200
		jae skipit
		mov byte ptr walock[eax], 199
skipit:
		shr eax, 3
		and ebx, 7
		mov dl, byte ptr gotpic[eax]
		mov bl, byte ptr pow2char[ebx]
		or dl, bl
		mov byte ptr gotpic[eax], dl
		pop ebx
	}
}

#elif defined(__GNUC__) && defined(__i386__) && !defined(NOASM)	// _MSC_VER

#define setgotpic(a) \
({ int32_t __a=(a); \
	__asm__ __volatile__ ( \
			       "movl %%eax, %%ebx\n\t" \
			       "cmpb $200, "ASMSYM("walock")"(%%eax)\n\t" \
			       "jae 0f\n\t" \
			       "movb $199, "ASMSYM("walock")"(%%eax)\n\t" \
			       "0:\n\t" \
			       "shrl $3, %%eax\n\t" \
			       "andl $7, %%ebx\n\t" \
			       "movb "ASMSYM("gotpic")"(%%eax), %%dl\n\t" \
			       "movb "ASMSYM("pow2char")"(%%ebx), %%bl\n\t" \
			       "orb %%bl, %%dl\n\t" \
			       "movb %%dl, "ASMSYM("gotpic")"(%%eax)" \
			       : "=a" (__a) : "a" (__a) \
			       : "ebx", "edx", "memory", "cc"); \
				       __a; })

#else	// __GNUC__ && __i386__

static inline void setgotpic(int32_t tilenume)
{
	if (walock[tilenume] < 200) walock[tilenume] = 199;
	gotpic[tilenume>>3] |= pow2char[tilenume&7];
}

#endif

#endif	/* ENGINE_PRIV_H */
