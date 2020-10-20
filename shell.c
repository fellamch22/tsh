#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER 1024
#define LECTURE  0
#define ECRITURE 1

#include <ctype.h>
#include "tar.h"
#include <sys/stat.h>
#include <fcntl.h>

static char* args[BUFFER];
pid_t pid;
static char* pwd; // current dir
static char ligne[BUFFER]; // commande a analyser
static int nbexecuteCmds = 0; // nombre de executeCmdes
char cwd[BUFFER]; // durrent dir
 


int ft_strlen(char *s) {
	int i = 0;
	while(s[i])
		i++;
	return i;
}

void afficher_repertoire(int fd, off_t position){
	if (fd == -1){
		perror("open");
	}

    struct stat buff;
    fstat(fd, &buff);
	// Ici on calcule le nombre de blocks qu'il y a dans le fichier.tar
	// tel que nbBlock * BLOCKSIZE = taille du fichier
	// <==> nbBlock = taille de fichier / BLOCKSIZE

    int nb_blocks = (buff.st_size + BLOCKSIZE - 1) / BLOCKSIZE;
    struct posix_header ph;

    off_t i = 0;
	while (i < position){
		bzero(&ph, sizeof(ph));
		if (read(fd, &ph, sizeof(ph)) == -1) {
			perror("read");
			return ;
		}
		i++;
	}
	
    while (i < nb_blocks - 2){
        bzero(&ph, sizeof(ph));
        read(fd, &ph, sizeof(ph));
		if (!strncmp(ph.name, ph.name, ft_strlen(ph.name)) && isdigit(ph.typeflag))
			printf(" [%c] > %s\n",ph.typeflag, ph.name);
        i++;
    }
} 


char get_fichier_type(int fd, char *chemin){   
    struct stat buff;
    fstat(fd, &buff);
    int nb_blocks = (buff.st_size + BLOCKSIZE - 1) / BLOCKSIZE;
    //printf("st_size = %lld", buff.st_size);
    struct posix_header ph;
    char nomfic[sizeof(ph.name)];
    int i=0;
    while (i < nb_blocks - 2)
    {
        bzero(&ph, sizeof(ph));
        read(fd, &ph, sizeof(ph));
        strcpy(nomfic,ph.name);

        // On copie le ph.name dans nomfic, et on remplace dans nomfic le dernier caractere 
        // par un \0 si il s'agit d'un '/' , De meme pour le chemin
        if (nomfic[ft_strlen(nomfic)-1] == '/') {nomfic[ft_strlen(nomfic)-1] = '\0';}
        
        if (chemin[ft_strlen(chemin)-1] == '/') {chemin[ft_strlen(chemin)-1] = '\0';}

         if   (!strcmp(nomfic, chemin) ) {
             switch(ph.typeflag){
                case '0' : 
                            printf("[%c] Le chemin mene a un Fichier\n",ph.typeflag);
                            break; 
                case '1' : 
                            printf("[%c] Le chemin mene a un Lien materiel\n",ph.typeflag);
                            break; 
                case '2' : 
                            printf("[%c] Le chemin mene a un Lien symbolique\n",ph.typeflag);
                            break; 
                case '3' : 
                            printf("[%c] Le chemin mene a un Fichier special caractere\n",ph.typeflag);
                            break; 
                case '4' : 
                            printf("[%c] Le chemin mene a un Fichier special bloc\n",ph.typeflag);
                            break; 
                case '5' : 
                            printf("[%c] Le chemin mene a un Repertoire\n",ph.typeflag);
                            break;
                case '6' : 
                            printf("[%c] Le chemin mene a un Tube nomme\n",ph.typeflag);
                            break; 
                case '7' : 
                            printf("[%c] Le chemin mene a un Fichier contigu\n",ph.typeflag);
                            break; 
                default  : 
                            printf("[%c] Le chemin mene a un autre type\n",ph.typeflag);
                            break;
             }
             return (ph.typeflag);
        }
        i++;
    }
    printf("Le chemin n'existe pas\n");
    return 0;
}

static void afficher_fichier(int fd, off_t position){
    
	if (fd >= 0){
		char buff[BUFFER]; 

		off_t i = 0;

		while (i < position){
			bzero(buff, sizeof(buff));
			if (read(fd, buff, sizeof(buff)) == -1) {
				perror("read");
				return ;
			}
			i++;
		}
		if (write(1, buff, sizeof(buff)) == -1){
			perror("write");
			return ;
		}
	}
	else perror("bad file descriptor");
}
 
 /*
 * fd: numero du file descriptor
 * premiere: 1 si premiere executeCmde dans la sequence
 * derniere: 1 si derniere executeCmde dans la sequence
 *
 * EXEMPLE avec la executeCmde "ls -l | head -n 2 | wc -l" :
 *    fd1  = executeCmd(0, 1, 0), with args[0] = "ls" and args[1] = "-l"
 *    fd2  = executeCmd(fd1, 0, 0), with args[0] = "grep" and args[1] = "-n" and args[2] = "2"
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
			// pour la premiere executeCmde
			dup2( tubes[ECRITURE], STDOUT_FILENO );
		} else if (fd != 0 && premiere == 0 && derniere == 0 ) {
			// pour les executeCmdes du milieu
			dup2(fd, STDIN_FILENO);
			dup2(tubes[ECRITURE], STDOUT_FILENO);
		} else {
			// pour la derniere executeCmde
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
		//executeCmdES SHELL
			//Implementation Exit
		if (strcmp(args[0], "exit") == 0) {
			printf("Bye ! \n"); 
			exit(0);
		}
			//Implementation CD
		else if (!strcmp(args[0], "cd")){
			if ((strcmp(args[1], "..")) == 0){
				chdir("..");
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
		//afficher_fichier 
		else if (!strcmp(args[0], "cat2")){
			    int fdx = open(args[1], O_RDONLY);
				if (fdx < 0){
					perror("open");
					return -1;
				}
				afficher_fichier(fdx, SEEK_CUR);
		}
		//afficher_repertoire
		else if (!strcmp(args[0], "affr")){
			    int fdx = open(args[1], O_RDONLY);
				if (fdx < 0){
					perror("open");
					return -1;
				}
				afficher_repertoire(fdx, 0);
		}
		//get_fichier_type
		else if (!strcmp(args[0], "gft")){
			    int fdx = open(args[1], O_RDONLY);
				if (fdx < 0){
					perror("open");
					return -1;
				}
				get_fichier_type(fd, args[2]);
		}
		
		//Execution executeCmde
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
		printf("%s$> ",pwd);
		fflush(NULL);
 
		// Lecture executeCmde
		if (!fgets(ligne, 1024, stdin)) {
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
