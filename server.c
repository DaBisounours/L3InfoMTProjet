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
#include <signal.h>
#include <time.h>
/// Personal
#include "globalconstants.h"

///
/// GLOBAL VALUES
///

/// List of players
clientInfo playersList[MAX_PLAYERS];

/// Mask of availability for the players slots, 0 for Free and 1 if there is a
/// Client
int playersListMask[MAX_PLAYERS];

/// File descriptor for Connection Pipe
int connectionPipe;

/// Tells if game is started or not
bool gameIsOn = false;

/// Mutex
pthread_mutex_t number_mutex;
/// The magic number
int theNumber;

/// Level of difficulty
int difficulty=DIFFICULTY;


//
// FUNCTIONS
//

// Init players list
void list_initPlayers(){
	int i;
	// Set all player slots available
	for(i=0;i<MAX_PLAYERS; i++) 
		playersListMask[i]=false;
}

// Add a player to the game
bool list_addPlayer(clientInfo client){
	int i=0;
	// Find an empty slot
	while(playersListMask[i]!=false && i<MAX_PLAYERS){i++;}
	// If we can't find one, return
	if(i==MAX_PLAYERS) return false;
	// Else add a player on the free slot
	else
	{
		playersListMask[i]=true;
		playersList[i]=client;
		return true;
	}
}

// Delete a player from the game
bool list_delPlayer(clientInfo client){
	int i=0;
	// Find the player
	while(playersList[i].pid != client.pid && i < MAX_PLAYERS){i++;}
	// If player not found return
	if(i==MAX_PLAYERS) return false;
	// Delete the player if we found it
	else {
		playersListMask[i]=false;
		return true;
	}
}

// Searching a player
// ...
/// Searching for a player by pid
bool list_getPlayerByPid(pid_t pid, clientInfo *dest_client)
{
	int i=0;
	/// Find the player
	while(playersList[i].pid != pid && i < MAX_PLAYERS){i++;}	
	/// Return a boolean telling if it is in
	if(i==MAX_PLAYERS) return false;
	else {
		/// Update the output clientInfo
		strncpy(dest_client->name, playersList[i].name,
			MAX_CLIENT_NAME_LENGTH);
		dest_client->pid=playersList[i].pid;
		strncpy(dest_client->pipeName, playersList[i].pipeName,
			MAX_NAMED_PIPE_NAME_LENGTH);
		return true;
	}
}

//...
/// Searching for a player by name. If multiple players have the same name,
/// Then another call will output the next one
/// Until the function returns false when no other has been found
/// An other call will then start from the beginning
bool list_getPlayerByName(char *name, clientInfo *dest_client)
{
	static int i=0;

	/// Find the player
	while(i < MAX_PLAYERS)
	{
		if (!strcmp(name, playersList[i].name) && playersListMask[i]==true) 
			break; 
		i++;
	}	
	/// Return a boolean telling if it is in
	if(i==MAX_PLAYERS) 
	{
		i=0;
		return false;
	}
	else 
	{
		/// Update the output clientInfo
		strncpy(dest_client->name, playersList[i].name,
			MAX_CLIENT_NAME_LENGTH);
		dest_client->pid=playersList[i].pid;
		strncpy(dest_client->pipeName, playersList[i].pipeName,
			MAX_NAMED_PIPE_NAME_LENGTH);
		return true;
	}
}

// Return a boolean telling if the string is numeric
bool isNumeric(char *s)
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

/// Displays client Information
void verboseClientInfo(clientInfo client){
	VERBOSE("Client Information : \n\tName: %s\n"
						"\tPID: %d\n"
						"\tPipe Name: %s\n", 
						client.name, client.pid, client.pipeName);
}

// Procedure forking a child process to behave like a client
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

// Procedure creating the connection pipe
void createConnectionPipe(){
	// If unable to create the fifo file
	if (mkfifo(CONNECTION_NAMED_PIPE_NAME, 700)==-1)
	{
		// Try unlink it
		// If unable to unlink
		if (unlink(CONNECTION_NAMED_PIPE_NAME)==-1)
		{
			// Exit with error message
			ERR("[SERVER] "ERR_NAMED_PIPE_CREATION_FAIL_MSG);
			exit(ERR_NAMED_PIPE_CREATION_FAIL);
		}
		// If unlinked but unable to create it again
		if(mkfifo(CONNECTION_NAMED_PIPE_NAME, 700)==-1){
			// Exit with error message
			ERR("[SERVER] "ERR_NAMED_PIPE_CREATION_FAIL_MSG);
			exit(ERR_NAMED_PIPE_CREATION_FAIL);
		}
	}
}



// Procedure to open 'safely' a file
int safeOpen(char *pathname, int mode)
{
	int file;
	// Open the file
	// If unable to open file
	if ((file=open(pathname, mode))==-1)
	{
		// Exit with error message
		ERR("[SERVER] "ERR_OPEN_FAIL_MSG);
		exit(ERR_OPEN_FAIL);
	}
	// Else return file descriptor
	return file;
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

// Initialise a game 
void initGame(){
	int i;

	/// Lock the number
	pthread_mutex_lock(&number_mutex);

	// Set the number within a predefined range multiplied by difficulty
	theNumber = randRange(-10*DIFFICULTY, (DIFFICULTY==0 ? 10 : 10*DIFFICULTY));

	// Start the game
	gameIsOn=true;
	DEBUG("[SERVER] The number: %d", theNumber);

	/// Unlock the number
	pthread_mutex_unlock(&number_mutex);

	// Tell all the players that the game started
	for(i=0; i<MAX_PLAYERS; i++)
	{
		if(playersListMask[i]==true)
		{
			kill(playersList[i].pid, SIG_START);
		}
	}

	// Start timer
	alarm(BASE_TIMEOUT/ (difficulty<1? 1 : difficulty) + 10);
}


/// Function to test number
int testNumber(int number)
{
	int dir, shift;
	pthread_mutex_lock(&number_mutex);

		/// Check
		if (theNumber == number) dir=WIN;
		else if(theNumber > number) dir = HIGHER;
		else dir=LOWER;

		/// Shift
		while((shift=randRange(-1,1))==0);
		theNumber+=shift;
		DEBUG("[SERVER] The number: %d, the shift: %d", theNumber, shift);

	pthread_mutex_unlock(&number_mutex);

	return dir;
}

/// Kicks a player from the server
void kickPlayer(clientInfo client, int reason)
{
	int communicationPipe;
	serverMessage messageFromServer;
	int flags;


	messageFromServer.type=KICK;
	messageFromServer.choice=reason;

	communicationPipe=safeOpen(client.pipeName, O_WRONLY);

  	flags = fcntl(communicationPipe, F_GETFL, 0);
  	flags &= ~O_NONBLOCK;
    fcntl(communicationPipe, F_SETFL, flags);


	kill(client.pid, SIGINT);
	usleep(1000);
	write(communicationPipe, &messageFromServer, sizeof(serverMessage));
	usleep(10000);			
	close(communicationPipe);
	list_delPlayer(client);
}

/// Deletes the connextion Pipe
void unlinkConnectionPipe() {
	DEBUG("[SERVER] Unlinking the connection named pipe");
	close(connectionPipe);
	if (unlink(CONNECTION_NAMED_PIPE_NAME)==-1) {
		ERR("[SERVER] Did not manage to unlink connection named pipe");
	}
}

/// Shuts down the server on SIGINT
void shutDown(int signal)
{
	VERBOSE("\nShutting down server.");
	int i;
	for(i=0; i<MAX_PLAYERS; i++)
	{
		if(playersListMask[i]==true){
			// Force disconnecting all the clients
			kickPlayer(playersList[i], REASON_SERVER_INTERRUPTION);
		}
	}
	/// Resert alarm
	alarm(0);
	unlinkConnectionPipe();
	exit(INTERRUPTED);
}



void endGame(int sig){
	VERBOSE("\nTime is out!");
	int i;
	for(i=0; i<MAX_PLAYERS; i++)
	{
		if(playersListMask[i]==true && playersList[i].pid!=sig){
			// Tell all the clients that they have lost
			kill(playersList[i].pid, SIG_STOP);
		}
	}
	gameIsOn=false;
	/// Resert alarm
	alarm(0);

}


/// Gets a command from server terminal
void getCommand(){
	
	int flags = fcntl(0, F_GETFL, 0);
	int i;
	clientInfo client;

	fcntl(0, F_SETFL, flags | O_NONBLOCK);
	
	
	char command[200];
	char arg[200];
	// If a command has been typed on the terminal
	if(read(0,command,100)>0) 
	{
		i=0;while(command[i] != '\n')i++;command[i+1]=0;

		DEBUG("[SERVER] Command : '%s'",command);
		if(!strncmp(command, "quit\n", 5) 
		|| !strncmp(command, "q\n", 2) 
		|| !strncmp(command, "exit\n", 5))
			shutDown(0);
		/// KICK COMMAND
		else if(!strncmp(command, "kick ", 5) && command[5] != '\n')
		{
			/// Separating argument
			strcpy(arg, &(command[5]));
			arg[strlen(arg)-1]=0;
			DEBUG("[SERVER] Kick: '%s'",arg);
			if (isNumeric(arg)) 
			{
				if(list_getPlayerByPid(atoi(arg), &client)) 
				{
					VERBOSE("%s has been kicked.", client.name);
					kickPlayer(client, REASON_SERVER_KICK);
				}
			}
			else
			{
				
				if(list_getPlayerByName(arg, &client))
				{
					VERBOSE("%s has been kicked.", client.name);
					kickPlayer(client, REASON_SERVER_KICK);
				}
			}
		}
		/// INFO COMMAND
		else if(!strncmp(command, "info ", 5) && command[5] != '\n')
		{
			/// Separating argument
			strcpy(arg, &(command[5]));
			arg[strlen(arg)-1]=0;

			DEBUG("[SERVER] Info: '%s'",arg);
			if (isNumeric(arg)) 
			{
				if(list_getPlayerByPid(atoi(arg), &client)) 
					verboseClientInfo(client);			
			}
			else
			{
				if(list_getPlayerByName(arg, &client))
					verboseClientInfo(client);
			}
		}
		/// START COMMAND
		else if(!strncmp(command, "start\n", 6) && command[6] != '\n')
		{
			initGame();
			VERBOSE("Game started!");
		}
		/// START COMMAND
		else if(gameIsOn && !strncmp(command, "stop\n", 5) && command[5] != '\n')
		{
			endGame(0);
			VERBOSE("Game stopped!");
		}
		/// UNKNOWN COMMAND
		else
		{
			VERBOSE("Unknown command.");
		}
	}	
}





/// Thread main
void *newConnection(void *arg){
	int communicationPipe;
	clientMessage messageFromClient;
	serverMessage messageFromServer;
	clientInfo *p_client = (clientInfo *) arg;
	clientInfo client;
	int highOrLow;

	client.pid=p_client->pid;
	strcpy(client.name,p_client->name);
	strcpy(client.pipeName,p_client->pipeName);

	DEBUG("[SERVER: THREAD %s] Thread created for %s (%d)", 
		client.name,client.name,(int)client.pid);
	
	VERBOSE("%s is connected.",client.name);
	communicationPipe=safeOpen(client.pipeName, O_RDWR);

	// Sending connection confirmation telling if game is on or not
	messageFromServer.type=ACCEPT;
	if(gameIsOn) messageFromServer.choice=GAME;
	else messageFromServer.choice=GAME_NOT_ON;
	write(communicationPipe, &messageFromServer, sizeof(serverMessage));

	while(true)
	{
		usleep(200000);
		read(communicationPipe, &messageFromClient, sizeof(clientMessage));
		DEBUG("[SERVER: THREAD %s] Received from %s:\n\t\t"
			  			"type=%d\n\t\tchoice=%d",  
		client.name,client.name,
		messageFromClient.type,messageFromClient.choice);
		
		if(messageFromClient.type==QUIT)
		{
			VERBOSE("%s has left the game.", client.name);
			break;
		}
		else if(messageFromClient.type==GUESS && gameIsOn)
		{
			messageFromServer.type=GAME;
			VERBOSE("%s has submitted %d", client.name, messageFromClient.choice);
			if((highOrLow=testNumber(messageFromClient.choice))==WIN){
				//WON
				messageFromServer.choice=WIN;
				endGame(client.pid);
				//TODO TELL EVERYONE WHO WON ???
			}
			else if(highOrLow == HIGHER)
				messageFromServer.choice=HIGHER;
			else
				messageFromServer.choice=LOWER;

			/// Sending message
			write(communicationPipe, &messageFromServer, 
					sizeof(serverMessage));
		}
		else if (!gameIsOn)
		{
			messageFromServer.type=GAME;
		}
	}
	list_delPlayer(client);
	alarm(0);
	pthread_exit(OK);
}

///
/// MAIN
///
int main(int argc, char const *argv[])
{
	clientInfo newClient;
	pthread_t thread;

	//_INITIALISATION_

	/// Managing sigint
	signal(SIGINT, shutDown);
	/// Managing sigalarm
	signal(SIGALRM, endGame);


	/// Randomisation srand initialisation
	srand(time(NULL));

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
	connectionPipe = safeOpen(CONNECTION_NAMED_PIPE_NAME, O_RDONLY);


	//_LISTENLOOP_
	VERBOSE("Server is ready to accept new players.");

	list_initPlayers();

	while(true){
		usleep(10000);
	
		getCommand();

		// If a client has sent connection information
		if((read(connectionPipe, &newClient, sizeof(clientInfo))>0))
		{
			// Update the list of clients
			if(list_addPlayer(newClient)){
				//_NEW THREAD FOR CLIENT_
				pthread_create(&thread, NULL, newConnection, &newClient);
			}
			else
			{
				ERR_NOPERROR("Could not accept new player %s: Room full.", newClient.name);
				kickPlayer(newClient, REASON_ROOM_FULL);
			}
		}	
	}

	//_END_

	return 0;
}