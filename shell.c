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


pid_t pid;
int fdt;

static char* arguments[BUFFER];
static char* pwd; // current dir

static char* pwdtmp; // tmp dir
static char* pwdtmp2; //  tmp dir

static char* old_pwd; // copy old pwd - used for "cd -"

static char* tarname; // tar path
static char* arboTar; // path into the tarfile

static char ligne[BUFFER]; // commande a analyser
static int nbexecuteCmds = 0; // nombre de executeCmdes
static int nbargs;
static int debug=1;

char cwd[BUFFER]; // durrent dir

 /*
 * fd: numero du file descriptor
 * debut: 1 si debut commande dans la sequence
 * dernier: 1 si derniere commande dans la sequence
 *
 * EXEMPLE avec la commande "ls -l | head -n 2 | wc -l" :
 *    fd1  = executeCmd(0, 1, 0), with args[0] = "ls" and args[1] = "-l"
 *    fd2  = executeCmd(fd1, 0, 0), with args[0] = "head" and args[1] = "-n" and args[2] = "2"
 *    fd3  = executeCmd(fd2, 0, 1), with args[0] = "wc" and args[1] = "-l"
 *
 */
 
 
static int executeCmd(int fd, int debut, int dernier)
{
	//Debug	printf("Execcmd => %d %d %d \n\n",fd,debut,dernier );
    //renvoi un FD
    int tubes[2];
 
    //creation des pipe et fils
    pipe(tubes);
    pid=fork();
 
    if (pid == 0) {
        
        //le fils
         if ( debug == 1 ) {
			 printf("(%d) Child exec <%s",getpid(),arguments[0]);
	        
	        for(int i=1;i<sizeof(arguments)/sizeof(arguments[0]); i++) {
	            if (arguments[i] != NULL) printf(" %s",arguments[i]);
	        }
	        
	        printf("> fd=%d debut=%d dernier=%d\n",fd,debut,dernier); 
   		}
        fflush(stdout);
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
            fprintf(stdout, "%s", pwd); 
		    fflush(stdout);
    		exit(0);
        }
        
        //##### REDEFINITION COMMANDES LS  et CAT =>  uniquement si le pwd contient ".tar" ou si l'argument 1 existe et contient ".tar"
        else if ( 
			( (strcmp(arguments[0], "ls") == 0) || (strcmp(arguments[0], "cat") == 0)   ) &&  //si la commande est ls ou cat
			(  
				(strstr(pwd, ".tar") != NULL ) // et que le pwd contient ".tar"
				|| 
				( (arguments[1] != NULL)  &&  (strstr(arguments[1], ".tar") != NULL ) )  // ou que arg1 existe avec un ".tar" dedans
				||
				( (arguments[2] != NULL)  &&  (strstr(arguments[2], ".tar") != NULL ) )  // ou que arg2 existe avec un ".tar" dedans (si arg1 = -l				
			) 
		) { //on utilise la version redefinie de LS
  		 	 if ( debug == 1 ) { printf("Commande REDEFINIE !\n"); }
  			
			// RESET VARIABLES
    		strcpy(pwdtmp,"");
            strcpy(arboTar,"");  	
			strcpy(tarname,"/");  	
				
  			//on set pwdtmp comme le repertoire a analyser. si ls <chemin avec .tar> , pwdtmp = pwd (si chemin relatif) + <chemin avec .tar> . si ls alors que notre pwd contient ".tar" , alors pwdtmp = pwd current 
  			if ( (arguments[1] != NULL) &&  (strstr(arguments[1], ".tar") != NULL) ) {
  				if(  arguments[1][0] == '/' ) { // chemin absolu commence par un /
					strcpy(pwdtmp,arguments[1]); //on analyse totalement l'args 1
				}
				else { // chemin relatif
					 strcpy(pwdtmp,pwd);
					 strcat(pwdtmp,"/");
					 strcat(pwdtmp,arguments[1]);	//on analyse le pwd + "/" + arg1			 
				}
  				 
			}
			else if ( (arguments[2] != NULL) &&  (strstr(arguments[2], ".tar") != NULL) ) { // cas du ls -l	, on regarde l'arg2
  				if(  arguments[2][0] == '/' ) { // chemin absolu commence par un /
					strcpy(pwdtmp,arguments[2]);
				}
				else { // chemin relatif
					 strcpy(pwdtmp,pwd);
					 strcat(pwdtmp,"/");
					 strcat(pwdtmp,arguments[2]);				 
				}
			}
			else if  (strstr(pwd, ".tar") != NULL ) { // on regarde le pwd contenant le .tar
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
			//ici pwdtmp est a jour avec le bon chemin.
			 if ( debug == 1 ) { printf("pwdtmp = %s\n",pwdtmp); }
			
			//Decoupe pwdtmp avec des / afin d'extraire le tarname et l'arborescence dans le tar
				int t=0;
				char d[] = "/";
				char *p = strtok(pwdtmp, d);
				strcat(pwdtmp2,"/") ;
				while(p != NULL)	{ 
					if(strstr(p,".tar") != 0) {
					//ajout tarname final : nom du tar, change la balise t a 1 
						t=1;
						strcat(tarname,p); // ici tarname est a jour avec le bon nom du tar
					}
					else if (t == 0) {
					//ajout tarname
						strcat(tarname,p);
						strcat(tarname,"/") ;
					}	
					else if (t==1) {
					//ajout arboTar
						strcat(arboTar,p) ; 
						strcat(arboTar,"/") ; 
					}
					//Debug  printf("'%s'\n", p);
					p = strtok(NULL, d);
				}	
			//FIN DECOUPE	
			 if ( debug == 1 )  { printf("ls TARNAME %s arboTar %s\n",tarname,arboTar);		}
			
  			//appel a la fonction d'affichage
  			   int fdxx;
                //afficher repertoire et droit  
                if(  (arguments[1] != NULL) && (!strcmp(arguments[1], "-l")) ){   // si l'arg1 = -l , on lance les fonctions d'affichage repertoire ou du tar directement avec la valeur 1 en argument
                    fdxx = open(tarname, O_RDONLY);
                    if (fdxx < 0){
	                    perror("open");
	                    return -1;
                    }
                   if  ( (strcmp(arguments[0], "ls") == 0)  && (strcmp(arboTar, "") != 0) )  { afficher_repertoire(fdxx, trouve(fdxx,arboTar), 1); }
                   if ( (strcmp(arguments[0], "ls") == 0) && (strcmp(arboTar, "") == 0) ) { afficher_tar_content(fdxx ,1);  } // ls a la racine du tar

                }else{ // si l'arg1 n'est pas -l
                    fdxx = open(tarname, O_RDONLY);
                    if (fdxx < 0){
	                    perror("open");
	                    return -1;
                    }
                    // commandes LS avec ou sans arboTar
                     if ( (strcmp(arguments[0], "ls") == 0) && (strcmp(arboTar, "") != 0) ) { afficher_repertoire(fdxx, trouve(fdxx,arboTar), 0); } // ls plus loin que la racine du tar
                     if ( (strcmp(arguments[0], "ls") == 0) && (strcmp(arboTar, "") == 0) ) { afficher_tar_content(fdxx ,0);  } // ls a la racine du tar
                    //commande cat
                     if (strcmp(arguments[0], "cat") == 0) { 
					 afficher_fichier(fdxx, arboTar); 
					 }
                     
                }
                close(fdxx);
            //fin appel 
  			
		    fflush(stdout);
    		exit(0); // kill le fils
        }
        else if (execvp( arguments[0], arguments) == -1) {
            if ( debug == 1 ) { printf("Commande Inconnue : %s %s\n",arguments[0] , arguments[1]); }
            fflush(stdout); 
			kill(getpid(),SIGTERM);
            return 1; // kill le fils
        }
    }
    
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
 
    //nombre de commandes a nettoyer
static void nettoyage(int n)
{
    int i;
    for (i = 0; i < n; ++i)
        wait(NULL);
}

static char* removeSpace(char* str)
{
    while ( isspace(*str) ) { ++str; }
    return str;
}
 
static void decoupe(char* cmd)
{
    //cette fonction remplis le tableau args proprement
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


static void cd(){
            	//CD Arg
            	
            	//Reset temp
            	strcpy(pwdtmp2,"");
            	strcpy(arboTar,"");
            	
            	//si arg = /*
      			if(  arguments[1][0] == '/' ) {
					strcpy(pwdtmp, arguments[1]); 
				}
				//si arg != /* , on ajoute l'arg au pwd actuel
				else {
				    strcpy(pwdtmp,pwd);
				    strcat(pwdtmp,"/");
				    strcat(pwdtmp,arguments[1]);
				}	
				
				//DEBUG printf(">>>> %s\n",pwdtmp);
				
				//Parse command. On essaye deja de chdir dessus 
				if(chdir(pwdtmp) == -1) { // -1 --> FAIL
			    				
				//si pwdtmp contient ".tar" , on decoupe pwdtmp en pwdtmp2 + tarname + arboTar avec les "/"
					if(strstr(pwdtmp, ".tar") != NULL) {	
						  int t=0;
						  char d[] = "/";
						  char *p = strtok(pwdtmp, d);
						  strcat(pwdtmp2,"/") ;
						  while(p != NULL)	{ 
						        //ajout tarname
							 if(strstr(p,".tar") != 0) {
						 	 	t=1;
						  		strcpy(tarname,p);
						 		}
							  else if (t==0) {
							  	//ajout pwdtmp
								  	strcat(pwdtmp2,p) ; 
									strcat(pwdtmp2,"/") ; 
								  }
						      else {
						      	//ajout arboTar
									strcat(arboTar,p) ; 
									strcat(arboTar,"/") ; 
						      }
					    	  //Debug  printf("'%s'\n", p);
						    p = strtok(NULL, d);
						  }	
				//Fin Decoupe			

						   if ( debug == 1 ) printf("\n");
						   if ( debug == 1 ) printf("PWDTMP = %s\n",pwdtmp2);
						   if ( debug == 1 ) printf("TARNAME = %s\n",tarname);
						   if ( debug == 1 ) printf("arboTar = %s\n\n",arboTar);
				  
						   //update env
						   if(chdir(pwdtmp2) == -1) {
						   	perror("Error : Bad Dir name");
						   }
						   else {
						   	   strcat(pwdtmp2,tarname) ;
							   fdt = open(pwdtmp2, O_RDONLY);
                               if (fdt < 0){
                                       perror("Error : open");
                               }else{
	                               if(  ( arboTar[0] == '\0' ) || ( get_fichier_type(fdt, arboTar,debug) == '5'  ) ) {
	                                   strcpy(old_pwd, pwd);
	                                   strcpy(pwd, pwdtmp2);
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
					else{  // si on arrive a faire un chdir , 0 --> SUCCESS
									//histo
                                   strcpy(old_pwd, pwd);
                                   
                                   //mise a jour pwd
                                   strcpy(pwd, pwdtmp);
                                   
                                   //si le pwd restant ne contient plus .tar , on clean tarname et arbotar
                                   if(strstr(pwd,".tar") ==0 ) {
                                   	    strcpy(tarname,"");
                                  		strcpy(arboTar,"");
								   }

                    }
}

static int analyse(char* cmd, int fd, int debut, int dernier)
{
    //printf("CMD= %s fd=%d debut=%d dernier=%d\n",cmd,fd,debut,dernier);
    decoupe(cmd); // decoupe la commande proprement dans le tableau d'args

    if (arguments[0] != NULL) {
        //REDEFINED SHELL COMMANDS
        //Implementation Exit 
        if (strcmp(arguments[0], "exit") == 0) {
            printf("Bye ! \n");
            free(tarname);
            free(arboTar);
            free(old_pwd);
            free(pwdtmp);
            exit(0);
        }
        //Implementation CD redefinie ls
        else if ( strcmp(arguments[0],"cd") == 0) {
            // cd & cd ~ lead to HOME path
            if ( (arguments[1] == NULL ) || (strcmp(arguments[1], "~")== 0) ) { 
                strcpy(old_pwd, pwd);
                strcpy(pwd, getenv("HOME")); // Read the HOME environment variable
                chdir(pwd);
            }
            //cd - / go to previous path , stored in old_pwd
            else if (strcmp(arguments[1], "-")== 0){ 
                chdir(old_pwd);
                strcpy(pwdtmp, pwd); //permuter pwd et pwdtmp
                strcpy(pwd, old_pwd); 
                strcpy(old_pwd, pwdtmp); 
            } 
            //cd .. remove the last object from the current path
            else if ((strstr(arguments[1], "..")) != 0){
							//on ajoute l'arg au pwd actuel dans pwdtmp , puis on decoupe la commande avec des / , pour supprimer ce qui se trouve avant les ".." avant de reconstruire la commande
                           strcpy(old_pwd, pwd); // for history
                           strcpy(pwdtmp2, "");
                           strcpy(pwdtmp,pwd);
				    	   strcat(pwdtmp,"/");
				           strcat(pwdtmp,arguments[1]);
                           //decoupe 
                           	
						    char d[] = "/";
						    char *p = strtok(pwdtmp, d);
						    strcat(pwdtmp2,"/") ;
						    while(p != NULL)	{ 
						        //ajout tarname
								if(strstr(p,"..") != 0 ) { 								  
							        for(int i = strlen(pwdtmp2)-2; i > 0; i--){
                            		   if(pwdtmp2[i] == '/'){
                                   			pwdtmp2[i+1] = '\0';
                                   			break;
                               		   }
									   else{
                                   		  pwdtmp2[i] = '\0';
                               			}
                           			}
								}
								else{						
									strcat(pwdtmp2 , p);
									strcat(pwdtmp2 , "/");
								//	printf("'%s' %s\n", p , pwdtmp2);
								}
							    p = strtok(NULL, d);
						    }
							//Fin Decoupe
							strcpy(arguments[1],pwdtmp2);
							cd();
                       }
            else {
				cd();				
            }
        }
        
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
        
        //Execution commande
        else {
            nbexecuteCmds += 1;
             return executeCmd(fd, debut, dernier);
        }
    }
    return 0;
}

int main()
{
    printf("(%d) Shell\n",getpid());
    pwd = getcwd(cwd, sizeof(cwd)); //un fois au debut : ou je suis
    
    tarname = malloc(sizeof(char) * BUFFER);
    arboTar = malloc(sizeof(char) * BUFFER);  
    old_pwd= malloc(sizeof(char) * BUFFER);
    pwdtmp = malloc(sizeof(char) * BUFFER);
    pwdtmp2 = malloc(sizeof(char) * BUFFER);

    strcpy(old_pwd, pwd);
    
    while (1) {
	    //suppression du dernier / dans le pwd
     	//	if(  pwd[strlen(pwd)-1] == '/' ) {	 pwd[strlen(pwd)-1] = '\0'; }
    	
	
        // Prompt
        printf("\n%s$> ",pwd);
        fflush(NULL);
 
        // Lecture executeCmde
        if (!fgets(ligne, BUFFER, stdin)) {
            return 0;
        }
 
        int fd = 0;
        int debut = 1;
        char* cmd = ligne;
        char* next = strchr(cmd, '|'); // decoupe entre chaque pipe
 
        while (next != NULL) {
            // next => pipe
            *next = '\0';
            fd = analyse(cmd, fd, debut, 0); // on lance une analyse des commandes precedant chaque pipe
            cmd = next + 1;
            next = strchr(cmd, '|'); // prochain pipe
            debut = 0;
        }
        fd = analyse(cmd, fd, debut, 1); // on analyse la cmd avec dernier = 1
        nettoyage(nbexecuteCmds);
        nbexecuteCmds = 0;
    }  

    return 0;
}
