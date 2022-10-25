
/***************************************************************************
*
*   Copyright (c) 1997-2001 Jeff V. Merkey
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
*
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  NWIMAGE.C
*   DESCRIP  :  NWFS Menu Image/Restore Utility
*   DATE     :  November 23, 2022
*
*
***************************************************************************/

#include "globals.h"
#include "imghelp.h"

#define NWCONFIG_LIB  1
#include "nwconfig.c"

extern NWSCREEN ConsoleScreen;
ULONG nw_menu = 0, nw_portal = 0;
extern void ScanVolumes(void);

#include "image.h"

#if (LINUX_UTIL | DOS_UTIL)
#include <dirent.h>
#endif

#if (LINUX_UTIL && LINUX_DEV)
extern int devfd;
#endif

extern ULONG DumpDirectoryFile(VOLUME *volume, FILE *fp, ULONG *totalDirs);
extern ULONG DumpExtendedDirectoryFile(VOLUME *volume, FILE *fp);

ULONG image_table_count = 0;
IMAGE image_table[IMAGE_TABLE_SIZE];
ULONG mark_table[IMAGE_TABLE_SIZE];
ULONG valid_table[IMAGE_TABLE_SIZE];
IMAGE_SET set_header;
ULONG SkipDeletedFiles = 0;
ULONG AutoCreatePartitions = 0;
extern ULONG EnableLogging;
BYTE ImageSetName[13] = "IMAGE000.SET";
BYTE RestoreSetName[13] = "IMAGE000.SET";
BYTE NameHeader[80] = "";
IMAGE_SET image_set_table[IMAGE_SET_TABLE_SIZE];

BYTE target_volume_table[IMAGE_TABLE_SIZE][20];
ULONG target_volume_flags[IMAGE_TABLE_SIZE];
BYTE set_file_name_table[IMAGE_SET_TABLE_SIZE][20];
ULONG image_set_table_count = 0;
ULONG set_file_use_table[IMAGE_SET_TABLE_SIZE];

FILE *logfile = 0;
BYTE Path[1056]="";
BYTE WorkPath[1056]="";

void ScanForDataSets(void)
{
    register ULONG i, k;
    BYTE displayBuffer[255];

    image_set_table_count = 0;

#if (DOS_UTIL)
	 {

	 struct find_t f;
	 register ULONG retCode;
	 register FILE *vfd;

	 WorkPath[0] = '\0';
	 strcat(WorkPath, Path);
	 strcat(WorkPath, "*.SET");
	 retCode = _dos_findfirst(WorkPath, _A_NORMAL, &f);
	 if (!retCode)
	 {
	    if (image_set_table_count < IMAGE_SET_TABLE_SIZE)
	    {
	       WorkPath[0] = '\0';
	       strcat(WorkPath, Path);
	       strcat(WorkPath, f.name);
	       if ((vfd = fopen(WorkPath, "rb+wb")) == 0)
	       {
		  log_sprintf(displayBuffer, " ERROR opening %s ", f.name);
		  error_portal(displayBuffer, 10);
	       }
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
		     {
			log_sprintf(displayBuffer, " Invalid stamp in file %s ", f.name);
			error_portal(displayBuffer, 10);
		     }
		  }
		  else
		  {
		     log_sprintf(displayBuffer, " Invalid stamp in file %s ", f.name);
		     error_portal(displayBuffer, 10);
		  }
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

		     WorkPath[0] = '\0';
		     strcat(WorkPath, Path);
		     strcat(WorkPath, f.name);
		     if ((vfd = fopen(WorkPath, "rb+wb")) == 0)
		     {
			log_sprintf(displayBuffer, " ERROR opening %s ", f.name);
			error_portal(displayBuffer, 10);
		     }
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
			   {
			      log_sprintf(displayBuffer, " Invalid stamp in file %s ", f.name);
			      error_portal(displayBuffer, 10);
			   }
			}
			else
			{
			    log_sprintf(displayBuffer, " Invalid stamp in file %s ", f.name);
			    error_portal(displayBuffer, 10);
			}
			fclose(vfd);
		     }
		  }
	       }
	    }
	 }

	 }
#endif

#if (WINDOWS_NT_UTIL)
	 {

	 HANDLE handle;
	 WIN32_FIND_DATA findData;
	 register FILE *vfd;

	 WorkPath[0] = '\0';
	 strcat(WorkPath, Path);
	 strcat(WorkPath, "*.SET");
	 handle = FindFirstFile(WorkPath, &findData);
	 if (handle != INVALID_HANDLE_VALUE)
	 {
	    if (image_set_table_count < IMAGE_SET_TABLE_SIZE)
	    {
	       WorkPath[0] = '\0';
	       strcat(WorkPath, Path);
	       strcat(WorkPath, findData.cFileName);
	       if ((vfd = fopen(WorkPath, "rb+wb")) == 0)
	       {
		  log_sprintf(displayBuffer, " ERROR opening %s ", findData.cFileName);
		  error_portal(displayBuffer, 10);
	       }
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
		     {
			log_sprintf(displayBuffer, " Invalid stamp in file %s ", findData.cFileName);
			error_portal(displayBuffer, 10);
		     }
		  }
		  else
		  {
		     log_sprintf(" Invalid stamp in file %s ", findData.cFileName);
		     error_portal(displayBuffer, 10);
		  }
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

		     WorkPath[0] = '\0';
		     strcat(WorkPath, Path);
		     strcat(WorkPath, findData.cFileName);
		     if ((vfd = fopen(WorkPath, "rb+wb")) == 0)
		     {
			log_sprintf(displayBuffer, " ERROR opening %s ", findData.cFileName);
			error_portal(displayBuffer, 10);
		     }
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
			   {
			      log_sprintf(displayBuffer, " Invalid stamp in file %s ", findData.cFileName);
			      error_portal(displayBuffer, 10);
			   }
			}
			else
			{
			   log_sprintf(displayBuffer, " Invalid stamp in file %s ", findData.cFileName);
			   error_portal(displayBuffer, 10);
			}
			fclose(vfd);
		     }
		  }
	       }
	    }
	    FindClose(handle);
	 }

	 }
#endif

#if (LINUX_UTIL)
	 {

	 register ULONG len;
	 register struct dirent *entry;
	 DIR *local_dir;
	 register FILE *vfd;

	 WorkPath[0] = '\0';
	 strcat(WorkPath, Path);
	 len = strlen(WorkPath);
	 if ((WorkPath[len - 1] == '\\') || (WorkPath[len - 1] == '/'))
	    WorkPath[len - 1] = '\0';

	 len = strlen(WorkPath);
	 if (!len)
	    strcat(WorkPath, "./");

	 if ((local_dir = opendir(WorkPath)) == NULL)
	 {
	    log_sprintf(displayBuffer, "open dir failed NULL (%s)", WorkPath);
	    error_portal(displayBuffer, 10);
	    return;
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
		  WorkPath[0] = '\0';
		  strcat(WorkPath, Path);
		  strcat(WorkPath, &entry->d_name[0]);
		  if ((vfd = fopen(WorkPath, "rb+wb")) == 0)
		  {
		     log_sprintf(displayBuffer, " ERROR opening %s ", &entry->d_name[0]);
		     error_portal(displayBuffer, 10);
		  }
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
			{
			   log_sprintf(displayBuffer, " Invalid stamp in file %s ", &entry->d_name[0]);
			   error_portal(displayBuffer, 10);
			}
		     }
		     else
		     {
			log_sprintf(displayBuffer, " Invalid stamp in file %s ", &entry->d_name[0]);
			error_portal(displayBuffer, 10);
		     }
		     fclose(vfd);
		  }
		  break;
	       }
	    }
	 }
	 closedir(local_dir);

	}
#endif
}

#define NWVP_NOT_ENOUGH_SPACE     -10
#define NWVP_NOT_ENOUGH_SEGMENTS  -20
#define NWVP_TOO_MANY_SEGMENTS    -30

typedef struct _nwvp_free_stats
{
    ULONG total_segments;
    ULONG total_blocks;
    ULONG max_unique_segments;
    ULONG max_unique_blocks;
} nwvp_free_stats;

ULONG nwvp_volume_alloc_space_create(ULONG *vpart_handle,
				     vpartition_info *vpart_info,
				     ULONG mirror_flag,
				     ULONG commit_flag,
				     nwvp_free_stats *stats)
{
    ULONG j, k, l, m;
    ULONG fit_flag = 0;
    ULONG new_space;
    ULONG cluster_count;
    ULONG cluster_needs;
    ULONG segment_size;
    ULONG diff;
    ULONG lowest_count;
    ULONG lowest_index;
    ULONG segment_count;
    nwvp_lpartition_space_return_info slist[5];
    nwvp_lpartition_scan_info llist[5];
    segment_info segment_assign[32];
    nwvp_payload payload;
    nwvp_payload payload1;

    new_space = TRUE;
    NWLockNWVP();

    stats->total_segments = 0;
    stats->total_blocks = 0;
    stats->max_unique_segments = 0;
    stats->max_unique_blocks = 0;

    while (new_space != 0)
    {
       new_space = 0;
       for (k=0; k < 32; k++)
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
	  for (m=0; m < payload1.payload_object_count; m++)
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

		for (j=0; j < payload.payload_object_count; j++)
		{
		   if ((slist[j].status_bits & INUSE_BIT) == 0)
		   {
		      if ((((slist[j].status_bits & MIRROR_BIT) == 0) && (mirror_flag == 0)) ||
			  (((slist[j].status_bits & MIRROR_BIT) != 0) && (mirror_flag != 0)))
		      {
			stats->total_segments ++;
			stats->total_blocks += slist[j].segment_count;

			 for (k=0; k < segment_count; k++)
			 {
			    if (segment_assign[k].lpartition_id == slist[j].lpart_handle)
			    {
			       if (segment_assign[k].block_count < slist[j].segment_count)
			       {
				  stats->max_unique_blocks -= segment_assign[k].block_count;
				  stats->max_unique_blocks += slist[j].segment_count;

				  segment_assign[k].block_count = (slist[j].segment_count + 15) & 0xFFFFFFF0;
				  segment_assign[k].block_offset = slist[j].segment_offset;
			       }
			       break;
			    }
			 }

			 if (k == segment_count)
			 {
			    stats->max_unique_segments ++;
			    stats->max_unique_blocks += slist[j].segment_count;

			    if (segment_count == vpart_info->segment_count)
			    {
			       lowest_count = 0xFFFFFFFF;
			       lowest_index = 0;
			       for (k=0; k < segment_count; k++)
			       {
				  if (segment_assign[k].block_count < lowest_count)
				  {
				     lowest_count = segment_assign[k].block_count;
				     lowest_index = k;
				  }
			       }

			       if (segment_assign[lowest_index].block_count < slist[j].segment_count)
			       {
				  segment_assign[lowest_index].block_count = slist[j].segment_count;
				  segment_assign[lowest_index].block_offset = slist[j].segment_offset;
				  segment_assign[lowest_index].lpartition_id = slist[j].lpart_handle;
			       }
			    }
			    else
			    {
			       segment_assign[k].lpartition_id = slist[j].lpart_handle;
			       segment_assign[k].block_offset = slist[j].segment_offset;
			       segment_assign[k].block_count = slist[j].segment_count;
			       segment_count++;
			    }
			 }

			 if (segment_count == vpart_info->segment_count)
			 {
			    cluster_count = vpart_info->cluster_count * vpart_info->blocks_per_cluster;
			    segment_size = ((cluster_count / segment_count) + 15 ) & 0xFFFFFFF0;

			    for (k=0; k < segment_count; k++)
			       segment_assign[k].extra = 0;

			    for (k=0; k < segment_count; k++)
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

				  for (l=k; l > 0; l--)
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
				fit_flag = 1;
					vpart_info->cluster_count = 0;
			       for (k=0; k < segment_count; k++)
			       {
						vpart_info->cluster_count += segment_assign[k].extra / vpart_info->blocks_per_cluster;
						segment_assign[k].block_count = segment_assign[k].extra;
						segment_assign[k].extra = 0;
			       }
			       if (commit_flag != 0)
			       {
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
		}
	     } while ((payload.payload_index != 0) && (payload.payload_object_count != 0));
	  }
       } while ((payload1.payload_index != 0) && (payload1.payload_object_count != 0));
    }
    NWUnlockNWVP();

    if (vpart_info->segment_count > stats->max_unique_segments)
       return (NWVP_TOO_MANY_SEGMENTS);

    cluster_count = vpart_info->cluster_count * vpart_info->blocks_per_cluster;
    if (cluster_count <= stats->max_unique_blocks)
    {
	if (fit_flag == 0)
	   return (NWVP_NOT_ENOUGH_SEGMENTS);
	return (0);
    }
    return (NWVP_NOT_ENOUGH_SPACE);
}

ULONG NWRestoreNetWareVolume(IMAGE *image, ULONG portal,
			     double *total_last, double *total_curr,
			     double *factor)
{
    register ULONG ccode, index, len, current, i, j, errorCode = (ULONG) -1;
    register ULONG vindex, dataOffset, vsize, total, acode;
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
    ULONG totalFiles = 0, update_count = 0, delta_clusters;
    double TotalSize = 0;
    LONGLONG DataSize = 0;
    BYTE displayBuffer[255];
    double ptotal, pdata, percent, percent_curr = 0, percent_last = 0;
    double base;
#if (LINUX_UTIL)
    struct termios init_settings, new_settings;
#endif

#if (LINUX_UTIL)
    tcgetattr(0, &init_settings);
#endif

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
       log_sprintf(displayBuffer, "Could Not Find Volume [%s]", &image->VolumeName[1]);
       error_portal(displayBuffer, 10);
       goto ReturnError;
    }

    // check if this volume has enough space to store the image set data
    // we adjust this volumes size and subtract the allocation that
    // is being used by the FAT tables.

    if (image->AllocatedVolumeClusters > (volume->VolumeClusters -
	(((volume->VolumeClusters * sizeof(FAT_ENTRY)) +
	(volume->ClusterSize - 1)) / volume->ClusterSize)))
    {
       log_sprintf(displayBuffer, "%u Clusters Required For Image Set [%s]",
		 (unsigned int)image->AllocatedVolumeClusters,
		 image->ImageSetName);
       error_portal(displayBuffer, 10);
       goto ReturnError;
    }

    log_sprintf(displayBuffer, "  Restoring Volume [%15s]",  &image->VolumeName[1]);
    write_portal_cleol(portal, displayBuffer, 1, 0, BRITEWHITE | BGBLUE);

    sprintf(displayBuffer, "  Files Processed                    0%% Complete ");
    write_portal_cleol(portal, displayBuffer, 2, 0, BRITEWHITE | BGBLUE);

    sprintf(displayBuffer, "0%%                               50%%                                100%%");
    write_portal_cleol(portal, displayBuffer, 6, 2, BRITEWHITE | BGBLUE);

#if (LINUX_UTIL)
    for (j=0; j < 72; j++)
    {
       if (HasColor)
	  write_portal_char(portal, 176, 7, 2 + j, BRITEWHITE | BGBLUE);
       else
	  write_portal_char(portal, '.', 7, 2 + j, BRITEWHITE | BGBLUE);
    }
#else
    for (j=0; j < 72; j++)
       write_portal_char(portal, 176, 7, 2 + j, BRITEWHITE | BGBLUE);
#endif
    base = 0;
    if (total_curr)
      base = *total_curr;


    sprintf(displayBuffer, "                          Percent Complete");
    write_portal_cleol(portal, displayBuffer, 8, 2, BRITEWHITE | BGBLUE);

    log_sprintf(displayBuffer, "  Mounting Volume %s", &image->VolumeName[1]);
    write_portal_cleol(portal, displayBuffer, 4, 0, BRITEWHITE | BGBLUE);

    sprintf(displayBuffer, "                     ESC/F3,Q,X,A - Abort Imaging Operation");
    write_portal_cleol(portal, displayBuffer, 15, 0, BRITEWHITE | BGBLUE);

    update_static_portal(portal);

    retCode = MountUtilityVolume(volume);
    if (retCode)
    {
       log_sprintf(displayBuffer, "  Volume %s Has Errors.  Remove Invalid Volume Definitions? ",
		   volume->VolumeName, volume->VolumeName);
       ccode = confirm_menu(displayBuffer, 10, YELLOW | BGBLUE);
       if (ccode)
       {
	  ULONG vpart_handle = 0, len;
	  BYTE volname[20];
	  extern void process_set_file(BYTE *);
	  nwvp_payload payload;
	  ULONG handles[7];
	  ULONG found_flag;
	  nwvp_vpartition_info vpart_info;

	  len = (BYTE)strlen(volume->VolumeName);
	  if (len <= 15)
	  {
	     volname[0] = (BYTE) len;
	     NWFSCopy(&volname[1], volume->VolumeName, len);
	     volname[len + 1] = '\0';

	     if (!convert_volume_name_to_handle(volname, &vpart_handle))
	     {
		if (!nwvp_vpartition_unformat(vpart_handle))
		{
		   message_portal("  Invalid Volume Removed -- Retry Restore <Return> ",
			   10, BRITEWHITE | BGBLUE, TRUE);

		   ScanVolumes();
		   ScanForDataSets();

		   if (image_set_table_count)
		   {
		      NWFSCopy(&RestoreSetName[0], &set_file_name_table[0][0], 12);
		      for (i=0; i < strlen(RestoreSetName); i++)
			       RestoreSetName[i] = toupper(RestoreSetName[i]);
		      RestoreSetName[12] = '\0';
		      process_set_file(RestoreSetName);
		   }
		   else
		   {
		      NWFSCopy(&RestoreSetName[0], "IMAGE000.SET", strlen("IMAGE000.SET"));
		      RestoreSetName[12] = '\0';
		      for (i=0; i < strlen(RestoreSetName); i++)
			 RestoreSetName[i] = toupper(RestoreSetName[i]);
		   }

		   image_table_count = 0;
		   process_set_file(RestoreSetName);

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
		      {
			 NWFSSet(&target_volume_table[i][0], 0, 16);
			 target_volume_flags[i] = 0;
		      }
		   }
		   sprintf(NameHeader, "  Restore Set Filename:  %s           ", RestoreSetName);
		   write_portal_header1(nw_portal, NameHeader, BRITEWHITE | BGBLUE);

		   goto ReturnError;
		}
	     }
	  }
       }
       goto ReturnError;
    }
    delta_clusters = volume->VolumeAllocatedClusters;

    WorkPath[0] = '\0';
    strcat(WorkPath, Path);
    strcat(WorkPath, DirectoryName);
    dir = fopen(WorkPath, "rb+wb");
    if (!dir)
    {
       log_sprintf(displayBuffer, "Volume Data Set [%s] not found", DirectoryName);
       error_portal(displayBuffer, 10);
       goto ReturnErrorDismountUtil;
    }

    log_sprintf(displayBuffer, "  Directory Catalog Opened   ... %s", DirectoryName);
    write_portal_cleol(portal, displayBuffer, 4, 0, BRITEWHITE | BGBLUE);
    update_static_portal(portal);

    WorkPath[0] = '\0';
    strcat(WorkPath, Path);
    strcat(WorkPath, ExtDirectoryName);
    ext = fopen(WorkPath, "rb+wb");
    if (!ext)
    {
       log_sprintf(displayBuffer, "Volume Data Set [%s] not found", ExtDirectoryName);
       error_portal(displayBuffer, 10);
       goto ReturnErrorDismountUtil;
    }

    log_sprintf(displayBuffer, "  Extended Directory Opened  ... %s", ExtDirectoryName);
    write_portal_cleol(portal, displayBuffer, 4, 0, BRITEWHITE | BGBLUE);
    update_static_portal(portal);

    WorkPath[0] = '\0';
    strcat(WorkPath, Path);
    strcat(WorkPath, DataName);
    data = fopen(WorkPath, "rb");
    if (!data)
    {
       log_sprintf(displayBuffer, "Volume Data Set [%s] not found", DataName);
       error_portal(displayBuffer, 10);
       goto ReturnErrorDismountUtil;
    }

    log_sprintf(displayBuffer, "  Data Archive Opened        ... %s", DataName);
    write_portal_cleol(portal, displayBuffer, 4, 0, BRITEWHITE | BGBLUE);
    update_static_portal(portal);

    DataSize = (LONGLONG)((LONGLONG)image->AllocatedVolumeClusters *
			  (LONGLONG)image->VolumeClusterSize);

    dos = (DOS *) malloc(sizeof(DOS));
    if (!dos)
    {
       log_sprintf(displayBuffer, "error allocating directory workspace");
       error_portal(displayBuffer, 10);
       goto ReturnErrorDismountUtil;
    }

    cBuffer = malloc(volume->ClusterSize);
    if (!cBuffer)
    {
       log_sprintf(displayBuffer, "error allocating data workspace");
       error_portal(displayBuffer, 10);
       goto ReturnErrorDismountUtil;
    }

    root = malloc(sizeof(ROOT));
    if (!root)
    {
       log_sprintf(displayBuffer, "error allocating data workspace");
       error_portal(displayBuffer, 10);
       goto ReturnErrorDismountUtil;
    }

    // check the volume directory chain heads
    Chain1 = volume->FirstDirectory;
    Chain2 = volume->SecondDirectory;
    if (!Chain1 || !Chain2)
    {
       log_sprintf(displayBuffer, "volume directory chains are NULL");
       error_portal(displayBuffer, 10);
       goto ReturnErrorDismountUtil;
    }

    // truncate the first directory to a single cluster
    ccode = TruncateClusterChain(volume, &Chain1, 0, 0,
				 volume->ClusterSize, 0, 0);
    if (ccode)
    {
       log_sprintf(displayBuffer, "error truncating volume directory file (primary)");
       error_portal(displayBuffer, 10);
       goto ReturnErrorDismountUtil;
    }

    // truncate the second directory to a single cluster
    ccode = TruncateClusterChain(volume, &Chain2, 0, 0,
				 volume->ClusterSize, 0, 0);
    if (ccode)
    {
       log_sprintf(displayBuffer, "error truncating volume directory file (mirror)");
       error_portal(displayBuffer, 10);
       goto ReturnErrorDismountUtil;
    }

    // restore the directory file
    log_sprintf(displayBuffer, "  Restoring Volume Directory ... ");
    write_portal_cleol(portal, displayBuffer, 4, 0, BRITEWHITE | BGBLUE);
    update_static_portal(portal);

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

#if (LINUX_UTIL)
    // if we are Linux, we need kbhit(), so we implement a terminal
    // based version of kbhit() so we can detect keystrokes during
    // the imaging operation, and abort the operation if asked to
    // do so by the user.  the code below puts a terminal into raw mode
    // so that is does not block when trying to read from the
    // keyboard device.

    tcgetattr(0, &init_settings);
    new_settings = init_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_settings);
#endif

    while (TRUE)
    {
#if (LINUX_UTIL)
       BYTE nread, ch;

       // put the terminal into raw mode, and see if we have a valid
       // keystroke pending.  if we have a keystroke pending, then
       // see if the user requested that we abort the imaging operation.

       new_settings.c_cc[VMIN] = 0;
       tcsetattr(0, TCSANOW, &new_settings);
       nread = read(0, &ch, 1);
       new_settings.c_cc[VMIN] = 1;
       tcsetattr(0, TCSANOW, &new_settings);

       if (nread)
       {
	  if ((toupper(ch) == 'X') || (toupper(ch) == 'Q') ||
	      (toupper(ch) == ESC) || (toupper(ch) == 'A'))
	  {
	     acode = confirm_menu("  Abort Restore Operation? ", 10, YELLOW | BGBLUE);
	     if (acode)
	     {
		log_sprintf(displayBuffer, "                     Restore Operation is Aborting ....");
		write_portal_cleol(portal, displayBuffer, 15, 0, BRITEWHITE | BGBLUE);
		update_static_portal(portal);

		errorCode = -3;
		goto ReturnErrorDismountUtil;
	     }
	  }
       }
#else
       BYTE ch;

       // DOS and Windows NT/2000 both support the kbhit() function.

       if (kbhit())
       {
	  ch = getch();
	  if ((toupper(ch) == 'X') || (toupper(ch) == 'Q') ||
	      (toupper(ch) == ESC) || (toupper(ch) == 'A'))
	  {
	     acode = confirm_menu("  Abort Restore Operation? ", 10, YELLOW | BGBLUE);
	     if (acode)
	     {
		log_sprintf(displayBuffer, "                     Restore Operation is Aborting ....");
		write_portal_cleol(portal, displayBuffer, 15, 0, BRITEWHITE | BGBLUE);
		update_static_portal(portal);

		errorCode = -3;
		goto ReturnErrorDismountUtil;
	     }
	  }
       }
#endif

       if (feof(dir))
	  break;

       current = ftell(dir);
       ccode = fread(dos, sizeof(DOS), 1, dir);
       if (!ccode)
	  break;

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
	  log_sprintf(displayBuffer, "error writing volume directory file");
	  error_portal(displayBuffer, 10);
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
	  log_sprintf(displayBuffer, "error writing volume directory file rc-%d ccode-%d",
		    (int)retCode, (int)ccode);
	  error_portal(displayBuffer, 10);
	  goto ReturnErrorDismountUtil;
       }

       // advance to next record
       index++;
    }
#if (LINUX_UTIL)
    tcsetattr(0, TCSANOW, &init_settings);
#endif

    if ((Chain1 != volume->FirstDirectory) ||
	(Chain2 != volume->SecondDirectory))
    {
       log_sprintf(displayBuffer, "chain error while writing directory file");
       error_portal(displayBuffer, 10);
       goto ReturnErrorDismountUtil;
    }

    TotalSize = ptotal = (double)
		    ((LONGLONG)(volume->VolumeAllocatedClusters - delta_clusters) *
		     (LONGLONG)volume->ClusterSize);
    pdata = (double)DataSize;

    percent = (ptotal / pdata) * 100;
    if (percent > 100)
       percent = 100;

    percent_curr = (percent * 72) / 100;
    if (total_curr)
    {
       if (factor)
       {
	  if (!(*factor))
	     *factor = 1;
	  *total_curr = base + ((percent * (72 / (*factor))) / 100);
       }
    }

    sprintf(displayBuffer, "  Bytes Processed %-10d         %-3d%% Compressed", (int)TotalSize, 0);
    write_portal_cleol(portal, displayBuffer, 3, 0, BRITEWHITE | BGBLUE);

    sprintf(displayBuffer, "  Files Processed %-10d         %-3d%% Complete ",
	   (int)totalFiles, (int)percent);
    write_portal_cleol(portal, displayBuffer, 2, 0, BRITEWHITE | BGBLUE);

#if (LINUX_UTIL)
    for (j=percent_last; (j < percent_curr) && (j < 72); j++)
    {
       if (HasColor)
	  write_portal_char(portal, 219, 7, 2 + j, BRITEWHITE | BGWHITE);
       else
	  write_portal_char(portal, 'X', 7, 2 + j, BRITEWHITE | BGWHITE);
    }

    for (j=(*total_last); (j < (*total_curr)) && (j < 72); j++)
    {
       if (HasColor)
	  write_portal_char(portal, 219, 12, 2 + j, BRITEWHITE | BGWHITE);
       else
	  write_portal_char(portal, 'X', 12, 2 + j, BRITEWHITE | BGWHITE);
    }
#else
    for (j=(ULONG)percent_last; (j < (ULONG)percent_curr) && (j < 72); j++)
       write_portal_char(portal, 219, 7, 2 + j, BRITEWHITE | BGBLUE);

    for (j=(ULONG)(*total_last); (j < (ULONG)(*total_curr)) && (j < 72); j++)
       write_portal_char(portal, 219, 12, 2 + j, BRITEWHITE | BGBLUE);
#endif
    percent_last = percent_curr;
    if (total_last && total_curr)
       *total_last = *total_curr;

    update_static_portal(portal);

    // restore the extended directory file
    // read root directory;

    fseek(dir, 0L, SEEK_SET);
    ccode = fread(root, sizeof(ROOT), 1, dir);
    if (!ccode)
    {
       log_sprintf(displayBuffer, "could not read root entry in catalog file");
       error_portal(displayBuffer, 10);
       goto ReturnErrorDismountUtil;
    }

    index = 0;
    Chain1 = Chain2 = (ULONG) -1;
    fseek(ext, 0L, SEEK_SET);

    log_sprintf(displayBuffer, "  Writing Extended Directory ... %c", widget[ndx++ & 3]);
    write_portal_cleol(portal, displayBuffer, 4, 0, BRITEWHITE | BGBLUE);
    update_static_portal(portal);

#if (LINUX_UTIL)
    // if we are Linux, we need kbhit(), so we implement a terminal
    // based version of kbhit() so we can detect keystrokes during
    // the imaging operation, and abort the operation if asked to
    // do so by the user.  the code below puts a terminal into raw mode
    // so that is does not block when trying to read from the
    // keyboard device.

    tcgetattr(0, &init_settings);
    new_settings = init_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_settings);
#endif

    while (TRUE)
    {
#if (LINUX_UTIL)
       BYTE nread, ch;

       // put the terminal into raw mode, and see if we have a valid
       // keystroke pending.  if we have a keystroke pending, then
       // see if the user requested that we abort the imaging operation.

       new_settings.c_cc[VMIN] = 0;
       tcsetattr(0, TCSANOW, &new_settings);
       nread = read(0, &ch, 1);
       new_settings.c_cc[VMIN] = 1;
       tcsetattr(0, TCSANOW, &new_settings);

       if (nread)
       {
	  if ((toupper(ch) == 'X') || (toupper(ch) == 'Q') ||
	      (toupper(ch) == ESC) || (toupper(ch) == 'A'))
	  {
	     acode = confirm_menu("  Abort Restore Operation? ", 10, YELLOW | BGBLUE);
	     if (acode)
	     {
		log_sprintf(displayBuffer, "                     Restore Operation is Aborting ....");
		write_portal_cleol(portal, displayBuffer, 15, 0, BRITEWHITE | BGBLUE);
		update_static_portal(portal);

		errorCode = -3;
		goto ReturnErrorDismountUtil;
	     }
	  }
       }
#else
       BYTE ch;

       // DOS and Windows NT/2000 both support the kbhit() function.

       if (kbhit())
       {
	  ch = getch();
	  if ((toupper(ch) == 'X') || (toupper(ch) == 'Q') ||
	      (toupper(ch) == ESC) || (toupper(ch) == 'A'))
	  {
	     acode = confirm_menu("  Abort Restore Operation? ", 10, YELLOW | BGBLUE);
	     if (acode)
	     {
		log_sprintf(displayBuffer, "                     Restore Operation is Aborting ....");
		write_portal_cleol(portal, displayBuffer, 15, 0, BRITEWHITE | BGBLUE);
		update_static_portal(portal);

		errorCode = -3;
		goto ReturnErrorDismountUtil;
	     }
	  }
       }
#endif

       if (feof(ext))
	  break;

       current = ftell(ext);
       ccode = fread(dos, sizeof(DOS), 1, ext);
       if (!ccode)
	  break;

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
	  log_sprintf(displayBuffer, "error writing extended directory file");
	  error_portal(displayBuffer, 10);
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
	  log_sprintf(displayBuffer, "error writing extended directory file");
	  error_portal(displayBuffer, 10);
	  goto ReturnErrorDismountUtil;
       }

       // advance to next record
       index++;
    }
#if (LINUX_UTIL)
    tcsetattr(0, TCSANOW, &init_settings);
#endif

    TotalSize = ptotal = (double)
		    ((LONGLONG)(volume->VolumeAllocatedClusters - delta_clusters) *
		     (LONGLONG)volume->ClusterSize);
    pdata = (double)DataSize;

    percent = (ptotal / pdata) * 100;
    if (percent > 100)
       percent = 100;

    percent_curr = (percent * 72) / 100;
    if (total_curr)
    {
       if (factor)
       {
	  if (!(*factor))
	     *factor = 1;
	  *total_curr = base + ((percent * (72 / (*factor))) / 100);
       }
    }

    sprintf(displayBuffer, "  Bytes Processed %-10d         %-3d%% Compressed", (int)TotalSize, 0);
    write_portal_cleol(portal, displayBuffer, 3, 0, BRITEWHITE | BGBLUE);

    sprintf(displayBuffer, "  Files Processed %-10d         %-3d%% Complete ",
	   (int)totalFiles, (int)percent);
    write_portal_cleol(portal, displayBuffer, 2, 0, BRITEWHITE | BGBLUE);

#if (LINUX_UTIL)
    for (j=percent_last; (j < percent_curr) && (j < 72); j++)
    {
       if (HasColor)
	  write_portal_char(portal, 219, 7, 2 + j, BRITEWHITE | BGWHITE);
       else
	  write_portal_char(portal, 'X', 7, 2 + j, BRITEWHITE | BGWHITE);
    }

    for (j=(*total_last); (j < (*total_curr)) && (j < 72); j++)
    {
       if (HasColor)
	  write_portal_char(portal, 219, 12, 2 + j, BRITEWHITE | BGWHITE);
       else
	  write_portal_char(portal, 'X', 12, 2 + j, BRITEWHITE | BGWHITE);
    }
#else
    for (j=(ULONG)percent_last; (j < (ULONG)percent_curr) && (j < 72); j++)
       write_portal_char(portal, 219, 7, 2 + j, BRITEWHITE | BGBLUE);

    for (j=(ULONG)(*total_last); (j < (ULONG)(*total_curr)) && (j < 72); j++)
       write_portal_char(portal, 219, 12, 2 + j, BRITEWHITE | BGBLUE);
#endif
    percent_last = percent_curr;
    if (total_last && total_curr)
       *total_last = *total_curr;

    update_static_portal(portal);

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
       log_sprintf(displayBuffer, "error writing volume directory file");
       error_portal(displayBuffer, 10);
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
       log_sprintf(displayBuffer, "error writing volume directory file");
       error_portal(displayBuffer, 10);
       goto ReturnErrorDismountUtil;
    }

    // now dismount and remount the volume to perform the
    // file restore.

    DismountUtilityVolume(volume);

    //
    // now we begin the actual file restore
    //

    log_sprintf(displayBuffer, "  File Restore in Progress ...");
    write_portal_cleol(portal, displayBuffer, 4, 0, BRITEWHITE | BGBLUE);
    update_static_portal(portal);

    retCode = MountUtilityVolume(volume);
    if (retCode)
    {
       log_sprintf(displayBuffer, "error mounting volume %s", volume->VolumeName);
       error_portal(displayBuffer, 10);
       goto ReturnError;
    }

    Dir1Size = GetChainSize(volume, volume->FirstDirectory);
    Dir2Size = GetChainSize(volume, volume->SecondDirectory);

    if (!Dir1Size || !Dir2Size || (Dir1Size != Dir2Size))
    {
       log_sprintf(displayBuffer, "primary and mirror directory files are mismatched");
       error_portal(displayBuffer, 10);
       goto ReturnErrorDismountUtil;
    }

#if (LINUX_UTIL)
    // if we are Linux, we need kbhit(), so we implement a terminal
    // based version of kbhit() so we can detect keystrokes during
    // the imaging operation, and abort the operation if asked to
    // do so by the user.  the code below puts a terminal into raw mode
    // so that is does not block when trying to read from the
    // keyboard device.

    tcgetattr(0, &init_settings);
    new_settings = init_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_settings);
#endif

    DirTotal = Dir1Size / sizeof(ROOT);
    for (total = index = i = 0; i < DirTotal; i++)
    {
#if (LINUX_UTIL)
       BYTE nread, ch;

       // put the terminal into raw mode, and see if we have a valid
       // keystroke pending.  if we have a keystroke pending, then
       // see if the user requested that we abort the imaging operation.

       new_settings.c_cc[VMIN] = 0;
       tcsetattr(0, TCSANOW, &new_settings);
       nread = read(0, &ch, 1);
       new_settings.c_cc[VMIN] = 1;
       tcsetattr(0, TCSANOW, &new_settings);

       if (nread)
       {
	  if ((toupper(ch) == 'X') || (toupper(ch) == 'Q') ||
	      (toupper(ch) == ESC) || (toupper(ch) == 'A'))
	  {
	     acode = confirm_menu("  Abort Restore Operation? ", 10, YELLOW | BGBLUE);
	     if (acode)
	     {
		log_sprintf(displayBuffer, "                     Restore Operation is Aborting ....");
		write_portal_cleol(portal, displayBuffer, 15, 0, BRITEWHITE | BGBLUE);
		update_static_portal(portal);

		errorCode = -3;
		goto ReturnErrorDismountUtil;
	     }
	  }
       }
#else
       BYTE ch;

       // DOS and Windows NT/2000 both support the kbhit() function.

       if (kbhit())
       {
	  ch = getch();
	  if ((toupper(ch) == 'X') || (toupper(ch) == 'Q') ||
	      (toupper(ch) == ESC) || (toupper(ch) == 'A'))
	  {
	     acode = confirm_menu("  Abort Restore Operation? ", 10, YELLOW | BGBLUE);
	     if (acode)
	     {
		log_sprintf(displayBuffer, "                     Restore Operation is Aborting ....");
		write_portal_cleol(portal, displayBuffer, 15, 0, BRITEWHITE | BGBLUE);
		update_static_portal(portal);

		errorCode = -3;
		goto ReturnErrorDismountUtil;
	     }
	  }
       }
#endif

       ccode = ReadDirectoryRecord(volume, dos, i);
       if (ccode)
       {
	  log_sprintf(displayBuffer, "nwfs:  error reading volume directory");
	  error_portal(displayBuffer, 10);
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

	  totalFiles++;

	  if (dos->FileNameLength)
	  {
	     TotalSize = ptotal = (double)
				    ((LONGLONG)(volume->VolumeAllocatedClusters - delta_clusters) *
				     (LONGLONG)volume->ClusterSize);
	     pdata = (double)DataSize;

	     percent = (ptotal / pdata) * 100;
	     if (percent > 100)
		percent = 100;

	     percent_curr = (percent * 72) / 100;
	     if (total_curr)
	     {
		if (factor)
		{
		   if (!(*factor))
		      *factor = 1;
		   *total_curr = base + ((percent * (72 / (*factor))) / 100);
		}
	     }

	     sprintf(displayBuffer, "  Bytes Processed %-10d         %-3d%% Compressed", (int)TotalSize, 0);
	     write_portal_cleol(portal, displayBuffer, 3, 0, BRITEWHITE | BGBLUE);

	     sprintf(displayBuffer, "  Files Processed %-10d         %-3d%% Complete ",
			   (int)totalFiles, (int)percent);
	     write_portal_cleol(portal, displayBuffer, 2, 0, BRITEWHITE | BGBLUE);

#if (LINUX_UTIL)
	     for (j=percent_last; (j < percent_curr) && (j < 72); j++)
	     {
		if (HasColor)
		   write_portal_char(portal, 219, 7, 2 + j, BRITEWHITE | BGWHITE);
		else
		   write_portal_char(portal, 'X', 7, 2 + j, BRITEWHITE | BGWHITE);
	     }

	     for (j=(*total_last); (j < (*total_curr)) && (j < 72); j++)
	     {
		if (HasColor)
		   write_portal_char(portal, 219, 12, 2 + j, BRITEWHITE | BGWHITE);
		else
		   write_portal_char(portal, 'X', 12, 2 + j, BRITEWHITE | BGWHITE);
	     }
#else
	     for (j=(ULONG)percent_last; (j < (ULONG)percent_curr) && (j < 72); j++)
		write_portal_char(portal, 219, 7, 2 + j, BRITEWHITE | BGBLUE);

	     for (j=(ULONG)(*total_last); (j < (ULONG)(*total_curr)) && (j < 72); j++)
		write_portal_char(portal, 219, 12, 2 + j, BRITEWHITE | BGBLUE);
#endif
	     percent_last = percent_curr;
	     if (total_last && total_curr)
		*total_last = *total_curr;

	     if (update_count++ > 50)
	     {
		update_count = 0;
		update_static_portal(portal);
	     }
	  }

	  if (dos->FileSize)
	  {
	     dataOffset = (dos->FirstBlock << 4);
	     bytesLeft = dos->FileSize;
	     dos->FirstBlock = (ULONG) -1;

	     ccode = fseek(data, dataOffset, SEEK_SET);
	     if (ccode == (ULONG) -1)
	     {
		log_sprintf(displayBuffer,
			    "range error searching data catalog (%d)",
			    dataOffset);
		error_portal(displayBuffer, 10);
		goto ReturnErrorDismountUtil;
	     }

	     vindex = 0;
	     while (bytesLeft > 0)
	     {
		if (feof(data))
		{
		   log_sprintf(displayBuffer, "EOF error in data catalog");
		   error_portal(displayBuffer, 10);
		   goto ReturnErrorDismountUtil;
		}

		vsize = ((bytesLeft > (long)IO_BLOCK_SIZE)
			 ? IO_BLOCK_SIZE : bytesLeft);

		ccode = fread(cBuffer, vsize, 1, data);
		if (!ccode)
		{
		   log_sprintf(displayBuffer, "read error in data catalog");
		   error_portal(displayBuffer, 10);
		   goto ReturnErrorDismountUtil;
		}

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
		   log_sprintf(displayBuffer, "error writing volume file %s", Buffer);
		   error_portal(displayBuffer, 10);
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
		log_sprintf(displayBuffer, "error truncating file %s", Buffer);
		error_portal(displayBuffer, 10);
		goto ReturnErrorDismountUtil;
	     }
	  }
	  else
	     dos->FirstBlock = (ULONG) -1;

	  ccode = WriteDirectoryRecord(volume, dos, i);
	  if (ccode)
	  {
	     log_sprintf(displayBuffer, "nwfs:  error writing volume directory");
	     error_portal(displayBuffer, 10);
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

	  totalFiles++;

	  if (mac->ResourceForkSize)
	  {
	     dataOffset = (mac->ResourceFork << 4);
	     bytesLeft = mac->ResourceForkSize;
	     mac->ResourceFork = (ULONG) -1;

	     ccode = fseek(data, dataOffset, SEEK_SET);
	     if (ccode == (ULONG) -1)
	     {
		log_sprintf(displayBuffer,
			    "range error searching data catalog (MAC-%d)",
			    dataOffset);
		error_portal(displayBuffer, 10);
		goto ReturnErrorDismountUtil;
	     }

	     vindex = 0;
	     while (bytesLeft > 0)
	     {
		if (feof(data))
		{
		   log_sprintf(displayBuffer, "EOF error in data catalog");
		   error_portal(displayBuffer, 10);
		   goto ReturnErrorDismountUtil;
		}

		vsize = ((bytesLeft > (long)IO_BLOCK_SIZE)
			 ? IO_BLOCK_SIZE : bytesLeft);

		ccode = fread(cBuffer, vsize, 1, data);
		if (!ccode)
		{
		   log_sprintf(displayBuffer, "read error in data catalog");
		   error_portal(displayBuffer, 10);
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
		   log_sprintf(displayBuffer, "error writing volume file %s", Buffer);
		   error_portal(displayBuffer, 10);
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
		log_sprintf(displayBuffer, "error truncating file %s", Buffer);
		error_portal(displayBuffer, 10);
		goto ReturnErrorDismountUtil;
	     }
	  }
	  else
	     mac->ResourceFork = (ULONG) -1;

	  ccode = WriteDirectoryRecord(volume, (DOS *)mac, i);
	  if (ccode)
	  {
	     log_sprintf(displayBuffer, "nwfs:  error writing volume directory");
	     error_portal(displayBuffer, 10);
	     goto ReturnErrorDismountUtil;
	  }
       }
    }
#if (LINUX_UTIL)
    tcsetattr(0, TCSANOW, &init_settings);
#endif

#if (LINUX_UTIL)
    for (j=percent_last; j < 72; j++)
    {
       if (HasColor)
	  write_portal_char(portal, 219, 7, 2 + j, BRITEWHITE | BGWHITE);
       else
	 write_portal_char(portal, 'X', 7, 2 + j, BRITEWHITE | BGWHITE);
    }
#else
    for (j=(ULONG)percent_last; j < 72; j++)
       write_portal_char(portal, 219, 7, 2 + j, BRITEWHITE | BGBLUE);
#endif

    log_sprintf(displayBuffer, "  Files Processed %-10d         %-3d%% Complete ",
	   (int)totalFiles, (int)100);
    write_portal_cleol(portal, displayBuffer, 2, 0, BRITEWHITE | BGBLUE);

    update_static_portal(portal);

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
#if (LINUX_UTIL)
    tcsetattr(0, TCSANOW, &init_settings);
#endif

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

    return (ULONG) errorCode;
}

ULONG warnAction(NWSCREEN *screen, ULONG index)
{
    register ULONG mNum, retCode;

    mNum = make_menu(screen,
		     " Exit NWIMAGE ?",
		     10,
		     30,
		     2,
		     BORDER_DOUBLE,
		     YELLOW | BGBLUE,
		     YELLOW | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     0,
		     0,
		     0,
		     TRUE,
		     0);

    add_item_to_menu(mNum, "Yes", 1);
    add_item_to_menu(mNum, "No", 0);

    retCode = activate_menu(mNum);
    if (retCode == (ULONG) -1)
       retCode = 0;

    free_menu(mNum);

    return retCode;
}

ULONG elementFunction(NWSCREEN *screen, ULONG value, BYTE *option, ULONG index)
{
   switch (value)
   {
      default:
	 break;
   }
   return 0;
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
    BYTE displayBuffer[255];

    table_index = image_table_count;

    WorkPath[0] = '\0';
    strcat(WorkPath, Path);
    strcat(WorkPath, &set_file_name[0]);
    if ((sfd = fopen(WorkPath, "rb+wb")) != 0)
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
	     log_sprintf(displayBuffer,
		      " Checksum Error In Image Set [%s] %X %X", &set_file_name[0],
		       (unsigned int)checksum,
		       (unsigned int)set_header.CheckSum);
	     error_portal(displayBuffer, 10);
	     valid_flag =  0;
	     return;
	  }

	  if (set_header.Stamp == IMAGE_SET_STAMP)
	  {
	     for (i=0; i < IMAGE_TABLE_SIZE; i++)
	     {
		if (!set_header.VolumeFile[i][0])
		   continue;

		WorkPath[0] = '\0';
		strcat(WorkPath, Path);
		strcat(WorkPath, &set_header.VolumeFile[i][0]);
		if ((vfd = fopen(WorkPath, "rb+wb")) != 0)
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
			    target_volume_flags[table_index] = 0;
			    mark_table[table_index] = 0;
			 }
			 else
			 {
			    NWFSSet(&target_volume_table[table_index][0], 0, 16);
			    target_volume_flags[table_index] = 0;
			    mark_table[table_index] = 1;
			 }
			 valid_table[table_index] = 1;
			 table_index++;
		      }
		      else
		      {
			 log_sprintf(displayBuffer, " Invalid Image Stamp in %s ", &set_header.VolumeFile[i][0]);
			 error_portal(displayBuffer, 10);
			 valid_flag = 0;
			 break;
		      }
		   }
		   else
		   {
		      log_sprintf(displayBuffer, " Error reading %s", &set_header.VolumeFile[i][0]);
		      error_portal(displayBuffer, 10);
		      valid_flag = 0;
		      break;
		   }
		   fclose(vfd);
		}
	     }
	  }
	  else
	  {
	     log_sprintf(displayBuffer, " Invalid Image Set Stamp in [%s]", &set_file_name[0]);
	     error_portal(displayBuffer, 10);
	     valid_flag = 0;
	  }
       }
       fclose(sfd);
    }
    else
    {
       log_sprintf(displayBuffer, " Error opening %s", &set_file_name[0]);
       error_portal(displayBuffer, 10);
       valid_flag = 0;
    }

    if (valid_flag != 0)
       image_table_count = table_index;
}

void delete_set(BYTE *set_file_name)
{
    register FILE *sfd;
    register ULONG i;
    register ULONG checksum = 0;
    register ULONG *checksum_ptr;
    IMAGE_SET set_header;
    BYTE displayBuffer[255];

    WorkPath[0] = '\0';
    strcat(WorkPath, Path);
    strcat(WorkPath, &set_file_name[0]);

    if ((sfd = fopen(WorkPath, "rb+wb")) != 0)
    {
       if (fread(&set_header, sizeof(IMAGE_SET), 1, sfd) != 0)
       {
	  register ULONG len;
	  BYTE DirectoryName[20] = "";
	  BYTE ExtDirectoryName[20] = "";
	  BYTE DataName[20] = "";
	  BYTE VolumeName[20] = "";

	  checksum_ptr = (ULONG *) &set_header;
	  for (i=0; i < (sizeof(IMAGE_SET) / 4); i++)
	     checksum += checksum_ptr[i];

	  if (checksum != 0xFFFFFFFF)
	  {
	     log_sprintf(displayBuffer,
		      " Checksum Error In Image Set [%s] %X %X", &set_file_name[0],
		       (unsigned int)checksum,
		       (unsigned int)set_header.CheckSum);
	     error_portal(displayBuffer, 10);
	  }

	  if (set_header.Stamp != IMAGE_SET_STAMP)
	  {
	     log_sprintf(displayBuffer, " Invalid Image Set Stamp in [%s]", &set_file_name[0]);
	     error_portal(displayBuffer, 10);
	  }

	  for (i=0; i < IMAGE_TABLE_SIZE; i++)
	  {
	     if (!set_header.VolumeFile[i][0])
		continue;

	     len = strlen(&set_header.VolumeFile[i][0]);
	     if ((len <= 4) || (len > 12))
		continue;

	     NWFSCopy(DirectoryName, &set_header.VolumeFile[i][0], len);
	     NWFSCopy(ExtDirectoryName, &set_header.VolumeFile[i][0], len);
	     NWFSCopy(DataName, &set_header.VolumeFile[i][0], len);
	     NWFSCopy(VolumeName, &set_header.VolumeFile[i][0], len);

	     DirectoryName[len - 3] = '\0';
	     strcat(DirectoryName, "DIR");
	     ExtDirectoryName[len - 3] = '\0';
	     strcat(ExtDirectoryName, "EXT");
	     DataName[len - 3] = '\0';
	     strcat(DataName, "DAT");

	     WorkPath[0] = '\0';
	     strcat(WorkPath, Path);
	     strcat(WorkPath, DirectoryName);
	     unlink(WorkPath);

	     WorkPath[0] = '\0';
	     strcat(WorkPath, Path);
	     strcat(WorkPath, ExtDirectoryName);
	     unlink(WorkPath);

	     WorkPath[0] = '\0';
	     strcat(WorkPath, Path);
	     strcat(WorkPath, DataName);
	     unlink(WorkPath);

	     WorkPath[0] = '\0';
	     strcat(WorkPath, Path);
	     strcat(WorkPath, VolumeName);
	     unlink(WorkPath);
	  }
       }
       fclose(sfd);
       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, set_file_name);
       unlink(WorkPath);
    }
    else
    {
       log_sprintf(displayBuffer, " Error opening %s", &set_file_name[0]);
       error_portal(displayBuffer, 10);
    }
    return;
}

void GetDateAndTimeString(BYTE *target)
{
#if (WINDOWS_NT_UTIL)
    SYSTEMTIME stime;

    GetLocalTime(&stime);
    sprintf(target, "%02d/%02d/%02d %02d:%02d %c",
	    stime.wMonth,
	    stime.wDay,
	    stime.wYear,
	    (stime.wHour > 12) ? (stime.wHour - 12) : stime.wHour,
	    stime.wMinute,
	    (stime.wHour > 12) ? 'P' : 'A');

#endif

#if (LINUX_UTIL | DOS_UTIL)
    time_t epoch_time;
    struct tm *tm;

    epoch_time = time(NULL);
    tm = localtime(&epoch_time);

    sprintf(target, "%02d/%02d/%02d %02d:%02d %c",
	    tm->tm_mon,
	    tm->tm_mday,
	    tm->tm_year,
	    (tm->tm_hour > 12) ? (tm->tm_hour - 12) : tm->tm_hour,
	    tm->tm_min,
	    (tm->tm_hour > 12) ? 'P' : 'A');

#endif
}

ULONG MangleVolumeNames(BYTE *VolumeName,
			BYTE *VolumeFile,
			BYTE *DirectoryFile,
			BYTE *ExtDirectoryFile,
			BYTE *DataFile)
{
    register ULONG BaseLength, i, collision, val, Size;
    register BYTE *p, *v;
    register FILE *fd;
    register VOLUME *volume;
    BYTE Buffer[20];
    BYTE BaseName[20] = { "" };

    volume = GetVolumeHandle(VolumeName);
    if (!volume)
       return -1;

    // skip whitespace
    p = volume->VolumeName;
    while (*p && *p == ' ')
       p++;

    // strip the first eight characters of the volume name
    for (v = BaseName, BaseLength=0;
	(BaseLength < 8) && (*p) && (*p != ' ');
	BaseLength++)
       *v++ = *p++;
	*v = '\0';

    // here we attempt to create a unique name for this data set.
    // we do check the current directory to see if there already
    // is an existing data set that collides with our name.

    if (BaseLength > 5)
       BaseLength = 5;

    if (VolumeFile)
       NWFSSet(VolumeFile, 0, 13);
    if (DirectoryFile)
       NWFSSet(DirectoryFile, 0, 13);
    if (ExtDirectoryFile)
       NWFSSet(ExtDirectoryFile, 0, 13);
    if (DataFile)
       NWFSSet(DataFile, 0, 13);

    if (VolumeFile)
       NWFSCopy(VolumeFile, BaseName, BaseLength);
    if (DirectoryFile)
       NWFSCopy(DirectoryFile, BaseName, BaseLength);
    if (ExtDirectoryFile)
       NWFSCopy(ExtDirectoryFile, BaseName, BaseLength);
    if (DataFile)
       NWFSCopy(DataFile, BaseName, BaseLength);

    if (VolumeFile)
       VolumeFile[BaseLength] = '~';
    if (DirectoryFile)
       DirectoryFile[BaseLength] = '~';
    if (ExtDirectoryFile)
       ExtDirectoryFile[BaseLength] = '~';
    if (DataFile)
       DataFile[BaseLength] = '~';

    // calculate horner hash value from the original name
    val = NWFSStringHash(volume->VolumeName, 15, 255);
    for (collision=i=0; i < 255; i++)
    {
       sprintf(Buffer, "%02X", (BYTE)(val + i));

       if (VolumeFile)
	  NWFSCopy(&VolumeFile[BaseLength + 1], Buffer, 2);
       if (DirectoryFile)
	  NWFSCopy(&DirectoryFile[BaseLength + 1], Buffer, 2);
       if (ExtDirectoryFile)
	  NWFSCopy(&ExtDirectoryFile[BaseLength + 1], Buffer, 2);
       if (DataFile)
	  NWFSCopy(&DataFile[BaseLength + 1], Buffer, 2);

       if (VolumeFile)
	  VolumeFile[BaseLength + 3] = '.';
       if (DirectoryFile)
	  DirectoryFile[BaseLength + 3] = '.';
       if (ExtDirectoryFile)
	  ExtDirectoryFile[BaseLength + 3] = '.';
       if (DataFile)
	  DataFile[BaseLength + 3] = '.';

       if (VolumeFile)
	  NWFSCopy(&VolumeFile[BaseLength + 4], "VOL", 3);
       if (DirectoryFile)
	  NWFSCopy(&DirectoryFile[BaseLength + 4], "DIR", 3);
       if (ExtDirectoryFile)
	  NWFSCopy(&ExtDirectoryFile[BaseLength + 4], "EXT", 3);
       if (DataFile)
	  NWFSCopy(&DataFile[BaseLength + 4], "DAT", 3);

       if (VolumeFile)
       {
	  WorkPath[0] = '\0';
	  strcat(WorkPath, Path);
	  strcat(WorkPath, VolumeFile);
	  fd = fopen(WorkPath, "rb");
	  if (!fd)
	     break;
	  else
	     fclose(fd);
       }
    }

    // if we could not create a unique name, then increase the
    // counter portion of the name and try until we generate
    // a unique name.

    if (collision)
    {
       i = BaseLength & 0xffff;
       Size = (BaseLength >> 16) & 0x7;

       if (BaseLength > 2)
	  BaseLength = 2;

       if (VolumeFile)
	  NWFSSet(VolumeFile, ' ', 13);
       if (DirectoryFile)
	  NWFSSet(DirectoryFile, ' ', 13);
       if (ExtDirectoryFile)
	  NWFSSet(ExtDirectoryFile, ' ', 13);
       if (DataFile)
	  NWFSSet(DataFile, ' ', 13);

       if (VolumeFile)
	  NWFSCopy(VolumeFile, BaseName, BaseLength);
       if (DirectoryFile)
	  NWFSCopy(DirectoryFile, BaseName, BaseLength);
       if (ExtDirectoryFile)
	  NWFSCopy(ExtDirectoryFile, BaseName, BaseLength);
       if (DataFile)
	  NWFSCopy(DataFile, BaseName, BaseLength);

       if (VolumeFile)
	  VolumeFile[BaseLength + 4] = '~';
       if (DirectoryFile)
	  DirectoryFile[BaseLength + 4] = '~';
       if (ExtDirectoryFile)
	  ExtDirectoryFile[BaseLength + 4] = '~';
       if (DataFile)
	  DataFile[BaseLength + 4] = '~';

       if (VolumeFile)
	  VolumeFile[BaseLength + 5] = (BYTE)('1' + Size);
       if (DirectoryFile)
	  DirectoryFile[BaseLength + 5] = (BYTE)('1' + Size);
       if (ExtDirectoryFile)
	  ExtDirectoryFile[BaseLength + 5] = (BYTE)('1' + Size);
       if (DataFile)
	  DataFile[BaseLength + 5] = (BYTE)('1' + Size);

       i -= 11;

       while (TRUE)
       {
	  sprintf(Buffer, "%04X", (unsigned int) i);

	  if (VolumeFile)
	     NWFSCopy(&VolumeFile[BaseLength], Buffer, 4);
	  if (DirectoryFile)
	     NWFSCopy(&DirectoryFile[BaseLength], Buffer, 4);
	  if (ExtDirectoryFile)
	     NWFSCopy(&ExtDirectoryFile[BaseLength], Buffer, 4);
	  if (DataFile)
	     NWFSCopy(&DataFile[BaseLength], Buffer, 4);

	  if (VolumeFile)
	     VolumeFile[BaseLength + 6] = '.';
	  if (DirectoryFile)
	     DirectoryFile[BaseLength + 6] = '.';
	  if (ExtDirectoryFile)
	     ExtDirectoryFile[BaseLength + 6] = '.';
	  if (DataFile)
	     DataFile[BaseLength + 6] = '.';

	  if (VolumeFile)
	     NWFSCopy(&VolumeFile[BaseLength + 7], "VOL", 3);
	  if (DirectoryFile)
	     NWFSCopy(&DirectoryFile[BaseLength + 7], "DIR", 3);
	  if (ExtDirectoryFile)
	     NWFSCopy(&ExtDirectoryFile[BaseLength + 7], "EXT", 3);
	  if (DataFile)
	     NWFSCopy(&DataFile[BaseLength + 7], "DAT", 3);

	  if (VolumeFile)
	  {
	     WorkPath[0] = '\0';
	     strcat(WorkPath, Path);
	     strcat(WorkPath, VolumeFile);
	     fd = fopen(WorkPath, "rb");
	     if (!fd)
		break;
	     else
		fclose(fd);
	  }

	  i -= 11;
       }
    }
    return 0;

}

ULONG CreateUniqueSet(BYTE *Name, BYTE *MangledName, ULONG MangledLength,
		      ULONG *NewLength)
{
    register ULONG BaseLength, i, collision, val, Size;
    register BYTE *p, *v;
    register FILE *fd;
    BYTE BaseNumber[20];
    BYTE BaseName[20] = { "" };
    BYTE Buffer[20] = { "" };

    WorkPath[0] = '\0';
    strcat(WorkPath, Path);
    strcat(WorkPath, Name);
    fd = fopen(WorkPath, "rb");
    if (!fd)
    {
       strcpy(MangledName, Name);
       if (NewLength)
	  *NewLength = strlen(MangledName);
       return 0;
    }
    else
       fclose(fd);

    if (MangledLength < 13)
       return -1;

    // strip the first eight characters of the name
    for (p = Name, v = BaseName, BaseLength=0;
	(BaseLength < 8) && (*p) && (*p != ' ');
	BaseLength++)
       *v++ = *p++;
    *v = '\0';

    // here we attempt to create a unique name for this data set.
    // we do check the current directory to see if there already
    // is an existing data set that collides with our name.

    if (BaseLength > 5)
       BaseLength = 5;

    if ((!NWFSCompare(BaseName, "IMAGE", 5)) && (strlen(Name) == 12))
    {
       for (i=0; i < 3; i++)
	  BaseNumber[i] = BaseName[i + 5];
       BaseNumber[i] = '\0';

       NWFSSet(MangledName, 0, MangledLength);
       NWFSCopy(MangledName, BaseName, BaseLength);

       i = atoi(BaseNumber);
       while (i < 999)
       {
	  i++;
	  sprintf(Buffer, "%03d", (int)i);

	  NWFSCopy(&MangledName[BaseLength], Buffer, 3);
	  MangledName[BaseLength + 3] = '.';
	  NWFSCopy(&MangledName[BaseLength + 4], "SET", 3);

	  WorkPath[0] = '\0';
	  strcat(WorkPath, Path);
	  strcat(WorkPath, MangledName);
	  fd = fopen(WorkPath, "rb");
	  if (!fd)
	  {
	     if (NewLength)
		*NewLength = strlen(MangledName);
	     return 0;
	  }
	  else
	     fclose(fd);
       }
    }

    NWFSSet(MangledName, 0, MangledLength);
    NWFSCopy(MangledName, BaseName, BaseLength);
    MangledName[BaseLength] = '~';

    // calculate horner hash value from the original name
    val = NWFSStringHash(Name, strlen(Name), 255);
    for (collision = i = 0; i < 255; i++)
    {
       sprintf(Buffer, "%02X", (BYTE)(val + i));

       NWFSCopy(&MangledName[BaseLength + 1], Buffer, 2);
       MangledName[BaseLength + 3] = '.';
       NWFSCopy(&MangledName[BaseLength + 4], "SET", 3);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, MangledName);
       fd = fopen(WorkPath, "rb");
       if (!fd)
	  break;
       else
	  fclose(fd);
    }

    // if we could not create a unique name, then increase the
    // counter portion of the name and try until we generate
    // a unique name.

    if (collision)
    {
       i = BaseLength & 0xffff;
       Size = (BaseLength >> 16) & 0x7;

       if (BaseLength > 2)
	  BaseLength = 2;

       NWFSSet(MangledName, ' ', 13);
       NWFSCopy(MangledName, BaseName, BaseLength);
       MangledName[BaseLength + 4] = '~';
       MangledName[BaseLength + 5] = (BYTE)('1' + Size);

       i -= 11;

       while (TRUE)
       {
	  sprintf(Buffer, "%04X", (unsigned int) i);

	  NWFSCopy(&MangledName[BaseLength], Buffer, 4);
	  MangledName[BaseLength + 6] = '.';
	  NWFSCopy(&MangledName[BaseLength + 7], "SET", 3);

	  WorkPath[0] = '\0';
	  strcat(WorkPath, Path);
	  strcat(WorkPath, MangledName);
	  fd = fopen(WorkPath, "rb");
	  if (!fd)
	     break;
	  else
	    fclose(fd);

	  i -= 11;
       }
    }

    if (NewLength)
       *NewLength = strlen(MangledName);

    return 0;

}

ULONG NWImageNetWareVolume(IMAGE *image, ULONG portal,
			   double *total_last, double *total_curr,
			   double *factor)
{
    ULONG ClusterChain;
    LONGLONG FileSize;
    register DOS *dos;
    register MACINTOSH *mac;
    register VOLUME *volume;
    register ULONG DirNo, len, i;
    ULONG totalDirs, ccode, acode;
    register ULONG total, index, update_count = 0;
    register long bytesLeft, bytesToRead, vsize;
    register BYTE *cBuffer, *buf;
    ULONG retCode = 0;
    ULONG current, recordOffset;
    FILE *file, *ptr, *fp;
    BYTE Buffer[20] = { "" };
    BYTE DirectoryName[20] = { "" };
    BYTE ExtDirectoryName[20] = { "" };
    BYTE DataName[20] = { "" };
    LONGLONG DataSize;
    LONGLONG Dir1Size, Dir2Size, Ext1Size, Ext2Size, Fat1Size, Fat2Size;
    double TotalSize = 0, base;
    double ptotal = 0, data = 0, percent = 0, percent_curr = 0, percent_last = 0;
    BYTE displayBuffer[255];
#if (LINUX_UTIL)
    struct termios init_settings, new_settings;
#endif

#if (LINUX_UTIL)
    tcgetattr(0, &init_settings);
#endif

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
       return -1;

    log_sprintf(displayBuffer, "  Imaging Volume [%-15s]   Strip Deleted Files = %s",
	    &image->VolumeName[1],
	    SkipDeletedFiles ? "NO" : "YES");
    write_portal_cleol(portal, displayBuffer, 1, 0, BRITEWHITE | BGBLUE);

    sprintf(displayBuffer, "  Files Processed                    0%% Complete ");
    write_portal_cleol(portal, displayBuffer, 2, 0, BRITEWHITE | BGBLUE);

    sprintf(displayBuffer, "0%%                               50%%                                100%%");
    write_portal_cleol(portal, displayBuffer, 6, 2, BRITEWHITE | BGBLUE);

#if (LINUX_UTIL)
    for (i=0; i < 72; i++)
    {
       if (HasColor)
	  write_portal_char(portal, 176, 7, 2 + i, BRITEWHITE | BGBLUE);
       else
	  write_portal_char(portal, '.', 7, 2 + i, BRITEWHITE | BGBLUE);
    }
#else
    for (i=0; i < 72; i++)
       write_portal_char(portal, 176, 7, 2 + i, BRITEWHITE | BGBLUE);
#endif
    base = 0;
    if (total_curr)
       base = *total_curr;

    sprintf(displayBuffer, "                          Percent Complete");
    write_portal_cleol(portal, displayBuffer, 8, 2, BRITEWHITE | BGBLUE);

    log_sprintf(displayBuffer, "  Mounting Volume %s", &image->VolumeName[1]);
    write_portal_cleol(portal, displayBuffer, 4, 0, BRITEWHITE | BGBLUE);

    sprintf(displayBuffer, "                     ESC/F3,Q,X,A - Abort Imaging Operation");
    write_portal_cleol(portal, displayBuffer, 15, 0, BRITEWHITE | BGBLUE);

    update_static_portal(portal);

    ccode = MountVolumeByHandle(volume);
    if (ccode)
    {
       log_sprintf(displayBuffer, "error mounting volume %s", volume->VolumeName);
       error_portal(displayBuffer, 10);
       return -1;
    }

    DataSize = (LONGLONG)((LONGLONG)volume->VolumeAllocatedClusters *
			  (LONGLONG)volume->ClusterSize);

    // get fat file lengths
    Fat1Size = GetChainSize(volume, volume->FirstFAT);
    if (Fat1Size >= DataSize)
       DataSize -= Fat1Size;

    Fat2Size = GetChainSize(volume, volume->SecondFAT);
    if (Fat2Size >= DataSize)
       DataSize -= Fat2Size;

    ptotal = (double)TotalSize;
    data = (double)DataSize;

    percent = (ptotal / data) * 100;
    if (percent > 100)
       percent = 100;

    percent_curr = (percent * 72) / 100;
    if (total_curr)
    {
       if (factor)
       {
	  if (!(*factor))
	     *factor = 1;
	  *total_curr = base + ((percent * (72 / (*factor))) / 100);
       }
    }

    sprintf(displayBuffer, "  Bytes Processed %-10d         %-3d%% Compressed", (int)TotalSize, 0);
    write_portal_cleol(portal, displayBuffer, 3, 0, BRITEWHITE | BGBLUE);

    sprintf(displayBuffer, "  Files Processed %-10d         %-3d%% Complete ",
	   (int)image->FilesInDataSet,
	   (int)percent);
    write_portal_cleol(portal, displayBuffer, 2, 0, BRITEWHITE | BGBLUE);

#if (LINUX_UTIL)
    for (i=percent_last; (i < percent_curr) && (i < 72); i++)
    {
       if (HasColor)
	  write_portal_char(portal, 219, 7, 2 + i, BRITEWHITE | BGWHITE);
       else
	  write_portal_char(portal, 'X', 7, 2 + i, BRITEWHITE | BGWHITE);
    }

    for (i=(*total_last); (i < (*total_curr)) && (i < 72); i++)
    {
       if (HasColor)
	  write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGWHITE);
       else
	  write_portal_char(portal, 'X', 12, 2 + i, BRITEWHITE | BGWHITE);
    }
#else
    for (i=(ULONG)percent_last; (i < (ULONG)percent_curr) && (i < 72); i++)
       write_portal_char(portal, 219, 7, 2 + i, BRITEWHITE | BGBLUE);

    for (i=(ULONG)(*total_last); (i < (ULONG)(*total_curr)) && (i < 72); i++)
       write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGBLUE);
#endif
    percent_last = percent_curr;
    if (total_last && total_curr)
       *total_last = *total_curr;

    update_static_portal(portal);

    log_sprintf(displayBuffer, "  Verifying Volume Allocation ...");
    write_portal_cleol(portal, displayBuffer, 4, 0, BRITEWHITE | BGBLUE);

    update_static_portal(portal);

    // save the volume directory file

    log_sprintf(displayBuffer, "  Creating Directory Table Archive       :  %s",
	    DirectoryName);
    write_portal_cleol(portal, displayBuffer, 4, 0, BRITEWHITE | BGBLUE);

    update_static_portal(portal);

    WorkPath[0] = '\0';
    strcat(WorkPath, Path);
    strcat(WorkPath, DirectoryName);
    fp = fopen(WorkPath, "wb");
    if (!fp)
    {
       log_sprintf(displayBuffer, "could not create file %s", DirectoryName);
       error_portal(displayBuffer, 10);
       return -1;
    }

    ccode = DumpDirectoryFile(volume, fp, &totalDirs);
    if (ccode == (ULONG) -13)
    {
       log_sprintf(displayBuffer, "                     Imaging Operation is Aborting ....");
       write_portal_cleol(portal, displayBuffer, 15, 0, BRITEWHITE | BGBLUE);
       update_static_portal(portal);
       fclose(fp);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, DirectoryName);
       unlink(WorkPath);
       return -3;
    }
    else
    if (ccode)
    {
       log_sprintf(displayBuffer, "could not dump directory tables");
       error_portal(displayBuffer, 10);
       fclose(fp);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, DirectoryName);
       unlink(WorkPath);
       return -1;
    }
    fclose(fp);

    // get directory file lengths
    Dir1Size = GetChainSize(volume, volume->FirstDirectory);
    TotalSize += Dir1Size;

    Dir2Size = GetChainSize(volume, volume->SecondDirectory);
    TotalSize += Dir2Size;

    ptotal = (double)TotalSize;
    data = (double)DataSize;

    percent = (ptotal / data) * 100;
    if (percent > 100)
       percent = 100;

    percent_curr = (percent * 72) / 100;
    if (total_curr)
    {
       if (factor)
       {
	  if (!(*factor))
	     *factor = 1;
	  *total_curr = base + ((percent * (72 / (*factor))) / 100);
       }
    }

    sprintf(displayBuffer, "  Bytes Processed %-10d         %-3d%% Compressed", (int)TotalSize, 0);
    write_portal_cleol(portal, displayBuffer, 3, 0, BRITEWHITE | BGBLUE);

    sprintf(displayBuffer, "  Files Processed %-10d         %-3d%% Complete ",
	   (int)image->FilesInDataSet,
	   (int)percent);
    write_portal_cleol(portal, displayBuffer, 2, 0, BRITEWHITE | BGBLUE);

#if (LINUX_UTIL)
    for (i=percent_last; (i < percent_curr) && (i < 72); i++)
    {
       if (HasColor)
	  write_portal_char(portal, 219, 7, 2 + i, BRITEWHITE | BGWHITE);
       else
	  write_portal_char(portal, 'X', 7, 2 + i, BRITEWHITE | BGWHITE);
    }

    for (i=(*total_last); (i < (*total_curr)) && (i < 72); i++)
    {
       if (HasColor)
	  write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGWHITE);
       else
	  write_portal_char(portal, 'X', 12, 2 + i, BRITEWHITE | BGWHITE);
    }
#else
    for (i=(ULONG)percent_last; (i < (ULONG)percent_curr) && (i < 72); i++)
       write_portal_char(portal, 219, 7, 2 + i, BRITEWHITE | BGBLUE);

    for (i=(ULONG)(*total_last); (i < (ULONG)(*total_curr)) && (i < 72); i++)
       write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGBLUE);
#endif
    percent_last = percent_curr;
    if (total_last && total_curr)
       *total_last = *total_curr;

    update_static_portal(portal);

    // save the volume extended directory file

    log_sprintf(displayBuffer, "  Creating Extended Directory Archive    :  %s",
	    ExtDirectoryName);
    write_portal_cleol(portal, displayBuffer, 4, 0, BRITEWHITE | BGBLUE);

    update_static_portal(portal);

    WorkPath[0] = '\0';
    strcat(WorkPath, Path);
    strcat(WorkPath, ExtDirectoryName);
    fp = fopen(WorkPath, "wb");
    if (!fp)
    {
       log_sprintf(displayBuffer, "could not create file %s", ExtDirectoryName);
       error_portal(displayBuffer, 10);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, DirectoryName);
       unlink(WorkPath);
       return -1;
    }

    ccode = DumpExtendedDirectoryFile(volume, fp);
    if (ccode == (ULONG) -13)
    {
       log_sprintf(displayBuffer, "                     Imaging Operation is Aborting ....");
       write_portal_cleol(portal, displayBuffer, 15, 0, BRITEWHITE | BGBLUE);
       update_static_portal(portal);
       fclose(fp);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, DirectoryName);
       unlink(WorkPath);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, ExtDirectoryName);
       unlink(WorkPath);
       return -3;
    }
    else
    if (ccode)
    {
       log_sprintf(displayBuffer, "could not dump extended directory tables");
       error_portal(displayBuffer, 10);
       fclose(fp);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, DirectoryName);
       unlink(WorkPath);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, ExtDirectoryName);
       unlink(WorkPath);
       return -1;
    }
    fclose(fp);

    // get extended directory file lengths
    Ext1Size = GetChainSize(volume, volume->ExtDirectory1);
    TotalSize += Ext1Size;

    Ext2Size = GetChainSize(volume, volume->ExtDirectory2);
    TotalSize += Ext2Size;

    ptotal = (double)TotalSize;
    data = (double)DataSize;

    percent = (ptotal / data) * 100;
    if (percent > 100)
       percent = 100;

    percent_curr = (percent * 72) / 100;
    if (total_curr)
    {
       if (factor)
       {
	  if (!(*factor))
	     *factor = 1;
	  *total_curr = base + ((percent * (72 / (*factor))) / 100);
       }
    }

    sprintf(displayBuffer, "  Bytes Processed %-10d         %-3d%% Compressed", (int)TotalSize, 0);
    write_portal_cleol(portal, displayBuffer, 3, 0, BRITEWHITE | BGBLUE);

    sprintf(displayBuffer, "  Files Processed %-10d         %-3d%% Complete ",
	   (int)image->FilesInDataSet,
	   (int)percent);
    write_portal_cleol(portal, displayBuffer, 2, 0, BRITEWHITE | BGBLUE);

#if (LINUX_UTIL)
    for (i=percent_last; (i < percent_curr) && (i < 72); i++)
    {
       if (HasColor)
	  write_portal_char(portal, 219, 7, 2 + i, BRITEWHITE | BGWHITE);
       else
	  write_portal_char(portal, 'X', 7, 2 + i, BRITEWHITE | BGWHITE);
    }

    for (i=(*total_last); (i < (*total_curr)) && (i < 72); i++)
    {
       if (HasColor)
	  write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGWHITE);
       else
	  write_portal_char(portal, 'X', 12, 2 + i, BRITEWHITE | BGWHITE);
    }
#else
    for (i=(ULONG)percent_last; (i < (ULONG)percent_curr) && (i < 72); i++)
       write_portal_char(portal, 219, 7, 2 + i, BRITEWHITE | BGBLUE);

    for (i=(ULONG)(*total_last); (i < (ULONG)(*total_curr)) && (i < 72); i++)
       write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGBLUE);
#endif
    percent_last = percent_curr;
    if (total_last && total_curr)
       *total_last = *total_curr;

    update_static_portal(portal);

    log_sprintf(displayBuffer, "  Creating Compressed Data Archive       :  %s",
	    DataName);
    write_portal_cleol(portal, displayBuffer, 4, 0, BRITEWHITE | BGBLUE);

    update_static_portal(portal);

    WorkPath[0] = '\0';
    strcat(WorkPath, Path);
    strcat(WorkPath, DataName);
    file = fopen(WorkPath, "wb");
    if (!file)
    {
       log_sprintf(displayBuffer, "could not create file %s", DataName);
       error_portal(displayBuffer, 10);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, DirectoryName);
       unlink(WorkPath);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, ExtDirectoryName);
       unlink(WorkPath);
       return -1;
    }

    cBuffer = malloc(volume->ClusterSize);
    if (!cBuffer)
    {
       log_sprintf(displayBuffer, "could not allocate workspace 1 memory");
       error_portal(displayBuffer, 10);
       fclose(file);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, DirectoryName);
       unlink(WorkPath);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, ExtDirectoryName);
       unlink(WorkPath);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, DataName);
       unlink(WorkPath);
       return -1;
    }

    dos = malloc(sizeof(DOS));
    if (!dos)
    {
       log_sprintf(displayBuffer, "could not allocate workspace 2 memory");
       error_portal(displayBuffer, 10);
       free(Buffer);
       fclose(file);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, DirectoryName);
       unlink(WorkPath);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, ExtDirectoryName);
       unlink(WorkPath);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, DataName);
       unlink(WorkPath);
       return -1;
    }

    WorkPath[0] = '\0';
    strcat(WorkPath, Path);
    strcat(WorkPath, DirectoryName);
    ptr = fopen(WorkPath, "rb+wb");
    if (!ptr)
    {
       log_sprintf(displayBuffer, "could not reopen directory file");
       error_portal(displayBuffer, 10);
       free(cBuffer);
       free(dos);
       fclose(file);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, DirectoryName);
       unlink(WorkPath);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, ExtDirectoryName);
       unlink(WorkPath);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, DataName);
       unlink(WorkPath);
       return -1;
    }

    log_sprintf(displayBuffer, "  Imaging Operation in Progress ...");
    write_portal_cleol(portal, displayBuffer, 4, 0, BRITEWHITE | BGBLUE);

    update_static_portal(portal);

    image->FilesInDataSet = 0;
    FileSize = DirNo = total = 0;
    fseek(ptr, 0L, SEEK_SET);

#if (LINUX_UTIL)
    // if we are Linux, we need kbhit(), so we implement a terminal
    // based version of kbhit() so we can detect keystrokes during
    // the imaging operation, and abort the operation if asked to
    // do so by the user.  the code below puts a terminal into raw mode
    // so that is does not block when trying to read from the
    // keyboard device.

    tcgetattr(0, &init_settings);
    new_settings = init_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_settings);
#endif

    while (!feof(ptr))
    {
#if (LINUX_UTIL)
       BYTE nread, ch;

       // put the terminal into raw mode, and see if we have a valid
       // keystroke pending.  if we have a keystroke pending, then
       // see if the user requested that we abort the imaging operation.

       new_settings.c_cc[VMIN] = 0;
       tcsetattr(0, TCSANOW, &new_settings);
       nread = read(0, &ch, 1);
       new_settings.c_cc[VMIN] = 1;
       tcsetattr(0, TCSANOW, &new_settings);

       if (nread)
       {
	  if ((toupper(ch) == 'X') || (toupper(ch) == 'Q') ||
	      (toupper(ch) == ESC) || (toupper(ch) == 'A'))
	  {
	     acode = confirm_menu("  Abort Imaging Operation? ", 10, YELLOW | BGBLUE);
	     if (acode)
	     {
		log_sprintf(displayBuffer, "                     Imaging Operation is Aborting ....");
		write_portal_cleol(portal, displayBuffer, 15, 0, BRITEWHITE | BGBLUE);
		update_static_portal(portal);

		retCode = -3;
		goto shutdown;
	     }
	  }
       }
#else
       BYTE ch;

       // DOS and Windows NT/2000 both support the kbhit() function.

       if (kbhit())
       {
	  ch = getch();
	  if ((toupper(ch) == 'X') || (toupper(ch) == 'Q') ||
	      (toupper(ch) == ESC) || (toupper(ch) == 'A'))
	  {
	     acode = confirm_menu("  Abort Imaging Operation? ", 10, YELLOW | BGBLUE);
	     if (acode)
	     {
		log_sprintf(displayBuffer, "                     Imaging Operation is Aborting ....");
		write_portal_cleol(portal, displayBuffer, 15, 0, BRITEWHITE | BGBLUE);
		update_static_portal(portal);

		retCode = -3;
		goto shutdown;
	     }
	  }
       }
#endif

       current = ftell(ptr);
       fread(dos, sizeof(DOS), 1, ptr);
       DirNo++;

       switch (dos->Subdirectory)
       {
	  case FREE_NODE:
	  case TRUSTEE_NODE:
	  case ROOT_NODE:
	  case RESTRICTION_NODE:
	     break;

	  case SUBALLOC_NODE:
	     // remove this record from the directory archive
	     NWFSSet(dos, 0, sizeof(DOS));
	     dos->Subdirectory = (ULONG) -1;

	     fseek(ptr, current, SEEK_SET);
	     ccode = fwrite(dos, sizeof(DOS), 1, ptr);
	     if (!ccode)
	     {
		log_sprintf(displayBuffer, "error writing suballoc records");
		error_portal(displayBuffer, 10);
		retCode = (ULONG) -1;
		goto shutdown;
	     }
	     fseek(ptr, current + sizeof(DOS), SEEK_SET);
	     break;

	  default:
	     if (dos->Flags & SUBDIRECTORY_FILE)
		break;

	     // check for deleted Netware 3.x and Netware 4.x files
	     if ((dos->Flags & NW3_DELETED_FILE) ||
		 (dos->Flags & NW4_DELETED_FILE))
	     {
		// if we are told to skip deleted files
		if (!SkipDeletedFiles)
		{
		   // remember the size of deleted files and skip over
		   // them.
		   if (dos->NameSpace == DOS_NAME_SPACE)
		      TotalSize += dos->FileSize;

		   // remove this record from the directory archive
		   NWFSSet(dos, 0, sizeof(DOS));
		   dos->Subdirectory = (ULONG) -1;

		   fseek(ptr, current, SEEK_SET);
		   ccode = fwrite(dos, sizeof(DOS), 1, ptr);
		   if (!ccode)
		   {
		      log_sprintf(displayBuffer, "error writing deleted directory record");
		      error_portal(displayBuffer, 10);
		      retCode = (ULONG) -1;
		      goto shutdown;
		   }
		   fseek(ptr, current + sizeof(DOS), SEEK_SET);
		   break;
		}
	     }

	     switch (dos->NameSpace)
	     {
		case DOS_NAME_SPACE:
		   NWFSCopy(Buffer, dos->FileName,
			    (dos->FileNameLength <= 12)
			    ? dos->FileNameLength : 12);
		   Buffer[(dos->FileNameLength <= 12)
			    ? dos->FileNameLength : 12] = '\0';

		   ClusterChain = dos->FirstBlock;
		   if ((ClusterChain) && (ClusterChain != (ULONG) -1))
		   {
		      // round to next 16 byte boundry
		      recordOffset = (((ftell(file) + 15L) / 16L) * 16L);
		      fseek(file, recordOffset, SEEK_SET);

		      dos->FirstBlock = (recordOffset >> 4);
		      FileSize = dos->FileSize;
		   }
		   goto UpdateDirRecord;

		case MAC_NAME_SPACE:
		   mac = (MACINTOSH *) dos;
		   NWFSCopy(Buffer, mac->FileName,
			    (mac->FileNameLength <= 12)
			    ? mac->FileNameLength : 12);
		   Buffer[(mac->FileNameLength <= 12)
			       ? mac->FileNameLength : 12] = '\0';

		   ClusterChain = mac->ResourceFork;
		   if ((ClusterChain) && (ClusterChain != (ULONG) -1))
		   {
		      // round to next 16 byte boundry
		      recordOffset = (((ftell(file) + 15L) / 16L) * 16L);
		      fseek(file, recordOffset, SEEK_SET);

		      mac->ResourceFork = (recordOffset >> 4);
		      FileSize = mac->ResourceForkSize;
		   }

UpdateDirRecord:;
		   fseek(ptr, current, SEEK_SET);
		   ccode = fwrite(dos, sizeof(DOS), 1, ptr);
		   if (!ccode)
		   {
		      log_sprintf(displayBuffer, "error writing updated directory record");
		      error_portal(displayBuffer, 10);
		      retCode = (ULONG) -1;
		      goto shutdown;
		   }
		   fseek(ptr, current + sizeof(DOS), SEEK_SET);

		   TotalSize += FileSize;
		   ptotal = (double)TotalSize;
		   data = (double)DataSize;

		   percent = (ptotal / data) * 100;
		   if (percent > 100)
		      percent = 100;

		   percent_curr = (percent * 72) / 100;
		   if (total_curr)
		   {
		      if (factor)
		      {
			 if (!(*factor))
			    *factor = 1;
			 *total_curr = base + ((percent * (72 / (*factor))) / 100);
		      }
		   }

		   sprintf(displayBuffer, "  Bytes Processed %-10d         %-3d%% Compressed", (int)TotalSize, 0);
		   write_portal_cleol(portal, displayBuffer, 3, 0, BRITEWHITE | BGBLUE);

		   sprintf(displayBuffer, "  Files Processed %-10d         %-3d%% Complete ",
			   (int)image->FilesInDataSet,
			   (int)percent);
		   write_portal_cleol(portal, displayBuffer, 2, 0, BRITEWHITE | BGBLUE);

#if (LINUX_UTIL)
		   for (i=percent_last; (i < percent_curr) && (i < 72); i++)
		   {
		      if (HasColor)
			 write_portal_char(portal, 219, 7, 2 + i, BRITEWHITE | BGWHITE);
		      else
			 write_portal_char(portal, 'X', 7, 2 + i, BRITEWHITE | BGWHITE);
		   }

		   for (i=(*total_last); (i < (*total_curr)) && (i < 72); i++)
		   {
		      if (HasColor)
			 write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGWHITE);
		      else
			 write_portal_char(portal, 'X', 12, 2 + i, BRITEWHITE | BGWHITE);
		   }
#else
		   for (i=(ULONG)percent_last; (i < (ULONG)percent_curr) && (i < 72); i++)
		      write_portal_char(portal, 219, 7, 2 + i, BRITEWHITE | BGBLUE);

		   for (i=(ULONG)(*total_last); (i < (ULONG)(*total_curr)) && (i < 72); i++)
		      write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGBLUE);
#endif
		   percent_last = percent_curr;
		   if (total_last && total_curr)
		      *total_last = *total_curr;

		   if (update_count++ > 50)
		   {
		      update_count = 0;
		      update_static_portal(portal);
		   }

		   if ((ClusterChain) && (ClusterChain != (ULONG) -1))
		   {
		      index = 0;
		      bytesToRead = FileSize;
		      while (bytesToRead > 0)
		      {
			 ccode = NWReadFile(volume,
					 &ClusterChain,
					 0,
					 FileSize,
					 index * IO_BLOCK_SIZE,
					 cBuffer,
					 IO_BLOCK_SIZE,
					 0,
					 0,
					 &retCode,
					 KERNEL_ADDRESS_SPACE,
					 TRUE,
					 0,
					 TRUE);
			 if (retCode)
			 {
			    log_sprintf(displayBuffer,"error reading volume ccode-%d",
				     (int)retCode);
			    error_portal(displayBuffer, 10);
			    goto shutdown;
			 }

			 bytesToRead -= ccode;
			 bytesLeft = ccode;
			 buf = cBuffer;
			 while (bytesLeft > 0)
			 {
			    vsize = ((bytesLeft > IO_BLOCK_SIZE)
				    ? IO_BLOCK_SIZE : bytesLeft);

			    ccode = fwrite(buf, vsize, 1, file);
			    if (!ccode)
			    {
			       log_sprintf(displayBuffer, "error writing to image file");
			       error_portal(displayBuffer, 10);
			       retCode = (ULONG) -1;
			       goto shutdown;
			    }
			    total += vsize;
			    bytesLeft -= vsize;
			    buf += vsize;
			 }
			 index++;
		      }
		   }
		   image->FilesInDataSet++;
		   break;
	     }
	     break;
       }

       if (feof(ptr))
       {
#if (LINUX_UTIL)
	  for (i=percent_last; i < 72; i++)
	  {
	     if (HasColor)
		write_portal_char(portal, 219, 7, 2 + i, BRITEWHITE | BGWHITE);
	     else
		write_portal_char(portal, 'X', 7, 2 + i, BRITEWHITE | BGWHITE);
	  }
#else
	  for (i=(ULONG)percent_last; i < 72; i++)
	     write_portal_char(portal, 219, 7, 2 + i, BRITEWHITE | BGBLUE);
#endif
	  log_sprintf(displayBuffer, "  Files Processed %-10d         %-3d%% Complete ",
			   (int)image->FilesInDataSet, (int)100);
	  write_portal_cleol(portal, displayBuffer, 2, 0, BRITEWHITE | BGBLUE);

	  update_static_portal(portal);
       }
    }

shutdown:;
#if (LINUX_UTIL)
    tcsetattr(0, TCSANOW, &init_settings);
#endif

    free(cBuffer);
    free(dos);
    fclose(ptr);
    fclose(file);

    ccode = DismountVolumeByHandle(volume);
    if (ccode)
    {
       log_sprintf(displayBuffer, "error dismounting volume [%15s]", volume->VolumeName);
       error_portal(displayBuffer, 10);
    }

    image->AllocatedVolumeClusters = (ULONG)(TotalSize +
					    (volume->ClusterSize - 1)) /
					     volume->ClusterSize;
    image->VolumeClusterSize = volume->ClusterSize;
    image->LogicalVolumeClusters = volume->VolumeClusters;

    if (retCode)
    {
       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, DirectoryName);
       unlink(WorkPath);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, ExtDirectoryName);
       unlink(WorkPath);

       WorkPath[0] = '\0';
       strcat(WorkPath, Path);
       strcat(WorkPath, DataName);
       unlink(WorkPath);
    }
    return retCode;

}

//
//
//   Utility Menu Functions
//
//

ULONG ImageMenuFunction(NWSCREEN *screen, ULONG value, BYTE *option, ULONG index)
{
   ULONG portal, ccode, i, k;
   FILE *sfd;
   BYTE displayBuffer[255];
   BYTE SetName[12] = "";
   BYTE WorkSpace[20] = "";
   register ULONG checksum;
   register ULONG *checksum_ptr;
   double total_last = 0, total_curr = 0, factor = 0;
   ULONG handles[7];
   nwvp_payload payload;
   nwvp_vpartition_info vpart_info;

   switch (value)
   {
      case 1:
	 write_screen_comment_line(&ConsoleScreen, " ", BLUE | BGWHITE);

	 {
	    register ULONG len;
#if (LINUX_UTIL | DOS_UTIL)
	    DIR *local_dir;
#endif
#if (WINDOWS_NT_UTIL)
	    HANDLE handle;
	    WIN32_FIND_DATA findData;
#endif

	    WorkPath[0] = '\0';
	    strcat(WorkPath, Path);
	    len = strlen(WorkPath);
	    if (len && ((WorkPath[len - 1] == '\\') || (WorkPath[len - 1] == '/')))
	       WorkPath[len - 1] = '\0';

#if (LINUX_UTIL)
	    if (!len)
	       strcat(WorkPath, "/var/log/");
#endif

#if (LINUX_UTIL | DOS_UTIL)
	    local_dir = opendir(WorkPath);
	    if (!local_dir)
	    {
	       if (strlen(WorkPath) > 15)
		  sprintf(displayBuffer, "  Directory %-15s Not Found.  Create? ",
		       WorkPath);
	       else
		  sprintf(displayBuffer, "  Directory %s Not Found.  Create? ",
		       WorkPath);
	       ccode = confirm_menu(displayBuffer, 10, YELLOW | BGBLUE);
	       if (ccode)
	       {
		  ccode = mkdir(WorkPath, S_IWUSR);
		  if (ccode)
		  {
		     error_portal("*** Could Not Create New Directory ***", 10);
		     sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Volumes");
		     write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
		     break;
		  }
	       }
	    }
	    else
	       closedir(local_dir);
#endif

#if (WINDOWS_NT_UTIL)
	    handle = FindFirstFile(WorkPath, &findData);
	    if (handle == INVALID_HANDLE_VALUE)
	    {
	       if (strlen(WorkPath) > 15)
		  sprintf(displayBuffer, "  Directory %-15s Not Found.  Create? ",
		       WorkPath);
	       else
		  sprintf(displayBuffer, "  Directory %s Not Found.  Create? ",
		       WorkPath);
	       ccode = confirm_menu(displayBuffer, 10, YELLOW | BGBLUE);
	       if (ccode)
	       {
		  ccode = CreateDirectory(WorkPath, 0);
		  if (!ccode)
		  {
		     error_portal("*** Could Not Create New Directory ***", 10);
		     sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Volumes");
		     write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
		     break;
		  }
	       }
	    }
	    else
	       FindClose(handle);
#endif
	 }

OpenImageSet:;
	 WorkPath[0] = '\0';
	 strcat(WorkPath, Path);
	 strcat(WorkPath, ImageSetName);

	 sfd = fopen(WorkPath, "rb");
         if (sfd)
	 {
	    fclose(sfd);
	    sprintf(displayBuffer,
		    "  Data Set %s Exists.  Overwrite Data Set? ",
		    ImageSetName);

	    ccode = confirm_menu(displayBuffer, 10, YELLOW | BGBLUE);
	    if (!ccode)
	    {
	       portal = make_portal(&ConsoleScreen,
				    0,
				    0,
				    11,
				    5,
				    15,
				    75,
				    3,
				    BORDER_DOUBLE,
				    YELLOW | BGBLUE,
				    YELLOW | BGBLUE,
				    BRITEWHITE | BGBLUE,
				    BRITEWHITE | BGBLUE,
				    0,
				    0,
				    0,
				    FALSE);

	       if (!portal)
	       {
#if (LINUX_UTIL)
		  sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Volumes");
#else
		  sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Volumes");
#endif
		  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
		  break;
	       }

	       activate_static_portal(portal);

	       NWFSCopy(SetName, "IMAGE000.SET", 12);
	       SetName[12] = '\0';

	       ccode = CreateUniqueSet(SetName, WorkSpace, 20, 0);
	       if (!ccode)
		  NWFSCopy(SetName, WorkSpace, strlen(WorkSpace));

	       SetName[8] = '\0';
	       ccode = add_field_to_portal(portal, 1, 2, WHITE | BGBLUE,
				 "Enter New Image Set Name: ",
				 strlen("Enter New Image Set Name: "),
				 SetName, 9,
				 0, 0, 0, 0, FIELD_ENTRY);
	       if (ccode)
	       {
		  error_portal("****  error:  could not allocate memory ****", 7);
		  if (portal)
		  {
		     deactivate_static_portal(portal);
		     free_portal(portal);
		  }
#if (LINUX_UTIL)
		  sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Volumes");
#else
		  sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Volumes");
#endif
		  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
		  return 0;
	       }

#if (LINUX_UTIL)
	       sprintf(displayBuffer, "  F3-Exit  ENTER-Accept ");
	       write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
#else
	       sprintf(displayBuffer, "  ESC-Exit  ENTER-Accept ");
	       write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
#endif

	       ccode = input_portal_fields(portal);
	       if (ccode)
	       {
		  if (portal)
		  {
		     deactivate_static_portal(portal);
		     free_portal(portal);
		  }
#if (LINUX_UTIL)
		  sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Volumes");
#else
		  sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Volumes");
#endif
		  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
		  return 0;
	       }

	       if (portal)
	       {
		  deactivate_static_portal(portal);
		  free_portal(portal);
	       }

	       // if the new name is invalid, then exit the loop
	       if ((!SetName[0]) || (SetName[0] == ' ') || (SetName[0] == '.'))
	       {
		  error_portal("*** Invalid Data Set Name ***", 10);
#if (LINUX_UTIL)
		  sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Volumes");
#else
		  sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Volumes");
#endif
		  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
		  break;
	       }

	       for (i=0; (i < strlen(SetName)) && (i < 8); i++)
	       {
		  if ((!SetName[i]) || (SetName[i] == ' ') || (SetName[i] == '.'))
		     break;
		  ImageSetName[i] = toupper(SetName[i]);
	       }

	       ImageSetName[i] = '\0';
	       strcat(ImageSetName, ".SET");

	       sprintf(NameHeader, "  Image Set Filename:  %s         ", ImageSetName);
	       write_portal_header1(nw_portal, NameHeader, BRITEWHITE | BGBLUE);

	       goto OpenImageSet;
	    }
	    else
	    {
	       sprintf(displayBuffer, "  Data Set %s Will Be Overwritten.  Confirm? ",
		       &ImageSetName[0]);
	       ccode = confirm_menu(displayBuffer, 10, YELLOW | BGBLUE);
	       if (ccode)
		  delete_set(&ImageSetName[0]);
	       else
	       {
#if (LINUX_UTIL)
		  sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Volumes");
#else
		  sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Volumes");
#endif
		  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
		  break;
	       }
	    }
	 }

	 WorkPath[0] = '\0';
	 strcat(WorkPath, Path);
	 strcat(WorkPath, ImageSetName);

	 sfd = fopen(WorkPath, "wb");
         if (!sfd)
	 {
	    log_sprintf(displayBuffer, "Error Opening %s For Writing", &ImageSetName[0]);
	    error_portal(displayBuffer, 10);
	 }
	 else
	 {
	    NWFSSet((BYTE *) &set_header, 0, sizeof(set_header));
	    checksum = 0;
	    set_header.Stamp = IMAGE_SET_STAMP;
	    set_header.CheckSum = 0;
	    set_header.VolumeCount = 0;

	    GetDateAndTimeString(&set_header.DateTimeString[0]);

	    for (i=0; i < image_table_count; i++)
	    {
	       if (mark_table[i] != 0)
	       {
		  if (MangleVolumeNames(&image_table[i].VolumeName[1],
					&image_table[i].VolumeFile[0],
					0, 0, 0) == 0)
		  {
		     NWFSCopy(&set_header.VolumeFile[i][0],
				  &image_table[i].VolumeFile[0], 16);

		     set_header.VolumeCount++;
		  }
		  else
		  {
		     log_sprintf(displayBuffer, "error creating unique names -- press <return>");
		     error_portal(displayBuffer, 10);
		     fclose(sfd);
		     goto ImageError1;
		  }
	       }
	    }

	    checksum_ptr = (ULONG *) &set_header;
	    for (i=0; i < (sizeof(IMAGE_SET) / 4); i++)
	    {
	       checksum += checksum_ptr[i];
	    }
	    set_header.CheckSum = ~checksum;
	    fwrite(&set_header, sizeof(IMAGE_SET), 1, sfd);

	    portal = make_portal(&ConsoleScreen,
		       0,
		       0,
		       5,
		       0,
		       ConsoleScreen.nLines - 2,
		       ConsoleScreen.nCols - 1,
		       30,
		       BORDER_SINGLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0,
		       0,
		       0,
		       FALSE);

	    if (!portal)
	    {
               fclose(sfd);
	       goto ImageError1;
	    }

	    activate_static_portal(portal);

	    sprintf(displayBuffer, "0%%                               50%%                                100%%");
	    write_portal_cleol(portal, displayBuffer, 11, 2, BRITEWHITE | BGBLUE);

#if (LINUX_UTIL)
	    for (i=0; i < 72; i++)
	    {
	       if (HasColor)
		  write_portal_char(portal, 176, 12, 2 + i, BRITEWHITE | BGBLUE);
	       else
		  write_portal_char(portal, '.', 12, 2 + i, BRITEWHITE | BGBLUE);
	    }
#else
	    for (i=0; i < 72; i++)
	       write_portal_char(portal, 176, 12, 2 + i, BRITEWHITE | BGBLUE);
#endif

	    sprintf(displayBuffer, "                         Overall Completion");
	    write_portal_cleol(portal, displayBuffer, 13, 2, BRITEWHITE | BGBLUE);
	    update_static_portal(portal);

	    total_last = 0;
	    total_curr = 0;
	    for (i=0; i < image_table_count; i++)
	       if (mark_table[i])
		  factor++;

	    for (i=0; i < image_table_count; i++)
	    {
	       if (mark_table[i] != 0)
	       {
		  sprintf(displayBuffer, "  Volume %s Imaging In Progress ...",
			  &image_table[i].VolumeName[1]);
		  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

		  ccode = NWImageNetWareVolume(&image_table[i], portal,
					       &total_last, &total_curr,
					       &factor);
		  if (ccode)
		  {
		     if (ccode == (ULONG) -3)
			log_sprintf(displayBuffer, "Imaging of Volume %s aborted -- press <return>",
			     &image_table[i].VolumeName[1]);
		     else
			log_sprintf(displayBuffer, "Error imaging volume %s -- press <return>",
			     &image_table[i].VolumeName[1]);
		     error_portal(displayBuffer, 10);
		     fclose(sfd);
		     if (portal)
			free_portal(portal);
		     goto ImageError1;
		  }
		  log_sprintf(displayBuffer, "  Volume %s Imaging Complete ...",
			      &image_table[i].VolumeName[1]);
		  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	       }
	    }
	    fclose(sfd);

	    log_sprintf(displayBuffer, "  Volume Imaging Complete ...");
	    write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

#if (LINUX_UTIL)
	    for (i=total_last; i < 72; i++)
	    {
	       if (HasColor)
		  write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGWHITE);
	       else
		  write_portal_char(portal, 'X', 12, 2 + i, BRITEWHITE | BGWHITE);
	    }
#else
	    for (i=(ULONG)total_last; i < 72; i++)
	       write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGBLUE);
#endif
	    update_static_portal(portal);

	    message_portal("  Volume Imaging Completed -- Press <Return>. ",
			   10, BRITEWHITE | BGBLUE, TRUE);

	    if (portal)
	       free_portal(portal);

	    for (i=0; i < image_table_count; i++)
	    {
	       if (mark_table[i] != 0)
	       {
		  memmove(&image_table[i].ImageSetName[0], &ImageSetName[0], 12);
		  WorkPath[0] = '\0';
		  strcat(WorkPath, Path);
		  strcat(WorkPath, &set_header.VolumeFile[i][0]);

		  sfd = fopen(WorkPath, "wb");
                  if (sfd)
		  {
		     image_table[i].Stamp = IMAGE_FILE_STAMP;
		     GetDateAndTimeString(&image_table[i].DateTimeString[0]);
		     image_table[i].CheckSum = 0;
		     image_table[i].Filler[0] = 0;
		     image_table[i].Filler[1] = 0;
		     checksum = 0;
		     checksum_ptr = (ULONG *) &image_table[i];

		     for (k=0; k < (sizeof(IMAGE) / 4); k++)
			checksum += checksum_ptr[k];
		     image_table[i].CheckSum = ~checksum;
		     fwrite(&image_table[i], sizeof(IMAGE), 1, sfd);
		     fclose(sfd);
		  }
		  else
		  {
		     log_sprintf(displayBuffer, " Error opening %s", &set_header.VolumeFile[i][0]);
		     error_portal(displayBuffer, 10);
		  }
	       }
	    }
	 }
#if (LINUX_UTIL)
	 sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Volumes");
#else
	 sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Volumes");
#endif
	 write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	 break;

ImageError1:;
	 WorkPath[0] = '\0';
	 strcat(WorkPath, Path);
	 strcat(WorkPath, ImageSetName);
	 unlink(WorkPath);

#if (LINUX_UTIL)
	 sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Volumes");
#else
	 sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Volumes");
#endif
	 write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	 break;

      case 2:
	 portal = make_portal(&ConsoleScreen,
		       0,
		       0,
		       11,
		       5,
		       15,
		       75,
		       3,
		       BORDER_DOUBLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0,
		       0,
		       0,
		       FALSE);

	 if (!portal)
	     break;

	 activate_static_portal(portal);

	 for (i=0; i < 8; i++)
	 {
	    if ((!ImageSetName[i]) || (ImageSetName[i] == ' ') || (ImageSetName[i] == '.'))
	       break;
	    SetName[i] = ImageSetName[i];
	 }
	 SetName[i] = '\0';

	 ccode = add_field_to_portal(portal, 1, 2, WHITE | BGBLUE,
			 "Enter Image Set Name: ",
			 strlen("Enter Image Set Name: "),
			 SetName, 9,
			 0, 0, 0, 0, FIELD_ENTRY);
	 if (ccode)
	 {
	    error_portal("****  error:  could not allocate memory ****", 7);
	    if (portal)
	    {
	       deactivate_static_portal(portal);
	       free_portal(portal);
	    }
	    return 0;
	 }

	 ccode = input_portal_fields(portal);
	 if (ccode)
	 {
	    if (portal)
	    {
	       deactivate_static_portal(portal);
	       free_portal(portal);
	    }
	    return 0;
	 }

	 if (portal)
	 {
	    deactivate_static_portal(portal);
	    free_portal(portal);
	 }

	 // if the new name is invalid, then break
	 if ((!SetName[0]) || (SetName[0] == ' ') || (SetName[0] == '.'))
	 {
	    error_portal("*** Invalid Data Set Name ***", 10);
	    break;
	 }

	 for (i=0; (i < strlen(SetName)) && (i < 8); i++)
	 {
	    if ((!SetName[i]) || (SetName[i] == ' ') || (SetName[i] == '.'))
	       break;
	    ImageSetName[i] = toupper(SetName[i]);
	 }
	 ImageSetName[i] = '\0';
	 strcat(ImageSetName, ".SET");

	 sprintf(NameHeader, "  Image Set Filename:  %s         ", ImageSetName);
	 write_portal_header1(nw_portal, NameHeader, BRITEWHITE | BGBLUE);
	 break;

      case 3:
	 ScanForDataSets();
	 menu = make_menu(&ConsoleScreen,
			  " Select Image Set To Delete",
			  8,
			  25,
			  6,
			  BORDER_DOUBLE,
			  YELLOW | BGBLUE,
			  YELLOW | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  0,
			  0,
			  0,
			  TRUE,
			  IMAGE_SET_TABLE_SIZE);
	 if (!menu)
	    break;

	 for (i=0; i < image_set_table_count; i++)
	    add_item_to_menu(menu, &set_file_name_table[i][0], i);

	 ccode = activate_menu(menu);

	 if (menu)
	    free_menu(menu);

	 if (ccode == (ULONG) -1)
	    break;

	 sprintf(displayBuffer,
		 "  This Will Delete All Volume Data for %s.  Continue? ",
		 &set_file_name_table[ccode][0]);

	 i = ccode;
	 ccode = confirm_menu(displayBuffer, 10, YELLOW | BGBLUE);
	 if (ccode)
	 {
	    delete_set(&set_file_name_table[i][0]);

	    // make the volume set name unique if a file with this name
	    // already exists

	    NWFSCopy(ImageSetName, "IMAGE000.SET", 12);
	    ImageSetName[12] = '\0';

	    ccode = CreateUniqueSet(ImageSetName, WorkSpace, 20, 0);
	    if (!ccode)
	       NWFSCopy(ImageSetName, WorkSpace, strlen(WorkSpace));

	    ScanForDataSets();
	    image_table_count = 0;
	    payload.payload_object_count = 0;
	    payload.payload_index = 0;
	    payload.payload_object_size_buffer_size = (7 << 20) + sizeof(handles);
	    payload.payload_buffer = (BYTE *) &handles[0];

	    do
	    {
	       nwvp_vpartition_scan(&payload);
	       for (i=0; i < payload.payload_object_count; i++)
	       {
		  if (nwvp_vpartition_return_info(handles[i], &vpart_info) == 0)
		  {
		     memmove(&image_table[image_table_count].VolumeName[0], &vpart_info.volume_name[0], 16);

		     image_table[image_table_count].VolumeClusterSize = vpart_info.blocks_per_cluster * 4096;
		     image_table[image_table_count].LogicalVolumeClusters = vpart_info.cluster_count;
		     image_table[image_table_count].SegmentCount = vpart_info.segment_count;
		     image_table[image_table_count].MirrorFlag = vpart_info.mirror_flag;

		     if ((valid_table[image_table_count] = ((vpart_info.volume_valid_flag == 0) ? 0xFFFFFFFF : 0)) != 0)
			mark_table[image_table_count] = 1;
		     image_table_count++;
		  }
	       }
	    } while ((payload.payload_index != 0) && (payload.payload_object_count != 0));

	    sprintf(NameHeader, "  Image Set Filename:  %s           ", ImageSetName);
	    write_portal_header1(nw_portal, NameHeader, BRITEWHITE | BGBLUE);

	    clear_portal(nw_portal);

	    for (i=0; i < image_table_count; i++)
	    {
	       sprintf(displayBuffer,
		    "  %s  %2d [%16s]  %6u MB     %2d K    %2d  %s   %s",
		    (mark_table[i] == 0) ? "   " : "***",
		    (int)i,
		    &image_table[i].VolumeName[1],
		    (unsigned int)
		       (((LONGLONG)image_table[i].LogicalVolumeClusters *
			 (LONGLONG)image_table[i].VolumeClusterSize) /
			 (LONGLONG)0x100000),
		    (int)(image_table[i].VolumeClusterSize / 1024),
		    (int)image_table[i].SegmentCount,
		    (image_table[i].MirrorFlag == 0) ?
		    "SINGLE" : "MIRROR",
		    (valid_table[i] == 0) ? "INVALID" : "OK");
	       write_portal_cleol(nw_portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);
	    }
	    update_static_portal(nw_portal);
	 }
	 break;

      default:
	 break;
   }

#if (LINUX_UTIL)
   sprintf(displayBuffer, "  F1-Help  F3-Menu  TAB-Edit Volumes");
#else
   sprintf(displayBuffer, "  F1-Help  ESC-Menu  TAB-Edit Volumes");
#endif
   write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
   return 0;
}

ULONG RestoreMenuFunction(NWSCREEN *screen, ULONG value, BYTE *option, ULONG index)
{
   vpartition_info volume_info;
   BYTE input_buffer[16] = "";
   BYTE *bptr;
   ULONG menu, ccode, i, portal, found_flag, j;
   BYTE displayBuffer[255];
   ULONG vpart_handle, iportal;
   nwvp_free_stats free_stats;
   double total_last = 0, total_curr = 0, factor = 0;
   ULONG handles[7];
   nwvp_payload payload;
   nwvp_vpartition_info vpart_info;

   switch (value)
   {
      case 1:
	 if (!image_table_count)
	 {
	    error_portal("***  No Volume Data Sets to Restore ***", 10);
	    break;
	 }

	 write_screen_comment_line(&ConsoleScreen, " ", BLUE | BGWHITE);
	 portal = make_portal(&ConsoleScreen,
		       0,
		       0,
		       5,
		       0,
		       ConsoleScreen.nLines - 2,
		       ConsoleScreen.nCols - 1,
		       30,
		       BORDER_SINGLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0,
		       0,
		       0,
		       FALSE);

	 if (!portal)
	 {
	    break;
	 }

	 activate_static_portal(portal);

	 sprintf(displayBuffer, "0%%                               50%%                                100%%");
	 write_portal_cleol(portal, displayBuffer, 11, 2, BRITEWHITE | BGBLUE);

#if (LINUX_UTIL)
	 for (i=0; i < 72; i++)
	 {
	    if (HasColor)
	       write_portal_char(portal, 176, 12, 2 + i, BRITEWHITE | BGBLUE);
	    else
	       write_portal_char(portal, '.', 12, 2 + i, BRITEWHITE | BGBLUE);
	 }
#else
	 for (i=0; i < 72; i++)
	    write_portal_char(portal, 176, 12, 2 + i, BRITEWHITE | BGBLUE);
#endif
	 sprintf(displayBuffer, "                         Overall Completion");
	 write_portal_cleol(portal, displayBuffer, 13, 2, BRITEWHITE | BGBLUE);
	 update_static_portal(portal);

	 if (!AutoCreatePartitions && image_table_count)
	    NWConfigAllocateUnpartitionedSpace();

	 for (i = 0; i < image_table_count; i ++)
	 {
	    while (mark_table[i] != 0)
	    {
	       mark_table[i] = 0;
	       NWFSCopy(&target_volume_table[i][0], &image_table[i].VolumeName[0], 16);
	       log_sprintf(displayBuffer, "  Auto creating volume %s ", &target_volume_table[i][1]);

	       found_flag = 0;
	       for (j=0; j < image_table_count; j++)
	       {
		  if (j != i)
		  {
		     if (NWFSCompare(&target_volume_table[j][0], &target_volume_table[i][0], target_volume_table[i][0] + 1) == 0)
		     {
			found_flag = 0xFFFFFFFF;
			break;
		     }
		  }
	       }

	       if (found_flag != 0)
	       {
		  NWFSSet(&target_volume_table[i][0], 0, 16);
		  target_volume_flags[i] = 0;

		  ccode = confirm_menu("Volume already assigned, Rename Target ?",
				       10, YELLOW | BGBLUE);
		  if (!ccode)
		  {
		     log_sprintf(displayBuffer, "Skipping Auto Create for image volume %s ",
			     &image_table[i].VolumeName[1]);
		     write_portal_cleol(portal, displayBuffer, 0, 0, BRITEWHITE | BGBLUE);
		     update_static_portal(portal);
		     break;
		  }

		  iportal = make_portal(&ConsoleScreen,
		       0,
		       0,
		       7,
		       5,
		       11,
		       75,
		       3,
		       BORDER_DOUBLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0,
		       0,
		       0,
		       FALSE);

		  if (!iportal)
		     break;

		  activate_static_portal(iportal);

		  NWFSSet(input_buffer, 0, sizeof(input_buffer));

		  sprintf(displayBuffer, "Enter New Target Volume Name: ");
		  ccode = add_field_to_portal(iportal, 1, 2, WHITE | BGBLUE,
				    displayBuffer, strlen(displayBuffer),
				    input_buffer, sizeof(input_buffer),
				    0, 0, 0, 0, FIELD_ENTRY);
		  if (ccode)
		  {
		     error_portal("****  error:  could not allocate memory ****", 7);
		     if (iportal)
		     {
			deactivate_static_portal(iportal);
			free_portal(iportal);
		     }
		     break;
		  }

		  ccode = input_portal_fields(iportal);
		  if (ccode)
		  {
		     if (iportal)
		     {
			deactivate_static_portal(iportal);
			free_portal(iportal);
		     }
		     break;
		  }

		  if (iportal)
		  {
		     deactivate_static_portal(iportal);
		     free_portal(iportal);
		  }

		  NWFSSet(&target_volume_table[i][0], 0, 16);
		  target_volume_flags[i] = 0;

		  bptr = input_buffer;
		  while (bptr[0] == ' ')
		     bptr ++;

		  while ((*bptr != 0) && (*bptr != 0xA) && (*bptr != 0xD) && (*bptr != ' '))
		  {
		     if ((bptr[0] >= 'a') && (bptr[0] <= 'z'))
			bptr[0] = (bptr[0] - 'a') + 'A';
		     target_volume_table[i][target_volume_table[i][0] + 1] = bptr[0];
		     target_volume_table[i][0] ++;
		     if (target_volume_table[i][0] == 15)
			break;
		     bptr++;
		  }

		  if (target_volume_table[i][0] == 0)
		  {
		     log_sprintf(displayBuffer, "Bad name for Auto-Create volume %s ", &image_table[i].VolumeName[1]);
		     write_portal_cleol(portal, displayBuffer, 0, 0, BRITEWHITE | BGBLUE);
		     update_static_portal(portal);
		     break;
		  }
	       }

	       // attempt to auto-create a volume the same size as the
	       // original volume specified in the image file.

	       NWFSCopy(&volume_info.volume_name[0], &target_volume_table[i][0], 16);
	       volume_info.blocks_per_cluster = image_table[i].VolumeClusterSize / 4096;
	       volume_info.cluster_count = image_table[i].LogicalVolumeClusters;
	       volume_info.segment_count = image_table[i].SegmentCount;

	       if (target_volume_flags[i])
		  volume_info.flags = target_volume_flags[i];
	       else
		  volume_info.flags =
			(SUB_ALLOCATION_ON | NDS_FLAG | NEW_TRUSTEE_COUNT);

AttemptVolumeCreate:;
	       ccode = nwvp_volume_alloc_space_create(&vpart_handle,
			    &volume_info,
			    (image_table[i].MirrorFlag == 0) ? 0 : 1, 1,
			    &free_stats);
	       if (ccode)
	       {
		  switch (ccode)
		  {
		     case NWVP_NOT_ENOUGH_SPACE:
			if ((free_stats.max_unique_blocks >=
			   (image_table[i].AllocatedVolumeClusters *
			    volume_info.blocks_per_cluster)) &&
			    (volume_info.cluster_count >=
			     image_table[i].AllocatedVolumeClusters))
			{
			   if (free_stats.max_unique_blocks >=
			      (image_table[i].LogicalVolumeClusters *
			       volume_info.blocks_per_cluster))
			      volume_info.cluster_count =
				     image_table[i].LogicalVolumeClusters;
			   else
			      volume_info.cluster_count =
				  (free_stats.max_unique_blocks /
				   volume_info.blocks_per_cluster);

			   if (volume_info.cluster_count <
			       image_table[i].AllocatedVolumeClusters)
			      break;

			   goto AttemptVolumeCreate;
			}
			log_sprintf(displayBuffer,
				    "Not enough space/mirrors to create volume %s",
				    &volume_info.volume_name[1]);
			error_portal(displayBuffer, 10);
			break;

		     case NWVP_NOT_ENOUGH_SEGMENTS:
			if (volume_info.segment_count <
			    free_stats.max_unique_segments)
			{
			   volume_info.segment_count++;
			   goto AttemptVolumeCreate;
			}
			log_sprintf(displayBuffer,
				    "Not enough segments/mirrors to fit volume %s",
				    &volume_info.volume_name[1]);
			error_portal(displayBuffer, 10);
			break;

		     case NWVP_TOO_MANY_SEGMENTS:
			if (volume_info.segment_count > 1)
			{
			   volume_info.segment_count--;
			   goto AttemptVolumeCreate;
			}
			log_sprintf(displayBuffer,
				    "Too many segments/mirrors requested for volume %s",
				    &volume_info.volume_name[1]);
			error_portal(displayBuffer, 10);
			break;

		     default:
			break;
		  }
		  log_sprintf(displayBuffer, "Could not create volume [%s]", &target_volume_table[i][1]);
		  write_portal_cleol(portal, displayBuffer, 1, 0, BRITEWHITE | BGBLUE);
		  update_static_portal(portal);

		  NWFSSet(&target_volume_table[i][0], 0, 16);
		  target_volume_flags[i] = 0;
		  goto RestoreExit;
	       }
	       else
		  ScanVolumes();
	    }
	 }


	 total_last = 0;
	 total_curr = 0;
	 for (i=0; i < image_table_count; i++)
	    if (target_volume_table[i][0] != 0)
	       factor++;

	 for (i=0; i < image_table_count; i++)
	 {
	    if (target_volume_table[i][0] != 0)
	    {
	       log_sprintf(displayBuffer, "  Volume %s Restore In Progress ...",
			   &target_volume_table[i][1]);
	       write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

	       NWFSCopy(&image_table[i].VolumeName[0],
			&target_volume_table[i][0], 16);

	       ccode = NWRestoreNetWareVolume(&image_table[i], portal,
					      &total_last, &total_curr,
					      &factor);
	       if (ccode)
	       {
		  if (ccode == (ULONG) -3)
		     log_sprintf(displayBuffer, "Volume %s restore aborted -- press <return>",
			     &image_table[i].VolumeName[1]);
		  else
		     log_sprintf(displayBuffer, "Error restoring volume %s -- press <return>",
			     &image_table[i].VolumeName[1]);
		  error_portal(displayBuffer, 10);
		  goto AbortExit;
	       }
	       log_sprintf(displayBuffer, "  Volume %s Restore Complete ...",
			   &image_table[i].VolumeName[1]);
	       write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	    }
	 }

RestoreExit:;
	 log_sprintf(displayBuffer, "  Volume Restore Complete ...",
		     &image_table[i].VolumeName[1]);
	 write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

#if (LINUX_UTIL)
	 for (i=total_last; i < 72; i++)
	 {
	    if (HasColor)
	       write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGWHITE);
	    else
	       write_portal_char(portal, 'X', 12, 2 + i, BRITEWHITE | BGWHITE);
	 }
#else
	 for (i=(ULONG)total_last; i < 72; i++)
	    write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGBLUE);
#endif
	 update_static_portal(portal);

	 message_portal("  Volume Restore Completed -- Press <Return>. ",
			   10, BRITEWHITE | BGBLUE, TRUE);
	 if (portal)
	    free_portal(portal);

	 clear_portal(nw_portal);

	 sprintf(NameHeader, "  Restore Set Filename:  %s           ", RestoreSetName);
	 write_portal_header1(nw_portal, NameHeader, BRITEWHITE | BGBLUE);

	 for (i=0; i < image_table_count; i++)
	 {
	    sprintf(displayBuffer,
		    " %s %2d  [%-15s] %6u MB    %2d K  %2d  %s [%-15s]",
		    (mark_table[i] == 0) ? "   " : "***",
		    (int)i,
		    &image_table[i].VolumeName[1],
		    (unsigned int)
		       (((LONGLONG)image_table[i].LogicalVolumeClusters *
			 (LONGLONG)image_table[i].VolumeClusterSize) /
			 (LONGLONG)0x100000),
		    (int)(image_table[i].VolumeClusterSize / 1024),
		    (int)image_table[i].SegmentCount,
		    (image_table[i].MirrorFlag == 0) ? "SINGLE" : "MIRROR",
		    &target_volume_table[i][1]);
	    write_portal_cleol(nw_portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);
	 }
	 update_static_portal(nw_portal);
	 break;

AbortExit:;
	 log_sprintf(displayBuffer, "  Volume Restore Aborted ...");
	 write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

#if (LINUX_UTIL)
	 for (i=total_last; i < 72; i++)
	 {
	    if (HasColor)
	       write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGWHITE);
	    else
	       write_portal_char(portal, 'X', 12, 2 + i, BRITEWHITE | BGWHITE);
	 }
#else
	 for (i=(ULONG)total_last; i < 72; i++)
	    write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGBLUE);
#endif
	 update_static_portal(portal);

	 if (portal)
	    free_portal(portal);

	 clear_portal(nw_portal);

	 sprintf(NameHeader, "  Restore Set Filename:  %s           ", RestoreSetName);
	 write_portal_header1(nw_portal, NameHeader, BRITEWHITE | BGBLUE);

	 for (i=0; i < image_table_count; i++)
	 {
	    sprintf(displayBuffer,
		    " %s %2d  [%-15s] %6u MB    %2d K  %2d  %s [%-15s]",
		    (mark_table[i] == 0) ? "   " : "***",
		    (int)i,
		    &image_table[i].VolumeName[1],
		    (unsigned int)
		       (((LONGLONG)image_table[i].LogicalVolumeClusters *
			 (LONGLONG)image_table[i].VolumeClusterSize) /
			 (LONGLONG)0x100000),
		    (int)(image_table[i].VolumeClusterSize / 1024),
		    (int)image_table[i].SegmentCount,
		    (image_table[i].MirrorFlag == 0) ? "SINGLE" : "MIRROR",
		    &target_volume_table[i][1]);
	    write_portal_cleol(nw_portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);
	 }
	 update_static_portal(nw_portal);
	 break;

      case 2:
	 if (!image_table_count)
	 {
	    error_portal("***  No Volume Data Sets to Restore ***", 10);
	    break;
	 }

	 write_screen_comment_line(&ConsoleScreen, " ", BLUE | BGWHITE);
	 portal = make_portal(&ConsoleScreen,
		       0,
		       0,
		       5,
		       0,
		       ConsoleScreen.nLines - 2,
		       ConsoleScreen.nCols - 1,
		       30,
		       BORDER_SINGLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0,
		       0,
		       0,
		       FALSE);

	 if (!portal)
	 {
	    break;
	 }

	 activate_static_portal(portal);

	 sprintf(displayBuffer, "0%%                               50%%                                100%%");
	 write_portal_cleol(portal, displayBuffer, 11, 2, BRITEWHITE | BGBLUE);

#if (LINUX_UTIL)
	 for (i=0; i < 72; i++)
	 {
	    if (HasColor)
	       write_portal_char(portal, 176, 12, 2 + i, BRITEWHITE | BGBLUE);
	    else
	       write_portal_char(portal, '.', 12, 2 + i, BRITEWHITE | BGBLUE);
	 }
#else
	 for (i=0; i < 72; i++)
	    write_portal_char(portal, 176, 12, 2 + i, BRITEWHITE | BGBLUE);
#endif
	 sprintf(displayBuffer, "                         Overall Completion");
	 write_portal_cleol(portal, displayBuffer, 13, 2, BRITEWHITE | BGBLUE);
	 update_static_portal(portal);

	 total_last = 0;
	 total_curr = 0;
	 for (i=0; i < image_table_count; i++)
	    if (target_volume_table[i][0] != 0)
	       factor++;

	 for (i=0; i < image_table_count; i++)
	 {
	    if (target_volume_table[i][0] != 0)
	    {
	       log_sprintf(displayBuffer, "  Volume %s Restore In Progress ...",
			   &target_volume_table[i][1]);
	       write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

	       NWFSCopy(&image_table[i].VolumeName[0],
			&target_volume_table[i][0], 16);

/**********************	       
	       ccode = NWRestoreNetWareVolume(&image_table[i], portal,
					      &total_last, &total_curr,
					      &factor);
	       if (ccode)
	       {
		  if (ccode == (ULONG) -3)
		     log_sprintf(displayBuffer, "Volume %s restore aborted -- press <return>",
			     &image_table[i].VolumeName[1]);
		  else
		     log_sprintf(displayBuffer, "Error restoring volume %s -- press <return>",
			     &image_table[i].VolumeName[1]);
		  error_portal(displayBuffer, 10);
		  goto AbortExit;
	       }
*********************/
	       
	       log_sprintf(displayBuffer, "  Volume %s Restore Complete ...",
			   &image_table[i].VolumeName[1]);
	       write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	    }
	 }

//RestoreExit1:;
	 log_sprintf(displayBuffer, "  Volume Restore Complete ...",
		     &image_table[i].VolumeName[1]);
	 write_screen_comment_line(&ConsoleScreen, displayBuffer, 
			           BLUE | BGWHITE);

#if (LINUX_UTIL)
	 for (i=total_last; i < 72; i++)
	 {
	    if (HasColor)
	       write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGWHITE);
	    else
	       write_portal_char(portal, 'X', 12, 2 + i, BRITEWHITE | BGWHITE);
	 }
#else
	 for (i=(ULONG)total_last; i < 72; i++)
	    write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGBLUE);
#endif
	 update_static_portal(portal);

	 message_portal("  Volume Restore Completed -- Press <Return>. ",
			   10, BRITEWHITE | BGBLUE, TRUE);
	 if (portal)
	    free_portal(portal);

	 clear_portal(nw_portal);

	 sprintf(NameHeader, "  Restore Set Filename:  %s           ", RestoreSetName);
	 write_portal_header1(nw_portal, NameHeader, BRITEWHITE | BGBLUE);

	 for (i=0; i < image_table_count; i++)
	 {
	    sprintf(displayBuffer,
		    " %s %2d  [%-15s] %6u MB    %2d K  %2d  %s [%-15s]",
		    (mark_table[i] == 0) ? "   " : "***",
		    (int)i,
		    &image_table[i].VolumeName[1],
		    (unsigned int)
		       (((LONGLONG)image_table[i].LogicalVolumeClusters *
			 (LONGLONG)image_table[i].VolumeClusterSize) /
			 (LONGLONG)0x100000),
		    (int)(image_table[i].VolumeClusterSize / 1024),
		    (int)image_table[i].SegmentCount,
		    (image_table[i].MirrorFlag == 0) ? "SINGLE" : "MIRROR",
		    &target_volume_table[i][1]);
	    write_portal_cleol(nw_portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);
	 }
	 update_static_portal(nw_portal);
	 break;

//AbortExit1:;
	 log_sprintf(displayBuffer, "  Volume Restore Aborted ...");
	 write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

#if (LINUX_UTIL)
	 for (i=total_last; i < 72; i++)
	 {
	    if (HasColor)
	       write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGWHITE);
	    else
	       write_portal_char(portal, 'X', 12, 2 + i, BRITEWHITE | BGWHITE);
	 }
#else
	 for (i=(ULONG)total_last; i < 72; i++)
	    write_portal_char(portal, 219, 12, 2 + i, BRITEWHITE | BGBLUE);
#endif
	 update_static_portal(portal);

	 if (portal)
	    free_portal(portal);

	 clear_portal(nw_portal);

	 sprintf(NameHeader, "  Restore Set Filename:  %s           ", RestoreSetName);
	 write_portal_header1(nw_portal, NameHeader, BRITEWHITE | BGBLUE);

	 for (i=0; i < image_table_count; i++)
	 {
	    sprintf(displayBuffer,
		    " %s %2d  [%-15s] %6u MB    %2d K  %2d  %s [%-15s]",
		    (mark_table[i] == 0) ? "   " : "***",
		    (int)i,
		    &image_table[i].VolumeName[1],
		    (unsigned int)
		       (((LONGLONG)image_table[i].LogicalVolumeClusters *
			 (LONGLONG)image_table[i].VolumeClusterSize) /
			 (LONGLONG)0x100000),
		    (int)(image_table[i].VolumeClusterSize / 1024),
		    (int)image_table[i].SegmentCount,
		    (image_table[i].MirrorFlag == 0) ? "SINGLE" : "MIRROR",
		    &target_volume_table[i][1]);
	    write_portal_cleol(nw_portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);
	 }
	 update_static_portal(nw_portal);
	 break;

      case 3:
	 ScanForDataSets();
	 menu = make_menu(&ConsoleScreen,
			  " Select New Image Set ",
			  8,
			  27,
			  6,
			  BORDER_DOUBLE,
			  YELLOW | BGBLUE,
			  YELLOW | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  0,
			  0,
			  0,
			  TRUE,
			  IMAGE_SET_TABLE_SIZE);
	 if (!menu)
	    break;

	 for (i=0; i < image_set_table_count; i++)
	    add_item_to_menu(menu, &set_file_name_table[i][0], i);

	 ccode = activate_menu(menu);

	 if (menu)
	    free_menu(menu);

	 if (ccode == (ULONG) -1)
	    break;

	 NWFSCopy(&RestoreSetName[0], &set_file_name_table[ccode][0], 12);
	 for (i=0; i < strlen(RestoreSetName); i++)
	    RestoreSetName[i] = toupper(RestoreSetName[i]);
	 RestoreSetName[12] = '\0';

	 image_table_count = 0;
	 process_set_file(RestoreSetName);

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
	    {
	       NWFSSet(&target_volume_table[i][0], 0, 16);
	       target_volume_flags[i] = 0;
	    }
	 }
	 clear_portal(nw_portal);

	 sprintf(NameHeader, "  Restore Set Filename:  %s           ", RestoreSetName);
	 write_portal_header1(nw_portal, NameHeader, BRITEWHITE | BGBLUE);

	 for (i=0; i < image_table_count; i++)
	 {
	    sprintf(displayBuffer,
		    " %s %2d  [%-15s] %6u MB    %2d K  %2d  %s [%-15s]",
		    (mark_table[i] == 0) ? "   " : "***",
		    (int)i,
		    &image_table[i].VolumeName[1],
		    (unsigned int)
		       (((LONGLONG)image_table[i].LogicalVolumeClusters *
			 (LONGLONG)image_table[i].VolumeClusterSize) /
			 (LONGLONG)0x100000),
		    (int)(image_table[i].VolumeClusterSize / 1024),
		    (int)image_table[i].SegmentCount,
		    (image_table[i].MirrorFlag == 0) ? "SINGLE" : "MIRROR",
		    &target_volume_table[i][1]);
	    write_portal(nw_portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);
	 }

	 update_static_portal(nw_portal);
	 break;

      case 4:
	 ScanForDataSets();
	 menu = make_menu(&ConsoleScreen,
			  " Select Image Set To Delete",
			  8,
			  25,
			  6,
			  BORDER_DOUBLE,
			  YELLOW | BGBLUE,
			  YELLOW | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  0,
			  0,
			  0,
			  TRUE,
			  IMAGE_SET_TABLE_SIZE);
	 if (!menu)
	    break;

	 for (i=0; i < image_set_table_count; i++)
	    add_item_to_menu(menu, &set_file_name_table[i][0], i);

	 ccode = activate_menu(menu);

	 if (menu)
	    free_menu(menu);

	 if (ccode == (ULONG) -1)
	    break;

	 sprintf(displayBuffer,
		 "  This Will Delete All Volume Data for %s.  Continue? ",
		 &set_file_name_table[ccode][0]);

	 i = ccode;
	 ccode = confirm_menu(displayBuffer, 10, YELLOW | BGBLUE);
	 if (ccode)
	 {
	    delete_set(&set_file_name_table[i][0]);

	    ScanForDataSets();
	    image_table_count = 0;
	    if (image_set_table_count)
	    {
	       NWFSCopy(&RestoreSetName[0], &set_file_name_table[0][0], 12);
	       for (i=0; i < strlen(RestoreSetName); i++)
		  RestoreSetName[i] = toupper(RestoreSetName[i]);
	       RestoreSetName[12] = '\0';
	       process_set_file(RestoreSetName);
	    }
	    else
	    {
	       NWFSCopy(&RestoreSetName[0], "IMAGE000.SET", strlen("IMAGE000.SET"));
	       RestoreSetName[12] = '\0';
	       for (i=0; i < strlen(RestoreSetName); i++)
		  RestoreSetName[i] = toupper(RestoreSetName[i]);
	    }

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
	       {
		  NWFSSet(&target_volume_table[i][0], 0, 16);
		  target_volume_flags[i] = 0;
	       }
	    }

	    sprintf(NameHeader, "  Restore Set Filename:  %s           ", RestoreSetName);
	    write_portal_header1(nw_portal, NameHeader, BRITEWHITE | BGBLUE);

	    clear_portal(nw_portal);

	    for (i=0; i < image_table_count; i++)
	    {
	       sprintf(displayBuffer,
		    " %s %2d  [%-15s] %6u MB    %2d K  %2d  %s [%-15s]",
		    (mark_table[i] == 0) ? "   " : "***",
		    (int)i,
		    &image_table[i].VolumeName[1],
		    (unsigned int)
		       (((LONGLONG)image_table[i].LogicalVolumeClusters *
			 (LONGLONG)image_table[i].VolumeClusterSize) /
			 (LONGLONG)0x100000),
		    (int)(image_table[i].VolumeClusterSize / 1024),
		    (int)image_table[i].SegmentCount,
		    (image_table[i].MirrorFlag == 0) ? "SINGLE" : "MIRROR",
		    &target_volume_table[i][1]);
	       write_portal_cleol(nw_portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);
	    }
	    update_static_portal(nw_portal);
	 }
	 break;

      default:
	 break;
   }

#if (LINUX_UTIL)
   sprintf(displayBuffer, "  F1-Help  F3-Menu  TAB-Edit Volumes");
#else
   sprintf(displayBuffer, "  F1-Help  ESC-Menu  TAB-Edit Volumes");
#endif
   write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

   return 0;
}

ULONG ImagePortalFunction(NWSCREEN *screen, ULONG value, BYTE *option, ULONG index)
{
   return 0;
}

ULONG RestorePortalFunction(NWSCREEN *screen, ULONG value, BYTE *option, ULONG index)
{
    register ULONG i, len, ccode, portal;
    BYTE displayBuffer[255];
    BYTE portalHeader[255];
    BYTE volnameBuffer[16] = { "" };
    BYTE sizeBuffer[16] = { "" };
    BYTE vtBuffer[10] = { "" };
    BYTE saBuffer[6] = { "" };
    BYTE fcBuffer[6] = { "" };
    BYTE vaBuffer[6] = { "" };
    BYTE dmBuffer[6] = { "" };
    BYTE vsizeBuffer[12] = "";
    BYTE segBuffer[3] = "";
    BYTE mirrorBuffer[8] = "";
    BYTE targetBuffer[16] = "";
    ULONG vtFlag, saFlag, fcFlag, ndsFlag, dmFlag, adFlag;
    ULONG mirror_sel = 0;
    ULONG cluster_sel = 4, cluster_size, segments;
    BYTE *mirrorMenu[]= { "SINGLE  ", "MIRROR  " };
    LONGLONG volumeLogicalSize, volumeDataSize, volumeRequestSize;
    vpartition_info volume_info;
    ULONG vpart_handle;
    nwvp_free_stats free_stats;

    NWFSSet(volnameBuffer, 0, 16);
    NWFSSet(sizeBuffer, 0 ,16);
    NWFSSet(saBuffer, 0, 6);
    NWFSSet(fcBuffer, 0, 6);
    NWFSSet(vaBuffer, 0, 6);
    NWFSSet(dmBuffer, 0, 6);
    NWFSSet(vsizeBuffer, 0, 12);
    NWFSSet(segBuffer, 0, 3);
    NWFSSet(mirrorBuffer, 0, 8);
    NWFSSet(targetBuffer, 0, 16);
    NWFSSet(displayBuffer, 0, 255);

    if (index < image_table_count)
    {
       vtFlag = saFlag = fcFlag = ndsFlag = dmFlag = adFlag = TRUE;

       sprintf(portalHeader, "  Volume Image Settings for %s ",
	       &image_table[index].VolumeName[1]);

       portal = make_portal(&ConsoleScreen,
		       portalHeader,
		       0,
		       5,
		       8,
		       22,
		       72,
		       14,
		       BORDER_DOUBLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0,
		       0,
		       0,
		       FALSE);

       if (!portal)
	  return 0;

#if (LINUX_UTIL)
       sprintf(displayBuffer, " [TAB,UP,DOWN]-Next/Prev  F10-Accept  F3-Back");
       write_portal_cleol(portal, displayBuffer, 13, 0, BLUE | BGWHITE);
#else
       sprintf(displayBuffer, " [TAB,UP,DOWN]-Next/Prev  F10-Accept  ESC-Back");
       write_portal_cleol(portal, displayBuffer, 13, 0, BLUE | BGWHITE);
#endif
       activate_static_portal(portal);

       len = strlen(image_table[index].VolumeName);
       if (len > 15)
	  len = 15;
       NWFSCopy(volnameBuffer, &image_table[index].VolumeName[1], len);
       volnameBuffer[len - 1] = '\0';

       ccode = add_field_to_portal(portal, 1, 2, WHITE | BGBLUE,
				     "Volume Name               : ",
			      strlen("Volume Name               : "),
			      volnameBuffer,
			      sizeof(volnameBuffer),
			      0, 0, 0, 0, 0);


       switch (image_table[index].VolumeClusterSize)
       {
	  case 4096:
	     cluster_sel = 0;
	     break;

	  case 8192:
	     cluster_sel = 1;
	     break;

	  case 16384:
	     cluster_sel = 2;
	     break;

	  case 32768:
	     cluster_sel = 3;
	     break;

	  case 65536:
	     cluster_sel = 4;
	     break;

	  default:
	     cluster_sel = 4;
	     break;
       }

       ccode = add_field_to_portal(portal, 2, 2, WHITE | BGBLUE,
				     "Volume Block Size (Kbytes): ",
			      strlen("Volume Block Size (Kbytes): "),
			      sizeBuffer,
			      sizeof(sizeBuffer),
			      BlockSizes,
			      5,
			      cluster_sel,
			      &cluster_sel, 0);

       ccode = add_field_to_portal(portal, 3, 2, WHITE | BGBLUE,
				     "Volume Version            : ",
			      strlen("Volume Version            : "),
			      vtBuffer,
			      sizeof(vtBuffer),
			      VolumeType, 2, 1, &vtFlag, 0);

       ccode = add_field_to_portal(portal, 4, 2, WHITE | BGBLUE,
				     "Volume Suballocation      : ",
			      strlen("Volume Suballocation      : "),
			      saBuffer,
			      sizeof(saBuffer),
			      OnOff, 2, 0, &saFlag, 0);

       ccode = add_field_to_portal(portal, 5, 2, WHITE | BGBLUE,
				     "Volume Compression        : ",
			      strlen("Volume Compression        : "),
			      fcBuffer,
			      sizeof(fcBuffer),
			      OnOff, 2, 0, &fcFlag, 0);

       ccode = add_field_to_portal(portal, 6, 2, WHITE | BGBLUE,
				     "Volume Auditing           : ",
			      strlen("Volume Auditing           : "),
			      vaBuffer,
			      sizeof(vaBuffer),
			      OnOff, 2, 1, &adFlag, 0);

       ccode = add_field_to_portal(portal, 7, 2, WHITE | BGBLUE,
				     "Volume Data Migration     : ",
			      strlen("Volume Data Migration     : "),
			      dmBuffer,
			      sizeof(dmBuffer),
			      OnOff, 2, 1, &dmFlag, 0);

       sprintf(segBuffer, "%-d", (int)image_table[index].SegmentCount);
       ccode = add_field_to_portal(portal, 8, 2, WHITE | BGBLUE,
				     "Number Of Segments        : ",
			      strlen("Number Of Segments        : "),
			      segBuffer,
			      sizeof(segBuffer),
			      0, 0, 0, 0, 0);

       ccode = add_field_to_portal(portal, 9, 2, WHITE | BGBLUE,
				     "Mirroring Status          : ",
			      strlen("Mirroring Status          : "),
			      mirrorBuffer,
			      sizeof(mirrorBuffer),
			      mirrorMenu, 2,
			      image_table[index].MirrorFlag ? 1 : 0,
			      &mirror_sel, 0);

       sprintf(vsizeBuffer, "%-d", (unsigned)
	      (((LONGLONG)image_table[index].VolumeClusterSize *
	       (LONGLONG)image_table[index].LogicalVolumeClusters) /
	       (LONGLONG)0x100000));
       ccode = add_field_to_portal(portal, 10, 2, WHITE | BGBLUE,
				     "Volume Size (MB)          : ",
			      strlen("Volume Size (MB)          : "),
			      vsizeBuffer,
			      sizeof(vsizeBuffer),
			      0, 0, 0, 0, 0);

       len = strlen(&target_volume_table[index][1]);
       if (len > 15)
	  len = 15;
       NWFSCopy(targetBuffer, &target_volume_table[index][1], len);
       targetBuffer[len] = '\0';

       ccode = add_field_to_portal(portal, 11, 2, WHITE | BGBLUE,
				     "Target Volume             : ",
			      strlen("Target Volume             : "),
			      targetBuffer,
			      sizeof(targetBuffer),
			      0, 0, 0, 0, 0);

       ccode = input_portal_fields(portal);
       if (ccode)
       {
	  if (portal)
	  {
	     deactivate_static_portal(portal);
	     free_portal(portal);
	  }
	  return 0;
       }

       if (portal)
       {
	  deactivate_static_portal(portal);
	  free_portal(portal);
       }

       switch (cluster_sel)
       {
	  case 0:
	     cluster_size = 4096;
	     break;

	  case 1:
	     cluster_size = 8192;
	     break;

	  case 2:
	     cluster_size = 16384;
	     break;

	  case 3:
	     cluster_size = 32768;
	     break;

	  case 4:
	     cluster_size = 65536;
	     break;

	  default:
	     error_portal("*** Volume Cluster Size is Invalid ***", 10);
	     return 0;
       }

       target_volume_flags[index] = 0;
       if (vtFlag)
       {
	  target_volume_flags[index] |= NEW_TRUSTEE_COUNT;
	  if (!saFlag)
	     target_volume_flags[index] |= SUB_ALLOCATION_ON;
	  if (!fcFlag)
	     target_volume_flags[index] |= FILE_COMPRESSION_ON;
	  if (!ndsFlag)
	     target_volume_flags[index] |= NDS_FLAG;
       }
       else
	  target_volume_flags[index] |= 0x80000000;

       if (!dmFlag)
	  target_volume_flags[index] |= DATA_MIGRATION_ON;
       if (!adFlag)
	  target_volume_flags[index] |= AUDITING_ON;

       segments = atoi(segBuffer);
       if ((segments < 1) || (segments > 32))
       {
	  error_portal("*** segments must be from 1 to 32 ***", 10);
	  return 0;
       }

       volumeLogicalSize = (LONGLONG)
		((LONGLONG)image_table[index].VolumeClusterSize *
		 (LONGLONG)image_table[index].LogicalVolumeClusters);

       volumeDataSize = (LONGLONG)
		((LONGLONG)image_table[index].VolumeClusterSize *
		 (LONGLONG)image_table[index].AllocatedVolumeClusters);

       volumeRequestSize = (LONGLONG)
		((LONGLONG)atol(vsizeBuffer) * (LONGLONG)0x100000);

       if (volumeRequestSize < volumeDataSize)
       {
	  log_sprintf(displayBuffer,
		     " Requested volume size invalid (must be > %dMB)",
		     (int)(((LONGLONG)image_table[index].VolumeClusterSize *
			    (LONGLONG)image_table[index].AllocatedVolumeClusters) /
			    (LONGLONG)0x100000));
	  error_portal(displayBuffer, 10);
	  return 0;
       }

       // uppercase string
       for (i=0; i < strlen(targetBuffer); i++)
	  targetBuffer[i] = toupper(targetBuffer[i]);

       // check if the requested target volume exists
       if (targetBuffer[0])
       {
	  if (!GetVolumeHandle(targetBuffer))
	  {
	     register ULONG menu;
	     register VOLUME *volume;
	     BYTE VolumeName[20] = "";

	     sprintf(displayBuffer,
		  "  Target Volume %s Not Found. Select From List? ", targetBuffer);
	     ccode = confirm_menu(displayBuffer, 10, YELLOW | BGBLUE);
	     if (ccode)
	     {
		menu = make_menu(&ConsoleScreen,
			  " Select a Target Volume ",
			  8,
			  26,
			  6,
			  BORDER_DOUBLE,
			  YELLOW | BGBLUE,
			  YELLOW | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  0,
			  0,
			  0,
			  TRUE,
			  MAX_VOLUMES);
		if (menu)
		{
		   for (i=0; i < MaximumNumberOfVolumes; i++)
		   {
		      if (VolumeTable[i])
		      {
			 volume = VolumeTable[i];
			 if (volume->VolumePresent)
			    add_item_to_menu(menu, volume->VolumeName, i);
		      }
		   }
		   ccode = activate_menu(menu);

		   if (menu)
		      free_menu(menu);

		   if (ccode == (ULONG) -1)
		      return 0;

		   len = strlen(VolumeTable[ccode]->VolumeName);
		   if (len > 15)
		      len = 15;

		   NWFSSet(VolumeName, 0, sizeof(VolumeName));
		   NWFSCopy(VolumeName, VolumeTable[ccode]->VolumeName, len);

		   for (i = 0; i < image_table_count; i++)
		   {
		      if ((i != index) && (len == target_volume_table[i][0]))
		      {
			 if (!NWFSCompare(&target_volume_table[i][1], VolumeName, len))
			 {
			    log_sprintf(displayBuffer,
					"  Target Volume %s Already Assigned ",
					VolumeName);
			    error_portal(displayBuffer, 8);
			    return 0;
			 }
		      }
		   }

		   len = strlen(VolumeName);
		   if (len > 15)
		      len = 15;
		   target_volume_table[index][0] = (BYTE) len;
		   NWFSCopy(&target_volume_table[index][1], VolumeName, len);
		   target_volume_table[index][len + 1] = '\0';
		   mark_table[index] = 0;
		}
		else
		{
		   error_portal("*** could not create menu ***", 10);
		   return 0;
		}
	     }
	     else
	     {
		sprintf(displayBuffer, "*** Volume %s not found ***", targetBuffer);
		error_portal(displayBuffer, 10);
		return 0;
	     }
	  }
	  else
	  {
	     len = strlen(targetBuffer);
	     if (len > 15)
		len = 15;
	     target_volume_table[index][0] = (BYTE) len;
	     NWFSCopy(&target_volume_table[index][1], targetBuffer, len);
	     target_volume_table[index][len + 1] = '\0';
	     mark_table[index] = 0;
	  }
       }

       // see if the requested volume is already assigned
       if (targetBuffer[0])
       {
	  len = strlen(targetBuffer);
	  for (i = 0; i < image_table_count; i++)
	  {
	     if ((i != index) && (len == target_volume_table[i][0]))
	     {
		// if already assigned as target volume, skip
		if (!NWFSCompare(&target_volume_table[i][1], targetBuffer, len))
		{
		   log_sprintf(displayBuffer,
			    "  Target Volume %s Already Assigned ",
			    targetBuffer);
		   error_portal(displayBuffer, 8);
		   return 0;
		}
	     }
	  }
       }

       // attempt to validate volumes we plan to create and make
       // certain there are sufficient segments/mirrors/space for
       // any volumes flagged auto-create.

       if (mark_table[index])
       {
	  for (i=0; i < strlen(volnameBuffer); i++)
	     volnameBuffer[i] = toupper(volnameBuffer[i]);

	  if (GetVolumeHandle(volnameBuffer))
	  {
	     log_sprintf(displayBuffer, " Volume %s Exists (AutoCreate) ",
			 volnameBuffer);
	     error_portal(displayBuffer, 10);
	     return 0;
	  }

	  len = strlen(volnameBuffer);
	  for (i=0; i < image_table_count; i++)
	  {
	     if ((i != index) && (mark_table[i]))
	     {
		if (len == (BYTE)image_table[i].VolumeName[0])
		{
		   if (!NWFSCompare(&image_table[i].VolumeName[1],
				    volnameBuffer, len))
		   {
		      log_sprintf(displayBuffer, " Volume Name %s In Use (AutoCreate) ",
				  volnameBuffer);
		      error_portal(displayBuffer, 10);
		      return 0;
		   }
		}
	     }
	  }

	  NWFSSet(&image_table[index].VolumeName[1], 0, 16);
	  NWFSCopy(&image_table[index].VolumeName[1], volnameBuffer, len);
	  image_table[index].VolumeName[0] = (BYTE) len;

	  NWFSCopy(&volume_info.volume_name[0],
		   &image_table[index].VolumeName[0], 16);

	  volume_info.blocks_per_cluster = image_table[index].VolumeClusterSize / IO_BLOCK_SIZE;
	  volume_info.cluster_count = image_table[index].LogicalVolumeClusters;
	  volume_info.segment_count = segments;
	  volume_info.flags = 0;

	  ccode = nwvp_volume_alloc_space_create(&vpart_handle,
						 &volume_info,
						 (mirror_sel ? 1 : 0),
						 0,
						 &free_stats);
	  switch (ccode)
	  {
	     case NWVP_NOT_ENOUGH_SPACE:
		error_portal("*** insufficient space to create volume", 10);
		return 0;

	     case NWVP_NOT_ENOUGH_SEGMENTS:
		error_portal("*** segment count too small to create volume ***", 10);
		return 0;

	     case NWVP_TOO_MANY_SEGMENTS:
		error_portal("*** segments too high for available space ***", 10);
		return 0;

	     default:
		break;
	  }

	  if (((LONGLONG)free_stats.max_unique_blocks * IO_BLOCK_SIZE) <
	       (LONGLONG)volumeRequestSize)
	  {
	     log_sprintf(displayBuffer,
		" Requested volume size invalid (must be < %dMB)",
		(int)(((LONGLONG)free_stats.max_unique_blocks *
		       (LONGLONG)IO_BLOCK_SIZE) /
		       (LONGLONG)0x100000));
	     error_portal(displayBuffer, 10);
	     return 0;
	  }
       }

       // wait until we have passed all consistency checks prior to
       // changing the data in the image table structure

       image_table[index].MirrorFlag = (mirror_sel ? 1 : 0);
       image_table[index].SegmentCount = segments;
       image_table[index].VolumeClusterSize = cluster_size;
       image_table[index].LogicalVolumeClusters = (ULONG)
	     ((LONGLONG)volumeRequestSize / (LONGLONG)cluster_size);
       image_table[index].AllocatedVolumeClusters = (ULONG)
	     ((LONGLONG)volumeDataSize / (LONGLONG)cluster_size);
    }

    clear_portal(nw_portal);

    for (i=0; i < image_table_count; i++)
    {
       sprintf(displayBuffer,
		    " %s %2d  [%-15s] %6u MB    %2d K  %2d  %s [%-15s]",
		    (mark_table[i] == 0) ? "   " : "***",
		    (int)i,
		    &image_table[i].VolumeName[1],
		    (unsigned int)
		       (((LONGLONG)image_table[i].LogicalVolumeClusters *
			 (LONGLONG)image_table[i].VolumeClusterSize) /
			 (LONGLONG)0x100000),
		    (int)(image_table[i].VolumeClusterSize / 1024),
		    (int)image_table[i].SegmentCount,
		    (image_table[i].MirrorFlag == 0) ? "SINGLE" : "MIRROR",
		    &target_volume_table[i][1]);
       write_portal_cleol(nw_portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);
    }
    update_static_portal(nw_portal);
    return 0;

}

ULONG ImagePortalKeyFunction(NWSCREEN *screen, ULONG key, ULONG index)
{
    register ULONG i, help;
    BYTE displayBuffer[255];

    switch (key)
    {
       case F1:
#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F3-Exit  PgUp-Scroll Up  PgDn-Scroll Down ");
#else
	  sprintf(displayBuffer, "  ESC-Exit  PgUp-Scroll Up  PgDn-Scroll Down ");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

	  help = make_portal(&ConsoleScreen,
		       " Help for Editing Imaging Settings",
		       0,
		       4,
		       0,
		       ConsoleScreen.nLines - 4,
		       ConsoleScreen.nCols - 1,
		       400,
		       BORDER_SINGLE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       0,
		       0,
		       0,
		       TRUE);
	  if (!help)
	     break;

	  for (i=0; i < sizeof(ImageSettingsHelp) / sizeof(BYTE *); i++)
	     write_portal_cleol(help, ImageSettingsHelp[i], i, 0, BLACK | BGWHITE);

	  activate_portal(help);
	  if (help)
	     free_portal(help);

#if (LINUX_UTIL)
	     sprintf(displayBuffer, "  F1-Help  F3-Menu  SPACE-Mark/Clear Volume");
#else
	     sprintf(displayBuffer, "  F1-Help  ESC-Menu  SPACE-Mark/Clear Volume");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	  break;

       case SPACE:
	  if (index < image_table_count)
	  {
	     if (valid_table[index] != 0)
		mark_table[index] ++;
	     mark_table[index] &= 0x00000001;
	  }
	  break;

       default:
	  break;
    }

    for (i=0; i < image_table_count; i++)
    {
       sprintf(displayBuffer,
	    "  %s  %2d [%16s]  %6u MB     %2d K    %2d  %s   %s",
	    (mark_table[i] == 0) ? "   " : "***",
	    (int)i,
	    &image_table[i].VolumeName[1],
	    (unsigned int)
	       (((LONGLONG)image_table[i].LogicalVolumeClusters *
		 (LONGLONG)image_table[i].VolumeClusterSize) /
		 (LONGLONG)0x100000),
	    (int)(image_table[i].VolumeClusterSize / 1024),
	    (int)image_table[i].SegmentCount,
	    (image_table[i].MirrorFlag == 0) ?
	    "SINGLE" : "MIRROR",
	    (valid_table[i] == 0) ? "INVALID" : "OK");
	write_portal_cleol(nw_portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);
    }
    return 0;
}

ULONG RestorePortalKeyFunction(NWSCREEN *screen, ULONG key, ULONG index)
{
    register ULONG i, help, menu, ccode, len;
    register VOLUME *volume;
    BYTE VolumeName[20] = "";
    BYTE displayBuffer[255];

    switch (key)
    {
       case F1:
#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F3-Exit  PgUp-Scroll Up  PgDn-Scroll Down ");
#else
	  sprintf(displayBuffer, "  ESC-Exit  PgUp-Scroll Up  PgDn-Scroll Down ");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

	  help = make_portal(&ConsoleScreen,
		       " Help for Editing Restore Settings ",
		       0,
		       4,
		       0,
		       ConsoleScreen.nLines - 4,
		       ConsoleScreen.nCols - 1,
		       400,
		       BORDER_SINGLE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       0,
		       0,
		       0,
		       TRUE);
	  if (!help)
	     break;

	  for (i=0; i < sizeof(RestoreSettingsHelp) / sizeof(BYTE *); i++)
	     write_portal_cleol(help, RestoreSettingsHelp[i], i, 0, BLACK | BGWHITE);

	  activate_portal(help);
	  if (help)
	     free_portal(help);

#if (LINUX_UTIL)
	     sprintf(displayBuffer, " F1-Help  F3-Menu  SPACE-Mark  DEL-Clr Targ  INS-Sel Vol  ENTER-Edit Vol");
#else
	     sprintf(displayBuffer, " F1-Help  ESC-Menu  SPACE-Mark  DEL-Clr Targ  INS-Sel Vol  ENTER-Edit Vol");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	  break;

       case SPACE:
	  if (index < image_table_count)
	  {
	     if (!target_volume_table[index][1])
	     {
		if (GetVolumeHandle(&image_table[index].VolumeName[1]))
		{
		    log_sprintf(displayBuffer, "  Volume %s Exists - Cannot Mark For Auto-Create. ",
			    &image_table[index].VolumeName[1]);
		    error_portal(displayBuffer, 8);
		}
		else
		{
		   if (valid_table[index])
		      mark_table[index]++;
		   mark_table[index] &= 0x00000001;
		}
	     }
	     else
	     {
		log_sprintf(displayBuffer, "  Image is already assigned to Volume %s  ",
		     &target_volume_table[index][1]);
		error_portal(displayBuffer, 8);
	     }
	  }
	  break;

       case DEL:
	  if (index < image_table_count)
	  {
	     if (target_volume_table[index][1])
	     {
		mark_table[index] = 0;
		NWFSSet(&target_volume_table[index][0], 0, 16);
		target_volume_flags[index] = 0;
	     }
	  }
	  break;

       case INS:
	  menu = make_menu(&ConsoleScreen,
			  " Select a Target Volume ",
			  8,
			  26,
			  6,
			  BORDER_DOUBLE,
			  YELLOW | BGBLUE,
			  YELLOW | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  BRITEWHITE | BGBLUE,
			  0,
			  0,
			  0,
			  TRUE,
			  MAX_VOLUMES);
	  if (!menu)
	     break;

	  for (i=0; i < MaximumNumberOfVolumes; i++)
	  {
	     if (VolumeTable[i])
	     {
		volume = VolumeTable[i];
		if (volume->VolumePresent)
		   add_item_to_menu(menu, volume->VolumeName, i);
	     }
	  }

	  ccode = activate_menu(menu);

	  if (menu)
	     free_menu(menu);

	  if (ccode == (ULONG) -1)
	     return 0;

	  len = strlen(VolumeTable[ccode]->VolumeName);
	  if (len > 15)
	     len = 15;

	  NWFSSet(VolumeName, 0, sizeof(VolumeName));
	  NWFSCopy(VolumeName, VolumeTable[ccode]->VolumeName, len);

	  for (i = 0; i < image_table_count; i++)
	  {
	     if ((i != index) && (len == target_volume_table[i][0]))
	     {
		if (!NWFSCompare(&target_volume_table[i][1], VolumeName, len))
		{
		   log_sprintf(displayBuffer, "  Target Volume %s Already Assigned ",
			VolumeName);
		   error_portal(displayBuffer, 8);
		   return 0;
		}
	     }
	  }

	  target_volume_table[index][0] = (BYTE) len;
	  NWFSCopy(&target_volume_table[index][1], VolumeName, len);
	  target_volume_flags[index] = 0;
	  target_volume_table[index][len + 1] = '\0';
	  mark_table[index] = 0;
	  break;

       default:
	  break;
    }

    for (i=0; i < image_table_count; i++)
    {
       sprintf(displayBuffer,
		    " %s %2d  [%-15s] %6u MB    %2d K  %2d  %s [%-15s]",
		    (mark_table[i] == 0) ? "   " : "***",
		    (int)i,
		    &image_table[i].VolumeName[1],
		    (unsigned int)
		       (((LONGLONG)image_table[i].LogicalVolumeClusters *
			 (LONGLONG)image_table[i].VolumeClusterSize) /
			 (LONGLONG)0x100000),
		    (int)(image_table[i].VolumeClusterSize / 1024),
		    (int)image_table[i].SegmentCount,
		    (image_table[i].MirrorFlag == 0) ? "SINGLE" : "MIRROR",
		    &target_volume_table[i][1]);
       write_portal_cleol(nw_portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);
    }
    return 0;
}

ULONG ImageMenuKeyHandler(NWSCREEN *screen, ULONG key, ULONG index)
{
    register ULONG help, i;
    BYTE displayBuffer[255];

    switch (key)
    {
       case TAB:
	  if (nw_portal)
	  {
#if (LINUX_UTIL)
	     sprintf(displayBuffer, "  F1-Help  F3-Menu  SPACE-Mark/Clear Volume");
#else
	     sprintf(displayBuffer, "  F1-Help  ESC-Menu  SPACE-Mark/Clear Volume");
#endif
	     write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

	     get_portal_resp(nw_portal);

#if (LINUX_UTIL)
	     sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Volumes");
#else
	     sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Volumes");
#endif
	     write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	  }
	  break;

       case F1:
#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F3-Exit  PgUp-Scroll Up  PgDn-Scroll Down ");
#else
	  sprintf(displayBuffer, "  ESC-Exit  PgUp-Scroll Up  PgDn-Scroll Down ");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

	  help = make_portal(&ConsoleScreen,
		       " Help for Imaging NetWare Volumes ",
		       0,
		       4,
		       0,
		       ConsoleScreen.nLines - 4,
		       ConsoleScreen.nCols - 1,
		       400,
		       BORDER_SINGLE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       0,
		       0,
		       0,
		       TRUE);
	  if (!help)
	     break;

	  for (i=0; i < sizeof(ImageHelp) / sizeof(BYTE *); i++)
	     write_portal_cleol(help, ImageHelp[i], i, 0, BLACK | BGWHITE);

	  activate_portal(help);
	  if (help)
	     free_portal(help);

#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Volumes");
#else
	  sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Volumes");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	  break;

       default:
	  break;
    }
    return 0;
}

ULONG RestoreMenuKeyHandler(NWSCREEN *screen, ULONG key, ULONG index)
{
    register ULONG help, i;
    BYTE displayBuffer[255];

    switch (key)
    {
       case TAB:
	  if (nw_portal)
	  {
#if (LINUX_UTIL)
	     sprintf(displayBuffer, " F1-Help  F3-Menu  SPACE-Mark  DEL-Clr Targ  INS-Sel Vol  ENTER-Edit Vol");
#else
	     sprintf(displayBuffer, " F1-Help  ESC-Menu  SPACE-Mark  DEL-Clr Targ  INS-Sel Vol  ENTER-Edit Vol");
#endif
	     write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

	     get_portal_resp(nw_portal);

#if (LINUX_UTIL)
	     sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Volumes");
#else
	     sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Volumes");
#endif
	     write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	  }
	  break;

       case F1:
#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F3-Exit  PgUp-Scroll Up  PgDn-Scroll Down ");
#else
	  sprintf(displayBuffer, "  ESC-Exit  PgUp-Scroll Up  PgDn-Scroll Down ");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

	  help = make_portal(&ConsoleScreen,
		       " Help for Restoring NetWare Volumes ",
		       0,
		       4,
		       0,
		       ConsoleScreen.nLines - 4,
		       ConsoleScreen.nCols - 1,
		       400,
		       BORDER_SINGLE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       0,
		       0,
		       0,
		       TRUE);
	  if (!help)
	     break;

	  for (i=0; i < sizeof(RestoreHelp) / sizeof(BYTE *); i++)
	     write_portal_cleol(help, RestoreHelp[i], i, 0, BLACK | BGWHITE);

	  activate_portal(help);
	  if (help)
	     free_portal(help);

#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F1-Help  F3-Exit  TAB-Edit Volumes");
#else
	  sprintf(displayBuffer, "  F1-Help  ESC-Exit  TAB-Edit Volumes");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	  break;

       default:
	  break;
    }
    return 0;
}

ULONG menuAction(NWSCREEN *screen, ULONG value, BYTE *option, ULONG index)
{
   ULONG i, j, message, iflags, ccode, menu, found_flag, len;
   BYTE displayBuffer[255];
   BYTE buf[5];
   ULONG handles[7];
   nwvp_payload payload;
   nwvp_vpartition_info vpart_info;
   BYTE WorkSpace[20];
   BYTE SetName[9] = "";
   BYTE LogName[9] = "";
   FILE *fp;
   extern int NWNTFS(void);
   extern BYTE LogFileName[20];

   switch (value)
   {
      case 1:
	 save_menu(nw_menu);
#if (LINUX_UTIL)
	 for (i=3; i < ConsoleScreen.nLines - 1; i++)
	    put_char_cleol(&ConsoleScreen, 176 | A_ALTCHARSET, i, CYAN | BGBLUE);
#endif

#if (DOS_UTIL | WINDOWS_NT_UTIL)
	 for (i=3; i < ConsoleScreen.nLines - 1; i++)
	    put_char_cleol(&ConsoleScreen, 176, i, CYAN | BGBLUE);
#endif

#if (LINUX_UTIL)
	 sprintf(displayBuffer, "  F1-Help  F3-Menu  TAB-Edit Volumes");
#else
	 sprintf(displayBuffer, "  F1-Help  ESC-Menu  TAB-Edit Volumes");
#endif
	 write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

	 nw_portal = make_portal(&ConsoleScreen,
		       " ",
		       "  Marked  #             Name    Total Size  Cluster  Segs Mirror Status    ",
		       3,
		       0,
		       14,
		       ConsoleScreen.nCols - 1,
		       100,
		       BORDER_SINGLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       ImagePortalFunction,
		       0,
		       ImagePortalKeyFunction,
		       TRUE);
	 if (!nw_portal)
	 {
	    restore_menu(nw_menu);
	    break;
	 }

	 NWFSSet(&set_header, 0, sizeof(IMAGE_SET));
	 NWFSSet(&image_table, 0, (sizeof(IMAGE) * IMAGE_TABLE_SIZE));
	 NWFSSet(&vpart_info, 0, sizeof(nwvp_vpartition_info));

	 // make the volume set name unique if a file with this name
	 // already exists

	 NWFSCopy(ImageSetName, "IMAGE000.SET", 12);
	 ImageSetName[12] = '\0';

	 WorkPath[0] = '\0';
	 strcat(WorkPath, Path);
	 strcat(WorkPath, ImageSetName);
	 fp = fopen(WorkPath, "rb");
	 if (fp)
	 {
	    fclose(fp);
	    ccode = CreateUniqueSet(ImageSetName, WorkSpace, 20, 0);
	    if (!ccode)
	       NWFSCopy(ImageSetName, WorkSpace, strlen(WorkSpace));
	 }

	 ScanForDataSets();
	 ScanVolumes();

	 image_table_count = 0;
	 payload.payload_object_count = 0;
	 payload.payload_index = 0;
	 payload.payload_object_size_buffer_size = (7 << 20) + sizeof(handles);
	 payload.payload_buffer = (BYTE *) &handles[0];

	 do
	 {
	    nwvp_vpartition_scan(&payload);
	    for (i=0; i < payload.payload_object_count; i++)
	    {
	       if (nwvp_vpartition_return_info(handles[i], &vpart_info) == 0)
	       {
		  memmove(&image_table[image_table_count].VolumeName[0], &vpart_info.volume_name[0], 16);

		  image_table[image_table_count].VolumeClusterSize = vpart_info.blocks_per_cluster * 4096;
		  image_table[image_table_count].LogicalVolumeClusters = vpart_info.cluster_count;
		  image_table[image_table_count].SegmentCount = vpart_info.segment_count;
		  image_table[image_table_count].MirrorFlag = vpart_info.mirror_flag;

		  if ((valid_table[image_table_count] = ((vpart_info.volume_valid_flag == 0) ? 0xFFFFFFFF : 0)) != 0)
		     mark_table[image_table_count] = 1;
		  image_table_count++;
	       }
	    }
	 } while ((payload.payload_index != 0) && (payload.payload_object_count != 0));

	 clear_portal(nw_portal);
	 for (i=0; i < image_table_count; i++)
	 {
	    sprintf(displayBuffer,
		    "  %s  %2d [%16s]  %6u MB     %2d K    %2d  %s   %s",
		    (mark_table[i] == 0) ? "   " : "***",
		    (int)i,
		    &image_table[i].VolumeName[1],
		    (unsigned int)
		       (((LONGLONG)image_table[i].LogicalVolumeClusters *
			 (LONGLONG)image_table[i].VolumeClusterSize) /
			 (LONGLONG)0x100000),
		    (int)(image_table[i].VolumeClusterSize / 1024),
		    (int)image_table[i].SegmentCount,
		    (image_table[i].MirrorFlag == 0) ?
		    "SINGLE" : "MIRROR",
		    (valid_table[i] == 0) ? "INVALID" : "OK");
	    write_portal_cleol(nw_portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);
	 }

	 activate_static_portal(nw_portal);

	 for (i=0; i < strlen(ImageSetName); i++)
	    ImageSetName[i] = toupper(ImageSetName[i]);

	 sprintf(NameHeader, "  Image Set Filename:  %s           ", ImageSetName);
	 write_portal_header1(nw_portal, NameHeader, BRITEWHITE | BGBLUE);

	 menu = make_menu(&ConsoleScreen,
		     " Volume Image Menu Options ",
		     15,
		     24,
		     3,
		     BORDER_DOUBLE,
		     YELLOW | BGBLUE,
		     YELLOW | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     ImageMenuFunction,
		     0,
		     ImageMenuKeyHandler,
		     TRUE,
		     0);
	 if (menu)
	 {
	    add_item_to_menu(menu, "Image Marked Volumes", 1);
	    add_item_to_menu(menu, "Change Image Set Name", 2);
	    add_item_to_menu(menu, "Delete Image Sets", 3);

	    ccode = activate_menu(menu);

	    free_menu(menu);
	 }

	 if (nw_portal)
	 {
	    deactivate_static_portal(nw_portal);
	    free_portal(nw_portal);
	 }
	 restore_menu(nw_menu);

#if (LINUX_UTIL)
	 sprintf(displayBuffer, "  F1-Help  F3-Exit");
#else
	 sprintf(displayBuffer, "  F1-Help  ESC-Exit");
#endif
	 write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	 break;

      case 2:
	 save_menu(nw_menu);
#if (LINUX_UTIL)
	 for (i=3; i < ConsoleScreen.nLines - 1; i++)
	    put_char_cleol(&ConsoleScreen, 176 | A_ALTCHARSET, i, CYAN | BGBLUE);
#endif

#if (DOS_UTIL | WINDOWS_NT_UTIL)
	 for (i=3; i < ConsoleScreen.nLines - 1; i++)
	    put_char_cleol(&ConsoleScreen, 176, i, CYAN | BGBLUE);
#endif

#if (LINUX_UTIL)
	 sprintf(displayBuffer, "  F1-Help  F3-Menu  TAB-Edit Volumes");
#else
	 sprintf(displayBuffer, "  F1-Help  ESC-Menu  TAB-Edit Volumes");
#endif
	 write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

	 nw_portal = make_portal(&ConsoleScreen,
		       " ",
		       "   Auto #    Image Volume    TotalSize Cluster Segs Mirror   Target Vol",
		       3,
		       0,
		       14,
		       ConsoleScreen.nCols - 1,
		       100,
		       BORDER_SINGLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       RestorePortalFunction,
		       0,
		       RestorePortalKeyFunction,
		       TRUE);
	 if (!nw_portal)
	 {
	    restore_menu(nw_menu);
	    break;
	 }

	 ScanForDataSets();
	 ScanVolumes();

	 image_table_count = 0;

	 if (image_set_table_count)
	 {
	    NWFSCopy(&RestoreSetName[0], &set_file_name_table[0][0], 12);
	    for (i=0; i < strlen(RestoreSetName); i++)
	       RestoreSetName[i] = toupper(RestoreSetName[i]);
	    RestoreSetName[12] = '\0';
	    process_set_file(RestoreSetName);
	 }
	 else
	 {
	    NWFSCopy(&RestoreSetName[0], "IMAGE000.SET", strlen("IMAGE000.SET"));
	    RestoreSetName[12] = '\0';
	    for (i=0; i < strlen(RestoreSetName); i++)
	       RestoreSetName[i] = toupper(RestoreSetName[i]);
	 }

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
	    {
	       NWFSSet(&target_volume_table[i][0], 0, 16);
	       target_volume_flags[i] = 0;
	    }
	 }

	 activate_static_portal(nw_portal);

	 sprintf(NameHeader, "  Restore Set Filename:  %s           ", RestoreSetName);
	 write_portal_header1(nw_portal, NameHeader, BRITEWHITE | BGBLUE);

	 clear_portal(nw_portal);
	 for (i=0; i < image_table_count; i++)
	 {
	    sprintf(displayBuffer,
		    " %s %2d  [%-15s] %6u MB    %2d K  %2d  %s [%-15s]",
		    (mark_table[i] == 0) ? "   " : "***",
		    (int)i,
		    &image_table[i].VolumeName[1],
		    (unsigned int)
		       (((LONGLONG)image_table[i].LogicalVolumeClusters *
			 (LONGLONG)image_table[i].VolumeClusterSize) /
			 (LONGLONG)0x100000),
		    (int)(image_table[i].VolumeClusterSize / 1024),
		    (int)image_table[i].SegmentCount,
		    (image_table[i].MirrorFlag == 0) ? "SINGLE" : "MIRROR",
		    &target_volume_table[i][1]);
	    write_portal_cleol(nw_portal, displayBuffer, i, 0, BRITEWHITE | BGBLUE);
	 }

	 update_static_portal(nw_portal);

	 menu = make_menu(&ConsoleScreen,
		     " Image Restore Menu Options ",
		     15,
		     24,
		     4,
		     BORDER_DOUBLE,
		     YELLOW | BGBLUE,
		     YELLOW | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     RestoreMenuFunction,
		     0,
		     RestoreMenuKeyHandler,
		     TRUE,
		     0);
	 if (menu)
	 {
	    add_item_to_menu(menu, "Restore Marked Volumes", 1);
	    add_item_to_menu(menu, "Restore Volumes To Path", 2);
	    add_item_to_menu(menu, "Select Image Set", 3);
	    add_item_to_menu(menu, "Delete Image Sets", 4);

	    ccode = activate_menu(menu);

	    free_menu(menu);
	 }

	 if (nw_portal)
	 {
	    deactivate_static_portal(nw_portal);
	    free_portal(nw_portal);
	 }
	 restore_menu(nw_menu);

#if (LINUX_UTIL)
	 sprintf(displayBuffer, "  F1-Help  F3-Exit");
#else
	 sprintf(displayBuffer, "  F1-Help  ESC-Exit");
#endif
	 write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	 break;

      case 3:
	 iflags = make_portal(&ConsoleScreen,
		       " Current Imaging Settings ",
		       0,
		       6,
		       8,
		       19,
		       72,
		       10,
		       BORDER_DOUBLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0,
		       0,
		       0,
		       FALSE);

	 if (!iflags)
	    break;

#if (LINUX_UTIL)
	 sprintf(displayBuffer, " [TAB,UP,DOWN]-Next/Prev  F10-Accept  F3-Back");
	 write_portal_cleol(iflags, displayBuffer, 9, 0, BLUE | BGWHITE);
#else
	 sprintf(displayBuffer, " [TAB,UP,DOWN]-Next/Prev  F10-Accept  ESC-Back");
	 write_portal_cleol(iflags, displayBuffer, 9, 0, BLUE | BGWHITE);
#endif

	 activate_static_portal(iflags);

	 ccode = add_field_to_portal(iflags, 1, 2, WHITE | BGBLUE,
				     "Strip Deleted Files      : ",
			      strlen("Strip Deleted Files      : "),
			      buf,
			      sizeof(buf),
			      YesNo, 2,
			      SkipDeletedFiles,
			      &SkipDeletedFiles, 0);

	 ccode = add_field_to_portal(iflags, 2, 2, WHITE | BGBLUE,
				     "Auto-Create Partitions   : ",
			      strlen("Auto-Create Partitions   : "),
			      buf,
			      sizeof(buf),
			      YesNo, 2,
			      AutoCreatePartitions,
			      &AutoCreatePartitions, 0);

	 ccode = add_field_to_portal(iflags, 3, 2, WHITE | BGBLUE,
				     "Enable Logging to a File : ",
			      strlen("Enable Logging to a File : "),
			      buf,
			      sizeof(buf),
			      YesNo, 2,
			      EnableLogging,
			      &EnableLogging, 0);

	 for (i=0; i < 8; i++)
	 {
	    if ((!ImageSetName[i]) || (ImageSetName[i] == ' ') || (ImageSetName[i] == '.'))
	       break;
	    SetName[i] = ImageSetName[i];
	 }
	 SetName[i] = '\0';

	 ccode = add_field_to_portal(iflags, 5, 2, WHITE | BGBLUE,
				     "Enter Image Set Name     : ",
			      strlen("Enter Image Set Name     : "),
			 SetName, 9,
			 0, 0, 0, 0, 0);

	 for (i=0; i < 8; i++)
	 {
	    if ((!LogFileName[i]) || (LogFileName[i] == ' ') || (LogFileName[i] == '.'))
	       break;
	    LogName[i] = LogFileName[i];
	 }
	 LogName[i] = '\0';

	 ccode = add_field_to_portal(iflags, 6, 2, WHITE | BGBLUE,
				     "Enter Log File Name      : ",
			      strlen("Enter Log File Name      : "),
			 LogName, sizeof(LogName),
			 0, 0, 0, 0, 0);

	 ccode = add_field_to_portal(iflags, 7, 2, WHITE | BGBLUE,
				     "Path:",
			      strlen("Path:"),
			 Path, 53,
			 0, 0, 0, 0, 0);


	 ccode = input_portal_fields(iflags);
	 if (ccode)
	 {
	    if (iflags)
	    {
	       deactivate_static_portal(iflags);
	       free_portal(iflags);
	    }
	    break;
	 }

	 if (iflags)
	 {
	    deactivate_static_portal(iflags);
	    free_portal(iflags);
	 }

	 // if the new name is invalid, then break
	 if ((!SetName[0]) || (SetName[0] == ' ') || (SetName[0] == '.'))
	 {
	    error_portal("*** Invalid Data Set Name ***", 10);
	    break;
	 }

	 for (i=0; (i < strlen(SetName)) && (i < 8); i++)
	 {
	    if ((!SetName[i]) || (SetName[i] == ' ') || (SetName[i] == '.'))
	       break;
	    ImageSetName[i] = toupper(SetName[i]);
	 }
	 ImageSetName[i] = '\0';
	 strcat(ImageSetName, ".SET");


	 // if the new name is invalid, then break
	 if ((!LogName[0]) || (LogName[0] == ' ') || (LogName[0] == '.'))
	 {
	    error_portal("*** Invalid Log File Name ***", 10);
	    break;
	 }

	 for (i=0; (i < strlen(LogName)) && (i < 8); i++)
	 {
	    if ((!LogName[i]) || (LogName[i] == ' ') || (LogName[i] == '.'))
	       break;
	    LogFileName[i] = toupper(LogName[i]);
	 }
	 LogFileName[i] = '\0';
	 strcat(LogFileName, ".LOG");

	 len = strlen(Path);
	 if (len)
	 {
#if (LINUX_UTIL)
	    if (Path[len - 1] != '/')
	       strcat(Path, "/");
#endif

#if (DOS_UTIL | WINDOWS_NT_UTIL)
#if (DOS_UTIL)
	    for (i=0; i < len; i++)
	       Path[i] = toupper(Path[i]);
#endif
	    if (Path[len - 1] == '/')
	       Path[len - 1] = '\\';
	    else
	    if (Path[len - 1] != '\\')
	       strcat(Path, "\\");
#endif
	 }

#if (LINUX_UTIL)
	 if (!len)
	    strcat(Path, "/var/log/");
#endif

	 break;

      case 4:
	 save_menu(nw_menu);
	 message = create_message_portal(" Invoking Volume Manager ... ", 10,
					 YELLOW | BGBLUE);
	 NWConfig();
#if (LINUX_UTIL)
	 for (i=0; i < ConsoleScreen.nLines - 1; i++)
	    put_char_cleol(&ConsoleScreen, 176 | A_ALTCHARSET, i, CYAN | BGBLUE);
#endif

#if (DOS_UTIL | WINDOWS_NT_UTIL)
	 for (i=0; i < ConsoleScreen.nLines - 1; i++)
	    put_char_cleol(&ConsoleScreen, 176, i, CYAN | BGBLUE);
#endif

	 sprintf(displayBuffer, IMAGER_NAME);
	 put_string_cleol(&ConsoleScreen, displayBuffer, 0, BLUE | BGCYAN);

	 sprintf(displayBuffer, COPYRIGHT_NOTICE1);
	 put_string_cleol(&ConsoleScreen, displayBuffer, 1, BLUE | BGCYAN);

	 sprintf(displayBuffer, COPYRIGHT_NOTICE2);
	 put_string_cleol(&ConsoleScreen, displayBuffer, 2, BLUE | BGCYAN);


#if (LINUX_UTIL)
	 sprintf(displayBuffer, "  F1-Help  F3-Exit");
#else
	 sprintf(displayBuffer, "  F1-Help  ESC-Exit");
#endif
	 write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	 if (message)
	    close_message_portal(message);
	 restore_menu(nw_menu);

	 ScanVolumes();
	 break;

      case 5:
	 error_portal("*** You Must Have NetMigrate To Perform NTFS Conversion ***", 10);
	 break;

      default:
	 break;
   }
   return 0;
}

ULONG keyHandler(NWSCREEN *screen, ULONG key, ULONG index)
{
    register ULONG help, i;
    BYTE displayBuffer[255];

    switch (key)
    {
       case F1:
#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F3-Exit  PgUp-Scroll Up  PgDn-Scroll Down ");
#else
	  sprintf(displayBuffer, "  ESC-Exit  PgUp-Scroll Up  PgDn-Scroll Down ");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

	  help = make_portal(&ConsoleScreen,
		       " General Help ",
		       0,
		       4,
		       0,
		       ConsoleScreen.nLines - 4,
		       ConsoleScreen.nCols - 1,
		       400,
		       BORDER_SINGLE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       BLACK | BGWHITE,
		       0,
		       0,
		       0,
		       TRUE);
	  if (!help)
	     break;

	  for (i=0; i < sizeof(AllHelp) / sizeof(BYTE *); i++)
	     write_portal_cleol(help, AllHelp[i], i, 0, BLACK | BGWHITE);

	  activate_portal(help);
	  if (help)
	     free_portal(help);

#if (LINUX_UTIL)
	  sprintf(displayBuffer, "  F1-Help  F3-Exit");
#else
	  sprintf(displayBuffer, "  F1-Help  ESC-Exit");
#endif
	  write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);
	  break;

       default:
	  break;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    register ULONG i, retCode, len;
    BYTE displayBuffer[255];

    // if an image path was passed, copy the path for use later
    if (argc > 1)
    {
#if (LINUX_UTIL | DOS_UTIL)
       DIR *local_dir;
#endif

#if (WINDOWS_NT_UTIL)
       HANDLE handle;
       WIN32_FIND_DATA findData;
#endif

       len = strlen(argv[1]);
       len = ((len >= 1024) ? 1024 : len);
       if (len && ((argv[1][len - 1] == '\\') || (argv[1][len - 1] == '/')))
	  argv[1][len - 1] = '\0';

       strcpy(WorkPath, argv[1]);
       len = strlen(WorkPath);

#if (LINUX_UTIL)
       if (!len)
	  strcat(WorkPath, "/var/log/");
#endif

#if (LINUX_UTIL | DOS_UTIL)
       local_dir = opendir(WorkPath);
       if (WorkPath[0] && local_dir)
#endif

#if (WINDOWS_NT_UTIL)
       handle = FindFirstFile(WorkPath, &findData);
       if (WorkPath[0] && (handle != INVALID_HANDLE_VALUE))
#endif
       {
	  strncpy(Path, WorkPath, len);

#if (LINUX_UTIL)
	  if (Path[len - 1] != '/')
	     strcat(Path, "/");
#endif

#if (DOS_UTIL | WINDOWS_NT_UTIL)
#if (DOS_UTIL)
	  for (i=0; i < len; i++)
	     Path[i] = toupper(Path[i]);
#endif
	  if (Path[len - 1] == '/')
	     Path[len - 1] = '\\';
	  else
	  if (Path[len - 1] != '\\')
	     strcat(Path, "\\");
#endif

#if (LINUX_UTIL | DOS_UTIL)
	 closedir(local_dir);
#endif

#if (WINDOWS_NT_UTIL)
	  FindClose(handle);
#endif
       }
    }
#if (LINUX_UTIL)
    else
    {
       Path[0] = '\0';
       strcat(Path, "/var/log/");
    }
#endif

    InitNWFS();
    if (InitConsole())
       return 0;

#if (LINUX_UTIL)
    for (i=0; i < ConsoleScreen.nLines - 1; i++)
       put_char_cleol(&ConsoleScreen, 176 | A_ALTCHARSET, i, CYAN | BGBLUE);
#endif

#if (DOS_UTIL | WINDOWS_NT_UTIL)
    for (i=0; i < ConsoleScreen.nLines - 1; i++)
       put_char_cleol(&ConsoleScreen, 176, i, CYAN | BGBLUE);
#endif

    sprintf(displayBuffer, IMAGER_NAME);
    put_string_cleol(&ConsoleScreen, displayBuffer, 0, BLUE | BGCYAN);

    sprintf(displayBuffer, COPYRIGHT_NOTICE1);
    put_string_cleol(&ConsoleScreen, displayBuffer, 1, BLUE | BGCYAN);

    sprintf(displayBuffer, COPYRIGHT_NOTICE2);
    put_string_cleol(&ConsoleScreen, displayBuffer, 2, BLUE | BGCYAN);

#if (LINUX_UTIL)
    sprintf(displayBuffer, "  F1-Help  F3-Exit");
#else
    sprintf(displayBuffer, "  F1-Help  ESC-Exit");
#endif
    write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

    nw_menu = make_menu(&ConsoleScreen,
		     " Available Menu Options ",
		     9,
		     19,
#if (DOS_UTIL | WINDOWS_NT_UTIL)
		     5,
#else
		     4,
#endif
		     BORDER_DOUBLE,
		     YELLOW | BGBLUE,
		     YELLOW | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     menuAction,
		     warnAction,
		     keyHandler,
		     TRUE,
		     0);

    if (!nw_menu)
	  goto ErrorExit;

    add_item_to_menu(nw_menu, "Create Volume Image Sets ", 1);
    add_item_to_menu(nw_menu, "Restore Volume Image Sets ", 2);
    add_item_to_menu(nw_menu, "Modify Imaging Settings ", 3);
    add_item_to_menu(nw_menu, "Configure NetWare Partitions/Volumes ", 4);

#if (LINUX_UTIL && LINUX_DEV)
    devfd = open("/dev/nwfs", O_RDWR);
    if (devfd < 0)
    {
       log_sprintf(displayBuffer, 
	    "Error opening /dev/nwfs device - some operations disabled");
       error_portal(displayBuffer, 11);
    }
#endif

    retCode = activate_menu(nw_menu);

ErrorExit:;
    sprintf(displayBuffer, " Exiting ... ");
    write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

    if (nw_menu)
       free_menu(nw_menu);

#if (LINUX_UTIL && LINUX_DEV)
    if (devfd)
       close(devfd);
#endif

    ReleaseConsole();
    CloseNWFS();
    return 0;

}





