#include <string.h>
#include <stdio.h>
#include "tar.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

void print_file(int fd, unsigned int nb_block){
    char buff[BLOCKSIZE] = { 0 };

    while (nb_block) {
        read(fd, buff, sizeof(buff));

        printf("%s", buff);

        memset(buff, 0, sizeof(buff));
        nb_block--;
    }
}

void afficher_fichier(int fd, char *chemin){
    struct stat buff;
    fstat(fd, &buff);
    int nb_blocks = (buff.st_size + BLOCKSIZE - 1) / BLOCKSIZE;
    struct posix_header ph;

    int i = 0;
    while (i < nb_blocks - 2)
    {
        memset(&ph, 0, sizeof(ph));
        read(fd, &ph, sizeof(ph));

        if (!strcmp(ph.name, chemin)) {
            if (ph.typeflag == '0') {
                unsigned int file_size= 0;
                sscanf(ph.size, "%o", &file_size);
                
                /*nb de block qu'on lit*/
                unsigned int nb_block_file = (file_size + BLOCKSIZE - 1) / BLOCKSIZE;
                print_file(fd, nb_block_file);
                return;
            } else
                printf("Le chemin mene pas a un ficher");
        }
        i++;
    }
}

int main(int argc, char * argv[])
{
    if (argc != 3)
    {
        printf("Wrong number of arguments\nUsage: ./afficher_fichier <file.tar> <chemin>\n");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        perror("open\n");
        return -1;
    }
    afficher_fichier(fd, argv[2]);
    if (close(fd) == -1)
    {
        perror("close\n");
        return -1;
    }

    return 0;
}
