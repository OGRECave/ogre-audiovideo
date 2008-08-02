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
#  include <stdlib.h>
#  include <unistd.h>
#  include <pthread.h>
#  ifdef __sun__
#    include <poll.h>
#  endif
#endif

#include "pasync.h"


PTYPES_BEGIN


void psleep(uint milliseconds) 
{
#if defined(WIN32)
    Sleep(milliseconds);
#elif defined(__sun__)
    poll(0, 0, milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}


pthread_id_t pthrself() 
{
#ifdef WIN32
    return (int)GetCurrentThreadId();
#else
    return pthread_self();
#endif
}


bool pthrequal(pthread_id_t id)
{
#ifdef WIN32
    return GetCurrentThreadId() == (uint)id;
#else
    return pthread_equal(pthread_self(), id);
#endif
}


PTYPES_END
