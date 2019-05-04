/***************************************************************************
*
*   Copyright (c) 1997, 1998 Jeff V. Merkey  All Rights
*			    Reserved.
*
*
*   AUTHOR   :  Darren Major
*   FILE     :  VNCONSOL.H
*   DESCRIP  :  VNDI console files
*   DATE     :  November 3, 1998
*
*
***************************************************************************/

#ifndef _VCONSOLE_H_
#define _VCONSOLE_H_

struct	nwvp_console_def;

typedef	struct	nwvp_console_command_def
{
	struct	nwvp_console_command_def	*next_link;
	struct	nwvp_console_command_def	*last_link;
	BYTE			*command_name;
	void			(*routine)(struct nwvp_console_def *console);
	BYTE			*help_string;
} nwvp_console_command;

typedef	struct	nwvp_console_def
{
	int		screen_handle;
	ULONG	token_count;
	ULONG	quit_flag;
	struct	pmachine_def		*machine_link;
//	struct	pmedia_def		*device_link;
//	struct	vgroup_def		*group_link;
//	struct	vdevice_def		*vdevice_link;

	BYTE	*screen_name;
	BYTE	*prompt;
	struct	nwvp_console_command_def	*command_list_head;

	BYTE	*token_table[20];
	BYTE	token_size_table[20];
	BYTE	token_delimitor_table[20];
	ULONG	token_value_table[20];
	BYTE	command_line[100];
} nwvp_console;

int	nwvp_create_console(
	nwvp_console *console,
	char *name);

int	nwvp_delete_console(
	nwvp_console	*console);

int	nwvp_add_console_command(
	nwvp_console	*console,
	BYTE	*name,
	void	(*routine)(nwvp_console *),
	BYTE	*help_string);

int	nwvp_start_console(
	nwvp_console	*console);

void    nwvp_add_commands(
	nwvp_console            *console);

void    nwvp_init_command_strings(void);


#endif

