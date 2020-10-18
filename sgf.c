#include<sys/types.h>
#include <unistd.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <assert.h>
#include"tar.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

off_t trouve(int fd, char *filename){
  int filesize = 0;
  lseek(fd, 0, SEEK_SET);
  struct posix_header p;
  if(fd < 0){
    perror("Fichier n'existe pas");
    exit(1);
  }

  read(fd, &p, BLOCKSIZE);
  while(p.name[0] != '\0' ){
    if(strcmp(p.name , filename) == 0){
      sscanf(p.size, "%o", &filesize);
      int diff = ((filesize + BLOCKSIZE - 1)/BLOCKSIZE)*BLOCKSIZE;
      return  lseek(fd, diff, SEEK_CUR);
    }
     
    read(fd, &p, BLOCKSIZE);
  }
  return  -1;
}

void delete_fichier(int fd, char *filename){
  int filesize = 0;
  struct posix_header p;
  off_t position = 0;
  trouve(fd, filename);
  if(trouve(fd, filename) == -1) {
    perror("delete_fichier : Erreur position");
    exit(2);
  }
  else{
    position = trouve(fd, filename);
    
    lseek(fd, position - 512, SEEK_CUR) ;
    read(fd, &p, BLOCKSIZE);
    while(p.name[0] != '\0'){
      printf("fichier %s de type %c et de  taille : %s\n", p.name, p.typeflag, p.size);    
      sscanf(p.size, "%o", &filesize);
      int diff = ((filesize + BLOCKSIZE - 1)/512)*BLOCKSIZE;
      lseek(fd, diff, SEEK_CUR);
    read(fd, &p, BLOCKSIZE);
  }
  }
}

int main(int argc , char **argv){
  int fd = open(argv[1], O_RDONLY);

  //printf("%ld\n", trouve(fd, argv[2]));
  delete_fichier(fd, argv[2]);
  return 0;
}
