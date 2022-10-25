
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Jeff V. Merkey  All Rights
*                           Reserved.
*
*   AUTHOR   :  
*   FILE     :  VNDIMAIN.C
*   DESCRIP  :  VNDI
*   DATE     :  November 3, 1998
*
*
***************************************************************************/

#include "globals.h"
#if !MANOS
#include "vconsole.h"
#include "stdio.h"
#include "stdlib.h"

extern ULONG consoleHandle;
extern	void	(*poll_routine)(void);

void    console_clear_routine(
	nwvp_console    *console)
{
	  ClearScreen(consoleHandle);
}

#else
void    console_clear_routine(
	nwvp_console    *console)
{
	vndi_clear_screen(console->screen_handle);
}

#endif

BYTE    quit_string[] = {"XQUIT"};
BYTE    quit_help_string[] = {"Xexit this console"};
BYTE    help_string[] = {"XHELP"};
BYTE    prompt_string[] = {"XVNDI: "};

BYTE    clear_string[] = {"XCLS"};
BYTE    clear_help_string[] = {"Xclear console screen"};
void    quit_routine(
	nwvp_console    *console)
{
	console->quit_flag = -1;
}

void    help_routine(
	nwvp_console    *console)
{
 	nwvp_console_command    *command;
        int count = 0;

	command = console->command_list_head;
//	nwvp_print(console->screen_handle, " \n", 2 , 7);
	NWFSPrint(" \n");
	do
	{
		if (command->help_string != 0)
		{
			NWFSPrint(&command->command_name[1]);
			NWFSPrint(" --> ");
			NWFSPrint(&command->help_string[1]);
			NWFSPrint(" \n");
//			NWFSPrint("%s --> %s\n", &command->command_name[1], &command->help_string[1]);
                        if (count++ > 20)                         
                        {
                           NWFSPrint("--- more ---");
                           getc(stdin); 
                           count = 0;
                        }
		}
		command = command->next_link;
	} while (command != console->command_list_head);
}

int     nwvp_add_console_command(
	nwvp_console    *console,
	BYTE    *name,
	void    (*routine)(nwvp_console *),
	BYTE    *help_string)
{
	nwvp_console_command    *command, *head;

	if (nwvp_alloc((void *) &command, sizeof(nwvp_console_command)) == 0)
	{
		nwvp_memset(command, 0, sizeof(nwvp_console_command));
		command->command_name = name;
		command->help_string = help_string;
		command->routine = routine;
		if ((head = console->command_list_head) == 0)
		{
			command->next_link = command;
			command->last_link = command;
			console->command_list_head = command;
		}
		else
		{
			command->last_link = head->last_link;
			command->next_link = head;
			head->last_link->next_link = command;
			head->last_link = command;
		}
		return(0);
	}
	return(-1);
}

int     nwvp_create_console(
	nwvp_console *console,
	char *name)
{
	quit_string[0] = sizeof(quit_string) - 2;
	quit_help_string[0] = sizeof(quit_help_string) - 2;
	help_string[0] = sizeof(help_string) - 2;
	prompt_string[0] = sizeof(prompt_string) -2 ;
	clear_string[0] = sizeof(clear_string) - 2;
	clear_help_string[0] = sizeof(clear_help_string) - 2;
	nwvp_memset((void *) console, 0, sizeof(nwvp_console));
	console->screen_name = name;
	console->prompt = name;
#if MANOS
	nwvp_create_screen(&console->screen_handle, &name[1]);
#endif
	nwvp_add_console_command(console, &clear_string[0], console_clear_routine, &clear_help_string[0]);
	nwvp_add_console_command(console, &quit_string[0], quit_routine, &quit_help_string[0]);
	nwvp_add_console_command(console, &help_string[0], help_routine, 0);
	return(0);
}

int     nwvp_delete_console(
	nwvp_console    *console)
{
	nwvp_console_command    *command;

	while ((command = console->command_list_head) != 0)
	{
		if (command->next_link != command)
		{
			console->command_list_head = command->next_link;
			command->next_link->last_link = command->last_link;
			command->last_link->next_link = command->next_link;
		}
		else
			console->command_list_head = 0;
		nwvp_free(command);
	}
#if MANOS
	nwvp_destroy_screen(console->screen_handle);
#endif
	return(0);
}

ULONG	command_execute(
	nwvp_console	*console,
	BYTE			*command_string)
{
	ULONG	i;
	nwvp_console_command	*command;

	if ((command = console->command_list_head) != 0)
	{

		do
		{
			for (i=0; i<command->command_name[0]; i++)
			{
				if (command->command_name[i+1] != command_string[i])
					break;
			}
			if (i == command->command_name[0])
			{
				command->routine(console);
				return(0);
			}
			command = command->next_link;
		} while (command != console->command_list_head);
	}
	return(-1);

}

void	get_command_line(
	ULONG		handle,
	BYTE		*command_line)
{
#if MANOS
	nwvp_input(handle, command_line, 80);
#else

#if (0 & DOS_UTIL)
    extern int getch(void);

    ULONG index = 0;
    while (1)
    {
       if (kbhit() == 0)
       {
          if (poll_routine != 0)
          {
             poll_routine();
          }
       }
       else
       {
          command_line[index] = getch();
	  if (command_line[index] < 0x20)
	  {
	     if ((command_line[index] == 0x0A) || (command_line[index] == 0x0D))
	     {
		NWFSPrint("\n");
		return;
	     }

	     if ((command_line[index] == 0x08) && (index > 0))
	     {
		NWFSPrint("%c", command_line[index]);
		NWFSPrint(" ");
		NWFSPrint("%c", command_line[index]);
		command_line[index] = 0;
		index--;
	     }
	  }
	  else
	  {
	     NWFSPrint("%c", command_line[index]);
	     index ++;
	  }
       }
    }
#else
    fgets(command_line, 100, stdin);
#endif
#endif
}

int     nwvp_start_console(
	nwvp_console    *console)
{
	BYTE            *bptr;
	ULONG            token_flag;
	ULONG            i;
	ULONG			open_paren;

#if !MANOS
	ClearScreen(consoleHandle);
#else
	vndi_clear_screen(console->screen_handle);
#endif
	while (console->quit_flag == 0)
	{
		bptr = &console->command_line[0];
		token_flag = 0;
		console->token_count = 0;
		for (i=0; i<16; i++)
		{
			console->token_table[i] = 0;
			console->token_size_table[i] = 0;
			console->token_delimitor_table[i] = 0;
		}
		NWFSPrint(&console->prompt[1]);
		NWFSPrint(": ");
		get_command_line(console->screen_handle, bptr);
		open_paren = 0;
		while ((*bptr != 0) && (*bptr != 0xA) && (*bptr != 0xD))
		{
			if (*bptr == '"')
			{
				if (open_paren == 0)
					open_paren = 1;
				else
					open_paren = 0;

				if (token_flag != 0)
				{
					console->token_delimitor_table[console->token_count] = *bptr;
					nwvp_ascii_to_int(&console->token_value_table[console->token_count], console->token_table[console->token_count]);
					console->token_count ++;
					token_flag = 0;
					*bptr = 0;
				}
			}
			else
			{
				if ((open_paren != 0) ||
					((*bptr != ' ') && (*bptr != '+') && (*bptr != '-') && (*bptr != '=') && (*bptr != ',')))
				{
					if ((*bptr >= 'a') && (*bptr <= 'z'))
						*bptr -= ('a' - 'A');
					if (token_flag == 0)
					{
						token_flag = -1;
						console->token_table[console->token_count] = bptr;
					}
					console->token_size_table[console->token_count]++;
				}
				else
				{
					if (token_flag != 0)
					{
						console->token_delimitor_table[console->token_count] = *bptr;
						nwvp_ascii_to_int(&console->token_value_table[console->token_count], console->token_table[console->token_count]);
						console->token_count ++;
						token_flag = 0;
						*bptr = 0;

					}
				}
			}
			bptr ++;
		}
		if (token_flag != 0)
		{
			console->token_delimitor_table[console->token_count] = *bptr;
			nwvp_ascii_to_int(&console->token_value_table[console->token_count], console->token_table[console->token_count]);
			console->token_count ++;
			*bptr = 0;
		}
		if (console->token_count > 0)
		{
//			for (i=0; i<console->token_size_table[0]; i++)
//				console->token_table[0][i] = toupper(console->token_table[0][i]);
			if (command_execute(console, console->token_table[0]) != 0)
//				nwvp_print(console->screen_handle, "\n invalid command \n", 19, 7);
				printf("\n invalid command \n");
		}
	}
    return(0);
}
