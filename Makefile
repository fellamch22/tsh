CC=gcc
CFLAGS=-Wall -g
 
ALL= get_fichier_type afficher_fichier afficher_repertoire shell sgf
all: $(ALL)

get_fichier_type:
	$(CC) -o get_fichier_type.o -c get_fichier_type.c $(CFLAGS) 
	$(CC) -o get_fichier_type  get_fichier_type.o

afficher_fichier:
	$(CC) -o afficher_fichier.o -c afficher_fichier.c $(CFLAGS) 
	$(CC) -o afficher_fichier  afficher_fichier.o

afficher_repertoire:
	$(CC) -o afficher_repertoire.o -c afficher_repertoire.c $(CFLAGS) 
	$(CC) -o afficher_repertoire  afficher_repertoire.o


shell :
	$(CC) -o shell.o -c shell.c $(CFLAGS) 
	$(CC) -o shell  shell.o

sgf :
	$(CC) -o sgf.o -c sgf.c $(CFLAGS) 
	$(CC) -o sgf  sgf.o
	
clean:
	rm -rf *.o
	rm -rf get_fichier_type  afficher_fichier afficher_repertoire shell
	rm -rf $(ALL) 

