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

#include "ptypes.h"


PTYPES_BEGIN

int objalloc = 0;


unknown::unknown() 
{
    objalloc++;
}


unknown::~unknown() 
{
    objalloc--;
}


PTYPES_END
