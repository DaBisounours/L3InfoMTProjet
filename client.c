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
/// Personal
#include "globalconstants.h"

///
/// GLOBAL VALUES
///
/// Boolean to know if this client is the virtual client used for opening 
/// of the connection named pipe
int isVirtualClient = false;

//
// FUNCTIONS
//
/// Procedure creating the communication pipe
void createCommunicationPipe(char *pathname){
	if (mkfifo(pathname, 700)==-1)
	{
		if (unlink(pathname)==-1)
		{
			ERR("[SERVER] "ERR_NAMED_PIPE_CREATION_FAIL_MSG);
			exit(ERR_NAMED_PIPE_CREATION_FAIL);
		}
		mkfifo(pathname, 700);
	}
}

/// Procedure to open 'safely' a file
int safeOpen(char *pathname)
{
	int file;
	if ((file=open(pathname, O_RDONLY))==-1)
	{
		ERR("[CLIENT] "ERR_OPEN_FAIL_MSG);
		exit(ERR_OPEN_FAIL);
	}
	return file;
}

//
// MAIN
//
int main(int argc, char const *argv[])
{
	char clientName[MAX_CLIENT_NAME_LENGTH];
	int connectionPipe;
	int communicationPipe;
	clientInfo this;

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
	// Open named pipe
	DEBUG("[CLIENT: %s] Opening connection named pipe.", clientName);
	if ((connectionPipe=open(CONNECTION_NAMED_PIPE_NAME, O_WRONLY))==-1)
	{
		ERR("[CLIENT: %s] "ERR_OPEN_FAIL_MSG, clientName);
		exit(ERR_OPEN_FAIL);
	}

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
	snprintf(this.namedTubeName, MAX_NAMED_TUBE_NAME_LENGTH, ".%d.pipe",getpid());

	// Create named tube to communicate with server
	DEBUG("[CLIENT %s] Creating communication pipe.", clientName);
	createCommunicationPipe(this.namedTubeName);

	// Send over data to server
	DEBUG("[CLIENT %s] Connection to the server.", clientName);
	write(connectionPipe, &this, sizeof(clientInfo));

	// Wait for server acknowledgment
	DEBUG("[CLIENT %s] Waiting for acknowledgment.", clientName);
	communicationPipe = open(this.namedTubeName, O_RDWR);

	DEBUG("[CLIENT %s] Connected.", clientName);
	//_GUESSLOOP_


	//_END_
	close(communicationPipe);
	DEBUG("[CLIENT %s] Disconnected.", clientName);
	return 0;
}