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

#ifndef __PASYNC_H__
#define __PASYNC_H__

#ifdef WIN32
#  define _WINSOCKAPI_   // prevent inclusion of winsock.h in windows.h
#  include <windows.h>
#else
#  include <pthread.h>
#  ifndef __bsdi__
#    include <semaphore.h>
#  endif
#endif

#ifndef __PPORT_H__
#include "pport.h"
#endif

#ifndef __PTYPES_H__
#include "ptypes.h"
#endif


PTYPES_BEGIN

//
//  Summary of implementation:
//
//  atomic increment/decrement/exchange
//    Win32: internal, asm
//    GCC/i386: internal, asm
//    Other: internal, mutex hash table
//
//  mutex
//    Win32: Critical section
//    Other: POSIX mutex
//
//  trigger
//    Win32: Event
//    Other: internal, POSIX condvar/mutex
//
//  rwlock:
//    Win32: internal, Event/mutex
//    MacOS: internal, POSIX condvar/mutex
//    Other: POSIX rwlock
//
//  semaphore:
//    Win32: = tsemaphore
//    MacOS: = tsemaphore
//    Other: POSIX semaphore
//
//  tsemaphore (with timed waiting):
//    Win32: Semaphore
//    Other: internal, POSIX mutex/condvar
//


#ifdef _MSC_VER
#pragma pack(push, 4)
#endif


#ifdef WIN32
   typedef int pthread_id_t;
   typedef HANDLE pthread_t;
#else
   typedef pthread_t pthread_id_t;
#endif


ptpublic void psleep(uint milliseconds);
ptpublic bool pthrequal(pthread_id_t id);  // note: this is NOT the thread handle, use thread::get_id()
ptpublic pthread_id_t pthrself();          // ... same


// -------------------------------------------------------------------- //
// --- mutex ---------------------------------------------------------- //
// -------------------------------------------------------------------- //


#ifdef WIN32

struct ptpublic mutex
{
protected:
    CRITICAL_SECTION critsec;
public:
    mutex()         { InitializeCriticalSection(&critsec); }
    ~mutex()        { DeleteCriticalSection(&critsec); }
    void enter()    { EnterCriticalSection(&critsec); }
    void leave()    { LeaveCriticalSection(&critsec); }
    void lock()     { enter(); }
    void unlock()   { leave(); }
};


#else


struct ptpublic mutex
{
protected:
    pthread_mutex_t mtx;
public:
    mutex()         { pthread_mutex_init(&mtx, 0); }
    ~mutex()        { pthread_mutex_destroy(&mtx); }
    void enter()    { pthread_mutex_lock(&mtx); }
    void leave()    { pthread_mutex_unlock(&mtx); }
    void lock()     { enter(); }
    void unlock()   { leave(); }
};

#endif


//
// scopelock
//

class scopelock
{
protected:
    mutex* mtx;
public:
    scopelock(mutex& imtx): mtx(&imtx)  { mtx->lock(); }
    ~scopelock()  { mtx->unlock(); }
};


//
// mutex table for hashed memory locking (undocumented)
//

#define _MUTEX_HASH_SIZE     17      // a prime number for hashing

#ifdef WIN32
#  define pmemlock        mutex
#  define pmementer(m)    (m)->lock()
#  define pmemleave(m)    (m)->unlock()
#else
#  define _MTX_INIT       PTHREAD_MUTEX_INITIALIZER
#  define pmemlock        pthread_mutex_t
#  define pmementer       pthread_mutex_lock
#  define pmemleave       pthread_mutex_unlock
#endif


ptpublic extern pmemlock _mtxtable[_MUTEX_HASH_SIZE];

#define pgetmemlock(addr) (_mtxtable + uint(addr) % _MUTEX_HASH_SIZE)


// -------------------------------------------------------------------- //
// --- trigger -------------------------------------------------------- //
// -------------------------------------------------------------------- //


#ifdef WIN32

class ptpublic trigger
{
protected:
    HANDLE handle;      // Event object
public:
    trigger(bool autoreset, bool state);
    ~trigger()          { CloseHandle(handle); }
    void wait()         { WaitForSingleObject(handle, INFINITE); }
    void post()         { SetEvent(handle); }
    void signal()       { post(); }
    void reset()        { ResetEvent(handle); }
};


#else


class ptpublic trigger
{
protected:
    pthread_mutex_t mtx;
    pthread_cond_t cond;
    int state;
    bool autoreset;
public:
    trigger(bool autoreset, bool state);
    ~trigger();
    void wait();
    void post();
    void signal()  { post(); }
    void reset();
};

#endif


// -------------------------------------------------------------------- //
// --- rwlock --------------------------------------------------------- //
// -------------------------------------------------------------------- //


#if defined(WIN32) || defined(__DARWIN__) || defined(__bsdi__)
#  define __PTYPES_RWLOCK__
#elif defined(linux)
   // on Linux rwlocks are included only with -D_GNU_SOURCE.
   // programs that don't use rwlocks, do not need to define
   // _GNU_SOURCE either.
#  if defined(_GNU_SOURCE) || defined(__USE_UNIX98)
#    define __POSIX_RWLOCK__
#  endif
#else
#  define __POSIX_RWLOCK__
#endif


#ifdef __PTYPES_RWLOCK__

struct ptpublic rwlock: protected mutex
{
protected:
#ifdef WIN32
    HANDLE  reading;    // Event object
    HANDLE  finished;   // Event object
    int     readcnt;
    int     writecnt;
#else
    pthread_mutex_t mtx;
    pthread_cond_t readcond;
    pthread_cond_t writecond;
    int locks;
    int writers;
    int readers;
#endif
public:
    rwlock();
    ~rwlock();
    void rdlock();
    void wrlock();
    void unlock();
    void lock()     { wrlock(); }
};


#elif defined(__POSIX_RWLOCK__)


struct ptpublic rwlock
{
protected:
    pthread_rwlock_t rw;
public:
    rwlock();
    ~rwlock()       { pthread_rwlock_destroy(&rw); }
    void rdlock()   { pthread_rwlock_rdlock(&rw); }
    void wrlock()   { pthread_rwlock_wrlock(&rw); }
    void unlock()   { pthread_rwlock_unlock(&rw); }
    void lock()     { wrlock(); }
};

#endif


#if defined(__PTYPES_RWLOCK__) || defined(__POSIX_RWLOCK__)

//
// scoperead & scopewrite
//

class scoperead
{
protected:
    rwlock* rw;
public:
    scoperead(rwlock& irw): rw(&irw)  { rw->rdlock(); }
    ~scoperead()  { rw->unlock(); }
};


class scopewrite
{
protected:
    rwlock* rw;
public:
    scopewrite(rwlock& irw): rw(&irw)  { rw->wrlock(); }
    ~scopewrite()  { rw->unlock(); }
};


#endif


// -------------------------------------------------------------------- //
// --- semaphore ------------------------------------------------------ //
// -------------------------------------------------------------------- //


#if defined(WIN32) || defined(__DARWIN__) || defined(__bsdi__)
#  define __SEM_TO_TIMEDSEM__
#endif


#ifdef __SEM_TO_TIMEDSEM__

// map ordinary semaphore to timed semaphore

class tsemaphore;
typedef tsemaphore semaphore;


#else


class ptpublic semaphore: public unknown
{
protected:
    sem_t handle;
public:
    semaphore(int initvalue);
    virtual ~semaphore();

    void wait();
    void post();
    void signal()  { post(); }
};

#endif


class ptpublic tsemaphore: public unknown
{
protected:
#ifdef WIN32
    HANDLE handle;
#else
    int count;
    pthread_mutex_t mtx;
    pthread_cond_t cond;
#endif
public:
    tsemaphore(int initvalue);
    virtual ~tsemaphore();
    bool wait(int msecs = -1);
    void post();
    void signal()  { post(); }
};


// -------------------------------------------------------------------- //
// --- thread --------------------------------------------------------- //
// -------------------------------------------------------------------- //


class ptpublic thread: public unknown
{
protected:
#ifdef WIN32
    unsigned id;
#endif
    pthread_t  handle;
    bool autofree;
    int  running;
    int  signaled;
    int  freed;
    bool finished;
    tsemaphore relaxsem;

    virtual void execute() = 0;
    virtual void cleanup();

    bool relax(int msecs) { return relaxsem.wait(msecs); }

    friend void _threadepilog(thread* thr);

#ifdef WIN32
    friend unsigned __stdcall _threadproc(void* arg);
#else
    friend void* _threadproc(void* arg);
#endif

public:
    thread(bool iautofree);
    virtual ~thread();

#ifdef WIN32
    pthread_id_t get_id()   { return int(id); }
#else
    pthread_id_t get_id()   { return handle; }
#endif

    bool get_running()    { return running != 0; }
    bool get_finished()   { return finished; }
    bool get_signaled()   { return signaled != 0; }

    void start();
    void signal();
    void waitfor();
};



// -------------------------------------------------------------------- //
// --- msgqueue ------------------------------------------------------- //
// -------------------------------------------------------------------- //


const int MSG_USER = 0;
const int MSG_USER_MAX = 0xBFFFF;
const int MSG_LIB = 0xC0000;
const int MSG_QUIT = MSG_LIB;

class ptpublic message: public unknown
{
protected:
    message* next;          // next in the message chain, used internally
    semaphore* sync;        // used internally by msgqueue::send(), when called from a different thread
    friend class msgqueue;  // my friend, message queue...
public:
    int id;
    int result;
    int param;
    message(int iid, int iparam = 0);
    virtual ~message();
};


class ptpublic msgqueue
{
private:
    message*  head;         // queue head
    message*  tail;         // queue tail
    int       qcount;       // number of items in the queue
    semaphore sem;          // queue semaphore
    mutex     qlock;        // critical sections in enqueue and dequeue
    mutex     thrlock;      // lock for the queue processing
    pthread_id_t owner;     // thread ID of the queue processing thread

    void enqueue(message* msg);
    void push(message* msg);
    message* dequeue(bool safe = true);

    void purgequeue();
    int  finishmsg(message* msg);
    void handlemsg(message* msg);
    void takeownership();

protected:
    bool quit;

    void defhandler(message& msg);
    virtual void msghandler(message& msg) = 0;

public:
    msgqueue();
    virtual ~msgqueue();

    // functions calling from the owner thread:
    int  msgsavail()  { return qcount; }
    void processone();  // process one message, may hang if no msgs in the queue
    void processmsgs(); // process all available messages and return
    void run();         // process messages until MSG_QUIT

    // functions calling from any thread:
    void post(message* msg);
    void post(int id, int param = 0);
    void posturgent(message* msg);
    void posturgent(int id, int param = 0);
    int  send(message* msg);
    int  send(int id, int param = 0);
};


#ifdef _MSC_VER
#pragma pack(pop)
#endif


PTYPES_END

#endif // __PASYNC_H__
