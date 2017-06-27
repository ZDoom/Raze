// Windows DIB/DirectDraw interface layer for the Build Engine
// Originally by Jonathon Fowler (jf@jonof.id.au)

#include "compat.h"

#include "build.h"

#define NEED_DINPUT_H
#define NEED_DDRAW_H
#ifdef _MSC_VER
# define NEED_CRTDBG_H
#endif
#include "windows_inc.h"

#include "winlayer.h"
#include "rawinput.h"
#include "mutex.h"

#include "winbits.h"
#include "engine_priv.h"

#include "dxdidf.h"	// comment this out if c_dfDI* is being reported as multiply defined

#include <signal.h>

// undefine to restrict windowed resolutions to conventional sizes
#define ANY_WINDOWED_SIZE

static mutex_t m_initprintf;
static int32_t winlayer_have_ATI = 0;

static int32_t   _buildargc = 0;
static const char **_buildargv = NULL;
static char *argvbuf = NULL;

// Windows crud
static HINSTANCE hInstance = 0;
static HWND hWindow = 0;
#define WINDOW_STYLE (WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX)
static BOOL window_class_registered = FALSE;

static DDGAMMARAMP sysgamma;

#ifdef USE_OPENGL
// OpenGL stuff
static HGLRC hGLRC = 0;
int32_t nofog=0;
char nogl=0;
char forcegl=0;
#endif

static const char *GetDDrawError(HRESULT code);
static const char *GetDInputError(HRESULT code);
static void ShowDDrawErrorBox(const char *m, HRESULT r);
static void ShowDInputErrorBox(const char *m, HRESULT r);
static LRESULT CALLBACK WndProcCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL InitDirectDraw(void);
static void UninitDirectDraw(void);
static int32_t RestoreDirectDrawMode(void);
static void ReleaseDirectDrawSurfaces(void);
static BOOL InitDirectInput(void);
static void UninitDirectInput(void);
static void GetKeyNames(void);
static void AcquireInputDevices(char acquire);
static inline void DI_PollJoysticks(void);
static int32_t SetupDirectDraw(int32_t width, int32_t height);
static void UninitDIB(void);
static int32_t SetupDIB(int32_t width, int32_t height);
#ifdef USE_OPENGL
static void ReleaseOpenGL(void);
static void UninitOpenGL(void);
static int32_t SetupOpenGL(int32_t width, int32_t height, int32_t bitspp);
#endif
static BOOL RegisterWindowClass(void);
static BOOL CreateAppWindow(int32_t modenum);
static void DestroyAppWindow(void);

// video
static int32_t desktopxdim=0;
static int32_t desktopydim=0;
static int32_t desktopbpp=0;
static int32_t modesetusing=-1;
static int32_t curvidmode = -1;
static int32_t customxdim = 640;
static int32_t customydim = 480;
static int32_t custombpp = 8;
static int32_t customfs = 0;
static uint32_t modeschecked=0;
int32_t xres=-1;
int32_t yres=-1;
int32_t fullscreen=0;
int32_t bpp=0;
int32_t bytesperline=0;
int32_t lockcount=0;
int32_t glcolourdepth=32;
static int32_t vsync_renderlayer;
uint32_t maxrefreshfreq=60;
intptr_t frameplace=0;
char modechange=1;
char repaintneeded=0;
char offscreenrendering=0;
char videomodereset = 0;

// input and events
char quitevent=0;
char appactive=1;
char realfs=0;
char regrabmouse=0;
int32_t inputchecked = 0;

//-------------------------------------------------------------------------------------------------
//  DINPUT (JOYSTICK)
//=================================================================================================

#define JOYSTICK	0

static HMODULE               hDInputDLL    = NULL;
static LPDIRECTINPUT7A        lpDI          = NULL;
static LPDIRECTINPUTDEVICE7A lpDID = NULL;
#define INPUT_BUFFER_SIZE	32
static GUID                  guidDevs;

char di_disabled = 0;
static char di_devacquired;
static HANDLE di_inputevt = 0;
static int32_t joyblast=0;

static struct
{
    char const *name;
    LPDIRECTINPUTDEVICE7A *did;
    LPCDIDATAFORMAT df;
} devicedef = { "joystick", &lpDID, &c_dfDIJoystick };

static struct _joydef
{
    char *name;
    uint32_t ofs;	// directinput 'dwOfs' value
} *axisdefs = NULL, *buttondefs = NULL, *hatdefs = NULL;



//-------------------------------------------------------------------------------------------------
//  MAIN CRAP
//=================================================================================================


//
// win_gethwnd() -- gets the window handle
//
HWND win_gethwnd(void)
{
    return hWindow;
}


//
// win_gethinstance() -- gets the application instance
//
HINSTANCE win_gethinstance(void)
{
    return hInstance;
}



//
// wm_msgbox/wm_ynbox() -- window-manager-provided message boxes
//
int32_t wm_msgbox(const char *name, const char *fmt, ...)
{
    char buf[2048];
    va_list va;

    va_start(va,fmt);
    vsprintf(buf,fmt,va);
    va_end(va);

    MessageBox(hWindow,buf,name,MB_OK|MB_TASKMODAL);
    return 0;
}


int32_t wm_ynbox(const char *name, const char *fmt, ...)
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
void wm_setapptitle(const char *name)
{
    if (name)
        Bstrncpyz(apptitle, name, sizeof(apptitle));

    if (hWindow) SetWindowText(hWindow, apptitle);
    startwin_settitle(apptitle);
}

static int32_t setgammaramp(LPDDGAMMARAMP gt);

//
// SignalHandler() -- called when we've sprung a leak
//
static void SignalHandler(int32_t signum)
{
    switch (signum)
    {
    case SIGSEGV:
        OSD_Printf("Fatal Signal caught: SIGSEGV. Bailing out.\n");
        if (gammabrightness)
            setgammaramp(&sysgamma);
        gammabrightness = 0;
        app_crashhandler();
        uninitsystem();
        if (stdout) fclose(stdout);
        break;
    default:
        break;
    }
}

static void divcommon(int32_t *ap, int32_t *bp)
{
    const int32_t p[] = {2,3,5,7,11,13,17,19};
    const int32_t N = (int32_t)ARRAY_SIZE(p);
    int32_t a=*ap, b=*bp;

    while (1)
    {
        int32_t i;

        for (i=0; i<N; i++)
            if (a%p[i] == 0 && b%p[i]==0)
            {
                a /= p[i];
                b /= p[i];
                break;
            }
        if (i == N)
            break;
    }

    *ap = a;
    *bp = b;
}

//
// WinMain() -- main Windows entry point
//
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
    int32_t r;
#ifdef USE_OPENGL
    char *argp;
#endif
    HDC hdc;

    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    hInstance = hInst;

#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF);
#endif

    if (!CheckWinVersion() || hPrevInst)
    {
        MessageBox(0, "This application requires a newer Windows version to run.",
                   apptitle, MB_OK|MB_ICONSTOP);
        return -1;
    }

    win_open();

    hdc = GetDC(NULL);
    r = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(NULL, hdc);
    if (r < 8)
    {
        MessageBox(0, "This application requires a desktop color depth of 256-colors or better.",
                   apptitle, MB_OK|MB_ICONSTOP);
        return -1;
    }

    // carve up the command line into more recognizable pieces
    argvbuf = Bstrdup(GetCommandLine());
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

        _buildargv = (const char **)Bmalloc(sizeof(char *)*(_buildargc+1));
        wp = argvbuf;
        for (i=0; i<_buildargc; i++,wp++)
        {
            _buildargv[i] = wp;
            while (*wp) wp++;
        }
        _buildargv[_buildargc] = NULL;
    }

    maybe_redirect_outputs();

#ifdef USE_OPENGL
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

    startwin_open();
    baselayer_init();

    r = app_main(_buildargc, _buildargv);

    fclose(stdout);

    if (r) Sleep(3000);

    startwin_close();

    win_close();

    Bfree(argvbuf);

    return r;
}


static int32_t set_maxrefreshfreq(osdfuncparm_t const * const parm)
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

//
// initsystem() -- init systems
//

// http://www.gamedev.net/topic/47021-how-to-determine-video-card-with-win32-api
static void determine_ATI(void)
{
    DISPLAY_DEVICE DevInfo;
    DWORD i;

    ZeroMemory(&DevInfo, sizeof(DevInfo));
    DevInfo.cb = sizeof(DISPLAY_DEVICE);

    for (i=0; EnumDisplayDevices(NULL, i, &DevInfo, 0); i++)
    {
        if ((DevInfo.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)==0)
            continue;

#ifdef UNICODE
#error Not implemented: UNICODE defined, DevInfo.DeviceString is a WCHAR
#endif
//        initprintf("%s *** %s\n", DevInfo.DeviceName, DevInfo.DeviceString);
        if (!Bmemcmp(DevInfo.DeviceString, "ATI ", 4) || !Bmemcmp(DevInfo.DeviceString, "AMD ", 4))
            winlayer_have_ATI = 1;
    }
}

int32_t initsystem(void)
{
    DEVMODE desktopmode;

    //    initprintf("Initializing Windows DirectX/GDI system interface\n");

    mutex_init(&m_initprintf);

    // get the desktop dimensions before anything changes them
    ZeroMemory(&desktopmode, sizeof(DEVMODE));
    desktopmode.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&desktopmode);

    desktopxdim = desktopmode.dmPelsWidth;
    desktopydim = desktopmode.dmPelsHeight;
    desktopbpp  = desktopmode.dmBitsPerPel;

    memset(curpalette, 0, sizeof(palette_t) * 256);

    atexit(uninitsystem);

    frameplace=0;
    lockcount=0;

    win_init();

#ifdef USE_OPENGL
    if (loadgldriver(getenv("BUILD_GLDRV")))
    {
        initprintf("Failure loading OpenGL. GL modes are unavailable.\n");
        nogl = 1;
    }
#endif

    // determine physical screen size
    {
        const int32_t oscreenx = GetSystemMetrics(SM_CXSCREEN);
        const int32_t oscreeny = GetSystemMetrics(SM_CYSCREEN);
        int32_t screenx=oscreenx, screeny=oscreeny, good=0;

        divcommon(&screenx, &screeny);

        if (screenx >= 1 && screenx <= 99 && screeny >= 1 && screeny <= 99)
            r_screenxy = screenx*100 + screeny, good=1;

        if (!good)
            initprintf("Automatic fullscreen size determination failed! %d %d -> %d %d.\n",
            oscreenx, oscreeny, screenx, screeny);
    }

    // try and start DirectDraw
    if (InitDirectDraw())
        initprintf("DirectDraw initialization failed. Fullscreen modes will be unavailable.\n");

    OSD_RegisterFunction("maxrefreshfreq", "maxrefreshfreq: maximum display frequency to set for OpenGL Polymost modes (0=no maximum)", set_maxrefreshfreq);

    // See if we're on an ATI card...
    determine_ATI();

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

    win_uninit();

#ifdef USE_OPENGL
    unloadgldriver();
#endif
}


//
// system_getcvars() -- propagate any cvars that are read post-initialization
//
void system_getcvars(void)
{
    vsync = setvsync(vsync);
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

    OSD_Puts(buf);

    mutex_lock(&m_initprintf);
    if ((Bstrlen(dabuf) + Bstrlen(buf) + 2) > sizeof(dabuf))
    {
        startwin_puts(dabuf);
        Bmemset(dabuf, 0, sizeof(dabuf));
    }

    Bstrcat(dabuf,buf);

    if (flushlogwindow || Bstrlen(dabuf) > 768)
    {
        startwin_puts(dabuf);
        handleevents();
        Bmemset(dabuf, 0, sizeof(dabuf));
    }
    mutex_unlock(&m_initprintf);
}


//
// debugprintf() -- sends a formatted debug string to the debugger
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


int32_t handleevents_peekkeys(void)
{
    return 0;
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

    RI_PollDevices(TRUE);

    if (hDInputDLL)
        DI_PollJoysticks();

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        switch (msg.message)
        {
        case WM_QUIT:
            quitevent = 1;
            continue;
        case WM_INPUT:
            // this call to RI_PollDevices() probably isn't necessary
            // RI_PollDevices(FALSE);
            RI_ProcessMessage(&msg);
            continue;
        }

        if (startwin_idle((void *)&msg) > 0) continue;

        TranslateMessage(&msg);
//        DispatchMessage(&msg);
        WndProcCallback(msg.hwnd, msg.message, msg.wParam, msg.lParam);
    }

    inputchecked = 0;

    if (!appactive || quitevent) rv = -1;

    sampletimer();

    return rv;
}


//
// initinput() -- init input system
//
int32_t initinput(void)
{
    int32_t i;

    Win_GetOriginalLayoutName();
    Win_SetKeyboardLayoutUS(1);

    moustat=0;
    memset(keystatus, 0, sizeof(keystatus));

    if (!keyremapinit)
        for (i=0; i<256; i++)
            keyremap[i]=i;

    keyremapinit=1;
    keyfifoplc = keyfifoend = 0;
    keyasciififoplc = keyasciififoend = 0;

    inputdevices = 1|2;
    joyisgamepad = joynumaxes = joynumbuttons = joynumhats=0;

    GetKeyNames();
    InitDirectInput();

    return 0;
}


//
// uninitinput() -- uninit input system
//
void uninitinput(void)
{
    Win_SetKeyboardLayoutUS(0);

    uninitmouse();
    UninitDirectInput();
}


void idle_waitevent_timeout(uint32_t timeout)
{
    // timeout becomes a completion deadline
    timeout += getticks();

    do
    {
        MSG msg;

        if (PeekMessage(&msg, 0, WM_INPUT, WM_INPUT, PM_QS_INPUT))
        {
            RI_PollDevices(TRUE);

            inputchecked = 0;

            return;
        }

        Sleep(10);
    }
    while (timeout > (getticks() + 10));
}


//
// setjoydeadzone() -- sets the dead and saturation zones for the joystick
//
void setjoydeadzone(int32_t axis, uint16_t dead, uint16_t satur)
{
    DIPROPDWORD dipdw;
    HRESULT result;

    if (!lpDID) return;

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

    result = IDirectInputDevice7_SetProperty(lpDID, bDIPROP_DEADZONE, &dipdw.diph);
    if (FAILED(result))
    {
        //ShowDInputErrorBox("Failed setting joystick dead zone", result);
        initprintf("Failed setting joystick dead zone: %s\n", GetDInputError(result));
        return;
    }

    dipdw.dwData = satur;

    result = IDirectInputDevice7_SetProperty(lpDID, bDIPROP_SATURATION, &dipdw.diph);
    if (FAILED(result))
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
    if (!lpDID) { *dead = *satur = 0; return; }
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

    result = IDirectInputDevice7_GetProperty(lpDID, bDIPROP_DEADZONE, (LPDIPROPHEADER)&dipdw.diph);
    if (FAILED(result))
    {
        //ShowDInputErrorBox("Failed getting joystick dead zone", result);
        initprintf("Failed getting joystick dead zone: %s\n", GetDInputError(result));
        return;
    }

    *dead = dipdw.dwData;

    result = IDirectInputDevice7_GetProperty(lpDID, bDIPROP_SATURATION, &dipdw.diph);
    if (FAILED(result))
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
        if (mouseb & 16) mousepresscallback(5, 0);
        if (mouseb & 32) mousepresscallback(6, 0);
        if (mouseb & 64) mousepresscallback(7, 0);
    }
    mouseb = 0;

    if (joypresscallback)
    {
        for (i=0; i<32; i++)
            if (joyb & (1<<i)) joypresscallback(i+1, 0);
    }
    joyb = joyblast = 0;

    for (i=0; i<KEYSTATUSSIZ; i++)
    {
        //if (!keystatus[i]) continue;
        //if (OSD_HandleKey(i, 0) != 0) {
        OSD_HandleScanCode(i, 0);
        SetKey(i, 0);
        if (keypresscallback) keypresscallback(i, 0);
        //}
    }
}


//
// InitDirectInput() -- get DirectInput started
//

// device enumerator
static BOOL CALLBACK InitDirectInput_enum(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
    const char *d;

    UNREFERENCED_PARAMETER(pvRef);

    if ((lpddi->dwDevType&0xff) != DIDEVTYPE_JOYSTICK)
        return DIENUM_CONTINUE;

    inputdevices |= 4;
    joyisgamepad = ((lpddi->dwDevType & (DIDEVTYPEJOYSTICK_GAMEPAD<<8)) != 0);
    d = joyisgamepad ? "GAMEPAD" : "JOYSTICK";
    Bmemcpy(&guidDevs, &lpddi->guidInstance, sizeof(GUID));

    initprintf("    * %s: %s\n", d, lpddi->tszProductName);

    return DIENUM_CONTINUE;
}

static BOOL CALLBACK InitDirectInput_enumobjects(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
    int32_t *typecounts = (int32_t *)pvRef;

    if (lpddoi->dwType & DIDFT_AXIS)
    {
        //initprintf(" Axis: %s (dwOfs=%d)\n", lpddoi->tszName, lpddoi->dwOfs);

        axisdefs[ typecounts[0] ].name = Bstrdup(lpddoi->tszName);
        axisdefs[ typecounts[0] ].ofs = lpddoi->dwOfs;

        typecounts[0]++;
    }
    else if (lpddoi->dwType & DIDFT_BUTTON)
    {
        //initprintf(" Button: %s (dwOfs=%d)\n", lpddoi->tszName, lpddoi->dwOfs);

        buttondefs[ typecounts[1] ].name = Bstrdup(lpddoi->tszName);
        buttondefs[ typecounts[1] ].ofs = lpddoi->dwOfs;

        typecounts[1]++;
    }
    else if (lpddoi->dwType & DIDFT_POV)
    {
        //initprintf(" POV: %s (dwOfs=%d)\n", lpddoi->tszName, lpddoi->dwOfs);

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

    if (hDInputDLL || di_disabled) return FALSE;

    initprintf("Initializing DirectInput...\n");

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

    aDirectInputCreateA = (HRESULT(WINAPI *)(HINSTANCE, DWORD, LPDIRECTINPUT7A *, LPUNKNOWN))GetProcAddress(hDInputDLL, "DirectInputCreateA");
    if (!aDirectInputCreateA) ShowErrorBox("Error fetching DirectInputCreateA()");

    result = aDirectInputCreateA(hInstance, DIRECTINPUT_VERSION, &lpDI, NULL);
    if (FAILED(result)) { HorribleDInputDeath("DirectInputCreateA() failed", result); }
    else if (result != DI_OK) initprintf("    Created DirectInput object with warning: %s\n",GetDInputError(result));

    initprintf("  - Enumerating attached game controllers\n");
    inputdevices = 1|2;
    result = IDirectInput7_EnumDevices(lpDI, DIDEVTYPE_JOYSTICK, InitDirectInput_enum, NULL, DIEDFL_ATTACHEDONLY);
    if (FAILED(result)) { HorribleDInputDeath("Failed enumerating attached game controllers", result); }
    else if (result != DI_OK) initprintf("    Enumerated game controllers with warning: %s\n",GetDInputError(result));

    if (inputdevices == (1|2))
    {
        initprintf("  - No game controllers found\n");
        UninitDirectInput();
        return TRUE;
    }

    *devicedef.did = NULL;

    //        initprintf("  - Creating %s device\n", devicedef.name);
    result = IDirectInput7_CreateDeviceEx(lpDI, bREFGUID guidDevs, bREFIID IID_IDirectInputDevice7, (LPVOID *)&dev, NULL);

    if (FAILED(result)) { HorribleDInputDeath("Failed creating device", result); }
    else if (result != DI_OK) initprintf("    Created device with warning: %s\n",GetDInputError(result));

    result = IDirectInputDevice7_QueryInterface(dev, bREFIID IID_IDirectInputDevice7, (LPVOID *)&dev2);
    IDirectInputDevice7_Release(dev);
    if (FAILED(result)) { HorribleDInputDeath("Failed querying DirectInput7 interface for device", result); }
    else if (result != DI_OK) initprintf("    Queried IDirectInputDevice7 interface with warning: %s\n",GetDInputError(result));

    result = IDirectInputDevice7_SetDataFormat(dev2, devicedef.df);
    if (FAILED(result)) { IDirectInputDevice7_Release(dev2); HorribleDInputDeath("Failed setting data format", result); }
    else if (result != DI_OK) initprintf("    Set data format with warning: %s\n",GetDInputError(result));

    di_inputevt = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (di_inputevt == NULL)
    {
        IDirectInputDevice7_Release(dev2);
        ShowErrorBox("Couldn't create event object");
        UninitDirectInput();
        return TRUE;
    }

    result = IDirectInputDevice7_SetEventNotification(dev2, di_inputevt);
    if (FAILED(result)) { IDirectInputDevice7_Release(dev2); HorribleDInputDeath("Failed setting event object", result); }
    else if (result != DI_OK) initprintf("    Set event object with warning: %s\n",GetDInputError(result));

    IDirectInputDevice7_Unacquire(dev2);

    memset(&dipdw, 0, sizeof(dipdw));
    dipdw.diph.dwSize = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj = 0;
    dipdw.diph.dwHow = DIPH_DEVICE;
    dipdw.dwData = INPUT_BUFFER_SIZE;

    result = IDirectInputDevice7_SetProperty(dev2, bDIPROP_BUFFERSIZE, &dipdw.diph);
    if (FAILED(result)) { IDirectInputDevice7_Release(dev2); HorribleDInputDeath("Failed setting buffering", result); }
    else if (result != DI_OK) initprintf("    Set buffering with warning: %s\n",GetDInputError(result));

    // set up device
    {
        int32_t typecounts[3] = {0,0,0};

        memset(&didc, 0, sizeof(didc));
        didc.dwSize = sizeof(didc);
        result = IDirectInputDevice7_GetCapabilities(dev2, &didc);
        if (FAILED(result)) { IDirectInputDevice7_Release(dev2); HorribleDInputDeath("Failed getting controller capabilities", result); }
        else if (result != DI_OK) initprintf("    Fetched controller capabilities with warning: %s\n",GetDInputError(result));

        joynumaxes    = (uint8_t)didc.dwAxes;
        joynumbuttons = min(32,(uint8_t)didc.dwButtons);
        joynumhats    = (uint8_t)didc.dwPOVs;
        initprintf("Controller has %d axes, %d buttons, and %d hat(s).\n",joynumaxes,joynumbuttons,joynumhats);

        axisdefs = (struct _joydef *)Bcalloc(didc.dwAxes, sizeof(struct _joydef));
        buttondefs = (struct _joydef *)Bcalloc(didc.dwButtons, sizeof(struct _joydef));
        if (didc.dwPOVs)
            hatdefs = (struct _joydef *)Bcalloc(didc.dwPOVs, sizeof(struct _joydef));

        joyaxis = (int32_t *)Bcalloc(didc.dwAxes, sizeof(int32_t));
        if (didc.dwPOVs)
            joyhat = (int32_t *)Bcalloc(didc.dwPOVs, sizeof(int32_t));

        result = IDirectInputDevice7_EnumObjects(dev2, InitDirectInput_enumobjects, (LPVOID)typecounts, DIDFT_ALL);
        if (FAILED(result)) { IDirectInputDevice7_Release(dev2); HorribleDInputDeath("Failed getting controller features", result); }
        else if (result != DI_OK) initprintf("    Fetched controller features with warning: %s\n",GetDInputError(result));
    }

    *devicedef.did = dev2;

    di_devacquired = 0;

    return FALSE;
}


//
// UninitDirectInput() -- clean up DirectInput
//
static void UninitDirectInput(void)
{
    int32_t i;

    if (hDInputDLL) initprintf("Uninitializing DirectInput...\n");

    AcquireInputDevices(0);

    if (axisdefs)
    {
        for (i=joynumaxes-1; i>=0; i--) Bfree(axisdefs[i].name);
        DO_FREE_AND_NULL(axisdefs);
    }
    if (buttondefs)
    {
        for (i=joynumbuttons-1; i>=0; i--) Bfree(buttondefs[i].name);
        DO_FREE_AND_NULL(buttondefs);
    }
    if (hatdefs)
    {
        for (i=joynumhats-1; i>=0; i--) Bfree(hatdefs[i].name);
        DO_FREE_AND_NULL(hatdefs);
    }

    if (di_inputevt)
    {
        CloseHandle(di_inputevt);
        di_inputevt = NULL;
    }

    if (*devicedef.did)
    {
        IDirectInputDevice7_Release(*devicedef.did);
        *devicedef.did = NULL;
    }

    if (lpDI)
    {
        IDirectInput7_Release(lpDI);
        lpDI = NULL;
    }

    if (hDInputDLL)
    {
        FreeLibrary(hDInputDLL);
        hDInputDLL = NULL;
    }
}


//
// GetKeyNames() -- retrieves the names for all the keys on the keyboard
//
static void GetKeyNames(void)
{
    int32_t i;
    char tbuf[MAX_PATH], *cp;

    memset(key_names,0,sizeof(key_names));

    for (i=0; i<256; i++)
    {
        tbuf[0] = 0;
        GetKeyNameText((i>128?(i+128):i)<<16, tbuf, sizeof(key_names[i])-1);
        Bstrncpy(&key_names[i][0], tbuf, sizeof(key_names[i])-1);
        for (cp=key_names[i]; *cp; cp++)
            if (!(*cp>=32 && *cp<127))
                *cp = '?';
    }
}

const char *getjoyname(int32_t what, int32_t num)
{
    switch (what)
    {
    case 0:	// axis
        return ((unsigned)num > (unsigned)joynumaxes) ? NULL : (char *)axisdefs[num].name;
    case 1: // button
        return ((unsigned)num > (unsigned)joynumbuttons) ? NULL : (char *)buttondefs[num].name;
    case 2: // hat
        return ((unsigned)num > (unsigned)joynumhats) ? NULL : (char *)hatdefs[num].name;
    default:
        return NULL;
    }
}

//
// AcquireInputDevices() -- (un)acquires the input devices
//
static void AcquireInputDevices(char acquire)
{
    DWORD flags;
    HRESULT result;

    if (!hDInputDLL) return;
    if (!hWindow) return;

    if (acquire)
    {
        //  	  if (!appactive) return;     // why acquire when inactive?

        if (! *devicedef.did) return;

        IDirectInputDevice7_Unacquire(*devicedef.did);

        flags = DISCL_FOREGROUND|DISCL_NONEXCLUSIVE;

        result = IDirectInputDevice7_SetCooperativeLevel(*devicedef.did, hWindow, flags);
        if (FAILED(result))
            initprintf("IDirectInputDevice7_SetCooperativeLevel(%s): %s\n",
                       devicedef.name, GetDInputError(result));

        if (SUCCEEDED(IDirectInputDevice7_Acquire(*devicedef.did)))
            di_devacquired = 1;
        else
            di_devacquired = 0;

        return;
    }

    di_devacquired = 0;

    releaseallbuttons();

    if (! *devicedef.did) return;

    IDirectInputDevice7_Unacquire(*devicedef.did);

    result = IDirectInputDevice7_SetCooperativeLevel(*devicedef.did, hWindow, DISCL_FOREGROUND|DISCL_NONEXCLUSIVE);
    if (FAILED(result))
        initprintf("IDirectInputDevice7_SetCooperativeLevel(%s): %s\n",
                   devicedef.name, GetDInputError(result));
}

//
// ProcessInputDevices() -- processes the input devices
//
static inline void DI_PollJoysticks(void)
{
    DWORD dwElements = INPUT_BUFFER_SIZE;
    HRESULT result;
    DIDEVICEOBJECTDATA didod[INPUT_BUFFER_SIZE];
    int32_t i, ev;

    if (*devicedef.did)
    {
        result = IDirectInputDevice7_Poll(*devicedef.did);

        if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED)
        {
            if (SUCCEEDED(IDirectInputDevice7_Acquire(*devicedef.did)))
            {
                di_devacquired = 1;
                IDirectInputDevice7_Poll(*devicedef.did);
            }
            else di_devacquired = 0;
        }

        if (di_devacquired)
        {
            // use event objects so that we can quickly get indication of when data is ready
            // to be read and input events processed
            ev = WaitForSingleObject(di_inputevt, 0);

            if (ev != WAIT_OBJECT_0 || !lpDID)
                return;

            result = IDirectInputDevice7_GetDeviceData(lpDID, sizeof(DIDEVICEOBJECTDATA),
                     (LPDIDEVICEOBJECTDATA)&didod, &dwElements, 0);

            if (result != DI_OK || !dwElements) return;

            for (i=dwElements-1; i>=0; i--)
            {
                int32_t j;

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
static const char *GetDInputError(HRESULT code)
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

static int32_t timerlastsample=0;
int32_t timerticspersec=0;
static double msperu64tick = 0;
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
    int64_t t;

    if (win_timerfreq) return 0;	// already installed

    //    initprintf("Initializing timer\n");

    t = win_inittimer();
    if (t < 0)
        return t;

    timerticspersec = tickspersecond;
    QueryPerformanceCounter((LARGE_INTEGER *)&t);
    timerlastsample = (int32_t)(t*timerticspersec / win_timerfreq);

    usertimercallback = NULL;

    msperu64tick = 1000.0 / (double)getu64tickspersec();

    return 0;
}

//
// uninittimer() -- shut down timer
//
void uninittimer(void)
{
    if (!win_timerfreq) return;

    win_timerfreq=0;
    timerticspersec = 0;

    msperu64tick = 0;
}

//
// sampletimer() -- update totalclock
//
void sampletimer(void)
{
    int64_t i;
    int32_t n;

    if (!win_timerfreq) return;

    QueryPerformanceCounter((LARGE_INTEGER *)&i);
    n = (int32_t)((i*timerticspersec / win_timerfreq) - timerlastsample);

    if (n <= 0) return;

    totalclock += n;
    timerlastsample += n;

    if (usertimercallback) for (; n>0; n--) usertimercallback();
}


//
// getticks() -- returns the windows ticks count
//
uint32_t getticks(void)
{
    int64_t i;
    if (win_timerfreq == 0) return 0;
    QueryPerformanceCounter((LARGE_INTEGER *)&i);
    return (uint32_t)(i*longlong(1000)/win_timerfreq);
}

// high-resolution timers for profiling
uint64_t getu64ticks(void)
{
    return win_getu64ticks();
}

uint64_t getu64tickspersec(void)
{
    return win_timerfreq;
}

// Returns the time since an unspecified starting time in milliseconds.
ATTRIBUTE((flatten))
double gethiticks(void)
{
    return (double)getu64ticks() * msperu64tick;
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
static char                *lpOffscreen = NULL;
static LPDIRECTDRAWPALETTE  lpDDPalette = NULL;
static BOOL                 bDDrawInited = FALSE;
static DWORD                DDdwCaps = 0, DDdwCaps2 = 0;

// DIB stuff
static HDC      hDC         = NULL;	// opengl shares this
static HDC      hDCSection  = NULL;
static HBITMAP  hDIBSection = NULL;
static HPALETTE hPalette    = NULL;
static VOID    *lpPixels    = NULL;

static int32_t setgammaramp(LPDDGAMMARAMP gt);
static int32_t getgammaramp(LPDDGAMMARAMP gt);

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

#ifdef USE_OPENGL
static HWND hGLWindow = NULL;
#endif

int32_t setvideomode(int32_t x, int32_t y, int32_t c, int32_t fs)
{
    char inp;
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

    inp = di_devacquired;
    AcquireInputDevices(0);

    if (hWindow && gammabrightness)
    {
        setgammaramp(&sysgamma);
        gammabrightness = 0;
    }

    win_setvideomode(c);

    if (!silentvideomodeswitch)
        initprintf("Setting video mode %dx%d (%d-bit %s)\n",
                   x,y,c, ((fs&1) ? "fullscreen" : "windowed"));

    if (CreateAppWindow(modenum)) return -1;

    if (!gammabrightness)
    {
        //        float f = 1.0 + ((float)curbrightness / 10.0);
        if (getgammaramp(&sysgamma) >= 0) gammabrightness = 1;
        if (gammabrightness && setgamma() < 0) gammabrightness = 0;
    }

#if defined USE_OPENGL && defined USE_GLEXT
    if (hGLWindow && glinfo.vsync) bwglSwapIntervalEXT(vsync_renderlayer);
#endif
    if (inp) AcquireInputDevices(1);
    modechange=1;
    videomodereset = 0;
    OSD_ResizeDisplay(xres,yres);

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

int32_t setvsync(int32_t newSync)
{
#ifdef USE_OPENGL
    if (!glinfo.vsync)
    {
        vsync_renderlayer = 0;
        return 0;
    }
# ifdef USE_GLEXT
    bwglSwapIntervalEXT(newSync);
# endif
#endif

    vsync_renderlayer = newSync;

    return newSync;
}

#ifdef USE_OPENGL
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

    return DDENUMRET_OK;
}

static int sortmodes(const void *a_, const void *b_)
{
    int32_t x;

    const struct validmode_t *a = (const struct validmode_t *)a_;
    const struct validmode_t *b = (const struct validmode_t *)b_;

    if ((x = a->fs   - b->fs)   != 0) return x;
    if ((x = a->bpp  - b->bpp)  != 0) return x;
    if ((x = a->xdim - b->xdim) != 0) return x;
    if ((x = a->ydim - b->ydim) != 0) return x;

    return 0;
}
void getvalidmodes(void)
{
    int32_t cdepths[2] = { 8, 0 };
    int32_t i, j, maxx=0, maxy=0;
    HRESULT result;

#ifdef USE_OPENGL
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
#ifdef USE_OPENGL
    cdsenummodes();
#endif

    // Windowed modes can be as big as the current desktop resolution, but not bigger.
    maxx = desktopxdim+1;
    maxy = desktopydim+1;

    // add windowed modes next
    for (j=0; j < 2; j++)
    {
        if (cdepths[j] == 0) continue;
        for (i=0; defaultres[i][0]; i++)
            CHECK(defaultres[i][0],defaultres[i][1])
            ADDMODE(defaultres[i][0],defaultres[i][1],cdepths[j],0,-1)
        }

    qsort((void *)validmode, validmodecnt, sizeof(struct validmode_t), &sortmodes);

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
#ifdef DEBUG_FRAME_LOCKING
uint32_t begindrawing_line[BEGINDRAWING_SIZE];
const char *begindrawing_file[BEGINDRAWING_SIZE];
void begindrawing_real(void)
#else
void begindrawing(void)
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

    if (lockcount++ > 0)
        return;		// already locked

    if (offscreenrendering) return;

    frameplace = fullscreen ? (intptr_t)lpOffscreen : (intptr_t)lpPixels;

    if (!modechange) return;

    modechange=0;

    if (!fullscreen)
    {
        bytesperline = xres|4;
    }
    else
    {
        bytesperline = xres|1;
    }

    calc_ylookup(bytesperline, ydim);
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

#ifdef USE_OPENGL
    if (bpp > 8)
    {
        if (palfadedelta)
            fullscreen_tint_gl(palfadergb.r, palfadergb.g, palfadergb.b, palfadedelta);

        bwglSwapBuffers(hDC);
        return;
    }
#endif

    //    w = 1;	// wait regardless. ken thinks it's better to do so.

    if (offscreenrendering) return;

    if (lockcount)
    {
        initprintf("Frame still locked %d times when showframe() called.\n", lockcount);
        while (lockcount) enddrawing();
    }

    if (!fullscreen)
    {
        BitBlt(hDC, 0, 0, xres, yres, hDCSection, 0, 0, SRCCOPY);
        return;
    }
/*
    else
        IDirectDraw_WaitForVerticalBlank(lpDD, DDWAITVB_BLOCKBEGIN, NULL);
*/

    if (!w && (IDirectDrawSurface_GetBltStatus(lpDDSBack, DDGBS_CANBLT) == DDERR_WASSTILLDRAWING ||
               IDirectDrawSurface_GetFlipStatus(lpDDSPrimary, DDGFS_CANFLIP) == DDERR_WASSTILLDRAWING))
        return;

    // lock the backbuffer surface
    Bmemset(&ddsd, 0, sizeof(ddsd));
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

    if (bpp > 8) return 0;	// no palette in opengl

    Bmemcpy(lpal.palPalEntry, curpalettefaded, sizeof(lpal.palPalEntry));
    for (i=start, n=num-1; n>0; i++, n--)
        curpalettefaded[i].f = lpal.palPalEntry[i].peFlags = PC_NOCOLLAPSE;

    if (fullscreen)
    {
        if (!lpDDPalette) return -1;
        result = IDirectDrawPalette_SetEntries(lpDDPalette, 0, 0, 256, (LPPALETTEENTRY)lpal.palPalEntry);
        if (result != DD_OK)
        {
            initprintf("Palette set failed: %s\n", GetDDrawError(result));
            return -1;
        }

        return 0;
    }

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
        Bfree(rgb);
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
static int32_t setgammaramp(LPDDGAMMARAMP gt)
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

        hr = IDirectDrawSurface_QueryInterface(lpDDSPrimary, bREFIID IID_IDirectDrawGammaControl, (LPVOID *)&gam);
        if (hr != DD_OK)
        {
            //            ShowDDrawErrorBox("Error querying gamma control", hr);
            initprintf("Error querying gamma control: %s\n",GetDDrawError(hr));
            return -1;
        }

        hr = IDirectDrawGammaControl_SetGammaRamp(gam, 0, gt);
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

    return 0;
}

int32_t setgamma(void)
{
    int32_t i;
    static DDGAMMARAMP gammaTable;
    float gamma = max(0.1f,min(4.f,vid_gamma));
    float contrast = max(0.1f,min(3.f,vid_contrast));
    float bright = max(-0.8f,min(0.8f,vid_brightness));

    double invgamma = 1 / gamma;
    double norm = pow(255., invgamma - 1);

    if (!hWindow) return -1;

    if (winlayer_have_ATI && bpp==8 && fullscreen)
        return -1;

    // This formula is taken from Doomsday

    for (i = 0; i < 256; i++)
    {
        double val = i * contrast - (contrast - 1) * 127;
        if (gamma != 1) val = pow(val, invgamma) / norm;
        val += bright * 128;

        gammaTable.red[i] = gammaTable.green[i] = gammaTable.blue[i] = (WORD)max(0.f,(double)min(0xffff,val*256));
    }

    return setgammaramp(&gammaTable);
}

static int32_t getgammaramp(LPDDGAMMARAMP gt)
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

        hr = IDirectDrawSurface_QueryInterface(lpDDSPrimary, bREFIID IID_IDirectDrawGammaControl, (LPVOID *)&gam);
        if (hr != DD_OK)
        {
            ShowDDrawErrorBox("Error querying gamma control", hr);
            return -1;
        }

        hr = IDirectDrawGammaControl_GetGammaRamp(gam, 0, gt);
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
    aDirectDrawEnumerate = (HRESULT(WINAPI *)(LPDDENUMCALLBACK, LPVOID))GetProcAddress(hDDrawDLL, "DirectDrawEnumerateA");
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
    aDirectDrawCreate = (HRESULT(WINAPI *)(GUID *, LPDIRECTDRAW *, IUnknown *))GetProcAddress(hDDrawDLL, "DirectDrawCreate");
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

    // if (lpOffscreen)
    {
        //        initprintf("  - Freeing offscreen buffer\n");
        DO_FREE_AND_NULL(lpOffscreen);
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
    lpOffscreen = (char *)Bmalloc((width|1)*height);
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

    result = IDirectDraw_CreatePalette(lpDD, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
                                       (LPPALETTEENTRY)curpalettefaded, &lpDDPalette, NULL);
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

#ifdef USE_OPENGL
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

    loadglextensions();

# if defined DEBUGGINGAIDS && defined USE_GLEXT
    // We should really be checking for the new WGL extension string instead
    // Enable this to leverage ARB_debug_output
    if (bwglCreateContextAttribsARB) {
        HGLRC debuggingContext = hGLRC;

        // This corresponds to WGL_CONTEXT_FLAGS_ARB set to WGL_CONTEXT_DEBUG_BIT_ARB
        // I'm too lazy to get a new wglext.h
        int attribs[] = {
            0x2094, 0x1,
            0
        };

        debuggingContext = bwglCreateContextAttribsARB(hDC, NULL, attribs);

        if (debuggingContext) {
            bwglDeleteContext(hGLRC);
            bwglMakeCurrent(hDC, debuggingContext);
            hGLRC = debuggingContext;

            // This should be able to get the ARB_debug_output symbols
            loadglextensions();
        }
    }
# endif

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

        glinfo.vendor     = (char const *)bglGetString(GL_VENDOR);
        glinfo.renderer   = (char const *)bglGetString(GL_RENDERER);
        glinfo.version    = (char const *)bglGetString(GL_VERSION);
        glinfo.extensions = (char const *)bglGetString(GL_EXTENSIONS);

        // GL driver blacklist

        if (!Bstrcmp(glinfo.vendor,"Microsoft Corporation")) err = 1;
        else if (!Bstrcmp(glinfo.vendor,"SiS")) err = 1;
        else if (!Bstrcmp(glinfo.vendor,"3Dfx Interactive Inc.")) err = 1;
#ifdef POLYMER
        else if (!Bstrcmp(glinfo.vendor, "Intel"))
            pr_ati_fboworkaround = 1;
#endif
        else
        {
            if (!Bstrcmp(glinfo.vendor,"ATI Technologies Inc."))
            {
                winlayer_have_ATI = 1;
#ifdef POLYMER
                pr_ati_fboworkaround = 1;
#endif
                if (Bstrstr(glinfo.renderer,"Radeon X1"))
                {
#ifdef USE_GLEXT
                    r_vbos = 0;
#endif
#ifdef POLYMER
                    pr_ati_nodepthoffset = 1;
                    initprintf("Enabling ATI R520 polygon offset workaround.\n");
#endif
                }
#ifdef POLYMER
                else
                    pr_ati_nodepthoffset = 0;
#endif
            }
#ifdef POLYMER
            else
                pr_ati_fboworkaround = 0;
#endif
        }

#ifdef POLYMER
        if (pr_ati_fboworkaround)
            initprintf("Enabling Intel/ATI FBO color attachment workaround.\n");
#endif

        if (!forcegl && err)
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

        glinfo.maxanisotropy = 1.0;
        glinfo.bgra = 0;
        glinfo.texcompr = 0;

        // process the extensions string and flag stuff we recognize
        p = (GLubyte *)Bstrdup(glinfo.extensions);
        p3 = p;
        while ((p2 = (GLubyte *)Bstrtoken(p3==p?(char *)p:NULL, " ", (char **)&p3, 1)) != NULL)
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
            else if (!Bstrcmp((char *)p2, "GL_ARB_texture_compression") && Bstrcmp(glinfo.vendor,"ATI Technologies Inc."))
            {
                // support texture compression
                glinfo.texcompr = 1;

#ifdef DYNAMIC_GLEXT
                if (!bglCompressedTexImage2DARB || !bglGetCompressedTexImageARB)
                {
                    // lacking the necessary extensions to do this
                    initprintf("Warning: the GL driver lacks necessary functions to use caching\n");
                    glinfo.texcompr = 0;
                }
#endif
            }
            else if (!Bstrcmp((char *)p2, "GL_ARB_texture_non_power_of_two"))
            {
                // support non-power-of-two texture sizes
                glinfo.texnpot = 1;
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
            else if (!Bstrcmp((char *)p2, "GL_ARB_buffer_storage"))
            {
                glinfo.bufferstorage = 1;
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
#ifdef USE_OPENGL
            ReleaseOpenGL();
#endif
        }
        else
        {
            ReleaseDirectDrawSurfaces();
        }

        if (!fs && fullscreen)
        {
            // restore previous display mode and set to normal cooperative level
            RestoreDirectDrawMode();
#ifdef USE_OPENGL
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
                      WindowClass,
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
#ifdef USE_OPENGL
            // yes, start up opengl
            if (SetupOpenGL(width,height,bitspp))
                return TRUE;
#endif
        }
        else if (SetupDIB(width,height)) // use DIB section
            return TRUE;

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

#ifdef USE_OPENGL
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
#ifdef USE_OPENGL
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

    xres = width;
    yres = height;
    bpp = bitspp;
    fullscreen = fs;
    curvidmode = modenum;

    frameplace = 0;
    lockcount = 0;

    // bytesperline is set when framebuffer is locked
    //bytesperline = width;

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
        setgammaramp(&sysgamma);
        gammabrightness = 0;
    }

#ifdef USE_OPENGL
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
static const char *GetDDrawError(HRESULT code)
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
// WndProcCallback() -- the Windows window callback
//
static LRESULT CALLBACK WndProcCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    /*    RECT rect;
    POINT pt;
    HRESULT result; */

#ifdef USE_OPENGL
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

    case WM_ACTIVATE:
    {
        appactive = (LOWORD(wParam) != 0);
#ifdef USE_OPENGL
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
                    grabmouse(AppMouseGrab);
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

        Win_SetKeyboardLayoutUS(appactive);

        if (backgroundidle)
            SetPriorityClass(GetCurrentProcess(),
                             appactive ? NORMAL_PRIORITY_CLASS : IDLE_PRIORITY_CLASS);

        if (appactive)
        {
            SetForegroundWindow(hWindow);
            SetFocus(hWindow);
        }

        Bmemset(keystatus, 0, sizeof(keystatus));
        AcquireInputDevices(appactive);
        return 0;
    }

    case WM_PALETTECHANGED:
        // someone stole the palette so try and steal it back
        if (bDDrawInited && bpp == 8 && fullscreen)
        {
            int32_t result;

            // PK: for me, happens on Vista when changing from fullscreen 8-bit to 32-bit
            if (!lpDDSPrimary || !lpDDPalette)
                break;

            result = IDirectDrawSurface_SetPalette(lpDDSPrimary, lpDDPalette);

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

    case WM_ENTERMENULOOP:
    case WM_ENTERSIZEMOVE:
        AcquireInputDevices(0);
        return 0;
    case WM_EXITMENULOOP:
    case WM_EXITSIZEMOVE:
        AcquireInputDevices(1);
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
    wcx.hIcon	= (HICON)LoadImage(hInstance, MAKEINTRESOURCE(100), IMAGE_ICON,
                            GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    wcx.hCursor	= LoadCursor(NULL, IDC_ARROW);
    wcx.hbrBackground = (HBRUSH)COLOR_GRAYTEXT;
    wcx.lpszMenuName = NULL;
    wcx.lpszClassName = WindowClass;
    wcx.hIconSm	= (HICON)LoadImage(hInstance, MAKEINTRESOURCE(100), IMAGE_ICON,
                            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    if (!RegisterClassEx(&wcx))
    {
        ShowErrorBox("Failed to register window class");
        return TRUE;
    }

    window_class_registered = TRUE;

    return FALSE;
}
