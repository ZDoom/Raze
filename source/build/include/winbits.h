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

extern BOOL CheckWinVersion(void);
extern void win_allowtaskswitching(int32_t onf);
extern int32_t win_checkinstance(void);

#if defined(RENDERTYPEWIN) || SDL_MAJOR_VERSION==1
extern int32_t win_inittimer(void);
extern uint64_t win_getu64ticks(void);
#endif

extern void win_open(void);
extern void win_init(void);
extern void win_setvideomode(int32_t c);
extern void win_uninit(void);
extern void win_close(void);

extern void Win_GetOriginalLayoutName(void);
extern void Win_SetKeyboardLayoutUS(int);

extern void ShowErrorBox(const char *m);

extern LPTSTR GetWindowsErrorMsg(DWORD code);

extern int32_t addsearchpath_ProgramFiles(const char *p);

extern int32_t G_GetVersionFromWebsite(char *buffer);
extern int32_t win_buildargs(char **argvbuf);