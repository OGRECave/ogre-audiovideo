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


objlist::objlist()
    : unknown(), list(nil), count(0), capacity(0), ownobjects(false) {}


objlist::objlist(bool iownobjects)
    : unknown(), list(nil), count(0), capacity(0), ownobjects(iownobjects) {}


objlist::~objlist() 
{
    clear();
}


void objlist::idxerror() const 
{
    fatal(CRIT_FIRST + 30, "Object list index out of bounds");
}


void objlist::setcapacity(int newcap) 
{
    if (newcap != capacity) 
    {
        list = (punknown*)(memrealloc((void*)(list), newcap * sizeof(punknown)));
        capacity = newcap;
    }
}


static int getlistdelta(int capacity) 
{
    if (capacity > 64)
        return capacity / 4;
    else if (capacity > 8)
        return 16;
    else
        return 4;
}


static int quantizecap(int v) 
{
    int delta = getlistdelta(v);
    return ((v + delta - 1) / delta) * delta;
}


void objlist::grow() 
{
    setcapacity(capacity + getlistdelta(capacity));
}


void objlist::clear() 
{
    setcount(0);
    setcapacity(0);
}


void objlist::setcount(int newcount) 
{
    if (newcount > count) 
    {
        if (newcount > capacity)
            setcapacity(quantizecap(newcount));
        memset(list + count, 0, (newcount - count) * sizeof(punknown));
        count = newcount;
    }
    else if (newcount < count) 
    {
        if (ownobjects)
            while (count > newcount)
                delete list[--count];
        else
            count = newcount;
        setcapacity(quantizecap(newcount));
    }
}


void objlist::insitem(int i, unknown* iobj) 
{
    if (count == capacity)
        grow();
    punknown* s = list + i;
    if (i < count)
        memmove(s + 1, s, (count - i) * sizeof(punknown));
    *s = iobj;
    count++;
}


void objlist::putitem(int i, unknown* iobj) 
{
    punknown* s = list + i;
    if (ownobjects)
        delete *s;
    *s = iobj;
}


void objlist::delitem(int i) 
{
    punknown* s = list + i;
    if (ownobjects)
        delete *s;
    count--;
    if (i < count)
        memmove(s, s + 1, (count - i) * sizeof(punknown));
}


void ins(objlist& s, int i, unknown* iobj) 
{
    if (i < 0 || i > s.count)
        s.idxerror();
    s.insitem(i, iobj);
}


int add(objlist& s, unknown* iobj) 
{
    s.insitem(s.count, iobj);
    return s.count - 1;
}


void put(objlist& s, int i, unknown* iobj) 
{
    s.checkidx(i);
    s.putitem(i, iobj);
}


void del(objlist& s, int i) 
{
    s.checkidx(i);
    s.delitem(i);
}


int indexof(const objlist& s, unknown* iobj) 
{
    for (int i = 0; i < s.count; i++)
        if (s.list[i] == iobj)
            return i;
    return -1;
}


unknown* pop(objlist& s) 
{
    unknown* ret = get(s, s.count - 1);
    s.count--;
    return ret;
}


#ifdef CHECK_BOUNDS

unknown* get(const objlist& s, int i) 
{
    s.checkidx(i);
    return s.getitem(i);
}

#endif


PTYPES_END

