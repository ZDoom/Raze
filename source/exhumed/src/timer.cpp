
#include "timer.h"
#include "exhumed.h"

#ifndef __WATCOMC__
extern "C" {
	#include "baselayer.h"
}
#endif

#ifdef __WATCOMC__

#include <dos.h>
#include <conio.h>
#include <i86.h>

void (__interrupt __far *oldtimerhandler)();
void __interrupt __far timerhandler();
#else
void timerhandler();
#endif

void InitTimer()
{
	htimer = 1;

#ifndef __WATCOMC__
	inittimer(kTimerTicks);
	installusertimercallback(timerhandler);
#else

	outp(0x43, 0x34);
	outp(0x40, (1193181 / kTimerTicks) & 255);
	outp(0x40, (1193181 / kTimerTicks) >> 8);
	oldtimerhandler = _dos_getvect(0x8);
	_disable(); _dos_setvect(0x8, timerhandler); _enable();

#endif
}

#ifdef __WATCOMC__
void uninittimer()
{
	outp(0x43, 0x34); outp(0x40, 0); outp(0x40, 0);
	_disable(); _dos_setvect(0x8, oldtimerhandler); _enable();
}
#endif
