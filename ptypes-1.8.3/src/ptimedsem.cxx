/*
 *
 *  C++ Portable Types Library (PTypes)
 *  Version 1.8.3  Released 25-Aug-2003
 *
 *  Copyright (c) 2001, 2002, 2003 Hovik Melikyan
 *
 *  http://www.melikyan.com/ptypes/
 *  http://ptypes.sourceforge.net/
 *
 */

#ifdef WIN32
#  include <windows.h>
#else
#  include <sys/time.h>
#  include <pthread.h>
#  include <errno.h>
#endif

#include "pasync.h"


PTYPES_BEGIN


static void tsem_fail()
{
    fatal(CRIT_FIRST + 41, "Timed semaphore failed");
}


#ifdef WIN32


tsemaphore::tsemaphore(int initvalue)
{
    handle = CreateSemaphore(nil, initvalue, 65535, nil);
    if (handle == 0)
        tsem_fail();
}


tsemaphore::~tsemaphore() 
{
    CloseHandle(handle);
}


bool tsemaphore::wait(int timeout)
{
    uint r = WaitForSingleObject(handle, timeout);
    if (r == WAIT_FAILED)
        tsem_fail();
    return r != WAIT_TIMEOUT;
}


void tsemaphore::post()
{
    if (ReleaseSemaphore(handle, 1, nil) == 0)
        tsem_fail();
}


#else


inline void syscheck(int r)
{
    if (r != 0)
        tsem_fail();
}


tsemaphore::tsemaphore(int initvalue)
    : unknown(), count(initvalue)
{
    syscheck(pthread_mutex_init(&mtx, 0));
    syscheck(pthread_cond_init(&cond, 0));
}


tsemaphore::~tsemaphore()
{
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mtx);
}


bool tsemaphore::wait(int timeout)
{
    pthread_mutex_lock(&mtx);
    while (count <= 0) { 
        if (timeout >= 0)
        {
            timespec abs_ts; 
            timeval cur_tv;
            gettimeofday(&cur_tv, NULL);
            abs_ts.tv_sec = cur_tv.tv_sec + timeout / 1000; 
            abs_ts.tv_nsec = cur_tv.tv_usec * 1000
                + (timeout % 1000) * 1000000;
            int rc = pthread_cond_timedwait(&cond, &mtx, &abs_ts);
            if (rc == ETIMEDOUT) { 
                pthread_mutex_unlock(&mtx);
                return false;
            }
        }
        else
            pthread_cond_wait(&cond, &mtx);
    } 
    count--;
    pthread_mutex_unlock(&mtx);
    return true;
} 


void tsemaphore::post()
{
    pthread_mutex_lock(&mtx);
    count++; 
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mtx);
}


#endif


PTYPES_END
