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
/**********************  FICHIER CONTENANT LE SERVEUR  ************************/
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
#include <pthread.h>
/// Personal
#include "globalconstants.h"

///
/// GLOBAL VALUES
///

///
/// FUNCTIONS
///

/// Procedure forking a child process to behave like a client
void createVirtualClient(){
	switch(fork()){
		case -1:
			ERR("[SERVER] "ERR_FORK_FAIL_MSG);
			exit(ERR_FORK_FAIL);
		case 0:
			execl("client", "client", VIRTUAL_CLIENT_ARGUMENT, NULL);
			system("pwd");
			ERR("[SERVER] "ERR_EXEC_FAIL_MSG);
			exit(ERR_EXEC_FAIL);
		default:
			break;
	}
}

/// Procedure creating the connection pipe
void createConnectionPipe(){
	if (mkfifo(CONNECTION_NAMED_PIPE_NAME, 700)==-1)
	{
		if (unlink(CONNECTION_NAMED_PIPE_NAME)==-1)
		{
			ERR("[SERVER] "ERR_NAMED_PIPE_CREATION_FAIL_MSG);
			exit(ERR_NAMED_PIPE_CREATION_FAIL);
		}
		mkfifo(CONNECTION_NAMED_PIPE_NAME, 700);
	}
}

void *newConnection(void *arg){
	clientInfo *client = (clientInfo *) arg;
	DEBUG("[SERVER: THREAD %s] Thread created for %s (%d)", 
		client->name,client->name,(int)client->pid);
	VERBOSE("%s is connected.",client->name);
	pthread_exit(OK);
}

/// Procedure to open 'safely' a file
int safeOpen(char *pathname)
{
	int file;
	if ((file=open(pathname, O_RDONLY))==-1)
	{
		ERR("[SERVER] "ERR_OPEN_FAIL_MSG);
		exit(ERR_OPEN_FAIL);
	}
	return file;
}

///
/// MAIN
///
int main(int argc, char const *argv[])
{
	int connectionPipe;
	clientInfo newClient;
	pthread_t thread;

	//_INITIALISATION_

	/// Clear the screen
	system("clear");

	/// Little welcome message
	VERBOSE("\t.::|[    GAME SERVER    ]|::.\n"
			"\t^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	
	VERBOSE("Initialisation.");

	// Create named pipe
	DEBUG("[SERVER] Creating connection named pipe.");
	createConnectionPipe();
	

	//WARNING Find another option
	//Forcing access to the pipe to the clients
	system("chmod 700 "CONNECTION_NAMED_PIPE_NAME);

	// Create a virtual client that will connect the pipe
	DEBUG("[SERVER] Starting virtual client.");
	createVirtualClient();

	// Open named pipe
	DEBUG("[SERVER] Opening connection named pipe.");
	connectionPipe = safeOpen(CONNECTION_NAMED_PIPE_NAME);


	//_LISTENLOOP_
	VERBOSE("Server is ready to accept new players.");
	while(true){
		usleep(100000);
		if(read(connectionPipe, &newClient, sizeof(clientInfo))<=0)
		{

		}
		else
		{
			//_NEW THREAD FOR CLIENT_
			pthread_create(&thread, NULL, newConnection, &newClient);
		}	
	}

	//_END_

	DEBUG("[SERVER] Unlinking the connection named pipe");
	close(connectionPipe);
	if (unlink(CONNECTION_NAMED_PIPE_NAME)==-1) {
		ERR("[SERVER] Did not manage to unlink connection named pipe");
	}
	

	return 0;
}