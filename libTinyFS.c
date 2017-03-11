#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "tinyFS_errno.h"
#include "tinyFS.h"

int tfs_mkfs(char *filename, int nBytes) {
	tfs_block buf;
	fileDescriptor fd;
	int traversed = 0;
	if (nBytes < BLOCKSIZE) {
	    return BLOCKSIZE_FAILURE;
	}
	else if ((fd = open(filename, 0)) >= 0) {
		while (nBytes % BLOCKSIZE != 0) {
			nBytes--;
		}
	    /* init and write superblock */
	    initSuperblock(&buf, 1);
		if (write(fd, buf.mem, 1) < BLOCKSIZE) {
			return MKFS_FAILURE;
		}
        while ((++traversed * BLOCKSIZE) < nBytes) {
		    /* init and write all free */
		    if ((traversed + 1) * BLOCKSIZE >= nBytes)
		        initFreeblock(&buf, '\0');
		    else
		        initFreeblock(&buf, traversed);
		    if (write(fd, buf.mem, 1) < BLOCKSIZE) {
			    return MKFS_FAILURE;
		    }
        }
	}
	else {
	    return MKFS_FAILURE;
	}
	return SUCCESS;
}

void initFreeblock(tfs_block *buf, char nextFree) {
    int i;
    buf->mem[0] = 4;
    buf->mem[1] = MAGIC_NUM;
    buf->mem[2] = nextFree;
	for (i = 3; i < BLOCKSIZE; i++) {
		buf->mem[i] = 0x00;
	}
}

void initSuperblock(tfs_block *buf, char firstFree) {
    int i;
    buf->mem[0] = 1;
    buf->mem[1] = MAGIC_NUM;
    buf->mem[2] = firstFree;
    buf->mem[3] = '\0';
	for (i = 4; i < BLOCKSIZE; i++) {
		buf->mem[i] = 0x00;
	}
}

int tfs_mount(char *diskname) {
	return 0;
}

int tfs_unmount(void) {
	return 0;
}

fileDescriptor tfs_openFile(char *name) {
	return 0;
}

int tfs_closeFile(fileDescriptor FD) {
	return 0;
}

int tfs_writeFile(fileDescriptor FD,char *buffer, int size) {
	return 0;
}

int tfs_deleteFile(fileDescriptor FD) {
	return 0;
}

int tfs_readByte(fileDescriptor FD, char *buffer) {
	return 0;
}

int tfs_seek(fileDescriptor FD, int offset) {
	return 0;
}