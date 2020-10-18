sgf.c                                                                                               0000644 0001750 0001750 00000004571 13743020216 010736  0                                                                                                    ustar   fella                           fella                                                                                                                                                                                                                  #include<sys/types.h>
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
      		//sscanf(p.size, "%o", &filesize);
      		//int diff = ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE;
      		return  lseek(fd,-512, SEEK_CUR);

    	}else{
    	 
		lseek(fd,((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE,SEEK_CUR);
   	 	read(fd, &p, BLOCKSIZE);

	}

}
  
  return  -1;
}


void delete_fichier(int fd, char *filename){

	int filesize = 0;
	//int position = lseek(fd, 0, SEEK_SET);
	off_t position , pos_decalage;

	off_t diff;
	struct stat s ;
	struct posix_header p;

  	/*if(fd < 0){
    	perror("Fichier n'existe pas dans Fichiers.tar");
    	exit(1);
 	 }*/

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
		
		write(fd,decalage,diff);
    //remove(filename);
   // position -= BLOCKSIZE;
   // read(fd, &p, BLOCKSIZE);
	
    /*while(p.name[0] != '\0'){

      printf("fichier %s de type %c et de  taille : %s\n", p.name, p.typeflag, p.size);
      sscanf(p.size, "%o", &filesize);
      int diff = ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE;
      lseek(fd, diff, SEEK_CUR);
      read(fd, &p, BLOCKSIZE);

    }*/
  }

}


int main(int argc , char **argv){

  int fd = open(argv[1], O_RDWR);

   printf("%ld\n", trouve(fd, argv[2]));
   delete_fichier(fd, argv[2]);
   close(fd);
  return 0;
}

                                                                                                                                       structure.txt                                                                                       0000644 0001750 0001750 00000002542 13740057736 012626  0                                                                                                    ustar   fella                           fella                                                                                                                                                                                                                  
02.10.2020
-bibliothèque tar.h

cd(char * chemin)
      .tar/…/…
	- int file to block (int fd, char * contenue)
		 ajouter file extérieur dans le .tar comme archiver
		 préparer sa structure pour mettre dans .tar
	- add-fichier(int fd, char * contenue, int nbdeblock, off_t position) 
                                     descripteur du fichier « .tar »               où on va ajouter le fichier
  Déplacer le fichier dans .tar                        
		        
		Exemple : 
		tar -cvf toto.tar rep1 f1 f2 f3
		affichier toto.tar 
					rep/ff1
					rep/ff2
					f1
					f2
					f3….
(si je veux ajouter f4 dans toto.tar, il faut déplacer les autre fichiers pour ne 
pas écraser les données.)
					
	- get fichier type(int fd, char * chemin) ;
			                nom de fichier ./tar/A/a/b…etc
				   renvoie 0, 5…etc comme tp1 
	- afficher fichier(int fd, off_t position) : cat
	- afficher repetoire(int fd, off_t position) : ls (cf. fseek : SETCUR, SETIN…)
			       pour éviter de parcourir tout le répertoire
	- off_t trouv(int fd, char * chemin)
	    Remplacement du fichier / répertoire dans .tar
	    s’il a trouvé le fichier, return la position, sinon return -1
	- delete fichier (int fd, char * chemin )
			nom de fichier
	- delete répertoire (int fd, char * chemin )
			nom de fichier	




	

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              