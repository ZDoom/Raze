// SDL interface layer
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)
//
// Use SDL 1.2 or 1.3 from http://www.libsdl.org

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS
#include <stdlib.h>
#include <math.h>  // pow
#include <signal.h>
#include "sdl_inc.h"
#include "compat.h"
#include "sdlayer.h"
#include "cache1d.h"
//#include "pragmas.h"
#include "a.h"
#include "build.h"
#include "osd.h"

#if (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION < 3) // SDL 1.2
// for SDL_WaitEventTimeout defined below
#include <SDL/SDL_events.h>
#endif

#ifdef USE_OPENGL
# include "glbuild.h"
#endif

#if defined _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include "winbits.h"
#elif defined __APPLE__
# include "osxbits.h"
# include <mach/mach.h>
# include <mach/mach_time.h>
#elif defined HAVE_GTK2
# include "gtkbits.h"
#elif defined GEKKO
# define HW_RVL
# include <ogc/lwp.h>
# include <ogc/lwp_watchdog.h>
#endif

#if !defined _WIN32 && !defined HAVE_GTK2 && !defined __APPLE__
int32_t startwin_open(void) { return 0; }
int32_t startwin_close(void) { return 0; }
int32_t startwin_puts(const char *s) { UNREFERENCED_PARAMETER(s); return 0; }
int32_t startwin_idle(void *s) { UNREFERENCED_PARAMETER(s); return 0; }
int32_t startwin_settitle(const char *s) { UNREFERENCED_PARAMETER(s); return 0; }
#endif

#if SDL_MAJOR_VERSION==2
# define SDL_GRAB_OFF SDL_FALSE
# define SDL_GRAB_ON SDL_TRUE
# define SDL_WM_GrabInput(yn) SDL_SetWindowGrab(sdl_window, yn)
#endif

/// These can be useful for debugging sometimes...
//#define SDL_WM_GrabInput(x) SDL_WM_GrabInput(SDL_GRAB_OFF)
//#define SDL_ShowCursor(x) SDL_ShowCursor(SDL_ENABLE)

#define SURFACE_FLAGS	(SDL_SWSURFACE|SDL_HWPALETTE|SDL_HWACCEL)

// undefine to restrict windowed resolutions to conventional sizes
#define ANY_WINDOWED_SIZE

// fix for mousewheel
#define MWHEELTICKS 10
static uint32_t mwheelup, mwheeldown;

extern int32_t app_main(int32_t argc, const char **argv);

char quitevent=0, appactive=1, novideo=0;

// video
static SDL_Surface *sdl_surface;
#if SDL_MAJOR_VERSION==2
static SDL_Surface *sdl_surface2;
static SDL_Palette *sdl_palptr;
static SDL_Window *sdl_window;
static SDL_Renderer *sdl_renderer;
#endif
int32_t xres=-1, yres=-1, bpp=0, fullscreen=0, bytesperline;
intptr_t frameplace=0;
int32_t lockcount=0;
char modechange=1;
char offscreenrendering=0;
char videomodereset = 0;
char nofog=0;
static uint16_t sysgamma[3][256];
extern int32_t curbrightness, gammabrightness;
#ifdef USE_OPENGL
// OpenGL stuff
char nogl=0;
#endif
int32_t vsync=0;

// last gamma, contrast, brightness
static float lastvidgcb[3];

//#define KEY_PRINT_DEBUG

#if (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION < 3)
static char keytranslation[SDLK_LAST];
#else
static char keytranslation[SDL_NUM_SCANCODES];
#endif
static int32_t buildkeytranslationtable(void);

//static SDL_Surface * loadtarga(const char *fn);		// for loading the icon
static SDL_Surface *appicon = NULL;
static SDL_Surface *loadappicon(void);

static mutex_t m_initprintf;

// Joystick dead and saturation zones
uint16_t *joydead, *joysatur;

#ifdef _WIN32
//
// win_gethinstance() -- gets the application instance
//
HINSTANCE win_gethinstance(void)
{
    return (HINSTANCE)GetModuleHandle(NULL);
}
#endif

int32_t wm_msgbox(char *name, char *fmt, ...)
{
    char buf[2048];
    va_list va;

    UNREFERENCED_PARAMETER(name);

    va_start(va,fmt);
    vsprintf(buf,fmt,va);
    va_end(va);

#if defined(__APPLE__)
    return osx_msgbox(name, buf);
#elif defined HAVE_GTK2
    if (gtkbuild_msgbox(name, buf) >= 0) return 1;
#endif
    puts(buf);
    puts("   (press Return or Enter to continue)");
    getchar();

    return 0;
}

int32_t wm_ynbox(char *name, char *fmt, ...)
{
    char buf[2048];
    char c;
    va_list va;
#if (!defined(__APPLE__) && defined(HAVE_GTK2))
    int32_t r;
#endif

    UNREFERENCED_PARAMETER(name);

    va_start(va,fmt);
    vsprintf(buf,fmt,va);
    va_end(va);

#if defined __APPLE__
    return osx_ynbox(name, buf);
#elif defined HAVE_GTK2
    if ((r = gtkbuild_ynbox(name, buf)) >= 0) return r;
#endif
    puts(buf);
    puts("   (type 'Y' or 'N', and press Return or Enter to continue)");
    do c = getchar(); while (c != 'Y' && c != 'y' && c != 'N' && c != 'n');
    if (c == 'Y' || c == 'y') return 1;

    return 0;
}

void wm_setapptitle(char *name)
{
    if (name)
        Bstrncpyz(apptitle, name, sizeof(apptitle));

#if SDL_MAJOR_VERSION == 1
    SDL_WM_SetCaption(apptitle, NULL);
#else
    SDL_SetWindowTitle(sdl_window, apptitle);
#endif

    startwin_settitle(apptitle);
}


//
//
// ---------------------------------------
//
// System
//
// ---------------------------------------
//
//

int32_t main(int32_t argc, char *argv[])
{
    int32_t r;
#ifdef USE_OPENGL
    char *argp;
#endif

    buildkeytranslationtable();

#ifdef _WIN32
    if (!CheckWinVersion())
    {
        MessageBox(0, "This application requires Windows XP or better to run.",
                   apptitle, MB_OK|MB_ICONSTOP);
        return -1;
    }

    win_open();
#endif

#ifdef HAVE_GTK2
    gtkbuild_init(&argc, &argv);
#endif
    startwin_open();

    maybe_redirect_outputs();

#ifdef USE_OPENGL
    if ((argp = Bgetenv("BUILD_NOFOG")) != NULL)
        nofog = Batol(argp);
#endif

    baselayer_init();
    r = app_main(argc, (const char **)argv);

    startwin_close();
#ifdef HAVE_GTK2
    gtkbuild_exit(r);
#endif
#ifdef _WIN32
    win_close();
#endif

    return r;
}

#ifdef USE_OPENGL
void setvsync(int32_t sync)
{
    if (vsync == sync) return;
    vsync = sync;
    resetvideomode();
    if (setgamemode(fullscreen,xdim,ydim,bpp))
        OSD_Printf("restartvid: Reset failed...\n");
}
#endif

static void attach_debugger_here(void) {}

/* XXX: libexecinfo could be used on systems without gnu libc. */
#if !defined _WIN32 && defined __GNUC__ && !defined __OpenBSD__ && !(defined __APPLE__ && defined __BIG_ENDIAN__) && !defined(GEKKO) && !defined(__ANDROID__)
# define PRINTSTACKONSEGV 1
# include <execinfo.h>
#endif

static void sighandler(int signum)
{
    UNREFERENCED_PARAMETER(signum);
//    if (signum==SIGSEGV)
    {
        SDL_WM_GrabInput(SDL_GRAB_OFF);
        SDL_ShowCursor(SDL_ENABLE);
#if PRINTSTACKONSEGV
        {
            void *addr[32];
            int32_t errfd = fileno(stderr);
            int32_t n=backtrace(addr, sizeof(addr)/sizeof(addr[0]));
            backtrace_symbols_fd(addr, n, errfd);
        }
        // This is useful for attaching the debugger post-mortem. For those pesky
        // cases where the program runs through happily when inspected from the start.
//        usleep(15000000);
#endif
        attach_debugger_here();
        app_crashhandler();
        uninitsystem();
        exit(8);
    }
}

//
// initsystem() -- init SDL systems
//
int32_t initsystem(void)
{
    /*
    #ifdef DEBUGGINGAIDS
    const SDL_VideoInfo *vid;
    #endif
    */
    SDL_version compiled;

#if SDL_MAJOR_VERSION < 2
    const SDL_version *linked = SDL_Linked_Version();
#else
    SDL_version linked_;
    const SDL_version *linked = &linked_;
    SDL_GetVersion(&linked_);
#endif

    SDL_VERSION(&compiled);

    mutex_init(&m_initprintf);

#ifdef _WIN32
    win_init();
#endif

    initprintf("Initializing SDL system interface "
               "(compiled against SDL version %d.%d.%d, found version %d.%d.%d)\n",
               compiled.major, compiled.minor, compiled.patch,
               linked->major, linked->minor, linked->patch);

    if (SDL_VERSIONNUM(linked->major,linked->minor,linked->patch) < SDL_REQUIREDVERSION)
    {
        /*reject running under SDL versions older than what is stated in sdl_inc.h */
        initprintf("You need at least v%d.%d.%d of SDL to run this game\n",SDL_MIN_X,SDL_MIN_Y,SDL_MIN_Z);
        return -1;
    }

    if (SDL_Init(SDL_INIT_VIDEO //| SDL_INIT_TIMER
#ifdef NOSDLPARACHUTE
                 | SDL_INIT_NOPARACHUTE
#endif
                ))
    {
        initprintf("Initialization failed! (%s)\nNon-interactive mode enabled\n", SDL_GetError());
        /*        if (SDL_Init(0))
                {
                    initprintf("Initialization failed! (%s)\n", SDL_GetError());
                    return -1;
                }
                else
                */
        novideo = 1;
#ifdef USE_OPENGL
        nogl = 1;
#endif
    }

    signal(SIGSEGV, sighandler);
    signal(SIGILL, sighandler);  /* clang -fcatch-undefined-behavior uses an ill. insn */
    signal(SIGABRT, sighandler);
    signal(SIGFPE, sighandler);

    atexit(uninitsystem);

    frameplace = 0;
    lockcount = 0;

#ifdef USE_OPENGL
    if (!novideo && loadgldriver(getenv("BUILD_GLDRV")))
    {
        initprintf("Failed loading OpenGL driver. GL modes will be unavailable.\n");
        nogl = 1;
    }
#endif

#if !defined(__APPLE__) && !defined(__ANDROID__)

    //icon = loadtarga("icon.tga");

    if (!novideo)
    {
        appicon = loadappicon();
        if (appicon)
        {
#if SDL_MAJOR_VERSION==1
            SDL_WM_SetIcon(appicon, 0);
#else
            SDL_SetWindowIcon(sdl_window, appicon);
#endif
        }
    }
#endif

    if (!novideo)
    {
#if SDL_MAJOR_VERSION==1
        char drvname[32];
        if (SDL_VideoDriverName(drvname, 32))
            initprintf("Using \"%s\" video driver\n", drvname);
#else
        const char *drvname = SDL_GetVideoDriver(0);
        if (drvname)
            initprintf("Using \"%s\" video driver\n", drvname);
#endif
    }

    /*
    // dump a quick summary of the graphics hardware
    #ifdef DEBUGGINGAIDS
    vid = SDL_GetVideoInfo();
    initprintf("Video device information:\n");
    initprintf("  Can create hardware surfaces?          %s\n", (vid->hw_available)?"Yes":"No");
    initprintf("  Window manager available?              %s\n", (vid->wm_available)?"Yes":"No");
    initprintf("  Accelerated hardware blits?            %s\n", (vid->blit_hw)?"Yes":"No");
    initprintf("  Accelerated hardware colourkey blits?  %s\n", (vid->blit_hw_CC)?"Yes":"No");
    initprintf("  Accelerated hardware alpha blits?      %s\n", (vid->blit_hw_A)?"Yes":"No");
    initprintf("  Accelerated software blits?            %s\n", (vid->blit_sw)?"Yes":"No");
    initprintf("  Accelerated software colourkey blits?  %s\n", (vid->blit_sw_CC)?"Yes":"No");
    initprintf("  Accelerated software alpha blits?      %s\n", (vid->blit_sw_A)?"Yes":"No");
    initprintf("  Accelerated colour fills?              %s\n", (vid->blit_fill)?"Yes":"No");
    initprintf("  Total video memory:                    %dKB\n", vid->video_mem);
    #endif
    */
    return 0;
}


//
// uninitsystem() -- uninit SDL systems
//
void uninitsystem(void)
{
    uninitinput();
    uninittimer();

    if (appicon)
    {
        SDL_FreeSurface(appicon);
        appicon = NULL;
    }

    SDL_Quit();

#ifdef _WIN32
    win_uninit();
#endif

#ifdef USE_OPENGL
    unloadgldriver();
#endif
}


//
// initprintf() -- prints a string to the intitialization window
//
void initprintf(const char *f, ...)
{
    va_list va;
    char buf[2048];
    static char dabuf[2048];

    va_start(va, f);
    Bvsnprintf(buf, sizeof(buf), f, va);
    va_end(va);

    OSD_Printf("%s", buf);
//    Bprintf("%s", buf);

    mutex_lock(&m_initprintf);
    if (Bstrlen(dabuf) + Bstrlen(buf) > 1022)
    {
        startwin_puts(dabuf);
        Bmemset(dabuf, 0, sizeof(dabuf));
    }

    Bstrcat(dabuf,buf);

    if (flushlogwindow || Bstrlen(dabuf) > 768)
    {
        startwin_puts(dabuf);
#ifndef _WIN32
        startwin_idle(NULL);
#endif
        Bmemset(dabuf, 0, sizeof(dabuf));
    }
    mutex_unlock(&m_initprintf);
}

//
// debugprintf() -- prints a debug string to stderr
//
void debugprintf(const char *f, ...)
{
#if defined DEBUGGINGAIDS && !(defined __APPLE__ && defined __BIG_ENDIAN__)
    va_list va;

    va_start(va,f);
    Bvfprintf(stderr, f, va);
    va_end(va);
#else
    UNREFERENCED_PARAMETER(f);
#endif
}


//
//
// ---------------------------------------
//
// All things Input
//
// ---------------------------------------
//
//

// static int32_t joyblast=0;
static SDL_Joystick *joydev = NULL;

//
// initinput() -- init input system
//
int32_t initinput(void)
{
    int32_t i,j;

#ifdef __APPLE__
    // force OS X to operate in >1 button mouse mode so that LMB isn't adulterated
    if (!getenv("SDL_HAS3BUTTONMOUSE")) putenv("SDL_HAS3BUTTONMOUSE=1");
#endif
    if (!remapinit)
        for (i=0; i<256; i++)
            remap[i]=i;
    remapinit=1;

#if SDL_MAJOR_VERSION==1
    if (SDL_EnableKeyRepeat(250, 30)) // doesn't do anything in 1.3
        initprintf("Error enabling keyboard repeat.\n");
    SDL_EnableUNICODE(1);	// let's hope this doesn't hit us too hard
#endif
    inputdevices = 1|2;	// keyboard (1) and mouse (2)
    mousegrab = 0;

    memset(key_names,0,sizeof(key_names));
#if (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION < 3)
    for (i=0; i<SDLK_LAST; i++)
    {
        if (!keytranslation[i]) continue;
        Bstrncpyz(key_names[ keytranslation[i] ], SDL_GetKeyName((SDLKey)i), sizeof(key_names[i]));
    }
#else
    for (i=0; i<SDL_NUM_SCANCODES; i++)
    {
        if (!keytranslation[i]) continue;
        Bstrncpyz(key_names[ keytranslation[i] ], SDL_GetKeyName(SDL_SCANCODE_TO_KEYCODE(i)), sizeof(key_names[i]));
    }
#endif

    if (!SDL_InitSubSystem(SDL_INIT_JOYSTICK))
    {
        i = SDL_NumJoysticks();
        initprintf("%d joystick(s) found\n",i);
        for (j=0; j<i; j++) initprintf("  %d. %s\n", j+1, SDL_JoystickName(j));
        joydev = SDL_JoystickOpen(0);
        if (joydev)
        {
            SDL_JoystickEventState(SDL_ENABLE);
            inputdevices |= 4;

            joynumaxes    = SDL_JoystickNumAxes(joydev);
            joynumbuttons = min(32,SDL_JoystickNumButtons(joydev));
            joynumhats    = SDL_JoystickNumHats(joydev);
            initprintf("Joystick 1 has %d axes, %d buttons, and %d hat(s).\n",
                       joynumaxes,joynumbuttons,joynumhats);

            joyaxis = (int32_t *)Bcalloc(joynumaxes, sizeof(int32_t));
            if (joynumhats)
                joyhat = (int32_t *)Bcalloc(joynumhats, sizeof(int32_t));

            for (i = 0; i < joynumhats; i++)
                joyhat[i] = -1; // centre

            joydead = (uint16_t *)Bcalloc(joynumaxes, sizeof(uint16_t));
            joysatur = (uint16_t *)Bcalloc(joynumaxes, sizeof(uint16_t));
        }
    }

    return 0;
}

//
// uninitinput() -- uninit input system
//
void uninitinput(void)
{
    uninitmouse();

    if (joydev)
    {
        SDL_JoystickClose(joydev);
        joydev = NULL;
    }
}

#ifndef GEKKO
const char *getjoyname(int32_t what, int32_t num)
{
    static char tmp[64];

    switch (what)
    {
    case 0:	// axis
        if ((unsigned)num > (unsigned)joynumaxes) return NULL;
        Bsprintf(tmp,"Axis %d",num);
        return (char *)tmp;

    case 1: // button
        if ((unsigned)num > (unsigned)joynumbuttons) return NULL;
        Bsprintf(tmp,"Button %d",num);
        return (char *)tmp;

    case 2: // hat
        if ((unsigned)num > (unsigned)joynumhats) return NULL;
        Bsprintf(tmp,"Hat %d",num);
        return (char *)tmp;

    default:
        return NULL;
    }
}
#else
static const char *joynames[3][15] =
{
	{
		"Left Stick X",
		"Left Stick Y",
		"Right Stick X",
		"Right Stick Y",
		"Axis 5",
		"Axis 6",
		"Axis 7",
		"Axis 8",
		"Axis 9",
		"Axis 10",
		"Axis 11",
		"Axis 12",
		"Axis 13",
		"Axis 14",
		"Axis 15",
	},
	{
		"Button A",
		"Button B",
		"Button 1",
		"Button 2",
		"Button -",
		"Button +",
		"Button HOME",
		"Button Z",
		"Button C",
		"Button X",
		"Button Y",
		"Trigger L",
		"Trigger R",
		"Trigger ZL",
		"Trigger ZR",
	},
	{
		"D-Pad Up",
		"D-Pad Right",
		"D-Pad Down",
		"D-Pad Left",
		"Hat 5",
		"Hat 6",
		"Hat 7",
		"Hat 8",
		"Hat 9",
		"Hat 10",
		"Hat 11",
		"Hat 12",
		"Hat 13",
		"Hat 14",
		"Hat 15",
	}
};
const char *getjoyname(int32_t what, int32_t num)
{
    switch (what)
    {
    case 0:	// axis
        if ((unsigned)num > (unsigned)joynumaxes) return NULL;
        return joynames[0][num];

    case 1: // button
        if ((unsigned)num > (unsigned)joynumbuttons) return NULL;
        return joynames[1][num];

    case 2: // hat
        if ((unsigned)num > (unsigned)joynumhats) return NULL;
        return joynames[2][num];

    default:
        return NULL;
    }
}
#endif


//
// initmouse() -- init mouse input
//
int32_t initmouse(void)
{
    moustat=1;
    grabmouse(1); // FIXME - SA
    return 0;
}

//
// uninitmouse() -- uninit mouse input
//
void uninitmouse(void)
{
    grabmouse(0);
    moustat=0;
}


//
// grabmouse() -- show/hide mouse cursor
//
void grabmouse(char a)
{
    if (appactive && moustat)
    {
        if (a != mousegrab)
        {
#if !defined __ANDROID__ && (!defined DEBUGGINGAIDS || defined _WIN32 || defined __APPLE__)
            SDL_GrabMode g;

            g = SDL_WM_GrabInput(a ? SDL_GRAB_ON : SDL_GRAB_OFF);
            mousegrab = (g == SDL_GRAB_ON);

            SDL_ShowCursor(mousegrab ? SDL_DISABLE : SDL_ENABLE);
#else
            mousegrab = a;
#endif
        }
    }
    else
    {
        mousegrab = a;
    }
    mousex = mousey = 0;
    mouseabsx = mouseabsy = 0;
}

//
// setjoydeadzone() -- sets the dead and saturation zones for the joystick
//
void setjoydeadzone(int32_t axis, uint16_t dead, uint16_t satur)
{
    joydead[axis] = dead;
    joysatur[axis] = satur;
}


//
// getjoydeadzone() -- gets the dead and saturation zones for the joystick
//
void getjoydeadzone(int32_t axis, uint16_t *dead, uint16_t *satur)
{
    *dead = joydead[axis];
    *satur = joysatur[axis];
}


//
// releaseallbuttons()
//
void releaseallbuttons(void)
{}


//
//
// ---------------------------------------
//
// All things Timer
// Ken did this
//
// ---------------------------------------
//
//

static Uint32 timerfreq=0;
static Uint32 timerlastsample=0;
int32_t timerticspersec=0;
static double msperhitick = 0;
static void(*usertimercallback)(void) = NULL;


//
// inittimer() -- initialize timer
//
int32_t inittimer(int32_t tickspersecond)
{
    if (timerfreq) return 0;	// already installed

//    initprintf("Initializing timer\n");

#ifdef _WIN32
    {
        int32_t t = win_inittimer();
        if (t < 0)
            return t;
    }
#endif

    timerfreq = 1000;
    timerticspersec = tickspersecond;
    timerlastsample = SDL_GetTicks() * timerticspersec / timerfreq;

    usertimercallback = NULL;

    msperhitick = 1000.0 / (double)gethitickspersec();

    return 0;
}

//
// uninittimer() -- shut down timer
//
void uninittimer(void)
{
    if (!timerfreq) return;

    timerfreq=0;

#ifdef _WIN32
    win_timerfreq=0;
#endif

    msperhitick = 0;
}

//
// sampletimer() -- update totalclock
//
void sampletimer(void)
{
    Uint32 i;
    int32_t n;

    if (!timerfreq) return;
    i = SDL_GetTicks();
    n = (int32_t)(i * timerticspersec / timerfreq) - timerlastsample;
    if (n>0)
    {
        totalclock += n;
        timerlastsample += n;
    }

    if (usertimercallback) for (; n>0; n--) usertimercallback();
}

//
// getticks() -- returns the sdl ticks count
//
uint32_t getticks(void)
{
    return (uint32_t)SDL_GetTicks();
}

// high-resolution timers for profiling
uint64_t gethiticks(void)
{
#if (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION < 3) // SDL 1.2
# if defined _WIN32
    return win_gethiticks();
# elif defined __APPLE__
    return mach_absolute_time();
# elif _POSIX_TIMERS>0 && defined _POSIX_MONOTONIC_CLOCK
    // This is SDL HG's SDL_GetPerformanceCounter() when clock_gettime() is
    // available.
    uint64_t ticks;
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);
    ticks = now.tv_sec;
    ticks *= 1000000000;
    ticks += now.tv_nsec;
    return ticks;
# elif defined GEKKO
    return ticks_to_nanosecs(gettime());
# else
// Blar. This pragma is unsupported on earlier GCC versions.
// At least we'll get a warning and a reference to this line...
#  pragma message "Using low-resolution (1ms) timer for gethiticks. Profiling will work badly."
    return SDL_GetTicks();
# endif
#else
    return SDL_GetPerformanceCounter();
#endif
}

uint64_t gethitickspersec(void)
{
#if (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION < 3) // SDL 1.2
# if defined _WIN32
    return win_timerfreq;
# elif defined __APPLE__
    static mach_timebase_info_data_t ti;
    if (ti.denom == 0)
        (void)mach_timebase_info(&ti);  // ti.numer/ti.denom: nsec/(m_a_t() tick)
    return (1000000000LL*ti.denom)/ti.numer;
# elif _POSIX_TIMERS>0 && defined _POSIX_MONOTONIC_CLOCK
    return 1000000000;
# elif defined GEKKO
    return TB_NSPERSEC;
# else
    return 1000;
# endif
#else  // SDL 1.3
    return SDL_GetPerformanceFrequency();
#endif
}

// Returns the time since an unspecified starting time in milliseconds.
// (May be not monotonic for certain configurations.)
ATTRIBUTE((flatten))
double gethitickms(void)
{
    return (double)gethiticks() * msperhitick;
}

//
// gettimerfreq() -- returns the number of ticks per second the timer is configured to generate
//
int32_t gettimerfreq(void)
{
    return timerticspersec;
}


//
// installusertimercallback() -- set up a callback function to be called when the timer is fired
//
void(*installusertimercallback(void(*callback)(void)))(void)
{
    void(*oldtimercallback)(void);

    oldtimercallback = usertimercallback;
    usertimercallback = callback;

    return oldtimercallback;
}



//
//
// ---------------------------------------
//
// All things Video
//
// ---------------------------------------
//
//


//
// getvalidmodes() -- figure out what video modes are available
//
static int32_t sortmodes(const struct validmode_t *a, const struct validmode_t *b)
{
    int32_t x;

    if ((x = a->fs   - b->fs)   != 0) return x;
    if ((x = a->bpp  - b->bpp)  != 0) return x;
    if ((x = a->xdim - b->xdim) != 0) return x;
    if ((x = a->ydim - b->ydim) != 0) return x;

    return 0;
}
static char modeschecked=0;
void getvalidmodes(void)
{
    int32_t i, j, maxx=0, maxy=0;
    static int32_t cdepths[] =
    {
        8,
#ifdef USE_OPENGL
        16,24,32,
#endif
        0
    };
#if SDL_MAJOR_VERSION==1
    SDL_Rect **modes;
    SDL_PixelFormat pf;

    pf.palette = NULL;
    pf.BitsPerPixel = 8;
    pf.BytesPerPixel = 1;
#endif

    if (modeschecked || novideo) return;

    validmodecnt=0;
//    initprintf("Detecting video modes:\n");

#define ADDMODE(x,y,c,f) do { \
    if (validmodecnt<MAXVALIDMODES) { \
        int32_t mn; \
        for(mn=0;mn<validmodecnt;mn++) \
            if (validmode[mn].xdim==x && validmode[mn].ydim==y && \
                validmode[mn].bpp==c  && validmode[mn].fs==f) break; \
        if (mn==validmodecnt) { \
            validmode[validmodecnt].xdim=x; \
            validmode[validmodecnt].ydim=y; \
            validmode[validmodecnt].bpp=c; \
            validmode[validmodecnt].fs=f; \
            validmodecnt++; \
            /*initprintf("  - %dx%d %d-bit %s\n", x, y, c, (f&1)?"fullscreen":"windowed");*/ \
        } \
    } \
} while (0)

#define CHECK(w,h) if ((w < maxx) && (h < maxy))

#if SDL_MAJOR_VERSION==1
    // do fullscreen modes first
    for (j=0; cdepths[j]; j++)
    {
# ifdef USE_OPENGL
        if (nogl && cdepths[j] > 8) continue;
# endif
        pf.BitsPerPixel = cdepths[j];
        pf.BytesPerPixel = cdepths[j] >> 3;

        modes = SDL_ListModes(&pf, SURFACE_FLAGS
// #if (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION < 3)
                              | SDL_FULLSCREEN // not implemented/working in SDL 1.3 SDL_compat.c
//#endif
                             );

        if (modes == (SDL_Rect **)0)
        {
            if (cdepths[j] > 8) cdepths[j] = -1;
            continue;
        }

        if (modes == (SDL_Rect **)-1)
        {
            for (i=0; defaultres[i][0]; i++)
                ADDMODE(defaultres[i][0],defaultres[i][1],cdepths[j],1);
        }
        else
        {
            for (i=0; modes[i]; i++)
            {
                if ((modes[i]->w > MAXXDIM) || (modes[i]->h > MAXYDIM)) continue;

                ADDMODE(modes[i]->w, modes[i]->h, cdepths[j], 1);

                if ((modes[i]->w > maxx) && (modes[i]->h > maxy))
                {
                    maxx = modes[i]->w;
                    maxy = modes[i]->h;
                }
            }
        }
    }
#endif  // SDL_MAJOR_VERSION==1
    if (maxx == 0 && maxy == 0)
    {
        initprintf("No fullscreen modes available!\n");
        maxx = MAXXDIM; maxy = MAXYDIM;
    }

    // add windowed modes next
    for (j=0; cdepths[j]; j++)
    {
#ifdef USE_OPENGL
        if (nogl && cdepths[j] > 8) continue;
#endif
        if (cdepths[j] < 0) continue;
        for (i=0; defaultres[i][0]; i++)
            CHECK(defaultres[i][0],defaultres[i][1])
                ADDMODE(defaultres[i][0],defaultres[i][1],cdepths[j],0);
        }

#undef CHECK
#undef ADDMODE

    qsort((void *)validmode, validmodecnt, sizeof(struct validmode_t), (int32_t( *)(const void *,const void *))sortmodes);

    modeschecked=1;
}


//
// checkvideomode() -- makes sure the video mode passed is legal
//
int32_t checkvideomode(int32_t *x, int32_t *y, int32_t c, int32_t fs, int32_t forced)
{
    int32_t i, nearest=-1, dx, dy, odx=9999, ody=9999;

    getvalidmodes();

    if (c>8
#ifdef USE_OPENGL
            && nogl
#endif
       ) return -1;

    // fix up the passed resolution values to be multiples of 8
    // and at least 320x200 or at most MAXXDIMxMAXYDIM
    if (*x < 320) *x = 320;
    if (*y < 200) *y = 200;
    if (*x > MAXXDIM) *x = MAXXDIM;
    if (*y > MAXYDIM) *y = MAXYDIM;
//    *x &= 0xfffffff8l;

    for (i=0; i<validmodecnt; i++)
    {
        if (validmode[i].bpp != c) continue;
        if (validmode[i].fs != fs) continue;
        dx = klabs(validmode[i].xdim - *x);
        dy = klabs(validmode[i].ydim - *y);
        if (!(dx | dy))
        {
            // perfect match
            nearest = i;
            break;
        }
        if ((dx <= odx) && (dy <= ody))
        {
            nearest = i;
            odx = dx; ody = dy;
        }
    }

#ifdef ANY_WINDOWED_SIZE
    if (!forced && (fs&1) == 0 && (nearest < 0 || (validmode[nearest].xdim!=*x || validmode[nearest].ydim!=*y)))
        return 0x7fffffffl;
#endif

    if (nearest < 0)
    {
        // no mode that will match (eg. if no fullscreen modes)
        return -1;
    }

    *x = validmode[nearest].xdim;
    *y = validmode[nearest].ydim;

    return nearest;		// JBF 20031206: Returns the mode number
}

static int32_t needpalupdate;
static SDL_Color sdlayer_pal[256];

#if SDL_MAJOR_VERSION==2
static void destroy_window_and_renderer()
{
//        if (sdl_surface)  // will be freed with SDL_DestroyWindow
//            SDL_FreeSurface(sdl_surface);
        sdl_surface = NULL;
        sdl_surface2 = NULL;
        if (sdl_renderer)
            SDL_DestroyRenderer(sdl_renderer);
        sdl_renderer = NULL;
        if (sdl_window)
            SDL_DestroyWindow(sdl_window);
        sdl_window = NULL;
}

static int32_t create_window_and_renderer(int32_t x, int32_t y, int32_t fs, uint32_t flags)
{
    sdl_window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,
                                  x,y, ((fs&1)?SDL_WINDOW_FULLSCREEN:0));
    if (!sdl_window)
    {
        initprintf("Unable to set video mode: SDL_CreateWindow failed: %s\n",
                   SDL_GetError());
        return -1;
    }

    sdl_renderer = SDL_CreateRenderer(sdl_window, -1, flags);
    if (!sdl_renderer)
    {
        initprintf("Unable to set video mode: SDL_CreateRenderer failed: %s\n",
                   SDL_GetError());
        SDL_DestroyWindow(sdl_window); sdl_window=NULL;
        return -1;
    }

    return 0;
}
#endif

//
// setvideomode() -- set SDL video mode
//
int32_t setvideomode(int32_t x, int32_t y, int32_t c, int32_t fs)
{
    int32_t regrab = 0;
#ifdef USE_OPENGL
    static int32_t warnonce = 0;
# if (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION < 3)
    static int32_t ovsync = 1;
# endif
#endif
    if ((fs == fullscreen) && (x == xres) && (y == yres) && (c == bpp) &&
            !videomodereset)
    {
        OSD_ResizeDisplay(xres,yres);
        return 0;
    }

    if (checkvideomode(&x,&y,c,fs,0) < 0) return -1;

    startwin_close();

    if (mousegrab)
    {
        regrab = 1;
        grabmouse(0);
    }

    if (lockcount) while (lockcount) enddrawing();

#ifdef USE_OPENGL
    if (bpp > 8 && sdl_surface) polymost_glreset();
#endif

    // clear last gamma/contrast/brightness so that it will be set anew
    lastvidgcb[0] = lastvidgcb[1] = lastvidgcb[2] = 0.0f;

#if SDL_MAJOR_VERSION==1
    // restore gamma before we change video modes if it was changed
    if (sdl_surface && gammabrightness)
    {
        SDL_SetGammaRamp(sysgamma[0], sysgamma[1], sysgamma[2]);
        gammabrightness = 0;	// redetect on next mode switch
    }
#endif

#ifdef USE_OPENGL
    if (c > 8)
    {
        int32_t i, j, multisamplecheck = (glmultisample > 0);
        struct
        {
            SDL_GLattr attr;
            int32_t value;
        }
        attributes[] =
        {
# if 0
            { SDL_GL_RED_SIZE, 8 },
            { SDL_GL_GREEN_SIZE, 8 },
            { SDL_GL_BLUE_SIZE, 8 },
            { SDL_GL_ALPHA_SIZE, 8 },
            { SDL_GL_BUFFER_SIZE, c },
            { SDL_GL_STENCIL_SIZE, 0 },
            { SDL_GL_ACCUM_RED_SIZE, 0 },
            { SDL_GL_ACCUM_GREEN_SIZE, 0 },
            { SDL_GL_ACCUM_BLUE_SIZE, 0 },
            { SDL_GL_ACCUM_ALPHA_SIZE, 0 },
            { SDL_GL_DEPTH_SIZE, 24 },
# endif
            { SDL_GL_DOUBLEBUFFER, 1 },
            { SDL_GL_MULTISAMPLEBUFFERS, glmultisample > 0 },
            { SDL_GL_MULTISAMPLESAMPLES, glmultisample },
            { SDL_GL_STENCIL_SIZE, 1 },
# if (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION < 3)
            { SDL_GL_SWAP_CONTROL, vsync },
# endif
        };

        if (nogl) return -1;

# ifdef _WIN32
        win_setvideomode(c);
# endif

        initprintf("Setting video mode %dx%d (%d-bpp %s)\n",
                   x,y,c, ((fs&1) ? "fullscreen" : "windowed"));
        do
        {
            for (i=0; i < (int32_t)(sizeof(attributes)/sizeof(attributes[0])); i++)
            {
                j = attributes[i].value;
                if (!multisamplecheck &&
                        (attributes[i].attr == SDL_GL_MULTISAMPLEBUFFERS ||
                         attributes[i].attr == SDL_GL_MULTISAMPLESAMPLES)
                   )
                {
                    j = 0;
                }
                SDL_GL_SetAttribute(attributes[i].attr, j);
            }

            /* HACK: changing SDL GL attribs only works before surface creation,
               so we have to create a new surface in a different format first
               to force the surface we WANT to be recreated instead of reused. */
# if (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION < 3)
            if (vsync != ovsync)
            {
                if (sdl_surface)
                {
                    SDL_FreeSurface(sdl_surface);
                    sdl_surface = SDL_SetVideoMode(1, 1, 8, SDL_NOFRAME | SURFACE_FLAGS | ((fs&1)?SDL_FULLSCREEN:0));
                    SDL_FreeSurface(sdl_surface);
                }
                ovsync = vsync;
            }
# endif

# if SDL_MAJOR_VERSION==1
            sdl_surface = SDL_SetVideoMode(x, y, c, SDL_OPENGL | ((fs&1)?SDL_FULLSCREEN:0));
            if (!sdl_surface)
            {
                if (multisamplecheck)
                {
                    initprintf("Multisample mode not possible. Retrying without multisampling.\n");
                    glmultisample = 0;
                    continue;
                }
                initprintf("Unable to set video mode!\n");
                return -1;
            }
# else
            destroy_window_and_renderer();

            if (create_window_and_renderer(x,y,fs, SDL_RENDERER_ACCELERATED) == -1)
                return -1;
#endif
        }
        while (multisamplecheck--);
    }
    else
#endif  // defined USE_OPENGL
    {
        initprintf("Setting video mode %dx%d (%d-bpp %s)\n",
                   x,y,c, ((fs&1) ? "fullscreen" : "windowed"));
#if SDL_MAJOR_VERSION==1
        sdl_surface = SDL_SetVideoMode(x, y, c, SURFACE_FLAGS | ((fs&1)?SDL_FULLSCREEN:0));
        if (!sdl_surface)
        {
            initprintf("Unable to set video mode!\n");
            return -1;
        }
#else
        // deinit
        destroy_window_and_renderer();

        // init
        if (create_window_and_renderer(x,y,fs, SDL_RENDERER_SOFTWARE |
                                       SDL_RENDERER_TARGETTEXTURE) == -1)
            return -1;

        sdl_surface2 = SDL_GetWindowSurface(sdl_window);

        if (!sdl_surface2)
        {
            initprintf("Unable to set video mode: SDL_GetWindowSurface failed: %s\n",
                       SDL_GetError());
            SDL_DestroyRenderer(sdl_renderer); sdl_renderer=NULL;
            SDL_DestroyWindow(sdl_window); sdl_window=NULL;
            return -1;
        }
# if 1
        sdl_surface = sdl_surface2;
# else
        sdl_surface = SDL_ConvertSurfaceFormat(sdl_surface2, SDL_PIXELFORMAT_INDEX8, 0);

        if (!sdl_surface)
        {
            initprintf("Unable to set video mode: SDL_ConvertSurfaceFormat failed: %s\n",
                       SDL_GetError());
            SDL_DestroyRenderer(sdl_renderer); sdl_renderer=NULL;
            SDL_DestroyWindow(sdl_window); sdl_window=NULL;
            return -1;
        }
# endif
        if (!sdl_palptr)
            sdl_palptr = SDL_AllocPalette(256);

        if (SDL_SetSurfacePalette(sdl_surface, sdl_palptr) < 0)
            initprintf("SDL_SetSurfacePalette failed: %s\n", SDL_GetError());
#endif
    }

#if 0
    {
        char flags[512] = "";
# define FLAG(x,y) if ((sdl_surface->flags & x) == x) { strcat(flags, y); strcat(flags, " "); }
        FLAG(SDL_HWSURFACE, "HWSURFACE") else
            FLAG(SDL_SWSURFACE, "SWSURFACE")
            FLAG(SDL_ASYNCBLIT, "ASYNCBLIT")
            FLAG(SDL_ANYFORMAT, "ANYFORMAT")
            FLAG(SDL_HWPALETTE, "HWPALETTE")
            FLAG(SDL_DOUBLEBUF, "DOUBLEBUF")
            FLAG(SDL_FULLSCREEN, "FULLSCREEN")
            FLAG(SDL_OPENGL, "OPENGL")
            FLAG(SDL_OPENGLBLIT, "OPENGLBLIT")
            FLAG(SDL_RESIZABLE, "RESIZABLE")
            FLAG(SDL_HWACCEL, "HWACCEL")
            FLAG(SDL_SRCCOLORKEY, "SRCCOLORKEY")
            FLAG(SDL_RLEACCEL, "RLEACCEL")
            FLAG(SDL_SRCALPHA, "SRCALPHA")
            FLAG(SDL_PREALLOC, "PREALLOC")
# undef FLAG
            initprintf("SDL Surface flags: %s\n", flags);
    }
#endif

    {
        //static char t[384];
        //sprintf(t, "%s (%dx%d %s)", apptitle, x, y, ((fs) ? "fullscreen" : "windowed"));
#if SDL_MAJOR_VERSION == 1
        SDL_WM_SetCaption(apptitle, 0);
#else
        SDL_SetWindowTitle(sdl_window, apptitle);
#endif
    }

#ifdef USE_OPENGL
    if (c > 8)
    {
        char *p,*p2,*p3;

        polymost_glreset();

        bglEnable(GL_TEXTURE_2D);
        bglShadeModel(GL_SMOOTH); //GL_FLAT
        bglClearColor(0,0,0,0.5); //Black Background
        bglHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST); //Use FASTEST for ortho!
        bglHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
        bglDisable(GL_DITHER);

        glinfo.vendor     = (const char *)bglGetString(GL_VENDOR);
        glinfo.renderer   = (const char *)bglGetString(GL_RENDERER);
        glinfo.version    = (const char *)bglGetString(GL_VERSION);
        glinfo.extensions = (const char *)bglGetString(GL_EXTENSIONS);

# ifdef POLYMER
        if (!Bstrcmp(glinfo.vendor,"ATI Technologies Inc."))
        {
            pr_ati_fboworkaround = 1;
            initprintf("Enabling ATI FBO color attachment workaround.\n");

            if (Bstrstr(glinfo.renderer,"Radeon X1"))
            {
                pr_ati_nodepthoffset = 1;
                initprintf("Enabling ATI R520 polygon offset workaround.\n");
            }
            else
                pr_ati_nodepthoffset = 0;
#  ifdef __APPLE__
            //See bug description at http://lists.apple.com/archives/mac-opengl/2005/Oct/msg00169.html
            if (!Bstrncmp(glinfo.renderer,"ATI Radeon 9600", 15))
            {
                pr_ati_textureformat_one = 1;
                initprintf("Enabling ATI Radeon 9600 texture format workaround.\n");
            }
            else
                pr_ati_textureformat_one = 0;
#  endif
        }
        else
            pr_ati_fboworkaround = 0;
# endif  // defined POLYMER

        glinfo.maxanisotropy = 1.0;
        glinfo.bgra = 0;
        glinfo.texcompr = 0;

        // process the extensions string and flag stuff we recognize
        p = Bstrdup(glinfo.extensions);
        p3 = p;
        while ((p2 = Bstrtoken(p3==p?p:NULL, " ", (char **)&p3, 1)) != NULL)
        {
            if (!Bstrcmp(p2, "GL_EXT_texture_filter_anisotropic"))
            {
                // supports anisotropy. get the maximum anisotropy level
                bglGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glinfo.maxanisotropy);
            }
            else if (!Bstrcmp(p2, "GL_EXT_texture_edge_clamp") ||
                     !Bstrcmp(p2, "GL_SGIS_texture_edge_clamp"))
            {
                // supports GL_CLAMP_TO_EDGE or GL_CLAMP_TO_EDGE_SGIS
                glinfo.clamptoedge = 1;
            }
            else if (!Bstrcmp(p2, "GL_EXT_bgra"))
            {
                // support bgra textures
                glinfo.bgra = 1;
            }
            else if (!Bstrcmp(p2, "GL_ARB_texture_compression") && Bstrcmp(glinfo.vendor,"ATI Technologies Inc."))
            {
                // support texture compression
                glinfo.texcompr = 1;
            }
            else if (!Bstrcmp(p2, "GL_ARB_texture_non_power_of_two"))
            {
                // support non-power-of-two texture sizes
                glinfo.texnpot = 1;
            }
            else if (!Bstrcmp(p2, "WGL_3DFX_gamma_control"))
            {
                // 3dfx cards have issues with fog
                nofog = 1;
                if (!(warnonce&1)) initprintf("3dfx card detected: OpenGL fog disabled\n");
                warnonce |= 1;
            }
            else if (!Bstrcmp(p2, "GL_ARB_multisample"))
            {
                // supports multisampling
                glinfo.multisample = 1;
            }
            else if (!Bstrcmp(p2, "GL_NV_multisample_filter_hint"))
            {
                // supports nvidia's multisample hint extension
                glinfo.nvmultisamplehint = 1;
            }
            else if (!Bstrcmp((char *)p2, "GL_ARB_fragment_program"))
            {
                glinfo.arbfp = 1;
            }
            else if (!Bstrcmp((char *)p2, "GL_ARB_depth_texture"))
            {
                glinfo.depthtex = 1;
            }
            else if (!Bstrcmp((char *)p2, "GL_ARB_shadow"))
            {
                glinfo.shadow = 1;
            }
            else if (!Bstrcmp((char *)p2, "GL_EXT_framebuffer_object"))
            {
                glinfo.fbos = 1;
            }
            else if (!Bstrcmp((char *)p2, "GL_NV_texture_rectangle") ||
                     !Bstrcmp((char *)p2, "GL_EXT_texture_rectangle"))
            {
                glinfo.rect = 1;
            }
            else if (!Bstrcmp((char *)p2, "GL_ARB_multitexture"))
            {
                glinfo.multitex = 1;
            }
            else if (!Bstrcmp((char *)p2, "GL_ARB_texture_env_combine"))
            {
                glinfo.envcombine = 1;
            }
            else if (!Bstrcmp((char *)p2, "GL_ARB_vertex_buffer_object"))
            {
                glinfo.vbos = 1;
            }
            else if (!Bstrcmp((char *)p2, "GL_EXT_gpu_shader4"))
            {
                glinfo.sm4 = 1;
            }
            else if (!Bstrcmp((char *)p2, "GL_ARB_occlusion_query"))
            {
                glinfo.occlusionqueries = 1;
            }
            else if (!Bstrcmp((char *)p2, "GL_ARB_shader_objects"))
            {
                glinfo.glsl = 1;
            }
            else if (!Bstrcmp((char *)p2, "GL_ARB_debug_output"))
            {
                glinfo.debugoutput = 1;
            }
        }
        Bfree(p);

        if (!glinfo.dumped)
        {
            int32_t oldbpp = bpp;
            bpp = 32;
            osdcmd_glinfo(NULL);
            glinfo.dumped = 1;
            bpp = oldbpp;
        }
    }
#endif  // defined USE_OPENGL

    xres = x;
    yres = y;
    bpp = c;
    fullscreen = fs;
    //bytesperline = sdl_surface->pitch;
    numpages = c>8?2:1;
    frameplace = 0;
    lockcount = 0;
    modechange=1;
    videomodereset = 0;
    OSD_ResizeDisplay(xres,yres);

    // save the current system gamma to determine if gamma is available
    if (!gammabrightness)
    {
//        float f = 1.0 + ((float)curbrightness / 10.0);
#if SDL_MAJOR_VERSION==1
        if (SDL_GetGammaRamp(sysgamma[0], sysgamma[1], sysgamma[2]) >= 0)
            gammabrightness = 1;
#else
        if (SDL_GetWindowGammaRamp(sdl_window, sysgamma[0], sysgamma[1], sysgamma[2]) == 0)
            gammabrightness = 1;
#endif
        // see if gamma really is working by trying to set the brightness
        if (gammabrightness && setgamma() < 0)
            gammabrightness = 0;	// nope
    }

    // setpalettefade will set the palette according to whether gamma worked
    setpalettefade(palfadergb.r, palfadergb.g, palfadergb.b, palfadedelta);

    //if (c==8) setpalette(0,256,0);
    //baselayer_onvideomodechange(c>8);

    if (regrab) grabmouse(1);

    return 0;
}


//
// resetvideomode() -- resets the video system
//
void resetvideomode(void)
{
    videomodereset = 1;
    modeschecked = 0;
}


//
// begindrawing() -- locks the framebuffer for drawing
//
void begindrawing(void)
{
    if (bpp > 8)
    {
        if (offscreenrendering) return;
        frameplace = 0;
        bytesperline = 0;
        modechange = 0;
        return;
    }

    // lock the frame
    if (lockcount++ > 0)
        return;

    if (offscreenrendering) return;

    if (SDL_MUSTLOCK(sdl_surface)) SDL_LockSurface(sdl_surface);
    frameplace = (intptr_t)sdl_surface->pixels;

    if (sdl_surface->pitch != bytesperline || modechange)
    {
        bytesperline = sdl_surface->pitch;

        calc_ylookup(bytesperline, ydim);

        modechange=0;
    }
}


//
// enddrawing() -- unlocks the framebuffer
//
void enddrawing(void)
{
    if (bpp > 8)
    {
        if (!offscreenrendering) frameplace = 0;
        return;
    }

    if (!frameplace) return;
    if (lockcount > 1) { lockcount--; return; }
    if (!offscreenrendering) frameplace = 0;
    if (lockcount == 0) return;
    lockcount = 0;

    if (offscreenrendering) return;

    if (SDL_MUSTLOCK(sdl_surface)) SDL_UnlockSurface(sdl_surface);
}

//
// showframe() -- update the display
//
void showframe(int32_t w)
{
//    int32_t i,j;
    UNREFERENCED_PARAMETER(w);

#ifdef USE_OPENGL
    if (bpp > 8)
    {
        if (palfadedelta)
            fullscreen_tint_gl(palfadergb.r, palfadergb.g, palfadergb.b, palfadedelta);

# if SDL_MAJOR_VERSION==1
        SDL_GL_SwapBuffers();
# else
        SDL_RenderPresent(sdl_renderer);
# endif
        return;
    }
#endif

    if (offscreenrendering) return;

    if (lockcount)
    {
        printf("Frame still locked %d times when showframe() called.\n", lockcount);
        while (lockcount) enddrawing();
    }

    // deferred palette updating
    if (needpalupdate)
    {
#if SDL_MAJOR_VERSION==1
        SDL_SetColors(sdl_surface, sdlayer_pal, 0, 256);
        // same as:
        //SDL_SetPalette(sdl_surface, SDL_LOGPAL|SDL_PHYSPAL, pal, 0, 256);
#else
        if (SDL_SetPaletteColors(sdl_palptr, sdlayer_pal, 0, 256) < 0)
            initprintf("SDL_SetPaletteColors failed: %s\n", SDL_GetError());
#endif
        needpalupdate = 0;
    }

#if SDL_MAJOR_VERSION==1
    SDL_Flip(sdl_surface);
#else
//    SDL_UpdateWindowSurface(sdl_window);
    SDL_RenderPresent(sdl_renderer);
#endif
}


//
// setpalette() -- set palette values
//
int32_t setpalette(int32_t start, int32_t num)
{
    int32_t i,n;

    if (bpp > 8) return 0;	// no palette in opengl

    Bmemcpy(sdlayer_pal, curpalettefaded, 256*4);

    for (i=start, n=num; n>0; i++, n--)
        curpalettefaded[i].f = sdlayer_pal[i].unused = 0;

    needpalupdate = 1;

    return 0;
}

//
// getpalette() -- get palette values
//
#if 0
int32_t getpalette(int32_t start, int32_t num, char *dapal)
{
    int32_t i;
    SDL_Palette *pal;

    // we shouldn't need to lock the surface to get the palette
    pal = sdl_surface->format->palette;

    for (i=num; i>0; i--, start++) {
        dapal[0] = pal->colors[start].b >> 2;
        dapal[1] = pal->colors[start].g >> 2;
        dapal[2] = pal->colors[start].r >> 2;
        dapal += 4;
    }

    return 1;
}
#endif

//
// setgamma
//
int32_t setgamma(void)
{
    int32_t i;
    uint16_t gammaTable[768];
    float gamma = max(0.1f,min(4.f,vid_gamma));
    float contrast = max(0.1f,min(3.f,vid_contrast));
    float bright = max(-0.8f,min(0.8f,vid_brightness));

    double invgamma = 1 / gamma;
    double norm = pow(255., invgamma - 1);

    if (novideo) return 0;

    if (lastvidgcb[0]==gamma && lastvidgcb[1]==contrast && lastvidgcb[2]==bright)
        return 0;

    // This formula is taken from Doomsday

    for (i = 0; i < 256; i++)
    {
        double val = i * contrast - (contrast - 1) * 127;
        if (gamma != 1) val = pow(val, invgamma) / norm;
        val += bright * 128;

        gammaTable[i] = gammaTable[i + 256] = gammaTable[i + 512] = (uint16_t)max(0.f,(double)min(0xffff,val*256));
    }
#if SDL_MAJOR_VERSION==1
    i = SDL_SetGammaRamp(&gammaTable[0],&gammaTable[256],&gammaTable[512]);
#else
    i = INT32_MIN;
    if (sdl_window)
        i = SDL_SetWindowGammaRamp(
            sdl_window,&gammaTable[0],&gammaTable[256],&gammaTable[512]);
#endif

#if SDL_MAJOR_VERSION==1
    if (i != -1)
#else
    if (i < 0)
    {
        if (i != INT32_MIN)
            initprintf("Unable to set gamma: SDL_SetWindowGammaRamp failed: %s\n", SDL_GetError());
    }
    else
#endif
    {
        lastvidgcb[0] = gamma;
        lastvidgcb[1] = contrast;
        lastvidgcb[2] = bright;
    }

    return i;
}

#if !defined(__APPLE__) && !defined(__ANDROID__)
extern struct sdlappicon sdlappicon;
static SDL_Surface *loadappicon(void)
{
    SDL_Surface *surf;

    surf = SDL_CreateRGBSurfaceFrom((void *)sdlappicon.pixels,
                                    sdlappicon.width, sdlappicon.height, 32, sdlappicon.width*4,
                                    0xffl,0xff00l,0xff0000l,0xff000000l);

    return surf;
}
#endif

//
//
// ---------------------------------------
//
// Miscellany
//
// ---------------------------------------
//
//

int32_t handleevents_peekkeys(void)
{
#if (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION == 2)
    SDL_PumpEvents();
    return SDL_PeepEvents(NULL, 1, SDL_PEEKEVENT, SDL_EVENTMASK(SDL_KEYDOWN));
#else
    // SDL 1.3 up has not been tested to compile.
    return 0;
#endif
}


//
// handleevents() -- process the SDL message queue
//   returns !0 if there was an important event worth checking (like quitting)
//

int32_t handleevents(void)
{
    int32_t code, rv=0, j;
    SDL_Event ev;

    while (SDL_PollEvent(&ev))
    {
        switch (ev.type)
        {
#if (SDL_MAJOR_VERSION > 1 || SDL_MINOR_VERSION > 2)
        case SDL_TEXTINPUT:
            j = 0;
            do
            {
                code = ev.text.text[j];

                if (code != scantoasc[OSD_OSDKey()] && ((keyasciififoend+1)&(KEYFIFOSIZ-1)) != keyasciififoplc)
                {
                    if (OSD_HandleChar(code))
                    {
                        keyasciififo[keyasciififoend] = code;
                        keyasciififoend = ((keyasciififoend+1)&(KEYFIFOSIZ-1));
                    }
                }
            }
            while (j < SDL_TEXTINPUTEVENT_TEXT_SIZE && ev.text.text[++j]);
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
            code = keytranslation[ev.key.keysym.scancode];

            if (ev.key.type == SDL_KEYDOWN &&
                (ev.key.keysym.scancode == SDL_SCANCODE_RETURN ||
                ev.key.keysym.scancode == SDL_SCANCODE_BACKSPACE ||
                ev.key.keysym.scancode == SDL_SCANCODE_TAB) &&
                ((keyasciififoend+1)&(KEYFIFOSIZ-1)) != keyasciififoplc)
            {
                if (OSD_HandleChar(ev.key.keysym.unicode & 0x7f))
                {
                    keyasciififo[keyasciififoend] = ev.key.keysym.unicode & 0x7f;
                    keyasciififoend = ((keyasciififoend+1)&(KEYFIFOSIZ-1));
                }
            }

//            printf("got key %d, %d\n",ev.key.keysym.scancode,code);

            // hook in the osd
            if (OSD_HandleScanCode(code, (ev.key.type == SDL_KEYDOWN)) == 0)
                break;

            if (ev.key.type == SDL_KEYDOWN)
            {
                if (!keystatus[code])
                {
                    SetKey(code, 1);
                    if (keypresscallback)
                        keypresscallback(code, 1);
                }
            }
            else
            {
# ifdef __linux
                // The pause key generates a release event right after
                // the pressing one on linux. As a result, it gets unseen
                // by the game most of the time.
                if (code == 0x59)  // pause
                    break;
# endif
                SetKey(code, 0);
                if (keypresscallback)
                    keypresscallback(code, 0);
            }
            break;
        case SDL_WINDOWEVENT:
            switch (ev.window.event)
            {
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                appactive = 1;
# if !defined DEBUGGINGAIDS || defined _WIN32
                if (mousegrab && moustat)
                {
                    SDL_WM_GrabInput(SDL_GRAB_ON);
                    SDL_ShowCursor(SDL_DISABLE);
                }
# endif
# ifdef _WIN32
                if (backgroundidle)
                    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
# endif
                break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
                appactive = 0;
# if !defined DEBUGGINGAIDS || defined _WIN32
                if (mousegrab && moustat)
                {
                    SDL_WM_GrabInput(SDL_GRAB_OFF);
                    SDL_ShowCursor(SDL_ENABLE);
                }
# endif
# ifdef _WIN32
                if (backgroundidle)
                    SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
# endif
                break;
            }
            break;
            /*
                    case SDL_MOUSEWHEEL:
                        initprintf("wheel y %d\n",ev.wheel.y);
                        if (ev.wheel.y > 0)
                        {
                            mwheelup = totalclock;
                            mouseb |= 16;
                            if (mousepresscallback)
                                mousepresscallback(5, 1);
                        }
                        if (ev.wheel.y < 0)
                        {
                            mwheeldown = totalclock;
                            mouseb |= 32;
                            if (mousepresscallback)
                                mousepresscallback(6, 1);
                        }
                        break;
                        */
// #warning Using SDL 1.3 or 2.X
#else  // SDL 1.3+ ^^^ | vvv SDL 1.2
// #warning Using SDL 1.2
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            code = keytranslation[ev.key.keysym.sym];
# ifdef KEY_PRINT_DEBUG
            printf("keytranslation[%d] = %s (%d)  %s\n", ev.key.keysym.sym,
                   key_names[code], code, ev.key.type==SDL_KEYDOWN?"DOWN":"UP");
# endif
            if (code != OSD_OSDKey() && ev.key.keysym.unicode != 0 && ev.key.type == SDL_KEYDOWN &&
                    (ev.key.keysym.unicode & 0xff80) == 0 &&
                    ((keyasciififoend+1)&(KEYFIFOSIZ-1)) != keyasciififoplc)
            {
                if (OSD_HandleChar(ev.key.keysym.unicode & 0x7f))
                {
                    keyasciififo[keyasciififoend] = ev.key.keysym.unicode & 0x7f;
                    keyasciififoend = ((keyasciififoend+1)&(KEYFIFOSIZ-1));
                }
            }

            // hook in the osd
//            if (ev.type==SDL_KEYDOWN)
//                printf("got key SDLK %d (%s), trans 0x%x\n", ev.key.keysym.sym, SDL_GetKeyName(ev.key.keysym.sym), code);
            if (OSD_HandleScanCode(code, (ev.key.type == SDL_KEYDOWN)) == 0)
                break;

            if (ev.key.type == SDL_KEYDOWN)
            {
                if (!keystatus[code])
                {
                    SetKey(code, 1);
                    if (keypresscallback)
                        keypresscallback(code, 1);
                }
            }
            else
            {
# ifdef __linux
                if (code == 0x59)  // pause
                    break;
# endif
                SetKey(code, 0);
                if (keypresscallback)
                    keypresscallback(code, 0);
            }
            break;

        case SDL_ACTIVEEVENT:
            if (ev.active.state & SDL_APPINPUTFOCUS)
            {
                appactive = ev.active.gain;
# if !defined DEBUGGINGAIDS || defined _WIN32
                if (mousegrab && moustat)
                {
                    if (appactive)
                    {
                        SDL_WM_GrabInput(SDL_GRAB_ON);
                        SDL_ShowCursor(SDL_DISABLE);
                    }
                    else
                    {
                        SDL_WM_GrabInput(SDL_GRAB_OFF);
                        SDL_ShowCursor(SDL_ENABLE);
                    }
                }
# endif
# ifdef _WIN32
                if (backgroundidle)
                    SetPriorityClass(GetCurrentProcess(),
                                     appactive ? NORMAL_PRIORITY_CLASS : IDLE_PRIORITY_CLASS);
# endif
                rv=-1;
            }
            break;
#endif  // SDL version

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
//if (ev.button.button>=0)
//    initprintf("BTN %d\n", ev.button.button);
            switch (ev.button.button)
            {
                // some of these get reordered to match winlayer
            default:
                j = -1; break;
            case SDL_BUTTON_LEFT:
                j = 0; break;
            case SDL_BUTTON_RIGHT:
                j = 1; break;
            case SDL_BUTTON_MIDDLE:
                j = 2; break;
            case 8 /*SDL_BUTTON_X1*/:  // 8 --> 3
                j = 3; break;
#if SDL_MAJOR_VERSION==1
            case SDL_BUTTON_WHEELUP:  // 4
            case SDL_BUTTON_WHEELDOWN:  // 5
                j = ev.button.button; break;
#endif
            case 9 /*SDL_BUTTON_X2*/:  // 9 --> 6
                j = 6; break;
            }
            if (j<0) break;

            if (ev.button.state == SDL_PRESSED)
            {
#if SDL_MAJOR_VERSION==1
                if (ev.button.button == SDL_BUTTON_WHEELUP)
                {
                    mwheelup = totalclock;
                }
                if (ev.button.button == SDL_BUTTON_WHEELDOWN)
                {
                    mwheeldown = totalclock;
                }
#endif
                mouseb |= (1<<j);
            }
            else
            {
#if SDL_MAJOR_VERSION==1
                if (j != SDL_BUTTON_WHEELUP && j != SDL_BUTTON_WHEELDOWN)
#endif
                    mouseb &= ~(1<<j);
            }

            if (mousepresscallback)
                mousepresscallback(j+1, ev.button.state == SDL_PRESSED);
            break;

        case SDL_MOUSEMOTION:
            // SDL <VER> doesn't handle relative mouse movement correctly yet as the cursor still clips to the screen edges
            // so, we call SDL_WarpMouse() to center the cursor and ignore the resulting motion event that occurs
            //  <VER> is 1.3 for PK, 1.2 for tueidj
            if (appactive && mousegrab)
            {
#ifdef GEKKO
                // check if it's a wiimote pointer pretending to be a mouse
                if (ev.motion.state & SDL_BUTTON_X2MASK)
                {
                    // the absolute values are used to draw the crosshair
                    mouseabsx = ev.motion.x;
                    mouseabsy = ev.motion.y;
                    // hack: reduce the scale of the "relative" motions
                    // to make it act more like a real mouse
                    ev.motion.xrel /= 16;
                    ev.motion.yrel /= 12;
                }
#endif
                if (ev.motion.x != xdim>>1 || ev.motion.y != ydim>>1)
                {
                    mousex += ev.motion.xrel;
                    mousey += ev.motion.yrel;
#if !defined DEBUGGINGAIDS || MY_DEVELOPER_ID==805120924
# if SDL_MAJOR_VERSION==1
                    SDL_WarpMouse(xdim>>1, ydim>>1);
# else
                    SDL_WarpMouseInWindow(sdl_window, xdim>>1, ydim>>1);
# endif
#endif
                }
            }
            break;

        case SDL_JOYAXISMOTION:
            if (appactive && ev.jaxis.axis < joynumaxes)
            {
                joyaxis[ev.jaxis.axis] = ev.jaxis.value * 10000 / 32767;
                if ((joyaxis[ev.jaxis.axis] < joydead[ev.jaxis.axis])
                        && (joyaxis[ev.jaxis.axis] > -joydead[ev.jaxis.axis]))
                    joyaxis[ev.jaxis.axis] = 0;
                else if (joyaxis[ev.jaxis.axis] >= joysatur[ev.jaxis.axis])
                    joyaxis[ev.jaxis.axis] = 10000;
                else if (joyaxis[ev.jaxis.axis] <= -joysatur[ev.jaxis.axis])
                    joyaxis[ev.jaxis.axis] = -10000;
                else
                    joyaxis[ev.jaxis.axis] = joyaxis[ev.jaxis.axis] *
                                             10000 / joysatur[ev.jaxis.axis];
            }
            break;

        case SDL_JOYHATMOTION:
        {
            int32_t hatvals[16] =
            {
                -1,	// centre
                0, 	// up 1
                9000,	// right 2
                4500,	// up+right 3
                18000,	// down 4
                -1,	// down+up!! 5
                13500,	// down+right 6
                -1,	// down+right+up!! 7
                27000,	// left 8
                27500,	// left+up 9
                -1,	// left+right!! 10
                -1,	// left+right+up!! 11
                22500,	// left+down 12
                -1,	// left+down+up!! 13
                -1,	// left+down+right!! 14
                -1,	// left+down+right+up!! 15
            };
            if (appactive && ev.jhat.hat < joynumhats)
                joyhat[ ev.jhat.hat ] = hatvals[ ev.jhat.value & 15 ];
            break;
        }

        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
            if (appactive && ev.jbutton.button < joynumbuttons)
            {
                if (ev.jbutton.state == SDL_PRESSED)
                    joyb |= 1 << ev.jbutton.button;
                else
                    joyb &= ~(1 << ev.jbutton.button);
            }
            break;

        case SDL_QUIT:
            quitevent = 1;
            rv=-1;
            break;

        default:
            //OSD_Printf("Got event (%d)\n", ev.type);
            break;
        }
    }

//        if (mousex|mousey) printf("%d,%d\n",mousex,mousey); ///

    sampletimer();

    if (moustat)
    {
        if ((mwheelup) && (mwheelup <= (unsigned)(totalclock - MWHEELTICKS)))
        {
            mouseb &= ~16;
            mwheelup = 0;
        }
        if ((mwheeldown) && (mwheeldown <= (unsigned)(totalclock - MWHEELTICKS)))
        {
            mouseb &= ~32;
            mwheeldown = 0;
        }
    }

#ifndef _WIN32
    startwin_idle(NULL);
#endif

#undef SetKey

    return rv;
}

#if (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION < 3) // SDL 1.2
// from SDL HG, modified
int32_t SDL_WaitEventTimeout(SDL_Event *event, int32_t timeout)
{
    uint32_t expiration = 0;

    if (timeout > 0)
        expiration = SDL_GetTicks() + timeout;

    for (;;)
    {
        SDL_PumpEvents();
        switch (SDL_PeepEvents(event, 1, SDL_GETEVENT, ~0))   //SDL_FIRSTEVENT, SDL_LASTEVENT)) {
        {
        case -1:
            return 0;
        case 1:
            return 1;
        case 0:
            if (timeout == 0)
            {
                /* Polling and no events, just return */
                return 0;
            }
            if (timeout > 0 && ((int32_t)(SDL_GetTicks() - expiration) >= 0))
            {
                /* Timeout expired and no events */
                return 0;
            }
            SDL_Delay(10);
            break;
        }
    }
}
#endif

#if (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION < 3) // SDL 1.2
static int32_t buildkeytranslationtable(void)
{
    memset(keytranslation,0,sizeof(keytranslation));

#define MAP(x,y) keytranslation[x] = y
    MAP(SDLK_BACKSPACE,	0xe);
    MAP(SDLK_TAB,		0xf);
    MAP(SDLK_RETURN,	0x1c);
    MAP(SDLK_PAUSE,		0x59);	// 0x1d + 0x45 + 0x9d + 0xc5
    MAP(SDLK_ESCAPE,	0x1);
    MAP(SDLK_SPACE,		0x39);
    MAP(SDLK_EXCLAIM,	0x2);	// '1'
    MAP(SDLK_QUOTEDBL,	0x28);	// '''
    MAP(SDLK_HASH,		0x4);	// '3'
    MAP(SDLK_DOLLAR,	0x5);	// '4'
    MAP(37,			0x6);	// '5' <-- where's the keysym SDL guys?
    MAP(SDLK_AMPERSAND,	0x8);	// '7'
    MAP(SDLK_QUOTE,		0x28);	// '''
    MAP(SDLK_LEFTPAREN,	0xa);	// '9'
    MAP(SDLK_RIGHTPAREN,	0xb);	// '0'
    MAP(SDLK_ASTERISK,	0x9);	// '8'
    MAP(SDLK_PLUS,		0xd);	// '='
    MAP(SDLK_COMMA,		0x33);
    MAP(SDLK_MINUS,		0xc);
    MAP(SDLK_PERIOD,	0x34);
    MAP(SDLK_SLASH,		0x35);
    MAP(SDLK_0,		0xb);
    MAP(SDLK_1,		0x2);
    MAP(SDLK_2,		0x3);
    MAP(SDLK_3,		0x4);
    MAP(SDLK_4,		0x5);
    MAP(SDLK_5,		0x6);
    MAP(SDLK_6,		0x7);
    MAP(SDLK_7,		0x8);
    MAP(SDLK_8,		0x9);
    MAP(SDLK_9,		0xa);
    MAP(SDLK_COLON,		0x27);
    MAP(SDLK_SEMICOLON,	0x27);
    MAP(SDLK_LESS,		0x33);
    MAP(SDLK_EQUALS,	0xd);
    MAP(SDLK_GREATER,	0x34);
    MAP(SDLK_QUESTION,	0x35);
    MAP(SDLK_AT,		0x3);	// '2'
    MAP(SDLK_LEFTBRACKET,	0x1a);
    MAP(SDLK_BACKSLASH,	0x2b);
    MAP(SDLK_RIGHTBRACKET,	0x1b);
    MAP(SDLK_CARET,		0x7);	// '7'
    MAP(SDLK_UNDERSCORE,	0xc);
    MAP(SDLK_BACKQUOTE,	0x29);
    MAP(SDLK_a,		0x1e);
    MAP(SDLK_b,		0x30);
    MAP(SDLK_c,		0x2e);
    MAP(SDLK_d,		0x20);
    MAP(SDLK_e,		0x12);
    MAP(SDLK_f,		0x21);
    MAP(SDLK_g,		0x22);
    MAP(SDLK_h,		0x23);
    MAP(SDLK_i,		0x17);
    MAP(SDLK_j,		0x24);
    MAP(SDLK_k,		0x25);
    MAP(SDLK_l,		0x26);
    MAP(SDLK_m,		0x32);
    MAP(SDLK_n,		0x31);
    MAP(SDLK_o,		0x18);
    MAP(SDLK_p,		0x19);
    MAP(SDLK_q,		0x10);
    MAP(SDLK_r,		0x13);
    MAP(SDLK_s,		0x1f);
    MAP(SDLK_t,		0x14);
    MAP(SDLK_u,		0x16);
    MAP(SDLK_v,		0x2f);
    MAP(SDLK_w,		0x11);
    MAP(SDLK_x,		0x2d);
    MAP(SDLK_y,		0x15);
    MAP(SDLK_z,		0x2c);
    MAP(SDLK_DELETE,	0xd3);
    MAP(SDLK_KP0,		0x52);
    MAP(SDLK_KP1,		0x4f);
    MAP(SDLK_KP2,		0x50);
    MAP(SDLK_KP3,		0x51);
    MAP(SDLK_KP4,		0x4b);
    MAP(SDLK_KP5,		0x4c);
    MAP(SDLK_KP6,		0x4d);
    MAP(SDLK_KP7,		0x47);
    MAP(SDLK_KP8,		0x48);
    MAP(SDLK_KP9,		0x49);
    MAP(SDLK_KP_PERIOD,	0x53);
    MAP(SDLK_KP_DIVIDE,	0xb5);
    MAP(SDLK_KP_MULTIPLY,	0x37);
    MAP(SDLK_KP_MINUS,	0x4a);
    MAP(SDLK_KP_PLUS,	0x4e);
    MAP(SDLK_KP_ENTER,	0x9c);
    //MAP(SDLK_KP_EQUALS,	);
    MAP(SDLK_UP,		0xc8);
    MAP(SDLK_DOWN,		0xd0);
    MAP(SDLK_RIGHT,		0xcd);
    MAP(SDLK_LEFT,		0xcb);
    MAP(SDLK_INSERT,	0xd2);
    MAP(SDLK_HOME,		0xc7);
    MAP(SDLK_END,		0xcf);
    MAP(SDLK_PAGEUP,	0xc9);
    MAP(SDLK_PAGEDOWN,	0xd1);
    MAP(SDLK_F1,		0x3b);
    MAP(SDLK_F2,		0x3c);
    MAP(SDLK_F3,		0x3d);
    MAP(SDLK_F4,		0x3e);
    MAP(SDLK_F5,		0x3f);
    MAP(SDLK_F6,		0x40);
    MAP(SDLK_F7,		0x41);
    MAP(SDLK_F8,		0x42);
    MAP(SDLK_F9,		0x43);
    MAP(SDLK_F10,		0x44);
    MAP(SDLK_F11,		0x57);
    MAP(SDLK_F12,		0x58);
    MAP(SDLK_NUMLOCK,	0x45);
    MAP(SDLK_CAPSLOCK,	0x3a);
    MAP(SDLK_SCROLLOCK,	0x46);
    MAP(SDLK_RSHIFT,	0x36);
    MAP(SDLK_LSHIFT,	0x2a);
    MAP(SDLK_RCTRL,		0x9d);
    MAP(SDLK_LCTRL,		0x1d);
    MAP(SDLK_RALT,		0xb8);
    MAP(SDLK_LALT,		0x38);
    MAP(SDLK_LSUPER,	0xdb);	// win l
    MAP(SDLK_RSUPER,	0xdc);	// win r
    MAP(SDLK_PRINT,		-2);	// 0xaa + 0xb7
    MAP(SDLK_SYSREQ,	0x54);	// alt+printscr
    MAP(SDLK_BREAK,		0xb7);	// ctrl+pause
    MAP(SDLK_MENU,		0xdd);	// win menu?
#undef MAP

    return 0;
}
#else // if SDL 1.3
static int32_t buildkeytranslationtable(void)
{
    memset(keytranslation,0,sizeof(keytranslation));

#define MAP(x,y) keytranslation[x] = y
    MAP(SDL_SCANCODE_BACKSPACE,	0xe);
    MAP(SDL_SCANCODE_TAB,		0xf);
    MAP(SDL_SCANCODE_RETURN,	0x1c);
    MAP(SDL_SCANCODE_PAUSE,		0x59);	// 0x1d + 0x45 + 0x9d + 0xc5
    MAP(SDL_SCANCODE_ESCAPE,	0x1);
    MAP(SDL_SCANCODE_SPACE,		0x39);
    MAP(SDL_SCANCODE_COMMA,		0x33);
    MAP(SDL_SCANCODE_MINUS,		0xc);
    MAP(SDL_SCANCODE_PERIOD,	0x34);
    MAP(SDL_SCANCODE_SLASH,		0x35);
    MAP(SDL_SCANCODE_0,		0xb);
    MAP(SDL_SCANCODE_1,		0x2);
    MAP(SDL_SCANCODE_2,		0x3);
    MAP(SDL_SCANCODE_3,		0x4);
    MAP(SDL_SCANCODE_4,		0x5);
    MAP(SDL_SCANCODE_5,		0x6);
    MAP(SDL_SCANCODE_6,		0x7);
    MAP(SDL_SCANCODE_7,		0x8);
    MAP(SDL_SCANCODE_8,		0x9);
    MAP(SDL_SCANCODE_9,		0xa);
    MAP(SDL_SCANCODE_SEMICOLON,	0x27);
    MAP(SDL_SCANCODE_EQUALS,	0xd);
    MAP(SDL_SCANCODE_LEFTBRACKET,	0x1a);
    MAP(SDL_SCANCODE_BACKSLASH,	0x2b);
    MAP(SDL_SCANCODE_RIGHTBRACKET,	0x1b);
    MAP(SDL_SCANCODE_A,		0x1e);
    MAP(SDL_SCANCODE_B,		0x30);
    MAP(SDL_SCANCODE_C,		0x2e);
    MAP(SDL_SCANCODE_D,		0x20);
    MAP(SDL_SCANCODE_E,		0x12);
    MAP(SDL_SCANCODE_F,		0x21);
    MAP(SDL_SCANCODE_G,		0x22);
    MAP(SDL_SCANCODE_H,		0x23);
    MAP(SDL_SCANCODE_I,		0x17);
    MAP(SDL_SCANCODE_J,		0x24);
    MAP(SDL_SCANCODE_K,		0x25);
    MAP(SDL_SCANCODE_L,		0x26);
    MAP(SDL_SCANCODE_M,		0x32);
    MAP(SDL_SCANCODE_N,		0x31);
    MAP(SDL_SCANCODE_O,		0x18);
    MAP(SDL_SCANCODE_P,		0x19);
    MAP(SDL_SCANCODE_Q,		0x10);
    MAP(SDL_SCANCODE_R,		0x13);
    MAP(SDL_SCANCODE_S,		0x1f);
    MAP(SDL_SCANCODE_T,		0x14);
    MAP(SDL_SCANCODE_U,		0x16);
    MAP(SDL_SCANCODE_V,		0x2f);
    MAP(SDL_SCANCODE_W,		0x11);
    MAP(SDL_SCANCODE_X,		0x2d);
    MAP(SDL_SCANCODE_Y,		0x15);
    MAP(SDL_SCANCODE_Z,		0x2c);
    MAP(SDL_SCANCODE_DELETE,	0xd3);
    MAP(SDL_SCANCODE_KP_0,		0x52);
    MAP(SDL_SCANCODE_KP_1,		0x4f);
    MAP(SDL_SCANCODE_KP_2,		0x50);
    MAP(SDL_SCANCODE_KP_3,		0x51);
    MAP(SDL_SCANCODE_KP_4,		0x4b);
    MAP(SDL_SCANCODE_KP_5,		0x4c);
    MAP(SDL_SCANCODE_KP_6,		0x4d);
    MAP(SDL_SCANCODE_KP_7,		0x47);
    MAP(SDL_SCANCODE_KP_8,		0x48);
    MAP(SDL_SCANCODE_KP_9,		0x49);
    MAP(SDL_SCANCODE_KP_PERIOD,	0x53);
    MAP(SDL_SCANCODE_KP_DIVIDE,	0xb5);
    MAP(SDL_SCANCODE_KP_MULTIPLY,	0x37);
    MAP(SDL_SCANCODE_KP_MINUS,	0x4a);
    MAP(SDL_SCANCODE_KP_PLUS,	0x4e);
    MAP(SDL_SCANCODE_KP_ENTER,	0x9c);
    //MAP(SDL_SCANCODE_KP_EQUALS,	);
    MAP(SDL_SCANCODE_UP,		0xc8);
    MAP(SDL_SCANCODE_DOWN,		0xd0);
    MAP(SDL_SCANCODE_RIGHT,		0xcd);
    MAP(SDL_SCANCODE_LEFT,		0xcb);
    MAP(SDL_SCANCODE_INSERT,	0xd2);
    MAP(SDL_SCANCODE_HOME,		0xc7);
    MAP(SDL_SCANCODE_END,		0xcf);
    MAP(SDL_SCANCODE_PAGEUP,	0xc9);
    MAP(SDL_SCANCODE_PAGEDOWN,	0xd1);
    MAP(SDL_SCANCODE_F1,		0x3b);
    MAP(SDL_SCANCODE_F2,		0x3c);
    MAP(SDL_SCANCODE_F3,		0x3d);
    MAP(SDL_SCANCODE_F4,		0x3e);
    MAP(SDL_SCANCODE_F5,		0x3f);
    MAP(SDL_SCANCODE_F6,		0x40);
    MAP(SDL_SCANCODE_F7,		0x41);
    MAP(SDL_SCANCODE_F8,		0x42);
    MAP(SDL_SCANCODE_F9,		0x43);
    MAP(SDL_SCANCODE_F10,		0x44);
    MAP(SDL_SCANCODE_F11,		0x57);
    MAP(SDL_SCANCODE_F12,		0x58);
    MAP(SDL_SCANCODE_NUMLOCKCLEAR,	0x45);
    MAP(SDL_SCANCODE_CAPSLOCK,	0x3a);
    MAP(SDL_SCANCODE_SCROLLLOCK,	0x46);
    MAP(SDL_SCANCODE_RSHIFT,	0x36);
    MAP(SDL_SCANCODE_LSHIFT,	0x2a);
    MAP(SDL_SCANCODE_RCTRL,		0x9d);
    MAP(SDL_SCANCODE_LCTRL,		0x1d);
    MAP(SDL_SCANCODE_RALT,		0xb8);
    MAP(SDL_SCANCODE_LALT,		0x38);
    MAP(SDL_SCANCODE_LGUI,	0xdb);	// win l
    MAP(SDL_SCANCODE_RGUI,	0xdc);	// win r
    MAP(SDL_SCANCODE_PRINTSCREEN,		-2);	// 0xaa + 0xb7
    MAP(SDL_SCANCODE_SYSREQ,	0x54);	// alt+printscr
    MAP(SDL_SCANCODE_PAUSE,		0xb7);	// ctrl+pause
    MAP(SDL_SCANCODE_MENU,		0xdd);	// win menu?
    MAP(SDL_SCANCODE_GRAVE,     0x29);  // tilde
#undef MAP

    return 0;
}
#endif
