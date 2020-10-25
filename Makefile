CC=gcc
CFLAGS=-Wall -g
 
ALL=shell sgf
all: $(ALL)

shell :
	$(CC) -o shell.o -c shell.c $(CFLAGS) 
	$(CC) -o shell  shell.o

sgf :
	$(CC) -o sgf.o -c sgf.c $(CFLAGS) 
	$(CC) -o sgf  sgf.o
	
clean:
	rm -rf *.o
	rm -rf shell sgf
	rm -rf $(ALL) 

