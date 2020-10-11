#include <string.h>
#include <stdio.h>
#include "tar.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

char get_fichier_type(int fd, char *chemin){
    struct stat buff;
    fstat(fd, &buff);
    int nb_blocks = (buff.st_size + 512 - 1) / 512;
    //nb blocks exitant dans le fichier.tar
    
    struct posix_header ph;

    int i = 0;
    while (i < nb_blocks - 2)
    {
        bzero(&ph, sizeof(ph));
        read(fd, &ph, sizeof(ph));

        if (!strcmp(ph.name, chemin)) {
            if (ph.typeflag == '0')
                printf("Le chemin mene a un fichier");
            else if (ph.typeflag == '5')
                printf("Le chemin mene a un directory");
            else
                printf("Le chemin mene a un autre type");
            return (ph.typeflag);
        }
        i++;
    }
    printf("Le chemin n'existe pas\n");
    return 0;
}

int main(int argc, char * argv[])
{
    if (argc != 3)
    {
        printf("Wrong number of arguments\nUsage: ./listar <file.tar> <chemin>\n");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    char ret = get_fichier_type(fd, argv[2]);
    close(fd);

    return 0;
}
