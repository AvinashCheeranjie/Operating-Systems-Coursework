#ifndef FS_INDEXED_H
#define FS_INDEXED_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Macros */ 
#define BLOCK_SIZE 1024 // 1 KB per block                          
#define TOTAL_BLOCKS 64 // total logical disk blocks               
#define MAX_FILES 10    // maximum files in the file system        
#define MAX_FILENAME 64 // maximum length of a file name           

/* Block */
typedef struct Block {
    unsigned char data[BLOCK_SIZE];
    int blockNumber;
} Block;

/* Free-list node – wraps a pointer to a disk Block */
typedef struct FreeNode {
    Block *block;
    struct FreeNode *next;
} FreeNode;

/* Index Block – Holds pointers to every data block allocated for a file */
typedef struct IndexBlock {
    Block *dataBlocks[TOTAL_BLOCKS]; // pointers to data blocks           
    int count;                       // number of valid pointers          
} IndexBlock;

/* File Information Block */
typedef struct FIB {
    int fibID;                   // unique FIB identifier         
    char fileName[MAX_FILENAME]; // null-terminated file name     
    int fileSize;                // file size in bytes            
    int blockCount;              // number of data blocks         
    Block *physIndexBlock;       // physical disk block for index 
    IndexBlock *indexBlock;      // in-memory index structure     
} FIB;

/* Used to track which FIB IDs are currently available for assignment */
typedef struct FIBIDNode {
    int id;
    struct FIBIDNode *next;
} FIBIDNode;

/* Volume Control Block */
typedef struct VCB {
    // free block list simulated as a linked list 
    FreeNode *freeListHead;
    FreeNode *freeListTail;
    int freeBlockCount;

    Block disk[TOTAL_BLOCKS]; // logical disk blocks available in the volume 

    // FIB ID pool – tracks available IDs
    FIBIDNode *fibIDHead;
    int fibIDCount;

    // list of FIBs and active file count
    FIB fibs[MAX_FILES];
    int fileCount;
} VCB;

/* File System */
typedef struct FileSystem {
    VCB vcb;
} FileSystem;

/* Core file system operations */
void initFS(void);
int createFile(const char *filename, int size);
int deleteFile(const char *filename);
void listFiles(void);

/* Utility functions */
Block *allocateFreeBlock(void);
void returnFreeBlock(Block *block);
void printFreeBlocks(void);

/* FIB ID management helpers */
int getFIBID(void);
void returnFIBID(int id);

#endif /* FS_INDEXED_H */