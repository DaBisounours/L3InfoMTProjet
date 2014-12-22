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

//
// MAIN
//
int main(int argc, char const *argv[])
{
	char clientName[MAX_CLIENT_NAME_LENGTH];
	int connectionPipe;

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
			snprintf(clientName,MAX_CLIENT_NAME_LENGTH, "%s", argv[1]);
		}
	}
	// No name, default name will be the pid
	else
	{
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

	//_GUESSLOOP_

	//_END_
	return 0;
}