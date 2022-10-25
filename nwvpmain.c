/***************************************************************************
*
*   Copyright (c) 1997, 1998 Jeff V. Merkey  All Rights
*			    Reserved.
*
*   AUTHOR   :  
*   FILE     :  NWVPMAIN.C
*   DESCRIP  :  NWVPMAIN
*   DATE     :  November 3, 1998
*
*
***************************************************************************/

#include "globals.h"
#if !MANOS
#include "vconsole.h"
#else
#include "lru.h"
#endif
extern	ULONG	consoleHandle;

extern	ULONG	current_vdevice;

int	main(void)
{
	nwvp_console	*console;

#if (LINUX_UTIL)
	sync();
#endif
//	mem_check_init();
	nwvp_alloc((void **) &console, sizeof(nwvp_console));
	nwvp_create_console(console, "\x4NWVP");
	nwvp_add_commands(console);
#if MANOS
    InitializeLRU();
#endif
	nwvp_start_console(console);
#if MANOS
    FreeLRU();
#endif
	nwvp_unscan_routine();
	nwvp_delete_console(console);
	nwvp_free(console);
//	RemoveDiskDevices();
//	FreePartitionResources();
//    ClearScreen(consoleHandle);

//	mem_check_uninit();
	return(0);
}
