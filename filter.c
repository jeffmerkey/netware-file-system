
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
*   FILE     :  FILTER.C
*   DESCRIP  :  printf() suppression object file
*   DATE     :  December 27, 1999
*
*
***************************************************************************/

#include "globals.h"

BYTE LogFileName[20] = "NWFS.LOG";
FILE *logfp = 0;
ULONG EnableLogging = 0;

int log_sprintf(char *buffer, char *format, ...)
{
    int ccode;
    va_list args;

    if (!EnableLogging)
    {
       if (logfp)
	  fclose(logfp);
       logfp = 0;

       logfp = fopen(LogFileName, "a+");
       if (logfp)
       {
	  va_start(args, format);
	  vfprintf(logfp, format, args);
	  fprintf(logfp, "\r\n");
	  va_end(args);

	  fclose(logfp);
	  logfp = 0;
       }
    }

    va_start(args, format);
    ccode = vsprintf(buffer, format, args);
    va_end(args);
    return ccode;
}

int log_printf(char *format, ...)
{
    va_list args;

    if (!EnableLogging)
    {
       if (logfp)
	  fclose(logfp);
       logfp = 0;

       logfp = fopen(LogFileName, "a+");
       if (logfp)
       {
	  va_start(args, format);
	  vfprintf(logfp, format, args);
	  va_end(args);

	  fclose(logfp);
	  logfp = 0;
       }
    }
    return 0;
}

int printk(char *format, ...)
{
    return 0;
}

int DbgPrint(char *format, ...)
{
    return 0;
}
