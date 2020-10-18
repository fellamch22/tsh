CC=gcc
CFLAGS=-Wall -g

ALL= sgf
#all: $(ALL)


sgf : sgf.o
	$(CC) $^ $(LDLIBS) -o $@

sgf.o : sgf.c sgf.h


clean:
	rm -rf $(ALL)
