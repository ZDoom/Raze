// On-screen display (ie. console)
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)

#ifndef osd_h_
#define osd_h_

#include <functional>

#include "compat.h"
#include "printf.h"
#include "c_dispatch.h"

const char *OSD_StripColors(char *outBuf, const char *inBuf);


#define OSDCMD_OK	CCMD_OK
#define OSDCMD_SHOWHELP CCMD_SHOWHELP


// void OSD_Draw();

// executes buffered commands
void C_RunDelayedCommands();
inline void OSD_DispatchQueued(void)
{
	C_RunDelayedCommands();
}

#endif // osd_h_

