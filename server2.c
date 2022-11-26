#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

//#include "testPile2.c"

#define PORT 12345
#define IP "127.0.0.1"

typedef struct Joueur
    {
        int nbCarte;
        int* cartes;
        int teteBoeuf;
    }Joueur;

typedef struct ClientCon
{
    // Descripteur du socket
    int fd;
    // Socket ouvert ?
    bool isOpened;
    // Thread associé à mla connexion et gérant la communication de ce client.
    pthread_t threadId;
}ClientCon;

//Liste des connexions clients
ClientCon cliSockets[10];
unsigned int nbConnections = 0;

// Descripteur du socket serveur
int srvSocket;
// Le socket serveur ouvert ?
bool isSocketOpened = false;


//Déclaration des fonctions
int createSocketServer();
void acceptConnections(unsigned int nbConnections);

int main(int argc, char const *argv[])
{
    printf("Entrez le nombre de joueur : \n");
    scanf("%d",&nbConnections);
    int test = createSocketServer(); 
    acceptConnections(nbConnections);

    close(srvSocket);

    return 0;
}

int createSocketServer()
{ 
    printf("Starting server...\n");

    // Création du socket serveur
    int serverSocket = socket(AF_INET,SOCK_STREAM, 0);
    if(serverSocket == -1){
        printf("[-] Error in connection. \n");
        exit(1);
    }

    // Parametrage du socket
    struct sockaddr_in addrServer;
    socklen_t recsize = sizeof(addrServer);
    addrServer.sin_addr.s_addr = inet_addr(IP);
    addrServer.sin_family = AF_INET;
    addrServer.sin_port = htons(PORT);

    // Autorise la réutilisateur du port pour pouvoir relancer le serveur
    int autoriser = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &autoriser, sizeof(autoriser));

    if(bind(serverSocket, (struct sockaddr*)&addrServer, recsize) == -1)
        perror("erreur bind");

    isSocketOpened = true;

    // Si erreur lors de l'écoute, 10 joueurs max
    if (listen(serverSocket, 10) == -1) 
        perror("erreur listen");

    return serverSocket;
}

void acceptConnections(unsigned int nbConnections)
{
    struct sockaddr_in addr;
    socklen_t csize = sizeof(addr);
    int acceptF;
    int retpthread;
    pthread_t *pthread_t_j_array = malloc(sizeof(pthread_t)*nbConnections);
    if(pthread_t_j_array == NULL){
    perror("erreur thread");
    };
    //Pour tous les joueurs
    for(size_t i=0; i<nbConnections;i++){
        acceptF = accept(srvSocket, (struct sockaddr *)&addr, &csize);
        if(acceptF==-1){
            perror("erreur accept");
        };
    }


    printf("Stopped listening client connections. Total of clients connected: %i.\n", nbConnections);
}

