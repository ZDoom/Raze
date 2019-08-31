
#include "compat.h"
#include "baselayer.h"
#include "timer.h"
#include "exhumed.h"

void timerhandler();

void InitTimer()
{
    htimer = 1;

    timerInit(kTimerTicks);
    timerSetCallback(timerhandler);
}

