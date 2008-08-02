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
#include <limits.h>

#include "ptypes.h"


PTYPES_BEGIN


const variant nullvar;


static void vfatal()
{
    fatal(CRIT_FIRST + 60, "Variant data corrupt");
}


static void vconverr(large v)
{
    throw new evariant("Value out of range: " + itostring(v));
}


evariant::~evariant()
{
}


void variant::initialize(_varray* a)
{
    tag = VAR_ARRAY;
    pincrement(&a->refcount);
    value.a = a;
}


void variant::initialize(component* o)
{
    tag = VAR_OBJECT;
    value.o = addref(o);
}


void variant::initialize(const variant& v)
{
    switch (v.tag)
    {
    case VAR_NULL:
        tag = VAR_NULL;
        break;
    case VAR_INT:
    case VAR_BOOL:
    case VAR_FLOAT:
        tag = v.tag;
        value = v.value;
        break;
    case VAR_STRING:
        initialize(PTR_TO_STRING(v.value.s));
        break;
    case VAR_ARRAY:
        initialize(v.value.a);
        break;
    case VAR_OBJECT:
        initialize(v.value.o);
        break;
    default:
        vfatal();
    }
}


void variant::finalize()
{
    if (tag >= VAR_COMPOUND)
    {
        switch (tag)
        {
        case VAR_STRING:
            PTYPES_NAMESPACE::finalize(PTR_TO_STRING(value.s));
            break;
        case VAR_ARRAY:
            if (pdecrement(&value.a->refcount) == 0)
                delete value.a;
            break;
        case VAR_OBJECT:
            release(value.o);
            break;
        default:
            vfatal();
        }
    }
    tag = VAR_NULL;
}


void variant::assign(large v)           { finalize(); initialize(v); }
void variant::assign(bool v)            { finalize(); initialize(v); }
void variant::assign(double v)          { finalize(); initialize(v); }
void variant::assign(const char* v)     { finalize(); initialize(v); }


void variant::assign(const string& v)   
{ 
    if (tag == VAR_STRING)
        PTR_TO_STRING(value.s) = v;
    else
    {
        finalize();
        initialize(v);
    }
}


void variant::assign(_varray* a)
{
    if (tag == VAR_ARRAY)
        if (value.a == a)
            return;
    finalize();
    initialize(a);
}


void variant::assign(component* o)
{
    if (tag == VAR_OBJECT)
    {
        if (value.o == o)
            return;
        else
            release(value.o);
    }
    else
        finalize();
    initialize(o);
}


void variant::assign(const variant& v)
{
    switch (v.tag)
    {
    case VAR_NULL:
        finalize();
        tag = VAR_NULL;
        break;
    case VAR_INT:
    case VAR_BOOL:
    case VAR_FLOAT:
        finalize();
        tag = v.tag;
        value = v.value;
        break;
    case VAR_STRING:
        assign(PTR_TO_STRING(v.value.s));
        break;
    case VAR_ARRAY:
        assign(v.value.a);
        break;
    case VAR_OBJECT:
        assign(v.value.o);
        break;
    default:
        vfatal();
    }
}


void clear(variant& v)
{
    v.finalize();
    v.initialize();
}


variant::operator int() const
{
    large t = operator large();
    if (t < INT_MIN || t > INT_MAX)
        vconverr(t);
    return int(t);
}


variant::operator unsigned int() const
{
    large t = operator large();
    if (t < 0 || t > UINT_MAX)
        vconverr(t);
    return uint(t);
}


variant::operator long() const
{
    large t = operator large();
    if (t < LONG_MIN || t > LONG_MAX)
        vconverr(t);
    return int(t);
}


variant::operator unsigned long() const
{
    large t = operator large();
    if (t < 0 || t > large(ULONG_MAX))
        vconverr(t);
    return uint(t);
}


variant::operator large() const
{
    switch(tag)
    {
    case VAR_NULL: return 0;
    case VAR_INT: return value.i;
    case VAR_BOOL: return int(value.b);
    case VAR_FLOAT: return int(value.f);
    case VAR_STRING: 
        {
            const char* p = PTR_TO_STRING(value.s);
            bool neg = *p == '-';
            if (neg)
                p++;
            large t = stringtoi(p);
            if (t < 0)
                return 0;
            else
                return neg ? -t : t;
        }
    case VAR_ARRAY: return value.a->count != 0;
    case VAR_OBJECT: return 0;
    default: vfatal();
    }
    return 0;
}


variant::operator bool() const
{
    switch(tag)
    {
    case VAR_NULL: return false;
    case VAR_INT: return value.i != 0;
    case VAR_BOOL: return value.b;
    case VAR_FLOAT: return value.f != 0;
    case VAR_STRING: return !isempty((PTR_TO_STRING(value.s)));
    case VAR_ARRAY: return value.a->count != 0;
    case VAR_OBJECT: return value.o != nil;
    default: vfatal();
    }
    return false;
}


variant::operator double() const
{
    switch(tag)
    {
    case VAR_NULL: return 0;
    case VAR_INT: return double(value.i);
    case VAR_BOOL: return int(value.b);
    case VAR_FLOAT: return value.f;
    case VAR_STRING: 
        {
            char* e;
            double t = strtod(PTR_TO_STRING(value.s), &e);
            if (*e != 0)
                return 0;
            else
                return t;
        }
    case VAR_ARRAY: return int(value.a->count != 0);
    case VAR_OBJECT: return 0;
    default: vfatal();
    }
    return 0;
}


void string::initialize(const variant& v)
{
    switch(v.tag)
    {
    case VAR_NULL: initialize(); break;
    case VAR_INT: initialize(itostring(v.value.i)); break;
    case VAR_BOOL: if (v.value.b) initialize('1'); else initialize('0'); break;
    case VAR_FLOAT:
        {
            char buf[256];
            sprintf(buf, "%g", v.value.f);
            initialize(buf);
        }
        break;
    case VAR_STRING: initialize(PTR_TO_STRING(v.value.s)); break;
    case VAR_ARRAY: initialize(); break;
    case VAR_OBJECT: initialize(); break;
    default: vfatal();
    }
}


variant::operator string() const
{
    // this is a 'dirty' solution to gcc 3.3 typecast problem. most compilers
    // handle variant::operator string() pretty well, while gcc 3.3 requires
    // to explicitly declare a constructor string::string(const variant&).
    // ironically, the presence of both the typecast operator and the constructor
    // confuses the MSVC compiler. so the only thing we can do to please all 
    // those compilers [that "move towards the c++ standard"] is to conditionally
    // exclude the constructor string(const variant&). and this is not the whole
    // story. i see you are bored with it and i let you go. nobody would ever care
    // about this. it just works, though i'm not happy with what i wrote here:
    string t;
    t.initialize(*this);
    return t;
}


variant::operator component*() const
{
    if (tag == VAR_OBJECT)
        return value.o;
    else
        return nil;
}


bool variant::equal(const variant& v) const
{
    if (tag != v.tag)
        return false;
    switch (tag)
    {
    case VAR_NULL: return true;
    case VAR_INT: return value.i == v.value.i;
    case VAR_BOOL: return value.b == v.value.b;
    case VAR_FLOAT: return value.f == v.value.f;
    case VAR_STRING: return strcmp(value.s, v.value.s) == 0;
    case VAR_ARRAY: return value.a == v.value.a;
    case VAR_OBJECT: return value.o == v.value.o;
    default: vfatal(); return false;
    }
}


varobject::~varobject()
{
}


_varray::~_varray()
{
}


PTYPES_END
