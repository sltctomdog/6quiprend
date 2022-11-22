#include <stdio.h>
#include <stdlib.h>

int** creerPiles();
void affichePiles(int** mat);
void detruitPiles(int** m);
void jouerCarte(int** mat, int carte);

int main(){
    int** piles = creerPiles();
    piles[0][0] = 6;
    piles[1][0] = 58;
    piles[2][0] = 21;
    piles[3][0] = 89;
    affichePiles(piles);
    jouerCarte(piles, 3);
    detruitPiles(piles);
}

void jouerCarte(int** mat, int carte){
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
    printf("%d, %d", indicePile, indiceCarte);
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