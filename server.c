#include "server.h"


int main(int argc, char const *argv[]){
    if(argc != 3){
    printf("Usage: %s nb_joueur nb_bot (total max 10) \n", argv[0]);
    return 1;
    }

    /* CREATION PDF */
    const char* page_title = "6quiprend";
    HPDF_Doc pdf = HPDF_New(error_handler, NULL);
    HPDF_Font font = HPDF_GetFont(pdf, "Helvetica", NULL);
    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetHeight(page, 14400);
    HPDF_REAL width = HPDF_Page_GetWidth(page);
    HPDF_REAL tw;

    /* GESTION D'ERREUR */
    if (!pdf) {
        printf ("error: cannot create PdfDoc object\n");
        FATAL();
    }
    if (setjmp(env)) {
        HPDF_Free (pdf);
        FATAL();
    }


    /* Affiche le titre */
    HPDF_Page_SetFontAndSize (page, font, 24);
    tw = HPDF_Page_TextWidth (page, page_title);
    HPDF_Page_BeginText (page);
    HPDF_Page_TextOut (page, (width - tw) / 2, 14400 - 50, page_title);
    HPDF_Page_EndText (page);
    HPDF_Page_SetFontAndSize (page, font, 10);
    HPDF_Page_BeginText(page);
    HPDF_Page_MoveTextPos (page, 60, 14400 - 80);

    /* INIT JOUEURS */
    size_t nb_client;
    size_t nb_bot;
    nb_client = atol(argv[1]);
    nb_bot = atol(argv[2]);
    int sock = createSock();
    joueurArray *lstJoueur = NULL;
    char client_input[SIZE_INPUT_USER];
    char *ret_fgets = NULL;
    int mancheCount=1;

    /* Lancement du serveur + attente des clients */ 
    printf("\033[92;01mNombre de clients : %lu\n", nb_client);
    printf("Nombre de bots : %lu\n", nb_bot);
    printf("En attente des clients ...\n");
    lstJoueur = createjoueurArray(nb_client+nb_bot, nb_client);
    acceptClient(sock, lstJoueur, nb_client);
    
    /* Clients receptionnés et écrit les clients dans le pdf */    
    HPDF_Page_ShowText(page, "Liste Joueurs : ");
    for (size_t i = 0; i < lstJoueur->size; i++) {
        printf("Client %lu : %s\n",i,lstJoueur->lst[i].name);
        HPDF_Page_ShowText(page, lstJoueur->lst[i].name);
        HPDF_Page_ShowText(page, " ");
    }
    printf("\033[92;00m");

    /* init paquet, piles, cartesTour */
    int* paquet;
    int** piles;
    int* cartesTour;
    int* cartesTourPrec;
    
    /* game loop */
    do{
        
        /* init manche */
        paquet = creerPaquet();
        for (size_t i = 0; i < lstJoueur->size; i++) {
            distribuerCartes(&lstJoueur->lst[i], paquet);
        }
        piles = creerPiles(paquet);    
        cartesTour = (int*)calloc(lstJoueur->size, sizeof(int));
        cartesTourPrec = (int*)calloc(lstJoueur->size, sizeof(int));

        printf("\033[91;01mManche %d \033[91;00m\n",mancheCount);

        /* PDF THINGS*/
        HPDF_Page_MoveTextPos(page, 0, -15);
        HPDF_Page_ShowText(page, "Manche");     
        char mancheCountStr[2];
        sprintf(mancheCountStr,"%d",mancheCount);  
        HPDF_Page_ShowText(page, mancheCountStr);   
        HPDF_Page_MoveTextPos(page, 0, -15);

        /* manche loop */
        do{    
            /* Affiche dans le serveur et écrit dans le pdf la pile de la manche en cours*/
            affichePiles(page,piles);

            /* Envoie les piles, la main et le nombre de tête de boeufs à chaque client */
            for (size_t i = 0; i < lstJoueur->nb_client; i++) {
                    envoyerPiles(piles, &lstJoueur->lst[i]);
                    if(cartesTourPrec[0]!=0){
	                    envoyerCartesJouees(&lstJoueur->lst[i], lstJoueur, cartesTourPrec, i);
                    }
                    fprintf(lstJoueur->lst[i].file_ptr, "\033[91;01mVous avez <%d> tetes de boeufs\033[91;01m\n", lstJoueur->lst[i].teteBoeuf);
                    envoyerMain(&lstJoueur->lst[i]);
            }

            /* Demande les cartes aux clients */
            demanderCartes(lstJoueur, cartesTour);

            /* Joue la carte des bots */
            for(int i=lstJoueur->nb_client;i<lstJoueur->size;i++){
                cartesTour[i]=lstJoueur->lst[i].cartes[0];
            }

            /* Affiche dans le serveur et ecrit dans le pdf les cartes jouées par chaque joueur */
            if(cartesTourPrec[0]!=0){          
                afficheCartesJouees(page, lstJoueur, cartesTourPrec);
            }else{
                HPDF_Page_ShowText(page, "------------------------");
                HPDF_Page_MoveTextPos(page, 0, -15);
                printf("\n");
            }

            /* Stock les cartes jouées pour pouvoir les afficher aux clients après le round*/
            for(int j=0;j<lstJoueur->size;j++){
                cartesTourPrec[j]=cartesTour[j];
            };

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
        mancheCount++;
        detruitPiles(piles);
        free(paquet);
        free(cartesTour);
        free(cartesTourPrec);
        sleep(2);

        HPDF_Page_MoveTextPos(page, 0, -15);      
        HPDF_Page_ShowText(page, "--------------------------------------------------------------------------------------------------------------------------------------");
        HPDF_Page_MoveTextPos(page, 0, -15);      
    }while(checkNewGame(lstJoueur)==true); //Verifie si tous les clients veulent rejouer

    /* Signal la fin de la partie et leur classement à chacun des clients */
    for(int k=0;k<lstJoueur->nb_client;k++){
            fprintf(lstJoueur->lst[k].file_ptr, "Fin de la partie !\n");
    }
    
    envoyerClassement(page, lstJoueur);

    //page=HPDF_AddPage(pdf);
    HPDF_Page_EndText(page);
    savePdf(pdf);

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

    // Autoriser la réutilisation du port pour pouvoir immédiatement réouvrir un serveur,
	// quand un vient d'être fermé.
	int reuseAddr = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(int));

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
    char c[10] = "123456789";
    int indice = 0;
    for(int i=nb_client;i<max_joueur;i++){
        ret->lst[i].isBot = true;
        char* nom = (char*)malloc(10*sizeof(char));
        memset(nom, 0, 10);
        strcpy(nom, "BobLeBot");
        strcat(nom, &c[indice]);
        nom[9]='\0';
        strcpy(ret->lst[i].name, nom);
        indice++;
        free(nom);
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

//! Envoie les cartes des piles au clients 
/*!
    \param mat pointeur sur le tableau contenant les pointeurs vers les piles 
    \param joueur pointeur sur la structure joueur
*/
void envoyerPiles(int** mat, joueur *joueur)
{
    fprintf(joueur->file_ptr, "\033[93;01m");

    for(int i=0; i<4; i++)
    {
        for(int j=0; j<6; j++)
        {
            fprintf(joueur->file_ptr, " %d ", mat[i][j]);
        }
        fprintf(joueur->file_ptr, "\n");
    }
    fprintf(joueur->file_ptr, "\033[93;00m");
}

//! Envoie les cartes de leur mains aux clients
/*!
    \param joueur pointeur sur la structure joueur
*/
void envoyerMain(joueur *joueur){
    fprintf(joueur->file_ptr, "\033[94;01mVos cartes : ");
    for(int j=0;j<joueur->nbCarte;j++)
        fprintf(joueur->file_ptr, "%d ", joueur->cartes[j]);
    fprintf(joueur->file_ptr, "\033[94;00m\n");
}

//! Envoie les cartes jouées au tour précédent 
/*!
    \param joueur pointeur sur la structure joueur
    \param lstJoueur pointeur sur la structure lstJoueur
    \param cartesTourPrec les cartes du tour précédent
    \param indice indice du joueur dans la lstJoueur
*/
void envoyerCartesJouees(joueur* joueur, joueurArray* lstJoueur, int* cartesTourPrec, int indice){
    for(int i=0;i<lstJoueur->size;i++){
        if(i!=indice){
            fprintf(joueur->file_ptr,"\033[35;01m-%s a joué la carte %d\033[35;00m\n", lstJoueur->lst[i].name, cartesTourPrec[i]);
        }
    }
}

//! Envoie le classement aux clients
/*!
    \param in pointeur sur la structure joueurArray
*/
void envoyerClassement(HPDF_Page page, joueurArray *in){


    HPDF_Page_MoveTextPos(page, 0,-15);      
    HPDF_Page_ShowText(page, "Resultats  : ");

    for(int i=0;i<in->size;i++){
        int indice = 1;
        for(int j=0;j<in->size;j++){
            if(in->lst[j].teteBoeuf<in->lst[i].teteBoeuf)
                indice++;
        }
        if(!in->lst[i].isBot){
            if(indice==1){
            fprintf(in->lst[i].file_ptr, "\033[93;01mVous finissez 1er de la partie !\033[93;00m");
        }else{
            fprintf(in->lst[i].file_ptr, "\033[93;01mVous finissez %deme de la partie !\033[93;00m", indice);
        }      
        }

        char si[2];
        sprintf(si,"%d",indice);
        HPDF_Page_MoveTextPos(page, 0, -15);    
        HPDF_Page_ShowText(page, si);
        HPDF_Page_ShowText(page, ". ");
        HPDF_Page_ShowText(page, in->lst[i].name);
        HPDF_Page_ShowText(page, " | ");
        sprintf(si,"%d",in->lst[i].teteBoeuf);
        HPDF_Page_ShowText(page, si);
    }
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

//! Affichage des piles et ecriture dans le pdf
/*!
    \param page page du pdf
    \param mat pointeur sur le tableau contenant les pointeurs vers les piles 
*/
void affichePiles(HPDF_Page page, int** mat)
{
    printf("\033[93;01m");

    for(int i=0; i<4; i++)
    {
        
        char ligne[50];
        memset(ligne, 0, 50);
        for(int j=0; j<6; j++)
        {
            char snum[5];
            sprintf(snum,"%d",mat[i][j]);
            strcat(ligne,snum);
            strcat(ligne," ");
        }
        ligne[strlen(ligne)]='\0';
        addLinePdf(page, ligne);    
        printf("%s\n", ligne);
    }
    printf("\033[93;00m");

    
}

//! Affiche et ecrit les cartes jouées 
/*!
    \param page page du pdf
    \param lstJoueur pointeur sur la structure lstJoueur
    \param cartesTourPrec les cartes du tour prec
*/
void afficheCartesJouees(HPDF_Page page, joueurArray* lstJoueur, int* cartesTourPrec){
    printf("\033[95;01m");
    
    for(int i=0;i<lstJoueur->size;i++){
        char ligne[124];
        memset(ligne, 0, 124);
        strcpy(ligne, "-");
        strcat(ligne, lstJoueur->lst[i].name);
        strcat(ligne, " a joue la carte ");
        char snum[5];
        sprintf(snum,"%d",cartesTourPrec[i]);
        strcat(ligne,snum);
        ligne[strlen(ligne)]='\0';
        addLinePdf(page, ligne);
       printf("%s\n", ligne);
    }
    HPDF_Page_ShowText(page, "------------------------");
    HPDF_Page_MoveTextPos(page, 0, -15);
    printf("\033[95;00m\n");
}

void addLinePdf(HPDF_Page page, char* str)
{
    HPDF_Page_ShowText(page, str);
    HPDF_Page_MoveTextPos(page, 0, -15);
}

void savePdf(HPDF_Doc pdf)
{
    char filename[25];
    strcpy (filename, "History.pdf");
    HPDF_SaveToFile (pdf, filename);

    /* clean up */
    HPDF_Free (pdf);
}

