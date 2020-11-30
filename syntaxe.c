#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "syntaxe.h"




int analyser_src_dest (char * pwd ,char * source , char* destination , int* is_srctar , int* is_destar , int*
is_srcfile , int* is_destdir, char * srcpart1 ,char* srcpart2,char* destpart1 ,char* destpart2 ){


/* ne pas oublier de liberer l'espace memoire */
int src_len = -1;
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



// parcourir source1  pour detecter si il ya un .tar dans le chemin et mettre a jour

strbuf = strtok(source,"/");
part1 = 1 ;

while ( strbuf != NULL ){

    if( strlen(strbuf) > 4){ // pour voir si il termine par .tar

        if( strncmp(strbuf[strlen(strbuf)-4],".tar",4) == 0 ){

			(*is_srctar) = 1;
			if(src_len != -1) snprintf(srcpart1,src_len,"%s",pwd); 
			src_len = -1 ;
			strcat(srcpart1,"/");
			strcat(srcpart1,strbuf);
			//free(strbuf); - to check

			part1 = 0 ;
		}else if( part1){ // on remplie la premiere partie de source avant de rencontrerun .tar

				if( strcmp(srcpart1,"") != 0 ) strcat(srcpart1,"/");

				if(strcmp(strbuf,"~")==0){ // chemin qui commence par tilde non acceptable
  				/* on met dans srcpart1 le chemin forme par home/username/ , car les fonctions open et read n'acceptent pas
   				le cehmin qui commencent par tilde */

   				sprintf(srcpart1,"/home/%s/",getpwuid(getuid())->pw_name);

				}else if (strcmp(strbuf,".") == 0){
			 	// chemin relatif qui commence par '.' , on le remplace par le chemin absolu du repertoire courant

				 sprintf(srcpart1 ,"%s",pwd);

				}else if( strncmp(source,"..",2) == 0){ 

 					// chemin relatif qui commence par '.' , on le remplace par le chemin absolu du repertoire courant
					// on retire le dernier repetoire du chemin courant --> need to find a solution
					// je parcours src depuis la fin et je decrmente un compteur
					// apres je copie avec snprintf
					if ( src_len == -1 ) src_len = strlen(pwd) - 1;

  					while(pwd[src_len ] != '/')  src_len -- ;
	
				}else{
		
						if(src_len != -1) snprintf(srcpart1,src_len,"%s/",pwd); 
						src_len = -1 ;
						strcat(srcpart1,strbuf);
				}

		}else{ // on remplie la deuxieme partie de source apres recontrer un .tar

				if( strcmp(srcpart2,"") != 0 ) strcat(srcpart2,"/");
				strcat(srcpart2,strbuf);

		}
		
	}

	strbuf = strtok(source,"/");

 
}

if(src_len != -1) snprintf(srcpart1,src_len,"%s/",pwd); 
src_len = -1 ;

// si partie 2 est remplie , on verifie la fin de source ( pour ne pas negliger le '/' de la fin  d'un repertoire dans un .tar )
if(!part1 && srccopie[strlen(srccopie)-1] == '/') strcat(srcpart2,"/");


// ida kan tar diri open  w hawssi b getfichiertype
if( (*is_srctar) ){
   // je vois seulement la fin si c'est / c'est un repertoire , else it's file 
   // (je ne teste pas avec un open et trouve pour diminuer le cout )
   is_srcfile = (srcpart2[strlen(srcpart2)-1] == '/') ? 0 : 1;

}else{
   // source n'et pas un tar , on fait un stat 
   // ida makach tar diri stat direct w djibi type tae hedek lfichier
   struct stat s ;
   if ( stat(srcpart1,&s)== -1){
       perror(" argument incorrect ");
       exit(1);
   }

   is_srcfile = (S_ISDIR(s.st_mode))? 0 : 1;

{

// analyser  la destination 

// faire de meme avec la destination

}

// strtok

	return 0;
}