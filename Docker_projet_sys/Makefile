CC=gcc
CFLAGS=-Wall -g
 
ALL=shell
all: $(ALL)

shell : shell.c syntaxe.c sgf.c 
	$(CC) shell.c syntaxe.c sgf.c $(CFLAGS) -o shell

clean:
	rm -rf *.o
	rm -rf $(ALL) 

