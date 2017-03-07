/* The default size of the disk and file system block */
#define BLOCKSIZE 256
/* Your program should use a 10240 Byte disk size giving you 40
blocks total. This is a default size. You must be able to support
different possible values */
#define DEFAULT_DISK_SIZE 10240
/* use this name for a default disk file name */
#define DEFAULT_DISK_NAME “tinyFSDisk”
typedef int fileDescriptor;