/***************************************************************************
*
*   Copyright (c) 1997, 1997-2022 Jeff V. Merkey  All Rights
*                           Reserved.
*
*
*   AUTHOR   :  
*   FILE     :  NWVPHAL.H
*   DESCRIP  :  NetWare Virtual Partition
*   DATE     :  August 1, 2022
*
***************************************************************************/

#ifndef _NWVPHAL_
#define _NWVPHAL_

#define	vndi_console	nwvp_console
#define	vndi_payload	nwvp_payload

typedef	struct	nwvp_payload_def
{
	ULONG		payload_index;
	ULONG		payload_object_count; 				// return value
	ULONG		payload_object_size_buffer_size;
	BYTE		*payload_buffer;
} nwvp_payload;

typedef	struct	nwvp_part_info_def
{
	ULONG	partition_type;
	ULONG	sector_size_shift;
	ULONG	sector_count;
	ULONG	low_level_handle;
} nwvp_part_info;

typedef	struct	nwvp_rpart_def
{
	ULONG	vndi_type;
	ULONG	vndi_handle;
	ULONG	rpart_table_index;
	ULONG	scan_field;

	BYTE	filename[32];
	
	ULONG	capacity;
	ULONG	sector_size_shift;
	ULONG	online_flag;
	ULONG    error_flag;
	
	ULONG	f_handle;
	ULONG	errors[32];
	ULONG	error_param[32];
	ULONG	index;
} nwvp_part;

typedef struct nwvp_entry_def
{
	ULONG	vndi_type;
	ULONG	index;
	BYTE	filename[32];
} nwvp_entry;

#define		RPART_ERROR_IO				1
#define		RPART_ONLINE				1
#define		RPART_OFFLINE				0	
#define		NWVP_READWRITE_BUFFER_SIZE	16384

void	nwvp_part_unscan(void);

ULONG	nwvp_part_scan(
	nwvp_payload	*req_payload);

ULONG	nwvp_part_scan_specific(
	ULONG	handle,
	nwvp_part_info	*part_info);

ULONG	nwvp_part_read(
	ULONG	handle,
	ULONG	sector_offset,
	ULONG	sector_count,
	BYTE	*data_buffer);

ULONG	nwvp_raw_read (
	nwvp_part				*rpart,
	ULONG					sector_offset,
	ULONG					sector_count,
	BYTE					*data_buffer);

ULONG	nwvp_part_write(
	ULONG	handle,
	ULONG	sector_offset,
	ULONG	sector_count,
	BYTE	*data_buffer);

ULONG	nwvp_raw_write (
	nwvp_part				*rpart,
	ULONG					sector_offset,
	ULONG					sector_count,
	BYTE					*data_buffer);

ULONG	dbase_write(void);

ULONG	rpart_open(
	BYTE					*filename,
	ULONG					index);


ULONG	nwvp_get_date_and_time(void);

ULONG	nwvp_thread_get_id(void);

void	nwvp_thread_delay(
	ULONG	amount);

void	nwvp_thread_sleep(void);

void	nwvp_thread_wakeup(
	ULONG	thread_id);

void	nwvp_thread_kill(
	ULONG	thread_id);

void	nwvp_thread_yield(void);

ULONG	nwvp_thread_create(
	BYTE		*name,
	ULONG		(*start_routine)(ULONG),
	ULONG		start_parameter);

ULONG    nwvp_memset(void *p, int c, long size);

ULONG    nwvp_memcpy(void *d, void *s, long size);

ULONG    nwvp_memcomp(void *d, void *s, long size);

ULONG    nwvp_memmove(void *d, void *s, long size);

ULONG	nwvp_init_partitions (void);

ULONG	nwvp_uninit_partitions (void);

ULONG	nwvp_create_file_partition (
	ULONG					sector_size_shift,
	ULONG					capacity);

ULONG	nwvp_delete_file_partition (
	ULONG					rpart_handle);

ULONG	partition_online (
	ULONG					part_handle);

ULONG	partition_offline (
	ULONG					part_handle);

//void    raw_offline_routine(
//	vndi_console    		*console);

ULONG	nwvp_part_scan_all(
	vndi_payload			*req_payload);
	
ULONG	inject_error (
	ULONG					rpart_handle,
	ULONG					error,
	ULONG					param);

ULONG	remove_error (
	ULONG					rpart_handle,
	ULONG					error,
	ULONG					param);

ULONG	nwvp_ascii_to_int(
	ULONG	*value,
	BYTE	*string);

ULONG	nwvp_int_to_ascii(
	ULONG	value,
	ULONG	base,
	BYTE	*string);

#endif

void NWLockNWVP(void);
void NWUnlockNWVP(void);
void NWLockHOTFIX(void);
void NWUnlockHOTFIX(void);
void NWLockREMIRROR(void);
void NWUnlockREMIRROR(void);
