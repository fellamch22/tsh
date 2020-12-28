
#ifndef SGF_H
#define SGF_H

char * fileToBlocks( int fd , char * filename ,int * nb_blocks );
void addFile( int fd, int fd1, char * src_filename ,off_t position);
void newEmptyDirectory(int fd ,char * directoryPath );
void afficher_tar_content(int fd , int mode);
void afficher_fichier(int fd, char *chemin);
void afficher_repertoire(int fd, off_t position, int mode);
char get_fichier_type(int fd, char *chemin,int debug);
char * modeToString(int mode, char type );
off_t trouve(int fd, char *filename);
void delete_fichier(int fd, char *filename);
void delete_repertoire(int fd, char *repname);
void block_to_file(int fd, char * src_path, char* dst_path);
void block_to_directory(int fd, char * src_path,char* dst_path);
void copy_tarball_into_tarball(char * src_path ,int fd_src ,char * dst_path, int fd_dst );
void copy_directory_to_tarball(char * src_path ,char * dst_path , int fd_dst);
int cp_srctar( char * src_path , int src_fd , char * dst_path  , int dst_fd , int option );
int cp_srcsimple( char * src_path , char * dst_path  , int dst_fd , int option );


#endif
