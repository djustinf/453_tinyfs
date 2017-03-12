#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "tinyFS_errno.h"
#include "libDisk.h"
#include "tinyFS.h"
#include "libDisk.h"

char *openFilesTable[9];
int *openFilesLocation;
char *mountedDisk = NULL;

int tfs_mkfs(char *filename, int nBytes) {
	tfs_block buf;
	fileDescriptor fd;
	int traversed = 0;
	if ((fd = openDisk(filename, nBytes)) >= 0) {
		while (nBytes % BLOCKSIZE != 0) {
			nBytes--;
		}
	    /* init and write superblock */
	    initSuperblock(&buf, 1, nBytes);
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

void initFreeblock(tfs_block *buf, unsigned char nextFree) {
    int i;
    buf->mem[0] = 4;
    buf->mem[1] = MAGIC_NUM;
    buf->mem[2] = nextFree;
	for (i = 3; i < BLOCKSIZE; i++) {
		buf->mem[i] = 0x00;
	}
}

void initSuperblock(tfs_block *buf, unsigned char firstFree, int nBytes) {
    int i;
    unsigned char blocks;
    blocks = (unsigned char) (nBytes / BLOCKSIZE);
    buf->mem[0] = 1;
    buf->mem[1] = MAGIC_NUM;
    buf->mem[2] = firstFree;
    buf->mem[3] = '\0';
    buf->mem[4] = blocks;
	for (i = 5; i < BLOCKSIZE; i++) {
		buf->mem[i] = 0x00;
	}
}

int tfs_mount(char *diskname) {
	char buf[BLOCKSIZE];
	int diskNum, i;
	
	// TFS is already mounted.
	if (mountedDisk) {
		perror("mount: TFS already mounted");
		return ERR_TFS_MOUNT;
	}
	
	// Not mounted, so mount it and verify the TFS type.
	else {
		// Open the disk.
		diskNum = openDisk(filename, 0);
		
		// Read the superblock.
		readBlock(diskNum, 0, buf);
		
		// Check that the block is valid.
		if (buf[1] != 0x44) {
			perror("mount: TFS is invalid");
			return ERR_INVALID_TFS;
		}
	}
	mountedDisk = filename;
	openFilesTable = (char*) malloc(sizeof(char*) * (unsigned char) buf[4]);
	openFilesLocation = (int) malloc(sizeof(int) * (unsigned char) buf[4])
	for (i = 0; i < (unsigned char) buf[4]; i++) {
	    openFilesTable[i][0] = '\0';
	    openFilesLocation[i] = 0;
	}
	return SUCCESS;
}

int tfs_unmount(void) {
	// TFS is already unmounted, so throw error.
	if (!mountedDisk) {
		perror("unmount: TFS already unmounted");
		return ERR_TFS_UNMOUNT;
		
	}
	// TFS is mounted, so unmount it.
	else {
		mountedDisk = NULL;
		free(openFilesTable);
		free(openFilesLocation);
	}
	
	return SUCCESS;
}

fileDescriptor tfs_openFile(char *name) {
	fileDescriptor fd;
	int fileExists;
	char buf[BLOCKSIZE];
	int diskNum;
	
	// Make sure we have a valid name length.
	if (strlen(name) > MAX_FILE_NAME_LENGTH) {
		perror("openFile: File name too long");
		return ERR_FILE_NAME_LENGTH;
	}
	
	// If we have a mounted disk, check if the file is already open.
	if (mountedDisk) {
		// TODO: Check if the file is already open. If it is, return a FD to it.
		// If file open.
		// Else open the disk.
	}
	
	// Disk is not already mounted, so we can't open the file.
	else {
		perror("openFile: TFS not mounted");
		return ERR_TFS_NOT_MOUNTED;
	}
	
	// Loop through the inodes and see if we have one that matches the specified name.
	for (int i = 0; i < DEFAULT_DISK_SIZE/BLOCKSIZE && !fileExists; i++) {
		// Read the block.
		readBlock(diskNum, i, buf);
		
		// Check if this block is an inode.
		if (buf[0] == 2) {
			
			// Now check if the names equal.
			if(!strcmp(name, buff+4)) {
				fileExists = 1;
				// TODO: Get a FD to this file.
				break;
			}
		}
	}
	
	// Existing file wasn't found, so we need to create one.
	if (!fileExists) {
		// TODO: Implement inode init, giving it location of next free block.
		for (int i = 0; i < DEFAULT_DISK_SIZE/BLOCKSIZE; i++) {
			// Read the block.
			readBlock(diskNum, i, buf);
			
			// Check if this block is free.
			if (buf[0] == 4) {
				break;
			}
		}
		
		// Buf now is the next free block, so let's initialize it.
		buf[0] = 2;
		
		// TODO: First file extent?
		//buf[2] = ?;
		
		// Next inode is null.
		buf[3] = '\0';
		
		// Write the name.
		memcpy(buf+4, name, strlen(name));
		
		// TODO: Write creation date, last modified date, and last accessed date.
		
		// Now write the new inode back.
		writeBlock(diskNum, i, buf);
	}
	
	// The file exists, we just need to open it.
	else {
		openFilesTable[fd] = 1;
	}
	
	return fd;
}

int tfs_closeFile(fileDescriptor FD) {
	// We need to have an array to hold all open files. In this function, we'll simply need to mark the specified file as no longer open.
	// e.g. a 1 indicates that the file is open, and a 0 indicates the file is closed.
	
	// File is open, so close it.
	if (openFilesTable[FD]) {
		openFilesTable[FD] = 0;
		return SUCCESS;
	}
	// Not open, so we can't close it.
	else {
		return ERR_FILE_CLOSE;
	}
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
}