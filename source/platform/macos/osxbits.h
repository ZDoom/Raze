#ifndef osxbits_h_
#define osxbits_h_
#include <sys/types.h>

int osx_msgbox(const char *name, const char *msg);
int osx_ynbox(const char *name, const char *msg);

char *osx_gethomedir(void);
char *osx_getsupportdir(int32_t local);
char *osx_getappdir(void);
char *osx_getapplicationsdir(int32_t local);

#endif
