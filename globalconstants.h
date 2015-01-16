/******************************************************************************/
/******************************************************************************/
/*******************                                        *******************/
/*******************            PROJET MULTITACHE           *******************/
/*******************                                        *******************/
/******************************************************************************/
/******************************************************************************/
/*******************          Auteur: Evan Graïne           *******************/
/***    Description: Mini jeu en pseudo-réseau local consistant à deviner    **/
/***                 un nombre fuyant.                                       **/
/******************************************************************************/
/******************************************************************************/
/********************  FICHIER CONTENANT LES CONSTANTES  **********************/
/******************************************************************************/
/******************************************************************************/

///BOOLEANS
#define true 1
#define false 0
#define bool int

///DEBUGGING
#define DEBUG_MODE false

///DISPLAYING MACROS
#define ERR(...) fprintf (stderr,"\x1B[31m"__VA_ARGS__);perror(" ");printf("\x1B[0m\n")
#define ERR_NOPERROR(...) fprintf (stderr,"\x1B[31m"__VA_ARGS__);printf("\x1B[0m\n")
#define VERBOSE(...) fprintf (stdout,"\x1B[32m"__VA_ARGS__);printf("\x1B[0m\n")
#define DEBUG(...) if(DEBUG_MODE){fprintf (stdout,"\x1B[33m"__VA_ARGS__);printf("\x1B[0m\n");}

///GAME RELATED CONSTANTS
#define DIFFICULTY 1
#define BASE_GAME_RANGE 20
#define BASE_TIMEOUT 20
#define MAX_PLAYERS 3
#define WIN 31
#define HIGHER 33
#define LOWER 34



///RETURN CODES AND MESSAGES
#define OK 0

#define INTERRUPTED 1