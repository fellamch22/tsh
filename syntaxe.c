#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "syntaxe.h"


/* analyse le chemin absolu donne en parametre 

  si le chemin mene a un tarball elle retourne  le chemin a l'interieur du tarball et le descripteur du tarball
  sinon elle retourne null et -1 dans fd

*/
char * analyser_path ( char * path , int * fd ){


	char path_save [100];
	char path_vers_tarball[100];
	char * path_interieur_tarball ;
	char * word ;

	int trouv_tar =0 ;

	// initialisation 
	memset(path_save,'\0',100);
	path_interieur_tarball = malloc(100);
	memset(path_interieur_tarball,'\0',100);
	memset(path_vers_tarball,'\0',100);


	strcpy(path_save,path);

   // analyse du chemin
	word = strtok(path_save,"/");

	while ( word != NULL )
	{
		printf(" %s \n ", word);
		if(strlen(word) > 4 && (path_interieur_tarball[0] =='\0')){
			// tester si le mot contient une extension .tar 

			if(strcmp(word+strlen(word)-4,".tar") == 0 ) trouv_tar = 1	;
			
			
			strcat(path_vers_tarball,"/");
			strcat(path_vers_tarball,word);

		}else{

			if(trouv_tar){ // on met le mot dans le chemin interne

				if(path_interieur_tarball[0]=='\0'){ // si c'est le premier mot

					strcpy(path_interieur_tarball,word);

				}else// il existe deja un mot
				{
					strcat(path_interieur_tarball,"/");
					strcat(path_interieur_tarball,word);
				}
				

			}else{// on met dans le chemin vers le tarball

				if(path_vers_tarball[0] == '\0'){ // si c'est le premier mot

					strcpy(path_vers_tarball,"/");
					strcat(path_vers_tarball,word);

				}else// il existe deja un mot
				{
					strcat(path_vers_tarball,"/");
					strcat(path_vers_tarball,word);
				}

			}
		}

		// obtenir le mot suivant
		word = strtok(NULL,"/");
		
	}
	
	if( trouv_tar ){// si on est dans un tar , 


		//si l'interieur different du vide on verifie le slash '/'  a la fin du path (celui des repertoires)

		if( (path_interieur_tarball[0] !='\0') && (path[strlen(path)-1] == '/') )
				 strcat(path_interieur_tarball,"/");



		// ouvrir le fichier ( en lecture et ecriture)

		(*fd) = open(path_vers_tarball,O_RDWR);

	}else{
		// si on est pas dans un tar
		// mettre fd a -1 
	 	(*fd) = -1 ;
	}

	return path_interieur_tarball;


}



int main(int argc, char* argv[]){

	int fd = -1 ;
	printf("le chemin : %s \n",analyser_path("/home/fella/Desktop/toto.tar/a/",&fd));
	printf("%d \n",fd);
    if( fd != -1 ) close(fd);
    return 0;
}