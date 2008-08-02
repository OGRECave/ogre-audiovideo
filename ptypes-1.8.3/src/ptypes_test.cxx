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

#include "pport.h"
#include "ptypes.h"
#include "pstreams.h"
#include "pinet.h"
#include "ptime.h"


#ifndef PTYPES_ST
#include "pasync.h"
#endif


USING_PTYPES


void showstr(const char* shouldbe, const char* is) 
{
    pout.putf("[%s] %s\n", shouldbe, is);
}


void showstr(const char* shouldbe, const string& is) 
{
    showstr(shouldbe, pconst(is));
}


void showhex(const char* shouldbe, const char* is, int islen) 
{
    string s;
    int i;
    for (i = 0; i < islen; i++)
        s += itobase((unsigned char)is[i], 16, 2);
    s = lowercase(s);
    showstr(shouldbe, s);
}


void showint(int shouldbe, int is) 
{
    pout.putf("[%d] %d\n", shouldbe, is);
}


void showint(large shouldbe, large is) 
{
    pout.putf("[%lld] %lld\n", shouldbe, is);
}


void showchar(char shouldbe, char is) 
{
    pout.putf("[%c] %c\n", shouldbe, is);
}


//
// string test
//

void string_test1() 
{
    pout.put("\n--- STRING CLASS\n");

    static char strbuf[10] = "STRING";

    char c = 'A';

    string s1 = "string";
    s1 = s1;
    s1 += s1;
    del(s1, 6, 6);
    string s2 = s1;
    string s3;
    string s4(strbuf, strlen(strbuf));
    string s5 = 'A';
    string s6 = c;

    showstr("string", s1);
    showstr(s1, s2);
    showstr("", s3);
    showstr("STRING", s4);
    showint(6, length(s4));
    showint(2, refcount(s1));
    clear(s2);
    showint(1, refcount(s1));
    s2 = s1;
    unique(s1);
    showint(1, refcount(s1));
    setlength(s1, 64);
    string s7 = s1;
    setlength(s1, 3);
    showstr("str", s1);
    showstr("strING", s1 += copy(s4, 3, 3));
    del(s1, 3, 3);
    showstr("str", s1);
    ins("ing", s1, 0);
    showstr("ingstr", s1);
    s2 = "str" + s1 + "ing";
    showstr("stringstring", s2);
    s3 = s2;
    s3 = "strungstrung";
    s3 = s2;
    s3 = "strung";

    s2 = "str" + s1;
    s2 = s1 + "str";
    s2 += "str";

    s2 = 's' + s1;
    s2 = s1 + 's';
    s2 += 's';
    
    s2 = c + s1;
    s2 = s1 + c;
    s2 += c;

    s2 = 'a';
    s2 = c;
}


void string_test2() 
{
    pout.put("\n--- STRING CLASS\n");

    string s1 = "ingstr";
    string s2 = "stringstring";
    string s4 = "STRING";

    showint(1, pos('t', s2));
    showint(2, pos("ri", s2));
    showint(3, pos(s1, s2));
    showint(1, contains("tr", s2, 1));
    showint(0, contains("tr", s2, 2));
    showint(1, s4 == "STRING");
    showchar('R', s4[2]);

    showstr("123456789", itostring(123456789));
    showstr("123", itostring(char(123)));
    showstr("-000123", itostring(-123, 10, 7, '0'));
    showstr("0ABCDE", itostring(0xabcde, 16, 6));
    showstr("-9223372036854775808", itostring(LARGE_MIN));
    showstr("18446744073709551615", itostring(ULARGE_MAX));

    showint(1234, (int)stringtoi("1234"));
    showint(large(0x15AF), stringtoue("15AF", 16));
    showint(LARGE_MAX, stringtoue("5zzzzzzzzzz", 64));

    try
    {
        // out of range by 1
        stringtoue("18446744073709551616", 10);
        fatal(0xb0, "Conversion overflow not detected");
    }
    catch (econv* e)
    {
        showstr("Out of range: ...", e->get_message());
        delete e;
    }

    showint(large(123), stringtoie("123"));
    showint(large(-123), stringtoie("-123"));
    showint(LARGE_MIN, stringtoie("-9223372036854775808"));
    showint(LARGE_MAX, stringtoie("9223372036854775807"));

    try
    {
        // out of range by 1
        stringtoie("9223372036854775808");
        fatal(0xb0, "Conversion overflow not detected");
    }
    catch (econv* e)
    {
        showstr("Out of range: ...", e->get_message());
        delete e;
    }

    showstr("abcabc", lowercase(s1 = "aBCAbc"));
    showstr("abcabc", lowercase(s1));
}


void string_benchmarks()
{
    pout.put("\n--- STRING BENCHMARKS\n");

    int i;
    string s1 = "string";
    string s2;

    // for the first time we run the test to let the VM settle down and stop swapping
    for (i = 0; i < 156000; i++)
        s2 += s1;

    // here is the actual test
    clear(s2);
    datetime start = now();
    for (i = 0; i < 156000; i++)
        s2 += s1;
    datetime diff = now() - start;

    pout.putf("Performance index compared to WinNT on P3/800MHz:\n%.3f\n", diff / 1000.0);
}


//
// cset test
//

void cset_test() 
{
    pout.put("\n--- CSET CLASS\n");

    cset s1 = "~09~0a~0d~F0 A-Z~~";
    cset s2 = "A-F";
    char c = 'B';

    showstr("~09~0a~0d A-Z~~~f0", asstring(s1));
    s1 -= char(0xf0);
    s1 -= cset("~00-~20");
    showstr("A-Z~~", asstring(s1));
    s1 -= s2;
    showstr("G-Z~~", asstring(s1));
    s1 += s2;
    s1 += ' ';
    showstr(" A-Z~~", asstring(s1));
    showint(1, s2 == cset("A-F"));
    showint(1, s2 <= s1);
    s1 -= 'A';
    s1 -= c;
    showint(0, s2 <= s1);
    s1 = s1 + char(0xf1);
    showint(1, char(0xf1) & s1);
    showint(0, char(0xf2) & s1);
}


//
// objlist
//


class known: public unknown {
public:
    int value;
    known(int ivalue): unknown(), value(ivalue) {}
    virtual ~known()  {}
};

typedef tobjlist<known> knownlist;


string ol_asstring(const knownlist& s) {
    string ret = "{";
    for (int i = 0; i < length(s); i++) {
        if (i > 0)
            concat(ret, ", ");
        ret += itostring(s[i]->value);
    }
    concat(ret, '}');
    return ret;
}


void objlist_test() {
    pout.put("\n--- OBJLIST CLASS\n");

    knownlist s1(true);
    known* obj;

    add(s1, new known(10));
    ins(s1, 0, new known(5));
    ins(s1, 2, obj = new known(20));
    add(s1, new known(30));
    add(s1, new known(40));
    del(s1, 4);
    del(s1, 1);

    get(s1, 0);
    showint(3, length(s1));
    showint(1, indexof(s1, obj));
    showstr("{5, 20, 30}", ol_asstring(s1));

    clear(s1);
    showstr("{}", ol_asstring(s1));
}



//
// strlist
//


typedef tstrlist<known> knownstrlist;


string sl_asstring(const knownstrlist& s) {
    string ret = "{";
    for (int i = 0; i < length(s); i++) {
        if (i > 0)
            concat(ret, ", ");
        ret += getstr(s, i) + ":" + itostring(s[i]->value);
    }
    concat(ret, '}');
    return ret;
}


void strlist_test() {
    pout.put("\n--- STRLIST CLASS\n");

    knownstrlist s1(SL_OWNOBJECTS);
    known* obj;

    add(s1, "ten", new known(10));
    ins(s1, 0, "five", new known(5));
    ins(s1, 2, "twenty", obj = new known(20));
    add(s1, "thirty", new known(30));
    add(s1, "forty", new known(40));
    del(s1, 4);
    del(s1, 1);

    showint(3, length(s1));
    showint(1, indexof(s1, obj));
    showint(2, find(s1, "thirty"));
    showint(2, find(s1, "THIRTY"));
    showint(-1, find(s1, "forty"));

    showstr("{five:5, twenty:20, thirty:30}", sl_asstring(s1));

    knownstrlist s2(slflags(SL_OWNOBJECTS | SL_SORTED | SL_CASESENS));
    add(s2, "five", new known(5));
    add(s2, "ten", new known(10));
    add(s2, "twenty", new known(20));
    add(s2, "thirty", new known(30));
    add(s2, "forty", new known(40));

    showint(5, length(s2));
    showint(3, find(s2, "thirty"));
    showint(-1, find(s2, "THIRTY"));
    showint(-1, find(s2, "hovik"));

    showstr("{five:5, forty:40, ten:10, thirty:30, twenty:20}", sl_asstring(s2));

    clear(s2);
    showstr("{}", sl_asstring(s2));

    strlist s3(slflags(SL_OWNOBJECTS | SL_SORTED | SL_DUPLICATES));

    add(s3, "a", nil);
    add(s3, "b", nil);
    add(s3, "b", nil);
    add(s3, "b", nil);
    add(s3, "b", nil);
    add(s3, "b", nil);
    add(s3, "b", nil);
    add(s3, "c", nil);

    showint(1, find(s3, "b"));
}


void strmap_test() {
    pout.put("\n--- STRMAP CLASS\n");

    tstrmap<known> s(SL_OWNOBJECTS);

    put(s, "five", new known(5));
    put(s, "ten", new known(10));
    put(s, "twenty", new known(20));
    put(s, "thirty", new known(30));
    put(s, "forty", new known(40));

    showint(20, get(s, "twenty")->value);
    showint(0, int(get(s, "hovik")));
    showint(5, length(s));
    put(s, "twenty", nil);
    showint(4, length(s));
}


//
// textmap
//


void textmap_test()
{
    pout.put("\n--- TEXTMAP CLASS\n");

    textmap c;

    put(c, "name1", "value1");
    put(c, "name2", "value2");
    put(c, "name1", "value3");
    put(c, "name2", "");
    showint(1, length(c));
    showstr("value3", get(c, "name1"));
    showstr("", get(c, "name2"));
}



//
// streams
//


char buf1[] = "This is a test.";
char buf2[] = "The file should contain readable human text. :)";

const char* fname = "stmtest.txt";

void outfile_test() 
{
    pout.put("\n--- OUTFILE CLASS\n");

    outfile f(fname, false);
    f.set_umode(0600);
    f.set_bufsize(3);

    f.open();
    f.put(buf1[0]);
    f.put(buf1[1]);
    f.put("is is a test.");
    f.puteol();
    f.close();
    
    f.set_append(true);
    f.open();
    f.write(buf2, strlen(buf2));
    f.puteol();
    f.close();

    pnull.put("This should go to nowhere I");
    pnull.put("This should go to nowhere II");
}


void infile_test() 
{
    pout.put("\n--- INFILE CLASS\n");

    infile f(fname);
    f.set_bufsize(3);

    char temp[4];

    f.open();
    pout.putf("%c", f.get());
    pout.putf("%s\n", pconst(f.line()));
    f.read(temp, sizeof temp - 1);
    temp[sizeof temp - 1] = 0;
    pout.putf("%s", temp);
    f.get();
    f.putback();
    pout.putf("%s", pconst(f.token(cset("~20-~FF"))));
    f.preview();
    if (f.get_eol()) 
    {
        f.skipline();
        pout.put("\n");
    }
    if (f.get_eof())
        pout.put("EOF\n");
//    f.error(1, "Test error message");

}


void mem_test()
{
    pout.put("\n--- OUT/IN MEMORY CLASS\n");
    
    outmemory m(12, 5);
    m.open();
    m.put("Memory");
    m.put(" c");
    m.put("lass is working");
    string s(m.get_data(), m.tell());
    showstr("Memory class", s);
}


#ifndef PTYPES_ST

//
// multithreading
//

//
// rwlock test
//

const int rw_max_threads = 30;
const int rw_max_tries = 30;
const int rw_max_delay = 20;
const int rw_rw_ratio = 5;
const bool rw_swap = false;


class rwthread: public thread
{
protected:
    virtual void execute();
public:
    rwthread(): thread(false)  {}
    virtual ~rwthread()        { waitfor(); }
};


rwlock rw;

int reader_cnt = 0;
int writer_cnt = 0;
int total_writers = 0;
int total_readers = 0;
int max_readers = 0;


int prand(int max)
{
    return rand() % max;
}


void rwthread::execute()
{

    for(int i = 0; i < rw_max_tries; i++)
    {
        psleep(prand(rw_max_delay));
        bool writer = prand(rw_rw_ratio) == 0;
        if (writer ^ rw_swap)
        {
            rw.wrlock();
            pout.put('w');
            if (pincrement(&writer_cnt) > 1)
                fatal(0xa0, "Writer: Huh?! Writers in here?");
            pincrement(&total_writers);
        }
        else
        {
            rw.rdlock();
            pout.put('.');
            int t;
            if ((t = pincrement(&reader_cnt)) > max_readers) 
                max_readers = t;
            if (writer_cnt > 0)
                fatal(0xa1, "Reader: Huh?! Writers in here?");
            pincrement(&total_readers);
        }
        psleep(prand(rw_max_delay));
        if (writer ^ rw_swap)
            pdecrement(&writer_cnt);
        else
            pdecrement(&reader_cnt);
        rw.unlock();
    }
}



void rwlock_test()
{
// #ifdef __PTYPES_RWLOCK__
    pout.put("\n--- RWLOCK\n");

    rwthread* threads[rw_max_threads];

    srand((unsigned)time(0));

    int i;
    for(i = 0; i < rw_max_threads; i++)
    {
        threads[i] = new rwthread();
        threads[i]->start();
    }
    for(i = 0; i < rw_max_threads; i++)
        delete threads[i];

    pout.putf("\nmax readers: %d\n", max_readers);
    pout.putline("do writers 'starve'?");
// #endif
}



const int MSG_DIAG = MSG_USER + 1;


//
// class diagmessage
//

class diagmessage: public message
{
protected:
    string module;
    string diagstr;
    friend class diagthread;
public:
    diagmessage(string imodule, string idiagstr)
        : message(MSG_DIAG), module(imodule),
          diagstr(idiagstr)  {}
};


//
// class diagthread
//

class diagthread: public thread, protected msgqueue
{
protected:
    virtual void execute();     // override thread::execute()
    virtual void cleanup();     // override thread::cleanup()
    virtual void msghandler(message& msg);  // override msgqueue::msghandler()
public:
    diagthread(): thread(false), msgqueue()  { }
    void postdiag(string module, string diagstr);
    void postquit();
};


void diagthread::postdiag(string module, string diagstr)
{  
    msgqueue::post(new diagmessage(module, diagstr));
}


void diagthread::postquit()
{ 
    msgqueue::post(MSG_QUIT); 
}


void diagthread::execute()
{
    // starts message queue processing; calls
    // msghandler for each message
    msgqueue::run();
}


void diagthread::cleanup()
{
}


void diagthread::msghandler(message& msg)
{
    switch (msg.id)
    {
    case MSG_DIAG:
	pout.putf("%s: %s\n", pconst(((diagmessage&)msg).module), pconst(((diagmessage&)msg).diagstr));
        break;
    default:
        defhandler(msg);
    }
}


//
// class testthread
//


class testthread: public thread
{
protected:
    diagthread* diag;
    string myname;
    virtual void execute();
    virtual void cleanup();
public:
    semaphore sem;
    tsemaphore tsem;
    testthread(diagthread* idiag)
        : thread(false), diag(idiag), myname("testthread"), sem(0), tsem(0)  {}
};


void testthread::execute()
{
    diag->postdiag(myname, "starts and enters sleep for 1 second");
    psleep(1000);
    diag->postdiag(myname, "signals the timed semaphore");
    tsem.post();
    diag->postdiag(myname, "releases the simple semaphore");
    sem.post();
    diag->postdiag(myname, "enters sleep for 1 more second");
    psleep(1000);
}


void testthread::cleanup()
{
    diag->postdiag(myname, "terminates");
}


int thread_test()
{
    pout.put("\n--- THREAD AND SEMAPHORE CLASSES\n");

    int v = 0;
    showint(0, pexchange(&v, 5));
    showint(5, pexchange(&v, 10));

    showint(11, pincrement(&v));
    showint(10, pdecrement(&v));

    diagthread diag;
    testthread thr(&diag);

    string myname = "main";

    diag.start();
    thr.start();

    diag.postdiag(myname, "waits 5 secs for the timed semaphore (actually wakes up after a second)");
    thr.tsem.wait(5000);    // must exit after 1 second instead of 5

    diag.postdiag(myname, "waits for the semaphore");
    thr.sem.wait();
    diag.postdiag(myname, "now waits for testthread to terminate");
    thr.waitfor();

    diag.postquit();
    diag.waitfor();
    return 0;
}


//
// trigger test
//


class trigthread: public thread
{
protected:
    diagthread* diag;
    string myname;
    virtual void execute();
public:
    trigger trig;
    trigthread(diagthread* idiag)
        : thread(false), diag(idiag), myname("trigthread"), trig(true, false)  {}
    virtual ~trigthread()  { waitfor(); }
};


void trigthread::execute()
{
    diag->postdiag(myname, "waits on the trigger");
    trig.wait();

    psleep(2000);
    diag->postdiag(myname, "waits on the trigger");
    trig.wait();
    
    diag->postdiag(myname, "terminates");
}


int trigger_test()
{
    pout.put("\n--- TRIGGER\n");

    diagthread diag;
    trigthread thr(&diag);

    string myname = "main";

    diag.start();
    thr.start();

    psleep(1000);
    diag.postdiag(myname, "posts the trigger");
    thr.trig.post();
    
    psleep(1000);
    diag.postdiag(myname, "posts the trigger again");
    thr.trig.post();

    thr.waitfor();
    diag.postquit();
    diag.waitfor();
    return 0;
}



#endif // PTYPES_ST


//
// md5 test
//

static md5_digest digest;

char* md5str(string data)
{
    outmd5 m;
    m.open();
    m.put(data);
    memcpy(digest, m.get_bindigest(), sizeof(md5_digest));
    return (char*)digest;
}

string cryptpw(string username, string password)
{
    outmd5 m;
    m.open();
    m.put(username);
    m.put(password);
    m.close();
    return m.get_digest();
}

void md5_test()
{
    pout.put("\n--- MD5 OUTPUT STREAM\n");
    // MD5 test suite from RFC1321
    showhex("d41d8cd98f00b204e9800998ecf8427e", md5str(""), md5_digsize);
    showhex("0cc175b9c0f1b6a831c399e269772661", md5str("a"), md5_digsize);
    showhex("900150983cd24fb0d6963f7d28e17f72", md5str("abc"), md5_digsize);
    showhex("f96b697d7cb7938d525a2f31aaf161d0", md5str("message digest"), md5_digsize);
    showhex("c3fcd3d76192e4007dfb496cca67e13b", md5str("abcdefghijklmnopqrstuvwxyz"), md5_digsize);
    showhex("d174ab98d277d9f5a5611c2c9f419d9f", md5str("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"), md5_digsize);
    showhex("57edf4a22be3c955ac49da2e2107b67a", md5str("12345678901234567890123456789012345678901234567890123456789012345678901234567890"), md5_digsize);
    
    showstr("t0htL.C9vunX8SPPsJjDmk", cryptpw("hovik", "myfavoritelonglonglongpassword"));
}


//
// outstm::putf() test
//

void putf_test()
{
    pout.put("\n--- PUTF TEST\n");
    outmemory m;
    m.open();

    m.putf("%s, %c, %d, %llx", "string", 'A', 1234, large(-1));
    showstr("string, A, 1234, ffffffffffffffff", string(m.get_data(), m.tell()));

    m.open();

    m.putf(" %%, %#o, %+010d", 0765, -3);
    showstr("%, 0765, -000000003", string(m.get_data(), m.tell()));
}


//
// pinet/socket tests
//


void inet_test1()
{
    try
    {
        pout.put("\n--- INET SOCKET & UTILITIES\n");
        
        ipaddress ip(127, 0, 0, 1);
        
        // as a test target host we use the one that would never be down! :)
        string testname = "www.apache.org";
        
        ip = phostbyname(testname);
        string ips = iptostring(ip);
        pout.putf("IP address of %s: %s\n", pconst(testname), (ip == ipnone) ? "failed" : pconst(ips));
        
        if (ip != ipnone)
        {
            string hs = phostbyaddr(ip);
            pout.putf("Name of %s: %s\n", pconst(ips), pconst(hs));
        }
        
        pout.putf("Canonical name of your local www: %s\n", pconst(phostcname("www")));
        
        testname = "www.melikyan.com";
        pout.putf("\nTrying %s:80...\n", pconst(testname));
        ipstream s(testname, 80);
        
        const char* request = 
            "GET /ptypes/test.txt HTTP/1.1\r\n"
            "Accept: * /*\r\n"
            "User-Agent: ptypes_test/1.7\r\n"
            "Host: www.melikyan.com\r\n"
            "Connection: close\r\n\r\n";
        
        s.open();
        s.put(request);
        s.flush();
        
        while (!s.get_eof())
        {
            char buf[16];
            int r = s.read(buf, sizeof(buf));
            pout.write(buf, r);
        }
        pout.put("\n");
        
        s.close();
    }
    catch(estream* e)
    {
        perr.putf("Socket error: %s\n", pconst(e->get_message()));
        delete e;
    }
}


#ifndef PTYPES_ST


const int testport = 8085;


class svthread: public thread, protected ipstmserver
{
protected:
    virtual void execute();     // override thread::execute()
    virtual void cleanup();     // override thread::cleanup()
public:
    svthread(): thread(false)  {}
    virtual ~svthread();
};


svthread::~svthread()
{
    waitfor();
}


void svthread::execute()
{
    ipstream client;            // a socket object to communicate with the client

    try
    {
        bindall(testport);      // listen to all local addresses on port 8081
        serve(client);          // wait infinitely for a connection request
        
        if (client.get_active())
        {
            // read one line from the stream; note that theoretically the line can be long,
            // so calling client.line(buf, sizeof(buf)) here is a much better idea
            string req = lowercase(client.line());
            if (req == "hello")
            {
                // try to reverse-lookup the client's IP
                string host = phostbyaddr(client.get_ip());
                if (isempty(host))
                    host = iptostring(client.get_ip());
                
                // now send our greeting to the client
                client.putf("Hello, %s (%a), nice to see you!\n",
                    pconst(host), long(client.get_ip()));
                client.flush();
            }

            // close() should be called explicitly to shut down the socket
            // *gracefully*; otherwise ipstream's destructor may close the 
            // socket but in a less polite manner
            client.close();
        }
    }
    catch(estream* e)
    {
        perr.putf("Server error: %s\n", pconst(e->get_message()));
        delete e;
    }
    
    // a real server could enter an infinite loop serving requests
    // and producing separate threads for each connection
}


void svthread::cleanup()
{
}


void inet_test2()
{
    pout.put("\n--- INET CLIENT/SERVER\n");
        
    // we run the server in a separate thread in order to be able
    // to imitate a client connection from the main thread
    svthread server;

    pout.put("\nStarting the server thread...\n");
    server.start();         // it's that easy! :)

    // sleep some time to let the server start its job
    psleep(1000);

    try
    {
        // now create a client socket and send a greeting to our server
        ipstream client(ipaddress(127, 0, 0, 1), testport);
        client.open();
        
        pout.put("Sending a request to the server...\n");
        client.putline("Hello");
        client.flush();
        string rsp = client.line();
        pout.putf("Received: %s\n", pconst(rsp));
        pout.putf("My address and port: %s:%d\n", 
            pconst(iptostring(client.get_myip())), client.get_myport());

        client.close();
    }
    catch(estream* e)
    {
        perr.putf("Error: %s\n", pconst(e->get_message()));
        delete e;
    }

}


//
// UDP test
//


class msgsvthread: public thread
{
protected:
    void execute();
public:
    msgsvthread(): thread(false) {}
    virtual ~msgsvthread() { waitfor(); }
};


void msgsvthread::execute()
{
    ipmsgserver s;
    s.bindall(testport);
    try
    {
        string req = s.receive(1024);
        pout.putf("Server received: %s\n", pconst(req));
        string rsp = "gotcha";
        s.send(rsp);
    }
    catch(estream* e)
    {
        perr.putf("Server error: %s\n", pconst(e->get_message()));
        delete e;
    }
}


void inet_test3()
{
    pout.put("\n--- INET MESSAGE CLIENT/SERVER\n");

    msgsvthread sv;
    sv.start();
    psleep(1000);

    ipmessage m(ipbcast /* ipaddress(127, 0, 0, 1) */, testport);
    try
    {
        string msg = "hello";
        m.send(msg);
        string rsp = m.receive(1024);
        pout.putf("Client received: %s\n", pconst(rsp));
    }
    catch(estream* e)
    {
        perr.putf("Client error: %s\n", pconst(e->get_message()));
        delete e;
    }
}


//
// named pipes test
//


#define TEST_PIPE "ptypes.test"


class npthread: public thread, protected npserver
{
protected:
    virtual void execute();
    virtual void cleanup();
public:
    npthread(): thread(false), npserver(TEST_PIPE)  {}
    virtual ~npthread();
};


npthread::~npthread()
{
    waitfor();
}


void npthread::execute()
{
    namedpipe client;

    try
    {
        serve(client);
        
        if (client.get_active())
        {
            string req = lowercase(client.line());
            if (req == "hello")
            {
                client.putline("Hello, nice to see you!");
                client.flush();
            }
            
            client.close();
        }
    }
    catch(estream* e)
    {
        perr.putf("Pipe server error: %s\n", pconst(e->get_message()));
        delete e;
    }
}


void npthread::cleanup()
{
}


void pipe_test()
{
    npthread server;

    pout.put("\n--- NAMED PIPES\n");
    pout.put("Starting the pipe server thread...\n");
    server.start();

    psleep(1000);

    namedpipe client(TEST_PIPE);

    try
    {
        client.open();
        
        pout.put("Sending a request to the server...\n");
        client.putline("Hello");
        client.flush();
        string rsp = client.line();
        pout.putf("Received: %s\n", pconst(rsp));

        client.close();
    }
    catch(estream* e)
    {
        perr.putf("Error: %s\n", pconst(e->get_message()));
        delete e;
    }
}


#endif // PTYPES_ST


//
// date/time/calendar
//


void time_test()
{
    pout.put("\n--- DATE/TIME/CALENDAR\n");

    tzupdate();

    int year, month, day;
    datetime d = encodedate(9999, 12, 31);
    decodedate(d, year, month, day);
    d = encodedate(1970, 1, 1);
    decodedate(d, year, month, day);

    datetime dt;
    dt = invdatetime;
    int hour, min, sec, msec;
    dt = encodetime(23, 59, 59, 998);
    decodetime(dt, hour, min, sec, msec);

    dt = encodedate(2001, 8, 27) + encodetime(14, 33, 10);
    d = encodedate(2001, 8, 28) + encodetime(14, 33, 10, 500);
    dayofweek(dt);

    dt = now(false);
    pout.putf("Local time: %s\n", pconst(dttostring(dt, "%x %X %Z")));
    datetime utc = now();
    pout.putf("UTC time:   %t GMT\n", utc);

    time_t ut;
    time(&ut);
    pout.putline(dttostring(utodatetime(ut), "%c"));

    int t = tzoffset();
    bool neg = t < 0;
    if (neg)
        t = -t;
    pout.putf("Time zone offset: %c%02d%02d\n", neg ? '-' : '+', t / 60, t % 60);
    {
        // PTypes' birthday (birth moment, if you wish)
        datetime d = encodedate(2000, 3, 30) + encodetime(13, 24, 58, 995);
        pout.putf("PTypes' birth moment: %T GMT\n", d);

        // now see how old is PTypes in days, hours, minutes, etc
        datetime diff = now() - d;
        int hours, mins, secs, msecs;
        decodetime(diff, hours, mins, secs, msecs);
        pout.putf("PTypes' life time: %d days %d hours %d minutes %d seconds and %d milliseconds\n",
            days(diff), hours, mins, secs, msecs);

#ifndef PTYPES_ST
        // measure the difference in milliseconds between two calls to now()
        datetime m = now();
        psleep(17);  // sleep for 17 milliseconds
        pout.putf("A 17 millisecond dream lasted actually %d milliseconds\n", int(now() - m));
        // this will show the actual precision of the clock on the given platform;
        // Windows, f.ex., always shows the difference in 10 msec increments
#endif
    }
}


//
// streams documentation example #2
//


const cset letters("_A-Za-z");
const cset digits("0-9");
const cset identchars = letters + digits;
const cset otherchars = !letters;


void doc_example() 
{
    strlist dic(SL_SORTED);

    infile f("../src/ptypes_test.cxx");

    try 
    {
        f.open();

        while (!f.get_eof()) 
        {
            char c = f.preview();

            // a C identifier starts with a letter
            if (c & letters)
            {
                // ... and may contain letters and digits
                string ident = f.token(identchars);
                int i;
                if (!search(dic, ident, i))
                    add(dic, ident, 0);
            }

            else
                f.skiptoken(otherchars);
        }

    }
    
    catch (estream* e) 
    {
        pout.putf("Error: %s\n", pconst(e->get_message()));
        delete e;
    }

    // now print the dictionary
    for (int i = 0; i < length(dic); i++)
        pout.putline(getstr(dic, i));
}


//
// variant
//


void variant_test()
{
    pout.put("\n--- VARIANT\n");

    variant v0 = 'A';
    variant v1 = short(33);
    variant v2 = "456";
    variant v3 = int(v1) + int(v2);
    variant v4 = string(v1) + " cows";
    string s = v4;
//    s = v4;
    variant v5 = new component();
    variant v6;

    put(v6, 291, v1);
    put(v6, "key", v2);
    put(v6, "another-key", "another-value");
    showstr("33 cows", v4);
    showint(456, v6["key"]);
    showstr("33", v1);
    showint(1, bool(v6));
    v2 = aclone(v6);
    v5 = v6;

    variant vi;
    int i;
    for (i = 0; anext(v6, i, vi);)
        pout.putf("%d\n", int(vi));

    variant v7;
    aadd(v7, v1);
    aadd(v7, v3);
    aadd(v7, v4);
    adel(v7, 2);
    for (i = 0; i < alength(v7); i++)
        pout.putf("%s\n", pconst(string(aget(v7, i))));
}


//
// main
//


int main()
{
    try
    {

        string_test1();
        string_test2();
        string_benchmarks();
        cset_test();

        objlist_test();
        strlist_test();
        strmap_test();
        textmap_test();

        mem_test();
        outfile_test();
        infile_test();
        md5_test();
        putf_test();

        time_test();
        variant_test();

        inet_test1();

#ifndef PTYPES_ST
        pipe_test();

        thread_test();
        rwlock_test();
	trigger_test();

        inet_test2();
        inet_test3();
#endif

    }

    catch (estream* e) 
    {
        perr.putf("\nError: %s\n", pconst(e->get_message()));
	exit(1);
        delete e;
    }

#ifdef DEBUG
    if (stralloc != 0 || objalloc != 0) 
    {
        perr.putf("DBG stralloc: %d, objalloc: %d\n", stralloc, objalloc);
        fatal(255, "Allocation problems");
    }
#endif

    return 0;
}

