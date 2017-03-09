#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "libDisk.h"
#include "tinyFS_errno.h"
#include "tinyFS.h"

int openDisk(char *filename, int nBytes) {
	char *buf;
	int i;
	fileDescriptor fd;
	if (nBytes == 0) {
		return open(filename, 0);
	}
	else if (nBytes < BLOCKSIZE) {
		errno = BLOCKSIZE_FAILURE;
		return -1;
	}
	else {
		if ((fd = open(filename, O_CREAT|O_WRONLY, 0777)) >= 0) {
			while (nBytes % BLOCKSIZE != 0) {
				nBytes--;
			}
			buf = (char*) malloc(nBytes * sizeof(char));
			for (i = 0; i < nBytes; i++) {
				buf[i] = '\0';
			}
			if (write(fd, buf, nBytes) < nBytes) {
				free(buf);
				errno = INIT_FILE_FAILURE;
				return -1;
			}
			free(buf);
		}
	}
	return fd;
}

int readBlock(int disk, int bNum, void *block) {
	UINT byteOffset = bNum * BLOCKSIZE;
	
	// Seek to the specified offset on the disk, check for error.
	if (lseek(disk, byteOffset, SEEK_SET) == -1) {
		perror("readBlock: Seek error");
		return ERR_SEEK;
	}
	
	// Now try to read from the disk and copy to the local buffer, check for error.
	if (read(disk, block, BLOCKSIZE) < -1) {
		perror("readBlock: Read error");
		return ERR_READ;
	}
	
	// If we reach here, we didn't have any errors so we return 0.
	return 0;
}

int writeBlock(int disk, int bNum, void *block) {
   int ret = 0;
   UINT offset = bNum * BLOCKSIZE;
   
   //Seek to specified offset, check for error
   if (lseek(disk, offset, SEEK_SET) == -1)
   {
      perror("writeBlock: seek error");
      return ERR_SEEK;
   }
   //Write to file, if success set ret to 0
   if ((write(disk, block, BLOCKSIZE)) <= 0)
   {
      perror("writeBlock: Write error");
      ret  = ERR_WRITE;
   }
	/*
	I don't think we need to do this... -Geoff
   //reset file to point to the beginning
   if (lseek(disk, 0, SEEK_SET) < 0)
   {
      perror("writeBlock: lseek:");
      ret = ERR_SEEK;
   }
	*/
	return ret;
}