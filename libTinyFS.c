#include <stdio.h>
#include "tinyFS_errno.h"
#include "tinyFS.h"

int tfs_mkfs(char *filename, int nBytes) {
	return 0;
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