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
#  include <process.h>
#else
#  include <pthread.h>
#endif

#include "pasync.h"


PTYPES_BEGIN


thread::thread(bool iautofree)
    :
#ifdef WIN32
    id(0),
#endif
    handle(0), autofree(iautofree), 
    running(0), signaled(0), freed(0), finished(false), relaxsem(0) 
{
}


thread::~thread() 
{
#ifdef WIN32
    if (autofree)
        // MSDN states this is not necessary, however, without closing
        // the handle debuggers show an obvious handle leak here
        CloseHandle(handle);
#endif
}


void thread::signal() 
{
    if (pexchange(&signaled, 1) == 0)
        relaxsem.post();
}


void thread::waitfor() 
{
    if (pexchange(&freed, 1) != 0)
        return;
    if (pthrequal(get_id()))
        fatal(CRIT_FIRST + 47, "Can not waitfor() on myself");
    if (autofree)
	fatal(CRIT_FIRST + 48, "Can not waitfor() on an autofree thread");
#ifdef WIN32
    WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
#else
    pthread_join(handle, nil);
    pthread_detach(handle);
#endif
    handle = 0;
}


#ifdef WIN32
unsigned _stdcall _threadproc(void* arg) 
{
#else
void* _threadproc(void* arg) 
{
#endif
    thread* thr = (thread*)arg;
    try 
    {
        thr->execute();
    }
    catch(exceptobj*) 
    {
        _threadepilog(thr);
        throw;
    }
    _threadepilog(thr);
    return 0;
}


void _threadepilog(thread* thr) 
{
    try
    {
        thr->cleanup();
    }
    catch(exceptobj* e)
    {
        delete e;
    }
    thr->finished = true;
    if (thr->autofree)
        delete thr;
}


void thread::start() 
{
    if (pexchange(&running, 1) == 0)
    {
#ifdef WIN32
        handle = (HANDLE)_beginthreadex(nil, 0, _threadproc, this, 0, &id);
        if (handle == 0)
            fatal(CRIT_FIRST + 40, "CreateThread() failed");
#else
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, 
            autofree ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE);
        if (pthread_create(&handle, &attr, _threadproc, this) != 0)
            fatal(CRIT_FIRST + 40, "pthread_create() failed");
        pthread_attr_destroy(&attr);
#endif
    }
}


void thread::cleanup()
{
}


PTYPES_END
