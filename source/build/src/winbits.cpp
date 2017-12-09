// Windows layer-independent code

#include "compat.h"
#include "build.h"
#include "baselayer.h"
#include "osd.h"
#include "cache1d.h"



#include "winbits.h"

#ifdef BITNESS64
# define EBACKTRACEDLL "ebacktrace1-64.dll"
#else
# define EBACKTRACEDLL "ebacktrace1.dll"
#endif

int32_t backgroundidle = 1;

int64_t win_timerfreq = 0;

char silentvideomodeswitch = 0;

static char taskswitching = 1;

static HANDLE instanceflag = NULL;

static OSVERSIONINFOEX osv;

static int32_t togglecomp = 0;

FARPROC pwinever;

//
// CheckWinVersion() -- check to see what version of Windows we happen to be running under
//
BOOL CheckWinVersion(void)
{
    HMODULE hntdll = GetModuleHandle("ntdll.dll");

    if (hntdll)
        pwinever = GetProcAddress(hntdll, "wine_get_version");

    ZeroMemory(&osv, sizeof(osv));
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (!GetVersionEx((LPOSVERSIONINFOA)&osv)) return FALSE;

    return TRUE;
}

static void win_printversion(void)
{
    const char *ver = "";

    switch (osv.dwPlatformId)
    {
        case VER_PLATFORM_WIN32_WINDOWS:
            if (osv.dwMinorVersion < 10)
                ver = "95";
            else if (osv.dwMinorVersion < 90)
                ver = "98";
            else
                ver = "ME";
            break;

        case VER_PLATFORM_WIN32_NT:
            switch (osv.dwMajorVersion)
            {
                case 5:
                    switch (osv.dwMinorVersion)
                    {
                        case 0: ver = "2000"; break;
                        case 1: ver = "XP"; break;
                        case 2: ver = osv.wProductType == VER_NT_WORKSTATION ? "XP x64" : "Server 2003"; break;
                    }
                    break;

                case 6:
                    switch (osv.dwMinorVersion)
                    {
                        case 0: ver = osv.wProductType == VER_NT_WORKSTATION ? "Vista" : "Server 2008"; break;
                        case 1: ver = osv.wProductType == VER_NT_WORKSTATION ? "7" : "Server 2008 R2"; break;
                        case 2: ver = osv.wProductType == VER_NT_WORKSTATION ? "8" : "Server 2012"; break;
                        case 3: ver = osv.wProductType == VER_NT_WORKSTATION ? "8.1" : "Server 2012 R2"; break;
                    }
                    break;

                case 10:
                    switch (osv.dwMinorVersion)
                    {
                        case 0: ver = osv.wProductType == VER_NT_WORKSTATION ? "10" : "Server 2016"; break;
                    }
                    break;
            }
            break;
    }

    char *str = (char *)Bcalloc(1, 256);
    int l;

    if (pwinever)
        l = Bsprintf(str, "Wine %s identifying as Windows %s", (char *)pwinever(), ver);
    else
        l = Bsprintf(str, "Windows %s", ver);

    // service packs
    if (osv.szCSDVersion[0])
    {
        str[l] = 32;
        Bstrcat(&str[l], osv.szCSDVersion);
    }

    initprintf("Running on %s (build %lu.%lu.%lu)\n", str, osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber);
    Bfree(str);
}

//
// win_allowtaskswitching() -- captures/releases alt+tab hotkeys
//
void win_allowtaskswitching(int32_t onf)
{
    if (onf == taskswitching) return;
    taskswitching = onf;

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
// high-resolution timers for profiling
//
#if defined(RENDERTYPEWIN) || SDL_MAJOR_VERSION==1
int32_t win_inittimer(void)
{
    int64_t t;

    if (win_timerfreq) return 0;	// already installed

    // OpenWatcom seems to want us to query the value into a local variable
    // instead of the global 'win_timerfreq' or else it gets pissed with an
    // access violation
    if (!QueryPerformanceFrequency((LARGE_INTEGER *)&t))
    {
        ShowErrorBox("Failed fetching timer frequency");
        return -1;
    }
    win_timerfreq = t;

    return 0;
}

uint64_t win_getu64ticks(void)
{
    uint64_t i;
    if (win_timerfreq == 0) return 0;
    QueryPerformanceCounter((LARGE_INTEGER *)&i);
    return i;
}
#endif


static void ToggleDesktopComposition(BOOL compEnable)
{
    static HMODULE              hDWMApiDLL        = NULL;
    static HRESULT(WINAPI *aDwmEnableComposition)(UINT);

    if (!hDWMApiDLL && (hDWMApiDLL = LoadLibrary("DWMAPI.DLL")))
        aDwmEnableComposition = (HRESULT(WINAPI *)(UINT))GetProcAddress(hDWMApiDLL, "DwmEnableComposition");

    if (aDwmEnableComposition)
    {
        aDwmEnableComposition(compEnable);
        if (!silentvideomodeswitch)
            initprintf("%sabling desktop composition...\n", (compEnable) ? "En" : "Dis");
    }
}

typedef void (*dllSetString)(const char*);

//
// win_open(), win_init(), win_setvideomode(), win_uninit(), win_close() -- shared code
//
void win_open(void)
{
#ifdef DEBUGGINGAIDS
    HMODULE ebacktrace = LoadLibraryA(EBACKTRACEDLL);
    if (ebacktrace)
    {
        dllSetString SetTechnicalName = (dllSetString) GetProcAddress(ebacktrace, "SetTechnicalName");
        dllSetString SetProperName = (dllSetString) GetProcAddress(ebacktrace, "SetProperName");

        if (SetTechnicalName)
            SetTechnicalName(AppTechnicalName);

        if (SetProperName)
            SetProperName(AppProperName);
    }
#endif

    instanceflag = CreateSemaphore(NULL, 1,1, WindowClass);
}

void win_init(void)
{
    uint32_t i;

    static osdcvardata_t cvars_win[] =
    {
        { "r_togglecomposition","enable/disable toggle of desktop composition when initializing screen modes",(void *) &togglecomp, CVAR_BOOL, 0, 1 },
    };

    for (i=0; i<ARRAY_SIZE(cvars_win); i++)
        OSD_RegisterCvar(&cvars_win[i], osdcmd_cvar_set);

    win_printversion();
}

void win_setvideomode(int32_t c)
{
    if (togglecomp && osv.dwMajorVersion == 6 && osv.dwMinorVersion < 2)
        ToggleDesktopComposition(c < 16);
}

void win_uninit(void)
{
    win_allowtaskswitching(1);
}

void win_close(void)
{
    if (instanceflag) CloseHandle(instanceflag);
}


// Keyboard layout switching

static void switchlayout(char const * layout)
{
    char layoutname[KL_NAMELENGTH];

    GetKeyboardLayoutName(layoutname);

    if (!Bstrcmp(layoutname, layout))
        return;

    initprintf("Switching keyboard layout from %s to %s\n", layoutname, layout);
    LoadKeyboardLayout(layout, KLF_ACTIVATE|KLF_SETFORPROCESS|KLF_SUBSTITUTE_OK);
}

static char OriginalLayoutName[KL_NAMELENGTH];

void Win_GetOriginalLayoutName(void)
{
    GetKeyboardLayoutName(OriginalLayoutName);
}

void Win_SetKeyboardLayoutUS(int const toggle)
{
    static int currentstate;

    if (toggle != currentstate)
    {
        if (toggle)
        {
            // 00000409 is "American English"
            switchlayout("00000409");
            currentstate = toggle;
        }
        else if (OriginalLayoutName[0])
        {
            switchlayout(OriginalLayoutName);
            currentstate = toggle;
        }
    }
}


//
// ShowErrorBox() -- shows an error message box
//
void ShowErrorBox(const char *m)
{
    TCHAR msg[1024];

    wsprintf(msg, "%s: %s", m, GetWindowsErrorMsg(GetLastError()));
    MessageBox(0, msg, apptitle, MB_OK|MB_ICONSTOP);
}

//
// GetWindowsErrorMsg() -- gives a pointer to a static buffer containing the Windows error message
//
LPTSTR GetWindowsErrorMsg(DWORD code)
{
    static TCHAR lpMsgBuf[1024];

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, code,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)lpMsgBuf, 1024, NULL);

    return lpMsgBuf;
}


//
// Miscellaneous
//

int32_t addsearchpath_ProgramFiles(const char *p)
{
    int32_t returncode = -1, i;
    const char *ProgramFiles[2] = { Bgetenv("ProgramFiles"), Bgetenv("ProgramFiles(x86)") };

    for (i = 0; i < 2; ++i)
    {
        if (ProgramFiles[i])
        {
            char *buffer = (char*)Bmalloc((strlen(ProgramFiles[i])+1+strlen(p)+1)*sizeof(char));
            Bsprintf(buffer,"%s/%s",ProgramFiles[i],p);
            if (addsearchpath(buffer) == 0) // if any work, return success
                returncode = 0;
            Bfree(buffer);
        }
    }

    return returncode;
}

int32_t win_buildargs(char **argvbuf)
{
    int32_t buildargc = 0;

    *argvbuf = Bstrdup(GetCommandLine());

    if (*argvbuf)
    {
        char quoted = 0, instring = 0, swallownext = 0;
        char *wp;
        for (const char *p = wp = *argvbuf; *p; p++)
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

    return buildargc;
}


// Workaround for a bug in mingwrt-4.0.0 and up where a function named main() in misc/src/libcrt/gdtoa/qnan.c takes precedence over the proper one in src/libcrt/crt/main.c.
#if (defined __MINGW32__ && EDUKE32_GCC_PREREQ(4,8)) || EDUKE32_CLANG_PREREQ(3,4)
# undef main
# include "mingw_main.cpp"
#endif
