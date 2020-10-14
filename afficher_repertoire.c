#include <string.h>
#include <stdio.h>
#include "tar.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

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
        if (!strncmp(ph.name, ph.name, ft_strlen(ph.name)))
            printf("> %s\n",ph.name);
        i++;
    }
}

int main(int argc, char * argv[]){
    if (argc != 2){
        perror("Wrong number of arguments\nUsage: ./afficher_repertoire <fichier.tar>");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    afficher_repertoire(fd, 0);
    close(fd);

    return 0;
}
