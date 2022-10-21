
#include"util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#define _GNU_SOURCE
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>

#define CACHE_LINE_SIZE 64 
#define L2_SIZE 262144
#define L2_SET_NUM 512
#define L2_ASSOCIATIVITY 8
#define ENCODING_SET_NUM 256

#define BUFF_SIZE (1<<21)
#define SAMPLE 50

inline unsigned int __attribute__((always_inline)) rdtscp() {
	unsigned int time;
	asm volatile ("rdtscp"
	: "=a" (time));

	return time;
}

int main(int argc, char **argv)
{
	// Put your covert channel setup code here

	uint8_t *probe_array= (uint8_t *)mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
	extern int errno;
	if (probe_array == (uint8_t*) - 1) {
		perror("mmap() error\n");
		printf("errno = %d\n", errno); 
		exit(EXIT_FAILURE);
	}

	*probe_array = 1;

	volatile uint64_t tmp;
	uint64_t time[ENCODING_SET_NUM] = {0};
	uint32_t last_index = 0;
	uint64_t time0 = 0, time1 = 0, interval = 0;
	uint32_t miss_counter[ENCODING_SET_NUM] = {0};
	uint8_t sample_counter = 0;

	printf("Please press enter.\n");
	char text_buf[2];
	fgets(text_buf, sizeof(text_buf), stdin);

	//warm up
	for (int i=0; i<1000; i++) {
		for (int j=0; j < L2_SET_NUM; j++) {		
			for (int k=0; k<L2_ASSOCIATIVITY; k++) {
				asm volatile("mfence\n\t"
				:::);	
				tmp = probe_array[j*CACHE_LINE_SIZE + k*L2_SET_NUM*CACHE_LINE_SIZE];
			}
		}
	}

	printf("Receiver now listening.\n");

	bool listening = true;
	while (listening) {
		// Put your covert channel code here
		for (int i = 0; i < ENCODING_SET_NUM; i++) {
			time[i] = 0;
		}

		for (int q=0; q<SAMPLE; q++) {
			for (int i=0; i<ENCODING_SET_NUM; i++) {
				int mix_i = ((i * 167) + 13) & 255;
				//Prime
				for (int j=0; j<5; j++) {
					for (int k=0; k<L2_ASSOCIATIVITY; k++) {
						asm volatile("mfence\n\t"
						:::);	
						tmp = probe_array[mix_i*CACHE_LINE_SIZE + k*L2_SET_NUM*CACHE_LINE_SIZE];
					}
				}
						
				//Probe
				for (int j=L2_ASSOCIATIVITY-1; j>=0; j--) {
					asm volatile("mfence\n\t"
					:::);	
					time[mix_i] += measure_one_block_access_time((uint64_t)&probe_array[mix_i*CACHE_LINE_SIZE + j*L2_SET_NUM*CACHE_LINE_SIZE]);
				}
			}
		}

		bool sample_count_flag = 0;
		for (int i=0; i<ENCODING_SET_NUM; i++) {
			time[i] = time[i] / SAMPLE;
			if (time[i] > 390) {
				sample_count_flag = 1;
				miss_counter[i]++;
			}
		}

		if (sample_count_flag) {
			sample_counter++;
		}
		if (sample_counter == 10) {
			sample_counter = 0;
			uint32_t max_index = 0, max_count = 0;
			time1 = rdtscp();
			interval = time1 - time0;
			time0 = time1;
			for (int i=0; i<ENCODING_SET_NUM; i++) {
				if ((i!=last_index || (i==last_index && interval>100000000000))&& miss_counter[i]>max_count) {
					max_count = miss_counter[i];
					max_index = i;
				}
				miss_counter[i] = 0;
			}
			if (max_count > 3) {
				printf("Receive %d! Count %d times\n", max_index, max_count);
				last_index = max_index;
			}
		}
		
	}

	printf("Receiver finished.\n");
	return 0;

}


