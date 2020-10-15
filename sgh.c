#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "sgh.h"

int ft_strlen(char *s) {
    int i = 0;
    while(s[i])
        i++;
    return i;
}

char get_fichier_type(int fd, char *chemin){
    
    struct stat buff;
    fstat(fd, &buff);
    int nb_blocks = (buff.st_size + 512 - 1) / 512;
    struct posix_header ph;
    char nomfic[100];
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
                default :
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



void afficher_fichier(int fd, off_t position){
    
    if (fd >= 0){
        char buff[BLOCKSIZE];

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
         if (!strncmp(ph.name, ph.name, ft_strlen(ph.name)))
             printf(" [%c] > %s\n",ph.typeflag, ph.name);
         i++;
     }
}

