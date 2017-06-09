
#ifndef elua_h_
#define elua_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_LUAJIT_2_1
# include <luajit-2.1/lua.h>
#else
# include <luajit-2.0/lua.h>
#endif

#ifdef __cplusplus
}
#endif

#endif
