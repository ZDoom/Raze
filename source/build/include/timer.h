#pragma once

#ifndef timer_h__
#define timer_h__

#include "compat.h"

// for compatibility
#define timerUninit()

int      timerInit(int const tickspersecond);
void     timerUpdateClock(void);
double   timerGetHiTicks(void);
uint32_t timerGetTicks(void);

#endif // timer_h__
