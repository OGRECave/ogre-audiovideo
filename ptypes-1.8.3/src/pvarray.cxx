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


_varray::_varray()
    : strlist(slflags(SL_OWNOBJECTS | SL_SORTED | SL_CASESENS)), refcount(0) 
{
}


_varray::_varray(const _varray& a)
    : strlist(slflags(SL_OWNOBJECTS | SL_SORTED | SL_CASESENS)), refcount(0) 
{
    setcapacity(a.count);
    for (int i = 0; i < a.count; i++)
    {
        _stritem* p = a.list + i;
        insitem(i, p->str, new varobject(pvarobject(p->obj)->var));
    }
}


const variant& _varray::getvar(const string& key) const
{
    int i;
    if (search(key, i))
        return pvarobject(getobj(i))->var;
    else
        return nullvar;
}


const variant& _varray::getvar(int index) const
{
    if (index >= 0 && index < count)
        return pvarobject(getobj(index))->var;
    else
        return nullvar;
}


string _varray::getkey(int index) const
{
    if (index >= 0 && index < count)
        return getstr(index);
    else
        return nullstring;
}


void _varray::putvar(const string& key, const variant& v)
{
    int i;
    if (search(key, i))
    {
        if (v.tag != VAR_NULL)
	{
            // for some mysterious reason GCC doesn't accept
            // the shorter form of the following, i.e.
            // pvarobject(getobj(i))->var = v
            varobject* p = (varobject*)getobj(i);
            p->var = v;
	}
        else
            delitem(i);
    }
    else if (v.tag != VAR_NULL)
        insitem(i, key, new varobject(v));
}


int _varray::addvar(const variant& v)
{
    int i;
    if (count > 0 && isempty(list[count - 1].str))
        i = count;
    else
        i = 0;
    insitem(i, nullstring, new varobject(v));
    return i;
}


void _varray::putvar(int index, const variant& v)
{
    if (index >= 0 && index < count)
    {
	varobject* p = (varobject*)getobj(index);
        p->var = v;
    }
}


void _varray::insvar(int index, const variant& v)
{
    if (index >= 0 && index <= count)
        insitem(index, nullstring, new varobject(v));
}


void _varray::delvar(const string& key)
{
    int i;
    if (search(key, i))
        delitem(i);
}


void _varray::delvar(int index)
{
    if (index >= 0 && index < count)
        delitem(index);
}


void aclear(variant& v)
{
    if (v.tag == VAR_ARRAY)
        clear(*v.value.a);
    else
    {
        v.finalize();
        v.initialize(new _varray());
    }
}


variant aclone(const variant& v)
{
    if (v.tag == VAR_ARRAY)
        return variant(new _varray(*(v.value.a)));
    else
        return variant(new _varray());
}


int alength(const variant& v)
{
    if (v.tag == VAR_ARRAY)
        return length(*v.value.a);
    else
        return 0;
}


const variant& get(const variant& v, const string& key)
{
    if (v.tag == VAR_ARRAY)
        return v.value.a->getvar(key);
    else
        return nullvar;
}


const variant& get(const variant& v, large key)
{
    return get(v, itostring(key, 16, 16, '0'));
}


void put(variant& v, const string& key, const variant& item)
{
    if (v.tag != VAR_ARRAY)
        aclear(v);
    v.value.a->putvar(key, item);
}


void put(variant& v, large key, const variant& item)
{
    put(v, itostring(key, 16, 16, '0'), item);
}


void del(variant& v, const string& key)
{
    if (v.tag == VAR_ARRAY)
        v.value.a->delvar(key);
}


void del(variant& v, large key)
{
    del(v, itostring(key, 16, 16, '0'));
}


bool anext(const variant& array, int& index, variant& item)
{
    string key;
    return anext(array, index, item, key);
}


bool anext(const variant& array, int& index, variant& item, string& key)
{
    if (array.tag != VAR_ARRAY)
    {
        clear(item);
        return false;
    }
    if (index < 0 || index >= length(*array.value.a))
    {
        clear(item);
        return false;
    }
    item = pvarobject(array.value.a->getobj(index))->var;
    key = array.value.a->getstr(index);
    index++;
    return true;
}


int aadd(variant& array, const variant& item)
{
    if (array.tag != VAR_ARRAY)
        aclear(array);
    return array.value.a->addvar(item);
}


const variant& aget(const variant& array, int index)
{
    if (array.tag == VAR_ARRAY)
        return array.value.a->getvar(index);
    else
        return nullvar;
}


string akey(const variant& array, int index)
{
    if (array.tag == VAR_ARRAY)
        return array.value.a->getkey(index);
    else
        return nullstring;
}


void adel(variant& array, int index)
{
    if (array.tag == VAR_ARRAY)
        array.value.a->delvar(index);
}

    
void aput(variant& array, int index, const variant& item)
{
    if (array.tag == VAR_ARRAY)
        array.value.a->putvar(index, item);
}


void ains(variant& array, int index, const variant& item)
{
    if (array.tag == VAR_ARRAY)
        array.value.a->insvar(index, item);
}


PTYPES_END
