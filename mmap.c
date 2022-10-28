
/***************************************************************************
*
*   Copyright (c) 1997-2022 Jeff V. Merkey
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
*   FILE     :  MMAP.C
*   DESCRIP  :  VFS Memory Mapping Module for Linux
*   DATE     :  December 6, 1998
*
*
***************************************************************************/

#include "globals.h"

extern ULONG insert_io(ULONG disk, ASYNCH_IO *io);
extern void RunAsynchIOQueue(ULONG disk);
extern ULONG nwfs_to_linux_error(ULONG NwfsError);

// this function returns the volume logical block number from the
// logical file block number.

ULONG NWFileMapBlock(VOLUME *volume, HASH *hash, ULONG Offset,
		     ULONG *boffset)
{
    register ULONG Length, vBlock;
    register long FATChain, index, voffset;
    register ULONG StartIndex, StartOffset, rc;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;
    SUBALLOC_MAP map;
#if (!HASH_FAT_CHAINS)
    register ULONG ccode;
    register MACINTOSH *mac;
    DOS dos;
#endif
    
    // if a subdirectory or bad bsize then return error
    if (!hash || (hash->Flags & SUBDIRECTORY_FILE))
       return (ULONG) -1;

    if (boffset)
       *boffset = 0;

#if (HASH_FAT_CHAINS)
    FATChain = hash->FirstBlock;
    Length = hash->FileSize;
#else
    ccode = ReadDirectoryRecord(volume, &dos, hash->DirNo);
    if (ccode)
       return -1;

    FATChain = dos.FirstBlock;
    Length = dos.FileSize;
#endif

    StartIndex = Offset / volume->ClusterSize;
    StartOffset = Offset % volume->ClusterSize;
    
    // we always start with an index of zero
    index = 0;
    if (FATChain & 0x80000000)
    {
       // check for EOF
       if (FATChain == (ULONG) -1)
          return (ULONG) -1;

       // index must be null here
       if (StartIndex)
          return (ULONG) -1;

       rc = MapSuballocNode(volume, &map, FATChain);
       if (rc)
          return (ULONG) -1;

       if (StartOffset >= map.Size)
          return (ULONG) -1;

       if (StartOffset >= map.clusterSize[0])
       {
	  if (map.Count == 1)
             return (ULONG) -1;

          voffset = StartOffset - map.clusterSize[0];
	  vBlock = (map.clusterNumber[1] * volume->BlocksPerCluster) +
	          ((map.clusterOffset[1] + voffset) / IO_BLOCK_SIZE);
          if (boffset)
	     *boffset = ((map.clusterOffset[1] + voffset) % IO_BLOCK_SIZE);
	  return vBlock;
       }
       else
       {
	  vBlock = (map.clusterNumber[0] * volume->BlocksPerCluster) +
		  ((map.clusterOffset[0] + StartOffset) / IO_BLOCK_SIZE);
          if (boffset)
	     *boffset = ((map.clusterOffset[0] + StartOffset) % IO_BLOCK_SIZE);
	  return vBlock;
       }
    }

    FAT = GetFatEntry(volume, FATChain, &FAT_S);
    if (FAT)
       index = FAT->FATIndex;

    while (FAT && FAT->FATCluster)
    {
       // we detected a hole in the file
       if (StartIndex < index)
       {
          // return 0 if we detect a hole in the file.
	  vBlock = 0;
	  return vBlock;
       }

       // we found our cluster in the chain
       if (StartIndex == index)
       {
	  vBlock = (FATChain * volume->BlocksPerCluster) + 
		   (StartOffset / IO_BLOCK_SIZE);
          if (boffset)
	     *boffset = (StartOffset % IO_BLOCK_SIZE);
	  return vBlock;
       }

       // bump to the next cluster
       FATChain = FAT->FATCluster;

       // check if the next cluster is a suballoc element or EOF marker
       if (FATChain & 0x80000000)
       {
	  // end of file
	  if (FATChain == (ULONG) -1)
             return (ULONG) -1;

	  // check for valid index
	  if ((index + 1) == StartIndex)
	  {
	     rc = MapSuballocNode(volume, &map, FATChain);
	     if (rc)
                return (ULONG) -1;

	     if (StartOffset >= map.Size)
                return (ULONG) -1;

             if (StartOffset >= map.clusterSize[0])
             {
	        if (map.Count == 1)
                   return (ULONG) -1;

                voffset = StartOffset - map.clusterSize[0];
	        vBlock = (map.clusterNumber[1] * volume->BlocksPerCluster) +
	                ((map.clusterOffset[1] + voffset) / IO_BLOCK_SIZE);
                if (boffset)
	           *boffset = ((map.clusterOffset[1] + voffset) % 
			       IO_BLOCK_SIZE);
	        return vBlock;
             }
             else
             {
	        vBlock = (map.clusterNumber[0] * volume->BlocksPerCluster) +
		        ((map.clusterOffset[0] + StartOffset) / IO_BLOCK_SIZE);
                if (boffset)
	           *boffset = ((map.clusterOffset[0] + StartOffset) % 
			       IO_BLOCK_SIZE);
	        return vBlock;
             }
	  }
          return (ULONG) -1;
       }

       // get next fat table entry and index
       FAT = GetFatEntry(volume, FATChain, &FAT_S);
       if (FAT)
	  index = FAT->FATIndex;
    }
    return (ULONG) -1;

}

#if (LINUX_24)

ULONG nwfs_map_extents(VOLUME *volume, HASH *hash, ULONG block, 
		       ULONG bsize, ULONG *device, ULONG extend, 
		       ULONG *retCode, ASYNCH_IO *io, 
		       struct inode *inode, ULONG *vblock, 
		       ULONG *voffset, nwvp_asynch_map *map,
		       ULONG *count)
{
    register ULONG ccode, disk, vBlock, vMap, mirror_index;
    ULONG boffset = 0;
    DOS dos;

    if (vblock)
       *vblock = 0;
    
    if (voffset)
       *voffset = 0;
    
    if (device)
       *device = 0;
    
    if (retCode)
       *retCode = 0;

    if (extend)
    {
       ccode = ReadDirectoryRecord(volume, &dos, hash->DirNo);
       if (ccode)
          return 0;
     
       // NWWriteFile() supports an extend mode that will extend the 
       // meta-data for a file if we pass a NULL address for the data 
       // buffer argument without writing any data to the extended 
       // area.

       ccode = NWWriteFile(volume,
		           &dos.FirstBlock, 
		           dos.Flags, 
		           block * bsize,
		           NULL,
		           (1 << PAGE_CACHE_SHIFT),
		           0,
		           0,
		           retCode,
		           KERNEL_ADDRESS_SPACE,
                           ((volume->VolumeFlags & SUB_ALLOCATION_ON) ? 1 : 0),
		           dos.FileAttributes);

       if (ccode != (1 << PAGE_CACHE_SHIFT))
          return 0;

       if (retCode && *retCode)
          return 0;

#if (HASH_FAT_CHAINS)
       hash->FirstBlock = dos.FirstBlock;
#endif

       // since we extended the file, flag as modified to force
       // suballocation during file close.
       hash->State |= NWFS_MODIFIED;
       
       ccode = WriteDirectoryRecord(volume, &dos, hash->DirNo);
       if (ccode)
          return 0;
    }	    

    if (map && count)
    {
       vBlock = NWFileMapBlock(volume, hash, block * bsize, &boffset);
       if (!vBlock || (vBlock == (ULONG) -1))
       {
#if (VERBOSE)
          NWFSPrint("[%s] no map vBlock-%d block-%d\n", hash->Name,
		 (int)vBlock, (int)block);
#endif
          return 0;
       }

       ccode = nwvp_vpartition_map_asynch_write(volume->nwvp_handle,
                                             vBlock, count, map); 
       if (!ccode && *count)
       {
          extern ULONG mirror_counter;
       
          // select a random mirror to read from
          mirror_index = mirror_counter % *count;   
          mirror_counter++;

          disk = map[mirror_index].disk_id; 
          if (SystemDisk[disk])
          { 
  	     *device = (ULONG)SystemDisk[disk]->PhysicalDiskHandle;

	     vMap = (map[mirror_index].sector_offset / (bsize / 512)) +
	         (boffset / bsize);

	     if (vblock)
	        *vblock = vBlock;

	     if (voffset)
	        *voffset = boffset;
	     
#if (VERBOSE)
             NWFSPrint("[%s] block/size-%d/%d vblock-%d vMap-%08X boff=%d ext-%d\n",
		    hash->Name,
		    (int)block,
		    (int)bsize,
		    (int)vBlock,
		    (unsigned)vMap,
		    (int)boffset,
		    (int)extend);
#endif
	     return vMap;
          } 
       }
    }
    return 0;
}

int nwfs_get_block(struct inode *inode, long block,
                   struct buffer_head *bh_result, int create)
{
    register VOLUME *volume = inode->i_sb->u.generic_sbp;
    register HASH *hash = inode->u.generic_ip;
    register ULONG lba, totalBlocks;
    ULONG device = 0, retCode = 0, count = 0;
    nwvp_asynch_map map[8];

    create = TRUE;
    
#if (VERBOSE)
    NWFSPrint("get_block block-%d create-%d [%s]\n", (int)block, 
	      (int)create, hash->Name);  
#endif

    if (!hash)
       return -EBADF;

    if (!volume)
       return -EBADF;

    if (!inode->i_sb->s_blocksize || 
       (inode->i_sb->s_blocksize > volume->BlockSize))
       return -EBADF;
    
    totalBlocks = (volume->VolumeClusters * volume->BlocksPerCluster);
    totalBlocks = totalBlocks * (IO_BLOCK_SIZE / inode->i_sb->s_blocksize);
    if ((block < 0) || (block > totalBlocks))
    {
       NWFSPrint("nwfs:  mmap exceeded volume size block-%d total-%d\n",
		 (int)block, (int)totalBlocks); 
       return -EIO;
    }

    NWLockFileExclusive(hash);

    // if someone passes in a NULL bh_result buffer, and create is set
    // TRUE, then we just want to extend the file without redundantly 
    // calling nwfs_map_extents() several times.

    lba = nwfs_map_extents(volume, hash, block, inode->i_sb->s_blocksize, 
		           (bh_result ? &device : NULL), 
			   (bh_result ? 0 : create), 
			   &retCode, 0, inode, 0, 0,
			   (bh_result ? &map[0] : NULL), 
			   (bh_result ? &count : NULL));
    if (retCode)
    {
       NWUnlockFile(hash);
       return nwfs_to_linux_error(retCode);
    }
    
    if (!bh_result)
    {
       NWUnlockFile(hash);
       return 0;
    }
        
    if (lba && device)
    {
       bh_result->b_dev = device;
       bh_result->b_blocknr = lba;
       bh_result->b_state |= (1UL << BH_Mapped);
       if (create) 
          bh_result->b_state |= (1UL << BH_New);
       NWUnlockFile(hash);
       return 0;
    }

    if (!create)
    {
       NWUnlockFile(hash);
       return 0;
    }
	    
    lba = nwfs_map_extents(volume, hash, block, inode->i_sb->s_blocksize, 
		           &device, create, &retCode, 0, inode, 0, 0,
			   &map[0], &count);
    if (retCode)
    {
       NWUnlockFile(hash);
       return nwfs_to_linux_error(retCode);
    }

    if (lba && device)
    {
       bh_result->b_dev = device;
       bh_result->b_blocknr = lba;
       bh_result->b_state |= (1UL << BH_Mapped);
       bh_result->b_state |= (1UL << BH_New);
       NWUnlockFile(hash);
       return 0;
    }

    NWUnlockFile(hash);
    return -EIO;
}

int nwfs_readpage(struct file *file, struct page *page)
{
    return (block_read_full_page(page, nwfs_get_block));
}

int nwfs_bmap(struct address_space *mapping, long block)
{
    return (generic_block_bmap(mapping, block, nwfs_get_block));
}

int nwfs_writepage(struct page *page)
{
    return (block_write_full_page(page, nwfs_get_block));
}

int nwfs_prepare_write(struct file *file, struct page *page, 
		       unsigned from, unsigned to)
{
    return (block_prepare_write(page, from, to, nwfs_get_block));
}

int nwfs_commit_write(struct file *file, struct page *page,
		      unsigned from, unsigned to)
{
    return (generic_commit_write(file, page, from, to));
}

struct address_space_operations nwfs_aops =
{
    readpage:         nwfs_readpage,
    writepage:        nwfs_writepage,
    prepare_write:    nwfs_prepare_write,
    commit_write:     nwfs_commit_write,
    bmap:	      nwfs_bmap,
};

#endif

#if (LINUX_20 || LINUX_22)

ULONG nwfs_map_file(VOLUME *volume, HASH *hash, ULONG block, 
		    ULONG bsize, ULONG *retCode, struct inode *inode)
{
    register ULONG ccode, disk, vblock, vmap, mirror_index;
    ULONG boffset = 0;
    nwvp_asynch_map map[8];
    ULONG count = 0;
    
    if (retCode)
       *retCode = 0;

    vblock = NWFileMapBlock(volume, hash, block * bsize, &boffset);
    if (!vblock || (vblock == (ULONG) -1))
       return 0;

    ccode = nwvp_vpartition_map_asynch_write(volume->nwvp_handle,
                                             vblock, &count, &map[0]); 
    if (!ccode && count)
    {
       extern ULONG mirror_counter;
       
       // select a random mirror to read from
       mirror_index = mirror_counter % count;   
       mirror_counter++;

       disk = map[mirror_index].disk_id; 
       if (SystemDisk[disk])
       { 
  	  inode->i_dev = (ULONG)SystemDisk[disk]->PhysicalDiskHandle;
	  vmap = (map[mirror_index].sector_offset / (bsize / 512)) +
	         (boffset / bsize);
	  return vmap;
       } 
    }
    return 0;
}

int nwfs_bmap(struct inode *inode, int block)
{
    register VOLUME *volume = (VOLUME *)inode->i_sb->u.generic_sbp;
    register HASH *hash = inode->u.generic_ip;
    ULONG retCode = 0;
    register ULONG lba;
    
#if (VERBOSE)
    NWFSPrint("bmap %s block-%d bsize-%d\n", hash->Name, (int)block,
		    (int)inode->i_sb->s_blocksize);
#endif

    NWLockFile(hash);

    lba = nwfs_map_file(volume, hash, block, inode->i_sb->s_blocksize, 
		        &retCode, inode);
    NWUnlockFile(hash);

    if (retCode)
       return nwfs_to_linux_error(retCode);
    
    return lba;
}

#endif

