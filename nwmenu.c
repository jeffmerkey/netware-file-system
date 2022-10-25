
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
*   You are free to modify and re-distribute this program in accordance
*   with the terms specified in the GNU Public License.  The copyright
*   contained in this code is required to be present in any derivative
*   works and you are required to provide the source code for this
*   program as part of any commercial or non-commercial distribution.
*   You are required to respect the rights of the Copyright holders
*   named within this code.
*
*   jeffmerkey@gmail.com is the official maintainer of
*   this code.  You are encouraged to report any bugs, problems, fixes,
*   suggestions, and comments about this software to jeffmerkey@gmail.com
*   .  New releases, patches, bug fixes, and
*   technical documentation can be found at repo.icapsql.com.  We will
*   periodically post new releases of this software to repo.icapsql.com
*   that contain bug fixes and enhanced capabilities.
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
*   FILE     :  NWMENU.C
*   DESCRIP  :   Text Utility Console Library
*   DATE     :  November 23, 2022
*
*
***************************************************************************/

#include "globals.h"

ULONG BarAttribute = BLUE | BGWHITE;
ULONG FieldAttribute = BLUE | BGWHITE;
ULONG FieldPopupHighlightAttribute = YELLOW | BGBLUE;
ULONG FieldPopupNormalAttribute = BRITEWHITE | BGBLUE;
ULONG ErrorAttribute = BRITEWHITE | BGMAGENTA;

#if (LINUX_UTIL)
ULONG cHandle = 0;
ULONG tHandle = 0;
ULONG HasColor = 0;
#endif

NWSCREEN ConsoleScreen =
{
   0,                      // VGA video address
   0,                      // curr_x = 0
   0,                      // curr_y = 0
   80,                     // columns
   25,                     // lines
   0,                      // mode
   0x07,                   // normal video
   0x71,                   // reverse video
   8                       // tab size
};

MENU_FRAME menu_frame[MAX_MENU];
int ScreenX, ScreenY;

#if (LINUX_UTIL)

//  Here we attempt to map the ncurses color pair numbers into
//  something a little more PC friendly.  Most programs that run
//  under DOS and Windows NT use a direct screen write interface
//  that uses coloring codes that conform to the 6845 CRT controller
//  bits.  This interface creates 64 ncurses color pairs with all possible
//  background colors, then provides a table driven interface that
//  allows you to use PC style color attributes.  This allows us to
//  use one consistent color attribute selection interface for all
//  the platforms this library supports (Linux, DOS, and Windows NT/2000).

int color_map[128]=
{
    1,  2,  3,  4,  5,  6,  7,  8, 8,  2,  3,  4,  5,  6,  7,  8,
    9, 10, 11, 12, 13, 14, 15, 16, 16, 10, 11, 12, 13, 14, 15, 16,
   17, 18, 19, 20, 21, 22, 23, 24, 24, 18, 19, 20, 21, 22, 23, 24,
   25, 26, 27, 28, 29, 30, 31, 32, 32, 26, 27, 28, 29, 30, 31, 32,
   33, 34, 35, 36, 37, 38, 39, 40, 40, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 48, 42, 43, 44, 45, 46, 47, 48,
   49, 50, 51, 52, 53, 54, 55, 56, 56, 50, 51, 52, 53, 54, 55, 56,
   57, 58, 59, 60, 61, 62, 53, 64, 64, 58, 59, 60, 61, 62, 53, 64
};

int attr_map[128]=
{
   0, 0, 0, 0, 0, 0, 0, A_BOLD, 0, A_BOLD, A_BOLD, A_BOLD,
   A_BOLD, A_BOLD, A_BOLD, A_BOLD,
   0, 0, 0, 0, 0, 0, 0, A_BOLD, 0, A_BOLD, A_BOLD, A_BOLD,
   A_BOLD, A_BOLD, A_BOLD, A_BOLD,
   0, 0, 0, 0, 0, 0, 0, A_BOLD, 0, A_BOLD, A_BOLD, A_BOLD,
   A_BOLD, A_BOLD, A_BOLD, A_BOLD,
   0, 0, 0, 0, 0, 0, 0, A_BOLD, 0, A_BOLD, A_BOLD, A_BOLD,
   A_BOLD, A_BOLD, A_BOLD, A_BOLD,
   0, 0, 0, 0, 0, 0, 0, A_BOLD, 0, A_BOLD, A_BOLD, A_BOLD,
   A_BOLD, A_BOLD, A_BOLD, A_BOLD,
   0, 0, 0, 0, 0, 0, 0, A_BOLD, 0, A_BOLD, A_BOLD, A_BOLD,
   A_BOLD, A_BOLD, A_BOLD, A_BOLD,
   0, 0, 0, 0, 0, 0, 0, A_BOLD, 0, A_BOLD, A_BOLD, A_BOLD,
   A_BOLD, A_BOLD, A_BOLD, A_BOLD,
   0, 0, 0, 0, 0, 0, 0, A_BOLD, 0, A_BOLD, A_BOLD, A_BOLD,
   A_BOLD, A_BOLD, A_BOLD, A_BOLD
};

ULONG GetColorPair(ULONG attr)
{
   return ((COLOR_PAIR(color_map[attr & 0x7F]) |
	  attr_map[attr & 0x7F] | ((attr & BLINK) ? A_BLINK : 0)));
}

void SetColor(ULONG attr)
{
    if (HasColor)
       attrset(GetColorPair(attr));
    else
    if (attr == BarAttribute)
       attrset(A_REVERSE);
}

void ClearColor(void)
{
    attroff(A_BOLD | A_BLINK | A_REVERSE);
}

#endif

void ScreenWrite(BYTE *p)
{

#if (DOS_UTIL)
    ScreenUpdate(p);
#endif

#if (LINUX_UTIL)
#endif

}

void hard_xy(ULONG row, ULONG col)
{

#if (WINDOWS_NT_UTIL)
   HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
   COORD coordScreen = { 0, 0 };
   BOOL bSuccess;

   coordScreen.X = (short)col;
   coordScreen.Y = (short)row;

   bSuccess = SetConsoleCursorPosition(hConsole, coordScreen);
#endif

#if (DOS_UTIL)
    ScreenSetCursor(row, col);
#endif

#if (LINUX_UTIL)
    move(row, col);
#endif

    return;
}

void enable_cursor(void)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    CONSOLE_CURSOR_INFO CursorInfo;

    bSuccess = GetConsoleCursorInfo(hConsole, &CursorInfo);
    CursorInfo.bVisible = TRUE;
    bSuccess = SetConsoleCursorInfo(hConsole, &CursorInfo);
#endif

#if (LINUX_UTIL)
    curs_set(1);  // turn on the cursor
#endif

}

void disable_cursor(void)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    CONSOLE_CURSOR_INFO CursorInfo;

    bSuccess = GetConsoleCursorInfo(hConsole, &CursorInfo);
    CursorInfo.bVisible = 0;
    bSuccess = SetConsoleCursorInfo(hConsole, &CursorInfo);
#endif

#if (DOS_UTIL)
    ScreenSetCursor(100, 100);
#endif

#if (LINUX_UTIL)
    curs_set(0);  // turn off the cursor
#endif

}

ULONG InitConsole(void)
{

#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coordScreen = { 0, 0 };
    BOOL bSuccess;
    CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
    DWORD dwConSize;                 /* number of character cells in
				       the current buffer */
    /* get the number of character cells in the current buffer */

    bSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );
    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

    ConsoleScreen.nCols = csbi.dwSize.X;
    ConsoleScreen.nLines = csbi.dwSize.Y;
    if (ConsoleScreen.nLines > 50)
       ConsoleScreen.nLines = 50;

    ConsoleScreen.pVidMem = malloc(ConsoleScreen.nCols *
				   ConsoleScreen.nLines * 2);
    if (!ConsoleScreen.pVidMem)
       return -1;

    ConsoleScreen.pSaved = malloc(ConsoleScreen.nCols *
				  ConsoleScreen.nLines * 2);
    if (!ConsoleScreen.pSaved)
    {
       free(ConsoleScreen.pVidMem);
       return -1;
    }

    disable_cursor();
    return 0;
#endif

#if (DOS_UTIL)
     ConsoleScreen.nCols = ScreenCols();
     ConsoleScreen.nLines = ScreenRows();

     ConsoleScreen.pVidMem = malloc(ConsoleScreen.nCols *
				    ConsoleScreen.nLines * 2);
     if (!ConsoleScreen.pVidMem)
	return -1;

     ConsoleScreen.pSaved = malloc(ConsoleScreen.nCols *
				    ConsoleScreen.nLines * 2);
     if (!ConsoleScreen.pSaved)
     {
	free(ConsoleScreen.pVidMem);
	return -1;
     }

     ScreenRetrieve(ConsoleScreen.pVidMem);
     ScreenRetrieve(ConsoleScreen.pSaved);
     ScreenGetCursor(&ScreenX, &ScreenY);

     disable_cursor();
#endif

#if (LINUX_UTIL)
     register int i, pair;
     int BGColors[8]=
     {
        COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
        COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE
     };

     initscr();
     nonl();
     intrflush(stdscr, FALSE);
     keypad(stdscr, TRUE);
     noecho();

     ConsoleScreen.nCols = COLS;
     ConsoleScreen.nLines = LINES;

     ConsoleScreen.pVidMem = malloc(ConsoleScreen.nCols *
				    ConsoleScreen.nLines * 2);
     if (!ConsoleScreen.pVidMem)
     {
	endwin();
	return -1;
     }

     ConsoleScreen.pSaved = malloc(ConsoleScreen.nCols *
				    ConsoleScreen.nLines * 2);
     if (!ConsoleScreen.pSaved)
     {
	free(ConsoleScreen.pVidMem);
	endwin();
	return -1;
     }

     // if the terminal does not support colors, or if the
     // terminal cannot support at least eight primary colors
     // for foreground/background color pairs, then default
     // the library to use ASCII characters < 127, disable
     // ncurses color attributes, and do not attempt to use
     // the alternate character set for graphic characters.

     if (has_colors())
     {
	if (start_color() == OK)
	{
	   if (COLORS >= 8)
	   {
	      HasColor = TRUE;
              pair = 1;

              // We create our color pairs in the order defined
              // by the PC based text attirbute color scheme.  We do
              // this to make it relatively simple to use a table
              // driven method for mapping the PC style text attributes
              // to ncurses.

              for (i=0; i < 8; i++)
              {
	         init_pair(pair++, COLOR_BLACK, BGColors[i]);
	         init_pair(pair++, COLOR_BLUE, BGColors[i]);
	         init_pair(pair++, COLOR_GREEN, BGColors[i]);
	         init_pair(pair++, COLOR_CYAN, BGColors[i]);
	         init_pair(pair++, COLOR_RED, BGColors[i]);
	         init_pair(pair++, COLOR_MAGENTA, BGColors[i]);
	         init_pair(pair++, COLOR_YELLOW, BGColors[i]);
	         init_pair(pair++, COLOR_WHITE, BGColors[i]);
	      }
	   }
	}
     }

     wclear(stdscr);
     refresh();
     disable_cursor();

#endif

    return 0;
}

ULONG ReleaseConsole(void)
{
#if (WINDOWS_NT_UTIL)
    clear_screen(&ConsoleScreen);
    enable_cursor();
    if (ConsoleScreen.pVidMem)
       free(ConsoleScreen.pVidMem);
    if (ConsoleScreen.pSaved)
       free(ConsoleScreen.pSaved);
#endif

    clear_screen(&ConsoleScreen);

#if (DOS_UTIL)
    clear_screen(&ConsoleScreen);
    enable_cursor();
    ScreenSetCursor(ScreenX, ScreenY);
    ScreenWrite(ConsoleScreen.pSaved);
    if (ConsoleScreen.pVidMem)
       free(ConsoleScreen.pVidMem);
    if (ConsoleScreen.pSaved)
       free(ConsoleScreen.pSaved);
#endif

#if (LINUX_UTIL)
    enable_cursor();
    endwin();

    printf("%c%c", 0x1B, 'c');

    if (ConsoleScreen.pVidMem)
       free(ConsoleScreen.pVidMem);
    if (ConsoleScreen.pSaved)
       free(ConsoleScreen.pSaved);
#endif

    return 0;
}

void copy_data(ULONG *src, ULONG *dest, ULONG len)
{
   register ULONG i;

   for (i=0; i < ((len + 3) / 4); i++)
      *dest++ = *src++;
   return;
}

void set_data(ULONG *dest, ULONG value, ULONG len)
{
   register ULONG i;

   for (i=0; i < ((len + 3) / 4); i++)
      *dest++ = value;
   return;

}

void set_data_b(BYTE *dest, BYTE value, ULONG len)
{
   register ULONG i;

   for (i=0; i < len; i++)
      *dest++ = value;
   return;

}

ULONG get_key(void)
{

#if (DOS_UTIL | WINDOWS_NT_UTIL)
    ULONG c;

    c = getch();
    if ((!c) || ((c & 0xFF) == 0xE0))
    {
       c = getch();
       c |= 0x0100; // identify this key as special
       return c;
    }
    return c;
#endif

#if (LINUX_UTIL)
    ULONG c;

    c = getch();
    return c;
#endif

}

void set_xy(NWSCREEN *screen, ULONG row, ULONG col)
{
    screen->CrntRow = row;
    screen->CrntColumn = col;
    hard_xy(row, col);
    return;
}

void get_xy(NWSCREEN *screen, ULONG *row, ULONG *col)
{
    *row = screen->CrntRow;
    *col = screen->CrntColumn;
    return;
}

void clear_screen(NWSCREEN *screen)
{

#if (WINDOWS_NT_UTIL)
   HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
   COORD coordScreen = { 0, 0 };
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
#endif

#if (DOS_UTIL | LINUX_UTIL)
   register ULONG fill;
   register BYTE ch;

   ch = (BYTE)(screen->NormVid & 0xFF);
   fill = 0x00200020;  //  ' ' space
   fill = fill | (ULONG)(ch << 24);
   fill = fill | (ULONG)(ch << 8);
   set_data((ULONG *)screen->pVidMem, fill,
	    (screen->nCols * screen->nLines * 2));

#if (DOS_UTIL)
   ScreenWrite(ConsoleScreen.pVidMem);
#endif

#if (LINUX_UTIL)
   wclear(stdscr);
   refresh();
#endif

#endif

   return;
}

void move_string(NWSCREEN *screen,
		 ULONG srcRow, ULONG srcCol,
		 ULONG destRow, ULONG destCol,
		 ULONG length)
{
    register ULONG i;
    register BYTE *src_v, *dest_v;
    BYTE buffer[256];
    register BYTE *p = buffer;
    int attr = 0;

    if (length > 255)
       return;

    src_v = screen->pVidMem;
    src_v += (srcRow * (screen->nCols * 2)) + srcCol * 2;

    dest_v = screen->pVidMem;
    dest_v += (destRow * (screen->nCols * 2)) + destCol * 2;

    for (i=0; i < length; i++)
    {
       *p++ = *src_v;
       attr = *(src_v + 1);
       *dest_v++ = *src_v++;
       *dest_v++ = *src_v++;
    }

#if (DOS_UTIL)
    *p = '\0';
    ScreenPutString(buffer, attr, destCol, destRow);
#endif

#if (LINUX_UTIL)
    *p = '\0';

    SetColor(attr);
    p = buffer;
    while (*p)
    {
       if (HasColor)
	  mvaddch(destRow, destCol++, *p > 127 ? *p | A_ALTCHARSET : *p);
       else
	  mvaddch(destRow, destCol++, *p > 127 ? ' ' : *p);
       p++;
    }
    ClearColor();

    refresh();
#endif

}


void put_string(NWSCREEN *screen, BYTE *s, ULONG row, ULONG col, ULONG attr)
{

#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;

    coordScreen.X = (short)col;
    coordScreen.Y = (short)row;

    bSuccess = WriteConsoleOutputCharacter( hConsole, (LPSTR) s,
				lstrlen(s), coordScreen, &cCharsWritten );

    bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
				lstrlen(s), coordScreen, &cCharsWritten );
#endif

#if (DOS_UTIL | LINUX_UTIL)
    register BYTE *v;
    register ULONG len = strlen(s), count;
    BYTE buffer[255];
    register BYTE *p = buffer;

    count = 0;
    v = screen->pVidMem;
    v += (row * (screen->nCols * 2)) + col * 2;
    while (*s)
    {
       if (count++ > ((len <= (screen->nCols - col))
		     ? len : (screen->nCols - col)))
	  break;
       *p++ = *s;
       *v++ = *s++;
       *v++ = attr;
    }

#if (DOS_UTIL)
    *p = '\0';
    ScreenPutString(buffer, attr, col, row);
#endif

#if (LINUX_UTIL)
    *p = '\0';

    SetColor(attr);
    p = buffer;
    while (*p)
    {
       if (HasColor)
	  mvaddch(row, col++, *p > 127 ? *p | A_ALTCHARSET : *p);
       else
	  mvaddch(row, col++, *p > 127 ? ' ' : *p);
       p++;
    }
    ClearColor();

    refresh();
#endif

#endif

}

void put_string_transparent(NWSCREEN *screen, BYTE *s, ULONG row, ULONG col, ULONG attr)
{

#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;

    coordScreen.X = (short)col;
    coordScreen.Y = (short)row;

    bSuccess = WriteConsoleOutputCharacter( hConsole, (LPSTR) s,
				    lstrlen(s), coordScreen, &cCharsWritten );

    if (attr)
       bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
				    lstrlen(s), coordScreen, &cCharsWritten );
#endif

#if (DOS_UTIL | LINUX_UTIL)
    register BYTE *v;
    register ULONG len = strlen(s), count;
    BYTE buffer[255];
    register BYTE *p = buffer;

    count = 0;
    v = screen->pVidMem;
    v += (row * (screen->nCols * 2)) + col * 2;
    while (*s)
    {
       if (count++ > ((len <= (screen->nCols - col))
		     ? len : (screen->nCols - col)))
	  break;
       *p++ = *s;
       *v++ = *s++;
       if (attr)
	  *v++ |= attr;
       else
	  v++;
    }

#if (DOS_UTIL)
    *p = '\0';
    ScreenPutString(buffer, attr, col, row);
#endif

#if (LINUX_UTIL)
    *p = '\0';

    SetColor(attr);
    p = buffer;
    while (*p)
    {
       if (HasColor)
	  mvaddch(row, col++, *p > 127 ? *p | A_ALTCHARSET : *p);
       else
	  mvaddch(row, col++, *p > 127 ? ' ' : *p);
       p++;
    }
    ClearColor();

    refresh();
#endif

#endif

}

void put_string_cleol(NWSCREEN *screen, BYTE *s, ULONG row, ULONG attr)
{

#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;
    ULONG len = lstrlen(s);
    ULONG left = 0;

    coordScreen.X = (short)0;
    coordScreen.Y = (short)row;

    bSuccess = WriteConsoleOutputCharacter( hConsole, (LPSTR) s,
		       len, coordScreen, &cCharsWritten );

    bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
		       len, coordScreen, &cCharsWritten );

    if (screen->nCols >= len)
       left = (screen->nCols - len);

    if (left)
    {
       coordScreen.X = (short)len;
       coordScreen.Y = (short)row;

       bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) ' ',
				    left, coordScreen, &cCharsWritten );

       bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
				    left, coordScreen, &cCharsWritten );
    }

#endif

#if (DOS_UTIL | LINUX_UTIL)
    register ULONG r, i;
    register BYTE *v;
#if (DOS_UTIL | LINUX_UTIL)
    BYTE buffer[255];
    register BYTE *p = buffer;
#endif

    v = screen->pVidMem;
    v += (row * (screen->nCols * 2)) + 0 * 2;
    for (r = 0; r < screen->nCols; r++)
    {
       switch (*s)
       {
	  case '\t':
	     s++;
	     *p++ = ' ';
	     *v++ = ' ';
	     *v++ = attr;
	     for (i=0; i < screen->TabSize - 1; i++)
	     {
		if (r++ > screen->nCols)
		   break;

		*p++ = ' ';
		*v++ = ' ';
		*v++ = attr;
	     }
	     break;

	  default:
	     if (!*s)
	     {
		*p++ = ' ';
		*v++ = ' ';
	     }
	     else
	     {
		*p++ = *s;
		*v++ = *s++;
	     }
	     *v++ = attr;
	     break;
       }
    }

#if (DOS_UTIL)
    *p = '\0';
    ScreenPutString(buffer, attr, 0, row);
#endif

#if (LINUX_UTIL)
    *p = '\0';

    SetColor(attr);
    p = buffer;
    r = 0;
    while (*p)
    {
       if (HasColor)
	  mvaddch(row, r++, *p > 127 ? *p | A_ALTCHARSET : *p);
       else
	  mvaddch(row, r++, *p > 127 ? ' ' : *p);
       p++;
    }
    ClearColor();

    refresh();
#endif

#endif

}

void put_string_to_length(NWSCREEN *screen, BYTE *s, ULONG row,
			  ULONG col, ULONG attr, ULONG len)
{

#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;
    ULONG string_len = lstrlen(s);
    ULONG left = 0;

    if (len > string_len)
       left = (len - string_len);
    else
       string_len = len;

    coordScreen.X = (short)col;
    coordScreen.Y = (short)row;

    bSuccess = WriteConsoleOutputCharacter( hConsole, (LPSTR) s,
				    string_len, coordScreen, &cCharsWritten );

    bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
				    string_len, coordScreen, &cCharsWritten );

    if (left)
    {
       coordScreen.X = (short)(col + string_len);
       coordScreen.Y = (short)row;

       bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) ' ',
				    left, coordScreen, &cCharsWritten );

       bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
				    left, coordScreen, &cCharsWritten );
    }

#endif

#if (DOS_UTIL | LINUX_UTIL)
    register ULONG r, i;
    register BYTE *v;
#if (DOS_UTIL | LINUX_UTIL)
    BYTE buffer[255];
    register BYTE *p = buffer;
#endif

    v = screen->pVidMem;
    v += (row * (screen->nCols * 2)) + col * 2;
    for (r = 0; r < len; r++)
    {
       switch (*s)
       {
	  case '\t':
	     s++;
	     *p++ = ' ';
	     *v++ = ' ';
	     *v++ = attr;
	     for (i=0; i < screen->TabSize - 1; i++)
	     {
		if (r++ > len)
		   break;

		*p++ = ' ';
		*v++ = ' ';
		*v++ = attr;
	     }
	     break;

	  default:
	     if (!*s)
	     {
		*p++ = ' ';
		*v++ = ' ';
	     }
	     else
	     {
		*p++ = *s;
		*v++ = *s++;
	     }
	     *v++ = attr;
	     break;
       }
    }

#if (DOS_UTIL)
    *p = '\0';
    ScreenPutString(buffer, attr, col, row);
#endif

#if (LINUX_UTIL)
    *p = '\0';

    SetColor(attr);
    p = buffer;
    while (*p)
    {
       if (HasColor)
	  mvaddch(row, col++, *p > 127 ? *p | A_ALTCHARSET : *p);
       else
	  mvaddch(row, col++, *p > 127 ? ' ' : *p);
       p++;
    }
    ClearColor();

    refresh();
#endif

#endif

}

void put_char(NWSCREEN *screen, int c, ULONG row, ULONG col, ULONG attr)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;

    coordScreen.X = (short)col;
    coordScreen.Y = (short)row;

    bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) c,
				    1, coordScreen, &cCharsWritten );

    bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
				    1, coordScreen, &cCharsWritten );
#endif

#if (DOS_UTIL | LINUX_UTIL)
    register BYTE *v;

    v = screen->pVidMem;
    v += (row * (screen->nCols * 2)) + col * 2;
    *v++ = c;
    *v = attr;

#if (DOS_UTIL)
    ScreenPutChar(c, attr, col, row);
#endif

#if (LINUX_UTIL)
    SetColor(attr);
    if (HasColor)
       mvaddch(row, col, c);
    else
       mvaddch(row, col, ((c & 0xFF) > 127) ? ' ' : c);
    ClearColor();
#endif

#endif

    return;
}

int get_char(NWSCREEN *screen, ULONG row, ULONG col)
{
#if (WINDOWS_NT_UTIL)
    BYTE v[2];
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;

    coordScreen.X = (short)col;
    coordScreen.Y = (short)row;

    bSuccess = ReadConsoleOutputCharacter( hConsole, (LPSTR) &v,
				    1, coordScreen, &cCharsWritten );
    return (BYTE) v[0];

#endif

#if (DOS_UTIL | LINUX_UTIL)
   register BYTE *v;

   v = screen->pVidMem;
   v += (row * (screen->nCols * 2)) + col * 2;

   return (BYTE)(*v);
#endif

}

int get_char_attribute(NWSCREEN *screen, ULONG row, ULONG col)
{
#if (WINDOWS_NT_UTIL)
    int v[2];
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;

    coordScreen.X = (short)col;
    coordScreen.Y = (short)row;

    bSuccess = ReadConsoleOutputAttribute( hConsole, (LPWORD) &v,
				    1, coordScreen, &cCharsWritten );
    return (BYTE) v[0];

#endif

#if (DOS_UTIL | LINUX_UTIL)
   register BYTE *v;

   v = screen->pVidMem;
   v += (row * (screen->nCols * 2)) + col * 2;

   v++;
   return (BYTE)(*v);
#endif

}

void put_char_cleol(NWSCREEN *screen, int c, ULONG row, ULONG attr)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;

    coordScreen.X = (short)0;
    coordScreen.Y = (short)row;

    bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) c,
			 screen->nCols, coordScreen, &cCharsWritten );

    bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
			 screen->nCols, coordScreen, &cCharsWritten );
#endif

#if (DOS_UTIL | LINUX_UTIL)
    register ULONG i;

    for (i=0; i < screen->nCols; i++)
       put_char(screen, c, row, i, attr);

#if (LINUX_UTIL)
    refresh();
#endif

#endif

}

void put_char_length(NWSCREEN *screen, int c, ULONG row, ULONG col, ULONG attr,
		     ULONG len)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten;
    COORD coordScreen;

    coordScreen.X = (short)col;
    coordScreen.Y = (short)row;
    bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) c,
			 len, coordScreen, &cCharsWritten );

    bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
			 len, coordScreen, &cCharsWritten );
#endif

#if (DOS_UTIL | LINUX_UTIL)
    register ULONG i;

    for (i=0; i < len; i++)
       put_char(screen, c, row, col, attr);

#if (LINUX_UTIL)
    refresh();
#endif

#endif

}

ULONG scroll_display(NWSCREEN *screen, ULONG row, ULONG col,
		     ULONG lines, ULONG cols, ULONG up)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL fSuccess;
    SMALL_RECT srctScrollRect, srctClipRect;
    CHAR_INFO chiFill;
    COORD coordDest;

    if (!cols || !lines)
       return -1;

    if (col > screen->nCols - 1)
       return -1;

    if (cols > screen->nCols)
       return -1;

    if (row > screen->nLines - 1)
       return -1;

    if (lines > screen->nLines)
       return -1;

    srctScrollRect.Top = (short)row;
    srctScrollRect.Bottom = (short)(row + lines - 1);
    srctScrollRect.Left = (short)col;
    srctScrollRect.Right = (short)(col + cols - 1);

    /* The destination for the scroll rectangle is one row up. */

    if (up)
    {
       coordDest.Y = (short)(row - 1);
       coordDest.X = (short)col;
    }
    else
    {
       coordDest.Y = (short)(row + 1);
       coordDest.X = (short)col;
    }

    /*
     * The clipping rectangle is the same as the scrolling rectangle.
     * The destination row is left unchanged.
     */

    srctClipRect = srctScrollRect;

    chiFill.Attributes = (unsigned short)screen->NormVid;
    chiFill.Char.AsciiChar = ' ';

    fSuccess = ScrollConsoleScreenBuffer(
	hConsole,        /* screen buffer handle     */
	&srctScrollRect, /* scrolling rectangle      */
	&srctClipRect,   /* clipping rectangle       */
	coordDest,       /* top left destination cell*/
	&chiFill);       /* fill character and color */

    if (!fSuccess)
       return -1;

    return 0;

#endif

#if (DOS_UTIL | LINUX_UTIL)
    register ULONG i, tCol, tRow;

    if (!cols || !lines)
       return -1;

    if (col > screen->nCols - 1)
       return -1;

    if (cols > screen->nCols)
       return -1;

    if (row > screen->nLines - 1)
       return -1;

    if (lines > screen->nLines)
       return -1;

    if (up)
    {
       tCol = col;
       tRow = row;
       for (i=1; i < lines; i++)
       {
	  move_string(screen,
			tRow + 1,
			tCol,
			tRow,
			tCol,
			cols);
	  tRow++;
       }
       for (i=0; i < cols; i++)
       {
	  put_char(screen,
		     ' ',
		     tRow,
		     tCol + i,
		     screen->NormVid);
       }
    }
    else
    {
       tCol = col;
       tRow = row + (lines - 1);
       for (i=1; i < lines; i++)
       {
	  move_string(screen,
			tRow - 1,
			tCol,
			tRow,
			tCol,
			cols);
	  tRow--;
       }
       for (i=0; i < cols; i++)
       {
	  put_char(screen,
		     ' ',
		     row,
		     tCol + i,
		     screen->NormVid);
       }
    }

#if (LINUX_UTIL)
    refresh();
#endif

#endif
    return 0;
}

//
//
//
//
//
//
//
//
//

ULONG frame_set_xy(ULONG num, ULONG row, ULONG col)
{
    if (!menu_frame[num].owner)
       return - 1;

    menu_frame[num].pcurRow = row;
    menu_frame[num].pcurColumn = col;

    if (strlen(menu_frame[num].header))
       set_xy(menu_frame[num].screen,
	   menu_frame[num].startRow + 3 + row,
	   menu_frame[num].startColumn + 2 + col);
    else
       set_xy(menu_frame[num].screen,
	   menu_frame[num].startRow + 1 + row,
	   menu_frame[num].startColumn + 2 + col);
    return 0;

}

ULONG frame_get_xy(ULONG num, ULONG *row, ULONG *col)
{
    if (!menu_frame[num].owner)
       return -1;

    *row = menu_frame[num].pcurRow;
    *col = menu_frame[num].pcurColumn;
    return 0;
}

void scroll_menu(ULONG num, ULONG up)
{
    register ULONG row, col;

    if (strlen(menu_frame[num].header))
       row = menu_frame[num].startRow + 3;
    else
       row = menu_frame[num].startRow + 1;

    col = menu_frame[num].startColumn + 1;

    scroll_display(menu_frame[num].screen,
		  row,
		  col,
		  menu_frame[num].endRow - row,
		  menu_frame[num].endColumn - col,
		  up);
    return;
}

ULONG get_resp(ULONG num)
{
    BYTE buf[255];
    register ULONG key, row, col, width, temp;
    register ULONG i, ccode;

    if (strlen(menu_frame[num].header))
       row = menu_frame[num].startRow + 3;
    else
       row = menu_frame[num].startRow + 1;

    col = menu_frame[num].startColumn + 1;
    width = menu_frame[num].endColumn - menu_frame[num].startColumn;
    if (width >= 1)
       width -= 1;
    menu_frame[num].top = 0;
    menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;

    temp = menu_frame[num].choice;
    menu_frame[num].choice = 0;
    menu_frame[num].index = 0;
    for (i=0; i < temp; i++)
    {
       menu_frame[num].choice++;
       menu_frame[num].index++;

       if (menu_frame[num].index >= (long)menu_frame[num].elementCount)
	  menu_frame[num].index--;

       if (menu_frame[num].index >= (long)menu_frame[num].windowSize)
       {
	  menu_frame[num].index--;
	  if (menu_frame[num].choice < (long)menu_frame[num].elementCount)
	  {
	     menu_frame[num].top++;
	     menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
	     scroll_menu(num, 1);
	  }
       }

       if (menu_frame[num].choice >= (long)menu_frame[num].elementCount)
	  menu_frame[num].choice--;
    }

    for (;;)
    {
       if (menu_frame[num].elementStrings[menu_frame[num].choice])
       {
	  if (menu_frame[num].ScrollFrame)
	     sprintf(buf, " %c%s", menu_frame[num].ScrollFrame, menu_frame[num].elementStrings[menu_frame[num].choice]);
	  else
	     sprintf(buf, " %s", menu_frame[num].elementStrings[menu_frame[num].choice]);
       }
       else
	  buf[0] = '\0';

       put_string_to_length(menu_frame[num].screen,
			    buf,
			    row + menu_frame[num].index,
			    col,
			    BarAttribute,
			    width);

       if (menu_frame[num].elementCount > menu_frame[num].windowSize && menu_frame[num].top)
       {
	  put_char(menu_frame[num].screen,
		  menu_frame[num].UpChar,
		  row,
		  col,
		  get_char_attribute(menu_frame[num].screen, row, col));
       }
       else
       {
	  put_char(menu_frame[num].screen,
		  ' ',
		  row,
		  col,
		  get_char_attribute(menu_frame[num].screen, row, col));
       }

       if (menu_frame[num].elementCount > menu_frame[num].windowSize
	  && menu_frame[num].bottom < (long)menu_frame[num].elementCount)
       {
	  put_char(menu_frame[num].screen,
		  menu_frame[num].DownChar,
		  row + menu_frame[num].windowSize - 1,
		  col,
		  get_char_attribute(menu_frame[num].screen,
				     row + menu_frame[num].windowSize - 1,
				     col));
       }
       else
       {
	  put_char(menu_frame[num].screen,
		  ' ',
		  row + menu_frame[num].windowSize - 1,
		  col,
		  get_char_attribute(menu_frame[num].screen,
				     row + menu_frame[num].windowSize - 1,
				     col));
       }

       key = get_key();
       if (menu_frame[num].keyboardMask)
	  continue;

       if (menu_frame[num].elementStrings[menu_frame[num].choice])
       {
	  if (menu_frame[num].ScrollFrame)
	     sprintf(buf, " %c%s", menu_frame[num].ScrollFrame, menu_frame[num].elementStrings[menu_frame[num].choice]);
	  else
	     sprintf(buf, " %s", menu_frame[num].elementStrings[menu_frame[num].choice]);
       }
       else
	  buf[0] = '\0';

       put_string_to_length(menu_frame[num].screen,
			    buf,
			    row + menu_frame[num].index,
			    col,
			    menu_frame[num].fill_color | menu_frame[num].text_color,
			    width);

       switch (key)
       {

	  case ENTER:
	     if (menu_frame[num].elementFunction)
	     {
		ccode = (menu_frame[num].elementFunction)(menu_frame[num].screen,
						  menu_frame[num].elementValues[menu_frame[num].choice],
						  menu_frame[num].elementStrings[menu_frame[num].choice],
						  menu_frame[num].choice);
		if (ccode)
		   return ccode;
	     }
	     else
	     {
		if (menu_frame[num].choice >= (long)menu_frame[num].elementCount)
		   return (ULONG) -1;
		else
		   return (menu_frame[num].elementValues[menu_frame[num].choice]);
	     }
	     break;

#if (LINUX_UTIL)
	  case F3:
#else
	  case ESC:
#endif
	     if (menu_frame[num].warnFunction)
	     {
		register ULONG retCode;

		retCode = (menu_frame[num].warnFunction)(menu_frame[num].screen,
							 menu_frame[num].choice);
		if (retCode)
		   return retCode;
		else
		   break;
	     }
	     else
		return -1;

	  case PG_UP:
	     for (i=0; i < menu_frame[num].windowSize - 1; i++)
	     {
		menu_frame[num].choice--;
		menu_frame[num].index--;

		if (menu_frame[num].index < 0)
		{
		   menu_frame[num].index = 0;
		   if (menu_frame[num].choice >= 0)
		   {
		      if (menu_frame[num].top)
			 menu_frame[num].top--;
		      menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
		      scroll_menu(num, 0);
		   }
		}

		if (menu_frame[num].choice < 0)
		   menu_frame[num].choice = 0;

		if (menu_frame[num].elementStrings[menu_frame[num].choice])
		{
		   if (menu_frame[num].ScrollFrame)
		      sprintf(buf, " %c%s", menu_frame[num].ScrollFrame, menu_frame[num].elementStrings[menu_frame[num].choice]);
		   else
		      sprintf(buf, " %s", menu_frame[num].elementStrings[menu_frame[num].choice]);
		}
		else
		   buf[0] = '\0';

		put_string_to_length(menu_frame[num].screen,
				     buf,
				     row + menu_frame[num].index,
				     col,
				     menu_frame[num].fill_color | menu_frame[num].text_color,
				     width);
	     }
	     break;

	  case PG_DOWN:
	     for (i=0; i < menu_frame[num].windowSize - 1; i++)
	     {
		menu_frame[num].choice++;
		menu_frame[num].index++;

		if (menu_frame[num].index >= (long)menu_frame[num].elementCount)
		   menu_frame[num].index--;

		if (menu_frame[num].index >= (long)menu_frame[num].windowSize)
		{
		   menu_frame[num].index--;
		   if (menu_frame[num].choice < (long)menu_frame[num].elementCount)
		   {
		      menu_frame[num].top++;
		      menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
		      scroll_menu(num, 1);
		   }
		}

		if (menu_frame[num].choice >= (long)menu_frame[num].elementCount)
		   menu_frame[num].choice--;

		if (menu_frame[num].elementStrings[menu_frame[num].choice])
		{
		   if (menu_frame[num].ScrollFrame)
		      sprintf(buf, " %c%s", menu_frame[num].ScrollFrame, menu_frame[num].elementStrings[menu_frame[num].choice]);
		   else
		      sprintf(buf, " %s", menu_frame[num].elementStrings[menu_frame[num].choice]);
		}
		else
		   buf[0] = '\0';

		put_string_to_length(menu_frame[num].screen,
				     buf,
				     row + menu_frame[num].index,
				     col,
				     menu_frame[num].fill_color | menu_frame[num].text_color,
				     width);
	     }
	     break;

	  case UP_ARROW:
	     menu_frame[num].choice--;
	     menu_frame[num].index--;

	     if (menu_frame[num].index < 0)
	     {
		menu_frame[num].index = 0;
		if (menu_frame[num].choice >= 0)
		{
		   if (menu_frame[num].top)
		      menu_frame[num].top--;
		   menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
		   scroll_menu(num, 0);
		}
	     }

	     if (menu_frame[num].choice < 0)
		menu_frame[num].choice = 0;

	     break;

	  case DOWN_ARROW:
	     menu_frame[num].choice++;
	     menu_frame[num].index++;

	     if (menu_frame[num].index >= (long)menu_frame[num].elementCount)
		menu_frame[num].index--;

	     if (menu_frame[num].index >= (long)menu_frame[num].windowSize)
	     {
		menu_frame[num].index--;
		if (menu_frame[num].choice < (long)menu_frame[num].elementCount)
		{
		   menu_frame[num].top++;
		   menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
		   scroll_menu(num, 1);
		}
	     }

	     if (menu_frame[num].choice >= (long)menu_frame[num].elementCount)
		menu_frame[num].choice--;

	     break;

	  default:
	     if (menu_frame[num].keyboardHandler)
	     {
		register ULONG retCode;

		retCode = (menu_frame[num].keyboardHandler)(menu_frame[num].screen,
							    key,
							    menu_frame[num].choice);
		if (retCode)
		   return (retCode);
	     }
	     break;
       }

    }

}


ULONG fill_menu(ULONG num, ULONG ch, ULONG attr)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten, i;
    COORD coordScreen;

    for (i=menu_frame[num].startRow; i < menu_frame[num].endRow + 1; i++)
    {
       coordScreen.X = (short)menu_frame[num].startColumn;
       coordScreen.Y = (short)i;

       bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) ch,
				   (menu_frame[num].endColumn + 1) -
				    menu_frame[num].startColumn,
				    coordScreen, &cCharsWritten );

       bSuccess = FillConsoleOutputAttribute( hConsole, (WORD)attr,
				   (menu_frame[num].endColumn + 1) -
				    menu_frame[num].startColumn,
				    coordScreen, &cCharsWritten );
    }
    return 0;

#endif

#if (DOS_UTIL | LINUX_UTIL)
   register ULONG i, j;
   BYTE *v;
   BYTE *t;

   v = menu_frame[num].screen->pVidMem;
   t = v;
   for (i=menu_frame[num].startColumn; i < menu_frame[num].endColumn + 1; i++)
   {
      for (j=menu_frame[num].startRow; j < menu_frame[num].endRow + 1; j++)
      {
	 v = t;
	 v += (j * menu_frame[num].screen->nCols * 2) + i * 2;
	 *v++ = (BYTE) ch;
	 *v = (BYTE) attr;
	 put_char(menu_frame[num].screen, ch, j, i, attr);
      }
   }
#if (LINUX_UTIL)
    refresh();
#endif
   return 0;

#endif

}

ULONG save_menu(ULONG num)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten, i, len;
    COORD coordScreen;
    BYTE *buf_ptr;

    buf_ptr = (BYTE *) menu_frame[num].p;
    for (i=menu_frame[num].startRow; i < menu_frame[num].endRow + 1; i++)
    {
       coordScreen.X = (short)menu_frame[num].startColumn;
       coordScreen.Y = (short)i;

       len = menu_frame[num].endColumn - menu_frame[num].startColumn + 1;
       bSuccess = ReadConsoleOutputCharacter(hConsole,
					    (LPSTR)buf_ptr,
					     len,
					     coordScreen,
					     &cCharsWritten);
       buf_ptr += len;
       bSuccess = ReadConsoleOutputAttribute(hConsole,
					    (LPWORD)buf_ptr,
					     len,
					     coordScreen,
					     &cCharsWritten);
       buf_ptr += len * 2;
    }
    return 0;
#endif

#if (DOS_UTIL | LINUX_UTIL)
   register ULONG i, j;
   BYTE *buf_ptr;
   BYTE *v;
   BYTE *t;

   buf_ptr = (BYTE *) menu_frame[num].p;
   v = menu_frame[num].screen->pVidMem;
   for (i=menu_frame[num].startColumn; i < menu_frame[num].endColumn + 1; i++)
   {
      for (j=menu_frame[num].startRow; j < menu_frame[num].endRow + 1; j++)
      {
	 t = (v + (j * menu_frame[num].screen->nCols * 2) + i * 2);
	 *buf_ptr++ = *t++;
	 *buf_ptr++ = *t;
	 *(t - 1) = ' ';  // fill window
      }
   }
   return 0;

#endif

}

ULONG restore_menu(ULONG num)
{
#if (WINDOWS_NT_UTIL)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    BOOL bSuccess;
    DWORD cCharsWritten, i, len;
    COORD coordScreen;
    BYTE *buf_ptr;

    buf_ptr = (BYTE *) menu_frame[num].p;
    for (i=menu_frame[num].startRow; i < menu_frame[num].endRow + 1; i++)
    {
       coordScreen.X = (short)menu_frame[num].startColumn;
       coordScreen.Y = (short)i;

       len = menu_frame[num].endColumn - menu_frame[num].startColumn + 1;
       bSuccess = WriteConsoleOutputCharacter(hConsole,
					    (LPSTR)buf_ptr,
					     len,
					     coordScreen,
					     &cCharsWritten);

       buf_ptr += len;
       bSuccess = WriteConsoleOutputAttribute(hConsole,
					    (LPWORD)buf_ptr,
					     len,
					     coordScreen,
					     &cCharsWritten);
       buf_ptr += len * 2;
    }
    return 0;
#endif

#if (DOS_UTIL | LINUX_UTIL)
    register ULONG i, j;
    BYTE *buf_ptr;
    BYTE *v;
    BYTE *t;

    buf_ptr = (BYTE *) menu_frame[num].p;
    v = menu_frame[num].screen->pVidMem;
    t = v;
    for (i=menu_frame[num].startColumn; i < menu_frame[num].endColumn + 1; i++)
    {
       for (j=menu_frame[num].startRow; j < menu_frame[num].endRow + 1; j++)
       {
	  v = t;
	  v += (j * menu_frame[num].screen->nCols * 2) + i * 2;
	  *v++ = *buf_ptr++;
	  *v = *buf_ptr++;

#if (LINUX_UTIL)
	  put_char(menu_frame[num].screen,
		  ((*(v - 1) > 127) || (*(v - 1) == 4))
		  ? *(v - 1) | A_ALTCHARSET : *(v - 1),
		  j, i, *v);
#else
	  put_char(menu_frame[num].screen, *(v - 1), j, i, *v);
#endif
       }
       menu_frame[num].active = 0;
    }

#if (LINUX_UTIL)
    refresh();
#endif
    return 0;

#endif

}

ULONG free_menu(ULONG num)
{
   register FIELD_LIST *fl;

   menu_frame[num].curRow = 0;
   menu_frame[num].curColumn = 0;
   menu_frame[num].choice = 0;
   menu_frame[num].index = 0;

   restore_menu(num);

   if (menu_frame[num].p)
      free((void *)menu_frame[num].p);

   if (menu_frame[num].elementStrings)
      free((void *)menu_frame[num].elementStrings);

   if (menu_frame[num].elementValues)
      free((void *)menu_frame[num].elementValues);

   menu_frame[num].p = 0;
   menu_frame[num].elementStrings = 0;
   menu_frame[num].elementValues = 0;
   menu_frame[num].elementCount = 0;
   menu_frame[num].elementFunction = 0;
   menu_frame[num].warnFunction = 0;
   menu_frame[num].owner = 0;

   while (menu_frame[num].head)
   {
      fl = menu_frame[num].head;
      menu_frame[num].head = fl->next;
      free(fl);
   }
   menu_frame[num].head = menu_frame[num].tail = 0;
   menu_frame[num].field_count = 0;

   return 0;

}

ULONG display_menu_header(ULONG num)
{

   register ULONG col, len, i;

   if (!menu_frame[num].header[0])
      return -1;

   col = menu_frame[num].startColumn;
   len = strlen((BYTE *) &menu_frame[num].header[0]);
   len = (menu_frame[num].endColumn - col - len) / 2;
   if (len < 0)
      return -1;

   col = col + len;
   for (i=0; i < menu_frame[num].endColumn - menu_frame[num].startColumn; i++)
   {
      put_char(menu_frame[num].screen,
		 menu_frame[num].HorizontalFrame,
		 menu_frame[num].startRow + 2,
		 menu_frame[num].startColumn + i,
		 menu_frame[num].border_color);
   }

   put_char(menu_frame[num].screen,
	      menu_frame[num].LeftFrame,
	      menu_frame[num].startRow + 2,
	      menu_frame[num].startColumn,
	      menu_frame[num].border_color);

   put_char(menu_frame[num].screen,
	      menu_frame[num].RightFrame,
	      menu_frame[num].startRow + 2,
	      menu_frame[num].endColumn,
	      menu_frame[num].border_color);

   put_string(menu_frame[num].screen,
		menu_frame[num].header,
		menu_frame[num].startRow + 1,
		col,
		menu_frame[num].header_color);

   return 0;


}

ULONG draw_menu_border(ULONG num)
{
   register ULONG i;
   BYTE *v;
   BYTE *t;

   v = menu_frame[num].screen->pVidMem;
   t = v;

   for (i=menu_frame[num].startRow + 1; i < menu_frame[num].endRow; i++)
   {
#if (WINDOWS_NT_UTIL | LINUX_UTIL | DOS_UTIL)
      put_char(menu_frame[num].screen, menu_frame[num].VerticalFrame,
	       i,
	       menu_frame[num].startColumn,
	       menu_frame[num].border_color);
      put_char(menu_frame[num].screen, menu_frame[num].VerticalFrame,
	       i,
	       menu_frame[num].endColumn,
	       menu_frame[num].border_color);
#endif
   }

   for (i=menu_frame[num].startColumn + 1; i < menu_frame[num].endColumn; i++)
   {
#if (WINDOWS_NT_UTIL | LINUX_UTIL | DOS_UTIL)
      put_char(menu_frame[num].screen, menu_frame[num].HorizontalFrame,
	       menu_frame[num].startRow,
	       i,
	       menu_frame[num].border_color);
      put_char(menu_frame[num].screen, menu_frame[num].HorizontalFrame,
	       menu_frame[num].endRow,
	       i,
	       menu_frame[num].border_color);
#endif
   }

   put_char(menu_frame[num].screen, menu_frame[num].UpperLeft,
	    menu_frame[num].startRow,
	    menu_frame[num].startColumn,
	    menu_frame[num].border_color);

   put_char(menu_frame[num].screen, menu_frame[num].LowerLeft,
	    menu_frame[num].endRow,
	    menu_frame[num].startColumn,
	    menu_frame[num].border_color);

   put_char(menu_frame[num].screen, menu_frame[num].UpperRight,
	    menu_frame[num].startRow,
	    menu_frame[num].endColumn,
	    menu_frame[num].border_color);

   put_char(menu_frame[num].screen, menu_frame[num].LowerRight,
	    menu_frame[num].endRow,
	    menu_frame[num].endColumn,
	    menu_frame[num].border_color);

   return 0;


}

void display_menu(ULONG num)
{
    register ULONG i, row, count, width;

    if (strlen(menu_frame[num].header))
       row = menu_frame[num].startRow + 3;
    else
       row = menu_frame[num].startRow + 1;

    count = menu_frame[num].windowSize;
    width = menu_frame[num].endColumn - menu_frame[num].startColumn;
    if (width >= 1)
       width -= 1;

    for (i=0; i < count; i++)
    {
       BYTE buf[255];

       if ((i < menu_frame[num].elementCount) &&
	   menu_frame[num].elementStrings &&
	   menu_frame[num].elementStrings[i])
       {
	  if (menu_frame[num].ScrollFrame)
	     sprintf(buf, " %c%s", menu_frame[num].ScrollFrame, menu_frame[num].elementStrings[i]);
	  else
	     sprintf(buf, " %s", menu_frame[num].elementStrings[i]);

	  put_string_to_length(menu_frame[num].screen,
			    buf,
			    row + i,
			    menu_frame[num].startColumn + 1,
			    menu_frame[num].fill_color | menu_frame[num].text_color,
			    width);
       }
       else
       {
	  if (menu_frame[num].ScrollFrame)
	  {
	     sprintf(buf, " %c", menu_frame[num].ScrollFrame);
	     put_string_to_length(menu_frame[num].screen,
			    buf,
			    row + i,
			    menu_frame[num].startColumn + 1,
			    menu_frame[num].fill_color | menu_frame[num].text_color,
			    width);
	  }
       }
    }
}

ULONG add_item_to_menu(ULONG num, BYTE *item, ULONG value)
{

    if (menu_frame[num].owner && menu_frame[num].elementStrings &&
        menu_frame[num].elementCount < menu_frame[num].elementLimit)
    {
       menu_frame[num].elementStrings[menu_frame[num].elementCount] = item;
       menu_frame[num].elementValues[menu_frame[num].elementCount++] = value;
       return 0;
    }
    return -1;

}

ULONG activate_menu(ULONG num)
{

   register ULONG len;
   register ULONG i, retCode;

   get_xy(menu_frame[num].screen, (ULONG *)&menu_frame[num].pcurRow, (ULONG *)&menu_frame[num].pcurColumn);

   len = 0;
   for (i=0; i < menu_frame[num].elementCount; i++)
   {
      if (strlen(menu_frame[num].elementStrings[i]) > len)
	 len = strlen(menu_frame[num].elementStrings[i]);
   }

   if (menu_frame[num].header)
   {
      if (strlen(menu_frame[num].header) > len)
	 len = strlen(menu_frame[num].header);
   }

   menu_frame[num].endColumn = len + 3 + menu_frame[num].startColumn;

   if (strlen(menu_frame[num].header))
   {
      if (menu_frame[num].windowSize)
	 menu_frame[num].endRow = menu_frame[num].windowSize + 3 +
				menu_frame[num].startRow;
      else
	 menu_frame[num].endRow = menu_frame[num].elementCount + 3 +
				menu_frame[num].startRow;
   }
   else
   {
      if (menu_frame[num].windowSize)
	 menu_frame[num].endRow = menu_frame[num].windowSize + 1 +
				menu_frame[num].startRow;
      else
	 menu_frame[num].endRow = menu_frame[num].elementCount + 1 +
				menu_frame[num].startRow;

   }

   if (menu_frame[num].endRow + 1 > menu_frame[num].screen->nLines - 1 ||
       menu_frame[num].endColumn + 1 > menu_frame[num].screen->nCols - 1)
   {
      return -1;
   }

   if (!menu_frame[num].active)
   {
      menu_frame[num].active = TRUE;
      save_menu(num);
      fill_menu(num, ' ', menu_frame[num].fill_color);
   }

   if (menu_frame[num].border)
   {
      draw_menu_border(num);
      display_menu_header(num);
   }

   display_menu(num);

   retCode = get_resp(num);

   restore_menu(num);

   return retCode;

}

ULONG make_menu(NWSCREEN *screen,
	       BYTE *header,
	       ULONG startRow,
	       ULONG startColumn,
	       ULONG windowSize,
	       ULONG border,
	       ULONG hcolor,
	       ULONG bcolor,
	       ULONG fcolor,
	       ULONG tcolor,
	       ULONG (*elementFunction)(NWSCREEN *, ULONG, BYTE *, ULONG),
	       ULONG (*warnFunction)(NWSCREEN *, ULONG),
	       ULONG (*keyboardHandler)(NWSCREEN *, ULONG, ULONG),
	       ULONG scrollBarPresent,
	       ULONG MaxElements)
{

   register ULONG i;
   register ULONG num;

   if ((MaxElements == (ULONG) -1) || (!MaxElements))
      MaxElements = 100;

   for (num=1; num < MAX_MENU; num++)
   {
      if (!menu_frame[num].owner)
	 break;
   }

   if (!num || num > MAX_MENU)
      return 0;

   if (startRow > screen->nLines - 1 || startRow < 0 ||
       startColumn > screen->nCols - 2 || startColumn < 0)
      return 0;

   menu_frame[num].p = malloc(screen->nLines * screen->nCols * 4);
   if (!menu_frame[num].p)
      return 0;
   set_data((ULONG *)menu_frame[num].p, 0,
            screen->nLines * screen->nCols * 4);

   menu_frame[num].elementStrings = malloc(MaxElements * sizeof(ULONG *));
   if (!menu_frame[num].elementStrings)
   {
      if (menu_frame[num].p)
	 free((void *) menu_frame[num].p);
      menu_frame[num].p = 0;
      return 0;
   }
   set_data((ULONG *)menu_frame[num].elementStrings, 0,
            MaxElements * sizeof(ULONG *));

   menu_frame[num].elementValues = malloc(MaxElements * sizeof(ULONG *));
   if (!menu_frame[num].elementValues)
   {
      if (menu_frame[num].elementStrings)
	 free((void *) menu_frame[num].elementStrings);
      menu_frame[num].elementStrings = 0;

      if (menu_frame[num].p)
	 free((void *) menu_frame[num].p);
      menu_frame[num].p = 0;
      return 0;
   }
   set_data((ULONG *)menu_frame[num].elementValues, 0,
            MaxElements * sizeof(ULONG *));

   for (i=0; i < screen->nCols; i++)
   {
      if (!header || !header[i])
	 break;
      menu_frame[num].header[i] = header[i];
   }
   menu_frame[num].header[i] = 0x00;   // null terminate string

   menu_frame[num].startRow = startRow;
   menu_frame[num].endRow = startRow + 1;
   menu_frame[num].startColumn = startColumn;
   menu_frame[num].endColumn = startColumn + 1;
   menu_frame[num].screen = screen;
   menu_frame[num].border = border;
   menu_frame[num].num = num;
   menu_frame[num].active = 0;
   menu_frame[num].curRow = 0;
   menu_frame[num].curColumn = 0;
   menu_frame[num].header_color = BRITEWHITE;
   menu_frame[num].border_color = BRITEWHITE;
   menu_frame[num].fill_color = BRITEWHITE;
   menu_frame[num].text_color = BRITEWHITE;
   menu_frame[num].windowSize = windowSize;
   menu_frame[num].elementFunction = elementFunction;
   menu_frame[num].elementCount = 0;
   menu_frame[num].warnFunction = warnFunction;
   menu_frame[num].keyboardHandler = keyboardHandler;
   menu_frame[num].elementLimit = MaxElements;
   menu_frame[num].head = menu_frame[num].tail = 0;
   menu_frame[num].field_count = 0;
   menu_frame[num].choice = 0;
   menu_frame[num].index = 0;
   menu_frame[num].top = 0;
   menu_frame[num].bottom = 0;

#if (DOS_UTIL | WINDOWS_NT_UTIL)
   if (border == BORDER_SINGLE)
   {
      menu_frame[num].UpperLeft       = 218;
      menu_frame[num].UpperRight      = 191;
      menu_frame[num].LowerLeft       = 192;
      menu_frame[num].LowerRight      = 217;
      menu_frame[num].LeftFrame       = 195;
      menu_frame[num].RightFrame      = 180;
      menu_frame[num].VerticalFrame   = 179;
      menu_frame[num].HorizontalFrame = 196;
      menu_frame[num].UpChar          = 0x1E;
      menu_frame[num].DownChar        = 0x1F;
   }
   else
   if (border == BORDER_DOUBLE)
   {
      menu_frame[num].UpperLeft       = 201;
      menu_frame[num].UpperRight      = 187;
      menu_frame[num].LowerLeft       = 200;
      menu_frame[num].LowerRight      = 188;
      menu_frame[num].LeftFrame       = 204;
      menu_frame[num].RightFrame      = 185;
      menu_frame[num].VerticalFrame   = 186;
      menu_frame[num].HorizontalFrame = 205;
      menu_frame[num].UpChar          = 0x1E;
      menu_frame[num].DownChar        = 0x1F;
   }
   else
   {
      menu_frame[num].UpperLeft       = 0;
      menu_frame[num].UpperRight      = 0;
      menu_frame[num].LowerLeft       = 0;
      menu_frame[num].LowerRight      = 0;
      menu_frame[num].LeftFrame       = 0;
      menu_frame[num].RightFrame      = 0;
      menu_frame[num].VerticalFrame   = 0;
      menu_frame[num].HorizontalFrame = 0;
      menu_frame[num].UpChar          = 0;
      menu_frame[num].DownChar        = 0;
   }
   if (scrollBarPresent)
      menu_frame[num].ScrollFrame     = 179;
   else
      menu_frame[num].ScrollFrame     = 0;

#endif

#if (LINUX_UTIL)
   if (HasColor)
   {
      if (border == BORDER_SINGLE)
      {
	 menu_frame[num].UpperLeft       = 218 | A_ALTCHARSET;
	 menu_frame[num].UpperRight      = 191 | A_ALTCHARSET;
	 menu_frame[num].LowerLeft       = 192 | A_ALTCHARSET;
	 menu_frame[num].LowerRight      = 217 | A_ALTCHARSET;
	 menu_frame[num].LeftFrame       = 195 | A_ALTCHARSET;
	 menu_frame[num].RightFrame      = 180 | A_ALTCHARSET;
	 menu_frame[num].VerticalFrame   = 179 | A_ALTCHARSET;
	 menu_frame[num].HorizontalFrame = 196 | A_ALTCHARSET;
	 menu_frame[num].UpChar          = 4 | A_ALTCHARSET;
	 menu_frame[num].DownChar        = 4 | A_ALTCHARSET;
      }
      else
      if (border == BORDER_DOUBLE)
      {
	 menu_frame[num].UpperLeft       = 201 | A_ALTCHARSET;
	 menu_frame[num].UpperRight      = 187 | A_ALTCHARSET;
	 menu_frame[num].LowerLeft       = 200 | A_ALTCHARSET;
	 menu_frame[num].LowerRight      = 188 | A_ALTCHARSET;
	 menu_frame[num].LeftFrame       = 204 | A_ALTCHARSET;
	 menu_frame[num].RightFrame      = 185 | A_ALTCHARSET;
	 menu_frame[num].VerticalFrame   = 186 | A_ALTCHARSET;
	 menu_frame[num].HorizontalFrame = 205 | A_ALTCHARSET;
	 menu_frame[num].UpChar          = 4 | A_ALTCHARSET;
	 menu_frame[num].DownChar        = 4 | A_ALTCHARSET;
      }
      else
      {
	 menu_frame[num].UpperLeft       = 0;
	 menu_frame[num].UpperRight      = 0;
	 menu_frame[num].LowerLeft       = 0;
	 menu_frame[num].LowerRight      = 0;
	 menu_frame[num].LeftFrame       = 0;
	 menu_frame[num].RightFrame      = 0;
	 menu_frame[num].VerticalFrame   = 0;
	 menu_frame[num].HorizontalFrame = 0;
	 menu_frame[num].UpChar          = 0;
	 menu_frame[num].DownChar        = 0;
      }
      if (scrollBarPresent)
	menu_frame[num].ScrollFrame     = 179 | A_ALTCHARSET;
      else
	 menu_frame[num].ScrollFrame     = 0;
   }
   else
   {
      if (border == BORDER_SINGLE)
      {
	 menu_frame[num].UpperLeft       = '+';
	 menu_frame[num].UpperRight      = '+';
	 menu_frame[num].LowerLeft       = '+';
	 menu_frame[num].LowerRight      = '+';
	 menu_frame[num].LeftFrame       = '+';
	 menu_frame[num].RightFrame      = '+';
	 menu_frame[num].VerticalFrame   = ':';
	 menu_frame[num].HorizontalFrame = '-';
	 menu_frame[num].UpChar          = '*';
	 menu_frame[num].DownChar        = '*';
      }
      else
      if (border == BORDER_DOUBLE)
      {
	 menu_frame[num].UpperLeft       = '*';
	 menu_frame[num].UpperRight      = '*';
	 menu_frame[num].LowerLeft       = '*';
	 menu_frame[num].LowerRight      = '*';
	 menu_frame[num].LeftFrame       = '*';
	 menu_frame[num].RightFrame      = '*';
	 menu_frame[num].VerticalFrame   = '|';
	 menu_frame[num].HorizontalFrame = '=';
	 menu_frame[num].UpChar          = '*';
	 menu_frame[num].DownChar        = '*';
      }
      else
      {
	 menu_frame[num].UpperLeft       = 0;
	 menu_frame[num].UpperRight      = 0;
	 menu_frame[num].LowerLeft       = 0;
	 menu_frame[num].LowerRight      = 0;
	 menu_frame[num].LeftFrame       = 0;
	 menu_frame[num].RightFrame      = 0;
	 menu_frame[num].VerticalFrame   = 0;
	 menu_frame[num].HorizontalFrame = 0;
	 menu_frame[num].UpChar          = 0;
	 menu_frame[num].DownChar        = 0;
      }
      if (scrollBarPresent)
	 menu_frame[num].ScrollFrame     = '|';
      else
	 menu_frame[num].ScrollFrame     = 0;
   }
#endif

   if (hcolor)
      menu_frame[num].header_color = hcolor;
   if (bcolor)
      menu_frame[num].border_color = bcolor;
   if (fcolor)
      menu_frame[num].fill_color = fcolor;
   if (tcolor)
      menu_frame[num].text_color = tcolor;

   menu_frame[num].owner = 1;

   return num;

}

ULONG menu_write_string(ULONG num, BYTE *p, ULONG row, ULONG col, ULONG attr)
{

   if (!menu_frame[num].active)
      return -1;

   put_string(menu_frame[num].screen,
		p,
		menu_frame[num].startRow + 1 + row,
		menu_frame[num].startColumn + 1 + col,
		attr);

   return 0;

}

void scroll_portal(ULONG num, ULONG up)
{

    register ULONG row, col;

    if (strlen(menu_frame[num].header) || strlen(menu_frame[num].header2))
    {
       row = menu_frame[num].startRow + 3;
       if (strlen(menu_frame[num].header2))
	  row++;
    }
    else
       row = menu_frame[num].startRow + 1;

    col = menu_frame[num].startColumn + 1;

    scroll_display(menu_frame[num].screen,
		  row,
		  col,
		  menu_frame[num].endRow - row,
		  menu_frame[num].endColumn - col,
		  up);
    return;

}

ULONG get_portal_resp(ULONG num)
{
    BYTE buf[255];
    register ULONG key, row, col, width;
    register ULONG i, ccode;

    if (strlen(menu_frame[num].header) || strlen(menu_frame[num].header2))
    {
       row = menu_frame[num].startRow + 3;
       if (strlen(menu_frame[num].header2))
	  row++;
    }
    else
       row = menu_frame[num].startRow + 1;

    col = menu_frame[num].startColumn + 1;
    width = menu_frame[num].endColumn - menu_frame[num].startColumn;
    if (width >= 1)
       width -= 1;

    if (!menu_frame[num].choice)
    {
       menu_frame[num].index = 0;
       menu_frame[num].top = 0;
       menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
    }
    menu_frame[num].selected = 1;

    for (;;)
    {
       if (menu_frame[num].elementStrings[menu_frame[num].choice])
       {
	  if (menu_frame[num].ScrollFrame)
	     sprintf(buf, " %c%s", menu_frame[num].ScrollFrame, menu_frame[num].elementStrings[menu_frame[num].choice]);
	  else
	     sprintf(buf, " %s", menu_frame[num].elementStrings[menu_frame[num].choice]);
       }
       else
	  buf[0] = '\0';

       put_string_to_length(menu_frame[num].screen,
			    buf,
			    row + menu_frame[num].index,
			    col,
			    BarAttribute,
			    width);

       if (menu_frame[num].elementCount > menu_frame[num].windowSize && menu_frame[num].top)
       {
	  put_char(menu_frame[num].screen,
		  menu_frame[num].UpChar,
		  row,
		  col,
		  get_char_attribute(menu_frame[num].screen, row, col));
       }
       else
       {
	  put_char(menu_frame[num].screen,
		  ' ',
		  row,
		  col,
		  get_char_attribute(menu_frame[num].screen, row, col));
       }

       if ((menu_frame[num].elementCount > menu_frame[num].windowSize) &&
	   (menu_frame[num].bottom < (long)menu_frame[num].elementCount) &&
	   (menu_frame[num].bottom < (long)menu_frame[num].elementLimit))
       {
	  put_char(menu_frame[num].screen,
		  menu_frame[num].DownChar,
		  row + menu_frame[num].windowSize - 1,
		  col,
		  get_char_attribute(menu_frame[num].screen,
				     row + menu_frame[num].windowSize - 1,
				     col));
       }
       else
       {
	  put_char(menu_frame[num].screen,
		  ' ',
		  row + menu_frame[num].windowSize - 1,
		  col,
		  get_char_attribute(menu_frame[num].screen,
				     row + menu_frame[num].windowSize - 1,
				     col));
       }

       key = get_key();
       if (menu_frame[num].keyboardMask)
	  continue;

       if (menu_frame[num].elementStrings[menu_frame[num].choice])
       {
	  if (menu_frame[num].ScrollFrame)
	     sprintf(buf, " %c%s", menu_frame[num].ScrollFrame, menu_frame[num].elementStrings[menu_frame[num].choice]);
	  else
	     sprintf(buf, " %s", menu_frame[num].elementStrings[menu_frame[num].choice]);
       }
       else
	  buf[0] = '\0';

       put_string_to_length(menu_frame[num].screen,
			    buf,
			    row + menu_frame[num].index,
			    col,
			    menu_frame[num].fill_color | menu_frame[num].text_color,
			    width);


       switch (key)
       {

#if (LINUX_UTIL)
	  case F3:
#else
	  case ESC:
#endif
	     if (menu_frame[num].warnFunction)
	     {
		register ULONG retCode;

		retCode = (menu_frame[num].warnFunction)(menu_frame[num].screen,
							 menu_frame[num].choice);
		if (retCode)
		{
		   menu_frame[num].selected = 0;
		   return retCode;
		}
		else
		   break;
	     }
	     else
	     {
		menu_frame[num].selected = 0;
		return -1;
	     };

	  case PG_UP:
	     for (i=0; i < menu_frame[num].windowSize - 1; i++)
	     {
		menu_frame[num].choice--;
		menu_frame[num].index--;

		if (menu_frame[num].index < 0)
		{
		   menu_frame[num].index = 0;
		   if (menu_frame[num].choice >= 0)
		   {
		      if (menu_frame[num].top)
			 menu_frame[num].top--;
		      menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
		      scroll_portal(num, 0);
		   }
		}

		if (menu_frame[num].choice < 0)
		   menu_frame[num].choice = 0;

		if (menu_frame[num].elementStrings[menu_frame[num].choice])
		{
		   if (menu_frame[num].ScrollFrame)
		      sprintf(buf, " %c%s", menu_frame[num].ScrollFrame, menu_frame[num].elementStrings[menu_frame[num].choice]);
		   else
		      sprintf(buf, " %s", menu_frame[num].elementStrings[menu_frame[num].choice]);
		}
		else
		   buf[0] = '\0';

		put_string_to_length(menu_frame[num].screen,
				     buf,
				     row + menu_frame[num].index,
				     col,
				     menu_frame[num].fill_color | menu_frame[num].text_color,
				     width);
	     }
	     break;

	  case PG_DOWN:
	     for (i=0; i < menu_frame[num].windowSize - 1; i++)
	     {
		if (menu_frame[num].choice >= (long)menu_frame[num].elementLimit)
		   break;

		menu_frame[num].choice++;
		menu_frame[num].index++;

		if (menu_frame[num].index >= (long)menu_frame[num].elementCount)
		   menu_frame[num].index--;

		if (menu_frame[num].index >= (long)menu_frame[num].windowSize)
		{
		   menu_frame[num].index--;
		   if (menu_frame[num].choice < (long)menu_frame[num].elementCount)
		   {
		      menu_frame[num].top++;
		      menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
		      scroll_portal(num, 1);
		   }
		}

		if (menu_frame[num].choice >= (long)menu_frame[num].elementCount)
		   menu_frame[num].choice--;

		if (menu_frame[num].elementStrings[menu_frame[num].choice])
		{
		   if (menu_frame[num].ScrollFrame)
		      sprintf(buf, " %c%s", menu_frame[num].ScrollFrame, menu_frame[num].elementStrings[menu_frame[num].choice]);
		   else
		      sprintf(buf, " %s", menu_frame[num].elementStrings[menu_frame[num].choice]);
		}
		else
		   buf[0] = '\0';

		put_string_to_length(menu_frame[num].screen,
				     buf,
				     row + menu_frame[num].index,
				     col,
				     menu_frame[num].fill_color | menu_frame[num].text_color,
				     width);
	     }
	     break;

	  case UP_ARROW:
	     menu_frame[num].choice--;
	     menu_frame[num].index--;

	     if (menu_frame[num].index < 0)
	     {
		menu_frame[num].index = 0;
		if (menu_frame[num].choice >= 0)
		{
		   if (menu_frame[num].top)
		      menu_frame[num].top--;
		   menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
		   scroll_portal(num, 0);
		}
	     }

	     if (menu_frame[num].choice < 0)
		menu_frame[num].choice = 0;

	     break;

	  case DOWN_ARROW:
	     if (menu_frame[num].choice >= (long)menu_frame[num].elementLimit)
		break;

	     menu_frame[num].choice++;
	     menu_frame[num].index++;

	     if (menu_frame[num].index >= (long)menu_frame[num].elementCount)
		menu_frame[num].index--;

	     if (menu_frame[num].index >= (long)menu_frame[num].windowSize)
	     {
		menu_frame[num].index--;
		if (menu_frame[num].choice < (long)menu_frame[num].elementCount)
		{
		   menu_frame[num].top++;
		   menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
		   scroll_portal(num, 1);
		}
	     }

	     if (menu_frame[num].choice >= (long)menu_frame[num].elementCount)
		menu_frame[num].choice--;

	     break;

	  case ENTER:
	     if (menu_frame[num].elementFunction)
	     {
		ccode = (menu_frame[num].elementFunction)(menu_frame[num].screen,
						  menu_frame[num].elementValues[menu_frame[num].choice],
						  menu_frame[num].elementStrings[menu_frame[num].choice],
						  menu_frame[num].choice);
		if (ccode)
		   return ccode;
	     }
	     else
		return (menu_frame[num].elementValues[menu_frame[num].choice]);
	     break;

	  default:
	     if (menu_frame[num].keyboardHandler)
	     {
		register ULONG retCode;

		retCode = (menu_frame[num].keyboardHandler)(menu_frame[num].screen,
							    key,
							    menu_frame[num].choice);
		if (retCode)
		   return (retCode);
	     }
	     break;
       }

    }

}

ULONG free_portal(ULONG num)
{
   register FIELD_LIST *fl;

   menu_frame[num].curRow = 0;
   menu_frame[num].curColumn = 0;
   menu_frame[num].selected = 0;
   menu_frame[num].choice = 0;
   menu_frame[num].index = 0;

   restore_menu(num);

   if (menu_frame[num].p)
      free((void *)menu_frame[num].p);
   menu_frame[num].p = 0;

   if (menu_frame[num].elementStrings)
      free((void *)menu_frame[num].elementStrings);
   menu_frame[num].elementStrings = 0;

   if (menu_frame[num].elementValues)
      free((void *)menu_frame[num].elementValues);
   menu_frame[num].elementValues = 0;

   if (menu_frame[num].elementStorage)
      free((void *)menu_frame[num].elementStorage);
   menu_frame[num].elementStorage = 0;

   menu_frame[num].elementCount = 0;
   menu_frame[num].elementFunction = 0;
   menu_frame[num].warnFunction = 0;
   menu_frame[num].owner = 0;

   while (menu_frame[num].head)
   {
      fl = menu_frame[num].head;
      menu_frame[num].head = fl->next;
      free(fl);
   }
   menu_frame[num].head = menu_frame[num].tail = 0;
   menu_frame[num].field_count = 0;

   return 0;

}

ULONG add_item_to_portal(ULONG num, BYTE *item, ULONG index)
{
    if (menu_frame[num].elementStrings)
    {
       menu_frame[num].elementStrings[index] = item;
       return 0;
    }
    return -1;

}

ULONG display_portal_header(ULONG num)
{
   register ULONG col, len, i, adjust;

   if (!menu_frame[num].header[0])
      return -1;

   if (menu_frame[num].header2[0])
   {
      adjust = 3;
      col = menu_frame[num].startColumn + 1;
   }
   else
   {
      adjust = 2;
      col = menu_frame[num].startColumn + 1;
      len = strlen((BYTE *) &menu_frame[num].header[0]);
      len = (menu_frame[num].endColumn - col - len) / 2;
      if (len < 0)
	 return -1;
      col = col + len;
   }

   for (i=0; i < menu_frame[num].endColumn - menu_frame[num].startColumn; i++)
   {
      put_char(menu_frame[num].screen,
		 menu_frame[num].HorizontalFrame,
		 menu_frame[num].startRow + adjust,
		 menu_frame[num].startColumn + i,
		 menu_frame[num].border_color);
   }

   put_char(menu_frame[num].screen,
	      menu_frame[num].LeftFrame,
	      menu_frame[num].startRow + adjust,
	      menu_frame[num].startColumn,
	      menu_frame[num].border_color);

   put_char(menu_frame[num].screen,
	      menu_frame[num].RightFrame,
	      menu_frame[num].startRow + adjust,
	      menu_frame[num].endColumn,
	      menu_frame[num].border_color);

   put_string(menu_frame[num].screen,
		menu_frame[num].header,
		menu_frame[num].startRow + 1,
		col,
		menu_frame[num].header_color);

   if (menu_frame[num].header2[0])
      put_string(menu_frame[num].screen,
		menu_frame[num].header2,
		menu_frame[num].startRow + 2,
		col,
		menu_frame[num].header_color);

   return 0;


}

ULONG write_portal_header1(ULONG num, BYTE *p, ULONG attr)
{
   register ULONG col, len, adjust, i;

   for (i=0; i < menu_frame[num].screen->nCols; i++)
   {
      if (!p || !p[i])
	 break;
      menu_frame[num].header[i] = p[i];
   }
   menu_frame[num].header[i] = 0x00;   // null terminate string

   if (menu_frame[num].header2[0])
   {
      adjust = 3;
      col = menu_frame[num].startColumn + 1;
   }
   else
   {
      adjust = 2;
      col = menu_frame[num].startColumn + 1;
      len = strlen((BYTE *) &menu_frame[num].header[0]);
      len = (menu_frame[num].endColumn - col - len) / 2;
      if (len < 0)
	 return -1;
      col = col + len;
   }

   put_string(menu_frame[num].screen,
              p, menu_frame[num].startRow + 1, col,
	      ((attr & 0x0F) | (menu_frame[num].header_color & 0xF0)));
   return 0;

}

ULONG write_portal_header2(ULONG num, BYTE *p, ULONG attr)
{
   register ULONG col, len, adjust, i;
   
   for (i=0; i < menu_frame[num].screen->nCols; i++)
   {
      if (!p || !p[i])
	 break;
      menu_frame[num].header2[i] = p[i];
   }
   menu_frame[num].header2[i] = 0x00;   // null terminate string

   
   if (menu_frame[num].header2[0])
   {
      adjust = 3;
      col = menu_frame[num].startColumn + 1;
   }
   else
   {
      adjust = 2;
      col = menu_frame[num].startColumn + 1;
      len = strlen((BYTE *) &menu_frame[num].header2[0]);
      len = (menu_frame[num].endColumn - col - len) / 2;
      if (len < 0)
	 return -1;
      col = col + len;
   }

   put_string(menu_frame[num].screen,
              p, menu_frame[num].startRow + 2, col,
	      ((attr & 0x0F) | (menu_frame[num].header_color & 0xF0)));
   return 0;

}

ULONG draw_portal_border(ULONG num)
{
   register ULONG i;
   BYTE *v;
   BYTE *t;

   v = menu_frame[num].screen->pVidMem;
   t = v;

   for (i=menu_frame[num].startRow + 1; i < menu_frame[num].endRow; i++)
   {
#if (WINDOWS_NT_UTIL | LINUX_UTIL | DOS_UTIL)
      put_char(menu_frame[num].screen, menu_frame[num].VerticalFrame,
	       i,
	       menu_frame[num].startColumn,
	       menu_frame[num].border_color);

      put_char(menu_frame[num].screen, menu_frame[num].VerticalFrame,
	       i,
	       menu_frame[num].endColumn,
	       menu_frame[num].border_color);
#endif
   }

   for (i=menu_frame[num].startColumn + 1; i < menu_frame[num].endColumn; i++)
   {
#if (WINDOWS_NT_UTIL | LINUX_UTIL | DOS_UTIL)
      put_char(menu_frame[num].screen, menu_frame[num].HorizontalFrame,
	       menu_frame[num].startRow,
	       i,
	       menu_frame[num].border_color);

      put_char(menu_frame[num].screen, menu_frame[num].HorizontalFrame,
	       menu_frame[num].endRow,
	       i,
	       menu_frame[num].border_color);
#endif
   }

   put_char(menu_frame[num].screen, menu_frame[num].UpperLeft,
	    menu_frame[num].startRow,
	    menu_frame[num].startColumn,
	    menu_frame[num].border_color);

   put_char(menu_frame[num].screen, menu_frame[num].LowerLeft,
	    menu_frame[num].endRow,
	    menu_frame[num].startColumn,
	    menu_frame[num].border_color);

   put_char(menu_frame[num].screen, menu_frame[num].UpperRight,
	    menu_frame[num].startRow,
	    menu_frame[num].endColumn,
	    menu_frame[num].border_color);

   put_char(menu_frame[num].screen, menu_frame[num].LowerRight,
	    menu_frame[num].endRow,
	    menu_frame[num].endColumn,
	    menu_frame[num].border_color);

   return 0;

}

void display_portal(ULONG num)
{
    register ULONG i, row, count, width;

    if (!menu_frame[num].choice)
    {
       menu_frame[num].index = 0;
       menu_frame[num].top = 0;
       menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
    }

    if (strlen(menu_frame[num].header) || strlen(menu_frame[num].header2))
    {
       row = menu_frame[num].startRow + 3;
       if (strlen(menu_frame[num].header2))
	  row++;
    }
    else
       row = menu_frame[num].startRow + 1;

    count = menu_frame[num].windowSize;
    width = menu_frame[num].endColumn - menu_frame[num].startColumn;
    if (width >= 1)
       width -= 1;

    for (i=0; i < count; i++)
    {
       BYTE buf[255];

       if ((i < menu_frame[num].elementCount) &&
	   menu_frame[num].elementStrings &&
	   menu_frame[num].elementStrings[i])
       {
	  if (menu_frame[num].ScrollFrame)
	     sprintf(buf, " %c%s", menu_frame[num].ScrollFrame, menu_frame[num].elementStrings[i]);
	  else
	     sprintf(buf, " %s", menu_frame[num].elementStrings[i]);

	  put_string_to_length(menu_frame[num].screen,
			    buf,
			    row + i,
			    menu_frame[num].startColumn + 1,
			    menu_frame[num].fill_color | menu_frame[num].text_color,
			    width);
       }
       else
       {
	  if (menu_frame[num].ScrollFrame)
	  {
	     sprintf(buf, " %c", menu_frame[num].ScrollFrame);
	     put_string_to_length(menu_frame[num].screen,
			    buf,
			    row + i,
			    menu_frame[num].startColumn + 1,
			    menu_frame[num].fill_color | menu_frame[num].text_color,
			    width);
	  }
       }
    }
}

ULONG update_portal(ULONG num)
{

    register ULONG i, row, col, width;
    BYTE buf[255];

    if (!menu_frame[num].active)
       return -1;

    if (!menu_frame[num].selected)
       return -1;

    if (strlen(menu_frame[num].header) || strlen(menu_frame[num].header2))
    {
       row = menu_frame[num].startRow + 3;
       if (strlen(menu_frame[num].header2))
	  row++;
    }
    else
       row = menu_frame[num].startRow + 1;

    col = menu_frame[num].startColumn + 1;
    width = menu_frame[num].endColumn - menu_frame[num].startColumn;
    if (width >= 1)
       width -= 1;

    for (i=0; i < menu_frame[num].windowSize; i++)
    {
       if (i < menu_frame[num].elementCount)
       {
	  if (menu_frame[num].elementStrings[menu_frame[num].top + i])
	  {
	     if (menu_frame[num].ScrollFrame)
		sprintf(buf, " %c%s", menu_frame[num].ScrollFrame, menu_frame[num].elementStrings[menu_frame[num].top + i]);
	     else
		sprintf(buf, " %s", menu_frame[num].elementStrings[menu_frame[num].top + i]);
	  }
	  else
	     buf[0] = '\0';

	  put_string_to_length(menu_frame[num].screen,
			    buf,
			    row + i,
			    menu_frame[num].startColumn + 1,
			    (row + i == row + menu_frame[num].index)
			    ? BarAttribute
			    : menu_frame[num].fill_color | menu_frame[num].text_color,
			    width);
       }
    }

    if (menu_frame[num].elementCount > menu_frame[num].windowSize && menu_frame[num].top)
    {
	  put_char(menu_frame[num].screen,
		  menu_frame[num].UpChar,
		  row,
		  col,
		  get_char_attribute(menu_frame[num].screen, row, col));
    }
    else
    {
	  put_char(menu_frame[num].screen,
		  ' ',
		  row,
		  col,
		  get_char_attribute(menu_frame[num].screen, row, col));
    }

    if (menu_frame[num].elementCount > menu_frame[num].windowSize
	  && menu_frame[num].bottom < (long)menu_frame[num].elementCount)
    {
	  put_char(menu_frame[num].screen,
		  menu_frame[num].DownChar,
		  row + menu_frame[num].windowSize - 1,
		  col,
		  get_char_attribute(menu_frame[num].screen,
				     row + menu_frame[num].windowSize - 1,
				     col));
    }
    else
    {
	  put_char(menu_frame[num].screen,
		  ' ',
		  row + menu_frame[num].windowSize - 1,
		  col,
		  get_char_attribute(menu_frame[num].screen,
				     row + menu_frame[num].windowSize - 1,
				     col));
    }

    return 0;

}

ULONG update_static_portal(ULONG num)
{

    register ULONG i, row, col, width;
    BYTE buf[255];

    if (strlen(menu_frame[num].header) || strlen(menu_frame[num].header2))
    {
       row = menu_frame[num].startRow + 3;
       if (strlen(menu_frame[num].header2))
	  row++;
    }
    else
       row = menu_frame[num].startRow + 1;

    col = menu_frame[num].startColumn + 1;
    width = menu_frame[num].endColumn - menu_frame[num].startColumn;
    if (width >= 1)
       width -= 1;

    for (i=0; i < menu_frame[num].windowSize; i++)
    {
       if (i < menu_frame[num].elementCount)
       {
	  if (menu_frame[num].elementStrings[menu_frame[num].top + i])
	  {
	     if (menu_frame[num].ScrollFrame)
		sprintf(buf, " %c%s", menu_frame[num].ScrollFrame, menu_frame[num].elementStrings[menu_frame[num].top + i]);
	     else
		sprintf(buf, " %s", menu_frame[num].elementStrings[menu_frame[num].top + i]);
	  }
	  else
	     buf[0] = '\0';

	  put_string_to_length(menu_frame[num].screen,
			    buf,
			    row + i,
			    menu_frame[num].startColumn + 1,
			    (row + i == row + menu_frame[num].index)
			    ? menu_frame[num].fill_color | menu_frame[num].text_color
			    : menu_frame[num].fill_color | menu_frame[num].text_color,
			    width);
       }
    }

    if (menu_frame[num].elementCount > menu_frame[num].windowSize &&
	menu_frame[num].top)
    {
	  put_char(menu_frame[num].screen,
		  menu_frame[num].UpChar,
		  row,
		  col,
		  get_char_attribute(menu_frame[num].screen, row, col));
    }
    else
    {
	  put_char(menu_frame[num].screen,
		  ' ',
		  row,
		  col,
		  get_char_attribute(menu_frame[num].screen, row, col));
    }

    if (menu_frame[num].elementCount > menu_frame[num].windowSize
	  && menu_frame[num].bottom < (long)menu_frame[num].elementCount)
    {
	  put_char(menu_frame[num].screen,
		  menu_frame[num].DownChar,
		  row + menu_frame[num].windowSize - 1,
		  col,
		  get_char_attribute(menu_frame[num].screen,
				     row + menu_frame[num].windowSize - 1,
				     col));
    }
    else
    {
	  put_char(menu_frame[num].screen,
		  ' ',
		  row + menu_frame[num].windowSize - 1,
		  col,
		  get_char_attribute(menu_frame[num].screen,
				     row + menu_frame[num].windowSize - 1,
				     col));
    }

    return 0;

}

ULONG activate_portal(ULONG num)
{
   register ULONG retCode;

   get_xy(menu_frame[num].screen, (ULONG *)&menu_frame[num].pcurRow, (ULONG *)&menu_frame[num].pcurColumn);

   if (!menu_frame[num].active)
   {
      menu_frame[num].active = TRUE;
      save_menu(num);
      fill_menu(num, ' ', menu_frame[num].fill_color);
      if (menu_frame[num].border)
      {
	 draw_portal_border(num);
	 display_portal_header(num);
      }
   }

   display_portal(num);

   retCode = get_portal_resp(num);

   restore_menu(num);

   return retCode;

}

ULONG activate_static_portal(ULONG num)
{
   get_xy(menu_frame[num].screen, (ULONG *)&menu_frame[num].pcurRow, (ULONG *)&menu_frame[num].pcurColumn);

   if (!menu_frame[num].active)
   {
      menu_frame[num].active = TRUE;
      save_menu(num);
      fill_menu(num, ' ', menu_frame[num].fill_color);
      if (menu_frame[num].border)
      {
	 draw_portal_border(num);
	 display_portal_header(num);
      }
   }

   display_portal(num);
   return 0;

}

ULONG deactivate_static_portal(ULONG num)
{
   if (!menu_frame[num].active)
      return -1;

   restore_menu(num);
   return 0;
}

ULONG make_portal(NWSCREEN *screen,
		 BYTE *header,
		 BYTE *header2,
		 ULONG startRow,
		 ULONG startColumn,
		 ULONG endRow,
		 ULONG endColumn,
		 ULONG numberOfLines,
		 ULONG border,
		 ULONG hcolor,
		 ULONG bcolor,
		 ULONG fcolor,
		 ULONG tcolor,
		 ULONG (*lineFunction)(NWSCREEN *, ULONG, BYTE *, ULONG),
		 ULONG (*warnFunction)(NWSCREEN *, ULONG),
		 ULONG (*keyboardHandler)(NWSCREEN *, ULONG, ULONG),
		 ULONG scrollBarPresent)
{

   register ULONG i;
   register ULONG num;

   for (num=1; num < MAX_MENU; num++)
   {
      if (!menu_frame[num].owner)
	 break;
   }

   if (!num || num > MAX_MENU)
      return 0;

   if (startRow > screen->nLines - 1 || startRow < 0 ||
       startColumn > screen->nCols - 2 || startColumn < 0)
      return 0;

   menu_frame[num].p = malloc(screen->nLines * screen->nCols * 4);
   if (!menu_frame[num].p)
      return 0;
   set_data((ULONG *) menu_frame[num].p, 0, screen->nLines * screen->nCols);

   menu_frame[num].elementStorage = malloc(numberOfLines * screen->nCols * 2);
   if (!menu_frame[num].elementStorage)
   {
      if (menu_frame[num].p)
	 free((void *) menu_frame[num].p);
      menu_frame[num].p = 0;

      return 0;
   }
   set_data_b((BYTE *) menu_frame[num].elementStorage, 0x20, numberOfLines * screen->nCols);

   menu_frame[num].elementStrings = malloc(numberOfLines * sizeof(ULONG) * 2);
   if (!menu_frame[num].elementStrings)
   {
      if (menu_frame[num].p)
	 free((void *) menu_frame[num].p);
      menu_frame[num].p = 0;

      if (menu_frame[num].elementStorage)
	 free((void *) menu_frame[num].elementStorage);
      menu_frame[num].elementStorage = 0;
      return 0;
   }
   set_data((ULONG *) menu_frame[num].elementStrings, 0, numberOfLines * sizeof(ULONG));

   menu_frame[num].elementValues =  malloc(numberOfLines * sizeof(ULONG) * 2);
   if (!menu_frame[num].elementValues)
   {
      if (menu_frame[num].elementStrings)
	 free((void *) menu_frame[num].elementStrings);
      menu_frame[num].elementStrings = 0;

      if (menu_frame[num].p)
	 free((void *) menu_frame[num].p);
      menu_frame[num].p = 0;

      if (menu_frame[num].elementStorage)
	 free((void *) menu_frame[num].elementStorage);
      menu_frame[num].elementStorage = 0;
      return 0;
   }
   set_data((ULONG *) menu_frame[num].elementValues, 0, numberOfLines * sizeof(ULONG));

   for (i=0; i < numberOfLines + 1; i++)
   {
      register BYTE *p = &menu_frame[num].elementStorage[i * screen->nCols];
      add_item_to_portal(num, p, i);
      p[screen->nCols - 1] = '\0';
   }

   for (i=0; i < screen->nCols; i++)
   {
      if (!header || !header[i])
	 break;
      menu_frame[num].header[i] = header[i];
   }
   menu_frame[num].header[i] = 0x00;   // null terminate string

   for (i=0; i < screen->nCols; i++)
   {
      if (!header2 || !header2[i])
	 break;
      menu_frame[num].header2[i] = header2[i];
   }
   menu_frame[num].header2[i] = 0x00;   // null terminate string

   menu_frame[num].startRow = startRow;
   menu_frame[num].endRow = endRow;
   menu_frame[num].startColumn = startColumn;
   menu_frame[num].endColumn = endColumn;
   menu_frame[num].screen = screen;
   menu_frame[num].border = border;
   menu_frame[num].num = num;
   menu_frame[num].active = 0;
   menu_frame[num].curRow = 0;
   menu_frame[num].curColumn = 0;
   menu_frame[num].selected = 0;
   menu_frame[num].header_color = BRITEWHITE;
   menu_frame[num].border_color = BRITEWHITE;
   menu_frame[num].fill_color = BRITEWHITE;
   menu_frame[num].text_color = BRITEWHITE;
   menu_frame[num].elementFunction = lineFunction;
   menu_frame[num].warnFunction = warnFunction;
   menu_frame[num].keyboardHandler = keyboardHandler;
   menu_frame[num].elementLimit = 0;
   menu_frame[num].head = menu_frame[num].tail = 0;
   menu_frame[num].field_count = 0;
   menu_frame[num].choice = 0;
   menu_frame[num].index = 0;
   menu_frame[num].top = 0;
   menu_frame[num].bottom = 0;

#if (DOS_UTIL | WINDOWS_NT_UTIL)
   if (border == BORDER_SINGLE)
   {
      menu_frame[num].UpperLeft       = 218;
      menu_frame[num].UpperRight      = 191;
      menu_frame[num].LowerLeft       = 192;
      menu_frame[num].LowerRight      = 217;
      menu_frame[num].LeftFrame       = 195;
      menu_frame[num].RightFrame      = 180;
      menu_frame[num].VerticalFrame   = 179;
      menu_frame[num].HorizontalFrame = 196;
      menu_frame[num].UpChar          = 0x1E;
      menu_frame[num].DownChar        = 0x1F;
   }
   else
   if (border == BORDER_DOUBLE)
   {
      menu_frame[num].UpperLeft       = 201;
      menu_frame[num].UpperRight      = 187;
      menu_frame[num].LowerLeft       = 200;
      menu_frame[num].LowerRight      = 188;
      menu_frame[num].LeftFrame       = 204;
      menu_frame[num].RightFrame      = 185;
      menu_frame[num].VerticalFrame   = 186;
      menu_frame[num].HorizontalFrame = 205;
      menu_frame[num].UpChar          = 0x1E;
      menu_frame[num].DownChar        = 0x1F;
   }
   else
   {
      menu_frame[num].UpperLeft       = 0;
      menu_frame[num].UpperRight      = 0;
      menu_frame[num].LowerLeft       = 0;
      menu_frame[num].LowerRight      = 0;
      menu_frame[num].LeftFrame       = 0;
      menu_frame[num].RightFrame      = 0;
      menu_frame[num].VerticalFrame   = 0;
      menu_frame[num].HorizontalFrame = 0;
      menu_frame[num].UpChar          = 0;
      menu_frame[num].DownChar        = 0;
   }
   if (scrollBarPresent)
      menu_frame[num].ScrollFrame     = 179;
   else
      menu_frame[num].ScrollFrame     = 0;

#endif

#if (LINUX_UTIL)
   if (HasColor)
   {
      if (border == BORDER_SINGLE)
      {
	 menu_frame[num].UpperLeft       = 218 | A_ALTCHARSET;
	 menu_frame[num].UpperRight      = 191 | A_ALTCHARSET;
	 menu_frame[num].LowerLeft       = 192 | A_ALTCHARSET;
	 menu_frame[num].LowerRight      = 217 | A_ALTCHARSET;
	 menu_frame[num].LeftFrame       = 195 | A_ALTCHARSET;
	 menu_frame[num].RightFrame      = 180 | A_ALTCHARSET;
	 menu_frame[num].VerticalFrame   = 179 | A_ALTCHARSET;
	 menu_frame[num].HorizontalFrame = 196 | A_ALTCHARSET;
	 menu_frame[num].UpChar          = 4 | A_ALTCHARSET;
	 menu_frame[num].DownChar        = 4 | A_ALTCHARSET;
      }
      else
      if (border == BORDER_DOUBLE)
      {
	 menu_frame[num].UpperLeft       = 201 | A_ALTCHARSET;
	 menu_frame[num].UpperRight      = 187 | A_ALTCHARSET;
	 menu_frame[num].LowerLeft       = 200 | A_ALTCHARSET;
	 menu_frame[num].LowerRight      = 188 | A_ALTCHARSET;
	 menu_frame[num].LeftFrame       = 204 | A_ALTCHARSET;
	 menu_frame[num].RightFrame      = 185 | A_ALTCHARSET;
	 menu_frame[num].VerticalFrame   = 186 | A_ALTCHARSET;
	 menu_frame[num].HorizontalFrame = 205 | A_ALTCHARSET;
	 menu_frame[num].UpChar          = 4 | A_ALTCHARSET;
	 menu_frame[num].DownChar        = 4 | A_ALTCHARSET;
      }
      else
      {
	 menu_frame[num].UpperLeft       = 0;
	 menu_frame[num].UpperRight      = 0;
	 menu_frame[num].LowerLeft       = 0;
	 menu_frame[num].LowerRight      = 0;
	 menu_frame[num].LeftFrame       = 0;
	 menu_frame[num].RightFrame      = 0;
	 menu_frame[num].VerticalFrame   = 0;
	 menu_frame[num].HorizontalFrame = 0;
	 menu_frame[num].UpChar          = 0;
	 menu_frame[num].DownChar        = 0;
      }
      if (scrollBarPresent)
	menu_frame[num].ScrollFrame     = 179 | A_ALTCHARSET;
      else
	 menu_frame[num].ScrollFrame     = 0;
   }
   else
   {
      if (border == BORDER_SINGLE)
      {
	 menu_frame[num].UpperLeft       = '+';
	 menu_frame[num].UpperRight      = '+';
	 menu_frame[num].LowerLeft       = '+';
	 menu_frame[num].LowerRight      = '+';
	 menu_frame[num].LeftFrame       = '+';
	 menu_frame[num].RightFrame      = '+';
	 menu_frame[num].VerticalFrame   = ':';
	 menu_frame[num].HorizontalFrame = '-';
	 menu_frame[num].UpChar          = '*';
	 menu_frame[num].DownChar        = '*';
      }
      else
      if (border == BORDER_DOUBLE)
      {
	 menu_frame[num].UpperLeft       = '*';
	 menu_frame[num].UpperRight      = '*';
	 menu_frame[num].LowerLeft       = '*';
	 menu_frame[num].LowerRight      = '*';
	 menu_frame[num].LeftFrame       = '*';
	 menu_frame[num].RightFrame      = '*';
	 menu_frame[num].VerticalFrame   = '|';
	 menu_frame[num].HorizontalFrame = '=';
	 menu_frame[num].UpChar          = '*';
	 menu_frame[num].DownChar        = '*';
      }
      else
      {
	 menu_frame[num].UpperLeft       = 0;
	 menu_frame[num].UpperRight      = 0;
	 menu_frame[num].LowerLeft       = 0;
	 menu_frame[num].LowerRight      = 0;
	 menu_frame[num].LeftFrame       = 0;
	 menu_frame[num].RightFrame      = 0;
	 menu_frame[num].VerticalFrame   = 0;
	 menu_frame[num].HorizontalFrame = 0;
	 menu_frame[num].UpChar          = 0;
	 menu_frame[num].DownChar        = 0;
      }
      if (scrollBarPresent)
	 menu_frame[num].ScrollFrame     = '|';
      else
	 menu_frame[num].ScrollFrame     = 0;
   }
#endif

   if (menu_frame[num].header[0] || menu_frame[num].header2[0])
   {
      menu_frame[num].windowSize = endRow - startRow - 3;
      if (menu_frame[num].header2[0] && menu_frame[num].windowSize)
	 menu_frame[num].windowSize--;
   }
   else
      menu_frame[num].windowSize = endRow - startRow - 1;

   menu_frame[num].elementCount = numberOfLines;
   if (hcolor)
      menu_frame[num].header_color = hcolor;
   if (bcolor)
      menu_frame[num].border_color = bcolor;
   if (fcolor)
      menu_frame[num].fill_color = fcolor;
   if (tcolor)
      menu_frame[num].text_color = tcolor;

   menu_frame[num].owner = 1;

   return num;


}

ULONG set_portal_limit(ULONG num, ULONG limit)
{
   if (!menu_frame[num].owner)
      return -1;

   if (limit < menu_frame[num].elementLimit)
      menu_frame[num].elementLimit = limit;

   return 0;
}

ULONG write_portal(ULONG num, BYTE *p, ULONG row, ULONG col, ULONG attr)
{

   register ULONG i;
   register BYTE *v;

   if (!menu_frame[num].owner)
      return -1;

   if (attr) {};

   if (row > menu_frame[num].elementCount)
      return -1;

   if (col > menu_frame[num].screen->nCols || !*p)
      return -1;

   if (menu_frame[num].elementStrings)
   {
      v = menu_frame[num].elementStrings[row];
      if (!v)
	 return -1;

      for (i=0; i < menu_frame[num].screen->nCols; i++)
      {
	 if (*p && i >= col)
	    *v = *p++;
	 if (*v == '\0')
	    *v = ' ';
	 v++;
      }
      menu_frame[num].elementStrings[row][menu_frame[num].screen->nCols - 1] = '\0';
      if (row > menu_frame[num].elementLimit)
	 menu_frame[num].elementLimit = row;

      return 0;
   }
   return -1;

}

ULONG write_portal_char(ULONG num, BYTE p, ULONG row, ULONG col, ULONG attr)
{
   register BYTE *v;

   if (!menu_frame[num].owner)
      return -1;

   if (attr) {};

   if (row > menu_frame[num].elementCount)
      return -1;

   if (col > menu_frame[num].screen->nCols)
      return -1;

   if (menu_frame[num].elementStrings)
   {
      v = menu_frame[num].elementStrings[row];
      if (!v)
	 return -1;

      v[col] = p;
      if (row > menu_frame[num].elementLimit)
	 menu_frame[num].elementLimit = row;

      return 0;
   }
   return -1;

}

ULONG write_portal_cleol(ULONG num, BYTE *p, ULONG row, ULONG col, ULONG attr)
{

   register ULONG i;
   register BYTE *v;

   if (!menu_frame[num].owner)
      return -1;

   if (attr) {};

   if (row > menu_frame[num].elementCount)
      return -1;

   if (col > menu_frame[num].screen->nCols || !*p)
      return -1;

   if (menu_frame[num].elementStrings)
   {
      v = menu_frame[num].elementStrings[row];
      if (!v)
	 return -1;

      for (i=0; i < menu_frame[num].screen->nCols; i++)
      {
	 if (*p && i >= col)
	    *v = *p++;
	 else
	 if (i >= col)
	    *v = ' ';
	 v++;
      }
      menu_frame[num].elementStrings[row][menu_frame[num].screen->nCols - 1] = '\0';
      if (row > menu_frame[num].elementLimit)
	 menu_frame[num].elementLimit = row;

      return 0;
   }
   return -1;

}

ULONG write_screen_comment_line(NWSCREEN *screen, BYTE *p, ULONG attr)
{
    put_string_cleol(screen,
		      p,
		      screen->nLines - 1,
		      attr);
    return 0;
}

ULONG clear_portal(ULONG num)
{
   register ULONG i, j;
   register BYTE *v;
   BYTE buffer[255];

   if (!menu_frame[num].owner)
      return -1;

   for (i=0; (i < menu_frame[num].screen->nCols) && (i < 255); i++)
      buffer[i] = ' ';
   buffer[i] = '\0';

   for (i=0; i < menu_frame[num].elementCount; i++)
   {
      v = menu_frame[num].elementStrings[i];
      if (!v)
	 return -1;

      for (j=0; (j < menu_frame[num].screen->nCols) && (j < 255); j++)
	 *v++ = ' ';
      menu_frame[num].elementStrings[i][menu_frame[num].screen->nCols - 1] = '\0';
   }
   menu_frame[num].elementLimit = 0;

   menu_frame[num].choice = 0;
   menu_frame[num].index = 0;
   menu_frame[num].top = 0;
   menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;

   return 0;

}

ULONG disable_portal_input(ULONG num)
{
   register ULONG retCode = menu_frame[num].keyboardMask;
   menu_frame[num].keyboardMask = TRUE;
   return retCode;
}

ULONG enable_portal_input(ULONG num)
{
   register ULONG retCode = menu_frame[num].keyboardMask;
   menu_frame[num].keyboardMask = 0;
   return retCode;
}

ULONG error_portal(BYTE *p, ULONG row)
{
    register ULONG portal;
    register ULONG len, startCol, endCol;

    len = strlen(p);
    if (!ConsoleScreen.nCols || (ConsoleScreen.nCols < len))
       return -1;

    startCol = ((ConsoleScreen.nCols - len) / 2) - 2;
    endCol = ConsoleScreen.nCols - startCol;
    portal = make_portal(&ConsoleScreen,
		       0,
		       0,
		       row,
		       startCol,
		       row + 4,
		       endCol,
		       3,
		       BORDER_SINGLE,
		       ErrorAttribute,
		       ErrorAttribute,
		       ErrorAttribute,
		       ErrorAttribute,
		       0,
		       0,
		       0,
		       FALSE);

    if (!portal)
       return -1;

    write_portal(portal, p, 1, 1, BRITEWHITE | BGMAGENTA);

    activate_static_portal(portal);

    get_key();

    if (portal)
    {
       deactivate_static_portal(portal);
       free_portal(portal);
    }
    return 0;
}

ULONG message_portal(BYTE *p, ULONG row, ULONG attr, ULONG wait)
{
    register ULONG portal;
    register ULONG len, startCol, endCol;

    len = strlen(p);
    if (!ConsoleScreen.nCols || (ConsoleScreen.nCols < len))
       return -1;

    startCol = ((ConsoleScreen.nCols - len) / 2) - 2;
    endCol = ConsoleScreen.nCols - startCol;
    portal = make_portal(&ConsoleScreen,
		       0,
		       0,
		       row,
		       startCol,
		       row + 4,
		       endCol,
		       3,
		       BORDER_SINGLE,
		       (attr & ~BLINK),
		       (attr & ~BLINK),
		       (attr & ~BLINK),
		       attr,
		       0,
		       0,
		       0,
		       FALSE);

    if (!portal)
       return -1;

    write_portal(portal, p, 1, 1, attr);

    activate_static_portal(portal);

    if (wait)
       get_key();

    if (portal)
    {
       deactivate_static_portal(portal);
       free_portal(portal);
    }
    return 0;
}

ULONG create_message_portal(BYTE *p, ULONG row, ULONG attr)
{
    register ULONG portal;
    register ULONG len, startCol, endCol;

    len = strlen(p);
    if (!ConsoleScreen.nCols || (ConsoleScreen.nCols < len))
       return -1;

    startCol = ((ConsoleScreen.nCols - len) / 2) - 2;
    endCol = ConsoleScreen.nCols - startCol;
    portal = make_portal(&ConsoleScreen,
		       0,
		       0,
		       row,
		       startCol,
		       row + 4,
		       endCol,
		       3,
		       BORDER_SINGLE,
		       (attr & ~BLINK),
		       (attr & ~BLINK),
		       (attr & ~BLINK),
		       attr,
		       0,
		       0,
		       0,
		       FALSE);

    if (!portal)
       return -1;

    write_portal(portal, p, 1, 1, attr);

    activate_static_portal(portal);

    return portal;
}

ULONG close_message_portal(ULONG portal)
{
    if (portal > MAX_MENU)
       return -1;

    if (portal)
    {
       deactivate_static_portal(portal);
       free_portal(portal);
    }
    return 0;
}

ULONG confirm_menu(BYTE *confirm, ULONG row, ULONG attr)
{
    register ULONG mNum, retCode, len, startCol;

    len = strlen(confirm);
    if (!ConsoleScreen.nCols || (ConsoleScreen.nCols < len))
       return -1;

    startCol = ((ConsoleScreen.nCols - len) / 2) - 2;
    mNum = make_menu(&ConsoleScreen,
		     confirm,
		     row,
		     startCol,
		     2,
		     BORDER_DOUBLE,
		     attr,
		     attr,
		     BRITEWHITE | (attr & 0xF0),
		     BRITEWHITE | (attr & 0xF0),
		     0,
		     0,
		     0,
		     TRUE,
		     0);

    add_item_to_menu(mNum, "No", 0);
    add_item_to_menu(mNum, "Yes", 1);

    retCode = activate_menu(mNum);
    if (retCode == (ULONG) -1)
       retCode = 0;

    free_menu(mNum);

    return retCode;
}

void insert_field_node(ULONG num, FIELD_LIST *fl)
{
    if (!menu_frame[num].head)
    {
       menu_frame[num].head = fl;
       menu_frame[num].tail = fl;
       fl->next = fl->prior = 0;
    }
    else
    {
       menu_frame[num].tail->next = fl;
       fl->next = 0;
       fl->prior = menu_frame[num].tail;
       menu_frame[num].tail = fl;
    }
    menu_frame[num].field_count++;
    return;
}

ULONG add_field_to_portal(ULONG num, ULONG row, ULONG col, ULONG attr,
			BYTE *prompt, ULONG plen,
			BYTE *buffer, ULONG buflen,
			BYTE **MenuStrings, ULONG MenuItems,
			ULONG MenuDefault, ULONG *MenuResult,
			ULONG flags)
{
    register FIELD_LIST *fl;

    if (!menu_frame[num].owner)
       return -1;

    if (row > menu_frame[num].elementCount)
       return -1;

    if (col > (menu_frame[num].screen->nCols - 1))
       return -1;

    if ((col + plen + buflen + 1) > (menu_frame[num].screen->nCols - 1))
       return -1;

    fl = (FIELD_LIST *)malloc(sizeof(FIELD_LIST));
    if (!fl)
       return -1;

    NWFSSet(fl, 0, sizeof(FIELD_LIST));

    fl->next = fl->prior = 0;
    fl->portal = num;
    fl->row = row;
    fl->col = col;
    fl->prompt = prompt;
    fl->plen = plen;
    fl->buffer = buffer;
    fl->buflen = buflen;
    fl->flags = flags;
    fl->attr = attr;
    fl->MenuStrings = MenuStrings;
    fl->MenuItems = MenuItems;
    fl->MenuResult = MenuResult;
    fl->result = MenuDefault;

    insert_field_node(num, fl);

    return 0;
}

ULONG input_portal_fields(ULONG num)
{
   register ULONG ccode, i, temp, insert = 0;
   register ULONG key, menuRow, len, screenRow, menuCol, adj;
   register FIELD_LIST *fl;
   register BYTE *p;

   if (!menu_frame[num].owner)
      return -1;

   if (strlen(menu_frame[num].header))
      adj = 3;
   else
      adj = 1;

   fl = menu_frame[num].head;
   while (fl)
   {
      ccode = write_portal(num, fl->prompt, fl->row, fl->col, fl->attr);
      if (ccode)
	 return ccode;
      fl->pos = strlen(fl->buffer);
      fl = fl->next;
   }
   update_static_portal(num);
   enable_cursor();

   fl = menu_frame[num].head;
   while (fl)
   {
      if (fl->MenuItems && fl->MenuStrings)
      {
	 put_string_to_length(menu_frame[num].screen,
			   fl->MenuStrings[fl->result],
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   fl->attr,
			   fl->buflen);
      }
      else
      {
	 put_string_to_length(menu_frame[num].screen,
			   fl->buffer,
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   fl->attr,
			   fl->buflen);
      }
      fl = fl->next;
   }

   // go to first field row,col coordinates

   fl = menu_frame[num].head;
   frame_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
   if (fl->MenuItems && fl->MenuStrings)
   {
      put_string_to_length(menu_frame[num].screen,
			   fl->MenuStrings[fl->result],
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   FieldAttribute,
			   fl->buflen);
   }
   else
      put_string_to_length(menu_frame[num].screen,
			fl->buffer,
			menu_frame[num].startRow + adj + fl->row,
			menu_frame[num].startColumn + 2 + fl->col +
			fl->plen + 1,
			FieldAttribute,
			fl->buflen);

   frame_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
   for (;;)
   {
      key = get_key();
      switch (key)
      {
#if (LINUX_UTIL)
	 case ESC:
	    break;
#else
	 case F3:
	    break;
#endif

	 case F1: case F2: case F4: case F5: case F6:
	 case F7: case F8: case F9: case F11: case F12:
	    break;

	 case F10:
	    (fl->buflen)
	    ? (fl->buffer[fl->buflen - 1] = '\0')
	    : (fl->buffer[0] = '\0');

	    disable_cursor();
	    update_static_portal(num);
	    return 0;

	 case HOME:
	    if (fl->MenuItems || fl->MenuStrings)
	       break;
	    fl->pos = 0;
	    frame_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	    break;

	 case END:
	    if (fl->MenuItems || fl->MenuStrings)
	       break;
	    fl->pos = strlen(fl->buffer);
	    frame_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	    break;

	 case UP_ARROW:
	    (fl->buflen)
	    ? (fl->buffer[fl->buflen - 1] = '\0')
	    : (fl->buffer[0] = '\0');

	    if (fl->MenuItems && fl->MenuStrings)
	    {
	       put_string_to_length(menu_frame[num].screen,
			   fl->MenuStrings[fl->result],
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   fl->attr,
			   fl->buflen);
	    }
	    else
	       put_string_to_length(menu_frame[num].screen,
			   fl->buffer,
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   fl->attr,
			   fl->buflen);
	    fl = fl->prior;
	    if (!fl)
	       fl = menu_frame[num].tail;
	    if (fl->MenuItems && fl->MenuStrings)
	    {
	       put_string_to_length(menu_frame[num].screen,
			   fl->MenuStrings[fl->result],
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   FieldAttribute,
			   fl->buflen);
	    }
	    else
	       put_string_to_length(menu_frame[num].screen,
			   fl->buffer,
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   FieldAttribute,
			   fl->buflen);

	    fl->pos = strlen(fl->buffer);
	    frame_set_xy(num, fl->row, fl->col + fl->plen +
			       fl->pos + 1);
	    break;

	 case DOWN_ARROW:
	    (fl->buflen)
	    ? (fl->buffer[fl->buflen - 1] = '\0')
	    : (fl->buffer[0] = '\0');

	    if (fl->MenuItems && fl->MenuStrings)
	    {
	       put_string_to_length(menu_frame[num].screen,
			   fl->MenuStrings[fl->result],
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   fl->attr,
			   fl->buflen);
	    }
	    else
	       put_string_to_length(menu_frame[num].screen,
			   fl->buffer,
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   fl->attr,
			   fl->buflen);
	    fl = fl->next;
	    if (!fl)
	       fl = menu_frame[num].head;
	    if (fl->MenuItems && fl->MenuStrings)
	    {
	       put_string_to_length(menu_frame[num].screen,
			   fl->MenuStrings[fl->result],
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   FieldAttribute,
			   fl->buflen);
	    }
	    else
	       put_string_to_length(menu_frame[num].screen,
			   fl->buffer,
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   FieldAttribute,
			   fl->buflen);
	    fl->pos = strlen(fl->buffer);
	    frame_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	    break;

	 case PG_UP:
	 case PG_DOWN:
            break;

	 case INS:
            if (insert)
               insert = 0;
            else
               insert = 1;
            break;

	 case DEL:
	    p = (BYTE *) &fl->buffer[fl->pos];
	    temp = fl->pos;
	    p++;
	    while ((*p) && (temp < fl->buflen))
	    {
	       fl->buffer[temp++] = *p++;
	    }
	    fl->buffer[temp] = '\0';

	    put_string_to_length(menu_frame[num].screen,
			   fl->buffer,
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   FieldAttribute,
			   fl->buflen);

	    frame_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	    break;

	 case LEFT_ARROW:
	    if (fl->MenuItems || fl->MenuStrings)
		     break;

	    if (fl->pos)
	    {
	       fl->pos--;
	       frame_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	    }
	    break;

	 case RIGHT_ARROW:
	    if (fl->MenuItems || fl->MenuStrings)
	       break;

	    if (fl->pos < (fl->buflen - 1))
	    {
	       fl->pos++;
	       frame_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	    }
	    break;


	 case SPACE:
	    if (fl->MenuItems && fl->MenuStrings)
	    {
	       len = (fl->MenuItems + 1) / 2;
	       screenRow = fl->row + adj + menu_frame[fl->portal].startRow;
	       if (screenRow >= len)
		  menuRow = screenRow - len;
	       else
		  menuRow = screenRow;

	       len = 0;
	       for (i=0; i < fl->MenuItems; i++)
	       {
		  if (strlen(fl->MenuStrings[i]) > len)
		     len = strlen(fl->MenuStrings[i]);
	       }
	       menuCol = ((ConsoleScreen.nCols - len) / 2);

	       fl->MenuPortal = make_menu(&ConsoleScreen,
						0,
						menuRow,
						menuCol,
						fl->MenuItems,
						BORDER_DOUBLE,
						FieldPopupHighlightAttribute,
						FieldPopupHighlightAttribute,
						FieldPopupNormalAttribute,
						FieldPopupNormalAttribute,
						0,
						0,
						0,
						TRUE,
						0);

	       if (!fl->MenuPortal)
		  break;

	       for (i=0; i < fl->MenuItems; i++)
		  add_item_to_menu(fl->MenuPortal,
					 fl->MenuStrings[i], i + 1);

	       disable_cursor();
	       menu_frame[fl->MenuPortal].choice = fl->result;
	       ccode = activate_menu(fl->MenuPortal);
	       enable_cursor();
	       frame_set_xy(num, fl->row, fl->col + fl->plen +
				  fl->pos + 1);

	       if (ccode == (ULONG) -1)
		  fl->result = 0;
	       else
	       if (ccode)
		  fl->result = ccode - 1;
	       else
		  fl->result = ccode;

	       if (fl->MenuResult)
		  *(fl->MenuResult) = fl->result;

	       if (fl->MenuPortal)
		  free_menu(fl->MenuPortal);
	       fl->MenuPortal = 0;

	       put_string_to_length(menu_frame[num].screen,
			   fl->MenuStrings[fl->result],
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   FieldAttribute,
			   fl->buflen);
	       break;
	    }

	    if (fl->pos < (fl->buflen - 1))
	    {
               if (!insert)
               {
	          fl->buffer[fl->pos] = ' ';
	          write_portal_char(num, ' ', fl->row, fl->col + fl->plen +
				       fl->pos + 1, fl->attr);
	          put_char(menu_frame[num].screen,
			 ' ',
			 menu_frame[num].startRow + adj + fl->row,
			 menu_frame[num].startColumn + 2 + fl->col +
			 fl->plen + fl->pos + 1,
			 FieldAttribute);

	          fl->pos++;
	          frame_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
               }
               else
               {
		  if (strlen(fl->buffer) < (fl->buflen - 1))
                  {
	             for (i = fl->buflen; i > fl->pos; i--)
		        fl->buffer[i] = fl->buffer[i - 1];

	             fl->buffer[fl->pos] = ' ';
	             write_portal_char(num, ' ', fl->row, fl->col + fl->plen +
				       fl->pos + 1, fl->attr);
	             put_char(menu_frame[num].screen,
			 ' ',
			 menu_frame[num].startRow + adj + fl->row,
			 menu_frame[num].startColumn + 2 + fl->col +
			 fl->plen + fl->pos + 1,
			 FieldAttribute);

		     put_string_to_length(menu_frame[num].screen,
			   fl->buffer,
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   FieldAttribute,
			   fl->buflen);

	             fl->pos++;
	             frame_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
                  }
               }
	    }
	    break;

	 case BKSP:
	    if (fl->MenuItems || fl->MenuStrings)
	       break;

	    if (fl->pos)
	    {
	       fl->pos--;
	       if (!fl->buffer[fl->pos + 1])
               {
	          write_portal_char(num, ' ', fl->row, fl->col + fl->plen +
				       fl->pos + 1, fl->attr);
	          put_char(menu_frame[num].screen,
			 ' ',
			 menu_frame[num].startRow + adj + fl->row,
			 menu_frame[num].startColumn + 2 + fl->col +
			 fl->plen + fl->pos + 1,
			 FieldAttribute);
		  fl->buffer[fl->pos] = '\0';
               }
	       else
               {
                  for (i = fl->pos; i < fl->buflen; i++)
	             fl->buffer[i] = fl->buffer[i + 1];
                  fl->buffer[i] = '\0';
               }

	       put_string_to_length(menu_frame[num].screen,
			   fl->buffer,
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   FieldAttribute,
			   fl->buflen);

	       frame_set_xy(num, fl->row, fl->col + fl->plen +
			  fl->pos + 1);
	    }
	    break;

	 case ENTER:
	    if (fl->MenuItems && fl->MenuStrings)
	    {
	       len = (fl->MenuItems + 1) / 2;
	       screenRow = fl->row + adj + menu_frame[fl->portal].startRow;
	       if (screenRow >= len)
		  menuRow = screenRow - len;
	       else
		  menuRow = screenRow;

	       len = 0;
	       for (i=0; i < fl->MenuItems; i++)
	       {
		  if (strlen(fl->MenuStrings[i]) > len)
		     len = strlen(fl->MenuStrings[i]);
	       }
	       menuCol = ((ConsoleScreen.nCols - len) / 2);

	       fl->MenuPortal = make_menu(&ConsoleScreen,
						0,
						menuRow,
						menuCol,
						fl->MenuItems,
						BORDER_DOUBLE,
						FieldPopupHighlightAttribute,
						FieldPopupHighlightAttribute,
						FieldPopupNormalAttribute,
						FieldPopupNormalAttribute,
						0,
						0,
						0,
						TRUE,
						0);

	       if (!fl->MenuPortal)
		  break;

	       for (i=0; i < fl->MenuItems; i++)
		  add_item_to_menu(fl->MenuPortal, fl->MenuStrings[i], i + 1);

	       disable_cursor();
	       menu_frame[fl->MenuPortal].choice = fl->result;
	       ccode = activate_menu(fl->MenuPortal);
	       enable_cursor();
	       frame_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);

	       if (ccode == (ULONG) -1)
		  fl->result = 0;
	       else
	       if (ccode)
		  fl->result = ccode - 1;
	       else
		  fl->result = ccode;

	       if (fl->MenuResult)
		  *(fl->MenuResult) = fl->result;

	       if (fl->MenuPortal)
		  free_menu(fl->MenuPortal);
	       fl->MenuPortal = 0;

	       put_string_to_length(menu_frame[num].screen,
			   fl->MenuStrings[fl->result],
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   FieldAttribute,
			   fl->buflen);
	    }

	    (fl->buflen)
	    ? (fl->buffer[fl->buflen - 1] = '\0')
	    : (fl->buffer[0] = '\0');

	    if (fl == menu_frame[num].tail && (fl->flags & FIELD_ENTRY))
	    {
	       disable_cursor();
	       update_static_portal(num);
	       return 0;
	    }
	    else
	    {
	       if (fl->MenuItems && fl->MenuStrings)
	       {
		  put_string_to_length(menu_frame[num].screen,
			   fl->MenuStrings[fl->result],
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   fl->attr,
			   fl->buflen);
	       }
	       else
		  put_string_to_length(menu_frame[num].screen,
			   fl->buffer,
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   fl->attr,
			   fl->buflen);
	       fl = fl->next;
	       if (!fl)
		  fl = menu_frame[num].head;
	       if (fl->MenuItems && fl->MenuStrings)
	       {
		  put_string_to_length(menu_frame[num].screen,
			   fl->MenuStrings[fl->result],
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   FieldAttribute,
			   fl->buflen);
	       }
	       else
		  put_string_to_length(menu_frame[num].screen,
			   fl->buffer,
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   FieldAttribute,
			   fl->buflen);
	       fl->pos = strlen(fl->buffer);
	       frame_set_xy(num, fl->row, fl->col + fl->plen +
				  fl->pos + 1);
	    }
	    break;

	 case TAB:
	    (fl->buflen)
	    ? (fl->buffer[fl->buflen - 1] = '\0')
	    : (fl->buffer[0] = '\0');

	    if (fl->MenuItems && fl->MenuStrings)
	    {
	       put_string_to_length(menu_frame[num].screen,
			   fl->MenuStrings[fl->result],
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   fl->attr,
			   fl->buflen);
	    }
	    else
	       put_string_to_length(menu_frame[num].screen,
			   fl->buffer,
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   fl->attr,
			   fl->buflen);
	    fl = fl->next;
	    if (!fl)
	       fl = menu_frame[num].head;
	    if (fl->MenuItems && fl->MenuStrings)
	    {
	       put_string_to_length(menu_frame[num].screen,
			   fl->MenuStrings[fl->result],
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   FieldAttribute,
			   fl->buflen);
	    }
	    else
	       put_string_to_length(menu_frame[num].screen,
			   fl->buffer,
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   FieldAttribute,
			   fl->buflen);
	    fl->pos = strlen(fl->buffer);
	    frame_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
	    break;

#if (LINUX_UTIL)
	  case F3:
#else
	  case ESC:
#endif
	    disable_cursor();
	    update_static_portal(num);
	    return 1;

	 default:
	    if (fl->MenuItems && fl->MenuStrings)
	    {
	       len = (fl->MenuItems + 1) / 2;
	       screenRow = fl->row + adj + menu_frame[fl->portal].startRow;
	       if (screenRow >= len)
		  menuRow = screenRow - len;
	       else
		  menuRow = screenRow;

	       len = 0;
	       for (i=0; i < fl->MenuItems; i++)
	       {
		  if (strlen(fl->MenuStrings[i]) > len)
		     len = strlen(fl->MenuStrings[i]);
	       }
	       menuCol = ((ConsoleScreen.nCols - len) / 2);

	       fl->MenuPortal = make_menu(&ConsoleScreen,
						0,
						menuRow,
						menuCol,
						fl->MenuItems,
						BORDER_DOUBLE,
						FieldPopupHighlightAttribute,
						FieldPopupHighlightAttribute,
						FieldPopupNormalAttribute,
						FieldPopupNormalAttribute,
						0,
						0,
						0,
						TRUE,
						0);
	       if (!fl->MenuPortal)
		  break;

	       for (i=0; i < fl->MenuItems; i++)
		  add_item_to_menu(fl->MenuPortal, fl->MenuStrings[i], i + 1);

	       disable_cursor();
	       menu_frame[fl->MenuPortal].choice = fl->result;
	       ccode = activate_menu(fl->MenuPortal);
	       enable_cursor();
	       frame_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);

	       if (ccode == (ULONG) -1)
		  fl->result = 0;
	       else
	       if (ccode)
		  fl->result = ccode - 1;
	       else
		  fl->result = ccode;

	       if (fl->MenuResult)
		  *(fl->MenuResult) = fl->result;

	       if (fl->MenuPortal)
		  free_menu(fl->MenuPortal);
	       fl->MenuPortal = 0;

	       put_string_to_length(menu_frame[num].screen,
			   fl->MenuStrings[fl->result],
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   FieldAttribute,
			   fl->buflen);
	       break;
	    }

	    if (fl->pos < (fl->buflen - 1))
	    {
               if (!insert)
               {
	          fl->buffer[fl->pos] = (BYTE)key;
	          write_portal_char(num, (BYTE)key, fl->row, fl->col + fl->plen +
				       fl->pos + 1, fl->attr);
	          put_char(menu_frame[num].screen,
			 (BYTE)key,
			 menu_frame[num].startRow + adj + fl->row,
			 menu_frame[num].startColumn + 2 + fl->col +
			 fl->plen + fl->pos + 1,
			 FieldAttribute);

	          fl->pos++;
	          frame_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
               }
               else
               {
                  if (strlen(fl->buffer) < (fl->buflen - 1))
                  {
	             for (i = fl->buflen; i > fl->pos; i--)
		        fl->buffer[i] = fl->buffer[i - 1];

	             fl->buffer[fl->pos] = (BYTE)key;
	             write_portal_char(num, (BYTE)key, fl->row, fl->col + fl->plen +
				       fl->pos + 1, fl->attr);
	             put_char(menu_frame[num].screen,
			 (BYTE)key,
			 menu_frame[num].startRow + adj + fl->row,
			 menu_frame[num].startColumn + 2 + fl->col +
			 fl->plen + fl->pos + 1,
			 FieldAttribute);

	             put_string_to_length(menu_frame[num].screen,
			   fl->buffer,
			   menu_frame[num].startRow + adj + fl->row,
			   menu_frame[num].startColumn + 2 + fl->col +
			   fl->plen + 1,
			   FieldAttribute,
			   fl->buflen);

	             fl->pos++;
	             frame_set_xy(num, fl->row, fl->col + fl->plen + fl->pos + 1);
                  }
               }
	    }
	    break;
      }
   }
   return 0;

}

