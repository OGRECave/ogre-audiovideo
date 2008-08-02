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

#include <string.h>

#ifdef WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif

#include "pstreams.h"


PTYPES_BEGIN


outstm::outstm(bool iflusheol, int ibufsize)
    : iobase(ibufsize), flusheol(iflusheol) {}


outstm::~outstm() 
{
}


int outstm::classid()
{
    return CLASS_OUTSTM;
}


int outstm::dorawwrite(const char* buf, int count)
{
    if (handle == invhandle)
	return -1;
#ifdef WIN32
    unsigned long ret;
    if (!WriteFile(HANDLE(handle), buf, count, &ret, nil)) 
    {
        error(uerrno(), "Couldn't write");
        ret = uint(-1);
    }
#else
    int ret;
    if ((ret = ::write(handle, buf, count)) < 0)
        error(uerrno(), "Couldn't write");
#endif
    return ret;
}


int outstm::rawwrite(const char* buf, int count) 
{
    if (!active)
        errstminactive();
    try 
    {
        int ret = dorawwrite(buf, count);
        if (ret < 0)
            ret = 0;
        else
            abspos += ret;
        chstat(IO_WRITING);
        if (ret < count) 
        {
            eof = true;
            chstat(IO_EOF);
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


void outstm::bufvalidate() 
{
    if (!active)
        errstminactive();
    if (bufend > 0)
        rawwrite(bufdata, bufend);
    bufclear();
}


int outstm::seek(int newpos, ioseekmode mode) 
{
    if (bufsize != 0 && mode != IO_END) 
    {
        int pos;
        if (mode == IO_BEGIN)
            pos = newpos;
        else
            pos = tell() + newpos;
        pos -= abspos;
        if (pos >= 0 && pos <= bufend) 
        {
            bufpos = pos;
            eof = false;
            return tell();
        }
    }
    return iobase::seek(newpos, mode);
}


bool outstm::canwrite() 
{
    if (bufsize > 0 && bufpos >= bufsize) 
    {
        bufvalidate();
        return bufend < bufsize;
    }
    else
        return true;
}


void outstm::flush() 
{
    if (bufsize > 0 && stmerrno == 0)
        bufvalidate();
}


void outstm::put(char c) 
{
    if (!active)
        errstminactive();
    if (bufsize == 0)
        rawwrite(&c, 1);
    else if (canwrite()) 
    {
        bufdata[bufpos] = c;    
        bufadvance(1);
        if (c == 10 && flusheol)
            flush();
    }
}


int outstm::write(const void* buf, int count) 
{
    if (!active)
        errstminactive();
    int ret = 0;
    if (bufsize == 0)
        ret = rawwrite(pconst(buf), count);
    else 
    {
        while (count > 0 && canwrite()) 
        {
            int n = imin(count, bufsize - bufpos);
            memcpy(bufdata + bufpos, buf, n);
            ret += n;
            count -= n;
            buf = pconst(buf) + n;
            bufadvance(n);
        }
    }

    return ret;
}


void outstm::put(const char* str) 
{
    if (str != nil)
        write(str, hstrlen(str));
}


void outstm::put(const string& str) 
{
    write(pconst(str), length(str));
}


void outstm::putline(const char* s) 
{
    put(s);
    puteol();
}


void outstm::putline(const string& s) 
{
    put(s);
    puteol();
}


void outstm::puteol() 
{
#ifdef WIN32
    put(13);
#endif
    put(10);
}


PTYPES_END
