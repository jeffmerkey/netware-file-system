
/***************************************************************************
*
*   Copyright (c) 1997-2001 Jeff V. Merkey  All Rights Reserved.
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
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  NWRESTOR.C
*   DESCRIP  :  NWFS Volume Restore Utility
*   DATE     :  August 6, 2022
*
*
***************************************************************************/

#include "globals.h"
#include "image.h"

#if (LINUX_UTIL)
#include <dirent.h>
#endif

#if (LINUX_UTIL && LINUX_DEV)
extern int devfd;
#endif

IMAGE_SET image_set_table[IMAGE_SET_TABLE_SIZE];
IMAGE image_table[IMAGE_TABLE_SIZE];
ULONG mark_table[IMAGE_TABLE_SIZE];
BYTE target_volume_table[IMAGE_TABLE_SIZE][16];
BYTE set_file_name_table[IMAGE_SET_TABLE_SIZE][16];
BYTE ImageSetName[] = "IMAGE000.SET";
ULONG image_set_table_count = 0;
ULONG set_file_use_table[IMAGE_SET_TABLE_SIZE];
ULONG image_table_count;
ULONG valid_table[IMAGE_TABLE_SIZE];
extern void UtilScanVolumes(void);

ULONG YesOrNo(BYTE *prompt)
{
    BYTE input_buffer[20] = {""};
    while (1)
    {
	NWFSPrint("\r%s", prompt);
	fgets(input_buffer, sizeof(input_buffer), stdin);
	if ((input_buffer[0] == 'n') || (input_buffer[0] == 'N'))
	    return(-1);
	if ((input_buffer[0] == 'y') || (input_buffer[0] == 'Y'))
	    return(0);
	NWFSPrint(" Invalid Response.  Try again \n\n");
    }
}

ULONG   nwvp_volume_alloc_space_create(
	ULONG           *vpart_handle,
	vpartition_info *vpart_info,
	ULONG		mirror_flag)
{
	ULONG           j, k, l, m;
//      ULONG           lpartition_id;
	ULONG           new_space;
	ULONG           cluster_count;
	ULONG           cluster_needs;
	ULONG           segment_size;
	ULONG           diff;
	ULONG           lowest_count;
	ULONG           lowest_index;
//      ULONG           base_count;
//      ULONG           base_index;
//      ULONG           mirror_index;
//      ULONG           base_block_count = 0;
	ULONG   segment_count;
//      ULONG   part_ids[4];
	nwvp_lpartition_space_return_info       slist[5];
	nwvp_lpartition_scan_info               llist[5];
	segment_info    segment_assign[32];
	nwvp_payload    payload;
	nwvp_payload    payload1;
//      nwvp_lpart              *lpart;
//      nwvp_lpart              *base_part;

	new_space = 1;
	NWLockNWVP();
	while (new_space != 0)
	{
		new_space = 0;
		for (k=0; k<32; k++)
		{
			segment_assign[k].lpartition_id = 0;
			segment_assign[k].block_count = 0;
			segment_assign[k].block_offset = 0;
			segment_assign[k].extra = 0;
		}
		segment_count = 0;

		payload1.payload_object_count = 0;
		payload1.payload_index = 0;
		payload1.payload_object_size_buffer_size = (sizeof(nwvp_lpartition_scan_info) << 20) + sizeof(llist);
		payload1.payload_buffer = (BYTE *) &llist[0];
		do
		{
			nwvp_lpartition_scan(&payload1);
			for (m=0; m< payload1.payload_object_count; m++)
			{
				payload.payload_object_count = 0;
				payload.payload_index = 0;
				payload.payload_object_size_buffer_size = (8 << 20) + sizeof(slist);
				payload.payload_buffer = (BYTE *) &slist[0];
				do
				{
					NWUnlockNWVP();
					nwvp_lpartition_return_space_info(llist[m].lpart_handle, &payload);
					NWLockNWVP();
					for (j=0; j< payload.payload_object_count; j++)
					{
						if ((slist[j].status_bits & INUSE_BIT) == 0)
						{
							if ((((slist[j].status_bits & MIRROR_BIT) == 0) && (mirror_flag == 0)) ||
								(((slist[j].status_bits & MIRROR_BIT) != 0) && (mirror_flag != 0)))
							{
								for (k=0; k<segment_count; k++)
								{
									if (segment_assign[k].lpartition_id == slist[j].lpart_handle)
									{
										if (segment_assign[k].block_count < slist[j].segment_count)
										{
											segment_assign[k].block_count = (slist[j].segment_count + 15) & 0xFFFFFFF0;
											segment_assign[k].block_offset = slist[j].segment_offset;
										}
										break;
									}
								}
								if (k == segment_count)
								{
									if (segment_count == vpart_info->segment_count)
									{
										lowest_count = 0xFFFFFFFF;
										lowest_index = 0;
										for (k=0; k<segment_count; k++)
										{
											if (segment_assign[k].block_count < lowest_count)
											{
												lowest_count = segment_assign[k].block_count;
												lowest_index = k;
											}
										}
										if (segment_assign[lowest_index].block_count < slist[j].segment_count)
										{
											segment_assign[lowest_index].block_count = (slist[j].segment_count + 15) & 0xFFFFFFF0;
											segment_assign[lowest_index].block_offset = slist[j].segment_offset;
											segment_assign[lowest_index].lpartition_id = slist[j].lpart_handle;
										}
									}
									else
									{
										segment_assign[k].lpartition_id = slist[j].lpart_handle;
										segment_assign[k].block_offset = slist[j].segment_offset;
										segment_assign[k].block_count = (slist[j].segment_count + 15) & 0xFFFFFFF0;
										segment_count ++;
									}
								}

								if (segment_count == vpart_info->segment_count)
								{
									cluster_count = vpart_info->cluster_count * vpart_info->blocks_per_cluster;
									segment_size = ((cluster_count / segment_count) + 15 ) & 0xFFFFFFF0;
									for (k=0; k<segment_count; k++)
										segment_assign[k].extra = 0;
									for (k=0; k<segment_count; k++)
									{
										if ((segment_assign[k].extra + segment_size) <= segment_assign[k].block_count)
										{
											segment_assign[k].extra += segment_size;
											if (cluster_count >= segment_assign[k].extra)
												cluster_count -= segment_assign[k].extra;
											else
											{
												segment_assign[k].extra -= (segment_assign[k].extra - cluster_count);
												cluster_count = 0;
											}
										}
										else
										{
											cluster_needs = (segment_assign[k].extra + segment_size) - segment_assign[k].block_count;
											segment_assign[k].extra = segment_assign[k].block_count;
											if (cluster_count >= segment_assign[k].extra)
												cluster_count -= segment_assign[k].extra;
											else
											{
												segment_assign[k].extra -= (segment_assign[k].extra - cluster_count);
												cluster_count = 0;
											}
											for(l=k; l>0; l--)
											{
												if ((diff = (segment_assign[l-1].block_count - segment_assign[l-1].extra)) > 0)
												{
													if (diff >= cluster_needs)
													{
														if (cluster_count >= cluster_needs)
														{
															segment_assign[l-1].extra += cluster_needs;
															cluster_count -= cluster_needs;
														}
														else
														{
															segment_assign[l-1].extra += cluster_count;
															cluster_count = 0;
														}
														break;
													}
													else
													{
														segment_assign[k].extra = segment_assign[k].block_count;
														cluster_needs -= diff;
														cluster_count -= diff;
													}
												}
											}
											if (cluster_needs > 0)
											{
												segment_assign[k+1].extra = cluster_needs;
											}
										}
									}
									if (cluster_count == 0)
									{
										for (k=0; k<segment_count; k++)
										{
											segment_assign[k].block_count = segment_assign[k].extra;
											segment_assign[k].extra = 0;
										}
										NWUnlockNWVP();
										if (nwvp_vpartition_format(vpart_handle, vpart_info, &segment_assign[0]) == 0)
										{
											return(0);
										}
										NWLockNWVP();
									}
								}
							}
						}
					}
				} while ((payload.payload_index != 0) && (payload.payload_object_count != 0));
			}
		} while ((payload1.payload_index != 0) && (payload1.payload_object_count != 0));

//
//      create new netware partitions
//
/*
		cluster_count = vpart_info->cluster_count * vpart_info->blocks_per_cluster;
		segment_size = ((cluster_count / vpart_info->segment_count) + 215 ) & 0xFFFFFFF0;
		for (k=0; k<segment_count; k++)
			cluster_count -= segment_assign[k].block_count;
		cluster_count = ((cluster_count + 15) & 0xFFFFFFF0) + 0x14;
		if ((vpart_info->flags & MIRROR_BIT) == 0)
		{
			base_count = 0;
			base_index = 0xFFFFFFFF;
			for (l=0; l<256; l++)
			{
				if (((base_part = raw_partition_table[l]) != 0) &&
					(base_part->lpartition_handle == 0xFFFFFFFF) &&
					((base_part->partition_type == 0x65) || (base_part->partition_type == 0x77)) &&
					(base_part->physical_block_count >= segment_size))
				{
					lowest_count = 0xFFFFFFFF;
//                                      lowest_index = 0xFFFFFFFF;
					for (k=0; k<256; k++)
					{
						if (((lpart = raw_partition_table[k]) != 0) &&
							(lpart != base_part) &&
							(lpart->lpartition_handle == 0xFFFFFFFF) &&
							((lpart->partition_type == 0x65) || (lpart->partition_type == 0x77)))
						{
							diff = (lpart->physical_block_count > base_part->physical_block_count) ?
									lpart->physical_block_count - base_part->physical_block_count :
									base_part->physical_block_count - lpart->physical_block_count;
							if (diff < lowest_count)
							{
								lowest_count = diff;
//                                                              lowest_index = k;
							}
						}
					}

					if (lowest_count > base_count)
					{
						base_count = lowest_count;
						base_index = l;
						base_block_count = (((base_part->physical_block_count - 256) + 15) & 0xFFFFFFF0) + 0x14;
						if ((base_block_count < cluster_count) && ((base_block_count + 200) > cluster_count))
							base_block_count = cluster_count;
					}
				}
			}
			if (base_index != 0xFFFFFFFF)
			{
				part_ids[0] = base_index;
				part_ids[1] = 0xFFFFFFFF;
				part_ids[2] = 0xFFFFFFFF;
				part_ids[3] = 0xFFFFFFFF;

				NWUnlockNWVP();
				if (nwvp_lpartition_format(&lpartition_id, base_block_count, &part_ids[0]) == 0)
					new_space = 1;
				NWLockNWVP();
			}
		}
		else
		{
			base_count = 0xFFFFFFFF;
			base_index = 0xFFFFFFFF;
			mirror_index = 0xFFFFFFFF;
			for (l=0; l<256; l++)
			{
				if (((base_part = raw_partition_table[l]) != 0) &&
					(base_part->lpartition_handle == 0xFFFFFFFF) &&
					((base_part->partition_type == 0x65) || (base_part->partition_type == 0x77)) &&
					(base_part->physical_block_count >= segment_size))
				{
					lowest_count = 0xFFFFFFFF;
					lowest_index = 0xFFFFFFFF;
					for (k=l+1; k<256; k++)
					{
						if (((lpart = raw_partition_table[k]) != 0) &&
							(lpart->lpartition_handle == 0xFFFFFFFF) &&
							((lpart->partition_type == 0x65) || (lpart->partition_type == 0x77)) &&
							(lpart->physical_block_count >= segment_size))

						{
							diff = (lpart->physical_block_count > base_part->physical_block_count) ?
									lpart->physical_block_count - base_part->physical_block_count :
									base_part->physical_block_count - lpart->physical_block_count;
							if (diff < lowest_count)
							{
								lowest_count = diff;
								lowest_index = k;
							}
						}
					}

					if (lowest_count < base_count)
					{
						base_count = lowest_count;
						mirror_index = lowest_index;
						base_index = l;
						base_block_count = (base_part->physical_block_count > raw_partition_table[mirror_index]->physical_block_count) ?
									raw_partition_table[mirror_index]->physical_block_count :
									base_part->physical_block_count;
						base_block_count = (((base_block_count - 256) + 15) & 0xFFFFFFF0) + 0x14;
						if ((base_block_count < cluster_count) && ((base_block_count + 200) > cluster_count))
							base_block_count = cluster_count;
					}
				}
			}
			if ((base_index != 0xFFFFFFFF) && (mirror_index != 0xFFFFFFFF))
			{
				part_ids[0] = base_index;
				part_ids[1] = mirror_index;
				part_ids[2] = 0xFFFFFFFF;
				part_ids[3] = 0xFFFFFFFF;

				NWUnlockNWVP();
				if (nwvp_lpartition_format(&lpartition_id, base_block_count, &part_ids[0]) == 0)
					new_space = 1;
				NWLockNWVP();
			}
		}
*/
	}
	NWUnlockNWVP();
	return(-1);
}


void edit_image(IMAGE *original_image)
{
    ULONG i;
    ULONG size;
    BYTE input_buffer[20] = {""};
    IMAGE image;

    NWFSCopy(&image, original_image, sizeof(image));
    while (TRUE)
    {
       ClearScreen(consoleHandle);

       NWFSPrint("\n NetWare Volume Restore Utility\n");
       NWFSPrint(" Edit Volume Info Screen\n");
       NWFSPrint("\n");
       NWFSPrint("            Volume Name: %s \n", &image.VolumeName[1]);
       NWFSPrint("           Cluster Size: %d K\n", (int)image.VolumeClusterSize / 1024);
       NWFSPrint("          Cluster Count: %d \n", (int)image.LogicalVolumeClusters);
       NWFSPrint("          Segment Count: %d \n", (int)image.SegmentCount);
       NWFSPrint("            Mirror Flag: %s \n", (image.MirrorFlag == 0) ? "SINGLE" : "MIRRORED");
       NWFSPrint("\n");
       NWFSPrint("            Volume Size: %d K\n", (int)(image.LogicalVolumeClusters * (image.VolumeClusterSize / 1024)));
       NWFSPrint("        Image Data Size: %d K\n", (int)(image.AllocatedVolumeClusters * (image.VolumeClusterSize / 1024)));
       NWFSPrint("       Image File Count: %d \n", (int)image.FilesInDataSet);
       NWFSPrint("\n");
       NWFSPrint("               1. Apply Changes And Return\n");
       NWFSPrint("               2. Change Volume Name\n");
       NWFSPrint("               3. Change Volume Cluster Size\n");
       NWFSPrint("               4. Change Volume Cluster Count\n");
       NWFSPrint("               5. Change Volume Segment Count\n");
       NWFSPrint("               6. Change Volume Mirror Flag\n");
       NWFSPrint("               7. Abort Changes And Exit\n\n");
       NWFSPrint("        Enter a Menu Option: ");

       fgets(input_buffer, sizeof(input_buffer), stdin);
       switch(input_buffer[0])
       {
	  case '1':
	     ClearScreen(consoleHandle);
	     NWFSCopy(original_image, &image, sizeof(image));
	     return;

	  case '2':
	     NWFSPrint("         Enter Volume Name: ");
	     fgets(input_buffer, sizeof(input_buffer), stdin);
	     for (i=0; i < 15; i++)
	     {
		if ((input_buffer[i] >= 'a') && (input_buffer[i] <= 'z'))
		   image.VolumeName[i+1] = (input_buffer[i] - 'a') + 'A';
		else
		if ((input_buffer[i] >= 'A') && (input_buffer[i] <= 'Z'))
		   image.VolumeName[i+1] = input_buffer[i];
		else
		if ((input_buffer[i] >= '0') && (input_buffer[i] <= '9'))
		   image.VolumeName[i+1] = input_buffer[i];
		else
		   break;
	     }
	     image.VolumeName[0] = (BYTE) i;
	     for (; i < 15; i++)
		image.VolumeName[i + 1] = 0;

	     WaitForKey();
	     break;

	  case '3':
	     NWFSPrint("         Enter Cluster Size: ");
	     fgets(input_buffer, sizeof(input_buffer), stdin);
	     size = 0xFFFFFFFF;
	     for (i=0; i < 16; i++)
	     {
		if ((input_buffer[i] >= '0') && (input_buffer[i] <='8'))
		{
		   if (size == 0xFFFFFFFF)
		      size = 0;
		   size = (size * 10) + input_buffer[i] - '0';
		}
		else
		   break;
	     }
	     if ((size == 64) || (size == 32) || (size == 16) || (size == 8) || (size == 4))
	     {
		image.LogicalVolumeClusters *= (image.VolumeClusterSize / 1024);
		image.AllocatedVolumeClusters *= (image.VolumeClusterSize / 1024);
		image.VolumeClusterSize = size * 1024;
		image.LogicalVolumeClusters /= (image.VolumeClusterSize / 1024);
		image.AllocatedVolumeClusters /= (image.VolumeClusterSize / 1024);
	     }
	     else
	     {
		if (size != 0xFFFFFFFF)
		{
			 NWFSPrint("       **************  invalid cluster size ************\n");
		   WaitForKey();
		}
	     }
	     break;

	  case '4':
	     NWFSPrint("        Enter Cluster Count: ");
	     fgets(input_buffer, sizeof(input_buffer), stdin);
	     size = 0xFFFFFFFF;
	     for (i=0; i < 16; i++)
	     {
		if ((input_buffer[i] >= '0') && (input_buffer[i] <='8'))
		{
		   if (size == 0xFFFFFFFF)
		      size = 0;
		   size = (size * 10) + input_buffer[i] - '0';
		}
		else
		   break;
	     }
	     if ((size > image.AllocatedVolumeClusters) && (size < 0xFFFFFFFF))
		image.LogicalVolumeClusters = size;
	     else
	     {
		if (size != 0xFFFFFFFF)
		{
		   NWFSPrint("  ************** cluster size can not be less that data set ************\n");
		   WaitForKey();
		}
	     }
	     break;

	  case '5':
	     NWFSPrint("        Enter Segment Count: ");
	     fgets(input_buffer, sizeof(input_buffer), stdin);
	     size = 0xFFFFFFFF;
	     for (i=0; i < 16; i++)
	     {
		if ((input_buffer[i] >= '0') && (input_buffer[i] <='8'))
		{
		   if (size == 0xFFFFFFFF)
		      size = 0;
		   size = (size * 10) + input_buffer[i] - '0';
		}
		else
		  break;
	     }
	     if (size < 32)
		image.SegmentCount = size;
	     break;

	  case '6':
	     image.MirrorFlag = (image.MirrorFlag == 0) ? 0xFFFFFFFF : 0;
	     break;

	  case '7':
	     ClearScreen(consoleHandle);
		 return;
       }
    }
}

void process_set_file(BYTE *set_file_name)
{
    register FILE *vfd;
    register FILE *sfd;
    register ULONG table_index, i, j;
    register ULONG valid_flag = 0xFFFFFFF;
    register ULONG checksum = 0;
    register ULONG *checksum_ptr;
    ULONG    found_flag;
    IMAGE_SET set_header;
    ULONG handles[7];
    nwvp_payload payload;
    nwvp_vpartition_info vpart_info;

    table_index = image_table_count;
    if ((sfd = fopen(&set_file_name[0], "rb+wb")) != 0)
    {
       if (fread(&set_header, sizeof(IMAGE_SET), 1, sfd) != 0)
       {
	  checksum_ptr = (ULONG *) &set_header;

	  for (i=0; i < (sizeof(IMAGE_SET) / 4); i++)
	  {
	     checksum += checksum_ptr[i];
	  }

	  if (checksum != 0xFFFFFFFF)
	  {
	     NWFSPrint(" Checksum Error In Image Set [%s] %X %X\n", &set_file_name[0],
		       (unsigned int)checksum,
		       (unsigned int)set_header.CheckSum);
	     valid_flag =  0;
	     return;
	  }

	  if (set_header.Stamp == IMAGE_SET_STAMP)
	  {
	     for (i=0; i < IMAGE_TABLE_SIZE; i++)
	     {
		if (!set_header.VolumeFile[i][0])
		   continue;
#if (VERBOSE)
		NWFSPrint(" Opening %s\n", &set_header.VolumeFile[i][0]);
#endif
		if ((vfd = fopen(&set_header.VolumeFile[i][0], "rb+wb")) != 0)
		{
		   if (fread(&image_table[table_index], sizeof(IMAGE), 1, vfd) != 0)
		   {
		      if (image_table[table_index].Stamp == IMAGE_FILE_STAMP)
		      {
			 payload.payload_object_count = 0;
			 payload.payload_index = 0;
			 payload.payload_object_size_buffer_size = (7 << 20) + sizeof(handles);
			 payload.payload_buffer = (BYTE *) &handles[0];
			 found_flag = 0;

			 do
			 {
			    nwvp_vpartition_scan(&payload);
			    for (j=0; j < payload.payload_object_count; j++)
			    {
			       if (nwvp_vpartition_return_info(handles[j], &vpart_info) == 0)
			       {
				  if (NWFSCompare(&vpart_info.volume_name[0], &image_table[table_index].VolumeName[0], vpart_info.volume_name[0] + 1) == 0)
				  {
				     found_flag = 0xFFFFFFFF;
				     break;
				  }
			       }
			    }
			 } while ((payload.payload_index != 0) && (payload.payload_object_count != 0) && (found_flag == 0));

			 if (found_flag != 0)
			 {
			    NWFSCopy(&target_volume_table[table_index][0], &image_table[table_index].VolumeName[0], 16);
			    mark_table[table_index] = 0;
			 }
			 else
			 {
			    NWFSSet(&target_volume_table[table_index][0], 0, 16);
			    mark_table[table_index] = 1;
			 }
			 valid_table[table_index] = 1;
			 table_index++;
		      }
		      else
		      {
			 NWFSPrint(" Invalid Image Stamp in %s \n", &set_header.VolumeFile[i][0]);
			 valid_flag = 0;
			 break;
		      }
		   }
		   else
		   {
		      NWFSPrint(" Error reading %s\n", &set_header.VolumeFile[i][0]);
		      valid_flag = 0;
		      break;
		   }
		   fclose(vfd);
		}
	     }
	  }
	  else
	  {
	     NWFSPrint(" Invalid Image Set Stamp in [%s]\n", &set_file_name[0]);
	     valid_flag = 0;
	  }
       }
       fclose(sfd);
    }
    else
    {
       NWFSPrint(" Error opening %s\n", &set_file_name[0]);
       valid_flag = 0;
    }

    if (valid_flag != 0)
    {
       image_table_count = table_index;
#if (VERBOSE)
       WaitForKey();
#endif
    }
}

void select_data_set(void)
{
    ULONG i;
    ULONG index;
    ULONG display_base = 0;
    BYTE input_buffer[20] = {""};

    while (TRUE)
    {
       ClearScreen(consoleHandle);
       NWFSPrint("\n NetWare Volume Restore Utility\n");
       NWFSPrint(" Select Data Set Screen\n");
       NWFSPrint("\n  Used  #           Name     Volume Count   Date \n\n");

       for (i=0; i < 8; i++)
       {
	  if ((index = display_base + i) < image_set_table_count)
	  {
	     NWFSPrint("  %s     %2d %16s    %d     %16s  \n",
	       (set_file_use_table[index] == 0) ? "   " : "***",
	       (int)index,
	       &set_file_name_table[index][0],
	       (int)image_set_table[index].VolumeCount,
		       &image_set_table[index].DateTimeString[0]);
	  }
	  else
	     NWFSPrint("\n");
       }

       NWFSPrint("\n\n");
       NWFSPrint("               1. Select New Image File\n");
       NWFSPrint("               2. Scroll Image Files\n");
       NWFSPrint("               3. Return\n\n");
       NWFSPrint("        Enter a Menu Option: ");

       fgets(input_buffer, sizeof(input_buffer), stdin);
       switch(input_buffer[0])
       {
	  case '1':
	     NWFSPrint("         Enter Image Number: ");
	     fgets(input_buffer, sizeof(input_buffer), stdin);
	     index = 0xFFFFFFFF;

	     for (i=0; i < 4; i++)
	     {
		if ((input_buffer[i] >= '0') && (input_buffer[i] <='8'))
		{
		   if (index == 0xFFFFFFFF)
		      index = 0;
		   index = (index * 10) + (input_buffer[i] - '0');
		}
		else
		   break;
	     }

	     if ((index < image_set_table_count) && (set_file_use_table[index] == 0))
	     {
		for (i=0; i < image_set_table_count; i++)
		   set_file_use_table[i] = 0;

		image_table_count = 0;
		set_file_use_table[index] = TRUE;
		process_set_file(&set_file_name_table[index][0]);
		NWFSCopy(&ImageSetName[0],&set_file_name_table[index][0], 12);
		break;
	     }
	     else
	     {
		if (index != 0xFFFFFFFF)
		{
		    NWFSPrint("       *********  Invalid Image ID **********\n");
		    WaitForKey();
		}
	     }
	     break;

	  case '2':
	     display_base += 8;
	     if (display_base >= image_set_table_count)
		display_base = 0;
	     break;

	  case '3':
	     ClearScreen(consoleHandle);
	     return;
       }
    }

}

ULONG RestoreNetWareVolume(IMAGE *image)
{
    register ULONG ccode, index, len, current, i;
    register ULONG vindex, dataOffset, vsize, total;
    register FILE *dir = 0, *ext = 0, *data = 0;
    register BYTE *cBuffer = 0;
    register DOS *dos = 0;
    register MACINTOSH *mac = 0;
    register ROOT *root = 0, *rootWork;
    register VOLUME *volume;
    LONGLONG Dir1Size, Dir2Size;
    ULONG Chain1, Chain2, retCode, DirTotal;
    BYTE Buffer[20] ={ "" };
    BYTE DirectoryName[20] = { "" };
    BYTE ExtDirectoryName[20] = { "" };
    BYTE DataName[20] = { "" };
    register ULONG ndx = 0;
    BYTE widget[4] = "\\|/-";
    ULONG totalFiles = 0;
    LONGLONG TotalSize = 0;
    LONGLONG DataSize;

    len = strlen(image->VolumeFile);
    if (len <= 4)
       return -1;

    NWFSCopy(DirectoryName, image->VolumeFile, len);
    NWFSCopy(ExtDirectoryName, image->VolumeFile, len);
    NWFSCopy(DataName, image->VolumeFile, len);

    DirectoryName[len - 3] = '\0';
    strcat(DirectoryName, "DIR");
    ExtDirectoryName[len - 3] = '\0';
    strcat(ExtDirectoryName, "EXT");
    DataName[len - 3] = '\0';
    strcat(DataName, "DAT");

    volume = GetVolumeHandle(&image->VolumeName[1]);
    if (!volume)
    {
       NWFSPrint("Could Find Volume [%s]\n", &image->VolumeName[1]);
       goto ReturnError;
    }

    if (image->AllocatedVolumeClusters > volume->VolumeClusters)
    {
       NWFSPrint("Volume [%s] Too Small (%u clusters) For Image Set\n",
		 volume->VolumeName,
		 (unsigned int)volume->VolumeClusters);
       NWFSPrint("%u Clusters Required For Image Set [%s]\n",
		 (unsigned int)image->AllocatedVolumeClusters,
		 image->ImageSetName);
       goto ReturnError;
    }

    retCode = MountUtilityVolume(volume);
    if (retCode)
    {
       NWFSPrint("error mounting volume %s\n", volume->VolumeName);
       goto ReturnError;
    }

    NWFSPrint("Restoring Volume [%15s]\n",  &image->VolumeName[1]);

    dir = fopen(DirectoryName, "rb+wb");
    if (!dir)
    {
       NWFSPrint("Volume Data Set [%s] not found\n", DirectoryName);
       goto ReturnErrorDismountUtil;
    }
    NWFSPrint("Directory Catalog Opened   ... %s\n", DirectoryName);

    ext = fopen(ExtDirectoryName, "rb+wb");
    if (!ext)
    {
       NWFSPrint("Volume Data Set [%s] not found\n", ExtDirectoryName);
       goto ReturnErrorDismountUtil;
    }
    NWFSPrint("Extended Directory Opened  ... %s\n", ExtDirectoryName);

    data = fopen(DataName, "rb");
    if (!data)
    {
       NWFSPrint("Volume Data Set [%s] not found\n", DataName);
       goto ReturnErrorDismountUtil;
    }
    NWFSPrint("Data Archive Opened        ... %s\n\n", DataName);

    DataSize = (LONGLONG)image->AllocatedVolumeClusters *
	       (LONGLONG)image->VolumeClusterSize;

    dos = (DOS *) malloc(sizeof(DOS));
    if (!dos)
    {
       NWFSPrint("error allocating directory workspace\n");
       goto ReturnErrorDismountUtil;
    }

    cBuffer = malloc(volume->ClusterSize);
    if (!cBuffer)
    {
       NWFSPrint("error allocating data workspace\n");
       goto ReturnErrorDismountUtil;
    }

    root = malloc(sizeof(ROOT));
    if (!root)
    {
       NWFSPrint("error allocating data workspace\n");
       goto ReturnErrorDismountUtil;
    }

    // check the volume directory chain heads
    Chain1 = volume->FirstDirectory;
    Chain2 = volume->SecondDirectory;
    if (!Chain1 || !Chain2)
    {
       NWFSPrint("volume directory chains are NULL\n");
       goto ReturnErrorDismountUtil;
    }

    // truncate the first directory to a single cluster
    ccode = TruncateClusterChain(volume, &Chain1, 0, 0,
				 volume->ClusterSize, 0, 0);
    if (ccode)
    {
       NWFSPrint("error truncating volume directory file (primary)\n");
       goto ReturnErrorDismountUtil;
    }

    // truncate the second directory to a single cluster
    ccode = TruncateClusterChain(volume, &Chain2, 0, 0,
				 volume->ClusterSize, 0, 0);
    if (ccode)
    {
       NWFSPrint("error truncating volume directory file (mirror)\n");
       goto ReturnErrorDismountUtil;
    }

    // restore the directory file
    NWFSPrint("Restoring Volume Directory ... \n");

    // zap this volumes in-memory suballoc records since we will
    // recreate them.
    volume->VolumeSuballocRoot = 0;
    volume->SuballocCount = 0;
    volume->SuballocChainComplete = 0;
    for (i=0; i < 5; i++)
       volume->SuballocChainFlag[i] = 0;

    volume->DirectoryCount = 0;
    volume->FreeDirectoryCount = 0;
    volume->FreeDirectoryBlockCount = 0;
    volume->DeletedDirNo = 0;

    volume->ExtDirList = 0;
    volume->ExtDirTotalBlocks = 0;
    volume->ExtDirSearchIndex = 0;
    for (i=0; i < 128; i++)
    {
       volume->SuballocTotalBlocks[i] = 0;
       volume->SuballocAssignedBlocks[i] = 0;
       volume->SuballocTurboFATCluster[i] = 0;
       volume->SuballocTurboFATIndex[i] = 0;
       volume->SuballocSearchIndex[i] = 0;
    }
    for (i=0; i < MAX_SEGMENTS; i++)
       volume->LastAllocatedIndex[i] = 0;
    
    total = index = 0;
    fseek(dir, 0L, SEEK_SET);
    while (TRUE)
    {
       if (feof(dir))
       {
	  NWFSPrint("\n");
	  break;
       }

       current = ftell(dir);
       ccode = fread(dos, sizeof(DOS), 1, dir);
       if (!ccode)
       {
	  NWFSPrint("\n");
	  break;
       }

       // update the volume flags to match the flag settings in the
       // directory file root entry.

       if ((dos->Subdirectory == ROOT_NODE) &&
	   (dos->NameSpace == DOS_NAME_SPACE))
       {
	  rootWork = (ROOT *) dos;
	  volume->VolumeFlags = rootWork->VolumeFlags;
	  rootWork->ExtendedDirectoryChain0 = 0;
	  rootWork->ExtendedDirectoryChain1 = 0;
       }

       // write first copy of directory
       ccode = NWWriteFile(volume,
			   &Chain1,
			   0,
			   index * sizeof(DOS),
			   (BYTE *)dos,
			   sizeof(DOS),
			   0,
			   0,
			   &retCode,
			   KERNEL_ADDRESS_SPACE,
			   0,
			   0);
       if ((ccode != sizeof(DOS)) || retCode)
       {
	  NWFSPrint("\nerror writing volume directory file\n");
	  goto ReturnErrorDismountUtil;
       }

       // write second copy of directory
       ccode = NWWriteFile(volume,
			   &Chain2,
			   0,
			   index * sizeof(DOS),
			   (BYTE *)dos,
			   sizeof(DOS),
			   0,
			   0,
			   &retCode,
			   KERNEL_ADDRESS_SPACE,
			   0,
			   0);

       if ((ccode != sizeof(DOS)) || retCode)
       {
	  NWFSPrint("\nerror writing volume directory file rc-%d ccode-%d\n",
		    (int)retCode, (int)ccode);
	  goto ReturnErrorDismountUtil;
       }
       // advance to next record
       index++;
    }


    if ((Chain1 != volume->FirstDirectory) ||
	(Chain2 != volume->SecondDirectory))
    {
       NWFSPrint("chain error while writing directory file\n");
       goto ReturnErrorDismountUtil;
    }

    // restore the extended directory file
    // read root directory;

    fseek(dir, 0L, SEEK_SET);
    ccode = fread(root, sizeof(ROOT), 1, dir);
    if (!ccode)
    {
       NWFSPrint("could not read root entry in catalog file\n");
       goto ReturnErrorDismountUtil;
    }

    index = 0;
    Chain1 = Chain2 = (ULONG) -1;
    fseek(ext, 0L, SEEK_SET);
    while (TRUE)
    {
       NWFSPrint("\rWriting Extended Directory ... %c\r", widget[ndx++ & 3]);

       if (feof(ext))
       {
	  NWFSPrint("\n");
	  break;
       }

       current = ftell(ext);
       ccode = fread(dos, sizeof(DOS), 1, ext);
       if (!ccode)
       {
	  NWFSPrint("\n");
	  break;
       }

       // write first copy of directory
       ccode = NWWriteFile(volume,
			     &Chain1,
			     0,
			     index * sizeof(DOS),
			     (BYTE *)dos,
			     sizeof(DOS),
			     0,
			     0,
			     &retCode,
			     KERNEL_ADDRESS_SPACE,
			     0,
			     0);
       if ((ccode != sizeof(DOS)) || retCode)
       {
	  NWFSPrint("error writing extended directory file\n");
	  goto ReturnErrorDismountUtil;
       }

       // write second copy of directory
       ccode = NWWriteFile(volume,
			     &Chain2,
			     0,
			     index * sizeof(DOS),
			     (BYTE *)dos,
			     sizeof(DOS),
			     0,
			     0,
			     &retCode,
			     KERNEL_ADDRESS_SPACE,
			     0,
			     0);
       if ((ccode != sizeof(DOS)) || retCode)
       {
	  NWFSPrint("error writing extended directory file\n");
	  goto ReturnErrorDismountUtil;
       }

       // advance to next record
       index++;
    }

    if (Chain1 == (ULONG) -1)
       root->ExtendedDirectoryChain0 = 0;
    else
       root->ExtendedDirectoryChain0 = Chain1;

    if (Chain2 == (ULONG) -1)
       root->ExtendedDirectoryChain1 = 0;
    else
       root->ExtendedDirectoryChain1 = Chain2;
    root->SubAllocationList = 0;

    // write root entry in the first copy of directory
    ccode = NWWriteFile(volume,
			 &volume->FirstDirectory,
			 0,
			 0,
			 (BYTE *)root,
			 sizeof(ROOT),
			 0,
			 0,
			 &retCode,
			 KERNEL_ADDRESS_SPACE,
			 0,
			 0);
    if ((ccode != sizeof(ROOT)) || retCode)
    {
       NWFSPrint("error writing volume directory file\n");
       goto ReturnErrorDismountUtil;
    }

    // write root entry in the second copy of directory
    ccode = NWWriteFile(volume,
			 &volume->SecondDirectory,
			 0,
			 0,
			 (BYTE *)root,
			 sizeof(ROOT),
			 0,
			 0,
			 &retCode,
			 KERNEL_ADDRESS_SPACE,
			 0,
			 0);
    if ((ccode != sizeof(ROOT)) || retCode)
    {
       NWFSPrint("error writing volume directory file\n");
       goto ReturnErrorDismountUtil;
    }

    // now dismount and remount the volume to perform the
    // file restore.

    DismountUtilityVolume(volume);

    //
    // now we begin the actual file restore
    //

    retCode = MountUtilityVolume(volume);
    if (retCode)
    {
       NWFSPrint("error mounting volume %s\n", volume->VolumeName);
       goto ReturnError;
    }

    Dir1Size = GetChainSize(volume, volume->FirstDirectory);
    Dir2Size = GetChainSize(volume, volume->SecondDirectory);

    if (!Dir1Size || !Dir2Size || (Dir1Size != Dir2Size))
    {
       NWFSPrint("primary and mirror directory files are mismatched\n");
       goto ReturnErrorDismountUtil;
    }

    DirTotal = Dir1Size / sizeof(ROOT);
    for (total = index = i = 0; i < DirTotal; i++)
    {
       ccode = ReadDirectoryRecord(volume, dos, i);
       if (ccode)
       {
	  NWFSPrint("nwfs:  error reading volume directory\n");
	  goto ReturnErrorDismountUtil;
       }

       if ((dos->Subdirectory == (ULONG)ROOT_NODE)        ||
	   (dos->Subdirectory == (ULONG)FREE_NODE)        ||
	   (dos->Subdirectory == (ULONG)TRUSTEE_NODE)     ||
	   (dos->Subdirectory == (ULONG)RESTRICTION_NODE) ||
	   (dos->Subdirectory == (ULONG)SUBALLOC_NODE)    ||
	   (dos->Flags & SUBDIRECTORY_FILE))
       {
	  dos = dos;
       }
       else
       if (dos->NameSpace == DOS_NAME_SPACE)
       {
	  register long bytesLeft;

	  NWFSCopy(Buffer, dos->FileName, (dos->FileNameLength <= 12)
		   ? dos->FileNameLength : 12);
	  Buffer[(dos->FileNameLength <= 12) ? dos->FileNameLength : 12] = '\0';

	  TotalSize += dos->FileSize;
	  totalFiles++;

	  if (dos->FileNameLength)
	  {
	     NWFSPrint("Processing [%12s] %10d bytes  Files Processed-%d  %d%% Complete\n",
		    Buffer,
		    (int)dos->FileSize,
		    (int)totalFiles,
		    (ULONG)(((TotalSize * 100) / DataSize) > 100)
		    ? (int)100 : (int)((TotalSize * 100) / DataSize));
	  }

	  if (dos->FileSize)
	  {
	     dataOffset = (dos->FirstBlock << 4);
	     bytesLeft = dos->FileSize;
	     dos->FirstBlock = (ULONG) -1;

	     ccode = fseek(data, dataOffset, SEEK_SET);
	     if (ccode == (ULONG) -1)
	     {
		NWFSPrint("range error searching data catalog\n");
		goto ReturnErrorDismountUtil;
	     }

	     vindex = 0;
	     while (bytesLeft > 0)
	     {
		if (feof(data))
		{
		   NWFSPrint("EOF error in data catalog\n");
		   goto ReturnErrorDismountUtil;
		}

		vsize = ((bytesLeft > (long)IO_BLOCK_SIZE)
			 ? IO_BLOCK_SIZE : bytesLeft);

		ccode = fread(cBuffer, vsize, 1, data);
		if (!ccode)
		{
		   NWFSPrint("read error in data catalog\n");
		   goto ReturnErrorDismountUtil;
		}

		// write first copy of directory
		ccode = NWWriteFile(volume,
				 &dos->FirstBlock,
				 0,
				 vindex * IO_BLOCK_SIZE,
				 cBuffer,
				 vsize,
				 0,
				 0,
				 &retCode,
				 KERNEL_ADDRESS_SPACE,
				 0,
				 0);
		if ((ccode != vsize) || retCode)
		{
		   NWFSPrint("\nerror writing volume file %s\n", Buffer);
		   goto ReturnErrorDismountUtil;
		}

		vindex++;
		bytesLeft -= vsize;
		total += vsize;
	     }

	     // suballocate this file
	     ccode = TruncateClusterChain(volume,
					  &dos->FirstBlock,
					  0,
					  0,
					  dos->FileSize,
					  ((volume->VolumeFlags &
					  SUB_ALLOCATION_ON) ? 1 : 0),
					  dos->FileAttributes);
	     if (ccode)
	     {
		NWFSPrint("error truncating file %s\n", Buffer);
		goto ReturnErrorDismountUtil;
	     }
	  }
	  else
	     dos->FirstBlock = (ULONG) -1;

	  ccode = WriteDirectoryRecord(volume, dos, i);
	  if (ccode)
	  {
	     NWFSPrint("nwfs:  error writing volume directory\n");
	     goto ReturnErrorDismountUtil;
	  }
       }
       else
       if (dos->NameSpace == MAC_NAME_SPACE)
       {
	  register long bytesLeft;

	  mac = (MACINTOSH *) dos;
	  NWFSCopy(Buffer, mac->FileName, (mac->FileNameLength <= 12)
		   ? mac->FileNameLength : 12);
	  Buffer[(mac->FileNameLength <= 12) ? mac->FileNameLength : 12] = '\0';

	  TotalSize += mac->ResourceForkSize;
	  totalFiles++;

	  if (mac->ResourceForkSize)
	  {
	     dataOffset = (mac->ResourceFork << 4);
	     bytesLeft = mac->ResourceForkSize;
	     mac->ResourceFork = (ULONG) -1;

	     ccode = fseek(data, dataOffset, SEEK_SET);
	     if (ccode == (ULONG) -1)
	     {
		NWFSPrint("range error searching data catalog\n");
		goto ReturnErrorDismountUtil;
	     }

	     vindex = 0;
	     while (bytesLeft > 0)
	     {
		if (feof(data))
		{
		   NWFSPrint("EOF error in data catalog\n");
		   goto ReturnErrorDismountUtil;
		}

		vsize = ((bytesLeft > (long)IO_BLOCK_SIZE)
			 ? IO_BLOCK_SIZE : bytesLeft);

		ccode = fread(cBuffer, vsize, 1, data);
		if (!ccode)
		{
		   NWFSPrint("read error in data catalog\n");
		   goto ReturnErrorDismountUtil;
		}

		// write first copy of directory
		ccode = NWWriteFile(volume,
				    &mac->ResourceFork,
				    0,
				    vindex * IO_BLOCK_SIZE,
				    cBuffer,
				    vsize,
				    0,
				    0,
				    &retCode,
				    KERNEL_ADDRESS_SPACE,
				    0,
				    0);
		if ((ccode != vsize) || retCode)
		{
		   NWFSPrint("\nerror writing volume file %s\n", Buffer);
		   goto ReturnErrorDismountUtil;
		}
		vindex++;
		bytesLeft -= vsize;
		total += vsize;
	     }

	     // suballocate this file
	     ccode = TruncateClusterChain(volume,
					  &mac->ResourceFork,
					  0,
					  0,
					  mac->ResourceForkSize,
					  ((volume->VolumeFlags &
					  SUB_ALLOCATION_ON) ? 1 : 0),
					  0);
	     if (ccode)
	     {
		NWFSPrint("error truncating file %s\n", Buffer);
		goto ReturnErrorDismountUtil;
	     }
	  }
	  else
	     mac->ResourceFork = (ULONG) -1;

	  ccode = WriteDirectoryRecord(volume, (DOS *)mac, i);
	  if (ccode)
	  {
	     NWFSPrint("nwfs:  error writing volume directory\n");
	     goto ReturnErrorDismountUtil;
	  }
       }
    }

    NWFSPrint("\ntotal written to volume [%12d] bytes\n", (int)total);

    DismountUtilityVolume(volume);

    if (root)
       free(root);
    if (cBuffer)
       free(cBuffer);
    if (dos)
       free(dos);
    if (dir)
       fclose(dir);
    if (ext)
       fclose(ext);
    if (data)
       fclose(data);

    SyncDisks();

    return 0;

ReturnErrorDismountUtil:;
     DismountUtilityVolume(volume);

ReturnError:;

    if (root)
       free(root);
    if (cBuffer)
       free(cBuffer);
    if (dos)
       free(dos);
    if (dir)
       fclose(dir);
    if (ext)
       fclose(ext);
    if (data)
       fclose(data);

    SyncDisks();

    WaitForKey();

    return (ULONG) -1;
}

int main(int argc, char *argv[])
{
    register ULONG i, j, k;
    register ULONG found_flag;
    register ULONG index;
    register ULONG display_base = 0;
    register long clusters;
    register FILE *vfd;

    ULONG vpart_handle;
    ULONG handles[7];
    BYTE input_buffer[20] = {""};
    BYTE *bptr;
    BYTE case_key;
    nwvp_payload payload;
    nwvp_vpartition_info vpart_info;
    vpartition_info volume_info;

#if (DOS_UTIL)
    struct find_t f;
    register ULONG retCode;

    retCode = _dos_findfirst("*.SET", _A_NORMAL, &f);
    if (!retCode)
    {
       if (image_set_table_count < IMAGE_SET_TABLE_SIZE)
       {
	  if ((vfd = fopen(f.name, "rb+wb")) == 0)
	     NWFSPrint(" ERROR opening %s \n", f.name);
	  else
	  {
	     if (fread(&image_set_table[image_set_table_count], sizeof(IMAGE_SET), 1, vfd) != 0)
	     {
		if (image_set_table[image_set_table_count].Stamp == IMAGE_SET_STAMP)
		{
		   for (k=0; k < 12; k++)
		   {
		      if (f.name[k] == 0)
			 break;

		      if ((f.name[k] >= 'a') && (f.name[k] <= 'z'))
			 set_file_name_table[image_set_table_count][k] = (f.name[k] - 'a') + 'A';
		      else
			 set_file_name_table[image_set_table_count][k] = f.name[k];
		   }
		   image_set_table_count++;
		}
		else
		   NWFSPrint(" Invalid stamp in file %s \n", f.name);
	     }
	     else
		NWFSPrint(" Invalid stamp in file %s \n", f.name);
	     fclose(vfd);
	  }
       }

       while (!_dos_findnext(&f))
       {
	  for (i=0; i < 8; i++)
	  {
	     if ((f.name[i+1] == '.') &&
		((f.name[i+2] == 'S') || (f.name[i+2] == 's')) &&
		((f.name[i+3] == 'E') || (f.name[i+3] == 'e')) &&
		((f.name[i+4] == 'T') || (f.name[i+4] == 't')))
	     {

		if (image_set_table_count >= IMAGE_SET_TABLE_SIZE)
		   break;

		if ((vfd = fopen(f.name, "rb+wb")) == 0)
		   NWFSPrint(" ERROR opening %s \n", f.name);
		else
		{
		   if (fread(&image_set_table[image_set_table_count], sizeof(IMAGE_SET), 1, vfd) != 0)
		   {
		      if (image_set_table[image_set_table_count].Stamp == IMAGE_SET_STAMP)
		      {
			 for (k=0; k < 12; k++)
			 {
			    if (f.name[k] == 0)
			       break;

			    if ((f.name[k] >= 'a') && (f.name[k] <= 'z'))
			       set_file_name_table[image_set_table_count][k] = (f.name[k] - 'a') + 'A';
			    else
			       set_file_name_table[image_set_table_count][k] = f.name[k];
			 }
			 image_set_table_count++;
		      }
		      else
			 NWFSPrint(" Invalid stamp in file %s \n", f.name);
		   }
		   else
		       NWFSPrint(" Invalid stamp in file %s \n", f.name);
		   fclose(vfd);
		}
	     }
	  }
       }
    }
#endif


#if (WINDOWS_NT_UTIL)
    HANDLE handle;
    WIN32_FIND_DATA findData;

    handle = FindFirstFile("*.SET", &findData);
    if (handle != INVALID_HANDLE_VALUE)
    {
       if (image_set_table_count < IMAGE_SET_TABLE_SIZE)
       {
	  if ((vfd = fopen(findData.cFileName, "rb+wb")) == 0)
	     NWFSPrint(" ERROR opening %s \n", findData.cFileName);
	  else
	  {
	     if (fread(&image_set_table[image_set_table_count], sizeof(IMAGE_SET), 1, vfd) != 0)
	     {
		if (image_set_table[image_set_table_count].Stamp == IMAGE_SET_STAMP)
		{
		   for (k=0; k < 12; k++)
		   {
		      if (findData.cFileName[k] == 0)
			 break;

		      if ((findData.cFileName[k] >= 'a') && (findData.cFileName[k] <= 'z'))
			 set_file_name_table[image_set_table_count][k] = (findData.cFileName[k] - 'a') + 'A';
		      else
			 set_file_name_table[image_set_table_count][k] = findData.cFileName[k];
		   }
		   image_set_table_count++;
		}
		else
		    NWFSPrint(" Invalid stamp in file %s \n", findData.cFileName);
	     }
	     else
		NWFSPrint(" Invalid stamp in file %s \n", findData.cFileName);
	     fclose(vfd);
	  }
       }

       while (FindNextFile(handle, &findData))
       {
	  for (i=0; i < 8; i++)
	  {
	     if ((findData.cFileName[i+1] == '.') &&
		((findData.cFileName[i+2] == 'S') || (findData.cFileName[i+2] == 's')) &&
		((findData.cFileName[i+3] == 'E') || (findData.cFileName[i+3] == 'e')) &&
		((findData.cFileName[i+4] == 'T') || (findData.cFileName[i+4] == 't')))
	     {
		if (image_set_table_count >= IMAGE_SET_TABLE_SIZE)
		   break;

		if ((vfd = fopen(findData.cFileName, "rb+wb")) == 0)
		   NWFSPrint(" ERROR opening %s \n", findData.cFileName);
		else
		{
		   if (fread(&image_set_table[image_set_table_count], sizeof(IMAGE_SET), 1, vfd) != 0)
		   {
		      if (image_set_table[image_set_table_count].Stamp == IMAGE_SET_STAMP)
		      {
			 for (k=0; k < 12; k++)
			 {
			    if (findData.cFileName[k] == 0)
			       break;

			    if ((findData.cFileName[k] >= 'a') && (findData.cFileName[k] <= 'z'))
			       set_file_name_table[image_set_table_count][k] = (findData.cFileName[k] - 'a') + 'A';
			    else
			       set_file_name_table[image_set_table_count][k] = findData.cFileName[k];
			 }
			 image_set_table_count++;
		      }
		      else
			 NWFSPrint(" Invalid stamp in file %s \n", findData.cFileName);
		   }
		   else
		      NWFSPrint(" Invalid stamp in file %s \n", findData.cFileName);
		   fclose(vfd);
		}
	     }
	  }
       }
       FindClose(handle);
    }
#endif


#if (LINUX_UTIL)
    register struct dirent *entry;
    DIR *local_dir;

    if ((local_dir = opendir("./")) == NULL)
    {
       NWFSPrint("open dir failed NULL\n");
       return 1;
    }

    while ((entry = readdir(local_dir)) != NULL)
    {
       if (image_set_table_count >= IMAGE_SET_TABLE_SIZE)
	  break;

       for (i=0; i < 8; i++)
       {
	  if ((entry->d_name[i+1] == '.') &&
	     ((entry->d_name[i+2] == 'S') || (entry->d_name[i+2] == 's')) &&
	     ((entry->d_name[i+3] == 'E') || (entry->d_name[i+3] == 'e')) &&
	     ((entry->d_name[i+4] == 'T') || (entry->d_name[i+4] == 't')))
	  {
	     if ((vfd = fopen(&entry->d_name[0], "rb+wb")) == 0)
		NWFSPrint(" ERROR opening %s \n", &entry->d_name[0]);
	     else
	     {
		if (fread(&image_set_table[image_set_table_count], sizeof(IMAGE_SET), 1, vfd) != 0)
		{
		   if (image_set_table[image_set_table_count].Stamp == IMAGE_SET_STAMP)
		   {
		      for (k=0; k < 12; k++)
		      {
			 if (entry->d_name[k] == 0)
			    break;

			 if ((entry->d_name[k] >= 'a') && (entry->d_name[k] <= 'z'))
			    set_file_name_table[image_set_table_count][k] = (entry->d_name[k] - 'a') + 'A';
			 else
			    set_file_name_table[image_set_table_count][k] = entry->d_name[k];
		      }
		      image_set_table_count++;
		   }
		   else
		      NWFSPrint(" Invalid stamp in file %s \n", &entry->d_name[0]);
		}
		else
		   NWFSPrint(" Invalid stamp in file %s \n", &entry->d_name[0]);
		fclose(vfd);
	     }
	     break;
	  }
       }
    }
    closedir(local_dir);
#endif

    InitNWFS();
    UtilScanVolumes();
    for (i=0; i < TotalDisks; i++)
       SyncDevice(i);

    if (argc > 1)
    {
       for (i=0; i < 8; i++)
       {
	  if ((argv[1][i] >= 'a') && (argv[1][i] <= 'z'))
	     ImageSetName[i] = (argv[1][i] - 'a') + 'A';
	  else
	  if ((argv[1][i] >= 'a') && (argv[1][i] <= 'z'))
	     ImageSetName[i] = argv[1][i];
	  else
	  if ((argv[1][i] >= '0') && (argv[1][i] <= '9'))
	     ImageSetName[i] = argv[1][i];
	  else
	     break;
       }
       ImageSetName[i] = '.';
       ImageSetName[i+1] = 'S';
       ImageSetName[i+2] = 'E';
       ImageSetName[i+3] = 'T';
       ImageSetName[i+4] = 0;
    }

    for (i=0; i < image_set_table_count; i++)
    {
       for (k=0; k < 12; k++)
       {
	  if (ImageSetName[k] == 0)
	     break;

	  if (ImageSetName[k] != set_file_name_table[i][k])
	     break;
       }

       if (ImageSetName[k] == 0)
       {
	  set_file_use_table[i] = TRUE;
	  process_set_file(&ImageSetName[0]);
	  break;
       }
    }

#if (LINUX_UTIL && LINUX_DEV)
    devfd = open("/dev/nwfs", O_RDWR);
    if (devfd < 0)
    {
       printf("Error opening /dev/nwfs device - some operations disabled\n");
       printf("press a key to continue ...\n");
       WaitForKey();
    }
#endif
    
    while (TRUE)
    {
       ClearScreen(consoleHandle);
       for (i=0; i < image_table_count; i++)
       {
	  payload.payload_object_count = 0;
	  payload.payload_index = 0;
	  payload.payload_object_size_buffer_size = (7 << 20) + sizeof(handles);
	  payload.payload_buffer = (BYTE *) &handles[0];
	  found_flag = 0;
	  do
	  {
	     nwvp_vpartition_scan(&payload);
	     for (j=0; j < payload.payload_object_count; j++)
	     {
		if (nwvp_vpartition_return_info(handles[j], &vpart_info) == 0)
		{
		   if (NWFSCompare(&vpart_info.volume_name[0], &target_volume_table[i][0], vpart_info.volume_name[0] + 1) == 0)
		   {
		      found_flag = 0xFFFFFFFF;
		      break;
		   }
		}
	     }
	  } while ((payload.payload_index != 0) && (payload.payload_object_count != 0) && (found_flag == 0));

	  if (found_flag == 0)
	     NWFSSet(&target_volume_table[i][0], 0, 16);
       }

       NWFSPrint("NetWare Volume Restore Utility\n");
       NWFSPrint("Copyright (c) 1999, 1998 JVM, Inc.  All Rights Reserved.\n");
       NWFSPrint("Portions Copyright (c) Lintell, Inc.\n");
       NWFSPrint(" Image Set: %s\n", &ImageSetName[0]);
       NWFSPrint("\n Auto #    Image Volume     Total Size  Cluster  Segs Mirror   Target Volume\n\n");

       for (i=0; i < 8; i++)
       {
	  if ((index = display_base + i) < image_table_count)
	  {
	     NWFSPrint(" %s %2d [%16s]  %6u MB     %2d K    %2d  %s   %s\n",
			(mark_table[index] == 0) ? "   " : "***",
			(int)index,
			&image_table[index].VolumeName[1],
			(unsigned int)
			   (((LONGLONG)image_table[index].LogicalVolumeClusters *
			     (LONGLONG)image_table[index].VolumeClusterSize) /
			     (LONGLONG)0x100000),
			(int)(image_table[index].VolumeClusterSize / 1024),
			(int)image_table[index].SegmentCount,
			(image_table[index].MirrorFlag == 0) ? "SINGLE" : "MIRROR",
			&target_volume_table[index][1]);
	  }
	  else
	     NWFSPrint("\n");
       }

       NWFSPrint("               1. Restore Marked Volumes\n");
       NWFSPrint("               2. Select Image Set\n");
       NWFSPrint("               3. Edit Volume Configuration\n");
       NWFSPrint("               4. Assign or Unassign Image To Volume\n");
       NWFSPrint("               5. Auto Create Mark or Unmark\n");
       NWFSPrint("               6. Scroll Volumes\n");
       NWFSPrint("               7. Exit NWRestore\n");
       NWFSPrint("        Enter a Menu Option: ");

       fgets(input_buffer, sizeof(input_buffer), stdin);
       case_key = input_buffer[0];
       switch(case_key)
       {
	  case '1':
	     ClearScreen(0);
	     NWAllocateUnpartitionedSpace();

	     for (index = 0; index < image_table_count; index ++)
	     {
		while (mark_table[index] != 0)
		{
		   mark_table[index] = 0;
		   NWFSCopy(&target_volume_table[index][0], &image_table[index].VolumeName[0], 16);
		   NWFSPrint("Auto creating volume %s \n", &target_volume_table[index][1]);

		   found_flag = 0;
		   for (j=0; j<image_table_count; j++)
		   {
		      if (j != index)
		      {
			 if (NWFSCompare(&target_volume_table[j][0], &target_volume_table[index][0], target_volume_table[index][0] + 1) == 0)
			 {
			    found_flag = 0xFFFFFFFF;
			    break;
			 }
		      }
		   }

		   if (found_flag != 0)
		   {
		      NWFSSet(&target_volume_table[index][0], 0, 16);
		      NWFSPrint("  Volume already is assigned\n");

		      if (YesOrNo("Do you want to rename target volume (Y or N) ?") != 0)
		      {
			 NWFSPrint("Skipping Auto Create for image volume %s \n", &image_table[index].VolumeName[1]);
			 break;
		      }

		      NWFSPrint("   Enter New Target Volume Name: ");
		      fgets(input_buffer, sizeof(input_buffer), stdin);
		      NWFSSet(&target_volume_table[index][0], 0, 16);

		      bptr = input_buffer;
		      while (bptr[0] == ' ')
			 bptr ++;

		      while ((*bptr != 0) && (*bptr != 0xA) && (*bptr != 0xD) && (*bptr != ' '))
		      {
			 if ((bptr[0] >= 'a') && (bptr[0] <= 'z'))
			    bptr[0] = (bptr[0] - 'a') + 'A';
			 target_volume_table[index][target_volume_table[index][0] + 1] = bptr[0];
			 target_volume_table[index][0] ++;
			 if (target_volume_table[index][0] == 15)
			    break;
			 bptr++;
		      }

		      if (target_volume_table[index][0] == 0)
		      {
			 NWFSPrint("  Invalid Name\n");
			 NWFSPrint("Skipping Auto Create for image volume %s \n", &image_table[index].VolumeName[1]);
			 break;
		      }
		   }

		   // attempt to auto-create a volume the same size as the
		   // original volume specified in the image file.

		   NWFSCopy(&volume_info.volume_name[0], &target_volume_table[index][0], 16);
		   volume_info.blocks_per_cluster = image_table[index].VolumeClusterSize / 4096;
		   volume_info.cluster_count = image_table[index].LogicalVolumeClusters;
		   volume_info.segment_count = image_table[index].SegmentCount;
		   volume_info.flags = (SUB_ALLOCATION_ON | NDS_FLAG | NEW_TRUSTEE_COUNT);

		   if (nwvp_volume_alloc_space_create(&vpart_handle, &volume_info, (image_table[index].MirrorFlag == 0) ? 0 : 1) == 0)
		   {
		      UtilScanVolumes();
		   }
		   else
		   {
		      // retry volume create operation in decrements of 10
		      // clusters until we succeed or we are equal to
		      // the amount of allocated clusters required to store
		      // the file archive data itself.

		      clusters = image_table[index].LogicalVolumeClusters;
		      while (TRUE)
		      {
			 clusters -= 10;
			 if (clusters < (long)image_table[index].AllocatedVolumeClusters)
			    break;

			 NWFSCopy(&volume_info.volume_name[0], &target_volume_table[index][0], 16);
			 volume_info.blocks_per_cluster = image_table[index].VolumeClusterSize / 4096;
			 volume_info.cluster_count = clusters;
			 volume_info.segment_count = image_table[index].SegmentCount;
			 volume_info.flags = (SUB_ALLOCATION_ON | NDS_FLAG | NEW_TRUSTEE_COUNT);

			 if (nwvp_volume_alloc_space_create(&vpart_handle, &volume_info, (image_table[index].MirrorFlag == 0) ? 0 : 1) == 0)
			 {
			    UtilScanVolumes();
			    goto VolumeCreated;
			 }
		      }

		      NWFSPrint("Could not create volume [%s]\n", &target_volume_table[index][1]);
		      NWFSPrint("No Free Space, Part/Volume Size Mismatch, or Volume Exists\n");
		      NWFSSet(&target_volume_table[index][0], 0, 16);
VolumeCreated:;
		   }
		}
	     }

	     for (i=0; i < image_table_count; i++)
	     {
		if (target_volume_table[i][0] != 0)
		{
		   NWFSCopy(&image_table[i].VolumeName[0], &target_volume_table[i][0], 16);
		   RestoreNetWareVolume(&image_table[i]);
		}
	     }

	     WaitForKey();
	     break;

	  case '2':
	     select_data_set();
	     break;

	  case '3':
	     NWEditVolumes();
	     UtilScanVolumes();
	     break;

	  case '4':
	     NWFSPrint("        Enter Volume Number: ");
	     fgets(input_buffer, sizeof(input_buffer), stdin);
	     index = 0xFFFFFFFF;

	     for (i=0; i < 4; i++)
	     {
		if ((input_buffer[i] >= '0') && (input_buffer[i] <='8'))
		{
		   if (index == 0xFFFFFFFF)
		      index = 0;
		   index = (index * 10) + (input_buffer[i] - '0');
		}
		else
		   break;
	     }

	     if (index < image_table_count)
	     {
		if (target_volume_table[index][0] == 0)
		{
		   NWFSPrint("   Enter Target Volume Name: ");
		   fgets(input_buffer, sizeof(input_buffer), stdin);
		   NWFSSet(&target_volume_table[index][0], 0, 16);

		   bptr = input_buffer;
		   while (bptr[0] == ' ')
		      bptr ++;

		   while ((*bptr != 0) && (*bptr != 0xA) && (*bptr != 0xD) && (*bptr != ' '))
		   {
		      if ((bptr[0] >= 'a') && (bptr[0] <= 'z'))
			 bptr[0] = (bptr[0] - 'a') + 'A';
		      target_volume_table[index][target_volume_table[index][0] + 1] = bptr[0];
		      target_volume_table[index][0] ++;

		      if (target_volume_table[index][0] == 15)
			 break;
		      bptr++;
		   }

		   if (target_volume_table[index][0] != 0)
		   {
		      found_flag = 0;
		      for (j=0; j<image_table_count; j++)
		      {
			 if (j != index)
			 {
			    if (NWFSCompare(&target_volume_table[j][0], &target_volume_table[index][0], target_volume_table[index][0] + 1) == 0)
			    {
			       found_flag = 0xFFFFFFFF;
			       break;
			    }
			 }
		      }

		      if (found_flag != 0)
		      {
			 mark_table[index] = 0;
			 NWFSSet(&target_volume_table[index][0], 0, 16);
			 NWFSPrint("       *********  Target Volume Already Assigned **********\n");
			 WaitForKey();
			 break;
		      }

		      payload.payload_object_count = 0;
		      payload.payload_index = 0;
		      payload.payload_object_size_buffer_size = (7 << 20) + sizeof(handles);
		      payload.payload_buffer = (BYTE *) &handles[0];
		      found_flag = 0;
		      do
		      {
			 nwvp_vpartition_scan(&payload);
			 for (j=0; j < payload.payload_object_count; j++)
			 {
			    if (nwvp_vpartition_return_info(handles[j], &vpart_info) == 0)
			    {
			       if (NWFSCompare(&vpart_info.volume_name[0],
                                               &target_volume_table[index][0],
                                               vpart_info.volume_name[0] + 1) == 0)
			       {
				  found_flag = 0xFFFFFFFF;
				  break;
			       }
			    }
			 }
		      } while ((payload.payload_index != 0) && (payload.payload_object_count != 0) && (found_flag == 0));

		      if (found_flag != 0)
		      {
			 mark_table[index] = 0;
			 break;
		      }
		   }

		   NWFSSet(&target_volume_table[index][0], 0, 16);
		   NWFSPrint("       *********  Invalid Volume Name **********\n");
		   WaitForKey();
		}
		else
		{
		   mark_table[index] = 0;
		   NWFSSet(&target_volume_table[index][0], 0, 16);
		}
		break;
	     }
	     else
	     {
		if (index != 0xFFFFFFFF)
		{
		   NWFSPrint("       *********  Invalid Volume ID **********\n");
		   WaitForKey();
		}
	     }
	     break;

	  case '5':
	     NWFSPrint("        Enter Volume Number: ");
	     fgets(input_buffer, sizeof(input_buffer), stdin);
	     index = 0xFFFFFFFF;
	     for (i=0; i < 4; i++)
	     {
		if ((input_buffer[i] >= '0') && (input_buffer[i] <='8'))
		{
		   if (index == 0xFFFFFFFF)
		      index = 0;
		   index = (index * 10) + (input_buffer[i] - '0');
		}
		else
		   break;
	     }

	     if ((index < image_table_count) && (target_volume_table[index][0] == 0))
	     {
		if (valid_table[index] != 0)
		   mark_table[index] ++;
		mark_table[index] &= 0x00000001;
		break;
	     }
	     NWFSPrint("       *********  Invalid Volume ID **********\n");
	     WaitForKey();
	     break;

	  case '6':
	     display_base += 8;
	     if (display_base >= image_table_count)
		display_base = 0;
	     break;

	  case '7':
	     ClearScreen(consoleHandle);
	     goto Shutdown;

       }
    }

Shutdown:;
#if (LINUX_UTIL && LINUX_DEV)
    if (devfd)
       close(devfd);
#endif

    DismountAllVolumes();
    NWFSVolumeClose();
    RemoveDiskDevices();
    FreePartitionResources();
    CloseNWFS();
    ClearScreen(consoleHandle);
    return 0;
}


