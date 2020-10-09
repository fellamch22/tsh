#include <string.h>
#include <unistd.h>
#include <strings.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include "tar.h"




char get_fichier_type(int fd, char * chemin){
    
    struct posix_header ph;
    read(fd, &ph, BLOCKSIZE);

    if(fd == -1){
      perror("open failed");
      exit(1);
    }

    if (ph.typeflag == '0') {
        printf("Type[0] : fichier ordinaire\n");
    }else if (ph.typeflag == '5') {
        printf("Type[5] : directory\n");   
    }else{printf("Type[%d] : autre\n", ph.typeflag);
    }

    close(fd);

}