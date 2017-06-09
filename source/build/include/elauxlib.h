
#ifndef elauxlib_h_
#define elauxlib_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_LUAJIT_2_1
# include <luajit-2.1/lauxlib.h>
#else
# include <luajit-2.0/lauxlib.h>
#endif

#ifdef __cplusplus
}
#endif

#endif
