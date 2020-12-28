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
#include <dirent.h>
#include "tar.h"
#include "sgf.h"
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

// retourne la position de fin d'un tarball -> exactemnt le debut des blocks nuls

off_t get_end_position(int fd_tar){

  off_t position;
  struct posix_header h ;
  int filesize;
	// chercher la position de fin du tarball 
					if(  lseek(fd_tar,0,SEEK_SET) == -1 ){

						perror("erreur lseek \n");
						exit(1);
					}

					// chercher la position de fin du tarball destination
					if(read(fd_tar,&h,BLOCKSIZE) == -1){

						perror(" erreur read \n");
						exit(1);

					}

					while (h.name[0] != '\0'){

						sscanf(h.size,"%o",&filesize);
						lseek(fd_tar, (filesize % 512 == 0)? filesize : ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE, SEEK_CUR);
     				read(fd_tar, &h, BLOCKSIZE);

					}
			
					// se positionner a la fin du fichier destination
					position = lseek(fd_tar,-BLOCKSIZE,SEEK_CUR);

          if(position == -1 ) perror("erreur lseek \n");

          return position;

}

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
  
/* remplissage du header correspondant au fichier source 'filename' */
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
  memset(m.prefix,'\0',155);
	set_checksum(&m);

	taille = (s.st_size % 512 == 0)? s.st_size+512 : ((s.st_size/512)+2)*512;
	char * contenu = malloc(taille);

	if ( contenu == NULL){

		perror(" erreur malloc");
		exit(1);
	}
	
	//printf(" %ld / %ld\n",s.st_size, ((s.st_size+2*512)/512)*512);

	memcpy(contenu,&m,sizeof(struct posix_header) );

	i+=512;

  if(lseek(fd,(off_t)0,SEEK_SET)== -1){
    
    perror("erreur lseek \n");
    exit(1);
  }

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


void newEmptyDirectory(int fd ,char * directoryPath ){


    struct stat t ;
    struct posix_header p , h;
	int filesize ;

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

	memset(pathpere,'\0',pathlen+1);
	strncpy(pathpere,s1,pathlen);

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

		time_t current_time = time(NULL);
		 
		struct passwd * pw = getpwuid(t.st_uid);
		memset(p.name,'\0',100);
    strcpy(p.name,directoryPath);
		memset(p.uname,'\0',32);
	  sprintf(p.uname,"%s",pw->pw_name);
	  sprintf(p.gname,"%s",getgrgid(t.st_gid)->gr_name);
	  sprintf(p.size,"%011lo",(off_t)0);
	  sprintf(p.mode,"%o",(mode_t)0755);
	  sprintf(p.uid,"%d",t.st_uid);
	  sprintf(p.gid,"%d",t.st_gid);
	  sprintf(p.mtime,"%011lo",current_time);
    memset(p.prefix,'\0',155);
    p.typeflag ='5';
	  sprintf(p.magic,"%s",TMAGIC);
		sprintf(p.version,"%s","");
	  set_checksum(&p);
      // se deplace a la fin du tarball , avant les deux blocks contenant

	
		if( lseek(fd,(off_t)0,SEEK_SET) == -1 ){

			perror(" newEmptyDirectory : Erreur seek");
			exit(1);
		}

        
			if(read(fd,&h,BLOCKSIZE) == -1){

				perror(" erreur read \n");
				exit(1);
			}

			while (h.name[0] != '\0'){

				sscanf(h.size,"%o",&filesize);
				//printf(" tversion : %s , t.magic : %s\n",h.version,h.magic);
				lseek(fd, (filesize % 512 == 0)? filesize : ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE, SEEK_CUR);
     			read(fd, &h, BLOCKSIZE);

			}
			
			// se positionner a la fin du fichier destination

			if( lseek(fd,-BLOCKSIZE,SEEK_CUR) == -1 ){

				perror("erreur lseek \n");
				exit(1);
			}

			char nuls[1024];// 2 blocks de 512
			memset(nuls,'\0',1024);

		
      		//inserer le header du repertoire
			if(write(fd,&p,BLOCKSIZE) <= 0  ){

				perror(" newEmptyDirectory : Erreur write ");
				exit(1);
			}

			// remettre les derniers blocks de null apres le header inseré

			if(write(fd,nuls,1024) <= 0  ){

			perror(" newEmptyDirectory : Erreur write ");
			exit(1);
			}

    }
    
  }
  
}


/*********************************************************************/
/* Partie  Suppression fichier et repertoire dans le fichier .tar   */
/*******************************************************************/

off_t trouve(int fd, char *filename){
  int filesize = 0;
  struct posix_header p;
  lseek(fd, 0, SEEK_SET);
  
  //on recopie le filename dans newFilename et enleve le / final
  char* newFilename = malloc(sizeof(char) * BUFFER);
  strcpy(newFilename,filename);	 
  if(  newFilename[strlen(newFilename)-1] == '/' ) {  newFilename[strlen(newFilename)-1] = '\0';}	
		
  if(fd < 0){
    perror("Fichier n'existe pas");
    exit(1);
  }
  read(fd, &p, BLOCKSIZE);
  sscanf(p.size,"%o",&filesize);
  while(p.name[0] != '\0' ){
  	//on compare a filename ( on cherche un repertoire precis) et a newfilename ( on cherche un fichier precis)
    if  ( ( strcmp(p.name , filename) == 0) || (strcmp(p.name , newFilename) == 0)){
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

	if( read(fd,content,filesize) < 0 ){

		perror(" Erreur de lecture  ");
		exit(1);

		
	}

	write(1,content,filesize);
	


}

// renvoi 1 si le prefix est inclus au debut de str
int startWith(const char *prefix, const char *str) {
	//printf("StartWith %s %s %d\n",str,prefix,strncmp(prefix,str,strlen(prefix)));
	if ( strncmp(prefix,str,strlen(prefix)) == 1 ) { return 1; }
	return 0;
}

void afficher_repertoire(int fd, off_t position, int mode , char* arboTar){
    struct posix_header p;
    unsigned int filesize ;
    time_t time;
    struct tm * m ;
    char * droits ;
    char  res [BUFFER] ;
    char mdate [15];

    if(lseek(fd , position, SEEK_SET) == -1 ){
        perror(" ERREUR lseek ");
        return;
    }

    if( read (fd , &p, BLOCKSIZE) <= 0 ){
        perror(" ERREUR read ");
        return;
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
        return;
    }

	  while(strcmp(p.name,"") != 0 ) {
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

             if(startWith(p.name,arboTar) == 1 ) write(1,res,strlen(res));


        }else{
            char name[strlen(p.name)+2];
            sprintf(name,"%s\n",p.name);
             if(startWith(p.name,arboTar) == 1 ) write(1, name, strlen(name));
        }
        

        sscanf(p.size,"%o",&filesize);

        if( lseek(fd,(filesize % 512 == 0)? filesize : ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE, SEEK_CUR)== -1){
            perror(" ERREUR read ");
            return;
        }

        if( read (fd , &p, BLOCKSIZE) <= 0 ){
	        perror(" ERREUR read ");
        	return;
        }
	}
}


void afficher_tar_content(int fd , int mode){
	printf("Afficher TAR CONTENT  \n");

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



/* copie le contenu d'un fichier  qui est a l'interieur d'un .tar  vers la destination */
void block_to_file(int fd, char * src_path, char* dst_path){

	// appeler trouv sur src_path
    off_t position;
	int cpt = strlen(src_path) - 1 ;
	int filesize ;
	char buf [250];
	char databuf[BLOCKSIZE];
	char filename[50];
	struct posix_header h;

	// recuperer le nom du fichier sans le chemin

	memset(filename,0,50);

	while ( (src_path[cpt] != '/') && (cpt >= 0)  )	cpt -- ;

	strncpy(filename,src_path+cpt+1,strlen(src_path)-(cpt+1));

	position = trouve(fd,src_path);

	if (position == -1 ){

			perror(" le fichier est inexistant \n");
			exit(1);
	}

	// se positionner au debut du block du fichier dans le tarball 

	if(lseek(fd,position,SEEK_SET) == -1 ){
			perror(" erreur lseek \n");
			exit(1);
	}

	if(read(fd,&h,BLOCKSIZE) == -1 ){

		perror("erreur read \n");
		exit(1);
	}


	sscanf(h.size,"%o",&filesize);

	// open with option create option

	
	memset(buf,0,250);
	strcpy(buf,dst_path);
	strcat(buf,"/");
	strcat(buf,filename);


	// ouvrir le fichier en ecriture et le creer a l'ouverture si il existe pas 
	int fd_dst = open(buf,O_WRONLY|O_CREAT,0777) ;

	if (fd_dst == -1 ){
		perror("erreur open \n");
		exit(1);

	}

 // lire le contenu du fichier et le copier dans fd1 ( la destination)

   int i = 0 ;

   while ( i < (filesize/BLOCKSIZE)){

	   read(fd,databuf,BLOCKSIZE);
	   write(fd_dst,databuf,BLOCKSIZE);
	   i++;
   }

	if ( (filesize % 512) > 0){

		read(fd,databuf,(filesize % 512));
		write(fd_dst,databuf,(filesize % 512));
	}

	close(fd_dst);

}


/* copie le contenu d'un repertoire qui est a l'interieur d'un tarball vers un repertoire externe 
 ( equivalent au desarchivage d'un repertoire ) */

void block_to_directory(int fd, char * src_path,char* dst_path){


			struct posix_header h ;
			off_t position;
			int filesize;
			mode_t mode;
			char path_buf[250];
			char directory_name [100];
			char * name;
			char * save_name ;
			char save_hname[100];
			char mkdir_path[250];


			// recuperer le nom du repertoire a partir de src_path sans le caractere '/' a la fin
			memset(directory_name,'\0',100);
			int cpt  = strlen(src_path) - 2 ;// on fait -2 pour eviter le '/' a la fin de src_path

			while( (src_path[cpt] != '/') && (cpt >= 0 ) ) cpt -- ;

			strncpy(directory_name,src_path+cpt+1,strlen(src_path)-(cpt+2));

			// creer le premier repertoire source dans la destination
			memset(mkdir_path,'\0',250);
			strcpy(mkdir_path,dst_path);
			strcat(mkdir_path,"/");
			strcat(mkdir_path,directory_name);

			position = trouve(fd,src_path);

			if( lseek(fd,position,SEEK_SET) == -1 ){
					perror(" erreur lseek \n");
					exit(1);
			}

			if(read(fd,&h,BLOCKSIZE) == -1){
				perror(" erreur read \n");
				exit(1);
			}

			sscanf(h.mode,"%o",&mode);

			mkdir(mkdir_path,mode);


			// parcourir le tarball en premier et creer tous
			// les sous repertoires du repertoire source

			if( lseek(fd,(off_t)0,SEEK_SET) == -1 ){

					perror(" lseek error \n");
					exit(1);
			}

			if( read(fd,&h,BLOCKSIZE) == -1 ){
				perror("erreur read \n");
				exit(1);
			}

			while (h.name[0] != '\0'){
				
				sscanf(h.size,"%o",&filesize);

				if(strncmp(src_path,h.name,strlen(src_path)) == 0 
				&& (strlen(h.name) > strlen(src_path)) ){

						memset(path_buf,'\0',250);
						strcpy(path_buf,mkdir_path);
						strcat(path_buf,"/");

						if(h.typeflag == '5'){ // un repertoire

							   name = strtok(h.name+strlen(src_path),"/");
								// je recupere le nom du repertoire et on fait 
					        while ( name != NULL)
							{
								strcat(path_buf,name);
								mkdir(path_buf,mode);
								strcat(path_buf,"/");

								name = strtok(NULL,"/");
							}
							
								
								// je fais un nouveau read pour verifier le block suivant

								if ( read(fd,&h,BLOCKSIZE) == -1 ){
									perror("erreur read \n");
									exit(1);
								}

						}else// autre type que repertoire
						{


							if( lseek(fd,((filesize % 512) == 0)?filesize:((filesize+512-1)/512)*512,SEEK_CUR) == -1){

								perror(" erreur lseek \n");
								exit(1);
							}

							if ( read(fd,&h,BLOCKSIZE) == -1 ){

									perror("erreur read \n");
									exit(1);
							}

						}
						
				}else{

						if( lseek(fd,((filesize % 512) == 0)?filesize:((filesize+512-1)/512)*512,SEEK_CUR) == -1){

								perror(" erreur lseek \n");
								exit(1);
							}

							if ( read(fd,&h,BLOCKSIZE) == -1 ){

									perror("erreur read \n");
									exit(1);
							}

				}


			}
			



			// parcourir le tarball une deuxieme fois pour copier les fichiers du repertoire source
			// et les fichiers de tout les sous repertoires

			if(lseek(fd,(off_t)0,SEEK_SET) == -1 ){

					perror("erreur lseek \n");
					exit(1);
			}

		
			if( read(fd,&h,BLOCKSIZE) == -1 ){
				perror("erreur read \n");
				exit(1);
			}

			while ( h.name[0] != '\0' ){

				sscanf(h.size,"%o",&filesize);

				if(strncmp(src_path,h.name,strlen(src_path)) == 0 
				&& (strlen(h.name) > strlen(src_path)) ){

				if ( h.typeflag != '5'){

				// pourchaque fichier composer d'abord son chemin dans le .tar avec mkdir_path
				// et l'envoyer a file_to_block pour le reste du traitement 


					memset(path_buf,'\0',250);
					strcpy(path_buf,mkdir_path);

					memset(save_hname,'\0',100);
					strcpy(save_hname,h.name);

					name = strtok(h.name+strlen(src_path),"/");

					while( name != NULL){

						save_name = name;
						name = strtok(NULL,"/");

						if(name == NULL ){
							 // sauv_name contient le nom d'un fichier
							 // -> lancer la fonction block_to_file pour effectuer la copie du fichier

							block_to_file(fd,save_hname,path_buf);



						}else// sauv_name contient le nom  d'un sous-repertoire dans le .tar , on le met dans path_buf
						{

						    strcat(path_buf,"/");
							strcat(path_buf,save_name);
						}
						
					}

					// lecture du prochain header

							// ajouter un lseek en cas ou la taille du fichier n'est pas divisible par 512

					lseek(fd,BLOCKSIZE -(filesize % BLOCKSIZE),SEEK_CUR);

					if( read(fd,&h,BLOCKSIZE) == -1 ){

							perror(" erreur read \n ");
							exit(1);

					}

				}else{ // un header repertoire , size = 0 -> on fait un read  directement pour passer au bloc suivant

						if( read(fd,&h,BLOCKSIZE) == -1 ){

							perror(" erreur read \n ");
							exit(1);

						}
				}

			}else{// on passe au fichier/repertoire suivant

              lseek(fd, (filesize % 512 == 0)? filesize : ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE, SEEK_CUR);

						if( read(fd,&h,BLOCKSIZE) == -1 ){

							perror(" erreur read \n ");
							exit(1);

						}
			}

			
		}

}

/* copier le contenu d'un tarball source vers un tarball destination 
 - si on veut copier le tarball source tout entier on met src_path a vide , sinon on met le chemin 
  ( le champ nom dans le tarball) du fichier ou repertoire concerné 
 - si on veut copier dans le  tarball destination tout  on met dst_path a vide , sinon on met le chemin 
  ( le champ nom dans le tarball) du repertoire concerné où on veut copier notre source.
  fd_src -> ouvert avec droit de lecture 
  fd_dst -> ouvert avec droit lecture et d'ecriture
 */

void copy_tarball_into_tarball(char * src_path ,int fd_src ,char * dst_path, int fd_dst ){

			// on parcourt le .tar entier
			// copier les blocks un par un et les inserer a la fin du .tar de la destination
			struct posix_header h ,h1;
			int i , nb_blocks , cpt ;
			int filesize , lenpath;
			char * word;
			char name_buffer[100];
			char source_name[100];
			char source_adapter[100];
			char buffer[BLOCKSIZE];

			// recuperer la source precise et enlever tous les repertoires parents du chemin source
			lenpath = strlen(src_path);
			if (src_path[lenpath-1]=='/'){
				cpt = lenpath -2 ;
			}else
			{
				cpt = lenpath -1;
			}
			
			while (src_path[cpt] != '/' && cpt >= 0) cpt --;
			
			memset(source_name,'\0',100);

			strncpy(source_name,src_path+cpt+1,lenpath-(cpt+1));
      

      if ( lseek(fd_dst,(off_t)0,SEEK_SET) == -1 ){
        perror("erreur lseek \n");
        exit(1);
      }

			if(read(fd_dst,&h1,BLOCKSIZE) == -1){

				perror(" erreur read \n");
				exit(1);
			}

			while (h1.name[0] != '\0'){

				sscanf(h1.size,"%o",&filesize);
				lseek(fd_dst, (filesize % 512 == 0)? filesize : ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE, SEEK_CUR);
     			read(fd_dst, &h1, BLOCKSIZE);

			}
			
			// se positionner a la fin du fichier destination

			if( lseek(fd_dst,-BLOCKSIZE,SEEK_CUR) == -1 ){

				perror("erreur lseek \n");
				exit(1);
			}


			if(read(fd_src,&h,BLOCKSIZE) == -1){

				perror(" erreur read \n");
				exit(1);
			}

      while( h.name[0] != '\0'){

					sscanf(h.size,"%o",&filesize);
				if( strncmp(src_path,h.name,strlen(src_path)) == 0 ){
					cpt = 0 ;
					memset(name_buffer,'\0',100);
					strcpy(name_buffer,dst_path);
					strcat(name_buffer,source_name);
					memset(source_adapter,'\0',100);
					strcpy(source_adapter,h.name);
					word = strtok(source_adapter,"/");

					while(strncmp(word,source_name,strlen(word)) != 0 ){

						word = strtok(NULL,"/");
					}

					word = strtok(NULL,"/");

					while (word != NULL){

						if( (cpt != 0) || (strcmp(name_buffer,"")!= 0) )strcat(name_buffer,"/");
						strcat(name_buffer,word);
						cpt ++;
						word = strtok(NULL,"/");

					}

					if(h.typeflag == '5' ) strcat(name_buffer,"/");

					memset(h.name,'\0',100);
					sprintf(h.name,"%s",name_buffer);
					set_checksum(&h);
					write(fd_dst,&h,BLOCKSIZE);
					i = 0 ;

					nb_blocks = (filesize % BLOCKSIZE == 0)? (filesize/BLOCKSIZE) : ((filesize + BLOCKSIZE - 1)/BLOCKSIZE) ;

					while (i < nb_blocks ){
					
					read(fd_src,buffer,BLOCKSIZE);
					write(fd_dst,buffer,BLOCKSIZE);
					i++;

				  }
				
				}else{

					lseek(fd_src, (filesize % 512 == 0)? filesize : ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE, SEEK_CUR);


				}


				if(read(fd_src,&h,BLOCKSIZE) == -1){

					perror(" erreur read \n");
					exit(1);
				}

				   

			}


            // ajout des deux blocs nuls
			memset(buffer,'\0',512);

			write(fd_dst,buffer,BLOCKSIZE);
			write(fd_dst,buffer,BLOCKSIZE);

}

/*
	effectue la copie d'un repertoire source ( et tout son contenu ) dans un tarball (ou sous-repertoire d'un
	tarball )
	- src_path -> indique le chemin absolu vers le repertoire source
	- dst_path -> indique le chemin ( le nom ) du sous repetoire du tarball , on le met a vide si on veut copier
	seulement dans le tarball.
	- fd_dst -> descripteur du tarball destination /-- doit etre ouvert en lecture et ecriture --/ 
*/
void copy_directory_to_tarball(char * src_path ,char * dst_path , int fd_dst){


		char directory_name [100];
		char directory_path_src[100];
		char mkdir_path[100];
		char file_path_src[100];
		char file_path_dst[100];

		struct posix_header h ;
		int filesize;
		off_t position ;

		int fd_file;

		int len = strlen(src_path)-1;

		// recuperer le nom du repertoire à copier

		while(src_path[len] != '/' && len > 0){
					len -- ;
		}

		memset(directory_name,'\0',100);
		strncpy(directory_name,src_path+len+1,strlen(src_path)-(len+1));

		// creer le repertoire dans le tarball destination
		memset(mkdir_path,'\0',100);

		strcpy(mkdir_path,dst_path);
		printf("directory_name : %s \n",directory_name);
		strcat(mkdir_path,directory_name);
		strcat(mkdir_path,"/");

		newEmptyDirectory(fd_dst,mkdir_path);

		// readdir
		// copie des fichiers dansla destination + une copie recursif des sous repertoires

		DIR * d = 	NULL;
		struct dirent * dt ;

		d = opendir(src_path);

		if (d == NULL){

			perror(" erreur opendir ");
			exit(1);
		}

		while  ((dt = readdir(d))  != NULL) 
		{

			// eviter les repertoires . et .. pour ne pas tomber dans une boucle infinie
		 	if((strcmp(dt->d_name,".") != 0) && (strcmp(dt->d_name,"..") != 0)){ 

				if(dt->d_type != DT_DIR){ // un fichier

					// construire le chemin vers le fichier source
					memset(file_path_src,'\0',100);
					strcpy(file_path_src,src_path);
					strcat(file_path_src,"/");
					strcat(file_path_src,dt->d_name);

					fd_file = open(file_path_src,O_RDONLY);

					if(fd_file == -1 ){
						perror("erreur open \n");
						exit(1);
					}

					// construire le chemin du fichier dans le tarball destination
					memset(file_path_dst,'\0',100);
					if(strcmp(dst_path,"") == 0){
				        strcpy(file_path_dst,directory_name);
						strcat(file_path_dst,"/");
						strcat(file_path_dst,dt->d_name);
					}else
					{
						strcpy(file_path_dst,dst_path);
						strcat(file_path_dst,directory_name);
						strcat(file_path_dst,"/");
						strcat(file_path_dst,dt->d_name);

					}

					
					// chercher la position de fin du tarball 
					if(  lseek(fd_dst,0,SEEK_SET) == -1 ){

						perror("erreur lseek \n");
						exit(1);
					}

					// chercher la position de fin du tarball destination
					if(read(fd_dst,&h,BLOCKSIZE) == -1){

						perror(" erreur read \n");
						exit(1);
					}

					while (h.name[0] != '\0'){

						sscanf(h.size,"%o",&filesize);
						lseek(fd_dst, (filesize % 512 == 0)? filesize : ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE, SEEK_CUR);
     					read(fd_dst, &h, BLOCKSIZE);

					}
			
					// se positionner a la fin du fichier destination
					position = lseek(fd_dst,-BLOCKSIZE,SEEK_CUR);
					// ouvrir le fichier en lecture
					addFile(fd_dst,fd_file,file_path_dst,position);

					close(fd_file);

				}else{// un repertoire 

					// construire le chemin vers le repertoire source 
					memset(directory_path_src,'\0',100);
					strcpy(directory_path_src,src_path);
					strcat(directory_path_src,"/");
					strcat(directory_path_src,dt->d_name);

					copy_directory_to_tarball(directory_path_src,mkdir_path,fd_dst);

				}
			
		 	}
		}
		 free(dt);
		closedir(d);


	
}

/*cp*
    analyse les arguments source et destionation d'une commande
	retourne -1 si les arguments ne satisfont pas les conditions

	pwd -> la variable globale de notre programme , qui indique l'emplacement courant
*/

/* execute la commande cp avec la source est soit un tarball entier ou un repertoire
ou un fichier qui se trouve dans le tarball et la destination peut etre soit un tarball ou un repertoire externe 
src_path -> indique le chemin du fichier(ou repertoire) 
src_fd -> ouverture relatif a la source ( le tarball ) / -- l'ouverture doit etre en lecture -- /
dst_path -> chemin absolu menant vers la destination , sinon le chemin d'un repertoire a l'interieur d'un tarball sinon 
vide si on copie dans un tarball.
dst_fd -> ouverture drelatif a la destination si destination est un taball , sinon on le met a -1
           / -- l'ouverture doit etre en lecture et ecriture --/

option -> valeur est 1 si la commande est avec l'oprion -r , 0 sinon
*/  

int cp_srctar( char * src_path , int src_fd , char * dst_path  , int dst_fd , int option ){

	char type ;

  
	if ( dst_fd == -1){// destination n'est pas un tar 

			struct stat s ;
      
			if (stat(dst_path,&s) == -1 ){
				perror("erreur stat ");
				exit(1);

			}

			if ( !S_ISDIR(s.st_mode)){ // destination n'est pas un repertoire

				perror(" la destinatination fournie n'est pas un repertoire");
				return -1 ;
			}

			// la destination est bien un repertoire 
		
			// analyse de la source 

			if(strcmp(src_path,"") != 0){
				
				type = get_fichier_type(src_fd,src_path,0);
				if ( type == '5'){// un repetoire

						if(!option){
							perror(" veuillez utiliser l'option -r pour copier un repertoire \n");
							exit(1);
						}

					// blocktodirectory
					block_to_directory(src_fd,src_path,dst_path);

				}else{ // un fichier

					// blocktofile
					block_to_file(src_fd,src_path,dst_path);


				}

			}else{// la source est le fichier .tar entier , on le fait avec le cp qui existe deja dans le shell

				return -1;
			}
			
			


	}else{ // destination est un fichier .tar 

			/* 	source est le fichier .tar entier 
				appeler copy_tarball_to_tarball pour effectuer la copie
				du tarball source dans le tarball destination */

      if(strcmp(src_path,"") != 0){
				
				type = get_fichier_type(src_fd,src_path,0);
				if ( type == '5'){// un repetoire

						if(!option){
							perror(" veuillez utiliser l'option -r pour copier un repertoire \n");
							exit(1);
						}
        }
      }

			copy_tarball_into_tarball(src_path,src_fd,dst_path,dst_fd);


	}

	return 0;
}


/*
implementation de la partie de la commande cp qui effectue 
la copie a partir du source simple (ie , qui n'est pas un tarball)
src_path -> donne le chemin absolu de la source 
dst_path -> donne le chemin du repertoire destination a l'interieur du tarball, et si on veut
copier dans le tarball ( ie inserer a la fin du tarball) on donne une chaine vide
dst_fd ->  donne le descripteur du tarball destiantion 
            /-- il doit etre ouvert en lecture et ecriture --/

*/
int cp_srcsimple( char * src_path , char * dst_path  , int dst_fd , int option ){


		struct stat s ;
		struct posix_header h;
		char * word ;
		char * save_word ;
		int filesize ,file_fd;
		off_t position;
		// faire un stat pour avoir le type de source
		if (stat(src_path,&s) == -1 ){
			perror("erreur stat \n");
			exit(1);
		}

		if(S_ISDIR(s.st_mode)){
			// si source est un repertoire 

			if(!option){

				// si l'option n'est pas faite on affiche une erreur
				perror(" veuillez utiliser l'option -r pour copier un repertoire \n");
				exit(1);

			}else{

				// sinon on fait une copie du repertoire vers le tarball
				copy_directory_to_tarball(src_path,dst_path,dst_fd);
			}

		}else{

			// sinon (un fichier)
			if(  lseek(dst_fd,0,SEEK_SET) == -1 ){

				perror("erreur lseek \n");
				exit(1);
			}

			// chercher la position de fin du tarball destination
			if(read(dst_fd,&h,BLOCKSIZE) == -1){

				perror(" erreur read \n");
				exit(1);
			}

			while (h.name[0] != '\0'){

				sscanf(h.size,"%o",&filesize);
				lseek(dst_fd, (filesize % 512 == 0)? filesize : ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE, SEEK_CUR);
     			read(dst_fd, &h, BLOCKSIZE);

			}
			
			// se positionner a la fin du fichier destination
			position = lseek(dst_fd,-BLOCKSIZE,SEEK_CUR);	

			if(  position == -1 ){

				perror("erreur lseek \n");
				exit(1);
			}

			// on appelle la fonction addFile

			file_fd = open(src_path,O_RDONLY);

			if (file_fd == -1 ){

				perror(" erreur open");
				exit(1);
			}

			// recuperer le nom du fichier 
			word = strtok(src_path,"/");

			while( word != NULL ){

				save_word = word ;
				word = strtok(NULL,"/");
			}

			addFile(dst_fd,file_fd,save_word,position);
			close(file_fd);

		}

	return 0;
}
