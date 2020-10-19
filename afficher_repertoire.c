#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "tar.h"


void afficher_repertoire(int fd, off_t position){


	struct posix_header p;
	unsigned int filesize ;

	if(lseek(fd , position, SEEK_SET) == -1 ){

		perror(" ERREUR lseek ");
		exit(1);

	}

	if( read (fd , &p, BLOCKSIZE) <= 0 ){

		perror(" ERREUR read ");
		exit(1);

	}

        char repname[strlen(p.name)+1];
	
	strcpy(repname,p.name);

	repname[strlen(p.name)]='\0';

		write(1,p.name,strlen(p.name));
		write(1,"\n",1);

   	if( read (fd , &p, BLOCKSIZE) <= 0 ){

		perror(" ERREUR read ");
		exit(1);

	}



	while(strncmp(repname,p.name,strlen(repname) )== 0){

		
		write(1,p.name,strlen(p.name));
		write(1,"\n",1);

		sscanf(p.size,"%o",&filesize);

		if( lseek(fd,(filesize % 512 == 0)? filesize : ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE, SEEK_CUR)== -1){

			perror(" ERREUR read ");
			exit(1);
		}

		if( read (fd , &p, BLOCKSIZE) <= 0 ){

		perror(" ERREUR read ");
		exit(1);

	}


	}


	
    
}

int main(int argc, char * argv[])
{
    int fd = open(argv[1], O_RDONLY);
    afficher_repertoire(fd, 0);
    close(fd);

    return 0;
}
