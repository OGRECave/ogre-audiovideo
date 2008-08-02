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

#include "pport.h"


PTYPES_BEGIN


void memerror() 
{
    fatal(CRIT_FIRST + 5, "Not enough memory");
}


void* memalloc(uint a) 
{
    if (a == 0)
        return nil;
    else
    {
        void* p = malloc(a);
        if (p == nil) 
            memerror();
        return p;
    }
}


void* memrealloc(void* p, uint a) 
{
    if (a == 0)
    {
	memfree(p);
	return nil;
    }
    else if (p == nil)
	return memalloc(a);
    else
    {
	p = realloc(p, a);
        if (p == nil) 
	    memerror();
        return p;
    }
}


void memfree(void* p) 
{
    if (p != nil)
        free(p);
}


PTYPES_END
