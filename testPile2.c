#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define DECK_SIZE 104

typedef struct Joueur
    {
        int choixCarte;
        int cartes[10];
        int teteBoeuf;
    }Joueur;

int** creerPiles();
void affichePiles(int** mat);
void detruitPiles(int** m);
void jouerCarte(int** mat, int carte);
void shuffle(int* array, size_t length);
void afficherDeck(int* array);
bool isCarteValid(Joueur joueur,int choixCarte);
int** poseCarte(int** pile, int choixCarte);

int main(){

    /*                                                          Initilalisation                                               */
    int cartesTotal[DECK_SIZE];
    int nbJoueur=2;
    int choixCarte;
    //printf("Nombre Joueur : ");
    //scanf("%d", &nbJoueur);
    Joueur joueurs[nbJoueur];

    /*      Shuffle le deck des 104 cartes    */
    for(int i=0; i<DECK_SIZE;i++)
        cartesTotal[i]=i+1;
    shuffle(cartesTotal,DECK_SIZE);

    //Afichage du deck
    afficherDeck(cartesTotal);


    /*      Distribution des cartes     */
    for(int i=0; i < nbJoueur; i++)
    {
        int count=0;
        
        printf("\nCartes de J%d : ",i);
        for(int j=0;j<DECK_SIZE;j++)
        {
            if(cartesTotal[j]!=0 && count<10)
            {
                joueurs[i].cartes[count]=cartesTotal[j];
                cartesTotal[j]=0;
                count++;
            }
        }
        // Affichage des cartes pour chaque joueur
        for(int j=0;j<10;j++)
        {
            printf("%d ",joueurs[i].cartes[j]);
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

    while(1)
    {
        affichePiles(piles);
        for(int i=0 ; i < nbJoueur; i++)
        {
            printf("J%d voici tes cartes : ",i);
            for(int j=0;j<10;j++)
                printf("%d ",joueurs[i].cartes[j]);
            do{
            printf("\nJ%d choisis une carte :",i);
            scanf("%d",&choixCarte);
            }while(!isCarteValid(joueurs[i],choixCarte));

            piles = poseCarte(piles, choixCarte);
        }
    }
}

void jouerCarte(int** mat, int carte)
{
    int indicePile = 10;
    int indiceCarte = 10;
    int valDiff = 1000;

    for(int i=0;i<4;i++){
        int posCarte = 0;
        for(int j=0;j<6;j++){
            if(mat[i][j]!=0){            
                posCarte+=1;
            }
        }
        if(carte>mat[i][posCarte-1] && (carte-mat[i][posCarte-1]<valDiff)){
            valDiff = carte-mat[i][posCarte-1];
            indicePile = i;
            indiceCarte = posCarte - 1;
        }
    }
    //Affiche à quelle pile la carte doit être ajouté (pas encore fait si la carte est inférieur à tout)
    printf("%d, %d\n", indicePile, indiceCarte);
    //Si la pile contient moins de 6 carte -> ajouter la carte a la pile
    //Sinon compter les points de la pile, reset la pile, ajouter la carte
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
    printf("\n");
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

bool isCarteValid(Joueur joueur,int choixCarte)
{
    bool res=false;
    for(int i=0; i<10; i++)
    {
        if(choixCarte==joueur.cartes[i])
            res=true;
    }

    return res;
}

int** poseCarte(int** pile, int choixCarte)
{
    int count=0;
    int* value=(int*)malloc(4 * sizeof(int));
    int correctValue=0;
    int correctPile=0;
    int sortie=0;

    /*  On récupère les dernières valeurs de piles  */
    for (int i=0 ; i<4; i++)
    {
        for (int j=0; j<6; j++)
        {
            if(pile[i][j]!=0)
            {
                value[count]=pile[i][j];
            }
        }
        count++;
    }

    printf("\nvalues\n");
    for(int i=0; i<4 ; i++)
        printf("%d\n",value[i]);

    /*  On vérifie la valeur inférieur la plus proche de la carte choisie   */
    for (int j = 0 ; j<4 ; j++)
    {
        if(value[j]<choixCarte && value[j]>=value[correctPile])
        {
            correctValue=choixCarte;
            correctPile=j; // On sauvegarde le numéro de la pile a remplir
        }
    }

    /*  On remplit le dernier placement de la pile  */
    for (int j=0; j<6; j++)
        {
            if(pile[correctPile][j]==0 && sortie==0)
            {
                pile[correctPile][j]=correctValue;
                sortie=1;
            }
        }
    return pile;
}

