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

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"

//DISPLAYING MACROS
#define ERR(...) fprintf (stderr,"\x1B[31m"__VA_ARGS__);perror(" ");printf("\x1B[0m\n")
#define VERBOSE(...) fprintf (stdout,"\x1B[32m"__VA_ARGS__);printf("\x1B[0m\n")
#define DEBUG(...) fprintf (stdout,"\x1B[33m"__VA_ARGS__);printf("\x1B[0m\n")

//GAME RELATED CONSTANTS
#define DIFFICULTY 1
#define BASE_GAME_RANGE 20
#define GAME_TIMEOUT 20


//COMMUNICATION RELATED CONSTANTS
#define MAX_CLIENT_NAME_LENGTH 100
#define MAX_NAMED_TUBE_NAME_LENGTH 50
#define CONNECTION_NAMED_PIPE_NAME ".serverConnection.pipe"

//RETURN CODES AND MESSAGES
#define OK 0

#define ERR_NAMED_PIPE_CREATION_FAIL 30
#define ERR_NAMED_PIPE_CREATION_FAIL_MSG "Error while creating the named communication named pipe"

#define ERR_OPEN_FAIL 20
#define ERR_OPEN_FAIL_MSG "Error while opening the named communication named pipe"

//TYPES
typedef struct struct_clientInfo
{
	// PID of the client
	pid_t pid;
	// Name of the client
	char name[MAX_CLIENT_NAME_LENGTH];
	// Name of the named tube
	char namedTubeName[MAX_NAMED_TUBE_NAME_LENGTH];

} clientInfo;

/// Message from the client
typedef struct struct_clientMessage
{
	/// Type of message: Guess / Quit
	int type;

	/// Choice: 
	/// 	Guess: number chosen
	///		Quit: reason why the client leaves
	int choice;
}clientMessage;

/// Message from the server
typedef struct struct_serverMessage
{
	/// Type of message: Kick / Answer
	int type;

	/// Choice:
	///		Kick: reason why the client is kicked
	///		Answer: higher/lower/found!
	int choice;

	/// Time left in seconds
	int timeLeft;
}serverMessage;