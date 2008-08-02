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


strlist::strlist()
    : unknown(), list(nil), count(0), capacity(0), flags(slflags(0)) {}


strlist::strlist(slflags iflags)
    : unknown(), list(nil), count(0), capacity(0), flags(iflags) {}


strlist::~strlist() 
{
    clear();
}


void strlist::idxerror() const 
{
    fatal(CRIT_FIRST + 31, "String list index out of bounds");
}


void strlist::sortederror() const 
{
    fatal(CRIT_FIRST + 32, "Operation not allowed on sorted string lists");
}


void strlist::notsortederror() const 
{
    fatal(CRIT_FIRST + 33, "Search only allowed on sorted string lists");
}


void strlist::duperror() const 
{
    fatal(CRIT_FIRST + 34, "Duplicate items not allowed in this string list");
}


int strlist::compare(const string& s1, const string& s2) const 
{
    if (get_casesens())
        return strcmp(s1, s2);
    else
        return strcasecmp(s1, s2);
}


void strlist::setcapacity(int newcap) 
{
    if (newcap != capacity) 
    {
        list = pstritem(memrealloc((void*)(list), newcap * _stritemsize));
        capacity = newcap;
    }
}


void strlist::grow() 
{
    int delta;
    if (capacity > 64)
        delta = capacity / 4;
    else if (capacity > 8)
        delta = 16;
    else
        delta = 4;
    setcapacity(capacity + delta);
}


void strlist::clear()
{
    while (count > 0)
        delitem(count - 1);
    setcapacity(0);
}


void strlist::insitem(int i, const string& istr, unknown* iobj)
{
    if (count == capacity)
        grow();
    _stritem* s = list + i;
    if (i < count)
        memmove(s + 1, s, (count - i) * _stritemsize);
    initialize(s->str, istr);
    s->obj = iobj;
    count++;
}


void strlist::putstr(int i, const string& istr)
{
    list[i].str = istr;
}


void strlist::putobj(int i, unknown* iobj) 
{
    _stritem* s = list + i;
    if (get_ownobjects())
        delete s->obj;
    s->obj = iobj;
}


void strlist::delitem(int i) 
{
    _stritem* s = list + i;
    finalize(s->str);
    if (get_ownobjects())
        delete s->obj;
    count--;
    if (i < count)
        memmove(s, s + 1, (count - i) * _stritemsize);
}


bool strlist::search(const string& s, int& index) const 
{
    int l, h, i, c;
    bool ret = false;
    l = 0;
    h = count - 1;
    while (l <= h) 
    {
        i = (l + h) / 2;
        c = compare(list[i].str, s);
        if (c < 0)
            l = i + 1;
        else 
        {
            h = i - 1;
            if (c == 0) 
            {
                ret = true;
                if (!get_duplicates())
                    l = i;
            }
        }
    }
    index = l;
    return ret;
}


#ifdef CHECK_BOUNDS

const string& getstr(const strlist& s, int i) 
{
    s.checkidx(i);
    return s.getstr(i);
}


unknown* get(const strlist& s, int i) 
{
    s.checkidx(i);
    return s.getobj(i);
}

#endif


void ins(strlist& s, int i, const string& istr, unknown* iobj) 
{
    if (i < 0 || i > s.count)
        s.idxerror();
    if (s.get_sorted())
        s.sortederror();
    s.insitem(i, istr, iobj);
}


void put(strlist& s, int i, const string& istr, unknown* iobj) 
{
    s.checkidx(i);
    if (s.get_sorted())
        s.sortederror();
    s.putstr(i, istr);
    s.putobj(i, iobj);
}


void put(strlist& s, int i, unknown* iobj) 
{
    s.checkidx(i);
    s.putobj(i, iobj);
}


int add(strlist& s, const string& istr, unknown* iobj) 
{
    int i;
    if (s.get_sorted()) 
    {
        if (s.search(istr, i) && !s.get_duplicates())
            s.duperror();
    }
    else
        i = s.count;
    s.insitem(i, istr, iobj);
    return i;
}


void del(strlist& s, int i) 
{
    s.checkidx(i);
    s.delitem(i);
}


bool search(const strlist& s, const string& istr, int& index) 
{
    if (!s.get_sorted())
        s.notsortederror();
    return s.search(istr, index);
}


int find(const strlist& s, const string& istr) 
{
    if (s.get_sorted()) 
    {
        int i;
        if (s.search(istr, i))
            return i;
    }
    else 
    {
        for (int i = 0; i < s.count; i++)
            if (s.compare(s.list[i].str, istr) == 0)
                return i;
    }
    return -1;
}


int indexof(const strlist& s, unknown* iobj) 
{
    for (int i = 0; i < s.count; i++)
        if (s.list[i].obj == iobj)
            return i;
    return -1;
}


string valueof(const strlist& s, const char* key) 
{
    string str = string(key) + '=';
    string ret;
    if (s.get_sorted()) 
    {
        int i;
        s.search(str, i);
        if (i < s.count) 
        {
            ret = s.list[i].str;
            if (contains(str, ret, 0))
                del(ret, 0, length(str));
            else
                clear(ret);
        }
    }
    else 
    {
        for (int i = 0; i < s.count; i++) 
        {
            ret = s.list[i].str;
            if (contains(str, ret, 0)) 
            {
                del(ret, 0, length(str));
                break;
            }
            else
                clear(ret);
        }
    }
    return ret;
}


//
// strmap
//

strmap::strmap()
    : strlist(SL_SORTED) {}


strmap::strmap(slflags iflags)
    : strlist(slflags((iflags | SL_SORTED) & ~SL_DUPLICATES)) {}


strmap::~strmap() 
{
    clear();
}


unknown* strmap::getobj(const string& istr) const 
{
    int i;
    if (strlist::search(istr, i))
        return strlist::getobj(i);
    else
        return nil;
}


unknown* strmap::getobj(const char* istr) const 
{
    return getobj(string(istr));
}


void strmap::putobj(const string& istr, unknown* iobj) 
{
    int i;
    if (strlist::search(istr, i)) {
        if (iobj != nil)
            strlist::putobj(i, iobj);
        else
            strlist::delitem(i);
    }
    else
        if (iobj != nil)
            strlist::insitem(i, istr, iobj);
}



//
// textmap
//


textmap::textmap()
    : strlist(SL_SORTED) {}


textmap::~textmap() 
{
    clear();
}


void textmap::delitem(int i)
{
    finalize(PTR_TO_STRING(list[i].obj));
    strlist::delitem(i);
}


void textmap::clear()
{
    while (count > 0)
        delitem(count - 1);
    setcapacity(0);
}


const string& textmap::getvalue(const string& name) const
{
    int i;
    if (strlist::search(name, i))
        return PTR_TO_STRING(list[i].obj);
    else
        return nullstring;
}


const string& textmap::getvalue(const char* name) const
{
    return getvalue(string(name));
}


void textmap::putvalue(const string& name, const string& value) 
{
    int i;
    if (strlist::search(name, i)) {
        if (!isempty(value))
            PTR_TO_STRING(list[i].obj) = value;
        else
            delitem(i);
    }
    else
        if (!isempty(value))
        {
            insitem(i, name, nil);
            initialize(PTR_TO_STRING(list[i].obj), value);
        }
            
}



PTYPES_END
