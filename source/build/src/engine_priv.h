#pragma once

#ifndef ENGINE_PRIV_H
#define ENGINE_PRIV_H

#define MAXPERMS 512
#define MAXARTFILES_BASE 200
#define MAXARTFILES_TOTAL 220
#define MAXCLIPDIST 1024

// Uncomment to clear the screen before each top-level draw (classic only).
// FIXME: doesn't work with mirrors.
//#define ENGINE_CLEAR_SCREEN

#ifdef YAX_ENABLE
# define YAX_MAXDRAWS 8
#endif

#if !defined(NOASM) && defined __cplusplus
extern "C" {
#endif
    extern intptr_t asm1, asm2, asm3, asm4;
    extern int32_t globalx1, globaly2;
#if !defined(NOASM) && defined __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern uint16_t ATTRIBUTE((used)) sqrtable[4096], ATTRIBUTE((used)) shlookup[4096+256];

#if defined(_MSC_VER) && !defined(NOASM)

    //
    // Microsoft C Inline Assembly Routines
    //

    static inline int32_t nsqrtasm(int32_t a)
    {
        _asm
        {
            push ebx
            mov eax, a
            test eax, 0xff000000
            mov ebx, eax
            jnz short over24
            shr ebx, 12
            mov cx, word ptr shlookup[ebx*2]
            jmp short under24
            over24 :
            shr ebx, 24
                mov cx, word ptr shlookup[ebx*2+8192]
                under24 :
                shr eax, cl
                mov cl, ch
                mov ax, word ptr sqrtable[eax*2]
                shr eax, cl
                pop ebx
        }
    }

    static inline int32_t getclipmask(int32_t a, int32_t b, int32_t c, int32_t d)
    {
        _asm
        {
            push ebx
            mov eax, a
            mov ebx, b
            mov ecx, c
            mov edx, d
            sar eax, 31
            add ebx, ebx
            adc eax, eax
            add ecx, ecx
            adc eax, eax
            add edx, edx
            adc eax, eax
            mov ebx, eax
            shl ebx, 4
            or al, 0xf0
            xor eax, ebx
            pop ebx
        }
    }

    static inline int32_t getkensmessagecrc(void *b)
    {
        _asm
        {
            push ebx
            mov ebx, b
            xor eax, eax
            mov ecx, 32
            beg:
            mov edx, dword ptr[ebx+ecx*4-4]
                ror edx, cl
                adc eax, edx
                bswap eax
                loop short beg
                pop ebx
        }
    }

#elif defined(__GNUC__) && defined(__i386__) && !defined(NOASM)	// _MSC_VER

    //
    // GCC "Inline" Assembly Routines
    //

#define nsqrtasm(a) \
    ({ int32_t __r, __a=(a); \
       __asm__ __volatile__ ( \
        "testl $0xff000000, %%eax\n\t" \
        "movl %%eax, %%ebx\n\t" \
        "jnz 0f\n\t" \
        "shrl $12, %%ebx\n\t" \
        "movw " ASMSYM("shlookup") "(,%%ebx,2), %%cx\n\t" \
        "jmp 1f\n\t" \
        "0:\n\t" \
        "shrl $24, %%ebx\n\t" \
        "movw (" ASMSYM("shlookup") "+8192)(,%%ebx,2), %%cx\n\t" \
        "1:\n\t" \
        "shrl %%cl, %%eax\n\t" \
        "movb %%ch, %%cl\n\t" \
        "movw " ASMSYM("sqrtable") "(,%%eax,2), %%ax\n\t" \
        "shrl %%cl, %%eax" \
        : "=a" (__r) : "a" (__a) : "ebx", "ecx", "cc"); \
     __r; })

#define getclipmask(a,b,c,d) \
    ({ int32_t __a=(a), __b=(b), __c=(c), __d=(d); \
       __asm__ __volatile__ ("sarl $31, %%eax; addl %%ebx, %%ebx; adcl %%eax, %%eax; " \
                "addl %%ecx, %%ecx; adcl %%eax, %%eax; addl %%edx, %%edx; " \
                "adcl %%eax, %%eax; movl %%eax, %%ebx; shl $4, %%ebx; " \
                "orb $0xf0, %%al; xorl %%ebx, %%eax" \
        : "=a" (__a), "=b" (__b), "=c" (__c), "=d" (__d) \
        : "a" (__a), "b" (__b), "c" (__c), "d" (__d) : "cc"); \
     __a; })


#define getkensmessagecrc(b) \
    ({ int32_t __a, __b=(b); \
       __asm__ __volatile__ ( \
        "xorl %%eax, %%eax\n\t" \
        "movl $32, %%ecx\n\t" \
        "0:\n\t" \
        "movl -4(%%ebx,%%ecx,4), %%edx\n\t" \
        "rorl %%cl, %%edx\n\t" \
        "adcl %%edx, %%eax\n\t" \
        "bswapl %%eax\n\t" \
        "loop 0b" \
        : "=a" (__a) : "b" (__b) : "ecx", "edx" \
     __a; })

#else   // __GNUC__ && __i386__

    static inline int32_t nsqrtasm(uint32_t a)
    {
        // JBF 20030901: This was a damn lot simpler to reverse engineer than
        // msqrtasm was. Really, it was just like simplifying an algebra equation.
        uint16_t c;

        if (a & 0xff000000)  			// test eax, 0xff000000  /  jnz short over24
        {
            c = shlookup[(a >> 24) + 4096];	// mov ebx, eax
                                            // over24: shr ebx, 24
                                            // mov cx, word ptr shlookup[ebx*2+8192]
        }
        else
        {
            c = shlookup[a >> 12];		// mov ebx, eax
                                        // shr ebx, 12
                                        // mov cx, word ptr shlookup[ebx*2]
                                        // jmp short under24
        }
        a >>= c&0xff;				// under24: shr eax, cl
        a = (a&0xffff0000)|(sqrtable[a]);	// mov ax, word ptr sqrtable[eax*2]
        a >>= ((c&0xff00) >> 8);		// mov cl, ch
                                        // shr eax, cl
        return a;
    }

    static inline int32_t getclipmask(int32_t a, int32_t b, int32_t c, int32_t d)
    {
        // Ken did this
        d = ((a<0)<<3) + ((b<0)<<2) + ((c<0)<<1) + (d<0);
        return (((d<<4)^0xf0)|d);
    }

    inline int32_t getkensmessagecrc(int32_t b)
    {
        UNREFERENCED_PARAMETER(b);
        return 0x56c764d4l;
    }

#endif

extern int16_t thesector[MAXWALLSB], thewall[MAXWALLSB];
extern int16_t bunchfirst[MAXWALLSB], bunchlast[MAXWALLSB];
extern int16_t maskwall[MAXWALLSB], maskwallcnt;
extern uspritetype *tspriteptr[MAXSPRITESONSCREEN + 1];
extern int32_t xdimen, xdimenrecip, halfxdimen, xdimenscale, xdimscale, ydimen;
extern float fxdimen;
extern intptr_t frameoffset;
extern int32_t globalposx, globalposy, globalposz, globalhoriz;
extern float fglobalposx, fglobalposy, fglobalposz;
extern int16_t globalang, globalcursectnum;
extern int32_t globalpal, cosglobalang, singlobalang;
extern int32_t cosviewingrangeglobalang, sinviewingrangeglobalang;
extern int32_t globalhisibility, globalpisibility, globalcisibility;
extern int32_t globvis, globalvisibility;
extern int32_t xyaspect;
extern int32_t globalshade;
extern int16_t globalpicnum;

extern int32_t globalorientation;

extern int16_t editstatus;

extern int16_t searchit;
extern int32_t searchx, searchy;
extern int16_t searchsector, searchwall, searchstat;
extern int16_t searchbottomwall, searchisbottom;

extern char inpreparemirror;

extern char picsiz[MAXTILES];
extern int16_t sectorborder[256];
extern int32_t qsetmode;
extern int32_t hitallsprites;

extern int32_t xb1[MAXWALLSB];
extern int32_t rx1[MAXWALLSB], ry1[MAXWALLSB];
extern int16_t bunchp2[MAXWALLSB];
extern int16_t numscans, numbunches;
extern int32_t rxi[8], ryi[8];

#ifdef USE_OPENGL

// For GL_EXP2 fog:
#define FOGSCALE 0.0000768f

void calc_and_apply_fog(int32_t tile, int32_t shade, int32_t vis, int32_t pal);
void calc_and_apply_fog_factor(int32_t tile, int32_t shade, int32_t vis, int32_t pal, float factor);
#endif

extern void get_wallspr_points(uspritetype const * const spr, int32_t *x1, int32_t *x2,
    int32_t *y1, int32_t *y2);
extern void get_floorspr_points(uspritetype const * const spr, int32_t px, int32_t py,
    int32_t *x1, int32_t *x2, int32_t *x3, int32_t *x4,
    int32_t *y1, int32_t *y2, int32_t *y3, int32_t *y4);


// int32_t wallmost(int16_t *mostbuf, int32_t w, int32_t sectnum, char dastat);
int32_t wallfront(int32_t l1, int32_t l2);

void set_globalang(int16_t ang);

#ifdef DEBUGGINGAIDS
int32_t animateoffs(int const tilenum, int fakevar);
#define DO_TILE_ANIM(Picnum, Fakevar) do { \
        if (picanm[Picnum].sf&PICANM_ANIMTYPE_MASK) Picnum += animateoffs(Picnum, Fakevar); \
    } while (0)
#else
int32_t animateoffs(int const tilenum);
#define DO_TILE_ANIM(Picnum, Fakevar) do { \
        if (picanm[Picnum].sf&PICANM_ANIMTYPE_MASK) Picnum += animateoffs(Picnum); \
    } while (0)
#endif

static FORCE_INLINE int32_t bad_tspr(const uspritetype *tspr)
{
    // NOTE: tspr->owner >= MAXSPRITES (could be model) has to be handled by
    // caller.
    return (tspr->owner < 0 || (unsigned)tspr->picnum >= MAXTILES);
}

//
// getpalookup (internal)
//
static FORCE_INLINE int32_t getpalookup(int32_t davis, int32_t dashade)
{
    return min(max(dashade + (davis >> 8), 0), numshades - 1);
}

static FORCE_INLINE int32_t getpalookupsh(int32_t davis) { return getpalookup(davis, globalshade) << 8; }

void dorotspr_handle_bit2(int32_t *sx, int32_t *sy, int32_t *z, int32_t dastat,
                          int32_t cx1_plus_cx2, int32_t cy1_plus_cy2,
                          int32_t *ret_ouryxaspect, int32_t *ret_ourxyaspect);

////// yax'y stuff //////
#ifdef USE_OPENGL
extern void polymost_scansector(int32_t sectnum);
#endif
int32_t engine_addtsprite(int16_t z, int16_t sectnum);
#ifdef YAX_ENABLE
extern int32_t g_nodraw, scansector_retfast;
extern int32_t yax_globallev, yax_globalbunch;
extern int32_t yax_globalcf, yax_nomaskpass, yax_nomaskdidit;
extern uint8_t haveymost[YAX_MAXBUNCHES>>3];
extern uint8_t yax_gotsector[MAXSECTORS>>3];

static FORCE_INLINE int32_t yax_isislandwall(int32_t line, int32_t cf) { return (yax_vnextsec(line, cf) >= 0); }
#endif

#ifdef YAX_DEBUG
extern char m32_debugstr[64][128];
extern int32_t m32_numdebuglines;
# define yaxdebug(fmt, ...)  do { if (m32_numdebuglines<64) Bsnprintf(m32_debugstr[m32_numdebuglines++], 128, fmt, ##__VA_ARGS__); } while (0)
# define yaxprintf(fmt, ...) do { initprintf(fmt, ##__VA_ARGS__); } while (0)
#else
# define yaxdebug(fmt, ...)
# define yaxprintf(fmt, ...)
#endif



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
			       "cmpb $200, " ASMSYM("walock") "(%%eax)\n\t" \
			       "jae 0f\n\t" \
			       "movb $199, " ASMSYM("walock") "(%%eax)\n\t" \
			       "0:\n\t" \
			       "shrl $3, %%eax\n\t" \
			       "andl $7, %%ebx\n\t" \
			       "movb " ASMSYM("gotpic") "(%%eax), %%dl\n\t" \
			       "movb " ASMSYM("pow2char") "(%%ebx), %%bl\n\t" \
			       "orb %%bl, %%dl\n\t" \
			       "movb %%dl, " ASMSYM("gotpic") "(%%eax)" \
			       : "=a" (__a) : "a" (__a) \
			       : "ebx", "edx", "memory", "cc"); \
				       __a; })

#else	// __GNUC__ && __i386__

static FORCE_INLINE void setgotpic(int32_t tilenume)
{
	if (walock[tilenume] < 200) walock[tilenume] = 199;
	gotpic[tilenume>>3] |= pow2char[tilenume&7];
}

#endif

// Get properties of parallaxed sky to draw.
// Returns: pointer to tile offset array. Sets-by-pointer the other three.
static FORCE_INLINE const int8_t *getpsky(int32_t picnum, int32_t *dapyscale, int32_t *dapskybits, int32_t *dapyoffs, int32_t *daptileyscale)
{
    psky_t const * const psky = &multipsky[getpskyidx(picnum)];

    if (dapskybits)
        *dapskybits = (pskybits_override == -1 ? psky->lognumtiles : pskybits_override);
    if (dapyscale)
        *dapyscale = (parallaxyscale_override == 0 ? psky->horizfrac : parallaxyscale_override);
    if (dapyoffs)
        *dapyoffs = psky->yoffs + parallaxyoffs_override;
    if (daptileyscale)
        *daptileyscale = psky->yscale;

    return psky->tileofs;
}

static FORCE_INLINE void set_globalpos(int32_t const x, int32_t const y, int32_t const z)
{
    globalposx  = x, fglobalposx = (float)x;
    globalposy  = y, fglobalposy = (float)y;
    globalposz  = z, fglobalposz = (float)z;
}

#ifdef __cplusplus
}
#endif

#endif	/* ENGINE_PRIV_H */
