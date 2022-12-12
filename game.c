#include "utils.c"

void shuffle(int *array, size_t length);
int** creerPiles(int* paquet); 
void detruitPiles(int** m); 
void affichePiles(int** mat); 
bool isCarteValid(joueur *joueur,int choixCarte); 
int indicePlusPetiteCarte(int* cartes, int nbCartes); 
void retirerCarte(joueur *joueur, int carte); 
int compterPointPile(int* pile);
void resetPile(int* pile, int carte); 
int compterCartePile(int* pile); 
void ajouterCartePile(int* pile, int carte); 
void distribuerCartes(joueur *joueur, int* paquet);
void initPiles(int** piles, int* paquet); 
int* creerPaquet();

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
    printf("\n");
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