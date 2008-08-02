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


large stringtoi(const char* p)
{
    if (p == 0)
        return -1;
    if (*p == 0)
        return -1;

    large r = 0;
    do 
    {
        char c = *p++;
        if (c < '0' || c > '9')
            return -1;              // invalid character
        large t = r * 10;
        if (t < r)
            return -1;              // overflow
        t += c - '0';
        if (t < r)
            return -1;              // overflow
        r = t;
    } while (*p != 0);

    return r;
}


econv::~econv()
{
}


static void throw_conv(const char* p)
{
    throw new econv("Invalid number: '" + string(p) + '\'');
}


static void throw_overflow(const char* p)
{
    throw new econv("Out of range: '" + string(p) + '\'');
}


ularge stringtoue(const char* str, int base)
{
    if (str == 0)
        throw_conv(str);
    if (*str == 0 || base < 2 || base > 64)
        throw_conv(str);

    const char* p = str;
    ularge result = 0;

    do 
    {
        int c = *p++;

        if (c >= 'a')
        {
            // for the numeration bases that use '.', '/', digits and
            // uppercase letters the letter case is insignificant.
            if (base <= 38)
                c -= 'a' - '9' - 1;
            else  // others use both upper and lower case letters
                c -= ('a' - 'Z' - 1) + ('A' - '9' - 1);
        }
        else if (c > 'Z')
            throw_conv(str);
        else if (c >= 'A')
            c -= 'A' - '9' - 1;
        else if (c > '9')
            throw_conv(str);

        c -= (base > 36) ? '.' : '0';
        if (c < 0 || c >= base)
            throw_conv(str);

        ularge t = result * uint(base);
        if (t / base != result)
            throw_overflow(str);
        result = t;
        t = result + uint(c);
        if (t < result)
            throw_overflow(str);
        result = t;
    } while (*p != 0);

    return result;
}


large stringtoie(const char* str)
{
    if (str == 0)
        throw_conv(str);
    bool neg = *str == '-';
    ularge result = stringtoue(str + int(neg), 10);
    if (result > (ularge(LARGE_MAX) + uint(neg)))
        throw_overflow(str);
    if (neg)
        return - large(result);
    else
        return large(result);
}


PTYPES_END

