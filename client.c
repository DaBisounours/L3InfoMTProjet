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
#include <time.h>
/// Personal
#include "globalconstants.h"

///
/// GLOBAL VALUES
///
//Default values
int difficulty=DIFFICULTY;
int timer=0;
int limit=-1;

int theNumber;


//
// FUNCTIONS
//

/// Print the rules
void verboseRules(){
	system("cat ./rules.txt");
}


// When receiving SIGINT
void interrupt(int sig)
{
	// Resetting alarm
	alarm(0);
	// Error Message
	if(sig!=-1){ERR_NOPERROR("\nInterrupted.");}
	// Exit with error code	
	exit(INTERRUPTED);
}

// When receiving SIGALRM
void lose(int sig)
{
	if(sig==SIGALRM){VERBOSE("TIME IS OUT!");sig=-1;}
	VERBOSE("Game over! Sorry but you have lost!\n");
	interrupt(sig);
}


// Return a boolean telling if the string is numeric
bool isNumeric(const char *s)
{
	int i=0;
	// If string is null return false
	if(s==NULL) return false;
	// Else if string is empty or return line then return false
	else if(*s == '\0' || *s == '\n') return false;
	// Browse the string
	while(s[i] <= '9' && s[i] >= '0')i++;
	// If string contains only numeric char
	// Return true
	if (i==strlen(s)) return true;
	// Else return false
	else return false;
}


// Acts like a router for special commands. Returns if it is a value or not
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
		if (*value == 0 && strncmp(command, "0\n",2))
		{
			return false;
		}
		else return true;
	}
	//TODO Implement this
	return true;
}

// Randomisation function within a range [left,right] (left and right included)
int randRange(int left, int right){
	// Get a random number within [0, RAND_MAX]
	int random=rand();
	/// Include right bound
	right += 1;
	// Return ranged random number
	return (random%(right-left))+left;
}

/// Function to test number
int testNumber(int number)
{
	int dir, shift;

	/// Check
	if (theNumber == number) dir=WIN;
	else if(theNumber > number) dir = HIGHER;
	else dir=LOWER;

	/// Shift
	while((shift=randRange(-1,1))==0);
	theNumber+=shift;

	return dir;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//
// MAIN
//
int main(int argc, char const *argv[])
{
	int processedOptions=0, i, clientChoice;

	//_INITIALISATION_

	srand(time(NULL));

	/// Clear the screen
	system("clear");

	// Little welcome message
	VERBOSE("Hello there! Welcome to the guessing game!\n"
"If at any moment during the game, you need the rules, type 'rules' or 'r'.\n"
"If you wish to leave the game, type 'q' or 'quit'.");



	//Browsing arguments to set difficulty and/or time limit/guesses limit
	if (argc<=3 || argc>6)
	{
		// Help required
		VERBOSE("Usage: %s difficulty [-t timer] [-l tries_limit]\n"
			"\tDifficulty levels :\n"
			"\t\t0 = VERY EASY\n"
			"\t\t1 = DOABLE (with minus values)\n"
			"\t\t2 = HARD\n"
			"\t\t3 = LEGEND\n"
			"\t\t4 = OMNISCIENT\n"
			"\t\t5 = ...\n"
			"\t\t6 = HARDER TO BE HARDER...\n"
			"\tYou have to give at least a timer or a limit.\n", argv[0]);
		exit(OK);
	}
	else
	{
		// Difficulty
		if(!isNumeric(argv[1])) {
			ERR_NOPERROR("Difficulty argument must be a number!");
		}
		else {
			difficulty=atoi(argv[1]);
		}

		// Other args
		for(i=2; i<argc; i++) {
			// If timer limit is set
			if(!strncmp(argv[i],"-t",2)){
				if(isNumeric(argv[i+1]) && (timer=atoi(argv[i+1]))>=0){
					VERBOSE("Timer set to %d second(s).", timer);
					processedOptions++;
				}
				else{
					ERR_NOPERROR(
					"You must set an unsigned integer value next to the -t argument");
				}
			}
			// If tries limit is set
			if(!strncmp(argv[i],"-l",2)){
				if(isNumeric(argv[i+1]) && (limit=atoi(argv[i+1]))>=0){
					VERBOSE("Limit set to trying %d time(s).", limit);
					processedOptions++;
				}
				else{
					ERR_NOPERROR(
					"You must set an unsigned integer value next to the -l argument");
				}
			}
				
		}
		if (processedOptions==0)
		{
			ERR_NOPERROR("Error processing arguments. Please consult --help section.");
			exit(OK);
		}
		
	}
	
	// Manage signals
	signal(SIGINT, interrupt);
	signal(SIGALRM, lose);


	// Set the number within a predefined range multiplied by difficulty
	theNumber = randRange(-10*difficulty, (difficulty==0 ? 10 : 10*difficulty));
	VERBOSE("Range is : [ %d, %d ]",
		-10*difficulty, (difficulty==0 ? 10 : 10*difficulty));

	//_GUESSLOOP_
	VERBOSE("Game started!");
	alarm(timer);
	while(true){
		if(limit>0){VERBOSE("(%d left...)", limit);}
		VERBOSE("\nType a number: ");

		if(getCommand(&clientChoice))
		{
			// If it is a win
			if(testNumber(clientChoice)==WIN){
				VERBOSE("\n .::! CONGRATULATIONS !::. ");
				VERBOSE("\n You won! \n\n");
				exit(OK);
			}
			// If it is higher
			else if(testNumber(clientChoice)==HIGHER){
				VERBOSE("Try with a higher value.");
			}
			// If it is lower
			else if(testNumber(clientChoice)==LOWER){
				VERBOSE("Try with a lower value.");
			}
			limit--;
		}
		if (limit==0) break;
	}

	
	//_END_
	lose(-1);
	return 0;
}