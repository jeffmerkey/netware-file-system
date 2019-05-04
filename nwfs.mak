
#
# Makefile for the FENRIS IFS module For Linux.
#


CC = gcc
LINK = ld
CFLAGS = -DMODULE -D__KERNEL__ -Wall -Wstrict-prototypes -O2 \
	 -fomit-frame-pointer -fno-strength-reduce -pipe -m386 -DCPU=386 \
	 -I/usr/src/linux/include -c

LFLAGS = -m elf_i386 -Map nwfs.map -r -o 

incdeps = nwfs.h nwstruct.h nwdir.h nwhash.h nwproc.h nwvfs.h nwerror.h \
	  globals.h

objects = nwvfs.o alloc.o disk.o nwpart.o volume.o globals.o lru.o  \
	  lock.o hash.o async.o fat.o bit.o block.o cluster.o dir.o      \
	  mmap.o inode.o super.o file.o ioctl.o nwdir.o nwfile.o nwfix.o  \
	  trustee.o nwext.o suballoc.o create.o date.o                    \
	  nwcreate.o symlink.o nwvp.o nwvphal.o 

nwfs.o: $(objects) 
	$(LINK) $(LFLAGS) nwfs.o $(objects)

nwvfs.o: nwvfs.c     $(incdeps)
	$(CC) $(CFLAGS) nwvfs.c
nwvp.o: nwvp.c       $(incdeps)
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
async.o: async.c   $(incdeps)
	$(CC) $(CFLAGS) async.c
nwfix.o: nwfix.c   $(incdeps)
	$(CC) $(CFLAGS) nwfix.c

clean:
	rm -f *.o

