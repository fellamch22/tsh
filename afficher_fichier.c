#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "tar.h"

off_t trouve(int fd, char *filename){
	int filesize = 0;
	struct posix_header p;
  	lseek(fd, 0, SEEK_SET);

	if(fd < 0){

    		perror("Fichier n'existe pas");
    		exit(1);
 	}

  	read(fd, &p, BLOCKSIZE);

	sscanf(p.size,"%o",&filesize);
  	while(p.name[0] != '\0' ){

   		if(strcmp(p.name , filename) == 0){
      		return  lseek(fd,-512, SEEK_CUR);
    	}else{ 

			lseek(fd,(filesize % 512 == 0)? filesize : ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE, SEEK_CUR);
   			read(fd, &p, BLOCKSIZE);
			sscanf(p.size,"%o",&filesize);

		}

	}

  	return  -1;
}

void afficher_fichier(int fd, char *chemin){

	off_t position ;
	unsigned int filesize;

	struct posix_header p;

   	position = trouve(fd,chemin);

	if ( position == -1 ){

		perror(" fichier inexistant ");
		exit(1);

	}


	// la tete de lecture se trouve au bon endroit , par la fonction trouv

	if( read(fd,&p,BLOCKSIZE) <= 0 ){

		perror(" Erreur de lecture  ");
		exit(1);

		
	}

	sscanf(p.size,"%o",&filesize);
	
	char content [filesize];

	if( read(fd,content,filesize) <= 0 ){

		perror(" Erreur de lecture  ");
		exit(1);

		
	}

	write(1,content,filesize);
	


}

int main(int argc, char * argv[])
{
    if (argc != 3)
    {
        printf("Wrong number of arguments\nUsage: ./afficher_fichier <file.tar> <chemin>\n");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        perror("open\n");
        return -1;
    }
    afficher_fichier(fd, argv[2]);
    if (close(fd) == -1)
    {
        perror("close\n");
        return -1;
    }

    return 0;
}
