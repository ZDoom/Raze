// Windows layer-independent code

#define NOMINMAX
#include <Windows.h>
#include "compat.h"
#include "build.h"
#include "baselayer.h"
#include "osd.h"
#include "cache1d.h"
#include "zstring.h"
#include "winbits.h"

FString progdir;
//
// CheckWinVersion() -- check to see what version of Windows we happen to be running under (stripped down to what is actually still supported.)
//

//==========================================================================
//
// win_buildargs
//
// This should be removed once everything can use the FArgs list.
//
//==========================================================================


int32_t win_buildargs(char **argvbuf)
{
    int32_t buildargc = 0;
	
	FString cmdline_utf8 = FString(GetCommandLineW());

    *argvbuf = Xstrdup(cmdline_utf8.GetChars());

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

