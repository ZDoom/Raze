#ifndef mutex_h_
#define mutex_h_

#include <mutex>

typedef std::mutex mutex_t;

inline void mutex_lock(mutex_t* mutex)
{
	mutex->lock();
}

inline void mutex_unlock(mutex_t* mutex)
{
	mutex->unlock();
}



#endif
