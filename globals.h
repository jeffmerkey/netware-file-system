
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
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  GLOBALS.H
*   DESCRIP  :  NWFS Global Declarations
*   DATE     :  November 1, 1998
*
*
***************************************************************************/

#ifndef _NWFS_GLOBALS_
#define _NWFS_GLOBALS_

#if (OVERRIDE)

#if LINUX_DRIVER

#if MODULE
#ifndef NWFSMOD_VERSION_INFO
#define __NO_VERSION__
#endif
#if MODVERSIONS
#include <linux/modversions.h>
#endif
#include <linux/module.h>
#endif
#include <linux/version.h>

#if LINUX_VERSION_CODE >= 0x020400
#define  LINUX_24                 1
#elif LINUX_VERSION_CODE >= 0x020200
#define  LINUX_22                 1
#elif LINUX_VERSION_CODE >= 0x020000
#define  LINUX_20                 1
#endif

#endif

#else

#define  WINDOWS_NT_RO            0
#define  WINDOWS_NT               0
#define  WINDOWS_NT_UTIL          0
#define  WINDOWS_CONVERT          0
#define  WINDOWS_98_UTIL          0
#define  LINUX_20                 0
#define  LINUX_22                 0
#define  LINUX_24                 0
#define  LINUX_UTIL               1
#define  DOS_UTIL                 0

#endif

#define	uint32	unsigned long
#define	uint8	unsigned char

#if (LINUX_20)
#define  LINUX            1
#define  LINUX_SLEEP      1
#define  LINUX_SPIN       0
#endif
#if (LINUX_22)
#define  LINUX            1
#define  LINUX_SLEEP      1
#define  LINUX_SPIN       1
#endif
#if (LINUX_24)
#define  LINUX            1
#define  LINUX_SLEEP      1
#define  LINUX_SPIN       1
#endif
#if (LINUX_UTIL)
#define  LINUX            1
#endif

#if (WINDOWS_98_UTIL)
#undef   DOS_UTIL
#define  DOS_UTIL         1
#endif

//
//   define version of this utility
//

#define  MAJOR_VERSION  4
#define  MINOR_VERSION  0
#define  BUILD_VERSION  0
#define  BUILD_YEAR     2001

// debug flags
#define  VERBOSE                0  // enable verbose output
#define  VFS_VERBOSE            0  // trace VFS/NWFS interaction
#define  MOUNT_VERBOSE          0  // enable verbose output during mounts
#define  HOTFIX_VERBOSE         0  // trace hotfix table lookups
#define  VOLUME_DUMP            0  // dump volume segment info as detected
#define  DEBUG_SA               0  // trace suballocation reads/writes
#define  CHECK_NLINK            0  // perform nlink validation during dir delete
#define  DEBUG_DEADLOCKS        0  // allows deadlocks to receive EINTR signals
#define  PROFILE_AIO_VERBOSE    0  // print profile aio efficiency stats
#define  DEBUG_AIO              0  // trace linux AIO calls
#define  DEBUG_LRU_AIO          0  // trace LRU/AIO interaction
#define  CREATE_FAT_MISMATCH    0  // generate fat mismatches
#define  CREATE_DIR_MISMATCH    0  // generate directory mismatches
#define  CREATE_EDIR_MISMATCH   0  // generate extended directory mismatches
#define  POST_IMMEDIATE         0  // force immediate post of I/O requests 
#define  DUMP_BUFFER_HEADS      0  // dump buffer heads to /var/log/messages
#define  DEBUG_MIRRORING        0  // debug remirroring and validation
#define  DEBUG_MIRROR_CONTROL   0  // debug remirroring control allocation
#define  DUMP_LEAKED_MEMORY     0  // dump leaked memory headers
#define  MONITOR_LRU_COUNT      0  // monitor lru cache sharing count
#define  VERIFY_NAMESPACE_LIST  0  // verify name hash list before delete
#define  PROFILE_BIT_SEARCH     0  // profile bit list search efficiency
#define  BIT_SEARCH_THRESHOLD   2  // bit list search threshold 
#define  DEBUG_CODE             0  // enable debugging code subsystems  

// feature enable flags
#define  PROFILE_AIO            1  // profile aio efficiency
#define  TRUE_NETWARE_MODE      0  // enable full NetWare compatibility
#define  SALVAGEABLE_FS         0  // enable the salvageable file system
#define  TURBO_FAT_ON           1  // enable Turbo Fat Indexing
#define  STRICT_NAMES           1  // strict name validation during mount
#define  LRU_FACTOR             2  // % dirty for flush (1-100% 2-50% 3-33%)
#define  LRU_AGE                2  // how many seconds to age dirty data blocks
#define  JOURNAL_AGE            3  // how many seconds to age dirty dir blocks
#define  FAT_AGE                3  // how many seconds to age dirty fat blocks
#define  RAPID_MOUNT_AGE        3  // how many seconds to age dirty fat blocks
#define  ALLOW_REMOUNT          1  // allow remounting of NetWare Volumes
#define  PAGE_CACHE_ON          0  // enable Linux page cache interface
#define  CACHE_FAT_TABLES       0  // cache and pin the fat tables in memory
#define  SINGLE_DIRTY_LIST      1  // disables dirty lru skip list 
#define  AUTO_CREATE_META_FILES 0  // auto create volume meta files    

// extended raid and I2O Device Support
#define  META_DISK              0  // enable /dev/mdXX Raid Devices
#define  ALL_EXT_DEVICES        0  // enable all extended device support
#define  COMPAQ_SMART2_RAID     0  // enable COMPAQ Smart2 Raid support
#define  DAC960_RAID            0  // enable DAC960 Raid Support 
#define  I2O_DEVICES            0  // enable I2O Device Support

// NOTE:  if you disable this, then the elevator moves to the LRU
// otherwise the AIO manager maintains the elevator.
#define  DO_ASYNCH_IO           1  // enable the asynch I/O subsystem
#define  READ_AHEAD_ON          1  // enable read ahead capability
#define  ASYNCH_READ_AHEAD      1  // enable asynch read ahead capability
#define  LINUX_BUFFER_CACHE     0  // double buffer data in the linux cache
#define  ZERO_FILL_SECTORS      0  // zero fill newly allocated sectors
#define  CONSERVE_MEMORY        0  // conserve unused AIO/WorkSpace Memory
#define  LINUX_AIO              1  // use Linux 2.2/2.4 AIO subsystem
#define  HASH_FAT_CHAINS        1  // hash file FAT chain heads 
#define  FREE_UNUSED_DIR_SPACE  1  // release unclaimed dir space during mount
#define  LOGICAL_BLOCK_SIZE   512  // set to 512, 1024, 2048, 4096 block size

#if (TRUE_NETWARE_MODE)
#undef LOGICAL_BLOCK_SIZE
#define  LOGICAL_BLOCK_SIZE    512 // NetWare mode defaults to 512 block size
#endif

// these values can be changed if you want to increase the numbers of
// disks/volumes that NWFS will support

#define  MAX_DISKS             256  // max disks
#define  MAX_DOS_DISKS         128  // max disks supported by MSDOS
#define  MAX_PARTITIONS        MAX_DISKS * 4
#define  MAX_VOLUMES           256  // netware supports 256 volumes/server
#define  MAX_BUFFER_HEADS     4096  // buffer head pool for AIO
#define  MAX_WORKSPACE_BUFFERS  32  // max workspaces/volume
#define  MAX_MACHINES          200  // max nodes per cluster

// these values are architecture dependent and should not be changed
#define  MAX_VOLUME_ENTRIES      8  // 8 entries per volume table (partition)
#define  MAX_SEGMENTS           32  // a volume can have 32 segments
#define  MAX_MIRRORS             8  // NetWare max is 8
#define  MAX_NAMESPACES         10  // 12 in 4.x, 10 in 3.x
#define  MAX_NAMESPACE_EXTANTS   3  // max number of extended namespaces
#define  MAX_SUBALLOC_NODES      5  // max number of suballoc headers/volume
#define  MAX_SUBALLOC_SPAN       2  // max suballoc nodes an entry can span
#define  SUBALLOC_BLOCK_SIZE   512  // minumum suballocation size
#define  MAX_HOTFIX_BLOCKS      30  // max hotfix block list table entries 

#define  IO_BLOCK_SIZE        4096  // volume block size default
#define  BIT_BLOCK_SIZE       IO_BLOCK_SIZE
#define  HOTFIX_BLOCK_SIZE    IO_BLOCK_SIZE


#if (ALL_EXT_DEVICES)
#undef   COMPAQ_SMART2_RAID
#undef   DAC960_RAID
#undef   I2O_DEVICES       
#define  COMPAQ_SMART2_RAID     1
#define  DAC960_RAID            1 
#define  I2O_DEVICES            1 
#endif

#if (COMPAQ_SMART2_RAID | DAC960RAID | I2O_DEVICES)
#undef   MAX_DISKS              
#define  MAX_DISKS                 600  
#endif

// this defines the maximum size of the SMP LRU for NWFS.  If you
// start getting "virtual memory exhausted" messages, you may want
// to crank down this value for Linux.  

#define  MIN_RAPID_MOUNT_BLOCKS    0
#define  MAX_RAPID_MOUNT_BLOCKS    100
#define  MIN_FAT_LRU_BLOCKS        0
#define  MAX_FAT_LRU_BLOCKS        100
#define  MIN_DIRECTORY_LRU_BLOCKS  10
#define  MAX_DIRECTORY_LRU_BLOCKS  500
#define  MIN_VOLUME_LRU_BLOCKS     10
#define  MAX_VOLUME_LRU_BLOCKS     1000

#if (DOS_UTIL)
#undef   MAX_VOLUME_LRU_BLOCKS
#undef   MAX_DIRECTORY_LRU_BLOCKS
#undef   MAX_FAT_LRU_BLOCKS
#define  MAX_VOLUME_LRU_BLOCKS     500
#define  MAX_DIRECTORY_LRU_BLOCKS  500
#define  MAX_FAT_LRU_BLOCKS        100
#endif

#if (WINDOWS_NT | WINDOWS_NT_RO)
#undef   MAX_VOLUME_LRU_BLOCKS
#undef   MAX_DIRECTORY_LRU_BLOCKS
#undef   MAX_FAT_LRU_BLOCKS
#define  MAX_VOLUME_LRU_BLOCKS     1000
#define  MAX_DIRECTORY_LRU_BLOCKS   500
#define  MAX_FAT_LRU_BLOCKS         100
#endif

#if (LINUX_20 | LINUX_22)
#undef   MAX_VOLUME_LRU_BLOCKS
#undef   MAX_DIRECTORY_LRU_BLOCKS
#undef   MAX_FAT_LRU_BLOCKS
#if (LINUX_BUFFER_CACHE)
#define  MAX_VOLUME_LRU_BLOCKS      500
#define  MAX_DIRECTORY_LRU_BLOCKS   500
#define  MAX_FAT_LRU_BLOCKS         100
#else
#define  MAX_VOLUME_LRU_BLOCKS     1000
#define  MAX_DIRECTORY_LRU_BLOCKS   500
#define  MAX_FAT_LRU_BLOCKS         100
#endif
#endif

#if (LINUX_24)
#undef   MAX_VOLUME_LRU_BLOCKS
#undef   MAX_DIRECTORY_LRU_BLOCKS
#undef   MAX_FAT_LRU_BLOCKS
#if (LINUX_BUFFER_CACHE)
#define  MAX_VOLUME_LRU_BLOCKS      500
#define  MAX_DIRECTORY_LRU_BLOCKS   500
#define  MAX_FAT_LRU_BLOCKS         100
#else
#define  MAX_VOLUME_LRU_BLOCKS     1000
#define  MAX_DIRECTORY_LRU_BLOCKS   500
#define  MAX_FAT_LRU_BLOCKS         100
#endif
#endif

#define MAX_LRU_BLOCKS   (MAX_VOLUME_LRU_BLOCKS  +                                                        MAX_DIRECTORY_LRU_BLOCKS +                                                      MAX_FAT_LRU_BLOCKS)

// LRU Eviction priority values
#define  DATA_PRIORITY           1
#define  DIRECTORY_PRIORITY      2
#define  FAT_PRIORITY            3
#define  RAW_PRIORITY            4

//
//  global includes for NT
//

#if (WINDOWS_NT | WINDOWS_NT_RO)

#define TargetOS "Windows NT"

#include "ntifs.h"
#include "ntdddisk.h"

#define NWFSPrint DbgPrint

typedef UCHAR BYTE;
typedef USHORT WORD;

#include "stdio.h"
#include "stdlib.h"

#include "nwfs.h"
#include "nwhash.h"
#include "nwstruct.h"
#include "nwerror.h"
#include "nwdir.h"
#include "nwproc.h"

#include "nwvphal.h"
#include "nwvp.h"

#include "nwioctl.h"

#endif

//
//  global includes for NT utilities
//

#if (WINDOWS_NT_UTIL)

#if (M2FS)
#define CONFIG_NAME        "  M2FS Volume Manager for Windows NT/2000"
#define IMAGER_NAME        "  M2FS Volume IMAGER for Windows NT/2000"
#else
#define CONFIG_NAME        "  NWFS Volume Manager for Windows NT/2000"
#define IMAGER_NAME        "  NWFS Volume IMAGER for Windows NT/2000"
#endif

#define TargetOS "Windows NT"

#define COPYRIGHT_NOTICE1  "  Copyright (c) 1997-2001 Jeff V. Merkey  All Rights Reserved."
#define COPYRIGHT_NOTICE2  "  "


#include "windows.h"
#include "winioctl.h"
#include "winuser.h"
#include "stdarg.h"

extern int log_printf(char *format, ...);
extern int log_sprintf(char *buffer, char *format, ...);

#define NWFSPrint log_printf

typedef UCHAR BYTE;
typedef USHORT WORD;

#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "conio.h"

#include "nwfs.h"
#include "nwhash.h"
#include "nwstruct.h"
#include "nwerror.h"
#include "nwdir.h"
#include "nwproc.h"

#include "nwvphal.h"
#include "nwvp.h"
#include "fileutil.h"

#include "nwmenu.h"

#endif

//
//
//

#if (DOS_UTIL)

#if (WINDOWS_98_UTIL)

#if (M2FS)
#define CONFIG_NAME        "  M2FS Volume Manager for Windows 95/98"
#define IMAGER_NAME        "  M2FS Volume IMAGER for Windows 95/98"
#else
#define CONFIG_NAME        "  NWFS Volume Manager for Windows 95/98"
#define IMAGER_NAME        "  NWFS Volume IMAGER for Windows 95/98"
#endif

#define TargetOS "Windows 95/98"

#else

#if (M2FS)
#define CONFIG_NAME        "  M2FS Volume Manager for DOS"
#define IMAGER_NAME        "  M2FS Volume IMAGER for DOS"
#else
#define CONFIG_NAME        "  NWFS Volume Manager for DOS"
#define IMAGER_NAME        "  NWFS Volume IMAGER for DOS"
#endif

#define TargetOS "DOS"
#endif

#define COPYRIGHT_NOTICE1  "  Copyright (c) 1997-2001 Jeff V. Merkey  All Rights Reserved."
#define COPYRIGHT_NOTICE2  "  "

#define  TRUE               1
#define  FALSE              0

#include "unistd.h"
#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "fcntl.h"
#include "ctype.h"
#include "string.h"
#include "memory.h"
#include "dos.h"
#include "conio.h"

#include "sys/farptr.h"
#include "sys/movedata.h"
#include "sys/segments.h"
#include "sys/stat.h"
#include "dpmi.h"

#include "time.h"

extern int log_printf(char *format, ...);
extern int log_sprintf(char *buffer, char *format, ...);

#define NWFSPrint log_printf

#ifndef LONGLONG
typedef long long LONGLONG;
#endif

typedef unsigned long  ULONG;
typedef unsigned short WORD;
typedef unsigned char  BYTE;

#include "nwfs.h"
#include "nwhash.h"
#include "nwstruct.h"
#include "nwerror.h"
#include "nwdir.h"
#include "nwproc.h"

#include "nwvphal.h"
#include "nwvp.h"
#include "fileutil.h"

#include "nwmenu.h"

#endif

//
//  global includes for Linux
//

//
//   Linux version 2.1.0
//

#if (LINUX_20)

#if (M2FS)
#define CONFIG_NAME        "  M2FS Clustered File System"
#define IMAGER_NAME        "  M2FS Clustered File System"
#else
#define CONFIG_NAME        "  NWFS NetWare File System"
#define IMAGER_NAME        "  NWFS NetWare File System"
#endif

#define COPYRIGHT_NOTICE1  "  Copyright (c) 1997-2001 Jeff V. Merkey  All Rights Reserved."
#define COPYRIGHT_NOTICE2  "  "

#define TargetOS "Linux 20"

#define  TRUE               1
#define  FALSE              0

#ifndef LONGLONG
typedef long long LONGLONG;
#endif

typedef unsigned long  ULONG;
typedef unsigned short WORD;
typedef unsigned char  BYTE;

#define NWFSPrint printk

#if MODULE
#ifdef NWFSMOD_VERSION_INFO
#include <linux/module.h>
#endif
#if MODVERSIONS
#include <linux/modversions.h>
#endif
#endif

#include <linux/version.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/locks.h>
#include <linux/fcntl.h>
#include <linux/malloc.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/mm.h>
#include <linux/cdrom.h>
#include <linux/locks.h>
#include <linux/pagemap.h>
#include <linux/swap.h>
#include <linux/swapctl.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>

#include <asm/system.h>
#include <asm/segment.h>
#include <asm/atomic.h>
#include <sys/ioctl.h>

#define __KERNEL_SYSCALLS__
#include <linux/unistd.h>

#define NWFSInitMutex(s)        struct semaphore (s)=MUTEX
#define NWFSInitSemaphore(s)    struct semaphore (s)=MUTEX_LOCKED

#include "nwfs.h"
#include "nwhash.h"
#include "nwstruct.h"
#include "nwerror.h"
#include "nwdir.h"
#include "nwproc.h"

#include "nwvphal.h"
#include "nwvp.h"

#include "nwvfs.h"

#endif

//
//   Linux version 2.1.2 
//

#if (LINUX_22)

#if (M2FS)
#define CONFIG_NAME        "  M2FS Clustered File System"
#define IMAGER_NAME        "  M2FS Clustered File System"
#else
#define CONFIG_NAME        "  NWFS NetWare File System"
#define IMAGER_NAME        "  NWFS NetWare File System"
#endif

#define COPYRIGHT_NOTICE1  "  Copyright (c) 1997-2001 Jeff V. Merkey  All Rights Reserved."
#define COPYRIGHT_NOTICE2  "  "

#define TargetOS "Linux 22"

#define  TRUE               1
#define  FALSE              0

#ifndef LONGLONG
typedef long long LONGLONG;
#endif

typedef unsigned long  ULONG;
typedef unsigned short WORD;
typedef unsigned char  BYTE;

#define NWFSPrint printk

#if MODULE
#ifndef NWFSMOD_VERSION_INFO
#define __NO_VERSION__
#endif
#if MODVERSIONS
#include <linux/modversions.h>
#endif
#include <linux/module.h>
#endif

#include <linux/version.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/locks.h>
#include <linux/fcntl.h>
#include <linux/malloc.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/mm.h>
#include <linux/cdrom.h>
#include <linux/locks.h>
#include <linux/pagemap.h>
#include <linux/swap.h>
#include <linux/swapctl.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>
#include <linux/poll.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <asm/atomic.h>

#define __KERNEL_SYSCALLS__
#include <linux/unistd.h>

#define NWFSInitMutex(s)        struct semaphore (s)=MUTEX
#define NWFSInitSemaphore(s)    struct semaphore (s)=MUTEX_LOCKED

#include "nwfs.h"
#include "nwhash.h"
#include "nwstruct.h"
#include "nwerror.h"
#include "nwdir.h"
#include "nwproc.h"

#include "nwvphal.h"
#include "nwvp.h"

#include "nwvfs.h"

#endif

//
//
//   Linux version 2.1.4 
//

#if (LINUX_24)

#if (M2FS)
#define CONFIG_NAME        "  M2FS Clustered File System"
#define IMAGER_NAME        "  M2FS Clustered File System"
#else
#define CONFIG_NAME        "  NWFS NetWare File System"
#define IMAGER_NAME        "  NWFS NetWare File System"
#endif

#define COPYRIGHT_NOTICE1  "  Copyright (c) 1997-2001 Jeff V. Merkey  All Rights Reserved."
#define COPYRIGHT_NOTICE2  "  "

#define TargetOS "Linux 24"

#define NWFSInitMutex(s)         DECLARE_MUTEX(s)
#define NWFSInitSemaphore(s)     DECLARE_MUTEX_LOCKED(s)

#define  TRUE               1
#define  FALSE              0

#ifndef LONGLONG
typedef long long LONGLONG;
#endif

typedef unsigned long  ULONG;
typedef unsigned short WORD;
typedef unsigned char  BYTE;

#define NWFSPrint printk

#if MODULE
#ifndef NWFSMOD_VERSION_INFO
#define __NO_VERSION__
#endif
#if MODVERSIONS
#include <linux/modversions.h>
#endif
#include <linux/module.h>
#endif

#include <linux/version.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/locks.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/mm.h>
#include <linux/cdrom.h>
#include <linux/locks.h>
#include <linux/pagemap.h>
#include <linux/swap.h>
#include <linux/swapctl.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <asm/atomic.h>

#define __KERNEL_SYSCALLS__
#include <linux/unistd.h>

#include "nwfs.h"
#include "nwhash.h"
#include "nwstruct.h"
#include "nwerror.h"
#include "nwdir.h"
#include "nwproc.h"

#include "nwvphal.h"
#include "nwvp.h"

#include "nwvfs.h"

#endif

//
//
//

#if (LINUX_UTIL)

#if (M2FS)
#define CONFIG_NAME        "  M2FS Volume Manager for Linux"
#define IMAGER_NAME        "  M2FS Volume IMAGER for Linux"
#else
#define CONFIG_NAME        "  NWFS Volume Manager for Linux"
#define IMAGER_NAME        "  NWFS Volume IMAGER for Linux"
#endif

#define COPYRIGHT_NOTICE1  "  Copyright (c) 1997-2001 Jeff V. Merkey  All Rights Reserved."
#define COPYRIGHT_NOTICE2  "  "

#define TargetOS "Linux"

extern int log_printf(char *format, ...);
extern int log_sprintf(char *buffer, char *format, ...);

#define NWFSPrint log_printf

#define VCS_RESERVED   4

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/uio.h>

#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "fcntl.h"
#include "ctype.h"
#include "time.h"
#include "string.h"

#include "ncurses.h"
#include "termios.h"

#ifndef LONGLONG
typedef long long LONGLONG;
#endif

typedef unsigned long  ULONG;
typedef unsigned short WORD;
typedef unsigned char  BYTE;

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/errno.h>

#include "nwfs.h"
#include "nwhash.h"
#include "nwstruct.h"
#include "nwerror.h"
#include "nwdir.h"
#include "nwproc.h"

#include "nwvphal.h"
#include "nwvp.h"

#include "nwmenu.h"

#endif

#endif     //  _NWFS_GLOBALS_
