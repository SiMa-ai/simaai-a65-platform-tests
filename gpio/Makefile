all : gpio_test

gpio_test : gpio_test.c
	${CC} ${LDFLAGS} $? -o $@

clean :
	rm -f gpio_test *.o
