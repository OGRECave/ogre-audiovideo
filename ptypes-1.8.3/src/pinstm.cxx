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

#include <errno.h>
#include <string.h>
#include <limits.h>

#ifdef WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif

#include "pstreams.h"


PTYPES_BEGIN


instm::instm(int ibufsize): iobase(ibufsize) 
{
}


instm::~instm() 
{
}


int instm::classid()
{
    return CLASS_INSTM;
}


int instm::dorawread(char* buf, int count)
{
    if (handle == invhandle)
	return -1;
#ifdef WIN32
    unsigned long ret;
    if (!ReadFile(HANDLE(handle), buf, count, &ret, nil)) 
#else
    int ret;
    if ((ret = ::read(handle, buf, count)) < 0)
#endif
    {
        int e = uerrno();
        if (e == EPIPE)
            ret = 0;
        else
            error(e, "Couldn't read");
    }
    return ret;
}


int instm::rawread(char* buf, int count) 
{
    if (!active)
        errstminactive();
    try 
    {
        int ret = dorawread(buf, count);
        if (ret <= 0) {
            ret = 0;
            eof = true;
            chstat(IO_EOF);
        }
        else 
        {
            abspos += ret;
            chstat(IO_READING);
        }
        return ret;
    }
    catch (estream*) 
    {
        eof = true;
        chstat(IO_EOF);
        throw;
    }
}


int instm::tell() 
{
    return abspos - bufend + bufpos;
}


void instm::bufvalidate() 
{
    if (!active)
        errstminactive();
    if (bufsize == 0)
        errbufrequired();
    bufclear();
    bufend = rawread(bufdata, bufsize);
}


int instm::seek(int newpos, ioseekmode mode) 
{
    if (bufsize != 0 && mode != IO_END) 
    {
        int pos;
        if (mode == IO_BEGIN)
            pos = newpos;
        else
            pos = tell() + newpos;
        pos -= abspos - bufend;
        if (pos >= 0 && pos <= bufend) 
        {
            bufpos = pos;
            eof = false;
            return tell();
        }
    }
    return iobase::seek(newpos, mode);
}


bool instm::get_eof() 
{
    if (!eof && bufsize != 0 && bufpos >= bufend)
        bufvalidate();
    return eof;
}


int instm::get_dataavail()
{
    get_eof();
    return bufend - bufpos;
}


char instm::preview() 
{
    if (!eof && bufpos >= bufend)
        bufvalidate();
    if (eof)
        return eofchar;
    return bufdata[bufpos];
}


void instm::putback()
{
    if (!active)
        errstminactive();
    if (bufpos == 0)
        fatal(CRIT_FIRST + 14, "putback() failed");
    bufpos--;
    eof = false;
}


bool instm::get_eol() 
{
    char c = preview();
    return (eof || c == 10 || c == 13);
}


void instm::skipeol() 
{
    switch (preview()) 
    {
    case 10: 
        get(); 
        break;
    case 13:
        get();
        if (preview() == 10)
            get();
        break;
    }
}


char instm::get() 
{
    char ret = preview();
    if (!eof)
        bufpos++;
    return ret;
}


string instm::token(const cset& chars, int limit) 
{
    if (bufsize == 0)
        errbufrequired();
    string ret;
    while (!get_eof()) 
    {
        char* b = bufdata + bufpos;
        char* e = bufdata + bufend;
        char* p = b;
        while (p < e && (*p & chars))
            p++;
        int n = p - b;
        limit -= n;
        if (limit < 0)
        {
            bufpos += n + limit;
            error(ERANGE, "Token too long");
        }
        concat(ret, b, n);
        bufpos += n;
        if (p < e)
            break;
    }
    return ret;
}


string instm::token(const cset& chars) 
{
    return token(chars, INT_MAX);
}


static cset linechars = cset("*") - cset("~0a~0d");


string instm::line(int limit) 
{
    string ret = token(linechars, limit);
    skipeol();
    return ret;
}


string instm::line()
{
    string ret = token(linechars, INT_MAX);
    skipeol();
    return ret;
}


int instm::token(const cset& chars, char* buf, int count) 
{
    if (bufsize == 0)
        errbufrequired();
    int ret = 0;
    while (count > 0 && !get_eof()) 
    {
        char* b = bufdata + bufpos;
        char* e = b + imin(count, bufend - bufpos);
        char* p = b;
        while (p < e && (*p & chars))
            p++;
        int n = p - b;
        memcpy(buf, b, n);
        buf += n;
        ret += n;
        count -= n;
        bufpos += n;
        if (p < e)
            break;
    }
    return ret;
}


int instm::line(char* buf, int size, bool eateol) 
{
    int ret = token(linechars, buf, size);
    if (eateol)
        skipeol();
    return ret;
}


int instm::read(void* buf, int count) 
{
    int ret = 0;
    if (bufsize == 0) 
        ret = rawread(pchar(buf), count);
    else 
    {
        while (count > 0 && !get_eof()) 
        {
            int n = imin(count, bufend - bufpos);
            memcpy(buf, bufdata + bufpos, n);
            buf = pchar(buf) + n;
            ret += n;
            count -= n;
            bufpos += n;
        }
    }
    return ret;
}


int instm::skip(int count) 
{
    int ret = 0;
    if (bufsize == 0)
        errbufrequired();
    else
        while (count > 0 && !get_eof()) 
        {
            int n = imin(count, bufend - bufpos);
            ret += n;
            count -= n;
            bufpos += n;
        }
    return ret;
}


int instm::skiptoken(const cset& chars) 
{
    int ret = 0;
    if (bufsize == 0)
        errbufrequired();
    while (!get_eof()) {
        char* b = bufdata + bufpos;
        char* e = bufdata + bufend;
        char* p = b;
        while (p < e && (*p & chars))
            p++;
        int n = p - b;
        bufpos += n;
        ret += n;
        if (p < e)
            break;
    }
    return ret;
}


void instm::skipline(bool eateol) 
{
    if (!get_eol())
        skiptoken(linechars);
    if (eateol)
        skipeol();
}


PTYPES_END
