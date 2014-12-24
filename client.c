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
/**********************  FICHIER CONTENANT LE CLIENT  *************************/
/******************************************************************************/
/******************************************************************************/

///
/// INCLUDES
///
/// Standard
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
/// Personal
#include "globalconstants.h"

///
/// GLOBAL VALUES
///
/// Boolean to know if this client is the virtual client used for opening 
/// of the connection named pipe
int isVirtualClient = false;

/// The information structure of the client
clientInfo this;

/// The communication pipe file descriptor
int communicationPipe;

/// Boolean to tell if a game has started on or not
bool gameIsOn=false;

/// Messages boxes from and to server
/// We keep them accessible to a signal handler to give a reason if the client
/// Loses connection.
serverMessage messageFromServer;
clientMessage messageToServer;

//
// FUNCTIONS
//

/// Print the rules
void verboseRules(){
	system("cat ./rules.txt");
}


/// Procedure creating the communication pipe
void createCommunicationPipe(char *pathname){
	char chmodCommand[MAX_NAMED_PIPE_NAME_LENGTH+10];

	if (mkfifo(pathname, 700)==-1)
	{
		if (unlink(pathname)==-1)
		{
			ERR("[SERVER] "ERR_NAMED_PIPE_CREATION_FAIL_MSG);
			exit(ERR_NAMED_PIPE_CREATION_FAIL);
		}
		mkfifo(pathname, 700);

	}
	snprintf(chmodCommand, MAX_NAMED_PIPE_NAME_LENGTH+10,
		"chmod 700 %s",pathname);
	system(chmodCommand);
}

/// Procedure to open 'safely' a file
int safeOpen(char *pathname, int mode)
{
	int file;
	if ((file=open(pathname, mode))==-1)
	{
		ERR("[CLIENT] %s"ERR_OPEN_FAIL_MSG, this.name);
		exit(ERR_OPEN_FAIL);
	}
	return file;
}


void sendQuit(int pipe, int reason)
{
	DEBUG("\n[CLIENT: %s] Leaving the game.", this.name);
	clientMessage msg;
	msg.type=QUIT;
	msg.choice=reason;
	if(write(pipe, &msg, sizeof (clientMessage))==-1)
	{
		ERR("Error while writing in the communication pipe");
	}
}


// When receiving SIGINT
void interrupt(int signal)
{
	int flags = fcntl(communicationPipe, F_GETFL, 0);
	fcntl(communicationPipe, F_SETFL, flags | O_NONBLOCK);
	// Sending quit request if connection to server is still on
	sendQuit(communicationPipe, REASON_INTERRUPTED);
	
	// If last message from server tells why
	usleep(100000);
	read(communicationPipe, &messageFromServer, sizeof(serverMessage));
	close(communicationPipe);
	// Unlink the pipe
	unlink(this.pipeName);
	if (messageFromServer.type==KICK)
	{
		if(messageFromServer.choice==REASON_SERVER_KICK){
			ERR_NOPERROR("You have been kicked by the server.");		
		}
		else if(messageFromServer.choice==REASON_SERVER_INTERRUPTION){
			ERR_NOPERROR("The server has shut down.");		
		}
		// Exit with error code
		exit(INTERRUPTED);
	}
	else if (messageFromServer.type==GAME)
	{
		if(messageFromServer.choice == WIN){
			VERBOSE("IT'S A WIN!");
		}
		exit(OK);
	}
	else
	{
		exit(OK);
	}

}

void lose(int signal)
{
	VERBOSE("Game over! Sorry but you have lost!\n");
	interrupt(0);
}

void start(int signal)
{
	gameIsOn=true;
	VERBOSE("Game started!");
	VERBOSE("\nType a number: ");
}


// Acts like a filter for special commands. Returns if it is a value or not
bool getCommand(int *value) 
{
	char command[100];
	read(0, command, 100);
	if(!strncmp(command, "quit\n",5) 
	|| !strncmp(command,"q\n",2) 
	|| !strncmp(command,"exit\n",5))
		interrupt(0);
	else if(!strncmp(command, "rules\n",6) 
		 || !strncmp(command,"r\n",2) ){
		verboseRules();
		return false;
	}
		
	else
	{
		*value = atoi(command);
		if (*value == 0 && strncmp(command, "0\n",5))
		{
			return false;
		}
	}
	//TODO Implement this
	return true;
}

//
// MAIN
//
int main(int argc, char const *argv[])
{
	int connectionPipe;
	int clientChoice;

	//_INITIALISATION_

	/// Manage a name or a help section in the first argument
	if (argc==2)
	{
		// Help required
		if(!strcmp(argv[1],"--help") 
		|| !strcmp(argv[1],"--usage") 
		|| !strcmp(argv[1],"-h"))
		{
			VERBOSE("Usage: %s [\"Your Name\"]", argv[0]);
			exit(OK);
		}
		// Virtual Client
		else if(!strcmp(argv[1],VIRTUAL_CLIENT_ARGUMENT))
		{
			isVirtualClient=true;
			snprintf(this.name,MAX_CLIENT_NAME_LENGTH, "Virtual Client");
		}
		// Name
		else
		{
			isVirtualClient=false;
			snprintf(this.name,MAX_CLIENT_NAME_LENGTH, "%s", argv[1]);

		}
	}
	// No name, default name will be the pid
	else
	{
		isVirtualClient=false;
		snprintf(this.name,MAX_CLIENT_NAME_LENGTH, "%d", (int)getpid());
	}

	// Little welcome message
	if(!isVirtualClient) 
	{		
		/// Clear the screen
		system("clear");
		VERBOSE("Hello %s! Welcome to the guessing game!\n"
"If at any moment during the game, you need the rules, type 'rules' or 'r'.\n"
"If you wish to leave the game, type 'q' or 'quit'.", this.name);
	}
	else
	{
		DEBUG("[CLIENT: %s] Virtual Client created.", this.name);
	}

	//_CONNECTION_

	// Manage signals
	signal(SIGINT, interrupt);
	signal(SIG_STOP, lose);
	signal(SIG_START, start);

	// Open named pipe
	DEBUG("[CLIENT: %s] Opening connection named pipe.", this.name);
	connectionPipe=safeOpen(CONNECTION_NAMED_PIPE_NAME, O_WRONLY);


	// If Virtual Client then die... Goodbye son...
	if(isVirtualClient) 
	{
		DEBUG("[CLIENT: %s] Closing connection named pipe.", this.name);
		close(connectionPipe);
		exit(OK);
	}
	
	DEBUG("[CLIENT: %s] Client started.", this.name);

	// Prepare data to send over to server
	DEBUG("[CLIENT: %s] Preparing Data.", this.name);
	this.pid=getpid();
	snprintf(this.pipeName, MAX_NAMED_PIPE_NAME_LENGTH, ".%d.pipe",getpid());

	// Create named tube to communicate with server
	DEBUG("[CLIENT: %s] Creating communication pipe.", this.name);
	createCommunicationPipe(this.pipeName);

	// Send over data to server
	DEBUG("[CLIENT: %s] Connection to the server.", this.name);
	write(connectionPipe, &this, sizeof(clientInfo));
	close(connectionPipe);
	// Wait for server acknowledgment
	DEBUG("[CLIENT: %s] Waiting for acknowledgment.", this.name);
	communicationPipe = safeOpen(this.pipeName, O_RDWR);

	// Cheking server confirmation
	read(communicationPipe, &messageFromServer, sizeof(serverMessage));
	if(messageFromServer.type==KICK) 
	{
		ERR_NOPERROR("Connection refused by server. Room full.");
		interrupt(0);
	}
	if(messageFromServer.type==ACCEPT){
		DEBUG("[CLIENT: %s] Connected.", this.name);
		if(messageFromServer.choice==GAME)
			gameIsOn=true;
	}

	//_GUESSLOOP_
	//TODO Commands
	while(true){
		if(getCommand(&clientChoice))
		{
			ERR_NOPERROR("PLOP gio:%d", gameIsOn);
			if(gameIsOn){
				DEBUG("[CLIENT: %s] Choice : %d.", this.name, clientChoice);
				messageToServer.type=GUESS;
				messageToServer.choice=clientChoice;
				write(communicationPipe, &messageToServer, sizeof(clientMessage));
				usleep(100000);
				read(communicationPipe, &messageFromServer, sizeof(serverMessage));
			}	
			if(messageFromServer.type==GAME)
			{
				if(messageFromServer.choice==HIGHER){
					DEBUG("[CLIENT: %s] Answer : %d (HIGHER).", this.name, messageFromServer.choice);
					VERBOSE("Incorrect. Try again with a higher value.");
				}
				else if(messageFromServer.choice==LOWER){
					DEBUG("[CLIENT: %s] Answer : %d (LOWER).", this.name, messageFromServer.choice);
					VERBOSE("Incorrect. Try again with a lower value.");
				}
				else {
					DEBUG("[CLIENT: %s] Answer : %d (WIN?).", this.name, messageFromServer.choice);
					interrupt(0);
				}
			}
			else if(messageFromServer.type==GAME_NOT_ON || gameIsOn==false){
				VERBOSE("Game is not started. Please wait...");
				gameIsOn=false;
			}
		}
	}

	
	//_END_
	sendQuit(communicationPipe, REASON_USER_REQUEST);
	close(communicationPipe);
	DEBUG("[CLIENT: %s] Disconnected.", this.name);

	// Delete the pipe
	unlink(this.pipeName);
	return 0;
}