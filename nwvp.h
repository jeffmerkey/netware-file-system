
/***************************************************************************
*
*   Copyright (c) 1998, 2022 Jeff V. Merkey
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
*      
*      
*
***************************************************************************
*
*   AUTHOR   :   and Jeff V. Merkey
*   FILE     :  NWVP.H
*   DESCRIP  :  NetWare Virtual Partition
*   DATE     :  August 1, 2022
*
***************************************************************************/

extern	BYTE	*bit_bucket_buffer;
#define	NWVP_ERROR_DATA_VALID		        0
#define	NWVP_ERROR_DATA_INVALID		        1
#define	NWVP_ERROR_NOT_PRESENT                  -2
#define	NWVP_ERROR_INVALID_INDEX	        -3
#define NWLOG_REMIRROR_START			0x54525453
#define NWLOG_REMIRROR_START_ERROR		0x45525453
#define NWLOG_REMIRROR_STOP			0x504F5453
#define NWLOG_REMIRROR_STOP_ERROR		0x45505453
#define NWLOG_REMIRROR_RESET1			0x31534552
#define NWLOG_REMIRROR_RESET2			0x32534552
#define NWLOG_REMIRROR_RESET3			0x33534552
#define NWLOG_REMIRROR_ABORT			0x54524241
#define NWLOG_REMIRROR_COMPLETE			0x504D4F43
#define	NWLOG_DEVICE_OFFLINE			0x4C46464F
#define	NWLOG_DEVICE_ONLINE			0x4E4C4E4F
#define	NWLOG_DEVICE_INSYNCH			0x434C5953
#define	NWLOG_DEVICE_OUT_OF_SYNCH		0x4C59534F

#define FFIX_NULL_NULL                  0x0000
#define FFIX_NULL_EOF                   0x0001
#define FFIX_NULL_SA                    0x0010
#define FFIX_NULL_FAT                   0x0011
#define FFIX_EOF_NULL                   0x0100
#define FFIX_EOF_EOF                    0x0101
#define FFIX_EOF_SA                     0x0110
#define FFIX_EOF_FAT                    0x0111
#define FFIX_SA_NULL                    0x1000
#define FFIX_SA_EOF                     0x1001
#define FFIX_SA_SA                      0x1010
#define FFIX_SA_FAT                     0x1011
#define FFIX_FAT_NULL                   0x1100
#define FFIX_FAT_EOF                    0x1101
#define FFIX_FAT_SA                     0x1110
#define FFIX_FAT_FAT                    0x1111


#if MANOS

#define MAX_VOLUMES         256
#define MAX_VOLUME_ENTRIES    8
#define MAX_SEGMENTS         32
#define MAX_MIRRORS           8
#define MAX_NAMESPACES       16
#define MAX_SUBALLOC_NODES    5
#define MAX_SUBALLOC_SPAN     2
#define MAX_HOTFIX_BLOCKS    30

typedef struct _VOLUME_TABLE_ENTRY
{
   BYTE VolumeName[16];
   ULONG LastVolumeSegment;
   ULONG VolumeSignature;
   ULONG VolumeRoot;
   ULONG SegmentSectors;
   ULONG VolumeClusters;
   ULONG SegmentClusterStart;
   ULONG FirstFAT;
   ULONG SecondFAT;
   ULONG FirstDirectory;
   ULONG SecondDirectory;
   ULONG Padding;
} VOLUME_TABLE_ENTRY;

typedef struct _VOLUME_TABLE
{
   BYTE VolumeTableSignature[16];
   ULONG NumberOfTableEntries;
   ULONG Reserved[3];
   VOLUME_TABLE_ENTRY VolumeEntry[MAX_VOLUME_ENTRIES];
} VOLUME_TABLE;

typedef struct _HOT_FIX_BLOCK_TABLE
{
   ULONG HotFix1;
   ULONG BadBlock1;
   ULONG HotFix2;
   ULONG BadBlock2;
} HOTFIX_BLOCK_TABLE;

typedef struct _HOTFIX {
   BYTE HotFixStamp[8];
   ULONG PartitionID;
   ULONG HotFixFlags;
   ULONG HotFixDateStamp;
   ULONG HotFixTotalSectors;
   ULONG HotFixSize;
   HOTFIX_BLOCK_TABLE HotFixTable[MAX_HOTFIX_BLOCKS];
} HOTFIX;

typedef struct _MIRROR
{
   BYTE MirrorStamp[8];
   ULONG PartitionID;
   ULONG MirrorFlags;
   ULONG Reserved1;
   ULONG MirrorStatus;
   ULONG MirrorTotalSectors;
   ULONG MirrorGroupID;
   ULONG MirrorMemberID[MAX_MIRRORS];
} MIRROR;

typedef struct _FAT_ENTRY
{
   long FATIndex;
   long FATCluster;
} FAT_ENTRY;

#endif


extern	void	mem_check_init(void);
extern	void	mem_check_uninit(void);

#define	NWVP_HOTFIX_READ			0x11111111
#define	NWVP_HOTFIX_WRITE			0x22222222
#define	NWVP_HOTFIX_FORCE			0x33333333
#define	NWVP_HOTFIX_FORCE_BAD		0x44444444


#define	NWVP_MIRROR_STATUS_OPENED			0x4E45504F
#define	NWVP_MIRROR_STATUS_CLOSED			0x534F4C43
#define	NWVP_MIRROR_STATUS_CHECK			0x4B454843
#define	NWVP_MIRROR_STATUS_REMOVED			0x4F4D4552
#define	NWVP_MIRROR_STATUS_BAD				0x4C444142

//#define	NWVPMIRR_GROUP_OPERATIONAL		0x00010000
//#define	NWVPMIRR_GROUP_COMPLETE			0x00020000

#define	NETWARE_IN_SYNCH_INSTANCE		        0x01860000
#define	NETWARE_OUT_OF_SYNCH_INSTANCE	                0x01850000
#define	NETWARE_INUSE_BIT				0x00000100
#define	NETWARE_OUT_OF_SYNCH_BIT		        0x00000002
#define	NETWARE_5_BASE_BIT				0x00010000

#define		RPART_ERROR_IO				1
#define		RPART_ONLINE				1
#define		RPART_OFFLINE				0	
#define		NWVP_READWRITE_BUFFER_SIZE	16384


#define		INUSE_BIT	0x80000000
#define		MIRROR_BIT	0x00080000

typedef	struct	_nwvp_asynch_map
{
	ULONG	sector_offset;
	ULONG	disk_id;
	ULONG	bad_bits;
	ULONG	extra;
} nwvp_asynch_map;

typedef	struct	segment_info_def
{
	ULONG	lpartition_id;
	ULONG	block_offset;
	ULONG	block_count;
	ULONG	extra;
} segment_info;

typedef	struct	vpartition_info_def
{
	BYTE	volume_name[16];
	ULONG	blocks_per_cluster;
	ULONG	cluster_count;
	ULONG	segment_count;
	ULONG	flags;
} vpartition_info;

typedef	struct	nwvp_remirror_message_def
{
	struct	nwvp_remirror_message_def	*next_link;
	ULONG	return_code;
	ULONG	block_number;
	ULONG	block_count;

	BYTE	*data_buffer;
	ULONG	thread_id;
	ULONG	filler[3];
} nwvp_remirror_message;

typedef	struct	nwvp_mirror_sector_def
{
	BYTE		nwvp_mirror_stamp[16];
	ULONG		mirror_id;
	ULONG		time_stamp;
	ULONG		modified_stamp;
	ULONG		table_index;
	struct
	{
		ULONG    partition_id;
		ULONG	time_stamp;
		ULONG	remirror_section;
		ULONG	status;
	}log[30];
} nwvp_mirror_sector;

#define	MIRROR_NEW_BIT			0x00000001
#define	MIRROR_PRESENT_BIT		0x00000002
#define	MIRROR_INSYNCH_BIT		0x00000004
#define	MIRROR_DELETED_BIT		0x00000008
#define	MIRROR_REMIRRORING_BIT		0x00000010
#define MIRROR_READ_ONLY_BIT            0x00000020

#define	MIRROR_GROUP_ESTABLISHED_BIT	0x00010000
#define	MIRROR_GROUP_ACTIVE_BIT		0x00020000
#define	MIRROR_GROUP_OPEN_BIT		0x00040000
#define	MIRROR_GROUP_MODIFY_CHECK_BIT	0x00080000
#define	MIRROR_GROUP_REMIRRORING_BIT	0x00100000
#define	MIRROR_GROUP_CHECK_BIT		0x00200000
#define	MIRROR_GROUP_READ_ONLY_BIT	0x00400000

typedef	struct	nwvp_rpartition_scan_info_def
{
	ULONG	rpart_handle;
	ULONG	physical_block_count;
	ULONG	lpart_handle;
	ULONG	partition_type;
} nwvp_rpartition_scan_info;

typedef	struct	nwvp_rpartition_info_def
{
	ULONG	rpart_handle;
	ULONG	lpart_handle;
	ULONG	physical_block_count;
	ULONG	partition_type;
	ULONG	physical_handle;
	ULONG	physical_disk_number;
	ULONG	partition_offset;
	ULONG	partition_number;
	ULONG	status;
}	nwvp_rpartition_info;

typedef	struct	nwvp_lpart_def
{
	ULONG	rpartition_handle;
	ULONG	lpartition_handle;
	ULONG	lpartition_type;
	ULONG	partition_type;
	ULONG	mirror_partition_id;
	ULONG	mirror_group_id;
	ULONG	open_count;
	ULONG	read_round_robin;
	ULONG	rpart_handle;
	ULONG	low_level_handle;
	ULONG	remirror_section;
	nwvp_remirror_message	*remirror_list_head;
	nwvp_remirror_message	*remirror_list_tail;
	ULONG	remirror_spin_lock;
	ULONG	physical_block_count;
	ULONG	logical_block_count;
	ULONG	minimum_segment_size;
	ULONG	maximum_segment_size;
	ULONG	available_capacity;
	ULONG	sectors_per_block;
	ULONG	sector_size_shift;
	struct
	{
		ULONG		block_count;
		ULONG		**bit_table;
		ULONG		**hotfix_table;
		ULONG		**bad_bit_table;
		HOTFIX		*sector;
	} hotfix;

	ULONG	scan_flag;
	ULONG	mirror_status;
	ULONG	mirror_count;
	ULONG	mirror_sector_index;
	struct nwvp_lpart_def	*primary_link;
	struct nwvp_lpart_def	*mirror_link;
	nwvp_mirror_sector		*mirror_sector;
	MIRROR					*netware_mirror_sector;
	VOLUME_TABLE    		*segment_sector;
} nwvp_lpart;

typedef	struct	nwvp_lpartition_scan_info_def
{
	ULONG	lpart_handle;
	ULONG	logical_block_count;
	ULONG	open_flag;
	ULONG	mirror_status;
} nwvp_lpartition_scan_info;

typedef	struct	nwvp_lpartition_space_return_info_Def
{
	ULONG	lpart_handle;
	ULONG	segment_offset;
	ULONG	segment_count;
	ULONG	status_bits;
} nwvp_lpartition_space_return_info;

typedef	struct	nwvp_lpartition_info_def
{
	ULONG	lpartition_handle;
	ULONG	lpartition_type;
	ULONG	lpartition_status;
	ULONG	open_count;
	ULONG	segment_count;
	ULONG	mirror_count;
	ULONG 	logical_block_count;
	struct
	{
		ULONG	rpart_handle;
		ULONG	hotfix_block_count;
		ULONG	status;
		ULONG	extra;
	} m[8];
} nwvp_lpartition_info;

typedef	struct	nwvp_vpartition_def
{
	ULONG	vpartition_handle;
	ULONG	vpartition_type;
	ULONG	volume_valid_flag;
	ULONG	open_flag;
	BYTE	volume_name[16];
	ULONG	mirror_flag;
	ULONG	blocks_per_cluster;
	ULONG	segment_count;
	ULONG	cluster_count;
	ULONG	FAT1;
	ULONG	FAT2;
	ULONG	Directory1;
	ULONG	Directory2;
	struct
	{
		ULONG	volume_entry_index;
		ULONG	segment_block_count;
		ULONG	partition_block_offset;
		ULONG	relative_cluster_offset;
		nwvp_lpart	*lpart_link;
	} segments[32];
} nwvp_vpartition;

typedef	struct	nwvp_vpartition_info_def
{
	ULONG	vpartition_handle;
	ULONG	vpartition_type;
	ULONG	volume_valid_flag;
	ULONG	open_flag;
	BYTE	volume_name[16];
	ULONG	mirror_flag;
	ULONG	segment_count;
	ULONG	cluster_count;
	ULONG	blocks_per_cluster;
	ULONG	vpart_handle;
	ULONG	FAT1;
	ULONG	FAT2;
	ULONG	Directory1;
	ULONG	Directory2;
	struct
	{
		ULONG	segment_block_count;
		ULONG	partition_block_offset;
		ULONG	relative_cluster_offset;
		ULONG	lpart_handle;
	} segments[32];
} nwvp_vpartition_info;

extern	ULONG	virtual_partition_table_count;
extern	nwvp_vpartition	*virtual_partition_table[256];

typedef	struct	nwvp_block_map_def
{
	ULONG	rpart_handle;
	ULONG	block_offset;
	ULONG	block_count;
	ULONG	block_bad_bits;
} nwvp_block_map;

#define		NWVP_VALIDATE_PHASE		0
#define		NWVP_REMIRROR_PHASE		1
#define		NWVP_UPDATE_PHASE		2
#define		NWVP_ABORT_PHASE		3

typedef	struct	nwvp_remirror_control_def
{
	struct	nwvp_remirror_control_def	*next_link;
	struct	nwvp_remirror_control_def	*last_link;
	ULONG	state;
	ULONG	index;
	ULONG	section;
	ULONG	original_section;
	ULONG	abort_flag;
	ULONG	lpart_handle;
	ULONG	check_flag;
	ULONG	remirror_flag;
	BYTE	valid_bits[1024 / 8];
} nwvp_remirror_control;

typedef  struct    nwvp_mismatch_def
{
    struct    nwvp_mismatch_def *next_link;
    struct    nwvp_mismatch_def *last_link;
    ULONG     block_number;
    nwvp_lpart    *lpart;
    ULONG         fat_block_number;
    ULONG         *fat_block1;
    ULONG         *fat_block2;
    ULONG         extra;
} nwvp_mismatch;

typedef   struct    nwvp_fat_fix_info_def
{
    ULONG   total_fat_blocks;
    ULONG   total_valid_mirror_blocks;
    ULONG   total_valid_single_blocks;
    ULONG   total_missing_blocks;
    ULONG   total_mismatch_blocks;

    ULONG   total_entries;
    ULONG   total_index_mismatch_entries;
    ULONG   total_end_null_entries;
    ULONG   total_SA_null_entries;
    ULONG   total_FAT_null_entries;
    ULONG   total_SA_end_entries;
    ULONG   total_SA_FAT_entries;
    ULONG   total_SA_mismatch_entries;
    ULONG   total_FAT_end_entries;
    ULONG   total_FAT_mismatch_entries;
} nwvp_fat_fix_info;

#define	NWVP_EXTENT_TABLE_SIZE	200

typedef	struct	nwvp_file_extent_def
{
    struct	nwvp_file_extent_def	*next_link;
    ULONG	directory_info[23];
    ULONG	table_index;
    struct
    {
	ULONG	file_offset;
	ULONG	sector_count;
	ULONG	partition_offset;
	ULONG	partition_handle;
    } table[NWVP_EXTENT_TABLE_SIZE];
} nwvp_file_extent;

void	nwvp_init(void);

void	nwvp_uninit(void);

ULONG	nwvp_segmentation_remove(
	nwvp_lpart		*lpart);

ULONG	nwvp_segmentation_add(
	nwvp_lpart		*lpart);

ULONG	nwvp_create_new_id(void);

ULONG	nwvp_hotfix_destroy(
	nwvp_lpart	*lpart);

ULONG	nwvp_segmentation_sectors_flush(
	nwvp_lpart	*lpart);

ULONG	nwvp_segmentation_sector_validate(
	VOLUME_TABLE	*seg_sector);

ULONG	nwvp_segmentation_sectors_load(
	nwvp_lpart	*lpart);

ULONG	nwvp_master_sectors_flush(
	nwvp_lpart	*lpart);

ULONG	nwvp_netware_mirror_update(
	nwvp_lpart	*lpart,
	ULONG		flush_flag);

ULONG	nwvp_hotfix_tables_flush(
	nwvp_lpart	*lpart,
	ULONG		table_index);

ULONG	nwvp_hotfix_tables_load(
	nwvp_lpart	*lpart);

ULONG	nwvp_hotfix_tables_alloc(
	nwvp_lpart		*lpart);

ULONG	nwvp_hotfix_tables_free(
	nwvp_lpart		*lpart);

ULONG	nwvp_hotfix_scan(
	nwvp_lpart	*lpart);

ULONG	nwvp_hotfix_create(
	nwvp_lpart	*lpart,
	ULONG		logical_block_count);

ULONG	nwvp_hotfix_block_lookup(
	nwvp_lpart	*lpart,
	ULONG		block_number,
	ULONG		*relative_block_number,
	ULONG		*bad_bits);

ULONG	nwvp_hotfix_block(
	nwvp_lpart	*original_part,
	ULONG		original_block_number,
	BYTE		*original_buffer,
	ULONG		*original_bad_bits,
	ULONG		read_flag);

ULONG	nwvp_hotfix_update_bad_bits(
	nwvp_lpart	*original_part,
	ULONG		original_block_number,
	ULONG		original_bad_bits);



ULONG	nwvp_mirror_create_entry(
	nwvp_lpart	   	*lpart,
	ULONG			mirror_sector_index,
	ULONG			partition_id,
	ULONG			group_id);

void	nwvp_mirror_group_establish(
	nwvp_lpart	   	*lpart,
	ULONG			hard_activate_flag);

ULONG	nwvp_mirror_group_modify_check(
	nwvp_lpart	*lpart);

ULONG	nwvp_mirror_group_remirror_update(
	ULONG	lpart_handle,
	ULONG	section,
	ULONG	*reset_flag);

ULONG	nwvp_mirror_group_add(
	nwvp_lpart	   	*lpart);

void	nwvp_mirror_group_delete(
	nwvp_lpart		*member);

ULONG	nwvp_mirror_group_update(
	nwvp_lpart		*member);

ULONG	nwvp_mirror_member_add(
	nwvp_lpart		*mpart,
	nwvp_lpart		*new_part);

ULONG	nwvp_mirror_member_delete(
	nwvp_lpart		*del_part);

ULONG	nwvp_mirror_group_open(
	nwvp_lpart	   	*lpart);

ULONG	nwvp_mirror_group_close(
	nwvp_lpart	   	*lpart);

ULONG	nwvp_rpartition_sector_read(
	ULONG		rpart_handle,
	ULONG		sector_number,
	ULONG		sector_count,
	BYTE		*data_buffer);

ULONG	nwvp_rpartition_sector_write(
	ULONG		rpart_handle,
	ULONG		sector_number,
	ULONG		sector_count,
	BYTE		*data_buffer);

/*
ULONG	nwvp_lpartition_open(
	ULONG	lpart_handle,
	ULONG	group_complete_flag);

ULONG	nwvp_lpartition_close(
	ULONG	lpart_handle);
*/

ULONG	nwvp_lpartition_activate(
	ULONG		lpart_handle);

ULONG	inwvp_lpartition_block_read(
	nwvp_lpart	*original_part,
	ULONG		block_number,
	ULONG		block_count,
	BYTE		*data_buffer);

ULONG	nwvp_lpartition_block_read(
	ULONG		lpart_handle,
	ULONG		block_number,
	ULONG		block_count,
	BYTE		*data_buffer);

ULONG	inwvp_lpartition_block_write(
	nwvp_lpart	*original_part,
	ULONG		block_number,
	ULONG		block_count,
	BYTE		*data_buffer);

ULONG	nwvp_lpartition_block_write(
	ULONG		lpart_handle,
	ULONG		block_number,
	ULONG		block_count,
	BYTE		*data_buffer);


ULONG	nwvp_vpartition_sub_block_read(
	ULONG		vpart_handle,
	ULONG		block_number,
	ULONG		block_offset,
	ULONG		data_size,
	BYTE		*data_buffer,
	ULONG		as);

ULONG	nwvp_vpartition_map_asynch_read(
	ULONG		vpart_handle,
	ULONG		block_number,
	ULONG		*map_entry_count,
	nwvp_asynch_map	*map);

ULONG	nwvp_vpartition_block_read(
	ULONG		vpart_handle,
	ULONG		block_number,
	ULONG		block_count,
	BYTE		*data_buffer);

ULONG	nwvp_vpartition_map_asynch_write(
	ULONG		vpart_handle,
	ULONG		block_number,
	ULONG		*map_entry_count,
	nwvp_asynch_map	*map);

ULONG	inwvp_vpartition_block_write(
	nwvp_vpartition	*vpartition,
	ULONG		block_number,
	ULONG		block_count,
	BYTE		*data_buffer);

ULONG	nwvp_vpartition_sub_block_write(
	ULONG		vpart_handle,
	ULONG		block_number,
	ULONG		block_offset,
	ULONG		data_size,
	BYTE		*data_buffer,
	ULONG		as);

ULONG	nwvp_vpartition_bad_bit_update(
	ULONG		vpart_handle,
	ULONG		block_number,
	ULONG		new_bad_bits);

ULONG	nwvp_vpartition_block_write(
	ULONG		vpart_handle,
	ULONG		block_number,
	ULONG		block_count,
	BYTE		*data_buffer);

ULONG	nwvp_vpartition_block_hotfix(
	ULONG		vpart_handle,
	ULONG		block_number,
	BYTE		*data_buffer);

ULONG	nwvp_vpartition_block_zero(
	ULONG		vpart_handle,
	ULONG		block_number,
	ULONG		block_count);

ULONG	nwvp_rpartition_scan(
	nwvp_payload	*payload);

ULONG	nwvp_rpartition_return_info(
	ULONG						rpart_handle,
	nwvp_rpartition_info		*rpart_info);

ULONG	nwvp_rpartition_map_rpart_handle(
	ULONG		*rpart_handle,
	ULONG		physical_handle);

ULONG	nwvp_lpartition_scan(
	nwvp_payload	*payload);

ULONG	nwvp_lpartition_return_info(
	ULONG						lpart_handle,
	nwvp_lpartition_info		*lpart_info);

ULONG	nwvp_vpartition_scan(
	nwvp_payload	*payload);

ULONG	nwvp_vpartition_return_info(
	ULONG					vpart_handle,
	nwvp_vpartition_info	*vpinfo);

ULONG	nwvp_lpartition_return_space_info(
	ULONG			lpart_handle,
	nwvp_payload	*payload);

ULONG	nwvp_lpartition_format(
	ULONG	*lpartition_id,
	ULONG	logical_block_count,
	ULONG	*base_partition_ids);

ULONG	nwvp_lpartition_abort_remirror(
	ULONG	lpart_handle);

ULONG	nwvp_lpartition_remirror(
	ULONG	lpart_handle);

ULONG	nwvp_lpartition_unformat(
	ULONG	lpart_handle);

ULONG	nwvp_remirror_section(
	nwvp_lpart	*lpart,
	ULONG		section_number,
	ULONG		block_count,
	BYTE		*buffer);

ULONG	nwvp_mirror_validate_process(void);

ULONG	nwvp_partition_add_mirror(
	ULONG	lpart_handle,
	ULONG	rpart_handle);

ULONG	nwvp_partition_del_mirror(
	ULONG	lpart_handle,
	ULONG	rpart_handle);

ULONG	nwvp_bounds_check(
	ULONG	start1,
	ULONG	count1,
	ULONG	start2,
	ULONG	count2);

ULONG	nwvp_vpartition_fat_check(
	ULONG		vpart_handle);

ULONG	nwvp_vpartition_build_fat(
	nwvp_vpartition	*vpartition,
	ULONG		current_segment);

ULONG	nwvp_vpartition_segment_flush(
	nwvp_vpartition	*vpartition);

ULONG	vpartition_update_entry_index(
	nwvp_lpart		*lpart,
	BYTE			*volume_name,
	ULONG			old_index,
	ULONG			new_index);

ULONG	vpartition_update_lpartition(
	nwvp_lpart		*lpart,
    nwvp_lpart      *new_lpart);

ULONG	nwvp_vpartition_fat_fix(
	ULONG		vpart_handle,
    nwvp_fat_fix_info        *fix_info,
    ULONG       fix_flag);

ULONG	inwvp_vpartition_open(
	nwvp_vpartition		*vpartition);

ULONG	nwvp_vpartition_open(
	ULONG		vpart_handle);

ULONG	inwvp_vpartition_close(
	nwvp_vpartition		*vpartition);

ULONG	nwvp_vpartition_close(
	ULONG		vpart_handle);

ULONG	nwvp_vpartition_format(
	ULONG					*vpartition_id,
	vpartition_info	*vpart_info,
	segment_info			*segment_info_table);

ULONG	nwvp_vpartition_add_segment(
	ULONG					vpart_handle,
	segment_info			*segment_info_table);

ULONG	nwvp_vpartition_unformat(
	ULONG		vpart_handle);

void	nwvp_scan_routine(
	ULONG	scan_flag);

void	nwvp_unscan_routine(void);

ULONG	nwvp_map_volume_physical(
	ULONG	vpart_handle,
	ULONG SectorOffsetInVolume,
	ULONG SectorCount,
	void **PartitionPointer,
	ULONG *SectorOffsetInPartition,
	ULONG *SectorsReturned);


void	nwvp_log(
	ULONG	event_type,
	ULONG	handle,
	ULONG	parameter0,
	ULONG	parameter1);

ULONG	nwvp_get_physical_partitions(
	ULONG	vpart_handle,
	ULONG	*ppartition_table_count,
	ULONG	*ppartition_table);

ULONG	nwvp_convert_local_handle(
	ULONG	rpart_handle,
	ULONG	*NT_handle);

ULONG	nwvp_hotfix_table_check(
	ULONG	rpart_handle,
	ULONG	index,
	ULONG	*value);

ULONG	nwvp_part_map_to_physical(
	ULONG	rpart_handle,
	ULONG	*disk_id,
	ULONG	*disk_offset,
	ULONG	*partition_number);

ULONG	nwvp_nt_partition_shutdown(
	ULONG	low_level_handle);

#if !MANOS

ULONG	nwvp_alloc(
	void	   **mem_ptr,
	ULONG		size);

ULONG	nwvp_io_alloc(
	void	   **mem_ptr,
	ULONG		size);

ULONG	nwvp_io_free(void *mem_ptr);

ULONG	nwvp_free(void *mem_ptr);

void	nwvp_spin_lock(
	ULONG	*spin_lock);

void	nwvp_spin_unlock(
	ULONG	*spin_lock);


#endif
