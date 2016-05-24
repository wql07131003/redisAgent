/*
 * Lock.cpp
 *
 *  Created on: 2009-12-26
 *      Author: Administrator
 */

#include <time.h>

#include "Lock.h"

namespace forum
{

    Lock::Lock()
    {
        pthread_mutex_init(&_mutex, NULL);
    }

    Lock::~Lock()
    {
        pthread_mutex_destroy(&_mutex);
    }

    int Lock::lock()
    {
        return pthread_mutex_lock(&_mutex);
    }

    int Lock::timed_lock(uint millisecond)
    {
        long second  = millisecond / 1000;
        long nanosecond = (millisecond % 1000) * 1000000;

        timespec tm = {time(NULL) + second, nanosecond};
        return pthread_mutex_timedlock(&_mutex, &tm);
    }

    void Lock::unlock()
    {
        pthread_mutex_unlock(&_mutex);
    }

}
