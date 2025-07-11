//SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2022 Sima ai
 */

#include <errno.h>
#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <libgen.h>
#include <simaai/simaai_memory.h>

unsigned long int modify_byte(unsigned long int value, int index, unsigned char new_byte);
bool check_adjacent_bytes(unsigned long int value, unsigned long int modified, int index);

typedef enum {
	PATTERN_55,
	PATTERN_AA,
	PATTERN_5A,
	PATTERN_A5,
	PATTERN_55AA,
	PATTERN_AA55,
	PATTERN_RANDOM,
	PATTERN_ADDRESS,
	PATTERN_USER,
	PATTERN_WALKING_1,
	PATTERN_WALKING_0,
	PATTERN_CHECK_ADJACENT,
	PATTERN_NUM,
} pattern_type;

typedef struct {
	pattern_type type;
	unsigned long int value;
	unsigned long int size;
	unsigned int sleep_time;
	unsigned int ddrc_mask;
	unsigned int threads;
	int random;
	int readback;
	int performance;
} args;

typedef struct {
	pthread_t thread;
	volatile int active;
	simaai_memory_t * buffer;
	pattern_type type;
	unsigned long int value;
	unsigned long int size;
	int random;
	int readback;
	int performance;
	unsigned int sleep_time;
} load_task;

static int targets[] = {
		SIMAAI_MEM_TARGET_DMS0,
		SIMAAI_MEM_TARGET_DMS1,
		SIMAAI_MEM_TARGET_DMS2,
		SIMAAI_MEM_TARGET_DMS3,
		SIMAAI_MEM_TARGET_OCM,
};

static int parse_args(const int argc, char *const argv[], args *args)
{
	char *filename = argv[0];
	struct option long_options[] = {
		{ "help",     no_argument,       NULL, 'h' },
		{ "ddrcmask", required_argument, NULL, 'd' },
		{ "readback", no_argument,       NULL, 'b' },
		{ "pattern",  required_argument, NULL, 'p' },
		{ "value",    required_argument, NULL, 'v' },
		{ "time",     required_argument, NULL, 't' },
		{ "size",     required_argument, NULL, 's' },
		{ "threads",  required_argument, NULL, 't' },
		{ "random",   no_argument,       NULL, 'r' },
		{ "performance", no_argument,    NULL, 'f' },
		{ 0,        0,                 0,     0  }
	};
	const char usage[] =
		"Usage: %s [OPTIONS]\n"
		"Load DDR memory controllers with patterns.\n"
		"\n"
		"  -h, --help            Display this help and exit\n"
		"  -d, --ddrcmask=MASK   Hex mask of controllers to be tested, default: 0xf (all)\n"
		"  -b, --readback        Read test after populating buffer, default: no\n"
		"  -p, --pattern=[0..11]  Pattern to use for testing, default: random\n"
		"                        Possible options:\n"
		"                            0 - 0x55\n"
		"                            1 - 0xAA\n"
		"                            2 - 0x5A\n"
		"                            3 - 0xA5\n"
		"                            4 - 0x55AA\n"
		"                            5 - 0xAA55\n"
		"                            6 - random\n"
		"                            7 - 8-byte address\n"
		"                            8 - 8-byte user defined\n"
		"                            9 - Walking 1's - 0x8040201008040201\n"
		"                            10 - Walking 0's - 0x7FBFDFEFF7FBFDFE\n"
		"                            11 - Checking adjacent bits upon modifying 2 bits in between\n"
		"  -v, --value=VALUE     Hex value of 8-byte pattern to use for testing in case if user defined pattern, default: 0xA55AAA555AA555AA\n"
		"  -t, --time=TIME       Time to run test, if 0 - run forever, default: run forever\n"
		"  -s, --size=SIZE       Size of the buffer to use for test, default: 0x100000\n"
		"  -w, --workers=THREADS Number of worker threads per DDRC, default: 1\n"
		"  -r, --random          Access to buffer not in sequential, but random order, default: no\n"
		"  -f, --performance     prints bandwidth number of bytes per second default:no\n";
	int option_index;
	int c;

	while (1) {
		option_index = 0;
		c = getopt_long(argc, argv, "hd:p:v:t:s:w:rbf", long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 0:
			/* getopt_long already set a value of option */
			break;
		case 'h':
			fprintf(stderr, usage, basename(filename));
			return -1;
			break;
		case 'd':
			args->ddrc_mask = strtol(optarg, NULL, 16);
			if (args->ddrc_mask > 0x1f) {
				fprintf(stderr, "Invalid DDRC mask\n");
				return -1;
			}
			break;
		case 'p':
			args->type = (pattern_type)strtol(optarg, NULL, 10);
			if (args->type > PATTERN_NUM) {
				fprintf(stderr, "Invalid pattern type\n");
				return -1;
			}
			break;
		case 'v':
			args->value = strtoul(optarg, NULL, 16);
			break;
		case 't':
			args->sleep_time = strtoul(optarg, NULL, 10);
			break;
		case 's':
			args->size = strtoul(optarg, NULL, 16);
			break;
		case 'w':
			args->threads = strtoul(optarg, NULL, 10);
			break;
		case 'r':
			args->random = 1;
			break;
		case 'b':
			args->readback = 1;
			break;
		case 'f':
			args->performance = 1;
			break;
		default:
			fprintf(stderr, usage, basename(filename));
			return -1;
			break;
		}
	}

	return 0;
}
unsigned long int modify_byte(unsigned long int value, int index, unsigned char new_byte) {
	unsigned long int mask = 0xFFUL << (index * 8);
	value&=~mask;
	value|=((unsigned long int)new_byte << (index * 8));
	return value;
}
bool check_adjacent_bytes(unsigned long int value, unsigned long int modified, int index) {
	unsigned long int mask_left = 0xFFUL << ((index+1) * 8);
	unsigned long int mask_right = 0xFFUL >> ((7-index) * 8);

	if(((value&mask_left)!=(modified&mask_left))&&((value&mask_right)!=(modified&mask_right)))
		return false;
	else
		return true;
}
static void* loader_task(void *arg)
{
	volatile load_task *task = (volatile load_task *)arg;
	unsigned long int value, i, offset, dummy = 0, err_count = 0;
	unsigned long int *addr;
	unsigned long int bytes_count = 0;
	struct timespec start, current;
	double elapsed_time;

	if(!task)
		return NULL;
	addr = (unsigned long int *)simaai_memory_map(task->buffer);
	if (addr == NULL) {
		fprintf(stderr, "Memory mapping failed\n");
		return NULL;
	}

	switch(task->type) {
	case PATTERN_55:
		value = 0x5555555555555555;
		break;
	case PATTERN_AA:
		value = 0xAAAAAAAAAAAAAAAA;
		break;
	case PATTERN_5A:
		value = 0x5A5A5A5A5A5A5A5A;
		break;
	case PATTERN_A5:
		value = 0xA5A5A5A5A5A5A5A5;
		break;
	case PATTERN_55AA:
		value = 0x55AA55AA55AA55AA;
		break;
	case PATTERN_AA55:
		value = 0xAA55AA55AA55AA55;
		break;
	case PATTERN_WALKING_1:
		for (i = 0; i < (task->size); i++){
			for (int bit = 0; bit < 8; bit++) {
				unsigned char val = (1 << (7-bit));
				((volatile char *)addr)[i] = val;
				unsigned char read = ((volatile char *)addr)[i];
				if(read != val){
					fprintf(stderr, "Data mismatch in Walking 1\n");
				}
			}
		}
		break;
	case PATTERN_WALKING_0:
		for (i = 0; i < (task->size); i++){
			for (int bit = 0; bit < 8; bit++) {
				unsigned char val = ~(1 << (7-bit));
				((volatile char *)addr)[i] = val;
				unsigned char read = ((volatile char *)addr)[i];
				if(read != val){
					fprintf(stderr, "Data mismatch in Walking 0\n");
				}
			}
		}
		break;
	case PATTERN_CHECK_ADJACENT:
	case PATTERN_USER:
	default:
		value = task->value;
		break;
	}

	static int target = SIMAAI_MEM_TARGET_DMS0;

	simaai_memory_t *input_buffer = simaai_memory_alloc_flags(task->size, target, SIMAAI_MEM_FLAG_CACHED);
    if (!input_buffer) {
        perror("Input buffer allocation failed");
    }
    void *input_addr = simaai_memory_map(input_buffer);
    if (!input_addr) {
        perror("Input buffer mapping failed");
        simaai_memory_free(input_buffer);
    }

	memset(input_addr, 0xAA, task->size);

	double time_diff(struct timespec *start, struct timespec *end) {
    	return (end->tv_sec - start->tv_sec) + (end->tv_nsec - start->tv_nsec) / 1e9;
	}
	clock_gettime(CLOCK_MONOTONIC, &start);

	while(task->active) {
		if (task->performance){
			memcpy(addr, input_addr, task->size);
			clock_gettime(CLOCK_MONOTONIC, &current);
			elapsed_time = time_diff(&start, &current);			
			if(elapsed_time >= (double)(task->sleep_time)) {
				task->active=0;
				break;
			} else {
				bytes_count += (task->size / (1024*1024));
			}
		} 
		else {
			for(i = 0; (i < (task->size >> 8)); i++) {
				if(task->random)
					offset = random() % (task->size >> 8);
				else
					offset = i;

				if(task->type == PATTERN_RANDOM)
					value = random();
				else if(task->type == PATTERN_ADDRESS)
					value = (long unsigned int)&(addr[offset]);
				else if(task->type == PATTERN_CHECK_ADJACENT){
					int index = 3;
					unsigned char new_byte = 0x55;
					unsigned long int modified = modify_byte(value, index, new_byte); 
					if (!check_adjacent_bytes(value, modified, index)) {
						err_count++;
						break;
					}
				}
				else 
					addr[offset] = value;
			}
			task->active = 0;
		}
		simaai_memory_flush_cache(task->buffer);
		if(task->readback)
			break;
	}

	fprintf(stderr, "Bytes Count (MB): %ld\n", bytes_count);
	fprintf(stderr, "Elapsed Time: %.2fs\n", elapsed_time);
	
	if (task->performance)
			fprintf(stderr, "Throughput: %.2fGB/s\n", ((double) bytes_count/(1024*elapsed_time)));
		else
			fprintf(stderr, "Pattern Loaded\n");

	if (err_count > 0)
		fprintf(stderr, "ERROR: Adjacent bits disturbed %lu\n", err_count);

	if(task->readback) {
		task->active = 1;
		while(task->active) {
			simaai_memory_invalidate_cache(task->buffer);
			for(i = 0; i < (task->size >> 8); i++) {
				dummy += addr[offset];
			}
			task->active = 0;
		}
	}
	simaai_memory_unmap(task->buffer);
	exit(EXIT_SUCCESS);

	return NULL;
}

int main(int argc, char *argv[])
{
	args args = {
			.type = PATTERN_RANDOM,
			.size = 0x100000,
			.sleep_time = 0,
			.ddrc_mask = 0x1,
			.threads = 1,
			.random = 0,
			.value = 0xA55AAA555AA555AA,
			.readback = 0,
			.performance  = 0,
	};
	int i, j, k = 0, res, threads = 0;
	load_task *tasks;

	if (parse_args(argc, argv, &args) != 0){
		return EXIT_FAILURE;
	}

	//Calculate amount of thread
	for(i = 0; i < 5; i++)
		if((args.ddrc_mask >> i) & 1)
			threads += args.threads;

	//Allocate space for tasks DDRCs*workers
	tasks = (load_task *)calloc(threads, sizeof(*tasks));
	if(!tasks){
		fprintf(stderr, "Not enough memory for allocating tasks\n");
		return EXIT_FAILURE;
	}

	for(i = 0; i < 5; i++) {
		if((args.ddrc_mask >> i) & 1)
			for(j = 0; j < args.threads; j++) {
				//Allocate CMA buffer size per worker
				if(args.type > 8 || args.readback){
					tasks[k].buffer = simaai_memory_alloc_flags(args.size, targets[i],SIMAAI_MEM_FLAG_DEFAULT);
				}
				else
					tasks[k].buffer = simaai_memory_alloc_flags(args.size, targets[i], SIMAAI_MEM_FLAG_CACHED);
				
				if(tasks[k].buffer == NULL){
					fprintf(stderr, "ERROR: Buffer is NULL\n");
					goto error;
				}
				//Fill task structure
				tasks[k].active = 1;
				tasks[k].type = args.type;
				tasks[k].value = args.value;
				tasks[k].size = args.size;
				tasks[k].random = args.random;
				tasks[k].readback = args.readback;
				tasks[k].performance = args.performance;
				tasks[k].sleep_time = args.sleep_time;
				//start thread
				res = pthread_create(&(tasks[k].thread), NULL, &loader_task, &(tasks[k]));
				if(res != 0)
					goto error;
				k++;
			}
	}

	//Sleep for n seconds or forever
	if(args.sleep_time == 0)
		while(1);
	else
		sleep(args.sleep_time);

error:
	//Set active to 0
	for(i = 0; i < threads; i++)
		tasks[i].active = 0;

	//Join all threads
	for(i = 0; i < threads; i++) {
		if(tasks[i].thread != 0) {
			res = pthread_join(tasks[i].thread, NULL);
			if (res != 0)
				fprintf(stderr, "Error joining the thread\n");
		}
	}

	//Free CMA buffers
	for(i = 0; i < threads; i++) {
		if(tasks[i].buffer != NULL)
			simaai_memory_free(tasks[i].buffer);
	}

	//Free tasks memory
	free(tasks);

	return res == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
