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
***************************************************************************
*
*   AUTHOR   :   an Jeff V. Merkey
*   FILE     :  NWVPHAL.C
*   DESCRIP  :  NetWare Virtual Partition
*   DATE     :  August 1, 2022
*
***************************************************************************/

#include "globals.h"
#include "nwstruct.h"
#include "nwfs.h"
extern	BYTE	*zero_block_buffer;


typedef	struct	nwvp_rrpart_def
{
	ULONG	vpart_handle;
	ULONG	rrpart_table_index;
	ULONG	scan_field;
	ULONG	capacity;
	ULONG	partition_offset;
	ULONG	sector_size_shift;
	ULONG	partition_type;
	ULONG	online_flag;
	ULONG    error_flag;
	ULONG	low_level_handle;
	ULONG	scan_flag;
} nwvp_rrpart;

ULONG		raw_instance = 0x10000000;
ULONG		base_partition_table_count = 0;
nwvp_rrpart	base_partition_table[MAX_PARTITIONS];

ULONG	nwvp_part_scan(
	nwvp_payload	*req_payload)
{
	ULONG	i, j, k;
	ULONG	*handle_ptr;
	ULONG	index;
	ULONG	object_count;
	ULONG	*hotfix_sector;
	ULONG	new_partition_flag;

	nwvp_io_alloc((void **) &hotfix_sector, 4096);

	ScanDiskDevices();
	for (k=0; k<base_partition_table_count; k++)
	{
		if (base_partition_table[k].vpart_handle != 0)
		{
			base_partition_table[k].scan_flag = 0xFFFFFFFF;
		}
	}

	for (j=0; j < MAX_DISKS; j++)
	{
	   if (SystemDisk[j] != 0)
	   {
		for (i=0; i < 4; i++)
		{
			if (SystemDisk[j]->PartitionTable[i].nSectorsTotal)
			{
				for (k=0; k<base_partition_table_count; k++)
				{
					if ((base_partition_table[k].vpart_handle & 0x0FFFFFFF) == ((j << 16) + (i << 12) + k))
					{
						new_partition_flag = 0;
						if (SystemDisk[j]->PartitionTable[i].SysFlag == 0x65)
						{
							ReadDiskSectors(j, SystemDisk[j]->PartitionTable[i].StartLBA + 0x20, (BYTE *) &hotfix_sector[0], 8, 8);
							if (hotfix_sector[0] != 0x46544f48)
							{
								new_partition_flag = 1;

							}
						}

						if ((base_partition_table[k].capacity != SystemDisk[j]->PartitionTable[i].nSectorsTotal) ||
						    (base_partition_table[k].partition_offset != SystemDisk[j]->PartitionTable[i].StartLBA) ||
						    (new_partition_flag != 0))
						{
							base_partition_table[k].vpart_handle += 0x10000000;
							base_partition_table[k].rrpart_table_index = k;
							base_partition_table[k].partition_offset = SystemDisk[j]->PartitionTable[i].StartLBA;
							base_partition_table[k].capacity = SystemDisk[j]->PartitionTable[i].nSectorsTotal;
							base_partition_table[k].sector_size_shift = 9;
							base_partition_table[k].partition_type = SystemDisk[j]->PartitionTable[i].SysFlag;
//							base_partition_table[k].low_level_handle = (ULONG) SystemDisk[j]->PhysicalDiskHandle;
							base_partition_table[k].low_level_handle = (ULONG) SystemDisk[j]->PartitionContext[i];
							WriteDiskSectors(j, SystemDisk[j]->PartitionTable[i].StartLBA + 0x20, zero_block_buffer, 8, 8);
							WriteDiskSectors(j, SystemDisk[j]->PartitionTable[i].StartLBA + 0x40, zero_block_buffer, 8, 8);
							WriteDiskSectors(j, SystemDisk[j]->PartitionTable[i].StartLBA + 0x60, zero_block_buffer, 8, 8);
							WriteDiskSectors(j, SystemDisk[j]->PartitionTable[i].StartLBA + 0x80, zero_block_buffer, 8, 8);
						}
						base_partition_table[k].scan_flag = 0;
						break;
					}
				}
				if ((k == base_partition_table_count) && (base_partition_table_count < MAX_PARTITIONS))
				{
					raw_instance += 0x10000000;
					base_partition_table[k].vpart_handle = (raw_instance + (j << 16) + (i << 12) + k) & 0x7FFFFFFF;
					base_partition_table[k].scan_flag = 0;
					base_partition_table[k].rrpart_table_index = k;
					base_partition_table[k].partition_offset = SystemDisk[j]->PartitionTable[i].StartLBA;
					base_partition_table[k].capacity = SystemDisk[j]->PartitionTable[i].nSectorsTotal;
					base_partition_table[k].sector_size_shift = 9;
					base_partition_table[k].partition_type = SystemDisk[j]->PartitionTable[i].SysFlag;
					base_partition_table[k].low_level_handle = (ULONG) SystemDisk[j]->PartitionContext[i];
					base_partition_table_count ++;
				}
			}
		}
	   }
	}
	nwvp_io_free(hotfix_sector);

	for (k=0; k<base_partition_table_count; k++)
	{
		if (base_partition_table[k].vpart_handle != 0)
		{
			if (base_partition_table[k].scan_flag != 0)
			{
				base_partition_table[k].vpart_handle = 0;
			}
		}
	}

	object_count = (req_payload->payload_object_size_buffer_size & 0x000FFFFF) / 4;
	handle_ptr= (ULONG *) req_payload->payload_buffer;
	index = req_payload->payload_index;
	req_payload->payload_object_count = 0;
	for (; index<base_partition_table_count; index++)
	{
		if (base_partition_table[index].vpart_handle != 0)
		{
			*handle_ptr = base_partition_table[index].vpart_handle;
			handle_ptr ++;
			req_payload->payload_object_count ++;

			if (req_payload->payload_object_count >= object_count)
			{
				for (; index<base_partition_table_count; index++)
				{
					if (base_partition_table[index].vpart_handle != 0)
					{
						req_payload->payload_index = index;
						return(0);
					}
					req_payload->payload_index = 0;
					return(0);
				}
			}
		}
	}
	req_payload->payload_index = 0;
	return(0);
}

ULONG	nwvp_part_scan_specific(
	ULONG	handle,
	nwvp_part_info	*part_info)
{
	if ((base_partition_table[handle & 0xFFF].vpart_handle != 0) &&
	    (base_partition_table[handle & 0xFFF].vpart_handle == handle))
	{
		part_info->partition_type = base_partition_table[handle & 0xFFF].partition_type;
		part_info->sector_size_shift = 9;
		part_info->sector_count = base_partition_table[handle & 0xFFF].capacity;
		part_info->low_level_handle = base_partition_table[handle & 0xFFF].low_level_handle;
		return(0);
	}
	return(-1);
}

ULONG	nwvp_part_map_to_physical(
	ULONG	rpart_handle,
	ULONG	*disk_id,
	ULONG	*disk_offset,
	ULONG	*partition_number)
{
	nwvp_rrpart	*rrpart;

	if (((rrpart = &base_partition_table[rpart_handle & 0xFFF]) != 0) &&
	    (rrpart->vpart_handle == rpart_handle))
	{
		if (disk_id != 0)
			*disk_id = (rrpart->vpart_handle & 0x0FFF0000) >> 16;
		if (disk_offset != 0)
			*disk_offset = rrpart->partition_offset;
		if (partition_number != 0)
			*partition_number = (rrpart->vpart_handle & 0x0000F000) >> 12;
		return(0);
	}
	return(-1);
}

ULONG	nwvp_part_read(
	ULONG	handle,
	ULONG	sector_offset,
	ULONG	sector_count,
	BYTE	*data_buffer)
{
	nwvp_rrpart	*rrpart;

	if (((rrpart = &base_partition_table[handle & 0xFFF]) != 0) &&
	    (rrpart->vpart_handle == handle))
	{
//		if (rrpart->online_flag != 0)
//		{
//			if ((rrpart->error_flag != 0xFFFFFFFF) && (rrpart->error_flag != sector_offset))
//			{
				if(ReadDiskSectors(
					(rrpart->vpart_handle & 0x0FFF0000) >> 16,
					rrpart->partition_offset + sector_offset,
					data_buffer,
					sector_count,
					sector_count) != 0)
				return(0);
//			}
//		}
	}
	return(-1);
}

ULONG	nwvp_part_write(
	ULONG	handle,
	ULONG	sector_offset,
	ULONG	sector_count,
	BYTE	*data_buffer)
{
	nwvp_rrpart	*rrpart;

//	NWFSPrint("nwvp_part_write called with %d %x %x %x \n", handle, sector_offset, sector_count, (ULONG) data_buffer);
	if (((rrpart = &base_partition_table[handle & 0xFFF]) != 0) &&
	    (rrpart->vpart_handle == handle))
	{
//		if (rrpart->online_flag != 0)
//		{
//			if ((rrpart->error_flag != 0xFFFFFFFF) && (rrpart->error_flag != sector_offset))
//			{
				if(WriteDiskSectors(
					(rrpart->vpart_handle & 0x0FFF0000) >> 16,
					rrpart->partition_offset + sector_offset,
					data_buffer,
					sector_count,
					sector_count) != 0)
				return(0);
//			}
//		}
	}
	return(-1);
}

void	mem_check_init(void)
{
	return;
}

void	mem_check_uninit(void)
{
	return;
}

ULONG	nwvp_alloc(
	void	   **mem_ptr,
	ULONG		size)
{
	if ((*mem_ptr = NWFSAlloc(size, NWVP_TAG)) != 0)
		return(0);
	return(-1);
}

ULONG	nwvp_io_alloc(
	void	   **mem_ptr,
	ULONG		size)
{
#if (LINUX_20 || LINUX_22 || LINUX_24)
	*mem_ptr = kmalloc(size, GFP_KERNEL);
        if (*mem_ptr)
	   return(0);
#else
	*mem_ptr = malloc(size);
        if (*mem_ptr)
           return(0);
#endif

	return(-1);
}

ULONG  nwvp_io_free(void *memptr)
{
#if (LINUX_20 || LINUX_22 || LINUX_24)
	kfree(memptr);
	return(0);
#else
	free(memptr);
	return(0);
#endif
}


ULONG	nwvp_free(
	void	   *mem_ptr)
{
	NWFSFree(mem_ptr);
	return(0);
}


ULONG	nwvp_thread_get_id()
{
//	return(get_running_process());
	return(0);
}

void	nwvp_thread_yield()
{
//	return(get_running_process());
	return;
}

void	nwvp_thread_delay(
	ULONG	amount)
{
//	delayThread(amount);
}

void	nwvp_thread_sleep()
{
//	sleepThread();
};

void	nwvp_thread_wakeup(
	ULONG	thread_id)
{
//	rescheduleThread(thread_id);
}

void	nwvp_thread_kill(
	ULONG	thread_id)
{
//	killThread(thread_id);
}

ULONG	nwvp_thread_create(
	BYTE		*name,
	ULONG		(*start_routine)(ULONG),
	ULONG		start_parameter)
{
//	return(createThread(name, start_routine, 4096, (void *) start_parameter, -1));
	return(0);
}

ULONG    nwvp_memset(void *p, int c, long size)
{
	NWFSSet(p, c, size);
   return(0);
}

ULONG    nwvp_memcpy(void *d, void *s, long size)
{
	NWFSCopy(d, s, size);
   return(0);
}

ULONG    nwvp_memcomp(void *d, void *s, long size)
{
	return(NWFSCompare(d, s, size));
}


ULONG    nwvp_memmove(void *d, void *s, long size)
{
	NWFSCopy(d, s, size);
	return (0);
}

void	nwvp_spin_lock(
	ULONG	*spin_lock)
{
	*spin_lock = 1;
}

void	nwvp_spin_unlock(
	ULONG	*spin_lock)
{
	*spin_lock = 0;
}

ULONG	nwvp_ascii_to_int(
	ULONG	*value,
	BYTE	*string)
{
	BYTE	*bptr;
	int		base = 10;
	*value = 0;

	bptr = string;
	if ((bptr[0] == '0') && ((bptr[1] == 'x') || (bptr[1] == 'X')))
	{
		base = 16;
		bptr += 2;
	}
	while (*bptr != 0)
	{
		if ((bptr[0] >= '0') && (bptr[0] <= '9'))
		{
			*value *= base;
			*value += bptr[0] - '0';
		}
		else
		if (base == 16)
		{
			if ((bptr[0] >= 'a') && (bptr[0] <= 'f'))
			{
				*value *= base;
				*value += bptr[0] - 'a' + 10;
			}
			else
			if ((bptr[0] >= 'A') && (bptr[0] <= 'F'))
			{
				*value *= base;
				*value += bptr[0] - 'A' + 10;
			}
			else
				return(-1);
		}
		else
			return(-1);
		bptr ++;
	}
	return(0);
}

ULONG	nwvp_int_to_ascii(
	ULONG	value,
	ULONG	base,
	BYTE	*string)
{
	BYTE	temp_string[16];
	BYTE	*sptr;
	ULONG	digit;
	ULONG	index = 0;
	ULONG	temp_value = value;

	string[0] = '0';
	string[1] = 0;
	sptr = string;
	while (temp_value != 0)
	{
		digit  = temp_value % base;
		if (digit < 10)
			temp_string[index] = (BYTE)(digit + '0');
		else
			temp_string[index] = (BYTE)(digit + 'A') - 10;
		temp_value /= base;
		index ++;
	}
	if (base == 16)
	{
		for (digit = 0; digit < (8 - index); digit ++)
		{
			sptr[0] = '0';
			sptr ++;
		}
	}
	while (index)
	{
		index --;
		sptr[0] = temp_string[index];
		sptr ++;
		sptr[0] = 0;
	}
	return(0);
}

#if (LINUX_SLEEP)
NWFSInitMutex(NWVPSemaphore);
#endif

ULONG double_check = 0;

void NWLockNWVP(void)
{
#if (LINUX_SLEEP)
    if (WaitOnSemaphore(&NWVPSemaphore) == -EINTR)
       NWFSPrint("lock nwvp was interrupted\n");
#endif
}

void NWUnlockNWVP(void)
{
#if (LINUX_SLEEP)
    SignalSemaphore(&NWVPSemaphore);
#endif
}

#if (LINUX_SLEEP)
NWFSInitMutex(HOTFIXSemaphore);
#endif

void NWLockHOTFIX(void)
{
#if (LINUX_SLEEP)
    if (WaitOnSemaphore(&HOTFIXSemaphore) == -EINTR)
       NWFSPrint("lock hotfix was interrupted\n");
#endif
}

void NWUnlockHOTFIX(void)
{
#if (LINUX_SLEEP)
    SignalSemaphore(&HOTFIXSemaphore);
#endif
}

#if (LINUX_SLEEP)
NWFSInitSemaphore(REMIRRORSemaphore);
#endif

void NWLockREMIRROR(void)
{
#if (LINUX_SLEEP)
    if (WaitOnSemaphore(&REMIRRORSemaphore) == -EINTR)
       NWFSPrint("lock remirror was interrupted\n");
#endif
}

void NWUnlockREMIRROR(void)
{
#if (LINUX_SLEEP)
    SignalSemaphore(&REMIRRORSemaphore);
#endif
}

ULONG	nwvp_get_date_and_time(void)
{
    return (NWFSSystemToNetwareTime(NWFSGetSystemTime()));
}
