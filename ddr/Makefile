all : ddr_test


ddr_test : ddr_test.c
	${CC} $? -o $@ ${LDFLAGS} -lsimaaimem

clean :
	rm -f ddr_test *.o
