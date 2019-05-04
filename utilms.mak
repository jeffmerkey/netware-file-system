
#
# Makefile for the FENRIS Utility Makefile For Windows NT.
#

!include <win32.mak>

CC = $(cc)
LINK = $(link)
CFLAGS = $(cdebug) $(cflags) $(cvars) /Zp1
LFLAGS = $(linkdebug)

#CFLAGS = $(cflags) $(cvars) /Zp1
#LFLAGS =

incdeps = nwfs.h nwstruct.h nwdir.h nwhash.h nwproc.h nwvfs.h nwerror.h \
	  globals.h nwmenu.h

objects = nwvfs.obj alloc.obj disk.obj nwpart.obj volume.obj globals.obj \
	  lru.obj lock.obj hash.obj fat.obj bit.obj \
	  block.obj cluster.obj dir.obj mmap.obj inode.obj super.obj file.obj \
	  ioctl.obj nwdir.obj nwfile.obj trustee.obj nwext.obj suballoc.obj \
	  create.obj date.obj nwcreate.obj symlink.obj nwvp.obj nwfix.obj \
	  nwvphal.obj fileutil.obj console.obj nwmenu.obj async.obj \
          nwtool.obj

all: nwvol.exe nwdisk.exe nwvp.exe nwdump.exe \
     nwview.exe nwconfig.exe nwrepair.exe nwmount.exe \
     nwdismnt.exe nwfs.lib

nwfs.lib: $(objects)
	lib $(objects) /out:nwfs.lib

nwmount.exe: nwmount.obj
	$(LINK) $(linkdebug) nwmount.obj -out:nwmount.exe $(MAPFILE)

nwdismnt.exe: nwdismnt.obj
	$(LINK) $(linkdebug) nwdismnt.obj -out:nwdismnt.exe $(MAPFILE)

nwconfig.exe: $(objects) nwconfig.obj filter.obj
	$(LINK) $(linkdebug) nwconfig.obj filter.obj \
	$(objects) user32.lib -out:nwconfig.exe $(MAPFILE)

nwrepair.exe:  $(objects) nwrepair.obj nofilt.obj
	$(LINK) $(linkdebug) nwrepair.obj nofilt.obj \
	$(objects) -out:nwrepair.exe $(MAPFILE)

nwvol.exe:  $(objects) nwvol.obj nofilt.obj
	$(LINK) $(linkdebug) nwvol.obj nofilt.obj \
	$(objects) -out:nwvol.exe $(MAPFILE)

nwdisk.exe:  $(objects) nwdisk.obj nofilt.obj
	$(LINK) $(linkdebug) nwdisk.obj nofilt.obj \
	$(objects) -out:nwdisk.exe $(MAPFILE)

nwvp.exe: $(objects) nwvpmain.obj vcommand.obj vconsole.obj nofilt.obj
	$(LINK) $(linkdebug) nwvpmain.obj vcommand.obj vconsole.obj \
	$(objects) nofilt.obj -out:nwvp.exe $(MAPFILE)

nwdump.exe:  $(objects) nwdump.obj nofilt.obj
	$(LINK) $(linkdebug) nwdump.obj nofilt.obj \
	$(objects) -out:nwdump.exe $(MAPFILE)

nwview.exe:  $(objects) nwview.obj nofilt.obj
	$(LINK) $(linkdebug) nwview.obj nofilt.obj \
	$(objects) -out:nwview.exe $(MAPFILE)

#
#  NWFS Utility Files
#

nwconfig.obj: nwconfig.c $(incdeps) nwhelp.h
	$(CC) $(CFLAGS) nwconfig.c

nwdisk.obj: nwdisk.c $(incdeps)
	$(CC) $(CFLAGS) nwdisk.c

nwrepair.obj: nwrepair.c $(incdeps)
	$(CC) $(CFLAGS) nwrepair.c

nwvol.obj: nwvol.c $(incdeps)
	$(CC) $(CFLAGS) nwvol.c

nwdump.obj: nwdump.c $(incdeps)
	$(CC) $(CFLAGS) nwdump.c

nwview.obj: nwview.c $(incdeps)
	$(CC) $(CFLAGS) nwview.c

vcommand.obj: vcommand.c $(incdeps)
	$(CC) $(CFLAGS) vcommand.c
vconsole.obj: vconsole.c $(incdeps)
	$(CC) $(CFLAGS) vconsole.c
nwvpmain.obj: nwvpmain.c $(incdeps)
	$(CC) $(CFLAGS) nwvpmain.c

nwformat.obj: nwformat.c $(incdeps)
	$(CC) $(CFLAGS) nwformat.c
nwvp.obj:	nwvp.c    $(incdeps)
	$(CC) $(CFLAGS) nwvp.c
nwvphal.obj: nwvphal.c $(incdeps)
	$(CC) $(CFLAGS) nwvphal.c

#
#  NWFS Library Files
#

nwvfs.obj: nwvfs.c     $(incdeps)
	$(CC) $(CFLAGS) nwvfs.c
alloc.obj: alloc.c     $(incdeps)
	$(CC) $(CFLAGS) alloc.c
disk.obj: disk.c       $(incdeps)
	$(CC) $(CFLAGS) disk.c
nwpart.obj: nwpart.c   $(incdeps)
	$(CC) $(CFLAGS) nwpart.c
volume.obj: volume.c   $(incdeps)
	$(CC) $(CFLAGS) volume.c
fat.obj: fat.c         $(incdeps)
	$(CC) $(CFLAGS) fat.c
globals.obj: globals.c $(incdeps)
	$(CC) $(CFLAGS) globals.c
lock.obj: lock.c       $(incdeps)
	$(CC) $(CFLAGS) lock.c
hash.obj: hash.c       $(incdeps)
	$(CC) $(CFLAGS) hash.c
string.obj: string.c   $(incdeps)
	$(CC) $(CFLAGS) string.c
lru.obj: lru.c         $(incdeps)
	$(CC) $(CFLAGS) lru.c
bit.obj: bit.c         $(incdeps)
	$(CC) $(CFLAGS) bit.c
block.obj: block.c     $(incdeps)
	$(CC) $(CFLAGS) block.c
cluster.obj: cluster.c $(incdeps)
	$(CC) $(CFLAGS) cluster.c
dir.obj: dir.c         $(incdeps)
	$(CC) $(CFLAGS) dir.c
mmap.obj: mmap.c       $(incdeps)
	$(CC) $(CFLAGS) mmap.c
inode.obj: inode.c     $(incdeps)
	$(CC) $(CFLAGS) inode.c
super.obj: super.c     $(incdeps)
	$(CC) $(CFLAGS) super.c
file.obj: file.c       $(incdeps)
	$(CC) $(CFLAGS) file.c
ioctl.obj: ioctl.c     $(incdeps)
	$(CC) $(CFLAGS) ioctl.c
nwdir.obj: nwdir.c     $(incdeps)
	$(CC) $(CFLAGS) nwdir.c
nwfile.obj: nwfile.c   $(incdeps)
	$(CC) $(CFLAGS) nwfile.c
trustee.obj: trustee.c   $(incdeps)
	$(CC) $(CFLAGS) trustee.c
nwext.obj: nwext.c   $(incdeps)
	$(CC) $(CFLAGS) nwext.c
suballoc.obj: suballoc.c   $(incdeps)
	$(CC) $(CFLAGS) suballoc.c
create.obj: create.c   $(incdeps)
	$(CC) $(CFLAGS) create.c
date.obj: date.c       $(incdeps)
	$(CC) $(CFLAGS) date.c
nwcreate.obj: nwcreate.c   $(incdeps)
	$(CC) $(CFLAGS) nwcreate.c
symlink.obj: symlink.c   $(incdeps)
	$(CC) $(CFLAGS) symlink.c
fileutil.obj: fileutil.c   $(incdeps)
	$(CC) $(CFLAGS) fileutil.c
console.obj: console.c   $(incdeps)
	$(CC) $(CFLAGS) console.c
nwmenu.obj: nwmenu.c   $(incdeps)
	$(CC) $(CFLAGS) nwmenu.c
filter.obj: filter.c   $(incdeps)
	$(CC) $(CFLAGS) filter.c
nofilt.obj: nofilt.c   $(incdeps)
	$(CC) $(CFLAGS) nofilt.c
async.obj: async.c   $(incdeps)
	$(CC) $(CFLAGS) async.c
nwtool.obj: nwtool.c   $(incdeps)
	$(CC) $(CFLAGS) nwtool.c
nwfix.obj: nwfix.c   $(incdeps)
	$(CC) $(CFLAGS) nwfix.c
nwmount.obj: nwmount.c   $(incdeps)
	$(CC) $(CFLAGS) nwmount.c
nwdismnt.obj: nwdismnt.c   $(incdeps)
	$(CC) $(CFLAGS) nwdismnt.c

clean:
    del *.obj
    del *.exe
    del *.ilk
    del *.pdb
    del *.opt
    del *.lib

