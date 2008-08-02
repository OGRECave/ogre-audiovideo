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


inmemory::inmemory(const string& imem)
    : instm(length(imem)), mem(imem) 
{
}


inmemory::~inmemory() 
{
    close();
}


int inmemory::classid()
{
    return CLASS_INMEMORY;
}


void inmemory::bufalloc() 
{
    bufdata = pchar(pconst(mem));
    bufend = length(mem);
}


void inmemory::buffree() 
{
    bufclear();
    bufdata = nil;
}


void inmemory::bufvalidate() 
{
    eof = bufpos >= bufend;
}


void inmemory::doopen() 
{
    if (isempty(mem))
        eof = true;
}


void inmemory::doclose() 
{
}


int inmemory::dorawread(char*, int) 
{
    return 0;
}


string inmemory::get_streamname() 
{
    return "mem";
}


PTYPES_END
