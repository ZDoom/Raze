#include "compat.h"
#include "mutex.h"

int32_t mutex_init(mutex_t *mutex)
{
#ifdef _WIN32
    *mutex = CreateMutex(0, FALSE, 0);
    return (*mutex == 0);
#elif !defined GEKKO
    return pthread_mutex_init(mutex, NULL);
#else
    if (mutex)
    {
        *mutex = SDL_CreateMutex();
        if (*mutex != NULL)
            return 0;
    }
    return -1;
#endif
}

int32_t mutex_lock(mutex_t *mutex)
{
#ifdef _WIN32
    return (WaitForSingleObject(*mutex, INFINITE) == WAIT_FAILED);
#elif !defined GEKKO
    return pthread_mutex_lock(mutex);
#else
    return SDL_LockMutex(*mutex);
#endif
}

int32_t mutex_unlock(mutex_t *mutex)
{
#ifdef _WIN32
    return (ReleaseMutex(*mutex) == 0);
#elif !defined GEKKO
    return pthread_mutex_unlock(mutex);
#else
    return SDL_UnlockMutex(*mutex);
#endif
}
