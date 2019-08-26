
#ifndef __timer_h__
#define __timer_h__

#define kTimerTicks		120

void InitTimer();

#ifdef __WATCOMC__
void uninittimer();
#endif

#endif