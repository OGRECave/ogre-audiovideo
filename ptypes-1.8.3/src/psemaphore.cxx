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
#  include <pthread.h>
#endif

#include "pasync.h"


PTYPES_BEGIN


#ifndef __SEM_TO_TIMEDSEM__


static void sem_fail()
{
    fatal(CRIT_FIRST + 41, "Semaphore failed");
}


semaphore::semaphore(int initvalue) 
{
    if (sem_init(&handle, 0, initvalue) != 0)
        sem_fail();
}


semaphore::~semaphore() 
{
    sem_destroy(&handle);
}


void semaphore::wait() 
{
    if (sem_wait(&handle) != 0)
        sem_fail();
}


void semaphore::post() 
{
    if (sem_post(&handle) != 0)
        sem_fail();
}


#else


int _psemaphore_dummy_symbol;  // avoid ranlib's warning message


#endif



PTYPES_END
