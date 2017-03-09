# 453_tinyfs

## Superblock
 - Has to be at Block 0
 - Byte 0: block type = 1
 - Byte 1: "magic number" = 0x44
 - Byte 2: first free block address
 
## Inode
 - Beginning of a file
 - Byte 0: block type = 2
 - Byte 1: "magic number" = 0x44
 - Byte 2: points to first file extent
 - Byte 3: EMPTY
 - Byte 4-11: file name
 - Byte 12: null term
 - Byte 13-16: file size (standard int)
 - Byte 17-UNKNOWN: 3 timestamps (created, modified, accessed)

## File Extent
 - Contains file data
 - Byte 0: block type = 3
 - Byte 1: "magic number" = 0x44
 - Byte 2: points to next file extent or NULL if last one
 - Byte 3: EMPTY
 - Byte 4-255: Content
 
## Free
 - Free block
 - Byte 0: block type = 4
 - Byte 1: "magic number" = 0x44
 - Byte 2: points to next free block or NULL if last one
