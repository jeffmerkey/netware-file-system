/***************************************************************************
*
*   Copyright (c) 1997-2022 Jeff V. Merkey
*   7260 SE Tenino St.
*   Portland, Oregon 97206
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
*   AUTHORS  :  Jeff V. Merkey
*   FILE     :  NWVP.C
*   DESCRIP  :  NetWare Virtual Partition
*   DATE     :  August 1, 2022
*
***************************************************************************/

#include "globals.h"

#if (LINUX_20 | LINUX_22 | LINUX_24)

ULONG	nwvp_auto_remirror_flag	= 1;
ULONG	nwvp_remirror_message_flag = 1;

#else

ULONG	nwvp_auto_remirror_flag	= 0;
ULONG	nwvp_remirror_message_flag = 0;

#endif

ULONG	nwvp_remirror_delay = 8;
ULONG	handle_instance = 7;
nwvp_remirror_control	*nwvp_remirror_control_list_head = 0;
void	(*poll_routine)(void) = 0;
ULONG	nwvp_remirror_poll(void);
BYTE	*zero_block_buffer = 0;
BYTE	*bit_bucket_buffer = 0;
BYTE	*remirror_buffer1 = 0;
BYTE	*remirror_buffer2 = 0;

ULONG	cluster_size_table[] = { 0xFFFFFFFF, 0x00000003, 0x00000004, 0xFFFFFFFF,
				 0x00000005, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
				 0x00000006, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
				 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
				 0x00000007};

ULONG	cluster_block_table[] = {0, 0, 0, 1, 2, 4, 8, 16};


ULONG		raw_partition_table_count = 0;
nwvp_lpart	*raw_partition_table[MAX_PARTITIONS];
ULONG		logical_partition_table_count = 0;
nwvp_lpart	*logical_partition_table[MAX_PARTITIONS];
ULONG		virtual_partition_table_count = 0;
nwvp_vpartition	*virtual_partition_table[MAX_VOLUMES];
ULONG	unique_id = 0x55500000;

ULONG	log_table[1024];
ULONG	log_index = 0;

void	nwvp_log(
	ULONG	event_type,
	ULONG	handle,
	ULONG	parameter0,
	ULONG	parameter1)
{
	log_table[log_index + 0] = event_type;
	log_table[log_index + 1] = handle;
	log_table[log_index + 2] = parameter0;
	log_table[log_index + 3] = parameter1;
	log_index = (log_index + 4) & 0x3FF;
}

ULONG	nwvp_create_new_id(void)
{
	return(nwvp_get_date_and_time() + unique_id ++);
}

ULONG	nwvp_init_flag = 0;

void	nwvp_init()
{
	nwvp_io_alloc((void **) &zero_block_buffer, 4096);
	nwvp_io_alloc((void **) &bit_bucket_buffer, 4096);
	nwvp_io_alloc((void **) &remirror_buffer1, 4096);
	nwvp_io_alloc((void **) &remirror_buffer2, 4096);
	nwvp_memset(zero_block_buffer, 0, 4096);
#if MANOS
	nwvp_thread_create("remirror thread ", nwvp_mirror_validate_process, 0);
#endif
#if DOS_UTIL
	poll_routine = (void (*)(void)) nwvp_remirror_poll;
#endif
	nwvp_init_flag = 1;
}

void	nwvp_uninit()
{
	if (nwvp_init_flag != 0)
	{
		if (zero_block_buffer != 0)						// initialize zero buffer
			nwvp_io_free(zero_block_buffer);
		zero_block_buffer = 0;
		if (bit_bucket_buffer != 0)						// initialize zero buffer
			nwvp_io_free(bit_bucket_buffer);
		bit_bucket_buffer = 0;
		if (remirror_buffer1 != 0)						// initialize zero buffer
			nwvp_io_free(remirror_buffer1);
		remirror_buffer1 = 0;
		if (remirror_buffer2 != 0)						// initialize zero buffer
			nwvp_io_free(remirror_buffer2);
		remirror_buffer2 = 0;
	}
	nwvp_init_flag = 0;
}


ULONG   fat_block_check(
        ULONG           *fat_block,
        ULONG           *check_sum,
	ULONG           max_index,
        ULONG           max_link)
{
    ULONG   i, j;
    ULONG   zero_flag = 0;

    *check_sum = 0;

    for (i=0; i < 4096/4; i+= 2)
    {
        *check_sum += fat_block[i] + fat_block[i+1];
        if (fat_block[i] >=max_index)
           return(-1);
        if (fat_block[i+1] == 0)
        {
            if (fat_block[i] != 0)
               return(-2);
        }
        else
	    {
            zero_flag = 1;
            if (fat_block[i+1] != 0xFFFFFFFF)
            {
                if ((fat_block[i+1] & 0x80000000) == 0)
                {
		    if (fat_block[i+1] >= max_link)
                       return(-3);
                }
                for (j=i+2; j < (4096/4); j+=2)
                {
                    if (fat_block[j+1] == fat_block[i+1])
                       return(-4);
		}
	    }
	}
    }
    return((zero_flag == 0) ? -5 : 0);
}


struct  entry_info_def
{
    ULONG   index1;
    ULONG   index2;
    ULONG   link1;
    ULONG   link2;
    ULONG   bits;
};

struct	fblock_info_def
{
	ULONG	*fat_block1;
	ULONG	*fat_block2;
    ULONG   block1_errors;
    ULONG   block2_errors;
	nwvp_lpart          *lpart;
    ULONG   block_number;
    ULONG   filler[2];
};



ULONG   nwvp_fat_table_fix(
    nwvp_vpartition	*vpartition,
    nwvp_fat_fix_info   *fix_info,
    ULONG               fix_flag)
{
    ULONG   i, j;
    ULONG   index, link;
    ULONG   block_number;
    ULONG   *fat_block1;
    ULONG   *fat_block2;
    ULONG   current_segment;
    ULONG   check_sum1;
    ULONG   check_sum2;
    ULONG   data_valid;
    ULONG   gap_count_down;
    ULONG   ending_fat_block_number;
    ULONG   cluster_fat_block_count;
    nwvp_lpart                  *lpart;
    struct  fblock_info_def     *fblock_info;
    struct  fblock_info_def     *test_info;
    ULONG   *master_fix_table;

    cluster_fat_block_count = (vpartition->cluster_count + 511) / 512;
    cluster_fat_block_count += (vpartition->blocks_per_cluster - 1);
    cluster_fat_block_count &= ~(vpartition->blocks_per_cluster - 1);

    nwvp_memset(fix_info, 0, sizeof(nwvp_fat_fix_info));
    fix_info->total_fat_blocks = cluster_fat_block_count;
    fix_info->total_entries = cluster_fat_block_count * 512;

    nwvp_alloc((void **) &master_fix_table, ((cluster_fat_block_count + 0x7F) / 0x80) * 4);
    nwvp_memset(master_fix_table, 0, ((cluster_fat_block_count + 0x7F) / 0x80) * 4);

    for (i=0; i<cluster_fat_block_count; i+=128)
    {
	nwvp_alloc((void **) &master_fix_table[i >> 7], 4096);
	nwvp_memset((void *) master_fix_table[i >> 7], 0, 4096);
    }


    gap_count_down = 0x40;
    current_segment = 0;
    block_number = vpartition->segments[current_segment].partition_block_offset;
    lpart = vpartition->segments[current_segment].lpart_link;

    for (i=0; i<cluster_fat_block_count; i++, block_number ++)
    {
	if ((i % vpartition->blocks_per_cluster) == 0)
	{
	    ending_fat_block_number = (vpartition->segments[current_segment].relative_cluster_offset +
		(vpartition->segments[current_segment].segment_block_count / vpartition->blocks_per_cluster) + 511) / 512;
	    while (i >= ending_fat_block_number)
	    {
		current_segment ++;
		block_number = vpartition->segments[current_segment].partition_block_offset;
		lpart = vpartition->segments[current_segment].lpart_link;
		gap_count_down = 0x40;
		ending_fat_block_number = (vpartition->segments[current_segment].relative_cluster_offset +
			(vpartition->segments[current_segment].segment_block_count / vpartition->blocks_per_cluster) + 511) / 512;
	    }
	}

	fblock_info =(struct fblock_info_def *)  master_fix_table[i >> 7];
	fblock_info += i & 0x7F;
	fblock_info->lpart = lpart;
	fblock_info->block_number = block_number;

	gap_count_down --;
	if (gap_count_down == 0)
	{
	    gap_count_down = 0x40;
	    block_number += 0x40;
	}
    }

    nwvp_alloc((void **) &fat_block1, 4096);
    nwvp_alloc((void **) &fat_block2, 4096);

    for (i=0; i<cluster_fat_block_count; i++)
    {
	fblock_info = (struct fblock_info_def *)  master_fix_table[i >> 7];
	fblock_info += i & 0x7F;
    lpart = fblock_info->lpart;
    block_number = fblock_info->block_number;

	data_valid = 0;
	check_sum1 = 0xFFFFFFFF;
	check_sum2 = 0xFFFFFFFF;
	if (inwvp_lpartition_block_read(lpart, block_number, 1, (BYTE *) fat_block1) == 0)
	{
	    if ((fat_block_check(fat_block1, &check_sum1, 0x01000000, vpartition->cluster_count) == 0) ||
		(nwvp_memcomp(fat_block1, zero_block_buffer, 4096) == 0))
	       data_valid |= 0x01;
	}

	if (inwvp_lpartition_block_read(lpart, block_number + 0x40, 1, (BYTE *) fat_block2) == 0)
	{
	    if ((fat_block_check(fat_block2, &check_sum2, 0x01000000, vpartition->cluster_count) == 0) ||
		(nwvp_memcomp(fat_block2, zero_block_buffer, 4096) == 0))
	       data_valid |= 0x02;
	}

	switch(data_valid)
	{
	    case    0:
		    nwvp_memset(fat_block1, 0, 4096);
		    fblock_info->fat_block1 = fat_block1;
		    fblock_info->fat_block2 = fat_block1;
		    nwvp_alloc((void **) &fat_block1, 4096);
		    fix_info->total_missing_blocks ++;
		    break;
	    case    1:
		    fblock_info->fat_block1 = fat_block1;
		    fblock_info->fat_block2 = fat_block1;
		    nwvp_alloc((void **) &fat_block1, 4096);
		    fix_info->total_valid_single_blocks ++;
		    break;
	    case    2:
		    fblock_info->fat_block1 = fat_block2;
		    fblock_info->fat_block2 = fat_block2;
		    nwvp_alloc((void **) &fat_block2, 4096);
		    fix_info->total_valid_single_blocks ++;
		    break;
	    case    3:
		    if (nwvp_memcomp(fat_block1, fat_block2, 4096) == 0)
		    {
			fblock_info->fat_block1 = fat_block1;
			fblock_info->fat_block2 = fat_block1;
			nwvp_alloc((void **) &fat_block1, 4096);
		    fix_info->total_valid_mirror_blocks ++;
		    }
		    else
		    {
			fblock_info->fat_block1 = fat_block1;
			fblock_info->fat_block2 = fat_block2;
			nwvp_alloc((void **) &fat_block1, 4096);
			nwvp_alloc((void **) &fat_block2, 4096);
	    fix_info->total_mismatch_blocks ++;
		    }
	}
    }

    nwvp_free(fat_block1);
    nwvp_free(fat_block2);

    if (fix_flag != 0)
    {
	for (i=0; i<cluster_fat_block_count; i++)
	{
		fblock_info = (struct fblock_info_def *) master_fix_table[i >> 7];
		fblock_info += i & 0x7F;

	    for (j=0; j<512; j++)
	    {
		if (fblock_info->fat_block1 != fblock_info->fat_block2)
		{
		    index = fblock_info->fat_block1[j * 2];
		    if ((link = fblock_info->fat_block1[(j * 2) + 1]) != 0)
		    {
			if ((link & 0x80000000) == 0)
			{
			    test_info = (struct fblock_info_def *) master_fix_table[link >> (7 + 9)];
			    test_info += (link >> 9) & 0x7F;
			    if ((test_info->fat_block1[(link % 512) * 2] <= index) &&
				(test_info->fat_block2[(link % 512) * 2] <= index))
				fblock_info->block1_errors ++;
                        }
		    }

                    index = fblock_info->fat_block2[j * 2];
                    if ((link = fblock_info->fat_block2[(j * 2) + 1]) != 0)
                    {
			if ((link & 0x80000000) == 0)
			{
                            test_info = (struct fblock_info_def *) master_fix_table[link >> (7 + 9)];
                            test_info += (link >> 9) & 0x7F;
                            if ((test_info->fat_block1[(link % 512) * 2] <= index) &&
                                (test_info->fat_block2[(link % 512) * 2] <= index))
				   fblock_info->block2_errors ++;
                        }
                    }

                }
                else
		{
		    index = fblock_info->fat_block1[j * 2];
                    if ((link = fblock_info->fat_block1[(j * 2) + 1]) != 0)
		    {
                        if ((link & 0x80000000) == 0)
                        {
                            test_info = (struct fblock_info_def *) master_fix_table[link >> (7 + 9)];
                            test_info += (link >> 9) & 0x7F;
			    if (test_info->fat_block1 != test_info->fat_block2)
			    {
                                if (test_info->fat_block1[(link % 512) * 2] <= index)
                                   test_info->block1_errors ++;
                                if (test_info->fat_block2[(link % 512) * 2] <= index)
                                   test_info->block2_errors ++;
			    }
                        }
                    }
		}
            }
        }


        for (i=0; i<cluster_fat_block_count; i++)
	{
	        fblock_info = (struct fblock_info_def *) master_fix_table[i >> 7];
	        fblock_info += i & 0x7F;
            lpart = fblock_info->lpart;
            block_number = fblock_info->block_number;
	    if (fblock_info->block1_errors <= fblock_info->block2_errors)
	    {
                inwvp_lpartition_block_write(lpart, block_number, 1, (BYTE *) fblock_info->fat_block1);
                inwvp_lpartition_block_write(lpart, block_number + 0x40, 1, (BYTE *) fblock_info->fat_block1);
            }
            else
	    {
                inwvp_lpartition_block_write(lpart, block_number, 1, (BYTE *) fblock_info->fat_block2);
                inwvp_lpartition_block_write(lpart, block_number + 0x40, 1, (BYTE *) fblock_info->fat_block2);
	    }
        }
        vpartition->volume_valid_flag = 0;
    }

    for (i=0; i<cluster_fat_block_count; i++)
    {
	fblock_info = (struct fblock_info_def *)  master_fix_table[i >> 7];
	    fblock_info += i & 0x7F;
	if (fblock_info->fat_block2 != fblock_info->fat_block1)
	    nwvp_free(fblock_info->fat_block2);
	nwvp_free(fblock_info->fat_block1);
    }

    if (fix_info->total_fat_blocks == fix_info->total_valid_mirror_blocks)
	 vpartition->volume_valid_flag = 0;

    for (i=0; i<cluster_fat_block_count; i+=128)
	nwvp_free((void *) master_fix_table[i >> 7]);
    nwvp_free((void *) master_fix_table);
    return(0);
}


ULONG	nwvp_vpartition_validate(
	nwvp_vpartition	**new_vpartition,
	VOLUME_TABLE_ENTRY	*ventry)
{
	ULONG	j;
	nwvp_vpartition	*vpartition;

	for (j=0; j<virtual_partition_table_count; j++)
	{
		if ((vpartition = virtual_partition_table[j]) != 0)
		{
			if ((nwvp_memcomp(&ventry->VolumeName[0], &vpartition->volume_name[0], 16) == 0) &&
				(ventry->LastVolumeSegment == (vpartition->segment_count - 1)) &&
				(((ventry->VolumeSignature >> 8) & 0xFF) == vpartition->segment_count) &&
				(cluster_block_table[ventry->VolumeSignature & 0xFF] == vpartition->blocks_per_cluster) &&
				(ventry->VolumeClusters == vpartition->cluster_count) &&
				(ventry->FirstFAT == vpartition->FAT1) &&
				(ventry->SecondFAT  == vpartition->FAT2) &&
				(ventry->FirstDirectory == vpartition->Directory1) &&
				(ventry->SecondDirectory == vpartition->Directory2))
			{
				*new_vpartition = vpartition;
				return(0);
			}
		}
	}
	return(-1);
}



ULONG	nwvp_segmentation_remove_specific(
	nwvp_lpart		*lpart,
	ULONG			segment_index)
{
	ULONG	i;

	for (i=segment_index; i<=lpart->segment_sector->NumberOfTableEntries; i++)
	{
		nwvp_memmove(&lpart->segment_sector->VolumeEntry[i], &lpart->segment_sector->VolumeEntry[i+1], sizeof(VOLUME_TABLE_ENTRY));
		vpartition_update_entry_index(lpart, lpart->segment_sector->VolumeEntry[i+1].VolumeName, i+1, i);
	}
	lpart->segment_sector->NumberOfTableEntries --;
	nwvp_segmentation_sectors_flush(lpart);
	return(0);
}

ULONG	nwvp_segmentation_remove(
	nwvp_lpart		*lpart)
{
	ULONG	i, j, k;
	nwvp_vpartition	*vpartition;

	for (j=0; j<virtual_partition_table_count; j++)
	{
		if ((vpartition = virtual_partition_table[j]) != 0)
		{
			for (i=0; i<vpartition->segment_count; i++)
			{
				if (vpartition->segments[i].lpart_link == lpart)
				{
					if (vpartition->open_flag != 0)
						inwvp_vpartition_close(vpartition);
					vpartition->volume_valid_flag = 0xFFFFFFFF;
					vpartition->segments[i].lpart_link = 0;
					vpartition->segments[i].segment_block_count = 0;
					vpartition->segments[i].partition_block_offset = 0;
					vpartition->segments[i].relative_cluster_offset = 0;
					vpartition->segments[i].volume_entry_index = 0;
					for (k=0; k<vpartition->segment_count; k++)
					{
						if (vpartition->segments[k].lpart_link != 0)
							break;
					}
					if (k == vpartition->segment_count)
					{
						virtual_partition_table[vpartition->vpartition_handle & 0x0000FFFF] = 0;
						nwvp_free(vpartition);
					}
					break;
				}
			}
		}
	}
	return(0);
}

ULONG	nwvp_segmentation_add(
	nwvp_lpart		*lpart)
{
	ULONG	i, k;
	ULONG	index;
	ULONG	valid_flag;
	ULONG	found_flag;
	nwvp_vpartition	*vpartition;

	if ((lpart->segment_sector->VolumeTableSignature[0] == 'N') &&
		(lpart->segment_sector->VolumeTableSignature[1] == 'e') &&
		(lpart->segment_sector->VolumeTableSignature[2] == 't') &&
		(lpart->segment_sector->VolumeTableSignature[3] == 'W') &&
		(lpart->segment_sector->VolumeTableSignature[4] == 'a') &&
		(lpart->segment_sector->VolumeTableSignature[5] == 'r') &&
		(lpart->segment_sector->VolumeTableSignature[6] == 'e') &&
		(lpart->segment_sector->VolumeTableSignature[7] == ' ') &&
		(lpart->segment_sector->VolumeTableSignature[8] == 'V') &&
		(lpart->segment_sector->VolumeTableSignature[9] == 'o') &&
		(lpart->segment_sector->VolumeTableSignature[10] == 'l') &&
		(lpart->segment_sector->VolumeTableSignature[11] == 'u') &&
		(lpart->segment_sector->VolumeTableSignature[12] == 'm') &&
		(lpart->segment_sector->VolumeTableSignature[13] == 'e') &&
		(lpart->segment_sector->VolumeTableSignature[14] == 's') &&
		(lpart->segment_sector->VolumeTableSignature[15] == 0))
	{
		for (i=0; i<lpart->segment_sector->NumberOfTableEntries; i++)
		{
			found_flag = 0;
			if (nwvp_vpartition_validate(&vpartition, &lpart->segment_sector->VolumeEntry[i]) == 0)
			{
				valid_flag = 0xFFFFFFFF;
				index = (lpart->segment_sector->VolumeEntry[i].VolumeSignature >> 16) & 0xFF;

				if (vpartition->segments[index].lpart_link != 0)
				{
					valid_flag = 0;
				}
				if ((index > 0) && (vpartition->segments[index-1].lpart_link != 0))
				{
					if (((vpartition->segments[index-1].relative_cluster_offset * vpartition->blocks_per_cluster) + vpartition->segments[index-1].segment_block_count) !=
						(lpart->segment_sector->VolumeEntry[i].SegmentClusterStart * vpartition->blocks_per_cluster))
					{
						valid_flag = 0;
					}
				}
				if ((index < (vpartition->segment_count - 1)) && (vpartition->segments[index+1].lpart_link != 0))
				{
					if ((vpartition->segments[index+1].relative_cluster_offset * vpartition->blocks_per_cluster) !=
						((lpart->segment_sector->VolumeEntry[i].SegmentClusterStart * vpartition->blocks_per_cluster) +
						 ((lpart->segment_sector->VolumeEntry[i].SegmentSectors / (lpart->sectors_per_block * vpartition->blocks_per_cluster)) * vpartition->blocks_per_cluster)))
					{
						valid_flag = 0;
					}
				}

				if (valid_flag != 0)
				{
					if (lpart->mirror_link != lpart)
						vpartition->mirror_flag = 0xFFFFFFFF;
					vpartition->segments[index].lpart_link = lpart;
					vpartition->segments[index].segment_block_count = (lpart->segment_sector->VolumeEntry[i].SegmentSectors / (lpart->sectors_per_block * vpartition->blocks_per_cluster)) * vpartition->blocks_per_cluster;
					vpartition->segments[index].partition_block_offset = lpart->segment_sector->VolumeEntry[i].VolumeRoot / lpart->sectors_per_block;
					vpartition->segments[index].relative_cluster_offset = lpart->segment_sector->VolumeEntry[i].SegmentClusterStart;
					vpartition->segments[index].volume_entry_index = i;
					found_flag = 1;
					for (k=0; k<vpartition->segment_count; k++)
					{
						if (vpartition->segments[k].lpart_link == 0)
							break;
					}
					if (k == vpartition->segment_count)
                    {
		       vpartition->volume_valid_flag = 0x00000000;
		    }
				}
			}
			if (found_flag == 0)
			{
				nwvp_alloc((void **) &vpartition, sizeof(nwvp_vpartition));
				nwvp_memset(vpartition, 0, sizeof(nwvp_vpartition));
			    vpartition->volume_valid_flag = 0xFFFFFFFF;

				for (k=0; k<virtual_partition_table_count; k++)
				{
					if (virtual_partition_table[k] == 0)
						break;
				}
				if (k >= MAX_VOLUMES)
				{
					nwvp_free(vpartition);
					return(-1);
				}
				if (k == virtual_partition_table_count)
				{
					virtual_partition_table_count ++;
				}
				if (lpart->mirror_link != lpart)
					vpartition->mirror_flag = 0xFFFFFFFF;
				virtual_partition_table[k] = vpartition;
				vpartition->vpartition_handle = (handle_instance ++ << 16) + k;
				index = (lpart->segment_sector->VolumeEntry[i].VolumeSignature >> 16) & 0xFF;
				vpartition->segment_count = (lpart->segment_sector->VolumeEntry[i].VolumeSignature >> 8) & 0xFF;
				vpartition->cluster_count = lpart->segment_sector->VolumeEntry[i].VolumeClusters;
				nwvp_memmove(&vpartition->volume_name[0], &lpart->segment_sector->VolumeEntry[i].VolumeName[0], 16);
				vpartition->FAT1 = lpart->segment_sector->VolumeEntry[i].FirstFAT;
				vpartition->FAT2 = lpart->segment_sector->VolumeEntry[i].SecondFAT;
				vpartition->Directory1 = lpart->segment_sector->VolumeEntry[i].FirstDirectory;
				vpartition->Directory2 = lpart->segment_sector->VolumeEntry[i].SecondDirectory;
				vpartition->blocks_per_cluster = cluster_block_table[lpart->segment_sector->VolumeEntry[i].VolumeSignature & 0xFF];
				vpartition->segments[index].lpart_link = lpart;
				vpartition->segments[index].segment_block_count = (lpart->segment_sector->VolumeEntry[i].SegmentSectors / (lpart->sectors_per_block * vpartition->blocks_per_cluster)) * vpartition->blocks_per_cluster;
				vpartition->segments[index].partition_block_offset = lpart->segment_sector->VolumeEntry[i].VolumeRoot / lpart->sectors_per_block;
				vpartition->segments[index].relative_cluster_offset = lpart->segment_sector->VolumeEntry[i].SegmentClusterStart;
				vpartition->segments[index].volume_entry_index = i;
				if (vpartition->segment_count == 1)
				{
					vpartition->volume_valid_flag = 0x00000000;
				}
			}
		}
	}
	else
	{
		nwvp_memset(lpart->segment_sector, 0, 512);
		lpart->segment_sector->VolumeTableSignature[0] = 'N';
		lpart->segment_sector->VolumeTableSignature[1] = 'e';
		lpart->segment_sector->VolumeTableSignature[2] = 't';
		lpart->segment_sector->VolumeTableSignature[3] = 'W';
		lpart->segment_sector->VolumeTableSignature[4] = 'a';
		lpart->segment_sector->VolumeTableSignature[5] = 'r';
		lpart->segment_sector->VolumeTableSignature[6] = 'e';
		lpart->segment_sector->VolumeTableSignature[7] = ' ';
		lpart->segment_sector->VolumeTableSignature[8] = 'V';
		lpart->segment_sector->VolumeTableSignature[9] = 'o';
		lpart->segment_sector->VolumeTableSignature[10] = 'l';
		lpart->segment_sector->VolumeTableSignature[11] = 'u';
		lpart->segment_sector->VolumeTableSignature[12] = 'm';
		lpart->segment_sector->VolumeTableSignature[13] = 'e';
		lpart->segment_sector->VolumeTableSignature[14] = 's';
		lpart->segment_sector->VolumeTableSignature[15] = 0;
	}
	return(0);
}

ULONG	nwvp_segmentation_sectors_flush(
	nwvp_lpart	*lpart)
{
	ULONG	i;
	ULONG	error_count = 0;
	BYTE	*temp_buffer;

	lpart->segment_sector->VolumeTableSignature[0] = 'N';
	lpart->segment_sector->VolumeTableSignature[1] = 'e';
	lpart->segment_sector->VolumeTableSignature[2] = 't';
	lpart->segment_sector->VolumeTableSignature[3] = 'W';
	lpart->segment_sector->VolumeTableSignature[4] = 'a';
	lpart->segment_sector->VolumeTableSignature[5] = 'r';
	lpart->segment_sector->VolumeTableSignature[6] = 'e';
	lpart->segment_sector->VolumeTableSignature[7] = ' ';
	lpart->segment_sector->VolumeTableSignature[8] = 'V';
	lpart->segment_sector->VolumeTableSignature[9] = 'o';
	lpart->segment_sector->VolumeTableSignature[10] = 'l';
	lpart->segment_sector->VolumeTableSignature[11] = 'u';
	lpart->segment_sector->VolumeTableSignature[12] = 'm';
	lpart->segment_sector->VolumeTableSignature[13] = 'e';
	lpart->segment_sector->VolumeTableSignature[14] = 's';
	lpart->segment_sector->VolumeTableSignature[15] = 0;
	nwvp_io_alloc((void **) &temp_buffer, 4096);
	nwvp_memset(temp_buffer, 0, 4096);
	for (i=4; i<=16; i+= 4)
	{
		if (inwvp_lpartition_block_read(
				lpart,
				i,
				1,
				(BYTE *) temp_buffer) == 0)
		{
			nwvp_memmove(temp_buffer, lpart->segment_sector, 512);
			if (inwvp_lpartition_block_write(
					lpart,
					i,
					1,
					(BYTE *) temp_buffer) != 0)
				error_count ++;
		}
		else
			error_count ++;
	}
	nwvp_io_free(temp_buffer);
	return(error_count);
}

ULONG	nwvp_segmentation_sector_validate(
	VOLUME_TABLE	*seg_sector)
{
	if ((seg_sector->VolumeTableSignature[0] == 'N') &&
		(seg_sector->VolumeTableSignature[1] == 'e') &&
		(seg_sector->VolumeTableSignature[2] == 't') &&
		(seg_sector->VolumeTableSignature[3] == 'W') &&
		(seg_sector->VolumeTableSignature[4] == 'a') &&
		(seg_sector->VolumeTableSignature[5] == 'r') &&
		(seg_sector->VolumeTableSignature[6] == 'e') &&
		(seg_sector->VolumeTableSignature[7] == ' ') &&
		(seg_sector->VolumeTableSignature[8] == 'V') &&
		(seg_sector->VolumeTableSignature[9] == 'o') &&
		(seg_sector->VolumeTableSignature[10] == 'l') &&
		(seg_sector->VolumeTableSignature[11] == 'u') &&
		(seg_sector->VolumeTableSignature[12] == 'm') &&
		(seg_sector->VolumeTableSignature[13] == 'e') &&
		(seg_sector->VolumeTableSignature[14] == 's'))
	{
		return(0);
	}
	return(-1);
}

ULONG	nwvp_segmentation_sectors_load(
	nwvp_lpart	*lpart)
{
	ULONG	i;
	BYTE	*temp_buffer;
	ULONG	valid_flag = 0;
	ULONG	invalid_count = 0;

	nwvp_io_alloc((void **) &temp_buffer, 4096);
	nwvp_memset(temp_buffer, 0, 4096);
	if (lpart->segment_sector == 0)
		nwvp_alloc((void **) &lpart->segment_sector, 512);
	for (i=0; i<4; i++)
	{
		if (inwvp_lpartition_block_read(
				lpart,
				(4 + (i * 4)),
				1,
				(BYTE *) temp_buffer) == 0)
		{
			if (valid_flag == 0)
			{
				if (nwvp_segmentation_sector_validate((VOLUME_TABLE * ) temp_buffer) == 0)
					valid_flag = 1;
				nwvp_memmove(lpart->segment_sector, temp_buffer, 512);
			}
			else
			{
				if (nwvp_memcomp(lpart->segment_sector, temp_buffer, 512) != 0)
					invalid_count ++;
			}
		}
		else
			invalid_count ++;
	}
    nwvp_io_free(temp_buffer);
	if (valid_flag != 0)
	{
		if (invalid_count != 0)
			nwvp_segmentation_sectors_flush(lpart);
		return(0);
	}
	return(-1);
}

ULONG	nwvp_master_sectors_flush(
	nwvp_lpart	*lpart)
{
	ULONG	i;
	BYTE	*temp_buffer;

    if ((lpart->mirror_status & MIRROR_READ_ONLY_BIT) == 0)
    {
	nwvp_io_alloc((void **) &temp_buffer, 4096);
	    if (nwvp_part_read(
        		lpart->rpart_handle,
		        0x20,
				8,
				temp_buffer) == 0)
        {
		    nwvp_memmove(&temp_buffer[0], lpart->hotfix.sector, 512);
            nwvp_memmove(&temp_buffer[512], lpart->netware_mirror_sector, 512);
		    nwvp_memmove(&temp_buffer[1024], lpart->mirror_sector, 512);
		    for (i=0x20; i<0x81; i+= 0x20)
		    {
			    if (nwvp_part_write(
					lpart->rpart_handle,
					i,
					8,
					temp_buffer) != 0)
			    {
			    }
            }
        }
	    nwvp_io_free(temp_buffer);
    }
	return(0);
}


ULONG	nwvp_netware_mirror_update(
	nwvp_lpart	*lpart,
	ULONG		flush_flag)
{
	ULONG	i, j;
	nwvp_lpart	*temp;

	nwvp_memset(lpart->netware_mirror_sector, 0, 512);

	lpart->netware_mirror_sector->MirrorStamp[0] = 'M';
	lpart->netware_mirror_sector->MirrorStamp[1] = 'I';
	lpart->netware_mirror_sector->MirrorStamp[2] = 'R';
	lpart->netware_mirror_sector->MirrorStamp[3] = 'R';
	lpart->netware_mirror_sector->MirrorStamp[4] = 'O';
	lpart->netware_mirror_sector->MirrorStamp[5] = 'R';
	lpart->netware_mirror_sector->MirrorStamp[6] = '0';
	lpart->netware_mirror_sector->MirrorStamp[7] = '0';
	lpart->netware_mirror_sector->PartitionID = lpart->mirror_partition_id;
	lpart->netware_mirror_sector->Reserved1 = 0x0703f808;;
	lpart->netware_mirror_sector->MirrorStatus = NETWARE_5_BASE_BIT;
	lpart->netware_mirror_sector->MirrorFlags = NETWARE_IN_SYNCH_INSTANCE;
	if (((lpart->primary_link->mirror_status & MIRROR_GROUP_OPEN_BIT) != 0) ||
		((lpart->primary_link->mirror_status & MIRROR_GROUP_CHECK_BIT) != 0))
		lpart->netware_mirror_sector->MirrorFlags |= NETWARE_INUSE_BIT;
	lpart->netware_mirror_sector->MirrorTotalSectors = lpart->hotfix.sector->HotFixTotalSectors;
	lpart->netware_mirror_sector->MirrorGroupID = lpart->mirror_group_id;

	for (i=0, j=0; i<30; i++)
	{
		if ((lpart->mirror_sector->log[i].partition_id != 0) &&
			(lpart->mirror_sector->log[i].status != NWVP_MIRROR_STATUS_REMOVED) &&
			(lpart->mirror_sector->log[i].status != NWVP_MIRROR_STATUS_BAD))
		{
#if (DEBUG_MIRROR_CONTROL)
NWFSPrint("flush_flag = %d lpart = %x i = %d status = %x \n", 
	  (int) flush_flag, (unsigned) lpart, 
	  (int)i, (unsigned)lpart->mirror_sector->log[i].status);
#endif

			lpart->netware_mirror_sector->MirrorMemberID[j] = lpart->mirror_sector->log[i].partition_id;
			temp = lpart;
			do
			{
				if (temp->mirror_partition_id == lpart->mirror_sector->log[i].partition_id)
				{
					if (temp->remirror_section != 0)
                                        {
#if (DEBUG_MIRROR_CONTROL)
NWFSPrint("out of synch guy \n");
#endif
						lpart->netware_mirror_sector->MirrorStatus |= 1 << j;
                                        }
				}
				temp = temp->mirror_link;
			} while (temp != lpart);
			j++;
		}
	}

	temp = lpart;
	do
	{
		temp = temp->mirror_link;
	} while (temp != lpart);
	if (flush_flag != 0)
		nwvp_master_sectors_flush(lpart);
	return(0);
}

ULONG	nwvp_segment_sectors_zap(
	nwvp_lpart	*lpart)
{
	nwvp_part_write(lpart->rpart_handle, (lpart->hotfix.block_count * lpart->sectors_per_block) + 0x20, lpart->sectors_per_block, zero_block_buffer);
	nwvp_part_write(lpart->rpart_handle, (lpart->hotfix.block_count * lpart->sectors_per_block) + 0x40, lpart->sectors_per_block, zero_block_buffer);
	nwvp_part_write(lpart->rpart_handle, (lpart->hotfix.block_count * lpart->sectors_per_block) + 0x60, lpart->sectors_per_block, zero_block_buffer);
	nwvp_part_write(lpart->rpart_handle, (lpart->hotfix.block_count * lpart->sectors_per_block) + 0x80, lpart->sectors_per_block, zero_block_buffer);
	return(0);
}

ULONG	nwvp_hotfix_sectors_zap(
	nwvp_lpart	*lpart)
{
	nwvp_part_write(lpart->rpart_handle, 0x20, lpart->sectors_per_block, zero_block_buffer);
	nwvp_part_write(lpart->rpart_handle, 0x40, lpart->sectors_per_block, zero_block_buffer);
	nwvp_part_write(lpart->rpart_handle, 0x60, lpart->sectors_per_block, zero_block_buffer);
	nwvp_part_write(lpart->rpart_handle, 0x80, lpart->sectors_per_block, zero_block_buffer);
	return(0);
}

ULONG	nwvp_hotfix_tables_flush(
	nwvp_lpart	*lpart,
	ULONG		table_index)
{
	ULONG	error_count = 0;
	ULONG	index;
    ULONG	*table_entry;
	ULONG	current_index;
	ULONG	master_flush_flag = 0;
	ULONG	*data_buffer = 0;

    if ((lpart->mirror_status & MIRROR_READ_ONLY_BIT) != 0)
        return(0);

	current_index = table_index >> 10;
	while (error_count < 5)
	{
		if (nwvp_part_write(
				lpart->rpart_handle,
				lpart->hotfix.sector->HotFixTable[current_index].HotFix1,
				lpart->sectors_per_block,
				(BYTE *) lpart->hotfix.hotfix_table[current_index]) == 0)
		{
			if (nwvp_part_write(
					lpart->rpart_handle,
					lpart->hotfix.sector->HotFixTable[current_index].HotFix2,
					lpart->sectors_per_block,
					(BYTE *) lpart->hotfix.hotfix_table[current_index]) == 0)
			{
				if (nwvp_part_write(
						lpart->rpart_handle,
						lpart->hotfix.sector->HotFixTable[current_index].BadBlock1,
						lpart->sectors_per_block,
						(BYTE *) lpart->hotfix.bad_bit_table[current_index]) == 0)
				{
					if (nwvp_part_write(
						lpart->rpart_handle,
						lpart->hotfix.sector->HotFixTable[current_index].BadBlock2,
						lpart->sectors_per_block,
						(BYTE *) lpart->hotfix.bad_bit_table[current_index]) == 0)
					{
						if (data_buffer != 0)
							nwvp_free(data_buffer);
						if (master_flush_flag != 0)
							nwvp_master_sectors_flush(lpart);
						return(0);
					}
					table_entry = &lpart->hotfix.sector->HotFixTable[current_index].BadBlock2;
				}
				else
				{
					table_entry = &lpart->hotfix.sector->HotFixTable[current_index].BadBlock1;
				}
			}
			else
			{
					table_entry = &lpart->hotfix.sector->HotFixTable[current_index].HotFix2;
			}
		}
		else
		{
			table_entry = &lpart->hotfix.sector->HotFixTable[current_index].HotFix1;
		}

		error_count ++;
		if (data_buffer == 0)
			nwvp_io_alloc((void **) &data_buffer, 4096);

		if (nwvp_part_read(lpart->rpart_handle, 0, lpart->sectors_per_block, (BYTE *) data_buffer) != 0)
			break;

		for (index = 0; index < lpart->hotfix.block_count; index ++)
		{
			if (lpart->hotfix.hotfix_table[index >> 10][index & 0x3FF] == 0)
			{
				lpart->hotfix.hotfix_table[index >> 10][index & 0x3FF] = 0xFFFFFFFF;
				lpart->hotfix.bad_bit_table[index >> 10][index & 0x3FF] = 0xFFFFFFFF;
				*table_entry = index * lpart->sectors_per_block;
				master_flush_flag = 1;
				break;
			}
		}

		if (index == lpart->hotfix.block_count)
			break;
	}
	nwvp_log(NWLOG_DEVICE_OFFLINE, lpart->lpartition_handle, lpart->rpart_handle, 0);
	lpart->rpart_handle = 0xFFFFFFFF;
	lpart->mirror_status &= ~(MIRROR_PRESENT_BIT | MIRROR_REMIRRORING_BIT);
	if (nwvp_mirror_group_update(lpart->primary_link) != 0)
	{
		nwvp_mirror_group_delete(lpart->primary_link);
	}
	if (data_buffer != 0)
	nwvp_io_free(data_buffer);
	return(-1);
}

ULONG	nwvp_hotfix_tables_load(
	nwvp_lpart	*lpart)
{
	ULONG	i;

	for (i=0; i<((lpart->hotfix.block_count + 0x3FF) >> 10); i++)
	{
		if (nwvp_part_read(
				lpart->rpart_handle,
				lpart->hotfix.sector->HotFixTable[i].HotFix1,
				lpart->sectors_per_block,
				(BYTE *) lpart->hotfix.hotfix_table[i]) != 0)
		{
			return(-1);
		}
		if (nwvp_part_read(
				lpart->rpart_handle,
				lpart->hotfix.sector->HotFixTable[i].HotFix2,
				lpart->sectors_per_block,
				(BYTE *) lpart->hotfix.hotfix_table[i]) != 0)
		{
			return(-1);
		}
		if (nwvp_part_read(
				lpart->rpart_handle,
				lpart->hotfix.sector->HotFixTable[i].BadBlock1,
				lpart->sectors_per_block,
				(BYTE *) lpart->hotfix.bad_bit_table[i]) != 0)
		{
			return(-1);
		}
		if (nwvp_part_read(
				lpart->rpart_handle,
				lpart->hotfix.sector->HotFixTable[i].BadBlock2,
				lpart->sectors_per_block,
				(BYTE *) lpart->hotfix.bad_bit_table[i]) != 0)
		{
			return(-1);
		}
	}
	return(0);
}

ULONG	nwvp_hotfix_tables_alloc(
	nwvp_lpart		*lpart)
{
	ULONG	i;
	if (lpart->hotfix.hotfix_table == 0)
	{
		nwvp_alloc((void **) &lpart->hotfix.hotfix_table, ((lpart->hotfix.block_count + 0x3FF) >> 10) * 4);
		nwvp_memset(lpart->hotfix.hotfix_table, 0, ((lpart->hotfix.block_count + 0x3FF) >> 10) * 4);
	}
	if (lpart->hotfix.bad_bit_table == 0)
	{
		nwvp_alloc((void **) &lpart->hotfix.bad_bit_table, ((lpart->hotfix.block_count + 0x3FF) >> 10) * 4);
		nwvp_memset(lpart->hotfix.bad_bit_table, 0, ((lpart->hotfix.block_count + 0x3FF) >> 10) * 4);
	}
	if (lpart->hotfix.bit_table == 0)
	{
		nwvp_alloc((void **) &lpart->hotfix.bit_table, ((lpart->logical_block_count + 0x7FFF) >> 15) * 4);
		nwvp_memset(lpart->hotfix.bit_table, 0, ((lpart->logical_block_count + 0x7FFF) >> 15) * 4);
	}
	if (lpart->hotfix.sector == 0)
	{
		nwvp_alloc((void **) &lpart->hotfix.sector, 512);
		nwvp_memset(lpart->hotfix.sector, 0, 512);
	}

	if (lpart->segment_sector == 0)
	{
		nwvp_alloc((void **) &lpart->segment_sector, 512);
		nwvp_memset(lpart->segment_sector, 0, 512);
	}
	for (i=0; i<lpart->hotfix.block_count; i+= 0x400)
	{
		if (lpart->hotfix.hotfix_table[i >> 10] == 0)
		{
			nwvp_io_alloc((void **) &(lpart->hotfix.hotfix_table[i >> 10]), 4096);
			nwvp_memset(lpart->hotfix.hotfix_table[i >> 10], 0, 4096);
		}
		if (lpart->hotfix.bad_bit_table[i >> 10] == 0)
		{
			nwvp_io_alloc((void **) &(lpart->hotfix.bad_bit_table[i >> 10]), 4096);
			nwvp_memset(lpart->hotfix.bad_bit_table[i >> 10], 0, 4096);
		}
	}
	for (i=0; i<lpart->logical_block_count; i+= 0x8000)
	{
		if (lpart->hotfix.bit_table[i >> 15] == 0)
		{
			nwvp_io_alloc((void **) &lpart->hotfix.bit_table[i >> 15], 4096);
			nwvp_memset(lpart->hotfix.bit_table[i >> 15], 0, 4096);
		}
	}
	return(0);
}

ULONG	nwvp_hotfix_tables_free(
	nwvp_lpart		*lpart)
{
	ULONG	i;
	if (lpart->hotfix.hotfix_table != 0)
	{
		for (i=0; i<lpart->hotfix.block_count; i+= 0x400)
		{
			if (lpart->hotfix.hotfix_table[i >> 10] != 0)
				nwvp_io_free(lpart->hotfix.hotfix_table[i >> 10]);
		}
		nwvp_free(lpart->hotfix.hotfix_table);
		lpart->hotfix.hotfix_table = 0;
	}

	if (lpart->hotfix.bad_bit_table != 0)
	{
		for (i=0; i<lpart->hotfix.block_count; i+= 0x400)
		{
			if (lpart->hotfix.bad_bit_table[i >> 10] != 0)
			nwvp_io_free(lpart->hotfix.bad_bit_table[i >> 10]);
		}
		nwvp_free(lpart->hotfix.bad_bit_table);
		lpart->hotfix.bad_bit_table = 0;
	}

	if (lpart->hotfix.bit_table != 0)
	{
		for (i=0; i<lpart->logical_block_count; i+= 0x8000)
		{
			nwvp_io_free(lpart->hotfix.bit_table[i >> 15]);
		}
		nwvp_free(lpart->hotfix.bit_table);
		lpart->hotfix.bit_table = 0;
	}

	if (lpart->hotfix.sector != 0)
	{
		nwvp_free(lpart->hotfix.sector);
		lpart->hotfix.sector = 0;
	}

	if (lpart->segment_sector != 0)
	{
		nwvp_free(lpart->segment_sector);
		lpart->segment_sector = 0;
	}
	return(0);
}

ULONG	nwvp_hotfix_destroy(
	nwvp_lpart	*lpart)
{
	ULONG	i;
	BYTE	*temp_buffer;

	nwvp_io_alloc((void **) &temp_buffer, 4096);
	nwvp_memset(temp_buffer, 0, 4096);

	lpart->remirror_section = 0xFFFFFFFF;
	lpart->lpartition_handle = 0xFFFFFFFF;
	lpart->mirror_partition_id = 0xFFFFFFFe;
	lpart->mirror_group_id = 0xFFFFFFFd;

	nwvp_hotfix_tables_free(lpart);
	nwvp_memset(lpart->netware_mirror_sector, 0, 512);
	nwvp_memset(lpart->mirror_sector, 0, 512);
	for (i=0x20; i<0x81; i+= 0x20)
	{
		nwvp_part_write(
				lpart->rpart_handle,
				i,
				8,
				temp_buffer);
	}
	nwvp_io_free(temp_buffer);
	return(0);
}

ULONG	nwvp_hotfix_and_mirror_validate(
	BYTE	*buffer,
	ULONG	capacity)
{
	HOTFIX	*hf_sector;
	MIRROR	*mr_sector;

	hf_sector = (HOTFIX *) &buffer[0];
	mr_sector = (MIRROR *) &buffer[512];
	if ((nwvp_memcomp(&hf_sector->HotFixStamp[0], "HOTFIX00", 8) == 0) &&
		(nwvp_memcomp(&mr_sector->MirrorStamp[0], "MIRROR00", 8) == 0))
	{
		if ((hf_sector->HotFixTotalSectors + hf_sector->HotFixSize) <= capacity)
		{
			return(0);
		}
	}
	return(-1);
}

ULONG	nwvp_hotfix_scan(
	nwvp_lpart	*lpart)
{
	ULONG	i;
	ULONG	block_number;
	ULONG	*table;
	BYTE	*table_buffer;
	HOTFIX	*hf_sector;

	nwvp_io_alloc((void **) &table_buffer, 4096);
	hf_sector = (HOTFIX *) &table_buffer[0];
	if (nwvp_part_read(lpart->rpart_handle,
			   4 * lpart->sectors_per_block,
			   lpart->sectors_per_block,
				(BYTE *) hf_sector) == 0)
	{
		if (nwvp_hotfix_and_mirror_validate(table_buffer, lpart->physical_block_count * lpart->sectors_per_block) == 0)
		{
			lpart->logical_block_count = hf_sector->HotFixTotalSectors / lpart->sectors_per_block;
			lpart->hotfix.block_count = hf_sector->HotFixSize / lpart->sectors_per_block;
			nwvp_hotfix_tables_alloc(lpart);
			nwvp_memmove(lpart->hotfix.sector, &table_buffer[0], 512);
			if ((lpart->primary_link->mirror_status & MIRROR_GROUP_ESTABLISHED_BIT) == 0)
			{
				nwvp_memmove(lpart->netware_mirror_sector, &table_buffer[512], 512);
				nwvp_memmove(lpart->mirror_sector, &table_buffer[1024], 512);
			}
			nwvp_hotfix_tables_load(lpart);
			for (i=0; i<lpart->hotfix.block_count; i++)
			{
				if ((table = lpart->hotfix.hotfix_table[i >> 10]) != 0)
				{
					if ((table[i & 0x3FF] != 0) && (table[i & 0x3FF] != 0xFFFFFFFF))
					{
						block_number = table[i & 0x3FF] / 8;
						lpart->hotfix.bit_table[block_number >> 15][(block_number >> 5) & 0x3FF] |= (1 << (block_number & 0x1F));
					}
				}
			}
			nwvp_io_free(table_buffer);
			return(0);
		}
	}
	nwvp_io_free(table_buffer);
	return(0);
}

ULONG	nwvp_hotfix_create(
	nwvp_lpart	*lpart,
	ULONG		logical_block_count)
{
	ULONG		i, j, index;

	lpart->logical_block_count = logical_block_count;
	if ((lpart->hotfix.block_count = lpart->physical_block_count - logical_block_count) > 0x4000)
		lpart->hotfix.block_count = 0x4000;
	nwvp_hotfix_tables_alloc(lpart);
	lpart->hotfix.sector->HotFixStamp[0] = 'H';
	lpart->hotfix.sector->HotFixStamp[1] = 'O';
	lpart->hotfix.sector->HotFixStamp[2] = 'T';
	lpart->hotfix.sector->HotFixStamp[3] = 'F';
	lpart->hotfix.sector->HotFixStamp[4] = 'I';
	lpart->hotfix.sector->HotFixStamp[5] = 'X';
	lpart->hotfix.sector->HotFixStamp[6] = '0';
	lpart->hotfix.sector->HotFixStamp[7] = '0';

	lpart->hotfix.sector->PartitionID = nwvp_create_new_id();
	lpart->hotfix.sector->HotFixFlags = 0x00010000;
	lpart->hotfix.sector->HotFixDateStamp = 0x0703f808;
	lpart->hotfix.sector->HotFixTotalSectors = lpart->logical_block_count * lpart->sectors_per_block;
	lpart->hotfix.sector->HotFixSize = lpart->hotfix.block_count * lpart->sectors_per_block;

	for (i=0; i<20; i++)
	{
		lpart->hotfix.hotfix_table[0][i] = 0xFFFFFFFF;
		lpart->hotfix.bad_bit_table[0][i] = 0xFFFFFFFF;
	}
	for (i=0; i<lpart->hotfix.block_count; i+= 0x400)
	{
		index = 20 + ((i >> 10) << 5);
		for (j=0; j<4; j++, index+=8)
		{
			lpart->hotfix.hotfix_table[index >> 10][index & 0x3FF] = 0xFFFFFFFF;
			lpart->hotfix.bad_bit_table[index >> 10][index & 0x3FF] = 0xFFFFFFFF;
		}
		lpart->hotfix.sector->HotFixTable[i >> 10].HotFix1 = (index - 0x20) * 8;
		lpart->hotfix.sector->HotFixTable[i >> 10].BadBlock1 = (index - 0x18) * 8;
		lpart->hotfix.sector->HotFixTable[i >> 10].HotFix2 = (index - 0x10) * 8;
		lpart->hotfix.sector->HotFixTable[i >> 10].BadBlock2 = (index - 0x08) * 8;
		nwvp_hotfix_tables_flush(lpart, i);
	}

	nwvp_memmove(&lpart->mirror_sector->nwvp_mirror_stamp[0], "NWVP MIRROR 0001", 16);
	lpart->mirror_sector->mirror_id = nwvp_create_new_id();
	lpart->mirror_sector->time_stamp = nwvp_get_date_and_time();
	lpart->mirror_sector->log[0].partition_id = lpart->hotfix.sector->PartitionID;
	lpart->mirror_link = lpart;
	lpart->primary_link = lpart;
	lpart->mirror_status = MIRROR_PRESENT_BIT | MIRROR_INSYNCH_BIT;
	lpart->mirror_partition_id = lpart->hotfix.sector->PartitionID;
	lpart->mirror_group_id = lpart->mirror_sector->mirror_id;
	nwvp_segment_sectors_zap(lpart);
	nwvp_netware_mirror_update(lpart, 0);
	nwvp_master_sectors_flush(lpart);
	return(0);
}

ULONG	nwvp_hotfix_block_lookup(
	nwvp_lpart	*lpart,
	ULONG		block_number,
	ULONG		*relative_block_number,
	ULONG		*bad_bits)
{
	ULONG	i;
	ULONG	*table;

	for (i=0; i<lpart->hotfix.block_count; i++)
	{
		if ((table = lpart->hotfix.hotfix_table[i >> 10]) != 0)
		{
			if (table[i & 0x3FF] == (block_number * 8))
			{
				*relative_block_number = i;
				*bad_bits = lpart->hotfix.bad_bit_table[i >> 10][i & 0x3FF];
				return(0);
			}
		}
	}
	return(-1);
}


ULONG	nwvp_hotfix_block(
	nwvp_lpart	*original_part,
	ULONG		original_block_number,
	BYTE		*original_buffer,
	ULONG		*original_bad_bits,
	ULONG		control_flag)
{
	ULONG		i;
	ULONG		index;
	ULONG		error_count;
	ULONG		data_found_flag;
	ULONG		multi_sector_count;
	ULONG		bytes;
	ULONG		bad_bits = 0;
	ULONG		relative_block_number = 0;
	nwvp_lpart	*lpart;

	lpart = original_part;
	NWLockHOTFIX();

//
//  	retry original operation to prevent concurrent access of a single
//		block to cause multiple hotfixes.
//

	bad_bits = (control_flag == NWVP_HOTFIX_FORCE_BAD) ? 0xFFFFFFFF : 0;

	if ((control_flag == NWVP_HOTFIX_READ) || (control_flag == NWVP_HOTFIX_FORCE_BAD))
	{
		if (lpart->hotfix.bit_table[original_block_number >> 15][(original_block_number >> 5) & 0x3FF] & (1 << (original_block_number & 0x1F)))
			nwvp_hotfix_block_lookup(lpart, original_block_number, &relative_block_number, &bad_bits);
		else
			relative_block_number = original_block_number + lpart->hotfix.block_count;

		if (nwvp_part_read(
				lpart->rpart_handle,
				relative_block_number * lpart->sectors_per_block,
				lpart->sectors_per_block,
				original_buffer) == 0)
		{
			*original_bad_bits = bad_bits;
			NWUnlockHOTFIX();
			return(0);
		}
	}
	if (control_flag == NWVP_HOTFIX_WRITE)
	{
		if (lpart->hotfix.bit_table[original_block_number >> 15][(original_block_number >> 5) & 0x3FF] & (1 << (original_block_number & 0x1F)))
			nwvp_hotfix_block_lookup(lpart, original_block_number, &relative_block_number, &bad_bits);
		else
			relative_block_number = original_block_number + lpart->hotfix.block_count;
		if (nwvp_part_write(
				lpart->rpart_handle,
				relative_block_number * lpart->sectors_per_block,
				lpart->sectors_per_block,
				original_buffer) == 0)
		{
			*original_bad_bits |= bad_bits;
			NWUnlockHOTFIX();
			return(0);
		}
	}

//
//	try to read from block 0, to detect device failure
//
	if ((control_flag == NWVP_HOTFIX_READ) || (control_flag == NWVP_HOTFIX_WRITE))
    {
        if (nwvp_part_read(
    		lpart->rpart_handle,
	        0,
		    lpart->sectors_per_block,
		    bit_bucket_buffer) != 0)
      	{
		    nwvp_log(NWLOG_DEVICE_OFFLINE, lpart->lpartition_handle, lpart->rpart_handle, 0);
		    lpart->rpart_handle = 0xFFFFFFFF;
		    lpart->mirror_status &= ~(MIRROR_PRESENT_BIT | MIRROR_REMIRRORING_BIT);
		    if (nwvp_mirror_group_update(lpart->primary_link) != 0)
		    {
			   nwvp_mirror_group_delete(lpart->primary_link);
		    }
		    NWUnlockHOTFIX();
		    return(-1);
        }
	}
//
//	try to recover data on read operation from mirrors
//
	if ((control_flag == NWVP_HOTFIX_READ) || (control_flag == NWVP_HOTFIX_FORCE))
	{
		lpart = original_part->mirror_link;
		data_found_flag = 0;
		while (lpart != original_part)
		{
			if ((((lpart->mirror_status & MIRROR_INSYNCH_BIT) != 0) &&
				 ((lpart->mirror_status & MIRROR_PRESENT_BIT) != 0)) ||
				(lpart->remirror_section < (original_block_number >> 10)))
			{
				bad_bits = 0;
				if (lpart->hotfix.bit_table[original_block_number >> 15][(original_block_number >> 5) & 0x3FF] & (1 << (original_block_number & 0x1F)))
					nwvp_hotfix_block_lookup(lpart, original_block_number, &relative_block_number, &bad_bits);
				else
					relative_block_number = original_block_number + lpart->hotfix.block_count;
				if (bad_bits == 0)
				{
					if (nwvp_part_read(
							lpart->rpart_handle,
							relative_block_number * lpart->sectors_per_block,
							lpart->sectors_per_block,
							original_buffer) == 0)
					{
						data_found_flag = 1;
						break;
					}
				}
			}
			lpart = lpart->mirror_link;
		}
		if (data_found_flag == 0)
		{
//
//	otherwise retry one sector at a time.
//
			bad_bits = 0;
			if (lpart->hotfix.bit_table[original_block_number >> 15][(original_block_number >> 5) & 0x3FF] & (1 << (original_block_number & 0x1F)))
				nwvp_hotfix_block_lookup(lpart, original_block_number, &relative_block_number, &bad_bits);
			else
				relative_block_number = original_block_number + lpart->hotfix.block_count;
			multi_sector_count = LogicalBlockSize / 512;
			for (bytes = 0, i=0; i<8; i+=multi_sector_count, bytes += (512 * multi_sector_count))
			{
				if (nwvp_part_read(
						lpart->rpart_handle,
						(relative_block_number * lpart->sectors_per_block) + i,
						multi_sector_count,
						&original_buffer[bytes]) != 0)
				{
					bad_bits |= (~(0xFFFFFFFF << multi_sector_count)) << i;
				}
			}
		}
	}

//
//  now that we have the data and the bad bits, we can hotfix
//
//  first locate a free entry in the table;
//

	lpart = original_part;
	index = 0;
	relative_block_number = 0xFFFFFFFF;
	if (lpart->hotfix.bit_table[original_block_number >> 15][(original_block_number >> 5) & 0x3FF] & (1 << (original_block_number & 0x1F)))
	{
		nwvp_hotfix_block_lookup(original_part, original_block_number, &relative_block_number, &bad_bits);
		index = relative_block_number;
		lpart->hotfix.hotfix_table[relative_block_number >> 10][relative_block_number & 0x3FF] = 0xFFFFFFFF;
		lpart->hotfix.bad_bit_table[relative_block_number >> 10][relative_block_number & 0x3FF] = 0xFFFFFFFF;
	}

//
//	write the data to the new location and flush tables
//

	for (error_count = 0; (index < lpart->hotfix.block_count) && (error_count < 5); index ++)
	{
		if (lpart->hotfix.hotfix_table[index >> 10][index & 0x3FF] == 0)
		{
			if (((lpart->mirror_status & MIRROR_READ_ONLY_BIT) != 0) ||
		(nwvp_part_write(
						original_part->rpart_handle,
						(index * original_part->sectors_per_block),
						original_part->sectors_per_block,
						&original_buffer[0]) == 0))
			{
				lpart->hotfix.hotfix_table[index >> 10][index & 0x3FF] = original_block_number * original_part->sectors_per_block;
				lpart->hotfix.bad_bit_table[index >> 10][index & 0x3FF] = bad_bits;
				lpart->hotfix.bit_table[original_block_number >> 15][(original_block_number >> 5) & 0x3FF] |= (1 << (original_block_number & 0x1F));
				if ((relative_block_number != 0xFFFFFFFF) && ((relative_block_number & 0xFFFFFC00) != (index & 0xFFFFFC00)))
					nwvp_hotfix_tables_flush(lpart, relative_block_number);
				if (nwvp_hotfix_tables_flush(lpart, index) == 0)
				{
					*original_bad_bits = bad_bits;
					NWUnlockHOTFIX();
					return(0);
				}

				nwvp_log(NWLOG_DEVICE_OFFLINE, lpart->lpartition_handle, lpart->rpart_handle, 0);
				lpart->rpart_handle = 0xFFFFFFFF;
				lpart->mirror_status &= ~(MIRROR_PRESENT_BIT | MIRROR_REMIRRORING_BIT);
				if (nwvp_mirror_group_update(lpart->primary_link) != 0)
				{
					nwvp_mirror_group_delete(lpart->primary_link);
				}
				break;
			}
			error_count ++;
		}
	}
	NWUnlockHOTFIX();
	return(-1);
}

ULONG	nwvp_hotfix_update_bad_bits(
	nwvp_lpart	*original_part,
	ULONG		original_block_number,
	ULONG		original_bad_bits)
{
	ULONG		bad_bits = 0;
	ULONG		relative_block_number = 0;
	nwvp_lpart	*lpart;

	lpart = original_part;

	if (lpart->hotfix.bit_table[original_block_number >> 15][(original_block_number >> 5) & 0x3FF] & (1 << (original_block_number & 0x1F)))
	{
		nwvp_hotfix_block_lookup(original_part, original_block_number, &relative_block_number, &bad_bits);
		lpart->hotfix.bad_bit_table[relative_block_number >> 10][relative_block_number & 0x3FF] = original_bad_bits;
		return(nwvp_hotfix_tables_flush(lpart, relative_block_number));
	}
	return(-1);
}

ULONG	nwvp_stop_remirror(
	nwvp_lpart	*lpart)
{
	nwvp_remirror_control	*control;

	if ((lpart->mirror_status & MIRROR_GROUP_REMIRRORING_BIT) != 0)
	{
		if ((control = nwvp_remirror_control_list_head) != 0)
		{

			do
			{
				if (control->lpart_handle == lpart->lpartition_handle)
				{
#if (DEBUG_MIRROR_CONTROL)
					NWFSPrint("quiesing control  %08X\n", (int) control);
#endif
					control->abort_flag = 0xFFFFFFFF;
					nwvp_log(NWLOG_REMIRROR_STOP, lpart->lpartition_handle, 0, 0);
					if (nwvp_remirror_message_flag != 0)
						NWFSPrint("Partition %d Remirroring Stopped\n", (int) (lpart->lpartition_handle & 0x0000FFFF));
					return(0);
				}
				control = control->next_link;
			} while (control != nwvp_remirror_control_list_head);
		}
	}
	nwvp_log(NWLOG_REMIRROR_STOP_ERROR, lpart->lpartition_handle, 0, 0);
	return(-1);
}

ULONG	nwvp_start_remirror(
	nwvp_lpart	*lpart)
{
#if (LINUX_20 | LINUX_22 | LINUX_24)
	extern void NWFSStartRemirror(void);
#endif
	nwvp_lpart		*temp;
	nwvp_remirror_control	*control, *head;

	nwvp_alloc((void **) &control, sizeof(nwvp_remirror_control));
#if (DEBUG_MIRROR_CONTROL)
	NWFSPrint(" Allocated remirror control structure %08X\n", (int) control);
#endif

	if (((lpart->mirror_status & MIRROR_GROUP_ACTIVE_BIT) != 0) &&
	    ((lpart->mirror_status & MIRROR_GROUP_READ_ONLY_BIT) == 0))
	{
		if ((lpart->mirror_status & MIRROR_GROUP_REMIRRORING_BIT) == 0)
		{
			lpart->mirror_status |= MIRROR_GROUP_REMIRRORING_BIT;
			nwvp_mirror_group_open(lpart);

			temp = lpart;
			do
			{
				if ((temp->mirror_status & MIRROR_PRESENT_BIT) != 0)
				{
					if (temp->remirror_section == 0xFFFFFFFF)
						temp->remirror_section = (lpart->logical_block_count >> 10) + 1;
					temp->mirror_status |= MIRROR_REMIRRORING_BIT;
				}
				temp = temp->mirror_link;
			} while (temp != lpart);

			nwvp_memset(control, 0, sizeof(nwvp_remirror_control));
			control->lpart_handle = lpart->lpartition_handle;
			control->section = (lpart->logical_block_count >> 10);
			control->original_section = control->section;
			control->state = NWVP_VALIDATE_PHASE;
			if ((head = nwvp_remirror_control_list_head) == 0)
			{
				control->next_link = control;
				control->last_link = control;
				nwvp_remirror_control_list_head = control;
			}
			else
			{
				control->last_link = head->last_link;
				control->next_link = head;
				head->last_link->next_link = control;
				head->last_link = control;
			}
			nwvp_log(NWLOG_REMIRROR_START, lpart->lpartition_handle, 0, 0);
			if (nwvp_remirror_message_flag != 0)
			{
			  if (lpart->mirror_status & MIRROR_INSYNCH_BIT)
			     NWFSPrint("Partition %d Improperly Shutdown, Validating Mirrors.\n",
				(int) (lpart->lpartition_handle & 0x0000FFFF));
			  else
			     NWFSPrint("Partition %d Remirroring Start\n", 
				(int) (lpart->lpartition_handle & 0x0000FFFF));
			}

#if (LINUX_20 | LINUX_22 | LINUX_24)
			NWFSStartRemirror();
#else
			NWUnlockREMIRROR();
#endif
			return(0);
		}
	}
#if (DEBUG_MIRROR_CONTROL)
	NWFSPrint(" Allocated remirror return %08X\n", (int) control);
#endif
	nwvp_free(control);
	nwvp_log(NWLOG_REMIRROR_START_ERROR, lpart->lpartition_handle, 0, 0);
	return(-1);
}

void	nwvp_quiese_remirroring(void)
{
	ULONG	i;

	NWLockNWVP();
	for (i=0; i<logical_partition_table_count; i++)
	{
		if (logical_partition_table[i] != 0)
			nwvp_stop_remirror(logical_partition_table[i]);
	}
	NWUnlockNWVP();
}


ULONG	nwvp_mirror_create_entry(
	nwvp_lpart	   	*lpart,
	ULONG			mirror_sector_index,
	ULONG			partition_id,
	ULONG			group_id)
{
	ULONG			j;
	nwvp_lpart	   	*temp;

	nwvp_alloc((void *) &temp, sizeof(nwvp_lpart));
	nwvp_memset(temp, 0, sizeof(nwvp_lpart));
	temp->remirror_section = 0xFFFFFFFF;
	temp->lpartition_handle = lpart->lpartition_handle;
	temp->mirror_partition_id = partition_id;
	temp->mirror_group_id = group_id;
	temp->primary_link = lpart;
	temp->mirror_link = lpart->mirror_link;
	lpart->mirror_link = temp;
	temp->mirror_sector_index = mirror_sector_index;

	for (j=0; j<raw_partition_table_count; j++)
	{
	   if (raw_partition_table[j] == 0)
	   {
			raw_partition_table[j] = temp;
			temp->rpartition_handle = ((handle_instance ++) << 16) + j;
			break;
	   }
	}
	if ((j == raw_partition_table_count) && (j < MAX_PARTITIONS))
	{
		raw_partition_table[j] = temp;
		temp->rpartition_handle = ((handle_instance ++) << 16) + j;
		raw_partition_table_count ++;
		return(0);
	}
	return(-1);
}

void	nwvp_mirror_group_establish(
	nwvp_lpart	   	*lpart,
	ULONG			hard_activate_flag)
{
	ULONG			i;
	ULONG			mirror_count;
	ULONG			found_flag;
	ULONG			modify_check_flag = 0;
	ULONG			complete_flag = 1;
	ULONG			active_flag = 0;
	ULONG			NetWareFlag = 0;
	ULONG			open_flag = 0;
	ULONG			part_index;
	ULONG			base_instance;
    ULONG			remirror_flag = 0;
	nwvp_lpart		*temp;
	nwvp_lpart		*temp2;
	nwvp_mirror_sector	base_sector;

	part_index = lpart->lpartition_handle & 0x0000FFFF;
	if ((lpart->primary_link->mirror_status & MIRROR_GROUP_ESTABLISHED_BIT) != 0)
	{
		if ((lpart->mirror_status & MIRROR_NEW_BIT) != 0)
		{
			lpart->mirror_status &= ~MIRROR_NEW_BIT;
			lpart->mirror_status |= MIRROR_PRESENT_BIT;
			nwvp_memmove(lpart->mirror_sector, lpart->primary_link->mirror_sector, 512);
			nwvp_netware_mirror_update(lpart, 1);
		}
		else
		{
			lpart->mirror_status |= MIRROR_PRESENT_BIT;
			nwvp_memmove(lpart->mirror_sector, lpart->primary_link->mirror_sector, 512);
			nwvp_netware_mirror_update(lpart, 1);
		}
		temp = lpart->primary_link;
		do
		{
			if ((temp->mirror_status & MIRROR_PRESENT_BIT) != 0)
			{
				if ((temp->mirror_status & MIRROR_INSYNCH_BIT) != 0)
					active_flag = 1;
				else
					remirror_flag = 1;
			}
			temp = temp->mirror_link;
		} while (temp != lpart->primary_link);

		if (((lpart->primary_link->mirror_status & MIRROR_GROUP_ACTIVE_BIT) == 0) &&
			(active_flag != 0))
		{
			temp->mirror_status |= MIRROR_GROUP_ACTIVE_BIT | MIRROR_GROUP_ESTABLISHED_BIT;
			nwvp_segmentation_sectors_load(temp);
			nwvp_segmentation_add(temp);
		}

		if ((lpart->primary_link->mirror_status & MIRROR_GROUP_ACTIVE_BIT) != 0)
		{
			if ((remirror_flag != 0) && (nwvp_auto_remirror_flag != 0))
				nwvp_start_remirror(logical_partition_table[part_index]);
		}
		return;
	}

	if ((lpart->mirror_status & MIRROR_NEW_BIT) != 0)
	{
		lpart->mirror_status &= ~MIRROR_NEW_BIT;
		lpart->mirror_status |= MIRROR_PRESENT_BIT;
	}

//
//	find out if the group is complete;
//

	temp = lpart->primary_link;
	do
	{
		if ((nwvp_memcomp(&temp->mirror_sector->nwvp_mirror_stamp[0], "NWVP MIRROR 0001", 16) != 0) ||
			(temp->netware_mirror_sector->MirrorGroupID != temp->mirror_sector->mirror_id) ||
			(((temp->netware_mirror_sector->MirrorFlags & 0xFFFF0000) != NETWARE_IN_SYNCH_INSTANCE) &&
			 ((temp->netware_mirror_sector->MirrorFlags & 0xFFFF0000) != NETWARE_OUT_OF_SYNCH_INSTANCE)))
			 NetWareFlag = 1;

		for (i=0; i<8; i++)
		{
			if (temp->netware_mirror_sector->MirrorMemberID[i] == 0)
				break;
			found_flag = 0;
			temp2 = lpart->primary_link;
			do
			{
				if (temp->netware_mirror_sector->MirrorMemberID[i] == temp2->mirror_partition_id)
				{
					found_flag = 1;
					break;
				}
				temp2 = temp2->mirror_link;
			} while (temp2 != lpart->primary_link);
			if (found_flag == 0)
			{
				complete_flag = 0;
			}
		}
		temp = temp->mirror_link;
	} while (temp != logical_partition_table[part_index]);

//
//
//

	if ((complete_flag != 0) && (lpart->mirror_link == lpart))
	{
		nwvp_log(NWLOG_DEVICE_INSYNCH, lpart->lpartition_handle, lpart->rpart_handle, 0);
		lpart->mirror_status |= MIRROR_INSYNCH_BIT;
		lpart->primary_link = lpart;
		lpart->mirror_count = 1;
		lpart->remirror_section = 0;
		active_flag = 1;
        if ((lpart->mirror_status & MIRROR_READ_ONLY_BIT) != 0)
           lpart->mirror_status |= MIRROR_GROUP_READ_ONLY_BIT;
	}
	else
	{
		if ((hard_activate_flag != 0) || (complete_flag != 0))
		{
//
//	here is where we establish the mirror and synch state of all members
//
			if (NetWareFlag == 0)
			{
				temp = logical_partition_table[part_index];
				base_instance = 0;
				do
				{
					if (temp->mirror_sector->time_stamp > base_instance)
					{
						base_instance = temp->mirror_sector->time_stamp;
						nwvp_memmove(&base_sector, temp->mirror_sector, 512);
					}
					temp = temp->mirror_link;
				} while (temp != lpart->primary_link);
			}
			else
			{
				nwvp_memset(&base_sector, 0, 512);
				nwvp_memmove(&base_sector.nwvp_mirror_stamp[0], "NWVP MIRROR 0001", 16);
				temp = logical_partition_table[part_index];
				active_flag = 0;
				temp2 = 0;
				base_instance = 0;
				do
				{
					if (temp->netware_mirror_sector->MirrorGroupID > base_instance)
					{
						base_instance = temp->netware_mirror_sector->MirrorGroupID;
						temp2 = temp;
					}
					temp = temp->mirror_link;
				} while (temp != lpart->primary_link);

				base_sector.mirror_id = temp2->netware_mirror_sector->MirrorGroupID;
				for (i=0; i<8; i++)
				{
					if (temp2->netware_mirror_sector->MirrorMemberID[i] == 0)
						break;
					temp2->mirror_count = 0;

					base_sector.log[i].partition_id = temp2->netware_mirror_sector->MirrorMemberID[i];
					base_sector.log[i].time_stamp = 0;
					base_sector.log[i].remirror_section = 0;
					if ((temp2->netware_mirror_sector->MirrorStatus & (1 << i)) != 0)
						base_sector.log[i].remirror_section = 0xFFFFFFFF;
					base_sector.log[i].status = NWVP_MIRROR_STATUS_CLOSED;
					if ((temp2->netware_mirror_sector->MirrorFlags & NETWARE_INUSE_BIT) != 0)
						base_sector.log[i].status = NWVP_MIRROR_STATUS_OPENED;
				}
			}

//
//	set mirror index and look for absent partitions
//
			mirror_count = 0;
			for  (i=0; i<30; i++)
			{
				if ((base_sector.log[i].partition_id != 0) &&
					(base_sector.log[i].status != NWVP_MIRROR_STATUS_REMOVED))
				{
					mirror_count ++;
					temp = logical_partition_table[part_index];
					if ((base_sector.log[i].status == NWVP_MIRROR_STATUS_OPENED) ||
						(base_sector.log[i].status == NWVP_MIRROR_STATUS_CHECK))
					{
						base_sector.log[i].status = NWVP_MIRROR_STATUS_CHECK;
						open_flag = 1;
					}
					found_flag = 0;
					do
					{
						if (temp->mirror_partition_id == base_sector.log[i].partition_id)
						{
							temp->remirror_section = base_sector.log[i].remirror_section;
							temp->mirror_sector_index = i;
							found_flag = 1;
							break;
						}
						temp = temp->mirror_link;
					} while (temp != lpart->primary_link);
					if (found_flag == 0)
					{
//
//	create any absent partitions
//
						nwvp_mirror_create_entry(logical_partition_table[part_index], i, base_sector.log[i].partition_id, base_sector.mirror_id);
						if (base_sector.log[i].remirror_section != 0xFFFFFFFF)
							modify_check_flag = 1;
					}
				}
			}
//
//	bases upon the previous evaluation, set correctly set the mirror state of each partition
//

			temp = logical_partition_table[part_index];
			do
			{
				if ((temp->remirror_section = base_sector.log[temp->mirror_sector_index].remirror_section) == 0)
				{
					if ((temp->mirror_status & MIRROR_PRESENT_BIT) != 0)
					{
						nwvp_log(NWLOG_DEVICE_INSYNCH, temp->lpartition_handle, temp->rpart_handle, 0);
						temp->mirror_status |= MIRROR_INSYNCH_BIT;
						if (open_flag > 0)
						{
							temp->mirror_status |= MIRROR_GROUP_CHECK_BIT;
							if (open_flag != 1)
								temp->remirror_section = 0xFFFFFFFF;
							open_flag ++;
						}
						active_flag = 1;
					}
				}
				else
				{
					nwvp_log(NWLOG_DEVICE_OUT_OF_SYNCH, temp->lpartition_handle, temp->rpart_handle, 0);
				}
				temp->mirror_count = mirror_count;
				temp = temp->mirror_link;
			} while (temp != logical_partition_table[part_index]);

//
//	if a valid partition exist flush established tables
//
			if (active_flag != 0)
			{
				base_sector.time_stamp = nwvp_get_date_and_time();
				temp = logical_partition_table[part_index];
				do
				{
					if (temp->mirror_sector != 0)
					{
						nwvp_memmove(temp->mirror_sector, &base_sector, 512);
						temp->mirror_sector->table_index = temp->mirror_sector_index;
					}
					temp = temp->mirror_link;
				} while (temp != logical_partition_table[part_index]);

				do
				{
					if ((temp->mirror_status & MIRROR_PRESENT_BIT) != 0)
					{
                        if ((temp->mirror_status & MIRROR_READ_ONLY_BIT) != 0)
                           logical_partition_table[part_index]->mirror_status |= MIRROR_GROUP_READ_ONLY_BIT;
						if (temp->remirror_section != 0)
							remirror_flag = 1;
						nwvp_netware_mirror_update(temp, 1);
					}
					temp = temp->mirror_link;
				} while (temp != logical_partition_table[part_index]);

			}
		}
	}
//
//	evaluate logical partition for volume segments
//
	if (active_flag != 0)
	{
		if (modify_check_flag != 0)
			temp->mirror_status |= MIRROR_GROUP_MODIFY_CHECK_BIT;

		temp->mirror_status |= MIRROR_GROUP_ACTIVE_BIT | MIRROR_GROUP_ESTABLISHED_BIT;
		nwvp_segmentation_sectors_load(temp);
		nwvp_segmentation_add(temp);

		if ((remirror_flag != 0) && (nwvp_auto_remirror_flag != 0))
			nwvp_start_remirror(logical_partition_table[part_index]);
	}
}

ULONG	nwvp_mirror_group_modify_check(
	nwvp_lpart	*lpart)
{
	ULONG	found_flag = 0;
	nwvp_lpart	*temp;

	if ((lpart->mirror_status & MIRROR_GROUP_ACTIVE_BIT) != 0)
	{
		if ((lpart->mirror_status & MIRROR_GROUP_MODIFY_CHECK_BIT) != 0)
		{
			temp = lpart;
			do
			{
				if ((temp->mirror_status & MIRROR_PRESENT_BIT) == 0)
				{
					lpart->mirror_sector->log[temp->mirror_sector_index].remirror_section = 0xFFFFFFFF;
					nwvp_log(NWLOG_DEVICE_OUT_OF_SYNCH, temp->lpartition_handle, temp->rpart_handle, temp->remirror_section);
					temp->remirror_section = 0xFFFFFFFF;
					temp->mirror_status &= ~MIRROR_INSYNCH_BIT;
					found_flag = 1;
				}
				temp = temp->mirror_link;
			} while (temp != lpart);

			if (found_flag != 0)
			{
				lpart->mirror_sector->time_stamp = nwvp_get_date_and_time();
				temp = lpart;
				do
				{
					if ((temp->mirror_sector != 0) && (lpart != temp))
					{
						nwvp_memmove(temp->mirror_sector, lpart->mirror_sector, 512);
						temp->mirror_sector->table_index = temp->mirror_sector_index;
					}
					temp = temp->mirror_link;
				} while (temp != lpart);

				do
				{
					if ((temp->mirror_status & MIRROR_PRESENT_BIT) != 0)
						nwvp_netware_mirror_update(temp, 1);
					temp = temp->mirror_link;
				} while (temp != lpart);
			}
		}
		lpart->mirror_status &= ~MIRROR_GROUP_MODIFY_CHECK_BIT;
		return(0);
	}
	lpart->mirror_status &= ~MIRROR_GROUP_MODIFY_CHECK_BIT;
	return(-1);
}

ULONG	nwvp_mirror_group_remirror_check(
	nwvp_lpart	*lpart,
	ULONG	section,
	ULONG	*reset_flag)
{
	ULONG	alive_count = 0;
	nwvp_lpart	*temp;

	temp = lpart;
	do
	{
		if ((temp->mirror_status & MIRROR_PRESENT_BIT) != 0)
		{
			if ((temp->mirror_status & MIRROR_REMIRRORING_BIT) == 0)
			{
				if (temp->remirror_section == 0xFFFFFFFF)
					temp->remirror_section = (lpart->logical_block_count >> 10) + 1;
				temp->mirror_status |= MIRROR_REMIRRORING_BIT;
				nwvp_log(NWLOG_REMIRROR_RESET1, lpart->lpartition_handle, temp->rpart_handle, section);
				*reset_flag = 0xFFFFFFFF;
				return(0);
			}
			alive_count ++;
			if (temp->remirror_section >= section + 1)
			{
				if (temp->remirror_section != (section + 1))
				{
					nwvp_log(NWLOG_REMIRROR_RESET2, lpart->lpartition_handle, temp->rpart_handle, section);
					*reset_flag = 0xFFFFFFFF;
					return(0);
				}
			}
		}
		temp = temp->mirror_link;
	} while (temp != lpart);
	return((alive_count > 1) ? 0 : 0xFFFFFFFF);
}

ULONG	nwvp_mirror_group_remirror_update(
	ULONG	lpart_handle,
	ULONG	section,
	ULONG	*reset_flag)
{
	nwvp_lpart	*lpart;
	nwvp_lpart	*temp;

	*reset_flag = 0;
	NWLockNWVP();
	if ((lpart_handle & 0x0000FFFF) < logical_partition_table_count)
	{
		if (((lpart = logical_partition_table[lpart_handle & 0x0000FFFF]) != 0) &&
			(lpart->lpartition_handle == lpart_handle))
		{
			temp = lpart;
			do
			{
				if ((temp->mirror_status & MIRROR_PRESENT_BIT) != 0)
				{
					if (temp->remirror_section >= section + 1)
					{
						if (temp->remirror_section != (section + 1))
						{
								*reset_flag = 0xFFFFFFFF;
								nwvp_log(NWLOG_REMIRROR_RESET3, lpart->lpartition_handle, temp->rpart_handle, section);
								NWUnlockNWVP();
								if (nwvp_remirror_message_flag != 0)
								{
					        if (lpart->mirror_status & MIRROR_INSYNCH_BIT)
					          NWFSPrint("Partition %d Validate Reset\n", 
										(int) (lpart->lpartition_handle & 0x0000FFFF));
					        else
									  NWFSPrint("Partition %d Remirroring Reset\n", 
														(int)(lpart->lpartition_handle & 0x0000FFFF));
								}
								return(0);
						}
						temp->remirror_section = section;
						lpart->mirror_sector->log[temp->mirror_sector_index].remirror_section = section;
					}
				}
				temp = temp->mirror_link;
			} while (temp != lpart);
			if (section == 0)
			{
				temp = lpart;
				do
				{
					if (((temp->mirror_status & MIRROR_INSYNCH_BIT) == 0) && ((temp->mirror_status & MIRROR_PRESENT_BIT) != 0))
					{
						nwvp_log(NWLOG_DEVICE_INSYNCH, temp->lpartition_handle, temp->rpart_handle, 0);
						temp->mirror_status |= MIRROR_INSYNCH_BIT;
						temp->remirror_section = section;
						lpart->mirror_sector->log[temp->mirror_sector_index].remirror_section = section;
						lpart->mirror_sector->log[temp->mirror_sector_index].status = NWVP_MIRROR_STATUS_CLOSED;
						if ((lpart->mirror_status & MIRROR_GROUP_OPEN_BIT) != 0)
							lpart->mirror_sector->log[temp->mirror_sector_index].status = NWVP_MIRROR_STATUS_OPENED;
					}
					temp = temp->mirror_link;
				} while (temp != lpart);

				lpart->mirror_sector->time_stamp = nwvp_get_date_and_time();
				temp = lpart;
				do
				{
					if (temp->mirror_sector != 0)
					{
						nwvp_memmove(temp->mirror_sector, lpart->mirror_sector, 512);
						temp->mirror_sector->table_index = temp->mirror_sector_index;
					}
					temp = temp->mirror_link;
				} while (temp != lpart);

				do
				{
					if ((temp->mirror_status & MIRROR_PRESENT_BIT) != 0)
						nwvp_netware_mirror_update(temp, 1);
					temp = temp->mirror_link;
				} while (temp != lpart);
				if (nwvp_remirror_message_flag != 0)
				{
					if (lpart->mirror_status & MIRROR_INSYNCH_BIT)
					  NWFSPrint("Partition %d Validate (100%% Complete)\n", 
										(int) (lpart->lpartition_handle & 0x0000FFFF));
					else
					  NWFSPrint("Partition %d Remirroring (100%% Complete)\n", 
										(int) (lpart->lpartition_handle & 0x0000FFFF));
				}
			}
			else
			{

				if (nwvp_remirror_message_flag != 0)
				{
					if ((((lpart->logical_block_count >> 10) / 4) * 3) == section)
					{
					  if (lpart->mirror_status & MIRROR_INSYNCH_BIT)
					    NWFSPrint("Partition %d Validate (25%% Complete)\n", 
										(int) (lpart->lpartition_handle & 0x0000FFFF));
					  else
						  NWFSPrint("Partition %d Remirroring (25%% Complete)\n", 
											(int) (lpart->lpartition_handle & 0x0000FFFF));
					}
					else
					if ((((lpart->logical_block_count >> 10) / 4) * 2) == section)
					{
					  if (lpart->mirror_status & MIRROR_INSYNCH_BIT)
					    NWFSPrint("Partition %d Validate (50%% Complete)\n", 
										(int) (lpart->lpartition_handle & 0x0000FFFF));
					  else
						  NWFSPrint("Partition %d Remirroring (50%% Complete)\n", 
										  (int) (lpart->lpartition_handle & 0x0000FFFF));
					}
					else
					if ((((lpart->logical_block_count >> 10) / 4) * 1) == section)
					{
					  if (lpart->mirror_status & MIRROR_INSYNCH_BIT)
					    NWFSPrint("Partition %d Validate (75%% Complete)\n", 
										(int) (lpart->lpartition_handle & 0x0000FFFF));
					  else
						  NWFSPrint("Partition %d Remirroring (75%% Complete)\n", 
											(int) (lpart->lpartition_handle & 0x0000FFFF));
					}
				}
			}
		}
	}
	NWUnlockNWVP();
	return(0);
}

ULONG	nwvp_mirror_group_add(
	nwvp_lpart	   	*lpart)
{
	ULONG	j;
	nwvp_lpart	   	*base_part;

	for (j=0; j<logical_partition_table_count; j++)
	{
		if ((base_part = logical_partition_table[j]) != 0)
		{
			if (base_part->mirror_group_id == lpart->mirror_group_id)
			{
				lpart->lpartition_handle = base_part->lpartition_handle;
				lpart->primary_link = base_part;
				lpart->mirror_link = base_part->mirror_link;
				lpart->mirror_count = base_part->mirror_count;
				base_part->mirror_link = lpart;
				nwvp_mirror_group_establish(base_part, 0);
				return(0);
			}
		}
	}

	for (j=0; j<logical_partition_table_count; j++)
	{
		if (logical_partition_table[j] == 0)
		{
			lpart->lpartition_handle = (handle_instance ++ << 16) + j;
			logical_partition_table[j] = lpart;
			break;
		}
	}
	if (j >= MAX_PARTITIONS)
		return(0);
	if (j == logical_partition_table_count)
	{
		lpart->lpartition_handle = (handle_instance ++ << 16)  + j;
		logical_partition_table[j] = lpart;
		logical_partition_table_count ++;
	}
	lpart->primary_link = lpart;
	lpart->mirror_link = lpart;
	lpart->mirror_count = 1;
	return(0);
}

void	nwvp_mirror_group_delete(
	nwvp_lpart		*member)
{
	nwvp_lpart		*temp;

	while ((temp = member->mirror_link) != member)
	{
		member->mirror_link = temp->mirror_link;
		nwvp_hotfix_tables_free(temp);
		if (temp->mirror_sector != 0)
		{
			nwvp_free(temp->mirror_sector);
			temp->mirror_sector = 0;
		}
		if (temp->netware_mirror_sector != 0)
		{
			nwvp_free(temp->netware_mirror_sector);
			temp->netware_mirror_sector = 0;
		}
		raw_partition_table[temp->rpartition_handle & 0x0000FFFF] = 0;
		nwvp_free(temp);
	}
	nwvp_hotfix_tables_free(temp);
	if (temp->mirror_sector != 0)
	{
		nwvp_free(temp->mirror_sector);
		temp->mirror_sector = 0;
	}
	if (temp->netware_mirror_sector != 0)
	{
		nwvp_free(temp->netware_mirror_sector);
		temp->netware_mirror_sector = 0;
	}
	raw_partition_table[temp->rpartition_handle & 0x0000FFFF] = 0;
	logical_partition_table[temp->lpartition_handle & 0x0000FFFF] = 0;
	nwvp_free(temp);
}

ULONG	nwvp_mirror_group_update(
	nwvp_lpart		*member)
{
	ULONG			present_flag = 1;
	ULONG			found_flag = 0;
	nwvp_lpart		*temp;
	nwvp_lpart		*temp2;

	temp = member->primary_link;
	do
	{
		if ((temp->mirror_status & MIRROR_PRESENT_BIT) != 0)
		{
			if ((temp->mirror_status & MIRROR_INSYNCH_BIT) != 0)
			{
				if (temp->remirror_section == 0)
					found_flag = 0xFFFFFFFF;
				else
				{
					if (found_flag == 0)
					{
						temp2 = temp;
						do
						{
							if (((temp2->mirror_status & MIRROR_INSYNCH_BIT) != 0) &&
								 (temp2->remirror_section == 0))
							{
								found_flag = 0xFFFFFFFF;
								break;
							}
							temp2 = temp2->mirror_link;
						} while (temp2 != member->primary_link);
						if (found_flag == 0)
						{
							temp->remirror_section = 0;
							found_flag = 0xFFFFFFFF;
						}
					}
				}
			}
			present_flag = 0;
		}
		else
		{
			member->primary_link->mirror_status |= MIRROR_GROUP_MODIFY_CHECK_BIT;
		}
		temp = temp->mirror_link;
	} while (temp != member->primary_link);

	if (found_flag == 0)
	{
		if ((member->primary_link->mirror_status & MIRROR_GROUP_ACTIVE_BIT) != 0)
		{
			member->primary_link->mirror_status &= ~MIRROR_GROUP_ACTIVE_BIT;
			nwvp_segmentation_remove(temp);
		}
	}
	return(present_flag);
}


ULONG	nwvp_mirror_member_add(
	nwvp_lpart		*mpart,
	nwvp_lpart		*new_part)
{
	ULONG		i;
	ULONG		remirror_flag = 0;
	nwvp_lpart	*temp;
	nwvp_mirror_sector	base_sector;

	nwvp_memmove(&base_sector, mpart->mirror_sector, 512);

	for (i=0; i<30; i++)
	{
		if (base_sector.log[i].partition_id == 0)
			break;
	}
	if (i != 30)
	{
		base_sector.log[i].partition_id = new_part->mirror_partition_id;
		base_sector.log[i].status = NWVP_MIRROR_STATUS_CLOSED;
		if ((mpart->mirror_status & MIRROR_GROUP_CHECK_BIT) != 0)
			base_sector.log[i].status = NWVP_MIRROR_STATUS_CHECK;
		if ((mpart->mirror_status & MIRROR_GROUP_OPEN_BIT) != 0)
			base_sector.log[i].status = NWVP_MIRROR_STATUS_OPENED;
		new_part->mirror_sector_index = i;
		if (i >= mpart->mirror_count)
			mpart->mirror_count = i+1;
		new_part->mirror_link = mpart->mirror_link;
		new_part->mirror_group_id = mpart->mirror_group_id;
		mpart->mirror_link = new_part;
		new_part->primary_link = mpart;
		if (mpart->segment_sector->NumberOfTableEntries == 0)
		{
			nwvp_log(NWLOG_DEVICE_INSYNCH, new_part->lpartition_handle, new_part->rpart_handle, 0);
			new_part->mirror_status |= MIRROR_INSYNCH_BIT;
			base_sector.log[i].remirror_section = 0;
		}
		else
		{
			nwvp_log(NWLOG_DEVICE_OUT_OF_SYNCH, new_part->lpartition_handle, new_part->rpart_handle, 0);
			new_part->mirror_status &= ~MIRROR_INSYNCH_BIT;
			base_sector.log[i].remirror_section = 0xFFFFFFFF;
			remirror_flag = 1;
		}
		base_sector.time_stamp = nwvp_get_date_and_time();
		temp = mpart;
		do
		{
			temp->mirror_count = mpart->mirror_count;
			if (temp->mirror_sector != 0)
			{
				nwvp_memmove(temp->mirror_sector, &base_sector, 512);
				temp->mirror_sector->table_index = temp->mirror_sector_index;
			}
			temp = temp->mirror_link;
		} while (temp != mpart);

		do
		{
			if ((temp->mirror_status & MIRROR_PRESENT_BIT) != 0)
				nwvp_netware_mirror_update(temp, 1);
			temp = temp->mirror_link;
		} while (temp != mpart);

		if ((remirror_flag != 0) && (nwvp_auto_remirror_flag != 0))
		{
			nwvp_start_remirror(mpart);
		}

		return(0);
	}
	return(-1);
}

void	nwvp_mirror_partition_destroy(
	nwvp_lpart		*del_part,
	nwvp_lpart		**new_primary)
{
	nwvp_lpart	*temp;
	nwvp_lpart	*primary_part;

	if ((primary_part = del_part->primary_link) == del_part)
	{
		primary_part = del_part->mirror_link;
		temp = del_part->mirror_link;
		while (1)
		{
			temp->primary_link = primary_part;
			if (temp->mirror_link == del_part)
			{
				temp->mirror_link = primary_part;
				break;
			}
			temp = temp->mirror_link;
		}
		primary_part->mirror_count = del_part->mirror_count;
		primary_part->mirror_status |= del_part->mirror_status & 0xFFFF0000;
		logical_partition_table[primary_part->lpartition_handle & 0x0000FFFF] = primary_part;
	nwvp_memmove(primary_part->segment_sector, del_part->segment_sector, 512);
        vpartition_update_lpartition(del_part, primary_part);
	}
	else
	{
		temp = primary_part;
		while (temp->mirror_link != del_part)
			temp = temp->mirror_link;
		temp->mirror_link = del_part->mirror_link;
	}
	nwvp_hotfix_tables_free(del_part);
	if (del_part->mirror_sector != 0)
	{
		nwvp_free(del_part->mirror_sector);
		del_part->mirror_sector = 0;
	}
	if (del_part->netware_mirror_sector != 0)
	{
		nwvp_free(del_part->netware_mirror_sector);
		del_part->netware_mirror_sector = 0;
	}
	nwvp_free(del_part);
	*new_primary = primary_part;
}

ULONG	nwvp_mirror_member_delete(
	nwvp_lpart		*del_part)
{
	ULONG		i;
	ULONG		base_index;
	nwvp_lpart	*temp, *primary_part;
	nwvp_mirror_sector	base_sector;

	primary_part = del_part->primary_link;
	nwvp_memmove(&base_sector, primary_part->mirror_sector, 512);

	for (i=0; i<primary_part->mirror_count; i++)
	{
		if (base_sector.log[i].partition_id == del_part->mirror_partition_id)
		{
			base_index = i;
			for (; i<30; i++)
			{
				nwvp_memmove(&base_sector.log[i], &base_sector.log[i+1], 16);
				if (base_sector.log[i+1].partition_id == 0)
					break;
			}
			primary_part->mirror_count --;

			if ((del_part->mirror_status & MIRROR_PRESENT_BIT) != 0)
				nwvp_hotfix_sectors_zap(del_part);
            else
	    {
				for (i=30; i>0; i++)
				{
					if (base_sector.log[i-1].partition_id == 0)
					{
						del_part->mirror_sector_index = i-1;
						base_sector.log[i-1].partition_id = del_part->mirror_partition_id;
						base_sector.log[i-1].status = NWVP_MIRROR_STATUS_REMOVED;
						break;
					}
				}
            }

			del_part->mirror_status &= ~(MIRROR_PRESENT_BIT | MIRROR_REMIRRORING_BIT);
			del_part->mirror_status |= MIRROR_DELETED_BIT;
			del_part->rpart_handle = 0xFFFFFFFD;
			raw_partition_table[del_part->rpartition_handle & 0x0000FFFF] = 0;
			if ((primary_part->mirror_status & MIRROR_GROUP_OPEN_BIT) == 0)
			{
				nwvp_mirror_partition_destroy(del_part, &primary_part);
			}
			base_sector.time_stamp = nwvp_get_date_and_time();
			temp = primary_part;
			do
			{
				temp->mirror_count = primary_part->mirror_count;
				if (temp->mirror_sector != 0)
				{
					nwvp_memmove(temp->mirror_sector, &base_sector, 512);
					temp->mirror_sector->table_index = temp->mirror_sector_index;
				}
				if (temp->mirror_sector_index > base_index)
					temp->mirror_sector_index --;
				temp = temp->mirror_link;
			} while (temp != primary_part);

			do
			{
				if ((temp->mirror_status & MIRROR_PRESENT_BIT) != 0)
					nwvp_netware_mirror_update(temp, 1);
				temp = temp->mirror_link;
			} while (temp != primary_part);

			if (nwvp_mirror_group_update(temp) != 0)
			{
				nwvp_mirror_group_delete(temp);
			}
			return(0);
		}
	}
	return(-1);
}

ULONG	nwvp_mirror_group_open(
	nwvp_lpart	   	*lpart)
{
	ULONG			i;
	ULONG			time_stamp;
	nwvp_lpart	   	*temp;

	if ((lpart->mirror_status & MIRROR_GROUP_ESTABLISHED_BIT) == 0)
		nwvp_mirror_group_establish(lpart, 1);

	if ((lpart->mirror_status & MIRROR_GROUP_ACTIVE_BIT) != 0)
	{
		if (lpart->open_count == 0)
		{
			time_stamp = nwvp_get_date_and_time();
			lpart->mirror_sector->time_stamp = time_stamp;
			for (i=0; i<lpart->mirror_count; i++)
			{
				if (lpart->mirror_sector->log[i].partition_id != 0)
				{
					lpart->mirror_sector->log[i].time_stamp = time_stamp;
					lpart->mirror_sector->log[i].status = NWVP_MIRROR_STATUS_OPENED;
				}
			}
			temp = lpart;
			lpart->mirror_status |= MIRROR_GROUP_OPEN_BIT;
			do
			{
				if ((temp != lpart) && (temp->mirror_sector != 0))
				{
					nwvp_memmove(temp->mirror_sector, lpart->mirror_sector, 512);
					temp->mirror_sector->table_index = temp->mirror_sector_index;
				}
				temp = temp->mirror_link;
			} while (temp != lpart);
			do
			{
				if ((temp->mirror_status & MIRROR_PRESENT_BIT) != 0)
					nwvp_netware_mirror_update(temp, 1);
				temp = temp->mirror_link;
			} while (temp != lpart);
		}
		lpart->open_count ++;
		return(0);
	}
	return(-1);
}

ULONG	nwvp_mirror_group_close(
	nwvp_lpart	   	*lpart)
{
	ULONG			i;
	ULONG			time_stamp;
	ULONG			done_flag;
	nwvp_lpart	   	*temp;

#if (DEBUG_MIRROR_CONTROL)
NWFSPrint(" MIRROR GROUP CLOSE \n");
#endif
	if (lpart->open_count > 0)
	{
		lpart->open_count --;
		if (lpart->open_count == 0)
		{
			time_stamp = nwvp_get_date_and_time();
			lpart->mirror_sector->time_stamp = time_stamp;
#if (DEBUG_MIRROR_CONTROL)
NWFSPrint(" UPDATING MIRRORS \n");
#endif
			for (i=0; i < lpart->mirror_count; i++)
			{
				if (lpart->mirror_sector->log[i].partition_id != 0)
				{
					lpart->mirror_sector->log[i].time_stamp = time_stamp;
					lpart->mirror_sector->log[i].status = NWVP_MIRROR_STATUS_CLOSED;
					if ((lpart->mirror_status & MIRROR_GROUP_CHECK_BIT) != 0)
                                        {
#if (DEBUG_MIRROR_CONTROL)
NWFSPrint(" RESETTING CHECK \n");
#endif
						lpart->mirror_sector->log[i].status = NWVP_MIRROR_STATUS_CHECK;
					}
				}
			}

			lpart->mirror_status &= ~MIRROR_GROUP_OPEN_BIT;
			done_flag = 0xFFFFFFFF;
			while (done_flag != 0)
			{
				done_flag = 0;
				temp = lpart;
				do
				{
			    		if ((temp->mirror_status & MIRROR_DELETED_BIT) != 0)
					{
						nwvp_mirror_partition_destroy(temp, &lpart);
						done_flag = 0xFFFFFFFF;
						break;
					}
					temp = temp->mirror_link;
				} while (temp != lpart);
			}

			temp = lpart;
			do
			{
				if ((temp != lpart) && (temp->mirror_sector != 0))
				{
					nwvp_memmove(temp->mirror_sector, lpart->mirror_sector, 512);
					temp->mirror_sector->table_index = temp->mirror_sector_index;
				}
				temp = temp->mirror_link;
			} while (temp != lpart);

			do
			{
				if ((temp->mirror_status & MIRROR_PRESENT_BIT) != 0)
					nwvp_netware_mirror_update(temp, 1);
				temp = temp->mirror_link;
			} while (temp != lpart);

		}
		return(0);
	}
	return(-1);
}


ULONG	nwvp_rpartition_sector_read(
	ULONG		rpart_handle,
	ULONG		sector_number,
	ULONG		sector_count,
	BYTE		*data_buffer)
{
	ULONG		ccode;
	nwvp_lpart	*lpartition;

	NWLockNWVP();
	if ((rpart_handle & 0x0000FFFF) < raw_partition_table_count)
	{
		if (((lpartition = raw_partition_table[rpart_handle & 0x0000FFFF]) != 0) &&
			(lpartition->rpartition_handle == rpart_handle))
		{
			ccode = nwvp_part_read(lpartition->rpart_handle, sector_number, sector_count, data_buffer);
			NWUnlockNWVP();
			return(ccode);
		}
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	nwvp_rpartition_sector_write(
	ULONG		rpart_handle,
	ULONG		sector_number,
	ULONG		sector_count,
	BYTE		*data_buffer)
{
	ULONG		ccode;
	nwvp_lpart	*lpartition;

	NWLockNWVP();
	if ((rpart_handle & 0x0000FFFF) < raw_partition_table_count)
	{
		if (((lpartition = raw_partition_table[rpart_handle & 0x0000FFFF]) != 0) &&
			(lpartition->rpartition_handle == rpart_handle))
		{
            if ((lpartition->mirror_status & MIRROR_READ_ONLY_BIT) == 0)
            {
			    ccode = nwvp_part_write(lpartition->rpart_handle, sector_number, sector_count, data_buffer);
			    NWUnlockNWVP();
			    return(ccode);
	    }
		}
	}
	NWUnlockNWVP();
	return(-1);
}



ULONG	nwvp_lpartition_activate(
	ULONG		lpart_handle)
{
	nwvp_lpart	*lpartition;

	NWLockNWVP();
	if ((lpart_handle & 0x0000FFFF) < logical_partition_table_count)
	{
		if (((lpartition = logical_partition_table[lpart_handle & 0x0000FFFF]) != 0) &&
			 (lpartition->lpartition_handle == lpart_handle))
		{
			nwvp_mirror_group_establish(lpartition, 1);
			NWUnlockNWVP();
			return(0);
		}
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	inwvp_lpartition_specific_read(
	nwvp_lpart	*lpart,
	ULONG		block_number,
	ULONG		block_count,
	BYTE		*data_buffer)
{
	ULONG	i;
	ULONG	bad_bits;
	ULONG	error_flag = 0;
	ULONG	relative_block_number;
	ULONG	hotfixed_flag = 0;

	for (i=0; i<block_count; i++)
	{
		if (lpart->hotfix.bit_table[(block_number + i) >> 15][((block_number + i) >> 5) & 0x3FF] & (1 << ((block_number + i) & 0x1F)))
		{
			hotfixed_flag = 1;
			break;
		}
	}

	relative_block_number = block_number + lpart->hotfix.block_count;
	if (hotfixed_flag == 0)
	{
		if (nwvp_part_read(
				lpart->rpart_handle,
				relative_block_number * lpart->sectors_per_block,
				block_count * lpart->sectors_per_block,
				data_buffer) == 0)
			return(0);
	}

	for (i=0; i<block_count; i++)
	{
		bad_bits = 0;
		if (lpart->hotfix.bit_table[(block_number + i) >> 15][((block_number + i) >> 5) & 0x3FF] & (1 << ((block_number + i) & 0x1F)))
			nwvp_hotfix_block_lookup(lpart, block_number + i, &relative_block_number, &bad_bits);
		else
			relative_block_number = block_number + i + lpart->hotfix.block_count;
		error_flag |= bad_bits;
		if (nwvp_part_read(
				lpart->rpart_handle,
				relative_block_number * lpart->sectors_per_block,
				lpart->sectors_per_block,
				&data_buffer[4096 * i]) != 0)
		{
			bad_bits = 0;
			if (nwvp_hotfix_block(lpart, block_number + i, &data_buffer[4096 * i], &bad_bits, NWVP_HOTFIX_READ) == 0)
				error_flag |= bad_bits;
			else
				error_flag |= 0xFFFF0000;
		}
	}
	return(error_flag);
}


ULONG	inwvp_lpartition_block_read(
	nwvp_lpart	*original_part,
	ULONG		block_number,
	ULONG		block_count,
	BYTE		*data_buffer)
{
	ULONG	i, count;
	nwvp_lpart	*lpart;


	count = (original_part->read_round_robin ++) & 0x3;
	for (i=0; i< count; i++)
		original_part = original_part->mirror_link;

	lpart = original_part;
	do
	{
		if ((lpart->mirror_status & MIRROR_PRESENT_BIT) != 0)
		{
			if (((lpart->mirror_status & MIRROR_INSYNCH_BIT) != 0) ||
				(lpart->remirror_section < (block_number >> 10)))
			{
				if (inwvp_lpartition_specific_read(lpart, block_number, block_count, data_buffer) == 0)
					return(0);
			}
		}
		lpart = lpart->mirror_link;
	} while (lpart != original_part);
	return(-1);
}


ULONG	nwvp_lpartition_block_read(
	ULONG		lpart_handle,
	ULONG		block_number,
	ULONG		block_count,
	BYTE		*data_buffer)
{
	ULONG		ccode;
	nwvp_lpart	*lpartition;

	NWLockNWVP();
	if ((lpart_handle & 0x0000FFFF) < logical_partition_table_count)
	{
		if (((lpartition = logical_partition_table[lpart_handle & 0x0000FFFF]) != 0) &&
			 (lpartition->lpartition_handle == lpart_handle))
		{
			ccode = inwvp_lpartition_block_read(lpartition, block_number, block_count, data_buffer);
			NWUnlockNWVP();
			return(ccode);

		}
	}
	NWUnlockNWVP();
	return(-1);
}


ULONG	inwvp_lpartition_bad_bit_update(
	nwvp_lpart	*original_part,
	ULONG		block_number,
	ULONG		new_bad_bits)
{
	ULONG	success_count = 0;
	nwvp_lpart	*lpart = original_part;

	if ((lpart->mirror_status & MIRROR_GROUP_READ_ONLY_BIT) != 0)
	       return(-1);

	do
	{
		if ((lpart->mirror_status & MIRROR_PRESENT_BIT) != 0)
		{
			if (nwvp_hotfix_update_bad_bits(lpart, block_number, new_bad_bits) == 0)
				success_count ++;
		}
		lpart = lpart->mirror_link;
	} while (lpart != original_part);

	return((success_count == 0) ? -1 : 0);
}


ULONG	inwvp_lpartition_specific_write(
	nwvp_lpart	*lpart,
	ULONG		block_number,
	ULONG		block_count,
	BYTE		*data_buffer)
{
	ULONG	i;
	ULONG	error_flag = 0;
	ULONG	bad_bits;
	ULONG	relative_block_number;
	ULONG	hotfixed_flag;

	if ((lpart->primary_link->mirror_status & MIRROR_GROUP_MODIFY_CHECK_BIT) != 0)
		if (nwvp_mirror_group_modify_check(lpart->primary_link) != 0)
			return(-1);

	hotfixed_flag = 0;
	for (i=0; i<block_count; i++)
	{
		if (lpart->hotfix.bit_table[(block_number + i) >> 15][((block_number + i) >> 5) & 0x3FF] & (1 << ((block_number + i) & 0x1F)))
		{
			hotfixed_flag = 1;
			break;
		}
	}

	relative_block_number = block_number + lpart->hotfix.block_count;
	if (hotfixed_flag == 0)
	{
		if (nwvp_part_write(
				lpart->rpart_handle,
				relative_block_number * lpart->sectors_per_block,
				block_count * lpart->sectors_per_block,
				data_buffer) == 0)
			return(0);
		else
			hotfixed_flag = 1;
	}

	if (hotfixed_flag != 0)
	{
		error_flag = 0;
		for (i=0; i<block_count; i++)
		{
			bad_bits = 0;
			if (lpart->hotfix.bit_table[(block_number + i) >> 15][((block_number + i) >> 5) & 0x3FF] & (1 << ((block_number + i) & 0x1F)))
				nwvp_hotfix_block_lookup(lpart, block_number + i, &relative_block_number, &bad_bits);
			else
				relative_block_number = block_number + i + lpart->hotfix.block_count;

			if (nwvp_part_write(
				lpart->rpart_handle,
				relative_block_number * lpart->sectors_per_block,
				lpart->sectors_per_block,
				&data_buffer[4096 * i]) != 0)
			{
				if (nwvp_hotfix_block(lpart, block_number + i, &data_buffer[4096 * i], &bad_bits, NWVP_HOTFIX_WRITE) != 0)
					error_flag ++;
			}
			else
			{
				if (bad_bits != 0)
				{
					if (nwvp_hotfix_update_bad_bits(lpart, block_number + i, 0) != 0)
							error_flag ++;
				}
			}
		}
	}
	return(error_flag);
}

ULONG	inwvp_lpartition_block_write(
	nwvp_lpart	*original_part,
	ULONG		block_number,
	ULONG		block_count,
	BYTE		*data_buffer)
{
	ULONG	success_count = 0;
	nwvp_lpart	*lpart = original_part;

	if ((lpart->mirror_status & MIRROR_GROUP_READ_ONLY_BIT) != 0)
       return(-1);

	do
	{
		if ((lpart->mirror_status & MIRROR_PRESENT_BIT) != 0)
		{
			if (inwvp_lpartition_specific_write(lpart, block_number, block_count, data_buffer) == 0)
				success_count ++;
		}
		lpart = lpart->mirror_link;
	} while (lpart != original_part);

	return((success_count == 0) ? -1 : 0);
}

ULONG	nwvp_lpartition_block_write(
	ULONG		lpart_handle,
	ULONG		block_number,
	ULONG		block_count,
	BYTE		*data_buffer)
{
	ULONG		ccode;
	nwvp_lpart	*lpartition;

	NWLockNWVP();
	if ((lpart_handle & 0x0000FFFF) < logical_partition_table_count)
	{
		if (((lpartition = logical_partition_table[lpart_handle & 0x0000FFFF]) != 0) &&
			 (lpartition->lpartition_handle == lpart_handle))
		{
			ccode = inwvp_lpartition_block_write(lpartition, block_number, block_count, data_buffer);
			NWUnlockNWVP();
			return(ccode);
		}
	}
	NWUnlockNWVP();
	return(-1);
}


ULONG	inwvp_lpartition_block_hotfix(
	nwvp_lpart	*original_part,
	ULONG		block_number,
	BYTE		*data_buffer)
{
	ULONG	success_count = 0;
    ULONG   bad_bits;
	nwvp_lpart	*lpart = original_part;

	do
	{
		if ((lpart->mirror_status & MIRROR_PRESENT_BIT) != 0)
		{
		if (nwvp_hotfix_block(lpart, block_number, data_buffer, &bad_bits, NWVP_HOTFIX_FORCE) == 0)
				success_count ++;
		}
		lpart = lpart->mirror_link;
	} while (lpart != original_part);

	return((success_count == 0) ? -1 : 0);
}


ULONG	nwvp_vpartition_sub_block_read(
	ULONG		vpart_handle,
	ULONG		block_number,
	ULONG		block_offset,
	ULONG		data_size,
	BYTE		*data_buffer,
	ULONG           as)
{
	ULONG	i;
	ULONG	relative_block;
	BYTE 	*temp_buffer;
	nwvp_vpartition	*vpartition;

#if !MANOS
	if ((block_offset + data_size) > 4096)
	{
	   NWFSPrint("AHA you dirty dog!!!! passed bad read size!!!\n");
	   return -1;
	}
#endif

	if (nwvp_io_alloc((void **) &temp_buffer, 4096) == 0)
	{
		if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
		{
			if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
				(vpartition->vpartition_handle == vpart_handle))
			{
				if (vpartition->open_flag != 0)
				{
					relative_block = block_number;
					for (i=0; i<vpartition->segment_count; i++)
					{
						if (relative_block < vpartition->segments[i].segment_block_count)
						{
							if (inwvp_lpartition_block_read(
									vpartition->segments[i].lpart_link,
									relative_block + vpartition->segments[i].partition_block_offset,
									1,
									temp_buffer) == 0)
							{
#if !MANOS
								if (as)
								   nwvp_memmove(data_buffer,
										 &temp_buffer[block_offset], data_size);
								else
								{
								   if (NWFSCopyToUserSpace(data_buffer,
										   &temp_buffer[block_offset], data_size))
									 {
								      nwvp_io_free(temp_buffer);
								      return (-1);
									 }
								}
#else
								if (as == 0)
									nwvp_memmove(data_buffer, &temp_buffer[block_offset], data_size);
#endif
								nwvp_io_free(temp_buffer);
								return(0);
							}
							break;
						}
						relative_block -= vpartition->segments[i].segment_block_count;
					}
				}
			}
		}
		nwvp_io_free(temp_buffer);
	}
	return(-1);
}

ULONG	inwvp_vpartition_block_read(
	nwvp_vpartition	*vpartition,
	ULONG		block_number,
	ULONG		block_count,
	BYTE		*data_buffer)
{
	ULONG	i;
	ULONG	relative_block;

	if (vpartition->open_flag != 0)
	{
		relative_block = block_number;
		for (i=0; i<vpartition->segment_count; i++)
		{
			if (relative_block < vpartition->segments[i].segment_block_count)
			{
				return (inwvp_lpartition_block_read(
					vpartition->segments[i].lpart_link,
					relative_block + vpartition->segments[i].partition_block_offset,
					block_count,
					data_buffer));
			}
			relative_block -= vpartition->segments[i].segment_block_count;
		}
	}
	return(-1);
}

ULONG	nwvp_vpartition_map_asynch_read(
	ULONG		vpart_handle,
	ULONG		block_number,
	ULONG		*map_entry_count,
	nwvp_asynch_map	*map)
{
	ULONG	i;
	ULONG	disk_id;
	ULONG	disk_offset;
	ULONG	relative_block;
	ULONG	hotfix_block;
	ULONG	map_index = 0;
	nwvp_vpartition	*vpartition;
	nwvp_lpart	*lpart;

	if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
	{
		if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
			(vpartition->vpartition_handle == vpart_handle))
		{
			if (vpartition->open_flag != 0)
			{
				relative_block = block_number;
				for (i=0; i<vpartition->segment_count; i++)
				{
					if (relative_block < vpartition->segments[i].segment_block_count)
					{
						lpart = vpartition->segments[i].lpart_link;
						do
						{
							if (((lpart->mirror_status & MIRROR_PRESENT_BIT) != 0) &&
								((lpart->mirror_status & MIRROR_INSYNCH_BIT) != 0))
							{
								if (lpart->hotfix.bit_table[relative_block >> 15][(relative_block >> 5) & 0x3FF] & (1 << (relative_block & 0x1F)))
								{
									nwvp_part_map_to_physical(lpart->rpart_handle, &disk_id, &disk_offset, 0);
									nwvp_hotfix_block_lookup(lpart, relative_block, &hotfix_block, &map[map_index].bad_bits);
									map[map_index].sector_offset = (hotfix_block * 8) + disk_offset;
									map[map_index].disk_id = disk_id;
									map_index ++;
								}
								else
								{
									nwvp_part_map_to_physical(lpart->rpart_handle, &disk_id, &disk_offset, 0);
									map[map_index].bad_bits = 0;
									map[map_index].sector_offset = ((relative_block + lpart->hotfix.block_count +  vpartition->segments[i].partition_block_offset) * 8) + disk_offset;
									map[map_index].disk_id = disk_id;
									map_index ++;
								}
							}
							lpart = lpart->mirror_link;
						} while (lpart != vpartition->segments[i].lpart_link);
						*map_entry_count = map_index;
						return(0);
					}
					relative_block -= vpartition->segments[i].segment_block_count;
				}
			}
		}
	}
	return(-1);
}


ULONG	nwvp_vpartition_block_read(
	ULONG		vpart_handle,
	ULONG		block_number,
	ULONG		block_count,
	BYTE		*data_buffer)
{
	ULONG	i;
	ULONG	relative_block;
	ULONG		ccode;
	nwvp_vpartition	*vpartition;

	if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
	{
		if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
			(vpartition->vpartition_handle == vpart_handle))
		{
			if (vpartition->open_flag != 0)
			{
				relative_block = block_number;
				for (i=0; i<vpartition->segment_count; i++)
				{
					if (relative_block < vpartition->segments[i].segment_block_count)
					{
						ccode = inwvp_lpartition_block_read(
								vpartition->segments[i].lpart_link,
								relative_block + vpartition->segments[i].partition_block_offset,
								block_count,
								data_buffer);
						return(ccode);
					}
					relative_block -= vpartition->segments[i].segment_block_count;
				}
			}
		}
	}
	return(-1);
}

ULONG	nwvp_vpartition_sub_block_write(
	ULONG		vpart_handle,
	ULONG		block_number,
	ULONG		block_offset,
	ULONG		data_size,
	BYTE		*data_buffer,
	ULONG           as)
{
	ULONG	i;
	ULONG	relative_block;
	BYTE	*temp_buffer;
	nwvp_vpartition	*vpartition;
#if !MANOS
	if ((block_offset + data_size) > 4096)
	{
	   NWFSPrint("AHA you dirty dog!!!! passed bad write size!!!\n");
	   return -1;
	}
#endif

	if (nwvp_io_alloc((void **) &temp_buffer, 4096) == 0)
	{
		if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
		{
			if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
				(vpartition->vpartition_handle == vpart_handle))
			{
				if (vpartition->open_flag != 0)
				{
					relative_block = block_number;
					for (i=0; i<vpartition->segment_count; i++)
					{
						if (relative_block < vpartition->segments[i].segment_block_count)
						{
							if (inwvp_lpartition_block_read(
									vpartition->segments[i].lpart_link,
									relative_block + vpartition->segments[i].partition_block_offset,
									1,
									temp_buffer) == 0)
							{
#if !MANOS
								if (as)
								   nwvp_memmove(&temp_buffer[block_offset],
										data_buffer, data_size);
								else
								{
								   if (NWFSCopyFromUserSpace(&temp_buffer[block_offset],
											data_buffer, data_size))
									 {
									    nwvp_io_free(temp_buffer);
									    return (-1);
									 }
								}
#else
								if (as == 0)
								   nwvp_memmove(&temp_buffer[block_offset], data_buffer, data_size);
#endif
								if (inwvp_lpartition_block_write(
										vpartition->segments[i].lpart_link,
										relative_block + vpartition->segments[i].partition_block_offset,
										1,
										temp_buffer) == 0)
								{
									nwvp_io_free(temp_buffer);
									return(0);
								}
							}
							break;
						}
						relative_block -= vpartition->segments[i].segment_block_count;
					}
				}
			}
		}
		nwvp_io_free(temp_buffer);
	}
	return(-1);
}

ULONG	inwvp_vpartition_block_write(
	nwvp_vpartition	*vpartition,
	ULONG		block_number,
	ULONG		block_count,
	BYTE		*data_buffer)
{
	ULONG	i;
	ULONG	relative_block;

	if (vpartition->open_flag != 0)
	{
		relative_block = block_number;
		for (i=0; i<vpartition->segment_count; i++)
		{
			if (relative_block < vpartition->segments[i].segment_block_count)
			{
				return (inwvp_lpartition_block_write(
					vpartition->segments[i].lpart_link,
					relative_block + vpartition->segments[i].partition_block_offset,
					block_count,
					data_buffer));
			}
			relative_block -= vpartition->segments[i].segment_block_count;
		}
	}
	return(0x33000011);
}

ULONG	nwvp_vpartition_map_asynch_write(
	ULONG		vpart_handle,
	ULONG		block_number,
	ULONG		*map_entry_count,
	nwvp_asynch_map	*map)
{
	ULONG	i;
	ULONG	disk_id;
	ULONG	disk_offset;
	ULONG	relative_block;
	ULONG	hotfix_block;
	ULONG	map_index = 0;
	nwvp_vpartition	*vpartition;
	nwvp_lpart	*lpart;

	if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
	{
		if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
			(vpartition->vpartition_handle == vpart_handle))
		{
			if (vpartition->open_flag != 0)
			{
				relative_block = block_number;
				for (i=0; i<vpartition->segment_count; i++)
				{
					if (relative_block < vpartition->segments[i].segment_block_count)
					{
						lpart = vpartition->segments[i].lpart_link;
						do
						{
							if ((lpart->mirror_status & MIRROR_PRESENT_BIT) != 0)
							{
								if (lpart->hotfix.bit_table[relative_block >> 15][(relative_block >> 5) & 0x3FF] & (1 << (relative_block & 0x1F)))
								{
									nwvp_part_map_to_physical(lpart->rpart_handle, &disk_id, &disk_offset, 0);
									nwvp_hotfix_block_lookup(lpart, relative_block, &hotfix_block, &map[map_index].bad_bits);
									map[map_index].sector_offset = (hotfix_block * 8) + disk_offset;
									map[map_index].disk_id = disk_id;
									map_index ++;
								}
								else
								{
									nwvp_part_map_to_physical(lpart->rpart_handle, &disk_id, &disk_offset, 0);
									map[map_index].bad_bits = 0;
									map[map_index].sector_offset = ((relative_block + lpart->hotfix.block_count +  vpartition->segments[i].partition_block_offset) * 8) + disk_offset;
									map[map_index].disk_id = disk_id;
									map_index ++;
								}
							}
							lpart = lpart->mirror_link;
						} while (lpart != vpartition->segments[i].lpart_link);
						*map_entry_count = map_index;
						return(0);
					}
					relative_block -= vpartition->segments[i].segment_block_count;
				}
				return(0x22000011);
			}
			return(0x22000022);
		}
		return(0x22000033);
	}
	return(0x22000044);
}


ULONG	nwvp_vpartition_bad_bit_update(
	ULONG		vpart_handle,
	ULONG		block_number,
	ULONG		new_bad_bits)
{
	ULONG		i;
	ULONG		relative_block;
	ULONG		ccode;
	nwvp_vpartition	*vpartition;

	if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
	{
		if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
			(vpartition->vpartition_handle == vpart_handle))
		{
			if (vpartition->open_flag != 0)
			{
				relative_block = block_number;
				for (i=0; i<vpartition->segment_count; i++)
				{
					if (relative_block < vpartition->segments[i].segment_block_count)
					{
						ccode = inwvp_lpartition_bad_bit_update(
									vpartition->segments[i].lpart_link,
									relative_block + vpartition->segments[i].partition_block_offset,
									new_bad_bits);

						return(ccode);
					}
					relative_block -= vpartition->segments[i].segment_block_count;
				}
				return(0x11000011);
			}
			return(0x11000022);
		}
		return(0x11000033);
	}
	return(0x11000044);
}
ULONG	nwvp_vpartition_block_write(
	ULONG		vpart_handle,
	ULONG		block_number,
	ULONG		block_count,
	BYTE		*data_buffer)
{
	ULONG		i;
	ULONG		relative_block;
	ULONG		ccode;
	nwvp_vpartition	*vpartition;

	if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
	{
		if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
			(vpartition->vpartition_handle == vpart_handle))
		{
			if (vpartition->open_flag != 0)
			{
				relative_block = block_number;
				for (i=0; i<vpartition->segment_count; i++)
				{
					if (relative_block < vpartition->segments[i].segment_block_count)
					{
						ccode = inwvp_lpartition_block_write(
									vpartition->segments[i].lpart_link,
									relative_block + vpartition->segments[i].partition_block_offset,
									block_count,
									data_buffer);

						return(ccode);
					}
					relative_block -= vpartition->segments[i].segment_block_count;
				}
				return(0x11000011);
			}
			return(0x11000022);
		}
		return(0x11000033);
	}
	return(0x11000044);
}

ULONG	inwvp_vpartition_block_hotfix(
	nwvp_vpartition	*vpartition,
	ULONG		block_number,
	BYTE		*data_buffer)
{
	ULONG	i;
	ULONG	relative_block;

	if (vpartition->open_flag != 0)
	{
		relative_block = block_number;
		for (i=0; i<vpartition->segment_count; i++)
		{
			if (relative_block < vpartition->segments[i].segment_block_count)
			{
				return (inwvp_lpartition_block_hotfix(
					vpartition->segments[i].lpart_link,
					relative_block + vpartition->segments[i].partition_block_offset,
					data_buffer));
			}
			relative_block -= vpartition->segments[i].segment_block_count;
		}
	}
	return(-1);
}

ULONG	nwvp_vpartition_block_hotfix(
	ULONG		vpart_handle,
	ULONG		block_number,
	BYTE		*data_buffer)
{
	ULONG		i;
	ULONG		relative_block;
	ULONG		ccode;
	nwvp_vpartition	*vpartition;

	if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
	{
		if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
			(vpartition->vpartition_handle == vpart_handle))
		{
			if (vpartition->open_flag != 0)
			{
				relative_block = block_number;
				for (i=0; i<vpartition->segment_count; i++)
				{
					if (relative_block < vpartition->segments[i].segment_block_count)
					{
						ccode = inwvp_lpartition_block_hotfix(
									vpartition->segments[i].lpart_link,
									relative_block + vpartition->segments[i].partition_block_offset,
									data_buffer);

//						NWUnlockNWVP();
						return(ccode);
					}
					relative_block -= vpartition->segments[i].segment_block_count;
				}
			}
		}
	}
	return(-1);
}

ULONG	nwvp_vpartition_block_zero(
	ULONG		vpart_handle,
	ULONG		block_number,
	ULONG		block_count)
{
	ULONG	i, j;
	ULONG	relative_block;
	nwvp_vpartition	*vpartition;

	NWLockNWVP();
	if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
	{
		if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
			(vpartition->vpartition_handle == vpart_handle))
		{
			if (vpartition->open_flag != 0)
			{
				relative_block = block_number;
				for (i=0; i<vpartition->segment_count; i++)
				{
					if (relative_block < vpartition->segments[i].segment_block_count)
					{
						for (j=0; j<block_count; j++)
						{
							if (inwvp_lpartition_block_write(
									vpartition->segments[i].lpart_link,
									relative_block + vpartition->segments[i].partition_block_offset + j,
									1,
									zero_block_buffer) != 0)
							{
								return(-1);
							}
						}
						NWUnlockNWVP();
						return(0);
					}
					relative_block -= vpartition->segments[i].segment_block_count;
				}
			}
		}
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	nwvp_rpartition_scan(
	nwvp_payload	*payload)
{
	ULONG		index;
	ULONG		object_count;
	nwvp_rpartition_scan_info	*info;
	nwvp_lpart	*lpart;

	NWLockNWVP();
	object_count = (payload->payload_object_size_buffer_size & 0x000FFFFF) / 16;
	info = (nwvp_rpartition_scan_info *) payload->payload_buffer;
	index = payload->payload_index;
	payload->payload_object_count = 0;
	for (; index<raw_partition_table_count; index++)
	{
		if ((lpart = raw_partition_table[index]) != 0)
		{
			info->rpart_handle =  lpart->rpartition_handle;
			info->lpart_handle = lpart->lpartition_handle;
			info->physical_block_count = ((lpart->mirror_status & MIRROR_PRESENT_BIT) == 0) ? 0 :lpart->physical_block_count;
			info->partition_type = ((lpart->mirror_status & MIRROR_PRESENT_BIT) == 0) ? 0 : lpart->partition_type;
			info ++;
			payload->payload_object_count ++;

			if (payload->payload_object_count >= object_count)
			{
                index ++;
				for (; index<raw_partition_table_count; index++)
				{
					if ((lpart = raw_partition_table[index]) != 0)
					{
						payload->payload_index = index;
						NWUnlockNWVP();
						return(0);
					}
				}
				payload->payload_index = 0;
				NWUnlockNWVP();
				return(0);
			}
		}
	}
	payload->payload_index = 0;
	NWUnlockNWVP();
	return(0);
}

ULONG	nwvp_rpartition_map_rpart_handle(
	ULONG		*rpart_handle,
	ULONG		physical_handle)
{
	ULONG		i;
	nwvp_lpart		*lpart;

	for (i=0; i<raw_partition_table_count; i++)
	{
		if ((lpart = raw_partition_table[i]) != 0)
		{
			if (lpart->rpart_handle == physical_handle)
			{
				*rpart_handle = lpart->rpartition_handle;
				return(0);
			}
		}
	}
    return(-1);
}

ULONG	nwvp_rpartition_return_info(
	ULONG				rpart_handle,
	nwvp_rpartition_info		*rpart_info)
{
	nwvp_lpart		*lpart;
	if ((rpart_handle & 0x0000FFFF) < raw_partition_table_count)
	{
		if (((lpart = raw_partition_table[rpart_handle & 0x0000FFFF]) != 0) &&
			(lpart->rpartition_handle == rpart_handle))
		{
			nwvp_memset(rpart_info, 0, sizeof(nwvp_rpartition_info));
			rpart_info->rpart_handle =  rpart_handle;
			rpart_info->lpart_handle = lpart->lpartition_handle;
			if ((lpart->mirror_status & MIRROR_PRESENT_BIT) != 0)
			{
				rpart_info->physical_block_count = ((lpart->mirror_status & MIRROR_PRESENT_BIT) == 0) ? 0 :lpart->physical_block_count;
				rpart_info->partition_type = ((lpart->mirror_status & MIRROR_PRESENT_BIT) == 0) ? 0 : lpart->partition_type;
				rpart_info->physical_handle = ((lpart->mirror_status & MIRROR_PRESENT_BIT) == 0) ? 0 : lpart->rpart_handle;
				rpart_info->status = (lpart->mirror_status & 0x0000FFFF) | (lpart->primary_link->mirror_status & 0xFFFF0000);
				nwvp_part_map_to_physical(lpart->rpart_handle, &rpart_info->physical_disk_number, &rpart_info->partition_offset, &rpart_info->partition_number);
			}
			return(0);
		}
	}
	return(-1);
}

ULONG	nwvp_lpartition_scan(
	nwvp_payload	*payload)
{
	ULONG		index;
	ULONG		temp_section;
	ULONG		object_count;
	ULONG		percent;
	nwvp_lpartition_scan_info	*info;
	nwvp_lpart	*lpart;
	nwvp_lpart	*temp;

	NWLockNWVP();
	object_count = (payload->payload_object_size_buffer_size & 0x000FFFFF) / 16;
	info = (nwvp_lpartition_scan_info *) payload->payload_buffer;
	index = payload->payload_index;
	payload->payload_object_count = 0;
	for (; index<raw_partition_table_count; index++)
	{
		if ((lpart = logical_partition_table[index]) != 0)
		{
			info->lpart_handle = lpart->lpartition_handle;
			info->logical_block_count = lpart->logical_block_count;
			info->open_flag = (lpart->mirror_status & MIRROR_GROUP_OPEN_BIT) ? 0xFFFFFFFF : 0;
			temp_section = 0;
			info->mirror_status = 0;
			temp = lpart;
			do
			{
				info->mirror_status += 1;
				if (temp->mirror_status & MIRROR_PRESENT_BIT)
				{
					info->mirror_status += 0x100;
					if (temp->remirror_section > temp_section)
					{
						if (temp->remirror_section < (temp->logical_block_count >> 10))
							temp_section = temp->remirror_section;
						else
							temp_section = temp->logical_block_count >> 10;
					}
				}
				if (temp->mirror_status & MIRROR_INSYNCH_BIT)
					info->mirror_status += 0x10000;
				temp = temp->mirror_link;
			} while (temp != lpart);
			percent = 50;
			if ((temp->logical_block_count >> 10) > 0)
				percent = ((((temp->logical_block_count >> 10) - temp_section) * 100) / (temp->logical_block_count >> 10));

			info->mirror_status += ((percent == 100) && (temp_section != 0)) ? 99 << 24 : percent << 24;

			info ++;
			payload->payload_object_count ++;

			if (payload->payload_object_count >= object_count)
			{
				for (; index<raw_partition_table_count; index++)
				{
					if ((lpart = raw_partition_table[index]) != 0)
					{
						if (lpart->lpartition_handle != 0xFFFFFFFF)
						{
							payload->payload_index = index + 1;
							NWUnlockNWVP();
							return(0);
						}
					}
				}
				payload->payload_index = 0;
				NWUnlockNWVP();
				return(0);
			}
		}
	}
	payload->payload_index = 0;
	NWUnlockNWVP();
	return(0);
}

ULONG	nwvp_lpartition_return_info(
	ULONG				lpart_handle,
	nwvp_lpartition_info		*lpart_info)
{
	nwvp_lpart		*lpart, *temp;

	nwvp_memset(lpart_info, 0, sizeof(nwvp_lpartition_info));
	NWLockNWVP();
	if ((lpart_handle & 0x0000FFFF) < logical_partition_table_count)
	{
		if (((lpart = logical_partition_table[lpart_handle & 0x0000FFFF]) != 0) &&
			(lpart->lpartition_handle == lpart_handle))
		{
			lpart_info->lpartition_handle = lpart->lpartition_handle;
			lpart_info->lpartition_type = lpart->lpartition_type;
			lpart_info->segment_count = (lpart->segment_sector == 0) ? 0 : lpart->segment_sector->NumberOfTableEntries;
			lpart_info->mirror_count = lpart->mirror_count;
			lpart_info->logical_block_count = lpart->logical_block_count;
			lpart_info->lpartition_status = lpart->mirror_status;
			lpart_info->open_count = lpart->open_count;
			temp = lpart;
			do
			{
				lpart_info->m[temp->mirror_sector_index].rpart_handle = temp->rpartition_handle;
				lpart_info->m[temp->mirror_sector_index].hotfix_block_count = temp->physical_block_count - temp->logical_block_count;
				lpart_info->m[temp->mirror_sector_index].status = temp->mirror_status;
				temp = temp->mirror_link;
			} while (temp != lpart);
			NWUnlockNWVP();
			return(0);
		}
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	nwvp_lpartition_return_space_info(
	ULONG			lpart_handle,
	nwvp_payload	*payload)
{
	ULONG		index1;
	ULONG		index2;
	ULONG		sindex;
	ULONG		current_blocks;
	ULONG		object_count;
	nwvp_lpartition_space_return_info	*info;
	nwvp_lpart	*lpart;

	NWLockNWVP();
	if ((lpart_handle & 0x0000FFFF) < logical_partition_table_count)
	{
		if (((lpart = logical_partition_table[lpart_handle & 0x0000FFFF]) != 0) &&
			(lpart->lpartition_handle == lpart_handle) &&
			((lpart->mirror_status & MIRROR_GROUP_ACTIVE_BIT) != 0))
		{
			object_count = (payload->payload_object_size_buffer_size & 0x000FFFFF) / 16;
			info = (nwvp_lpartition_space_return_info *) payload->payload_buffer;
			sindex = payload->payload_index;
			index1 = 0;
			index2 = 0;
			payload->payload_object_count = 0;
			current_blocks = 0x14;
			while (current_blocks < lpart->logical_block_count)
			{
				if ((lpart->segment_sector != 0) &&
					(index1 < lpart->segment_sector->NumberOfTableEntries))
				{
					if ((lpart->segment_sector->VolumeEntry[index1].VolumeRoot / lpart->sectors_per_block) == current_blocks)
					{
						if (index2 == sindex)
						{
							info->lpart_handle = lpart->lpartition_handle;
							info->segment_offset = lpart->segment_sector->VolumeEntry[index1].VolumeRoot / lpart->sectors_per_block;
							info->segment_count = lpart->segment_sector->VolumeEntry[index1].SegmentSectors / lpart->sectors_per_block;
							info->status_bits = INUSE_BIT;
							if (lpart->mirror_link != lpart)
								info->status_bits |= MIRROR_BIT;
							info ++;
							sindex ++;

							payload->payload_object_count ++;
							if (payload->payload_object_count >= object_count)
							{
								payload->payload_index = sindex;
								NWUnlockNWVP();
								return(0);
							}
						}
						current_blocks = (lpart->segment_sector->VolumeEntry[index1].VolumeRoot + lpart->segment_sector->VolumeEntry[index1].SegmentSectors) / lpart->sectors_per_block;
						index2 ++;
						index1 ++;
					}
					else
					{
						if (index2 == sindex)
						{
							info->lpart_handle = lpart->lpartition_handle;
							info->segment_offset = current_blocks;
							info->segment_count = (lpart->segment_sector->VolumeEntry[index1].VolumeRoot / lpart->sectors_per_block) - current_blocks;
							info->status_bits = 0;
							if (lpart->mirror_link != lpart)
								info->status_bits |= MIRROR_BIT;
							info ++;
							sindex ++;

							payload->payload_object_count ++;
							if (payload->payload_object_count >= object_count)
							{
								payload->payload_index = sindex;
								NWUnlockNWVP();
								return(0);
							}
						}
						current_blocks = lpart->segment_sector->VolumeEntry[index1].VolumeRoot / lpart->sectors_per_block;
						index2 ++;
					}
				}
				else
				{
					if (index2 == sindex)
					{
						if ((info->segment_count = (lpart->logical_block_count - current_blocks)) != 0)
						{
							info->lpart_handle = lpart->lpartition_handle;
							info->segment_offset = current_blocks;
							info->status_bits = 0;
							if (lpart->mirror_link != lpart)
								info->status_bits |= MIRROR_BIT;
							payload->payload_object_count ++;
							if (payload->payload_object_count >= object_count)
							{
								payload->payload_index = 0;
								NWUnlockNWVP();
								return(0);
							}
						}
					}
					break;
				}
			}
		}
	}
	payload->payload_index = 0;
	NWUnlockNWVP();
	return(0);
}

ULONG	nwvp_vpartition_scan(
	nwvp_payload	*payload)
{
	ULONG		index;
	ULONG		object_count;
	ULONG		*handles;
	nwvp_vpartition	*vpartition;

	NWLockNWVP();
	object_count = (payload->payload_object_size_buffer_size & 0x000FFFFF) / 4;
	handles = (ULONG *) payload->payload_buffer;
	index = payload->payload_index;
	payload->payload_object_count = 0;
	for (; index<virtual_partition_table_count; index++)
	{
		if ((vpartition = virtual_partition_table[index]) != 0)
		{
			*handles = vpartition->vpartition_handle;
			handles ++;
			payload->payload_object_count ++;

			if (payload->payload_object_count >= object_count)
			{
				for (; index<virtual_partition_table_count; index++)
				{
					if ((vpartition = virtual_partition_table[index]) != 0)
					{
						payload->payload_index = index + 1;
						NWUnlockNWVP();
						return(0);
					}
				}
				payload->payload_index = 0;
				NWUnlockNWVP();
				return(0);
			}
		}
	}
	payload->payload_index = 0;
	NWUnlockNWVP();
	return(0);
}


ULONG	nwvp_vpartition_return_info(
	ULONG						vpart_handle,
	nwvp_vpartition_info		*vpinfo)
{
	ULONG			i;
	nwvp_vpartition	*vpartition;

	NWLockNWVP();
	if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
	{
		if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
			(vpartition->vpartition_handle == vpart_handle))
		{
			nwvp_memmove(&vpinfo->volume_name[0], &vpartition->volume_name[0], 16);
			vpinfo->volume_valid_flag = vpartition->volume_valid_flag;
			vpinfo->open_flag = vpartition->open_flag;
			vpinfo->mirror_flag = vpartition->mirror_flag;
			vpinfo->cluster_count = vpartition->cluster_count;
			vpinfo->segment_count = vpartition->segment_count;
			vpinfo->vpart_handle = vpart_handle;
			vpinfo->blocks_per_cluster = vpartition->blocks_per_cluster;
			vpinfo->FAT1 = vpartition->FAT1;
			vpinfo->FAT2 = vpartition->FAT2;
			vpinfo->Directory1 = vpartition->Directory1;
			vpinfo->Directory2 = vpartition->Directory2;
			for (i=0; i<vpartition->segment_count; i++)
			{
				vpinfo->segments[i].segment_block_count = vpartition->segments[i].segment_block_count;
				vpinfo->segments[i].partition_block_offset = vpartition->segments[i].partition_block_offset;
				vpinfo->segments[i].relative_cluster_offset = vpartition->segments[i].relative_cluster_offset;
				vpinfo->segments[i].lpart_handle = (vpartition->segments[i].lpart_link != 0) ?
					vpartition->segments[i].lpart_link->lpartition_handle:
					0xFFFFFFFF;
			}
			NWUnlockNWVP();
			return(0);
		}
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	nwvp_lpartition_remirror(
	ULONG	lpart_handle)
{
	nwvp_lpart	*lpart;
	NWLockNWVP();
	if ((lpart_handle & 0x0000FFFF) < logical_partition_table_count)
	{
		if (((lpart = logical_partition_table[lpart_handle & 0x0000FFFF]) != 0) &&
			 (lpart->lpartition_handle == lpart_handle))
		{
			if (nwvp_start_remirror(lpart) == 0)
			{
				NWUnlockNWVP();
				return(0);
			}
		}
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	nwvp_lpartition_abort_remirror(
	ULONG	lpart_handle)
{
	nwvp_lpart	*lpart;
	NWLockNWVP();
	if ((lpart_handle & 0x0000FFFF) < logical_partition_table_count)
	{
		if (((lpart = logical_partition_table[lpart_handle & 0x0000FFFF]) != 0) &&
			 (lpart->lpartition_handle == lpart_handle))
		{
			if (nwvp_stop_remirror(lpart) == 0)
			{
				NWUnlockNWVP();
				return(0);
			}
		}
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	nwvp_lpartition_unformat(
	ULONG	lpart_handle)
{
	nwvp_lpart	*lpart;
	nwvp_lpart	*temp;

	NWLockNWVP();
	if ((lpart_handle & 0x0000FFFF) < logical_partition_table_count)
	{
		if (((lpart = logical_partition_table[lpart_handle & 0x0000FFFF]) != 0) &&
			 (lpart->lpartition_handle == lpart_handle) &&
			 (lpart->mirror_link == lpart))
		{
			logical_partition_table[lpart_handle & 0x0000FFFF] = 0;
			while ((temp = lpart->mirror_link) != lpart)
			{
				lpart->mirror_link = temp->mirror_link;
				nwvp_hotfix_destroy(temp);
				temp->lpartition_handle = 0xFFFFFFFF;
			}
			nwvp_hotfix_destroy(temp);
			logical_partition_table[lpart_handle & 0x0000FFFF] = 0;
			temp->lpartition_handle = 0xFFFFFFFF;
			NWUnlockNWVP();
			return(0);
		}
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	nwvp_lpartition_format(
	ULONG	*lpartition_id,
	ULONG	logical_block_count,
	ULONG	*base_partition_ids)
{
	ULONG	i;
	nwvp_lpart	*lparts[4];
	nwvp_lpart	*base_part = 0;

	NWLockNWVP();
	for (i=0; i<4; i++)
	{
		lparts[i] = 0;
		if (base_partition_ids[i] != 0xFFFFFFFF)
		{
			if ((base_partition_ids[i] & 0x0000FFFF) < raw_partition_table_count)
			{
				if (((lparts[i] = raw_partition_table[base_partition_ids[i] & 0x0000FFFF]) != 0) &&
					(lparts[i]->rpartition_handle == base_partition_ids[i]))
				{
					if (base_part == 0)
						base_part = lparts[i];
					if ((lparts[i]->partition_type != 0x65) && (lparts[i]->partition_type != 0x77))
					{
						NWUnlockNWVP();
						return(-1);
					}
					if (lparts[i]->lpartition_handle != 0xFFFFFFFF)
					{
						NWUnlockNWVP();
						return(-1);
					}
					if (lparts[i]->physical_block_count < (logical_block_count + 64))
					{
						NWUnlockNWVP();
						return(-1);
					}
				}
				else
					lparts[i] = 0;
			}
		}
	}
	if (base_part == 0)
	{
		NWUnlockNWVP();
		return(-1);
	}
	for (i=0; i < 4; i++)
	{
		if (lparts[i] != 0)
		{
			if (nwvp_hotfix_create(lparts[i], logical_block_count) != 0)
			{
				for (; i > 0; i--)
				{
					if (lparts[i-1] != 0)
						nwvp_hotfix_destroy(lparts[i-i]);
				}
				NWUnlockNWVP();
				return(-1);
			}
		}
	}
	nwvp_mirror_group_add(base_part);
	for (i=0; i<4; i++)
	{
		if (lparts[i] != 0)
		{
			if (lparts[i] != base_part)
			{
				lparts[i]->lpartition_handle = base_part->lpartition_handle;
				nwvp_mirror_member_add(base_part, lparts[i]);
			}
		}
	}
	nwvp_mirror_group_establish(base_part, 0);
	*lpartition_id = base_part->lpartition_handle;
	NWUnlockNWVP();
	return(0);
}


ULONG	nwvp_partition_add_mirror(
	ULONG	lpart_handle,
	ULONG	rpart_handle)
{
    ULONG       temp_handle;
	nwvp_lpart	*rpart;
	nwvp_lpart	*lpart;

	NWLockNWVP();
	if ((lpart_handle & 0x0000FFFF) < logical_partition_table_count)
	{
		if (((lpart = logical_partition_table[lpart_handle & 0x0000FFFF]) != 0) &&
			 (lpart->lpartition_handle == lpart_handle) &&
			 ((lpart->mirror_status & MIRROR_GROUP_ACTIVE_BIT) != 0))
		{
			if ((rpart_handle & 0x0000FFFF) < raw_partition_table_count)
			{
				if (((rpart = raw_partition_table[rpart_handle & 0x0000FFFF]) != 0) &&
					(rpart->rpartition_handle == rpart_handle))
				{
					if (rpart->physical_block_count < (lpart->logical_block_count + 64))
					{
						NWUnlockNWVP();
						return(-1);
					}
                    if ((temp_handle = rpart->lpartition_handle) != 0xFFFFFFFF)
                    {
                        if (rpart->mirror_link != rpart)
                        {
						    NWUnlockNWVP();
						    return(-1);
					    }
                        nwvp_hotfix_destroy(rpart);
                        logical_partition_table[temp_handle & 0x0000FFFF] = 0;
			            rpart->lpartition_handle = 0xFFFFFFFF;
                    }
					if (nwvp_hotfix_create(rpart, lpart->logical_block_count) == 0)
					{
						rpart->lpartition_handle = lpart->lpartition_handle;
						nwvp_mirror_member_add(lpart, rpart);
						NWUnlockNWVP();
						return(0);
					}
				}
			}
		}
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	nwvp_partition_del_mirror(
	ULONG	lpart_handle,
	ULONG	rpart_handle)
{
	nwvp_lpart	*rpart;
	nwvp_lpart	*lpart;

	NWLockNWVP();
	if ((lpart_handle & 0x0000FFFF) < logical_partition_table_count)
	{
		if (((lpart = logical_partition_table[lpart_handle & 0x0000FFFF]) != 0) &&
			 (lpart->lpartition_handle == lpart_handle) &&
			 ((lpart->mirror_status & MIRROR_GROUP_ACTIVE_BIT) != 0) &&
			 (lpart->mirror_count > 1))
		{
			if ((rpart_handle & 0x0000FFFF) < raw_partition_table_count)
			{
				if (((rpart = raw_partition_table[rpart_handle & 0x0000FFFF]) != 0) &&
					(rpart->rpartition_handle == rpart_handle))
				{
					if (nwvp_mirror_member_delete(rpart) == 0)
					{
						NWUnlockNWVP();
						return(0);
					}
				}
			}
		}
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	nwvp_bounds_check(
	ULONG	start1,
	ULONG	count1,
	ULONG	start2,
	ULONG	count2)
{
	if (start2 >= 0xA0)
	{
		if ((start2 + count2) > (start1 + count1))
			return(0);
		if ((start2 + count2) <= start1)
			return(0);
	}
	return(-1);
}

ULONG	nwvp_vpartition_build_directory(
	nwvp_vpartition	*vpartition,
	ULONG	flags)
{
	ULONG	i;
	ULONG	dir_block1;
	ULONG	dir_block2;
	BYTE	*dir_table;

	nwvp_io_alloc((void **) &dir_table, 4096);
	nwvp_memset(dir_table, 0, 4096);
	dir_block1 = vpartition->Directory1 * vpartition->blocks_per_cluster;
	dir_block2 = vpartition->Directory2 * vpartition->blocks_per_cluster;

#if !MANOS
	if (CreateRootDirectory(dir_table,
				IO_BLOCK_SIZE,
				flags,
				vpartition->blocks_per_cluster *
				IO_BLOCK_SIZE) == 0)
	{
#else
	if (vpartition != 0)		// dummy check
	{
		nwvp_memset(dir_table, 0, 4096);
		for (i=0; i<4096; i+=128)
		{
			dir_table[i + 0] = 0xFF;
			dir_table[i + 1] = 0xFF;
			dir_table[i + 2] = 0xFF;
			dir_table[i + 3] = 0xFF;
		}
#endif
		inwvp_vpartition_block_write(vpartition, dir_block1, 1, (BYTE *) dir_table);
		inwvp_vpartition_block_write(vpartition, dir_block2, 1, (BYTE *) dir_table);
		nwvp_memset(dir_table, 0, 4096);
		for (i=0; i<4096; i+=128)
		{
			dir_table[i + 0] = 0xFF;
			dir_table[i + 1] = 0xFF;
			dir_table[i + 2] = 0xFF;
			dir_table[i + 3] = 0xFF;
		}
		for (i=1; i<vpartition->blocks_per_cluster; i++)
		{
			dir_block1 ++;
			dir_block2 ++;
			inwvp_vpartition_block_write(vpartition, dir_block1, 1, (BYTE *) dir_table);
			inwvp_vpartition_block_write(vpartition, dir_block2, 1, (BYTE *) dir_table);
		}
		nwvp_io_free(dir_table);
		return(0);
	}
	nwvp_io_free(dir_table);
	return(-1);
}

/**************************************************************************
*
*	The NetWare VRepair program assumes that the fat tables will be organized
*  in a specific manor.  Depending upon the cluster size, the second fat table
*  will always begin at a certain block offset defined in gap_table[].  The
*  fat chains then alternate.
*
*  (32k example:  F1=0 - 7  F2=8 - 15 F1= 16 - 23 F2= 24 - 31 ...
*
*  After the fats are layed out, the directory tables are allocated in the
*  holes or at the end.
*
***************************************************************************/


ULONG	gap_table[] = { 0x00, 0x40, 0x20, 0x00,
						0x10, 0x00, 0x00, 0x00,
						0x08, 0x00, 0x00, 0x00,
						0x00, 0x00, 0x00, 0x00,
						0x04};

ULONG	nwvp_vpartition_fat_check(
	ULONG		vpart_handle)
{
	ULONG	close_flag = 0;
	ULONG	pass;
	ULONG	gap;
	ULONG	fat_index;
	ULONG	current_index;
	ULONG	current_block_offset;
    ULONG	current_block_entry;
	ULONG	entries_per_cluster;
	ULONG	entries_per_block;
	ULONG	total_fat_clusters;
	FAT_ENTRY 			*fat_block;
	nwvp_vpartition		*vpartition;

	nwvp_io_alloc((void **) &fat_block, 4096);
	NWLockNWVP();
	if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
	{
		if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
			(vpartition->vpartition_handle == vpart_handle))
		{
			if (vpartition->open_flag == 0)
			{
				inwvp_vpartition_open(vpartition);
				close_flag = 1;
			}
			entries_per_cluster = ((vpartition->blocks_per_cluster << 12) >> 3);
			entries_per_block = 4096 / sizeof(FAT_ENTRY);
			gap = gap_table[vpartition->blocks_per_cluster];
			total_fat_clusters = (vpartition->cluster_count + (entries_per_cluster -1)) / entries_per_cluster;

			fat_index = 0;
			pass = 0;
			while (1)
			{
				current_index = 0;
				current_block_offset = 0;
				current_block_entry = 0x200;
				inwvp_vpartition_block_read(vpartition, current_block_offset, 1, (BYTE *) fat_block);
				while (fat_block[fat_index & 0x1FF].FATCluster != (long) 0xFFFFFFFF)
				{
					if (fat_block[fat_index & 0x1FF].FATIndex != (long) current_index)
						break;

					if (fat_block[fat_index & 0x1FF].FATCluster >= (long) current_block_entry)
					{
						current_block_entry = fat_block[fat_index & 0x1FF].FATCluster / entries_per_block;
						if ((current_block_entry / vpartition->blocks_per_cluster) == current_index)
							current_block_offset = (fat_index * vpartition->blocks_per_cluster) + (current_block_entry % vpartition->blocks_per_cluster);
						else
							current_block_offset = fat_block[fat_index & 0x1FF].FATCluster * vpartition->blocks_per_cluster;
						fat_index = fat_block[fat_index & 0x1FF].FATCluster;
						current_block_entry = (current_block_entry + 1) * entries_per_block;
						inwvp_vpartition_block_read(vpartition, current_block_offset, 1, (BYTE *) fat_block);
					}
					else
						fat_index = fat_block[fat_index & 0x1FF].FATCluster;
					current_index ++;
				}
				if (fat_block[fat_index & 0x1FF].FATCluster != (long) 0xFFFFFFFF)
					break;
				if (fat_block[fat_index & 0x1FF].FATIndex != (long) current_index)
					break;
				if ((current_index + 1) != total_fat_clusters)
					break;
				if (pass == 1)
				{
					if (close_flag != 0)
						inwvp_vpartition_close(vpartition);
					NWUnlockNWVP();
					nwvp_io_free(fat_block);
					return(0);
				}
				pass = 1;
				fat_index = gap;
			}
			if (close_flag != 0)
				inwvp_vpartition_close(vpartition);

		}
	}
	NWUnlockNWVP();
	nwvp_io_free(fat_block);
	return(-1);
}

ULONG	nwvp_vpartition_build_fat(
	nwvp_vpartition	*vpartition,
	ULONG		current_segment)
{
	ULONG	i, j, k;
	ULONG	entries_per_block;
	ULONG	entries_per_cluster;
	ULONG	current_index;
	ULONG	base_index;
	ULONG	fat_index;
	ULONG	next_index;
	ULONG	dir_flag = 0;
	ULONG	current_fat_clusters;
	ULONG	previous_fat_clusters;
	ULONG	prev_offset;
	ULONG	current_block_offset;
	ULONG	current_block_entry;
	ULONG	extra_block_offset;
	ULONG	gap;
	ULONG	block_gap;
	ULONG	gap_fudge;
	FAT_ENTRY *fat_block;


	nwvp_io_alloc((void **) &fat_block, 4096 * 2);
	nwvp_memset(fat_block, 0, 4096 * 2);

	entries_per_cluster = ((vpartition->blocks_per_cluster << 12) >> 3);
	entries_per_block = 4096 / sizeof(FAT_ENTRY);
	gap = gap_table[vpartition->blocks_per_cluster];
	block_gap = gap * vpartition->blocks_per_cluster;

	if (current_segment == 0)
	{
		dir_flag = 1;
		current_index = 0;
		prev_offset = 0;
		previous_fat_clusters = 0;
		for (k=0; k<vpartition->blocks_per_cluster; k++)
		{
			inwvp_vpartition_block_write(vpartition, 0 + k, 1, (BYTE *) zero_block_buffer);
			inwvp_vpartition_block_write(vpartition, 0 + block_gap + k, 1, (BYTE *) zero_block_buffer);
		}
	}
	else
	{
		fat_index = 0;
		current_index = 0;
		current_block_offset = 0;
		current_block_entry = 0x200;
		inwvp_vpartition_block_read(vpartition, current_block_offset, 2, (BYTE *) fat_block);
		while (fat_block[fat_index & 0x1FF].FATCluster != 0xFFFFFFFF)
		{
			if (fat_block[fat_index & 0x1FF].FATIndex != (long) current_index)
				break;
			if (fat_block[fat_index & 0x1FF].FATCluster >= (long) current_block_entry)
			{
				current_block_entry = fat_block[fat_index & 0x1FF].FATCluster / entries_per_block;
				if ((current_block_entry / vpartition->blocks_per_cluster) == current_index)
					current_block_offset = (fat_index * vpartition->blocks_per_cluster) + (current_block_entry % vpartition->blocks_per_cluster);
				else
					current_block_offset = fat_block[fat_index & 0x1FF].FATCluster * vpartition->blocks_per_cluster;
				fat_index = fat_block[fat_index & 0x1FF].FATCluster;
				current_block_entry = (current_block_entry + 1) * entries_per_block;
				inwvp_vpartition_block_read(vpartition, current_block_offset, 2, (BYTE *) fat_block);
			}
			else
				fat_index = fat_block[fat_index & 0x1FF].FATCluster;
			current_index ++;
		}
		prev_offset = fat_index;
		current_index ++;
		current_fat_clusters = vpartition->segments[current_segment].relative_cluster_offset + (vpartition->segments[current_segment].segment_block_count / vpartition->blocks_per_cluster);
		current_fat_clusters = (current_fat_clusters + (entries_per_cluster -1)) / entries_per_cluster;
		previous_fat_clusters = current_index;
		if (current_fat_clusters <= previous_fat_clusters)
		{
			nwvp_io_free(fat_block);
			return(0);
		}
		fat_block[fat_index & 0x1FF].FATCluster = vpartition->segments[current_segment].relative_cluster_offset;
		fat_block[(fat_index & 0x1FF) + gap].FATCluster = vpartition->segments[current_segment].relative_cluster_offset + gap;
		inwvp_vpartition_block_write(vpartition, current_block_offset, 2, (BYTE *) fat_block);
		inwvp_vpartition_block_write(vpartition, current_block_offset + block_gap, 2, (BYTE *) fat_block);
	}

	for (i=current_segment; i<vpartition->segment_count; i++)
	{
		current_fat_clusters = vpartition->segments[i].relative_cluster_offset + (vpartition->segments[i].segment_block_count / vpartition->blocks_per_cluster);
		current_fat_clusters = (current_fat_clusters + (entries_per_cluster -1)) / entries_per_cluster;
		fat_index = vpartition->segments[i].relative_cluster_offset;
		base_index = current_index;
		gap_fudge = gap - (fat_index % gap);
		if ((fat_index / entries_per_cluster) < base_index)
		{
			current_block_offset = (prev_offset * vpartition->blocks_per_cluster) + ((fat_index / entries_per_block) % vpartition->blocks_per_cluster);
			if (((fat_index + gap) / entries_per_cluster) < base_index)
				extra_block_offset = current_block_offset + 1;
			else
				extra_block_offset = vpartition->segments[i].relative_cluster_offset * vpartition->blocks_per_cluster;
		}
		else
		{
			current_block_offset = vpartition->segments[i].relative_cluster_offset * vpartition->blocks_per_cluster;
			extra_block_offset = current_block_offset + 1;
		}
		inwvp_vpartition_block_read(vpartition, current_block_offset, 1, (BYTE *) fat_block);
		nwvp_memset(&fat_block[0x200], 0, 4096);
		prev_offset = vpartition->segments[i].relative_cluster_offset * vpartition->blocks_per_cluster;

		for (j=0; j < (current_fat_clusters - previous_fat_clusters); j++)
		{

			for (k=0; k<vpartition->blocks_per_cluster; k++)
			{
				inwvp_vpartition_block_write(vpartition, (fat_index * vpartition->blocks_per_cluster) + k, 1, (BYTE *) zero_block_buffer);
				inwvp_vpartition_block_write(vpartition, (fat_index * vpartition->blocks_per_cluster) + block_gap + k, 1, (BYTE *) zero_block_buffer);
			}
			if ((j+1) < (current_fat_clusters - previous_fat_clusters))
			{
				fat_block[fat_index & 0x1FF].FATIndex = current_index;
				fat_block[(fat_index & 0x1FF) + gap].FATIndex = current_index;
				if (((fat_index + gap_fudge + 1) % gap) == 0)
				{
					fat_block[fat_index & 0x1FF].FATCluster = fat_index + gap + 1;
					fat_block[(fat_index & 0x1FF) + gap].FATCluster = fat_index + gap + gap + 1;
				}
				else
				{
					fat_block[fat_index & 0x1FF].FATCluster = fat_index + 1;
					fat_block[(fat_index & 0x1FF) + gap].FATCluster = fat_index + gap + 1;
				}
				if (((fat_index & 0x1FF) == 0x1FF) || (((fat_index+gap) & 0x1FF) == 0x1FF))
				{
					inwvp_vpartition_block_write(vpartition, current_block_offset, 1, (BYTE *) fat_block);
					inwvp_vpartition_block_write(vpartition, current_block_offset + block_gap, 1, (BYTE *) fat_block);
					if (((fat_index & 0x1FF) + gap) > 0x200)
					{
						inwvp_vpartition_block_write(vpartition, extra_block_offset, 1, (BYTE *) &fat_block[0x200]);
						inwvp_vpartition_block_write(vpartition, extra_block_offset + block_gap, 1, (BYTE *) &fat_block[0x200]);
					}
					nwvp_memmove(fat_block, &fat_block[0x200], 4096);
					nwvp_memset(&fat_block[0x200], 0, 4096);

					if (current_block_offset < (vpartition->segments[i].relative_cluster_offset * vpartition->blocks_per_cluster))
					{
						current_block_offset = vpartition->segments[i].relative_cluster_offset * vpartition->blocks_per_cluster;
						extra_block_offset = current_block_offset + 1;
					}
					else
					{
						current_block_offset ++;
						extra_block_offset ++;
					}

					if (((fat_index + gap_fudge + 1) % gap) == 0)
						fat_index += gap;
					fat_index ++;

					if ((fat_index / entries_per_cluster) < base_index)
					{
						current_block_offset ++;
						if (((fat_index + gap) / entries_per_cluster) < current_index)
							extra_block_offset = current_block_offset + 1;
						else
							extra_block_offset = vpartition->segments[i].relative_cluster_offset * vpartition->blocks_per_cluster;
					}
					else
					{
						if (current_block_offset < (vpartition->segments[i].relative_cluster_offset * vpartition->blocks_per_cluster))
						{
							current_block_offset = vpartition->segments[i].relative_cluster_offset * vpartition->blocks_per_cluster;
							extra_block_offset = current_block_offset + 1;
						}
						else
						{
							current_block_offset ++;
							extra_block_offset ++;
						}
					}
				}
				else
				{
					if (((fat_index + gap_fudge + 1) % gap) == 0)
						fat_index += gap;
					fat_index ++;
				}
			}
			else
			{
				fat_block[fat_index & 0x1FF].FATIndex = current_index;
				fat_block[(fat_index & 0x1FF) + gap].FATIndex = current_index;
				while ((i+1) < vpartition->segment_count)
				{
					next_index = (vpartition->segments[i+1].relative_cluster_offset + (vpartition->segments[i+1].segment_block_count / vpartition->blocks_per_cluster)) / entries_per_cluster;
					if (next_index == current_index)
						i++;
					else
					{
						fat_block[fat_index & 0x1FF].FATCluster = vpartition->segments[i+1].relative_cluster_offset;
						fat_block[(fat_index & 0x1FF) + gap].FATCluster = vpartition->segments[i+1].relative_cluster_offset + gap;
						break;
					}
				}
				if ((i+1) == vpartition->segment_count)
				{
					fat_block[fat_index & 0x1FF].FATCluster = 0xFFFFFFFF;
					fat_block[(fat_index & 0x1FF) + gap].FATCluster = 0xFFFFFFFF;
				}
				if (dir_flag == 1)
				{
					dir_flag = 0;
					for (k=0; k<entries_per_block - (gap+1); k++)
					{
						if ((fat_block[k].FATCluster == 0) && (fat_block[k + gap + 1].FATCluster == 0))
						{
							vpartition->FAT1 = 0;
							vpartition->FAT2 = gap;
							vpartition->Directory1 = (fat_index & 0xFFFFFE00) + k;
							vpartition->Directory2 = (fat_index & 0xFFFFFE00) + k + gap + 1;
							fat_block[k].FATCluster = 0xFFFFFFFF;
							fat_block[k + gap + 1].FATCluster = 0xFFFFFFFF;
							break;
						}
					}
				}
				inwvp_vpartition_block_write(vpartition, current_block_offset, 1, (BYTE *) fat_block);
				inwvp_vpartition_block_write(vpartition, current_block_offset + block_gap, 1, (BYTE *) fat_block);
				if (((fat_index & 0x1FF) + gap) > 0x200)
				{
					inwvp_vpartition_block_write(vpartition, extra_block_offset, 1, (BYTE *) &fat_block[0x200]);
					inwvp_vpartition_block_write(vpartition, extra_block_offset + block_gap, 1, (BYTE *) &fat_block[0x200]);
				}
				prev_offset = fat_index;
			}
			current_index ++;
		}
		previous_fat_clusters = current_fat_clusters;
	}
	nwvp_io_free(fat_block);
	return(0);
}

ULONG	nwvp_vpartition_segment_flush(
	nwvp_vpartition	*vpartition)
{
	ULONG	i;
	for (i=0; i<vpartition->segment_count; i++)
	{
		nwvp_segmentation_sectors_flush(vpartition->segments[i].lpart_link);
	}
	return(0);
}

ULONG	vpartition_update_entry_index(
	nwvp_lpart		*lpart,
	BYTE			*volume_name,
	ULONG			old_index,
	ULONG			new_index)
{
	ULONG	i, j;
	nwvp_vpartition		*vpartition;

	for (j=0; j<virtual_partition_table_count; j++)
	{
		if ((vpartition = virtual_partition_table[j]) != 0)
		{
			if (nwvp_memcomp(&volume_name[0], &vpartition->volume_name[0], 16) == 0)
			{
				for (i=0; i<vpartition->segment_count; i++)
				{
					if ((vpartition->segments[i].lpart_link == lpart) && (vpartition->segments[i].volume_entry_index == old_index))
					{
						vpartition->segments[i].volume_entry_index = new_index;
						return (0);
					}
				}
			}
		}
	}
	return(-1);
}

ULONG	vpartition_update_lpartition(
	nwvp_lpart		*lpart,
    nwvp_lpart      *new_lpart)
{
	ULONG	i, j;
	nwvp_vpartition		*vpartition;

	for (j=0; j<virtual_partition_table_count; j++)
	{
		if ((vpartition = virtual_partition_table[j]) != 0)
		{
			for (i=0; i<vpartition->segment_count; i++)
			{
				if (vpartition->segments[i].lpart_link == lpart)
                {
                    vpartition->segments[i].lpart_link = new_lpart;
                }
			}
		}
	}
	return(0);
}

ULONG	nwvp_vpartition_fat_fix(
	ULONG		vpart_handle,
    nwvp_fat_fix_info        *fix_info,
    ULONG       fix_flag)
{
	ULONG			ccode;
	nwvp_vpartition		*vpartition;

	NWLockNWVP();
	if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
	{
		if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
			(vpartition->vpartition_handle == vpart_handle))
		{
			NWUnlockNWVP();
	    ccode = nwvp_fat_table_fix(vpartition, fix_info, fix_flag);
			return(ccode);
		}
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	inwvp_vpartition_open(
	nwvp_vpartition		*vpartition)
{
	ULONG			i;

	if ((vpartition->open_flag == 0) && (vpartition->volume_valid_flag != 0xFFFFFFFF))
	{
		for (i=0; i<vpartition->segment_count; i++)
		{
			if (nwvp_mirror_group_open(vpartition->segments[i].lpart_link) != 0)
			{
				for (; i > 0; i--)
					nwvp_mirror_group_close(vpartition->segments[i-1].lpart_link);
				break;
			}
		}
		if (i == vpartition->segment_count)
		{
			vpartition->open_flag = 0xFFFFFFFF;
			return(0);
		}
	}
	return(-1);
}

ULONG	nwvp_vpartition_open(
	ULONG		vpart_handle)
{
	ULONG			ccode;
	nwvp_vpartition		*vpartition;

	NWLockNWVP();
	if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
	{
		if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
			(vpartition->vpartition_handle == vpart_handle))
		{
			ccode = inwvp_vpartition_open(vpartition);
			NWUnlockNWVP();
			return(ccode);
		}
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	inwvp_vpartition_close(
	nwvp_vpartition		*vpartition)
{
	ULONG			i;

	if (vpartition->open_flag != 0)
	{
		for (i=0; i<vpartition->segment_count; i++)
		{
			nwvp_mirror_group_close(vpartition->segments[i].lpart_link);
		}
		vpartition->open_flag = 0;
		return(0);
	}
	return(-1);
}
ULONG	nwvp_vpartition_close(
	ULONG		vpart_handle)
{
	ULONG		ccode;
	nwvp_vpartition		*vpartition;

	NWLockNWVP();
	if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
	{
		if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
			(vpartition->vpartition_handle == vpart_handle))
		{
			ccode = inwvp_vpartition_close(vpartition);
			NWUnlockNWVP();
			return(ccode);
		}
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	nwvp_vpartition_format(
	ULONG					*vpart_handle,
	vpartition_info			*vpart_info,
	segment_info			*segment_info_table)
{
	ULONG			i, j, k;
	ULONG			total_blocks = 0;
	nwvp_lpart		*lpart;
	segment_info	*sinfo;
	nwvp_vpartition	*vpartition;
	nwvp_lpart			*lpartition;


	NWLockNWVP();
	if (virtual_partition_table_count >= MAX_VOLUMES)
	{
		NWUnlockNWVP();
		return(-1);
	}

	sinfo = segment_info_table;
	for (i=0; i<vpart_info->segment_count; i++, sinfo ++)
	{
		if (((lpart = logical_partition_table[sinfo->lpartition_id & 0x0000FFFF]) != 0) &&
			 (lpart->lpartition_handle == sinfo->lpartition_id))
		{
			if ((sinfo->block_offset + sinfo->block_count) > lpart->logical_block_count)
			{
				NWUnlockNWVP();
				return(-1);
			}
			for (j=0; j<lpart->segment_sector->NumberOfTableEntries; j++)
			{

				if (nwvp_bounds_check(lpart->segment_sector->VolumeEntry[j].VolumeRoot, lpart->segment_sector->VolumeEntry[j].SegmentSectors, sinfo->block_offset * lpart->sectors_per_block, sinfo->block_count * lpart->sectors_per_block) != 0)
				{
					NWUnlockNWVP();
					return(-1);
				}
			}
			total_blocks += (sinfo->block_count / vpart_info->blocks_per_cluster) * vpart_info->blocks_per_cluster;
		}
		else
		{
			NWUnlockNWVP();
			return(-1);
		}
	}
	if (total_blocks != vpart_info->cluster_count * vpart_info->blocks_per_cluster)
	{
		NWUnlockNWVP();
		return(-1);
	}

	sinfo = segment_info_table;
	nwvp_alloc((void **) &vpartition, sizeof(nwvp_vpartition));
	nwvp_memset((void *) vpartition, 0, sizeof(nwvp_vpartition));
	for (i=0; i<virtual_partition_table_count; i++)
	{
		if (virtual_partition_table[i] == 0)
			break;

	}
	if (i == virtual_partition_table_count)
	{
		virtual_partition_table_count ++;
	}
	virtual_partition_table[i] = vpartition;
	vpartition->vpartition_handle = (handle_instance ++ << 16) + i;
	vpartition->segment_count = vpart_info->segment_count;
	vpartition->cluster_count = vpart_info->cluster_count;
	vpartition->blocks_per_cluster = vpart_info->blocks_per_cluster;


	total_blocks = 0;
	for (i=0; i<vpart_info->segment_count; i++, sinfo ++)
	{
		if ((lpart = logical_partition_table[sinfo->lpartition_id & 0x0000FFFF]) != 0)
		{
			for (j=0; j<lpart->segment_sector->NumberOfTableEntries; j++)
			{
				if (((sinfo->block_count + sinfo->block_offset) * lpart->sectors_per_block) <= lpart->segment_sector->VolumeEntry[j].VolumeRoot)
				{
					for (k=lpart->segment_sector->NumberOfTableEntries; k>j; k--)
					{
						nwvp_memmove(&lpart->segment_sector->VolumeEntry[k], &lpart->segment_sector->VolumeEntry[k-1], sizeof(VOLUME_TABLE_ENTRY));
						vpartition_update_entry_index(lpart, lpart->segment_sector->VolumeEntry[k].VolumeName, k-1, k);
					}
					lpart->segment_sector->NumberOfTableEntries ++;
					break;
				}
			}
			if (j == lpart->segment_sector->NumberOfTableEntries)
			{
				lpart->segment_sector->NumberOfTableEntries ++;
			}

			for (k=0; k < (ULONG)(vpart_info->volume_name[0] + 1); k++)
			{
				lpart->segment_sector->VolumeEntry[j].VolumeName[k] = vpart_info->volume_name[k];
				vpartition->volume_name[k] = vpart_info->volume_name[k];
			}
			for (; k<16; k++)
			{
				lpart->segment_sector->VolumeEntry[j].VolumeName[k] = 0;
				vpartition->volume_name[k] = 0;
			}

//			nwvp_memmove(&lpart->segment_sector->VolumeEntry[j].VolumeName[0], &vpart_info->volume_name[0], 16);
//			nwvp_memmove(&vpartition->volume_name[0], &vpart_info->volume_name[0], 16);

			lpart->segment_sector->VolumeEntry[j].LastVolumeSegment = vpart_info->segment_count - 1;
			lpart->segment_sector->VolumeEntry[j].VolumeSignature = (i << 16) + (vpart_info->segment_count << 8) + cluster_size_table[vpart_info->blocks_per_cluster];
			lpart->segment_sector->VolumeEntry[j].VolumeRoot = sinfo->block_offset * lpart->sectors_per_block;
			lpart->segment_sector->VolumeEntry[j].SegmentSectors = sinfo->block_count * lpart->sectors_per_block;
			lpart->segment_sector->VolumeEntry[j].VolumeClusters = vpart_info->cluster_count;
			lpart->segment_sector->VolumeEntry[j].SegmentClusterStart = total_blocks / vpart_info->blocks_per_cluster;
			lpart->segment_sector->VolumeEntry[j].FirstFAT = 0xFFFFFFFF;
			lpart->segment_sector->VolumeEntry[j].SecondFAT = 0xFFFFFFFF;
			lpart->segment_sector->VolumeEntry[j].FirstDirectory = 0xFFFFFFFF;
			lpart->segment_sector->VolumeEntry[j].SecondDirectory = 0xFFFFFFFF;
			lpart->segment_sector->VolumeEntry[j].Padding = 0;
			vpartition->segments[i].lpart_link = logical_partition_table[sinfo->lpartition_id & 0x0000FFFF];
			vpartition->segments[i].segment_block_count = (sinfo->block_count / vpart_info->blocks_per_cluster) * vpart_info->blocks_per_cluster;
			vpartition->segments[i].partition_block_offset = sinfo->block_offset;
			vpartition->segments[i].relative_cluster_offset = total_blocks / vpart_info->blocks_per_cluster;
			vpartition->segments[i].volume_entry_index = j;
			total_blocks += (sinfo->block_count / vpart_info->blocks_per_cluster) * vpart_info->blocks_per_cluster;
		}
	}
	vpartition->volume_valid_flag = 0;
	inwvp_vpartition_open(vpartition);
	nwvp_vpartition_build_fat(vpartition, 0);
	nwvp_vpartition_build_directory(vpartition, vpart_info->flags);
	for (i=0; i<vpartition->segment_count; i++)
	{
		lpartition = vpartition->segments[i].lpart_link;
		lpartition->segment_sector->VolumeEntry[vpartition->segments[i].volume_entry_index].FirstFAT = vpartition->FAT1;
		lpartition->segment_sector->VolumeEntry[vpartition->segments[i].volume_entry_index].SecondFAT = vpartition->FAT2;
		lpartition->segment_sector->VolumeEntry[vpartition->segments[i].volume_entry_index].FirstDirectory = vpartition->Directory1;
		lpartition->segment_sector->VolumeEntry[vpartition->segments[i].volume_entry_index].SecondDirectory = vpartition->Directory2;
	}
	nwvp_vpartition_segment_flush(vpartition);
	*vpart_handle = vpartition->vpartition_handle;
	inwvp_vpartition_close(vpartition);
	NWUnlockNWVP();
	return(0);
}

ULONG	nwvp_vpartition_add_segment(
	ULONG					vpart_handle,
	segment_info			*segment_info_table)
{
	ULONG				i, j, k;
	ULONG				total_blocks;
	ULONG				close_flag = 0;
	nwvp_lpart			*lpart;
	segment_info		*sinfo;
	nwvp_vpartition		*vpartition;
	nwvp_lpart			*lpartition;

	NWLockNWVP();
	if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
	{
		if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
			(vpartition->vpartition_handle == vpart_handle) &&
			(vpartition->volume_valid_flag == 0))
		{
			sinfo = segment_info_table;
			if (((lpart = logical_partition_table[sinfo->lpartition_id & 0x0000FFFF]) != 0) &&
				 (lpart->lpartition_handle == sinfo->lpartition_id))
			{
				for (j=0; j<lpart->segment_sector->NumberOfTableEntries; j++)
				{
					if (nwvp_bounds_check(lpart->segment_sector->VolumeEntry[j].VolumeRoot, lpart->segment_sector->VolumeEntry[j].SegmentSectors, sinfo->block_offset * lpart->sectors_per_block, sinfo->block_count * lpart->sectors_per_block) != 0)
					{
						NWUnlockNWVP();
						return(-1);
					}
				}
			}
			else
			{
				NWUnlockNWVP();
				return(-1);
			}
			if (((lpart = logical_partition_table[sinfo->lpartition_id & 0x0000FFFF]) != 0) &&
				(lpart->lpartition_handle == sinfo->lpartition_id))
			{
				for (j=0; j<lpart->segment_sector->NumberOfTableEntries; j++)
				{
					if (((sinfo->block_count + sinfo->block_offset) * lpart->sectors_per_block) <= lpart->segment_sector->VolumeEntry[j].VolumeRoot)
					{
						for (k=lpart->segment_sector->NumberOfTableEntries; k>j; k--)
						{
							nwvp_memmove(&lpart->segment_sector->VolumeEntry[k], &lpart->segment_sector->VolumeEntry[k-1], sizeof(VOLUME_TABLE_ENTRY));
							vpartition_update_entry_index(lpart, lpart->segment_sector->VolumeEntry[k].VolumeName, k-1, k);
						}
						lpart->segment_sector->NumberOfTableEntries ++;
						break;
					}
				}
				if (j == lpart->segment_sector->NumberOfTableEntries)
				{
					lpart->segment_sector->NumberOfTableEntries ++;
				}
				total_blocks = vpartition->cluster_count * vpartition->blocks_per_cluster;
				vpartition->cluster_count += sinfo->block_count / vpartition->blocks_per_cluster;
				i = vpartition->segment_count;
				vpartition->segment_count ++;
				nwvp_memmove(&lpart->segment_sector->VolumeEntry[j].VolumeName[0], &vpartition->volume_name[0], 16);
				lpart->segment_sector->VolumeEntry[j].VolumeRoot = sinfo->block_offset * lpart->sectors_per_block;
				lpart->segment_sector->VolumeEntry[j].SegmentSectors = sinfo->block_count * lpart->sectors_per_block;
				lpart->segment_sector->VolumeEntry[j].SegmentClusterStart = total_blocks / vpartition->blocks_per_cluster;
				lpart->segment_sector->VolumeEntry[j].FirstFAT = vpartition->FAT1;
				lpart->segment_sector->VolumeEntry[j].SecondFAT = vpartition->FAT2;
				lpart->segment_sector->VolumeEntry[j].FirstDirectory = vpartition->Directory1;
				lpart->segment_sector->VolumeEntry[j].SecondDirectory = vpartition->Directory2;
				lpart->segment_sector->VolumeEntry[j].Padding = 0;
				vpartition->segments[i].lpart_link = logical_partition_table[sinfo->lpartition_id & 0x0000FFFF];
				vpartition->segments[i].segment_block_count = (sinfo->block_count / vpartition->blocks_per_cluster) * vpartition->blocks_per_cluster;
				vpartition->segments[i].partition_block_offset = sinfo->block_offset;
				vpartition->segments[i].relative_cluster_offset = total_blocks / vpartition->blocks_per_cluster;
				vpartition->segments[i].volume_entry_index = j;
				for (i=0; i<vpartition->segment_count; i++)
				{
					vpartition->segments[i].lpart_link->segment_sector->VolumeEntry[vpartition->segments[i].volume_entry_index].LastVolumeSegment = vpartition->segment_count - 1;
					vpartition->segments[i].lpart_link->segment_sector->VolumeEntry[vpartition->segments[i].volume_entry_index].VolumeSignature = (i << 16) + (vpartition->segment_count << 8) + cluster_size_table[vpartition->blocks_per_cluster];
					vpartition->segments[i].lpart_link->segment_sector->VolumeEntry[vpartition->segments[i].volume_entry_index].VolumeClusters = vpartition->cluster_count;
					lpartition = vpartition->segments[i].lpart_link;
					lpartition->segment_sector->VolumeEntry[vpartition->segments[i].volume_entry_index].FirstFAT = vpartition->FAT1;
					lpartition->segment_sector->VolumeEntry[vpartition->segments[i].volume_entry_index].SecondFAT = vpartition->FAT2;
					lpartition->segment_sector->VolumeEntry[vpartition->segments[i].volume_entry_index].FirstDirectory = vpartition->Directory1;
					lpartition->segment_sector->VolumeEntry[vpartition->segments[i].volume_entry_index].SecondDirectory = vpartition->Directory2;
				}

				if (vpartition->open_flag == 0)
				{
					close_flag = 1;
					inwvp_vpartition_open(vpartition);
				}

				nwvp_vpartition_build_fat(vpartition, i-1);
				nwvp_vpartition_segment_flush(vpartition);

				if (close_flag != 0)
					inwvp_vpartition_close(vpartition);

				NWUnlockNWVP();
				return(0);
			}
		}
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	nwvp_vpartition_unformat(
	ULONG					vpart_handle)
{
	ULONG	i;
	nwvp_vpartition	*vpartition;

	NWLockNWVP();
	if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
	{
		if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
			(vpartition->vpartition_handle == vpart_handle))
		{
			for (i=0; i<vpartition->segment_count; i++)
			{
				if (vpartition->segments[i].lpart_link != 0)
					nwvp_segmentation_remove_specific(vpartition->segments[i].lpart_link, vpartition->segments[i].volume_entry_index);
			}
			nwvp_free(vpartition);
			virtual_partition_table[vpart_handle & 0x0000FFFF] = 0;
			NWUnlockNWVP();
			return(0);
		}
	}
	NWUnlockNWVP();
	return(-1);
}

void	nwvp_scan_routine(
	ULONG	scan_flag)
{
	ULONG		i, j;
	ULONG		free_slot;
	ULONG		handles[10];
	ULONG		partition_id;
	ULONG		group_id;
	BYTE		*temp_buffer;
	nwvp_payload			payload;
	nwvp_part_info			part_info;
	nwvp_lpart				*lpart;
	MIRROR					*mirror_sector;


	if (nwvp_init_flag == 0)
		nwvp_init();


	NWLockNWVP();
	nwvp_io_alloc((void **) &temp_buffer, 4096);

	for (i=0; i<raw_partition_table_count; i++)
	{
		if (raw_partition_table[i] != 0)
			raw_partition_table[i]->scan_flag = 0xFFFFFFFF;
	}

	payload.payload_object_count = 0;
	payload.payload_index = 0;
	payload.payload_object_size_buffer_size = (sizeof(ULONG) << 20) + sizeof(handles);
	payload.payload_buffer = (BYTE *) &handles[0];
	do
	{
		nwvp_part_scan(&payload);
		for (j=0; j< payload.payload_object_count; j++)
		{
			if (nwvp_part_scan_specific (handles[j], &part_info) == 0)
			{
				partition_id = 0xFFFFFFFF;
				group_id = 0xFFFFFFFF;
				if ((part_info.partition_type & 0xFF) == 0x65)
				{
					mirror_sector = (MIRROR *) &temp_buffer[512];
					for (i=0x20; i<=0x80; i+=0x20)
					{
						if (nwvp_part_read(handles[j], i, 1 << (12 - part_info.sector_size_shift), temp_buffer) == 0)
						{
							if (nwvp_hotfix_and_mirror_validate(temp_buffer, part_info.sector_count) == 0)
							{
								partition_id = mirror_sector->PartitionID;
								group_id = mirror_sector->MirrorGroupID;
								break;
							}
							else
							{
								nwvp_memset(temp_buffer, 0, 4096);
							}
						}
					}
				}

				free_slot = 0xFFFFFFFF;
				for (i=0; i<raw_partition_table_count; i++)
				{
				   if ((lpart = raw_partition_table[i]) != 0)
				   {
						if (lpart->rpart_handle != handles[j])
						{
							if ((partition_id != 0xFFFFFFFF) &&
								(lpart->mirror_partition_id == partition_id) &&
								(lpart->mirror_group_id == group_id))
							{
								if ((lpart->mirror_status & MIRROR_PRESENT_BIT) == 0)
								{
				    if ((part_info.partition_type & 0x80000000) != 0)
				       lpart->mirror_status |= MIRROR_READ_ONLY_BIT;
									lpart->partition_type = part_info.partition_type & 0xFF;
									lpart->rpart_handle = handles[j];
									lpart->physical_block_count = part_info.sector_count >> (12 - part_info.sector_size_shift);
									lpart->sectors_per_block = 1 << (12 - part_info.sector_size_shift);
									lpart->sector_size_shift = part_info.sector_size_shift;
									nwvp_hotfix_scan(lpart);
									lpart->mirror_status |= MIRROR_NEW_BIT;
									lpart->mirror_status |= MIRROR_PRESENT_BIT;
									nwvp_log(NWLOG_DEVICE_ONLINE, lpart->lpartition_handle, lpart->rpart_handle, 0);
									nwvp_mirror_group_establish(lpart->primary_link, 0);
								}
								lpart->scan_flag = 0;
								break;
							}
						}
						else
						{
							lpart->scan_flag = 0;
							break;
						}
					}
					else
					{
						free_slot = i;
					}
				}

				if (i == raw_partition_table_count)
				{
					if (free_slot == 0xFFFFFFFF)
					{
						free_slot = raw_partition_table_count;
						raw_partition_table_count ++;
					}

					nwvp_alloc((void *) &lpart, sizeof(nwvp_lpart));
					nwvp_memset(lpart, 0, sizeof(nwvp_lpart));
					nwvp_alloc((void *) &lpart->netware_mirror_sector, 512);
					nwvp_alloc((void *) &lpart->mirror_sector, 512);
					nwvp_memmove(lpart->netware_mirror_sector, &temp_buffer[512], 512);
					nwvp_memmove(lpart->mirror_sector, &temp_buffer[1024], 512);

		    if ((part_info.partition_type & 0x80000000) != 0)
			lpart->mirror_status |= MIRROR_READ_ONLY_BIT;

		    raw_partition_table[free_slot] = lpart;
					lpart->remirror_section = 0xFFFFFFFF;
					lpart->lpartition_handle = 0xFFFFFFFF;
					lpart->low_level_handle = part_info.low_level_handle;
					lpart->primary_link = lpart;
					lpart->mirror_link = lpart;
					lpart->mirror_partition_id = partition_id;
					lpart->mirror_group_id = group_id;
					lpart->rpartition_handle = ((handle_instance ++) << 16) + free_slot;
					lpart->partition_type = part_info.partition_type;
					lpart->rpart_handle = handles[j];
					lpart->physical_block_count = part_info.sector_count >> (12 - part_info.sector_size_shift);
					lpart->sectors_per_block = 1 << (12 - part_info.sector_size_shift);
					lpart->sector_size_shift = part_info.sector_size_shift;
//					if (nwvp_part_read(handles[j], lpart->physical_block_count * lpart->sectors_per_block, lpart->sectors_per_block, temp_buffer) != 0)
//						nwvp_part_write(handles[j], lpart->physical_block_count * lpart->sectors_per_block, lpart->sectors_per_block, temp_buffer);
					if ((partition_id != 0xFFFFFFFF) && (scan_flag == 0))
					{
						nwvp_hotfix_scan(lpart);
						lpart->mirror_status |= MIRROR_PRESENT_BIT;
						nwvp_log(NWLOG_DEVICE_ONLINE, lpart->lpartition_handle, lpart->rpart_handle, 0);
						nwvp_mirror_group_add(lpart);
						nwvp_mirror_group_establish(lpart->primary_link, 0);
					}
					else
					{
						lpart->mirror_status |= MIRROR_PRESENT_BIT;
						nwvp_log(NWLOG_DEVICE_ONLINE, lpart->lpartition_handle, lpart->rpart_handle, 0);
					}
				}
			}
		}
	} while ((payload.payload_index != 0) && (payload.payload_object_count != 0));

	for (i=0; i<raw_partition_table_count; i++)
	{
		if (raw_partition_table[i] != 0)
		{
			if (raw_partition_table[i]->scan_flag != 0)
			{
				raw_partition_table[i]->scan_flag = 0;
				raw_partition_table[i]->rpart_handle = 0xFFFFFFFF;
				if (raw_partition_table[i]->mirror_status & MIRROR_PRESENT_BIT)
					nwvp_log(NWLOG_DEVICE_OFFLINE, raw_partition_table[i]->lpartition_handle, raw_partition_table[i]->rpart_handle, 0);
				raw_partition_table[i]->mirror_status &= ~(MIRROR_PRESENT_BIT | MIRROR_REMIRRORING_BIT);
				if (raw_partition_table[i]->lpartition_handle != 0xFFFFFFFF)
				{
					if (nwvp_mirror_group_update(raw_partition_table[i]->primary_link) != 0)
					{
						nwvp_mirror_group_delete(raw_partition_table[i]);
					}
				}
				else
				{
					if (raw_partition_table[i]->mirror_sector != 0)
					{
						nwvp_free(raw_partition_table[i]->mirror_sector);
						raw_partition_table[i]->mirror_sector = 0;
					}
					if (raw_partition_table[i]->netware_mirror_sector != 0)
					{
						nwvp_free(raw_partition_table[i]->netware_mirror_sector);
						raw_partition_table[i]->netware_mirror_sector = 0;
					}
					nwvp_free(raw_partition_table[i]);
					raw_partition_table[i] = 0;
				}
			}
		}
	}
	nwvp_io_free(temp_buffer);
	NWUnlockNWVP();
}

void	nwvp_unscan_routine()
{
	ULONG	i;

	NWLockNWVP();
	for (i=0; i < virtual_partition_table_count; i++)
	{
		if (virtual_partition_table[i] != 0)
		{
			nwvp_free(virtual_partition_table[i]);
			virtual_partition_table[i] = 0;
		}
	}
	virtual_partition_table_count = 0;
#if !(LINUX_20 | LINUX_22 | LINUX_24)
	NWUnlockNWVP();
	nwvp_quiese_remirroring();
	while (nwvp_remirror_control_list_head != 0)
	{
		nwvp_remirror_poll();

	}
	NWLockNWVP();
#endif
	for (i=0; i<raw_partition_table_count; i++)
	{
		if (raw_partition_table[i] != 0)
		{
			if (raw_partition_table[i]->mirror_status & MIRROR_PRESENT_BIT)
				nwvp_log(NWLOG_DEVICE_OFFLINE, raw_partition_table[i]->lpartition_handle, raw_partition_table[i]->rpart_handle, 0);
			raw_partition_table[i]->mirror_status &= ~(MIRROR_PRESENT_BIT | MIRROR_REMIRRORING_BIT);
			if (raw_partition_table[i]->lpartition_handle != 0xFFFFFFFF)
			{
				if (nwvp_mirror_group_update(raw_partition_table[i]->primary_link) != 0)
				{
					nwvp_mirror_group_delete(raw_partition_table[i]);
				}
			}
			else
			{
				if (raw_partition_table[i]->mirror_sector != 0)
				{
					nwvp_free(raw_partition_table[i]->mirror_sector);
					raw_partition_table[i]->mirror_sector = 0;
				}
				if (raw_partition_table[i]->netware_mirror_sector != 0)
				{
					nwvp_free(raw_partition_table[i]->netware_mirror_sector);
					raw_partition_table[i]->netware_mirror_sector = 0;
				}
				nwvp_free(raw_partition_table[i]);
				raw_partition_table[i] = 0;
			}
		}
	}
	raw_partition_table_count = 0;

	for (i=0; i<logical_partition_table_count; i++)
	{
		logical_partition_table[i] = 0;
	}
	logical_partition_table_count = 0;
	NWUnlockNWVP();
	nwvp_uninit();

}


ULONG	nwvp_mirror_block_validate(
	ULONG			lpart_handle,
	ULONG			block_number,
	BYTE			*buffer1,
	BYTE			*buffer2,
	ULONG			*reset_flag)
{
	nwvp_lpart		*lpart;
	nwvp_lpart		*temp;
	ULONG	mirror_count;
	ULONG	index = 0;
	ULONG	buffer_valid = 0;

	NWLockNWVP();
	while (1)
	{
		if ((lpart_handle & 0x0000FFFF) >= logical_partition_table_count)
		{
			NWUnlockNWVP();
			return(-1);

		}
		if (((lpart = logical_partition_table[lpart_handle & 0x0000FFFF]) == 0) ||
			(lpart->lpartition_handle != lpart_handle))
		{
			NWUnlockNWVP();
			return(-1);
		}
		if (nwvp_mirror_group_remirror_check(lpart, block_number >> 10, reset_flag) != 0)
		{
			NWUnlockNWVP();
			return(-1);
		}
		if (*reset_flag != 0)
		{
			NWUnlockNWVP();
			return(0);
		}
		if (block_number >= lpart->logical_block_count)
		{
			NWUnlockNWVP();
			return(0);
		}
		mirror_count = lpart->mirror_count;
		temp = lpart;
		do
		{
			if (temp->mirror_sector_index == index)
				break;
			temp = temp->mirror_link;
		} while (temp != lpart);

		if (temp->mirror_sector_index != index)
		{
			NWUnlockNWVP();
			return(-1);
		}

		if ((temp->mirror_status & MIRROR_PRESENT_BIT) != 0)
		{
			if (buffer_valid == 0)
			{
				NWUnlockNWVP();
				if (inwvp_lpartition_specific_read(temp, block_number, 1, buffer1) != 0)
					return(-1);
				NWLockNWVP();
				buffer_valid = 1;
			}
			else
			{
				NWUnlockNWVP();
				if (inwvp_lpartition_specific_read(temp, block_number, 1, buffer2) != 0)
					return(-1);
				NWLockNWVP();
				if (nwvp_memcomp(buffer1, buffer2, 4096) != 0)
				{
					NWUnlockNWVP();
					return(-1);
				}
			}
		}
		index ++;
		if (index >= mirror_count)
			break;
	}
	NWUnlockNWVP();
	return(0);
}


ULONG	nwvp_mirror_block_remirror(
	ULONG			lpart_handle,
	ULONG			block_number,
	BYTE			*buffer1,
	BYTE			*buffer2,
	ULONG			*reset_flag)
{
	nwvp_lpart		*lpart;
	nwvp_lpart		*temp;
	ULONG	mirror_count;
	ULONG	index = 0;
	ULONG	read_index = 0;
	ULONG	buffer_valid = 0;

	*reset_flag = 0;
	NWLockNWVP();
	while (1)
	{
		if ((lpart_handle & 0x0000FFFF) >= logical_partition_table_count)
		{
#if (DEBUG_MIRROR_CONTROL)
                	NWFSPrint(" remirror block 1  %x/n", (int) block_number);
#endif
			NWUnlockNWVP();
			return(-1);

		}
		if (((lpart = logical_partition_table[lpart_handle & 0x0000FFFF]) == 0) ||
			(lpart->lpartition_handle != lpart_handle))
		{
#if (DEBUG_MIRROR_CONTROL)
                	NWFSPrint(" remirror block 2  %x/n", (int) block_number);
#endif
			NWUnlockNWVP();
			return(-1);
		}
		if (nwvp_mirror_group_remirror_check(lpart, block_number >> 10, reset_flag) != 0)
		{
#if (DEBUG_MIRROR_CONTROL)
                	NWFSPrint(" remirror block 3  %x/n", (int) block_number);
#endif
			NWUnlockNWVP();
			return(-1);
		}
		if (*reset_flag != 0)
		{
#if (DEBUG_MIRROR_CONTROL)
                	NWFSPrint(" remirror block 4  %x/n", (int) block_number);
#endif
			NWUnlockNWVP();
			return(0);
		}

		if ((lpart->mirror_status & MIRROR_GROUP_REMIRRORING_BIT) == 0)
		{
#if (DEBUG_MIRROR_CONTROL)
                	NWFSPrint(" remirror block 5  %x/n", (int) block_number);
#endif
			NWUnlockNWVP();
			return(-1);
		}

		mirror_count = lpart->mirror_count;
		if (buffer_valid == 0)
		{
			temp = lpart;
			do
			{
				if (temp->mirror_sector_index == index)
				{
					if (((temp->mirror_status & MIRROR_INSYNCH_BIT) != 0) &&
					    ((temp->mirror_status & MIRROR_PRESENT_BIT) != 0) &&
                                            (temp->remirror_section == 0))
					{
						NWUnlockNWVP();
						if (inwvp_lpartition_specific_read(temp, block_number, 1, buffer1) != 0)
                                                {
#if (DEBUG_MIRROR_CONTROL)
                	NWFSPrint(" remirror block 6  %x/n", (int) block_number);
#endif
							return(-1);
                        			}
						NWLockNWVP();
						buffer_valid = 0xFFFFFFFF;
						read_index = index;
						index = 0;
					}
					break;
				}
				temp = temp->mirror_link;
			} while (temp != lpart);

			if (buffer_valid == 0)
			{
				if (temp->mirror_sector_index != index)
					break;
				index ++;
				if (index >= mirror_count)
				{
#if (DEBUG_MIRROR_CONTROL)
                	NWFSPrint(" remirror block 7  %x/n", (int) block_number);
#endif
					NWUnlockNWVP();
					return(-1);
				}
			}
		}
		else
		{
			temp = lpart;
			do
			{
				if (temp->mirror_sector_index == index)
				{
					if (index != read_index)
					{
						if ((temp->mirror_status & MIRROR_PRESENT_BIT) != 0)
						{

							if (((temp->mirror_status & MIRROR_INSYNCH_BIT) == 0) ||
								(temp->remirror_section > (block_number >> 10)))
							{
								NWUnlockNWVP();
								if (inwvp_lpartition_specific_write(temp, block_number, 1, buffer1) != 0)
                                                                {
									return(-1);
	                       					}
								NWLockNWVP();
							}
						}
					}
					break;
				}
				temp = temp->mirror_link;
			} while (temp != lpart);

			if (temp->mirror_sector_index != index)
				break;
			index ++;
			if (index >= mirror_count)
				break;
		}
	}
	NWUnlockNWVP();
	return(0);
}


ULONG	nwvp_remirror_poll()
{
	ULONG		reset_flag = 0;
	nwvp_remirror_control	*control;
	nwvp_lpart	*lpart;
	nwvp_lpart	*temp;

	NWLockNWVP();
	if ((control = nwvp_remirror_control_list_head) != 0)
	{
		nwvp_remirror_control_list_head = control->next_link;

		if (control->abort_flag != 0)
		{
		        if (nwvp_remirror_control_list_head == control)
                        {
				if ((nwvp_remirror_control_list_head = control->next_link) == control)
				nwvp_remirror_control_list_head = 0;
                        }
			else
			{
				control->next_link->last_link = control->last_link;
				control->last_link->next_link = control->next_link;
			}
			if ((control->lpart_handle & 0x0000FFFF) < logical_partition_table_count)
			{
				if (((lpart = logical_partition_table[control->lpart_handle & 0x0000FFFF]) != 0) &&
					(lpart->lpartition_handle == control->lpart_handle))
				{
					temp = lpart;
					do
					{
						temp->mirror_status &= ~MIRROR_REMIRRORING_BIT;
						temp = temp->mirror_link;
					} while (temp != lpart);
					lpart->mirror_status &= ~MIRROR_GROUP_REMIRRORING_BIT;
					nwvp_log(NWLOG_REMIRROR_ABORT, lpart->lpartition_handle, control->section, 0);
					nwvp_mirror_group_close(lpart);
				}
			}
			NWUnlockNWVP();
#if (DEBUG_MIRROR_CONTROL)
			NWFSPrint("1 returning remirror control structure %08X\n", (int) control);
#endif
			nwvp_free(control);
			return(0);
		}

		NWUnlockNWVP();
		switch(control->state)
		{
			case	NWVP_VALIDATE_PHASE:
				while ((control->valid_bits[control->index/8] & (1 << (control->index % 8))) != 0)
				{
					control->index ++;
					if (control->index == 1024)
					{
#if (DEBUG_MIRROR_CONTROL)
						NWFSPrint("validate phase complete %08X\n", (int) control->section);
#endif
						break;
					}
				}
				if (control->index < 1024)
				{
					if (nwvp_mirror_block_validate(control->lpart_handle, control->index + (control->section << 10), remirror_buffer1, remirror_buffer2, &reset_flag) == 0)
					{
						if (reset_flag != 0)
						{
							nwvp_memset(&control->valid_bits[0], 0, 1024 / 8);
							control->index = 0;
							control->check_flag = 0;
							control->remirror_flag = 0;
							control->state = NWVP_VALIDATE_PHASE;
							control->section = control->original_section;
							break;
						}
						else
						{
							control->valid_bits[control->index/8] |= 1 << (control->index % 8);
						}
					}
					else
                                        {
						control->check_flag = 1;
                			}
					control->index ++;
				}
				if ((control->index & 0xFF) == 0xFF)
					nwvp_thread_delay(nwvp_remirror_delay);
				if (control->index == 1024)
				{
					control->state = (control->check_flag == 0) ? NWVP_UPDATE_PHASE : NWVP_REMIRROR_PHASE;
					control->index = 0;
					control->check_flag = 0;
				}
				break;
			case	NWVP_REMIRROR_PHASE:
				while ((control->valid_bits[control->index/8] & (1 << (control->index % 8))) != 0)
				{
					control->index ++;
					if (control->index == 1024)
                                        {
#if (DEBUG_MIRROR_CONTROL)
						NWFSPrint("remirror phase complete %08X\n", (int) control->section);
#endif
						break;
                                        }
				}
				if (control->index < 1024)
				{
					if (nwvp_mirror_block_remirror(control->lpart_handle, control->index + (control->section << 10), remirror_buffer1, remirror_buffer2, &reset_flag) == 0)
					{
						if (reset_flag != 0)
						{
#if (DEBUG_MIRROR_CONTROL)
						NWFSPrint("remirror reset called %08X\n", (int) control->section);
#endif

							nwvp_memset(&control->valid_bits[0], 0, 1024 / 8);
							control->index = 0;
							control->check_flag = 0;
							control->remirror_flag = 0;
							control->state = NWVP_VALIDATE_PHASE;
							control->section = control->original_section;
							break;
						}
					}
					else
					{
						control->abort_flag = 0xFFFFFFFF;
					}
					control->index ++;
				}
				if (control->index == 1024)
				{
					control->index = 0;
					control->check_flag = 0;
					control->state = NWVP_VALIDATE_PHASE;
				}
				break;
			case	NWVP_UPDATE_PHASE:
#if (DEBUG_MIRRORING)
				NWFSPrint("update phase active control-%08X control->section-%08X\n",
					  (unsigned)control, (unsigned)control->section);
#endif

				if (nwvp_mirror_group_remirror_update(control->lpart_handle, control->section, &reset_flag) != 0)
				{
					control->abort_flag = 0xFFFFFFFF;
					break;
				}
				if (reset_flag != 0)
				{
					nwvp_memset(&control->valid_bits[0], 0, 1024 / 8);
					control->index = 0;
					control->check_flag = 0;
					control->remirror_flag = 0;
					control->state = NWVP_VALIDATE_PHASE;
					control->section = control->original_section;
					break;
				}
				if (control->section == 0)
				{
					NWLockNWVP();
                                        if (control == nwvp_remirror_control_list_head)
                                        {
						if ((nwvp_remirror_control_list_head = control->next_link) == control)
							nwvp_remirror_control_list_head = 0;
                                        }
					else
					{

						control->next_link->last_link = control->last_link;
						control->last_link->next_link = control->next_link;
					}
					if ((control->lpart_handle & 0x0000FFFF) < logical_partition_table_count)
					{
						if (((lpart = logical_partition_table[control->lpart_handle & 0x0000FFFF]) != 0) &&
							(lpart->lpartition_handle == control->lpart_handle))
						{
							temp = lpart;
							do
							{
								temp->mirror_status &= ~MIRROR_REMIRRORING_BIT;
								temp = temp->mirror_link;
							} while (temp != lpart);
							lpart->mirror_status &= ~MIRROR_GROUP_REMIRRORING_BIT;
							lpart->mirror_status &= ~MIRROR_GROUP_CHECK_BIT;
							nwvp_log(NWLOG_REMIRROR_COMPLETE, lpart->lpartition_handle, control->section, 0);
							nwvp_mirror_group_close(lpart);
						}
					}
					NWUnlockNWVP();
#if (DEBUG_MIRROR_CONTROL)
					NWFSPrint("2 returning remirror control structure %08X\n", (int) control);
#endif
					nwvp_free(control);
					break;
				}
				nwvp_memset(&control->valid_bits[0], 0, 1024 / 8);
				control->index = 0;
				control->check_flag = 0;
				control->remirror_flag = 0;
				control->state = NWVP_VALIDATE_PHASE;
				control->section --;
				break;
		}
		return(0);
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	nwvp_mirror_validate_process_done = 0;

ULONG	nwvp_mirror_validate_process()
{
	NWLockREMIRROR();
	while (nwvp_mirror_validate_process_done == 0)
	{
		while (nwvp_remirror_poll() == 0);
		NWLockREMIRROR();
	}
	return(0);
}

ULONG	nwvp_map_volume_physical(
	ULONG	vpart_handle,
	ULONG SectorOffsetInVolume,
	ULONG SectorCount,
	void **PartitionPointer,
	ULONG *SectorOffsetInPartition,
	ULONG *SectorsReturned)
{
	ULONG	i, j;
	ULONG	bad_bits;
	ULONG	count;
	ULONG	relative_offset;
	ULONG	relative_block;
	ULONG	relative_count;
	ULONG	block_number;
	nwvp_lpart	*lpart;
	nwvp_lpart	*original_part;
	nwvp_vpartition	*vpartition;

	if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
	{
	    if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
		(vpartition->vpartition_handle == vpart_handle))
		{
		if (vpartition->open_flag != 0)
		{
		    relative_block = SectorOffsetInVolume / 8;
		    relative_offset = SectorOffsetInVolume % 8;
		    relative_count = (relative_offset + SectorCount + 7) / 8;
		    for (i=0; i<vpartition->segment_count; i++)
		    {
			if (relative_block < vpartition->segments[i].segment_block_count)
			{
				if ((vpartition->segments[i].segment_block_count - relative_block) < relative_count)
					relative_count = vpartition->segments[i].segment_block_count - relative_block;

				block_number = relative_block + vpartition->segments[i].partition_block_offset;
				original_part = vpartition->segments[i].lpart_link;
				count = (original_part->read_round_robin ++) & 0x3;
				for (j=0; j< count; j++)
				original_part = original_part->mirror_link;

				lpart = original_part;
				do
				{
				if (((lpart->mirror_status & MIRROR_PRESENT_BIT) != 0) &&
					((lpart->mirror_status & MIRROR_INSYNCH_BIT) != 0))
				{
					for (j=0; j<relative_count; j++)
					{
						if (lpart->hotfix.bit_table[(block_number + j) >> 15][((block_number + j) >> 5) & 0x3FF] & (1 << ((block_number + j) & 0x1F)))
							break;
					}
					if (j == relative_count)
					{
						*SectorOffsetInPartition = ((block_number + lpart->hotfix.block_count) * 8) + relative_offset;
						*SectorsReturned = (relative_count * 8) - relative_offset;
						*PartitionPointer = (void *) lpart->rpart_handle;
						return(0);
					}
				}
				lpart = lpart->mirror_link;
				} while (lpart != original_part);

//
//	there are one or more hotfixed blocks in this region
//

				do
				{
				if (((lpart->mirror_status & MIRROR_PRESENT_BIT) != 0) &&
					((lpart->mirror_status & MIRROR_INSYNCH_BIT) != 0))
				{
					for (j=0; j<relative_count; j++)
					{
					if (lpart->hotfix.bit_table[(block_number + j) >> 15][((block_number + j) >> 5) & 0x3FF] & (1 << ((block_number + j) & 0x1F)))
					{
						if (j == 0)
						{
						nwvp_hotfix_block_lookup(lpart, block_number, &relative_block, &bad_bits);
						*SectorOffsetInPartition = (relative_block * 8) + relative_offset;
						*SectorsReturned = 8 - relative_offset;
						*PartitionPointer = (void *) lpart->rpart_handle;
						return((bad_bits == 0) ? 0 : -1);
						}
						else
						{
						*SectorOffsetInPartition = ((block_number + lpart->hotfix.block_count) * 8) + relative_offset;
						*SectorsReturned = (j * 8) - relative_offset;
						*PartitionPointer = (void *) lpart->rpart_handle;
						return(0);
						}
					}
					}
				}
				lpart = lpart->mirror_link;
				} while (lpart != original_part);

//
//	the code should never reach this point
//
			}
			relative_block -= vpartition->segments[i].segment_block_count;
			}
		}
		}
	}
	return (-1);
}


ULONG	nwvp_get_physical_partitions(
	ULONG	vpart_handle,
	ULONG	*ppartition_table_count,
	ULONG	*ppartition_table)
{
	ULONG	i;
	nwvp_vpartition	*vpartition;

	NWLockNWVP();
	if ((vpart_handle & 0x0000FFFF) < virtual_partition_table_count)
	{
		if (((vpartition = virtual_partition_table[vpart_handle & 0x0000FFFF]) != 0) &&
			(vpartition->vpartition_handle == vpart_handle) &&
			(vpartition->volume_valid_flag == 0))
		{
		    for (i=0; i<vpartition->segment_count; i++)
		    {
			ppartition_table[i] = vpartition->segments[i].lpart_link->rpartition_handle;
		    }
		    *ppartition_table_count = i;
		    NWUnlockNWVP();
		    return(0);

		}
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	nwvp_convert_local_handle(
	ULONG	rpart_handle,
	ULONG	*NT_handle)
{
	nwvp_lpart	*lpartition;

	NWLockNWVP();
	if ((rpart_handle & 0x0000FFFF) < raw_partition_table_count)
	{
		if (((lpartition = raw_partition_table[rpart_handle & 0x0000FFFF]) != 0) &&
			(lpartition->rpartition_handle == rpart_handle))
		{
			*NT_handle = lpartition->low_level_handle;
			return(0);
		}
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	nwvp_hotfix_table_check(
	ULONG	rpart_handle,
	ULONG	index,
	ULONG	*value)
{
	ULONG	*table;
	nwvp_lpart	*lpart;

	NWLockNWVP();
	if ((rpart_handle & 0x0000FFFF) < raw_partition_table_count)
	{
		if (((lpart = raw_partition_table[rpart_handle & 0x0000FFFF]) != 0) &&
			(lpart->rpartition_handle == rpart_handle) &&
			(index < lpart->hotfix.block_count))
		{
			if ((table = lpart->hotfix.hotfix_table[index >> 10]) != 0)
			{
				*value = table[index & 0x3FF];
				NWUnlockNWVP();
				return(0);
			}
		}
	}
	NWUnlockNWVP();
	return(-1);
}

ULONG	nwvp_nt_partition_shutdown(
	ULONG	low_level_handle)
{
	ULONG	i;

	NWLockNWVP();
	for (i=0; i<raw_partition_table_count; i++)
	{
		if ((raw_partition_table[i] != 0) &&
		    (raw_partition_table[i]->low_level_handle == low_level_handle))
		{
			raw_partition_table[i]->rpart_handle = 0xFFFFFFFF;
			if (raw_partition_table[i]->mirror_status & MIRROR_PRESENT_BIT)
				nwvp_log(NWLOG_DEVICE_OFFLINE, raw_partition_table[i]->lpartition_handle, raw_partition_table[i]->rpart_handle, 0);
			raw_partition_table[i]->mirror_status &= ~(MIRROR_PRESENT_BIT | MIRROR_REMIRRORING_BIT);
			if (raw_partition_table[i]->lpartition_handle != 0xFFFFFFFF)
			{
				if (nwvp_mirror_group_update(raw_partition_table[i]->primary_link) != 0)
				{
					nwvp_mirror_group_delete(raw_partition_table[i]);
				}
				NWUnlockNWVP();
				return(0);
			}
		}
	}
	NWUnlockNWVP();
	return(-1);
}
