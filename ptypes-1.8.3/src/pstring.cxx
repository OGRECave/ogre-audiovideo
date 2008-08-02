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

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ptypes.h"
#include "ptime.h"      // nowstring is defined in this module


PTYPES_BEGIN


static void stringoverflow() 
{
    fatal(CRIT_FIRST + 21, "String overflow");
}


const int quant = 64;
const int qmask = ~63;

const int quant2 = 4096;
const int qmask2 = ~4095;

int stralloc;

char   emptystrbuf[strrecsize + 4];
char*  emptystr = emptystrbuf + strrecsize;


string nullstring;


static int quantize(int numchars) 
{
    int numalloc = numchars + 1 + strrecsize;
    if (numalloc <= 16)
        return 16;
    if (numalloc <= 32)
        return 32;
    else if (numalloc <= 2048)
        return (numalloc + quant - 1) & qmask;
    else
        return (numalloc + quant2 - 1) & qmask2;
}


void string::_alloc(int numchars) 
{
    if (numchars <= 0) stringoverflow();
    size_t a = quantize(numchars);
#ifdef DEBUG
    stralloc += a;
#endif
    data = (char*)(memalloc(a)) + strrecsize;
    STR_LENGTH(data) = numchars;
    STR_REFCOUNT(data) = 1;
    data[numchars] = 0;
}


void string::_realloc(int numchars) 
{
    if (numchars <= 0 || STR_LENGTH(data) <= 0)
        stringoverflow();
    int a = quantize(numchars);
    int b = quantize(STR_LENGTH(data));
    if (a != b)
    {
#ifdef DEBUG
        stralloc += a - b;
#endif
        data = (char*)(memrealloc(data - strrecsize, a)) + strrecsize;
    }
    STR_LENGTH(data) = numchars;
    data[numchars] = 0;
}


void string::_free() 
{
#ifdef DEBUG
    stralloc -= quantize(STR_LENGTH(data));
#endif
    memfree((char*)(STR_BASE(data)));
    data = emptystr;
}


void string::initialize(const char* sc, int initlen) 
{
    if (initlen <= 0 || sc == nil)
        data = emptystr; 
    else 
    {
        _alloc(initlen);
        memmove(data, sc, initlen);
    }
}


void string::initialize(const char* sc) 
{
    initialize(sc, hstrlen(sc));
}


void string::initialize(char c) 
{
    _alloc(1);
    data[0] = c;
}


void string::initialize(const string& s)
{
    data = s.data;
#ifdef PTYPES_ST
    STR_REFCOUNT(data)++;
#else
    pincrement(&STR_REFCOUNT(data));
#endif
}


void string::finalize() 
{
    if (STR_LENGTH(data) != 0)
#ifdef PTYPES_ST
        if (--STR_REFCOUNT(data) == 0)
#else
        if (pdecrement(&STR_REFCOUNT(data)) == 0)
#endif
            _free();
    data = emptystr;
}


#ifdef PTYPES_ST
// single-threaded


char* unique(string& s) 
{
    if (STR_LENGTH(s.data) > 0 && STR_REFCOUNT(s.data) > 1)
    {
        char* olddata = s.data;
        s._alloc(STR_LENGTH(s.data));
        memcpy(s.data, olddata, STR_LENGTH(s.data));
        STR_REFCOUNT(olddata)--;
    }
    return s.data;
}


void setlength(string& s, int newlen)
{
    int curlen = STR_LENGTH(s.data);
    if (newlen < 0)
        return;

    // if becoming empty
    if (newlen == 0)
        s.finalize();

    // if otherwise s was empty before
    else if (curlen == 0)
        s._alloc(newlen);

    // if length is not changing, return a unique string
    else if (newlen == curlen)
        unique(s);

    // non-unique reallocation
    else if (STR_REFCOUNT(s.data) > 1)
    {
        char* olddata = s.data;
        s._alloc(newlen);
        memcpy(s.data, olddata, imin(curlen, newlen));
        STR_REFCOUNT(olddata)--;
    }

    // unique reallocation
    else
        s._realloc(newlen);
}


void string::assign(const char* sc, int initlen) 
{
    if (STR_LENGTH(data) > 0 && initlen > 0 && STR_REFCOUNT(data) == 1)
    {
        // reuse data buffer if unique
        _realloc(initlen);
        memmove(data, sc, initlen);
    }
    else
    {
        finalize();
        if (initlen == 1)
            initialize(sc[0]);
        else if (initlen > 1)
            initialize(sc, initlen);
    }
}


#else
// multi-threaded version


char* unique(string& s) 
{
    if (STR_LENGTH(s.data) > 0)
    {
        // by incrementing the refcount for this object we "lock" the buffer
        // from being concurrently unique'd
        // if refcount > 2 it means it was actually > 1 before
        if (pincrement(&STR_REFCOUNT(s.data)) > 2)
        {
            string t;
            int len = STR_LENGTH(s.data);

            // the refcount is already incremented, so that we can assign the pointer directly
            t.data = s.data;

            // allocate and copy data
            s._alloc(len);
            memcpy(s.data, t.data, len);

            // we must decrement refcount for t since we detached its buffer 'illegally'
            pdecrement(&STR_REFCOUNT(t.data));

            // here the destructor for t is being called; the buffer may be freed
            // if string buffer was detached concurrently
        }
        else
            // refcount was 1 before incrementing, so we can assign it directly
            // instead of costly pdecrement()
            STR_REFCOUNT(s.data) = 1;
    }

    return s.data;
}


void setlength(string& s, int newlen)
{
    int curlen = length(s);
    if (newlen < 0)
        return;

    // if becoming empty
    if (newlen == 0)
        s.finalize();

    // if otherwise s was empty before
    else if (curlen == 0)
        s._alloc(newlen);

    // if length is not changing, return a unique string
    else if (newlen == curlen)
        unique(s);

    // thread-safe reallocation; see comments in unique()
    else
    {
        if (pincrement(&STR_REFCOUNT(s.data)) > 2)
        {
            string t;
            t.data = s.data;
            s._alloc(newlen);
            memcpy(s.data, t.data, imin(curlen, newlen));
            pdecrement(&STR_REFCOUNT(t.data));
        }
        else
        {
            STR_REFCOUNT(s.data) = 1;
            s._realloc(newlen);
        }
    }
}


void string::assign(const char* sc, int initlen) 
{
    if (STR_LENGTH(data) > 0 && initlen > 0)
    {
        // thread-safe assignment with possible
        // reuse of the buffer
        if (pincrement(&STR_REFCOUNT(data)) > 2)
        {
            string t;
            t.data = data;
            _alloc(initlen);
            memcpy(data, sc, initlen);
            pdecrement(&STR_REFCOUNT(t.data));
        }
        else
        {
            STR_REFCOUNT(data) = 1;
            _realloc(initlen);
            memmove(data, sc, initlen);
        }
    }
    else
    {
        finalize();
        if (initlen == 1)
            initialize(sc[0]);
        else if (initlen > 1)
            initialize(sc, initlen);
    }
}


#endif // single/multi-threaded


void string::assign(const char* sc) 
{
    assign(sc, hstrlen(sc));
}


void string::assign(char c) 
{
    assign(&c, 1);
}


void string::assign(const string& s) 
{
    if (data != s.data)
    {
        finalize();
        initialize(s);
    }
}


string dup(const string& s)
{
    return string(s.data, length(s));
}


string nowstring(const char* fmt, bool utc)
{
    char buf[128];
    time_t longtime;
    time(&longtime);

#if defined(PTYPES_ST) || defined(WIN32)
    tm* t;
    if (utc)
        t = gmtime(&longtime);
    else
        t = localtime(&longtime);
    int r = strftime(buf, sizeof(buf), fmt, t);
#else
    tm t;
    if (utc)
        gmtime_r(&longtime, &t);
    else
        localtime_r(&longtime, &t);
    int r = strftime(buf, sizeof(buf), fmt, &t);
#endif

    buf[r] = 0;
    return string(buf);
}


PTYPES_END
