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

#ifndef __PTIME_H__
#define __PTIME_H__

#ifndef __PPORT_H__
#include "pport.h"
#endif

#ifndef __PTYPES_H__
#include "ptypes.h"
#endif


#include <time.h>


PTYPES_BEGIN

// datetime type: 64-bit, number of milliseconds since midnight 01/01/0001
typedef large datetime;

#define invdatetime LLCONST(-1)

#define _msecsmax 86400000                    // number of milliseconds in one day
#define _daysmax  3652059                     // number of days between 01/01/0001 and 12/31/9999
#define _datetimemax LLCONST(315537897600000) // max. allowed number for datetime type
#define _unixepoch LLCONST(62135596800000)    // difference between time_t and datetime in milliseconds


// datetime general utilities
inline int days(datetime d)            { return int(d / _msecsmax); }
inline int msecs(datetime d)           { return int(d % _msecsmax); }

ptpublic datetime mkdt(int days, int msecs);
ptpublic bool     isvalid(datetime);
ptpublic datetime now(bool utc = true);
ptpublic void     tzupdate();
ptpublic int      tzoffset();
ptpublic string   dttostring(datetime, const char* fmt);
ptpublic string   nowstring(const char* fmt, bool utc = true);
ptpublic datetime utodatetime(time_t u);
ptpublic struct tm* dttotm(datetime dt, struct tm& t);

// date/calendar manipulation
ptpublic bool     isleapyear(int year);
ptpublic int      daysinmonth(int year, int month);
ptpublic int      daysinyear(int year, int month);
ptpublic int      dayofweek(datetime);
ptpublic bool     isdatevalid(int year, int month, int day);
ptpublic datetime encodedate(int year, int month, int day);
ptpublic bool     decodedate(datetime, int& year, int& month, int& day);

// time manipulation
ptpublic bool     istimevalid(int hour, int min, int sec, int msec = 0);
ptpublic datetime encodetime(int hour, int min, int sec, int msec = 0);
ptpublic bool     decodetime(datetime, int& hour, int& min, int& sec, int& msec);
ptpublic bool     decodetime(datetime, int& hour, int& min, int& sec);


PTYPES_END

#endif // __PTIME_H__
