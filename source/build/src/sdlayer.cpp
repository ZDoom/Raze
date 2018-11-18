// SDL interface layer for the Build Engine
// Use SDL 1.2 or 2.0 from http://www.libsdl.org

#include <signal.h>

#include "a.h"
#include "build.h"
#include "cache1d.h"
#include "compat.h"
#include "engine_priv.h"
#include "osd.h"
#include "palette.h"
#include "renderlayer.h"
#include "sdl_inc.h"
#include "softsurface.h"

#ifdef USE_OPENGL
# include "glad/glad.h"
# include "glbuild.h"
# include "glsurface.h"
#endif

#if defined HAVE_GTK2
# include "gtkbits.h"
#endif

#ifdef __ANDROID__
# include <android/log.h>
#elif defined __APPLE__
# include "osxbits.h"
# include <mach/mach.h>
# include <mach/mach_time.h>
#elif defined GEKKO
# include "wiibits.h"
# include <ogc/lwp.h>
# include <ogc/lwp_watchdog.h>
#elif defined _WIN32
# include "winbits.h"
#endif

#if SDL_MAJOR_VERSION != 1
static SDL_version linked;
#endif

#if !defined STARTUP_SETUP_WINDOW
int32_t startwin_open(void) { return 0; }
int32_t startwin_close(void) { return 0; }
int32_t startwin_puts(const char *s) { UNREFERENCED_PARAMETER(s); return 0; }
int32_t startwin_idle(void *s) { UNREFERENCED_PARAMETER(s); return 0; }
int32_t startwin_settitle(const char *s) { UNREFERENCED_PARAMETER(s); return 0; }
int32_t startwin_run(void) { return 0; }
#endif

/// These can be useful for debugging sometimes...
//#define SDL_WM_GrabInput(x) SDL_WM_GrabInput(SDL_GRAB_OFF)
//#define SDL_ShowCursor(x) SDL_ShowCursor(SDL_ENABLE)

#define SURFACE_FLAGS	(SDL_SWSURFACE|SDL_HWPALETTE|SDL_HWACCEL)

// undefine to restrict windowed resolutions to conventional sizes
#define ANY_WINDOWED_SIZE

// fix for mousewheel
int32_t inputchecked = 0;

char quitevent=0, appactive=1, novideo=0;

// video
static SDL_Surface *sdl_surface/*=NULL*/;

#if SDL_MAJOR_VERSION==2
static SDL_Window *sdl_window=NULL;
static SDL_GLContext sdl_context=NULL;
#endif

int32_t xres=-1, yres=-1, bpp=0, fullscreen=0, bytesperline;
intptr_t frameplace=0;
int32_t lockcount=0;
char modechange=1;
char offscreenrendering=0;
char videomodereset = 0;
int32_t nofog=0;
#ifndef EDUKE32_GLES
static uint16_t sysgamma[3][256];
#endif
#ifdef USE_OPENGL
// OpenGL stuff
char nogl=0;
#endif
static int32_t vsync_renderlayer;
int32_t maxrefreshfreq=0;
#if SDL_MAJOR_VERSION==2
static uint32_t currentVBlankInterval=0;
#endif

// last gamma, contrast, brightness
static float lastvidgcb[3];

//#define KEY_PRINT_DEBUG

#include "sdlkeytrans.cpp"

static SDL_Surface *appicon = NULL;
#if !defined __APPLE__ && !defined EDUKE32_TOUCH_DEVICES
static SDL_Surface *loadappicon(void);
#endif

static mutex_t m_initprintf;

// Joystick dead and saturation zones
uint16_t *joydead, *joysatur;

#ifdef _WIN32
# if SDL_MAJOR_VERSION != 1
//
// win_gethwnd() -- gets the window handle
//
HWND win_gethwnd(void)
{
    struct SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);

    if (SDL_GetWindowWMInfo(sdl_window, &wmInfo) != SDL_TRUE)
        return 0;

    if (wmInfo.subsystem == SDL_SYSWM_WINDOWS)
        return wmInfo.info.win.window;

    initprintf("win_gethwnd: Unknown WM subsystem?!\n");

    return 0;
}
# endif
//
// win_gethinstance() -- gets the application instance
//
HINSTANCE win_gethinstance(void)
{
    return (HINSTANCE)GetModuleHandle(NULL);
}
#endif


int32_t wm_msgbox(const char *name, const char *fmt, ...)
{
    char buf[2048];
    va_list va;

    UNREFERENCED_PARAMETER(name);

    va_start(va,fmt);
    vsnprintf(buf,sizeof(buf),fmt,va);
    va_end(va);

#if defined EDUKE32_OSX
    return osx_msgbox(name, buf);
#elif defined _WIN32
    MessageBox(win_gethwnd(),buf,name,MB_OK|MB_TASKMODAL);
    return 0;
#elif defined EDUKE32_TOUCH_DEVICES
    initprintf("wm_msgbox called. Message: %s: %s",name,buf);
    return 0;
#elif defined GEKKO
    puts(buf);
    return 0;
#else
# if defined HAVE_GTK2
    if (gtkbuild_msgbox(name, buf) >= 0)
        return 0;
# endif
# if SDL_MAJOR_VERSION > 1
#  if !defined _WIN32
    // Replace all tab chars with spaces because the hand-rolled SDL message
    // box diplays the former as N/L instead of whitespace.
    for (size_t i=0; i<sizeof(buf); i++)
        if (buf[i] == '\t')
            buf[i] = ' ';
#  endif
    return SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, name, buf, NULL);
# else
    puts(buf);
    puts("   (press Return or Enter to continue)");
    getchar();

    return 0;
# endif
#endif
}

int32_t wm_ynbox(const char *name, const char *fmt, ...)
{
    char buf[2048];
    va_list va;

    UNREFERENCED_PARAMETER(name);

    va_start(va,fmt);
    vsnprintf(buf,sizeof(buf),fmt,va);
    va_end(va);

#if defined EDUKE32_OSX
    return osx_ynbox(name, buf);
#elif defined _WIN32
    return (MessageBox(win_gethwnd(),buf,name,MB_YESNO|MB_ICONQUESTION|MB_TASKMODAL) == IDYES);
#elif defined EDUKE32_TOUCH_DEVICES
    initprintf("wm_ynbox called, this is bad! Message: %s: %s",name,buf);
    initprintf("Returning false..");
    return 0;
#elif defined GEKKO
    puts(buf);
    puts("Assuming yes...");
    return 1;
#else
# if defined HAVE_GTK2
    int ret = gtkbuild_ynbox(name, buf);
    if (ret >= 0)
        return ret;
# endif
# if SDL_MAJOR_VERSION > 1
    int r = -1;

    const SDL_MessageBoxButtonData buttons[] = {
        {
            SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            0,
            "No"
        },{
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            1,
            "Yes"
        },
    };

    SDL_MessageBoxData data = {
        SDL_MESSAGEBOX_INFORMATION,
        NULL, /* no parent window */
        name,
        buf,
        2,
        buttons,
        NULL /* Default color scheme */
    };

    SDL_ShowMessageBox(&data, &r);

    return r;
# else
    char c;

    puts(buf);
    puts("   (type 'Y' or 'N', and press Return or Enter to continue)");
    do c = getchar(); while (c != 'Y' && c != 'y' && c != 'N' && c != 'n');
    return c == 'Y' || c == 'y';
# endif
#endif
}

void wm_setapptitle(const char *name)
{
#ifndef EDUKE32_TOUCH_DEVICES
    if (name != apptitle)
        Bstrncpyz(apptitle, name, sizeof(apptitle));

#if !defined(__APPLE__)
    if (!appicon)
        appicon = loadappicon();
#endif

#if SDL_MAJOR_VERSION == 1
    SDL_WM_SetCaption(apptitle, NULL);

    if (appicon && sdl_surface)
        SDL_WM_SetIcon(appicon, 0);
#else
    if (sdl_window)
    {
        SDL_SetWindowTitle(sdl_window, apptitle);

        if (appicon)
        {
#if defined _WIN32
        if (!EDUKE32_SDL_LINKED_PREREQ(linked, 2, 0, 5))
#endif
            SDL_SetWindowIcon(sdl_window, appicon);
        }
    }
#endif

    startwin_settitle(apptitle);
#else
    UNREFERENCED_PARAMETER(name);
#endif
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

/* XXX: libexecinfo could be used on systems without gnu libc. */
#if !defined _WIN32 && defined __GNUC__ && !defined __OpenBSD__ && !(defined __APPLE__ && defined __BIG_ENDIAN__) && !defined GEKKO && !defined EDUKE32_TOUCH_DEVICES && !defined __OPENDINGUX__
# define PRINTSTACKONSEGV 1
# include <execinfo.h>
#endif

static inline char grabmouse_low(char a);

#ifndef __ANDROID__
static void attach_debugger_here(void) {}

static void sighandler(int signum)
{
    UNREFERENCED_PARAMETER(signum);
    //    if (signum==SIGSEGV)
    {
        grabmouse_low(0);
#if PRINTSTACKONSEGV
        {
            void *addr[32];
            int32_t errfd = fileno(stderr);
            int32_t n=backtrace(addr, ARRAY_SIZE(addr));
            backtrace_symbols_fd(addr, n, errfd);
        }
        // This is useful for attaching the debugger post-mortem. For those pesky
        // cases where the program runs through happily when inspected from the start.
        //        usleep(15000000);
#endif
        attach_debugger_here();
        app_crashhandler();
        uninitsystem();
        Bexit(8);
    }
}
#endif

#ifdef __ANDROID__
int mobile_halted = 0;
#ifdef __cplusplus
extern "C"
{
#endif
void G_Shutdown(void);
#ifdef __cplusplus
}
#endif

int sdlayer_mobilefilter(void *userdata, SDL_Event *event)
{
    switch (event->type)
    {
        case SDL_APP_TERMINATING:
            // yes, this calls into the game, ugh
            if (mobile_halted == 1)
                G_Shutdown();

            mobile_halted = 1;
            return 0;
        case SDL_APP_LOWMEMORY:
            gltexinvalidatetype(INVALIDATE_ALL);
            return 0;
        case SDL_APP_WILLENTERBACKGROUND:
            mobile_halted = 1;
            return 0;
        case SDL_APP_DIDENTERBACKGROUND:
            gltexinvalidatetype(INVALIDATE_ALL);
            // tear down video?
            return 0;
        case SDL_APP_WILLENTERFOREGROUND:
            // restore video?
            return 0;
        case SDL_APP_DIDENTERFOREGROUND:
            mobile_halted = 0;
            return 0;
        default:
            return 1;//!halt;
    }

    UNREFERENCED_PARAMETER(userdata);
}
#endif

#ifdef __ANDROID__
# include <setjmp.h>
static jmp_buf eduke32_exit_jmp_buf;
static int eduke32_return_value;

void eduke32_exit_return(int retval)
{
    eduke32_return_value = retval;
    longjmp(eduke32_exit_jmp_buf, 1);
    EDUKE32_UNREACHABLE_SECTION(return);
}
#endif

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
#elif defined __ANDROID__
# ifdef __cplusplus
extern "C" int eduke32_android_main(int argc, char const *argv[]);
# endif
int eduke32_android_main(int argc, char const *argv[])
#elif defined GEKKO
int SDL_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
#ifdef __ANDROID__
    if (setjmp(eduke32_exit_jmp_buf))
    {
        return eduke32_return_value;
    }
#endif

#if defined _WIN32 && defined SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING
    // Thread naming interferes with debugging using MinGW-w64's GDB.
    SDL_SetHint(SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING, "1");
#endif

    int32_t r;

#ifdef USE_OPENGL
    char *argp;

    if ((argp = Bgetenv("BUILD_NOFOG")) != NULL)
        nofog = Batol(argp);

#ifndef _WIN32
    setenv("__GL_THREADED_OPTIMIZATIONS", "1", 0);
#endif
#endif

    buildkeytranslationtable();

#ifndef __ANDROID__
    signal(SIGSEGV, sighandler);
    signal(SIGILL, sighandler);  /* clang -fcatch-undefined-behavior uses an ill. insn */
    signal(SIGABRT, sighandler);
    signal(SIGFPE, sighandler);
#else
    SDL_SetEventFilter(sdlayer_mobilefilter, NULL);
#endif

#ifdef _WIN32
    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(hPrevInst);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    win_open();

    if (!CheckWinVersion())
    {
        MessageBox(0, "This application requires a newer Windows version to run.", apptitle, MB_OK | MB_ICONSTOP);
        return -1;
    }
#elif defined(GEKKO)
    wii_open();
#elif defined(HAVE_GTK2)
    // Pre-initialize SDL video system in order to make sure XInitThreads() is called
    // before GTK starts talking to X11.
    uint32_t inited = SDL_WasInit(SDL_INIT_VIDEO);
    if (inited == 0)
        SDL_Init(SDL_INIT_VIDEO);
    else if (!(inited & SDL_INIT_VIDEO))
        SDL_InitSubSystem(SDL_INIT_VIDEO);
    gtkbuild_init(&argc, &argv);
#endif

    startwin_open();
    maybe_redirect_outputs();

#ifdef _WIN32
    char *argvbuf;
    int32_t buildargc = win_buildargs(&argvbuf);
    const char **buildargv = (const char **) Xmalloc(sizeof(char *)*(buildargc+1));
    char *wp = argvbuf;

    for (bssize_t i=0; i<buildargc; i++, wp++)
    {
        buildargv[i] = wp;
        while (*wp) wp++;
    }
    buildargv[buildargc] = NULL;

    r = app_main(buildargc, (const char **)buildargv);
#else
    r = app_main(argc, (char const * const *)argv);
#endif

    startwin_close();

#ifdef _WIN32
    win_close();
#elif defined(HAVE_GTK2)
    gtkbuild_exit(r);
#endif

    return r;
}


#if SDL_MAJOR_VERSION != 1
int32_t videoSetVsync(int32_t newSync)
{
    if (vsync_renderlayer == newSync)
        return newSync;

#ifdef USE_OPENGL
    if (sdl_context)
    {
        int result = SDL_GL_SetSwapInterval(newSync);

        if (result == -1)
        {
            if (newSync == -1)
            {
                newSync = 1;
                result = SDL_GL_SetSwapInterval(newSync);
            }

            if (result == -1)
            {
                newSync = 0;
                OSD_Printf("Unable to enable VSync!\n");
            }
        }

        vsync_renderlayer = newSync;
    }
    else
#endif
    {
        vsync_renderlayer = newSync;

        videoResetMode();
        if (videoSetGameMode(fullscreen, xres, yres, bpp, upscalefactor))
            OSD_Printf("restartvid: Reset failed...\n");
    }

    return newSync;
}
#endif

int32_t sdlayer_checkversion(void);
#if SDL_MAJOR_VERSION != 1
int32_t sdlayer_checkversion(void)
{
    SDL_version compiled;

    SDL_GetVersion(&linked);
    SDL_VERSION(&compiled);

    if (!Bmemcmp(&compiled, &linked, sizeof(SDL_version)))
        initprintf("Initializing SDL %d.%d.%d\n",
            compiled.major, compiled.minor, compiled.patch);
    else
    initprintf("Initializing SDL %d.%d.%d"
               " (built against SDL version %d.%d.%d)\n",
               linked.major, linked.minor, linked.patch, compiled.major, compiled.minor, compiled.patch);

    if (SDL_VERSIONNUM(linked.major, linked.minor, linked.patch) < SDL_REQUIREDVERSION)
    {
        /*reject running under SDL versions older than what is stated in sdl_inc.h */
        initprintf("You need at least v%d.%d.%d of SDL to run this game\n",SDL_MIN_X,SDL_MIN_Y,SDL_MIN_Z);
        return -1;
    }

    return 0;
}

//
// initsystem() -- init SDL systems
//
int32_t initsystem(void)
{
    const int sdlinitflags = SDL_INIT_VIDEO;

    mutex_init(&m_initprintf);

#ifdef _WIN32
    win_init();
#endif

    if (sdlayer_checkversion())
        return -1;

    int32_t err = 0;
    uint32_t inited = SDL_WasInit(sdlinitflags);
    if (inited == 0)
        err = SDL_Init(sdlinitflags);
    else if ((inited & sdlinitflags) != sdlinitflags)
        err = SDL_InitSubSystem(sdlinitflags & ~inited);

    if (err)
    {
        initprintf("Initialization failed! (%s)\nNon-interactive mode enabled\n", SDL_GetError());
        novideo = 1;
#ifdef USE_OPENGL
        nogl = 1;
#endif
    }

#if SDL_MAJOR_VERSION > 1
    SDL_StopTextInput();
#endif

    atexit(uninitsystem);

    frameplace = 0;
    lockcount = 0;

    if (!novideo)
    {
#ifdef USE_OPENGL
        if (SDL_GL_LoadLibrary(0))
        {
            initprintf("Failed loading OpenGL Driver.  GL modes will be unavailable. Error: %s\n", SDL_GetError());
            nogl = 1;
        }
#ifdef POLYMER
        if (loadglulibrary(getenv("BUILD_GLULIB")))
        {
            initprintf("Failed loading GLU.  GL modes will be unavailable. Error: %s\n", SDL_GetError());
            nogl = 1;
        }
#endif
#endif

#ifndef _WIN32
        const char *drvname = SDL_GetVideoDriver(0);

        if (drvname)
            initprintf("Using \"%s\" video driver\n", drvname);
#endif
        wm_setapptitle(apptitle);
    }

    return 0;
}
#endif


//
// uninitsystem() -- uninit SDL systems
//
void uninitsystem(void)
{
    uninitinput();
    timerUninit();

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
# if SDL_MAJOR_VERSION!=1
    SDL_GL_UnloadLibrary();
# endif
# ifdef POLYMER
    unloadglulibrary();
# endif
#endif
}


//
// system_getcvars() -- propagate any cvars that are read post-initialization
//
void system_getcvars(void)
{
    vsync = videoSetVsync(vsync);
}

//
// initprintf() -- prints a formatted string to the intitialization window
//
void initprintf(const char *f, ...)
{
    va_list va;
    char buf[2048];

    va_start(va, f);
    Bvsnprintf(buf, sizeof(buf), f, va);
    va_end(va);

    initputs(buf);
}


//
// initputs() -- prints a string to the intitialization window
//
void initputs(const char *buf)
{
    static char dabuf[2048];

#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO,"DUKE", "%s",buf);
#endif
    OSD_Puts(buf);
//    Bprintf("%s", buf);

    mutex_lock(&m_initprintf);
    if (Bstrlen(dabuf) + Bstrlen(buf) > 1022)
    {
        startwin_puts(dabuf);
        Bmemset(dabuf, 0, sizeof(dabuf));
    }

    Bstrcat(dabuf,buf);

    if (g_logFlushWindow || Bstrlen(dabuf) > 768)
    {
        startwin_puts(dabuf);
#ifndef _WIN32
        startwin_idle(NULL);
#else
        handleevents();
#endif
        Bmemset(dabuf, 0, sizeof(dabuf));
    }
    mutex_unlock(&m_initprintf);
}

//
// debugprintf() -- prints a formatted debug string to stderr
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
    int32_t i;

#ifdef _WIN32
    Win_GetOriginalLayoutName();
    Win_SetKeyboardLayoutUS(1);
#endif

#if defined EDUKE32_OSX
    // force OS X to operate in >1 button mouse mode so that LMB isn't adulterated
    if (!getenv("SDL_HAS3BUTTONMOUSE"))
    {
        static char sdl_has3buttonmouse[] = "SDL_HAS3BUTTONMOUSE=1";
        putenv(sdl_has3buttonmouse);
    }
#endif

    inputdevices = 1 | 2;  // keyboard (1) and mouse (2)
    g_mouseGrabbed = 0;

    memset(g_keyNameTable, 0, sizeof(g_keyNameTable));

#if SDL_MAJOR_VERSION == 1
#define SDL_SCANCODE_TO_KEYCODE(x) (SDLKey)(x)
#define SDL_JoystickNameForIndex(x) SDL_JoystickName(x)
#define SDL_NUM_SCANCODES SDLK_LAST
    if (SDL_EnableKeyRepeat(250, 30))
        initprintf("Error enabling keyboard repeat.\n");
    SDL_EnableUNICODE(1);  // let's hope this doesn't hit us too hard
#endif

    for (i = SDL_NUM_SCANCODES - 1; i >= 0; i--)
    {
        if (!keytranslation[i])
            continue;

        Bstrncpyz(g_keyNameTable[keytranslation[i]], SDL_GetKeyName(SDL_SCANCODE_TO_KEYCODE(i)), sizeof(g_keyNameTable[0]));
    }

    if (!SDL_InitSubSystem(SDL_INIT_JOYSTICK))
    {
        i = SDL_NumJoysticks();
        initprintf("%d joystick(s) found\n", i);

        for (int32_t j = 0; j < i; j++)
            initprintf("  %d. %s\n", j + 1, SDL_JoystickNameForIndex(j));

        joydev = SDL_JoystickOpen(0);

        if (joydev)
        {
            SDL_JoystickEventState(SDL_ENABLE);
            inputdevices |= 4;

            // KEEPINSYNC duke3d/src/gamedefs.h, mact/include/_control.h
            joystick.numAxes = min(9, SDL_JoystickNumAxes(joydev));
            joystick.numButtons = min(32, SDL_JoystickNumButtons(joydev));
            joystick.numHats = min((36-joystick.numButtons)/4,SDL_JoystickNumHats(joydev));
            initprintf("Joystick 1 has %d axes, %d buttons, and %d hat(s).\n", joystick.numAxes, joystick.numButtons, joystick.numHats);

            joystick.pAxis = (int32_t *)Xcalloc(joystick.numAxes, sizeof(int32_t));

            if (joystick.numHats)
                joystick.pHat = (int32_t *)Xcalloc(joystick.numHats, sizeof(int32_t));

            for (i = 0; i < joystick.numHats; i++) joystick.pHat[i] = -1;  // centre

            joydead = (uint16_t *)Xcalloc(joystick.numAxes, sizeof(uint16_t));
            joysatur = (uint16_t *)Xcalloc(joystick.numAxes, sizeof(uint16_t));
        }
    }

    return 0;
}

//
// uninitinput() -- uninit input system
//
void uninitinput(void)
{
#ifdef _WIN32
    Win_SetKeyboardLayoutUS(0);
#endif
    mouseUninit();

    if (joydev)
    {
        SDL_JoystickClose(joydev);
        joydev = NULL;
    }
}

#ifndef GEKKO
const char *joyGetName(int32_t what, int32_t num)
{
    static char tmp[64];

    switch (what)
    {
        case 0:  // axis
            if ((unsigned)num > (unsigned)joystick.numAxes)
                return NULL;
            Bsprintf(tmp, "Axis %d", num);
            return (char *)tmp;

        case 1:  // button
            if ((unsigned)num > (unsigned)joystick.numButtons)
                return NULL;
            Bsprintf(tmp, "Button %d", num);
            return (char *)tmp;

        case 2:  // hat
            if ((unsigned)num > (unsigned)joystick.numHats)
                return NULL;
            Bsprintf(tmp, "Hat %d", num);
            return (char *)tmp;

        default: return NULL;
    }
}
#endif


//
// initmouse() -- init mouse input
//
void mouseInit(void)
{
    mouseGrabInput(g_mouseEnabled = g_mouseLockedToWindow);  // FIXME - SA
}

//
// uninitmouse() -- uninit mouse input
//
void mouseUninit(void)
{
    mouseGrabInput(0);
    g_mouseEnabled = 0;
}


#if SDL_MAJOR_VERSION != 1
//
// grabmouse_low() -- show/hide mouse cursor, lower level (doesn't check state).
//                    furthermore return 0 if successful.
//

static inline char grabmouse_low(char a)
{
#if !defined EDUKE32_TOUCH_DEVICES
    /* FIXME: Maybe it's better to make sure that grabmouse_low
       is called only when a window is ready?                */
    if (sdl_window)
        SDL_SetWindowGrab(sdl_window, a ? SDL_TRUE : SDL_FALSE);
    return SDL_SetRelativeMouseMode(a ? SDL_TRUE : SDL_FALSE);
#else
    UNREFERENCED_PARAMETER(a);
    return 0;
#endif
}
#endif

//
// grabmouse() -- show/hide mouse cursor
//
void mouseGrabInput(bool grab)
{
    if (appactive && g_mouseEnabled)
    {
#if !defined EDUKE32_TOUCH_DEVICES
        if ((grab != g_mouseGrabbed) && !grabmouse_low(grab))
#endif
            g_mouseGrabbed = grab;
    }
    else
        g_mouseGrabbed = grab;

    g_mousePos.x = g_mousePos.y = 0;
}

void mouseLockToWindow(char a)
{
    if (!(a & 2))
    {
        mouseGrabInput(a);
        g_mouseLockedToWindow = g_mouseGrabbed;
    }

    SDL_ShowCursor((osd && osd->flags & OSD_CAPTURE) ? SDL_ENABLE : SDL_DISABLE);
}

//
// setjoydeadzone() -- sets the dead and saturation zones for the joystick
//
void joySetDeadZone(int32_t axis, uint16_t dead, uint16_t satur)
{
    joydead[axis] = dead;
    joysatur[axis] = satur;
}


//
// getjoydeadzone() -- gets the dead and saturation zones for the joystick
//
void joyGetDeadZone(int32_t axis, uint16_t *dead, uint16_t *satur)
{
    *dead = joydead[axis];
    *satur = joysatur[axis];
}


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

static uint32_t timerfreq;
static uint32_t timerlastsample;
int32_t timerticspersec=0;
static double msperu64tick = 0;
static void(*usertimercallback)(void) = NULL;


//
// inittimer() -- initialize timer
//
int32_t timerInit(int32_t tickspersecond)
{
    if (timerfreq) return 0;	// already installed

//    initprintf("Initializing timer\n");

#if defined(_WIN32) && SDL_MAJOR_VERSION == 1
    int32_t t = win_inittimer();
    if (t < 0)
        return t;
#endif

    timerfreq = 1000;
    timerticspersec = tickspersecond;
    timerlastsample = SDL_GetTicks() * timerticspersec / timerfreq;

    usertimercallback = NULL;

    msperu64tick = 1000.0 / (double)timerGetFreqU64();

    return 0;
}

//
// uninittimer() -- shut down timer
//
void timerUninit(void)
{
    timerfreq=0;
#if defined(_WIN32) && SDL_MAJOR_VERSION==1
    win_timerfreq=0;
#endif
    msperu64tick = 0;
}

//
// sampletimer() -- update totalclock
//
void timerUpdate(void)
{
    if (!timerfreq) return;

    int64_t i = SDL_GetTicks();
    int32_t n = tabledivide64(i * timerticspersec, timerfreq) - timerlastsample;

    if (n <= 0) return;

    totalclock += n;
    timerlastsample += n;

    if (usertimercallback)
        for (; n > 0; n--) usertimercallback();
}

#if defined LUNATIC
//
// getticks() -- returns the sdl ticks count
//
uint32_t timerGetTicks(void)
{
    return (uint32_t)SDL_GetTicks();
}
#endif

// high-resolution timers for profiling

#if SDL_MAJOR_VERSION != 1
uint64_t timerGetTicksU64(void)
{
    return SDL_GetPerformanceCounter();
}

uint64_t timerGetFreqU64(void)
{
    return SDL_GetPerformanceFrequency();
}
#endif

// Returns the time since an unspecified starting time in milliseconds.
// (May be not monotonic for certain configurations.)
ATTRIBUTE((flatten))
double timerGetHiTicks(void)
{
    return (double)timerGetTicksU64() * msperu64tick;
}

//
// gettimerfreq() -- returns the number of ticks per second the timer is configured to generate
//
int32_t timerGetFreq(void)
{
    return timerticspersec;
}


//
// installusertimercallback() -- set up a callback function to be called when the timer is fired
//
void(*timerSetCallback(void(*callback)(void)))(void)
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
static int sortmodes(const void *a_, const void *b_)
{

    auto a = (const struct validmode_t *)a_;
    auto b = (const struct validmode_t *)b_;

    int x;

    if ((x = a->fs   - b->fs)   != 0) return x;
    if ((x = a->bpp  - b->bpp)  != 0) return x;
    if ((x = a->xdim - b->xdim) != 0) return x;
    if ((x = a->ydim - b->ydim) != 0) return x;

    return 0;
}

static char modeschecked=0;

#if SDL_MAJOR_VERSION != 1
void videoGetModes(void)
{
    int32_t i, maxx = 0, maxy = 0;
    SDL_DisplayMode dispmode;

    if (modeschecked || novideo)
        return;

    validmodecnt = 0;
    //    initprintf("Detecting video modes:\n");

    // do fullscreen modes first
    for (i = 0; i < SDL_GetNumDisplayModes(0); i++)
    {
        SDL_GetDisplayMode(0, i, &dispmode);

        if (!SDL_CHECKMODE(dispmode.w, dispmode.h) ||
            (maxrefreshfreq && (dispmode.refresh_rate > maxrefreshfreq)))
            continue;

        // HACK: 8-bit == Software, 32-bit == OpenGL
        SDL_ADDMODE(dispmode.w, dispmode.h, 8, 1);
#ifdef USE_OPENGL
        if (!nogl)
            SDL_ADDMODE(dispmode.w, dispmode.h, 32, 1);
#endif
        if ((dispmode.w > maxx) || (dispmode.h > maxy))
        {
            maxx = dispmode.w;
            maxy = dispmode.h;
        }
    }

    SDL_CHECKFSMODES(maxx, maxy);

    // add windowed modes next
    // SDL sorts display modes largest to smallest, so we can just compare with mode 0
    // to make sure we aren't adding modes that are larger than the actual screen res
    SDL_GetDisplayMode(0, 0, &dispmode);

    for (i = 0; g_defaultVideoModes[i].x; i++)
    {
        auto const &mode = g_defaultVideoModes[i];

        if (mode.x > dispmode.w || mode.y > dispmode.h || !SDL_CHECKMODE(mode.x, mode.y))
            continue;

        // 8-bit == Software, 32-bit == OpenGL
        SDL_ADDMODE(mode.x, mode.y, 8, 0);

#ifdef USE_OPENGL
        if (nogl)
            continue;

        SDL_ADDMODE(mode.x, mode.y, 32, 0);
#endif
    }

    qsort((void *)validmode, validmodecnt, sizeof(struct validmode_t), &sortmodes);

    modeschecked = 1;
}
#endif

//
// checkvideomode() -- makes sure the video mode passed is legal
//
int32_t videoCheckMode(int32_t *x, int32_t *y, int32_t c, int32_t fs, int32_t forced)
{
    int32_t i, nearest=-1, dx, dy, odx=9999, ody=9999;

    videoGetModes();

    if (c>8
#ifdef USE_OPENGL
            && nogl
#endif
       ) return -1;

    // fix up the passed resolution values to be multiples of 8
    // and at least 320x200 or at most MAXXDIMxMAXYDIM
    *x = clamp(*x, 320, MAXXDIM);
    *y = clamp(*y, 200, MAXYDIM);

    for (i = 0; i < validmodecnt; i++)
    {
        if (validmode[i].bpp != c || validmode[i].fs != fs)
            continue;

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
            odx = dx;
            ody = dy;
        }
    }

#ifdef ANY_WINDOWED_SIZE
    if (!forced && (fs&1) == 0 && (nearest < 0 || (validmode[nearest].xdim!=*x || validmode[nearest].ydim!=*y)))
        return 0x7fffffffl;
#endif

    if (nearest < 0)
        return -1;

    *x = validmode[nearest].xdim;
    *y = validmode[nearest].ydim;

    return nearest;
}

static void destroy_window_resources()
{
/* We should NOT destroy the window surface. This is done automatically
   when SDL_DestroyWindow or SDL_SetVideoMode is called.             */

#if SDL_MAJOR_VERSION == 2
    if (sdl_context)
        SDL_GL_DeleteContext(sdl_context);
    sdl_context = NULL;
    if (sdl_window)
        SDL_DestroyWindow(sdl_window);
    sdl_window = NULL;
#endif
}

#ifdef USE_OPENGL
void sdlayer_setvideomode_opengl(void)
{
    glsurface_destroy();
    polymost_glreset();

    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);  // GL_FLAT
    glClearColor(0, 0, 0, 1.0);  // Black Background
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);  // Use FASTEST for ortho!
//    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

#ifndef EDUKE32_GLES
    glDisable(GL_DITHER);
#endif

    glinfo.vendor = (const char *) glGetString(GL_VENDOR);
    glinfo.renderer = (const char *) glGetString(GL_RENDERER);
    glinfo.version = (const char *) glGetString(GL_VERSION);
    glinfo.extensions = (const char *) glGetString(GL_EXTENSIONS);

#ifdef POLYMER
    if (!Bstrcmp(glinfo.vendor, "ATI Technologies Inc."))
    {
        pr_ati_fboworkaround = 1;
        initprintf("Enabling ATI FBO color attachment workaround.\n");

        if (Bstrstr(glinfo.renderer, "Radeon X1"))
        {
            pr_ati_nodepthoffset = 1;
            initprintf("Enabling ATI R520 polygon offset workaround.\n");
        }
        else
            pr_ati_nodepthoffset = 0;
#ifdef __APPLE__
        // See bug description at http://lists.apple.com/archives/mac-opengl/2005/Oct/msg00169.html
        if (!Bstrncmp(glinfo.renderer, "ATI Radeon 9600", 15))
        {
            pr_ati_textureformat_one = 1;
            initprintf("Enabling ATI Radeon 9600 texture format workaround.\n");
        }
        else
            pr_ati_textureformat_one = 0;
#endif
    }
    else
        pr_ati_fboworkaround = 0;
#endif  // defined POLYMER

    glinfo.maxanisotropy = 1.0;
    glinfo.bgra = 0;
    glinfo.clamptoedge = 1;
    glinfo.multitex = 1;

    // process the extensions string and flag stuff we recognize

    glinfo.texnpot = !!Bstrstr(glinfo.extensions, "GL_ARB_texture_non_power_of_two") || !!Bstrstr(glinfo.extensions, "GL_OES_texture_npot");
    glinfo.multisample = !!Bstrstr(glinfo.extensions, "GL_ARB_multisample");
    glinfo.nvmultisamplehint = !!Bstrstr(glinfo.extensions, "GL_NV_multisample_filter_hint");
    glinfo.arbfp = !!Bstrstr(glinfo.extensions, "GL_ARB_fragment_program");
    glinfo.depthtex = !!Bstrstr(glinfo.extensions, "GL_ARB_depth_texture");
    glinfo.shadow = !!Bstrstr(glinfo.extensions, "GL_ARB_shadow");
    glinfo.fbos = !!Bstrstr(glinfo.extensions, "GL_EXT_framebuffer_object") || !!Bstrstr(glinfo.extensions, "GL_OES_framebuffer_object");

#if !defined EDUKE32_GLES
    glinfo.texcompr = !!Bstrstr(glinfo.extensions, "GL_ARB_texture_compression") && Bstrcmp(glinfo.vendor, "ATI Technologies Inc.");
# ifdef DYNAMIC_GLEXT
    if (glinfo.texcompr && (!glCompressedTexImage2D || !glGetCompressedTexImage))
    {
        // lacking the necessary extensions to do this
        initprintf("Warning: the GL driver lacks necessary functions to use caching\n");
        glinfo.texcompr = 0;
    }
# endif

    glinfo.bgra = !!Bstrstr(glinfo.extensions, "GL_EXT_bgra");
    glinfo.clamptoedge = !!Bstrstr(glinfo.extensions, "GL_EXT_texture_edge_clamp") ||
                         !!Bstrstr(glinfo.extensions, "GL_SGIS_texture_edge_clamp");
    glinfo.rect =
    !!Bstrstr(glinfo.extensions, "GL_NV_texture_rectangle") || !!Bstrstr(glinfo.extensions, "GL_EXT_texture_rectangle");

    glinfo.multitex = !!Bstrstr(glinfo.extensions, "GL_ARB_multitexture");

    glinfo.envcombine = !!Bstrstr(glinfo.extensions, "GL_ARB_texture_env_combine");
    glinfo.vbos = !!Bstrstr(glinfo.extensions, "GL_ARB_vertex_buffer_object");
    glinfo.sm4 = !!Bstrstr(glinfo.extensions, "GL_EXT_gpu_shader4");
    glinfo.occlusionqueries = !!Bstrstr(glinfo.extensions, "GL_ARB_occlusion_query");
    glinfo.glsl = !!Bstrstr(glinfo.extensions, "GL_ARB_shader_objects");
    glinfo.debugoutput = !!Bstrstr(glinfo.extensions, "GL_ARB_debug_output");
    glinfo.bufferstorage = !!Bstrstr(glinfo.extensions, "GL_ARB_buffer_storage");
    glinfo.sync = !!Bstrstr(glinfo.extensions, "GL_ARB_sync");

    if (Bstrstr(glinfo.extensions, "WGL_3DFX_gamma_control"))
    {
        static int32_t warnonce;
        // 3dfx cards have issues with fog
        nofog = 1;
        if (!(warnonce & 1))
            initprintf("3dfx card detected: OpenGL fog disabled\n");
        warnonce |= 1;
    }
#else
    // don't bother checking because ETC2 et al. are not listed in extensions anyway
    glinfo.texcompr = 1; // !!Bstrstr(glinfo.extensions, "GL_OES_compressed_ETC1_RGB8_texture");
#endif

//    if (Bstrstr(glinfo.extensions, "GL_EXT_texture_filter_anisotropic"))
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glinfo.maxanisotropy);

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

//
// setvideomode() -- set SDL video mode
//

int32_t setvideomode_sdlcommon(int32_t *x, int32_t *y, int32_t c, int32_t fs, int32_t *regrab)
{
    if ((fs == fullscreen) && (*x == xres) && (*y == yres) && (c == bpp) && !videomodereset)
        return 0;

    if (videoCheckMode(x, y, c, fs, 0) < 0)
        return -1;

#ifdef GEKKO
    if (!sdl_surface) // only run this the first time we set a video mode
        wii_initgamevideo();
#endif

    startwin_close();

    if (g_mouseGrabbed)
    {
        *regrab = 1;
        mouseGrabInput(0);
    }

    while (lockcount) videoEndDrawing();

#ifdef USE_OPENGL
    if (sdl_surface)
    {
        if (bpp > 8)
            polymost_glreset();
    }
    if (!nogl)
    {
        if (bpp == 8)
            glsurface_destroy();
        if ((fs == fullscreen) && (*x == xres) && (*y == yres) && (bpp != 0) && !videomodereset)
            return 0;
    }
    else
#endif
    {
       softsurface_destroy();
    }

    // clear last gamma/contrast/brightness so that it will be set anew
    lastvidgcb[0] = lastvidgcb[1] = lastvidgcb[2] = 0.0f;

    return 1;
}

void setvideomode_sdlcommonpost(int32_t x, int32_t y, int32_t c, int32_t fs, int32_t regrab)
{
    wm_setapptitle(apptitle);

#ifdef USE_OPENGL
    if (!nogl)
        sdlayer_setvideomode_opengl();
#endif

    xres = x;
    yres = y;
    bpp = c;
    fullscreen = fs;
    // bytesperline = sdl_surface->pitch;
    numpages = c > 8 ? 2 : 1;
    frameplace = 0;
    lockcount = 0;
    modechange = 1;
    videomodereset = 0;

    // save the current system gamma to determine if gamma is available
#ifndef EDUKE32_GLES
    if (!gammabrightness)
    {
        //        float f = 1.0 + ((float)curbrightness / 10.0);
#if SDL_MAJOR_VERSION != 1
        if (SDL_GetWindowGammaRamp(sdl_window, sysgamma[0], sysgamma[1], sysgamma[2]) == 0)
#else
        if (SDL_GetGammaRamp(sysgamma[0], sysgamma[1], sysgamma[2]) >= 0)
#endif
            gammabrightness = 1;

        // see if gamma really is working by trying to set the brightness
        if (gammabrightness && videoSetGamma() < 0)
            gammabrightness = 0;  // nope
    }
#endif

    videoFadePalette(palfadergb.r, palfadergb.g, palfadergb.b, palfadedelta);

    if (regrab)
        mouseGrabInput(g_mouseLockedToWindow);
}

#if SDL_MAJOR_VERSION!=1
void setrefreshrate(void)
{
    SDL_DisplayMode dispmode;
    SDL_GetCurrentDisplayMode(0, &dispmode);

    dispmode.refresh_rate = maxrefreshfreq;

    SDL_DisplayMode newmode;
    SDL_GetClosestDisplayMode(0, &dispmode, &newmode);

    if (dispmode.refresh_rate != newmode.refresh_rate)
    {
        initprintf("Refresh rate: %dHz\n", newmode.refresh_rate);
        SDL_SetWindowDisplayMode(sdl_window, &newmode);
    }

    if (!newmode.refresh_rate)
        newmode.refresh_rate = 60;

    currentVBlankInterval = 1000/newmode.refresh_rate;
}

int32_t videoSetMode(int32_t x, int32_t y, int32_t c, int32_t fs)
{
    int32_t regrab = 0, ret;

    ret = setvideomode_sdlcommon(&x, &y, c, fs, &regrab);
    if (ret != 1)
    {
        if (ret == 0)
        {
            setvideomode_sdlcommonpost(x, y, c, fs, regrab);
        }
        return ret;
    }

    // deinit
    destroy_window_resources();

    initprintf("Setting video mode %dx%d (%d-bpp %s)\n", x, y, c, ((fs & 1) ? "fullscreen" : "windowed"));

    SDL_DisplayMode desktopmode;
    SDL_GetDesktopDisplayMode(0, &desktopmode);

    int const windowedMode = (desktopmode.w == x && desktopmode.h == y) ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;

#ifdef USE_OPENGL
    if (c > 8 || !nogl)
    {
        int32_t i;
#ifdef USE_GLEXT
        int32_t multisamplecheck = (glmultisample > 0);
#else
        int32_t multisamplecheck = 0;
#endif
        if (nogl)
            return -1;

        struct glattribs
        {
            SDL_GLattr attr;
            int32_t value;
        } sdlayer_gl_attributes[] =
        {
#ifdef EDUKE32_GLES
              { SDL_GL_CONTEXT_MAJOR_VERSION, 1 },
              { SDL_GL_CONTEXT_MINOR_VERSION, 1 },
#endif
              { SDL_GL_DOUBLEBUFFER, 1 },
#ifdef USE_GLEXT
              { SDL_GL_MULTISAMPLEBUFFERS, glmultisample > 0 },
              { SDL_GL_MULTISAMPLESAMPLES, glmultisample },
#endif
              { SDL_GL_STENCIL_SIZE, 1 },
              { SDL_GL_ACCELERATED_VISUAL, 1 },
          };

        do
        {
            SDL_GL_ATTRIBUTES(i, sdlayer_gl_attributes);

            /* HACK: changing SDL GL attribs only works before surface creation,
               so we have to create a new surface in a different format first
               to force the surface we WANT to be recreated instead of reused. */
            sdl_window = SDL_CreateWindow("", windowpos ? windowx : (int)SDL_WINDOWPOS_CENTERED,
                                          windowpos ? windowy : (int)SDL_WINDOWPOS_CENTERED, x, y,
                                          SDL_WINDOW_OPENGL);

            if (sdl_window)
                sdl_context = SDL_GL_CreateContext(sdl_window);

            if (!sdl_window || !sdl_context)
            {
                initprintf("Unable to set video mode: %s failed: %s\n", sdl_window ? "SDL_GL_CreateContext" : "SDL_GL_CreateWindow",  SDL_GetError());
                destroy_window_resources();
                return -1;
            }

            gladLoadGLLoader(SDL_GL_GetProcAddress);
            if (GLVersion.major < 2)
            {
                initprintf("Your computer does not support OpenGL version 2 or greater. GL modes are unavailable.\n");
                nogl = 1;
                destroy_window_resources();
                return -1;
            }

            SDL_SetWindowFullscreen(sdl_window, ((fs & 1) ? SDL_WINDOW_FULLSCREEN : windowedMode));
            SDL_GL_SetSwapInterval(vsync_renderlayer);

            setrefreshrate();
        } while (multisamplecheck--);
    }
    else
#endif  // defined USE_OPENGL
    {
        // init
        sdl_window = SDL_CreateWindow("", windowpos ? windowx : (int)SDL_WINDOWPOS_CENTERED,
                                      windowpos ? windowy : (int)SDL_WINDOWPOS_CENTERED, x, y,
                                      0);
        if (!sdl_window)
            SDL2_VIDEO_ERR("SDL_CreateWindow");

        setrefreshrate();

        if (!sdl_surface)
        {
            sdl_surface = SDL_GetWindowSurface(sdl_window);
            if (!sdl_surface)
                SDL2_VIDEO_ERR("SDL_GetWindowSurface");
        }

        SDL_SetWindowFullscreen(sdl_window, ((fs & 1) ? SDL_WINDOW_FULLSCREEN : windowedMode));
    }

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1");
    setvideomode_sdlcommonpost(x, y, c, fs, regrab);

    return 0;
}
#endif

//
// resetvideomode() -- resets the video system
//
void videoResetMode(void)
{
    videomodereset = 1;
    modeschecked = 0;
}

//
// begindrawing() -- locks the framebuffer for drawing
//

#ifdef DEBUG_FRAME_LOCKING
uint32_t begindrawing_line[BEGINDRAWING_SIZE];
const char *begindrawing_file[BEGINDRAWING_SIZE];
void begindrawing_real(void)
#else
void videoBeginDrawing(void)
#endif
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

#ifdef USE_OPENGL
    if (!nogl)
    {
        frameplace = (intptr_t)glsurface_getBuffer();
        if (modechange)
        {
            bytesperline = xdim;
            calc_ylookup(bytesperline, ydim);
            modechange=0;
        }
        return;
    }
#endif

    frameplace = (intptr_t)softsurface_getBuffer();
    if (modechange)
    {
        bytesperline = xdim;
        calc_ylookup(bytesperline, ydim);
        modechange=0;
    }
}


//
// enddrawing() -- unlocks the framebuffer
//
void videoEndDrawing(void)
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
}

//
// showframe() -- update the display
//
#if SDL_MAJOR_VERSION != 1

#ifdef __ANDROID__
extern "C" void AndroidDrawControls();
#endif

void videoShowFrame(int32_t w)
{
    UNREFERENCED_PARAMETER(w);

#ifdef __ANDROID__
    if (mobile_halted) return;
#endif

#ifdef USE_OPENGL
    if (!nogl)
    {
        if (bpp > 8)
        {
            if (palfadedelta)
                fullscreen_tint_gl(palfadergb.r, palfadergb.g, palfadergb.b, palfadedelta);

#ifdef __ANDROID__
            AndroidDrawControls();
#endif
        }
        else
        {
            glsurface_blitBuffer();
        }

        static uint32_t lastSwapTime = 0;
        SDL_GL_SwapWindow(sdl_window);
        if (vsync)
        {
            // busy loop until we're ready to update again
            while (SDL_GetTicks()-lastSwapTime < currentVBlankInterval) {}
        }
        lastSwapTime = SDL_GetTicks();
        return;
    }
#endif

    if (offscreenrendering) return;

    if (lockcount)
    {
        printf("Frame still locked %d times when showframe() called.\n", lockcount);
        while (lockcount) videoEndDrawing();
    }

    if (SDL_MUSTLOCK(sdl_surface)) SDL_LockSurface(sdl_surface);
    softsurface_blitBuffer((uint32_t*) sdl_surface->pixels, sdl_surface->format->BitsPerPixel);
    if (SDL_MUSTLOCK(sdl_surface)) SDL_UnlockSurface(sdl_surface);

    if (SDL_UpdateWindowSurface(sdl_window))
    {
        // If a fullscreen X11 window is minimized then this may be required.
        // FIXME: What to do if this fails...
        sdl_surface = SDL_GetWindowSurface(sdl_window);
        SDL_UpdateWindowSurface(sdl_window);
    }
}
#endif
//
// setpalette() -- set palette values
//
int32_t videoUpdatePalette(int32_t start, int32_t num)
{
    UNREFERENCED_PARAMETER(start);
    UNREFERENCED_PARAMETER(num);

    if (bpp > 8)
        return 0;  // no palette in opengl

#ifdef USE_OPENGL
    if (!nogl)
        glsurface_setPalette(curpalettefaded);
    else
#endif
    {
        if (sdl_surface)
            softsurface_setPalette(curpalettefaded,
                                   sdl_surface->format->Rmask,
                                   sdl_surface->format->Gmask,
                                   sdl_surface->format->Bmask);
    }

    return 0;
}

//
// setgamma
//
int32_t videoSetGamma(void)
{
    if (novideo)
        return 0;

    int32_t i;
    uint16_t gammaTable[768];
    float gamma = max(0.1f, min(4.f, g_videoGamma));
    float contrast = max(0.1f, min(3.f, g_videoContrast));
    float bright = max(-0.8f, min(0.8f, g_videoBrightness));

    float invgamma = 1.f / gamma;
    float norm = powf(255.f, invgamma - 1.f);

    if (lastvidgcb[0] == gamma && lastvidgcb[1] == contrast && lastvidgcb[2] == bright)
        return 0;

    // This formula is taken from Doomsday

    for (i = 0; i < 256; i++)
    {
        float val = i * contrast - (contrast - 1.f) * 127.f;
        if (gamma != 1.f)
            val = powf(val, invgamma) / norm;

        val += bright * 128.f;

        gammaTable[i] = gammaTable[i + 256] = gammaTable[i + 512] = (uint16_t)max(0.f, min(65535.f, val * 256.f));
    }

#if SDL_MAJOR_VERSION == 1
    i = SDL_SetGammaRamp(&gammaTable[0], &gammaTable[256], &gammaTable[512]);
    if (i != -1)
#else
    i = INT32_MIN;

    if (sdl_window)
        i = SDL_SetWindowGammaRamp(sdl_window, &gammaTable[0], &gammaTable[256], &gammaTable[512]);

    if (i < 0)
    {
#ifndef __ANDROID__  // Don't do this check, it is really supported, TODO
/*
        if (i != INT32_MIN)
            initprintf("Unable to set gamma: SDL_SetWindowGammaRamp failed: %s\n", SDL_GetError());
*/
#endif

#ifndef EDUKE32_GLES
#if SDL_MAJOR_VERSION == 1
        SDL_SetGammaRamp(&sysgamma[0][0], &sysgamma[1][0], &sysgamma[2][0]);
#else
        if (sdl_window)
            SDL_SetWindowGammaRamp(sdl_window, &sysgamma[0][0], &sysgamma[1][0], &sysgamma[2][0]);
#endif
        gammabrightness = 0;
#endif
    }
    else
#endif
    {
        lastvidgcb[0] = gamma;
        lastvidgcb[1] = contrast;
        lastvidgcb[2] = bright;

        gammabrightness = 1;
    }

    return i;
}

#if !defined __APPLE__ && !defined EDUKE32_TOUCH_DEVICES
extern "C" struct sdlappicon sdlappicon;
static inline SDL_Surface *loadappicon(void)
{
    SDL_Surface *surf = SDL_CreateRGBSurfaceFrom((void *)sdlappicon.pixels, sdlappicon.width, sdlappicon.height, 32,
                                                 sdlappicon.width * 4, 0xffl, 0xff00l, 0xff0000l, 0xff000000l);
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
    SDL_PumpEvents();

#if SDL_MAJOR_VERSION==1
    return SDL_PeepEvents(NULL, 1, SDL_PEEKEVENT, SDL_EVENTMASK(SDL_KEYDOWN));
#else
    return SDL_PeepEvents(NULL, 1, SDL_PEEKEVENT, SDL_KEYDOWN, SDL_KEYDOWN);
#endif
}

void handleevents_updatemousestate(uint8_t state)
{
    g_mouseClickState = state == SDL_RELEASED ? MOUSE_RELEASED : MOUSE_PRESSED;
}


//
// handleevents() -- process the SDL message queue
//   returns !0 if there was an important event worth checking (like quitting)
//

int32_t handleevents_sdlcommon(SDL_Event *ev)
{
    switch (ev->type)
    {
#if !defined EDUKE32_IOS
        case SDL_MOUSEMOTION:
#ifndef GEKKO
            g_mouseAbs.x = ev->motion.x;
            g_mouseAbs.y = ev->motion.y;
#endif
            // SDL <VER> doesn't handle relative mouse movement correctly yet as the cursor still clips to the
            // screen edges
            // so, we call SDL_WarpMouse() to center the cursor and ignore the resulting motion event that occurs
            //  <VER> is 1.3 for PK, 1.2 for tueidj
            if (appactive && g_mouseGrabbed)
            {
# if SDL_MAJOR_VERSION==1
                if (ev->motion.x != xdim >> 1 || ev->motion.y != ydim >> 1)
# endif
                {
                    g_mousePos.x += ev->motion.xrel;
                    g_mousePos.y += ev->motion.yrel;
# if SDL_MAJOR_VERSION==1
                    SDL_WarpMouse(xdim>>1, ydim>>1);
# endif
                }
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        {
            int32_t j;

            // some of these get reordered to match winlayer
            switch (ev->button.button)
            {
                default: j = -1; break;
                case SDL_BUTTON_LEFT: j = 0; handleevents_updatemousestate(ev->button.state); break;
                case SDL_BUTTON_RIGHT: j = 1; break;
                case SDL_BUTTON_MIDDLE: j = 2; break;

#if SDL_MAJOR_VERSION == 1
                case SDL_BUTTON_WHEELUP:    // 4
                case SDL_BUTTON_WHEELDOWN:  // 5
                    j = ev->button.button;
                    break;
#endif
                /* Thumb buttons. */
#if SDL_MAJOR_VERSION==1 || !defined _WIN32
                // NOTE: SDL1 does have SDL_BUTTON_X1, but that's not what is
                // generated. Neither with SDL2 on Linux. (Other OSs: not tested.)
                case 8: j = 3; break;
                case 9: j = 6; break;
#else
                // On SDL2/Windows, everything is as it should be.
                case SDL_BUTTON_X1: j = 3; break;
                case SDL_BUTTON_X2: j = 6; break;
#endif
            }

            if (j < 0)
                break;

            if (ev->button.state == SDL_PRESSED)
                g_mouseBits |= (1 << j);
            else
#if SDL_MAJOR_VERSION==1
                if (j != SDL_BUTTON_WHEELUP && j != SDL_BUTTON_WHEELDOWN)
#endif
                g_mouseBits &= ~(1 << j);

            if (g_mouseCallback)
                g_mouseCallback(j+1, ev->button.state == SDL_PRESSED);
            break;
        }
#else
# if SDL_MAJOR_VERSION != 1
        case SDL_FINGERUP:
            g_mouseClickState = MOUSE_RELEASED;
            break;
        case SDL_FINGERDOWN:
            g_mouseClickState = MOUSE_PRESSED;
        case SDL_FINGERMOTION:
            g_mouseAbs.x = Blrintf(ev->tfinger.x * xdim);
            g_mouseAbs.y = Blrintf(ev->tfinger.y * ydim);
            break;
# endif
#endif

        case SDL_JOYAXISMOTION:
            if (appactive && ev->jaxis.axis < joystick.numAxes)
            {
                joystick.pAxis[ev->jaxis.axis] = ev->jaxis.value * 10000 / 32767;
                if ((joystick.pAxis[ev->jaxis.axis] < joydead[ev->jaxis.axis]) &&
                    (joystick.pAxis[ev->jaxis.axis] > -joydead[ev->jaxis.axis]))
                    joystick.pAxis[ev->jaxis.axis] = 0;
                else if (joystick.pAxis[ev->jaxis.axis] >= joysatur[ev->jaxis.axis])
                    joystick.pAxis[ev->jaxis.axis] = 10000;
                else if (joystick.pAxis[ev->jaxis.axis] <= -joysatur[ev->jaxis.axis])
                    joystick.pAxis[ev->jaxis.axis] = -10000;
                else
                    joystick.pAxis[ev->jaxis.axis] = joystick.pAxis[ev->jaxis.axis] * 10000 / joysatur[ev->jaxis.axis];
            }
            break;

        case SDL_JOYHATMOTION:
        {
            int32_t hatvals[16] = {
                -1,     // centre
                0,      // up 1
                9000,   // right 2
                4500,   // up+right 3
                18000,  // down 4
                -1,     // down+up!! 5
                13500,  // down+right 6
                -1,     // down+right+up!! 7
                27000,  // left 8
                27500,  // left+up 9
                -1,     // left+right!! 10
                -1,     // left+right+up!! 11
                22500,  // left+down 12
                -1,     // left+down+up!! 13
                -1,     // left+down+right!! 14
                -1,     // left+down+right+up!! 15
            };
            if (appactive && ev->jhat.hat < joystick.numHats)
                joystick.pHat[ev->jhat.hat] = hatvals[ev->jhat.value & 15];
            break;
        }

        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
            if (appactive && ev->jbutton.button < joystick.numButtons)
            {
                if (ev->jbutton.state == SDL_PRESSED)
                    joystick.bits |= 1 << ev->jbutton.button;
                else
                    joystick.bits &= ~(1 << ev->jbutton.button);

#ifdef GEKKO
                if (ev->jbutton.button == 0) // WII_A
                    handleevents_updatemousestate(ev->jbutton.state);
#endif
            }
            break;

        case SDL_QUIT:
            quitevent = 1;
            return -1;
    }

    return 0;
}

int32_t handleevents_pollsdl(void);
#if SDL_MAJOR_VERSION != 1
// SDL 2.0 specific event handling
int32_t handleevents_pollsdl(void)
{
    int32_t code, rv=0, j;
    SDL_Event ev;

    while (SDL_PollEvent(&ev))
    {
        switch (ev.type)
        {
            case SDL_TEXTINPUT:
                j = 0;
                do
                {
                    code = ev.text.text[j];

                    if (code != g_keyAsciiTable[OSD_OSDKey()] && !keyBufferFull())
                    {
                        if (OSD_HandleChar(code))
                            keyBufferInsert(code);
                    }
                } while (j < SDL_TEXTINPUTEVENT_TEXT_SIZE-1 && ev.text.text[++j]);
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                auto const &sc = ev.key.keysym.scancode;
                code = keytranslation[sc];

                // Modifiers that have to be held down to be effective
                // (excludes KMOD_NUM, for example).
                static const int MODIFIERS =
                    KMOD_LSHIFT|KMOD_RSHIFT|KMOD_LCTRL|KMOD_RCTRL|
                    KMOD_LALT|KMOD_RALT|KMOD_LGUI|KMOD_RGUI;

                // XXX: see osd.c, OSD_HandleChar(), there are more...
                if (ev.key.type == SDL_KEYDOWN && !keyBufferFull() &&
                    (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_KP_ENTER ||
                     sc == SDL_SCANCODE_ESCAPE ||
                     sc == SDL_SCANCODE_BACKSPACE ||
                     sc == SDL_SCANCODE_TAB ||
                     (((ev.key.keysym.mod) & MODIFIERS) == KMOD_LCTRL &&
                      (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z))))
                {
                    char keyvalue;
                    switch (sc)
                    {
                        case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: keyvalue = '\r'; break;
                        case SDL_SCANCODE_ESCAPE: keyvalue = 27; break;
                        case SDL_SCANCODE_BACKSPACE: keyvalue = '\b'; break;
                        case SDL_SCANCODE_TAB: keyvalue = '\t'; break;
                        default: keyvalue = sc - SDL_SCANCODE_A + 1; break;  // Ctrl+A --> 1, etc.
                    }
                    if (OSD_HandleChar(keyvalue))
                        keyBufferInsert(keyvalue);
                }
                else if (ev.key.type == SDL_KEYDOWN &&
                         ev.key.keysym.sym != g_keyAsciiTable[OSD_OSDKey()] && !keyBufferFull() &&
                         !SDL_IsTextInputActive())
                {
                    /*
                    Necessary for Duke 3D's method of entering cheats to work without showing IMEs.
                    SDL_TEXTINPUT is preferable overall, but with bitmap fonts it has no advantage.
                    */
                    SDL_Keycode keyvalue = ev.key.keysym.sym;

                    if ('a' <= keyvalue && keyvalue <= 'z')
                    {
                        if (!!(ev.key.keysym.mod & KMOD_SHIFT) ^ !!(ev.key.keysym.mod & KMOD_CAPS))
                            keyvalue -= 'a'-'A';
                    }
                    else if (ev.key.keysym.mod & KMOD_SHIFT)
                    {
                        switch (keyvalue)
                        {
                            case '\'': keyvalue = '"'; break;

                            case ',': keyvalue = '<'; break;
                            case '-': keyvalue = '_'; break;
                            case '.': keyvalue = '>'; break;
                            case '/': keyvalue = '?'; break;
                            case '0': keyvalue = ')'; break;
                            case '1': keyvalue = '!'; break;
                            case '2': keyvalue = '@'; break;
                            case '3': keyvalue = '#'; break;
                            case '4': keyvalue = '$'; break;
                            case '5': keyvalue = '%'; break;
                            case '6': keyvalue = '^'; break;
                            case '7': keyvalue = '&'; break;
                            case '8': keyvalue = '*'; break;
                            case '9': keyvalue = '('; break;

                            case ';': keyvalue = ':'; break;

                            case '=': keyvalue = '+'; break;

                            case '[': keyvalue = '{'; break;
                            case '\\': keyvalue = '|'; break;
                            case ']': keyvalue = '}'; break;

                            case '`': keyvalue = '~'; break;
                        }
                    }
                    else if (ev.key.keysym.mod & KMOD_NUM) // && !(ev.key.keysym.mod & KMOD_SHIFT)
                    {
                        switch (keyvalue)
                        {
                            case SDLK_KP_1: keyvalue = '1'; break;
                            case SDLK_KP_2: keyvalue = '2'; break;
                            case SDLK_KP_3: keyvalue = '3'; break;
                            case SDLK_KP_4: keyvalue = '4'; break;
                            case SDLK_KP_5: keyvalue = '5'; break;
                            case SDLK_KP_6: keyvalue = '6'; break;
                            case SDLK_KP_7: keyvalue = '7'; break;
                            case SDLK_KP_8: keyvalue = '8'; break;
                            case SDLK_KP_9: keyvalue = '9'; break;
                            case SDLK_KP_0: keyvalue = '0'; break;
                            case SDLK_KP_PERIOD: keyvalue = '.'; break;
                            case SDLK_KP_COMMA: keyvalue = ','; break;
                        }
                    }

                    switch (keyvalue)
                    {
                        case SDLK_KP_DIVIDE: keyvalue = '/'; break;
                        case SDLK_KP_MULTIPLY: keyvalue = '*'; break;
                        case SDLK_KP_MINUS: keyvalue = '-'; break;
                        case SDLK_KP_PLUS: keyvalue = '+'; break;
                    }

                    if ((unsigned)keyvalue <= 0x7Fu)
                    {
                        if (OSD_HandleChar(keyvalue))
                            keyBufferInsert(keyvalue);
                    }
                }

                // initprintf("SDL2: got key %d, %d, %u\n", ev.key.keysym.scancode, code, ev.key.type);

                // hook in the osd
                if ((j = OSD_HandleScanCode(code, (ev.key.type == SDL_KEYDOWN))) <= 0)
                {
                    if (j == -1)  // osdkey
                    {
                        for (j = 0; j < NUMKEYS; ++j)
                        {
                            if (keyGetState(j))
                            {
                                keySetState(j, 0);
                                if (keypresscallback)
                                    keypresscallback(j, 0);
                            }
                        }
                    }
                    break;
                }

                if (ev.key.type == SDL_KEYDOWN)
                {
                    if (!keyGetState(code))
                    {
                        keySetState(code, 1);
                        if (keypresscallback)
                            keypresscallback(code, 1);
                    }
                }
                else
                {
# if 1
                    // The pause key generates a release event right after
                    // the pressing one. As a result, it gets unseen
                    // by the game most of the time.
                    if (code == 0x59)  // pause
                        break;
# endif
                    keySetState(code, 0);
                    if (keypresscallback)
                        keypresscallback(code, 0);
                }
                break;
            }

            case SDL_MOUSEWHEEL:
                // initprintf("wheel y %d\n",ev.wheel.y);
                if (ev.wheel.y > 0)
                {
                    g_mouseBits |= 16;
                    if (g_mouseCallback)
                        g_mouseCallback(5, 1);
                }
                if (ev.wheel.y < 0)
                {
                    g_mouseBits |= 32;
                    if (g_mouseCallback)
                        g_mouseCallback(6, 1);
                }
                break;

            case SDL_WINDOWEVENT:
                switch (ev.window.event)
                {
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        appactive = (ev.window.event == SDL_WINDOWEVENT_FOCUS_GAINED);
                        if (g_mouseGrabbed && g_mouseEnabled)
                            grabmouse_low(appactive);
#ifdef _WIN32
                        // Win_SetKeyboardLayoutUS(appactive);

                        if (backgroundidle)
                            SetPriorityClass(GetCurrentProcess(), appactive ? NORMAL_PRIORITY_CLASS : IDLE_PRIORITY_CLASS);
#endif
                        break;

                    case SDL_WINDOWEVENT_MOVED:
                        if (windowpos)
                        {
                            windowx = ev.window.data1;
                            windowy = ev.window.data2;
                        }
                        break;
                    case SDL_WINDOWEVENT_ENTER:
                        g_mouseInsideWindow = 1;
                        break;
                    case SDL_WINDOWEVENT_LEAVE:
                        g_mouseInsideWindow = 0;
                        break;
                }
                break;

            default:
                rv = handleevents_sdlcommon(&ev);
                break;
        }
    }

    return rv;
}
#endif

int32_t handleevents(void)
{
#ifdef __ANDROID__
    if (mobile_halted) return 0;
#endif

    int32_t rv;

    if (inputchecked && g_mouseEnabled)
    {
        if (g_mouseCallback)
        {
            if (g_mouseBits & 16)
                g_mouseCallback(5, 0);
            if (g_mouseBits & 32)
                g_mouseCallback(6, 0);
        }
        g_mouseBits &= ~(16 | 32);
    }

    rv = handleevents_pollsdl();

    inputchecked = 0;
    timerUpdate();

#ifndef _WIN32
    startwin_idle(NULL);
#endif

    return rv;
}

#if SDL_MAJOR_VERSION == 1
#include "sdlayer12.cpp"
#endif
