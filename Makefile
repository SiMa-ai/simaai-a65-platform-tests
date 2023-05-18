MAKEFILES = $(shell find . -maxdepth 2 -type f -name Makefile)
SUBDIRS   = $(filter-out ./,$(dir $(MAKEFILES)))

all:
	for dir in $(SUBDIRS); do \
		make -C $$dir all; \
	done
clean:
	for dir in $(SUBDIRS); do \
		make -C $$dir clean; \
	done

