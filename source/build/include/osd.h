// On-screen display (ie. console)
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)

#ifndef osd_h_
#define osd_h_

#include <functional>

#include "compat.h"
#include "mutex.h"
#include "printf.h"

struct osdfuncparm_t
{
	int32_t numparms;
	const char* name;
	const char** parms;
	const char* raw;
};

using osdcmdptr_t = osdfuncparm_t const * const;

const char *OSD_StripColors(char *outBuf, const char *inBuf);


#define OSDCMD_OK	0
#define OSDCMD_SHOWHELP 1


// void OSD_Draw();

// executes buffered commands
void C_RunDelayedCommands();
inline void OSD_DispatchQueued(void)
{
	C_RunDelayedCommands();
}

// registers a function
//   name = name of the function
//   help = a short help string
//   func = the entry point to the function
int OSD_RegisterFunction(const char *pszName, const char *pszDesc, int (*func)(osdcmdptr_t));

#endif // osd_h_

