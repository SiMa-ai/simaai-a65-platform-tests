//SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2022 Sima ai
 */

#include <errno.h>
#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <libgen.h>
#include <simaai/simaai_memory.h>

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
} load_task;

static int targets[] = {
		SIMAAI_MEM_TARGET_DMS0,
		SIMAAI_MEM_TARGET_DMS1,
		SIMAAI_MEM_TARGET_DMS2,
		SIMAAI_MEM_TARGET_DMS3,
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
		{ 0,        0,                 0,     0  }
	};
	const char usage[] =
		"Usage: %s [OPTIONS]\n"
		"Load DDR memory controllers with patterns.\n"
		"\n"
		"  -h, --help            Display this help and exit\n"
		"  -d, --ddrcmask=MASK   Hex mask of controllers to be tested, default: 0xf (all)\n"
		"  -b, --readback        Read test after populating buffer, default: no\n"
		"  -p, --pattern=[0..8]  Pattern to use for testing, default: random\n"
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
		"  -v, --value=VALUE     Hex value of 8-byte pattern to use for testing in case if user defined pattern, default: 0xA55AAA555AA555AA\n"
		"  -t, --time=TIME       Time to run test, if 0 - run forever, default: run forever\n"
		"  -s, --size=SIZE       Size of the buffer to use for test, default: 1048510\n"
		"  -w, --workers=THREADS Number of worker threads per DDRC, default: 1\n"
		"  -r, --random          Access to buffer not in sequential, but random order, default: no\n";
	int option_index;
	int c;

	while (1) {
		option_index = 0;
		c = getopt_long(argc, argv, "hd:p:v:t:s:w:rb", long_options, &option_index);

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
			if (args->ddrc_mask > 0xf) {
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
			args->size = strtoul(optarg, NULL, 10);
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
		default:
			fprintf(stderr, usage, basename(filename));
			return -1;
			break;
		}
	}

	return 0;
}

static void* loader_task(void *arg)
{
	volatile load_task *task = (volatile load_task *)arg;
	unsigned long int value, i, offset, dummy = 0;
	unsigned long int *addr;

	if(!task)
		return NULL;

	addr = (unsigned long int *)simaai_memory_map(task->buffer);
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
	case PATTERN_USER:
	default:
		value = task->value;
		break;
	}

	while(task->active) {
		for(i = 0; i < (task->size >> 8); i++) {
			if(task->random)
				offset = random() % (task->size >> 8);
			else
				offset = i;
			if(task->type == PATTERN_RANDOM)
				value = random();
			else if(task->type == PATTERN_ADDRESS)
				value = (long unsigned int)&(addr[offset]);
			addr[offset] = value;
		}
		simaai_memory_flush_cache(task->buffer);
		if(task->readback)
			break;
	}

	if(task->readback) {
		while(task->active) {
			simaai_memory_invalidate_cache(task->buffer);
			for(i = 0; i < (task->size >> 8); i++) {
				dummy += addr[offset];
			}
		}
	}

	simaai_memory_unmap(task->buffer);

	return NULL;
}

int main(int argc, char *argv[])
{
	args args = {
			.type = PATTERN_RANDOM,
			.size = 0x100000,
			.sleep_time = 0,
			.ddrc_mask = 0xf,
			.threads = 1,
			.random = 0,
			.value = 0xA55AAA555AA555AA,
			.readback = 0,
	};
	int i, j, k = 0, res, threads = 0;
	load_task *tasks;

	if (parse_args(argc, argv, &args) != 0){
		return EXIT_FAILURE;
	}

	//Calculate amount of thread
	for(i = 0; i < 4; i++)
		if((args.ddrc_mask >> i) & 1)
			threads += args.threads;

	//Allocate space for tasks DDRCs*workers
	tasks = (load_task *)calloc(threads, sizeof(*tasks));
	if(!tasks){
		fprintf(stderr, "Not enough memory for allocating tasks\n");
		return EXIT_FAILURE;
	}

	for(i = 0; i < 4; i++) {
		if((args.ddrc_mask >> i) & 1)
			for(j = 0; j < args.threads; j++) {
				//Allocate CMA buffer size per worker
				tasks[k].buffer = simaai_memory_alloc_flags(args.size, targets[i], SIMAAI_MEM_FLAG_CACHED);
				if(tasks[k].buffer == NULL)
					goto error;
				//Fill task structure
				tasks[k].active = 1;
				tasks[k].type = args.type;
				tasks[k].value = args.value;
				tasks[k].size = args.size;
				tasks[k].random = args.random;
				tasks[k].readback = args.readback;
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
