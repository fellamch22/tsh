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

void cmd_ls(int fd, char *chemin){
    struct stat buff;
    fstat(fd, &buff);
    int nb_blocks = (buff.st_size + BLOCKSIZE - 1) / BLOCKSIZE;
    struct posix_header ph;

    off_t i = 0;
    while (i < nb_blocks - 2){
        memset(&ph, 0, sizeof(ph));
        if (read(fd, &ph, sizeof(ph)) == -1) {
            perror("read\n");
            return ;
        }
        if (!strcmp(ph.name, chemin)) {

            char directory[100] = { 0 };

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
        i++;
    }
}

int main(int argc, char * argv[])
{
    int fd = open(argv[1], O_RDONLY);
    cmd_ls(fd, argv[2]);
    close(fd);

    return 0;
}
