#include "fs_indexed.h"

int main(void) {

    initFS();

    createFile("alpha.txt", 3 * BLOCK_SIZE);
    createFile("beta.txt",  5 * BLOCK_SIZE);

    listFiles();
    printFreeBlocks();

    deleteFile("alpha.txt");

    listFiles();
    printFreeBlocks();

    createFile("gamma.txt", 4 * BLOCK_SIZE);
    createFile("delta.txt", 8 * BLOCK_SIZE);

    listFiles();
    printFreeBlocks();

    deleteFile("beta.txt");
    deleteFile("gamma.txt");
    deleteFile("delta.txt");

    listFiles();
    printFreeBlocks();

    return 0;
}