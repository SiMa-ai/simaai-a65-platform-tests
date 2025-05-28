all : ddr_test memory_test


ddr_test : ddr_test.c
	${CC} $? -o $@ ${LDFLAGS} -lsimaaimem

memory_test : memory_test.c
	${CC} $? -o $@ ${LDFLAGS} -lsimaaimem

clean :
	rm -f ddr_test *.o
	rm -f memory_test *.o
