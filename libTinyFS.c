/* Program 4
 * Daniel Foxhoven
 * Geoff Wacker
 * Adair Camacho
 * Due Date: 3/19/17
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "tinyFS_errno.h"
#include "tinyFS.h"
#include "libTinyFS.h"
#include "libDisk.h"

int diskFD;
int freeBlocks;
int numBlocks;
int *openFilesLocation;
char **openFilesTable;
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
		
		if (writeBlock(fd, traversed, buf.mem) < 0) {
			return MKFS_FAILURE;
		}

        while ((++traversed * BLOCKSIZE) < nBytes) {
		    /* init and write all free */
		    if ((traversed + 1) * BLOCKSIZE >= nBytes) {
		        initFreeblock(&buf, '\0');
            }
		    else {
		        initFreeblock(&buf, traversed+1);
            }
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
	// Set block type.
	buf->mem[0] = 2;

    buf->mem[2] = '\0';

	// Read/Write permission
    // read only 0
    // read-write 1
	buf->mem[3] =  1;

	// Write the name.
    strncpy(&(buf->mem[5]), name, 9);
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

		// Check that the block is valid.
		if (buf[1] != 0x44) {
			perror("mount: TFS is invalid");
			return ERR_INVALID_TFS;
		}
    }
    numBlocks = buf[4];
    freeBlocks = numBlocks - 1;
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
   if (mountedDisk == NULL) {
      perror("Error: disk not mounted");
      return ERR_TFS_NOT_MOUNTED;
   }
   if(openFilesTable[FD] == '\0') {
      perror("Error: file closed or does not exist");
      return ERR_INVALID_TFS;
   }

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
	char tempName[9];
	unsigned char firstFree;
    fileExists = 0;
    time_t curTime;

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

		// Get the address of the first free block.
		firstFree = buf.mem[2];

		// Use that as the upper bound for the open files table so we can iterate through it.
		// Iterate through the table, and check if we find an entry that equals our name.
		for (int i = 1; i < numBlocks; i++) {

            if(openFilesTable[i] != '\0') {
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
	for (int i = 1; i <= numBlocks && !fileExists; i++) {
		// Read the block.
		readBlock(diskNum, i, &(buf.mem));

		// Check if this block is an inode.
		if (buf.mem[0] == 2) {

			// Now check if the names equal.
			if(!strcmp(name, buf.mem+5)) {
				fileExists = 1;
				fd = i;
				break;
			}
		}
	}

	// Existing file wasn't found, so we need to create one.
	if (!fileExists) {

        // Read in the super block.
        readBlock(diskNum, 0, &(super.mem));

		// Read in the first free block.
		readBlock(diskNum, firstFree, &(buf.mem));

        // Get reference.
        super.mem[2] = buf.mem[2];

        // Init the inode block at that free block.
		initInodeblock(&buf, name);

        // Update the first free in the super block.
        writeBlock(diskNum, 0, &(super.mem));

		// NGet a file descriptor to the next free block.
        fd = firstFree;

        // Get the current time.
        time(&curTime);

        // Write creation date.
        memcpy(&(buf.mem[18]), &curTime, sizeof(time_t));

        // Write last modified date.
        memcpy(&(buf.mem[18 + sizeof(time_t)]), &curTime, sizeof(time_t));

        // Write last accessed date.
        memcpy(&(buf.mem[18 + (2 * sizeof(time_t))]), &curTime, sizeof(time_t));

		writeBlock(diskNum, fd, &(buf.mem));

        strcpy(openFilesTable[fd], name);

		// Reset the file descriptor location.
		openFilesLocation[fd] = 0;
	}

	// The file exists, we just need to open it.
	else {
        openFilesTable[fd] = calloc(strlen(name), sizeof(char));
		strcpy(openFilesTable[fd], name);

		// Reset the file descriptor location.
		openFilesLocation[fd] = 0;

        // Get the current time.
        time(&curTime);

        // Write last accessed date.
        memcpy(&(buf.mem[18 + (2 * sizeof(time_t))]), &curTime, sizeof(time_t));
	}

	return fd;
}

int tfs_closeFile(fileDescriptor FD) {

	// File is open, so close it.
	if (openFilesTable[FD] != '\0') {
		openFilesTable[FD] = '\0';
		return SUCCESS;
	}
	// Not open, so we can't close it.
	else {
		return ERR_FILE_CLOSE;
	}
}

int getNumBlocks(int size) {
   int blocks = size / BLOCKSIZE;

   if (size % BLOCKSIZE)
      blocks++;

   return blocks;
}

int tfs_writeFile(fileDescriptor FD,char *buffer, int size) {
    int ret, index = 0, reqBlocks = getNumBlocks(size), offset = 0, i, position;
    tfs_block super, inode, temp;
    time_t curTime;

    //check if file is mounted and that the file exists
    if((ret = checkMountAndFile(FD)) < 0) {
        return ret;
    }

    //read inode out
    if(readBlock(diskFD, FD, &(inode.mem)) < 0) {
        fprintf(stderr, "writeFile inode\n");
        return ERR_READ;
    }

    //check the file permission
    if(inode.mem[3] == 0)
    {
        return ERR_READ_ONLY;
    }
    
    //check to see if we have enough space to write the data
    if (freeBlocks  <  reqBlocks) {
        fprintf(stderr, "Error: not enough space available, numBlocks %d, freeBlocks %d, reqBlocks %d\n",
        numBlocks, freeBlocks, reqBlocks);

        return ERR_INVALID_SPACE;
    }

    //deallocate data blocks
    if (resetFile(FD) < 0) {
        fprintf(stderr, "could not reset file, FD is %d\n\n", FD);
    }
    
    //get the super block to get the current number of free blocks
    if(readBlock(diskFD, 0, &(super.mem)) < 0) {
        perror("Error: reading super block failed\n");
        return ERR_READ;
    }
    //update the inode block
    inode.mem[14] = reqBlocks;

    // Get the time.
    time(&curTime);

    // Write last modified date.
    memcpy(&(inode.mem[18 + sizeof(time_t)]), &curTime, sizeof(time_t));

    // Write last accessed date.
    memcpy(&inode.mem[18 + (2 * sizeof(time_t))], &curTime, sizeof(time_t));

    //at this point we have the inode and the super block
    inode.mem[2] = super.mem[2];
    writeBlock(diskFD, FD, inode.mem);

    //read in inode
    readBlock(diskFD, super.mem[2], &(temp.mem));
    //get the position of the first file extent
    position = super.mem[2];
    //initialize the file extent
    initExtent(&temp, temp.mem[2]);

    //check the offset to see if it's greater than 252 bytes
    offset = (size - index >= 252) ? (252) : (size - index);
    //copy the buffer into a temp/file extent block
    memcpy(temp.mem + 4, buffer + index, offset);
    //increment the current file pointer
    index += offset;
    //updae the file extent
    writeBlock(diskFD, inode.mem[2], temp.mem);
    freeBlocks--;

    //repeat the process for the next file extent blocks
    for (i = 1; i < reqBlocks; i++) {
        position = temp.mem[2];
        readBlock(diskFD, temp.mem[2], &(temp.mem));
        initExtent(&temp, temp.mem[2]);
        offset = (size - index >= 252) ? (252) : (size - index);
        memcpy(temp.mem + 4, buffer + index, offset);
        index += offset;
        writeBlock(diskFD, position, temp.mem);
        freeBlocks--;
    }
    super.mem[2] = temp.mem[2];
    temp.mem[2] = '\0';
    //update super block
    writeBlock(diskFD, 0, super.mem);
    writeBlock(diskFD, position, temp.mem);
    openFilesLocation[FD] = 0;

    return SUCCESS;
}

void initExtent(tfs_block *block, unsigned char next) {
    int i;
    block->mem[0] = 3;
    block->mem[1] = MAGIC_NUM;
    block->mem[2] = next;
    
    for (i = 3; i < BLOCKSIZE; i++) {
        block->mem[i] = 0x00;
    }
}

int tfs_deleteFile(fileDescriptor FD) {
    unsigned char position;
    tfs_block buf, lastFree;
    //ensure that disk is mounted
    if (mountedDisk == NULL) {
        return ERR_TFS_NOT_MOUNTED;
    }

    //read in inode
    readBlock(diskFD, FD, &(buf.mem));
    if (buf.mem[0] != 2) {
        fprintf(stderr, "block is not inode, type: %d, FD %d\n\n", buf.mem[0], FD);
        return ERR_INVALID_INODE;
    }

    //check file permission
    if(buf.mem[3] == 0)
    {
        fprintf(stdout, "tfs_deleteFile: File is read-only\n");
        return ERR_READ_ONLY;
    }

    //get last free block
    readBlock(diskFD, 0, &(lastFree.mem));
    
    while (lastFree.mem[2] != '\0') {
		position = lastFree.mem[2];
        readBlock(diskFD, lastFree.mem[2], &(lastFree.mem));
    }

    //add inode to free chain of blocks
    lastFree.mem[2] = (unsigned char) FD;
	writeBlock(diskFD, position, lastFree.mem);

    //add all blocks to free chain except for last one
    while (buf.mem[2] != '\0') {
		freeBlocks++;
        initFreeblock(&buf, buf.mem[2]);
        writeBlock(diskFD, lastFree.mem[2], buf.mem);
        lastFree = buf;
        readBlock(diskFD, buf.mem[2], &(buf.mem));
    }

    //add last one to chain
	freeBlocks++;
    initFreeblock(&buf, '\0');
    writeBlock(diskFD, lastFree.mem[2], buf.mem);

    openFilesLocation[FD] = 0;
    openFilesTable[FD] = '\0';

	return SUCCESS;
}

int tfs_readByte(fileDescriptor FD, char *buffer) {
    int ret, idx;
    tfs_block inode, fileEx;
    time_t curTime;

    //check if file is mounted and that file exists
    if((ret = checkMountAndFile(FD)) < 0) {
        return ret;
    }
   
    //get the current location of the file pouinter
    idx = openFilesLocation[FD];

    //read from inode to get the first ref to file extent
    if (readBlock(diskFD, FD, &(inode.mem)) < 0) {
        fprintf(stderr, "inode\n");
        return ERR_READ;
    }
    
    //get the location of the file extent

    //if (readBlock(diskFD, FD, &(fileEx.mem)) < 0)
    if (readBlock(diskFD, inode.mem[2], &(fileEx.mem)) < 0) {
        fprintf(stderr, "file extent\n");
        return ERR_READ;
    }

    // Get the time.
    time(&curTime);

    // Write last accessed date.
    memcpy(&(inode.mem[18 + (2 * sizeof(time_t))]), &curTime, sizeof(time_t));

    // Write the date back to the inode.
    writeBlock(diskFD, FD, inode.mem);

    while ((idx >= 252) && (fileEx.mem[2] != '\0')) {
        ret = fileEx.mem[2];

        if (readBlock(diskFD, ret, &(fileEx.mem)) < 0) {
            perror("failed to read file extent");
            return ERR_READ;
        }

        idx -= 252;
    }
   
    //check if EOF
    if (idx >= 252) {
        return ERR_INVALID_TFS;
    }

    //get the reference to the first inode
    memcpy(buffer, fileEx.mem + 4 + idx, sizeof(char));

    if (buffer[0] == '\0') {
        return ERR_READ;
    }

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

int tfs_rename(fileDescriptor FD, char* newName) {
	tfs_block buf;
    time_t curTime;
	
    //check for file name length
	if (strlen(newName) > 8 || strlen(newName) == 0) {
		perror("rename: invalid name");
		return ERR_FILE_NAME_LENGTH;
	}
	
    //check if file is open
	if (openFilesTable[FD] == '\0') {
		perror("rename: file closed");
		return ERR_FILE_CLOSED;
	}

	//read in inode of file to rename
	readBlock(diskFD, FD, &(buf.mem));
	//copy in new name
	strncpy(&(buf.mem[5]), newName, 9);

    // Get the current time.
    time(&curTime);

    // Write last modified date.
    memcpy(&(buf.mem[18 + sizeof(time_t)]), &curTime, sizeof(time_t));

    // Write last accessed date.
    memcpy(&(buf.mem[18 + (2 * sizeof(time_t))]), &curTime, sizeof(time_t));

	strncpy(openFilesTable[FD], newName, 9);
	//write block back with modifications
	writeBlock(diskFD, FD, buf.mem);

    return SUCCESS;
}

void tfs_readdir() {
	int i;
	tfs_block buf;

	for (i = 1; i < freeBlocks; i++) {
		readBlock(diskFD, i, &(buf.mem));
		//if it's a inode, print the name and size
		if (buf.mem[0] == 2) {
			printf("%s : %d blocks\n", &(buf.mem[5]), buf.mem[14]);
		}
	}
}

time_t tfs_readFileInfo(fileDescriptor FD) {
    time_t creationTime;
	tfs_block buf;

	//check if file is open
	if (openFilesTable[FD] == '\0') {
		perror("readFileInfo: file closed");
		return ERR_FILE_CLOSED;
	}

	//read in inode of file to access time.
	readBlock(diskFD, FD, &(buf.mem));

	// Grab the creation time.
	memcpy(&creationTime, &buf.mem[18], sizeof(time_t));

	return creationTime;
}

time_t tfs_readFileLastModified(fileDescriptor FD) {
    time_t lastModifiedTime;
	tfs_block buf;

	//check if file is open
	if (openFilesTable[FD] == '\0') {
		perror("readFileLastModified: file closed");
		return ERR_FILE_CLOSED;
	}

	//read in inode of file to access time.
	readBlock(diskFD, FD, &(buf.mem));

	// Grab the last modified time.
	memcpy(&lastModifiedTime, &buf.mem[26], sizeof(time_t));

	return lastModifiedTime;
}
time_t tfs_readFileLastAccessed(fileDescriptor FD) {
    time_t lastAccessedTime;
	tfs_block buf;

	//check if file is open
	if (openFilesTable[FD] == '\0') {
		perror("readFileLastAccessed: file closed");
		return ERR_FILE_CLOSED;
	}

	//read in inode of file to access time.
	readBlock(diskFD, FD, &(buf.mem));

	// Grab the last accessed time.
	memcpy(&lastAccessedTime, &buf.mem[34], sizeof(time_t));

	return lastAccessedTime;
}

//read only 0
//read-write 1

void tfs_makeRO(char *name) {
    int idx, extentIdx = -1;
    tfs_block inode;

    for(idx = 1; idx <= numBlocks -freeBlocks; idx++) {
        //read the inode
        readBlock(diskFD, idx, &(inode.mem));

        if (inode.mem[0] == 2)
        {
            //check to see if the file is in the disk
            if (!strcmp(inode.mem + 5, name)) {
                extentIdx = inode.mem[2];
                inode.mem[3] = 0;

                writeBlock(diskFD, idx, inode.mem);
                fprintf(stdout, "file %s is now read-only\n", name);
                break;
            }
        }
    }
    if (extentIdx < 0)
        fprintf(stderr, "tfs_makeRO: file %s not found\n", name);
}

void tfs_makeRW(char *name) {
    int idx, extentIdx = -1;
    tfs_block inode;

    for(idx = 1; idx <= numBlocks - freeBlocks; idx++) {
        //read the inode
        readBlock(diskFD, idx, &(inode.mem));
            
        //check to see if the file is in the disk
        if (inode.mem[0] == 2)
        {
            if (!strcmp(inode.mem + 5, name)) {
                extentIdx = inode.mem[2];
                inode.mem[3] = 1;

                writeBlock(diskFD, idx, inode.mem);
                break;
            }
        }
    }
    if (extentIdx < 0)
        fprintf(stderr, "tfs_makeRW: file %s not found\n", name);
}

int writeByte(fileDescriptor FD, unsigned int data) {
    char buffer;
    int ret, idx;
    tfs_block inode, fileEx;
    time_t curTime;


    //check if file is mounted and that file exists
    if((ret = checkMountAndFile(FD)) < 0) {
        return ret;
    }
   
    //get the current location of the file pointer
    idx = openFilesLocation[FD];

    //read from inode to get the first ref to file extent
    if (readBlock(diskFD, FD, &(inode.mem)) < 0) {
        fprintf(stderr, "writeByte: could not read inode\n");
        return ERR_READ;
    }
    
    //get the location of the file extent

    //if (readBlock(diskFD, FD, &(fileEx.mem)) < 0)
    if (readBlock(diskFD, inode.mem[2], &(fileEx.mem)) < 0) {
        fprintf(stderr, "writeByte: could not read file extent\n");
        return ERR_READ;
    }

    // Get the time.
    time(&curTime);

    // Write last accessed/modified date.
    memcpy(&(inode.mem[18 + sizeof(time_t)]), &curTime, sizeof(time_t));
    memcpy(&(inode.mem[18 + (2 * sizeof(time_t))]), &curTime, sizeof(time_t));

    // Write the date back to the inode.
    writeBlock(diskFD, FD, inode.mem);
    ret = inode.mem[2];
    while ((idx >= 252) && (fileEx.mem[2] != '\0')) {
        ret = fileEx.mem[2];

        if (readBlock(diskFD, ret, &(fileEx.mem)) < 0) {
            perror("failed to read file extent");
            return ERR_READ;
        }

        idx -= 252;
    }
   
    //check if EOF
    if (idx >= 252) {
        return ERR_INVALID_TFS;
    }

    //get the reference to the first inode
    memcpy(&buffer, fileEx.mem + 4 + idx, sizeof(char));

    if (buffer == '\0') {
        return ERR_READ;
    }

    fileEx.mem[4+idx] = (unsigned char) data;
    writeBlock(diskFD, ret, fileEx.mem);

	return SUCCESS;
}

int resetFile(fileDescriptor FD) {
    unsigned char position;
    tfs_block buf, lastFree;

    //read in inode
    readBlock(diskFD, FD, &(buf.mem));
    if (buf.mem[0] != 2)
    {
        fprintf(stderr, "block is not inode, type: %d, FD %d\n\n", buf.mem[0], FD);
        return ERR_INVALID_INODE;
    }
    //get last free block
    readBlock(diskFD, 0, &(lastFree.mem));
    while (lastFree.mem[2] != '\0') {
		position = lastFree.mem[2];
        readBlock(diskFD, lastFree.mem[2], &(lastFree.mem));
    }

    //don't do anything if there are no file extents
    if (buf.mem[2] == '\0')
        return SUCCESS;

    lastFree.mem[2] = (unsigned char) buf.mem[2];
	writeBlock(diskFD, position, lastFree.mem);

    //update inode to point to nothing
    buf.mem[2] = '\0';
    writeBlock(diskFD, FD, buf.mem);

    //add all blocks to free chain except for last one
    readBlock(diskFD, lastFree.mem[2], &(buf.mem));
    while (buf.mem[2] != '\0') {
		freeBlocks++;
        initFreeblock(&buf, buf.mem[2]);
        writeBlock(diskFD, lastFree.mem[2], buf.mem);
        lastFree = buf;
        readBlock(diskFD, buf.mem[2], &(buf.mem));
    }

    //add last one to chain
	freeBlocks++;
    initFreeblock(&buf, '\0');
    writeBlock(diskFD, lastFree.mem[2], buf.mem);

    openFilesLocation[FD] = 0;

	return SUCCESS;
}

int tfs_displayFragments() {
    int i;
    int count = 0;
    tfs_block buf;

    // TFS is already unmounted, so throw error.
	if (!mountedDisk) {
		perror("displayFragments: TFS already unmounted");
		return ERR_TFS_UNMOUNT;
	}

    // Loop through all of our memory and print out visual representation of each block.
    for (i = 0; i < numBlocks; i++) {
        // Read the current block into the buffer.
        if (readBlock(diskFD, i, &(buf.mem)) < 0) {
            perror("displayFragments: read error");
            return ERR_READ;
        }

        // Block is superblock.
        if (buf.mem[0] == 1) {
            printf("[S]");
        }
        // Block is inode.
        else if (buf.mem[0] == 2) {
            printf("[I]");
        }
        // Block is file extent.
        else if (buf.mem[0] == 3) {
            printf("[E]");
        }
        // Block is free.
        else if (buf.mem[0] == 4) {
            printf("[F]");
        }
        count++;
        if (count % 10 == 0) {
            printf("\n");
        }
    }

    printf("\n");

    return SUCCESS;
}

int tfs_defrag() {
    int i;
   //unsigned char firstFree;
    tfs_block buf;

    // TFS is already unmounted, so throw error.
	if (!mountedDisk) {
		perror("displayFragments: TFS already unmounted");
		return ERR_TFS_UNMOUNT;
	}

    // Loop through all of our memory.
    for (i = 1; i < numBlocks; i++) {
        // Read the current block into the buffer.
        if (readBlock(diskFD, i, &(buf.mem)) < 0) {
            perror("defrag: read error");
            return ERR_READ;
        }
        // Keep looping until we find our first free block.
        if (buf.mem[0] != 4) {
            continue;
        }
    }

    return SUCCESS;
}
