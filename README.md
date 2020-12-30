Systèmes L3 2020-2021
======================
 
**L'équipe 6 : MECHOUAR Fella / SU LiFang / BADJI Sidy**
 
# Installation du projet
 
 ## Branche : master, Tag : soutenance, Rapport : ARCHITECTURE.md

### Comment installer sur antiX (en root)

* Package à installer : gcc, manpages-dev et rlwarp
    - apt update  //pour mettre à jour la liste des packages
    - apt install build essential //pour installer gcc
    - apt install rlwrap //pour installer rlwrap
    - apt-get install manpages-dev // pour mettre le man a jour
       
* Mettre à jour la date et l'heure sur antiX avec la commande root 
    - /usr/local/bin/set_time-and_date.sh
    
* Si besoin utiliser la commande "setxkbmap fr" afin de mettre le clavier en azerty
    
* Le programme se compile via make, un Makefile étant présent afin de compiler les sources. Cela crée un unique binaire exécutable : shell
    - make clean
    - make
       
* Le shell s'exécute ainsi :
    - rlwrap ./shell
       
*  Un mode debug est aussi disponible via l'ajout de l'argument "-debug"
    - rlwrap ./shell -debug
      
* L'utilitaire rlwrap permettant l'utilisation des flèches du haut et bas afin de rappeler les commandes précédentes du shell.
