#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<time.h>
#include "tar.h"

#define BUFFER 1024
#define LECTURE  0
#define ECRITURE 1


pid_t pid;
int fdt;
static char* Tmp;
static char* args[BUFFER];
static char* pwd; // current dir
static char* pwdtmp; // copy pwd tmp dir
static char* pwdtmp2; // copy pwd tmp dir
static char* old_pwd; // copy old pwd
static char* home_pwd; // copy home 
static char* tarname; // chemin du tar
static char* arboTar; // arborescence tar
static char* arboTarTmp; // arborescence tar
static char ligne[BUFFER]; // commande a analyser
static int nbexecuteCmds = 0; // nombre de executeCmdes
static int estDansTar = 0; // 0 = non, 1 = oui
static int argc;

char cwd[BUFFER]; // durrent dir


static off_t trouve(int fd, char *filename){
    int filesize = 0;
    struct posix_header p;
      lseek(fd, 0, SEEK_SET);

    if(fd < 0){
            perror("Fichier n'existe pas");
     }

      read(fd, &p, BLOCKSIZE);

    sscanf(p.size,"%o",&filesize);
    int sizefilename = strlen(filename);

    //enlever le dernier / dans filename
    if (filename[sizefilename-1] == '/') {
        filename[sizefilename-1] = '\0';
    }
    
      while(p.name[0] != '\0' ){
        
        //enlever le dernier / dans p.name
        if (p.name[strlen(p.name)-1] == '/') {
            p.name[strlen(p.name)-1] = '\0';
        }

           if(strcmp(p.name , filename) == 0){
              return  lseek(fd,-512, SEEK_CUR);
        }else{

            lseek(fd,(filesize % 512 == 0)? filesize : ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE, SEEK_CUR);
               read(fd, &p, BLOCKSIZE);
            sscanf(p.size,"%o",&filesize);
        }
    }
      return  -1;
}

static char get_fichier_type(int fd, char *chemin){
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


static void afficher_fichier(int fd, char *chemin){

    off_t position ;
    unsigned int filesize;

    struct posix_header p;

       position = trouve(fd,chemin);

    if ( position == -1 ){
        perror(" fichier inexistant ");
    }

    // la tete de lecture se trouve au bon endroit , par la fonction trouv

    if( read(fd,&p,BLOCKSIZE) <= 0 ){
        perror(" Erreur de lecture  ");
    }

    sscanf(p.size,"%o",&filesize);
    
    char content [filesize];

    if( read(fd,content,filesize) <= 0 ){
        perror(" Erreur de lecture  ");
    }

    write(1,content,filesize);

}

/* transforme le champs mode et type flag du posix header en une suite de caracteres 
    pour l'affichage de la commande ls -l*/
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

/* cette fonction permet d'afficher le contenu integral du fichier .tar pointe par le descripteur fd
--> si le mode == 1 elle fait un affichage similaire a ls -l sinon elle affiche juste la liste des fichiers que contient
le fichier .tar */
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
        
        if (mode == 1){/* ls -l x.tar*/

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

        }else{/* ls x.tar */

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
static void afficher_repertoire(int fd, off_t position, int mode){

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

 
 /*
 * fd: numero du file descriptor
 * debut: 1 si debut commande dans la sequence
 * dernier: 1 si dernier commande dans la sequence
 *
 * EXEMPLE avec la commande "ls -l | head -n 2 | wc -l" :
 *    fd1  = executeCmd(0, 1, 0), with args[0] = "ls" and args[1] = "-l"
 *    fd2  = executeCmd(fd1, 0, 0), with args[0] = "head" and args[1] = "-n" and args[2] = "2"
 *    fd3  = executeCmd(fd2, 0, 1), with args[0] = "wc" and args[1] = "-l"
 *
 */
 
 
static int executeCmd(int fd, int debut, int dernier)
{
	printf("Execcmd => %d %d %d \n\n",fd,debut,dernier );
    //renvoi un FD
    int tubes[2];
 
    //creation des pipe et fils
    pipe(tubes);
    pid=fork();
 
    if (pid == 0) {
        
        //le fils
        printf("(%d) Child exec <%s",getpid(),args[0]);
        //write(1,  );
        
        for(int i=1;i<sizeof(args)/sizeof(args[0]); i++) {
            if (args[i] != NULL) printf(" %s",args[i]);
        }
        
        printf("> fd=%d debut=%d dernier=%d\n",fd,debut,dernier);
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
        
		if (strcmp(args[0], "pwd") == 0){ //pwd passe en tarball
        //    printf("PWD INTERNE %s\n", pwd);
         //   write(tubes[ECRITURE], &pwd,strlen(pwd));
            fprintf(stdout, "%s", pwd); 
        }
        else if (execvp( args[0], args) == -1) {
            printf("Commande Inconnue : %s\n",args[0]);
            fflush(stdout); 
			kill(getpid(),SIGTERM);
            return 1; // si l'exec fail
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
    argc = 0;
    while(next != NULL) { // tant qu un nouvel espace est present,
        args[i] = cmd;
        next[0] = '\0';
        ++i;
        argc ++;
        cmd = removeSpace(next + 1);
        next = strchr(cmd, ' ');
    }
 
    if (cmd[0] != '\0') {
        args[i] = cmd;
        next = strchr(cmd, '\n');
        next[0] = '\0';
        argc ++;
        ++i;
    }

    args[i] = NULL; // dernier arg de args = NULL
}

// Liste des commandes reconnues par le shell //
static void cd(){
            	//CD Arg
            	
            	//Reset temp
            	strcpy(pwdtmp2,"");
            	strcpy(arboTarTmp,"");
            	
            	//si arg = /*
      			if(  args[1][0] == '/' ) {
					strcpy(pwdtmp, args[1]); 
				}
				//si arg != /* , on ajoute l'arg au pwd actuel
				else {
				    strcpy(pwdtmp,pwd);
				    strcat(pwdtmp,"/");
				    strcat(pwdtmp,args[1]);
				}	
				
				//DEBUG printf(">>>> %s\n",pwdtmp);
				
				//Parse command. On essaye deja de chdir dessus 
				if(chdir(pwdtmp) == -1) { // -1 --> FAIL
			    				
				//si pwdtmp contient ".tar" , on decoupe pwdtmp en pwdtmp2 + tarname + arboTar avec les "/"
					if(strstr(pwdtmp, ".tar") != NULL) {	
						  int len = strlen(pwdtmp);
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
						      	//ajout arboTarTmp
									strcat(arboTarTmp,p) ; 
									strcat(arboTarTmp,"/") ; 
						      }
					    	  //Debug  printf("'%s'\n", p);
						    p = strtok(NULL, d);
						  }	
				//Fin Decoupe			
														   
					/* DEBUG
						   printf("\n");
						   printf("PWDTMP = %s\n",pwdtmp2);
						   printf("TARNAME = %s\n",tarname);
						   printf("ARBOTARTmp = %s\n\n",arboTarTmp);
					*/	   
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
	                               if(  ( arboTarTmp[0] == '\0' ) || ( get_fichier_type(fdt, arboTarTmp) == '5'  ) ) {
	                                   strcpy(old_pwd, pwd);
	                                   strcpy(pwd, pwdtmp2);
	                                   strcat(pwd, "/");
	                                   strcat(pwd, arboTarTmp);
	                                   
	                                   //Maj arbotar
	                                   strcpy(arboTar, arboTarTmp);
	                                   strcat(arboTar,"/");
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

    if (args[0] != NULL) {
        //commandes SHELL
            //Implementation Exit
        if (strcmp(args[0], "exit") == 0) {
            printf("Bye ! \n");
            free(tarname);
            free(arboTar);
            free(old_pwd);
            free(pwdtmp);
            exit(0);
        }
            //Implementation PWD pour tarball
   //     else if (strcmp(args[0], "pwd") == 0){ //pwd passe en tarball
   //         printf("%s\n", pwd);
   //     }

            //Implementation CD
        else if (strcmp(args[0],"cd") == 0){

            // cd <sans args> / go to HOME
            if (args[1] == NULL ){ //determine the size of tableau args 
                strcpy(old_pwd, pwd);
                strcpy(pwd, home_pwd);
                chdir(pwd);
            }
            //cd - / go to previous
            else if (strcmp(args[1], "-")== 0){
                chdir(old_pwd);
                strcpy(pwdtmp, pwd);
                strcpy(pwd, old_pwd);
                strcpy(old_pwd, pwdtmp); //permuter pwd et pwdtmp
            } 
                   
            else if ((strstr(args[1], "..")) != 0){//cd ..
							//on ajoute l'arg au pwd actuel dans pwdtmp , puis on decoupe la commande avec des / , pour supprimer ce qui se trouve avant les ".." avant de reconstruire la commande
                           strcpy(old_pwd, pwd);
                           strcpy(pwdtmp2, "");
                           strcpy(pwdtmp,pwd);
				    	   strcat(pwdtmp,"/");
				           strcat(pwdtmp,args[1]);
                           
                           //decoupe 
                           	int len = strlen(pwdtmp);
						    int prevdist=0;
						    int dist=0;
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
							strcpy(args[1],pwdtmp2);
							cd();
			
                       }
            
            //cd ~ / go to HOME
            else if (strcmp(args[1], "~")== 0){
                strcpy(old_pwd, pwd);
                strcpy(pwd, home_pwd);
                chdir(pwd);
            } 

            else {
				cd();				
            }
        }
        
        
        
        //afficher_fichier => cat
        else if (!strcmp(args[0], "cat2")){
              if (estDansTar == 1 ) {
                 //int fdx = open(args[1], O_RDONLY);
                  int fdx = open(tarname, O_RDONLY);
                  if (fdx < 0){
                       perror("open");
                       return -1;
                  }
                  Tmp = malloc(sizeof(char) * BUFFER);
                  strcpy(Tmp,arboTar);
                  strcat(Tmp,args[1]);
                  afficher_fichier(fdx,Tmp);
                  close(fdx);
                  free(Tmp);
                  }
              else {
                  printf("Use cat2 after entering in a tar file\n");
              }
          }
        //afficher_fichier => cat
        else if (!strcmp(args[0], "cat2")){
             int fdx = open(args[1], O_RDONLY);
             if (fdx < 0){
                  perror("open");
                  return -1;
             }
             afficher_fichier(fdx, args[2]);
             close(fdx);
        }
        
        //afficher_repertoire => ls
        else if (!strcmp(args[0], "ls2")){
                int fdx;
                //afficher repertoire et droit
                if (!strcmp(args[1], "-l")){
                    fdx = open(args[2], O_RDONLY);
                    if (fdx < 0){
                    perror("open");
                    return -1;
                    }
                    if ( argc == 4) afficher_repertoire(fdx, trouve(fdx,args[3]), 1);
                    if ( argc == 3) afficher_tar_content(fdx ,1);

                }else{
                    fdx = open(args[1], O_RDONLY);
                    if (fdx < 0){
                    perror("open");
                    return -1;
                    }
                    if (argc == 3) afficher_repertoire(fdx, trouve(fdx,args[2]), 0);
                    if ( argc == 2) afficher_tar_content(fdx ,0);

                }
                close(fdx);
        }
        
        
        //get_fichier_type =>
        else if (!strcmp(args[0], "gft")){
                int fdx = open(args[1], O_RDONLY);
                if (fdx < 0){
                    perror("open");
                    return -1;
                }
                get_fichier_type(fdx, args[2]);
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
    arboTarTmp = malloc(sizeof(char) * BUFFER);
    home_pwd = getenv("HOME");
    strcpy(old_pwd, pwd);
    
    while (1) {
	    //suppression du dernier / dans la commande demandee => xx/toto.tar/ devient xx/toto.tar
		if(  pwd[strlen(pwd)-1] == '/' ) {
		   pwd[strlen(pwd)-1] = '\0';
		}
    	
	
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
        char* next = strchr(cmd, '|'); // recherche du premier pipe
 
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
