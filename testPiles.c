#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define DECK_SIZE 104

typedef struct Joueur
    {
        int nbCarte;
        int* cartes;
        int teteBoeuf;
    }Joueur;

int** creerPiles();
void affichePiles(int** mat);
void detruitPiles(int** m);
int jouerCarte(int** piles, int carte);
int compterPointPile(int* pile);
void resetPile(int* pile, int carte);
int compterCartePile(int* pile);
void ajouterCartePile(int* pile, int carte);
void shuffle(int* array, size_t length);
void afficherDeck(int* array);
bool isCarteValid(Joueur* joueur,int choixCarte);
void retirerCarte(Joueur* joueur, int carte);

int main(){
/*                                                          Initilalisation                                               */
    int cartesTotal[DECK_SIZE];
    int nbJoueur=2;
    int choixCarte;
    //printf("Nombre Joueur : ");
    //scanf("%d", &nbJoueur);
    Joueur** joueurs = (Joueur**)malloc(nbJoueur*sizeof(Joueur*));
    
    /*      Shuffle le deck des 104 cartes    */
    for(int i=0; i<DECK_SIZE;i++)
        cartesTotal[i]=i+1;
    shuffle(cartesTotal,DECK_SIZE);

    //Afichage du deck
    //afficherDeck(cartesTotal);


    /*      Distribution des cartes et initialise la vie des joueurs à 0     */
    for(int i=0; i < nbJoueur; i++)
    {
        
        int count=0;
        joueurs[i] = (Joueur*)malloc(sizeof(Joueur));    
        joueurs[i]->teteBoeuf=0;
        joueurs[i]->nbCarte = 10;  
        joueurs[i]->cartes = (int*)calloc(10, sizeof(int));
        

        for(int j=0;j<DECK_SIZE;j++)
        {
            if(cartesTotal[j]!=0 && count<10)
            {
                joueurs[i]->cartes[count]=cartesTotal[j];
                cartesTotal[j]=0;
                count++;
            }
        }
    }

    /*      Création et remplissage des piles       */
    int** piles = creerPiles();

    int count=0;
    for(int i=0; i<DECK_SIZE; i++)
    {
        if(cartesTotal[i]!=0 && count<4)
        {
            piles[count][0]=cartesTotal[i];
            cartesTotal[i]=0;
            count++;
        }
    }
    
    while(joueurs[0]->nbCarte>0)
    {      
        for(int i=0 ; i < nbJoueur; i++)
        {
            printf("\n");
            affichePiles(piles);
            printf("\nTetes de boeufs du joueur %d : %d\n",i, joueurs[i]->teteBoeuf);
            printf("Cartes du joueur %d : ",i);
            for(int j=0;j<joueurs[i]->nbCarte;j++)
                printf("%d ",joueurs[i]->cartes[j]);
            do{
            printf("\nJoueur %d choisis une carte : ",i);
            scanf("%d",&choixCarte);
            }while(!isCarteValid(joueurs[i],choixCarte));
            retirerCarte(joueurs[i], choixCarte);
            joueurs[i]->teteBoeuf+=jouerCarte(piles,choixCarte);
        }   
    }
    detruitPiles(piles);
    for(int i=0;i<nbJoueur;i++){
        free(joueurs[i]->cartes);
        free(joueurs);
    }
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

void retirerCarte(Joueur* joueur, int carte){
    //Création d'un tableau recevant les cartes du joueur, sans celle qui vient d'être joué
    int* temp = (int*)calloc(joueur->nbCarte-1, sizeof(int));
    int cpt = 0;
    for(int i=0;i<joueur->nbCarte;i++){
        if(joueur->cartes[i]!=carte){
            temp[cpt]=joueur->cartes[i];
            cpt++;
        }
    }

    //Libération de la mémoire occupé par l'ancienne main du joueur
    free(joueur->cartes);
    joueur->cartes = temp;  
    joueur->nbCarte--;
}

void ajouterCartePile(int* pile, int carte){
    int i = compterCartePile(pile);
    pile[i]=carte;
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

int** creerPiles()
{
    int** mat = (int**)malloc(4*sizeof(int*));
    for(int i=0; i<4; i++)
    {
        mat[i] = (int*)calloc(6, sizeof(int));
    }
    return mat;
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

void detruitPiles(int** m)
{
    for(int i=0; i<4; i++)
    {
        free(m[i]);
    }
    free(m);
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

void afficherDeck(int* array)
{
    for(int j=0;j<DECK_SIZE;j++)
    {
        printf("cartesTotal[%d]=%d\n",j,array[j]);
    }
}

bool isCarteValid(Joueur* joueur,int choixCarte)
{
    bool res=false;
    for(int i=0; i<10; i++)
    {
        if(choixCarte==joueur->cartes[i])
            res=true;
    }

    return res;
}