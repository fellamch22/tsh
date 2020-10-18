#include<sys/types.h>
#include <unistd.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include"tar.h"

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
			lseek(fd, ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE, SEEK_CUR);
   			read(fd, &p, BLOCKSIZE);
		}
	}
  	return  -1;
}


void delete_fichier(int fd, char *filename){
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
		write(fd, decalage, diff);
  	}
}

int main(int argc , char **argv){
	int fd = open(argv[1], O_RDWR);
	//printf("%ld\n" ,trouve(fd, argv[2]));
	delete_fichier(fd, argv[2]);
	close(fd);
	return 0;
}

