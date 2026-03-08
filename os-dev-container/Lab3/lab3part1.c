#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 10

// Page properties based on a 4-KB page size (2^12 bytes)
#define OFFSET_BITS 12
#define PAGE_SIZE (1 << OFFSET_BITS) // 4096 bytes
#define OFFSET_MASK (PAGE_SIZE - 1)  // 0xFFF or 4095
#define PAGES 8                      

int main(void)
{
    // open labaddr.txt file for reading
    FILE *fptr = fopen("labaddr.txt", "r");
    if (fptr == NULL) {
        perror("Error opening file");
        return 1;
    }

    int page_table[PAGES] = {6, 4, 3, 7, 0, 1, 2, 5};

    char buff[BUFFER_SIZE];

    while (fgets(buff, BUFFER_SIZE, fptr) != NULL) { // read each line
        char *newline = strchr(buff, '\n');
        if (newline) {
            *newline = '\0';
        }

        // convert the string to an integer (logical address)
        int logical_addr = atoi(buff);

        int page_number = logical_addr >> OFFSET_BITS;

        int offset = logical_addr & OFFSET_MASK;

        int frame_number = page_table[page_number];

        // shift the frame number left by OFFSET_BITS and combine it with the offset.
        int physical_addr = (frame_number << OFFSET_BITS) | offset;

        printf("Virtual addr is %d: Page# = %d & Offset = %d. Physical addr = %d.\n",
               logical_addr, page_number, offset, physical_addr);
    }

    fclose(fptr);
    return 0;
}