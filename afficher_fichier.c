#include <string.h>
#include <stdio.h>
#include "tar.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

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

int main(int argc, char * argv[]){
    if (argc != 2){
        perror("Wrong number of arguments\nUsage: ./afficher_fichier <fichier>\n");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0){
        perror("open");
        return -1;
    }

    afficher_fichier(fd, 1);
    if (close(fd) == -1){
        perror("close");
        return -1;
    }

    return 0;
}
