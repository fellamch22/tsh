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
    struct posix_header ph;

    int i=0;
    while (i < nb_blocks - 2)
    {
        bzero(&ph, sizeof(ph));
        read(fd, &ph, sizeof(ph));

        
         if (!strcmp(ph.name, chemin)) {
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

int main(int argc, char * argv[])
{
    if (argc != 3)
    {
        printf("Wrong number of arguments\nUsage: ./listar <file.tar> <chemin>\n");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    get_fichier_type(fd, argv[2]);
   
    close(fd);

    return 0;
}
