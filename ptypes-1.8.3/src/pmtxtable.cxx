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


#include "pasync.h"


PTYPES_BEGIN


pmemlock _mtxtable[_MUTEX_HASH_SIZE]
#ifndef WIN32
    = {
    _MTX_INIT, _MTX_INIT,
    _MTX_INIT, _MTX_INIT,
    _MTX_INIT, _MTX_INIT,
    _MTX_INIT, _MTX_INIT,
    _MTX_INIT, _MTX_INIT,
    _MTX_INIT, _MTX_INIT,
    _MTX_INIT, _MTX_INIT,
    _MTX_INIT, _MTX_INIT,
    _MTX_INIT
}
#endif
;


PTYPES_END
