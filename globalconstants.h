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
#define DEBUG_MODE 1

///DISPLAYING MACROS
#define ERR(...) fprintf (stderr,"\x1B[31m"__VA_ARGS__);perror(" ");printf("\x1B[0m\n")
#define ERR_NOPERROR(...) fprintf (stderr,"\x1B[31m"__VA_ARGS__);printf("\x1B[0m\n")
#define VERBOSE(...) fprintf (stdout,"\x1B[32m"__VA_ARGS__);printf("\x1B[0m\n")
#define DEBUG(...) if(DEBUG_MODE){fprintf (stdout,"\x1B[33m"__VA_ARGS__);printf("\x1B[0m\n");}

///GAME RELATED CONSTANTS
#define DIFFICULTY 1
#define BASE_GAME_RANGE 20
#define GAME_TIMEOUT 20
#define MAX_PLAYERS 3
#define NO_PLAYER -1


///COMMUNICATION RELATED CONSTANTS
#define MAX_CLIENT_NAME_LENGTH 100
#define MAX_NAMED_TUBE_NAME_LENGTH 50
#define CONNECTION_NAMED_PIPE_NAME ".serverConnection.pipe"
#define VIRTUAL_CLIENT_ARGUMENT "--!VC!"

	///CLIENT MESSAGES
#define GUESS 0
#define QUIT 1
#define REASON_USER_REQUEST 0
#define REASON_INTERRUPTED 1
	///SERVER MESSAGES
#define ACCEPT 1
#define KICK 2
#define COMMUNICATION 3
#define REASON_ROOM_FULL 0


///RETURN CODES AND MESSAGES
#define OK 0

#define INTERRUPTED 1

#define ERR_NAMED_PIPE_CREATION_FAIL 30
#define ERR_NAMED_PIPE_CREATION_FAIL_MSG "Error while creating the named communication named pipe"

#define ERR_OPEN_FAIL 20
#define ERR_OPEN_FAIL_MSG "Error while opening the named communication named pipe"

#define ERR_FORK_FAIL 10
#define ERR_FORK_FAIL_MSG "Error while creating child process"

#define ERR_EXEC_FAIL 40
#define ERR_EXEC_FAIL_MSG "Error while executing binary"

//TYPES
// Client information
typedef struct struct_clientInfo
{
	/// PID of the client
	pid_t pid;
	// Name of the client
	char name[MAX_CLIENT_NAME_LENGTH];
	// Name of the named tube
	char pipeName[MAX_NAMED_TUBE_NAME_LENGTH];

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