// Windows layer-independent code

#include "compat.h"

#ifdef APPNAME
# define WindowClass APPNAME
#else
# define WindowClass "buildapp"
#endif

extern int32_t backgroundidle;	// set to 1 to tell winlayer to go to idle priority when inactive

extern int64_t win_timerfreq;

extern char silentvideomodeswitch;

extern void win_init(void);
extern void win_setvideomode(int32_t c);
extern void win_uninit(void);

extern int32_t addsearchpath_ProgramFiles(const char *p);

extern int32_t win_buildargs(char **argvbuf);