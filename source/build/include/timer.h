#pragma once

#ifndef timer_h__
#define timer_h__

#include "compat.h"

// for compatibility
#define timerUninit()

double   timerGetHiTicks(void);
uint32_t timerGetTicks(void);

#endif // timer_h__
