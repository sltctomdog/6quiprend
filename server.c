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
}joueur;

typedef struct{
  joueur *lst;
  size_t size;
  size_t nb_client;
} joueurArray;

int createSock();  //gestionServeur 
joueurArray* createjoueurArray(size_t max_joueur, size_t nb_bot); //gestionServeur 
void acceptClient(int serverSock,joueurArray *joueurArr, size_t nb); //gestionServeur 
void *pthreadInitClient(void *ptrClient); //gestionServeur 
void freejoueurArray(joueurArray *in); //gestionServeur 
void shuffle(int *array, size_t length); //gestionJeu
int** creerPiles(int* paquet); //gestionJeu
void detruitPiles(int** m); //gestionJeu
void affichePiles(int** mat); //gestionJeu
void envoyerPiles(int** mat, joueur *joueur); //gestionServeur 
void envoyerMain(joueur *joueur); //gestionServeur 
void demanderCartes(joueurArray *in, int* cartesTour); //gestionServeur 
void *pthreadDemanderCartes(void *ptrClient); //gestionServeur 
bool isCarteValid(joueur *joueur,int choixCarte); //gestionJeu 
int indicePlusPetiteCarte(int* cartes, int nbCartes); //gestionJeu
int jouerCarte(int** piles, int carte, joueur* joueur); //gestionJeu
void retirerCarte(joueur *joueur, int carte); //gestionJeu 
int compterPointPile(int* pile); //gestionJeu 
void resetPile(int* pile, int carte); //gestionJeu
int compterCartePile(int* pile); //gestionJeu
void ajouterCartePile(int* pile, int carte); //gestionJeu
void envoyerClassement(joueurArray *in); //gestionServeur  
void distribuerCartes(joueur *joueur, int* paquet); //gestionJeu
void initPiles(int** piles, int* paquet); //gestionJeu
int* creerPaquet(); //gestionJeu
bool checkNewGame(joueurArray *in); //gestionServeur 
void *pthreadAskClient(void *ptrClient); //gestionServeur 


int main(int argc, char const *argv[]){
    if(argc != 3){
    printf("Usage: %s nb_joueur nb_bot (total max 10) \n", argv[0]);
    return 1;
    }

    /* init joueurs */
    size_t nb_client;
    size_t nb_bot;
    nb_client = atol(argv[1]);
    nb_bot = atol(argv[2]);
    int sock = createSock();
    joueurArray *lstJoueur = NULL;
    char client_input[SIZE_INPUT_USER];
    char *ret_fgets = NULL;

    /* Lancement du serveur + attente des clients */ 
    printf("Nombre de clients : %lu\n", nb_client);
    printf("Nombre de bots : %lu\n", nb_bot);
    printf("En attente des clients ...\n");
    lstJoueur =createjoueurArray(nb_client+nb_bot, nb_client);
    acceptClient(sock, lstJoueur, nb_client);


    /* Clients receptionnés */
    for (size_t i = 0; i < lstJoueur->size; i++) {
        printf("Client %lu : %s\n",i,lstJoueur->lst[i].name);
    }

    /* init paquet, piles, cartesTour */
    int* paquet;
    int** piles;
    int* cartesTour;
    
    /* game loop */
    do{
        
        /* init manche */
        paquet = creerPaquet();
        for (size_t i = 0; i < lstJoueur->size; i++) {
            distribuerCartes(&lstJoueur->lst[i], paquet);
        }
        piles = creerPiles(paquet);    
        cartesTour = (int*)calloc(lstJoueur->size, sizeof(int));
        /* manche loop */
        do{    
            affichePiles(piles); 

            /* Envoie les piles, la main et le nombre de tête de boeufs à chaque client */
            for (size_t i = 0; i < lstJoueur->nb_client; i++) {
                    envoyerPiles(piles, &lstJoueur->lst[i]);
                    fprintf(lstJoueur->lst[i].file_ptr, "Vous avez <%d> tetes de boeufs\n", lstJoueur->lst[i].teteBoeuf);
                    envoyerMain(&lstJoueur->lst[i]);
            }

            /* Demande les cartes aux clients */
            demanderCartes(lstJoueur, cartesTour);

            /* Joue la carte des bots */
            for(int i=lstJoueur->nb_client;i<lstJoueur->size;i++){
                cartesTour[i]=lstJoueur->lst[i].cartes[0];
            }

            /* Joue la carte de chaque joueur dans le bon ordre */
            for(int j=0;j<lstJoueur->size;j++){
                int iMin = indicePlusPetiteCarte(cartesTour, lstJoueur->size);
                lstJoueur->lst[iMin].teteBoeuf+=jouerCarte(piles, cartesTour[iMin],&lstJoueur->lst[iMin]);
                retirerCarte(&lstJoueur->lst[iMin], cartesTour[iMin]);
                cartesTour[iMin]=0;
            }
           
            usleep(5000); //anti saturation du cpu
        }while(lstJoueur->lst->nbCarte>0); //Tant que les joueurs ont des cartes

        /* Signal la fin de la manche à tous les clients */
        for(int k=0;k<lstJoueur->nb_client;k++){
            fprintf(lstJoueur->lst[k].file_ptr, "Fin de la manche !\n");
        }

        /* Déallocation de la mémoire utilisée par les piles, le paquet et les cartesTour utilisée pendant cette manche */
        detruitPiles(piles);
        free(paquet);
        free(cartesTour);
        sleep(2);
    }while(checkNewGame(lstJoueur)==true); //Verifie si tous les clients veulent rejouer

    /* Signal la fin de la partie et leur classement à chacun des clients */
    for(int k=0;k<lstJoueur->nb_client;k++){
            fprintf(lstJoueur->lst[k].file_ptr, "Fin de la partie !\n");
    }
    
    envoyerClassement(lstJoueur);

    /* Déallocation de l'espace mémoire utilisé par la liste des joueurs */
    freejoueurArray(lstJoueur);
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

//! Crée la structure joueurArray dans le tas
/*!
  \param max_joueur nombre de joueur
  \param nb_client nombre de client
  \return pointeur sur la structure joueurArray
*/
joueurArray* createjoueurArray(size_t max_joueur, size_t nb_client){
    joueurArray* ret = malloc(sizeof(joueurArray));
    if(ret==NULL)FATAL();
    ret->lst = malloc(sizeof(joueur)*max_joueur);
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
  \param clientArr pointeur sur la structure joueurArray
  \param nb nombre de client
*/
void acceptClient(int serverSock,joueurArray *joueurArr, size_t nb){
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
        joueurArr->lst[i].file_ptr = fdopen(tmp,"a+");
        if(joueurArr->lst[i].file_ptr == NULL)FATAL();

        setvbuf(joueurArr->lst[i].file_ptr,NULL, _IONBF, 0);
        retpthread = pthread_create(pthread_t_lst +i, NULL, pthreadInitClient, (void *)(joueurArr->lst + i));
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
  \param ptrClient pointeur sur la structure joueur
  \return NULL defeat warning
*/
void *pthreadInitClient(void *ptrClient){
    joueur *cli = (joueur *)ptrClient;
    fprintf(cli->file_ptr, "Entrez votre nom : ");
    fgets(cli->name,SIZE_NAME,cli->file_ptr);
    cli->name[strlen(cli->name)-1] = '\0';
    cli->cartes= NULL;
    cli->nbCarte = 0;
    cli->teteBoeuf = 0;
    cli->isBot = false;
    pthread_exit (ptrClient);
}

//! Déallocation de la mémoire pour joueurArray
/*!
    \param in pointeur sur la structure joueurArray
*/
void freejoueurArray(joueurArray *in){
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
    \param joueur pointeur sur la structure joueur
*/
void envoyerPiles(int** mat, joueur *joueur)
{
    for(int i=0; i<4; i++)
    {
        for(int j=0; j<6; j++)
        {
            fprintf(joueur->file_ptr, " %d ", mat[i][j]);
        }
        fprintf(joueur->file_ptr, "\n");
    }
}

//! Envoie les cartes de leur mains aux clients
/*!
    \param joueur pointeur sur la structure joueur
*/
void envoyerMain(joueur *joueur){
    fprintf(joueur->file_ptr, "Vos cartes : ");
    for(int j=0;j<joueur->nbCarte;j++)
        fprintf(joueur->file_ptr, "%d ", joueur->cartes[j]);
    fprintf(joueur->file_ptr, "\n");
}

//! Verifie si le joueur possède la carte choisie
/*!
    \param joueur pointeur sur la structure joueur
    \param choixCarte carte à verifier
    \return true si le joueur possède la carte sinon false
*/
bool isCarteValid(joueur *joueur,int choixCarte)
{
    bool res=false;
    for(int i=0; i<10; i++)
    {
        if(choixCarte==joueur->cartes[i])
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


//! Fonction principale de jeu, ajoute la carte jouée à l'une des piles et retourne le nombre de tête de boeufs recuperées par le joueur
/*!
    \param piles pointeur sur le tableau contenant les pointeurs vers les piles 
    \param carte carte à jouer
    \param joueur le joueur qui joue 
    \return le nombre de tête de boeufs 
*/
int jouerCarte(int** piles, int carte, joueur* joueur){

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
        if(joueur->isBot){
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
            envoyerPiles(piles, joueur);
            do{
                fprintf(joueur->file_ptr, "Saisir la pile à enlever (1-4) : ");
                ret_fgets= fgets(client_input, SIZE_INPUT_USER,joueur->file_ptr);
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
    \param joueur pointeur sur la structure joueur
    \param carte carte a enlever de la main du joueur
*/
void retirerCarte(joueur *joueur, int carte){
    int* temp = (int*)calloc(joueur->nbCarte-1, sizeof(int));
    int cpt = 0;
    for(int i=0;i<joueur->nbCarte;i++){
        if(joueur->cartes[i]!=carte){
            temp[cpt]=joueur->cartes[i];
            cpt++;
        }
    }
    free(joueur->cartes);
    joueur->cartes = temp;  
    joueur->nbCarte--;
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

//! Envoie le classement aux clients
/*!
    \param in pointeur sur la structure joueurArray
*/
void envoyerClassement(joueurArray *in){
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

//! Distribue les cartes du paquet aux joueurs
/*!
    \param joueur pointeur sur la structure joueur
    \param paquet pointeur sur le paquet
*/
void distribuerCartes(joueur *joueur, int* paquet){
    if(joueur->cartes!=NULL)
        free(joueur->cartes);
    joueur->cartes = (int*)malloc(10*sizeof(int));
    joueur->nbCarte=10;
    int count=0;
    for(int j=0;j<DECK_SIZE;j++)
    {
        if(paquet[j]!=0 && count<10)
        {
            joueur->cartes[count]=paquet[j];
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


//! Demande aux clients pour une potentielle nouvelle partie
/*!
    \param in pointeur sur la structure joueurArray
    \return true si tous les clients veulent rejouer sinon false
*/
bool checkNewGame(joueurArray *in){
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
  \param ptrClient pointeur sur la structure joueur
  \return return pointeur sur le char
*/
void *pthreadAskClient(void *ptrClient){
  char *reponse = malloc(sizeof(char)*3);
  memset(reponse,0,3);
  joueur *cli=(joueur *)ptrClient;
  if(reponse==NULL) FATAL();
  do {
    fprintf(cli->file_ptr,"\e[1;1H\e[2JVoulez vous rejouer une partie ?(y/n): ");
    fgets(reponse,3,cli->file_ptr);
  } while(reponse[0]!='y' && reponse[0]!='n');
  pthread_exit((void *)reponse);
}

//! Demande les cartes à jouer aux clients
/*!
    \param in pointeur sur la structure joueurArray
    \param cartesTour pointeur sur le tableau contenant les cartes du tour
    \return true si tous les joueurs veulent rejouer sinon false
*/
void demanderCartes(joueurArray *in, int* cartesTour){
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
  \param ptrClient pointeur sur la structure joueur
  \return return pointeur sur la reponse
*/
void *pthreadDemanderCartes(void *ptrClient){
  char *reponse = malloc(sizeof(char)*SIZE_INPUT_USER);
  int rep;
  memset(reponse,0,SIZE_INPUT_USER);
  joueur *cli=(joueur *)ptrClient;
  if(reponse==NULL) FATAL();
  do {
    fprintf(cli->file_ptr,"Saisir la carte à jouer : ");
    fgets(reponse,SIZE_INPUT_USER,cli->file_ptr);
    rep = atol(reponse);
  } while(rep<1 || rep>104 || !isCarteValid(ptrClient,rep));
  pthread_exit((void *)reponse);
}