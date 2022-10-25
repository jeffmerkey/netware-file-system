
CC = gcc
LINK = ld
CFLAGS = -Wall -Wstrict-prototypes -c -g

LFLAGS = -Map nwfs.map -r -o

incdeps = nwfs.h nwstruct.h nwdir.h nwhash.h nwproc.h nwvfs.h nwerror.h \
	  globals.h nwmenu.h

objects = nwvfs.o alloc.o disk.o nwpart.o volume.o globals.o lru.o  \
	  lock.o hash.o fat.o bit.o block.o cluster.o dir.o         \
	  mmap.o inode.o super.o file.o ioctl.o nwdir.o nwfile.o    \
	  trustee.o nwext.o suballoc.o create.o date.o nwfix.o      \
	  nwcreate.o symlink.o nwvp.o nwvphal.o fileutil.o     \
	  console.o async.o nwmenu.o nwtool.o

all: nwfs.o nwvol.exe nwdisk.exe nwvp.exe \
     nwdump.exe nwview.exe nwconfig.exe nwrepair.exe

nwconfig.exe: $(objects) nwfs.o nwconfig.o filter.o
	$(CC) nwconfig.o filter.o nwfs.o -o nwconfig.exe

nwrepair.exe:  $(objects) nwfs.o nwrepair.o nofilt.o
	$(CC) nwrepair.o nofilt.o nwfs.o -o nwrepair.exe

nwvol.exe:  $(objects) nwfs.o nwvol.o nofilt.o
	$(CC) nwvol.o nofilt.o nwfs.o -o nwvol.exe

nwdisk.exe:  $(objects) nwfs.o nwdisk.o nofilt.o
	$(CC) nwdisk.o nofilt.o nwfs.o -o nwdisk.exe

nwvp.exe: $(objects) nwvpmain.o vcommand.o vconsole.o nwfs.o nofilt.o
	$(CC) nwvpmain.o vcommand.o vconsole.o nofilt.o nwfs.o -o nwvp.exe

nwdump.exe:  $(objects) nwfs.o nwdump.o nofilt.o
	$(CC) nwdump.o nofilt.o nwfs.o -o nwdump.exe

nwview.exe:  $(objects) nwfs.o nwview.o nofilt.o
	$(CC) nwview.o nofilt.o nwfs.o -o nwview.exe

#
#
#

nwfs.o: $(objects)
	$(LINK) $(LFLAGS) nwfs.o $(objects)

nwconfig.o : nwconfig.c $(incdeps) nwhelp.h
	$(CC) $(CFLAGS) nwconfig.c

nwimage.o : nwimage.c $(incdeps) nwconfig.c nwhelp.h
	$(CC) $(CFLAGS) nwimage.c

nwbackup.o : nwbackup.c $(incdeps)
	$(CC) $(CFLAGS) nwbackup.c

nwrestor.o : nwrestor.c $(incdeps)
	$(CC) $(CFLAGS) nwrestor.c

nwrepair.o : nwrepair.c $(incdeps)
	$(CC) $(CFLAGS) nwrepair.c

nwvol.o : nwvol.c $(incdeps)
	$(CC) $(CFLAGS) nwvol.c

nwdisk.o : nwdisk.c $(incdeps)
	$(CC) $(CFLAGS) nwdisk.c

nwdump.o : nwdump.c $(incdeps)
	$(CC) $(CFLAGS) nwdump.c

nwview.o : nwview.c $(incdeps)
	$(CC) $(CFLAGS) nwview.c

vcommand.o: vcommand.c $(incdeps)
	$(CC) $(CFLAGS) vcommand.c
vconsole.o: vconsole.c $(incdeps)
	$(CC) $(CFLAGS) vconsole.c
nwvpmain.o: nwvpmain.c $(incdeps)
	$(CC) $(CFLAGS) nwvpmain.c

#
#
#

nwvfs.o: nwvfs.c     $(incdeps)
	$(CC) $(CFLAGS) nwvfs.c
nwformat.o: nwformat.c $(incdeps)
	$(CC) $(CFLAGS) nwformat.c
nwvp.o:	nwvp.c    $(incdeps)
	$(CC) $(CFLAGS) nwvp.c
nwvphal.o: nwvphal.c $(incdeps)
	$(CC) $(CFLAGS) nwvphal.c
alloc.o: alloc.c     $(incdeps)
	$(CC) $(CFLAGS) alloc.c
disk.o: disk.c       $(incdeps)
	$(CC) $(CFLAGS) disk.c
nwpart.o: nwpart.c   $(incdeps)
	$(CC) $(CFLAGS) nwpart.c
volume.o: volume.c   $(incdeps)
	$(CC) $(CFLAGS) volume.c
fat.o: fat.c         $(incdeps)
	$(CC) $(CFLAGS) fat.c
globals.o: globals.c $(incdeps)
	$(CC) $(CFLAGS) globals.c
lock.o: lock.c       $(incdeps)
	$(CC) $(CFLAGS) lock.c
hash.o: hash.c       $(incdeps)
	$(CC) $(CFLAGS) hash.c
string.o: string.c   $(incdeps)
	$(CC) $(CFLAGS) string.c
lru.o: lru.c         $(incdeps)
	$(CC) $(CFLAGS) lru.c
bit.o: bit.c         $(incdeps)
	$(CC) $(CFLAGS) bit.c
block.o: block.c     $(incdeps)
	$(CC) $(CFLAGS) block.c
cluster.o: cluster.c $(incdeps)
	$(CC) $(CFLAGS) cluster.c
dir.o: dir.c         $(incdeps)
	$(CC) $(CFLAGS) dir.c
mmap.o: mmap.c       $(incdeps)
	$(CC) $(CFLAGS) mmap.c
inode.o: inode.c     $(incdeps)
	$(CC) $(CFLAGS) inode.c
super.o: super.c     $(incdeps)
	$(CC) $(CFLAGS) super.c
file.o: file.c       $(incdeps)
	$(CC) $(CFLAGS) file.c
ioctl.o: ioctl.c     $(incdeps)
	$(CC) $(CFLAGS) ioctl.c
nwdir.o: nwdir.c     $(incdeps)
	$(CC) $(CFLAGS) nwdir.c
nwfile.o: nwfile.c   $(incdeps)
	$(CC) $(CFLAGS) nwfile.c
trustee.o: trustee.c   $(incdeps)
	$(CC) $(CFLAGS) trustee.c
nwext.o: nwext.c   $(incdeps)
	$(CC) $(CFLAGS) nwext.c
suballoc.o: suballoc.c   $(incdeps)
	$(CC) $(CFLAGS) suballoc.c
create.o: create.c   $(incdeps)
	$(CC) $(CFLAGS) create.c
date.o: date.c       $(incdeps)
	$(CC) $(CFLAGS) date.c
nwcreate.o: nwcreate.c   $(incdeps)
	$(CC) $(CFLAGS) nwcreate.c
symlink.o: symlink.c   $(incdeps)
	$(CC) $(CFLAGS) symlink.c
fileutil.o: fileutil.c   $(incdeps)
	$(CC) $(CFLAGS) fileutil.c
console.o: console.c   $(incdeps)
	$(CC) $(CFLAGS) console.c
nwmenu.o: nwmenu.c   $(incdeps)
	$(CC) $(CFLAGS) nwmenu.c
filter.o: filter.c   $(incdeps)
	$(CC) $(CFLAGS) filter.c
nofilt.o: nofilt.c   $(incdeps)
	$(CC) $(CFLAGS) nofilt.c
async.o: async.c   $(incdeps)
	$(CC) $(CFLAGS) async.c
nwtool.o: nwtool.c   $(incdeps)
	$(CC) $(CFLAGS) nwtool.c
nwfix.o: nwfix.c   $(incdeps)
	$(CC) $(CFLAGS) nwfix.c

clean:
	del *.o
	del *.exe

