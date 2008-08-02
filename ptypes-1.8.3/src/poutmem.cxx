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

#include "pstreams.h"


PTYPES_BEGIN


outmemory::outmemory(int ilimit, int iincrement)
    : outstm(false, 0), mem(), limit(ilimit), increment(iincrement)
{
}


outmemory::~outmemory()
{
    close();
}


int outmemory::classid()
{
    return CLASS_OUTMEMORY;
}


void outmemory::doopen()
{
}


void outmemory::doclose()
{
    clear(mem);
}


int outmemory::dorawwrite(const char* buf, int count)
{
    if (count <= 0)
        return 0;
    if (limit > 0 && abspos + count > limit)
    {
        count = limit - abspos;
        if (count <= 0)
            return 0;
    }
    int cursize = length(mem);
    int newsize = abspos + count;
    if (newsize > cursize)
    {
        newsize = ((newsize - 1) / increment + 1) * increment;
        setlength(mem, newsize);
    }
    memcpy(pchar(pconst(mem)) + abspos, buf, count);
    return count;
}


string outmemory::get_streamname() 
{
    return "mem";
}


string outmemory::get_strdata()
{
    if (!active)
        errstminactive();
    setlength(mem, abspos);
    string result = mem;
    close();
    return result;
}


PTYPES_END
