
#ifndef SGF_H
#define SGF_H
#include "tar.h"

int fileToBlocks( int fd , char * filename ,char * contenu );
void addFile( int fd , char * contenu , int nb_blocks , off_t position);


#endif
