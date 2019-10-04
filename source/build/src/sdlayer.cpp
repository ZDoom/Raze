// SDL interface layer for the Build Engine
// Use SDL 1.2 or 2.0 from http://www.libsdl.org

#include <Windows.h>
#include <CommCtrl.h>
#include <signal.h>
#include <string>

#include "a.h"
#include "build.h"
#include "cache1d.h"
#include "common.h"
#include "compat.h"
#include "engine_priv.h"
#include "osd.h"
#include "palette.h"
#include "renderlayer.h"
#include "sdl_inc.h"
#include "softsurface.h"
#include "m_argv.h"
#include "mmulti.h"
#include "scriptfile.h"
#include "zstring.h"
#include "../../glbackend/glbackend.h"

#ifdef USE_OPENGL
# include "glad/glad.h"
# include "glbuild.h"
# include "glsurface.h"
#endif

#if defined HAVE_GTK2
# include "gtkbits.h"
#endif

#if defined __APPLE__
# include "osxbits.h"
# include <mach/mach.h>
# include <mach/mach_time.h>
#elif defined _WIN32
# include "win32/winbits.h"
#endif

#include "vfs.h"

#if SDL_MAJOR_VERSION != 1
static SDL_version linked;
#endif

GameInterface* gi;
FArgs* Args;

#if !defined STARTUP_SETUP_WINDOW
int32_t startwin_open(void) { return 0; }
int32_t startwin_close(void) { return 0; }
int32_t startwin_puts(const char *s) { UNREFERENCED_PARAMETER(s); return 0; }
int32_t startwin_idle(void *s) { UNREFERENCED_PARAMETER(s); return 0; }
int32_t startwin_settitle(const char *s) { UNREFERENCED_PARAMETER(s); return 0; }
int32_t startwin_run(void) { return 0; }
#else
int32_t startwin_open(void) { return gi->startwin_open(); }
int32_t startwin_close(void) { return gi->startwin_close(); }
int32_t startwin_puts(const char* s) { return gi->startwin_puts(s); }
int32_t startwin_idle(void* s) { return gi->startwin_idle(s); }
int32_t startwin_settitle(const char* s) { return gi->startwin_settitle(s); }
int32_t startwin_run(void) { return gi->startwin_run(); }
#endif

int myconnectindex, numplayers;
int connecthead, connectpoint2[MAXMULTIPLAYERS];
unsigned char syncstate;

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

int32_t xres=-1, yres=-1, bpp=0, fullscreen=0, bytesperline, refreshfreq=-1;
intptr_t frameplace=0;
int32_t lockcount=0;
char modechange=1;
char offscreenrendering=0;
char videomodereset = 0;
int32_t nofog=0;
static uint16_t sysgamma[3][256];
#ifdef USE_OPENGL
// OpenGL stuff
char nogl=0;
#endif
static int32_t vsync_renderlayer;
int32_t maxrefreshfreq=0;

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
uint16_t joydead[9], joysatur[9];

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
    MessageBoxA(win_gethwnd(),buf,name,MB_OK|MB_TASKMODAL);
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
    return (MessageBoxA(win_gethwnd(),buf,name,MB_YESNO|MB_ICONQUESTION|MB_TASKMODAL) == IDYES);
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
        gi->app_crashhandler();
        uninitsystem();
        Bexit(8);
    }
}
#endif


#ifdef _WIN32

namespace Duke
{
	extern GameInterface Interface;
}
namespace Redneck
{
	extern GameInterface Interface;
}
namespace Blood
{
	extern GameInterface Interface;
}

GameInterface *CheckFrontend()
{
	FILE* f = fopen("blood.rff", "rb");
	if (f)
	{
		fclose(f);
		return &Blood::Interface;
	}
	else
	{
		f = fopen("redneck.grp", "rb");
		if (f)
		{
			fclose(f);
			return &Redneck::Interface;
		}
		else
			return &Duke::Interface;
	}
}

void ChooseGame()
{
	auto dir = Args->CheckValue("-game");
	if (dir && !chdir(dir))
	{
		gi = CheckFrontend();
		return;
	}

	TArray<FString> paths;
	std::vector<std::wstring> wgames;
	TArray<TASKDIALOG_BUTTON> buttons;
	char* token;
	auto script = scriptfile_fromfile("./games.list");
	int id = 1000;
	while (!scriptfile_eof(script))
	{
		scriptfile_getstring(script, &token);
		if (scriptfile_eof(script))
		{
			break;
		}
		FString game = token;
		scriptfile_getstring(script, &token);
		paths.Push(token);
		FStringf display("%s\n%s", game.GetChars(), token);
		wgames.push_back(display.WideString());
		buttons.Push({ id++, wgames.back().c_str() });
	}
	if (paths.Size() == 0)
	{
		exit(1);
	}

	int nResult = 0;

	TASKDIALOGCONFIG stTaskConfig;
	ZeroMemory(&stTaskConfig, sizeof(stTaskConfig));

	stTaskConfig.cbSize = sizeof(TASKDIALOGCONFIG);
	stTaskConfig.hwndParent = NULL;
	stTaskConfig.hInstance = NULL;

	stTaskConfig.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION| TDF_USE_COMMAND_LINKS;

	if (!gi)
	{
		// Open a popup to select the game.
		// The entire startup code just doesn't work right if this isn't checked as the very first thing.
		stTaskConfig.pszWindowTitle = L"Demolition";
		stTaskConfig.pszMainInstruction = L"Choose your game";
		stTaskConfig.pszContent = L"";
		stTaskConfig.cButtons = buttons.Size();

		stTaskConfig.pButtons = buttons.Data();
		stTaskConfig.nDefaultButton = 1000;

		if (SUCCEEDED(TaskDialogIndirect(&stTaskConfig, &nResult, NULL, NULL)))
		{
			nResult -= 1000;
			chdir(paths[nResult]);
			gi = CheckFrontend();
		}
		if (gi == nullptr) exit(1);
	}
}

int WINAPI WinMain(HINSTANCE , HINSTANCE , LPSTR , int )
#else
int main(int argc, char *argv[])
#endif
{

#ifdef _WIN32
	char* argvbuf;
	int32_t buildargc =  win_buildargs(&argvbuf);
	const char** buildargv = (const char**)Xmalloc(sizeof(char*) * (buildargc + 1));
	char* wp = argvbuf;

	for (bssize_t i = 0; i < buildargc; i++, wp++)
	{
		buildargv[i] = wp;
		while (*wp) wp++;
	}
	buildargv[buildargc] = NULL;
#else
	auto buildargc = argc;
	auto buildargv = argv;
#endif

	Args = new FArgs(buildargc, buildargv);

	ChooseGame();

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

#ifndef _WIN32	// catching signals is not really helpful on Windows.
    signal(SIGSEGV, sighandler);
    signal(SIGILL, sighandler);  /* clang -fcatch-undefined-behavior uses an ill. insn */
    signal(SIGABRT, sighandler);
    signal(SIGFPE, sighandler);
#endif


#if defined(HAVE_GTK2)
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

    r = gi->app_main(buildargc, (const char **)buildargv);

    startwin_close();

#if defined(HAVE_GTK2)
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
		/*
        vsync_renderlayer = newSync;

        videoResetMode();
        if (videoSetGameMode(fullscreen, xres, yres, bpp, upscalefactor))
            OSD_Printf("restartvid: Reset failed...\n");
		*/
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

#ifdef USE_OPENGL
# if SDL_MAJOR_VERSION!=1
    SDL_GL_UnloadLibrary();
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
#if SDL_MAJOR_VERSION >= 2
static SDL_GameController *controller = NULL;

static void LoadSDLControllerDB()
{
    buildvfs_kfd fh = kopen4load("gamecontrollerdb.txt", 0);
    if (fh == buildvfs_kfd_invalid)
        return;

    int flen = kfilelength(fh);
    if (flen <= 0)
    {
        kclose(fh);
        return;
    }

    char * dbuf = (char *)malloc(flen + 1);
    if (!dbuf)
    {
        kclose(fh);
        return;
    }

    if (kread_and_test(fh, dbuf, flen))
    {
        free(dbuf);
        kclose(fh);
        return;
    }

    dbuf[flen] = '\0';
    kclose(fh);

    SDL_RWops * rwops = SDL_RWFromConstMem(dbuf, flen);
    if (!rwops)
    {
        free(dbuf);
        return;
    }

    int i = SDL_GameControllerAddMappingsFromRW(rwops, 0);
    if (i == -1)
        buildprintf("Failed loading game controller database: %s\n", SDL_GetError());
    else
        buildputs("Loaded game controller database\n");

    SDL_free(rwops);
    free(dbuf);
}
#endif

void joyScanDevices()
{
    inputdevices &= ~4;

    if (controller)
    {
        SDL_GameControllerClose(controller);
        controller = nullptr;
    }
    if (joydev)
    {
        SDL_JoystickClose(joydev);
        joydev = nullptr;
    }

    int numjoysticks = SDL_NumJoysticks();
    if (numjoysticks < 1)
    {
        buildputs("No game controllers found\n");
    }
    else
    {
        buildputs("Game controllers:\n");
        for (int i = 0; i < numjoysticks; i++)
        {
            const char * name;
#if SDL_MAJOR_VERSION >= 2
            if (SDL_IsGameController(i))
                name = SDL_GameControllerNameForIndex(i);
            else
#endif
                name = SDL_JoystickNameForIndex(i);

            buildprintf("  %d. %s\n", i+1, name);
        }

#if SDL_MAJOR_VERSION >= 2
        for (int i = 0; i < numjoysticks; i++)
        {
            if ((controller = SDL_GameControllerOpen(i)))
            {
                buildprintf("Using controller %s\n", SDL_GameControllerName(controller));

                joystick.numAxes    = SDL_CONTROLLER_AXIS_MAX;
                joystick.numButtons = SDL_CONTROLLER_BUTTON_MAX;
                joystick.numHats    = 0;
                joystick.isGameController = 1;

                Xfree(joystick.pAxis);
                joystick.pAxis = (int32_t *)Xcalloc(joystick.numAxes, sizeof(int32_t));
                Xfree(joystick.pHat);
                joystick.pHat = nullptr;

                inputdevices |= 4;

                return;
            }
        }
#endif

        for (int i = 0; i < numjoysticks; i++)
        {
            if ((joydev = SDL_JoystickOpen(i)))
            {
                buildprintf("Using joystick %s\n", SDL_JoystickName(joydev));

                // KEEPINSYNC duke3d/src/gamedefs.h, mact/include/_control.h
                joystick.numAxes = min(9, SDL_JoystickNumAxes(joydev));
                joystick.numButtons = min(32, SDL_JoystickNumButtons(joydev));
                joystick.numHats = min((36-joystick.numButtons)/4,SDL_JoystickNumHats(joydev));
                joystick.isGameController = 0;

                initprintf("Joystick %d has %d axes, %d buttons, and %d hat(s).\n", i+1, joystick.numAxes, joystick.numButtons, joystick.numHats);

                Xfree(joystick.pAxis);
                joystick.pAxis = (int32_t *)Xcalloc(joystick.numAxes, sizeof(int32_t));

                Xfree(joystick.pHat);
                if (joystick.numHats)
                    joystick.pHat = (int32_t *)Xcalloc(joystick.numHats, sizeof(int32_t));
                else
                    joystick.pHat = nullptr;

                for (int j = 0; j < joystick.numHats; j++)
                    joystick.pHat[j] = -1; // center

                SDL_JoystickEventState(SDL_ENABLE);
                inputdevices |= 4;

                return;
            }
        }

        buildputs("No controllers are usable\n");
    }
}

//
// initinput() -- init input system
//
int32_t initinput(void)
{
    int32_t i;


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

    for (i = SDL_NUM_SCANCODES - 1; i >= 0; i--)
    {
        if (!keytranslation[i])
            continue;

        Bstrncpyz(g_keyNameTable[keytranslation[i]], SDL_GetKeyName(SDL_SCANCODE_TO_KEYCODE(i)), sizeof(g_keyNameTable[0]));
    }

    if (!SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER))
    {
        LoadSDLControllerDB();
        joyScanDevices();
    }

    return 0;
}

//
// uninitinput() -- uninit input system
//
void uninitinput(void)
{
    mouseUninit();

    if (controller)
    {
        SDL_GameControllerClose(controller);
        controller = NULL;
    }

    if (joydev)
    {
        SDL_JoystickClose(joydev);
        joydev = NULL;
    }
}

const char *joyGetName(int32_t what, int32_t num)
{
    static char tmp[64];

    switch (what)
    {
        case 0:  // axis
            if ((unsigned)num > (unsigned)joystick.numAxes)
                return NULL;

            if (controller)
            {
                static char const * axisStrings[] =
                {
                    "Left Stick X-Axis",
                    "Left Stick Y-Axis",
                    "Right Stick X-Axis",
                    "Right Stick Y-Axis",
                    "Left Trigger",
                    "Right Trigger",
                    NULL
                };
                return axisStrings[num];
            }

            Bsprintf(tmp, "Axis %d", num);
            return (char *)tmp;

        case 1:  // button
            if ((unsigned)num > (unsigned)joystick.numButtons)
                return NULL;

            if (controller)
            {
                static char const * buttonStrings[] =
                {
                    "A",
                    "B",
                    "X",
                    "Y",
                    "Back",
                    "Guide",
                    "Start",
                    "Left Stick",
                    "Right Stick",
                    "Left Shoulder",
                    "Right Shoulder",
                    "D-Pad Up",
                    "D-Pad Down",
                    "D-Pad Left",
                    "D-Pad Right",
                    NULL
                };
                return buttonStrings[num];
            }

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


//
// grabmouse_low() -- show/hide mouse cursor, lower level (doesn't check state).
//                    furthermore return 0 if successful.
//

static inline char grabmouse_low(char a)
{
    /* FIXME: Maybe it's better to make sure that grabmouse_low
       is called only when a window is ready?                */
    if (sdl_window)
        SDL_SetWindowGrab(sdl_window, a ? SDL_TRUE : SDL_FALSE);
    return SDL_SetRelativeMouseMode(a ? SDL_TRUE : SDL_FALSE);
}

//
// grabmouse() -- show/hide mouse cursor
//
void mouseGrabInput(bool grab)
{
    if (appactive && g_mouseEnabled)
    {
        if ((grab != g_mouseGrabbed) && !grabmouse_low(grab))
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

void mouseMoveToCenter(void)
{
    if (sdl_window)
    {
        g_mouseAbs = { xdim >> 1, ydim >> 1 };
        SDL_WarpMouseInWindow(sdl_window, g_mouseAbs.x, g_mouseAbs.y);
    }
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
    auto a = (const struct validmode_t *)b_;
    auto b = (const struct validmode_t *)a_;

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
    int const display = r_displayindex < SDL_GetNumVideoDisplays() ? r_displayindex : 0;

    if (modeschecked || novideo)
        return;

    validmodecnt = 0;
    //    initprintf("Detecting video modes:\n");

    // do fullscreen modes first
    for (i = 0; i < SDL_GetNumDisplayModes(display); i++)
    {
        SDL_GetDisplayMode(display, i, &dispmode);

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
    SDL_GetDisplayMode(display, 0, &dispmode);

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

void sdlayer_setvideomode_opengl(void)
{
    glsurface_destroy();
    polymost_glreset();

	GLInterface.Deinit();
	GLInterface.Init();
	GLInterface.InitGLState(r_usenewshading, glmultisample);
	GLInterface.mSamplers->SetTextureFilterMode(gltexfiltermode, glanisotropy);

}

//
// setvideomode() -- set SDL video mode
//

int32_t setvideomode_sdlcommon(int32_t *x, int32_t *y, int32_t c, int32_t fs, int32_t *regrab)
{
    if ((fs == fullscreen) && (*x == xres) && (*y == yres) && (c == bpp) && !videomodereset)
        return 0;

    if (videoCheckMode(x, y, c, fs, 0) < 0)
        return -1;

    startwin_close();

    if (g_mouseGrabbed)
    {
        *regrab = 1;
        mouseGrabInput(0);
    }

    while (lockcount) videoEndDrawing();

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
    if (!gammabrightness)
    {
        //        float f = 1.0 + ((float)curbrightness / 10.0);
        if (SDL_GetWindowGammaRamp(sdl_window, sysgamma[0], sysgamma[1], sysgamma[2]) == 0)
            gammabrightness = 1;

        // see if gamma really is working by trying to set the brightness
        if (gammabrightness && videoSetGamma() < 0)
            gammabrightness = 0;  // nope
    }

    videoFadePalette(palfadergb.r, palfadergb.g, palfadergb.b, palfadedelta);

    if (regrab)
        mouseGrabInput(g_mouseLockedToWindow);
}

#if SDL_MAJOR_VERSION!=1
void setrefreshrate(void)
{
    int const display = r_displayindex < SDL_GetNumVideoDisplays() ? r_displayindex : 0;

    SDL_DisplayMode dispmode;
    SDL_GetCurrentDisplayMode(display, &dispmode);

    dispmode.refresh_rate = maxrefreshfreq;

    SDL_DisplayMode newmode;
    SDL_GetClosestDisplayMode(display, &dispmode, &newmode);

    char error = 0;

    if (dispmode.refresh_rate != newmode.refresh_rate)
    {
        initprintf("Refresh rate: %dHz\n", newmode.refresh_rate);
        error = SDL_SetWindowDisplayMode(sdl_window, &newmode);
    }

    if (!newmode.refresh_rate)
        newmode.refresh_rate = 60;

    refreshfreq = error ? -1 : newmode.refresh_rate;
}

int called = 0;
int32_t videoSetMode(int32_t x, int32_t y, int32_t c, int32_t fs)
{
    int32_t regrab = 0, ret;

	if (called++)
	{
		assert(0);
	}
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

    int const display = r_displayindex < SDL_GetNumVideoDisplays() ? r_displayindex : 0;

    SDL_DisplayMode desktopmode;
    SDL_GetDesktopDisplayMode(display, &desktopmode);

    int const matchedResolution = (desktopmode.w == x && desktopmode.h == y);
    int const borderless = (r_borderless == 1 || (r_borderless == 2 && matchedResolution)) ? SDL_WINDOW_BORDERLESS : 0;
#ifdef USE_OPENGL
    if (c > 8 || !nogl)
    {
        int32_t i;
        int32_t multisamplecheck = (glmultisample > 0);
        if (nogl)
            return -1;

        struct glattribs
        {
            SDL_GLattr attr;
            int32_t value;
        } sdlayer_gl_attributes[] =
        {
              { SDL_GL_DOUBLEBUFFER, 1 },
              { SDL_GL_MULTISAMPLEBUFFERS, glmultisample > 0 },
              { SDL_GL_MULTISAMPLESAMPLES, glmultisample },
              { SDL_GL_STENCIL_SIZE, 1 },
              { SDL_GL_ACCELERATED_VISUAL, 1 },
          };

        do
        {
            SDL_GL_ATTRIBUTES(i, sdlayer_gl_attributes);

            /* HACK: changing SDL GL attribs only works before surface creation,
               so we have to create a new surface in a different format first
               to force the surface we WANT to be recreated instead of reused. */


            sdl_window = SDL_CreateWindow("", windowpos ? windowx : (int)SDL_WINDOWPOS_CENTERED_DISPLAY(display),
                                          windowpos ? windowy : (int)SDL_WINDOWPOS_CENTERED_DISPLAY(display), x, y,
                                          SDL_WINDOW_OPENGL | borderless);

            if (sdl_window)
                sdl_context = SDL_GL_CreateContext(sdl_window);

            if (!sdl_window || !sdl_context)
            {
                initprintf("Unable to set video mode: %s failed: %s\n", sdl_window ? "SDL_GL_CreateContext" : "SDL_GL_CreateWindow",  SDL_GetError());
                nogl = 1;
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

            SDL_SetWindowFullscreen(sdl_window, ((fs & 1) ? (matchedResolution ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN) : 0));
            SDL_GL_SetSwapInterval(vsync_renderlayer);

            setrefreshrate();
        } while (multisamplecheck--);
    }
    else
#endif  // defined USE_OPENGL
    {
        // init
        sdl_window = SDL_CreateWindow("", windowpos ? windowx : (int)SDL_WINDOWPOS_CENTERED_DISPLAY(display),
                                      windowpos ? windowy : (int)SDL_WINDOWPOS_CENTERED_DISPLAY(display), x, y,
                                      borderless);
        if (!sdl_window)
            SDL2_VIDEO_ERR("SDL_CreateWindow");

        setrefreshrate();

        if (!sdl_surface)
        {
            sdl_surface = SDL_GetWindowSurface(sdl_window);
            if (!sdl_surface)
                SDL2_VIDEO_ERR("SDL_GetWindowSurface");
        }

        SDL_SetWindowFullscreen(sdl_window, ((fs & 1) ? (matchedResolution ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN) : 0));
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

void videoBeginDrawing(void)
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

    static intptr_t backupFrameplace = 0;

    if (inpreparemirror)
    {
        //POGO: if we are offscreenrendering and we need to render a mirror
        //      or we are rendering a mirror and we start offscreenrendering,
        //      backup our offscreen target so we can restore it later
        //      (but only allow one level deep,
        //       i.e. no viewscreen showing a camera showing a mirror that reflects the same viewscreen and recursing)
        if (offscreenrendering)
        {
            if (!backupFrameplace)
                backupFrameplace = frameplace;
            else if (frameplace != (intptr_t)mirrorBuffer &&
                     frameplace != backupFrameplace)
                return;
        }

        frameplace = (intptr_t)mirrorBuffer;

        if (offscreenrendering)
            return;
    }
    else if (offscreenrendering)
    {
        if (backupFrameplace)
        {
            frameplace = backupFrameplace;
            backupFrameplace = 0;
        }
        return;
    }
    else
#ifdef USE_OPENGL
    if (!nogl)
    {
        frameplace = (intptr_t)glsurface_getBuffer();
    }
    else
#endif
    {
        frameplace = (intptr_t)softsurface_getBuffer();
    }

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

void videoShowFrame(int32_t w)
{
    UNREFERENCED_PARAMETER(w);


#ifdef USE_OPENGL
    if (!nogl)
    {
        if (bpp > 8)
        {
            if (palfadedelta)
                fullscreen_tint_gl(palfadergb.r, palfadergb.g, palfadergb.b, palfadedelta);
            if (playing_blood) 
				fullscreen_tint_gl_blood();

        }
        else
        {
            glsurface_blitBuffer();
        }

        static uint32_t lastSwapTime = 0;
#ifdef  TIMING
		cycle_t clock;
		clock.Reset();
		clock.Clock();
#endif
		glFinish();
#ifdef TIMING
		clock.Unclock();
		OSD_Printf("glfinish time: %2.3f\n", clock.TimeMS());
#endif
        SDL_GL_SwapWindow(sdl_window);

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

    i = INT32_MIN;

    if (sdl_window)
        i = SDL_SetWindowGammaRamp(sdl_window, &gammaTable[0], &gammaTable[256], &gammaTable[512]);

    if (i < 0)
    {
        OSD_Printf("videoSetGamma(): %s\n", SDL_GetError());


        if (sdl_window)
            SDL_SetWindowGammaRamp(sdl_window, &sysgamma[0][0], &sysgamma[1][0], &sysgamma[2][0]);
        gammabrightness = 0;
    }
    else
    {
        lastvidgcb[0] = gamma;
        lastvidgcb[1] = contrast;
        lastvidgcb[2] = bright;

        gammabrightness = 1;
    }

    return i;
}

#if !defined __APPLE__ && !defined EDUKE32_TOUCH_DEVICES
extern struct sdlappicon sdlappicon;
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

    return SDL_PeepEvents(NULL, 1, SDL_PEEKEVENT, SDL_KEYDOWN, SDL_KEYDOWN);
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
        case SDL_MOUSEMOTION:
            g_mouseAbs.x = ev->motion.x;
            g_mouseAbs.y = ev->motion.y;
            // SDL <VER> doesn't handle relative mouse movement correctly yet as the cursor still clips to the
            // screen edges
            // so, we call SDL_WarpMouse() to center the cursor and ignore the resulting motion event that occurs
            //  <VER> is 1.3 for PK, 1.2 for tueidj
            if (appactive && g_mouseGrabbed)
            {
                {
                    g_mousePos.x += ev->motion.xrel;
                    g_mousePos.y += ev->motion.yrel;
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

                /* Thumb buttons. */
                // On SDL2/Windows and SDL >= 2.0.?/Linux, everything is as it should be.
                // If anyone cares about old versions of SDL2 on Linux, patches welcome.
                case SDL_BUTTON_X1: j = 3; break;
                case SDL_BUTTON_X2: j = 6; break;
            }

            if (j < 0)
                break;

            if (ev->button.state == SDL_PRESSED)
                g_mouseBits |= (1 << j);
            else
            g_mouseBits &= ~(1 << j);

            if (g_mouseCallback)
                g_mouseCallback(j+1, ev->button.state == SDL_PRESSED);
            break;
        }

        case SDL_JOYAXISMOTION:
#if SDL_MAJOR_VERSION >= 2
            if (joystick.isGameController)
                break;
            fallthrough__;
        case SDL_CONTROLLERAXISMOTION:
#endif
            if (appactive && ev->jaxis.axis < joystick.numAxes)
            {
                joystick.pAxis[ev->jaxis.axis] = ev->jaxis.value;
                int32_t const scaledValue = ev->jaxis.value * 10000 / 32767;
                if ((scaledValue < joydead[ev->jaxis.axis]) &&
                    (scaledValue > -joydead[ev->jaxis.axis]))
                    joystick.pAxis[ev->jaxis.axis] = 0;
                else if (scaledValue >= joysatur[ev->jaxis.axis])
                    joystick.pAxis[ev->jaxis.axis] = 32767;
                else if (scaledValue <= -joysatur[ev->jaxis.axis])
                    joystick.pAxis[ev->jaxis.axis] = -32767;
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
#if SDL_MAJOR_VERSION >= 2
            if (joystick.isGameController)
                break;
            fallthrough__;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
#endif
            if (appactive && ev->jbutton.button < joystick.numButtons)
            {
                if (ev->jbutton.state == SDL_PRESSED)
                    joystick.bits |= 1 << ev->jbutton.button;
                else
                    joystick.bits &= ~(1 << ev->jbutton.button);

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
                                if (keypresscallback)
                                    keypresscallback(j, 0);
                            }
                            keySetState(j, 0);
                        }
                    }
                    break;
                }

                if (ev.key.type == SDL_KEYDOWN)
                {
                    if (!keyGetState(code))
                    {
                        if (keypresscallback)
                            keypresscallback(code, 1);
                    }
                    keySetState(code, 1);
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
                        break;

                    case SDL_WINDOWEVENT_MOVED:
                    {
                        if (windowpos)
                        {
                            windowx = ev.window.data1;
                            windowy = ev.window.data2;
                        }

                        r_displayindex = SDL_GetWindowDisplayIndex(sdl_window);
                        modeschecked = 0;
                        videoGetModes();
                        break;
                    }
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

auto vsnprintfptr = vsnprintf;	// This is an inline in Visual Studio but we need an address for it to satisfy the MinGW compiled libraries.
