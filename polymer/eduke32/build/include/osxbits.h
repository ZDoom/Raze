#ifndef osxbits_h_
#define osxbits_h_
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t osx_msgbox(const char *name, const char *msg);
int32_t osx_ynbox(const char *name, const char *msg);

#ifdef __cplusplus
}
#endif

#endif
