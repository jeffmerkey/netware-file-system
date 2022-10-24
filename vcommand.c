
/***************************************************************************
*
*   Copyright (c) 1997, 1998, 1999 Jeff V. Merkey  All Rights
*                           Reserved.
*
*   This program is an unpublished work of TRG, Inc. and contains
*   trade secrets and other proprietary information.  Unauthorized
*   use, copying, disclosure, or distribution of this file without the
*   consent of TRG, Inc. can subject the offender to severe criminal
*   and/or civil penalities.
*
*
*   AUTHOR   :  
*   FILE     :  VCOMMAND.C
*   DESCRIP  :  VNDI OS files
*   DATE     :  November 30, 1998
*
*
***************************************************************************/

#include "globals.h"
#if !MANOS
#include "vconsole.h"
#else
#include "lru.h"
#endif

#define nwvp_sprintf       sprintf

ULONG   rand_seed = 0x7777;
ULONG	test_random(
	ULONG	modulo)
{
	rand_seed = (1664525 * (rand_seed)) + 1013904223;
	return(rand_seed % modulo);
}


int     nwvp_add_console_command(
	nwvp_console    *console,
	BYTE    *name,
	void    (*routine)(nwvp_console *),
	BYTE    *help_string);

BYTE    vfill_string[] = {"XVFILL"};
BYTE    vfill_help_string[] = {"Xfill virtual partition"};
BYTE    vcheck_string[] = {"XVCHECK"};
BYTE    vcheck_help_string[] = {"Xcheck virtual partition"};
BYTE    lfill_string[] = {"XLFILL"};
BYTE    lfill_help_string[] = {"Xfill logical partition"};
BYTE    lcheck_string[] = {"XLCHECK"};
BYTE    lcheck_help_string[] = {"Xcheck logical partition"};
BYTE    scan_string[] = {"XSCAN"};
BYTE    scan_help_string[] = {"Xscan partitions"};
BYTE    rscan_string[] = {"XRSCAN"};
BYTE    rscan_help_string[] = {"Xscan partitions without evaluation"};
BYTE    unscan_string[] = {"XUNSCAN"};
BYTE    unscan_help_string[] = {"X unscan raw partitions"};
BYTE    lformat_string[] = {"XLFORMAT"};
BYTE    lformat_help_string[] = {"Xformat raw partitions"};
BYTE    lunformat_string[] = {"XLUNFORMAT"};
BYTE    lunformat_help_string[] = {"Xunformat logical partitions"};
BYTE    vcreate_string[] = {"XVCREATE"};
BYTE    vcreate_help_string[] = {"Xcreate virtual partitions"};
BYTE    vadd_segment_string[] = {"XVADD"};
BYTE    vadd_segment_help_string[] = {"X add segment to virtual volume"};
//BYTE    vfat_check_string[] = {"XVFATCHECK"};
//BYTE    vfat_check_help_string[] = {"X check volume fats"};
BYTE    vuncreate_string[] = {"XVUNCREATE"};
BYTE    vuncreate_help_string[] = {"Xuncreate virtual partitions"};
//BYTE    volfix_string[] = {"XVUNCREATE"};
//BYTE    volfix_help_string[] = {"Xuncreate virtual partitions"};
BYTE    ladd_mirror_string[] = {"XLADDMIRROR"};
BYTE    ladd_mirror_help_string[] = {"Xadd mirror to group"};
BYTE    ldel_mirror_string[] = {"XLDELMIRROR"};
BYTE    ldel_mirror_help_string[] = {"Xdelete mirror from group"};
BYTE    rlist_string[] = {"XRLIST"};
BYTE    rlist_help_string[] = {"X list raw partitions"};
BYTE    llist_string[] = {"XLLIST"};
BYTE    llist_help_string[] = {"X list logical partitions"};
BYTE    lspace_string[] = {"XLSPACE"};
BYTE    lspace_help_string[] = {"X display logical partition space"};
BYTE    lactivate_string[] = {"XLACTIVATE"};
BYTE    lactivate_help_string[] = {"X activate logical partition"};
BYTE    lremirror_string[] = {"XLREMIRROR"};
BYTE    lremirror_help_string[] = {"X remirror logical partition "};
BYTE    labort_string[] = {"XLABORT"};
BYTE    labort_help_string[] = {"X abort remirror of logical partition "};
BYTE    lfatfind_string[] = {"XLFATFIND"};
BYTE    lfatfind_help_string[] = {"X remirror logical partition space"};
BYTE    vlist_string[] = {"XVLIST"};
BYTE    vlist_help_string[] = {"X display virtual partitions"};
BYTE    vspace_string[] = {"XVSPACE"};
BYTE    vspace_help_string[] = {"X virtual partition segment info"};
BYTE    vhotfix_string[] = {"XVHOTFIX"};
BYTE    vhotfix_help_string[] = {"X hotfix virtual partition block"};
BYTE    vfatfix_string[] = {"XVFATFIX"};
BYTE    vfatfix_help_string[] = {"X fix virtual partition fat tables"};
BYTE    vfatcheck_string[] = {"XVFATCHECK"};
BYTE    vfatcheck_help_string[] = {"X check virtual partition fat tables"};
BYTE    vmap_string[] = {"XVMAP"};
BYTE    vmap_help_string[] = {"X map virtual partition sectors to physical sectors"};
BYTE    vopen_string[] = {"XVOPEN"};
BYTE    vopen_help_string[] = {"X open virtual partition"};
BYTE    vclose_string[] = {"XVCLOSE"};
BYTE    vclose_help_string[] = {"X close virtual partition "};
BYTE    lread_string[] = {"XLREAD"};
BYTE    lread_help_string[] = {"Xread logical partition"};
BYTE    lwrite_string[] = {"XLWRITE"};
BYTE    lwrite_help_string[] = {"Xwrite logical partition"};
BYTE    vread_string[] = {"XVREAD"};
BYTE    vread_help_string[] = {"Xread virtual partition"};
BYTE    vwrite_string[] = {"XVWRITE"};
BYTE    vwrite_help_string[] = {"Xwrite virtual partition"};
BYTE    rread_string[] = {"XRREAD"};
BYTE    rread_help_string[] = {"Xread raw partition"};
BYTE    rwrite_string[] = {"XRWRITE"};
BYTE    rwrite_help_string[] = {"Xwrite raw partition"};
BYTE    display_string[] = {"XD"};
BYTE    display_help_string[] = {"Xdisplay buffer in BYTES"};
BYTE    ddisplay_string[] = {"XDD"};
BYTE    ddisplay_help_string[] = {"Xdisplay buffer in DWORDS"};
BYTE    set_string[] = {"XSET"};
BYTE    set_help_string[] = {"Xset value in memory buffer"};
BYTE    cbyte_string[] = {"XCBYTE"};
BYTE    cbyte_help_string[] = {"XChange byte value in memory buffer"};
BYTE    clong_string[] = {"XCULONG"};
BYTE    clong_help_string[] = {"XChange long value in memory buffer"};
#if MANOS
BYTE    vstart_test_string[] = {"XVSTART"};
BYTE    vstart_test_help_string[] = {"Xstart vpartition test"};
BYTE    vstop_test_string[] = {"XVSTOP"};
BYTE    vstop_test_help_string[] = {"Xstop vpartition test"};
BYTE    lmirror_test_string[] = {"XLMIRRORTEST"};
BYTE    lmirror_test_help_string[] = {"Xtest mirror group"};
BYTE    lru_lock_string[] = {"XLRULOCK"};
BYTE    lru_lock_help_string[] = {"Xlock lru buffer"};
BYTE    lru_release_string[] = {"XLRUREL"};
BYTE    lru_release_help_string[] = {"Xrelease lru buffer"};
BYTE    lru_dirty_string[] = {"XLRUDIRTY"};
BYTE    lru_dirty_help_string[] = {"Xrelease and dirty lru buffer"};
BYTE    lru_display_string[] = {"XLRUDISPLAY"};
BYTE    lru_display_help_string[] = {"Xdisplay lru data"};
#endif

BYTE	global_data_buffer[4096];

void    cbyte_routine(
	nwvp_console    *console)
{
	if (console->token_value_table[1] < 4096)
	{
		global_data_buffer[console->token_value_table[1]] = (BYTE)console->token_value_table[2];
	}
}

void    clong_routine(
	nwvp_console    *console)
{
	if (console->token_value_table[1] < 4095)
	{
		*(ULONG *) &global_data_buffer[console->token_value_table[1]] = console->token_value_table[2];
	}
}

void    scan_routine(
	nwvp_console    *console)
{
	NWFSPrint("scan physical partitions \n");
	nwvp_scan_routine(0);
}

void    rscan_routine(
	nwvp_console    *console)
{
	NWFSPrint("scan only raw partitions \n");
	nwvp_scan_routine(1);
}

void    unscan_routine(
	nwvp_console    *console)
{
	NWFSPrint("scan physical partitions \n");
	nwvp_unscan_routine();
}


void    vcheck_routine(
	nwvp_console    *console)
{
	ULONG		i;
	ULONG		value;
	BYTE		temp[16];
	for (i=0; i<console->token_value_table[3]; i++)
	{
		value = console->token_value_table[4] + console->token_value_table[2] + i;
		if (nwvp_vpartition_block_read(
			console->token_value_table[1],
			i + console->token_value_table[2],
			1,
			&global_data_buffer[0]) != 0)
		{
			NWFSPrint("check io error at block ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(console->token_value_table[2], 16, &temp[0]);
			NWFSPrint(&temp[0]);
			NWFSPrint("\n");
			return;
		}
		if (*(ULONG *) &global_data_buffer[0] != value)
		{
			NWFSPrint("check value error at block ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(console->token_value_table[2], 16, &temp[0]);
			NWFSPrint(&temp[0]);
			NWFSPrint("\n");
			return;
		}
	}
	NWFSPrint("fill complete  \n");
}

void    vfill_routine(
	nwvp_console    *console)
{
	ULONG		i;
	ULONG		value;
	BYTE		temp[16];
	for (i=0; i<console->token_value_table[3]; i++)
	{
		value = console->token_value_table[4] + console->token_value_table[2] + i;
		nwvp_memset(global_data_buffer, value, 4096);
		if (nwvp_vpartition_block_write(
			console->token_value_table[1],
			i + console->token_value_table[2],
			1,
			&global_data_buffer[0]) != 0)
		{
			NWFSPrint("fill error at block ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(console->token_value_table[2], 16, &temp[0]);
			NWFSPrint(&temp[0]);
			NWFSPrint("\n");
			return;
		}
	}
	NWFSPrint("fill complete  \n");
}


void    vread_routine(
	nwvp_console    *console)
{

	if (console->token_count > 2)
	{
		if (nwvp_vpartition_block_read(
			console->token_value_table[1],
			console->token_value_table[2],
			1,
			&global_data_buffer[0]) == 0)
		{
			NWFSPrint("read successful \n");
			return;
		}
	}
	NWFSPrint("VREAD VPARTITION_ID BLOCK_NUMBER  \n");
}


void    vwrite_routine(
	nwvp_console    *console)
{
	if (console->token_count > 2)
	{
		if (nwvp_vpartition_block_write(
				console->token_value_table[1],
				console->token_value_table[2],
				1,
				&global_data_buffer[0]) == 0)
		{
			NWFSPrint("write successful \n");
			return;
		}
	}
	NWFSPrint("VWRITE VPARTITION_ID BLOCK_NUMBER  \n");
}

void    vmap_routine(
	nwvp_console    *console)
{
	ULONG	rpart_handle;
	ULONG	sector_offset;
    ULONG	sector_count;
	BYTE	temp[16];

	if (console->token_count > 2)
	{
		if (nwvp_map_volume_physical(
			console->token_value_table[1],
			console->token_value_table[2],
			console->token_value_table[3],
			(void **) &rpart_handle,
			&sector_offset,
			&sector_count) == 0)
		{

			NWFSPrint("\n RPART_HANDLE = ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(rpart_handle, 16, &temp[0]);
			NWFSPrint(&temp[0]);
			NWFSPrint("\n SECTOR OFFSET = ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(sector_offset, 16, &temp[0]);
			NWFSPrint(&temp[0]);
			NWFSPrint("\n SECTOR COUNT = ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(sector_count, 16, &temp[0]);
			NWFSPrint(&temp[0]);
			NWFSPrint("\n");
			return;
		}
	}
	NWFSPrint("VMAP VPARTITION_ID SECTOR_NUMBER SECTOR_COUNT  \n");
}

void    vfatfix_routine(
	nwvp_console    *console)
{
    nwvp_fat_fix_info   fix_info;

	if (nwvp_vpartition_fat_fix(console->token_value_table[1], &fix_info, 1) == 0)
	{
		NWFSPrint("fatfix successful \n");
		return;
	}
	NWFSPrint("VFATFIX VPARTITION_ID  \n");
}

void    vfatcheck_routine(
	nwvp_console    *console)
{
    nwvp_fat_fix_info   fix_info;
    BYTE                buffer[80];

	if (nwvp_vpartition_fat_fix(console->token_value_table[1], &fix_info, 0) == 0)
	{
        NWFSPrint("\n");
	nwvp_sprintf(&buffer[0], "          total fat blocks = %d \n", (int) fix_info.total_fat_blocks);
	NWFSPrint(&buffer[0]);
	nwvp_sprintf(&buffer[0], " total valid mirror blocks = %d \n", (int) fix_info.total_valid_mirror_blocks);
	NWFSPrint(&buffer[0]);
	nwvp_sprintf(&buffer[0], " total valid single blocks = %d \n", (int) fix_info.total_valid_single_blocks);
	NWFSPrint(&buffer[0]);
	nwvp_sprintf(&buffer[0], "      total missing blocks = %d \n", (int) fix_info.total_missing_blocks);
	NWFSPrint(&buffer[0]);
	nwvp_sprintf(&buffer[0], "     total mismatch blocks = %d \n\n", (int) fix_info.total_mismatch_blocks);
	NWFSPrint(&buffer[0]);
	nwvp_sprintf(&buffer[0], "         total FAT entries = %d \n\n", (int) fix_info.total_entries);
	NWFSPrint(&buffer[0]);

	nwvp_sprintf(&buffer[0], "             total index mismatch entries = %d \n", (int) fix_info.total_index_mismatch_entries);
	NWFSPrint(&buffer[0]);
	nwvp_sprintf(&buffer[0], "       total end or null mismatch entries = %d \n", (int) fix_info.total_end_null_entries);
	NWFSPrint(&buffer[0]);
	nwvp_sprintf(&buffer[0], " total sub-alloc or null mismatch entries = %d \n", (int) fix_info.total_SA_null_entries);
	NWFSPrint(&buffer[0]);
	nwvp_sprintf(&buffer[0], "       total FAT or null mismatch entries = %d \n", (int) fix_info.total_FAT_null_entries);
	NWFSPrint(&buffer[0]);
	nwvp_sprintf(&buffer[0], "  total sub-alloc or end mismatch entries = %d \n", (int) fix_info.total_SA_end_entries);
	NWFSPrint(&buffer[0]);
	nwvp_sprintf(&buffer[0], "  total sub-alloc or FAT mismatch entries = %d \n", (int) fix_info.total_SA_FAT_entries);
	NWFSPrint(&buffer[0]);
	nwvp_sprintf(&buffer[0], "         total sub-alloc mismatch entries = %d \n", (int) fix_info.total_SA_mismatch_entries);
	NWFSPrint(&buffer[0]);
	nwvp_sprintf(&buffer[0], "        total FAT or end mismatch entries = %d \n", (int) fix_info.total_FAT_end_entries);
	NWFSPrint(&buffer[0]);
	nwvp_sprintf(&buffer[0], "               total FAT mismatch entries = %d \n", (int) fix_info.total_FAT_mismatch_entries);
        NWFSPrint(&buffer[0]);

		return;
	}
	NWFSPrint("VFATFIX VPARTITION_ID  \n");
}

void    vhotfix_routine(
	nwvp_console    *console)
{

	if (console->token_count > 2)
	{
		if (nwvp_vpartition_block_hotfix(
			console->token_value_table[1],
			console->token_value_table[2],
			&global_data_buffer[0]) == 0)
		{
			NWFSPrint("hotfix successful \n");
			return;
		}
	}
	NWFSPrint("VHOTFIX VPARTITION_ID BLOCK_NUMBER  \n");
}

void    lcheck_routine(
	nwvp_console    *console)
{
	ULONG		i;
	ULONG		value;
	BYTE		temp[16];
	for (i=0; i<console->token_value_table[3]; i++)
	{
		value = console->token_value_table[4] + console->token_value_table[2] + i;
		if (nwvp_lpartition_block_read(
			console->token_value_table[1],
			i + console->token_value_table[2],
			1,
			&global_data_buffer[0]) != 0)
		{
			NWFSPrint("check io error at block ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(console->token_value_table[2], 16, &temp[0]);
			NWFSPrint(&temp[0]);
			NWFSPrint("\n");
			return;
		}
		if (*(ULONG *) &global_data_buffer[0] != value)
		{
			NWFSPrint("check value error at block ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(console->token_value_table[2], 16, &temp[0]);
			NWFSPrint(&temp[0]);
			NWFSPrint("\n");
			return;
		}
	}
	NWFSPrint("fill complete  \n");
}

void    lfill_routine(
	nwvp_console    *console)
{
	ULONG		i;
	ULONG		value;
	BYTE		temp[16];
	for (i=0; i<console->token_value_table[3]; i++)
	{
		value = console->token_value_table[4] + console->token_value_table[2] + i;
		nwvp_memset(global_data_buffer, value, 4096);
		if (nwvp_lpartition_block_write(
			console->token_value_table[1],
			i + console->token_value_table[2],
			1,
			&global_data_buffer[0]) != 0)
		{
			NWFSPrint("fill error at block ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(console->token_value_table[2], 16, &temp[0]);
			NWFSPrint(&temp[0]);
			NWFSPrint("\n");
			return;
		}
	}
	NWFSPrint("fill complete  \n");
}
void    lread_routine(
	nwvp_console    *console)
{
	if (nwvp_lpartition_block_read(
			console->token_value_table[1],
			console->token_value_table[2],
			1,
			&global_data_buffer[0]) == 0)
	{
		NWFSPrint("read successful \n");
		return;
	}
	NWFSPrint("read failure  \n");
}


void    lwrite_routine(
	nwvp_console    *console)
{
	if (nwvp_lpartition_block_write(
			console->token_value_table[1],
			console->token_value_table[2],
			1,
			&global_data_buffer[0]) == 0)
	{
		NWFSPrint("write successful \n");
		return;
	}
	NWFSPrint("write failure  \n");
}

void    rread_routine(
	nwvp_console    *console)
{
	ULONG	count = 8;
	if (console->token_count > 3)
		count = (console->token_value_table[3] > 8) ? 8 : console->token_value_table[3];
	if (nwvp_rpartition_sector_read(
			console->token_value_table[1],
			console->token_value_table[2],
			count,
			&global_data_buffer[0]) == 0)
	{
		NWFSPrint("read successful \n");
		return;
	}
	NWFSPrint("read failure  \n");
}


void    rwrite_routine(
	nwvp_console    *console)
{
	ULONG	count = 8;
	if (console->token_count > 3)
		count = (console->token_value_table[3] > 8) ? 8 : console->token_value_table[3];

	if (nwvp_rpartition_sector_write(
			console->token_value_table[1],
			console->token_value_table[2],
			count,
			&global_data_buffer[0]) == 0)
	{
		NWFSPrint("write successful \n");
		return;
	}
	NWFSPrint("write failure  \n");
}


void    ladd_mirror_routine(
	nwvp_console    *console)
{

	if (nwvp_partition_add_mirror(console->token_value_table[1], console->token_value_table[2]) == 0)
	{
		NWFSPrint("mirror added successfully \n");
		return;
	}
	NWFSPrint("mirror add failed \n");
}

void    ldel_mirror_routine(
	nwvp_console    *console)
{
	ULONG	rpart_handle;
    ULONG	vhandle;
	ULONG	raw_ids[4];
	nwvp_rpartition_info		rpart_info;

	nwvp_rpartition_return_info(console->token_value_table[2], &rpart_info);
	if (nwvp_partition_del_mirror(console->token_value_table[1], console->token_value_table[2]) == 0)
	{
		nwvp_scan_routine(0);
		nwvp_rpartition_map_rpart_handle(&rpart_handle, rpart_info.physical_handle);
		raw_ids[0] = rpart_handle;
		raw_ids[1] = 0xFFFFFFFF;
		raw_ids[2] = 0xFFFFFFFF;
		raw_ids[3] = 0xFFFFFFFF;

		if (nwvp_lpartition_format(&vhandle, rpart_info.physical_block_count - 264, &raw_ids[0]) == 0)
		{
			NWFSPrint("mirror deleted successfully \n");
			return;
		}
		NWFSPrint("mirror deleted successfully ...  reformat failed\n");
		return;
	}
	NWFSPrint("mirror delete failed \n");
}

void    vcreate_routine(
	nwvp_console    *console)
{
	ULONG		i, j;
	ULONG		vhandle;
	BYTE		temp[16];
	segment_info		   stable[8];
	vpartition_info		vpart_info;
	vpart_info.cluster_count = 0;
	if (console->token_count > 5)
	{
		if (console->token_value_table[2] == 64)
			vpart_info.blocks_per_cluster = 0x10;
		else
		if (console->token_value_table[2] == 32)
			vpart_info.blocks_per_cluster = 0x8;
		else
		if (console->token_value_table[2] == 16)
			vpart_info.blocks_per_cluster = 0x4;
		else
		if (console->token_value_table[2] == 8)
			vpart_info.blocks_per_cluster = 0x2;
		else
		if (console->token_value_table[2] == 4)
			vpart_info.blocks_per_cluster = 1;
		else
		{
			NWFSPrint("invalid cluster size 64K 32K 16K 8K 4K \n" );
			return;
		}
		for (i=5, j=0; i<console->token_count; i+=3, j++)
		{
			stable[j].lpartition_id = console->token_value_table[i-2];
			stable[j].block_offset = console->token_value_table[i-1];
			stable[j].block_count = console->token_value_table[i-0];
			stable[j].extra = 0;
			vpart_info.cluster_count += console->token_value_table[i-0] / vpart_info.blocks_per_cluster;
		}

		if (j > 0)
		{
			nwvp_memset(&vpart_info.volume_name[0], 0, 16);
			nwvp_memmove(&vpart_info.volume_name[1], console->token_table[1], console->token_size_table[1]);
			vpart_info.volume_name[0] = console->token_size_table[1];
			vpart_info.segment_count = j;
			vpart_info.flags = (SUB_ALLOCATION_ON | NDS_FLAG | NEW_TRUSTEE_COUNT);

			if (nwvp_vpartition_format(&vhandle, &vpart_info, &stable[0]) == 0)
			{
				NWFSPrint("virtual partition created  new handle = " );
				nwvp_memset(&temp[0], 0, 16);
				nwvp_int_to_ascii(vhandle, 16, &temp[0]);
				NWFSPrint(&temp[0]);
				NWFSPrint("\n");
				return;
			}
		}
	}
	NWFSPrint("SYNTAX VCREATE NAME CLUSTER_SIZE_IN_K LHANDLE1 OFFSET COUNT ... \n");
}

void    vadd_segment_routine(
	nwvp_console    *console)
{
	segment_info		   seg_info;

	if (console->token_count >= 5)
	{
		seg_info.lpartition_id = console->token_value_table[2];
		seg_info.block_offset = console->token_value_table[3];
		seg_info.block_count = console->token_value_table[4];
		seg_info.extra = 0;
		if (nwvp_vpartition_add_segment(console->token_value_table[1], &seg_info) == 0)
		{
			NWFSPrint("Segment Successfully Added\n");
			return;
		}
	}
	NWFSPrint("SYNTAX VADD VHANDLE LHANDLE1 OFFSET COUNT \n");
}

/*void    vfat_check_routine(
	nwvp_console    *console)
{
	if (nwvp_vpartition_fat_check(console->token_value_table[1]) == 0)
	{
		NWFSPrint("volume fat check successfull\n");
		return;
	}
	NWFSPrint("SYNTAX VFATCHECK VHANDLE \n");
}
*/

void    vuncreate_routine(
	nwvp_console    *console)
{
	if (nwvp_vpartition_unformat(console->token_value_table[1]) == 0)
	{
		NWFSPrint("virtual partition uncreated successfull \n" );
		return;
	}
	NWFSPrint("UNCREATE VPARTITION_ID \n");
}


void    lremirror_routine(
	nwvp_console    *console)
{
	if (nwvp_lpartition_remirror(console->token_value_table[1]) == 0)
	{
		NWFSPrint("starting remirror process \n");
		return;
	}
	NWFSPrint("ERROR starting remirror process \n");
}

void    labort_routine(
	nwvp_console    *console)
{
	if (nwvp_lpartition_abort_remirror(console->token_value_table[1]) == 0)
	{
		NWFSPrint("aborting remirror process \n");
		return;
	}
	NWFSPrint("ERROR aborting remirror process \n");
}

void    lformat_routine(
	nwvp_console    *console)
{
	ULONG		vhandle;
	ULONG		raw_ids[4];
	BYTE		temp[16];

	if (console->token_count > 2)
	{
		raw_ids[0] = console->token_value_table[2];
		raw_ids[1] = (console->token_count > 3) ? console->token_value_table[3] : 0xFFFFFFFF;
		raw_ids[2] = (console->token_count > 4) ? console->token_value_table[4] : 0xFFFFFFFF;
		raw_ids[3] = (console->token_count > 5) ? console->token_value_table[5] : 0xFFFFFFFF;

		if (nwvp_lpartition_format(&vhandle, console->token_value_table[1], &raw_ids[0]) == 0)
		{
			NWFSPrint("format success  new handle = " );
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(vhandle, 16, &temp[0]);
			NWFSPrint(&temp[0]);
			NWFSPrint("\n");
			return;
		}
	}
	NWFSPrint("SYNTAX FORMAT size handle1, [handle2], [handle3], [handle4] \n" );

}

void    lunformat_routine(
	nwvp_console    *console)
{
	if (nwvp_lpartition_unformat(console->token_value_table[1]) == 0)
	{
		NWFSPrint("unformat successful\n");
		return;
	}
	NWFSPrint("unformat failed\n");
}

void    rlist_routine(
	nwvp_console    *console)
{
	ULONG		j;
	BYTE		temp[16];
	nwvp_rpartition_scan_info rlist[5];
	nwvp_payload	payload;
	nwvp_rpartition_info	 rpart_info;

	NWFSPrint(" \n");
	NWFSPrint("\n  RHandle   LHandle   LStatus    LSynch    Type   Capacity  \n\n");
	payload.payload_object_count = 0;
	payload.payload_index = 0;
	payload.payload_object_size_buffer_size = (8 << 20) + sizeof(rlist);
	payload.payload_buffer = (BYTE *) &rlist[0];
	do
	{
		nwvp_rpartition_scan(&payload);
		for (j=0; j< payload.payload_object_count; j++)
		{
			nwvp_rpartition_return_info(rlist[j].rpart_handle, &rpart_info);
			NWFSPrint("  ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(rlist[j].rpart_handle, 16, &temp[0]);
			NWFSPrint(&temp[0]);
			NWFSPrint("  " );
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(rpart_info.lpart_handle, 16, &temp[0]);
			NWFSPrint(&temp[0]);
			if ((rpart_info.status & MIRROR_GROUP_ACTIVE_BIT) != 0)
				NWFSPrint("  ACTIVE  ");
			else
				NWFSPrint(" INACTIVE ");
			if ((rpart_info.lpart_handle == 0xFFFFFFFF) || ((rpart_info.status & MIRROR_INSYNCH_BIT) != 0))
				NWFSPrint(" IN SYCNCH    "  );
			else
				NWFSPrint(" OUT OF SYNCH "  );
			if (rpart_info.physical_block_count != 0)
			{
				nwvp_memset(&temp[0], 0, 16);
				nwvp_int_to_ascii(rpart_info.partition_type, 16, &temp[0]);
				NWFSPrint(&temp[6]);
				NWFSPrint("    ");
				nwvp_memset(&temp[0], 0, 16);
				nwvp_int_to_ascii(rpart_info.physical_block_count, 16, &temp[0]);
				NWFSPrint(&temp[0]);
			}
			else
				NWFSPrint(" NOT PRESENT ");
			NWFSPrint(" \n");
		}
	} while ((payload.payload_index != 0) && (payload.payload_object_count != 0));
	NWFSPrint(" \n");
}

void    llist_routine(
	nwvp_console    *console)
{
	ULONG		j;
	BYTE		temp[16];
	nwvp_lpartition_scan_info llist[5];
	nwvp_payload	payload;
	nwvp_lpartition_info	lpart_info;

	NWFSPrint("\n  LHandle   Capacity   LStatus   Open   MCount Present Synch Remirror\n");
	NWFSPrint("\n");
	payload.payload_object_count = 0;
	payload.payload_index = 0;
	payload.payload_object_size_buffer_size = (sizeof(nwvp_lpartition_scan_info) << 20) + sizeof(llist);
	payload.payload_buffer = (BYTE *) &llist[0];
	do
	{
		nwvp_lpartition_scan(&payload);
		for (j=0; j< payload.payload_object_count; j++)
		{
			nwvp_lpartition_return_info(llist[j].lpart_handle, &lpart_info);

			NWFSPrint("  ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(llist[j].lpart_handle, 16, &temp[0]);
			NWFSPrint(&temp[0]);

			NWFSPrint("  ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(lpart_info.logical_block_count, 16, &temp[0]);
			NWFSPrint(&temp[0]);


			if ((lpart_info.lpartition_status & MIRROR_GROUP_ACTIVE_BIT) != 0)
				NWFSPrint("   ACTIVE  ");
			else
				NWFSPrint("  INACTIVE ");

			NWFSPrint("    ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(lpart_info.open_count, 10, &temp[0]);
			NWFSPrint(&temp[0]);

			NWFSPrint("      ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(llist[j].mirror_status & 0xFF, 10, &temp[0]);
			NWFSPrint(&temp[0]);

			NWFSPrint("       ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii((llist[j].mirror_status >> 8) & 0xFF, 10, &temp[0]);
			NWFSPrint(&temp[0]);

			NWFSPrint("      ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii((llist[j].mirror_status >> 16) & 0xFF, 10, &temp[0]);
			NWFSPrint(&temp[0]);

			NWFSPrint("     ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii((llist[j].mirror_status >> 24) & 0xFF, 10, &temp[0]);
			NWFSPrint(&temp[0]);

			NWFSPrint(" \n");
		}
	} while ((payload.payload_index != 0) && (payload.payload_object_count != 0));
	NWFSPrint(" \n");
}

void    vopen_routine(
	nwvp_console    *console)
{
	if (nwvp_vpartition_open(console->token_value_table[1]) == 0)
	{
		NWFSPrint("virtual partition opened\n");
		return;
	}
	NWFSPrint("virtual partition NOT opened\n");
}

void    vclose_routine(
	nwvp_console    *console)
{
	if (nwvp_vpartition_close(console->token_value_table[1]) == 0)
	{
		NWFSPrint("virtual partition closed\n");
		return;
	}
	NWFSPrint("virtual partition NOT closed\n");
}


void    vlist_routine(
	nwvp_console    *console)
{
	ULONG		j;
	BYTE		temp[16];
	ULONG		handles[7];
	nwvp_payload	payload;
	nwvp_vpartition_info		vpinfo;


	NWFSPrint("\n   VHandle            VName  VCLusters  Size  Segments  Open  Complete \n\n");
	payload.payload_object_count = 0;
	payload.payload_index = 0;
	payload.payload_object_size_buffer_size = (7 << 20) + sizeof(handles);
	payload.payload_buffer = (BYTE *) &handles[0];
	do
	{
		nwvp_vpartition_scan(&payload);
		for (j=0; j< payload.payload_object_count; j++)
		{
			if (nwvp_vpartition_return_info(handles[j], &vpinfo) == 0)
			{
				NWFSPrint("  ");
				nwvp_memset(&temp[0], 0, 16);
				nwvp_int_to_ascii(handles[j], 16, &temp[0]);
				NWFSPrint(&temp[0]);

				NWFSPrint("  ");
#if MANOS
				NWFSPrint(&vpinfo.volume_name[1]);
#else
				NWFSPrint("%16s", &vpinfo.volume_name[1]);
#endif

				NWFSPrint("  ");
				nwvp_memset(&temp[0], 0, 16);
				nwvp_int_to_ascii(vpinfo.cluster_count, 16, &temp[0]);
				NWFSPrint(&temp[0]);

				NWFSPrint("   ");
				nwvp_memset(&temp[0], 0, 16);
				nwvp_int_to_ascii(vpinfo.blocks_per_cluster * 4, 10, &temp[0]);
				NWFSPrint(&temp[0]);
				NWFSPrint("K      ");

				nwvp_memset(&temp[0], 0, 16);
				nwvp_int_to_ascii(vpinfo.segment_count, 10, &temp[0]);
				NWFSPrint(&temp[0]);

				NWFSPrint((vpinfo.open_flag == 0) ? "      No " : "     Yes ");
				NWFSPrint((vpinfo.volume_valid_flag == 0) ? "    Yes " : (vpinfo.volume_valid_flag == 0xFFFFFFFF) ? "     No " : "    BAD ");

				NWFSPrint("\n");
			}
		}
	} while ((payload.payload_index != 0) && (payload.payload_object_count != 0));
	NWFSPrint("\n");
}

void    vspace_routine(
	nwvp_console    *console)
{
	ULONG		i;
	BYTE		temp[16];
	nwvp_vpartition_info		vpinfo;


	if (nwvp_vpartition_return_info(console->token_value_table[1], &vpinfo) == 0)
	{
		NWFSPrint("\n  Seg #  LHandle   LOffset   LCount    VOffset   VCount  \n\n");
		for (i=0; i<vpinfo.segment_count; i++)
		{
			NWFSPrint("    ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(i, 10, &temp[0]);
			NWFSPrint(&temp[0]);

			NWFSPrint("    ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(vpinfo.segments[i].lpart_handle, 16, &temp[0]);
			NWFSPrint(&temp[0]);

			NWFSPrint("  ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(vpinfo.segments[i].partition_block_offset, 16, &temp[0]);
			NWFSPrint(&temp[0]);

			NWFSPrint("  ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(vpinfo.segments[i].segment_block_count, 16, &temp[0]);
			NWFSPrint(&temp[0]);

			NWFSPrint("  ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(vpinfo.segments[i].relative_cluster_offset, 16, &temp[0]);
			NWFSPrint(&temp[0]);

			NWFSPrint("  ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(vpinfo.segments[i].segment_block_count / vpinfo.blocks_per_cluster, 16, &temp[0]);
			NWFSPrint(&temp[0]);
			NWFSPrint("\n");
		}
		NWFSPrint("\n");
		return;
	}
	NWFSPrint("\n SYNTAX VSPACE vhandle\n");
}

void    lactivate_routine(
	nwvp_console    *console)
{
	if (nwvp_lpartition_activate(console->token_value_table[1]) == 0)
	{
		NWFSPrint("logical partition activated\n");
		return;
	}
	NWFSPrint("logical partition activation failed\n");
}

void    lspace_routine(
	nwvp_console    *console)
{
	ULONG		j, k;
	BYTE		temp[16];
	nwvp_lpartition_space_return_info	slist[5];
	nwvp_payload	payload1;

	nwvp_lpartition_scan_info llist[5];
	nwvp_payload	payload;

	NWFSPrint("\n  Lhandle   LOffset  LCount   Use    Mirror \n\n");
	payload.payload_object_count = 0;
	payload.payload_index = 0;
	payload.payload_object_size_buffer_size = (sizeof(nwvp_lpartition_scan_info) << 20) + sizeof(llist);
	payload.payload_buffer = (BYTE *) &llist[0];
	do
	{
		nwvp_lpartition_scan(&payload);
		for (k=0; k< payload.payload_object_count; k++)
		{
			payload1.payload_object_count = 0;
			payload1.payload_index = 0;
			payload1.payload_object_size_buffer_size = (8 << 20) + sizeof(slist);
			payload1.payload_buffer = (BYTE *) &slist[0];
			do
			{
				nwvp_lpartition_return_space_info(llist[k].lpart_handle, &payload1);
				for (j=0; j< payload1.payload_object_count; j++)
				{
					NWFSPrint(" ");
					nwvp_memset(&temp[0], 0, 16);
					nwvp_int_to_ascii(slist[j].lpart_handle, 16, &temp[0]);
					NWFSPrint(&temp[0]);

					NWFSPrint("  ");
					nwvp_memset(&temp[0], 0, 16);
					nwvp_int_to_ascii(slist[j].segment_offset, 16, &temp[0]);
					NWFSPrint(&temp[0]);

					NWFSPrint(" ");
					nwvp_memset(&temp[0], 0, 16);
					nwvp_int_to_ascii(slist[j].segment_count, 16, &temp[0]);
					NWFSPrint(&temp[0]);

					NWFSPrint(((slist[j].status_bits & INUSE_BIT) == 0) ? " FREE " : " INUSE");
					NWFSPrint(((slist[j].status_bits & MIRROR_BIT) != 0) ? "  MIRRORED" : "  SINGLE");

					NWFSPrint(" \n");
				}
			} while ((payload1.payload_index != 0) && (payload1.payload_object_count != 0));
		}
	} while ((payload.payload_index != 0) && (payload.payload_object_count != 0));
	NWFSPrint(" \n");
}

void    display_routine(
	nwvp_console    *console)
{
	ULONG		i, j;
	ULONG		base;
	BYTE		temp[16];
	BYTE		ascii[20];
	BYTE		*buffer;
	BYTE		b_value;

		buffer = &global_data_buffer[0];

		if ((base = (console->token_count > 1) ? console->token_value_table[1] : 0) > 0xF00)
			base = 0xF00;
		for (i=0; i<16; i++)
		{
			NWFSPrint("\n");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(base + (i * 16), 16, &temp[0]);
			NWFSPrint(&temp[0]);
			NWFSPrint("  ");
			for (j=0; j<16; j++)
			{
				b_value = buffer[base + (i * 16) + j];
				nwvp_memset(&temp[0], 0, 16);
				nwvp_int_to_ascii(b_value, 16, &temp[0]);
				NWFSPrint(&temp[6]);
				NWFSPrint(" ");
				ascii[j] = ((b_value >= '0') && (b_value <='z')) ? b_value : '.';
			}
			ascii[j] = 0;
			NWFSPrint(" ");
			NWFSPrint(&ascii[0]);
		}
		NWFSPrint("\n");
		return;
}

void    ddisplay_routine(
	nwvp_console    *console)
{
	ULONG		i, j;
	ULONG		base;
	ULONG		l_value;
	BYTE		temp[16];
	BYTE		ascii[20];
	BYTE		*buffer;
	BYTE		b_value;



		buffer = &global_data_buffer[0];
		if ((base = (console->token_count > 1) ? console->token_value_table[1] : 0) > 0xF00)
			base = 0xF00;
		for (i=0; i<16; i++)
		{
			NWFSPrint("\n");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(base + (i * 16), 16, &temp[0]);
			NWFSPrint(&temp[0]);
			NWFSPrint("  ");
			for (j=0; j<16; j+=4)
			{
				l_value = *(ULONG *) &buffer[base + (i * 16) + j];
				nwvp_memset(&temp[0], 0, 16);
				nwvp_int_to_ascii(l_value, 16, &temp[0]);
				NWFSPrint(&temp[0]);
				NWFSPrint(" ");
			}
			for (j=0; j<16; j++)
			{
				b_value = buffer[base + (i * 16) + j];
				ascii[j] = ((b_value >= '0') && (b_value <='z')) ? b_value : '.';
			}
			ascii[j] = 0;
			NWFSPrint(" ");
			NWFSPrint(&ascii[0]);
		}
		NWFSPrint ("\n");
		return;
}

void    set_routine(
	nwvp_console    *console)
{
	nwvp_memset(&global_data_buffer[0], console->token_value_table[1], 4096);
}

#if MANOS

ULONG   test_thread_count = 0;
ULONG   test_vpartition = 0;
ULONG   test_init_flag = 0;
ULONG   test_thread_table[1024];
ULONG   test_range = 0x1000;

ULONG   test_write_thread(
        ULONG       block_count)
{
    ULONG   i;
    ULONG   value;
    BYTE    *buffer;

    nwvp_alloc((void **) &buffer, 4096);
    nwvp_vpartition_block_read(test_vpartition, 0, 1, (BYTE *) &test_thread_table[0]);
    while (1)
    {
        for (i=1; i<test_range; i++)
        {
            if (test_thread_count == 0)
            {
                nwvp_free(buffer);
                return(0);
            }
            value = i + test_thread_table[(i % 512) + 512];
            nwvp_memset(buffer, value, 4096);
            nwvp_vpartition_block_write(test_vpartition, i, 1, buffer);
            thread_switch();
        }
        nwvp_memmove(&test_thread_table[0], &test_thread_table[512], 1024 * 2);
        for (i=0; i<512; i++)
            test_thread_table[i + 512] = test_random(0xFFFF) << 16;
        nwvp_vpartition_block_write(test_vpartition, 0, 1, (BYTE *) &test_thread_table[0]);
    }
}

void    vstart_test_routine(
	nwvp_console    *console)
{
    if (test_thread_count == 0)
    {
        test_thread_count ++;
        test_vpartition = console->token_value_table[1];
        test_range = console->token_value_table[2];
        nwvp_thread_create("random write thread ", test_write_thread, (ULONG) 0);
    }
}

void    vstop_test_routine(
	nwvp_console    *console)
{
    if (console != 0)
        test_thread_count = 0;
}



#define     NEW_MASTER             0x11111111
#define     CURRENT_MASTER         0x22222222

struct      test_thread_def
{
    ULONG   done_flag;
    ULONG   count_down;
    ULONG   master_flag;
    ULONG   rpart_handle;

    ULONG   lpart_handle;
    ULONG   mirror_index;
    ULONG   my_state;
    nwvp_console     *console;
} thread_table[8];
ULONG    mirror_test_active = 0;
ULONG    mirror_count = 0;

ULONG    insynch_test(
    ULONG  lpart_handle,
    ULONG  rpart_handle)
{
    ULONG       i;
	nwvp_lpartition_info	lpart_info;

    if (nwvp_lpartition_return_info(lpart_handle, &lpart_info) == 0)
    {
        for (i=0; i<lpart_info.mirror_count; i++)
	{
            if (lpart_info.m[i].rpart_handle == rpart_handle)
            {
                if ((lpart_info.m[i].status & MIRROR_INSYNCH_BIT) != 0)
                    return(0);
            }
        }
    }
    return(-1);
}



ULONG   lmirror_test_thread(
        struct  test_thread_def *thread)
{
    ULONG   skip;
    ULONG   i;
	nwvp_rpartition_info		rpart_info;
	nwvp_console    *console;

    console = thread->console;

    while (thread->done_flag == 0)
    {
        nwvp_thread_delay((test_random(10) + 1) * 18);
	switch(thread->my_state)
        {
            case    0:
                    if (thread->master_flag != CURRENT_MASTER)
                    {
                       thread->my_state = 4;
                       thread->count_down = test_random(10) + 1;
                    }
                    break;
            case    1:
                    if (thread->master_flag == NEW_MASTER)
                    {
                        for (i=0; i<mirror_count; i++)
			    thread_table[i].master_flag = 0;

                        skip = test_random(mirror_count);
                        if (skip == thread->mirror_index)
                            skip = (skip == 0) ? 1 : skip - 1;
                        thread_table[skip].master_flag = NEW_MASTER;
                        thread->master_flag = CURRENT_MASTER;
                        thread->my_state = 0;
                        switch(thread->mirror_index)
                        {
                        case 0:
                            NWFSPrint(" becoming master 0 \n");
                            break;
			case 1:
                            NWFSPrint(" becoming master 1 \n");
                            break;
                        case 2:
                            NWFSPrint(" becoming master 2 \n");
                            break;
                        case 3:
                            NWFSPrint(" becoming master 3 \n");
                            break;
                        }
                    }
                    else
                    {
			switch(test_random(8))
                        {
                            case    0:
                            case    1:
                            case    2:
                                    break;
                            case    3:
                            case    4:
                                    nwvp_rpartition_return_info(thread->rpart_handle, &rpart_info);
                                    nwvp_partition_del_mirror(thread->lpart_handle, thread->rpart_handle);
                                    nwvp_scan_routine(0);
                                    nwvp_rpartition_map_rpart_handle(&thread->rpart_handle, rpart_info.physical_handle);
                                    thread->my_state = 3;
				    switch(thread->mirror_index)
                                    {
                                    case 0:
                                            NWFSPrint(" delete mirror 0 \n");
                                            break;
                                    case 1:
                                            NWFSPrint(" delete mirror 1 \n");
                                            break;
                                    case 2:
                                            NWFSPrint(" delete mirror 2 \n");
                                            break;
                                    case 3:
                                            NWFSPrint(" delete mirror 3 \n");
					    break;
                                    }
                                    break;
                            case    5:
                            case    6:
                            case    7:
//                            off_line(thread->rpart_handle);
//                            thread->my_state = 2;
                                    break;
                        }
                    }
                    break;
            case    2:
//                    online(thread->rpart_handle);
                    nwvp_scan_routine(0);
                    nwvp_lpartition_remirror(thread->lpart_handle);
                    thread->my_state = 4;
                    break;
            case    3:
                    nwvp_partition_add_mirror(thread->lpart_handle, thread->rpart_handle);
                    nwvp_scan_routine(0);
                    nwvp_lpartition_remirror(thread->lpart_handle);
                    thread->my_state = 4;
                    switch(thread->mirror_index)
                    {
                    case 0:
			NWFSPrint(" adding mirror 0 \n");
                        break;
                    case 1:
                        NWFSPrint(" adding mirror 1 \n");
                        break;
                    case 2:
                        NWFSPrint(" adding mirror 2 \n");
                        break;
                    case 3:
                        NWFSPrint(" adding mirror 3 \n");
                        break;
                    }
                    break;
	    case    4:
                    if (thread->count_down == 0)
                       thread->my_state = 5;
                    thread->count_down --;
                    break;
            case    5:
                    if (insynch_test(thread->lpart_handle, thread->rpart_handle) == 0)
                    {
                        thread->my_state = 1;
                        break;
                    }
                    if (thread->master_flag == 0)
                    {
			switch(test_random(8))
                        {
                            case    0:
                            case    1:
                            case    2:
                                    break;
                            case    3:
                            case    4:
                                    nwvp_rpartition_return_info(thread->rpart_handle, &rpart_info);
                                    nwvp_partition_del_mirror(thread->lpart_handle, thread->rpart_handle);
                                    nwvp_scan_routine(0);
                                    nwvp_rpartition_map_rpart_handle(&thread->rpart_handle, rpart_info.physical_handle);
                                    thread->my_state = 3;
				    switch(thread->mirror_index)
                                    {
                                    case 0:
                                            NWFSPrint(" delete mirror 0 \n");
                                            break;
                                    case 1:
                                            NWFSPrint(" delete mirror 1 \n");
                                            break;
                                    case 2:
                                            NWFSPrint(" delete mirror 2 \n");
                                            break;
                                    case 3:
                                            NWFSPrint(" delete mirror 3 \n");
					    break;
                                    }
                            case    5:
                            case    6:
                            case    7:
//                            off_line(thread->rpart_handle);
//                            thread->my_state = 2;
                                    break;
                        }
                    }
                    break;
        }
    }
    return(0);
}

void    lmirror_test_routine(
	nwvp_console    *console)
{
    ULONG       i;
	nwvp_lpartition_info	lpart_info;

    if (mirror_test_active != 0)
    {
        mirror_test_active = 0;
        for (i=0; i<8; i++)
	    thread_table[i].done_flag = 1;
        NWFSPrint(" stopping mirror test \n");
        return;
    }

    if (nwvp_lpartition_return_info(console->token_value_table[1], &lpart_info) == 0)
    {
        mirror_count = lpart_info.mirror_count;
        mirror_test_active = 1;
        for (i=0; i<lpart_info.mirror_count; i++)
        {
            thread_table[i].my_state = (i== 0) ? 0 : 1;
            thread_table[i].master_flag = (i == 0) ? CURRENT_MASTER : ((i == 1) ? NEW_MASTER : 0);
	    thread_table[i].lpart_handle = lpart_info.lpartition_handle;
            thread_table[i].rpart_handle = lpart_info.m[i].rpart_handle;
            thread_table[i].done_flag = 0;
            thread_table[i].mirror_index = i;
            thread_table[i].console = console;
	    nwvp_thread_create("mirror test thread ", lmirror_test_thread, (ULONG) &thread_table[i]);
        }
        NWFSPrint(" starting mirror test \n");
        return;
    }
    NWFSPrint(" invalide logical partition handle \n");
}

void    lru_lock_routine(
	nwvp_console    *console)
{
    ULONG   lru_address;
    BYTE    temp[16];

	if (console->token_count > 2)
	{
        if ((lru_address = (ULONG)  ReadLRU(console->token_value_table[1], console->token_value_table[2], 0)) != 0)
        {
			NWFSPrint("\n LRU ADDRESS = ");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(lru_address, 16, &temp[0]);
			NWFSPrint(&temp[0]);
			NWFSPrint("\n");
			return;
		}
	}
	NWFSPrint("LRU_LOCK VPARTITION_ID BLOCK_NUMBER  \n");
}

void    lru_release_routine(
	nwvp_console    *console)
{
    ReleaseLRU((LRU *) console->token_value_table[1]);
}

void    lru_dirty_routine(
	nwvp_console    *console)
{
    ReleaseDirtyLRU((LRU *) console->token_value_table[1]);
}

void    lru_display_routine(
	nwvp_console    *console)
{
	ULONG		i, j;
	ULONG		base;
	ULONG		l_value;
	BYTE		temp[16];
	BYTE		ascii[20];
	BYTE		*buffer;
	BYTE		b_value;

    LRU         *lru;
    lru = (LRU *) console->token_value_table[1];

		buffer = &lru->buffer[0];
		if ((base = (console->token_count > 1) ? console->token_value_table[2] : 0) > 0xF00)
			base = 0xF00;
		for (i=0; i<16; i++)
		{
			NWFSPrint("\n");
			nwvp_memset(&temp[0], 0, 16);
			nwvp_int_to_ascii(base + (i * 16), 16, &temp[0]);
			NWFSPrint(&temp[0]);
			NWFSPrint("  ");
			for (j=0; j<16; j+=4)
			{
				l_value = *(ULONG *) &buffer[base + (i * 16) + j];
				nwvp_memset(&temp[0], 0, 16);
				nwvp_int_to_ascii(l_value, 16, &temp[0]);
				NWFSPrint(&temp[0]);
				NWFSPrint(" ");
			}
			for (j=0; j<16; j++)
			{
				b_value = buffer[base + (i * 16) + j];
				ascii[j] = ((b_value >= '0') && (b_value <='z')) ? b_value : '.';
			}
			ascii[j] = 0;
			NWFSPrint(" ");
			NWFSPrint(&ascii[0]);
		}
		NWFSPrint ("\n");
		return;
}
#endif
void    nwvp_add_commands(
	nwvp_console	*console)
{

	rscan_string[0] = sizeof(rscan_string) - 2;
	rscan_help_string[0] = sizeof(rscan_help_string) - 2;
	nwvp_add_console_command(console, rscan_string, rscan_routine, rscan_help_string);

	scan_string[0] = sizeof(scan_string) - 2;
	scan_help_string[0] = sizeof(scan_help_string) - 2;
	nwvp_add_console_command(console, scan_string, scan_routine, scan_help_string);

	unscan_string[0] = sizeof(unscan_string) - 2;
	unscan_help_string[0] = sizeof(unscan_help_string) - 2;
	nwvp_add_console_command(console, unscan_string, unscan_routine, unscan_help_string);

	rlist_string[0] = sizeof(rlist_string) - 2;
	rlist_help_string[0] = sizeof(rlist_help_string) - 2;
	nwvp_add_console_command(console, rlist_string, rlist_routine, rlist_help_string);

	rread_string[0] = sizeof(rread_string) - 2;
	rread_help_string[0] = sizeof(rread_help_string) - 2;
	nwvp_add_console_command(console, rread_string, rread_routine, rread_help_string);

	rwrite_string[0] = sizeof(rwrite_string) - 2;
	rwrite_help_string[0] = sizeof(rwrite_help_string) - 2;
	nwvp_add_console_command(console, rwrite_string, rwrite_routine, rwrite_help_string);

	lremirror_string[0] = sizeof(lremirror_string) - 2;
	lremirror_help_string[0] = sizeof(lremirror_help_string) - 2;
	nwvp_add_console_command(console, lremirror_string, lremirror_routine, lremirror_help_string);

	labort_string[0] = sizeof(labort_string) - 2;
	labort_help_string[0] = sizeof(labort_help_string) - 2;
	nwvp_add_console_command(console, labort_string, labort_routine, labort_help_string);

	lformat_string[0] = sizeof(lformat_string) - 2;
	lformat_help_string[0] = sizeof(lformat_help_string) - 2;
	nwvp_add_console_command(console, lformat_string, lformat_routine, lformat_help_string);

	lunformat_string[0] = sizeof(lunformat_string) - 2;
	lunformat_help_string[0] = sizeof(lunformat_help_string) - 2;
	nwvp_add_console_command(console, lunformat_string, lunformat_routine, lunformat_help_string);

	ladd_mirror_string[0] = sizeof(ladd_mirror_string) - 2;
	ladd_mirror_help_string[0] = sizeof(ladd_mirror_help_string) - 2;
	nwvp_add_console_command(console, ladd_mirror_string, ladd_mirror_routine, ladd_mirror_help_string);

	ldel_mirror_string[0] = sizeof(ldel_mirror_string) - 2;
	ldel_mirror_help_string[0] = sizeof(ldel_mirror_help_string) - 2;
	nwvp_add_console_command(console, ldel_mirror_string, ldel_mirror_routine, ldel_mirror_help_string);

	llist_string[0] = sizeof(llist_string) - 2;
	llist_help_string[0] = sizeof(llist_help_string) - 2;
	nwvp_add_console_command(console, llist_string, llist_routine, llist_help_string);

	lspace_string[0] = sizeof(lspace_string) - 2;
	lspace_help_string[0] = sizeof(lspace_help_string) - 2;
	nwvp_add_console_command(console, lspace_string, lspace_routine, lspace_help_string);

	lactivate_string[0] = sizeof(lactivate_string) - 2;
	lactivate_help_string[0] = sizeof(lactivate_help_string) - 2;
	nwvp_add_console_command(console, lactivate_string, lactivate_routine, lactivate_help_string);

	lfill_string[0] = sizeof(lfill_string) - 2;
	lfill_help_string[0] = sizeof(lfill_help_string) - 2;
	nwvp_add_console_command(console, lfill_string, lfill_routine, lfill_help_string);

	lcheck_string[0] = sizeof(lcheck_string) - 2;
	lcheck_help_string[0] = sizeof(lcheck_help_string) - 2;
	nwvp_add_console_command(console, lcheck_string, lcheck_routine, lcheck_help_string);

	lread_string[0] = sizeof(lread_string) - 2;
	lread_help_string[0] = sizeof(lread_help_string) - 2;
	nwvp_add_console_command(console, lread_string, lread_routine, lread_help_string);

	lwrite_string[0] = sizeof(lwrite_string) - 2;
	lwrite_help_string[0] = sizeof(lwrite_help_string) - 2;
	nwvp_add_console_command(console, lwrite_string, lwrite_routine, lwrite_help_string);

	vlist_string[0] = sizeof(vlist_string) - 2;
	vlist_help_string[0] = sizeof(vlist_help_string) - 2;
	nwvp_add_console_command(console, vlist_string, vlist_routine, vlist_help_string);

	vcreate_string[0] = sizeof(vcreate_string) - 2;
	vcreate_help_string[0] = sizeof(vcreate_help_string) - 2;
	nwvp_add_console_command(console, vcreate_string, vcreate_routine, vcreate_help_string);

	vadd_segment_string[0] = sizeof(vadd_segment_string) - 2;
	vadd_segment_help_string[0] = sizeof(vadd_segment_help_string) - 2;
	nwvp_add_console_command(console, vadd_segment_string, vadd_segment_routine, vadd_segment_help_string);

//	vfat_check_string[0] = sizeof(vfat_check_string) - 2;
//	vfat_check_help_string[0] = sizeof(vfat_check_help_string) - 2;
//	nwvp_add_console_command(console, vfat_check_string, vfat_check_routine, vfat_check_help_string);

	vuncreate_string[0] = sizeof(vuncreate_string) - 2;
	vuncreate_help_string[0] = sizeof(vuncreate_help_string) - 2;
	nwvp_add_console_command(console, vuncreate_string, vuncreate_routine, vuncreate_help_string);

	vspace_string[0] = sizeof(vspace_string) - 2;
	vspace_help_string[0] = sizeof(vspace_help_string) - 2;
	nwvp_add_console_command(console, vspace_string, vspace_routine, vspace_help_string);

	vopen_string[0] = sizeof(vopen_string) - 2;
	vopen_help_string[0] = sizeof(vopen_help_string) - 2;
	nwvp_add_console_command(console, vopen_string, vopen_routine, vopen_help_string);

	vclose_string[0] = sizeof(vclose_string) - 2;
	vclose_help_string[0] = sizeof(vclose_help_string) - 2;
	nwvp_add_console_command(console, vclose_string, vclose_routine, vclose_help_string);

	vfill_string[0] = sizeof(vfill_string) - 2;
	vfill_help_string[0] = sizeof(vfill_help_string) - 2;
	nwvp_add_console_command(console, vfill_string, vfill_routine, vfill_help_string);

	vcheck_string[0] = sizeof(vcheck_string) - 2;
	vcheck_help_string[0] = sizeof(vcheck_help_string) - 2;
	nwvp_add_console_command(console, vcheck_string, vcheck_routine, vcheck_help_string);

	vread_string[0] = sizeof(vread_string) - 2;
	vread_help_string[0] = sizeof(vread_help_string) - 2;
	nwvp_add_console_command(console, vread_string, vread_routine, vread_help_string);

	vwrite_string[0] = sizeof(vwrite_string) - 2;
	vwrite_help_string[0] = sizeof(vwrite_help_string) - 2;
	nwvp_add_console_command(console, vwrite_string, vwrite_routine, vwrite_help_string);

	vfatfix_string[0] = sizeof(vfatfix_string) - 2;
	vfatfix_help_string[0] = sizeof(vfatfix_help_string) - 2;
	nwvp_add_console_command(console, vfatfix_string, vfatfix_routine, vfatfix_help_string);

	vfatcheck_string[0] = sizeof(vfatcheck_string) - 2;
	vfatcheck_help_string[0] = sizeof(vfatcheck_help_string) - 2;
	nwvp_add_console_command(console, vfatcheck_string, vfatcheck_routine, vfatcheck_help_string);

	vhotfix_string[0] = sizeof(vhotfix_string) - 2;
	vhotfix_help_string[0] = sizeof(vhotfix_help_string) - 2;
	nwvp_add_console_command(console, vhotfix_string, vhotfix_routine, vhotfix_help_string);

	vmap_string[0] = sizeof(vmap_string) - 2;
	vmap_help_string[0] = sizeof(vmap_help_string) - 2;
	nwvp_add_console_command(console, vmap_string, vmap_routine, vmap_help_string);

	ddisplay_string[0] = sizeof(ddisplay_string) - 2;
	ddisplay_help_string[0] = sizeof(ddisplay_help_string) - 2;
	nwvp_add_console_command(console, ddisplay_string, ddisplay_routine, ddisplay_help_string);

	display_string[0] = sizeof(display_string) - 2;
	display_help_string[0] = sizeof(display_help_string) - 2;
	nwvp_add_console_command(console, display_string, display_routine, display_help_string);


	set_string[0] = sizeof(set_string) - 2;
	set_help_string[0] = sizeof(set_help_string) - 2;
	nwvp_add_console_command(console, set_string, set_routine, set_help_string);

	cbyte_string[0] = sizeof(cbyte_string) - 2;
	cbyte_help_string[0] = sizeof(cbyte_help_string) - 2;
	nwvp_add_console_command(console, cbyte_string, cbyte_routine, cbyte_help_string);

	clong_string[0] = sizeof(clong_string) - 2;
	clong_help_string[0] = sizeof(clong_help_string) - 2;
	nwvp_add_console_command(console, clong_string, clong_routine, clong_help_string);

#if MANOS

	vstart_test_string[0] = sizeof(vstart_test_string) - 2;
	vstart_test_help_string[0] = sizeof(vstart_test_help_string) - 2;
	nwvp_add_console_command(console, vstart_test_string, vstart_test_routine, vstart_test_help_string);

	vstop_test_string[0] = sizeof(vstop_test_string) - 2;
	vstop_test_help_string[0] = sizeof(vstop_test_help_string) - 2;
	nwvp_add_console_command(console, vstop_test_string, vstop_test_routine, vstop_test_help_string);

	lmirror_test_string[0] = sizeof(lmirror_test_string) - 2;
	lmirror_test_help_string[0] = sizeof(lmirror_test_help_string) - 2;
	nwvp_add_console_command(console, lmirror_test_string, lmirror_test_routine, lmirror_test_help_string);

	lru_lock_string[0] = sizeof(lru_lock_string) - 2;
	lru_lock_help_string[0] = sizeof(lru_lock_help_string) - 2;
	nwvp_add_console_command(console, lru_lock_string, lru_lock_routine, lru_lock_help_string);

	lru_release_string[0] = sizeof(lru_release_string) - 2;
	lru_release_help_string[0] = sizeof(lru_release_help_string) - 2;
	nwvp_add_console_command(console, lru_release_string, lru_release_routine, lru_release_help_string);

	lru_dirty_string[0] = sizeof(lru_dirty_string) - 2;
	lru_dirty_help_string[0] = sizeof(lru_dirty_help_string) - 2;
	nwvp_add_console_command(console, lru_dirty_string, lru_dirty_routine, lru_dirty_help_string);

	lru_display_string[0] = sizeof(lru_display_string) - 2;
	lru_display_help_string[0] = sizeof(lru_display_help_string) - 2;
	nwvp_add_console_command(console, lru_display_string, lru_display_routine, lru_display_help_string);
#endif
}
