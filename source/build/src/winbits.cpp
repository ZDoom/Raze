// Windows layer-independent code

#include "compat.h"
#include "build.h"
#include "baselayer.h"
#include "osd.h"
#include "cache1d.h"



#include "winbits.h"

int32_t backgroundidle = 1;

char silentvideomodeswitch = 0;

static char taskswitching = 1;

static HANDLE instanceflag = NULL;

static OSVERSIONINFOEXA osv;

static int32_t togglecomp = 0;

FARPROC pwinever;

//
// CheckWinVersion() -- check to see what version of Windows we happen to be running under
//

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

    char *str = (char *)Xcalloc(1, 256);
    int l;

    if (pwinever)
        l = Bsprintf(str, "Wine %s, identifying as Windows %s", (char *)pwinever(), ver);
    else
        l = Bsprintf(str, "Windows %s", ver);

    // service packs
    if (osv.szCSDVersion[0])
    {
        str[l] = 32;
        Bstrcat(&str[l], osv.szCSDVersion);
    }

    initprintf("Running on %s (build %lu.%lu.%lu)\n", str, osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber);
    Xfree(str);
}


static void ToggleDesktopComposition(BOOL compEnable)
{
    static HMODULE              hDWMApiDLL        = NULL;
    static HRESULT(WINAPI *aDwmEnableComposition)(UINT);

    if (!hDWMApiDLL && (hDWMApiDLL = LoadLibraryA("DWMAPI.DLL")))
        aDwmEnableComposition = (HRESULT(WINAPI *)(UINT))(void (*)(void))GetProcAddress(hDWMApiDLL, "DwmEnableComposition");

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
    instanceflag = CreateSemaphoreA(NULL, 1,1, WindowClass);
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
}

void win_close(void)
{
    if (instanceflag) CloseHandle(instanceflag);
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
            char *buffer = (char*)Xmalloc((strlen(ProgramFiles[i])+1+strlen(p)+1)*sizeof(char));
            Bsprintf(buffer,"%s/%s",ProgramFiles[i],p);
            if (addsearchpath(buffer) == 0) // if any work, return success
                returncode = 0;
            Xfree(buffer);
        }
    }

    return returncode;
}

int32_t win_buildargs(char **argvbuf)
{
    int32_t buildargc = 0;

    *argvbuf = Xstrdup(GetCommandLineA());

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


//==========================================================================
//
// CalculateCPUSpeed
//
// Make a decent guess at how much time elapses between TSC steps. This can
// vary over runtime depending on power management settings, so should not
// be used anywhere that truely accurate timing actually matters.
//
//==========================================================================

double PerfToSec, PerfToMillisec;
#include "stats.h"

static void CalculateCPUSpeed()
{
	LARGE_INTEGER freq;

	QueryPerformanceFrequency(&freq);

	if (freq.QuadPart != 0)
	{
		LARGE_INTEGER count1, count2;
		cycle_t       ClockCalibration;
		DWORD         min_diff;

		ClockCalibration.Reset();

		// Count cycles for at least 55 milliseconds.
		// The performance counter may be very low resolution compared to CPU
		// speeds today, so the longer we count, the more accurate our estimate.
		// On the other hand, we don't want to count too long, because we don't
		// want the user to notice us spend time here, since most users will
		// probably never use the performance statistics.
		min_diff = freq.LowPart * 11 / 200;

		// Minimize the chance of task switching during the testing by going very
		// high priority. This is another reason to avoid timing for too long.
		SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

		// Make sure we start timing on a counter boundary.
		QueryPerformanceCounter(&count1);
		do
		{
			QueryPerformanceCounter(&count2);
		} while (count1.QuadPart == count2.QuadPart);

		// Do the timing loop.
		ClockCalibration.Clock();
		do
		{
			QueryPerformanceCounter(&count1);
		} while ((count1.QuadPart - count2.QuadPart) < min_diff);
		ClockCalibration.Unclock();

		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

		PerfToSec = double(count1.QuadPart - count2.QuadPart) / (double(ClockCalibration.GetRawCounter()) * freq.QuadPart);
		PerfToMillisec = PerfToSec * 1000.0;
	}
}

class Initer
{
public:
	Initer() { CalculateCPUSpeed(); }
};

static Initer initer;


// Workaround for a bug in mingwrt-4.0.0 and up where a function named main() in misc/src/libcrt/gdtoa/qnan.c takes precedence over the proper one in src/libcrt/crt/main.c.
#if (defined __MINGW32__ && EDUKE32_GCC_PREREQ(4,8)) || EDUKE32_CLANG_PREREQ(3,4)
# undef main
# include "mingw_main.cpp"
#endif
