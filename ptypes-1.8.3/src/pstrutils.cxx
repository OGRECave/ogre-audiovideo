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

#include "ptypes.h"


PTYPES_BEGIN


string fill(int width, char pad) 
{
    string res;
    if (width > 0) {
        setlength(res, width);
        memset(pchar(pconst(res)), pad, length(res));
    }
    return res;
}


string pad(const string& s, int width, char c, bool left)
{
    int len = length(s);
    if (len < width && width > 0)
    {
        string res;
        setlength(res, width);
        if (left)
        {
            if (len > 0)
                memcpy(pchar(pconst(res)), pconst(s), len);
            memset(pchar(pconst(res)) + len, c, width - len);
        }
        else
        {
            memset(pchar(pconst(res)), c, width - len);
            if (len > 0)
                memcpy(pchar(pconst(res)) + width - len, pconst(s), len);
        }
        return res;
    }
    else
        return s;
}


PTYPES_END
