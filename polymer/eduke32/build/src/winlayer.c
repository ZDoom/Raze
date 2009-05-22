// Windows DIB/DirectDraw interface layer
// for the Build Engine
// by Jonathon Fowler (jonof@edgenetwk.com)
//
// This is all very ugly.
//
// Written for DirectX 6.0

#ifndef _WIN32
#error winlayer.c is for Windows only.
#endif

#define DIRECTINPUT_VERSION 0x0700
#define DIRECTDRAW_VERSION  0x0600

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddraw.h>
#include <dinput.h>
#ifndef DIK_PAUSE
# define DIK_PAUSE 0xC5
#endif
#include <math.h>

#include "dxdidf.h"	// comment this out if c_dfDI* is being reported as multiply defined
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>

#if defined(USE_OPENGL) && defined(POLYMOST)
#include "glbuild.h"
#endif

#include "compat.h"
#include "winlayer.h"
#include "pragmas.h"
#include "build.h"
#include "a.h"
#include "osd.h"
#include "mmulti.h"

// undefine to restrict windowed resolutions to conventional sizes
#define ANY_WINDOWED_SIZE

int32_t   _buildargc = 0;
const char **_buildargv = NULL;
static char *argvbuf = NULL;
extern int32_t app_main(int32_t argc, const char *argv[]);
extern void app_crashhandler(void);

// Windows crud
static HINSTANCE hInstance = 0;
static HWND hWindow = 0;
#define WINDOW_STYLE (WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX)
#define WindowClass "buildapp"
static BOOL window_class_registered = FALSE;
static HANDLE instanceflag = NULL;

int32_t    backgroundidle = 1;
int32_t	   is_vista = 0;

static WORD sysgamma[3][256];
extern int32_t curbrightness, gammabrightness;

#if defined(USE_OPENGL) && defined(POLYMOST)
// OpenGL stuff
static HGLRC hGLRC = 0;
char nofog=0;
char nogl=0;
char forcegl=0;
#endif

static LPTSTR GetWindowsErrorMsg(DWORD code);
static const char * GetDDrawError(HRESULT code);
static const char * GetDInputError(HRESULT code);
static void ShowErrorBox(const char *m);
static void ShowDDrawErrorBox(const char *m, HRESULT r);
static void ShowDInputErrorBox(const char *m, HRESULT r);
static BOOL CheckWinVersion(void);
static LRESULT CALLBACK WndProcCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL InitDirectDraw(void);
static void UninitDirectDraw(void);
static int32_t RestoreDirectDrawMode(void);
static void ReleaseDirectDrawSurfaces(void);
static BOOL InitDirectInput(void);
static void UninitDirectInput(void);
static void GetKeyNames(void);
static void AcquireInputDevices(char acquire, int8_t device);
static inline void ProcessInputDevices(void);
static int32_t SetupDirectDraw(int32_t width, int32_t height);
static void UninitDIB(void);
static int32_t SetupDIB(int32_t width, int32_t height);
static void ReleaseOpenGL(void);
static void UninitOpenGL(void);
static int32_t SetupOpenGL(int32_t width, int32_t height, int32_t bitspp);
static BOOL RegisterWindowClass(void);
static BOOL CreateAppWindow(int32_t modenum);
static void DestroyAppWindow(void);
static void SaveSystemColours(void);
static void SetBWSystemColours(void);
static void RestoreSystemColours(void);

// video
static int32_t desktopxdim=0,desktopydim=0,desktopbpp=0,modesetusing=-1;
int32_t xres=-1, yres=-1, fullscreen=0, bpp=0, bytesperline=0, imageSize=0;
intptr_t frameplace=0;
int32_t lockcount=0;
static int32_t curvidmode = -1;
static int32_t customxdim = 640, customydim = 480, custombpp = 8, customfs = 0;
static uint32_t modeschecked=0;
uint32_t maxrefreshfreq=60;
char modechange=1, repaintneeded=0;
char offscreenrendering=0;
int32_t glcolourdepth=32;
char videomodereset = 0;
char silentvideomodeswitch = 0;
int32_t vsync=0;
// input and events
char inputdevices=0;
char quitevent=0, appactive=1, realfs=0, regrabmouse=0;
volatile int32_t mousex=0, mousey=0, mouseb=0;
static uint32_t mousewheel[2] = { 0,0 };
#define MouseWheelFakePressTime (50)	// getticks() is a 1000Hz timer, and the button press is faked for 100ms
int32_t *joyaxis = NULL, joyb=0, *joyhat = NULL;
char joyisgamepad=0, joynumaxes=0, joynumbuttons=0, joynumhats=0;

static char taskswitching=1;

char keystatus[256], keyfifo[KEYFIFOSIZ], keyfifoplc, keyfifoend;
char keyasciififo[KEYFIFOSIZ], keyasciififoplc, keyasciififoend;
char remap[256];
int32_t remapinit=0;
static char key_names[256][24];
static uint32_t lastKeyDown = 0;
static uint32_t lastKeyTime = 0;

void (*keypresscallback)(int32_t,int32_t) = 0;
void (*mousepresscallback)(int32_t,int32_t) = 0;
void (*joypresscallback)(int32_t,int32_t) = 0;




//-------------------------------------------------------------------------------------------------
//  MAIN CRAP
//=================================================================================================


//
// win_gethwnd() -- gets the window handle
//
int32_t win_gethwnd(void)
{
    return (int32_t)hWindow;
}


//
// win_gethinstance() -- gets the application instance
//
int32_t win_gethinstance(void)
{
    return (int32_t)hInstance;
}


//
// win_allowtaskswitching() -- captures/releases alt+tab hotkeys
//
void win_allowtaskswitching(int32_t onf)
{
    if (onf == taskswitching) return;

    if (onf)
    {
        UnregisterHotKey(0,0);
        UnregisterHotKey(0,1);
    }
    else
    {
        RegisterHotKey(0,0,MOD_ALT,VK_TAB);
        RegisterHotKey(0,1,MOD_ALT|MOD_SHIFT,VK_TAB);
    }

    taskswitching = onf;
}


//
// win_checkinstance() -- looks for another instance of a Build app
//
int32_t win_checkinstance(void)
{
    if (!instanceflag) return 0;
    return (WaitForSingleObject(instanceflag,0) == WAIT_TIMEOUT);
}


//
// wm_msgbox/wm_ynbox() -- window-manager-provided message boxes
//
int32_t wm_msgbox(char *name, char *fmt, ...)
{
    char buf[2048];
    va_list va;

    va_start(va,fmt);
    vsprintf(buf,fmt,va);
    va_end(va);

    MessageBox(hWindow,buf,name,MB_OK|MB_TASKMODAL);
    return 0;
}
int32_t wm_ynbox(char *name, char *fmt, ...)
{
    char buf[2048];
    va_list va;
    int32_t r;

    va_start(va,fmt);
    vsprintf(buf,fmt,va);
    va_end(va);

    r = MessageBox((HWND)win_gethwnd(),buf,name,MB_YESNO|MB_ICONQUESTION|MB_TASKMODAL);
    if (r==IDYES) return 1;
    return 0;
}

//
// wm_setapptitle() -- changes the window title
//
void wm_setapptitle(char *name)
{
    if (name)
    {
        Bstrncpy(apptitle, name, sizeof(apptitle)-1);
        apptitle[ sizeof(apptitle)-1 ] = 0;
    }

    if (hWindow) SetWindowText(hWindow, apptitle);
    startwin_settitle(apptitle);
}

static int32_t setgammaramp(WORD gt[3][256]);

//
// SignalHandler() -- called when we've sprung a leak
//
static void SignalHandler(int32_t signum)
{
    switch (signum)
    {
    case SIGSEGV:
        printOSD("Fatal Signal caught: SIGSEGV. Bailing out.\n");
        if (gammabrightness)
            setgammaramp(sysgamma);
        gammabrightness = 0;
        app_crashhandler();
        uninitsystem();
        if (stdout) fclose(stdout);
        break;
    default:
        break;
    }
}

//
// WinMain() -- main Windows entry point
//
int32_t WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int32_t nCmdShow)
{
    int32_t r;
    char *argp;
    FILE *fp;
    HDC hdc;

    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    hInstance = hInst;

    if (CheckWinVersion() || hPrevInst)
    {
        MessageBox(0, "This application must be run under Windows 95/98/Me or Windows 2000/XP or better.",
                   apptitle, MB_OK|MB_ICONSTOP);
        return -1;
    }

    hdc = GetDC(NULL);
    r = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(NULL, hdc);
    if (r < 8)
    {
        MessageBox(0, "This application requires a desktop colour depth of 256-colours or more.",
                   apptitle, MB_OK|MB_ICONSTOP);
        return -1;
    }

    // carve up the commandline into more recognizable pieces
    argvbuf = strdup(GetCommandLine());
    _buildargc = 0;
    if (argvbuf)
    {
        char quoted = 0, instring = 0, swallownext = 0;
        char *p,*wp; int32_t i;
        for (p=wp=argvbuf; *p; p++)
        {
            if (*p == ' ')
            {
                if (instring && !quoted)
                {
                    // end of a string
                    *(wp++) = 0;
                    instring = 0;
                }
                else if (instring)
                {
                    *(wp++) = *p;
                }
            }
            else if (*p == '"' && !swallownext)
            {
                if (instring && quoted)
                {
                    // end of a string
                    if (p[1] == ' ')
                    {
                        *(wp++) = 0;
                        instring = 0;
                        quoted = 0;
                    }
                    else
                    {
                        quoted = 0;
                    }
                }
                else if (instring && !quoted)
                {
                    quoted = 1;
                }
                else if (!instring)
                {
                    instring = 1;
                    quoted = 1;
                    _buildargc++;
                }
            }
            else if (*p == '\\' && p[1] == '"' && !swallownext)
            {
                swallownext = 1;
            }
            else
            {
                if (!instring) _buildargc++;
                instring = 1;
                *(wp++) = *p;
                swallownext = 0;
            }
        }
        *wp = 0;

        _buildargv = (const char**)malloc(sizeof(char*)*_buildargc);
        wp = argvbuf;
        for (i=0; i<_buildargc; i++,wp++)
        {
            _buildargv[i] = wp;
            while (*wp) wp++;
        }
    }

    // pipe standard outputs to files
    if ((argp = Bgetenv("BUILD_LOGSTDOUT")) != NULL)
        if (!Bstrcasecmp(argp, "TRUE"))
        {
            fp = freopen("stdout.txt", "w", stdout);
            if (!fp)
            {
                fp = fopen("stdout.txt", "w");
            }
            if (fp) setvbuf(fp, 0, _IONBF, 0);
            *stdout = *fp;
            *stderr = *fp;
        }

#if defined(USE_OPENGL) && defined(POLYMOST)
    if ((argp = Bgetenv("BUILD_NOFOG")) != NULL)
        nofog = Batol(argp);
    if (Bgetenv("BUILD_FORCEGL") != NULL)
        forcegl = 1;
#endif

    // install signal handlers
#if !defined(_MSC_VER) || !defined(DEBUGGINGAIDS)
    signal(SIGSEGV, SignalHandler);
#endif

    if (RegisterWindowClass()) return -1;

#ifdef DISCLAIMER
    MessageBox(0,
               DISCLAIMER,
               "Notice",
               MB_OK|MB_ICONINFORMATION);
#endif

//    atexit(uninitsystem);

    instanceflag = CreateSemaphore(NULL, 1,1, WindowClass);

    startwin_open();
    baselayer_init();

    r = app_main(_buildargc, _buildargv);

    fclose(stdout);

    startwin_close();
    if (instanceflag) CloseHandle(instanceflag);

    if (argvbuf) free(argvbuf);

    return r;
}


static int32_t set_maxrefreshfreq(const osdfuncparm_t *parm)
{
    int32_t freq;
    if (parm->numparms == 0)
    {
        if (maxrefreshfreq == 0)
            OSD_Printf("\"maxrefreshfreq\" is \"No maximum\"\n");
        else
            OSD_Printf("\"maxrefreshfreq\" is \"%d\"\n",maxrefreshfreq);
        return OSDCMD_OK;
    }
    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    freq = Batol(parm->parms[0]);
    if (freq < 0) return OSDCMD_SHOWHELP;

    maxrefreshfreq = (unsigned)freq;
    modeschecked = 0;

    return OSDCMD_OK;
}

static int32_t set_windowpos(const osdfuncparm_t *parm)
{
    if (parm->numparms == 0)
    {
        OSD_Printf("\"r_windowpositioning\" is \"%d\"\n",windowpos);
        return OSDCMD_OK;
    }
    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    windowpos = Batol(parm->parms[0])>0;
    OSD_Printf("r_windowpositioning %d\n",windowpos);

    return OSDCMD_OK;
}

//
// initsystem() -- init systems
//

static void print_os_version(void)
{
    OSVERSIONINFO osv;
    const char *ver = NULL;
    // I was going to call this 'windows_9x_is_awful', but I couldn't justify ever setting it to 0
    int32_t awful_windows_9x = 0;

    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osv);

    switch (osv.dwPlatformId)
    {
    case VER_PLATFORM_WIN32_NT:
        if (osv.dwMajorVersion == 5)
        {
            switch (osv.dwMinorVersion)
            {
            case 0:
                ver = "2000";
                break;
            case 1:
                ver = "XP";
                break;
            case 2:
                ver = "Server 2003";
                break;
            }
            break;
        }

        if (osv.dwMajorVersion == 6)
        {
            is_vista = 1;
            switch (osv.dwMinorVersion)
            {
            case 0:
                ver = "Vista";
                break;
            case 1:
                ver = "7";
                break;
            }
            break;
        }
        break;

    case VER_PLATFORM_WIN32_WINDOWS:
        awful_windows_9x = 1;
        if (osv.dwMinorVersion < 10)
            ver = "95";
        else if (osv.dwMinorVersion < 90)
            ver = "98";
        else ver = "Me";
        break;

    default:
        ver = "Unknown";
        initprintf("OS: Unknown OS\n");
        return;
    }

    if (ver != NULL)
    {
        initprintf("OS: Windows %s (%lu.%lu.%lu) %s\n", ver, osv.dwMajorVersion, osv.dwMinorVersion,
                   awful_windows_9x?osv.dwBuildNumber&0xffff:osv.dwBuildNumber,osv.szCSDVersion);
//        if (osv.szCSDVersion[0])
        //          initprintf("  - %s\n", osv.szCSDVersion);
    }
}


int32_t initsystem(void)
{
    DEVMODE desktopmode;

//    initprintf("Initializing Windows DirectX/GDI system interface\n");

    // get the desktop dimensions before anything changes them
    ZeroMemory(&desktopmode, sizeof(DEVMODE));
    desktopmode.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&desktopmode);

    desktopxdim = desktopmode.dmPelsWidth;
    desktopydim = desktopmode.dmPelsHeight;
    desktopbpp  = desktopmode.dmBitsPerPel;

    if (desktopbpp <= 8)
        // save the system colours
        SaveSystemColours();

    memset(curpalette, 0, sizeof(palette_t) * 256);

    atexit(uninitsystem);

    frameplace=0;
    lockcount=0;

    print_os_version();

#if defined(USE_OPENGL) && defined(POLYMOST)
    if (loadgldriver(getenv("BUILD_GLDRV")))
    {
        initprintf("Failed loading OpenGL driver. GL modes will be unavailable.\n");
        nogl = 1;
    }
#endif

    // try and start DirectDraw
    if (InitDirectDraw())
        initprintf("DirectDraw initialization failed. Fullscreen modes will be unavailable.\n");

    OSD_RegisterFunction("maxrefreshfreq", "maxrefreshfreq: maximum display frequency to set for OpenGL Polymost modes (0=no maximum)", set_maxrefreshfreq);
    OSD_RegisterFunction("r_windowpositioning", "r_windowpositioning: enable/disable window position memory", set_windowpos);
    return 0;
}


//
// uninitsystem() -- uninit systems
//
void uninitsystem(void)
{
    DestroyAppWindow();

    startwin_close();

    uninitinput();
    uninittimer();

    win_allowtaskswitching(1);

#if defined(USE_OPENGL) && defined(POLYMOST)
    unloadgldriver();
#endif
}


//
// initprintf() -- prints a string to the intitialization window
//
void initprintf(const char *f, ...)
{
    va_list va;
    char buf[1024];
    static char dabuf[1024];
    static int32_t cnt = 0;

    va_start(va, f);
    Bvsnprintf(buf, 1024, f, va);
    va_end(va);

    OSD_Printf(buf);

    if (Bstrlen(dabuf) + Bstrlen(buf) > 1022)
    {
        startwin_puts(dabuf);
        Bmemset(dabuf, 0, sizeof(dabuf));
    }

    Bstrcat(dabuf,buf);

    if (++cnt < 16 || flushlogwindow || Bstrlen(dabuf) > 768 || numplayers > 1)
    {
        startwin_puts(dabuf);
        handleevents();
        Bmemset(dabuf, 0, sizeof(dabuf));
    }
}


//
// debugprintf() -- sends a debug string to the debugger
//
void debugprintf(const char *f, ...)
{
#if 0 // def DEBUGGINGAIDS
    va_list va;
    char buf[1024];

    if (!IsDebuggerPresent()) return;

    va_start(va,f);
    Bvsnprintf(buf, 1024, f, va);
    va_end(va);

    OutputDebugString(buf);
#else
    UNREFERENCED_PARAMETER(f);
#endif
}


//
// handleevents() -- process the Windows message queue
//   returns !0 if there was an important event worth checking (like quitting)
//
int32_t handleevents(void)
{
    int32_t rv=0;
    MSG msg;

    //if (frameplace && fullscreen) printf("Offscreen buffer is locked!\n");

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            quitevent = 1;

        if (startwin_idle((void*)&msg) > 0) continue;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ProcessInputDevices();

    if (!appactive || quitevent) rv = -1;

    sampletimer();

    return rv;
}

inline void idle(void)
{
    Sleep(1);
}



//-------------------------------------------------------------------------------------------------
//  INPUT (MOUSE/KEYBOARD/JOYSTICK?)
//=================================================================================================

#define KEYBOARD	0
#define MOUSE		1
#define JOYSTICK	2
#define NUM_INPUTS	3

static HMODULE               hDInputDLL    = NULL;
static LPDIRECTINPUT7A        lpDI          = NULL;
static LPDIRECTINPUTDEVICE7A lpDID[NUM_INPUTS] =  { NULL, NULL, NULL };
static BOOL                  bDInputInited = FALSE;
#define INPUT_BUFFER_SIZE	32
static GUID                  guidDevs[NUM_INPUTS];

static char devacquired[NUM_INPUTS] = { 0,0,0 };
static HANDLE inputevt[NUM_INPUTS] = {0,0,0};
static int32_t joyblast=0;
static char volatile moustat = 0, mousegrab = 0;
HANDLE  mousethread;

static struct
{
    char *name;
    LPDIRECTINPUTDEVICE7A *did;
    const DIDATAFORMAT *df;
} devicedef[NUM_INPUTS] =
{
    { "keyboard", &lpDID[KEYBOARD], &c_dfDIKeyboard },
    { "mouse",    &lpDID[MOUSE],    &c_dfDIMouse2 },
    { "joystick", &lpDID[JOYSTICK], &c_dfDIJoystick }
};
static struct _joydef
{
    const char *name;
    uint32_t ofs;	// directinput 'dwOfs' value
} *axisdefs = NULL, *buttondefs = NULL, *hatdefs = NULL;

struct _joydevicefeature
{
    uint32_t ofs;
    const char *name;
};
struct _joydevicedefn
{
    uint32_t devid;	// is the value of DIDEVICEINSTANCE.guidProduct.Data1
    int32_t nfeatures;
    struct _joydevicefeature *features;
};

// This is to give more realistic names to the buttons, axes, etc on a controller than
// those the driver reports. Curse inconsistency.
struct _joydevicefeature joyfeatures_C20A046D[] =  	// Logitech WingMan RumblePad USB
{
    { 0,  "Left Stick X" }, { 4,  "Left Stick Y" }, { 8,  "Right Stick X" },
    { 20, "Right Stick Y" }, { 24, "Throttle" },
};
struct _joydevicefeature joyfeatures_C218046D[] =  	// Logitech RumblePad 2 USB
{
    { 0,  "Left Stick X" }, { 4,  "Left Stick Y" }, { 8,  "Right Stick X" },
    { 20, "Right Stick Y" }, { 32, "D-Pad" }, { 48, "Button 1" },
    { 49, "Button 2" }, { 50, "Button 3" }, { 51, "Button 4" },
    { 52, "Button 5" }, { 53, "Button 6" }, { 54, "Button 7" },
    { 55, "Button 8" }, { 56, "Button 9" }, { 57, "Button 10" },
    { 58, "Left Stick Press" }, { 59, "Right Stick Press" },
};
#define featurecount(x) (sizeof(x)/sizeof(struct _joydevicefeature))
static struct _joydevicedefn *thisjoydef = NULL, joyfeatures[] =
{
    { 0xC20A046D, featurecount(joyfeatures_C20A046D), joyfeatures_C20A046D },	// Logitech WingMan RumblePad USB
    { 0xC218046D, featurecount(joyfeatures_C218046D), joyfeatures_C218046D },	// Logitech RumblePad 2 USB
};
#undef featurecount

// I don't see any pressing need to store the key-up events yet
#define SetKey(key,state) { \
        keystatus[remap[key]] = state; \
		if (state) { \
        keyfifo[keyfifoend] = remap[key]; \
	keyfifo[(keyfifoend+1)&(KEYFIFOSIZ-1)] = state; \
	keyfifoend = ((keyfifoend+2)&(KEYFIFOSIZ-1)); \
		} \
}

char map_dik_code(int32_t scanCode)
{
    // ugly table for remapping out of range DIK_ entries will go here
    // if I can't figure out the layout problem
    return scanCode;
}

//
// initinput() -- init input system
//
int32_t initinput(void)
{
    int32_t i;
    moustat=0;
    memset(keystatus, 0, sizeof(keystatus));
    if (!remapinit)
        for (i=0; i<256; i++)
            remap[i]=map_dik_code(i);
    remapinit=1;
    keyfifoplc = keyfifoend = 0;
    keyasciififoplc = keyasciififoend = 0;

    inputdevices = 0;
    joyisgamepad=0, joynumaxes=0, joynumbuttons=0, joynumhats=0;

    {
        TCHAR layoutname[KL_NAMELENGTH];
//        GetKeyboardLayoutName(layoutname);
//        initprintf("    * Keyboard layout: %s\n",layoutname);
        LoadKeyboardLayout("00000409", KLF_ACTIVATE|KLF_SETFORPROCESS|KLF_SUBSTITUTE_OK);
        GetKeyboardLayoutName(layoutname);
        initprintf("Using keyboard layout %s\n",layoutname);
    }

    if (InitDirectInput())
        return -1;

    return 0;
}


//
// uninitinput() -- uninit input system
//
void uninitinput(void)
{
    uninitmouse();
    UninitDirectInput();
}


//
// bgetchar, bkbhit, bflushchars -- character-based input functions
//
char bgetchar(void)
{
    char c;
    if (keyasciififoplc == keyasciififoend) return 0;
    c = keyasciififo[keyasciififoplc];
    keyasciififoplc = ((keyasciififoplc+1)&(KEYFIFOSIZ-1));
    //OSD_Printf("bgetchar %d, %d-%d\n",c,keyasciififoplc,keyasciififoend);
    return c;
}

int32_t bkbhit(void)
{
    return (keyasciififoplc != keyasciififoend);
}

void bflushchars(void)
{
    Bmemset(&keyasciififo,0,sizeof(keyasciififo));
    keyasciififoplc = keyasciififoend = 0;
}


//
// set{key|mouse|joy}presscallback() -- sets a callback which gets notified when keys are pressed
//
void setkeypresscallback(void (*callback)(int32_t, int32_t)) { keypresscallback = callback; }
void setmousepresscallback(void (*callback)(int32_t, int32_t)) { mousepresscallback = callback; }
void setjoypresscallback(void (*callback)(int32_t, int32_t)) { joypresscallback = callback; }


DWORD WINAPI ProcessMouse(LPVOID lpThreadParameter)
{
    UNREFERENCED_PARAMETER(lpThreadParameter);
    while (moustat && lpDID[MOUSE])
    {
        if (!appactive)
        {
            Sleep(100);
            continue;
        }
        if ((WaitForSingleObject(inputevt[MOUSE], INFINITE)) != WAIT_OBJECT_0)
            continue;
        {
            /*DWORD i;*/
            uint32_t t;
            int32_t result;
            DIDEVICEOBJECTDATA didod;
            DWORD dwElements = 1;

            do
            {
                if (!mousegrab)
                    break;

                t = getticks();
                result = IDirectInputDevice7_GetDeviceData(lpDID[MOUSE], sizeof(DIDEVICEOBJECTDATA),
                         (LPDIDEVICEOBJECTDATA)&didod, &dwElements, 0);

                if (!dwElements || result != DI_OK)
                    break;
                //        else if (result == DI_OK)
                {
                    // process the mouse events
                    //  			  mousex=0;
                    //  			  mousey=0;
                    //            for (i=0; i<dwElements; i++)
                    {
                        if (didod.dwOfs == DIMOFS_X)
                            mousex += (int16_t)didod.dwData;
                        else if (didod.dwOfs == DIMOFS_Y)
                            mousey += (int16_t)didod.dwData;
                        else if (didod.dwOfs == DIMOFS_Z)
                        {
                            if ((int32_t)didod.dwData > 0)   	// wheel up
                            {
                                if (mousewheel[0] > 0 && mousepresscallback) mousepresscallback(5,0);
                                mousewheel[0] = t;
                                mouseb |= 16; if (mousepresscallback) mousepresscallback(5, 1);
                            }
                            else if ((int32_t)didod.dwData < 0)  	// wheel down
                            {
                                if (mousewheel[1] > 0 && mousepresscallback) mousepresscallback(6,0);
                                mousewheel[1] = t;
                                mouseb |= 32; if (mousepresscallback) mousepresscallback(6, 1);
                            }
                        }
                        else if (didod.dwOfs >= DIMOFS_BUTTON0 && didod.dwOfs <= DIMOFS_BUTTON7)
                        {
                            if (didod.dwOfs == DIMOFS_BUTTON0)
                            {
                                if (didod.dwData & 0x80) mouseb |= 1;
                                else mouseb &= ~1;
                                if (mousepresscallback)
                                    mousepresscallback(1, (mouseb&1)==1);
                            }
                            else if (didod.dwOfs == DIMOFS_BUTTON1)
                            {
                                if (didod.dwData & 0x80) mouseb |= 2;
                                else mouseb &= ~2;
                                if (mousepresscallback)
                                    mousepresscallback(2, (mouseb&2)==2);
                            }
                            else if (didod.dwOfs == DIMOFS_BUTTON2)
                            {
                                if (didod.dwData & 0x80) mouseb |= 4;
                                else mouseb &= ~4;
                                if (mousepresscallback)
                                    mousepresscallback(3, (mouseb&4)==4);
                            }
                            else if (didod.dwOfs == DIMOFS_BUTTON3)
                            {
                                if (didod.dwData & 0x80) mouseb |= 8;
                                else mouseb &= ~8;
                                if (mousepresscallback)
                                    mousepresscallback(4, (mouseb&8)==8);
                            }
                            else if (didod.dwOfs == DIMOFS_BUTTON4)
                            {
                                OSD_Printf("got button4\n");
                                if (didod.dwData & 0x80) mouseb |= 64;
                                else mouseb &= ~64;
                                if (mousepresscallback)
                                    mousepresscallback(7, (mouseb&64)==64);
                            }
                            else if (didod.dwOfs == DIMOFS_BUTTON5)
                            {
                                OSD_Printf("got button5\n");
                                if (didod.dwData & 0x80) mouseb |= 128;
                                else mouseb &= ~128;
                                if (mousepresscallback)
                                    mousepresscallback(8, (mouseb&128)==128);
                            }
                            else if (didod.dwOfs == DIMOFS_BUTTON6)
                            {
                                OSD_Printf("got button6\n");
                                if (didod.dwData & 0x80) mouseb |= 256;
                                else mouseb &= ~256;
                                if (mousepresscallback)
                                    mousepresscallback(9, (mouseb&256)==256);
                            }
                            else if (didod.dwOfs == DIMOFS_BUTTON7)
                            {
                                OSD_Printf("got button7\n");
                                if (didod.dwData & 0x80) mouseb |= 512;
                                else mouseb &= ~512;
                                if (mousepresscallback)
                                    mousepresscallback(10, (mouseb&512)==512);
                            }
                        }
                    }
                }
            }
            while (1);
        }
    }
    return 0;
}
//
// initmouse() -- init mouse input
//
int32_t initmouse(void)
{
    DWORD   threadid;

    if (moustat) return 0;

//    initprintf("Initializing mouse... ");

    moustat=1;
    mousethread = CreateThread
                  (
                      NULL,
                      0,
                      ProcessMouse,
                      NULL,
                      CREATE_SUSPENDED,
                      &threadid
                  );

    if (!mousethread)
    {
        initprintf("Failed to create mouse thread!\n");
        return 0;
    }

    SetThreadPriority(mousethread, THREAD_PRIORITY_ABOVE_NORMAL);
    ResumeThread(mousethread);
//    initprintf("OK\n");

    // grab input
    grabmouse(1);

    return 0;
}


//
// uninitmouse() -- uninit mouse input
//
void uninitmouse(void)
{
    if (!moustat) return;

    grabmouse(0);
    moustat=mousegrab=0;
    SetEvent(inputevt[MOUSE]);
    if (WaitForSingleObject(mousethread, 300) != WAIT_OBJECT_0)
//        initprintf("DirectInput: Mouse thread has exited\n");
//    else
        initprintf("DirectInput: Mouse thread failed to exit!\n");
}


//
// grabmouse() -- show/hide mouse cursor
//
void grabmouse(char a)
{
    if (!moustat) return;

    mousegrab = a;
    AcquireInputDevices(a,MOUSE);	// only release or grab the mouse

    mousex = 0;
    mousey = 0;
    mouseb = 0;
}


//
// readmousexy() -- return mouse motion information
//
void readmousexy(int32_t *x, int32_t *y)
{
    if (!moustat || !devacquired[MOUSE] || !mousegrab) { *x = *y = 0; return; }
    *x = mousex;
    *y = mousey;
    mousex -= *x;
    mousey -= *y;
}


//
// readmousebstatus() -- return mouse button information
//
void readmousebstatus(int32_t *b)
{
    if (!moustat || !devacquired[MOUSE] || !mousegrab) *b = 0;
    else *b = mouseb;
}


//
// setjoydeadzone() -- sets the dead and saturation zones for the joystick
//
void setjoydeadzone(int32_t axis, uint16_t dead, uint16_t satur)
{
    DIPROPDWORD dipdw;
    HRESULT result;

    if (!lpDID[JOYSTICK]) return;

    if (dead > 10000) dead = 10000;
    if (satur > 10000) satur = 10000;
    if (dead >= satur) dead = satur-100;
    if (axis >= joynumaxes) return;

    memset(&dipdw, 0, sizeof(dipdw));
    dipdw.diph.dwSize = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    if (axis < 0)
    {
        dipdw.diph.dwObj = 0;
        dipdw.diph.dwHow = DIPH_DEVICE;
    }
    else
    {
        dipdw.diph.dwObj = axisdefs[axis].ofs;
        dipdw.diph.dwHow = DIPH_BYOFFSET;
    }
    dipdw.dwData = dead;

    result = IDirectInputDevice7_SetProperty(lpDID[JOYSTICK], DIPROP_DEADZONE, &dipdw.diph);
    if FAILED(result)
    {
        //ShowDInputErrorBox("Failed setting joystick dead zone", result);
        initprintf("Failed setting joystick dead zone: %s\n", GetDInputError(result));
        return;
    }

    dipdw.dwData = satur;

    result = IDirectInputDevice7_SetProperty(lpDID[JOYSTICK], DIPROP_SATURATION, &dipdw.diph);
    if FAILED(result)
    {
        //ShowDInputErrorBox("Failed setting joystick saturation point", result);
        initprintf("Failed setting joystick saturation point: %s\n", GetDInputError(result));
        return;
    }
}


//
// getjoydeadzone() -- gets the dead and saturation zones for the joystick
//
void getjoydeadzone(int32_t axis, uint16_t *dead, uint16_t *satur)
{
    DIPROPDWORD dipdw;
    HRESULT result;

    if (!dead || !satur) return;
    if (!lpDID[JOYSTICK]) { *dead = *satur = 0; return; }
    if (axis >= joynumaxes) { *dead = *satur = 0; return; }

    memset(&dipdw, 0, sizeof(dipdw));
    dipdw.diph.dwSize = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    if (axis < 0)
    {
        dipdw.diph.dwObj = 0;
        dipdw.diph.dwHow = DIPH_DEVICE;
    }
    else
    {
        dipdw.diph.dwObj = axisdefs[axis].ofs;
        dipdw.diph.dwHow = DIPH_BYOFFSET;
    }

    result = IDirectInputDevice7_GetProperty(lpDID[JOYSTICK], DIPROP_DEADZONE, &dipdw.diph);
    if FAILED(result)
    {
        //ShowDInputErrorBox("Failed getting joystick dead zone", result);
        initprintf("Failed getting joystick dead zone: %s\n", GetDInputError(result));
        return;
    }

    *dead = dipdw.dwData;

    result = IDirectInputDevice7_GetProperty(lpDID[JOYSTICK], DIPROP_SATURATION, &dipdw.diph);
    if FAILED(result)
    {
        //ShowDInputErrorBox("Failed getting joystick saturation point", result);
        initprintf("Failed getting joystick saturation point: %s\n", GetDInputError(result));
        return;
    }

    *satur = dipdw.dwData;
}


void releaseallbuttons(void)
{
    int32_t i;

    if (mousepresscallback)
    {
        if (mouseb & 1) mousepresscallback(1, 0);
        if (mouseb & 2) mousepresscallback(2, 0);
        if (mouseb & 4) mousepresscallback(3, 0);
        if (mouseb & 8) mousepresscallback(4, 0);
        if (mousewheel[0]>0) mousepresscallback(5,0);
        if (mousewheel[1]>0) mousepresscallback(6,0);
        if (mouseb & 64) mousepresscallback(7, 0);
        if (mouseb & 128) mousepresscallback(8, 0);
        if (mouseb & 256) mousepresscallback(9, 0);
        if (mouseb & 512) mousepresscallback(10, 0);
    }
    mousewheel[0]=mousewheel[1]=0;
    mouseb = 0;

    if (joypresscallback)
    {
        for (i=0; i<32; i++)
            if (joyb & (1<<i)) joypresscallback(i+1, 0);
    }
    joyb = joyblast = 0;

    for (i=0; i<256; i++)
    {
        //if (!keystatus[i]) continue;
        //if (OSD_HandleKey(i, 0) != 0) {
        OSD_HandleScanCode(i, 0);
        SetKey(i, 0);
        if (keypresscallback) keypresscallback(i, 0);
        //}
    }
    lastKeyDown = lastKeyTime = 0;
}


//
// InitDirectInput() -- get DirectInput started
//

// device enumerator
static BOOL CALLBACK InitDirectInput_enum(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
    const char *d;

    UNREFERENCED_PARAMETER(pvRef);

#define COPYGUID(d,s) memcpy(&d,&s,sizeof(GUID))

    switch (lpddi->dwDevType&0xff)
    {
    case DIDEVTYPE_KEYBOARD:
        inputdevices |= (1<<KEYBOARD);
        d = "KEYBOARD";
        COPYGUID(guidDevs[KEYBOARD],lpddi->guidInstance);
        break;
    case DIDEVTYPE_MOUSE:
        inputdevices |= (1<<MOUSE);
        d = "MOUSE";
        COPYGUID(guidDevs[MOUSE],lpddi->guidInstance);
        break;
    case DIDEVTYPE_JOYSTICK:
    {
        int32_t i;
        inputdevices |= (1<<JOYSTICK);
        joyisgamepad = ((lpddi->dwDevType & (DIDEVTYPEJOYSTICK_GAMEPAD<<8)) != 0);
        d = joyisgamepad ? "GAMEPAD" : "JOYSTICK";
        COPYGUID(guidDevs[JOYSTICK],lpddi->guidInstance);

        thisjoydef = NULL;
        for (i=0; i<(int32_t)(sizeof(joyfeatures)/sizeof(joyfeatures[0])); i++)
        {
            if (lpddi->guidProduct.Data1 == joyfeatures[i].devid)
            {
                thisjoydef = &joyfeatures[i];
                break;
            }
        }

        // Outputs the GUID of the joystick to the console
        /*
        initprintf("GUID = {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n",
        		lpddi->guidProduct.Data1,
        		lpddi->guidProduct.Data2, lpddi->guidProduct.Data3,
        		lpddi->guidProduct.Data4[0], lpddi->guidProduct.Data4[1],
        		lpddi->guidProduct.Data4[2], lpddi->guidProduct.Data4[3],
        		lpddi->guidProduct.Data4[4], lpddi->guidProduct.Data4[5],
        		lpddi->guidProduct.Data4[6], lpddi->guidProduct.Data4[7]
        	);
        */
    }
    break;
    default:
        d = "OTHER"; break;
    }

    initprintf("    * %s: %s\n", d, lpddi->tszProductName);

    return DIENUM_CONTINUE;
}

static const char *joyfindnameforofs(int32_t ofs)
{
    int32_t i;
    if (!thisjoydef) return NULL;
    for (i=0; i<thisjoydef->nfeatures; i++)
    {
        if (ofs == (int32_t)thisjoydef->features[i].ofs)
            return Bstrdup(thisjoydef->features[i].name);
    }
    return NULL;
}

static BOOL CALLBACK InitDirectInput_enumobjects(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
    int32_t *typecounts = (int32_t*)pvRef;

    if (lpddoi->dwType & DIDFT_AXIS)
    {
        //initprintf(" Axis: %s (dwOfs=%d)\n", lpddoi->tszName, lpddoi->dwOfs);

        axisdefs[ typecounts[0] ].name = joyfindnameforofs(lpddoi->dwOfs);
        if (!axisdefs[ typecounts[0] ].name)
            axisdefs[ typecounts[0] ].name = Bstrdup(lpddoi->tszName);
        axisdefs[ typecounts[0] ].ofs = lpddoi->dwOfs;

        typecounts[0]++;
    }
    else if (lpddoi->dwType & DIDFT_BUTTON)
    {
        //initprintf(" Button: %s (dwOfs=%d)\n", lpddoi->tszName, lpddoi->dwOfs);

        buttondefs[ typecounts[1] ].name = joyfindnameforofs(lpddoi->dwOfs);
        if (!buttondefs[ typecounts[1] ].name)
            buttondefs[ typecounts[1] ].name = Bstrdup(lpddoi->tszName);
        buttondefs[ typecounts[1] ].ofs = lpddoi->dwOfs;

        typecounts[1]++;
    }
    else if (lpddoi->dwType & DIDFT_POV)
    {
        //initprintf(" POV: %s (dwOfs=%d)\n", lpddoi->tszName, lpddoi->dwOfs);

        hatdefs[ typecounts[2] ].name = joyfindnameforofs(lpddoi->dwOfs);
        if (!hatdefs[ typecounts[2] ].name)
            hatdefs[ typecounts[2] ].name = Bstrdup(lpddoi->tszName);
        hatdefs[ typecounts[2] ].ofs = lpddoi->dwOfs;

        typecounts[2]++;
    }

    return DIENUM_CONTINUE;
}

#define HorribleDInputDeath( x, y ) \
	ShowDInputErrorBox(x,y); \
	UninitDirectInput(); \
	return TRUE

static BOOL InitDirectInput(void)
{
    HRESULT result;
    HRESULT(WINAPI *aDirectInputCreateA)(HINSTANCE, DWORD, LPDIRECTINPUT7A *, LPUNKNOWN);
    DIPROPDWORD dipdw;
    LPDIRECTINPUTDEVICE7A dev;
    LPDIRECTINPUTDEVICE7A dev2;
    DIDEVCAPS didc;

    int32_t devn;

    if (bDInputInited) return FALSE;

    initprintf("Initializing DirectInput...\n");

    // load up the DirectInput DLL
    if (!hDInputDLL)
    {
//        initprintf("  - Loading DINPUT.DLL\n");
        hDInputDLL = LoadLibrary("DINPUT.DLL");
        if (!hDInputDLL)
        {
            ShowErrorBox("Error loading DINPUT.DLL");
            return TRUE;
        }
    }

    // get the pointer to DirectInputCreate
    aDirectInputCreateA = (void *)GetProcAddress(hDInputDLL, "DirectInputCreateA");
    if (!aDirectInputCreateA) ShowErrorBox("Error fetching DirectInputCreateA()");

    // create a new DirectInput object
//    initprintf("  - Creating DirectInput object\n");
    result = aDirectInputCreateA(hInstance, DIRECTINPUT_VERSION, &lpDI, NULL);
    if FAILED(result) { HorribleDInputDeath("DirectInputCreateA() failed", result); }
    else if (result != DI_OK) initprintf("    Created DirectInput object with warning: %s\n",GetDInputError(result));

    // enumerate devices to make us look fancy
    initprintf("  - Enumerating attached input devices\n");
    inputdevices = 0;
    result = IDirectInput7_EnumDevices(lpDI, 0, InitDirectInput_enum, NULL, DIEDFL_ATTACHEDONLY);
    if FAILED(result) { HorribleDInputDeath("Failed enumerating attached input devices", result); }
    else if (result != DI_OK) initprintf("    Enumerated input devices with warning: %s\n",GetDInputError(result));
    if (!(inputdevices & (1<<KEYBOARD)))
    {
        ShowErrorBox("No keyboard detected!");
        UninitDirectInput();
        return TRUE;
    }

    // ***
    // create the devices
    // ***
    for (devn = 0; devn < NUM_INPUTS; devn++)
    {
        if ((inputdevices & (1<<devn)) == 0) continue;
        *devicedef[devn].did = NULL;

//        initprintf("  - Creating %s device\n", devicedef[devn].name);
        result = IDirectInput7_CreateDeviceEx(lpDI, &guidDevs[devn], &IID_IDirectInputDevice7, (void *)&dev, NULL);
        if FAILED(result) { HorribleDInputDeath("Failed creating device", result); }
        else if (result != DI_OK) initprintf("    Created device with warning: %s\n",GetDInputError(result));

        result = IDirectInputDevice7_QueryInterface(dev, &IID_IDirectInputDevice7, (LPVOID *)&dev2);
        IDirectInputDevice7_Release(dev);
        if FAILED(result) { HorribleDInputDeath("Failed querying DirectInput7 interface for device", result); }
        else if (result != DI_OK) initprintf("    Queried IDirectInputDevice7 interface with warning: %s\n",GetDInputError(result));

        result = IDirectInputDevice7_SetDataFormat(dev2, devicedef[devn].df);
        if FAILED(result) { IDirectInputDevice7_Release(dev2); HorribleDInputDeath("Failed setting data format", result); }
        else if (result != DI_OK) initprintf("    Set data format with warning: %s\n",GetDInputError(result));

        inputevt[devn] = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (inputevt[devn] == NULL)
        {
            IDirectInputDevice7_Release(dev2);
            ShowErrorBox("Couldn't create event object");
            UninitDirectInput();
            return TRUE;
        }

        result = IDirectInputDevice7_SetEventNotification(dev2, inputevt[devn]);
        if FAILED(result) { IDirectInputDevice7_Release(dev2); HorribleDInputDeath("Failed setting event object", result); }
        else if (result != DI_OK) initprintf("    Set event object with warning: %s\n",GetDInputError(result));

        IDirectInputDevice7_Unacquire(dev2);

        memset(&dipdw, 0, sizeof(dipdw));
        dipdw.diph.dwSize = sizeof(DIPROPDWORD);
        dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        dipdw.diph.dwObj = 0;
        dipdw.diph.dwHow = DIPH_DEVICE;
        dipdw.dwData = INPUT_BUFFER_SIZE;

        result = IDirectInputDevice7_SetProperty(dev2, DIPROP_BUFFERSIZE, &dipdw.diph);
        if FAILED(result) { IDirectInputDevice7_Release(dev2); HorribleDInputDeath("Failed setting buffering", result); }
        else if (result != DI_OK) initprintf("    Set buffering with warning: %s\n",GetDInputError(result));

        // set up device
        if (devn == JOYSTICK)
        {
            int32_t typecounts[3] = {0,0,0};

            memset(&didc, 0, sizeof(didc));
            didc.dwSize = sizeof(didc);
            result = IDirectInputDevice7_GetCapabilities(dev2, &didc);
            if FAILED(result) { IDirectInputDevice7_Release(dev2); HorribleDInputDeath("Failed getting joystick capabilities", result); }
            else if (result != DI_OK) initprintf("    Fetched joystick capabilities with warning: %s\n",GetDInputError(result));

            joynumaxes    = (uint8_t)didc.dwAxes;
            joynumbuttons = min(32,(uint8_t)didc.dwButtons);
            joynumhats    = (uint8_t)didc.dwPOVs;
            initprintf("Joystick has %d axes, %d buttons, and %d hat(s).\n",joynumaxes,joynumbuttons,joynumhats);

            axisdefs = (struct _joydef *)Bcalloc(didc.dwAxes, sizeof(struct _joydef));
            buttondefs = (struct _joydef *)Bcalloc(didc.dwButtons, sizeof(struct _joydef));
            hatdefs = (struct _joydef *)Bcalloc(didc.dwPOVs, sizeof(struct _joydef));

            joyaxis = (int32_t *)Bcalloc(didc.dwAxes, sizeof(int32_t));
            joyhat = (int32_t *)Bcalloc(didc.dwPOVs, sizeof(int32_t));

            result = IDirectInputDevice7_EnumObjects(dev2, InitDirectInput_enumobjects, (LPVOID)typecounts, DIDFT_ALL);
            if FAILED(result) { IDirectInputDevice7_Release(dev2); HorribleDInputDeath("Failed getting joystick features", result); }
            else if (result != DI_OK) initprintf("    Fetched joystick features with warning: %s\n",GetDInputError(result));
        }

        *devicedef[devn].did = dev2;
    }

    GetKeyNames();
    memset(devacquired, 0, sizeof(devacquired));

    bDInputInited = TRUE;
    return FALSE;
}


//
// UninitDirectInput() -- clean up DirectInput
//
static void UninitDirectInput(void)
{
    int32_t devn;
    int32_t i;

    if (bDInputInited) initprintf("Uninitializing DirectInput...\n");

    AcquireInputDevices(0,-1);

    if (axisdefs)
    {
        for (i=joynumaxes-1; i>=0; i--) if (axisdefs[i].name) free((void*)axisdefs[i].name);
        free(axisdefs); axisdefs = NULL;
    }
    if (buttondefs)
    {
        for (i=joynumbuttons-1; i>=0; i--) if (buttondefs[i].name) free((void*)buttondefs[i].name);
        free(buttondefs); buttondefs = NULL;
    }
    if (hatdefs)
    {
        for (i=joynumhats-1; i>=0; i--) if (hatdefs[i].name) free((void*)hatdefs[i].name);
        free(hatdefs); hatdefs = NULL;
    }

    for (devn = 0; devn < NUM_INPUTS; devn++)
    {
        if (*devicedef[devn].did)
        {
//            initprintf("  - Releasing %s device\n", devicedef[devn].name);

            if (devn != JOYSTICK) IDirectInputDevice7_SetEventNotification(*devicedef[devn].did, NULL);

            IDirectInputDevice7_Release(*devicedef[devn].did);
            *devicedef[devn].did = NULL;
        }
        if (inputevt[devn])
        {
            CloseHandle(inputevt[devn]);
            inputevt[devn] = NULL;
        }
    }

    if (lpDI)
    {
//        initprintf("  - Releasing DirectInput object\n");
        IDirectInput7_Release(lpDI);
        lpDI = NULL;
    }

    if (hDInputDLL)
    {
//        initprintf("  - Unloading DINPUT.DLL\n");
        FreeLibrary(hDInputDLL);
        hDInputDLL = NULL;
    }

    bDInputInited = FALSE;
}


//
// GetKeyNames() -- retrieves the names for all the keys on the keyboard
//
static void GetKeyNames(void)
{
    int32_t i;
    DIDEVICEOBJECTINSTANCE key;
    HRESULT res;
    char tbuf[MAX_PATH];

    memset(key_names,0,sizeof(key_names));
    for (i=0; i<256; i++)
    {
        ZeroMemory(&key,sizeof(key));
        key.dwSize = sizeof(DIDEVICEOBJECTINSTANCE);

        res = IDirectInputDevice7_GetObjectInfo(*devicedef[KEYBOARD].did, &key, i, DIPH_BYOFFSET);
        if (FAILED(res)) continue;

        CharToOem(key.tszName, tbuf);
        Bstrncpy((char *)key_names[i], tbuf, sizeof(key_names[i])-1);

        tbuf[0] = 0;
        GetKeyNameText((i>128?(i+128):i)<<16, tbuf, sizeof(key_names[i])-1);
//        initprintf("%d %15s  %15s\n",i,key_names[i],tbuf);
        if (*tbuf)Bstrncpy(&key_names[i][0], tbuf, sizeof(key_names[i])-1);
    }
}

const char *getkeyname(int32_t num)
{
    if ((unsigned)num >= 256) return NULL;
    return key_names[num];
}

const char *getjoyname(int32_t what, int32_t num)
{
    switch (what)
    {
    case 0:	// axis
        if ((unsigned)num > (unsigned)joynumaxes) return NULL;
        return (char *)axisdefs[num].name;

    case 1: // button
        if ((unsigned)num > (unsigned)joynumbuttons) return NULL;
        return (char *)buttondefs[num].name;

    case 2: // hat
        if ((unsigned)num > (unsigned)joynumhats) return NULL;
        return (char *)hatdefs[num].name;

    default:
        return NULL;
    }
}

//
// AcquireInputDevices() -- (un)acquires the input devices
//
static void AcquireInputDevices(char acquire, int8_t device)
{
    DWORD flags;
    HRESULT result;
    int32_t i;

    if (!bDInputInited) return;
    if (!hWindow) return;

    if (acquire)
    {
//  	  if (!appactive) return;     // why acquire when inactive?
        for (i=0; i<NUM_INPUTS; i++)
        {
            if (! *devicedef[i].did) continue;
            if (device != -1 && i != device) continue;	// don't touch other devices if only the mouse is wanted
            else if (!mousegrab && i == MOUSE) continue;	// don't grab the mouse if we don't want it grabbed

            IDirectInputDevice7_Unacquire(*devicedef[i].did);

            if (i == MOUSE /*&& fullscreen*/) flags = DISCL_FOREGROUND|DISCL_EXCLUSIVE;
            else flags = DISCL_FOREGROUND|DISCL_NONEXCLUSIVE;

            result = IDirectInputDevice7_SetCooperativeLevel(*devicedef[i].did, hWindow, flags);
            if (FAILED(result))
                initprintf("IDirectInputDevice7_SetCooperativeLevel(%s): %s\n",
                           devicedef[i].name, GetDInputError(result));

            if (SUCCEEDED(IDirectInputDevice7_Acquire(*devicedef[i].did)))
                devacquired[i] = 1;
            else
                devacquired[i] = 0;
        }
    }
    else
    {
        releaseallbuttons();

        for (i=0; i<NUM_INPUTS; i++)
        {
            if (! *devicedef[i].did) continue;
            if (device != -1 && i != device) continue;	// don't touch other devices if only the mouse is wanted

            IDirectInputDevice7_Unacquire(*devicedef[i].did);

            result = IDirectInputDevice7_SetCooperativeLevel(*devicedef[i].did, hWindow, DISCL_FOREGROUND|DISCL_NONEXCLUSIVE);
            if (FAILED(result))
                initprintf("IDirectInputDevice7_SetCooperativeLevel(%s): %s\n",
                           devicedef[i].name, GetDInputError(result));

            devacquired[i] = 0;
        }
    }
}

//
// ProcessInputDevices() -- processes the input devices
//
static inline void ProcessInputDevices(void)
{
    DWORD i;
    HRESULT result;

    DIDEVICEOBJECTDATA didod[INPUT_BUFFER_SIZE];
    DWORD dwElements = INPUT_BUFFER_SIZE;
    DWORD ev;
    uint32_t t,u;
    uint32_t idevnums[NUM_INPUTS], numdevs = 0;
    HANDLE waithnds[NUM_INPUTS];

    for (t = 0; t < NUM_INPUTS; t++)
    {
        if (*devicedef[t].did&&t!=MOUSE)
        {
            result = IDirectInputDevice7_Poll(*devicedef[t].did);
            if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED)
            {
                if (SUCCEEDED(IDirectInputDevice7_Acquire(*devicedef[t].did)))
                {
                    devacquired[t] = 1;
                    IDirectInputDevice7_Poll(*devicedef[t].did);
                }
                else
                {
                    devacquired[t] = 0;
                }
            }

            if (devacquired[t])
            {
                waithnds[numdevs] = inputevt[t];
                idevnums[numdevs] = t;
                numdevs++;
            }
        }
    }

    t = getticks();

    // do this here because we only want the wheel to signal once, but hold the state for a moment
    if (mousegrab)
    {
        if (mousewheel[0] > 0 && t - mousewheel[0] > MouseWheelFakePressTime)
        {
            if (mousepresscallback) mousepresscallback(5,0);
            mousewheel[0] = 0; mouseb &= ~16;
        }
        if (mousewheel[1] > 0 && t - mousewheel[1] > MouseWheelFakePressTime)
        {
            if (mousepresscallback) mousepresscallback(6,0);
            mousewheel[1] = 0; mouseb &= ~32;
        }
    }

    if (numdevs == 0) return;	// nothing to do

    // use event objects so that we can quickly get indication of when data is ready
    // to be read and input events processed
    ev = MsgWaitForMultipleObjects(numdevs, waithnds, FALSE, 0, 0);
    if (/*(ev >= WAIT_OBJECT_0) &&*/ (ev < (WAIT_OBJECT_0+numdevs)))
    {
        switch (idevnums[ev - WAIT_OBJECT_0])
        {
        case KEYBOARD:		// keyboard
            if (!lpDID[KEYBOARD]) break;
            result = IDirectInputDevice7_GetDeviceData(lpDID[KEYBOARD], sizeof(DIDEVICEOBJECTDATA),
                     (LPDIDEVICEOBJECTDATA)&didod, &dwElements, 0);
            if (result == DI_OK)
            {
                DWORD k, numlockon = FALSE;

                numlockon = (GetKeyState(VK_NUMLOCK) & 1);

                // process the key events
                for (i=0; i<dwElements; i++)
                {
                    k = didod[i].dwOfs;

                    if (k == DIK_PAUSE) continue;	// fucking pause

                    //if (IsDebuggerPresent() && k == DIK_F12) continue;

                    // hook in the osd
                    if (OSD_HandleScanCode(k, (didod[i].dwData & 0x80)) != 0)
                    {
                        SetKey(k, (didod[i].dwData & 0x80) == 0x80);

                        if (keypresscallback)
                            keypresscallback(k, (didod[i].dwData & 0x80) == 0x80);
                    }

                    if (((lastKeyDown & 0x7fffffffl) == k) && !(didod[i].dwData & 0x80))
                        lastKeyDown = 0;
                    else if (didod[i].dwData & 0x80)
                    {
                        lastKeyDown = k;
                        lastKeyTime = t;
                    }
                }
            }
            break;

        case JOYSTICK:		// joystick
            if (!lpDID[JOYSTICK]) break;
            result = IDirectInputDevice7_GetDeviceData(lpDID[JOYSTICK], sizeof(DIDEVICEOBJECTDATA),
                     (LPDIDEVICEOBJECTDATA)&didod, &dwElements, 0);
            if (result == DI_OK)
            {
                int32_t j;

                for (i=0; i<dwElements; i++)
                {
                    // check axes
                    for (j=0; j<joynumaxes; j++)
                    {
                        if (axisdefs[j].ofs != didod[i].dwOfs) continue;
                        joyaxis[j] = didod[i].dwData - 32767;
                        break;
                    }
                    if (j<joynumaxes) continue;

                    // check buttons
                    for (j=0; j<joynumbuttons; j++)
                    {
                        if (buttondefs[j].ofs != didod[i].dwOfs) continue;
                        if (didod[i].dwData & 0x80) joyb |= (1<<j);
                        else joyb &= ~(1<<j);
                        if (joypresscallback)
                            joypresscallback(j+1, (didod[i].dwData & 0x80)==0x80);
                        break;
                    }
                    if (j<joynumbuttons) continue;

                    // check hats
                    for (j=0; j<joynumhats; j++)
                    {
                        if (hatdefs[j].ofs != didod[i].dwOfs) continue;
                        joyhat[j] = didod[i].dwData;
                        break;
                    }
                }

                /*
                OSD_Printf("axes:");
                for (i=0;i<joynumaxes;i++) OSD_Printf(" %d", joyaxis[i]);
                OSD_Printf(" - buttons: %x - hats:", joyb);
                for (i=0;i<joynumhats;i++) OSD_Printf(" %d", joyhat[i]);
                OSD_Printf("\n");
                */
            }
            break;
        }
    }

    // key repeat
    // this is like this because the period of t is 1000ms
    if (lastKeyDown > 0)
    {
        u = (1000 + t - lastKeyTime)%1000;
        if ((u >= 250) && !(lastKeyDown&0x80000000l))
        {
            if (OSD_HandleScanCode(lastKeyDown, 1) != 0)
                SetKey(lastKeyDown, 1);
            lastKeyDown |= 0x80000000l;
            lastKeyTime = t;
        }
        else if ((u >= 30) && (lastKeyDown&0x80000000l))
        {
            if (OSD_HandleScanCode(lastKeyDown&(0x7fffffffl), 1) != 0)
                SetKey(lastKeyDown&(0x7fffffffl), 1);
            lastKeyTime = t;
        }
    }
}


//
// ShowDInputErrorBox() -- shows an error message box for a DirectInput error
//
static void ShowDInputErrorBox(const char *m, HRESULT r)
{
    TCHAR msg[1024];

    wsprintf(msg, "%s: %s", m, GetDInputError(r));
    MessageBox(0, msg, apptitle, MB_OK|MB_ICONSTOP);
}


//
// GetDInputError() -- stinking huge list of error messages since MS didn't want to include
//   them in the DLL
//
static const char * GetDInputError(HRESULT code)
{
    switch (code)
    {
    case DI_OK:
        return "DI_OK";
    case DI_BUFFEROVERFLOW:
        return "DI_BUFFEROVERFLOW";
    case DI_DOWNLOADSKIPPED:
        return "DI_DOWNLOADSKIPPED";
    case DI_EFFECTRESTARTED:
        return "DI_EFFECTRESTARTED";
    case DI_POLLEDDEVICE:
        return "DI_POLLEDDEVICE";
    case DI_TRUNCATED:
        return "DI_TRUNCATED";
    case DI_TRUNCATEDANDRESTARTED:
        return "DI_TRUNCATEDANDRESTARTED";
    case DIERR_ACQUIRED:
        return "DIERR_ACQUIRED";
    case DIERR_ALREADYINITIALIZED:
        return "DIERR_ALREADYINITIALIZED";
    case DIERR_BADDRIVERVER:
        return "DIERR_BADDRIVERVER";
    case DIERR_BETADIRECTINPUTVERSION:
        return "DIERR_BETADIRECTINPUTVERSION";
    case DIERR_DEVICEFULL:
        return "DIERR_DEVICEFULL";
    case DIERR_DEVICENOTREG:
        return "DIERR_DEVICENOTREG";
    case DIERR_EFFECTPLAYING:
        return "DIERR_EFFECTPLAYING";
    case DIERR_HASEFFECTS:
        return "DIERR_HASEFFECTS";
    case DIERR_GENERIC:
        return "DIERR_GENERIC";
    case DIERR_HANDLEEXISTS:
        return "DIERR_HANDLEEXISTS";
    case DIERR_INCOMPLETEEFFECT:
        return "DIERR_INCOMPLETEEFFECT";
    case DIERR_INPUTLOST:
        return "DIERR_INPUTLOST";
    case DIERR_INVALIDPARAM:
        return "DIERR_INVALIDPARAM";
    case DIERR_MOREDATA:
        return "DIERR_MOREDATA";
    case DIERR_NOAGGREGATION:
        return "DIERR_NOAGGREGATION";
    case DIERR_NOINTERFACE:
        return "DIERR_NOINTERFACE";
    case DIERR_NOTACQUIRED:
        return "DIERR_NOTACQUIRED";
    case DIERR_NOTBUFFERED:
        return "DIERR_NOTBUFFERED";
    case DIERR_NOTDOWNLOADED:
        return "DIERR_NOTDOWNLOADED";
    case DIERR_NOTEXCLUSIVEACQUIRED:
        return "DIERR_NOTEXCLUSIVEACQUIRED";
    case DIERR_NOTFOUND:
        return "DIERR_NOTFOUND";
    case DIERR_NOTINITIALIZED:
        return "DIERR_NOTINITIALIZED";
    case DIERR_OLDDIRECTINPUTVERSION:
        return "DIERR_OLDDIRECTINPUTVERSION";
    case DIERR_OUTOFMEMORY:
        return "DIERR_OUTOFMEMORY";
    case DIERR_UNSUPPORTED:
        return "DIERR_UNSUPPORTED";
    case E_PENDING:
        return "E_PENDING";
    default:
        break;
    }
    return "Unknown error";
}




//-------------------------------------------------------------------------------------------------
//  TIMER
//=================================================================================================

static int64 timerfreq=0;
static int32_t timerlastsample=0;
int32_t timerticspersec=0;
static void (*usertimercallback)(void) = NULL;

//  This timer stuff is all Ken's idea.

//
// installusertimercallback() -- set up a callback function to be called when the timer is fired
//
void (*installusertimercallback(void (*callback)(void)))(void)
{
    void (*oldtimercallback)(void);

    oldtimercallback = usertimercallback;
    usertimercallback = callback;

    return oldtimercallback;
}


//
// inittimer() -- initialize timer
//
int32_t inittimer(int32_t tickspersecond)
{
    int64 t;

    if (timerfreq) return 0;	// already installed

//    initprintf("Initializing timer\n");

    // OpenWatcom seems to want us to query the value into a local variable
    // instead of the global 'timerfreq' or else it gets pissed with an
    // access violation
    if (!QueryPerformanceFrequency((LARGE_INTEGER*)&t))
    {
        ShowErrorBox("Failed fetching timer frequency");
        return -1;
    }
    timerfreq = t;
    timerticspersec = tickspersecond;
    QueryPerformanceCounter((LARGE_INTEGER*)&t);
    timerlastsample = (int32_t)(t*timerticspersec / timerfreq);

    usertimercallback = NULL;

    return 0;
}

//
// uninittimer() -- shut down timer
//
void uninittimer(void)
{
    if (!timerfreq) return;

    timerfreq=0;
    timerticspersec = 0;
}

//
// sampletimer() -- update totalclock
//
inline void sampletimer(void)
{
    int64 i;
    int32_t n;

    if (!timerfreq) return;

    QueryPerformanceCounter((LARGE_INTEGER*)&i);
    n = (int32_t)(i*timerticspersec / timerfreq) - timerlastsample;
    if (n>0)
    {
        totalclock += n;
        timerlastsample += n;
    }

    if (usertimercallback) for (; n>0; n--) usertimercallback();
}


//
// getticks() -- returns the windows ticks count
//
uint32_t getticks(void)
{
    int64 i;
    if (timerfreq == 0) return 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&i);
    return (uint32_t)(i*longlong(1000)/timerfreq);
}


//
// gettimerfreq() -- returns the number of ticks per second the timer is configured to generate
//
int32_t gettimerfreq(void)
{
    return timerticspersec;
}




//-------------------------------------------------------------------------------------------------
//  VIDEO
//=================================================================================================

// DirectDraw objects
static HMODULE              hDDrawDLL      = NULL;
static LPDIRECTDRAW         lpDD           = NULL;
static LPDIRECTDRAWSURFACE  lpDDSPrimary   = NULL;
static LPDIRECTDRAWSURFACE  lpDDSBack      = NULL;
static char *               lpOffscreen = NULL;
static LPDIRECTDRAWPALETTE  lpDDPalette = NULL;
static BOOL                 bDDrawInited = FALSE;
static DWORD                DDdwCaps = 0, DDdwCaps2 = 0;

// DIB stuff
static HDC      hDC         = NULL;	// opengl shares this
static HDC      hDCSection  = NULL;
static HBITMAP  hDIBSection = NULL;
static HPALETTE hPalette    = NULL;
static VOID    *lpPixels    = NULL;

#define NUM_SYS_COLOURS	25
static int32_t syscolouridx[NUM_SYS_COLOURS] =
{
    COLOR_SCROLLBAR,		// 1
    COLOR_BACKGROUND,
    COLOR_ACTIVECAPTION,
    COLOR_INACTIVECAPTION,
    COLOR_MENU,
    COLOR_WINDOW,
    COLOR_WINDOWFRAME,
    COLOR_MENUTEXT,
    COLOR_WINDOWTEXT,
    COLOR_CAPTIONTEXT,		// 10
    COLOR_ACTIVEBORDER,
    COLOR_INACTIVEBORDER,
    COLOR_APPWORKSPACE,
    COLOR_HIGHLIGHT,
    COLOR_HIGHLIGHTTEXT,
    COLOR_BTNFACE,
    COLOR_BTNSHADOW,
    COLOR_GRAYTEXT,
    COLOR_BTNTEXT,
    COLOR_INACTIVECAPTIONTEXT,	// 20
    COLOR_BTNHIGHLIGHT,
    COLOR_3DDKSHADOW,
    COLOR_3DLIGHT,
    COLOR_INFOTEXT,
    COLOR_INFOBK			// 25
};
static DWORD syscolours[NUM_SYS_COLOURS];
static char system_colours_saved = 0, bw_colours_set = 0;

static int32_t setgammaramp(WORD gt[3][256]);
static int32_t getgammaramp(WORD gt[3][256]);

//
// checkvideomode() -- makes sure the video mode passed is legal
//
int32_t checkvideomode(int32_t *x, int32_t *y, int32_t c, int32_t fs, int32_t forced)
{
    int32_t i, nearest=-1, dx, dy, odx=9999, ody=9999;

    getvalidmodes();

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
        if (!(dx | dy))   	// perfect match
        {
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
    if (!forced && (fs&1) == 0 && (nearest < 0 || validmode[nearest].xdim!=*x || validmode[nearest].ydim!=*y))
    {
        // check the colour depth is recognised at the very least
        for (i=0; i<validmodecnt; i++)
            if (validmode[i].bpp == c)
                return 0x7fffffffl;
        return -1;	// strange colour depth
    }
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


//
// setvideomode() -- set the video mode
//

#if defined(USE_OPENGL) && defined(POLYMOST)
static HWND hGLWindow = NULL;
#endif

int32_t setvideomode(int32_t x, int32_t y, int32_t c, int32_t fs)
{
    char i,inp[NUM_INPUTS];
    int32_t modenum;

    if ((fs == fullscreen) && (x == xres) && (y == yres) && (c == bpp) && !videomodereset)
    {
        OSD_ResizeDisplay(xres,yres);
        return 0;
    }

    modenum = checkvideomode(&x,&y,c,fs,0);
    if (modenum < 0) return -1;
    if (modenum == 0x7fffffff)
    {
        customxdim = x;
        customydim = y;
        custombpp  = c;
        customfs   = fs;
    }

    for (i=0; i<NUM_INPUTS; i++) inp[i] = devacquired[i];
    AcquireInputDevices(0,-1);

    if (hWindow && gammabrightness)
    {
        setgammaramp(sysgamma);
        gammabrightness = 0;
    }

    if (!silentvideomodeswitch)
        initprintf("Setting video mode %dx%d (%d-bit %s)\n",
                   x,y,c, ((fs&1) ? "fullscreen" : "windowed"));

    if (CreateAppWindow(modenum)) return -1;

    if (!gammabrightness)
    {
//        float f = 1.0 + ((float)curbrightness / 10.0);
        if (getgammaramp(sysgamma) >= 0) gammabrightness = 1;
        if (gammabrightness && setgamma() < 0) gammabrightness = 0;
    }

#if defined(USE_OPENGL) && defined(POLYMOST)
    if (hGLWindow && glinfo.vsync) bwglSwapIntervalEXT(vsync);
#endif
    for (i=0; i<NUM_INPUTS; i++) if (inp[i]) AcquireInputDevices(1,i);
    modechange=1;
    videomodereset = 0;
    OSD_ResizeDisplay(xres,yres);
    //baselayer_onvideomodechange(c>8);

    return 0;
}


//
// getvalidmodes() -- figure out what video modes are available
//
#define ADDMODE(x,y,c,f,n) if (validmodecnt<MAXVALIDMODES) { \
	validmode[validmodecnt].xdim=x; \
	validmode[validmodecnt].ydim=y; \
	validmode[validmodecnt].bpp=c; \
	validmode[validmodecnt].fs=f; \
	validmode[validmodecnt].extra=n; \
	validmodecnt++; \
    }
/*	initprintf("  - %dx%d %d-bit %s\n", x, y, c, (f&1)?"fullscreen":"windowed"); \
	} */

#define CHECK(w,h) if ((w < maxx) && (h < maxy))

#if defined(USE_OPENGL) && defined(POLYMOST)
void setvsync(int32_t sync)
{
    if (!glinfo.vsync)
    {
        vsync = 0;
        return;
    }
    vsync = sync;
    bwglSwapIntervalEXT(sync);
}

static void cdsenummodes(void)
{
    DEVMODE dm;
    int32_t i = 0, j = 0;

    struct { uint32_t x,y,bpp,freq; } modes[MAXVALIDMODES];
    int32_t nmodes=0;
    uint32_t maxx = MAXXDIM, maxy = MAXYDIM;


    ZeroMemory(&dm,sizeof(DEVMODE));
    dm.dmSize = sizeof(DEVMODE);
    while (EnumDisplaySettings(NULL, j, &dm))
    {
        if (dm.dmBitsPerPel > 8)
        {
            for (i=0; i<nmodes; i++)
            {
                if (modes[i].x == dm.dmPelsWidth
                        && modes[i].y == dm.dmPelsHeight
                        && modes[i].bpp == dm.dmBitsPerPel)
                    break;
            }
            if ((i==nmodes) ||
                    (dm.dmDisplayFrequency <= maxrefreshfreq && dm.dmDisplayFrequency > modes[i].freq && maxrefreshfreq > 0) ||
                    (dm.dmDisplayFrequency > modes[i].freq && maxrefreshfreq == 0))
            {
                if (i==nmodes) nmodes++;

                modes[i].x = dm.dmPelsWidth;
                modes[i].y = dm.dmPelsHeight;
                modes[i].bpp = dm.dmBitsPerPel;
                modes[i].freq = dm.dmDisplayFrequency;
            }
        }

        j++;
        ZeroMemory(&dm,sizeof(DEVMODE));
        dm.dmSize = sizeof(DEVMODE);
    }

    for (i=0; i<nmodes; i++)
    {
        CHECK(modes[i].x, modes[i].y)
        ADDMODE(modes[i].x, modes[i].y, modes[i].bpp, 1, modes[i].freq);
    }
}
#endif

// mode enumerator
static HRESULT WINAPI getvalidmodes_enum(DDSURFACEDESC *ddsd, VOID *udata)
{
    uint32_t maxx = MAXXDIM, maxy = MAXYDIM;

    UNREFERENCED_PARAMETER(udata);

    if (ddsd->ddpfPixelFormat.dwRGBBitCount == 8)
    {
        CHECK(ddsd->dwWidth, ddsd->dwHeight)
        ADDMODE(ddsd->dwWidth, ddsd->dwHeight, ddsd->ddpfPixelFormat.dwRGBBitCount, 1,-1);
    }

    return(DDENUMRET_OK);
}

static int32_t sortmodes(const struct validmode_t *a, const struct validmode_t *b)
{
    int32_t x;

    if ((x = a->fs   - b->fs)   != 0) return x;
    if ((x = a->bpp  - b->bpp)  != 0) return x;
    if ((x = a->xdim - b->xdim) != 0) return x;
    if ((x = a->ydim - b->ydim) != 0) return x;

    return 0;
}
void getvalidmodes(void)
{
    static int32_t defaultres[][2] =
    {
        {1920,1440},{1920,1200},{1600,1200},{1280,1024},{1280,960},{1152,864},{1024,768},{1024,600},{800,600},{640,480},
        {640,400},{512,384},{480,360},{400,300},{320,240},{320,200},{0,0}
    };
    int32_t cdepths[2] = { 8, 0 };
    int32_t i, j, maxx=0, maxy=0;
    HRESULT result;

#if defined(USE_OPENGL) && defined(POLYMOST)
    if (desktopbpp > 8 && !nogl) cdepths[1] = desktopbpp;
    else cdepths[1] = 0;
#endif

    if (modeschecked) return;

    validmodecnt=0;
//    initprintf("Detecting video modes:\n");

    if (bDDrawInited)
    {
        // if DirectDraw initialization didn't fail enumerate fullscreen modes

        result = IDirectDraw_EnumDisplayModes(lpDD, 0, NULL, 0, getvalidmodes_enum);
        if (result != DD_OK)
        {
            initprintf("Unable to enumerate fullscreen modes. Using default list.\n");
            for (j=0; j < 2; j++)
            {
                if (cdepths[j] == 0) continue;
                for (i=0; defaultres[i][0]; i++)
                    ADDMODE(defaultres[i][0],defaultres[i][1],cdepths[j],1,-1)
                }
        }
    }
#if defined(USE_OPENGL) && defined(POLYMOST)
    cdsenummodes();
#endif

    // windowed modes cant be bigger than the current desktop resolution
    maxx = desktopxdim-1;
    maxy = desktopydim-1;

    // add windowed modes next
    for (j=0; j < 2; j++)
    {
        if (cdepths[j] == 0) continue;
        for (i=0; defaultres[i][0]; i++)
            CHECK(defaultres[i][0],defaultres[i][1])
            ADDMODE(defaultres[i][0],defaultres[i][1],cdepths[j],0,-1)
        }

    qsort((void*)validmode, validmodecnt, sizeof(struct validmode_t), (int32_t(*)(const void*,const void*))sortmodes);

    modeschecked=1;
}

#undef CHECK
#undef ADDMODE


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
    int32_t i,j;

    if (bpp > 8)
    {
        if (offscreenrendering) return;
        frameplace = 0;
        bytesperline = 0;
        imageSize = 0;
        modechange = 0;
        return;
    }

    if (lockcount++ > 0)
        return;		// already locked

    if (offscreenrendering) return;

    if (!fullscreen)
    {
        frameplace = (intptr_t)lpPixels;
    }
    else
    {
        frameplace = (intptr_t)lpOffscreen;
    }

    if (!modechange) return;

    if (!fullscreen)
    {
        bytesperline = xres|4;
    }
    else
    {
        bytesperline = xres|1;
    }

    imageSize = bytesperline*yres;
    setvlinebpl(bytesperline);

    j = 0;
    for (i=0; i<=ydim; i++) ylookup[i] = j, j += bytesperline;
    modechange=0;
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
    lockcount = 0;
}


//
// showframe() -- update the display
//
void showframe(int32_t w)
{
    HRESULT result;
    DDSURFACEDESC ddsd;
    char *p,*q;
    int32_t i,j;

#if defined(USE_OPENGL) && defined(POLYMOST)
    if (bpp > 8)
    {
        if (palfadedelta)
        {
            bglMatrixMode(GL_PROJECTION);
            bglPushMatrix();
            bglLoadIdentity();
            bglMatrixMode(GL_MODELVIEW);
            bglPushMatrix();
            bglLoadIdentity();

            bglDisable(GL_DEPTH_TEST);
            bglDisable(GL_ALPHA_TEST);
            bglDisable(GL_TEXTURE_2D);

            bglEnable(GL_BLEND);
            bglColor4ub(palfadergb.r, palfadergb.g, palfadergb.b, palfadedelta);

            bglBegin(GL_QUADS);
            bglVertex2i(-1, -1);
            bglVertex2i(1, -1);
            bglVertex2i(1, 1);
            bglVertex2i(-1, 1);
            bglEnd();

            bglDisable(GL_BLEND);

            bglMatrixMode(GL_MODELVIEW);
            bglPopMatrix();
            bglMatrixMode(GL_PROJECTION);
            bglPopMatrix();
        }

        bwglSwapBuffers(hDC);
        return;
    }
#endif

    w = 1;	// wait regardless. ken thinks it's better to do so.

    if (offscreenrendering) return;

    if (lockcount)
    {
        initprintf("Frame still locked %d times when showframe() called.\n", lockcount);
        while (lockcount) enddrawing();
    }

    if (!fullscreen)
    {
        BitBlt(hDC, 0, 0, xres, yres, hDCSection, 0, 0, SRCCOPY);
    }
    else
    {
        if (!w)
        {
            if ((result = IDirectDrawSurface_GetBltStatus(lpDDSBack, DDGBS_CANBLT)) == DDERR_WASSTILLDRAWING)
                return;

            if ((result = IDirectDrawSurface_GetFlipStatus(lpDDSPrimary, DDGFS_CANFLIP)) == DDERR_WASSTILLDRAWING)
                return;
        }

        // lock the backbuffer surface
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);

        result = IDirectDrawSurface_Lock(lpDDSBack, NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_NOSYSLOCK | DDLOCK_WAIT, NULL);
        if (result == DDERR_SURFACELOST)
        {
            if (!appactive)
                return;	// not in a position to restore display anyway

            IDirectDrawSurface_Restore(lpDDSPrimary);
            result = IDirectDrawSurface_Lock(lpDDSBack, NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_NOSYSLOCK | DDLOCK_WAIT, NULL);
        }
        if (result != DD_OK)
        {
            if (result != DDERR_WASSTILLDRAWING)
                initprintf("Failed locking back-buffer surface: %s\n", GetDDrawError(result));
            return;
        }

        // copy each scanline
        p = (char *)ddsd.lpSurface;
        q = (char *)lpOffscreen;
        j = xres >> 2;

        for (i=0; i<yres; i++, p+=ddsd.lPitch, q+=bytesperline)
            copybuf(q,p,j);

        // unlock the backbuffer surface
        result = IDirectDrawSurface_Unlock(lpDDSBack, NULL);
        if (result != DD_OK)
        {
            initprintf("Failed unlocking back-buffer surface: %s\n", GetDDrawError(result));
            return;
        }

        // flip the chain
        result = IDirectDrawSurface_Flip(lpDDSPrimary, NULL, w?DDFLIP_WAIT:0);
        if (result == DDERR_SURFACELOST)
        {
            if (!appactive)
                return;	// not in a position to restore display anyway
            IDirectDrawSurface_Restore(lpDDSPrimary);
            result = IDirectDrawSurface_Flip(lpDDSPrimary, NULL, w?DDFLIP_WAIT:0);
        }
        if (result != DD_OK)
        {
            if (result != DDERR_WASSTILLDRAWING)
                initprintf("IDirectDrawSurface_Flip(): %s\n", GetDDrawError(result));
        }
    }
}


//
// setpalette() -- set palette values
// New behaviour: curpalettefaded is the live palette, and any changes this function
// makes are done to it and not the base palette.
//
int32_t setpalette(int32_t start, int32_t num)
{
    int32_t i, n;
    HRESULT result;
    RGBQUAD *rgb;
    //HPALETTE hPalPrev;

    static struct logpal
    {
        WORD palVersion;
        WORD palNumEntries;
        PALETTEENTRY palPalEntry[256];
    } lpal;

//    copybufbyte(curpalettefaded, lpal.palPalEntry, 256);
    Bmemcpy(lpal.palPalEntry, curpalettefaded, sizeof(lpal.palPalEntry));
    for (i=start, n=num-1; n>0; i++, n--)
        curpalettefaded[i].f = lpal.palPalEntry[i].peFlags = PC_NOCOLLAPSE;

    if (bpp > 8) return 0;	// no palette in opengl

    if (!fullscreen)
    {
        if (num > 0)
        {
            rgb = (RGBQUAD *)Bmalloc(sizeof(RGBQUAD)*num);
            for (i=start, n=0; n<num; i++, n++)
            {
                rgb[n].rgbBlue = lpal.palPalEntry[i].peBlue;
                rgb[n].rgbGreen = lpal.palPalEntry[i].peGreen;
                rgb[n].rgbRed = lpal.palPalEntry[i].peRed;
                rgb[n].rgbReserved = 0;
            }

            SetDIBColorTable(hDCSection, start, num, rgb);
            free(rgb);
        }

        if (desktopbpp > 8) return 0;	// only if an 8bit desktop do we do what follows

        // set 0 and 255 to black and white
        lpal.palVersion = 0x300;
        lpal.palNumEntries = 256;
        lpal.palPalEntry[0].peBlue = 0;
        lpal.palPalEntry[0].peGreen = 0;
        lpal.palPalEntry[0].peRed = 0;
        lpal.palPalEntry[0].peFlags = 0;
        lpal.palPalEntry[255].peBlue = 255;
        lpal.palPalEntry[255].peGreen = 255;
        lpal.palPalEntry[255].peRed = 255;
        lpal.palPalEntry[255].peFlags = 0;

        if (SetSystemPaletteUse(hDC, SYSPAL_NOSTATIC) == SYSPAL_ERROR)
        {
            initprintf("Problem setting system palette use.\n");
            return -1;
        }

        if (hPalette)
        {
            if (num == 0) { start = 0; num = 256; }		// refreshing the palette only
            SetPaletteEntries(hPalette, start, num, lpal.palPalEntry+start);
        }
        else
        {
            hPalette = CreatePalette((LOGPALETTE *)lpal.palPalEntry);
            if (!hPalette)
            {
                initprintf("Problem creating palette.\n");
                return -1;
            }
        }

        if (SelectPalette(hDC, hPalette, FALSE) == NULL)
        {
            initprintf("Problem selecting palette.\n");
            return -1;
        }

        if (RealizePalette(hDC) == GDI_ERROR)
        {
            initprintf("Failure realizing palette.\n");
            return -1;
        }

        SetBWSystemColours();
    }
    else
    {
        if (!lpDDPalette) return -1;
        result = IDirectDrawPalette_SetEntries(lpDDPalette, 0, 0, 256, (LPPALETTEENTRY)lpal.palPalEntry);
        if (result != DD_OK)
        {
            initprintf("Palette set failed: %s\n", GetDDrawError(result));
            return -1;
        }
    }

    return 0;
}

//
// getpalette() -- get palette values
//
/*
int32_t getpalette(int32_t start, int32_t num, char *dapal)
{
	int32_t i;

	for (i=num; i>0; i--, start++) {
		dapal[0] = curpalette[start].b >> 2;
		dapal[1] = curpalette[start].g >> 2;
		dapal[2] = curpalette[start].r >> 2;
		dapal += 4;
	}

	return 0;
}*/


//
// setgamma
//
static int32_t setgammaramp(WORD gt[3][256])
{
    if (!fullscreen || bpp > 8)
    {
        // GL and windowed mode use DIB method
        int32_t i;
        HDC hDC = GetDC(hWindow);
        i = SetDeviceGammaRamp(hDC, gt) ? 0 : -1;
        ReleaseDC(hWindow, hDC);
        return i;
    }
    else if (appactive)
    {
        // fullscreen uses DirectX
        LPDIRECTDRAWGAMMACONTROL gam;
        HRESULT hr;

        if (!(DDdwCaps2 & DDCAPS2_PRIMARYGAMMA)) return -1;

        hr = IDirectDrawSurface_QueryInterface(lpDDSPrimary, &IID_IDirectDrawGammaControl, (LPVOID)&gam);
        if (hr != DD_OK)
        {
//            ShowDDrawErrorBox("Error querying gamma control", hr);
            initprintf("Error querying gamma control: %s\n",GetDDrawError(hr));
            return -1;
        }

        hr = IDirectDrawGammaControl_SetGammaRamp(gam, 0, (LPDDGAMMARAMP)gt);
        if (hr != DD_OK)
        {
            IDirectDrawGammaControl_Release(gam);
            initprintf("Error setting gamma ramp: %s\n",GetDDrawError(hr));
//            ShowDDrawErrorBox("Error setting gamma ramp", hr);
            return -1;
        }

        IDirectDrawGammaControl_Release(gam);
        return 0;
    }
    else return 0;
}

int32_t setgamma(void)
{
    int32_t i;
    WORD gammaTable[3][256];
    float gamma = max(0.1f,min(4.f,vid_gamma));
    float contrast = max(0.1f,min(3.f,vid_contrast));
    float bright = max(-0.8f,min(0.8f,vid_brightness));

    double invgamma = 1 / gamma;
    double norm = pow(255., invgamma - 1);

    if (!hWindow) return -1;

    // This formula is taken from Doomsday

    for (i = 0; i < 256; i++)
    {
        double val = i * contrast - (contrast - 1) * 127;
        if (gamma != 1) val = pow(val, invgamma) / norm;
        val += bright * 128;

        gammaTable[0][i] = gammaTable[1][i] = gammaTable[2][i] = (WORD)max(0.f,(double)min(0xffff,val*256));
    }
    return setgammaramp(gammaTable);
}

static int32_t getgammaramp(WORD gt[3][256])
{
    if (!hWindow) return -1;
    if (!fullscreen || bpp > 8)
    {
        int32_t i;
        HDC hDC = GetDC(hWindow);
        i = GetDeviceGammaRamp(hDC, gt) ? 0 : -1;
        ReleaseDC(hWindow, hDC);
        return i;
    }
    else
    {
        LPDIRECTDRAWGAMMACONTROL gam;
        HRESULT hr;

        if (!(DDdwCaps2 & DDCAPS2_PRIMARYGAMMA)) return -1;

        hr = IDirectDrawSurface_QueryInterface(lpDDSPrimary, &IID_IDirectDrawGammaControl, (LPVOID)&gam);
        if (hr != DD_OK)
        {
            ShowDDrawErrorBox("Error querying gamma control", hr);
            return -1;
        }

        hr = IDirectDrawGammaControl_GetGammaRamp(gam, 0, (LPDDGAMMARAMP)gt);
        if (hr != DD_OK)
        {
            IDirectDrawGammaControl_Release(gam);
            ShowDDrawErrorBox("Error getting gamma ramp", hr);
            return -1;
        }

        IDirectDrawGammaControl_Release(gam);

        return 0;
    }
}

//
// InitDirectDraw() -- get DirectDraw started
//

// device enumerator
static BOOL WINAPI InitDirectDraw_enum(GUID *lpGUID, LPSTR lpDesc, LPSTR lpName, LPVOID lpContext)
{
    UNREFERENCED_PARAMETER(lpGUID);
    UNREFERENCED_PARAMETER(lpName);
    UNREFERENCED_PARAMETER(lpContext);
    UNREFERENCED_PARAMETER(lpDesc);
//    initprintf("    * %s\n", lpDesc);
    return 1;
}

static BOOL InitDirectDraw(void)
{
    HRESULT result;
    HRESULT(WINAPI *aDirectDrawCreate)(GUID *, LPDIRECTDRAW *, IUnknown *);
    HRESULT(WINAPI *aDirectDrawEnumerate)(LPDDENUMCALLBACK, LPVOID);
    DDCAPS ddcaps;

    if (bDDrawInited) return FALSE;

    initprintf("Initializing DirectDraw...\n");

    // load up the DirectDraw DLL
    if (!hDDrawDLL)
    {
//        initprintf("  - Loading DDRAW.DLL\n");
        hDDrawDLL = LoadLibrary("DDRAW.DLL");
        if (!hDDrawDLL)
        {
            ShowErrorBox("Error loading DDRAW.DLL");
            return TRUE;
        }
    }

    // get the pointer to DirectDrawEnumerate
    aDirectDrawEnumerate = (void *)GetProcAddress(hDDrawDLL, "DirectDrawEnumerateA");
    if (!aDirectDrawEnumerate)
    {
        ShowErrorBox("Error fetching DirectDrawEnumerate()");
        UninitDirectDraw();
        return TRUE;
    }

    // enumerate the devices to make us look fancy
//    initprintf("  - Enumerating display devices\n");
    aDirectDrawEnumerate(InitDirectDraw_enum, NULL);

    // get the pointer to DirectDrawCreate
    aDirectDrawCreate = (void *)GetProcAddress(hDDrawDLL, "DirectDrawCreate");
    if (!aDirectDrawCreate)
    {
        ShowErrorBox("Error fetching DirectDrawCreate()");
        UninitDirectDraw();
        return TRUE;
    }

    // create a new DirectDraw object
//    initprintf("  - Creating DirectDraw object\n");
    result = aDirectDrawCreate(NULL, &lpDD, NULL);
    if (result != DD_OK)
    {
        ShowDDrawErrorBox("DirectDrawCreate() failed", result);
        UninitDirectDraw();
        return TRUE;
    }

    // fetch capabilities
//    initprintf("  - Checking capabilities\n");
    ddcaps.dwSize = sizeof(DDCAPS);
    result = IDirectDraw_GetCaps(lpDD, &ddcaps, NULL);
    if (result != DD_OK)
    {
        initprintf("    Unable to get capabilities.\n");
    }
    else
    {
        DDdwCaps = ddcaps.dwCaps;
        DDdwCaps2 = ddcaps.dwCaps2;
    }

    bDDrawInited = TRUE;

    return FALSE;
}


//
// UninitDirectDraw() -- clean up DirectDraw
//
static void UninitDirectDraw(void)
{
    if (bDDrawInited) initprintf("Uninitializing DirectDraw...\n");

    ReleaseDirectDrawSurfaces();

    RestoreDirectDrawMode();

    if (lpDD)
    {
//        initprintf("  - Releasing DirectDraw object\n");
        IDirectDraw_Release(lpDD);
        lpDD = NULL;
    }

    if (hDDrawDLL)
    {
//        initprintf("  - Unloading DDRAW.DLL\n");
        FreeLibrary(hDDrawDLL);
        hDDrawDLL = NULL;
    }

    bDDrawInited = FALSE;
}


//
// RestoreDirectDrawMode() -- resets the screen mode
//
static int32_t RestoreDirectDrawMode(void)
{
    HRESULT result;

    if (fullscreen == 0 || /*bpp > 8 ||*/ !bDDrawInited) return FALSE;

    if (modesetusing == 1) ChangeDisplaySettings(NULL,0);
    else if (modesetusing == 0)
    {
        // restore previous display mode and set to normal cooperative level
        result = IDirectDraw_RestoreDisplayMode(lpDD);
        if (result != DD_OK)
        {
            ShowDDrawErrorBox("Error restoring display mode", result);
            UninitDirectDraw();
            return TRUE;
        }

        result = IDirectDraw_SetCooperativeLevel(lpDD, hWindow, DDSCL_NORMAL);
        if (result != DD_OK)
        {
            ShowDDrawErrorBox("Error setting cooperative level", result);
            UninitDirectDraw();
            return TRUE;
        }
    }
    modesetusing = -1;

    return FALSE;
}


//
// ReleaseDirectDrawSurfaces() -- release the front and back buffers
//
static void ReleaseDirectDrawSurfaces(void)
{
    if (lpDDPalette)
    {
//        initprintf("  - Releasing palette\n");
        IDirectDrawPalette_Release(lpDDPalette);
        lpDDPalette = NULL;
    }

    if (lpDDSBack)
    {
//        initprintf("  - Releasing back-buffer surface\n");
        IDirectDrawSurface_Release(lpDDSBack);
        lpDDSBack = NULL;
    }

    if (lpDDSPrimary)
    {
//        initprintf("  - Releasing primary surface\n");
        IDirectDrawSurface_Release(lpDDSPrimary);
        lpDDSPrimary = NULL;
    }

    if (lpOffscreen)
    {
//        initprintf("  - Freeing offscreen buffer\n");
        free(lpOffscreen);
        lpOffscreen = NULL;
    }
}


//
// SetupDirectDraw() -- sets up DirectDraw rendering
//
static int32_t SetupDirectDraw(int32_t width, int32_t height)
{
    HRESULT result;
    DDSURFACEDESC ddsd;
    int32_t i;

    // now create the DirectDraw surfaces
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
    ddsd.dwBackBufferCount = 2;	// triple-buffer

//    initprintf("  - Creating primary surface\n");
    result = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpDDSPrimary, NULL);
    if (result != DD_OK)
    {
        ShowDDrawErrorBox("Failure creating primary surface", result);
        UninitDirectDraw();
        return TRUE;
    }

    ZeroMemory(&ddsd.ddsCaps, sizeof(ddsd.ddsCaps));
    ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
    numpages = 1;	// KJS 20031225

//    initprintf("  - Getting back buffer\n");
    result = IDirectDrawSurface_GetAttachedSurface(lpDDSPrimary, &ddsd.ddsCaps, &lpDDSBack);
    if (result != DD_OK)
    {
        ShowDDrawErrorBox("Failure fetching back-buffer surface", result);
        UninitDirectDraw();
        return TRUE;
    }

//    initprintf("  - Allocating offscreen buffer\n");
    lpOffscreen = (char *)malloc((width|1)*height);
    if (!lpOffscreen)
    {
        ShowErrorBox("Failure allocating offscreen buffer");
        UninitDirectDraw();
        return TRUE;
    }

    // attach a palette to the primary surface
//    initprintf("  - Creating palette\n");
    for (i=0; i<256; i++)
        curpalettefaded[i].f = PC_NOCOLLAPSE;
    result = IDirectDraw_CreatePalette(lpDD, DDPCAPS_8BIT | DDPCAPS_ALLOW256, (LPPALETTEENTRY)curpalettefaded, &lpDDPalette, NULL);
    if (result != DD_OK)
    {
        ShowDDrawErrorBox("Failure creating palette", result);
        UninitDirectDraw();
        return TRUE;
    }

    result = IDirectDrawSurface_SetPalette(lpDDSPrimary, lpDDPalette);
    if (result != DD_OK)
    {
        ShowDDrawErrorBox("Failure setting palette", result);
        UninitDirectDraw();
        return TRUE;
    }

    return FALSE;
}


//
// UninitDIB() -- clean up the DIB renderer
//
static void UninitDIB(void)
{
    if (desktopbpp <= 8)
        RestoreSystemColours();

    if (hPalette)
    {
        DeleteObject(hPalette);
        hPalette = NULL;
    }

    if (hDCSection)
    {
        DeleteDC(hDCSection);
        hDCSection = NULL;
    }

    if (hDIBSection)
    {
        DeleteObject(hDIBSection);
        hDIBSection = NULL;
    }

    if (hDC)
    {
        ReleaseDC(hWindow, hDC);
        hDC = NULL;
    }
}


//
// SetupDIB() -- sets up DIB rendering
//
static int32_t SetupDIB(int32_t width, int32_t height)
{
    struct binfo
    {
        BITMAPINFOHEADER header;
        RGBQUAD colours[256];
    } dibsect;
    int32_t i;

    if (!hDC)
    {
        hDC = GetDC(hWindow);
        if (!hDC)
        {
            ShowErrorBox("Error getting device context");
            return TRUE;
        }
    }

    if (hDCSection)
    {
        DeleteDC(hDCSection);
        hDCSection = NULL;
    }

    // destroy the previous DIB section if it existed
    if (hDIBSection)
    {
        DeleteObject(hDIBSection);
        hDIBSection = NULL;
    }

    // create the new DIB section
    memset(&dibsect, 0, sizeof(dibsect));
    numpages = 1;	// KJS 20031225
    dibsect.header.biSize = sizeof(dibsect.header);
    dibsect.header.biWidth = width|1;	// Ken did this
    dibsect.header.biHeight = -height;
    dibsect.header.biPlanes = 1;
    dibsect.header.biBitCount = 8;
    dibsect.header.biCompression = BI_RGB;
    dibsect.header.biClrUsed = 256;
    dibsect.header.biClrImportant = 256;
    for (i=0; i<256; i++)
    {
        dibsect.colours[i].rgbBlue = curpalette[i].b;
        dibsect.colours[i].rgbGreen = curpalette[i].g;
        dibsect.colours[i].rgbRed = curpalette[i].r;
    }

    hDIBSection = CreateDIBSection(hDC, (BITMAPINFO *)&dibsect, DIB_RGB_COLORS, &lpPixels, NULL, 0);
    if (!hDIBSection)
    {
        ReleaseDC(hWindow, hDC);
        hDC = NULL;

        ShowErrorBox("Error creating DIB section");
        return TRUE;
    }

    memset(lpPixels, 0, width*height);

    // create a compatible memory DC
    hDCSection = CreateCompatibleDC(hDC);
    if (!hDCSection)
    {
        ReleaseDC(hWindow, hDC);
        hDC = NULL;

        ShowErrorBox("Error creating compatible DC");
        return TRUE;
    }

    // select the DIB section into the memory DC
    if (!SelectObject(hDCSection, hDIBSection))
    {
        ReleaseDC(hWindow, hDC);
        hDC = NULL;
        DeleteDC(hDCSection);
        hDCSection = NULL;

        ShowErrorBox("Error creating compatible DC");
        return TRUE;
    }

    return FALSE;
}

#if defined(USE_OPENGL) && defined(POLYMOST)
//
// ReleaseOpenGL() -- cleans up OpenGL rendering stuff
//

static void ReleaseOpenGL(void)
{
    if (hGLRC)
    {
        polymost_glreset();
        if (!bwglMakeCurrent(0,0)) { }
        if (!bwglDeleteContext(hGLRC)) { }
        hGLRC = NULL;
    }
    if (hGLWindow)
    {
        if (hDC)
        {
            ReleaseDC(hGLWindow, hDC);
            hDC = NULL;
        }

        DestroyWindow(hGLWindow);
        hGLWindow = NULL;
    }
}


//
// UninitOpenGL() -- unitializes any openGL libraries
//
static void UninitOpenGL(void)
{
    ReleaseOpenGL();
}


//
// SetupOpenGL() -- sets up opengl rendering
//
static int32_t SetupOpenGL(int32_t width, int32_t height, int32_t bitspp)
{
    PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,                             //Version Number
        PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER, //Must Support these
        PFD_TYPE_RGBA,                 //Request An RGBA Format
        0,                        //Select Our Color Depth
        0,0,0,0,0,0,                   //Color Bits Ignored
        0,                             //No Alpha Buffer
        0,                             //Shift Bit Ignored
        0,                             //No Accumulation Buffer
        0,0,0,0,                       //Accumulation Bits Ignored
        32,                            //16/24/32 Z-Buffer depth
        1,                             //No Stencil Buffer
        0,                             //No Auxiliary Buffer
        PFD_MAIN_PLANE,                //Main Drawing Layer
        0,                             //Reserved
        0,0,0                          //Layer Masks Ignored
    };
    GLuint PixelFormat;
    int32_t minidriver;
    int32_t err;
    static int32_t warnonce = 0;
    pfd.cColorBits = bitspp;

    hGLWindow = CreateWindow(
                    WindowClass,
                    "OpenGL Window",
                    WS_CHILD|WS_VISIBLE,
                    0,0,
                    width,height,
                    hWindow,
                    (HMENU)0,
                    hInstance,
                    NULL);
    if (!hGLWindow)
    {
        ShowErrorBox("Error creating OpenGL child window.");
        return TRUE;
    }

    hDC = GetDC(hGLWindow);
    if (!hDC)
    {
        ReleaseOpenGL();
        ShowErrorBox("Error getting device context");
        return TRUE;
    }

    minidriver = Bstrcasecmp(gldriver,"opengl32.dll");

    if (minidriver) PixelFormat = bwglChoosePixelFormat(hDC,&pfd);
    else PixelFormat = ChoosePixelFormat(hDC,&pfd);
    if (!PixelFormat)
    {
        ReleaseOpenGL();
        ShowErrorBox("Can't choose pixel format");
        return TRUE;
    }

    if (minidriver) err = bwglSetPixelFormat(hDC, PixelFormat, &pfd);
    else err = SetPixelFormat(hDC, PixelFormat, &pfd);
    if (!err)
    {
        ReleaseOpenGL();
        ShowErrorBox("Can't set pixel format");
        return TRUE;
    }

    hGLRC = bwglCreateContext(hDC);
    if (!hGLRC)
    {
        ReleaseOpenGL();
        ShowErrorBox("Can't create GL RC");
        return TRUE;
    }

    if (!bwglMakeCurrent(hDC, hGLRC))
    {
        ReleaseOpenGL();
        ShowErrorBox("Can't activate GL RC");
        return TRUE;
    }

    polymost_glreset();

    bglEnable(GL_TEXTURE_2D);
    bglShadeModel(GL_SMOOTH); //GL_FLAT
    bglClearColor(0,0,0,0.5); //Black Background
    bglHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST); //Use FASTEST for ortho!
    bglHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    bglHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);
    bglDisable(GL_DITHER);

    {
        GLubyte *p,*p2,*p3;
        int32_t err = 0;

        glinfo.vendor     = (char *)bglGetString(GL_VENDOR);
        glinfo.renderer   = (char *)bglGetString(GL_RENDERER);
        glinfo.version    = (char *)bglGetString(GL_VERSION);
        glinfo.extensions = (char *)bglGetString(GL_EXTENSIONS);

        // GL driver blacklist
        if (!forcegl)
        {
            if (!Bstrcmp(glinfo.vendor,"Microsoft Corporation")) err = 1;
            else if (!Bstrcmp(glinfo.vendor,"SiS")) err = 1;
            else if (!Bstrcmp(glinfo.vendor,"3Dfx Interactive Inc.")) err = 1;
            else if (!Bstrcmp(glinfo.vendor,"Intel"))
            {
                if (!Bstrcmp(glinfo.renderer,"Intel 865G"))
                    err = 0;
                else if (!Bstrcmp(glinfo.renderer,"Intel 945GM"))
                    err = 0;
                else if (!Bstrcmp(glinfo.renderer,"Intel 965/963 Graphics Media Accelerator"))
                    err = 0;
                else err = 1;
            }

            if (err)
            {
                OSD_Printf("Unsupported OpenGL driver detected. GL modes will be unavailable. Use -forcegl to override.\n");
                wm_msgbox("Unsupported OpenGL driver", "Unsupported OpenGL driver detected.  GL modes will be unavailable.");
                ReleaseOpenGL();
                unloadgldriver();
                nogl = 1;
                modeschecked = 0;
                getvalidmodes();
                return TRUE;
            }
        }

        glinfo.maxanisotropy = 1.0;
        glinfo.bgra = 0;
        glinfo.texcompr = 0;

        // process the extensions string and flag stuff we recognize
        p = (GLubyte *)Bstrdup(glinfo.extensions);
        p3 = p;
        while ((p2 = (GLubyte *)Bstrtoken(p3==p?(char *)p:NULL, " ", (char**)&p3, 1)) != NULL)
        {
            if (!Bstrcmp((char *)p2, "GL_EXT_texture_filter_anisotropic"))
            {
                // supports anisotropy. get the maximum anisotropy level
                bglGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glinfo.maxanisotropy);
            }
            else if (!Bstrcmp((char *)p2, "GL_EXT_texture_edge_clamp") ||
                     !Bstrcmp((char *)p2, "GL_SGIS_texture_edge_clamp"))
            {
                // supports GL_CLAMP_TO_EDGE or GL_CLAMP_TO_EDGE_SGIS
                glinfo.clamptoedge = 1;
            }
            else if (!Bstrcmp((char *)p2, "GL_EXT_bgra"))
            {
                // support bgra textures
                glinfo.bgra = 1;
            }
            else if (!Bstrcmp((char *)p2, "GL_ARB_texture_compression"))
            {
                // support texture compression
                glinfo.texcompr = 1;
            }
            else if (!Bstrcmp((char *)p2, "GL_ARB_texture_non_power_of_two"))
            {
                // support non-power-of-two texture sizes
                glinfo.texnpot = 1;
            }
            else if (!Bstrcmp((char *)p2, "WGL_3DFX_gamma_control"))
            {
                // 3dfx cards have issues with fog
                nofog = 1;
                if (!(warnonce&1)) initprintf("3dfx card detected: OpenGL fog disabled\n");
                warnonce |= 1;
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
            else if (!Bstrcmp((char *)p2, "WGL_EXT_swap_control"))
            {
                glinfo.vsync = 1;
            }
            else if (!Bstrcmp((char *)p2, "GL_EXT_gpu_shader4"))
            {
                glinfo.sm4 = 1;
            }
        }
        Bfree(p);
    }
    numpages = 2;	// KJS 20031225: tell rotatesprite that it's double buffered!

    if (!glinfo.dumped)
    {
        int32_t oldbpp = bpp;
        bpp = 32;
        osdcmd_glinfo(NULL);
        glinfo.dumped = TRUE;
        bpp = oldbpp;
    }

    return FALSE;
}
#endif

//
// CreateAppWindow() -- create the application window
//
static BOOL CreateAppWindow(int32_t modenum)
{
    RECT rect;
    int32_t w, h, x, y, stylebits = 0, stylebitsex = 0;
    int32_t width, height, fs, bitspp;

    HRESULT result;

    if (modenum == 0x7fffffff)
    {
        width = customxdim;
        height = customydim;
        fs = customfs;
        bitspp = custombpp;
    }
    else
    {
        width = validmode[modenum].xdim;
        height = validmode[modenum].ydim;
        fs = validmode[modenum].fs;
        bitspp = validmode[modenum].bpp;
    }

    if (width == xres && height == yres && fs == fullscreen && bitspp == bpp && !videomodereset) return FALSE;

    if (hWindow)
    {
        if (bpp > 8)
        {
#if defined(USE_OPENGL) && defined(POLYMOST)
            ReleaseOpenGL();	// release opengl
#endif
        }
        else
        {
            ReleaseDirectDrawSurfaces();	// releases directdraw surfaces
        }

        if (!fs && fullscreen)
        {
            // restore previous display mode and set to normal cooperative level
            RestoreDirectDrawMode();
#if defined(USE_OPENGL) && defined(POLYMOST)
        }
        else if (fs && fullscreen)
        {
            // using CDS for GL modes, so restore from DirectDraw
            if (bpp != bitspp) RestoreDirectDrawMode();
#endif
        }


        ShowWindow(hWindow, SW_HIDE);	// so Windows redraws what's behind if the window shrinks
    }

    if (fs)
    {
        stylebitsex = WS_EX_TOPMOST;
        stylebits = WS_POPUP;
    }
    else
    {
        stylebitsex = 0;
        stylebits = WINDOW_STYLE;
    }

    if (!hWindow)
    {
        hWindow = CreateWindowEx(
                      stylebitsex,
                      "buildapp",
                      apptitle,
                      stylebits,
                      CW_USEDEFAULT,
                      CW_USEDEFAULT,
                      320,
                      200,
                      NULL,
                      NULL,
                      hInstance,
                      0);
        if (!hWindow)
        {
            ShowErrorBox("Unable to create window");
            return TRUE;
        }

        startwin_close();
    }
    else
    {
        SetWindowLong(hWindow,GWL_EXSTYLE,stylebitsex);
        SetWindowLong(hWindow,GWL_STYLE,stylebits);
    }

    // resize the window
    if (!fs)
    {
        rect.left = 0;
        rect.top = 0;
        rect.right = width-1;
        rect.bottom = height-1;
        AdjustWindowRect(&rect, stylebits, FALSE);

        w = (rect.right - rect.left);
        h = (rect.bottom - rect.top);
        x = (desktopxdim - w) / 2;
        y = (desktopydim - h) / 2;
    }
    else
    {
        x=y=0;
        w=width;
        h=height;
    }

    if (windowx == -1)
        windowx = x;

    if (windowy == -1)
        windowy = y;

    SetWindowText(hWindow, apptitle);
    ShowWindow(hWindow, SW_SHOWNORMAL);
    SetForegroundWindow(hWindow);
    SetFocus(hWindow);

    SetWindowPos(hWindow, HWND_TOP, windowpos?windowx:x, windowpos?windowy:y, w, h, 0);

    // fullscreen?
    if (!fs)
    {
        if (bitspp > 8)
        {
#if defined(USE_OPENGL) && defined(POLYMOST)
            // yes, start up opengl
            if (SetupOpenGL(width,height,bitspp))
            {
                return TRUE;
            }
#endif
        }
        else
        {
            // no, use DIB section
            if (SetupDIB(width,height))
            {
                return TRUE;
            }
        }

        modesetusing = -1;
    }
    else
    {
        // yes, set up DirectDraw

        // clean up after the DIB renderer if it was being used
        UninitDIB();

        if (!bDDrawInited)
        {
            DestroyWindow(hWindow);
            hWindow = NULL;
            return TRUE;
        }

#if defined(USE_OPENGL) && defined(POLYMOST)
        if (bitspp > 8)
        {
            DEVMODE dmScreenSettings;

            ZeroMemory(&dmScreenSettings, sizeof(DEVMODE));
            dmScreenSettings.dmSize = sizeof(DEVMODE);
            dmScreenSettings.dmPelsWidth = width;
            dmScreenSettings.dmPelsHeight = height;
            dmScreenSettings.dmBitsPerPel = bitspp;
            dmScreenSettings.dmFields = DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
            if (modenum != 0x7fffffff)
            {
                dmScreenSettings.dmDisplayFrequency = validmode[modenum].extra;
                dmScreenSettings.dmFields |= DM_DISPLAYFREQUENCY;
            }

            if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
            {
                ShowErrorBox("Video mode not supported");
                return TRUE;
            }

            ShowWindow(hWindow, SW_SHOWNORMAL);
            SetForegroundWindow(hWindow);
            SetFocus(hWindow);

            modesetusing = 1;
        }
        else
#endif
        {
            // set exclusive cooperative level
            result = IDirectDraw_SetCooperativeLevel(lpDD, hWindow, DDSCL_ALLOWMODEX|DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN);
            if (result != DD_OK)
            {
                ShowDDrawErrorBox("Error setting cooperative level", result);
                UninitDirectDraw();
                return TRUE;
            }

            result = IDirectDraw_SetDisplayMode(lpDD, width, height, bitspp);
            if (result != DD_OK)
            {
                ShowDDrawErrorBox("Error setting display mode", result);
                UninitDirectDraw();
                return TRUE;
            }

            modesetusing = 0;
        }

        if (bitspp > 8)
        {
#if defined(USE_OPENGL) && defined(POLYMOST)
            // we want an opengl mode
            if (SetupOpenGL(width,height,bitspp))
            {
                return TRUE;
            }
#endif
        }
        else
        {
            // we want software
            if (SetupDirectDraw(width,height))
            {
                return TRUE;
            }
        }
    }

#if defined(USE_OPENGL) && defined(POLYMOST)
    if (bitspp > 8) loadglextensions();
#endif

    xres = width;
    yres = height;
    bpp = bitspp;
    fullscreen = fs;
    curvidmode = modenum;

    frameplace = 0;
    lockcount = 0;

    // bytesperline is set when framebuffer is locked
    //bytesperline = width;
    //imageSize = bytesperline*yres;

    modechange = 1;
    OSD_ResizeDisplay(xres,yres);

    UpdateWindow(hWindow);

    return FALSE;
}


//
// DestroyAppWindow() -- destroys the application window
//
static void DestroyAppWindow(void)
{
    if (hWindow && gammabrightness)
    {
        setgammaramp(sysgamma);
        gammabrightness = 0;
    }

#if defined(USE_OPENGL) && defined(POLYMOST)
    UninitOpenGL();
#endif
    UninitDirectDraw();
    UninitDIB();

    if (hWindow)
    {
        DestroyWindow(hWindow);
        hWindow = NULL;
    }
}


//
// SaveSystemColours() -- save the Windows-reserved colours
//
static void SaveSystemColours(void)
{
    int32_t i;

    if (system_colours_saved) return;

    for (i=0; i<NUM_SYS_COLOURS; i++)
        syscolours[i] = GetSysColor(syscolouridx[i]);

    system_colours_saved = 1;
}


//
// SetBWSystemColours() -- set system colours to a black-and-white scheme
//
static void SetBWSystemColours(void)
{
#define WHI RGB(255,255,255)
#define BLA RGB(0,0,0)
    static COLORREF syscoloursbw[NUM_SYS_COLOURS] =
    {
        WHI, BLA, BLA, WHI, WHI, WHI, WHI, BLA, BLA, WHI,
        WHI, WHI, WHI, BLA, WHI, WHI, BLA, BLA, BLA, BLA,
        BLA, BLA, WHI, BLA, WHI
    };
#undef WHI
#undef BLA
    if (!system_colours_saved || bw_colours_set) return;

    SetSysColors(NUM_SYS_COLOURS, syscolouridx, syscoloursbw);
    bw_colours_set = 1;
}


//
// RestoreSystemColours() -- restore the Windows-reserved colours
//
static void RestoreSystemColours(void)
{
    if (!system_colours_saved || !bw_colours_set) return;

    SetSystemPaletteUse(hDC, SYSPAL_STATIC);
    SetSysColors(NUM_SYS_COLOURS, syscolouridx, syscolours);
    bw_colours_set = 0;
}


//
// ShowDDrawErrorBox() -- shows an error message box for a DirectDraw error
//
static void ShowDDrawErrorBox(const char *m, HRESULT r)
{
    TCHAR msg[1024];

    wsprintf(msg, "%s: %s", m, GetDDrawError(r));
    MessageBox(0, msg, apptitle, MB_OK|MB_ICONSTOP);
}


//
// GetDDrawError() -- stinking huge list of error messages since MS didn't want to include
//   them in the DLL
//
static const char * GetDDrawError(HRESULT code)
{
    switch (code)
    {
    case DD_OK:
        return "DD_OK";
    case DDERR_ALREADYINITIALIZED:
        return "DDERR_ALREADYINITIALIZED";
    case DDERR_BLTFASTCANTCLIP:
        return "DDERR_BLTFASTCANTCLIP";
    case DDERR_CANNOTATTACHSURFACE:
        return "DDERR_CANNOTATTACHSURFACE";
    case DDERR_CANNOTDETACHSURFACE:
        return "DDERR_CANNOTDETACHSURFACE";
    case DDERR_CANTCREATEDC:
        return "DDERR_CANTCREATEDC";
    case DDERR_CANTDUPLICATE:
        return "DDERR_CANTDUPLICATE";
    case DDERR_CANTLOCKSURFACE:
        return "DDERR_CANTLOCKSURFACE";
    case DDERR_CANTPAGELOCK:
        return "DDERR_CANTPAGELOCK";
    case DDERR_CANTPAGEUNLOCK:
        return "DDERR_CANTPAGEUNLOCK";
    case DDERR_CLIPPERISUSINGHWND:
        return "DDERR_CLIPPERISUSINGHWND";
    case DDERR_COLORKEYNOTSET:
        return "DDERR_COLORKEYNOTSET";
    case DDERR_CURRENTLYNOTAVAIL:
        return "DDERR_CURRENTLYNOTAVAIL";
    case DDERR_DCALREADYCREATED:
        return "DDERR_DCALREADYCREATED";
    case DDERR_DEVICEDOESNTOWNSURFACE:
        return "DDERR_DEVICEDOESNTOWNSURFACE";
    case DDERR_DIRECTDRAWALREADYCREATED:
        return "DDERR_DIRECTDRAWALREADYCREATED";
    case DDERR_EXCEPTION:
        return "DDERR_EXCEPTION";
    case DDERR_EXCLUSIVEMODEALREADYSET:
        return "DDERR_EXCLUSIVEMODEALREADYSET";
    case DDERR_EXPIRED:
        return "DDERR_EXPIRED";
    case DDERR_GENERIC:
        return "DDERR_GENERIC";
    case DDERR_HEIGHTALIGN:
        return "DDERR_HEIGHTALIGN";
    case DDERR_HWNDALREADYSET:
        return "DDERR_HWNDALREADYSET";
    case DDERR_HWNDSUBCLASSED:
        return "DDERR_HWNDSUBCLASSED";
    case DDERR_IMPLICITLYCREATED:
        return "DDERR_IMPLICITLYCREATED";
    case DDERR_INCOMPATIBLEPRIMARY:
        return "DDERR_INCOMPATIBLEPRIMARY";
    case DDERR_INVALIDCAPS:
        return "DDERR_INVALIDCAPS";
    case DDERR_INVALIDCLIPLIST:
        return "DDERR_INVALIDCLIPLIST";
    case DDERR_INVALIDDIRECTDRAWGUID:
        return "DDERR_INVALIDDIRECTDRAWGUID";
    case DDERR_INVALIDMODE:
        return "DDERR_INVALIDMODE";
    case DDERR_INVALIDOBJECT:
        return "DDERR_INVALIDOBJECT";
    case DDERR_INVALIDPARAMS:
        return "DDERR_INVALIDPARAMS";
    case DDERR_INVALIDPIXELFORMAT:
        return "DDERR_INVALIDPIXELFORMAT";
    case DDERR_INVALIDPOSITION:
        return "DDERR_INVALIDPOSITION";
    case DDERR_INVALIDRECT:
        return "DDERR_INVALIDRECT";
    case DDERR_INVALIDSTREAM:
        return "DDERR_INVALIDSTREAM";
    case DDERR_INVALIDSURFACETYPE:
        return "DDERR_INVALIDSURFACETYPE";
    case DDERR_LOCKEDSURFACES:
        return "DDERR_LOCKEDSURFACES";
    case DDERR_MOREDATA:
        return "DDERR_MOREDATA";
    case DDERR_NO3D:
        return "DDERR_NO3D";
    case DDERR_NOALPHAHW:
        return "DDERR_NOALPHAHW";
    case DDERR_NOBLTHW:
        return "DDERR_NOBLTHW";
    case DDERR_NOCLIPLIST:
        return "DDERR_NOCLIPLIST";
    case DDERR_NOCLIPPERATTACHED:
        return "DDERR_NOCLIPPERATTACHED";
    case DDERR_NOCOLORCONVHW:
        return "DDERR_NOCOLORCONVHW";
    case DDERR_NOCOLORKEY:
        return "DDERR_NOCOLORKEY";
    case DDERR_NOCOLORKEYHW:
        return "DDERR_NOCOLORKEYHW";
    case DDERR_NOCOOPERATIVELEVELSET:
        return "DDERR_NOCOOPERATIVELEVELSET";
    case DDERR_NODC:
        return "DDERR_NODC";
    case DDERR_NODDROPSHW:
        return "DDERR_NODDROPSHW";
    case DDERR_NODIRECTDRAWHW:
        return "DDERR_NODIRECTDRAWHW";
    case DDERR_NODIRECTDRAWSUPPORT:
        return "DDERR_NODIRECTDRAWSUPPORT";
    case DDERR_NOEMULATION:
        return "DDERR_NOEMULATION";
    case DDERR_NOEXCLUSIVEMODE:
        return "DDERR_NOEXCLUSIVEMODE";
    case DDERR_NOFLIPHW:
        return "DDERR_NOFLIPHW";
    case DDERR_NOFOCUSWINDOW:
        return "DDERR_NOFOCUSWINDOW";
    case DDERR_NOGDI:
        return "DDERR_NOGDI";
    case DDERR_NOHWND:
        return "DDERR_NOHWND";
    case DDERR_NOMIPMAPHW:
        return "DDERR_NOMIPMAPHW";
    case DDERR_NOMIRRORHW:
        return "DDERR_NOMIRRORHW";
    case DDERR_NONONLOCALVIDMEM:
        return "DDERR_NONONLOCALVIDMEM";
    case DDERR_NOOPTIMIZEHW:
        return "DDERR_NOOPTIMIZEHW";
    case DDERR_NOOVERLAYDEST:
        return "DDERR_NOOVERLAYDEST";
    case DDERR_NOOVERLAYHW:
        return "DDERR_NOOVERLAYHW";
    case DDERR_NOPALETTEATTACHED:
        return "DDERR_NOPALETTEATTACHED";
    case DDERR_NOPALETTEHW:
        return "DDERR_NOPALETTEHW";
    case DDERR_NORASTEROPHW:
        return "DDERR_NORASTEROPHW";
    case DDERR_NOROTATIONHW:
        return "DDERR_NOROTATIONHW";
    case DDERR_NOSTRETCHHW:
        return "DDERR_NOSTRETCHHW";
    case DDERR_NOT4BITCOLOR:
        return "DDERR_NOT4BITCOLOR";
    case DDERR_NOT4BITCOLORINDEX:
        return "DDERR_NOT4BITCOLORINDEX";
    case DDERR_NOT8BITCOLOR:
        return "DDERR_NOT8BITCOLOR";
    case DDERR_NOTAOVERLAYSURFACE:
        return "DDERR_NOTAOVERLAYSURFACE";
    case DDERR_NOTEXTUREHW:
        return "DDERR_NOTEXTUREHW";
    case DDERR_NOTFLIPPABLE:
        return "DDERR_NOTFLIPPABLE";
    case DDERR_NOTFOUND:
        return "DDERR_NOTFOUND";
    case DDERR_NOTINITIALIZED:
        return "DDERR_NOTINITIALIZED";
    case DDERR_NOTLOADED:
        return "DDERR_NOTLOADED";
    case DDERR_NOTLOCKED:
        return "DDERR_NOTLOCKED";
    case DDERR_NOTPAGELOCKED:
        return "DDERR_NOTPAGELOCKED";
    case DDERR_NOTPALETTIZED:
        return "DDERR_NOTPALETTIZED";
    case DDERR_NOVSYNCHW:
        return "DDERR_NOVSYNCHW";
    case DDERR_NOZBUFFERHW:
        return "DDERR_NOZBUFFERHW";
    case DDERR_NOZOVERLAYHW:
        return "DDERR_NOZOVERLAYHW";
    case DDERR_OUTOFCAPS:
        return "DDERR_OUTOFCAPS";
    case DDERR_OUTOFMEMORY:
        return "DDERR_OUTOFMEMORY";
    case DDERR_OUTOFVIDEOMEMORY:
        return "DDERR_OUTOFVIDEOMEMORY";
    case DDERR_OVERLAPPINGRECTS:
        return "DDERR_OVERLAPPINGRECTS";
    case DDERR_OVERLAYCANTCLIP:
        return "DDERR_OVERLAYCANTCLIP";
    case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
        return "DDERR_OVERLAYCOLORKEYONLYONEACTIVE";
    case DDERR_OVERLAYNOTVISIBLE:
        return "DDERR_OVERLAYNOTVISIBLE";
    case DDERR_PALETTEBUSY:
        return "DDERR_PALETTEBUSY";
    case DDERR_PRIMARYSURFACEALREADYEXISTS:
        return "DDERR_PRIMARYSURFACEALREADYEXISTS";
    case DDERR_REGIONTOOSMALL:
        return "DDERR_REGIONTOOSMALL";
    case DDERR_SURFACEALREADYATTACHED:
        return "DDERR_SURFACEALREADYATTACHED";
    case DDERR_SURFACEALREADYDEPENDENT:
        return "DDERR_SURFACEALREADYDEPENDENT";
    case DDERR_SURFACEBUSY:
        return "DDERR_SURFACEBUSY";
    case DDERR_SURFACEISOBSCURED:
        return "DDERR_SURFACEISOBSCURED";
    case DDERR_SURFACELOST:
        return "DDERR_SURFACELOST";
    case DDERR_SURFACENOTATTACHED:
        return "DDERR_SURFACENOTATTACHED";
    case DDERR_TOOBIGHEIGHT:
        return "DDERR_TOOBIGHEIGHT";
    case DDERR_TOOBIGSIZE:
        return "DDERR_TOOBIGSIZE";
    case DDERR_TOOBIGWIDTH:
        return "DDERR_TOOBIGWIDTH";
    case DDERR_UNSUPPORTED:
        return "DDERR_UNSUPPORTED";
    case DDERR_UNSUPPORTEDFORMAT:
        return "DDERR_UNSUPPORTEDFORMAT";
    case DDERR_UNSUPPORTEDMASK:
        return "DDERR_UNSUPPORTEDMASK";
    case DDERR_UNSUPPORTEDMODE:
        return "DDERR_UNSUPPORTEDMODE";
    case DDERR_VERTICALBLANKINPROGRESS:
        return "DDERR_VERTICALBLANKINPROGRESS";
    case DDERR_VIDEONOTACTIVE:
        return "DDERR_VIDEONOTACTIVE";
    case DDERR_WASSTILLDRAWING:
        return "DDERR_WASSTILLDRAWING";
    case DDERR_WRONGMODE:
        return "DDERR_WRONGMODE";
    case DDERR_XALIGN:
        return "DDERR_XALIGN";
    default:
        break;
    }
    return "Unknown error";
}






//-------------------------------------------------------------------------------------------------
//  MOSTLY STATIC INTERNAL WINDOWS THINGS
//=================================================================================================

//
// ShowErrorBox() -- shows an error message box
//
static void ShowErrorBox(const char *m)
{
    TCHAR msg[1024];

    wsprintf(msg, "%s: %s", m, GetWindowsErrorMsg(GetLastError()));
    MessageBox(0, msg, apptitle, MB_OK|MB_ICONSTOP);
}


//
// CheckWinVersion() -- check to see what version of Windows we happen to be running under
//
static BOOL CheckWinVersion(void)
{
    OSVERSIONINFO osv;

    ZeroMemory(&osv, sizeof(osv));
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osv)) return TRUE;

    // haha, yeah, like it will work on Win32s
    if (osv.dwPlatformId == VER_PLATFORM_WIN32s) return TRUE;

    // we don't like NT 3.51
    if (osv.dwMajorVersion < 4) return TRUE;

    // nor do we like NT 4
    if (osv.dwPlatformId == VER_PLATFORM_WIN32_NT &&
            osv.dwMajorVersion == 4) return TRUE;

    return FALSE;
}


//
// WndProcCallback() -- the Windows window callback
//
static LRESULT CALLBACK WndProcCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    /*    RECT rect;
        POINT pt;
        HRESULT result; */

#if defined(POLYMOST) && defined(USE_OPENGL)
    if (hWnd == hGLWindow) return DefWindowProc(hWnd,uMsg,wParam,lParam);
#endif

    switch (uMsg)
    {
    case WM_SYSCOMMAND:
        // don't let the monitor fall asleep or let the screensaver activate
        if (wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER) return 0;

        // Since DirectInput won't let me set an exclusive-foreground
        // keyboard for some unknown reason I just have to tell Windows to
        // rack off with its keyboard accelerators.
        if (wParam == SC_KEYMENU || wParam == SC_HOTKEY) return 0;
        break;

    case WM_ACTIVATEAPP:
    {
        appactive = wParam;
#if defined(POLYMOST) && defined(USE_OPENGL)
        if (hGLWindow)
        {
            if (!appactive && fullscreen)
            {
                if (mousegrab)
                {
                    grabmouse(0);
                    regrabmouse = 1;
                }
                realfs = fullscreen;
                silentvideomodeswitch = 1;
                setgamemode(!fullscreen,xdim,ydim,bpp);
                ShowWindow(hWindow, SW_MINIMIZE);
            }
            else if (appactive && realfs)
            {
                if (regrabmouse)
                {
                    grabmouse(1);
                    regrabmouse = 0;
                }
                ShowWindow(hWindow, SW_RESTORE);
                SetForegroundWindow(hWindow);
                SetFocus(hWindow);
                setgamemode(realfs,xdim,ydim,bpp);
                silentvideomodeswitch = 0;
                realfs = 0;
            }
        }
#endif

        if (backgroundidle)
            SetPriorityClass(GetCurrentProcess(),
                             appactive ? NORMAL_PRIORITY_CLASS : IDLE_PRIORITY_CLASS);

        if (appactive)
        {
            SetForegroundWindow(hWindow);
            SetFocus(hWindow);
        }
        AcquireInputDevices(appactive,-1);
        break;
    }
    case WM_ACTIVATE:
        if (desktopbpp <= 8)
        {
            if (appactive)
            {
                setpalette(0,0);
                SetBWSystemColours();
                //					initprintf("Resetting palette.\n");
            }
            else
            {
                RestoreSystemColours();
                //					initprintf("Resetting system colours.\n");
            }
        }

        if (appactive)
        {
            SetForegroundWindow(hWindow);
            SetFocus(hWindow);
        }

        AcquireInputDevices(appactive,-1);
        break;

    case WM_SIZE:
        if (wParam == SIZE_MAXHIDE || wParam == SIZE_MINIMIZED) appactive = 0;
        else appactive = 1;
        AcquireInputDevices(appactive,-1);
        break;

    case WM_PALETTECHANGED:
        // someone stole the palette so try and steal it back
        if (bDDrawInited && bpp == 8 && fullscreen)
        {
            int32_t result = IDirectDrawSurface_SetPalette(lpDDSPrimary, lpDDPalette);
            if (result != DD_OK)
            {
                initprintf("Palette set failed: %s\n", GetDDrawError(result));
                break;
            }
            setpalette(0,256);
            break;
        }
        if (appactive && (HWND)wParam != hWindow) setpalette(0,256);
        break;

    case WM_DISPLAYCHANGE:
        // desktop settings changed so adjust our world-view accordingly
        desktopxdim = LOWORD(lParam);
        desktopydim = HIWORD(lParam);
        desktopbpp  = wParam;
        getvalidmodes();
        break;

    case WM_PAINT:
        repaintneeded=1;
        break;

        // don't draw the frame if fullscreen
        //case WM_NCPAINT:
        //if (!fullscreen) break;
        //return 0;

    case WM_ERASEBKGND:
        return TRUE;

    case WM_MOVE:
//        windowx = LOWORD(lParam);
//        windowy = HIWORD(lParam);
        return 0;

    case WM_MOVING:
    {
        RECT *RECTYMcRECT = (LPRECT)lParam;

        windowx = RECTYMcRECT->left;
        windowy = RECTYMcRECT->top;
        return 0;
    }
    case WM_CLOSE:
        quitevent = 1;
        return 0;

    case WM_KEYDOWN:
    case WM_KEYUP:
        // pause sucks. I read that apparently it doesn't work the same everwhere
        // with DirectInput but it does with Windows messages. Oh well.
        if (wParam == VK_PAUSE && (lParam & 0x80000000l))
        {
            SetKey(0x59, 1);

            if (keypresscallback)
                keypresscallback(0x59, 1);
        }
        break;

        // JBF 20040115: Alt-F4 upsets us, so drop all system keys on their asses
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        return 0;

    case WM_CHAR:
        if (((keyasciififoend+1)&(KEYFIFOSIZ-1)) == keyasciififoplc) return 0;
        if ((keyasciififoend - keyasciififoplc) > 0) return 0;
        if (Btolower(scantoasc[OSD_OSDKey()]) == Btolower((uint8_t)wParam)) return 0;
        if (!OSD_HandleChar((uint8_t)wParam)) return 0;
        keyasciififo[keyasciififoend] = (uint8_t)wParam;
        keyasciififoend = ((keyasciififoend+1)&(KEYFIFOSIZ-1));
        //OSD_Printf("WM_CHAR %d, %d-%d\n",wParam,keyasciififoplc,keyasciififoend);
        return 0;

    case WM_HOTKEY:
        return 0;

    case WM_ENTERMENULOOP:
    case WM_ENTERSIZEMOVE:
        AcquireInputDevices(0,-1);
        return 0;
    case WM_EXITMENULOOP:
    case WM_EXITSIZEMOVE:
        AcquireInputDevices(1,-1);
        return 0;

    case WM_DESTROY:
        hWindow = 0;
        //PostQuitMessage(0);	// JBF 20040115: not anymore
        return 0;

    default:
        break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


//
// RegisterWindowClass() -- register the window class
//
static BOOL RegisterWindowClass(void)
{
    WNDCLASSEX wcx;

    if (window_class_registered) return FALSE;

    //initprintf("Registering window class\n");

    wcx.cbSize	= sizeof(wcx);
    wcx.style	= CS_OWNDC;
    wcx.lpfnWndProc	= WndProcCallback;
    wcx.cbClsExtra	= 0;
    wcx.cbWndExtra	= 0;
    wcx.hInstance	= hInstance;
    wcx.hIcon	= LoadImage(hInstance, MAKEINTRESOURCE(100), IMAGE_ICON,
                          GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    wcx.hCursor	= LoadCursor(NULL, IDC_ARROW);
    wcx.hbrBackground = (HBRUSH)COLOR_GRAYTEXT;
    wcx.lpszMenuName = NULL;
    wcx.lpszClassName = WindowClass;
    wcx.hIconSm	= LoadImage(hInstance, MAKEINTRESOURCE(100), IMAGE_ICON,
                            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    if (!RegisterClassEx(&wcx))
    {
        ShowErrorBox("Failed to register window class");
        return TRUE;
    }

    window_class_registered = TRUE;

    return FALSE;
}


//
// GetWindowsErrorMsg() -- gives a pointer to a static buffer containing the Windows error message
//
static LPTSTR GetWindowsErrorMsg(DWORD code)
{
    static TCHAR lpMsgBuf[1024];

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, code,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)lpMsgBuf, 1024, NULL);

    return lpMsgBuf;
}

//
// makeasmwriteable() -- removes write protection from the self-modifying assembly code
//
void makeasmwriteable(void)
{
#ifndef ENGINE_USING_A_C
    extern int32_t dep_begin, dep_end;
    DWORD oldprot;

    if (!VirtualProtect((LPVOID)&dep_begin, (SIZE_T)&dep_end - (SIZE_T)&dep_begin, PAGE_EXECUTE_READWRITE, &oldprot))
    {
        ShowErrorBox("Problem making code writeable");
    }
#endif
}

