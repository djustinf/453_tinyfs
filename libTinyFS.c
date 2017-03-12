#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "tinyFS_errno.h"
#include "libTinyFS.h"
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

void initInodeblock(tfs_block *buf, char* name) {
	int i;
	
	// Set block type.
	buf->mem[0] = 2;

	// Next inode is null since this is the newest inode.
	buf->mem[3] = '\0';
	
	// Write the name.
	for (i = 5; i < 5 + strlen(name); i++) {
			buf->mem[i] = *name;
			name++;
	}

	// TODO: Write creation date, last modified date, and last accessed date.
	// When we first create the file, all of these will be equal.
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
		diskNum = openDisk(diskname, 0);
		
		// Read the superblock.
		readBlock(diskNum, 0, buf);
		
		// Check that the block is valid.
		if (buf[1] != 0x44) {
			perror("mount: TFS is invalid");
			return ERR_INVALID_TFS;
		}
	}
	mountedDisk = diskname;
	openFilesTable = (char**) malloc(sizeof(char*) * (unsigned char) buf[4]);
	openFilesLocation = (int*) malloc(sizeof(int) * (unsigned char) buf[4]);
	for (i = 0; i < (unsigned char) buf[4]; i++) {
	    openFilesTable[i][0] = '\0';
	    openFilesLocation[i] = 0;
	}
	return SUCCESS;
}

int checkMountAndFile(fileDescriptor FD)
{
   if (!mountedDisk)
      return ERR_TFS_NOT_MOUNTED;

   if(openFilesTable[FD] == '\0')
      return ERR_INVALID_TFS;

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
	tfs_block buf;
	int diskNum;
	unsigned char numFiles;
	char tempName[9];
	unsigned char firstFree;
	
	// Make sure we have a valid name length.
	if ((strlen(name) > MAX_FILE_NAME_LENGTH) || !strlen(name)) {
		perror("openFile: Invalid file name length");
		return ERR_FILE_NAME_LENGTH;
	}
	
	// If we have a mounted disk, check if the file is already open.
	if (mountedDisk) {
		// Open the disk.
		diskNum = openDisk(mountedDisk, 0);
		
		// Read the superblock from the disk. 
		// TODO: Pointer issue?
		readBlock(diskNum, 0, &(buf.mem));
		
		// Get the number of blocks from the superblock.
		numFiles = buf.mem[4] - 1;
		
		// Get the address of the first free block.
		firstFree = buf.mem[2];
		
		// Use that as the upper bound for the open files table so we can iterate through it.
		// Iterate through the table, and check if we find an entry that equals our name. 
		for (int i = 0; i < numFiles; i++) {
			strcpy(&tempName, openFilesTable[i]);
			
			// If we find one, return the index of that as the FD.
			if (!strcmp(tempName, name)) {
				return i;
			}	
		}
	}
	
	// Disk is not already mounted, so we can't open the file.
	else {
		perror("openFile: TFS not mounted");
		return ERR_TFS_NOT_MOUNTED;
	}

	// Loop through the inodes and see if we have one that matches the specified name.
	for (int i = 1; i <= numFiles && !fileExists; i++) {
		// Read the block.
		readBlock(diskNum, i, &(buf.mem));
		
		// Check if this block is an inode.
		if (buf.mem[0] == 2) {
			
			// Now check if the names equal.
			// TODO: Pointer issue?
			if(!strcmp(name, buf.mem+4)) {
				fileExists = 1;
				fd = i;
				break;
			}
		}
	}
	
	// Existing file wasn't found, so we need to create one.
	if (!fileExists) {
		
		// Read in the first free block.
		// TODO: Pointer issue?
		readBlock(diskNum, firstFree, &(buf.mem));
		
		// Init the inode block at that free block.
		// TODO: Pointer issue?
		initInodeblock(&buf, name);
		
		// Now write the new inode back.
		// Pointer issue?
		writeBlock(diskNum, fd, &(buf.mem));
	}
	
	// The file exists, we just need to open it.
	else {
		strcpy(openFilesTable[fd], name);
		
		// Reset the file descriptor location.
		openFilesLocation[fd] = 0;
		
		// TODO: Set last accessed time.
	}
	
	return fd;
}

int tfs_closeFile(fileDescriptor FD) {
	
	// File is open, so close it.
	if (openFilesTable[FD] != '\0') {
		openFilesTable[FD] = "\0";
		return SUCCESS;
	}
	// Not open, so we can't close it.
	else {
		return ERR_FILE_CLOSE;
	}
}

int tfs_writeFile(fileDescriptor FD,char *buffer, int size) {

    int data[BLOCK_SIZE], ret;
   tfs_block block;

   //check if file is mounted and that file exists
   if( (ret = checkMountAndFile) < 0)
      return ret;

   //get the data at FD

   //check read-only
   if ((curIdx = getCurIdx(FD)) < 0)
      return ERR_WRITE;

   //write the data

   //increment cursor

   //update inode

   if (writeBlock(FD, tinyFS.inode[],block))
   {
      return ERR_WRITE;
   }
   tinyFS.curIdx++;
   *buffer = data;

	return SUCCESS;
}

int tfs_deleteFile(fileDescriptor FD) {
	return 0;
}

int tfs_readByte(fileDescriptor FD, char *buffer) {
    int ret, idx;
   tfs_block inode, fileEx;
   
   //check if file is mounted and that file exists
   if((ret = checkMountAndFile(FD)) < 0)
      return ret;

   //get the current location of the file pouinter
   idx = openFilesLocation[FD];

   //read from inode to get the first ref to file extent
   if (readBlock(FD, 0, inode) < 0)
      return ERR_READ;
   
   //get the location of the file extent

   if (readBlock(FD, inode[2], fileEx) < 0)
   {
      return ERR_READ;
   }
 
   
   while ((idx > 252) && (fileEx.mem[2] != '\0'))
   {
      ret = fileEx[2];

      if (readBlock(FD, retm fileEx) < 0)
      {
         return ERR_READ;
      }
      
      idx -= 252;
   }
   //check if EOF
   if (idx <= 252)
   {
      return ERR_INVALID_TFS;      
      return ERR_WRITE;
   }

   //get the reference to the first inode
    memcpy(buffer, fileEx + 4 + idx, sizeof(char));

   //update inode
   openFilesLocation[FD]++;
   
	return SUCCESS;
}

int tfs_seek(fileDescriptor FD, int offset) {
	// Check if FD is in list of open files. 
	// If it isn't, we can't seek so return error.
	
	// Check if offset puts you past last file extent.
	
	// If neither of these fails, set the FD to the offset.
	openFilesLocation[FD] = offset;
	
	return SUCCESS;
}
