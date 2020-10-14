#ifndef SGF_H
#define SGF_H
#include "tar.h"

int ft_strlen(char *s);
void afficher_repertoire(int fd, off_t position);
char get_fichier_type(int fd, char *chemin);
void afficher_fichier(int fd, off_t position);


#endif
