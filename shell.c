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
#include "syntaxe.h"
#include "sgf.h"
#define BUFFER 1024
#define LECTURE  0
#define ECRITURE 1
#define BLOCKSIZE 512

char *redirection; // Redirection output file
char* arguments[BUFFER]; // tableau d'arguments de chaque sous commande
char* pwd; // current dir
char* old_pwd; // copy old pwd - used for "cd -"
int nbexecuteCmds=0; // nombre de sous commandes executees - utilise pour CleanUp
int nbargs;
int debug=0; // Debug mode disabled by default
int redirFlag=0; // 1 = overwrite , 2 = append
char* pwdtmp;
char* tarname;
char* arboTar;

char* removePointPoint(char* NewChemin , char* begin) {
	
	    char* Temp = malloc(sizeof(char) * BUFFER);
		strcpy(Temp,"");
		char *p = strtok(NewChemin, "/"); //d�couper NC avec "/", p prend le 1er argc 
		// *p = strtok ([/home/user/vb/../toto], "/")
		// p = home --> else, Temp=/home/ --> next = user 
		// p = user --> else, Temp=/home/user/ --> next = vb 
		// p = vb  --> else, Temp=/home/user/vb/ --> next = ..
		// p = .. --> if, for (strlen(12 = b)), else b !=/ --> Temp[12] = '\0' --> Temp=/home/user/v\0
		// 				  for (strlen(11 = v)), else v !=/ --> Temp[11] = '\0' --> Temp=/home/user/\0
		// 				  for (strlen(10 = /)), if / = / --> break for
		// p = toto --> else, Temp=/home/user/toto/ --> next = NULL
		// p = NULL, fin du while
		
		strcat(Temp,begin) ;
		while(p != NULL)	{ 
			// des que l'on rencontre le motif "..", on efface la sous partie pr?c?dente
			if(strstr(p,"..") != 0 ) { 								  
				for(int i = strlen(Temp)-2; i > 0; i--){
	                if(Temp[i] == '/'){
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
			p = strtok(NULL, "/"); //reinitialize p with the next string
		}
		
		
		//suppression du dernier / dans la commande demandee => xx/toto.tar/ devient xx/toto.tar
		if(  Temp[strlen(Temp)-1] == '/' ) {
		   Temp[strlen(Temp)-1] = '\0';
		}
		
		return Temp;
}

//convertChemin permet la Convertion chemin absolu (si relatif) ,  la Gestion des ".." et supprime le "/" final si present
// "toto.tar/toto/titi/../titi/f3/" =>  "<pwd>/toto.tar/toto/titi/f3"
 char* convertChemin(char* chemin, char* charfinal){	
	//VARIABLES
        char* Temp = malloc(sizeof(char) * BUFFER);
        char* NewChemin = malloc(sizeof(char) * BUFFER);
        strcpy(Temp,"");
        strcpy(NewChemin,chemin); //recopier Chemin pour �viter le conflits
            	
        //si arg commence par '/' , chemin absolu
      	if(  NewChemin[0] == '/' ) {
			strcpy(Temp, NewChemin); 
		}
		//sinon chemin relatif , on ajoute l'arg a la suite du pwd actuel
		else {
			strcpy(Temp,pwd); //copy pwd
			strcat(Temp,"/"); //add / --> pwd/
			strcat(Temp,NewChemin); //add NC --> pwd/NC 
		}	
		
		strcpy(NewChemin,Temp);
		//  Newchemin est ici le chemin complet ou on souhaite aller
		
		strcpy(Temp,removePointPoint(NewChemin,"/"));
		//remove the ".."
	
		
		//continue l'exemple : Temp=/home/user/toto/ strlen=16
		//if Temp[15] == / final --> Temp[15] = '\0 --> Temp=/home/user/toto\0

    	//Cas si on est a la racinecd -
		//la cas qu'on n'ajoute jamais p dedans (entrant jamais dans while)
    	if(strcmp(Temp,"") == 0) {strcpy(Temp,"/");}
        printf("temp %s \n",Temp);
		strcat(Temp,charfinal);
		return Temp;
}

//findGoodPath permet de renvoyer le chemin correct a utiliser selon si le motif ".tar" est present dans le pwd ou arg1 ou arg2
 char* findGoodPath() {
        char* pwdtmp=malloc(sizeof(char) * BUFFER);
            //si l'arg1 contient le motif ".tar" on copie l'arg1 (avec convertion chemin relatif/absolu)  dans pdwtmp
              if ( (arguments[1] != NULL) &&  (strstr(arguments[1], ".tar") != NULL) ) {
                strcpy(pwdtmp,convertChemin(arguments[1],""));
            }
            // cas du ls -l     : on regarde l'arg2 => si l'arg2 contient le motif ".tar" on copie l'arg2 (avec convertion chemin relatif/absolu)  dans pdwtmp
            else if ( (arguments[2] != NULL) &&  (strstr(arguments[2], ".tar") != NULL) ) {
                strcpy(pwdtmp,convertChemin(arguments[2],""));
            }
            // enfin si le pwd contient le motif ".tar" on le recopie dans pwdtmp
            else if  (strstr(pwd, ".tar") != NULL ) {
                strcpy(pwdtmp,pwd);
                //Cas ou arg[1] est un chemin
                if      ( (arguments[1] != NULL) &&  (
                    (strcmp(arguments[1], "-r") != 0)
                    &&
                    (strcmp(arguments[1], "-l") != 0 )
                    ) ) {
                    
                    strcat(pwdtmp,"/");
                    strcat(pwdtmp,arguments[1]);
                }
                //Cas ou arg[1] est un option
                else if (arguments[2] != NULL && arguments[1] != NULL) {
                //if (arguments[2] != NULL) {
                    strcat(pwdtmp,"/");
                    strcat(pwdtmp,arguments[2]);
                    
                }
            }
            
            return convertChemin(pwdtmp,"");
}

//UseRedefCmd permet de voir si pwd ou arg1 ou arg2 contient le motif ".tar" , et si oui renvoi 1
 int UseRedefCmd() {
    if (
        (strstr(pwd, ".tar") != NULL ) // si le pwd contient ".tar"
        ||
        ( (arguments[1] != NULL)  &&  (strstr(arguments[1], ".tar") != NULL ) )  // ou que arg1 existe avec un ".tar" dedans
        ||
        ( (arguments[2] != NULL)  &&  (strstr(arguments[2], ".tar") != NULL ) )  // ou que arg2 existe avec un ".tar" dedans (si arg1 = -l    )
        )
    {
            return 1;
    }
    return 0;
}

//getTarPath permet de recuperer le chemin vers un tar a partir d'un chemin
// convert /home/user/Shell/toto.tar/toto/titi => /home/user/Shell
  char* getTarParentDir(char*chemin) {
                 char* tarDir=malloc(sizeof(char) * BUFFER);
                 strcpy(tarDir,"/");
                 char* newchemin = malloc(sizeof(char) * BUFFER);
                 strcpy(newchemin,chemin);
                 int t=0;
                char *p = strtok(newchemin, "/");
                while(p != NULL)    {
                    if(strstr(p,".tar") != 0) {
                        t=1; // t = flag to switch the result on tarDir or on arbotar
                    }
                    else if (t == 0) {
                    //ajout tarDir
                        strcat(tarDir,p);
                        strcat(tarDir,"/") ;
                    }
                    //Debug  printf("'%s'\n", p);
                    p = strtok(NULL, "/");
                }
                return tarDir;
 }
 

//getTarPath permet de recuperer le chemin vers un tar a partir d'un chemin
// convert /home/user/Shell/toto.tar/toto/titi => /home/user/Shell/toto.tar
  char* getTarPath(char*chemin) {
                 char* tarname=malloc(sizeof(char) * BUFFER);
                 strcpy(tarname,"/");
                 char* newchemin = malloc(sizeof(char) * BUFFER);
                 strcpy(newchemin,chemin);
                 int t=0;
                char *p = strtok(newchemin, "/");
                while(p != NULL)    {
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
// convert /home/user/Shell/toto.tar/toto/titi => toto/titi
  char* getTarArbo(char*chemin) {
                 char* arboTar=malloc(sizeof(char) * BUFFER);
                 strcpy(arboTar,"");
                 char* newchemin = malloc(sizeof(char) * BUFFER);
                 strcpy(newchemin,chemin);
                 int t=0;
                char *p = strtok(newchemin, "/");
                while(p != NULL)    {
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

//get nom du fichier
char* getTmpFileName(char*chemin) {
                char* FileName=malloc(sizeof(char) * BUFFER);
                strcpy(FileName,"");
                char* newchemin = malloc(sizeof(char) * BUFFER);
                strcpy(newchemin,chemin);
                char *p = strtok(newchemin, "/");
                while(p != NULL)    {
                    //ajout FileName
                        strcpy(FileName, "/tmp/");
                        strcat(FileName,p) ;
                    //Debug  printf("'%s'\n", p);
                    p = strtok(NULL, "/");
                }

                return FileName;
 }

void decoupePwdtmp(){
    //decoupe pwdtmp en tarname + arbotar

    //VARIABLES du fils
             pwdtmp=malloc(sizeof(char) * BUFFER);
            tarname=malloc(sizeof(char) * BUFFER);
            arboTar=malloc(sizeof(char) * BUFFER);
            strcpy(pwdtmp,"");
            strcpy(tarname,"/");
            strcpy(arboTar,"");

                if ( debug == 1 ) { printf("COMMANDE REDEFINIE !\n"); }

            strcpy(pwdtmp,findGoodPath());
            if ( debug == 1 ) printf("PWDTMP = %s\n",pwdtmp);
            
            //Exception pour le cas ou on fait un cat hors du tar depuis le tar ex :   pwd = /home/user/Shell/toto.tar/toto$> cat ../../f2
            if(strstr(pwdtmp, ".tar") == NULL ) {
                if ( debug == 1 ) {printf("on sort du TAR , commande normale %s\n",pwdtmp); }
                strcpy(arguments[1],pwdtmp);
                execvp( arguments[0], arguments);
            }
            
            //Decoupe pwdtmp en 2 avec des / afin d'extraire le tarname et l'arborescence dans le tar
            //ex avec pwdtmp = /home/user/Shell/toto.tar/toto/titi => tarname = /home/user/Shell/toto.tar    arboTar = toto/titi
            strcpy(tarname,getTarPath(pwdtmp));
            if ( debug == 1 ) { printf("tarname=%s\n", tarname); }
            
            strcpy(arboTar,getTarArbo(pwdtmp));
            
}

 int rmdir_redefini(){
     
            decoupePwdtmp();
            //la partie rm2
            //Ajoute le "/" final du arboTar
            if (arboTar[strlen(arboTar)-1] != '/'){
                strcat(arboTar, "/");
            }
            if ( debug == 1 ) { printf("arboTar=%s\n", arboTar); }

            int fdxx = open(tarname, O_RDWR);
            if (fdxx < 0){ perror(" Error open "); return -1; }

            delete_repertoire(fdxx, arboTar);

            close(fdxx);
            free(pwdtmp);
            free(tarname);
            free(arboTar);
            return 0;
 }

 int rm_redefini(){
     
             decoupePwdtmp();
            //la partie rm2
            //enleve le "/" final du arboTar
            if (arboTar[strlen(arboTar)-1] == '/'){
                arboTar[strlen(arboTar)-1] = '\0';
            }
            if ( debug == 1 ) { printf("arboTar=%s\n", arboTar); }

            int fdxx;
            if(!strcmp(arguments[1], "-r")){ //il y a "-r" --> delete_rep
                //rm --> rmdir, -r --> chemin, args[2]--> NULL

                //Ajoute le "/" final du arboTar
                if (arboTar[strlen(arboTar)-1] != '/'){
                    strcat(arboTar, "/");
                }
                if ( debug == 1 ) { printf("arboTar=%s\n", arboTar); }

                int fdxx = open(tarname, O_RDWR);
                if (fdxx < 0){ perror(" Error open "); return -1; }

                delete_repertoire(fdxx, arboTar);
            

                // strcat(arboTar, "/");
                // if ( debug == 1 ) { printf("with - r arboTar=%s\n", arboTar); }
                // fdxx = open(tarname, O_RDWR);
                // if (fdxx < 0){ perror(" Error open "); return -1; }
                // delete_repertoire(fdxx, arboTar);
            }else{
                fdxx = open(tarname, O_RDWR);
                if (fdxx < 0){
                    perror(" Error open");
                    return -1;
                }
                delete_fichier(fdxx, arboTar);
            }

            close(fdxx);
            free(pwdtmp);
            free(tarname);
            free(arboTar);
            return 0;
 }
 
 // redefintion de la fonction cp


 int cp_redefinir(){

     int option ;
     char * src_path = NULL;
     char * dst_path = NULL ;
     char * src_arbotar = NULL ;
     char * dst_arbotar = NULL ;
     int fd_src = -1 ;
     int fd_dst = -1;
     // voir le nombre d'arguments
    
     if( nbargs == 3 ){// commande cp sans option

        option = 0 ;

     }else if ( nbargs == 4 ){ // commande cp avec option

        option = 1 ;

         if( (option == 1) && (strcmp(arguments[1],"-r") != 0)){
         
             execvp(arguments[0],arguments);
        }

     }else
     {
         // nombre incompatible d'arguments
         perror(" veuillez introduire le bon nombre d'arguments pour la commande 'cp' \n");
         return -1;
     }
     
     
        src_path = convertChemin(arguments[nbargs - 2],(arguments[nbargs - 2][strlen(arguments[nbargs - 2])-1] == '/')?"/":"");
        dst_path  = convertChemin(arguments[nbargs-1],"");

        //printf("nb : %d , conversion source : %s , conversion destination : %s \n\n",nbargs,src_path,dst_path);


        src_arbotar = analyser_path(src_path,&fd_src);
        dst_arbotar = analyser_path(dst_path,&fd_dst);

        //printf(" source arb : %s ,  destination arb : %s \n",src_arbotar,dst_arbotar);

        if(fd_src != -1 ){// la source est un tarball

            if(fd_dst != -1 ){// destination tarball

                if(strcmp(src_arbotar,"") == 0){// la copier concerne le fichier avec extension .tar

                    perror(" imbriquation de tarballs impossible \n");
                    return -1 ;

                }else{
                    cp_srctar(src_arbotar,fd_src,dst_arbotar,fd_dst,option);
                }

            }else{// destination exterieur

                if(strcmp(src_arbotar,"") == 0){// la copier concerne le fichier avec extension .tar

                    free(src_path);
                    free(src_arbotar);
                    free(dst_path);
                    free(dst_arbotar);
                    close(fd_src);
                    close(fd_dst);
                    execvp(arguments[0],arguments);

                }else{// destintion tar
                    
                    cp_srctar(src_arbotar,fd_src,dst_path,fd_dst,option);
                }
            }
        }else // une source simple ( fichier ou repertoire exterieur )
        {

            if(fd_dst != -1){// destination tarball

                cp_srcsimple(src_path,dst_arbotar,fd_dst,option);

            }else // destination simple
            {
                    free(src_path);
                    free(src_arbotar);
                    free(dst_path);
                    free(dst_arbotar);
                    close(fd_src);
                    close(fd_dst);
                execvp(arguments[0],arguments);
                
            }
            
        }
        
        free(src_path);
        free(src_arbotar);
        free(dst_path);
        free(dst_arbotar);
        close(fd_src);
        close(fd_dst);
        return 1;

 }


 //redefintion de la fonction mv

 int mv_redefini(){
     
     char * src_path = NULL;
     char * dst_path = NULL ;
     char * src_arbotar = NULL ;
     char * dst_arbotar = NULL ;
     int fd_src = -1 ;
     int fd_dst = -1;
     // voir le nombre d'arguments
    
     if( nbargs == 3 ){// commande mv sans option

        src_path = convertChemin(arguments[nbargs - 2],(arguments[nbargs - 2][strlen(arguments[nbargs - 2])-1] == '/')?"/":"");
        dst_path  = convertChemin(arguments[nbargs-1],"");

        //printf("nb : %d , conversion source : %s , conversion destination : %s \n\n",nbargs,src_path,dst_path);


        src_arbotar = analyser_path(src_path,&fd_src);
        dst_arbotar = analyser_path(dst_path,&fd_dst);

        //printf(" source arb : %s ,  destination arb : %s \n",src_arbotar,dst_arbotar);

        if(fd_src != -1 ){// la source est un tarball

            if(fd_dst != -1 ){// destination tarball

                if(strcmp(src_arbotar,"") == 0){// la copier concerne le fichier avec extension .tar

                    perror(" imbriquation de tarballs impossible \n");
                    return -1 ;

                }else{
                    cp_srctar(src_arbotar,fd_src,dst_arbotar,fd_dst,1);
                    nbargs --;
                    if(src_path[strlen(src_path)-1] != '/'){
                        rm_redefini();
                    }else
                    {
                        rmdir_redefini();
                    }
                    
                }

            }else{// destination exterieur

                if(strcmp(src_arbotar,"") == 0){// la copier concerne le fichier avec extension .tar

                    free(src_path);
                    free(src_arbotar);
                    free(dst_path);
                    free(dst_arbotar);
                    close(fd_src);
                    close(fd_dst);
                    execvp(arguments[0],arguments);

                }else{// destintion tar
                    
                    cp_srctar(src_arbotar,fd_src,dst_path,fd_dst,1);
                    nbargs --;
                    if(src_path[strlen(src_path)-1] != '/'){
                        rm_redefini();
                    }else
                    {
                        rmdir_redefini();
                    }
                    
                }
            }
        }else // une source simple ( fichier ou repertoire exterieur )
        {

            if(fd_dst != -1){// destination tarball

                cp_srcsimple(src_path,dst_arbotar,fd_dst,1);
                    nbargs --;
                    if(src_path[strlen(src_path)-1] != '/'){
                        rm_redefini();
                    }else
                    {
                        rmdir_redefini();
                    }
                    

            }else // destination simple
            {
                    free(src_path);
                    free(src_arbotar);
                    free(dst_path);
                    free(dst_arbotar);
                    close(fd_src);
                    close(fd_dst);
                execvp(arguments[0],arguments);
                
            }
            
        }
        
        free(src_path);
        free(src_arbotar);
        free(dst_path);
        free(dst_arbotar);
        close(fd_src);
        close(fd_dst);



    }else { // commande mv avec option

         
             execvp(arguments[0],arguments);
     }
        
     return 1;
 }

 int mkdir_redefini(){
             
            decoupePwdtmp();
            
            if (arboTar[strlen(arboTar)-1] != '/'){
                strcat(arboTar, "/");
            }
            if ( debug == 1 ) { printf("arboTar=%s\n", arboTar); }

            int fdxx = open(tarname, O_RDWR);
            if (fdxx < 0){ perror(" Error open "); return -1; }

            newEmptyDirectory(fdxx, arboTar);

            close(fdxx);
            free(pwdtmp);
            free(tarname);
            free(arboTar);
            return 0;
 }
 

//Redefinition de la fonction cat
// exemple : cat toto/tar.h
// strcpy(pwdtmp, FGP) --> pwdtmp=/home/user/v12/toto.tar/toto/tar.h
// remplie tarname=/home..../toto.tar et remplie arboTar=toto/tar.h

 int cat_redefini(){
     
            decoupePwdtmp();
            int fdxx = open(tarname, O_RDONLY);
            if (fdxx < 0){
                perror(" Error open ");
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
 int ls_redefini(){
     
            decoupePwdtmp();
            
              //appel aux fonctions d'affichage : afficher_tar_content si on est a la racine du tar , afficher_repertoire si on est plus loin , avec arboTar en argument
              int fdxx;
                //affLSicher repertoire et droit "ls -l"
                if(  (arguments[1] != NULL) && (!strcmp(arguments[1], "-l")) ){   // si l'arg1 = -l , on lance les fonctions d'affichage repertoire ou du tar directement avec la valeur 1 en argument
                    fdxx = open(tarname, O_RDONLY);
                    if (fdxx < 0){
                        perror(" Error open ");
                        return -1;
                    }
                   if  ( (strcmp(arguments[0], "ls") == 0)  && (strcmp(arboTar, "") != 0) )  { afficher_repertoire(fdxx, trouve(fdxx,arboTar), 1,arboTar); }
                   if ( (strcmp(arguments[0], "ls") == 0) && (strcmp(arboTar, "") == 0) ) { afficher_tar_content(fdxx ,1);  } // ls a la racine du tar

                }else{
                // si l'arg1 n'est pas -l
                    fdxx = open(tarname, O_RDONLY);
                    if (fdxx < 0){
                        perror(" Error open ");
                        return -1;
                    }
                     if ( (strcmp(arguments[0], "ls") == 0) && (strcmp(arboTar, "") != 0) ) { afficher_repertoire(fdxx, trouve(fdxx,arboTar), 0,arboTar); } // ls plus loin que la racine du tar
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
 // après avoir appelé 3 fois analyser, on a 3 sous commands : ls -l, head -n 2, wc -l en tab arguments
 //    fd1  = executeCmd(0, 1, 0), with args[0] = "ls" and args[1] = "-l"
 //    fd2  = executeCmd(fd1, 0, 0), with args[0] = "head" and args[1] = "-n" and args[2] = "2"
 //    fd3  = executeCmd(fd2, 0, 1), with args[0] = "wc" and args[1] = "-l"
 int executeCmd(int fd, int debut, int dernier) {
    if ( debug == 1 ) printf("Le pere entre dans executeCMD - FD = %d , DEBUT = %d , DERNIER = %d pour y lancer %s\n",fd,debut,dernier,arguments[0]);
    
    //Variables
    int tubes[2];
     pid_t pid;
    char msg[BUFFER]={0};
    int val_op;
     
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
                   
        //##### Gestion des divers processus fils tel un train
        // exemple tube et dup2 :
        //    fd1  = executeCmd(0, 1, 0), with args[0] = "ls" and args[1] = "-l"
         //    fd2  = executeCmd(fd1, 0, 0), with args[0] = "head" and args[1] = "-n" and args[2] = "2"
         //    fd3  = executeCmd(fd2, 0, 1), with args[0] = "wc" and args[1] = "-l"
        // Construction la communication entnre les fils
        // 1er exC, 1er fork : (0, 1, 0) -->  if_first --> dup2(tubes[1], STDOUT) remplie STDOUT par le contenu de tubes[1]
        // if_sec --> NO --> on entre dans else if(execvp(ls, ls -l)) car il ne contient pas .tar --> fils dead
        // le processus pere continue et refait le boucle while --> on a 2nd Analyser --> 2nd exC
        // 2nd exC, 2nd fork : (fd1, 0, 0) --> else if --> dup2(fd_1er, STDIN) remplie STDIN par fd_1er
        //                                                    dup2(tubes[1], STDOUT) remplie STDOUT par le contenu de tubes[1]
        // if_sec --> NO --> on entre dans else if(execvp(head, head -n 2)) --> ferme fd, fils dead
        // le processus pere (...) --> 3re exC
        // 3rd exC, 3rd fork : (fd2, 0, 1) --> else --> dup2(fd_2nd, STDIN) remplie STDIN par le contenu de fd_2nd
        // if_sec --> NO --> on entre dans else if(execvp(wc, wc -l)) --> ferme fd, return tube[0] pour lire ce que dedans, après fils dead

        if (fd == 0 && debut == 1 && dernier == 0  ) {
            // pour la debut commande redirection de la sortie dans le tube
            // on change la sortie standard par l'entree du pipe--> rewrite tubes[1] par STDOUT
            dup2( tubes[ECRITURE], STDOUT_FILENO );
        } else if (fd != 0 && debut == 0 && dernier == 0 ) {
            // pour les commende du milieu, ecoute de l entree depuis fd,
            // redirection de la sortie dans tubes[ECRITURE]
            dup2(fd, STDIN_FILENO); // on change l'entree standard  le fd pere
            dup2(tubes[ECRITURE], STDOUT_FILENO); // on changert la sortie standard par l'entree du pipe
        } else { // dernier=1
            // pour la dernier commande , ecoute de l entree depuis fd
            dup2( fd, STDIN_FILENO ); // on change l'entree standard  le fd pere
            
            //la redirection se fait uniquement sur le dernier process de la file
            if( strcmp(redirection,"") != 0 ) {
                for (int i=0;i<strlen(redirection);i++) {
                    if(redirection[i] == '\n' ) { redirection[i] =  '\0'; break;}
                }
                if (debug == 1) printf("Redirection Chemin = <%s> \n",convertChemin(redirection,"")) ;
                
                //cas hors tar : redirection >
                if ( (UseRedefCmd() == 0) && (redirFlag == 1) ) {
                    //open
                    val_op = open(convertChemin(redirection,""), O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                    if(val_op == -1) { perror(" Error open "); }

                    dup2(val_op, STDOUT_FILENO);
                }
                //cas hors tar : redirection double >>
                else if ( (UseRedefCmd() == 0) && (redirFlag == 2) ) {
                    //open
                    val_op = open(convertChemin(redirection,""), O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                    if(val_op == -1) { perror(" Error open "); }

                    dup2(val_op, STDOUT_FILENO);
                }
                //cas hors tar : redirection double >>
                else if ( (UseRedefCmd() == 0) && (redirFlag == 3) ) {
                    //open
                    val_op = open(convertChemin(redirection,""), O_RDONLY);
                    if(val_op == -1) { perror(" Error open "); }

                    dup2(val_op, STDIN_FILENO);
                }
                // cas stderr
                else if ( (UseRedefCmd() == 0) && (redirFlag == 4) ) {
                    //open
                    int val_op = open(convertChemin(redirection,""), O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                    if(val_op == -1) { perror(" Error open "); }

                    dup2(val_op, STDERR_FILENO);
                }
                //DANS UN TAR
                else if ( (UseRedefCmd() == 1) && (redirFlag == 1) ) {
                    printf(" Redirection dans un Tar\n");
                        decoupePwdtmp();
                        //on ouvre le fd du tar
                        int fd_du_tar = open(tarname, O_RDWR);
                        if (fd < 0){
                            perror(" Error open ");
                            return -1;
                        }
                        char* FileName = malloc(sizeof(char) * BUFFER);
                        strcpy(FileName, getTmpFileName(redirection)) ;

                        // on cree un fichier temporaire local avec le resultat des commandes
                        
                        //Mise a jour de la redirection via le chemin absolu, reprise uniquement de l'arbo du tar , suppression dernier / de l'arbo

                        redirection = getTarArbo(convertChemin(redirection,""));
                        if(  redirection[strlen(redirection)-1] == '/' ) {
                            redirection[strlen(redirection)-1] = '\0';
                        }
                        printf("CREATING LOCAL FILE : %s, redirection = %s\n", FileName, redirection);

                    
                        if ( debug == 1 ) printf("REDIRECTION = %s \n ",redirection);
                        int fd_fichier = open(FileName, O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                        dup2(fd_fichier, STDOUT_FILENO);
                        
                        // on ajoute le fichier local dans le tar a la bonne position
                        // /!\ FIXME /!\ la position est mal set et corrompt le TARBALL !
                        off_t position;
                        position = get_end_position(fd_du_tar);
                        addFile( fd_du_tar, fd_fichier , redirection ,  position);
                        free(FileName);
                        

                }
            
            }
        }
        
        //##### REDEFINITION COMMANDE PWD
        if (strcmp(arguments[0], "pwd") == 0){ //pwd passe en tarball
            sprintf(msg, "%s\n", pwd);
            write(STDOUT_FILENO, msg, sizeof(msg));
            exit(0);
        }
        
         //##### REDEFINITION COMMANDE CAT   uniquement si le pwd contient ".tar" ou si l'argument 1 existe et contient ".tar" (UseRedefCmd)
        else if ( (strcmp(arguments[0], "cat") == 0) && ( UseRedefCmd() == 1 ) ) {
            int ret = cat_redefini();
            exit(ret); // kill le fils
        }

        // ##### REDEFINITION COMMANDE RM   uniquement si le pwd contient ".tar" ou si l'argument 1 existe et contient ".tar" (UseRedefCmd)
        else if ( (strcmp(arguments[0], "rm") == 0) && ( UseRedefCmd() == 1 ) ) {
            int ret = rm_redefini();
            exit(ret); // kill le fils
        }
         //##### REDEFINITION COMMANDE RMDIR   uniquement si le pwd contient ".tar" ou si l'argument 1 existe et contient ".tar" (UseRedefCmd)
        else if ( (strcmp(arguments[0], "rmdir") == 0) && ( UseRedefCmd() == 1 ) ) {
            int ret = rmdir_redefini();
            exit(ret); // kill le fils
        }

        //##### REDEFINITION COMMANDES LS   uniquement si le pwd contient ".tar" ou si l'argument 1 existe et contient ".tar" ou l'arg 2 existe et contient ".tar" (UseRedefCmd)
        else if (  (strcmp(arguments[0], "ls") == 0)  && ( UseRedefCmd() == 1 ))   //si la commande est ls ou cat et rempli les conditions de redef
         {
            int ret = ls_redefini();
            exit(ret); // kill le fils
        }
        
        //##### REDEFINITION COMMANDES MKDIR   uniquement si le pwd contient ".tar" ou si l'argument 1 existe et contient ".tar" ou l'arg 2 existe et contient ".tar" (UseRedefCmd)
        else if ( (strcmp(arguments[0], "mkdir") == 0) && ( UseRedefCmd() == 1)) {
            int ret = mkdir_redefini();
            exit(ret);
        }

        else if ( (strcmp(arguments[0], "cp") == 0) && ( UseRedefCmd() == 1)) {
            int ret = cp_redefinir();
            exit(ret);
        }

        else if ( (strcmp(arguments[0], "mv") == 0) && ( UseRedefCmd() == 1)) {
            int ret = mv_redefini();
            exit(ret);
        }        

        else if (execvp( arguments[0], arguments) == -1) {
            perror("Commande Inconnue \n");
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
    while (isspace(*str) ) { ++str; }
    return str;
}

// decoupe la sous commande  dans le tableau d'arguments. ex: ls -l => arguments[0] = "ls" , arguments[1] = "-l"
// exemple : ls -l --> removeSpace(ls -l) --> ls -l
// *next = strchr(ls -l, ' ') --> next = " "-l, cmd = ls
// i=0, tant que next != NULL, on entre dand while1 --> args[0]=ls, next[0] = \0 --> next = -l
// i=1, nbargs=1, cmd = removeSpace('\0'-l + 1 ) -->  cmd = -l
// next = strchr(-l, ' ') --> next = NULL, sort du while
// cmd = -l, i=1, if cmd[0] != '\0', on entre dans if --> args[1] = -l
// next = strchr(-l, '\n') --> cmd = -l, next = NULL
// next[0] = '\0', nbargs = 2, i=2
// args[2] = NULL;
// Au final, on a arguments[0]= ls, arguments[1]= -l, arguments[2]=NULL
 void decoupe(char* cmd)
{
    arguments[BUFFER] = ""; //init le tab arguemnts
    cmd = removeSpace(cmd); // on enleve les espaces multiples
    char* next = strchr(cmd, ' '); // on decoupe la string avec chaque espace
    int i = 0;
    nbargs = 0;
    while(next != NULL) { // tant qu un nouvel espace est present,
        arguments[i] = cmd;
        next[0] = '\0';
        if(strcmp(cmd,">") == 0 ) {
            cmd = removeSpace(next + 1);
            if(debug == 1)  printf ("Redirection on file : <%s>\n",cmd);
            redirection=cmd;
            arguments[i]=NULL;
            redirFlag=1; // 1 = overwrite , 2 = append
            return;
        }
        else if(strcmp(cmd,">>") == 0 ) {
            cmd = removeSpace(next + 1);
            if(debug == 1)  printf ("Redirection on file : <%s>\n",cmd);
            redirection=cmd;
            arguments[i]=NULL;
            redirFlag=2; // 1 = overwrite , 2 = append
            return;
        }
        else if(strcmp(cmd,"<") == 0 ) {
            cmd = removeSpace(next + 1);
            if(debug == 1)  printf ("Redirection from file : <%s>\n",cmd);
            redirection=cmd;
            arguments[i]=NULL;
            redirFlag=3; // 1 = overwrite , 2 = append, 3 = write into
            return;
        }
        else if(strcmp(cmd,"2") == 0 ) {
            cmd = removeSpace(next + 1);
            next = strchr(cmd, ' ');
            cmd = removeSpace(next + 1);
            if(debug == 1)  printf ("Redirection on file : <%s>\n",cmd);
            redirection=cmd;
            arguments[i]=NULL;
            redirFlag=4; // 1 = overwrite , 2 = append , 3 = write into , 4 = err
            return;
        }
        else if(strcmp(cmd,"2>") == 0 ) { ;
            cmd = removeSpace(next + 1);
            if(debug == 1)  printf ("Redirection on file : <%s>\n",cmd);
            redirection=cmd;
            arguments[i]=NULL;
            redirFlag=4; // 1 = overwrite , 2 = append , 3 = write into , 4 = err
            return;
        }
        else{
            ++i;
            nbargs ++;
            cmd = removeSpace(next + 1); //pour alller au char après '\0'
            next = strchr(cmd, ' ');
        }
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
                strcpy(tempPath,convertChemin(chemin,""));

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
                          while(p != NULL)    {
                                //ajout tarname
                             if(strstr(p,".tar") != 0) { //toto.tar, NT = toto.tar, t=1
                                      t=1;
                                      strcpy(nomTar,p);
                              }
                             else if (t==0) {
                                  //ajout pwdtmp
                                      strcat(finalPath, p) ;
                                    strcat(finalPath,"/") ; // FP = /home/user/vb/v12
                             }
                             else {
                                  //ajout arboTar
                                    strcat(arboTar,p) ; //t=1, AT=toto/
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
                               perror("Error : Bad Directory name");
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
                                       strcpy(pwd, convertChemin(pwd,""));
                                   }
                                   else{
                                       perror("Error : Bad Directory name in tar");
                                   }
                               }
                           }
                        }
                        
                        //si pwdtmp ne contient pas .tar
                        else {
                            perror(" TSH Error ");
                        }
                    }

                    //free
                    free(finalPath);
                    free(tempPath);
                    free(arboTar);
                    free(nomTar);
}

//Analyse les sous commandes, si elles ne contiennent pas une fonction totalement redefinie, la sous commande va dans executeCmd
//autre exemple : on a 3 appelé analyse, un par les sous commande de cmd : ls -l, grep shell, wc -l
//1 decoupe ls -l dans le tab d'args --> args[0]=ls, args[1]=-l
 int analyse(char* cmd, int fd, int debut, int dernier)
{
    // decoupe la sous commande  dans le tableau d'args : ls -l => arguments[0] = "ls" , arguments[1] = "-l"
    decoupe(cmd);

    // exemple1 : 1er appele analyse, on a arguments[0]= ls, arguments[1]= -l, arguments[2]=NULL
    // args[0] != NULL, on entre dans if_outside
    // compare ls avec "exit", if match --> on entre if_inside --> NO
    // compare ls avec "cd", if match --> on entre if_inside --> NO
    // compare ls avec "cat2", if match --> on entre if_inside --> NO
     // compare ls avec "ls2", if match --> on entre if_inside --> NO
     // compare ls avec "gft", if match --> on entre if_inside --> NO
    // else_outside --> nbexecuteCmds=1, executeCmd(0, 1, 0); (exemple dans main, 1er appele d'analyser)

    // exemple2 : args[0] = exit, on entre dans if_outside
    // compare args[0] avec "exit", if match --> on entre if_inside --> YES
    // printf(Bye), free(), exit(0) --> finir le programme

    // exemple3 : args[0] = cat2, args[1]= f2,  on entre dans if_outside
    // compare args[0] avec "cat2", if match --> on entre if_inside --> YES
    // fdx = open(f2, R); --> read f2
    // appler afficher_fichier(num of free fd, f2); --> afficher le contenu de f2
    // close(fdx);
    // fin du premier analyser, commencer 2nd analyer

    if (arguments[0] != NULL) {
        //TOTALLY REDEFINED SHELL COMMANDS
        //Implementation Exit
        if (strcmp(arguments[0], "exit") == 0) {
            printf("Bye ! \n");
            free(old_pwd);
            //free(redirection);
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
             if (fdx < 0){ perror(" Error open "); return -1; }
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
                    if ( nbargs == 4) afficher_repertoire(fdx, trouve(fdx,arguments[3]), 1,arguments[3]);
                    if ( nbargs == 3) afficher_tar_content(fdx ,1);

                }else{
                    fdx = open(arguments[1], O_RDONLY);
                    if (fdx < 0){
                    perror("open");
                    return -1;
                    }
                    if ( nbargs == 3) afficher_repertoire(fdx, trouve(fdx,arguments[2]), 0,arguments[2]);
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

        //delete_(fichier et repertoire)
        else if(!strcmp(arguments[0], "find2")){
            int fdx = open(arguments[1], O_RDONLY);
            if (fdx < 0){
                perror("Error open");
                return -1;
            }
            printf("%ld", trouve(fdx, arguments[2]));
            close(fdx);
        }
        else if(!strcmp(arguments[0], "rmdir2")){
            int fdx = open(arguments[1], O_RDWR);
            if (fdx < 0){
                perror("Error open");
                return -1;
            }
            delete_repertoire(fdx, arguments[2]);
            close(fdx);
        }

        else if(!strcmp(arguments[0], "rm2")){
            int fdx;
            if(!strcmp(arguments[1], "-r")){
                fdx = open(arguments[2], O_RDWR);
                if (fdx < 0){
                    perror("Error open");
                    return -1;
                }
                delete_repertoire(fdx, arguments[3]);
            }else{
                fdx = open(arguments[1], O_RDWR);
                if (fdx < 0){
                    perror(" Error open");
                    return -1;
                }
                delete_fichier(fdx, arguments[2]);
            }
            close(fdx);
        }
        //mkdir
        //$> mkdir2 toto.tar toto/lili
        else if(!strcmp(arguments[0], "mkdir2")){
            int fdx = open(arguments[1], O_RDWR);
            if (fdx < 0){
                perror("Error open");
                return -1;
            }
            if(debug == 1) printf("Testing MKDIR\n");
            newEmptyDirectory(fdx, arguments[2]);
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
    //Variable
    char ligne[BUFFER]; // commande a analyser
    pid_t pid = getpid();
    char msg[BUFFER] = {0};
    char buff[BUFFER] = { 0 };
    int val_read = 0; //recuperer la valeur de retour de read
    int val_write = 0;


    
    sprintf(msg, "(PID = %d) TSH Shell\n", pid);
    val_write = write(STDOUT_FILENO, msg, sizeof(msg)/sizeof(msg[1]));
    if(val_write == -1) perror(" Error write ");

    //Enable Debug Mode
    if( (argv[1] != NULL)  && (strcmp(argv[1] ,"-debug")) == 0 ) { printf("< DEBUG MODE > \n"); debug =1;}
            
    //Premiere initialisation des variables pwd et old_pwd
    pwd = malloc(sizeof(char) * BUFFER);
    strcpy(pwd, getenv("PWD"));
    old_pwd= malloc(sizeof(char) * BUFFER);
    strcpy(old_pwd, pwd);
      redirection = malloc(sizeof(char) * BUFFER);
    
    //Shell
    while (1) {

        // Prompt
        sprintf(msg, "\n%s$> ", pwd);
        val_write = write(STDOUT_FILENO, msg, (strlen(msg)*sizeof(char))/sizeof(msg[1]));
        if(val_write == -1) perror(" Error write ");
    
        // Lecture Commande
        memset(buff, 0, BUFFER); //met ligne tout en 0
        val_read = read(STDIN_FILENO, buff, sizeof(buff));
        if(val_read == -1) perror(" Error read ");
        
        strcpy(ligne,buff);

         //Parsing de la commande, decoupage des sous commandes
         //ex: la commande  "ls -lrt | grep f3" donne 2 sous commandes cmd : "ls -lrt" , puis "grep f3" qui vont dans analyse
        int fd = 0; //chque fois fd doit = 0
        int debut = 1;
        char* cmd = "";
        cmd = ligne;
        char* next = strchr(cmd, '|'); // cherche 1er pipe dans cmd, return pointeur de sa position, next = ce qu'il y a dans cmd apres le premier pipe

        //exemple : cmd = ligne = ls | grep shell | wc,
        //*next = | grep shell | wc,
        //tant que next != NULL, on rentre dans while1 --> *next = '\0' --> cmd = ls
        //fd = analyse(ls, 0, 1, 0) --> fd=0, debut=1, dernier=0 vont reprendre par executeCmd plus tard
        //cmd = next + 1 --> cmd = grep shell | wc
        //next = | wc, debut = 0
        //tant que next != NULL, on rentre dans while2 --> *next ='\0' --> cmd = grep shell
        //fd = analyse(grep shell, fd_precedent, 0, 0)
        //cmd = next + 1 --> cmd = wc
        //next = NULL, debut = 0
        //fin du while
        //fd = analyse(wc, fd_precedent, 0, 1)
        //Au final, on a 3 appelé analyse, un par les sous commande de cmd : ls, grep shell, wc
        while (next != NULL) { // on rentre dans ce while uniquement si on a au moins un "|" dans la commande
            *next = '\0'; // on remplace dans cmd tout ce qu'il y a apres le premier | par '\0'
            fd = analyse(cmd, fd, debut, 0); // on lance une analyse de chaque sous commande dans l'ordre, avec les bons attributs de fd , debut, fin
            cmd = next + 1;
            next = strchr(cmd, '|'); // prochain pipe
            debut = 0;
        }
        //derniere sous commande
        fd = analyse(cmd, fd, debut, 1); // on analyse la cmd avec dernier = 1
        
        //cleanup : le processus pere wait ces fils termine
        attenteDuPere(nbexecuteCmds);
        nbexecuteCmds = 0; //finir le traitement de ligne/cmd global, apres on fait new Prompt
        
        //Reset redirection : il faut etre vide avant le cmd
        redirection="";
        redirFlag=0;
    }

    return 0;
}
