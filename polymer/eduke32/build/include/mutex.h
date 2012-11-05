#ifndef __mutex_h__
#define __mutex_h__

/* Mutual exclusion mechanism wrappers for the different platforms */

#if defined(_WIN32)
# include <windows.h>
# include <process.h>
#elif !defined GEKKO
# include <pthread.h>
#else
# include <SDL.h>
#endif

#ifdef EXTERNC
extern "C" {
#endif

#if defined(_WIN32)
typedef HANDLE mutex_t;
#elif !defined GEKKO
typedef pthread_mutex_t mutex_t;
#else
/* PK: I don't like pointer typedefs, but SDL_CreateMutex() _returns_ one,
 *     so we're out of luck with our interface. */
typedef SDL_mutex* mutex_t;
#endif

extern int32_t mutex_init(mutex_t *mutex);
extern int32_t mutex_lock(mutex_t *mutex);
extern int32_t mutex_unlock(mutex_t *mutex);


#ifdef EXTERNC
}
#endif

#endif
