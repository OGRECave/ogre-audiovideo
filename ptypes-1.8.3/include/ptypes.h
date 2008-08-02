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

#ifndef __PTYPES_H__
#define __PTYPES_H__


#ifndef _INC_STRING
#include <string.h>
#endif

#ifndef __PPORT_H__
#include "pport.h"
#endif


PTYPES_BEGIN


#ifdef _MSC_VER
#pragma pack(push, 4)
#endif


#ifdef WIN32
#  define __PFASTCALL __fastcall
#else
#  define __PFASTCALL
#endif

ptpublic int   __PFASTCALL pincrement(int* target);
ptpublic int   __PFASTCALL pdecrement(int* target);
ptpublic int   __PFASTCALL pexchange(int* target, int value);
ptpublic void* __PFASTCALL pexchange(void** target, void* value);

template <class T> inline T* tpexchange(T** target, T* value)
    { return (T*)pexchange((void**)target, (void*)value); }


// -------------------------------------------------------------------- //
// ---  string class -------------------------------------------------- //
// -------------------------------------------------------------------- //


struct _strrec 
{
    int refcount;
    int length;
};
typedef _strrec* _pstrrec;


#define STR_BASE(x)      (_pstrrec(x)-1)
#define STR_REFCOUNT(x)  (STR_BASE(x)->refcount)
#define STR_LENGTH(x)    (STR_BASE(x)->length)

#define PTR_TO_PSTRING(p)   (pstring(&(p)))
#define PTR_TO_STRING(p)    (*PTR_TO_PSTRING(p))

#if (__GNUC__ == 3) && (__GNUC_MINOR__ >= 3)
#  define VARIANT_TYPECAST_HACK
#endif


const int strrecsize = sizeof(_strrec);

ptpublic extern char* emptystr;

ptpublic extern int stralloc;


class ptpublic string 
{
    friend class variant;

protected:
    char* data;

    void _alloc(int);
    void _realloc(int);
    void _free();

    void initialize()                             { data = emptystr; }
    void initialize(const char*, int);
    void initialize(const char*);
    void initialize(char);
    void initialize(const string& s);
    void initialize(const char*, int, const char*, int);
    void initialize(const variant&);
    void finalize();

    void assign(const char*, int);
    void assign(const char*);
    void assign(const string&);
    void assign(char);

    string(const char* s1, int len1, const char* s2, int len2)  { initialize(s1, len1, s2, len2); }

public:
    ptpublic friend int  length(const string& s);
    ptpublic friend int  refcount(const string& s);
    ptpublic friend void assign(string& s, const char* buf, int len);
    ptpublic friend void clear(string& s);
    ptpublic friend bool isempty(const string& s);

    ptpublic friend void   setlength(string&, int);
    ptpublic friend char*  unique(string&);
    ptpublic friend void   concat(string& s, const char* sc, int catlen);
    ptpublic friend void   concat(string& s, const char* s1);
    ptpublic friend void   concat(string& s, char s1);
    ptpublic friend void   concat(string& s, const string& s1);
    ptpublic friend string copy(const string& s, int from, int cnt);
    ptpublic friend void   ins(const char* s1, int s1len, string& s, int at);
    ptpublic friend void   ins(const char* s1, string& s, int at);
    ptpublic friend void   ins(char s1, string& s, int at);
    ptpublic friend void   ins(const string& s1, string& s, int at);
    ptpublic friend void   del(string& s, int at, int cnt);
    ptpublic friend int    pos(const char* s1, const string& s);
    ptpublic friend int    pos(char s1, const string& s);
    ptpublic friend int    rpos(char s1, const string& s);
    ptpublic friend bool   contains(const char* s1, int len, const string& s, int at);
    ptpublic friend bool   contains(const char* s1, const string& s, int at);
    ptpublic friend bool   contains(char s1, const string& s, int at);
    ptpublic friend bool   contains(const string& s1, const string& s, int at);
    ptpublic friend int    pos(const string& s1, const string& s);
    ptpublic friend string dup(const string& s);

    string()                                      { initialize(); }
    string(const char* sc, int initlen)           { initialize(sc, initlen); }
    string(const char* sc)                        { initialize(sc); }
    string(char c)                                { initialize(c); }
    string(const string& s)                       { initialize(s); }
    ~string()                                     { finalize(); }

#ifdef VARIANT_TYPECAST_HACK
    string(const variant& v)                      { initialize(v); }
#endif

    string& operator=  (const char* sc)           { assign(sc); return *this; }
    string& operator=  (char c)                   { assign(c); return *this; }
    string& operator=  (const string& s)          { assign(s); return *this; }
    string& operator+= (const char* sc)           { concat(*this, sc); return *this; }
    string& operator+= (char c)                   { concat(*this, c); return *this; }
    string& operator+= (const string& s)          { concat(*this, s); return *this; }

    string  operator+  (const char* sc) const;
    string  operator+  (char c) const;
    string  operator+  (const string& s) const;

    ptpublic friend string operator+ (const char* sc, const string& s);
    ptpublic friend string operator+ (char c, const string& s);

    bool    operator== (const char* sc) const     { return strcmp(data, sc) == 0; }
    bool    operator== (char) const;
    bool    operator== (const string&) const;
    bool    operator!= (const char* sc) const     { return !(*this == sc); }
    bool    operator!= (char c) const             { return !(*this == c); }
    bool    operator!= (const string& s) const    { return !(*this == s); }
    operator const char*() const                  { return data; }
    operator const uchar*() const                 { return (uchar*)data; }

    char&   operator[] (int i);

    ptpublic friend void initialize(string& s);
    ptpublic friend void initialize(string& s, const string& s1);
    ptpublic friend void initialize(string& s, const char* s1);
    ptpublic friend void finalize(string& s);
};


typedef string* pstring;


inline int length(const string& s)                      { return STR_LENGTH(s.data); }
inline int refcount(const string& s)                    { return STR_REFCOUNT(s.data); }
inline void assign(string& s, const char* buf, int len) { s.assign(buf, len); }
inline void clear(string& s)                            { s.finalize(); }
inline bool isempty(const string& s)                    { return length(s) == 0; }
inline int pos(const string& s1, const string& s)       { return pos(s1.data, s); }
inline void initialize(string& s)                       { s.initialize(); }
inline void initialize(string& s, const string& s1)     { s.initialize(s1); }
inline void initialize(string& s, const char* s1)       { s.initialize(s1); }
inline void finalize(string& s)                         { s.finalize(); }

#ifndef CHECK_BOUNDS
inline char& string::operator[] (int i)                 { return unique(*this)[i]; }
#endif



ptpublic extern string nullstring;


// -------------------------------------------------------------------- //
// ---  string utilities ---------------------------------------------- //
// -------------------------------------------------------------------- //


ptpublic string fill(int width, char pad);
ptpublic string pad(const string& s, int width, char c, bool left = true);

ptpublic string itostring(large value, int base, int width = 0, char pad = 0);
ptpublic string itostring(ularge value, int base, int width = 0, char pad = 0);
ptpublic string itostring(int value, int base, int width = 0, char pad = 0);
ptpublic string itostring(unsigned value, int base, int width = 0, char pad = 0);
ptpublic string itostring(large v);
ptpublic string itostring(ularge v);
ptpublic string itostring(int v);
ptpublic string itostring(unsigned v);

ptpublic large  stringtoi(const char*);
ptpublic large  stringtoie(const char*);
ptpublic ularge stringtoue(const char*, int base);

ptpublic string lowercase(const char* s);
ptpublic string lowercase(const string& s);

// itobase is now alias to itostring, deprecated
ptpublic string itobase(large value, int base, int width = 0, char pad = 0);


char hex4(char c);

inline char locase(char c) 
    { if (c >= 'A' && c <= 'Z') return char(c + 32); return c; }

inline char upcase(char c) 
    { if (c >= 'a' && c <= 'z') return char(c - 32); return c; }

inline int hstrlen(const char* p) // some Unix systems do not accept NULL
    { return p == nil ? 0 : strlen(p); }




// -------------------------------------------------------------------- //
// ---  character set class ------------------------------------------- //
// -------------------------------------------------------------------- //


const int  _csetbits = 256;
const int  _csetbytes = _csetbits / 8;
const int  _csetwords = _csetbytes / sizeof(int);
const char _csetesc = '~';


class ptpublic cset 
{
protected:
    char data[_csetbytes];

    void assign(const cset& s)                  { memcpy(data, s.data, _csetbytes); }
    void assign(const char* setinit);
    void clear()                                { memset(data, 0, _csetbytes); }
    void fill()                                 { memset(data, -1, _csetbytes); }
    void include(char b)                        { data[uchar(b) / 8] |= uchar(1 << (uchar(b) % 8)); }
    void include(char min, char max);
    void exclude(char b)                        { data[uchar(b) / 8] &= uchar(~(1 << (uchar(b) % 8))); }
    void unite(const cset& s);
    void subtract(const cset& s);
    void intersect(const cset& s);
    void invert();
    bool contains(char b) const                 { return (data[uchar(b) / 8] & (1 << (uchar(b) % 8))) != 0; }
    bool eq(const cset& s) const                { return memcmp(data, s.data, _csetbytes) == 0; }
    bool le(const cset& s) const;

public:
    cset()                                      { clear(); }
    cset(const cset& s)                         { assign(s); }
    cset(const char* setinit)                   { assign(setinit); }

    cset& operator=  (const cset& s)            { assign(s); return *this; }
    cset& operator+= (const cset& s)            { unite(s); return *this; }
    cset& operator+= (char b)                   { include(b); return *this; }
    cset  operator+  (const cset& s) const      { cset t = *this; return t += s; }
    cset  operator+  (char b) const             { cset t = *this; return t += b; }
    cset& operator-= (const cset& s)            { subtract(s); return *this; }
    cset& operator-= (char b)                   { exclude(b); return *this; }
    cset  operator-  (const cset& s) const      { cset t = *this; return t -= s; }
    cset  operator-  (char b) const             { cset t = *this; return t -= b; }
    cset& operator*= (const cset& s)            { intersect(s); return *this; }
    cset  operator*  (const cset& s) const      { cset t = *this; return t *= s; }
    cset  operator!  () const                   { cset t = *this; t.invert(); return t; }
    bool  operator== (const cset& s) const      { return eq(s); }
    bool  operator!= (const cset& s) const      { return !eq(s); }
    bool  operator<= (const cset& s) const      { return le(s); }
    bool  operator>= (const cset& s) const      { return s.le(*this); }

    ptpublic friend cset operator+ (char b, const cset& s);
    ptpublic friend bool operator& (char b, const cset& s);
    ptpublic friend void assign(cset& s, const char* setinit);
    ptpublic friend void clear(cset& s);
    ptpublic friend void fill(cset& s);
    ptpublic friend void include(cset& s, char b);
    ptpublic friend void exclude(cset& s, char b);
    ptpublic friend void include(cset& s, char min, char max);

    ptpublic friend string asstring(const cset& s);
};


inline cset operator+ (char b, const cset& s)     { return s + b; }
inline bool operator& (char b, const cset& s)     { return s.contains(b); }
inline void assign(cset& s, const char* setinit)  { s.assign(setinit); }
inline void clear(cset& s)                        { s.clear(); }
inline void fill(cset& s)                         { s.fill(); }
inline void include(cset& s, char b)              { s.include(b); }
inline void exclude(cset& s, char b)              { s.exclude(b); }
inline void include(cset& s, char min, char max)  { s.include(min, max); }


// -------------------------------------------------------------------- //
// ---  basic abstract class ------------------------------------------ //
// -------------------------------------------------------------------- //


class ptpublic unknown 
{
public:
    unknown();
    virtual ~unknown();
};

typedef unknown* punknown;


ptpublic extern int objalloc;


// -------------------------------------------------------------------- //
// ---  component ----------------------------------------------------- //
// -------------------------------------------------------------------- //


//
// class ID's for all basic types: the first byte (least significant)
// contains the base ID, the next is for the second level of inheritance,
// etc. total of 4 levels allowed for basic types. call classid() for an
// object, mask out first N bytes of interest and compare with a CLASS_XXX
// value. f.ex. to determine whether an object is of type infile or any
// derivative: (o->classid() & 0xffff) == CLASS_INFILE. this scheme is for
// internal use by PTypes and Objection; partly replaces the costly C++ RTTI
// system.
//

// first level of inheritance
const int CLASS_UNDEFINED = 0x00000000;
const int CLASS_INSTM     = 0x00000001;
const int CLASS_OUTSTM    = 0x00000002;
const int CLASS_UNIT      = 0x00000003;

// second level of inheritance
const int CLASS_INFILE    = 0x00000100 | CLASS_INSTM;
const int CLASS_OUTFILE   = 0x00000100 | CLASS_OUTSTM;
const int CLASS_INMEMORY  = 0x00000200 | CLASS_INSTM;
const int CLASS_OUTMEMORY = 0x00000200 | CLASS_OUTSTM;

// third level of inheritance
const int CLASS_LOGFILE   = 0x00010000 | CLASS_OUTFILE;


class objlist;

class ptpublic component: public unknown 
{
protected:
    int       refcount;     // reference counting, used by addref() and release()
    objlist*  freelist;     // list of components to notify about destruction, safer alternative to ref-counting
    void*     typeinfo;     // reserved for future use

    virtual void freenotify(component* sender);

public:
    component();
    virtual ~component();
    void addnotification(component* obj);
    void delnotification(component* obj);
    
    ptpublic friend component* addref(component*);
    ptpublic friend bool release(component*);
    ptpublic friend int refcount(component* c);

    virtual int classid();

    void  set_typeinfo(void* t) { typeinfo = t; }
    void* get_typeinfo()        { return typeinfo; }
};

typedef component* pcomponent;


inline int refcount(component* c)  { return c->refcount; }


template <class T> inline T* taddref(T* c)   // MSVC can't handle this template correctly if we name
    { return (T*)addref((component*)c); }    // it as addref(). (gcc is fine though)


template <class T> class compref
{
protected:
    T* ref;
public:
    compref()                                   { ref = 0; }
    compref(const compref& r)                   { ref = taddref<T>(r.ref); }
    compref(T* u)                               { ref = taddref<T>(u); }
    ~compref()                                  { release(ref); }
    compref& operator= (const compref& r)       { release(ref); ref = taddref<T>(r.ref); return *this; }
    compref& operator= (T* u)                   { release(ref); ref = taddref<T>(u); return *this; }
    T&   operator* ()                           { return *ref; }
    T*   operator-> ()                          { return ref; }
    bool operator== (const compref& r) const    { return ref == r.ref; }
    bool operator== (const T* u) const          { return ref == u; }
    bool operator!= (const compref& r) const    { return ref != r.ref; }
    bool operator!= (const T* u) const          { return ref != u; }
         operator T* ()                         { return ref; }
};


// -------------------------------------------------------------------- //
// ---  exception class ----------------------------------------------- //
// -------------------------------------------------------------------- //


class ptpublic exceptobj: public unknown 
{
protected:
    string message;
public:
    exceptobj(const char* imsg);
    exceptobj(const string& imsg);
    virtual ~exceptobj();
    virtual string get_message() { return message; }
};


//
// conversion exception class
// (see stringtoie() and stringtoue()
//


class ptpublic econv: public exceptobj
{
protected:
public:
    econv(const char* msg): exceptobj(msg)  {}
    econv(const string& msg): exceptobj(msg)  {}
    virtual ~econv();
};


// -------------------------------------------------------------------- //
// ---  object list --------------------------------------------------- //
// -------------------------------------------------------------------- //


class ptpublic objlist: public unknown 
{
protected:
    unknown**   list;
    int         count;
    int         capacity;
    bool        ownobjects;

    void idxerror() const;

    void setcapacity(int newcap);
    int  getcount() const { return count; }
    void setcount(int newcount);
    void grow();
    void clear();
    void insitem(int i, unknown* iobj);
    void putitem(int i, unknown* iobj);
    void delitem(int i);
    unknown* getitem(int i) const  { return list[i]; }

public:
    objlist();
    objlist(bool iownobjects);
    virtual ~objlist();

    void checkidx(int i) const                      { if (i < 0 || i >= count) idxerror(); }

    ptpublic friend int      length(const objlist& s);
    ptpublic friend void     setlength(objlist& s, int newcount);
    ptpublic friend void     pack(objlist& s);
    ptpublic friend void     clear(objlist& s);
    ptpublic friend void     ins(objlist& s, int i, unknown* iobj);
    ptpublic friend int      add(objlist& s, unknown* iobj);
    ptpublic friend void     put(objlist& s, int i, unknown* iobj);
    ptpublic friend unknown* get(const objlist& s, int i);
    ptpublic friend int      push(objlist& s, unknown* obj);
    ptpublic friend unknown* top(const objlist& s);
    ptpublic friend unknown* pop(objlist& s);
    ptpublic friend void     del(objlist& s, int i);
    ptpublic friend int      indexof(const objlist& s, unknown* iobj);

    unknown* operator[] (int i) const;
};


inline int length(const objlist& s)                { return s.getcount(); }
inline void setlength(objlist& s, int newcount)    { s.setcount(newcount); }
inline void pack(objlist& s)                       { s.setcapacity(s.count); }
inline void clear(objlist& s)                      { s.clear(); }
inline unknown* top(const objlist& s)              { return get(s, length(s) - 1); }
inline int push(objlist& s, unknown* obj)          { return add(s, obj); }

#ifndef CHECK_BOUNDS
inline unknown* get(const objlist& s, int i)       { return s.getitem(i); }
#endif

inline unknown* objlist::operator[] (int i) const  { return PTYPES_NAMESPACE::get(*this, i); }


// a fully inlined objlist template for use with any class
// derived from unknown. provided for extra typechecking and
// to get rid of typecasts. 'unknown* t = obj' assignments in
// these functions are to check whether T is derived from
// unknown. similarly, template lists are provided for strlist
// and strmap.

template <class T> class tobjlist: public objlist
{
public:
    tobjlist(): objlist()  {}
    tobjlist(bool iownobjects): objlist(iownobjects)  {}

    friend inline void ins(tobjlist& s, int i, T* obj)
        { unknown* t = obj; PTYPES_NAMESPACE::ins(s, i, t); }

    friend inline int add(tobjlist& s, T* obj)
        { unknown* t = obj; return PTYPES_NAMESPACE::add(s, t); }

    friend inline void put(tobjlist& s, int i, T* obj)
        { unknown* t = obj; PTYPES_NAMESPACE::put(s, i, t); }

    friend inline int indexof(const tobjlist& s, T* obj)
        { unknown* t = obj; return PTYPES_NAMESPACE::indexof(s, t); }

    friend inline int push(tobjlist& s, T* obj)
        { unknown* t = obj; return PTYPES_NAMESPACE::push(s, t); }

    friend inline T* pop(tobjlist& s)
        { return (T*)PTYPES_NAMESPACE::pop((objlist&)s); }

    friend inline T* top(const tobjlist& s)
        { return (T*)PTYPES_NAMESPACE::get(s, length(s) - 1); }

    friend inline T* get(const tobjlist& s, int i)
        { return (T*)PTYPES_NAMESPACE::get((const objlist&)s, i); }

    T* operator[] (int i) const
        { return (T*)PTYPES_NAMESPACE::get(*this, i); }
};


// -------------------------------------------------------------------- //
// ---  string list --------------------------------------------------- //
// -------------------------------------------------------------------- //


struct _stritem 
{
    string str;
    unknown* obj;
};

typedef _stritem* pstritem;
const int _stritemsize = sizeof(_stritem);

enum slflags 
{
    SL_SORTED = 0x0001,
    SL_DUPLICATES = 0x0002,
    SL_CASESENS = 0x0004,
    SL_OWNOBJECTS = 0x0008
};


class ptpublic strlist: public unknown 
{
protected:
    _stritem*   list;
    int         count;
    int         capacity;
    slflags     flags;

    void idxerror() const;
    void sortederror() const;
    void notsortederror() const;
    void duperror() const;

    void setcapacity(int newcap);
    int  getcount() const { return count; }
    void grow();
    void clear();
    virtual int compare(const string& s1, const string& s2) const;
    bool search(const string& s, int& index) const;
    void insitem(int i, const string& istr, unknown* iobj);
    void putstr(int i, const string& istr);
    void putobj(int i, unknown* iobj);
    void delitem(int i);
    const string& getstr(int i) const  { return list[i].str; }
    unknown* getobj(int i) const  { return list[i].obj; }

public:
    strlist();
    strlist(slflags iflags);
    virtual ~strlist();

    void checkidx(int i) const                  { if (i < 0 || i >= count) idxerror(); }
    bool get_sorted() const                     { return (SL_SORTED & flags) != 0; }
    bool get_duplicates() const                 { return (SL_DUPLICATES & flags) != 0; }
    bool get_casesens() const                   { return (SL_CASESENS & flags) != 0; }
    bool get_ownobjects() const                 { return (SL_OWNOBJECTS & flags) != 0; }
    int  get_capacity() const                   { return capacity; }

    ptpublic friend int      length(const strlist& s);
    ptpublic friend void     clear(strlist& s);
    ptpublic friend void     pack(strlist& s);
    ptpublic friend bool     search(const strlist& s, const string& key, int& index);
    ptpublic friend void     ins(strlist& s, int i, const string& key, unknown* obj);
    ptpublic friend int      add(strlist& s, const string& key, unknown* obj);
    ptpublic friend void     put(strlist& s, int i, const string& key, unknown* obj);
    ptpublic friend void     put(strlist& s, int i, unknown* obj);
    ptpublic friend unknown* get(const strlist& s, int i);
    ptpublic friend const string& getstr(const strlist& s, int i);
    ptpublic friend void     del(strlist& s, int i);
    ptpublic friend int      find(const strlist& s, const string& key);
    ptpublic friend int      indexof(const strlist& s, unknown* obj);
    ptpublic friend string   valueof(const strlist& s, const char* key);

    unknown* operator[] (int i) const;
};


inline int length(const strlist& s)                   { return s.getcount(); }
inline void clear(strlist& s)                         { s.clear(); }
inline void pack(strlist& s)                          { s.setcapacity(s.count); }

#ifndef CHECK_BOUNDS
inline const string& getstr(const strlist& s, int i)  { return s.getstr(i); }
inline unknown* get(const strlist& s, int i)          { return s.getobj(i); }
#endif

inline unknown* strlist::operator[] (int i) const     { return PTYPES_NAMESPACE::get(*this, i); }


template <class T> class tstrlist: public strlist
{
public:
    tstrlist(): strlist() {}
    tstrlist(slflags iflags): strlist(iflags)  {}

    friend inline void ins(tstrlist& s, int i, const string& str, T* obj)
        { unknown* t = obj; PTYPES_NAMESPACE::ins(s, i, str, t); }

    friend inline int  add(tstrlist& s, const string& str, T* obj)
        { unknown* t = obj; return PTYPES_NAMESPACE::add(s, str, t); }

    friend inline void put(tstrlist& s, int i, const string& str, T* obj)
        { unknown* t = obj; PTYPES_NAMESPACE::put(s, i, str, t); }

    friend inline void put(tstrlist& s, int i, T* obj)
        { unknown* t = obj; PTYPES_NAMESPACE::put(s, i, t); }

    friend inline int indexof(const tstrlist& s, T* obj)
        { unknown* t = obj; return PTYPES_NAMESPACE::indexof(s, t); }

    friend inline T* get(const tstrlist& s, int i)
        { return (T*)PTYPES_NAMESPACE::get((const strlist&)s, i); }

    T* operator[] (int i) const
        { return (T*)PTYPES_NAMESPACE::get(*this, i); }
};



// -------------------------------------------------------------------- //
// ---  string map ---------------------------------------------------- //
// -------------------------------------------------------------------- //


class ptpublic strmap: protected strlist 
{
protected:
    unknown* getobj(const string& key) const;
    unknown* getobj(const char* key) const;
    void putobj(const string& key, unknown* obj);

public:
    strmap();
    strmap(slflags iflags);
    virtual ~strmap();

    ptpublic friend int      length(const strmap& s);
    ptpublic friend void     clear(strmap& m);
    ptpublic friend void     put(strmap& m, const string& key, unknown* obj);
    ptpublic friend unknown* get(const strmap& m, const string& key);
    ptpublic friend void     del(strmap& m, const string& key);

    unknown* operator[] (const string& key);
};


inline int length(const strmap& s)                             { return s.getcount(); }
inline void clear(strmap& m)                                   { m.clear(); }
inline unknown* get(const strmap& m, const string& key)        { return m.getobj(key); }
inline void put(strmap& m, const string& key, unknown* obj)    { m.putobj(key, obj); }
inline void del(strmap& m, const string& key)                  { m.putobj(key, nil); }
inline unknown* strmap::operator[] (const string& key)         { return getobj(key); }


template <class T> class tstrmap: public strmap
{
public:
    tstrmap(): strmap()  {}
    tstrmap(slflags iflags): strmap(iflags)  {}

    friend inline T* get(const tstrmap& m, const string& str)
        { return (T*)PTYPES_NAMESPACE::get((const strmap&)m, str); }

    friend inline void put(tstrmap& m, const string& str, T* obj)
        { unknown* t = obj; PTYPES_NAMESPACE::put(m, str, t); }

    T* operator[] (const string& str)
        { return (T*)PTYPES_NAMESPACE::get(*this, str); }
};


// -------------------------------------------------------------------- //
// ---  text map ------------------------------------------------------ //
// -------------------------------------------------------------------- //

// undocumented

class ptpublic textmap: protected strlist 
{
protected:
    const string& getvalue(const string& name) const;
    const string& getvalue(const char* name) const;
    void putvalue(const string& name, const string& value);
    void delitem(int i);
    void clear();

public:
    textmap();
    virtual ~textmap();

    ptpublic friend int   length(const textmap& m);
    ptpublic friend void  clear(textmap& m);
    ptpublic friend const string& get(const textmap& m, const string& name);
    ptpublic friend void  put(textmap& m, const string& name, const string& value);
    ptpublic friend void  del(textmap& m, const string& name);

    const string& operator[] (const string& name);
};


inline int length(const textmap& m)                                   { return m.getcount(); }
inline void clear(textmap& m)                                         { m.clear(); }
inline const string& get(const textmap& m, const string& name)        { return m.getvalue(name); }
inline void put(textmap& m, const string& name, const string& value)  { m.putvalue(name, value); }
inline void del(textmap& m, const string& name)                       { m.putvalue(name, nullstring); }
inline const string& textmap::operator[] (const string& name)         { return getvalue(name); }


// -------------------------------------------------------------------- //
// ---  variant class ------------------------------------------------- //
// -------------------------------------------------------------------- //


enum {
    VAR_NULL,
    VAR_INT,
    VAR_BOOL,
    VAR_FLOAT,
    VAR_STRING,
    VAR_ARRAY,
    VAR_OBJECT,

    VAR_COMPOUND = VAR_STRING
};


class ptpublic variant
{
    friend class string;
    friend class _varray;

protected:
    int tag;            // VAR_XXX
    union {
        large      i;   // 64-bit int value
        bool       b;   // bool value
        double     f;   // double value
        char*      s;   // string object; can't declare as string because of the union
        _varray*   a;   // pointer to a reference-counted strlist object
        component* o;   // pointer to a reference-counted component object (or derivative)
    } value;            // we need this name to be able to copy the entire union in some situations

    void initialize()                       { tag = VAR_NULL; }
    void initialize(large v)                { tag = VAR_INT; value.i = v; }
    void initialize(bool v)                 { tag = VAR_BOOL; value.b = v; }
    void initialize(double v)               { tag = VAR_FLOAT; value.f = v; }
    void initialize(const char* v)          { tag = VAR_STRING; PTYPES_NAMESPACE::initialize(PTR_TO_STRING(value.s), v); }
    void initialize(const string& v)        { tag = VAR_STRING; PTYPES_NAMESPACE::initialize(PTR_TO_STRING(value.s), v); }
    void initialize(_varray* a);
    void initialize(component* o);
    void initialize(const variant& v);
    void finalize();

    void assign(large);
    void assign(bool);
    void assign(double);
    void assign(const char*);
    void assign(const string&);
    void assign(_varray*);
    void assign(component*);
    void assign(const variant&);

    bool equal(const variant& v) const;

    variant(_varray* a)                     { initialize(a); }

public:
    // construction
    variant()                               { initialize(); }
    variant(int v)                          { initialize(large(v)); }
    variant(unsigned int v)                 { initialize(large(v)); }
    variant(large v)                        { initialize(v); }
    variant(bool v)                         { initialize(v); }
    variant(double v)                       { initialize(v); }
    variant(const char* v)                  { initialize(v); }
    variant(const string& v)                { initialize(v); }
    variant(component* v)                   { initialize(v); }
    variant(const variant& v)               { initialize(v); }
    ~variant()                              { finalize(); }

    // assignment
    variant& operator= (int v)              { assign(large(v)); return *this; }
    variant& operator= (unsigned int v)     { assign(large(v)); return *this; }
    variant& operator= (large v)            { assign(v); return *this; }
    variant& operator= (bool v)             { assign(v); return *this; }
    variant& operator= (double v)           { assign(v); return *this; }
    variant& operator= (const char* v)      { assign(v); return *this; }
    variant& operator= (const string& v)    { assign(v); return *this; }
    variant& operator= (component* v)       { assign(v); return *this; }
    variant& operator= (const variant& v)   { assign(v); return *this; }

    // typecast
    operator int() const;
    operator unsigned int() const;
    operator long() const;
    operator unsigned long() const;
    operator large() const;
    operator bool() const;
    operator double() const;
    operator string() const;
    operator component*() const;

    // comparison
    bool operator== (const variant& v) const  { return equal(v); }
    bool operator!= (const variant& v) const  { return !equal(v); }

    // typification
    ptpublic friend void clear(variant&);
    ptpublic friend int  vartype(const variant& v);
    ptpublic friend bool isnull(const variant& v);
    ptpublic friend bool isint(const variant& v);
    ptpublic friend bool isbool(const variant& v);
    ptpublic friend bool isfloat(const variant& v);
    ptpublic friend bool isstring(const variant& v);
    ptpublic friend bool isarray(const variant& v);
    ptpublic friend bool isobject(const variant& v);
    ptpublic friend bool iscompound(const variant& v);

    // array manipulation
    ptpublic friend void aclear(variant&);
    ptpublic friend variant aclone(const variant&);
    ptpublic friend const variant& get(const variant&, const string& key);
    ptpublic friend const variant& get(const variant&, large key);
    ptpublic friend void put(variant&, const string& key, const variant& item);
    ptpublic friend void put(variant&, large key, const variant& item);
    ptpublic friend void del(variant&, const string& key);
    ptpublic friend void del(variant&, large key);

    // keyless arrays (experimental)
    ptpublic friend int  alength(const variant&);
    ptpublic friend bool anext(const variant& a, int&, variant& item);
    ptpublic friend bool anext(const variant& a, int&, variant& item, string& key);
    ptpublic friend int  aadd(variant&, const variant& item);
    ptpublic friend void aput(variant&, int index, const variant& item);
    ptpublic friend void ains(variant&, int index, const variant& item);
    ptpublic friend void adel(variant&, int index);
    ptpublic friend const variant& aget(const variant&, int index);
    ptpublic friend string akey(const variant&, int index);

    const variant& operator[](const char* key) const    { return get(*this, string(key)); }
    const variant& operator[](const string& key) const  { return get(*this, key); }
    const variant& operator[](large key) const          { return get(*this, key); }

    // 'manual' initialization/finalization, undocumented. use with care!
    ptpublic friend void initialize(variant& v);
    ptpublic friend void initialize(variant& v, large i);
    ptpublic friend void initialize(variant& v, int i);
    ptpublic friend void initialize(variant& v, unsigned int i);
    ptpublic friend void initialize(variant& v, bool i);
    ptpublic friend void initialize(variant& v, double i);
    ptpublic friend void initialize(variant& v, const char* i);
    ptpublic friend void initialize(variant& v, const string& i);
    ptpublic friend void initialize(variant& v, component* i);
    ptpublic friend void initialize(variant& v, const variant& i);
    ptpublic friend void finalize(variant& v);
};


typedef variant* pvariant;


inline int  vartype(const variant& v)       { return v.tag; }
inline bool isnull(const variant& v)        { return v.tag == VAR_NULL; }
inline bool isint(const variant& v)         { return v.tag == VAR_INT; }
inline bool isbool(const variant& v)        { return v.tag == VAR_BOOL; }
inline bool isfloat(const variant& v)       { return v.tag == VAR_FLOAT; }
inline bool isstring(const variant& v)      { return v.tag == VAR_STRING; }
inline bool isarray(const variant& v)       { return v.tag == VAR_ARRAY; }
inline bool isobject(const variant& v)      { return v.tag == VAR_OBJECT; }
inline bool iscompound(const variant& v)    { return v.tag >= VAR_COMPOUND; }

inline void initialize(variant& v)                   { v.initialize(); }
inline void initialize(variant& v, large i)          { v.initialize(i); }
inline void initialize(variant& v, int i)            { v.initialize(large(i)); }
inline void initialize(variant& v, unsigned int i)   { v.initialize(large(i)); }
inline void initialize(variant& v, bool i)           { v.initialize(i); }
inline void initialize(variant& v, double i)         { v.initialize(i); }
inline void initialize(variant& v, const char* i)    { v.initialize(i); }
inline void initialize(variant& v, const string& i)  { v.initialize(i); }
inline void initialize(variant& v, component* i)     { v.initialize(i); }
inline void initialize(variant& v, const variant& i) { v.initialize(i); }
inline void finalize(variant& v)                     { if (v.tag >= VAR_COMPOUND) v.finalize(); }


ptpublic extern const variant nullvar;


//
// variant exception class
//


class ptpublic evariant: public exceptobj
{
protected:
public:
    evariant(const char* msg): exceptobj(msg)  {}
    evariant(const string& msg): exceptobj(msg)  {}
    virtual ~evariant();
};


//
// internal class, used as a variant array element
//


class ptpublic varobject: public unknown
{
public:
    variant var;
    varobject()                   : var()   {}
    varobject(const variant& v)   : var(v)  {}
    virtual ~varobject();
};

typedef varobject* pvarobject;


//
// internal variant array class
//


class ptpublic _varray: protected strlist
{
protected:
    friend class variant;

    int refcount;

    ptpublic friend void clear(_varray& v);
    ptpublic friend int length(_varray& v);

public:
    _varray();
    _varray(const _varray&);
    virtual ~_varray();

    const variant& getvar(const string&) const;
    const variant& getvar(int index) const;
    string getkey(int index) const;
    void putvar(const string&, const variant&);
    void putvar(int index, const variant&);
    void insvar(int index, const variant&);
    int addvar(const variant&);
    void delvar(const string&);
    void delvar(int index);

    ptpublic friend bool anext(const variant& a, int&, variant& item);
    ptpublic friend bool anext(const variant& a, int&, variant& item, string& key);
};


inline void clear(_varray& v)   { v.clear(); }
inline int length(_varray& v)   { return v.count; }



#ifdef _MSC_VER
#pragma pack(pop)
#endif


PTYPES_END

#endif // __PTYPES_H__
