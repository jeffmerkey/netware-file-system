
/***************************************************************************
*
*   Copyright (c) 1998, 1999 Jeff V. Merkey
*   895 West Center Street
*   Orem, Utah  84057
*   jmerkey@utah-nac.org
*
*   This program is free software; you can redistribute it and/or modify it
*   under the terms of the GNU General Public License as published by the
*   Free Software Foundation, version 2, or any later version.
*
*   This program is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   General Public License for more details.
*
*   You are free to modify and re-distribute this program in accordance
*   with the terms specified in the GNU Public License.  The copyright
*   contained in this code is required to be present in any derivative
*   works and you are required to provide the source code for this
*   program as part of any commercial or non-commercial distribution.
*   You are required to respect the rights of the Copyright holders
*   named within this code.
*
*   jmerkey@utah-nac.org is the official maintainer of
*   this code.  You are encouraged to report any bugs, problems, fixes,
*   suggestions, and comments about this software to jmerkey@utah-nac.org
*   or linux-kernel@vger.kernel.org.  New releases, patches, bug fixes, and
*   technical documentation can be found at www.kernel.org.  We will
*   periodically post new releases of this software to www.kernel.org
*   that contain bug fixes and enhanced capabilities.
*
*   Original Authorship      :
*      source code written by Jeff V. Merkey
*
*   Original Contributors    :
*      Jeff V. Merkey
*      Darren Major
*      
*
****************************************************************************
*
*
*   AUTHOR   :  Jeff V. Merkey (jmerkey@utah-nac.org)
*   FILE     :  DATE.C
*   DESCRIP  :  FENRIS Netware and Unix Date/Time conversion routines
*   DATE     :  January 30, 1999
*
*
***************************************************************************/

#include "globals.h"

ULONG MakeNWTime(ULONG second, ULONG minute, ULONG hour,
		 ULONG day, ULONG month, ULONG year)
{
    ULONG DateAndTime = 0;

    // 32-bit format for NetWare date and time
    // Y - year bits
    // M - month bits
    // D - day bits
    // H - hour bits
    // m - minute bits
    // S - second bits
    // [ YYYYYYYM MMMDDDDD HHHHHmmm mmmSSSSS ]

    DateAndTime |= second >> 1;
    DateAndTime |= minute << 5;
    DateAndTime |= hour << 11;
    DateAndTime |= day << 16;
    DateAndTime |= month << 21;

    // year is delta of 1980
    year = year % 100;
    if (year >= 80)
    {
       // year is relative to 1980
       DateAndTime |= (year - 80) << 25;
    }
    else
    {
       // year is relative to 2000
       DateAndTime |= (year + 80) << 25;
    }
    return DateAndTime;

}

void GetNWTime(ULONG DateAndTime,
	       ULONG *second, ULONG *minute, ULONG *hour,
	       ULONG *day, ULONG *month, ULONG *year)
{
    if (second)
       *second = (DateAndTime & 0x1F) << 1;
    if (minute)
       *minute = (DateAndTime >> 5) & 0x3F;
    if (hour)
       *hour = (DateAndTime >> 11) & 0x1F;
    if (day)
       *day = (DateAndTime >> 16) & 0x1F;
    if (month)
       *month = (DateAndTime >> 21) & 0xF;
    if (year)
    {
       *year = (DateAndTime >> 25);
       if ((*year) >= 80)
	  *year = ((*year) - 80) + 2000;
       else
	  *year = (*year) + 1980;
    }
    return;
}


#if (LINUX_20 | LINUX_22 | LINUX_24)

//
//  NOTE:  all dates in the Netware file system are relative to
//         1980.  Novell's current date format can handle dates
//         from 1980 through 2079.  This means that NetWare has
//         a year 2079 problem where the date format would be assumed
//         to "flip" over from 2079 to the next century (i.e. the
//         date format would be assumed to be 2080 through 2179).
//
//         The date/time support routines in this modules all perform
//         date calculations relative to the year 1980.  This could easily
//         be changed to become relative to 2080.
//

ULONG NWFSGetSeconds(void)
{
    return CURRENT_TIME;
}

ULONG NWFSGetSystemTime(void)
{
    return (CURRENT_TIME & ~3);
}

ULONG NWFSSystemToNetwareTime(ULONG SystemTime)
{
    ULONG year, month, day, hour, minute, second;

    GetUnixTime(SystemTime, &second, &minute, &hour, &day, &month, &year);
    return (MakeNWTime(second, minute, hour, day, month, year));
}

ULONG NWFSNetwareToSystemTime(ULONG NetwareTime)
{
    ULONG year, month, day, hour, minute, second;

    GetNWTime(NetwareTime, &second, &minute, &hour, &day, &month, &year);
    return (MakeUnixTime(second, minute, hour, day, month, year));
}

//  The time conversion portion of the functions below come from
//  the fat file system.  MakeUnixTime is derived from the
//  mktime function.

/* Linear day numbers of the respective 1sts in non-leap years. */
int day_n[] =
{
     0,  31,  59, 90, 120, 151, 181, 212,
   243, 273, 304, 334,  0,   0,   0,   0
};
/* JanFebMarApr May Jun Jul Aug Sep Oct Nov Dec */

extern struct timezone sys_tz;

ULONG MakeUnixTime(ULONG second, ULONG minute, ULONG hour,
		   ULONG day, ULONG month, ULONG year)
{
    ULONG secs;

    year = year % 100;
    if (year >= 80)
       year -= 80;
    else
       year += 20;

    month = (month & 15) - 1;
    secs = (second & 31) * 2 + 60 * (minute & 63) + hour * 3600 + 86400 *
	   ((day & 31) - 1 + day_n[month] +
	   (year / 4) + year * 365 - ((year & 3) == 0 && month < 2 ? 1 : 0) + 3653);

    /* days since 1.1.70 plus 80's leap day */
    secs += sys_tz.tz_minuteswest * 60;
    if (sys_tz.tz_dsttime)
    {
       secs -= 3600;
    }
    return secs;
}

void GetUnixTime(ULONG unixdate, ULONG *second, ULONG *minute, ULONG *hour,
		 ULONG *day, ULONG *month, ULONG *year)
{
    int lday, lyear, nl_day, lmonth;

    unixdate -= sys_tz.tz_minuteswest * 60;

    if (sys_tz.tz_dsttime)
       unixdate += 3600;

    if (second)
       *second = (unixdate % 60) / 2;
    if (minute)
       *minute = (unixdate / 60) % 60;
    if (hour)
       *hour = (unixdate / 3600) % 24;

    lday = unixdate / 86400 - 3652;
    lyear = lday / 365;
    if ((lyear + 3) / 4 + 365 * lyear > lday)
	   lyear--;

    lday -= (lyear + 3) / 4 + 365 * lyear;
    if ((lday == 59) && !(lyear & 3))
    {
       nl_day = lday;
       lmonth = 2;
    }
    else
    {
       nl_day = (lyear & 3) || lday <= 59 ? lday : lday - 1;
       for (lmonth = 0; lmonth < 12; lmonth++)
       {
	  if (day_n[lmonth] > nl_day)
	     break;
       }
    }
    if (day)
       *day = nl_day-day_n[lmonth - 1] + 1;
    if (month)
       *month = lmonth;
    if (year)
       *year = (lyear + 1980);

}

#endif

#if (LINUX_UTIL | DOS_UTIL)

ULONG NWFSGetSeconds(void)
{
    return time(NULL);
}

ULONG NWFSGetSystemTime(void)
{
    time_t epoch_time;
    struct tm *tm;
    ULONG DateAndTime = 0, year;

    epoch_time = time(NULL);
    tm = localtime(&epoch_time);

    DateAndTime |= tm->tm_sec >> 1;
    DateAndTime |= tm->tm_min << 5;
    DateAndTime |= tm->tm_hour << 11;
    DateAndTime |= tm->tm_mday << 16;
    DateAndTime |= tm->tm_mon << 21;

    // year is delta of 1980
    year = tm->tm_year % 100;
    if (year >= 80)
    {
       // year is relative to 1980
       DateAndTime |= (year - 80) << 25;
    }
    else
    {
       // year is relative to 2000
       DateAndTime |= (year + 80) << 25;
    }
    return DateAndTime;
}

ULONG NWFSSystemToNetwareTime(ULONG SystemTime)
{
    return SystemTime;
}

ULONG NWFSNetwareToSystemTime(ULONG NetwareTime)
{
    return NetwareTime;
}
#endif


#if (WINDOWS_NT | WINDOWS_NT_RO)

ULONG NWFSGetSeconds(void)
{
    return 0;
}

ULONG NWFSGetSystemTime(void)
{
    return 0;
}

ULONG NWFSSystemToNetwareTime(ULONG SystemTime)
{
    return SystemTime;
}

ULONG NWFSNetwareToSystemTime(ULONG NetwareTime)
{
    return NetwareTime;
}

ULONG NWFSConvertToNetwareTime(IN PTIME_FIELDS tf)
{
    ULONG DateAndTime = 0;

    // 32-bit format for NetWare date and time
    // Y - year bits
    // M - month bits
    // D - day bits
    // H - hour bits
    // m - minute bits
    // S - second bits
    // [ YYYYYYYM MMMDDDDD HHHHHmmm mmmSSSSS ]

    DateAndTime |= tf->Second >> 1;
    DateAndTime |= tf->Minute << 5;
    DateAndTime |= tf->Hour << 11;
    DateAndTime |= tf->Day << 16;
    DateAndTime |= tf->Month << 21;

    // year is delta of 1980
    if (tf->Year >= 80)
       // year is delta of 1900
       DateAndTime |= (tf->Year - 80) << 25;
    else
       // year is delta of 2000
       DateAndTime |= (tf->Year + 20) << 25;
    return DateAndTime;

}

ULONG NWFSConvertFromNetwareTime(IN ULONG DateAndTime,
				 IN PTIME_FIELDS tf)
{
    if (tf)
    {
       tf->Second = (DateAndTime & 0x1F) << 1;
       tf->Minute = (DateAndTime >> 5) & 0x3F;
       tf->Hour = (DateAndTime >> 11) & 0x1F;
       tf->Day = (DateAndTime >> 16) & 0x1F;
       tf->Month = (DateAndTime >> 21) & 0xF;
       tf->Year = (DateAndTime >> 25) + 1980;
       return 0;
    }
    return -1;
}



#endif


#if (WINDOWS_NT_UTIL)

ULONG NWFSGetSeconds(void)
{
    SYSTEMTIME stime;
    register ULONG Seconds;

    GetLocalTime(&stime);

    Seconds = stime.wSecond + (stime.wMinute * 60) +
	      (stime.wHour * 60 * 60) + (stime.wDay * 24 * 60 * 60);
    return Seconds;
}

ULONG NWFSGetSystemTime(void)
{
    SYSTEMTIME stime;
    ULONG DateAndTime = 0;

    GetLocalTime(&stime);

    DateAndTime |= stime.wSecond >> 1;
    DateAndTime |= stime.wMinute << 5;
    DateAndTime |= stime.wHour << 11;
    DateAndTime |= stime.wDay << 16;
    DateAndTime |= stime.wMonth << 21;

    // year is delta of 1980
    stime.wYear = stime.wYear % 100;
    if (stime.wYear >= 80)
    {
       // year is relative to 1980
       DateAndTime |= (stime.wYear - 80) << 25;
    }
    else
    {
       // year is relative to 2000
       DateAndTime |= (stime.wYear + 80) << 25;
    }
    return DateAndTime;
}

ULONG NWFSSystemToNetwareTime(ULONG SystemTime)
{
    return SystemTime;
}

ULONG NWFSNetwareToSystemTime(ULONG NetwareTime)
{
    return NetwareTime;
}


#endif
