/***************************************************************************
*
*   Copyright (c) 1997, 1998 Jeff V. Merkey  All Rights
*			    Reserved.
*
*   AUTHOR   :  Doug Ward
*   FILE     :  NWVPCMD.H
*   DESCRIP  :  NWVP command function prototypes and datastructures
*   DATE     :  May 17, 1998
*
*
***************************************************************************/

#ifndef NWVPCMD_H
#define NWVPCMD_H

//=====================================
//	PROTOTYPES
//=====================================

void    raw_create_routine(
	vndi_console    *console);

void    raw_delete_routine(
	vndi_console    *console);

void    raw_online_routine(
	vndi_console    *console);

void    raw_offline_routine(
	vndi_console    *console);

void    raw_scan_routine(
	vndi_console    *console);

void    raw_scana_routine(
	vndi_console    *console);

void    raw_show_error_routine(
	vndi_console    *console);

void    raw_scans_routine(
	vndi_console    *console);

void    raw_error_routine(
	vndi_console    *console);

void    raw_clear_error_routine(
	vndi_console    *console);

void    raw_read_routine(
	vndi_console    *console);

void    raw_write_routine(
	vndi_console    *console);

void    display_buffer_routine(
	vndi_console            *console);

void    set_buffer_routine(
	vndi_console            *console);

void 	blank_help_routine (
	vndi_console		*console);

void    vndi_add_commands(
	vndi_console            *console);

BOOL	vndi_validate_params( 
	vndi_console		*console,
	int32			min_number_of_params,
	int32			max_number_of_params,
	char 			*param_types,
	void			(*help_routine)(vndi_console *));

void    do_nothing(
	vndi_console		*console);

#endif
