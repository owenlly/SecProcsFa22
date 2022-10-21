
#include "util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#define _GNU_SOURCE
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>

// TODO: define your own buffer size
#define BUFF_SIZE (1 << 21)

#define CACHE_LINE_SIZE 64
#define L2_SIZE 262144
#define L2_SET_NUM 512
#define L2_ASSOCIATIVITY 8
#define ENCODING_SET_NUM 256

int main(int argc, char **argv)
{
  // Allocate a buffer using huge page
  // See the handout for details about hugepage management
  uint8_t *evict_array = (uint8_t *)mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);

  extern int errno;
  if (evict_array == (void *)-1) {
    perror("mmap() error\n");
    printf("errno = %d\n", errno);
    exit(EXIT_FAILURE);
  }

  // The first access to a page triggers overhead associated with
  // page allocation, TLB insertion, etc.
  // Thus, we use a dummy write here to trigger page allocation
  // so later access will not suffer from such overhead.
  *evict_array = 1; // dummy write to trigger page allocation

  // TODO:
  // Put your covert channel setup code here
  printf("evict_array_start:%p\n", (void *)evict_array);

  //printf("Please type a message.\n");
  volatile uint64_t tmp;
  int secret_number = 0;
  bool sending = true;
  while (sending) {
    printf("Please type a message.\n");
    char text_buf[128];
    fgets(text_buf, sizeof(text_buf), stdin);
    //printf("sending %d\n", secret_number);
    // TODO:
    // Put your covert channel code here
    secret_number = string_to_int(text_buf);
    if (secret_number < 0 || secret_number > 255) {
      printf("Out of bound!\n");
      exit(0);
    }

    for (uint64_t i = 0; i < 40000000; i++) {
      for (int j = 0; j < L2_ASSOCIATIVITY; j++) {
        tmp = evict_array[secret_number * CACHE_LINE_SIZE + j * L2_SET_NUM * CACHE_LINE_SIZE];
      }
    }
    asm volatile("mfence\n\t"
    :::);	
  }

  printf("Sender finished.\n");
  return 0;
}
