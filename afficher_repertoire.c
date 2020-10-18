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
    struct stat buff;
    fstat(fd, &buff);
    // Ici on calcule le nombre de blocks qu'il y a dans le fichier.tar
    // tel que nbBlock * 512 = taille du fichier
    // <==> nbBlock = taille de fichier / 512
    int nb_blocks = (buff.st_size + BLOCKSIZE - 1) / BLOCKSIZE;
    struct posix_header ph;

    off_t i = 0;
    while (i < position){
        memset(&ph, 0, sizeof(ph));
        if (read(fd, &ph, sizeof(ph)) == -1) {
            perror("read\n");
            return ;
        }
        i++;
    }
    memset(&ph, 0, sizeof(ph));
    read(fd, &ph, sizeof(ph));
    char directory[sizeof(ph.name)];
    memset(&directory, 0, sizeof(ph.name));
    
    for (int j = 0; j < ft_strlen(ph.name); j++) {
        directory[j] = ph.name[j];
    }
    
    while (i < nb_blocks - 2){
        memset(&ph, 0, sizeof(ph));
        read(fd, &ph, sizeof(ph));
        if (!strncmp(directory, ph.name, ft_strlen(directory))){
            printf("%s\n", &ph.name[ft_strlen(directory)]);
        }
        i++;
    }
}

int main(int argc, char * argv[])
{
    int fd = open(argv[1], O_RDONLY);
    afficher_repertoire(fd, 0);
    close(fd);

    return 0;
}
