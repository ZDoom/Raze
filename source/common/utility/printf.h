#pragma once

// Just to let the ZDoom-based code print to the Build console without changing it all

#if defined __GNUC__ || defined __clang__
# define ATTRIBUTE(attrlist) __attribute__(attrlist)
#else
# define ATTRIBUTE(attrlist)
#endif

void OSD_Printf(const char *fmt, ...) ATTRIBUTE((format(printf,1,2)));
#define Printf OSD_Printf	

void I_Error(const char *fmt, ...) ATTRIBUTE((format(printf,1,2)));

