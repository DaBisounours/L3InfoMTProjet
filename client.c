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

//
// FUNCTIONS
//

//
// MAIN
//
int main(int argc, char const *argv[])
{
	char clientName[MAX_CLIENT_NAME_LENGTH];


	//_INITIALISATION_

	/// Clear the screen
	system("clear");

	/// Manage a name or a help section in the first argument
	if (argc==2)
	{
		// Help required
		if(!strcmp(argv[1],"--help") || !strcmp(argv[1],"--usage") || !strcmp(argv[1],"-h"))
		{
			VERBOSE("Usage: %s [\"Your Name\"]", argv[0]);
			exit(OK);
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
	VERBOSE("Hello %s! Welcome to the guessing game!\n"
			"If at any moment during the game, you need the rules, type 'rules' or 'r'.\n"
			"If you wish to leave the game, type 'q' or 'quit'.", clientName);

	//_CONNECTION_

	//_GUESSLOOP_

	//_END_
	return 0;
}