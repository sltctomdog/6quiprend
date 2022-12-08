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
}client;

typedef struct{
  client *lst;
  size_t size;
} clientArray;

int createSock();
clientArray* createClientArray(size_t max_client);
void acceptClient(int serverSock,clientArray *clientArr, size_t nb);
void *pthreadInitClient(void *ptrClient);
void freeClientArray(clientArray *in);
void shuffle(int *array, size_t length);
int** creerPiles(int* paquet);
void detruitPiles(int** m);
void affichePiles(int** mat);
void changeAllClientIOBlock(clientArray *in);
void changeAllClientIONonBlock(clientArray *in);
void envoyerPiles(int** mat, client client);
void envoyerMain(client client);
bool isCarteValid(client client,int choixCarte);
int indicePlusPetiteCarte(int* cartes, int nbCartes);
int jouerCarte(int** piles, int carte);
void retirerCarte(client *client, int carte);
int compterPointPile(int* pile);
void resetPile(int* pile, int carte);
int compterCartePile(int* pile);
void ajouterCartePile(int* pile, int carte);
void envoyerClassement(clientArray *in);
void distribuerCartes(client *c, int* paquet);
void initPiles(int** piles, int* paquet);
int* creerPaquet();
bool checkNewGame(clientArray *in);
void *pthreadAskClient(void *ptrClient);

int main(int argc, char const *argv[]){
    if(argc != 2){
    printf("Usage: %s nb_joueur(2-10) \n", argv[0]);
    return 1;
    }

    //init client
    size_t nb_client;
    nb_client = atol(argv[1]);
    int sock = createSock();
    clientArray *lstClient = NULL;
    char client_input[SIZE_INPUT_USER];
    char *ret_fgets = NULL;

    //Lancement du serveur + attente des clients
    printf("Nombre de clients : %lu\n", nb_client);
    printf("En attente des clients ...\n");
    lstClient =createClientArray(nb_client);
    acceptClient(sock, lstClient, nb_client);

    //Clients receptionnés
    for (size_t i = 0; i < lstClient->size; i++) {
        printf("Client %lu : %s\n",i,lstClient->lst[i].name);
    }

    int* paquet;
    int** piles;
    int* cartesTour;
    
    //game loop
    do{
        changeAllClientIONonBlock(lstClient);
            //init jeu
        paquet = creerPaquet();
        for (size_t i = 0; i < lstClient->size; i++) {
            distribuerCartes(&lstClient->lst[i], paquet);
        }
        piles = creerPiles(paquet);    
        cartesTour = (int*)calloc(lstClient->size, sizeof(int));
        //manche loop
        do{
            
            affichePiles(piles); //--------
            for (size_t i = 0; i < lstClient->size; i++) {
                envoyerPiles(piles, lstClient->lst[i]);
                fprintf(lstClient->lst[i].file_ptr, "Vous avez <%d> tetes de boeufs\n", lstClient->lst[i].teteBoeuf);
                envoyerMain(lstClient->lst[i]);
            }
            for (size_t i = 0; i < lstClient->size && ret_fgets==NULL; i++) {
                int choixCarte;
                do{
                    fprintf(lstClient->lst[i].file_ptr, "Saisir la carte à jouer : ");
                    changeAllClientIOBlock(lstClient);
                    ret_fgets= fgets(client_input, SIZE_INPUT_USER,lstClient->lst[i].file_ptr);
                    changeAllClientIONonBlock(lstClient);
                    choixCarte = atol(client_input);
                }while(choixCarte<1 || choixCarte>104 || !isCarteValid(lstClient->lst[i],choixCarte));
                ret_fgets=NULL;
                cartesTour[i]=choixCarte;          
            }
            ret_fgets=NULL;       

            for(int j=0;j<lstClient->size;j++){
                int iMin = indicePlusPetiteCarte(cartesTour, lstClient->size);
                lstClient->lst[iMin].teteBoeuf+=jouerCarte(piles, cartesTour[iMin]);
                retirerCarte(&lstClient->lst[iMin], cartesTour[iMin]);
                cartesTour[iMin]=0;
            }
            //anti saturation du cpu
            usleep(5000);
        }while(lstClient->lst->nbCarte>0);

        for(int k=0;k<lstClient->size;k++){
            fprintf(lstClient->lst[k].file_ptr, "Fin de la manche !\n");
        }

        detruitPiles(piles);
        free(paquet);
        free(cartesTour);
        changeAllClientIOBlock(lstClient);
        sleep(2);
    }while(checkNewGame(lstClient)==true);

    for(int k=0;k<lstClient->size;k++){
        fprintf(lstClient->lst[k].file_ptr, "Fin de la partie !\n");
    }
    envoyerClassement(lstClient);

    //Libération de l'espace mémoire
    freeClientArray(lstClient);
    close(sock);
}

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

clientArray* createClientArray(size_t max_client){
    clientArray* ret = malloc(sizeof(clientArray));
    if(ret==NULL)FATAL();
    ret->lst = malloc(sizeof(client)*max_client);
    if(ret->lst == NULL)FATAL();
    for(size_t i=0; i < max_client; i++){
        ret->lst[i].file_ptr=NULL;
        ret->lst[i].cartes =NULL;
    }
    ret->size = max_client;
    return ret;
}

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

void *pthreadInitClient(void *ptrClient){
    client *cli = (client *)ptrClient;
    fprintf(cli->file_ptr, "Entrez votre nom : ");
    fgets(cli->name,SIZE_NAME,cli->file_ptr);
    cli->name[strlen(cli->name)-1] = '\0';
    cli->cartes= NULL;
    cli->nbCarte = 0;
    cli->teteBoeuf = 0;
    pthread_exit (ptrClient);
}

void freeClientArray(clientArray *in){
    for(size_t i=0; i<in->size ; i++){
        fclose(in->lst[i].file_ptr);
        if(in->lst[i].cartes!=NULL){
        free(in->lst[i].cartes);
        }
    }
    free(in->lst);
    free(in);
    return;
}

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

void detruitPiles(int** m)
{
    for(int i=0; i<4; i++)
    {
        free(m[i]);
    }
    free(m);
}

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

void changeAllClientIOBlock(clientArray *in){
  int status;
  int fd;
  for(size_t i=0;i<in->size;i++){
    fd = fileno(in->lst[i].file_ptr);
    status = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & (~O_NONBLOCK));
    if(status==-1) FATAL();
  }
}

void changeAllClientIONonBlock(clientArray *in){
  int status;
  int fd;
  for(size_t i=0;i<in->size;i++){
    fd = fileno(in->lst[i].file_ptr);
    status = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    if(status==-1) FATAL();
  }
}

void envoyerPiles(int** mat, client client)
{
    for(int i=0; i<4; i++)
    {
        for(int j=0; j<6; j++)
        {
            fprintf(client.file_ptr, " %d ", mat[i][j]);
        }
        fprintf(client.file_ptr, "\n");
    }
}

void envoyerMain(client client){
    fprintf(client.file_ptr, "Vos cartes : ");
    for(int j=0;j<client.nbCarte;j++)
        fprintf(client.file_ptr, "%d ", client.cartes[j]);
    fprintf(client.file_ptr, "\n");
}

bool isCarteValid(client client,int choixCarte)
{
    bool res=false;
    for(int i=0; i<10; i++)
    {
        if(choixCarte==client.cartes[i])
            res=true;
    }
    return res;
}

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

int jouerCarte(int** piles, int carte){
    int indicePile = 10;
    int indiceCarte = 10;
    int valDiff = 1000;

    for(int i=0;i<4;i++){
        int posCarte = 0;
        for(int j=0;j<6;j++){
            //Recup la position de la derniere carte de la pile
            if(piles[i][j]!=0){            
                posCarte+=1;
            }
        }
        //Cherche la difference entre la dernire carte de la pile et la carte jouee la plus petite des 4 piles
        if(carte>piles[i][posCarte-1] && (carte-piles[i][posCarte-1]<valDiff)){
            valDiff = carte-piles[i][posCarte-1];
            indicePile = i;
            indiceCarte = posCarte - 1;
        }
    }

    //Si la carte est inférieur à tout -> remplace la pile la plus faible et retourne le nombre de têtes
    if(indiceCarte == 10 && indicePile==10){
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
    }

    //Si moins de 6 cartes dans la pile, ajoute la carte 
    if(compterCartePile(piles[indicePile])<5){
        ajouterCartePile(piles[indicePile], carte);
        return 0;
    }else{ // sinon compte les points de la pile, reset la pile (donc ajoute la carte), retourne le nombre de tête
        int pointsPile = compterPointPile(piles[indicePile]);
        resetPile(piles[indicePile], carte);
        return pointsPile;
    }
}

void retirerCarte(client *client, int carte){
    //Création d'un tableau recevant les cartes du joueur, sans celle qui vient d'être joué
    int* temp = (int*)calloc(client->nbCarte-1, sizeof(int));
    int cpt = 0;
    for(int i=0;i<client->nbCarte;i++){
        if(client->cartes[i]!=carte){
            temp[cpt]=client->cartes[i];
            cpt++;
        }
    }

    //Libération de la mémoire occupé par l'ancienne main du joueur
    free(client->cartes);
    client->cartes = temp;  
    client->nbCarte--;
}

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

void resetPile(int* pile, int carte){
    for(int i=0; i<6;i++){
        pile[i] = 0;
    }
    pile[0] = carte;
}

int compterCartePile(int* pile){
    int cpt=0;
    for(int i=0;i<6;i++){
        if(pile[i]!=0){
            cpt++;
        }
    }
    return cpt;
}

void ajouterCartePile(int* pile, int carte){
    int i = compterCartePile(pile);
    pile[i]=carte;
}

void envoyerClassement(clientArray *in){
    for(int i=0;i<in->size;i++){
        int indice = 1;
        for(int j=0;j<in->size;j++){
            if(in->lst[j].teteBoeuf<in->lst[i].teteBoeuf)
                indice++;
        }
        if(indice==1){
            fprintf(in->lst[i].file_ptr, "Vous finissez 1er de la partie !");
        }else{
            fprintf(in->lst[i].file_ptr, "Vous finissez %deme de la partie !", indice);
        }         
    }
}

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

int* creerPaquet(){
    int* ret = (int*)malloc(DECK_SIZE*sizeof(int));;
    for(int i=0; i<DECK_SIZE;i++)
        ret[i]=i+1;
    shuffle(ret,DECK_SIZE);
    return ret;
}

bool checkNewGame(clientArray *in){
    pthread_t *lst = malloc(sizeof(pthread_t)*in->size);
    int check;
    if(lst==NULL)FATAL();
    for(size_t i=0;i<in->size;i++){
      check=pthread_create(lst+i,NULL, pthreadAskClient, (void *)(in->lst+i));

      if(check!=0)FATAL();
    }
    char *ret;
    char bool_ret='y';
    for(size_t i=0; i<in->size;i++){
      pthread_join(lst[i], (void *)&ret);
      bool_ret = bool_ret & (*ret);
      free(ret);
    }
    free(lst);
    if(bool_ret=='y') return true;
    return false;
}

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