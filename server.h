#include "game.c"
#include "utils.h"
#include "hpdf.h"
#include <setjmp.h>
jmp_buf env;
#ifdef HPDF_DLL
void  __stdcall
#else
void
#endif
error_handler  (HPDF_STATUS   error_no, HPDF_STATUS   detail_no, void *user_data)
{
    printf ("ERROR: error_no=%04X, detail_no=%u\n", (HPDF_UINT)error_no,(HPDF_UINT)detail_no);
    longjmp(env, 1);
}

int createSock();  
joueurArray* createjoueurArray(size_t max_joueur, size_t nb_bot); 
void acceptClient(int serverSock,joueurArray *joueurArr, size_t nb); 
void *pthreadInitClient(void *ptrClient); 
void freejoueurArray(joueurArray *in); 
void envoyerPiles(int** mat, joueur *joueur); 
void affichePiles(HPDF_Page page,int** mat); 
void envoyerMain(joueur *joueur); 
void envoyerCartesJouees(joueur* joueur, joueurArray* lstJoueur, int* cartesTourPrec, int indice);
void demanderCartes(joueurArray *in, int* cartesTour); 
void *pthreadDemanderCartes(void *ptrClient); 
void envoyerClassement(HPDF_Page page, joueurArray *in); 
bool checkNewGame(joueurArray *in); 
void *pthreadAskClient(void *ptrClient); 
int jouerCarte(int** piles, int carte, joueur* joueur); 
void afficheCartesJouees(HPDF_Page page, joueurArray* lstJoueur, int* cartesTourPrec);
void initPdf();
void addLinePdf(HPDF_Page, char*);
void savePdf(HPDF_Doc);