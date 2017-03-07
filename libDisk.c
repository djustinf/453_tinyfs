#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "libDisk.h"
#include "TinyFS_errno.h"

/* This functions opens a regular UNIX file and designates the first
nBytes of it as space for the emulated disk. If nBytes is not exactly
a multiple of BLOCKSIZE then the disk size will be the closest
multiple of BLOCKSIZE that is lower than nByte (but greater than 0)
If nBytes is less than BLOCKSIZE failure should be returned. If
nBytes > BLOCKSIZE and there is already a file by the given filename,
that file’s content may be overwritten. If nBytes is 0, an existing
disk is opened, and should not be overwritten. There is no
requirement to maintain integrity of any file content beyond nBytes.
The return value is -1 on failure or a disk number on success. */
int openDisk(char *filename, int nBytes) {
	return 0;
}

/* readBlock() reads an entire block of BLOCKSIZE bytes from the open
disk (identified by ‘disk’) and copies the result into a local buffer
(must be at least of BLOCKSIZE bytes). The bNum is a logical block
number, which must be translated into a byte offset within the disk.
The translation from logical to physical block is straightforward:
bNum=0 is the very first byte of the file. bNum=1 is BLOCKSIZE bytes
into the disk, bNum=n is n*BLOCKSIZE bytes into the disk. On success,
it returns 0. -1 or smaller is returned if disk is not available
(hasn’t been opened) or any other failures. You must define your own
error code system. */
int readBlock(int disk, int bNum, void *block) {
	unsigned int byteOffset = bNum * BLOCKSIZE;
	int readStatus = 0;
	
	// Seek to the specified offset on the disk. 
	readStatus = lseek(disk, byteOffset, SEEK_SET);
	
	// There was an error moving to the specified offset.
	if (readStatus == -1) {
		printf("Seek error!\n");
		return ERR_SEEK;
	}
	
	// Now try to read from the disk and copy to the local buffer.
	readStatus = read(disk, block, BLOCKSIZE);
	
	// Return value of read was negative, so there was an error.
	if (readStatus < -1) {
		printf("Read error!\n");
		return ERR_READ;
	}
	
	// If we reach here, we didn't have any errors so we return 0.
	return 0;
}

/* writeBlock() takes disk number ‘disk’ and logical block number
‘bNum’ and writes the content of the buffer ‘block’ to that location.
‘block’ must be integral with BLOCKSIZE. Just as in readBlock(),
writeBlock() must translate the logical block bNum to the correct
byte position in the file. On success, it returns 0. -1 or smaller is
returned if disk is not available (i.e. hasn’t been opened) or any
other failures. You must define your own error code system. */
int writeBlock(int disk, int bNum, void *block) {
	return 0;
}