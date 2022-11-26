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

#define PORT 12345
#define IP "127.0.0.1"

// Descripteur du socket serveur
int cliSocket;
// Le socket serveur ouvert ?
bool isSocketOpened = false;

//Déclaration des fonctions
bool socket_connect();


int main(int argc, char const *argv[])
{
    bool test = socket_connect();    
    return 0;
}

bool socket_connect()
{
    printf("Connecting to the server...\n");

    // Création du socket.
    cliSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(cliSocket == -1){
        printf("[-] Error in connection. \n");
        exit(1);
    }

    // Parametrage du socket
    struct sockaddr_in addCli;
    addCli.sin_addr.s_addr = inet_addr(IP);
    addCli.sin_family = AF_INET;
    addCli.sin_port = htons(PORT);
	
    // Connexion au serveur.
    if (connect(cliSocket, (struct sockaddr*)&addCli, sizeof(addCli)) == -1)
    {
        printf("Erreur de connexion au serveur");
        return false;
    }

    isSocketOpened = true;
    printf("Connected to the server.\n");

}