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
#include "tar.h"

#define BUFFER 1024
#define LECTURE  0
#define ECRITURE 1

static char* args[BUFFER];
pid_t pid;
static char* pwd; // current dir
static char ligne[BUFFER]; // commande a analyser
static int nbexecuteCmds = 0; // nombre de executeCmdes
char cwd[BUFFER]; // durrent dir


static off_t trouve(int fd, char *filename){
    int filesize = 0;
    struct posix_header p;
      lseek(fd, 0, SEEK_SET);

    if(fd < 0){

            perror("Fichier n'existe pas");
            exit(1);
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


static void afficher_fichier(int fd, char *chemin){

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

static void afficher_repertoire(int fd, off_t position, int mode){

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


    if(mode == 1){
        //typeflag [0/5/...]
        char *res = (char*) malloc (BUFFER);
        strcat(res, &p.typeflag);
        //ls -l
        strcat(res, p.mode);//mode
        //-rwxr-xr-x nboflink username staff inode dd mm date filename
        /*switch(p.mode){
              case '0007' :
                printf();
                break;
                
                
             }*/
        /*printf("File Permissions: \t");
        printf( (S_ISDIR(p.st_mode)) ? "d" : "-");
        printf( (p.st_mode & S_IRUSR) ? "r" : "-");
        printf( (p.st_mode & S_IWUSR) ? "w" : "-");
        printf( (p.st_mode & S_IXUSR) ? "x" : "-");
        printf( (p.st_mode & S_IRGRP) ? "r" : "-");
        printf( (p.st_mode & S_IWGRP) ? "w" : "-");
        printf( (p.st_mode & S_IXGRP) ? "x" : "-");
        printf( (p.st_mode & S_IROTH) ? "r" : "-");
        printf( (p.st_mode & S_IWOTH) ? "w" : "-");
        printf( (p.st_mode & S_IXOTH) ? "x" : "-");*/

        strcat(res, p.name);
        write(1, res, strlen(res));

    
    }else{
        write(1, p.name, strlen(p.name));
    }

    write(1,"\n",1);

       if( read (fd , &p, BLOCKSIZE) <= 0 ){
        perror(" ERREUR read ");
        exit(1);
    }

    while(strncmp(repname,p.name,strlen(repname)) == 0){



        if(mode == 1){
            //typeflag [0/5/...]
            char *res2 = (char*) malloc (BUFFER);
            strcat(res2, &p.typeflag);
            strcat(res2, p.name);
            write(1,res2,strlen(res2));
        }else{
            write(1,p.name,strlen(p.name));
        }
        
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

 
 /*
 * fd: numero du file descriptor
 * premiere: 1 si premiere commande dans la sequence
 * derniere: 1 si derniere commande dans la sequence
 *
 * EXEMPLE avec la commande "ls -l | head -n 2 | wc -l" :
 *    fd1  = executeCmd(0, 1, 0), with args[0] = "ls" and args[1] = "-l"
 *    fd2  = executeCmd(fd1, 0, 0), with args[0] = "head" and args[1] = "-n" and args[2] = "2"
 *    fd3  = executeCmd(fd2, 0, 1), with args[0] = "wc" and args[1] = "-l"
 *
 */
 
 
static int executeCmd(int fd, int premiere, int derniere)
{
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
        
        printf("> fd=%d premiere=%d derniere=%d\n",fd,premiere,derniere);
        
        if (fd == 0 && premiere == 1 && derniere == 0  ) {
            // pour la premiere commande redirection de la sortie dans le tube
            dup2( tubes[ECRITURE], STDOUT_FILENO );
        } else if (fd != 0 && premiere == 0 && derniere == 0 ) {
            // pour les commende du milieu, ecoute de l entree depuis fd,
            // redirection de la sortie dans tubes[ECRITURE]
            dup2(fd, STDIN_FILENO);
            dup2(tubes[ECRITURE], STDOUT_FILENO);
        } else {
            // pour la derniere commande , ecoute de l entree depuis fd
            dup2( fd, STDIN_FILENO );
        }
        if (execvp( args[0], args) == -1) {
            printf("Commande Inconnue : %s\n",args[0]);
            return 1; // si l'exec fail
        }
    }
    
    //nettoyage fd
    if (fd != 0) {
        close(fd);
    }
 
    // plus rien ne doit etre ecrit
    close(tubes[ECRITURE]);
 
    // plus rien ne doit etre lu si il s'agit de la derniere executeCmde
    if (derniere == 1) {
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
    while(next != NULL) { // tant qu un nouvel espace est present,
        args[i] = cmd;
        next[0] = '\0';
        ++i;
        cmd = removeSpace(next + 1);
        next = strchr(cmd, ' ');
    }
 
    if (cmd[0] != '\0') {
        args[i] = cmd;
        next = strchr(cmd, '\n');
        next[0] = '\0';
        ++i;
    }

    args[i] = NULL; // dernier arg de args = NULL
}

// Liste des commandes reconnues par le shell //

static int analyse(char* cmd, int fd, int premiere, int derniere)
{
    //printf("CMD= %s fd=%d premiere=%d derniere=%d\n",cmd,fd,premiere,derniere);
    decoupe(cmd); // decoupe la commande proprement dans le tableau d'args
    if (args[0] != NULL) {
        //commandes SHELL
            //Implementation Exit
        if (strcmp(args[0], "exit") == 0) {
            printf("Bye ! \n");
            exit(0);
        }
            //Implementation CD
        else if (!strcmp(args[0], "cd")){
            if ((strcmp(args[1], "..")) == 0){
                chdir(".."); //change repertoire
            }
            else if (strcmp(args[1],"..") != 0 ) {
                strcat(pwd,"/");
                strcat(pwd,args[1]);
                if(chdir(pwd) == -1) {
                    printf("Cannot change directory \n");
                }
            }
            //il faut ajouter ici si c'est un .tar ... et exec les commandes appropri�es
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

                    afficher_repertoire(fdx, 0, 1);
                }else{
                    fdx = open(args[1], O_RDONLY);
                    if (fdx < 0){
                    perror("open");
                    return -1;
                    }
                    afficher_repertoire(fdx, 0, 0);
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
        return executeCmd(fd, premiere, derniere);
        }
    }
    return 0;
}

int main()
{
    printf("(%d) Shell\n",getpid());
    while (1) {
        pwd = getcwd(cwd, sizeof(cwd));
        // Prompt
        printf("\n%s$> ",pwd);
        fflush(NULL);
 
        // Lecture executeCmde
        if (!fgets(ligne, BUFFER, stdin)) {
            return 0;
        }
 
        int fd = 0;
        int premiere = 1;
        char* cmd = ligne;
        char* next = strchr(cmd, '|'); // recherche du premier pipe
 
        while (next != NULL) {
            // next => pipe
            *next = '\0';
            fd = analyse(cmd, fd, premiere, 0); // on lance une analyse des commandes precedant chaque pipe
            cmd = next + 1;
            next = strchr(cmd, '|'); // prochain pipe
            premiere = 0;
        }
        fd = analyse(cmd, fd, premiere, 1); // on analyse la cmd avec derniere = 1
        nettoyage(nbexecuteCmds);
        nbexecuteCmds = 0;
    }
    return 0;
}
