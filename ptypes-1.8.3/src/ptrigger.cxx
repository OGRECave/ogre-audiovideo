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


static void trig_fail()
{
    fatal(CRIT_FIRST + 41, "Trigger failed");
}



#ifdef WIN32


trigger::trigger(bool autoreset, bool state)
{
    handle = CreateEvent(0, !autoreset, state, 0);
    if (handle == 0)
        trig_fail();
}


#else


inline void syscheck(int r)
{
    if (r != 0)
        trig_fail();
}


trigger::trigger(bool iautoreset, bool istate)
    : state(int(istate)), autoreset(iautoreset)
{
    syscheck(pthread_mutex_init(&mtx, 0));
    syscheck(pthread_cond_init(&cond, 0));
}


trigger::~trigger()
{
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mtx);
}


void trigger::wait()
{
    pthread_mutex_lock(&mtx);
    if (state == 0)
        pthread_cond_wait(&cond, &mtx);
    if (autoreset)
	state = 0;
    pthread_mutex_unlock(&mtx);
} 


void trigger::post()
{
    pthread_mutex_lock(&mtx);
    state = 1;
    if (autoreset)
        pthread_cond_signal(&cond);
    else
        pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mtx);
}


void trigger::reset()
{
    state = 0;
}


#endif


PTYPES_END
