#include <linux/gpio.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define PORTA "/dev/gpiochip0"
#define PORTB "/dev/gpiochip1"
#define PORTC "/dev/gpiochip2"
#define PORTD "/dev/gpiochip3"

char *port_paths[] = {
	PORTA,
	PORTB,
	PORTC,
	PORTD
};

#define TOTAL_NUM_PORTS (sizeof(port_paths)/sizeof(port_paths[0]))
#define MAX_GPIOS_PER_PORT 8

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

void print_menu() {

	printf("\t ===== select one of the below option =====\n");
	printf("\t 1. Print port info \n");
	printf("\t 2. Toggle all leds \n");
	printf("\t 3. Print Binary\n");
	printf("\t 4. Overnight Test \n");
	printf("\t 5. Toggle all GPIOs\n");
	printf("\t 0. Quit\n");
}

int main() {

	printf("\t========== Hello GPIO world ==========\n");
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
						overnight_test();
						break;
					case 5:
						printf("Toggling all GPIOs\n");
						toggle_all_gpios(10);
						break;
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
