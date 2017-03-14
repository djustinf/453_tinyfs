#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "tinyFS_errno.h"
#include "tinyFS.h"
#include "libTinyFS.h"
#include "libDisk.h"

int diskFD;
char **openFilesTable;
int *openFilesLocation;
int numBlocks;
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
		 //printf("\nMKFS: %c", buf.mem[1]);
		//if (write(fd, buf.mem, 1) < BLOCKSIZE) {
		if (writeBlock(fd, traversed, buf.mem) < 0) {
			return MKFS_FAILURE;
		}
        while ((++traversed * BLOCKSIZE) < nBytes) {
		    /* init and write all free */
		    if ((traversed + 1) * BLOCKSIZE >= nBytes)
		        initFreeblock(&buf, '\0');
		    else
		        initFreeblock(&buf, traversed);
		    //if (write(fd, buf.mem, 1) < BLOCKSIZE) {
			if (writeBlock(fd, traversed, buf.mem) < 0) {
			
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

    numBlocks = blocks;

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

   buf->mem[i] = 0;

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
      diskFD = diskNum;
		
		// Read the superblock.
		readBlock(diskNum, 0, &buf);
		
		//printf("\nDEBUG: %c", buf[1]);
		
		// Check that the block is valid.
		if (buf[1] != 0x44) {
			perror("mount: TFS is invalid");
			return ERR_INVALID_TFS;
		}
	}
	mountedDisk = diskname;
	openFilesTable = (char**) malloc(sizeof(char*) * numBlocks);
	openFilesLocation = (int*) malloc(sizeof(int) * numBlocks);
	for (i = 0; i < numBlocks; i++) {
	    openFilesTable[i] = (char*) malloc(sizeof(char) * 9);
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
    int i;
	// TFS is already unmounted, so throw error.
	if (!mountedDisk) {
		perror("unmount: TFS already unmounted");
		return ERR_TFS_UNMOUNT;
		
	}
	// TFS is mounted, so unmount it.
	else {
		mountedDisk = NULL;
      diskFD = -1;
		for (i = 0; i < numBlocks; i++) {
		    free(openFilesTable[i]);
		}
		free(openFilesTable);
		free(openFilesLocation);
		numBlocks = -1;
	}
	
	return SUCCESS;
}

fileDescriptor tfs_openFile(char *name) {
	fileDescriptor fd;
	int fileExists;
	tfs_block buf, super;
	int diskNum;
	unsigned char numFiles;
	char tempName[9];
	unsigned char firstFree;

   printf("openFile\n");
	
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
		readBlock(diskNum, 0, &(buf.mem));
		
		// Get the number of blocks from the superblock.
		numFiles = numBlocks - 1;
		
		// Get the address of the first free block.
		firstFree = buf.mem[2];
		
		// Use that as the upper bound for the open files table so we can iterate through it.
		// Iterate through the table, and check if we find an entry that equals our name. 
		for (int i = 0; i < numFiles; i++) {
         //printf("%d\n", i);


         if(openFilesTable[i] != NULL)
         {
			   strcpy(tempName, openFilesTable[i]);

			   // If we find one, return the index of that as the FD.
			   if (!strcmp(tempName, name)) {
				   return i;
			   }
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
			if(!strcmp(name, buf.mem+4)) {
				fileExists = 1;
				fd = i;
				break;
			}
		}
	}
	
	// Existing file wasn't found, so we need to create one.
	if (!fileExists) {
      
      readBlock(diskNum, 0, &(super.mem));
		
		// Read in the first free block.
		readBlock(diskNum, firstFree, &(buf.mem));
		
      super.mem[2] = buf.mem[2];

      writeBlock(diskNum, 0, &(super.mem));

		// Init the inode block at that free block.
		initInodeblock(&buf, name);
		
		// Now write the new inode back.
		// Pointer issue?
      fd = firstFree;

		writeBlock(diskNum, fd, &(buf.mem));
      strcpy(openFilesTable[fd], name);
		
		// Reset the file descriptor location.
		openFilesLocation[fd] = 0;
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

int getNumBlocks(int size)
{
   int blocks = size / BLOCKSIZE;

   if (size % BLOCKSIZE)
      blocks++;

   return blocks;
}

int tfs_writeFile(fileDescriptor FD,char *buffer, int size) {
    int ret, idx, freeBlocks = 0, reqBlocks = getNumBlocks(size), offset = 0;
   tfs_block super, inode, fileEx, fBlock;

   printf("tfs_writeFile, diskFD is %d\n", diskFD);

   //check if file is mounted and that the file exists
   if((ret = checkMountAndFile(FD)) < 0)
      return ret;

   //get the super block to get the current number of free blocks
   if(readBlock(diskFD, 0, &(super.mem)) < 0)
      return ERR_READ;

   freeBlocks = super.mem[4];

   //check to see if we have enough space to write the data
   if (numBlocks - freeBlocks  <  reqBlocks)
      return ERR_INVALID_SPACE;

   //read from inode to get the first ref to file extent
   if(readBlock(diskFD, FD, &(inode.mem)) < 0)
   {
      fprintf(stderr, "writeFile inode\n");
      return ERR_READ;
   }

   //get the location of the file extent
   if (readBlock(diskFD, inode.mem[2], &(fileEx.mem)) < 0)
   {
      fprintf(stderr, "writeFile file extent\n");
      return ERR_READ;
   }

   //deallocate data blocks
   while ((fileEx.mem[2] != '\0'))
   {
      //get the address of the next free block
      ret = fileEx.mem[3];

      //get the freeBlock at the address
      if(readBlock(diskFD, ret, &(fBlock.mem)) < 0)
         return ERR_READ;

      //marks this as free
      fBlock.mem[2] = idx;

      if (writeBlock(diskFD, ret, &(fBlock.mem)) < 0)
         return ERR_WRITE;

      super.mem[2] = ret;
      if (writeBlock(diskFD, 0, &(super.mem)) < 0)
         return ERR_WRITE;

      //read the next inode
      if (readBlock(diskFD, fileEx.mem[2], &(fileEx.mem)) < 0)
         return ERR_READ;
      
   }
   //write new data
   for (idx = 0; idx < reqBlocks -1;idx++)
   {
      //update inode
      inode.mem[2] = idx;
      inode.mem[3] = idx + 1;

      //write to file extent
      fileEx.mem[2] = idx;
      fileEx.mem[3] = 0;
      memcpy(fileEx.mem + 4, buffer + offset, 252);
      
      if (writeBlock(diskFD, idx, &(fileEx.mem)) < 0)
         return ERR_WRITE;
      offset += 252;
   }
  
	return SUCCESS;
	return SUCCESS;
}

int tfs_deleteFile(fileDescriptor FD) {
    unsigned char numFiles, position;
    tfs_block buf, lastFree;
    //ensure that disk is mounted
    if (mountedDisk == NULL)
        return ERR_TFS_NOT_MOUNTED;

    //read in inode
    readBlock(diskFD, FD, &(buf.mem));
    if (buf.mem[0] != 2)
        return ERR_INVALID_INODE;

    //get last free block
    readBlock(diskFD, 0, &(lastFree.mem));
    while (lastFree.mem[2] != '\0') {
        readBlock(diskFD, lastFree.mem[2], &(lastFree.mem));
    }

    //add inode to free chain of blocks
    lastFree.mem[2] = (unsigned char) FD;

    //add all blocks to free chain except for last one
    while (buf.mem[2] != '\0') {
        initFreeblock(&buf, buf.mem[2]);
        writeBlock(diskFD, lastFree.mem[2], buf.mem);
        lastFree = buf;
        readBlock(diskFD, buf.mem[2], &(buf.mem));
    }

    //add last one to chain
    initFreeblock(&buf, '\0');
    writeBlock(diskFD, lastFree.mem[2], buf.mem);

    openFilesLocation[FD] = 0;
    openFilesTable[FD] = "\0";

	return SUCCESS;
}

int tfs_readByte(fileDescriptor FD, char *buffer) {
    int ret, idx;
   tfs_block inode, fileEx;

   printf("readByte\n");
   
   //check if file is mounted and that file exists
   if((ret = checkMountAndFile(FD)) < 0)
      return ret;

   //get the current location of the file pouinter
   idx = openFilesLocation[FD];

   //read from inode to get the first ref to file extent
   if (readBlock(diskFD, FD, &(inode.mem)) < 0)
   {
      fprintf(stderr, "inode\n");
      return ERR_READ;
   }
   //get the location of the file extent

   //if (readBlock(diskFD, FD, &(fileEx.mem)) < 0)
   if (readBlock(diskFD, inode.mem[2], &(fileEx.mem)) < 0)
   {
      fprintf(stderr, "file extent\n");
      return ERR_READ;
   }
 
   
   while ((idx > 252) && (fileEx.mem[2] != '\0'))
   {
      ret = fileEx.mem[2];

      if (readBlock(diskFD, ret, &(fileEx.mem)) < 0)
      {
         return ERR_READ;
      }
      
      idx -= 252;
   }
   //check if EOF
   if (idx <= 252)
   {
      return ERR_INVALID_TFS;      
   }

   //get the reference to the first inode
    memcpy(buffer, fileEx.mem + 4 + idx, sizeof(char));

   //update inode
   openFilesLocation[FD]++;
   
	return SUCCESS;
}

int tfs_seek(fileDescriptor FD, int offset) {
	// Check if FD is in list of open files. 
	if (openFilesTable[FD] == '\0') {
		perror("seek: FD is not in list of open files");
		return ERR_SEEK;
	}
	
	// TODO: Check if offset puts you past last file extent.
	
	// Make sure the disk is mounted.
	if (!mountedDisk) {
		perror("seek: disk is not mounted");
		return ERR_SEEK;
	}
	
	// Check that we have a valid offset.
	if (offset < 0) {
		perror("seek: offset is invalid");
		return ERR_SEEK;
	}
	
	// If neither of these fails, set the FD to the offset.
	openFilesLocation[FD] = offset;
	
	return SUCCESS;
}
