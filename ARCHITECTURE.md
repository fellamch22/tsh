SYSTÈME : premier rendu projet
======================

**L3 Informatique l'équipe 6 : MECHOUAR Fella / SU LiFang / BADJI Sidy**

# Final Rendu

## Branche : master, Tag : soutenace



### Introduction
#### Fonctionnement du Shell
Vous pouvez trouver ces  foncitons dans le fichier sgf.c et shell.c. Tous les tests sont effetués sur antiX.

* Le shell tsh est fait d'un prompt mis dans une boucle infinie, attendant une commande jusqu'a l'écriture de la commande "exit" par l'utilsateur.

* Ce prompt est composé par une variable interne au shell nommée pwd , représentant le répertoire courant lors de l'execution du programme, par exemple :  /home/user/VB/Shell$>

* Cette pwd sera évolué par la suite , et elle est différents du PWD systeme lors des passages dans les fichiers tar. On peut changer de répertoire via la commande "cd" qui a été entièrement  recodée, avec une légère modification : lors d'un appel cd sur un fichier tar, on simule une entrée dans ce fichier tar, en updatant le prompt de facon appropriée. Par la suite, on peut faire de nouveaux cd sur les différents répertoires le composant.  La commande "cd .." permet de remonter d'un niveau, même si on se trouve dans un fichier tar.

* La commande recue sur le prompt est ensuite découpée à chaque "pipe" afin de séparer les sous-commandes la composant, par exemple : la commande "ls -l | head -n 2 | wc -l " sera découpée en 3 parties. 

* Chaque sous commande est analysée via la fonction analyse( ) ou la sous commande est envoyée avec le fichier descripteur du processus précédent, et des variables indiquant si la commande est la premiere ou la derniere d'une chaine, par exemple : la commande "ls -l | head -n 2 | wc -l ", pour la premiere partie de commande "ls -l" sera la premiere,  "wc -l" sera la derniere. 

* Lors de l'analyse, chaque sous commande est découpée dans un tableau d'arguments args comprenant dans notre cas "ls" "-l", dans cette table, args[0] = "ls", args[1] = "-l". (cf. void decoupe(char* cmd) ) 

* Si la sous commmande est reconnue comme une commande shell, les commandes seront executées par le code de fonctions spécifiques, sinon les fonctions seront considérées comme des fonctions usuelles et executées via la fonction executeCmd. Par exemple, la commande "ls",  on peut l'executer comme "ls" hors des tarballs (commande standard) , soit on l'execute comme une commande spécifique pour afficher les contenus d'un fichier tar. (cf. ls2 dans shell.c)

* Chaque sous commande fork un processus l'executant.
    - La premiere sous commande crée un process qui  comprend un tube en sortie vers le process suivant.
    - Les sous commandes centrales crée un process qui comprennent un tube en entrée depuis le process précédent, ainsi qu'un tube en sortie vers le process suivant.
    - La derniere sous commande crée un process qui comprend un tube en entrée depuis le process précédent.
    - Sur l'exemple , le shell crée un process child afin d'executer "wc -l", qui écoute le retour du process child executant " head -n 2 ", qui écoute lui-meme le retour du process executant le "ls -l"

                    fork1         fork2             fork3 (C'est le shell qui fait les forks)
            Shell <------- wc -l <------ head -n 2 <------ ls -l

### Installation
    comment installer sur antiX 
    - msg date, time 
    - package installé(gcc, rlwarp)
    comment installer sur Docker
    - dockerfile
    - package installé...etc
    
### Structure du shell ---> shell.c
    #### role de chaque fonction
    #### schema
### Gestion des fichiers tarball ---> sgf.c
    - explique chaque fonction dans sgf.c (afficher_rep, afficher_fichier, get_fichier_type...etc)
    ### Fonctions permettant l'ajout d'un fichier externe dans un tarball ( dans le fichier sgf.c )

    * char * fileToBlocks( int fd , char * filename , int * nb_blocks)
        - Cette fonction effectue la transformation du fichier pointé par le descripteur fd en un ensemble de blocks des taille de 512 chacun , compatibles avec la representation d'un fichier dans un tarball.
        - la fonction retourne un pointeur vers les blocs contruits pour le fichier , ainsi que le nombre de blocks aloués dans la variable 'nb_blocks'

    * void addFile( int fd, int fd1 , char * src_filename , off_t position)

        -  Cette fonction utilise le résultat  de la conversion du fichier pointé par fd1 par la fonction fileToBlocks, et l'insère à la position "position" dans le fichier .tar pointé par fd.

    ### Les fonctions pour supprimer fichier et repertoire dans le fichier .tar

    * off_t trouve(int fd, char *filename) 
        - Cette fonction permet de donner la position d’un fichier dans le fichier .tar
        - Si le fichier passé en argument existe celle ci renvoie une valeur de retour positive désignant la position du fichier sinon retourne -1
    * void delete_fichier(int fd, char *filename)
        - Cette fonction utilise la fonction trouve(int fd, char *filename) pour obtenir la position du fichier passer en argument
        - Si elle a sa position elle supprime le fichier
        - La suppression se fait avec decalage dans le fichier fichier .tar
        
    * void delete_repertoire(int fd, char *filename)
        - Cette fonction utilise la fonction trouve(int fd, char *filename) et la     fonction delete_fichier(int fd, char *repname)
        
    ### Les fonctions pour afficher les fichiers et les répertroires dans les tarballs 

    * char get_fichier_type(int fd, char *chemin){...}
        - Cette fonction est pour obtenir le typeflag des tarballs
        - Elle permet de gérer la variable pwd interne au shell
        - Vérifie que l'on fait les cd sur des répertoires dans les fichiers tar
        - La commande pour exectuer dans le shell : gft <fichier.tar> <fichier>
        
    * void afficher_fichier(int fd, char *chemin){...}
        - Cette fonction permet d'afficher le contenu d'un fichier dans un fichier tar
        - Elle est le composant de la fonction cat2 que nous avons crée
        - La commande pour exectuer dans le shell : il faut d'abord renter dans un fichier tar, après on peut executer cat2 <fichier>

    * void afficher_repertoire(int fd, off_t position, int mode){...}
        - Cette fonction est pour afficher le contenu d'un fichier tar.
        - Cela est fait une partie du syntaxe de "ls2"

### Analyse syntaxique ? ---> syntaxique.c

### Test effectué
    * Le programme se compile via make. Cela crée deux binary executable : shell (tsh) et sgf (Test des fonctions), s'executant sans arguments. 

    * Pour la partie shell : 
        - des l'executions, le prompt apparait avec la localisation actuelle sur le systeme de fichiers (pwd). 
        - Commandes executables : 
            - exit (Termine le programme)
            - cd < dir >, cd < tarfile >, cd < dir dans un tarfile >, cd ..
            - cat2 < fichier inclus dans un tarfile >, apres etre deja rentre dedans via cd <tarfile>
            - ls2 <fichier.tar> <chemin  (option)>
            - ls2 -l <fichier.tar> <chemin  (option)>
            - Toutes les commandes externes fonctionnent directement, dont les commandes avec pipe (" | ") 
            
    * Pour la partie sgf : 
        - Partie Ajout dans le fichier .tar : fileToBlocks / addFile
            - ./sgf 
        - Partie  Suppression fichier et repertoire dans le fichier .tar : delete_repertoire / delete_fichier / trouve
            - ./sgf < fichier.tar > < fichier >

# Partie suppession:

    Tout d'abord j'ai commencé par créer dans notre systeme de gestion de fichiers trois fonctions utilisant la structure posix et les conditions pour pouvoir manipuler les fichiers ".tar" ayant chacun ces fonctionalités et agissant sur les commandes concernées telles que : 
     
        -rmdir
        -rm
        -rm -r

   # Les fonctions pour suppression dans SGF:

            * off_t trouve(int fd, char *filename) 
                - Cette fonction permet de donner la position d’un fichier dans le fichier .tar
                - Si le fichier passé en argument existe celle ci renvoie une valeur de retour positive désignant la position du fichier sinon retourne -1
            * void delete_fichier(int fd, char *filename)
                - Cette fonction utilise la fonction trouve(int fd, char *filename) pour obtenir la position du fichier passer en argument
                - Si elle a sa position elle supprime le fichier
                - La suppression se fait avec decalage dans le fichier fichier .tar
    
            * void delete_repertoire(int fd, char *repname)
                - Cette fonction utilise la fonction trouve(int fd, char *filename) et la     fonction delete_fichier(int fd, char *repname)

   # L'intégration des fonctions dans Shell :
            
            En effet, j'ai utilisé ces fonctions pour faire les différentes commandes ci dessous :

                *rmdir() :
                    Cette commande permet de supprimer un repertoire mis en argument.
                    Ainsi, je fais appel à la fonction void delete_repertoire(int fd, char *repname) qui permet à partir des processus d'ouvrir un fichier descripteur, si ce dernier ne renvoie pas d'erreur, en suivant la structure d'un fichier ".tar" :
                    Il pointe sur l'entête du fichier qui permettra son tours de pointé sur les sur les fichiers contenus dans le fichier grâce a la fonction off_t trouve(int fd, char *filename) qui renvoie la position du fichier en argument et ensuite suivre le reste des instructions fait dans la fonction void delete_repertoire(int fd, char *repname) pour la suppression.
                

                *rm() :

                    Cette commande permet de supprimer un fichier simple mis en argument.
                    Ainsi, je fais appel à la fonction void delete_fichier(int fd, char *filename) qui permet à partir des processus d'ouvrir un fichier descripteur, si ce dernier ne renvoie pas d'erreur, en suivant la structure d'un fichier ".tar" :
                    Il pointe sur l'entête du fichier qui permettra son tours de pointé sur les sur les fichiers contenus dans le fichier grâce a la fonction off_t trouve(int fd, char *filename) qui renvoie la position du fichier en argument et ensuite suivre le reste des instructions fait dans la fonction void delete_filename(int fd, char *repname) pour la suppression.
                    Ce qui fait la meme procéder pour la suppression d'un repertoire sauf qu'ici les deux fonctions dans le SGF sont differente.
                    Cependant il y'a une autre perpective avec rm() que l'on va decrire ci dessous sa fonctionalité.

                *rmr():

                    Cette commande permet de faire la suppression recursive.
                    Cependant dans le shell elle utilise les deux fonctions en même temps qui suit le même procedé au depart mais ici la différence est qu'il permet de supprimer un repertoire contenant d'autres repertoires ou des fichiers.
                    En effet la commande regarde si c'est un repertoire elle parcours recursivement le repertoire avec une suppression recursive si:
                        Le reppetoire contient un fichier ou des fichiers elle utilise la fonction void delete_fichier(int fd, char *filename) qui va lui permettre de supprimer c/ces dernier(s)
                        Le repertoir ccontient un repertoire ou des repertoires elle utilise la fonction  void delete_repertoire(int fd, char *repname) pour faire la suppression, et enfin supprimer le repertoire courant.

   # Mode d'utilisation :
            Après  avoir compiler pour acceder dans notre shell on utilise les commandes comme montré ci dessous :

                    Exemple :

                        -rmdir() :
                             
                             rmdir fichier.tar repertoire/

                        -rm() :

                            rm fichier.tar fichier

                        -rmr() :

                            rm -r fichier.tar repertoire/
                            le repertoir peut contenir des fichier ou pas il sera supprimer


   # Problèmes rencontrés :

            J'ai eu a rencontré quelques problème   d'inattention pour la réalisation de mes commandes qui a un peu duré mais avec le travail d'équipe j'ai fini par le résoudre.
            J'avais ouvert le fichier descripteur en lecture seulement alors que je devais le faire en lecture ecriture.

            Pour les fonctions dans le SGF j'ai eu en rencontré plusieurs prblème mais ça concernait l'incompréhension de la structure d'un fichier ".tar"
            et cela pu être resouds avec l'aides de mes coéquipiés.
            Ainsi des problèmes de temps comme la majeur partie des étudiants vu le déroulement de cette année. 

   # Conclusion :

        En somme le travail a été très enrichissant sur le plan strategique c'est à dire la répartition des taches, le travail de groupe.
        Cependant,ça reste quand même stressant à cause du temps les questions qui ont été posés sur gitLab qui permettaient toujours d'améliorer le travail et de revoir les parties qui ont échappés à notre vigilanche.
        
### Problème rencontré 
### Conclusion

