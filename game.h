#include "utils.h"

void shuffle(int *array, size_t length);
int** creerPiles(int* paquet); 
void detruitPiles(int** m); 
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