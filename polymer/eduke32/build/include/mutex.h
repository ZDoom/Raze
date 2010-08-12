#ifndef __mutex_h__
#define __mutex_h__

#if defined(_WIN32)
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

#if defined(_WIN32)
typedef HANDLE mutex_t;
#else
typedef pthread_mutex_t mutex_t;
#endif

extern int32_t mutex_init(mutex_t *mutex);
extern int32_t mutex_lock(mutex_t *mutex);
extern int32_t mutex_unlock(mutex_t *mutex);

#endif
