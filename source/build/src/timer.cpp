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
