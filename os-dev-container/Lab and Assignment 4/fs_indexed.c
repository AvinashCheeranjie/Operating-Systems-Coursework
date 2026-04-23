#include "fs_indexed.h"

/* Global file system instance */
FileSystem fs;

/*
 * getFileInformationBlockID
 *   Pops the next available FIB ID from the head of the ID pool.
 *   Returns the ID on success, or -1 if no IDs are available.
 */
int getFIBID(void)
{
    VCB *vcb = &fs.vcb;

    if (vcb->fibIDHead == NULL) {
        fprintf(stderr, "getFIBID: no FIB IDs available.\n");
        return -1;
    }

    FIBIDNode *node = vcb->fibIDHead;
    int id = node->id;
    vcb->fibIDHead = node->next;
    free(node);
    vcb->fibIDCount--;

    return id;
}

/*
 * returnFIBID
 *   Returns a FIB ID back to the head of the ID pool so it can be
 *   reassigned to a future file.
 */
void returnFIBID(int id)
{
    VCB *vcb = &fs.vcb;

    FIBIDNode *node = (FIBIDNode *)malloc(sizeof(FIBIDNode));
    if (!node) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    node->id = id;
    node->next = vcb->fibIDHead; /* prepend – ID recycled at the front  */
    vcb->fibIDHead = node;
    vcb->fibIDCount++;
}

/*
 * allocateFreeBlock
 *   Removes the block at the HEAD of the free-list and returns it.
 *   Returns NULL if no free blocks are available.
 */
Block *allocateFreeBlock(void)
{
    VCB *vcb = &fs.vcb;

    if (vcb->freeListHead == NULL) {
        fprintf(stderr, "allocateFreeBlock: no free blocks available.\n");
        return NULL;
    }

    FreeNode *node = vcb->freeListHead;
    vcb->freeListHead = node->next;
    if (vcb->freeListHead == NULL)
        vcb->freeListTail = NULL;

    Block *block = node->block;
    free(node);
    vcb->freeBlockCount--;
    return block;
}

/*
 * returnFreeBlock
 *   Appends a block to the tail of the free-list.
 */
void returnFreeBlock(Block *block)
{
    VCB *vcb = &fs.vcb;

    FreeNode *node = (FreeNode *)malloc(sizeof(FreeNode));
    if (!node) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    node->block = block;
    node->next = NULL;

    if (vcb->freeListTail == NULL) {
        vcb->freeListHead = node;
        vcb->freeListTail = node;
    } else {
        vcb->freeListTail->next = node;
        vcb->freeListTail = node;
    }
    vcb->freeBlockCount++;
}

/*
 * printFreeBlocks
 *   Displays every block number in the free-list and the total count.
 */
void printFreeBlocks(void)
{
    VCB *vcb = &fs.vcb;
    FreeNode *cur = vcb->freeListHead;

    printf("\nFree Blocks (%d): ", vcb->freeBlockCount);
    while (cur) {
        printf("[%d]", cur->block->blockNumber);
        if (cur->next)
            printf(" -> ");
        cur = cur->next;
    }
    printf(" -> NULL\n");
}

/*
 * initFS
 *   Initializes the file system
 */
void initFS(void)
{
    VCB *vcb = &fs.vcb;

    // Initialise every disk block
    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        memset(vcb->disk[i].data, 0, BLOCK_SIZE);
        vcb->disk[i].blockNumber = i;
    }

    // Build the free-list (all blocks free at start)
    vcb->freeListHead = NULL;
    vcb->freeListTail = NULL;
    vcb->freeBlockCount = 0;

    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        FreeNode *node = (FreeNode *)malloc(sizeof(FreeNode));
        if (!node) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        node->block = &vcb->disk[i];
        node->next = NULL;

        if (vcb->freeListHead == NULL) {
            vcb->freeListHead = node;
            vcb->freeListTail = node;
        } else {
            vcb->freeListTail->next = node;
            vcb->freeListTail = node;
        }
        vcb->freeBlockCount++;
    }

    // Seed FIB ID pool with IDs 0 … MAX_FILES-1
    vcb->fibIDHead = NULL;
    vcb->fibIDCount = 0;

    // Insert in reverse so that ID 0 ends up at the head
    for (int i = MAX_FILES - 1; i >= 0; i--) {
        FIBIDNode *node = (FIBIDNode *)malloc(sizeof(FIBIDNode));
        if (!node) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        node->id = i;
        node->next = vcb->fibIDHead;
        vcb->fibIDHead = node;
        vcb->fibIDCount++;
    }

    // Clear the FIB table
    for (int i = 0; i < MAX_FILES; i++) {
        vcb->fibs[i].fibID = -1;
        vcb->fibs[i].fileName[0] = '\0';
        vcb->fibs[i].fileSize = 0;
        vcb->fibs[i].blockCount = 0;
        vcb->fibs[i].physIndexBlock = NULL;
        vcb->fibs[i].indexBlock = NULL;
    }
    vcb->fileCount = 0;

    printf("Filesystem initialized successfully: %d blocks of %d bytes each.\n",
           TOTAL_BLOCKS, BLOCK_SIZE);
}

/*
 * createFile
 *   Creates a file of the requested size (bytes) using indexed allocation.
 *   Returns 0 on success, -1 on failure.
 */
int createFile(const char *filename, int size)
{
    VCB *vcb = &fs.vcb;

    // Validate constraints
    if (vcb->fileCount >= MAX_FILES) {
        fprintf(stderr,
                "createFile error: maximum file limit (%d) reached. "
                "Cannot create '%s'.\n",
                MAX_FILES, filename);
        return -1;
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (vcb->fibs[i].fibID != -1 &&
            strcmp(vcb->fibs[i].fileName, filename) == 0) {
            fprintf(stderr,
                    "createFile error: file '%s' already exists.\n", filename);
            return -1;
        }
    }

    if (size <= 0) {
        fprintf(stderr,
                "createFile error: invalid size %d for file '%s'.\n",
                size, filename);
        return -1;
    }

    // Check block availability
    int dataBlocksNeeded = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int totalNeeded = dataBlocksNeeded + 1; /* +1 for the index block */

    if (vcb->freeBlockCount < totalNeeded) {
        fprintf(stderr,
                "createFile error: not enough free blocks for '%s' "
                "(need %d, have %d).\n",
                filename, totalNeeded, vcb->freeBlockCount);
        return -1;
    }

    // Allocate index block and data blocks
    Block *physIdx = allocateFreeBlock(); /* physical disk block for index */

    IndexBlock *ib = (IndexBlock *)malloc(sizeof(IndexBlock));
    if (!ib) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    ib->count = dataBlocksNeeded;

    for (int i = 0; i < dataBlocksNeeded; i++) {
        ib->dataBlocks[i] = allocateFreeBlock();
    }

    // Obtain a FIB ID
    int fibID = getFIBID();
    if (fibID == -1) {
        fprintf(stderr, "createFile error: could not obtain a FIB ID.\n");
        returnFreeBlock(physIdx);
        for (int i = 0; i < dataBlocksNeeded; i++)
            returnFreeBlock(ib->dataBlocks[i]);
        free(ib);
        return -1;
    }

    // Populate and store the FIB
    FIB *fib = &vcb->fibs[fibID];
    fib->fibID = fibID;
    strncpy(fib->fileName, filename, MAX_FILENAME - 1);
    fib->fileName[MAX_FILENAME - 1] = '\0';
    fib->fileSize = size;
    fib->blockCount = dataBlocksNeeded;
    fib->physIndexBlock = physIdx;
    fib->indexBlock = ib;

    vcb->fileCount++;

    printf("File '%s' created successfully with %d data block(s) + 1 index block.\n",
           filename, dataBlocksNeeded);
    return 0;
}

/*
 * deleteFile
 *   Returns 0 on success, -1 if the file is not found.
 */
int deleteFile(const char *filename)
{
    VCB *vcb = &fs.vcb;

    // locate the FIB
    int slot = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (vcb->fibs[i].fibID != -1 &&
            strcmp(vcb->fibs[i].fileName, filename) == 0) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        fprintf(stderr,
                "deleteFile error: file '%s' not found.\n", filename);
        return -1;
    }

    FIB *fib = &vcb->fibs[slot];

    // Return data blocks to the free-list
    for (int i = 0; i < fib->blockCount; i++) {
        returnFreeBlock(fib->indexBlock->dataBlocks[i]);
    }

    // return the physical index block
    returnFreeBlock(fib->physIndexBlock);

    // free the in-memory index structure
    free(fib->indexBlock);
    fib->indexBlock = NULL;
    fib->physIndexBlock = NULL;

    // Return the FIB ID to the pool
    returnFIBID(fib->fibID);

    // Clear the FIB entry
    fib->fibID = -1;
    fib->fileName[0] = '\0';
    fib->fileSize = 0;
    fib->blockCount = 0;

    vcb->fileCount--;

    printf("File '%s' deleted successfully.\n", filename);
    return 0;
}

/*
 * listFiles
 *   Displays all active files in the flat file system directory.
 */
void listFiles(void)
{
    VCB *vcb = &fs.vcb;

    printf("\nRoot Directory Listing (%d file%s):\n",
           vcb->fileCount, vcb->fileCount == 1 ? "" : "s");

    for (int i = 0; i < MAX_FILES; i++) {
        if (vcb->fibs[i].fibID != -1) {
            FIB *fib = &vcb->fibs[i];
            printf("  %-12s | %6d bytes | %2d data blocks | FIBID = %d\n",
                   fib->fileName, fib->fileSize,
                   fib->blockCount, fib->fibID);
        }
    }
}