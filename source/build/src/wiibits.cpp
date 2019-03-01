
#include "wiibits.h"
#include "baselayer.h"
#include "common.h"

#include <ogc/system.h>
#include <ogc/video.h>
#include <ogc/video_types.h>
#include <ogc/gx.h>
#include <ogc/pad.h>
#include <ogc/consol.h>
#include <ogc/lwp.h>
#include <ogc/lwp_watchdog.h>
#include <ogc/lwp_threads.h>
#include <ogc/ios.h>
#include <ogc/color.h>
#include <gctypes.h> // for bool

#include "vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

extern buildvfs_FILE osdlog;

extern void L2Enhance();
extern void CON_EnableGecko(int channel,int safe);
extern bool fatInit(uint32_t cacheSize, bool setAsDefaultDevice);

extern void WII_InitVideoSystem(void);

extern unsigned char *xfb;
extern GXRModeObj *vmode;

#ifdef __cplusplus
}
#endif

enum
{
    Black = 0,
    Red = 1,
    Green = 2,
    Yellow = 3,
    Blue = 4,
    Magenta = 5,
    Cyan = 6,
    White = 7
};

static void ConsoleColor(uint8_t bgcolor, uint8_t bgbold, uint8_t fgcolor, uint8_t fgbold)
{
    Bprintf("\x1b[%u;%dm\x1b[%u;%dm", fgcolor + 30, fgbold, bgcolor + 40, bgbold);
}

static void print_centered(const int32_t width, const char *part1, const char *part2)
{
    const int32_t length = Bstrlen(part1) + Bstrlen(part2) + 1;
    const int32_t leftbuf = (width-1 - length) / 2;
    const int32_t rightbuf = width-1 - leftbuf - length;

    Bprintf("%*s%s %s%*s\n", leftbuf, " ", part1, part2, rightbuf, " ");
}

void wii_open(void)
{
    struct { int x, y; } ConsoleExtent;

    L2Enhance();
    CON_EnableGecko(1, 1);
    Bprintf("Console started\n");
    fatInit(28, true);

    // init the console for the title bar
    CON_InitEx(vmode, 0, 12, vmode->viWidth, 68);
    ConsoleColor(Blue, 0, White, 0);

    CON_GetMetrics(&ConsoleExtent.x, &ConsoleExtent.y);
    print_centered(ConsoleExtent.x, AppProperName, s_buildRev);
    print_centered(ConsoleExtent.x, "Built", s_buildTimestamp);

    VIDEO_WaitVSync();
    if (vmode->viTVMode&VI_NON_INTERLACE)
        VIDEO_WaitVSync();

    // reinit console below the title bar
    CON_InitEx(vmode, 8, 50, vmode->viWidth - 16, vmode->viHeight - 62);
    ConsoleColor(Black, 0, White, 0);
}

// Reset the video system to remove the startup console.
void wii_initgamevideo(void)
{
    WII_InitVideoSystem();
}

/*
 *  linux/lib/vsprintf.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/* vsprintf.c -- Lars Wirzenius & Linus Torvalds. */
/*
 * Wirzenius wrote this portably, Torvalds fucked it up :-)
 */

/* we use this so that we can do without the ctype library */
#define is_digit(c) ((c) >= '0' && (c) <= '9')

static int skip_atoi(const char **s)
{
    int i=0;

    while (is_digit(**s))
        i = i*10 + *((*s)++) - '0';
    return i;
}

#define ZEROPAD 1       /* pad with zero */
#define SIGN    2       /* unsigned/signed long */
#define PLUS    4       /* show plus */
#define SPACE   8       /* space if plus */
#define LEFT    16      /* left justified */
#define SPECIAL 32      /* 0x */
#define LARGE   64      /* use 'ABCDEF' instead of 'abcdef' */

#define do_div(n,base) ({ \
int __res; \
__res = ((unsigned long) n) % (unsigned) base; \
n = ((unsigned long) n) / (unsigned) base; \
__res; })

static char * number(char * str, long num, int base, int size, int precision
    ,int type)
{
    char c,sign,tmp[66];
    const char *digits="0123456789abcdefghijklmnopqrstuvwxyz";
    int i;

    if (type & LARGE)
        digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (type & LEFT)
        type &= ~ZEROPAD;
    if (base < 2 || base > 36)
        return 0;
    c = (type & ZEROPAD) ? '0' : ' ';
    sign = 0;
    if (type & SIGN) {
        if (num < 0) {
            sign = '-';
            num = -num;
            size--;
        } else if (type & PLUS) {
            sign = '+';
            size--;
        } else if (type & SPACE) {
            sign = ' ';
            size--;
        }
    }
    if (type & SPECIAL) {
        if (base == 16)
            size -= 2;
        else if (base == 8)
            size--;
    }
    i = 0;
    if (num == 0)
        tmp[i++]='0';
    else while (num != 0)
        tmp[i++] = digits[do_div(num,base)];
    if (i > precision)
        precision = i;
    size -= precision;
    if (!(type&(ZEROPAD+LEFT)))
        while(size-->0)
            *str++ = ' ';
    if (sign)
        *str++ = sign;
    if (type & SPECIAL) {
        if (base==8)
            *str++ = '0';
        else if (base==16) {
            *str++ = '0';
            *str++ = digits[33];
        }
    }
    if (!(type & LEFT))
        while (size-- > 0)
            *str++ = c;
    while (i < precision--)
        *str++ = '0';
    while (i-- > 0)
        *str++ = tmp[i];
    while (size-- > 0)
        *str++ = ' ';
    return str;
}

static int exception_vsprintf(char *buf, const char *fmt, va_list args)
{
    int len;
    unsigned long num;
    int i, base;
    char * str;
    const char *s;

    int flags;      /* flags to number() */

    int field_width;    /* width of output field */
    int precision;      /* min. # of digits for integers; max
                   number of chars for from string */
    int qualifier;      /* 'h', 'l', or 'L' for integer fields */

    for (str=buf ; *fmt ; ++fmt) {
        if (*fmt != '%') {
            *str++ = *fmt;
            continue;
        }

        /* process flags */
        flags = 0;
        repeat:
            ++fmt;      /* this also skips first '%' */
            switch (*fmt) {
                case '-': flags |= LEFT; goto repeat;
                case '+': flags |= PLUS; goto repeat;
                case ' ': flags |= SPACE; goto repeat;
                case '#': flags |= SPECIAL; goto repeat;
                case '0': flags |= ZEROPAD; goto repeat;
                }

        /* get field width */
        field_width = -1;
        if (is_digit(*fmt))
            field_width = skip_atoi(&fmt);
        else if (*fmt == '*') {
            ++fmt;
            /* it's the next argument */
            field_width = va_arg(args, int);
            if (field_width < 0) {
                field_width = -field_width;
                flags |= LEFT;
            }
        }

        /* get the precision */
        precision = -1;
        if (*fmt == '.') {
            ++fmt;
            if (is_digit(*fmt))
                precision = skip_atoi(&fmt);
            else if (*fmt == '*') {
                ++fmt;
                /* it's the next argument */
                precision = va_arg(args, int);
            }
            if (precision < 0)
                precision = 0;
        }

        /* get the conversion qualifier */
        qualifier = -1;
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
            qualifier = *fmt;
            ++fmt;
        }

        /* default base */
        base = 10;

        switch (*fmt) {
        case 'c':
            if (!(flags & LEFT))
                while (--field_width > 0)
                    *str++ = ' ';
            *str++ = (unsigned char) va_arg(args, int);
            while (--field_width > 0)
                *str++ = ' ';
            continue;

        case 's':
            s = va_arg(args, char *);
            if (!s)
                s = "<NULL>";

            len = strnlen(s, precision);

            if (!(flags & LEFT))
                while (len < field_width--)
                    *str++ = ' ';
            for (i = 0; i < len; ++i)
                *str++ = *s++;
            while (len < field_width--)
                *str++ = ' ';
            continue;

        case 'p':
            if (field_width == -1) {
                field_width = 2*sizeof(void *);
                flags |= ZEROPAD;
            }
            str = number(str,
                (unsigned long) va_arg(args, void *), 16,
                field_width, precision, flags);
            continue;


        case 'n':
            if (qualifier == 'l') {
                long * ip = va_arg(args, long *);
                *ip = (str - buf);
            } else {
                int * ip = va_arg(args, int *);
                *ip = (str - buf);
            }
            continue;

        case '%':
            *str++ = '%';
            continue;

        /* integer number formats - set up the flags and "break" */
        case 'o':
            base = 8;
            break;

        case 'X':
            flags |= LARGE;
        case 'x':
            base = 16;
            break;

        case 'd':
        case 'i':
            flags |= SIGN;
        case 'u':
            break;

        default:
            *str++ = '%';
            if (*fmt)
                *str++ = *fmt;
            else
                --fmt;
            continue;
        }
        if (qualifier == 'l')
            num = va_arg(args, unsigned long);
        else if (qualifier == 'h') {
            num = (unsigned short) va_arg(args, int);
            if (flags & SIGN)
                num = (short) num;
        } else if (flags & SIGN)
            num = va_arg(args, int);
        else
            num = va_arg(args, unsigned int);
        str = number(str, num, base, field_width, precision, flags);
    }
    *str = '\0';
    return str-buf;
}


#define DOUTBUFSIZE 256

static int exception_output;

static void exception_printf(const char *str, ...)
{
    char outstr[DOUTBUFSIZE];

    int len;

    va_list args;

    va_start(args, str);
    len = exception_vsprintf(outstr, str, args);
    va_end(args);

    if (exception_output)
        Bfputs(outstr, osdlog);
    else
        Bwrite(2, outstr, len);
}


/*-------------------------------------------------------------

exception.c -- PPC exception handling support

Copyright (C) 2004
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)

Copyright (C) 2014
Modified for EDuke32

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.  The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.  Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.  This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/

//#define _EXC_DEBUG

#define CPU_STACK_TRACE_DEPTH       10

typedef struct _framerec {
    struct _framerec *up;
    void *lr;
} frame_rec, *frame_rec_t;

static void *exception_xfb = (void*)0xC1700000;         //we use a static address above ArenaHi.

#ifdef __cplusplus
extern "C" {
#endif

extern void udelay(int us);
extern void __reload();
extern void VIDEO_SetFramebuffer(void *);
extern void __console_init(void *framebuffer,int xstart,int ystart,int xres,int yres,int stride);

extern void __wrap_c_default_exceptionhandler(frame_context *pCtx);

#ifdef __cplusplus
}
#endif

static const char *exception_name[NUM_EXCEPTIONS] = {
        "System Reset", "Machine Check", "DSI", "ISI",
        "Interrupt", "Alignment", "Program", "Floating Point",
        "Decrementer", "System Call", "Trace", "Performance",
        "IABR", "Reserved", "Thermal"};


extern void __wrap_c_default_exceptionhandler(frame_context *pCtx);

static void _cpu_print_stack(void *pc,void *lr,void *r1)
{
    u32 i = 0;
    frame_rec_t l,p = (frame_rec_t)lr;

    l = p;
    p = (frame_rec_t)r1;
    if(!p) __asm__ __volatile__("mr %0,%%r1" : "=r"(p));

    exception_printf("\n\tSTACK DUMP:");

    for(i=0;i<CPU_STACK_TRACE_DEPTH-1 && p->up;p=p->up,i++) {
        if(i%4) exception_printf(" --> ");
        else {
            if(i>0) exception_printf(" -->\n\t");
            else exception_printf("\n\t");
        }

        switch(i) {
            case 0:
                if(pc) exception_printf("%p",pc);
                break;
            case 1:
                if(!l) l = (frame_rec_t)mfspr(8);
                exception_printf("%p",(void*)l);
                break;
            default:
                exception_printf("%p",(void*)(p->up->lr));
                break;
        }
    }
}

static void waitForReload(void)
{
    u32 level;

    exception_printf("\tPress the Reset button to return to loader.\n\n");

    while ( 1 )
    {
        if( SYS_ResetButtonDown() )
        {
            exception_printf("\tReturning to loader...\n");
            _CPU_ISR_Disable(level);
            __reload ();
        }

        udelay(20000);
    }
}

void __wrap_c_default_exceptionhandler(frame_context *pCtx)
{
    GX_AbortFrame();
    VIDEO_SetFramebuffer(exception_xfb);
    __console_init(exception_xfb,20,20,640,574,1280);
    CON_EnableGecko(1, true);

    exception_printf("\n\n\n");

    if (osdlog && osdlogfn)
    {
        exception_printf("\tAn unrecoverable error has occurred.\n\tPlease submit \"%s\" to the %s developers.\n\n", osdlogfn, AppProperName);
        exception_output = 1;
    }

    exception_printf("\tException: %s\n\n", exception_name[pCtx->EXCPT_Number]);
    exception_printf("\tGPR00 %08X GPR08 %08X GPR16 %08X GPR24 %08X\n",pCtx->GPR[0], pCtx->GPR[8], pCtx->GPR[16], pCtx->GPR[24]);
    exception_printf("\tGPR01 %08X GPR09 %08X GPR17 %08X GPR25 %08X\n",pCtx->GPR[1], pCtx->GPR[9], pCtx->GPR[17], pCtx->GPR[25]);
    exception_printf("\tGPR02 %08X GPR10 %08X GPR18 %08X GPR26 %08X\n",pCtx->GPR[2], pCtx->GPR[10], pCtx->GPR[18], pCtx->GPR[26]);
    exception_printf("\tGPR03 %08X GPR11 %08X GPR19 %08X GPR27 %08X\n",pCtx->GPR[3], pCtx->GPR[11], pCtx->GPR[19], pCtx->GPR[27]);
    exception_printf("\tGPR04 %08X GPR12 %08X GPR20 %08X GPR28 %08X\n",pCtx->GPR[4], pCtx->GPR[12], pCtx->GPR[20], pCtx->GPR[28]);
    exception_printf("\tGPR05 %08X GPR13 %08X GPR21 %08X GPR29 %08X\n",pCtx->GPR[5], pCtx->GPR[13], pCtx->GPR[21], pCtx->GPR[29]);
    exception_printf("\tGPR06 %08X GPR14 %08X GPR22 %08X GPR30 %08X\n",pCtx->GPR[6], pCtx->GPR[14], pCtx->GPR[22], pCtx->GPR[30]);
    exception_printf("\tGPR07 %08X GPR15 %08X GPR23 %08X GPR31 %08X\n",pCtx->GPR[7], pCtx->GPR[15], pCtx->GPR[23], pCtx->GPR[31]);
    exception_printf("\tLR %08X SRR0 %08x SRR1 %08x MSR %08x\n", pCtx->LR, pCtx->SRR0, pCtx->SRR1,pCtx->MSR);
    exception_printf("\tDAR %08X DSISR %08X\n", mfspr(19), mfspr(18));

    _cpu_print_stack((void*)pCtx->SRR0,(void*)pCtx->LR,(void*)pCtx->GPR[1]);

    if((pCtx->EXCPT_Number==EX_DSI) || (pCtx->EXCPT_Number==EX_FP)) {
        u32 i;
        u32 *pAdd = (u32*)pCtx->SRR0;
        exception_printf("\n\n\tCODE DUMP:\n");
        for (i=0; i<12; i+=4)
            exception_printf("\t%p:  %08X %08X %08X %08X\n",
            &(pAdd[i]),pAdd[i], pAdd[i+1], pAdd[i+2], pAdd[i+3]);
    }

    if (exception_output)
    {
        exception_output = 0;
        MAYBE_FCLOSE_AND_NULL(osdlog);
    }
    else
    {
        exception_printf("\n");
    }

    waitForReload();

    return;
}
