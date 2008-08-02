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
#include <stdio.h>
#include <string.h>

#include "pport.h"

#if defined(WIN32) && !defined(NO_CRIT_MSGBOX)
#  include <windows.h>
#  define CRIT_MSGBOX
#endif


PTYPES_BEGIN


static void defhandler(int code, const char* msg) 
{
#ifdef CRIT_MSGBOX
    char buf[2048];
    _snprintf(buf, sizeof(buf) - 1, "Fatal [%05x]: %s", code, msg);
    MessageBox(0, buf, "Internal error", MB_OK | MB_ICONSTOP);
#else
    fprintf(stderr, "\nInternal [%04x]: %s\n", code, msg);
#endif
}

static _pcrithandler crith = defhandler;


_pcrithandler getcrithandler() 
{
    return crith;
}


_pcrithandler setcrithandler(_pcrithandler newh) 
{
    _pcrithandler ret = crith;
    crith = newh;
    return ret;
}


void fatal(int code, const char* msg)
{
    if (crith != nil)
        (*crith)(code, msg);
    exit(code);
}


PTYPES_END
