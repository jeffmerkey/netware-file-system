
# netware-file-system 

NetWare SMP File System for Linux, Windows, and DOS

## Table of Contents 
- [Using the Build Script](using-the-build-script)
- [Building with Manual Make Files](building-with-manual-make-files)
- [Windows File System Driver Components](windows-file-system-driver-components)
- [Linux File System Driver Components](linux-file-system-driver-components)
- [MS-DOS and DR-DOS File System Components](dos-file-system-components)

## Using the Build Script

```sh
#
# ./build <option>
#
```

from the directory where you install the NWFS sources.  This 
newer model auto detects modversioning, smp, etc.  There is 
an assumption made that a symbolic link to /usr/src/linux ->

newer Red Hat kernels no longer use this syntax.  For the 
NWFS build to work properly, you will need to have a 
pre-configured Linux source tree present with a symbolic 
link to the /usr/src/linux directory.

Simply type ./build for a list of build options.

 
## Building with Manual Make Files

The globals.h file contains the following table of options:

```sh
#define  WINDOWS_NT_RO    0
#define  WINDOWS_NT       0
#define  WINDOWS_NT_UTIL  0
#define  WINDOWS_CONVERT  0
#define  WINDOWS_98_UTIL  0
#define  LINUX_20         0
#define  LINUX_22         1
#define  LINUX_24         0
#define  LINUX_UTIL       0
#define  DOS_UTIL         0
```

select build environment (you can only select one at a time).  

There are makefiles included for different kernel configurations.  To
make the NWFS driver for Linux, select one of the following.  The
makefiles support modversioned kernels and naked kernels.

```sh
#define  LINUX_20         0
#define  LINUX_22         1
#define  LINUX_24         0
```
- make -f nwfs.mak         This will make an NWFS driver SMP-no  MODVER-no
- make -f nwfsmod.mak      This will make an NWFS driver SMP-no  MODVER-yes
- make -f nwfssmp.mak      This will make an NWFS driver SMP-yes MODVER-no
- make -f nwmodsmp.mak     This will make an NWFS driver SMP-yes MODVER-yes

The Linux versions are supported under the GCC Linux compiler
To make the Linux tools:
```sh
#define  LINUX_UTIL       1
```
make -f util.mak

The DOS versions are supported under the DJ Delorie GCC MS-DOS compiler.
To make the MS-DOS tools:
```sh
#define  DOS_UTIL       1
```
make -f dos.mak

The Windows NT/2000 versions are supported under Microsoft C++ 5.0 or higher
To make the WIndows NT/2000 tools:
```sh
#define  WINDOWS_NT_UTIL       1
```
nmake /f utilms.mak

## Windows File System Driver Components

- NWFSRO.SYS     Windows NT/2000 File System Driver
- NWFSRO.INI     Regini.exe import file
- NWMOUNT.EXE    Volume Mount Utility
- NWDISMNT.EXE   Volume Dismount Utility
- NWVOL.EXE      Volume Display Utility
- NWCONFIG.EXE   Volume Create/Mirroring/Namespace/Partition Manager (CWorthy)
- NWDISK.EXE     Volume Create/Mirroring/Namespace/Partition Manager (terminal)
- NWVIEW.EXE     Volume Directory Viewer
- NWREPAIR.EXE   Volume Repair Utility

To install the Windows NT/2000 version, copy the NWFSRO.SYS driver
to the \%SYSTEM_ROOT%\System32\drivers directory (WINNT is the name of
the %SYSTEM_ROOT% on most systems) and run regini.exe (which comes with
the Windows NT/2000 DDK) to add the registry entries to your system
that will allow the driver to load at the next reboot.

Command syntax for regini.exe is:
```sh
C:\>regini nwfsro.ini <enter>
```
To query available volumes on this machine that can be mounted, invoke
NWVOL.EXE by typing:
```sh
C:\>NWVOL
```
NWVOL will list all volumes detected on this server.
```sh
NetWare Volume(s)
[SYS            ] sz-00003751 blk-65536  F/D-0000/0001  (OK)
 NAMESPACES  [ DOS   LONG  NFS  ]
 COMPRESS-YES SUBALLOC-YES MIGRATE-NO  AUDIT-NO
  segment #0 Start-00000000 size-00001BA9
  segment #1 Start-00001BA9 size-00001BA8
```
To mount a NetWare volume, type:
```sh
C:\>nwmount *
```
This will mount all NetWare volumes on an NT/2000 server as Native Windows
NT File Systems.  You can also mount a specific volume by typing "nwmount
VOLUME_NAME".

Simply type NWMOUNT.EXE or NWDISMNT.EXE with no arguments and these
programs will display additional help information.  

You should copy the utilities into the /WINNT/System32 directory.  If
you want to change any volume configurations, you should only do so
with the driver unloaded and all volumes dismounted.  See NWCONFIG.EXE
for very complete help on how to manage NetWare volumes.


## Linux File System Driver Components)

- NWFS.O         Linux NWFS 2.0 File System Driver
- NWVOL          Volume Display Utility
- NWCONFIG       Volume Create/Mirroring/Namespace/Partition Manager (ncurses)
- NWDISK         Volume Create/Mirroring/Namespace/Partition Manager (terminal)
- NWVIEW         Volume Directory Viewer
- NWREPAIR       Volume Repair Utility

To install the Linux version of NWFS 2.0, copy the NWFS.O driver
to the \lib\modules\misc directory.  Load the driver by typing:
```sh
[root@localhost]#
[root@localhost]# insmod nwfs
[root@localhost]#
```
To query available volumes on this machine that can be mounted, invoke
NWVOL by typing:
```sh
[root@localhost]# NWVOL
```
NWVOL will list all volumes detected on this server.
```sh
NetWare Volume(s)
[SYS            ] sz-00003751 blk-65536  F/D-0000/0001  (OK)
 NAMESPACES  [ DOS   LONG  NFS  ]
 COMPRESS-YES SUBALLOC-YES MIGRATE-NO  AUDIT-NO
  segment #0 Start-00000000 size-00001BA9
  segment #1 Start-00001BA9 size-00001BA8
```

To mount a NetWare Volume named "SYS" to a pre-exisitng mount point
(directory) called /SYS, type:
```sh
[root@localhost]#
[root@localhost]# mount sys /SYS -t nwfs -o SYS
[root@localhost]#
```
To dismount, type:
```sh
[root@localhost]#
[root@localhost]# umount /SYS -t
[root@localhost]#
```
You should copy the utilities into the /usr/bin directory.  If you want to
change any volume configurations, you should only do so with the driver
unloaded and all volumes dismounted.  See NWCONFIG
for very complete help on how to manage NetWare volumes.


## DOS File System Components

- NWVOL.EXE      Volume Display Utility
- NWCONFIG.EXE   Volume Create/Mirroring/Namespace/Partition Manager (CWorthy)
- NWDISK.EXE     Volume Create/Mirroring/Namespace/Partition Manager (terminal)
- NWVIEW.EXE     Volume Directory Viewer
- NWREPAIR.EXE   Volume Repair Utility

You should copy the utilities into the /DOS directory on your machine.
See NWCONFIG.EXE for very complete help on how to manage NetWare volumes
from DOS.

To query available volumes on this machine that can be mounted, invoke
NWVOL.EXE by typing:
```sh
C:\>NWVOL
```
NWVOL will list all volumes detected on this server.
```sh
NetWare Volume(s)
[SYS            ] sz-00003751 blk-65536  F/D-0000/0001  (OK)
 NAMESPACES  [ DOS   LONG  NFS  ]
 COMPRESS-YES SUBALLOC-YES MIGRATE-NO  AUDIT-NO
  segment #0 Start-00000000 size-00001BA9
  segment #1 Start-00001BA9 size-00001BA8
```
