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


component::component()
    : unknown(), refcount(0), freelist(nil), typeinfo(nil)  {}


component::~component() 
{
    if (freelist != nil) 
    {
        for (int i = 0; i < length(*freelist); i++) 
        {
            component* p = pcomponent((*freelist)[i]);
            p->freenotify(this);
        }
        delete freelist;
        freelist = nil;
    }
}


void component::freenotify(component*) 
{
}


void component::addnotification(component* obj) 
{
    if (freelist == nil)
        freelist = new objlist();
    add(*freelist, obj);
}


void component::delnotification(component* obj) 
{
    int i = -1;
    if (freelist != nil) 
    {
        i = indexof(*freelist, obj);
        if (i >= 0) {
            del(*freelist, i);
            if (length(*freelist) == 0) 
            {
                delete freelist;
                freelist = nil;
            }
        }
    }
    if (i == -1)
        fatal(CRIT_FIRST + 1, "delnotification() failed: no such object");
}


int component::classid()
{
    return CLASS_UNDEFINED;
}


component* addref(component* c)
{
    if (c != nil)
#ifdef PTYPES_ST
        c->refcount++;
#else
        pincrement(&c->refcount);
#endif
    return c;
}


bool release(component* c)
{
    if (c != nil)
    {
#ifdef PTYPES_ST
        if (--c->refcount == 0)
#else
        if (pdecrement(&c->refcount) == 0)
#endif
            delete c;
        else
            return false;
    }
    return true;
}


PTYPES_END
