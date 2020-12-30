FROM ubuntu:latest
RUN apt-get update
RUN apt-get -y install build-essential
RUN apt install  linux-libc-dev 
RUN apt install make
RUN mkdir /home/projet_sys
COPY Makefile /home/projet_sys/Makefile
COPY sgf.c /home/projet_sys/sgf.c
COPY sgf.h /home/projet_sys/sgf.h
COPY shell.c /home/projet_sys/shell.c
COPY syntaxe.c /home/projet_sys/syntaxe.c
COPY syntaxe.h /home/projet_sys/syntaxe.h
COPY tar.h /home/projet_sys/tar.h
