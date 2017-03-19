#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tinyFS.h"
#include "libTinyFS.h"
#include "tinyFS_errno.h"

void waitForEnter() {
    printf("Press ENTER to proceed...\n");
    getchar();
}

int main(int argc, char *argv[]) {
    char readBuffer;
    char content1[] = "content 1";
    char content2[] = "content 2";
    char content3[] = "content 3";

    fileDescriptor FD1, FD2, FD3;
    waitForEnter();
    printf("Attempting to mount disk\n");
    if (tfs_mount(DEFAULT_DISK_NAME) < 0) {
        tfs_mkfs(DEFAULT_DISK_NAME, DEFAULT_DISK_SIZE);
        if (tfs_mount(DEFAULT_DISK_NAME) < 0) {
            perror("disk mount failed, exiting...\n");
            return -1;
        }
    }
    printf("%s: mounted\n", DEFAULT_DISK_NAME);
    waitForEnter();
    printf("Attempting to open files\n");
    FD1 = tfs_openFile("File1");
    if (FD1 < 0)
        perror("failed to open File1");
    else
        printf("File1 opened\n");

    FD2 = tfs_openFile("File2");
    if (FD2 < 0)
        perror("failed to open File2");
    else
        printf("File2 opened\n");
    
    FD3 = tfs_openFile("File3");
    if (FD3 < 0)
        perror("failed to open File3");
    else
        printf("File3 opened\n");

    waitForEnter();
    printf("Writing content to files\n");
    if (tfs_writeFile(FD1, content1, 20) < 0)
        perror("write to File1 failed");
    else
        printf("Wrote to File1\n");
    
    if (tfs_writeFile(FD2, content2, 20) < 0)
        perror("write to File2 failed");
    else
        printf("Wrote to File2\n");

    if (tfs_writeFile(FD3, content3, 20) < 0)
        perror("write to File3 failed");
    else
        printf("Wrote to File3\n");

    waitForEnter();
    printf("Reading from File 1...\n");
    while (tfs_readByte(FD1, &readBuffer) >= 0)
        printf("%c", readBuffer);
    printf("\n");
    
    // Try to read from file 1. Won't print anything.
    // Print seek to begin of file1 
    // seek
    //read again, should get data

    waitForEnter();
    printf("Files present:\n");
    tfs_readdir();
    
    waitForEnter();
    printf("Renaming File2 to Blah2\n");
    if ((tfs_rename(FD2, "Blah2")) < 0)
        perror("failed to rename File2");
    printf("Files present:\n");
    tfs_readdir();

    waitForEnter();
    printf("Closing File3, then attempting to rename to Blah3\n");
    if (tfs_closeFile(FD3) < 0)
        perror("Error closing File3");
    if ((tfs_rename(FD3, "Blah3")) < 0)
        perror("failed to rename File3");
    printf("Files present:\n");
    tfs_readdir();

    waitForEnter();
    tfs_displayFragments("File1");

    /*
        other tests here
        tfs_seek

        // Test timestamps        

        tfs_displayFragments
        make_RO
        make_RW
        writeByte

        writeFile with larger files

        defrag (if time permits. function is not done yet)
    */

    waitForEnter();
    printf("Changing permission to read-only\n");
    tfs_makeRO("File1");
    
    waitForEnter();
    printf("Writing to file1\n");
    if (tfs_writeFile(FD1, content1, 20) < 0)
        fprintf(stderr, "tfs_writeFile: File is read-only\n");
    else
        printf("Wrote to File1\n");


    waitForEnter();
    printf("Deleting File1\n");
    tfs_deleteFile(FD1);

    waitForEnter();
    printf("Changing to permission to read-write\n");
    tfs_makeRW("File1");

    waitForEnter();
    printf("Deleting File1\n");
    tfs_deleteFile(FD1);

    waitForEnter();
    printf("Deleting Blah2\n");
    tfs_deleteFile(FD2);
    printf("Files present:\n");
    tfs_readdir();

    waitForEnter();
    printf("Unmounting: %s\n", DEFAULT_DISK_NAME);
    if (tfs_unmount() < 0)
        perror("Unmount failed");
    else
        printf("Unmount successful");
    printf("\nEND OF DEMO\n");
    return 0;
}
