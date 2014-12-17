
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

#ifdef __cplusplus
extern "C" {
#endif

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
    vec2_t ConsoleExtent;

    L2Enhance();
    CON_EnableGecko(1, 1);
    Bprintf("Console started\n");
    fatInit(28, true);

    // init the console for the title bar
    CON_InitEx(vmode, 0, 12, vmode->viWidth, 68);
    ConsoleColor(Blue, 0, White, 0);

    CON_GetMetrics(&ConsoleExtent.x, &ConsoleExtent.y);
    print_centered(ConsoleExtent.x, AppProperName, s_buildRev);
    print_centered(ConsoleExtent.x, "Compiled", s_buildTimestamp);

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
