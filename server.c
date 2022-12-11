#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h> 
#include <time.h>   
#include <pthread.h>
#include <fcntl.h>
#include <stdbool.h>

#define FATAL(){fprintf(stderr,"%s\n",strerror(errno));exit(errno);}
#define PORT 4440
#define SIZE_INPUT_USER 5
#define SIZE_NAME 50
#define DECK_SIZE 104

typedef struct{
    char name[SIZE_NAME];
    int *cartes;
    FILE *file_ptr;
    size_t nbCarte;
    size_t teteBoeuf;
    bool isBot;
}client;

typedef struct{
  client *lst;
  size_t size;
  size_t nb_client;
} clientArray;

int createSock();  //gestionServeur 
clientArray* createClientArray(size_t max_joueur, size_t nb_bot); //gestionServeur 
void acceptClient(int serverSock,clientArray *clientArr, size_t nb); //gestionServeur 
void *pthreadInitClient(void *ptrClient); //gestionServeur 
void freeClientArray(clientArray *in); //gestionServeur 
void shuffle(int *array, size_t length); //gestionJeu
int** creerPiles(int* paquet); //gestionJeu
void detruitPiles(int** m); //gestionJeu
void affichePiles(int** mat); //gestionJeu
void envoyerPiles(int** mat, client *client); //gestionServeur 
void envoyerMain(client *client); //gestionServeur 
void demanderCartes(clientArray *in, int* cartesTour); //gestionServeur 
void *pthreadDemanderCartes(void *ptrClient); //gestionServeur 
bool isCarteValid(client *client,int choixCarte); //gestionJeu 
int indicePlusPetiteCarte(int* cartes, int nbCartes); //gestionJeu
int jouerCarte(int** piles, int carte, client* cli); //gestionJeu
void retirerCarte(client *client, int carte); //gestionJeu 
int compterPointPile(int* pile); //gestionJeu 
void resetPile(int* pile, int carte); //gestionJeu
int compterCartePile(int* pile); //gestionJeu
void ajouterCartePile(int* pile, int carte); //gestionJeu
void envoyerClassement(clientArray *in); //gestionServeur  
void distribuerCartes(client *c, int* paquet); //gestionJeu
void initPiles(int** piles, int* paquet); //gestionJeu
int* creerPaquet(); //gestionJeu
bool checkNewGame(clientArray *in); //gestionServeur 
void *pthreadAskClient(void *ptrClient); //gestionServeur 


int main(int argc, char const *argv[]){
    if(argc != 3){
    printf("Usage: %s nb_joueur nb_bot (total max 10) \n", argv[0]);
    return 1;
    }

    /* init client */
    size_t nb_client;
    size_t nb_bot;
    nb_client = atol(argv[1]);
    nb_bot = atol(argv[2]);
    int sock = createSock();
    clientArray *lstClient = NULL;
    char client_input[SIZE_INPUT_USER];
    char *ret_fgets = NULL;

    /* Lancement du serveur + attente des clients */ 
    printf("Nombre de clients : %lu\n", nb_client);
    printf("Nombre de bots : %lu\n", nb_bot);
    printf("En attente des clients ...\n");
    lstClient =createClientArray(nb_client+nb_bot, nb_client);
    acceptClient(sock, lstClient, nb_client);


    /* Clients receptionnés */
    for (size_t i = 0; i < lstClient->size; i++) {
        printf("Client %lu : %s\n",i,lstClient->lst[i].name);
    }

    /* init paquet, piles, cartesTour */
    int* paquet;
    int** piles;
    int* cartesTour;
    
    /* game loop */
    do{
        
        /* init manche */
        paquet = creerPaquet();
        for (size_t i = 0; i < lstClient->size; i++) {
            distribuerCartes(&lstClient->lst[i], paquet);
        }
        piles = creerPiles(paquet);    
        cartesTour = (int*)calloc(lstClient->size, sizeof(int));
        /* manche loop */
        do{    
            affichePiles(piles); 

            /* Envoie les piles, la main et le nombre de tête de boeufs à chaque client */
            for (size_t i = 0; i < lstClient->nb_client; i++) {
                    envoyerPiles(piles, &lstClient->lst[i]);
                    fprintf(lstClient->lst[i].file_ptr, "Vous avez <%d> tetes de boeufs\n", lstClient->lst[i].teteBoeuf);
                    envoyerMain(&lstClient->lst[i]);
            }

            /* Demande les cartes aux clients */
            demanderCartes(lstClient, cartesTour);

            /* Joue la carte des bots */
            for(int i=lstClient->nb_client;i<lstClient->size;i++){
                cartesTour[i]=lstClient->lst[i].cartes[0];
            }

            /* Joue la carte de chaque joueur dans le bon ordre */
            for(int j=0;j<lstClient->size;j++){
                int iMin = indicePlusPetiteCarte(cartesTour, lstClient->size);
                lstClient->lst[iMin].teteBoeuf+=jouerCarte(piles, cartesTour[iMin],&lstClient->lst[iMin]);
                retirerCarte(&lstClient->lst[iMin], cartesTour[iMin]);
                cartesTour[iMin]=0;
            }
           
            usleep(5000); //anti saturation du cpu
        }while(lstClient->lst->nbCarte>0); //Tant que les joueurs ont des cartes

        /* Signal la fin de la manche à tous les clients */
        for(int k=0;k<lstClient->nb_client;k++){
            fprintf(lstClient->lst[k].file_ptr, "Fin de la manche !\n");
        }

        /* Déallocation de la mémoire utilisée par les piles, le paquet et les cartesTour utilisée pendant cette manche */
        detruitPiles(piles);
        free(paquet);
        free(cartesTour);
        sleep(2);
    }while(checkNewGame(lstClient)==true); //Verifie si tous les clients veulent rejouer

    /* Signal la fin de la partie et leur classement à chacun des clients */
    for(int k=0;k<lstClient->nb_client;k++){
            fprintf(lstClient->lst[k].file_ptr, "Fin de la partie !\n");
    }
    
    envoyerClassement(lstClient);

    /* Déallocation de l'espace mémoire utilisé par la liste des joueurs */
    freeClientArray(lstClient);
    close(sock);
}

//! Crée un serveur et renvois le filedescriptor
int createSock(){
    int sock = socket(AF_INET, SOCK_STREAM,0);
    if(sock == -1){
        fprintf(stderr, "%s\n", strerror(errno));
        exit(errno);
    }
    struct sockaddr_in sin;
    socklen_t recsize = sizeof(sin);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);
    if (bind(sock, (struct sockaddr*)&sin, recsize) == -1) {
        fprintf(stderr,"%s\n", strerror(errno));
        exit(errno);
    }
    if (listen(sock, 10) == -1) {
        fprintf(stderr,"%s\n", strerror(errno));
        exit(errno);
    }
    return sock;
}

//! Crée la structure clientArray dans le tas
/*!
  \param max_joueur nombre max de joueur
  \param nb_bot nombre de client
  \return pointeur sur la structure clientArray
*/
clientArray* createClientArray(size_t max_joueur, size_t nb_client){
    clientArray* ret = malloc(sizeof(clientArray));
    if(ret==NULL)FATAL();
    ret->lst = malloc(sizeof(client)*max_joueur);
    if(ret->lst == NULL)FATAL();
    for(size_t i=0; i < max_joueur; i++){
        ret->lst[i].file_ptr=NULL;
        ret->lst[i].cartes =NULL;
    }
    for(int i=nb_client;i<max_joueur;i++){
        ret->lst[i].isBot = true;
        strcpy(ret->lst[i].name, "BobLeBot");
    }
    ret->size = max_joueur;
    ret->nb_client=nb_client;
    return ret;
}

//! Accepte la connexion des clients 
/*!
  \param serverSock serveur
  \param clientArr pointeur sur la structure clientArray
  \param nb nombre de client
*/
void acceptClient(int serverSock,clientArray *clientArr, size_t nb){
    struct sockaddr_in sin;
    socklen_t csize = sizeof(sin);
    int tmp;
    int retpthread;
    char *ip;
    pthread_t *pthread_t_lst = malloc(sizeof(pthread_t)*nb);
    if(pthread_t_lst == NULL)FATAL();
    for(size_t i=0; i<nb;i++){
        tmp = accept(serverSock, (struct sockaddr *)&sin, &csize);
        if(tmp==-1)FATAL();
        ip = inet_ntoa(sin.sin_addr);
        printf("Incoming connection [%s]\n",ip);
        clientArr->lst[i].file_ptr = fdopen(tmp,"a+");
        if(clientArr->lst[i].file_ptr == NULL)FATAL();

        setvbuf(clientArr->lst[i].file_ptr,NULL, _IONBF, 0);
        retpthread = pthread_create(pthread_t_lst +i, NULL, pthreadInitClient, (void *)(clientArr->lst + i));
        if(retpthread!=0)FATAL();
    }
    //wait all thread
    for(size_t i=0; i < nb; i++){
        retpthread = pthread_join(pthread_t_lst[i],NULL);
        if(retpthread!=0)FATAL();
    }
    free(pthread_t_lst);
    return;
}

//! thread initialisation client
/*!
  \param ptrClient pointeur sur la strucutre client
  \return NULL defeat warning
*/
void *pthreadInitClient(void *ptrClient){
    client *cli = (client *)ptrClient;
    fprintf(cli->file_ptr, "Entrez votre nom : ");
    fgets(cli->name,SIZE_NAME,cli->file_ptr);
    cli->name[strlen(cli->name)-1] = '\0';
    cli->cartes= NULL;
    cli->nbCarte = 0;
    cli->teteBoeuf = 0;
    cli->isBot = false;
    pthread_exit (ptrClient);
}

//! Déallocation de la mémoire pour clientArray
/*!
    \param in pointeur sur la structure clientArray
*/
void freeClientArray(clientArray *in){
    for(size_t i=0; i<in->size ; i++){       
        if(in->lst[i].cartes!=NULL){
            free(in->lst[i].cartes);
        }
    }
    for(int i=0;i<in->nb_client;i++){
        fclose(in->lst[i].file_ptr);
    }
    free(in->lst);
    free(in);
    return;
}

//! Mélange du paquet 
/*!
    \param array pointeur sur le tableau representant le paquet 
    \param lenght taille du paquet
*/
void shuffle(int *array, size_t length)
{
    int i,j,temp;

    srand(time(NULL));

    for(i = 0; i < DECK_SIZE; i++)
    {
        j = (rand()%DECK_SIZE-1)+1;
        temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

//! Création des 4 piles de jeu
/*!
    \param paquet pointeur sur le tableau representant le paquet
    \return pointeur sur le tableau contenant les pointeurs vers les piles 
*/
int** creerPiles(int* paquet)
{
    int** mat = (int**)malloc(4*sizeof(int*));
    for(int i=0; i<4; i++)
    {
        mat[i] = (int*)calloc(6, sizeof(int));
    }
    initPiles(mat, paquet);
    return mat;
}

//! Déallocation de la mémoire pour les piles
/*!
    \param m pointeur sur le tableau contenant les pointeurs vers les piles 
*/
void detruitPiles(int** m)
{
    for(int i=0; i<4; i++)
    {
        free(m[i]);
    }
    free(m);
}

//! Affichage des piles 
/*!
    \param mat pointeur sur le tableau contenant les pointeurs vers les piles 
*/
void affichePiles(int** mat)
{
    for(int i=0; i<4; i++)
    {
        for(int j=0; j<6; j++)
        {
            printf(" %d", mat[i][j]);
        }
        printf("\n");
    }
}

//! Envoie les cartes des piles au clients 
/*!
    \param mat pointeur sur le tableau contenant les pointeurs vers les piles 
    \param client pointeur sur la structure client
*/
void envoyerPiles(int** mat, client *client)
{
    for(int i=0; i<4; i++)
    {
        for(int j=0; j<6; j++)
        {
            fprintf(client->file_ptr, " %d ", mat[i][j]);
        }
        fprintf(client->file_ptr, "\n");
    }
}

//! Envoie les cartes de sa main au client
/*!
    \param client pointeur sur la structure client
*/
void envoyerMain(client *client){
    fprintf(client->file_ptr, "Vos cartes : ");
    for(int j=0;j<client->nbCarte;j++)
        fprintf(client->file_ptr, "%d ", client->cartes[j]);
    fprintf(client->file_ptr, "\n");
}

//! Verifie si le client possède la carte choisie
/*!
    \param client pointeur sur la structure client
    \param choixCarte carte à verifier
    \return true si le client possède la carte sinon false
*/
bool isCarteValid(client *client,int choixCarte)
{
    bool res=false;
    for(int i=0; i<10; i++)
    {
        if(choixCarte==client->cartes[i])
            res=true;
    }
    return res;
}

//! Indice de la plus petite carte jouée
/*!
    \param cartes pointeur sur le tableau des cartes jouées 
    \param nbCartes nombre de cartes jouées
    \return indice de la plus petite carte dans le tableau 
*/
int indicePlusPetiteCarte(int* cartes, int nbCartes){
    int indice = 0;
    int valeur = 1000;
    for(int i=0;i<nbCartes;i++){
        if(cartes[i]<valeur && cartes[i]!=0){
            indice=i;
            valeur=cartes[i];
        }           
    }
    return indice;
}


//! Fonction principale de jeu, ajoute la carte joué à l'une des piles et retourne le nombre de tête de boeufs recuperées par le joueur
/*!
    \param piles pointeur sur le tableau contenant les pointeurs vers les piles 
    \param carte carte à jouer
    \param cli le client qui joue 
    \return le nombre de tête de boeufs 
*/
int jouerCarte(int** piles, int carte, client* cli){

    char *ret_fgets = NULL;
    char client_input[SIZE_INPUT_USER];

    int indicePile = 10;
    int indiceCarte = 10;
    int valDiff = 1000;

    for(int i=0;i<4;i++){
        int posCarte = 0;
        for(int j=0;j<6;j++){
            /* Recup la position de la derniere carte de la pile */
            if(piles[i][j]!=0){            
                posCarte+=1;
            }
        }
        /* Cherche la difference entre la dernire carte de la pile et la carte jouee la plus petite des 4 piles */
        if(carte>piles[i][posCarte-1] && (carte-piles[i][posCarte-1]<valDiff)){
            valDiff = carte-piles[i][posCarte-1];
            indicePile = i;
            indiceCarte = posCarte - 1;
        }
    }

    /* Si la carte est inférieur à tout -> demande la pile à remplacer au client/remplace la pile avec le moins de tête pour le bot et retourne le nombre de têtes */
    if(indiceCarte == 10 && indicePile==10){
        if(cli->isBot){
            int pointsPile = compterPointPile(piles[0]);
            int indicePile = 0;
            for(int i=1;i<4;i++){
                int points = compterPointPile(piles[i]);
                if(pointsPile>points){
                    pointsPile = points;
                    indicePile = i;
                }
            }
            resetPile(piles[indicePile], carte);
            return pointsPile;
        }else{
            int choixPile;
            envoyerPiles(piles, cli);
            do{
                fprintf(cli->file_ptr, "Saisir la pile à enlever (1-4) : ");
                ret_fgets= fgets(client_input, SIZE_INPUT_USER,cli->file_ptr);
                choixPile = atol(client_input);
            }while(choixPile<1 || choixPile>4);
            ret_fgets = NULL;
            int pointsPile = compterPointPile(piles[choixPile-1]);
            resetPile(piles[choixPile-1], carte);
            return pointsPile;
        }    
    }

    /* Si moins de 6 cartes dans la pile, ajoute la carte  */
    if(compterCartePile(piles[indicePile])<5){
        ajouterCartePile(piles[indicePile], carte);
        return 0;
    }else{ // sinon compte les points de la pile, reset la pile (donc ajoute la carte), retourne le nombre de tête
        int pointsPile = compterPointPile(piles[indicePile]);
        resetPile(piles[indicePile], carte);
        return pointsPile;
    }
}

//! Retire la carte de la main du client
/*!
    \param client pointeur sur la structure client
    \param carte carte a enlever de la main du client
*/
void retirerCarte(client *client, int carte){
    int* temp = (int*)calloc(client->nbCarte-1, sizeof(int));
    int cpt = 0;
    for(int i=0;i<client->nbCarte;i++){
        if(client->cartes[i]!=carte){
            temp[cpt]=client->cartes[i];
            cpt++;
        }
    }
    free(client->cartes);
    client->cartes = temp;  
    client->nbCarte--;
}

//! Compte les points dans une pile
/*!
    \param pile pointeur sur une pile 
    \return le nombre de tête de boeufs dans la pile 
*/
int compterPointPile(int* pile){
    int sommePoints = 0;
    for(int i=0;i<6;i++){
        if(pile[i]!=0){
            if(pile[i]==55){
                sommePoints+=7;
            }else if(pile[i]%10==0){
                sommePoints+=3;
            }else if(pile[i]%5==0){
                sommePoints+=2;
            }else if(pile[i]%11==0){
                sommePoints+=5;
            }else{
                sommePoints+=1;
            }
        }
    }
    return sommePoints;
}

//! Enleve les cartes d'une pile et remplace la première carte par celle choisie 
/*!
    \param pile pointeur sur une pile
    \param carte la carte à mettre au début de la pile
*/
void resetPile(int* pile, int carte){
    for(int i=0; i<6;i++){
        pile[i] = 0;
    }
    pile[0] = carte;
}

//! Compte le nombre da carte dans une pile
/*!
    \param pile pointeur sur une pile
*/
int compterCartePile(int* pile){
    int cpt=0;
    for(int i=0;i<6;i++){
        if(pile[i]!=0){
            cpt++;
        }
    }
    return cpt;
}

//! Ajoute une carte à une pile
/*!
    \param pile pointeur vers une pile
    \param carte la carte à ajouter à la pile 
*/
void ajouterCartePile(int* pile, int carte){
    int i = compterCartePile(pile);
    pile[i]=carte;
}

//! Envoie le classement aux joueurs 
/*!
    \param in pointeur sur la structure clientArray
*/
void envoyerClassement(clientArray *in){
    for(int i=0;i<in->size;i++){
        int indice = 1;
        for(int j=0;j<in->size;j++){
            if(in->lst[j].teteBoeuf<in->lst[i].teteBoeuf)
                indice++;
        }
        if(!in->lst[i].isBot){
            if(indice==1){
            fprintf(in->lst[i].file_ptr, "Vous finissez 1er de la partie !");
        }else{
            fprintf(in->lst[i].file_ptr, "Vous finissez %deme de la partie !", indice);
        }      
        }        
    }
}

//! Distribue les cartes du paquet aux clients
/*!
    \param c pointeur sur la structure client
    \param paquet pointeur sur le paquet
*/
void distribuerCartes(client *c, int* paquet){
    if(c->cartes!=NULL)
        free(c->cartes);
    c->cartes = (int*)malloc(10*sizeof(int));
    c->nbCarte=10;
    int count=0;
    for(int j=0;j<DECK_SIZE;j++)
    {
        if(paquet[j]!=0 && count<10)
        {
            c->cartes[count]=paquet[j];
            paquet[j]=0;
            count++;
        }
    }
}


//! Initialise les piles
/*!
    \param piles pointeur sur le tableau contenant les pointeurs vers les piles 
    \param paquet pointeur sur le paquet
*/
void initPiles(int** piles, int* paquet){
    int count=0;
    for(int i=0; i<DECK_SIZE; i++)
    {
        if(paquet[i]!=0 && count<4)
        {
            piles[count][0]=paquet[i];
            paquet[i]=0;
            count++;
        }
    }
}

//! Créer le paquet de carte 
/*!
    \return pointeur sur le paquet 
*/
int* creerPaquet(){
    int* ret = (int*)malloc(DECK_SIZE*sizeof(int));;
    for(int i=0; i<DECK_SIZE;i++)
        ret[i]=i+1;
    shuffle(ret,DECK_SIZE);
    return ret;
}


//! Demande aux joueurs pour une potentielle nouvelle partie
/*!
    \param in pointeur sur la structure clientArray
    \return true si tous les joueurs veulent rejouer sinon false
*/
bool checkNewGame(clientArray *in){
    pthread_t *lst = malloc(sizeof(pthread_t)*in->nb_client);
    int check;
    if(lst==NULL)FATAL();
    for(size_t i=0;i<in->nb_client;i++){
      check=pthread_create(lst+i,NULL, pthreadAskClient, (void *)(in->lst+i));

      if(check!=0)FATAL();
    }
    char *ret;
    char bool_ret='y';
    for(size_t i=0; i<in->nb_client;i++){
      pthread_join(lst[i], (void *)&ret);
      bool_ret = bool_ret & (*ret);
      free(ret);
    }
    free(lst);
    if(bool_ret=='y') return true;
    return false;
}

//! thread demande valeur y ou n aux client
/*!
  \param ptrClient pointeur sur la structure client
  \return return pointeur sur le char
*/
void *pthreadAskClient(void *ptrClient){
  char *reponse = malloc(sizeof(char)*3);
  memset(reponse,0,3);
  client *cli=(client *)ptrClient;
  if(reponse==NULL) FATAL();
  do {
    fprintf(cli->file_ptr,"\e[1;1H\e[2JVoulez vous rejouer une partie ?(y/n): ");
    fgets(reponse,3,cli->file_ptr);
  } while(reponse[0]!='y' && reponse[0]!='n');
  pthread_exit((void *)reponse);
}

//! Demande les cartes à jouer aux joueurs
/*!
    \param in pointeur sur la structure clientArray
    \param cartesTour pointeur sur le tableau contenant les cartes du tour
    \return true si tous les joueurs veulent rejouer sinon false
*/
void demanderCartes(clientArray *in, int* cartesTour){
    pthread_t *lst = malloc(sizeof(pthread_t)*in->nb_client);
    int check;
    if(lst==NULL)FATAL();
    for(size_t i=0;i<in->nb_client;i++){
      check=pthread_create(lst+i,NULL, pthreadDemanderCartes, (void *)(in->lst+i));

      if(check!=0)FATAL();
    }

    char *ret;
    int choixCarte;
    for(size_t i=0; i<in->nb_client;i++){
        pthread_join(lst[i], (void *)&ret);
        choixCarte = atol(ret);
        cartesTour[i]=choixCarte;
        free(ret);
    }
    free(lst);
}

//! thread demande la carte à jouer au client
/*!
  \param ptrClient pointeur sur la structure client
  \return return pointeur sur la reponse
*/
void *pthreadDemanderCartes(void *ptrClient){
  char *reponse = malloc(sizeof(char)*SIZE_INPUT_USER);
  int rep;
  memset(reponse,0,SIZE_INPUT_USER);
  client *cli=(client *)ptrClient;
  if(reponse==NULL) FATAL();
  do {
    fprintf(cli->file_ptr,"Saisir la carte à jouer : ");
    fgets(reponse,SIZE_INPUT_USER,cli->file_ptr);
    rep = atol(reponse);
  } while(rep<1 || rep>104 || !isCarteValid(ptrClient,rep));
  pthread_exit((void *)reponse);
}