#include <stdio.h>
#include <stdlib.h>

int** creerPiles();
void affichePiles(int** mat);
void detruitPiles(int** m);

int main(){
    int** piles = creerPiles();
    piles[1][2] = 6;
    piles[2][2] = 6;
    piles[1][3] = 9;
    piles[2][3] = 9;
    affichePiles(piles);
    detruitPiles(piles);
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