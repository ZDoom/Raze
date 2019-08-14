
#include "timer.h"

#include "build.h"
#include "compat.h"

#include <chrono>

using namespace std;
using namespace chrono;

static time_point<steady_clock> timerlastsample;
static int timerticspersec;
static void(*usertimercallback)(void) = NULL;

int      timerGetRate(void)     { return timerticspersec; }
uint32_t timerGetTicks(void)    { return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count(); }
uint64_t timerGetTicksU64(void) { return steady_clock::now().time_since_epoch().count(); }
uint64_t timerGetFreqU64(void)  { return steady_clock::period::den; }

// Returns the time since an unspecified starting time in milliseconds.
// (May be not monotonic for certain configurations.)
double timerGetHiTicks(void) { return duration<double, nano>(steady_clock::now().time_since_epoch()).count() / 1000000.0; }

int timerInit(int const tickspersecond)
{
    timerticspersec = tickspersecond;
    timerlastsample = steady_clock::now();

    usertimercallback = NULL;

    return 0;
}

ATTRIBUTE((flatten)) void timerUpdate(void)
{
    auto time = steady_clock::now();
    int n = (time - timerlastsample).count() * (double)timerticspersec / timerGetFreqU64();

    if (n <= 0) return;

    totalclock += n;
    timerlastsample = time;

    if (usertimercallback)
        for (; n > 0; n--) usertimercallback();
}

void(*timerSetCallback(void(*callback)(void)))(void)
{
    void(*oldtimercallback)(void);

    oldtimercallback = usertimercallback;
    usertimercallback = callback;

    return oldtimercallback;
}
