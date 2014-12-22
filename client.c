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

/// The name of the client
char clientName[MAX_CLIENT_NAME_LENGTH];

/// The communication pipe file descriptor
int communicationPipe;

/// Boolean to tell if this client has lost or not
int lost = 0;

//
// FUNCTIONS
//
/// Procedure creating the communication pipe
void createCommunicationPipe(char *pathname){
	char chmodCommand[MAX_NAMED_TUBE_NAME_LENGTH+10];

	if (mkfifo(pathname, 700)==-1)
	{
		if (unlink(pathname)==-1)
		{
			ERR("[SERVER] "ERR_NAMED_PIPE_CREATION_FAIL_MSG);
			exit(ERR_NAMED_PIPE_CREATION_FAIL);
		}
		mkfifo(pathname, 700);

	}
	snprintf(chmodCommand, MAX_NAMED_TUBE_NAME_LENGTH+10,
		"chmod 700 %s",pathname);
	system(chmodCommand);
}

/// Procedure to open 'safely' a file
int safeOpen(char *pathname, int mode)
{
	int file;
	if ((file=open(pathname, mode))==-1)
	{
		ERR("[CLIENT] %s"ERR_OPEN_FAIL_MSG, clientName);
		exit(ERR_OPEN_FAIL);
	}
	return file;
}


void sendQuit(int pipe, int reason)
{
	VERBOSE("Leaving the game.");
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
	// Sending quit request if connection to server is still on
	sendQuit(communicationPipe, REASON_INTERRUPTED);

	// ERRor message
	if (signal != 0)
	{
		ERR_NOPERROR("Lost connection to server.");
	}
	

	// Exit with error code
	exit(INTERRUPTED);
}


// Acts like a filter for special commands. Returns if it is a value or not
int getCommand(int *value) 
{
	char command[100];
	read(0, command, 100);
	if(!strncmp(command, "quit\n",5) || !strncmp(command,"q\n",2))
		interrupt(0);
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

	clientInfo this;
	serverMessage messageFromServer;

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
			snprintf(clientName,MAX_CLIENT_NAME_LENGTH, "Virtual Client");
		}
		// Name
		else
		{
			isVirtualClient=false;
			snprintf(clientName,MAX_CLIENT_NAME_LENGTH, "%s", argv[1]);
		}
	}
	// No name, default name will be the pid
	else
	{
		isVirtualClient=false;
		snprintf(clientName,MAX_CLIENT_NAME_LENGTH, "%d", (int)getpid());
	}

	// Little welcome message
	if(!isVirtualClient) 
	{		
		/// Clear the screen
		system("clear");
		VERBOSE("Hello %s! Welcome to the guessing game!\n"
"If at any moment during the game, you need the rules, type 'rules' or 'r'.\n"
"If you wish to leave the game, type 'q' or 'quit'.", clientName);
	}
	else
	{
		DEBUG("[CLIENT: %s] Virtual Client created.", clientName);
	}

	//_CONNECTION_

	// Manage signals
	signal(SIGINT, interrupt);

	// Open named pipe
	DEBUG("[CLIENT: %s] Opening connection named pipe.", clientName);
	connectionPipe=safeOpen(CONNECTION_NAMED_PIPE_NAME, O_WRONLY);


	// If Virtual Client then die... Goodbye son...
	if(isVirtualClient) 
	{
		DEBUG("[CLIENT: %s] Closing connection named pipe.", clientName);
		close(connectionPipe);
		exit(OK);
	}
	
	DEBUG("[CLIENT %s] Client started.", clientName);

	// Prepare data to send over to server
	DEBUG("[CLIENT %s] Preparing Data.", clientName);
	this.pid=getpid();
	snprintf(this.name, MAX_CLIENT_NAME_LENGTH, "%s", clientName);
	snprintf(this.pipeName, MAX_NAMED_TUBE_NAME_LENGTH, ".%d.pipe",getpid());

	// Create named tube to communicate with server
	DEBUG("[CLIENT %s] Creating communication pipe.", clientName);
	createCommunicationPipe(this.pipeName);

	// Send over data to server
	DEBUG("[CLIENT %s] Connection to the server.", clientName);
	write(connectionPipe, &this, sizeof(clientInfo));
	close(connectionPipe);
	// Wait for server acknowledgment
	DEBUG("[CLIENT %s] Waiting for acknowledgment.", clientName);
	communicationPipe = safeOpen(this.pipeName, O_RDWR);

	// Cheking server confirmation
	read(communicationPipe, &messageFromServer, sizeof(serverMessage));
	if(messageFromServer.type==KICK) 
	{
		ERR_NOPERROR("Connection refused by server. Room full.");
		interrupt(0);
	}
	if(messageFromServer.type==ACCEPT){
		DEBUG("[CLIENT %s] Connected.", clientName);
	}

	//_GUESSLOOP_
	//TODO Commands
	while(!lost){
		if(getCommand(&clientChoice))
		{
			DEBUG("[CLIENT %s] Choice : %d.", clientName, clientChoice);
			//write
			usleep(100000);
			//read
		}
	}

	
	//_END_
	sendQuit(communicationPipe, REASON_USER_REQUEST);
	close(communicationPipe);
	DEBUG("[CLIENT %s] Disconnected.", clientName);

	// Delete the pipe
	unlink(this.pipeName);
	return 0;
}