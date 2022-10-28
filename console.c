
/***************************************************************************
*
*   Copyright (c) 1997-2022 Jeff V. Merkey
*   7260 SE Tenino St.
*   Portland, Oregon 97206
*   jeffmerkey@gmail.com
*
*   This program is free software; you can redistribute it and/or modify it
*   under the terms of the Lesser GNU Public License as published by the
*   Free Software Foundation, version 2.1, or any later version.
*
*   This program is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   General Public License for more details.
*
*   Original Authorship      :
*      source code written by Jeff V. Merkey
*
*   Original Contributors    :
*      Jeff V. Merkey
*      
*      
*
****************************************************************************
*
*
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  CONSOLE.C
*   DESCRIP  :  Console Abstraction Layer
*   DATE     :  November 1, 1998
*
*
***************************************************************************/

#include "globals.h"


#if (WINDOWS_NT_UTIL)

void cls(void)
{
   HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

   COORD coordScreen = { 0, 0 };    /* here's where we'll home the
				       cursor */
   BOOL bSuccess;
   DWORD cCharsWritten;
   CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
   DWORD dwConSize;                 /* number of character cells in
				       the current buffer */

   /* get the number of character cells in the current buffer */

   bSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );
   dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

   /* fill the entire screen with blanks */

   bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) ' ',
      dwConSize, coordScreen, &cCharsWritten );

   /* get the current text attribute */

   bSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );

   /* now set the buffer's attributes accordingly */

   bSuccess = FillConsoleOutputAttribute( hConsole, csbi.wAttributes,
      dwConSize, coordScreen, &cCharsWritten );

   /* put the cursor at (0, 0) */

   bSuccess = SetConsoleCursorPosition( hConsole, coordScreen );

   return;
}

void ClearScreen(ULONG context)
{
    cls();
}

void WaitForKey()
{
    NWFSPrint("        Press Any Key To Continue ...");
    getc(stdin);
}

ULONG Pause(void)
{
    BYTE ch;

    ch = getc(stdin);
    if (ch == 0x27)
       return 1;
    return 0;
}

#endif


#if (LINUX_UTIL)

void ClearScreen(ULONG context)
{
    printf("%c%c", 0x1B, 'c'); // Reset Console
}

void WaitForKey()
{
    NWFSPrint("        Press Any Key To Continue ...");
    getc(stdin);
}

ULONG Pause(void)
{
    BYTE ch;

    ch = getc(stdin);
    if (ch == 0x27)
       return 1;
    return 0;
}

#endif


#if (DOS_UTIL)

void ClearScreen(ULONG context)
{
    union REGS r;

    // clear the screen

    r.h.ah = 6;  // scroll command
    r.h.al = 0;  // clear screen code
    r.h.ch = 0;  // start row
    r.h.cl = 0;  // start column
    r.h.dh = 24; // end row
    r.h.dl = 79; // end column
    r.h.bh = 7;  // blank line is blank

    int86(0x10, &r, &r);  // go to real mode

    // goto_xy (0, 0)

    r.h.ah = 2;  // cursor address command
    r.h.bh = 0;  // code page 0
    r.h.dh = 0;  // x = 0
    r.h.dl = 0;  // y = 0

    int86(0x10, &r, &r);  // go to real mode

}

void WaitForKey()
{
    NWFSPrint("        Press Any Key To Continue ...");
    getc(stdin);
}

ULONG Pause(void)
{
    BYTE ch;

    ch = getc(stdin);
    if (ch == 0x27)
       return 1;
    return 0;
}

#endif

