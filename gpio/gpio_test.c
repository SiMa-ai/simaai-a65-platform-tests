#include <linux/gpio.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define PORTA "/dev/gpiochip0"
#define PORTB "/dev/gpiochip1"
#define PORTC "/dev/gpiochip2"
#define PORTD "/dev/gpiochip3"

#define GPIO_PAD_REGISTER 			0x719000
#define GPIOS_PER_PAD_REGISTER		4
#define DEFAULT_PAD_REGISTER_VALUE	0x2040810

#define GPIO_0_PULL_UP_BIT			1
#define GPIO_1_PULL_UP_BIT			7
#define GPIO_2_PULL_UP_BIT			14
#define GPIO_3_PULL_UP_BIT			21

#define GPIO_0_PULL_DOWN_BIT			0
#define GPIO_1_PULL_DOWN_BIT			8
#define GPIO_2_PULL_DOWN_BIT			16
#define GPIO_3_PULL_DOWN_BIT			24

#define GPIO_0_PULL_UP_BIT_MASK		(1 << GPIO_0_PULL_UP_BIT)
#define GPIO_1_PULL_UP_BIT_MASK		(1 << GPIO_1_PULL_UP_BIT)
#define GPIO_2_PULL_UP_BIT_MASK		(1 << GPIO_2_PULL_UP_BIT)
#define GPIO_3_PULL_UP_BIT_MASK		(1 << GPIO_3_PULL_UP_BIT)

#define GPIO_0_PULL_DOWN_BIT_MASK	(1 << GPIO_0_PULL_DOWN_BIT)
#define GPIO_1_PULL_DOWN_BIT_MASK	(1 << GPIO_1_PULL_DOWN_BIT)
#define GPIO_2_PULL_DOWN_BIT_MASK	(1 << GPIO_2_PULL_DOWN_BIT)
#define GPIO_3_PULL_DOWN_BIT_MASK	(1 << GPIO_3_PULL_DOWN_BIT)

#define DEVMEM_STR "devmem2 0x%x %c 0x%x"


char *port_paths[] = {
	PORTA,
	PORTB,
	PORTC,
	PORTD
};

#define TOTAL_NUM_PORTS (sizeof(port_paths)/sizeof(port_paths[0]))
#define MAX_GPIOS_PER_PORT 	8
#define MAX_GPIOS			32

int port_pull_up_regs_bit_mask[] = {
	GPIO_0_PULL_UP_BIT_MASK,
	GPIO_1_PULL_UP_BIT_MASK,
	GPIO_2_PULL_UP_BIT_MASK,
	GPIO_3_PULL_UP_BIT_MASK
};

int port_pull_down_regs_bit_mask[] = {
	GPIO_0_PULL_DOWN_BIT_MASK,
	GPIO_1_PULL_DOWN_BIT_MASK,
	GPIO_2_PULL_DOWN_BIT_MASK,
	GPIO_3_PULL_DOWN_BIT_MASK
};

/*
** Print port information
*/

void print_port_info(char *port_path) {

	int fd, rv;
	struct gpiochip_info chip_info;

	fd = open(port_path, O_RDWR);
	if(fd == -1) {
		printf("ERROR : opening port %s, errno: %d\n",port_path, errno);
		return;
	}

	rv = ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &chip_info);
	if(rv == -1) {
		printf("ERROR : ioctl GPIO_GET_CHIPINFO failed errno:%d\n",errno);
		return;
	}

	printf("name : %s, label : %s, lines %u\n", chip_info.name, chip_info.label,
					chip_info.lines);

	close(fd);
}

/*
** Toggles all LEDS given in the array led_arr
*/
void toggle_gpio(char *port_path, unsigned int *led_arr,
					unsigned int arr_size, int toggle_count) {

	struct gpiohandle_request req;
	struct gpiohandle_data data;
	int fd, rv;
	unsigned int iter = 0;

	if(toggle_count <= 0) {
		printf("ERROR : provide valid toggle count %d\n", toggle_count);
		return;
	}

	fd = open(port_path, O_RDWR);
	if(fd == -1) {
		printf("ERROR : opening port %s, errno:%d\n",port_path, errno);
		return;
	}

	for(iter = 0; iter < arr_size; iter++) {
		req.lineoffsets[iter] = led_arr[iter];
		req.default_values[iter] = 0;
	}

	req.lines = arr_size;
	req.flags = GPIOHANDLE_REQUEST_OUTPUT;
	strncpy(req.consumer_label,"LED_TOGGLER",12);

	rv = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &req);
	if(rv == -1) {
		printf("ERROR : ioctl GPIO_GET_LINEHANDLE_IOCTL failed errno:%d\n",errno);
		return;
	}

	while ((--toggle_count) >= 0 ) {

		for(iter = 0; iter < arr_size; iter++) {
			data.values[iter] = !data.values[iter];
		}

		rv = ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
		if(rv == -1) {
			printf("ERROR : ioctl GPIOHANDLE_SET_LINE_VALUES_IOCTL failed errno:%d\n",errno);
			return;
		}
		sleep(1);
	}
	close(req.fd);
	close(fd);
}

void toggle_all_gpios(int toggle_count) {

	unsigned int iter = 0;
	unsigned int gpio_arr[] = {0,1,2,3,4,5,6,7};

	if(toggle_count <= 0) {
		printf("ERROR : provide valid toggle count %d\n", toggle_count);
		return ;
	}

	while((--toggle_count) >= 0) {
		for(iter = 0; iter < TOTAL_NUM_PORTS; iter++) {
			printf("INFO : toggling port %c\n", 'a' + iter);
			toggle_gpio(port_paths[iter], gpio_arr, 8, 2);
		}
	}
}

void print_binary_pattern(char *port_path, unsigned int *led_arr, unsigned int arr_size,
							unsigned int num) {

	struct gpiohandle_request req;
	struct gpiohandle_data data;
	int fd, rv;
	unsigned int iter = 0;
	unsigned int pattern = 0;

	printf("INFO : printing binary pattern for number %u\n", num);

	fd = open(port_path, O_RDWR);
	if(fd == -1) {
		printf("ERROR : opening port %s, errno:%d\n",port_path, errno);
		return;
	}

	for(iter = 0; iter < arr_size; iter++) {
		req.lineoffsets[iter] = led_arr[iter];
		req.default_values[iter] = 0;
	}

	req.lines = arr_size;
	req.flags = GPIOHANDLE_REQUEST_OUTPUT;
	strncpy(req.consumer_label,"LED_BINARY_PATTERN",19);

	rv = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &req);
	if(rv == -1) {
		printf("ERROR : ioctl GPIO_GET_LINEHANDLE_IOCTL failed errno:%d\n",errno);
		return;
	}

	while (pattern != num) {

		for(iter = 0; iter <arr_size; iter++) {
			data.values[iter] = pattern & (1UL << iter);
		}

		rv = ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
		if(rv == -1) {
			printf("ERROR : ioctl GPIOHANDLE_SET_LINE_VALUES_IOCTL failed errno:%d\n",errno);
			return;
		}

		pattern++;
		sleep(1);
	}
	close(req.fd);
	close(fd);
}

#if 0
void overnight_test() {

	unsigned int num = 0;
	struct gpiohandle_request req[TOTAL_NUM_PORTS];
	struct gpiohandle_data set_data, get_data;
	int fd[TOTAL_NUM_PORTS], rv;
	unsigned int iter = 0;
	unsigned int pattern = 0;

	printf("============ RUNING OVERNIGHT TEST ============\n");

	for(iter = 0; iter < TOTAL_NUM_PORTS; iter++) {
		unsigned int gpio_iter = 0;

		fd[iter] = open(port_paths[iter], O_RDWR);
		if(fd[iter] == -1) {
			printf("ERROR : opening port %s, errno:%d\n", port_paths[iter],
							errno);
			return;
		}

		//Setting all GPIOs of a ports to output mode
		for(gpio_iter = 0; gpio_iter < MAX_GPIOS_PER_PORT; gpio_iter++) {
			req[iter].lineoffsets[gpio_iter] = gpio_iter;
			req[iter].default_values[gpio_iter] = 0;
		}

		req[iter].lines = MAX_GPIOS_PER_PORT;
		req[iter].flags = GPIOHANDLE_REQUEST_OUTPUT;
		snprintf(req[iter].consumer_label, 22, "OVERNIGHT_TEST_PORT_%c",
							('A' + iter));

		rv = ioctl(fd[iter], GPIO_GET_LINEHANDLE_IOCTL, &(req[iter]));
		if(rv == -1) {
			printf("ERROR : ioctl GPIO_GET_LINEHANDLE_IOCTL failed for port %s "
					"errno:%d\n", port_paths[iter], errno);
			return;
		}
	}

	while(pattern != 0x100) {

		printf("============ PATTERN %d ============ \n", pattern);
		for(iter = 0; iter < TOTAL_NUM_PORTS; iter++) {
			unsigned int gpio_iter;

			for(gpio_iter = 0; gpio_iter < MAX_GPIOS_PER_PORT; gpio_iter++) {
				set_data.values[gpio_iter] = pattern & (1UL << gpio_iter);
				printf("gpio : %d, set data : %d\n", gpio_iter,
								set_data.values[gpio_iter]);
			}

			rv = ioctl(req[iter].fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &set_data);
			if(rv == -1) {
				printf("ERROR : ioctl GPIOHANDLE_SET_LINE_VALUES_IOCTL failed for "
							"iter %d errno:%d\n",iter, errno);
				return;
			}

			rv = ioctl(req[iter].fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &get_data);
			if(rv == -1) {
				printf("ERROR : ioctl GPIOHANDLE_GET_LINE_VALUES_IOCTL failed for "
							"iter %d errno:%d\n",iter, errno);
				return;
			}

			for(gpio_iter = 0; gpio_iter < MAX_GPIOS_PER_PORT; gpio_iter++) {
				if((__u8)(set_data.values[gpio_iter]) !=
								(__u8)(get_data.values[gpio_iter])) {
					printf("Gpio : %d, set data : %u, get data : %u\n",
								gpio_iter, (__u8)(set_data.values[gpio_iter]),
								(__u8)(get_data.values[gpio_iter]));
					printf("PORT_%c : FAILED\n", ('A' + iter));
					break;
				}
			}

			if(gpio_iter == MAX_GPIOS_PER_PORT) {
				printf("PORT_%c : PASSED\n", ('A' + iter));
			}
		}
		pattern++;
	}

	for(iter = 0; iter < TOTAL_NUM_PORTS ;  iter++) {
		close(req[iter].fd);
		close(fd[iter]);
	}
}
#endif

void get_chip_and_gpio_number(unsigned int *chip_number,
				unsigned int *gpio_number) {

	if((!chip_number) || (!gpio_number)) {
		printf("ERROR : invalid input parameters\n");
		return;
	}

	while(1) {
		printf("Enter the chip or port number\n");
		if(scanf("%u",chip_number) == 1) {
			if(((*chip_number) < 0) || ((*chip_number) > 3)) {
				printf("ERROR : invalid chip number, valid range 0 to 3\n");
				continue;
			}
			printf("Enter the gpio number for chip %u\n",*chip_number);
			if(scanf("%u", gpio_number) == 1) {
				if(((*gpio_number) < 0) || ((*gpio_number) > 7)) {
					printf("ERROR : invalid port number, valid range 0 to 7\n");
					continue;
				}
				//printf("Reading port %u from chip %u\n", *gpio_number,
				//					*chip_number);
				break;
			} else {
				printf("invalid gpio number\n");
				continue;
			}
		} else {
			printf("invalid chip number\n");
			continue;
		}
	} //while(1) ends
}

unsigned int
__read_gpio_port(unsigned int chip_number, unsigned int gpio_number) {

	struct gpiohandle_request req;
	struct gpiohandle_data data;
	int fd, rv;

	fd = open(port_paths[chip_number], O_RDWR);
	if(fd == -1) {
		printf("ERROR : opening port %s, errno:%d\n",port_paths[chip_number],
						errno);
		return fd;
	}

	req.lineoffsets[0] = gpio_number;
	//req.default_values[0] = 0;

	req.lines = 1;
	req.flags = GPIOHANDLE_REQUEST_INPUT;
	strncpy(req.consumer_label,"GPIO_READ_PORT",15);

	rv = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &req);
	if(rv == -1) {
		printf("ERROR : ioctl GPIO_GET_LINEHANDLE_IOCTL failed errno:%d\n",errno);
		return rv;
	}

	rv = ioctl(req.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
	if(rv == -1) {
		printf("ERROR : ioctl GPIOHANDLE_GET_LINE_VALUES_IOCTL failed errno:%d\n",errno);
		return rv;
	}

	close(req.fd);
	close(fd);

	return (data.values[0]);
}

void read_gpio_port() {

	unsigned int chip_number = -1;
	unsigned int gpio_number = -1;
	int read_val = -1;

	get_chip_and_gpio_number(&chip_number, &gpio_number);
	printf("INFO : reading gpio %u from chip %u\n", gpio_number, chip_number);

	read_val = __read_gpio_port(chip_number, gpio_number);
	if(read_val == -1) {
		printf("ERROR: reading value\n");
		return;
	}

	printf("INFO : value is %u\n",read_val);

}

int __write_gpio_port(unsigned int chip_number, unsigned int gpio_number,
						unsigned int val) {

	struct gpiohandle_request req;
	struct gpiohandle_data data;
	int fd, rv;

	fd = open(port_paths[chip_number], O_RDWR);
	if(fd == -1) {
		printf("ERROR : opening port %s, errno:%d\n",port_paths[chip_number],
						errno);
		return fd;
	}

	req.lineoffsets[0] = gpio_number;
	//req.default_values[0] = 0;

	req.lines = 1;
	req.flags = GPIOHANDLE_REQUEST_OUTPUT;
	strncpy(req.consumer_label,"GPIO_WRITE_PORT",16);

	rv = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &req);
	if(rv == -1) {
		printf("ERROR : ioctl GPIO_GET_LINEHANDLE_IOCTL failed errno:%d\n",errno);
		return rv;
	}

	data.values[0] = val;
	rv = ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
	if(rv == -1) {
		printf("ERROR : ioctl GPIOHANDLE_GET_LINE_VALUES_IOCTL failed errno:%d\n",errno);
		return rv;
	}

	close(req.fd);
	close(fd);

	return 0;

}

void write_gpio_port() {

	unsigned int chip_number = -1;
	unsigned int gpio_number = -1;
	unsigned int val;
	int rc = -1;
	get_chip_and_gpio_number(&chip_number, &gpio_number);
	printf("Enter the value to write, valid values 0 or 1\n");
	if(scanf("%u",&val) != 1) {
		printf("ERROR : invalid value try again!!!\n");
		return;
	}
	printf("INFO : writing %u to gpio %u from chip %u\n", val, gpio_number,
						chip_number);

	rc = __write_gpio_port(chip_number, gpio_number, val);
	if(rc == -1) {
		printf("ERROR :  writing to gpio\n");
		return;
	}

	printf("SUCCESS : set %u to gpio %u\n", val, gpio_number);

}

/*
** PAD register starts at 0x719000
** PORT A pad registers are 0x719000 and 0x719004
** PORT B pad registers are 0x719008 and 0x71900c
** PORT C pad registers are 0x719010 and 0x719014
** PORT D pad registers are 0x719018 and 0x71901c
*/
void pull_up_gpio_port () {

	unsigned int chip_number = -1;
	unsigned int gpio_number = -1;
	int ret = 0;
	char cmd[100];

	get_chip_and_gpio_number(&chip_number, &gpio_number);
	printf("INFO : Pulling up gpio %u from chip %u\n", gpio_number, chip_number);

	ret = sprintf(cmd, DEVMEM_STR,
			GPIO_PAD_REGISTER + (chip_number*MAX_GPIOS_PER_PORT) +
			((gpio_number/GPIOS_PER_PAD_REGISTER)*GPIOS_PER_PAD_REGISTER), 'w',
			(port_pull_up_regs_bit_mask[gpio_number%GPIOS_PER_PAD_REGISTER] |
					DEFAULT_PAD_REGISTER_VALUE));
	if(ret <= 0) {
		printf("ERROR : command is not valid\n");
		return;
	}

	printf("INFO : command = %s\n", cmd);

	ret = system(cmd);
	printf("INFO : ret %d\n",ret);

}

void pull_down_gpio_port () {

	unsigned int chip_number = -1;
	unsigned int gpio_number = -1;
	int ret = 0;
	char cmd[100];

	get_chip_and_gpio_number(&chip_number, &gpio_number);
	printf("INFO : Pulling down gpio %u from chip %u\n", gpio_number, chip_number);

	ret = sprintf(cmd, DEVMEM_STR,
			GPIO_PAD_REGISTER + (chip_number*MAX_GPIOS_PER_PORT) +
			((gpio_number/GPIOS_PER_PAD_REGISTER)*GPIOS_PER_PAD_REGISTER), 'w',
			(port_pull_down_regs_bit_mask[gpio_number%GPIOS_PER_PAD_REGISTER] |
					DEFAULT_PAD_REGISTER_VALUE));
	if(ret <= 0) {
		printf("ERROR : command is not valid\n");
		return;
	}

	printf("INFO : command = %s\n", cmd);

	ret = system(cmd);
	printf("INFO : ret %d\n",ret);
}

void __loopback_test(unsigned int source_chip_number,
						unsigned int source_gpio_number,
						unsigned int sink_chip_number,
						unsigned int sink_gpio_number) {

	int write_val = 0;;
	int read_val = -1;

	printf("===== %d,%d <-------> %u,%u =====\n",
									source_chip_number, source_gpio_number,
									sink_chip_number, sink_gpio_number);
	printf("INFO : Writing %u to source loopback\n", write_val);
	__write_gpio_port(source_chip_number, source_gpio_number, write_val);
	read_val = __read_gpio_port(sink_chip_number, sink_gpio_number);
	if(read_val != write_val) {
		printf("ERROR: XXXXXXXXXXXXXXXXXXX FAIL XXXXXXXXXXXXXXXXXXXXX\n");
		printf("written val is %u, read val is %u\n", write_val, read_val);
		exit(0);
	}

	printf("INFO : Writing %u to sink loopback\n", write_val);
	__write_gpio_port(sink_chip_number, sink_gpio_number, write_val);
	read_val = __read_gpio_port(source_chip_number, source_gpio_number);
	if(read_val != write_val) {
		printf("ERROR : XXXXXXXXXXXXXXXXXXX FAIL XXXXXXXXXXXXXXXXXXXXX\n");
		printf("written val is %u, read val is %u\n", write_val, read_val);
		exit(0);
	}

	write_val = write_val ^ 0x1;
	printf("INFO : Writing %u to source loopback\n", write_val);
	__write_gpio_port(source_chip_number, source_gpio_number, write_val);
	read_val = __read_gpio_port(sink_chip_number, sink_gpio_number);
	if(read_val != write_val) {
		printf("ERROR: XXXXXXXXXXXXXXXXXXX FAIL XXXXXXXXXXXXXXXXXXXXX\n");
		printf("written val is %u, read val is %u\n", write_val, read_val);
		exit(0);
	}

	printf("INFO : Writing %u to sink loopback\n", write_val);
	__write_gpio_port(sink_chip_number, sink_gpio_number, write_val);
	read_val = __read_gpio_port(source_chip_number, source_gpio_number);
	if(read_val != write_val) {
		printf("ERROR : XXXXXXXXXXXXXXXXXXX FAIL XXXXXXXXXXXXXXXXXXXXX\n");
		printf("written val is %u, read val is %u\n", write_val, read_val);
		exit(0);
	}
}

void loopback_test() {

	unsigned int source_chip_number;
	unsigned int source_gpio_number;
	unsigned int sink_chip_number;
	unsigned int sink_gpio_number;

	printf("INFO : Enter LOOPBACK source details\n");
	get_chip_and_gpio_number(&source_chip_number, &source_gpio_number);

	printf("INFO : Enter LOOPBACK sink details\n");
	get_chip_and_gpio_number(&sink_chip_number, &sink_gpio_number);

	__loopback_test(source_chip_number, source_gpio_number, sink_chip_number,
						sink_gpio_number);
}

/*
** Skipping gpio 17 and 18 because of the hw fault, and because of how the test
** is laid out skipping gpio 0  and 1
*/
void loopback_long_run_test() {

	unsigned int source = 2;
	unsigned int sink = (MAX_GPIOS/2) + 2;

	while(1) {
		if(sink == MAX_GPIOS) {
			source = 2;
			sink = (MAX_GPIOS/2) + 2;
		}

		__loopback_test((source/8), (source%8), (sink/8), (sink%8));
		source++;
		sink++;
	}
}

#if 0
void *event_thread(void *arg) {

	struct gpioevent_request *event_req = (struct gpioevent_request *)arg;
	char *ret;
	struct gpioevent_data  event;

	while(1) {
		printf("INFO : waiting for event\n");
		int rc = read(event_req->fd, &event, sizeof(event));

		printf("GPIO_EVENT : timestamp %llu\n", event.timestamp);
	}
	close(event_req->fd);

#if 0
		if ((ret = (char*) malloc(20)) == NULL) {
			perror("malloc() error");
			exit(2);
		}
		strcpy(ret, "This is a test");
#endif
	pthread_exit(ret);
}


void interrupt_test() {

	pthread_t tid;
	struct gpioevent_request event_req;
	struct gpiohandle_data data;
	int fd, rv;
  	void *ret;

	fd = open(port_paths[0], O_RDWR);
	if(fd == -1) {
		printf("ERROR : opening port %s, errno:%d\n",port_paths[0],
						errno);
		return;
	}

	event_req.lineoffset = 3;
	event_req.handleflags = GPIOHANDLE_REQUEST_INPUT;
	event_req.eventflags = GPIOEVENT_REQUEST_RISING_EDGE;
	strncpy(event_req.consumer_label,"GPIO_INTERRUPT",13);

	rv = ioctl(fd, GPIO_GET_LINEEVENT_IOCTL, &event_req);
	if(rv == -1) {
		printf("ERROR : ioctl GPIO_GET_LINEEVENT_IOCTL failed errno:%d\n",errno);
		return;
	}

	if (pthread_create(&tid, NULL, event_thread, &event_req) != 0) {
		printf("ERROR : pthread_create() failed");
		exit(1);
	}

	//close(fd);
}
#endif

void print_menu() {

	printf("==============================================================\n");
	printf("=============== select one of the below option ===============\n");
	printf("==============================================================\n");
	printf("\t 1. Print port info \n");
	printf("\t 2. Toggle all leds \n");
	printf("\t 3. Print Binary on leds\n");
	printf("\t 4. Loopback Test \n");
	printf("\t 5. Loopback long run Test \n");
	printf("\t 6. Read from GPIO PORT\n");
	printf("\t 7. Write to GPIO PORT\n");
	printf("\t 8. Pull up GPIO PORT\n");
	printf("\t 9. Pull down GPIO PORT\n");
	printf("\t 10. Toggle all GPIOs\n");
	//printf("\t 11. Interrupt Test\n");
	printf("\t 0. Quit\n");
}

int main() {

	unsigned int led_arr[] = {0, 4, 5};
	unsigned int quit = 0;
	int option = 0;

	do {

		print_menu();
		printf("Select one of the option: ");
		if(scanf("%d",&option) == 1) {
				switch(option) {
					case 1:
						printf("====== PORT A ======\n");
						print_port_info(PORTA);

						printf("====== PORT B ======\n");
						print_port_info(PORTB);

						printf("====== PORT C ======\n");
						print_port_info(PORTC);

						printf("====== PORT D ======\n");
						print_port_info(PORTD);
						break;
					case 2:
						printf("Toggling LED D18, D19 and D20\n");
						toggle_gpio(PORTA, led_arr, sizeof(led_arr)/sizeof(led_arr[0]),
											10);
						break;
					case 3:
						printf("Print Binary pattern\n");
						print_binary_pattern(PORTA, led_arr,
								sizeof(led_arr)/sizeof(led_arr[0]), (1 << 3));
						break;
					case 4:
						loopback_test();
						break;
					case 5:
						loopback_long_run_test();
						break;
					case 6:
						read_gpio_port();
						break;
					case 7:
						write_gpio_port();
						break;
					case 8:
						pull_up_gpio_port();
						break;
					case 9:
						pull_down_gpio_port();
						break;
					case 10:
						printf("Toggling all GPIOs\n");
						toggle_all_gpios(10);
						break;
#if 0
					case 11:
						printf("Interrupt Test\n");
						interrupt_test();
						break;
#endif
					case 0:
						printf("Good bye\n");
						quit = 1;
						break;
					default:
						printf("invalid option\n");
				}
		} else {
			printf("ERROR : invalid input type\n");
			return -1;
		}
	} while(quit == 0);

	return 0;
}
