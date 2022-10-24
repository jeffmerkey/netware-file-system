
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
****************************************************************************
*
*
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  BIT.C
*   DESCRIP  :   Bit Block Routines for Managing Free Lists
*   DATE     :  November 16, 1998
*
*
***************************************************************************/

#include "globals.h"

void NWLockBB(BIT_BLOCK_HEAD *bb)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_lock_irqsave(&bb->BB_spinlock, bb->BB_flags);
#else
    if (WaitOnSemaphore(&bb->BBSemaphore) == -EINTR)
       NWFSPrint("lock bit block was interupted\n");
#endif
#endif
}

void NWUnlockBB(BIT_BLOCK_HEAD *bb)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_unlock_irqrestore(&bb->BB_spinlock, bb->BB_flags);
#else
    SignalSemaphore(&bb->BBSemaphore);
#endif
#endif
}

//
//   general bit block allocation, chaining, and search routines
//

BIT_BLOCK *AllocateBitBlock(ULONG BlockSize)
{
    register BIT_BLOCK *blk;

    blk = NWFSAlloc(sizeof(BIT_BLOCK), BITBLOCK_TAG);
    if (!blk)
       return 0;

    NWFSSet(blk, 0, sizeof(BIT_BLOCK));

    blk->BlockBuffer = NWFSCacheAlloc(BlockSize, BITBUFFER_TAG);
    if (!blk->BlockBuffer)
    {
       NWFSFree(blk);
       return 0;
    }
    NWFSSet(blk->BlockBuffer, 0, BlockSize);
    return blk;
}

void FreeBitBlock(BIT_BLOCK *blk)
{
    if (blk->BlockBuffer)
       NWFSFree(blk->BlockBuffer);
    NWFSFree(blk);
    return;
}

ULONG GetBitBlockLimit(BIT_BLOCK_HEAD *bb)
{
    return (bb->Limit);
}

ULONG GetBitBlockBlockSize(BIT_BLOCK_HEAD *bb)
{
    return (bb->BlockSize);
}

ULONG CreateBitBlockList(BIT_BLOCK_HEAD *bb, ULONG Limit, ULONG BlockSize,
		         BYTE *name)
{
    register BIT_BLOCK *blk;
    register int Blocks, BlockNo, i;

#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_lock_init(&bb->BB_spinlock);
    bb->BB_flags = 0;
#else
    AllocateSemaphore(&bb->BBSemaphore, 1);
#endif
#endif

    NWLockBB(bb);
    bb->Head = bb->Tail = 0;
    bb->Count = 0;
    bb->BlockSize = BlockSize;
    bb->Limit = Limit;
    
    if (name)
    {
       for (i=0; (i < 15) && (name[i]); i++)
          bb->Name[i] = name[i];
       bb->Name[i] = '\0';
    }
    else
       bb->Name[0] = '\0';
    
    if (!bb->BlockSize)
    {
       NWUnlockBB(bb);
       return -1;
    }

    // always have one block by default

    Blocks = ((((Limit + (8 - 1)) / 8) +
	     (bb->BlockSize - 1)) / bb->BlockSize);
    for (BlockNo=0; BlockNo < Blocks; BlockNo++)
    {
       blk = AllocateBitBlock(BlockSize);
       if (!blk)
       {
	  NWUnlockBB(bb);
	  NWFSPrint("nwfs:  could not allocate bit block\n");
	  return -1;
       }

       blk->BlockIndex = bb->Count;
       if (!InsertBitBlock(bb, blk))
       {
	  NWUnlockBB(bb);
	  NWFSPrint("nwfs:  could not create bit block list");
	  return -1;
       }
       bb->Count++;
    }
    NWUnlockBB(bb);
    return 0;
}

ULONG FreeBitBlockList(BIT_BLOCK_HEAD *bb)
{
    register BIT_BLOCK *blk;

    NWLockBB(bb);
    while (bb->Head)
    {
       blk = RemoveBitBlock(bb, bb->Head);
       if (blk)
       {
	  NWFSFree(blk->BlockBuffer);
	  NWFSFree(blk);
       }
    }

    NWUnlockBB(bb);
    return 0;
}

ULONG AdjustBitBlockList(BIT_BLOCK_HEAD *bb, ULONG NewLimit)
{
    register BIT_BLOCK *blk, *list, *prev = 0;
    register ULONG TotalBlocks, NewBlocks, i;

    NWLockBB(bb);

    // always have one block by default

    TotalBlocks = ((((NewLimit + (8 - 1)) / 8) +
		      (bb->BlockSize - 1)) / bb->BlockSize);

    // we need to expand the bit block list;

    if (TotalBlocks > bb->Count)
    {
       // get the delta between current blocks and total requested
       // blocks.

       NewBlocks = TotalBlocks - bb->Count;
       for (i=0; i < NewBlocks; i++)
       {
	  blk = AllocateBitBlock(bb->BlockSize);
	  if (!blk)
	  {
	     NWFSPrint("nwfs:  could not allocate bit block\n");
	     NWUnlockBB(bb);
	     return -1;
	  }
	  blk->BlockIndex = bb->Count;
	  if (!InsertBitBlock(bb, blk))
	  {
	     NWFSPrint("nwfs:  could not extend bit block list\n");
	     NWUnlockBB(bb);
	     return -1;
	  }
	  bb->Count++;
       }

       bb->Limit = NewLimit;
       NWUnlockBB(bb);
       return 0;
    }

    // we need to shrink the bit block list
    if (TotalBlocks < bb->Count)
    {
       list = bb->Head;
       for (i=0; i < TotalBlocks; i++)
       {
	  prev = list;
	  list = list->next;
       }
       bb->Count = i;
       if (prev)
	  prev->next = 0;

       // purge the remainder of the list
       while (list)
       {
	  prev = list;
	  list = list->next;
	  FreeBitBlock(prev);
       }
       bb->Limit = NewLimit;

       NWUnlockBB(bb);
       return 0;
    }

    //  if current blocks and computed blocks are equal, then we
    //  return success (fall through case) but update Limit

    bb->Limit = NewLimit;

    NWUnlockBB(bb);
    return 0;
}

BIT_BLOCK *InsertBitBlockByValue(BIT_BLOCK_HEAD *bb, BIT_BLOCK *i, BIT_BLOCK *top)
{
    BIT_BLOCK *old, *p;

    if (!bb->Tail)
    {
       i->next = i->prior = NULL;
       bb->Tail = i;
       return i;
    }
    p = top;
    old = NULL;
    while (p)
    {
       if (p->BlockIndex < i->BlockIndex)
       {
	  old = p;
	  p = p->next;
       }
       else
       {
	  if (p->prior)
	  {
	     p->prior->next = i;
	     i->next = p;
	     i->prior = p->prior;
	     p->prior = i;
	     return top;
	  }
	  i->next = p;
	  i->prior = NULL;
	  p->prior = i;
	  return i;
       }
    }
    old->next = i;
    i->next = NULL;
    i->prior = old;
    bb->Tail = i;
    return bb->Head;
}

BIT_BLOCK *InsertBitBlock(BIT_BLOCK_HEAD *bb, BIT_BLOCK *blk)
{
    register BIT_BLOCK *searchblk;

    searchblk = bb->Head;
    while (searchblk)
    {
       if (searchblk == blk)
	  return 0;
       searchblk = searchblk->next;
    }
    bb->Head = InsertBitBlockByValue(bb, blk, bb->Head);
    return blk;
}

BIT_BLOCK *RemoveBitBlock(BIT_BLOCK_HEAD *bb, BIT_BLOCK *blk)
{
    register BIT_BLOCK *searchblk;

    searchblk = bb->Head;
    while (searchblk)
    {
       if (searchblk == blk)
       {
	  if (bb->Head == blk)
	  {
	     bb->Head = (void *) blk->next;
	     if (bb->Head)
		bb->Head->prior = NULL;
	     else
		bb->Tail = NULL;
	  }
	  else
	  {
	     blk->prior->next = blk->next;
	     if (blk != bb->Tail)
		blk->next->prior = blk->prior;
	     else
		bb->Tail = blk->prior;
	  }
	  blk->next = blk->prior = 0;
	  return blk;
       }
       searchblk = searchblk->next;
    }
    return 0;
}


//
//
//

ULONG InitBitBlockList(BIT_BLOCK_HEAD *bb, ULONG value)
{
    register BIT_BLOCK *blk;

    NWLockBB(bb);
    blk = bb->Head;
    while (blk)
    {
       NWFSSet(blk->BlockBuffer, (value ? 1 : 0), bb->BlockSize);
       blk = blk->next;
    }
    NWUnlockBB(bb);
    return 0;
}

//
//

ULONG GetBitBlockValue(BIT_BLOCK_HEAD *bb, ULONG Index)
{
    register BIT_BLOCK *blk;
    register ULONG EntriesPerBlock;
    register ULONG Block, Offset, retCode;

    NWLockBB(bb);
    if (Index > (bb->Limit - 1))
    {
       NWUnlockBB(bb);
       return -1;
    }

    EntriesPerBlock = (bb->BlockSize * 8);
    Block = Index / EntriesPerBlock;
    Offset = Index % EntriesPerBlock;

    blk = bb->Head;
    while (blk)
    {
       if (blk->BlockIndex == Block)
       {
	  retCode = (ULONG)(((blk->BlockBuffer[Offset >> 3]) >> (Offset & 7)) & 1);
	  NWUnlockBB(bb);
	  return retCode;
       }
       blk = blk->next;
    }
    NWUnlockBB(bb);
    return -1;
}

ULONG SetBitBlockValue(BIT_BLOCK_HEAD *bb, ULONG Index, ULONG Value)
{
    register BIT_BLOCK *blk;
    register ULONG EntriesPerBlock;
    register ULONG Block, Offset;

    NWLockBB(bb);
    if (Index > (bb->Limit - 1))
    {
       NWUnlockBB(bb);
       return -1;
    }

    EntriesPerBlock = (bb->BlockSize * 8);
    Block = Index / EntriesPerBlock;
    Offset = Index % EntriesPerBlock;

    blk = bb->Head;
    while (blk)
    {
       if (blk->BlockIndex == Block)
       {
	  (Value & 1) ? (blk->BlockBuffer[Offset >> 3] |= (1 << (Offset & 7)))
		      : (blk->BlockBuffer[Offset >> 3] &= ~(1 << (Offset & 7)));
	  NWUnlockBB(bb);
	  return 0;
       }
       blk = blk->next;
    }
    NWUnlockBB(bb);
    return -1;
}

ULONG SetBitBlockValueWithRange(BIT_BLOCK_HEAD *bb, ULONG Index, ULONG Value,
		                ULONG Range)
{
    register BIT_BLOCK *blk;
    register ULONG EntriesPerBlock;
    register ULONG Block, Offset;
    register int count = Range, i;

    NWLockBB(bb);
    if (Index > (bb->Limit - 1))
    {
       NWUnlockBB(bb);
       return -1;
    }

    EntriesPerBlock = (bb->BlockSize * 8);
    Block = Index / EntriesPerBlock;
    Offset = Index % EntriesPerBlock;

    blk = bb->Head;
    while (blk && (count > 0))
    {
       if (blk->BlockIndex == Block)
       {
          while (blk)
          {
             i = 0;
             if (blk->BlockIndex == Block)
                i = Offset;

	     while ((i < EntriesPerBlock) &&
	           (((blk->BlockIndex * EntriesPerBlock) + i) < bb->Limit))
	     {
                (Value & 1) 
	        ? (blk->BlockBuffer[i >> 3] |= (1 << (i & 7)))
	        : (blk->BlockBuffer[i >> 3] &= ~(1 << (i & 7)));

		count--;
                if (!count)
		{
		   NWUnlockBB(bb);
                   return 0;
		}
	        i++;
	     }
	     blk = blk->next;
	  }
          NWUnlockBB(bb);
          return -1;
       }
       blk = blk->next;
    }
    NWUnlockBB(bb);
    return -1;
}

ULONG GetBitBlockValueNoLock(BIT_BLOCK_HEAD *bb, ULONG Index)
{
    register BIT_BLOCK *blk;
    register ULONG EntriesPerBlock;
    register ULONG Block, Offset, retCode;

    if (Index > (bb->Limit - 1))
    {
       return -1;
    }

    EntriesPerBlock = (bb->BlockSize * 8);
    Block = Index / EntriesPerBlock;
    Offset = Index % EntriesPerBlock;

    blk = bb->Head;
    while (blk)
    {
       if (blk->BlockIndex == Block)
       {
	  retCode = (ULONG)(((blk->BlockBuffer[Offset >> 3]) >> (Offset & 7)) & 1);
	  return retCode;
       }
       blk = blk->next;
    }
    return -1;
}

ULONG SetBitBlockValueNoLock(BIT_BLOCK_HEAD *bb, ULONG Index, ULONG Value)
{
    register BIT_BLOCK *blk;
    register ULONG EntriesPerBlock;
    register ULONG Block, Offset;

    if (Index > (bb->Limit - 1))
    {
       return -1;
    }

    EntriesPerBlock = (bb->BlockSize * 8);
    Block = Index / EntriesPerBlock;
    Offset = Index % EntriesPerBlock;

    blk = bb->Head;
    while (blk)
    {
       if (blk->BlockIndex == Block)
       {
	  (Value & 1) ? (blk->BlockBuffer[Offset >> 3] |= (1 << (Offset & 7)))
		      : (blk->BlockBuffer[Offset >> 3] &= ~(1 << (Offset & 7)));
	  return 0;
       }
       blk = blk->next;
    }
    return -1;
}

//
//
//

ULONG ScanBitBlockValueWithIndex(BIT_BLOCK_HEAD *bb, ULONG Value,
				 ULONG Index, ULONG Range)
{
    register ULONG i;
    register BIT_BLOCK *blk;
    register ULONG EntriesPerBlock;
    register ULONG Block, Offset;
    
    NWLockBB(bb);

#if (PROFILE_BIT_SEARCH)
    bb->bit_search_count = 0;
    bb->skip_search_count = 0;
#endif
    
    if (Index > (bb->Limit - 1))
    {
       NWUnlockBB(bb);
#if (VERBOSE)
       NWFSPrint("ScanBitBlockValueWithIndex - index(%d) > limit(%d)\n",
                 (int)Index, (int)bb->Limit);
#endif

#if (PROFILE_BIT_SEARCH)
       NWFSPrint("swi:count-%d-%d value-%d index/lim-%d/%d (index > limit) %s\n", 
		       (int)bb->bit_search_count,
		       (int)bb->skip_search_count,
		       (int)Value, (int)Index, (int)bb->Limit,
		       bb->Name);
#endif
       return -1;
    }

    EntriesPerBlock = (bb->BlockSize * 8);
    Block = Index / EntriesPerBlock;
    Offset = Index % EntriesPerBlock;

    blk = bb->Head;
    while (blk)
    {
       if (blk->BlockIndex == Block)
       {
	  while (blk)
	  {
             i = 0;
             if (blk->BlockIndex == Block)
                i = Offset;

	     if (Value & 1)
             {
	        while ((i < EntriesPerBlock) &&
	              (((blk->BlockIndex * EntriesPerBlock) + i) < bb->Limit))
	        {
                   // if this index is on a dword boundry, then evaluate
		   // this value as a dword to speed table searching
     		   if (!(i % (sizeof(ULONG) * 8)) && 
		       ((i + (sizeof(ULONG) * 8)) < EntriesPerBlock))
		   {
		      if (*(ULONG *)&blk->BlockBuffer[i >> 3] == 0)
		      {
                         i += (sizeof(ULONG) * 8);
#if (PROFILE_BIT_SEARCH)
			 bb->bit_search_count += (sizeof(ULONG) * 8);
                         bb->skip_search_count++;
#endif
			 continue;
		      }
		   }
				   
     		   if (((blk->BlockBuffer[i >> 3]) >> (i & 7)) & 1)
                   {
                      register int j, k; 
     
	              for (k = 0, j = i; 
		          (k < Range) && ((j < EntriesPerBlock) &&
	                  (((blk->BlockIndex * EntriesPerBlock) + j) < 
			     bb->Limit));
			  j++, k++)
	              {
                         if (!(((blk->BlockBuffer[j >> 3]) >> (j & 7)) & 1))
		            break;
	              }

		      if (k == Range) 
		      {
		         NWUnlockBB(bb);
#if (VERBOSE)
                         NWFSPrint("ScanBitBlockValueWithIndex - %d\n",
		                (int)((blk->BlockIndex * EntriesPerBlock) + i));
#endif

#if (PROFILE_BIT_SEARCH)
                         if (bb->bit_search_count > BIT_SEARCH_THRESHOLD) 
		            NWFSPrint("swi:count-%d-%d value-%d index/lim-%d/%d (%X) %s\n", 
		               (int)bb->bit_search_count,
		               (int)bb->skip_search_count,
		               (int)Value, (int)Index, (int)bb->Limit,
		               (unsigned)((blk->BlockIndex * 
					EntriesPerBlock) + i), 
         		       bb->Name);
#endif
		         return ((blk->BlockIndex * EntriesPerBlock) + i);
		      }
		      else
		         i = j;
                   }
                   i++;
#if (PROFILE_BIT_SEARCH)
                   bb->bit_search_count++;
#endif
    
		}
             }
             else
             {
	        while ((i < EntriesPerBlock) &&
	              (((blk->BlockIndex * EntriesPerBlock) + i) < bb->Limit))
		{
                   // if this index is on a dword boundry, then evaluate
		   // this value as a dword to speed table searching
     		   if (!(i % (sizeof(ULONG) * 8)) && 
		       ((i + (sizeof(ULONG) * 8)) < EntriesPerBlock))
		   {
		      if (*(ULONG *)&blk->BlockBuffer[i >> 3] == (ULONG)-1)
		      {
                         i += (sizeof(ULONG) * 8);
#if (PROFILE_BIT_SEARCH)
			 bb->bit_search_count += (sizeof(ULONG) * 8);
                         bb->skip_search_count++;
#endif
			 continue;
		      }
		   }

		   if (!(((blk->BlockBuffer[i >> 3]) >> (i & 7)) & 1))
                   {
                      register int j, k; 
     
	              for (k = 0, j = i; 
			  (k < Range) && ((j < EntriesPerBlock) &&
	                  (((blk->BlockIndex * EntriesPerBlock) + j) < 
			     bb->Limit));
			  j++, k++)
	              {
                         if (((blk->BlockBuffer[j >> 3]) >> (j & 7)) & 1)
		            break;
	              }

		      if (k == Range) 
		      {
		         NWUnlockBB(bb);
#if (VERBOSE)
                         NWFSPrint("ScanBitBlockValueWithIndex - %d\n",
		                (int)((blk->BlockIndex * EntriesPerBlock) + i));
#endif

#if (PROFILE_BIT_SEARCH)
                         if (bb->bit_search_count > BIT_SEARCH_THRESHOLD) 
                            NWFSPrint("swi:count-%d-%d value-%d index/lim-%d/%d (%X) %s\n", 
		               (int)bb->bit_search_count,
		               (int)bb->skip_search_count,
		               (int)Value, (int)Index, (int)bb->Limit,
		               (unsigned)((blk->BlockIndex * 
					EntriesPerBlock) + i), 
            		       bb->Name);
#endif
                         return ((blk->BlockIndex * EntriesPerBlock) + i);
		      }
		      else
		         i = j;
                   }
                   i++;
#if (PROFILE_BIT_SEARCH)
                   bb->bit_search_count++;
#endif
                }
	     }
	     blk = blk->next;
	  }
          NWUnlockBB(bb);
#if (VERBOSE)
          NWFSPrint("ScanBitBlockValueWithIndex(2) - -1\n");
#endif

#if (PROFILE_BIT_SEARCH)
          NWFSPrint("swi:count-%d-%d value-%d index/lim-%d/%d (-1) %s\n", 
		       (int)bb->bit_search_count,
		       (int)bb->skip_search_count,
		       (int)Value, (int)Index, (int)bb->Limit,
		       bb->Name);
#endif
          return -1;
       }
       blk = blk->next;
    }
    NWUnlockBB(bb);
#if (VERBOSE)
    NWFSPrint("ScanBitBlockValueWithIndex(1) - -1\n");
#endif

#if (PROFILE_BIT_SEARCH)
    NWFSPrint("swi:count-%d-%d value-%d index/lim-%d/%d (-1) %s\n", 
		       (int)bb->bit_search_count,
		       (int)bb->skip_search_count,
		       (int)Value, (int)Index, (int)bb->Limit,
		       bb->Name);
#endif
    return -1;

}

ULONG ScanAndSetBitBlockValueWithIndex(BIT_BLOCK_HEAD *bb, ULONG Value,
				       ULONG Index, ULONG Range)
{
    register ULONG i;
    register BIT_BLOCK *blk;
    register ULONG EntriesPerBlock;
    register ULONG Block, Offset;

    NWLockBB(bb);

#if (PROFILE_BIT_SEARCH)
    bb->bit_search_count = 0;
    bb->skip_search_count = 0;
#endif

    if (Index > (bb->Limit - 1))
    {
       NWUnlockBB(bb);
#if (VERBOSE)
       NWFSPrint("ScanAndSetBitBlockValueWithIndex - index(%d) > limit(%d)\n",
                 (int)Index, (int)bb->Limit);
#endif

#if (PROFILE_BIT_SEARCH)
       NWFSPrint("sswi:count-%d-%d value-%d index/lim-%d/%d (index > limit) %s\n",
		       (int)bb->bit_search_count,
		       (int)bb->skip_search_count,
		       (int)Value, (int)Index, (int)bb->Limit,
		       bb->Name);
#endif
       return -1;
    }

    EntriesPerBlock = (bb->BlockSize * 8);
    Block = Index / EntriesPerBlock;
    Offset = Index % EntriesPerBlock;

    blk = bb->Head;
    while (blk)
    {
       if (blk->BlockIndex == Block)
       {
	  while (blk)
	  {
             i = 0;
             if (blk->BlockIndex == Block)
                i = Offset;

	     if (Value & 1)
             {
	        while ((i < EntriesPerBlock) &&
	              (((blk->BlockIndex * EntriesPerBlock) + i) < bb->Limit))
	        {
                   // if this index is on a dword boundry, then evaluate
		   // this value as a dword to speed table searching

     		   if (!(i % (sizeof(ULONG) * 8)) && 
		       ((i + (sizeof(ULONG) * 8)) < EntriesPerBlock))
		   {
		      if (*(ULONG *)&blk->BlockBuffer[i >> 3] == 0)
		      {
                         i += (sizeof(ULONG) * 8);
#if (PROFILE_BIT_SEARCH)
                         bb->bit_search_count += (sizeof(ULONG) * 8);
                         bb->skip_search_count++;
#endif
     		         continue;
		      }
		   }
			
		   if (((blk->BlockBuffer[i >> 3]) >> (i & 7)) & 1)
                   {
                      register int j, k; 
     
	              for (k = 0, j = i; 
			  (k < Range) && ((j < EntriesPerBlock) &&
	                  (((blk->BlockIndex * EntriesPerBlock) + j) < 
			     bb->Limit));
			  j++, k++)
	              {
                         if (!(((blk->BlockBuffer[j >> 3]) >> (j & 7)) & 1))
		            break;
	              }

		      if (k == Range) 
		      {
	                 for (k = 0, j = i; 
			     (k < Range) && ((j < EntriesPerBlock) &&
	                     (((blk->BlockIndex * EntriesPerBlock) + j) < 
			        bb->Limit));
			     j++, k++)
		         {
		            blk->BlockBuffer[j >> 3] &= ~(1 << (j & 7));
		         }

			 NWUnlockBB(bb);
#if (VERBOSE)
                         NWFSPrint("ScanAndSetBitBlockValueWithIndex - %d\n",
		                (int)((blk->BlockIndex * EntriesPerBlock) + i));
#endif

#if (PROFILE_BIT_SEARCH)
                         if (bb->bit_search_count > BIT_SEARCH_THRESHOLD) 
                            NWFSPrint("sswi:count-%d-%d value-%d index/lim-%d/%d (%X) %s\n", 
		               (int)bb->bit_search_count,
		               (int)bb->skip_search_count,
		               (int)Value, (int)Index, (int)bb->Limit,
		               (unsigned)((blk->BlockIndex * 
					EntriesPerBlock) + i), 
              		       bb->Name);
#endif
                         return ((blk->BlockIndex * EntriesPerBlock) + i);
		      }
                      else
		         i = j;
                   }
                   i++;
#if (PROFILE_BIT_SEARCH)
                   bb->bit_search_count++;
#endif
		}
             }
             else
             {
	        while ((i < EntriesPerBlock) &&
	              (((blk->BlockIndex * EntriesPerBlock) + i) < bb->Limit))
	        {
                   // if this index is on a dword boundry, then evaluate
		   // this value as a dword to speed table searching

     		   if (!(i % (sizeof(ULONG) * 8)) && 
		       ((i + (sizeof(ULONG) * 8)) < EntriesPerBlock))
		   {
		      if (*(ULONG *)&blk->BlockBuffer[i >> 3] == (ULONG)-1)
		      {
                         i += (sizeof(ULONG) * 8);
#if (PROFILE_BIT_SEARCH)
			 bb->bit_search_count += (sizeof(ULONG) * 8);
                         bb->skip_search_count++;
#endif
			 continue;
		      }
		   }

		   if (!(((blk->BlockBuffer[i >> 3]) >> (i & 7)) & 1))
                   {
                      register int j, k; 
     
	              for (k = 0, j = i; 
			  (k < Range) && ((j < EntriesPerBlock) &&
	                  (((blk->BlockIndex * EntriesPerBlock) + j) < 
			     bb->Limit));
			  j++, k++)
	              {
                         if (((blk->BlockBuffer[j >> 3]) >> (j & 7)) & 1)
		            break;
	              }

		      if (k == Range) 
		      {
	                 for (k = 0, j = i; 
			     (k < Range) && ((j < EntriesPerBlock) &&
	                     (((blk->BlockIndex * EntriesPerBlock) + j) < 
			        bb->Limit));
			     j++, k++)
		         {
	                    blk->BlockBuffer[j >> 3] |= (1 << (j & 7));
		         }

			 NWUnlockBB(bb);
#if (VERBOSE)
                         NWFSPrint("ScanAndSetBitBlockValueWithIndex - %d\n",
		                (int)((blk->BlockIndex * EntriesPerBlock) + i));
#endif

#if (PROFILE_BIT_SEARCH)
                         if (bb->bit_search_count > BIT_SEARCH_THRESHOLD) 
                            NWFSPrint("sswi:count-%d-%d value-%d index/lim-%d/%d (%X) %s\n", 
		               (int)bb->bit_search_count,
		               (int)bb->skip_search_count,
        		       (int)Value, (int)Index, (int)bb->Limit,
		               (unsigned)((blk->BlockIndex * 
					EntriesPerBlock) + i),
             		       bb->Name);
#endif
		         return ((blk->BlockIndex * EntriesPerBlock) + i);
		      }
		      else
		         i = j;
                   }
                   i++;
#if (PROFILE_BIT_SEARCH)
                   bb->bit_search_count++;
#endif
                }
	     }
	     blk = blk->next;
	  }
          NWUnlockBB(bb);
#if (VERBOSE)
          NWFSPrint("ScanAndSetBitBlockValueWithIndex(2) - -1\n");
#endif

#if (PROFILE_BIT_SEARCH)
          NWFSPrint("sswi:count-%d-%d value-%d index/lim-%d/%d (-1) %s\n", 
		       (int)bb->bit_search_count,
		       (int)bb->skip_search_count,
		       (int)Value, (int)Index, (int)bb->Limit,
		       bb->Name);
#endif
          return -1;
       }
       blk = blk->next;
    }
    NWUnlockBB(bb);
#if (VERBOSE)
    NWFSPrint("ScanAndSetBitBlockValueWithIndex(1) - -1\n");
#endif

#if (PROFILE_BIT_SEARCH)
    NWFSPrint("sswi:count-%d-%d value-%d index/lim-%d/%d (-1) %s\n", 
		       (int)bb->bit_search_count,
		       (int)bb->skip_search_count,
		       (int)Value, (int)Index, (int)bb->Limit,
		       bb->Name);
#endif
    return -1;

}

ULONG BitScanValueWithIndex(BYTE *p, ULONG size, ULONG Value, 
		            ULONG Index, ULONG Range)
{
    register ULONG i;
    register ULONG EntriesPerBlock;
    
    if (Index > (size - 1))
       return -1;

    EntriesPerBlock = (size * 8);
    i = Index % EntriesPerBlock;
    if (Value & 1)
    {
       while ((i < EntriesPerBlock) && ((EntriesPerBlock + i) < size))
       {
          if (!(i % (sizeof(ULONG) * 8)) && 
              ((i + (sizeof(ULONG) * 8)) < EntriesPerBlock))
	  {
	     if (*(ULONG *)&p[i >> 3] == 0)
	     {
                i += (sizeof(ULONG) * 8);
	        continue;
	     }
	  }
				   
          if (((p[i >> 3]) >> (i & 7)) & 1)
          {
             register int j, k; 
     
	     for (k = 0, j = i; (k < Range) && ((j < EntriesPerBlock) && 
			 ((EntriesPerBlock + j) < size)); j++, k++)
	     {
                if (!(((p[j >> 3]) >> (j & 7)) & 1))
		   break;
	     }
	     if (k == Range)
	        return i;
	     else
	        i = j;
          }
          i++;
       }
    }
    else
    {
       while ((i < EntriesPerBlock) && ((EntriesPerBlock + i) < size))
       {
          if (!(i % (sizeof(ULONG) * 8)) && 
	      ((i + (sizeof(ULONG) * 8)) < EntriesPerBlock))
	  {
             if (*(ULONG *)&p[i >> 3] == (ULONG)-1)
	     {
                i += (sizeof(ULONG) * 8);
	        continue;
	     }
	  }

	  if (!(((p[i >> 3]) >> (i & 7)) & 1))
          {
             register int j, k; 
     
	     for (k = 0, j = i; (k < Range) && ((j < EntriesPerBlock) && 
			 ((EntriesPerBlock + j) < size)); j++, k++)
	     {
                if (((p[j >> 3]) >> (j & 7)) & 1)
		   break;
	     }
	     if (k == Range)
	        return i;
	     else
	        i = j;
          }
          i++;
       }
    }  
    return -1;

}

ULONG BitScanAndSetValueWithIndex(BYTE *p, ULONG size, ULONG Value,
				  ULONG Index, ULONG Range)
{
    register ULONG i;
    register ULONG EntriesPerBlock;

    if (Index > (size - 1))
       return -1;

    EntriesPerBlock = (size * 8);
    i = Index % EntriesPerBlock;
    if (Value & 1)
    {
       while ((i < EntriesPerBlock) && ((EntriesPerBlock + i) < size))
       {
          if (!(i % (sizeof(ULONG) * 8)) && 
              ((i + (sizeof(ULONG) * 8)) < EntriesPerBlock))
	  {
	     if (*(ULONG *)&p[i >> 3] == 0)
	     {
                i += (sizeof(ULONG) * 8);
     	        continue;
	     }
	  }
			
	  if (((p[i >> 3]) >> (i & 7)) & 1)
          {
             register int j, k; 
     
	     for (k = 0, j = i; (k < Range) && ((j < EntriesPerBlock) && 
			 ((EntriesPerBlock + j) < size)); j++, k++)
	     {
                if (!(((p[j >> 3]) >> (j & 7)) & 1))
		   break;
	     }

	     if (k == Range)
	     {
	        for (k = 0, j = i; (k < Range) && ((j < EntriesPerBlock) && 
			 ((EntriesPerBlock + j) < size)); j++, k++)
		{
     	           p[j >> 3] &= ~(1 << (j & 7));
		}
	        return i;
	     }
	     else
	        i = j;
          }
          i++;
       }
    }
    else
    {
       while ((i < EntriesPerBlock) && ((EntriesPerBlock + i) < size))
       {
          if (!(i % (sizeof(ULONG) * 8)) && 
	      ((i + (sizeof(ULONG) * 8)) < EntriesPerBlock))
	  {
	     if (*(ULONG *)&p[i >> 3] == (ULONG)-1)
	     {
                i += (sizeof(ULONG) * 8);
	        continue;
	     }
	  }

	  if (!(((p[i >> 3]) >> (i & 7)) & 1))
          {
             register int j, k; 
     
	     for (k = 0, j = i; (k < Range) && ((j < EntriesPerBlock) && 
			 ((EntriesPerBlock + j) < size)); j++, k++)
	     {
                if (((p[j >> 3]) >> (j & 7)) & 1)
		   break;
	     }
	     
	     if (k == Range)
	     {
	        for (k = 0, j = i; (k < Range) && ((j < EntriesPerBlock) && 
			 ((EntriesPerBlock + j) < size)); j++, k++)
		{
                   p[j >> 3] |= (1 << (j & 7));
		}
	        return i;
	     }
	     else
	        i = j;
          }
          i++;
       }
    }
    return -1;

}

