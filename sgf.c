#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include "sgf.h"
#include "tar.h"
#define BUFFER 1024

/**********************************************/
/*   Partie Ajout dans le fichier .tar       */
/********************************************/

/* cette fonction effectue la transformation du fichier refernce par l'ouverture fd1 
      en un ensemble de blocs compatible pour 
	la representation des fichiers dans un fichier .tar */

char * modeToString(int mode, char type ){

    char* droits = malloc(10);
    int d = 100;
    if( type == '5'){
        strcat(droits,"d");
    }else{

         strcat(droits,"-");
    }

    while( d != 0){

        switch(mode / d){
            case(0):
                strcat(droits,"---");
            break;
             case(1):
                 strcat(droits,"--x");
            break;
            case(2):
                 strcat(droits,"-w-");

            break;
             case(3):
                strcat(droits,"-wx");
            break;
             case(4):
              strcat(droits,"r--");
            break;
             case(5):
              strcat(droits,"r-x");
            break;
             case(6):
              strcat(droits,"rw-");
            break;
             case(7):
              strcat(droits,"rwx");
            break;
        }

        mode = mode % d ;
        d = d / 10;
    }
   

    //



    return droits;
}

char * fileToBlocks( int fd , char * filename , int * nb_blocks ){

	ssize_t c =0;
	int i = 0;
	int taille;
	struct posix_header m ;
	struct stat s ;

	//faire un fstat pour recuperer les informations necessaires a la creation de
	//la structure header du fichier 
	
	if ( fstat(fd,&s)  == -1 ){

		perror(" fileToBlocks : Erreur fstat");
		exit(2);
	};


 // remplissage du header correspondant au fichier source 'filename' 
	strcpy(m.name,filename);
	sprintf(m.uname,"%s",getpwuid(s.st_uid)->pw_name);
	sprintf(m.gname,"%s",getgrgid(s.st_gid)->gr_name);
	sprintf(m.size,"%011lo",s.st_size);
	sprintf(m.mode,"%o",s.st_mode);
	sprintf(m.uid,"%d",s.st_uid);
	sprintf(m.gid,"%d",s.st_gid);
	sprintf(m.mtime,"%lo",s.st_mtime);
	m.typeflag =  (S_ISREG(s.st_mode))? '0': (S_ISDIR(s.st_mode))?'5' :(S_ISCHR(s.st_mode))? '3' : (S_ISLNK(s.st_mode))? '2' : '\0';
	sprintf(m.magic,"%s",TMAGIC);
	sprintf(m.version,"%s",TVERSION);
	set_checksum(&m);

	taille = (s.st_size % 512 == 0)? s.st_size+512 : ((s.st_size/512)+2)*512;
	char * contenu = malloc(taille);

	if ( contenu == NULL){

		perror(" erreur malloc");
		exit(1);
	}
	
	printf(" %ld / %ld\n",s.st_size, ((s.st_size+2*512)/512)*512);

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


	int nb_blocks = 0;

	// transformer le fichier refernce par l'ouverture fd1 en un ensemble de blocs compatible pour 
	//la representation des fichiers dans un fichier .tar 
	char * contenu = fileToBlocks(fd1,src_filename,&nb_blocks);

	off_t  diff ;
	struct stat s;

	if ( fstat(fd,&s)  == -1 ){

		perror(" fileToBlocks : Erreur fstat");
		exit(2);
	};


	diff = s.st_size - position ;

	char decalage[diff];

	
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


	// inserer le contenu du fichier source a la position 'position' 

	if( write(fd, contenu , nb_blocks * BLOCKSIZE ) <= 0){
		perror(" addFile : Erreur write");
		exit(2);
	};

    // liberer l'espace alloe pour le contenu du fichier source 
	free(contenu);


	// ecrire le contenu decalee juste apres l'insertion du contennu du nouveau fichier 

	if( write(fd, decalage , diff ) <= 0){
		perror(" addFile : Erreur write");
		exit(2);
	};




}

/*********************************************************************/
/* Partie  Suppression fichier et repertoire dans le fichier .tar   */
/*******************************************************************/

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
      lseek(fd, (filesize % 512 == 0)? filesize : ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE, SEEK_CUR);
      read(fd, &p, BLOCKSIZE);
      sscanf(p.size,"%o",&filesize);
      
    }
     
  }
  return -1;
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


void delete_repertoire(int fd, char *repname){
  // le descripteur donne fd  doit etre un descripteur d'un fichier .tar et ouvert en lecture et ecriture.
  //   repname est le nom du repertoire a supprimer suivi par '/' a la fin
  

  off_t position;		
  struct posix_header p ;
  char prefix[strlen(repname)+1] ;
  memset(prefix ,'\0',strlen(repname)+1);

  position = trouve(fd, repname);
  if(position == -1){
    perror("Repertoire n'existe pas dans Fichiers.tar");
    exit(1);
  }
	
  // supprimer le premier block correspondant au filename
  delete_fichier(fd,repname);
	
  // revenir a position , car le contenu du fichier est decale par delete_file
  if (lseek(fd,position,SEEK_SET) == -1 ){
    perror("delete_repertoire / ERREUR LSEEK ");
    exit(1);
  }

  if (read(fd,&p,BLOCKSIZE) < 0 ){
    perror("delete_repertoire  / ERREUR READ ");
    exit(1);
  }

  // VERIFIER DANS LES HEADER SUIVANTS SI L'UN D'EUX COMMENCE PAR FILENAME ALORS ON LE SUPPRIME
  // TANT QUE CA COMMENCE PAR repname ON CONTINUE
  strncpy(prefix,p.name,strlen(repname));

  while(strcmp(repname,prefix)== 0 ){
    delete_fichier(fd,p.name);
    
    // revenir a la bonne position 
    if(lseek(fd,position,SEEK_SET) == -1){
      perror("delete_repertoire / ERREUR LSEEK ");
      exit(1);
    }
    
    if (read(fd,&p,BLOCKSIZE) < 0 ){
      perror("delete_repertoire  / ERREUR READ ");
      exit(1);
    }
    
    strncpy(prefix,p.name,strlen(repname));
    printf("%s \n",prefix);
  }
}


/**********************************************/
/*   Partie Affichage dans le fichier .tar   */
/********************************************/

void afficher_fichier(int fd, char *chemin){

	off_t position ;
	unsigned int filesize;

	struct posix_header p;

	//suppression du dernier / du chemin
	if(  chemin[strlen(chemin)-1] == '/' ) {	 chemin[strlen(chemin)-1] = '\0'; }
		
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

void afficher_repertoire(int fd, off_t position, int mode){

    struct posix_header p;
    unsigned int filesize ;
    time_t time;
    struct tm * m ;
    char * droits ;
    char  res [BUFFER] ;
    char mdate [15];

    if(lseek(fd , position, SEEK_SET) == -1 ){
        perror(" ERREUR lseek ");
    }

    if( read (fd , &p, BLOCKSIZE) <= 0 ){
        perror(" ERREUR read ");
    }

    char repname[strlen(p.name)+1];
    
    strcpy(repname,p.name);

    repname[strlen(p.name)]='\0';


    if(mode == 1){
        //typeflag [0/5/...]
        droits = modeToString(atoi(p.mode),p.typeflag);
      
        sscanf(p.mtime,"%lo",&time);
        m = localtime(&time);
        strftime(mdate,15,"%b. %d %H:%M",m);
        
        sprintf(res,"%s %s %s %10d %s %s\n",droits,p.uname,p.gname,filesize,mdate,p.name);

        free(droits);
        
        write(1, res, strlen(res));

    }else{
         char name[strlen(p.name)+2];
            sprintf(name,"%s\n",p.name);
            write(1, name, strlen(name));
    }

       if( read (fd , &p, BLOCKSIZE) <= 0 ){
        perror(" ERREUR read ");
    }

    while(strncmp(repname,p.name,strlen(repname)) == 0){

        if(mode == 1){
          
            droits = modeToString(atoi(p.mode),p.typeflag);
            sscanf(p.mtime,"%lo",&time);

            m = localtime(&time);

            sscanf(p.size,"%o",&filesize);

            if ( p.typeflag != '5'){

            strftime(mdate,15,"%b. %d %Y",m);

            }else{

            strftime(mdate,15,"%b. %d %H:%M",m);

            }
        
            sprintf(res,"%s %s %s %10d %s %s\n",droits,p.uname,p.gname,filesize,mdate,p.name);
            
             free(droits);

            write(1,res,strlen(res));


        }else{
             char name[strlen(p.name)+2];
            sprintf(name,"%s\n",p.name);
            write(1, name, strlen(name));
        }
        

        sscanf(p.size,"%o",&filesize);

        if( lseek(fd,(filesize % 512 == 0)? filesize : ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE, SEEK_CUR)== -1){
            perror(" ERREUR read ");
        }

        if( read (fd , &p, BLOCKSIZE) <= 0 ){

        perror(" ERREUR read ");
        }
    }
}

void afficher_tar_content(int fd , int mode){


    struct posix_header p;
    unsigned int filesize ;
    time_t time;
    struct tm * m ;
    char * droits ;
    char  res [BUFFER] ;
    char mdate [15];

    if( lseek(fd,0,SEEK_SET) < 0 ){
        perror(" erreur lseek ");
    }

    if(read(fd,&p,BLOCKSIZE) <= 0){
        perror(" erreur de lecture ");
    }
    
    sscanf(p.size,"%o",&filesize);

    while (p.name[0] != '\0'){
        
        if (mode == 1){// ls -l x.tar

            //typeflag [0/5/...]
            droits = modeToString(atoi(p.mode),p.typeflag);
      
            sscanf(p.mtime,"%lo",&time);
            m = localtime(&time);

            if ( p.typeflag != '5'){

            strftime(mdate,15,"%b. %d %Y",m);

            }else{

            strftime(mdate,15,"%b. %d %H:%M",m);

            }

        
        
            sprintf(res,"%s %s %s %10d %s %s\n",droits,p.uname,p.gname,filesize,mdate,p.name);

            free(droits);
        
            write(1, res, strlen(res));

        }else{// ls x.tar 

            char name[strlen(p.name)+2];
            sprintf(name,"%s\n",p.name);
            write(1, name, strlen(name));
            

        }

        if(lseek(fd,(filesize % 512 == 0)? filesize : ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE, SEEK_CUR) == -1){
            perror(" erreur lseek ");
            exit(1);
        }

         if(read(fd, &p, BLOCKSIZE) <= 0){
             perror("erreur de lecture ");
             exit(1);
         }

         sscanf(p.size,"%o",&filesize); 
    }
}

char get_fichier_type(int fd, char *chemin, int debug){
    struct posix_header p;
     off_t position ;
    
    position = trouve(fd ,chemin);
    
    if ( position == -1 ){
        perror(" fichier inexistant ");
        return 0;
    }

    // la tete de lecture se trouve au bon endroit , par la fonction trouv

    if( read(fd,&p,BLOCKSIZE) <= 0 ){

        perror(" Erreur de lecture  ");
        return 0;
    }
    
            switch(p.typeflag){
              case '0' :
                if ( debug == 1 ) printf("[%c] Le chemin mene a un Fichier\n",p.typeflag);
                break;
              case '1' :
                if ( debug == 1 ) printf("[%c] Le chemin mene a un Lien materiel\n",p.typeflag);
                break;
              case '2' :
                if ( debug == 1 ) printf("[%c] Le chemin mene a un Lien symbolique\n",p.typeflag);
                break;
              case '3' :
                if ( debug == 1 ) printf("[%c] Le chemin mene a un Fichier special caractere\n",p.typeflag);
                break;
              case '4' :
                if ( debug == 1 ) printf("[%c] Le chemin mene a un Fichier special bloc\n",p.typeflag);
                break;
              case '5' :
                if ( debug == 1 ) printf("[%c] Le chemin mene a un Repertoire\n",p.typeflag);
                break;
              case '6' :
                if ( debug == 1 ) printf("[%c] Le chemin mene a un Tube nomme\n",p.typeflag);
                break;
              case '7' :
                if ( debug == 1 ) printf("[%c] Le chemin mene a un Fichier contigu\n",p.typeflag);
                break;
              default  :
                if ( debug == 1 ) printf("[%c] Le chemin mene a un autre type\n",p.typeflag);
                break;
            }

            return (p.typeflag);
       
}



