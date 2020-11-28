
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include "sgf.h"


/**********************************************/
/*   Partie Ajout dans le fichier .tar       */
/********************************************/

/* cette fonction effectue la transformation du fichier refernce par l'ouverture fd1 
      en un ensemble de blocs compatible pour 
	la representation des fichiers dans un fichier .tar */

char * fileToBlocks( int fd , char * filename , int * nb_blocks ){

	ssize_t c =0;
	int i = 0;
	int taille;
	struct posix_header m ;
	struct stat s ;

	/* faire un fstat pour recuperer les informations necessaires a la creation de
	la structure header du fichier */
	
	if ( fstat(fd,&s)  == -1 ){

		perror(" fileToBlocks : Erreur fstat");
		exit(2);
	};

	/* faire un fstat pour recuperer les informations necessaires a la creation de
	la structure header du fichier */
	
	if ( fstat(fd,&s)  == -1 ){

		perror(" fileToBlocks : Erreur fstat");
		exit(2);
	};

/* remplissage du header correspondant au fichier source 'filename' */
	memset(m.name,'\0',100);
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

	/* transformer le fichier refernce par l'ouverture fd1 en un ensemble de blocs compatible pour 
	la representation des fichiers dans un fichier .tar */
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


	/* inserer le contenu du fichier source a la position 'position' */

	if( write(fd, contenu , nb_blocks * BLOCKSIZE ) <= 0){
		perror(" addFile : Erreur write");
		exit(2);
	};

    /* liberer l'espace alloe pour le contenu du fichier source */
	free(contenu);


	/* ecrire le contenu decalee juste apres l'insertion du contennu du nouveau fichier */

	if( write(fd, decalage , diff ) <= 0){
		perror(" addFile : Erreur write");
		exit(2);
	};




}

/*mkdir* fonction qui cree un repertoire vide dans le tarball reference par l'ouverture 
   fd , et le chemin du repertoire le fichier .tar , sans le "slash a la fin " ,
   fd doit etre ouvert en lecture et ecriture  **/

void newEmptyDirectory(int fd ,char * directoryPath ){


    struct stat t ;

  /** verifier si directoryPath existe deja et renvoyer un message 
   * de warning a l'utilisateur  */

  if (trouve(fd,directoryPath) >= 0)
  {
      char s [50] = "cannot create Directory , already exists \n";
      write(1,s,strlen(s));

  }else
  { 
	// verifier si le repertoire pere existe
    // effectuer un analyse de path avant pour enlever le dernier nom directory a la fin
    int pathlen = strlen(directoryPath);
    char s1 [pathlen+1];
	char pathpere [pathlen+1];
    strcpy(s1,directoryPath);
    s1[pathlen-1] ='\0';


    while( (s1[pathlen] != '/') && (pathlen > 0) ){
         pathlen --;
    }

	strncpy(pathpere,s1,pathlen);
	printf("%s\n",pathpere);
    if( (strcmp(pathpere,"") < 0 ) && (trouve(fd,pathpere) == -1) ){
      /* repertoire pere n'existe pas */
     	char s2 [100] = " cannot create Directory , No such file or directory  \n";
     	 write(1,s2,strlen(s2));
    }else
    {
      	/* faire un fstat pour recuperer les informations necessaires a la creation de
	      la structure header du fichier */
	
	      if ( fstat(fd,&t)  == -1 ){

		      perror(" newEmptyDirectory : Erreur fstat");
		      exit(1);
	      };

      // on cree le header du repertoire et on l'insere a la fin du .tar

      	struct posix_header p;
		time_t current_time = time(NULL);
		memset(p.name,'\0',100);
      	strcpy(p.name,directoryPath);
	    sprintf(p.uname,"%s",getpwuid(t.st_uid)->pw_name);
	    sprintf(p.gname,"%s",getgrgid(t.st_gid)->gr_name);
	    sprintf(p.size,"%011lo",(off_t)0);
	    sprintf(p.mode,"%o",(mode_t)0755);
	    sprintf(p.uid,"%d",t.st_uid);
	    sprintf(p.gid,"%d",t.st_gid);
	    sprintf(p.mtime,"%lo",current_time);
      	p.typeflag ='5';
	    sprintf(p.magic,"%s",TMAGIC);
	    sprintf(p.version,"%s",TVERSION);
	    set_checksum(&p);
		printf("%s\n",p.name);
      // se deplace a la fin du tarball , avant les deux blocks contenant

	/*	if( lseek(fd,(off_t)-1024,SEEK_END) == -1 ){

			perror(" newEmptyDirectory : Erreur seek");
			exit(1);

		}*/
		if( lseek(fd,(off_t)0,SEEK_SET) == -1 ){

			perror(" newEmptyDirectory : Erreur seek");
			exit(1);
		}
		//char sauvegarde[1024];// 2 blocks de 512
		//memset(sauvegarde,'\0',1024);
		char sauvegarde[t.st_size];
		// sauvegarder le contenu des deux derniers blocks

		if(read(fd,sauvegarde,t.st_size) <= 0  ){

			perror(" newEmptyDirectory : Erreur read ");
			exit(1);
		}

		// revenir a la derniere position avant de faire le read
		if( lseek(fd,(off_t)0,SEEK_SET) == -1 ){

			perror(" newEmptyDirectory : Erreur seek");
			exit(1);

		}	
		
      	//inserer le header du repertoire
		if(write(fd,&p,BLOCKSIZE) <= 0  ){

			perror(" newEmptyDirectory : Erreur write ");
			exit(1);
		}

		// remettre les derniers blocks de null apres le header inseré

		if(write(fd,sauvegarde,t.st_size) <= 0  ){

			perror(" newEmptyDirectory : Erreur write ");
			exit(1);
		}

    }
    
  }
  
}



/*cp*
    analyse les arguments source et destionation d'une commande
	retourne -1 si les arguments ne satisfont pas les conditions

	pwd -> la variable globale de notre programme , qui indique l'emplacement courant
*/

int analyser_src_dest (char * pwd ,char * source , char* destination , int* is_srctar , int* is_destar , int*
is_srcfile , int* is_destdir, char * srcpart1 ,char* srcpart2,char* destpart1 ,char* destpart2 ){


/* ne pas oublier de liberer l'espace memoire */
int part1 ;
char * strbuf;
char * srccopie [100];
char * destcopie [100];
char srcbuf[100];
char destbuf[100];
srcpart1 = malloc(100);
srcpart2 = malloc(100);
destpart1 = malloc(100);
destpart2 = malloc(100);
memset(srcpart1,'\0',100);
memset(srcpart2,'\0',100);
memset(destpart1,'\0',100);
memset(destpart2,'\0',100);
strcpy(srccopie,source);
strcpy(destcopie,source);

/** analyser source **/


if(source[0] =='~'){ // chemin qui commence par tilde non acceptable
  /* on met dans srcpart1 le chemin forme par home/username/ , car les fonctions open et read n'acceptent pas
   le cehmin qui commencent par tilde */

   sprintf(srcpart1,"/home/%s/",getpwuid(getuid())->pw_name);

}else if ( source[0] == '.'){
 // chemin relatif qui commence par '.' , on le remplace par le chemin absolu du repertoire courant

	sprintf(srcpart1 ,"%s",pwd);

}else if( strncmp(source,"..",2) == 0){ 

 // chemin relatif qui commence par '.' , on le remplace par le chemin absolu du repertoire courant
// on retire le dernier repetoire du chemin courant --> need to find a solution
// je parcours src depuis la fin et je decrmente un compteur
// apres je copie avec snprintf

 int src_len = strlen(source) - 1;

  while(source[src_len ] != '/')  src_len -- ;
  
  snprintf(srcbuf,src_len,"%s",source);

}

// parcourir source1  pour detecter si il ya un .tar dans le chemin et mettre a jour
strbuf = strtok(source,"/");
part1 = 1 ;

while ( strbuf != NULL ){

    if( strlen(strbuf) > 4){ // pour voir si il termine par .tar

        if( strncmp(strbuf[strlen(strbuf)-4],".tar",4) == 0 ){
			(*is_srctar) = 1;
			strcat(srcpart1,"/");
			strcat(srcpart1,strbuf);
			//free(strbuf); - to check

			part1 = 0 ;
		}else if( part1){ // on remplie la premiere partie de source avant de rencontrerun .tar
				if( strcmp(srcpart1,"") != 0 ) strcat(srcpart1,"/");
				strcat(srcpart1,strbuf);
		}else{ // on remplie la deuxieme partie de source apres recontrer un .tar
				if( strcmp(srcpart2,"") != 0 ) strcat(srcpart2,"/");
				strcat(srcpart2,strbuf);

		}
		
	}

	strbuf = strtok(source,"/");

 
}

// si partie 2 est remplie , on verifie la fin de source ( pour ne pas negliger le '/' de la fin  d'un repertoire dans un .tar )
if(!part1 && srccopie[strlen(srccopie)-1] == '/') strcat(srcpart2,"/");
// analyser 
// srcpart1 srcpart2
// ida kan tar diri open  w hawssi b getfichiertype
if( (*is_srctar) ){
   // je vois seulement la fin si c'est / c'est un repertoire , else it's file (je ne teste pas pour ne pas avoir une repetition )
}else
   // source n'et pas un tar , on fait un stat 
{
	
}

// ida makach tar diri stat direct w djibi type tae hedek lfichier
// strtok

	return 0;
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
  /* le descripteur donne fd  doit etre un descripteur d'un fichier .tar et ouvert en lecture et ecriture.
     repname est le nom du repertoire a supprimer suivi par '/' a la fin
  */

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

/* a corriger  , la condition  du while je la mets a l'interieur et 
 et la condition d'arret ser ala findu fichier */
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


/*******************************/
/*        Partie main         */
/******************************/
int main( int argc , char * argv[]){

	/* test d'insertion du fichier book1.txt au debut du fichier .tar */

	struct stat t ;
	int fd = open("book1.txt",O_RDONLY);
	int fd1 = open("toto.tar",O_RDWR);

	//fstat(fd1,&t);
	/*addFile(fd1,fd,"book1.txt/",(off_t)0);*/
	//newEmptyDirectory(fd1,"c/");
	printf("%s\n",getpwuid(getuid())->pw_name);
	close(fd);
	close(fd1);
	/*test des fonction : trouve delete_fichier delete_repertoire
	 * Décommenter pour tester
	 */
	//int fd = open(argv[1], O_RDWR);
	//printf("%ld\n" ,trouve(fd, argv[2]));
	//delete_fichier(fd, argv[2]);
	//delete_repertoire(fd, argv[2]);
	//int fd3 = open(argv[1], O_RDWR);
	//printf("%ld\n" ,trouve(fd3, argv[2]));
	//delete_fichier(fd3, argv[2]);
	//delete_repertoire(fd3, argv[2]);
	//close(fd3);
	  //close(fd1);
	  //close(fd);

	/*test pour afficher*/
	//afficher_fichier(fd, argv[2]);
	/*if (argc != 3)
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

	    afficher_repertoire(fd, 0);
	    get_fichier_type(fd, argv[2]);*/

	return 0;
	
	
}



