// Windows layer-independent code

#include <compat.h>

#define WindowClass "buildapp"

extern int32_t backgroundidle;	// set to 1 to tell winlayer to go to idle priority when inactive

extern int64_t win_timerfreq;

extern char silentvideomodeswitch;

extern BOOL CheckWinVersion(void);
extern void win_allowtaskswitching(int32_t onf);
extern int32_t win_checkinstance(void);

extern int32_t win_inittimer(void);
extern uint64_t win_getu64ticks(void);

extern void win_open(void);
extern void win_init(void);
extern void win_setvideomode(int32_t c);
extern void win_uninit(void);
extern void win_close(void);

extern void ShowErrorBox(const char *m);

extern LPTSTR GetWindowsErrorMsg(DWORD code);

extern int32_t addsearchpath_ProgramFiles(const char *p);

extern int32_t G_GetVersionFromWebsite(char *buffer);
extern int32_t win_buildargs(char **argvbuf);