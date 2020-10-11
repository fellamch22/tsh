
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "sgf.h"

int fileToBlocks( int fd , char * filename , char * contenu ){


	ssize_t c ;
	int i = 0;
	struct posix_header m ;
	struct stat s ;
	// faire un fstat pour creer la strucuture header du fichier
	
	if ( fstat(fd,&s)  == -1 ){

		perror(" fileToBlocks : Erreur fstat");
		exit(2);
	};


	sprintf(m.name,"%s",filename);
	sprintf(m.size,"%011lo",s.st_size);
	sprintf(m.mode,"%o",s.st_mode);
	sprintf(m.uid,"%o",s.st_uid);
	sprintf(m.gid,"%o",s.st_gid);
	sprintf(m.mtime,"%ld",s.st_mtime);
	m.typeflag =  (S_ISREG(s.st_mode))? '0': (S_ISDIR(s.st_mode))?'5' :(S_ISCHR(s.st_mode))? '3' : (S_ISLNK(s.st_mode))? '2' : '\0';
	sprintf(m.magic,"%s",TMAGIC);
	sprintf(m.version,"%s",TVERSION);

	/*if( lseek( fd , 0 , SEEK_SET) == -1 ){
		perror(" fileToBlocks : Erreur seek");
		exit(2);
	};

	// utiliser lseek pour recuperer la taille du fichier fd
	if( (size = lseek( fd , 0 , SEEK_END) ) == -1 ){
		perror(" fileToBlocks : Erreur seek");
		exit(2);
	};*/

	contenu = malloc(sizeof((s.st_size % 512 == 0)? s.st_size : ((s.st_size+512/512)*BLOCKSIZE)));

	if( contenu == NULL){

		perror(" fileToBlocks : Erreur malloc");
		exit(2);
	}

	memmove(contenu,&m,sizeof(struct posix_header) );
	
	while( (c = read(fd,contenu+i ,512)) == 512)
	
		i += 512;

	if ( c > 0 ){

		// le dernier block lu n'atteint pas 512 bits , on complete la zone restante par le caractere '\0'

		memset( contenu+i+c , '\0' , 512 - c );


	}


	// retourner le nombre de blocks en devisant la totalite des caracter lus par 512 (taille d'un seul block)

	

	return ( c > 0)? (( i + 512)/ 512) : (i / 512) ;

	
	

}

// Add file content to .tar reference by 'fd' at the position 'position'

void addFile( int fd , char * contenu , int nb_blocks , off_t position){

	
	off_t size , diff ;

	if( lseek( fd , 0 , SEEK_SET) == -1 ){
		perror(" addFile : Erreur seek");
		exit(2);
	};

	// utiliser lseek pour recuperer la taille du fichier fd
	if( (size = lseek( fd , 0 , SEEK_END) ) == -1 ){
		perror(" addFile : Erreur seek");
		exit(2);
	};


	diff = size - position ;

	char * decalage = malloc(sizeof(diff));

	
	if( lseek( fd , position , SEEK_SET) == -1 ){
		perror(" addFile : Erreur seek");
		exit(2);
	};

	
	
	if ( read(fd , decalage , diff ) < 0 ){

		perror(" addFile : Erreur read");
		exit(2);
	}

	if( lseek( fd , position , SEEK_SET) == -1 ){
		perror(" addFile : Erreur seek");
		exit(2);
	};


	write(fd, contenu , nb_blocks * BLOCKSIZE );
	write(fd, decalage , diff );


}



