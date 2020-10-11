#include <string.h>
#include <stdio.h>
#include "tar.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

off_t ft_strlen(char *s) {
	off_t i = 0;
	while(s[i])
		i++;
	return 1;
}

void afficher_fichier(int fd, off_t position){
    
	if (fd >= 0)
	{
		char buff[512];

		off_t i = 0;
		while (i < position)
		{
			bzero(buff, sizeof(buff));
			if (read(fd, buff, sizeof(buff)) == -1) {
                perror("read\n");
				return ;
			}
			i++;
		}
		if (write(1, buff, sizeof(buff)) == -1)
		{
            perror("write : ");
			return ;
		}
	}
	else {
        perror("bad file descriptor : ");
}

int main(int argc, char * argv[])
{
    if (argc != 2)
    {
        perror("Wrong number of arguments : ");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
	if (fd < 0){
        perror("write : open");
		return -1;
	}
    afficher_fichier(fd, 3);
    
    if (close(fd) == -1){
		perror("close : ");
		return -1;
	}

    return 0;
}
