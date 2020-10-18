
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "sgf.h"
#include <sys/types.h>
#include <fcntl.h>

char * fileToBlocks( int fd , char * filename , int * nb_blocks ){

	char * contenu = NULL;
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
	sprintf(m.mtime,"%ju",s.st_mtime);
	m.typeflag =  (S_ISREG(s.st_mode))? '0': (S_ISDIR(s.st_mode))?'5' :(S_ISCHR(s.st_mode))? '3' : (S_ISLNK(s.st_mode))? '2' : '\0';
	sprintf(m.magic,"%s",TMAGIC);
	sprintf(m.version,"%s",TVERSION);
	set_checksum(&m);
	//printf(" checksum correct %d\n",check_checksum(&m)); 


	/*if( lseek( fd , 0 , SEEK_SET) == -1 ){
		perror(" fileToBlocks : Erreur seek");
		exit(2);
	};

	// utiliser lseek pour recuperer la taille du fichier fd
	if( (size = lseek( fd , 0 , SEEK_END) ) == -1 ){
		perror(" fileToBlocks : Erreur seek");
		exit(2);
	};*/

	contenu = malloc((s.st_size % 512 == 0)? s.st_size+512 : ((s.st_size+2*512)/512)*512);
	//contenu = malloc(190000);
	if( contenu == NULL){

		perror(" fileToBlocks : Erreur malloc");
		exit(2);
	}
	
	//printf(" %ld / %ld\n",s.st_size, ((s.st_size+2*512)/512)*512);
	memcpy(contenu,&m,sizeof(struct posix_header) );
	i+=512;
	while( (c = read(fd,contenu+i ,512)) == 512){
	

		i += 512;

	}

	if ( c > 0 ){

		// le dernier block lu n'atteint pas 512 bits , on complete la zone restante par le caractere '\0'

		memset( contenu+i+c , '\0' , 512 - c );


	}


	// retourner le nombre de blocks en devisant la totalite des caracter lus par 512 (taille d'un seul block)

	

	(*nb_blocks) = ( c > 0)? (( i + 512)/ 512) : (i / 512) ;
	return contenu;

	
	

}

// Add file content to .tar reference by 'fd' at the position 'position'

void addFile( int fd, int fd1 , char * src_filename , off_t position){


	char * contenu = NULL;
	ssize_t c =0;
	int i = 0;
        int nb_blocks = 0;
	struct posix_header m ;
	struct stat s ;
	// faire un fstat pour creer la strucuture header du fichier
	
	if ( fstat(fd1,&s)  == -1 ){

		perror(" fileToBlocks : Erreur fstat");
		exit(2);
	};


	strcpy(m.name,src_filename);
	sprintf(m.size,"%011lo",s.st_size);
	sprintf(m.mode,"%o",s.st_mode);
	sprintf(m.uid,"%d",s.st_uid);
	sprintf(m.gid,"%d",s.st_gid);
	sprintf(m.mtime,"%ld",s.st_mtime);
	m.typeflag =  (S_ISREG(s.st_mode))? '0': (S_ISDIR(s.st_mode))?'5' :(S_ISCHR(s.st_mode))? '3' : (S_ISLNK(s.st_mode))? '2' : '\0';
	sprintf(m.magic,"%s",TMAGIC);
	sprintf(m.version,"%s",TVERSION);
	set_checksum(&m);
	printf(" checksum correct %d %c\n",check_checksum(&m),m.typeflag); 


	/*if( lseek( fd , 0 , SEEK_SET) == -1 ){
		perror(" fileToBlocks : Erreur seek");
		exit(2);
	};

	// utiliser lseek pour recuperer la taille du fichier fd
	if( (size = lseek( fd , 0 , SEEK_END) ) == -1 ){
		perror(" fileToBlocks : Erreur seek");
		exit(2);
	};*/

	contenu = malloc((s.st_size % 512 == 0)? s.st_size+512 : ((s.st_size/512+2))*512);
	//contenu = malloc(190000);
	if( contenu == NULL){

		perror(" fileToBlocks : Erreur malloc");
		exit(2);
	}
	
	printf(" %ld / %ld\n",s.st_size, ((s.st_size+2*512)/512)*512);

	memcpy(contenu,&m,sizeof(struct posix_header) );

	i+=512;

	while( (c = read(fd1,contenu+i ,512)) == 512){
	

		i += 512;

	}

	if ( c > 0 ){

		// le dernier block lu n'atteint pas 512 bits , on complete la zone restante par le caractere '\0'

		memset( contenu+i+c , '\0' , 512 - c );


	}


	// retourner le nombre de blocks en devisant la totalite des caracter lus par 512 (taille d'un seul block)

	

	nb_blocks = ( c > 0)? (( i + 512)/ 512) : (i / 512) ;



/********************************************************************************************************************************/
	
	off_t size , diff ;
	struct stat s1;

	/*if( lseek( fd , 0 , SEEK_SET) == -1 ){
		perror(" addFile : Erreur seek");
		exit(2);
	};

	// utiliser lseek pour recuperer la taille du fichier fd
	if( (size = lseek( fd , 0 , SEEK_END) ) == -1 ){
		perror(" addFile : Erreur seek");
		exit(2);
	};*/

	if ( fstat(fd,&s1)  == -1 ){

		perror(" fileToBlocks : Erreur fstat");
		exit(2);
	};

	//sscanf(s.st_size,"%011lo",&size);
	//printf(" %lo \n",size);

	diff = s.st_size - position ;

	char * decalage;
	decalage = malloc(diff);

	
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


	if( write(fd, contenu , nb_blocks * BLOCKSIZE ) <= 0){
		perror(" addFile : Erreur write");
		exit(2);
	};


	if( write(fd, decalage , diff ) <= 0){
		perror(" addFile : Erreur write");
		exit(2);
	};




}



int main( int argc , char * argv[]){


	//char * v;
	//int n ;
	//off_t position ;
	//int blocks;
	struct posix_header m;

	int fd = open("book1.txt",O_RDONLY);
	int fd1 = open("toto.tar",O_RDWR);

	//v = fileToBlocks(fd,"book1.txt",&blocks);
	//read(fd1,&m,512);
	//sscanf(m.size,"%o",&n);
	//printf("%ld\n",sizeof(v));
	//position = lseek(fd1, -2*512-1,SEEK_END);
	addFile(fd1,fd,"book1.txt",0);
	addFile(fd1,fd,"book1.txt",0);
	//write(0,v,blocks*512);
	//printf(" blocks : %d \n",blocks);
	close(fd);
	close(fd1);

}



