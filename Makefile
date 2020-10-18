CC=gcc
CFLAGS=-Wall -g

ALL=sgf 
all: $(ALL)


sgf : sgf.o 
	$(CC) $^ $(LDLIBS) -o $@

sgf.o : sgf.c 

clean:
	rm -rf $(ALL)
