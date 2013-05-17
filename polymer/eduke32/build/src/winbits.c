// Windows layer-independent code

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "build.h"
#include "baselayer.h"
#include "osd.h"
#include "cache1d.h"
#include "winbits.h"

#ifndef DEBUGGINGAIDS
# define DISABLE_EXCEPTIONS
# include "nedmalloc.h"
#endif

#if defined(_M_X64) || defined(__amd64__) || defined(__x86_64__)
# define EBACKTRACEDLL "ebacktrace1-64.dll"
# define NEDMALLOCDLL "nedmalloc-64.dll"
#else
# define EBACKTRACEDLL "ebacktrace1.dll"
# define NEDMALLOCDLL "nedmalloc.dll"
#endif

int32_t backgroundidle = 1;

int64_t win_timerfreq = 0;

char silentvideomodeswitch = 0;

static char taskswitching = 1;

static HANDLE instanceflag = NULL;

static OSVERSIONINFOEX osv;

static HMODULE nedhandle = NULL;

static int32_t togglecomp = 1;

//
// CheckWinVersion() -- check to see what version of Windows we happen to be running under
//
BOOL CheckWinVersion(void)
{
    ZeroMemory(&osv, sizeof(osv));
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    // we don't like anything older than Windows XP

    if (!GetVersionEx((LPOSVERSIONINFOA)&osv)) return FALSE;

    if (osv.dwMajorVersion >= 6) return TRUE;
    if (osv.dwMajorVersion == 5 && osv.dwMinorVersion >= 1) return TRUE;

    return FALSE;
}

static void win_printversion(void)
{
    const char *ver = "";

    switch (osv.dwPlatformId)
    {
    case VER_PLATFORM_WIN32_NT:
        if (osv.dwMajorVersion == 5)
        {
            switch (osv.dwMinorVersion)
            {
            case 1:
                ver = "XP";
                break;
            case 2:
                ver = osv.wProductType == VER_NT_WORKSTATION ? "XP x64" : "Server 2003";
                break;
            }
            break;
        }

        if (osv.dwMajorVersion == 6)
        {
            switch (osv.dwMinorVersion)
            {
            case 0:
                ver = osv.wProductType == VER_NT_WORKSTATION ? "Vista" : "Server 2008";
                break;
            case 1:
                ver = osv.wProductType == VER_NT_WORKSTATION ? "7" : "Server 2008 R2";
                break;
            case 2:
                ver = osv.wProductType == VER_NT_WORKSTATION ? "8" : "Server 2012";
                break;
            }
            break;
        }
        break;
    }

    initprintf("Windows %s %s (build %lu.%lu.%lu)\n", ver, osv.szCSDVersion,
               osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber);
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
// high-resolution timers for profiling
//
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

uint64_t win_gethiticks(void)
{
    uint64_t i;
    if (win_timerfreq == 0) return 0;
    QueryPerformanceCounter((LARGE_INTEGER *)&i);
    return i;
}



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


//
// win_open(), win_init(), win_setvideomode(), win_uninit(), win_close() -- shared code
//
void win_open(void)
{
#ifndef DEBUGGINGAIDS
    if ((nedhandle = LoadLibrary(NEDMALLOCDLL)))
    {
#ifdef __cplusplus
        nedalloc::nedpool_t *(WINAPI *nedcreatepool)(size_t, int);
        if ((nedcreatepool = (nedalloc::nedpool_t *(WINAPI *)(size_t, int))GetProcAddress(nedhandle, "nedcreatepool")))
#else
        nedpool *(WINAPI *nedcreatepool)(size_t, int);
        if ((nedcreatepool = (void *)GetProcAddress(nedhandle, "nedcreatepool")))
#endif
            nedcreatepool(SYSTEM_POOL_SIZE, -1);
    }
#else
    LoadLibraryA(EBACKTRACEDLL);
/*
        wm_msgbox("boo","didn't load backtrace DLL (code %d)\n", (int)GetLastError());
    else
        wm_msgbox("yay","loaded backtrace DLL\n");
*/
#endif

    instanceflag = CreateSemaphore(NULL, 1,1, WindowClass);
}

void win_init(void)
{
    uint32_t i;

    cvar_t cvars_win[] =
    {
        { "r_togglecomposition","enable/disable toggle of desktop composition when initializing screen modes",(void *) &togglecomp, CVAR_BOOL, 0, 1 },
    };

    for (i=0; i<sizeof(cvars_win)/sizeof(cvars_win[0]); i++)
    {
        if (OSD_RegisterCvar(&cvars_win[i]))
            continue;

        OSD_RegisterFunction(cvars_win[i].name, cvars_win[i].desc, osdcmd_cvar_set);
    }

    win_printversion();

    if (nedhandle)
        initprintf("Initialized nedmalloc\n");
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
