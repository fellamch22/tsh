#include<sys/types.h>
#include <unistd.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <assert.h>
#include"tar.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

off_t trouve(int fd, char *filename){

  int filesize = 0;
  struct posix_header p;

  lseek(fd, 0, SEEK_SET);



  if(fd < 0){

    perror("Fichier n'existe pas");
    exit(1);
  }

  read(fd, &p, BLOCKSIZE);

  while(p.name[0] != '\0' ){

   	 if(strcmp(p.name , filename) == 0){
      		
      		return  lseek(fd,-512, SEEK_CUR);

    	}else{
    	 
		lseek(fd,((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE,SEEK_CUR);
   	 	read(fd, &p, BLOCKSIZE);

	}

}
  
  return  -1;
}

void delete_fichier(int fd, char *filename){
<<<<<<< HEAD
  int filesize = 0;
  struct posix_header p;
  off_t position = 0;
  trouve(fd, filename);
  if(trouve(fd, filename) == -1) {
    perror("delete_fichier : Erreur position");
    exit(2);
  }
  else{
    position = trouve(fd, filename);
    
    lseek(fd, position - 512, SEEK_CUR) ;
    read(fd, &p, BLOCKSIZE);
    while(p.name[0] != '\0'){
      printf("fichier %s de type %c et de  taille : %s\n", p.name, p.typeflag, p.size);    
      sscanf(p.size, "%o", &filesize);
      int diff = ((filesize + BLOCKSIZE - 1)/512)*BLOCKSIZE;
      lseek(fd, diff, SEEK_CUR);
    read(fd, &p, BLOCKSIZE);
  }
=======

	int filesize = 0;

	off_t position , pos_decalage;

	off_t diff;
	struct stat s ;
	struct posix_header p;

  	
  	position = trouve(fd, filename);

 	 if(position == -1){

   		 perror("Fichier n'existe pas dans Fichiers.tar");
    		exit(1);
	
  	}else{



		if(fstat(fd,&s) == -1){

			
   		 	perror(" fstat failed ");
    			exit(1);
	
		}

	
	// tete de lecture se trouve deja sur position

		if(read(fd,&p,BLOCKSIZE) < 0){

			
   		 	perror(" read failed ");
    			exit(1);
	
		}

		sscanf(p.size,"%o",&filesize);

		pos_decalage = lseek(fd,((filesize % 512) == 0)? filesize : ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE , SEEK_CUR);

		diff = s.st_size - pos_decalage;
		char  decalage[diff];


		if(read(fd,decalage,diff) < 0){

			
   		 	perror(" read failed ");
    			exit(1);
	
		}

		if(lseek(fd,position,SEEK_SET) == -1){

			
   		 	perror(" lseek failed ");
    			exit(1);
	
		}
		

>>>>>>> 74fe3de6d4559d713088080f2724ec4efcda140b
  }

}


int main(int argc , char **argv){

<<<<<<< HEAD
  //printf("%ld\n", trouve(fd, argv[2]));
  delete_fichier(fd, argv[2]);
  return 0;
=======
	int fd = open(argv[1], O_RDWR);

	delete_fichier(fd, argv[2]);

	close(fd);

	return 0;
>>>>>>> 74fe3de6d4559d713088080f2724ec4efcda140b
}
