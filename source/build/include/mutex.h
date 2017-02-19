#ifndef mutex_h_
#define mutex_h_

/* Mutual exclusion mechanism wrappers for the different platforms */

#ifdef RENDERTYPEWIN
# include "windows_inc.h"
#else
# define SDL_MAIN_HANDLED
# include "sdl_inc.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef RENDERTYPEWIN
typedef HANDLE mutex_t;
#else
/* PK: I don't like pointer typedefs, but SDL_CreateMutex() _returns_ one,
 *     so we're out of luck with our interface. */
typedef SDL_mutex* mutex_t;
#endif

extern int32_t mutex_init(mutex_t *mutex);
extern int32_t mutex_lock(mutex_t *mutex);
extern int32_t mutex_unlock(mutex_t *mutex);


#ifdef __cplusplus
}
#endif

#endif
