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

char get_fichier_type(int fd, char *chemin){

	struct posix_header p;
 	off_t position ;
	
	position = trouve(fd ,chemin);
	
	
	if ( position == -1 ){

		perror(" fichier inexistant ");
		exit(1);

	}


	// la tete de lecture se trouve au bon endroit , par la fonction trouv

	if( read(fd,&p,BLOCKSIZE) <= 0 ){

		perror(" Erreur de lecture  ");
		exit(1);

		
	}
   
	
            switch(p.typeflag){
              case '0' :
                printf("[%c] Le chemin mene a un Fichier\n",p.typeflag);
                break;
              case '1' :
                printf("[%c] Le chemin mene a un Lien materiel\n",p.typeflag);
                break;
              case '2' :
                printf("[%c] Le chemin mene a un Lien symbolique\n",p.typeflag);
                break;
              case '3' :
                printf("[%c] Le chemin mene a un Fichier special caractere\n",p.typeflag);
                break;
              case '4' :
                printf("[%c] Le chemin mene a un Fichier special bloc\n",p.typeflag);
                break;
              case '5' :
                printf("[%c] Le chemin mene a un Repertoire\n",p.typeflag);
                break;
              case '6' :
                printf("[%c] Le chemin mene a un Tube nomme\n",p.typeflag);
                break;
              case '7' :
                printf("[%c] Le chemin mene a un Fichier contigu\n",p.typeflag);
                break;
              default  :
                printf("[%c] Le chemin mene a un autre type\n",p.typeflag);
                break;
            }


            return (p.typeflag);
       

}


int main(int argc, char * argv[]){
    if (argc != 3){
        printf("Wrong number of arguments\nUsage: ./listar <file.tar> <chemin>\n");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    get_fichier_type(fd, argv[2]);

    close(fd);

    return 0;
}
