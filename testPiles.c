#include <stdio.h>
#include <stdlib.h>

int** creerPiles();
void affichePiles(int** mat);
void detruitPiles(int** m);
int jouerCarte(int** piles, int carte);
int compterPointPile(int* pile);
void resetPile(int* pile, int carte);
int compterCartePile(int* pile);
void ajouterCartePile(int* pile, int carte);

int main(){
    int** piles = creerPiles();
    piles[0][0] = 21;
    piles[1][0] = 58;
    piles[2][0] = 6;
    piles[3][0] = 89;
    affichePiles(piles);

    char var[20];
    int points = jouerCarte(piles, 8);
    points = jouerCarte(piles, 10);
    points = jouerCarte(piles, 11);
    points = jouerCarte(piles, 12);
    points = jouerCarte(piles, 60);
    //points = jouerCarte(piles, 19);
    
    printf("%d\n", points);
    printf("\n");
    affichePiles(piles);
    detruitPiles(piles);
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