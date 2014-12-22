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
/// Personal
#include "globalconstants.h"

///
/// GLOBAL VALUES
///

///
/// FUNCTIONS
///

///
/// MAIN
///
int main(int argc, char const *argv[])
{
	int connectionPipe;


	//_INITIALISATION_

	/// Clear the screen
	system("clear");

	/// Little welcome message
	VERBOSE("\t.::|[    GAME SERVER    ]|::.\n\t^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

	// Create named pipe
	DEBUG("[SERVER] Creating connection named pipe.");
	if (mkfifo(CONNECTION_NAMED_PIPE_NAME, 770)==-1)
	{
		if (unlink(CONNECTION_NAMED_PIPE_NAME)==-1)
		{
			ERR("[SERVER] "ERR_NAMED_PIPE_CREATION_FAIL_MSG);
			exit(ERR_NAMED_PIPE_CREATION_FAIL);
		}
		mkfifo(CONNECTION_NAMED_PIPE_NAME, 770);
	}


	// Create a virtual client that will connect the pipe


	// Open named pipe
	DEBUG("[SERVER] Opening connection named pipe.");
	if ((connectionPipe=open(CONNECTION_NAMED_PIPE_NAME, O_RDONLY))==-1)
	{
		ERR("[SERVER] "ERR_OPEN_FAIL_MSG);
		exit(ERR_OPEN_FAIL);
	}

	//_LISTENLOOP_

		//_NEW THREAD FOR CLIENT_

	//_END_

	DEBUG("[SERVER] Unlinking the connection named pipe");
	if (unlink(CONNECTION_NAMED_PIPE_NAME)==-1) {
		ERR("[SERVER] Did not manage to unlink connection named pipe");
	}
	

	return 0;
}