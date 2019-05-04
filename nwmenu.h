
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
*   FILE     :  NWMENU.H
*   DESCRIP  :  FENRIS Menu Lib
*   DATE     :  November 29, 1999
*
*
***************************************************************************/

#ifndef _NWMENU_
#define _NWMENU_

//
//  keyboard scan code values after translation
//

#if (MANOS)

#define   F1           0x0F
#define   F2           0x10
#define   F3           0x11
#define   F4           0x12
#define   F5           0x13
#define   F6           0x14
#define   F7           0x15
#define   F8           0x16
#define   F9           0x17
#define   F10          0x18
#define   F11          0x19
#define   F12          0x1A
#define   INS          0x0E
#define   DEL          0x7F
#define   ENTER        0x0D
#define   ESC          0x1B
#define   BKSP         0x08
#define   SPACE        0x20
#define   TAB          0x09
#define   HOME         0x06
#define   END          0x0B
#define   PG_UP        0x05
#define   PG_DOWN      0x0C
#define   UP_ARROW     0x01
#define   DOWN_ARROW   0x02
#define   LEFT_ARROW   0x03
#define   RIGHT_ARROW  0x04

#endif

#if (DOS_UTIL | WINDOWS_NT_UTIL)

#define   F1           0x13B
#define   F2           0x13C
#define   F3           0x13D
#define   F4           0x13E
#define   F5           0x13F
#define   F6           0x140
#define   F7           0x141
#define   F8           0x142
#define   F9           0x143
#define   F10          0x144
#define   F11          0x185
#define   F12          0x186
#define   INS          0x152
#define   DEL          0x153
#define   ENTER        0x0D
#define   ESC          0x1B
#define   BKSP         0x08
#define   SPACE        0x20
#define   TAB          0x09
#define   HOME         0x147
#define   END          0x14F
#define   PG_UP        0x149
#define   PG_DOWN      0x151
#define   UP_ARROW     0x148
#define   DOWN_ARROW   0x150
#define   LEFT_ARROW   0x14B
#define   RIGHT_ARROW  0x14D

#endif

#if (LINUX_UTIL)

#define   F1           0x109
#define   F2           0x10A
#define   F3           0x10B
#define   F4           0x10C
#define   F5           0x10D
#define   F6           0x10E
#define   F7           0x10F
#define   F8           0x110
#define   F9           0x111
#define   F10          0x112
#define   F11          0x113
#define   F12          0x114
#define   INS          0x14B
#define   DEL          0x14A
#define   ENTER        0x0D
#define   ESC          0x1B
#define   BKSP         0x107
#define   SPACE        0x20
#define   TAB          0x09
#define   HOME         0x106
#define   END          0x168
#define   PG_UP        0x153
#define   PG_DOWN      0x152
#define   UP_ARROW     0x103
#define   DOWN_ARROW   0x102
#define   LEFT_ARROW   0x104
#define   RIGHT_ARROW  0x105

#endif

#define BLINK		0x80
#define	BLACK		0x00
#define BLUE		0x01
#define GREEN		0x02
#define CYAN		0x03
#define RED		0x04
#define MAGENTA		0x05
#define BROWN		0x06
#define WHITE		0x07
#define	GRAY		0x08
#define LTBLUE		0x09
#define LTGREEN		0x0A
#define LTCYAN		0x0B
#define LTRED		0x0C
#define LTMAGENTA	0x0D
#define YELLOW		0x0E
#define BRITEWHITE	0x0F
#define	BGBLACK		0x00
#define BGBLUE		0x10
#define BGGREEN		0x20
#define BGCYAN		0x30
#define BGRED		0x40
#define BGMAGENTA	0x50
#define BGBROWN		0x60
#define BGWHITE		0x70

#define UP_CHAR         0x1E
#define DOWN_CHAR       0x1F

#define UP              1
#define DOWN            0
#define MAX_MENU        100

#define BORDER_SINGLE   1
#define BORDER_DOUBLE   2

typedef struct _NWSCREEN
{
   BYTE *pVidMem;	 // pointer to crnt video buffer
   BYTE *pSaved;
   ULONG CrntRow;		 // Current cursor position
   ULONG CrntColumn;
   ULONG nCols;		 // Virtual Screen Size
   ULONG nLines;
   ULONG VidMode;	 // 0x00 = 80x25 VGA color text
   ULONG NormVid;	 // 0x07 = WhiteOnBlack
   ULONG RevVid;	 // 0x71 = RevWhiteOnBlack
   ULONG TabSize;
} NWSCREEN;

typedef struct _FIELD_LIST
{
   struct _FIELD_LIST *next;
   struct _FIELD_LIST *prior;
   ULONG portal;
   ULONG row;
   ULONG col;
   BYTE *prompt;
   ULONG plen;
   BYTE *buffer;
   ULONG buflen;
   ULONG flags;
   ULONG attr;
   ULONG pos;
   ULONG result;
   BYTE **MenuStrings;
   ULONG MenuItems;
   ULONG MenuPortal;
   ULONG *MenuResult;
} FIELD_LIST;

typedef struct _MENU_FRAME
{
   ULONG num;
   BYTE *elementStorage;
   BYTE *p;
   BYTE **elementStrings;
   ULONG *elementValues;
   ULONG startRow;
   ULONG endRow;
   ULONG startColumn;
   ULONG endColumn;
   ULONG curRow;
   ULONG curColumn;
   ULONG pcurRow;
   ULONG pcurColumn;
   ULONG windowSize;
   ULONG border;
   ULONG active;
   ULONG header_color;
   ULONG border_color;
   ULONG fill_color;
   ULONG text_color;
   BYTE header[80];
   BYTE header2[80];
   NWSCREEN *screen;
   ULONG elementCount;
   ULONG elementLimit;
   ULONG owner;
   ULONG (*elementFunction)(NWSCREEN *, ULONG, BYTE *, ULONG);
   ULONG (*warnFunction)(NWSCREEN *, ULONG);
   long choice;
   long index;
   long top;
   long bottom;
   ULONG selected;
   ULONG (*keyboardHandler)(NWSCREEN *, ULONG, ULONG);
   ULONG keyboardMask;
   ULONG screenMode;
   ULONG nLines;
   ULONG scrollBar;

   // window border characters
   int UpperLeft;
   int UpperRight;
   int LowerLeft;
   int LowerRight;
   int LeftFrame;
   int RightFrame;
   int VerticalFrame;
   int HorizontalFrame;
   int UpChar;
   int DownChar;
   int ScrollFrame;
   FIELD_LIST *head;
   FIELD_LIST *tail;
   ULONG field_count;
} MENU_FRAME;


extern ULONG BarAttribute;
extern ULONG FieldAttribute;
extern ULONG FieldPopupHighlightAttribute;
extern ULONG FieldPopupNormalAttribute;
extern ULONG ErrorAttribute;

//
//   hal functions
//

void ScreenWrite(BYTE *p);
ULONG InitConsole(void);
ULONG ReleaseConsole(void);
void copy_data(ULONG *src, ULONG *dest, ULONG len);
void set_data(ULONG *dest, ULONG value, ULONG len);
void set_data_b(BYTE *dest, BYTE value, ULONG len);
void hard_xy(ULONG row, ULONG col);
ULONG get_key(void);
void set_xy(NWSCREEN *screen, ULONG row, ULONG col);
void get_xy(NWSCREEN *screen, ULONG *row, ULONG *col);
void clear_screen(NWSCREEN *screen);
void move_string(NWSCREEN *screen,
		 ULONG srcRow, ULONG srcCol,
		 ULONG destRow, ULONG destCol,
		 ULONG length);
void put_string(NWSCREEN *screen, BYTE *s, ULONG row, ULONG col, ULONG attr);
void put_string_transparent(NWSCREEN *screen, BYTE *s, ULONG row, ULONG col, ULONG attr);
void put_string_cleol(NWSCREEN *screen, BYTE *s, ULONG line, ULONG attr);
void put_string_to_length(NWSCREEN *screen, BYTE *s, ULONG row,
			  ULONG col, ULONG attr, ULONG len);
void put_char(NWSCREEN *screen, int c, ULONG row, ULONG col, ULONG attr);
int get_char(NWSCREEN *screen, ULONG row, ULONG col);
int get_char_attribute(NWSCREEN *screen, ULONG row, ULONG col);
void put_char_cleol(NWSCREEN *screen, int c, ULONG line, ULONG attr);
void put_char_length(NWSCREEN *screen, int c, ULONG row, ULONG col, ULONG attr,
		     ULONG len);
ULONG scroll_display(NWSCREEN *screen, ULONG row, ULONG col,
		   ULONG cols, ULONG lines, ULONG up);

//
//  menu functions
//

void scroll_menu(ULONG num, ULONG up);
ULONG get_resp(ULONG num);
ULONG fill_menu(ULONG num, ULONG ch, ULONG attr);
ULONG save_menu(ULONG num);
ULONG restore_menu(ULONG num);
ULONG free_menu(ULONG num);
ULONG display_menu_header(ULONG num);
ULONG draw_menu_border(ULONG num);
void display_menu(ULONG num);
ULONG add_item_to_menu(ULONG num, BYTE *item, ULONG value);
ULONG activate_menu(ULONG num);
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
	       ULONG MaxElements);
ULONG menu_write_string(ULONG num, BYTE *p, ULONG row, ULONG col, ULONG attr);

//
//  portal functions
//

void scroll_portal(ULONG num, ULONG up);
ULONG get_portal_resp(ULONG num);
ULONG free_portal(ULONG num);
ULONG add_item_to_portal(ULONG num, BYTE *item, ULONG index);
ULONG display_portal_header(ULONG num);
ULONG draw_portal_border(ULONG num);
void display_portal(ULONG num);
ULONG update_portal(ULONG num);
ULONG update_static_portal(ULONG num);
ULONG activate_portal(ULONG num);
ULONG activate_static_portal(ULONG num);
ULONG deactivate_static_portal(ULONG num);
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
		 ULONG scrollBarPresent);
ULONG set_portal_limit(ULONG num, ULONG limit);
ULONG write_portal(ULONG num, BYTE *p, ULONG row, ULONG col, ULONG attr);
ULONG write_portal_char(ULONG num, BYTE p, ULONG row, ULONG col, ULONG attr);
ULONG write_portal_cleol(ULONG num, BYTE *p, ULONG row, ULONG col, ULONG attr);
ULONG write_screen_comment_line(NWSCREEN *screen, BYTE *p, ULONG attr);
ULONG disable_portal_input(ULONG num);
ULONG enable_portal_input(ULONG num);
ULONG clear_portal(ULONG num);
ULONG error_portal(BYTE *p, ULONG row);
ULONG confirm_menu(BYTE *confirm, ULONG row, ULONG attr);
ULONG input_portal_fields(ULONG num);

//
//  field flags
//

#define FIELD_ENTRY   0x00000001

ULONG add_field_to_portal(ULONG num, ULONG row, ULONG col, ULONG attr,
			BYTE *prompt, ULONG plen,
			BYTE *buffer, ULONG buflen,
			BYTE **MenuStrings, ULONG MenuItems,
			ULONG MenuDefault, ULONG *MenuResult,
			ULONG flags);
ULONG message_portal(BYTE *p, ULONG row, ULONG attr, ULONG wait);
ULONG create_message_portal(BYTE *p, ULONG row, ULONG attr);
ULONG close_message_portal(ULONG portal);
ULONG write_portal_header1(ULONG num, BYTE *p, ULONG attr);
ULONG write_portal_header2(ULONG num, BYTE *p, ULONG attr);

#endif
