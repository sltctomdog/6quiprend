#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>   
#include <pthread.h>
#include <fcntl.h>

#define FATAL(){fprintf(stderr,"%s\n",strerror(errno));exit(errno);}
#define PORT 4440
#define SIZE_INPUT_USER 5
#define SIZE_NAME 50
#define DECK_SIZE 104
#define SERVER_IP "127.0.0.1"

typedef struct{
    char name[SIZE_NAME];
    int *cartes;
    FILE *file_ptr;
    size_t nbCarte;
    size_t teteBoeuf;
    bool isBot;
}joueur;

typedef struct{
  joueur *lst;
  size_t size;
  size_t nb_client;
} joueurArray;