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
#endif

#include "pasync.h"


PTYPES_BEGIN


#ifdef PTYPES_ST
// single-threaded version


int __PFASTCALL pexchange(int* target, int value)
{
    int r = *target;
    *target = value;
    return r;
}


void* __PFASTCALL pexchange(void** target, void* value)
{
    void* r = *target;
    *target = value;
    return r;
}


int __PFASTCALL pincrement(int* target)
{
    return ++(*target);
}


int __PFASTCALL pdecrement(int* target)
{
    return --(*target);
}


#else
// multi-threaded version

#if defined(__GNUC__) && (defined(__i386__) || defined(__I386__))
#  define GCC_i386
#elif defined(_MSC_VER) && defined(_M_IX86)
#  define MSC_i386
#elif defined(__BORLANDC__) && defined(_M_IX86)
#  define BCC_i386
#endif


#if defined(MSC_i386) || defined(BCC_i386)

//
// atomic operations for Microsoft C or Borland C on Windows
//

#if defined(_MSC_VER)
#  pragma warning (disable: 4035)
#elif defined(__BORLANDC__)
#  pragma warn -rvl
#endif


// !!! NOTE
// the following functions implement atomic exchange/inc/dec on
// windows. they are dangerous in that they rely on the calling
// conventions of MSVC and BCC. the first one passes the first
// two arguments in ECX and EDX, and the second one - in EAX and 
// EDX.

int __PFASTCALL pincrement(int*)
{
    __asm
    {
#ifdef BCC_i386
.486
        mov         ecx,eax
#endif
        mov         eax,1;
        lock xadd   [ecx],eax;
        inc         eax
    }
}


int __PFASTCALL pdecrement(int*)
{
    __asm
    {
#ifdef BCC_i386
.486
        mov         ecx,eax
#endif
        mov         eax,-1;
        lock xadd   [ecx],eax;
        dec         eax
    }
}


int __PFASTCALL pexchange(int*, int)
{
    __asm
    {
#ifdef BCC_i386
.486
        xchg        eax,edx;
        lock xchg   eax,[edx];
#else
        mov         eax,edx;
        lock xchg   eax,[ecx];
#endif
    }
}


void* __PFASTCALL pexchange(void**, void*)
{
    __asm
    {
#ifdef BCC_i386
.486
        xchg        eax,edx;
        lock xchg   eax,[edx];
#else
        mov         eax,edx;
        lock xchg   eax,[ecx];
#endif
    }
}


#elif defined(GCC_i386)

//
// GNU C compiler on any i386 platform (actually 486+ for xadd)
//

int pexchange(int* target, int value)
{
    __asm__ __volatile ("lock ; xchgl (%1),%0" : "+r" (value) : "r" (target));
    return value;
}


void* pexchange(void** target, void* value)
{
    __asm__ __volatile ("lock ; xchgl (%1),%0" : "+r" (value) : "r" (target));
    return value;
}


int pincrement(int* target)
{
    int temp = 1;
    __asm__ __volatile ("lock ; xaddl %0,(%1)" : "+r" (temp) : "r" (target));
    return temp + 1;
}


int pdecrement(int* target)
{
    int temp = -1;
    __asm__ __volatile ("lock ; xaddl %0,(%1)" : "+r" (temp) : "r" (target));
    return temp - 1;
}


#else

//
// other platforms: mutex locking
//

int pexchange(int* target, int value)
{
    pmemlock* m = pgetmemlock(target);
    pmementer(m);
    int r = *target;
    *target = value;
    pmemleave(m);
    return r;
}


void* pexchange(void** target, void* value)
{
    pmemlock* m = pgetmemlock(target);
    pmementer(m);
    void* r = *target;
    *target = value;
    pmemleave(m);
    return r;
}


int pincrement(int* target)
{
    pmemlock* m = pgetmemlock(target);
    pmementer(m);
    int r = ++(*target);
    pmemleave(m);
    return r;
}


int pdecrement(int* target)
{
    pmemlock* m = pgetmemlock(target);
    pmementer(m);
    int r = --(*target);
    pmemleave(m);
    return r;
}

#endif


#endif


PTYPES_END
