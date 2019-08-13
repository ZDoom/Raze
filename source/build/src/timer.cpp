
#include "timer.h"

#include "build.h"
#include "compat.h"

#include <chrono>

using namespace std;
using namespace chrono;

static int32_t timerlastsample;
static int32_t timerticspersec=0;
static void(*usertimercallback)(void) = NULL;

int32_t timerGetRate(void) { return timerticspersec; }
void timerUninit(void) { timerticspersec = 0; }

uint32_t timerGetTicks(void) { return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count(); }
uint64_t timerGetTicksU64(void) { return high_resolution_clock::now().time_since_epoch().count(); }
uint64_t timerGetFreqU64(void) { return high_resolution_clock::period::den; }

// Returns the time since an unspecified starting time in milliseconds.
// (May be not monotonic for certain configurations.)
ATTRIBUTE((flatten))
double timerGetHiTicks(void) { return duration<double, milli>(high_resolution_clock::now().time_since_epoch()).count(); }

int32_t timerInit(int32_t const tickspersecond)
{
    timerticspersec = tickspersecond;
    timerlastsample = timerGetTicksU64() * timerticspersec / timerGetFreqU64();

    usertimercallback = NULL;

    return 0;
}

void timerUpdate(void)
{
    if (!timerticspersec) return;

    uint64_t n = (timerGetTicksU64() * timerticspersec / timerGetFreqU64()) - timerlastsample;
    if (n <= 0) return;

    totalclock += n;
    timerlastsample += n;

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
