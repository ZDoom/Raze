
#include "exhumed.h"

extern "C" {
#ifndef __WATCOMC__
#include "build.h"
#include "osd.h"
#include "baselayer.h"
#endif

void faketimerhandler(void)
{
}

#ifdef __WATCOMC__
void main(int argc, char *argv[])
{
	ExhumedMain(argc, argv); // main function in exhumed.cpp
}
#else
int app_main(int argc, char *argv[])
{
	wm_setapptitle("Exhumed/PowerSlave PC RE");
	ExhumedMain(argc, argv); // main function in exhumed.cpp
	return 0;
}
#endif

}
