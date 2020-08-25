// Build engine timer stuff

#include "timer.h"

#include "build.h"
#include "compat.h"

#include <chrono>

using namespace std;
using namespace chrono;

EDUKE32_STATIC_ASSERT((steady_clock::period::den/steady_clock::period::num) >= 1000000000);

static time_point<steady_clock> timerlastsample;
static int timerticspersec;

uint32_t timerGetTicks(void)    { return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count(); }

// Returns the time since an unspecified starting time in milliseconds.
// (May be not monotonic for certain configurations.)
double timerGetHiTicks(void) { return duration<double, nano>(steady_clock::now().time_since_epoch()).count() / 1000000.0; }

int timerInit(int const tickspersecond)
{
    timerticspersec = tickspersecond;
    timerlastsample = steady_clock::now();
    return 0;
}

void timerUpdateClock(void)
{
    auto time = steady_clock::now();
    auto elapsedTime = time - timerlastsample;

    uint64_t numerator = (elapsedTime.count() * (uint64_t) timerticspersec * steady_clock::period::num);
    uint64_t freq = steady_clock::period::den;
    int n = numerator / freq;

    if (n <= 0) return;

    totalclock += n;
    timerlastsample += n*nanoseconds(1000000000/timerticspersec);
}
