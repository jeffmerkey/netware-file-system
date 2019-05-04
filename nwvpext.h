

/***************************************************************************
*
*   Copyright (c) 1998, 1999 Jeff V. Merkey
*   895 West Center Street
*   Orem, Utah  84057
*
****************************************************************************
*
*   AUTHOR   :  Darren Major
*   FILE     :  NWVP1.H
*   DESCRIP  :  NetWare Virtual Partition
*   DATE     :  August 1, 1999
*
***************************************************************************/


#define		NWVP_HOTFIX_FOUND		1
#define		NWVP_VOLUMES_FOUND		2
#define		NWVP_FIRST_FAT_FOUND	3

typedef	struct	nwvp_hotfix_scan_info_def
{
	ULONG	hotfix_partition_offset;
	ULONG	hotfix_data_offset;
	ULONG	hotfix_data_size;
} nwvp_hotfix_scan_info;

typedef	struct	nwvp_volume_scan_info_def
{
	ULONG	volume_sector_offset;
	ULONG	volume_segment_count;
	struct
	{
    	ULONG	total_cluster_count;
		ULONG	segment_sector_offset;
		ULONG	segment_sector_count;
		ULONG	segment_info;
	} segments[8];
} nwvp_volume_scan_info;

typedef	struct	nwvp_fat_scan_info_def
{
	ULONG	complete_flag;
	ULONG	blocks_per_cluster;
	ULONG	DIR_cluster_count;
	ULONG	DIR1_cluster_offset;
	ULONG	DIR2_cluster_offset;
	ULONG	FAT_sector_offset;
	ULONG	FAT_cluster_count;
	ULONG	FAT1_cluster_offset;
	ULONG	FAT2_cluster_offset;
} nwvp_fat_scan_info;

ULONG	nwvp_FAT_compare(
	FAT_ENTRY	*fat1,
	FAT_ENTRY	*fat2,
	ULONG		mismatch_count);

ULONG	nwvp_recover_nwpart_info(
	ULONG			lpart_handle,
	ULONG			total_sector_count,
	ULONG			block_offset,
	ULONG			block_count,
	nwvp_hotfix_scan_info	*hotfix_info,
	nwvp_volume_scan_info	*volume_info,
	nwvp_fat_scan_info		*fat_info);

ULONG	inwvp_lpartition_block_map(
	nwvp_lpart	*original_part,
	ULONG		block_number,
	ULONG		block_count,
	ULONG		mirror_index,
	nwvp_block_map	*bmap);

ULONG	nwvp_vpartition_block_map(
	ULONG		vpart_handle,
	ULONG		block_number,
	ULONG		block_count,
	ULONG		mirror_index,
	nwvp_block_map	*bmap);

