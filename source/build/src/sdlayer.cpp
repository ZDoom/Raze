// SDL interface layer for the Build Engine
// Use SDL 1.2 or 2.0 from http://www.libsdl.org

#include <Windows.h>
#include <CommCtrl.h>
#include <signal.h>
#include <string>
#include <stdexcept>
# include "gl_load.h"

#include "a.h"
#include "build.h"

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
#include "gameconfigfile.h"
#include "gamecontrol.h"
#include "resourcefile.h"
#include "sc_man.h"
#include "i_specialpaths.h"
#include "inputstate.h"
#include "c_cvars.h"
#include "i_time.h"
#include "c_dispatch.h"
#include "d_gui.h"
#include "menu.h"
#include "utf8.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"



#ifndef NETCODE_DISABLE
#include "enet.h"
#endif
#include "../../glbackend/glbackend.h"

#ifdef USE_OPENGL
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
#endif

FString progdir;

CVAR(Int, r_displayindex, 0, CVAR_ARCHIVE | CVAR_VIDEOCONFIG)
CVAR(Int, r_borderless, 2, CVAR_ARCHIVE | CVAR_VIDEOCONFIG)
CVAR(Int, maxrefreshfreq, 0, CVAR_ARCHIVE | CVAR_VIDEOCONFIG)
CVAR(Int, windowpos, 0, CVAR_ARCHIVE | CVAR_VIDEOCONFIG)
CVAR(Int, windowx, -1, CVAR_ARCHIVE | CVAR_VIDEOCONFIG)
CVAR(Int, windowy, -1, CVAR_ARCHIVE | CVAR_VIDEOCONFIG)

#if SDL_MAJOR_VERSION != 1
static SDL_version linked;
#endif

double g_beforeSwapTime;
GameInterface* gi;

void buildkeytranslationtable();;

#if !defined STARTUP_SETUP_WINDOW
int32_t startwin_open(void) { return 0; }
int32_t startwin_close(void) { return 0; }
int32_t startwin_puts(const char *s) { UNREFERENCED_PARAMETER(s); return 0; }
int32_t startwin_idle(void *s) { UNREFERENCED_PARAMETER(s); return 0; }
int32_t startwin_settitle(const char *s) { UNREFERENCED_PARAMETER(s); return 0; }
int32_t startwin_run(void) { return 0; }
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
bool screenshot_requested;

char appactive=1, novideo=0;

// video
static SDL_Surface *sdl_surface/*=NULL*/;

#if SDL_MAJOR_VERSION==2
SDL_Window *sdl_window=NULL;
SDL_GLContext sdl_context=NULL;
#endif

void ImGui_Init_Backend()
{
	ImGui_ImplSDL2_InitForOpenGL(sdl_window, sdl_context);
}

void ImGui_Begin_Frame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(sdl_window);
	ImGui::NewFrame();
}

int32_t xres=-1, yres=-1, bpp=0, fullscreen=0, bytesperline, refreshfreq=-1;
intptr_t frameplace=0;
int32_t lockcount=0;
char modechange=1;
char offscreenrendering=0;
char videomodereset = 0;
static uint16_t sysgamma[3][256];
#ifdef USE_OPENGL
// OpenGL stuff
char nogl=0;
#endif
static int32_t vsync_renderlayer;

// last gamma, contrast, brightness

//#define KEY_PRINT_DEBUG


static SDL_Surface *appicon = NULL;
#if !defined __APPLE__ && !defined EDUKE32_TOUCH_DEVICES
static SDL_Surface *loadappicon(void);
#endif

// Joystick dead and saturation zones
uint16_t joydead[9], joysatur[9];

#define MAX_ERRORTEXT 4096

//==========================================================================
//
// I_Error
//
// Throw an error that will send us to the console if we are far enough
// along in the startup process.
//
//==========================================================================

void I_Error(const char *error, ...)
{
	va_list argptr;
	char errortext[MAX_ERRORTEXT];

	va_start(argptr, error);
	vsnprintf(errortext, MAX_ERRORTEXT, error, argptr);
	va_end(argptr);
	#ifdef _WIN32
	OutputDebugStringA(errortext);
	#endif

	throw std::runtime_error(errortext);
}

void I_FatalError(const char* error, ...)
{
    va_list argptr;
    char errortext[MAX_ERRORTEXT];

    va_start(argptr, error);
    vsnprintf(errortext, MAX_ERRORTEXT, error, argptr);
    va_end(argptr);
#ifdef _WIN32
    OutputDebugStringA(errortext);
#endif

    throw std::runtime_error(errortext);
}


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

#else
    UNREFERENCED_PARAMETER(name);
#endif
}

//==========================================================================
//
// win_buildargs
//
// This should be removed once everything can use the FArgs list.
//
//==========================================================================


int32_t win_buildargs(char** argvbuf)
{
    int32_t buildargc = 0;

    FString cmdline_utf8 = FString(GetCommandLineW());

    *argvbuf = Xstrdup(cmdline_utf8.GetChars());

    if (*argvbuf)
    {
        char quoted = 0, instring = 0, swallownext = 0;
        char* wp;
        for (const char* p = wp = *argvbuf; *p; p++)
        {
            if (*p == ' ')
            {
                if (instring)
                {
                    if (!quoted)
                    {
                        // end of a string
                        *(wp++) = 0;
                        instring = 0;
                    }
                    else
                        *(wp++) = *p;
                }
            }
            else if (*p == '"' && !swallownext)
            {
                if (instring)
                {
                    if (quoted && p[1] == ' ')
                    {
                        // end of a string
                        *(wp++) = 0;
                        instring = 0;
                    }
                    quoted = !quoted;
                }
                else
                {
                    instring = 1;
                    quoted = 1;
                    buildargc++;
                }
            }
            else if (*p == '\\' && p[1] == '"' && !swallownext)
                swallownext = 1;
            else
            {
                if (!instring)
                    buildargc++;

                instring = 1;
                *(wp++) = *p;
                swallownext = 0;
            }
        }
        *wp = 0;
    }

    // Figure out what directory the program resides in.

    wchar_t buffer[256];
    GetModuleFileNameW(0, buffer, 256);
    progdir = buffer;
    progdir.Substitute("\\", "/");
    auto lastsep = progdir.LastIndexOf('/');
    if (lastsep != -1)
        progdir.Truncate(lastsep + 1);

    return buildargc;
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

char grabmouse_low(char a);

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
		std::terminate();
		// NOTE: It is not safe to call any of this from a signal handler!
		//gi->app_crashhandler();
        //uninitsystem();
        //Bexit(EXIT_FAILURE);
    }
}
#endif

int GameMain();

#if 0
#ifdef _WIN32

int WINAPI WinMain(HINSTANCE , HINSTANCE , LPSTR , int )
#else
int main(int argc, char *argv[])
#endif
{
    buildkeytranslationtable();

	if (initsystem()) Bexit(9);
	SDL_StartTextInput();

	r = GameMain();

#if defined(HAVE_GTK2)
    gtkbuild_exit(r);
#endif
	return r;
}
#endif

// The resourge manager in cache1d is far too broken to add some arbitrary file without some adjustment.
// For now, keep this file here, until the resource management can be redone in a more workable fashion.

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
                Printf("Unable to enable VSync!\n");
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

//
// system_getcvars() -- propagate any cvars that are read post-initialization
//
void system_getcvars(void)
{
    vid_vsync = videoSetVsync(vid_vsync);
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
    FileReader fh;
    if (!fh.OpenFile("gamecontrollerdb.txt"))
        return;

	int flen = fh.GetLength();
    if (flen <= 0)
    {
        return;
    }

    char * dbuf = (char *)malloc(flen + 1);
    if (!dbuf)
    {
        return;
    }

    if (fh.Read(dbuf, flen) != flen)
    {
        free(dbuf);
        return;
    }

    dbuf[flen] = '\0';

    SDL_RWops * rwops = SDL_RWFromConstMem(dbuf, flen);
    if (!rwops)
    {
        free(dbuf);
        return;
    }

    int i = SDL_GameControllerAddMappingsFromRW(rwops, 1);

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
        initprintf("No game controllers found\n");
    }
    else
    {
        initprintf("Game controllers:\n");
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
                joystick.numBalls   = 0;
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
                joystick.numBalls   = SDL_JoystickNumBalls(joydev);
                joystick.numButtons = min(32, SDL_JoystickNumButtons(joydev));
                joystick.numHats    = min((36 - joystick.numButtons) / 4, SDL_JoystickNumHats(joydev));

                joystick.isGameController = 0;

                buildprint("Joystick ", i+1, " has ", joystick.numAxes, " axes, ", joystick.numButtons, " buttons, ",
                            (joystick.numHats ? std::to_string(joystick.numHats).c_str() : "no"), " hats, and ",
                            (joystick.numBalls ? std::to_string(joystick.numBalls).c_str() : "no"), " balls.\n");

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

        initprintf("No controllers are usable\n");
    }
}

//
// initinput() -- init input system
//
int32_t initinput(void)
{

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

void WindowMoved(int x, int y)
{
    if (windowpos)
    {
        windowx = x;
        windowy = y;
    }

    r_displayindex = SDL_GetWindowDisplayIndex(sdl_window);
    modeschecked = 0;
    videoGetModes();
}


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

void sdlayer_setvideomode_opengl(int y)
{
    glsurface_destroy();
    polymost_glreset();

	GLInterface.Deinit();
	GLInterface.Init(y);
	GLInterface.InitGLState(4, glmultisample);

	GLInterface.mSamplers->SetTextureFilterMode(hw_texfilter, hw_anisotropy);

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

    return 1;
}

void setvideomode_sdlcommonpost(int32_t x, int32_t y, int32_t c, int32_t fs, int32_t regrab)
{

#ifdef USE_OPENGL
    if (!nogl)
        sdlayer_setvideomode_opengl(y);
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

	// SDL's fullscreen is a deadly trap in the debugger.
#ifdef _DEBUG
	if (fs)
	{
		fs = false;
		x -= 10;
		y -= 10;
	}
	if (called++)
	{
		//assert(0);
		return 0;
	}
#endif


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
        if (bpp == 8)
        {
            glsurface_blitBuffer();
        }

        static uint32_t lastSwapTime = 0;
		glFinish();
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

	return 1;
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

auto vsnprintfptr = vsnprintf;	// This is an inline in Visual Studio but we need an address for it to satisfy the MinGW compiled libraries.

//
// debugprintf() -- sends a debug string to the debugger
//
void debugprintf(const char* f, ...)
{
	va_list va;
	char buf[1024];

	if (!IsDebuggerPresent()) return;

	va_start(va, f);
	Bvsnprintf(buf, 1024, f, va);
	va_end(va);

	OutputDebugStringA(buf);
}

