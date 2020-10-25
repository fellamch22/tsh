
#ifndef SGF_H
#define SGF_H
#include "tar.h"

char * fileToBlocks( int fd , char * filename ,int * nb_blocks );
void addFile( int fd, int fd1, char * src_filename ,off_t position);
off_t trouve(int fd, char *filename);
void delete_fichier(int fd, char *filename);
void delete_repertoire(int fd, char *repname);
void afficher_fichier(int fd, char *chemin);
void afficher_repertoire(int fd, off_t position);
char get_fichier_type(int fd, char *chemin);

#endif
