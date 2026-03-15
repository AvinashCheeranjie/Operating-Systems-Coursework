#include <fcntl.h> /* For open() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h> /* For mmap() */
#include <unistd.h>   /* For close() */

#define OFFSET_BITS 8
#define PAGE_SIZE (1 << OFFSET_BITS) // 256 bytes
#define OFFSET_MASK (PAGE_SIZE - 1)  // 0xFF or 255
#define PAGES 256
#define FRAMES 128
#define TLB_SIZE 16
#define MEMORY_SIZE 65536

typedef struct {
    int page_number;
    int frame_number;
} TLBEntry;

// TLB implemented as a circular array with FIFO replacement
TLBEntry tlb[TLB_SIZE];
int tlb_next_insert = 0;
int tlb_count = 0;

// Function prototypes
static void init_tlb(void);
int search_TLB(int page_number);
void TLB_Add(int page_number, int frame_number);
int TLB_Update(int page_to_replace, int new_page, int new_frame);

int main(void)
{
    int page_table[PAGES];                          // maps page -> frame
    int frame_table[FRAMES];                        // maps frame -> page
    signed char physical_memory[FRAMES][PAGE_SIZE]; // simulated physical memory

    int num_addresses = 0; // track number of addresses processed
    int page_faults = 0;   // track number of page faults
    int next_free_frame = 0;
    int tlb_hits = 0; // track number of TLB hits
    int fifo_frame_ptr = 0;

    // open addresses.txt file for reading
    FILE *fptr = fopen("addresses.txt", "r");
    if (fptr == NULL) {
        perror("Error opening file");
        return 1;
    }

    // open BACKING_STORE.bin for reading
    int fd = open("BACKING_STORE.bin", O_RDONLY);
    if (fd < 0) {
        perror("Error opening BACKING_STORE.bin");
        exit(EXIT_FAILURE);
    }

    // map the file
    signed char *mmapfptr = mmap(NULL, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mmapfptr == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < PAGES; i++) {
        page_table[i] = -1; // initialize page table entries to -1 (indicating not present)
    }

    for (int i = 0; i < FRAMES; i++) {
        frame_table[i] = -1; // initialize frame table entries to -1 (indicating free)
    }

    init_tlb();

    char buff[32];
    while (fgets(buff, sizeof(buff), fptr) != NULL) {
        char *newline = strchr(buff, '\n');
        if (newline) {
            *newline = '\0';
        }

        if (buff[0] == '\0') {
            continue;
        }

        num_addresses++;

        int logical_addr = atoi(buff);
        int page_number = logical_addr >> OFFSET_BITS;
        int offset = logical_addr & OFFSET_MASK;

        int frame_number = search_TLB(page_number);
        int tlb_updated = 0;
        int page_fault_handled = 0;

        if (frame_number != -1 && page_table[page_number] == frame_number) {
            tlb_hits++;
        } else {
            frame_number = page_table[page_number];

            if (frame_number == -1) {
                page_faults++;
                page_fault_handled = 1;

                if (next_free_frame < FRAMES) {
                    frame_number = next_free_frame;
                    next_free_frame++;
                } else {
                    frame_number = fifo_frame_ptr;
                    fifo_frame_ptr = (fifo_frame_ptr + 1) % FRAMES;

                    int old_page = frame_table[frame_number];
                    if (old_page != -1) {
                        page_table[old_page] = -1;
                        tlb_updated = TLB_Update(old_page, page_number, frame_number);
                    }
                }

                memcpy(physical_memory[frame_number], mmapfptr + (page_number * PAGE_SIZE), PAGE_SIZE);

                page_table[page_number] = frame_number;
                frame_table[frame_number] = page_number;
            }

            if (page_fault_handled && !tlb_updated) {
                TLB_Add(page_number, frame_number);
            }
        }

        int physical_addr = (frame_number << OFFSET_BITS) | offset;
        signed char value = physical_memory[frame_number][offset];
        printf("Virtual address: %d Physical address = %d Value=%d \n", logical_addr, physical_addr, value);
    }

    // Ouput Statistics:
    printf("Total addresses = %d \n", num_addresses);
    printf("Page_faults = %d \n", page_faults);
    printf("TLB Hits = %d \n", tlb_hits);

    if (munmap(mmapfptr, MEMORY_SIZE) == -1) {
        perror("Error unmapping file");
    }

    close(fd);
    fclose(fptr);
    return 0;
}

// Initialize TLB entries to -1 (indicating invalid)
static void init_tlb(void)
{
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].page_number = -1;
        tlb[i].frame_number = -1;
    }
}

// Search TLB for the given page number. Return frame number if found, else -1.
int search_TLB(int page_number)
{
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].page_number == page_number) {
            return tlb[i].frame_number;
        }
    }
    return -1;
}

// Add a new page-frame mapping to the TLB using FIFO replacement policy
void TLB_Add(int page_number, int frame_number)
{
    tlb[tlb_next_insert].page_number = page_number;
    tlb[tlb_next_insert].frame_number = frame_number;
    tlb_next_insert = (tlb_next_insert + 1) % TLB_SIZE;
    if (tlb_count < TLB_SIZE) {
        tlb_count++;
    }
}

/* Update the TLB by removing the entry for page_to_replace and adding the new page-frame mapping.
   Return 1 if update was successful, else 0. */
int TLB_Update(int page_to_replace, int new_page, int new_frame)
{
    int oldest = (tlb_next_insert - tlb_count + TLB_SIZE) % TLB_SIZE;
    int pos = -1;

    for (int i = 0; i < tlb_count; i++) {
        int idx = (oldest + i) % TLB_SIZE;
        if (tlb[idx].page_number == page_to_replace) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        return 0;
    }

    for (int i = pos; i < tlb_count - 1; i++) {
        int from_idx = (oldest + i + 1) % TLB_SIZE;
        int to_idx = (oldest + i) % TLB_SIZE;
        tlb[to_idx] = tlb[from_idx];
    }

    int vacated_idx = (tlb_next_insert - 1 + TLB_SIZE) % TLB_SIZE;
    tlb[vacated_idx].page_number = -1;
    tlb[vacated_idx].frame_number = -1;

    tlb_next_insert = vacated_idx;
    tlb_count--;

    TLB_Add(new_page, new_frame);
    return 1;
}