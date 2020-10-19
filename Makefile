CC=gcc
CFLAGS=-Wall -g "-mmacosx-version-min=10.7"
 
ALL= get_fichier_type afficher_fichier afficher_repertoire cmd_ls 
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


cmd_ls:
	$(CC) -o cmd_ls.o -c cmd_ls.c $(CFLAGS) 
	$(CC) -o cmd_ls  cmd_ls.o
	
clean:
	rm -rf *.o
	rm -rf get_fichier_type  afficher_fichier afficher_repertoire cmd_ls  
	rm -rf $(ALL) 

