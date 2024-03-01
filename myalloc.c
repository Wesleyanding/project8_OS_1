#include <stdio.h>
#include <sys/mman.h>

#define ALIGNMENT 16   // Must be power of 2
#define GET_PAD(x) ((ALIGNMENT - 1) - (((x) - 1) & (ALIGNMENT - 1)))
#define PADDED_SIZE(x) ((x) + GET_PAD(x))

// Macro for pointer arithmetic
#define PTR_OFFSET(p, offset) ((void*)((char *)(p) + (offset)))

struct block {
    int size;            // Size in bytes
    int in_use;          // Boolean
    struct block *next;
};

void *heap = NULL; // Global variable to hold the start of the heap

void *myalloc(int size) {
    // One-off initialization
    if (heap == NULL) {
        heap = mmap(NULL, 1024, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
        if (heap == MAP_FAILED) {
            perror("Error mapping memory");
            return NULL;
        }
        // Initialize the first block
        struct block *head = (struct block *)heap;
        head->size = 1024 - PADDED_SIZE(sizeof(struct block));
        head->in_use = 0;
        head->next = NULL;
    }

    // Walk the linked list to find a free block big enough
    struct block *curr = (struct block *)heap;
    while (curr != NULL) {
        if (!curr->in_use && curr->size >= PADDED_SIZE(size)) {
            // check if splitting is necessary
            int remaining_size = curr->size - PADDED_SIZE(size) - PADDED_SIZE(sizeof(struct block));
            if ((unsigned long)remaining_size >= 16 + 2 * PADDED_SIZE(sizeof(struct block))) {
                struct block *new_block = (struct block *)((char *)curr + PADDED_SIZE(sizeof(struct block)) + PADDED_SIZE(size));
                new_block->size = remaining_size;
                new_block->in_use = 0;
                new_block->next = curr->next;
                curr->size = PADDED_SIZE(size);
                curr->in_use = 1;
                curr->next = new_block;
            } else {
                curr->in_use = 1;
            }
            // Return pointer to the data area
            return (void *)(curr + 1);
        }
        curr = curr->next;
    }
    // No free block big enough
    return NULL;
}

void myfree(void *p) {
    struct block *blk = (struct block *)((char *)p - PADDED_SIZE(sizeof(struct block)));
    blk->in_use = 0;
        struct block *curr = heap;
        while (curr->next != NULL) {
            if ((curr->in_use == 0) && (curr->next->in_use == 0)) {
                curr->size += PADDED_SIZE(sizeof(struct block)) + (curr->next)->size;
                curr->next = curr->next->next;
            } else {
                curr = curr->next;
            }
        }
}

// Printing the linked list of blocks
void print_data(void) {
    struct block *b = (struct block *)heap;

    if (b == NULL) {
        printf("[empty]\n");
        return;
    }

    while (b != NULL) {
        // Uncomment the following line if you want to see the pointer values
        //printf("[%p:%d,%s]", b, b->size, b->in_use? "used": "free");
        printf("[%d,%s]", b->size, b->in_use? "used": "free");
        if (b->next != NULL) {
            printf(" -> ");
        }

        b = b->next;
    }
    printf("\n");
}

int main() {

    void *p, *q;

    p = myalloc(10); print_data();
    q = myalloc(20); print_data();

    myfree(p); print_data();
    myfree(q); print_data();

    
    return 0;
}
