#include "compat.h"
#include "mutex.h"

int32_t mutex_init(mutex_t *mutex)
{
#ifdef _WIN32
    *mutex = CreateMutex(0, FALSE, 0);
    return (*mutex == 0);
#else
    return pthread_mutex_init(mutex, NULL);
#endif
    return -1;
}

int32_t mutex_lock(mutex_t *mutex)
{
#ifdef _WIN32
    return (WaitForSingleObject(*mutex, INFINITE) == WAIT_FAILED);
#else
    return pthread_mutex_lock(mutex);
#endif
    return -1;
}

int32_t mutex_unlock(mutex_t *mutex)
{
#ifdef _WIN32
    return (ReleaseMutex(*mutex) == 0);
#else
    return pthread_mutex_unlock(mutex);
#endif
    return -1;
}


