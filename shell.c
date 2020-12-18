#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "sgf.h"
#define BUFFER 1024
#define LECTURE  0
#define ECRITURE 1

char* arguments[BUFFER]; // tableau d'arguments de chaque sous commande
char* pwd; // current dir
char* old_pwd; // copy old pwd - used for "cd -"
int nbexecuteCmds = 0; // nombre de sous commandes executees - utilise pour CleanUp
int nbargs;
int debug=0; // Debug mode disabled by default


//convertChemin permet la Convertion chemin absolu (si relatif) ,  la Gestion des ".." et supprime le "/" final si present
// "toto.tar/toto/titi/../titi/f3/" =>  "<pwd>/toto.tar/toto/titi/f3"
 char* convertChemin(char* chemin){	
	//VARIABLES
        char* Temp = malloc(sizeof(char) * BUFFER);
        char* NewChemin = malloc(sizeof(char) * BUFFER);
        strcpy(Temp,"");
        strcpy(NewChemin,chemin);
            	
        //si arg commence par '/' , chemin absolu
      	if(  NewChemin[0] == '/' ) {
			strcpy(Temp, NewChemin); 
		}
		//sinon chemin relatif , on ajoute l'arg a la suite du pwd actuel
		else {
			strcpy(Temp,pwd);
			strcat(Temp,"/");
			strcat(Temp,NewChemin);
		}	
		
		strcpy(NewChemin,Temp);
		//  chemin est ici le chemin complet ou on souhaite aller
				
		//remove the ".."
		strcpy(Temp,"");
		char *p = strtok(NewChemin, "/");
		strcat(Temp,"/") ;
		while(p != NULL)	{ 
						    
			// des que l'on rencontre le motif "..", on efface la sous partie précédente
			if(strstr(p,"..") != 0 ) { 								  
				for(int i = strlen(Temp)-2; i > 0; i--){
	                if(Temp[i] == '/'){
	                    Temp[i+1] = '\0';
	                    break;
	                }
					else{
	                	Temp[i] = '\0';
	                }
	            }
			}
			else{
				//si on ne rencontre pas le motif ".." , on ajoute la suite de la sous partie p a la commande finale pwdtmp2						
				strcat(Temp , p);
				strcat(Temp , "/");
			}
			p = strtok(NULL, "/");
		}
		
		//suppression du dernier / dans la commande demandee => xx/toto.tar/ devient xx/toto.tar
		if(  Temp[strlen(Temp)-1] == '/' ) {
		   Temp[strlen(Temp)-1] = '\0';
		}
    	
    	//Cas si on est a la racinecd -
    	if(strcmp(Temp,"") == 0) {strcpy(Temp,"/");}
		
		
		return Temp;
}


//findGoodPath permet de renvoyer le chemin correct a utiliser selon si le motif ".tar" est present dans le pwd ou arg1 ou arg2
 char* findGoodPath() {
		char* pwdtmp=malloc(sizeof(char) * BUFFER);
				//si l'arg1 contient le motif ".tar" on copie l'arg1 (avec convertion chemin relatif/absolu)  dans pdwtmp 
  			if ( (arguments[1] != NULL) &&  (strstr(arguments[1], ".tar") != NULL) ) {
				strcpy(pwdtmp,convertChemin(arguments[1]));  				 
			}
			// cas du ls -l	 : on regarde l'arg2 => si l'arg2 contient le motif ".tar" on copie l'arg2 (avec convertion chemin relatif/absolu)  dans pdwtmp 
			else if ( (arguments[2] != NULL) &&  (strstr(arguments[2], ".tar") != NULL) ) { 
				strcpy(pwdtmp,convertChemin(arguments[2]));
			}
			// enfin si le pwd contient le motif ".tar" on le recopie dans pwdtmp
			else if  (strstr(pwd, ".tar") != NULL ) { 
				strcpy(pwdtmp,pwd); 
				if 	 ( (arguments[1] != NULL) &&  (strcmp(arguments[1], "-l") != 0 ) )  { 
				    strcat(pwdtmp,"/");
					strcat(pwdtmp,arguments[1]);
				}
				if 	 (arguments[2] != NULL) {
					strcat(pwdtmp,"/");
					strcat(pwdtmp,arguments[2]);
				}
			}
			return convertChemin(pwdtmp);
}

 
//UseRedefCmd permet de voir si pwd ou arg1 ou arg2 contient le motif ".tar" , et si oui renvoi 1
 int UseRedefCmd() {
	if (	
		(strstr(pwd, ".tar") != NULL ) // si le pwd contient ".tar"
		|| 
		( (arguments[1] != NULL)  &&  (strstr(arguments[1], ".tar") != NULL ) )  // ou que arg1 existe avec un ".tar" dedans
		||
		( (arguments[2] != NULL)  &&  (strstr(arguments[2], ".tar") != NULL ) )  // ou que arg2 existe avec un ".tar" dedans (si arg1 = -l	)
		)
	{ 
			return 1; 
	}
	return 0;	
} 
 
 
//getTarPath permet de recuperer le chemin vers un tar a partir d'un chemin
// convert /home/lifang/Shell/toto.tar/toto/titi => /home/lifang/Shell/toto.tar 
  char* getTarPath(char*chemin) {
 				char* tarname=malloc(sizeof(char) * BUFFER);
 				strcpy(tarname,"/");
 				char* newchemin = malloc(sizeof(char) * BUFFER);
 				strcpy(newchemin,chemin);
 				int t=0;
				char *p = strtok(newchemin, "/");
				while(p != NULL)	{ 
					if(strstr(p,".tar") != 0) {
						t=1; // t = flag to switch the result on tarname or on arbotar
						strcat(tarname,p); 
					}
					else if (t == 0) {
					//ajout tarname
						strcat(tarname,p);
						strcat(tarname,"/") ;
					}	
					//Debug  printf("'%s'\n", p);
					p = strtok(NULL, "/");
				}
				return tarname;
 }
 
//getTarArbo permet de recuperer l'arborescence d'un tar a partir d'un chemin
// convert /home/lifang/Shell/toto.tar/toto/titi => toto/titi 
  char* getTarArbo(char*chemin) {
 				char* arboTar=malloc(sizeof(char) * BUFFER);
 				strcpy(arboTar,"");
 				char* newchemin = malloc(sizeof(char) * BUFFER);
 				strcpy(newchemin,chemin);
 				int t=0;
				char *p = strtok(newchemin, "/");
				while(p != NULL)	{ 
					if(strstr(p,".tar") != 0) {
						t=1; // t = flag to switch the result on tarname or on arbotar
					}
					else if (t==1) {
					//ajout arboTar					
						strcat(arboTar,p) ; 
						strcat(arboTar,"/") ; 
					}
					//Debug  printf("'%s'\n", p);
					p = strtok(NULL, "/");
				}
				return arboTar;
 }
 
//Redefinition de la fonction cat 
 int cat(){
			//VARIABLES du fils
     		char* pwdtmp=malloc(sizeof(char) * BUFFER);
			char* tarname=malloc(sizeof(char) * BUFFER);
			char* arboTar=malloc(sizeof(char) * BUFFER);
			strcpy(pwdtmp,"");
			strcpy(tarname,"/");
			strcpy(arboTar,"");
		
   		 	if ( debug == 1 ) { printf("COMMANDE REDEFINIE !\n"); }  
   		 	
			strcpy(pwdtmp,findGoodPath());
			if ( debug == 1 ) printf("PWDTMP = %s\n",pwdtmp);
			
			//Exception pour le cas ou on fait un cat hors du tar depuis le tar ex :   pwd = /home/lifang/Shell/toto.tar/toto$> cat ../../f2
			if(strstr(pwdtmp, ".tar") == NULL ) {
				if ( debug == 1 ) {printf("on sort du TAR , commande normale %s\n",pwdtmp); }
				strcpy(arguments[1],pwdtmp);
				execvp( arguments[0], arguments);
			}
			
			//Decoupe pwdtmp en 2 avec des / afin d'extraire le tarname et l'arborescence dans le tar
			//ex avec pwdtmp = /home/lifang/Shell/toto.tar/toto/titi => tarname = /home/lifang/Shell/toto.tar    arboTar = toto/titi  
			strcpy(tarname,getTarPath(pwdtmp));
		    strcpy(arboTar,getTarArbo(pwdtmp));
		    
			int fdxx = open(tarname, O_RDONLY);
            if (fdxx < 0){
	            perror("open");
	            return -1;
            }   
			afficher_fichier(fdxx, arboTar);
			close(fdxx);	
			free(pwdtmp);
			free(tarname);
			free(arboTar);	
			return 0;
}

//Redefinition de la fonction ls
 int ls(){
        	//VARIABLES du fils
     		char* pwdtmp=malloc(sizeof(char) * BUFFER);
			char* tarname=malloc(sizeof(char) * BUFFER);
			char* arboTar=malloc(sizeof(char) * BUFFER);
 			strcpy(pwdtmp,"");
			strcpy(tarname,"/");
			strcpy(arboTar,"");

  		 	if ( debug == 1 ) { printf("COMMANDE REDEFINIE !\n"); }

				
  			//on set pwdtmp comme le repertoire a analyser, il prend la valeur soit de pwd soit de arg1 soit de arg2. 
		    strcpy(pwdtmp,findGoodPath());

			//ici pwdtmp est a jour avec le bon chemin.
			 if ( debug == 1 ) { printf("pwdtmp = %s\n",pwdtmp); }

			//Exception pour le cas ou on fait un cat hors du tar depuis le tar ex :   pwd = /home/lifang/Shell/toto.tar/toto$> ls ../../f2
			if(strstr(pwdtmp, ".tar") == NULL ) {
				if ( debug == 1 ) {printf("on sort du TAR , commande normale %s\n",pwdtmp); }
				strcpy(arguments[1],pwdtmp);
				execvp( arguments[0], arguments);
			}
			
			//Decoupe pwdtmp en 2 avec des / afin d'extraire le tarname et l'arborescence dans le tar
			//ex : pwdtmp = /home/lifang/Shell/toto.tar/toto/titi => tarname = /home/lifang/Shell/toto.tar    arboTar = toto/titi
			 strcpy(tarname,getTarPath(pwdtmp));
		     strcpy(arboTar,getTarArbo(pwdtmp));
			
  			//appel aux fonctions d'affichage : afficher_tar_content si on est a la racine du tar , afficher_repertoire si on est plus loin , avec arboTar en argument
  			int fdxx;
                //afficher repertoire et droit "ls -l" 
                if(  (arguments[1] != NULL) && (!strcmp(arguments[1], "-l")) ){   // si l'arg1 = -l , on lance les fonctions d'affichage repertoire ou du tar directement avec la valeur 1 en argument
                    fdxx = open(tarname, O_RDONLY);
                    if (fdxx < 0){
	                    perror("open");
	                    return -1;
                    }
                   if  ( (strcmp(arguments[0], "ls") == 0)  && (strcmp(arboTar, "") != 0) )  { afficher_repertoire(fdxx, trouve(fdxx,arboTar), 1); }
                   if ( (strcmp(arguments[0], "ls") == 0) && (strcmp(arboTar, "") == 0) ) { afficher_tar_content(fdxx ,1);  } // ls a la racine du tar

                }else{ 
				// si l'arg1 n'est pas -l
                    fdxx = open(tarname, O_RDONLY);
                    if (fdxx < 0){
	                    perror("open");
	                    return -1;
                    }
                     if ( (strcmp(arguments[0], "ls") == 0) && (strcmp(arboTar, "") != 0) ) { afficher_repertoire(fdxx, trouve(fdxx,arboTar), 0); } // ls plus loin que la racine du tar
                     if ( (strcmp(arguments[0], "ls") == 0) && (strcmp(arboTar, "") == 0) ) { afficher_tar_content(fdxx ,0);  } // ls a la racine du tar                     
                }
            close(fdxx);
            free(pwdtmp);
			free(tarname);
			free(arboTar);	
			return 0;
}


 // executeCmd : Execution des commandes via creation d'un fils temporaire
 //
 // fd: numero du file descriptor , 0 pour le premier fils
 // debut: 1 si il s'agit de la premiere sous commande dans la commande principale
 // dernier: 1 si il s'agit de la derniere sous commande dans la commande principale
 //
 // EXEMPLE avec la commande "ls -l | head -n 2 | wc -l" :
 //    fd1  = executeCmd(0, 1, 0), with args[0] = "ls" and args[1] = "-l"
 //    fd2  = executeCmd(fd1, 0, 0), with args[0] = "head" and args[1] = "-n" and args[2] = "2"
 //    fd3  = executeCmd(fd2, 0, 1), with args[0] = "wc" and args[1] = "-l"
 //
 int executeCmd(int fd, int debut, int dernier)
{
	if ( debug == 1 ) printf("Le pere entre dans executeCMD - FD = %d , DEBUT = %d , DERNIER = %d pour y lancer %s\n",fd,debut,dernier,arguments[0]);
	
	//Variables
    int tubes[2];
 	pid_t pid;
 	
    //creation des pipe et fils
    pipe(tubes);
    pid=fork();
 
    if (pid == 0) {     
        //le fils
        if ( debug == 1 ) {
			 printf("(PID = %d) Child Exec <%s",getpid(),arguments[0]);	        
	        for(int i=1;i<sizeof(arguments)/sizeof(arguments[0]); i++) {
	            if (arguments[i] != NULL) printf(" %s",arguments[i]);
	        }	        
	        printf("> fd=%d debut=%d dernier=%d\n",fd,debut,dernier); 
   		}
   		
        fflush(stdout);
        
        //##### Gestion des divers processus fils tel un train
        if (fd == 0 && debut == 1 && dernier == 0  ) {
            // pour la debut commande redirection de la sortie dans le tube
            dup2( tubes[ECRITURE], STDOUT_FILENO ); // on change la sortie standard par l'entrée du pipe
        } else if (fd != 0 && debut == 0 && dernier == 0 ) {
            // pour les commende du milieu, ecoute de l entree depuis fd,
            // redirection de la sortie dans tubes[ECRITURE]
            dup2(fd, STDIN_FILENO); // on change l'entrée standard  le fd pere
            dup2(tubes[ECRITURE], STDOUT_FILENO); // on sochangert la sortie standard par l'entrée du pipe
        } else { // dernier=1
            // pour la dernier commande , ecoute de l entree depuis fd
            dup2( fd, STDIN_FILENO ); // on change l'entrée standard  le fd pere
        }
        
        //##### REDEFINITION COMMANDE PWD
		if (strcmp(arguments[0], "pwd") == 0){ //pwd passe en tarball
            fprintf(stdout, "%s\n", pwd);
		    fflush(stdout);
    		exit(0);
        }
        
         //##### REDEFINITION COMMANDE CAT   uniquement si le pwd contient ".tar" ou si l'argument 1 existe et contient ".tar" (UseRedefCmd)
        else if ( (strcmp(arguments[0], "cat") == 0) && ( UseRedefCmd() == 1 ) ) {
            int ret = cat();
			fflush(stdout);
    		exit(ret); // kill le fils          
		}

        //##### REDEFINITION COMMANDES LS   uniquement si le pwd contient ".tar" ou si l'argument 1 existe et contient ".tar" ou l'arg 2 existe et contient ".tar" (UseRedefCmd)
        else if (  (strcmp(arguments[0], "ls") == 0)  && ( UseRedefCmd() == 1 ))   //si la commande est ls ou cat et rempli les conditions de redef
		 { 
            int ret = ls();
		    fflush(stdout);
    		exit(ret); // kill le fils
        }
        else if (execvp( arguments[0], arguments) == -1) {
            perror("Commande Inconnue \n");
            fflush(stdout); 
			kill(getpid(),SIGTERM);
            return 1; // kill le fils pour eviter zombie
        }
    }// fin fils
    
    //nettoyage fd
    if (fd != 0) {
        close(fd);
    }
 
    // plus rien ne doit etre ecrit
    close(tubes[ECRITURE]);
 
    // plus rien ne doit etre lu si il s'agit de la dernier executeCmde
    if (dernier == 1) {
        close(tubes[LECTURE]);
    }
    return tubes[LECTURE];
}
 
//le pere attend le retour de tous ses fils apres l'execution d une commande (et donc de x sous commandes)
 void attenteDuPere(int n)
{
    int i;
    for (i = 0; i < n; ++i)
        wait(NULL);
}

//fonction pour enlever les espaces multiples :  "ls     -l" => "ls -l"  
 char* removeSpace(char* str)
{
    while ( isspace(*str) ) { ++str; }
    return str;
}

// decoupe la sous commande  dans le tableau d'arguments. ex: ls -l => arguments[0] = "ls" , arguments[1] = "-l" 
 void decoupe(char* cmd)
{  
    cmd = removeSpace(cmd); // on enleve les espaces multiples
    char* next = strchr(cmd, ' '); // on decoupe la string avec chaque espace
    int i = 0;
    nbargs = 0;
    while(next != NULL) { // tant qu un nouvel espace est present,
        arguments[i] = cmd;
        next[0] = '\0';
        ++i;
        nbargs ++;
        cmd = removeSpace(next + 1);
        next = strchr(cmd, ' ');
    }
 
    if (cmd[0] != '\0') {
        arguments[i] = cmd;
        next = strchr(cmd, '\n');
        next[0] = '\0';
        nbargs ++;
        ++i;
    }
    arguments[i] = NULL; // dernier arg de args = NULL
}

//Redefinition commande CD
 void cd(char *chemin){
            	//CD with Arg
            	
            	//Variables with init
            	int fdt;
            	char* finalPath = malloc(sizeof(char) * BUFFER);
            	char* tempPath = malloc(sizeof(char) * BUFFER);
            	char* arboTar = malloc(sizeof(char) * BUFFER);
              	char* nomTar = malloc(sizeof(char) * BUFFER);
            	strcpy(finalPath,"");
            	strcpy(tempPath,"");
            	strcpy(arboTar,"");
             	strcpy(nomTar,"");
            	

				//On converti le chemin selon la Gestion chemin absolu et relatif dans tempPath
				strcpy(tempPath,convertChemin(chemin));	
										
				//Parse command. On essaye deja de chdir dessus si on arrive a faire un chdir , 
				if(chdir(tempPath) == 0){ 
					//chdir(tempPath) => 0 --> SUCCESS
								   //historique
                                   strcpy(old_pwd, pwd);
                                   
                                   //mise a jour pwd avec le tempPath
                                   strcpy(pwd, tempPath);            
                    } 
				else { 
			    	//chdir(tempPath) => -1 --> FAIL , on si il y a un motif ".tar" dans le tempPath			
			    	
					//si tempPath contient ".tar" , on decoupe tempPath dans finalPath + nomTar + arboTar avec les "/"
					if(strstr(tempPath, ".tar") != NULL) {	
						  int t=0;
						  char *p = strtok(tempPath, "/");
						  strcat(finalPath,"/") ;
						  while(p != NULL)	{ 
						        //ajout tarname
							 if(strstr(p,".tar") != 0) {
						 	 		t=1;
						  			strcpy(nomTar,p);
						 		}
							 else if (t==0) {
							  	//ajout pwdtmp
								  	strcat(finalPath,p) ; 
									strcat(finalPath,"/") ; 
								  }
						     else {
						      	//ajout arboTar
									strcat(arboTar,p) ; 
									strcat(arboTar,"/") ; 
						      }
					    	  //Debug  printf("'%s'\n", p);
						    p = strtok(NULL, "/");
						  }	
				//Fin Decoupe			

						   if ( debug == 1 ) printf("\n");
						   if ( debug == 1 ) printf("finalPath = %s\n",finalPath);
						   if ( debug == 1 ) printf("nomTar = %s\n",nomTar);
						   if ( debug == 1 ) printf("arboTar = %s\n\n",arboTar);
				  
						   //update env  on essaye de chdir sur le repertoire (avant le tar )
						   if(chdir(finalPath) == -1) {
						   	perror("Error : Bad Dir name");
						   }
						   else {
						   	   strcat(finalPath,nomTar) ;
							   fdt = open(finalPath, O_RDONLY);
                               if (fdt < 0){
                                       perror("Error : open");
                               }else{
	                               if(  ( arboTar[0] == '\0' ) || ( get_fichier_type(fdt, arboTar,debug) == '5'  ) ) {
	                                   strcpy(old_pwd, pwd);
	                                   //on remplace pwd par finalPath + '/' + arboTar
	                                   strcpy(pwd, finalPath);
	                                   strcat(pwd, "/");
	                                   strcat(pwd, arboTar);	                                   
	                               }
	                               else{
	                                   perror("Error : Bad Dir name in tar");
								   }
                               }
						   }
                        }
                        
                        //si pwdtmp ne contient pas .tar
                        else {
                        	perror("Error : not found");
						}
                    }

                    //free
                    free(finalPath);
                    free(tempPath);
                    free(arboTar);
                    free(nomTar);
}

//Analyse les sous commandes, si elles ne contiennent pas une fonction totalement redefinie, la sous commande va dans executeCmd
 int analyse(char* cmd, int fd, int debut, int dernier)
{

    // decoupe la sous commande  dans le tableau d'args : ls -l => arguments[0] = "ls" , arguments[1] = "-l"
    decoupe(cmd); 

    if (arguments[0] != NULL) {
        //TOTALLY REDEFINED SHELL COMMANDS
        //Implementation Exit 
        if (strcmp(arguments[0], "exit") == 0) {
            printf("Bye ! \n");
            free(old_pwd);
            exit(0);
        }
        //Implementation CD redefinie - se fait ici car pas besoin de creer un fils
        else if ( strcmp(arguments[0],"cd") == 0) {
            // cd & cd ~ lead to HOME path
            if ( (arguments[1] == NULL ) || (strcmp(arguments[1], "~")== 0) ) { 
                strcpy(old_pwd, pwd);
                strcpy(pwd, getenv("HOME")); // Read the HOME environment variable
                chdir(pwd);
            }
            //cd - / go to previous path , stored in old_pwd
            else if (strcmp(arguments[1], "-")== 0){ 
            
            	//Variables
				char* pwdtmp=malloc(sizeof(char) * BUFFER);;
				
                chdir(old_pwd);
                //permute pwd et old_pwd 
                strcpy(pwdtmp, pwd);
                strcpy(pwd, old_pwd); 
                strcpy(old_pwd, pwdtmp); 
                free(pwdtmp);
            } 
            else {
				cd(arguments[1]);				
            }
        }
 
// ########### PARTIE POUR TESTER LES FONCTIONS UNITAIREMENT ET LEUR DONNER UN ALIAS SOUS LE SHELL  ###########
        
        //afficher_fichier => cat
        else if (!strcmp(arguments[0], "cat2")){
             int fdx = open(arguments[1], O_RDONLY);
             if (fdx < 0){
                  perror("open");
                  return -1;
             }
             afficher_fichier(fdx, arguments[2]);
             close(fdx);
        }
        
        //afficher_repertoire => ls
        else if (!strcmp(arguments[0], "ls2")){
                int fdx;
                //afficher repertoire et droit
                if (!strcmp(arguments[1], "-l")){
                    fdx = open(arguments[2], O_RDONLY);
                    if (fdx < 0){
                    perror("open");
                    return -1;
                    }
                    if ( nbargs == 4) afficher_repertoire(fdx, trouve(fdx,arguments[3]), 1);
                    if ( nbargs == 3) afficher_tar_content(fdx ,1);

                }else{
                    fdx = open(arguments[1], O_RDONLY);
                    if (fdx < 0){
                    perror("open");
                    return -1;
                    }
                    if ( nbargs == 3) afficher_repertoire(fdx, trouve(fdx,arguments[2]), 0);
                    if ( nbargs == 2) afficher_tar_content(fdx ,0);

                }
                close(fdx);
        }
        
        
        //get_fichier_type =>
        else if (!strcmp(arguments[0], "gft")){
                int fdx = open(arguments[1], O_RDONLY);
                if (fdx < 0){
                    perror("open");
                    return -1;
                }
                get_fichier_type(fdx, arguments[2],debug);
                close(fdx);
        }
 //########### FIN DES FONCTIONS DE TEST ###########
        
        //Execution commande
        else {
            nbexecuteCmds += 1;
            return executeCmd(fd, debut, dernier);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
	printf("(PID = %d) TSH Shell\n",getpid());
	
	//Enable Debug Mode	
	if( (argv[1] != NULL)  && (strcmp(argv[1] ,"-debug")) == 0 ) { printf("< DEBUG MODE > \n"); debug =1;}
			
	//Variable
	 char ligne[BUFFER]; // commande a analyser

	//Premiere initialisation des variables pwd et old_pwd
    pwd = malloc(sizeof(char) * BUFFER);
    strcpy(pwd, getenv("PWD"));
    old_pwd= malloc(sizeof(char) * BUFFER); 
    strcpy(old_pwd, pwd);
    
    //Shell 
    while (1) {	

        // Prompt
        fprintf(stdout,"\n%s$> ",pwd);
        fflush(NULL);
 
        // Lecture Commande
        if (!fgets(ligne, BUFFER, stdin)) { return 0; }
 
 		//Parsing de la commande, decoupage des sous commandes
 		//ex: la commande  "ls -lrt | grep f3" donne 2 sous commandes cmd : "ls -lrt" , puis "grep f3" qui vont dans analyse
        int fd = 0;
        int debut = 1;
        char* cmd = ligne;
        char* next = strchr(cmd, '|'); // next = ce qu'il y a dans cmd apres le premier pipe
        while (next != NULL) { // on rentre dans ce while uniquement si on a au moins un "|" dans la commande
            *next = '\0'; // on remplace dans cmd tout ce qu'il y a apres le premier | par '\0'
            fd = analyse(cmd, fd, debut, 0); // on lance une analyse de chaque sous commande dans l'ordre, avec les bons attributs de fd , debut, fin 
            cmd = next + 1;
            next = strchr(cmd, '|'); // prochain pipe
            debut = 0;
        }
        //derniere sous commande
        fd = analyse(cmd, fd, debut, 1); // on analyse la cmd avec dernier = 1
        
        //cleanup
        attenteDuPere(nbexecuteCmds);
        nbexecuteCmds = 0;
    }  

    return 0;
}
