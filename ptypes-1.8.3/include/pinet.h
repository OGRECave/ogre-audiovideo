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

#ifndef __PINET_H__
#define __PINET_H__

#ifndef __PPORT_H__
#include "pport.h"
#endif

#ifndef __PTYPES_H__
#include "ptypes.h"
#endif

#ifndef __PSTREAMS_H__
#include "pstreams.h"
#endif


#ifdef WIN32
#  include <winsock2.h>
#else
#  include <netdb.h>       // for socklen_t
#  include <sys/types.h>
#  include <sys/socket.h>
#endif


PTYPES_BEGIN


#ifdef _MSC_VER
#pragma pack(push, 4)
#endif


//
// BSD-compatible socket error codes for Win32
//

#if defined(WSAENOTSOCK) && !defined(ENOTSOCK)

#define EWOULDBLOCK             WSAEWOULDBLOCK
#define EINPROGRESS             WSAEINPROGRESS
#define EALREADY                WSAEALREADY
#define ENOTSOCK                WSAENOTSOCK
#define EDESTADDRREQ            WSAEDESTADDRREQ
#define EMSGSIZE                WSAEMSGSIZE
#define EPROTOTYPE              WSAEPROTOTYPE
#define ENOPROTOOPT             WSAENOPROTOOPT
#define EPROTONOSUPPORT         WSAEPROTONOSUPPORT
#define ESOCKTNOSUPPORT         WSAESOCKTNOSUPPORT
#define EOPNOTSUPP              WSAEOPNOTSUPP
#define EPFNOSUPPORT            WSAEPFNOSUPPORT
#define EAFNOSUPPORT            WSAEAFNOSUPPORT
#define EADDRINUSE              WSAEADDRINUSE
#define EADDRNOTAVAIL           WSAEADDRNOTAVAIL
#define ENETDOWN                WSAENETDOWN
#define ENETUNREACH             WSAENETUNREACH
#define ENETRESET               WSAENETRESET
#define ECONNABORTED            WSAECONNABORTED
#define ECONNRESET              WSAECONNRESET
#define ENOBUFS                 WSAENOBUFS
#define EISCONN                 WSAEISCONN
#define ENOTCONN                WSAENOTCONN
#define ESHUTDOWN               WSAESHUTDOWN
#define ETOOMANYREFS            WSAETOOMANYREFS
#define ETIMEDOUT               WSAETIMEDOUT
#define ECONNREFUSED            WSAECONNREFUSED
#define ELOOP                   WSAELOOP
// #define ENAMETOOLONG            WSAENAMETOOLONG
#define EHOSTDOWN               WSAEHOSTDOWN
#define EHOSTUNREACH            WSAEHOSTUNREACH
// #define ENOTEMPTY               WSAENOTEMPTY
#define EPROCLIM                WSAEPROCLIM
#define EUSERS                  WSAEUSERS
#define EDQUOT                  WSAEDQUOT
#define ESTALE                  WSAESTALE
#define EREMOTE                 WSAEREMOTE

// NOTE: these are not errno constants in UNIX!
#define HOST_NOT_FOUND          WSAHOST_NOT_FOUND
#define TRY_AGAIN               WSATRY_AGAIN
#define NO_RECOVERY             WSANO_RECOVERY
#define NO_DATA                 WSANO_DATA

#endif


// shutdown() constants

#if defined(SD_RECEIVE) && !defined(SHUT_RD)
#  define SHUT_RD       SD_RECEIVE
#  define SHUT_WR       SD_SEND
#  define SHUT_RDWR     SD_BOTH
#endif


// max backlog value for listen()

#ifndef SOMAXCONN
#  define SOMAXCONN -1
#endif

typedef char* sockval_t;

#ifndef WIN32
#  define closesocket close
#endif


#if defined(__DARWIN__) || defined(WIN32)
  typedef int psocklen;
#else
  typedef socklen_t psocklen;
#endif


// -------------------------------------------------------------------- //
// ---  IP address class and DNS utilities ---------------------------- //
// -------------------------------------------------------------------- //

//
// IP address
//

struct ptpublic ipaddress
{
    union
    {
        uchar   data[4];
        ulong   ldata;
    };
    ipaddress()                             {}
    ipaddress(ulong a)                      { ldata = a; }
    ipaddress(int a, int b, int c, int d);
    ipaddress& operator= (ulong a)          { ldata = a; return *this; }
    uchar& operator [] (int i)              { return data[i]; }
    operator ulong() const                  { return ldata; }
};


ptpublic extern ipaddress ipnone;
ptpublic extern ipaddress ipany;
ptpublic extern ipaddress ipbcast;


//
// IP peer info: host name, IP and the port name
// used internally in ipstream and ipmessage
//


class ptpublic ippeerinfo
{
protected:
    ipaddress ip;         // target IP
    string    host;       // target host name; either IP or hostname must be specified
    int       port;       // target port number

    void      notfound(); // throws a (estream*) exception

    friend ptpublic bool psockname(int, ippeerinfo&);
public:
    ippeerinfo();
    ippeerinfo(ipaddress iip, const string& ihost, int iport);

    ipaddress get_ip();     // resolves the host name if necessary (only once)
    string    get_host();   // performs reverse-lookup if necessary (only once)
    int       get_port()    { return port; }
    void      clear();
    string    asstring(bool showport) const;
};


ptpublic string    iptostring(ipaddress ip);
ptpublic ipaddress phostbyname(const char* name);
ptpublic string    phostbyaddr(ipaddress ip);
ptpublic string    phostcname(const char* name);

// internal utilities
ptpublic int usockerrno();
ptpublic const char* usockerrmsg(int code);
ptpublic bool psockwait(int handle, int timeout);
ptpublic bool psockname(int handle, ippeerinfo&);


// -------------------------------------------------------------------- //
// ---  TCP socket classes -------------------------------------------- //
// -------------------------------------------------------------------- //


// additional IO status codes

const int IO_RESOLVING  = 10;
const int IO_RESOLVED   = 11;
const int IO_CONNECTING = 20;
const int IO_CONNECTED  = 21;


//
// ipstream
//

class ptpublic ipstream: public fdxstm, public ippeerinfo
{
    friend class ipstmserver;

protected:
    int svsocket;   // server socket descriptor, used internally by ipstmserver

#ifdef WIN32
    // A citation from MSDN: "While nothing in the Windows Sockets prevents an 
    // implementation from using regular file handles to identify sockets, nothing requires 
    // it either". Nothing requires us to spend less time writing programs?
    virtual int dorawread(char* buf, int count);
    virtual int dorawwrite(const char* buf, int count);
#endif

    virtual int  uerrno();
    virtual const char* uerrmsg(int code);
    virtual void doopen();
    virtual int  doseek(int newpos, ioseekmode mode);
    virtual void doclose();
    void closehandle();

public:
    ipstream();
    ipstream(ipaddress ip, int port);
    ipstream(const char* host, int port);
    ipstream(const string& host, int port);
    virtual ~ipstream();

    virtual string get_streamname();

    bool      waitfor(int timeout);
    ipaddress get_myip();
    int       get_myport();
    void      set_ip(ipaddress);
    void      set_host(const string&);
    void      set_host(const char*);
    void      set_port(int);
};


//
// common internal interfaces for ipstmserver and ipmsgserver
//

class ipbindinfo: public unknown, public ippeerinfo
{
public:
    int handle;

    ipbindinfo(ipaddress iip, const string& ihost, int iport);
    virtual ~ipbindinfo();
};


class ptpublic ipsvbase: public unknown
{
protected:
    int     socktype;
    bool    active;
    objlist addrlist;       // list of local socket addresses to bind to (ipbindinfo*)

    void error(ippeerinfo& peer, int code, const char* defmsg);
    bool dopoll(int* i, int timeout);
    void setupfds(void* set, int i);
    virtual void open();
    virtual void close();
    virtual void dobind(ipbindinfo*) = 0;

public:
    ipsvbase(int isocktype);
    virtual ~ipsvbase();

    void bind(ipaddress ip, int port);
    void bindall(int port);

    int get_addrcount()                  { return length(addrlist); }
    const ipbindinfo& get_addr(int i)    { return *(ipbindinfo*)addrlist[i]; }
    void clear();
};


//
// ipstmserver
//

class ptpublic ipstmserver: public ipsvbase
{
protected:
    virtual void dobind(ipbindinfo*);

public:
    ipstmserver();
    virtual ~ipstmserver();

    bool poll(int i = -1, int timeout = 0);
    bool serve(ipstream& client, int i = -1, int timeout = -1);
};


typedef ipstream ipsocket;      // pre-1.7 compatibility aliases
typedef ipstmserver ipserver;


// -------------------------------------------------------------------- //
// ---  UDP socket classes -------------------------------------------- //
// -------------------------------------------------------------------- //


//
// ipmessage
//

class ptpublic ipmessage: public unknown, public ippeerinfo
{
protected:
    int handle;

    void error(int code, const char* msg);
    void open();
    void close();

public:
    ipmessage();
    ipmessage(ipaddress ip, int port);
    ipmessage(const char* host, int port);
    ipmessage(const string& host, int port);
    virtual ~ipmessage();

    void set_ip(ipaddress iip);
    void set_host(const string&);
    void set_host(const char*);
    void set_port(int);
    ipaddress get_myip();
    int get_myport();
    int get_handle()                            { return handle; }

    bool   waitfor(int timeout);
    int    receive(char* buf, int count);
    string receive(int max);
    void   send(const char* buf, int count);
    void   send(const string& s)                { send(s, length(s)); }
};


//
// ipmsgserver
//

class ptpublic ipmsgserver: public ipsvbase, public ippeerinfo
{
protected:
    int handle;

    virtual void close();
    virtual void dobind(ipbindinfo*);

public:
    ipmsgserver();
    virtual ~ipmsgserver();

    int get_handle()                            { return handle; }

    bool   poll(int i = -1, int timeout = 0);
    int    receive(char* buf, int count);
    string receive(int max);
    void   send(const char* buf, int count);
    void   send(const string& s)                { send(s, length(s)); }
    void   sendto(const char* buf, int count, ipaddress ip, int port);
    void   sendto(const string& s, ipaddress ip, int port)
                                                { sendto(s, length(s), ip, port); }
};


#ifdef _MSC_VER
#pragma pack(pop)
#endif


PTYPES_END


#endif // __PINET_H__

